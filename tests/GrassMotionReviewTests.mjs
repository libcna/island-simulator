import assert from "node:assert/strict";
import { parseGrassMotionSamples, grassMotionProgress, grassMotionReleaseTarget, reviewGrassMotion } from "../tools/grass_motion_review.mjs";
import { acknowledgedInputGesture } from "../tools/browser_input_gesture.mjs";

const line = "island-simulator: input sample captured 1 relative 1 player 43 -18 camera 43 7.4 -18 free 0 yaw -1.5707963 pitch -0.085 viewport 1280 720 run 0 velocity 0 0 grass 120 80 400 cells 12 draws 17 canopy 1 1 trees 1000:2000:N,3000:4000:F shrubs 1 1 bushes -5000:6000:N,7000:-8000:F";
const start = parseGrassMotionSamples(line)[0];
assert.deepEqual(grassMotionReleaseTarget(6,12,1.5), {forward:true,lead:1.5,release:10.5});
assert.deepEqual(grassMotionReleaseTarget(6,0,1.5), {forward:false,lead:1.5,release:1.5});
assert.deepEqual(grassMotionReleaseTarget(-2.65,0,8), {forward:true,lead:1.325,release:-1.325});
assert.deepEqual(grassMotionReleaseTarget(0,6,0), {forward:true,lead:0,release:6});
assert.throws(() => grassMotionReleaseTarget(0,6,-1), /Invalid/);
assert.throws(() => grassMotionReleaseTarget(Infinity,6,1), /Invalid/);
assert.equal(start.middle, 80);
assert.equal(start.cameraY, 7.4);
assert.equal(parseGrassMotionSamples("old input sample without grass telemetry").length, 0);
assert.throws(() => parseGrassMotionSamples(line.replace("player 43", "player 1e999")), /Non-finite/);
assert.throws(() => parseGrassMotionSamples(line.replace(/ grass.*/, "")), /Malformed or stale/);
assert.throws(() => parseGrassMotionSamples(line.replace(/ canopy.*/, "")), /Malformed or stale/);
assert.throws(() => parseGrassMotionSamples(line.replace("3000:4000:F", "1000:2000:F")), /Duplicate/);
assert.throws(() => parseGrassMotionSamples(line.replace("1000:2000:N", "1000:2000:X")), /Malformed canopy/);
assert.throws(() => parseGrassMotionSamples(line.replace("canopy 1 1", "canopy 2 1")), /counts disagree/);
assert.equal(start.bushNear, 1);
assert.equal(start.bushes["-5000:6000"], "N");
assert.throws(() => parseGrassMotionSamples(line.replace(/ shrubs.*/, "")), /Malformed or stale/);
assert.throws(() => parseGrassMotionSamples(line.replace("7000:-8000:F", "-5000:6000:F")), /Duplicate/);
assert.throws(() => parseGrassMotionSamples(line.replace("7000:-8000:F", "7000:-8000:X")), /Malformed shrub/);
assert.throws(() => parseGrassMotionSamples(line.replace("shrubs 1 1", "shrubs 2 1")), /counts disagree/);
assert.throws(() => parseGrassMotionSamples(line + " unexpected"), /Malformed shrub/);
assert.deepEqual(parseGrassMotionSamples(line.replace(/ shrubs.*/, " shrubs 0 0 bushes none"))[0].bushes, {});
const at = (distance, index = distance) => ({ ...start, x: 43 - distance, near: 120 + index,
  trees: index < 9 ? { ...start.trees } : { "1000:2000": "F", "3000:4000": "N" },
  bushes: index < 9 ? { ...start.bushes } : { "-5000:6000": "F", "7000:-8000": "N" } });
