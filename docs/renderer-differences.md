# Renderer Differences

Grass surface shading shares GLSL 330/ES 300 pigment separation and filtered
longitudinal detail. Native llvmpipe, WebGL 2 Chrome/SwiftShader and Android GLES 3
KVM/SwiftShader runs pass; physical GPU qualification remains stopped. BasicEffect
retains its former unlit baked colour policy. See `grass-surface-material.md` for
the 40-byte vertex, nine attributes, image evidence and remaining limits.

Jungle root origins follow rendered terrain triangles on both custom and stock
paths. Near rosette litter additionally samples the shared RGB24 height texture
per vertex in GLSL 330/ES 300; its normals derive from the deformed surface. Grass
and jungle share one application-owned map. The BasicEffect fallback keeps litter
on the root plane and does not implement this draping. See `jungle-root-contact.md`
and `fallen-leaf-contact.md` for measured and actually validated scope.

Forest-floor recruitment uses the same renderer-independent placement and existing
ground-plant batches on all APIs. The default 199 groups / 866 companions are verified
in native llvmpipe, browser SwiftShader and Android KVM/SwiftShader logs. No new
material, texture or mesh is introduced. See `forest-floor-colonies.md` for actual
visual scope and increased instance/triangle cost, not physical-GPU qualification.

Shrubs use the same CPU-generated shoots and individual blades on all three APIs,
with retained leaf identities, complete leaf outlines and compact closed-end meshes
in both LODs. Far removes the interior rib, not the oval boundary. They reuse
the jungle leaf/wood shader and existing instance attributes. BasicEffect receives
the geometry but not custom vein/bark response. See `shrub-lod-continuity.md` for actual
runtime scope and increased triangle cost; no physical GPU qualification is implied.

Split ground leaves likewise share one connected profile and LOD outline knots
across APIs, without a new shader or texture. Their local above-origin bounds are
not exact per-vertex ground conformance. See `split-ground-leaves.md` for evidence.

Ground broadleaf variant 1 uses the same CPU-generated curved geometry, analytic-profile
normals and existing leaf material on all three APIs. Its far mesh retains four of six
source leaves rather than substituting fronds. BasicEffect receives the curved geometry but
not custom leaf shading; that pass left other ground species unchanged. Runtime scope and the extra
10,816 bytes of static vertex data are documented in `ground-broadleaf.md`.

The custom grass shader now contracts clusters over the last 16 m of the existing 135 m
Linux/Web or 112 m Android range. Shared GLSL 330/ES 300 performs this without alpha or extra
passes; CPU cell rejection is conservative for anchor distance. Mesh-tier selection is unchanged.
The BasicEffect fallback does not implement this fade or exact root reprojection. Consult
`grass-distance-continuity.md` for actual tests and additional far-instance submission cost;
source equivalence alone is not cross-platform runtime evidence.

The game now requests bounded backbuffer MSAA through CNA: 4x for Linux/Web and 2x for Android.
It logs renderer-applied samples and explicitly reports a device-limited downgrade. Current
software checks cover native readback, Web walking/resize and Android grass rendering; see
`edge-antialiasing.md`. This smooths polygon edges, not texture/shader aliasing or LOD changes,
and adds multisample color/depth storage outside the generated-texture inventory. Physical
GPU/mobile performance remains unqualified.

The floor-image integration uses the same 512-square ten-mip RGBA8 texture and four-window
mapping on all three APIs. GLSL ES 300 explicitly declares high-precision integers for its
unsigned hash. All paths rotate explicit world-space texture gradients. When CNA reports
anisotropic filtering, the floor uses its 4x AnisotropicClamp preset (device-clamped); otherwise
it retains LinearClamp/trilinear filtering. There is no mandatory extension, negative LOD bias,
geometry shader, compute shader or texture array. Native title files,
Web `.data` preloading and Android APK assets supply the same source PNG. Consult
`forest-floor-material-review.md` for current software-only validation; older hardware results
below are not post-incident qualification or evidence for this new material.

## Contract

