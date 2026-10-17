# Inland research bunker

The bunker is an original project-owned procedural asset. It uses no downloaded model,
texture or third-party licence. The entrance sits in the inland ridge at `(92, 24)` and
opens into a descending access passage, a 36 m main spine, a computer room, a service
workshop and a deliberately fictional reactor-generator room. It is scenery for quiet
exploration, not an operational reactor design or a mission system.

## Traversal and terrain integration

`BunkerLayout` owns the deterministic local plan, floor support and player-capsule
collision. The exterior heightfield remains intact over the buried facility, while the
terrain shader suppresses only fragments below the bunker ceiling. This keeps soil and
vegetation above the roof without filling the playable rooms. The final corridor wall is
a full sealed maintenance bulkhead, so the player can no longer see the underside of the
island beyond the authored interior.

The portal threshold uses a packed-earth apron and a gentle floor transition. A native
controller regression starts outside, walks through the portal and down the ramp, reaches
the room branches and deep corridor, then returns to dry exterior ground. Grass and
ordinary vegetation reserve the entrance and facility footprint.

## Visual construction

The current static mesh is 6,328 triangles in two culled draws. It includes:

- a rock-concealed reinforced portal, open blast door, jamb bolts and drainage threshold;
- repeated structural corridor bays, slope-fitted recessed access panels, safety stripe, cable
  trays, paired entrance conduits, transverse floor joints, utility rails and six housed warm
  ceiling-light pools;
- four active computer desks with keyboards and chairs, eight server cabinets, a wall
  situation display, tiled floor and cable details;
- a complete service-room shell with cabinets, workbench and amber task lighting;
- a cyan fictional generator core, open tubular containment hoops, radial braces, luminous core
  bands, support posts, control cabinets, pipes and caution markings;
- a reinforced sealed end bulkhead that replaces the former view beyond the facility, now with a
  round six-spoke handwheel, closed hub, perimeter bolts and a powered status panel.

Opaque concrete, steel and rock use one portable desktop-GL/GLES shader with subtle
world-space mottling, damp ageing, restrained oxidation and baked per-face shading.
Luminous fixtures, displays and the generator core share the second alpha draw. If custom
effects are unavailable, the geometry retains a `BasicEffect` fallback and logs it; the
qualified runs below used the custom material without fallback.

## Verification — 2026-10-14

Native Release `island_bunker_tests` passes real `PlayerMotion` entry, descent, room
access, collision and exit. Inspected native software captures include the computer room
and the sealed-end view `20261014T184005.204770Z-bunker-end`; the latter shows the complete
bulkhead with no exposed island underside.

Focused Chrome/SwiftShader evidence is
`build/browser-software/20261014T190856.457455Z-chrome-bunker`. It contains six distinct
inspected views (`bunker`, corridor, computers, reactor, service and end), each with twelve
finite post-capture intervals, `glError=0`, an intact context, the current material/closure
diagnostics and an empty kernel GPU-fault delta. Twelve intervals are intentional here:
the scope owns six static artistic cameras and the CPU-only renderer can take more than
ten seconds for one full-island frame. This is a shader/visual portability gate, not a
WebGL performance claim.

Android ARM64 `assembleDebug` and `lintDebug` pass with two workers. The generated debug
APK is 23,919,699 bytes. No emulator, KVM setting or device runtime was touched, so GLES
runtime traversal and touch-control review remain open.

## Remaining work

The facility is intentionally compact and still visibly procedural at close range.
Moving first-person artistic review, richer prop variation, subjective audio listening, Android
device runtime and real-GPU performance qualification remain future polish. They do not
reopen the fixed entrance collision or the sealed-end terrain leak.

## Maintenance bulkhead handwheel — 2026-10-16

A later landmark audit accepted the repaired closure but selected the end door's two gold
rectangular braces: together they looked like a large cross or sign rather than believable
maintenance hardware. They are replaced by a sixteen-segment circular wheel, six tubular spokes,
a closed central hub and eight perimeter fasteners. A small powered three-lamp status panel breaks
the otherwise monochrome door leaf. All additions sit in front of the existing complete bulkhead;
none changes the authoritative shell or collision.

