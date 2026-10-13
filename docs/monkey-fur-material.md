# Monkey fur material

## Fitted nose and readable corneas — 2026-10-13

The latest fur-framed head still presented one nearly uniform pale mask at ordinary
island distance. This follow-up moves each cornea 6mm forward, enlarges it slightly and
adds a shallow 76mm-wide fitted nose pad to the short muzzle. The pad is skinned to the
existing head bone. A reserved negative texture-U tag darkens only this geometry in the
existing face shader and draw, while the pre-existing nostrils and mouth remain separate
dark detail. Cost is 78 vertices and 120 triangles per near face; there is no new texture,
bone, material, draw, animal or simulation work.

The first current-binary native capture `20261013T194439.335771Z-monkeys` passes its
technical gates at 1280x720 (190 colour bins, 34.81 luminance deviation, 147.09 spatial
contrast and an empty kernel delta), but its rear/occluded view was rejected as facial
evidence. The complete nine-stage llvmpipe sequence
`20261013T194613.603137Z-monkey-tree` passes at 204--211 bins, 44.21--44.94 deviation and
192.17--196.26 contrast. All nine frames were inspected together; the clear side pose
shows the attached dark nose, raised cornea and mouth, while tree reach, cheek ruffs and
the compact fur-broken hooked tail remain connected. Its kernel delta is empty.

Focused Chrome/SwiftShader `20261013T195104.174891Z-chrome-monkeys` passes 60 finite
intervals with `glError=0`, an intact context and empty final kernel delta. The initial,
four intermediate and final images were inspected as a montage: front/side frames retain
the new face through the live turn, while two deliberately sampled poses are partly
occluded by foliage. Release and Debug monkey tests pass 3/3, both asset gates pass, the
33 browser-runner tests pass, and Android ARM64 assemble/lint succeeds without an emulator.
The result improves first-person facial readability; procedural joint topology, continuous
gait naturalness, facial micro-anatomy and Android-device rendering remain open, so this
does not claim photorealism or complete the wider monkey task.

## Fur-framed face and broken tail outline — 2026-10-13

A fresh current-binary native side capture `20261013T141856.054706Z-monkeys` was
inspected before this pass. The shorter/deeper body already read as a quadrupedal
primate, but the light facial surface still ended in a hard oval and the uniformly
smooth tail surface retained a rodent-like tube silhouette. This pass addresses those
two visible defects without changing gait, behavior, bones, materials or draw count.

The near head now carries two overlapping fur-material cheek ruffs on the existing head
bone. They frame the skin mask with a broader dark-fur silhouette instead of changing
the mask colour. The tail root grows from 90 to 105 mm, stays above 56 mm through 80
percent and above 46 mm through 90 percent, then closes rapidly. Thirty-six raised
three-sided fur clumps replace the previous twenty-four low clumps. Each clump shares
the underlying section's two tail-bone weights, and its varying 32--38 mm lift breaks
the outline rather than painting fur onto a smooth cylinder. Near/far geometry is now
2,315/797 vertices and 3,448/1,208 triangles; all added geometry is near-LOD-only and
uses existing buffers, fur atlas, shader, 14-bone palette and three draws per near
animal.

The resulting native side image `20261013T142114.732847Z-monkeys` passes at 1280x720
with 173 colour bins, 29.11 luminance deviation and 113.26 spatial contrast. Direct
before/after inspection shows the mask recessed between furred cheeks and a visibly
irregular, heavier hooked tail. The complete nine-image tree-contact sequence
`20261013T142214.661677Z-monkey-tree` also passes and was inspected as a montage: the
ruffs remain attached to the turning head, tail clumps follow the animated hook, and
both hands retain their bark reach. Both native kernel deltas are empty.

Current Chrome/SwiftShader evidence `20261013T142410.234670Z-chrome-monkeys` passes all
60 finite intervals with `glError=0`, an intact context and empty final kernel delta.
Its initial, four intermediate and final frames were inspected together; the fur-framed
face and attached tail survive the scripted live turn. Android ARM64 assemble/lint also
passes, without launching an emulator. The model remains procedural: shoulder/hip
topology, continuous gait naturalism, facial micro-anatomy and Android device rendering
remain open, so this is not a final photorealism acceptance.

## Compact primate proportion follow-up — 2026-10-13

A renewed current-binary ordinary capture `20261013T090043.364406Z-monkeys` confirmed that the
previously thickened tail did not solve the user's complaint by itself. The animal still read as
a long low rodent: nearly level capsule torso, small dark facial mask, thin distal limbs and a
large smooth hook. This pass changes the connected anatomy and preserves the existing seven-agent
population, 14-bone GPU skinning, fur atlas, behavior and draw structure.

