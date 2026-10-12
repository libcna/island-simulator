# Beach-crab form pass

## Scope

This 2026-10-12 pass addresses the explicit request to improve the visual form of the
island's crabs. It preserves the existing thirty agents, six debris-associated beach
colonies, behaviour, habitat constraints, culling, LOD distance and single dynamic draw.
No downloaded model or texture is used.

## Geometry change

The former near model used a pointed fan for the shell and one double-sided triangle
for each complete leg. In a close capture this read as a flat polygon with hairline
limbs and arrow-shaped claws. The replacement uses:

- four elliptical height rings and ten sectors for a rounded, broad carapace;
- restrained alternating upper-shell panels and distinct darker side plates;
- two crossed, tapered surfaces for both segments of all eight walking legs;
- a thickened arm and octahedral palm for each claw, followed by two separated fingers;
- short volumetric eye stalks with small dark eyes;
- paired mouthparts and two subtle lateral carapace teeth.

The exact near budget is 594 vertices / 198 triangles and the far silhouette remains
12 vertices / 4 triangles. The renderer checks both counts while assembling every visible
animal. Worst-case dynamic capacity is 17,820 vertices (285,120 bytes at 16 bytes per
`VertexPositionColor`), although ordinary culling and the 38 m near threshold make that
case deliberately conservative. All visible near and far animals still share one upload
and one draw call.

The `crabs` audit camera is now 1.75 m seaward of its subject and 0.62 m above the sampled
beach. This deliberately allows the normal non-hostile retreat behaviour rather than
freezing an artificial display pose, while making palm-sized anatomy inspectable.

## Native evidence

The pre-change ordinary view is retained at
`build-consumer/software-captures/20261012T211908.604459Z-crabs`; it shows the five-crab
colony mostly as small flat reddish marks. The closer diagnostic baseline
`20261012T212204.895592Z-crabs` exposes the pointed shells, single-surface legs and spike-like
eyes.

Final isolated pixman/llvmpipe evidence is
`build-consumer/software-captures/20261012T212449.173818Z-crabs`. The 1280 by 720 gate passes
with 166 colour bins, 28.25 luminance deviation and 175.93 spatial contrast. The inspected
frame shows five near crabs in one draw with rounded shells, jointed legs, palms, split
claws and shortened eye stalks. `kernel-live.jsonl` and `kernel-after.json` contain no new
kernel messages. No AMD GPU was used.

## Remaining limits

The animals remain deliberately low-poly and vertex-coloured. They do not yet have a
filtered shell microtexture, self-shadowing, burrows or individually curled distal leg
segments. WebGL 2, Android GLES 3, physical-device performance and interactive motion review
remain open, so the broader aquatic-wildlife polish item remains in progress.
