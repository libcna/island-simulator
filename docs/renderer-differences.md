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
| Environment | Mesa OpenGL 4.6 Core through CNA `OPENGL33` on the development GPU | Chrome 152.0.7977.82 with SwiftShader WebGL 2 | Android Emulator 36.5.11, API 35 x86_64 AVD, KVM, gfxstream host GPU on AMD Radeon 780M |
| Runtime state | Repeated interactive play plus isolated GPU smoke/captures | Automated real-browser render and controlled input | Installed complete-scene render, independent movement/look gestures, and same-process background/foreground recovery |
| Terrain | Portable procedural sand/soil shader with close dry-sand granulation, soft-edged pebble/shell flecks, shoreline-raked ripples, interior microvariation, sparse curved leaf litter, fixed-sun inland dappling, a narrow noise-mottled authored footpath, and distinct east/west clearing materials; one 58,081-vertex/115,200-triangle shared mesh retains 64 cull cells and coalesces adjacent visible index ranges | Same GLSL ES 300 material/cell/detail/path/clearing diagnostics required by the harness; shared-buffer diagnostic reports 58 visible cells in three draws; dedicated dry-beach captures expose the close cues | Revised material compiles and packages through GLES 3; the prior shared-buffer overview is visually validated without seams, while close fleck appearance and physical-mobile fragment cost remain unqualified |
| Sky | Full fixed gradient/sun shader with sparse layered horizontal high wisps selected | Same GLSL ES 300 shader required by every harness scene; browser captures show no former vertical streaks | Sky-inclusive arm64 APK builds; current shader runtime remains unqualified |
| Distant scenery | Five fogged unreachable islets and 11 simplified palms in one 787-triangle `BasicEffect` batch | Same batch; dedicated browser capture visually matches native | Batch compiles into the arm64 APK; current runtime capture does not qualify it |
| Contact shadows | One terrain-conforming `BasicEffect` batch grounds nearby palms, canopy trees, land rocks, wood, and monkeys; static sampling and small-movement uploads are cached | Same batch; close browser prop capture reports 35 shadows and 420 triangles in one draw | Pass compiles into the arm64 APK; current runtime capture does not qualify it |
| Ocean/shore | Full portable ocean with distance-faded crossing-wave colour, movement-driven player ripples, and an exact 2,695-vertex animated foam ribbon | The GLSL ES 300 crossing-wave diagnostic, both shaders, and a nonzero ripple in the dedicated water camera are required by the harness | Revised ocean, foam, and ripple paths compile into the APK; current runtime capture does not qualify the new colour treatment |
| Near-surface view | Depth/current-responsive generated edge veil; one conditional `SpriteBatch` draw while swimming | Same veil and draw count required by dedicated calm-water/current captures | Compiles into the APK; current runtime capture does not qualify it |
| Palms | Hardware instancing, wind, irregular faceted near-root wedges, segmented near leaflets, procedural near-trunk scars, two restrained coconut-bearing variants, widened-ribbon far crowns, and staggered per-instance LOD | Same shader/instancing path and root diagnostic; dedicated browser capture reports nonzero near/far rendering and visually matches native | Revised palm-inclusive arm64 APK builds; current geometry/material/LOD runtime remains unqualified |
| Jungle | 281 canopy trees with branch-supported/pointed-leaf near crowns, rounded far crowns, and staggered per-tree LOD; 1,822 understory plants, 565 low bushes with folded pointed near leaves, and 2,225 forest-floor plants with upright-broadleaf and folded split-leaf near variants; every tier uses per-instance range/LOD after cell rejection and derivative-based two-sided fixed-sun facet lighting | Same GLSL ES 300 instanced wind/LOD/facet-light path; jungle and split-leaf ground cameras prove every detail diagnostic, nonzero near/far populations, and all four vegetation layers while visually matching native | Revised lit canopy/bush/ground-plant arm64 APK builds; runtime appearance and cost remain unqualified |
| Natural props | 157 land rocks and 14 reef outcrops with faded near-only strata/lichen, 12 logs with bark/growth rings, 31 driftwood pieces with softer sun-bleached grain and one three-limb forked near silhouette, 63 fallen fronds, and 265 shell/pebble clusters; three pieces form a non-solid authored cove arc, while five rocks plus one log form clearing landmarks | Same GLSL ES 300 instanced/material path; dedicated clearing cameras require the shared landmark diagnostic, while the cove camera requires all three driftwood instances, the landmark diagnostic, and a nonzero per-instance near LOD | Revised prop-inclusive arm64 APK builds and lints; cove-group appearance and cost remain runtime-unqualified |
| Grass | Hardware instancing, wind, curved/lit 34/10/5-blade near/middle/far clusters with adjacent-tier population transitions, a sparse near juvenile layer, per-instance blade variation, biome-aware coast/path/clearing colour, and speed-responsive local player bending | Same curved, biome-aware two-sided daylight shader/instancing path; the full-interaction capture requires the new blade-variation diagnostic, proves nonzero near/middle/far populations and mixed cells, and matches native | The current shader and denser near mesh compile in the arm64 APK and reached a clean KVM/gfxstream GLES 3 first frame with visible grass; close appearance and sustained physical-mobile cost remain unqualified |
| Birds | 24 procedural agents, near/far meshes, CPU wing animation, one dynamic draw | Same simulation and `BasicEffect` batch; dedicated browser capture is visually inspected | Bird-inclusive arm64 APK builds; runtime remains unqualified |
| Fish | 48 agents in eight depth-bounded schools, 51/12-vertex CPU-lit near/far meshes, single-surface fins, CPU tail animation, one dynamic draw | Same simulation, reduced geometry, and `BasicEffect` batch; dedicated browser capture is visually inspected and requires the exact mesh diagnostic | Revised fish-inclusive arm64 APK builds; runtime remains unqualified |
| Shark | One occasional non-hostile deep-water patrol, 51/15-vertex CPU-lit near/far meshes, single-surface fins, CPU body/tail animation, one dynamic draw | Same simulation, reduced geometry, and `BasicEffect` batch; dedicated browser capture is visually inspected and requires the exact mesh diagnostic | Revised shark-inclusive arm64 APK builds; runtime remains unqualified |
| Monkeys | Seven agents, three near/far silhouettes, four palettes, generated fourteen-bone rig, four clips, local-pose crossfades, CNA `SkinnedEffect` | Same mesh-selection/GPU-skinning/crossfade path; dedicated browser capture and all four live states are visually inspected | Monkey-inclusive arm64 APK builds and app lint passes; runtime remains unqualified |
| Audio | CNA opened a 44.1 kHz stereo device, plays four procedural mono loops, modulates the water loop with the offshore undertow cue, prepared both wave/water accent variants, and accepted a scheduled stronger-wave one-shot | First click opened the CNA mixer and prepared every current effect; deterministic shoreline traversal played a selected water-transition variant, while the current camera validates the shared undertow target | AAudio opened 44.1 kHz stereo and paused/restarted across a same-process Home/resume cycle; audible quality and external audio-focus interruption remain open |
| Camera feel | Speed-responsive, positional-only walking/swimming motion; B opt-out | Same renderer-independent model; B disable/restore is automated in Chromium | Independent movement and relative-look gestures work; real-finger simultaneous feel and a runtime opt-out UI remain open |
| Debug UI | Runtime-generated font/SpriteBatch panel with authoritative seed rows and confirmed F4 clipboard copy, plus two-draw F2 `BasicEffect` lines for traversal, exact attempted/resolved movement and nine-point terrain contact/normal queries, scene cells, wildlife headings/navigation, all three bird route volumes and 24 spawn headings, six ambient-zone contours, audio candidates, and the active positional emitter. Debug builds add bounded F3 inspection flight with real Xwayland input coverage | Same panel and F2 line path; F4 uses the browser clipboard surface but reports unavailable when permission or gesture policy prevents verified readback. Automated F2 input validation requires the movement/contact/navigation/bird-route/audio diagnostic. The production release excludes F3 ownership and update code at compile time | Same seed panel and F4 clipboard path; debug APKs compile the F3 keyboard path, but there is no touch affordance and runtime remains unqualified |
| Touch UI | Hidden until a touch is observed | Hidden until a touch is observed; the finite-safe interpreter shares the same compiled path | Premultiplied generated movement/look rings are enabled by default and visibly validated as translucent rather than rectangular; invalid display/contact values cancel safely in renderer-independent tests |
| Presentation | 1280 by 720 preferred window | 1280 by 720 canvas with aspect-preserving CSS fit | Fixed landscape/fullscreen 2400 by 1080 surface with a centred 1920 by 1080 game viewport |

