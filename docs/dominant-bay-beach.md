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
Android and Web builds/runs for this stage are pending; earlier rocky-coast evidence
does not qualify this revision. The Android lifecycle issue remains open and the
AMD hardware stop latch remains in force.
