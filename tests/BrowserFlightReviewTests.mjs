import assert from 'node:assert/strict';
import {parseFlightSamples,reviewFlight} from '../tools/browser_flight_review.mjs';
const point = height => ({x:0,y:height+1,z:-225,height});
const stages = [{label:'activation',sample:point(1.721)},
  {label:'ceiling',sample:point(99.999),heldSamples:4},
  {label:'descent',sample:point(55)}, {label:'landed',safeReturn:true,cameraY:2.72}];
const rows = [1.721,20,40,60,80,99.999,99.999,99.999,99.999,99.999,80,55]
  .map(h => `island-simulator: flight position 0 ${h+1} -225 above-ground ${h}\n`).join('');
const output = 'flight on;\n'+rows+'flight off; safe dry-land return\n';
assert.equal(reviewFlight(output,stages).passed,true);
assert.equal(parseFlightSamples(output).length,12);
for (const invalid of [output.replace('above-ground 20','above-ground 101'),
  output.replace('position 0 21 -225','position 400 21 -225'),
  output.replace('position 0 21 -225','position 0 -1 -225'),
  output.replace('flight off; safe dry-land return',''), output+'flight on;', output+rows])
  assert.equal(reviewFlight(invalid,stages).passed,false);
for (const malformed of ['flight position NaN 2 3 above-ground 2',
  'flight position 1e999 2 3 above-ground 2']) assert.throws(()=>parseFlightSamples(malformed));
assert.equal(reviewFlight(output,stages.slice(0,3)).passed,false);
for (const mutate of [s=>{s[1].heldSamples=0;},s=>{s[2].sample.height=99;},
  s=>{s[3].cameraY=100;},s=>{s[3].cameraY=-Infinity;},s=>{s[3].cameraY=-1;},
  s=>{s[3].cameraY=NaN;},s=>{s[0].sample.height=99;},s=>{s[1].sample.height=NaN;}]) {
  const copy=structuredClone(stages); mutate(copy);
  assert.equal(reviewFlight(output,copy).passed,false);
}
console.log('Browser flight bounds, stage order, held ceiling and return regressions passed');
