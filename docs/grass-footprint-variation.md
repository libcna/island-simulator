# Grass footprint and pigment variation

## Visual finding

The fixed close-grass view already contained curved blades, low turf, folds, seed heads and
biome tint, but the foreground still read as repeated dark-green upright clusters. The baseline
Release llvmpipe capture `20261012T225839.281872Z-grass` passed its renderer and image gates at
188 colour bins, 22.33 luminance deviation and 66.09 spatial contrast. Full-resolution review,
not those aggregate values, identified the repeated footprint and narrow pigment range.

## Bounded correction

The existing blade-root attribute now seeds a stable per-blade planar transform together with
the instance wind phase and colour identity. One local axis contracts by 0.82--0.94 while its
perpendicular expands by the reciprocal amount. The determinant is therefore one: the transform
changes the outline without reducing nominal footprint area or adding grass. Its maximum stretch
is 1/0.82, and `GrassMotionBounds` includes that exact factor before slope, height, interaction
and wind envelopes are added.

Normals receive the inverse-transpose of the same planar transform before the existing
spread/stature, yaw and terrain-basis transforms. Roots and all vertices belonging to a blade,
including seed geometry, use one identity, so blades do not tear. Near, middle and far source
identities remain nested. The shader uses the existing 48-byte surface vertex, 44-byte instance,
buffers, terrain-height texture and draw paths; no vertex, triangle, instance, texture, uniform,
upload or draw is added.

Three restrained deterministic pigment families replace the former single base green: a warmer
olive family, the established yellow-green family and a cooler shaded family. Existing biome,
height, cured-tip, instance, canopy and fixed-sun terms still apply afterward. This is pigment
variation, not random light baked into the mesh.

## Evidence and limits

CPU tests cover determinism, invalid input, distinct instance outcomes, locale-independent shader
constants and the conservative maximum radial stretch. The independent motion-bound sampler now
uses that worst-case stretch while combining slope, stature, wind and player interaction.

The updated Release llvmpipe capture `20261012T230309.954997Z-grass` passes at 198 colour bins,
22.45 luminance deviation and 66.30 spatial contrast, with empty live/final kernel deltas. The
1280x720 image was inspected at original resolution: foreground clusters have less uniform
footprints and their olive/cool leaf families remain restrained. The matching supervised
Chrome/SwiftShader WebGL 2 vegetation run
`20261012T232521.261689Z-chrome-vegetation` passes all eight views with 60 presented intervals,
zero GL error, a live context and an empty final kernel delta; its grass image was also inspected.
The first attempt exposed and corrected an integer `smoothstep` literal in the unrelated campfire
fragment shader; later attempts exposed stale grass-population and boar-material harness markers,
which now match the current runtime inventory. These image statistics are sanity checks, not
realism scores. Moving LOD review, GLES 3 runtime validation, physical-device cost, wider field
composition and further blade anatomy remain open.
