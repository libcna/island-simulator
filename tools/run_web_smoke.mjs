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
  const captureVisualFrame = async (sessionId, fileName, debugOverlayEnabled = false) => {
    if (debugOverlayEnabled) {
      if (!(await waitForOutput(
        sessionId, "island-simulator: debug overlay active", 80))) {
        throw new Error(`Visual capture ${fileName} did not start with the debug overlay active`);
      }
      await call("Page.bringToFront", {}, sessionId);
      const focusResult = await call("Runtime.evaluate", {
        expression: `(() => {
          const canvas = document.getElementById("canvas");
          if (!canvas) return false;
          canvas.tabIndex = -1;
          canvas.focus();
          return document.activeElement === canvas;
        })()`,
        returnByValue: true,
      }, sessionId);
      if (focusResult.result.value !== true) {
        throw new Error(`Visual capture ${fileName} could not focus the canvas`);
      }
      const overlayHidden = await dispatchKeyUntilOutput(
        sessionId, "F1", "F1", 112, "island-simulator: debug overlay disabled");
      if (!overlayHidden) {
        throw new Error(`Visual capture ${fileName} could not hide the debug overlay`);
      }
      // Let at least one heavily loaded SwiftShader frame present without the overlay.
      await delay(300);
    }
    const screenshot = await call("Page.captureScreenshot", { format: "png" }, sessionId);
    const screenshotPath = path.join(buildDirectory, fileName);
    await writeFile(screenshotPath, Buffer.from(screenshot.data, "base64"));
    return screenshotPath;
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

  const terrainBatchMatch = (result.output ?? "").match(
    /visible terrain cells ([1-9][0-9]*)\/64 in ([1-9][0-9]*) shared-buffer draws/);
  const viewportMatch = (result.output ?? "").match(
    /island-simulator: viewport ([1-9][0-9]*)x([1-9][0-9]*) logical backbuffer/);
  const smokePassed = result.exited === true
    && result.exitCode === 0
    && result.output?.includes("island-simulator: renderer WEBGL2")
    && result.output?.includes(
      "island-simulator: quality profile full grass 135m shadows 100% wildlife 100% ocean 160x160 textures 143624/163840 bytes audio 6/6 sources")
    && viewportMatch !== null
    && Number(viewportMatch[1]) === 1280
    && Number(viewportMatch[2]) === 720
    && /island-simulator: startup world [0-9.]+ ms, framework [0-9.]+ ms, content [0-9.]+ ms, pre-draw [0-9.]+ ms, first draw [0-9.]+ ms, total [0-9.]+ ms/.test(
      result.output ?? "")
    && result.output?.includes("island-simulator: terrain portable sand/soil material shader")
    && result.output?.includes(
      "island-simulator: terrain wet sand uses bounded view-dependent sun sheen")
    && result.output?.includes(
      "island-simulator: terrain 58081 vertices, 115200 triangles in 64 cull cells with shared buffers")
    && terrainBatchMatch !== null
    && Number(terrainBatchMatch[2]) < Number(terrainBatchMatch[1])
    && result.output?.includes(
      "island-simulator: terrain near-field microvariation and procedural interior leaf litter")
    && result.output?.includes(
      "island-simulator: terrain close sand granulation and shoreline-raked ripples")
    && result.output?.includes(
      "island-simulator: terrain sparse close sand pebbles and shell flecks")
    && result.output?.includes(
      "island-simulator: terrain fixed-sun interior dapple from shared portable noise fields")
    && result.output?.includes(
      "island-simulator: terrain authored primary footpath material")
    && result.output?.includes(
      "island-simulator: terrain footpath narrow irregular wear")
    && result.output?.includes(
      "island-simulator: terrain distinct authored east/west clearing materials")
    && result.output?.includes(
      "island-simulator: clearing floors reuse existing noise fields for broad patch breakup")
    && result.output?.includes("island-simulator: ocean portable custom shader")
    && result.output?.includes(
      "island-simulator: ocean reused crossing-wave subsurface colour")
    && result.output?.includes(
      "island-simulator: ocean shallow ridge light reuses animated slope field")
    && result.output?.includes(
      "island-simulator: ocean sunward ripple crests reuse analytic slope field")
    && result.output?.includes(
      "island-simulator: ocean grazing reflection bands reuse analytic slope field")
    && result.output?.includes(
      "island-simulator: fixed sunny sky with layered horizontal high wisps, portable shader")
    && result.output?.includes(
      "island-simulator: distant scenery 5 unreachable low islets, one portable batch")
    && result.output?.includes("island-simulator: palms 330 in 64 cells, hardware instancing")
    && result.output?.includes("palm portable wind shader with near/far mesh LOD")
    && result.output?.includes("palm near mesh segmented fuller leaflet silhouettes")
    && result.output?.includes(
      "island-simulator: palm near leaflets use staggered asymmetric silhouettes")
    && result.output?.includes(
      "island-simulator: palm near geometry uses fixed-sun facet shading")
    && result.output?.includes("palm near root wedges ground trunk bases")
    && result.output?.includes("palm procedural bark scars and restrained coconut clusters")
    && result.output?.includes(
      "island-simulator: palm per-instance frond tint reuses existing transforms")
    && result.output?.includes("island-simulator: jungle 281 canopy trees, 1822 understory plants, 565 low bushes, hardware instancing")
    && result.output?.includes("island-simulator: jungle ground layer 2225 small plants")
    && result.output?.includes(
      "island-simulator: jungle fixed-sun two-sided geometric facet lighting")
    && result.output?.includes(
      "island-simulator: jungle per-instance foliage tint reuses existing transforms")
    && result.output?.includes("island-simulator: birds 24 agents")
    && result.output?.includes(
      "island-simulator: bird flight uses slow radial drift, banking, and intermittent glide")
    && result.output?.includes("island-simulator: fish 48 agents")
    && result.output?.includes("island-simulator: one occasional non-hostile offshore shark patrol")
    && result.output?.includes(
      "island-simulator: aquatic wildlife uses depth pitch, banked turns, forked tails, and near head detail")
    && result.output?.includes(
      "island-simulator: aquatic wildlife uses depth tint and restrained moving sunlight bands in CPU vertex colours")
    && result.output?.includes(
      "island-simulator: fish near/far geometry 51/12 vertices, single-surface fins and CPU faceted daylight")
    && result.output?.includes(
      "island-simulator: shark near/far geometry 51/15 vertices, single-surface fins and CPU faceted daylight")
    && result.output?.includes("island-simulator: monkeys 7 agents with portable GPU skeletal animation")
    && result.output?.includes(
      "island-simulator: keyboard run/walk toggle uses a transient mode confirmation")
    && result.output?.includes(
      "island-simulator: near monkeys use restrained procedural crown and cheek fur tufts")
    && result.output?.includes(
      "island-simulator: near monkeys use bone-following finger and toe silhouettes")
    && result.output?.includes(
      "island-simulator: calm monkeys use bounded skeletal head attention")
    && result.output?.includes("3 silhouettes, 4 palettes")
    && result.output?.includes(
      "island-simulator: terrain-conforming contact shadows in one portable near-field batch")
    && result.output?.includes("jungle portable wind shader with canopy/understory/bush/ground LOD")
    && result.output?.includes(
      "jungle near canopy tapered support branches and pointed leaf silhouettes")
    && result.output?.includes(
      "island-simulator: selected near canopy variants include hanging leafy lianas")
    && result.output?.includes(
      "island-simulator: jungle near canopy irregular buttress roots ground trunk bases")
    && result.output?.includes(
      "island-simulator: jungle near canopy procedural bark fissures")
    && result.output?.includes(
      "jungle near bushes with pointed leaf edge breakup")
    && result.output?.includes(
      "jungle near ground plants include upright and split-leaf silhouettes")
    && result.output?.includes(
      "jungle near ground plants reuse one variant for sparse fallen-leaf silhouettes")
    && result.output?.includes("island-simulator: natural props 171 rocks, 14 reef outcrops, 13 fallen logs, 31 driftwood pieces, 63 fallen palm fronds, hardware instancing")
    && result.output?.includes(
      "island-simulator: clearing landmarks 5 rocks and 2 fallen logs, shared authored placement")
    && result.output?.includes(
      "island-simulator: cove landmark 3 driftwood pieces, shared authored shoreline profile")
    && result.output?.includes("island-simulator: beach detail 265 shell/pebble/wrack clusters, near-field culled")
    && result.output?.includes("island-simulator: dry wrack reuses one of four beach-detail variants")
    && result.output?.includes(
      "island-simulator: near-surface swimming view uses generated depth haze, surface scatter, and peripheral veil in one draw")
    && result.output?.includes(
      "island-simulator: movement ripples and shoreline-transition bursts integrated into the portable ocean shader")
    && result.output?.includes(
      "island-simulator: ocean shoreline foam uses broken continuity mask")
    && result.output?.includes(
      "island-simulator: outer-ocean current uses progressive inward return with mild coherent circulation")
    && result.output?.includes("natural-prop portable shader with near/far LOD")
    && result.output?.includes(
      "island-simulator: near rocks use procedural strata and restrained lichen flecks")
    && result.output?.includes(
      "island-simulator: near low rocks use a fixed-sea-level wet material band")
    && result.output?.includes(
      "island-simulator: near rocks use irregular crown profiles and fixed-sun facet shading")
    && result.output?.includes(
      "island-simulator: rock per-instance tint reuses existing transforms")
    && result.output?.includes(
      "island-simulator: near fallen logs use procedural bark grooves and cut-end growth rings")
    && result.output?.includes(
      "island-simulator: near fallen logs use irregular broken end contours")
    && result.output?.includes(
      "island-simulator: near fallen-log end grain uses restrained radial cracks")
    && result.output?.includes(
      "island-simulator: near driftwood uses faded sun-bleached longitudinal grain")
    && result.output?.includes(
      "island-simulator: near driftwood variant 2 uses three tapered fork limbs")
    && result.output?.includes(
      "island-simulator: all near driftwood variants use tapered three-dimensional branches")
    && result.output?.includes(
      "island-simulator: driftwood end caps match irregular body rings")
    && result.output?.includes("shoreline portable animated foam ribbon, 2695 vertices, 4608 triangles")
    && result.output?.includes(
      "island-simulator: shoreline broken crest mask modulates wash continuity")
    && result.output?.includes("portable instanced wind shader")
    && result.output?.includes(
      "island-simulator: grass biome tint follows coast, path, and authored clearings")
    && result.output?.includes(
      "island-simulator: grass near clusters use 42 blades for denser first-person cover")
    && result.output?.includes(
      "island-simulator: selected near grass blades use a geometric V-fold ridge")
    && result.output?.includes(
      "island-simulator: grass sparse near seed heads share blade wind and instancing")
    && result.output?.includes(
      "island-simulator: near birds use layered trailing flight-feather silhouettes")
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
  await call("Page.bringToFront", {}, deploymentSessionId);
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
  deploymentState.audioStagingBounded = deploymentState.output?.includes(
    "island-simulator: audio sources use sequential generation and upload with bounded staging memory");
  deploymentState.ambientPcmDcCentered = deploymentState.output?.includes(
    "island-simulator: persistent ambient PCM is DC-centered before quantization");
  deploymentState.ambientBedVariationEnabled = deploymentState.output?.includes(
    "island-simulator: ambient beds use slow non-amplifying gain drift");
  deploymentState.waveAccentsEnabled = deploymentState.output?.includes(
    "island-simulator: sparse stronger-wave accents enabled");
  deploymentState.waterMovementEnabled = deploymentState.output?.includes(
    "island-simulator: player water-movement layer enabled");
  deploymentState.waterTransitionsEnabled = deploymentState.output?.includes(
    "island-simulator: water-transition accents enabled");
  deploymentState.waterTransitionVariantsEnabled = deploymentState.output?.includes(
    "island-simulator: two water-transition variants enabled");
  deploymentState.landFootstepsEnabled = deploymentState.output?.includes(
    "island-simulator: distance-scheduled sand and soil footsteps enabled");
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
    && deploymentState.audioStagingBounded === true
    && deploymentState.ambientPcmDcCentered === true
    && deploymentState.ambientBedVariationEnabled === true
    && deploymentState.waveAccentsEnabled === true
    && deploymentState.waterMovementEnabled === true
    && deploymentState.waterTransitionsEnabled === true
    && deploymentState.waterTransitionVariantsEnabled === true
    && deploymentState.landFootstepsEnabled === true
    && deploymentState.jungleAmbienceEnabled === true
    && deploymentState.waveVariantsEnabled === true
    && deploymentState.positionalWildlifeCallsEnabled === true
    && deploymentState.pointerLocked === true;

  // Exercise CNA StorageDevice's browser IDBFS path with a real preference change and a fresh
  // runtime in the same origin/profile. QA scene pages disable settings explicitly, so this is
  // isolated from their deterministic input assumptions while still testing the production shell.
  let settingsWriteOutput = "";
  for (let keyAttempt = 0; keyAttempt < 3; ++keyAttempt) {
    await dispatchKeyPress(deploymentSessionId, "b", "KeyB", 66, 650, 350);
    for (let outputAttempt = 0; outputAttempt < 12; ++outputAttempt) {
      const evaluation = await call("Runtime.evaluate", {
        expression: "globalThis.islandDeploymentOutput ?? ''",
        returnByValue: true,
      }, deploymentSessionId);
      settingsWriteOutput = evaluation.result.value ?? "";
      if (settingsWriteOutput.includes("island-simulator: camera motion disabled")
          && settingsWriteOutput.includes("island-simulator: user settings saved")) break;
      await delay(250);
    }
    if (settingsWriteOutput.includes("island-simulator: camera motion disabled")
        && settingsWriteOutput.includes("island-simulator: user settings saved")) break;
  }
  const webSettingsWritten = settingsWriteOutput.includes(
    "island-simulator: camera motion disabled")
    && settingsWriteOutput.includes("island-simulator: user settings saved");
  await delay(700);
  await call("Target.closeTarget", { targetId: deploymentTargetId });

  const { targetId: settingsTargetId } = await call(
    "Target.createTarget", { url: "about:blank" });
  const { sessionId: settingsSessionId } = await call(
    "Target.attachToTarget", { targetId: settingsTargetId, flatten: true });
  await call("Runtime.enable", {}, settingsSessionId);
  await call("Page.enable", {}, settingsSessionId);
  await call("Emulation.setDeviceMetricsOverride", {
    width: 1280, height: 720, deviceScaleFactor: 1, mobile: false,
  }, settingsSessionId);
  await call("Page.navigate", {
    url: `http://127.0.0.1:${port}/island-simulator.html`,
  }, settingsSessionId);
  let settingsReloadOutput = "";
  for (let attempt = 0; attempt < 240; ++attempt) {
    await delay(100);
    const evaluation = await call("Runtime.evaluate", {
      expression: "globalThis.islandDeploymentOutput ?? ''",
      returnByValue: true,
    }, settingsSessionId);
    settingsReloadOutput = evaluation.result.value ?? "";
    if (settingsReloadOutput.includes(
      "island-simulator: user settings loaded run disabled camera motion disabled"
        + " audio enabled touch standard")) break;
  }
  const webSettingsPersistencePassed = webSettingsWritten
    && settingsReloadOutput.includes(
      "island-simulator: user settings loaded run disabled camera motion disabled"
        + " audio enabled touch standard")
    && !/settings unavailable|settings could not be (?:read|saved)/i.test(settingsReloadOutput);
  await call("Target.closeTarget", { targetId: settingsTargetId });

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

  const screenshotPath = await captureVisualFrame(
    visualSessionId, "island-browser-smoke.png");

  const visualPassed = visualResult.output?.includes("island-simulator: renderer WEBGL2")
    && visualResult.output?.includes("island-simulator: ocean portable custom shader")
    && visualResult.output?.includes(
      "island-simulator: ocean reused crossing-wave subsurface colour")
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
    && visualResult.output?.includes(
      "island-simulator: near low rocks use a fixed-sea-level wet material band")
    && visualResult.output?.includes(
      "island-simulator: near rocks use irregular crown profiles and fixed-sun facet shading")
    && visualResult.output?.includes(
      "island-simulator: rock per-instance tint reuses existing transforms")
    && visualResult.output?.includes("portable instanced wind shader")
    && visualResult.output?.includes(
      "island-simulator: grass biome tint follows coast, path, and authored clearings")
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

  const beachScreenshotPath = await captureVisualFrame(
    visualSessionId, "island-browser-beach.png", true);
  const beachPassed = beachResult.output?.includes("island-simulator: renderer WEBGL2")
    && beachResult.output?.includes("island-simulator: ocean portable custom shader")
    && beachResult.output?.includes(
      "island-simulator: ocean reused crossing-wave subsurface colour")
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
      "island-simulator: selected near grass blades use a geometric V-fold ridge")
    && beachResult.output?.includes(
      "island-simulator: grass biome tint follows coast, path, and authored clearings")
    && beachResult.output?.includes(
      "island-simulator: terrain authored primary footpath material")
    && beachResult.output?.includes(
      "island-simulator: terrain footpath narrow irregular wear")
    && beachResult.output?.includes(
      "island-simulator: terrain sparse close sand pebbles and shell flecks")
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

  const grassScreenshotPath = await captureVisualFrame(
    visualSessionId, "island-browser-grass.png", true);
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
    && grassResult.output?.includes(
      "island-simulator: grass near clusters use 42 blades for denser first-person cover")
    && grassResult.output?.includes(
      "island-simulator: selected near grass blades use a geometric V-fold ridge")
    && grassResult.output?.includes(
      "island-simulator: grass biome tint follows coast, path, and authored clearings")
    && grassResult.output?.includes(
      "island-simulator: grass sparse sun-cured tip variation in shared vertex colours")
    && grassResult.output?.includes(
      "island-simulator: grass per-instance blade stature with near juvenile undergrowth")
    && grassResult.output?.includes(
      "island-simulator: grass sparse near seed heads share blade wind and instancing")
    && grassResult.output?.includes(
      "island-simulator: grass three-tier per-patch LOD transitions 62-78/86-102 metres")
    && /grass [1-9][0-9]*\/18556 patches in [1-9][0-9]* cells, [1-9][0-9]* draws, interaction 1, custom shader 1, biome tint 1, LOD near [1-9][0-9]*, middle [1-9][0-9]*, far [1-9][0-9]*, mixed cells [1-9][0-9]*/.test(
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

  const groundScreenshotPath = await captureVisualFrame(
    visualSessionId, "island-browser-ground.png", true);
  const groundPassed = groundResult.output?.includes("island-simulator: renderer WEBGL2")
    && groundResult.output?.includes(
      "jungle near ground plants include upright and split-leaf silhouettes")
    && groundResult.output?.includes(
      "jungle near ground plants reuse one variant for sparse fallen-leaf silhouettes")
    && groundResult.output?.includes(
      "island-simulator: ground QA target variant 2 at player-eye detail distance")
    && /[1-9][0-9]*\/2225 ground plants \(near [1-9][0-9]*, far [0-9]+\)/.test(
      groundResult.output ?? "")
    && groundResult.output?.includes("island-simulator: debug overlay active")
    && !groundResult.error;

  await call("Page.navigate", {
    url: `http://127.0.0.1:${port}/island-smoke.html?mode=litter`,
  }, visualSessionId);
  let litterResult = {};
  for (let attempt = 0; attempt < 200; ++attempt) {
    await delay(100);
    const evaluation = await call("Runtime.evaluate", {
      expression: "JSON.stringify(globalThis.islandSmokeResult ?? {})",
      returnByValue: true,
    }, visualSessionId);
    litterResult = JSON.parse(evaluation.result.value ?? "{}");
    if (litterResult.output?.includes("island-simulator: terrain") || litterResult.error) break;
  }
  await delay(1000);
  const completedLitterEvaluation = await call("Runtime.evaluate", {
    expression: "JSON.stringify(globalThis.islandSmokeResult ?? {})",
    returnByValue: true,
  }, visualSessionId);
  litterResult = JSON.parse(completedLitterEvaluation.result.value ?? "{}");

  const litterScreenshotPath = await captureVisualFrame(
    visualSessionId, "island-browser-litter.png", true);
  const litterPassed = litterResult.output?.includes("island-simulator: renderer WEBGL2")
    && litterResult.output?.includes(
      "jungle near ground plants reuse one variant for sparse fallen-leaf silhouettes")
    && litterResult.output?.includes(
      "island-simulator: ground-litter QA target variant 0 at player-eye detail distance")
    && /[1-9][0-9]*\/2225 ground plants \(near [1-9][0-9]*, far [0-9]+\)/.test(
      litterResult.output ?? "")
    && litterResult.output?.includes("island-simulator: debug overlay active")
    && !litterResult.error;

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

  const palmScreenshotPath = await captureVisualFrame(
    visualSessionId, "island-browser-palms.png", true);
  const palmPassed = palmResult.output?.includes("island-simulator: renderer WEBGL2")
    && palmResult.output?.includes(
      "island-simulator: fixed sunny sky with layered horizontal high wisps, portable shader")
    && palmResult.output?.includes("palm portable wind shader with near/far mesh LOD")
    && palmResult.output?.includes("palm near mesh segmented fuller leaflet silhouettes")
    && palmResult.output?.includes(
      "island-simulator: palm near leaflets use staggered asymmetric silhouettes")
    && palmResult.output?.includes(
      "island-simulator: palm near geometry uses fixed-sun facet shading")
    && palmResult.output?.includes("palm near root wedges ground trunk bases")
    && palmResult.output?.includes(
      "palm procedural bark scars and restrained coconut clusters")
    && palmResult.output?.includes(
      "island-simulator: palm per-instance frond tint reuses existing transforms")
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

  const jungleScreenshotPath = await captureVisualFrame(
    visualSessionId, "island-browser-jungle.png", true);
  const junglePassed = jungleResult.output?.includes("island-simulator: renderer WEBGL2")
    && jungleResult.output?.includes(
      "island-simulator: fixed sunny sky with layered horizontal high wisps, portable shader")
    && jungleResult.output?.includes(
      "island-simulator: terrain near-field microvariation and procedural interior leaf litter")
    && jungleResult.output?.includes(
      "island-simulator: terrain fixed-sun interior dapple from shared portable noise fields")
    && jungleResult.output?.includes(
      "island-simulator: terrain authored primary footpath material")
    && jungleResult.output?.includes(
      "island-simulator: terrain footpath narrow irregular wear")
    && jungleResult.output?.includes("jungle portable wind shader with canopy/understory/bush/ground LOD")
    && jungleResult.output?.includes(
      "island-simulator: jungle fixed-sun two-sided geometric facet lighting")
    && jungleResult.output?.includes(
      "island-simulator: jungle per-instance foliage tint reuses existing transforms")
    && jungleResult.output?.includes(
      "jungle near canopy tapered support branches and pointed leaf silhouettes")
    && jungleResult.output?.includes(
      "island-simulator: selected near canopy variants include hanging leafy lianas")
    && jungleResult.output?.includes(
      "island-simulator: jungle near canopy irregular buttress roots ground trunk bases")
    && jungleResult.output?.includes(
      "island-simulator: jungle near canopy procedural bark fissures")
    && jungleResult.output?.includes(
      "jungle near bushes with pointed leaf edge breakup")
    && jungleResult.output?.includes(
      "jungle near ground plants include upright and split-leaf silhouettes")
    && jungleResult.output?.includes(
      "jungle near ground plants reuse one variant for sparse fallen-leaf silhouettes")
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
    const clearingScreenshotPath = await captureVisualFrame(
      visualSessionId, fileName, true);
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
      "island-simulator: east clearing uses a warm dry ground cue")
    && eastClearingResult.output?.includes(
      "island-simulator: clearing floors reuse existing noise fields for broad patch breakup")
    && eastClearingResult.output?.includes(
      "island-simulator: east clearing QA camera at authored centre")
    && eastClearingResult.output?.includes(
      "island-simulator: clearing landmarks 5 rocks and 2 fallen logs, shared authored placement")
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
      "island-simulator: clearing floors reuse existing noise fields for broad patch breakup")
    && westClearingResult.output?.includes(
      "island-simulator: west clearing QA camera at authored centre")
    && westClearingResult.output?.includes(
      "island-simulator: clearing landmarks 5 rocks and 2 fallen logs, shared authored placement")
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

  const propScreenshotPath = await captureVisualFrame(
    visualSessionId, "island-browser-props.png", true);
  const propPassed = propResult.output?.includes("island-simulator: renderer WEBGL2")
    && propResult.output?.includes(
      "island-simulator: fixed sunny sky with layered horizontal high wisps, portable shader")
    && propResult.output?.includes("natural-prop portable shader with near/far LOD")
    && propResult.output?.includes(
      "island-simulator: near fallen logs use procedural bark grooves and cut-end growth rings")
    && propResult.output?.includes(
      "island-simulator: near fallen logs use irregular broken end contours")
    && propResult.output?.includes(
      "island-simulator: near fallen-log end grain uses restrained radial cracks")
    && propResult.output?.includes(
      "island-simulator: rock per-instance tint reuses existing transforms")
    && propResult.output?.includes("jungle portable wind shader with canopy/understory/bush/ground LOD")
    && /props [0-9]+\/171 rocks, [0-9]+\/14 reef outcrops, [1-9][0-9]*\/13 logs/.test(
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

  const driftwoodScreenshotPath = await captureVisualFrame(
    visualSessionId, "island-browser-driftwood.png", true);
  const driftwoodPassed = driftwoodResult.output?.includes("island-simulator: renderer WEBGL2")
    && driftwoodResult.output?.includes(
      "island-simulator: fixed sunny sky with layered horizontal high wisps, portable shader")
    && driftwoodResult.output?.includes("natural-prop portable shader with near/far LOD")
    && driftwoodResult.output?.includes(
      "island-simulator: near driftwood uses faded sun-bleached longitudinal grain")
    && driftwoodResult.output?.includes(
      "island-simulator: near driftwood variant 2 uses three tapered fork limbs")
    && driftwoodResult.output?.includes(
      "island-simulator: all near driftwood variants use tapered three-dimensional branches")
    && driftwoodResult.output?.includes(
      "island-simulator: driftwood end caps match irregular body rings")
    && /props [0-9]+\/171 rocks, [0-9]+\/14 reef outcrops, [0-9]+\/13 logs, [1-9][0-9]*\/31 driftwood \(near [1-9][0-9]*, far [0-9]+\), [1-9][0-9]*\/265 shell\/pebble\/wrack clusters/.test(
      driftwoodResult.output ?? "")
    && driftwoodResult.output?.includes(
      "island-simulator: cove landmark 3 driftwood pieces, shared authored shoreline profile")
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

  const reefScreenshotPath = await captureVisualFrame(
    visualSessionId, "island-browser-reef.png", true);
  const reefPassed = reefResult.output?.includes("island-simulator: renderer WEBGL2")
    && reefResult.output?.includes(
      "island-simulator: fixed sunny sky with layered horizontal high wisps, portable shader")
    && reefResult.output?.includes("natural-prop portable shader with near/far LOD")
    && reefResult.output?.includes(
      "island-simulator: near rocks use procedural strata and restrained lichen flecks")
    && reefResult.output?.includes(
      "island-simulator: near low rocks use a fixed-sea-level wet material band")
    && reefResult.output?.includes(
      "island-simulator: near rocks use irregular crown profiles and fixed-sun facet shading")
    && reefResult.output?.includes(
      "island-simulator: rock per-instance tint reuses existing transforms")
    && /[1-9][0-9]*\/14 reef outcrops/.test(reefResult.output ?? "")
    && !reefResult.error;

  await call("Page.navigate", {
    url: `http://127.0.0.1:${port}/island-smoke.html?mode=shipwreck`,
  }, visualSessionId);
  let shipwreckResult = {};
  for (let attempt = 0; attempt < 200; ++attempt) {
    await delay(100);
    const evaluation = await call("Runtime.evaluate", {
      expression: "JSON.stringify(globalThis.islandSmokeResult ?? {})",
      returnByValue: true,
    }, visualSessionId);
    shipwreckResult = JSON.parse(evaluation.result.value ?? "{}");
    if (shipwreckResult.output?.includes("island-simulator: terrain")
        || shipwreckResult.error) break;
  }
  await delay(1000);
  const completedShipwreckEvaluation = await call("Runtime.evaluate", {
    expression: "JSON.stringify(globalThis.islandSmokeResult ?? {})",
    returnByValue: true,
  }, visualSessionId);
  shipwreckResult = JSON.parse(completedShipwreckEvaluation.result.value ?? "{}");

  const shipwreckScreenshotPath = await captureVisualFrame(
    visualSessionId, "island-browser-shipwreck.png", true);
  const shipwreckPassed = shipwreckResult.output?.includes(
      "island-simulator: renderer WEBGL2")
    && shipwreckResult.output?.includes(
      "island-simulator: shipwreck portable aged wood, waterline, and salt-wear material")
    && shipwreckResult.output?.includes(
      "island-simulator: shipwreck 20m broken hull, 12 debris pieces, one portable culled batch")
    && shipwreckResult.output?.includes(
      "island-simulator: shipwreck broken hull exposes ribs, partial deck, and snapped mast")
    && shipwreckResult.output?.includes(
      "island-simulator: shipwreck QA camera views the 20m wreck from the beach")
    && shipwreckResult.output?.includes("shipwreck visible, 12 debris, 1 draw")
    && shipwreckResult.output?.includes(
      "island-simulator: ocean portable custom shader")
    && !shipwreckResult.error;

  await call("Page.navigate", {
    url: `http://127.0.0.1:${port}/island-smoke.html?mode=crabs`,
  }, visualSessionId);
  let crabResult = {};
  for (let attempt = 0; attempt < 200; ++attempt) {
    await delay(100);
    const evaluation = await call("Runtime.evaluate", {
      expression: "JSON.stringify(globalThis.islandSmokeResult ?? {})",
      returnByValue: true,
    }, visualSessionId);
    crabResult = JSON.parse(evaluation.result.value ?? "{}");
    if (crabResult.output?.includes("island-simulator: terrain")
        || crabResult.error) break;
  }
  await delay(1000);
  const completedCrabEvaluation = await call("Runtime.evaluate", {
    expression: "JSON.stringify(globalThis.islandSmokeResult ?? {})",
    returnByValue: true,
  }, visualSessionId);
  crabResult = JSON.parse(completedCrabEvaluation.result.value ?? "{}");

  const crabScreenshotPath = await captureVisualFrame(
    visualSessionId, "island-browser-crabs.png", true);
  const crabPassed = crabResult.output?.includes("island-simulator: renderer WEBGL2")
    && crabResult.output?.includes(
      "island-simulator: beach crabs 30 agents in deterministic sand-constrained colonies")
    && crabResult.output?.includes(
      "island-simulator: beach crabs use articulated legs, claws, and one culled batch")
    && crabResult.output?.includes(
      "island-simulator: crab QA camera views an active beach colony")
    && /crabs [1-9][0-9]*\/30 visible \(near [1-9][0-9]*, far [0-9]+\), 1 draw/.test(
      crabResult.output ?? "")
    && !crabResult.error;

  await call("Page.navigate", {
    url: `http://127.0.0.1:${port}/island-smoke.html?mode=butterflies`,
  }, visualSessionId);
  let butterflyResult = {};
  for (let attempt = 0; attempt < 200; ++attempt) {
    await delay(100);
    const evaluation = await call("Runtime.evaluate", {
      expression: "JSON.stringify(globalThis.islandSmokeResult ?? {})",
      returnByValue: true,
    }, visualSessionId);
    butterflyResult = JSON.parse(evaluation.result.value ?? "{}");
    if (butterflyResult.output?.includes("island-simulator: terrain")
        || butterflyResult.error) break;
  }
  await delay(1000);
  const completedButterflyEvaluation = await call("Runtime.evaluate", {
    expression: "JSON.stringify(globalThis.islandSmokeResult ?? {})",
    returnByValue: true,
  }, visualSessionId);
  butterflyResult = JSON.parse(completedButterflyEvaluation.result.value ?? "{}");

  const butterflyScreenshotPath = await captureVisualFrame(
    visualSessionId, "island-browser-butterflies.png", true);
  const butterflyPassed = butterflyResult.output?.includes(
      "island-simulator: renderer WEBGL2")
    && butterflyResult.output?.includes(
      "island-simulator: clearing butterflies 18 agents in deterministic low-altitude habitats")
    && butterflyResult.output?.includes(
      "island-simulator: clearing butterflies use hinged patterned wings in one culled batch")
    && butterflyResult.output?.includes(
      "island-simulator: butterfly QA camera views a sunny clearing group")
    && /butterflies [1-9][0-9]*\/18 visible \(near [1-9][0-9]*, far [0-9]+\), 1 draw/.test(
      butterflyResult.output ?? "")
    && !butterflyResult.error;

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

  const waterScreenshotPath = await captureVisualFrame(
    visualSessionId, "island-browser-water.png", true);
  const waterPassed = waterResult.output?.includes("island-simulator: renderer WEBGL2")
    && waterResult.output?.includes(
      "island-simulator: ocean reused crossing-wave subsurface colour")
    && waterResult.output?.includes(
      "island-simulator: ocean sunward ripple crests reuse analytic slope field")
    && waterResult.output?.includes(
      "island-simulator: ocean grazing reflection bands reuse analytic slope field")
    && waterResult.output?.includes(
      "island-simulator: fixed sunny sky with layered horizontal high wisps, portable shader")
    && waterResult.output?.includes(
      "island-simulator: near-surface swimming view uses generated depth haze, surface scatter, and peripheral veil in one draw")
    && /water view 0[.][1-9][0-9]*, 1 draw, player ripple 0[.][1-9][0-9]*, phase [01](?:[.][0-9]+)?, splash 0[.][1-9][0-9]*, phase 0[.][0-9]+/.test(
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

  const currentScreenshotPath = await captureVisualFrame(
    visualSessionId, "island-browser-current.png", true);
  const currentPassed = currentResult.output?.includes("island-simulator: renderer WEBGL2")
    && currentResult.output?.includes(
      "island-simulator: ocean reused crossing-wave subsurface colour")
    && currentResult.output?.includes(
      "island-simulator: ocean sunward ripple crests reuse analytic slope field")
    && currentResult.output?.includes(
      "island-simulator: ocean grazing reflection bands reuse analytic slope field")
    && currentResult.output?.includes(
      "island-simulator: fixed sunny sky with layered horizontal high wisps, portable shader")
    && currentResult.output?.includes(
      "island-simulator: near-surface swimming view uses generated depth haze, surface scatter, and peripheral veil in one draw")
    && /water view 0[.][1-9][0-9]*, 1 draw, player ripple [01](?:[.][0-9]+)?, phase [01](?:[.][0-9]+)?, splash 0, phase 1, current 0[.][1-9][0-9]*/.test(
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

  const distantScreenshotPath = await captureVisualFrame(
    visualSessionId, "island-browser-distant.png", true);
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

  const frondScreenshotPath = await captureVisualFrame(
    visualSessionId, "island-browser-fronds.png", true);
  const frondPassed = frondResult.output?.includes("island-simulator: renderer WEBGL2")
    && frondResult.output?.includes(
      "island-simulator: fixed sunny sky with layered horizontal high wisps, portable shader")
    && frondResult.output?.includes("natural-prop portable shader with near/far LOD")
    && frondResult.output?.includes(
      "island-simulator: fallen fronds use curved asymmetric leaflet silhouettes")
    && frondResult.output?.includes(
      "island-simulator: fallen frond leaflets use geometric central folds")
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

  const birdScreenshotPath = await captureVisualFrame(
    visualSessionId, "island-browser-birds.png", true);
  const birdPassed = birdResult.output?.includes("island-simulator: renderer WEBGL2")
    && birdResult.output?.includes(
      "island-simulator: fixed sunny sky with layered horizontal high wisps, portable shader")
    && birdResult.output?.includes("island-simulator: birds 24 agents")
    && birdResult.output?.includes(
      "island-simulator: bird flight uses slow radial drift, banking, and intermittent glide")
    && birdResult.output?.includes(
      "island-simulator: near birds use layered trailing flight-feather silhouettes")
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

  const fishScreenshotPath = await captureVisualFrame(
    visualSessionId, "island-browser-fish.png", true);
  const fishPassed = fishResult.output?.includes("island-simulator: renderer WEBGL2")
    && fishResult.output?.includes(
      "island-simulator: fixed sunny sky with layered horizontal high wisps, portable shader")
    && fishResult.output?.includes("island-simulator: fish 48 agents")
    && fishResult.output?.includes(
      "island-simulator: aquatic wildlife uses depth pitch, banked turns, forked tails, and near head detail")
    && fishResult.output?.includes(
      "island-simulator: aquatic wildlife uses depth tint and restrained moving sunlight bands in CPU vertex colours")
    && fishResult.output?.includes(
      "island-simulator: fish near/far geometry 51/12 vertices, single-surface fins and CPU faceted daylight")
    && /fish [1-9][0-9]*\/48 visible/.test(fishResult.output ?? "")
    && !fishResult.error;

  await call("Page.navigate", {
    url: `http://127.0.0.1:${port}/island-smoke.html?mode=turtles`,
  }, visualSessionId);
  let turtleResult = {};
  for (let attempt = 0; attempt < 200; ++attempt) {
    await delay(100);
    const evaluation = await call("Runtime.evaluate", {
      expression: "JSON.stringify(globalThis.islandSmokeResult ?? {})",
      returnByValue: true,
    }, visualSessionId);
    turtleResult = JSON.parse(evaluation.result.value ?? "{}");
    if (turtleResult.output?.includes("island-simulator: terrain")
        || turtleResult.error) break;
  }
  await delay(1000);
  const completedTurtleEvaluation = await call("Runtime.evaluate", {
    expression: "JSON.stringify(globalThis.islandSmokeResult ?? {})",
    returnByValue: true,
  }, visualSessionId);
  turtleResult = JSON.parse(completedTurtleEvaluation.result.value ?? "{}");

  const turtleScreenshotPath = await captureVisualFrame(
    visualSessionId, "island-browser-turtles.png", true);
  const turtlePassed = turtleResult.output?.includes("island-simulator: renderer WEBGL2")
    && turtleResult.output?.includes(
      "island-simulator: fixed sunny sky with layered horizontal high wisps, portable shader")
    && turtleResult.output?.includes(
      "island-simulator: sea turtles 5 agents in bounded near-shore cruise, forage, and surface routes")
    && turtleResult.output?.includes(
      "island-simulator: sea turtles use faceted shells and articulated flippers in one culled batch")
    && turtleResult.output?.includes(
      "island-simulator: sea-turtle QA camera views a surfacing route")
    && /turtles [1-9]\/5 visible \(near [1-9], far [0-9]\), 1 draw/.test(
      turtleResult.output ?? "")
    && !turtleResult.error;

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

  const sharkScreenshotPath = await captureVisualFrame(
    visualSessionId, "island-browser-shark.png", true);
  const sharkPassed = sharkResult.output?.includes("island-simulator: renderer WEBGL2")
    && sharkResult.output?.includes(
      "island-simulator: fixed sunny sky with layered horizontal high wisps, portable shader")
    && sharkResult.output?.includes("island-simulator: one occasional non-hostile offshore shark patrol")
    && sharkResult.output?.includes(
      "island-simulator: aquatic wildlife uses depth pitch, banked turns, forked tails, and near head detail")
    && sharkResult.output?.includes(
      "island-simulator: aquatic wildlife uses depth tint and restrained moving sunlight bands in CPU vertex colours")
    && sharkResult.output?.includes(
      "island-simulator: shark near/far geometry 51/15 vertices, single-surface fins and CPU faceted daylight")
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

  const monkeyScreenshotPath = await captureVisualFrame(
    visualSessionId, "island-browser-monkeys.png");
  const monkeyPassed = monkeyResult.output?.includes("island-simulator: renderer WEBGL2")
    && monkeyResult.output?.includes(
      "island-simulator: fixed sunny sky with layered horizontal high wisps, portable shader")
    && monkeyResult.output?.includes("island-simulator: monkeys 7 agents with portable GPU skeletal animation")
    && monkeyResult.output?.includes("3 silhouettes, 4 palettes")
    && monkeyResult.output?.includes(
      "island-simulator: near monkeys use restrained procedural crown and cheek fur tufts")
    && monkeyResult.output?.includes(
      "island-simulator: near monkeys use bone-following finger and toe silhouettes")
    && monkeyResult.output?.includes(
      "island-simulator: calm monkeys use bounded skeletal head attention")
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

  await call("Page.bringToFront", {}, visualSessionId);
  await delay(250);
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

  const parseInputSamples = output => [...output.matchAll(
    /input sample captured (\d) relative (\d) player ([-+0-9.eE]+) ([-+0-9.eE]+) camera ([-+0-9.eE]+) ([-+0-9.eE]+) ([-+0-9.eE]+) free (\d) yaw ([-+0-9.eE]+) pitch ([-+0-9.eE]+) viewport (\d+) (\d+) run (\d) velocity ([-+0-9.eE]+) ([-+0-9.eE]+)/g)]
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
      running: Number(match[13]),
      velocityX: Number(match[14]),
      velocityZ: Number(match[15]),
    }));

  const concurrentStartLength = (await readSmokeOutput(visualSessionId)).length;
  await call("Input.dispatchKeyEvent", {
    type: "keyDown", key: "w", code: "KeyW", windowsVirtualKeyCode: 87,
    nativeVirtualKeyCode: 87,
  }, visualSessionId);
  await delay(300);
  await call("Input.dispatchMouseEvent", {
    type: "mouseMoved", x: 700, y: 360, button: "none", buttons: 0,
  }, visualSessionId);
  await delay(400);
  await call("Input.dispatchMouseEvent", {
    type: "mouseMoved", x: 620, y: 340, button: "none", buttons: 0,
  }, visualSessionId);
  await delay(400);
  await call("Input.dispatchMouseEvent", {
    type: "mouseMoved", x: 730, y: 390, button: "none", buttons: 0,
  }, visualSessionId);
  await delay(400);
  await call("Input.dispatchMouseEvent", {
    type: "mouseMoved", x: 590, y: 320, button: "none", buttons: 0,
  }, visualSessionId);
  await delay(400);
  await call("Input.dispatchKeyEvent", {
    type: "keyUp", key: "w", code: "KeyW", windowsVirtualKeyCode: 87,
    nativeVirtualKeyCode: 87,
  }, visualSessionId);
  await delay(400);
  const concurrentSamples = parseInputSamples(
    (await readSmokeOutput(visualSessionId)).slice(concurrentStartLength));
  const concurrentFirst = concurrentSamples[0];
  const concurrentLast = concurrentSamples.at(-1);
  const concurrentYawRange = concurrentSamples.length > 0
    ? Math.max(...concurrentSamples.map(sample => sample.yaw))
      - Math.min(...concurrentSamples.map(sample => sample.yaw))
    : 0;
  const simultaneousMoveAndLook = concurrentFirst && concurrentLast
    && Math.hypot(concurrentLast.x - concurrentFirst.x,
      concurrentLast.z - concurrentFirst.z) > 0.35
    && concurrentYawRange > 0.005;

  await dispatchKeyPress(visualSessionId, "F5", "F5", 116, 650, 400);
  const arrowStartLength = (await readSmokeOutput(visualSessionId)).length;
  await dispatchKeyPress(visualSessionId, "ArrowRight", "ArrowRight", 39, 900, 400);
  const arrowSamples = parseInputSamples(
    (await readSmokeOutput(visualSessionId)).slice(arrowStartLength));
  const arrowFirst = arrowSamples[0];
  const arrowLast = arrowSamples.at(-1);
  const rightArrowMovedRight = arrowFirst && arrowLast
    && arrowLast.x < arrowFirst.x - 0.25;

  await dispatchKeyPress(visualSessionId, "F5", "F5", 116, 650, 400);
  // Confirm each toggle edge from game output. A single short synthetic edge can fall entirely
  // between frames when SwiftShader and the browser share a heavily loaded worker.
  await call("Input.dispatchKeyEvent", {
    type: "keyDown", key: "Shift", code: "ShiftLeft", windowsVirtualKeyCode: 16,
    nativeVirtualKeyCode: 16,
  }, visualSessionId);
  const runModeEnabled = await waitForOutput(
    visualSessionId, "island-simulator: run mode enabled (8.0 m/s)");
  await delay(100);
  const runModeScreenshot = await call(
    "Page.captureScreenshot", { format: "png" }, visualSessionId);
  const runModeScreenshotPath = path.join(
    buildDirectory, "island-browser-run-mode.png");
  await writeFile(runModeScreenshotPath, Buffer.from(runModeScreenshot.data, "base64"));
  await call("Input.dispatchKeyEvent", {
    type: "keyUp", key: "Shift", code: "ShiftLeft", windowsVirtualKeyCode: 16,
    nativeVirtualKeyCode: 16,
  }, visualSessionId);
  await delay(350);
  const runStartLength = (await readSmokeOutput(visualSessionId)).length;
  await call("Input.dispatchKeyEvent", {
    type: "keyDown", key: "ArrowUp", code: "ArrowUp", windowsVirtualKeyCode: 38,
    nativeVirtualKeyCode: 38,
  }, visualSessionId);
  for (let attempt = 0; attempt < 20; ++attempt) {
    await delay(250);
    const activeRunSamples = parseInputSamples(
      (await readSmokeOutput(visualSessionId)).slice(runStartLength));
    if (activeRunSamples.some(sample => sample.running === 1
      && Math.hypot(sample.velocityX, sample.velocityZ) > 7.0)) break;
  }
  await call("Input.dispatchKeyEvent", {
    type: "keyUp", key: "ArrowUp", code: "ArrowUp", windowsVirtualKeyCode: 38,
    nativeVirtualKeyCode: 38,
  }, visualSessionId);
  await delay(400);
  const runSamples = parseInputSamples(
    (await readSmokeOutput(visualSessionId)).slice(runStartLength));
  const runFirst = runSamples[0];
  const runLast = runSamples.at(-1);
  const retainedRunWorked = runFirst && runLast
    && runLast.z > runFirst.z + 0.35
    && runSamples.some(sample => sample.running === 1
      && Math.hypot(sample.velocityX, sample.velocityZ) > 7.0);

  const runModeDisabled = await dispatchKeyUntilOutput(
    visualSessionId, "Shift", "ShiftLeft", 16,
    "island-simulator: run mode disabled (4.3 m/s)");
  await dispatchKeyPress(visualSessionId, "F5", "F5", 116, 650, 400);
  const walkStartLength = (await readSmokeOutput(visualSessionId)).length;
  await call("Input.dispatchKeyEvent", {
    type: "keyDown", key: "ArrowUp", code: "ArrowUp", windowsVirtualKeyCode: 38,
    nativeVirtualKeyCode: 38,
  }, visualSessionId);
  for (let attempt = 0; attempt < 20; ++attempt) {
    await delay(250);
    const activeWalkSamples = parseInputSamples(
      (await readSmokeOutput(visualSessionId)).slice(walkStartLength));
    if (activeWalkSamples.some(sample => sample.running === 0
      && Math.hypot(sample.velocityX, sample.velocityZ) > 3.5)) break;
  }
  await call("Input.dispatchKeyEvent", {
    type: "keyUp", key: "ArrowUp", code: "ArrowUp", windowsVirtualKeyCode: 38,
    nativeVirtualKeyCode: 38,
  }, visualSessionId);
  await delay(400);
  const walkSamples = parseInputSamples(
    (await readSmokeOutput(visualSessionId)).slice(walkStartLength));
  const walkFirst = walkSamples[0];
  const walkLast = walkSamples.at(-1);
  const restoredWalkWorked = walkFirst && walkLast
    && walkLast.z > walkFirst.z + 0.35
    && walkSamples.some(sample => sample.running === 0
      && Math.hypot(sample.velocityX, sample.velocityZ) > 3.5)
    && walkSamples.every(sample => sample.running === 0
      && Math.hypot(sample.velocityX, sample.velocityZ) <= 4.35);
  const runToggleWorked = runModeEnabled && retainedRunWorked
    && runModeDisabled && restoredWalkWorked;

  await dispatchKeyUntilOutput(
    visualSessionId, "b", "KeyB", 66, "island-simulator: camera motion disabled");
  await dispatchKeyUntilOutput(
    visualSessionId, "b", "KeyB", 66, "island-simulator: camera motion enabled");
  await dispatchKeyPress(visualSessionId, "F5", "F5", 116, 650, 400);
  await call("Input.dispatchKeyEvent", {
    type: "keyDown", key: "s", code: "KeyS", windowsVirtualKeyCode: 83,
    nativeVirtualKeyCode: 83,
  }, visualSessionId);
  for (let attempt = 0; attempt < 15; ++attempt) {
    await delay(1000);
    if ((await readSmokeOutput(visualSessionId)).includes(
      "island-simulator: first traversal entered swimming state")) break;
  }
  await call("Input.dispatchKeyEvent", {
    type: "keyUp", key: "s", code: "KeyS", windowsVirtualKeyCode: 83,
    nativeVirtualKeyCode: 83,
  }, visualSessionId);
  await delay(300);

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

  const escapeOutputLengthBefore = (await readSmokeOutput(visualSessionId)).length;
  await dispatchKeyPress(visualSessionId, "Escape", "Escape", 27, 250, 350);
  const unlockedEvaluation = await call("Runtime.evaluate", {
    expression: "document.pointerLockElement === null",
    returnByValue: true,
  }, visualSessionId);
  const pointerUnlocked = unlockedEvaluation.result.value === true;
  const escapeStateEvaluation = await call("Runtime.evaluate", {
    expression: `JSON.stringify({
      exited: globalThis.islandSmokeResult?.exited === true,
      error: globalThis.islandSmokeResult?.error ?? "",
      outputLength: (globalThis.islandSmokeResult?.output ?? "").length
    })`,
    returnByValue: true,
  }, visualSessionId);
  const escapeState = JSON.parse(escapeStateEvaluation.result.value ?? "{}");
  const escapeKeptSessionActive = pointerUnlocked
    && escapeState.exited === false
    && escapeState.error === ""
    && escapeState.outputLength > escapeOutputLengthBefore;
  // Chromium rejects a new request made immediately after Escape releases pointer lock.
  // Model a deliberate follow-up click rather than racing that browser security cooldown.
  await delay(1250);
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
  let lifecycleResumed = false;
  // A resumed SwiftShader page can spend several seconds rebuilding browser/GPU state. Poll the
  // authoritative game samples rather than assuming a fixed resume latency.
  for (let attempt = 0; attempt < 20; ++attempt) {
    await delay(500);
    const resumedEvaluation = await call("Runtime.evaluate", {
      expression: "globalThis.islandSmokeResult?.output ?? ''",
      returnByValue: true,
    }, visualSessionId);
    const resumedSamples = (resumedEvaluation.result.value ?? "")
      .match(/island-simulator: input sample/g)?.length ?? 0;
    if (resumedSamples > samplesBeforeLifecycle) {
      lifecycleResumed = true;
      break;
    }
  }

  const completedInputEvaluation = await call("Runtime.evaluate", {
    expression: "JSON.stringify(globalThis.islandSmokeResult ?? {})",
    returnByValue: true,
  }, visualSessionId);
  inputResult = JSON.parse(completedInputEvaluation.result.value ?? "{}");
  const samples = parseInputSamples(inputResult.output ?? "");
  const initialSample = samples[0];
  const finalSample = samples.at(-1);
  const keyboardMoved = initialSample && finalSample
    && Math.hypot(finalSample.x - initialSample.x, finalSample.z - initialSample.z) > 0.5;
  const mouseLookChanged = concurrentYawRange > 0.005;
  const waterTransitionPlayed = inputResult.output?.includes(
    "island-simulator: first water-transition accent played");
  const swimmingStateEntered = inputResult.output?.includes(
    "island-simulator: first traversal entered swimming state");
  const landFootstepPlayed = inputResult.output?.includes(
    "island-simulator: first distance-scheduled land footstep played");
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
      "island-simulator: keyboard run/walk toggle uses a transient mode confirmation")
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
    && simultaneousMoveAndLook
    && rightArrowMovedRight
    && runToggleWorked
    && cameraMotionToggled
    && landFootstepPlayed
    && waterTransitionPlayed
    && swimmingStateEntered
    && mouseLookChanged
    && viewportResized
    && pointerUnlocked
    && escapeKeptSessionActive
    && pointerRelocked
    && !inputResult.error;

  const passed = smokePassed && deploymentPassed && webSettingsPersistencePassed
    && visualPassed && beachPassed && grassPassed
    && groundPassed && litterPassed
    && palmPassed && junglePassed && eastClearingPassed && westClearingPassed
    && propPassed && driftwoodPassed && reefPassed && shipwreckPassed && crabPassed
    && butterflyPassed
    && waterPassed && currentPassed
    && distantPassed && frondPassed
    && birdPassed && fishPassed && turtlePassed && sharkPassed && monkeyPassed && inputPassed;
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
      status: deploymentState.status ?? "",
      width: deploymentState.width ?? 0,
      height: deploymentState.height ?? 0,
      hasWebGl2: deploymentState.hasWebGl2 === true,
      pointerLocked: deploymentState.pointerLocked === true,
      audioStarted: deploymentState.audioStarted === true,
      audioStagingBounded: deploymentState.audioStagingBounded === true,
      ambientPcmDcCentered: deploymentState.ambientPcmDcCentered === true,
      ambientBedVariationEnabled: deploymentState.ambientBedVariationEnabled === true,
      waveAccentsEnabled: deploymentState.waveAccentsEnabled === true,
      waterMovementEnabled: deploymentState.waterMovementEnabled === true,
      waterTransitionsEnabled: deploymentState.waterTransitionsEnabled === true,
      waterTransitionVariantsEnabled: deploymentState.waterTransitionVariantsEnabled === true,
      landFootstepsEnabled: deploymentState.landFootstepsEnabled === true,
      jungleAmbienceEnabled: deploymentState.jungleAmbienceEnabled === true,
      waveVariantsEnabled: deploymentState.waveVariantsEnabled === true,
      positionalWildlifeCallsEnabled:
        deploymentState.positionalWildlifeCallsEnabled === true,
      output: deploymentState.output ?? "",
    },
    settingsPersistence: {
      passed: webSettingsPersistencePassed,
      written: webSettingsWritten,
      writeOutput: settingsWriteOutput,
      reloadOutput: settingsReloadOutput,
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
    litter: {
      passed: litterPassed,
      output: litterResult.output ?? "",
      error: litterResult.error ?? "",
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
    shipwreck: {
      passed: shipwreckPassed,
      output: shipwreckResult.output ?? "",
      error: shipwreckResult.error ?? "",
    },
    crabs: {
      passed: crabPassed,
      output: crabResult.output ?? "",
      error: crabResult.error ?? "",
    },
    butterflies: {
      passed: butterflyPassed,
      output: butterflyResult.output ?? "",
      error: butterflyResult.error ?? "",
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
    turtles: {
      passed: turtlePassed,
      output: turtleResult.output ?? "",
      error: turtleResult.error ?? "",
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
      simultaneousMoveAndLook: Boolean(simultaneousMoveAndLook),
      rightArrowMovedRight: Boolean(rightArrowMovedRight),
      runModeEnabled: Boolean(runModeEnabled),
      retainedRunWorked: Boolean(retainedRunWorked),
      runModeDisabled: Boolean(runModeDisabled),
      restoredWalkWorked: Boolean(restoredWalkWorked),
      cameraMotionToggled: Boolean(cameraMotionToggled),
      waterTransitionPlayed: Boolean(waterTransitionPlayed),
      swimmingStateEntered: Boolean(swimmingStateEntered),
      mouseLookChanged: Boolean(mouseLookChanged),
      viewportResized: Boolean(viewportResized),
      pointerUnlocked,
      escapeKeptSessionActive,
      escapeState,
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
    litterScreenshot: path.basename(litterScreenshotPath),
    palmScreenshot: path.basename(palmScreenshotPath),
    jungleScreenshot: path.basename(jungleScreenshotPath),
    eastClearingScreenshot: path.basename(eastClearingCapture.screenshotPath),
    westClearingScreenshot: path.basename(westClearingCapture.screenshotPath),
    propScreenshot: path.basename(propScreenshotPath),
    driftwoodScreenshot: path.basename(driftwoodScreenshotPath),
    reefScreenshot: path.basename(reefScreenshotPath),
    shipwreckScreenshot: path.basename(shipwreckScreenshotPath),
    crabScreenshot: path.basename(crabScreenshotPath),
    butterflyScreenshot: path.basename(butterflyScreenshotPath),
    waterScreenshot: path.basename(waterScreenshotPath),
    currentScreenshot: path.basename(currentScreenshotPath),
    distantScreenshot: path.basename(distantScreenshotPath),
    frondScreenshot: path.basename(frondScreenshotPath),
    birdScreenshot: path.basename(birdScreenshotPath),
    fishScreenshot: path.basename(fishScreenshotPath),
    turtleScreenshot: path.basename(turtleScreenshotPath),
    sharkScreenshot: path.basename(sharkScreenshotPath),
    monkeyScreenshot: path.basename(monkeyScreenshotPath),
    runModeScreenshot: path.basename(runModeScreenshotPath),
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
