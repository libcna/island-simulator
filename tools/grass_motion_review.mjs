// CPU-only parsing and review gates shared with the browser driver. Counts describe the
// previous rendered frame; they are not a same-frame GPU query. Plant IDs identify actual
// submitted instances, allowing detail switches to be distinguished from visibility changes.
function parseIdentities(encoded, near, far, label) {
  const identities = {};
  if (encoded !== "none") for (const entry of encoded.split(",")) {
    if (!/^-?\d+:-?\d+:[NF]$/.test(entry)) throw new Error(`Malformed ${label} identity`);
    const [x, z, tier] = entry.split(":");
    const id = `${x}:${z}`;
    if (Object.hasOwn(identities, id)) throw new Error(`Duplicate ${label} identity`);
    identities[id] = tier;
  }
  if (!validIdentities(identities, near, far)) {
    throw new Error(`${label} identity counts disagree with submitted populations`);
  }
  return identities;
}

function validIdentities(identities, near, far) {
  return identities && typeof identities === "object" && !Array.isArray(identities)
    && Object.entries(identities).every(([id,t]) => /^-?\d+:-?\d+$/.test(id) && (t === "N" || t === "F"))
    && Object.values(identities).filter(t => t === "N").length === near
    && Object.values(identities).filter(t => t === "F").length === far;
}

export function parseGrassMotionSamples(output) {
  const number = "([-+0-9.eE]+)";
  const pattern = new RegExp(`input sample captured (\\d) relative (\\d) player ${number} ${number}`
    + ` camera ${number} ${number} ${number} free (\\d) yaw ${number} pitch ${number}`
    + ` viewport (\\d+) (\\d+) run (\\d) velocity ${number} ${number}`
    + " grass (\\d+) (\\d+) (\\d+) cells (\\d+) draws (\\d+)"
    + " canopy (\\d+) (\\d+) trees ([^\\s]+)"
    + " shrubs (\\d+) (\\d+) bushes ([^\\r\\n]+)", "g");
  const matches = [...output.matchAll(pattern)];
  if (matches.length !== output.split("\n").filter(line => line.includes("input sample captured ")).length) {
    throw new Error("Malformed or stale grass motion telemetry");
  }
  return matches.map(match => {
    const values = [...match.slice(1, 23), ...match.slice(24, 26)].map(Number);
    if (!values.every(Number.isFinite)) throw new Error("Non-finite grass motion telemetry");
    const [captured, relative, x, z, cameraX, cameraY, cameraZ, free, yaw, pitch,
      width, height, running, velocityX, velocityZ, near, middle, far, cells, draws,
      canopyNear, canopyFar, bushNear, bushFar] = values;
    const trees = parseIdentities(match[23], canopyNear, canopyFar, "canopy");
    const bushes = parseIdentities(match[26], bushNear, bushFar, "shrub");
    return { captured, relative, x, z, cameraX, cameraY, cameraZ, free, yaw, pitch,
      width, height, running, velocityX, velocityZ, near, middle, far, cells, draws,
      canopyNear, canopyFar, trees, bushNear, bushFar, bushes };
  });
}

export function grassMotionProgress(start, sample) {
  return (sample.x - start.x) * Math.sin(start.yaw) + (sample.z - start.z) * Math.cos(start.yaw);
}

// Learn actual command/telemetry latency plus braking from the preceding stop. Never release
// before covering half the remaining distance; corrections still use real W/S player input.
export function grassMotionReleaseTarget(current, target, observedStopLead) {
  if (![current,target,observedStopLead].every(Number.isFinite) || observedStopLead < 0) {
    throw new Error("Invalid measured stopping lead");
  }
  const forward = target >= current;
  const lead = Math.min(observedStopLead,Math.abs(target-current)*0.5);
  return { forward, lead, release: target-(forward ? lead : -lead) };
}

export function reviewGrassMotion(stages, samples) {
  const errors = [];
  const fields = ["captured", "relative", "x", "z", "free", "yaw", "velocityX", "velocityZ",
    "near", "middle", "far", "cells", "draws", "canopyNear", "canopyFar", "bushNear", "bushFar"];
  const valid = sample => fields.every(field => Number.isFinite(sample?.[field]))
    && validIdentities(sample.trees, sample.canopyNear, sample.canopyFar)
    && validIdentities(sample.bushes, sample.bushNear, sample.bushFar);
  if (!samples.every(valid) || !stages.every(stage => valid(stage?.sample))) {
    return { passed: false, errors: ["Invalid telemetry fields"], progress: [] };
  }
  const expected = [0, 6, 12, 18, 12, 6, 0];
  if (stages.length !== expected.length) errors.push("Incomplete outward/return capture sequence");
  if (samples.length < 12) errors.push("Insufficient movement telemetry");
  if (!stages.length) return { passed: false, errors, progress: [] };
  const start = stages[0].sample;
  const progress = stages.map(stage => grassMotionProgress(start, stage.sample));
  for (let i = 0; i < stages.length; ++i) {
    const sample = stages[i].sample;
    if (i >= expected.length || Math.abs(progress[i] - expected[i]) > 2.5) {
      errors.push(`Capture ${i} missed its measured distance target`);
    }
    if (Math.hypot(sample.velocityX, sample.velocityZ) > 0.15) errors.push(`Capture ${i} was not stationary`);
  }
  if (Math.max(...progress) < 17) errors.push("Outward traversal was too short");
  const final = stages.at(-1).sample;
  const returnError = Math.hypot(final.x - start.x, final.z - start.z);
  if (returnError > 2.5) errors.push("Return traversal did not reach the starting region");
  for (const sample of samples) {
    if (!Object.entries(sample).filter(([key]) => key !== "trees" && key !== "bushes")
      .every(([,v]) => Number.isFinite(v))) {
      errors.push("Invalid telemetry fields");
    }
    if (!sample.captured || !sample.relative || sample.free) errors.push("Input capture/free-camera state changed");
    if (Math.abs(sample.yaw - start.yaw) > 0.015) errors.push("Camera heading changed during the review");
    const lateral = (sample.x - start.x) * Math.cos(start.yaw) - (sample.z - start.z) * Math.sin(start.yaw);
    if (Math.abs(lateral) > 1.5) errors.push("Traversal left its intended clear corridor");
    if (sample.near + sample.middle + sample.far <= 0 || sample.cells <= 0 || sample.draws <= 0) {
      errors.push("No rendered grass population during traversal");
    }
  }
  const lodStates = new Set(samples.map(s => `${s.near}/${s.middle}/${s.far}`)).size;
  if (lodStates < 4) errors.push("Traversal did not exercise enough different visible LOD populations");
  const transitions = field => {
    const result = [];
    for (let i = 1; i < samples.length; ++i) {
      for (const [id, tier] of Object.entries(samples[i][field])) {
        const previous = samples[i-1][field][id];
        if (previous && previous !== tier) result.push({ sample: i, id, from: previous, to: tier });
      }
    }
    return result;
  };
  const canopyTransitions = transitions("trees"), bushTransitions = transitions("bushes");
  if (!canopyTransitions.length) errors.push("No observed per-tree canopy LOD switches");
  if (!bushTransitions.length) errors.push("No observed per-shrub LOD switches");
  return { passed: errors.length === 0, errors: [...new Set(errors)], progress, returnError, lodStates,
    canopyTransitions, canopySwitchedTrees: new Set(canopyTransitions.map(t => t.id)).size,
    bushTransitions, bushSwitchedPlants: new Set(bushTransitions.map(t => t.id)).size };
}