Linux/OpenGL 3.3, Web/WebGL 2, and Android/OpenGL ES 3 are three presentations of the same game.
Game code uses CNA abstractions, portable 16-bit index buffers, and explicit GLSL 330 plus GLSL ES
300 shader variants. Geometry, tessellation, and compute shaders are outside the baseline. A
platform may reduce density or distance after measurement, but it must not remove core terrain,
ocean, vegetation, traversal, wildlife, or soundscape systems.

The broad table below is historical, from before the October 8 GPUVM incident and subsequent
visual-development passes. **It is not current AMD hardware qualification.** The affected desktop
GPU and host-GPU emulator must not be used while the incident stop latch is active. Current
software-renderer and build-only evidence is tracked in `PLAN.md`, `build-web.md` and
`build-android.md`. Consult each pass's scope: a focused scene/walking check does not automatically
renew the complete browser/input/audio matrix. Physical-device qualification remains pending.

Current Android software evidence now includes the API 35 x86_64 interactive scene, touch
sensitivity selection, same-process Home/resume, cold relaunch and an unobstructed clearing-edge
view under explicit SwiftShader GLES 3.0 at 1280x720. The owned headless emulator cannot access
host DRM/display devices. These are functional scene/selector/lifecycle checks, not physical-mobile
performance or post-incident AMD qualification; historical 2400x1080 host-GPU results below are
not automatically renewed by this narrower software run.

Canopy trunks, branches and buttress ridges now have a portable explicit wood tag in the unchanged
52-byte vertex format. Custom shaders select raw bark albedo and validated outward normals for
one-sided sunny daylight; branch/root winding is corrected. Stock fallback retains its baked
RGB, while leaf/crown lighting is unchanged. Native llvmpipe and all five focused SwiftShader
WebGL vegetation views validate this pass; see `PLAN.md` for Android execution evidence. No new
texture, instance attribute, triangle or renderer-specific API is required.
Tagged wood now uses local circular/axial bark coordinates on that same layout, including
seam-independent angular footprint filtering. Native llvmpipe, all five software WebGL
vegetation views and the isolated Android clearing-edge view execute this shared shader;
exact evidence is in `PLAN.md`. Continuous junction atlases and physical-GPU cost are not qualified.
The subsequent instance-stable noise warp, fissure interruptions and plate-tone variation also
pass these three software paths. The shared ES 300 shader explicitly uses high-precision integers
for its lattice hash and nonnegative wrapped indices. No extra vertex attribute or texture is
used; extra fragment cost remains unmeasured on hardware.
Canopy blades have deterministic spatial pitch/roll and variant-dependent proportions with
exact retained source poses at far LOD. The first posture pass used five-vertex diamonds and
passed native, WebGL and isolated Android checks. The subsequent canopy-only seven-vertex
outline trades fewer sprigs for fuller individual blades while preserving triangle counts;
bushes/lianas keep the legacy shape. Runtime evidence for this revision is tracked in `PLAN.md`.
The frozen comparison improves at one partly occluded tree, not all viewpoints: see
`canopy-lod-review.md`. Runtime culling uses remeasured mesh bounds; coarse light-bake proxies
remain unchanged and approximate. Neither pass qualifies physical GPU performance.
The far tier additionally retains the same seven primary branches as near. This is shared CPU
geometry (84 vertices/112 triangles added per far mesh), with unchanged shader formats and draws.
Four native software near/far pairs show modestly reduced transition error; current Web/GLES
regression evidence is tracked separately in `PLAN.md`. The new explicit `canopy` inspection
camera is elevated and capture-only, not a normal player viewpoint.
Trunk albedo now uses one linear normalized-height pigment field rather than alternating ring
parity, whose different near/far tessellations previously generated different colour bands.
This shared CPU colour path adds no rendering cost/format change. Native static pairs, a measured
software Web walking route and isolated GLES jungle execute it; evidence/limits are in `PLAN.md`.

Post-incident additions include boars, placed-canopy lighting, filtered leaf materials, moving
porous surf, a shared ocean spectrum, depth-dependent shallow-water tint, continuous sand and
terrain-triangle-conforming coastal contact patches. Stone now has shared smooth normals and
scale-correct daylight with refined close geometry instead of the historical facet-lighting path.
These share portable GLSL 330/ES 300 or stock effects, not hardware-specific extensions.
Terrain material overrides now retain smooth slope daylight through a separate float in the
28-byte vertex, while RGB remains prelit/alpha opaque for fallback. Substrate identity persists
at distance on both shader variants. These changes add 232,324 vertex bytes and one varying, no
draw or texture; physical Android GPU cost and native hardware qualification remain pending.