The 13-section trunk is shorter and deeper, with separately readable rump, pinched waist and
raised shoulder/chest masses. Limb sleeves keep the same fixed-length ground/bark rig but gain
broader upper-arm/thigh and distal sections; palms, soles and four digits are also fuller. The
twelve-row face is wider and more forward without becoming a pointed wedge, the cranium and ears
move slightly up/back, and lighter skin makes the existing corneas, two added nostrils and curved
mouth readable against the dark short-fur coat.

The three-bone tail centreline is about one quarter shorter and lower while retaining a forward-
turning terminal hook and its 90mm muscular root. Twenty-four low, longitudinal three-sided fur
clumps replace twelve sparse radial spikes. They use the underlying section's two-bone weights and
the existing fur material, so the outline stays attached throughout turns. Near/far body geometry
is 2,113/797 vertices and 3,172/1,208 triangles; the change adds no bones, textures or draws.

Inspected native llvmpipe `20261013T091557.189672Z-monkeys` passes at 1280x720 with 174 colour
bins, 29.11 luminance deviation and 113.30 spatial contrast. The side view now shows a compact
raised hook rather than the long rat-like cord, a deeper shoulder/rump silhouette, heavier limbs
and a lighter blunt face. The complete nine-state tree sequence
`20261013T091711.731086Z-monkey-tree` also passes and was inspected as a montage: both hands reach
the trunk, the animal returns to ground travel, and the shortened tail stays connected without a
joint kink. The focused Chrome/SwiftShader run
`20261013T092049.915559Z-chrome-monkeys` passes all 60 finite intervals with `glError=0`, an intact
WebGL context and an empty final kernel delta. Its initial, four intermediate and final frames were
inspected together and retain the broader face, deep trunk and attached compact tail throughout
the scripted turn. This is a substantial silhouette correction, not final photorealism;
continuous gait, fine shoulder/hip topology, Android and physical performance remain open.

## Muscular prehensile-tail volume — 2026-10-13

The preceding pass was directionally better but the 66 mm root and roughly 37 mm
four-fifths radius still read as a long rat-like cord beside the much larger torso in
the actual side image. The profile now starts at 90 mm, remains above 48 mm through
80 percent and above 39 mm through 90 percent of its centreline, then closes rapidly
without a blunt cap. The existing Hermite centreline, forward/upward terminal hook and
three animated tail bones are unchanged.

The near model now distributes twelve rather than eight raised fur clumps from the root
through 83.6 percent of the tail. Their height is 22–27 mm and every root retains the
same two-bone blend as the underlying skin, preventing gaps during motion. This adds only
16 vertices / 12 triangles to the preceding near mesh: current near/far geometry is
2,065/797 vertices and 3,136/1,208 triangles with the same atlas, draw count, bones and
seven-agent population. Profile tests enforce monotonic taper, the new retained volume,
closed tip, C1 centre/tangent continuity and normalized blend weights.

Release and Debug skinning-shader, gait/profile and sole tests pass. Actual contained
llvmpipe `20261013T051947.895356Z-monkey-tree` passes all nine deterministic states
with an empty kernel delta; selected ground, reach and release poses were inspected and
keep the fuller hook connected. The ordinary `20261013T052129.036707Z-monkeys` capture
also passes, but a foreground trunk partly obscures the subject. An attempted opposite-
side camera produced worse trunk/leaf occlusion and was reverted rather than retained.

The first current Chrome/SwiftShader attempt
`20261013T053301.870364Z-chrome-monkeys` exceeded its software-capture time limit after
writing the initial and four intermediate images; it remains failed. An unchanged retry
with a larger outer limit, `20261013T054045.083047Z-chrome-monkeys`, passes 60 finite
intervals with `glError=0`, an intact context and empty final kernel delta. Start, four
intermediate and end images were all inspected: the larger tail remains connected and
voluminous through turns, although broad leaves obscure two intermediate poses. Limb/
torso joints, continuous natural gait and current Android rendering remain open, so this
is another bounded visual improvement rather than final realism acceptance.

## Short primate face and fur-broken hooked tail — 2026-10-13

The renewed current-binary audit used the complete deterministic tree excursion
`20261013T014015.873783Z-monkey-tree`. Its inspected side poses still read as a
dog/rodent hybrid: a small cranium behind a long narrow facial wedge and a uniformly
smooth hose-like tail. The existing fur material, quadrupedal gait and tree contact
were retained; this pass changes those two dominant silhouettes rather than another
colour or locomotion subsystem.

The head now uses a 183×202×168mm cranium (up from 165×185×150mm), moved slightly
back/up, with larger protruding outer and inner ears. The connected twelve-row facial
surface is broader through brow, cheeks and muzzle, but its maximum authored front is
0.648m instead of0.678m. Eyes, eyelids, nostrils and curved mouth follow the shortened
surface. This aims at a generic capuchin-like short primate profile rather than claiming
an exact species model.

