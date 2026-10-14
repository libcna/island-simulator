import { spawn } from "node:child_process";
import { once } from "node:events";
import { mkdtemp, readFile, rm, writeFile } from "node:fs/promises";
import path from "node:path";
import { fileURLToPath } from "node:url";
import { requireBrowserContainment, recordBrowserProcess } from "./browser_containment.mjs";
import { acknowledgedInputGesture } from "./browser_input_gesture.mjs";

requireBrowserContainment("firefox");

const toolsDirectory = path.dirname(fileURLToPath(import.meta.url));
const projectRoot = path.resolve(toolsDirectory, "..");
const buildDirectory = path.resolve(process.argv[2] ?? path.join(projectRoot, "build-probe"));
const artifactDirectory = process.env.ISLAND_BROWSER_EVIDENCE;
const webPort = Number(process.env.ISLAND_FIREFOX_WEB_PORT ?? "18765");
const remotePort = Number(process.env.ISLAND_FIREFOX_REMOTE_PORT ?? "19223");
const firefoxExecutable = process.env.ISLAND_FIREFOX ?? "firefox-esr";
const nodeHasWebSocket = typeof WebSocket !== "undefined";

if (!nodeHasWebSocket) {
  throw new Error("Firefox smoke requires a Node runtime with the global WebSocket API");
}
for (const [name, value] of [["web", webPort], ["remote", remotePort]]) {
  if (!Number.isInteger(value) || value < 1024 || value > 65535) {
    throw new Error(`Invalid ${name} port: ${value}`);
  }
}
await Promise.all([
  readFile(path.join(buildDirectory, "island-simulator.html")),
  readFile(path.join(buildDirectory, "island-simulator.js")),
  readFile(path.join(buildDirectory, "island-simulator.wasm")),
  readFile(path.join(buildDirectory, "island-simulator.data")),
]);
const smokeTemplate = await readFile(path.join(toolsDirectory, "web_smoke.html.in"), "utf8");
await writeFile(path.join(buildDirectory, "island-smoke.html"), smokeTemplate);

const profile = await mkdtemp(path.join(artifactDirectory, "firefox-profile-"));
const server = spawn("python3", [
  "-m", "http.server", String(webPort), "--bind", "127.0.0.1",
  "--directory", buildDirectory,
], { stdio: ["ignore", "ignore", "pipe"] });
let serverError = "";
server.stderr.on("data", chunk => { serverError += chunk; });

let firefox;
let firefoxLog = "";
let displayServer;
let displayLog = "";
let browserSocket;
let bidiCommand;
let collectPageEvidence;
const escapeGestures = [];
const shiftGestures = [];
const delay = milliseconds => new Promise(resolve => setTimeout(resolve, milliseconds));
const countMarker = (text, marker) => text.split(marker).length - 1;

const stopProcess = async child => {
  if (!child || child.exitCode !== null || child.signalCode !== null) return;
  child.kill("SIGTERM");
  await Promise.race([once(child, "exit"), delay(2000)]);
  if (child.exitCode === null && child.signalCode === null) {
    child.kill("SIGKILL");
    await Promise.race([once(child, "exit"), delay(2000)]);
  }
};

const connectWebSocket = endpoint => new Promise((resolve, reject) => {
  const socket = new WebSocket(endpoint);
  const connected = () => {
    socket.removeEventListener("error", failed);
    resolve(socket);
  };
  const failed = event => {
    socket.removeEventListener("open", connected);
    socket.close();
    reject(event.error ?? new Error(`Unable to connect to ${endpoint}`));
  };
  socket.addEventListener("open", connected, { once: true });
  socket.addEventListener("error", failed, { once: true });
});

