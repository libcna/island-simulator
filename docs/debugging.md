# Debugging and Runtime Diagnostics

## Overlay

Press F1 during an interactive desktop or browser run to toggle the diagnostics overlay. It starts
hidden during ordinary play. Use `--debug-overlay` to enable it from the first frame for automated
captures:

```bash
build/island-simulator --debug-overlay
```

The overlay is deliberately implemented with CNA's public `SpriteBatch`, `SpriteFont`, and
`Texture2D` APIs. Its original 5 by 7 uppercase font atlas is generated at runtime, so debugging
does not depend on a platform font, a content compiler, or an undeclared asset.

While the overlay is visible, F5 through F8 teleport to the south beach, jungle trail, east
clearing, and west clearing. These fixed locations shorten traversal and vegetation QA without
exposing teleports during the ordinary presentation.

Current fields are:

- smoothed wall-clock frame duration and a half-second FPS sample;
- player eye position, walking/swimming state, ground support height, and maximum footprint slope;
- positive offshore distance and normalized ocean-current strength;
- active renderer identity;
- visible/total terrain cells;
- visible/total palms, near/far LOD population, cells, and draw submissions;
- visible/total broadleaf canopy, understory, low-bush, and forest-floor populations plus
  cells/draw submissions;
- visible/total rocks, fallen logs, and beach driftwood plus shared prop cells/draw submissions;
- visible/total grass clusters, cells, and draw submissions;
- audio availability/mute state, current surf/breeze mix levels, and played stronger-wave count.

Frame timing measures intervals between actual `Draw` calls rather than the game's fixed simulation
timestep. Startup and four-frame smoke captures are intentionally too short for a representative
FPS number. Use a sustained interactive run and compare representative viewpoints before drawing
performance conclusions.

## Deterministic captures

The executable provides fixed views used by CTest and the browser harness:

```bash
build/island-simulator --capture /tmp/island-overview.ppm
build/island-simulator --capture /tmp/island-beach.ppm --viewpoint beach
build/island-simulator --capture /tmp/island-jungle.ppm --viewpoint jungle
build/island-simulator --capture /tmp/island-props.ppm --viewpoint props
build/island-simulator --capture /tmp/island-driftwood.ppm --viewpoint driftwood
build/island-simulator --capture /tmp/island-debug.ppm --viewpoint beach --debug-overlay
```

These runs render four frames and exit. The WebGL 2 harness writes corresponding overview, beach,
jungle, prop, and driftwood PNG captures plus a JSON report under `build-probe/`. Captures prove
initialization, shader selection, draw-path
execution, and deterministic composition. A separate live harness section records browser input
and pointer-lock evidence. The deployable-page pass also proves first-gesture CNA mixer/track
startup. It does not prove audible hardware-browser quality, hardware-GPU frame pacing, genuine tab
lifecycle, or long-session stability.

## Planned diagnostics

World-space collision, shoreline/current, vegetation-cell, LOD, wildlife-state, and audio-emitter
visualizations are not implemented yet. Neither are a teleport menu, free camera, recorded frame
histograms, or target-specific budget warnings. Keep these tools opt-in and absent from the normal
release presentation as they are added.
