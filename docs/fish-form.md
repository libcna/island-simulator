# Shallow-water fish form

## Layered palette-aware eyes (2026-10-16)

The renewed close-school audit retained the rounded head, four family palettes, surface pigment,
fins and live movement, but each eye still read as one detached black bead. The existing six-facet
oval is now a muted iris derived from that fish family's underwater-adjusted belly and fin colours.
A smaller six-facet dark pupil sits just above it, with the restrained highlight raised once more
to avoid coplanar flicker. The three fitted layers keep the original eye footprint and follow the
cheek rather than projecting a larger sphere from it.

The pupil layer adds 36 vertices / 12 triangles per near fish. Near/far inventory is now
1,422/12 vertices; the conservative all-48-near allocation is 68,256 vertices or 1,092,096 bytes.
Population, eight schools, four families, body/fin geometry, animation, habitat, LOD and one dynamic
draw remain unchanged. Native baseline `20261016T074018.769503Z-fish` and accepted current-binary
evidence `20261016T084028.237645Z-fish` were inspected at original resolution. The accepted frame
passes with 117 colour bins, 19.83 luminance deviation, 131.96 spatial contrast and an empty kernel
delta. Focused Chrome/SwiftShader evidence `20261016T084213.266338Z-chrome-fish` contains two
inspected distinct moving poses, 60 finite positive intervals, GL error zero, a live context and an
empty kernel delta. Native and Web gates require the layered-eye diagnostic. Physical Android
appearance and hardware performance remain unqualified.

## Restrained flank sheen and family gills (2026-10-16)

The next current-binary wildlife audit found that the close fish silhouettes, heads and four
family patterns were sound, but broad upper flanks still read as one flat cyan patch under the
shared water tint. The existing body vertices now carry a broad, low-contrast shoulder sheen.
It fades continuously over the back, belly, snout and caudal peduncle and varies gently along the
body, avoiding a checkerboard or painted stripe. Gill creases now derive from each fish's
underwater-adjusted back/body palette instead of using one identical dark green on every family.

This is solely vertex pigment on the existing mesh: the exact 1,386/12 near/far inventory,
48 agents, four families, animation, habitat, LOD, buffer size and one dynamic draw are unchanged.
Inspected same-camera native baseline `20261015T201745.737686Z-fish` and accepted evidence
`20261016T010828.510003Z-fish` show a softer readable shoulder transition without neon bands. The
accepted 1280x720 frame passes with 119 colour bins, 19.64 luminance deviation, 131.95 spatial
contrast and an empty kernel delta. Focused Chrome/SwiftShader evidence
`20261016T010941.369459Z-chrome-fish` passes two inspected moving frames, 60 finite positive
intervals, GL error zero, a live context and an empty kernel delta. Native and Web guards require
the surface-pigment diagnostic so a stale executable cannot qualify.

## Blunt convex head profile (2026-10-15)

A current whole-animal contact sheet found that the fish still ended in a pointed wedge despite
their eleven-station, sixteen-sided close body. The first ring was only 10 by 14 mm and expanded
abruptly to 150 by 138 mm over the short forehead. The first four existing stations now widen
progressively from a 30-by-27-mm blunt snout through a convex cheek into the unchanged shoulder.
The fitted oval eyes, gill marks and lower mouth cue remain on that continuous surface.

No geometry was added: the near/far inventory remains exactly 1,386/12 vertices, even if all 48
agents enter the close tier. Schools, one dynamic draw, four shape/pigment families, progressive
body wave, habitat and LOD are unchanged. Inspected native baseline
`20261015T152714.068234Z-fish` and accepted current-binary evidence
`20261015T201745.737686Z-fish` show the triangular faces replaced by rounded cheek and snout
profiles. The final 1280x720 frame passes with 116 colour bins, 19.89 luminance deviation, 131.96
spatial contrast and an empty kernel delta. Native and Web gates pin the explicit blunt-head
diagnostic.
Focused Chrome/SwiftShader evidence `20261015T201936.041330Z-chrome-fish` passes 60 finite
intervals, two distinct inspected moving frames, GL error zero, a live context and an empty final
kernel delta. Android packaging/runtime remains grouped with final integration.

## Oval eyes and tapered fin profile (2026-10-15)

The current close school audit found that the body loft, gills, mouth, family pigment and movement
were readable, but the broad dark-edged tail lobes still resembled crossed rectangular plates and
the old two-triangle eye remained a conspicuous diamond. Coordinate-only fin changes now give both
caudal lobes a narrower leading edge, farther swept pointed tip, deeper central notch and shorter
trailing edge. The six-point dorsal is lower and bends farther rearward instead of peaking as a
rigid triangle. No fin vertex or polygon is added.

Each near eye now uses a fitted six-facet oval plus the retained small highlight. This replaces two
dark eye triangles per side and adds only 24 vertices per near fish. The near/far inventory becomes
1,386/12 vertices; even the conservative all-48-near allocation is 66,528 vertices or 1,064,448
bytes. School count, patterns, palettes, habitat, LOD, progressive rear-body bend and one dynamic
draw are unchanged.

Inspected native baseline `20261015T152048.597141Z-fish` and accepted evidence
`20261015T152714.068234Z-fish` show a thinner fork rather than crossed slabs and small oval eyes
rather than black diamonds. The accepted 1280x720 frame passes with 119 colour bins, 20.22
luminance deviation, 110.65 spatial contrast and an empty kernel delta.

## Swept caudal silhouette (2026-10-14)

