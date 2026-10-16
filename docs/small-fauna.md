# Small fauna

Eleven bounded stages complete macro-ecosystem item 17 with eight deterministic inland geckos, eight
open-ground skinks, nine stream/wetland dragonflies, ten stream/wetland frogs, ten beach hermit
crabs, six coconut crabs, twelve field mice, eight rats, twenty-four flies in four swarms and
sixty-four ants in four trails, twelve trunk-hosted cicadas and fifteen leaf-litter beetles.

## Beetles

Fifteen harmless tropical ground beetles occupy three deterministic inland leaf-litter colonies at
the two clearing edges and one organic forest-floor patch. They rest, forage within 3.0--3.5 m
homes and hide in litter when a player comes within 1.5 m, then emerge after a bounded delay. Every
position resamples authoritative terrain height and normal. Full simulation ends at 22 m, reduced
simulation at 48 m and the common activation policy sleeps more distant agents.

The QA-readable close form uses separate paired domed six-station elytra with a dark centre seam,
a fitted pronotum and head, lateral eyes, paired two-section antennae and six articulated legs.
Four jewel-green, copper, blue and bronze palettes keep the species readable in deep ground shade.
This deliberately remains bounded low-poly fauna rather than a photoreal hero model. Geometry
appends to the existing small-fauna dynamic buffer and draw, adding no renderer, texture, material
switch, GPU buffer, audio source or draw call.

The deterministic CPU test runs three simulated minutes and covers exact count/colonies, grounded
terrain contact, bounded homes, repeatability, rest/forage/hide states, player-triggered visibility,
invalid input and all simulation tiers. The first two native iterations were rejected for angular
rock-like elytra, long thin legs and a flat top-down composition. Accepted inspected native evidence
is `build-consumer/software-captures/20261015T111916.737720Z-beetles`: 169 colour bins, 24.48
luminance deviation and 151.13 spatial contrast. Chrome/SwiftShader evidence is
`build/browser-software/20261015T112152.398929Z-chrome-beetles`: 60 finite positive intervals,
GL0, live context, two captured frames and no new kernel GPU fault. Native release plus the active
non-display range pass 71/71 tests. ARM64 assemble/lint passes without emulator/KVM; the
24,527,523-byte APK has SHA-256
`e36638a22d44b849015d57e5e73f7cbfc900c4d95faffd5aaa0a3ddc19dbcf02` and packages a
19,484,952-byte `libmain.so`. Physical-device appearance remains final acceptance work.

## Cicadas

Twelve harmless deterministic cicadas use real non-mangrove broadleaf hosts selected from the
current `JungleLayout`. Each home follows its rendered trunk's scale, radius and surface frame.
Animals rest against bark, enter sparse call intervals and make short bounded flights around their
own crown before returning. Full simulation ends at 28 m, reduced simulation at 65 m and the shared
wildlife policy sleeps more distant agents.

The close form uses a broad head, paired highlighted eyes, tapered thorax and segmented abdomen,
six legs and two roof-folded veined wings. It is intentionally a compact angular low-poly detail,
not a photoreal hero asset. Cicada geometry appends to the existing small-fauna dynamic buffer and
draw, adding no renderer, GPU buffer, texture, material switch or draw call. Two original
procedural 1.4-second pulsed buzz clips use the existing positional wildlife voice. Scheduling is
restricted to calling, non-dormant animals inside 48 m and uses a sparse deterministic cooldown.

The dedicated CPU test runs three simulated minutes and covers exact count, determinism, finite
normalized frames, actual host contact, rest/call/flight cycling, bounded excursion and all three
distance tiers. Audio tests cover deterministic/distinct PCM with silent endpoints, scheduling,
real playback, range and mute behavior. The inspected native llvmpipe capture is
`build-consumer/software-captures/20261015T105838.525179Z-cicadas`: 117 colour bins, 20.62
luminance deviation and 95.24 spatial contrast. Chrome/SwiftShader evidence is
`build/browser-software/20261015T110742.564194Z-chrome-cicadas`: 60 finite positive intervals,
GL0, live context, two captured frames and no new kernel GPU fault. Native release build plus the
active non-display range pass 70/70 tests. ARM64 assemble/lint passes with the two-worker wrapper
and no emulator/KVM; the 24,506,323-byte APK has SHA-256
`f4f08230d715d87dfd943c1070a7764c904956a2c03440bb8e3bd55e6bdeed09` and packages a
19,463,752-byte `libmain.so`. Physical-device appearance remains final acceptance work.

