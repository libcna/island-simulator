# F11 fullscreen controls

F11 is reserved for presentation, independently of the diagnostics overlay. The former
monkey debug teleport is now J (still requires F1). Android keeps its existing immersive
mode; desktop uses `GraphicsDeviceManager::ToggleFullScreen()` on a rising key edge while
active. The native diagnostic says **requested**, because the graphics API can retain its
requested flag even if a window manager refuses the change. Native actual window state,
windowed-size restoration, focus loss and mouse capture still need contained input QA.

Web links `tools/browser_fullscreen.js` into the executable with `--pre-js`, so deployment
and QA share the same handler. A trusted F11 event directly requests fullscreen for the
document element or exits the currently active fullscreen element. It prevents the browser
F11 default, ignores repeats, suppresses concurrent requests, resets held state on blur,
and reports the actual `fullscreenchange` state (including external exits). Promise and
synchronous failures are logged; browser/embedding policy may forbid fullscreen. No
pointer-lock acquisition or release is synthesized by this handler.

CPU `BrowserFullscreenTests.mjs` exercises trusted versus synthetic events, held/repeated
keys, pending requests, blur, state notifications, rejected promises and disabled policy.
Native build and 38/38 non-display groups pass (71.33 s); browser supervisor regressions
15/15 pass after adding the fullscreen scope. Web Release build/package succeeds with
11,069,478 raw / 4,163,284 gzip bytes. Initial Web compilation caught a desktop-only
unused member; its declaration now has the same platform guard as its use.

`run_browser_software.py --scope fullscreen` performs two actual F11 on/off cycles,
injects repeat events while held, verifies DOM fullscreen and exact confirmation counts,
captures fullscreen and windowed images, then checks 60 rendering intervals and the
existing graphics/material gates. It uses the existing supervised SwiftShader isolation.
The 1280x720 emulated viewport is fixed: this is not evidence for arbitrary sizes, real
monitor resolution, hardware performance, Firefox, pointer-lock interaction or tab focus.

First run `20261011T164526.828216Z-chrome-fullscreen` failed: DOM state completed four
transitions, but the asynchronous final `fullscreenchange` log had not arrived when the
harness counted it. Three messages and a fullscreen screenshot are retained, kernel delta
empty. The harness now waits for both DOM state and the corresponding notification count;
it does not omit the fourth transition or weaken acceptance.

Final `20261011T164625.592602Z-chrome-fullscreen` passes: two on/off cycles, repeat
suppression, matching DOM/events, 60 intervals, GL error 0, context intact, empty final
kernel delta. Fullscreen and restored-window screenshots were actually inspected at
1280x720: rendered island retained, no new blank/cropped scene. No extra post-interval
PNG is emitted by this scope. Native window/focus/capture and broader Web interactions
remain open; the PLAN task is partial, not complete. GPU stop latch unchanged.
