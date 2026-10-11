# Monkey fur material — first integration

## Provenance

Built-in ImageGen (not CLI/API fallback), generated without reference images. Exact
prompt: `assets/art-studies/monkey-fur-albedo-v1-prompt.txt`. The original output
`01a129bd-2c19-7192-a43c-adcfe79c0fb9/exec-21714962-5c96-485b-9ad1-2835b81941a6.png`
was copied unchanged to `assets/art-studies/monkey-fur-albedo-v1.png`.
Actual size is 1254 by 1254, opaque RGB8, despite the requested 1024-square prompt.
Both source and prompt hashes are recorded in `assets/manifest.json`.

## Implementation

Existing bounded host area reduction prepares `Content/wildlife/monkey-fur.png`
at 512 square for native, Web preloading and Android assets. The original stays intact.
The shared opaque-image loader constructs ten mips (1,398,100 RGBA8 bytes).
Missing or invalid fur is a startup error, not an undiagnosed plain-colour fallback.

Rest-mesh UVs travel with skinning. Longitudinal body/limb/tail coordinates follow
each mesh part; transverse mapping uses a mirrored interior window, avoiding direct
repeat sampling of an image whose seamlessness has not been established. Head mapping
is projected in its rest frame. The texture modulates diffuse lighting relative to its
mean, bounded to 0.45–1.8; face and eyes explicitly disable fur modulation. Existing
canopy shade, specular and fog remain independent.

## Verification and remaining scope

Native incremental build passes. Contained software capture
`20261011T114648.821520Z-monkeys` passes the strict close-subject and fur-load gates;
the 1280x720 image was inspected. Fur variation is visible on the flank/head instead
of a constant brown fill; the face and limb junctions remain visibly stylized. Kernel
delta is empty. This static frame does not prove animation quality or absence of
stretching. Android and Web qualification of this changed model/material is pending.
Mirrored windows and per-part density still need art direction. This is not silhouette
fur, a completed monkey overhaul, terrain-aware planting, or tree climbing. Those and
the full island audit remain open.

Native non-display suite: 36/36 pass (69.53 s). After adding the fur source to the
actual-image CPU pixel-oracle comparison, material/shader recheck passes 2/2 (9.98 s).
Android software-launcher CPU tests: 21 pass; native launcher tests: 29 pass. Asset
manifest validation and Web runner syntax pass. None constitutes Android/Web runtime
qualification. Host GPU stop latch is unchanged; no dependency or emulator edits.