The renewed school audit found that otherwise readable bodies ended in tall, short caudal lobes
that overlapped into a cross-shaped silhouette. The last three body stations and rear cap now
extend the narrow peduncle by 60 mm. Both closed tail lobes are lower and sweep farther aft around
a deeper notch, producing a shallow fork rather than a plus sign. The 12-vertex distant silhouette
receives matching peduncle, lobe-height and aft-sweep corrections.

This coordinate-only pass preserves the exact 1,362/12-vertex budgets, closed-volume fin topology,
48-agent schools, animation, habitat, patterns, palettes and one draw. Native baseline
`20261014T172131.861383Z-fish` and accepted `20261014T172946.135210Z-fish` were inspected at
original resolution. The accepted school retains all four body families while each tail reads as
a thinner rearward fork.
Focused Chrome/SwiftShader evidence `20261014T173744.712303Z-chrome-fish` passes with two
inspected different live poses, 60 finite positive intervals, GL error zero, an intact context
and no new kernel GPU fault. The default arm64 APK assembles and application lint passes through
the two-worker wrapper without starting an emulator; Android runtime appearance remains unqualified.

## Family-specific flank pigment (2026-10-14)

A fresh fish/dolphin/goat/bear matrix confirmed that the revised fish geometry is active,
but shared underwater attenuation still compressed the four fish families toward nearly one
cyan body colour. The existing eleven longitudinal body stations now carry softly interpolated
family markings: a restrained lateral line, two broad flank bars, a lighter flank with a dark
caudal mark, or three narrow bars. The masks fade over the back and belly and retain radial
daylight, dorsal/flank/belly countershading and moving underwater light.

This is vertex pigment on the existing body surface: no image texture, vertex, buffer capacity,
draw, population, LOD or simulation state is added. The first study was visually rejected because
ordinary water tint erased it; only the bounded blend weights were then raised. Baseline/final
native evidence is `20261014T140659.730960Z-fish` and
`20261014T141738.403396Z-fish`. The final remains deliberately subtle, but the nearest family
silhouettes no longer share exactly the same uninterrupted flank. The capture and focused Web
guards require the new pigment diagnostic so stale executables cannot qualify. Focused
Chrome/SwiftShader evidence `20261014T142042.272849Z-chrome-fish` passes 60 finite intervals;
both different-time frames were inspected, the context remained live and the kernel delta is
clean. WebGL builds; Android ARM64 assemble/lint passes without an emulator/KVM. All 61 enabled
non-display CTest cases and 112 cross-platform harness unit tests pass.

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

The renderer checks every generated close fish against the compile-time near-vertex budget, and the
native/Web gates pin both that inventory and the current anatomy marker. Actual Android execution,
sustained performance, scale/colour variety, fin articulation and further texture/material polish
remain.

## Closed-fin pass (2026-10-13)

The current visual audit retained the rounded body, eye/gill cues and school readability, but the
forked tail, dorsal and paired pectorals were still coincident sheets. Both triangular tail lobes
are now closed triangular prisms; the dorsal and both pectorals are thin closed four-sided forms.
They preserve their authored outlines and palette colours while adding separately lit faces and
darker perimeter edges with only 4--6 mm local half-thickness. Simulation, progressive body bend,
schools, 72 m near threshold, far silhouette and one-draw batching are unchanged.

The five replacements add 132 vertices, bringing the near tier to 546 vertices / 182 triangles.
The conservative 48-near capacity is 26,208 vertices / 419,328 bytes, about 101 KiB above the
previous capacity; ordinary culling/LOD submits fewer. Inspected llvmpipe evidence
`build-consumer/software-captures/20261013T114848.142975Z-fish` passes with 119 colour bins, 20.76
luminance deviation, 122.64 spatial contrast and an empty kernel delta. Focused
Chrome/SwiftShader evidence `20261013T115244.372229Z-chrome-fish` also passes: both inspected
frames retain the school in distinct poses, their SHA-256 identities differ, all 60 intervals are
finite, `glError` is zero, the context remains live and the kernel delta is empty. Android
execution, sustained hardware performance, articulated fin strokes and texture/material polish
remain open.

## Smooth capped body and restrained fins (2026-10-14)

The next whole-island comparison found the fish visibly weaker than the revised dolphin, shark,
turtle and crab: the seven body rings still formed a pointed football and the large triangular
tail/pectoral surfaces dominated the school. The near body now uses eleven longitudinal stations
and sixteen radial sides, with small separately triangulated caps sealing the nose and caudal
peduncle. Analytic radial-normal daylight and interpolated dorsal/flank/belly colours replace the
old constant colour and brightness per triangle. Progressive rear-body bending is retained.

Both tail lobes now use five-point swept closed outlines; the dorsal and paired pectorals use six
points. A first render with 25cm tail lobes and 38cm pectorals was rejected as oversized. The
accepted 18cm/22cm profiles keep the fork and steering surfaces readable without producing a
cross-shaped silhouette, and the yellow/dark fin palettes are less saturated. Eyes, gills, mouth,
four palette/shape variants, 48-agent simulation, 12-vertex far tier and one dynamic draw remain.

Near geometry is 1,362 vertices / 454 triangles, at most 65,376 vertices or 1,046,016 bytes if
all 48 animals are near. Inspected native llvmpipe evidence
`20261014T024923.271904Z-fish` passes at 1280x720 with 114 colour bins, 20.10 luminance deviation,
132.11 spatial contrast and an empty kernel delta. Chrome/SwiftShader evidence
`20261014T025156.580245Z-chrome-fish` passes with two inspected different-time school views, 60
finite intervals, `glError=0`, an intact context and no new kernel GPU fault.
