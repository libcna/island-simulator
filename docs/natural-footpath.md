# Narrow forest footpath

2026-10-11, first pass on macro-geography item 6.

The terrain already shaded a relatively narrow dirt path, but vegetation excluded a 12.4 m
wide corridor. That clearing read as an artificial cut through the island. The shared reserve
is now 2.8 m wide; the dirt material varies from about 1.7 to 2.6 m across its mid-coverage
edge, with softer irregular edges and the existing gradual endpoint fades. Canopies can
overhang the route, while ground vegetation approaches its sides.

Solid palm and canopy trunks and rock footprints now check the complete reserved corridor,
including conservative allowance for later emergent-tree promotion. A new regression walks
the route from z=-215 to 180 at one-metre intervals, sampling its centre and both +/-0.65 m
lanes against every static collision proxy plus the player's 0.34 m radius. It caught one
rock overlapping two lane samples; footprint rejection fixes that case. Existing footprint
tests also cover clearing and forest-floor companion plants.

The route still uses its original broad bend. Local detours, forks, more variable wear and
integration with future terrain drainage are open; this pass does not complete item 6.

## Populations and budget

New default inventory (deterministic world fingerprint `2816679faacc0474`): 372 palms,
288 canopy trees, 1,872 understory plants, 598 bushes, 3,223 small ground plants,
20,595 grass clusters, 173 rocks (14 offshore), 12 logs, 32 driftwood pieces and 67 fallen
fronds. Clearing accents: 14 bushes / 69 ground plants. Forest companions: 209 colonies /
855 plants. Mesh templates, textures and render passes are unchanged. Web harness inventory
pins were updated together with the placement changes.

## Evidence

- Native build succeeds. All 32 other non-display test groups passed; the core group then
  passed after the rock-clearance fix and inventory updates (26.82 s).
- Native software overview `20261011T070738.959114Z-overview` and close jungle
  `20261011T070801.578109Z-jungle` inspected; the central forest cut is less prominent.
  Final rock-clearance overview `20261011T070930.553742Z-overview` passes.
- Android x86_64 build/lint and contained SwiftShader interactive run
  `20261011T071027.731091Z-interactive` pass touch sensitivity selection, land footsteps,
  shore/swim traversal, same-process Home/resume (PID 1803) and cold relaunch (PID 3158).
  Image inspected; empty kernel delta. Default ARM64 build/lint restored, runtime untested.
  APK SHA-256: x86_64 `ff585a34e5a9ad94bb7b85f0ca2e166a3525e0c0ccc5303c012ee3176600001b`,
  ARM64 `571c304e5a5c9c3ee268768994e3e1b7929d5b64bb4fc72b1ee8cd1c3e5adc8d`.
- Web build succeeds. The first grass-motion check
  `20261011T071355.532471Z-chrome-grass-motion` failed its fixed 15 s wait at 5.9895 m
  of a requested 6 m walk, with valid focus/capture and 4.3 m/s simulated velocity.
  This was a wall-clock/slow-frame mismatch, not a collision. The harness now bounds each
  wait by 120 new game samples and 90 s wall time. Retry
  `20261011T071524.062886Z-chrome-grass-motion` passes all seven movement stages with
  93 distinct recorded frames, no context loss and GL error zero. Mid-walk image inspected;
  failed evidence retained. Overall focused suite passes, including 30 additional frame
  intervals. No new kernel GPU faults. This does not requalify the complete Web lifecycle suite.
  Web release refreshed: 10,338,572 raw / 3,524,786 gzip bytes.
  Hardware qualification remains stopped.
