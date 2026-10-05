# Architecture

## Boundaries

`island-simulator` is a game built on CNA, not an engine fork. CNA `next` and Sharp Runtime `next`
are read-only. Gameplay and rendering code may use their public APIs, but fixes and compatibility
adapters remain in this repository. Confirmed dependency defects are recorded in `BUGS.md`.

The current source layers are:

```text
IslandTerrain ───────── authoritative terrain and shoreline queries
      │
      ├── PlayerMotion ─────── renderer-independent walking, swimming, obstacle collision,
      │                        and ocean limit
      ├── VegetationLayout ─── deterministic palm placement and trunk collision proxies
      │       │
      │       └── PalmRenderer ─ near/far meshes, cell culling, instancing, and shader wind
      ├── GrassLayout ──────── deterministic density, scale, phase, and colour anchors
      │       │
      │       └── GrassRenderer ─ near/far blade clusters, cell culling, instancing, and wind
      ├── JungleLayout ─────── canopy/understory/bush/ground placement and trunk proxies
      │       │
      │       └── JungleRenderer ─ four-layer LOD, culling, instancing, and wind
      ├── NaturalPropLayout ── rock/log/driftwood placement and solid-prop collision proxies
      │       │
      │       └── NaturalPropRenderer ─ rock/log meshes, LOD, culling, and instancing
      ├── SkyRenderer ──────── fixed gradient, sun disc, high clouds, and stock-effect fallback
      ├── OceanRenderer ────── portable animated water shader and stock-effect fallback
      ├── DebugOverlay ─────── asset-free font, frame timing, and scene diagnostics
      ├── TouchControls ────── renderer-independent two-zone contact ownership and output
      ├── TouchOverlay ─────── generated movement/look affordances through SpriteBatch
      ├── SoundscapeModel ──── deterministic PCM generation and shoreline-driven crossfades
      ├── Soundscape ───────── CNA loop ownership, volume application, mute, and failure fallback
      └── IslandGame ───────── CNA lifecycle, input, camera, terrain cells, and pass ordering
```

`IslandTerrain`, `PlayerMotion`, `VegetationLayout`, `GrassLayout`, `JungleLayout`,
`NaturalPropLayout`, `TouchControls`, and `SoundscapeModel` form `IslandSimulator::Core` and
deliberately do not depend on CNA. They compile and test quickly without a graphics device.
`IslandGame` converts CNA keyboard, mouse, and touch snapshots to a world-space movement direction
and converts the core player position back to CNA `Vector3` for the camera. Collision and rendering
consume the same palm, broadleaf, rock, and fallen-log placements; rendering never becomes a
separate source of world truth.

## Coordinates and scale

- One world unit is one metre.
- `Y` is up and mean water level is `Y = 0`.
- The nominal island diameter is 500 m.
- The initial south-beach spawn faces roughly toward the island centre along positive `Z`.
- Camera yaw zero looks along positive `Z`; pitch is clamped before it can flip.

The island is an authored-procedural hybrid. The current analytic terrain is the deterministic base
layer: an irregular radial shoreline, a shallow shelf, beaches, rolling interior relief, and a low
viewpoint hill. Later biome masks, routes, coves, and landmarks must preserve the single-query rule:
rendering, collision, vegetation, wildlife, water, and tests all consult the same authoritative
field or a documented derived cache.

## Dependency resolution

`dependencies.lock` records the qualified commits. CMake verifies:

- CNA on `next`, or a pristine generated export carrying the exact CNA revision marker;
- Sharp Runtime on `next`;
- EasyGL and MetaGL on their CNA-required `develop` branches;
- exact revisions by default.

The main shared CNA checkout is currently on another branch and must not be switched by this
project. `tools/materialize_cna_next.sh` uses `git archive` to produce an unmodified, ignored source
tree under `build/dependencies/cna-next`. It refuses a stale destination and never edits the source
repository. Developers may instead provide a separate read-only CNA `next` checkout through
`CNA_ROOT_DIR`.

## Frame flow