## Ants

Four deterministic close-range trails each contain sixteen harmless workers. Three run across
qualified inland soil at the primary footpath, west-clearing edge and seasonal wetland. The fourth
uses a real non-mangrove `JungleLayout` broadleaf selected near the east clearing: its radius and
vertical span follow the actual canopy variant and instance scale, so the workers sit on the
rendered trunk rather than an unrelated cylinder. Full simulation ends at 16 m, reduced simulation
at 34 m, and the common wildlife activation policy sleeps more distant agents.

Workers travel in both directions, briefly pause and return carrying visible leaf fragments. Their
pose supplies a normalized surface normal and travel tangent in three dimensions. Ground routes
resample terrain height and normal; the trunk route climbs while slowly wrapping around the bark.
The roughly 3 cm QA-readable close form uses closed eight-sided head, thorax and abdomen volumes,
six articulated two-section legs and two antennae. It appends to the existing small-fauna dynamic
buffer and draw. There is no new renderer, texture, material switch, GPU buffer or draw call.

The dedicated deterministic CPU test runs three simulated minutes and covers the exact 4x16
layout, finite normalized frames, terrain contact, actual trunk ascent, walking/carrying/paused
states and full/reduced/dormant tiers. The inspected final native llvmpipe capture is
`build-consumer/software-captures/20261015T102927.642859Z-ants`: 195 colour bins, 30.93 luminance
deviation and 96.34 spatial contrast. Chrome/SwiftShader evidence is
`build/browser-software/20261015T103518.572889Z-chrome-ants`: 60 finite positive intervals, GL0,
a live context, two distinct frames and no new kernel GPU fault. ARM64 assemble/lint passes with
the two-worker wrapper and without launching an emulator or touching KVM. The APK is 24,473,043
bytes with SHA-256 `618d1eb6a9bcd706c87790c897a1685e547f15363f8813742d8f295989680005`;
its packaged `libmain.so` is 19,430,472 bytes. Physical-device appearance remains final acceptance
work.

## Flies

Four deterministic inland swarms each contain six harmless close-range flies. The authored homes
cover the seasonal wetland, both clearings and one organic forest-floor patch; every initial animal
requires dry land at least 20 m from the coast. Full simulation ends at 22 m and reduced simulation
at 48 m, after which the shared wildlife activation policy sleeps the agents. Individuals alternate
hover and orbit states, make brief darts and react with a dart when the player comes within 1.2 m.

The roughly 4.5 cm close model has a tapered dark body, separate head, small dark-red eyes, two
rear-swept beating wings and six legs. All flies append to the existing small-fauna vertex buffer
and draw; the stage adds no renderer, texture, material switch or draw call. The deterministic CPU
test covers the exact 4x6 layout, bounded swarm motion, all three behaviours, repeatability and
non-finite input containment.

The first native capture was rejected because triangular propeller-like wings, oversized diamond
eyes and a dark wetland composition made the insect read poorly. The corrected exact-final native
llvmpipe evidence is `build-consumer/software-captures/20261015T091929.300514Z-flies`: 265 colour
bins, 34.66 luminance deviation and 159.88 spatial contrast. Its 1280x720 image was inspected at
original resolution. Chrome/SwiftShader evidence is
`build/browser-software/20261015T092031.533084Z-chrome-flies`: 60 finite positive intervals,
GL0, a live context, distinct separated-time frames, clean kernel monitoring and an inspected
initial image. The updated Web package is 11,431,365 bytes of Wasm, 2,230,701 bytes of data and
280,439 bytes of JavaScript. ARM64 `assembleDebug` and `lintDebug` pass with two workers and no
emulator/KVM. The APK is 24,454,987 bytes with SHA-256
`b5c481c91cbd55a512fbd516c730f5432dc04ca13c55afa8e537c744b7f18f0a`; its stripped
`libmain.so` is 19,412,416 bytes. Physical-device appearance remains final acceptance work.

