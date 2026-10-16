# Forest stands, emergent trees, saplings and standing deadwood

## Flared deadwood branch collars — 2026-10-16

A renewed vegetation/terrain/water comparison selected the close standing snags. Their roots,
bleached bark, irregular fracture caps and habitat were already sound, but each large broken limb
still began as a narrow tube intersecting the trunk at a hard angle. This made the silhouettes
look assembled rather than grown and storm-broken.

Each near-LOD limb now starts with a short open-ended tapered collar. Its first broad ring is buried
inside the trunk, then narrows into the existing bent limb without an internal fracture cap. Only
the actual broken end retains pale exposed wood. The two near variants grow from 729 vertices /
1,170 triangles to 939 / 1,530 each; far variants remain exactly 62/84 and 93/126. All twelve
placements, the shared instanced draws, material, collision footprint and culling distances remain
unchanged. Even an all-near worst case adds only 4,320 triangles across the twelve existing trees.

Inspected native baseline `20261016T130936.255742Z-snags` and final evidence
`20261016T131725.835873Z-snags` show the main fork and lower stub widening continuously into the
trunk. The final image passes at 205 colour bins, 41.87 luminance deviation and 191.41 spatial
contrast with an empty kernel delta. Inspected Chrome/SwiftShader evidence
`20261016T132035.150848Z-chrome-snags` passes 60 finite positive intervals, GL error zero, a live
context and no new kernel GPU fault. WebAssembly rebuilt; Android ARM64 assemble/lint passed
against the locked Sharp Runtime copy, producing a 24,671,067-byte APK with a 19,628,496-byte
`libmain.so`. No emulator or KVM was used.

2026-10-10, from the macro-geography analysis items 7 and 8: the canopy was an even 14.5 m
jittered lattice with mild cluster noise, so from above the forest read as a park of separate
trees of one height.

## What changed (`JungleLayout.cpp`)

- Candidates every 10.5 m over the same area (grid radius 28) so crowns can touch inside stands.
- A contrast-shaped stand field (`smoothstep` of 0.72 broad 61 m + 0.28 fine 23 m cluster noise
  around 0.40-0.64) drives acceptance: `density = (0.52 + 0.48 interior) * (0.045 + 0.60 stand)`.
  Dense stands approach closure, open ground between them stays open, and a thin scatter of
  solitary trees remains.
- Emergents: trees with `stand > 0.60` and a 9 % hashed chance are promoted in a second pass
  only when at least two accepted neighbours stand within 20 m, to scale 1.60-1.85 (about 19-24 m
  overall). Their collision radius follows the trunk (`0.43 x scale`). Crown lobes, the canopy
  light occluders, per-tree culling boxes and contact shadows all scale with the tree.
- Saplings: a separate 10% candidate channel inside the stand field converts a bounded subset of
  later placements to scale 0.18--0.31. `CanopyNominalHeight` measures the full nominal crown
  envelope of every shared form, and the regression requires every juvenile to remain in the
  requested 2--5 m band. A sapling must retain an established neighbour within 18 m. The default
  world contains 16; no tree, mesh, buffer or draw is added. Their root collision shrinks with the
  visible trunk down to a 0.12 m minimum. Monkeys do not select them as
  closed-canopy climbing trees, and the seven qualified climbing-home indices remain adult.
- Standing deadwood: twelve separate deterministic placements are selected only after every live
  vegetation layer is complete. Each must be on dry stable inland ground, have at least two mature
  canopy neighbours within 25 m, keep the complete route/clearing envelope open, avoid living
  trunks and the full authored bush/forest-floor footprints, and stay separated from other snags.
  Their obstacles are appended after the canopy prefix so canopy-index consumers stay stable.
  Two shared silhouettes use bent tapered stems, asymmetric thick broken branches, a short lower
  scar, desaturated sun-bleached bark and pale uneven fracture caps. Near/far meshes are instanced,
  cell-culled and bounded by the same terrain-grounded renderer path as living jungle plants.
  Their near LOD also carries five uneven, low structural root spurs. The spurs vary in length,
  rise and sideways bow so the dead trunks transfer weight visibly into the soil instead of reading
  as cylindrical poles; they reuse the same mesh, bark material and draw.

Current default population after the later ecology passes: 261 living canopy trees including 16
saplings and 22 emergents, plus 12 separate standing snags; 1,737 understory plants, 569 bushes, 2,903 ground
plants, 171 forest-floor colonies with 674 companions, 203 rocks, 12 fallen logs and 33 driftwood.
Tests pin the stand structure (>= 60 % grouped, <= 15 % solitary, 3-15 % emergent, emergents only
inside stands), the literal sapling height envelope and the snag habitat/clearance invariants.