1. CNA polls platform events and publishes keyboard, mouse, and touch snapshots.
2. `IslandGame::Update` reads each input device once. `TouchControls` retains stable movement/look
   pointer roles, emits one-frame relative look deltas, and cancels invalid contacts explicitly.
3. Input is transformed by camera yaw into a normalized horizontal wish direction.
4. `PlayerMotion` applies frame-time clamping, acceleration, slope checks, swimming speed, the
   island-relative current field, and the remote safety boundary.
5. `PlayerMotion` tests the proposed capsule footprint against terrain and visible trunk-sized
   circular obstacles, retaining axis sliding when one component is valid.
6. `SoundscapeModel` derives surf/breeze targets and player-water gain/pitch from authoritative
   shoreline, support-depth, locomotion, and velocity state; `Soundscape` applies smoothed changes
   to three persistent CNA loop instances and schedules bounded wave accents.
7. `IslandGame::Draw` submits the camera-centred sky with depth disabled, restores normal depth
   state, then submits opaque terrain, natural props, broadleaf jungle, mesh grass, palms, and the
   transparent ocean using one view/projection pair.
8. On Android, or after a touch interaction on another platform, `TouchOverlay` submits generated
   translucent control rings through CNA `SpriteBatch` with conservative edge margins.
9. When explicitly enabled, `DebugOverlay` submits its translucent panel and bitmap text through
   CNA `SpriteBatch` after all 3D passes.
10. CNA presents once from `Game::EndDraw`; game code does not call `Present`.

The normal game loop uses CNA's fixed-timestep mode. Core movement still clamps a long individual
step to 100 ms so a debugger pause or loading stall cannot launch the player through the world.

## Ocean boundary model

The current boundary is deliberately layered:

1. Shoreline water remains traversable and transitions to swimming by terrain depth with
   hysteresis.
2. At 42 m offshore, an inward current begins gently and outward input progressively loses effect.
3. By 72 m offshore, the current is fully developed.
4. At 92 m offshore, a last-resort positional safety cap removes residual outward velocity.

All distances derive from `IslandTerrain::SignedShoreDistance`; there is no beach collision wall or
hard-coded rectangular boundary. Later audio, wave, and camera feedback can communicate the same
current without changing its physical invariant.

## Rendering baseline

The terrain's authoritative 241 by 241 source grid is partitioned into 64 render cells. Each cell
has portable 16-bit indices and an independent bounding box for frustum rejection; exact shared
sample coordinates prevent cracks. Fixed daylight is currently baked into procedural terrain and
palm vertex colours. Terrain, ocean, and sky share one project-owned fixed sun direction so the
surface lighting, water glint, and visible sun cannot silently diverge.

`SkyRenderer` draws a camera-centred 900 m sphere before world geometry. Its GLSL 330 / GLSL ES
300 fragment shader combines a horizon-to-zenith tropical gradient, warm sun disc and halo, and
subtle static high cloud wisps. The sky has no clock or weather input by design. Depth writes and
testing are disabled only for this pass and restored immediately afterward. A vertex-coloured
`BasicEffect` gradient remains available with an explicit diagnostic if custom effects are not
supported. Native OpenGL 3.3 and Chromium/WebGL 2 execute the full shader; the current arm64 GLES
3 package compiles its GLSL ES path, but stable Android runtime validation remains open.

Four palm variants use curved tapered trunk meshes and 10--12 ribbon fronds in the near tier, with
root wedges around the trunk base as a low-cost contact cue. A lower-segment trunk and seven-frond
far tier reduces distant vertex work. A deterministic 330-instance layout is grouped into the same
8 by 8 spatial grid as terrain. CNA hardware instancing reduces a typical overview from hundreds
of palm submissions to eight near/far variant draws; capability-checked per-palm drawing remains
the functional fallback. Cells are rejected by frustum and a conservative distance bound before
instances are gathered. The portable GLSL 330 / GLSL ES 300 shader bends only frond-weighted
vertices with per-frond phase variation, leaving roots and trunks stable in a calm breeze.

