# Tropical parrots and fruit doves

## Ground-dove close-anatomy audit (2026-10-16)

The continuing animal audit found that the planted fruit dove still read as a flat green wedge:
its folded wings were broad closed plates, its feet were two double-sided triangles, the head had
a coarse pointed outline, and its split tail was too long for the otherwise compact body. The
ground state now substitutes two low rounded folded-wing volumes that follow the breast and back.
Its lofted body is fuller through the shoulder and belly, while the tail is shorter and narrower.
The head uses a twelve-sided, six-band ellipsoid with a fitted restrained orbital ring and pupil
instead of the prior coarse shared head shell.

Each planted foot now has a solid tapered lower leg and three individually tapered toes in a warm
bare-skin colour. Flight, ground approach/forage/depart animation, terrain contact, population,
placement, culling, LOD and the single dynamic bird draw remain unchanged. Generic birds remain at
822 vertices in flight and 846 with feet; the richest ground-dove state is 1,602 vertices. The
conservative all-near buffer therefore grows from 20,304 to 38,448 vertices, still submitted only
for visible near birds and still in one existing draw.

The inspected exact-final native evidence is
`build-consumer/software-captures/20261016T163528.881908Z-ground-doves` (225 colour bins, 31.25
luminance deviation, 156.86 spatial contrast). It visibly shows the rounded folded wing, shorter
tail, planted toes and smaller fitted eye instead of the previous plate-and-triangle silhouette.
Focused Chrome/SwiftShader evidence
`build/browser-software/20261016T163955.588807Z-chrome-birds` passes the flying parrot and ground
dove views with 60 finite positive intervals each, GL error zero, live contexts and clean kernel
monitoring; both matching frames were inspected. ARM64 assemble/lint passes without emulator/KVM
(24,699,603-byte APK, SHA-256
`450941c845574b3dd408386a1ef916a89e77d37427addc89a9dd0805952352d4`, 19,657,032-byte stripped
`libmain.so`). Physical-device appearance remains unqualified.

## Rounded close profile and solid flight surfaces (2026-10-16)

The next whole-island audit selected the airborne parrot as the weakest close animal: its head was
nearly as large as its torso, each eye was a dark triangular slash, the pale beak was one broad
wedge, and edge-on wing/tail sheets reduced to long coloured lines. Both body and head lofts now
use sixteen radial sides. The head is smaller and remains blended into the chest; only the
camera-facing fitted six-facet eye is submitted, preventing the far eye from escaping the
three-quarter silhouette. A two-stage four-sided beak transitions from the face through a rounded
cere to a down-curved tip instead of terminating in one triangle.

Each near wing is now a closed six-point swept volume with separate upper, lower and edge light,
and the tail is two staggered closed feathers rather than one coincident sheet. The same geometry
folds for perching and ground foraging. A flying near bird is 822 vertices / 274 triangles and the
feet-bearing folded maximum is 846 / 282; the unchanged 12-vertex far silhouette remains. The
conservative all-24-near buffer is 20,304 vertices or 324,864 bytes. Population, route/perch/ground
state machines, CPU wing pose, culling, LOD threshold and one dynamic draw are unchanged.

Inspected native baseline `20261016T011403.533449Z-birds` and accepted flying evidence
`20261016T012736.422384Z-birds` show the smaller head, round fitted eye, curved hook and split solid
flight surfaces. The final frame passes with 346 colour bins, 38.19 luminance deviation and 230.69
spatial contrast. Folded-state evidence `20261016T012812.489441Z-ground-doves` passes with
208 / 31.16 / 156.85. Both kernel deltas are empty. Focused Chrome/SwiftShader evidence
`20261016T012926.788171Z-chrome-birds` passes both inspected views with 60 finite positive
intervals each, GL error zero, live contexts and an empty kernel delta. Native, Web lifecycle and
Android-runtime guards require the new profile diagnostic.

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
