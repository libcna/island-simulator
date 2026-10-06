import { spawn } from "node:child_process";
import { readFile, writeFile, mkdtemp, rm } from "node:fs/promises";
import { tmpdir } from "node:os";
import path from "node:path";
import { fileURLToPath } from "node:url";

const toolsDirectory = path.dirname(fileURLToPath(import.meta.url));
const projectRoot = path.resolve(toolsDirectory, "..");
const buildDirectory = path.resolve(process.argv[2] ?? path.join(projectRoot, "build-probe"));
const port = Number(process.env.ISLAND_WEB_SMOKE_PORT ?? "18764");
const chromeExecutable = process.env.ISLAND_CHROME ?? "google-chrome";
const scriptPath = path.join(buildDirectory, "island-simulator.js");
const wasmPath = path.join(buildDirectory, "island-simulator.wasm");

await Promise.all([readFile(scriptPath), readFile(wasmPath)]);
const template = await readFile(path.join(toolsDirectory, "web_smoke.html.in"), "utf8");
await writeFile(path.join(buildDirectory, "island-smoke.html"), template);

const profile = await mkdtemp(path.join(tmpdir(), "island-web-smoke-"));
const server = spawn("python3", ["-m", "http.server", String(port), "--bind", "127.0.0.1",
  "--directory", buildDirectory], { stdio: ["ignore", "ignore", "pipe"] });
let serverError = "";
server.stderr.on("data", chunk => { serverError += chunk; });

let chrome;
let browserSocket;
const delay = milliseconds => new Promise(resolve => setTimeout(resolve, milliseconds));

