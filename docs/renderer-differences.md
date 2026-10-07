# Renderer Differences

## Contract

Linux/OpenGL 3.3, Web/WebGL 2, and Android/OpenGL ES 3 are three presentations of the same game.
Game code uses CNA abstractions, portable 16-bit index buffers, and explicit GLSL 330 plus GLSL ES
300 shader variants. Geometry, tessellation, and compute shaders are outside the baseline. A
platform may reduce density or distance after measurement, but it must not remove core terrain,
ocean, vegetation, traversal, wildlife, or soundscape systems.

Current build, Web runtime, and Android overview/current-view evidence use the dependency revisions
in `dependencies.lock`. The broader six-view Android comparison matrix and lifecycle/touch
observations predate the latest CNA-only renderer-retirement refresh where explicitly stated.

## Current evidence

| Area | Linux / OpenGL 3.3 | Web / WebGL 2 | Android / OpenGL ES 3 |
|---|---|---|---|
| Environment | Mesa OpenGL 4.6 Core through CNA `OPENGL33` on the development GPU | Chrome 152.0.7977.82 with SwiftShader WebGL 2 | Android Emulator 36.5.11, API 35 x86_64 AVD, KVM, gfxstream host GPU on AMD Radeon 780M; current-pin arm64 build/lint uses the same SDK/NDK toolchain |
| Runtime state | Current `9fbe34468c69` executable links and passes a live-desktop four-frame overview smoke with `OPENGL33`, every current shader diagnostic, and a 1280 by 720 logical backbuffer; broader fixed captures predate the latest CNA-only refresh | Automated real-browser render and controlled input on current `9fbe34468c69` | Current `9fbe34468c69` overview, offshore-current, water-haze, buttress-root, and logical-viewport views render successfully; independent movement/look gestures and same-process background/foreground recovery remain evidence from preceding `fe20e3e9c` |
| Terrain | Portable procedural sand/soil shader with close dry-sand granulation, soft-edged pebble/shell flecks, shoreline-raked ripples, a darkened wet edge with bounded view-dependent fixed-sun sheen, interior microvariation, sparse curved leaf litter, fixed-sun inland dappling, a narrow noise-mottled authored footpath, a clearer warm-dry east clearing, and a leaf-rich west clearing; one 58,081-vertex/115,200-triangle shared mesh retains 64 cull cells and coalesces adjacent visible index ranges | Same GLSL ES 300 material/cell/detail/path/clearing/wet-sheen diagnostics required by the harness, including the east warm-dry cue; shared-buffer diagnostic reports 58 visible cells in three draws; dedicated dry-beach, shoreline, and clearing captures expose the close cues | Fixed beach, grass, and jungle captures visibly match the shared geometry and broad material transitions without seams; the revised east-clearing colour and wet sheen have build/lint coverage but no new Android runtime capture, and physical-mobile fragment cost remains unqualified |
| Sky | Full fixed gradient/sun shader with sparse layered horizontal high wisps selected | Same GLSL ES 300 shader required by every harness scene; browser captures show no former vertical streaks | The six fixed captures retain the same sunny gradient, haze, and horizontal-wisp presentation |
| Distant scenery | Five fogged unreachable islets and 11 simplified palms in one 787-triangle `BasicEffect` batch | Same batch; dedicated browser capture visually matches native | Batch compiles into the arm64 APK; current runtime capture does not qualify it |
| Contact shadows | One terrain-conforming `BasicEffect` batch grounds nearby palms, canopy trees, land rocks, wood, and monkeys; static sampling and small-movement uploads are cached | Same batch; close browser prop capture reports 35 shadows and 420 triangles in one draw | Pass compiles into the arm64 APK; current runtime capture does not qualify it |
| Ocean/shore | Full portable ocean with distance-faded crossing-wave colour, shallow-ridge light and short sunward crests reused from the analytic slope, movement-driven player ripples, speed-scaled hysteretic entry/exit surface bursts, and an exact 2,695-vertex animated foam ribbon; broad foam and the ribbon wash use broken continuity masks rather than an uninterrupted bright band; the current fixed water capture visibly retains the restrained crest treatment | The GLSL ES 300 crossing-wave/ridge/sunward-crest diagnostics, both shaders, both foam-continuity diagnostics, and nonzero ripple/burst states in the dedicated water camera are required by the harness and visibly inspected | A current fixed GLES3 water capture requires and visibly retains the sunward-crest path alongside the tropical colour, shoreline, ripple, and 0.6-strength broken-ring burst; physical-mobile appearance and cost remain unqualified |
| Near-surface view | Depth/current-responsive generated depth haze, upper-surface scatter, and peripheral veil; one conditional `SpriteBatch` draw while swimming | Same generated cue and draw count required by dedicated calm-water/current captures | A current-pin fixed calm-water capture visibly matches the vertical depth cue, reports intensity 0.6 and exactly one draw; the fixed offshore capture separately reports intensity 1.0, current 0.7, and drift 0.3/1.2 |
| Palms | Hardware instancing, wind, irregular faceted near-root wedges, fuller segmented near leaflets, procedural near-trunk scars, two restrained coconut-bearing variants, widened-ribbon far crowns, and staggered per-instance LOD | Same shader/instancing path and fuller-leaflet/root diagnostics; dedicated browser capture reports nonzero near/far rendering and visually matches native | Beach, jungle, water, and monkey captures show matching trunk/crown geometry and material response; dedicated LOD-transition and physical-mobile cost checks remain open |
| Jungle | 281 canopy trees with branch-supported/pointed-leaf near crowns, four/five short irregular near buttress roots, distance-faded procedural bark fissures, rounded far crowns, and staggered per-tree LOD; 1,822 understory plants, 565 low bushes with folded pointed near leaves, and 2,225 forest-floor plants with upright-broadleaf and folded split-leaf near variants; every tier uses per-instance range/LOD after cell rejection and derivative-based two-sided fixed-sun facet lighting | Same GLSL ES 300 instanced wind/LOD/facet-light/bark path; jungle and split-leaf ground cameras prove every detail diagnostic, nonzero near/far populations, and all four vegetation layers while visually matching native | The fixed monkey capture visibly matches the near buttress roots and the jungle capture matches all four vegetation layers plus restrained bark grain/facet response; sustained physical-mobile cost remains unqualified |
| Natural props | 157 land rocks and 14 reef outcrops with irregular near crowns, derivative fixed-sun facet shading, faded near-only strata/lichen, and a low fixed-sea-level wet band, 13 logs with bark/growth rings, 31 driftwood pieces with softer sun-bleached grain and one three-limb forked near silhouette, 63 fallen fronds, and 265 shell/pebble/dry-wrack clusters; three pieces form a non-solid authored cove arc, while five rocks plus one log in each clearing form landmarks | Same GLSL ES 300 instanced/material path; the WebGL reef camera requires the crown/facet and wet-band diagnostics and visually matches native, dedicated clearing cameras require the shared landmark diagnostic, and the cove camera requires all three driftwood instances, the landmark diagnostic, dry-wrack variant, and a nonzero per-instance near LOD | The current fixed reef-detail x86_64 KVM/gfxstream capture requires the crown/facet diagnostic and visibly matches the irregular silhouette, planar light, and wet-band treatment; the revised arm64 APK builds/lints, while physical-GPU cost remains unqualified |
| Grass | Hardware instancing, wind, curved/lit 34/10/5-blade near/middle/far clusters with adjacent-tier population transitions, a sparse near juvenile layer, four restrained seed-bearing stems in the near mesh, per-instance blade variation, biome-aware coast/path/clearing colour, and speed-responsive local player bending | Same curved, biome-aware two-sided daylight shader/instancing path; the full-interaction capture requires the blade-variation and seed-head diagnostics, proves nonzero near/middle/far populations and mixed cells, and its original-resolution frame matches native | The current fixed close-grass capture requires the seed-head platform diagnostic and visibly retains the curved varied blades, juvenile underlayer, sparse heads, and surrounding density; sustained physical-mobile cost remains unqualified |
| Birds | 24 procedural agents, near/far meshes, CPU wing animation, one dynamic draw | Same simulation and `BasicEffect` batch; dedicated browser capture is visually inspected | Bird-inclusive arm64 APK builds; runtime remains unqualified |
| Fish | 48 agents in eight depth-bounded schools, 51/12-vertex CPU-lit near/far meshes, restrained depth/caustic palette response, single-surface fins, CPU tail animation, one dynamic draw | Same simulation, lighting profile, reduced geometry, and `BasicEffect` batch; dedicated browser capture is visually inspected and requires both exact lighting and mesh diagnostics | Revised fish-inclusive arm64 APK builds; runtime appearance remains unqualified |
| Shark | One occasional non-hostile deep-water patrol, 51/15-vertex CPU-lit near/far meshes, restrained depth/caustic palette response, single-surface fins, CPU body/tail animation, one dynamic draw | Same simulation, lighting profile, reduced geometry, and `BasicEffect` batch; dedicated browser capture is visually inspected and requires both exact lighting and mesh diagnostics | Revised shark-inclusive arm64 APK builds; runtime appearance remains unqualified |
| Monkeys | Seven agents, three near/far silhouettes, four palettes, generated fourteen-bone rig, four clips, local-pose crossfades, near crown/cheek fur tufts and bone-following fingers/toes, CNA `SkinnedEffect` | Same mesh-selection/GPU-skinning/crossfade path; dedicated browser capture requires both near-silhouette diagnostics and all four live states are visually inspected | The preceding fixed monkey capture visibly matches the near skinned silhouette/material; the revised arm64 package builds and lints, while digit appearance, all-state transition inspection, and physical-mobile animation cost remain open |
| Audio | CNA opened a 44.1 kHz stereo device, stages one generated source at a time, plays four mean-centred procedural mono loops, modulates the water loop with the offshore undertow cue, prepares both wave/water accent and sand/soil footstep pairs, and accepted scheduled one-shots | First click opened the CNA mixer and prepared every current effect; deterministic land traversal played a distance-scheduled footstep, shoreline traversal played a selected water-transition variant, and the current camera validates the shared undertow target | The arm64 package builds/lints; the current x86_64 KVM run opened 44.1 kHz stereo AAudio, passed strict staging/DC/gain gates, then a sustained touch traversal played a land footstep. The preceding same-process Home/resume cycle paused/restarted audio; audible quality and external audio-focus interruption remain open |
| Camera feel | Speed-responsive, positional-only walking/swimming motion; B opt-out; keyboard Shift toggles persistent 4.3/8.0 m/s modes with a 1.4-second confirmation | Same renderer-independent model; B disable/restore, persistent Shift toggling, and the warm-gold run confirmation are automated/visually captured in Chromium | Independent movement and relative-look gestures work; prior external-keyboard diagnostics prove both Shift edges, while the revised notice only has build/lint coverage; real-finger simultaneous feel and a runtime opt-out UI remain open |
| Debug UI | Runtime-generated font/SpriteBatch panel with renderer/logical-viewport identity, authoritative seed rows, and confirmed F4 clipboard copy, plus two-draw F2 `BasicEffect` lines for traversal, the exact inward/circulating current vector, attempted/resolved movement and nine-point terrain contact/normal queries, scene cells, wildlife headings/navigation, all three bird route volumes and 24 spawn headings, six ambient-zone contours, audio candidates, and the active positional emitter. Debug builds add bounded F3 inspection flight with real Xwayland input coverage | Same panel and F2 line path; F4 uses the browser clipboard surface but reports unavailable when permission or gesture policy prevents verified readback. The first-frame gate requires the 1280 by 720 logical backbuffer and automated F2 input validation requires the movement/contact/navigation/bird-route/audio diagnostic. The production release excludes F3 ownership and update code at compile time | The F1 panel visibly reports `OPENGLES3` and `1280X720` in the fixed monkey capture; the runtime gate requires the same platform-log value. F2/F3/F4 still have no touch affordance |
| Touch UI | Hidden until a touch is observed | Hidden until a touch is observed; the finite-safe interpreter shares the same compiled path | Premultiplied generated movement/look rings are enabled by default and visibly validated as translucent rather than rectangular; invalid display/contact values cancel safely in renderer-independent tests; a full-travel KVM gesture visibly selected the hysteretic warm-gold run cue and moved the player, while physical-device feel remains open |
| Presentation | 1280 by 720 preferred window/logical backbuffer | 1280 by 720 logical backbuffer and canvas with aspect-preserving CSS fit | 1280 by 720 logical backbuffer scaled into a centred 1920 by 1080 region of the fixed landscape/fullscreen 2400 by 1080 surface |

