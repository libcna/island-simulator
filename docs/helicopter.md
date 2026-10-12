# Grounded helicopter

## Native first pass

The island now places one deterministic grounded utility helicopter at
`(-50.2571, 48.7260)`, near the centre of the island and well inland. The site owns a
6.20 m vegetation/grass reservation around the 5.65 m main-rotor radius. Placement rejects
steep or high-relief ground, the stream and every existing solid obstacle. Four skid contacts
sample `RenderedTerrainSurface`; the body is not allowed to float or use one guessed centre
height.

The model is original project procedural geometry, not a downloaded asset. One static
vertex-colour batch contains 1,006 triangles: a multi-ring fuselage, sealed glazed nose,
windshield mullion, tail boom and empennage, stationary main and tail rotors, terrain-following
skids, four open doorways, two threshold/ramp levels, a supported cabin floor, two pilot seats,
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
panel, glazing, skids and stationary rotors. This is a reduced-detail first pass for the 100-hour
plan, not a photorealism claim.

The inspected final evidence is
`build-consumer/software-captures/20261012T215230.210248Z-helicopter` (275 colour bins,
54.49 luminance deviation, 230.02 spatial contrast) and
`build-consumer/software-captures/20261012T215239.544205Z-helicopter-inside` (216 bins,
32.48 deviation, 132.84 contrast). Both report one helicopter draw, the 1,006-triangle marker
and an empty new-kernel-fault delta.

Open work includes textured paint/glass/upholstery, richer curved anatomy and small mechanical
detail, improved interior and exterior lighting, door animation, explicit sitting interaction,
live free-walk visual review, performance measurement, and Web/Android capture qualification.
