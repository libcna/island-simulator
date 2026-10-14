# Known Limitations

- Close forest-floor image detail is integrated with portable mip/gradient filtering, not a final
  photorealistic material. Grazing views remain soft, overlapping windows can ghost leaf fragments,
  and source crevices contain baked-looking shading. Four samples per covered terrain fragment
  and startup PNG decoding add cost; physical-GPU/mobile performance is unqualified. The source
  and new `.data` Web payload are larger than a future offline-prepared 512-square variant.
  See `forest-floor-material-review.md` for measured software evidence and exact memory scope.

This document describes the current build, not the intended final quality. `PLAN.md` is the
authoritative work ledger. Confirmed CNA or Sharp Runtime defects belong in `BUGS.md`; none have
been confirmed so far.

## Current browser input evidence

Current browser input evidence is described in `browser-input-ownership.md`. A game-side
Web short-click capture mismatch was reproduced and corrected; the contained Firefox full
gate passes using llvmpipe on owned Xvfb. The older Chrome input flake lacks enough event
evidence to assign it the same cause. None of these software tests requalifies AMD hardware
or resolves the GPUVM incident. Broader historical platform claims below retain their dates.

## Current Android software evidence

The isolated API 35 x86_64 SwiftShader path now runs current sources without exposing host GPU
devices. The interactive selector/Home-resume/cold-relaunch gate passes with inspected frames,
and the current clearing-edge image is unobstructed and retains the new leaf/terrain/grass layers.
See `build-android.md` for exact evidence. This does not qualify AMD hardware, physical-mobile
speed/thermals, audible output, or all close wildlife/material views. A separate 1280x720 software
touch route now also passes land-footstep, swimming-state and water-accent gates with an inspected
near-water frame. Right-zone look, simultaneous real-finger controls and physical-device layouts
remain separate requirements; normalized-coordinate unit tests are not multi-device runtime QA.
The AMD incident remains unresolved. Headless software frame times are not phone benchmarks.

## Environment and art

- Juvenile ground broadleaf variant 1 now has a curved tagged lamina and four retained
  source leaves at far LOD. Its dedicated `ground-broadleaf` view avoids confusing it with
  nearby bushes. Far geometry still drops two leaves. See `ground-broadleaf.md` for cost
  and actual image scope.

- Split ground variant 2 now has connected, rounded lobes and matching retained far
  identities. Its local above-origin bounds are not per-vertex terrain conformance;
  sloping ground can still intersect leaves. Far geometry is coarser and drops one
  leaf; no invisible LOD transition is claimed. See `split-ground-leaves.md` for mesh
  cost and matched-camera evidence.

- Shrubs replace solid faceted volumes with supported shoots and individual leaves.
  Far retains source identity and the complete leaf outline but removes the inner
  rib; shading/interior projection and shoot detail can still change. The procedural shoot pattern and rest-shape wind normals need
  further visual review. Triangle cost is higher despite compact nondegenerate
  topology; physical GPU performance is unqualified. See `shrub-lod-continuity.md`.

- Custom grass now contracts per anchor over the final 16 m and uses conservative cell
  distance rejection; this removes the old whole-cell distance cutoff before nearby anchors
  had reached their limit. It is a geometric shrink, not an opacity fade or smooth mesh morph.
  Extra far-instance submissions, open-sightline motion quality and discrete mesh-tier changes
  remain separate concerns. BasicEffect retains its degraded cutoff. See
  `grass-distance-continuity.md` for measured coverage and validation scope.

- Bounded MSAA improves polygon edges (4x Linux/Web, 2x Android request) but does not filter
  shader-detail shimmer, stabilize visibility/LOD changes or add geometric detail. Applied
  counts are reported; unsupported devices may downgrade. Extra framebuffer memory, resolve
  bandwidth, high-DPI cost and physical-mobile thermals still need hardware measurements.

