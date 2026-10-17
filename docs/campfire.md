# Inland-clearing campfire

One continuously burning fire is placed at `(43,24.6174,-18)` in the east inland clearing.
A separate stack of11 gathered logs is at `(49,25.2661,-18)`, 6m away. Twelve irregular
stones surround five burning logs, a layered ash bed, four bright
embers and six darker charcoal chunks.
The woodpile's log ends follow the actual rendered slope, rather than hovering on a
horizontal shelf. No fuel/crafting controls or damage system are introduced.

All geometry, shader material and PCM are original project code. There are no imported
models, external licences or new raster textures. Rock mottling, bark, end-grain rings,
char cracks, ash and glowing coals use procedural material variation.

## Rendering and light

The solid mesh is5976 vertices/1992 triangles. Moving effects are21 camera-facing quads
(42 triangles): six independently varying flame layers, three soft smoke layers and
twelve drifting sparks. Vertex buffers are uploaded once. The effects receive time and
camera uniforms, with premultiplied blending and depth testing against the complete
opaque scene. Solids and effects are drawn in separate scene passes, two culled draws
in total. The shader is supplied in both desktopGL330 and GLES300 forms.

Flame tongues change height and turbulent edge shape over time; coals and warm nearby
light flicker together. Terrain, grass and jungle materials receive the same bounded
6m local light. This preserves the permanent daytime setting. The first-pass appearance
is still stylized: smoke is very subtle, stones are faceted and wood detailing remains
procedural. Further realism and portable/performance qualification remain open.

## Placement and collision

CampfireSite is shared with grass/vegetation/prop layout reservations. The fire has5.5m
plant clearance and the stored wood has2m clearance. The existing world collision grid
contains radius1.25m firepit and radius1.4m woodpile proxies. Mesh containment and actual
PlayerMotion approach tests check that these barriers prevent walking through the fire
or stack while permitting a close approach. The site does not overlap the stream sound
region or the cave. Existing vegetation outside these small reserves remains generated.

## Audio

An original19s mono PCM16 loop combines soft ember hiss, lower fire noise and173 uneven
wood crackles/pops. It is generated at22050Hz, mean centred and loop checked. It adds
837900 PCM bytes; the current audio inventory is32 clips/5576444 bytes. No new playing voice is
added: fire, stream and player-water playback share the fourth persistent slot. This
clearing lies beyond the stream's audible region. Swimming takes priority, and fire
audibility uses a bounded3D24m distance with smoothed gain and transition hysteresis.
Spatial mode is configured before the first Play, preserving the prior dropout fix.

Real-CNA dummy-audio tests cover far/near startup, repeated fire→stream→far→swim→high
flight→fire transitions and mute/unmute, querying actual Playing state and the4–6 source
budget. This establishes backend operation, not speaker listening or perceived naturalness.
The independent WAV exporter also checks all32 clips and their complete PCM footprint.

## Native evidence

Release core, campfire, world report, audio inventory/export and audio runtime tests pass
6/6 in17.07s. Python software-supervisor checks49/49. The additional physical approach
test passes in Release0.28s. Rebuilt Debug cave/campfire/audio-runtime tests pass3/3 in7.34s.

Inspected isolated llvmpipe/pixman captures:

- `build-consumer/software-captures/20261012T180909.580398Z-campfire`: near view,
  four explicit0.8/1.6/2.4/3.2s animation stages;370 colour bins/deviation39.12, PASS.
- `build-consumer/software-captures/20261012T180918.134168Z-campfire-wide`: clearing,
  ring and stored wood together;335 bins/deviation42.89, PASS.

The four-stage montage visibly changes flame tongues and spark positions; the wide view
shows the fire and distinct woodpile with surrounding ground clearance. No new kernel
GPU fault occurred. Capture mode deliberately disables audio; audio evidence comes from
the separate runtime and PCM tests. The first capture180516 produced valid images but
the initial visual harness wrongly required an audio-start diagnostic from silent smoke
mode. That scope error was corrected without changing the runtime audio requirement.

World fingerprint including the cave, campfire mesh and both collision proxies:
`97cc56c53b547613`. WorldReport exports the centres and solid/log counts.

Live native keyboard/disk-audio gate passes at
`build-consumer/software-campfire/20261012T182016.847082Z`. F1+O places the player nearby;
W reaches the protected stone ring without crossing its collision boundary. The gate
queries actual CNA loop Playing state and gain, checks mute with1032192 exact-zero PCM
bytes, resumes204800 nonzero bytes, then walks beyond24m and verifies voice release.
The close screenshot was inspected. Both native executable trees are refreshed; Debug
audio lifecycle tests pass2.92s. CPU-only harness tests50/50. This focused gate omits F11.