## Historical evidence (before the GPU incident)

| Area | Linux / OpenGL 3.3 | Web / WebGL 2 | Android / OpenGL ES 3 |
|---|---|---|---|
| Environment | Mesa OpenGL 4.6 Core through CNA `OPENGL33` on the development GPU | Chrome 152.0.7977.82 with SwiftShader WebGL 2 for the complete matrix; Firefox ESR 140.15 reports WebGL 2 on a Radeon backend for the narrower second-engine gate | Android Emulator 36.5.11, API 35 x86_64 AVD, KVM, gfxstream host GPU on AMD Radeon 780M; the arm64 build/lint uses the same SDK/NDK toolchain |
| Runtime state | Current `97783d1e090c` executable passes the complete private-display suite 46/46 with `OPENGL33`, all deterministic captures including the refined jungle, 20 m wreck, crab colony, butterfly habitat, and near-shore turtle, actual simultaneous movement/mouse input, persistent Shift run/walk toggling, and a three-process settings write/load/backup-recovery gate | Chrome render and controlled input pass all 27 stages, including dedicated jungle, shipwreck, crab, butterfly, and turtle views, resolved walking-to-swimming transition, sparse-fallen-leaf view, and IDBFS preference round trip; Firefox independently passes production first-frame, pointer lock/relock, responsive sizing, audio, input/lifecycle, jungle/water, shipwreck, crab, butterfly, and turtle gates on current `97783d1e090c` | Current `97783d1e090c` x86_64 APK passes the strict irregular-trunk/bent-branch jungle gate plus 20 m shipwreck, thirty-agent beach-crab, eighteen-agent butterfly, and five-agent sea-turtle GLES 3 gates in addition to the qualified audio/settings/lifecycle and other natural-detail runs. The restored arm64-only package builds, passes lint, contains only `arm64-v8a`, and verifies with signature scheme v2 |
| Terrain | Portable procedural sand/soil shader with close dry-sand granulation, soft-edged pebble/shell flecks, shoreline-raked ripples, a darkened wet edge with bounded view-dependent fixed-sun sheen, interior microvariation, sparse curved leaf litter, fixed-sun inland dappling, broad leaf-humus/cool-moss substrate breakup, a narrow noise-mottled authored footpath, and distinct leaf-rich west plus stronger sun-dry east clearing floors with broad reused-noise patch breakup; one 58,081-vertex/115,200-triangle shared mesh retains 64 cull cells and coalesces adjacent visible index ranges | Same GLSL ES 300 material/cell/detail/path/clearing/wet-sheen diagnostics required by the harness, now including the humus/moss marker plus warm-dry and reused-noise patch cues; current Chrome and Firefox jungle frames preserve the same restrained inland breakup without seams | A current fixed-jungle KVM/gfxstream run requires the humus/moss marker and its inspected clean 2400 by 1080 frame matches the desktop/Web composition; physical-mobile fragment cost remains unqualified |
| Sky | Full fixed gradient/sun shader with sparse layered horizontal high wisps selected | Same GLSL ES 300 shader required by every harness scene; browser captures show no former vertical streaks | The seven fixed captures retain the same sunny gradient, haze, and horizontal-wisp presentation |
| Distant scenery | Five fogged unreachable islets and 11 simplified palms in one 787-triangle `BasicEffect` batch | Same batch; dedicated browser capture visually matches native | Batch compiles into the arm64 APK; current runtime capture does not qualify it |
| Contact shadows | One terrain-conforming `BasicEffect` batch grounds nearby palms, canopy trees, land rocks, wood, and monkeys; static sampling and small-movement uploads are cached | Same full-distance batch; close browser prop capture reports 35 shadows and 420 triangles in one draw | Same batch and geometry with maximum distances scaled to 72%; the fixed grass profile capture reports 16 visible shadows versus 27 in the matching full-profile scene |
| Ocean/shore | Full portable 160 by 160-quad ocean with distance-faded crossing-wave colour, shallow-ridge light, short sunward crests, and view-aligned grazing reflection bands reused from the analytic slope, movement-driven player ripples, speed-scaled hysteretic entry/exit surface bursts, and an exact 2,695-vertex animated foam ribbon; broad foam and the ribbon wash use broken continuity masks rather than an uninterrupted bright band; the current relinked OpenGL 3.3 executable passes the complete water/current/reef capture set | The same 160 by 160 grid and GLSL ES 300 crossing-wave/ridge/sunward-crest/grazing-reflection diagnostics, both shaders, both foam-continuity diagnostics, and nonzero ripple/burst states in the dedicated water camera are required by the 27-stage Chrome harness; before/after current, water, and reef frames were inspected, and a clean Firefox run independently requires the current water path | The mobile profile retains the same shader, extent, shoreline ribbon, and interactions on a 128 by 128 grid: 16,641 vertices/32,768 triangles instead of 25,921/51,200. The current gate requires the mobile-profile diagnostic; physical-mobile appearance and cost remain unqualified |
| Near-surface view | Depth/current-responsive generated depth haze, upper-surface scatter, and peripheral veil; one conditional `SpriteBatch` draw while swimming | Same generated cue and draw count required by dedicated calm-water/current captures | A current-pin fixed calm-water capture visibly matches the vertical depth cue, reports intensity 0.6 and exactly one draw; the fixed offshore capture separately reports intensity 1.0, current 0.7, and drift 0.3/1.2 |
| Palms | Hardware instancing, wind, flared slender stems with smooth normals and one-sided wood daylight, low rounded near-root ridges, fuller segmented near leaflets with deterministic length/stagger/sweep/droop asymmetry, sparse incomplete hanging dry fronds on two near variants, restrained near-only two-sided fixed-sun facet shading, procedural near-trunk scars, two coconut-bearing variants, transform-derived green-only per-instance frond tint, widened-ribbon far crowns, and staggered per-instance LOD | Same shader/instancing/tint/near-lighting path; the dedicated browser capture requires the asymmetry, dry-frond, facet-shading, and tint markers, reports nonzero near/far rendering, and preserves one colour identity per transform across LOD | The current fixed-palm x86_64 KVM/gfxstream capture requires the dry-frond marker together with the earlier leaflet/facet markers and visibly preserves selected irregular brown remnants, clean varied living crowns, and brown trunks/roots/coconuts; dedicated LOD-transition and physical-mobile cost checks remain open |
| Jungle | 277 canopy trees (closed stands plus 22 emergents) with branch-supported/pointed-leaf near crowns, ring-varying lobe contours, sparse secondary drooping lower-edge leaves, restrained secondary trunk bends/girth variation, tapered two-segment bent support branches, hanging leafy lianas on two near variants, four/five short irregular near buttress roots, distance-faded procedural bark fissures, rounded far crowns, and staggered per-tree LOD; 1,822 understory plants, 565 low bushes with folded pointed near leaves, and 2,225 forest-floor plants with upright-broadleaf, folded split-leaf, and sparse near fallen-leaf detail; every tier uses per-instance range/LOD after cell rejection, transform-derived green-only colour variation, and derivative-based two-sided fixed-sun facet lighting | Same GLSL ES 300 instanced wind/LOD/facet-light/bark/tint path; Chrome jungle, split-leaf ground, and fallen-leaf cameras prove every detail diagnostic, including the layered-edge plus irregular-trunk/bent-branch markers and nonzero near/far populations, while Firefox independently requires and renders the same path | The current fixed-jungle x86_64 KVM/gfxstream capture requires the layered-edge, irregular-trunk/bent-branch, and liana markers and visibly retains the restrained near-trunk/branch structure, irregular crown skirts, sparse vertical strands, and complete four-layer bark/facet/colour/contact response; sustained physical-mobile cost remains unqualified |
| Natural props | 159 land rocks and 14 reef outcrops with stable transform-derived warm/cool stone tint across LOD, irregular near crowns, derivative fixed-sun facet shading, faded near-only strata/lichen, and a low fixed-sea-level wet band, 13 logs with bark/growth rings, restrained radial end-grain cracks, and irregular near broken-end contours, 31 driftwood pieces with softer sun-bleached grain, exact body-ring-matched end caps, and one/two/three tapered three-dimensional branches by near variant, 63 curved fallen fronds with asymmetric sweep/droop/ageing plus geometric central folds and two-sided fixed-sun response, and 265 shell/pebble/dry-wrack clusters; three pieces form a non-solid authored cove arc, while five rocks plus one log in each clearing form landmarks | Same GLSL ES 300 instanced/material path; Chrome prop, driftwood, and fallen-frond gates require their exact material/topology markers, the dedicated captures retain a restrained cracked end-grain surface, three closed cove silhouettes, plus a subtly folded 132-vertex/74-triangle frond, and the overview/reef plus independent Firefox checks retain the remaining material paths | Current fixed-prop and fixed-frond x86_64 KVM/gfxstream captures require the radial-crack and fold markers and visibly retain both details; earlier reef/driftwood captures retain tint/crown/facet/wet-band and three closed tapered cove silhouettes. Physical-GPU prop cost remains unqualified |
| Shipwreck | Exact 20.0 m by 5.2 m original broken wooden ship with exposed frames, partial deck, cabin, snapped main mast, broken foremast, bowsprit, rigging, split keel/rudder, broken rails, side openings, 12 bidirectionally scattered beachward debris pieces, and six major-hull collision proxies; one 4,628-triangle fogged/cullable draw. A prepared mipmapped weathered-wood image follows per-piece UVs; the portable shader adds procedural weathering, wetness, salt wear and bounded sheen, while tagged iron straps/rivets/hinges/braces receive rust and separate sheen over fallback fixed-sun vertex colours. The current native llvmpipe frame is inspected | The shared GLSL ES 300 source, asset preloading and strict texture/UV/iron marker gates are updated. Earlier geometry/material passed Chrome and Firefox, but the new file-backed wood and iron path still needs actual browser rendering and inspection | Android staging, memory budgets and strict texture/UV/iron marker gates are updated. Earlier geometry/material passed API 35 KVM/gfxstream GLES 3, but the new path still needs an actual translated-host run; physical-device cost remains unqualified |
| Grass | Hardware instancing, wind, curved/lit 42/10/5-blade near/middle/far clusters with adjacent-tier population transitions, a sparse near juvenile layer, six restrained seed-bearing stems, a real V-fold ridge on 14 selected near blades, narrower asymmetric close-blade edges, occasional bent tips, per-instance blade variation, biome-aware coast/path/clearing colour, speed-responsive local player bending, and a 135 m maximum range; the near mesh remains 622 vertices/496 triangles and adds no submission | Same curved, folded, asymmetric, biome-aware two-sided daylight shader/instancing path and 135 m range; the full-interaction capture requires the 42-blade density, V-fold, natural-blade, blade-variation, and seed-head diagnostics, proves nonzero near/middle/far populations and mixed cells, and its original-resolution frame is clean | Identical near/middle/far meshes and density inside a reduced 112 m maximum range. The current fixed close-grass x86_64 capture visibly retains the 42-blade cover plus narrower varied silhouettes and reports 2,222 visible clusters rather than 2,827 in the full-profile fixed view; sustained physical-mobile cost remains unqualified |
| Birds | 24 procedural agents, a 54-vertex/18-triangle layered-feather near mesh, a 12-vertex/4-triangle far silhouette, CPU wing animation, and one dynamic draw | Same simulation and `BasicEffect` batch; the dedicated browser capture requires the trailing-feather diagnostic and is visually inspected | The same 24-agent simulation and meshes use an 82% maximum draw range; no species or near detail is removed |
| Beach crabs | Thirty deterministic agents in six terrain-validated sand colonies, asynchronous pause/forage/scuttle behaviour, mild player retreat, four palettes, a 738-vertex rounded/facet-lit near model with three-segment legs and socketed eyes, 12-vertex far silhouette, and one culled dynamic draw | Same state/geometry/`BasicEffect` batch; the current focused Chrome/SwiftShader gate retains the moving colony in two different-hash frames, 60 intervals and exactly one draw | Earlier KVM/gfxstream imagery predates the current lighting/toe/eye pass. Current Android rendering and physical-device cost remain unqualified |
| Butterflies | Eighteen deterministic agents in three terrain-validated sunny habitats, asynchronous rest/flutter/nectar-hover behaviour, mild player response, five palettes, hinged patterned near wings, a far silhouette, and one culled dynamic draw | Same state/geometry/`BasicEffect` batch; dedicated Chrome and Firefox captures require a nonzero population, near detail, and exactly one draw. Firefox reports 11 visible agents, seven near and four far | The strict KVM/gfxstream scene reports 11 visible agents, seven near and four far, in one draw; the complete population and mobile wildlife range remain enabled |
| Sea turtles | Five deterministic non-hostile agents in separate 7--38 m offshore routes, authoritative seabed/surface clamps, cruise/forage/surface states, mild player separation, three palettes, a 1,962-vertex staggered-scute articulated near mesh, a 36-vertex far silhouette, and one culled dynamic draw | Same state, aquatic colour model, geometry, and `BasicEffect` batch; the current focused Chrome/SwiftShader gate retains the close subject in two different-hash frames, 60 intervals, and exactly one draw | Earlier KVM/gfxstream imagery predates the current head/scute pass. Current Android execution and physical-device cost remain unqualified |
| Fish | 48 agents in eight depth-bounded schools, 414/12-vertex seven-station/ten-facet near/far meshes, restrained depth/caustic palette response, two-sided lit single-surface fins, two-facet highlighted eyes, CPU tail animation, one dynamic draw | Same simulation, lighting, geometry, and `BasicEffect` batch; the focused Chrome/SwiftShader gate retains live fish in two different-hash frames over 60 intervals and requires the exact anatomy/inventory diagnostics | The same 48-agent simulation, lighting, and meshes use an 82% maximum draw range; current close appearance remains unqualified on Android |
| Shark | One occasional non-hostile deep-water patrol, 594/15-vertex eight-station/ten-facet near/far meshes, restrained depth/caustic palette response, lit shaped single-surface fins, faceted eyes, five paired gill slits, CPU body/tail animation, one dynamic draw | Same simulation, lighting, geometry, and `BasicEffect` batch; the focused Chrome/SwiftShader gate retains the live patrol in two different-hash frames over 60 intervals and requires exact anatomy/inventory diagnostics | The same occasional patrol, animation, and meshes use an 82% maximum draw range; current close appearance remains unqualified on Android |
| Dolphins | Four peaceful bounded offshore agents with cruise/surface/dive states, 558/15-vertex near/far forms, short closed beaks, shaped lit single-surface fins, faceted eyes, mouth/blowhole cues, CPU pitch/bank/fluke animation, one dynamic draw | Same simulation, lighting, geometry, and `BasicEffect` batch; the focused Chrome/SwiftShader gate retains a live dolphin in two different-hash frames over 60 intervals and requires exact anatomy/inventory diagnostics | The same pod, animation and meshes use the mobile wildlife range policy; current Android appearance and cost remain unqualified |
| Monkeys | Seven agents, three near/far silhouettes, four palettes, generated fourteen-bone rig, four clips, local-pose crossfades, near crown/cheek fur tufts and bone-following fingers/toes, CNA `SkinnedEffect` | Same mesh-selection/GPU-skinning/crossfade path; dedicated browser capture requires both near-silhouette diagnostics and all four live states are visually inspected | The same seven-agent simulation, skinning, and near detail use an 82% maximum draw range; digit appearance, all-state transition inspection, and physical-mobile animation cost remain open |
| Audio | CNA opened a 44.1 kHz stereo device, stages one generated source at a time, plays four mean-centred procedural mono loops, modulates the water loop with the offshore undertow cue, prepares both wave/water accent and sand/soil footstep pairs, and accepted scheduled one-shots | First click opened the CNA mixer and prepared every current effect; deterministic land traversal played a distance-scheduled footstep, shoreline traversal played a selected water-transition variant, and the current camera validates the shared undertow target | The arm64 package builds/lints; the current x86_64 KVM run opened 44.1 kHz stereo AAudio, passed strict staging/DC/gain gates, played a land footstep, and started a water-transition accent during the touch shoreline crossing. The preceding same-process Home/resume cycle paused/restarted audio; audible quality and external audio-focus interruption remain open |
| Camera feel | Speed-responsive, positional-only walking/swimming motion; B opt-out; keyboard Shift toggles persistent 4.3/8.0 m/s modes with a 1.4-second confirmation | Same renderer-independent model; B disable/restore, persistent Shift toggling, and the warm-gold run confirmation are automated/visually captured in Chromium | Independent movement and relative-look gestures work, a KVM touch route enters swimming, and the top-right selector visibly cycles 0.72/1.00/1.35 look response while rejecting repeat cycling during a hold. Real-finger simultaneous feel, handed layout, and a runtime camera-motion opt-out UI remain open |
| Debug UI | Runtime-generated font/SpriteBatch panel with renderer/logical-viewport identity, authoritative seed rows, and confirmed F4 clipboard copy, plus two-draw F2 `BasicEffect` lines for traversal, the exact inward/circulating current vector, attempted/resolved movement and nine-point terrain contact/normal queries, scene cells, wildlife headings/navigation, all three bird route volumes and 24 spawn headings, six ambient-zone contours, audio candidates, and the active positional emitter. Debug builds add bounded F3 inspection flight with real Xwayland input coverage | Same panel and F2 line path; F4 uses the browser clipboard surface but reports unavailable when permission or gesture policy prevents verified readback. The first-frame gate requires the 1280 by 720 logical backbuffer and automated F2 input validation requires the movement/contact/navigation/bird-route/audio diagnostic. The production release excludes F3 ownership and update code at compile time | The F1 panel visibly reports `OPENGLES3` and `1280X720` in the fixed monkey capture; the runtime gate requires the same platform-log value. F2/F3/F4 still have no touch affordance |
| Touch UI | Hidden until a touch is observed | Hidden until a touch is observed; the finite-safe interpreter and batched three-dot sensitivity selector share the same compiled path | Premultiplied generated movement/look rings and sensitivity selector are enabled by default and visibly translucent; KVM gates exercise high selection through the aspect-fit viewport, preserve it across hot resume and an app-private cold-process settings reload, select hysteretic running, play a footstep, and cross into swimming, while physical-device feel remains open |
| Presentation | 1280 by 720 preferred window/logical backbuffer | 1280 by 720 logical backbuffer and canvas with aspect-preserving CSS fit | 1280 by 720 logical backbuffer scaled into a centred 1920 by 1080 region of the fixed landscape/fullscreen 2400 by 1080 surface |

