# Grass distance continuity

## Finding

The previous custom grass path rejected a complete cell when the midpoint of its
anchor bounds exceeded the quality profile's range (135 m full / 112 m mobile).
Cells span 37.5 m. Individual anchors could still be inside range when the cell
vanished. This is distinct from the existing staggered near/middle/far mesh
replacement, and neither problem is solved by MSAA.

`GrassVisibilityTests.cpp` groups the actual 18,556 deterministic placements using
the renderer's shared cell mapping. For each occupied cell it probes 16 camera
directions, 1 cm outside the old centre cutoff. At both quality radii it finds
1,870 cell/view combinations containing anchors still in range, a maximum of
134 such anchors in one combination, and a maximum premature distance of
26.2993 m. The combined counts are 142,283 anchor observations at 112 m and
143,308 at 135 m; repeated observations are **not** unique plants.
This is a horizontal coverage audit, not a measurement of image visibility:
terrain, vegetation occlusion and camera framing can hide these anchors.

## Correction

The custom shader computes `1 - smoothstep(start, end, anchorDistance)` in the
horizontal plane. End is the existing quality radius; start is 16 m earlier,
clamped to zero for unusually short configured ranges. This is per cluster,
not per cell or individual vertex. It requires one additional vec2 uniform;
instance attributes, buffers, textures, mesh topology and populations are unchanged.

The weight contracts the entire footprint toward its anchor, scales blade height,
and scales wind/player displacement. Terrain sampling occurs at the contracted
footprint. At weight zero all positions collapse onto the same sampled anchor,
including seed geometry. Scaling only height would leave flat depth-writing
triangles, so it is deliberately insufficient here. Rest-shape normal calculation
does not divide by the visibility weight and remains finite at zero.

CPU rejection now uses distance to the nearest point of the cell's anchor rectangle.
Only a cell with all anchors beyond the radius can be distance-rejected. Existing
geometry/motion/terrain-range bounds remain conservative under contraction and are
still used for frustum culling. Centre-based LOD selection is unchanged, as are
the 62–78 m / 86–102 m population transition bands. The same GLSL source is used
for desktop 330 and ES 300. No alpha blending, dither, temporal noise, extra render
pass, dynamic instance upload or framework modification is introduced.

BasicEffect is a degraded path: it retains the older centre cutoff and does not
perform contraction or exact terrain reprojection. Runtime qualification rejects
this fallback; it must not be presented as equivalent quality.

## Cost and validation

The native grass viewpoint now submits 4,109 patches in 22 cells / 28 grass draws,
versus 2,827 in 15 cells / 21 draws before the change. Near/middle counts remain
957 / 637; all 1,282 additional submitted instances use the 42-vertex far mesh.
This is 53,844 additional vertex instances, approximately 5.9% above the former
914,940 grass vertex instances for this view, not a measured frame-time increase.
Submitted counts include fully contracted or occluded clusters; they are not
visible pixel counts. Physical GPU/mobile cost remains unqualified.

CPU tests check endpoints, midpoint, monotonicity, flat endpoint slopes, short
ranges, invalid inputs, rectangle corners/interiors/single anchors, and cell mapping.
Actual-layout probes check every anchor against the conservative rectangle bound
and assert that every distance-rejected cell has only zero-weight anchors. A second
set of probes 27 m beyond each centre cutoff exercises fully faded rejection.

Native incremental build and all 32 non-display CTest groups pass (20.84 s).
The contained llvmpipe/private-pixman capture
`build/software-captures/20261009T083728.138463Z-grass/` passes shader, image and 4x
MSAA gates, with empty live/final kernel deltas. The inspected 1280x720 frame
preserves foreground roots, blades, ground and foliage. A still frame cannot
establish temporal smoothness of distant coverage behind occluding vegetation.

Contained Chrome/SwiftShader walking check
`build/browser-software/20261009T083852.440443Z-chrome-grass-motion/` passes:
18.1906 m outward, 1.1761 m return error, 105 telemetry samples, 53 grass population
states and ten canopy identities changing mesh tiers. All 108 recorded frames are
distinct, with no truncation; GL error is zero, context remains live and both kernel
deltas are empty. Inspect samples 0/20/40/60/80/107 in
`build-probe/island-grass-distance-walk.png`. They retain the walking corridor and
foreground coverage, but the vegetation remains sparse/stylized. This six-frame
review is not an exhaustive temporal metric or proof of invisible mesh switches.
The Web release package validates at 13,881,283 raw / 6,963,488 gzip bytes.

Android x86_64 assembly/lint passes (6 s). The isolated API 35 KVM/SwiftShader
grass check `build/android-software/20261009T084206.226212Z-grass/` passes the
new shader diagnostic and requested/applied 2x MSAA, app PID 1830 / entry TID
1925. The 1280x720 PNG is inspected; kernel deltas are empty and the owned
emulator exits zero. This fixed-view check does not renew touch/lifecycle tests.
Default ARM64 assembly/lint is restored (4 s), with only arm64-v8a native libraries
in the APK. Physical-device runtime and performance remain unverified. Current
package identities are recorded in `build-android.md`.

Final dense-jungle native capture `20261009T084424.287480Z-jungle` also passes and
is visually inspected, with empty live/final kernel deltas. The final 32-group
non-display suite passes again (24.05 s). Capture/startup validators in that suite
may inspect retained artifacts; the named capture/browser/emulator runs above are
the actual new runtime checks. No owned game, Weston, Xvfb or emulator process
remains after the checks, and the exact owned Chrome PID is absent. The hardware
stop-latch hash is unchanged.

The AMDGPU incident remains open. These changes address application visual
continuity, not its unidentified GPUVM fault. No hardware qualification or
dependency edits are implied.

## Remaining visual work

Review longer open sightlines during travel and the apparent shrinking transition;
geometric contraction is not an exact opacity fade. Investigate near/middle/far
silhouette replacement separately. Existing sparse cover, simplified vegetation,
approximate wind normals and shader/texture shimmer remain. Passing a walking
harness or a startup marker alone is not a perceptual realism verdict.
