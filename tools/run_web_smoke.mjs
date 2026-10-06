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
    && result.output?.includes("island-simulator: terrain portable sand/soil material shader")
    && result.output?.includes("island-simulator: ocean portable custom shader")
    && result.output?.includes("island-simulator: fixed sunny sky portable shader")
    && result.output?.includes("island-simulator: palms 330 in 64 cells, hardware instancing")
    && result.output?.includes("palm portable wind shader with near/far mesh LOD")
    && result.output?.includes("island-simulator: jungle 281 canopy trees, 1822 understory plants, 565 low bushes, hardware instancing")
    && result.output?.includes("island-simulator: jungle ground layer 2225 small plants")
    && result.output?.includes("island-simulator: birds 24 agents")
    && result.output?.includes("island-simulator: fish 48 agents")
    && result.output?.includes("island-simulator: one occasional non-hostile offshore shark patrol")
    && result.output?.includes("island-simulator: monkeys 7 agents with portable GPU skeletal animation")
    && result.output?.includes("3 silhouettes, 4 palettes")
    && result.output?.includes("jungle portable wind shader with canopy/understory/bush/ground LOD")
    && result.output?.includes("island-simulator: natural props 152 rocks, 11 fallen logs, 28 driftwood pieces, hardware instancing")
    && result.output?.includes("island-simulator: beach detail 265 shell/pebble clusters, near-field culled")
    && result.output?.includes("natural-prop portable shader with near/far LOD")
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
    && deploymentState.output?.includes("island-simulator: fixed sunny sky portable shader")
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
    && visualResult.output?.includes("island-simulator: fixed sunny sky portable shader")
    && visualResult.output?.includes("island-simulator: palms 330 in 64 cells, hardware instancing")
    && visualResult.output?.includes("palm portable wind shader with near/far mesh LOD")
    && visualResult.output?.includes(
      "island-simulator: jungle 281 canopy trees, 1822 understory plants, 565 low bushes, hardware instancing")
    && visualResult.output?.includes("jungle portable wind shader with canopy/understory/bush/ground LOD")
    && visualResult.output?.includes("natural-prop portable shader with near/far LOD")
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
    && beachResult.output?.includes("island-simulator: fixed sunny sky portable shader")
    && beachResult.output?.includes("palm portable wind shader with near/far mesh LOD")
    && beachResult.output?.includes("jungle portable wind shader with canopy/understory/bush/ground LOD")
    && beachResult.output?.includes("portable instanced wind shader")
    && beachResult.output?.includes("island-simulator: debug overlay active")
    && /grass [1-9][0-9]*\/[1-9][0-9]* patches/.test(beachResult.output ?? "")
    && !beachResult.error;

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
    && jungleResult.output?.includes("island-simulator: fixed sunny sky portable shader")
    && jungleResult.output?.includes("jungle portable wind shader with canopy/understory/bush/ground LOD")
    && /jungle [1-9][0-9]*\/281 canopy, [1-9][0-9]*\/1822 understory, [1-9][0-9]*\/565 bushes, [1-9][0-9]*\/2225 ground plants/.test(
      jungleResult.output ?? "")
    && !jungleResult.error;

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
    && propResult.output?.includes("island-simulator: fixed sunny sky portable shader")
    && propResult.output?.includes("natural-prop portable shader with near/far LOD")
    && propResult.output?.includes("jungle portable wind shader with canopy/understory/bush/ground LOD")
    && /props [0-9]+\/152 rocks, [1-9][0-9]*\/11 logs/.test(propResult.output ?? "")
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
    && driftwoodResult.output?.includes("island-simulator: fixed sunny sky portable shader")
    && driftwoodResult.output?.includes("natural-prop portable shader with near/far LOD")
    && /props [0-9]+\/152 rocks, [0-9]+\/11 logs, [1-9][0-9]*\/28 driftwood, [1-9][0-9]*\/265 shell\/pebble clusters/.test(
      driftwoodResult.output ?? "")
    && !driftwoodResult.error;

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
    && birdResult.output?.includes("island-simulator: birds 24 agents")
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
    && fishResult.output?.includes("island-simulator: fish 48 agents")
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
    && sharkResult.output?.includes("island-simulator: one occasional non-hostile offshore shark patrol")
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

  await call("Input.dispatchMouseEvent", {
    type: "mousePressed", x: 640, y: 360, button: "left", buttons: 1, clickCount: 1,
  }, visualSessionId);
  await delay(150);
  await call("Input.dispatchMouseEvent", {
    type: "mouseReleased", x: 640, y: 360, button: "left", buttons: 0, clickCount: 1,
  }, visualSessionId);
  await delay(500);
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
  await call("Input.dispatchKeyEvent", {
    type: "keyDown", key: "F5", code: "F5", windowsVirtualKeyCode: 116,
    nativeVirtualKeyCode: 116,
  }, visualSessionId);
  await delay(100);
  await call("Input.dispatchKeyEvent", {
    type: "keyUp", key: "F5", code: "F5", windowsVirtualKeyCode: 116,
    nativeVirtualKeyCode: 116,
  }, visualSessionId);
  await delay(150);
  await call("Input.dispatchKeyEvent", {
    type: "keyDown", key: "s", code: "KeyS", windowsVirtualKeyCode: 83,
    nativeVirtualKeyCode: 83,
  }, visualSessionId);
  await delay(6000);
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

  await call("Input.dispatchKeyEvent", {
    type: "keyDown", key: "F2", code: "F2", windowsVirtualKeyCode: 113,
    nativeVirtualKeyCode: 113,
  }, visualSessionId);
  await delay(100);
  await call("Input.dispatchKeyEvent", {
    type: "keyUp", key: "F2", code: "F2", windowsVirtualKeyCode: 113,
    nativeVirtualKeyCode: 113,
  }, visualSessionId);
  await delay(300);

  await call("Input.dispatchKeyEvent", {
    type: "keyDown", key: "F1", code: "F1", windowsVirtualKeyCode: 112,
    nativeVirtualKeyCode: 112,
  }, visualSessionId);
  await delay(100);
  await call("Input.dispatchKeyEvent", {
    type: "keyUp", key: "F1", code: "F1", windowsVirtualKeyCode: 112,
    nativeVirtualKeyCode: 112,
  }, visualSessionId);
  await delay(300);

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
  await call("Input.dispatchMouseEvent", {
    type: "mousePressed", x: 480, y: 270, button: "left", buttons: 1, clickCount: 1,
  }, visualSessionId);
  await call("Input.dispatchMouseEvent", {
    type: "mouseReleased", x: 480, y: 270, button: "left", buttons: 0, clickCount: 1,
  }, visualSessionId);
  await delay(450);
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
  await delay(1500);

  const completedInputEvaluation = await call("Runtime.evaluate", {
    expression: "JSON.stringify(globalThis.islandSmokeResult ?? {})",
    returnByValue: true,
  }, visualSessionId);
  inputResult = JSON.parse(completedInputEvaluation.result.value ?? "{}");
  const samples = [...(inputResult.output ?? "").matchAll(
    /input sample captured (\d) relative (\d) x ([-+0-9.eE]+) z ([-+0-9.eE]+) yaw ([-+0-9.eE]+) pitch ([-+0-9.eE]+) viewport (\d+) (\d+)/g)]
    .map(match => ({
      captured: Number(match[1]),
      relative: Number(match[2]),
      x: Number(match[3]),
      z: Number(match[4]),
      yaw: Number(match[5]),
      pitch: Number(match[6]),
      viewportWidth: Number(match[7]),
      viewportHeight: Number(match[8]),
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
    && inputResult.output?.includes("island-simulator: fixed sunny sky portable shader")
    && inputResult.output?.includes("island-simulator: world debug visualization enabled")
    && inputResult.output?.includes("island-simulator: debug overlay disabled")
    && lockedState.pointerLocked === true
    && lockedState.focused === true
    && samples.some(sample => sample.captured === 1 && sample.relative === 1)
    && keyboardMoved
    && waterTransitionPlayed
    && mouseLookChanged
    && viewportResized
    && pointerUnlocked
    && pointerRelocked
    && !inputResult.error;

  const passed = smokePassed && deploymentPassed && visualPassed && beachPassed
    && junglePassed && propPassed && driftwoodPassed && birdPassed && fishPassed
    && sharkPassed && monkeyPassed && inputPassed;
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
    jungle: {
      passed: junglePassed,
      output: jungleResult.output ?? "",
      error: jungleResult.error ?? "",
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
    jungleScreenshot: path.basename(jungleScreenshotPath),
    propScreenshot: path.basename(propScreenshotPath),
    driftwoodScreenshot: path.basename(driftwoodScreenshotPath),
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
