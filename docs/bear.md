# Island bear — procedural second pass in progress

The requested island contains exactly one large peaceful adult bear. The current implementation is a
real simulated and rendered animal, not a static prop, but it is deliberately not recorded as
final photorealism or portable acceptance.

No third-party mesh or downloaded texture is shipped. A brief asset review found either deliberately
low-poly models or realistic listings without a simultaneously verifiable downloadable and
redistributable artifact. `BearRenderer.cpp` therefore creates original geometry and a
project-owned ImageGen material now supplies the 128x128 mipmapped directional fur. The close model includes a heavy torso,
raised shoulders, neck, short tail, muzzle, nose, restrained eyes, rounded outer/inner ears,
four jointed legs, broad paws and five claws per paw. The second visual pass replaces the pale cylindrical
snout with two overlapping tapered muzzle volumes, a smaller volumetric nose, paired nostrils,
a shallow philtrum/divided mouth crease and highlighted inset eyes. A 55m near tier uses 5,578
triangles; the cheaper far tier omits face/claw detail. Body and one 18-triangle terrain shadow
remain two bounded draws.

## Continuous trunk and limbs — 2026-10-13

The three overlapping torso, shoulder and neck ellipsoids have been replaced by one thirteen-
station loft from rump through belly, raised shoulder and neck into the still articulated head.
Its station centres and elliptical radii respond to breathing, body bob and the forage pose, so
the former three spherical lighting seams cannot appear while the bear lowers its head. The
twenty-two existing fur wedges now follow the exact loft upper envelope.

Each leg similarly replaces upper tube + spherical knee + lower tube with one nine-station
shoulder/haunch-to-wrist loft through the authoritative IK joint and paw target. Fore and hind
profiles use different muscular radii; the lower limbs remain broad rather than tapering into
sticks. Every near paw now exposes five restrained claws instead of three. Planted-foot targets,
fixed bone lengths, behavior, shadow, material, texture and two-draw contract are unchanged.

The old three body shells cost 1,848 near triangles; the continuous torso costs 528. Continuous
limbs add 8 triangles total and the eight extra claws add 112, so the complete close model falls
from 6,778 to 5,578 triangles (-1,200 / 17.7%) despite the anatomical additions. An initial Web
review correctly rejected longitudinally stretched fur and a visible non-tileable atlas seam.
The final mapping runs guard hairs along the trunk and rotates the source-window seam beneath the
belly rather than across a broad flank.

Final native llvmpipe evidence `20261013T232603.134747Z-bear` passes at 1280x720 with
189 colour bins, 36.58 luminance deviation, 168.50 spatial contrast and no new kernel fault.
All eight poses were inspected. Focused Chrome/SwiftShader
`20261013T233028.007588Z-chrome-bear` passes 60 finite intervals, two distinct inspected
frames, `glError=0`, intact context and an empty kernel delta. Release and Debug bear-motion
tests, 89 focused harness tests, WebAssembly, and ARM64 assemble/lint pass. This removes the
most conspicuous body/joint assembly seams but does not claim photorealism, Android runtime,
physical performance or subjective speaker acceptance.

## Fine fur material and silhouette follow-up — 2026-10-13

The renewed current-binary audit `20261013T094848.144035Z-bear` confirmed that the original
runtime-generated greyscale atlas read as a smooth muddy surface. Built-in ImageGen produced
two original 1254-square sources with exact prompts preserved under
`assets/art-studies/bear-fur-albedo-v1*` and `bear-fur-albedo-v2*`. The first live integration
`20261013T100755.881806Z-bear` was visually rejected: large wavy locks became repeated wool-like
ropes on the separate body volumes. It remains provenance and failed-iteration evidence only.

The selected v2 source deliberately uses finer, straighter, lower-contrast guard hairs and a
dense dark undercoat. The host pipeline area-reduces it to `Content/wildlife/bear-fur.png` at
128 square; runtime loads eight opaque mips / 87,380 RGBA8 bytes instead of synthesizing pixels.
The existing UVs now use the complete clamp-bounded source, mirrored for dark inner ears and a
small bounded facial window. Geometry still renders in the same body draw and keeps the exact
texture-memory budget.

Twenty-two near-only two-triangle fur wedges follow the computed upper envelope of the live
breathing torso, shoulder, neck and foraging head. They add 44 body triangles but no texture,
draw, permanent buffer or population cost. Release/Debug bear-motion and exact prepared-pixel
tests, the asset/provenance validator and native packaging preparation pass.

