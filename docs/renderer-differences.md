# Renderer Differences

## Contract

Linux/OpenGL 3.3, Web/WebGL 2, and Android/OpenGL ES 3 are three presentations of the same game.
Game code uses CNA abstractions, portable 16-bit index buffers, and explicit GLSL 330 plus GLSL ES
300 shader variants. Geometry, tessellation, and compute shaders are outside the baseline. A
platform may reduce density or distance after measurement, but it must not remove core terrain,
ocean, vegetation, traversal, wildlife, or soundscape systems.

All evidence below uses the dependency revisions in `dependencies.lock`.

## Current evidence

| Area | Linux / OpenGL 3.3 | Web / WebGL 2 | Android / OpenGL ES 3 |
|---|---|---|---|
| Environment | Mesa OpenGL 4.6 Core through CNA `OPENGL33` on the development GPU | Chrome 152.0.7977.82 with SwiftShader WebGL 2 | API 35 x86_64 AVD with SwiftShader GLES, no KVM |
| Runtime state | Repeated interactive play plus isolated GPU smoke/captures | Automated real-browser render and controlled input | One installed first-launch render; unstable host System UI prevents full lifecycle qualification |
| Terrain | Portable procedural sand/soil shader with close interior microvariation, sparse curved leaf litter, and fixed-sun inland dappling; 64 frustum-culled cells | Same GLSL ES 300 material/cell/detail path and dapple diagnostic required by the harness | Detail-inclusive arm64 APK builds; runtime appearance and fragment cost remain unqualified |
| Sky | Full fixed gradient/sun shader with sparse layered horizontal high wisps selected | Same GLSL ES 300 shader required by every harness scene; browser captures show no former vertical streaks | Sky-inclusive arm64 APK builds; current shader runtime remains unqualified |
| Distant scenery | Five fogged unreachable islets and 11 simplified palms in one 787-triangle `BasicEffect` batch | Same batch; dedicated browser capture visually matches native | Batch compiles into the arm64 APK; current runtime capture does not qualify it |
| Contact shadows | One terrain-conforming `BasicEffect` batch grounds nearby palms, canopy trees, land rocks, wood, and monkeys; static sampling and small-movement uploads are cached | Same batch; close browser prop capture reports 35 shadows and 420 triangles in one draw | Pass compiles into the arm64 APK; current runtime capture does not qualify it |
| Ocean/shore | Full portable ocean with movement-driven player ripples plus exact 2,695-vertex animated foam ribbon | Both GLSL ES 300 shaders and a nonzero ripple in the dedicated water camera are required by the harness | Both layers and the ripple path compile into the APK; current runtime capture does not qualify them |
| Near-surface view | Depth/current-responsive generated edge veil; one conditional `SpriteBatch` draw while swimming | Same veil and draw count required by dedicated calm-water/current captures | Compiles into the APK; current runtime capture does not qualify it |
| Palms | Hardware instancing, wind, segmented near leaflets, procedural near-trunk scars, two restrained coconut-bearing variants, widened-ribbon far crowns, and staggered per-instance LOD | Same shader/instancing path; dedicated browser capture reports nonzero near/far rendering and visually matches native | Revised palm-inclusive arm64 APK builds; current geometry/material/LOD runtime remains unqualified |
| Jungle | 281 canopy trees with branch-supported/pointed-leaf near crowns, rounded far crowns, and staggered per-tree LOD; 1,822 understory plants, 565 low bushes with folded pointed near leaves, and 2,225 forest-floor plants including an upright broadleaf near variant | Same GLSL ES 300 instanced wind/LOD path; jungle and cleared ground cameras prove every detail diagnostic and all four populations while visually matching native | Revised canopy/bush/ground-plant arm64 APK builds; runtime appearance and cost remain unqualified |
| Natural props | 152 land rocks and 14 reef outcrops with faded near-only strata/lichen, 11 logs with bark/growth rings, 28 driftwood pieces with softer sun-bleached grain, 63 fallen fronds, and 265 shell/pebble clusters | Same GLSL ES 300 instanced/material path; dedicated cameras prove all three material diagnostics plus nonzero log, driftwood, reef, frond, and beach-detail draws | Revised prop-inclusive arm64 APK builds and lints; material appearance and cost remain runtime-unqualified |
| Grass | Hardware instancing, wind, five-level curved/lit near blades, reduced far clusters, and speed-responsive local player bending | Same curved, two-sided daylight shader/instancing path; dedicated full-interaction browser capture visually matches native | Shading-inclusive arm64 APK builds; runtime appearance and sustained mobile cost remain unqualified |
| Birds | 24 procedural agents, near/far meshes, CPU wing animation, one dynamic draw | Same simulation and `BasicEffect` batch; dedicated browser capture is visually inspected | Bird-inclusive arm64 APK builds; runtime remains unqualified |
| Fish | 48 agents in eight depth-bounded schools, near/far meshes, CPU tail animation, one dynamic draw | Same simulation and `BasicEffect` batch; dedicated browser capture is visually inspected | Fish-inclusive arm64 APK builds; runtime remains unqualified |
| Shark | One occasional non-hostile deep-water patrol, near/far meshes, CPU body/tail animation, one dynamic draw | Same simulation and `BasicEffect` batch; dedicated browser capture is visually inspected | Shark-inclusive arm64 APK builds; runtime remains unqualified |
| Monkeys | Seven agents, three near/far silhouettes, four palettes, generated fourteen-bone rig, four clips, local-pose crossfades, CNA `SkinnedEffect` | Same mesh-selection/GPU-skinning/crossfade path; dedicated browser capture and all four live states are visually inspected | Monkey-inclusive arm64 APK builds and app lint passes; runtime remains unqualified |
| Audio | CNA opened a 44.1 kHz stereo device, plays four procedural mono loops, modulates the water loop with the offshore undertow cue, prepared both wave/water accent variants, and accepted a scheduled stronger-wave one-shot | First click opened the CNA mixer and prepared every current effect; deterministic shoreline traversal played a selected water-transition variant, while the current camera validates the shared undertow target | Complete current audio layer builds into the arm64 APK; device playback/focus remains unqualified |
| Camera feel | Speed-responsive, positional-only walking/swimming motion; B opt-out | Same renderer-independent model; B disable/restore is automated in Chromium | Same model compiles; touch feel and a runtime opt-out UI remain open |
| Debug UI | Runtime-generated font/SpriteBatch panel plus two-draw F2 `BasicEffect` lines for traversal, exact attempted/resolved movement and nine-point terrain contact/normal queries, scene cells, wildlife headings/navigation, all three bird route volumes and 24 spawn headings, six ambient-zone contours, audio candidates, and the active positional emitter. Debug builds add bounded F3 inspection flight with real Xwayland input coverage | Same panel and F2 line path; automated F2 input validation requires the movement/contact/navigation/bird-route/audio diagnostic. The production release excludes F3 ownership and update code at compile time | Debug APKs compile the F3 keyboard path, but there is no touch affordance and runtime remains unqualified |
| Touch UI | Hidden until a touch is observed | Hidden until a touch is observed | Generated movement/look rings are enabled by default; post-HUD runtime capture remains open |
| Presentation | 1280 by 720 preferred window | 1280 by 720 canvas with aspect-preserving CSS fit | Fixed landscape/fullscreen request; emulator surface was 2400 by 1080 |

