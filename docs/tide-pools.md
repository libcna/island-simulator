# Rocky-shore tide pools

## Current implementation

`TidePoolLayout` owns five deterministic basins on the exposed rocky coastal bench.
Their centres are derived from the authoritative shoreline radius and remain 24–34 m
inland. `IslandTerrain::Height` subtracts a smooth compact bowl, so the pools are real
terrain depressions rather than water decals or ocean-connected cuts.

Each pool contains a 20-segment calm-water disc. The vertices are appended to the
existing shoreline mesh and selected inside its portable shader, retaining one
shoreline draw. Water sits above the carved floor and below the undepressed rim. A
very small slow ripple prevents a perfectly frozen surface without pretending to
simulate tides.

`NaturalPropLayout` adds two low non-solid existing-rock variants and one existing
shell/pebble cluster to each rim. These fifteen accents reuse existing prop renderers,
textures and draw paths. They are marked explicitly so tests can distinguish rocky
landmarks from the normal dry-sand detail belt.

## Acceptance evidence

- Long `island_core_tests` run passes, including basin separation, rocky-shore
  suitability, floor/water/rim ordering, deterministic prop placement and bounded
  accent counts.
- World report remains valid with fingerprint `e83f46fcbb2600c6`.
- Inspected llvmpipe capture
  `build/software-captures/20261012T115636.611276Z-tide-pools` passes at 1280x720 with
  195 colour bins, luminance deviation 40.29 and spatial contrast 193.56. It shows a
  closed foreground pool on the rocky plateau, visibly separate from the ocean, with
  rim stones and shell/pebble detail.
- The first three camera/location captures were rejected because the basin was too
  small, obscured or visually connected to the cliff/ocean. They are not acceptance
  evidence.

## Remaining work

The first pass is deliberately small and bounded. Improve the polygonal-looking rim
transition, consider a very small subset of existing crabs only if their sand habitat
and route logic can remain credible, and measure the incremental terrain/prop/shader
cost. Web and Android runtime and visual qualification are still required. No physical
tide simulation is planned.