try {
  let serverReady = false;
  for (let attempt = 0; attempt < 50; ++attempt) {
    try {
      const response = await fetch(`http://127.0.0.1:${port}/island-smoke.html`);
      if (response.ok) {
        serverReady = true;
        break;
      }
    } catch {
      await delay(100);
    }
  }
  if (!serverReady) {
    throw new Error(`Local smoke server did not start: ${serverError}`);
  }

  chrome = spawn(chromeExecutable, [
    "--headless=new",
    "--no-sandbox",
    "--disable-dev-shm-usage",
    "--no-first-run",
    "--no-default-browser-check",
    "--use-angle=swiftshader",
    "--enable-unsafe-swiftshader",
    `--user-data-dir=${profile}`,
    "--remote-debugging-port=0",
    "about:blank",
  ], { stdio: ["ignore", "ignore", "pipe"] });
  let chromeLog = "";
  chrome.stderr.on("data", chunk => { chromeLog += chunk; });

  let debuggingPort;
  for (let attempt = 0; attempt < 100; ++attempt) {
    try {
      debuggingPort = (await readFile(path.join(profile, "DevToolsActivePort"), "utf8"))
        .split("\n")[0];
      break;
    } catch {
      await delay(100);
    }
  }
  if (!debuggingPort) {
    throw new Error(`Chrome did not expose its debugging port: ${chromeLog}`);
  }

  const version = await (await fetch(`http://127.0.0.1:${debuggingPort}/json/version`)).json();
  browserSocket = new WebSocket(version.webSocketDebuggerUrl);
  await new Promise((resolve, reject) => {
    browserSocket.onopen = resolve;
    browserSocket.onerror = reject;
  });

  let nextId = 0;
  const pending = new Map();
  const consoleMessages = [];
  browserSocket.onmessage = event => {
    const message = JSON.parse(event.data);
    if (message.id) {
      const callback = pending.get(message.id);
      pending.delete(message.id);
      if (message.error) callback?.reject(new Error(JSON.stringify(message.error)));
      else callback?.resolve(message.result);
    } else if (message.method === "Runtime.consoleAPICalled") {
      consoleMessages.push(message.params.args
        .map(argument => argument.value ?? argument.description ?? "").join(" "));
    } else if (message.method === "Runtime.exceptionThrown") {
      consoleMessages.push(message.params.exceptionDetails.exception?.description
        ?? message.params.exceptionDetails.text);
    }
  };
  const call = (method, params = {}, sessionId) => new Promise((resolve, reject) => {
    const id = ++nextId;
    pending.set(id, { resolve, reject });
    browserSocket.send(JSON.stringify({ id, method, params, ...(sessionId ? { sessionId } : {}) }));
  });
  const dispatchKeyPress = async (sessionId, key, code, virtualKeyCode,
    holdMilliseconds = 500, settleMilliseconds = 400) => {
    await call("Input.dispatchKeyEvent", {
      type: "keyDown", key, code, windowsVirtualKeyCode: virtualKeyCode,
      nativeVirtualKeyCode: virtualKeyCode,
    }, sessionId);
    await delay(holdMilliseconds);
    await call("Input.dispatchKeyEvent", {
      type: "keyUp", key, code, windowsVirtualKeyCode: virtualKeyCode,
      nativeVirtualKeyCode: virtualKeyCode,
    }, sessionId);
    await delay(settleMilliseconds);
  };
  const dispatchMouseClick = async (sessionId, x, y) => {
    await call("Input.dispatchMouseEvent", {
      type: "mousePressed", x, y, button: "left", buttons: 1, clickCount: 1,
    }, sessionId);
    // A real click spans game updates. Keep the button down long enough for a heavily loaded
    // SwiftShader frame to observe CNA's pressed state as well as the browser pointer-lock event.
    await delay(500);
    await call("Input.dispatchMouseEvent", {
      type: "mouseReleased", x, y, button: "left", buttons: 0, clickCount: 1,
    }, sessionId);
    await delay(500);
  };
  const readSmokeOutput = async sessionId => {
    const evaluation = await call("Runtime.evaluate", {
      expression: "globalThis.islandSmokeResult?.output ?? ''",
      returnByValue: true,
    }, sessionId);
    return evaluation.result.value ?? "";
  };
  const waitForOutput = async (sessionId, pattern, attempts = 12) => {
    for (let attempt = 0; attempt < attempts; ++attempt) {
      const output = await readSmokeOutput(sessionId);
      if (typeof pattern === "string" ? output.includes(pattern) : pattern.test(output)) {
        return true;
      }
      await delay(250);
    }
    return false;
  };
  const dispatchKeyUntilOutput = async (sessionId, key, code, virtualKeyCode, pattern) => {
    if (await waitForOutput(sessionId, pattern, 1)) return true;
    for (let attempt = 0; attempt < 3; ++attempt) {
      await dispatchKeyPress(sessionId, key, code, virtualKeyCode, 650, 350);
      if (await waitForOutput(sessionId, pattern)) return true;
    }
    return false;
  };

  const { targetId: smokeTargetId } = await call("Target.createTarget", { url: "about:blank" });
  const { sessionId: smokeSessionId } = await call(
    "Target.attachToTarget", { targetId: smokeTargetId, flatten: true });
  await call("Runtime.enable", {}, smokeSessionId);
  await call("Page.enable", {}, smokeSessionId);
  await call("Emulation.setDeviceMetricsOverride", {
    width: 1280, height: 720, deviceScaleFactor: 1, mobile: false,
  }, smokeSessionId);
  await call("Page.navigate", { url: `http://127.0.0.1:${port}/island-smoke.html` },
    smokeSessionId);

  let result = {};
  for (let attempt = 0; attempt < 200; ++attempt) {
    await delay(100);
    const evaluation = await call("Runtime.evaluate", {
      expression: "JSON.stringify(globalThis.islandSmokeResult ?? {})",
      returnByValue: true,
    }, smokeSessionId);
    result = JSON.parse(evaluation.result.value ?? "{}");
    if (result.exited || result.error) break;
  }

  const smokePassed = result.exited === true
    && result.exitCode === 0
    && result.output?.includes("island-simulator: renderer WEBGL2")
    && /island-simulator: startup world [0-9.]+ ms, framework [0-9.]+ ms, content [0-9.]+ ms, pre-draw [0-9.]+ ms, first draw [0-9.]+ ms, total [0-9.]+ ms/.test(
      result.output ?? "")
    && result.output?.includes("island-simulator: terrain portable sand/soil material shader")
    && result.output?.includes(
      "island-simulator: terrain near-field microvariation and procedural interior leaf litter")
    && result.output?.includes(
      "island-simulator: terrain fixed-sun interior dapple from shared portable noise fields")
    && result.output?.includes(
      "island-simulator: terrain authored primary footpath material")
    && result.output?.includes(
      "island-simulator: terrain distinct authored east/west clearing materials")
    && result.output?.includes("island-simulator: ocean portable custom shader")
    && result.output?.includes(
      "island-simulator: fixed sunny sky with layered horizontal high wisps, portable shader")
    && result.output?.includes(
      "island-simulator: distant scenery 5 unreachable low islets, one portable batch")
    && result.output?.includes("island-simulator: palms 330 in 64 cells, hardware instancing")
    && result.output?.includes("palm portable wind shader with near/far mesh LOD")
    && result.output?.includes("palm near mesh segmented leaflet silhouettes")
    && result.output?.includes("palm procedural bark scars and restrained coconut clusters")
    && result.output?.includes("island-simulator: jungle 281 canopy trees, 1822 understory plants, 565 low bushes, hardware instancing")
    && result.output?.includes("island-simulator: jungle ground layer 2225 small plants")
    && result.output?.includes(
      "island-simulator: jungle fixed-sun two-sided geometric facet lighting")
    && result.output?.includes("island-simulator: birds 24 agents")
    && result.output?.includes(
      "island-simulator: bird flight uses slow radial drift, banking, and intermittent glide")
    && result.output?.includes("island-simulator: fish 48 agents")
    && result.output?.includes("island-simulator: one occasional non-hostile offshore shark patrol")
    && result.output?.includes(
      "island-simulator: aquatic wildlife uses depth pitch, banked turns, forked tails, and near head detail")
    && result.output?.includes("island-simulator: monkeys 7 agents with portable GPU skeletal animation")
    && result.output?.includes("3 silhouettes, 4 palettes")
    && result.output?.includes(
      "island-simulator: terrain-conforming contact shadows in one portable near-field batch")
    && result.output?.includes("jungle portable wind shader with canopy/understory/bush/ground LOD")
    && result.output?.includes(
      "jungle near canopy tapered support branches and pointed leaf silhouettes")
    && result.output?.includes(
      "jungle near bushes with pointed leaf edge breakup")
    && result.output?.includes(
      "jungle near ground plants include upright and split-leaf silhouettes")
    && result.output?.includes("island-simulator: natural props 171 rocks, 14 reef outcrops, 12 fallen logs, 28 driftwood pieces, 63 fallen palm fronds, hardware instancing")
    && result.output?.includes(
      "island-simulator: clearing landmarks 5 rocks and 1 fallen log, shared authored placement")
    && result.output?.includes("island-simulator: beach detail 265 shell/pebble clusters, near-field culled")
    && result.output?.includes("island-simulator: near-surface swimming view procedural edge veil")
    && result.output?.includes(
      "island-simulator: movement-driven player ripples integrated into the portable ocean shader")
    && result.output?.includes("natural-prop portable shader with near/far LOD")
    && result.output?.includes(
      "island-simulator: near rocks use procedural strata and restrained lichen flecks")
    && result.output?.includes(
      "island-simulator: near fallen logs use procedural bark grooves and cut-end growth rings")
    && result.output?.includes(
      "island-simulator: near driftwood uses faded sun-bleached longitudinal grain")
    && result.output?.includes(
      "island-simulator: near driftwood variant 2 uses three tapered fork limbs")
    && result.output?.includes("shoreline portable animated foam ribbon, 2695 vertices, 4608 triangles")
    && result.output?.includes("portable instanced wind shader")
    && /birds [1-9][0-9]*\/24 visible/.test(result.output ?? "")
    && result.output?.includes("island-simulator: smoke test rendered 4 frames")
    && !result.error;
  await call("Target.closeTarget", { targetId: smokeTargetId });

  const { targetId: deploymentTargetId } = await call(
    "Target.createTarget", { url: "about:blank" });
  const { sessionId: deploymentSessionId } = await call(
    "Target.attachToTarget", { targetId: deploymentTargetId, flatten: true });
  await call("Runtime.enable", {}, deploymentSessionId);
  await call("Page.enable", {}, deploymentSessionId);
  await call("Emulation.setDeviceMetricsOverride", {
    width: 1280, height: 720, deviceScaleFactor: 1, mobile: false,
  }, deploymentSessionId);
  await call("Page.navigate", {
    url: `http://127.0.0.1:${port}/island-simulator.html`,
  }, deploymentSessionId);
  let deploymentState = {};
  for (let attempt = 0; attempt < 200; ++attempt) {
    await delay(100);
    const evaluation = await call("Runtime.evaluate", {
      expression: `JSON.stringify({
        status: document.getElementById("status")?.textContent ?? "",
        width: Module.canvas?.width ?? 0,
        height: Module.canvas?.height ?? 0,
        hasWebGl2: Module.canvas?.getContext("webgl2") != null,
        output: globalThis.islandDeploymentOutput ?? ""
      })`,
      returnByValue: true,
    }, deploymentSessionId);
    deploymentState = JSON.parse(evaluation.result.value ?? "{}");
    if (deploymentState.status === "Ready"
        && deploymentState.output?.includes("island-simulator: renderer WEBGL2")) break;
  }
  await call("Input.dispatchMouseEvent", {
    type: "mousePressed", x: 640, y: 360, button: "left", buttons: 1, clickCount: 1,
  }, deploymentSessionId);
  await delay(150);
  await call("Input.dispatchMouseEvent", {
    type: "mouseReleased", x: 640, y: 360, button: "left", buttons: 0, clickCount: 1,
  }, deploymentSessionId);
  let postGestureState = {};
  for (let attempt = 0; attempt < 80; ++attempt) {
    await delay(100);
    const evaluation = await call("Runtime.evaluate", {
      expression: `JSON.stringify({
        pointerLocked: document.pointerLockElement === Module.canvas,
        output: globalThis.islandDeploymentOutput ?? ""
      })`,
      returnByValue: true,
    }, deploymentSessionId);
    postGestureState = JSON.parse(evaluation.result.value ?? "{}");
    if (postGestureState.output?.includes("island-simulator: soundscape started")) break;
  }
  deploymentState.pointerLocked = postGestureState.pointerLocked === true;
  deploymentState.output = postGestureState.output ?? deploymentState.output;
  deploymentState.audioStarted = deploymentState.output?.includes(
    "island-simulator: soundscape started (procedural surf and breeze)");
  deploymentState.waveAccentsEnabled = deploymentState.output?.includes(
    "island-simulator: sparse stronger-wave accents enabled");
  deploymentState.waterMovementEnabled = deploymentState.output?.includes(
    "island-simulator: player water-movement layer enabled");
  deploymentState.waterTransitionsEnabled = deploymentState.output?.includes(
    "island-simulator: water-transition accents enabled");
  deploymentState.waterTransitionVariantsEnabled = deploymentState.output?.includes(
    "island-simulator: two water-transition variants enabled");
  deploymentState.jungleAmbienceEnabled = deploymentState.output?.includes(
    "island-simulator: subtle daytime-jungle layer enabled");
  deploymentState.waveVariantsEnabled = deploymentState.output?.includes(
    "island-simulator: two stronger-wave variants enabled");
  deploymentState.positionalWildlifeCallsEnabled = deploymentState.output?.includes(
    "island-simulator: positional bird and monkey calls enabled (one pooled wildlife voice)");
  const deploymentPassed = deploymentState.status === "Ready"
    && deploymentState.width === 1280
    && deploymentState.height === 720
    && deploymentState.hasWebGl2 === true
    && deploymentState.output?.includes("island-simulator: renderer WEBGL2")
    && deploymentState.output?.includes(
      "island-simulator: fixed sunny sky with layered horizontal high wisps, portable shader")
    && deploymentState.output?.includes("island-simulator: terrain")
    && deploymentState.audioStarted === true
    && deploymentState.waveAccentsEnabled === true
    && deploymentState.waterMovementEnabled === true
    && deploymentState.waterTransitionsEnabled === true
    && deploymentState.waterTransitionVariantsEnabled === true
    && deploymentState.jungleAmbienceEnabled === true
    && deploymentState.waveVariantsEnabled === true
    && deploymentState.positionalWildlifeCallsEnabled === true
    && deploymentState.pointerLocked === true;
  await call("Target.closeTarget", { targetId: deploymentTargetId });

  const { targetId: visualTargetId } = await call("Target.createTarget", { url: "about:blank" });
  const { sessionId: visualSessionId } = await call(
    "Target.attachToTarget", { targetId: visualTargetId, flatten: true });
  await call("Runtime.enable", {}, visualSessionId);
  await call("Page.enable", {}, visualSessionId);
  await call("Emulation.setDeviceMetricsOverride", {
    width: 1280, height: 720, deviceScaleFactor: 1, mobile: false,
  }, visualSessionId);
  await call("Page.navigate", {
    url: `http://127.0.0.1:${port}/island-smoke.html?mode=overview`,
  }, visualSessionId);

  let visualResult = {};
  for (let attempt = 0; attempt < 200; ++attempt) {
    await delay(100);
    const evaluation = await call("Runtime.evaluate", {
      expression: "JSON.stringify(globalThis.islandSmokeResult ?? {})",
      returnByValue: true,
    }, visualSessionId);
    visualResult = JSON.parse(evaluation.result.value ?? "{}");
    if (visualResult.output?.includes("island-simulator: terrain") || visualResult.error) break;
  }
  await delay(1000);
  const completedVisualEvaluation = await call("Runtime.evaluate", {
    expression: "JSON.stringify(globalThis.islandSmokeResult ?? {})",
    returnByValue: true,
  }, visualSessionId);
  visualResult = JSON.parse(completedVisualEvaluation.result.value ?? "{}");

  const screenshot = await call("Page.captureScreenshot", { format: "png" }, visualSessionId);
  const screenshotPath = path.join(buildDirectory, "island-browser-smoke.png");
  await writeFile(screenshotPath, Buffer.from(screenshot.data, "base64"));

  const visualPassed = visualResult.output?.includes("island-simulator: renderer WEBGL2")
    && visualResult.output?.includes("island-simulator: ocean portable custom shader")
    && visualResult.output?.includes(
      "island-simulator: fixed sunny sky with layered horizontal high wisps, portable shader")
    && visualResult.output?.includes("island-simulator: palms 330 in 64 cells, hardware instancing")
    && visualResult.output?.includes("palm portable wind shader with near/far mesh LOD")
    && visualResult.output?.includes(
      "island-simulator: jungle 281 canopy trees, 1822 understory plants, 565 low bushes, hardware instancing")
    && visualResult.output?.includes("jungle portable wind shader with canopy/understory/bush/ground LOD")
    && visualResult.output?.includes("natural-prop portable shader with near/far LOD")
    && visualResult.output?.includes(
      "island-simulator: near rocks use procedural strata and restrained lichen flecks")
    && visualResult.output?.includes("portable instanced wind shader")
    && visualResult.output?.includes("island-simulator: terrain")
    && !visualResult.error;

  await call("Page.navigate", {
    url: `http://127.0.0.1:${port}/island-smoke.html?mode=beach`,
  }, visualSessionId);
  let beachResult = {};
  for (let attempt = 0; attempt < 200; ++attempt) {
    await delay(100);
    const evaluation = await call("Runtime.evaluate", {
      expression: "JSON.stringify(globalThis.islandSmokeResult ?? {})",
      returnByValue: true,
    }, visualSessionId);
    beachResult = JSON.parse(evaluation.result.value ?? "{}");
    if (beachResult.output?.includes("island-simulator: terrain") || beachResult.error) break;
  }
  await delay(1000);
  const completedBeachEvaluation = await call("Runtime.evaluate", {
    expression: "JSON.stringify(globalThis.islandSmokeResult ?? {})",
    returnByValue: true,
  }, visualSessionId);
  beachResult = JSON.parse(completedBeachEvaluation.result.value ?? "{}");

  const beachScreenshot = await call("Page.captureScreenshot", { format: "png" }, visualSessionId);
  const beachScreenshotPath = path.join(buildDirectory, "island-browser-beach.png");
  await writeFile(beachScreenshotPath, Buffer.from(beachScreenshot.data, "base64"));
  const beachPassed = beachResult.output?.includes("island-simulator: renderer WEBGL2")
    && beachResult.output?.includes("island-simulator: ocean portable custom shader")
    && beachResult.output?.includes(
      "island-simulator: fixed sunny sky with layered horizontal high wisps, portable shader")
    && beachResult.output?.includes("palm portable wind shader with near/far mesh LOD")
    && beachResult.output?.includes("jungle portable wind shader with canopy/understory/bush/ground LOD")
    && beachResult.output?.includes("portable instanced wind shader")
    && beachResult.output?.includes(
      "island-simulator: grass player interaction integrated into the instanced vertex shader")
    && beachResult.output?.includes(
      "island-simulator: grass five-level near curvature with portable two-sided daylight shading")
    && beachResult.output?.includes(
      "island-simulator: terrain authored primary footpath material")
    && beachResult.output?.includes("island-simulator: debug overlay active")
    && /grass [1-9][0-9]*\/[1-9][0-9]* patches/.test(beachResult.output ?? "")
    && !beachResult.error;

  await call("Page.navigate", {
    url: `http://127.0.0.1:${port}/island-smoke.html?mode=grass`,
  }, visualSessionId);
  let grassResult = {};
  for (let attempt = 0; attempt < 200; ++attempt) {
    await delay(100);
    const evaluation = await call("Runtime.evaluate", {
      expression: "JSON.stringify(globalThis.islandSmokeResult ?? {})",
      returnByValue: true,
    }, visualSessionId);
    grassResult = JSON.parse(evaluation.result.value ?? "{}");
    if (grassResult.output?.includes("island-simulator: terrain") || grassResult.error) break;
  }
  await delay(1000);
  const completedGrassEvaluation = await call("Runtime.evaluate", {
    expression: "JSON.stringify(globalThis.islandSmokeResult ?? {})",
    returnByValue: true,
  }, visualSessionId);
  grassResult = JSON.parse(completedGrassEvaluation.result.value ?? "{}");

  const grassScreenshot = await call("Page.captureScreenshot", { format: "png" }, visualSessionId);
  const grassScreenshotPath = path.join(buildDirectory, "island-browser-grass.png");
  await writeFile(grassScreenshotPath, Buffer.from(grassScreenshot.data, "base64"));
  const grassPassed = grassResult.output?.includes("island-simulator: renderer WEBGL2")
    && grassResult.output?.includes(
      "island-simulator: fixed sunny sky with layered horizontal high wisps, portable shader")
    && grassResult.output?.includes(
      "island-simulator: terrain near-field microvariation and procedural interior leaf litter")
    && grassResult.output?.includes(
      "island-simulator: terrain fixed-sun interior dapple from shared portable noise fields")
    && grassResult.output?.includes(
      "island-simulator: grass player interaction integrated into the instanced vertex shader")
    && grassResult.output?.includes(
      "island-simulator: grass five-level near curvature with portable two-sided daylight shading")
    && /grass [1-9][0-9]*\/18556 patches in [1-9][0-9]* cells, [1-9][0-9]* draws, interaction 1, custom shader 1/.test(
      grassResult.output ?? "")
    && !grassResult.error;

  await call("Page.navigate", {
    url: `http://127.0.0.1:${port}/island-smoke.html?mode=ground`,
  }, visualSessionId);
  let groundResult = {};
  for (let attempt = 0; attempt < 200; ++attempt) {
    await delay(100);
    const evaluation = await call("Runtime.evaluate", {
      expression: "JSON.stringify(globalThis.islandSmokeResult ?? {})",
      returnByValue: true,
    }, visualSessionId);
    groundResult = JSON.parse(evaluation.result.value ?? "{}");
    if (groundResult.output?.includes("island-simulator: terrain") || groundResult.error) break;
  }
  await delay(1000);
  const completedGroundEvaluation = await call("Runtime.evaluate", {
    expression: "JSON.stringify(globalThis.islandSmokeResult ?? {})",
    returnByValue: true,
  }, visualSessionId);
  groundResult = JSON.parse(completedGroundEvaluation.result.value ?? "{}");

  const groundScreenshot = await call("Page.captureScreenshot", { format: "png" }, visualSessionId);
  const groundScreenshotPath = path.join(buildDirectory, "island-browser-ground.png");
  await writeFile(groundScreenshotPath, Buffer.from(groundScreenshot.data, "base64"));
  const groundPassed = groundResult.output?.includes("island-simulator: renderer WEBGL2")
    && groundResult.output?.includes(
      "jungle near ground plants include upright and split-leaf silhouettes")
    && groundResult.output?.includes(
      "island-simulator: ground QA target variant 2 at player-eye detail distance")
    && /[1-9][0-9]*\/2225 ground plants \(near [1-9][0-9]*, far [0-9]+\)/.test(
      groundResult.output ?? "")
    && groundResult.output?.includes("island-simulator: debug overlay active")
    && !groundResult.error;

  await call("Page.navigate", {
    url: `http://127.0.0.1:${port}/island-smoke.html?mode=palms`,
  }, visualSessionId);
  let palmResult = {};
  for (let attempt = 0; attempt < 200; ++attempt) {
    await delay(100);
    const evaluation = await call("Runtime.evaluate", {
      expression: "JSON.stringify(globalThis.islandSmokeResult ?? {})",
      returnByValue: true,
    }, visualSessionId);
    palmResult = JSON.parse(evaluation.result.value ?? "{}");
    if (palmResult.output?.includes("island-simulator: terrain") || palmResult.error) break;
  }
  await delay(1000);
  const completedPalmEvaluation = await call("Runtime.evaluate", {
    expression: "JSON.stringify(globalThis.islandSmokeResult ?? {})",
    returnByValue: true,
  }, visualSessionId);
  palmResult = JSON.parse(completedPalmEvaluation.result.value ?? "{}");

  const palmScreenshot = await call("Page.captureScreenshot", { format: "png" }, visualSessionId);
  const palmScreenshotPath = path.join(buildDirectory, "island-browser-palms.png");
  await writeFile(palmScreenshotPath, Buffer.from(palmScreenshot.data, "base64"));
  const palmPassed = palmResult.output?.includes("island-simulator: renderer WEBGL2")
    && palmResult.output?.includes(
      "island-simulator: fixed sunny sky with layered horizontal high wisps, portable shader")
    && palmResult.output?.includes("palm portable wind shader with near/far mesh LOD")
    && palmResult.output?.includes("palm near mesh segmented leaflet silhouettes")
    && palmResult.output?.includes(
      "palm procedural bark scars and restrained coconut clusters")
    && /palms [1-9][0-9]*\/330 \(near [1-9][0-9]*, far [0-9]+\)/.test(
      palmResult.output ?? "")
    && !palmResult.error;

  await call("Page.navigate", {
    url: `http://127.0.0.1:${port}/island-smoke.html?mode=jungle`,
  }, visualSessionId);
  let jungleResult = {};
  for (let attempt = 0; attempt < 200; ++attempt) {
    await delay(100);
    const evaluation = await call("Runtime.evaluate", {
      expression: "JSON.stringify(globalThis.islandSmokeResult ?? {})",
      returnByValue: true,
    }, visualSessionId);
    jungleResult = JSON.parse(evaluation.result.value ?? "{}");
    if (jungleResult.output?.includes("island-simulator: terrain") || jungleResult.error) break;
  }
  await delay(1000);
  const completedJungleEvaluation = await call("Runtime.evaluate", {
    expression: "JSON.stringify(globalThis.islandSmokeResult ?? {})",
    returnByValue: true,
  }, visualSessionId);
  jungleResult = JSON.parse(completedJungleEvaluation.result.value ?? "{}");

  const jungleScreenshot = await call("Page.captureScreenshot", { format: "png" }, visualSessionId);
  const jungleScreenshotPath = path.join(buildDirectory, "island-browser-jungle.png");
  await writeFile(jungleScreenshotPath, Buffer.from(jungleScreenshot.data, "base64"));
  const junglePassed = jungleResult.output?.includes("island-simulator: renderer WEBGL2")
    && jungleResult.output?.includes(
      "island-simulator: fixed sunny sky with layered horizontal high wisps, portable shader")
    && jungleResult.output?.includes(
      "island-simulator: terrain near-field microvariation and procedural interior leaf litter")
    && jungleResult.output?.includes(
      "island-simulator: terrain fixed-sun interior dapple from shared portable noise fields")
    && jungleResult.output?.includes(
      "island-simulator: terrain authored primary footpath material")
    && jungleResult.output?.includes("jungle portable wind shader with canopy/understory/bush/ground LOD")
    && jungleResult.output?.includes(
      "island-simulator: jungle fixed-sun two-sided geometric facet lighting")
    && jungleResult.output?.includes(
      "jungle near canopy tapered support branches and pointed leaf silhouettes")
    && jungleResult.output?.includes(
      "jungle near bushes with pointed leaf edge breakup")
    && jungleResult.output?.includes(
      "jungle near ground plants include upright and split-leaf silhouettes")
    && /jungle [1-9][0-9]*\/281 canopy \(near [1-9][0-9]*, far [0-9]+\), [1-9][0-9]*\/1822 understory \(near [1-9][0-9]*, far [0-9]+\), [1-9][0-9]*\/565 bushes \(near [1-9][0-9]*, far [0-9]+\), [1-9][0-9]*\/2225 ground plants \(near [1-9][0-9]*, far [0-9]+\)/.test(
      jungleResult.output ?? "")
    && !jungleResult.error;

  const captureClearingView = async (mode, fileName) => {
    await call("Page.navigate", {
      url: `http://127.0.0.1:${port}/island-smoke.html?mode=${mode}`,
    }, visualSessionId);
    let clearingResult = {};
    for (let attempt = 0; attempt < 200; ++attempt) {
      await delay(100);
      const evaluation = await call("Runtime.evaluate", {
        expression: "JSON.stringify(globalThis.islandSmokeResult ?? {})",
        returnByValue: true,
      }, visualSessionId);
      clearingResult = JSON.parse(evaluation.result.value ?? "{}");
      if (clearingResult.output?.includes("island-simulator: terrain")
          || clearingResult.error) break;
    }
    await delay(1000);
    const completedEvaluation = await call("Runtime.evaluate", {
      expression: "JSON.stringify(globalThis.islandSmokeResult ?? {})",
      returnByValue: true,
    }, visualSessionId);
    clearingResult = JSON.parse(completedEvaluation.result.value ?? "{}");
    const clearingScreenshot = await call(
      "Page.captureScreenshot", { format: "png" }, visualSessionId);
    const clearingScreenshotPath = path.join(buildDirectory, fileName);
    await writeFile(clearingScreenshotPath,
      Buffer.from(clearingScreenshot.data, "base64"));
    return { result: clearingResult, screenshotPath: clearingScreenshotPath };
  };

  const eastClearingCapture = await captureClearingView(
    "east", "island-browser-east-clearing.png");
  const eastClearingResult = eastClearingCapture.result;
  const eastClearingPassed = eastClearingResult.output?.includes(
    "island-simulator: renderer WEBGL2")
    && eastClearingResult.output?.includes(
      "island-simulator: terrain distinct authored east/west clearing materials")
    && eastClearingResult.output?.includes(
      "island-simulator: east clearing QA camera at authored centre")
    && eastClearingResult.output?.includes(
      "island-simulator: clearing landmarks 5 rocks and 1 fallen log, shared authored placement")
    && eastClearingResult.output?.includes("island-simulator: debug overlay active")
    && !eastClearingResult.error;

  const westClearingCapture = await captureClearingView(
    "west", "island-browser-west-clearing.png");
  const westClearingResult = westClearingCapture.result;
  const westClearingPassed = westClearingResult.output?.includes(
    "island-simulator: renderer WEBGL2")
    && westClearingResult.output?.includes(
      "island-simulator: terrain distinct authored east/west clearing materials")
    && westClearingResult.output?.includes(
      "island-simulator: west clearing QA camera at authored centre")
    && westClearingResult.output?.includes(
      "island-simulator: clearing landmarks 5 rocks and 1 fallen log, shared authored placement")
    && westClearingResult.output?.includes("island-simulator: debug overlay active")
    && !westClearingResult.error;

  await call("Page.navigate", {
    url: `http://127.0.0.1:${port}/island-smoke.html?mode=props`,
  }, visualSessionId);
  let propResult = {};
  for (let attempt = 0; attempt < 200; ++attempt) {
    await delay(100);
    const evaluation = await call("Runtime.evaluate", {
      expression: "JSON.stringify(globalThis.islandSmokeResult ?? {})",
      returnByValue: true,
    }, visualSessionId);
    propResult = JSON.parse(evaluation.result.value ?? "{}");
    if (propResult.output?.includes("island-simulator: terrain") || propResult.error) break;
  }
  await delay(1000);
  const completedPropEvaluation = await call("Runtime.evaluate", {
    expression: "JSON.stringify(globalThis.islandSmokeResult ?? {})",
    returnByValue: true,
  }, visualSessionId);
  propResult = JSON.parse(completedPropEvaluation.result.value ?? "{}");

  const propScreenshot = await call("Page.captureScreenshot", { format: "png" }, visualSessionId);
  const propScreenshotPath = path.join(buildDirectory, "island-browser-props.png");
  await writeFile(propScreenshotPath, Buffer.from(propScreenshot.data, "base64"));
  const propPassed = propResult.output?.includes("island-simulator: renderer WEBGL2")
    && propResult.output?.includes(
      "island-simulator: fixed sunny sky with layered horizontal high wisps, portable shader")
    && propResult.output?.includes("natural-prop portable shader with near/far LOD")
    && propResult.output?.includes(
      "island-simulator: near fallen logs use procedural bark grooves and cut-end growth rings")
    && propResult.output?.includes("jungle portable wind shader with canopy/understory/bush/ground LOD")
    && /props [0-9]+\/171 rocks, [0-9]+\/14 reef outcrops, [1-9][0-9]*\/12 logs/.test(
      propResult.output ?? "")
    && /contact shadows [1-9][0-9]*, [1-9][0-9]* triangles, 1 draw/.test(
      propResult.output ?? "")
    && !propResult.error;

  await call("Page.navigate", {
    url: `http://127.0.0.1:${port}/island-smoke.html?mode=driftwood`,
  }, visualSessionId);
  let driftwoodResult = {};
  for (let attempt = 0; attempt < 200; ++attempt) {
    await delay(100);
    const evaluation = await call("Runtime.evaluate", {
      expression: "JSON.stringify(globalThis.islandSmokeResult ?? {})",
      returnByValue: true,
    }, visualSessionId);
    driftwoodResult = JSON.parse(evaluation.result.value ?? "{}");
    if (driftwoodResult.output?.includes("island-simulator: terrain")
        || driftwoodResult.error) break;
  }
  await delay(1000);
  const completedDriftwoodEvaluation = await call("Runtime.evaluate", {
    expression: "JSON.stringify(globalThis.islandSmokeResult ?? {})",
    returnByValue: true,
  }, visualSessionId);
  driftwoodResult = JSON.parse(completedDriftwoodEvaluation.result.value ?? "{}");

  const driftwoodScreenshot = await call(
    "Page.captureScreenshot", { format: "png" }, visualSessionId);
  const driftwoodScreenshotPath = path.join(
    buildDirectory, "island-browser-driftwood.png");
  await writeFile(driftwoodScreenshotPath, Buffer.from(driftwoodScreenshot.data, "base64"));
  const driftwoodPassed = driftwoodResult.output?.includes("island-simulator: renderer WEBGL2")
    && driftwoodResult.output?.includes(
      "island-simulator: fixed sunny sky with layered horizontal high wisps, portable shader")
    && driftwoodResult.output?.includes("natural-prop portable shader with near/far LOD")
    && driftwoodResult.output?.includes(
      "island-simulator: near driftwood uses faded sun-bleached longitudinal grain")
    && driftwoodResult.output?.includes(
      "island-simulator: near driftwood variant 2 uses three tapered fork limbs")
    && /props [0-9]+\/171 rocks, [0-9]+\/14 reef outcrops, [0-9]+\/12 logs, [1-9][0-9]*\/28 driftwood \(near [1-9][0-9]*, far [0-9]+\), [1-9][0-9]*\/265 shell\/pebble clusters/.test(
      driftwoodResult.output ?? "")
    && /driftwood QA per-instance near LOD [1-9][0-9]* pieces/.test(
      driftwoodResult.output ?? "")
    && !driftwoodResult.error;

  await call("Page.navigate", {
    url: `http://127.0.0.1:${port}/island-smoke.html?mode=reef`,
  }, visualSessionId);
  let reefResult = {};
  for (let attempt = 0; attempt < 200; ++attempt) {
    await delay(100);
    const evaluation = await call("Runtime.evaluate", {
      expression: "JSON.stringify(globalThis.islandSmokeResult ?? {})",
      returnByValue: true,
    }, visualSessionId);
    reefResult = JSON.parse(evaluation.result.value ?? "{}");
    if (reefResult.output?.includes("island-simulator: terrain")
        || reefResult.error) break;
  }
  await delay(1000);
  const completedReefEvaluation = await call("Runtime.evaluate", {
    expression: "JSON.stringify(globalThis.islandSmokeResult ?? {})",
    returnByValue: true,
  }, visualSessionId);
  reefResult = JSON.parse(completedReefEvaluation.result.value ?? "{}");

  const reefScreenshot = await call(
    "Page.captureScreenshot", { format: "png" }, visualSessionId);
  const reefScreenshotPath = path.join(buildDirectory, "island-browser-reef.png");
  await writeFile(reefScreenshotPath, Buffer.from(reefScreenshot.data, "base64"));
  const reefPassed = reefResult.output?.includes("island-simulator: renderer WEBGL2")
    && reefResult.output?.includes(
      "island-simulator: fixed sunny sky with layered horizontal high wisps, portable shader")
    && reefResult.output?.includes("natural-prop portable shader with near/far LOD")
    && reefResult.output?.includes(
      "island-simulator: near rocks use procedural strata and restrained lichen flecks")
    && /[1-9][0-9]*\/14 reef outcrops/.test(reefResult.output ?? "")
    && !reefResult.error;

  await call("Page.navigate", {
    url: `http://127.0.0.1:${port}/island-smoke.html?mode=water`,
  }, visualSessionId);
  let waterResult = {};
  for (let attempt = 0; attempt < 200; ++attempt) {
    await delay(100);
    const evaluation = await call("Runtime.evaluate", {
      expression: "JSON.stringify(globalThis.islandSmokeResult ?? {})",
      returnByValue: true,
    }, visualSessionId);
    waterResult = JSON.parse(evaluation.result.value ?? "{}");
    if (waterResult.output?.includes("island-simulator: terrain")
        || waterResult.error) break;
  }
  await delay(1000);
  const completedWaterEvaluation = await call("Runtime.evaluate", {
    expression: "JSON.stringify(globalThis.islandSmokeResult ?? {})",
    returnByValue: true,
  }, visualSessionId);
  waterResult = JSON.parse(completedWaterEvaluation.result.value ?? "{}");

  const waterScreenshot = await call(
    "Page.captureScreenshot", { format: "png" }, visualSessionId);
  const waterScreenshotPath = path.join(buildDirectory, "island-browser-water.png");
  await writeFile(waterScreenshotPath, Buffer.from(waterScreenshot.data, "base64"));
  const waterPassed = waterResult.output?.includes("island-simulator: renderer WEBGL2")
    && waterResult.output?.includes(
      "island-simulator: fixed sunny sky with layered horizontal high wisps, portable shader")
    && waterResult.output?.includes(
      "island-simulator: near-surface swimming view procedural edge veil")
    && /water view 0[.][1-9][0-9]*, 1 draw, player ripple 0[.][1-9][0-9]*, phase [01](?:[.][0-9]+)?/.test(
      waterResult.output ?? "")
    && !waterResult.error;

  await call("Page.navigate", {
    url: `http://127.0.0.1:${port}/island-smoke.html?mode=current`,
  }, visualSessionId);
  let currentResult = {};
  for (let attempt = 0; attempt < 200; ++attempt) {
    await delay(100);
    const evaluation = await call("Runtime.evaluate", {
      expression: "JSON.stringify(globalThis.islandSmokeResult ?? {})",
      returnByValue: true,
    }, visualSessionId);
    currentResult = JSON.parse(evaluation.result.value ?? "{}");
    if (currentResult.output?.includes("island-simulator: terrain")
        || currentResult.error) break;
  }
  await delay(1000);
  const completedCurrentEvaluation = await call("Runtime.evaluate", {
    expression: "JSON.stringify(globalThis.islandSmokeResult ?? {})",
    returnByValue: true,
  }, visualSessionId);
  currentResult = JSON.parse(completedCurrentEvaluation.result.value ?? "{}");

  const currentScreenshot = await call(
    "Page.captureScreenshot", { format: "png" }, visualSessionId);
  const currentScreenshotPath = path.join(buildDirectory, "island-browser-current.png");
  await writeFile(currentScreenshotPath, Buffer.from(currentScreenshot.data, "base64"));
  const currentPassed = currentResult.output?.includes("island-simulator: renderer WEBGL2")
    && currentResult.output?.includes(
      "island-simulator: fixed sunny sky with layered horizontal high wisps, portable shader")
    && currentResult.output?.includes(
      "island-simulator: near-surface swimming view procedural edge veil")
    && /water view 0[.][1-9][0-9]*, 1 draw, player ripple [01](?:[.][0-9]+)?, phase [01](?:[.][0-9]+)?, current 0[.][1-9][0-9]*/.test(
      currentResult.output ?? "")
    && /undertow target 0[.][1-9][0-9]*/.test(currentResult.output ?? "")
    && !currentResult.error;

  await call("Page.navigate", {
    url: `http://127.0.0.1:${port}/island-smoke.html?mode=distant`,
  }, visualSessionId);
  let distantResult = {};
  for (let attempt = 0; attempt < 200; ++attempt) {
    await delay(100);
    const evaluation = await call("Runtime.evaluate", {
      expression: "JSON.stringify(globalThis.islandSmokeResult ?? {})",
      returnByValue: true,
    }, visualSessionId);
    distantResult = JSON.parse(evaluation.result.value ?? "{}");
    if (distantResult.output?.includes("island-simulator: terrain")
        || distantResult.error) break;
  }
  await delay(1000);
  const completedDistantEvaluation = await call("Runtime.evaluate", {
    expression: "JSON.stringify(globalThis.islandSmokeResult ?? {})",
    returnByValue: true,
  }, visualSessionId);
  distantResult = JSON.parse(completedDistantEvaluation.result.value ?? "{}");

  const distantScreenshot = await call(
    "Page.captureScreenshot", { format: "png" }, visualSessionId);
  const distantScreenshotPath = path.join(buildDirectory, "island-browser-distant.png");
  await writeFile(distantScreenshotPath, Buffer.from(distantScreenshot.data, "base64"));
  const distantPassed = distantResult.output?.includes("island-simulator: renderer WEBGL2")
    && distantResult.output?.includes(
      "island-simulator: fixed sunny sky with layered horizontal high wisps, portable shader")
    && distantResult.output?.includes(
      "island-simulator: distant scenery 5 unreachable low islets, one portable batch")
    && /distant 5 islets, 1 draw/.test(distantResult.output ?? "")
    && !distantResult.error;

  await call("Page.navigate", {
    url: `http://127.0.0.1:${port}/island-smoke.html?mode=fronds`,
  }, visualSessionId);
  let frondResult = {};
  for (let attempt = 0; attempt < 200; ++attempt) {
    await delay(100);
    const evaluation = await call("Runtime.evaluate", {
      expression: "JSON.stringify(globalThis.islandSmokeResult ?? {})",
      returnByValue: true,
    }, visualSessionId);
    frondResult = JSON.parse(evaluation.result.value ?? "{}");
    if (frondResult.output?.includes("island-simulator: terrain")
        || frondResult.error) break;
  }
  await delay(1000);
  const completedFrondEvaluation = await call("Runtime.evaluate", {
    expression: "JSON.stringify(globalThis.islandSmokeResult ?? {})",
    returnByValue: true,
  }, visualSessionId);
  frondResult = JSON.parse(completedFrondEvaluation.result.value ?? "{}");

  const frondScreenshot = await call(
    "Page.captureScreenshot", { format: "png" }, visualSessionId);
  const frondScreenshotPath = path.join(buildDirectory, "island-browser-fronds.png");
  await writeFile(frondScreenshotPath, Buffer.from(frondScreenshot.data, "base64"));
  const frondPassed = frondResult.output?.includes("island-simulator: renderer WEBGL2")
    && frondResult.output?.includes(
      "island-simulator: fixed sunny sky with layered horizontal high wisps, portable shader")
    && frondResult.output?.includes("natural-prop portable shader with near/far LOD")
    && /[1-9][0-9]*\/63 fallen fronds/.test(frondResult.output ?? "")
    && !frondResult.error;

  await call("Page.navigate", {
    url: `http://127.0.0.1:${port}/island-smoke.html?mode=birds`,
  }, visualSessionId);
  let birdResult = {};
  for (let attempt = 0; attempt < 200; ++attempt) {
    await delay(100);
    const evaluation = await call("Runtime.evaluate", {
      expression: "JSON.stringify(globalThis.islandSmokeResult ?? {})",
      returnByValue: true,
    }, visualSessionId);
    birdResult = JSON.parse(evaluation.result.value ?? "{}");
    if (birdResult.output?.includes("island-simulator: terrain") || birdResult.error) break;
  }
  await delay(1000);
  const completedBirdEvaluation = await call("Runtime.evaluate", {
    expression: "JSON.stringify(globalThis.islandSmokeResult ?? {})",
    returnByValue: true,
  }, visualSessionId);
  birdResult = JSON.parse(completedBirdEvaluation.result.value ?? "{}");

  const birdScreenshot = await call(
    "Page.captureScreenshot", { format: "png" }, visualSessionId);
  const birdScreenshotPath = path.join(buildDirectory, "island-browser-birds.png");
  await writeFile(birdScreenshotPath, Buffer.from(birdScreenshot.data, "base64"));
  const birdPassed = birdResult.output?.includes("island-simulator: renderer WEBGL2")
    && birdResult.output?.includes(
      "island-simulator: fixed sunny sky with layered horizontal high wisps, portable shader")
    && birdResult.output?.includes("island-simulator: birds 24 agents")
    && birdResult.output?.includes(
      "island-simulator: bird flight uses slow radial drift, banking, and intermittent glide")
    && /birds [1-9][0-9]*\/24 visible/.test(birdResult.output ?? "")
    && !birdResult.error;

  await call("Page.navigate", {
    url: `http://127.0.0.1:${port}/island-smoke.html?mode=fish`,
  }, visualSessionId);
  let fishResult = {};
  for (let attempt = 0; attempt < 200; ++attempt) {
    await delay(100);
    const evaluation = await call("Runtime.evaluate", {
      expression: "JSON.stringify(globalThis.islandSmokeResult ?? {})",
      returnByValue: true,
    }, visualSessionId);
    fishResult = JSON.parse(evaluation.result.value ?? "{}");
    if (fishResult.output?.includes("island-simulator: terrain") || fishResult.error) break;
  }
  await delay(1000);
  const completedFishEvaluation = await call("Runtime.evaluate", {
    expression: "JSON.stringify(globalThis.islandSmokeResult ?? {})",
    returnByValue: true,
  }, visualSessionId);
  fishResult = JSON.parse(completedFishEvaluation.result.value ?? "{}");

  const fishScreenshot = await call(
    "Page.captureScreenshot", { format: "png" }, visualSessionId);
  const fishScreenshotPath = path.join(buildDirectory, "island-browser-fish.png");
  await writeFile(fishScreenshotPath, Buffer.from(fishScreenshot.data, "base64"));
  const fishPassed = fishResult.output?.includes("island-simulator: renderer WEBGL2")
    && fishResult.output?.includes(
      "island-simulator: fixed sunny sky with layered horizontal high wisps, portable shader")
    && fishResult.output?.includes("island-simulator: fish 48 agents")
    && fishResult.output?.includes(
      "island-simulator: aquatic wildlife uses depth pitch, banked turns, forked tails, and near head detail")
    && /fish [1-9][0-9]*\/48 visible/.test(fishResult.output ?? "")
    && !fishResult.error;

  await call("Page.navigate", {
    url: `http://127.0.0.1:${port}/island-smoke.html?mode=shark`,
  }, visualSessionId);
  let sharkResult = {};
  for (let attempt = 0; attempt < 200; ++attempt) {
    await delay(100);
    const evaluation = await call("Runtime.evaluate", {
      expression: "JSON.stringify(globalThis.islandSmokeResult ?? {})",
      returnByValue: true,
    }, visualSessionId);
    sharkResult = JSON.parse(evaluation.result.value ?? "{}");
    if (sharkResult.output?.includes("island-simulator: terrain") || sharkResult.error) break;
  }
  await delay(1000);
  const completedSharkEvaluation = await call("Runtime.evaluate", {
    expression: "JSON.stringify(globalThis.islandSmokeResult ?? {})",
    returnByValue: true,
  }, visualSessionId);
  sharkResult = JSON.parse(completedSharkEvaluation.result.value ?? "{}");

  const sharkScreenshot = await call(
    "Page.captureScreenshot", { format: "png" }, visualSessionId);
  const sharkScreenshotPath = path.join(buildDirectory, "island-browser-shark.png");
  await writeFile(sharkScreenshotPath, Buffer.from(sharkScreenshot.data, "base64"));
  const sharkPassed = sharkResult.output?.includes("island-simulator: renderer WEBGL2")
    && sharkResult.output?.includes(
      "island-simulator: fixed sunny sky with layered horizontal high wisps, portable shader")
    && sharkResult.output?.includes("island-simulator: one occasional non-hostile offshore shark patrol")
    && sharkResult.output?.includes(
      "island-simulator: aquatic wildlife uses depth pitch, banked turns, forked tails, and near head detail")
    && /shark visible activity/.test(sharkResult.output ?? "")
    && !sharkResult.error;

  await call("Page.navigate", {
    url: `http://127.0.0.1:${port}/island-smoke.html?mode=monkeys`,
  }, visualSessionId);
  let monkeyResult = {};
  for (let attempt = 0; attempt < 200; ++attempt) {
    await delay(100);
    const evaluation = await call("Runtime.evaluate", {
      expression: "JSON.stringify(globalThis.islandSmokeResult ?? {})",
      returnByValue: true,
    }, visualSessionId);
    monkeyResult = JSON.parse(evaluation.result.value ?? "{}");
    if (monkeyResult.output?.includes("island-simulator: terrain") || monkeyResult.error) break;
  }
  await delay(1000);
  const completedMonkeyEvaluation = await call("Runtime.evaluate", {
    expression: "JSON.stringify(globalThis.islandSmokeResult ?? {})",
    returnByValue: true,
  }, visualSessionId);
  monkeyResult = JSON.parse(completedMonkeyEvaluation.result.value ?? "{}");

  const monkeyScreenshot = await call(
    "Page.captureScreenshot", { format: "png" }, visualSessionId);
  const monkeyScreenshotPath = path.join(buildDirectory, "island-browser-monkeys.png");
  await writeFile(monkeyScreenshotPath, Buffer.from(monkeyScreenshot.data, "base64"));
  const monkeyPassed = monkeyResult.output?.includes("island-simulator: renderer WEBGL2")
    && monkeyResult.output?.includes(
      "island-simulator: fixed sunny sky with layered horizontal high wisps, portable shader")
    && monkeyResult.output?.includes("island-simulator: monkeys 7 agents with portable GPU skeletal animation")
    && monkeyResult.output?.includes("3 silhouettes, 4 palettes")
    && /monkeys [1-9]\/7 visible/.test(monkeyResult.output ?? "")
    && /blends [1-9][0-9]*\/0 active\/completed/.test(monkeyResult.output ?? "")
    && !monkeyResult.error;

  await call("Emulation.setDeviceMetricsOverride", {
    width: 1280, height: 720, deviceScaleFactor: 1, mobile: false,
  }, visualSessionId);
  await call("Page.navigate", {
    url: `http://127.0.0.1:${port}/island-smoke.html?mode=input`,
  }, visualSessionId);
  let inputResult = {};
  for (let attempt = 0; attempt < 200; ++attempt) {
    await delay(100);
    const evaluation = await call("Runtime.evaluate", {
      expression: "JSON.stringify(globalThis.islandSmokeResult ?? {})",
      returnByValue: true,
    }, visualSessionId);
    inputResult = JSON.parse(evaluation.result.value ?? "{}");
    if (inputResult.output?.includes("island-simulator: input sample") || inputResult.error) break;
  }

  await dispatchMouseClick(visualSessionId, 640, 360);
  const lockedEvaluation = await call("Runtime.evaluate", {
    expression: `JSON.stringify({
      pointerLocked: document.pointerLockElement === Module.canvas,
      focused: document.activeElement === Module.canvas,
      width: Module.canvas.width,
      height: Module.canvas.height
    })`,
    returnByValue: true,
  }, visualSessionId);
  const lockedState = JSON.parse(lockedEvaluation.result.value ?? "{}");

  await call("Input.dispatchKeyEvent", {
    type: "keyDown", key: "w", code: "KeyW", windowsVirtualKeyCode: 87,
    nativeVirtualKeyCode: 87,
  }, visualSessionId);
  await delay(1000);
  await call("Input.dispatchKeyEvent", {
    type: "keyUp", key: "w", code: "KeyW", windowsVirtualKeyCode: 87,
    nativeVirtualKeyCode: 87,
  }, visualSessionId);
  await dispatchKeyUntilOutput(
    visualSessionId, "b", "KeyB", 66, "island-simulator: camera motion disabled");
  await dispatchKeyUntilOutput(
    visualSessionId, "b", "KeyB", 66, "island-simulator: camera motion enabled");
  await dispatchKeyUntilOutput(
    visualSessionId, "F5", "F5", 116, "island-simulator: debug teleport south beach");
  await call("Input.dispatchKeyEvent", {
    type: "keyDown", key: "s", code: "KeyS", windowsVirtualKeyCode: 83,
    nativeVirtualKeyCode: 83,
  }, visualSessionId);
  for (let attempt = 0; attempt < 15; ++attempt) {
    await delay(1000);
    if ((await readSmokeOutput(visualSessionId)).includes(
      "island-simulator: first water-transition accent played")) break;
  }
  await call("Input.dispatchKeyEvent", {
    type: "keyUp", key: "s", code: "KeyS", windowsVirtualKeyCode: 83,
    nativeVirtualKeyCode: 83,
  }, visualSessionId);
  await delay(300);
  await call("Input.dispatchMouseEvent", {
    type: "mouseMoved", x: 700, y: 360, button: "none", buttons: 0,
  }, visualSessionId);
  await call("Input.dispatchMouseEvent", {
    type: "mouseMoved", x: 620, y: 340, button: "none", buttons: 0,
  }, visualSessionId);
  await delay(700);

  await dispatchKeyUntilOutput(
    visualSessionId, "F2", "F2", 113,
    "island-simulator: world debug visualization enabled with 9 ground-contact samples");
  await dispatchKeyUntilOutput(
    visualSessionId, "F4", "F4", 115,
    /island-simulator: world seed clipboard copy (confirmed|unavailable)/);
  // F2 intentionally adds a large diagnostic line batch. Wait for authoritative game output
  // after every edge so even sub-10-FPS SwiftShader runs cannot silently miss the input.
  await dispatchKeyUntilOutput(
    visualSessionId, "F1", "F1", 112, "island-simulator: debug overlay disabled");

  await call("Emulation.setDeviceMetricsOverride", {
    width: 1000, height: 600, deviceScaleFactor: 1, mobile: false,
  }, visualSessionId);
  await delay(800);
  const resizedEvaluation = await call("Runtime.evaluate", {
    expression: `JSON.stringify({
      width: Module.canvas.width,
      height: Module.canvas.height,
      clientWidth: Module.canvas.clientWidth,
      clientHeight: Module.canvas.clientHeight
    })`,
    returnByValue: true,
  }, visualSessionId);
  const resizedState = JSON.parse(resizedEvaluation.result.value ?? "{}");

  await call("Runtime.evaluate", { expression: "document.exitPointerLock()" }, visualSessionId);
  await delay(250);
  const unlockedEvaluation = await call("Runtime.evaluate", {
    expression: "document.pointerLockElement === null",
    returnByValue: true,
  }, visualSessionId);
  const pointerUnlocked = unlockedEvaluation.result.value === true;
  await dispatchMouseClick(visualSessionId, 480, 270);
  const relockedEvaluation = await call("Runtime.evaluate", {
    expression: "document.pointerLockElement === Module.canvas",
    returnByValue: true,
  }, visualSessionId);
  const pointerRelocked = relockedEvaluation.result.value === true;

  const beforeLifecycleEvaluation = await call("Runtime.evaluate", {
    expression: "globalThis.islandSmokeResult?.output ?? ''",
    returnByValue: true,
  }, visualSessionId);
  const samplesBeforeLifecycle = (beforeLifecycleEvaluation.result.value ?? "")
    .match(/island-simulator: input sample/g)?.length ?? 0;
  await call("Page.setWebLifecycleState", { state: "frozen" }, visualSessionId);
  await delay(500);
  await call("Page.setWebLifecycleState", { state: "active" }, visualSessionId);
  await call("Page.bringToFront", {}, visualSessionId);
  // Allow several quarter-second game diagnostics even when the first resumed frames rebuild
  // browser/SwiftShader state.
  await delay(3000);

  const completedInputEvaluation = await call("Runtime.evaluate", {
    expression: "JSON.stringify(globalThis.islandSmokeResult ?? {})",
    returnByValue: true,
  }, visualSessionId);
  inputResult = JSON.parse(completedInputEvaluation.result.value ?? "{}");
  const samples = [...(inputResult.output ?? "").matchAll(
    /input sample captured (\d) relative (\d) player ([-+0-9.eE]+) ([-+0-9.eE]+) camera ([-+0-9.eE]+) ([-+0-9.eE]+) ([-+0-9.eE]+) free (\d) yaw ([-+0-9.eE]+) pitch ([-+0-9.eE]+) viewport (\d+) (\d+)/g)]
    .map(match => ({
      captured: Number(match[1]),
      relative: Number(match[2]),
      x: Number(match[3]),
      z: Number(match[4]),
      cameraX: Number(match[5]),
      cameraY: Number(match[6]),
      cameraZ: Number(match[7]),
      freeCamera: Number(match[8]),
      yaw: Number(match[9]),
      pitch: Number(match[10]),
      viewportWidth: Number(match[11]),
      viewportHeight: Number(match[12]),
    }));
  const initialSample = samples[0];
  const finalSample = samples.at(-1);
  const lifecycleResumed = samples.length > samplesBeforeLifecycle;
  const keyboardMoved = initialSample && finalSample
    && Math.hypot(finalSample.x - initialSample.x, finalSample.z - initialSample.z) > 0.5;
  const mouseLookChanged = initialSample && finalSample
    && (Math.abs(finalSample.yaw - initialSample.yaw) > 0.005
      || Math.abs(finalSample.pitch - initialSample.pitch) > 0.005);
  const waterTransitionPlayed = inputResult.output?.includes(
    "island-simulator: first water-transition accent played");
  const cameraMotionToggled = inputResult.output?.includes(
    "island-simulator: camera motion disabled")
    && inputResult.output?.includes("island-simulator: camera motion enabled");
  const backingAspect = resizedState.width / resizedState.height;
  const clientAspect = resizedState.clientWidth / resizedState.clientHeight;
  const viewportResized = finalSample?.viewportWidth === 1280
    && finalSample?.viewportHeight === 720
    && resizedState.clientWidth <= 1000
    && resizedState.clientHeight <= 600
    && (resizedState.clientWidth === 1000 || resizedState.clientHeight === 600)
    && Math.abs(backingAspect - 16 / 9) < 0.001
    && Math.abs(clientAspect - 16 / 9) < 0.01;
  const inputPassed = inputResult.output?.includes("island-simulator: renderer WEBGL2")
    && inputResult.output?.includes(
      "island-simulator: fixed sunny sky with layered horizontal high wisps, portable shader")
    && inputResult.output?.includes(
      "island-simulator: world debug visualization enabled with 9 ground-contact samples,"
        + " resolved movement trace, wildlife navigation guides, 3 bird route regions,"
        + " 24 bird spawns, 6 audio-zone contours, and positional-emitter markers")
    && inputResult.output?.includes("island-simulator: debug overlay disabled")
    && /island-simulator: world seed clipboard copy (confirmed|unavailable)/.test(
      inputResult.output ?? "")
    && lockedState.pointerLocked === true
    && lockedState.focused === true
    && samples.some(sample => sample.captured === 1 && sample.relative === 1)
    && keyboardMoved
    && cameraMotionToggled
    && waterTransitionPlayed
    && mouseLookChanged
    && viewportResized
    && pointerUnlocked
    && pointerRelocked
    && !inputResult.error;

  const passed = smokePassed && deploymentPassed && visualPassed && beachPassed && grassPassed
    && groundPassed
    && palmPassed && junglePassed && eastClearingPassed && westClearingPassed
    && propPassed && driftwoodPassed && reefPassed
    && waterPassed && currentPassed
    && distantPassed && frondPassed
    && birdPassed && fishPassed && sharkPassed && monkeyPassed && inputPassed;
  const report = {
    passed,
    browser: version.Browser ?? "unknown",
    renderer: "WEBGL2",
    lifecycle: {
      passed: smokePassed,
      exitCode: result.exitCode ?? null,
      output: result.output ?? "",
      error: result.error ?? "",
    },
    deployment: {
      passed: deploymentPassed,
      ...deploymentState,
    },
    visual: {
      passed: visualPassed,
      output: visualResult.output ?? "",
      error: visualResult.error ?? "",
    },
    beach: {
      passed: beachPassed,
      output: beachResult.output ?? "",
      error: beachResult.error ?? "",
    },
    grass: {
      passed: grassPassed,
      output: grassResult.output ?? "",
      error: grassResult.error ?? "",
    },
    ground: {
      passed: groundPassed,
      output: groundResult.output ?? "",
      error: groundResult.error ?? "",
    },
    palms: {
      passed: palmPassed,
      output: palmResult.output ?? "",
      error: palmResult.error ?? "",
    },
    jungle: {
      passed: junglePassed,
      output: jungleResult.output ?? "",
      error: jungleResult.error ?? "",
    },
    eastClearing: {
      passed: eastClearingPassed,
      output: eastClearingResult.output ?? "",
      error: eastClearingResult.error ?? "",
    },
    westClearing: {
      passed: westClearingPassed,
      output: westClearingResult.output ?? "",
      error: westClearingResult.error ?? "",
    },
    props: {
      passed: propPassed,
      output: propResult.output ?? "",
      error: propResult.error ?? "",
    },
    driftwood: {
      passed: driftwoodPassed,
      output: driftwoodResult.output ?? "",
      error: driftwoodResult.error ?? "",
    },
    reef: {
      passed: reefPassed,
      output: reefResult.output ?? "",
      error: reefResult.error ?? "",
    },
    water: {
      passed: waterPassed,
      output: waterResult.output ?? "",
      error: waterResult.error ?? "",
    },
    current: {
      passed: currentPassed,
      output: currentResult.output ?? "",
      error: currentResult.error ?? "",
    },
    distant: {
      passed: distantPassed,
      output: distantResult.output ?? "",
      error: distantResult.error ?? "",
    },
    fronds: {
      passed: frondPassed,
      output: frondResult.output ?? "",
      error: frondResult.error ?? "",
    },
    birds: {
      passed: birdPassed,
      output: birdResult.output ?? "",
      error: birdResult.error ?? "",
    },
    fish: {
      passed: fishPassed,
      output: fishResult.output ?? "",
      error: fishResult.error ?? "",
    },
    shark: {
      passed: sharkPassed,
      output: sharkResult.output ?? "",
      error: sharkResult.error ?? "",
    },
    monkeys: {
      passed: monkeyPassed,
      output: monkeyResult.output ?? "",
      error: monkeyResult.error ?? "",
    },
    input: {
      passed: inputPassed,
      pointerLocked: lockedState.pointerLocked ?? false,
      focused: lockedState.focused ?? false,
      keyboardMoved: Boolean(keyboardMoved),
      cameraMotionToggled: Boolean(cameraMotionToggled),
      waterTransitionPlayed: Boolean(waterTransitionPlayed),
      mouseLookChanged: Boolean(mouseLookChanged),
      viewportResized: Boolean(viewportResized),
      pointerUnlocked,
      pointerRelocked,
      lifecycleResumed,
      lockedState,
      resizedState,
      initialSample: initialSample ?? null,
      finalSample: finalSample ?? null,
      output: inputResult.output ?? "",
      error: inputResult.error ?? "",
    },
    console: consoleMessages,
    screenshot: path.basename(screenshotPath),
    beachScreenshot: path.basename(beachScreenshotPath),
    grassScreenshot: path.basename(grassScreenshotPath),
    groundScreenshot: path.basename(groundScreenshotPath),
    palmScreenshot: path.basename(palmScreenshotPath),
    jungleScreenshot: path.basename(jungleScreenshotPath),
    eastClearingScreenshot: path.basename(eastClearingCapture.screenshotPath),
    westClearingScreenshot: path.basename(westClearingCapture.screenshotPath),
    propScreenshot: path.basename(propScreenshotPath),
    driftwoodScreenshot: path.basename(driftwoodScreenshotPath),
    reefScreenshot: path.basename(reefScreenshotPath),
    waterScreenshot: path.basename(waterScreenshotPath),
    currentScreenshot: path.basename(currentScreenshotPath),
    distantScreenshot: path.basename(distantScreenshotPath),
    frondScreenshot: path.basename(frondScreenshotPath),
    birdScreenshot: path.basename(birdScreenshotPath),
    fishScreenshot: path.basename(fishScreenshotPath),
    sharkScreenshot: path.basename(sharkScreenshotPath),
    monkeyScreenshot: path.basename(monkeyScreenshotPath),
  };
  await writeFile(path.join(buildDirectory, "island-browser-smoke.json"),
    `${JSON.stringify(report, null, 2)}\n`);
  console.log(report.lifecycle.output.trim());
  if (!passed) {
    console.error(JSON.stringify(report, null, 2));
    process.exitCode = 1;
  } else {
    console.log(`Island WebGL 2 browser smoke: PASS (capture: ${screenshotPath})`);
  }

  await call("Target.closeTarget", { targetId: visualTargetId });
  await call("Browser.close");
} finally {
  browserSocket?.close();
  server.kill();
  if (chrome && chrome.exitCode === null) {
    const exited = new Promise(resolve => chrome.once("exit", resolve));
    chrome.kill();
    await Promise.race([exited, delay(2000)]);
  }
  await rm(profile, { recursive: true, force: true, maxRetries: 5, retryDelay: 100 });
}
