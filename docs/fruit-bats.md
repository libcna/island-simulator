# Fruit bats

## Close-anatomy and visible-roost audit — 2026-10-16

The continuing animal comparison found that a nearby roosting bat was an angular orange cluster:
body, head and muzzle were pointed bipyramids, eyes and ears were flat triangles, all fingers and
toes were cards, and the folded membrane had no edge thickness. Worse, every authored roost sat at
70% of the tree's nominal crown height, inside opaque leaf cores; neither the QA camera nor an
ordinary ground observer could inspect the animal.

The near form now uses separately rounded CPU-lit body, head, muzzle and fitted dark nose volumes.
Each side has a closed ear with a contrasting inner face, a fitted compound eye and highlight,
closed membrane edges, five solid supporting arm/finger rays, a lower leg and two articulated
gripping toes. Both spread and wrapped wing poses use the same 2,268-vertex bounded form inside a
2,300-vertex slot. All six still share the existing dynamic buffer and one draw; the far silhouette,
agent count, 4/2 daytime behavior split, animation, culling and activation tiers are unchanged.

Roost anchors now use 56% rather than 70% of nominal crown height. They remain deterministic,
separated inland points on their existing real host trees, but hang beneath the lower crown instead
of inside the leaf mass. The fixed roost camera approaches from the host's actual crown-outward
direction rather than an unrelated island-radial vector. Two camera/placement attempts were
visually rejected for trunk and leaf occlusion before the accepted composition.

Inspected exact-final native evidence is
`build-consumer/software-captures/20261016T171323.746653Z-bats` for flight (301 colour bins, 27.19
luminance deviation, 101.38 spatial contrast) and
`build-consumer/software-captures/20261016T171358.644484Z-bat-roost` for the inverted lower-crown
pose (127 bins, 31.54 deviation, 184.11 contrast). Both show the same fitted face, solid anatomy
and newly divided membranes; the roost frame also proves that the animal is visible beneath its
host crown.
The first Chrome attempt `20261016T171808.229228Z-chrome-bats` produced a valid flight frame and
live context but was rejected for a residual GL 1282 after its interval window. The exact rerun
`build/browser-software/20261016T172154.275601Z-chrome-bats` passes both flight and roost views with
60 finite positive intervals apiece, GL error zero, live contexts and clean kernel monitoring; both
matching frames were inspected. ARM64 assemble/lint passes without emulator/KVM (24,711,875-byte
APK, SHA-256 `3faa95fa613eee2b153819d65f65888241a2267f3e965b1ddfa106f9d620cf77`,
19,669,304-byte stripped `libmain.so`). Physical-device appearance remains unqualified.

## Implemented population and behaviour

The island has six deterministic fruit bats in a dedicated `FruitBatColony`; they are not bird
variants. Their six roosts are selected from real mature inland `JungleLayout` canopy crowns. The
fixed fair-weather daytime setting deliberately limits conspicuous activity: four bats hang inverted
at their roosts while exactly two make small, bounded flights around their home crowns. The update
uses the shared full, reduced and dormant distance tiers and fails dormant on non-finite input.

The renderer keeps the feature portable and bounded in one dynamic draw. The near form includes a
faceted fur-coloured torso and chest, head, short muzzle, pointed ears, eyes, inverted claws and
separately posed arm/finger strips supporting a segmented wing membrane. Three restrained
brown/russet palettes break up the colony. Distant bats use a reduced readable silhouette. This is a
purpose-built low-poly treatment, not a claim of photorealism or imported licensed artwork.

## Evidence

- Native Release flight view: `build-consumer/software-captures/20261015T055902.395475Z-bats`
  (301 colour bins, 49.90 luminance deviation, 229.78 spatial contrast).
- Native Release inverted-roost view:
  `build-consumer/software-captures/20261015T060830.767868Z-bat-roost` (97 colour bins, 26.52
  luminance deviation, 207.06 spatial contrast).
- Chrome/SwiftShader WebGL flight and roost views:
  `build/browser-software/20261015T061816.558810Z-chrome-bats`; both frames were inspected, the
  WebGL context remained live, GL error was zero and kernel monitoring was clean.
- `IslandFruitBatColonyTests` checks deterministic placement and motion, six separated inland
  canopy homes, the four-roost/two-flight daytime bound, terrain and home containment over 60
  seconds, activation-tier accounting and fail-closed invalid input.
- `IslandCoreTests`, `IslandWebPackageValidation` and `IslandCommandLineValidation` pass together
  with the focused colony test.
- ARM64 Android assemble/lint passes without starting an emulator or touching KVM. The debug APK is
  24,248,083 bytes with SHA-256
  `d8fb7e6751f7b8243b029e2b77ce745694fc8272ccddf6bb68572dff21feb47e`; its stripped
  `libmain.so` is 19,205,512 bytes. The deterministic world report exports six bats and validates
  with fingerprint `84ad3b7173bd40f0`.

The fixed cameras are review tools: `--fixed-view bats` follows one bounded daytime flight and
`--fixed-view bat-roost` frames one hanging animal. Runtime diagnostics report the six-agent
population, its 4/2 split, visible near/far counts and the one-draw budget.

## Remaining limits

The bats do not yet have a distinct positional voice, which is appropriate for their deliberately
quiet daytime state but could be revisited if a dusk state is ever added. Physical Android-device
appearance remains a platform qualification task; an APK build alone cannot prove it. The model is
recognisable but intentionally low-poly under the reduced visual-iteration budget.
