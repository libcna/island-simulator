# Grounded helicopter

## Flight-control cockpit pass

The previous cabin contained two seats, a flat panel and small rectangular lamps, but the
pilot station remained too empty at first-person distance. Each seat now has a two-segment
tubular cyclic with a transverse rubber grip, a conventionally left-mounted three-segment
collective and a pair of linked rudder pedals. Four recessed sixteen-sided round gauges use
metal bezels, dark faces and individually angled needles; six small annunciators remain below
them. The interior QA camera now looks down the centre aisle into the accessible footwell so
these controls can be judged rather than hidden behind the seat backs.

The additions stay in the existing opaque batch and raise the complete model from 1,788 to
2,104 triangles. They do not add a texture, shader, draw, collision obstacle, flight state or
working interaction, and all four authored entry corridors remain unchanged. The controls are
visual cockpit anatomy only; the parked helicopter still cannot fly.

Final inspected native llvmpipe evidence is
`20261013T201600.553835Z-helicopter` (301 colour bins, 55.12 luminance deviation, 235.22 spatial
contrast) and `20261013T201518.621138Z-helicopter-inside` (167 bins, 25.15 deviation, 110.19
contrast). Both pass with an empty kernel delta. An earlier rear-cabin view
`20261013T201141.581953Z-helicopter-inside` passed technically but hid the controls behind the
seat backs and was not accepted as control evidence.

Focused Chrome/SwiftShader `20261013T201903.851542Z-chrome-helicopter` passes exterior and
interior views with 60 finite intervals each, `glError=0`, intact contexts and an empty final
kernel delta. All four separated-time images were inspected and retain the gauges, cyclics,
pedals, centre collective, glazing and open doors. Release and Debug layout/asset tests, the
33 browser-runner tests, Web syntax gate and Android ARM64 assemble/lint pass without an emulator
or KVM. Free-walk control clearance, physical Android rendering, richer cockpit mechanics and
interactive sitting remain open.

## Native second visual pass

The island now places one deterministic grounded utility helicopter at
`(-50.2571, 48.7260)`, near the centre of the island and well inland. The site owns a
6.20 m vegetation/grass reservation around the 5.65 m main-rotor radius. Placement rejects
steep or high-relief ground, the stream and every existing solid obstacle. Four skid contacts
sample `RenderedTerrainSurface`; the body is not allowed to float or use one guessed centre
height.

The model is original project procedural geometry, not a downloaded asset. An opaque batch plus
a depth-read translucent-glass batch contain 1,788 triangles: a multi-ring fuselage, sealed glazed nose,
windshield mullion, tail boom and empennage, stationary main and tail rotors, terrain-following
skids, four outward-open framed and glazed door panels, two threshold/ramp levels, a supported cabin floor, two pilot seats,
paired cyclics, instrument lamps and a rear bench. Provenance is recorded in
`ASSET_LICENSES.md` and `assets/manifest.json`.

## Traversal and collision

The helicopter deliberately has no flight state and no rotor controls. It is also not represented
by one enclosing circular player obstacle. `HelicopterPlacement` instead provides swept capsule
tests for the fuselage shell, centre posts, seats and rear bench, plus floor/threshold support.
The paired pilot and rear door bands remain open on both sides. `PlayerMotion` queries those
surfaces during ordinary walking, so the player can step into either pilot footwell or the rear
cabin and stand at the authored floor height.

Wildlife uses a separate conservative 3.2 m avoidance circle. This keeps animals out of the
airframe without making the player-facing world collision grid seal the doors.

`island_helicopter_tests` checks deterministic placement, shore/centre limits, rotor clearance,
all four terrain-exact skid contacts, solid nose and centre posts, floor support and real
controller entry through all four door paths. The deterministic world report exposes the complete
placement, entry flags and skid contacts; current fingerprint is `76af8e15925526d1`.

## Visual evidence and limits

The fixed cameras are `--viewpoint helicopter` and `--viewpoint helicopter-inside`. Native
llvmpipe/pixman exterior and rear-cabin captures pass the image-diversity, diagnostic and kernel
fault gates. They establish a recognizable grounded silhouette, open side access, cockpit seats,
panel, glazing, skids and stationary rotors. This is a reduced-detail pass for the 100-hour
plan, not a photorealism claim.

