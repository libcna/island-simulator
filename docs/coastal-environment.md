# Shared coastal geology and material field

2026-10-11, first integrated coastal subset of macro-geography items 2–3/19.

`IslandEnvironmentField` now provides height-independent regional rock potential and
normalized coastal rockiness/beach suitability. The same authored headland directions used
by the shoreline define these broad geology lobes. The field fades offshore and inland,
leaving the existing interior ridges independent. Non-finite queries return empty controls.
This is **not** the complete planned moisture/soil/drainage/wind/reef environment model.

## Integrated consumers

- Terrain height raises a coastal bench by up to 7.5 m times the regional rock potential,
  rising between 2–12 m inland and merging back into the interior between 32–70 m. The
  headland now has a physical steep face, not only rock-coloured sand. Offshore bathymetry
  and the mean shoreline are unchanged.
- `TerrainSample` carries both controls and classifies strongly rocky dry terrain as
  `RockyGround`, separately from both sandy habitat and geometrically steep ground.
- Adult/juvenile palms and canopy exclude strongly rocky roots; understory, bushes and
  ground plants use a looser cutoff. Grass density also decreases with rockiness.
- `CanopyLighting` packs the controls into existing blue/alpha channels. Red canopy light
  and green cover patches are unchanged. The terrain shader reads those same coastal
  values to suppress sand/turf and expose existing filtered weathered-rock detail.
  Texture size, terrain vertex stride, mesh counts and draw passes are unchanged.

CPU placement uses analytic field samples; GPU material controls are RGBA8 texel-centre
samples on the existing canopy-light grid with bilinear interpolation. They are not claimed
to be bit-identical. No extra sampler or fragment noise loop was added. The alpha channel
is data only: this field is not used as rendered opacity.

## Wildlife and player access

The first integrated test exposed eleven authored crab starts on now-unsuitable coastal
relief. Initialisation now searches alternating angular offsets around each authored colony
(bounded to 97 candidates), accepting a real sandy, gently sloped site. Failure to find one
throws rather than silently spawning an invalid crab. Population remains 30, and the existing
long-running crab habitat and determinism assertions are retained.

Rock faces intentionally reject uphill traversal. The existing six swimming round trips
now use adjacent sandy access for stations whose rock potential exceeds 0.55; independent
regression drives a player at the actual eastern cliff and requires dry-footed rejection
before the cliff top. No movement slope limit or water transition threshold changed.

## Qualification

First complete non-display run after habitat/access fixes: 33/33 pass (43.55 s). Final
33/33 groups pass (55.12 s), including additional root-exclusion and invalid-field regressions.
Native headland `20261011T081910.752616Z-headland` and bay
`20261011T082154.597630Z-bay` pass and were inspected. The latter shows a sheltered pale
beach between two rocky arms. Final headland `20261011T082316.930694Z-headland` also
passes and was inspected. Final bay/headland kernel deltas are empty; hardware latch unchanged.
Android/Web build/runtime qualification has not yet been repeated for this changed terrain
and shader. Previous coast-shape qualification is not evidence for this new stage. The Web
terrain suite now includes the sixth `headland` viewpoint.

Current fingerprint `b0f79ae14adeb8cd`: 328 palms, 266 canopy trees, 1,777 understory,
592 bushes, 2,993 ground plants, 17,251 grass clusters; 164 rocks (14 offshore), 10 logs,
22 driftwood, 184 beach details and 94 fallen fronds. Clearing accents 12/60, forest
colonies 188 with 751 companions. Browser population pins updated from this report.

## Remaining work

Keep the full environment-field design open: coast orientation/exposure, soil depth,
drainage/moisture, reef suitability and habitat-driven prop distribution remain. The sandy
regions still share their old broad geometric profile; a deliberately wider dominant beach,
wave-energy response, broken rock strata and local talus are future stages. No full coast-
type or environment backlog item is marked complete. AMD hardware remains stopped.
