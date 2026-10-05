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
| Terrain | CNA `BasicEffect`, 64 frustum-culled cells | Same scene and cell path | Visibly rendered in the captured GLES3 frame |
| Sky | Full fixed gradient/sun/cloud shader selected | Same GLSL ES 300 shader required by every harness scene | Sky-inclusive arm64 APK builds; current shader runtime remains unqualified |
| Ocean | Full portable custom shader selected | Full GLSL ES 300 shader required by the harness | Compiled into the APK; the captured camera did not prove every water effect |
| Palms | Hardware instancing, wind, near/far LOD | Same shader/instancing path required by the harness | Palms visibly rendered; draw/LOD counters not captured |
| Jungle | 281 canopy trees, 1,822 understory plants, 565 low bushes, and 2,225 forest-floor plants with instanced wind/LOD | Same GLSL ES 300 path; jungle camera proves nonzero rendering for all four layers | Ground-layer-inclusive arm64 APK builds; runtime remains unqualified |
| Natural props | 152 rocks, 11 fallen logs, and 28 driftwood pieces with instanced near/far meshes | Same GLSL ES 300 path; dedicated cameras prove nonzero log and driftwood draws | Prop-inclusive arm64 APK builds; runtime remains unqualified |
| Grass | Hardware instancing, wind, near/far clusters | Same shader/instancing path and nonzero beach population required | Dense mesh grass visibly rendered; sustained cost is unmeasured |
| Birds | 24 procedural agents, near/far meshes, CPU wing animation, one dynamic draw | Same simulation and `BasicEffect` batch; dedicated browser capture is visually inspected | Bird-inclusive arm64 APK builds; runtime remains unqualified |
| Fish | 48 agents in eight depth-bounded schools, near/far meshes, CPU tail animation, one dynamic draw | Same simulation and `BasicEffect` batch; dedicated browser capture is visually inspected | Fish-inclusive arm64 APK builds; runtime remains unqualified |
| Shark | One occasional non-hostile deep-water patrol, near/far meshes, CPU body/tail animation, one dynamic draw | Same simulation and `BasicEffect` batch; dedicated browser capture is visually inspected | Shark-inclusive arm64 APK builds; runtime remains unqualified |
| Audio | CNA opened a 44.1 kHz stereo device, plays four procedural mono loops, prepared both wave/water accent variants, and accepted a scheduled stronger-wave one-shot | First click opened the CNA mixer and prepared every current effect; deterministic shoreline traversal played a selected water-transition variant in Chromium | Complete current audio layer builds into the arm64 APK; device playback/focus remains unqualified |
| Debug UI | Runtime-generated font and SpriteBatch panel | Same panel captured in Chromium | Compiles; not enabled during the first emulator frame |
| Touch UI | Hidden until a touch is observed | Hidden until a touch is observed | Generated movement/look rings are enabled by default; post-HUD runtime capture remains open |
| Presentation | 1280 by 720 preferred window | 1280 by 720 canvas with aspect-preserving CSS fit | Fixed landscape/fullscreen request; emulator surface was 2400 by 1080 |

No deliberate material or content downgrade exists yet. That is not a promise that the current
desktop density is appropriate for all mobile GPUs; Android quality tiers will be introduced only
after representative physical-device profiling.

## Input and lifecycle differences

- Linux uses click-to-capture relative mouse input, WASD, F1, and Escape.
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

## Open qualification work

- Capture matched beach and ocean views on Android after the touch HUD change.
- Exercise simultaneous touch movement/look, swimming, cancellation, and pause/resume on a stable
  Android target.
- Run a visible hardware-accelerated Web session and another browser engine.
- Measure representative beach, jungle, hill, and waterline views on every target.
- Record any measured quality-tier divergence here before enabling it by default.
