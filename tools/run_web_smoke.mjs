import { spawn } from "node:child_process";
import { readFile, writeFile, mkdtemp, rm } from "node:fs/promises";
import path from "node:path";
import { fileURLToPath } from "node:url";
import { createHash } from "node:crypto";
import { parseGrassMotionSamples, grassMotionProgress, grassMotionReleaseTarget, reviewGrassMotion } from "./grass_motion_review.mjs";
import { requireBrowserContainment, recordBrowserProcess } from "./browser_containment.mjs";
import { acknowledgedInputGesture } from "./browser_input_gesture.mjs";
import { parseFlightSamples, reviewFlight } from "./browser_flight_review.mjs";

requireBrowserContainment("chrome");

const toolsDirectory = path.dirname(fileURLToPath(import.meta.url));
const projectRoot = path.resolve(toolsDirectory, "..");
const buildDirectory = path.resolve(process.argv[2] ?? path.join(projectRoot, "build-probe"));
const artifactDirectory = process.env.ISLAND_BROWSER_EVIDENCE;
const port = Number(process.env.ISLAND_WEB_SMOKE_PORT ?? "18764");
const chromeExecutable = process.env.ISLAND_CHROME ?? "google-chrome";
const scriptPath = path.join(buildDirectory, "island-simulator.js");
const wasmPath = path.join(buildDirectory, "island-simulator.wasm");
const dataPath = path.join(buildDirectory, "island-simulator.data");
if (typeof WebSocket === "undefined") {
  throw new Error("This harness requires Node.js with global WebSocket; use EMSDK_NODE (Node 24 in this workspace)");
}
const focusedFoliageCheck = process.env.ISLAND_WEB_SMOKE_SCOPE === "wildlife-foliage";
const focusedCoastCheck = process.env.ISLAND_WEB_SMOKE_SCOPE === "coast";
const focusedShipwreckCheck = process.env.ISLAND_WEB_SMOKE_SCOPE === "shipwreck";
const focusedGroundingCheck = process.env.ISLAND_WEB_SMOKE_SCOPE === "grounding";
const focusedVegetationCheck = process.env.ISLAND_WEB_SMOKE_SCOPE === "vegetation";
const focusedPalmCheck = process.env.ISLAND_WEB_SMOKE_SCOPE === "palms";
const focusedMangroveCheck = process.env.ISLAND_WEB_SMOKE_SCOPE === "mangroves";
const focusedTerrainCheck = process.env.ISLAND_WEB_SMOKE_SCOPE === "terrain";
const focusedCliffCheck = process.env.ISLAND_WEB_SMOKE_SCOPE === "cliffs";
const focusedSkyCheck = process.env.ISLAND_WEB_SMOKE_SCOPE === "sky";
const focusedDistantCheck = process.env.ISLAND_WEB_SMOKE_SCOPE === "distant";
const focusedCanopyLightCheck = process.env.ISLAND_WEB_SMOKE_SCOPE === "canopy-light";
const focusedSnagCheck = process.env.ISLAND_WEB_SMOKE_SCOPE === "snags";
const focusedSaplingCheck = process.env.ISLAND_WEB_SMOKE_SCOPE === "saplings";
const focusedBushCheck = process.env.ISLAND_WEB_SMOKE_SCOPE === "bushes";
const focusedGroundBroadleafCheck = process.env.ISLAND_WEB_SMOKE_SCOPE === "ground-broadleaf";
const focusedSplitGroundCheck = process.env.ISLAND_WEB_SMOKE_SCOPE === "split-ground";
const focusedGrassMotionCheck = process.env.ISLAND_WEB_SMOKE_SCOPE === "grass-motion";
const focusedLitterCheck = process.env.ISLAND_WEB_SMOKE_SCOPE === "litter";
const focusedMaterialsCheck = process.env.ISLAND_WEB_SMOKE_SCOPE === "materials";
const focusedStreamCheck = process.env.ISLAND_WEB_SMOKE_SCOPE === "stream";
const focusedWetlandCheck = process.env.ISLAND_WEB_SMOKE_SCOPE === "wetland";
const focusedBirdCheck = process.env.ISLAND_WEB_SMOKE_SCOPE === "birds";
const focusedBatCheck = process.env.ISLAND_WEB_SMOKE_SCOPE === "bats";
const focusedMonkeyCheck = process.env.ISLAND_WEB_SMOKE_SCOPE === "monkeys";
const focusedBoarCheck = process.env.ISLAND_WEB_SMOKE_SCOPE === "boars";
const focusedGoatCheck = process.env.ISLAND_WEB_SMOKE_SCOPE === "goats";
const focusedDeerCheck = process.env.ISLAND_WEB_SMOKE_SCOPE === "deer";
const focusedSpiderCheck = process.env.ISLAND_WEB_SMOKE_SCOPE === "spiders";
const focusedClimbCheck = process.env.ISLAND_WEB_SMOKE_SCOPE === "monkey-climb";
const focusedFullscreenCheck = process.env.ISLAND_WEB_SMOKE_SCOPE === "fullscreen";
const focusedFlightCheck = process.env.ISLAND_WEB_SMOKE_SCOPE === "flight";
const focusedSnakeCheck = process.env.ISLAND_WEB_SMOKE_SCOPE === "snakes";
const focusedHutCheck = process.env.ISLAND_WEB_SMOKE_SCOPE === "hut";
const focusedTurtleCheck = process.env.ISLAND_WEB_SMOKE_SCOPE === "turtles";
const focusedCrabCheck = process.env.ISLAND_WEB_SMOKE_SCOPE === "crabs";
const focusedHermitCrabCheck = process.env.ISLAND_WEB_SMOKE_SCOPE === "hermit-crabs";
const focusedCoconutCrabCheck = process.env.ISLAND_WEB_SMOKE_SCOPE === "coconut-crabs";
const focusedButterflyCheck = process.env.ISLAND_WEB_SMOKE_SCOPE === "butterflies";
const focusedGeckoCheck = process.env.ISLAND_WEB_SMOKE_SCOPE === "geckos";
const focusedSkinkCheck = process.env.ISLAND_WEB_SMOKE_SCOPE === "skinks";
const focusedDragonflyCheck = process.env.ISLAND_WEB_SMOKE_SCOPE === "dragonflies";
const focusedFrogCheck = process.env.ISLAND_WEB_SMOKE_SCOPE === "frogs";
const focusedRodentCheck = process.env.ISLAND_WEB_SMOKE_SCOPE === "rodents";
const focusedFlyCheck = process.env.ISLAND_WEB_SMOKE_SCOPE === "flies";
const focusedAntCheck = process.env.ISLAND_WEB_SMOKE_SCOPE === "ants";
const focusedCicadaCheck = process.env.ISLAND_WEB_SMOKE_SCOPE === "cicadas";
const focusedBeetleCheck = process.env.ISLAND_WEB_SMOKE_SCOPE === "beetles";
const focusedCrocodileCheck = process.env.ISLAND_WEB_SMOKE_SCOPE === "crocodiles";
const focusedFishCheck = process.env.ISLAND_WEB_SMOKE_SCOPE === "fish";
const focusedStingrayCheck = process.env.ISLAND_WEB_SMOKE_SCOPE === "stingrays";
const focusedOctopusCheck = process.env.ISLAND_WEB_SMOKE_SCOPE === "octopuses";
const focusedSharkCheck = process.env.ISLAND_WEB_SMOKE_SCOPE === "shark";
const focusedDolphinCheck = process.env.ISLAND_WEB_SMOKE_SCOPE === "dolphins";
const focusedBearCheck = process.env.ISLAND_WEB_SMOKE_SCOPE === "bear";
const focusedResidentCheck = process.env.ISLAND_WEB_SMOKE_SCOPE === "resident";
const focusedHelicopterCheck = process.env.ISLAND_WEB_SMOKE_SCOPE === "helicopter";
const focusedCampfireCheck = process.env.ISLAND_WEB_SMOKE_SCOPE === "campfire";
const focusedCaveCheck = process.env.ISLAND_WEB_SMOKE_SCOPE === "cave";
const focusedBunkerCheck = process.env.ISLAND_WEB_SMOKE_SCOPE === "bunker";
const focusedCheck = focusedSpiderCheck || focusedGoatCheck || focusedDeerCheck || focusedHutCheck || focusedHelicopterCheck || focusedCampfireCheck || focusedCaveCheck || focusedBunkerCheck || focusedTurtleCheck || focusedCrabCheck || focusedHermitCrabCheck || focusedCoconutCrabCheck || focusedButterflyCheck || focusedGeckoCheck || focusedSkinkCheck || focusedDragonflyCheck || focusedFrogCheck || focusedRodentCheck || focusedFlyCheck || focusedAntCheck || focusedCicadaCheck || focusedBeetleCheck || focusedCrocodileCheck || focusedFishCheck || focusedStingrayCheck || focusedOctopusCheck || focusedSharkCheck || focusedDolphinCheck || focusedBearCheck || focusedResidentCheck || focusedBoarCheck || focusedSnakeCheck || focusedFlightCheck || focusedFullscreenCheck || focusedClimbCheck || focusedFoliageCheck || focusedCoastCheck || focusedShipwreckCheck || focusedGroundingCheck || focusedVegetationCheck || focusedPalmCheck || focusedMangroveCheck || focusedBushCheck || focusedGroundBroadleafCheck || focusedSplitGroundCheck || focusedGrassMotionCheck || focusedTerrainCheck || focusedCliffCheck || focusedSkyCheck || focusedDistantCheck || focusedCanopyLightCheck || focusedSnagCheck || focusedSaplingCheck || focusedLitterCheck || focusedMaterialsCheck || focusedStreamCheck || focusedWetlandCheck || focusedBirdCheck || focusedBatCheck || focusedMonkeyCheck;
const focusedArtifactPrefix = focusedSpiderCheck ? "spiders" : focusedGoatCheck ? "goats" : focusedDeerCheck ? "deer" : focusedHutCheck ? "hut" : focusedHelicopterCheck ? "helicopter" : focusedCampfireCheck ? "campfire" : focusedCaveCheck ? "cave" : focusedBunkerCheck ? "bunker" : focusedTurtleCheck ? "turtles" : focusedCrabCheck ? "crabs" : focusedHermitCrabCheck ? "hermit-crabs" : focusedCoconutCrabCheck ? "coconut-crabs" : focusedButterflyCheck ? "butterflies" : focusedGeckoCheck ? "geckos" : focusedSkinkCheck ? "skinks" : focusedDragonflyCheck ? "dragonflies" : focusedFrogCheck ? "frogs" : focusedRodentCheck ? "rodents" : focusedFlyCheck ? "flies" : focusedAntCheck ? "ants" : focusedCicadaCheck ? "cicadas" : focusedBeetleCheck ? "beetles" : focusedCrocodileCheck ? "crocodiles" : focusedFishCheck ? "fish" : focusedStingrayCheck ? "stingrays" : focusedOctopusCheck ? "octopuses" : focusedSharkCheck ? "shark" : focusedDolphinCheck ? "dolphins" : focusedBearCheck ? "bear" : focusedResidentCheck ? "resident" : focusedBoarCheck ? "boars" : focusedSnakeCheck ? "snakes" : focusedFlightCheck ? "flight" : focusedFullscreenCheck ? "fullscreen" : focusedClimbCheck ? "monkey-climb" : focusedBirdCheck ? "birds" : focusedBatCheck ? "bats" : focusedMonkeyCheck ? "monkeys" : focusedWetlandCheck ? "wetland" : focusedStreamCheck ? "stream" : focusedMaterialsCheck ? "materials" : focusedLitterCheck ? "litter" : focusedSaplingCheck ? "saplings" : focusedSnagCheck ? "snags" : focusedCanopyLightCheck ? "canopy-light" : focusedDistantCheck ? "distant" : focusedSkyCheck ? "sky" : focusedCliffCheck ? "cliffs" : focusedTerrainCheck ? "terrain" : focusedGrassMotionCheck ? "grass-motion" : focusedSplitGroundCheck ? "split-ground" : focusedGroundBroadleafCheck ? "ground-broadleaf" : focusedBushCheck ? "bushes" : focusedMangroveCheck ? "mangroves" : focusedPalmCheck ? "palms" : focusedVegetationCheck ? "vegetation"
  : focusedShipwreckCheck ? "shipwreck" : focusedCoastCheck ? "coast" : focusedGroundingCheck ? "grounding" : "foliage";
const focusedViews = focusedSpiderCheck ? ["spiders", "spider-web"] : focusedGoatCheck ? ["goats"] : focusedDeerCheck ? ["deer"] : focusedHutCheck ? ["hut", "hut-material"] : focusedHelicopterCheck ? ["helicopter", "helicopter-inside"] : focusedCampfireCheck ? ["campfire"] : focusedCaveCheck ? ["cave", "cave-inside"] : focusedBunkerCheck ? ["bunker", "bunker-corridor", "bunker-computers", "bunker-reactor", "bunker-service", "bunker-end"] : focusedTurtleCheck ? ["turtles"] : focusedCrabCheck ? ["crabs"] : focusedHermitCrabCheck ? ["hermit-crabs"] : focusedCoconutCrabCheck ? ["coconut-crabs"] : focusedButterflyCheck ? ["butterflies"] : focusedGeckoCheck ? ["geckos"] : focusedSkinkCheck ? ["skinks"] : focusedDragonflyCheck ? ["dragonflies"] : focusedFrogCheck ? ["frogs"] : focusedRodentCheck ? ["mice", "rats"] : focusedFlyCheck ? ["flies"] : focusedAntCheck ? ["ants"] : focusedCicadaCheck ? ["cicadas"] : focusedBeetleCheck ? ["beetles"] : focusedCrocodileCheck ? ["crocodiles"] : focusedFishCheck ? ["fish"] : focusedStingrayCheck ? ["stingrays"] : focusedOctopusCheck ? ["octopuses"] : focusedSharkCheck ? ["shark"] : focusedDolphinCheck ? ["dolphins"] : focusedBearCheck ? ["bear"] : focusedResidentCheck ? ["resident"] : focusedBoarCheck ? ["boars"] : focusedSnakeCheck ? ["snakes", "snake-head", "tree-snakes"] : focusedFlightCheck ? ["flight"] : focusedFullscreenCheck ? ["overview"] : focusedClimbCheck ? ["monkey-climb"] : focusedBirdCheck ? ["birds", "ground-doves"] : focusedBatCheck ? ["bats", "bat-roost"] : focusedMonkeyCheck ? ["monkeys"] : focusedWetlandCheck ? ["wetland"] : focusedStreamCheck ? ["stream"] : focusedSaplingCheck ? ["sapling"] : focusedSnagCheck ? ["snags"] : focusedMaterialsCheck ? ["litter", "grass", "bay-ground", "wet-sand"]
  : focusedLitterCheck ? ["litter", "grass"] : focusedCanopyLightCheck ? ["jungle"] : focusedDistantCheck ? ["distant"] : focusedSkyCheck ? ["overview"] : focusedCliffCheck ? ["east", "headland"] : focusedTerrainCheck ? ["overview", "bay", "bay-ground", "headland", "beach", "east", "jungle"]
  : focusedGrassMotionCheck ? ["grass-motion"] : focusedSplitGroundCheck ? ["ground"] : focusedGroundBroadleafCheck ? ["ground-broadleaf"] : focusedBushCheck ? ["bush"] : focusedMangroveCheck ? ["mangroves"] : focusedPalmCheck ? ["palms", "fallen-palm", "broken-palm"] : focusedVegetationCheck ? ["grass", "jungle", "fronds", "east", "clearing-edge", "ground-broadleaf", "ground", "bush"]
  : focusedGroundingCheck ? ["props", "driftwood", "contact", "reef"]
  : focusedShipwreckCheck ? ["shipwreck"]
  : focusedCoastCheck ? ["shipwreck", "water", "west", "reef"]
  : ["jungle", "boars", "monkeys", "east"];