- Grass surface colour now separates baked lighting from pigment and adds filtered
  longitudinal detail. This is restrained material variation, not a solution to sparse
  coverage, repeated tufts or approximate wind-deformed normals. Software still frames
  do not prove temporal stability or hardware performance. See `grass-surface-material.md`.
  See `edge-antialiasing.md` for software evidence and nominal memory estimates.

- Boars now fit individual sole centres to rendered terrain with fixed-length limbs and explicit
  unreachable-target diagnostics. This is not world-space stance locking or stride-speed
  synchronization. Toe-surface orientation on uneven patches and walking/turning slip still need
  close visual review; CPU contact accuracy does not establish natural-looking animation.

- Tagged leaves now have stronger directional daylight contrast and restrained distance-stable
  aged tips. Mottling and bark grain/fissures have footprint filtering as well as distance fades.
  This does not supply physical subsurface scattering or new cast shadows. Untagged crown volumes
  retain the previous lighting. Custom canopy wood now separates albedo from fallback-only baked
  colours, validates outward normals and uses one-sided fixed daylight. Crown self-shadowing,
  cast branch shadows and wind-deformed normals still need work; smoother lighting alone does
  not supply botanical detail or remove repeated tree silhouettes.
  Bark now uses part-local circular/axial coordinates with seam-safe footprint filtering instead
  of a shared plant-origin angle on trunks, branches and roots. Instance-stable periodic warping,
  interrupted fissures and plate-tone variation reduce regular stripes, but the procedural
  character remains visible. Junction phases are not continuous and height only approximates
  trunk arc length; anatomical knots, bark relief and broader age/species variation remain
  unimplemented.
  Stock-effect fallback does not gain these custom leaf materials. Moving-camera aliasing and
  physical-mobile shader cost remain unqualified.

- Medium understory no longer uses equal-angle/equal-height umbrella forms: leaves have varied
  posture, emerging growth, narrowed petioles and restrained curl; far models retain actual near
  leaf identities. Two forms now have paired leaflets on a curved rachis, with exact root wind
  continuity and rachis-derived leaflet planes. The four shared meshes still repeat, use coarse
  leaflets, and lose leaves/curvature at far LOD. Profile-derived normals do not follow dynamic
  wind. Far ferns increase from 20 to 320 triangles per plant; physical-mobile cost and moving
  coverage/aliasing need qualification. See `understory-ferns.md` for the bounded visual pass.
  Broadleaf forms now redistribute their existing five columns over paddle/lance laminae with
  restrained asymmetric edges and a rounded cross section. Some still look oversized and retain
  simple rosette architecture; the unchanged coarse far mesh cannot preserve every near margin.
  See `broadleaf-lamina.md` for the modest before/after result, not a completed realism claim.

- Small clearing-edge colonies now group 13 shrubs with 57 low plants, keeping complete animated
  footprints outside reserved paths/clearings. This is local composition, not an island-wide
  density or realism solution. Existing base plants still use point reservations; colonies avoid
  canopy trunks but not every palm, rock or log. The centre clearing view hides most changes;
  the fixed `clearing-edge` camera provides a closer review. Existing procedural silhouettes,
  sparse ground contact and limited species diversity remain conspicuous.

- Forest-floor recruitment adds 866 plants in 199 canopy-associated groups without
  changing grass, paths or clearing accents. The trail comparison is visually modest
  and partly occluded; a close off-path review remains needed before claiming natural
  density. Existing discrete LOD, slope intersection and foliage/prop overlaps remain.
  See `forest-floor-colonies.md`; physical GPU cost has not been measured.

- Jungle root origins now match rendered terrain triangles instead of the analytic
  collision field, removing measured origin gaps up to 12.24 cm. Broad roots and
  multiple shoot bases are not slope-draped; leaf/ground
  intersections can remain away from the origin. See `jungle-root-contact.md`.

