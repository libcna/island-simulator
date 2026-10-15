# Wildlife habitat integration

Plan item 18 connects existing wildlife to readable places instead of adding another population.

## Land habitats

- Thirty beach crabs remain grouped into six deterministic five-animal colonies. Each colony is
  selected beside real driftwood or dry wrack on a low-slope sandy beach section. It now exposes a
  grounded, irregular dark burrow with a small displaced-sand rim derived from the five home ranges.
- Eighteen butterflies remain split across three sunny clearing habitats. Each habitat now has one
  seven-flower nectar patch. Patch bases use a local terrain gradient so stems do not form a flat
  floating bouquet on sloped ground.
- Every butterfly owns a deterministic blossom target. `NectarHover` turns toward that target;
  rest, free flutter and mild player avoidance remain unchanged.
- Ten birds follow the coastal belt, eight use canopy-height interior routes and six use high
  island/seabird routes. Existing landing rules match canopy birds to branches and shore birds to
  rocks or the wreck; high birds do not perch.

The flower and burrow vertices are appended to the existing culled butterfly/crab dynamic batches.
The feature adds no texture, renderer, GPU buffer submission or draw call.

## Aquatic habitats

The audited aquatic code was already spatial rather than decorative:

- eight schools (48 fish) score and continuously revalidate route centres with
  `IslandEnvironmentField::AquaticHabitatSuitability`, shared with the reef and seagrass layout;
- all five turtle homes and moving states use the same shallow reef/seagrass suitability field;
- tests reject routes that leave the field, seabed clearance or water-depth envelope.

## Evidence

- `IslandCoreTests` pins flower-patch count, deterministic ground placement, butterfly-home
  ownership, burrow count, beach grounding and all five nearby colony members.
- `IslandWorldReport` includes all patch and burrow data in the deterministic fingerprint and emits
  explicit `butterflyFlowerPatches` and `crabBurrows` inventory fields. Fingerprint
  `ae3382b4e9eca621` is valid, and the correctly filtered non-display suite passes 60/60.
- Accepted native llvmpipe views:
  `build-consumer/software-captures/20261015T005059.201550Z-butterflies` and
  `build-consumer/software-captures/20261015T005104.406312Z-crabs`.
- Chrome/SwiftShader
  `build/browser-software/20261015T010147.912276Z-chrome-butterflies` and
  `build/browser-software/20261015T010437.480963Z-chrome-crabs` each pass 60 finite positive
  intervals, GL0, live contexts and clean kernel monitoring. Initial and separated-time images are
  distinct and the accepted initial images were inspected.
- ARM64 `assembleDebug` and `lintDebug` pass through the two-worker wrapper using the pinned local
  Sharp Runtime. The 24,062,091-byte APK has SHA-256
  `c29f89582f2d1e48586e62c08c43b22d18600851f5fad67dfd669a665516c753` and contains a
  19,019,520-byte `libmain.so`. No Android emulator or KVM state was touched.
