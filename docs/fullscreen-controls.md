# F11 fullscreen controls

F11 is reserved for presentation, independently of the diagnostics overlay. The former
monkey debug teleport is now J (still requires F1). Android keeps its existing immersive
mode; desktop uses `GraphicsDeviceManager::ToggleFullScreen()` on a rising key edge while
active. The native diagnostic says **requested**, because the graphics API can retain its
requested flag even if a window manager refuses the change. Native X11/software state,
window restoration, background-key isolation and mouse recovery are verified below;
hardware and other desktop/window-manager combinations are not newly qualified.

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

## Native private-window-manager verification

`tools/run_fullscreen_software.py build` uses the existing kernel supervisor, private
device/PID containment, llvmpipe and headless Weston/pixman plus private Xwayland. It
starts the already available user-space Openbox with a project-owned minimal config,
not the user's desktop configuration. No package installation or host display use.
The worker refuses unsupervised launch, uses only owned windows and records native X11
properties/geometry, screenshots and full logs. CPU geometry/launch-guard regressions
extend `SoftwareCaptureTests.py` to 33 passing tests.

Retained setup failures (all terminal, no GPU fault):

- `20261011T165141.848690Z`: bare private Xwayland had no window manager to advertise
  actual EWMH fullscreen; request log alone was correctly insufficient.
- `20261011T165340.534186Z`: Openbox respected the game's non-resizable size hints and
  refused a forced 960x540 resize. The test now checks restoration of the actual original
  supported window, not an externally invented size. Arbitrary resizable windows untested.
- `20261011T165429.904236Z`: second synthetic key edge arrived without a polled release
  frame. Now wait for a fresh game input sample after each release. Kernel delta contained
  unrelated allowed audit messages, no GPU fault.

`20261011T165602.723088Z` passes two actual on/off cycles and mouse capture/release.
Final expanded `20261011T165736.522833Z` also passes focus transfer to an owned dialog,
F11 delivered only to that dialog, and focus return without a phantom fullscreen toggle.
After focus return W captures the mouse and Escape releases it. This is not a test of
losing focus *while* F11 is held, nor fullscreen while already mouse-captured.

In both successful runs, EWMH fullscreen is confirmed, held F11 does not repeat, and
the fullscreen window covers the entire private display at 1280x720. Windowed mode
restores its original 1280x720 at (561,326), while the private display returns to
1920x1080. Both final fullscreen/windowed PNGs inspected: scene retained without
new blank regions, cropping or material loss. No monkey teleport from F11. Final kernel
delta empty; stop latch unchanged. Actual physical monitor mode switching is untested.

Remaining: Web Escape/pointer-lock/focus interaction and held-key focus-loss cases.
Space flight and the visual island/wildlife backlog are separate unfinished tasks.
