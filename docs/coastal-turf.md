# Shared sandy-substrate response for turf

Status: native implementation qualified, 2026-10-11; Android/Web pending.

The wider bay exposed a remaining independent shoreline band in the grass system.
`GroundCover::Density` now accepts normalized rockiness and beach suitability, in
addition to its existing canopy light, patch richness, distance and slope controls.
The CPU and generated GLSL use the same constants: sandy-substrate suppression rises
smoothly between suitability 0.15 and 0.70, multiplied by one minus rockiness.
Grass placement and terrain turf undercoat both consume this response. Existing
distance/height support bounds remain; this is not the full biome-field migration.

The placement acceptance predicate additionally rejects suitability at or above
0.70. This applies to parent and companion tufts, before the density floor and route
boost, so those two exceptions cannot reintroduce grass on the sandy apron.

Tests cover monotonic bounded response, independent bare-rock/sand rejection,
invalid coastal inputs, a same-distance bay/ordinary-coast contrast and every
placed tuft's exclusion from the sandy apron. Inputs to placement remain analytic;
terrain shader inputs are bilinear RGBA8 samples, not bit-identical samples.
No additional texture, vertex attribute or draw pass is introduced.

`bay-ground` adds an eye-height review camera near (-147,-8), looking along the
western beach. It is mapped through the native CLI, contained capture allowlist,
Android launcher and Web smoke template. It is not yet part of an automatic Web
view matrix. Existing `beach` remains the inland-looking primary path camera.

## Verification

Native build succeeds. Final non-display suite passes 33/33 groups (52.01 s).
The first run passed core assertions but required the CLI help expectation to be
updated for the new camera; the second full run passes without weakened predicates.

Contained native captures pass and were inspected: `20261011T091417.768738Z-bay-ground`,
`20261011T091436.406649Z-grass`, `20261011T091521.122820Z-bay`. All kernel deltas are
empty; the hardware latch is unchanged. The ground view exposes the wide sandy apron
and sparse transition into vegetation; the inland grass view retains its cover and
floor detail. The distant bay composition remains intact. This is a modest substrate
consistency change, not a wholesale vegetation redistribution.

Fingerprint `5d0b62ad43a25895`, 17,225 grass patches (two fewer). Other populations
match the wider-beach stage; rendered jungle root gaps remain zero. Web grass count
pin updated from the generated world report. Web harness syntax and diff checks pass.
Android/Web runtime qualification remains pending. The completed wider-beach Web suite
does not qualify these newer changes. No hardware rendering is authorized; Android
lifecycle remains separate and unresolved.

The close bay camera also exposes a useful next realism target: dry sand still looks
too smooth, with broad pigment fields and sparse flecks but little readable fine
relief. Footprint-filtered wind ripples/grain should be reviewed here before calling
the beach material finished. Rock strata and the broader environment model remain open.