- Near rosette litter now follows the rendered surface through the shared grass
  height texture, removing sampled slope penetrations while retaining its folds.
  It still repeats three coarse leaves, disappears at the near LOD boundary, and
  remains flat on the stock-effect fallback. See `fallen-leaf-contact.md`.

- Grass middle/far meshes now inherit the seven near tuft roots, profile identities and wind
  phases instead of unrelated arrangements. Widths preserve a shared upright-area approximation;
  this is not seamless LOD blending or equal coverage from every angle. Far meshes rise from
  five to seven blades (20 to 28 triangles); physical-GPU/mobile cost remains unqualified.
  Missing low turf in proxies, sparse nearby cover and distant undergrowth still need work.

- The island has a strong terrain/ocean/palm/grass foundation and a first layered broadleaf jungle,
  but the jungle is not final. A first low-bush layer plus rocks, fallen logs, and sparse driftwood
  exist, while small rosette/forest-floor plants, grove-associated fallen palm fronds, and
  near-field shell/pebble/dry-wrack clusters improve close ground contact. Near broadleaf crowns
  now have separated irregular lobes, visible support branches, pointed edge leaves, sparse leafy
  hanging lianas on two near variants, and staggered per-tree LOD into a rounded far mesh. Near
  bushes now use supported multi-shoot structures with individually curved leaves;
  far retains those identities with reduced tessellation. Near forest-floor variants include an upright
  broadleaf form with visible petioles and a separate five-leaf folded split form. All jungle
  tiers now choose LOD and maximum distance from each instance after coarse cell rejection. A
  stable green-only per-instance tint derived from the existing transform adds restrained colour
  variation across all four layers while retaining the same identity through near/far LOD. The
  fallen fronds now curve their rachis and apply restrained asymmetric sweep, droop, and ageing to
  their existing 28 leaflets. The third near driftwood variant adds a curved three-limb fork
  instead of another simple branch stub; near fallen-log end rings now form irregular non-planar
  broken contours instead of perfectly circular planar cuts without changing topology or the far
  LOD, and their end grain adds subtle radial drying cracks without a texture lookup; neutral
  stone tones now receive restrained transform-derived warm/cool per-instance
  variation with stable near/far identity; and three existing silhouettes form a first authored
  non-solid landmark in one cove. Rocks,
  logs, and driftwood now also choose range/LOD per instance. Bush
  material diversity, additional ground-plant material diversity, broader shell/wrack/branch/beach-prop
  variety, richer mid-distance scenery, wet
  shoreline detail, and final material richness remain limited.
- Canopy trees now have eight structurally distinct crown forms and shared near/far trunk bends,
  with the same seven primary support branches retained in both tiers. Four outward static
  inspection pairs show modestly reduced transition error, but some lower portions remain
  palm-occluded. Crown cores, removed leaves, trunk detail, roots and lianas still differ;
  this does not qualify moving-camera transitions or hardware cost.
  Trees still repeat eight procedural meshes; the additional forms cost shared buffers and
  variant draws, not extra trees. New-form static review does not qualify moving transitions.
  Canopy blades now have spatial pitch/roll and
  restrained variant-dependent proportions
  instead of uniformly horizontal axes. The same source identities/poses survive far selection.
  Canopy blades now have a seven-vertex curved outline instead of five-vertex diamonds, funded
  by fewer retained sprigs within the same triangle budget. Low-poly edges, large blade scale
  and simple branch hierarchy remain apparent; this does not add anatomical branching or exact
  per-leaf cast shadows. Mesh-measured culling
  bounds update with the poses, while the shared light-bake ellipsoids remain approximate.
  Far trees retain the same eight lobe anchors, core contours and a subset of actual near leaves.
  An earlier partly occluded, frozen-camera
  comparison reduced changed pixels from 222 to 181 and mean colour error by about 20%, not to
  zero. The newer posture comparison is mixed: 180→153 changed pixels but approximately 9.7%
  higher mean colour error; see `canopy-lod-review.md` for exact pairs and limitations.
  The subsequent curved-outline pair improves both metrics (153→139 changed pixels and about
  17.3% less mean colour error), still only on that partly occluded target. Equal triangle counts
  do not prove identical coverage or fragment cost after redistributing the leaves.
  Other trees/angles and moving-camera transitions need further review. The far tier rises from
  464 to 952 triangles; physical-mobile cost remains unqualified.
  Per-tree conservative frustum boxes now reject offscreen canopy after cell culling; four frozen
  native comparisons are byte-identical while submitting fewer trees. This does not remove
  trees hidden behind terrain/other foliage and does not qualify every edge trajectory or device.
  Crown self-shadowing and convincing bark/anatomical variety remain limited. Changed crown
  volumes update the static light field automatically; its coverage is not calibrated to real
  botanical optical density.
