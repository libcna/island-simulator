# Small inland mountain cave — implementation in progress

This is a project-owned procedural mesh, without imported assets or licence dependencies.
The native game now constructs and renders the cave and terrain replacement.
Do not mark the PLAN entry complete: moving visual polish and Android runtime remain.

## High-clearance eroded ceiling detail — 2026-10-13

The accepted rock material still left the twenty-metre interior as one visually continuous smooth
arch. Six small project-owned ceiling pendants now break that repetition between 2.4m and 11m
depth. Each uses a seven-sided irregular root, a narrower rotated middle ring and an offset tip,
rather than a single regular cone. The lowest tip remains above 2.3m local height, safely above the
walking capsule, so the change neither narrows the corridor nor invents collision the player can
reach. It adds 126 triangles to the existing rock buffer/draw: the rendered cave is now 1,510
triangles, while the unchanged closed 1,424-triangle `CaveLayout` still owns collision and the
forty coplanar floor triangles remain omitted from the rock renderer.

The first single-ring attempt `20261013T210440.655985Z-cave-inside` passed technically but was
visually rejected because the pieces read as regular geometric cones. Final strict native
llvmpipe evidence is `20261013T212901.927564Z-cave` outside (273 colour bins, 48.99 luminance
deviation, 226.19 spatial contrast) and `20261013T212749.826507Z-cave-inside` inside (65 bins,
24.57 deviation, 116.01 contrast); both pass the exact 1,510-triangle/detail marker gate with
empty kernel deltas and were inspected. The exterior portal stays clean, while the interior gains
small attached overhead silhouettes without blocking its view to the sea.

Focused Chrome/SwiftShader `20261013T211636.896127Z-chrome-cave` passes both views with 60 finite
intervals each, `glError=0`, intact contexts and an empty final kernel delta. All four images were
inspected. Release and Debug cave-layout/asset tests, 33 browser-runner tests and Android ARM64
assemble/lint pass without an emulator or KVM. This is a bounded interior-detail improvement, not
final geological realism; free-walk moving review, broader exterior integration and Android-device
rendering remain open.

## Mirrored triplanar rock material and current WebGL views — 2026-10-13

The shell now uses an original ImageGen volcanic-rock albedo study preserved with its
exact prompt at `assets/art-studies/cave-rock-albedo-v1*`. A repeated-edge audit found
that the requested source was not truly seamless, so it is not described or sampled as
such. The build area-reduces the unchanged 1254-square source to a 256-square runtime
base; nine opaque mip levels total 349,524 RGBA8 bytes. Mirrored triplanar sampling
projects it across walls and ceiling without an edge wrap, while normalized mean colour
retains the existing large-scale mineral, fracture, moss, dampness and depth-lighting
model. Connected exterior/interior strip vertices share lighting normals; the eroded
portal and rear break remain sharp. Geometry remains 1,384 rendered rock triangles in
one draw and collision still uses the unchanged closed shell.

The actual replacement terrain now extends an irregular procedural stone apron beside
the open corridor. This softens the former hard grey-shell/green-ground join without a
decal, prop, additional texture binding or draw. It does not hide or narrow the entrance.

Final inspected native llvmpipe evidence is `20261013T134635.899208Z-cave` outside
(279 colour bins, 49.00 luminance deviation, 226.27 spatial contrast) and
`20261013T134710.416889Z-cave-inside` inside (65 bins, 24.76 deviation, 119.65
contrast); both pass with empty kernel-fault deltas. The new focused
Chrome/SwiftShader run `20261013T135551.626255Z-chrome-cave` contains independently
inspected outside and inside images, 60 finite intervals per view, GL error zero, intact
contexts and no new kernel GPU fault. It validates the packaged texture and shader on
WebGL2, not free walking or final artistic realism. The exterior is smoother and better
grounded than the previous large flat-panel shell, but its overall outcrop silhouette is
still visibly procedural. Android runtime, moving exterior review and further silhouette
weathering remain open.

