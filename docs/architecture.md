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
      ├── WorldCollisionGrid ─ static 16 m broad phase over shared collision proxies
      ├── VegetationLayout ─── deterministic palm placement and trunk collision proxies
      │       │
      │       └── PalmRenderer ─ near/far meshes, cell culling, instancing, and shader wind
      ├── GrassLayout ──────── deterministic density, scale, phase, and colour anchors
      │       │
      │       ├── GrassInteractionModel ─ smoothed player-motion bending strength
      │       ├── GrassLod ─── renderer-independent progressive population split
      │       └── GrassRenderer ─ three mesh tiers, culling, instancing, wind, and local bending
      ├── JungleLayout ─────── canopy/understory/bush/ground placement and trunk proxies
      │       │
      │       └── JungleRenderer ─ four-layer LOD, culling, instancing, and wind
      ├── NaturalPropLayout ── land/reef rock and fallen-dressing placement plus solid proxies
      │       │
      │       └── NaturalPropRenderer ─ rock/log/frond meshes, LOD, culling, and instancing
      ├── ShipwreckLayout ──── authored shore-relative hull, debris, and bounded solid proxies
      │       │
      │       └── ShipwreckRenderer ─ one portable culled static batch for the complete landmark
      ├── DistantSceneryLayout ─ deterministic unreachable horizon-islet composition
      │       │
      │       └── DistantSceneryRenderer ─ fogged low-poly rocks/palms in one static batch
      ├── AquaticLightingModel ─ portable shared depth/sun-band colour profile
      ├── BirdFlock ───────── deterministic shore/canopy/high flight and simulation tiers
      │       │
      │       └── BirdRenderer ─ CPU wing pose, near/far meshes, culling, one dynamic batch
      ├── CrabColony ──────── sand/home-range state machine and simulation tiers
      │       │
      │       └── CrabRenderer ─ articulated near/far poses, culling, one dynamic batch
      ├── ButterflyMeadow ─── clearing habitat state machine and simulation tiers
      │       │
      │       └── ButterflyRenderer ─ hinged near/far wings, culling, one dynamic batch
      ├── SeaTurtleGroup ─── bounded near-shore depth/state model and simulation tiers
      │       │
      │       └── SeaTurtleRenderer ─ articulated near/far poses, culling, one dynamic batch
      ├── FishSchools ─────── deterministic depth-bounded schools and simulation tiers
      │       │
      │       └── FishRenderer ─ CPU tail pose, near/far meshes, culling, one dynamic batch
      ├── SharkPatrol ─────── occasional offshore patrol, dive cycle, and simulation tier
      │       │
      │       └── SharkRenderer ─ CPU body pose, near/far meshes, culling, one dynamic batch
      ├── MonkeyTroop ─────── interior state machine, terrain constraints, and simulation tiers
      │       │
      │       └── MonkeyRenderer ─ generated rig/mesh/clips and portable CNA GPU skinning
      ├── WildlifeActivationPolicy ─ shared full/reduced/dormant update cadence
      ├── ProceduralMeshValidation ─ renderer-independent 16-bit triangle upload gate
      ├── RuntimeQualityProfile ── centralized full/mobile distance and resource policy
      ├── SkyRenderer ──────── fixed gradient, sun disc, high clouds, and stock-effect fallback
      ├── OceanRenderer ────── portable animated water shader and stock-effect fallback
      ├── ShorelineRenderer ── exact-coast foam/wash ribbon and stock-effect fallback
      ├── TerrainRenderer ──── shared mesh, cell culling/run batching, portable material, fallback
      ├── DebugOverlay ─────── asset-free font, viewport/frame timing, and scene diagnostics
      ├── WorldDebugRenderer ─ portable boundaries, cells, collision, and wildlife lines
      ├── DebugFreeCamera ──── bounded renderer-independent inspection flight in debug builds
      ├── FirstPersonCameraMotion ─ bounded walking/swimming positional motion and opt-out
      ├── WaterRippleModel ──── movement ripples plus hysteretic entry/exit surface bursts
      ├── WaterViewModel ───── depth/current-responsive veil intensity and transition smoothing
      ├── WaterViewOverlay ─── generated premultiplied edge texture and one conditional draw
      ├── TouchControls ────── renderer-independent two-zone contact ownership and output
      ├── TouchOverlay ─────── generated movement/look affordances through SpriteBatch
      ├── SoundscapeModel ──── deterministic PCM generation and shoreline-driven crossfades
      ├── AudioAnalysis ────── deterministic PCM measurements for host-only quality reports
      ├── WorldIdentity ───── shared procedural-layout seeds and stable diagnostic text
      ├── UserSettings ───── bounded schema-1 preference model and strict text parser
      ├── UserSettingsStore ─ CNA storage adapter with primary/backup recovery
      ├── Soundscape ──────── CNA loop ownership, volume application, mute, and failure fallback
      └── IslandGame ───────── CNA lifecycle, input, camera, and pass ordering
