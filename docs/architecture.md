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
      ├── DistantSceneryLayout ─ deterministic unreachable horizon-islet composition
      │       │
      │       └── DistantSceneryRenderer ─ fogged low-poly rocks/palms in one static batch
      ├── AquaticLightingModel ─ portable shared depth/sun-band colour profile
      ├── BirdFlock ───────── deterministic shore/canopy/high flight and simulation tiers
      │       │
      │       └── BirdRenderer ─ CPU wing pose, near/far meshes, culling, one dynamic batch
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
      ├── WorldIdentity ────── shared procedural-layout seeds and stable diagnostic text
      ├── Soundscape ───────── CNA loop ownership, volume application, mute, and failure fallback
      └── IslandGame ───────── CNA lifecycle, input, camera, and pass ordering
```

`IslandTerrain`, `PlayerMotion`, `DebugFreeCamera`, `FirstPersonInput`, `FirstPersonCameraMotion`, `WaterViewModel`, `VegetationLayout`, `GrassLayout`,
`JungleLayout`,
`NaturalPropLayout`, `DistantSceneryLayout`, `BirdFlock`, `FishSchools`, `AquaticLightingModel`,
`SharkPatrol`, `MonkeyTroop`,
`WildlifeActivationPolicy`, `ProceduralMeshValidation`, `TouchControls`, and
`SoundscapeModel`, `AudioAnalysis`, and `WorldIdentity` form `IslandSimulator::Core` and
deliberately do not depend on CNA. They compile and test quickly without a graphics device.
`IslandGame` maps CNA keyboard snapshots into `FirstPersonDigitalInput`; the renderer-independent
resolver combines them with touch axes and camera yaw using the screen-right basis required by the
view convention. It converts the core player position back to CNA `Vector3` for the camera. Collision and rendering
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
small textures, and PCM audio are generated deterministically during startup. Source-side asset
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
   proposed capsule footprint exactly against nearby trunk, land/reef rock, and log circular
   obstacles while retaining axis sliding when one component is valid.
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
8. `SoundscapeModel` derives surf/breeze/jungle targets and player-water gain/pitch/undertow from
   authoritative shoreline, support-depth, locomotion, velocity, and current state; `Soundscape` applies smoothed changes
   to four persistent CNA loop instances and schedules bounded wave/water accents, resolved-travel
   sand/soil footsteps, plus one pooled positional bird/monkey call voice from nearby wildlife
   state.
9. `BirdFlock`, `FishSchools`, `SharkPatrol`, and `MonkeyTroop` update steering, terrain/depth
   constraints, behaviour/animation state, and observer-relative full/reduced/dormant cadence
   independently of their renderers.
10. `IslandGame::Draw` submits the camera-centred sky with depth disabled, restores normal depth
   state, then submits opaque terrain and distant scenery. `ContactShadowRenderer` overlays its
   depth-read terrain grounding batch before natural props, broadleaf jungle, mesh grass, palms,
   the one-batch procedural birds, shallow-water fish, the occasional offshore shark, GPU-skinned
   monkeys, and transparent ocean using one view/projection pair. Aquatic wildlife is submitted
   before water so ocean transparency colours and reveals it. The near-surface veil follows the
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

The portable terrain material reconstructs broad, medium, and fine world-space variation from
the same interpolated shore-distance and slope fields on GLSL 330 and GLSL ES 300. Inside the
jungle belt, a 24--68 m camera fade bounds an additional microvariation octave and two sparse
deterministic leaf-litter grids. Each litter cell derives position, scale, bend, orientation, and
presence from one hash; pointed curved silhouettes replace the rejected first oval pass. This
detail uses no texture, geometry, asset, or draw submission, does not alter authoritative terrain
queries, and disappears before it can alias into mid-distance scenery.

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

The same material reuses its already-computed broad, medium, and fine fields for a restrained
fixed-sun floor-dapple pass. It begins 34 m inland, fades in through 68 m, rejects fully steep
ground, and shares the existing 95--285 m detail fade. A coherent early branch avoids the extra
colour shaping on shore, distant, and fully steep fragments. The effect is static material
breakup suited to the permanent daylight art direction, not a claim of geometric canopy shadowing.

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
smaller west room receives a darker leaf-rich tint and increased litter. Both suppress the static
canopy dapple inside their open centres and retain short grass. No texture, geometry, placement,
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
precomputes twelve-segment terrain-conforming soft ellipses for every palm, canopy trunk, land
rock, fallen log, and driftwood item, then generates the same form for the seven moving monkeys.
Only frustum-visible instances inside class-specific 78--105 m ranges enter one premultiplied-alpha
`BasicEffect` batch. Alpha fades over the outer 22 percent of each range. Cached geometry is
refreshed after 0.25 m of camera motion, a small view/projection change, or 0.08 m of monkey motion;
this keeps close grounding responsive while avoiding repeated procedural terrain sampling and
per-frame buffer uploads at a nearly fixed view. The pass uses no textures, shadow maps, custom
shader, or renderer-specific feature.

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

Four palm variants use curved tapered trunk meshes and 10--12 fronds in the near tier, with five
faceted root wedges around the trunk base as a low-cost contact cue. Their deterministic angle,
length, width, and ridge-height variation avoids a regular radial stamp. Each near frond combines a
thin curved rachis with nine pairs of independently tapered leaflets. Their modestly widened roots
close the skeletal gaps visible from first-person height without adding vertices or merging the
individual silhouettes; the lower-segment far tier uses seven
widened ribbon fronds that preserve the feathered crown envelope without retaining sub-pixel
leaflet geometry. Two near variants add four or five small faceted coconuts beneath the crown; the
near draw also enables texture-free leaf-scar rings on trunks, while the far draw disables that
fragment detail to avoid distant aliasing and unnecessary work. These details reuse the existing
geometry, shader, and draw batches. A deterministic 330-instance layout is grouped into the same
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
symmetry therefore reuse the existing 494--566-triangle near meshes. The near-only fragment path
also reconstructs a two-sided geometric normal from world-position derivatives and applies a
restrained fixed-sun diffuse/fill term before the existing bark detail. Far palms skip the added
fragment work, while no normal attribute or buffer is required.

Grass uses 18,556 deterministic cluster anchors grouped into a finer 16 by 16 cell grid. Each
visible anchor instances one shared 34-blade near, ten-blade middle, or five-blade far cluster. A
stable per-patch ordering and renderer-independent smoothstep split progressively move each cell
from near to middle across 62--78 m and middle to far across 86--102 m. This avoids both the old
37.5 m whole-cell replacement and a direct extreme-mesh jump. Pure cells use one draw; a mixed
cell binds adjacent-tier ranges of the same instance buffer through two draws. No per-frame
instance upload is required. The vertex
shader reconstructs a terrain-aligned basis from the authoritative surface normal, applies
independent height/spread/colour/phase variation, and adds a bounded two-frequency breeze. The
near mesh interleaves five shorter, narrower juvenile blades among 29 mature blades. Four mature
blades carry small seed-bearing stems with six sparse spikelets each. They use the same opaque
vertex format, packed facing, per-blade phase, wind shader, shared mesh, and instanced submission
as the blades. The existing per-instance colour value also seeds a portable per-blade stature,
colour, and wind-amplitude offset, so repeated shared geometry retains distinct silhouettes
without another attribute or instance upload. The
path keeps denser but shorter turf rather than an artificial bare stripe. Distance colouring merges
the far tier into the ground before culling. Each instance also carries one coherent cool-to-warm
biome value derived from shore distance and the shared route/clearing definitions. It makes coastal
and path turf slightly drier, keeps the jungle cooler, gives the sunny east clearing an olive-gold
identity, and makes the leaf-rich west clearing deeper and cooler. The fragment shader preserves
that identity in its far-ground merge; the extra four-byte attribute does not change placement,
geometry, or placement counts. `GrassInteractionModel` eases a normalized envelope
from horizontal player speed. The vertex shader retains a small 0.55 m stationary body radius and
expands it to 1.35 m during fast walking, bending only blade tips outward and down before wind. The
interaction shares every existing instance and draw. Near blades use five curved control levels;
their four sparse seed heads bring the shared cluster to 428 vertices and 304 triangles. Middle
clusters use three levels and 40 triangles, while the far tier uses three levels and 20 triangles.
Blade
orientation is packed into the otherwise-unused vertex alpha channel, avoiding another buffer
attribute while enabling two-sided fixed-sun diffuse, subtle view-edge transmission, folded side
colour, and root darkening. Native OpenGL 3.3 and WebGL 2 use the portable instanced shader; a
slower static `BasicEffect` path remains functional but omits bending and the fragment-lighting
detail.

The layered interior jungle adds 281 broadleaf trees, 1,822 medium plants, 565 low bushes, and
2,225 small forest-floor plants. Four
canopy variants combine tapered trunks with irregularly scalloped crown clusters. Their
800/806-triangle near meshes add four/five short irregular buttress roots, seven tapered support
branches, and eight pointed perimeter leaves; rounded 208-triangle far meshes preserve the crown
volume without the close silhouette detail.
A stable yaw-derived 105--129 m threshold selects LOD per tree after cell rejection, avoiding the
old 155 m whole-cell switch. Four understory variants
build fern and broad-leaf silhouettes from curved opaque strips. Three bush variants combine
offset low lobes with two folded pointed leaves per lobe in the near tier to bridge the visual gap
between grass and medium plants. Their unchanged 48-triangle far tier keeps only the compact
volume. Near variants contain 320, 400, or 480 triangles without adding a submission. Three small
forest-floor variants break up grass-heavy ground at first-person range. Variant 0 uses seven
short curved opaque leaves. Near variant 1 uses six visible petioles and folded upright broad
leaves in 36 triangles; near variant 2 uses five low leaves with solid midribs and three separated,
folded lobe pairs per leaf in 140 triangles. Both retain their unchanged 16-triangle far forms.
Placement uses
independent broad/fine cluster fields plus terrain height, slope, and shoreline constraints while
preserving the winding route, two clearings, and beach spawn. A shared cell AABB first rejects
irrelevant regions; actual instance distance then enforces maximum range and selects near/far LOD
for canopy, understory, bushes, and forest-floor plants. Instances remain gathered into variant
batches. The deterministic jungle camera renders 63 near and 46 far canopy trees, 462 understory
plants, 108 bushes, and 162 ground plants in 28 jungle submissions. A portable GLSL
330 / GLSL ES 300 shader applies weighted calm wind. Its fragment stage reconstructs a safe
geometric face normal from world-position derivatives, uses the absolute fixed-sun response for
two-sided foliage, and adds a small upward fill. A brown-material mask adds local-space
longitudinal grain and narrow fissures to nearby canopy bark, fading between 38 and 68 m before
the far tier. The vertex stage derives a stable bounded tint from each existing instance
transform's scale/yaw coefficients and applies it only to green materials. Near/far batches reuse
the same transform, so colour identity does not pop with LOD; no instance field, buffer, texture,
placement value, or submission was added. These cues give the deliberately low-poly crowns and
leaves coherent facet depth without a normal stream, texture, mesh change, or additional
submission. `BasicEffect` remains the explicit unlit fallback. Native OpenGL 3.3 compilation,
actual Chromium/Firefox WebGL 2 execution, and a current KVM/gfxstream GLES 3 jungle launch qualify
the full shader. A current native runtime image remains deferred while the operator-visible prior
executable is open.

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
and ageing colour so the close silhouette is not a mirrored comb. Placement validates complete footprints
against shore distance, route reservations, nearby trees, slope, and support-height variation.
The otherwise-unused natural-prop vertex alpha is a private material selector. It marks near-rock
fragments for subtle world-space strata, sparse lichen, a low fixed-sea-level wet band, and
derivative-based two-sided facet light from the shared fixed sun;
near-log sides for local-space bark fissures; near cut ends for gently irregular growth rings; and
near-driftwood sides/branches for softer sun-bleached longitudinal weathering. Driftwood cut ends
remain plain. Distance fades remove all cues before their mesh transition. Contact geometry and
every far mesh retain the plain path; this keeps the material treatment texture-free and free of
extra instance data or draws. Near fallen logs reuse their existing nine-sided end rings and cap
vertices, matching their radii and applying bounded deterministic axial offsets so the former
planar circular cuts become spatially closed irregular broken contours. Vertex/index counts,
material selection, instances, collision proxies, far LOD, and submissions are unchanged. The near
rock's existing seven crown rings keep the same topology,
but bounded per-slice height offsets and a taller asymmetric cap break the former flat top line.
Its 110 vertices and 192 triangles per shared variant remain unchanged. The vertex shader also
uses the existing relative neutral colour span to select stone-family vertices and recombines four existing
scale/yaw transform coefficients into a stable bounded warm/cool tint. This applies consistently
to near and far rock meshes without another attribute, buffer, texture, geometry, upload, or draw;
the colour-span mask leaves red-dominant wood and green foliage unchanged.
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

The ocean is a 160 by 160 quad grid drawn after opaque geometry. `OceanRenderer` supplies explicit
GLSL 330 and GLSL ES 300 variants through CNA's public portable shader-package extension. The
shader combines vertex waves, analytic fragment normals, Fresnel sky response, sun glint,
depth-driven tropical colour/transparency, moving foam, haze, two thin movement-driven ripple
bands, and a speed-scaled pair of broken entry/exit rings centred on the player. The two
crossing-wave values already used to break up broad foam also
apply restrained bright/dark subsurface colour in the non-reflective surface component. The colour
detail favours shallow water and fades from 135 to 420 m so it reads in exploration views without
aliasing across the horizon or adding another trigonometric field. A bounded shallow-ridge cue
reuses the length of the same five-wave analytic slope already needed for normals. It is limited
to the non-reflective, shallow, near/middle-distance surface and therefore adds no new sine,
texture, varying, geometry, or draw. A second narrow cue projects that same slope toward the fixed
sun, windows only the sun-facing signed range, and fades from 85 to 245 m. This gives nearby calm
water short directional crests while leaving the reflective horizon quiet, again without another
wave field or resource. A third cue projects the same slope toward the horizontal eye direction,
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
Native OpenGL 3.3 compiles the current shader object, while actual Chrome/WebGL 2 and a
KVM-accelerated API 35 Android GLES3 surface visibly render the full transition-burst,
sunward-crest, and grazing-reflection paths. The operator-visible native process intentionally
remains on its prior linked build until it closes. Physical mobile-GPU profiling remains open.

`ShorelineRenderer` adds one low-cost transparent pass after the ocean. Its 384-segment, six-strip
ribbon is generated from `IslandTerrain::ShoreRadius`, spans 2.5 m landward to 5.5 m offshore, and
therefore follows the authoritative coastline independently of the ocean grid's 10.625 m spacing.
Explicit GLSL 330 / GLSL ES 300 stages reuse the ocean's three vertex waves and animate a narrow,
broken crest plus a restrained receding wash. Terrain depth naturally clips the landward edge; a
premultiplied vertex-colour `BasicEffect` ribbon remains as the logged fallback. The full layer is
2,695 vertices, 4,608 triangles, one transparent draw submission, and no texture or external asset.

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
interpolated percentiles. Fixed benchmarks select an existing QA camera. The traversal benchmark
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

The four persistent beds use 10, 11, 29, and 7 second loop lengths. Their combined sample-aligned
pattern therefore does not recur for 22,330 seconds (6 hours, 12 minutes, 10 seconds). The longer
jungle source deliberately spends a modest additional 749,700 PCM bytes on reducing the most
recognizable local repetition rather than adding another simultaneous source.

Before PCM16 quantization, each persistent bed measures and subtracts its generated floating-point
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
PCM payloads total 3,336,164 bytes (about 3.18 MiB) before backend bookkeeping, including the
wildlife phrases described below. `Soundscape::Start` generates, converts, uploads, and releases
one source at a time. It therefore no longer retains all 3,336,164 PCM bytes and a second
3,336,164-byte conversion copy simultaneously; the largest game-owned staging pair is the jungle
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

## Current limitations

- Terrain has shared-buffer cell culling and contiguous visible-run batching but no distance LOD,
  authored material textures, or final biome masks.
- Palms have four geometry variants, segmented near leaflets, widened far crown silhouettes,
  staggered per-instance LOD, placement clustering, cell culling, subtle shader wind, trunk
  collision, irregular faceted root wedges, transform-derived green-only per-instance frond tint,
  and a near-field terrain contact shadow. True LOD
  blending, richer bark/frond materials, and full directional/self-shadowing remain open.
- The first grass, layered-jungle, rock, and fallen-wood tiers substantially improve first-person
  grounding, and near bushes now replace their largest smooth lobes with folded pointed leaves,
  while near rocks have restrained strata/lichen plus a low sea-level wet band, near logs have
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
  plus dedicated fixed bird and palm qualification frames.
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