The current ARM64 `assembleDebug`/`lintDebug` build passes without launching an
emulator. Its APK contains exactly one 155,369-byte prepared cave PNG; SHA-256
`8533c4d9e044dce0c5f3a1f3dbeeaa3539f9a8a3e920029b96624a40b9ef4b21` matches
the Gradle-owned prepared source byte for byte. This is packaging evidence only, not
GLES execution. The native release package includes the same material (15,088,798-byte
installed payload), and the rebuilt Web package is 13,091,260 bytes raw / 5,412,965
bytes gzip. Runtime texture inventory is now 10,091,644 bytes inside a 10,111,860-byte
quality budget.

## Geometry and siting

Entrance approximately `(108.005,14.0317,-73.8899)`, on the south-eastern face of the main
ridge,100m inland from the radial shore. Axis points farther into the rising mountain.
Interior20m, outer shell22m; usable width
roughly5m, ceiling roughly3.4m above the floor. The floor rises0.5m toward the back.
The tide-pool footprints do not overlap the cave reserve.

`CaveLayout` emits1424 consistently wound triangles with unit face normals. Every edge
has two oppositely directed incident faces: closed finite rock, not an infinitely thin
visual arch. The empty cavity remains open at the entrance. Signed rock volume1286.82m³
after the first visual revision (broader irregular mass and projecting entrance lip).
Those exact shell triangles drive full-height capsule collision, including initial
solid occupancy, walls, ceiling, floor and back. Sweeps bound steps to10cm and refine
the first blocked interval. The intended player capsule is radius0.34m,height1.9m;
this is not a general continuous-collision solver for arbitrarily tiny objects.
Whole-landmark and per-triangle broad phases avoid expensive distant collision work.

`CaveGroundPatch` replaces a grid-aligned rectangle of the existing coarse terrain.
Four subdivisions per coarse cell produce2793 vertices/5376 triangles. Perimeter
heights exactly match the original emitted terrain. Interior support follows the
planar cave floor. Queries barycentrically sample the actual replacement triangles.
Flight tracing includes the rounded patch grid edges and triangle diagonals, not just
endpoint heights. TerrainRenderer removes the covered coarse quads, adds the replacement
in one separately culled draw, and shades the floor with depth-dependent subdued stone.
CaveRenderer skips the40 coplanar floor triangles and adds126 high-clearance decorative ceiling
triangles:1510 rock triangles in one culled draw.
Both use portable desktopGL/GLES shaders; the shell now samples the prepared rock image
described above while the replacement floor and apron stay procedural.

## Controller checks

`PlayerMotion::SetCave` and `FlightMotion::SetCave` are non-owning opt-in hooks; both
referents must outlive the controllers. Without hooks existing movement stays unchanged.
Tests exercise entry, reaching the back wall, blocked forward movement, exit, invalid
placement recovery, activating flight inside, upward flight into the inner roof,
downward flight onto the outer roof, floor containment and landing inside.
Shell tests also cover both side walls, all surface groups, invalid inputs, and exact
edge topology. Patch tests independently verify every emitted triangle's height and
affine flight intervals. No visual or platform-runtime success follows from these checks.

## Scoped world integration

Conservative placement survey before mutation:5 palms (5m margin),0 canopy (6m),
5 understory (2m),1 bush (2m),7 ground plants (1m),1 rock (3m),0 logs (4m),1 frond (2m).
The shared lightweight CaveSite reserve is now respected by route-aware layout generators
and grass. The corresponding survey counts are all zero after integration. The walking
test also includes actual generated palm/jungle/prop collision proxies. Compared with the
previous deterministic population fixture, ordinary bushes fall576→574 and ordinary
ground plants2205→2193. Animal populations are not intentionally reduced.