## Mice and rats

One deterministic rodent simulation supplies twelve field mice and eight larger rats without a
new renderer or draw call. Each animal owns a 4.2--6.9 m inland home on dry interior, coastal or
rocky ground, remains more than 28 m from the shore and rejects steep terrain. The shared wildlife
activation policy reduces or sleeps distant updates. Ordinary agents alternate rest and slow
foraging. A player within 2.5 m of a mouse or 3.3 m of a rat triggers a short fast scurry followed
by a visibility-faded hiding interval before the animal returns.

Both species use lofted low bodies and pointed heads, articulated short legs and paws, highlighted
eyes, a dark nose, six whiskers and a volumetric three-section tail. Mice have warmer coats and
large round ears. Rats use darker palettes, smaller relative ears, a longer narrower muzzle and a
longer silhouette. Breathing/sniffing and diagonal stride motion are computed on the CPU. All
twenty append to the existing butterfly/gecko/skink/dragonfly/frog dynamic buffer and one draw;
hidden animals append no geometry.

The 2026-10-15 follow-up close audit rejected the flat alternating-colour ear fans, diamond eyes and
nose, stick legs and abrupt body/head join. Ears now use smaller twelve-segment outer rims around a
uniform recessed inner bowl. Eyes and nose are compact multi-ring volumes, a tapered neck bridges
the chest into the head, and legs, paws and all three tail sections use closed tapered geometry.
Mice retain larger relative ears and the shorter muzzle; rats retain the longer narrow face, larger
body and tail. The shared dynamic-buffer budget was also raised to its conservative all-visible
geometry maximum rather than relying on normal culling.

The dedicated CPU test covers exact species counts, deterministic construction and long-running
simulation, valid dry habitats, home bounds, rest/forage states, player-triggered scurry/hide and
non-finite input containment. The exact-final native llvmpipe captures are
`build-consumer/software-captures/20261015T083214.554735Z-mice` (187 colour bins, 22.00 luminance
deviation and 109.47 spatial contrast) and
`build-consumer/software-captures/20261015T083425.592907Z-rats` (66 colour bins, 25.62 luminance
deviation and 82.42 spatial contrast). Both 1280x720 frames were inspected at original resolution;
the first overly tight cameras and a steep rat habitat were rejected and revised. WebGL and ARM64
qualification also pass. Chrome/SwiftShader evidence is
`build/browser-software/20261015T084209.265876Z-chrome-rodents`: both views complete 60 finite
positive intervals with GL0, live contexts, distinct separated-time frames and clean kernel
monitoring; both initial frames were inspected. The Web package is 11,410,304 bytes of Wasm,
2,230,701 bytes of data and 280,439 bytes of JavaScript. ARM64 `assembleDebug` and `lintDebug`
pass with two workers and without launching an emulator or touching KVM. The APK is 24,440,739
bytes with SHA-256 `62f90fc83e90364de79ec507e3a1de6d8d1bc88ae44251f2aeaab2f42daf25be`;
its stripped `libmain.so` is 19,398,168 bytes. Physical Android appearance remains final-device work.

Accepted follow-up native evidence is
`build-consumer/software-captures/20261015T112747.332768Z-mice` and
`build-consumer/software-captures/20261015T112751.367465Z-rats`; both were inspected at original
resolution. Chrome/SwiftShader two-view evidence
`build/browser-software/20261015T112922.278599Z-chrome-rodents` passes 60 finite positive intervals
per view, GL0, live contexts and clean kernel monitoring. ARM64 assemble/lint passes without
emulator/KVM. The current APK is 24,531,251 bytes with SHA-256
`b046291bc9814f05bd19a37bc94231e7b2768b5800ec3a6681c2f1e9e778ce3c` and packages a
19,488,680-byte `libmain.so`.

