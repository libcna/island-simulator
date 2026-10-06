# Known Limitations

This document describes the current build, not the intended final quality. `PLAN.md` is the
authoritative work ledger. Confirmed CNA or Sharp Runtime defects belong in `BUGS.md`; none have
been confirmed so far.

## Environment and art

- The island has a strong terrain/ocean/palm/grass foundation and a first layered broadleaf jungle,
  but the jungle is not final. A first low-bush layer plus rocks, fallen logs, and sparse driftwood
  exist, while small rosette/forest-floor plants, grove-associated fallen palm fronds, and
  near-field shell/pebble clusters improve close ground contact. Bush and ground-plant
  silhouette/material diversity, broader shell/branch/beach-prop variety, richer mid-distance scenery, wet
  shoreline detail, and final material richness remain limited.
- Terrain combines procedural vertex colour with a texture-free sand/soil, wet-edge, exposed-slope,
  close substrate-microvariation, and sparse pointed leaf-litter material shader. A portable
  near-field contact layer now grounds palms, canopy trunks, land rocks, wood, and monkeys, but
  there is still no authored biome texture blend, decals, broad substrate diversity, baked ambient
  lighting, or full directional/self-shadowing.
- The ocean has depth colour, analytic waves/specular response, a coarse broad foam mask, and an
  exact shore-following animated crest/wash ribbon. Swimming now adds a deliberately subtle,
  depth-responsive turquoise edge veil that cools and strengthens with the offshore current, but
  full underwater fog, a geometric waterline, refraction-like cues, splash particles, and final
  wash/material tuning remain absent.
- Palms, grass, canopy trees, understory, low bushes, and forest-floor plants have deterministic
  calm wind, and nearby mesh grass now bends around the player's stationary body radius and expands
  with movement speed. Its curved near blades have directional two-sided daylight, root, fold, and
  restrained transmission cues. Grass collision/displacement does not persist after the player
  passes, and LOD blending, richer biome variation, aliasing work, and physical-device density
  tuning remain.
  Contact shadows improve the main trunk/prop/animal classes, but small foliage is intentionally
  unshadowed; visible transition popping and close-up material simplicity remain.
- A portable permanent-daylight sky now supplies a tropical gradient, sun disc/halo, and sparse
  layered high wisps shaped horizontally instead of the earlier conspicuous vertical streaks.
  Terrain, ocean, sky, and contact-shadow projection share one sun
  direction. Five unreachable
  low islets with 11 simplified palm silhouettes now break up the horizon through one fogged
  portable batch. Final fill light, shadows, exposure, unified world haze, and more varied distant
  atmospheric composition are not complete.

## Traversal and interaction

- Walking, slopes, palm/broadleaf trunk collision, swimming, and the offshore current are
  functional. Static palm, broadleaf, land/reef-rock, and log proxies use a 16 m spatial broad
  phase in both locomotion modes, but dynamic obstacles and depenetration are not implemented.
- Substantial rocks and fallen logs have conservative collision; smaller rock dressing remains
  non-solid. Fourteen emergent reef outcrops remain solid while swimming. Walking and swimming now
  use very mild speed-responsive positional camera motion,
  which can be smoothly disabled with B. There is no geometric step-up, jump, or complete
  underwater presentation; restrained visual near-surface/current and audio entry/exit feedback
  are implemented. Movement in water now produces restrained shader-integrated expanding ripple
  bands; entry splash particles and body/shore displacement remain absent. The outer-ocean current
  also deepens the existing player-water loop into a restrained undertow cue; subjective
  headphone/speaker tuning remains open.
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
  wave/splash counts, and mute/availability state. F2 adds the first world-space shoreline/current,
  collision, shared-cell, and wildlife view; audio-emitter visualization and deeper per-system
  overlays remain open.
- Eight direct debug teleports and a five-view native frame-time suite exist, but there is no
  teleport menu, sustained moving benchmark route, asset validation command, or automated
  renderer-image comparison yet.