The first native outside image (`20261012T172702.248748Z-cave`) passed the mechanical
capture gate but failed artistic review: too regular/manufactured and a patterned floor.
The revision broadens/breaks up the rock, adds a modest buried terrain shoulder and replaces
the floor's periodic grain with noise. Inspected revised captures:
`20261012T173120.671828Z-cave` (267 colour bins, luminance deviation49.90) and
`20261012T173129.978957Z-cave-inside` (66 bins, deviation25.15), both mechanical PASS,
no new kernel GPU faults. Entrance/interior are unobstructed, floor aliasing is gone,
and the sea is visible through the mouth. The first-pass shape/material still looks
stylized and tunnel-like; natural erosion, rear blending and less regular strata need
further polish. These images are not evidence of live traversal or portable runtime.
Core and cave tests pass2/2 in24.36s; software-runner CPU tests47/47.
Flight, spider habitat, snake population and WorldReport regressions pass4/4 in18.40s;
world fingerprint after the reserved footprint is `8d45b4d307bfeff3`.
Both existing native build trees are refreshed; Debug cave tests pass in6.06s.

## Native live keyboard traversal

`python3 tools/run_fullscreen_software.py build-consumer --cave` uses the existing supervised
private llvmpipe/pixman display. F1+L places the player8m outside the cave. Real W input walks
to the back wall at depth19.6597 and remains held there; Tab/Space tests inner-roof blocking,
Shift descends, Tab lands, and S walks out to depth-6.527. Evidence
`build-consumer/software-cave/20261012T173950.239540Z` passes with88diagnostic capsule samples,
none intersecting solid rock. Back/exit screenshots were inspected. The very near back-wall
image is intentionally dark and does not substitute for the separate inward/outward visual views.
The validator rejects missing/out-of-order stages, invalid data, rock penetration and incorrect
wall/roof/floor/exit positions; Python runner tests48/48.

This focused gate excludes F11. The separate fullscreen regression currently fails exact window
position restoration in private Xwayland/Openbox, including after35s; it is an open finding,
not a reason to claim cave traversal failed or fullscreen passed.

Next gates:

- Check exterior silhouette/terrain transition, vegetation, contact shadows and animal routes.
- Inspect fixed `cave` and `cave-inside` captures after every material/shape revision.
- Verify live outer-roof containment, performance and supported Web/Android runtime.

No AMD workload, Android emulator or KVM change is needed for the CPU foundation.

## Wildlife integration follow-up

The original6m half-width reserve did not include the expanded exterior. CaveSite now
reserves8m, exceeding the shell's analytical7.62m maximum. Tests check every emitted
triangle vertex against this footprint. Boars and goats reject it with their whole-body
clearance radius; snakes check every body point and connecting segment, and spiders
check both homes/patrol clearance and live body poses. Existing population counts remain
6 boars,6 goats,50 snakes,200 spiders. An18s nearby simulation verifies bodies and spider
feet remain outside the rock footprint. No rendering cost is added by these CPU checks.

The current wider-reserve fixture contains570 ordinary bushes and2187 ordinary ground plants
after later world integrations; all surveyed cave-overlap counts remain zero.
The WorldReport now fingerprints the actual shell and replacement mesh and exports its
entrance, length and geometry counts. Cave/core/spider-habitat/snake/WorldReport checks
pass5/5 in36.64s. Both native binaries were subsequently refreshed with the campfire
integration; Debug cave/campfire/audio-runtime checks pass3/3 in7.34s.

## Compact outcrop and material pass

The next inspected exterior audit still read the first shell as a freestanding hangar. The
walkable cavity remains roughly5m wide,3.4m high and20m deep, but the surrounding mantle is now
lower and narrower: measured signed rock volume is646.184m³ instead of1286.82m³. Entrance-only
height/width perturbations make the visible opening less symmetric and decay into the interior;
the outer lip projects less. The replacement terrain now forms only a low bank at the mouth.
An attempted larger bank produced two obvious heightfield pyramids and was rejected rather than
retained.

The rock draw still uses1384 triangles and one culled opaque submission. Its project-owned
40-byte vertex now carries position, face normal, cave-local position and an exterior/interior
material weight. The fragment shader uses fixed-sun normal lighting, non-horizontal perturbed
fractures, broad warm/cool mineral variation, restrained upward-facing exterior moss and subtle
depth-dependent interior dampness. It adds no texture, transparency or draw. Interior ambient
light remains dark but preserves enough reflected colour to read the surface and exit.

