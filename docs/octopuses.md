# Octopuses

Four peaceful procedural octopuses now occupy deterministic reef and rocky-shallow dens. Placement
requires authoritative shallow seabed between 0.9 and 7 metres deep and scores reef suitability,
coastal rockiness and low beach suitability. Each animal remains within a 4.8–6.4 metre home.

The controller cycles through den, crawl, forage and short swim states, returns before leaving valid
habitat, hides when the observer comes within 2.7 metres and maintains both seabed and water-surface
clearance. Mantle pulse and independent arm phase remain renderer-independent. Full, reduced and
dormant distance tiers bound work; invalid input fails dormant.

The one-draw near model uses rounded multi-ring mantle/head volumes, paired eyes and eight
continuous tapered ribbon arms with independent curl plus sparse pale sucker marks. Three muted
reef palettes and a reduced far silhouette keep the feature portable. The rejected first native
capture used a pointed five-ring mantle and disconnected triangular arms. After replacing those
with an eight-ring/sixteen-side mantle and continuous quads, the inspected current native evidence
is `build-consumer/software-captures/20261015T070526.758025Z-octopuses` (104 colour bins, 31.04
luminance deviation, 174.53 spatial contrast).

`IslandOctopusGroupTests` runs two exact colonies for 120 seconds and verifies deterministic
movement, four shallow dens, bounded homes, actual travel, seabed/surface clearance, activation
accounting and invalid-input dormancy. The world report exports four animals and validates with
fingerprint `f662f6e9ac0f9c45`. WebAssembly compilation passes. ARM64 Android assemble/lint passes
without an emulator or KVM; the 24,358,979-byte APK has SHA-256
`eee224db4323c5528744db071f59c430f996736e8d1a48158b21f9ffc7112211` and its stripped
`libmain.so` is 19,316,408 bytes. A focused WebGL image audit and another facial/material pass
remain open; this document does not claim those gates.
