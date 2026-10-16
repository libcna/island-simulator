# Clearing butterflies

## Rounded abdomen, legs and restrained wing markings — 2026-10-16

The next close wildlife audit found that the existing near butterfly still read as a flat yellow
and brown arrow from above. Its abdomen now continues through a fourth six-sided ring to a short
rounded cap instead of ending in a long triangular point. Alternating restrained pigment separates
the rear segments, and three swept pairs of lightweight planar legs provide an insect silhouette
without adding a draw. The forewing gained an extra shoulder station for a rounder leading edge;
the old root-to-centre triangular accent was replaced by a small local wing patch.

Near geometry rises from 264 to exactly 384 vertices per animal. Even in the conservative case
where all eighteen agents use the near tier, this adds only 2,160 vertices (34,560 bytes) to the
existing shared dynamic buffer. Population, simulation, habitat, far mesh, culling, live wing fold
and the single shared draw are unchanged. The accepted native frame
`20261016T210847.695170Z-butterflies` passes at 230 colour bins, 31.69 luminance deviation and
194.16 spatial contrast with an empty kernel delta. Focused Chrome/SwiftShader evidence
`20261016T210956.200199Z-chrome-butterflies` passes a live 60-interval view with GL error zero and
no context loss. Android arm64 assembly and lint pass without running an emulator; the debug APK is
24,746,275 bytes (SHA-256
`181d6c1e68c88030de23f75d86480252c13bf03e44d3ac1e79ad40cf61e9b977`) and its packaged
`libmain.so` is 19,703,704 bytes. Physical Android appearance and performance remain unqualified.

## Segmented tapered body and live close audit — 2026-10-16

The renewed small-animal audit retained the rounded patterned wings, but the near butterfly still
collapsed into two coloured planes around one dark rod. The close body now has three tapered
six-sided rings instead of two, creating a fuller thorax and narrowing rear abdomen with a
restrained palette-linked pigment break. The faceted head, paired antennae, rounded fore/hind wing
lobes, dark margins, central accent and live wing fold are unchanged.

Near geometry rises from 228 to 264 vertices per animal. Even if all eighteen animals enter the
near tier, the change adds only 648 vertices / 10,368 bytes to the already shared small-fauna
dynamic buffer; the population, three habitats, far tier, behavior and one draw remain unchanged.
The renderer now asserts the exact per-animal near budget.

The previous fixed camera was actually about five metres from the flower patch and could not prove
body anatomy. It now follows animal two from roughly 0.7 m while supplying an observer eighteen
metres overhead to the behavior system. The subject therefore remains live and changes position and
wing fold without the audit camera repeatedly triggering retreat. A first closer patch view
`20261016T091433.107331Z-butterflies` was retained only as rejected composition evidence because it
still framed the group rather than the subject. Inspected accepted native evidence
`20261016T092020.815244Z-butterflies` passes at 219 colour bins, 31.39 luminance deviation and
193.69 spatial contrast with kernel `[]`. Focused Chrome/SwiftShader
`20261016T092424.885223Z-chrome-butterflies` passes two inspected distinct live wing poses, 60
finite positive intervals, GL error zero, a live context and an empty kernel delta. Native and Web
gates require both the segmented-body and live-close-camera diagnostics.

## Near anatomy and close audit — 2026-10-14

The 18 existing deterministic animals retain their three clearing habitats, rest/flutter/nectar
states, real scale, hinged wing animation, distance tiers and one dynamic draw. The earlier QA view
looked at an entire habitat from almost four metres away. It now follows animal 2 from less than a
metre and keeps the capture-only activation observer overhead, so the camera does not continually
invoke the normal close-player retreat. This does not affect interactive gameplay.

The old near mesh used a flat body diamond and three large triangles per wing. The current bounded
mesh uses a closed six-sided tapered body, a separate faceted head, paired antennae, five outline
stations for rounded fore/hind-wing lobes, dark outer margins and the existing central accent.
That pass raised near geometry from 72 to 228 vertices per animal; far geometry, the 18-agent cap,
culling and one-draw batching were unchanged.

Native llvmpipe baseline `build-consumer/software-captures/20261014T130501.521721Z-butterflies`,
camera-only `20261014T133056.877305Z-butterflies` and final
`20261014T133448.341718Z-butterflies` all pass the ordinary image validator and were inspected at
full resolution. The final high-fold frame clearly shows the dark rounded wing margins and thicker
body. The paired antennae are close to edge-on in that frame, so a future multi-stage wing/heading
sequence should establish their live readability rather than inferring it from geometry alone.
