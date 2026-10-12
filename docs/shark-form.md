# Offshore shark form

## Image-driven repair

The former 51-vertex close shark used the same diamond body construction as the old fish and a
symmetric forked tail. Its baseline contained llvmpipe capture
`build-consumer/software-captures/20261012T210613.753528Z-shark` was recognizable mostly from one
oversized triangular dorsal fin.

The revised close form uses seven eight-sided body stations plus a capped blunt snout. Maximum
girth is forward of the midpoint, the rear contracts through a distinct caudal peduncle, and the
rear stations progressively follow the existing body-wave state. The caudal fin is now
heterocercal: its upper lobe is longer and more swept than the lower lobe. A two-facet primary
dorsal, small secondary dorsal, paired pectoral and pelvic fins, surface-fitted eyes, three visible
gill slits per side, and a short mouth crease provide shark-specific cues. Dark dorsal facets,
grey flanks and a pale belly share the existing activity fade and underwater daylight model.

The occasional, peaceful patrol behaviour, depth pitch, turn bank, presence schedule, habitat and
15-vertex distant silhouette are unchanged.

## Cost and evidence

The close model rises from 51 to 375 vertices. Because only one shark exists, its fixed dynamic
buffer is 6,000 bytes and it still uses at most one draw. Each generated close form is checked
against that exact compile-time budget. Native and Web smoke diagnostics pin the same inventory.

Contained llvmpipe/pixman evidence
`build-consumer/software-captures/20261012T211055.205953Z-shark` passed image, runtime, OpenGL and
kernel-delta checks with one close shark in one draw. The image was inspected and is byte-identical
to the preceding successful final-camera run. The initial side-on composition was also inspected
but failed the generic scene colour-bin gate because it showed almost only underwater tones; it is
retained as failed evidence, not cited as acceptance.

The model remains deliberately low-poly. Teeth, gums, nostril recesses, curved fin thickness,
skin texture/scars, portable WebGL/Android captures and integrated performance measurement remain
open under the broader aquatic visual-polish task.
