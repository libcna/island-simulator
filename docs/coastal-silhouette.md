# Sheltered western bay and three headlands

2026-10-11, shoreline stage of macro-geography items 1–2.

The shared radial shoreline now combines smaller-scale irregularity with three periodic
directional lobes and a deeper western bay. Northern/southern bay arms are centred at
angles +/-2.48 radians; the exposed eastern promontory is at 0.35 radians. Cosine-distance
lobes keep both the function and its slope continuous across the polar seam.

The first 37 m bay displacement looked nearly straight in the new western inspection
camera despite passing a radial-radius comparison. It was rejected visually. The final
70 m displacement has a 159.026 m central radius, with regional shoreline extrema of
158.994 / 258.680 m. A world-space regression now requires the bay shore to lie at least
25 m behind **both** headland tips, in addition to bounds, periodicity and the eastern
promontory check. The old blanket 185 m minimum-radius test was deliberately replaced
with a 150 m connected-interior bound to permit this authored bay.

All consumers still use the same `IslandTerrain::ShoreRadius` and terrain queries:
rendered coastline, foam, vegetation, props, shipwreck and marine habitats. No independent
visual shoreline mask was introduced. The signed distance remains a radial approximation,
not a Euclidean distance field; coast bands can therefore vary in apparent width.

## Turtle bank steering

The deeper bay exposed a sea-turtle movement failure near a shallow bank. At its home
centre the invalid-step correction aimed at its own location, while observer avoidance
also had a zero-length direction. A turtle could repeatedly reject movement without
escaping. Steering now tests one metre ahead and chooses homeward water when displaced,
or radially offshore at the home centre. Invalid steps turn toward that selected heading
rather than resetting to the degenerate home vector. The existing four-second observer
separation regression is retained unchanged, along with habitat, determinism and depth tests.

## Measured state

World fingerprint: `9dc988ef0ffd9f67`. Population: 362 palms, 264 canopy trees,
1,760 understory, 588 bushes, 2,972 ground plants, 18,720 grass clusters, 172 rocks
(14 offshore), 10 logs, 28 driftwood, 248 beach-detail clusters and 94 fallen fronds.
Clearing accents: 12 bushes / 60 plants. Forest colonies: 186 / 742 companions.
Coastal shadow geometry: 28 patches / 23,772 vertices. Terrain mesh budgets unchanged.

## Evidence and remaining scope

The `bay` fixed viewpoint uses (-365, 115, 0), facing east with a downward pitch of
0.38 radians. It is available through the existing contained native/Android launchers.
Native bay `20261011T074941.874456Z-bay` passes and was inspected. Earlier bay
`20261011T074839.528996Z-bay` is the rejected shallow design. Native overview
`20261011T074701.584126Z-overview` also predates the deeper bay and is not final evidence.

Final 33/33 non-display regression groups pass (37.13 s), including the unchanged turtle
separation test, habitat/depth/contact checks and shoreline walking/swimming transects.
Native turtle capture `20261011T075118.375540Z-turtles` passes and was inspected. All
captures used contained software rendering; no new GPU faults, hardware latch unchanged.
Android x86_64 build/lint succeeds. Bay run `20261011T075311.442314Z-bay` passes and its
image was inspected against native. Interactive `20261011T075435.702413Z-interactive`
fails swimming after one fixed nine-second gesture: software frames are approximately
0.7 s, while movement steps are capped at 0.1 s. Replaced the gesture-plus-idle-wait with
repeated real gestures within a 100 s deadline, retaining swimming and transition-audio
assertions. A device-free regression executes the actual shell block with fake touch/time/log
providers: success on gesture three, failure at the deadline if either evidence gate is absent.

Identical-APK run `20261011T075739.370693Z-interactive` confirms touch sensitivity,
land footsteps and swimming/transition audio after two gestures. Its Home/resume restores
surface/focus and the same PID 1869, but fails missing AAudio pause/start evidence. No fatal
swap error in this run; do not conflate it with or close `CNA-ISLAND-001`. Both kernel deltas
are empty. Cold relaunch was not reached. Android lifecycle remains degraded.
Tested x86_64 APK: 20,433,195 bytes, SHA-256
`318c88535538fea89045a58a52f71e59dddc12cce577be86dbd4db0b5d7b8d46`.
Default ARM64 assembly/lint restored: 20,518,644 bytes, SHA-256
`35ae8bea06eaaecb1cf4021295bfb7e4f2bb56041d0e9c4862a03eafeb4b5823`;
physical-device runtime untested. Android/browser preflight and command-line regressions pass,
as do shell and JavaScript syntax checks. Web build/runtime qualification remains pending; the terrain
suite now includes `bay` as a fifth view. Previous ridge qualification is not evidence for
this changed coast. AMD hardware remains stopped.

This stage changes the silhouette, not coast ecology/geology. Headlands are not yet rocky,
the sand still forms a continuous band, and neither a dominant beach nor a cliff has been
implemented. Those require the shared environment field and material/placement integration.
Do not mark the complete macro-geography or coast-type backlog finished.