// The bunker scope owns six static review cameras. Twelve post-capture intervals are
// sufficient to catch shader/context failures at every camera without making the
// software-only SwiftShader gate spend several minutes in each enclosed room.
const focusedFrameCount = focusedBunkerCheck ? 12 : focusedGrassMotionCheck ? 30 : focusedCoastCheck ? 120 : 60;
if (process.env.ISLAND_WEB_SMOKE_SCOPE && !focusedCheck) {
  throw new Error("Unknown ISLAND_WEB_SMOKE_SCOPE; omit for the full suite or use wildlife-foliage/coast/shipwreck/grounding/vegetation/palms/mangroves/bushes/ground-broadleaf/split-ground/terrain/cliffs/sky/distant/canopy-light/snags/saplings/grass-motion/litter/materials/stream/wetland/birds/bats/monkeys/boars/turtles/crabs/hermit-crabs/coconut-crabs/butterflies/geckos/skinks/dragonflies/frogs/crocodiles/fish/stingrays/octopuses/shark/dolphins/bear/resident/helicopter/campfire/cave/bunker");
}
// The outer Python supervisor monitors the host kernel continuously. A user/PID namespace
// must not depend on retaining the host's supplementary journal-access group.
const kernelChecks = [];
const checkKernel = label => {
  kernelChecks.push({ label, utc: new Date().toISOString() });
};

await Promise.all([readFile(scriptPath), readFile(wasmPath), readFile(dataPath)]);
const template = await readFile(path.join(toolsDirectory, "web_smoke.html.in"), "utf8");
await writeFile(path.join(buildDirectory, "island-smoke.html"), template);

