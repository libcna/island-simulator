# Canopy LOD review — 2026-10-09

## Method and scope

The `canopy` viewpoint is an additional explicit diagnostic camera. It requires
`--canopy-lod-inspection X_MM Z_MM near|far`; ordinary gameplay cannot select it by itself.
It sits radially outward from the tree at that tree's actual horizontal LOD distance and aims
at the measured crown bounds from crown height. No neighbors or other scene layers are hidden.
This is elevated static inspection, not first-person traversal or proof of unobstructed coverage.
The CPU world report now lists one deterministic outermost tree per variant in
`placements.canopyReviewTargets`. Use the listed millimetre coordinates for each serial pair:

```bash
build/island-world-report --output build/island-world-report.json
jq '.placements.canopyReviewTargets' build/island-world-report.json
# Substitute the coordinates reported above; run near and far separately.
sg systemd-journal -c 'python3 tools/run_software_capture.py build canopy --canopy-lod-inspection X_MM Z_MM near'
sg systemd-journal -c 'python3 tools/run_software_capture.py build canopy --canopy-lod-inspection X_MM Z_MM far'
```

Check actual target visibility and the logged camera/variant/threshold before interpreting
differences. The older east-camera evidence below remains unchanged and independently useful.

As of the 2026-10-15 world fingerprint `f662f6e9ac0f9c45`, the current targets are:
`-21985:-186920`, `-154087:135164`, `-133291:146013`, `-166856:83909`,
`-156957:-107255`, `-2679:-179962`, `-114516:166946`, `-95327:166368`, and
`-164496:-25056` for variants 0 through 8 respectively. Earlier literal coordinates in this
historical document describe their own retained captures; they are not valid current layout
targets and must not be copied into a new run without consulting the world report.

Use the capture-only `--canopy-lod-inspection X_MM Z_MM near|far` option through
`tools/run_software_capture.py`. See [Linux instructions](build-linux.md) for the two serial
commands. These are forced llvmpipe/private-pixman inspections, not AMD qualification. The normal
scene validator rejects diagnostic markers unless this exact inspection was requested.

The fixed east camera is unchanged; simulation is frozen at zero elapsed time and shaders at
one second. Only the selected tree's LOD changes. The target `-87120:-15369` is variant 1, with
a 125.736 m LOD threshold, about 130.15 m from the camera in X/Z. Two initial near captures are
byte-identical (`cmp` exits zero), establishing determinism for this pair on this software stack.
It is partly occluded by a nearer trunk/crown, so the result covers only the visible portion.
A second submitted target, `-87715:-42400`, produces identical near/far images: it is not useful
visible-transition evidence. Submission telemetry does not prove that a tree reaches the image.

All evidence directories below are under `build/software-captures/`; all runs have empty live
and final kernel deltas, no GL errors/material fallback, and 1280x720 captures. Preserve these
iterations instead of keeping only the most favorable result.

| Geometry | Near directory | Far directory | Changed pixels | Normalized mean absolute RGB error |
|---|---|---|---:|---:|
| Original four merged far lobes | `20261009T005416.659433Z-east` | `20261009T005420.727930Z-east` | 222 | 0.0000303366 |
| Eight lobes, enlarged cores, replacement leaves | `20261009T005642.738721Z-east` | `20261009T005646.783041Z-east` | 218 | 0.0000314925 |
| Shared core scales/contours, replacement leaves | `20261009T005938.411254Z-east` | `20261009T005942.167108Z-east` | 211 | 0.0000350839 |
| Shared cores and retained original leaves | `20261009T010205.339228Z-east` | `20261009T010209.311080Z-east` | 181 | 0.0000241921 |

The repeat near capture is `20261009T005502.394304Z-east`. The completely occluded second-target
pair is `20261009T005454.391012Z-east` / `20261009T005458.381787Z-east`.

Measure each pair with `MAGICK_THREAD_LIMIT=2 magick compare -metric AE NEAR.ppm FAR.ppm null:`
and `-metric MAE`; exit 1 means the images differ, not an invocation failure. MAE is averaged over
the whole image, so the small number must not be presented as universal visual fidelity. The
original difference occupies 39x38 pixels at X=599, Y=259. Inspection sheets under `build-probe/`
use the same 70x70 crop at X=580, Y=240 enlarged with nearest-neighbor filtering; they expose
pixels, not additional rendered detail. Sheets are `island-canopy-lod-pair-one.png`,
`island-canopy-lod-pair-revised.png`, `island-canopy-lod-pair-shared-core.png` and
`island-canopy-lod-pair-retained-leaves.png`.

## Result and cost