Android selects one conservative mobile default through the game-owned `RuntimeQualityProfile`;
Web selects the full profile, and native Linux now selects `native-performance` after a measured
flight-frame-rate regression. The native profile disables MSAA and more strongly bounds distant
grass, palm/jungle, shadow, wildlife and ocean-grid work without reducing populations or close
models. The mobile profile reduces distance-dependent work and ocean grid density. It retains all materials, shader layers,
wildlife populations, 42-blade near grass, the 143,624-byte generated texture set, and the complete
six-source soundscape. The exact choice is logged and required by each platform smoke gate.

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
- Browser first-gesture mixer/track startup is automated in Chromium, and Firefox qualifies one
  short actual hidden/visible tab cycle with resumed animation/input. Audible hardware-browser
  quality, extended suspension, and audio-focus behaviour remain open. Android surface and AAudio
  restoration survive a same-process Home/resume cycle, and a force-stop/new-process launch
  recreates a qualified GLES 3 frame plus audio resources. External audio-focus changes, long
  suspension, OS-initiated eviction, and physical-device lifecycle coverage remain open.

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
- Exercise simultaneous real-finger movement/look, shoreline-crossing feel, cancellation,
  audio-focus changes, process death, and long pause/resume on a physical Android target; the
  deterministic force-stop/new-process path is already qualified on the KVM AVD.
- Run a visible interactive hardware-accelerated Web session; extend the qualified Firefox subset
  beyond its current first-frame, essential-input/audio, jungle, and water gates where useful.
- Measure representative beach, jungle, hill, waterline, and active-current views on every target.
- Tune the mobile distances on representative low/mid/high devices if measured frame time,
  thermals, or visual popping justify a later device-tier selector.