No deliberate material or content downgrade exists yet. That is not a promise that the current
desktop density is appropriate for all mobile GPUs; Android quality tiers will be introduced only
after representative physical-device profiling.

## Input and lifecycle differences

- Linux and Web use click-to-capture relative mouse input, WASD, B camera-motion opt-out, F1/F2
  diagnostics, M, and Escape.
- Browsers require a user gesture for pointer lock. The production shell requests it on the first
  click and handles release/reacquisition and responsive resizing.
- Android uses a stable left movement pointer and right relative-look pointer. Display-size changes
  and missing/released contacts cancel safely. Fixed landscape avoids an unsupported live rotation
  transition during play.
- Browser first-gesture mixer/track startup is automated in Chromium; audible hardware-browser
  quality and genuine tab suspension/resume remain open. Android audio focus and graphics resource
  restoration require a stable accelerated emulator or physical device.

## Performance interpretation

Do not compare the current Android emulator frame time with Linux or browser performance. Without
`/dev/kvm`, the AVD uses QEMU TCG for CPU execution and SwiftShader for graphics; measured frames
took roughly 0.6--1.7 seconds and the operating system itself repeatedly entered ANR states. This
only establishes that the package loads and produces GLES3 output. Mobile density, memory, thermal,
and frame-time decisions require a real device or KVM-accelerated emulator.

Linux now has a repeatable five-view uncapped frame-time suite, a 676 m moving traversal, and an
initial accelerated Radeon 780M baseline documented in `docs/performance.md`. The same C++
benchmark path compiles into Web
and Android, but no browser or device number is considered qualified until it runs on explicitly
identified hardware without SwiftShader or unaccelerated QEMU.

## Open qualification work

- Capture matched beach and ocean views on Android after the touch HUD change.
- Exercise simultaneous touch movement/look, swimming, cancellation, and pause/resume on a stable
  Android target.
- Run a visible hardware-accelerated Web session and another browser engine.
- Measure representative beach, jungle, hill, waterline, and active-current views on every target.
- Record any measured quality-tier divergence here before enabling it by default.
