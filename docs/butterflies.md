# Clearing butterflies

## Near anatomy and close audit — 2026-10-14

The 18 existing deterministic animals retain their three clearing habitats, rest/flutter/nectar
states, real scale, hinged wing animation, distance tiers and one dynamic draw. The earlier QA view
looked at an entire habitat from almost four metres away. It now follows animal 2 from less than a
metre and keeps the capture-only activation observer overhead, so the camera does not continually
invoke the normal close-player retreat. This does not affect interactive gameplay.

The old near mesh used a flat body diamond and three large triangles per wing. The current bounded
mesh uses a closed six-sided tapered body, a separate faceted head, paired antennae, five outline
stations for rounded fore/hind-wing lobes, dark outer margins and the existing central accent.
Near geometry is 228 vertices per animal instead of 72; far geometry, the 18-agent cap, culling and
one-draw batching are unchanged.

Native llvmpipe baseline `build-consumer/software-captures/20261014T130501.521721Z-butterflies`,
camera-only `20261014T133056.877305Z-butterflies` and final
`20261014T133448.341718Z-butterflies` all pass the ordinary image validator and were inspected at
full resolution. The final high-fold frame clearly shows the dark rounded wing margins and thicker
body. The paired antennae are close to edge-on in that frame, so a future multi-stage wing/heading
sequence should establish their live readability rather than inferring it from geometry alone.
