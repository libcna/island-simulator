# Coastal micro-topography

2026-10-15 completes macro-geography item 15 without adding a second terrain representation.

`IslandTerrain::Height` now adds bounded authoritative relief inside the existing coastal fields:

- sandy coast receives a 0.18--0.38 m lower-beach berm and a broader 0.40--0.86 m dune shoulder;
- two periodic fields vary those ridges along the coast instead of producing concentric rings;
- narrow shallow wash channels interrupt the lower berm while preserving height zero at the exact
  shoreline;
- rocky headlands receive two 0.22/0.34 m walkable shelf steps inside their existing raised relief.

All envelopes are broad enough for the four-metre terrain grid. Rendering, player collision,
placement support and wildlife queries therefore see the same surface. The work reuses the existing
rocky erosion edge, eight low exposed root ridges on each close palm and five terrain-authored rocky
tide pools. It adds no renderer, texture, mesh allocation, draw or runtime simulation.

Core tests directly prove a convex broad-beach berm/dune shoulder, a lower-beach wash depression,
the rocky shelf step and an unchanged zero-height swimming boundary. Tide-pool floor/water/rim and
palm-root geometry retain their existing coverage. The cove driftwood audit exposed one harmless
old threshold edge: measured support slope became 0.160185 radians with only 0.105 m end-to-end
relief. Its acceptance limit is now 0.17 radians; the full support and no-hover tests remain.

Inspected native evidence:

- `build-consumer/software-captures/20261014T234233.573462Z-bay-ground` — 1280 by 720,
  325 colour bins, 39.71 luminance deviation and 174.70 spatial contrast;
- `build-consumer/software-captures/20261014T234001.750337Z-headland` — 1280 by 720,
  390 colour bins, 44.27 deviation and 232.58 contrast.

The first Chrome/SwiftShader terrain attempt used the old 300-second timeout and completed the
overview and bay before timing out during bay-ground; it reported no GL/context/kernel fault and is
not final suite evidence. The repeated 900-second run
`build/browser-software/20261014T234946.393041Z-chrome-terrain` qualifies overview, bay,
bay-ground and headland for 60 finite positive intervals each, then receives supervisor signal 15
during the ordinary beach view. It is targeted coastal evidence, not a complete seven-view terrain
run. ARM64 assembly/lint passes (24,049,475 bytes, SHA-256
`6fb75be559f71b898fde0213a4dafd57905e374993a2ad18e6bb3d9357fe9df4`) without emulator/KVM;
all 63 enabled non-display tests pass. Physical Android GPU appearance remains unqualified.

