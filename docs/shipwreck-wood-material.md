# Shipwreck weathered-wood material

## Broken stern deckhouse — 2026-10-13

The last accepted broadside still left the surviving stern deckhouse as one horizontal roof slab
over two solid rectangular walls. This pass replaces that box cue with ten separated longitudinal
roof-board lanes. Their ends, elevation and length vary within a small weathered range, and one
lane consists of two shorter broken planks with a real gap. Both side walls now use lower/upper
plank bands plus four structural uprights to form three window bays. The middle bay is genuinely
open through the deckhouse; the two surviving outer panes are dark recessed surfaces rather than
black marks painted on a solid wall.

The existing texture, local wood coordinates, collision, placement, 12 debris pieces, buffer and
single draw remain unchanged. Replacing the old boxes costs 528 triangles, raising the whole wreck
from 5,004 to 5,532 triangles. The close beach camera sees an irregular open stern silhouette
instead of a compact cabin block; it remains deliberately ruined rather than a restored cabin.

Final inspected native llvmpipe `20261013T204318.064303Z-shipwreck` passes at 1280x720 with 193
colour bins, 38.55 luminance deviation, 161.29 spatial contrast and an empty kernel delta. The
first image of the same geometry (`20261013T203937.288568Z-shipwreck`) was also inspected, but
predated the tightened deckhouse-marker gate. Focused Chrome/SwiftShader
`20261013T204536.386851Z-chrome-shipwreck` passes 60 finite intervals, two inspected frames,
`glError=0`, an intact context and empty final kernel delta. Release and Debug core/asset tests,
33 browser-runner tests and Android ARM64 assemble/lint pass without an emulator or KVM.
End-grain refinement, debris burial/water contact, a matched wide comparison and Android-device
rendering remain open; this does not complete the umbrella realism item.

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

## Hull silhouette follow-up

The hull now uses an asymmetric rising sheer instead of a flat top and bottom profile: the bow
rises by up to 0.64m, the transom by up to 0.34m, and the same longitudinal function bends the
deck, gunwales, keel, fittings, openings, masts and cabin supports together. The two exposed
break sections receive deterministic per-band longitudinal offsets, yielding uneven broken plank
ends without another material, buffer or draw. The weathered cut colour and rim-beam radius were
reduced after the first screenshot made the exposed cross-sections look like bright starbursts.
The bow and deck line now read less like a pair of rectangular boxes. A subsequent broadside pass
replaces two former black surface openings plus two additional intact panels on the beach-facing
side with true missing-plank bays. Unequal retained plank lengths give adjacent strakes offset
break edges, while three dark inner ribs stand behind the openings. The opposite shell remains
closed, so the holes reveal structural depth rather than an ocean-coloured backface.

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

The current inspected native geometry result is
`build-consumer/software-captures/20261013T040134.503348Z-shipwreck`: 1280 by 720,
187 colour bins, luminance deviation 38.81, spatial contrast 162.24, 4,628 triangles, one wreck
draw, twelve coastal contact-shadow patches and no kernel fault. The complete focused coast suite
`build/browser-software/20261013T040508.125953Z-chrome-coast` passed shipwreck, water, west and
reef scenes with 120 finite intervals each under ANGLE SwiftShader, GL error zero, intact contexts
and no new kernel GPU fault. Both time-separated Web shipwreck images were inspected.

This material and silhouette slice does not complete the broader realism item. Finer broadside
plank thickness, transom detail, debris burial/water intersections, a dedicated matched wide wreck view,
and Android GLES 3 visual inspection remain required.

The current inspected broadside-damage result is
`build-consumer/software-captures/20261013T071626.841306Z-shipwreck`: 1280 by 720,
195 colour bins, luminance deviation 38.67, spatial contrast 161.87, one wreck draw, 4,972
triangles and no new kernel fault. The retained holes and ribs are visible from the deterministic
beach camera; collision, debris placement and render batching are unchanged.

The refreshed focused coast run
`build/browser-software/20261013T072820.354747Z-chrome-coast` passes shipwreck, water, west and
reef scenes with 120 finite intervals per view under ANGLE SwiftShader, GL error zero, intact
contexts and an empty new-kernel-fault delta. Both shipwreck frames and the first frame from every
companion scene were inspected; the broadside openings retain depth and do not expose a water or
shared-state artifact.

## Raked bow and stern profiles — 2026-10-13

