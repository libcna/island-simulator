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

## Remaining acceptance work

1. Finish weathered wood and natural irregular thatch materials/geometry; the existing
   roof bundles are not visually convincing palm-leaf thatch yet.
2. Ground contact is implemented; consider the remaining directional roof/self-shadow
   limitations. Confirm grass/wind-driven foliage clearance
   through representative views; conservative site discs are not a triangle proof.
3. Audit global player-route reachability and refine the coarse building collider if
   needed. No inventory, crafting, quests or unrelated systems.
4. Add world-report data and Android/Web QA routing; perform actual portable image
   audits and budget checks. Build success is not runtime qualification.
