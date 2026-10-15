# Tropical parrots and fruit doves

## Scope

The 24-agent bird flock still consists of ten coastal, eight canopy and six high-route birds.
The canopy family is now explicitly split into four parrots and four tropical fruit doves.
This is a species pass inside the existing system, not a population or renderer expansion.

`BirdSpecies` is deterministic state alongside route family. Parrots use two green tropical
palettes with red or gold heads, hooked deep beaks and long narrow tails. Fruit doves use two
restrained green palettes with pink or blue-grey heads, fine beaks and shorter rounded tails.
The existing closed lofted body, head rings, lateral eyes, layered wings, feet, CPU pose, flight,
banking, glide and perching paths are retained. Every bird still joins one dynamic vertex buffer
and one draw. No texture, material, draw call, agent or per-frame allocation was added.

The close `birds` QA view now targets agent 11, a flying parrot, and freezes only the QA simulation
clock. Targeting seeded perched agent 10 put the camera inside its canopy crown; allowing agent 11
to simulate let it leave the fixed camera before capture. Ordinary interactive birds remain fully
animated.

## Validation

- `IslandCoreTests` passes and asserts the deterministic four-parrot/four-fruit-dove split in
  addition to the existing route, terrain-clearance, long-run and activation invariants.
- Exact-final native llvmpipe evidence
  `build-consumer/software-captures/20261015T052957.655925Z-birds` passes with 366 colour bins,
  37.93 luminance deviation and 230.52 spatial contrast. The inspected image shows the green body,
  warm underside, red faceted head, fitted pale hook and elongated tail.
- A new narrow browser `birds` scope avoids rerunning the multi-view wildlife/foliage matrix.
  Chrome/SwiftShader evidence `build/browser-software/20261015T054015.321390Z-chrome-birds`
  passes 60 finite positive intervals, GL0, live context and clean kernel monitoring; its image
  was inspected and matches native.
- ARM64 assemble/lint passes without emulator/KVM (24,196,227-byte APK, SHA-256
  `abec02b0a7961528218681b2c7a0378ad69be5f432ca01da578b2093eb47bf21`, 19,153,656-byte stripped
  `libmain.so`). Physical-device appearance and performance remain unqualified.

## Remaining limits

The procedural forms remain deliberately low-poly at close inspection. Both species reuse the two
generic positional bird-call phrases; recognisable parrot vocalisations are still open. Fruit doves
currently fly and perch in canopy like their route family. A distinct ground-dove walk, forage and
ground retreat state remains open, as do moving-camera LOD and distant-aliasing review.