Final inspected llvmpipe `20261013T102141.572218Z-bear` passes at 1280x720 with 189 colour bins,
36.45 luminance deviation, 165.37 spatial contrast and an empty kernel delta. All eight poses
retain fine fur, the restrained broken top outline, 2.3135m displacement and 0.4769m aggregate
paw motion; the renderer reports 6,778 body / 18 shadow triangles in two draws. Focused
Chrome/SwiftShader `20261013T103349.512195Z-chrome-bear` passes 60 finite intervals, two distinct
images, `glError=0`, an intact context and empty final kernel delta. Both images were inspected.
The result is materially less toy-like, but body-volume joins, finer facial topology, Android and
subjective speaker listening remain open; this is not final photorealism acceptance.

## Habitat and motion

`BearMotion` deterministically finds one dry, mild-slope interior home clear of palms, jungle,
props, wreck, hut, fire and cave. The animal remains within a 34m home radius. It does not
attack or chase: a grounded observer inside 12m causes a bounded withdrawal; a high flying
observer does not. Idle, walk, trot, turn, forage, rest and withdraw states use bounded speed
and turn rate.

Each paw retains a world-space stance anchor. At most two paws swing while the others stay
planted. Two-link IK preserves 0.48m upper and 0.46m lower limb lengths and rejects unreachable
body movement rather than stretching a leg. Paw soles sample the same emitted terrain surface
as the renderer. The 600-second Release and Debug regressions are bitwise deterministic,
travel 219.719m, bound one update to 0.16559m, exercise every behavior and every leg, preserve
two-foot support, prevent stance sliding/penetration, miss the complete world obstacle set and
reject a fully blocked island.

## Voice and source budget

Six original 1.6-second mono PCM clips provide two variants each of low breathing, snuffling
and a short non-aggressive huff. Call choice and cadence follow behavior; 3D attenuation keeps
quiet details local. The bear consumes the existing single transient wildlife slot, so the
application still caps playback at six simultaneous sources. The 32-clip WAV export is
5,576,444 PCM bytes and passes signal, endpoint, identity and exact-frame validation.

The real CNA dummy-backend regression starts an audible nearby bear call, observes the Playing
path and 4–6 source bound, verifies mute stops/prevents it, and confirms an out-of-range listener
does not create playback. This is backend/lifecycle evidence, not subjective headphone or
speaker listening.

## Native visual evidence

The dedicated `--viewpoint bear` capture uses a fixed 0.1s simulation clock, follows the animal
without making the QA camera trigger withdrawal, and records eight ordered stages. The final
kernel-monitored llvmpipe/pixman run is:

`build-consumer/software-captures/20261013T034649.650571Z-bear`

It shows one visible animal in two draws, 6734 body triangles, 18 shadow triangles, more than
2.3m body displacement and four moving paw chains. All stage images were inspected. The first
visual version had a black shoulder patch, boot-like dark legs and an oversized round tail;
the second pass unifies the coat, reduces the tail, rounds the ears, shrinks bead-like eyes/nose
and raises close tessellation. This follow-up darkens and tapers the muzzle and separates the
nostril, mouth and eye detail. The result reads more clearly as a brown bear with a heavy
quadrupedal gait, but remains visibly procedural: overlapping muzzle/neck surfaces, the smooth
fur silhouette and joint volumes need more artistic refinement. The later selected-v2 material
and silhouette evidence supersedes this image for current appearance while preserving it as the
smooth-atlas baseline.

The focused WebGL audit is now part of the supervised Chrome harness rather than an ad-hoc page.
`build/browser-software/20261013T034840.525068Z-chrome-bear` contains two inspected, different
time-separated images with the bear still framed, 60 finite frame intervals, ANGLE SwiftShader,
GL error zero, an intact context and an empty kernel-fault delta. The second frame is a rear view
partly occluded by foreground vegetation; this proves portable rendering and camera tracking, not
all-angle visual acceptance or physical-device performance.

WorldReport exports count one, finite home/position and grounded limb state; current fingerprint
is `d0edd20a977a3eaf`. Texture accounting includes the complete 87,380-byte bear texture mip chain,
for 9,305,216 audited runtime RGBA8 bytes inside a 9,325,432-byte profile limit.

## Still required

- Refine the remaining head/neck transition, fur silhouette and gait weight transfer.
- Review longer walk/trot/turn/forage sequences and hear all call kinds on real speakers.
- Run Android visual/audio lifecycle tests and physical performance checks.

These open items keep the PLAN entry `[~]`. The whole island plan remains active.
