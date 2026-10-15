# Browser input ownership — 2026-10-09

## Grass-motion pacing — 2026-10-11

The narrowed-footpath qualification run `20261011T071355.532471Z-chrome-grass-motion`
failed a fixed 15 s wait at 5.9895 m of the first 6 m leg. Retained samples show correct
focus/capture, westward motion and 4.3 m/s simulated velocity; the final output continued
past 7 m during cleanup. Slow software frames and clamped simulation steps exhausted wall
time before the required sample was observed. The wait now allows at most 120 new game
samples or 90 s, whichever is reached first. Motion, stopping tolerance and rendering checks
are unchanged. Retry `20261011T071524.062886Z-chrome-grass-motion` passes all seven movement
stages with 93 distinct recorded frames and GL error zero at every stage.

## Focused screenshot F1 follow-up

During compact-material qualification, contained Chrome run
`20261009T122907.762729Z-chrome-litter` passed litter but failed while trying to hide
the grass scene's overlay. Kernel deltas were empty. `captureVisualFrame` used
three fixed 650 ms F1 pulses, not the acknowledgement helper used elsewhere.
The failure proves the expected disable message was absent; the old harness did
not retain that page's input state, so a missed frame-polled press is plausible,
not a proven complete explanation of this particular failure.

Screenshot overlay control now holds one actual F1 key-down until the game logs
`debug overlay disabled`, with a bounded wait and unconditional key-up through
the existing tested `acknowledgedInputGesture`. It neither edits game state nor
relaxes the requirement that the overlay disappear. Before/release input, focus,
visibility and output-tail evidence is written for every attempt. Failed focused
pages retain their output/error/input state before the target is closed. No
gameplay or framework code changed. Corrected run
`20261009T123357.811353Z-chrome-litter` passes both views / 60 intervals each,
GL error zero, no context loss and empty kernel deltas. Actual disable
acknowledgements take 1,043 ms (litter) and 2,183 ms (grass); gesture JSONs retain
focused canvas state and both are released. This establishes successful new
delivery, not retroactive proof of the exact old failure. Litter image inspected.

This is an application/input investigation, **not** a cause or resolution of the AMDGPU
GPUVM incident. All runtime experiments use the monitored GPU-free software launcher.
CNA and Sharp Runtime remain read-only. No confirmed dependency defect was identified.

## Grass-motion F7 teleport follow-up — 2026-10-10

After the ground-cover and trunk pass made SwiftShader frames heavier, two contained
grass-motion runs (`20261010T131944.941204Z` and `132110.766963Z`) failed at setup: three
fixed 650 ms F7 pulses from `dispatchKeyUntilOutput` were never acknowledged, and the retained
page state held only the initial focus event. The setup now holds one actual F7 key-down through
`acknowledgedInputGesture` until the game logs the east-clearing teleport, releases
unconditionally and writes `island-browser-grass-motion-teleport-gesture.json`. The corrected run
`20261010T132225.050061Z` acknowledged after 2,859 ms, longer than any fixed pulse. As with F1,
this establishes delivery, not a proof of the exact earlier miss. No game code changed.

## Full-suite key delivery — 2026-10-10

The first full Chrome/SwiftShader suite since the forest-floor colonies (`20261010T134317Z`)
failed in lifecycle, settings persistence, input and four scene views. One cause was a stale
`jungle ground layer 2282` pin (now 3148). The rest were fixed 650 ms key pulses (`F5`
teleports, `b`, `Shift`, `ArrowRight`) that the heavier frames no longer observed. The second
run (`135232Z`) after converting `dispatchKeyUntilOutput` and the `F5`/`b`/`ArrowRight` presses
to acknowledged holds passed lifecycle and persistence but still missed the second `b` toggle:
the key-up was sent and the next key-down followed within 350 ms, before the frame-polled
game had observed the release, so there was no new press edge. Holds now wait for two further
game output lines after releasing. The south-beach swim walk is held until the game reports
water (bounded 90 s) because `PlayerMotion` clamps each step to 0.1 s, so wall-clock progress
falls with frame time; the resume wait grew to 30 s for the same reason. No game code changed.

## Reproduced defect