The second visual pass raises the cockpit cross-section from fourteen to twenty sides, adds two
rounding stations to remove the pointed windshield cone, separates genuinely translucent glazing,
adds paired sloping windshield frames and builds an eight-link rotor hub around the mast. Through
the glass, the panel and seats are now visible from outside; from the rear cabin the player has a
real forward view instead of facing an opaque blue shell.

The third bounded silhouette pass replaces the broad square-section landing rails with eight-sided
tubes that follow all four retained terrain contacts and rise through two segments at each nose.
It also replaces the box-shaped horizontal stabilizer with a thin swept, tapered tailplane. Both
changes stay in the existing opaque batch, so the complete model remains two draws and adds only
120 triangles. The collision and floor plan are unchanged.

The inspected current native evidence is
`build-consumer/software-captures/20261013T042032.089144Z-helicopter` (296 colour bins,
54.31 luminance deviation, 230.01 spatial contrast) and
`build-consumer/software-captures/20261013T042051.967138Z-helicopter-inside` (255 bins,
30.87 deviation, 131.98 contrast). Both report two helicopter draws, the 1,316-triangle marker
and an empty new-kernel-fault delta. Release and Debug `IslandHelicopterLayoutTests` still pass all
four door routes, floor support, skid contacts and collision boundaries.

Focused Web evidence `build/browser-software/20261013T042639.045892Z-chrome-helicopter` contains
inspected exterior and interior images with different hashes, 60 finite intervals per view,
ANGLE SwiftShader, GL error zero, intact contexts and no new kernel GPU fault.

Current third-pass evidence is
`build-consumer/software-captures/20261013T070234.719775Z-helicopter` and
`build-consumer/software-captures/20261013T070301.958014Z-helicopter-inside`. The corresponding
focused Web run is `build/browser-software/20261013T070806.833263Z-chrome-helicopter`; both inspected
images are distinct and both views again pass 60 finite intervals, ANGLE SwiftShader, GL error zero,
intact contexts and an empty new-kernel-fault delta.

## Generated surface-material atlas

The fourth visual pass replaces flat vertex colour as the only surface cue with one original,
deterministically generated128-square mipmapped grayscale atlas. Four padded quadrants carry
low-contrast oxidized matte-paint mottling and rare fine scuffs, directional brushed metal,
unequal warp/weft seat cloth, and fine rubber/interior stipple. Geometry chooses a stable
normal-dominant local projection; glass and instrument/navigation lights sample a restrained
constant atlas point so the new texture does not contaminate transparency or lamp colours.
Vertex colour still supplies material identity and the existing CPU fixed-sun response.

The complete eight-level RGBA8 payload is87,380bytes. Total runtime texture accounting is now
9,742,120bytes inside a9,762,336-byte quality-profile ceiling. The helicopter remains1,436
triangles and exactly two draws; there is no new shader, draw, vertex, collision or movement cost.
Release and Debug core/layout tests pass, including all four door-entry routes.

Inspected native evidence `20261013T123312.897455Z-helicopter` and
`20261013T123336.026355Z-helicopter-inside` passes with293/258 colour bins,54.66/30.80
luminance deviation,231.64/134.58 spatial contrast and empty kernel deltas. The exterior retains
restrained weathering; the cabin visibly separates woven seat stripes, stippled floor/rubber and
metal. Chrome/SwiftShader `20261013T123738.786800Z-chrome-helicopter` passes exterior and interior
views,60 finite intervals each, GL0, intact contexts and kernel[]; both Web images were inspected
and show no atlas bleed through the glazing.

## Visible open-door pass

The fifth bounded pass replaces the four visually empty side openings with four actual doors held
open outside the cabin: the pilot doors hinge forward and the rear doors hinge aft. Each door has
an opaque lower skin, upper cap, translucent window, volumetric perimeter and belt frames, plus a
small metal handle. They reuse the existing opaque and depth-read glass batches, so the complete
model remains exactly two draws while rising from 1,436 to 1,748 triangles. The panels sit outside
the four tested perpendicular approach corridors; collision, thresholds, walkable floor and all
four controller entry routes are unchanged.