No deliberate material or content downgrade exists yet. That is not a promise that the current
desktop density is appropriate for all mobile GPUs; Android quality tiers will be introduced only
after representative physical-device profiling.

## Input and lifecycle differences

- Linux supports click or movement-key capture; Web requires a click for browser pointer lock. Both
  use WASD or arrows, Shift run/walk toggling, B camera-motion opt-out, F1/F2 diagnostics, F4 seed
  copy, M, and Escape. Automated native and browser tests cover simultaneous movement and mouse
  look.
- Browsers require a user gesture for pointer lock. The production shell requests it on the first
  click and handles release/reacquisition and responsive resizing.
- Android uses a stable left movement pointer and right relative-look pointer. Display-size changes
  and missing/released contacts cancel safely. Fixed landscape avoids an unsupported live rotation
  transition during play. Independent movement/look gestures are validated on the accelerated AVD;
  simultaneous real-finger ergonomics remain a physical-device task.
- Browser first-gesture mixer/track startup is automated in Chromium; audible hardware-browser
  quality and genuine tab suspension/resume remain open. Android surface and AAudio restoration
  survive one same-process Home/resume cycle; external audio-focus changes and process death remain
  open.

## Performance interpretation

The current Android emulator uses KVM for CPU execution and host-GPU gfxstream rather than the
former QEMU TCG/SwiftShader path. It is suitable for functional GLES, input, lifecycle, startup,
and same-environment before/after checks. Its compositor cadence and translated host graphics do
not represent a physical mobile GPU, so shipping density, memory, thermal, power, and frame-time
decisions still require a representative device.

Linux now has a repeatable five-view uncapped frame-time suite, a 676 m moving traversal, and an
initial accelerated Radeon 780M baseline documented in `docs/performance.md`. The same C++
benchmark path compiles into Web
and Android. Emulator measurements retain their exact AVD, KVM, gfxstream, and host-GPU identity;
physical-device budgets remain separate.

## Open qualification work

- Capture matched close beach, jungle, ocean, wildlife, and swimming views on Android.
- Exercise simultaneous real-finger movement/look, swimming, cancellation, audio-focus changes,
  process death, and long pause/resume on a physical Android target.
- Run a visible hardware-accelerated Web session and another browser engine.
- Measure representative beach, jungle, hill, waterline, and active-current views on every target.
- Record any measured quality-tier divergence here before enabling it by default.
