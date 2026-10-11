# Island hut — implementation in progress

The hut now has a **first visible runtime model and collision**, but is not visually
complete or fully qualified. Placement uses the renderer-independent `HutLayout::Find`.

## Placement contract

- A 4.2 × 3.4 m level floor, six supports and 0.55 m roof overhang.
- At most 576 deterministic candidate sites, 18–38 m inland, with the front facing
  radially toward the coast. Stable score/order chooses a relatively flat site.
- A 5 m site-clearance circle must not intersect supplied occupied discs. Existing
  palm, jungle, rock and wreck solids are included, together with conservative discs
  for understory (1.8 × scale), bushes (1.2 × scale) and ground plants (0.5 × scale).
- The exact rendered terrain height range over the whole roof's enclosing rectangle
  must be above 1.2 m and have at most 0.45 m relief. The level floor is 0.45 m above
  that range's maximum; each support bottom independently samples rendered terrain.
- Sixteen points around the approach ring must be dry, inland and locally walkable.
  This is **not** a global path-connectivity proof.
- Invalid occupancy is rejected. A fully occupied world returns no placement instead
  of forcing an overlapping landmark or modifying terrain/vegetation.

Current solid-obstacle fixture selects approximately x=2.58166, z=204.115, floorY=
2.51744, yaw=0.0126473 radians. The same search is used by the live game.
Tests cover determinism, existing-solid clearance, independent support grounding,
dense floor terrain clearance, separate posts, reserving the chosen site, blocked
world and invalid occupancy. No new texture assets are used yet.

## First runtime version and evidence

`HutRenderer` builds 6,804 triangles (20,412 vertex-color vertices), a single static
buffer/draw, actual mesh bounds for frustum rejection and a 300m distance cutoff.
The mesh has six posts grounded 8cm into the rendered surface, braces, individually
separated deck boards, slatted walls/front opening, rafters and overlapping narrow
roof bundles. Fixed-sun CPU face lighting and portable BasicEffect fog are used;
this is provisional material work, not realistic textured wood or finished thatch.
Upload validates finite vertices, nondegenerate triangles and a30,000vertex limit.

A conservative2.83m solid disc encloses the floor and outside post corners. It enters
the existing shared collision grid for players and wildlife together with the model.
The building is currently decorative/non-enterable; the opening is visual, not a
new indoor navigation system. Eight radial player approaches test contact without
penetration and confirm the player can actually reach the proxy. Collider is deliberately
coarser than individual walls/posts; no detailed interior collision is claimed.

CLI `--viewpoint hut` and the native software supervisor support the new camera.
Actual contained llvmpipe `20261011T221344.265945Z-hut/capture.png` inspected: raised
timber hut clearly visible on dry sand, floor/posts grounded, no broadleaf intersection
visible. Capture gate PASS (393 color bins, luminanceSD50.51, spatial contrast196.80),
kernel-after[]. Visual shortcomings: roof looks too regular/slat-like, untextured
wood is plain, and missing contact/roof shadows make the hut look insufficiently
connected to the ground. These are open, not waived by the capture pass.

## Terrain-conforming contact shadow

The live hut now contributes seven static patches to the existing contact-shadow batch:
a soft ellipse under the raised floor (28% maximum opacity) and six small footing
patches (34%,25cm radius). The floor ellipse follows hut yaw. All patches are clipped
to exact rendered terrain triangles with the existing25mm vertical bias, cached once,
culled/faded at90m scaled by the existing quality profile, and counted separately as
hut contacts. There is no extra texture, shader or draw call. This is ambient grounding,
not a directional roof silhouette or structural self-shadow.

CPU tests check yaw, exact footing centers, finite bounded opacity, vertex and triangle
centroid height against rendered ground, and a12,000vertex cap. Current geometry is
3,300vertices/1,100triangles. All eight player collision approaches remain covered.
Focused suite PASS0.65s. Actual llvmpipe `20261011T222202.355026Z-hut/capture.png`
inspected: subtle darkening under the deck and around support bases improves grounding.
Image gate PASS (395color bins/luminanceSD50.45/spatial contrast196.80), kernel-after[].
Log confirms hut1draw/7contact patches; all nearby contacts remain1draw (1,704triangles
total in this view). Native build and Android ARM64 assemble/lint PASS4s.
Web build/package PASS (11,208,839bytes raw/4,213,502gzip); no portable runtime audit
yet. Full CPU suite initially40/41 passed: the process-supervisor SIGKILL fixture did
not start within4s. The whole supervisor group passed on an isolated rerun in1.82s
without changes to timeout/acceptance; both results are retained.

## Roof geometry refinement