Fresh inspected native evidence is
`build-consumer/software-captures/20261013T153007.168360Z-helicopter` (303 colour bins,
54.97 luminance deviation, 234.95 spatial contrast) and
`build-consumer/software-captures/20261013T153247.865624Z-helicopter-inside` (256 colour bins,
30.43 luminance deviation, 137.19 spatial contrast). Both pass the diagnostic, image-diversity and
empty-kernel-delta gates. Focused Chrome/SwiftShader evidence
`build/browser-software/20261013T153637.307230Z-chrome-helicopter` passes both inspected views with
60 finite intervals each, GL error zero, intact contexts and no new kernel GPU fault. The ARM64
debug APK and Android lint also build successfully without starting an emulator.

## Closed main-rotor blade pass

The former main rotor used one infinitely thin quad per blade and therefore collapsed into a
wire-like line from the exterior eye height. Each of the four stationary blades is now a closed
tapered prism with separate upper/lower faces and visible leading, trailing, root and tip edges.
The authored 5.65m radius and hub remain fixed; a restrained 68mm total depth and a small planform
sweep make the parked rotor readable without implying motion or flight controls. Forty triangles
raise the complete model from 1,748 to 1,788 triangles, still in the same two opaque/glass draws and
with no collision, placement, traversal, texture or simulation change.

Inspected native llvmpipe evidence `20261013T191917.508759Z-helicopter` passes at 1280 by 720 with
304 colour bins, luminance deviation 55.11, spatial contrast 234.93 and an empty kernel delta. The
focused Chrome/SwiftShader run `20261013T192258.296430Z-chrome-helicopter` passes exterior and
interior views with 60 finite intervals each, GL error zero, intact contexts and no new kernel GPU
fault; all four separated-time images have distinct hashes and were inspected.
Release and Debug layout/asset tests, all 33 browser-runner tests and Android ARM64 assemble/lint
pass without an emulator or KVM.

## Rounded canopy and rotor-head silhouette — 2026-10-14

The cockpit shell now uses32 circumferential sides and15 progressively tapered longitudinal
stations instead of20 sides and9 widely spaced rings. Alternating dark/light windshield wedges
were removed; consistent glazing pigment lets the denser curved normals describe the canopy.
A raised tapered engine cowling feeds the paired exhausts. The parked rotor gains a mast cap,
four grip housings, pitch links and separate muted-yellow tip sections without adding motion.
The complete procedural model is2780 triangles in the existing two opaque/glass draws; floor,
doors, entry corridors, collision and the non-flying contract are unchanged.

Inspected llvmpipe evidence `20261014T004417.223126Z-helicopter` passes at1280x720 with297
colour bins,55.08 luminance deviation,237.29 spatial contrast and no captured kernel fault.
The nose/canopy contour is smoother while open doors and structural frames retain hard edges.
The matching cockpit view `20261014T011306.734195Z-helicopter-inside` passes with170 colour
bins,22.98 deviation and102.10 contrast and confirms that both pilot controls and the rear
passenger opening remain usable after the exterior reshaping.

Open work includes richer body anatomy and small mechanical detail, improved interior and
exterior lighting, door animation, explicit sitting interaction, live free-walk visual review,
physical performance measurement, and Android runtime/capture qualification.

## Airframe and cabin anatomy pass — 2026-10-14

The next close review found that the technical canopy and open access were sound, but the side
silhouette still read as an assembly of rectangular frames. The body now has a restrained rounded
chin fairing and a buried tail-root fairing. The tail boom grows from four broad eight-sided bands
to six twelve-sided taper stations, with maintenance seams, a dorsal rise, paired engine-intake
lips and a cleaner transition into the vertical fin. Door glazing retains the four unobstructed
entry corridors but gains raked belt and roof edges instead of a pure rectangle.

