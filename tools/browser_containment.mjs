import { existsSync, readFileSync, readlinkSync, writeFileSync } from "node:fs";
import path from "node:path";

// Fail before profile creation/server/browser launch. This is an accidental-bypass guard,
// not a hostile-process security boundary; the Python supervisor owns the namespace.
export function requireBrowserContainment(browser) {
  const environment = process.env;
  const outerPid = Number(readlinkSync("/proc/self"));
  if (environment.ISLAND_BROWSER_SUPERVISED !== browser
      || !environment.ISLAND_BROWSER_EVIDENCE || !Number.isSafeInteger(outerPid)
      || outerPid <= 0 || outerPid === process.pid
      || existsSync("/dev/dri") || existsSync("/dev/nvidia0")
      || environment.DISPLAY || environment.WAYLAND_DISPLAY || environment.XAUTHORITY
      || environment.LIBGL_ALWAYS_SOFTWARE !== "1" || environment.GALLIUM_DRIVER !== "llvmpipe"
      || environment.LP_NUM_THREADS !== "8") {
    throw new Error("Browser suite requires tools/run_browser_software.py: PID containment, private devices and forced software rendering");
  }
  writeFileSync(path.join(environment.ISLAND_BROWSER_EVIDENCE, "browser-worker.json"),
    `${JSON.stringify({ browser, outerPid, namespacePid: process.pid, command: process.argv,
      privateGpuDevices: true }, null, 2)}\n`);
}

export function recordBrowserProcess(child, browser, filename = "browser-process.json") {
  if (!["browser-process.json", "display-process.json"].includes(filename)) {
    throw new Error("Unsupported contained process evidence filename");
  }
  // The launcher is our direct child; procfs exposes outer IDs while spawn returns local IDs.
  // Do not scan or signal unrelated host processes. A vanished launcher is a startup failure.
  const workerPid = readlinkSync("/proc/self");
  const children = readFileSync(`/proc/self/task/${workerPid}/children`, "utf8").trim().split(/\s+/);
  let outerPid = null;
  for (const candidate of children.filter(value => /^\d+$/.test(value))) {
    let status;
    try { status = readFileSync(`/proc/${candidate}/status`, "utf8"); }
    catch (error) { if (error.code === "ENOENT" || error.code === "ESRCH") continue; throw error; }
    const ids = status.match(/^NSpid:\s+([\d\s]+)$/m)?.[1].trim().split(/\s+/).map(Number);
    if (ids?.at(-1) === child.pid) outerPid = Number(candidate);
  }
  if (!outerPid) throw new Error(`Cannot attribute ${browser} launcher PID ${child.pid}`);
  const identity = { browser, outerPid, namespacePid: child.pid, workerOuterPid: Number(workerPid) };
  writeFileSync(path.join(process.env.ISLAND_BROWSER_EVIDENCE, filename),
    `${JSON.stringify(identity, null, 2)}\n`);
  return identity;
}