## Frogs

Ten deterministic frogs occupy eight compact bank homes along qualified permanent-stream sections
and two homes in the seasonal wetland. Placement uses `IslandEnvironmentField::moisture`, requires
more than 18 m of shore clearance, moderate slope and valid dry ground, and keeps every animal in a
4.8--6.5 m range. They alternate between still breathing and short hops; a player within 2.8 m
triggers a faster two-or-more-hop retreat. The whole body follows a bounded sine arc while folded
hind legs extend, and the shared full/reduced/dormant policy limits distant simulation.

The close procedural animal has a broad low lofted head and body, attached green eye mounds with
small gold irises, dark pupils and highlights, paired dorsolateral folds, three restrained dorsal
spots, stout folded hind legs, shorter forelegs and splayed toes. All ten append to the existing
butterfly/gecko/skink/dragonfly dynamic buffer and single draw, adding no renderer, GPU buffer,
texture, material switch or draw call. CPU tests cover deterministic construction, wet habitat,
grounding, home bounds, invalid input, rest/hop/flee states and player reaction. The world report
fingerprints all states and homes and validates as `e989290219afffa4`.

Nearby resting or ordinarily hopping frogs also use two deterministic original 1.1-second low
croak phrases. A sparse scheduler selects a finite non-dormant animal within 42 m, excludes fleeing
frogs, spatializes the voice at the animal and enforces a 7--15 second local cooldown. It reuses the
single pooled wildlife transient, so mute state, underwater attenuation and the existing maximum
of six simultaneously playing sources remain intact. The runtime dummy-audio regression proves
actual playback, source-budget bounds, mute suppression and distance rejection; core tests prove
the PCM variants are deterministic, distinct and silent at both endpoints.

Current evidence:

- exact-final native llvmpipe capture
  `build-consumer/software-captures/20261015T043811.339015Z-frogs` (193 colour bins, 21.71
  luminance deviation and 91.86 spatial contrast; inspected at original resolution);
- Chrome/SwiftShader
  `build/browser-software/20261015T044850.271176Z-chrome-frogs`, 60 finite positive intervals,
  GL0, live context, two distinct separated-time frames and no new kernel GPU fault;
- ARM64 assemble/lint passes without emulator/KVM; the APK is 24,190,467 bytes with SHA-256
  `15d65a61c8d34063fff82ac2efa99eb4ae005ebaf368eee7a6ae29fc40f1459e` and its stripped
  `libmain.so` is 19,147,896 bytes.
- after adding the frog voice, native Release and WebAssembly builds pass, the runtime dummy-audio
  test reaches actual frog playback, and ARM64 assemble/lint passes without emulator/KVM. The APK
  is 24,377,971 bytes with SHA-256
  `d354d33a0588d047c0c60675029f1b7abb172b89e61b2faea9b89ef81c6bd1b2`; its stripped
  `libmain.so` is 19,335,400 bytes.

The first rejected native view exposed gold whole-eye volumes and threadlike rear legs. The second
confirmed the corrected attached eyes and stronger legs but viewed the face from behind. The
accepted front-quarter view uses the exact final geometry and camera.

Each gecko owns a compact 5.2--6.1 m home range in one of the two clearings, near the seasonal
wetland, or on suitable inland ground. Placement rejects beach proximity, steep slopes and invalid
surface classes. The three-state simulation rests, briefly scurries away when the player comes
within 2.7 m, then disappears into cover before returning. It uses the shared wildlife activation
policy, including reduced and dormant update tiers.

The close model is deliberately small (roughly 37--47 cm including the tail) and has an eight-sided
lofted trunk and flattened head, tapered four-section tail, four articulated splayed legs, three
toes per foot, raised amber eyes with dark pupils, dorsal markings and four restrained habitat
palettes. All eight animals append to the existing butterfly dynamic vertex buffer and draw, so the
stage adds no renderer, GPU buffer, material switch or draw call. Hidden animals contribute no
vertices. The fixed `geckos` camera holds one animal visible for anatomy review; ordinary play and
the deterministic CPU tests retain the full rest/flee/hide cycle.

