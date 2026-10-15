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
