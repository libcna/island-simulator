# Island bear — procedural second pass in progress

The requested island contains exactly one large peaceful adult bear. The current implementation is a
real simulated and rendered animal, not a static prop, but it is deliberately not recorded as
final photorealism or portable acceptance.

No third-party mesh or texture is shipped. A brief asset review found either deliberately
low-poly models or realistic listings without a simultaneously verifiable downloadable and
redistributable artifact. `BearRenderer.cpp` therefore creates original geometry and a
128x128 mipmapped directional-fur atlas at runtime. The close model includes a heavy torso,
raised shoulders, neck, short tail, muzzle, nose, restrained eyes, rounded outer/inner ears,
four jointed legs, broad paws and claws. The second visual pass replaces the pale cylindrical
snout with two overlapping tapered muzzle volumes, a smaller volumetric nose, paired nostrils,
a shallow philtrum/divided mouth crease and highlighted inset eyes. A 55m near tier uses 6734
triangles; the cheaper far tier omits face/claw detail. Body and one 18-triangle terrain shadow
remain two bounded draws.

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
fur silhouette and joint volumes need more artistic refinement.

The focused WebGL audit is now part of the supervised Chrome harness rather than an ad-hoc page.
`build/browser-software/20261013T034840.525068Z-chrome-bear` contains two inspected, different
time-separated images with the bear still framed, 60 finite frame intervals, ANGLE SwiftShader,
GL error zero, an intact context and an empty kernel-fault delta. The second frame is a rear view
partly occluded by foreground vegetation; this proves portable rendering and camera tracking, not
all-angle visual acceptance or physical-device performance.

WorldReport exports count one, finite home/position and grounded limb state; current fingerprint
is `d0edd20a977a3eaf`. Texture accounting includes the complete 87,380-byte bear atlas mip chain,
for 9,305,216 audited runtime RGBA8 bytes inside a 9,325,432-byte profile limit.

## Still required

- Refine muzzle/neck joins, fur silhouette and leg-joint volume beyond the current face pass.
- Review longer walk/trot/turn/forage sequences and hear all call kinds on real speakers.
- Run Android visual/audio lifecycle tests and physical performance checks.

These open items keep the PLAN entry `[~]`. The whole island plan remains active.
