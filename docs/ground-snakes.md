# Ground snake implementation and visual evidence

Four live nonhostile ground snakes are now instantiated after the complete world
obstacle grid, updated alongside other wildlife and drawn in normal IslandGame play.
Each body uses 41 terrain-supported stations and a connected 16-sided tube, with a
tapered tail, capped head/nose and small eyes. One bounded dynamic batch (20,000 vertex
capacity, at most 16,032 vertices for all four) uses stock BasicEffect, canopy lighting,
fog, frustum checks and a 70 m distance limit. The distance currently does not scale
with the mobile wildlife profile; mobile visual/performance review remains open.

The original code-generated 64×64 RGBA8 staggered-scale texture adds 16,384 bytes.
Updated exact generated total: 4,652,172 bytes; full runtime texture total 6,399,796,
quality allowance 6,420,012. Numerical test and platform log expectations were updated
by the exact allocation, not weakened. Provenance is in ASSET_LICENSES.md and the asset
manifest (25 procedural records, 77 generator sources, 10 project files, no external
files). Scale detail is procedural prototype art, not an accepted final material.

Use `--viewpoint snakes` for the close ground view. This is an ordinary live scene,
not a frozen replacement pose or hidden population. Draw totals and startup visibility
include snakes. F1 per-species rows, WorldReport population/fingerprint coverage and
dedicated Web/Android snake runtime gates still need extension.

## Actual native image review

All runs used the existing supervised private-display llvmpipe launcher. No host GPU
qualification or emulator configuration was attempted. Each actual image below was
inspected, not inferred from its metrics:

- `20261011T180203.942707Z-snakes`: snake clearly visible, but oversized lozenge scales.
  Close-view scene-contrast gate failed. Increased UV density and softened scale borders.
- `20261011T180405.133764Z-snakes`: wider camera passes generic image checks, but a
  foreground shrub hides the subject. Rejected as useful snake appearance evidence.
- `20261011T180523.790109Z-snakes`: opposite-side camera also passes generic checks,
  but grass hides most of the body. Also rejected for close snake review.
- **Final `20261011T180722.242708Z-snakes`**: restored useful close camera; full tapered
  olive body and much finer scales visible against the ground, head bent away as it
  retreats. Grass still crosses parts of the body. Head/eyes are insufficiently distinct
  at this distance, and the nearly straight starting body needs continuous-motion review.
  Image has 87 color bins, luminance deviation 11.8835 and spatial contrast 17.8797,
  below unchanged generic scene thresholds 18/30. **Automated image gate remains failed**;
  do not describe the final capture as passing because earlier obscured wide views passed.
  Kernel delta is empty. Final PNG is in the run directory beside the retained PPM.

Native build and 40 non-display groups pass (73.86 s); 34 supervisor CPU checks pass.
Final graphics-log replay passes independently of the failed image-contrast gate.
Web Release build/package passes (11,118,476 raw / 4,183,185 gzip bytes); ARM64
assembleDebug/lintDebug passes in 10 s. These are builds, not portable image evidence.
These establish integration and numerical regression coverage, not final appearance.
No tree snakes exist yet. Remaining work: recognizable head/face, continuous body/gait
and final mesh/ground clearance audit, appropriate close-subject capture acceptance
that still rejects blank/missing animals, grass intersections, contact shadows, profile
distance/LOD and reporting completeness, portable actual images. Keep the full snake
task unchecked, including its ground and tree requirements.
