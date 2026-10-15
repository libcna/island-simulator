# Offshore dolphin pod

## Scope

The island now has four peaceful dolphins beyond the western reef. `DolphinPod` owns a
deterministic bounded route and three-state `Cruise -> Surface/Dive -> Cruise` cycle for each
animal. Homes remain 34--105 m offshore, over seabed at least 3 m below the surface, and every
animal remains within its 17--21 m home radius. The existing wildlife activation policy reduces
or stops simulation outside 190/520 m rather than updating an invisible ocean-wide population.

This is an original procedural implementation. It does not use a downloaded model, texture,
animation or sound recording.

## Form and rendering

`DolphinRenderer` emits all visible animals into one dynamic `BasicEffect` batch. The close form
has a nineteen-station, twenty-four-sided tapered fusiform body, pale underside, short closed
twelve-facet beak below the melon,
two-facet highlighted eyes, shaped swept pectoral fins, a rear-curved dorsal fin and broad
horizontal flukes. The dorsal, both pectorals and both flukes are thin closed volumes with separate
faces and darker perimeter edges rather than coincident zero-thickness sheets. A two-part mouth
crease and two-facet blowhole retain cetacean head cues. The
body follows smoothed dive pitch and turn bank, while the tail stock bends vertically through the
horizontal flukes' up/down stroke. Three
restrained grey-blue palettes share the existing CPU depth tint and moving shallow-sun response.

The fixed budgets are 3,144 vertices for a close animal and 15 vertices for a distant silhouette.
Four animals still produce at most one draw call. There
is no dolphin texture, separate shadow, collision proxy or audio voice.

## Closed melon and smooth body lighting — 2026-10-14

The next whole-island audit after the monkey pass selected the dolphin because its close body
still showed the thirteen longitudinal rings, a visibly cut front section, broad flat colour
facets and angular fin outlines. The accepted body uses nineteen progressively spaced stations
and twenty-four radial sides from the tail stock through the rounded forehead. A small forward
cap closes the melon while the separately modelled beak stays buried into the lower face.

Countershading is now interpolated at body vertices and receives analytic radial-normal sunlight
instead of one brightness value per triangle. This keeps the smooth skin visually distinct from
the intentionally firmer edges of the fins. The dorsal and both pectorals grow from five to seven
outline points, while each horizontal fluke uses a six-point swept outline. Existing pitch, bank,
vertical tail stroke, depth tint, LOD threshold, four-agent population and single dynamic draw are
unchanged. The close cost is 3,144 vertices / 1,048 triangles, or 201,216 bytes if all four
dolphins are simultaneously near.

Inspected native llvmpipe evidence `20261014T023119.008608Z-dolphins` passes at 1280x720 with
238 colour bins, 28.37 luminance deviation, 147.92 spatial contrast and an empty kernel delta.
Chrome/SwiftShader evidence `20261014T023401.772283Z-chrome-dolphins` passes with two inspected
different-time poses, 60 finite intervals, `glError=0`, an intact context and no new kernel GPU
fault. The underwater view still limits tiny facial-cue readability, and this pass does not add a
skin texture, wake or breach behavior.

## Verification

- `IslandDolphinPodTests` simulates 18,000 frames (600 seconds), reconstructs a second pod in
  lockstep, and checks deterministic state, finite motion, shore/depth/home bounds, surface and
  dive travel, pitch in both directions, both tail extremes, all three behaviours and activation
  tiers.
- The deterministic world report records the four states and home ranges, validates their
  habitat, and currently has fingerprint `049597fca36b232e`.
- The contained llvmpipe/pixman capture
  `build-consumer/software-captures/20261013T032529.689761Z-dolphins` passed image, runtime,
  OpenGL and kernel-delta checks. The inspected 1280x720 image contains one close dolphin in one
  draw call. The closer `--viewpoint dolphins` camera follows subject translation without freezing
  pitch, bank, fluke beat or behaviour.
- Inspected focused Chrome/SwiftShader evidence
  `build/browser-software/20261013T032653.774852Z-chrome-dolphins` retains the animal in two
  different-hash frames over 60 positive intervals with GL error zero, an intact context and an
  empty kernel delta.

The current closed-fin native pass is
`build-consumer/software-captures/20261013T112745.515773Z-dolphins`: 240 colour bins, 28.30
luminance deviation, 149.09 spatial contrast and an empty kernel delta. The first
`20261013T112616.939803Z-dolphins` tuning image retained an excessively dark fin treatment and was
not accepted; the final pass uses the body grey-blue on broad faces and the darker tone only around
their solid edges. Current Android execution remains pending.

Focused Chrome/SwiftShader evidence `20261013T113206.342870Z-chrome-dolphins` passes and both
inspected frames retain the animal in distinct pitch/tail poses. The PNG SHA-256 identities differ,
all 60 intervals are finite, `glError` is zero, the context remains live and the kernel delta is
empty. This qualifies current Web execution; Android, continuous interactive observation and
hardware performance remain open.

## Smoother cetacean silhouette and countershading — 2026-10-13

A fresh current-binary baseline `20261013T175140.727008Z-dolphins` made the remaining visual
weaknesses unambiguous: eight ten-sided body rings left a long flat dorsal slab, the melon-to-beak
transition was abrupt, and oversized triangular pectorals reinforced a fish-like silhouette. The
close body now uses thirteen longitudinal stations and sixteen radial sides. A modest upward shift
through the final head stations forms a separate rounded melon above the short closed rostrum,
while a narrower maximum girth and five additional peduncle/body transitions produce a smoother,
more slender cetacean profile.

