# Current wildlife visual audit — 2026-10-12

## Latest boar shoulder/bristle pass

Current baseline062020.501241Z-boars and revised062718.488953Z-boars were both
actually inspected as full1280x720 native software images. Latest gate PASS317bins,
luminance deviation43.88, spatial contrast195.58, kernel-after[]. Comparison files:
`build/boar-current-review.png` and `build/boar-silhouette-after.png`.

The rear/abdominal loft is narrower/lower while shoulder/head/leg anchors are retained.
The shoulder rises17cm above the rear anchor instead of13cm. Three staggered dorsal
bristle rows replace the single45-strand row (135triangles, +270near vertices per
animal, no new draw/material/texture). Each root independently seats6mm into the same
body profile; adult lengths vary34–67mm, juveniles20–53mm with rearward lean.
CPU tests cover rooting/bob/age variants, attached roots, raised tips/bounded envelope
and the shoulder/rump difference. Pose/herd/coat tests PASS4.59s.
Inclusive55 CPU groups PASS94.79s (`build/boar-silhouette-full-tests.log`).

The image shows a slightly leaner rear and a fine broken dorsal edge; this is a modest
improvement, NOT a completed realistic boar. The flank remains smooth/plain, muzzle
is long, legs remain angular and vegetation partly obscures feet. The body texture and
head/limb transitions need a more substantial next pass, followed by motion and
native/Web/Android inspection. No portable build/runtime or listening in this step.

## Earlier audit

Actual current native build after `5fc2a94`, contained llvmpipe/private pixman,
one renderer at a time. Both captures passed their technical image/runtime gates,
and both complete1280x720 PNGs were actually inspected. Kernel deltas are empty.
This is not anatomy, animation, audio or overall wildlife acceptance.

## Wild boars

Evidence: `build/software-captures/20261011T233059.789662Z-boars/capture.png`.
Technical capture PASS:295colour bins, luminance deviation56.24, spatialcontrast275.31.

The primary animal is partly hidden behind foreground shrubs; a large diagonal leaf
occupies the right foreground. Most legs and much of the torso are not available
for a meaningful anatomy/contact review. The visible head/back outline is still
coarsely angular, with a steep wedge-like face and tiny eye. The image does not show
enough of the coat to claim natural bristles, nor does a still prove gait quality.

Next actions, in order:

- [~] Establish an unobscured close three-quarter/side inspection view of the live
  animal. Change QA camera framing, not population or vegetation to hide problems.
- [ ] Inspect/refine head-to-shoulder continuity, muzzle/nose/ear anatomy and silhouette
  using that view; retain the shared collision and fitted-leg behavior.
- [ ] Improve close bristle/coat readability where the unobscured image supports it.
- [ ] Inspect multiple walking poses and audible grunts/snuffles separately. Existing
  sole-centre numerical accuracy and audio counters do not establish natural motion/sound.

## Boar close-view follow-up and rounded profile

The first close camera attempt, `20261011T233518.569115Z-boars`, passed technical
capture gates but was rejected for anatomy inspection: leaves still covered the head
and the closer observer triggered the normal6m retreat behavior. No animal AI or
vegetation was changed to force a pretty image.

The accepted inspection camera uses initial heading-relative forward2.8m/side-6.6m
offset (7.17m horizontal), elevation max(terrain+0.4m,animal+2.4m), and0.50rad vertical
FOV only for `boars`. Ordinary gameplay FOV stays unchanged. It avoids the initial
retreat trigger, not later movement/retreat forever; camera changes naturally alter
the observer input. There is no frozen/teleported animal or hidden foliage.
`20261011T233703.060365Z-boars` PASS and actual image inspected: head, shoulder and most
of the torso are readable; grass still obscures lower legs. Angular ears, coarse
shoulder outline and weak coat readability are now easier to assess.

`MakeBoarBodyProfile` now retains its11 authored rings and adds10 bounded nonuniform
Hermite half-stations. Each interpolated component is clamped between its adjacent
anchors; extrema, length and rooting dip remain bounded. Nearby geometry uses all21
rings (+960vertices/+320triangles per animal); far geometry retains the11 anchors.
The existing36000vertex batch remains; an additional per-animal6000vertex guard
prevents a partly visible herd from masking an over-budget model. Collision, AI and
fixed-length leg fitting are unchanged. CPU tests verify order/finite positive radii/
no overshoot alongside existing full-cycle foot/joint/grounding regressions.