The initial448 rectangular beam bundles were replaced by672 thin folded leaf strips,
each with three longitudinal stations and a raised central crease. Deterministic
length, course staggering, curvature, pigment and uneven eave tips break the rigid
grid. Constrained lateral drift preserves coverage; no alpha texture or new shader.
Both versions use16,128 roof vertices, so the complete hut remains6,804triangles /
20,412vertices in one draw. The ridge/frame, materials and collider are unchanged.

CPU tests check all672 strips for finite bounds, exact repeatability, downhill station
order, adjacent-strip coverage, overlapping courses and varied lengths. The first
version failed the gap test; lateral jitter was reduced instead of relaxing the test.
Final focused test PASS0.58s. Actual llvmpipe `20261011T222902.709084Z-hut/capture.png`
inspected: the strong rectangular roof grid is gone and the eave is less regular.
The surface still lacks fine fibre/material detail and is not final realistic thatch.
Capture PASS (392color bins/luminanceSD49.92/spatial contrast195.97), kernel-after[].
Native build and Android ARM64 assemble/lint PASS6s; no portable actual audit yet.

The initial plain vertex-color material has now been superseded by the material
described below; earlier image limitations remain historical evidence, not current
claims that the shader is absent.

## Beam-aligned wood and thatch material

The hut now uses a portable GLSL330/GLES300 material with per-vertex grain coordinates.
Longitudinal coordinates follow each beam's actual axis (including floorboards and
diagonal braces), while end faces use transverse coordinates for rings. Thatch uses
its own slope-aligned frame. A per-face material tag selects wood, end grain or dry
fibres. Broad weathering/desaturation and mild basal darkening complement the original
CPU sunlight. Fine sinusoidal bands fade according to screen derivatives and distance;
this is an anti-aliasing implementation, not yet a temporal-motion flicker qualification.

No raster texture or extra draw was added. VertexPositionColorTexture replaces
VertexPositionColor: two grain floats add8bytes per vertex (163,296bytes for20,412
vertices). Geometry, contact shadows and the one-draw layout remain unchanged.
Finite grain coordinates are validated before upload. A clearly logged BasicEffect
fallback remains for unsupported/failed custom effects; it is not accepted as shader
qualification. Native and Android logs confirm the new material without fallback.

Actual native overview `20261011T223611.972091Z-hut` and new close material view
`20261011T223822.323931Z-hut-material` both PASS/image inspected/kernel-after[].
Close view shows longitudinal grain on beams/slats and the layered roof; surface
appearance is still stylized/plain in places, not claimed photorealistic.
CLI/native supervisor accept `hut-material`; Android accepts both hut views and
requires the material startup marker (negative/positive CPU fixtures included).

Actual Android `20261011T224038.084741Z-hut-material` PASS, screenshot inspected:
wood/roof render with contact shadows in GLES SwiftShader; kernel-final[], normal
owned-emulator exit0. Tested APK archived as
`build/android-software/island-hut-material-x86_64.apk`, SHA256
`c04e08f64a718f814a9e1b2f521cff562a46827f00d611a71d3f14916e581541`.
Android23/software37 CPU tests PASS; x86_64 build/lint PASS5s, standard ARM64 restored
and build/lint PASS4s. Web actual material audit, global route/reporting, long-range
aliasing and remaining realism work are still open. No AMD incident fix is claimed.

## Web overview and material audit

The contained Chrome supervisor now supports `--scope hut`, running `hut` and
`hut-material` sequentially with SwiftShader. Each retains the existing overlay
removal/image checks and 60 measured frame intervals, captures before/after simulation,
and requires the material marker, no fallback, one hut draw, a live WebGL2 context and
zero GL error. Exact visible contact counts are seven for the overview and six for
the close camera (the counter follows frustum culling, not total cached footprints).
The latter also matches the earlier native close-view workload log. Parent report
validation rejects missing views, wrong counts, absent material, fallback and lost
contexts; 19 browser and 37 native-supervisor CPU tests pass.

Retained unsuccessful runs:

- `20261011T224903.183259Z-chrome-hut`: the new template mappings omitted the debug
  overlay required by the existing capture-removal check. Added the explicit argument.
- `20261011T225033.343839Z-chrome-hut`: overview passed; close view completed all
  60 intervals with GL error zero and live context but failed the wrongly shared
  seven-patch expectation. Corrected the per-camera expectation and added negative
  fixtures; did not disable the gate. Kernel delta empty. Close initial/after images
  actually inspected: materials are present but the roof still reads too much like
  wooden slats, not convincing natural palm-leaf thatch.

Neither unsuccessful run is relabeled as passed. Static before/after frames and
compositor intervals do not qualify camera-motion aliasing or hardware frame budgets.