The dorsal and both pectorals are now five-point closed volumes rather than four-point wedges. The
dorsal is lower and swept, and the pectorals are shorter, narrower and pulled rearward. Dorsal,
flank and ventral colour selection now follows vertical surface position rather than an asymmetric
side test; the three palettes use a smaller dark/light contrast and a restrained grey-blue belly.
Existing pitch, bank, tail-beat timing, depth tint, moving sunlight, 175 m LOD threshold and one
dynamic draw remain unchanged. The renderer also corrects the old fish-like lateral tail bend:
both near and far tiers now use a vertical tail-stock bend and up/down horizontal-fluke stroke.

The near budget rises from 708 vertices / 236 triangles to 1,476 / 492. Even the deliberately
conservative all-four-near allocation is only 5,904 `VertexPositionColor` vertices, or 94,464
bytes. There is no new texture, shader, draw, simulation step, state, asset or world placement;
the current world fingerprint remains `301ea795731c0732`.

Final inspected native llvmpipe evidence `20261013T181811.002065Z-dolphins` passes at 1280x720
with 235 colour bins, 28.33 luminance deviation, 146.81 spatial contrast and an empty kernel
delta. Compared with the baseline, the live subject has a smoother arched back, a distinct
forehead above its beak, a narrower trunk, smaller swept pectorals and less banded pigmentation.

Final Chrome/SwiftShader evidence `20261013T182032.131304Z-chrome-dolphins` passes with two
inspected different-hash swimming poses, 60 finite intervals, `glError=0`, an intact context and
no new kernel GPU fault. ARM64 debug packaging and lint pass without launching or modifying the
emulator; Release and Debug 600-second pod regressions pass. Current Android runtime appearance,
continuous interactive observation, skin microtexture and physical-device performance remain
open.

## Facial readability audit — 2026-10-14

The cross-species audit found that the close broadside dolphin still lost its most important
cetacean cue: the narrow beak collapsed into a nearly flat plate, while the eye mark read as an
oversized black diamond. The rostrum now starts deeper beneath the melon, tapers through a larger
twelve-sided base and retains thickness at its rounded forward cap. Each eye is a much smaller
six-facet oval with a restrained highlight. This adds only 24 close-tier vertices, changing the
fixed budget from 3,144 to 3,168 vertices; population, dynamic batching, LOD, motion and one draw
remain unchanged.

Native llvmpipe evidence `20261014T175547.884290Z-dolphins` passes at 1280 by 720 with 120 colour
bins, 44.72 luminance deviation, 210.79 spatial contrast and no captured kernel fault. The image
was inspected at original resolution. Focused WebGL and Android qualification are grouped with the
larger secret-bunker integration pass.

## Known limitations

The close animal remains a deliberately economical procedural model; its mouth crease, blowhole
and skin surface are geometric/vertex-colour cues rather than a filtered skin material and can
receive further art refinement.
There is no breach, leap, wake, exhalation plume, social pod interaction or dolphin sound. Actual
Android execution, continuous interactive observation, and measured integrated frame cost remain
open. These limits keep both the explicit aquatic-animal visual-polish item and the dolphin backlog
entry in progress rather than complete.

## Skin and broadside profile pass — 2026-10-15

The current aquatic contact sheet selected the dolphin as weaker than the shark, turtle and crab.
Although its motion and major anatomy were present, the broadside body remained one nearly uniform
grey mass, the beak still read as a short plate and the dorsal/pectoral outlines retained rigid
triangular cues. An initial low-frequency colour pass was rejected because one colour sampled per
body band exposed the nineteen longitudinal rings as visible stripes.

The accepted implementation evaluates colour independently at the rear and forward end of every
body band and interpolates it over both triangles. A subtle variant-specific tonal variation and
pale swept lower-flank transition break up the flat side without becoming spots or painted bands;
the fitted rostrum uses the same continuous pigment. The beak is slightly longer and rounder, and
the existing seven-point dorsal and pectorals receive lower, softer rear-swept outlines. The fixed
3,168/15 near/far vertex inventory, four animals, one draw, dynamic-buffer capacity, habitat,
simulation, LOD, pitch, bank and vertical fluke stroke are unchanged.

Inspected native evidence `20261015T151802.766412Z-dolphins` passes at 1280x720 with 135 colour
bins, 45.15 luminance deviation, 209.70 spatial contrast and an empty kernel delta. The retained
failed tuning evidence `20261015T151345.591315Z-dolphins` documents the rejected banded result;
the current frame restores a continuous body surface while retaining the more readable pale flank,
longer rostrum and softer fin profile.

## Slimmer bottlenose proportions and rooted fins — 2026-10-15

The current cross-animal audit selected the dolphin because its closed nineteen-station body still
read as a broad grey whale and its low dorsal and uniformly thin control surfaces collapsed toward
cards. The accepted pass reduces the widest midbody sections by roughly 5--8 percent without
changing axial stations or the fitted melon. Its closed twelve-sided rostrum is narrower and
slightly longer. The pale ventral palette now reaches farther onto the lower flank, preserving
countershading through the shallow-water tint.

The dorsal becomes a taller swept falcate outline. Dorsal, paired pectorals and paired flukes retain
their existing closed polygon topology and exact triangle count, but their two faces now use
per-vertex thickness: broad buried roots taper toward thin tips and outer edges. The four-agent pod,
cruise/surface/dive simulation, bank/pitch, vertical tail-stock bend, horizontal flukes, LOD,
3,168/15 near/far vertices and one draw are unchanged.

`IslandDolphinPodTests` and `IslandCoreTests` pass. Baseline
`20261015T193337.232080Z-dolphins` and accepted current-binary
`20261015T195011.255544Z-dolphins` were inspected side by side. The accepted 1280x720 frame passes
with 144 colour bins, 45.17 luminance deviation, 213.05 spatial contrast and an empty kernel delta.
Portable gates require the new proportion diagnostic; consolidated Web/Android qualification
remains separate from this native visual pass.
