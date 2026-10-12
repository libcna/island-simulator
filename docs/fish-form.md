# Shallow-water fish form

## Image-driven repair

The previous close fish was an eight-triangle diamond body with flat single-triangle fins. In the
fixed `fish` view it read as intersecting coloured wedges rather than a living fish. The baseline
contained llvmpipe capture is
`build-consumer/software-captures/20261012T205724.527481Z-fish`.

The revised close model lofts six body stations with eight radial facets. Its forehead rounds into
a small snout, the maximum body depth sits ahead of the midpoint, and a narrow caudal peduncle now
separates the body from the forked tail. Darker dorsal facets and a pale belly follow the actual
body surface. Two-facet dorsal curvature, separately rooted pectoral fins, enlarged surface-fitted
eyes, gill creases and a short mouth crease preserve recognizable features from side and oblique
views. Palette variants retain modest width/depth differences instead of only changing colour.

The existing simulation still supplies depth pitch, turn bank and a progressively bent rear body.
The new loft applies that bend along the last four stations so the tail motion no longer starts at
a single rigid hinge. Population, school routes and the distant 12-vertex silhouette are unchanged.

## Cost and evidence

The close geometry rises from 51 to 276 vertices per visible fish. The fixed 48-agent capacity is
13,248 `VertexPositionColor` vertices, about 212 KiB, and all visible fish remain in one dynamic
draw. The inspected side-on capture
`build-consumer/software-captures/20261012T210406.805334Z-fish` passed image, OpenGL, runtime and
kernel-delta gates with six close fish in one draw. It is a substantially clearer low-poly fish
school, not a claim of photorealism or a measured hardware performance result.

The renderer checks every generated close fish against the compile-time 276-vertex budget, and the
native/Web smoke diagnostics pin the same inventory. Actual WebGL and Android captures, sustained
performance, scale/colour variety, fin articulation and further texture/material polish remain.
