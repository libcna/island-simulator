# Known Limitations

This document describes the current build, not the intended final quality. `PLAN.md` is the
authoritative work ledger. Confirmed CNA or Sharp Runtime defects belong in `BUGS.md`; none have
been confirmed so far.

## Environment and art

- The island has a strong terrain/ocean/palm/grass foundation and a first layered broadleaf jungle,
  but the jungle is not final. A first low-bush layer plus rocks, fallen logs, and sparse driftwood
  exist, and a small rosette/forest-floor layer improves close ground contact. Bush and ground-plant
  silhouette/material diversity, broader shell/branch/beach-prop variety, distant scenery, wet
  shoreline detail, and final material variation remain limited or absent.
- Terrain colour is procedural vertex colour. It has no authored texture blend, fine sand/soil
  detail, or baked ambient/contact lighting.
- Palms, grass, canopy trees, understory, low bushes, and forest-floor plants have deterministic variation, wind,
  culling, and LOD, but visible transition popping, close-up material simplicity, and weak
  grounding/contact cues remain.
- A first portable permanent-daylight sky now supplies a tropical gradient, sun disc/halo, and
  fixed high cloud suggestion. Terrain, ocean, and sky share one sun direction. Final fill light,
  shadows, exposure, richer cloud shaping, and distant atmospheric composition are not complete.

## Traversal and interaction

- Walking, slopes, palm/broadleaf trunk collision, swimming, and the offshore current are
  functional. Collision has no spatial broad phase yet and checks every trunk proxy.
- Substantial rocks and fallen logs have conservative collision; smaller rock dressing remains
  non-solid. There is no geometric step-up, jump, camera bob, underwater presentation, or
  visual water-entry feedback; restrained audio entry/exit accents are implemented.
- Desktop mouse sensitivity and mobile touch sensitivity are compile-time values. There is no
  settings UI, remapping, gamepad control scheme, or handed touch layout yet.
- The game intentionally has no objectives, quests, combat, inventory, crafting, survival meters,
  score, progression, day/night cycle, or dynamic weather.

## Wildlife and audio

- A first 24-bird procedural flock has varied shore/canopy/high paths, animated wing strokes,
  culling, terrain clearance, and distance-scaled simulation. It still lacks calls, perching,
  landing, and long subjective behaviour tuning. Eight shallow-water schools add 48 fish with
  depth/shore constraints, animated tails, culling, and distance-scaled simulation; their geometry,
  schooling variation, water integration, and long subjective tuning remain first-pass quality. A
  first non-hostile shark alternates occasional offshore patrols with longer absences, stays out of
  the shallows, and has body/tail animation plus distance-scaled simulation; appearance timing and
  underwater integration still need subjective tuning. Seven first-pass procedural monkeys use a
  real fourteen-bone CNA GPU-skinning path and idle/observe/walk/run animation driven by a compact
  autonomous state machine. They share the player's trunk, solid-rock, and fallen-log proxies and
  use short lateral deflections before abandoning blocked targets. Idle/observe/walk/run changes
  now use short local-pose crossfades and preserve a continuous walk phase. They remain
  intentionally simple: tightly cluttered pockets can cause an early target reselection, and
  three silhouettes/four palettes cannot hide all procedural repetition; detailed surface
  treatment and long subjective behaviour tuning remain limited.
- Original procedural layers supply shoreline-distance-blended surf, sparse stronger-wave accents,
  light canopy breeze, subtle daytime-jungle texture, and speed/depth-responsive player movement
  through water. A single bounded positional voice now places two bird phrases and two restrained
  monkey contact phrases at actual nearby agents. The soundscape remains incomplete: richer jungle
  variation, additional wave/water/wildlife phrases, and a long fatigue/listening pass remain
  absent; call balance has only numerical and short native runtime validation so far.

## Platform qualification

- Linux/OpenGL 3.3 is the primary playable path and has repeated real-GPU and isolated capture
  coverage. Release packaging and extended whole-island sessions remain open.
- WebGL 2 runs in Chromium with automated rendering, pointer-lock, keyboard/mouse, overlay, resize,
  and first-gesture audio-mixer/track initialization coverage. Audible hardware-browser quality,
  genuine tab suspend/resume, fullscreen, and a second browser engine remain unverified.
- Android arm64-v8a and x86_64 APKs build, and an API 35 x86_64 AVD rendered one GLES3 scene. The
  host lacks KVM and the software-emulated System UI is unstable, so touch feel, lifecycle, audio,
  performance, thermal behaviour, and physical-device compatibility remain unverified.

## Development tooling

- The overlay reports frame timing, player/water state, renderer identity, terrain/vegetation
  visibility, LOD populations, draw counts, bird/fish/shark/monkey render and simulation tiers,
  monkey behaviour counts, current
  surf/breeze/jungle/player-water levels, played
  wave/splash counts, and mute/availability state. It does not yet provide world-space collision,
  shoreline/current, cell, wildlife, or audio-emitter visualization.
- Seven direct debug teleports exist, but there is no teleport menu, benchmark route, asset
  validation command, or automated renderer-image comparison yet.
