# Forest-floor recruitment colonies

Date: 2026-10-09. This is a deterministic placement pass using existing original
plant meshes, not an additional asset, AI system, vegetation shader or GPU workaround.

## Composition

The original floor layer selects independent jittered 4.7 m sites. Only the two
clearing edges previously had related local groups. The new pass adds irregular
small-plant colonies near some canopy trees, retaining the original island-wide
placements and clearing accents. Grass density and all solid collisions are unchanged.

Each eligible parent has a 22% deterministic omission chance. Its group centre sits
3.2–7.2 parent-scale metres from the trunk. Independently rotated ellipses use
2.5–3.8 m and 1.1–2.4 m semi-axes, with four to eight uniformly area-distributed candidate
sites. Hash channels derive from parent millimetre coordinates and the world seed,
not vector ordering or a shared random stream. Accepted plants usually share one
species, with an occasional contrasting member; short rosettes are predominant.
Centre plants tend to be larger and edge plants smaller. Scale remains 0.58–0.94,
inside the existing species envelope. This is authored composition, not simulated
seed dispersal or a botanical/ecological claim.

Rejection checks include inland shoreline distance (43–238 m), ground height,
maximum 24-degree slope, minimum 0.72 m separation from earlier floor roots,
0.9 m from shrub roots and 0.85 m from medium-plant roots. The full scaled animated
footprint must avoid reserved routes and canopy trunks. Groups remain outside the
separate clearing-accent bands, preserving their original placements. Not every
candidate survives, and some parents have no group at all.

`ForestFloorColony` retains parent index, ellipse and contiguous plant range for
validation/reporting. It adds no renderer-specific ownership or animation state.
The renderer consumes the same placement vector, cell lists, instancing and LOD.
`island-world-report` exposes group/companion counts; startup logs full-population
near/far triangle totals explicitly, not a claim that all plants are visible.

## Measured default layout and cost

- 199 groups add **866 floor plants**, increasing the floor total from 2,282 to
  **3,148** (37.95%). There remain 281 canopy trees, 1,822 medium plants, 578 shrubs
  and 18,556 grass clusters. All original 13 clearing shrubs / 57 companions remain.
- Added plants would total 374,700 triangles if all used near detail, or 37,200
  if all used far detail. Actual culling/LOD reduces submissions; these are budgets,
  not frame cost or hardware timing.
- Shared jungle mesh storage is unchanged at 2,893,748 vertex bytes. No new mesh,
  texture, material or batch type exists. Instance/placement and colony metadata
  storage grow; initialization performs bounded candidate/root checks and is not free.
- The fixed jungle view submits 173 -> 235 floor plants: near 35 -> 48, far
  138 -> 187. Jungle draw calls remain 35. Other submitted populations are unchanged.

## Validation and visual scope

All 32 non-display test groups pass in 22.31 s. Three seeded layouts check count
budgets without increasing the pre-existing 5,000-plant limit, contiguous membership,
unique parent association, ellipse bounds, restrained stature, shore/slope constraints,
root separation and full path/trunk clearance. Independent 64-point footprint samples
test the route exclusion. Repeated construction checks exact positions and metadata.
Original default base counts and clearing accents are explicitly protected.

Contained native llvmpipe matched-camera captures under `build/software-captures/`:

- Before: `20261009T103813.830248Z-jungle`.
- After: `20261009T104357.002910Z-jungle`.

Both pass shader/image validation with empty live/final kernel deltas. Converted
PNGs `build-probe/island-floor-colonies-before.png` and
`build-probe/island-floor-colonies-after.png` are inspected. The path remains open;
the effect from this camera is modest, with much new foliage occluded by grass.
Wind/wildlife are active, so this is not a frozen pixel-isolation experiment.
This inspection does not establish realistic density throughout the island.

Contained Chrome/SwiftShader matrix `20261009T104814.928788Z-chrome-vegetation`
passes all eight scenes (grass, jungle, fronds, east, clearing-edge, ground-broadleaf,
ground, bush), 60 presentation intervals each. All report zero GL error and no context
loss; live/final kernel deltas are empty. Host browser PID 4043102 exits normally.
All eight runtime logs report exactly 199 groups / 866 companions and matching
triangle totals. Post-simulation jungle and ground-broadleaf images are inspected.
This focused matrix does not repeat full input/lifecycle or moving-camera coverage.
The refreshed Web package is 13,902,136 raw / 6,970,160 gzip bytes and passes package validation.

Android x86_64 and restored default ARM64 assembly/lint pass (4 s each). Contained
API 35 KVM/SwiftShader `20261009T105740.149768Z-jungle` passes, app PID 1755 /
entry TID 1894, first-draw total 9,830.804 ms. Its inspected 1280x720 frame retains
the jungle floor and touch overlay. Runtime logs match the native/Web group and
triangle counts. Live/final kernel deltas are empty and the owned emulator exits
zero. APK hashes/sizes are recorded in `build-android.md`; only rolling generated
packages were replaced. No renewed Android touch/lifecycle/traversal gate or ARM64
runtime/physical-device performance claim. All builds use shared ccache/two workers.

Final 32 non-display groups pass in 22.17 s, including the refreshed package.
Script syntax and diff checks pass. No owned graphics process survives; the final
kernel interval since 12:37 CEST is empty and the hardware stop latch is unchanged.
359 GiB remains free: browser evidence 217 MiB, native captures 579 MiB and Android
software 1.7 GiB. Original incident evidence and read-only dependencies are preserved.

## Remaining work

Inspect a close off-path group and additional headings before increasing density.
Sparse floor cover, repeated silhouettes, grass shape/material and weak grounding
still limit realism. All historical plants are not retroactively given full-footprint
reservation. New roots avoid canopy trunks, shrubs and medium plants but not every
palm, rock or fallen log; overlapping foliage is allowed. Leaves are positioned from
the root's terrain height, not fully draped over sloping terrain. New groups use the
existing discrete ground-plant LOD transitions and do not solve their popping.
Physical GPU/mobile cost remains unqualified while the AMD incident stop latch is active.
