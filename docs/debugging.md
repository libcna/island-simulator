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
- a yellow inward arrow at the player once the current is active;
- the player footprint in white and nearby palm, canopy, rock, and log collision proxies in
  orange;
- nearby shared terrain/vegetation cell edges in green;
- wildlife heading arrows, with full-simulation agents in species/state colours, reduced agents
  in yellow, and dormant agents in grey. Monkey full-tier colours distinguish idle, observe,
  ordinary locomotion, and run-away states.

The local proxies, cells, and wildlife markers are intentionally x-ray lines so dense grass or
foliage cannot hide them. Shore/current contours remain depth-aware. F2 is a developer aid only;
F1 hides all world lines without changing the stored F2 toggle.

The overlay is deliberately implemented with CNA's public `SpriteBatch`, `SpriteFont`, and
`Texture2D` APIs. Its original 5 by 7 uppercase font atlas is generated at runtime, so debugging
does not depend on a platform font, a content compiler, or an undeclared asset.

While the overlay is visible, F5 through F12 teleport to the south beach, jungle trail, east
clearing, west clearing, south fish shallows, offshore shark view, monkey troop, and the 62 m
offshore current field. These fixed
locations shorten traversal and wildlife/vegetation QA without exposing teleports during the
ordinary presentation. Repeated F11 presses cycle through all seven monkeys for silhouette,
animation, and obstacle-clearance inspection.

Current fields are:

- smoothed wall-clock frame duration and a half-second FPS sample;
- player eye position, walking/swimming state, ground support height, and maximum footprint slope;
- positive offshore distance and normalized ocean-current strength;
- player water-ripple intensity and wrapped radial phase;
- near-surface water-view intensity and its conditional draw count;
- local collision broad-phase candidates versus the complete static obstacle population;
- active renderer identity;
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
- played positional bird/monkey call counts and pooled wildlife-voice active/idle state.
- F2 visibility plus nearby obstacle, shared-cell, wildlife-marker, line, and draw counts.

Frame timing measures intervals between actual `Draw` calls rather than the game's fixed simulation
timestep. Startup and four-frame smoke captures are intentionally too short for a representative
FPS number. Use a sustained interactive run and compare representative viewpoints before drawing
performance conclusions.

## Recorded performance reports

`--benchmark <report.json>` selects an uncapped, variable-timestep fixed-view run, discards 120
warm-up intervals, records 600 intervals, writes interpolated median/p95/p99 statistics and scene
counts, including contact-shadow population and triangle count, then exits. It cannot be combined
with capture or debug modes. For example:

```bash
build/island-simulator --benchmark /tmp/island-jungle.json --viewpoint jungle
```

`tools/run_native_benchmarks.sh` repeats the measurement for overview, beach, jungle, driftwood,
and monkey views. See `docs/performance.md` for the isolated-GPU command, interpretation rules,
initial baseline, and explicit limitations.

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
```

These runs render four frames and exit, except the monkey capture waits 24 frames for the startup
crossfades to settle. The ground view selects a cleared variant-1 forest-floor plant beside the
primary path for repeatable leaf-silhouette review. The WebGL 2 harness writes corresponding
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

The first F2 pass covers the world geometry listed above. Exact wildlife targets/spawn regions,
individual renderer culling volumes and LOD radii, grass-density fields, terrain contact normals,
audio zones/emitters, and source-count visualization remain open. So do a teleport menu, free
camera, target-specific budget warnings, and renderer-specific CPU/GPU separation. All additions
must remain opt-in and absent from the normal release presentation.