Actual rounded-profile `20261011T233914.459629Z-boars` PASS, image comparison viewed:
modest smoothing of shoulder/rump contours, not a completed anatomy overhaul.
Before/after crop retained as `build/boar-rounded-comparison.png`; kernel-after empty.
The captured version used the extra rings at distance too; the subsequent far-mesh
decimation keeps only original anchors. Final `20261011T234145.503731Z-boars`
PASS/image actually inspected/kernel-after[]; body/head still readable, grass partly
covers feet. All41non-display tests PASS63.03s; Android ARM64 build/lint PASS7s.
No actual Android/Web run of the new profile/camera yet; no animation acceptance.
Web build/package PASS11,343,447raw/4,332,469gzip bytes; all owned jobs terminal,
hardware stop latch unchanged. Builds do not replace portable runtime qualification.
Remaining priorities: cupped ears, muzzle/eye detail, coat/bristles and unobscured
moving feet. Do not replace those tasks with a claim that a smoother loft is realistic.

## Cupped boar ears

`BoarEarGeometry.hpp` replaces each three-triangle fin with an eight-point rounded
outline. Near ears have a rim, recessed inner surface and closed back (32triangles);
far ears retain the same outline as an8triangle fan. Both sides are mirrored with
reversed winding; rooting dip/body bob only translate the posed mesh. Front normals
are area-weighted across the rim/cup transition, with a separate back-side normal
group. Four side/LOD rest meshes cache this calculation; it is not repeated per frame.
Colour varies continuously with distance from the cup centre, instead of a hard dark
polygon. Existing coat colour/BasicEffect/one batch remain, no new texture/shader.
Added174near vertices/58triangles or30far vertices/10triangles per animal, within the
existing6000per-animal/36000batch guards.

CPU tests check both LODs/sides/dip/bob extremes, finite bounds, exact mirror position,
normal mirror/unit length, triangle winding/nondegeneracy, closed near surface edges
and rejection of invalid sides. First test found1.5mm inward overshoot at the root;
fixed the generated inner contour, retained the failed `boar-ear-tests.log` and did
not relax the bound. Final41/41 non-display tests PASS59.05s; asset audit includes
the generator (82sources,12project assets,0external).

Actual `20261011T234702.122104Z-boars` technicalPASS/image comparison inspected: new
outline readable but inner flat-shaded polygons too harsh. Normals/colour revised;
`20261011T234925.212313Z-boars` PASS and comparison actually inspected, kernel-after[].
Second image has softer shading and rounded/cupped outlines rather than triangular
fins. Crops in `build/boar-ear-comparison.png` and `build/boar-ear-smooth-comparison.png`.
The model is still stylized; ears are only one anatomy step. Muzzle/eyes, coat and
unobscured moving-foot/whole-animal review remain open. No AMD fix claim.
Actual Android `20261011T235223.099135Z-boars` PASS, screenshot actually inspected:
current telephoto framing, rounded body and cupped ears present; grass still obscures
feet. GLES SwiftShader, kernel-final empty. Tested archive
`build/android-software/island-boar-ears-x86_64.apk`, SHA256
`cd1ba83ea64346b53eaccf3144217b4d0fe0ef4be413511e2cdfbfe74cf831ff`.
x86_64 build/lint PASS5s; default ARM64 restored/build/lint PASS2s. No emulator/KVM
configuration changes. This is a still-image/material check, not natural-gait proof.
Web build/package PASS11,350,607raw/4,331,592gzip bytes. Actual browser audit remains
pending for current boar geometry/framing; all owned jobs terminal, stop latch unchanged.

## Surface-fitted boar eyes

`BoarEyeGeometry.hpp` replaces near-eye ellipsoids with a shallow dark cornea and
eight-segment eyelid rim. Each point is fitted against the actual16-sided head
triangles, not an analytic ellipse that differs from the rendered surface. Outer
rim, inner rim and cornea centre sit1.5/4/9mm outward in local X; rooting dip and
body bob are included and both sides mirror exactly. Circular head directions are
cached. Existing BasicEffect, coat material, batch and vertex guards remain unchanged;
far geometry and nostrils are unchanged. This does not add a new texture or shader.

Tests cover31 rooting poses at both bob extremes, exact mirror/contact, independently
constructed head-triangle centroids, outward/nondegenerate eye triangles and invalid
side/nonfinite pose rejection. Full41/41 non-display tests PASS68.62s. Native build
and Android ARM64 assemble/lint PASS (5s); no Android eye runtime is claimed.

Actual native `20261012T000031.761433Z-boars` PASS; complete image and comparison
`build/boar-eye-comparison.png` inspected. Final cached-direction build capture
`20261012T000617.039228Z-boars` also PASS and full image inspected; both kernel deltas
empty. The eye is flatter and less button-like, but the visual difference is modest:
the rim remains poorly readable at this distance, the head still looks stylized,
and grass hides much of the feet. These live captures are not pose-identical images.
This is a surface-contact improvement, not completed face realism or gait acceptance.
Next: muzzle/cheek shape, readable eyelids/coat and unobscured moving-foot review.
Actual Android/Web eye appearance remains unverified; the AMD incident stays open.
Web build/package PASS11,354,610raw/4,333,324gzip bytes. Asset validation PASS with
83generator sources,12project files and0external files. All owned jobs terminal;
hardware stop latch unchanged. These builds do not replace portable runtime checks.