```

`IslandTerrain`, `PlayerMotion`, `DebugFreeCamera`, `FirstPersonInput`, `FirstPersonCameraMotion`, `WaterViewModel`, `VegetationLayout`, `GrassLayout`,
`JungleLayout`,
`NaturalPropLayout`, `ShipwreckLayout`, `DistantSceneryLayout`, `BirdFlock`, `CrabColony`,
`ButterflyMeadow`, `SeaTurtleGroup`, `FishSchools`, `AquaticLightingModel`,
`SharkPatrol`, `MonkeyTroop`,
`WildlifeActivationPolicy`, `ProceduralMeshValidation`, `RuntimeQualityProfile`, `TouchControls`,
`SoundscapeModel`, `AudioAnalysis`, `WorldIdentity`, and `UserSettings` form
`IslandSimulator::Core` and
deliberately do not depend on CNA. They compile and test quickly without a graphics device.
`IslandGame` maps CNA keyboard snapshots into `FirstPersonDigitalInput`; the renderer-independent
resolver combines them with touch axes and camera yaw using the screen-right basis required by the
view convention. It converts the core player position back to CNA `Vector3` for the camera. Collision and rendering
consume the same palm, broadleaf, rock, fallen-log, and major ship-hull placements; rendering
never becomes a separate source of world truth.

`UserSettingsStore` is the narrow persistence boundary. It uses CNA `StorageDevice` and
`StorageContainer`, so Linux uses the app's XDG-backed data root, Web uses origin-scoped IDBFS, and
Android uses app-private storage without platform paths leaking into gameplay code. Writes first
create a temporary record, retain the previous valid primary as `settings.ini.bak`, then replace
the primary `settings.ini`. The parser has a 4 KiB limit, rejects duplicate known keys and invalid
schema values, and ignores unknown keys for forward compatibility. Ordinary interactive launches
apply settings before content startup; isolated QA modes and `--no-user-settings` deliberately do
not touch them.

## Codebase conventions

- Production code lives in the `island` namespace. The platform entry point is the only intended
  global-scope boundary; anonymous namespaces hold translation-unit implementation details.
- A source file includes its matching project header first, then other island headers, C++ standard
  library headers, and finally CNA/XNA headers. Headers include what they use and do not depend on a
  source file's transitive includes.
- `IslandGame` owns the application lifetime and high-level subsystems. Renderers own their GPU
  resources through values or `std::unique_ptr`; core world data prefers values and standard
  containers. References and raw pointers are non-owning and must not outlive the owner that
  supplied them. Cleanup remains RAII-driven rather than ordered manual shutdown code.
- One world unit is one metre, durations passed to simulation are seconds, speeds are metres per
  second, and internal angles are radians. Degrees are limited to diagnostics or user-facing
  configuration. Names that cross a unit boundary include the unit or make the conversion local.
- The world basis is right-handed: positive `X` is camera-right at yaw zero, positive `Y` is up,
  and positive `Z` is the initial inland/forward direction. Camera construction uses explicit eye
  and target points through CNA/XNA; code must not silently substitute a negative-`Z` convention.
  CPU transforms keep the established scale-then-rotation-then-translation order.
- Types and public/member functions use `UpperCamelCase`; local variables and data members use
  `lowerCamelCase`, with a trailing underscore for members; compile-time constants use
  `UpperCamelCase`. Files that define a primary type use that type's English name. Diagnostics,
  comments, tests, documentation, asset metadata, and in-game text remain English.
- Project-specific math helpers belong in the renderer-independent core only when they encode a
  tested island rule or remove repeated error-prone work. CNA/XNA vector, matrix, frustum, colour,
  and bounding primitives remain the default rather than being wrapped for stylistic reasons.

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

The first large authored landmark is a 20.0 m by 5.2 m grounded wooden sailing-ship wreck. Its
layout resolves the placement from a coast angle and signed distance through the authoritative
shore field, samples terrain support for the heeled hull and twelve beachward debris pieces, and
publishes only six conservative circular proxies for major structure. Small debris is intentionally
non-solid. The renderer derives its complete broken hull, exposed frames, partial deck, cabin,
snapped main mast, broken foremast, bowsprit, rigging, and debris from that immutable layout in one
3,948-triangle static batch; it does not own an independent position or scale. A denser cross-
section rounds the lower hull, two localized missing-board openings expose the break, and a keel,
rudder, broken deck rails, plus dark side openings provide readable large-ship scale. The beach
debris uses irregular splintered prism outlines and stable bidirectional angle variation instead
of plain parallel boxes. Its portable GLSL 330 / GLSL ES 300 material
projects longitudinal grain along the authored ship axis and adds bounded weathering, waterline
darkening, salt wear, and wet sheen. Vertex-colour lighting remains an actionable fallback when
custom effects are unavailable.

The public terrain-query contract accepts horizontal coordinates whose absolute components are at
most 1,000,000 m, vastly beyond every reachable or rendered position. Non-finite or larger input
fails closed to a finite, upward-facing deep-seabed sample and water classification before value
noise can convert a cell coordinate to `int32_t`. A non-finite polar angle selects the deterministic
zero-angle shoreline profile. This boundary keeps malformed debug, spawn, or movement data from
poisoning later collision and rendering state without complicating valid-world queries.

Terrain vertices carry the authoritative signed shoreline distance and slope beside their
prelit base colour. A portable GLSL 330 / GLSL ES 300 material pass uses those values for sand,
wet-edge, interior-soil, and exposed-slope transitions, then adds two scales of smooth value noise
and fine breakup near the camera. Detail fades with distance to control aliasing and fill cost.
`BasicEffect` remains an actionable fallback if custom effects are unavailable or fail to compile.

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

## Runtime content location

The current game has no runtime dependency on its process working directory or on an absolute
developer path. Portable shader sources are embedded in their owning renderers, while meshes,
most small textures, and PCM audio are generated deterministically during startup. One file-backed
floor material uses `TitleContainer` to load `Content/terrain/forest-floor.png` relative to the title,
not the current directory. Native install, Web preload and Android AssetManager package that exact
path. `ForestFloorMaterial` owns and uploads the full mip chain; failed loading logs its path and
retains procedural soil. A one-time CNA capability query selects the immutable 4x anisotropic
clamp sampler where supported, otherwise ordinary trilinear clamp; explicit gradients and all
mip levels remain unchanged. See `forest-floor-material-review.md` for sampling and transient memory.
Source-side asset
provenance files and Web packaging templates are development inputs rather than files opened by a
running game.

Adding an external runtime asset later requires an explicit cross-platform packaging and lookup
design; the game must not silently assume that it starts in the repository root. The
`IslandGameRelocatableWorkingDirectory` display test launches the actual executable from an
otherwise empty build-owned directory to protect the present contract.

## Frame flow

1. CNA polls platform events and publishes keyboard, mouse, and touch snapshots.
2. `IslandGame::Update` reads each input device once. `TouchControls` retains stable movement/look
   pointer roles, emits one-frame relative look deltas, and cancels invalid contacts explicitly.
   Non-finite or implausibly remote contact coordinates cannot acquire or retain a role, invalid
   display geometry releases both roles, and a one-frame look delta is capped to one display span.
   A separate bounded top-right contact cycles the retained 0.72/1.00/1.35 touch-look response
   without becoming a look pointer; its active ownership also suppresses SDL's synthetic mouse
   path. Resize and ordinary ownership reset preserve the selection.
   Native desktop movement can acquire relative mouse mode on its first key edge; acquisition never
   consumes the pre-capture absolute cursor position as look input.
3. `FirstPersonInput` gives WASD and arrows the same local axes, combines touch, and transforms the
   result by current camera yaw before physics. Mouse yaw is updated first, so held forward movement
   turns during the same update. Its right basis matches visible screen-right rather than world +X.
   Full touch-stick travel requests the faster land pace through a hysteretic threshold, while
   partial travel remains analog walking.
4. `LocomotionPaceToggle` edge-detects either Shift key and retains the selected 4.3/8.0 m/s
   walk/run mode. It still consumes physical key state while player toggling is disabled, so a
   Shift press used by the debug free camera cannot leak into player pace after leaving that
   camera. `IslandGame` starts a 1.4-second normal-play confirmation on each accepted change;
   `PlayerMotion` applies frame-time clamping, direction-independent vector acceleration,
   slope checks, swimming speed, the island-relative current field, the remote safety boundary,
   and horizontal solid-prop collision in both locomotion modes. The current velocity eases from
   zero at 42 m offshore to a dominant shoreward return plus restrained, spatially coherent
   coast-parallel circulation at 72 m. The first indexed update after
   explicit placement performs a bounded static-proxy depenetration pass, preventing a debug
   teleport or future spawn from leaving the capsule embedded.
5. `PlayerMotion` asks the immutable 16 m `WorldCollisionGrid` for local candidates, then tests the
   proposed capsule footprint exactly against nearby trunk, land/reef rock, log, and major
   ship-hull circular obstacles while retaining axis sliding when one component is valid.
6. `FirstPersonCameraMotion` derives a bounded local right/up displacement from physical speed and
   locomotion mode. It eases between walking and swimming rhythms, clamps long frames, resets on
   debug teleports, and fades to the authoritative eye point when B disables it.
   In debug builds, the F3 `DebugFreeCamera` instead advances a separately bounded yaw-relative
   inspection position while `PlayerMotion` remains frozen; leaving F3 restores the unchanged
   authoritative player eye.
7. `WaterViewModel` converts swimming state, authoritative support depth, and normalized offshore
   current to a smoothed veil intensity. `WaterViewOverlay` later uses that value for one generated
   `SpriteBatch` draw with a vertical depth haze, restrained upper-surface scatter, and peripheral
   colour; it cools slightly as current develops and submits nothing while dry.
   `WaterRippleModel` independently derives a smoothed, wrapped surface-ripple state from support
   depth and horizontal speed. The same finite-safe model detects moving wet/dry crossings with
   7.5/2.5 cm hysteresis, a 1.6-second cooldown, and a bounded 1.6-second transition phase; the
   ocean shader consumes both states without another render submission.
8. `SoundscapeModel` derives surf/breeze/jungle/stream targets and player-water gain/pitch/undertow from
   authoritative shoreline, the shared stream route, support-depth, locomotion, velocity, and current state; `Soundscape` applies smoothed changes
   to four concurrently active CNA loop instances (stream and player-water dynamically share one
   active voice) and schedules bounded wave/water accents, resolved-travel
   sand/soil footsteps, plus one pooled positional bird/monkey call voice from nearby wildlife
   state.
9. `BirdFlock`, `CrabColony`, `ButterflyMeadow`, `SeaTurtleGroup`, `FishSchools`, `SharkPatrol`,
   and `MonkeyTroop`
   update steering, terrain/depth
   constraints, behaviour/animation state, and observer-relative full/reduced/dormant cadence
   independently of their renderers. `BirdPerchLayout` derives branch/rock/wreck anchors from
   existing world geometry; `BirdFlock` admits at most two shore/canopy birds to approach, contact,
   perch and departure states while high birds remain in uninterrupted flight.
10. `IslandGame::Draw` submits the camera-centred sky with depth disabled, restores normal depth
   state, then submits opaque terrain and distant scenery. `ContactShadowRenderer` overlays its
   depth-read terrain grounding batch before natural props, the shipwreck, broadleaf jungle, mesh grass, palms,
   the one-batch beach crabs, clearing butterflies, and procedural birds, shallow-water fish,
   near-shore sea turtles, the occasional offshore shark, GPU-skinned
   monkeys, and transparent ocean using one view/projection pair. The shipwreck is submitted after
   other opaque natural props and before water so ocean transparency colours and reveals its
   submerged structure. Aquatic wildlife is also submitted before water. The near-surface veil follows the
   3D and shoreline passes so it colours their combined result.
11. On Android, or after a touch interaction on another platform, `TouchOverlay` submits generated
   translucent control rings through CNA `SpriteBatch` with conservative edge margins. Both the
   generated texture and tint colours use premultiplied RGB, matching the default `AlphaBlend`
   contract and avoiding opaque quads or bright antialiased fringes.
12. For 1.4 seconds after a keyboard Shift edge, `DebugOverlay` uses the same generated font for a
   small premultiplied run/walk confirmation; it submits nothing after the notice expires.
13. When explicitly enabled, `DebugOverlay` submits its translucent developer panel and bitmap
   text through CNA `SpriteBatch` after all 3D passes.
14. CNA presents once from `Game::EndDraw`; game code does not call `Present`.

The normal game loop uses CNA's fixed-timestep mode. Core movement still clamps a long individual
step to 100 ms so a debugger pause or loading stall cannot launch the player through the world.

## Ocean boundary model

The current boundary is deliberately layered:

1. Shoreline water remains traversable and transitions to swimming by terrain depth with
   hysteresis.
2. At 42 m offshore, an inward current begins gently and outward input progressively loses effect.
3. By 72 m offshore, the current is fully developed.
4. The generated peripheral water veil strengthens and cools between the same current thresholds,
   providing a quiet visual cue without a HUD warning or geometric wall.
5. The existing continuous player-water loop develops a bounded deeper undertow bed from the same
   normalized field, remaining silent outside swimming and adding no persistent audio source.
6. At 92 m offshore, a last-resort positional safety cap removes residual outward velocity.

All distances derive from `IslandTerrain::SignedShoreDistance`; there is no beach collision wall or
hard-coded rectangular boundary. Later audio, wave, and camera feedback can communicate the same
current without changing its physical invariant.

## Rendering baseline

The terrain's authoritative 241 by 241 source grid is stored once as 58,081 vertices in one
portable 16-bit indexed mesh. Its indices are ordered in 64 render cells with independent bounding
boxes for frustum rejection. Adjacent visible cells are coalesced into contiguous index ranges, so
an overview currently submits 58 visible cells in three draws instead of 58 separate vertex/index
buffer draws. Exact shared vertices prevent cracks and eliminate duplicated cell-edge data while
retaining cell-granular culling. Fixed daylight is currently baked into procedural terrain and
palm vertex colours. Terrain, ocean, and sky share one project-owned fixed sun direction so the
surface lighting, water glint, and visible sun cannot silently diverge.

Terrain uses a standard-layout 28-byte upload: float3 position, four opaque RGBA bytes, then
float3 shore distance/slope/daylight. A three-element CNA declaration uses offsets 0/12/16;
compile-time layout assertions and a runtime declaration-stride check protect uploads. Compared
with the previous 24-byte vertex, the one shared buffer adds 232,324 bytes (1,626,268 total).
`TerrainDaylight` preserves the existing bounded fixed-sun response and rejects non-unit/non-finite
normals before upload. CPU tests compare every rendered-grid sample with CNA's legacy calculation
and every possible RGB byte. RGB remains prelit and alpha 255 for the untextured/unlit BasicEffect
fallback; EasyGL's inspected stock coloured program requires only position and colour, not UV.
This is source/CPU compatibility evidence, not a newly forced fallback runtime test.

The custom shader interpolates the separate daylight float and applies it only to replacement
palette colours and additive albedo variation. Multiplicative variation already follows lit RGB;
it is not lit twice. Sand flecks, humus, moss, clearing/path earth and fallen leaves now agree with
the slope instead of overwriting its light. Canopy transmission still applies afterwards, as do
fog and the separate view-dependent wet-sand sheen. This adds one varying, no normal attribute,
texture or draw. It does not add terrain cast shadows or change the existing analytic clod-relief
approximation; physical-GPU memory/fragment cost and Android execution remain unqualified.

The portable terrain material reconstructs broad, medium, and fine world-space variation from
the same interpolated shore-distance and slope fields on GLSL 330 and GLSL ES 300. Inside the
jungle belt, a 24--68 m camera fade bounds an additional microvariation octave and two
deterministic leaf-litter grids. Each litter cell derives position, scale, bend, orientation, and
presence from one hash; pointed curved silhouettes replace the rejected first oval pass. Warm
humus now dominates the exposed forest floor, with a narrower cool-moss region instead of a
nearly continuous green substrate. Two dry/decayed leaf populations have per-leaf colour,
restrained fold/midrib shading, and footprint-filtered edges/detail. World-position derivatives
are evaluated before any material branch. At maximum size, bend, filtering and jitter, a leaf's
conservative axis extent is 0.48086 cell units, below the 0.5 boundary: single-cell evaluation
does not cut off a neighbouring leaf. Entire leaves fade as they become subpixel, independently
of the existing camera-distance fade; this is not a claim of fully shimmer-free camera motion.
They remain flat material marks, not displaced geometry or physical leaf self-shadowing. This
detail uses no texture, geometry, asset, or draw submission, does not alter authoritative terrain
queries, and reduces unresolved detail before mid-distance scenery. Broad substrate identity
does not fade with camera distance: humus, moss, clearings and paths remain beyond 285 m instead
of reverting to the legacy green vertex palette. Broad/medium value noise attenuates between
0.18 and 0.50 cells per pixel; the fine sinusoidal product additionally retains the 95--285 m
detail fade. The path edge widens with pixel footprint. Close detail and fog retain their separate
fades. The two broad noise fields can now execute beyond 285 m; this adds fragment arithmetic but
no resource, varying or submission. Continuous moving-camera appearance and physical-GPU cost
still need broader review.

The beach branch adds one close-only value-noise octave to break up otherwise flat dry sand. A
second bounded term follows the interpolated authoritative signed shore distance, so its broken
crest lines remain approximately parallel to the irregular water edge instead of using world-axis
stripes. Both terms fade with camera distance; the raked crests additionally stop 14 m inland.
They change only material colour and add no texture, mesh, draw submission, or collision detail.

Two deterministic grids add sparse dark-pebble and pale-shell flecks to exposed dry sand within
58 m. Each occupied cell hashes a jittered centre, orientation, and elliptical scale; a fixed
soft edge avoids derivative work and keeps the branch valid in GLSL 330 and GLSL ES 300. An early
dry-substrate branch avoids both grid evaluations outside that close dry-beach region. This layer
is material-only and remains separate from the larger instanced shell/pebble prop clusters.

Terrain, grass and low jungle layers share a static canopy transmission texture, replacing the earlier noise-only
floor-dapple approximation. `CanopyProfile` supplies the same eight crown lobes used by each
near tree mesh. `CanopyLightField` integrates forward sun-ray lengths through porous ellipsoids
at terrain-height texel centres; bounded projected caster rectangles avoid an all-pairs bake.
The 1024-square field covers 600 m and uses linear-clamped sampling. The GPU adapter uploads
one RGBA8 texture (4 MiB), lives longer than all receivers, and adds no draw submission.
Grass and low jungle plants project elevated samples back along the sun ray before lookup.
Jungle instances supply their ground-anchor height from the existing transform; the uniform
receiver switch is reset before crown draws every frame and enabled only for understory,
bushes and ground plants. The same shared GLSL helper keeps grass/plant projection consistent.
Ambient cool
skylight remains in full shade. This is approximate fixed canopy shading, not animated,
leaf-exact shadows: palms do not cast into it, and wildlife other than boars/monkeys does not yet receive it. Projecting
onto each low receiver's ground-anchor plane is approximate on sloping terrain. Crown
self-shadowing is deliberately excluded because a ground field cannot represent it correctly.
Material noise still describes soil variation, but no longer invents independent sun patches.

`CanopyLighting` also retains the immutable one-byte CPU field (1 MiB) for boar and monkey receivers.
`CanopyLightField::SampleReceiver` uses the same fixed-sun projection and bilinear texel mapping
as the GLSL helper. Boar mesh construction attenuates each already-lit vertex colour, preserving
alpha, curved-body shading and the existing BasicEffect single-batch path. The shared ambient
tint constants also feed the GPU uniform, preventing CPU/GPU tint drift. No extra GPU buffer,
shader, texture or draw is added; the CPU field owner outlives the boar renderer. This is still
a ground-plane approximation and triangle-interpolated light, not per-pixel shadow mapping.

Monkeys keep CNA's stock `SkinnedEffect`: a five-tap spatial average around each body's elevated
centre tints fur/face/eye diffuse colours, while direct specular strength follows transmission.
The footprint is scaled with the animal but independent of camera, LOD and animation pose. It
avoids a hard whole-body cell switch, but remains a uniform body-light approximation rather
than articulated per-limb shadows. Bone transforms, animation playback, textures and draw counts
are unchanged. The initial visible-scene diagnostic reports sampled transmission extrema.

Deeper inland, the shader also reuses those three fields to prevent the grass substrate from
remaining one uniform saturated green. A 24--54 m shoreline-distance transition,
slope rejection and clearing suppression bound a dry leaf-humus tint and a
separate cool-moss tint. The base substrate receives a restrained warm desaturation first, so the
two irregular classifications remain readable without looking like decals. This pass adds no
noise evaluation, texture, varying, mesh, allocation, collision rule, or draw submission, and the
same source compiles as GLSL 330 and GLSL ES 300.

The primary south-to-north walking corridor has one renderer-independent sinusoidal centre line,
range, and reserved half-width owned by `VegetationLayout`. Palm, jungle, prop, grass, QA-camera,
and terrain-material code consume that same definition rather than duplicating path coordinates.
The terrain shader receives the centre-line parameters as uniforms and reuses its
existing noise/detail fields to form a narrow broken edge, compacted centre, inland/endpoint fades,
and muted dry-earth colour. The already-computed medium and fine fields give the compacted earth
shaded/sun-dry mottling without adding another noise evaluation. The visible material is
deliberately narrower than the vegetation reservation. Short mesh grass remains on the route, so
it reads as a lightly worn tropical footpath rather than a bare road or objective marker. The cue
adds no geometry, texture, draw submission, or collision rule.

The same layout owns the exact centres and radii of the east and west clearings. Vegetation
reservation, F7/F8 teleports, fixed QA cameras, tests, and terrain rendering consume those shared
definitions. The shader evaluates only the relevant clearing for each side of the island and uses
squared distance plus existing noise to avoid a per-fragment square root while retaining a soft,
irregular edge. The larger east room receives a restrained warm, dry tint and reduced litter; the
smaller west room receives a darker leaf-rich tint and increased litter. Both retain short grass;
their lighting now follows actual crown placement. No clearing-specific texture, geometry, placement,
collision, instance, or draw submission is added. A later composition pass strengthened the east
blend and reuses both already-computed broad and medium fields so its warm olive identity remains
legible against surrounding forest soil without becoming a hard circular or sand-coloured patch.
Both rooms additionally classify warm/cool or leaf/moss patches from the already-computed broad,
medium, and fine fields. Four bounded colour blends make open ground less uniform without another
noise evaluation, texture, varying, geometry, or submission.

`DistantSceneryLayout` places five low islets at least 570 m from the island centre, outside every
player-accessible ocean boundary but within the 850 m ocean surface. `DistantSceneryRenderer`
combines their irregular rock ridges and 11 simplified trunk/frond/crown silhouettes into one
2,361-vertex, 787-triangle opaque `BasicEffect` batch. Fog from 260 to 790 m merges the muted
palette into the horizon; the later transparent ocean pass visually covers the low bases. The
layout is deterministic and renderer-independent, while the rendering path uses no texture,
custom shader, instancing requirement, or modern-only feature.

`ContactShadowRenderer` provides a deliberately bounded alternative to a full shadow map. It
precomputes twelve-segment soft ellipses for every palm, canopy trunk, land rock, fallen log,
and driftwood item, then generates the same form for the moving monkeys and boars.
Only frustum-visible instances inside class-specific 78--105 m ranges enter one premultiplied-alpha
`BasicEffect` batch. Alpha fades over the outer 22 percent of each range. Cached geometry is
refreshed after 0.25 m of camera motion, a small view/projection change, or 0.08 m of monkey motion;
this keeps close grounding responsive while avoiding repeated procedural terrain sampling and
per-frame buffer uploads at a nearly fixed view. The pass uses no textures, shadow maps, custom
shader, or renderer-specific feature.

The 201 land-rock/log/driftwood footprints use a compact twelve-sector, one-ring profile clipped
against the rendered terrain triangles, rather than sampling only an analytic-height fan. Their
20,304 vertices replace 7,236 old fan vertices (+209,088 bytes per packed vertex copy), with no
extra draw or texture and unchanged 78 m range, radii and opacity. Tree and animal fans still use
analytic heights. `WoodModelYaw` converts the placement/support/collision heading `(cos, sin)`
to CNA's model-matrix yaw `(cos, -sin)` at both wood rendering boundaries. Rock model yaw is
unchanged; its footprint follows the distinct negative-Z convention. This fixes a game-side
model/proxy disagreement without relocating props or changing the world fingerprint. Tests cover
every log collision sample, both heading signs, footprint area and terrain-plane conformity.

Coastal grounding adds 28 static patches (14 offshore rocks, two separate hull halves, 12 debris
pieces) to the same batch. `ContactShadowPatch` creates a 24-sector, three-ring soft footprint,
clips each source triangle against the actual `TerrainMeshLayout` cell triangles, and interpolates
their vertex heights with a 2.5 cm bias. Analytic terrain sampling alone would not match the rendered
coarse seabed. Separate hull ellipses retain a >2 m gap; model-local X for rocks and local Z for
hull/debris use their respective yaw conventions. Small fixed-sun biases improve grounding but
do not claim full directional shadows, tilt-exact silhouettes or water refraction.

The coastal addition is 23,970 vertices / 7,990 triangles (383,520 bytes per packed 16-byte vertex
copy). It adds no texture or draw submission. Static bounds are derived from actual patch geometry,
including projection offset and terrain slope, rather than the source object's pivot. One dynamic
GPU buffer is allocated after the exact static geometry size is known, with reserved monkey/boar
capacity; population and upload bounds are checked. Each vertex retains its own faded alpha, and
coastal range is 110 m before platform scaling. The pass precedes opaque props and transparent
water. `coastal_contact_shadows` in benchmark JSON and the runtime count identify this feature.

`SkyRenderer` draws a camera-centred 900 m sphere before world geometry. Its GLSL 330 / GLSL ES
300 fragment shader combines a horizon-to-zenith tropical gradient, warm sun disc and halo, and
subtle static high cloud wisps. Four trigonometric samples form elevation-dominant bands, a small
directional warp, and azimuth breakup; this keeps the sparse forms horizontally legible without a
noise texture, seam, animation state, or high-end shader feature. The sky has no clock or weather
input by design. Depth writes and
testing are disabled only for this pass and restored immediately afterward. A vertex-coloured
`BasicEffect` gradient remains available with an explicit diagnostic if custom effects are not
supported. Native OpenGL 3.3 and Chromium/WebGL 2 execute the full shader; the current arm64 GLES
3 package compiles its GLSL ES path, and the KVM/API 35 gfxstream matrix visibly executes the same
full sunny-sky path. Physical-device appearance and fragment cost remain open.

Four palm variants use curved, slender, root-flared trunk meshes with smooth analytic normals and
10--12 fronds in the near tier, with eight low rounded root ridges around the bole as a low-cost
contact cue. Their deterministic angle, length, width, and ridge-height variation avoids a regular
radial stamp. Each near frond combines a
thin curved rachis with nine pairs of independently tapered leaflets. Their modestly widened roots
close the skeletal gaps visible from first-person height without adding vertices or merging the
individual silhouettes; the lower-segment far tier uses seven
widened ribbon fronds that preserve the feathered crown envelope without retaining sub-pixel
leaflet geometry. Two near variants add four or five small faceted coconuts beneath the crown; the
near draw also enables texture-free leaf-scar rings on trunks, while the far draw disables that
fragment detail to avoid distant aliasing and unnecessary work. Two different near variants also
add two or three downward-curving dry rachises with incomplete ochre-brown leaflet remnants. This
keeps age variation sparse and asymmetric instead of placing an identical dead skirt on all four
shared silhouettes. The dry geometry adds 156 vertices and 82 triangles across the four stored
near meshes; it creates no buffer object, material, texture, instance field, or submission, and the
far meshes stay unchanged. These details reuse the existing shader and draw batches. A
deterministic 330-instance layout is grouped into the same
8 by 8 spatial grid as terrain. CNA hardware instancing reduces a typical overview from hundreds
of palm submissions to four far-variant draws; mixed first-person scenes remain bounded to eight
near/far variant draws.
Cells are rejected by frustum and a conservative distance bound, then each gathered palm selects
its LOD against a deterministic 125--145 m threshold derived from placement yaw. This replaces the
old whole-cell 210 m switch and staggers visible changes around the player. Capability-checked
per-palm drawing remains the functional fallback. The portable GLSL 330 / GLSL ES 300 shader bends
only frond-weighted vertices with per-frond phase variation, leaving roots and trunks stable in a
calm breeze. That vertex path also hashes four already-present scale/yaw transform coefficients
into a bounded per-instance colour seed. A vertex-colour green mask applies the tint only to
fronds, preserving brown trunks, roots, and coconuts. Because both LOD tiers reuse the same
transform, their colour identity cannot change at the threshold. This adds no instance attribute,
buffer, texture, draw, placement state, or collision state. Near fronds retain nine paired
single-triangle leaflet stations, but deterministic per-frond/station phases modestly vary their
length, longitudinal stagger, forward sweep, and droop. The broader silhouette and broken comb-like
symmetry therefore uses 526--607-triangle near meshes after the selected dry-frond additions. The
near-only fragment path
also reconstructs a two-sided geometric normal from world-position derivatives and applies a
restrained fixed-sun diffuse/fill term before the existing bark detail. Far palms skip the added
fragment work, while no normal attribute or buffer is required.

Grass uses 20,499 deterministic cluster anchors grouped into a finer 16 by 16 cell grid. Their acceptance follows `GroundCover::Density` (baked canopy transmission, a three-octave patch field, shore distance and slope), the same formula the terrain shader uses for its turf undercoat, so dense swards and bare humus coincide between placement and material (`docs/ground-cover-and-trunks.md`). Each
visible anchor instances one shared near cluster (42 tall/juvenile blades plus 48 low turf
blades), ten-blade middle, or seven-blade far cluster. A
stable per-patch ordering and renderer-independent smoothstep split progressively move each cell
from near to middle across 62--78 m and middle to far across 86--102 m. This avoids both the old
37.5 m whole-cell replacement and a direct extreme-mesh jump. Pure cells use one draw; a mixed
cell binds adjacent-tier ranges of the same instance buffer through two draws. No per-frame
instance upload is required. The vertex
shader reconstructs a terrain-aligned basis from the authoritative surface normal, applies
independent height/spread/colour/phase variation, and adds a bounded two-frequency breeze. The
near mesh interleaves seven shorter, narrower juvenile blades among 35 mature blades. Six selected
blades carry small seed-bearing stems with six sparse spikelets each. They use the same opaque
vertex format, per-blade phase, wind shader, shared mesh, and instanced submission
as the blades. The existing per-instance colour value also seeds a portable per-blade stature,
colour, and wind-amplitude offset, so repeated shared geometry retains distinct silhouettes
without another attribute or instance upload. The
near roots now form seven asymmetric local tufts with populations 8/5/7/4/6/7/5. Different
root spreads and outward blade headings avoid identical radial bouquets; short juvenile blades
reach slightly outside each core. A shared per-tuft wind phase has small per-blade offsets,
preserving coherent motion without synchronizing the whole patch. The renderer-independent
`NearGrassBladePlacement` supplies the same 42 tall/juvenile blades, maximum root radius below
1.02 m and unchanged five-level geometry. `NearGrassTurfBlade` distributes a separate layer of
48 short, spreading blades over an irregularly facing golden-angle disk with root radius below
1.10 m. Their nominal heights are 0.16--0.27 m before instance scale and terminal drop, with
three cross-sections/three triangles each, with a shared pointed tip. One quarter has a restrained dry palette; wind and
player-interaction weights scale with actual blade height rather than applying tall-blade tip
displacement to the low layer. This adds 240 vertices and 144 triangles to the near mesh only,
not another instance, buffer, attribute, texture or draw. Measured animation bounds include the
larger spreading footprint. Middle/far meshes retain their old placement. Tests cover
determinism, population, root separation, neighbour distance, phase bounds and sampled full-blade
extents, every low strip's shared normals and low-layer index rejection. Moving-camera LOD
continuity still needs targeted review. The
path keeps denser but shorter turf rather than an artificial bare stripe. Blades retain their leaf
material through distance changes instead of fading into a synthetic green ground palette at
82--125 m. Actual soil remains visible through gaps; this does not fix geometry LOD/cull changes.
Both custom and stock paths use terrain's atmospheric fog colour/range rather than green fake fog
(custom smoothstep versus stock linear fog). Each instance also carries one coherent cool-to-warm
biome value derived from shore distance and the shared route/clearing definitions. It makes coastal
and path turf slightly drier, keeps the jungle cooler, gives the sunny east clearing an olive-gold
identity, and makes the leaf-rich west clearing deeper and cooler. The vertex shader preserves
that identity at all ranges; the existing four-byte attribute does not change placement,
geometry, or placement counts. `GrassInteractionModel` eases a normalized envelope
from horizontal player speed. The vertex shader retains a small 0.55 m stationary body radius and
expands it to 1.35 m during fast walking, bending only blade tips outward and down before wind. The
interaction shares every existing instance and draw. Near blades use five curved control levels.

Custom grass has a restrained basal pigment over the bottom 24% of each blade, evaluated per
fragment so three- and five-level geometry do not stretch the colour transition differently.
The otherwise unused packed vertex-alpha byte carries normalized material height, not opacity;
the shader explicitly writes alpha one. A transient per-mesh height vector keeps wind weights
separate: low turf reaches material height one at its tip despite its reduced wind amplitude,
and seed heads remain at one. Upload checks the vector size and finite normalized range. The
surface vertex now occupies 40 bytes and uses nine attributes including a signed across-blade
coordinate; the instance remains 44 bytes. The across coordinate and existing stable blade phase
feed footprint-filtered longitudinal veins, midrib and margin modulation. Seed heads use a
separate tag and skip this detail. No texture or draw is added.
`GrassMaterial.hpp` supplies CPU reference, validated encoding and locale-independent GLSL
constants. Tests bound the three/five-level interpolated tint error below 0.0028 per channel.
Custom RGB retains pigment but no longer includes fixed side/height shading; actual normals
provide directional light and a separate normalized-height occlusion ranges from 0.62 to 1.
Stock-effect mesh colours remain unchanged; the static fallback does not get
the per-fragment basal pigment. Root colour is artistic material variation, not soil lighting
or a substitute for cast/contact shadows. Normal deformation and distant density remain limited.
See `grass-surface-material.md` for cost, filtering and validation scope.

`GrassMotionBounds` derives conservative per-anchor boxes from measured maximum radius/height
across all three uploaded meshes (including seed stems), instance scale and the shader's exact
terrain-normal reconstruction. It adds the full bounded player push/lowering, wind vector and
stature range, with a 1 cm numeric margin. The same constants generate locale-independent GLSL,
so changing an amplitude changes both motion and bounds. This replaces the insufficient fixed
x/z +/-1.4 m, y -0.08/+1.35 m box: even a sloped root could lie below the old minimum. Mesh
generation rejects nonfinite or below-ground input before upload. The bounds also include the
unanimated stock-effect fallback. Per-cell anchor bounds separately preserve the original LOD
and distance reference; enlarging the animation envelope must not shift those decisions. Bounds
are calculated only during setup, with no added GPU buffer, attribute or per-frame motion work.
More correctly intersecting edge cells can now be submitted; this is not a draw-count reduction.
CPU tests independently transform a containing cylinder through extreme slope, stature, scale,
wind and player displacement, and cover old-box escapes, invalid inputs and GLSL constant mapping.

The custom path now drapes each undeformed footprint onto the rendered terrain triangles before
adding blade height and motion. `RenderedTerrainSurface` supplies a one-time 241x241 RGB24 height
texture (232,324 RGBA8 bytes) through existing CNA APIs. Three `texelFetch` calls reconstruct a
triangle, not a bilinear patch; no new attributes, draws or dynamic uploads are needed. Culling
bounds additionally include all terrain grid heights covering each footprint. This fixes the
analytic-anchor/tangent-plane mismatch measured in [the grounding audit](grass-grounding.md).
The fallback remains approximate, and shading normals still approximate surface deformation.

Opt-in `--input-diagnostics` appends the previous rendered grass near/middle/far populations,
visible cell count and draw count to the existing player/camera samples. Canopy diagnostics
serialize the actual previous near/far instance batches using stable millimetre X/Z identities.
The parser rejects duplicate IDs and population mismatches; adjacent-sample comparisons ignore
frustum entries/exits and count only changed tiers on a shared identity. Whole-cell culling can
submit off-screen trees, and sampled states can miss intervening switches. These diagnostics
perform no GPU queries and do not log during normal play.
The software-only browser `grass-motion` scope uses these samples to drive real W/S traversal,
learn stopping lead, allow bounded corrective attempts, capture seven stationary stages and
reject lost input capture or invalid movement. Parser/control/review gates have CPU-only Node
tests. A capped 640x360 compositor recording adds at most 240 JPEGs/16 MiB with timestamps and
hashes; writes are acknowledged/drained and failures/truncation reported. Its telemetry
association is approximate, not same-frame synchronization. Per-tree switches prove workload
coverage, not smooth visual continuity; isolated full-resolution LOD comparisons remain needed.

The custom grass path uses area-weighted shared rest-shape normals from the same indexed mesh,
including folds, bent tips and seed stems. A 36-byte position/RGBA8/wind/normal vertex replaces
the earlier 24-byte vertex with alpha-packed horizontal facing. Including the new low layer,
the three meshes total 964 vertices (34,704 vertex bytes; low turf adds 8,640 vertex bytes
and 864 index bytes). World placement and draw counts are unchanged. Eight attributes include
the four unchanged instance attributes. Shader normals
use inverse-transpose height/spread/stature scaling, then yaw and the terrain basis; the fragment
shader renormalizes interpolation before two-sided lighting. Normal generation rejects nonfinite,
degenerate or cancelling geometry before upload. CPU tests compare a bent V-fold strip under 36
scale/yaw combinations with independently transformed triangles. Actual startup generates normals
for every LOD vertex. This corrects rest curvature but does not yet deform normals with wind/player
motion. The stock-effect path retains its original 24-byte unlit vertices.

`GrassBladeForTier` selects middle/far representatives from the same seven near tufts.
Roots, facing, source height/curvature, colour identity and wind phase are shared; all seven far
representatives also exist in the ten-blade middle tier. Near geometry stays unchanged. Proxy
widths retain 75% of each tuft's summed tall-blade upright area, assigning nearby source blades
to representatives within their tuft. This is a width/height trapezoidal estimate, not
view-independent projected coverage; it excludes low turf, seeds, lean and fold overlap.
Proxies use three levels without V-folds. Middle and far each cover all seven tufts, but blade
orientation, reduced curvature sampling and omitted low turf still cause visible LOD changes.
Mesh inventories are logged: near 862 vertices/640 triangles, middle 60/40, far 42/28.
Replacing the old five-blade far tier adds 12 vertices and eight triangles per far cluster
(432 shared custom vertex bytes and 48 index bytes), without additional draws or textures.

Near grass retains its 622-vertex/496-triangle tall/juvenile geometry, including six sparse seed
heads and 14 selected V-fold ridges. The low layer brings the full near cluster to 862 vertices
and 640 triangles. The near profile is narrower than the middle tier, offsets its two edges by a
small deterministic asymmetric amount, varies its lateral bow, and gives a minority of blades a
bent, slightly dropped tip. `GrassBladeProfile` samples a local cross-section shared by mesh
generation and CPU tests. Its near width narrows progressively through the upper half instead of
remaining broad until the final segment. The seed stem uses the same terminal centre/ridge,
including lateral bow, drop and fold; this removes the old 4--16 mm nominal-tip offset on five
of the six seeded blades. Middle/far silhouettes retain their previous width profile. Invalid or
overflowing inputs are rejected before GPU upload. These are shared-mesh position changes: counts, instance data, wind
work, LOD thresholds, and submissions remain unchanged. Middle clusters use three levels and 40
triangles, while the far tier uses three levels and 20 triangles.
The surface-normal attribute enables two-sided fixed-sun diffuse and subtle view-edge
transmission alongside folded side colour and root darkening. Native OpenGL 3.3 and WebGL 2 use the portable instanced shader; a
slower static `BasicEffect` path remains functional but omits bending and the fragment-lighting
detail.

The layered interior jungle adds 277 broadleaf trees (closed stands with a sparse scatter and 22 emergents, `docs/forest-stands.md`), 1,822 medium plants, 579 low bushes, and
3,116 small forest-floor plants. Fourteen shrubs and 64 small plants form local clearing-edge
colonies on top of the unchanged base populations. Each clearing tests twelve jittered angular
sectors, leaves deliberate gaps and varies radial offset. Accepted shrubs receive four to six
candidate companions, mostly one leaf family with occasional contrast, varied scale/yaw and
minimum root separation. Terrain and canopy-trunk checks may reject candidates; the hard limits
are 24 added shrubs and 144 floor plants. No extra mesh, material or draw batch is introduced.
Startup reports the added all-near/all-far triangle budgets from the actual current meshes,
not a claim that all are simultaneously visible. Existing per-instance culling/LOD still applies.

Canopy-associated forest colonies add another 866 small plants in 199 irregular
elliptical groups. Each parent supplies independently hashed geometry and bounded
candidate sites; smaller edge members and related species provide local variation.
Their full footprints avoid routes/trunks, and the earlier clearing-accent bands
remain unchanged. `ForestFloorColony` records parent and contiguous placement
membership for tests and reports, while rendering reuses normal ground instances.
See `forest-floor-colonies.md` for validation, measured submission cost and limitations.

Jungle rendering now copies each layout placement through `GroundJunglePlacement`,
anchoring its origin to the exact rendered triangle surface. A startup-only heap
height cache is released after cell construction. Instance matrices and culling
bounds consume the same corrected copy, while analytic layout/collision data stays
unchanged. This does not drape full plant footprints; see `jungle-root-contact.md`.

Near rosette litter now samples the existing RGB24 height texture per vertex,
preserving its fold above the drawn triangles instead of the parent's horizontal
plane. `RenderedTerrainHeightMap` owns that immutable texture once at application
scope; grass and jungle borrow it and bind slot 1. Its owner outlives both renderers.
Dry-litter tag 2 uses deformed geometric normals and expanded terrain-aware cell
bounds without changing live foliage. See `fallen-leaf-contact.md` for tests and cost.

`VegetationLayout::OverlapsReservedRoute` expands the two clearing circles and spawn circle by
the complete plant footprint. It conservatively expands the bent path using its maximum
derivative and both end caps; invalid input fails closed. The colony placement uses local radii
2.1 m for shrubs and 1.4 m for floor plants, including wind, multiplied by instance scale.
Renderer startup checks both actual LOD meshes against those envelopes. This new policy applies
to colonies, not retroactively to all historical plants. Three-seed CPU tests cover budgets,
locality, separation, canopy-trunk clearance and sampled footprints around path bends/ends.
`--viewpoint clearing-edge` supplies a fixed close review camera; the normal centre camera hides
most changes behind grass and is not sufficient quality evidence. The colony layout does not
yet test palm/rock/log overlap or produce full botanical habitat simulation.

Eight shared `CanopyProfile` forms now give trees spreading,
upright, swept, divided, broad-low, narrow-high, hanging and open-fork crowns, rather than mostly
scaling one umbrella silhouette. `CanopyFormCount` is shared by placement, geometry and leaf
posture tables. Each form
contains four structural groups, each split into two unequal lobes in both detail tiers. Both
tiers use identical lobe anchors/dimensions, core scales and irregular contour functions, with
coarser far-core tessellation. Near/far tiers retain 24/four of each lobe's 36 source sprig
identities and the same perimeter/lower leaves, including position, colour and wind identity. It no
longer merges pairs into oversized opaque masses or substitutes unrelated leaf shapes. Remaining
leaf removal and tessellation changes still affect silhouette/coverage; this is not seamless LOD.
The fixed-sun bake uses the same profiles, including the four added forms.

Both trunk LODs sample one bounded centreline, with identical lower knots at 0, 1.5 and 3 local
metres. The bottom 3 m stays straight; upper growth leans and bends across the lean direction.
Near meshes retain five-percent ring girth and six-and-a-half-percent azimuthal variation,
four/five buttresses and seven two-segment tapered support branches. Branch roots now sample the
actual trunk centreline at staggered heights instead of assuming the crown-tip X/Z position
below the tip. Tree placements, collision proxies, instance counts, buffer layout and draw
strategy are unchanged, but eight forms require more shared meshes and variant batches. Near
variants 0/2/4/6 use 4,620 vertices/4,328 triangles, variants 1/3 use 4,657/4,378, and variants
5/7 use 4,626/4,334. Every far variant uses 1,029 vertices/1,064 triangles with retained primary
branches. All jungle meshes total 2,526,004 custom vertex bytes; adding forms 4–7 costs
1,175,616 vertex bytes and 129,480 index bytes. One inspected jungle view increases from 28 to
35 jungle draws, with unchanged 65 visible trees (38 near/27 far). Physical-mobile cost is not
qualified. Measured near/far bounds include the fixed local wind envelope before scaling.

Each near cluster also
offsets successive shell rings sideways and
modulates their contour by ring and azimuth. Four alternating clusters append one smaller pointed
leaf below the existing perimeter leaf with stronger droop, adding 20 vertices and 16 triangles
per stored near variant. This breaks the former regular lower-crown line without adding a buffer,
instance field, material, or submission; coarse far cores with retained original leaves approximate
the crown volume without close silhouette detail. Near variants one and three append one shared
31-vertex/48-triangle tapered three-sided liana with two small pointed leaves. The top-to-bottom
wind weight gives the strand a restrained flexible response; no liana exists in the far LOD and
the thin decoration is intentionally non-solid.
A stable yaw-derived 105--129 m threshold selects LOD per tree after cell rejection, avoiding the
old 155 m whole-cell switch. Four understory variants build two pinnate fern and two broad-leaf
silhouettes. `UnderstoryLeafProfile` gives each actual leaf a stable identity,
golden-angle placement with angular jitter, age/stature-dependent length/width, independent lift,
short emerging leaves, a narrow petiole, side bow, terminal drop and restrained edge curl.
Root position remains fixed. Far meshes select five actual near profiles with identical angle,
phase, pigment and posture, rather than rebuilding an unrelated evenly spaced rosette. Surface
normals sample the complete deformed profile, including the petiole transition; ground plants
retain their previous profile. CPU tests check dense finite/above-ground/envelope samples, roots,
nested identities and unit normals perpendicular to independently sampled tangents.
Fern forms 0/2 attach seven pairs of folded leaflets to nine curved rachis knots, with centre
vertices sharing exact position/phase/weight at the attachment. Their planes follow the local
rachis tangent. The far tier retains four original pairs on each of five original fronds with
unchanged poses. Near variants use 776/405/970/495 vertices and 704/576/880/704 triangles;
fern far meshes use 335 vertices/320 triangles, broadleaf far meshes 30/20. No layout, attribute,
material or submission format changes. See `understory-ferns.md` for cost and visual evidence.
Broadleaf forms 1/3 now have separate paddle/lance width distributions and restrained asymmetric
margin undulation. The five near columns span both wings evenly; a rounded cross section replaces
the former wide planar wings around a very narrow midrib strip. `UnderstoryBladeShape::Narrow`
leaves the fern source curves unchanged. Both tiers sample the same profile; counts, buffers and
materials remain unchanged. See `broadleaf-lamina.md` for visual evidence and remaining limits.

Ground species 1 and 2 use renderer-independent `GroundBroadleaf` and `SplitGroundLeaf`
profiles. Both retain actual near-source identities at far LOD and feed the existing
52-byte jungle leaf vertex layout. Split leaves use one connected indexed surface,
with shared outline knots and root-anchored wind; near quarter-interval tessellation
retains curved shoulders that midpoint-only sampling misses. These are static mesh
changes, not new renderer APIs, per-frame uploads or individual-leaf draw calls. See
`ground-broadleaf.md` and `split-ground-leaves.md` for costs and validation scope.

`BushProfile` supplies three shrub forms with rooted shoots and individual blades.
Both LODs keep all leaf identities and their supporting attachment knots; leaf root
phase/weight matches its shoot. All seven leaf outline rows survive in far; only
the inner rib column is removed, and shoots use fewer radial sides.
Closed leaf endpoints are single vertices, avoiding degenerate triangles. The
existing jungle leaf/wood material, instance transforms, footprint and draw batches
are reused. See `shrub-lod-continuity.md` for measured mesh inventory and remaining limits.
The opt-in walking log records actual submitted canopy/shrub identities, not just
aggregate counts; submission does not prove image visibility. Other species' coarse
far sampling and omitted leaves still change silhouette/coverage. The four shared
understory variants remain visibly repetitive, not anatomically complete fern species.

Tagged leaf surfaces use a bounded two-sided daylight response: 0.48 fill, 0.44 sun-facing and
0.08 upward fill. This replaces their old dominant 0.78 fill without changing unresolved crown
response. Existing canopy transmission still applies afterward. Stable per-leaf phase selects
restrained warm/desaturated ageing only over the final 38% of a blade, independent of distance.
`JungleMaterial.hpp` supplies CPU references and locale-independent GLSL constants. Existing leaf
mottling is attenuated by a conservative UV phase-frequency bound; bark grain/fissures use
screen derivatives. Detail fades between 0.18 and 0.50 cycles/pixel. Unresolved bark fissures
converge to their numerically verified 0.29043848 cycle mean instead of brightening to zero.
This is a portable art-directed material model, not measured leaf optics or new cast shadows.
No textures, attributes or mesh changes are introduced. Stock fallback retains its prior colours.

Canopy wood uses material tag -1 in the existing surface coordinate; tag 1 remains a leaf and
tag 0 an unresolved/legacy surface. Trunk/branch/root stock vertices retain their historical
baked colours, while custom uploads select a separate raw bark albedo (including neutral trunk
ring variation). Custom wood lighting is 0.52 ambient, 0.40 positive outward-normal sun response,
and 0.08 positive upward fill, not absolute two-sided light. Branch/root inward triangle winding
is corrected; shared area-weighted normals are checked against radial branch/trunk directions
and semantic side/crest directions on open tapered roots. An averaged centroid is not a valid
interior reference for that non-convex open wedge. Invalid normals fail during mesh construction,
before upload. Geometry counts/positions, fallback RGB, 52-byte GPU layout and nine attributes
are unchanged; only temporary CPU surface metadata retains the separate wood colour.
Normals still approximate subtle wind deformation. Crown self-shadowing and cast branch shadows
are not introduced by this material pass.

Tagged wood reuses the three existing material floats for `(cos(angle), sin(angle), along)`.
Trunks use ring-local angle and shared local height; branches use their local radial frame and
cumulative distance through the bent centreline. Root wedges use a three-corner circular frame
and crest length. No seam vertices, extra attributes, textures or geometry are introduced.
The shader obtains angular derivatives from the circular pair, not wrapped `atan`, and bounds
the axial phase modulation before filtering the periodic grain/fissures. Root end caps can
interpolate through a zero-length circular pair: detail fades there and `atan(0,0)` is avoided.
Upload validation requires finite unit circular pairs and nonnegative axial coordinates. Tests
cover all strip side counts, branch-cut finite differences and singular/invalid input.
This follows each bent segment but is not an anatomically continuous bark atlas: branch/root
junctions have separate phases, trunk height approximates arc distance, and low-poly radial
interpolation is not uniform angular interpolation. Fine bark relief and anatomical knots remain
future visual work. Untagged stems and stock fallback preserve their old material behavior.

Trunk pigment no longer alternates by ring parity: near and far have different ring counts and
heights, so that generated different material bands at the LOD switch. Both paths now use
`JungleMaterial::TrunkPigment(t)`, a bounded linear 0.96→1.02 normalized-height factor applied
before custom albedo/stock daylight separation. Its interpolation is independent of ring
segmentation, apart from at most one byte/channel of colour quantization. CPU tests cover range,
invalid heights and interpolation, including the actual rounded-byte packing rule. Branch/root
colours and shader bark detail are unchanged; this does not remove geometric/normal LOD errors.

`BarkMaterial.hpp` now warps the grain with periodic smooth value noise, varies fissure strength
along the wood, and adds restrained plate-tone variation. Its integer lattice hash is shared
with a CPU reference; ES shaders explicitly use high-precision integers. Angular lattice indices
wrap using nonnegative remainders, avoiding the undefined negative-remainder behavior in
[GLSL ES 3.00](https://registry.khronos.org/OpenGL/specs/es/3.0/GLSL_ES_Specification_3.00.pdf).
Each layer uses a pixel-footprint fade, with a conservative smoothstep-slope bound for warping.
Unresolved noise tends to its statistical 0.5 reference and fissures to the existing area mean;
this is an art-directed filter, not an exact convolution of correlated material layers.
The scalar albedo multiplier is bounded to [0.76,1.08] before the existing 38–68 m detail fade.
The vertex shader packs a 0–1023 seed from the existing transform into the negative wood tag;
every vertex in a wood primitive gets the same value, decoded by rounding in the fragment shader.
Tree identity survives LOD and wind without a new varying, attribute, mesh or texture. Uploaded
wood tags still validate as -1. Leaves and legacy stems retain their existing interpretation.
This reduces identical straight stripes but does not provide geometric bark relief, continuous
junction phases, botanical species identity or measured mobile fragment cost.

`CanopyLeafPosture.hpp` gives each source sprig/member a deterministic pitch, roll and restrained
variant-dependent blade proportion. Upper/lower shell positions bias pitch; asymmetric rotation
breaks the prior uniformly horizontal blade axes. Perimeter and lower blades have explicit
identities 36/37. The outline pass retains 24 of the original 36 near sprigs (skipping identities
congruent to 2 modulo 3), with a nested far subset 3/12/21/30. Poses still use original identities,
never reduced-list indices. `PointedLeafPositions` preserves the legacy five-vertex/four-triangle
bush/liana crease. Canopy-only `CanopyBladePositions` adds two outline vertices and a six-triangle
fan around the crease: shoulders at 30% length, taper at 72%, root/crease/tip unchanged. Side rows
have shallow lift and quadratic gravity droop. The same orthonormal frame handles pitch/roll.
One retained shoot (identity 30) has two members in alternate lobes, balancing the perimeter
blades' added triangles. Total canopy leaf triangles remain 3,504 near / 624 far, with 584 / 104
blades instead of 876 / 156. The outline pass produced 4,620 or 4,657 near / 945 far vertices,
with 4,328 or 4,378 near / 952 far triangles. The subsequent far-branch correction retains all
seven primary supports with identical geometry, albedo, normals and wind in both tiers: far now
has 1,029 vertices / 1,064 triangles. Jungle vertex payload is 1,350,388 bytes, including the
17,472 added shared far-branch vertex bytes (plus 2,688 index bytes). This measured silhouette
tradeoff adds no draws/textures and leaves near meshes unchanged. Intermediate leaf vertices have
bounded existing-range wind weights; normals are reconstructed from the actual posed vertices.
Runtime culling continues to measure actual near/far mesh extents, union both tiers and add the
shared wind margins. Shared crown anchors and coarse light-bake ellipsoids stay unchanged: those
proxies approximate canopy transmission, not individual leaf orientation/occlusion. CPU tests
cover every source identity, frame orthogonality, nondegenerate posed triangles, local extents,
default-shape preservation, nested source identity selection and exact triangle budgets.
This trades leaf count for shape, not guaranteed equal silhouette coverage. Large low-poly
blades, simple branch hierarchy, crown-core facets and four repeated overall forms still limit
realism. Static interior/clearing views and the frozen pair inform acceptance; broader moving
and unobscured LOD review remains open.

Three shrub variants use 45/54/63 leaves on supported shoots; both tiers retain all
source leaves with shared outline vertices. Three small forest-floor variants break
up grass-heavy ground at first-person range. Variant 0 uses seven short curved opaque
leaves; variant 1 uses juvenile broad leaves and petioles, while variant 2 has connected
lobed blades. Their current profiles, retained far subsets and geometry costs are
documented in `ground-broadleaf.md` and `split-ground-leaves.md`.
Placement uses
independent broad/fine cluster fields plus terrain height, slope, and shoreline constraints while
preserving the winding route, two clearings, and beach spawn. A shared cell AABB first rejects
irrelevant regions; actual instance distance then enforces maximum range and selects near/far LOD
for canopy, understory, bushes, and forest-floor plants. Canopy additionally rejects individual
world AABBs outside CNA's frustum. The boxes combine measured near/far mesh radii/Y extents,
local wind margins, positive uniform instance scale and a 0.01 m world rounding cushion. The
horizontal radial envelope is invariant under yaw. Only canopy instances store these boxes;
the same boxes extend cell bounds. The primitive is CNA's existing `BoundingFrustum::Contains`,
not a parallel game-side frustum implementation. This is not occlusion culling: terrain and
foreground trees can still hide submitted geometry. Visible density/LOD distances are unchanged.
First-draw diagnostics and benchmark JSON expose per-tree rejection counts.

Instances remain gathered into variant batches. A frozen jungle comparison reduces submitted
canopy from 109 to 65 (with one forced far target: 62/47 near/far -> 37/28), preserving all image
bytes, 462 understory plants, 108 bushes, 162 ground plants and 28 jungle submissions. Frozen
east/grass comparisons also preserve all pixels; see `canopy-culling.md` for scope and evidence.
A portable GLSL
330 / GLSL ES 300 shader applies weighted calm wind. `JungleWind` supplies shared CPU/GLSL
amplitudes, static displacement-envelope checks and locale-independent shader constants. Mesh
upload validates finite wind inputs and weights in [0,1]; the motion itself is unchanged.
Its fragment stage uses smooth normals for
marked curved/pointed leaves and reconstructs a safe geometric normal for other geometry. It uses
the absolute fixed-sun response for two-sided foliage and adds a small upward fill. A brown-material mask adds local-space
longitudinal grain and narrow fissures to nearby canopy bark, fading between 38 and 68 m before
the far tier. The vertex stage derives a stable bounded tint from each existing instance
transform's scale/yaw coefficients and applies it only to green materials. Near/far batches reuse
the same transform, so colour identity does not pop with LOD; no instance field, buffer, texture,
placement value, or submission was added by the instance-tint pass. The subsequent close-leaf pass
uses a game-owned 52-byte standard-layout vertex (position, packed color, wind, normal, leaf UV/
variation/mask) through CNA's generic `VertexBuffer::SetData<T>` and declaration APIs. Five vertex
attributes precede four instance columns: nine total, within the portable target feature set.
The current 50,123 static jungle vertices use 2,606,396 bytes in the 52-byte format. The eight-form
expansion adds variant batches, but keeps declarations, texture inventory and instance layout.
Curved blade normals come from the CPU-tested surface derivatives, including tapered endpoints;
pointed leaves average their adjacent face normals. Placements use uniform scale/yaw, allowing
normal transformation by their existing matrix. Normal deformation under the subtle wind is
approximate, not a complete deformed-surface derivative.

Leaf coordinates drive a narrow midrib, angled secondary veins, restrained albedo mottling and
a broad waxy reflection. Pixel-footprint filtering and a 16–38 m fade bound aliasing; distant
fragments skip this detail. The old broad painted midrib stripe was reduced. Opaque alpha remains
one. Positions and custom leaf fields are validated before upload, and size/standard-layout assertions guard
the transfer format. `BasicEffect` remains the explicit unlit fallback using the unchanged stock
vertex packing rather than interpreting the custom material layout. Historical native OpenGL 3.3,
Chromium/Firefox WebGL 2 and KVM/gfxstream GLES 3 runs qualified the previous shader and layered
near-crown geometry. Their fixed jungle frames were inspected, but those observations predate the GPU incident
and the new leaf material. Current leaf-material qualification is tracked separately in PLAN.md;
moving LOD/wind behaviour and physical-mobile cost remain open.

Natural props add 157 irregular rocks across beach/interior bands, 14 larger emergent outcrops on
the 1.8--9.4 m offshore shelf, 13 mossy fallen logs in the low-slope interior, 31 small
sun-bleached driftwood pieces, 63 fallen palm fronds around suitable groves, and 265
shell/pebble/dry-wrack clusters in the dry beach belt. Four rock, three log, three driftwood,
three frond, and four beach-detail variants use cell culling, variant batching, and a portable
static GLSL 330 / GLSL ES 300 shader; the larger classes retain independent near/far meshes while
tiny beach details disappear beyond 52 m and fronds beyond 76 m. One beach-detail variant uses
four low curved ribbons and two small holdfast forms for dry wrack instead of repeating another
shell/pebble composition. Opaque geometry grounds the
layers without transparency overdraw. Driftwood variants 0 and 1 use an irregular bent tube plus
one or two branch stubs; near variant 2 uses three curved, six-sided tapered fork limbs for a
distinct 160-triangle silhouette while its far tier stays at 30 triangles. Each frond remains a
132-vertex/74-triangle opaque mesh with a curved rachis and 28 tapered leaflets. The leaf roots
follow the rachis curve, while bounded pair/side variation changes span, forward sweep, tip droop,
and ageing colour so the close silhouette is not a mirrored comb. Each existing root-to-tip
diagonal is raised into a shallow central fold; a private material-selector value enables
two-sided fixed-sun facet response only for these fronds without another vertex, normal stream, or
submission. Placement validates complete footprints against shore distance, route reservations,
nearby trees, slope, and support-height variation.
The otherwise-unused natural-prop vertex alpha is a private material selector. It marks rock
fragments for shared-normal fixed-sun lighting, filtered mineral noise/lichen and a low sea-level wet band;
near-log sides for local-space bark fissures; near cut ends for gently irregular growth rings and
three thin, slightly curved radial drying cracks; and near-driftwood sides/branches for softer
sun-bleached longitudinal weathering. Driftwood cut ends remain plain. Distance fades remove all
cues before their mesh transition. Contact geometry retains the plain path; far wood retains
normal-based lighting without near material detail. This keeps materials texture-free and free
of extra instance data or draws. Near fallen logs reuse their existing nine-sided end rings and cap
vertices, matching their radii and applying bounded deterministic axial offsets so the former
planar circular cuts become spatially closed irregular broken contours. Vertex/index counts,
material selection, instances, collision proxies, far LOD, and submissions are unchanged. The near
rocks use `RockSurface`'s ten rings and 24 angular samples, with an asymmetric cap and
area-weighted shared-vertex normals. Including the basal disk, each near variant has 290 vertices
and 528 triangles (formerly 110/192); far variants retain 44 vertices and 70 triangles. Both stone
tiers use albedo rather than baked local-space lighting in the custom path, with an inverse-transpose
normal transform specialized to the existing orthogonal scale/yaw instance matrices. Fine mineral
grain fades over 55--90 m and is derivative-footprint filtered; broad mottling and lighting remain
in the far tier. The vertex shader also
uses the existing relative neutral colour span to select stone-family vertices and recombines four existing
scale/yaw transform coefficients into a stable bounded warm/cool tint. This applies consistently
to near and far rock meshes without another attribute, buffer, texture, geometry, upload, or draw;
the colour-span mask leaves red-dominant wood and green foliage unchanged.

The packed custom prop vertex is now 28 bytes (position, RGBA8 material colour, normal), with
seven attributes including the unchanged four instance columns. The total static prop vertex
inventory is 2,718 vertices / 76,104 bytes (+44,136 bytes versus the old layout/geometry); added
shared rock index data costs 8,064 bytes. No new texture or draw is required. A shader-unavailable
fallback uploads stock 16-byte position/colour vertices with CPU-lit stone instead. CPU tests
validate all eight rock meshes, finite/outward/unit normals, independent area-weight accumulation,
nondegenerate faces, upload indices, yaw/nonuniform-scale normal orthogonality and the shared
1.23-height culling bound. The previous 1.2 bound could exclude the tallest cap tip.
Logs and driftwood now use `SurfaceNormals` to accumulate triangle-area-weighted normals from
their existing shared topology. Separate end-cap vertices retain the hard rim; formerly inward
cap indices are corrected, with startup assertions checking outward cap normals for all twelve
wood meshes. CPU tests cover weighting, winding reversal, duplicated seams, all eight independent
rock normal references, unused/cancelling vertices, degenerate triangles and invalid input.
Custom wood uploads unlit albedo and uses the same scale-correct fixed-sun response as stone;
both vertex and fragment normals are normalized. Far wood remains lit while near bark/grain fades.
Decorative moss/contact quads retain zero normals and their previous colours. The stock fallback
retains its prelit approximation. No wood topology, vertex stride, texture, draw or placement count
is added. Fork geometry still intersects at branch roots, and this is diffuse shading rather than
new bark relief or a physically based specular model.
World-position derivatives execute before material/distance branching; filtered noise receives
an explicit footprint, including a conservative factor of 11 for the warped mineral coordinates.
This keeps derivatives defined at the detail cutoff while allowing the two finer noise evaluations
to be skipped in the distance.
The shared prop-cell AABB is only a conservative range rejection. Rocks, fallen logs, and
driftwood then enforce maximum range and select near/far geometry from their actual instance
position, preventing early switches at cell boundaries. Three non-solid driftwood pieces use the
three existing silhouettes to form an authored arc at the shared `-2.35` radian cove transect.
Their centres and complete footprints remain inside the dry-beach belt, outside reserved routes,
and separated from trunks and other driftwood. The driftwood QA camera frames the group centroid
from inland and exposes its near/far population.
Collision is limited to substantial rocks, every visibly emergent reef outcrop, and sampled
fallen-log spans, leaving small dressing non-solid. The player applies those horizontal proxies
while both walking and swimming. Native OpenGL 3.3 and Chromium/WebGL 2 execute the full instanced
path; dedicated prop, driftwood, reef, and frond cameras provide deterministic close review. Five
of the land rocks and two fallen logs are deliberately authored in irregular groups around the
outer rings of the two shared clearing definitions. Each clearing has one log, and the east log
balances the established west composition. They use the same geometry, rendering, collision, and
culling paths as procedural props, while invariant tests keep every landmark inside its clearing
and outside the central walking room.

The first wildlife renderer draws 24 procedural birds distributed across shore, canopy, and high
route families. `BirdFlock` combines soft radial steering with per-agent slow radius drift, two
incommensurate wander terms, individual speeds, vertical oscillation, flap phases, smoothly
entered intermittent glide windows, banked turns, and terrain-derived minimum clearance.
Observer distance selects full, quarter-rate, or one-thirtieth-rate simulation while accumulated
time preserves average motion. `BirdRenderer` frustum/distance-culls individual agents, selects a
closed near mesh or double-sided far silhouette, applies CPU wing and bank poses, and streams all
visible triangles through one CNA `DynamicVertexBuffer` draw. The near mesh uses its complete
preallocated 54-vertex budget: paired double-sided trailing-feather triangles extend the existing
wing roots/tips and share the same flap pose, while the 12-vertex far silhouette is unchanged. The
portable `BasicEffect` path has no custom shader or modern-only feature. Native OpenGL 3.3, actual
Chromium/WebGL 2, and KVM/gfxstream GLES 3 execute and visually match the dedicated bird capture.

Eighteen deterministic butterflies occupy three sunny clearing and jungle-edge habitats.
`ButterflyMeadow` alternates asynchronous `Rest`, `Flutter`, and `NectarHover` states, combines
bounded turn wander with slow altitude variation, and applies only a mild lift/retreat response to
a nearby observer. Every candidate pose is revalidated against authoritative coastal/interior
ground, a minimum 28 m signed-shore distance, maximum slope, individual home radius, and a narrow
terrain-relative altitude envelope. The shared wildlife activation policy reduces or suspends
distant updates without changing the population. `ButterflyRenderer` frustum/distance-culls each
agent, chooses a hinged patterned near model or compact far silhouette, and streams all visible
double-sided opaque triangles through one `DynamicVertexBuffer`/`BasicEffect` draw. Five palettes
and individual phase/scale variation break repetition; the largest procedural wingspan is roughly
0.27 m. Native OpenGL 3.3, Chrome/Firefox WebGL 2, and KVM/gfxstream GLES 3 dedicated captures
pass; the Android view reports eleven visible agents, seven near and four far, in one draw.

Five deterministic sea turtles occupy separate bounded near-shore route regions. `SeaTurtleGroup`
alternates asynchronous `Cruise`, `Forage`, and `Surface` states, steers slowly within each home
range, and applies mild separation from a nearby player. Every proposed pose uses the authoritative
shore and seabed fields: the route remains 7--38 m offshore, requires at least 0.72 m water depth,
and clamps each animal between seabed clearance and the surface. This keeps turtles out of land,
the immediate breaking edge, and the player's 42 m offshore-current onset. The shared wildlife
activation policy reduces or suspends distant updates without changing the five-agent population.
`SeaTurtleRenderer` selects a 204-vertex near model with a faceted scute shell, head, eyes, tail,
and four articulated flippers or a 24-vertex far silhouette. Three palettes reuse the aquatic
depth/sun-band colour model, and every visible animal streams through one portable
`DynamicVertexBuffer`/`BasicEffect` batch before the transparent ocean. Dedicated OpenGL 3.3,
Chrome/Firefox WebGL 2, and KVM/gfxstream GLES 3 captures pass with one near turtle in one draw.

Eight deterministic shallow-water schools contain six fish each. Their route centres follow the
authoritative irregular shoreline instead of a circular approximation; each route searches outward
for sufficient seabed depth. Stable local formation offsets supply separation/cohesion while
individual steering, speed, depth phase, and tail phase prevent rigid synchronized motion. Every
proposed move must remain 3--48 m offshore on submerged terrain, and each vertical pose is clamped
between a seabed clearance and the surface. Smoothed vertical velocity supplies bounded pitch and
heading error supplies bounded turn banking, so schools follow depth changes without remaining
mechanically level. School-level activation uses the same shared wildlife
tiers, so only nearby water receives full-rate updates. `FishRenderer` selects near articulated
body/fin geometry or a far silhouette, gives both tiers a forked tail, adds eyes and gill marks only
to the near tier, and applies fixed-sun CPU face colours to the closed near body plus two-sided
lighting to the flat far silhouette. One shared CPU sample per visible animal smoothly tints the
palette with depth and adds two restrained moving shallow-sun bands before facet lighting. Because
the batch
uses `CullNone`, each thin fin/tail surface is emitted once instead of duplicating an exactly
coincident reverse-winding triangle. Near/far geometry is therefore 51/12 vertices per fish rather
than 66/24. The renderer frustum/distance-culls agents and streams all visible fish
in one `DynamicVertexBuffer` draw before the transparent ocean. Native OpenGL 3.3 and actual
Chromium/WebGL 2 dedicated captures match; the fish-inclusive arm64 GLES 3 APK builds but has not
run on a stable Android target.

`SharkPatrol` adds one non-hostile offshore animal without turning it into a threat or interaction
system. A deterministic schedule alternates roughly 42--66 second visible patrols with longer
92--160 second deep-diving absences. During a patrol, bounded radial steering keeps the shark
34--88 m offshore and searches for seabed no shallower than -3.5 m; a slow depth cycle, body wave,
tail stroke, vertical-velocity pitch, and heading-error bank prevent rigid motion. The shared
observer-distance policy reduces or suspends
updates when appropriate. `SharkRenderer` uses a recognizable original low-poly silhouette,
forked tail in both LODs, near eye/gill accents, fixed-sun CPU face colours, the same depth/sun-band
profile as the fish, individual
frustum/distance rejection, and one portable `BasicEffect` dynamic
batch. The shared `WildlifeRenderLighting` helper keeps fish and shark facet treatment identical.
Because culling is disabled for the thin fins, they are emitted once rather than as coincident
reverse-winding copies; near/far geometry is 51/15 rather than 66/30 vertices. The batch renders
before the ocean. Native OpenGL 3.3 and actual Chromium/WebGL 2 dedicated captures match;
the shark-inclusive arm64 GLES 3 APK builds but awaits stable-device runtime validation.

Seven original procedural monkeys exercise CNA's actual skeletal-animation path without an
external asset or license dependency. `MonkeyTroop` owns deterministic interior homes and a compact
`Idle`, `Observe`, `Wander`, `Walk`, and `RunAway` state machine. Authoritative terrain queries keep
every proposed move at least 48 m inland on supported low-slope ground, while nearby-player
awareness produces a brief non-hostile retreat. The shared wildlife activation policy throttles
distant decisions. `MonkeyRenderer` generates a fourteen-bone hierarchy, three pairs of near/far
silhouette meshes, four coordinated body/face palettes, and idle, observe, walk, and run clips; one
CNA `AnimationPlayer` per agent supplies palettes to the portable `SkinnedEffect`. Near monkeys add
a light upper-face mask plus muzzle/ear detail in one pass and surface-visible eye/nose geometry in
a second. Dark eyes use a restrained glossy response. Three small tetrahedral crown/cheek tufts
break up the near head silhouette in the existing body pass and follow the head bone. They add 27
vertices and nine triangles to each shared near body variant; the shared face/eye buffers add 86
vertices and 122 triangles per visible near monkey. Each near body variant also adds three short
rounded fingers per hand and three toes per foot, rigidly following the existing forearm and shin
bones. This adds 216 vertices and 240 triangles to the selected near body mesh. No bone, palette,
material, draw, or far-LOD geometry is added, and far monkeys remain one draw each. Mesh
selection does not otherwise change the per-agent draw count. Calm idle/observe states derive a
distance-faded horizontal look angle toward a player 7.5--30 m away, approach it at a bounded rate,
and clamp it to 0.34 radians. The renderer composes that angle into the existing head-bone local
transform and rebuilds the same fourteen-matrix skin palette; locomotion relaxes the look and a
closer player still causes the established non-hostile retreat. The troop copies the player's palm,
broadleaf, solid-rock, and
fallen-log circular proxies once at world construction. Spawn, target, and movement queries retain
a 0.32 m body clearance; blocked agents try bounded alternating lateral headings and then choose a
new state rather than walking through geometry. Native OpenGL 3.3 and actual Chromium/WebGL 2
captures match, and an accelerated Android x86_64/GLES 3 first-frame run constructs the same
monkey-inclusive scene; the arm64 APK builds and passes app lint. State changes
snapshot the last presented local pose and use a 0.14--0.26 second eased blend: translation and
scale interpolate linearly, rotation uses quaternion slerp, and the game recomposes world and
inverse-bind skin palettes. `Wander` and `Walk` share one continuous locomotion phase. More detailed
surface treatment, calls, long observation of the attention timing, and physical-device animation
observation remain open.

The procedural monkey path validates its data before the first GPU upload. The skeleton must have
exactly fourteen consistently sized bind, inverse-bind, hierarchy, and optional root-prefix slots;
parents must precede children; all four clips must target the joint palette and contain finite,
ordered, in-range tracks; and every generated triangle index must address an existing 16-bit mesh
vertex. Development builds additionally assert every crossfade input and output palette size.
Renderer-independent player and wildlife inputs fail closed on non-finite values, with development
assertions guarding persistent finite pose, range, population, and animation invariants.

All game-generated indexed rendering uses the same renderer-independent triangle-mesh gate before
allocating a GPU buffer. It accepts the complete 0--65,535 portable index range and rejects empty
geometry, partial triangles, excessive vertex/index counts, and references outside the supplied
vertex array with a subsystem-labelled exception. This covers terrain, water, sky, grass, palms,
jungle plants, natural props, and every monkey mesh. It runs once during content construction, not
in the frame loop, and its valid and malformed boundaries are host-tested without a graphics device.

The ocean uses a runtime-selected regular grid drawn after opaque geometry: 160 by 160 quads for
Linux/Web and 128 by 128 quads for the Android mobile profile. Both grids cover the same 850 m
half-extent and pass the shared unsigned 16-bit index validation. `OceanRenderer` supplies explicit
GLSL 330 and GLSL ES 300 variants through CNA's public portable shader-package extension. The
shader combines vertex waves, analytic fragment normals, Fresnel sky response, sun glint,
depth-driven tropical colour/transparency, moving foam, haze, two thin movement-driven ripple
bands, and a speed-scaled pair of broken entry/exit rings centred on the player. The two
crossing-wave values already used to break up broad foam also
apply restrained bright/dark subsurface colour in the non-reflective surface component. The colour
detail favours shallow water and fades from 135 to 420 m so it reads in exploration views without
aliasing across the horizon or adding another trigonometric field. A bounded shallow-ridge cue
reuses the length of the broad five-wave analytic slope. It is limited
to the non-reflective, shallow, near/middle-distance surface and therefore adds no new sine,
texture, varying, geometry, or draw. A second narrow cue projects that same slope toward the fixed
sun, windows only the sun-facing signed range, and fades from 85 to 245 m. This gives nearby calm
water short directional crests while leaving the reflective horizon quiet, again without another
wave field or resource. A third cue projects the complete normal slope toward the horizontal eye direction,
smoothly saturates the signed result, and applies it only at grazing angles inside the existing
135--420 m detail fade. The shallow-weighted cyan reflection bands break up broad low-angle water
without another wave evaluation, texture, varying, geometry, resource, or draw. The ripple state
eases from
authoritative depth/speed input, wraps
deterministically, uses subtle spatial breakup, and skips its fragment work while inactive. A
separate finite transition phase expands and fades its broken double crest, then retires fully;
both effects share the existing ocean submission. A stock `BasicEffect` fallback keeps the scene operable with
a clear diagnostic if the custom effect cannot be created, but intentionally omits the shader-only
ripple or transition burst.

`OceanWaveSpectrum` is the single coefficient source for the ocean and shoreline displacement
functions and the ocean normal shader, emitted with locale-independent GLSL numbers. The first
three waves retain the original 0.193 m summed displacement envelope. Two intermediate waves and
four short waves affect normals only, with no extra geometry, texture or draw. The short trains
are bent by a slowly moving analytic coordinate distortion; its determinant remains above 0.7,
and the chain-rule gradient is included in both the shading normal and per-wave screen filter.
The summed conservative slope bound stays below 0.21 (about 12 degrees). Each wave smoothly
attenuates between 1.1 and 2.6 radians per pixel, using projected phase derivatives on both pixel
axes. This avoids evaluating fine detail as distant high-contrast noise. Broad foam/crest colour
cues retain the broad slope, rather than turning the added ripples into large whitecaps.
Grazing Fresnel reflection also raises opacity, without removing downward-view shallow transparency.
The CPU reference tests 22,326 space/time samples against independent numerical gradients,
displacement/slope/speed bounds, filtering and invalid inputs; actual shader compilation and visual
checks are separate runtime gates. This is not a physically simulated spectrum or a scene-reflection
solution: reflections still approximate the fixed sky, not the island silhouette.

The four normal-only short waves have separately phased slow amplitude envelopes in [0.30,1.00],
using sine/cosine angle addition to reuse the existing warp phases. Exact slopes include both
the carrier gradient and the envelope gradient (the product rule); merely multiplying a normal
would be incorrect. Their filter includes carrier +/- both envelope sidebands. The two shortest
directions are spread to about 70/115 degrees instead of duplicating the longer short waves.
Displacement stays identical. The conservative slope bound, including packet gradients, remains
below 0.21. This adds arithmetic and four carrier sine evaluations, not geometry, textures,
uniforms or draws; it does not add envelope trigonometric evaluations in the GPU shader.
Independent double-precision height differences validate the CPU slopes over 22,326 samples,
along with time continuity, complete quiet/strong packet ranges and unchanged calm-water limits.
The result is still a small analytic spectrum, not a simulated ocean or a finished realism pass.
Fresnel coverage and its alpha floor use the five broad normal waves, approximating an integrated
rough-surface response; the complete packet normal still determines reflected sky direction and
sun glint. A software flat-normal A/B established that pointwise short-normal Fresnel was turning
wave families into opaque cyan crosshatching. The final path does not flatten or remove the fine
normal waves. This is an inexpensive approximation, not a microfacet integration or scene reflection.

`CoastalSurfaceAppearance` uploads one additional vec4 for bounded supplementary reflection and
calm breaker coverage. Reflection uses `s/(0.18+abs(s))` rather than the old 0.010 denominator;
its small-slope gain drops from 100 to about 5.56 without changing actual normal waves or Fresnel
sky reflection. Broad breaker coverage decays over 4 m of shore distance and fades with depth
from 0.35 to 1.25 m. The separate advancing shore wash and player ripple/splash remain intact.
Crossing colour phases and foam continuity now use the same 1.1--2.6 radians/pixel filter as
normal waves. Derivatives are evaluated before player branches. CPU tests cover 10,001 slopes,
15,000 depth/distance cases and invalid inputs; no geometry, texture or draw is added. These
changes limit artificial contrast, but do not eliminate the visible regularity of the short-wave
trains. Phase/direction organization still needs visual improvement, especially at swimming height.

Earlier pre-incident native, browser and KVM Android runs covered the older transition-burst,
sunward-crest and grazing-reflection paths. They do not qualify the updated spectrum or post-incident
AMD hardware. Current feature validation uses llvmpipe and SwiftShader plus Android cross-builds;
physical mobile-GPU profiling and current Android runtime verification remain open.

`CoastalMaterial` centralizes the continuous sand palette and bounded clear-water tuning. Seabed
colour no longer pre-tints the terrain turquoise or jumps at mean sea level. Close sand noise
extends underwater; restrained shore-aligned seabed ripples fade with pixel footprint, and the
landward wet band uses height with small existing-noise variation. The ocean's original two-float
attribute now carries bed height and signed shore distance, not precomputed nonlinear shallow/foam
weights. Fragment depth is `max(surfaceHeight - interpolatedBedHeight, 0)`: lagoon colour weight
decays exponentially, body opacity rises toward 0.92, and coverage fades over the first 0.12 m.
The existing Fresnel opacity and foam remain, multiplied by contact coverage. The four optical
parameters come from the same constants exercised by CPU tests and are uploaded in one vec4.
This is vertical-depth artistic attenuation, not refracted ray traversal or a full absorption model.
It reuses the coarse ocean grid's bed samples, so local depth is approximate and does not replace
authoritative terrain/collision queries. No depth texture, additional render target or pass is used.

`RuntimeQualityProfile` is the single game-owned quality-policy boundary. Linux and Web select the
full profile; Android selects the mobile profile at compile time. The profile requests bounded
4x/2x MSAA through CNA's `PreparingDeviceSettings` extension, logs the applied count, and permits
explicit device-limited fallback. CNA retains ownership of framebuffer allocation/resolve;
that extra storage is separate from the generated texture budget below. See `edge-antialiasing.md`.
Renderer constructors receive
explicit values rather than consulting platform macros: grass culls beyond 112 m instead of
135 m, contact-shadow distances scale to 72%, wildlife maximum draw distances scale to 82%, and
the ocean uses 128 rather than 160 quads per axis. Near grass retains the same 42-blade tuft mesh
plus 48 low turf blades on every target;
wildlife populations and simulations remain intact, and every visual shader path is shared. The
profile also enforces the 6,383,412-byte runtime texture payload against a 6,403,628-byte
ceiling (the original 160 KiB allowance plus a 4 MiB fixed-sun canopy field, 64 KiB boar coat,
232,324-byte shared terrain height texture, 1,398,100-byte full-mip floor material and
349,524-byte live-leaf material) and preserves
the bounded six-source soundscape. The static canopy field currently shades terrain, grass and
low jungle layers, boars and monkeys, and approximates porous crowns rather than individual leaves.
Boars additionally use a deterministic 128x128 RGBA8 coat atlas (64 KiB). `BoarCoatTexture`
generates integer-only grayscale longitudinal fibers; body UVs span texel centers and duplicate
the circumferential seam. Eight white rows isolate unpatterned anatomy from linear filtering.
The atlas is opaque, single-level, LinearClamp, shared by all coats and near/far meshes, and owned
longer than the BasicEffect that references it. `VertexPositionColorTexture` adds two UV floats
per vertex to the existing 36,000-vertex bounded batch; no extra geometry or draw pass is added.
This is albedo detail, not simulated fur, normal mapping, or a substitute for anatomical polish.
`BoarPose` shares the 11-ring body profile between the mesh and near-only bristle roots.
It also provides a small analytic two-segment leg pose: a 64% local-ground stance and sinusoidal
raised return, fixed segment lengths, phase offsets for four feet, and finite resting fallbacks.
The renderer builds connected tapered limbs and split near hooves in the same bounded batch.
`BoarGroundPose` now anchors the body to rendered terrain and independently fits each sole centre
against the exact terrain triangles. Body orientation retains the smooth analytic normal, avoiding
triangle-normal snaps. An 18-step bounded solve follows body-up (including its horizontal motion),
then fixed-length limb IK fits the contact plus existing swing lift. Both LODs consume the same
pose. A private 232,324-byte CPU height cache avoids repeated procedural height evaluation; no
GPU resource, vertex count, texture byte or draw count is added. Unreachable targets preserve
segment lengths and set a flag; rendering logs a one-time warning, rejected by software gates.
This fits the sole centre, not every curved toe-surface vertex, and is not planted world-space
foot locking. Stride synchronization and turning/sliding review remain future animation work.
Wildlife behavior, collision dimensions and population are unchanged.
Invalid profiles or resource-budget
overruns fail during content creation with actionable diagnostics.

`ShorelineRenderer` adds one low-cost transparent pass after the ocean. Its 384-segment, six-strip
ribbon is generated from `IslandTerrain::ShoreRadius`, spans 2.5 m landward to 5.5 m offshore, and
therefore follows the authoritative coastline independently of the ocean grid's 10.625 m spacing.
Explicit GLSL 330 / GLSL ES 300 stages reuse the ocean's three vertex waves. A roughly 8.7-second
cycle advances a narrow crest shoreward and fades it before restarting, with along-coast phase and
width variation. Trailing wash is on the seaward side of the advancing front. A nine-neighbor
procedural cell field produces joined foam rims and transparent interiors without a texture.
Derivative-based footprint filtering merges unresolved cells toward their mean coverage; the
crest edge is filtered separately. Phase variation is periodic around the closed coast.
Terrain depth naturally clips the landward edge; a
premultiplied vertex-colour `BasicEffect` ribbon remains as the logged fallback. The full layer is
2,695 vertices, 4,608 triangles, one transparent draw submission, and no texture or external asset.
This is an artistic surf model, not a fluid simulation or terrain-wetting solver. The ocean and
ribbon still interpolate their shared vertex waves on different meshes; exact surface conformity
and mobile GPU fragment cost require further hardware qualification after the incident is resolved.

The game requests the XNA `HiDef` profile because its deterministic QA capture uses back-buffer
readback and the intended 3D feature set exceeds a `Reach`-only design. This is an application
requirement, not a CNA workaround.

The developer overlay creates its 5 by 7 uppercase glyph atlas at runtime, so it has no font-file
or content-pipeline dependency. It measures wall-clock spacing between `Draw` calls instead of
reporting the fixed simulation timestep. F1 toggles it during interactive play; `--debug-overlay`
is the explicit automation path. Native OpenGL 3.3 and actual WebGL 2 captures validate the same
SpriteBatch route.

`WorldIdentity` is the renderer-independent owner of the diagnostic seed set and its stable text
representations. It obtains each value from the corresponding layout's public `DefaultSeed`, so
the F1 rows, F4 clipboard block, core tests, and `island-world-report` cannot drift into separate
manually maintained copies. Clipboard access remains an application-edge concern in `IslandGame`;
the deterministic core does not depend on CNA or a window system.

`WorldDebugRenderer` is the F2-only 3D companion to the panel. It uses `BasicEffect` vertex-colour
line lists rather than custom shaders: shoreline/current contours are depth-aware, while local
collision, cell, player, current-vector, and wildlife markers use an x-ray pass so vegetation does
not conceal diagnostic state. Six additional depth-aware contours use the authoritative soundscape
crossfade distances and terrain height. X-ray markers show the nearest non-dormant bird/monkey
audio candidates and the exact pooled positional emitter while a call is active. Distance gates
limit local marker generation. The renderer also consumes the same public nine-point unit offsets
used by `PlayerMotion`, resamples their authoritative terrain height/slope/normal, rings the points
that resolve the player's support height, and colours over-limit slopes red. `PlayerMotion` also
retains its latest nonzero attempted step, resolved endpoint, obstacle/terrain rejection counts,
axis-slide count, and safety-clamp flag; F2 draws both endpoints at exact world scale.
`FishSchools` exposes one compact centre/tier snapshot per school, while monkey state carries its
authoritative home and active target and shark state carries its current route target. `BirdFlock`
exposes one immutable authored route/spawn record per agent plus three derived family envelopes.
F2 uses those simulation-owned values for route markers, live radial targets, spawn headings, and
terrain-following monkey home ranges rather than reconstructing them from rendered poses. The
complete view still costs two draw submissions.
The `--viewpoint contact --world-debug` combination provides an offset deterministic review
camera, the movement viewpoint reproduces obstacle rejection, and the monkey viewpoint provides a
repeatable navigation-guide capture in addition to the overview.

The opt-in benchmark path remains game-owned and does not add renderer hooks. `IslandGame` records
wall-clock intervals between consecutive `Draw` entries after a fixed warm-up, while the
renderer-independent `PerformanceStatistics` component validates samples and calculates stable
interpolated percentiles plus exact 120/60/30 Hz frame-budget miss counts. Fixed benchmarks select
an existing QA camera. The traversal benchmark
samples a renderer-independent, open Catmull-Rom rail at one deterministic position per measured
frame, resolves camera height through `PlayerMotion`, and crosses shallow water, beach, the main
trail, both clearings, dense jungle, and the opposite coast. All benchmarks disable VSync and the
fixed timestep, omit debug presentation, aggregate scene-count ranges, serialize JSON, and exit.
Ordinary play retains its fixed-timestep/VSync configuration.

Startup timing is likewise game-owned and opt-in only for file output. `IslandGame` timestamps its
constructor entry, completed deterministic world construction, `LoadContent` entry/exit, first
`Draw` entry, and first `Draw` exit. The schema-1 report therefore distinguishes world generation,
framework/device setup, GPU-backed content creation, pre-draw scheduling, and game-owned draw
submission without reaching into CNA. CNA calls `EndDraw` and presents only after the override
returns, so the report deliberately does not call its last phase present or GPU-completion time.
Every renderer emits the concise first-draw timing diagnostic; native OpenGL JSON has an ordered
producer/parser CTest and the actual-browser WebGL harness gates the portable log path.

The native-only `island-world-report` target links only `IslandSimulator::Core`. It reconstructs
terrain, deterministic layouts, the combined collision grid, and initial wildlife twice, validates
their invariants, and emits ordered JSON plus a platform-local FNV-1a fingerprint over sampled
terrain and generated world data. `tools/validate_assets.py` separately checks the versioned asset
manifest, generator ownership, attribution rows, external-file hashes, and the rule that every
file under `assets/` is declared. Neither content-audit path enters WebAssembly or Android packages.

The native-only `island-audio-report` target likewise links only the core. `AudioAnalysis` scans
the exact generated PCM without copying it and records normalized DC offset, RMS energy, peak,
loop-seam and internal steps, zero-crossing rate, duration, and a deterministic payload
fingerprint. The tool validates all 16 sources and can serialize them as ordinary mono PCM16 WAV
files for disposable listening review. Neither the report nor those previews enter a game package.

## Audio baseline

`SoundscapeModel` supplies original deterministic 22.05 kHz mono PCM16 source material at startup.
Circular filtering keeps the procedural noise continuous at loop boundaries; peak
normalization retains mixing headroom. Surf is strongest along the beach and while swimming, then
falls to a quiet distant bed inland. Light canopy breeze increases toward the interior. A 1.8
second exponential crossfade prevents zone snaps, and M applies a reversible zero-gain mute without
restarting the tracks.

Three deterministic pairs of mismatched slow oscillations apply only downward gain drift to surf,
breeze, and jungle after the authored zone target. Their roughly 44--131 second component periods
keep a stationary mix from becoming a fixed wall of sound without adding PCM, voices, allocations,
or startup work. The drift depths are capped at 3.5, 7.5, and 5.5 percent respectively and reset
to unity, so existing maximum volume budgets remain unchanged and zone crossfades still prevent
hard edges.

The five generated beds use 10, 11, 29, 7, and 13 second loop lengths. Their combined sample-aligned
pattern therefore does not recur for 290,290 seconds (over 80 hours). Only four play concurrently:
the local stream and player-water loops exchange one voice according to proximity and swimming.
The longer jungle source deliberately spends a modest additional 749,700 PCM bytes on reducing the most
recognizable local repetition rather than adding another simultaneous source.

Before PCM16 quantization, each generated loop bed measures and subtracts its floating-point
mean. The quantizer then derives peak normalization from the centred samples in a second bounded
pass, adding no intermediate allocation or PCM payload. One-shots deliberately retain their
separate path so their explicitly silent first and last samples cannot be shifted away from zero.

A twenty-nine-second daytime-jungle loop adds broad filtered forest texture and sixteen short,
low-level insect phrases in irregular singles and pairs placed away from the loop boundary. It is
silent through the beach belt, reaches a conservative 0.078 gain deep inland, falls to eight
percent of that target while swimming, and uses a slower 2.6-second fade so the biome does not
sound like an audio trigger volume.

A seven-second water-motion loop follows the player's authoritative support depth, locomotion mode,
horizontal velocity, and normalized offshore-current strength. Dry movement and calm stationary
water produce zero gain; shallow wading and active swimming fade in with different conservative
pitch ranges. Past the 42 m current threshold, the same loop supplies a slowly deepening undertow
bed capped at 0.08 gain, so the safety feedback does not consume a fifth persistent source. One
persistent loop avoids the rapid repeated one-shot cadence common to footstep-style water effects,
while 140 ms attack, 340 ms release, and 650 ms current-cue smoothing prevent hard edges and
state-transition chatter. All current procedural
PCM payloads total 4,315,184 bytes (about 4.12 MiB) before backend bookkeeping, including the
wildlife phrases described below. `Soundscape::Start` generates, converts, uploads, and releases
one source at a time. It therefore does not retain all 4,315,184 PCM bytes and a second
4,315,184-byte conversion copy simultaneously; the largest game-owned staging pair is the jungle
bed at 2,557,800 bytes. Backend-owned decoded audio remains unchanged.

Two genuinely different 1.5-second splash textures handle crossings between dry and shallow-water
states. The detector uses 7.5 cm entry and 2.5 cm exit thresholds, ignores stationary state
changes, and applies a 1.6-second cooldown longer than either sample, so shoreline jitter cannot
overlap or machine-gun the effect. Entry and exit select a source and apply different bounded
pitch/pan/gain variation. A transition may overlap a stronger-wave accent, bounding the present mix
to four persistent loops and at most two short one-shots.

Four original 0.3-second mono footsteps form paired dry-sand and interior-soil families. A
renderer-independent scheduler measures authoritative resolved listener travel, interpolates a
2.05--2.75 m stride across walk/run speed, and rejects blocked motion, swimming, non-finite input,
and teleport-sized discontinuities. Shore distance at the emitted step selects sand within the
18 m beach belt and soil inland. Alternating pan plus bounded pitch/gain variation and two source
variants reduce repetition without allocating or adding a persistent voice.

Two original four-second PCM washes with different noise, filtering, envelope, and crest shapes
supply occasional stronger waves. A deterministic 7--13 second initial delay and 14--32 second
repeat interval select a source and vary conservative pitch/stereo pan without allowing overlaps.
The scheduler virtualizes the accent while inland surf is inaudible, so it contributes at most one
short one-shot to the broader bounded source budget.

Two short bird phrases and two restrained monkey contact phrases are synthesized from deterministic
oscillators, envelopes, harmonics, and filtered breath texture. A shared scheduler selects the
nearest non-dormant real agent, requires the bird to be within 110 m or the land-based monkey to be
within 78 m, and uses CNA `AudioListener`, `AudioEmitter`, and `SoundEffectInstance::Apply3D` for
view-relative stereo placement and inverse-distance falloff. Bird intervals are 13--30 seconds;
the less frequent monkey phrases use 27--50 seconds. Swimming suppresses monkey calls and softens
bird gain.

Wildlife uses one explicitly owned voice rather than one source per animal. It only starts while
the wave and water-transition slots are idle and may share the transient budget with one footstep.
A later wave may share the second transient slot; a water crossing pre-empts wildlife, wave, then
footstep as needed to preserve its feedback. Footsteps defer when both other transient slots are
occupied. Four persistent loops plus at most two transients therefore remain the hard
simultaneous-source ceiling. Muting stops every transient voice, and a positional-call or footstep
failure disables only that optional layer without silencing the ambient beds.

`Soundscape` owns CNA `SoundEffect` plus every looping and transient `SoundEffectInstance`; M mute
therefore silences the loops and immediately stops any active wave, splash, footstep, or wildlife
call. It
never calls SDL or a platform audio API. Linux and Android start after content initialization. Web defers PCM/audio
device creation until the first mouse, touch, or movement-key gesture so browser activation and
pointer-lock requests share one user action. Missing or disconnected audio degrades to a logged,
visible `UNAVAILABLE` state rather than terminating rendering. Core tests cover deterministic PCM,
length/rate, headroom, audible energy, loop-boundary discontinuity, zone ordering, crossfade,
daytime-jungle gating, water depth/speed gating and smoothing, transition hysteresis/cooldown,
wave-accent interval/virtualization bounds, land-footstep signal/cadence/surface/discontinuity
bounds, wildlife PCM headroom/silent endpoints, habitat gates, voice-slot holding, sparse
frequency, parameter bounds, and variant reachability. Tests also
require each source pair to contain different PCM. A 72,000-frame accelerated model regression
cycles representative zones for a simulated twenty minutes while checking finite bounds, event
budgets, crossings, and variant reachability.
The renderer-independent control path also fails closed on non-finite runtime input: invalid time
does not advance fades or schedulers, invalid depth/speed/current produces a finite calm mix,
invalid shoreline state cannot fabricate a crossing, and invalid listener/source coordinates
cannot request a positional call. The next valid update resumes from uncorrupted state; this guard
does not alter the generated PCM or ordinary finite-input mix.
Native playback started all four loops, prepared all effects, accepted a scheduled stronger-wave
one-shot, and played a positional bird call through the real Linux device. Actual Chromium proves
first-gesture mixer/track/effect initialization including the wildlife resources, and a
deterministic shoreline traversal from `(0, -225)` to `(0, -251.332)` successfully played the
transition effect; a visible hardware-browser listening pass and Android audio focus/lifecycle
testing remain open.

The Web release package remains a deployment concern rather than a runtime asset subsystem.
`tools/package_web_release.sh` copies the Emscripten HTML, JavaScript, and WebAssembly outputs,
produces deterministic gzip siblings, verifies decompression and SHA-256 identity, and writes a
small manifest for host configuration. No packaging code or precompressed payload is linked into
the game, and the uncompressed artifacts remain available for hosts that do not negotiate gzip.

The native Linux release follows the same separation. The `linux-release` preset reuses the
workspace-approved `build-consumer/` tree and creates an
optimized OpenGL 3.3 build, while `tools/package_linux_release.sh` performs a component install
into a staging directory, strips the game and `libcna.so`, verifies the `$ORIGIN/lib` runpath,
records fixed-file SHA-256/size metadata, and creates a deterministic archive. The bundle includes
the licences and notices for the CNA, Sharp Runtime, EasyGL, and MetaGL code linked into the CNA
runtime. SDL 3, SDL_mixer 3, the C/C++ runtimes, and the graphics driver remain declared host
dependencies; the project does not copy distribution-specific system libraries into the archive.

## Current limitations

- Terrain has shared-buffer cell culling and contiguous visible-run batching but no distance LOD
  or final biome masks. Close forest-floor detail uses the documented generated PNG windows.
- Palms have four geometry variants, segmented near leaflets, widened far crown silhouettes,
  staggered per-instance LOD, placement clustering, cell culling, subtle shader wind, trunk
  collision, low rounded root ridges, transform-derived green-only per-instance frond tint,
  and a near-field terrain contact shadow. True LOD
  blending, richer bark/frond materials, and full directional/self-shadowing remain open.
- The first grass, layered-jungle, rock, and fallen-wood tiers substantially improve first-person
  grounding, and bushes now replace solid lobes with supported shoots and individual curved leaves,
  while rocks have smooth normals, filtered mineral/lichen detail and a low sea-level wet band, near logs have
  bark/growth rings, and driftwood has subtle faded grain. Transition popping, richer close-up prop
  shapes, bush diversity, biome variation, and measured mobile density still need repeated polish.
  Broader beach-prop variety remains.
- The first distant-scenery layer provides five fogged low islets and 11 simplified palms in one
  portable batch. Its silhouette variety is deliberately restrained; unified world haze, richer
  mid-distance composition, and final horizon balance remain open.
- Collision uses a static 16 m spatial grid over every palm, canopy-trunk, solid-rock, and sampled
  log proxy. Exact narrow-phase circle checks preserve the original result, and placement performs
  bounded deterministic recovery from an overlapping static proxy. Dynamic obstacles, geometric
  step-up, and continuous depenetration remain absent.
- Swimming camera height, restrained opt-out camera motion, movement-responsive water audio, and a
  subtle depth/current-responsive near-surface depth/edge veil and surface scatter are functional.
  The same normalized
  current now deepens the existing water loop into a bounded undertow cue. Moving wet/dry
  crossings add one bounded broken-ring surface burst. Full underwater fog, geometric waterline,
  refraction, and particle splash presentation remain open.
- WebGL 2 has run in Chromium/SwiftShader with the full sky, ocean, palm, jungle, prop, and grass
  shaders.
  Automated browser input covers pointer lock, focus, keyboard traversal, relative look, actual
  Escape release without session termination, delayed click reacquisition, responsive 16:9
  presentation, and first-gesture CNA audio startup. Audible
  hardware-browser quality, extended tab suspension/audio focus, the remaining Firefox scene/input
  matrix, and visible hardware-GPU performance remain unverified. Firefox ESR 140.15 now independently passes
  the narrower production first-frame, pointer-lock/reacquisition, Shift, movement/footstep,
  audio-start, Escape-release, and responsive-shell contract through its built-in WebDriver BiDi
  endpoint. It also qualifies one actual second-tab activation through hidden/visible transitions,
  resumed animation and fresh game input, then qualifies dedicated jungle and near-surface-water
  scenes with live-state gates.
- Android arm64-v8a and x86_64 APKs build and application-owned Android lint passes. The API 35
  x86_64 AVD now runs with KVM and host-GPU gfxstream; the complete GLES3 scene, independent
  movement/look gestures, same-process background/foreground recovery, and AAudio pause/resume are
  validated. The game-owned activity can pass a string-array QA command line into SDL only for a
  debuggable package; release packages ignore it. This has produced a six-view beach, grass,
  jungle, water, monkey, and reef GLES3 comparison matrix against the matching WebGL 2 captures,
  plus dedicated fixed bird, updated palm, folded-frond, radial-log-end, and liana-inclusive jungle
  qualification frames.
  Simultaneous-gesture ergonomics, audio-focus interruption, sustained performance,
  thermals, and physical-device behaviour remain unverified.
- First flying-bird, shallow-water fish, occasional offshore-shark, and GPU-skinned monkey layers
  exist, and birds/monkeys share a bounded positional call voice. The first F2 world-space view
  now includes the exact terrain footprint/normals, ambient zones, wildlife audio candidates,
  active-emitter marking, fish-school centres, shark and monkey targets, monkey home ranges, and
  source-budget counts. Its bird layer also shows all three route-family volumes, the exact 24
  authored spawn headings, and nearby live radial targets; avoidance proposals, individual
  renderer culling/LOD layers, and configuration UI remain absent. Eight
  direct QA teleports and deterministic
  bird/fish/shark/monkey close-review cameras are available.
