# Debugging and Runtime Diagnostics

## Overlay

Press F1 during an interactive desktop or browser run to toggle the diagnostics overlay. It starts
hidden during ordinary play. Use `--debug-overlay` to enable it from the first frame for automated
captures:

```bash
build/island-simulator --debug-overlay
```

Press F2 to toggle the world-space diagnostic while the F1 panel is visible, or launch directly
with `--world-debug`. The view uses the same portable `BasicEffect` line-list path on OpenGL 3.3,
WebGL 2, and OpenGL ES 3. It draws:

- the authoritative irregular shoreline in cyan;
- the 42 m soft-current threshold in yellow, 72 m full-current threshold in orange, and 92 m
  safety threshold in magenta-red;
- six terrain-following ambient-zone contours: canopy breeze 12--105 m inland in yellow-green,
  surf 18--125 m inland in cyan/blue, and jungle texture 30--112 m inland in green;
- a yellow inward arrow at the player once the current is active;
- the player footprint in white and nearby palm, canopy, rock, and log collision proxies in
  orange;
- the exact nine-point terrain footprint used by walking collision. Green lines show sampled
  normals, red lines exceed the 38-degree walk limit, white rings identify samples supporting the
  resolved player height, and a white line joins ground support to the player eye;
- the last nonzero horizontal player step at its exact world scale. Cyan/orange/red/magenta marks
  the attempted endpoint for clear/obstacle/terrain/safety-limit motion, while green marks the
  actual resolved endpoint after substepping and axis sliding;
- nearby shared terrain/vegetation cell edges in green;
- wildlife heading arrows, with full-simulation agents in species/state colours, reduced agents
  in yellow, and dormant agents in grey. Monkey full-tier colours distinguish idle, observe,
  ordinary locomotion, and run-away states;
- exact nearby fish-school centres, the current shark route target, and each nearby monkey's
  terrain-following 34 m home range. Walking/running monkeys add their exact active target and a
  line from the current pose;
- the three authored bird-family route volumes, every bird's exact initial pose and heading, and
  each nearby bird's live radial steering target. Cyan identifies shore birds, green canopy birds,
  and violet high birds;
- the nearest non-dormant bird audio candidate in cyan and monkey candidate in violet. A playing
  positional call adds a larger pale marker and an x-ray line from the listener to the exact
  emitter position captured when that call began.

The local proxies, cells, and wildlife markers are intentionally x-ray lines so dense grass or
foliage cannot hide them. Shore/current and audio-zone contours remain depth-aware. F2 is a
developer aid only; F1 hides all world lines without changing the stored F2 toggle.

The overlay is deliberately implemented with CNA's public `SpriteBatch`, `SpriteFont`, and
`Texture2D` APIs. Its original 5 by 7 uppercase font atlas is generated at runtime, so debugging
does not depend on a platform font, a content compiler, or an undeclared asset.

The panel shows all four authoritative procedural-layout seeds as fixed-width hexadecimal values.
Press F4 while F1 is visible to copy this stable text block:

```text
island-simulator world seeds
palms=0x7a11d5e3
grass=0x6b4d2f91
jungle=0x43b0a7d1
natural-props=0xa7816c39
```

`COPIED` appears for three seconds only if immediate clipboard readback exactly matches the block.
`UNAVAILABLE` means the platform declined or could not confirm the write; the visible values remain
authoritative. This distinction matters in browsers, where clipboard access can be permission- and
user-gesture-gated even though the key was handled by the game. The native private-display input
gate presses F4 and requires a confirmed round trip.

While the overlay is visible, F5 through F12 teleport to the south beach, jungle trail, east
clearing, west clearing, south fish shallows, offshore shark view, monkey troop, and the 62 m
offshore current field. These fixed
locations shorten traversal and wildlife/vegetation QA without exposing teleports during the
ordinary presentation. Repeated F11 presses cycle through all seven monkeys for silhouette,
animation, and obstacle-clearance inspection.

In debug builds, press F3 while the F1 panel is visible to detach a bounded inspection camera at
the current eye position. WASD moves relative to camera yaw, Q/E moves down/up, and either Shift
key raises speed from 12 m/s to 36 m/s. Mouse look is unchanged. The player and locomotion camera
motion remain frozen, while render culling and distance-scaled wildlife simulation follow the
inspection camera. Press F3 again to return to the player; hiding F1 also exits this mode so it
cannot remain active without an obvious developer presentation. The camera is limited to
`+/-820 m` horizontally and `-18..260 m` vertically.

