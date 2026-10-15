# Inland deer

Four deterministic peaceful feral deer occupy the east and west inland clearings. Their placement
requires dry ground, more than 40 m of shore clearance, moderate slope and clearance from the
shared static-world obstacle set. Each remains in a 13--15 m home. The bounded state machine
alternates grazing, resting and walking; a player within 9 m causes a non-aggressive retreat. The
existing full/reduced/dormant wildlife activation policy limits distant simulation.

The portable procedural mesh uses a narrower layered torso with raised shoulder and rump masses,
a tapered elevated neck, elongated head and muzzle, pointed ears, eyes, pale tail and underside,
restrained flank spots and articulated three-section legs with compact hooves. One adult has a
branched antler silhouette. Near geometry increases roundness and detail while distant animals use
a reduced form. Every visible animal appends to one dynamic vertex buffer and one opaque draw.

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