## Evidence and limitations

Native llvmpipe `build/software-captures/20261010T150916.889020Z-jungle`, `150924.874906Z-overview`,
`150932.482781Z-east` (pass, inspected): the overview shows clumped masses with open gaps and
emergents breaking the canopy line. Masses are still made of the same eight crown forms and the
forest edge is not yet shaped by wind or soil; terrain macro-geography (item 1) remains open.
Platform runs: Chrome/SwiftShader vegetation scope `build/browser-software/20261010T152203.104225Z-chrome-vegetation`
passes all eight views. The Android emulator crash first attributed to this pass was later
traced to the cumulus loop in the sky shader (`docs/sky-atmosphere.md`); the stands and the
forest-floor colonies were innocent and are unchanged. Android results after the sky fix are
recorded in `PLAN.md`.

The corrected sapling pass is evidenced by native detail capture
`build-consumer/software-captures/20261014T213040.320780Z-sapling`: the first sapling measures
3.74935 m and its inspected silhouette is visibly juvenile beside the mature stand. Matching
Chrome/SwiftShader evidence is
`build/browser-software/20261014T213320.262214Z-chrome-saplings`: the inspected frame matches native,
all 60 intervals are positive and finite, GL error is zero, the context remains live and the kernel
delta is empty. The core regression first caught, then prevented, loss of the ordinary monkey climb
path. Standing dead and broken trunks are covered by the later native detail evidence
`build-consumer/software-captures/20261014T211429.632358Z-snags`. The first image exposed a dark,
thin-armed pole silhouette; the inspected final image has thicker asymmetric stubs, a blunter
broken crown and lighter deadwood separation. Core regressions pin all twelve placements, habitat,
route clearance, living-trunk separation, collision extent and determinism. Chrome/SwiftShader
subsequently passes the dedicated
`build/browser-software/20261014T212223.406022Z-chrome-snags` view: the inspected frame matches
native, all 60 intervals are positive and finite, GL error is zero, the context remains live and
the kernel delta is empty. The ARM64 debug APK assembles to 24,014,955 bytes with SHA-256
`25ae9ff46e6acafba0079a570f2516a986b45bca51cab1386d5c0227f11e5372`; no emulator or KVM was
started, so physical GLES runtime evidence is still not claimed.

## Flexible juvenile-form pass (2026-10-15)

A new bush/sapling/snag detail comparison found that juvenile stature and placement were correct,
but scaling the complete adult form to 0.18--0.31 left a conspicuously straight miniature pole and
regular vertical crown tiers. The portable instanced vertex shader now recognizes that disjoint
scale band and applies a deterministic instance-seeded post-root sweep. A second perpendicular,
height-continuous component gives successive trunk and crown heights unequal lateral offsets.
Because every wood and leaf vertex at a given height follows the same field, the tree remains
connected; the first 2.8 local metres remain on the qualified collision/root axis.

The response is zero before the ordinary 0.78 minimum adult scale, so mature trees, emergents,
mangroves and monkey climbing geometry are unchanged. No vertex, mesh, instance attribute, buffer
or draw is added. Juvenile culling bounds include a conservative 0.75-local-metre horizontal margin
covering the maximum deformation.

Inspected baseline `20261015T164408.451014Z-sapling` and accepted current-binary llvmpipe/pixman
evidence `20261015T170854.285692Z-sapling` retain the same first 3.749 m tree and surrounding stand,
while removing exact vertical tier alignment. The final 1280x720 frame passes with 236 colour bins,
46.36 luminance deviation, 225.31 spatial contrast and an empty kernel delta. Native, focused Web
and Android-runtime gates pin the new juvenile-form diagnostic; physical-device rendering remains
unqualified.

The renewed 2026-10-15 soil-contact audit is evidenced by exact-final native capture
`build-consumer/software-captures/20261015T050551.611871Z-snags` (199 colour bins, 42.06 luminance
deviation, 189.17 spatial contrast) and Chrome/SwiftShader capture
`build/browser-software/20261015T050856.083534Z-chrome-snags`. Both inspected frames show the five
root spurs grounding the near trunk; WebGL passes 60 finite positive intervals, GL0 and live-context
checks with no kernel fault. The far mesh and render resource counts are unchanged.
ARM64 assemble/lint passes without emulator/KVM (24,191,747-byte APK, SHA-256
`6d2ab4119a61b3b623337b82c9a02d6d8cf0bda0910bfa48364a3c78abde5fc1`, 19,149,176-byte stripped
`libmain.so`); physical-device appearance remains unqualified.