The original project-owned bunker grows from 3,876 to 4,508 triangles inside the same two static
draws and existing 24,000-vertex construction guard. No texture, shader input, buffer, collision or
gameplay object is added. The visually correct first result
`20261016T112318.215292Z-bunker-end` was rejected because its 60 quantized colours missed the
generic 64-bin scene gate. The retained status-panel result
`20261016T112758.539261Z-bunker-end` passes at 1280x720 with 65 bins, 49.82 luminance deviation,
228.58 spatial contrast and an empty kernel delta.

Four focused native groups and all 97 browser/capture harness tests pass. Focused Chrome
155/SwiftShader evidence `build/browser-software/20261016T113224.443896Z-chrome-bunker` contains
six inspected current views. Each supplies 12 finite positive intervals, GL error zero and a live
context; the final frame shows the wheel, bolts and status panel, while the room views retain the
computer, generator and service spaces. The kernel delta is empty. Physical Android/AMD runtime
and moving first-person performance remain grouped with final qualification.

## Local electrical ambience — 2026-10-16

The underground footprint now has a dedicated original seventeen-second electrical/generator hum.
It crossfades only when the listener follows the playable descending floor into the corridor or
rooms; the vertical camera-to-floor check prevents the same world footprint from leaking onto the
grassy ridge above. Deeper presence attenuates the outdoor surf, breeze and jungle beds to create a
bounded enclosed transition without a reverberation engine.

The hum reuses the existing fourth regional source, taking priority over the mutually exclusive
campfire, stream and player-water loops. It therefore adds no simultaneous voice beyond the
existing four persistent / six maximum source contract. Real CNA dummy-backend tests enter and
leave the acoustic volume, query actual Playing state and volume, exercise the source budget and
confirm that the ridge remains outside. The exact PCM is also covered by deterministic format,
energy, headroom, DC and seam tests plus independent WAV export validation. Release and WebGL 2
compilation, five focused native test groups, Android ARM64 `assembleDebug` and `lintDebug` pass
with at most three build workers (the Android wrapper uses two). The APK is 24,664,315 bytes and
contains only the 19,621,744-byte `arm64-v8a/libmain.so`. No emulator or KVM setting was touched.
The current Chrome 155/SwiftShader evidence
`build/browser-software/20261016T115641.304967Z-chrome-bunker` passes all six bunker views with
twelve positive frame intervals each, a live context, GL error zero and an empty kernel delta.
Subjective listening, browser/device audio lifecycle and physical output remain part of final
audio qualification.

## Access and generator-cage visual pass — 2026-10-17

The next landmark comparison found two remaining large flat forms. From the entrance ramp, the first
ten metres were an uninterrupted concrete wedge even though the deeper corridor already had structural
bays. In the generator room, four capped cylinders formed solid grey shelves that hid the cyan core.

Four pairs of recessed access panels now follow the real sloping floor and ceiling. Their frames,
fasteners, expansion joints, paired ceiling conduits and opaque lamp housings continue the existing
facility language without narrowing the authoritative walkable corridor. The generator shelves are
replaced by open sixteen-section tubular hoops with radial braces, two luminous core bands and a low
machinery plinth. A lower caution band and centre seam add scale to the sealed end door without changing
its collision or reopening the terrain leak.

The mesh rises from 4,508 to 6,328 triangles, remains inside the unchanged 24,000-vertex construction
guard and still uses one opaque plus one glow draw. Native layout/core tests and the 55 capture harness
tests pass. Inspected native evidence is:

- corridor baseline/final: `20261017T070528.433880Z-bunker-corridor` /
  `20261017T071038.551081Z-bunker-corridor`;
- rejected solid-disc reactor baseline: `20261017T071215.535551Z-bunker-reactor`;
- accepted open-cage reactor: `20261017T071556.777512Z-bunker-reactor`;
- accepted sealed bulkhead: `20261017T072043.709398Z-bunker-end`.

The accepted reactor and bulkhead pass the common scene image gate at 75 and 72 quantized colour bins
respectively. The baseline reactor's 58-bin failure is intentionally retained as evidence of the
selected visual defect. WebGL builds, and inspected Chrome 155/SwiftShader evidence
`build/browser-software/20261017T072701.009087Z-chrome-bunker` passes all six current views with
twelve positive intervals each, GL error zero, live contexts and an empty kernel delta. Android ARM64
assemble/lint pass without starting an emulator; the 24,756,019-byte APK has SHA-256
`2758207cda2deae9d28c0dba9265db3e848231097218a2a07d9d8244ca523bfd` and contains a
19,713,448-byte `libmain.so`. Physical Android appearance and hardware-GPU performance remain
unqualified.
