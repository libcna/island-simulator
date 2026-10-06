# Known Limitations

This document describes the current build, not the intended final quality. `PLAN.md` is the
authoritative work ledger. Confirmed CNA or Sharp Runtime defects belong in `BUGS.md`; none have
been confirmed so far.

## Environment and art

- The island has a strong terrain/ocean/palm/grass foundation and a first layered broadleaf jungle,
  but the jungle is not final. A first low-bush layer plus rocks, fallen logs, and sparse driftwood
  exist, while small rosette/forest-floor plants, grove-associated fallen palm fronds, and
  near-field shell/pebble clusters improve close ground contact. Near broadleaf crowns now have
  separated irregular lobes, visible support branches, pointed edge leaves, and staggered per-tree
  LOD into a rounded far mesh. Near bushes now layer paired folded pointed leaves over smaller
  lobes while the far tier stays compact. Near forest-floor variants now include an upright
  broadleaf form with visible petioles and a separate five-leaf folded split form. All jungle
  tiers now choose LOD and maximum distance from each instance after coarse cell rejection. The
  third near driftwood variant adds a curved three-limb fork instead of another simple branch stub,
  and rocks, logs, and driftwood now also choose range/LOD per instance. Bush
  material diversity, additional ground-plant material diversity, broader shell/branch/beach-prop
  variety, richer mid-distance scenery, wet
  shoreline detail, and final material richness remain limited.
- Terrain combines procedural vertex colour with a texture-free sand/soil, wet-edge, exposed-slope,
  close substrate-microvariation, sparse pointed leaf litter, and restrained fixed-sun interior
  dappling. The authored main corridor now has a narrower, noise-mottled broken-edge dry-earth
  footpath under its short grass while retaining a wider vegetation-free walking corridor. The two
  clearings have distinct warm-dry and darker leaf-rich treatments, and their
  outer rings now carry a first five-rock/one-log natural-landmark composition without blocking
  their centres. Broader unique composition and bespoke landmark geometry remain limited. The
  dapple is static material breakup rather
  than canopy-accurate shadow projection. A
  portable near-field contact layer now grounds palms, canopy trunks, land rocks, wood, and monkeys,
  and all instanced jungle foliage now receives two-sided fixed-sun geometric facet lighting, but
  there is still no authored biome texture blend, decals, broad substrate diversity, baked ambient
  lighting, canopy-accurate occlusion, or full directional/self-shadowing.
- The ocean has depth colour, distance-faded crossing-wave subsurface variation, analytic
  waves/specular response, a coarse broad foam mask, and an exact shore-following animated
  crest/wash ribbon. Swimming now adds a deliberately subtle,
  depth-responsive turquoise edge veil that cools and strengthens with the offshore current, but
  full underwater fog, a geometric waterline, refraction-like cues, splash particles, and final
  wash/material tuning remain absent.
- Palms, grass, canopy trees, understory, low bushes, and forest-floor plants have deterministic
  calm wind, and nearby mesh grass now bends around the player's stationary body radius and expands
  with movement speed. Its curved near blades have directional two-sided daylight, root, fold, and
  restrained transmission cues. Near palms now use segmented tapered leaflets with widened ribbon
  far silhouettes and staggered per-instance LOD thresholds; near trunks have subtle procedural
  leaf-scar rings and two variants carry restrained faceted coconut clusters. Grass
  collision/displacement does not
  persist after the player passes, and true LOD blending, richer biome variation, aliasing work,
  and physical-device density tuning remain.
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
- Desktop mouse sensitivity, 4.3/8.0 m/s walk/run speeds, and mobile touch sensitivity are
  compile-time values. WASD and arrows are supported, but there is no settings UI, remapping,
  gamepad control scheme, or handed touch layout yet.
- The game intentionally has no objectives, quests, combat, inventory, crafting, survival meters,
  score, progression, day/night cycle, or dynamic weather.

## Wildlife and audio

- A first 24-bird procedural flock has varied shore/canopy/high paths, animated wing strokes,
  slow per-agent radial drift, banked turns, intermittent glides, positional calls, culling,
  terrain clearance, and distance-scaled simulation. It still lacks perching, landing, and long
  subjective behaviour tuning. Eight shallow-water schools add 48 fish with
  depth/shore constraints, depth-following pitch, banked turns, forked animated tails, near head
  detail, fixed-sun faceted body colour, culled single-surface fins, culling, and distance-scaled
  simulation; their schooling variation, water integration, and long subjective tuning remain
  first-pass quality. A
  first non-hostile shark alternates occasional offshore patrols with longer absences, stays out of
  the shallows, and has dive pitch, banked turns, body/forked-tail animation, near head detail, and
  distance-scaled simulation; appearance timing and
  underwater integration still need subjective tuning. Seven first-pass procedural monkeys use a
  real fourteen-bone CNA GPU-skinning path and idle/observe/walk/run animation driven by a compact
  autonomous state machine. They share the player's trunk, solid-rock, and fallen-log proxies and
  use short lateral deflections before abandoning blocked targets. Idle/observe/walk/run changes
  now use short local-pose crossfades and preserve a continuous walk phase. The near tier has a
  readable light face mask, protruding eyes and nose, restrained eye gloss, and brighter fur/face
  palettes without another draw call. Calm animals now add a bounded, distance-faded head turn
  toward a nearby player using the existing skeleton and draw count. They remain intentionally
  simple: tightly cluttered pockets
  can cause an early target reselection, and three silhouettes/four palettes cannot hide all
  procedural repetition; detailed surface treatment and long subjective behaviour tuning remain
  limited.
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
- Android arm64-v8a and x86_64 APKs build, and a KVM-accelerated API 35 x86_64 AVD renders the
  complete GLES3 scene. Independent movement/look gestures, translucent touch controls, a
  same-process hot resume, and AAudio pause/resume are validated. Simultaneous real-finger
  ergonomics, audio-focus interruption, process-death recovery, physical mobile-GPU performance,
  thermal behaviour, and physical-device compatibility remain unverified.

## Development tooling

- The overlay reports frame timing, player/water state, renderer identity, terrain/vegetation
  visibility, LOD populations, draw counts, bird/fish/shark/monkey render and simulation tiers,
  monkey behaviour counts, current
  surf/breeze/jungle/player-water levels, played
  wave/splash counts, mute/availability state, and live source use against the six-source budget.
  F2 adds world-space shoreline/current, collision, exact attempted/resolved player motion,
  shared-cell, wildlife heading/target/home-range, ambient-zone, nearest audio-candidate, active
  positional-emitter, exact nine-sample terrain-contact, and complete bird route-family/spawn views;
  movement history, steering proposals, and deeper per-system culling/LOD overlays remain open.
- Eight direct debug teleports, a bounded debug-only inspection camera, a five-view native
  fixed-frame suite plus a 676 m moving benchmark, an asset-manifest validator, and a deterministic
  world-statistics report exist. The native overview has automated P6 integrity and broad
  non-degeneracy checks, but there is no teleport menu, pixel-golden comparison, or automated
  native/Web/Android image comparison yet. The moving benchmark has no Web/Android timing baseline,
  and the world fingerprint is a same-toolchain change detector rather than a cross-libm canonical
  world hash.
