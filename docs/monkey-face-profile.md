# Connected monkey face profile — native first pass

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
