# Connected monkey face profile — native first pass

## 2026-10-15 compact primate head and socketed readable eyes

A fresh side-by-side animal audit found the bear recognisable and the boar acceptable, while the
monkey still had the weakest close silhouette: a large circular cranium and skin mask, oversized
disc ears, almost hidden black eye slits and a projecting dog-like paired muzzle. The cranium is now
10--15% smaller, the ears and cheek ruffs are reduced and moved inward, and the twelve-row facial
surface tapers from a compact brow into narrower cheeks. The muzzle pads, nose and curved mouth are
shorter and flatter; their existing signed-UV material separation is retained but the pads are a
less severe midtone.

The corneas now intersect the recessed socket surface and sit behind fitted eyelids. Each eye adds
one tiny raised catchlight inside the existing eye buffer; a reserved negative-U shader tag raises
only those vertices from the same dark-brown eye material, so this costs no texture, buffer or draw.
Slightly lighter muted skin palettes keep the face modelled in canopy shade. The existing head bone,
fourteen-bone palette, fur draw, face draw, eye draw, LOD distances and animation clips are unchanged.
The CPU contracts now explicitly protect the compact heart-shaped outline, low-profile muzzle,
cornea/socket intersection and portable catchlight tag.

Exact-final native close evidence
`build-consumer/software-captures/20261015T041051.347080Z-monkeys` passes (212 colour bins, 43.01
luminance deviation, 222.47 spatial contrast) and was inspected. The current-binary eight-stage
climb sequence `build-consumer/software-captures/20261015T041133.348233Z-monkey-climb` passes all
stage validations. Chrome/SwiftShader
`build/browser-software/20261015T041359.433988Z-chrome-monkeys` passes 60 finite positive intervals,
GL error zero, a live context and clean kernel monitoring; all six movement frames were inspected.
ARM64 assemble/lint passes through the bounded wrapper without emulator/KVM (24,168,603-byte APK,
SHA-256 `1d35652edf2aa577df109d11cfaefde72da110a49811c4af1cd0a617d8973f43`, 19,126,032-byte stripped
`libmain.so`). Physical-device appearance remains unqualified.

## 2026-10-14 muted skin and recessed perimeter

The portable movement review after the compact-body pass exposed the remaining face mismatch:
the uniform high-value tan palette made the entire connected facial surface read as a flat beige
shield against dark fur. All four skin palettes now use muted brown-grey values. The existing
signed face UV adds a bounded lateral contour shade, darkening the embedded perimeter by up to 22%
while leaving the centre, projecting muzzle pads, nose, eyes, eyelids and socket retention intact.
This is vertex-shader colour modelling, not another texture or draw.

Geometry, head bone, face/eye buffers, animation and draw counts are unchanged. Ordinary native
captures `20261014T165801.538669Z-monkeys` and `165936.822910Z` pass but are rejected as appearance
evidence because a foreground leaf and rearward head angle hide the face. The complete current
nine-frame tree sequence `20261014T170120.376359Z-monkey-tree` passes and was inspected as a
montage: multiple front-quarter states retain the eyes, fitted nose and muzzle while the darker
perimeter joins the cheek fur more naturally.
Focused Chrome/SwiftShader `20261014T170630.483212Z-chrome-monkeys` passes six inspected movement
frames, 60 finite positive intervals, GL error zero, an intact context and no new kernel GPU fault.
The default arm64 APK assembles and application lint passes through the two-worker wrapper without
starting an emulator; Android runtime appearance remains unqualified.

## 2026-10-14 close-audit muzzle repair

The first unobstructed front-quarter capture exposed two defects that the former lateral
camera had hidden behind a trunk: the facial skin ended in a sharp planar forehead and four
dark nose marks made the animal read as pig-snouted. The deterministic `monkeys` viewpoint
now sits inside the clear stand-off between the subject and its selected tree, follows only
translation, and targets the face. This is capture-only framing; behaviour is unchanged.

The skin outline now has a rounded upper/lower transition, while two embedded, midtone
ellipsoids form projecting muzzle pads in the existing face buffer. A smaller volumetric
nose has exactly one fitted nostril per side, and the mouth crease was moved in front of the
sampled muzzle surface so it is no longer depth-hidden. The negative-U face tag gained a
separate bounded muzzle shade; the same two detail draws, head bone, palettes, fur texture,
seven agents and animation remain. Native close evidence
`20261014T135115.551391Z-monkeys` passes and was inspected against the unobstructed baseline
`20261014T134837.560489Z-monkeys`: the projecting muzzle, two nostrils and mouth are now
readable, and the silhouette remains recognisably quadrupedal. It is still a deliberately
procedural, stylised animal rather than a scanned photoreal model. The WebGL target builds;
Android ARM64 assemble/lint passes without starting an emulator or touching KVM. All 61
enabled non-display CTest cases and the 112 software/browser/Android harness unit tests pass.

The close audit rejected the two pale oval face components as a toy mask. They are now
replaced by one twelve-row transverse surface from forehead to cheeks, muzzle and chin,
recessed into the fur around the perimeter. Surface normals follow the neighbouring rows;
the muzzle projects beyond the orbital plane. Smaller eyes and nostrils meet that surface.
The mouth is a narrow curved strip following the muzzle, not a straight floating bar.
Four muted skin palettes replace the yellow-beige mask colours. The same head bone and
existing face/eye draws are retained. No new image asset or dependency change.

`MonkeyFaceProfile.hpp` is registered in the asset manifest. The existing monkey profile/
gait CPU test covers ordered noncollapsed rows, finite bilateral samples, envelope bounds
and muzzle projection. Native startup validates the generated triangles. Build and all
37 non-display groups pass (93.53 s); asset gate passes with 67 generator sources.

Native `20261011T125510.820617Z-monkeys` passes and was inspected against the prior close
view. The two separate bright oval patches are gone, replaced by one continuous lower-
contrast face. Kernel delta empty. The face still reads as simple, low-relief geometry;
eye sockets/lids, cheek/forehead transitions and skin variation need further work. The
coarse/wavy fur and assembled shoulder silhouette are also still defects. This is not
complete monkey realism. Current Android/Web face qualification and moving views remain
pending; previous portable images show the old face. Contact locking and climbing remain open.
