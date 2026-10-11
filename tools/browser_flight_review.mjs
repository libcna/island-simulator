// CPU-only review of real game diagnostics; no browser or renderer is created here.
export function parseFlightSamples(output) {
  const number = '([-+0-9.eE]+)';
  const pattern = new RegExp(`flight position ${number} ${number} ${number} above-ground ${number}`, 'g');
  const samples = [...output.matchAll(pattern)].map(m => {
    const [x,y,z,height] = m.slice(1).map(Number);
    if (![x,y,z,height].every(Number.isFinite)) throw new Error('Non-finite flight telemetry');
    return {x,y,z,height};
  });
  if (samples.length !== output.split('flight position ').length-1)
    throw new Error('Malformed flight telemetry');
  return samples;
}

export function reviewFlight(output, stages) {
  const samples = parseFlightSamples(output), errors = [];
  if (output.split('flight on;').length-1 !== 1 || output.split('flight off; safe dry-land return').length-1 !== 1)
    errors.push('Expected exactly one flight activation and safe return');
  if (samples.length < 10) errors.push('Missing flight samples');
  if (samples.some(p => p.height < 1.719 || p.height > 100.001 || p.y < 1.719 || Math.hypot(p.x,p.z)>330.001))
    errors.push('Flight escaped its terrain/sea/ceiling/radius envelope');
  if (stages.map(s => s.label).join(',') !== 'activation,ceiling,descent,landed')
    errors.push('Missing or reordered flight stages');
  const [activation,ceiling,descent,landed] = stages;
  for (const stage of [activation,ceiling,descent]) {
    if (!stage?.sample || !['x','y','z','height'].every(k=>Number.isFinite(stage.sample[k]))
        || !samples.some(p=>['x','y','z','height'].every(k=>p[k]===stage.sample[k])))
      errors.push('Stage is not an observed finite flight sample');
  }
  let held=0, maximumHeld=0;
  for(const sample of samples) {
    held=sample.height>=99.9 ? held+1 : 0; maximumHeld=Math.max(maximumHeld,held);
  }
  if(maximumHeld<4) errors.push('Missing consecutive samples held at ceiling');
  if (!activation?.sample || activation.sample.height > 3 || !ceiling?.sample
      || ceiling.sample.height < 99.9 || !(ceiling.heldSamples >= 4)
      || !(ceiling.sample.y - activation.sample.y > 90)) errors.push('Ascent/held ceiling unproven');
  if (!descent?.sample || !(descent.sample.height < 60) || !(ceiling?.sample?.y-descent.sample.y > 35))
    errors.push('Descent unproven');
  if (!landed?.safeReturn || !Number.isFinite(landed.cameraY) || landed.cameraY < 1.719
      || !(landed.cameraY < (activation?.sample?.y ?? -Infinity)+2))
    errors.push('Ground return unproven');
  const afterReturn = output.split('flight off; safe dry-land return')[1];
  if (afterReturn?.includes('flight position ')) errors.push('Flight continued after return');
  return {passed:errors.length===0,errors,samples,stages,
    scope:'software vertical flight controls and return; horizontal ridges/radius are CPU tested, not traversed here'};
}