try {
  let serverReady = false;
  for (let attempt = 0; attempt < 50; ++attempt) {
    try {
      const response = await fetch(
        `http://127.0.0.1:${webPort}/island-simulator.html`);
      if (response.ok) {
        serverReady = true;
        break;
      }
    } catch {
      await delay(100);
    }
  }
  if (!serverReady) throw new Error(`Local Firefox server did not start: ${serverError}`);

  // Firefox's Linux headless backend refuses WebGL2 in this GPU-free namespace. Give it an
  // owned Xvfb framebuffer, still with no GPU devices and forced Mesa software rendering.
  // Let Xvfb allocate an unused display; never attach to the operator's desktop.
  const displayArguments = ["-displayfd", "3", "-screen", "0", "1280x720x24", "-nolisten", "tcp"];
  displayServer = spawn("Xvfb", displayArguments, { stdio: ["ignore", "ignore", "pipe", "pipe"] });
  let displayNumber = "", displayFailure = "";
  displayServer.on("error", error => { displayFailure = String(error); });
  displayServer.stderr.on("data", chunk => { displayLog += chunk; });
  displayServer.stdio[3].on("data", chunk => { displayNumber += chunk; });
  for (let attempt = 0; attempt < 100 && !displayNumber.includes("\n"); ++attempt) {
    if (displayFailure || displayServer.exitCode !== null || displayServer.signalCode !== null) break;
    await delay(100);
  }
  if (!/^\d+\n$/.test(displayNumber)) {
    throw new Error(`Private Xvfb startup failed: ${displayFailure || displayLog || "no display acknowledgement"}`);
  }
  const display = `:${displayNumber.trim()}`;
  const displayIdentity = recordBrowserProcess(displayServer, "Xvfb", "display-process.json");
  await writeFile(path.join(artifactDirectory, "firefox-display.json"),
    `${JSON.stringify({ ...displayIdentity, command: ["Xvfb", ...displayArguments], display }, null, 2)}\n`);
  const firefoxEnvironment = { ...process.env, DISPLAY: display };
  delete firefoxEnvironment.MOZ_HEADLESS;
  // Inherit the supervisor's <=8 available CPUs instead of assuming CPUs 0--7 exist.
  firefox = spawn(firefoxExecutable, [
    "--no-remote", "--profile", profile,
    "--remote-debugging-port", String(remotePort), "about:blank",
  ], {
    cwd: profile,
    env: firefoxEnvironment,
    stdio: ["ignore", "ignore", "pipe"],
  });
  firefox.stderr.on("data", chunk => { firefoxLog += chunk; });
  const browserIdentity = recordBrowserProcess(firefox, "firefox");

  for (let attempt = 0; attempt < 120; ++attempt) {
    try {
      browserSocket = await connectWebSocket(`ws://127.0.0.1:${remotePort}/session`);
      break;
    } catch {
      await delay(100);
    }
  }
  if (!browserSocket) {
    throw new Error(`Firefox did not expose WebDriver BiDi:\n${firefoxLog}`);
  }

  let nextId = 1;
  const pending = new Map();
  browserSocket.addEventListener("message", event => {
    const message = JSON.parse(event.data);
    if (!message.id || !pending.has(message.id)) return;
    const callback = pending.get(message.id);
    pending.delete(message.id);
    if (message.type === "error") {
      callback.reject(new Error(`${message.error}: ${message.message}`));
    } else {
      callback.resolve(message.result);
    }
  });
  bidiCommand = (method, params = {}) => new Promise((resolve, reject) => {
    const id = nextId++;
    pending.set(id, { resolve, reject });
    browserSocket.send(JSON.stringify({ id, method, params }));
  });

  const evaluate = async (context, expression) => {
    const response = await bidiCommand("script.evaluate", {
      expression,
      target: { context },
      awaitPromise: false,
      resultOwnership: "none",
    });
    if (response.type !== "success") {
      throw new Error(`Firefox script evaluation failed: ${response.type}`);
    }
    return response.result.value;
  };
  const capture = async (context, filename) => {
    const result = await bidiCommand("browsingContext.captureScreenshot", {
      context,
      origin: "viewport",
      format: { type: "image/png" },
    });
    await writeFile(path.join(artifactDirectory, filename), Buffer.from(result.data, "base64"));
  };
  const performKeyActions = async (context, actions) => bidiCommand(
    "input.performActions",
    { context, actions: [{ type: "key", id: "keyboard", actions }] });
  const performPointerClick = async (context, x, y) => bidiCommand(
    "input.performActions",
    {
      context,
      actions: [{
        type: "pointer",
        id: "mouse",
        parameters: { pointerType: "mouse" },
        actions: [
          { type: "pointerMove", x, y, duration: 0 },
          { type: "pointerDown", button: 0 },
          // A short genuine click must work even when both edges fall between game updates.
          { type: "pause", duration: 0 },
          { type: "pointerUp", button: 0 },
        ],
      }],
    });
  const waitForOutput = async (context, marker, attempts = 120) => {
    let output = "";
    for (let attempt = 0; attempt < attempts; ++attempt) {
      output = await evaluate(context, "globalThis.islandDeploymentOutput || ''");
      if (output.includes(marker)) return output;
      await delay(100);
    }
    throw new Error(`Firefox output did not contain ${marker}:\n${output.slice(-5000)}`);
  };
  const waitForMarkerCount = async (context, marker, minimum, attempts = 120) => {
    let output = "";
    for (let attempt = 0; attempt < attempts; ++attempt) {
      output = await evaluate(context, "globalThis.islandDeploymentOutput || ''");
      if (countMarker(output, marker) >= minimum) return output;
      await delay(100);
    }
    throw new Error(
      `Firefox output did not contain ${minimum} occurrences of ${marker}:\n${output.slice(-5000)}`);
  };
  const qualifyScene = async (context, mode, filename, requiredMarkers, populationPattern) => {
    await bidiCommand("browsingContext.setViewport", {
      context,
      viewport: { width: 1280, height: 720 },
      devicePixelRatio: 1,
    });
    await bidiCommand("browsingContext.navigate", {
      context,
      url: `http://127.0.0.1:${webPort}/island-smoke.html?mode=${mode}`,
      wait: "complete",
    });

    let result = {};
    for (let attempt = 0; attempt < 240; ++attempt) {
      result = JSON.parse(await evaluate(
        context, "JSON.stringify(globalThis.islandSmokeResult ?? {})"));
      if (result.error) throw new Error(`Firefox ${mode} scene failed: ${result.error}`);
      if (result.output?.includes("island-simulator: visible terrain cells ")) break;
      await delay(100);
    }
    if (!result.output?.includes("island-simulator: visible terrain cells ")) {
      throw new Error(`Firefox ${mode} scene did not render:\n${result.output?.slice(-5000) ?? ""}`);
    }
    await delay(1000);
    result = JSON.parse(await evaluate(
      context, "JSON.stringify(globalThis.islandSmokeResult ?? {})"));
    for (const marker of requiredMarkers) {
      if (!result.output?.includes(marker)) {
        throw new Error(`Firefox ${mode} scene omitted: ${marker}`);
      }
    }
    if (!populationPattern.test(result.output ?? "")) {
      throw new Error(`Firefox ${mode} scene omitted its expected live population/state`);
    }
    if (result.error || /fatal error|shader.*(?:failed|error)/i.test(result.output ?? "")) {
      throw new Error(`Firefox ${mode} scene emitted a fatal or shader diagnostic`);
    }
    await capture(context, filename);
    return {
      qualified: true,
      visibleState: result.output.split("\n").find(
        line => line.startsWith("island-simulator: visible terrain cells ")) ?? "",
    };
  };

  const session = await bidiCommand("session.new", { capabilities: { alwaysMatch: {} } });
  const created = await bidiCommand("browsingContext.create", { type: "tab" });
  const context = created.context;
  collectPageEvidence = () => evaluate(context, `JSON.stringify({
    output: globalThis.islandDeploymentOutput ?? globalThis.islandSmokeResult?.output ?? "",
    error: globalThis.islandSmokeResult?.error ?? "", focused: document.hasFocus(),
    pointerLocked: document.pointerLockElement === document.getElementById("canvas"),
    inputEvents: globalThis.islandSmokeResult?.inputEvents ?? [] })`);
  const preflightRenderer = JSON.parse(await evaluate(context, `JSON.stringify((() => {
    const canvas = document.createElement("canvas");
    let creationError = "";
    canvas.addEventListener("webglcontextcreationerror", event => { creationError = event.statusMessage; });
    const gl = canvas.getContext("webgl2");
    const debug = gl?.getExtension("WEBGL_debug_renderer_info");
    const renderer = debug ? gl.getParameter(debug.UNMASKED_RENDERER_WEBGL) : "";
    const result = { webgl2: !!gl, debugRendererInfo: !!debug, renderer, creationError,
      maskedRenderer: gl?.getParameter(gl.RENDERER) ?? "", version: gl?.getParameter(gl.VERSION) ?? "",
      userAgent: navigator.userAgent };
    gl?.getExtension("WEBGL_lose_context")?.loseContext();
    return result;
  })())`));
  await writeFile(path.join(artifactDirectory, "firefox-renderer-preflight.json"),
    `${JSON.stringify(preflightRenderer, null, 2)}\n`);
  if (!/llvmpipe|softpipe|SwiftShader/i.test(preflightRenderer.renderer)) {
    throw new Error(`Firefox software renderer not established before game load: ${JSON.stringify(preflightRenderer)}`);
  }
  console.log(`Firefox host PID ${browserIdentity.outerPid}, namespace PID ${firefox.pid}: ${preflightRenderer.renderer}`);
  await bidiCommand("browsingContext.setViewport", {
    context,
    viewport: { width: 1280, height: 720 },
    devicePixelRatio: 1,
  });
  await bidiCommand("browsingContext.navigate", {
    context,
    url: `http://127.0.0.1:${webPort}/island-simulator.html`,
    wait: "complete",
  });

  let output = await waitForOutput(
    context, "island-simulator: viewport 1280x720 logical backbuffer", 240);
  const requiredFirstFrameMarkers = [
    "island-simulator: edge antialiasing requested 4x renderer-applied 4x",
    "island-simulator: grass anchor-distance contraction uses nearest-bound cell culling",
    "island-simulator: juvenile ground broadleaf uses layered arching laminae, readable midribs, and retained near/far leaf identities",
    "island-simulator: split ground leaves use layered asymmetric lobes, readable midribs, shared LOD knots, and root-anchored wind",
    "island-simulator: shrubs use supported shoots and curved individual leaves with shared LOD identities and complete leaf outlines",
    "island-simulator: forest-floor colonies retain route footprints; groups=",
    "island-simulator: jungle root origins and culling bounds follow rendered terrain triangles",
    "island-simulator: fallen leaves conform to rendered triangles using the shared grass height texture",
    "island-simulator: broadleaf paddle and lance laminae share cupped profiles and asymmetric margins across LOD",
    "island-simulator: pinnate understory shares rachis attachment knots and retained leaflet wind across LOD",
    "island-simulator: renderer WEBGL2",
    "island-simulator: quality profile full grass 135m vegetation 100% shadows 100% wildlife 100% ocean 160x160 textures 10091644/10111860 bytes audio 6/6 sources",
    "forest-floor material loaded Content/terrain/forest-floor.png; 512x512, 10 mips, 1398100 RGBA8 bytes; interior windows",
    "leaf material loaded Content/vegetation/leaf-lamina.png; 256x256, 9 mips, 349524 RGBA8 bytes",
    "grass pigment uses separate height occlusion and filtered longitudinal veins",
    "island-simulator: forest-floor filter ",
    "island-simulator: clearing floors reuse existing noise fields for broad patch breakup",
    "island-simulator: terrain interior humus and moss reuse portable material fields",
    "island-simulator: grass near blades use asymmetric edges and occasional bent tips",
    "island-simulator: grass instances use area-preserving asymmetric tuft footprints",
    "island-simulator: jungle per-instance foliage tint reuses existing transforms",
    "island-simulator: jungle near canopy uses irregular layered lower-crown edges",
    "island-simulator: jungle near canopy uses irregular trunks and bent support branches",
    "island-simulator: jungle ground layers receive shared canopy shade; crowns excluded",
    "island-simulator: boars receive bilinear per-vertex canopy light in their existing batch",
    "island-simulator: boar RGB bristle coat and solid anatomy, two opaque stock BasicEffect draws",
    "island-simulator: jungle leaves use smooth normals, filtered veins and restrained wax shading",
    "island-simulator: boars use connected limb poses, stance contact and body-rooted bristles",
    "island-simulator: monkeys receive footprint-averaged canopy light through 14-bone skinning",
    "island-simulator: selected near canopy variants include hanging leafy lianas",
    "island-simulator: palm per-instance frond tint reuses existing transforms",
    "island-simulator: selected palm near crowns include sparse hanging dry fronds",
    "island-simulator: near fallen logs use irregular broken end contours",
    "island-simulator: near fallen-log end grain uses restrained radial cracks",
    "island-simulator: rock per-instance tint reuses existing transforms",
    "island-simulator: shipwreck portable aged wood, waterline, and salt-wear material",
    "island-simulator: shipwreck wood loaded Content/structures/shipwreck-weathered-wood.png; 256x256, 9 mips, 349524 RGBA8 bytes; mirrored interior atlas",
    "island-simulator: shipwreck weathered wood follows per-piece board and beam UVs",
    "island-simulator: shipwreck restrained rusted iron straps, rivets, hinges and rudder braces",
    "island-simulator: shipwreck 20m broken hull, 12 debris pieces, one portable culled batch",
    "island-simulator: shipwreck large-ship silhouette uses deck rails, hull openings, keel, rudder, and a broken foremast",
    "island-simulator: clearing butterflies use hinged patterned wings in one culled batch",
    "island-simulator: sea turtles use faceted shells and articulated flippers in one culled batch",
    "island-simulator: ocean sunward ripple crests reuse analytic slope field",
    "island-simulator: ocean grazing reflection bands reuse analytic slope field",
    "island-simulator: shoreline advancing wash uses footprint-filtered porous foam",
    "island-simulator: ocean shared nine-wave spectrum uses filtered short-wave normals",
    "island-simulator: ocean shallow attenuation uses interpolated bed depth and contact fade",
    "island-simulator: coastal sand stays continuous below water with filtered seabed ripples",
    "island-simulator: startup world ",
    "island-simulator: visible terrain cells ",
  ];
  for (const marker of requiredFirstFrameMarkers) {
    if (!output.includes(marker)) throw new Error(`Firefox first frame omitted: ${marker}`);
  }
  const renderer = JSON.parse(await evaluate(context, `JSON.stringify((() => {
    const gl = document.getElementById("canvas").getContext("webgl2");
    return {
      userAgent: navigator.userAgent,
      webgl2: Boolean(gl),
      version: gl ? gl.getParameter(gl.VERSION) : "",
      renderer: gl ? gl.getParameter(gl.RENDERER) : ""
    };
  })())`));
  if (!renderer.webgl2 || renderer.version !== "WebGL 2.0") {
    throw new Error(`Firefox did not expose WebGL 2: ${JSON.stringify(renderer)}`);
  }
  const releaseWithEscape = async () => {
    const evidence = {};
    escapeGestures.push(evidence);
    const marker = "island-simulator: Escape released browser pointer lock without exiting";
    const capturedMarker = "island-simulator: browser Escape state before release captured 1 relative 1";
    const beforeCount = countMarker(await evaluate(context, "globalThis.islandDeploymentOutput || ''"), marker);
    await acknowledgedInputGesture({ evidence,
      inspect: async () => JSON.parse(await collectPageEvidence()),
      press: () => performKeyActions(context, [{ type: "keyDown", value: "\uE00C" }]),
      acknowledge: async () => {
        // Require a fresh game Escape edge AND actual browser release; no DOM lock mutation.
        const acknowledgedOutput = await waitForMarkerCount(context, marker, beforeCount + 1);
        if (countMarker(acknowledgedOutput, capturedMarker) !== beforeCount + 1) {
          throw new Error("Firefox short click did not establish game/CNA relative capture");
        }
        for (let attempt = 0; attempt < 120; ++attempt) {
          if (await evaluate(context, "document.pointerLockElement === null")) return;
          await delay(100);
        }
        throw new Error("Firefox Escape acknowledged by game but browser lock remained");
      },
      release: () => performKeyActions(context, [{ type: "keyUp", value: "\uE00C" }]),
    });
  };
  const toggleWithShift = async marker => {
    const evidence = { marker };
    shiftGestures.push(evidence);
    const beforeCount = countMarker(await evaluate(context, "globalThis.islandDeploymentOutput || ''"), marker);
    await acknowledgedInputGesture({ evidence,
      inspect: async () => JSON.parse(await collectPageEvidence()),
      press: () => performKeyActions(context, [{ type: "keyDown", value: "\uE008" }]),
      acknowledge: () => waitForMarkerCount(context, marker, beforeCount + 1),
      release: () => performKeyActions(context, [{ type: "keyUp", value: "\uE008" }]),
    });
    // Observe frame progression after release instead of assuming a fixed pause spans updates.
    await bidiCommand("script.evaluate", {
      expression: "new Promise(resolve => requestAnimationFrame(() => requestAnimationFrame(() => resolve(true))))",
      target: { context }, awaitPromise: true, resultOwnership: "none",
    });
  };
  await capture(context, "island-firefox-esr.png");

  await performPointerClick(context, 640, 360);
  await delay(300);
  const pointerLocked = await evaluate(
    context, "document.pointerLockElement === document.getElementById('canvas')");
  if (!pointerLocked) throw new Error("Firefox did not grant pointer lock after a trusted click");

  await toggleWithShift("island-simulator: run mode enabled (8.0 m/s)");
  await performKeyActions(context, [
    { type: "keyDown", value: "w" },
    { type: "pause", duration: 700 },
    { type: "keyUp", value: "w" },
  ]);
  output = await waitForOutput(context, "island-simulator: first distance-scheduled land footstep played");
  const runEnabled = output.includes("island-simulator: run mode enabled (8.0 m/s)");
  const audioInitialized = output.includes(
    "island-simulator: audio sources use sequential generation and upload with bounded staging memory")
    && output.includes(
      "island-simulator: sparse positional shipwreck creaks share the stronger-wave voice");
  if (!runEnabled) throw new Error("Firefox did not observe the first persistent Shift edge");
  if (!audioInitialized) throw new Error("Firefox did not initialize audio after the trusted click");
  await capture(context, "island-firefox-esr-input.png");

  await toggleWithShift("island-simulator: run mode disabled (4.3 m/s)");
  output = await waitForOutput(context, "island-simulator: run mode disabled (4.3 m/s)");
  await releaseWithEscape();
  const pointerReleased = !await evaluate(
    context, "document.pointerLockElement === document.getElementById('canvas')");
  if (!pointerReleased) throw new Error("Firefox Escape did not release pointer lock");

  await bidiCommand("browsingContext.setViewport", {
    context,
    viewport: { width: 1000, height: 600 },
    devicePixelRatio: 1,
  });
  await delay(300);
  const resizedState = JSON.parse(await evaluate(context, `JSON.stringify((() => {
    const canvas = document.getElementById("canvas");
    return {
      width: canvas.width,
      height: canvas.height,
      clientWidth: canvas.clientWidth,
      clientHeight: canvas.clientHeight
    };
  })())`));
  const backingAspect = resizedState.width / resizedState.height;
  const clientAspect = resizedState.clientWidth / resizedState.clientHeight;
  const responsiveSizing = Math.abs(backingAspect - 16 / 9) < 0.01
    && Math.abs(clientAspect - 16 / 9) < 0.01
    && resizedState.clientWidth <= 1000
    && resizedState.clientHeight <= 600
    && (resizedState.clientWidth === 1000 || resizedState.clientHeight === 600);
  if (!responsiveSizing) {
    throw new Error(`Firefox production canvas lost 16:9 sizing: ${JSON.stringify(resizedState)}`);
  }
  const resizedGraphics = JSON.parse(await evaluate(context, `JSON.stringify((() => {
    const gl = document.getElementById("canvas").getContext("webgl2");
    return { lost: !gl || gl.isContextLost(), error: gl?.getError() ?? -1 };
  })())`));
  if (resizedGraphics.lost || resizedGraphics.error !== 0) {
    throw new Error(`Firefox graphics failed after resize: ${JSON.stringify(resizedGraphics)}`);
  }
  await capture(context, "island-firefox-esr-resized.png");

  // Firefox applies a short pointer-lock cooldown after Escape. A delayed real click must still
  // reacquire control without reloading the production page.
  await delay(700);
  await performPointerClick(context, 500, 281);
  await delay(300);
  const pointerRelocked = await evaluate(
    context, "document.pointerLockElement === document.getElementById('canvas')");
  if (!pointerRelocked) {
    throw new Error("Firefox did not reacquire pointer lock after Escape and a delayed click");
  }
  await releaseWithEscape();
  const pointerReleasedAfterRelock = !await evaluate(
    context, "document.pointerLockElement === document.getElementById('canvas')");
  if (!pointerReleasedAfterRelock) {
    throw new Error("Firefox Escape did not release reacquired pointer lock");
  }

  await evaluate(context, `(() => {
    globalThis.islandTabProbe = { frames: 0, states: [document.visibilityState] };
    document.addEventListener("visibilitychange", () => {
      globalThis.islandTabProbe.states.push(document.visibilityState);
    });
    const countFrame = () => {
      ++globalThis.islandTabProbe.frames;
      requestAnimationFrame(countFrame);
    };
    requestAnimationFrame(countFrame);
    return true;
  })()`);
  await delay(200);
  const tabProbeBefore = JSON.parse(await evaluate(
    context, "JSON.stringify(globalThis.islandTabProbe)"));
  const backgroundTab = await bidiCommand("browsingContext.create", { type: "tab" });
  await bidiCommand("browsingContext.activate", { context: backgroundTab.context });
  await delay(600);
  const tabProbeBackground = JSON.parse(await evaluate(
    context, "JSON.stringify(globalThis.islandTabProbe)"));
  await bidiCommand("browsingContext.activate", { context });
  await delay(350);
  const tabProbeForeground = JSON.parse(await evaluate(
    context, "JSON.stringify(globalThis.islandTabProbe)"));
  await bidiCommand("browsingContext.close", { context: backgroundTab.context });

  const tabVisibilityQualified = tabProbeBackground.states.includes("hidden")
    && tabProbeForeground.states.at(-1) === "visible";
  const animationResumed = tabProbeForeground.frames > tabProbeBefore.frames;
  if (!tabVisibilityQualified || !animationResumed) {
    throw new Error(`Firefox tab visibility/resume failed: ${JSON.stringify({
      tabProbeBefore, tabProbeBackground, tabProbeForeground,
    })}`);
  }

  const outputBeforeResumeInput = await evaluate(
    context, "globalThis.islandDeploymentOutput || ''");
  const enabledCountBefore = countMarker(
    outputBeforeResumeInput, "island-simulator: run mode enabled (8.0 m/s)");
  const disabledCountBefore = countMarker(
    outputBeforeResumeInput, "island-simulator: run mode disabled (4.3 m/s)");
  await toggleWithShift("island-simulator: run mode enabled (8.0 m/s)");
  output = await waitForMarkerCount(
    context,
    "island-simulator: run mode enabled (8.0 m/s)",
    enabledCountBefore + 1);
  await toggleWithShift("island-simulator: run mode disabled (4.3 m/s)");
  output = await waitForMarkerCount(
    context,
    "island-simulator: run mode disabled (4.3 m/s)",
    disabledCountBefore + 1);
  const gameInputResumed = countMarker(
    output, "island-simulator: run mode enabled (8.0 m/s)") > enabledCountBefore
    && countMarker(
      output, "island-simulator: run mode disabled (4.3 m/s)") > disabledCountBefore;
  if (!gameInputResumed) {
    throw new Error("Firefox game input did not resume after tab foregrounding");
  }

  output = await evaluate(context, "globalThis.islandDeploymentOutput || ''");
  const productionOutput = output;
  const fatalDiagnostic = /fatal error|shader.*(?:failed|error)/i.test(productionOutput);
  if (fatalDiagnostic) throw new Error("Firefox emitted a fatal or shader diagnostic");
  const junglePopulationPattern = new RegExp(
    "jungle [1-9][0-9]*/262 canopy \\(near [1-9][0-9]*, far [0-9]+\\), "
      + "[1-9][0-9]*/1768 understory \\(near [1-9][0-9]*, far [0-9]+\\), "
      + "[1-9][0-9]*/588 bushes \\(near [1-9][0-9]*, far [0-9]+\\), "
      + "[1-9][0-9]*/2975 ground plants \\(near [1-9][0-9]*, far [0-9]+\\)");
  const waterStatePattern = new RegExp(
    "water view 0[.][1-9][0-9]*, 1 draw, player ripple 0[.][1-9][0-9]*, "
      + "phase [01](?:[.][0-9]+)?, splash 0[.][1-9][0-9]*, phase 0[.][0-9]+");
  const jungleScene = await qualifyScene(
    context,
    "jungle",
    "island-firefox-esr-jungle.png",
    [
      "island-simulator: renderer WEBGL2",
      "island-simulator: jungle portable wind shader with canopy/understory/bush/ground LOD",
      "island-simulator: jungle per-instance foliage tint reuses existing transforms",
      "island-simulator: jungle fixed-sun two-sided geometric facet lighting",
    ],
    junglePopulationPattern);
  const waterScene = await qualifyScene(
    context,
    "water",
    "island-firefox-esr-water.png",
    [
      "island-simulator: renderer WEBGL2",
      "island-simulator: ocean sunward ripple crests reuse analytic slope field",
      "island-simulator: ocean grazing reflection bands reuse analytic slope field",
      "island-simulator: near-surface swimming view uses generated depth haze, surface scatter, and peripheral veil in one draw",
    ],
    waterStatePattern);
  const shipwreckScene = await qualifyScene(
    context,
    "shipwreck",
    "island-firefox-esr-shipwreck.png",
    [
      "island-simulator: renderer WEBGL2",
      "island-simulator: shipwreck portable aged wood, waterline, and salt-wear material",
      "island-simulator: shipwreck wood loaded Content/structures/shipwreck-weathered-wood.png; 256x256, 9 mips, 349524 RGBA8 bytes; mirrored interior atlas",
      "island-simulator: shipwreck weathered wood follows per-piece board and beam UVs",
      "island-simulator: shipwreck restrained rusted iron straps, rivets, hinges and rudder braces",
      "island-simulator: shipwreck 20m broken hull, 12 debris pieces, one portable culled batch",
      "island-simulator: shipwreck broken hull exposes ribs, partial deck, and snapped mast",
      "island-simulator: shipwreck large-ship silhouette uses deck rails, hull openings, keel, rudder, and a broken foremast",
      "island-simulator: shipwreck QA camera views the 20m wreck from the beach",
    ],
    /shipwreck visible, 12 debris, 1 draw/);
  const crabScene = await qualifyScene(
    context,
    "crabs",
    "island-firefox-esr-crabs.png",
    [
      "island-simulator: renderer WEBGL2",
      "island-simulator: beach crabs 30 agents in deterministic sand-constrained colonies",
      "island-simulator: beach crabs use rounded carapaces, volumetric jointed legs, eye stalks, and one culled batch",
      "island-simulator: crab QA camera views an active beach colony",
    ],
    /crabs [1-9][0-9]*\/30 visible \(near [1-9][0-9]*, far [0-9]+\), 1 draw/);
  const butterflyScene = await qualifyScene(
    context,
    "butterflies",
    "island-firefox-esr-butterflies.png",
    [
      "island-simulator: renderer WEBGL2",
      "island-simulator: clearing butterflies 18 agents in deterministic low-altitude habitats",
      "island-simulator: clearing butterflies use hinged patterned wings in one culled batch",
      "island-simulator: butterfly QA camera views a sunny clearing group",
    ],
    /butterflies [1-9][0-9]*\/18 visible \(near [1-9][0-9]*, far [0-9]+\), 1 draw/);
  const turtleScene = await qualifyScene(
    context,
    "turtles",
    "island-firefox-esr-turtles.png",
    [
      "island-simulator: renderer WEBGL2",
      "island-simulator: sea turtles 5 agents in bounded near-shore cruise, forage, and surface routes",
      "island-simulator: sea turtles use faceted shells and articulated flippers in one culled batch",
      "island-simulator: sea-turtle QA camera views a surfacing route",
    ],
    /turtles [1-9]\/5 visible \(near [1-9], far [0-9]\), 1 draw/);
  const report = {
    schema: 1,
    browser: "Firefox ESR",
    renderer,
    firstFrameQualified: true,
    pointerLocked,
    pointerReleased,
    pointerRelocked,
    pointerReleasedAfterRelock,
    tabLifecycle: {
      hiddenObserved: tabProbeBackground.states.includes("hidden"),
      visibleRestored: tabProbeForeground.states.at(-1) === "visible",
      animationResumed,
      gameInputResumed,
      framesBefore: tabProbeBefore.frames,
      framesWhileBackgrounded: tabProbeBackground.frames,
      framesAfterForeground: tabProbeForeground.frames,
    },
    responsiveSizing,
    resizedState,
    resizedGraphics,
    audioInitialized,
    runEnabled,
    runDisabled: true,
    footstepPlayed: true,
    fatalDiagnostic,
    scenes: {
      jungle: jungleScene,
      water: waterScene,
      shipwreck: shipwreckScene,
      crabs: crabScene,
      butterflies: butterflyScene,
      turtles: turtleScene,
    },
    sessionId: session.sessionId,
    outputTail: productionOutput.split("\n").slice(-28),
  };
  await writeFile(
    path.join(artifactDirectory, "island-firefox-esr-report.json"),
    `${JSON.stringify(report, null, 2)}\n`);
  process.stdout.write(
    `Island Firefox ESR WebGL 2 smoke: PASS `
      + `(report: ${path.join(artifactDirectory, "island-firefox-esr-report.json")})\n`);
  process.stdout.write(`${JSON.stringify(report, null, 2)}\n`);
  // Keep the session alive until final evidence is collected; cleanup closes the browser.
} finally {
  if (collectPageEvidence) {
    try { await writeFile(path.join(artifactDirectory, "firefox-final-page.json"), `${await collectPageEvidence()}\n`); }
    catch (error) { await writeFile(path.join(artifactDirectory, "firefox-page-evidence-error.txt"), `${error}\n`); }
  }
  await writeFile(path.join(artifactDirectory, "firefox-escape-gestures.json"), `${JSON.stringify(escapeGestures, null, 2)}\n`);
  await writeFile(path.join(artifactDirectory, "firefox-shift-gestures.json"), `${JSON.stringify(shiftGestures, null, 2)}\n`);
  browserSocket?.close();
  await stopProcess(firefox);
  await stopProcess(displayServer);
  await stopProcess(server);
  await writeFile(path.join(artifactDirectory, "firefox-stderr.log"), firefoxLog);
  await writeFile(path.join(artifactDirectory, "xvfb-stderr.log"), displayLog);
  await rm(profile, { recursive: true, force: true });
}