No deliberate material or content downgrade exists yet. That is not a promise that the current
desktop density is appropriate for all mobile GPUs; Android quality tiers will be introduced only
after representative physical-device profiling.

## Input and lifecycle differences

- Linux supports click or movement-key capture; Web requires a click for browser pointer lock. Both
  use WASD or arrows, Shift run/walk toggling, B camera-motion opt-out, F1/F2 diagnostics, F4 seed
  copy, M, and Escape. Automated native and browser tests cover simultaneous movement and mouse
  look.
- Browsers require a user gesture for pointer lock. The production shell requests it on the first
  click. Escape only releases browser pointer lock and never exits the Web session; the automated
  path proves continued frames and clean click reacquisition after Chromium's short security
  cooldown. Responsive resizing remains covered in the same input stage.
- Android uses a stable left movement pointer and right relative-look pointer. Display-size changes
  and missing/released contacts cancel safely. Fixed landscape avoids an unsupported live rotation
  transition during play. Independent movement/look gestures are validated on the accelerated AVD;
  simultaneous real-finger ergonomics remain a physical-device task.
- The 42--72 m offshore current and its mild coast-parallel circulation are renderer-independent
  player physics. Linux, Web, and Android compile the same bounded vector field; F1 exposes its
  exact world-space drift and F2 renders that same vector rather than a renderer-specific estimate.