The production HTML shell requests pointer lock inside its trusted `mousedown` handler.
The game previously enabled CNA relative mouse mode only when a game update observed the
left button still pressed. A complete short click between updates can therefore grant
browser pointer lock without enabling game/CNA relative input. The browser lock alone is
not proof that mouse look works.

The zero-hold trusted WebDriver click in the retained Firefox run
`build/browser-software/20261009T074925.345232Z-firefox-full/` establishes the mismatch:
browser lock remains true, and the game logs `captured 0 relative 0` when Escape is pressed.
The game acknowledges Escape, but the browser remains locked throughout bounded polling.
An earlier 80 ms click run passed (`074832.290488Z-firefox-full`), with both game/CNA flags
true. Earlier 80 ms failures are also preserved (`074431`, `074551`); the latter proves
that simply waiting longer for Escape acknowledgement does not solve the discrepancy.

Read-only source inspection explains the no-op release:

- `IslandGame::UpdateCamera` formerly depended on a polled pressed button for acquisition.
- CNA `modules/input/src/Xna/Mouse.cpp::setIsRelativeMouseModeEXTProperty` forwards to the
  current platform mouse; `Sdl3Mouse::SetRelativeMode` forwards to SDL.
- Vendored SDL `src/video/SDL_video.c::SDL_SetWindowRelativeMouseMode` returns immediately
  if the requested flag already matches. Thus disabling an already-disabled SDL relative
  mode does not release a lock independently obtained by the HTML shell.

This is island-simulator mixing browser-owned acquisition and frame-polled game ownership,
not evidence that CNA violates its API contract. The earlier Chrome walking failure at
`072216.611324Z-chrome-grass-motion` lacked browser event evidence; do not retroactively
claim its exact cause is proven by the Firefox reproducer.

## Game-side correction

On Web only, query Emscripten's public pointer-lock status each update and recognize the
actual `canvas` lock, independent of whether the original button remains down. Enable CNA
relative mode when that grant is observed, retaining its existing delta flush and skipping
look on the acquisition frame. Clear relative mode when the browser loses its lock.
Escape also releases browser ownership explicitly, even if pressed before the first game
update that observes capture. A pending-release flag prevents reacquisition while the
asynchronous release is outstanding; only an observed unlocked state clears it.

Native/Android input paths and movement tuning are unchanged. Query/release errors are
reported, not ignored. This is browser platform integration inside the game, not a CNA fork.

## Test infrastructure

`browser_input_gesture.mjs` holds actual input until a bounded caller-provided acknowledgement
and always sends release, including an ambiguous press transport failure. Ten CPU fixtures
exercise success, failed press/acknowledgement/inspection/release, and preservation of the
primary exception. No fixture assigns browser or game capture state. The smoke page records
up to 128 passive input/focus/pointer-lock events and counts overflow without changing focus
or event propagation.

The Firefox headless backend cannot create WebGL2 in the GPU-free namespace. Failed preflights
`074243` and `074321` are retained; the latter records `AllowWebgl2:false`. Firefox now uses
an owned Xvfb framebuffer with automatically allocated display, no TCP listener, explicit PID
attribution and cleanup inside the existing lifetime namespace. It still has no GPU devices,
inherits the eight-CPU cap and forced llvmpipe, and must prove a software WebGL2 renderer on a
blank page before the game loads. Browser stderr, preflight, Escape gestures and final page
state are preserved even after failure. No desktop display is used or stopped.

Both browser suites retain exact deterministic population assertions, updated from the base
565/2225 populations to totals 578/2282: the current CPU world report independently counts
13 clearing shrubs and 57 clearing ground plants. Nonzero near/far visibility requirements
were not relaxed.

## Validation ledger

- Pre-fix Chrome walking with event instrumentation passes at
  `074114.669206Z-chrome-grass-motion`: host PID 3956392, 337 ms capture acknowledgement,
  18.7217 m outward, 1.0033 m return error, 94 telemetry samples, 101 distinct recorded frames.
  Two lock-change events report locked=true; no recorded lock error. This does not establish
  why the older fixed-duration Chrome gesture failed.
