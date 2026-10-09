# Grass grounding audit and correction

This is a game-side placement defect, unrelated to the unresolved AMDGPU GPUVM incident.
CNA and Sharp Runtime were not changed.

## Finding

`GrassLayout` stores analytic terrain height. Previously `GrassRenderer` placed each shared
cluster in the tangent plane of that analytic surface. `TerrainRenderer`, however, draws a
241 by 241 vertex mesh with a top-right/bottom-left triangle diagonal and approximately 2.833 m
spacing. An analytic anchor and tangent plane need not agree with those triangles.

The deterministic audit covers both root edges of all 48 short blades at all 18,556 anchors:
1,781,376 samples, including 41,664 in the east clearing. It reconstructs instance yaw, spread
and the shader's packed-normal basis. Wind and interaction weights are zero at these roots.
Reported errors are root height minus the rendered triangle height, not distance to the analytic
collision field. These CPU measurements are not GPU position readback.

| Placement model | Minimum error (mm) | Maximum error (mm) | Mean absolute error (mm) |
|---|---:|---:|---:|
| Original analytic anchor/basis | -141.709 | 144.386 | 15.9821 |
| Mesh anchor height, analytic normal (rejected alternative) | -92.415 | 111.375 | 7.40087 |
| Mesh anchor height and face normal (rejected alternative) | -146.683 | 195.311 | 4.35557 |
| RGB24 surface reprojection, CPU shader emulation | -0.018120 | 0.019074 | 0.003510 |

Moving only the anchor improves the mean but leaves centimetre-scale errors. A mesh-face normal
also fails when a footprint crosses into adjacent triangles; it worsens the largest deviations.
Neither alternative is used in the game. The original model buries 25.33% of audited roots by
more than 1 cm; the correction's CPU emulation has none beyond 0.1 mm in either direction.

## Implementation

`RenderedTerrainSurface` caches exactly the heights uploaded by `TerrainRenderer`. Its exact CPU
sampler uses the actual rounded grid coordinates and the two triangle planes, not bilinear
interpolation. Tests independently verify all 58,081 grid vertices, cross-product planes/normals,
shared edges, footprint ranges and invalid inputs.

The custom grass path owns a single immutable 241 by 241 RGBA8 texture (232,324 bytes). RGB
encodes height in [-64,64] m using 24 bits; alpha is opaque. Out-of-range/nonfinite heights are
rejected before upload. There are no mipmaps, interpolation of packed bytes, render targets,
floating-point texture requirements or dynamic uploads. Standard CNA `Texture2D`/`ShaderEffect`
APIs bind it to unit 1; unit 0 retains canopy transmission. Each vertex makes three level-zero
`texelFetch` reads and reconstructs the matching triangle height. The same source is used by
GLSL 330 and GLSL ES 300.

Before wind/interaction, the vertex's undeformed footprint is draped onto that surface, then
its blade height is added along the existing normal basis. Roots remain planted while blades
retain bending. The canopy receiver also uses the reprojected base height. Root normals and
animated normals remain approximate; this is positional grounding, not full differential lighting.
Player collision, island layout, mesh/instance counts and draw calls are unchanged.

Cell bounds include the minimum/maximum heights of all grid vertices covering each patch
footprint, plus the existing tested motion/geometry envelope and margin. The old fallback box
is retained in that union. The stock-effect fallback does **not** perform reprojection; it retains
its older approximate placement and emits the existing fallback diagnostic. Qualification rejects
that fallback. The temporary CPU height cache/upload data are released after setup.

## Reproduction and limits

```sh
export CCACHE_DIR=/rv/cnaccache CCACHE_BASEDIR=/rv
cmake --build --preset linux-debug
ctest --test-dir build -R 'Island(RenderedTerrainTests|GrassGroundingReport)$' --output-on-failure -j2
build/island-grass-grounding-report build/grass-grounding-rgb24-report.json
```

The report fails if its packed-float reconstruction exceeds 0.1 mm error. It records worst-root
coordinates and anchor/blade/edge indices; the complete before/after reports are retained in
`build/grass-grounding-analytic-report.json` and `build/grass-grounding-rgb24-report.json`.
The CTest report uses `build/island-grass-grounding-report.json`.

Do not equate this numerical audit with hardware qualification, measured mobile cost, dense
realistic turf or smooth moving-camera LOD. The native llvmpipe capture
`20261008T234516.431168Z-grass` exercises the new shader with zero GL errors/fallback and an empty
kernel delta; its inspected frame retains the same 2,827 visible patches/15 cells/21 draws.
Serial SwiftShader grass/jungle/fronds/east scenes also pass, with 60 presentation intervals
each, zero GL errors/context loss/fallback, and nine empty kernel checks (Chrome PID 3787236).
The grass/east images were inspected; this is visual evidence rather than numerical GPU readback.
Android arm64 assembly and lint pass in 5 s, but no current Android runtime was exercised.
The AMD hardware stop latch remains active and the original GPU fault's cause remains unknown.