assert.ok(Math.abs(grassMotionProgress(start, at(18)) - 18) < 0.00001);
const stages = [0, 6, 12, 18, 12, 6, 0].map(target => ({ sample: at(target), target }));
const samples = Array.from({ length: 19 }, (_, i) => at(i));
assert.equal(reviewGrassMotion(stages, samples).passed, true);
assert.equal(reviewGrassMotion(stages, samples).canopySwitchedTrees, 2);
assert.equal(reviewGrassMotion(stages, samples).canopyTransitions.length, 2);
assert.equal(reviewGrassMotion(stages, samples).bushSwitchedPlants, 2);
assert.deepEqual(reviewGrassMotion(stages, samples).bushTransitions, [
  { sample: 9, id: "-5000:6000", from: "N", to: "F" },
  { sample: 9, id: "7000:-8000", from: "F", to: "N" },
]);
for (const mutate of [
  s => { s.captured = 0; }, s => { s.relative = 0; }, s => { s.free = 1; },
  s => { s.z += 4; }, s => { s.yaw += 1; }, s => { s.near = s.middle = s.far = 0; },
  s => { s.draws = 0; }, s => { s.x = Infinity; },
  s => { s.canopyNear = 8; }, s => { s.trees = {}; },
  s => { s.trees = "NF"; },
  s => { s.bushNear = 8; }, s => { s.bushes = {}; }, s => { s.bushes = "NF"; },
  s => { s.bushes = {invalid:"N","7000:-8000":"F"}; },
]) {
  const altered = samples.map(s => ({ ...s }));
  mutate(altered[5]);
  assert.equal(reviewGrassMotion(stages, altered).passed, false);
}
assert.equal(reviewGrassMotion(stages.slice(0, -1), samples).passed, false);
assert.equal(reviewGrassMotion(stages, samples.slice(0, 3)).passed, false);
assert.equal(reviewGrassMotion(stages, samples.map(s => ({ ...s, near: 100 }))).passed, false);
assert.equal(reviewGrassMotion(stages, samples.map(s => ({ ...s, trees: start.trees }))).passed, false);
assert.equal(reviewGrassMotion(stages, samples.map(s => ({ ...s, bushes: start.bushes }))).passed, false);
// Visibility changes alone are not LOD switches, even if aggregate populations differ.
const visibilityOnly = samples.map((s,i) => ({ ...s, canopyNear: 1, canopyFar: 0,
  trees: { [`${i}:0`]: "N" } }));
assert.equal(reviewGrassMotion(stages, visibilityOnly).passed, false);
assert.equal(reviewGrassMotion(stages, samples.map((s,i) => ({ ...s, bushNear: 1, bushFar: 0,
  bushes: { [`${i}:0`]: "N" } }))).passed, false);
// Reappearing in a different tier after absence is not an observed adjacent-frame switch.
const interruptedVisibility = samples.map((s,i) => i === 8
  ? { ...s, bushNear: 0, bushFar: 0, bushes: {} } : s);
assert.equal(reviewGrassMotion(stages, interruptedVisibility).passed, false);
const movingCapture = stages.map(s => ({ ...s, sample: { ...s.sample, velocityX: 1 } }));
assert.equal(reviewGrassMotion(movingCapture, samples).passed, false);
const wrongReturn = stages.map(s => ({ ...s, sample: { ...s.sample } }));
wrongReturn.at(-1).sample.x -= 4;
assert.equal(reviewGrassMotion(wrongReturn, samples).passed, false);
assert.equal(reviewGrassMotion([], []).passed, false);
assert.equal(reviewGrassMotion([{ sample: {} }], samples).passed, false);
console.log("Grass motion parser and measured traversal gates passed");

// CPU-only asynchronous input transport tests: no browser, timers or graphics are used.
for (const fault of ["none", "press", "acknowledge", "inspect", "release", "acknowledge+release",
  "press+release", "acknowledge+inspect", "inspect+release"]) {
  const calls = [], evidence = {};
  const primary = new Error(fault), secondary = new Error("release failure");
  let inspections = 0, clock = 0;
  const operation = acknowledgedInputGesture({ evidence, now: () => ++clock,
    press: async () => { calls.push("press"); if (fault.includes("press")) throw primary; },
    acknowledge: async () => { calls.push("acknowledge"); if (fault.includes("acknowledge")) throw primary; return 42; },
    inspect: async () => {
      calls.push("inspect");
      if (++inspections === 2 && fault.includes("inspect")) throw new Error("inspection failure");
      return { pointerLocked: inspections === 2 };
    },
    release: async () => { calls.push("release"); if (fault.includes("release")) throw secondary; },
  });
  if (fault.includes("press") || fault.includes("acknowledge")) {
    await assert.rejects(operation, error => error === primary);
    assert.equal(evidence.failure, String(primary));
  } else if (fault.includes("release")) {
    await assert.rejects(operation, error => error === secondary);
  } else assert.equal(await operation, 42);
  assert.equal(calls.at(-1), "release");
  assert.equal(calls.filter(call => call === "release").length, 1);
  assert.equal(calls.includes("acknowledge"), !fault.includes("press"));
  assert.ok(evidence.heldMilliseconds > 0);
  if (fault.includes("inspect")) assert.match(evidence.inspectionError, /inspection failure/);
  if (fault.includes("release")) assert.equal(evidence.releaseError, String(secondary));
}
{
  const calls = [];
  await assert.rejects(acknowledgedInputGesture({ evidence: {},
    inspect: async () => { throw new Error("preflight inspection failed"); },
    press: async () => calls.push("press"), acknowledge: async () => calls.push("acknowledge"),
    release: async () => calls.push("release"),
  }), /preflight inspection failed/);
  assert.deepEqual(calls, [], "no input begins when the initial inspection fails");
}
console.log("Acknowledged mouse gesture: 10 success/failure and cleanup fixtures passed");
