# Distant islets and aerial perspective

2026-10-10, from the macro-geography analysis item 14: the five horizon islets had similar size,
distance and contrast, so they read as a row of props rather than as depth.

## What changed

- Layout (`DistantSceneryLayout.cpp`): a nearer wooded islet at 575 m, a mid companion, one
  low treeless **sandbar** (new variant 3, 2.2 m high, pale sand palette, no palms) at about
  870 m, and two far silhouettes on the diagonals (about 680 m and 1,000 m). Tests require one
  islet within 600 m, one beyond 950 m, exactly one sandbar, the existing 570 m minimum, the
  ocean extent and separation.
- Aerial perspective (`DistantSceneryRenderer.cpp`): every islet colour is pre-blended toward
  the shared fog colour by distance (22 % at 480 m rising smoothly to 84 % at 1,100 m) before
  the stock effect's distance fog, whose end moved from 790 to 1,300 m so far silhouettes stay
  faintly visible instead of vanishing. Still one draw and the stock `BasicEffect`.

## Evidence

Native llvmpipe `build/software-captures/20261010T172812.566297Z-distant` and
`172806.329415Z-overview` (pass, inspected): the nearer wooded islet reads with palms, the far
ones as hazy silhouettes. Platform runs are recorded in `PLAN.md`.