Eight anchors alone did not improve the colour difference. The final tier retains six evenly
distributed original three-leaf sprigs per lobe plus the original perimeter/lower leaves. It
reuses their exact positions, shapes, colours and wind parameters. Both tiers share core
dimensions/contour functions, while far cores have lower tessellation. No alpha fade, extra
pass, new texture or change to tree placement/collision is introduced. The near geometry and
fixed-sun bake remain unchanged.

For this visible portion, changed pixels decrease by 18.5% and colour error by about 20.3% from
the original. The enlarged crop shows less replacement of porous leaf edges by unrelated solid
masses. Significant differences remain: leaf removal, core tessellation, missing branches and
lianas, and material normals still change coverage/colour. This does not close moving-camera
review, other variants/angles, or mobile performance qualification.

Far geometry increases from 457 vertices/464 triangles to 997/952 per variant. Near variants
remain 4912/4328 or 4949/4378. All jungle vertex payload grows by 112,320 bytes to 1,404,468 bytes;
the four far index buffers add 11,712 bytes. The visible jungle's 46 far trees therefore add
22,448 submitted triangles, with no extra draws. This is a deliberate visual-quality tradeoff,
not a free optimization. Hardware measurements remain suspended behind the incident latch.

The final normal jungle capture is `20261009T010213.376912Z-jungle`: 191 colour bins,
41.27 luminance deviation, 205.49 spatial contrast. It uses normal time and LOD selection,
without the inspection override. Browser and platform validation are tracked in `PLAN.md`.

## Next review

### Four representative targets and retained primary branches

The outward diagnostic camera now covers all four variants, selected by the CPU world report.
The target crowns are visible; nearby palms still cover parts of the lower trees, especially
variant 1. This improves coverage beyond the original east view but is not fully unobstructed
all-angle or walking-camera qualification. All 16 original/revised captures pass software scene
validation and have empty `kernel-live.jsonl` and `kernel-after.json`, with no recorded failures.

| Variant / target (millimetres) | Original near / far timestamps | Revised near / far timestamps | Changed pixels before → after | Normalized RGB MAE before → after |
|---|---|---|---:|---:|
| 0 / -87082:-170810 | 042556.991102 / 042611.829179 | 042830.677552 / 042804.477874 | 1175 → 1152 | 0.0000559016 → 0.0000552364 |
| 1 / 163600:126958 | 042625.147520 / 042628.904326 | 042834.434268 / 042808.235070 | 346 → 333 | 0.0000212333 → 0.0000206177 |
| 2 / 187360:27079 | 042632.660704 / 042636.415819 | 042838.187238 / 042811.998395 | 1044 → 1021 | 0.0000656318 → 0.0000629964 |
| 3 / -82985:174885 | 042640.170939 / 042643.978434 | 042841.940690 / 042815.758321 | 1171 → 1112 | 0.0000810157 → 0.0000737549 |

Directories use `20261009T<TIMESTAMP>Z-canopy` under `build/software-captures/`.
The revised far tier retains the same seven primary support branches as near: +84 vertices and
+112 triangles per far mesh, from 945/952 to 1029/1064. Four shared far meshes add 17,472 vertex
bytes and 2,688 index bytes, no draw/texture. Jungle vertex payload is now 1,350,388 bytes.
Near meshes and foliage are unchanged. These branches are already only four-sided/two-segment;
further simplification would introduce another mismatched shape at this boundary.

Both sides of each revised pair were recaptured: changing all far trees also changes background
trees, so mixing an old near image with a revised far image would confound the comparison.
Differences are confined to the selected tree's image region. The 160x160 crop at X=560/Y=280,
enlarged 3x with nearest-neighbor filtering, is retained in
`build-probe/island-canopy-variant{0,1,2,3}-lod.png`; the four revised pairs use
`island-canopy-variant{0,1,2,3}-branches-lod.png`. All eight crops were inspected.
The correction gives a modest consistent improvement, not seamless LOD.
Core tessellation/lighting, leaf removal, trunk geometry/material bands, roots and lianas still
change. Next investigate trunk height-dependent colour continuity and moving-camera visibility
before spending more triangles on the remaining differences. Hardware cost is not measured.

### Height-based trunk pigment follow-up

The same four outward targets were recaptured after replacing ring-parity pigment with the
shared linear height field. Original branch-pass pairs in the preceding table are the baseline.
All eight new runs pass strict software checks with empty live/final kernel deltas. Geometry,
wind, camera/bounds, texture/vertex formats and shader detail are unchanged.

