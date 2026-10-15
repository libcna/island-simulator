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

The close `birds` QA view targets agent 11, a flying parrot, and freezes only the QA simulation
clock. Targeting seeded perched agent 10 put the camera inside its canopy crown; allowing agent 11
to simulate let it leave the fixed camera before capture. Ordinary interactive birds remain fully
animated. A second `ground-doves` view frames agent 14 in the open east clearing. Its longer planted
legs, folded wings and terrain-following body height keep it visibly standing rather than lying on
the soil.

Fruit doves now alternate canopy flight/perches with a bounded ground cycle: approach a qualified
dry, moderately flat interior patch, walk and forage inside a 2.4 m radius, then flap back to the
canopy route. At most two birds may be outside free flight at once. Parrots retain the shared sparse
call scheduler and source budget, but a nearby parrot now selects one of two original synthesized
lower rough squawk phrases instead of the generic high chirps.

## Validation

- `IslandCoreTests` passes and asserts the deterministic four-parrot/four-fruit-dove split in
  addition to approach/forage/depart transitions, planted terrain contact, folded walking wings,
  bounded routes and deterministic parrot PCM.
- Exact-final native llvmpipe evidence
  `build-consumer/software-captures/20261015T052957.655925Z-birds` passes with 366 colour bins,
  37.93 luminance deviation and 230.52 spatial contrast. The inspected image shows the green body,
  warm underside, red faceted head, fitted pale hook and elongated tail.
- Final native ground-dove evidence is
  `build-consumer/software-captures/20261015T074238.202267Z-ground-doves` (188 colour bins, 31.83
  luminance deviation, 156.87 spatial contrast); the image was inspected after rejecting lower,
  obscured poses.
- The narrow browser `birds` scope now covers both the flying parrot and ground dove.
  Chrome/SwiftShader evidence `build/browser-software/20261015T074421.451978Z-chrome-birds`
  passes 60 finite positive intervals for each view, GL0, live contexts and clean kernel monitoring;
  both images were inspected.
- ARM64 assemble/lint passes without emulator/KVM (24,373,091-byte APK, SHA-256
  `e2008520ac5441e323cd5aed01b5d36fbe770da4212c6e8d8a8b99590571ab6d`, 19,330,520-byte stripped
  `libmain.so`). Physical-device appearance and performance remain unqualified.

## Remaining limits

The procedural forms remain deliberately low-poly at close inspection. Calls are original synthetic
approximations, not recordings, and still share the existing single transient wildlife voice.
Moving-camera LOD, distant aliasing and physical-device audio/appearance remain release-wide review.