- Terrain combines procedural vertex colour with a texture-free sand/soil, darkened wet-edge with
  bounded view-dependent fixed-sun sheen, exposed-slope,
  close substrate-microvariation, warm humus/local moss and footprint-filtered pointed leaf
  litter with per-leaf colour/midrib shading, close dry-sand pebble/shell flecks,
  and fixed-sun interior canopy transmission. The authored main corridor now has a narrower,
  noise-mottled broken-edge dry-earth
  footpath under its short grass while retaining a wider vegetation-free walking corridor. The two
  clearings have distinct warm-dry and darker leaf-rich treatments plus broad irregular dry/cool
  and leaf/moss patch breakup from already-computed terrain noise, and their
  outer rings now carry a five-rock/two-log natural-landmark composition without blocking
  their centres. Fallen leaves are flat shader marks; they lack physical thickness, overlap
  shadows and decomposed organic clusters. Substrate masks no longer fade into the old green
  vertex palette at 95--285 m; broad/medium noise is filtered by pixel footprint, and close detail
  retains separate fades. This reveals the same brown forest floor in the overview, where missing
  distant undergrowth remains conspicuous. Palette overrides now retain the same smooth slope
  daylight as the baked vertex base through a separate interpolated float (28-byte terrain
  vertices, +232,324 bytes). This does not supply directional cast shadows or full relief normals.
  Moving-camera shimmer and grass/substrate colour agreement still need review.
  Broader unique composition and bespoke landmark geometry remain limited. The
  dapple uses static porous crown volumes rather than mesh-exact canopy shadow projection. A
  portable near-field contact layer now grounds palms, canopy trunks, land rocks, wood, and monkeys.
  Rock/wood/coastal footprints conform to rendered terrain triangles; trees and moving animals
  still use analytic-height fans. These soft ellipses are not mesh-exact cast shadows. Wood model
  heading now agrees with the existing support queries and collision proxies,
  and all instanced jungle foliage now receives two-sided fixed-sun geometric facet lighting, but
  there is still no authored biome texture blend, decals, broad substrate diversity, baked ambient
  lighting, canopy-accurate occlusion, or full directional/self-shadowing.
- The ocean has depth colour, distance-faded crossing-wave subsurface variation, restrained
  shallow-ridge light, short sunward crests, and view-aligned grazing reflection bands from the
  existing analytic slope, waves/specular response, a coarse broad
  depth-limited foam mask, and an exact shore-following animated
  crest/wash ribbon. Both foam layers now use broken continuity masks so the bright waterline does
  not read as one uniform ring. Supplementary grazing contrast is now restrained and unresolved
  colour phases are filtered. Phased wind packets, wider short-wave directions and broad-normal
  Fresnel coverage reduce the strongest cyan crosshatch at swimming height. The spectrum remains
  small and stylized; there is no physically integrated rough-surface or scene-reflection model.
  Swimming adds a deliberately subtle,
  depth-responsive turquoise edge veil with restrained upper-surface scatter and sparse generated
  suspended motes that cools and strengthens with the offshore current, but full volumetric
  underwater fog, a geometric waterline, refraction-like cues, true splash particles, and final
  wash/material tuning remain incomplete. Moving water entry/exit does produce a short
  speed-scaled pair of broken shader-integrated surface rings.
