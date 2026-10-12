# Small headland cave — implementation in progress

This is a project-owned procedural mesh, without imported assets or licence dependencies.
The core foundation exists; **the live game does not yet construct or render the cave**.
Do not mark the PLAN entry complete on the strength of unit tests.

## Geometry and siting

Entrance approximately `(184.04, 7.63779, 115.384)`, east/northeast headland,38m inland
from the radial shore. Axis points inland. Interior20m, outer shell22m; usable width
roughly5m, ceiling roughly3.4m above the floor. The floor rises0.5m toward the back.
The tide-pool footprints do not overlap the cave reserve.

`CaveLayout` emits1424 consistently wound triangles with unit face normals. Every edge
has two oppositely directed incident faces: closed finite rock, not an infinitely thin
visual arch. The empty cavity remains open at the entrance. Signed rock volume482.874m³.
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
endpoint heights. Integration must remove covered coarse quads, not overlay this mesh.

## Controller checks

`PlayerMotion::SetCave` and `FlightMotion::SetCave` are non-owning opt-in hooks; both
referents must outlive the controllers. Without hooks existing movement stays unchanged.
Tests exercise entry, reaching the back wall, blocked forward movement, exit, invalid
placement recovery, activating flight inside, upward flight into the inner roof,
downward flight onto the outer roof, floor containment and landing inside.
Shell tests also cover both side walls, all surface groups, invalid inputs, and exact
edge topology. Patch tests independently verify every emitted triangle's height and
affine flight intervals. No visual or platform-runtime success follows from these checks.

## Scoped world integration still required

Conservative placement survey before mutation:5 palms (5m margin),0 canopy (6m),
5 understory (2m),1 bush (2m),7 ground plants (1m),1 rock (3m),0 logs (4m),1 frond (2m).
Reserve the actual excavation and entrance approach in layout generation; check grass,
obstacles and wildlife separately. Do not delete arbitrary surrounding vegetation.

Next gates:

- Wire the replacement terrain, rock renderer and both controller hooks into IslandGame.
- Avoid duplicate coplanar floor geometry; shade the rendered floor consistently with
  the interior, and retain daylight at the entrance.
- Check exterior silhouette/terrain transition, vegetation, contact shadows and animal routes.
- Add fixed exterior/interior cameras; inspect isolated llvmpipe captures first.
- Verify live entry/exit and flight containment, performance and supported Web/Android runtime.

No AMD workload, Android emulator or KVM change is needed for the CPU foundation.
