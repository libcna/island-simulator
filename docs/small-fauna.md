# Small fauna

Four bounded stages of macro-ecosystem item 17 now add eight deterministic inland geckos, nine
stream/wetland dragonflies, ten beach hermit crabs and six coconut crabs. This is not completion of
the whole small-fauna list: skinks, additional shorebirds, fruit bats, cicadas and beetles remain
candidates for later stages.

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
