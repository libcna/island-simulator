# Eastern clearing campfire — native first pass

One continuously burning fire is placed at `(46,24.7219,-16)` in the existing eastern
clearing. A separate stack of11 gathered logs is at `(51.5,groundY,-18.5)`, about6.04m
away. Twelve irregular stones surround five burning logs, an ash bed and ten embers.
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
837900 PCM bytes; the audio inventory is26 clips/5153084 bytes. No new playing voice is
added: fire, stream and player-water playback share the fourth persistent slot. This
clearing lies beyond the stream's audible region. Swimming takes priority, and fire
audibility uses a bounded3D24m distance with smoothed gain and transition hysteresis.
Spatial mode is configured before the first Play, preserving the prior dropout fix.

Real-CNA dummy-audio tests cover far/near startup, repeated fire→stream→far→swim→high
flight→fire transitions and mute/unmute, querying actual Playing state and the4–6 source
budget. This establishes backend operation, not speaker listening or perceived naturalness.
The independent WAV exporter also checks all26 clips and their complete PCM footprint.

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

Remaining: live keyboard/audio-device encounter, listening review, smoother natural
material/smoke refinement, representative performance and consolidated Web/Android runtime.
Both native executable build trees are refreshed. The separate private F11 restoration
finding remains open. No AMD, Android emulator or
KVM setting was changed for this work; all builds are restricted to CPUs0–3 and at most4 workers.
