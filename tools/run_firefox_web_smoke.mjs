import { spawn } from "node:child_process";
import { once } from "node:events";
import { mkdtemp, readFile, rm, writeFile } from "node:fs/promises";
import { tmpdir } from "node:os";
import path from "node:path";
import { fileURLToPath } from "node:url";

const toolsDirectory = path.dirname(fileURLToPath(import.meta.url));
const projectRoot = path.resolve(toolsDirectory, "..");
const buildDirectory = path.resolve(process.argv[2] ?? path.join(projectRoot, "build-probe"));
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
]);

const profile = await mkdtemp(path.join(tmpdir(), "island-firefox-smoke-"));
const server = spawn("python3", [
  "-m", "http.server", String(webPort), "--bind", "127.0.0.1",
  "--directory", buildDirectory,
], { stdio: ["ignore", "ignore", "pipe"] });
let serverError = "";
server.stderr.on("data", chunk => { serverError += chunk; });

let firefox;
let firefoxLog = "";
let browserSocket;
let bidiCommand;
const delay = milliseconds => new Promise(resolve => setTimeout(resolve, milliseconds));

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

  firefox = spawn("taskset", [
    "-c", "0-7", firefoxExecutable, "--headless", "--no-remote", "--profile", profile,
    "--remote-debugging-port", String(remotePort), "about:blank",
  ], {
    cwd: profile,
    env: { ...process.env, MOZ_HEADLESS: "1" },
    stdio: ["ignore", "ignore", "pipe"],
  });
  firefox.stderr.on("data", chunk => { firefoxLog += chunk; });

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
    await writeFile(path.join(buildDirectory, filename), Buffer.from(result.data, "base64"));
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
          { type: "pause", duration: 80 },
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

  const session = await bidiCommand("session.new", { capabilities: { alwaysMatch: {} } });
  const created = await bidiCommand("browsingContext.create", { type: "tab" });
  const context = created.context;
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
    "island-simulator: renderer WEBGL2",
    "island-simulator: clearing floors reuse existing noise fields for broad patch breakup",
    "island-simulator: jungle per-instance foliage tint reuses existing transforms",
    "island-simulator: ocean sunward ripple crests reuse analytic slope field",
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
  await capture(context, "island-firefox-esr.png");

  await performPointerClick(context, 640, 360);
  await delay(300);
  const pointerLocked = await evaluate(
    context, "document.pointerLockElement === document.getElementById('canvas')");
  if (!pointerLocked) throw new Error("Firefox did not grant pointer lock after a trusted click");

  await performKeyActions(context, [
    { type: "keyDown", value: "\uE008" },
    { type: "pause", duration: 120 },
    { type: "keyUp", value: "\uE008" },
    { type: "pause", duration: 100 },
    { type: "keyDown", value: "w" },
    { type: "pause", duration: 700 },
    { type: "keyUp", value: "w" },
  ]);
  output = await waitForOutput(context, "island-simulator: first distance-scheduled land footstep played");
  const runEnabled = output.includes("island-simulator: run mode enabled (8.0 m/s)");
  const audioInitialized = output.includes(
    "island-simulator: audio sources use sequential generation and upload with bounded staging memory");
  if (!runEnabled) throw new Error("Firefox did not observe the first persistent Shift edge");
  if (!audioInitialized) throw new Error("Firefox did not initialize audio after the trusted click");
  await capture(context, "island-firefox-esr-input.png");

  await performKeyActions(context, [
    { type: "keyDown", value: "\uE008" },
    { type: "pause", duration: 120 },
    { type: "keyUp", value: "\uE008" },
  ]);
  output = await waitForOutput(context, "island-simulator: run mode disabled (4.3 m/s)");
  await performKeyActions(context, [
    { type: "keyDown", value: "\uE00C" },
    { type: "pause", duration: 80 },
    { type: "keyUp", value: "\uE00C" },
  ]);
  await delay(150);
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
  await performKeyActions(context, [
    { type: "keyDown", value: "\uE00C" },
    { type: "pause", duration: 80 },
    { type: "keyUp", value: "\uE00C" },
  ]);
  await delay(150);
  const pointerReleasedAfterRelock = !await evaluate(
    context, "document.pointerLockElement === document.getElementById('canvas')");
  if (!pointerReleasedAfterRelock) {
    throw new Error("Firefox Escape did not release reacquired pointer lock");
  }

  output = await evaluate(context, "globalThis.islandDeploymentOutput || ''");
  const fatalDiagnostic = /fatal error|shader.*(?:failed|error)/i.test(output);
  if (fatalDiagnostic) throw new Error("Firefox emitted a fatal or shader diagnostic");
  const report = {
    schema: 1,
    browser: "Firefox ESR",
    renderer,
    firstFrameQualified: true,
    pointerLocked,
    pointerReleased,
    pointerRelocked,
    pointerReleasedAfterRelock,
    responsiveSizing,
    resizedState,
    audioInitialized,
    runEnabled,
    runDisabled: true,
    footstepPlayed: true,
    fatalDiagnostic,
    sessionId: session.sessionId,
    outputTail: output.split("\n").slice(-28),
  };
  await writeFile(
    path.join(buildDirectory, "island-firefox-esr-report.json"),
    `${JSON.stringify(report, null, 2)}\n`);
  process.stdout.write(
    `Island Firefox ESR WebGL 2 smoke: PASS `
      + `(report: ${path.join(buildDirectory, "island-firefox-esr-report.json")})\n`);
  process.stdout.write(`${JSON.stringify(report, null, 2)}\n`);
  await bidiCommand("session.end");
} finally {
  browserSocket?.close();
  await stopProcess(firefox);
  await stopProcess(server);
  await rm(profile, { recursive: true, force: true });
}
