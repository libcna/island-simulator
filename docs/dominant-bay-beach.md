# Dominant sheltered beach

2026-10-11, continuation of the coastal environment field.

The western bay now has an approximately 36 m beach profile, compared with the
ordinary 18 m coast and approximately 6.5 m at the eastern rocky headland. Width is
a smooth periodic angular field, narrowed by the existing regional rock potential.
It controls both the physical beach rise/interior blend and the sandy material
potential, replacing the shader's independent fixed-width sand band. Forest-floor
material also yields to the sandy apron. Shoreline position, offshore bathymetry,
ridge shapes and coastal rock relief are unchanged.

The field still occupies the existing canopy texture alpha channel: no new sampler,
mesh, vertex data or draw pass. This is an authored coastline profile, not an erosion
simulation or a complete environmental model. Wind-driven waves, dunes, rock strata,
moisture and soil fields remain open.

Regression checks cover the wide bay/narrow headland contrast, low physical relief
25 m inside the bay, periodic seam continuity and finite bounded width around the
coast. Existing wildlife habitat, cliff rejection, adjacent sandy access and root
contact tests remain in place. The first suite failed only the old exact vegetation
inventory assertion; its replacement values come from the regenerated world report.

World fingerprint: `08ced4994845ae68`. Inventory: 328 palms, 262 canopy trees,
1,768 understory, 588 bushes, 2,975 ground plants, 17,227 grass patches; 165 rocks
(14 offshore), 10 logs, 21 driftwood, 184 beach details and 92 fallen fronds.
Clearing accents remain 12/60, forest colonies 185 with 741 companions. Rendered
jungle root gaps remain zero. Browser inventory pins follow this report.

## Qualification

Native bay `20261011T084410.221147Z-bay` passes capture validation and was visually
inspected: a wider pale central beach separates the darker rocky arms. Empty kernel
delta. Final non-display suite passes 33/33 groups (53.06 s). Headland
`20261011T084654.624120Z-headland` and beach `20261011T084740.315381Z-beach` pass
and were inspected, also with empty kernel deltas. The existing `beach` camera now
looks inland through vegetation; it confirms ground/vegetation rendering, not a
close inspection of the new bay sand. A dedicated bay ground-level view remains useful.
Android x86_64 assembly/lint passes. First bay run `20261011T084849.011688Z-bay`
passed the old log gate but its image was black: it is NOT visual qualification.
The harness now uses `android_capture.py`, a bounded 30-second wait with shared
image-content validation, for initial/hot/cold screenshots. Nineteen CPU-only Android
tests pass, including black/truncated rejection, colourful-image acceptance, delayed
presentation and bounded failure. Shell syntax and the Android CTest group pass.
Repeat bay `20261011T085244.623709Z-bay` passes the strengthened gate and was visually
inspected; it matches native. Interactive `20261011T085027.124944Z-interactive`
passes touch sensitivity, footsteps and swimming/audio after two gestures; its image
was inspected too (this run predates the new image gate). Neither run tests lifecycle.
No GPU faults; kernel deltas contain only Wi-Fi bandwidth messages. Latch unchanged.

Tested x86_64 APK: 20,439,739 bytes, SHA-256
`8c89473af7fb392e167fef6cc4ac21de28e70fa50288044195abc4ba093c2c69`, retained at
`build/android-software/island-x86_64.apk`. Default ARM64 assembly/lint restored:
20,525,084 bytes, SHA-256
`b31230970a690adae7a24c3cd43c65084fa6db65ce214969163a0969c5c2a36c`;
physical-device runtime remains untested.

Web build/package refreshed: 10,343,092 raw / 3,528,055 gzip bytes. Contained Chrome
terrain suite `20261011T085703.088366Z-chrome-terrain` passes overview, bay, headland,
beach, east and jungle, with 60 frame intervals each. Overview/bay/headland/east/jungle
images inspected. No new GPU faults; final kernel delta contains Wi-Fi bandwidth,
EXT4 unmount and USB disconnect notices.
This qualifies the committed beach revision `53a2442`, not subsequent coastal-turf
source preparation. Full Web input/lifecycle and Firefox were not rerun.

The original no-dunes limitation was superseded on 2026-10-15 by the authoritative bounded berm,
dune, wash-channel and rocky-step work in `coastal-microtopography.md`; this section otherwise
remains the historical evidence for the broad bay profile.
The Android lifecycle issue remains open and the AMD hardware stop latch remains in force.