Inside, rounded seat cushions and headrests, crossed webbing harnesses, a centre pedestal,
glareshield and overhead console provide human and mechanical scale around the existing gauges,
cyclics, collectives and pedals. These parts reuse the material atlas and the same opaque/glass
batches. Collision, floor support, doors and the deliberately disabled flight state are unchanged.
Native Release evidence `20261014T174852.988461Z-helicopter` and
`20261014T174906.588260Z-helicopter-inside` passed and was inspected; the four-door controller
test also passes. WebGL and Android compile/runtime qualification for this latest anatomy pass
remain to be run with the broader bunker integration.

## Continuous curved-surface lighting — 2026-10-15

A renewed same-camera exterior/interior audit found that geometry density was no longer the main
cause of the angular cabin. Lighting was still constant across each generated triangle, so the
thirty-two-sided canopy, tapered cowling, twelve-sided tail boom and rounded fairings exposed broad
polygon bands despite having sufficient silhouette resolution.

The fuselage, nose closure, engine cowling and tail boom now derive analytic normals from the local
loft slope and elliptical cross-section. Ellipsoidal chin and tail-root fairings, seat cushions,
headrests and intake lips use their corresponding analytic surface normals. Texture coordinates
retain face-local projection so this shading change does not stretch the generated worn-paint,
metal, upholstery or rubber atlas. It adds no geometry, texture, shader, collision or draw cost:
the helicopter remains 2,780 triangles in two draws with all four entry corridors and the grounded
non-flying state unchanged.

Inspected native evidence `20261015T150827.344868Z-helicopter` passes at 1280x720 with 318 colour
bins, 55.41 luminance deviation, 235.01 spatial contrast and an empty kernel delta. The matching
accessible cockpit evidence `20261015T150858.183359Z-helicopter-inside` passes with 182 bins,
22.87 deviation, 96.99 contrast and an empty kernel delta. The exterior shows a continuous rounded
nose, roof fairing and tail-root transition rather than alternating planar bands; the interior
retains readable instruments, cyclics, collectives, pedals, seats and open exit sightlines.

## Open-cabin light and avionics pass — 2026-10-15

The next paired audit accepted the exterior silhouette but rejected the cockpit's nearly black,
full-width rectangular instrument block. Since all four doors are visibly open, cabin surfaces now
receive a restrained 70-percent indirect-light floor and upholstery a 60-percent floor, while
exterior paint, glass, rubber and metal keep the existing 48-percent base. Emissive needles,
annunciators and displays retain authored colour independent of sun direction. This is baked
portable vertex colour, not a dynamic light or additional render pass.

The panel itself is now a shallow tapered binnacle whose narrower crown follows the windscreen.
Two recessed radio/navigation displays occupy the lower centre stack and the six annunciators are
split into outer banks. The two thin display volumes add 24 triangles; the complete current model
is 3682 triangles in the same opaque/glass two draws. The atlas, shell, floor, collision, all four
entry routes and deliberately disabled flight state are unchanged.

`IslandHelicopterLayoutTests` and `IslandCoreTests` pass. Inspected current-binary native captures
`20261015T173412.277344Z-helicopter` and `20261015T173428.597718Z-helicopter-inside` pass at
1280x720 with 332/186 colour bins, 55.03/21.74 luminance deviation, 230.70/95.95 spatial contrast
and empty kernel deltas. The former confirms an unchanged grounded exterior and open doorways; the
latter distinguishes the panel, gauges, twin displays, pedestal, flight controls and seats without
turning the cabin into an implausibly bright room. Physical Android/AMD appearance and interactive
sitting remain outside this focused visual pass.

## Fuller wrapped cockpit bubble — 2026-10-15

The next same-camera comparison retained the airframe proportions and open access, but found that
the forward windscreen still narrowed almost uniformly into a dark conical wedge. The nine forward
longitudinal stations now retain more width and height through the upper windscreen and close through
a shorter, blunter nose cap. Glazing wraps progressively below the belt toward that cap rather than
using one constant cross-section cutoff. This changes the actual silhouette and glass/paint boundary;
it is not a colour-only attempt to hide the old shape.

