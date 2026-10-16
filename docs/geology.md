# Readable island geology

## Distance-stable coastal strata — 2026-10-16

The renewed overview/headland audit found that the existing close bedding, joints and grain
correctly filtered away with pixel footprint, but left the full seven-metre coastal face looking
like one grey curtain. The coastal-cliff branch now retains a separate broad geological rhythm:
gently warped metre-scale strata, restrained warmer mineral beds and irregular vertical drainage
streaks. The pattern is applied once within the rock colour and again at low amplitude after the
far outcrop blend, so atmospheric distance can soften it without erasing it. A stronger
intermediate was visually rejected because it looked artificially striped.

This is a material-only change. It adds one low-frequency value-noise sample plus bounded sine and
blend arithmetic only on steep rocky fragments; terrain geometry, collision, cells, textures and
draw count are unchanged. Terrain-material, rendered-terrain and core tests pass, together with
all 103 browser/capture harness tests. Inspected native baseline
`20261016T213030.275073Z-headland` and accepted evidence
`20261016T213833.545701Z-headland` preserve the beach and talus while making the face readable; the
accepted image passes at 429 colour bins, 44.23 luminance deviation and 232.21 spatial contrast.
Focused Chrome/SwiftShader evidence `20261016T214114.151756Z-chrome-cliffs` passes inspected east
and headland views with 60 finite positive intervals each, GL error zero, live contexts and kernel
`[]`. Android arm64 assembly/lint pass without an emulator; the 24,747,667-byte debug APK has
SHA-256 `f1298cc2e8f7096714284fb8cf1d067c0cf9b369f84e00bd62326b64a7502495` and contains a
19,705,096-byte `libmain.so`. Physical Android appearance and hardware cost remain unqualified.

Macro-geography item 19 is complete as a bounded composition, not a geological simulation.

The eastern windward lobe supplies a real approximately 7.5 m coastal rise and a short steep face.
The portable terrain material already projects low-frequency mineral breakup, bedding and vertical
joints in a face-oriented frame on steep slopes, so its exposed bedrock does not smear downward
from a top-down texture. Coastal rock potential keeps the headland predominantly salt-worn stone
while inland slope masks retain soil and turf where plausible.

The final missing erosion cue was talus. Six authored solid blocks now occupy two irregular groups
1.45--2.70 m inland at the foot of the eastern face. Axis scales vary strongly (1.15--3.35), yaw,
height and spacing are asymmetric, and partial silhouette overlap replaces the rejected first
evenly spaced row. They reuse the existing four near/far rock forms, portable weathered material,
cell/distance culling, contact shadows and collision grid. This adds six instances and proxies but
no mesh, texture, shader or draw call.

CPU coverage pins deterministic placement, high coastal rockiness, the narrow cliff-foot band,
solid collision, variant bounds and large-block proportions. The world report is valid at
fingerprint `fc91ff87bc4dd741`: 209 total rock placements, including 14 offshore outcrops and six
cliff talus blocks.

Native rejected row evidence is `build-consumer/software-captures/20261015T000705.311327Z-headland`.
Accepted and inspected llvmpipe evidence is
`build-consumer/software-captures/20261015T001032.141208Z-headland` (1280 by 720, 381 colour bins,
44.36 luminance deviation, 232.60 spatial contrast). Chrome/SwiftShader cliffs evidence
`build/browser-software/20261015T001249.174981Z-chrome-cliffs` passes both east and headland at 60
finite positive intervals, GL0, live contexts and no new kernel GPU fault; both images were
inspected. ARM64 assemble/lint passes through the two-worker wrapper (24,051,347 bytes, SHA-256
`6e99224ff26fd2007d122b003dcac14b23646fc9bcd60819d33e3729a0af615c`) without emulator/KVM.
The parallel non-display regression passes 62/63; its only failure is the unrelated supervisor
interruption timing case under load, which passes immediately in isolation in 2.80 seconds.