- Palms, grass, canopy trees, understory, low bushes, and forest-floor plants have deterministic
  calm wind, and nearby mesh grass now bends around the player's stationary body radius and expands
  with movement speed. Its curved near blades have directional two-sided daylight, root, fold, and
  restrained transmission cues. Shared rest-shape normals now follow blade curvature/folds and
  anisotropic instance scale, but do not yet respond to animated wind/player bending. A sparse
  juvenile underlayer plus deterministic per-instance
  blade stature, colour, and wind variation reduce repeated close silhouettes. Six restrained
  seed-bearing stems add near-field vertical accents without changing the middle/far tiers or draw
  count. Near blades now narrow progressively through their upper half and seed roots follow
  their actual curved tips. Seven unequal loose local tufts replace isolated roots, but the
  five-level silhouettes, sparse patches and smooth substrate still look stylized close up.
  A near-only low layer now adds 48 short curved blades between tall tufts without extra draws,
  but adds 144 triangles per near instance and does not fill every bare region. It is not a
  complete ground-material solution or physical-mobile performance qualification.
  Grass now retains foliage colour at distance instead of fading into unrelated green ground;
  custom blades have a normalized-height warm/dark basal tint using the existing alpha byte as
  material data, with opaque output. This makes colour treatment consistent, but does not blend
  geometry transitions, remove culling boundaries, increase cover or add root contact shadows.
  The stock fallback retains original mesh colours without the per-fragment basal tint.
  The custom grass shader now samples an immutable RGB24 terrain-height texture to correct
  measured root burial/floating. CPU shader emulation is within 0.02 mm for all audited low-blade
  roots; this is not GPU readback. The stock fallback retains approximate placement. Normal
  deformation and substrate integration remain incomplete; see `grass-grounding.md`.
  Grass frustum bounds now include measured geometry, slope and full shared motion limits;
  a bounded software outward/return walk now records positions, grass populations and per-tree
  submitted canopy LODs. Seven stationary frames plus a capped low-resolution compositor
  sequence cover sampled movement, not every game frame. Full-resolution targeted transition
  review remains necessary; no claim of seamless LOD or alias-free motion follows from these
  telemetry gates. Sparse ground cover and substrate integration remain open.
  Near palms now use
  fuller segmented tapered leaflets with widened ribbon
  far silhouettes and staggered per-instance LOD thresholds; near trunks have subtle procedural
  leaf-scar rings, two variants carry restrained faceted coconut clusters, and transform-derived
  green-only per-instance tint varies fronds without altering brown trunk/root/coconut materials
  or near/far identity. Grass
  collision/displacement does not
  persist after the player passes, and true LOD blending, richer biome variation, aliasing work,
  and physical-device density tuning remain.
  Nearby broadleaf trunks use restrained texture-free longitudinal grain and fissures with a
  bounded distance fade; physical-mobile aliasing and fragment cost remain unqualified.
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
  phase in both locomotion modes. Explicit placement and debug teleports get a bounded one-time
  recovery from overlapping static proxies; dynamic obstacles and continuous depenetration are not
  implemented.
- Substantial rocks and fallen logs have conservative collision; smaller rock dressing remains
  non-solid. Fourteen emergent reef outcrops remain solid while swimming. Their near geometry now
  has smoother shared normals, refined crown silhouettes and filtered procedural stone detail,
  but still uses four stylized profiles rather than scanned geology; there are no normal maps.
  New shading has software-renderer evidence, not post-incident hardware qualification. Walking
  and swimming now
  use very mild speed-responsive positional camera motion,
  which can be smoothly disabled with B. There is no geometric step-up, jump, or complete
  underwater presentation; restrained visual near-surface/current and audio entry/exit feedback
  are implemented. Movement in water now produces restrained shader-integrated expanding ripple
  bands, while moving entry/exit produces one stronger 1.6-second broken-ring burst protected by
  the same 7.5/2.5 cm hysteresis and cooldown as the sound cue. True particles and persistent
  body/shore displacement remain absent. The outer-ocean current eases into a dominant shoreward
  return with a first deterministic coast-parallel circulation variation; it is not a spatially
  turbulent flow simulation and does not react to individual coves, rocks, or waves. It also
  deepens the existing player-water loop into a restrained undertow cue; subjective
  headphone/speaker tuning remains open.
