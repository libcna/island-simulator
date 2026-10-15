# Octopuses

Four peaceful procedural octopuses now occupy deterministic reef and rocky-shallow dens. Placement
requires authoritative shallow seabed between 0.9 and 7 metres deep and scores reef suitability,
coastal rockiness and low beach suitability. Each animal remains within a 4.8–6.4 metre home.

The controller cycles through den, crawl, forage and short swim states, returns before leaving valid
habitat, hides when the observer comes within 2.7 metres and maintains both seabed and water-surface
clearance. Mantle pulse and independent arm phase remain renderer-independent. Full, reduced and
dormant distance tiers bound work; invalid input fails dormant.

The one-draw near model uses rounded multi-ring mantle/head volumes, compact amber eyes with
horizontal pupils and eight continuous tapered diamond-section arms with independent curl plus sparse pale sucker marks. Three muted
reef palettes and a reduced far silhouette keep the feature portable. The rejected first native
capture used a pointed five-ring mantle and disconnected triangular arms. After replacing those
with an eight-ring/sixteen-side mantle and continuous volumes, the inspected final native evidence
is `build-consumer/software-captures/20261015T071621.426076Z-octopuses` (106 colour bins, 31.20
luminance deviation, 176.62 spatial contrast). An intermediate oversized eye pass was rejected as
too cartoon-like before the smaller horizontal-pupil treatment was accepted.

`IslandOctopusGroupTests` runs two exact colonies for 120 seconds and verifies deterministic
movement, four shallow dens, bounded homes, actual travel, seabed/surface clearance, activation
accounting and invalid-input dormancy. The world report exports four animals and validates with
fingerprint `f662f6e9ac0f9c45`. The focused Chrome/SwiftShader audit
`build/browser-software/20261015T071824.635104Z-chrome-octopuses` passes 60 live intervals with
a clean WebGL context and an inspected matching image. ARM64 Android assemble/lint passes without
an emulator or KVM; the refreshed 24,363,299-byte APK has SHA-256
`00895083b4cff088b89e1a91be621602e3cd9421f151d63a923c6accfc21ddac` and its stripped
`libmain.so` is 19,320,728 bytes. Physical Android rendering remains part of the release-wide
device qualification rather than this species gate.