const profile = await mkdtemp(path.join(artifactDirectory, "chrome-profile-"));
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
    "--use-gl=angle",
    "--use-angle=swiftshader",
    "--enable-unsafe-swiftshader",
    `--user-data-dir=${profile}`,
    "--remote-debugging-port=0",
    "about:blank",
  ], { stdio: ["ignore", "ignore", "pipe"] });
  let chromeLog = "";
  chrome.stderr.on("data", chunk => { chromeLog += chunk; });
  const browserIdentity = recordBrowserProcess(chrome, "chrome");

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
  let screencastConsumer = null, screencastError = "";
  const screencastJobs = new Set();
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
    } else if (message.method === "Page.screencastFrame") {
      const work = (async () => {
        try { if (screencastConsumer) await screencastConsumer(message.params, message.sessionId); }
        finally {
          await call("Page.screencastFrameAck", { sessionId: message.params.sessionId }, message.sessionId);
        }
      })();
      screencastJobs.add(work);
      work.then(() => screencastJobs.delete(work), error => {
        screencastError = String(error);
        screencastJobs.delete(work);
      });
    }
  };
  const call = (method, params = {}, sessionId) => new Promise((resolve, reject) => {
    const id = ++nextId;
    pending.set(id, { resolve, reject });
    browserSocket.send(JSON.stringify({ id, method, params, ...(sessionId ? { sessionId } : {}) }));
  });
  let targetCloseTimeouts = 0;
  const closeTargetBounded = async targetId => {
    const result = await Promise.race([
      call("Target.closeTarget", { targetId })
        .then(() => ({ closed: true }))
        .catch(error => ({ error })),
      delay(3000).then(() => ({ timedOut: true })),
    ]);
    if (result.error) throw result.error;
    if (result.timedOut) {
      ++targetCloseTimeouts;
      console.warn(`Target.closeTarget acknowledgement timed out for ${targetId}; Browser.close remains authoritative`);
    }
  };
  const closeBrowserBounded = async () => {
    const result = await Promise.race([
      call("Browser.close").then(() => ({ closed: true })).catch(error => ({ error })),
      delay(3000).then(() => ({ timedOut: true })),
    ]);
    if (result.error) throw result.error;
    if (result.timedOut)
      console.warn("Browser.close acknowledgement timed out; owned-process cleanup remains authoritative");
  };
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
  const countOutput = (output, pattern) => typeof pattern === "string"
    ? output.split(pattern).length - 1
    : (output.match(new RegExp(pattern.source, `${pattern.flags.replace("g", "")}g`)) ?? []).length;
  // Hold one actual key-down until the game logs a NEW occurrence of the pattern, then release.
  // Fixed 650 ms pulses fell between frame-polled updates once SwiftShader frames exceeded that
  // (two retained 2026-10-10 grass-motion failures and a full-suite input failure).
  const holdKeyUntilNewOutput = async (sessionId, key, code, virtualKeyCode, pattern,
    attempts = 60, settleMilliseconds = 350) => {
    const before = countOutput(await readSmokeOutput(sessionId), pattern);
    let acknowledged = false;
    await call("Input.dispatchKeyEvent", {
      type: "keyDown", key, code, windowsVirtualKeyCode: virtualKeyCode,
      nativeVirtualKeyCode: virtualKeyCode,
    }, sessionId);
    try {
      for (let attempt = 0; attempt < attempts && !acknowledged; ++attempt) {
        await delay(250);
        acknowledged = countOutput(await readSmokeOutput(sessionId), pattern) > before;
      }
    } finally {
      await call("Input.dispatchKeyEvent", {
        type: "keyUp", key, code, windowsVirtualKeyCode: virtualKeyCode,
        nativeVirtualKeyCode: virtualKeyCode,
      }, sessionId);
      // The game polls keys per frame: a second edge sent before it observed this release is
      // not a press. Wait for two more game output lines (two frames) before returning.
      const releasedAt = countOutput(await readSmokeOutput(sessionId), "\n");
      for (let frame = 0; frame < 24; ++frame) {
        await delay(250);
        if (countOutput(await readSmokeOutput(sessionId), "\n") >= releasedAt + 2) break;
      }
      await delay(settleMilliseconds);
    }
    return acknowledged;
  };
  const dispatchKeyUntilOutput = async (sessionId, key, code, virtualKeyCode, pattern) => {
    if (await waitForOutput(sessionId, pattern, 1)) return true;
    for (let attempt = 0; attempt < 2; ++attempt) {
      if (await holdKeyUntilNewOutput(sessionId, key, code, virtualKeyCode, pattern)) return true;
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
      const evidence = {};
      const inspect = async () => {
        const snapshot = await call("Runtime.evaluate", {
          expression: `({ focused: document.hasFocus(), visibility: document.visibilityState,
            canvasActive: document.activeElement === Module.canvas,
            events: globalThis.islandSmokeResult?.inputEvents ?? [],
            error: globalThis.islandSmokeResult?.error,
            outputTail: (globalThis.islandSmokeResult?.output ?? '').slice(-4000) })`,
          returnByValue: true,
        }, sessionId);
        return snapshot.result.value;
      };
      let overlayHidden;
      try {
        overlayHidden = await acknowledgedInputGesture({ evidence, inspect,
          press: () => call("Input.dispatchKeyEvent", { type: "keyDown", key: "F1", code: "F1",
            windowsVirtualKeyCode: 112, nativeVirtualKeyCode: 112 }, sessionId),
          // A bunker room can make one SwiftShader frame exceed ten seconds on a
          // CPU-only runner. Keep the real F1 gesture held until one such frame has
          // polled it; other scopes retain the established shorter bound.
          acknowledge: () => waitForOutput(sessionId, "island-simulator: debug overlay disabled",
            focusedBunkerCheck ? 120 : 40),
          release: () => call("Input.dispatchKeyEvent", { type: "keyUp", key: "F1", code: "F1",
            windowsVirtualKeyCode: 112, nativeVirtualKeyCode: 112 }, sessionId),
        });
      } finally {
        await writeFile(path.join(artifactDirectory, fileName.replace(/\.png$/, "-overlay-gesture.json")),
          `${JSON.stringify({ ...evidence, acknowledged: overlayHidden === true }, null, 2)}\n`);
      }
      if (!overlayHidden) {
        throw new Error(`Visual capture ${fileName} could not hide the debug overlay`);
      }
      // Let at least one heavily loaded SwiftShader frame present without the overlay.
      await delay(300);
    }
    const screenshot = await call("Page.captureScreenshot", { format: "png" }, sessionId);
    const screenshotPath = path.join(artifactDirectory, fileName);
    await writeFile(screenshotPath, Buffer.from(screenshot.data, "base64"));
    return screenshotPath;
  };

  // Verify the browser's implementation before loading the game, not merely its launch flags.
  const systemInfo = await call("SystemInfo.getInfo");
  const browserRenderer = systemInfo.gpu?.auxAttributes?.glRenderer ?? "";
  if (!/SwiftShader/i.test(browserRenderer)) {
    throw new Error(`Software-only WebGL check refused renderer: ${browserRenderer}`);
  }
  console.log(`Island browser host PID ${browserIdentity.outerPid}, namespace PID ${chrome.pid}: ${browserRenderer}`);

  const runGrassMotionReview = async sessionId => {
    const stages = [];
    const controlAttempts = [];
    let samples = [], startCount = 0, failure = "", result = { passed: false };
    const activation = {};
    const recording = { scope: "bounded compositor samples, not every game frame; telemetry association is approximate",
      maximumFrames: 240, maximumBytes: 16 * 1024 * 1024, bytes: 0, truncated: false, frames: [] };
    let recordingStarted = false;
    const stopRecording = async () => {
      if (!recordingStarted) return;
      recordingStarted = false;
      await call("Page.stopScreencast", {}, sessionId);
      await Promise.allSettled([...screencastJobs]);
      screencastConsumer = null;
      recording.error = screencastError;
      recording.distinctFrames = new Set(recording.frames.map(frame => frame.sha256)).size;
    };
    const read = async () => {
      const all = parseGrassMotionSamples(await readSmokeOutput(sessionId));
      samples = all.slice(Math.max(0, startCount - 1));
      return { all, latest: all.at(-1) };
    };
    const until = async (predicate, label) => {
      // SwiftShader frames can exceed a second while PlayerMotion clamps dt. Pace this
      // by game samples: the former 15 s wall limit expired at 5.99 m of a 6 m walk.
      // Both budgets remain finite so a stalled game or blocked player still fails.
      const initialCount = (await read()).all.length;
      const deadline = Date.now() + 90000;
      while (Date.now() < deadline) {
        const state = await read();
        if (predicate(state.latest, state.all.length)) return state;
        if (state.all.length - initialCount >= 120) break;
        await delay(200);
      }
      throw new Error(`Grass motion timeout: ${label}`);
    };
    const key = (down, forward) => call("Input.dispatchKeyEvent", {
      type: down ? "keyDown" : "keyUp", key: forward ? "w" : "s", code: forward ? "KeyW" : "KeyS",
      windowsVirtualKeyCode: forward ? 87 : 83, nativeVirtualKeyCode: forward ? 87 : 83,
    }, sessionId);
    const snapshot = async (target, sample) => {
      const evaluation = await call("Runtime.evaluate", {
        expression: `(() => { const gl=Module.canvas.getContext('webgl2'); return {
          lost: !gl || gl.isContextLost(), error: gl?.getError(),
          gameError: globalThis.islandSmokeResult.error, exited: globalThis.islandSmokeResult.exited }; })()`,
        returnByValue: true,
      }, sessionId);
      const gl = evaluation.result.value;
      if (!gl || gl.lost || gl.error !== 0 || gl.gameError || gl.exited) {
        throw new Error(`Grass motion graphics failure: ${JSON.stringify(gl)}`);
      }
      const file = `island-browser-grass-motion-step-${stages.length}.png`;
      await captureVisualFrame(sessionId, file, false);
      stages.push({ target, sample, screenshot: file, gl });
      checkKernel(`grass-motion-step-${stages.length - 1}`);
      console.log(`Island grass motion: capture ${stages.length - 1}, player ${sample.x}, ${sample.z}`);
    };
    try {
      await call("Page.bringToFront", {}, sessionId);
      const browserInputState = async () => {
        const state = await call("Runtime.evaluate", {
          expression: `(() => { const c=Module.canvas, r=c.getBoundingClientRect(); return {
            focused: document.hasFocus(), canvasActive: document.activeElement === c,
            centreHitsCanvas: document.elementFromPoint(640,360) === c,
            pointerLocked: document.pointerLockElement === c, visibility: document.visibilityState,
            rectangle: {x:r.x,y:r.y,width:r.width,height:r.height},
            error: globalThis.islandSmokeResult?.error,
            events: globalThis.islandSmokeResult?.inputEvents,
            droppedEvents: globalThis.islandSmokeResult?.droppedInputEvents }; })()`, returnByValue: true,
        }, sessionId);
        return state.result.value;
      };
      const initialCount = (await read()).all.length;
      // A fixed-duration click is not an acknowledgement. This does not establish the cause
      // of the earlier flake: keep browser event history and require fresh game telemetry.
      const initial = await acknowledgedInputGesture({ evidence: activation, inspect: browserInputState,
        press: () => call("Input.dispatchMouseEvent", {
          type: "mousePressed", x: 640, y: 360, button: "left", buttons: 1, clickCount: 1,
        }, sessionId),
        acknowledge: () => until((s,count) => count > initialCount && s && s.captured && s.relative && !s.free
          && Math.hypot(s.velocityX,s.velocityZ) < 0.15,"focused stationary start"),
        release: () => call("Input.dispatchMouseEvent", {
          type: "mouseReleased", x: 640, y: 360, button: "left", buttons: 0, clickCount: 1,
        }, sessionId),
      });
      startCount = initial.all.length;
      const start = initial.latest;
      if (Math.hypot(start.x - 43, start.z + 18) > 0.1) {
        throw new Error("Grass motion did not start at the east clearing");
      }
      await snapshot(0, start);
      screencastError = "";
      screencastConsumer = async (frame, frameSession) => {
        if (frameSession !== sessionId) return;
        const bytes = Buffer.from(frame.data, "base64");
        if (recording.frames.length >= recording.maximumFrames
          || recording.bytes + bytes.length > recording.maximumBytes) {
          recording.truncated = true;
          return;
        }
        const file = `island-browser-grass-motion-${chrome.pid}-frame-${String(recording.frames.length).padStart(3,"0")}.jpg`;
        recording.bytes += bytes.length;
        recording.frames.push({ file, timestamp: frame.metadata?.timestamp ?? null,
          observedSample: samples.length - 1, observedX: samples.at(-1)?.x, observedZ: samples.at(-1)?.z,
          sha256: createHash("sha256").update(bytes).digest("hex") });
        await writeFile(path.join(artifactDirectory,file),bytes);
      };
      recordingStarted = true;
      await call("Page.startScreencast", {
        format: "jpeg", quality: 65, maxWidth: 640, maxHeight: 360, everyNthFrame: 1,
      }, sessionId);
      let stopLead = 0, stopped = initial;
      for (const target of [6, 12, 18, 12, 6, 0]) {
        for (let attempt = 0; attempt < 3; ++attempt) {
          const control = grassMotionReleaseTarget(grassMotionProgress(start,stopped.latest),target,stopLead);
          let released;
          await key(true, control.forward);
          try {
            released = await until(s => s && (control.forward ? grassMotionProgress(start,s) >= control.release
              : grassMotionProgress(start,s) <= control.release), `approach ${target} metres`);
          } finally {
            await key(false, control.forward);
          }
          const releasedCount = (await read()).all.length;
          stopped = await until((s,count) => count > releasedCount
            && Math.hypot(s.velocityX,s.velocityZ) < 0.15, `stop at ${target} metres`);
          const releaseProgress = grassMotionProgress(start,released.latest);
          const stoppedProgress = grassMotionProgress(start,stopped.latest);
          stopLead = Math.max(0,(stoppedProgress-releaseProgress)*(control.forward ? 1 : -1));
          controlAttempts.push({ target,attempt,...control,releaseProgress,stoppedProgress,observedStopLead:stopLead });
          if (Math.abs(stoppedProgress-target) <= 2.5) break;
        }
        await snapshot(target, stopped.latest);
      }
      await read();
      await stopRecording();
      result = reviewGrassMotion(stages,samples);
      if (!result.passed) throw new Error(`Grass motion review failed: ${result.errors.join("; ")}`);
      if (recording.error || recording.frames.length < 12 || recording.distinctFrames < 8) {
        throw new Error(`Moving-frame recording incomplete: ${recording.error || recording.frames.length + " frames"}`);
      }
      return { ...result, stages, samples, recording, controlAttempts };
    } catch (error) {
      failure = String(error);
      throw error;
    } finally {
      try { await stopRecording(); }
      catch (error) { failure ||= `Stop recording failed: ${error}`; }
      screencastConsumer = null;
      await writeFile(path.join(artifactDirectory,"island-browser-grass-motion-traversal.json"),
        `${JSON.stringify({ ...result, passed: result.passed && !failure, failure, browserPid: chrome.pid,
          scope: "software walking review; prior-frame grass counts and submitted canopy/shrub identities",
          stages, samples, recording, controlAttempts, activation },null,2)}\n`);
      await writeFile(path.join(artifactDirectory,"island-browser-grass-motion-output.log"),
        await readSmokeOutput(sessionId));
    }
  };

  const runFlightReview = async sessionId => {
    const stages = [], held = new Map();
    let result = {passed:false}, failure = "";
    const keys = {Tab:["Tab",9], Space:[" ",32], ShiftLeft:["Shift",16]};
    const key = async (code,down) => {
      const [name,value]=keys[code];
      await call("Input.dispatchKeyEvent", {type:down ? "keyDown" : "keyUp",key:name,code,
        windowsVirtualKeyCode:value,nativeVirtualKeyCode:value},sessionId);
      if(down) held.set(code,true); else held.delete(code);
    };
    const read = async () => parseFlightSamples(await readSmokeOutput(sessionId));
    const until = async (predicate,description) => {
      for(let attempt=0;attempt<240;++attempt) {
        const samples=await read();
        if(predicate(samples)) return samples;
        await delay(250);
      }
      throw new Error(`Flight timed out: ${description}`);
    };
    try {
      await call("Page.bringToFront",{},sessionId);
      await call("Runtime.evaluate",{expression:"Module.canvas.focus()"},sessionId);
      await key("Tab",true);
      let samples=await until(s=>s.length>=4,"held Tab activation");
      stages.push({label:"activation",sample:samples[0]});
      if(countOutput(await readSmokeOutput(sessionId),"flight on;")!==1)
        throw new Error("Held Tab repeated flight activation");
      await key("Tab",false);
      const releasedCount=samples.length;
      await until(s=>s.length>=releasedCount+2,"released Tab frames");
      await key("Space",true);
      samples=await until(s=>s.at(-1)?.height>99.9,"terrain-relative ceiling");
      const ceilingCount=samples.length;
      samples=await until(s=>s.length>=ceilingCount+4,"held ascent at ceiling");
      stages.push({label:"ceiling",sample:samples.at(-1),heldSamples:samples.length-ceilingCount});
      await key("Space",false);
      await dispatchMouseClick(sessionId,640,360);
      const locked=await call("Runtime.evaluate", {
        expression:"document.pointerLockElement===Module.canvas",returnByValue:true},sessionId);
      if(!locked.result.value) throw new Error("Flight view could not acquire pointer lock");
      await call("Input.dispatchMouseEvent",{type:"mouseMoved",x:640,y:570,button:"none",buttons:0},sessionId);
      samples=await read();
      const lookCount=samples.length;
      await until(s=>s.length>=lookCount+3,"aerial look presentation");
      await captureVisualFrame(sessionId,"island-browser-flight-ceiling.png");
      await key("ShiftLeft",true);
      samples=await until(s=>s.at(-1)?.height<60,"descent");
      stages.push({label:"descent",sample:samples.at(-1)});
      await key("ShiftLeft",false);
      await key("Tab",true);
      if(!await waitForOutput(sessionId,"flight off; safe dry-land return",240))
        throw new Error("Flight did not return safely to land");
      await key("Tab",false);
      const prior=countOutput(await readSmokeOutput(sessionId),"input sample captured");
      for(let attempt=0;attempt<240;++attempt) {
        if(countOutput(await readSmokeOutput(sessionId),"input sample captured")>=prior+3) break;
        if(attempt===239) throw new Error("No landed input frames");
        await delay(250);
      }
      const output=await readSmokeOutput(sessionId);
      const positions=[...output.split("flight off; safe dry-land return").at(-1).matchAll(
        /camera ([-+0-9.eE]+) ([-+0-9.eE]+) ([-+0-9.eE]+) free/g)];
      stages.push({label:"landed",safeReturn:true,cameraY:Number(positions.at(-1)?.[2])});
      await captureVisualFrame(sessionId,"island-browser-flight-landed.png");
      result=reviewFlight(output,stages);
      if(!result.passed) throw new Error(result.errors.join("; "));
      return result;
    } catch(error) {
      failure=String(error); throw error;
    } finally {
      for(const code of [...held.keys()]) await key(code,false);
      const output=await readSmokeOutput(sessionId);
      await writeFile(path.join(artifactDirectory,"island-browser-flight-input.json"),
        JSON.stringify({...result,failure,stages,output},null,2)+"\n");
    }
  };

  if (focusedCheck) {
    const views = [];
    const reportPath = path.join(artifactDirectory,
      focusedFoliageCheck ? "island-browser-wildlife-foliage.json" : `island-browser-${focusedArtifactPrefix}.json`);
    try {
      for (const mode of focusedViews) {
        checkKernel(`before-${mode}`);
        // One live game at a time; closing the page also releases its WebGL context.
        const { targetId } = await call("Target.createTarget", { url: "about:blank" });
        let sessionId;
        try {
          ({ sessionId } = await call("Target.attachToTarget", { targetId, flatten: true }));
          await call("Runtime.enable", {}, sessionId);
          await call("Page.enable", {}, sessionId);
          await call("Emulation.setDeviceMetricsOverride", {
            width: 1280, height: 720, deviceScaleFactor: 1, mobile: false,
          }, sessionId);
          await call("Page.navigate", {
            url: `http://127.0.0.1:${port}/island-smoke.html?mode=${mode}`,
          }, sessionId);
          if (!await waitForOutput(sessionId, "island-simulator: visible terrain cells", 160)) {
            throw new Error(`${mode} did not reach a rendered scene: ${await readSmokeOutput(sessionId)}`);
          }
          const startupLog = await readSmokeOutput(sessionId);
          const expectedWorkload = focusedGrassMotionCheck || focusedFlightCheck ? 'workload mode="input-diagnostics" viewpoint="interactive"'
            : `workload mode="interactive" viewpoint="${mode}"`;
          if (!startupLog.includes(expectedWorkload)) {
            throw new Error(`${mode} launched the wrong workload: ${startupLog}`);
          }
          const flightReview = focusedFlightCheck ? await runFlightReview(sessionId) : null;
          if (focusedFullscreenCheck) {
            await call("Page.bringToFront", {}, sessionId);
            await call("Runtime.evaluate", {expression:"Module.canvas.focus()"}, sessionId);
            const fullscreenState = async () => (await call("Runtime.evaluate", {
              expression: "({active:!!document.fullscreenElement,width:innerWidth,height:innerHeight})",
              returnByValue: true,
            }, sessionId)).result.value;
            const key = async (type, repeat = false) => call("Input.dispatchKeyEvent", {
              type, key:"F11", code:"F11", windowsVirtualKeyCode:122,
              nativeVirtualKeyCode:122, autoRepeat:repeat,
            }, sessionId);
            const requireState = async (active, occurrence) => {
              for (let attempt=0; attempt<40; ++attempt) {
                const state = await fullscreenState();
                // fullscreenElement can change before the asynchronous fullscreenchange
                // notification is delivered; require both, not a race with the log reader.
                const confirmed = countOutput(await readSmokeOutput(sessionId),
                  `island-simulator: fullscreen ${active ? "on" : "off"}`);
                if (state.active === active && confirmed === occurrence) return state;
                await delay(100);
              }
              throw new Error(`F11 did not set DOM fullscreen to ${active}`);
            };
            const stages = [];
            for (let cycle=0; cycle<2; ++cycle) {
              await key("keyDown");
              stages.push(await requireState(true, cycle+1));
              await key("keyDown", true); await delay(400);
              if (!(await fullscreenState()).active) throw new Error("Held F11 toggled fullscreen twice");
              if (cycle===0) await captureVisualFrame(sessionId, "island-browser-fullscreen-active.png");
              await key("keyUp");
              await key("keyDown"); stages.push(await requireState(false, cycle+1));
              await key("keyUp");
            }
            const log = await readSmokeOutput(sessionId);
            if (countOutput(log,"fullscreen on")!==2 || countOutput(log,"fullscreen off")!==2
                || log.includes("fullscreen failed:") || log.includes("fullscreen unavailable:"))
              throw new Error("Fullscreen diagnostics do not match actual transitions");
            await writeFile(path.join(artifactDirectory,"island-browser-fullscreen-transitions.json"),
              JSON.stringify({passed:true,stages,heldRepeatSuppressed:true},null,2)+"\n");
          }
          if (focusedClimbCheck && !await waitForOutput(sessionId,
              "monkey climb held summit presented after eight cycle stages", 320)) {
            throw new Error("monkey-climb did not finish its rendered cycle before summit capture");
          }
          if (focusedGrassMotionCheck) {
            // Set up once through the existing diagnostic teleport. Fixed --viewpoint cameras
            // deliberately reject input; all measured travel after this setup uses the player.
            if (!await waitForOutput(sessionId, "island-simulator: debug overlay active", 80)) {
              throw new Error("Grass motion setup requires the diagnostic overlay");
            }
            await call("Page.bringToFront", {}, sessionId);
            const focused = await call("Runtime.evaluate", {
              expression: `(() => { Module.canvas.focus(); return document.activeElement === Module.canvas; })()`,
              returnByValue: true,
            }, sessionId);
            if (!focused.result.value) throw new Error("Grass motion setup could not focus the canvas");
            // Fixed 650 ms F7 pulses fell between frame-polled updates on heavily loaded
            // SwiftShader frames (two retained 2026-10-10 failures with only a focus event).
            // Hold one actual key-down until the game acknowledges the teleport, as F1 does.
            const teleportPattern = "island-simulator: debug teleport east clearing x 43 z -18";
            const teleportEvidence = {};
            const teleportInspect = async () => (await call("Runtime.evaluate", {
              expression: `({ focused: document.hasFocus(), visibility: document.visibilityState,
                canvasActive: document.activeElement === Module.canvas,
                events: globalThis.islandSmokeResult?.inputEvents ?? [],
                error: globalThis.islandSmokeResult?.error,
                outputTail: (globalThis.islandSmokeResult?.output ?? '').slice(-2000) })`,
              returnByValue: true,
            }, sessionId)).result.value;
            let teleported = await waitForOutput(sessionId, teleportPattern, 1);
            if (!teleported) {
              try {
                teleported = await acknowledgedInputGesture({ evidence: teleportEvidence, inspect: teleportInspect,
                  press: () => call("Input.dispatchKeyEvent", { type: "keyDown", key: "F7", code: "F7",
                    windowsVirtualKeyCode: 118, nativeVirtualKeyCode: 118 }, sessionId),
                  acknowledge: () => waitForOutput(sessionId, teleportPattern, 60),
                  release: () => call("Input.dispatchKeyEvent", { type: "keyUp", key: "F7", code: "F7",
                    windowsVirtualKeyCode: 118, nativeVirtualKeyCode: 118 }, sessionId),
                });
              } finally {
                await writeFile(path.join(artifactDirectory, "island-browser-grass-motion-teleport-gesture.json"),
                  `${JSON.stringify({ ...teleportEvidence, acknowledged: teleported === true }, null, 2)}\n`);
              }
            }
            if (!teleported) throw new Error("Grass motion east-clearing setup failed");
          }
          const screenshotPath = await captureVisualFrame(sessionId,
            `island-browser-${focusedArtifactPrefix}-${mode}.png`,
            !["spiders", "spider-web", "goats", "deer", "boars", "monkeys", "monkey-climb", "overview", "flight", "snakes", "snake-head", "tree-snakes"].includes(mode));
          const motionReview = focusedGrassMotionCheck ? await runGrassMotionReview(sessionId) : null;
          // Preserve intermediate poses for real visual review of the quadrupedal gait;
          // endpoint screenshots alone can hide foot sliding and limb inversion.
          const monkeyMotionFrames = [];
          const snakeMotionFrames = [];
          const goatMotionFrames = [];
          const spiderMotionFrames = [];
          if ((focusedSpiderCheck && mode === "spiders") || focusedGoatCheck || focusedMonkeyCheck || focusedSnakeCheck) {
            for (let pose = 0; pose < 4; ++pose) {
              await call("Runtime.evaluate", {
                expression: `new Promise(resolve => {
                  let frames = 0;
                  const tick = () => ++frames >= 8 ? resolve(true) : requestAnimationFrame(tick);
                  requestAnimationFrame(tick);
                })`, awaitPromise: true, returnByValue: true,
              }, sessionId);
              const frame = path.basename(await captureVisualFrame(sessionId,
                `island-browser-${focusedArtifactPrefix}-${mode}-pose-${pose}.png`, false));
              if (focusedSpiderCheck && mode === "spiders") spiderMotionFrames.push(frame);
              else if (focusedGoatCheck) goatMotionFrames.push(frame);
              else if (focusedSnakeCheck) snakeMotionFrames.push(frame);
              else monkeyMotionFrames.push(frame);
              const checkpoint = await call("Runtime.evaluate", {
                expression: `({ result: globalThis.islandSmokeResult,
                  contextLost: Module.canvas.getContext('webgl2')?.isContextLost() })`,
                returnByValue: true,
              }, sessionId);
              // Keep attribution even if the outer supervisor reaches its deadline
              // before the final 60-interval result. Partial evidence is never a pass.
              await writeFile(path.join(artifactDirectory, `island-browser-${focusedArtifactPrefix}-progress.json`),
                `${JSON.stringify({ complete: false, spiderMotionFrames, goatMotionFrames, monkeyMotionFrames, snakeMotionFrames,
                  checkpoint: checkpoint.result.value }, null, 2)}\n`);
            }
          }
          const sampled = await call("Runtime.evaluate", {
            expression: `new Promise(resolve => {
              const gl = Module.canvas.getContext('webgl2');
              const debug = gl?.getExtension('WEBGL_debug_renderer_info');
              const renderer = debug ? gl.getParameter(debug.UNMASKED_RENDERER_WEBGL) : '';
              const intervals = [];
              let previous;
              const tick = now => {
                if (previous !== undefined) intervals.push(now - previous);
                previous = now;
                if (intervals.length < ${focusedFrameCount}) requestAnimationFrame(tick);
                else resolve({ renderer, intervals, result: globalThis.islandSmokeResult,
                  contextLost: !gl || gl.isContextLost(), glError: gl?.getError() });
              };
              requestAnimationFrame(tick);
            })`, awaitPromise: true, returnByValue: true,
          }, sessionId);
          const state = sampled.result.value;
          if (!state) throw new Error(`${mode} did not return frame evidence`);
          const afterSimulationScreenshot = focusedSpiderCheck || focusedGoatCheck || focusedHutCheck || focusedHelicopterCheck || focusedCampfireCheck || focusedCaveCheck || focusedBunkerCheck || focusedTurtleCheck || focusedCrabCheck || focusedHermitCrabCheck || focusedCoconutCrabCheck || focusedButterflyCheck || focusedGeckoCheck || focusedSkinkCheck || focusedDragonflyCheck || focusedFrogCheck || focusedRodentCheck || focusedFlyCheck || focusedAntCheck || focusedCicadaCheck || focusedBeetleCheck || focusedCrocodileCheck || focusedFishCheck || focusedOctopusCheck || focusedSharkCheck || focusedDolphinCheck || focusedBearCheck || focusedResidentCheck || focusedSnakeCheck || focusedFlightCheck || focusedStreamCheck || focusedShipwreckCheck || mode === "boars" || mode === "monkeys" || focusedClimbCheck || focusedCoastCheck || focusedGroundingCheck || focusedVegetationCheck || focusedMangroveCheck || focusedTerrainCheck
            ? path.basename(await captureVisualFrame(sessionId,
              `island-browser-${focusedArtifactPrefix}-${mode}-after-simulation.png`, false)) : null;
          const log = state.result?.output ?? "";
          const passed = /SwiftShader/i.test(state.renderer) && !state.contextLost
            && log.includes("island-simulator: edge antialiasing requested 4x renderer-applied 4x")
            && (!focusedGrassMotionCheck || (motionReview?.passed
              && log.includes("close grass uses broadened planted bases and lower-blade curvature")))
            && (!focusedFlightCheck || flightReview?.passed)
            && (!focusedSpiderCheck || (mode === "spiders" ? (spiderMotionFrames.length === 4
              && /spider render proof: spiders (?:[1-9]|[1-9][0-9]|1[0-9]{2}|200)\/200 visible, 2 draw; contact-shadows=(?:[1-9]|[1-9][0-9]|1[0-9]{2}|200)(?:\s|$)/.test(log)
              && log.includes("spider setae loaded Content/wildlife/spider-setae.png; 256x256, 9 mips, 349524 RGBA8 bytes")
              && log.includes("near spiders use tiered fitted eyes with upper-forward catchlights")
              && log.includes("near spider abdomens use a tapered muted dorsal lance in the existing setae draw"))
              : /spider-web render proof: webs [1-6]\/6 visible, strands [1-9][0-9]*, 1 draw/.test(log)))
            && (!focusedGoatCheck || (goatMotionFrames.length === 4
              && /goats [1-6]\/6 visible, 2 draw;/.test(log)
              && log.includes("goat fur loaded Content/wildlife/goat-fur.png; 256x256, 9 mips, 349524 RGBA8 bytes")
              && log.includes("goats use separated haunch, waist and raised shoulder masses, six-station necks, and six deterministic natural coat tints")
              && log.includes("near goats use paired surface nostrils and fitted lateral mouth creases")))
            && (!focusedDeerCheck || (log.includes("four inland deer use long legs, pointed ears, spotted coats and one antlered silhouette")
              && log.includes("near deer use vertex-blended ellipsoid light, curved tapered necks, articulated hocks, fitted muzzles, and smooth antler beams, with continuous five-ring necks, separate nasal pads, and surface-fitted oval coat spots")
              && log.includes("deer use separated chest and haunch masses, muscle-profiled upper legs, and normal-driven dorsal-to-belly coat tone")
              && log.includes("deer QA camera frames one antlered clearing animal broadside")
              && /deer [1-4]\/4 visible, 1 draw/.test(log)))
            && (!focusedHutCheck || (log.includes("hut beam-aligned weathered grain and filtered thatch fibres")
              && log.includes("hut timber material uses coarse warped grain, recessed dark fibres, knot bands, and sun-bleached board variation")
              && log.includes("hut four-pane muted glazing and braced plank door")
              && log.includes("hut raised veranda uses full-height outer posts, split rails, fascia, and three steps")
              && log.includes("hut enclosed slatted gable, framed attic light, and four knee-braced veranda supports")
              && log.includes("hut wall boards meet edge-to-edge on all four sides and both gables without see-through gaps")
              && log.includes("hut palm fibre loaded Content/structures/hut-palm-fibre.png; 256x256, 9 mips, 349524 RGBA8 bytes")
              && log.includes(`hut 1 draw, ${mode === "hut" ? 7 : 6} contact patches`)))
            && (!focusedHelicopterCheck || (log.includes("helicopter uses a smoothly tapered thirty-two-sided cockpit canopy, an articulated eight-link rotor hub, tubular rising-tip skids, and a tapered tailplane")
              && log.includes("helicopter main rotor uses four closed tapered blades with visible leading, trailing, and tip edges")
              && log.includes("helicopter rotor head adds four blade grips, pitch links, a mast cap, and muted yellow tip bands")
              && log.includes("helicopter raised tapered engine cowling feeds two rear exhaust outlets behind the rotor mast")
              && log.includes("helicopter uses a generated 128-square atlas for worn paint, brushed metal, woven seats, and fine rubber")
              && log.includes("helicopter carries four outward-open framed door panels with glazing and handles")
              && log.includes("helicopter cockpit uses four round gauges, paired articulated cyclic and collective controls, and four pedals")
              && log.includes("helicopter instruments use recessed ticked dials, one artificial horizon, needle hubs, and framed multi-line radio displays")
              && log.includes("helicopter open cabin uses restrained indirect light, a tapered instrument binnacle, and twin radio displays")
              && log.includes("helicopter realism pass adds a rounded chin and tail-root fairing, twelve-sided six-station tail boom, maintenance seams, engine intakes, shaped cushions, harnesses, centre pedestal, and overhead console")
              && log.includes("helicopter curved shell uses analytic per-vertex lighting across the canopy, cowling, tail boom, fairings, and cushions without added geometry")
              && log.includes("helicopter cockpit uses a fuller nine-station longitudinal bubble and a progressively wrapped lower windscreen")
              && log.includes("helicopter divided windscreen carries paired articulated parked wipers and restrained translucent sky glints")
              && log.includes("parked main rotor uses three-span quadratic blade droop, progressive chord taper, and restrained tip sweep")
              && log.includes("parked main rotor uses closed cambered airfoil sections with fine leading and trailing edges")
              && log.includes("lateral tail-rotor shaft carries four closed swept blades with visible chord, thickness, and muted tip bands")
              && log.includes("helicopter empennage uses a closed swept vertical fin and separate tapered horizontal stabilizers")
              && log.includes("helicopter four open doorways use twin-rail tubular anti-slip entry steps instead of solid slabs")
              && log.includes("helicopter cabin uses rounded closed pilot cushions on exposed metal frames and a seam-divided rear bench")
              && log.includes("helicopter rotor mast uses concentric tubular shafts, a flattened round swashplate, and a rounded gearbox housing")
              && /helicopter 2 draw/.test(log)))
            && (!focusedCampfireCheck || (log.includes("campfire first visible animated draw")
              && log.includes("campfire flame material uses broken translucent tongues, pale cores, a restrained blue base, and layered smoke")
              && log.includes("campfire bed uses a pale mottled ash centre, scorched rim, and mixed glowing charcoal")
              && log.includes("campfire and stored firewood stand on qualified broad dry-sand beach")
              && log.includes("campfire 1992 solid triangles, 42 flame/smoke/spark triangles in two culled draws")))
            && (!focusedCaveCheck || (log.includes(`cave QA camera ${mode === "cave-inside" ? "inside" : "outside"}`)
              && log.includes("cave rock first visible draw")
              && log.includes("cave rock 2623 triangles in one culled draw; 20m walkable interior")
              && log.includes("cave terrain shoulders use an irregular rock apron around the open corridor")
              && log.includes("cave connected shell strips share smooth lighting normals while portal breaks stay sharp")
              && log.includes("cave adds six high-clearance eroded ceiling pendants in the existing draw")
              && log.includes("cave mouth is embedded in a rising inland mountain slope")
              && log.includes("cave crown is concealed from above by blended earth and 371 irregular three-blade tufts in the existing draw")
              && log.includes("cave rock loaded Content/structures/cave-rock.png; 256x256, 9 mips, 349524 RGBA8 bytes; mirrored triplanar sampling")))
            && (!focusedBunkerCheck || (log.includes(`bunker QA camera ${mode}`)
              && log.includes("concealed inland bunker has an open rock portal, descending access corridor, computer room, service room, and fictional reactor-generator chamber")
              && log.includes("bunker uses bounded baked light pools, luminous ceiling fixtures, active terminal screens, and a contained cyan generator core in two static draws")
              && log.includes("bunker computer room has four desks and terminals, keyboard consoles, eight server cabinets, and a separate generator control bank")
              && log.includes("bunker concrete and metal use portable world-space mottling, restrained age variation, and baked face shading")
              && log.includes("bunker deep spine ends at a sealed maintenance bulkhead and the service branch has a complete lit shell")
              && !log.includes("bunker material fallback:")))
            && (!focusedTurtleCheck || (log.includes("near turtles use staggered scutes and closed tapered-volume paddles")
              && log.includes("near turtles use compact integrated heads, narrower hydrodynamic paddles, and restrained shell seams")
              && log.includes("near turtles use a twenty-by-sixteen carapace surface, eighteen-sided seven-ring heads, and irregular mottled shell and skin tones")
              && /turtles [1-5]\/5 visible \(near [1-5], far [0-5]\), 1 draw/.test(log)))
            && (!focusedCrabCheck || (log.includes("near crabs use tapered toes, socketed eyes, and solid pincer fingers")
              && log.includes("near crabs use flattened lateral carapaces, sturdy legs, and enlarged claw palms")
              && log.includes("near crabs use eighteen-sided smooth-lit mottled carapaces and solid tapered legs")
              && log.includes("near crabs use low broad shells with retained lateral carapace corners")
              && log.includes("near crab carapaces close around a shallow dark underside instead of ending at an open rim")
              && log.includes("near beach crabs use rounded claw palms and solid tapered forearms")
              && log.includes("six debris-linked crab colonies share visible sand burrows")
              && log.includes("live beach crabs keep 0.72m centre separation while foraging")
              && log.includes("crab QA camera frames a debris-linked burrow and its foraging colony")
              && /crab QA subject [0-9]+ is [0-9.]+m from the burrow/.test(log)
              && /crabs [1-9][0-9]*\/30 visible \(near [1-9][0-9]*, far [0-9]+\), 1 draw/.test(log)))
            && (!focusedHermitCrabCheck || (log.includes("ten sheltered-beach hermit crabs pause, forage, and retreat in the shared crab batch")
              && log.includes("hermit crabs use raised spiral shells, projecting eyes and antennae, six walking legs, and unequal claws")
              && log.includes("hermit-crab QA camera frames one shell-bearing animal on sheltered sand")
              && /hermit crabs [1-9][0-9]*\/10 visible in shared draw/.test(log)))
            && (!focusedCoconutCrabCheck || (log.includes("six coastal palm-zone coconut crabs rest, forage, and retreat in the shared crab batch")
              && log.includes("coconut crabs use broad armored bodies, segmented terrestrial abdomens, robust walking legs, and massive unequal claws")
              && log.includes("coconut crabs use elevated compact bodies, planted high-jointed legs, and arched abdominal plates")
              && log.includes("near coconut crabs use rounded unequal claw palms over separated solid pincer fingers")
              && log.includes("coconut crabs use cool blue-grey, violet-brown, slate, and warm umber limb armour with a front-raised terrestrial stance")
              && log.includes("coconut-crab QA camera frames one large terrestrial animal in coastal vegetation")
              && /coconut crabs [1-6]\/6 visible in shared draw/.test(log)))
            && (!focusedButterflyCheck || (log.includes("butterflies feed at three visible deterministic nectar-flower patches")
              && log.includes("butterfly QA camera frames one concrete nectar patch and its six animals")
              && /butterflies [1-9][0-9]*\/18 visible \(near [1-9][0-9]*, far [0-9]+\), 1 draw/.test(log)))
            && (!focusedGeckoCheck || (log.includes("eight inland geckos rest, scurry from the player, and hide in the shared butterfly batch")
              && log.includes("gecko QA camera frames a resting inland lizard at ground level")
              && /geckos [1-8]\/8 visible in shared draw/.test(log)))
            && (!focusedSkinkCheck || (log.includes("eight open-ground skinks bask, forage, and flee in the shared butterfly batch")
              && log.includes("skinks use smooth elongated trunks, short articulated limbs, tapered tails, longitudinal stripes, and highlighted eyes")
              && log.includes("skink QA camera frames one striped basking lizard on open inland ground")
              && /skinks [1-8]\/8 visible in shared draw/.test(log)))
            && (!focusedDragonflyCheck || (log.includes("nine stream and wetland dragonflies hover, patrol, and dart in the shared butterfly batch")
              && log.includes("dragonfly QA camera frames one visible wet-habitat insect broadside")
              && /dragonflies [1-9]\/9 visible in shared draw/.test(log)))
            && (!focusedFrogCheck || (log.includes("ten stream and wetland frogs rest, hop, and retreat in the shared butterfly batch")
              && log.includes("frog QA camera frames one resting stream-bank animal")
              && /frogs [1-9][0-9]*\/10 visible in shared draw/.test(log)))
            && (!focusedRodentCheck || (log.includes("twelve field mice and eight rats forage, scurry, and hide in the shared small-fauna batch")
              && log.includes("near rodents use species-scaled lofted bodies, pointed muzzles, rounded ears, whiskers, paws, eyes, and long segmented tails")
              && (mode !== "mice" || (log.includes("mouse QA camera frames one field mouse at ground level")
                && /mice [1-9][0-9]*\/12 visible in shared draw/.test(log)))
              && (mode !== "rats" || (log.includes("rat QA camera frames one larger inland rat at ground level")
                && /rats [1-9][0-9]*\/8 visible in shared draw/.test(log)))))
            && (!focusedFlyCheck || (log.includes("four close-range fly swarms use twenty-four hover, orbit, and dart agents in the shared small-fauna batch")
              && log.includes("near flies use compact dark thoraxes, tapered abdomens, highlighted eyes, six legs, and two beating pale wings")
              && log.includes("fly QA camera frames one close insect within its inland swarm")
              && /flies [1-9][0-9]*\/24 visible in shared draw/.test(log)))
            && (!focusedAntCheck || (log.includes("four close-range ant trails use sixty-four soil and trunk-climbing agents in the shared small-fauna batch")
              && log.includes("close ants use three-part bodies, six articulated legs, paired antennae, and visible carried leaf fragments")
              && log.includes("ant QA camera frames a visible worker on one inland soil trail")
              && /ants [1-9][0-9]*\/64 visible in shared draw/.test(log)))
            && (!focusedCicadaCheck || (log.includes("twelve cicadas rest, call, and make short flights from real broadleaf trunks in the shared small-fauna batch")
              && log.includes("close cicadas use broad heads, highlighted eyes, stout tapered bodies, six legs, and two veined roof-folded wings")
              && log.includes("cicada QA camera frames one trunk-resting animal broadside")
              && /cicadas [1-9][0-9]*\/12 visible in shared draw/.test(log)))
            && (!focusedBeetleCheck || (log.includes("fifteen tropical ground beetles rest, forage, and hide in three inland leaf-litter colonies")
              && log.includes("close beetles use domed paired elytra, a centre seam, pronotum, head, antennae, eyes, and six articulated legs")
              && log.includes("beetle QA camera frames one foraging animal in inland leaf litter")
              && /beetles [1-9][0-9]*\/15 visible in shared draw/.test(log)))
            && (!focusedCrocodileCheck || (log.includes("crocodiles use long low armored bodies, broad skulls, muscular tails, and planted splayed limbs")
              && log.includes("near crocodiles use raised eyes, nostrils, separate jaws, teeth, dorsal osteoderms, and wet mottled facet shading, with multi-section rounded snouts and lateral eyes")
              && log.includes("crocodile armor uses low paired trunk scutes that merge into a diminishing articulated tail keel")
              && log.includes("near crocodile torso, tail, skull, jaw, and limb roots use bounded reticulated facet pigment")
              && log.includes("crocodile QA camera frames an estuary animal broadside")
              && /crocodiles [1-3]\/3 visible \(near [1-3], far [0-3]\), 1 draw/.test(log)))
            && (!focusedFishCheck || (log.includes("near fish use eleven-station sixteen-sided bodies, closed swept fins, and fitted highlighted eyes")
              && log.includes("near fish use softly interpolated family flank lines, bars, and caudal marks")
              && log.includes("fish fins inherit body-family pigment with dark fitted root edges")
              && log.includes("fish use narrow caudal peduncles and shallow swept forked tails")
              && log.includes("near fish use capped smooth-lit bodies, caudal peduncles and progressive rear-body bends")
              && log.includes("fish profile pass uses six-facet oval eyes, tapered fork tips, and a lower rear-swept dorsal")
              && log.includes("near fish use blunt convex foreheads and rounded snouts instead of needle-point wedges")
              && log.includes("near fish use restrained upper-flank iridescence and palette-matched gill creases")
              && /fish [1-9][0-9]*\/48 visible \(near [1-9][0-9]*, far [0-9]+\), 1 draw/.test(log)))
            && (!focusedStingrayCheck || (log.includes("stingrays use broad undulating discs, raised eyes, dorsal spots, pale undersides, and long tapered tails in one draw")
              && log.includes("near stingrays use a curved diamond disc, raised central body, wedge nose, and restrained dark rim")
              && log.includes("stingray near/far geometry 846/18 vertices with a crossed-volume tapered tail")
              && log.includes("stingrays 5 agents glide, forage, and settle over shallow sandy seabed")
              && log.includes("stingray QA camera follows one shallow-sand glide")
              && /stingrays [1-5]\/5 visible \(near [1-5], far [0-5]\), 1 draw/.test(log)))
            && (!focusedOctopusCheck || (log.includes("octopuses use pulsing mantles, expressive eyes, eight independently curled arms, and pale sucker rows in one draw")
              && log.includes("near octopuses use a rear-swept mantle, smooth analytic skin light, and thicker tapered arms")
              && log.includes("octopus near/far geometry 3186/24 vertices in one bounded batch")
              && log.includes("near octopuses use raised ocular bulbs, horizontal pupils, and fitted eye highlights")
              && log.includes("octopuses 4 agents den, crawl, forage, and swim in reef and rocky shallows")
              && log.includes("octopus QA camera frames one exposed reef forager and all eight arms")
              && /octopuses [1-4]\/4 visible \(near [1-4], far [0-4]\), 1 draw/.test(log)))
            && (!focusedBirdCheck || (log.includes("canopy flock includes hooked-beak long-tailed parrots and compact tropical fruit doves")
              && log.includes("four fruit doves alternate canopy perches with bounded dry-ground walking and foraging")
              && log.includes("near birds use compact rounded heads, camera-facing six-facet eyes, fitted hooked beaks, solid swept wings, and split tail feathers")
              && log.includes(mode === "ground-doves"
                ? "ground-dove QA camera frames one walking fruit dove on dry interior terrain"
                : "bird QA camera frames one flying hooked-beak parrot")
              && /birds [1-9][0-9]*\/24 visible \(near [1-9][0-9]*, far [0-9]+\), 1 draw/.test(log)))
            && (!focusedSharkCheck || (log.includes("near shark uses a fourteen-station eighteen-sided capped body, closed fins, five paired gill slits, and fitted eyes")
              && log.includes("shark retains deep dorsal and pale ventral countershading through water tint")
              && log.includes("near shark uses a smooth-lit blunt head and body, caudal peduncle and heterocercal tail")
              && log.includes("shark uses a long low spindle, reinforced caudal peduncle, and swept restrained fins")
              && log.includes("shark profile pass uses charcoal flank contrast, a lower rear-swept dorsal, and shorter pectorals")
              && log.includes("shark skin pass preserves a dark dorsal and fin-edge silhouette with a raised pale lower flank after water tint")
              && log.includes("shark near/far geometry 1896/15 vertices, closed fins and CPU faceted daylight")
              && /shark visible activity \S+, 1 draw/.test(log)))
            && (!focusedDolphinCheck || (log.includes("near dolphins use closed-volume fins and flukes, short beaks, faceted eyes, mouth creases, and a blowhole")
              && log.includes("near dolphins use a nineteen-station closed melon profile, twenty-four-sided body, and vertex-blended countershading")
              && log.includes("dolphin tail stock bends vertically through horizontal fluke strokes")
              && log.includes("near dolphins use an extended fitted rostrum, lower swept fins, and readable grey countershading")
              && log.includes("dolphin facial pass uses a stout tapered rostrum and small six-facet oval eyes instead of flat diamond marks")
              && log.includes("dolphin skin pass uses continuous axial pigment, a pale swept lower flank, restrained tonal variation, a longer rounded rostrum, and softer swept fins")
              && log.includes("near dolphins use a slimmer fusiform midbody, narrower bottlenose rostrum, stronger countershading, a taller falcate dorsal, and root-thickened tapered fins and flukes")
              && log.includes("near dolphins use narrow curved-back pectorals and a restrained fitted lower-jaw mouth seam")
              && log.includes("dolphin silhouette pass uses a ten-point falcate dorsal, nine-point curved pectorals, eight-point forked flukes, and brighter swept ventral countershading")
              && log.includes("dolphin near/far geometry 3300/15 vertices, closed fins and CPU faceted daylight")
              && /dolphins [1-4]\/4 visible \(near [1-4], far [0-4]\), 1 draw/.test(log)))
            && (!focusedShipwreckCheck || (log.includes("shipwreck wood loaded Content/structures/shipwreck-weathered-wood.png; 256x256, 9 mips, 349524 RGBA8 bytes; mirrored interior atlas")
              && log.includes("shipwreck weathered wood follows per-piece board and beam UVs")
              && log.includes("shipwreck restrained rusted iron straps, rivets, hinges and rudder braces")
              && log.includes("shipwreck hull uses asymmetric rising sheer, curved keel, and irregular splintered break sections")
              && log.includes("shipwreck beach broadside has true missing plank bays with exposed structural ribs")
              && log.includes("shipwreck bow and stern use raked lower profiles with thick end timbers")
              && log.includes("shipwreck missing plank bays expose eleven-centimetre inner edges")
              && log.includes("shipwreck stern deckhouse uses separated broken roof planks and framed window bays")
              && log.includes("shipwreck beach debris uses buried ends, restrained warp, unequal settling, and five genuinely split long planks")
              && log.includes("shipwreck 20m broken hull, 12 debris pieces, one portable culled batch, 5712 triangles")
              && log.includes("shipwreck visible, 12 debris, 1 draw")))
            && (!focusedBearCheck || (log.includes("bear QA camera frames and follows the single peaceful adult")
              && log.includes("bear fur loaded Content/wildlife/bear-fur.png; 128x128, 8 mips, 87380 RGBA8 bytes")
              && log.includes("near bear uses twenty-two body-envelope fur wedges without another draw")
              && log.includes("near bear uses a two-volume tapered muzzle, volumetric nose, paired nostrils, divided mouth crease, and highlighted eyes")
              && log.includes("bear legs use continuous shoulder-to-wrist lofts and five-clawed broad paws")
              && log.includes("near bear paws use short two-segment horn claws with buried roots and downward tips")
              && log.includes("bear torso uses a continuous rump-to-neck loft without overlapping body shells")
              && log.includes("near bear uses a broader rounded skull, fitted brow volumes, larger rounded ears, and a short wide muzzle")
              && /bear 1\/1 visible, 2 draws, [1-9][0-9]* body triangles/.test(log)))
            && (!focusedResidentCheck || (log.includes("resident QA camera frames and follows one peaceful adult")
              && log.includes("resident uses a shoulder-sloped woven tunic with open cap sleeves and anatomically tapered calves")
              && log.includes("resident uses buried deltoid shoulders, split hands, and layered facial anatomy")
              && log.includes("resident anatomy uses a tapered trapezius neck, rounded sleeve-deltoid transitions, shaped forearms, cheeks, jaw, eyelids, and nostril wings")
              && log.includes("resident face uses a restrained nasal tip and separate upper and lower lip contours")
              && log.includes("resident woven clothing adds a bound V neckline and split thigh-following wrap")
              && log.includes("resident gait uses foot-synchronised thirty-degree arm counter-swing with relaxed elbows")
              && /resident 1\/1 visible, 2 draws, [1-9][0-9]* body triangles/.test(log)))
            && (!focusedStreamCheck || (log.includes("route-following downstream ripple and translucent current streaks")
              && log.includes("stream water uses bank-edge sheen and broken shallow riffles")
              && log.includes("stream water uses depth-graded colour and braided moving current threads")
              && log.includes("stream channel uses shared low-frequency pools and pinches")
              && log.includes("stream route uses three broad valley-scale bends with fixed source and mouth")
              && /stream-bank stones (?:1[0-8]), non-solid existing rock variants beyond the carved bank/.test(log)
              && log.includes("stream terrain uses a compact incised profile and slope-limited exposed bank strata")
              && log.includes("stream water first visible draw")))
            && (!focusedWetlandCheck || (log.includes(
                "seasonal damp depression 42 fern-rich ground plants, dark humic soil, no permanent water")
              && /jungle [1-9][0-9]* canopy trees, [1-9][0-9]* understory plants/.test(log)
              && !log.includes("terrain material fallback:")))
            && (!focusedSnakeCheck || (snakeMotionFrames.length === 4
              && log.includes("snakes use a readable dorsal saddle and active forked-tongue flicks")
              && log.includes("near snakes use a broad low temple, tapered blunt muzzle, raised amber eyes, and fitted vertical pupils")
              && (mode !== "snake-head" || log.includes("snake-head QA isolates facial geometry while the companion body view retains habitat grass"))
              && (mode !== "tree-snakes" || (log.includes("tree-snake QA frames a live climber on the sampled trunk surface")
                && log.includes("six arboreal snakes cling and climb on sampled broadleaf trunk triangles")))
              && /snakes (?:[1-9]|1[0-6])\/56 visible, 1 draw/.test(log)))
            && (!focusedClimbCheck || log.includes("monkey climb held summit presented after eight cycle stages"))
            && (!focusedPalmCheck || (log.includes("palm crowns use dense narrow leaflets, irregular arcs, and emerging spear fronds")
              && log.includes("palm far crowns retain twelve arched ribbons and two narrow emerging spears")
              && log.includes("palm storm history 6 fallen, 8 broken trunks")
              && log.includes("palm groves include grounded fallen crowns and splintered storm-broken trunks")
              && /palms [1-9][0-9]*\/327 \(near [1-9][0-9]*, far [0-9]+\)/.test(log)))
            && (!focusedMangroveCheck || (log.includes("mangrove salt-worn bark uses a pale warm palette above shared damp-base moss")
              && log.includes("mangrove QA target exposes stilt roots and low crown")
              && /localized mangrove colony (?:1[5-9]|2[0-9]|30) trees, low crowns, multiple stems, stilt roots, canopy variant 8/.test(log)))
            && (!focusedCliffCheck || (log.includes("terrain steep faces use dominant-axis rock projection with broken bedding and joints")
              && log.includes("steep terrain rock macrocolour follows the cliff face instead of top-down curtains")
              && log.includes("inland ridge rock starts on sustained slopes and retains cool sky fill in fixed-sun shade")
              && log.includes("inland ridge outcrops follow measured steepness with broken medium-scale exposure")
              && log.includes("low coastal cliffs retain salt-worn mineral colour and cool open-sky fill")))
            && (!focusedSkyCheck || (log.includes("sky cumulus use layered billow silhouettes with blue-grey undersides and sunlit crowns")
              && log.includes("sky cumulus use compact normalized breakup instead of horizon-spanning ribbons")))
            && (!focusedCanopyLightCheck || (log.includes("canopy shade adds bounded world-anchored low-frequency sun patches")
              && log.includes("canopy trunks use root flare, rounded buttress ridges and grey-brown bark with shade-facing moss, lichen, contact occlusion and canopy shade")
              && /canopy age structure includes [1-9][0-9]? deterministic 2-5m saplings beside established stands; first=-?[0-9]+:-?[0-9]+/.test(log)))
            && (!focusedSaplingCheck || (log.includes("saplings use a flexible post-root sweep and unequal lateral crown tiers within expanded bounds")
              && log.includes("sapling QA target variant=")
              && /canopy age structure includes [1-9][0-9]? deterministic 2-5m saplings beside established stands; first=-?[0-9]+:-?[0-9]+/.test(log)))
            && (!focusedBushCheck || (log.includes("shrubs use early radial spread, crooked shoots, independently rolled cupped leaves, and wider pigment variation")
              && log.includes("bush QA target variant 0 at player-eye detail distance")))
            && (!focusedGroundBroadleafCheck || (log.includes("juvenile ground broadleaf uses layered arching laminae, readable midribs, and retained near/far leaf identities")
              && log.includes("ground-broadleaf QA target variant 1 at player-eye detail distance")))
            && (!focusedSplitGroundCheck || (log.includes("split ground leaves use layered asymmetric lobes, readable midribs, shared LOD knots, and root-anchored wind")
              && log.includes("ground QA target variant 2 at player-eye detail distance")))
            && state.glError === 0 && !state.result?.error && !state.result?.exited
            && log.includes("island-simulator: renderer WEBGL2")
            && log.includes("atmosphere fog colour shares one CPU and portable shader definition")
            && log.includes("ocean 160x160 textures 10091644/10111860 bytes audio 6/6 sources")
            && log.includes("forest-floor material loaded Content/terrain/forest-floor.png; 512x512, 10 mips, 1398100 RGBA8 bytes; interior windows")
            && log.includes("leaf material loaded Content/vegetation/leaf-lamina.png; 256x256, 9 mips, 349524 RGBA8 bytes")
            && /forest-floor filter (?:anisotropic, requested maximum 4x, device-clamped|trilinear, anisotropy unavailable)/.test(log)
            && log.includes("grass shared rest-shape normals follow curvature and instance scale")
            && log.includes("grass retains foliage colour at distance with normalized-height basal tint")
            && log.includes("grass pigment uses separate height occlusion and filtered longitudinal veins")
            && log.includes("grass LOD proxies share seven tuft roots, profiles and wind phases")
            && log.includes("grass anchor-distance contraction uses nearest-bound cell culling")
            && log.includes("juvenile ground broadleaf uses layered arching laminae, readable midribs, and retained near/far leaf identities")
            && log.includes("split ground leaves use layered asymmetric lobes, readable midribs, shared LOD knots, and root-anchored wind")
            && log.includes("shrubs use supported shoots and curved individual leaves with shared LOD identities and complete leaf outlines")
            && log.includes("forest-floor colonies retain route footprints; groups=")
            && (!focusedMaterialsCheck || (log.includes("wet sand uses a broader tide-darkened band with cool grazing sky sheen")
              && log.includes("wet sand adds filtered shore-normal drainage threads and a warm shallow-film response")))
            && log.includes("jungle root origins and culling bounds follow rendered terrain triangles")
            && log.includes("fallen leaves conform to rendered triangles using the shared grass height texture")
            && log.includes("forest-floor litter includes two terrain-draped curved twigs in the shared near rosette mesh")
            && (mode !== "bush" || log.includes("bush QA target variant 0 at player-eye detail distance"))
            && (mode !== "ground-broadleaf" || log.includes("ground-broadleaf QA target variant 1 at player-eye detail distance"))
            && (mode !== "litter" || log.includes("ground-litter QA target variant 0 at player-eye detail distance"))
            && log.includes("grass mesh vertices/triangles near=862/640 middle=60/40 far=42/28")
            && log.includes("grass progressive near taper and seed roots share the sampled blade profile")
            && log.includes("grass near blades form seven unequal fanned tufts with coherent wind")
            && log.includes("grass near mesh adds 48 low curved turf blades with height-scaled motion")
            && log.includes("grass roots sample RGB24 rendered terrain triangles before wind")
            && log.includes("grass culling bounds cover measured meshes, slope and full shared motion limits")
            && log.includes("shoreline advancing wash uses footprint-filtered porous foam")
            && log.includes("ocean shared nine-wave spectrum uses filtered short-wave normals")
            && log.includes("shallow ocean uses filtered crossing ripple cells with restrained sky flecks")
            && log.includes("ocean ripple cells use filtered low-frequency phase warp to avoid a repeated reef lattice")
            && log.includes("ocean shallow attenuation uses interpolated bed depth and contact fade")
            && log.includes("distant grazing ocean converges on shared sky haze without a horizon seam")
            && log.includes("sky horizon locks to shared aerial haze above the below-horizon skirt")
            && log.includes("coastal sand stays continuous below water with filtered seabed ripples")
            && log.includes("exposed beach sand reuses filtered broad fields for warm and cool mineral patches")
            && log.includes("reef substrate separates pale lagoon, shared habitat patches, crest, sand channels, and deep drop-off")
            && log.includes("coastal contact patches follow rendered seabed triangles in the existing batch")
            && log.includes("land prop shadows match model yaw and rendered terrain triangles")
            && log.includes("wreck, hut, and helicopter project bounded fixed-sun mass shadows in the shared contact batch")
            && log.includes("wood uses outward shared normals and scale-correct fixed-sun lighting")
            && log.includes("ocean bounded grazing contrast and depth-limited filtered breaker foam")
            && log.includes("ocean short waves use phased wind packets with analytic envelope gradients")
            && (!focusedGroundingCheck || /contact shadows [1-9][0-9]*, [1-9][0-9]* triangles, 1 draw/.test(log))
            && log.includes("rocks use shared smooth normals, scale-correct daylight and filtered mineral weathering")
            && log.includes("near rocks use filtered mineral strata and a local soil-stained base")
            && (mode !== "shipwreck" || /coastal contact shadows [1-9][0-9]*/.test(log))
            && log.includes("near understory uses curved five-column blades with raised midribs")
            && log.includes("terrain close soil uses footprint-filtered clods and analytic relief")
            && log.includes("forest soil separates humus and moss with footprint-filtered fallen leaves")
            && log.includes("terrain substrate persists at distance with footprint-filtered noise")
            && log.includes("terrain palette overrides preserve smooth slope daylight; opaque 28-byte vertices")
            && log.includes("terrain steep faces use dominant-axis rock projection with broken bedding and joints")
            && log.includes("steep terrain rock macrocolour follows the cliff face instead of top-down curtains")
            && log.includes("inland ridge rock starts on sustained slopes and retains cool sky fill in fixed-sun shade")
            && log.includes("inland ridge outcrops follow measured steepness with broken medium-scale exposure")
            && log.includes("eight structural crown forms share eight LOD lobe anchors and supported trunk bends")
            && log.includes("canopy per-tree frustum boxes contain both LODs and wind")
            && log.includes("clearing-edge colonies respect full route footprints; bushes=")
            && log.includes("understory leaf identities share asymmetric posture and curled profiles across LOD")
            && log.includes("understory pigment stays in a mature-leaf range with pale emerging blades and restrained olive older foliage")
            && log.includes("pinnate understory shares rachis attachment knots and retained leaflet wind across LOD")
            && log.includes("broadleaf paddle and lance laminae share cupped profiles and asymmetric margins across LOD")
            && log.includes("leaf daylight contrast and stable tip pigment; footprint-filtered leaf and bark detail")
            && log.includes("canopy wood separates albedo from baked shade with validated outward normals and one-sided daylight")
            && log.includes("canopy bark follows local wood axes with seam-safe circular footprint filtering")
            && log.includes("canopy bark has instance-stable warped grain and interrupted filtered fissures")
            && log.includes("canopy trunk pigment follows normalized height across LOD tiers")
            && log.includes("canopy bark palette separates pale, warm, and dark fissured tree forms")
            && log.includes("canopy trunks retain diffuse sky fill and readable bark detail in deep crown shade")
            && log.includes("canopy blades use spatial pitch and roll with identical retained LOD poses")
            && log.includes("curved canopy blades retain source identities within original triangle budgets")
            && log.includes("shared fixed-sun tree and palm crown transmission lights terrain and grass")
            && log.includes("canopy shade adds bounded world-anchored low-frequency sun patches")
            && log.includes("jungle ground layers receive shared canopy shade; crowns excluded")
            && log.includes("jungle leaves use smooth normals, filtered veins and restrained wax shading")
            && log.includes("boars receive bilinear per-vertex canopy light in their existing batch")
            && log.includes("boar RGB bristle coat and solid anatomy, two opaque stock BasicEffect draws")
            && log.includes("boars use connected limb poses, stance contact and body-rooted bristles")
            && log.includes("near boars use broad tapered upper legs with compact split hooves")
            && log.includes("near boar limbs blend into overlapping furred shoulder and haunch masses")
            && log.includes("near boar lower legs use a continuous knee-to-cannon taper without spherical joint caps")
            && log.includes("boars use a lifted shoulder mantle and broad sealed nasal disc")
            && log.includes("near boars retain warm shoulder, cheek, nose, and eye contrast in canopy shade")
            && log.includes("boar face retains a warm nasal disc and visible amber eye catchlight in wet understory")
            && log.includes("near adult boars use sharp triangular pinnae and paired two-segment curved ivory tusks")
            && log.includes("boar soles fit rendered terrain independently with fixed-length limbs")
            && !log.includes("boar terrain contact limited by limb reach")
            && log.includes("monkeys receive footprint-averaged canopy light through 14-bone skinning")
            && log.includes("monkey fur loaded Content/wildlife/monkey-fur.png; 512x512, 10 mips, 1398100 RGBA8 bytes")
            && log.includes("monkey fur uses one hidden underside/rear UV seam instead of mirrored crown and dorsal folds")
            && log.includes("near monkey faces use a fitted dark nose pad and raised readable corneas")
            && log.includes("monkey shoulder and hip mantles blend the trunk into all four animated limbs")
            // Curved leaf budgets are unchanged; flared trunks, rounded buttresses and six-sided branches.
            && /canopy mesh variant=0 near-vertices=4874 near-triangles=4688 far-vertices=1071 far-triangles=1120/.test(log)
            && (mode !== "boars" || /boars [1-6]\/6 visible, 2 draw/.test(log))
            && (mode !== "monkeys" || /monkeys [1-7]\/7 visible/.test(log))
            && !/(?:stock-effect|material) fallback:|GL_INVALID_|gl-error |gl-debug .*severity=high/i.test(log);
          const shadeEvent = log.match(/visible monkey entered canopy shade, transmission ([0-9.eE+-]+)/);
          const shadeTransmission = shadeEvent ? Number(shadeEvent[1]) : NaN;
          views.push({ mode, passed, motionReview, monkeyMotionFrames, monkeyShadeTransmission: Number.isFinite(shadeTransmission)
              && shadeTransmission >= 0 && shadeTransmission < 0.95 ? shadeTransmission : null,
            screenshot: path.basename(screenshotPath), afterSimulationScreenshot, spiderMotionFrames, goatMotionFrames, snakeMotionFrames, flightReview, ...state });
          checkKernel(`after-${mode}`);
          if (!passed) throw new Error(`${mode} failed software WebGL checks; see ${reportPath}`);
          console.log(`Island focused WebGL ${mode}: PASS (${focusedFrameCount} CPU-rendered frame intervals)`);
        } catch (error) {
          // Preserve the failing page before closing its context; a partial views array
          // alone cannot distinguish input delivery, application exit and GL failure.
          let state, inspectionError;
          try {
            state = await call("Runtime.evaluate", {
              expression: "globalThis.islandSmokeResult", returnByValue: true,
            }, sessionId);
          } catch (failure) { inspectionError = String(failure); }
          await writeFile(path.join(artifactDirectory, `island-browser-${focusedArtifactPrefix}-${mode}-failure.json`),
            `${JSON.stringify({ mode, error: String(error), state: state?.result?.value, inspectionError }, null, 2)}\n`);
          throw error;
        } finally {
          // Closing the final focused target can make Chrome tear down the renderer before
          // acknowledging the CDP request. Browser.close immediately follows and owns that
          // final target; intermediate targets still close explicitly to bound memory.
          if (mode !== focusedViews[focusedViews.length - 1])
            await closeTargetBounded(targetId);
        }
      }
    } finally {
      await writeFile(reportPath, `${JSON.stringify({ scope: focusedSpiderCheck
          ? "software WebGL spider material, grounding and intermediate poses; not native image-contrast or continuous gait qualification" : focusedGoatCheck
          ? "software WebGL goat coat and intermediate pose images; not continuous stance qualification" : focusedHutCheck
          ? "software WebGL hut overview and material detail; not temporal aliasing or route qualification" : focusedHelicopterCheck
          ? "software WebGL helicopter exterior and interior glazing; not free-walk traversal qualification" : focusedCampfireCheck
          ? "software WebGL campfire flame material and separated-time frames; not speaker or continuous-motion qualification" : focusedCaveCheck
          ? "software WebGL cave exterior/interior material views; not free-walk traversal or final artistic acceptance" : focusedBunkerCheck
          ? "software WebGL bunker exterior, corridor, rooms and sealed end; native controller test owns traversal qualification" : focusedTurtleCheck
          ? "software WebGL close sea-turtle anatomy and separated-time frames; not continuous stroke qualification" : focusedCrabCheck
          ? "software WebGL close beach-crab anatomy and separated-time frames; not continuous gait qualification" : focusedHermitCrabCheck
          ? "software WebGL close hermit-crab anatomy and separated-time frames; not continuous retreat qualification" : focusedCoconutCrabCheck
          ? "software WebGL close coconut-crab anatomy and separated-time frames; not continuous forage qualification" : focusedButterflyCheck
          ? "software WebGL butterfly nectar habitat and separated-time frames; not continuous flight qualification" : focusedGeckoCheck
          ? "software WebGL gecko anatomy and separated-time frames; not continuous flee/hide qualification" : focusedSkinkCheck
          ? "software WebGL skink anatomy and separated-time frames; not continuous flee qualification" : focusedDragonflyCheck
          ? "software WebGL dragonfly anatomy and separated-time frames; not continuous dart qualification" : focusedFrogCheck
          ? "software WebGL frog anatomy and separated-time frames; not continuous hop qualification" : focusedRodentCheck
          ? "software WebGL mouse and rat anatomy and separated-time frames; CPU tests own scurry/hide qualification" : focusedFlyCheck
          ? "software WebGL close fly anatomy and separated-time frames; CPU tests own hover/orbit/dart qualification" : focusedAntCheck
          ? "software WebGL close ant anatomy and trail context; CPU tests own walking, carrying and trunk-climb qualification" : focusedCicadaCheck
          ? "software WebGL close trunk-cicada anatomy and habitat; CPU/audio tests own behavior and call qualification" : focusedBeetleCheck
          ? "software WebGL close leaf-litter beetle anatomy and habitat; CPU tests own forage/hide qualification" : focusedCrocodileCheck
          ? "software WebGL close estuary-crocodile anatomy and separated-time frames; not continuous gait qualification" : focusedFishCheck
          ? "software WebGL close schooling-fish anatomy and separated-time frames; not continuous swim qualification" : focusedStingrayCheck
          ? "software WebGL close shallow-water stingray anatomy; CPU tests own continuous undulation qualification" : focusedOctopusCheck
          ? "software WebGL close reef-octopus anatomy; CPU tests own continuous arm and mantle qualification" : focusedSharkCheck
          ? "software WebGL close offshore-shark anatomy and separated-time frames; not continuous patrol qualification" : focusedDolphinCheck
          ? "software WebGL close offshore-dolphin anatomy and separated-time frames; not continuous pod qualification" : focusedBearCheck
          ? "software WebGL close bear anatomy and separated-time frames; not continuous gait qualification" : focusedResidentCheck
          ? "software WebGL close resident anatomy and separated-time frames; not continuous gait qualification" : focusedBoarCheck
          ? "software WebGL close boar anatomy and separated-time frames; not continuous gait qualification" : focusedSnakeCheck
          ? "software WebGL ground-snake body/head and intermediate images; real-time, not deterministic native stages" : focusedFlightCheck
          ? "software WebGL vertical Tab flight, ceiling and safe return; not horizontal route qualification" : focusedClimbCheck
          ? "software WebGL staged climbing diagnostics and held summit; not continuous animation" : focusedMonkeyCheck
          ? "software WebGL monkey close view only" : focusedBirdCheck
          ? "software WebGL flying parrot and walking ground-dove views; CPU tests own continuous behavior and generated PCM" : focusedWetlandCheck
          ? "software WebGL seasonal wetland soil and vegetation only" : focusedStreamCheck
          ? "software WebGL route-following stream material and separated-time frames" : focusedMaterialsCheck
          ? "software WebGL soil, grass, dry sand and wet sand only" : focusedLitterCheck
          ? "software WebGL litter and shared-height grass only" : focusedTerrainCheck
          ? "software WebGL terrain distance and close views only" : focusedBushCheck
          ? "software WebGL close shrub silhouette only" : focusedSkyCheck
          ? "software WebGL sky silhouette and lighting only" : focusedCliffCheck
          ? "software WebGL steep terrain material only" : focusedGrassMotionCheck
          ? "software measured grass traversal only" : focusedVegetationCheck
          ? "software WebGL grass and vegetation only" : focusedPalmCheck
          ? "software WebGL close palm crown only" : focusedMangroveCheck
          ? "software WebGL close mangrove roots, trunk and crown only" : focusedCoastCheck
          ? "software WebGL coast only" : focusedShipwreckCheck
          ? "software WebGL shipwreck material and separated-time frames" : focusedGroundingCheck
            ? "software WebGL prop grounding only" : "software WebGL wildlife/foliage only",
        browser: version.Browser, browserPid: browserIdentity.outerPid,
        browserNamespacePid: chrome.pid, browserRenderer, views, targetCloseTimeouts,
        shadedMonkeyPathObserved: views.some(view => view.monkeyShadeTransmission !== null),
        passed: views.length === focusedViews.length && views.every(view => view.passed),
        kernelSupervision: { evidence: artifactDirectory, stages: kernelChecks,
          acceptance: "see parent run.json; this report only qualifies browser assertions" } }, null, 2)}\n`);
    }
    await closeBrowserBounded();
  } else {
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
      "island-simulator: quality profile full grass 135m vegetation 100% shadows 100% wildlife 100% ocean 160x160 textures 10091644/10111860 bytes audio 6/6 sources")
    && result.output?.includes("forest-floor material loaded Content/terrain/forest-floor.png; 512x512, 10 mips, 1398100 RGBA8 bytes; interior windows")
    && result.output?.includes("leaf material loaded Content/vegetation/leaf-lamina.png; 256x256, 9 mips, 349524 RGBA8 bytes")
    && result.output?.includes("island-simulator: edge antialiasing requested 4x renderer-applied 4x")
    && /forest-floor filter (?:anisotropic, requested maximum 4x, device-clamped|trilinear, anisotropy unavailable)/.test(result.output ?? "")
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
    && result.output?.includes(
      "island-simulator: authoritative coast uses sub-metre berms, dunes, wash channels, rock steps, and tide-pool basins")
    && terrainBatchMatch !== null
    && Number(terrainBatchMatch[2]) < Number(terrainBatchMatch[1])
    && result.output?.includes(
      "island-simulator: terrain near-field microvariation and procedural interior leaf litter")
    && result.output?.includes(
      "island-simulator: terrain close sand granulation and shoreline-raked ripples")
    && result.output?.includes(
      "island-simulator: terrain sparse close sand pebbles and shell flecks")
    && result.output?.includes(
      "island-simulator: terrain fixed-sun shading from placed canopy volumes")
    && result.output?.includes(
      "island-simulator: terrain authored primary footpath material")
    && result.output?.includes(
      "island-simulator: cave narrow eroded lip grows into an irregular closed rock mantle")
    && result.output?.includes(
      "island-simulator: terrain footpath narrow irregular wear")
    && result.output?.includes(
      "island-simulator: terrain distinct authored east/west clearing materials")
    && result.output?.includes(
      "island-simulator: clearing floors reuse existing noise fields for broad patch breakup")
    && result.output?.includes(
      "island-simulator: terrain interior humus and moss reuse portable material fields")
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
      "island-simulator: ocean shared nine-wave spectrum uses filtered short-wave normals")
    && result.output?.includes(
      "island-simulator: ocean shallow attenuation uses interpolated bed depth and contact fade")
    && result.output?.includes(
      "island-simulator: coastal sand stays continuous below water with filtered seabed ripples")
    && result.output?.includes(
      "island-simulator: reef substrate separates pale lagoon, shared habitat patches, crest, sand channels, and deep drop-off")
    && result.output?.includes(
      "island-simulator: fixed sunny sky with layered horizontal high wisps, portable shader")
    && result.output?.includes(
      "island-simulator: distant scenery 5 unreachable low islets, one portable batch")
    && result.output?.includes(
      "island-simulator: distant wooded islets use rounded stepped ridges, clustered low crowns, and broader palm silhouettes")
    && result.output?.includes("island-simulator: palms 327 in 64 cells, hardware instancing")
    && result.output?.includes("palm portable wind shader with near/far mesh LOD")
    && result.output?.includes("palm near mesh segmented fuller leaflet silhouettes")
    && result.output?.includes(
      "island-simulator: palm near leaflets use staggered asymmetric silhouettes")
    && result.output?.includes(
      "island-simulator: palm crowns use dense narrow leaflets, irregular arcs, and emerging spear fronds")
    && result.output?.includes(
      "island-simulator: palm far crowns retain twelve arched ribbons and two narrow emerging spears")
    && result.output?.includes(
      "island-simulator: palm near geometry uses fixed-sun facet shading")
    && result.output?.includes("palm near root ridges ground flared trunk bases")
    && result.output?.includes("palm procedural bark scars and restrained coconut clusters")
    && result.output?.includes(
      "island-simulator: selected palm near crowns include sparse hanging dry fronds")
    && result.output?.includes(
      "island-simulator: palm per-instance frond tint reuses existing transforms")
    && result.output?.includes("island-simulator: jungle 261 canopy trees, 1737 understory plants, 569 low bushes, hardware instancing")
    && result.output?.includes("island-simulator: jungle ground layer 3087 small plants")
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
      "island-simulator: fish near/far geometry 1386/12 vertices, closed fins and CPU faceted daylight")
    && result.output?.includes(
      "island-simulator: shark near/far geometry 1896/15 vertices, closed fins and CPU faceted daylight")
    && result.output?.includes("island-simulator: monkeys 7 agents with portable GPU skeletal animation")
    && result.output?.includes(
      "island-simulator: monkey fur uses one hidden underside/rear UV seam instead of mirrored crown and dorsal folds")
    && result.output?.includes(
      "island-simulator: keyboard run/walk toggle uses a transient mode confirmation")
    && result.output?.includes(
      "island-simulator: monkeys use a horizontal lofted trunk and distance-driven quadrupedal gait")
    && result.output?.includes(
      "island-simulator: near monkeys use bone-following finger and toe silhouettes")
    && result.output?.includes(
      "island-simulator: near monkeys use an arched shoulder-rump trunk, broad blunt face, robust limbs, and compact tail")
    && result.output?.includes(
      "island-simulator: near monkeys use a lower tapered trunk, separated crown, and restrained furry tail root")
    && result.output?.includes(
      "island-simulator: near monkeys use a shallow waisted torso and shortened facial projection")
    && result.output?.includes(
      "island-simulator: monkey shoulder and hip mantles blend the trunk into all four animated limbs")
    && result.output?.includes(
      "island-simulator: near monkey tails keep a compact muscular hook and thirty-six raised fur clumps")
    && result.output?.includes(
      "island-simulator: near monkey faces use paired fur cheek ruffs around the skin mask")
    && result.output?.includes(
      "island-simulator: near monkey faces use a fitted dark nose pad and raised readable corneas")
    && result.output?.includes(
      "island-simulator: monkey face skin uses muted brown palettes and a recessed perimeter shade")
    && result.output?.includes(
      "island-simulator: calm monkeys use bounded skeletal head attention")
    && result.output?.includes("3 silhouettes, 4 palettes")
    && result.output?.includes(
      "island-simulator: terrain-conforming contact shadows in one portable near-field batch")
    && result.output?.includes("jungle portable wind shader with canopy/understory/bush/ground LOD")
    && result.output?.includes("jungle ground layers receive shared canopy shade; crowns excluded")
    && result.output?.includes("jungle leaves use smooth normals, filtered veins and restrained wax shading")
    && result.output?.includes("boars receive bilinear per-vertex canopy light in their existing batch")
    && result.output?.includes("boar RGB bristle coat and solid anatomy, two opaque stock BasicEffect draws")
    && result.output?.includes("boars use connected limb poses, stance contact and body-rooted bristles")
    && result.output?.includes("near boars use broad tapered upper legs with compact split hooves")
    && result.output?.includes("near boar limbs blend into overlapping furred shoulder and haunch masses")
    && result.output?.includes("near boar lower legs use a continuous knee-to-cannon taper without spherical joint caps")
    && result.output?.includes("boars use a lifted shoulder mantle and broad sealed nasal disc")
    && result.output?.includes("near boars retain warm shoulder, cheek, nose, and eye contrast in canopy shade")
    && result.output?.includes("boar face retains a warm nasal disc and visible amber eye catchlight in wet understory")
    && result.output?.includes("near adult boars use sharp triangular pinnae and paired two-segment curved ivory tusks")
    && result.output?.includes("boar soles fit rendered terrain independently with fixed-length limbs")
    && !result.output?.includes("boar terrain contact limited by limb reach")
    && result.output?.includes("monkeys receive footprint-averaged canopy light through 14-bone skinning")
    && result.output?.includes(
      "jungle near canopy tapered support branches and pointed leaf silhouettes")
    && result.output?.includes(
      "island-simulator: jungle near canopy uses irregular layered lower-crown edges")
    && result.output?.includes(
      "island-simulator: jungle near canopy uses irregular trunks and bent support branches")
    && result.output?.includes(
      "island-simulator: selected near canopy variants include hanging leafy lianas")
    && result.output?.includes(
      "island-simulator: selected near canopy trunks carry elevated epiphyte rosettes")
    && result.output?.includes(
      "island-simulator: jungle near canopy irregular buttress roots ground trunk bases")
    && result.output?.includes(
      "island-simulator: jungle near canopy procedural bark fissures")
    && result.output?.includes(
      "jungle near bushes with pointed leaf edge breakup")
    && result.output?.includes(
      "shrub variants retain terminal crowns and deterministic paired-leaf gaps across LOD")
    && result.output?.includes(
      "jungle near ground plants include upright and split-leaf silhouettes")
    && result.output?.includes(
      "jungle near ground plants reuse one variant for sparse fallen-leaf silhouettes")
    && result.output?.includes(
      "forest-floor litter includes two terrain-draped curved twigs in the shared near rosette mesh")
    && result.output?.includes("island-simulator: natural props 185 rocks, 14 reef outcrops, 314 submerged reef habitat instances, 11 fallen logs, 33 driftwood pieces, 35 fallen palm fronds, hardware instancing")
    && result.output?.includes(
      "island-simulator: clearing landmarks 5 rocks and 2 fallen logs, shared authored placement")
    && result.output?.includes(
      "island-simulator: cove landmark 3 driftwood pieces, shared authored shoreline profile")
    && result.output?.includes("island-simulator: beach detail 217 shell/pebble/wrack clusters, near-field culled")
    && result.output?.includes("island-simulator: prevailing-wind shelter adds 10 driftwood and 24 shell/wrack placements to the leeward bay")
    && result.output?.includes("island-simulator: dry wrack reuses one of four beach-detail variants")
    && result.output?.includes(
      "island-simulator: near-surface swimming view uses generated depth haze, surface scatter, peripheral veil, and sparse suspended motes in one draw")
    && result.output?.includes(
      "island-simulator: movement ripples and shoreline-transition bursts integrated into the portable ocean shader")
    && result.output?.includes(
      "island-simulator: ocean shoreline foam uses broken continuity mask")
    && result.output?.includes(
      "island-simulator: outer-ocean current uses progressive inward return with mild coherent circulation")
    && result.output?.includes("natural-prop portable shader with near/far LOD")
    && result.output?.includes(
      "island-simulator: near rocks use filtered mineral grain and restrained lichen patches")
    && result.output?.includes(
      "island-simulator: near low rocks use a fixed-sea-level wet material band")
    && result.output?.includes(
      "island-simulator: rocks use shared smooth normals, scale-correct daylight and filtered mineral weathering")
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
    && result.output?.includes(
      "island-simulator: shoreline advancing wash uses footprint-filtered porous foam")
    && result.output?.includes("portable instanced wind shader")
    && result.output?.includes(
      "island-simulator: grass biome tint follows coast, path, and authored clearings")
    && result.output?.includes(
      "island-simulator: grass near clusters use 42 blades for denser first-person cover")
    && result.output?.includes(
      "island-simulator: selected near grass blades use a geometric V-fold ridge")
    && result.output?.includes(
      "island-simulator: grass near blades use asymmetric edges and occasional bent tips")
    && result.output?.includes(
      "island-simulator: grass instances use area-preserving asymmetric tuft footprints")
    && result.output?.includes(
      "island-simulator: grass sparse near seed heads share blade wind and instancing")
    && result.output?.includes(
      "island-simulator: near birds use layered trailing flight-feather silhouettes")
    && result.output?.includes(
      "island-simulator: near birds use closed lofted bodies, multi-ring heads, fitted beaks, and lateral eyes")
    && result.output?.includes(
      "island-simulator: near birds use compact rounded heads, camera-facing six-facet eyes, fitted hooked beaks, solid swept wings, and split tail feathers")
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
  deploymentState.coastalWindMixEnabled = deploymentState.output?.includes(
    "island-simulator: surf and breeze gains follow bounded coastal wind exposure");
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
  deploymentState.shipwreckCreaksEnabled = deploymentState.output?.includes(
    "island-simulator: sparse positional shipwreck creaks share the stronger-wave voice");
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
    && deploymentState.coastalWindMixEnabled === true
    && deploymentState.waveAccentsEnabled === true
    && deploymentState.waterMovementEnabled === true
    && deploymentState.waterTransitionsEnabled === true
    && deploymentState.waterTransitionVariantsEnabled === true
    && deploymentState.landFootstepsEnabled === true
    && deploymentState.jungleAmbienceEnabled === true
    && deploymentState.waveVariantsEnabled === true
    && deploymentState.shipwreckCreaksEnabled === true
    && deploymentState.positionalWildlifeCallsEnabled === true
    && deploymentState.pointerLocked === true;

  // Exercise CNA StorageDevice's browser IDBFS path with a real preference change and a fresh
  // runtime in the same origin/profile. QA scene pages disable settings explicitly, so this is
  // isolated from their deterministic input assumptions while still testing the production shell.
  let settingsWriteOutput = "";
  const readDeploymentOutput = async () => (await call("Runtime.evaluate", {
    expression: "globalThis.islandDeploymentOutput ?? ''", returnByValue: true,
  }, deploymentSessionId)).result.value ?? "";
  for (let keyAttempt = 0; keyAttempt < 2; ++keyAttempt) {
    // Hold the key until the game observes it; a fixed pulse misses heavy SwiftShader frames.
    const disabledBefore = countOutput(await readDeploymentOutput(), "island-simulator: camera motion disabled");
    await call("Input.dispatchKeyEvent", { type: "keyDown", key: "b", code: "KeyB",
      windowsVirtualKeyCode: 66, nativeVirtualKeyCode: 66 }, deploymentSessionId);
    try {
      for (let hold = 0; hold < 60; ++hold) {
        await delay(250);
        if (countOutput(await readDeploymentOutput(), "island-simulator: camera motion disabled") > disabledBefore) break;
      }
    } finally {
      await call("Input.dispatchKeyEvent", { type: "keyUp", key: "b", code: "KeyB",
        windowsVirtualKeyCode: 66, nativeVirtualKeyCode: 66 }, deploymentSessionId);
      await delay(350);
    }
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
    && visualResult.output?.includes("island-simulator: palms 327 in 64 cells, hardware instancing")
    && visualResult.output?.includes("palm portable wind shader with near/far mesh LOD")
    && visualResult.output?.includes(
      "island-simulator: jungle 261 canopy trees, 1737 understory plants, 569 low bushes, hardware instancing")
    && visualResult.output?.includes("jungle portable wind shader with canopy/understory/bush/ground LOD")
    && visualResult.output?.includes("natural-prop portable shader with near/far LOD")
    && visualResult.output?.includes(
      "island-simulator: near rocks use filtered mineral grain and restrained lichen patches")
    && visualResult.output?.includes(
      "island-simulator: near low rocks use a fixed-sea-level wet material band")
    && visualResult.output?.includes(
      "island-simulator: rocks use shared smooth normals, scale-correct daylight and filtered mineral weathering")
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
      "island-simulator: grass near blades use asymmetric edges and occasional bent tips")
    && beachResult.output?.includes(
      "island-simulator: grass instances use area-preserving asymmetric tuft footprints")
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
      "island-simulator: grass culling bounds cover measured meshes, slope and full shared motion limits")
    && grassResult.output?.includes(
      "island-simulator: grass near blades form seven unequal fanned tufts with coherent wind")
    && grassResult.output?.includes(
      "island-simulator: grass progressive near taper and seed roots share the sampled blade profile")
    && grassResult.output?.includes(
      "island-simulator: grass shared rest-shape normals follow curvature and instance scale")
    && grassResult.output?.includes(
      "island-simulator: fixed sunny sky with layered horizontal high wisps, portable shader")
    && grassResult.output?.includes(
      "island-simulator: terrain near-field microvariation and procedural interior leaf litter")
    && grassResult.output?.includes(
      "island-simulator: terrain fixed-sun shading from placed canopy volumes")
    && grassResult.output?.includes(
      "island-simulator: grass player interaction integrated into the instanced vertex shader")
    && grassResult.output?.includes(
      "island-simulator: grass five-level near curvature with portable two-sided daylight shading")
    && grassResult.output?.includes(
      "island-simulator: grass near clusters use 42 blades for denser first-person cover")
    && grassResult.output?.includes(
      "island-simulator: selected near grass blades use a geometric V-fold ridge")
    && grassResult.output?.includes(
      "island-simulator: grass near blades use asymmetric edges and occasional bent tips")
    && grassResult.output?.includes(
      "island-simulator: close grass uses broadened planted bases and lower-blade curvature")
    && grassResult.output?.includes(
      "island-simulator: grass instances use area-preserving asymmetric tuft footprints")
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
    && /grass [1-9][0-9]*\/16809 patches in [1-9][0-9]* cells, [1-9][0-9]* draws, interaction 1, custom shader 1, biome tint 1, LOD near [1-9][0-9]*, middle [1-9][0-9]*, far [1-9][0-9]*, mixed cells [1-9][0-9]*/.test(
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
      "forest-floor litter includes two terrain-draped curved twigs in the shared near rosette mesh")
    && groundResult.output?.includes(
      "island-simulator: ground QA target variant 2 at player-eye detail distance")
    && /[1-9][0-9]*\/3087 ground plants \(near [1-9][0-9]*, far [0-9]+\)/.test(
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
      "forest-floor litter includes two terrain-draped curved twigs in the shared near rosette mesh")
    && litterResult.output?.includes(
      "island-simulator: ground-litter QA target variant 0 at player-eye detail distance")
    && /[1-9][0-9]*\/3087 ground plants \(near [1-9][0-9]*, far [0-9]+\)/.test(
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
      "island-simulator: palm crowns use dense narrow leaflets, irregular arcs, and emerging spear fronds")
    && palmResult.output?.includes(
      "island-simulator: palm far crowns retain twelve arched ribbons and two narrow emerging spears")
    && palmResult.output?.includes(
      "island-simulator: palm near geometry uses fixed-sun facet shading")
    && palmResult.output?.includes("palm near root ridges ground flared trunk bases")
    && palmResult.output?.includes(
      "palm procedural bark scars and restrained coconut clusters")
    && palmResult.output?.includes(
      "island-simulator: selected palm near crowns include sparse hanging dry fronds")
    && palmResult.output?.includes(
      "island-simulator: palm per-instance frond tint reuses existing transforms")
    && /palms [1-9][0-9]*\/327 \(near [1-9][0-9]*, far [0-9]+\)/.test(
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
      "island-simulator: terrain fixed-sun shading from placed canopy volumes")
    && jungleResult.output?.includes(
      "island-simulator: terrain authored primary footpath material")
    && jungleResult.output?.includes(
      "island-simulator: terrain footpath narrow irregular wear")
    && jungleResult.output?.includes("jungle portable wind shader with canopy/understory/bush/ground LOD")
    && jungleResult.output?.includes("jungle ground layers receive shared canopy shade; crowns excluded")
    && jungleResult.output?.includes(
      "island-simulator: jungle fixed-sun two-sided geometric facet lighting")
    && jungleResult.output?.includes(
      "island-simulator: jungle per-instance foliage tint reuses existing transforms")
    && jungleResult.output?.includes(
      "jungle near canopy tapered support branches and pointed leaf silhouettes")
    && jungleResult.output?.includes(
      "island-simulator: jungle near canopy uses irregular layered lower-crown edges")
    && jungleResult.output?.includes(
      "island-simulator: jungle near canopy uses irregular trunks and bent support branches")
    && jungleResult.output?.includes(
      "island-simulator: selected near canopy variants include hanging leafy lianas")
    && jungleResult.output?.includes(
      "island-simulator: selected near canopy trunks carry elevated epiphyte rosettes")
    && jungleResult.output?.includes(
      "island-simulator: jungle near canopy irregular buttress roots ground trunk bases")
    && jungleResult.output?.includes(
      "island-simulator: jungle near canopy procedural bark fissures")
    && jungleResult.output?.includes(
      "jungle near bushes with pointed leaf edge breakup")
    && jungleResult.output?.includes(
      "shrub variants retain terminal crowns and deterministic paired-leaf gaps across LOD")
    && jungleResult.output?.includes(
      "jungle near ground plants include upright and split-leaf silhouettes")
    && jungleResult.output?.includes(
      "jungle near ground plants reuse one variant for sparse fallen-leaf silhouettes")
    && jungleResult.output?.includes(
      "forest-floor litter includes two terrain-draped curved twigs in the shared near rosette mesh")
    && /jungle [1-9][0-9]*\/261 canopy \(near [1-9][0-9]*, far [0-9]+\), [0-9]+\/12 standing snags, [1-9][0-9]*\/1737 understory \(near [1-9][0-9]*, far [0-9]+\), [1-9][0-9]*\/569 bushes \(near [1-9][0-9]*, far [0-9]+\), [1-9][0-9]*\/3087 ground plants \(near [1-9][0-9]*, far [0-9]+\)/.test(
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
      "island-simulator: terrain interior humus and moss reuse portable material fields")
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
      "island-simulator: terrain interior humus and moss reuse portable material fields")
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
    && /props [0-9]+\/185 rocks, [0-9]+\/14 reef outcrops, [0-9]+\/314 reef habitat, [1-9][0-9]*\/11 logs/.test(
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
    && /props [0-9]+\/185 rocks, [0-9]+\/14 reef outcrops, [0-9]+\/314 reef habitat, [0-9]+\/11 logs, [1-9][0-9]*\/33 driftwood \(near [1-9][0-9]*, far [0-9]+\), [1-9][0-9]*\/217 shell\/pebble\/wrack clusters/.test(
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
      "island-simulator: near rocks use filtered mineral grain and restrained lichen patches")
    && reefResult.output?.includes(
      "island-simulator: near low rocks use a fixed-sea-level wet material band")
    && reefResult.output?.includes(
      "island-simulator: rocks use shared smooth normals, scale-correct daylight and filtered mineral weathering")
    && reefResult.output?.includes(
      "island-simulator: rock per-instance tint reuses existing transforms")
    && reefResult.output?.includes(
      "island-simulator: branching reef form uses dense upright staghorn stems, offset side forks, and small growing tips")
    && reefResult.output?.includes(
      "island-simulator: near staghorn coral uses bent two-stage stems and paired lower and upper forks")
    && reefResult.output?.includes(
      "island-simulator: near reef coral adds footprint-filtered polyp flecks, broad living pigment, and pale growing tips")
    && reefResult.output?.includes(
      "island-simulator: near staghorn bases carry five low encrusting coral and sponge colonies in the shared reef draw")
    && reefResult.output?.includes(
      "island-simulator: broken reef-crest foam follows the shared 62-86m habitat field without new draws")
    && reefResult.output?.includes(
      "island-simulator: reef QA camera targets peak broken crest weight")
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
      "island-simulator: shipwreck wood loaded Content/structures/shipwreck-weathered-wood.png; 256x256, 9 mips, 349524 RGBA8 bytes; mirrored interior atlas")
    && shipwreckResult.output?.includes(
      "island-simulator: shipwreck weathered wood follows per-piece board and beam UVs")
    && shipwreckResult.output?.includes(
      "island-simulator: shipwreck restrained rusted iron straps, rivets, hinges and rudder braces")
    && shipwreckResult.output?.includes(
      "island-simulator: shipwreck beach broadside has true missing plank bays with exposed structural ribs")
    && shipwreckResult.output?.includes(
      "island-simulator: shipwreck bow and stern use raked lower profiles with thick end timbers")
    && shipwreckResult.output?.includes(
      "island-simulator: shipwreck missing plank bays expose eleven-centimetre inner edges")
    && shipwreckResult.output?.includes(
      "island-simulator: shipwreck stern deckhouse uses separated broken roof planks and framed window bays")
    && shipwreckResult.output?.includes(
      "island-simulator: shipwreck beach debris uses buried ends, restrained warp, unequal settling, and five genuinely split long planks")
    && shipwreckResult.output?.includes(
      "island-simulator: shipwreck 20m broken hull, 12 debris pieces, one portable culled batch")
    && shipwreckResult.output?.includes(
      "island-simulator: shipwreck broken hull exposes ribs, partial deck, and snapped mast")
    && shipwreckResult.output?.includes(
      "island-simulator: shipwreck large-ship silhouette uses deck rails, hull openings, keel, rudder, and a broken foremast")
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
      "island-simulator: beach crabs 30 agents in deterministic sand-and-debris-constrained colonies")
    && crabResult.output?.includes(
      "island-simulator: beach crabs use rounded carapaces, volumetric jointed legs, eye stalks, and one culled batch")
    && crabResult.output?.includes(
      "island-simulator: crab QA camera frames a debris-linked burrow and its foraging colony")
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
      "island-simulator: near butterflies use closed bodies, paired antennae, rounded wing lobes, and dark margins")
    && butterflyResult.output?.includes(
      "island-simulator: butterfly QA camera frames one concrete nectar patch and its six animals")
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
      "island-simulator: near-surface swimming view uses generated depth haze, surface scatter, peripheral veil, and sparse suspended motes in one draw")
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
      "island-simulator: near-surface swimming view uses generated depth haze, surface scatter, peripheral veil, and sparse suspended motes in one draw")
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
    && distantResult.output?.includes(
      "island-simulator: distant wooded islets use rounded stepped ridges, clustered low crowns, and broader palm silhouettes")
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
    && frondResult.output?.includes(
      "island-simulator: fallen fronds use narrow folded pinnae with staggered roots and broken dry gaps")
    && /[1-9][0-9]*\/35 fallen fronds/.test(frondResult.output ?? "")
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
    && birdResult.output?.includes(
      "island-simulator: near birds use closed lofted bodies, multi-ring heads, fitted beaks, and lateral eyes")
    && birdResult.output?.includes(
      "island-simulator: near birds use compact rounded heads, camera-facing six-facet eyes, fitted hooked beaks, solid swept wings, and split tail feathers")
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
      "island-simulator: fish near/far geometry 1386/12 vertices, closed fins and CPU faceted daylight")
    && fishResult.output?.includes(
      "island-simulator: near fish use eleven-station sixteen-sided bodies, closed swept fins, and fitted highlighted eyes")
    && fishResult.output?.includes(
      "island-simulator: fish profile pass uses six-facet oval eyes, tapered fork tips, and a lower rear-swept dorsal")
    && fishResult.output?.includes(
      "island-simulator: near fish use blunt convex foreheads and rounded snouts instead of needle-point wedges")
    && fishResult.output?.includes(
      "island-simulator: near fish use restrained upper-flank iridescence and palette-matched gill creases")
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
      "island-simulator: sea turtles 5 agents in bounded habitat routes, three based in the sheltered western bay")
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
      "island-simulator: shark near/far geometry 1896/15 vertices, closed fins and CPU faceted daylight")
    && sharkResult.output?.includes(
      "island-simulator: near shark uses a fourteen-station eighteen-sided capped body, closed fins, five paired gill slits, and fitted eyes")
    && sharkResult.output?.includes(
      "island-simulator: near shark uses a smooth-lit blunt head and body, caudal peduncle and heterocercal tail")
    && sharkResult.output?.includes(
      "island-simulator: shark uses a long low spindle, reinforced caudal peduncle, and swept restrained fins")
    && sharkResult.output?.includes(
      "island-simulator: shark profile pass uses charcoal flank contrast, a lower rear-swept dorsal, and shorter pectorals")
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
    && monkeyResult.output?.includes("monkeys receive footprint-averaged canopy light through 14-bone skinning")
    && monkeyResult.output?.includes(
      "island-simulator: fixed sunny sky with layered horizontal high wisps, portable shader")
    && monkeyResult.output?.includes("island-simulator: monkeys 7 agents with portable GPU skeletal animation")
    && monkeyResult.output?.includes(
      "island-simulator: monkey fur uses one hidden underside/rear UV seam instead of mirrored crown and dorsal folds")
    && monkeyResult.output?.includes("3 silhouettes, 4 palettes")
    && monkeyResult.output?.includes(
      "island-simulator: monkeys use a horizontal lofted trunk and distance-driven quadrupedal gait")
    && monkeyResult.output?.includes(
      "island-simulator: near monkeys use bone-following finger and toe silhouettes")
    && monkeyResult.output?.includes(
      "island-simulator: near monkeys use an arched shoulder-rump trunk, broad blunt face, robust limbs, and compact tail")
    && monkeyResult.output?.includes(
      "island-simulator: near monkeys use a lower tapered trunk, separated crown, and restrained furry tail root")
    && monkeyResult.output?.includes(
      "island-simulator: near monkeys use a shallow waisted torso and shortened facial projection")
    && monkeyResult.output?.includes(
      "island-simulator: monkey shoulder and hip mantles blend the trunk into all four animated limbs")
    && monkeyResult.output?.includes(
      "island-simulator: near monkey tails keep a compact muscular hook and thirty-six raised fur clumps")
    && monkeyResult.output?.includes(
      "island-simulator: near monkey faces use paired fur cheek ruffs around the skin mask")
    && monkeyResult.output?.includes(
      "island-simulator: near monkey faces use a fitted dark nose pad and raised readable corneas")
    && monkeyResult.output?.includes(
      "island-simulator: monkey face skin uses muted brown palettes and a recessed perimeter shade")
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

  await holdKeyUntilNewOutput(visualSessionId, "F5", "F5", 116, "island-simulator: debug teleport ");
  const arrowStartLength = (await readSmokeOutput(visualSessionId)).length;
  await call("Input.dispatchKeyEvent", {
    type: "keyDown", key: "ArrowRight", code: "ArrowRight", windowsVirtualKeyCode: 39,
    nativeVirtualKeyCode: 39,
  }, visualSessionId);
  for (let attempt = 0; attempt < 20; ++attempt) {
    await delay(250);
    const samples = parseInputSamples((await readSmokeOutput(visualSessionId)).slice(arrowStartLength));
    if (samples.length > 1 && samples.at(-1).x < samples[0].x - 0.25) break;
  }
  await call("Input.dispatchKeyEvent", {
    type: "keyUp", key: "ArrowRight", code: "ArrowRight", windowsVirtualKeyCode: 39,
    nativeVirtualKeyCode: 39,
  }, visualSessionId);
  await delay(400);
  const arrowSamples = parseInputSamples(
    (await readSmokeOutput(visualSessionId)).slice(arrowStartLength));
  const arrowFirst = arrowSamples[0];
  const arrowLast = arrowSamples.at(-1);
  const rightArrowMovedRight = arrowFirst && arrowLast
    && arrowLast.x < arrowFirst.x - 0.25;

  await holdKeyUntilNewOutput(visualSessionId, "F5", "F5", 116, "island-simulator: debug teleport ");
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
  await holdKeyUntilNewOutput(visualSessionId, "F5", "F5", 116, "island-simulator: debug teleport ");
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
  await holdKeyUntilNewOutput(visualSessionId, "F5", "F5", 116, "island-simulator: debug teleport ");
  await call("Input.dispatchKeyEvent", {
    type: "keyDown", key: "s", code: "KeyS", windowsVirtualKeyCode: 83,
    nativeVirtualKeyCode: 83,
  }, visualSessionId);
  // Walking speed is simulated per clamped frame step, so a heavily loaded SwiftShader page
  // covers the beach much more slowly in wall time. Hold until the game reports water.
  for (let attempt = 0; attempt < 90; ++attempt) {
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
  // A resumed SwiftShader page can spend many seconds rebuilding browser/GPU state and its
  // first frames exceed one second. Poll the authoritative game samples for up to 30 s rather
  // than assuming a fixed resume latency.
  for (let attempt = 0; attempt < 60; ++attempt) {
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
      coastalWindMixEnabled: deploymentState.coastalWindMixEnabled === true,
      waveAccentsEnabled: deploymentState.waveAccentsEnabled === true,
      waterMovementEnabled: deploymentState.waterMovementEnabled === true,
      waterTransitionsEnabled: deploymentState.waterTransitionsEnabled === true,
      waterTransitionVariantsEnabled: deploymentState.waterTransitionVariantsEnabled === true,
      landFootstepsEnabled: deploymentState.landFootstepsEnabled === true,
      jungleAmbienceEnabled: deploymentState.jungleAmbienceEnabled === true,
      waveVariantsEnabled: deploymentState.waveVariantsEnabled === true,
      shipwreckCreaksEnabled: deploymentState.shipwreckCreaksEnabled === true,
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
  await writeFile(path.join(artifactDirectory, "island-browser-smoke.json"),
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
  }
} finally {
  browserSocket?.close();
  server.kill();
  if (chrome && chrome.exitCode === null) {
    const exited = new Promise(resolve => chrome.once("exit", resolve));
    chrome.kill();
    await Promise.race([exited, delay(2000)]);
  }
  await rm(profile, { recursive: true, force: true, maxRetries: 5, retryDelay: 100 });
  if (focusedCheck) {
    try {
      checkKernel("after-browser-exit");
    } finally {
      await writeFile(path.join(artifactDirectory, `island-browser-${focusedArtifactPrefix}-kernel.json`),
        `${JSON.stringify({ scope: "stage markers only; kernel acceptance belongs to the parent supervisor",
          evidence: process.env.ISLAND_BROWSER_EVIDENCE, stages: kernelChecks }, null, 2)}\n`);
    }
  }
}