The existing 32-sided cross sections and analytic loft normals consume the new stations directly.
No geometry, texture, instance, collision or draw is added: the complete helicopter remains 3,682
triangles in two draws, with its walkable floor, four door corridors, cockpit controls, stationary
rotors and non-flying contract intact. `IslandHelicopterLayoutTests`, `IslandCoreTests` and all 95
browser/capture gate tests pass.

Inspected baseline `20261015T173412.277344Z-helicopter` and current-binary exterior
`20261015T182504.108836Z-helicopter` show the fuller lower windscreen and less pointed nose. The
accepted 1280x720 frame passes with 322 colour bins, 54.80 luminance deviation, 230.69 spatial
contrast and an empty kernel delta. Interior guard `20261015T182542.524784Z-helicopter-inside`
passes with 195 bins, 21.80 deviation, 93.69 contrast and an empty kernel delta; the panel, radios,
controls, seats and open exits remain readable. This focused pass does not substitute for physical
AMD/Android review or future evidence-driven rotor/body-detail work.

## Parked rotor blade droop and chord taper — 2026-10-15

The fuller-cockpit exterior audit still showed all four 5.65 m main-rotor blades as nearly one
perfectly horizontal dark bar. The former dark portion used one closed span followed only by the
short coloured tip. It now uses three connected closed spans. Their shared centreline receives a
quadratic 18 cm maximum parked droop and a restrained 11 cm progressive backward sweep, while chord
half-width reduces through 0.155, 0.145, 0.128 and 0.110 m before the existing 0.098 m tip. Adjacent
spans share exact endpoints, so the change bends one continuous blade rather than assembling gaps.

The two extra spans per blade add 96 triangles and raise the complete helicopter from 3,682 to
3,778 triangles. Both existing opaque/glass submissions, generated atlas, rotor radius, site
clearance, stationary state, non-flying contract and all cabin access remain unchanged.
`IslandHelicopterLayoutTests`, `IslandCoreTests` and all 96 browser/capture tests pass; the focused
native, Chrome and Web-smoke gates now require the new blade-shape diagnostic.

Inspected same-camera baseline `20261015T182504.108836Z-helicopter` and accepted current-binary
evidence `20261015T184819.938307Z-helicopter` show the long parked blades carrying visible weight
instead of forming a ruler-straight line. The accepted 1280x720 frame passes with 314 colour bins,
54.76 luminance deviation, 230.71 spatial contrast and an empty kernel delta. This is a static
silhouette improvement, not rotor animation or a helicopter flight feature.

## Correctly oriented anti-torque rotor — 2026-10-15

The next exterior broadside exposed an actual mechanical orientation error. With local Z as the
aircraft longitudinal axis, the former four tail-rotor beams varied in X/Y and therefore formed a
front-facing disc normal to the tail boom. They disappeared into the fin from the normal side view
and did not describe a helicopter anti-torque rotor.

The tail rotor now occupies the Y/Z plane and turns about a projecting lateral X shaft on the
visible port side. Its compact gearbox hub carries four blades; every blade has an inner and a
yellow-tipped outer closed span, progressive chord taper, 56 mm total thickness and restrained
planform sweep. This adds no animation or flight ability: the entire assembly remains visibly
parked. Replacing the five old beams adds 92 triangles, taking the helicopter from 3,778 to 3,870
triangles in the existing opaque/glass two draws. Tail-boom geometry, collision, vegetation
reservation, all four door corridors, the supported floor and interior controls are unchanged.

`IslandHelicopterLayoutTests` and `IslandCoreTests` pass. Inspected baseline
`20261015T190813.243002Z-helicopter` and accepted current-binary
`20261015T191511.970681Z-helicopter` show the rotor disc and gearbox readable at the tail instead
of hidden as a mechanically incorrect front-facing cross. The accepted 1280x720 frame passes with
320 colour bins, 54.77 luminance deviation, 230.68 spatial contrast and an empty kernel delta.
Independent interior guard `20261015T191453.213246Z-helicopter-inside` retains the panel, controls,
floor and open exit sightlines at 194 bins, 21.80 deviation and 94.02 contrast with an empty kernel
delta.