- Browser first-gesture mixer/track startup is automated in Chromium; audible hardware-browser
  quality and genuine tab suspension/resume remain open. Android surface and AAudio restoration
  survive one same-process Home/resume cycle; external audio-focus changes and process death remain
  open.

## Performance interpretation

The recorded Android emulator evidence uses KVM for CPU execution and host-GPU gfxstream rather
than the former QEMU TCG/SwiftShader path. It is suitable for functional GLES, input, lifecycle, startup,
and same-environment before/after checks. Its compositor cadence and translated host graphics do
not represent a physical mobile GPU, so shipping density, memory, thermal, power, and frame-time
decisions still require a representative device.

Linux now has a repeatable five-view uncapped frame-time suite, a 676 m moving traversal, and an
initial accelerated Radeon 780M baseline documented in `docs/performance.md`. The same C++
benchmark path compiles into Web
and Android. Emulator measurements retain their exact AVD, KVM, gfxstream, and host-GPU identity;
physical-device budgets remain separate.

## Open qualification work

- Extend the matched Android matrix with dedicated birds, fish, shark, swimming-veil, distant-scenery,
  contact-shadow, and LOD-transition views where those checks provide useful additional evidence.
- Exercise simultaneous real-finger movement/look, swimming, cancellation, audio-focus changes,
  process death, and long pause/resume on a physical Android target.
- Run a visible hardware-accelerated Web session and another browser engine.
- Measure representative beach, jungle, hill, waterline, and active-current views on every target.
- Record any measured quality-tier divergence here before enabling it by default.