Remaining: speaker listening review, smoother natural
material/smoke refinement, representative performance and consolidated Web/Android runtime.
Both native executable build trees are refreshed. The separate private F11 restoration
finding remains open. No AMD, Android emulator or
KVM setting was changed for this work; all builds are restricted to CPUs0–3 and at most4 workers.

## Translucent flame breakup and focused WebGL evidence

The next shader-only visual pass keeps all21 quads,42 effect triangles and both existing draws.
Time-varying high-frequency breakup and narrow strand modulation stop the six flame planes from
merging into one opaque orange cut-out. A surface-distance core adds pale yellow-white heat, while
a restrained centre-weighted blue component is confined to the lowest17% of the flame. Smoke uses
a second drifting opacity modulation and a slightly broader warm-grey range; sparks, solid geometry,
local lighting and audio are unchanged. The premultiplied output remains depth-read against solids.

Native llvmpipe `20261013T124837.259373Z-campfire` passes all four ordered0.8–3.2s stages,
366 colour bins,39.03 luminance deviation,172.00 spatial contrast and kernel[]. The final frame
and four-stage montage were inspected; the tongues retain visible motion while showing softer,
broken edges and less solid overlap.

A new focused browser scope makes this more than a Web build claim. Chrome/SwiftShader evidence
`20261013T125359.878012Z-chrome-campfire` passes two different inspected time-separated images,
60 finite intervals, GL0, an intact context and kernel[]. Static validators require the current
material diagnostic, exact42-triangle/two-draw budget and both image paths. Android runtime,
speaker listening, continuous moving-view review and representative performance remain open.

## Ash and charcoal material pass

The existing solid mesh now separates the fire bed into a pale grey-brown, two-frequency
mottled ash centre and a dark scorched outer rim. The same ten low pieces are spread slightly
wider and enlarged enough to remain readable between the burning logs: four retain a warm
ember material, while six use near-black charcoal with sparse pulsing orange fissures. This
adds one material tag but no vertices, buffers, textures, lights or draw calls; the solid/effect
budgets remain exactly1992/42 triangles in two draws.

Native llvmpipe `20261013T214043.021426Z-campfire` passes all four animation stages with365
colour bins,39.05 luminance deviation,172.00 spatial contrast and empty kernel-fault evidence.
The close crop and four-stage montage were inspected: the ash is now distinct from the soil and
the coal bed no longer reads as ten identical red stones. Chrome/SwiftShader evidence
`20261013T214342.603885Z-chrome-campfire` passes two different images,60 finite intervals,
GL0, intact context and empty live/final kernel reports; the final image was inspected. Release
core/campfire tests and88 focused Python gate tests pass. The deterministic world fingerprint is
`0ecd98ad6f56b15b`.

## Dry-beach relocation and visual audit — 2026-10-14 (superseded)

This historical pass moved the fire from the inland eastern clearing to the broad southern
beach. The 2026-10-17 user request below supersedes that placement.
The wide llvmpipe evidence `20261014T003928.816157Z-campfire-wide` was inspected: shoreline,
shallow water, fire ring and separate wood stack are simultaneously visible on sand. It passes
at1280x720 with412 colour bins,18.46 luminance deviation and125.23 spatial contrast.
The rebuilt world report records the new centres and fingerprint `a33aa6c2913b5796`.

## Player-start corridor correction — 2026-10-15

The now-superseded beach relocation put the stored-wood collision proxy only about 1.73 m ahead of the historical
player start `(0,-225)`. Forward motion therefore resolved correctly but was rejected by the solid
woodpile. The player still starts four metres to the east at `(4,-225)` for compatibility. The
named spawn coordinates are shared by game initialization and placement tests.
`CampfireTests` rejects any future fire or wood proxy that overlaps the spawn capsule or its first
forward metre.

## Inland-clearing relocation — 2026-10-17

The fire and stored wood now occupy the east clearing at least55m inside the shoreline. Both
positions must be level interior/coastal ground; all geometry, collision, lighting and positional
audio follow the shared `CampfireSite`. The resident's deterministic route rebuilds around both
collision proxies. Native focused captures `20261017T123726.385229Z-campfire` and
`20261017T123801.490843Z-campfire-wide` pass and were inspected; the wider view establishes that
the landmark is surrounded by inland grass, animals and jungle rather than beach. Campfire and
resident tests pass, and WorldReport validates fingerprint `c4be22fb2b05ce89`.