- Desktop mouse sensitivity, 4.3/8.0 m/s walk/run speeds, and the 92/78-percent touch-run
  thresholds are compile-time values. Mobile look offers low/standard/high runtime multipliers,
  and that selection now persists across process restart with the walk/run, camera-motion, and
  audio-mute preferences. WASD and arrows are supported, but there is no general settings UI,
  remapping, gamepad control scheme, or handed touch layout yet.
  Keyboard Shift selection receives a short normal-play confirmation; the touch stick deliberately
  remains a direct hold-style request rather than changing that persistent mode.
- The game intentionally has no objectives, quests, combat, inventory, crafting, survival meters,
  score, progression, day/night cycle, or dynamic weather.

## Wildlife and audio

- A first 24-bird procedural flock has varied shore/canopy/high paths, animated wing strokes,
  layered near trailing feathers, slow per-agent radial drift, banked turns, intermittent glides,
  positional calls, culling, terrain clearance, and distance-scaled simulation. It still lacks
  perching, landing, and long subjective behaviour tuning. Eight shallow-water schools add 48 fish with
  depth/shore constraints, depth-following pitch, banked turns, forked animated tails, near head
  detail, fixed-sun faceted body colour, restrained depth/caustic colour response, culled
  single-surface fins, culling, and distance-scaled simulation; their schooling variation, water
  integration, and long subjective tuning remain
  first-pass quality. A
  first non-hostile shark alternates occasional offshore patrols with longer absences, stays out of
  the shallows, and has dive pitch, banked turns, body/forked-tail animation, near head detail, and
  depth-integrated colour plus distance-scaled simulation; appearance timing and
  underwater integration still need subjective tuning. Seven first-pass procedural monkeys use a
  real fourteen-bone CNA GPU-skinning path and idle/observe/walk/run animation driven by a compact
  autonomous state machine. They share the player's trunk, solid-rock, and fallen-log proxies and
  use short lateral deflections before abandoning blocked targets. Idle/observe/walk/run changes
  now use short local-pose crossfades and preserve a continuous walk phase. The near tier has a
  readable light face mask, protruding eyes and nose, restrained eye gloss, brighter fur/face
  palettes, small near-only crown/cheek fur tufts, and bone-following finger/toe silhouettes
  without another draw call. Calm animals now add a bounded, distance-faded head turn
  toward a nearby player using the existing skeleton and draw count. They remain intentionally
  simple: tightly cluttered pockets
  can cause an early target reselection, and three silhouettes/four palettes cannot hide all
  procedural repetition; detailed surface treatment and long subjective behaviour tuning remain
  limited.
- Original procedural layers supply shoreline-distance-blended surf, sparse stronger-wave accents,
  light canopy breeze, a twenty-nine-second daytime-jungle texture with sixteen irregularly spaced
  insect phrases, paired distance-scheduled sand/soil land footsteps, and speed/depth-responsive
  player movement through water. A single bounded
  positional voice now places two bird phrases and two restrained
  monkey contact phrases at actual nearby agents. The soundscape remains incomplete: richer jungle
  variation, additional wave/water/wildlife phrases, and a long fatigue/listening pass remain
  absent; footstep material/balance and call balance have only numerical plus automated runtime
  validation so far.

## Platform qualification