The continuous tail now retains a66mm furry base and more than35mm radius through
80 percent of its centreline, then closes progressively. Its terminal Hermite point
turns another8cm forward. Eight small three-sided fur clumps use the existing texture
and identical two-bone blend weights at their roots; they add32 near vertices and24
triangles without a texture, material, draw call, bone or permanent allocation. Current
near/far geometry is2049/797 vertices and3124/1208 triangles.

Release and Debug gait/profile, skinning-shader and sole tests pass. The profile
regression covers broad face/head/ear bounds, orbital intersection, bilateral eyelids,
monotonic tail taper, retained mid-tail volume, closed tip and C1 centre/tangent/weight
continuity. Native llvmpipe `20261013T015052.930547Z-monkey-tree` passes all nine
captured states; selected exact-phase before/after pairs against the baseline were
inspected and the kernel delta is empty. The head is visibly fuller and shorter, while
the tail is thicker and ends in a stronger forward hook. The fur clumps remain subtle
rather than producing a spiky cartoon outline.

Release WebAssembly builds. Actual Chrome/SwiftShader
`20261013T015419.973741Z-chrome-monkeys` passes 60 intervals with `glError=0`, intact
context and empty final kernel delta. Start, four intermediate and end images were all
inspected: the revised head/tail stay connected through sampled movement and turns,
although broad leaves hide much of two intermediate poses. Procedural limb/torso joins,
front-face simplicity and continuous biological gait quality remain visibly open. No
Android runtime qualification or final realism acceptance is claimed.

## Renewed rat-like silhouette complaint — 2026-10-12

Actual contained native baseline `20261012T061354.741487Z-monkeys` was inspected
(`build/monkey-rat-review-before.png`): thin pointed tail, projecting narrow muzzle,
long torso and dark simplified face. This confirms visible defects despite existing
fur/rig work; previous implementation is not appearance acceptance.

The tail now retains a fuller fur-bearing radius through most of its length and rounds
to a closed tip, instead of linearly tapering from36mm to3mm. Radius is
`(.032+.022*(1-t))*sqrt(max(0,1-t^12))`; near24rings/12sides, far12rings/8sides.
The terminal ring is closed with a single triangle fan rather than collapsed quads.
Existing three-bone centreline, blending, fur material and UVs remain. Cranial radii
increase from150/160mm to166/176mm; the muzzle is shortened/widened, nostrils and
mouth follow it, the abdomen is narrower and shoulders fuller. No gait/AI change.

Contained native after-view `20261012T061652.692874Z-monkeys` PASS322bins/std36.01/
spatial210.29, kernel-after[]; actual image inspected in
`build/monkey-rat-review-after.png`. Tail has more volume and face is less pointed.
Same camera, not an exact pose/wind/frame match. The tail still reads as a smooth
tube at this distance and limb junctions, facial detail and overall realism remain
open. No visual completion, full movement/climbing or portable qualification claim.

Safety deviation: the initial `ctest -R Monkey` mistakenly included a direct display
test and ran on AMD at08:16CEST. It terminated normally, but was NOT authorized by
the stop latch and is not accepted qualification. Read-only kernel query08:15–08:18
returned no entries. Subsequent checks use contained software/`linux-core` only.
CMake now disables direct display-labelled tests when the retained incident stop
file exists at configuration time; `ctest -N -R IslandGameMonkeyCapture` confirms
Disabled. This is not a runtime guard on direct executable launches. The latch itself
is unchanged. This guard is not a driver fix or permission to resume hardware tests.
Final incremental build and inclusive55 CPU groups PASS100.52s; logs
`build/monkey-silhouette-final-build.log` and `build/monkey-silhouette-full-tests.log`.

## Hooked tail, facial rims and smoother trunk — 2026-10-12

The retained `monkey-tree` sequence made the remaining tail defect clearer than the
single ordinary still: the thicker tube projected nearly straight behind the animal and
read as a dog-like rod. The three existing tail bones now follow an upward curve and the
terminal Hermite point turns forward, forming a continuous hook. The rounded fur-bearing
radius profile, 24/12 near rings/sides, skin weights and animated phase offsets are retained.
Tests require the terminal centre and tangent to turn forward/upward while preserving C1
continuity and normalized two-bone blending.

The body loft now has 13 longitudinal sections instead of nine, with additional bounded
hip, abdomen, shoulder and neck transitions. The cranium grew about 7–8%, and rounded
outer/inner ears are larger. Each dark cornea is now framed by separate skin-coloured upper
and lower eyelid strips (40 sampled points and 40 triangles per monkey face) whose profile
is finite, bilaterally mirrored, nonzero-width and slightly forward of the orbital surface.
This uses the existing face/eye draws, bone palette, fur texture and materials; no texture,
shader, animation state or draw call was added.

