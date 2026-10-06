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
| Terrain | Portable procedural sand/soil shader, 64 frustum-culled cells | Same GLSL ES 300 material/cell path required by the harness | Material-inclusive arm64 APK builds; runtime remains unqualified |
| Sky | Full fixed gradient/sun/cloud shader selected | Same GLSL ES 300 shader required by every harness scene | Sky-inclusive arm64 APK builds; current shader runtime remains unqualified |
| Distant scenery | Five fogged unreachable islets and 11 simplified palms in one 787-triangle `BasicEffect` batch | Same batch; dedicated browser capture visually matches native | Batch compiles into the arm64 APK; current runtime capture does not qualify it |
| Contact shadows | One terrain-conforming `BasicEffect` batch grounds nearby palms, canopy trees, land rocks, wood, and monkeys; static sampling and small-movement uploads are cached | Same batch; close browser prop capture reports 35 shadows and 420 triangles in one draw | Pass compiles into the arm64 APK; current runtime capture does not qualify it |
| Ocean/shore | Full portable ocean plus exact 2,695-vertex animated foam ribbon | Both GLSL ES 300 shaders required by the harness | Both layers compile into the APK; current runtime capture does not qualify them |
| Near-surface view | Depth/current-responsive generated edge veil; one conditional `SpriteBatch` draw while swimming | Same veil and draw count required by dedicated calm-water/current captures | Compiles into the APK; current runtime capture does not qualify it |
| Palms | Hardware instancing, wind, near/far LOD | Same shader/instancing path required by the harness | Palms visibly rendered; draw/LOD counters not captured |
| Jungle | 281 canopy trees, 1,822 understory plants, 565 low bushes, and 2,225 forest-floor plants with instanced wind/LOD | Same GLSL ES 300 path; jungle camera proves nonzero rendering for all four layers | Ground-layer-inclusive arm64 APK builds; runtime remains unqualified |
| Natural props | 152 land rocks, 14 emergent reef outcrops, 11 logs, 28 driftwood pieces, 63 fallen palm fronds, and 265 near-field shell/pebble clusters | Same GLSL ES 300 instanced path; dedicated cameras prove nonzero log, driftwood, reef, frond, and beach-detail draws | Prop-inclusive arm64 APK builds; runtime remains unqualified |
| Grass | Hardware instancing, wind, near/far clusters | Same shader/instancing path and nonzero beach population required | Dense mesh grass visibly rendered; sustained cost is unmeasured |
| Birds | 24 procedural agents, near/far meshes, CPU wing animation, one dynamic draw | Same simulation and `BasicEffect` batch; dedicated browser capture is visually inspected | Bird-inclusive arm64 APK builds; runtime remains unqualified |
| Fish | 48 agents in eight depth-bounded schools, near/far meshes, CPU tail animation, one dynamic draw | Same simulation and `BasicEffect` batch; dedicated browser capture is visually inspected | Fish-inclusive arm64 APK builds; runtime remains unqualified |
| Shark | One occasional non-hostile deep-water patrol, near/far meshes, CPU body/tail animation, one dynamic draw | Same simulation and `BasicEffect` batch; dedicated browser capture is visually inspected | Shark-inclusive arm64 APK builds; runtime remains unqualified |
| Monkeys | Seven agents, three near/far silhouettes, four palettes, generated fourteen-bone rig, four clips, local-pose crossfades, CNA `SkinnedEffect` | Same mesh-selection/GPU-skinning/crossfade path; dedicated browser capture and all four live states are visually inspected | Monkey-inclusive arm64 APK builds and app lint passes; runtime remains unqualified |
| Audio | CNA opened a 44.1 kHz stereo device, plays four procedural mono loops, modulates the water loop with the offshore undertow cue, prepared both wave/water accent variants, and accepted a scheduled stronger-wave one-shot | First click opened the CNA mixer and prepared every current effect; deterministic shoreline traversal played a selected water-transition variant, while the current camera validates the shared undertow target | Complete current audio layer builds into the arm64 APK; device playback/focus remains unqualified |
| Camera feel | Speed-responsive, positional-only walking/swimming motion; B opt-out | Same renderer-independent model; B disable/restore is automated in Chromium | Same model compiles; touch feel and a runtime opt-out UI remain open |
| Debug UI | Runtime-generated font/SpriteBatch panel plus F2 `BasicEffect` world lines | Same panel and F2 line path; automated F2 input validation | Compiles; F2 runtime remains unqualified |
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

Linux now has a repeatable five-view uncapped frame-time suite and an initial accelerated Radeon
780M baseline documented in `docs/performance.md`. The same C++ benchmark path compiles into Web
and Android, but no browser or device number is considered qualified until it runs on explicitly
identified hardware without SwiftShader or unaccelerated QEMU.

## Open qualification work

- Capture matched beach and ocean views on Android after the touch HUD change.
- Exercise simultaneous touch movement/look, swimming, cancellation, and pause/resume on a stable
  Android target.
- Run a visible hardware-accelerated Web session and another browser engine.
- Measure representative beach, jungle, hill, waterline, and active-current views on every target.
- Record any measured quality-tier divergence here before enabling it by default.