The world report fingerprints all initial states and home ranges, reports exactly eight geckos and
currently validates as `c4874b5d2f4ed411`.
Core tests cover deterministic construction and long simulation, inland grounding, home bounds,
finite invalid-input handling, all three behaviours, visibility fade, player-triggered flight and
complete activation-tier accounting.

Current evidence:

- native 1280x720 llvmpipe capture:
  `build-consumer/software-captures/20261015T013548.592879Z-geckos` (281 colour bins, 19.41
  luminance deviation and 73.04 spatial contrast; inspected at full resolution);
- Chrome/SwiftShader:
  `build/browser-software/20261015T013200.753923Z-chrome-geckos`, 60 finite positive intervals,
  GL0, live context, two distinct separated-time frames and no new kernel GPU fault;
- ARM64 assemble and lint pass without launching an emulator or touching KVM.
- the filtered non-display regression passes 60/60; its five display-producing tests remain
  intentionally disabled in this run.

The first native attempt exposed an endlessly reset flee timer and the test rejected it. Two later
browser attempts are retained as rejected evidence: the first lacked the second-frame harness path;
the second let the inspected animal complete its hide cycle before the slow software renderer took
the screenshot. Neither is accepted as visual proof.

### Gecko solid-anatomy pass — 2026-10-16

A renewed deer/gecko/rat comparison selected the gecko after the deer repair. Its body and head
were closed lofts, but all four legs, twelve digits and the segmented tail were still double-sided
flat cards. From ordinary elevated viewing distance they collapsed into hairline strokes, while the
eyes sat inside the broad head and five large triangular dorsal marks reinforced a toy silhouette.

The body now has six unequal sections and a fuller shoulder profile. A longer six-section wedge
head narrows from broad rear jaw corners into a compact blunt snout. Every upper/lower limb, digit
and all five curved tail sections are closed six-sided tapered tubes. Each digit terminates in a
small closed expanded adhesive pad, giving twelve readable gecko toes. Amber eye volumes are fitted
to the upper lateral head surface with dark pupils, and six smaller eight-sided oval dorsal spots
replace the large triangles.

All eight animals remain in the existing shared small-fauna dynamic buffer and single draw, with
the same scale, habitat, rest/scurry/hide simulation and visibility fade. Conservative gecko
capacity rises from 7,200 to 18,400 vertices across the complete population, adding 179,200 bytes
at the 16-byte vertex format; the current detailed animal uses 2,112 vertices inside its 2,300
slot bound. Inspected native baseline `20261016T142237.642668Z-geckos` and accepted
`build-consumer/software-captures/20261016T145148.912962Z-geckos` show solid legs/tail, separated
toe pads, surface eyes and compact spots. Accepted evidence passes at 296 colour bins, 19.99
luminance deviation, 72.65 spatial contrast and kernel `[]`.

`IslandCoreTests` and all 43 browser/capture harness tests pass. Focused Chrome/SwiftShader
`build/browser-software/20261016T145535.589704Z-chrome-geckos` provides two distinct separated-time
frames and passes 60 finite positive intervals, GL0, a live context and kernel `[]`. WebAssembly
builds. Android ARM64 assemble/lint passes without emulator/KVM; the APK is 24,683,675 bytes and
the packaged `libmain.so` is 19,641,104 bytes. Physical-device appearance remains open.

## Dragonflies

Nine deterministic dragonflies occupy three groups of three: one over the seasonal wetland and two
beside qualified moist sections of the stream. Placement requires dry land, more than 12 m inland,
a slope below 0.55 radians and shared environment-field moisture above 0.34. Each owns an 8.5--10.5
m home range and alternates between hover, patrol and a short dart; a player within 2.8 m triggers
the dart. Ground-relative altitude is continuously resampled, and the same full/reduced/dormant
wildlife policy bounds simulation work.