## Cheek-to-muzzle shape and closed-mouth crease

The existing head profile now has broader cheeks and a raised, narrower nasal
bridge, rather than a nearly straight forehead-to-nose wedge. Nose anchor, body
envelope, limb roots, rooting coefficients and21-ring count remain unchanged.
Both body LODs use the revised anchors. Near animals additionally have a tapered
closed-mouth crease on each side, fitted to the same actual head triangles as the
eyes. It uses21 paired stations,40triangles per side (+240vertices per animal),
the existing batch/material and no extra texture, shader or draw.

Initial6-station crease passed vertex-contact checks and the image gate, but a new
interior-face sampling test found buried patches during rooting. The failed full
suite remains in `build/boar-mouth-suite.log`. Subdividing each authored span into
four segments fixed this without relaxing the test or moving the whole line further
off the head. Tests cover31 dip poses, both bob extremes, exact mirrored vertex
contact, strip order/winding/nondegeneracy, sampled face clearance and invalid inputs.
This is sampled contact coverage, not a continuous mathematical nonintersection proof.

Native cheek pass `20261012T000914.687774Z-boars` and first crease pass
`20261012T001030.745801Z-boars` technically PASS; before/after crops actually viewed.
Final subdivided crease `20261012T001239.781186Z-boars` PASS, complete image actually
inspected, kernel-after empty. Cheek fullness and the narrower muzzle are readable;
the mouth crease is subtle/partly obscured, not a new prominent black outline.
Animals remain stylized and these live poses differ, so this is not a pixel-aligned
comparison or natural-motion acceptance. Nose/nostrils, coat readability and moving
feet still need review. No population, vegetation, camera or AI changes were made.
Final full CPU suite PASS41/41 in61.02s. Actual Android SwiftShader run
`20261012T001308.822363Z-boars` PASS, screenshot actually inspected, kernel-final[].
It now covers current fitted eyes as well as cheeks/crease; head details remain
subtle and foliage still occludes the feet. Tested APK retained as
`build/android-software/island-boar-face-x86_64.apk`, SHA256
`58f81d0d116d21fb263f595ffbf7fee9d1cc81f6754277f97eded70d0a90885a`.
x86_64 and restored default ARM64 build/lint both PASS3s. No emulator/KVM configuration
changes. Asset audit PASS84sources/12project files/0external. Actual Web inspection
and continuous gait/audio remain open; this does not resolve the AMD flight incident.
Web build/package PASS11,356,327raw/4,334,892gzip bytes. All owned jobs terminal;
hardware stop latch unchanged. No push.

## Monkeys (unchanged baseline)

Evidence: `build/software-captures/20261011T233132.293700Z-monkeys/capture.png`.
Technical capture PASS:326colour bins, luminance deviation33.35, spatialcontrast180.72.

The subject is recognizable with directional fur and a horizontal trunk, so this is
not the old plain-brown primitive-only material. Remaining defects are visible:
flat mask-like face with very small dark eyes, coarse limb/joint transitions, an
upright sharply bending tail, and grass obscuring parts of the hands/feet. The trunk
still reads somewhat box-like. No continuous walking or climbing sequence was captured
in this pass; previous separate climbing evidence does not close these appearance issues.

- [ ] Add facial relief/eye sockets/lids and soften cheek/muzzle transitions.
- [ ] Refine shoulder/hip contours and tail curvature within measured mesh/bone budgets.
- [ ] Recheck unobscured feet and a sequence of walking/climbing poses on all software
  targets before marking natural anatomy or motion accepted.

The full requested species/resident backlog remains open. These new images neither
add missing species nor diagnose/fix the unresolved AMD flight incident.

## Actual Web wildlife/foliage follow-up

Current build after `4593dc8`, serial contained Chrome/ANGLE SwiftShader run:
`build/browser-software/20261012T001634.994190Z-chrome-wildlife-foliage`.
This is actual browser rendering, not the previously recorded compile-only evidence.

Inspected all six PNGs: initial jungle, boar, monkey and east views, plus boar and
monkey post-simulation views:

- Jungle: recognizable layers of ground cover and trees, but angular crown silhouettes
  and repeated leaf shapes remain conspicuous. Technical rendering is not visual realism.