| Variant | New near / far timestamps | Changed pixels before → after | Normalized RGB MAE before → after |
|---|---|---:|---:|
| 0 | 044112.709663 / 044127.743074 | 1152 → 1147 | 0.0000552364 → 0.0000547088 |
| 1 | 044116.465616 / 044131.561780 | 333 → 318 | 0.0000206177 → 0.0000204646 |
| 2 | 044120.231923 / 044135.323639 | 1021 → 982 | 0.0000629964 → 0.0000626674 |
| 3 | 044123.985318 / 044139.134534 | 1112 → 1077 | 0.0000737549 → 0.0000730585 |

Use the same `20261009T<TIMESTAMP>Z-canopy` path convention. The enlarged variant-2 pair is
`build-probe/island-canopy-variant2-pigment-lod.png`, inspected at the same crop/scale as before.
The ordinary jungle frame `20261009T044207.629306Z-jungle` was also inspected. The improvement is
small but consistent and removes a known material-identity mismatch without adding GPU cost.
Core/leaf/normal/geometry changes still dominate the remaining transition differences. Do not
infer seamless transitions, universal coverage or performance from these whole-image numbers.

### Spatial leaf posture follow-up (earlier pass)

The October 9 posture pass retains the same source leaf identities in both tiers and does not
change mesh counts. A fresh pair was captured immediately before rebuilding, after the intervening
material changes, so the older metrics above are not silently reused as this pass's baseline.
All four frozen captures use the same east target `-87120:-15369`, shader time one and zero
simulation time. They pass strict software rendering checks with empty live/final kernel deltas.

| Geometry | Near directory | Far directory | Changed pixels | Normalized mean absolute RGB error |
|---|---|---|---:|---:|
| Horizontal blades / current bark | `20261009T035350.086814Z-east` | `20261009T035444.374160Z-east` | 180 | 0.0000253282 |
| Spatially posed blades | `20261009T035754.704151Z-east` | `20261009T035758.724840Z-east` | 153 | 0.0000277792 |

The result is mixed: 15% fewer changed pixels, approximately 9.7% more aggregate colour error.
The same 70x70 crop at X=580/Y=240 is inspected at 4x nearest-neighbor enlargement in
`build-probe/island-canopy-posture-lod-pairs.png` (before near/far, after near/far, left to right).
The remaining changes include brighter sky-edge differences. This is not proof of improved
overall LOD quality, seamless switching or all-angle coverage. The normal jungle view shows
more spatial crown silhouettes, but the target here is still partly occluded. The broader
unobscured/moving review below remains necessary. An initial metric command mistakenly placed
`-limit` before the `compare` subcommand and failed; the reported results use the successful
`magick compare -limit thread 2 -metric ...` invocation, not those errors.

### Remaining checks

The 2026-10-15 asymmetric-fill follow-up replaces the one repeated core/shell proportion with
five deterministic per-lobe scales. Both tiers share these scales and geometry counts are
unchanged. Current variant-0 near/far evidence is
`20261015T100248.607815Z-canopy` / `20261015T100301.936606Z-canopy`; normal jungle evidence is
`20261015T100219.613016Z-jungle`. A focused Chrome/SwiftShader jungle run at
`20261015T100953.209242Z-chrome-canopy-light` passes 60 frame intervals with GL zero and no context
loss. This is a static silhouette/fill correction, not a claim that all moving LOD transitions are
seamless.

The later curved-outline pass reuses the spatial-posture pair above as its unchanged baseline.
It keeps 24 source sprigs near and four nested source sprigs far, with paired shoots in alternate
lobes, exchanging leaf count for seven-vertex outlines at exactly the same triangle totals.
Near/far directories `20261009T041121.451577Z-east` / `20261009T041125.210872Z-east` pass with
empty kernel deltas and no GL/fallback diagnostics. Changed pixels fall from 153 to 139; normalized
mean RGB error falls from 0.0000277792 to 0.0000229708 (approximately 17.3%). The inspection sheet
`build-probe/island-canopy-outline-lod-pairs.png` uses the same crop/enlargement, before near/far
then after near/far. Static jungle/clearing images show less diamond-like foliage without gross
exposed-core gaps in those views. The mesh triangle totals remain 4,328/4,378 near and 952 far;
near vertices are now 4,620/4,657, far 945, and total jungle vertex payload is 1,332,916 bytes.
These results support the shape/coverage tradeoff for continued development, not universal LOD
equivalence or physical-GPU performance. The remaining checks below still apply.

- Inspect unobscured trees of all four variants near their own thresholds, not only this target.
- Repeat the bounded moving-camera route; retain the previous telemetry/recording gates.
- Review far-core normals and missing branch coverage before considering additional geometry.
- Measure physical mobile cost when hardware testing is safely available; do not infer it from
  llvmpipe or SwiftShader timing.
