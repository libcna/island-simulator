# Fallen-leaf contact with sloped terrain

Date: 2026-10-09. This is game-side scene correctness, not an AMDGPU incident fix.

## Evidence and scope

Five folded dry leaves accompany each near-detail ground rosette after the 2026-10-15 visual
follow-up described below. At the original contact-correction milestone, three leaves shared the
parent's horizontal root plane, even after that origin had been
correctly grounded. Across three world seeds, 607,320 vertex/interior samples
range from **-0.186663 to +0.229299 m** above the actual terrain; 126,987 samples
are below ground. These are sampled triangle points, not unique plants or pixels.

`GroundLitter.hpp` contains the shared leaf profiles,
shading weights and indices for both rendering and the independent contact audit.
The close native comparison restores a previously buried leaf behind the rosette;
this is a local contact improvement, not a completed natural forest-floor appearance.

## 2026-10-15 pointed-leaf visual follow-up

A new same-camera forest-floor audit found that the three five-point/four-triangle
profiles still read as small square cards. The near-only cluster now uses five
seven-point/six-triangle profiles. Two tapered stations lead to a pointed tip on
each side, and a raised off-centre centre vertex forms the fold ridge. The first
three origins and all contact rules are retained. Two additional deterministic
angles/radii stay inside the existing 0.74 m footprint rather than extending the
old radius sequence beyond its qualified height bounds.

This raises one affected plant from 12 to 30 litter triangles. The authored world
adds 270 clearing-edge and 5,940 forest-floor near triangles, with no change to the
far tier, population, instances, vertex format, buffer count, texture use or draw
count. `GroundLitterVertexCount` is now the single shared compile-time boundary for
generation and the independent contact audit; the first implementation exposed and
then fixed the audit's stale hard-coded five-element array before acceptance.

Native baseline `20261015T174806.814354Z-litter` and current-binary evidence
`20261015T181610.307246Z-litter` were inspected side by side. The accepted frame
passes at 196 colour bins, 17.95 luminance deviation, 43.95 spatial contrast and an
empty kernel delta. `IslandRenderedTerrainSurfaceTests` and `IslandCoreTests` pass.

## 2026-10-16 small fallen-twig follow-up

The same near-only rosette now carries two short, curved fallen twigs alongside the five leaves.
Each twig uses four taper rings with a six-sided cross-section, unequal length and bend, and a
separate restrained brown tone. Both reuse dry-litter material tag 2, so every ring is motionless
and terrain-draped by the existing shared height texture rather than intersecting one horizontal
root plane. The established 1.4 m plant footprint still contains both shapes.

The shared variant-zero near mesh grows by 52 vertices / 96 triangles, from 210/254 to 262/350;
the 24/16 far mesh, vertex format, material, height texture, buffers and draw type are unchanged.
Increased local recruitment is deliberately separate placement work and is described in
`forest-floor-colonies.md`. Native `20261015T235018.093212Z-litter` passes at 195 colour bins,
17.95 luminance deviation, 43.96 spatial contrast and an empty kernel delta; the inspected frame
shows the slim twig at the foreground rosette without obscuring the ground texture. Native jungle
guard `20261015T235121.551613Z-jungle` also passes and was inspected. Focused Chrome/SwiftShader
`20261016T002147.822040Z-chrome-litter` passes the inspected litter and grass scenes with 60 finite
intervals each, GL0, live contexts, zero close-target timeouts and an empty final kernel delta. Two
earlier five-minute attempts timed out during the second scene and two diagnostic attempts were
stopped manually; these four rejected runs are not used as evidence.

## Implementation and resource lifetime

- Material tag 2 identifies only dry litter in the existing 52-byte jungle vertex
  format. A startup check rejects wind on that tag. Live-leaf and wood lighting
  retain their own tags; dry-leaf normals use derivatives of the deformed surface.
- The portable vertex shader samples exact rendered terrain triangles at each
  litter vertex and adds its original scaled fold height. Upright foliage is not
  flattened or deformed. Canopy-light receiving uses that vertex's new ground height.
