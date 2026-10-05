# Known Limitations

This document describes the current build, not the intended final quality. `PLAN.md` is the
authoritative work ledger. Confirmed CNA or Sharp Runtime defects belong in `BUGS.md`; none have
been confirmed so far.

## Environment and art

- The island has a strong terrain/ocean/palm/grass foundation and a first layered broadleaf jungle,
  but the jungle is not final. The first rocks, fallen logs, and sparse driftwood exist; bush
  diversity, smaller ground plants, broader shell/branch/beach-prop variety, distant scenery, wet
  shoreline detail, and final material variation remain absent.
- Terrain colour is procedural vertex colour. It has no authored texture blend, fine sand/soil
  detail, or baked ambient/contact lighting.
- Palms, grass, canopy trees, and understory have deterministic variation, wind, culling, and LOD,
  but visible transition popping, close-up material simplicity, and weak grounding/contact cues
  remain.
- A first portable permanent-daylight sky now supplies a tropical gradient, sun disc/halo, and
  fixed high cloud suggestion. Terrain, ocean, and sky share one sun direction. Final fill light,
  shadows, exposure, richer cloud shaping, and distant atmospheric composition are not complete.

## Traversal and interaction

- Walking, slopes, palm/broadleaf trunk collision, swimming, and the offshore current are
  functional. Collision has no spatial broad phase yet and checks every trunk proxy.
- Substantial rocks and fallen logs have conservative collision; smaller rock dressing remains
  non-solid. There is no geometric step-up, jump, camera bob, underwater presentation, or
  water-entry feedback.
- Desktop mouse sensitivity and mobile touch sensitivity are compile-time values. There is no
  settings UI, remapping, gamepad control scheme, or handed touch layout yet.
- The game intentionally has no objectives, quests, combat, inventory, crafting, survival meters,
  score, progression, day/night cycle, or dynamic weather.

## Wildlife and audio

- Birds, fish, the occasional shark, monkeys, skeletal animation, wildlife behaviour, and wildlife
  spatial activation are not implemented yet.
- Ocean surf, jungle ambience, wind, insects, birds, monkeys, positional emitters, and player water
  sounds are not implemented. The current build is acoustically incomplete.

## Platform qualification

- Linux/OpenGL 3.3 is the primary playable path and has repeated real-GPU and isolated capture
  coverage. Release packaging and extended whole-island sessions remain open.
- WebGL 2 runs in Chromium with automated rendering, pointer-lock, keyboard/mouse, overlay, and
  resize coverage. Visible hardware-GPU play, audio unlock, genuine tab suspend/resume, fullscreen,
  and a second browser engine remain unverified.
- Android arm64-v8a and x86_64 APKs build, and an API 35 x86_64 AVD rendered one GLES3 scene. The
  host lacks KVM and the software-emulated System UI is unstable, so touch feel, lifecycle, audio,
  performance, thermal behaviour, and physical-device compatibility remain unverified.

## Development tooling

- The overlay reports frame timing, player/water state, renderer identity, terrain/vegetation
  visibility, LOD populations, and draw counts. It does not yet provide world-space collision,
  shoreline/current, cell, wildlife, or audio visualization.
- Four direct debug teleports exist, but there is no teleport menu, benchmark route, asset
  validation command, or automated renderer-image comparison yet.