- Pre-fix CPU suite: all 32 non-display groups pass in 30.19 s.
- Post-fix Firefox at `075251.180341Z-firefox-full` fails an unrelated harness assumption:
  its fixed 120 ms Shift press is not observed. Preserve this failed gate. Use the same
  acknowledged-gesture helper for Shift, with actual animation-frame progression after
  release, rather than assuming a fixed pause spans game updates. Keep zero-hold mouse
  clicks and all gameplay assertions. No movement/controller code was changed for Shift.
- Post-fix Firefox full gate passes at `075434.226844Z-firefox-full`, host PID 3964106,
  llvmpipe on owned Xvfb. Both zero-hold clicks establish game/CNA relative input; both
  Escape gestures record `captured 1 relative 1` and actual browser unlock (120/102 ms
  acknowledgement). Shift toggles, footstep, first-gesture audio initialization, resize,
  actual tab hide/resume, jungle/water/wreck/crab/butterfly/turtle views all pass. Inspect
  the input PNG: rendered world and RUN MODE notice remain; no realism-quality claim.
  Live kernel log is empty and final delta is `[]`. Hardware GPUVM qualification remains
  prohibited.
- Post-fix Chrome full gate passes at `075634.365065Z-chrome-full`, host PID 3965021,
  SwiftShader. All 28 report sections marked `passed` are true, including simultaneous
  movement/look, right-arrow direction, persistent run/walk toggle, swimming transition,
  Escape release/reacquisition, resize, lifecycle and settings persistence. Live kernel log
  is empty and final delta is `[]`. Exact owned Firefox/Xvfb/Chrome PIDs are absent afterward.
- The updated Web package is 13,874,521 raw / 6,962,156 gzip bytes. No textures, meshes,
  shaders, or dependency sources changed in this input correction.
- Final serial incremental native/Web builds use shared ccache and two workers in `build` /
  `build-probe`. All 32 non-display CTest groups pass in 20.99 s, including ten containment
  fixtures and ten input-gesture fixtures. After the final comment-only cleanup/rebuild,
  all four Web artifact hashes exactly match the Chrome runtime-tested package. No Android
  runtime was repeated for this Web-specific correction. The AMD stop latch remains unchanged.

Reproduction/qualification commands (one browser at a time):

```sh
sg systemd-journal -c 'python3 tools/run_browser_software.py build-probe --node /home/robertvokac/emsdk/node/24.19.0_64bit/bin/node --browser firefox --scope full --timeout 600'
sg systemd-journal -c 'python3 tools/run_browser_software.py build-probe --node /home/robertvokac/emsdk/node/24.19.0_64bit/bin/node --browser chrome --scope grass-motion --timeout 300'
```

## Current Firefox requalification and beach-start collision — 2026-10-15

The current full-world Firefox gate initially received a trusted `KeyW` event while pointer lock
and document focus were both true, but CNA/SDL polling did not retain that held state at roughly
1–2 llvmpipe frames per second. A bounded production-page bridge now mirrors only WASD/arrow state
into the existing `FirstPersonDigitalInput`; blur and hidden-document transitions clear all four
bits. It does not synthesize motion, change speed, bypass collision or affect native/Android input.

Instrumented Wasm evidence then proved that the callback and game update both sampled forward
input with `dt=0.0166667` and wish vector `(0,1)`, but collision rejected the first step twice. The
cause was the beach firewood proxy: its centre at `(-0.62,-223.45)` lay only about 1.73 m from the
old `(0,-225)` start. The fire and wood remain on the requested beach. The player start moves four
metres laterally to `(4,-225)` inside the existing reserved room, and `CampfireTests` now requires
the spawn capsule and its first forward metre to be clear.

Contained Firefox 140/llvmpipe evidence
`build/browser-software/20261015T140330.390633Z-firefox-full/` passes the complete production gate.
The camera advances from `(4,-225)` to about `(4,-217.5)`, the first distance-scheduled footstep is
played, audio starts, both run modes toggle, both pointer-lock Escape cycles pass, resize and actual
tab hide/resume recover, and all jungle/water/wreck/crab/butterfly/turtle scenes qualify. The six
scene images were inspected; the WebGL 2 context remains live with GL error zero, live kernel log
is empty and final kernel delta is `[]`. The active native range passes 72/72 and ARM64
assemble/lint passes without emulator/KVM. This is software-renderer and cross-build evidence, not
AMD or physical-Android qualification.
