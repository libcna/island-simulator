import assert from 'node:assert/strict';
import { readFile } from 'node:fs/promises';
import vm from 'node:vm';

const source = await readFile(new URL('../tools/browser_fullscreen.js', import.meta.url), 'utf8');
const handlers = {}, reports = [];
let enters = 0, exits = 0, reject = false, finish;
const document = {
  fullscreenEnabled: true, fullscreenElement: null,
  addEventListener: (type, fn) => { handlers[type] = fn; },
  documentElement: { requestFullscreen() {
    ++enters;
    if (reject) return Promise.reject(new Error('denied'));
    return new Promise(resolve => { finish = resolve; });
  } },
  exitFullscreen() { ++exits; return Promise.resolve(); },
};
const Module = { print: line => reports.push(line) };
vm.runInNewContext(source, { Module, document, window: {
  addEventListener: (type, fn) => { handlers[type] = fn; },
}, console });
Module.preRun[0]();
const press = (extras = {}) => {
  let prevented = false;
  handlers.keydown({code:'F11', isTrusted:true, repeat:false,
    preventDefault() { prevented = true; }, ...extras});
  return prevented;
};
const release = () => handlers.keyup({code:'F11'});
const settle = () => new Promise(resolve => setImmediate(resolve));
assert.equal(press({code:'J'}), false);
Module.canvas = {};
for (const code of ['Tab', 'Space']) {
  assert.equal(press({code}), false, 'page keyboard navigation remains available');
  document.activeElement = Module.canvas;
  assert.equal(press({code}), true, 'focused game consumes flight controls');
  document.activeElement = null;
  document.pointerLockElement = Module.canvas;
  assert.equal(press({code}), true, 'locked game consumes flight controls');
  document.pointerLockElement = null;
}
press({isTrusted:false}); assert.equal(enters, 0);
assert.equal(press(), true); assert.equal(enters, 1);
press(); press({repeat:true}); assert.equal(enters, 1);
release(); press(); assert.equal(enters, 1, 'pending request cannot be duplicated');
document.fullscreenElement = document.documentElement;
handlers.fullscreenchange(); finish(); await settle();
release(); press(); await settle(); assert.equal(exits, 1);
document.fullscreenElement = null; handlers.fullscreenchange();
handlers.blur(); reject = true; press(); await settle();
assert.ok(reports.some(line => line.includes('failed: Error: denied')));
release(); document.fullscreenEnabled = false; press();
assert.equal(enters, 2);
assert.ok(reports.some(line => line.includes('unavailable:')));
assert.ok(reports.includes('island-simulator: fullscreen on'));
assert.ok(reports.includes('island-simulator: fullscreen off'));
console.log('Browser fullscreen event, repeat, pending, blur, exit and denial tests passed');