- Linux/OpenGL 3.3 is the primary playable path and has repeated real-GPU and isolated capture
  coverage. A stripped relocatable release archive includes its pinned CNA runtime, dependency
  notices, and an integrity manifest and passes an extracted `/tmp` smoke run. Compatibility across
  Linux distributions/driver families and extended whole-island release sessions remain open.
- WebGL 2 runs in Chromium with automated rendering, pointer-lock, keyboard/mouse, actual-Escape
  release plus continued-session/delayed-reacquisition checks, overlay, resize, and first-gesture
  audio-mixer/track initialization coverage. Deterministic gzip packaging is validated by local
  hash/round-trip tests, but production CDN/server headers and hardware-browser cold-load time have
  not been qualified. Firefox ESR 140.15 separately passes a production-shell first-frame,
  pointer-lock/reacquisition, Shift toggle, traversal/footstep, audio-start, Escape-release, and
  responsive-sizing gate through its built-in WebDriver BiDi endpoint. Dedicated fixed jungle and
  near-surface-water views additionally require nonzero live LOD/effect state and produce inspected
  frames. One actual second-tab activation reports `visible` to `hidden` to `visible`, resumed
  animation frames, and fresh game input after foregrounding. Audible hardware-browser quality,
  extended tab suspension/audio-focus behaviour, fullscreen, simultaneous mouse look, and the
  remaining Firefox interaction/scene matrix remain unverified.
- Android arm64-v8a and x86_64 APKs build, and a KVM-accelerated API 35 x86_64 AVD renders the
  complete GLES3 scene. Independent movement/look gestures, translucent touch controls, a
  same-process hot resume, AAudio pause/resume, and fixed beach, close-grass, jungle, water,
  monkey, and reef views are validated. The six frames were visually compared with the matching
  WebGL 2 captures through the debug-only intent bridge; newer dedicated bird, palm, frond, prop,
  and liana-inclusive jungle frames additionally qualify their current natural-detail paths. A deterministic reverse
  movement-zone gesture also crosses the south shoreline, starts the water-transition accent, and
  enters the authoritative swimming state. A combined lifecycle gate also validates same-process
  Home/resume and a force-stop/new-process cold relaunch, including fresh GLES 3 and audio-resource
  qualification. Simultaneous real-finger ergonomics, audio-focus interruption, OS-initiated
  process eviction, long suspension, physical mobile-GPU performance, thermal behaviour, and
  physical-device compatibility remain unverified.
  The Android mobile quality profile is now functional and visually inspected on gfxstream, but
  its 112 m grass, 72% shadow, 82% wildlife, and 128-grid ocean choices remain conservative
  defaults rather than device-tier thresholds established from physical hardware measurements.

## Development tooling

- The overlay reports frame timing, player/water state, renderer/logical-viewport identity, terrain/vegetation
  visibility, LOD populations, draw counts, bird/fish/shark/monkey render and simulation tiers,
  monkey behaviour counts, current
  surf/breeze/jungle/player-water levels, played
  wave/splash/footstep counts, mute/availability state, and live source use against the six-source
  budget.
  F2 adds world-space shoreline/current, collision, exact attempted/resolved player motion,
  shared-cell, wildlife heading/target/home-range, ambient-zone, nearest audio-candidate, active
  positional-emitter, exact nine-sample terrain-contact, and complete bird route-family/spawn views;
  movement history, steering proposals, and deeper per-system culling/LOD overlays remain open.
- Eight direct debug teleports, a bounded debug-only inspection camera, a native/Web fixed-frame
  suite plus a debug-only Android fixed-view launch bridge and a 676 m moving benchmark, an
  asset-manifest validator, and a deterministic
  world-statistics report exist. The native overview has automated P6 integrity and broad
  non-degeneracy checks, but there is no teleport menu, pixel-golden comparison, or automated
  native/Web/Android image comparison yet. The moving benchmark has no Web/Android timing baseline,
  and the world fingerprint is a same-toolchain change detector rather than a cross-libm canonical
  world hash.
