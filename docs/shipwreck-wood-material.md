# Shipwreck weathered-wood material

## Source and provenance

The runtime material uses the original 1254 by 1254 PNG at
`assets/art-studies/shipwreck-weathered-wood-albedo-v1.png`. It was generated with the built-in
OpenAI ImageGen tool from the exact project-authored prompt preserved beside it in
`assets/art-studies/shipwreck-weathered-wood-albedo-v1-prompt.txt`; no reference image or external
model was supplied. The source SHA-256 is
`9fca0839d68b23408a42336564ddf0ed9e19b1eb4c0b319784a5a3b1df64596c` and the prompt SHA-256 is
`9aa36f4c838080c7a6885fe6eac3459664194896065da711f56e3006d17cd290`.

This is AI-generated project artwork, not a photograph or scan. The repository license is not yet
declared; no CC0 or exclusive-copyright claim is made. `ASSET_LICENSES.md` is authoritative.

## Preparation and sampling

The common host preparation tool performs deterministic area reduction to
`Content/structures/shipwreck-weathered-wood.png`, a 256-square opaque base. Runtime construction
adds the complete nine-level mip chain: 349,524 RGBA8 bytes. CMake installs and preloads the file
for native and Web builds; Android's staged-assets task prepares the same pixels.

The generated image was not accepted as seamless. Direct source-edge measurements produced
horizontal edge MAE 23.56 versus representative interior MAE 11.70 (ratio 2.013), and vertical
edge MAE 20.24 versus interior MAE 26.45 (ratio 0.765). The shader therefore folds coordinates
into the 7.5--92.5 percent interior window before clamped sampling. It makes no seamless-source
claim and never samples the failed source boundary.

Every emitted wreck vertex carries a material coordinate. Quad construction selects its longest
edge as the timber grain axis unless the deck explicitly selects its fore-aft edge; tapered beams
and independently rotated debris consequently follow their own length instead of inheriting the
main hull's orientation. Triangular end caps receive bounded local coordinates. The texture is
normalized by its measured one-pixel terminal-mip mean and contributes only a bounded modulation.
Fresh splinters strongly suppress it so their lighter procedural material remains readable.

The earlier procedural long/fine grain, weathering, split marks, wet and submerged darkening,
waterline salt, restrained sheen, fog and distance fade remain. When custom effects are unavailable,
the existing vertex-colour `BasicEffect` path remains the visual fallback.

## Evidence and remaining work

The untextured native llvmpipe reference is
`build-consumer/software-captures/20261012T222715.392938Z-shipwreck`. The final inspected
board-local UV and iron-fitting result is
`build-consumer/software-captures/20261012T224035.100025Z-shipwreck`: 1280 by 720,
183 colour bins, luminance deviation 38.36, spatial contrast 163.64, one wreck draw, 4,628
triangles, and no kernel event. Both use the deterministic close wreck camera.

Eight narrow hull straps, sixteen volumetric rivets, four cabin hinges and two rudder braces carry
a dedicated material tag. The fragment path excludes these faces from wood sampling, adds bounded
large-scale rust mottling and a slightly broader fixed-sun sheen, and retains waterline wetness.
All fittings stay in the existing static batch; this pass adds 680 triangles and no draw call.

This material slice does not complete the broader realism item. Hull damage and plank geometry,
debris burial/water intersections, matched wide views, plus
actual WebGL 2 and Android GLES 3 visual inspection remain required.

The self-contained Linux archive built after the texture and fitting slices contains 14,752,032
installed bytes and 7,880,492 compressed bytes, with SHA-256
`0ce8ba429a69200952cbeceed15a3e295c73a730fb85ffa0410924cb15d6e0bf`. A temporary extraction
launched from outside the repository through the private pixman/llvmpipe harness and reproduced
the strict wreck scene without a material fallback or kernel event. That package frame passed at
1280 by 720 with 185 colour bins, 38.37 luminance deviation and 163.78 spatial contrast.