The close procedural model is deliberately about 9--10.5 cm in body length and uses a lofted
thorax, compound highlighted eyes, seven tapering abdominal segments, four independently shaped
wings with fitted veins, and six swept legs. Bank affects the complete local frame and a rapid wing
phase animates the wing plane. The nine animals join the existing butterfly/gecko dynamic buffer and
single draw, so this stage adds no renderer, material switch, GPU buffer or draw call.

Deterministic core tests cover construction, habitat moisture, altitude, home bounds, all three
behaviours, invalid input, player-triggered escape and activation-tier accounting. The world report
fingerprints all initial states and homes, reports exactly nine dragonflies and validates as
`2754a52691f4b662` after the coconut-crab stage below.

Current dragonfly evidence:

- native 1280x720 llvmpipe capture:
  `build-consumer/software-captures/20261015T022433.392934Z-dragonflies` (175 colour bins, 18.68
  luminance deviation and 38.59 spatial contrast; inspected at full resolution);
- Chrome/SwiftShader:
  `build/browser-software/20261015T022628.199799Z-chrome-dragonflies`, 60 finite positive
  intervals, GL0, live context, two distinct separated-time frames and no new kernel GPU fault;
- ARM64 assemble/lint passes without launching an emulator or touching KVM; the APK is 24,107,467
  bytes with SHA-256 `14239a5c904f4f97be5589638eabcd84fa73244ec83846e732ec7176ff961a23`
  and its stripped `libmain.so` is 19,064,896 bytes.

Earlier native captures are retained as rejected evidence: they exposed an oversized aircraft-like
silhouette, then a rear-quarter composition that hid the long abdomen, and finally a side view that
collapsed the four wings into lines. The accepted elevated broadside uses the exact final binary.

## Hermit crabs

Ten deterministic hermit crabs occupy compact 3.8--4.7 m home ranges on sheltered sandy coast.
Placement requires terrain at or above the wash edge, a signed shore distance of 2--14 m and a
slope below 0.34 radians. They pause, forage slowly and retreat with their shells when a player
comes within 3.5 m. Ground height is resampled continuously and the shared wildlife activation
policy reduces distant simulation.

Each close model has an asymmetric five-ring gastropod shell, a dark aperture and connected raised
spiral, exposed body, projecting eye stalks, moving antennae, six jointed walking legs and unequal
claws. Ten shell/limb palettes are derived from four restrained variants. They append to the
existing beach-crab dynamic buffer and single draw, adding no renderer, GPU buffer, material switch
or draw call.

Core tests cover deterministic construction, sandy habitat and home bounds, grounding, pause and
forage states, finite invalid-input handling, player-triggered retreat and simulation-tier
accounting. The world report fingerprints all ten initial animals and homes and validates as
`6feb901b29f26f0c`.

Current evidence:

- native 1280x720 llvmpipe capture:
  `build-consumer/software-captures/20261015T024132.187658Z-hermit-crabs` (251 colour bins, 26.86
  luminance deviation and 141.61 spatial contrast; inspected at full resolution);
- Chrome/SwiftShader:
  `build/browser-software/20261015T024746.729975Z-chrome-hermit-crabs`, 60 finite positive
  intervals, GL0, live context, two distinct separated-time frames and no new kernel GPU fault;
- ARM64 assemble/lint passes without launching an emulator or touching KVM; the APK is 24,128,011
  bytes with SHA-256 `20468080cb9b924288cac60aeb58a50895da93cf54cb2f8353397f5a9f8c5f91`
  and its stripped `libmain.so` is 19,085,440 bytes.

The rejected first capture placed the camera behind an overly round shell, making the body read as
disconnected pieces. The second introduced a conical shell and front camera but retained visible
gaps. The accepted exact-final-binary capture adds a connected exposed body, forearms and stronger
leg roots and uses a clean front three-quarter beach composition.

## Coconut crabs

Six deterministic coconut crabs occupy compact 7.5--9.1 m home ranges on dry coastal palm-zone
ground 22--58 m inland. Placement rejects beach surfaces, steep slopes and low terrain. They rest,
forage beneath the palms and retreat from a player within 5.5 m, continuously resampling terrain
height and using the shared full/reduced/dormant wildlife policy.

