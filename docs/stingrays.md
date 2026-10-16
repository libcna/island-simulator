# Stingrays

## Population and habitat

Five deterministic, peaceful stingrays live over shallow sandy seabed. Their homes are selected
from authoritative `IslandTerrain` samples rather than being placed at arbitrary water coordinates:
the surface must be `ShallowSeabed`, between 0.72 and 6.2 metres below mean water, within the
configured near-shore band, sufficiently beach-like and not excessively rocky. Each animal stays
inside an 8.5–11.7 metre home range.

The CPU controller cycles through glide, forage and settle states. It steers a bounded local route,
turns home before leaving valid sand, gently retreats from a player inside 3.6 metres and keeps its
body between the seabed and water surface. A phase-driven bilateral disc wave is faster while
gliding. The shared full, reduced and dormant simulation tiers bound distant work and invalid input
fails dormant.

## Visual implementation

`StingrayRenderer` is a separate portable one-draw batch. The near animal uses a 32-section broad
diamond disc built from concentric bands: a shallow dorsal crown, thin dark margin and pale ventral
surface. It adds paired raised eyes and spiracles, restrained dorsal spots, a five-section tapering
tail and a small tail barb. The live disc edge follows the controller's wing wave and bank. Three
sandy palettes fit the habitat; a much smaller far silhouette retains the disc and tail. This is a
bounded low-poly model under the reduced visual-iteration budget, not photorealism.

## Verification

- `IslandStingrayGroupTests` simulates two identical colonies for 120 seconds and verifies exact
  determinism, five shallow-sand homes, bounded travel, actual motion, seabed and surface clearance,
  activation-tier accounting and invalid-input dormancy.
- Final inspected native Release/llvmpipe anatomy view:
  `build-consumer/software-captures/20261015T063452.530454Z-stingrays` (102 colour bins, 24.87
  luminance deviation, 167.49 spatial contrast). Three rejected/intermediate captures remain as
  evidence of the discarded coarse circular body and camera iterations.
- Inspected Chrome/SwiftShader WebGL view:
  `build/browser-software/20261015T065331.793599Z-chrome-stingrays`; 60 positive finite frame
  intervals, zero GL error, live context and no new kernel GPU fault.
- The deterministic world report exports five stingrays and validates with fingerprint
  `e21f8bd045bf6a25`. Focused colony, core, world-report, Web-package and command-line tests pass.
- ARM64 Android assemble/lint passes without starting an emulator or touching KVM. The debug APK is
  24,303,307 bytes with SHA-256
  `88191fb0217c57919202f7191f78cdb11e70a8f8581cf1fd6a493e39896a4366`; its stripped
  `libmain.so` is 19,260,736 bytes.

The fixed `--viewpoint stingrays` camera holds a symmetric anatomy pose for repeatable visual
comparison. Continuous undulation and route validity belong to the deterministic CPU test rather
than being inferred from one screenshot. A physical Android-device visual/performance run remains
open even after package-only verification.

## Curved diamond and raised-body pass (2026-10-15)

A new stingray/octopus/coconut-crab comparison selected the stingray as the weakest close coastal
animal. Despite the intended diamond construction, the previous power outline and almost coplanar
centre projected as a rounded, concentric oval carpet from swimming height. Its single horizontal
tail ribbon also became a nearly invisible line against the seabed.

The near outline is now a low-power superellipse with separate front and rear lengths, producing a
clear wedge nose and broad pointed wings. The flexible disc has a deeper central crown and receives
bounded per-facet daylight. A twelve-section shallow head/shoulder loft adds real height above it,
with the existing eyes and spiracles refitted to that surface. The dark margin is narrower and less
contrasting, and the five tail sections combine horizontal and vertical tapered faces. Matching far
coordinates retain the characteristic silhouette. The fixed QA camera is slightly higher and more
lateral to expose the crown and tail in one repeatable frame.

The close form uses exactly 846 vertices and the far tier 18, with a fixed 900-vertex per-animal
capacity. The population, habitat, controller, wing wave, banking, culling, LOD and single batch are
unchanged. Inspected baseline `20261015T155635.743252Z-stingrays` and accepted current-binary
llvmpipe/pixman evidence `20261015T160522.697524Z-stingrays` show a substantially stronger diamond
profile, raised centre and visible tapered tail. The final 1280x720 frame passes with 94 colour
bins, 26.66 luminance deviation, 164.60 spatial contrast and an empty kernel delta. Native and Web
gates pin the new profile and exact geometry diagnostics so a stale executable cannot satisfy the
focused view.

## Continuous tail and rounded-eye pass (2026-10-16)

A fresh current-binary audit found that the earlier volume pass had fixed the disc silhouette, but
its five tail sections still ended at single points. In profile this exposed gaps and read as a row
of detached arrowheads. The paired eyes also remained pointed pyramids, the spots were angular
diamonds and the pale underside made the outer edge appear thicker than intended.

Each tail section now joins matching horizontal and vertical start/end rings, producing one
continuous crossed-volume taper from the posterior disc through the barb. Eyes use base and middle
rings beneath a low cap, with separate inset pupils and fitted oblong spiracles. Six-facet circular
dorsal spots replace the diamonds; the dark rim is reduced from the outer five percent to the outer
2.5 percent, and a gentle feather drop thins the flexible wing edge. The central body loft gains
four sections and now takes its lower ring directly from the live disc surface.

The near tier grows from 846 to 1,086 vertices per animal, still below the explicit 1,100 capacity;
the 18-vertex far tier, five-agent population, one draw, simulation, habitat, culling and motion are
unchanged. Inspected same-camera native evidence is
`20261016T120257.632994Z-stingrays` before and `20261016T120650.062187Z-stingrays` after. The final
1280x720 frame passes at 98 colour bins, 27.33 luminance deviation and 166.33 spatial contrast.
Chrome 155/SwiftShader `20261016T120846.720660Z-chrome-stingrays` passes 60 positive intervals,
GL error zero, a live context and no new kernel GPU faults; the retained frame was inspected.
Three native/harness groups pass. WebGL compiles, and Android ARM64 assemble/lint pass without an
emulator or KVM change; the 24,668,667-byte APK contains only the 19,626,096-byte arm64 library.
