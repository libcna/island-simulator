# Coconut palm groves

2026-10-10, from the macro-geography analysis item 9: 330 palms were spread as a near-uniform
scatter (178 solitary, one with three neighbours inside 12 m) and 134 stood more than 60 m
inland, so the coast had no groves and the forest was salted with palms.

## What changed (`VegetationLayout.cpp`)

- Candidates every 8 m (grid radius 33). A contrast-shaped two-octave grove field (62 m and
  21 m value noise, `smoothstep` around 0.42-0.64) times a coastal weight that is full within
  32 m of the shore and fades over the next 70 m: `density = 0.006 + coastal * (0.012 + 0.36
  grove)`. A tiny floor keeps a few old solitary palms inland.
- Young palms: inside groves (`grove >= 0.45`) an adult recruits a juvenile with 32 % chance,
  2.6-4.4 m away, scale 0.42-0.64, at least 2.2 m from any other palm, on valid coastal ground
  and off the reserved routes. Juveniles get their own collision proxy (`0.31 x scale`).
- Current result after the later terrain/ecology passes: 327 palms. Six established adults are
  deterministically storm-fallen and eight are broken; the total population does not increase.
  The wind lean from `docs/prevailing-wind.md` applies to the remaining upright palms.
- Fallen candidates lie only on stable dry ground, remain clear of the authored route and retain
  a full living crown. Five additional round collision samples follow each visible horizontal
  stem, so it cannot be walked through. Broken palms use dedicated near/far meshes: tapered
  4--6 m boles with pale, uneven splinter caps instead of clipped versions of the living mesh.
  Juveniles are never selected for damage.
- Tests require >= 40 % grouped, <= 5 % beyond 90 m, 30+ young palms at <= 35 % of the total,
  juvenile scale/collision ranges and 2.2 m trunk clearance. They also pin six fallen and eight
  broken adults, the complete fallen-stem terrain/route envelope and five extra collision samples
  per horizontal log; Web/Firefox/Android harness pins expect the current 327 palms.

## Evidence and limitations

Native llvmpipe `build/software-captures/20261010T174808.840978Z-overview`,
`174754.658042Z-palms`, `174801.794546Z-beach` (pass, inspected): the coast reads as groves with
gaps and the interior as broadleaf forest. The completed storm-history pass is inspected in
`build-consumer/software-captures/20261014T222248.112957Z-fallen-palm` and
`20261014T220547.163070Z-broken-palm`. The fallen view shows a full grounded stem and crown across
the beach-backed grove; the broken view shows several distinct stump heights with readable pale
fractures. The render transform compensates its local fall rotation, so the visible stem, five collision
samples and elongated terrain contact shadow follow the same world-space axis; sampled stem and
crown clear every standing palm trunk. Chrome/SwiftShader evidence
`build/browser-software/20261014T222441.379093Z-chrome-palms` matches native in all three palm
views, each with 60 positive finite intervals, GL error zero, live context and empty kernel delta.
The valid world fingerprint is `766a63088483e673`. ARM64 assemble/lint passes with the locked
dependency checkout through the two-worker wrapper; the APK is 24,024,963 bytes with SHA-256
`c67f6894397ab76b8522fe93fa90c1143258feb92fe6ec6b15a5cb7d8149640b`. No emulator or KVM was
started. Earlier Android `palms`, `jungle` and `east` pass
(`build/android-software/20261010T175715.080427Z-palms`, `175828.715762Z-jungle`,
`175941.257408Z-east`); the full Web suite result is recorded in `PLAN.md`.