`IslandGameFreeCameraInput` drives this path with real X11 input on the private Xwayland display.
It verifies both F3 edges and an active diagnostic sample; renderer-independent core tests cover
frame-time clamping, normalized three-axis movement, yaw-relative fast travel, bounds, and the
inactive-state invariant. This does not qualify Wayland-native or Android touch input.

Current fields are:

- smoothed wall-clock frame duration and a half-second FPS sample;
- player eye position, walking/swimming state, ground support height, and maximum footprint slope;
- positive offshore distance and normalized ocean-current strength;
- player water-ripple intensity and wrapped radial phase;
- near-surface water-view intensity and its conditional draw count;
- local collision broad-phase candidates versus the complete static obstacle population;
- active renderer identity;
- all four authoritative layout seeds plus confirmed/unavailable F4 clipboard feedback;
- visible/total terrain cells;
- fixed distant-scenery islet count and its single draw submission;
- visible contact-shadow total split across palms, canopy trees, land rocks, wood, and monkeys,
  plus the pass draw count;
- visible/total palms, near/far LOD population, cells, and draw submissions;
- visible/total broadleaf canopy with near/far LOD population, understory, low-bush, and
  forest-floor populations plus cells/draw submissions;
- visible/total rocks, the emergent-reef subset, fallen logs, beach driftwood, fallen palm fronds,
  and shell/pebble clusters plus shared prop cells/draw submissions;
- visible/total grass clusters, cells, draw submissions, and smoothed player-interaction strength;
- visible/total birds, near/far render LOD, full/reduced/dormant simulation tiers, and draw count;
- visible/total fish, near/far render LOD, full/reduced/dormant simulation tiers, and draw count;
- shark presence, visibility, activity, near/far render LOD, simulation tier, patrol cycles, and
  draw count;
- visible/total monkeys, near/far render LOD, full/reduced/dormant simulation tiers, compact
  idle/observe/walk/run state counts, draw count, cumulative obstacle-avoidance deflections, and
  active/completed skeletal crossfades;
- audio availability/mute state, current surf/breeze/jungle/player-water mix levels, smoothed
  undertow cue strength, and played stronger-wave/water-transition counts;
- played positional bird/monkey call counts, pooled wildlife-voice active/idle state, and currently
  playing sources against the hard six-source ceiling;
- F2 visibility plus nearby obstacle, shared-cell, wildlife marker/target/monkey-home-range
  (`WILDLIFE M/T/H`), bird route-family/spawn/live-target (`BIRD R/S/T`), audio
  zone/candidate/active-emitter (`AUDIO Z/C/E`), terrain
  sample/support/over-limit (`CONTACT S/S/O`), line, and draw
  counts. `MOVE O/T/S/C` reports obstacle rejections, terrain rejections, slide substeps, and
  ocean-safety clamping for the retained movement trace.

Frame timing measures intervals between actual `Draw` calls rather than the game's fixed simulation
timestep. Startup and four-frame smoke captures are intentionally too short for a representative
FPS number. Use a sustained interactive run and compare representative viewpoints before drawing
performance conclusions.

## Recorded performance reports

`--benchmark <report.json>` selects an uncapped, variable-timestep run, discards 120 warm-up
intervals, records 600 intervals, writes interpolated median/p95/p99 statistics plus
min/max/mean scene-population ranges, and exits. It cannot be combined with capture or debug
modes. A fixed-view example is:

```bash
build/island-simulator --benchmark /tmp/island-jungle.json --viewpoint jungle
```

The `traversal` viewpoint is valid only with benchmark mode. It moves the authoritative player and
camera over a deterministic 676 m rail through shallow water, beach, trail, clearings, dense
jungle, and the opposite shore so culling, LOD, and contact-shadow refreshes occur throughout the
sample window:

```bash
build/island-simulator --benchmark /tmp/island-traversal.json --viewpoint traversal
```