- Boar initial view: current rounded body, cupped ears, fitted eyes and fuller cheek
  profile are present. Mouth detail is subtle; grass still obscures much of the feet.
  The coat looks like broad brown streaks rather than clearly resolved bristles.
- Boar post-simulation view: the animal has moved to the far left behind vegetation.
  This demonstrates visible scene change, not correct continuous gait or foot contact.
  A fixed camera cannot provide sustained whole-animal motion acceptance here.
- Monkey initial view: directional fur and horizontal trunk are readable; face remains
  a flat mask with point-like eyes, limbs have abrupt transitions and the tail bends
  sharply. This is usable facial-detail evidence, not an accepted natural monkey model.
- Monkey post-simulation view: the tracked subject remains visible with a changed pose;
  the flat face/point eyes and angular limb transitions remain apparent. Two stills do
  not establish continuous gait quality, absence of foot sliding, or climbing behavior.
- East: the steep exposed slope reads as a broad blurry brown/grey smear, lacking clear
  rock/soil structure. The initial image alone does not identify whether texture scale,
  projection, filtering or material blending is responsible; inspect those before fixing it.

Next substantive audits/fixes:

- [ ] Improve close boar bristle scale/readability, preserving atlas padding and budget.
- [ ] Add a tracking motion review that keeps a naturally moving boar in view without
  freezing/teleporting it or removing vegetation. Record multiple unobscured foot poses.
- [ ] Refine monkey face relief/lids and limb transitions using the actual close view.
- [ ] Revisit crown/leaf silhouette repetition in the jungle view.
- [ ] Diagnose and improve material readability on the steep eastern slope, retaining
  the established terrain shape/collision and portable texture budget.

Final result: Chrome155.0.8059.39, all four views PASS with60 requestAnimationFrame
intervals each, GL error0 and contextLost=false. Parent `run.json` also passed;
`kernel-final.json` is empty. These software timings are not hardware performance
evidence. No code/build/dependency/emulator/KVM changes were made during this audit.
All owned processes terminal; hardware stop latch SHA c0bd1704 unchanged. Current
boar Web appearance is now actually checked, but visual quality/gait/audio, the full
species/resident backlog and AMD incident remain open.

## Monkey orbital relief follow-up

`MonkeyFaceProfile` now sculpts bilateral recessed orbits, raised brow arcs, a nasal
bridge and cheek relief into the existing connected face grid. Compact smooth bumps
fade to zero at their limits and are multiplied by the transverse arc, retaining
the fur-joining perimeter and existing head envelope. The eye centres move10mm back
to remain seated in the recessed surface rather than floating ahead of it.

Because the inspected face is largely in shade, the skin vertices also carry bounded
0.8..1 ambient retention in the eye sockets, encoded by reserved negative V values.
The existing skinning shader applies this only to ambient light; direct sun/specular
are unchanged. Ordinary fur UVs remain positive and unaffected. This is an authored
local-occlusion approximation, not dynamic self-shadowing. No additional triangles,
draws, textures, vertex attributes or uniforms; existing14bones/92uniform-vector
budget, animations, population and camera remain unchanged.

CPU tests verify bilateral bounded relief/ambient retention, brow/orbit/nasal shape,
the authored envelope, approximate orbital eye seating and both shader dialect
contracts. Eye seating uses interpolation of the sampled profile, not an exhaustive
triangle/eyeball intersection proof. Full41/41 non-display tests PASS56.96s.
Actual native `20261012T003001.503299Z-monkeys` PASS, complete image and enlarged
comparison actually inspected, kernel-after[]. Comparison
`build/monkey-face-relief-comparison.png` uses the previous Web image and current
native image; poses/backends differ, so it is not a pixel-equivalence test.
The brow/orbital shading is now faintly readable, but the change is modest at the
ordinary inspection distance. The face is still simplified; eyelid detail, cheek/fur
boundary, ears and limb transitions remain open, as do continuous walking/climbing
and the full species/resident/audio backlog. No AMD qualification or fix is claimed.
Actual Android SwiftShader `20261012T003122.519072Z-monkeys` PASS; complete screenshot
actually inspected, parent passed/kernel-final[]. It renders the current orbital
profile and skinning shader, but does not establish natural motion or climbing.
Archived `build/android-software/island-monkey-orbits-x86_64.apk`, SHA256
`61502971ca76ad607c78ddf578bfc06534de5218042b1baf3c841456c45de34a`.
x86_64 and restored default ARM64 build/lint both PASS3s. Asset audit PASS84sources/
12project files/0external. No Android/KVM/dependency configuration changes.
Web build/package PASS11,358,303raw/4,335,409gzip bytes; actual Web runtime of this
latest shader/profile remains pending. All owned jobs terminal, hardware stop latch
unchanged. Local-only work, no push.
