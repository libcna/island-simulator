# Shallow-water fish form

## Image-driven repair

The previous close fish was an eight-triangle diamond body with flat single-triangle fins. In the
fixed `fish` view it read as intersecting coloured wedges rather than a living fish. The baseline
contained llvmpipe capture is
`build-consumer/software-captures/20261012T205724.527481Z-fish`.

The first revised close model lofted six body stations with eight radial facets. A second inspected
pass now uses seven stations and ten facets: the extra forehead transition removes the pointed
wedge snout, while the maximum body depth remains ahead of the midpoint and a narrow caudal
peduncle separates the body from the forked tail. Darker dorsal facets and a pale belly follow the
actual surface. The tail and dorsal fin now receive two-sided fixed-sun shading, each pectoral fin
uses a shaped two-triangle outline instead of one flat triangle, and each eye is a two-facet dark
diamond with a restrained highlight. Gill and mouth creases remain surface fitted. Palette variants
retain modest width/depth differences instead of only changing colour.

The existing simulation still supplies depth pitch, turn bank and a progressively bent rear body.
The new loft applies that bend along the last four stations so the tail motion no longer starts at
a single rigid hinge. Population, school routes and the distant 12-vertex silhouette are unchanged.

## Cost and evidence

The close geometry rises from the original 51 through 276 to 414 vertices per visible fish. The
fixed 48-agent capacity is 19,872 `VertexPositionColor` vertices, about 318 KiB, and all visible
fish remain in one dynamic draw. The closer QA camera follows only the first subject's translation,
so separated-time captures retain it without freezing heading, bank or tail movement. Inspected
native evidence `build-consumer/software-captures/20261013T031027.224425Z-fish` passed image,
OpenGL, runtime and kernel-delta gates with six close fish in one draw. Inspected focused
Chrome/SwiftShader evidence `build/browser-software/20261013T031048.022591Z-chrome-fish` retained
live fish in two different-hash frames across 60 positive intervals with GL error zero, an intact
context and an empty kernel delta. It is a clearer low-poly school, not a claim of photorealism or
a measured hardware performance result.

The renderer checks every generated close fish against the compile-time 414-vertex budget, and the
native/Web gates pin both that inventory and the current anatomy marker. Actual Android execution,
sustained performance, scale/colour variety, fin articulation and further texture/material polish
remain.