Grass uses 18,556 deterministic cluster anchors grouped into a finer 16 by 16 cell grid. Each
visible anchor instances one shared 28-blade near cluster or five-blade far cluster. The vertex
shader reconstructs a terrain-aligned basis from the authoritative surface normal, applies
independent height/spread/colour/phase variation, and adds a bounded two-frequency breeze. The
path keeps denser but shorter turf rather than an artificial bare stripe. Distance colouring merges
the far tier into the ground before culling. Native OpenGL 3.3 and WebGL 2 use the portable
instanced shader; a slower static `BasicEffect` path is retained for unsupported renderers.

The layered interior jungle adds 281 broadleaf trees, 1,822 medium plants, 565 low bushes, and
2,225 small forest-floor plants. Four
canopy variants combine tapered trunks with clustered low-poly crowns; four understory variants
build fern and broad-leaf silhouettes from curved opaque strips. Three bush variants combine
offset low ellipsoid lobes to bridge the visual gap between grass and medium plants. Three small
rosette variants use short curved opaque leaves to break up grass-heavy ground at first-person
range. Placement uses
independent broad/fine cluster fields plus terrain height, slope, and shoreline constraints while
preserving the winding route, two clearings, and beach spawn. Each layer has independent near/far
distances but remains gathered into variant batches; the deterministic jungle camera renders 102
bushes and 129 ground plants in 24 submissions. A portable GLSL 330 / GLSL ES 300 shader
applies weighted calm wind, and `BasicEffect` remains the explicit fallback. Native OpenGL 3.3 and
actual Chromium/WebGL 2 execute the full shader; the current arm64 GLES 3 package compiles but has
not been rerun on stable Android hardware.

Natural props add 152 irregular rocks across beach/interior bands, 11 mossy fallen logs in the
low-slope interior, and 28 small sun-bleached driftwood pieces in the dry beach belt. Four rock,
three log, and three driftwood variants use independent near/far meshes, cell culling, variant
batching, and a portable static GLSL 330 / GLSL ES 300 shader. Opaque contact geometry grounds the
larger classes without transparency overdraw; driftwood uses an irregular bent tube plus one or two
branch stubs. Placement validates each complete log/driftwood footprint against shore distance,
route reservations, nearby trees, slope, and support-height variation. Collision is limited to
substantial rocks and sampled fallen-log spans, leaving small driftwood non-solid. Native OpenGL
3.3 and Chromium/WebGL 2 execute the full instanced path; dedicated prop and driftwood cameras
provide deterministic close review.

The ocean is a 160 by 160 quad grid drawn after opaque geometry. `OceanRenderer` supplies explicit
GLSL 330 and GLSL ES 300 variants through CNA's public portable shader-package extension. The
shader combines vertex waves, analytic fragment normals, Fresnel sky response, sun glint,
depth-driven tropical colour/transparency, moving foam, and haze. A stock `BasicEffect` fallback
keeps the scene operable with a clear diagnostic if the custom effect cannot be created. Native
OpenGL 3.3 and actual WebGL 2 execution select the full path. The API 35 Android emulator has
created a real GLES3 surface and visibly rendered the current terrain/palm/grass scene; the captured
camera did not establish every ocean effect variant, and mobile-GPU profiling remains open.

The game requests the XNA `HiDef` profile because its deterministic QA capture uses back-buffer
readback and the intended 3D feature set exceeds a `Reach`-only design. This is an application
requirement, not a CNA workaround.

The developer overlay creates its 5 by 7 uppercase glyph atlas at runtime, so it has no font-file
or content-pipeline dependency. It measures wall-clock spacing between `Draw` calls instead of
reporting the fixed simulation timestep. F1 toggles it during interactive play; `--debug-overlay`
is the explicit automation path. Native OpenGL 3.3 and actual WebGL 2 captures validate the same
SpriteBatch route.

## Audio baseline

`SoundscapeModel` supplies original deterministic 22.05 kHz mono PCM16 source material at startup.
Circular filtering keeps the procedural noise continuous at loop boundaries; peak
normalization retains mixing headroom. Surf is strongest along the beach and while swimming, then
falls to a quiet distant bed inland. Light canopy breeze increases toward the interior. A 1.8
second exponential crossfade prevents zone snaps, and M applies a reversible zero-gain mute without
restarting the tracks.