- `RenderedTerrainHeightMap` owns the one existing 241 by 241 RGBA8/RGB24 texture
  previously owned privately by grass. `IslandGame` constructs it before borrowing
  renderers and declares it before them, so reverse member destruction releases it
  last. Reload replaces consumers before replacing the old owner; failed construction
  destroys local consumers before their local owner. Neither renderer destroys or
  uploads this texture during draws. Slot 0 is canopy light; slot 1 is terrain height.
- Grass retains its existing shader/decoding/sampler path. There is no duplicate
  texture: the required custom paths still use 232,324 GPU bytes for this map. A
  startup-only CPU height cache creates the shared texture; no CPU cache is retained
  by the map. The shared object is also allocated in stock-effect configurations,
  where it is unused; fallback litter remains on the original flat root plane.
- Cell bounds include the full terrain height range under the litter's 0.74 m
  scaled radius, its folded height and 0.1 mm packing/numerical allowance. Near/far
  mesh bounds remain included. No extra draw or mesh attribute is added.
  With the later two-twig addition the jungle shared vertex payload is 3,419,260 bytes and
  variant-zero rosette near geometry is 262 vertices / 350 triangles; its 24/16 far tier remains
  unchanged. Each tagged vertex adds three vertex texture fetches
  and the shader branch; physical GPU performance remains unmeasured.

## Validation

All 32 non-display groups pass (24.71 s). Tests validate profile bounds, index/triangle
planform, invalid profile IDs, scaled/yawed placements, packed-height reconstruction
and conservative renderer height bounds. Four barycentric subdivisions per triangle
across three seeds yield corrected sampled clearances **0.0103827–0.0634251 m**,
with zero sampled penetrations. These nonzero clearances preserve the designed
folds; they are not floating-origin errors. This is a CPU shader emulation and a
dense sampled check, not GPU readback or proof for every possible seed/point.

Native contained llvmpipe `litter` captures:

- Before: `20261009T112732.839300Z-litter`.
- After: `20261009T113215.353315Z-litter`.

Both pass image/shader checks and have empty live/final kernel deltas. Converted
PNGs in `build-probe/island-litter-contact-{before,after}.png` are inspected. The
folded leaves remain coarse and small; wind remains active in surrounding foliage,
so these are not frozen pixel-isolation measurements.

Contained Chrome/SwiftShader `20261009T113543.634482Z-chrome-litter` passes both
litter and grass, 60 intervals each, with zero GL errors/context loss and empty
live/final kernel deltas. Both logs include the new diagnostic; both captured images
are inspected. These captures precede the interval series, not post-simulation
images. Host browser PID 4067759 exits. The focused `litter` scope requires the
intended rosette camera and both shared-resource consumers; existing broad suites
remain unchanged. It does not repeat full input/lifecycle or moving-camera checks.

Android x86_64/restored ARM64 assembly and lint pass (5 s each); CNA retains its
existing warning that GLES3 is primarily tested on Linux. Contained API 35
KVM/SwiftShader `20261009T113904.058650Z-litter` passes, app PID 1958 / entry
TID 2035, first-draw total 10,925.940 ms. Required shader marker is present, the
1280x720 image is inspected, live/final kernel deltas are empty and the owned
emulator exits zero. This is not renewed touch/lifecycle/traversal testing or
ARM64 runtime qualification. Only rolling APKs were replaced; hashes are recorded
in `build-android.md`. Software evidence does not qualify AMD hardware or physical
mobile performance. Refreshed Web package: 13,910,123 raw / 6,973,507 gzip bytes.

Final native incremental build and all 32 non-display groups pass (22.25 s),
including Web package validation. Script syntax/whitespace checks pass. No owned
graphics process survives, the final kernel interval since 13:25 CEST is empty,
and the hardware stop-latch hash is unchanged. Disk has 359 GiB free; original
incident evidence remains preserved.

## Remaining work

The stock-effect fallback is not terrain-draped. Litter still disappears with the
rosette's near LOD at 30 m and repeats five simple folded shapes. This is not a
complete forest litter system or slope correction for broad tree/shrub roots.
The original 2.1 cm local base clearance is retained, rather than claiming every
edge touches the ground; reducing it needs a finer surface-intersection audit.
Close off-path composition, surface detail, smooth LOD and vegetation/prop overlaps
remain open. No CNA/Sharp Runtime files or AMD hardware qualification state changed.