Inspected native llvmpipe captures `20261012T235905.250725Z-cave` (outside:277 colour bins,
luminance deviation48.99, spatial contrast226.27) and
`20261012T235910.914031Z-cave-inside` (inside:64 bins, deviation24.33, contrast109.22) both pass;
live and final kernel deltas are empty. Cave/Core tests pass2/2 after the compact roof collision
envelope and solid-placement fixture were updated to the emitted shape. The current complete world
report remains valid with fingerprint `a03613b6a3bbbbab`. This is a bounded visual
improvement, not a claim of photorealism. The fresh native keyboard run
`software-cave/20261012T235642.504887Z` again completes entry, back-wall stop, inner-roof block,
landing and exit across88 non-solid samples. Chrome/SwiftShader WebGL2 run
`20261012T235501.030155Z-chrome-fullscreen` initializes and draws the complete scene with the new
cave shader, zero GL error/context loss and an empty kernel delta; it is portability evidence,
not a cave-view image. GLES3 runtime, moving exterior review, contact treatment and further
erosion detail remain open.

## Eroded entrance silhouette pass

A further native visual audit rejected two experiments rather than retaining them. A taller
terrain shoulder produced a conspicuous green heightfield wall, and eight ordinary prop rocks
read as differently coloured pebbles pasted onto the portal. The retained change stays within
the single closed cave solid: stronger entrance-local width/height variation makes the opening
less symmetric, the outer mantle now undulates at broader non-repeating frequencies, and a
restrained narrow lip grows into the wider rock mass several metres inland. The exact collision
and rendered triangles therefore still agree. Measured rock volume is now544.920m³, inside the
existing compact350–800m³ budget; the 1.9m capsule corridor remains unchanged.

Inspected final-build llvmpipe evidence `20261013T061242.381645Z-cave` shows the revised exterior,
while `20261013T061246.258725Z-cave-inside` verifies the asymmetric daylight opening from inside.
The focused live keyboard run `software-cave/20261013T061331.542394Z` again reaches the back
wall, blocks at the inner roof, lands on the replacement floor and exits without any solid-rock
sample or kernel GPU fault. Chrome/SwiftShader evidence
`20261013T061512.375165Z-chrome-fullscreen` supplies60 finite presentation intervals with zero
GL error, an intact context and no kernel fault after constructing/drawing the revised closed
mesh; it is portability evidence, not a cave-view visual acceptance. The outcrop remains
intentionally marked in progress: its exposed
mantle still reads more procedural than a naturally weathered headland and needs future contact
and moving-view polish.

## Inland mountain relocation — 2026-10-14

The former38m-inland headland site still read as a freestanding tunnel beside the coast. The
entrance is now100m inland at `(108.005,14.0317,-73.8899)`. The unmodified slope rises about
3.67m over the first10m behind the portal and7.50m over20m, so the closed shell terminates inside
the mountain rather than on the beach. Construction and tests require more than90m signed shore
distance and at least5.5m terrain rise at the back. The replacement patch now has2597 vertices /
4992 triangles; a widened full-excavation interval prevents steep original terrain from bleeding
through the interpolated walkable floor.

Inspected llvmpipe evidence is `20261014T004021.580610Z-cave` (overview:250 colour bins,
46.99 deviation,243.05 contrast) and `20261014T004118.743382Z-cave-inside` (entrance from
inside:157 bins,26.56 deviation,115.36 contrast). The overview shows the portal attached to a
forested rising slope with no beach beneath it; the interior image shows an unobstructed opening.
`IslandCaveLayoutTests` walks from outside to depth19.6592, blocks at the back/roof, lands on the
floor and exits, while all cave footprint surveys remain zero.
The live llvmpipe keyboard run `software-cave/20261014T011438.261480Z` independently repeats
entry, back-wall, ceiling, landing and exit stages and finishes without a new kernel GPU fault.
Chrome/SwiftShader evidence `20261014T005636.229249Z-chrome-cave` also renders both the wooded
mountain overview and the unobstructed view out of the entrance with its WebGL context intact.
The relocated complete world validates with fingerprint `a33aa6c2913b5796`.
