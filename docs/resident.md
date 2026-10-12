# Island resident — movement foundation in progress

The operator requested exactly one dark-skinned adult fictional resident with believable
human proportions, textured skin/hair and practical woven-cloth/plant-fibre tropical
clothing. Natural walking/turning, pauses, terrain-aware foot placement and obstacle/player
avoidance are required. No combat, quests, trading or compulsory interaction is planned.

**There is no visible resident in IslandGame yet.** The renderer-independent controller
and grounded body pose now exist. They do not satisfy the model, material, appearance or
supported-platform visual requirements. Do not mark the resident PLAN entry complete.

## Deterministic tour

ResidentRoute builds a closed tour using the current rendered terrain and actual world
collision proxies: palms, jungle trunks, props, wreck, campfire/woodpile and hut. It keeps
clear of the entire cave reserve and excavated stream. Five approaches connect the fire
clearing, northern walking corridor, western clearing, southern corridor and central trail.
Default world:25 simplified route points,688.249m total, five designated pause locations.
Construction takes about135ms in an isolated native Release test; this is setup cost,
not a frame-rate measurement or mobile timing claim.

A3m navigation grid caches static point/edge eligibility. Deterministic A* ties use node
index, and every simplified line is checked against actual ground and solid clearance.
Disconnected/fully blocked tours are refused rather than teleported or cleared of trees.
The route follows dry, walkable ground. It does not require a new island navigation engine.

## Contact and animation state

ResidentMotion owns the terrain cache and copied collision grid and refers to an immutable
IslandTerrain which must outlive it. The usual walking pace is0.95m/s, reduced on slopes;
heading turns are bounded. Named stops pause6–10s. A nearby grounded player makes the
resident pause/yield; movement resumes when the player leaves. Sidestepping and visible
human/player interaction still need integration and review.

The controller keeps two world-space planted soles. One foot swings at a time, with a
bounded lift and a terrain-fitted landing. Heel/toe/side probes fit the emitted terrain
triangles and tilt the sole to its support. Two-segment IK preserves0.43m thigh and0.44m
shin lengths, including recovery on slopes; it never silently accepts stretched contact.
Arm counter-swing follows the stepping foot instead of a fixed-distance clock that could
drift after hill strides or interrupted steps. Pelvis support adjusts to planted feet.
This is physical posing, not evidence that the final human animation looks natural.

Large elapsed times are clamped to0.1s and subdivided into at most0.02s steps; invalid
times do not move the actor. No background worker, graphics context or audio source is used.

## Native CPU evidence

IslandResidentTests runs two identical controllers through1200 simulated seconds against
the actual generated world. Their routes, state and poses are exactly equal. The resident
completes at least one full tour; the trace reaches847.907m travel with6 pauses. Tests
check both legs actually swing, planted feet do not slide, sole probes never penetrate or
float, full foot discs miss world obstacles, arm/leg lengths stay fixed, and movement never
teleports. They also exercise yield/resume, stall bounds and rejection of a blocked island.

The initial long test exposed a blocked body rolling back an airborne foot indefinitely.
Recovery now allows that foot to settle before the next step. The final tour has8 rejected
body microsteps which recover, rather than a frozen resident or a relaxed collision test.
The final tests pass in Release1.19s and Debug6.67s, including full foot-obstacle checks.
The independent world grid also verifies the body's clearance. Timing varies with the host;
these are CPU test durations, not game FPS measurements.

## Required next work

- Build the human model, facial detail, hair and practical clothes, with explicit material
  and asset provenance; all new controller code is original project code.
- Bind the pose to a portable renderer, integrate exactly one actor into IslandGame and
  WorldReport, and keep simulation/render work bounded by distance.
- Add close and moving in-world QA cameras, contact shadows and actual appearance review.
- Check player sidestepping, gait/turn transitions and representative performance.
- Perform consolidated native/Web/Android model, motion and lifecycle qualification.

The normal game is still the campfire/cave build. No resident visibility, final artistic
quality or portable runtime claim is made by these CPU results.
