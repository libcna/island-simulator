# Small headland cave — implementation in progress

This is a project-owned procedural mesh, without imported assets or licence dependencies.
The native game now constructs and renders the cave and terrain replacement.
Do not mark the PLAN entry complete: wildlife/visual polish and portable runtime gates remain.

## Geometry and siting

Entrance approximately `(184.04, 7.63779, 115.384)`, east/northeast headland,38m inland
from the radial shore. Axis points inland. Interior20m, outer shell22m; usable width
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
CaveRenderer skips the40 coplanar floor triangles:1384 rock triangles in one culled draw.
Both use portable desktopGL/GLES shaders without a texture download or extra image asset.

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
