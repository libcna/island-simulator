# Offshore shark form

## Image-driven repair

The former 51-vertex close shark used the same diamond body construction as the old fish and a
symmetric forked tail. Its baseline contained llvmpipe capture
`build-consumer/software-captures/20261012T210613.753528Z-shark` was recognizable mostly from one
oversized triangular dorsal fin.

The first revised close form used seven eight-sided body stations plus a capped blunt snout. The
second inspected pass now uses eight stations and ten facets, including a shorter rounded forehead
transition. Maximum girth remains forward of the midpoint, the rear contracts through a distinct
caudal peduncle, and the rear stations progressively follow the existing body-wave state. The caudal fin is
heterocercal: its upper lobe is longer and more swept than the lower lobe. A two-facet primary
dorsal, small secondary dorsal, paired shaped pectoral and pelvic fins, two-facet highlighted eyes,
five narrow swept gill slits per side, paired nostril marks and a two-part mouth crease provide
shark-specific cues. Every thin fin now receives two-sided fixed-sun lighting. Dark dorsal facets,
grey flanks and a pale belly share the existing activity fade and underwater daylight model.

The occasional, peaceful patrol behaviour, depth pitch, turn bank, presence schedule, habitat and
15-vertex distant silhouette are unchanged.

## Cost and evidence

The close model rises from the original 51 through 375 to 594 vertices. Because only one shark
exists, its fixed dynamic buffer is 9,504 bytes and it still uses at most one draw. Each generated close form is checked
against that exact compile-time budget. Native and Web smoke diagnostics pin the same inventory.

Contained llvmpipe/pixman evidence
`build-consumer/software-captures/20261013T031641.255166Z-shark` passed image, runtime, OpenGL and
kernel-delta checks with one close shark in one draw. The closer QA camera follows translation only,
keeping live patrol heading, pitch, bank and tail motion visible. Inspected focused
Chrome/SwiftShader evidence `build/browser-software/20261013T031750.468097Z-chrome-shark` retains
the animal in two different-hash frames over 60 positive intervals with GL error zero, an intact
context and an empty kernel delta.

The model remains deliberately low-poly. Teeth, gums, nostril recesses, curved fin thickness,
skin texture/scars, current Android execution and integrated performance measurement remain open
under the broader aquatic visual-polish task.

## Closed-fin pass (2026-10-13)

The current visual audit confirmed that the body and shark-specific cues remained readable, while
all eight near fin/lobe surfaces were still coincident sheets. The primary and secondary dorsals,
paired pectorals, paired pelvics and asymmetric upper/lower caudal lobes are now thin closed
volumes. Each keeps its authored outline, adds separate broad faces plus four darker perimeter
edges, and uses restrained 6--12 mm local half-thickness. Patrol motion, body wave, habitat,
activity tint, far silhouette and one-draw batching are unchanged.

The eight replacements add 240 vertices, bringing the exact near budget to 834 vertices / 278
triangles and 13,344 bytes for the single shark. Inspected llvmpipe evidence
`build-consumer/software-captures/20261013T113920.218101Z-shark` passes with 266 colour bins, 31.43
luminance deviation, 133.04 spatial contrast and an empty kernel delta. The side view preserves the
blunt head, gill row and asymmetric tail while retaining fin edges through the live bank/tail pose.
Focused Chrome/SwiftShader evidence `20261013T114307.083425Z-chrome-shark` passes; both inspected
frames retain the close patrol in different body/tail poses and their SHA-256 identities differ.
All 60 intervals are finite, `glError` is zero, the context remains live and the kernel delta is
empty. Android execution, continuous interactive observation, curved skin transitions and
texture/scars remain open.

## Smooth body pass (2026-10-14)

The next visual audit found the eight-station/ten-facet torso readable but conspicuously angular
beside the revised fish, dolphin and turtle. The close shark now uses fourteen longitudinal
stations and eighteen radial samples, with capped nose and rear-peduncle transitions. Per-vertex
radial daylight and blended dorsal/flank/ventral countershading replace the former whole-face
colour bands. The peaceful patrol, live body wave, closed fin set, anatomy marks, far silhouette
and one-draw batch are unchanged.

The exact close budget is 1,896 vertices / 632 triangles (30,336 bytes for the single animal).
Inspected llvmpipe/pixman evidence
`build-consumer/software-captures/20261014T030010.513058Z-shark` passes with 289 colour bins, 31.47
luminance deviation and 134.33 spatial contrast. Focused Chrome/SwiftShader evidence
`build/browser-software/20261014T030558.932289Z-chrome-shark` passes with two inspected animated
frames, 60 finite intervals, GL error zero, an intact context and no new kernel GPU fault. The
ARM64 debug APK assembles and lint passes without starting an emulator.
