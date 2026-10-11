# Connected monkey face profile — native first pass

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