Corrected actual run `20261011T225804.399610Z-chrome-hut` passes both views and parent
validation: 60 intervals per view, GL error zero, contextLost=false, material marker
present, no fallback, seven/six visible contact patches, kernel-final empty. Both
final overview/detail screenshots were actually viewed. Roof/wood remain stylized;
the portable material works, but that is not final realism acceptance. Existing Web
payload reused (build/package successful, no source rebuild needed); no native or
Android runtime rerun for these harness-only changes.

## Tapered leaf ends instead of square strip tips

The inspected Web close image still read as wooden roof slats. Each of the672 blades
now has four longitudinal stations: broad overlapping base/body, then a short narrow
tip with deterministic lateral variation and stronger droop at the eave. Reduced
course lift and a raised midrib give a folded leaf profile instead of a square end.
This intentionally adds8,064 vertices /2,688triangles: total28,476vertices /
9,492triangles, still below the30,000vertex upload guard and still one static draw.
No new shader, texture or draw, and no frame-time improvement is claimed.

CPU tests retain finite bounds, repeatability, downhill ordering, course overlap and
length variation. Tip width is required below4.1cm. Body-station adjacency remains
checked; separate tips are now intentional. A stronger combined coverage check unions
all projected strips at1cm slope intervals on both roof sides, requiring no internal
gap over x=0..2.55m,z=-2.20..2.20m. Only the outer fringe may be ragged. Focused CPU
test PASS0.65s.

Actual native `20261011T230406.778103Z-hut-material/capture.png` PASS and inspected:
the eave is visibly narrower/frayed rather than square-ended; the roof surface still
looks too regular/wood-like. This is a silhouette improvement, not completed realism.
Capture416color bins/luminanceSD52.76/spatialcontrast192.60; kernel-after empty.
Android ARM64 build/lint PASS3s, no new Android runtime check for this geometry yet.
The earlier portable screenshots predate these leaf tips and do not qualify them.
Full41/41 non-display tests PASS72.75s; Web build/package PASS11,228,998raw /
4,221,248gzip bytes. All owned jobs terminal; hardware stop latch unchanged.

## Authoritative inventory and continuous spawn-to-hut traversal

The world report previously omitted the hut and its collision proxy. `HutLayout::FindInWorld`
now shares the solid-plus-foliage reservation between the live game and the report.
The separate test fixture explicitly reconstructs exclusions and verifies identical
position/yaw/floor. Geometry and placement are unchanged by this refactor.

The report includes hut count, position/floor/yaw, collision radius, non-enterable
status and all six footings; its collision grid now includes the actual hut proxy.
The fingerprint incorporates the hut and supports. Validation requires a finite hut,
exact rendered-ground footings and the matching proxy. Actual report passes repeat
determinism, fingerprint `a590b7e4fb787269`,790solids/417occupied cells. This does not
fix other report omissions such as ground snakes; those remain separate work.

`tests/HutReachability.hpp` performs a bounded test-only2m cardinal A* search over
the actual terrain and complete solid grid. Conservative expanded obstacle/ground
checks suggest a route; the acceptance proof is replay with ordinary `PlayerMotion`
at1/60s from the real normal spawn(0,-225), with no intermediate SetPosition or flight.
Every step must remain walking, finite, outside solid proxies and free of safety
clamping; each waypoint has a240step deadline. Final point must reach the front
approach within7.5m of the hut. A fully blocked world is rejected.

Actual replay:459.108m,6548continuous steps,225waypoints, reaching the hut approach.
This establishes one dry walkable route in the current simulation, not universal
path connectivity, non-solid foliage visibility or a native/Android/Web input run.
The earlier eight local collision approaches are retained. A visual walk along the
global route and remaining material polish are still required.
Final replay also rejects nonfinite x/y/z, per-step displacement above ordinary walking
speed and any obstacle depenetration. Focused rerun passes unchanged459.108m/6548steps.
Full41/41 non-display tests PASS69.89s before that final test-only strengthening;
Android ARM64 build/lint PASS6s; Web build/package PASS11,228,439raw/4,221,482gzip.
No new graphics run for the placement-sharing/report changes; no visual improvement
or platform input qualification is claimed by these CPU checks.

## Remaining acceptance work

1. Finish weathered wood and natural irregular thatch materials/geometry; the existing
   roof bundles are not visually convincing palm-leaf thatch yet.
2. Ground contact is implemented; consider the remaining directional roof/self-shadow
   limitations. Confirm grass/wind-driven foliage clearance
   through representative views; conservative site discs are not a triangle proof.
3. Visually audit the now CPU-verified spawn-to-hut route and refine the coarse building
   collider if needed. No inventory, crafting, quests or unrelated systems.
4. Complete portable image audits and budget checks; hut world-report data is now included.
   Android/Web QA routing is now implemented. Build success is not runtime qualification.