A six-second water-motion loop follows the player's authoritative support depth, locomotion mode,
and horizontal velocity. Dry movement and stationary water produce zero gain; shallow wading and
active swimming fade in with different conservative pitch ranges. One persistent loop avoids the
rapid repeated one-shot cadence common to footstep-style water effects, while 140 ms attack and
340 ms release smoothing prevent hard edges and state-transition chatter. All current procedural
PCM payloads total 1,389,150 bytes (about 1.33 MiB) before backend bookkeeping.

A separate 1.5-second splash texture handles crossings between dry and shallow-water states. The
detector uses 7.5 cm entry and 2.5 cm exit thresholds, ignores stationary state changes, and applies
a 1.6-second cooldown longer than the sample, so shoreline jitter cannot overlap or machine-gun the
effect. Entry and exit share the source with different bounded pitch/pan/gain variation. It may
overlap a stronger-wave accent, bounding the present mix to three persistent loops and at most two
short one-shots.

A separate original four-second PCM wash supplies occasional stronger waves. A deterministic
7--13 second initial delay and 14--32 second repeat interval vary conservative pitch and stereo pan
without allowing overlaps. The scheduler virtualizes the accent while inland surf is inaudible,
so it contributes at most one short one-shot to the broader bounded source budget.

`Soundscape` owns CNA `SoundEffect` and looping `SoundEffectInstance` objects and never calls SDL
or a platform audio API. Linux and Android start after content initialization. Web defers PCM/audio
device creation until the first mouse, touch, or movement-key gesture so browser activation and
pointer-lock requests share one user action. Missing or disconnected audio degrades to a logged,
visible `UNAVAILABLE` state rather than terminating rendering. Core tests cover deterministic PCM,
length/rate, headroom, audible energy, loop-boundary discontinuity, zone ordering, crossfade,
water depth/speed gating and smoothing, transition hysteresis/cooldown, and wave-accent interval/
virtualization bounds. Native playback started all three loops and accepted the scheduled
stronger-wave one-shot. Actual Chromium proves first-gesture mixer/track initialization, and a
deterministic shoreline traversal from `(0, -225)` to `(0, -251.332)` successfully played the
transition effect; a visible hardware-browser listening pass and Android audio focus/lifecycle
testing remain open.

## Current limitations

- Terrain has cell culling but no distance LOD, authored material textures, or final biome masks.
- Palms have four geometry variants, placement clustering, cell culling, near/far meshes, subtle
  shader wind, trunk collision, and small root contact wedges. Pop-free LOD blending, richer
  close-up geometry/materials, and grounding shadow remain open.
- The first grass, layered-jungle, rock, and fallen-wood tiers substantially improve first-person
  grounding, but transition popping, close-up leaf/material shape, bush diversity, biome variation,
  and measured mobile density still need repeated polish. Broader beach-prop variety remains.
- Collision checks every palm, canopy-trunk, solid-rock, and sampled log proxy and has no spatial
  broad phase yet; geometric step-up remains absent.
- Swimming camera height and movement-responsive water audio are functional, but camera bob,
  underwater presentation, and distinct water entry/exit details remain open.
- WebGL 2 has run in Chromium/SwiftShader with the full sky, ocean, palm, jungle, prop, and grass
  shaders.
  Automated browser input covers pointer lock, focus, keyboard traversal, relative look,
  release/reacquisition, responsive 16:9 presentation, and first-gesture CNA audio startup. Audible
  hardware-browser quality, genuine tab lifecycle, another browser engine, and visible
  hardware-GPU performance remain unverified.
- Android arm64-v8a and x86_64 APKs build and application-owned Android lint passes. One x86_64
  API 35 GLES3 frame is visually validated, but the host has no KVM and the software-emulated
  Google Play image repeatedly enters System UI ANRs. Touch ergonomics, lifecycle restoration,
  audio focus, sustained performance, and physical-device behaviour remain unverified.
- No wildlife, positional/biome-rich soundscape, world-space debug visualization, or configuration
  UI exists yet. The first two ambient beds and four direct QA teleports are available.
