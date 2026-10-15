# Fruit bats

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