The close model has a broad armoured carapace, compact segmented terrestrial abdomen, robust
jointed walking legs, stalked eyes and massive unequal palms and claws. It extends each standard
near-crab mesh within the existing beach-crab dynamic buffer and single draw, so this stage adds no
renderer, GPU buffer, material switch or draw call.

Core tests cover deterministic construction, qualified dry habitat, grounding and home bounds,
rest/forage/retreat behaviour, invalid input, player reaction and activation-tier accounting. The
world report fingerprints all six initial animals and homes and validates as `2754a52691f4b662`.

Current evidence:

- native 1280x720 llvmpipe capture:
  `build-consumer/software-captures/20261015T030303.964102Z-coconut-crabs` (271 colour bins, 44.50
  luminance deviation and 218.26 spatial contrast; inspected at full resolution);
- Chrome/SwiftShader:
  `build/browser-software/20261015T030704.036881Z-chrome-coconut-crabs`, 60 finite positive
  intervals, GL0, live context, two distinct separated-time frames and no new kernel GPU fault;
- ARM64 assemble/lint passes without launching an emulator or touching KVM; the APK is 24,146,555
  bytes with SHA-256 `25d754f9303bc1fa3c716a9745c09438f7b0c6253987d8cedf453630f67a3f9a`
  and its stripped `libmain.so` is 19,103,984 bytes.

Two rejected native captures document the correction from a lobster-like long abdomen and weak
side silhouette to the accepted compact front-facing terrestrial animal.

### Elevated stance pass (2026-10-15)

A renewed stingray/octopus/coconut-crab matrix found that the current close coconut crab still
projected as a broad object nearly flush with the ground. Its inherited beach-crab carapace and
leg roots were too low for a large terrestrial species, and the straight transverse abdomen marks
resembled stacked boards.

The renderer still builds on the qualified detailed beach-crab anatomy, but only the coconut-crab
vertices are now transformed in the animal's local frame: the body is compacted laterally while
all positive-height shell, claw and leg joints are lifted. Feet at and below the terrain plane are
not raised, producing a planted high-jointed stance without changing simulation grounding. The
rear armour is narrower and deeper, and every transverse plate is split around a raised centre to
follow the dorsal arch. Existing unequal palms, fingers, eyes, marks and animated leg/claw state
remain attached.

The small number of extra plate facets remains inside the fixed 1,800-vertex-per-animal coconut
budget, with the same shared crab buffer and one draw. Population, habitat, controller, culling and
simulation tiers are unchanged. Inspected baseline `20261015T155703.579232Z-coconut-crabs` and
accepted current-binary llvmpipe/pixman evidence `20261015T162015.996548Z-coconut-crabs` show the
higher body and articulated stance. The final 1280x720 frame passes with 282 colour bins, 44.35
luminance deviation, 216.92 spatial contrast and an empty kernel delta. Native and Web gates now
pin the elevated-profile diagnostic.

### Rounded chela pass (2026-10-15)

A later current-binary animal audit found that the two inherited octahedral palms still read as
pyramids in a close view. The accepted replacement is applied inside the shared near-crab builder,
only for coconut crabs: each palm is a closed ten-sided, three-ring ellipsoidal volume with analytic
vertex light, unequal major/minor dimensions and a flattened vertical profile. The original two
solid pincer fingers remain attached but use thicker separated bent centre lines for this species.
Ordinary beach-crab palms are byte-for-byte on their previous path.

