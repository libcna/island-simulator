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
