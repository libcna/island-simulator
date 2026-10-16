# Canopy trunk epiphytes

The 2026-10-16 vegetation audit accepted the established crown mass, bark material and traversable
forest floor, but found that the long clear space between buttress roots and crowns still repeated
too uniformly. Three of the eight ordinary broadleaf forms now carry a single elevated bird's-nest
epiphyte rosette. Eight individually pitched, rolled and length-varied pointed leaves grow from a
radially offset trunk attachment. They use the existing live-leaf material and wind channel rather
than a billboard or a new texture.

The feature is near-LOD only and excludes mangroves. Each affected mesh gains 56 vertices and 48
triangles, so only an affected nearby tree pays that geometry cost. It adds no population object,
collision, texture, material, buffer, shader input or draw call. Variant zero is now 4,874 vertices
and 4,688 triangles near while its 1,071/1,120 far tier remains byte-for-byte unchanged.

Native current-binary evidence `20261016T041115.046293Z-jungle` passes at 1280x720 with 275 colour
bins, 46.22 luminance deviation, 213.01 spatial contrast and an empty kernel delta. Targeted canopy
evidence `20261016T041218.426519Z-canopy` passes at 375/32.91/146.72 and confirms that the changed
near mesh remains inside its measured culling bounds. `IslandCoreTests`, leaf geometry, canopy
lighting/visibility and capture gates pass. Focused Chrome/SwiftShader evidence
`20261016T042604.764619Z-chrome-canopy-light` passes its jungle view with 60 finite positive
intervals, GL error zero, a live context and kernel `[]`. The earlier broad vegetation attempt
`20261016T041449.453724Z-chrome-vegetation` exceeded its multi-scene software-capture time limit
and is rejected rather than treated as evidence.