The lower bow and stern no longer terminate as nearly vertical rectangular walls. A bounded
longitudinal rake begins only inside the outer 1.45m of the hull and increases below the 1.33m deck
edge: the bow pulls inward by at most 0.90m and the stern by at most 0.65m. Hull shell stations,
deck ends, plank seams and cap centroids use the same profile, while the sternpost now follows the
aft rake from keel to deck. The authored 20m top endpoints, waterline placement, collision proxies,
materials, debris, 4,972-triangle budget and one-draw batch are unchanged.

The unchanged baseline is `20261013T182707.864823Z-shipwreck` (198 colour bins, luminance deviation
38.66, spatial contrast 161.47). A first 0.46/0.34m version
`20261013T183023.809069Z-shipwreck` was visually inspected and rejected as too subtle. The accepted
native llvmpipe capture `20261013T183155.768012Z-shipwreck` has 194 colour bins, luminance deviation
38.50, spatial contrast 161.09 and an empty kernel delta; its bow visibly rises into the deck rather
than reading as a box end. The broad Chrome coast run
`20261013T183440.535268Z-chrome-coast` is deliberately not claimed as a suite pass: shipwreck,
water and west each passed 120 intervals, but the overall run timed out after the first reef frame.
A new narrow shipwreck scope then passed as
`20261013T185915.467977Z-chrome-shipwreck`: 60 finite SwiftShader intervals, two distinct inspected
time-separated frames, GL error zero, live context and no new kernel fault. Linux Release and Debug
core/asset tests, the 32 browser-harness tests, and Android ARM64 assemble/lint pass. No Android
emulator, KVM or hardware GPU was used.

## Missing-plank thickness — 2026-10-13

Each of the four genuine beach-facing missing-plank bays now closes all four visible cut boundaries
with an 11cm inward edge. Weathered fresh-break colour distinguishes the two transverse fracture
faces; darker inner wood closes the upper and lower retained-board edges. These sixteen quads add
32 triangles inside the existing buffer and draw, taking the landmark from 4,972 to 5,004 triangles
without changing collision, debris or layout.

Inspected native llvmpipe evidence `20261013T190835.072388Z-shipwreck` passes at 1280 by 720 with
214 colour bins, luminance deviation 38.30, spatial contrast 160.11 and an empty kernel delta. The
focused Chrome/SwiftShader result `20261013T191119.148542Z-chrome-shipwreck` passes 60 finite
intervals, two distinct inspected time-separated images, GL error zero, a live context and no new
kernel fault. Release and Debug asset validation, all 32 browser-runner tests, and Android ARM64
assemble/lint pass; the emulator, KVM and hardware GPU were not used.

The self-contained Linux archive built after the texture and fitting slices contains 14,752,032
installed bytes and 7,880,492 compressed bytes, with SHA-256
`0ce8ba429a69200952cbeceed15a3e295c73a730fb85ffa0410924cb15d6e0bf`. A temporary extraction
launched from outside the repository through the private pixman/llvmpipe harness and reproduced
the strict wreck scene without a material fallback or kernel event. That package frame passed at
1280 by 720 with 185 colour bins, 38.37 luminance deviation and 163.78 spatial contrast.

## Settled and split beach debris — 2026-10-15

A renewed wide-beach audit found the ship itself visually coherent but the detached debris too
cleanly laid out. Planks used a centre 80 mm above a layout point that already carried a 55 mm
support guard, leaving their 65 mm half-thickness visibly above sand. Non-spar debris now consumes
that guard and settles a further 5 mm into the sampled support. A deterministic cross-slope and
longitudinal tilt plus a five-section, sub-thickness warp leaves each middle slightly uneven and
tucks both splintered ends into sand.

Five plank placements at least three metres long are rendered as two unequal storm-broken
fragments. The pair retains the authored centre, overall direction and beachward trail but has a
real central gap, different fragment lengths and widths, and a small lateral slip. Four shorter
planks remain whole, as do the spar and both three-board hull-fragment clusters. The layout still
reports twelve bounded debris placements and the non-solid traversal policy is unchanged.

The five additional broken-plank meshes add 180 triangles, raising the current single-batch wreck
from 5532 to 5712 triangles without another texture, draw or collision proxy. `IslandCoreTests`
passes. The inspected baseline is `20261015T173543.349535Z-shipwreck`; the accepted current-binary
capture `20261015T174534.505199Z-shipwreck` passes at 1280x720 with 238 colour bins, 39.62
luminance deviation, 167.75 spatial contrast and an empty kernel delta. The foreground now shows
actual gaps and buried irregular ends instead of long floating construction timber. Physical
Android/AMD appearance and subjective free-walk collision feel remain open.