`tools/run_native_benchmarks.sh` repeats the measurement for overview, beach, jungle, driftwood,
and monkey fixed views, then the moving traversal. See `docs/performance.md` for the isolated-GPU
command, interpretation rules, initial baseline, and explicit limitations.

## Deterministic captures

The executable provides fixed views used by CTest and the browser harness:

```bash
build/island-simulator --capture /tmp/island-overview.ppm
build/island-simulator --capture /tmp/island-beach.ppm --viewpoint beach
build/island-simulator --capture /tmp/island-palms.ppm --viewpoint palms
build/island-simulator --capture /tmp/island-jungle.ppm --viewpoint jungle
build/island-simulator --capture /tmp/island-ground.ppm --viewpoint ground
build/island-simulator --capture /tmp/island-props.ppm --viewpoint props
build/island-simulator --capture /tmp/island-driftwood.ppm --viewpoint driftwood
build/island-simulator --capture /tmp/island-reef.ppm --viewpoint reef
build/island-simulator --capture /tmp/island-water.ppm --viewpoint water
build/island-simulator --capture /tmp/island-current.ppm --viewpoint current
build/island-simulator --capture /tmp/island-distant.ppm --viewpoint distant
build/island-simulator --capture /tmp/island-birds.ppm --viewpoint birds
build/island-simulator --capture /tmp/island-fish.ppm --viewpoint fish
build/island-simulator --capture /tmp/island-shark.ppm --viewpoint shark
build/island-simulator --capture /tmp/island-monkeys.ppm --viewpoint monkeys
build/island-simulator --capture /tmp/island-debug.ppm --viewpoint beach --debug-overlay
build/island-simulator --capture /tmp/island-world-debug.ppm --viewpoint overview --world-debug
build/island-simulator --capture /tmp/island-contact.ppm --viewpoint contact --world-debug
build/island-simulator --capture /tmp/island-movement.ppm --viewpoint movement --world-debug
build/island-simulator --capture /tmp/island-wildlife-nav.ppm --viewpoint monkeys --world-debug
```

These runs render four frames and exit, except the monkey capture waits 24 frames for the startup
crossfades to settle. The ground view selects a cleared variant-2 split-leaf forest-floor plant
beside the primary path for repeatable leaf-silhouette review. The contact view places an offset QA
camera above the jungle path while leaving the player footprint at the target, making all nine
support samples and their normals visible. The movement view deterministically walks east into one palm,
retains the rejected direct step and axis-slide resolution, and observes both endpoints from an
offset camera. Native CTest follows the overview with `tools/validate_capture.py`; it validates the
complete 1280 by 720 P6 payload and broad luminance, colour-diversity, and spatial-contrast metrics,
then writes `island-capture-report.json`. This is a deterministic non-degeneracy gate, not a
pixel-golden or native-versus-Web image comparator. The WebGL 2 harness writes corresponding
overview, beach, grass, ground, palm,
jungle, prop, driftwood, reef, water, current, frond, distant, bird, fish, shark, and monkey PNG
captures plus a JSON report under
`build-probe/`. Captures prove
initialization, shader selection, draw-path
execution, and deterministic composition. The prop capture requires a nonzero contact-shadow
population and its single draw. A separate live harness section records browser input
and pointer-lock evidence. The deployable-page pass also proves first-gesture CNA mixer/track
startup for all four looping layers, and the deterministic live-input pass proves acceptance of
one shoreline transition effect. These checks do not prove audible hardware-browser quality,
hardware-GPU frame pacing, genuine tab lifecycle, or long-session stability.

## Planned diagnostics

The F2 pass covers the world geometry and audio diagnostics listed above, including the complete
bird route/spawn layer. Steering avoidance proposals, spawn regions for other wildlife, individual
renderer culling volumes and LOD radii, and grass-density fields remain open. The movement trace
retains the latest resolved frame;
a longer history and geometric step-up/depenetration views are not implemented. So do a teleport
menu, target-specific budget warnings, and renderer-specific CPU/GPU separation. All
additions must remain opt-in and absent from the normal release presentation.

For startup diagnosis, `--startup-report <path>` records five application-owned phases through the
end of the first `Draw` body. It is intended for repeatable process-level measurement rather than
the F1 overlay; see `docs/performance.md` for semantics and comparison rules.
