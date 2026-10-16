# Inland deer

Four deterministic peaceful feral deer occupy the east and west inland clearings. Their placement
requires dry ground, more than 40 m of shore clearance, moderate slope and clearance from the
shared static-world obstacle set. Each remains in a 13--15 m home. The bounded state machine
alternates grazing, resting and walking; a player within 9 m causes a non-aggressive retreat. The
existing full/reduced/dormant wildlife activation policy limits distant simulation.

The portable procedural mesh uses one continuous eight-section torso, a shoulder-grown tapered
neck, compact head and muzzle, leaf-shaped ears, readable eyes, pale tail and underside, restrained
staggered flank spots and articulated three-section legs with split hooves. One adult has a curved
multi-tine antler silhouette. Near geometry increases roundness and detail while distant animals
use a reduced form. Every visible animal appends to one dynamic vertex buffer and one opaque draw.

Deterministic CPU coverage checks the exact population, safe habitat, repeatable simulation,
walking, grazing, bounded homes and player-triggered retreat. The accepted native Release
llvmpipe image is
`build-consumer/software-captures/20261015T080715.879463Z-deer`: 224 colour bins, 33.98 luminance
deviation and 145.98 spatial contrast. The original-resolution frame was visually inspected after
an earlier barrel-bodied, straight-legged draft was rejected and revised.

Chrome/SwiftShader evidence is
`build/browser-software/20261015T081449.186060Z-chrome-deer`: 60 finite positive CPU-rendered
intervals, GL0, a live context, clean kernel monitoring and a matching inspected image. ARM64
assemble/lint passes without launching an emulator or touching KVM; the APK is 24,413,771 bytes
with SHA-256 `6c56d7ec6bb56e0eac8f21d5d2e13cc375b4d201b1936d92c28723afef525a4f`, and the stripped
`libmain.so` is 19,371,200 bytes. Physical-device appearance remains part of final acceptance.

## Continuous torso and species silhouette — 2026-10-16

The current close animal matrix compared deer, geckos and rats and selected the deer. Although its
previous parts were individually rounded, three overlapping barrel/chest/haunch ellipsoids still
read as a blocky assembled figurine. Its large upright neck and tiny face suggested a llama, the
ears collapsed into pale triangles behind the antlers, and the single-piece hooves remained blunt.

One closed eight-section body loft now establishes the rump, waist, withers and chest as a single
surface. It uses twenty radial samples near the camera and nine in the far tier, analytic
normal-driven daylight and restrained deterministic coat variation. The neck begins inside the
raised shoulder and tapers through five existing animated stations. The face is shorter and more
compact; ears have an outer leaf and inset inner plane; eyes gain small fitted catchlights. Fuller
upper/lower legs terminate in two separated toes, and the antlered adult uses a curved three-part
beam with three progressively tapered tines per side. Seven smaller spots form two staggered rows
and derive their lateral position from the new body cross-section, so they remain surface-fitted.

The connected body is cheaper than the three overlapping volumes. Even with the additional facial,
hoof and antler detail, the conservative shared staging allocation is reduced from 28,000 to
26,000 vertices. Four agents, one draw, habitat, state machine, grazing, retreat, contact and the
48 m near threshold are unchanged. Inspected native evidence
`build-consumer/software-captures/20261016T143955.877693Z-deer` passes at 236 colour bins, 33.58
luminance deviation, 135.73 spatial contrast and kernel `[]`; baseline is
`20261016T142214.520131Z-deer`. `IslandCoreTests`, `IslandDeerHerdTests` and all 42 browser/capture
harness tests pass. Focused Chrome/SwiftShader
`build/browser-software/20261016T144210.079030Z-chrome-deer` passes 60 finite intervals, GL0, a
live context and kernel `[]`. WebAssembly builds. Android ARM64 assemble/lint passes without
emulator/KVM; the APK is 24,680,923 bytes and packaged `libmain.so` is 19,638,352 bytes.

## Separated body masses, upper-leg muscles and coat tone — 2026-10-15

The renewed large-animal contact sheet selected the deer as the weakest remaining broadside: the
long central ellipsoid hid the existing shoulder and rump, upper legs narrowed immediately into
sticks, and uniform vertex colour flattened the coat. The central loin is now shorter and
shallower, exposing a raised chest, a rounded haunch and a visible waist while retaining overlap.
Each upper leg uses one continuous four-ring muscle profile from its embedded root to the existing
animated knee instead of a single rapidly tapering tube. Hooves, hocks, phase, terrain contact and
behavior are unchanged.

The same CPU vertex-colour path now derives restrained coat tone from each analytic normal: the
dorsal surface trends darker and the underside lighter across body, shoulder, rump, neck and head.
This adds no texture, shader, material or draw. The muscle profiles add 432 vertices per near deer;
the conservative four-animal staging ceiling grows from 23,200 to 28,000 vertices (76,800 bytes at
the current 16-byte vertex format), still uploaded once and drawn once. Far animals receive the
same body proportions and cheaper profiled legs.

The first proportion-only candidate `20261015T211715.963205Z-deer` and intermediate leg-profile
candidate `20261015T212002.268480Z-deer` passed but were not retained as final evidence. Final
current-binary native llvmpipe `20261015T212646.918760Z-deer` passes at 1280x720 with 232 colour
bins, 33.62 luminance deviation, 135.47 spatial contrast and an empty kernel delta. Direct
original-resolution comparison with `20261015T184054.192941Z-deer` was inspected. Focused
Chrome/SwiftShader `20261015T212930.471582Z-chrome-deer` passes 60 finite positive intervals,
`glError=0`, a live context and an empty kernel delta; its original PNG was inspected. Core and
deer-herd tests pass. Current Android runtime evidence remains open.

## Continuous neck, separate nose and fitted spots — 2026-10-15

A later twelve-family audit retained the deer's proportions and behavior but found three close-view
construction cues. Two independently framed straight tubes left a collar around the middle of the
neck; the entire muzzle was one oversized dark ellipsoid; and the five markings on each flank were
double-sided triangular cards outside the body surface.

The animated base, mid-neck and top targets now feed one five-ring loft. Its cross-section begins
inside the shoulder mass, follows the grazing bend and narrows continuously into the head. The
furred muzzle uses a restrained coat-to-dark blend and carries a separate compact nasal ellipsoid at
its tip. Each marking is now a small six-sided oval. Its lateral centre is derived from the maximum
surface width of the actual overlapping barrel, shoulder and rump ellipsoids at that longitudinal
and vertical coordinate, so it hugs the current coat rather than floating at a constant width.

Population, placement, state machine, gait targets, antlers, near/far threshold, buffer and one-draw
submission remain unchanged. `IslandDeerHerdTests` and `IslandCoreTests` pass. Baseline
`20261015T124054.189699Z-deer` and accepted current-binary native evidence
`20261015T184054.192941Z-deer` were inspected side by side. The latter passes at 1280x720 with 235
colour bins, 33.63 luminance deviation, 135.46 spatial contrast and an empty kernel delta. Current
native, Chrome and Web-smoke gates require the complete neck/nose/spot diagnostic; refreshed Web and
physical-Android images remain part of consolidated portable qualification.