The former 1,734 vertices become an exact runtime-checked 1,998 vertices per close coconut crab,
adding 88 triangles per animal and 1,188 allocated worst-case vertices (about 19 KiB) for all six.
There is no new draw, texture, material or simulation work. Inspected baseline
`20261015T195442.123468Z-coconut-crabs` and accepted llvmpipe/pixman evidence
`20261015T200537.538658Z-coconut-crabs` show the two pyramids replaced by lower rounded unequal
chelae with visible pincer gaps. The accepted frame passes at 1280x720 with 285 colour bins, 44.34
luminance deviation, 216.91 spatial contrast and an empty kernel delta. Two intermediate renders
were visually rejected: the first retained a hidden inherited pyramid above the new volume and the
second produced palms that were too spherical. Native capture and Web gates pin the new rounded-
claw diagnostic. Focused Chrome/SwiftShader evidence
`20261015T200935.227024Z-chrome-coconut-crabs` passes 60 finite intervals, two distinct inspected
frames, GL error zero, a live context and an empty final kernel delta. Android packaging remains
grouped with later integration.

### Domed blue-grey terrestrial armour (2026-10-16)

The next whole-animal comparison still found the coconut crab too similar to a flat orange beach
crab. Its shared shell is now compacted further laterally, positive-height joints rise while toes
remain on the authored support plane, and the front gains a restrained upward inclination. A new
closed smooth-lit cephalothorax volume overlaps the inherited shell and every existing leg root,
replacing the long flat centre bar with a deep terrestrial armour dome.

Four deterministic cool blue-grey, violet-brown, slate and warm-umber families are blended with
the original per-part lighting, preserving dark eyes, joints and pincer separation rather than
painting the entire animal one colour. The two existing unequal palms and the new body dome now
use five contour rings instead of three; the palms are slightly narrower while retaining their
two separated solid fingers. Near geometry rises from 1,998 to an exact runtime-checked 2,538
vertices per animal. All six still share the existing opaque crab buffer and draw, with no texture,
shader, simulation, habitat or population change.

The first recolour candidate `20261015T225113.385755Z-coconut-crabs` was rejected for flat uniform
blue shading, and `20261015T225336.090689Z-coconut-crabs` was retained only as an intermediate
three-ring body/claw study. Accepted native evidence `20261015T231514.688471Z-coconut-crabs`
passes at 1280x720 with 282 colour bins, 44.19 luminance deviation, 216.93 spatial contrast and an
empty kernel delta. Focused Chrome/SwiftShader
`20261015T231815.216785Z-chrome-coconut-crabs` passes two distinct inspected frames, 60 finite
intervals, GL error zero, a live context and an empty final kernel delta.

## Skinks

Eight deterministic diurnal skinks occupy 6.4--7.5 m homes around clearings, a wetland edge and
two open inland patches. Their homes are separated from the gecko homes by at least four metres.
They bask, forage slowly and flee from a player within 3.1 m while remaining on dry, moderate-slope
ground and using the shared full/reduced/dormant simulation policy.

The close model is a distinct smooth-bodied lizard with a narrow elongated trunk and head, short
articulated limbs and toes, a long five-section tapered tail, paired longitudinal stripes, small
highlighted eyes and restrained breathing motion. It joins the existing butterfly/gecko dynamic
buffer and single draw, adding no renderer, GPU buffer, material switch or draw call.

Core tests cover deterministic construction and long simulation, habitat and ground contact, home
bounds and gecko separation, invalid input, bask/forage/flee behaviour, player reaction and
activation tiers. The world report fingerprints all eight animals and homes and validates as
`be62b2929cd5657f`.

Current evidence:

- native 1280x720 llvmpipe capture:
  `build-consumer/software-captures/20261015T031940.818537Z-skinks` (259 colour bins, 34.54
  luminance deviation and 161.63 spatial contrast; inspected at full resolution);
- Chrome/SwiftShader:
  `build/browser-software/20261015T032541.058370Z-chrome-skinks`, 60 finite positive intervals,
  GL0, live context, two distinct separated-time frames and no new kernel GPU fault;
- ARM64 assemble/lint passes without launching an emulator or touching KVM; the APK is 24,166,699
  bytes with SHA-256 `ece98364f55a7c0566fabf2662bff944b0018bf12185661c4334855cd7343c5e`
  and its stripped `libmain.so` is 19,124,128 bytes.

The first rejected capture exposed a skink and gecko occupying the same home. The accepted capture
uses the separated final habitats and shows one unobstructed striped animal.