Actual contained llvmpipe `20261012T202913.511429Z-monkey-tree` passes all nine images;
selected stages were inspected together. They show the subject walking quadrupedally,
reaching one arm to the trunk and returning, while the hooked tail remains volumetric and
animated. Final metrics are 203 colour bins, luminance deviation 44.72 and spatial contrast
192.54. Ordinary close `20261012T202959.340747Z-monkeys` also passes and was inspected
(164 bins / 29.02 / 113.80). Both kernel deltas are empty. The new hook is a clear
silhouette improvement, but the procedural limbs/trunk and small face detail remain
visibly stylized; this is not final realism or portable qualification.

Release monkey profile/gait, skinning-shader and sole tests pass 3/3; Debug gait and both
native game builds pass. No AMD GPU, Android emulator/KVM or dependency setting was used.

## Shorter fur and non-collapsing side UVs (2026-10-11)

The first source and cosine UV projection produced large wood-like streaks on the actual
monkey. The derivative of `.5+.4*cos(angle)` vanishes on side-facing flank vertices: a
narrow strip of the image is spread over that surface. `MonkeyFurUv::Around` now uses an
angular mirrored interior window, with folds at top/bottom instead of the broad sides.
Head UVs also use angular coordinates rather than planar X/Y projection. Body, limbs,
head and tail retain skin-attached rest UVs; no repeat/seamless claim is required. CPU
tests cover side density, range, circumference closure and longitudinal endpoints.

New source `assets/art-studies/monkey-fur-albedo-v2.png` is the unchanged built-in ImageGen
output `01a129bd-2c19-7192-a43c-adcfe79c0fb9/exec-64a1789b-5a13-41e9-bf2d-a0d42e3bef38.png`.
Exact final prompt is `assets/art-studies/monkey-fur-albedo-v2-prompt.txt`. Generated without
reference inputs, actually 1254-square opaque RGB8 despite the 1024 prompt. SHA-256 and
provenance are registered in the asset manifest and ASSET_LICENSES.md. A first attempted
revision (`exec-f27ea7ba-8883-4926-80a7-7b13f5ef2764.png`) still had long flowing strands
and was rejected; it is not a runtime asset. Historical v1 remains unchanged. CMake and
Android Gradle now prepare v2 at the same 512-square runtime path; mip/texture memory,
shaders, palette, draw count and texture-loading diagnostics are unchanged.

Native evidence `20261011T141731.945186Z-monkey-tree` passes all eight stage gates; peak
image inspected against `20261011T141011.681705Z-monkey-tree`: coarse flank/head ridges
are replaced by finer, shorter fur detail. Ordinary moving-view capture
`20261011T141831.862271Z-monkeys` also passes/inspected. Both kernel deltas are empty.
This is a visible material improvement, not a realistic-anatomy acceptance: bulky limb
attachments, simplified facial anatomy, geometric silhouette and stiff tail remain.
Static frames do not prove full animation quality, LOD transition quality or climbing.

Native build and 37/37 non-display groups pass in 48.27 seconds
(`build/monkey-fur-v2-tests.log`). Asset gate: 24 procedural records, 70 generator sources,
10 project files, zero external assets. Web runtime qualification remains outstanding.

Android x86_64 build/lint passes (10 s). Isolated KVM/SwiftShader GLES evidence
`20261011T142013.159547Z-monkeys` passes on its first presentation attempt; the actual
1280x720 screenshot was inspected and shows the same finer coat. Kernel delta is empty.
Retained APK `build/android-software/island-monkey-fur-v2-x86_64.apk`, 21,158,731 bytes,
SHA-256 `fa80bf806c7540f9fafc71a010b38909d5730ab4e7f5ba92d09eeef7f0abde2b`.
This ordinary view does not qualify Android tree-contact poses, lifecycle or real hardware.
Default ARM64 build/lint restored successfully (9 s), runtime untested; APK 21,233,236
bytes, SHA-256 `d6078c5ef061eb3127e2c7193758f8ef835a230ba37a05bcc7740970c12cb0aa`.
Stop latch unchanged; all launched jobs finished, no host/emulator configuration edits.

Subsequent WebGL check of current fur plus limb collars passes:
`20261011T143701.925390Z-chrome-monkeys`, six images inspected, 60 intervals, GL error
zero, intact context, empty kernel delta. Prepared fur pixels match native by SHA-256.
See `docs/monkey-limb-skin.md` for scope and remaining visual defects.

## Historical v1 provenance

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
