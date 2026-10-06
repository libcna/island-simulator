# island-simulator Development Plan

## Document purpose

This is the living execution ledger for a roughly 300-hour development campaign. It is not a
feature wish list: checked items require the validation named by their phase, and platform claims
must be supported by actual runtime evidence. New discoveries, regressions, framework workarounds,
asset provenance work, and deferred polish belong here as soon as they become material.

Status markers:

- `[x]` completed and evidenced
- `[ ]` not completed
- `[~]` in progress
- `[!]` blocked or degraded, with the reason written next to the item

Repository content, source, diagnostics, UI text, documentation, and commit messages are English.
CNA and Sharp Runtime are read-only dependencies for the entire campaign.

## Product definition and non-goals

`island-simulator` is a peaceful first-person tropical island walking and near-shore swimming
experience. Its approximately 500 m island is interesting because of terrain composition,
vegetation, wildlife, water, light, motion, sound, and polished traversal. It has no quests,
missions, combat, enemies, inventory, crafting, survival meters, economy, score, progression,
multiplayer, story campaign, dynamic weather, or day/night cycle.

The required release targets are:

| Target | Renderer | Minimum release evidence |
|---|---|---|
| Linux desktop | OpenGL 3.3 | Clean build plus repeated real-GPU play sessions |
| Web browser | WebGL 2 | Clean Emscripten build plus an actual supported-browser play session |
| Android | OpenGL ES 3 | Clean Android build plus a real device or emulator play session with touch input |

Compilation is not runtime validation. A target stays unverified until it has run in its intended
environment.

## Non-negotiable engineering constraints

- Use C++23.
- Consume current CNA `next` and current Sharp Runtime `next` only.
- Never edit CNA or Sharp Runtime source, worktrees, branches, indexes, or repository metadata.
- Keep framework workarounds in this repository, narrowly scoped and cross-referenced from
  `BUGS.md` when they address a confirmed framework defect.
- Use no more than eight CPU jobs. Use no more than two jobs for any operation governed by Sharp
  Runtime's stricter repository policy.
- Use the shared ccache at `/rv/cnaccache` with `CCACHE_BASEDIR=/rv`; never create another cache.
- Reuse only the approved build directory names: `build/`, `build-probe/`, `build-consumer/`,
  `build-asan/`, `build-ubsan/`, and `build-tsan/`.
- Never build in `/tmp` and never retain obsolete large generated artifacts.
- Prefer CNA/XNA abstractions and supported CNA extensions over raw OpenGL, SDL, or a new engine.
- Keep simulation and placement deterministic when this improves tests, debugging, and captures.
- Preserve a playable build at meaningful milestones; do not accumulate large untested batches.

## Verified workspace baseline (refreshed 2026-10-06)

- [x] Confirmed `island-simulator` is a new repository on unborn branch `develop`, with no source
  files or commits at campaign start.
- [x] Read the workspace build policy in `../AGENTS.md`.
- [x] Located CNA at `../cna` and inspected its repository instructions and current APIs.
- [x] Qualified and locked CNA `next` at commit `39e84add7777` (`refactor(PLAT-142): retire the
  direct Win32, X11 and Wayland platforms`, 2026-10-06). The preceding qualified export was
  `f243c63a1dc5`; the campaign started from `b0e97bb1bb87`.
- [x] Confirmed no separate stale CNA worktree is being mistaken for the current `next` identity.
- [x] Located Sharp Runtime at `../sharp-runtime`, read its repository instructions, and confirmed
  it is on local `next` at `21c06910f7cf` (`fix(MSR-030): repair Windows consumer portability`,
  2026-10-05). The local branch is one commit ahead of `origin/next`; the local `next` commit is
  the campaign baseline until deliberately refreshed and requalified.
- [x] Confirmed CNA `next` exposes compile-time `OPENGL33`, `WEBGL2`, and `OPENGLES3` identities
  through a shared EasyGL implementation with profile-specific shader adaptation.
- [x] Confirmed CNA `next` supports the needed high-level surfaces: `Game`, `GraphicsDevice`,
  `BasicEffect`, indexed and instanced drawing, keyboard, mouse, touch, audio emitters/listeners,
  direct glTF loading, skinning, animation, and renderer-neutral content facilities.
- [x] Located the required GL-family dependencies `../easy-gl` and `../meta-gl`.
- [x] Surveyed relevant sibling projects and selected focused references:
  - `../cna-template`: current cross-renderer CMake, Android packaging, Web presets, and known
    consumer-side CNA workarounds.
  - `../house-simulator`: first-person traversal architecture, content-authored world data,
    validation discipline, and framework-boundary practices.
  - `../cna-car-simulator`: terrain queries, collision, spatial grids, walking mode, procedural
    vegetation, sky, layered/positional audio, and debug/performance documentation.
  - `../cna-street`: instanced meshes, cameras, atmosphere, skeletal meshes, autonomous agents,
    audio soundscape, and deterministic procedural geometry.
  - `../living-room-simulator`: lighting, shadows, fog, post-processing, render-target management,
    and documented renderer limitations; techniques will be selected conservatively for mobile.
  - `../cna-gltf-viewer`: current CNA glTF and animation consumer patterns.
  - `../cna-samples`: renderer and platform qualification evidence.
- [x] Confirmed host tools: CMake 3.31.6, Ninja 1.12.1, GCC 14.2.0, ccache 4.11.2, and `adb`.
- [x] Confirmed and qualified the existing Emscripten SDK under `~/emsdk`; Web builds use its
  pinned compiler and Node runtime without creating a duplicate installation.
- [x] Recorded initial disk state: approximately 217 GiB free; `island-simulator` used about
  124 KiB before artifacts. Existing large CNA/sibling build trees are outside this project's
  ownership and will not be removed.
- [~] Complete a targeted line-by-line API review of the exact CNA `next` entry point, 3D buffer,
  effect, texture, audio, touch, content, and animation headers used by the first vertical slice.
  Entry-point, game loop, input, buffers, `BasicEffect`, capture, audio, glTF, and skinning surfaces
  have been reviewed; platform packaging and later content APIs require phase-specific review.
- [x] Record exact EasyGL and MetaGL branch/commit baselines used by the three renderer paths in
  `dependencies.lock` and enforce them at configure time.
- [ ] Recheck all four dependency identities whenever a baseline is intentionally advanced.

## Architectural decisions

### Dependency identity and read-only consumption

The build must reject an accidental non-`next` dependency. Because the only CNA working tree is on
`samples` and may not be switched or modified, initial Linux work will use a reproducible,
unmodified export of the exact CNA `next` tree under the ignored `build/` area, or an externally
provided read-only CNA `next` worktree. The source export is build input, never vendored or edited
game code. CMake will receive explicit roots for CNA, Sharp Runtime, EasyGL, and MetaGL and record
their resolved identities in configure output. Normal developer documentation will recommend
separate sibling checkouts on their required branches.

### Runtime boundary

Game code uses the XNA-shaped `Microsoft::Xna::Framework` API and only narrowly justified CNA
extensions. Raw OpenGL/GLES/WebGL and native SDL calls are prohibited in gameplay/rendering code.
This protects renderer portability and makes framework limitations explicit.

### World representation

The island uses a finite terrain mesh/height field suited to a roughly 500 m landmass, not a
planet-scale terrain engine. A deterministic authored-procedural hybrid provides repeatable base
terrain, biome/density maps, authored landmarks, clearings, paths, coves, rock groups, and spawn
regions. Height, normal, slope, shoreline distance, biome, and navigability queries share one
authoritative CPU representation so rendering, collision, placement, wildlife, and tests agree.

### Rendering strategy

Start with portable CNA vertex/index buffers and stock effects. Use static batching and CNA's
instancing where verified on all required renderers. Near grass uses compact mesh clusters with
vertex wind; middle/far grass reduces density and complexity. Avoid geometry, tessellation, and
compute shaders. Fixed sunny lighting permits precomputed ambient/occlusion information and stable
shadow choices. Expensive desktop-only effects must have deliberate Web/mobile fallbacks.

### Content strategy

Prefer original procedural meshes/textures/audio and redistributable CC0/public-domain inputs.
Every external asset is copied into the reproducible asset pipeline and entered in
`ASSET_LICENSES.md` before use. Runtime assets are generated deterministically where practical;
source assets remain available when licensing permits.

### Simulation strategy

Player and wildlife simulation run at bounded fixed steps with interpolated presentation where
useful. Wildlife shares a lightweight state/steering framework with species-specific constraints.
Distance tiers suspend or reduce invisible simulation. The ocean limit is a soft, island-relative
current field followed by an emergency recovery only beyond a second safety radius.

## Milestone and budget summary

The hour allocations are planning envelopes, not time-filling targets. Reallocate only when the
reason is recorded here.

| Phase | Focus | Budget | Exit milestone |
|---|---|---:|---|
| 0 | Reconnaissance and plan | 12 h | Audited, pinned, documented baseline |
| 1 | Technical bootstrap | 20 h | Linux OpenGL 3.3 window and tested core library |
| 2 | Core traversal | 32 h | Pleasant full-island walking and near-shore swimming |
| 3 | Environment foundation | 50 h | Composed island, ocean, beach, jungle, grass, sky |
| 4 | Wildlife and animation | 42 h | Birds, fish, shark, monkeys with believable behaviour |
| 5 | Atmosphere and audio | 24 h | Cohesive sunny presentation and layered soundscape |
| 6 | Web/WebGL 2 | 24 h | Actual browser build and play validation |
| 7 | Android/GLES 3 | 28 h | Actual Android runtime and touch validation |
| 8 | Performance and robustness | 28 h | Budgets met with profiling evidence |
| 9 | Long polish and release QA | 40 h | Repeated cross-platform polish and release evidence |
| **Total** |  | **300 h** |  |

## Phase 0 — Reconnaissance and planning (12 h)

Dependencies: none.

- [x] Inspect repository state, disk usage, available tools, and workspace policies.
- [x] Locate and identify CNA `next` and Sharp Runtime `next` without modifying either.
- [x] Inspect CNA renderer, input, model/animation, audio, Web, and Android documentation.
- [x] Inventory sibling CNA projects and select proven patterns worth deeper review.
- [~] Read exact public headers and minimal examples for the vertical slice.
- [x] Inspect CNA `next` Android activity/entry-point requirements, SDL3 Java bridge, shared
  `libmain.so` entry point, renderer selection, and the template's package boundary in detail.
- [x] Confirm an existing Emscripten SDK at `~/emsdk`; browser inventory and test tooling still
  need inspection before the Web build.
- [x] Inventory Android SDK platform 35, Build Tools 36.1.0, NDK 29.0.14206865, SDK CMake 4.1.2,
  JDK 21, Gradle 8.12, arm64-v8a/x86_64 package paths, and three API 35 x86_64 AVDs. No physical
  device or `/dev/kvm` acceleration is available in the current environment.
- [x] Capture the initial subsystem and dependency decisions in `docs/architecture.md`.
- [x] Create this plan before implementation.

Exit validation:

- Dependency commits and renderer identities are recorded.
- No dependency file, branch, index, worktree, or repository metadata changed.
- Phase 1 has no unresolved API guess that would force a likely redesign.

## Phase 1 — Technical bootstrap (20 h)

Dependencies: Phase 0 dependency/API baseline.

### Build and application shell

- [x] Add `.gitignore`, `CMakeLists.txt`, CMake presets, narrow source layout, tests, and tools.
- [x] Enforce C++23 and warnings-as-errors for first-party code. The CNA consumer target omits
  `-Wpedantic` because Sharp Runtime intentionally exposes compiler-provided `__int128`.
- [x] Add read-only dependency-root validation and configure-time branch/commit diagnostics.
- [x] Add a reproducible local CNA `next` source-export helper for the current workspace while
  accepting a normal read-only `CNA_ROOT_DIR` on other machines.
- [x] Point CNA explicitly at Sharp Runtime `next`, EasyGL, and MetaGL without fetching duplicates.
- [x] Configure CNA with tests/examples and unused network/video/Draco/font/CNB compression
  features disabled. Add CNA as `EXCLUDE_FROM_ALL` so unrelated offline tools do not enter normal
  game builds while linked runtime targets still build transitively.
- [x] Select `OPENGL33` explicitly for the Linux preset; never rely on CNA's platform default.
- [x] Add Linux entry point and a `Game` subclass with actionable fatal diagnostics.
- [x] Create a window, HiDef graphics device, depth-tested indexed terrain, ocean, and fixed sky.
- [x] Use the live viewport aspect ratio and close/release pointer capture cleanly with Escape.
- [ ] Add content-root discovery without absolute developer paths.
- [x] Add a core test executable that avoids graphics initialization.

### Foundational code quality

- [ ] Establish namespace, include, ownership, units, coordinate, handedness, and naming rules.
- [x] Add deterministic noise/terrain helpers with repeatability and scale tests.
- [ ] Add small math/geometry helpers only where CNA's existing types do not already suffice.
- [~] Add startup/render statistics and fatal exception diagnostics. A schema-1 first-draw report
  now separates world construction, framework initialization, GPU/content creation, pre-draw, and
  game-owned draw submission on the portable game path; structured subsystem logging and
  asset-specific failure context remain open.
- [x] Add reproducible configure, bounded build, core-test, private GPU smoke, and capture commands.
- [x] Create `README.md`, `BUGS.md`, `ASSET_LICENSES.md`, initial architecture, Linux build, and
  controls documentation.

Exit validation:

- Clean configure and incremental Linux `OPENGL33` build using CNA `next` and Sharp Runtime `next`.
- Actual private-display/real-GPU launch, visible primitive, stable close, no shader errors.
- Unit test binary passes; `git diff --check` is clean; repository contains no build products.

## Phase 2 — Core traversal (32 h)

Dependencies: Phase 1 graphics/input shell and authoritative terrain query interface.

### Terrain and collision

- [~] Implement a deterministic roughly 500 m analytic island height/shore field. Biome masks,
  authored landmarks, and final composition remain open.
- [~] Generate a 241 by 241 render mesh with procedural normals/colours, subdivided into 64
  independently frustum-culled cells with portable 16-bit indices. Textures/UVs and distance LOD
  remain open.
- [~] Implement and test height, land/water, island-relative shoreline distance, unit normal,
  slope, and coarse physical-surface queries. Authored biome masks and bounds queries remain open.
- [~] Add broad-phase spatial partitioning for rocks/trees and capsule-vs-world collision. A static
  16 m grid now indexes the shared palm, broadleaf, solid land/reef-rock, and sampled fallen-log
  proxies;
  exact narrow-phase checks retain substepped axis sliding. Dynamic obstacles and depenetration
  remain open.
- [~] Add footprint-aware ground support, ground following, a 38-degree uphill limit, axis sliding,
  bounded step rejection, and palm-trunk collision. Solid land-rock, reef-outcrop, and log
  collision is active while walking; the same horizontal proxies constrain swimming.
  Depenetration and geometric step-up remain open.
- [x] Prevent terrain tunnelling under low frame rates with a 100 ms time clamp and 0.14 m maximum
  horizontal substeps; keep swept prop tests open with prop collision.

### First-person controller

- [~] Implement keyboard movement and mouse look with pitch limits. Runtime configuration remains
  open.
- [~] Capture/release relative mouse input on Linux. Focus, Web, and mobile differences remain open.
- [~] Establish initial eye height, acceleration, deceleration, walking speed, and a 0.34 m capsule
  footprint. Full capsule-vs-prop collision and evidence-based tuning remain open.
- [x] Add optional mild head motion only if it improves presence and can be disabled. A tested,
  renderer-independent positional model uses at most 10 mm lateral and 18 mm vertical motion,
  blends between walking and swimming rhythms, resets after QA teleports, and toggles with B.
- [ ] Keep jumping absent unless traversal testing demonstrates a clear need.

### Swimming and ocean boundary

- [x] Define depth-based water-entry/exit hysteresis.
- [~] Implement near-surface camera height, reduced acceleration, swimming speed, and drag. A
  restrained swimming camera rhythm, solid emergent-outcrop collision, and subtle
  depth-responsive near-surface edge veil plus movement-driven surface ripples are complete; a
  true geometric waterline and full underwater presentation remain open.
- [x] Define accessible shoreline distance from the authoritative island field, not world boxes.
- [~] Add progressive outward resistance and inward correction from 42 to 72 m offshore. The
  water-view veil and undertow audio now strengthen across the same normalized field; lateral
  current variation remains open.
- [x] Add subtle audiovisual warning before the hard safety boundary. Current-modulated peripheral
  colour/intensity and a bounded deeper undertow bed in the existing water loop share the same
  normalized field without a HUD warning, geometric wall, or fifth persistent source.
- [x] Add a tested 92 m last-resort radial safety cap that removes outward velocity.

### Tests and tuning

- [~] Unit-test terrain scale/determinism, shore distance, normals/slopes/surfaces, footprint ground
  support, movement integration, water hysteresis, long-frame substeps, boundary forces, six
  shoreline round trips, an automatically selected steep-bank approach, and an authored land-rock
  collision route. Longer route-duration and input-feel tests remain.
- [x] Add deterministic traversal scenarios for coves, rocks, steep banks, and shore entry/exit.
- [ ] Run repeated 10+ minute Linux traversal sessions and record feel defects in this plan.

Exit validation:

- Player can circle the beach, cross the island, climb intended slopes, enter/leave water, and
  cannot swim indefinitely offshore.
- No free-camera feel, common snagging, visible jitter, or reproducible terrain fall-through.

## Phase 3 — Environment foundation (50 h)

Dependencies: stable terrain/shore queries and representative performance instrumentation.

### Island composition

- [ ] Shape multiple coves, beach widths, low ridges, viewpoints, jungle pockets, clearings, and
  readable paths without an implausible mountain.
- [ ] Create biome/density masks with authored overrides and deterministic placement seeds.
- [~] Add sand, wet-sand, soil, grass/jungle-floor, and rock material transitions. Terrain vertices
  now carry authoritative shore distance and slope into a portable texture-free material shader;
  two-scale value noise and fine breakup vary sand/interior soil, the wet edge darkens coherently,
  steeper interior ground exposes a muted soil tint, and a distance-bounded microvariation octave
  plus two sparse deterministic grids add curved pointed leaf litter to the interior. A restrained
  fixed-sun dapple reuses those noise fields on low-slope ground beyond the beach transition. The
  same portable shader now draws a subdued broken-edge dry-earth footpath from the authoritative
  vegetation-reserved corridor, with short grass retained over it. The two shared authored clearing
  definitions now drive distinct warm-dry and darker leaf-rich surface treatments. Five authored
  rocks and one fallen log add a first natural landmark composition around their outer rings while
  retaining open centres. Authored biome masks, richer landmarks, decals, broader litter diversity,
  and final colour/scale tuning remain.
- [~] Break the island silhouette with rocks, palms, shallow banks, and small offshore features.
  Fourteen deterministic emergent outcrops now interrupt the shallow-water perimeter. Five
  unreachable haze-softened low islets with 11 simplified palm silhouettes now break up the
  open-ocean horizon in one static batch; grouped shallow banks remain.
- [~] Keep routes traversable and provide alternating enclosed/revealing views. The deterministic
  south-to-north corridor and two clearings reserve vegetation/prop space; its first shared terrain
  material cue improves first-person legibility without becoming a road or HUD marker. The two
  rooms now have distinct ground identities, first rock/log landmark groups, and dedicated cross-
  renderer QA views. Whole-route composition review, broader landmarks, and stronger enclosed/
  revealing rhythm remain.

### Ocean and shoreline

- [x] Build a bounded 25,921-vertex animated ocean grid large enough to hide the finite world
  boundary while retaining portable 16-bit indices.
- [x] Implement explicit GLSL 330 / GLSL ES 300 vertex wave motion and analytic layered
  normal/specular response through CNA's portable `ShaderEffect` package API.
- [x] Derive shallow/deep tropical colour and transparency from authoritative terrain depth and
  shore information baked into the ocean vertices.
- [~] Blend shoreline foam/readable moving wave edge and translucent water over the beach without
  visible z-fighting. A 384-segment ribbon now follows the authoritative shore independently of
  the coarse ocean grid and adds a narrow broken animated crest plus restrained receding wash over
  the wet-sand pass. A subtle generated near-surface edge veil now fades with swimming state and
  support depth, while depth/speed-responsive player ripples reuse the animated ocean submission;
  full underwater transitions and final wash/material tuning remain open.
- [~] Add underwater/near-surface tint, fog, and refraction-like cues only when portable and
  stable. The first tint is a renderer-portable, premultiplied 160 by 90 generated edge veil with
  one conditional `SpriteBatch` draw; waterline geometry, underwater fog, and refraction remain.
- [~] Provide an actionable stock `BasicEffect` fallback when portable custom effects are
  unavailable or fail compilation. WebGL 2 uses the full shader successfully; Android GLES 3
  compiles and runs the scene, but the captured inland camera does not yet qualify the water path.

### Vegetation and props

- [~] Build multiple procedural palm variants with different height, lean, curvature, crown
  rotation, frond arrangement, scale, and clustering. Four curved tapered trunk variants, 330
  varied instances, segmented near leaflets, widened-ribbon far crowns, root contact wedges,
  staggered per-instance near/far thresholds, near-only procedural leaf scars, two restrained
  coconut-bearing variants, and portable shader wind now run on native OpenGL 3.3 and WebGL 2;
  broader bark/fruit variation, true LOD blending, and instrumented Android wind/LOD qualification
  remain open despite the first visible GLES3 scene.
- [~] Add broadleaf canopy, medium plants, bushes, ferns, ground plants, rocks, fallen wood, and
  sparse natural beach props. The first layer now provides 281 four-variant canopy trees and 1,822
  four-variant fern/large-leaf plants, 565 three-variant low bushes, 2,225 three-variant
  forest-floor plants, 157 land rocks, 14 emergent reef outcrops, 12 fallen logs, 28 three-variant
  driftwood pieces, 63 grove-associated fallen palm fronds, and 265 four-variant shell/pebble
  clusters. Near canopy meshes now add tapered support branches, irregular separated lobes, and pointed
  edge leaves. Near bushes now use smaller lobes with paired folded pointed leaves while retaining
  their compact far tier. Near forest-floor variants now include six upright broad leaves with
  visible petioles and a separate five-leaf folded split form while retaining compact far
  tiers. All four jungle layers now enforce range and near/far LOD per instance after conservative
  cell rejection. The shared portable shader now derives
  two-sided geometric facet light from the fixed sun for every jungle tier without another vertex
  stream or draw; further floor-plant/bush material diversity and broader natural beach-prop
  variety remain.
- [~] Keep placement biome-aware, slope-aware, collision-aware, clustered, and reproducible. Palm,
  canopy, understory, bush, forest-floor, rock, log, driftwood, fallen-frond, and beach-detail
  placement is deterministic, shore-distance/slope/height constrained, multi-scale cluster-noise
  weighted, and reserves a winding primary path, two clearings, and spawn; final biome masks and
  prop separation remain open.
- [~] Add subtle coherent breeze phases without storm-scale motion. Palm fronds, grass, canopy
  crowns, understory leaves, low bushes, and forest-floor plants use bounded weighted motion on OpenGL 3.3 and WebGL 2;
  Android GLES 3 runtime validation of the expanded jungle shader remains open.
- [~] Ensure collision is applied only where visually necessary; avoid a forest of snag hazards.
  One radius-matched circular proxy is active per palm and canopy trunk; substantial land rocks,
  emergent reef outcrops, and fallen logs are solid, while fronds, crowns, understory, ground
  plants, small rocks, and driftwood remain non-colliding. The same proxies now constrain walking
  and swimming. Broader play-tuning remains open; the shared static-proxy spatial broad phase is
  active.

### Grass quality campaign

- [~] Prototype clustered mesh blades and evaluate first-person silhouette/overdraw on all backends.
  A 28-blade, five-control-level near mesh and five-blade, three-level far mesh now run on OpenGL
  3.3 and actual WebGL 2. Near blades have folded side colour, two-sided fixed-sun lighting, root
  darkening, and restrained transmission; the earlier grass is visible in the first GLES3 frame,
  while the current shading package only compiles there. Measured mobile overdraw remains.
- [~] Use density, colour, height, lean, and phase variation tied to terrain/biome data. The first
  18,556-patch layout varies all five deterministically from terrain constraints and smooth cluster
  noise; final biome masks remain open.
- [x] Align clusters to authoritative terrain normals without flattening individual blade growth.
- [~] Add vertex wind with bounded amplitude and coherent gust-free sunny-day motion. The portable
  two-frequency wind path is verified on OpenGL 3.3 and WebGL 2; GLES 3 remains unqualified.
- [x] Add bounded player interaction without a CPU per-patch update. Nearby blade tips now bend
  away from a 0.55 m stationary body radius, smoothly expand to 1.35 m at walking speed, and ease
  back after movement inside the existing portable instanced vertex shader.
- [~] Implement near/middle/far density and mesh LOD with dither/fade or masked transitions. Near
  and far meshes plus ground-colour distance merging exist; a middle tier and pop-free transition
  still need visual/performance evidence.
- [~] Cull cells by frustum/distance and batch or instance where CNA's verified path permits. A
  256-cell grid and one hardware-instanced draw per visible cell are verified on native and WebGL 2;
  Android counters and sustained performance remain open.
- [~] Test worst-case first-person grass views for aliasing, transparency, overdraw, and frame time.
  A deterministic close grass camera now runs in native and browser suites, verifies the full
  interaction path, and has a two-repeat native debug-build timing envelope. Transparent-overdraw,
  sustained movement, release, and physical-mobile measurements remain open.

### Lighting, sky, distance, and shadows

- [~] Establish one fixed attractive sun direction, colour temperature, fill light, and exposure.
  Terrain, ocean glint, and sky now consume one shared direction and warm daylight palette; a
  measured exposure/fill-light pass and Android runtime comparison remain open.
- [~] Build a portable sky gradient/dome with sun disc and high, calm tropical cloud suggestion.
  A camera-centred sphere with explicit GLSL 330 / GLSL ES 300 shaders, a stock-effect gradient
  fallback, and sparse layered horizontal high wisps is visually verified on OpenGL 3.3 and actual
  WebGL 2. The earlier conspicuous vertical cloud streaks have been removed. The sky-inclusive
  GLES 3 package compiles, but stable Android runtime qualification remains open.
- [~] Add distance haze/fog that enlarges perceived scale without washing out nearby jungle. The
  distant-scenery batch now uses restrained 260--790 m `BasicEffect` fog; unified world haze and
  near/middle-distance atmospheric tuning remain.
- [~] Evaluate one bounded directional shadow solution or baked/vertex alternatives on all targets.
  The retained portable contact-shadow batch is the first measured fallback; full directional or
  baked canopy shadowing and Android runtime evidence remain open.
- [x] Provide contact/grounding cues for palms, rocks, props, and animals even when shadows degrade.
  Root wedges remain on near palms, while one terrain-conforming soft batch now covers nearby
  palms, canopy trees, land rocks, fallen logs, driftwood, and monkeys on native and actual WebGL 2.
- [x] Add distant low-cost scenery and vegetation cards/meshes without exposing the world edge.
  Five deterministic islets stay beyond the player's 92 m offshore safety region, remain inside
  the 850 m ocean surface, and combine 787 opaque triangles plus 11 palm silhouettes in one
  portable fogged draw.

Exit validation:

- Representative beach, jungle, clearing, hill, cove, and offshore views look intentionally
  composed from first-person height.
- Grass and palms do not read as uniform stamps.
- Linux frame timing is instrumented before density is increased further.

## Phase 4 — Wildlife and animation (42 h)

Dependencies: terrain/water/spawn queries, culling, and content/animation pipeline.

### Reusable wildlife framework

- [x] Implement spawn regions, deterministic variation, simple steering, state timers, activation
  range, simulation LOD, animation state, and optional audio emitter. The first shared
  `WildlifeActivationPolicy` supplies full/quarter-rate/one-thirtieth-rate tiers, while `BirdFlock`
  `FishSchools`, `SharkPatrol`, and `MonkeyTroop` provide deterministic steering, state timing,
  and animation pose. A bounded shared audio scheduler now selects actual nearby bird/monkey
  positions without allocating a voice per agent.
- [x] Validate positions against land/water/depth/slope constraints before and during movement.
  Birds continuously enforce family-specific terrain clearance and bounded island-relative
  airspace. Fish continuously enforce a 3--48 m offshore belt, submerged terrain, seabed clearance,
  and surface clearance. The shark continuously enforces a 34--88 m offshore belt, minimum water
  depth, seabed clearance, and surface clearance. Monkeys validate every proposed move against a
  48 m interior margin, dry support, a 26-degree slope limit, and the player collision set of
  trunks, solid rocks, and sampled fallen logs.
- [~] Add debug visualization for regions, paths, state, and activation tiers. F2 now covers all
  bird route families/spawns/live radial targets, fish-school centres, the shark target, monkey
  home/active targets, headings, behaviour colours, and full/reduced/dormant tiers; steering
  proposals and non-bird spawn volumes remain open.
- [~] Unit-test state transitions, bounds, spawn validity, avoidance, and deterministic seeds. Bird
  population, path-family mix, deterministic initialization/long-run motion, animation extremes,
  terrain clearance, route radius, and tier accounting are covered through a simulated 20-minute
  flight. Fish population/school distribution, water validity, deterministic motion, tail extremes,
  meaningful route travel, and tier accounting are covered through a simulated ten-minute swim.
  Shark deterministic motion, water/depth validity, presence/absence cycles, body animation,
  meaningful route travel, and tier accounting are covered through a simulated 20-minute patrol.
  Monkey population, deterministic long-run state/motion, finite pose, terrain/slope validity,
  idle/observe/walk/run-away reachability, meaningful travel, and tier accounting are covered
  through a simulated 20-minute session. The same run samples clearance against the complete
  world obstacle set and requires deterministic steering deflections.

### Birds

- [x] Add multiple path families above shore/interior with altitude, speed, turn, and group variation.
  Ten shore, eight canopy, and six high agents use distinct radii, altitudes, speeds, directions,
  vertical phases, and two-frequency steering variation.
- [x] Avoid short obvious loops and synchronized animation. Per-agent incommensurate steering,
  altitude, and flap phases remove shared cycles; a long deterministic regression guards bounded
  nondegenerate motion, while subjective whole-island observation remains a polish task.
- [x] Add occasional near and distant appearances with bounded active counts. The fixed 24-agent
  budget uses individual frustum/distance culling, near/far procedural meshes, distance-throttled
  updates, and one dynamic-buffer draw submission.
- [x] Add sparse positional calls without attaching a sound to every visible bird. Two original
  short phrases share one voice with monkey calls, select the nearest non-dormant bird within
  110 m, and use CNA listener/emitter placement with 13--30 second scheduling.

### Fish and shark

- [x] Add shallow-water fish schools with depth bands, body motion, separation/cohesion, and land
  avoidance based on authoritative water queries. Eight six-member schools follow the irregular
  shore, search outward for sufficient depth, retain stable varied formation offsets, animate
  individual body/tail phases, and reject invalid land/deep-ocean moves.
- [x] Activate/cull schools spatially; never simulate large invisible populations. The fixed
  48-agent budget uses school-level full/reduced/dormant simulation tiers, per-fish frustum/distance
  culling, near/far meshes, and one dynamic-buffer draw submission.
- [x] Add one occasional offshore shark with credible silhouette, patrol variation, swimming
  animation, and no hostile behaviour. A single original procedural model uses near/far geometry,
  CPU body/tail motion, frustum/distance culling, and one portable dynamic-buffer draw.
- [x] Keep the shark out of immediate beach shallows and tune rarity for atmosphere, not tension.
  It patrols 34--88 m offshore for roughly 42--66 seconds, then remains below the presentation
  layer for roughly 92--160 seconds before the next appearance.

### Monkeys and skeletal animation

- [x] Select or create a legally redistributable monkey/substitute model and record provenance.
  The first troop uses three original runtime-generated low-poly silhouettes, four palettes, and a
  fourteen-bone rig recorded in `ASSET_LICENSES.md`, so it has no external asset or
  disappearing-source dependency.
- [~] Validate skeleton, clips, scale, axes, material fallback, and all three renderers. Native
  OpenGL 3.3 and actual Chromium/WebGL 2 captures agree; the arm64 GLES 3 APK builds and lints, but
  stable Android runtime remains unqualified.
- [x] Integrate CNA skeletal animation without framework modification. Project-generated bind and
  inverse-bind poses plus four clips drive one CNA `AnimationPlayer` per agent and the portable
  `SkinnedEffect`; no dependency source changed.
- [x] Implement compact `Idle`, `Observe`, `Wander`, `Walk`, and `RunAway` states. Seven agents use
  deterministic timers/targets, and the close-review scene exercises idle, observe, walk, and mild
  avoidance in the same frame.
- [x] Blend or transition clips without visible snapping; synchronize movement with animation
  speed. Walk/run cycles are speed-matched, `Wander` to `Walk` preserves its locomotion phase, and
  idle/observe/walk/run changes use 0.14--0.26 second eased local-transform crossfades before
  recomposing the world and skinning palettes.
- [x] Constrain monkeys to valid land/terrain regions and avoid water, steep slopes, and props.
  Terrain, shore, slope, and bounded-home constraints are enforced continuously. The troop also
  consumes the same palm, broadleaf, solid-rock, and fallen-log circular proxies as player
  collision, validates spawn/target/movement clearance, and tries deterministic lateral steering
  before abandoning an obstructed target.
- [x] Add mild player awareness without enemies, attacks, quests, or interaction systems. A player
  within 7.5 m causes a brief bounded retreat followed by observation.
- [x] Distance-throttle animation and decisions while preserving nearby quality. The shared
  full/reduced/dormant policy throttles decisions, while render distance/frustum culling and a
  one-draw far mesh replace the three-pass facial-detail near LOD.

Exit validation:

- All four wildlife categories appear naturally during an ordinary island circuit.
- No frequent land/water violations, obvious repeated loops, animation explosions, or dead crowds.
- Monkeys remain environmental life, not a gameplay objective or threat.

## Phase 5 — Atmosphere and audio (24 h)

Dependencies: stable zones, wildlife emitters, and representative locations.

- [~] Confirm CNA audio format, streaming, lifetime, listener/emitter, and platform constraints.
  Raw PCM16 mono loading, explicit loop regions, instance lifetime, mute/volume, graceful device
  failure, Linux playback startup, first-gesture Chromium startup, and real native CNA positional
  playback are verified. Focus restoration, Android playback, and streaming remain phase-specific
  work.
- [~] Build a layered mixer/soundscape controller with beach, ocean, jungle, clearing, and water
  zones and smooth hysteretic crossfades. The first renderer-independent shoreline/swimming model
  smoothly blends surf and canopy breeze, while player water audio uses depth/speed smoothing and
  a hysteretic transition detector and a slower interior texture fade; authored clearing-specific
  mixing and richer biome zones remain.
- [~] Add non-repetitive surf beds and occasional stronger waves tied to shoreline proximity. A
  deterministic ten-second filtered-noise bed now follows shoreline distance, and two distinct
  four-second washes are selected at varied 14--32 second intervals near audible surf. Subjective
  balance and long-session fatigue tuning remain.
- [~] Add wind-through-vegetation, jungle beds, birds, insects where tasteful, monkey calls, distant
  wildlife, and sparse water splashes. A light-canopy-breeze loop and subtle twelve-second daytime
  jungle bed increase inland, two cooldown-protected water entry/exit textures are active, and two
  bird plus two monkey phrases now play sparsely from real nearby agents; additional insect/accent
  variation and richer biome layers remain.
- [x] Add positional one-shots/emitters where direction and distance improve presence. One pooled
  mono wildlife voice uses CNA `AudioListener`/`AudioEmitter`/`Apply3D`, camera orientation, agent
  positions, 24 m reference falloff, distance gates, and species habitat gates.
- [x] Add player wading/swimming movement audio without repetitive rapid triggering. One continuous
  six-second procedural loop follows authoritative support depth, locomotion mode, and horizontal
  speed with asymmetric fade smoothing and wading/swimming pitch variation. The same source now
  gains a deeper 0.08-maximum undertow bed across the 42--72 m current field, with separate 650 ms
  cue smoothing and no dry-state leakage.
- [x] Bound simultaneously active sources and virtualize/throttle inaudible emitters. The current
  system has exactly four persistent loops and at most two transient voices. Wildlife owns one
  pooled voice, starts only with both transient slots idle, and is pre-empted when a water crossing
  would otherwise exceed the ceiling. Inland waves and distant/dormant wildlife are virtualized.
- [~] Normalize source material, remove clicks at loops, and test mono/stereo spatial assumptions.
  Core tests cover deterministic duration/rate, peak headroom, RMS energy, loop-boundary delta,
  distinct source PCM, silent wildlife endpoints, and the mono-to-CNA positional output path. A
  native report now measures all 12 exact sources for DC offset, peak/RMS, loop seam versus normal
  internal steps, zero-crossing rate, duration, uniqueness, and PCM footprint, and can export
  standard PCM16 WAV previews; headphone listening remains.
- [x] Add an accelerated long-session numerical regression. A deterministic 72,000-frame/
  twenty-minute route cycles beach, jungle, swimming/current, and wading/dry transitions while
  checking finite bounded mixes, undertow strength, bounded wave activity, exact crossing events,
  and both accent variants.
- [x] Provide graceful no-audio diagnostics/fallback without crashing the visual experience. Start
  and runtime volume failures log an actionable reason, disable audio resources, and leave graphics
  and traversal active; the F1 overlay exposes `UNAVAILABLE` rather than hiding the failure.
- [ ] Walk every major region specifically listening for silence, abrupt transitions, masking, and
  short-loop fatigue.

Exit validation:

- The island is never unintentionally acoustically dead.
- Zone changes are subtle, directional events are useful, and no loop becomes obviously annoying
  during a 20-minute session.

## Phase 6 — Web / WebGL 2 (24 h)

Dependencies: portable baseline features and bounded initial asset size.

- [x] Locate and activate the existing Emscripten SDK without duplicating it.
- [x] Add a `WEBGL2` preset and browser packaging with explicit WebGL 2 requirements.
- [x] Preserve CNA's Emscripten game-loop lifetime model; an actual Chromium smoke run rendered
  four frames, returned from `Game::Run()`, and exited with code zero.
- [~] Verify stock `BasicEffect` terrain fallback and explicit GLSL ES 300 ocean/palm/grass/jungle
  shaders through actual WebGL 2 browser compilation and execution. The current 330-palm,
  18,556-patch grass, 281-canopy, 1,822-understory, 565-bush, and 2,225-ground-plant
  hardware-instancing paths, wind
  shaders, and LOD selection are verified; later wildlife and material variants remain open.
- [~] Implement pointer-lock acquisition/release and clear click-to-focus/fullscreen instructions.
  The production shell provides click-to-play controls, first-gesture lock, Escape release, and
  clean click reacquisition; fullscreen presentation remains open.
- [~] Verify keyboard, mouse, focus loss, resize, audio unlock, and browser pause/resume behaviour.
  Controlled Chromium input now proves focus, WASD movement, relative look, F1, pointer
  release/reacquisition, aspect-preserving resize, and first-click CNA mixer plus dual-track
  startup. Audible hardware-browser output and genuine tab background/foreground recovery remain
  unverified; synthetic CDP freeze/thaw was inconclusive.
- [ ] Package assets into the virtual filesystem with useful missing-asset diagnostics.
- [ ] Reduce download/startup size through mesh/texture/audio choices and measured compression.
- [~] Add quality defaults appropriate to browser GPU/memory limits. The first browser policy uses
  a stable 1280 by 720 render surface with aspect-preserving CSS scaling; measured density and
  memory tiers remain open.
- [~] Run the current terrain/ocean/palm/jungle scene in Chromium/SwiftShader with overview, beach,
  grass-detail, and jungle active-render captures; the grass view requires the full player-bend
  strength on the custom shader. A visible hardware-accelerated interactive pass and another
  browser engine remain open.
- [~] Capture browser console output, `WEBGL2` renderer identity, clean lifecycle, and an
  active-render screenshot. Startup time, memory, and representative frame-time evidence remain.
- [~] Document browser requirements and verified versions without claiming untested browsers.
  Chromium/SwiftShader evidence and explicit gaps are documented; record a visible hardware run
  and a second browser engine before completing this item.

Exit validation:

- A user can load, enter pointer lock, walk, swim, observe wildlife, hear audio, and recover focus
  in an actual browser.
- The environment remains recognizably the same game, with documented quality differences only.

## Phase 7 — Android / OpenGL ES 3 (28 h)

Dependencies: stable input abstraction, scalable UI/debug display, mobile quality tiers.

- [x] Inventory Android SDK, NDK, Gradle, Java, device/emulator, and supported ABI state. The
  available API 35 emulator has no KVM acceleration; no physical device is connected.
- [x] Adapt the CNA template Android shell with island-specific package/application identities,
  API 24 minimum/API 35 target, a launcher icon, no-backup policy, landscape presentation, and an
  isolated read-only-source SDL Java bridge module.
- [x] Add explicit `OPENGLES3` CMake and Gradle configurations plus reproducible arm64-v8a and
  x86_64 packaging instructions. Both ABI APKs build; the default output is arm64-v8a.
- [~] Implement dual-zone touch controls: a floating movement stick, relative look region, stable
  contact ownership, and a minimal asset-free HUD. Deterministic interpreter tests pass and the
  HUD compiles on all three renderers; real-device ergonomic validation remains open.
- [~] Handle pointer IDs, safe display margins, fixed-landscape orientation, resize, and missing or
  released contact cancellation. Core tests cover ownership/cancellation/resize. Pause/resume
  resource restoration still needs a stable emulator or physical device.
- [ ] Add configurable sensitivity and left/right layout considerations where practical.
- [ ] Verify audio focus/lifecycle and resource restoration after backgrounding.
- [ ] Add mobile quality defaults for grass, shadows, wildlife distance, water, textures, and audio.
- [ ] Profile CPU/GPU frame time, memory, thermal behaviour, and loading on a real device when
  available; use emulator evidence only for claims it can support.
- [~] Test install, first launch, relaunch, background/foreground, rotation policy, and clean exit.
  The x86_64 debug APK installed and first-launched on the API 35 `Medium_Phone` AVD, produced a
  2400 by 1080 GLES3 frame, and survived while rendering. The unaccelerated emulator's own System
  UI repeatedly entered ANR/restart states, so touch and lifecycle claims remain open.
- [~] Document device/API/ABI evidence and all unverified hardware classes. Build/install/runtime
  commands and the current emulator limitations are recorded; real-device evidence is absent.

Exit validation:

- On an actual device or explicitly identified emulator, touch traversal and swimming are usable,
  lifecycle transitions do not corrupt rendering/audio, and the complete environment is present.

## Phase 8 — Performance, diagnostics, and robustness (28 h)

Dependencies: representative complete scene on all targets.

### Instrumentation and budgets

- [~] Add debug overlay: FPS, frame time, player coordinates/state, renderer/backend, visible
  terrain/vegetation cells, draw counts if available, wildlife active/simulated counts, and audio
  source counts. The asset-free F1 overlay now covers all existing terrain/player/water/vegetation
  fields plus ambient availability/mute, surf/breeze/jungle/player-water levels, undertow strength,
  played wave/splash counts, positional bird/monkey call counts and voice state, and
  bird/fish/shark/monkey render/simulation statistics plus the five-islet/one-draw distant-scenery
  status and grass-interaction envelope on OpenGL 3.3 and WebGL 2. F2 also reports world-line, nearby
  obstacle, shared-cell, and wildlife-marker counts. A separate uncapped native benchmark records
  warm mean/median/p95/p99 frame intervals and scene counts; a total backend voice counter remains.
- [~] Add toggles for collision, shoreline/current field, vegetation cells, spawn regions, wildlife
  states, LOD, and culling; keep them disabled in normal release presentation. One F2 toggle now
  covers shoreline/current thresholds and vector, nearby collision/player footprints, shared
  cells, terrain normals, tier/state-coloured wildlife headings, bird route/spawn/target regions,
  and audio zones/emitters. Per-layer toggles and detailed culling/LOD/density fields remain.
- [x] Add debug teleport locations and a debug-only free camera. Eight direct F5--F12 locations are
  active only while the diagnostics overlay is visible, including the 62 m current field. Debug
  builds add bounded F3 inspection flight with real private-Xwayland input coverage; a richer
  teleport menu remains an optional separate improvement.
- [~] Establish measured frame-time, draw-call, triangle, texture, mesh, audio, and startup budgets
  per target rather than one desktop-only target. A first five-view 1280 by 720 debug-build Linux
  baseline records frame distributions and tracked scene submissions. A schema-2 moving-route
  report adds population ranges. A schema-1 first-draw report now exposes five startup phases and
  is gated on actual OpenGL 3.3 plus the WebGL 2 timing-log path, but clean repeated route/startup
  timing, release, browser cold-load, Android, memory, audio, and thermal budgets remain open.

### Optimization

- [~] Profile representative beach, dense jungle, hill vista, waterline, and wildlife scenes. The
  first fixed-view native suite covers overview, beach, dense jungle, shore detail, and monkeys;
  a deterministic 676 m native route now forces culling/LOD/contact-shadow refreshes across
  shallows, both coasts, trail, clearings, and dense jungle. Clean repeated traversal timing,
  release configuration, WebGL 2, and physical Android profiles remain open.
- [~] Implement terrain/vegetation cell frustum and distance culling. Terrain and palms share a
  64-cell partition; grass uses 256 finer cells. All are frustum culled, palms/grass have distance
  cutoffs, palms choose staggered 125--145 m LOD per instance after cell rejection, broadleaf
  canopy trees choose staggered 105--129 m LOD per instance, and grass selects near/far geometry
  by cell distance. Per-instance wildlife policy and pop-free vegetation transitions remain open.
- [~] Tune static batching/instancing and verify it does not increase memory or Web/mobile stalls.
  CNA hardware instancing reduces the 330-palm overview to four far-variant draws and bounds mixed
  first-person views to eight near/far variant draws on native OpenGL 3.3 and actual WebGL 2.
  Android GLES 3 rendered the earlier palm/grass scene once in the software-emulated AVD;
  representative revised-geometry cost still requires real-hardware profiling.
- [~] Add mesh/material/animation LOD only where profiles show a worthwhile return. Palms select
  reduced trunk/frond geometry per instance with deterministic transition staggering, broadleaf
  trees select 776/208-triangle meshes per instance, and grass selects 28-blade/five-blade
  clusters; the initial native scene baseline is recorded, while target budgets and true blended
  transitions remain open.
- [x] Throttle distant wildlife and audio updates without visible popping. Shared
  full/reduced/dormant simulation tiers bound agent work; positional calls select only nearby
  non-dormant sources, use one voice, and are held while transient slots are occupied.
- [ ] Reduce transparency/overdraw hotspots, especially grass and layered shoreline effects.
- [ ] Audit texture formats/sizes/mipmaps and create smaller variants only when justified.
- [~] Measure loading and remove redundant runtime generation/conversion. Game-owned wall-clock
  phases now identify deterministic world construction and GPU/content creation separately through
  first draw. Repeat isolated release measurements and profile the dominant generators before
  changing the reproducible runtime-generation architecture.

### Reliability and automated coverage

- [ ] Test malformed/missing terrain, configuration, model, texture, animation, shader, and audio
  inputs for actionable failure diagnostics.
- [ ] Add assertions for finite transforms, valid indices, terrain query domains, animation palette
  sizes, and impossible player/wildlife states in development builds.
- [x] Run ASan and UBSan selectively on first-party core/tests; do not build unused sanitizer sets.
  A host-only two-worker preset instruments the game-owned core, tests, and world-report tool while
  omitting CNA and the graphical application. The first complete pass is clean. LeakSanitizer
  cannot run under the managed process tracer, so it remains a separately documented
  ordinary-terminal check.
- [x] Add deterministic smoke mode plus fixed overview, first-person beach-path, and close-palm
  camera/capture modes for renderer evidence. The native overview now has an independent P6 parser
  and broad non-degeneracy gate with unit-tested malformed/truncated/solid-image failures.
- [x] Rebuild from the documented clean state to detect undeclared files and absolute paths. The
  clean Linux release reconstruction uses a new `/tmp` tree and explicit read-only sibling
  dependencies; all project outputs remain outside dependency repositories.
- [ ] Review active `BUGS.md` workarounds against newer dependency revisions before every baseline
  update; remove only after native behaviour is validated.

Exit validation:

- Representative bottlenecks have before/after measurements.
- No known high-severity crash, corruption, runaway allocation, offshore escape, or silent asset
  failure remains.

## Phase 9 — Long polish and release QA (40 h)

Dependencies: feature-complete builds on all required targets. This phase is mandatory; reaching
Android is not project completion.

### Repeated experience passes

- [ ] Complete at least three recorded whole-island critique passes focused separately on traversal,
  visual composition, and sound/wildlife.
- [ ] Retune acceleration, deceleration, slope response, eye height, mouse/touch sensitivity,
  swimming, and water transitions from play evidence.
- [~] Improve grass close-up shape, density transitions, colour variation, wind coherence, and
  aliasing until it holds up at first-person height. Five-level near curvature, folded blade-side
  colour, shared-sun two-sided lighting, root darkening, and restrained transmission now add
  first-person depth without textures or extra draws; pop-free LOD, richer biome colour, aliasing,
  and physical-device tuning remain.
- [~] Break repeated palm/prop silhouettes and correct floating/sunken placements. Nine paired
  tapered-leaflet stations now replace each near-palm ribbon while widened far crowns preserve the
  envelope; near-only procedural trunk scars and coconut clusters on two variants add a first
  material/content pass without new draws. Prop repetition, root placement review, broader palm
  variation, and true LOD blending remain.
- [~] Refine shoreline contours, wet edge, shallow colour, foam, wave scale, and water/terrain
  seams. The exact foam ribbon, wet terrain edge, tropical depth colour, and subtle movement-driven
  player ripples are functional; repeated whole-island composition tuning remains.
- [~] Refine jungle density versus traversability and strengthen landmarks/clearings/navigation
  cues. The existing open corridor now has a subtle authored footpath material tied to its exact
  placement curve, and the east/west clearing definitions drive distinct surface treatments plus a
  first natural landmark pass made from five rocks and one fallen log around their outer rings.
  Broader landmark variety and whole-route density retuning remain.
- [~] Refine fixed lighting, shadow/contact cues, sky, haze, exposure, and distant scenery as one
  coherent permanent sunny art direction. Terrain/ocean/sky share one sun, grass has two-sided
  blade lighting, the jungle now has derivative-based two-sided facet light, and a bounded contact
  batch grounds major near objects; unified atmospheric/exposure tuning and broader material
  response remain.
- [ ] Improve bird/fish/shark/monkey timing, animation transitions, path variation, and clipping.
- [ ] Improve soundscape variation, spatial placement, transitions, balance, and long-session fatigue.
- [ ] Fix frame pacing, loading hitches, input focus issues, touch ergonomics, and platform-specific
  rendering differences found during real play.

### Release qualification

- [~] Run Linux release build and multiple fresh-user smoke sessions. A clean two-worker release
  tree now builds directly against the read-only sibling dependencies, passes the core/tool gates,
  starts four times on isolated OpenGL 3.3, and produces a visually inspected overview. A longer
  subjective release traversal from a fresh user environment remains open.
- [ ] Run actual-browser release build, cold load, focus/pointer-lock, audio, resize/fullscreen, and
  extended-play tests.
- [ ] Run Android release-like package install, cold/warm launch, lifecycle, touch, audio, thermal,
  and extended-play tests.
- [ ] Compare a representative view matrix across all three renderers and document unavoidable
  differences rather than hiding them.
- [ ] Review every external asset against `ASSET_LICENSES.md` and redistribution requirements.
- [ ] Review `BUGS.md`, known limitations, disabled fallbacks, warnings, and unverified claims.
- [ ] Review repository size and remove only project-owned obsolete artifacts after checking for
  live users/processes.
- [ ] Synchronize README, architecture, controls, Linux/Web/Android instructions, renderer
  differences, debugging, asset pipeline, performance notes, and known limitations.
- [ ] Run formatting/static checks, unit tests, smoke tests, `git diff --check`, and a source-only
  repository inspection.

Exit validation:

- A fresh user can follow the documentation and launch each verified target.
- The full island supports walking, jungle exploration, near-shore swimming with a natural limit,
  wildlife observation, and a cohesive soundscape.
- No documentation claims a platform, renderer, effect, control, or asset that was not actually
  validated.

## Cross-platform rendering contract

Portable baseline:

- GLES 3.0 / WebGL 2 compatible vertex/fragment functionality.
- 16-bit indices where practical; split large batches rather than assuming unrestricted meshes.
- Bounded uniform/bone palettes and texture units.
- Mipmapped compressed or sensibly sized uncompressed textures with explicit fallbacks.
- Stock CNA effects first; custom shaders are authored from one semantic source or kept in a small,
  explicitly equivalent profile set.
- No geometry, tessellation, or compute shader dependency.
- No raw API capability query scattered through gameplay code; centralize feature/quality policy.

Expected scalable differences:

| Feature | Linux OpenGL 3.3 | WebGL 2 | Android GLES 3 |
|---|---|---|---|
| Grass density/distance | High | Medium | Device-tiered medium/low |
| Dynamic shadows | Best supported portable tier | Reduced resolution/distance | Reduced or baked/contact fallback |
| Ocean detail | Full portable layers | Fewer layers if measured | Fewer layers and lower mesh density |
| Wildlife range/count | Full | Reduced if CPU-bound | Reduced with stronger update throttling |
| Texture/audio sizes | Full budget | Download-conscious | Memory/device-conscious |

Any larger difference must be added to the renderer-differences documentation and accepted only
after the same experience remains available.

## Asset pipeline and licensing gates

- No external asset enters a runtime build before source, author, license, modifications, and use
  are recorded in `ASSET_LICENSES.md`.
- Store reproducible source and conversion settings; do not depend on an asset remaining online.
- Do not commit temporary archives, duplicate conversions, source-control-hostile build products,
  or unlicensed placeholders.
- Procedural outputs use stable seeds and can be regenerated.
- Model validation includes scale, axes, normals/tangents, UVs, material slots, skeleton, clip names,
  bounds, and all renderer fallbacks.
- Audio validation includes sample rate/channels, loop boundaries, normalization, license, and
  spatial suitability.

## Test and runtime evidence matrix

| System | Automated evidence | Runtime evidence |
|---|---|---|
| Terrain | Query, normal, slope, deterministic generation tests | Walk every biome and boundary |
| Player | Integration, collision, water transition, water-view fade tests | Feel/tuning sessions at varied frame rates |
| Ocean limit | Distance/current invariants and escape scenarios | Repeated outward swims from multiple shores |
| Placement | Seed/golden counts, slope/biome validity | Pop-in, repetition, collision, composition review |
| Wildlife | State, bounds, spawn, deterministic tests | Observe all species near/far for long loops |
| Animation | Clip/state/palette validity tests | Transition and renderer comparison captures |
| Audio | Config/attenuation/zone logic tests | Headphone/speaker walk-through and long-loop review |
| Assets | Manifest/load/config parsing tests | Missing/corrupt asset diagnostic smoke tests |
| Renderers | Build/shader/package checks | Linux GPU, actual browser, Android device/emulator |

## Risk register

| Risk | Likelihood / impact | Mitigation and trigger |
|---|---|---|
| CNA checkout is not on `next` | Current / high | Exact-commit read-only export or separate read-only worktree; configure rejects mismatch |
| Upstream branch movement breaks build | Medium / high | Record commits, update deliberately, run all active workaround regressions |
| CNA consumer integration assumes repository root | Known / medium | Use isolated game-side CMake adapters; document confirmed defects in `BUGS.md` |
| Web shader/runtime parity gaps | Medium / high | Use shared portable baseline early; actual browser milestone before content is final |
| Android lifecycle/input differences | Medium / high | Integrate package/touch shell before final content; test pause/resume early |
| Grass overdraw and draw calls | High / high | Cell batching/instancing, opaque/masked geometry, aggressive LOD, profile worst views |
| Dense jungle impairs traversal | High / medium | Authored paths/clearings, collision-light small plants, repeated route tests |
| Ocean edge feels like an invisible wall | Medium / high | Multi-stage current/resistance/feedback field and test from diverse shore normals |
| Legal monkey/animal assets unavailable | Medium / medium | Prefer original/simple legal substitute, document replacement path, never use dubious assets |
| Skeletal animation differs by renderer | Medium / high | Validate one small asset on all three paths before building monkey behaviour around it |
| Audio loops become fatiguing | High / medium | Long beds, layered random events, cooldowns, 20+ minute listening passes |
| Scope grows into general engine work | Medium / high | Keep island-specific systems explicit; reject features outside product definition |
| Disk growth from cross-builds/assets | Medium / medium | Measure periodically, reuse approved trees, delete only owned superseded artifacts |

## Debug and developer tooling backlog

- [~] Runtime debug overlay and renderer identity. The F1/`--debug-overlay` panel reports actual
  frame spacing, player/shore/current/water-ripple/water-view state, renderer,
  terrain/vegetation visibility,
  distant scenery, contact-shadow class/triangle counts, LOD counts, draws,
  bird/fish/shark/monkey render LOD and simulation tiers,
  shark presence/activity, monkey state counts, plus audio mix/undertow/event counters. The F2
  world view and deterministic capture now add boundary/collision/cell/wildlife markers; warning
  budgets and deeper views remain open.
- [x] Deterministic seed display/copy. One game-owned `WorldIdentity` source now feeds the four
  authoritative layout seeds to `island-world-report`, deterministic tests, and two F1 rows. F4
  copies a stable English report and distinguishes confirmed readback from unavailable clipboard
  access; native private-display input covers the confirmed round trip, while browser policy can
  fail visibly without hiding the seed values.
- [~] Teleports for beach, cove, jungle, clearing, hill, deep-water limit, fish, shark, and monkeys.
  South beach, jungle trail, two clearings, fish shallows, offshore shark, monkey troop, and the
  active current field are implemented; remaining landmark locations remain.
- [~] Terrain/capsule/contact visualization. Player footprint, nearby solid horizontal proxies,
  the exact nine terrain samples, 1.5 m normals, resolved support rings, player support line, and
  red over-limit slope markers are visible. The latest exact attempted/resolved horizontal step is
  retained with obstacle/terrain/axis-slide/safety-clamp classification; movement history,
  geometric step-up, and depenetration remain.
- [~] Shore distance/current vector visualization. F2 draws the irregular shoreline, all three
  current thresholds, and the player's active inward-current arrow; a scalar field view remains.
- [~] Vegetation cell, culling, LOD, and density visualization. F2 draws nearby shared 8 by 8
  terrain/palm/jungle/prop cells; grass cells, actual frusta, LOD radii, and density fields remain.
- [~] Wildlife spawn region, state, steering, and simulation-tier visualization. F2 heading arrows
  use full/reduced/dormant colours and monkey full-tier behaviour colours. It also draws exact
  fish-school centres, the shark route target, terrain-following monkey home ranges, and active
  monkey movement targets. Three bird-family route envelopes, all 24 exact bird spawn headings,
  and nearby live radial targets are complete; avoidance proposals and non-bird spawn regions remain.
- [x] Audio zone/emitter/source-count visualization. F2 draws the six authoritative terrain-
  following surf/breeze/jungle crossfade boundaries, nearest non-dormant bird/monkey candidates,
  and the exact active pooled emitter; F1 reports the active-source count against the six-source
  ceiling.
- [x] Debug-only free camera and fixed screenshot viewpoints. Command-line overview, first-person
  beach-path, grass-interaction detail, ground-contact, movement-trace, close-palm, jungle-trail,
  natural-prop, driftwood, emergent-reef,
  near-surface-water, active-current, fallen-frond, distant-horizon, airborne bird-detail, and
  shallow-water fish views plus offshore-shark and GPU-skinned-monkey views are implemented and
  used by native/browser captures. Debug builds add bounded F3 flight with frozen player state,
  12/36 m/s movement, automatic F1-visible lifetime, and real private-Xwayland key-input coverage.
- [x] Asset validation command and deterministic world-statistics report. The standard-library-only
  manifest validator gates every file under `assets/`, generator ownership, hashes, provenance,
  and attribution rows. The native `island-world-report` reconstructs the world twice and emits
  terrain, placement, collision, wildlife, seed, and platform-local fingerprint JSON.
- [~] Frame-time capture and representative scene benchmark route. `--benchmark` now performs 120
  warm-up and 600 sampled uncapped intervals with JSON mean/median/p95/p99 output, while
  `tools/run_native_benchmarks.sh` covers five fixed representative scenes plus a deterministic
  676 m moving rail. Schema-2 reports aggregate per-frame scene-population ranges. Clean repeated
  route timing and Web/Android equivalents remain open.

## Documentation deliverables

- [~] `README.md`: concept, controls, quick start, and honest status exist; representative
  screenshots and release-facing presentation remain.
- [x] `PLAN.md`: this living ledger.
- [x] `BUGS.md`: defect schema and current no-confirmed-defects record exist; update whenever
  evidence qualifies a dependency defect.
- [x] `ASSET_LICENSES.md`: the original procedural baseline is recorded and cross-checked against
  the versioned asset manifest; it remains a maintained gate before external content enters.
- [x] `docs/build-linux.md`: prerequisites, exact dependencies, configure/build/run/test.
- [x] `docs/build-web.md`: SDK, package, serve, browser requirements, validation, and explicit
  unverified interaction/performance scope.
- [x] `docs/build-android.md`: SDK/NDK/Gradle, ABI, install/run/logging, lint, evidence, and explicit
  emulator/real-device validation limits.
- [~] `docs/architecture.md`: current subsystem boundaries, ownership, update/render flow, and
  coordinates exist; extend as systems are added.
- [~] `docs/controls.md`: desktop/Web and implemented Android dual-zone touch controls are
  documented; configurable sensitivity/layout and real-device ergonomic notes remain open.
- [x] `docs/asset-pipeline.md`: current generation model, versioned manifest schema, validation,
  world report, external intake, deterministic conversion requirements, and limitations.
- [~] `docs/renderer-differences.md`: current visual/input/runtime evidence and explicit gaps are
  documented; matched Android captures and representative cross-target measurements remain open.
- [~] `docs/debugging.md`: overlay fields, direct QA teleports, deterministic captures, and the
  native performance-report workflow are documented; deeper world-space visualizers and
  common-failure guidance remain open.
- [~] `docs/performance.md`: native method, five representative fixed scenes, the initial Linux
  baseline, startup phase/report semantics, interpretation rules, and open budget work are
  documented. Release/Web/Android, memory, repeated cold loading, audio, and thermal budgets
  remain open.
- [x] `docs/known-limitations.md`: current environment, traversal, content, platform, and tooling
  limitations are recorded without claiming planned systems.

## Current execution queue

Work next, in order:

1. Tune the complete four-layer soundscape, current-responsive undertow mix, both accent pairs,
   and the pooled positional bird/monkey voice through a 20-minute headphone/speaker route.
   Numerical headroom, exact-source signal reports, disposable validated WAV previews, actual
   Linux playback, and the six-source ceiling are verified; subjective masking and fatigue are not.
2. Continue long subjective observation of the GPU-skinned monkey troop and its restrained
   27--50 second positional call cadence. Three silhouettes, four palettes, short clip crossfades,
   shared trunk/rock/log avoidance, and two call phrases are complete first passes. Keep bird,
   fish, and shark layers under subjective observation during play.
3. Improve ground/contact variation and enrich the forest-floor silhouettes and sparse driftwood
   layer only where composition review needs it. The first low-bush silhouette follow-up now adds
   paired folded leaves over smaller near lobes without touching its far tier; near ground-plant
   variants now include both an upright six-leaf form and a five-leaf folded split form. The
   dedicated cross-renderer QA camera targets the latter and verifies a nonzero per-instance near
   LOD population.
   Near land and reef rocks now add restrained, texture-free strata and lichen only in their near
   LOD, without new geometry, textures, instances, or submissions.
   Near fallen logs now add distance-faded local-space bark fissures and subtle irregular growth
   rings through the same material path, also without new geometry, textures, or submissions.
   Near sun-bleached driftwood now adds a softer faded longitudinal grain while leaving cut ends
   and far meshes plain. Its third near variant now uses a curved three-limb fork, while the
   dedicated native/WebGL QA camera proves a nonzero per-instance near LOD. Rocks, logs, and
   driftwood now enforce range and LOD from their actual positions after conservative cell bounds.
   The portable sand/soil, wet-edge, and slope-material pass, the initial small-plant layer, a
   first near-field shell/pebble substrate layer, a first grove-associated fallen-frond layer, and 14 emergent
   shallow-reef outcrops are complete. Texture-free near-field substrate microvariation and sparse
   curved interior leaf litter and a first fixed-sun floor-dapple treatment now break up exposed
   jungle-floor gaps. The vegetation-reserved primary corridor now has a subtle shared broken-edge
   dry-earth material cue under its short grass. The east/west clearings now share their placement,
   QA, and distinct warm-dry/leaf-rich terrain definitions. Five authored rocks and one fallen log
   now give their outer rings different natural silhouettes while preserving the central walking
   room; broader landmark variety, prop diversity, contact cues, authored biome variation, and
   material tuning remain.
   The first portable near-surface swimming veil and movement-driven surface ripples are also
   complete; defer waterline geometry, underwater fog, and refraction until a similarly portable
   design is justified.
4. Extend the completed F2 shoreline/current, terrain-contact, nearby collision, shared-cell,
   wildlife-heading/navigation, bird route/spawn/live-target, and audio-zone/emitter visualization
   with individual culling/LOD/density fields only where they materially improve debugging. F1 now
   reports local static-collision candidates, nine terrain support samples, wildlife
   marker/target/home-range counts, bird route-family/spawn/target counts, and live audio sources
   against their complete populations/budgets.
5. Qualify touch traversal, audio focus, pause/resume, and performance on a physical Android device or a stable
   KVM-accelerated emulator; the current software AVD is evidence for rendering only.
6. Perform a visible hardware-accelerated Web listening/play pass and genuine tab
   background/foreground test; automated first-gesture audio initialization is complete.
7. Continue palm root-placement review and observe the segmented-leaflet versus widened-ribbon
   125--145 m transition during traversal. The first silhouette and near material/coconut passes
   are measured; do not add more geometry without physical/mobile evidence.
8. Observe the retained branch-supported broadleaf crowns and staggered 105--129 m transition
   during traversal. The first silhouette/LOD pass is measured; further canopy geometry should be
   justified by physical-mobile evidence or a clear close-view defect. The first fixed-sun
   geometric facet-light pass is visually matched on native OpenGL and actual WebGL 2; observe its
   readability and cost on physical mobile before extending the technique to more layers.
9. Record two isolated debug and release startup reports after closing other GPU clients, then use
   the phase split to investigate only demonstrated world/content bottlenecks. Add browser cold-load
   and physical-Android launch evidence before assigning cross-target startup budgets.

## Validation ledger

### 2026-10-05 — Bootstrap and first traversal slice

- Qualified and locked CNA `next` at `b0e97bb1bb87`, Sharp Runtime `next` at `21c06910f7cf`,
  EasyGL `develop` at `1edf4c1a4708`, and MetaGL `develop` at `40829ca76a4e` without editing any
  dependency repository.
- Configured the reusable `build/` tree for C++23 and `OPENGL33` with shared ccache. All operations
  that compiled Sharp Runtime used at most two jobs.
- Built `island-simulator` and `island_core_tests` successfully with GCC 14.2.0 and warnings as
  errors for first-party code.
- Passed `IslandCoreTests`, including deterministic terrain, shoreline variation, movement,
  walking/swimming transitions, long-frame clamping, and ocean-boundary invariants.
- Passed `IslandGameSmoke` and `IslandGameCapture` under CNA's isolated Weston/Xwayland runner.
  Runtime reported the CNA `OPENGL33` backend over Mesa OpenGL 4.6 Core Profile.
- Inspected the deterministic capture after increasing terrain resolution and correcting ocean
  occlusion of the finite underwater mesh boundary.
- Android/GLES 3 remains uncompiled and unverified; no Android portability claim is made.
- Project-owned workspace usage after this milestone was approximately 2.0 GiB, primarily the
  single reusable native build tree and read-only CNA source export. A one-time bare all-target
  build produced unnecessary CNA tool objects; subsequent builds are target-limited and CNA is now
  excluded from the default all-target surface.

### 2026-10-05 — First WebGL 2 qualification

- Activated the existing Emscripten SDK and added the `web-webgl2` configure/build presets using
  the closed `build-probe/` directory and at most two compile jobs.
- Extended the reproducible CNA `next` export to include only the three pinned SDL snapshots after
  verifying their gitlinks against `next`; all dependency repositories remained read-only.
- Kept the Emscripten SDL cache in project-owned `build/cna-sdl-prebuilt-emscripten` rather than
  writing generated content into the CNA export.
- Built the complete HTML/JavaScript/WebAssembly package successfully. The initial Wasm payload is
  approximately 7.1 MiB before later release-size optimization.
- Passed a real Chromium/SwiftShader WebGL 2 lifecycle test: CNA reported `WEBGL2`, created an OpenGL
  ES 3.0/WebGL 2.0 context, rendered four frames, returned from `Game::Run()`, and exited cleanly.
- Passed a second live browser render and visually inspected its screenshot; terrain, sky, lighting,
  and first-person camera output were present without a shader/runtime exception.
- Browser pointer lock, real keyboard/mouse play, hardware-GPU performance, audio unlock, focus
  recovery, resize, and non-Chromium engines remain explicitly unverified.

### 2026-10-05 — Terrain cells and first portable ocean pass

- Split the 241 by 241 source terrain grid into 64 independently bounded render cells while
  preserving exact shared seams, 61,504 total vertices, 115,200 triangles, and 16-bit indices.
- Added deterministic layout tests for cell coverage/seams and observed 58 of 64 cells in the wide
  overview view; first-person views can reject more cells through the same frustum path.
- Added a 160 by 160 ocean grid (25,921 vertices, 51,200 triangles) with portable 16-bit indices,
  analytic vertex displacement, five-layer fragment normals, Fresnel sky response, sun glint,
  depth-driven tropical colour/transparency, moving shore foam, and distance haze.
- Kept CNA and Sharp Runtime unchanged. The ocean consumes CNA's public portable shader-package
  extension with explicit GLSL Desktop 330 and GLSL ES 300 variants and an isolated stock-effect
  fallback with actionable diagnostics.
- Rebuilt and passed core tests plus isolated native `IslandGameSmoke`/`IslandGameCapture` under
  `OPENGL33`; the runtime selected the full custom shader and the capture was visually inspected.
- Rebuilt the Emscripten package and passed actual Chromium/SwiftShader WebGL 2 lifecycle and live
  rendering tests. The harness now requires the full ocean shader, uses a deterministic live
  `--overview` camera at 1280 x 720, and records a clean ocean/shoreline screenshot.
- OpenGL ES 3 / Android selection of the GLSL ES variant remains uncompiled and unverified.

### 2026-10-05 — First procedural palm and collision pass

- Added a renderer-independent deterministic vegetation layout with 330 palms constrained by
  terrain height, slope, and shore distance. Smoothed cluster noise varies density while a curved
  south-to-north walking corridor, two clearings, and the player spawn remain reserved.
- Added four original procedural palm meshes with curved tapered eight-sided trunks, varied height
  and lean, 10--12 ribbon fronds, crown/yaw/scale variation, and no external asset dependency.
- Grouped palms into the terrain's 64 cells, added frustum/distance rejection, and used CNA's
  capability-checked multi-stream hardware instancing. The deterministic overview draws 299 visible
  palms from 25 cells in four submissions; a per-palm fallback remains available.
- Added one visually matched circular trunk proxy per palm to the substepped player collision path.
  Automated coverage checks placement determinism and validity, route/clearing reservations,
  obstacle correspondence, and rejection of attempted movement through a trunk.
- Passed core tests and all three isolated native OpenGL tests: lifecycle smoke, overview capture,
  and first-person beach-path capture. The beach camera now derives its X coordinate from the
  authoritative path centre and the inspected view remains unobstructed.
- Rebuilt the Emscripten package and passed actual Chromium/SwiftShader WebGL 2 lifecycle and live
  rendering checks. The harness now requires all 330 palms and hardware instancing as well as the
  portable ocean shader; the inspected overview matches the native scene composition.
- Grass, broadleaf jungle layers, palm wind/LOD/contact shadows, and a collision broad phase remain
  open. Android/GLES 3 is still uncompiled and unverified.

### 2026-10-05 — First instanced mesh-grass pass

- Added a renderer-independent layout of 18,556 deterministic grass-cluster anchors constrained by
  terrain height, shore distance, and slope. Smoothed cluster noise plus height, spread, lean,
  colour, and wind-phase variation avoids a uniform grid; the primary path uses denser short turf.
- Added shared 28-blade near and five-blade far meshes made from tapered multi-segment geometry,
  avoiding alpha-test overdraw and crossed-billboard dependence at first-person height.
- Added a 256-cell grass partition with frustum/distance rejection, near/far mesh selection, and one
  hardware-instanced draw per visible cell. The first-person beach view processes 2,954 patches in
  15 draws; the distant overview correctly rejects all grass rather than showing aliased speckles.
- Added explicit GLSL 330 / GLSL ES 300 vertex wind, terrain-normal basis reconstruction, per-instance
  variation, and distance merging into the ground colour through CNA's portable shader package.
  A static stock-effect fallback remains available with an actionable diagnostic.
- Replaced the initial baked-per-cell geometry experiment with instanced clusters after visual QA:
  the revised path increased close grass density while reducing native capture startup/runtime from
  roughly 2.35 seconds to 1.25 seconds in the same isolated test shape.
- Passed deterministic core tests and the complete native OpenGL smoke/capture suite. Rebuilt and
  ran Chromium/SwiftShader WebGL 2; the browser harness explicitly requires the full instanced wind
  shader, a nonzero first-person grass draw, palm instancing, and the ocean shader. Both overview
  and first-person browser captures were visually inspected.
- Remaining grass work includes pop-free LOD transitions, closer blade/material variation, measured
  frame-time/overdraw budgets, biome authoring, and Android GLES 3 qualification.

### 2026-10-05 — First palm wind, contact, and mesh-LOD pass

- Replaced the static palm effect with explicit portable GLSL 330 / GLSL ES 300 shaders. Wind
  weights and deterministic per-frond phases produce a bounded two-frequency breeze while trunks
  and roots remain stable; an actionable stock-effect fallback remains available.
- Added near and far meshes for all four palm variants. Near palms retain the curved eight-sided
  trunk and 10--12 seven-segment fronds and gain five small root wedges; far palms use a six-sided
  trunk and seven three-segment fronds. Cell-distance selection keeps batching bounded.
- The deterministic overview reports 299 visible palms split into 35 near and 264 far instances
  across 25 cells and eight draws. The first-person beach view reports 301 visible palms split into
  106 near and 195 far instances, also across 25 cells and eight draws.
- Rebuilt with first-party warnings as errors and passed isolated native overview and first-person
  captures. Both captures were visually inspected for transform, culling, contact, and shader
  regressions.
- Rebuilt the Emscripten package and passed actual Chromium/SwiftShader WebGL 2 lifecycle,
  overview, and beach runs. The harness now requires the custom palm shader in every run as well as
  the ocean and grass shaders; the beach capture was visually inspected for native/Web parity.
- This is a technical first pass, not final art. Rich bark/frond materials, self/contact shadowing,
  pop-free LOD transitions, silhouette polish, measured performance, and Android GLES 3 remain
  open.

### 2026-10-05 — First portable runtime diagnostics pass

- Added an opt-in F1 developer overlay using only CNA public APIs and an original runtime-generated
  5 by 7 bitmap atlas. Normal presentation starts with it hidden; `--debug-overlay` enables
  deterministic QA without adding an external font asset or content-pipeline dependency.
- Added smoothed wall-clock frame duration/FPS, player position and locomotion, support height and
  slope, offshore distance, normalized current strength, renderer identity, terrain cell counts,
  and palm/grass visibility, LOD, cell, and draw statistics.
- Promoted normalized ocean-current strength to a renderer-independent `PlayerMotion` query and
  added core regression coverage for inactive walking/near-shore states and full outer strength.
- Added an isolated native overlay capture test. Core tests passed 1/1 and all four native OpenGL
  lifecycle/capture tests passed 4/4; the overlay capture was visually inspected for legibility and
  correct beach-scene values.
- Rebuilt the Emscripten package and passed the real Chromium/SwiftShader WebGL 2 harness. Its beach
  pass now requires the overlay activation diagnostic and the inspected screenshot confirms the
  same font, panel, and values render through WebGL 2 SpriteBatch composition.
- Startup smoke captures are too short for representative FPS evidence. Hardware-GPU profiling,
  frame histograms/budgets, world-space visualizers, teleports, wildlife/audio counters, and Android
  GLES 3 remain open.

### 2026-10-05 — First controlled Web input and presentation pass

- Added a production Emscripten shell with English controls, an explicit loading state, a 1280 by
  720 render surface, and aspect-preserving fit for arbitrary browser windows.
- Requests pointer lock directly from the first canvas mouse gesture to satisfy browser activation
  policy. The game synchronizes its capture state with CNA's relative-mouse state so browser Escape
  or focus loss does not prevent a later click from reacquiring control.
- Extended the Chromium/SwiftShader harness beyond rendering: it proves canvas focus and pointer
  lock, drives W far enough to measure player displacement, changes yaw/pitch through relative
  mouse input, toggles F1, verifies explicit unlock and reacquisition, and resizes the browser to
  1000 by 600 while the canvas preserves 16:9 at 1000 by 563. The latest run used Chrome
  152.0.7977.82.
- Opened the actual generated `island-simulator.html`, not only the instrumented test shell, and
  required its Ready state, 1280 by 720 canvas, live WebGL 2 context, game renderer/terrain output,
  and first-click pointer lock. The deployable shell itself now has automated coverage.
- Raised the native and Web presentation baseline from the framework default 800 by 480 to 1280 by
  720. All four isolated OpenGL lifecycle/capture tests still pass, the 1280 by 720 native overlay
  capture was visually inspected, and the full WebGL 2 lifecycle/render/input harness passes.
- Chromium DevTools synthetic `frozen` to `active` did not resume requestAnimationFrame in this
  headless setup even when the page was brought forward. This is insufficient evidence of a CNA
  defect and does not qualify genuine tab lifecycle; it remains an explicit manual-browser task.
- Audio unlock, visible hardware acceleration, extended play, fullscreen, and a second browser
  engine remain open. Android/GLES 3 is still uncompiled and unverified.

### 2026-10-05 — First Android GLES 3 package and runtime pass

- Audited CNA `next` Android entry-point, SDLActivity, renderer, template, and Gradle patterns while
  keeping CNA and Sharp Runtime strictly read-only. Added a C++23 `OPENGLES3` preset and an
  island-owned Gradle application targeting API 35 with minimum API 24.
- Android builds the shared game entry point as `libmain.so`. The application uses an isolated
  `sdlbridge` library module whose Java source points at pinned CNA's read-only SDL snapshot, so
  application-owned Java/resources keep a strict lint boundary without copying or patching SDL.
- Built the complete debug APK for both arm64-v8a and x86_64 with NDK 29.0.14206865, SDK CMake
  4.1.2, Gradle 8.12, JDK 21, and no more than two workers. The current arm64 package is about
  17 MiB, requires GLES 3.0, contains only the selected ABI, and verifies with the Android debug
  v2 signature. Application lint passes.
- Added renderer-independent two-zone touch interpretation with stable pointer ownership, floating
  movement origin, dead zone, clamping, relative look, resize/reset, and missing/released-contact
  handling. Added a portable asset-free SpriteBatch HUD with display-edge margins; core regression
  tests cover simultaneous contacts, proportional movement, look deltas, ownership, cancellation,
  dead zone, clamping, presentation coordinates, and resize.
- Installed and launched the x86_64 APK on the API 35 `Medium_Phone` AVD. SDL loaded `libSDL3.so`
  and `libmain.so`, created a 2400 by 1080 EGL/GLES surface, and the captured frame visibly contains
  the island terrain, palm population, and mesh grass. No application crash or shader exception
  was observed during the render run.
- The host exposes no `/dev/kvm`; QEMU TCG plus SwiftShader rendered at roughly 0.6--1.7 seconds per
  frame and the Google Play system image repeatedly produced System/System UI ANRs independent of
  the game. A clean first frame is valid GLES3 evidence, but touch ergonomics, background/foreground
  restoration, audio focus, thermal behaviour, and performance remain unqualified until a stable
  KVM emulator or physical device is available. Root inside the guest would not address this host
  virtualization constraint.
- After touch/HUD integration, core tests pass, all four isolated Linux OpenGL smoke/capture tests
  pass, the Emscripten package rebuilds, the actual Chromium WebGL 2 render/input harness passes,
  and both Android ABIs rebuild. No qualifying CNA or Sharp Runtime defect was found.

### 2026-10-05 — First layered broadleaf jungle pass

- Added renderer-independent deterministic placement for 281 canopy trees and 1,822 medium plants.
  Height, slope, shoreline distance, two-scale cluster fields, and authored route/clearing masks
  shape the interior; all four tree and four understory variants have tested bounds and stable seed
  output.
- Added four procedural broadleaf canopy meshes with tapered trunks and clustered opaque crowns,
  plus four curved-strip fern/large-leaf meshes. Separate near/far geometry and canopy/understory
  distance limits retain close silhouettes while bounding distant and mobile work.
- Added cell frustum/distance culling and variant-level hardware instancing. The fixed jungle camera
  sees 109 of 281 canopy trees and 429 of 1,822 understory plants in 15 submissions on WebGL 2;
  the overview draws 225 distant canopy trees in four submissions and rejects all understory.
- Added a portable GLSL 330 / GLSL ES 300 weighted-wind shader and actionable stock-effect fallback.
  Native OpenGL 3.3 and actual Chromium/SwiftShader WebGL 2 select the full shader; the Web harness
  now requires nonzero canopy and understory rendering and records a dedicated jungle screenshot.
- Combined canopy trunk proxies with palm proxies for player collision while leaving leaves and
  understory non-solid. Added jungle population, placement, scale, slope, route reservation, and
  obstacle correspondence regression tests.
- Added a deterministic jungle capture to the isolated native suite and direct F5--F8 debug
  teleports for the south beach, jungle trail, and two clearings. The debug overlay now reports
  jungle visibility, cell, and draw counts.
- Core tests pass, all five isolated OpenGL lifecycle/capture tests pass, and the strengthened
  Chromium WebGL 2 lifecycle/render/input harness passes. The current arm64 GLES 3 APK and Android
  lint also build successfully; Android runtime validation of the new jungle layer remains open
  because the available unaccelerated emulator is not a stable qualification target.
- Visual review confirms the intended first dense interior layer and native/Web parity. Crown
  silhouettes and materials are deliberately still first-pass; low bushes, rocks, fallen wood,
  grounding cues, prop separation, pop-free LOD transitions, and measured mobile budgets remain.
- Removed the superseded 1.1 GiB standalone Android build tree and 945 MiB x86_64 NDK intermediate
  tree after recording their evidence. The reusable native/Web trees, current arm64 intermediate,
  signed APK, and shared compiler cache remain; the removed outputs are reproducible from presets.

### 2026-10-05 — First natural-prop composition pass

- Added deterministic placement for 152 irregular rocks across coastal/interior ground and 11
  fallen logs on low-slope jungle floor. Placement is height/shore/slope/cluster aware, separates
  props from palm and canopy trunks, preserves routes/clearings, and validates the complete log
  footprint rather than only its centre.
- Added four faceted rock variants and three bark/cut-end/moss log variants with independent
  near/far meshes, opaque contact geometry, cell frustum/distance culling, variant-level hardware
  instancing, and a stock-effect fallback. The overview sees 39 rocks in four draws; the prop-detail
  WebGL view sees three logs plus 33 rocks in 11 draws.
- Added a portable GLSL 330 / GLSL ES 300 prop shader and extended the real Chromium harness to
  require its non-fallback instanced path. A dedicated native/Web prop camera records the log close
  view and proves nonzero log rendering; inspected native and Web captures agree.
- Added conservative collision for substantial rocks and seven sampled proxies along each fallen
  log. Smaller dressing rocks remain non-solid. Core tests cover deterministic populations,
  placement bounds, complete route clearance, variant/scale limits, and exact proxy correspondence.
- Added prop visibility/cell/draw counters to the debug overlay. The complete core suite and six
  isolated OpenGL lifecycle/capture tests pass, and the expanded Chromium lifecycle/render/input
  harness passes. The arm64 GLES 3 package compiles and lint passes; runtime of these new props on
  stable Android hardware remains open.
- Visual review found the first log deliberately simple but correctly grounded and readable, with
  cut end and subtle moss. Later passes should add branch stubs, finer material variation, smaller
  beach driftwood, rock grouping/shore wetness, and measured mobile budgets rather than multiplying
  geometry blindly.

### 2026-10-05 — First fixed sunny sky pass

- Added a camera-centred procedural sky sphere with a portable GLSL 330 / GLSL ES 300 shader. The
  fixed presentation combines a tropical horizon-to-zenith gradient, warm sun disc and halo, and
  subtle static high cloud wisps without introducing a clock, weather, or day/night state.
- Added a vertex-coloured stock-effect gradient fallback with an actionable startup diagnostic.
  The sky pass temporarily disables depth, renders before the world, restores normal depth state,
  and stays centred on the camera so the finite sphere cannot expose a travel boundary.
- Centralized the fixed sun direction in island-simulator and applied it to terrain lighting,
  ocean glint, and the sky disc. Visual review exposed and corrected the ocean's previously
  mirrored horizontal sun direction.
- Fixed a game-owned `class`/`struct` forward-declaration mismatch that native GCC tolerated but
  Emscripten Clang rejected under warnings-as-errors. No framework defect was involved and CNA and
  Sharp Runtime remained unchanged.
- Core tests pass and all six isolated OpenGL lifecycle/capture tests pass. Overview, beach,
  jungle, and prop captures were visually inspected after the change.
- Rebuilt the WebAssembly package and passed the full Chromium/SwiftShader WebGL 2 lifecycle,
  render, deployment, input, and screenshot harness. Every browser scene now requires the fixed
  sky shader diagnostic, preventing a silent custom-effect fallback.
- Rebuilt the arm64-v8a Android APK and passed application lint with the GLSL ES sky path included.
  Runtime evidence for the new sky remains open until a stable KVM-backed emulator or physical
  device is available; compilation is not reported as runtime qualification.

### 2026-10-05 — First sparse beach-driftwood pass

- Added 28 deterministic sun-bleached driftwood pieces in the narrow dry-beach belt. Placement
  follows a low-density cluster field, derives a shoreline-tangent orientation, preserves authored
  routes and tree clearance, and validates five samples along every complete footprint against
  shore distance, slope, and support-height variation.
- Added three original bent-tube mesh variants with one or two branch stubs and independent near/far
  geometry. They share the existing portable prop shader, cell culling, variant instancing, and
  distance LOD instead of introducing another rendering subsystem.
- Kept the small wood non-solid so decorative beach detail cannot become a first-person snag
  hazard. Core regressions cover deterministic population/data, authored size and variant bounds,
  complete-footprint placement, route clearance, and the invariant that driftwood creates no
  collision proxy.
- Extended startup and overlay diagnostics with visible/total driftwood counts. Added a dedicated
  `--viewpoint driftwood` camera and native capture because the interior-facing beach camera could
  count the new props without showing them clearly.
- Core tests pass and all seven isolated OpenGL lifecycle/capture tests pass. The dedicated native
  view was visually inspected and shows a grounded branch silhouette against sand and ocean.
- Rebuilt the WebAssembly package and passed the expanded Chromium/SwiftShader WebGL 2 lifecycle,
  deployment, input, and five-scene image suite. The dedicated browser capture matches the native
  view and must report nonzero driftwood through the full custom prop shader.
- Rebuilt the arm64-v8a Android APK and passed application lint. Runtime qualification of the new
  geometry remains open until a stable KVM-backed emulator or physical device is available.

### 2026-10-05 — First low-bush jungle layer

- Added 565 deterministic low bushes as a distinct third jungle layer between mesh grass and the
  existing medium understory. Height, slope, shoreline distance, broad/fine cluster fields, and
  route/clearing reservations shape their distribution; they remain intentionally non-solid.
- Added three original bush variants assembled from offset opaque low-poly leaf lobes. Independent
  near/far geometry, bounded shader wind, cell culling, variant instancing, and distance LOD reuse
  the portable jungle path rather than adding transparency overdraw or per-bush submissions.
- Core regressions cover deterministic population/data, biome belt, ground suitability, route
  clearance, variant range, authored scale range, and the existing canopy-only collision invariant.
- Extended startup and overlay diagnostics with visible/total bush counts. The deterministic jungle
  camera sees 102 of 565 bushes; adding three near/far variants raises the same visible scene from
  15 to 21 jungle draw submissions, a bounded six-draw increase.
- Core tests pass and all seven isolated OpenGL lifecycle/capture tests pass. Native visual review
  confirms the intended lower volumetric layer while retaining open walking routes; richer bush
  silhouettes, materials, and contact cues remain a later polish task.
- Rebuilt the WebAssembly package and passed the complete Chromium/SwiftShader WebGL 2 lifecycle,
  deployment, input, and five-image suite. The jungle pass now requires nonzero canopy,
  understory, and bushes through the expanded custom wind/LOD shader; the inspected browser view
  matches native composition and counts.
- Rebuilt the arm64-v8a Android APK and passed application lint. Runtime qualification of bush
  density, shader motion, and performance remains open until stable Android hardware is available.

### 2026-10-05 — First procedural ambient soundscape

- Audited CNA `next` raw PCM, authored-loop, `SoundEffectInstance`, dynamic-stream, mixer-device,
  and example usage without changing CNA or Sharp Runtime. Selected small resident loops rather
  than dynamic streaming for the first continuous beds to reduce underrun and lifecycle risk.
- Added original deterministic ten-second, 22.05 kHz mono PCM16 surf and light-canopy-breeze loops.
  Circular filtered noise and periodic swell/gust envelopes avoid short synthetic tones; peak
  normalization retains headroom and no external asset or runtime file is required.
- Added a renderer-independent mix model driven by the authoritative signed shoreline distance and
  swimming state. Surf dominates the beach and near-shore water, canopy breeze grows inland, and a
  1.8-second exponential transition prevents abrupt zone changes. M toggles reversible mute.
- Added a CNA-only playback owner with the initial two persistent looping instances, explicit startup state
  verification, actionable startup/runtime failure diagnostics, resource cleanup, and silent visual
  fallback. Web defers creation until the first click/touch/movement key; Linux and Android start
  after content initialization.
- Added core regression coverage for deterministic samples, exact rate/duration, peak headroom, RMS
  energy, loop-boundary delta, zone ordering, swimming emphasis, and smooth convergence. Core tests
  pass, and all seven isolated OpenGL lifecycle/capture tests pass with audio intentionally disabled
  in four-frame automation.
- Opened the Linux game on the real display and CNA opened a 44.1 kHz stereo device; both ambient
  tracks reached `Playing`. The live build remains open for operator play/listening with F1 mix
  diagnostics.
- Rebuilt the Emscripten package and extended the actual deployable-page harness to require first-
  gesture audio startup. Chrome 152/SwiftShader opened CNA's 44.1 kHz mixer, started both tracks,
  retained pointer lock, and passed the complete render/input/five-image suite.
- Rebuilt the arm64-v8a APK with both procedural beds and passed application lint. Android audio
  playback/focus remains explicitly unqualified because the host has no KVM and no physical device
  is connected. No qualifying CNA or Sharp Runtime defect was found.
- At that milestone, remaining audio work included subjective balance, a 20-minute fatigue pass,
  additional wave variation, richer jungle texture, distinct entry/exit water details, positional
  wildlife, focus/lifecycle recovery, and measured source/memory budgets.

### 2026-10-05 — First forest-floor plant layer

- Added 2,225 deterministic small rosette/forest-floor plants as the fourth jungle vegetation
  layer. Terrain height, slope, signed shore distance, broad/fine cluster fields, and authored
  route/clearing reservations constrain placement; the plants remain deliberately non-solid.
- Added three original curved opaque-leaf variants with near/far meshes, weighted light-breeze
  motion, cell culling, distance LOD, and variant-level hardware instancing through the existing
  jungle renderer. No transparency overdraw or extra rendering subsystem was introduced.
- Added regression coverage for population budget, bitwise seed repeatability, biome belt, ground
  suitability, route clearance, variant/scale bounds, and the canopy-only jungle collision rule.
- Extended startup, first-frame, and F1 overlay diagnostics with total/visible ground plants. The
  deterministic jungle camera draws 129 of 2,225 plants and increases jungle submissions only from
  21 to 24; native and WebGL 2 captures show readable close rosettes without distant speckle.
- Core tests pass and all seven isolated OpenGL lifecycle/capture tests pass. The rebuilt
  Chrome/WebGL 2 harness requires the expanded canopy/understory/bush/ground shader and a nonzero
  ground-plant population in the active jungle view; the complete render/input/audio suite passes.
- Rebuilt the arm64-v8a Android APK and passed application lint with the expanded GLSL ES jungle
  path. Mobile runtime density/performance remains unqualified without KVM or a physical device.
- Remaining polish includes greater leaf silhouette/material diversity, contact darkening, biome
  colour variation, pop-free LOD transitions, and representative physical-mobile profiling.

### 2026-10-05 — Sparse stronger-wave accents

- Added an original four-second mono PCM16 wave wash with an explicit silent attack/release, peak
  headroom, and no external asset. Core tests verify exact format/duration, audible RMS energy, and
  zero-valued endpoints.
- Added a deterministic sparse scheduler: the first accent waits 7--13 seconds, subsequent accents
  wait 14--32 seconds, and conservative volume/pitch/pan variation prevents a mechanical identical
  repeat. Four-second accents cannot overlap by construction.
- Virtualized expired accents while inland surf is below the audible threshold. At this milestone,
  the complete audio source budget was two persistent beds plus at most one one-shot; returning to
  shore permits a waiting accent without continuously creating silent tracks.
- Added graceful one-shot acquisition/exception diagnostics while leaving both ambient beds alive.
  F1 reports the played-wave count, and the real Linux CNA device accepted the first scheduled
  one-shot after both loops reached `Playing`.
- Passed the core suite and all seven isolated OpenGL 3.3 GPU smoke tests. The full Chromium/WebGL 2
  harness passed and confirms accent-effect creation after its first user gesture; it does not wait
  long enough to qualify audible browser one-shot playback.
- Rebuilt and linted the arm64-v8a Android APK successfully. Android audible playback and focus
  behaviour remain explicitly unqualified because this host has no KVM device and no physical
  Android device is connected. No qualifying CNA or Sharp Runtime defect was found.

### 2026-10-05 — Player wading and swimming audio

- Added an original deterministic six-second mono PCM16 water-motion loop with circular filtering,
  portable 22.05 kHz format, conservative headroom, and no external asset. Core tests cover exact
  format/duration, deterministic regeneration, audible energy, and loop-boundary continuity.
- Added a renderer-independent water mix driven by authoritative player support depth, walking or
  swimming mode, and actual horizontal velocity. Dry walking and stationary water remain silent;
  shallow wading and swimming use distinct gain/pitch ranges.
- Used one persistent loop with 140 ms attack, 340 ms release, and 420 ms pitch smoothing instead
  of retriggered short splashes. This avoids rapid repetition and hard edges as input, depth, and
  locomotion state change; sparse entry/exit detail remains a later polish item. All current source
  PCM payloads total 1,323,000 bytes (about 1.26 MiB) before backend bookkeeping.
- Integrated the third loop through CNA `SoundEffectInstance`, the global M mute, graceful audio
  fallback, and F1 `WATER` gain diagnostics. The real Linux device transitioned all three loops to
  `Playing`; the newly rebuilt interactive OpenGL 3.3 game is open for operator listening.
- Passed the core suite and all seven isolated OpenGL 3.3 GPU tests. The complete Chromium/WebGL 2
  harness passed and now requires first-gesture creation/startup of the water loop in addition to
  both ambient beds and the wave effect. Visual review confirms the added `WATER` diagnostic fits
  the 1280 by 720 browser overlay without clipping.
- Rebuilt and linted the arm64-v8a APK successfully with the new audio layer. Audible mobile
  balance, focus, and lifecycle behaviour remain unqualified without KVM or a physical Android
  device. No qualifying CNA or Sharp Runtime defect was found.

### 2026-10-05 — Water entry and exit accents

- Added an original deterministic 1.5-second mono PCM16 splash texture with explicit silent
  endpoints, conservative headroom, and no external asset. The complete current procedural PCM
  payload is 1,389,150 bytes (about 1.33 MiB) before backend bookkeeping.
- Added a renderer-independent shoreline transition detector with 7.5 cm entry and 2.5 cm exit
  thresholds, a 1.6-second cooldown longer than the sample, stationary-transition suppression,
  and bounded entry/exit pitch, gain, and pan variation. One transition cannot overlap another.
- Integrated a separately failure-isolated CNA one-shot, M mute behaviour, actionable diagnostics,
  and an F1 `SPLASHES` counter. Three persistent loops, one wave accent, and one transition accent
  are the current hard simultaneous-source ceiling.
- Added core coverage for exact format/duration, RMS/headroom, silent endpoints, hysteresis,
  cooldown, stationary suppression, re-entry, exit identity, and playback parameter bounds. The
  core suite and all seven isolated OpenGL 3.3 lifecycle/capture tests pass.
- Extended the complete Chromium/WebGL 2 input harness to reset through the existing F5 south-beach
  QA location and walk from `(0, -225)` to `(0, -251.332)`. The player crossed into the ocean, the
  actual transition `SoundEffect::Play` succeeded, and the full render/input/audio harness passed.
  Visual review confirms `SPLASHES` fits the 1280 by 720 debug panel without clipping.
- Rebuilt and linted the arm64-v8a APK successfully. Android audible playback, focus, and lifecycle
  behaviour remain unqualified without KVM or a physical device. The latest Linux build is open for
  operator play/listening; no qualifying CNA or Sharp Runtime defect was found.

### 2026-10-05 — First daytime-jungle ambience layer

- Added an original deterministic twelve-second mono PCM16 jungle bed. Circularly filtered forest
  texture and eight low-level, interior-only insect phrases avoid a short pure-tone loop; all events
  finish away from the seamless boundary. No external audio asset is used.
- Added an explicit `jungleVolume` zone target that is zero through the beach belt, reaches 0.078
  deep inland, falls to eight percent while swimming, and uses a slower 2.6-second fade than the
  surf/breeze crossfade. This keeps the transition gradual and leaves headroom for future wildlife.
- Integrated a fourth persistent CNA loop with global mute, startup/runtime failure handling, and
  F1 `JUNGLE` gain diagnostics. Four persistent loops plus one bounded wave and one bounded water
  transition are the current simultaneous-source ceiling; source PCM totals 1,918,350 bytes
  (about 1.83 MiB) before backend bookkeeping.
- Added core coverage for exact format/duration, deterministic regeneration, RMS/headroom,
  loop-boundary continuity, beach silence, inland presence, swimming attenuation, and slow fade
  convergence. The core suite and all seven isolated OpenGL 3.3 tests pass.
- The real Linux device transitioned all four loops to `Playing`, and the latest interactive build
  is open for F6 jungle listening. The complete Chrome/WebGL 2 render/input/audio harness passes
  and requires first-gesture initialization of the new layer; visual review confirms the wider
  audio diagnostic remains unclipped at 1280 by 720.
- Rebuilt and linted the arm64-v8a APK successfully. Android audible quality, focus, and lifecycle
  behaviour remain unqualified without KVM or a physical device. No qualifying CNA or Sharp
  Runtime defect was found.

### 2026-10-05 — Wave and shoreline accent variation

- Added a second original four-second stronger-wave wash with a different deterministic noise seed,
  filter widths, swell envelope, and crest shape. Added a second original 1.5-second shoreline
  texture with a softer attack, broader wash, and longer body. No external assets are used.
- Extended both schedulers with a deterministic two-way source selection before existing bounded
  pitch/pan/gain variation. Both cooldowns and the source ceiling are unchanged: variants never
  create overlapping copies of their own accent category.
- Prepared both effect pairs through CNA and kept failure isolation, global mute, diagnostic counts,
  and the existing browser-gesture boundary unchanged. Current source PCM totals 2,160,900 bytes
  (about 2.06 MiB); the hard simultaneous ceiling remains four loops plus two one-shots.
- Added core coverage for both variants' exact format/duration, RMS/headroom, silent endpoints,
  genuinely different PCM, scheduler bounds, and deterministic long-run reachability of both
  choices. The core suite and all seven isolated OpenGL 3.3 tests pass.
- The real Linux device initialized every effect and transitioned all four loops to `Playing`; a
  scheduled selected wave was accepted. The full Chrome/WebGL 2 harness passes, requires both pairs
  to be prepared after the first gesture, and successfully plays the selected shoreline variant.
- Rebuilt and linted the arm64-v8a APK successfully. Audible variant balance still requires the
  planned listening pass, and Android playback remains unqualified without KVM or a physical
  device. No qualifying CNA or Sharp Runtime defect was found.

### 2026-10-05 — Twenty-minute soundscape model regression

- Added a deterministic 72,000-frame simulation representing twenty minutes at 60 Hz. It cycles
  20-second beach, inland jungle, swimming, and wading-to-dry blocks without sleeping, opening a
  device, or depending on wall-clock timing.
- The regression continuously checks surf, breeze, jungle, water gain, and water pitch for finite
  values and authored limits. It also verifies bounded wave frequency, exactly one transition per
  entry/exit crossing, valid variant indices, and long-run reachability of both source choices.
- The complete core suite still finishes in about 1.2 seconds and passes. This is numerical
  stability/coverage evidence only; it deliberately does not close the separate subjective
  20-minute listening/fatigue task.

### 2026-10-05 — First procedural flying-bird layer

- Added a renderer-independent 24-agent flock split across ten shoreline, eight canopy, and six
  high-altitude route families. Deterministic spawn variation, soft radial steering, two
  incommensurate wander terms, speed/height variation, independent flap phases, and authoritative
  terrain clearance prevent synchronized short-loop traffic while keeping birds inside the island
  airspace.
- Added the first reusable wildlife primitive: an observer-distance activation policy with full,
  quarter-rate, and one-thirtieth-rate update tiers. Agents accumulate skipped time with a bounded
  catch-up step, so distant work drops without changing the renderer-facing state contract.
- Added original near/far low-poly bird geometry with CPU wing pose, individual colour/scale
  variation, per-agent frustum/distance rejection, and a single streamed CNA dynamic-vertex-buffer
  draw. The path uses portable `BasicEffect`, 16-bit-independent non-indexed triangles, and no
  geometry/compute shader or desktop-only feature.
- Added F1 bird visibility, render-LOD, simulation-tier, and draw counters plus a deterministic
  bird-detail camera. Core coverage simulates twenty minutes of flight and checks deterministic
  motion, population/path diversity, finite poses, terrain clearance, bounded radii, both wing
  extremes, and complete tier accounting.
- Passed the core suite and native OpenGL 3.3 lifecycle/capture tests, including a visually
  inspected dedicated close bird capture. Rebuilt WebGL 2 and passed the complete actual-Chromium
  harness with a new dedicated bird screenshot and nonzero 24-agent assertion; native and browser
  captures are compositionally consistent.
- Rebuilt and linted the arm64-v8a GLES 3 APK successfully with the bird simulation and renderer.
  Android bird runtime and performance remain unqualified because the host has no KVM and no
  physical device is connected. The latest Linux build is open on the real desktop for play and
  behaviour review. No qualifying CNA or Sharp Runtime defect was found.

### 2026-10-06 — First shallow-water fish schools

- Added eight deterministic six-member schools around the irregular shoreline. Route centres use
  `IslandTerrain::ShoreRadius`, search outward for sufficient depth, and move slowly in alternating
  directions; stable formation offsets, individual steering, speed, depth, and tail phases create
  cohesion without a rigid synchronized block.
- Every proposed movement is validated against authoritative terrain and signed shore distance.
  Fish remain on submerged terrain within the 3--48 m near-shore belt, at least 18 cm above the
  sampled seabed, and below the water surface. Full/reduced/dormant activation occurs per school
  through the shared wildlife policy rather than simulating all invisible fish at full rate.
- Added original near/far low-poly body, tail, dorsal, and side-fin geometry with CPU body/tail
  animation and four restrained tropical palettes. Individual frustum/distance culling gathers all
  visible agents into one CNA dynamic-buffer draw before the transparent ocean, letting the water
  shader tint and reveal them without a modern-only rendering feature.
- Added F1 fish visibility, render-LOD, simulation-tier, and draw counters plus a deterministic
  above-water shoreline composition camera. Core coverage simulates ten minutes and checks exact
  population/school distribution, deterministic movement, finite pose, continuous water/depth
  validity, meaningful shoreline travel, both tail extremes, and complete tier accounting.
- Passed native OpenGL 3.3 smoke and dedicated fish capture review. Rebuilt WebGL 2 and passed the
  complete actual-Chromium harness with a dedicated fish screenshot and nonzero 48-agent assertion;
  native and browser captures agree. Rebuilt and linted the arm64-v8a GLES 3 APK successfully.
  Android fish runtime/performance remain unqualified without KVM or a physical device. No
  qualifying CNA or Sharp Runtime defect was found.

### 2026-10-06 — Occasional non-hostile offshore shark

- Added one deterministic offshore shark patrol that exists purely as ambient wildlife. Visible
  42--66 second patrol windows alternate with longer 92--160 second deep-diving absences, making
  sightings occasional without adding attacks, danger, objectives, or player interaction.
- Authoritative shoreline and seabed queries constrain movement to a 34--88 m offshore belt with
  at least 3.5 m of water, seabed clearance, and surface clearance. Soft radial steering, varied
  angular speed, a slow depth cycle, and an independent body wave avoid a rigid circular loop.
- Added original near/far low-poly body, dorsal/pectoral fins, and animated tail geometry.
  Frustum/distance culling and the shared full/reduced/dormant observer policy bound render and
  simulation cost; visible geometry is streamed through one portable CNA `BasicEffect` draw before
  the transparent ocean.
- Added F1 presence/activity/LOD/simulation/cycle/draw diagnostics, an F10 offshore observation
  teleport, and deterministic native/browser shark-detail cameras. Core coverage simulates twenty
  minutes and checks repeatability, continuous water/depth validity, meaningful travel, body-wave
  extremes, appearance and absence windows, cycle count, and simulation-tier accounting.
- Passed the complete core suite and all ten native OpenGL 3.3 smoke/capture tests, with the shark
  silhouette and underwater composition visually inspected. Rebuilt WebGL 2 and passed the
  complete actual Chrome 152/SwiftShader harness with a dedicated shark screenshot and active
  patrol assertion; native and browser captures agree.
- Rebuilt and linted the arm64-v8a GLES 3 APK successfully. Android shark runtime/performance
  remain unqualified because the host exposes no KVM and no physical device is connected. No
  qualifying CNA or Sharp Runtime defect was found; both dependencies remain unmodified.

### 2026-10-06 — First GPU-skinned procedural monkey troop

- Audited CNA `next`'s current `SkinningData`, `AnimationPlayer`, `SkinnedEffect`, skinned vertex
  layout, runtime glTF path, and sibling viewer/street usage before choosing the implementation.
  The game uses only public CNA APIs and leaves CNA and Sharp Runtime unmodified.
- Added seven deterministic interior agents with compact `Idle`, `Observe`, `Wander`, `Walk`, and
  `RunAway` behaviour. Each agent has a bounded home region; terrain queries continuously enforce
  dry ground, a 48 m shore margin, and a 26-degree slope ceiling. A player inside 7.5 m causes a
  brief non-hostile retreat rather than an attack or objective.
- Generated an original low-poly monkey, fourteen-bone topologically ordered rig, bind/inverse-bind
  poses, and idle/observe/walk/run clips entirely at runtime. Near LOD adds light muzzle/inner-ear
  and eye passes for facial readability; far LOD remains one draw. CNA `AnimationPlayer` produces
  each palette and portable `SkinnedEffect` performs real GPU skinning.
- Added full/reduced/dormant decision tiers, render distance/frustum culling, an F11 troop teleport,
  overlay visibility/LOD/simulation/state/draw counters, and deterministic native/browser monkey
  cameras. Core coverage simulates twenty minutes and checks deterministic motion, all behavioural
  categories, all activation tiers, continuous terrain validity, finite poses, and meaningful
  travel.
- A first native capture exposed an application-side material mistake: a null texture made the
  otherwise correct skinned silhouette black. A project-owned neutral 1 by 1 texture fixed it;
  repeated native and browser captures show the intended lit brown material and face detail. This
  was incorrect game setup, not a qualifying framework defect, so `BUGS.md` remains unchanged.
- Passed the complete core and native OpenGL 3.3 suites. Rebuilt WebGL 2 and passed the full actual
  Chrome 152/SwiftShader harness, including a dedicated monkey capture with all four rendered
  animation states represented in the live troop. Rebuilt and linted the arm64-v8a GLES 3 APK.
  Android monkey runtime remains unqualified without KVM or a physical device.
- Added a second navigation pass that installs the same palm, broadleaf, solid-rock, and
  fallen-log collision proxies used by the player. Spawn, target, and every movement proposal now
  preserve a 0.32 m body clearance; blocked agents try deterministic alternating lateral headings
  before selecting a new state. The debug overlay exposes cumulative avoidance deflections.
- Re-ran the twenty-minute core simulation against the complete deterministic world proxy set. It
  verifies continuous sampled clearance, deterministic deflection counts, valid terrain, all
  states and activation tiers, and meaningful travel. Remaining monkey work is explicit: greater
  silhouette/material variation, sparse positional calls, long subjective behaviour tuning, and
  stable Android runtime validation.
- Re-ran all eleven native OpenGL 3.3 smoke/capture scenes and visually reviewed the relocated
  obstacle-aware monkey composition. Rebuilt WebGL 2 and passed the complete actual-Chromium
  scene/input/audio-init harness with the same placement. Rebuilt and linted the arm64-v8a GLES 3
  APK; device runtime remains explicitly unqualified.
- Added game-side 0.14--0.26 second skeletal crossfades. The renderer snapshots the actually
  presented local pose, samples CNA's new clip pose, blends scale/translation and slerps rotation,
  then recomposes the hierarchy and inverse-bind palette. A continuous per-agent animation clock
  prevents the `Wander` to `Walk` alias from restarting the same locomotion clip. The dedicated
  native monkey capture now waits 24 frames so visual review sees the settled pose; the overlay
  reports active/completed crossfades and WebGL validation requires the startup blend path.
- The crossfade pass retains the complete twenty-minute core regression, passes all eleven native
  OpenGL 3.3 scenes with the monkey test requiring a completed startup blend, and passes the full
  actual-Chromium WebGL 2 harness with an active startup-blend assertion. The arm64-v8a GLES 3 APK
  also rebuilds and passes app lint; only Android runtime remains unqualified.
- Replaced the single cloned body mesh with three prebuilt procedural silhouettes: balanced,
  long-limbed/slender, and compact/stocky. Each has matched near/far topology and shares the same
  fourteen-bone palette, so choosing a variant adds no draw call or per-frame mesh generation.
  Four coordinated body/face palettes and existing individual scale variation further break up
  repetition. Repeated F11 teleports cycle through all seven agents for direct comparison.
- Extended the core regression to require all four material variants and meaningful scale spread.
  All eleven native OpenGL 3.3 scenes, the complete actual-Chromium WebGL 2 harness with explicit
  silhouette/palette initialization checks, and the arm64-v8a GLES 3 build plus app lint pass.
  Android runtime remains unqualified.

### 2026-10-06 — Bounded positional wildlife sound

- Audited CNA `next`'s current `AudioListener`, `AudioEmitter`, `SoundEffectInstance::Apply3D`,
  distance-scale, lifetime, pan, attenuation, and Doppler behaviour without editing CNA or Sharp
  Runtime. World units remain meters and the game configures a 24 m full-gain reference radius.
- Added four original deterministic PCM16 mono phrases: two 1.2--1.35 second bird patterns and two
  1.8--2.1 second restrained monkey contact patterns. Oscillator sweeps, harmonic shaping,
  envelopes, tremolo, and filtered breath texture are generated at runtime; no external audio
  asset or license dependency was introduced.
- Added one deterministic cross-species scheduler. It selects the nearest non-dormant real agent,
  gates birds beyond 110 m and monkeys beyond 78 m, suppresses monkeys while swimming or near the
  shore, varies source/pitch/gain conservatively, and spaces bird calls 13--30 seconds and monkey
  calls 27--50 seconds apart.
- Kept the hard audio ceiling at four loops plus two transients. The one explicitly owned wildlife
  voice starts only when both transient slots are idle; wave playback can occupy the second slot,
  and an urgent shoreline transition pre-empts wildlife if required. Muting stops the voice and a
  wildlife-only failure degrades without taking down the ambient beds. Wave and shoreline accents
  are now owned instances too, so M stops all active transients immediately rather than merely
  suppressing later fire-and-forget requests.
- Added F1 bird/monkey call counters and pooled-voice state. Core tests cover format, duration,
  headroom, meaningful energy, silent endpoints, source distinction, busy-slot holding, swimming
  habitat gating, bounded parameters, both variants, and sparse twenty-minute event counts.
- Passed the complete core suite and all eleven OpenGL 3.3 smoke/capture scenes. A real Linux audio
  device started all four loops and successfully played the first positional bird call; the latest
  debug-overlay build remains open for operator play and listening.
- Rebuilt WebGL 2 and passed the complete Chrome 152/SwiftShader renderer/input/audio harness, now
  requiring wildlife-resource preparation on the first user gesture. Rebuilt and linted the
  arm64-v8a GLES 3 APK. Audible Web hardware-browser quality and Android runtime/audio focus remain
  explicitly unqualified; no dependency defect was found.
- Refreshed the locked read-only CNA `next` export from `b0e97bb1bb87` to the workspace's current
  `f243c63a1dc5` after reviewing the intervening platform/graphics changes. Reconfigured and rebuilt
  native, Web, and Android from that export; 12/12 native tests, the full actual-Chrome WebGL 2
  harness, arm64 APK assembly, app lint, and real Linux audio startup/positional playback all pass.
  Sharp Runtime remains current at `21c06910f7cf`. The superseded 327 MiB generated export was
  removed after qualification; both dependency worktrees remain clean and unmodified.

### 2026-10-06 — Portable world-space diagnostics

- Added an opt-in F2 world view, visible only with the existing F1 developer presentation, plus a
  `--world-debug` deterministic capture path. Cyan, yellow, orange, and magenta-red contours show
  the authoritative irregular shoreline and the exact 42/72/92 m ocean-current thresholds; an
  active yellow arrow shows the player's actual inward-current direction.
- Added a white player footprint, orange circles/crosses/posts for nearby palm, broadleaf, rock,
  and fallen-log collision proxies, and green edges for nearby shared 8 by 8 terrain/vegetation
  cells. These local diagnostics use an intentionally x-ray line pass so grass, terrain, and dense
  foliage cannot hide collision state, while the long-distance shoreline contours remain
  depth-aware.
- Added heading arrows for nearby birds, fish, the shark, and monkeys. Full-tier markers retain
  species/state colours, reduced simulation uses yellow, and dormant simulation uses grey; monkey
  full-tier colours distinguish idle, observe, ordinary locomotion, and run-away behaviour. The
  F1 panel reports visible obstacles, cells, wildlife markers, total line count, and the two debug
  draw submissions.
- Kept the feature entirely inside island-simulator and on CNA public `BasicEffect` plus
  `DrawUserPrimitives` APIs. The ordinary scene creates no debug vertices or draw submissions when
  F1/F2 is inactive. No CNA or Sharp Runtime file was modified and no dependency defect was found.
- Added `IslandGameWorldDebugCapture`, visually reviewed both overview and first-person beach
  captures, and passed all 13 native tests including 12 real OpenGL 3.3 scenes. Rebuilt WebGL 2;
  Chrome 152/SwiftShader passed the full renderer/input/audio harness with a synthesized F2 toggle
  and successful world-line rendering. Rebuilt the arm64-v8a GLES 3 APK and passed app lint; F2
  Android runtime remains unqualified without KVM or a physical device.

### 2026-10-06 — Portable terrain material variation

- Extended every terrain vertex with the authoritative signed shoreline distance and slope while
  retaining the existing prelit biome colour. Added explicit GLSL 330 and GLSL ES 300 terrain
  stages through CNA's public `ShaderEffect` package API; `BasicEffect` remains the logged graceful
  fallback if custom effects are unavailable or shader compilation fails.
- Added texture-free near-field sand, wet-edge, interior-soil, and steep-ground variation. Two
  scales of smooth value noise avoid conspicuous repeated stripes, a restrained fine component
  breaks up close surfaces, and distance fading removes high-frequency work and aliasing beyond
  the useful first-person range. No texture memory, remote asset, or new licensed content was
  introduced.
- Visually reviewed native first-person beach and driftwood captures, including the wet shoreline,
  broad bare sand, grass transition, and OpenGL fallback diagnostics. The native smoke test now
  requires the full material path; all 13 native tests pass.
- The complete Chrome 152/SwiftShader WebGL 2 harness passes with an explicit terrain-shader gate,
  and its driftwood capture matches the OpenGL 3.3 material composition. The arm64-v8a GLES 3 APK
  rebuilds with the shader; physical-device visual/performance qualification remains open. No CNA
  or Sharp Runtime source was modified and no dependency defect was found.

### 2026-10-06 — Static collision broad phase

- Added an immutable game-owned 16 m spatial grid over all 794 palm, broadleaf, solid-rock, and
  sampled fallen-log collision proxies. Player substeps query nearby cell populations before the
  unchanged exact circle test, retaining slope rejection and axis sliding without scanning every
  obstacle.
- Kept the exhaustive span-based `PlayerMotion` path as a regression oracle. Core coverage compares
  indexed and exhaustive overlap results across 1,681 island-wide queries and confirms identical
  stop/slide results against a focused trunk case. It also requires a material average reduction in
  broad-phase candidates.
- Added startup grid diagnostics and an F1 local-candidate counter. The deterministic beach view
  considers 14 of 794 proxies while preserving the same world result; the 794 proxies occupy 467
  non-empty cells. All 13 native tests pass (12 create the real OpenGL renderer), the complete
  Chrome 152/WebGL 2 harness passes, and the arm64-v8a GLES 3 APK rebuilds with clean app lint.

### 2026-10-06 — Near-field shell and pebble clusters

- Added 265 deterministic non-solid substrate clusters to the 3--18 m dry-sand belt. Placement is
  low-slope and height constrained, keeps authored routes/spawn clear, uses multi-scale clustering,
  and varies yaw, scale, and four original game-generated mesh compositions.
- Each tiny opaque mesh combines four differently proportioned faceted shells/pebbles with muted
  coral, limestone, grey, and sun-bleached palettes plus close contact geometry. The layer reuses
  the portable natural-prop shader and hardware-instancing path, culls individual instances beyond
  52 m, and costs no more than four extra draw submissions when all variants are nearby. No asset,
  texture memory, transparency overdraw, collision proxy, or license attribution was added.
- Extended F1/startup/first-frame counts and the deterministic layout regression with population,
  exact repeatability, dry-sand belt, support, route-clearance, scale, and full-variant checks. The
  native driftwood capture now requires visible substrate clusters. Native and Chrome/WebGL 2
  driftwood captures were visually compared and match; all 13 native tests and the full browser
  harness pass. The arm64-v8a GLES 3 APK rebuilds and app lint remains clean; device runtime is
  still unqualified without KVM or physical hardware.

### 2026-10-06 — Repeatable native frame-time evidence

- Added a game-owned `--benchmark <report.json>` mode that selects a fixed QA camera, disables
  VSync and the fixed timestep, discards 120 warm-up intervals, records 600 end-to-end intervals,
  and exits with a machine-readable mean/median/p95/p99/min/max/FPS report. Debug, capture, and
  smoke combinations are rejected so measurements retain one defined presentation path.
- Added tested renderer-independent statistics with interpolated percentiles and explicit invalid-
  input diagnostics. JSON also records resolution, renderer, sampling configuration, tracked scene
  submissions, and representative visible populations. `IslandGameBenchmark` protects the complete
  native report path without treating a hardware-specific duration as a pass threshold.
- Added a five-view private-GPU suite for overview, beach, jungle, shore detail, and monkeys plus
  `docs/performance.md` methodology and limitations. On the 1280 by 720 debug build using the
  accelerated Radeon 780M/Mesa 25.0.7 environment, beach was the heaviest fixed scene at 1.6497 ms
  mean and 1.8729 ms p95 across 600 samples and 118 tracked submissions after the shoreline-ribbon
  pass. Audio was unavailable in the isolated compositor, so this is a comparison baseline rather
  than a shipping budget.
- All 14 native tests pass, including 13 real OpenGL 3.3 GPU scenes. The full actual-Chrome WebGL 2
  renderer/input/audio-init harness passes after rebuilding, and the arm64-v8a GLES 3 APK rebuilds
  with clean app lint. Web hardware performance and Android device performance remain unqualified;
  no CNA or Sharp Runtime source was modified and no dependency defect was found.

### 2026-10-06 — Exact shoreline foam and wash ribbon

- Added a game-owned 384-segment by six-strip ribbon derived directly from the authoritative
  `ShoreRadius` contour. Its 2.5 m landward-to-5.5 m offshore span avoids depending on the ocean
  grid's 10.625 m spacing, while terrain depth clips foam that should remain below raised sand.
- Added explicit GLSL 330 / GLSL ES 300 stages that reuse the ocean's three surface waves, animate
  a narrow phase-varied broken crest, add a restrained receding wash, distance-fade the pass, and
  output correct premultiplied alpha. A static premultiplied vertex-colour `BasicEffect` fallback
  remains available with an actionable diagnostic.
- The layer costs 2,695 vertices, 4,608 triangles, one transparent draw submission, no texture,
  no collision, and no external asset. Core tests lock the portable 16-bit topology and radial
  extents. Native and actual Chromium/WebGL 2 driftwood captures were visually compared and show
  the same narrow shore edge without a visible ring seam.
- Repeated the five-view accelerated native benchmark after integration: the heaviest beach view
  remains 1.6497 ms mean / 1.8729 ms p95 in this debug-build comparison and now tracks 118 scene
  submissions. All 14 native tests pass, the complete Chrome/WebGL 2 renderer/input/audio-init
  harness passes with an explicit shoreline-shader gate, and the arm64-v8a GLES 3 APK rebuilds with
  clean app lint. Android runtime appearance remains unqualified without suitable hardware; no CNA
  or Sharp Runtime source was modified and no dependency defect was found.

### 2026-10-06 — Restrained locomotion camera motion

- Added a renderer-independent first-person camera-motion model driven by actual horizontal travel
  rather than requested velocity. Full-speed walking stays within 7 mm lateral / 12 mm vertical;
  swimming smoothly blends to a slower rhythm bounded at 10 mm / 18 mm. Using physical travel
  prevents false head movement while input is blocked by a trunk, rock, log, or steep slope.
- Kept the displacement presentational: physics, wildlife simulation, and the positional-audio
  listener retain the authoritative eye point, while every camera-dependent renderer receives one
  matching displaced position. Fixed capture/benchmark viewpoints remain motionless, QA teleports
  clear residual phase, long frames clamp at 100 ms, and B smoothly disables/restores the effect.
- Added deterministic core coverage for walking and swimming bounds, style blending, opt-out
  convergence, reset, and long-frame equivalence. All 14 native tests pass, including 13 private-
  display OpenGL scenes. The actual Chrome 152/WebGL 2 harness passes and now verifies both B
  transitions. The final arm64-v8a GLES 3 APK rebuilds with `libmain.so` and passes app lint;
  Android touch feel remains unqualified without suitable hardware. CNA and Sharp Runtime remain
  clean and unmodified, and no dependency defect was found.

### 2026-10-06 — Grove-associated fallen palm fronds

- Added 63 deterministic non-solid fallen fronds derived from suitable palm groves rather than a
  uniform world scatter. Placement samples each complete 2.45--3.75 m footprint, rejects steep or
  uneven support, water, distant interior, routes, nearby trunks, and centres closer than 2.9 m.
  Core coverage locks population, exact repeatability, scale/variant bounds, all support samples,
  route clearance, support height, spacing, and exclusion from the solid collision population.
- Added three opaque generated meshes with a curved rachis, 28 tapered leaflets, fresh/olive/dry
  palettes, shared portable natural-prop shading, variant-level hardware instancing, cell/frustum
  culling, and an individual 76 m cutoff. The complete layer needs at most three extra submissions,
  no texture memory, transparency overdraw, collision proxy, or external asset. F1 and benchmark
  JSON now expose visible/total fronds.
- Added a deterministic `fronds` detail camera and native/Web capture gates. The first eight-pair
  silhouette was rejected during visual review as too skeletal; the denser second mesh was inspected
  natively and in actual Chrome 152/WebGL 2 and matches across renderers. All 15 native tests pass,
  the complete browser renderer/input/audio-init harness passes, and the arm64-v8a GLES 3 APK
  rebuilds with clean app lint. Two five-view accelerated debug benchmark repeats keep the heaviest
  beach scene at 1.7127--1.7321 ms mean / 1.8899--1.9729 ms p95 with four visible fronds and four
  additional tracked submissions. Physical Android appearance/performance remains unqualified; CNA
  and Sharp Runtime remain clean and unmodified.

### 2026-10-06 — Emergent reef outcrops and swimming collision

- Added 14 deterministic solid outcrops around the authoritative 1.8--9.4 m offshore shelf. A
  jittered 56-site angular layout avoids grid stamping, scales every rock from its actual seabed
  support so at least 0.39 m remains visibly emergent, favours the grey/olive coastal palettes, and
  reuses the existing four instanced rock batches. Core coverage locks repeatability, placement
  belt, submerged support, emergence, scale bounds, solid-proxy accounting, and population budget.
- Unified horizontal obstacle resolution across walking and swimming. Swimmers retain reduced
  acceleration and omit terrain slope rejection, but now use the same substepped exact/grid-backed
  proxy tests and axis sliding, preventing passage through the new outcrops. Exhaustive and indexed
  swimming paths have a deterministic equivalence regression.
- Rejected the initial pointed five-ring capture as too diamond-like. The inspected revision gives
  near rocks seven irregular shaped rings, twelve sides, a broad embedded base, a small asymmetric
  cap, brighter faceted colour variation, and a reduced four-ring/seven-side far mesh. Native and
  actual Chrome 152/WebGL 2 reef captures match; the F1 prop row exposes the visible/total subset.
- All 16 native tests pass, including the new dedicated reef capture. The complete Chrome WebGL 2
  renderer/input/audio-init harness passes with its new reef scene, and the arm64-v8a GLES 3 APK
  rebuilds with clean application lint. Two five-view accelerated benchmark repeats retain the
  existing 80/122/89/24/95 tracked submissions; the heaviest beach view measures
  1.6988--1.7015 ms mean and 1.8836--1.9754 ms p95 with five visible outcrops. Physical Android
  collision feel and appearance remain unqualified without KVM or a connected device. CNA and
  Sharp Runtime remain clean and unmodified.

### 2026-10-06 — Near-surface swimming view

- Added a renderer-independent water-view model that maps swimming state and authoritative support
  depth to a restrained 0.60--1.00 target, fades in at 2.4/s and out at 1.8/s, and clamps long or
  non-finite frame durations. Core coverage verifies dry suppression, shallow/deep ordering,
  smoothing, deterministic reset, long-frame handling, and complete exit.
- Added a runtime-generated 160 by 90 premultiplied turquoise edge veil, stretched through one
  conditional CNA `SpriteBatch` draw after the 3D/shore passes and before touch/debug UI. It uses
  no external asset and submits nothing while dry. F1 and capture diagnostics expose intensity and
  draw count; `--viewpoint water` supplies a deterministic near-surface QA composition.
- Rejected the first native capture because straight-alpha colour data was incorrect for CNA's
  premultiplied blend path and visibly overexposed the whole frame. After premultiplying the
  generated pixels and tint, native and actual Chrome 152/WebGL 2 captures match and retain the
  intended subtle edge treatment.
- All 17 native tests pass, including 16 isolated OpenGL 3.3 scenes. The complete WebGL 2
  lifecycle/render/input/audio-init harness passes with the dedicated water scene. The arm64-v8a
  GLES 3 APK builds and app lint passes; the stripped `libmain.so` is 15,635,824 bytes and the APK
  is 33,368,200 bytes. Android runtime appearance remains unqualified without KVM or a connected
  device. Two accelerated water-view benchmark repeats measured 1.5405--1.5895 ms mean,
  1.6881--1.8650 ms p95, and 110 tracked submissions. CNA and Sharp Runtime remain clean and
  unmodified; no dependency defect was found.

### 2026-10-06 — Natural offshore-current visual feedback

- Refined the water-view target into a calm 0.60--0.78 depth cue plus up to 0.22 from the existing
  normalized 42--72 m offshore-current field. The single premultiplied overlay draw also removes
  up to 12% red and 3% green from its tint as the current develops. This makes the periphery
  quietly cooler and stronger while preserving the island view and avoiding text, alarm colours,
  a geometric wall, or another render submission.
- Added deterministic tests for full-current normalization and non-finite input fallback, plus a
  fixed `--viewpoint current` camera 62 m off the south shore. Native and actual Chrome 152/WebGL 2
  captures match; the browser overlay records intensity 0.963, normalized current 0.741, and one
  draw. All 18 native tests pass, including 17 isolated OpenGL 3.3 scenes, and the expanded browser
  lifecycle/render/input/audio-init harness passes.
- The arm64-v8a GLES 3 APK rebuilds and app lint passes; stripped `libmain.so` is 15,636,704 bytes
  and the APK is 33,369,080 bytes. Runtime appearance remains unqualified without KVM or a physical
  device. Two accelerated current-view benchmark repeats tracked 91 submissions and measured
  1.4501--1.4756 ms mean, 1.6750--1.8775 ms p95, and 1.8992--2.2975 ms p99. CNA and Sharp Runtime
  remain clean and unmodified; no dependency defect was found.

### 2026-10-06 — Portable distant tropical horizon scenery

- Added five deterministic low islets between 570 m and 665 m from the island centre, safely
  beyond the player's 92 m offshore boundary and inside the 850 m ocean extent. Core coverage
  locks exact repeatability, shore-safety margin, ocean containment, dimensions, variants, and
  inter-islet separation.
- Added one static 2,361-vertex/787-triangle `BasicEffect` batch combining asymmetric low rock
  ridges with 11 simplified palm silhouettes. Restrained 260--790 m fog merges the muted palette
  into the horizon, while the later transparent ocean pass covers the low bases. The layer uses
  no texture, external asset, custom shader, instancing dependency, or modern-only GPU feature.
- Rejected the first overly smooth, undersized silhouette and iterated ridge asymmetry, scale,
  frond width, and filled crown hubs until the two-islet south-beach composition remained readable
  without competing with the explorable island. Native OpenGL 3.3 and actual Chrome 152/WebGL 2
  captures visually match. A dedicated `--viewpoint distant` camera and native/browser gates
  require all five islets and exactly one draw.
- All 19 native tests pass, including 18 real OpenGL 3.3 scenes. The full browser renderer,
  lifecycle, input, and audio-initialization harness passes. The arm64-v8a GLES 3 APK rebuilds and
  app lint passes; stripped `libmain.so` is 15,650,040 bytes and the APK is 33,382,416 bytes.
  Android appearance remains unqualified without KVM or a physical device.
- Two five-view accelerated benchmark repeats show the scenery adds exactly one tracked submission
  to every scene: 81/123/90/25/96. The heaviest beach view measures 1.6756--1.6996 ms mean and
  1.8311--1.8803 ms p95. CNA and Sharp Runtime remain clean and unmodified; no dependency defect
  was found.

### 2026-10-06 — Current-responsive undertow audio cue

- Extended the existing six-second player-water loop mix with the authoritative normalized
  42--72 m offshore-current strength. Calm stationary water remains silent; while swimming, a
  smoothstep-controlled deeper undertow bed grows to at most 0.08 gain and lowers pitch by at most
  0.08. Separate 650 ms cue smoothing prevents a trigger-like boundary without changing physics.
- Kept the hard audio budget unchanged at four persistent loops plus two transients. The cue
  reuses the existing procedural player-water PCM, allocates no fifth source, adds no asset or
  sample memory, and remains exactly zero outside swimming or for non-finite input. F1 exposes the
  smoothed `UNDERTOW` value.
- Added an F12 overlay-only teleport to the south 62 m current field for direct feedback QA. The
  deterministic current capture now requires a nonzero renderer-independent undertow target while
  correctly leaving the audio device disabled in capture mode. Core coverage locks full-current,
  dry, invalid-input, fade-in/release, zero residual gain, convergence, and twenty-minute
  bounded-ramp behaviour.
- All 19 native tests pass, including 18 real OpenGL 3.3 scenes. The complete Chrome 152/WebGL 2
  renderer/lifecycle/input/audio-init harness passes; its current camera reports normalized current
  0.741 and undertow target 0.851. A normal (non-capture) Linux run at the same fixed camera opened
  the real 44.1 kHz mixer, started all four persistent loops, and reported the same current/target.
  The arm64-v8a GLES 3 APK rebuilds and app lint passes; stripped
  `libmain.so` is 15,651,008 bytes and the APK is 33,383,384 bytes. Android playback/focus and
  subjective cross-platform listening remain unqualified without a physical device or suitable
  accelerated emulator. CNA and Sharp Runtime remain clean and unmodified; no dependency defect
  was found.

### 2026-10-06 — Portable terrain contact shadows

- Added one texture-free, terrain-conforming `BasicEffect` batch for nearby palms, broadleaf
  canopy trees, land rocks, fallen logs, driftwood, and monkeys. Twelve-segment radial-alpha
  ellipses follow authoritative terrain height, use the shared fixed sun direction for the taller
  silhouettes, fade over the outer 22 percent of class-specific 78--105 m ranges, and draw before
  their opaque owners with depth reads but no depth writes. Offshore reef rocks and small foliage
  are intentionally excluded.
- Rejected the first working implementation after measurement: per-frame procedural height
  sampling raised the beach mean to 2.9014 ms. Static geometry is now sampled once at load, while
  culling and buffer uploads refresh after 0.25 m of camera movement, a small view/projection
  change, or 0.08 m of monkey movement. The retained two-repeat beach mean is 1.7500--1.7589 ms;
  the five views submit 0/29/28/3/26 shadows and 0/348/336/36/312 triangles. A nonempty view costs
  exactly one tracked draw; the distant overview remains at zero shadow work.
- Extended the F1 overlay with total and palm/canopy/rock/wood/monkey shadow counts plus draws, and
  extended benchmark JSON with visible contact shadows and triangles. The native prop capture and
  actual-Chromium prop camera both require a nonzero one-draw batch. Native and Web captures were
  visually inspected for grounding, dark decals, and terrain fighting.
- All 19 native tests pass, including 18 real OpenGL 3.3 scenes. The complete Chrome 152/WebGL 2
  renderer/lifecycle/input/audio-init harness passes after the final optimized rebuild. The final
  arm64-v8a GLES 3 APK rebuild and app lint pass; stripped `libmain.so` is 15,682,120 bytes and the
  APK is 33,414,496 bytes. Android runtime appearance remains unqualified without KVM or a physical
  device. CNA and Sharp Runtime remain clean on `next` and unmodified; no dependency defect was
  found.

### 2026-10-06 — Movement-driven player water ripples

- Added a renderer-independent `WaterRippleModel` that converts authoritative ground depth,
  swimming state, and horizontal speed into a bounded eased intensity plus wrapped radial phase.
  Dry and stationary water states remain exactly zero; long frames, invalid inputs, wading,
  swimming, sustained phase wrapping, deterministic reset, and complete release have core tests.
- Integrated two thin expanding, spatially varied bands into the existing ocean fragment shader.
  They share its displaced surface, shallow-water weighting, foam colour, GLSL 330/ES 300 package,
  and single draw submission. A uniform branch skips ripple math while inactive; no geometry,
  texture, external asset, or additional draw was added. Visual review rejected an initially broad,
  bright ring and retained a much thinner 0.24-maximum foam contribution.
- Extended F1 diagnostics and benchmark JSON with ripple intensity/phase. The native and browser
  water gates require a nonzero deterministic ripple, while the offshore-current camera proves the
  zero-ripple stationary case remains valid. Two accelerated active-water repeats retained 112
  tracked submissions and measured 1.6218--1.7052 ms mean and 2.0327--2.4040 ms p95.
- All 19 native tests pass, including 18 real OpenGL 3.3 scenes. The complete Chrome
  152/WebGL 2 renderer/lifecycle/input/audio-init harness passes and visually rendered the new
  shader path. The arm64-v8a GLES 3 APK rebuild and app lint pass; WebAssembly is 8,293,998 bytes,
  stripped `libmain.so` is 15,687,656 bytes, and the APK is 33,420,032 bytes. Android runtime
  appearance remains unqualified without KVM or a physical device. CNA and Sharp Runtime remain
  clean on `next` and unmodified; no dependency defect was found.

### 2026-10-06 — Speed-responsive player grass interaction

- Added a renderer-independent `GrassInteractionModel` that maps authoritative horizontal player
  speed into a finite, clamped, smoothly entering and releasing 0--1 envelope. Core coverage checks
  stationary, negative, non-finite, normal-walk, long-frame, deterministic-reset, and release
  behaviour.
- Integrated local displacement into the existing instanced grass vertex shader. Blade tips bend
  radially away and lower slightly within a radius that grows from 0.55 m while stationary to
  1.35 m at walking speed. It adds no texture, geometry, draw call, CPU patch search, or persistent
  trail; the stock-effect fallback deliberately omits the cosmetic bend.
- Visual review rejected a broad static clearing and retained the small speed-responsive body
  envelope. A deterministic obstacle- and foliage-cleared `--viewpoint grass` camera now exercises
  full strength in native and browser captures. F1 and benchmark JSON expose the envelope, and the
  automated gates require both the custom shader and full QA interaction.
- Two five-view native repeats retained 81/124/91/26/97 submissions and measured
  1.2165--1.2169 / 1.7383--1.7462 / 1.3831--1.4192 / 0.8063--0.8162 /
  1.5090--1.5768 ms mean. Two full-interaction grass-view repeats rendered 2,827 clusters in 126
  submissions at 1.8014--1.8117 ms mean and 2.2166--2.3616 ms p95.
- All 20 native tests pass, including 19 real OpenGL 3.3 scenes. The complete Chrome
  152/WebGL 2 renderer/lifecycle/input/audio-init harness passes and visually matches the native
  grass composition. The arm64-v8a GLES 3 APK rebuild and app lint pass; WebAssembly is 8,297,201
  bytes, stripped `libmain.so` is 15,694,160 bytes, and the APK is 33,426,536 bytes. Android
  runtime appearance remains unqualified without KVM or a physical device. CNA and Sharp Runtime
  remain clean on `next` and unmodified; no dependency defect was found.

### 2026-10-06 — Curved and daylight-shaped grass blades

- A close native critique identified flat, uniformly coloured grass as the weakest part of the
  otherwise dense ground layer. The retained near cluster now uses five curved control levels and
  224 triangles instead of four levels and 168 triangles; the far cluster deliberately drops to
  three levels and 20 triangles from 30 because the intermediate curve was not useful at distance.
- Packed deterministic blade orientation into the existing vertex-colour alpha channel rather
  than expanding the vertex or instance stream. The portable GLSL 330 / GLSL ES 300 path now adds
  shared fixed-sun two-sided diffuse, subtle folded-side colour, root darkening, view-edge light,
  and restrained back-transmission. It adds no texture, instance data, CPU update, or draw call.
  Visual review rejected the first overly dark tuning and retained a brighter bounded response.
- Added an explicit native/browser startup gate for the five-level two-sided path. Native and
  actual Chrome/SwiftShader grass captures were visually compared after final tuning and match;
  the old `BasicEffect` fallback remains functional but intentionally omits detailed fragment
  lighting.
- Two complete native five-view repeats preserved 81/124/91/26/97 submissions. The beach view
  measured 1.7259--1.7574 ms mean and 2.1388--2.1531 ms p95; the full-interaction close-grass view
  measured 1.7429--1.7595 ms mean and 1.8919--1.9007 ms p95 across two repeats. The results stay
  inside ordinary debug-build variation; no isolated performance improvement is claimed.
- All 20 native tests pass, including 19 real OpenGL 3.3 scenes. The complete Chrome
  152/WebGL 2 renderer/lifecycle/input/audio-init harness passes with the new shading gate. The
  arm64-v8a GLES 3 APK rebuild and app lint pass; WebAssembly is 8,299,390 bytes, stripped
  `libmain.so` is 15,698,448 bytes, and the APK is 33,430,824 bytes. Android runtime appearance and
  mobile fragment cost remain unqualified without KVM or a physical device. CNA and Sharp Runtime
  remain clean on `next` and unmodified; no dependency defect was found.

### 2026-10-06 — Near-field jungle substrate and leaf litter

- Extended the existing texture-free terrain material with one close microvariation octave and two
  deterministic leaf-litter grids, bounded to low-slope interior ground and a 24--68 m camera fade.
  One hash per litter cell derives placement, scale, bend, orientation, and presence; the feature
  adds no asset, texture, geometry, CPU placement state, terrain-query change, or draw submission.
- Visual review rejected the first large, frequent oval marks. The retained pass uses smaller,
  sparser, pointed and slightly curved leaves with reduced contrast and clustered density. Native
  close-grass, jungle, and fallen-log captures were inspected; the beach remains intentionally
  unaffected and detail disappears before mid-distance aliasing becomes prominent.
- Added an explicit native smoke and WebGL 2 lifecycle/grass/jungle gate for the new material path.
  Native and actual Chrome/SwiftShader captures visually agree.
- Two complete five-view native repeats preserved 81/124/91/26/97 submissions. Mean frame time was
  1.2074--1.2233 ms overview, 1.7705--1.7916 ms beach, 1.4494--1.4675 ms jungle,
  0.8202--0.8285 ms driftwood, and 1.5414--1.6410 ms monkeys. Two dedicated close-grass repeats
  measured 1.8411--1.8561 ms mean and 2.0394--2.2592 ms p95. The retained close detail costs about
  0.04--0.11 ms mean in affected integrated views on this debug baseline.
- All 20 native tests pass, including 19 real OpenGL 3.3 scenes. The complete Chrome
  152/WebGL 2 renderer/lifecycle/input/audio-init harness passes with the new terrain-detail gate.
  The arm64-v8a GLES 3 APK rebuild and app lint pass; WebAssembly is 8,301,584 bytes, stripped
  `libmain.so` is 15,700,608 bytes, and the APK is 33,432,984 bytes. Android runtime appearance and
  fragment cost remain unqualified without KVM or a physical device. CNA and Sharp Runtime remain
  clean on `next` and unmodified; no dependency defect was found.

### 2026-10-06 — Layered high-wisp sky polish

- Replaced the sky's direction-linear cloud waves, which projected as conspicuous vertical streaks
  in first-person captures, with elevation-dominant long and fine bands, one directional warp, and
  one azimuth breakup sample. The retained four-sample field produces sparse horizontal high wisps,
  stays static for the permanent sunny-day art direction, and adds no texture, state, or draw.
- Visual review rejected an initial version that was too sparse and a six-sample follow-up that was
  unnecessarily expensive for a full-screen mobile fragment path. Overview, beach, jungle, and
  distant-horizon native captures were inspected after the retained density/cost tuning; the former
  vertical streaks are absent. An actual Chrome/WebGL 2 distant capture was also inspected.
- Changed the native smoke and every relevant browser scene gate to require the new explicit sky
  diagnostic. All 20 native tests pass, including 19 real OpenGL 3.3 scenes. After one transient
  aggregate harness failure, a clean rerun reported all 18 Chrome 152/WebGL 2 lifecycle, deployment,
  scene, input, and audio-init stages passing.
- Two complete native five-view repeats preserved 81/124/91/26/97 submissions. Mean frame time was
  1.2251--1.2462 ms overview, 1.8222--1.8359 ms beach, 1.4726--1.6432 ms jungle,
  0.8299--0.8814 ms driftwood, and 1.6486--1.8296 ms monkeys. The broad upper-end variation across
  the second run prevents attributing an isolated cost; the low ends remain within roughly
  0.01--0.08 ms of the preceding integrated baseline.
- The arm64-v8a GLES 3 APK rebuild and application lint pass. WebAssembly is 8,301,850 bytes,
  stripped `libmain.so` is 15,700,864 bytes, and the APK is 33,433,240 bytes. Android appearance
  and fragment cost remain unqualified without KVM or a physical device. CNA and Sharp Runtime
  remain clean on `next` and unmodified; no dependency defect was found.

### 2026-10-06 — Segmented palm crowns and per-instance LOD

- Replaced each near palm's solid ribbon frond with a thin curved rachis and nine pairs of tapered
  leaflets. The four near variants now contain 469--533 triangles instead of 289--317; the
  unchanged 102-triangle far tier uses wider ribbons that preserve the new feathered crown envelope.
  Native review rejected the first sparse, overlong seven-station leaflets and retained a denser,
  shorter nine-station silhouette.
- Replaced the 210 m whole-cell mesh choice with per-palm distance selection after cell rejection.
  A deterministic yaw-derived 125--145 m threshold staggers transitions rather than flipping every
  palm in a 62.5 m cell together. Benchmark JSON now records near and far palm populations.
- Added a deterministic `--viewpoint palms` composition, a native capture/gate, a browser mode and
  PNG, and an explicit segmented-leaflet startup diagnostic. Native and actual Chrome/SwiftShader
  WebGL 2 captures were inspected and agree; the browser scene reports 17 near and 7 far palms in
  four palm draws.
- Two complete five-view native repeats measured 1.1554--1.2202 ms overview,
  1.8172--1.9053 ms beach, 1.4788--1.6474 ms jungle, 0.8373--0.9157 ms driftwood, and
  1.5833--1.7823 ms monkeys. Overview submissions fall from 81 to 77 because all 299 visible palms
  use four far-variant draws; the mixed jungle view rises from 91 to 94, and other submissions stay
  at 124/26/97. Integrated timings overlap the preceding sample, so no isolated speedup is claimed.
- All 21 native tests pass, including 20 real OpenGL 3.3 scenes. The expanded Chrome 152/WebGL 2
  renderer/lifecycle/input/audio-init harness passes all 19 stages. The arm64-v8a GLES 3 APK and
  application lint pass; WebAssembly is 8,307,543 bytes, stripped `libmain.so` is 15,709,232 bytes,
  and the APK is 33,441,608 bytes. Revised Android appearance, aliasing, and cost remain unqualified
  without KVM or a physical device. CNA and Sharp Runtime remain clean on `next` and unmodified; no
  dependency defect was found.

### 2026-10-06 — Near-palm bark scars and coconut variation

- Added subtle texture-free leaf-scar rings to the near palm draws. A per-batch uniform skips the
  material branch for the far tier, preventing distant band aliasing and unnecessary fragment work.
  Added four or five small faceted coconuts beneath the crowns of variants three and one,
  respectively. The affected near meshes gain 32 or 40 triangles, bringing the four-variant range
  to 469--541 triangles; far meshes remain 102 triangles. No texture, asset, instance stream, CPU
  update, or draw submission was added.
- Moved the deterministic palm QA camera from the cluttered inner belt to a sparse coastal palm and
  view from the beach side. Native and actual Chrome/SwiftShader WebGL 2 captures were inspected;
  the leaf scars remain restrained, fruits sit partly beneath the leaflets, and both renderers agree.
  Native and browser gates now require an explicit material/coconut diagnostic.
- Two complete final five-view repeats retained 77/124/94/26/97 submissions and the exact prior
  near/far populations. Mean frame time measured 1.1773--1.2073 ms overview,
  1.7777--1.8420 ms beach, 1.4675--1.5164 ms jungle, 0.8188--0.8599 ms driftwood, and
  1.5762--1.5991 ms monkeys. The ranges overlap the previous integrated baseline, so no isolated
  speed change is claimed.
- All 21 native tests pass, including 20 real OpenGL 3.3 scenes. The complete Chrome 152/WebGL 2
  renderer/lifecycle/input/audio-init harness passes all 19 stages. The final arm64-v8a GLES 3 APK
  rebuild and application lint pass; WebAssembly is 8,314,191 bytes, stripped `libmain.so` is
  15,712,544 bytes, and the APK is 33,444,920 bytes. Android runtime appearance and cost remain
  unqualified without KVM or a physical device. CNA and Sharp Runtime remain clean on `next` and
  unmodified; no dependency defect was found.

### 2026-10-06 — Branch-supported broadleaf crowns and per-tree LOD

- Reworked only the near broadleaf canopy silhouette: eight slightly smaller and separated crown
  lobes gain deterministic contour irregularity, seven visible tapered support branches, and eight
  pointed perimeter leaves. The near tree rises from 688 to 776 triangles. Rounded the far mesh
  from 136 to 208 triangles to avoid the original diamond-like mid-distance silhouette while
  remaining under 27 percent of near geometry. No texture, asset, instance stream, or new content
  population was introduced.
- Replaced the 155 m whole-cell canopy choice with a deterministic yaw-derived 105--129 m per-tree
  threshold after cell rejection. The jungle QA view now splits 109 visible trees into 63 near and
  46 far instances; F1, first-frame diagnostics, and benchmark JSON expose the split. One additional
  variant/tier group raises that view from 94 to 95 total tracked submissions; other representative
  scene totals remain 77/124/26/97.
- Visual review retained the spatial branch-supported lobe design after rejecting an experimental small
  core with five-leaf fans per cluster that made the trees flat and star-shaped. Native overview,
  beach, and jungle captures were inspected. The actual Chrome/SwiftShader WebGL 2 jungle capture
  visually agrees and its gate requires the new diagnostic plus nonzero near/far populations.
- One benchmark run that overlapped the Emscripten link was explicitly discarded. Two isolated
  repeats measured 1.1770--1.2129 ms overview, 1.7932 ms beach, 1.4689--1.5011 ms jungle,
  0.8360--0.8621 ms driftwood, and 1.6264--1.6429 ms monkeys. The first three overlap the prior
  integrated baseline; the near-canopy-heavy monkey view adds roughly 0.04--0.07 ms mean.
- All 21 native tests pass, including 20 real OpenGL 3.3 scenes. The complete Chrome 152/WebGL 2
  renderer/lifecycle/input/audio-init harness passes all 19 stages. The arm64-v8a GLES 3 APK and
  application lint pass; WebAssembly is 8,326,360 bytes, stripped `libmain.so` is 15,721,568 bytes,
  and the APK is 33,453,944 bytes. Android runtime appearance and cost remain unqualified without
  KVM or a physical device. CNA and Sharp Runtime remain clean on `next` and unmodified; no
  dependency defect was found.

### 2026-10-06 — Pointed-leaf near bushes

- Reworked only the near meshes for all three low-bush variants. Each of four to six smaller,
  separated volume lobes now carries two folded pointed leaves with deterministic size, angle,
  colour, droop, ridge, and wind-phase variation. The variants rise from 288/360/432 to
  320/400/480 triangles. Their 48-triangle far meshes, 565 placements, instance streams, culling,
  CPU updates, and draw submissions are unchanged.
- Reused the canopy's five-vertex pointed-leaf primitive after generalizing its ridge height. Two
  more timid iterations were inspected and rejected because their small edge-only leaves were
  nearly invisible at first-person height. The retained smaller-lobe/overlaid-leaf composition is
  readable without forming a regular star. Native and actual Chrome/SwiftShader WebGL 2 jungle
  captures agree; both platform gates require the explicit near-bush diagnostic.
- Two isolated five-view repeats retained 77/124/95/26/97 tracked submissions and exact visible
  populations. Mean frame time measured 1.1614--1.1628 ms overview, 1.7704--1.7861 ms beach,
  1.4870--1.5316 ms jungle, 0.8477--0.8619 ms driftwood, and 1.6192--1.6376 ms monkeys. These
  ranges overlap the preceding integrated baseline closely enough that no isolated performance
  change is claimed.
- All 21 native tests pass, including 20 real OpenGL 3.3 scenes. The complete Chrome 152/WebGL 2
  renderer/lifecycle/input/audio-init harness passes all 19 stages. The arm64-v8a GLES 3 APK and
  application lint pass; WebAssembly is 8,326,857 bytes, stripped `libmain.so` is 15,722,544 bytes,
  and the APK is 33,454,920 bytes. Android runtime appearance and cost remain unqualified without
  KVM or a physical device. CNA and Sharp Runtime remain clean on `next` and unmodified; no
  dependency defect was found.

### 2026-10-06 — Upright broadleaf forest-floor variant

- Replaced the near mesh of ground-plant variant 1 with six upright broad leaves. Each leaf has a
  visible two-triangle petiole and a folded four-triangle pointed blade with deterministic height,
  reach, width, colour, and wind phase. The variant falls from 64 to 36 triangles while creating a
  materially different first-person silhouette. Its 16-triangle far mesh, all 2,225 placements,
  culling, instance data, CPU work, and draw submissions are unchanged; the other two variants
  retain their curved strap-leaf forms.
- Added `--viewpoint ground`, which deterministically selects a variant-1 plant near the winding
  path with clearance from understory, bushes, other floor plants, and static obstacles, then views
  it from player eye height. `IslandGameGroundPlantCapture` and a new actual-browser ground stage
  retain native/WebGL 2 screenshots and require the explicit mesh diagnostic plus a nonzero visible
  ground-plant population. Both images were inspected and agree.
- Two preliminary benchmark suites that overlapped the visible interactive game were rejected.
  Two clean isolated repeats retained 77/124/95/26/97 submissions and the exact prior populations.
  Mean frame time measured 1.1504--1.1513 ms overview, 1.6825--1.7669 ms beach,
  1.4611--1.4616 ms jungle, 0.7627--0.7730 ms driftwood, and 1.5347--1.5658 ms monkeys. The
  ranges show no regression; no speedup is claimed from the small triangle reduction.
- All 22 native tests pass, including 21 real OpenGL 3.3 scenes. The complete Chrome 152/WebGL 2
  renderer/lifecycle/input/audio-init harness passes all 20 stages. The arm64-v8a GLES 3 APK and
  application lint pass; WebAssembly is 8,336,760 bytes, stripped `libmain.so` is 15,729,232 bytes,
  and the APK is 33,461,608 bytes. Android runtime appearance and cost remain unqualified without
  KVM or a physical device. CNA and Sharp Runtime remain clean on `next` and unmodified; no
  dependency defect was found.

### 2026-10-06 — Near-rock strata and lichen material

- Added subtle world-space strata and sparse lichen colour modulation to only the near land/reef
  rock path in the existing portable natural-prop shader. The otherwise-unused vertex alpha is a
  private material selector; far rocks and contact disks retain the plain path. The pass adds no
  texture, geometry, instance data, CPU update, draw submission, collision change, or asset.
- Compared deterministic before/after reef captures and inspected the enlarged near rock plus the
  jungle composition. The retained contrast is readable close to first-person range without
  becoming a painted high-frequency pattern. Native OpenGL 3.3 and actual Chrome/SwiftShader
  WebGL 2 compile and execute the same path; browser lifecycle, overview, and reef gates require
  its explicit diagnostic.
- Two isolated five-view repeats retained 77/124/95/26/97 tracked submissions and exact scene
  populations. Mean frame time measured 1.1003--1.1116 ms overview, 1.6633--1.7042 ms beach,
  1.4197--1.4318 ms jungle, 0.7637--0.8007 ms driftwood, and 1.5232--1.5473 ms monkeys. These
  ranges overlap or improve on the preceding baseline, so no isolated speed change is claimed.
- All 22 native tests pass, including 21 real OpenGL 3.3 scenes. The complete Chrome 152/WebGL 2
  renderer/lifecycle/input/audio-init harness passes all 20 stages. The arm64-v8a GLES 3 APK and
  application lint pass; WebAssembly is 8,337,616 bytes, stripped `libmain.so` is 15,730,000 bytes,
  and the APK is 33,462,376 bytes. Android runtime appearance and cost remain unqualified without
  KVM or a physical device. CNA and Sharp Runtime remain clean on `next` and unmodified; no
  dependency defect was found.

### 2026-10-06 — Near-log bark and growth-ring material

- Added local-space longitudinal bark grooves/narrow fissures to near fallen-log sides and gently
  irregular growth rings to their cut ends through the existing portable natural-prop shader.
  Alpha remains a private material selector; moss/contact geometry and all far meshes keep the
  plain path. Wood detail fades from 42--70 m before the 72 m mesh transition. The same revision
  fades the preceding rock treatment from 58--90 m before its 92 m transition.
- Rejected the first visually inspected cut-end pass because its frequent high-contrast rings
  resembled a painted target. The retained lower-contrast, lower-frequency rings and bark fissures
  are readable at first-person range. Native OpenGL 3.3 and actual Chrome/SwiftShader WebGL 2
  close-log captures agree; the browser lifecycle and prop gates require the explicit diagnostic.
- Two isolated five-view repeats retain 77/124/95/26/97 tracked submissions and exact populations.
  Mean frame time measured 1.1121--1.1313 ms overview, 1.7399--1.7865 ms beach,
  1.4497--1.4644 ms jungle, 0.7623--0.7831 ms driftwood, and 1.5258--1.5775 ms monkeys. The
  small upper shift in log-bearing views is retained for the visual improvement; no free-cost
  claim is made.
- All 22 native tests pass, including 21 real OpenGL 3.3 scenes. The complete Chrome 152/WebGL 2
  renderer/lifecycle/input/audio-init harness passes all 20 stages. The arm64-v8a GLES 3 APK and
  application lint pass; WebAssembly is 8,339,116 bytes, stripped `libmain.so` is 15,731,472 bytes,
  and the APK is 33,463,848 bytes. Android runtime appearance and fragment cost remain unqualified
  without KVM or a physical device. CNA and Sharp Runtime remain clean on `next` and unmodified;
  no dependency defect was found.

### 2026-10-06 — Sun-bleached near-driftwood grain

- Added a soft local-space longitudinal weathering field to near driftwood sides and branch stubs
  through the existing natural-prop shader. Cut ends and far meshes retain the plain path; detail
  fades from 30--56 m before the 58 m mesh transition. The change adds no geometry, texture,
  instance data, CPU update, draw submission, collision change, or asset.
- Rejected the first inspected version because frequent dark dashes made its grain look stamped.
  The retained lower-frequency, lower-contrast pattern forms longer irregular marks and preserves
  the calm sun-bleached palette. Native OpenGL 3.3 and actual Chrome/SwiftShader WebGL 2 driftwood
  captures agree; the browser lifecycle and driftwood gates require its diagnostic.
- Two isolated five-view repeats retained 77/124/95/26/97 tracked submissions and exact scene
  populations. Mean frame time measured 1.0887--1.1593 ms overview, 1.6999--1.7333 ms beach,
  1.4114--1.4611 ms jungle, 0.7763--0.8006 ms driftwood, and 1.4925--1.5420 ms monkeys. The
  ranges overlap the preceding integrated baseline, so no measurable change is claimed.
- All 22 native tests pass, including 21 real OpenGL 3.3 scenes. The complete Chrome 152/WebGL 2
  renderer/lifecycle/input/audio-init harness passes all 20 stages. The arm64-v8a GLES 3 APK and
  application lint pass; WebAssembly is 8,340,007 bytes, stripped `libmain.so` is 15,732,256 bytes,
  and the APK is 33,464,632 bytes. Android runtime appearance and fragment cost remain unqualified
  without KVM or a physical device. CNA and Sharp Runtime remain clean on `next` and unmodified;
  no dependency defect was found.

### 2026-10-06 — Soundscape world diagnostics

- Completed the audio portion of the F2 tooling backlog. Six depth-aware contours now follow the
  terrain at the authoritative breeze 12--105 m, surf 18--125 m, and jungle 30--112 m inland
  crossfade boundaries. The distances became named `SoundscapeModel` constants, so rendering and
  mixing cannot silently drift apart. The opt-in pass remains two line-list submissions.
- The same view marks the nearest non-dormant bird and monkey audio candidates. While the pooled
  positional voice is playing, a larger species-coloured marker and listener-to-emitter line show
  the exact snapshotted source used by CNA `Apply3D`. F1 reports `AUDIO Z/C/E` counts plus live
  sources against the hard six-source ceiling. Candidate refresh outside ordinary playback is
  limited to F2 mode; the normal release presentation gains no extra wildlife scan.
- Strengthened the native world-debug capture gate to require all six contours and at least one
  selected candidate. The inspected overview contains 2,024 line segments in the same two draws.
  The Web input gate now requires the exact new F2 diagnostic. Its F1 key hold was raised from 100
  to 350 ms after two runs reproduced a missed event under the new SwiftShader debug-only load;
  the resulting final run passes rather than hiding the timing weakness.
- All 22 native tests pass, including 21 real OpenGL 3.3 scenes. The final Chrome 152/SwiftShader
  WebGL 2 renderer/lifecycle/input/audio-init harness passes all 20 stages and directly observes
  the new F2 path. The arm64-v8a GLES 3 APK and application lint pass; WebAssembly is 8,347,459
  bytes, stripped `libmain.so` is 15,738,144 bytes, and the APK is 33,470,520 bytes. Android
  runtime appearance remains unqualified without KVM or a physical device. CNA and Sharp Runtime
  remain clean on `next` and unmodified; no dependency defect was found.

### 2026-10-06 — Authoritative player ground-contact diagnostics

- Made `PlayerMotion`'s centre/cardinal/diagonal unit footprint offsets a single public
  authoritative array and retained that same array in the hot support-height/slope query. F2 now
  resamples exactly those nine points: 1.5 m green terrain-normal lines show permitted ground,
  red lines identify samples beyond the 38-degree walk limit, white rings identify every point at
  the resolved maximum support height, and a white vertical joins support to the player eye.
- Added `CONTACT sample/support/over-limit` counts without adding another draw submission. The
  renderer's extra terrain queries and lines exist only while F2 is enabled. Core regression
  coverage proves all offsets stay inside the capsule footprint, every query remains finite, and
  their maximum exactly reproduces `PlayerMotion::GroundHeight`.
- Added `--viewpoint contact`, which keeps the player on the clear jungle path while placing an
  independent elevated camera 6.5 m away. The new deterministic capture visibly resolves all nine
  markers instead of accepting counters for geometry hidden beneath a first-person camera. Both
  overview and detail capture gates require nine samples and at least one support point. The Web
  live-input gate requires the exact combined contact/audio F2 diagnostic.
- All 23 native tests pass, including 22 real OpenGL 3.3 scenes. The Chrome 152/SwiftShader WebGL 2
  renderer/lifecycle/input/audio-init harness passes all 20 stages. The arm64-v8a GLES 3 APK and
  application lint pass; WebAssembly is 8,352,169 bytes, stripped `libmain.so` is 15,741,472 bytes,
  and the APK is 33,473,848 bytes. Android runtime appearance remains unqualified without KVM or
  a physical device. CNA and Sharp Runtime remain clean on `next` and unmodified; no dependency
  defect was found.

### 2026-10-06 — Authoritative wildlife navigation diagnostics

- Exposed one compact authoritative centre/tier snapshot per fish school and moved the shark route
  target plus each monkey's home/active target into their renderer-independent public state. The
  simulations remain the only owners of those values; F2 no longer needs to infer navigation from
  a rendered heading. The seven monkeys retain the same 34 m home constraint and all runtime
  wildlife behaviour remains deterministic.
- Extended the existing two-draw F2 batch with nearby fish-school centre markers, the current
  shark route target and pose-to-target line, terrain-following monkey home circles, exact home
  markers, and active walk/run targets. The panel now reports wildlife marker/target/home-range
  counts. A dedicated monkey navigation capture reported 18 markers, 2 active targets, and 4
  nearby home ranges without another draw submission.
- Expanded long-session core coverage to verify deterministic fish-school centres, shark route
  targets, and monkey navigation state; all coordinates stay finite and inside the corresponding
  water belt or monkey home radius. One initial shark assertion incorrectly assumed every route
  target itself had deep seabed even though actual movement independently rejects shallow
  proposals; the final test checks the simulation's real offshore-belt contract.
- All 24 native tests pass, including 23 real OpenGL 3.3 scenes. The Chrome 152/SwiftShader WebGL 2
  renderer/lifecycle/input/audio-init harness passes all 20 stages. The arm64-v8a GLES 3 APK and
  application lint pass; WebAssembly is 8,360,630 bytes, stripped `libmain.so` is 15,755,280 bytes,
  and the APK is 33,487,656 bytes. Android runtime appearance remains unqualified without KVM or a
  physical device. CNA and Sharp Runtime remain clean on `next` and unmodified; no dependency
  defect was found.

### 2026-10-06 — Attempted versus resolved player-movement diagnostics

- Replaced the controller's private boolean step result with an internal accepted/obstacle/terrain
  classification while preserving the same collision, slope, substep, and axis-slide decisions.
  `PlayerMotion` now retains the latest nonzero start, intended endpoint, resolved endpoint,
  rejection counts, slide-substep count, and outer-ocean safety-clamp flag. Teleports reset the
  trace so stale data cannot be mistaken for post-teleport movement.
- Extended the existing F2 x-ray batch with exact-scale attempted and resolved lines and endpoint
  rings. Clear attempts are cyan, obstacle rejections orange, terrain rejections red, safety
  clamps magenta, and the resolved endpoint green. F1 reports `MOVE O/T/S/C`; the normal
  presentation still pays no extra draw submission and the complete F2 view remains two draws.
- Added `--viewpoint movement`, which deterministically walks east into a real palm proxy before
  positioning an independent camera. Its automated capture and visual inspection show the two
  overlapping exact endpoints at the trunk and report `2/0/1/0`: two obstacle rejections, zero
  terrain rejections, one slide substep, and no ocean clamp. Core coverage separately verifies an
  unobstructed move, blocked obstacle classification, and a forced ocean safety correction.
- All 25 native tests pass, including 24 real OpenGL 3.3 scenes. The Chrome 152/SwiftShader WebGL 2
  renderer/lifecycle/input/audio-init harness passes all 20 stages. The arm64-v8a GLES 3 APK and
  application lint pass; WebAssembly is 8,366,637 bytes, stripped `libmain.so` is 15,759,632 bytes,
  and the APK is 33,492,008 bytes. Android runtime appearance remains unqualified without KVM or a
  physical device. CNA and Sharp Runtime remain clean on `next` and unmodified; no dependency
  defect was found.

### 2026-10-06 — Fixed-sun jungle-floor material breakup

- Added restrained warm sun patches and cooler shaded areas to low-slope ground beginning 34 m
  inland. The terrain shader reuses its existing broad, medium, and fine fields plus the existing
  steep and 95--285 m detail fades; it adds no noise sample, texture, geometry, CPU state, asset,
  or draw submission. A coherent early branch skips the colour shaping on shore, distant, and
  fully steep fragments. This is deliberately static material breakup for the permanent sunny
  presentation, not geometric canopy-shadow simulation.
- Compared deterministic native before/after jungle and ground captures and retained the second,
  readable-but-bounded contrast pass. The beach composition remained unchanged. Native OpenGL 3.3
  and actual Chrome/SwiftShader WebGL 2 jungle and ground images agree; the browser startup, grass,
  and jungle stages now require the explicit dapple diagnostic.
- Built and measured a same-session no-dapple control twice, restored the final path, then ran two
  final five-view suites. Four content-heavy ranges overlap the control. Final means were
  1.1409--1.2137 ms overview, 1.8151--1.9109 ms beach, 1.4837--1.6552 ms jungle,
  0.8237--0.9109 ms driftwood, and 1.6669--1.7469 ms monkeys at unchanged
  77/124/95/26/97 submissions. No isolated performance change is claimed; hardware-browser and
  physical-mobile fragment cost remain open.
- All 25 native tests pass, including 24 real OpenGL 3.3 scenes. The complete Chrome 152/SwiftShader
  WebGL 2 renderer/lifecycle/input/audio-init harness passes all 20 stages. The arm64-v8a GLES 3
  APK and application lint pass; WebAssembly is 8,367,403 bytes, stripped `libmain.so` is
  15,760,336 bytes, and the APK is 33,492,712 bytes. Android runtime appearance remains unqualified
  without KVM or a physical device. CNA `f243c63a1dc5` and Sharp Runtime `21c06910f7cf` remain
  clean on `next` and unmodified; no dependency defect was found.

### 2026-10-06 — Deterministic traversal scenarios

- Added six complete dry-ground to near-shore-swimming to dry-ground controller runs. The set
  derives the deepest cove and furthest headland from 720 shoreline samples, then adds four fixed
  coast directions. Every route must cross the water hysteresis exactly once in each direction,
  reach 20 m offshore, return at least 5 m inland, and keep all positions finite.
- Added a deterministic island-wide search for the steepest approach that begins on valid walkable
  support. The resulting controller run must either follow changing ground uphill or exercise the
  authoritative slope limit, remain in walking mode, and stay numerically stable. This adapts to
  intentional terrain composition changes without hard-coding a fragile coordinate.
- Added a route into a real solid land rock selected from `NaturalPropLayout`, using its exact
  generated collision proxy. The controller must advance toward it, preserve capsule clearance,
  accumulate obstacle rejections, and accumulate zero terrain rejections, proving the scenario
  reaches the intended collision path rather than failing incidentally on terrain.
- The focused renderer-independent core test passes, and the complete isolated accelerated native
  suite passes 25/25 in 34.58 seconds, including 24 real OpenGL 3.3 scenes and the graphics-profile
  refusal audit. Web and Android products are unaffected because their runtime sources did not
  change. CNA and Sharp Runtime remain read-only; no dependency defect was found.

### 2026-10-06 — Bounded debug inspection camera

- Added a renderer-independent `DebugFreeCamera` with normalized yaw-relative three-axis flight,
  0.1-second frame clamping, 12/36 m/s normal/Shift speeds, `+/-820 m` horizontal bounds, and
  `-18..260 m` vertical bounds. In debug builds, F3 activates it only while F1 is visible; WASD
  flies horizontally, Q/E vertically, the player and locomotion bob remain frozen, and F3 or
  hiding F1 restores the unchanged authoritative player eye. Release builds omit the member,
  activation, and flight branch at compile time.
- Extended F1 with explicit camera/player positions and free-camera state. Input diagnostics now
  distinguish player and camera coordinates. Added core coverage for inactivity, frame clamping,
  normalized diagonal movement, yaw-relative fast travel, bounds, and deactivation retention.
- Added `IslandGameFreeCameraInput`, which uses real X11 events on CNA's private Xwayland display
  and proves both F3 edges, camera displacement, and zero player displacement. It runs serially so
  concurrent capture windows cannot steal focus. Updated the Web harness for the explicit input
  schema and made its B/F2 key edges span several SwiftShader frames, eliminating observed
  input-test timing races rather than weakening any assertion.
- The complete isolated native suite passes 26/26, including 25 display-backed OpenGL tests. The
  complete Chrome 152/SwiftShader WebGL 2 renderer/lifecycle/input/audio-init harness passes all
  20 stages. The arm64-v8a GLES 3 APK and application lint pass; WebAssembly is 8,367,853 bytes,
  stripped `libmain.so` is 15,765,176 bytes, and the APK is 33,497,552 bytes. Android runtime
  appearance remains unqualified without KVM or a physical device. CNA and Sharp Runtime remain
  clean on `next` and unmodified; no dependency defect was found.

### 2026-10-06 — Asset gate and deterministic world report

- Added schema-versioned `assets/manifest.json`. Its 17 procedural records map every current
  generated-content attribution row to 32 owning source files; the external-file list is empty
  because the game still ships no external mesh, texture, animation, font, or audio data.
- Added the standard-library-only `tools/validate_assets.py`. It rejects malformed schema, unsafe
  paths, duplicate records, missing generator sources, orphaned `ASSET_LICENSES.md` rows, unlisted
  files under `assets/`, incomplete provenance fields, missing files, and SHA-256 mismatches. It
  can also write a small machine-readable JSON report.
- Added the native-only `island-world-report` target. It reconstructs the authoritative world
  twice, validates finite/ranged placement data and derived populations, then writes stable-order
  JSON with public seeds, terrain/surface sampling, all content counts, collision cells, wildlife,
  and a platform-local fingerprint. The current fingerprint is `2e2cd5d73dba8a18`; sampled shore
  radii are 209.7295--252.0567 m, heights are -14.5815--19.3941 m, and the report agrees with 808
  static obstacles in 479 cells.
- Added `IslandAssetValidation` and `IslandWorldReport` to native CTest. The expanded isolated
  suite passes 28/28 in 38.74 seconds, including 25 display-backed OpenGL tests and the serial
  real-input free-camera test. Web reconfiguration confirms the host target is absent and leaves
  the 8,367,853-byte WASM unchanged. Android reconfiguration, arm64 APK assembly, and lint pass
  with the same 15,765,176-byte stripped library and 33,497,552-byte APK. The tools are excluded
  when cross-compiling, so runtime products and their qualification claims are unchanged.

### 2026-10-06 — Sustained moving performance route

- Added renderer-independent `BenchmarkTraversalRoute`, an 11-waypoint open Catmull-Rom camera
  rail measuring 675.9635 m. Core regression coverage checks clamped/non-finite inputs, exact
  endpoints, finite normalized tangents, island-scale bounds, shallow-water inclusion, deep-
  interior coverage, and a route length above 600 m.
- Added the benchmark-only `--viewpoint traversal`. It moves the authoritative player and camera
  once per measured frame through southern shallows, beach, the primary trail, east and west
  clearings, interior jungle, the north coast, and northern shallows. Terrain-resolved camera
  height and walking/swimming presentation speed keep water, ripple, grass-interaction, wildlife,
  culling, LOD, and quarter-metre contact-shadow refresh paths active without changing ordinary
  fixed-timestep play.
- Bumped the benchmark report to schema 2 and added min/max/mean ranges for tracked submissions,
  terrain cells, contact shadows, palms, canopy, and grass, plus route length, camera endpoints,
  and sampled shore-distance coverage. The native runner now writes five fixed reports and the
  moving traversal report; `IslandGameTraversalBenchmark` protects the complete 600-sample path.
- The first actual OpenGL 3.3 run completed all 600 samples and produced valid JSON. It covered
  -18.5544--208.6531 m signed shore distance, 12--118 tracked submissions, 6--47 terrain cells,
  0--43 contact shadows, 0--325 palms, 0--281 canopy trees, and 0--3,466 grass clusters. Its timing
  is not a baseline because the operator's visible interactive game was using the same GPU. Clean
  repeated native timing and Web/physical-Android equivalents remain open. No CNA or Sharp Runtime
  file was changed and no dependency defect was found.
- The expanded isolated native suite passes 29/29 in 45.82 seconds, including 26 actual OpenGL 3.3
  scenes and the real-input free-camera check. Chrome 152/SwiftShader passes the complete WebGL 2
  renderer, scene, lifecycle, input, and audio-initialization harness. A clean arm64-v8a GLES 3
  rebuild, Gradle APK assembly, and application lint pass; current artifacts are 8,377,460 bytes
  for WebAssembly, 15,774,728 bytes for the stripped Android library, and 33,507,104 bytes for the
  APK. Android runtime appearance and route timing remain unqualified without KVM or a physical
  device.

### 2026-10-06 — Authoritative bird route diagnostics

- Consolidated each bird's authored orbit, altitude/clearance, direction, and exact initial pose
  into one immutable public route snapshot. `BirdFlock` also derives three family envelopes from
  those same 24 records: shore birds at 218--248 m radius, canopy birds at 70--150 m, and high
  birds at 125--233 m. The simulation and diagnostic layer no longer maintain parallel route
  constants.
- Extended the existing two-draw F2 x-ray batch with all three minimum/maximum radius and altitude
  volumes, all 24 spawn headings, and a live radial steering target for each nearby bird. Cyan,
  green, and violet distinguish shore, canopy, and high families. `BIRD R/S/T` reports exact
  family-region, spawn, and active-target counts without affecting ordinary presentation work.
- Added deterministic core coverage for route/spawn identity, bounded steering inputs, family
  populations, and exact orbit limits. The dedicated wildlife capture gate now requires three
  route regions, 24 spawn markers, and a nonzero live-target population. Visual review of both the
  overview and monkey-navigation captures confirmed that the route volumes remain readable and
  led to splitting the formerly overlong F1 world line into compact world, navigation, and contact
  rows.
- The complete isolated native suite passes 29/29 in 65.46 seconds, including 26 actual OpenGL 3.3
  scenes and real X11 free-camera input. Chrome 152/SwiftShader passes the complete WebGL 2
  renderer, scene, lifecycle, input, and audio-initialization harness with the stronger F2 output
  assertion. The arm64-v8a GLES 3 build, Gradle APK assembly, and lint pass; artifacts are 8,387,290
  bytes for WebAssembly, 15,792,120 bytes for the stripped Android library, and 33,524,496 bytes for
  the APK. The visible Linux game remained running during validation, so these native timings are
  regression evidence rather than a performance baseline. Android runtime remains unqualified
  without KVM or a physical device. No CNA or Sharp Runtime file was changed and no dependency
  defect was found.

### 2026-10-06 — First-party ASan and UBSan gate

- Added `ISLAND_ENABLE_SANITIZERS` and a dedicated `linux-sanitize` configure/build/test preset.
  It uses AddressSanitizer plus UndefinedBehaviorSanitizer on `island_core`, its test executable,
  and the deterministic world-report executable, with frame pointers retained and the build held
  to two workers. `ISLAND_BUILD_GAME=OFF` keeps CNA, Sharp Runtime, renderer code, and unrelated
  dependency targets outside this focused diagnostic build.
- The initial managed-runner attempt confirmed that LeakSanitizer cannot operate under the host's
  process tracing and exits before a project result. The reproducible CTest preset therefore
  disables only leak detection; the Linux guide provides a direct `detect_leaks=1` invocation for
  an ordinary untraced terminal. This is an execution-environment limitation, not a CNA or Sharp
  Runtime defect.
- The final instrumented suite passes 3/3 in 46.73 seconds: the complete deterministic core test,
  world-report generation/validation, and the asset provenance gate. No address, bounds, lifetime,
  alignment, integer, floating-point, enum, or other enabled undefined-behaviour diagnostic was
  emitted. `build-sanitize/` is reproducible and is removed after the pass to avoid retaining a
  redundant build tree.

### 2026-10-06 — Clean Linux release reconstruction

- Configured a new release tree under `/tmp` without reusing any project object, generated file,
  or configured dependency target. It consumed the clean read-only sibling CNA `next` at
  `f243c63a1dc5`, Sharp Runtime `next` at `21c06910f7cf`, EasyGL `develop` at `1edf4c1a4708`, and
  MetaGL `develop` at `40829ca76a4e`, while all build outputs stayed outside those repositories.
  The complete 623-step game/core/tool build succeeded with two workers.
- GCC 14 release optimization emitted four dependency-only diagnostics in CNA bitmap, CNB/glTF,
  and XNB import/write helpers. No island-simulator source warning was emitted, the game does not
  use those source-content paths, and no behavioural reproduction or material game impact was
  found; they are therefore not recorded as confirmed framework defects in `BUGS.md`.
- The clean release core, world-report, and asset gates pass 3/3 in 3.68 seconds. The actual OpenGL
  3.3 executable then passed one isolated smoke followed by three repeated fresh-process smokes;
  a release overview capture was also rendered and visually inspected. The optimized game
  executable is 974,776 bytes before its dynamic CNA/runtime dependencies. A longer subjective
  release traversal remains open.
- The 94 MB clean build tree and its temporary converted capture were removed after validation.
  CNA and Sharp Runtime remain clean and unmodified.

### 2026-10-06 — Native capture integrity and non-degeneracy gate

- Added a dependency-free P6 PPM parser and capture analyzer. It requires a complete 8-bit payload,
  minimum/exact dimensions, useful mean brightness, luminance variation, at least 64 quantized RGB
  bins, and coarse 8 by 4 spatial contrast; overwhelmingly black or white output is rejected. The
  report records channel extrema/means, luminance statistics, dark/bright fractions, colour-bin
  count, spatial contrast, and a payload SHA-256 without treating the hash as a portable golden.
- Added parser regression coverage for comments and CRLF headers plus rejection of ASCII PPM,
  truncated payloads, wrong dimensions, and flat solid output. `IslandGameCaptureValidation`
  depends on the actual OpenGL overview capture and writes a machine-readable JSON report only
  after that producer succeeds.
- The current 1280 by 720 OpenGL 3.3 overview passes with 291 quantized colour bins, 26.05
  luminance standard deviation, and 172.87 coarse spatial contrast. This catches empty/flat render
  regressions while remaining tolerant of harmless driver-level pixel variation; semantic golden
  and cross-renderer image comparison remain open.
- The expanded isolated native suite passes 31/31 in 66.20 seconds, including 26 actual OpenGL 3.3
  scenes, the ordered capture analyzer, and real X11 free-camera input. WebGL 2 and Android GLES 3
  configurations both regenerate and report no build work because the new gate is intentionally
  native-host tooling. CNA and Sharp Runtime remain clean and unmodified; no dependency defect was
  found.

### 2026-10-06 — Reproducible in-game world identity

- Added the renderer-independent `WorldIdentity` component as the single diagnostic view of the
  palm, grass, jungle, and natural-prop layout defaults. The deterministic report fingerprint and
  JSON now consume that component instead of repeating the four constants; the established
  fingerprint remains `2e2cd5d73dba8a18`. Core tests lock the exact seed set, fixed-width overlay
  formatting, and stable English clipboard text.
- Added two seed rows to the F1 panel and an F4 action that writes the stable report through CNA's
  public clipboard API. A three-second status says `COPIED` only after exact immediate readback;
  unavailable or permission-gated platforms report `UNAVAILABLE` while leaving the values visible.
  The existing private-Xwayland input gate now proves a native round trip before exercising F3.
  Visual review of the 1280 by 720 OpenGL capture confirmed that all 22 rows and the expanded key
  legend fit without clipping.
- The complete native OpenGL 3.3 suite passes 31/31 in 66.18 seconds. The first-party ASan/UBSan
  suite passes 4/4 in 47.71 seconds with leak detection disabled only for the documented managed
  tracing limitation; its 37 MB temporary build was removed afterward. Chrome 152/SwiftShader
  passes the complete WebGL 2 harness, including an actual confirmed F4 clipboard round trip from
  the focused canvas. The arm64-v8a GLES 3 build, APK assembly, and application lint pass; current
  artifacts are 8,397,216 bytes for WebAssembly, 15,817,616 bytes for the stripped Android
  library, and 33,549,992 bytes for the APK.
- CNA and Sharp Runtime remain clean and unmodified and no dependency defect was found. Project
  build trees currently occupy approximately 2.6 GB native, 73 MB Web, 1.1 GB standalone Android,
  1.0 GB Gradle native, and 380 MB Gradle package output, with 379 GB free on the volume.

### 2026-10-06 — Deterministic audio source audit and preview export

- Added renderer-independent `AudioAnalysis` over the exact generated PCM16 buffers. It reports
  normalized DC offset, RMS energy, peak, end-to-start seam step, maximum ordinary adjacent step,
  zero-crossing rate, duration, and a stable source fingerprint without copying samples. Synthetic
  core coverage locks normalization, duration, RMS, peak, seam, step, and invalid-empty behaviour.
- Added the native `island-audio-report` host tool and CTest gate for all 12 procedural sources. It
  locks the intended 22.05 kHz frame counts, bounded energy/headroom/DC/steps, exact silent
  endpoints for one-shots, loop seams no worse than 125 percent of normal internal steps, unique
  fingerprints, and the 2,445,344-byte PCM footprint. Current normalized peaks are 0.56--0.86,
  absolute DC offset is below 0.0068, and loop seams are 0.0036--0.0402.
- Added optional standard little-endian mono PCM16 WAV export for isolated listening. An independent
  Python CTest invokes the real exporter in a temporary directory, parses every WAV, verifies the
  complete 12-file inventory, channel/sample-width/rate/frame count/payload, and deletes all
  previews. `docs/audio-review.md` separates these objective gates from the still-open 20-minute
  headphone/speaker route and defines exact review locations and fatigue criteria.
- The expanded native suite passes 33/33 in 67.14 seconds, including 26 OpenGL 3.3 scenes. The
  first-party ASan/UBSan suite passes 6/6 in 52.68 seconds; its 41 MB build and the manual 2.4 MB
  WAV preview were removed. The current Chrome 152/SwiftShader WebGL 2 harness passes in full, and
  the arm64-v8a GLES 3 build, Gradle APK assembly, and application lint pass. Because analysis is
  host-only and its static object is unreferenced by the game, package sizes remain 8,397,216 bytes
  WebAssembly, 15,817,616 bytes stripped Android library, and 33,549,992 bytes APK.
- CNA and Sharp Runtime remain clean and unmodified. No framework defect was found and `BUGS.md`
  requires no new entry.

### 2026-10-06 — First-draw startup phase instrumentation

- Added application-owned monotonic timing for deterministic world construction, CNA framework
  initialization before `LoadContent`, GPU-backed/procedural content creation, the interval before
  the first draw, the first game-owned draw body, and total construction-to-draw time. The normal
  presentation is unchanged; every backend emits one concise diagnostic on its first frame.
- Added `--startup-report <path>` with schema-1 JSON containing renderer, viewpoint, resolution,
  explicit audio startup state, and finite millisecond phase values. The documentation distinguishes
  game-owned draw submission from CNA's later present and GPU completion, so the evidence is not
  overstated as GPU-only timing.
- Added an ordered native producer/parser pair. An actual private-display OpenGL 3.3 overview draw
  creates the file; the independent Python gate rejects wrong identity/audio state, missing,
  negative, non-finite, incoherent, or over-one-minute values. The first concurrent debug run
  measured 494.8805 ms world construction, 526.8526 ms GPU/content creation, 2.6629 ms first draw,
  and 1075.9222 ms total. Because the operator's visible game remained active on the same GPU,
  these values validate instrumentation but are not adopted as a performance baseline.
- Extended the actual-browser WebGL 2 harness to require the same finite startup timing line. The
  complete Chrome 152/SwiftShader harness passes all 20 renderer/lifecycle/input stages and reports
  134.8 ms world construction, 246.8 ms content creation, 15.7 ms first draw, and 438.0 ms total;
  those values are functional evidence, not a hardware-browser baseline.
- The expanded native suite passes 35/35 in 41.07 seconds, including 27 actual OpenGL scenes and
  real X11 free-camera input. The arm64-v8a GLES 3 target builds, the 33,555,128-byte APK assembles,
  and application lint passes; its stripped library is 15,822,752 bytes and the WebAssembly module
  is 8,407,294 bytes. Android runtime startup remains unqualified without KVM or a physical device.
  Repeated isolated Linux measurements and cross-target startup budgets remain open. CNA and Sharp
  Runtime remain clean and unmodified; no framework defect was found.

### 2026-10-06 — Fixed-sun jungle facet lighting

- Reconstructed a safe per-fragment geometric face normal from derivatives of the already-present
  world position in the portable instanced jungle shader. An absolute fixed-sun response treats
  opaque foliage as two-sided and a restrained upward fill preserves tropical daylight. The pass
  covers canopy near/far meshes, understory, bushes, and forest-floor plants without adding a
  normal stream, textures, vertices, instances, CPU updates, or draw submissions.
- Kept the technique inside island-simulator and within GLSL 330 / GLSL ES 300 core functionality.
  The full Chrome 152/SwiftShader WebGL 2 harness passes all 20 stages and explicitly gates the new
  path. Side-by-side native OpenGL and browser jungle/ground captures show the same composition and
  clearer crown, trunk, and leaf facets. The stock-effect fallback remains functional but unlit.
- The complete native suite passes 35/35 in 41.13 seconds, including 27 OpenGL scenes and real X11
  input. Its concurrent jungle benchmark completed at 2.6643 ms mean / 3.3668 ms p95 and traversal
  at 2.9798 ms mean / 3.7292 ms p95; the visible game shared the GPU, so these are safety checks,
  not an isolated cost baseline. Physical-mobile and hardware-browser fragment cost remain open.
- The arm64 GLES 3 target builds, the 33,555,720-byte APK assembles, and application lint passes;
  its stripped library is 15,823,344 bytes and the WebAssembly module is 8,407,981 bytes. Android
  runtime shader compilation/appearance remains unqualified without KVM or a physical device. CNA
  and Sharp Runtime remain clean and unmodified, and no dependency defect was found.

### 2026-10-06 — Authored primary jungle footpath material

- Promoted the established south-to-north corridor curve, range, and reserved half-width to one
  authoritative `VegetationLayout` definition. Palm, jungle, prop, grass, QA-camera, and terrain
  material code now consume those constants; deterministic core coverage locks the centre crossing,
  ordered range, useful reserved width, and sampled route membership without changing the stable
  world fingerprint `2e2cd5d73dba8a18`.
- Added a portable terrain-shader treatment that reuses existing noise/detail fields to form a
  narrow broken edge, lightly compacted centre, muted dry-earth colour, and endpoint, shoreline,
  slope, and camera-distance fades. Existing short mesh grass remains over the surface, so the cue
  reads as a lightly worn tropical footpath rather than a bare road or objective marker. It adds no
  texture, geometry, CPU placement, instance, or draw submission.
- Native OpenGL 3.3 and actual Chrome 152/SwiftShader WebGL 2 beach captures were inspected at 1280
  by 720 and agree on the retained cue. The Web harness passes all 20 renderer, lifecycle, input,
  audio-initialization, and scene stages with the new diagnostic required by lifecycle, approach,
  and jungle views. The complete isolated native suite passes 35/35 in 41.58 seconds, including 27
  actual OpenGL scenes and real X11 free-camera input.
- Concurrent native safety measurements completed at 2.6113 ms mean / 3.3813 ms p95 for the jungle
  view and 3.0229 ms mean / 3.8266 ms p95 over the 675.9635 m traversal. The operator's visible game
  shared the GPU, so these are not isolated branch-cost baselines. The arm64 GLES 3 target builds,
  the 33,557,200-byte APK assembles, and application lint passes; the stripped library is 15,824,824
  bytes and WebAssembly is 8,409,554 bytes. Android runtime appearance and physical-mobile fragment
  cost remain unqualified without a suitable device. CNA and Sharp Runtime remain clean and
  unmodified, and no dependency defect was found.

### 2026-10-06 — Distinct authored clearing identities

- Promoted the existing east/west clearing centres and radii into shared renderer-independent
  definitions. Vegetation reservation, F7/F8 teleports, two new fixed QA viewpoints, deterministic
  tests, and terrain rendering now consume the same values instead of repeating coordinates. The
  stable world fingerprint remains `2e2cd5d73dba8a18` and no placement count changed.
- Added portable warm-dry east and darker leaf-rich west ground treatments with soft noise-broken
  boundaries, reduced/increased litter respectively, and restrained suppression of the static
  canopy dapple inside their open centres. Short grass remains. The mask evaluates only the
  relevant side and the retained squared-distance implementation avoids the first pass's fragment
  square root; no texture, geometry, CPU placement, instance, collision, or draw submission was
  added.
- Dedicated 1280 by 720 native OpenGL 3.3 and actual Chrome 152/SwiftShader WebGL 2 captures of
  both clearings were inspected and agree without visible hard circular edges. The final Web
  harness passes all 22 lifecycle, deployment, renderer, scene, and input stages. After the mask
  optimization, targeted native clearing/benchmark coverage passes 4/4 and the final complete
  suite passes 37/37 in 43.05 seconds, including 29 actual OpenGL scenes and real X11 input.
- Two final concurrent native safety runs measure 2.4283--2.7870 ms mean / 3.1108--3.5510 ms p95
  for the jungle view and 3.0796--3.0869 ms mean / 3.8265--3.9338 ms p95 over the 675.9635 m
  traversal. They are not isolated branch-cost baselines because the operator's visible game shared
  the GPU. The arm64 GLES 3 target builds,
  the 33,559,808-byte APK assembles, and application lint passes; the stripped library is
  15,827,432 bytes and WebAssembly is 8,412,017 bytes. Android appearance and physical-mobile cost
  remain unqualified without a suitable device.
- No CNA or Sharp Runtime file was edited by island-simulator and no dependency defect was found.
  Sharp Runtime remains clean on locked `next`. During this milestone, the sibling CNA `next`
  worktree acquired extensive external staged/unstaged changes at the locked commit; they were not
  touched or consumed. Configure/build validation continued against the clean project-owned export
  of locked CNA commit `f243c63a1dc56404d732deed902bd18bde9db9f7`.

### 2026-10-06 — Authored natural clearing landmarks

- Added five deliberately placed land rocks and one fallen log to the outer rings of the shared
  clearing definitions. The east room uses an irregular three-rock group; the west room uses two
  rocks and a transverse cut-ended log. All reuse the existing natural-prop meshes, variants,
  shader, LOD, culling, contact-shadow, instance batches, and collision paths. No texture, mesh,
  shader, render pass, or draw submission was added, and the open central walking areas remain
  intact.
- Exposed one renderer-independent clearing-membership query and marked authored landmark records
  explicitly. Core tests now lock deterministic flags/counts, deep-interior support, outer-ring
  position, complete log-footprint membership, central-room clearance, and separation between
  procedural props and reserved routes. The world report hashes the new identity flags. Its
  intentional new fingerprint is `d83ecd714e10fc9f`, with 171 total rocks including 14 reef
  outcrops, 12 fallen logs, and 820 static proxies in 481 occupied 16 m cells.
- Dedicated 1280 by 720 native OpenGL 3.3 and actual Chrome 152/SwiftShader WebGL 2 east/west
  clearing captures were inspected and agree. The final complete native suite passes 37/37 in
  46.62 seconds, including 29 actual OpenGL scenes, the hardened core assertions, and real X11
  input. The complete Chrome harness passes all 22
  lifecycle, deployment, renderer, scene, and input stages and requires the shared landmark
  diagnostic in both clearing views.
- Sequential native safety runs measured 2.3671 ms mean / 2.7291 ms p95 for the fixed jungle view
  and 2.7827 ms mean / 3.4859 ms p95 over the 675.9635 m traversal. They are not isolated baselines
  because the operator's visible game shared the GPU. The arm64 GLES 3 target builds, the
  33,563,264-byte APK assembles, and application lint passes; the stripped library is 15,830,888
  bytes and WebAssembly is 8,414,099 bytes. Android landmark appearance and physical-mobile cost
  remain unqualified without a suitable device.
- No CNA or Sharp Runtime file was edited by island-simulator and no dependency defect was found.
  Sharp Runtime remains clean on locked `next`. The sibling CNA `next` worktree still contains
  extensive external staged/unstaged changes at the locked commit; they were neither touched nor
  consumed. All builds used the clean project-owned export of CNA commit
  `f243c63a1dc56404d732deed902bd18bde9db9f7` and locked Sharp Runtime commit
  `21c06910f7cf887cceebb45200e07b43dd45fef4`.

### 2026-10-06 — Split-leaf floor plant and instance-accurate jungle LOD

- Replaced the near mesh of ground-plant variant 2 with five low tropical leaves. Each leaf has a
  visible petiole, solid curved midrib, and three separated pairs of folded lobes with deterministic
  length, width, colour, and wind phase. The retained third visual pass is 140 triangles instead of
  the former 72-triangle strap-leaf rosette; its 16-triangle far mesh, all 2,225 placements,
  instance layout, batching, shader, and draw-submission ceiling remain unchanged.
- Visual review rejected the first pass as too broad and the second as too skeletal. It also caught
  a pre-existing game-side LOD precision problem: the variant-2 plant 2.8 m from the QA camera was
  rendered with its far mesh because the renderer measured the centre of a shared 80 m scene cell.
  No CNA defect was involved. The game now uses the cell AABB only for conservative rejection, then
  uses actual instance distance to enforce maximum range and select near/far LOD for canopy,
  understory, bushes, and ground plants. The existing instance record already contained the
  necessary positions and thresholds.
- Added explicit near/far counters for the three lower jungle layers. The fixed first-frame report,
  native ground gate, and Chrome harness now expose per-instance LOD populations; the ground QA
  view targets variant 2 and reports 18 near / 34 far ground plants. Native and actual Chrome
  152/SwiftShader 1280 by 720 ground captures were inspected and agree. The world layout and stable
  fingerprint remain `d83ecd714e10fc9f`.
- Two complete six-view native safety suites ran while the operator's visible client shared the
  GPU. Mean/p95 ranges were 1.7681--1.7834/1.9791--2.0042 ms overview,
  2.7009--2.7467/3.1837--3.2864 ms beach, 2.3368--2.8219/2.6892--3.4801 ms jungle,
  1.3576--1.3643/1.7558--1.8695 ms driftwood, 2.5465--2.9731/3.3558--3.8870 ms monkeys,
  and 2.8276--3.2742/3.5034--4.2125 ms traversal. These are functional safety evidence, not an
  isolated performance baseline.
- The final native suite passes 37/37 in 171.17 seconds under shared host/GPU load, including 29
  real OpenGL scenes and real X11 input. The complete Chrome harness passes all 22 lifecycle,
  deployment, renderer, scene, and input stages. The arm64 GLES 3 target builds, the 33,573,584-byte
  APK assembles, and application lint passes; the stripped library is 15,841,208 bytes and
  WebAssembly is 8,428,883 bytes. Android appearance and physical-mobile cost remain unqualified.
- No CNA or Sharp Runtime file was edited and no dependency defect was found. Sharp Runtime remains
  clean on locked `next`; the externally dirty sibling CNA `next` worktree was neither touched nor
  consumed. Validation used the clean project-owned export of locked CNA commit
  `f243c63a1dc56404d732deed902bd18bde9db9f7` and Sharp Runtime commit
  `21c06910f7cf887cceebb45200e07b43dd45fef4`.

### 2026-10-06 — Forked driftwood and instance-accurate natural-prop LOD

- Replaced the single four-triangle branch spike on near driftwood variant 2 with three curved,
  tapered, six-sided limbs. The retained oblique composition is 160 triangles instead of 74 and
  reads as a distinct weathered fork from first-person height; the 30-triangle far mesh, all 28
  placements, material shader, instance format, batching, and submission ceiling remain unchanged.
  The first side-on review was rejected because the branch depth collapsed into the camera axis.
- Extended exact instance-distance range/LOD selection to rocks, fallen logs, and driftwood after
  discovering their remaining cell-centre decision. Shared cell AABBs now provide conservative
  rejection only. Existing log/driftwood position fields are populated and consumed without
  changing GPU instance data; the dedicated QA view reports four near / zero far driftwood. Core
  coverage now locks the presence of all three deterministic driftwood variants. This was
  game-side code and no dependency defect was involved.
- Native OpenGL 3.3 and actual Chrome 152/SwiftShader WebGL 2 captures were inspected and match.
  Two six-view native safety suites completed under shared GPU load; representative mean/p95
  ranges were 1.7762--1.7842/2.0748--2.1294 ms overview,
  2.6986--2.7497/3.0540--3.4583 ms beach, 2.3285--2.3619/2.6086--2.6343 ms jungle,
  1.4848--1.4964/1.8351--1.9153 ms driftwood, 2.5316--2.5605/3.3698--3.3737 ms monkeys,
  and 2.8703--2.9211/3.7702--3.9156 ms traversal. The changed driftwood QA camera makes its
  historical scene population non-comparable; these are functional safety checks, not isolated
  geometry/culling cost claims.
- The final native suite passes 37/37 in 42.98 seconds, including 29 actual OpenGL scenes and real
  X11 input. The complete Chrome harness passes all 22 lifecycle, deployment, renderer, scene, and
  input stages. The arm64 GLES 3 target builds, the 33,578,336-byte APK assembles, and application
  lint passes; the stripped library is 15,845,960 bytes and WebAssembly is 8,434,685 bytes. Android
  appearance and physical-mobile cost remain unqualified. The world layout and stable fingerprint
  remain `d83ecd714e10fc9f`.
- No CNA or Sharp Runtime file was edited and no dependency defect was found. Validation used the
  clean project-owned CNA export at locked commit `f243c63a1dc56404d732deed902bd18bde9db9f7`
  and Sharp Runtime `21c06910f7cf887cceebb45200e07b43dd45fef4`. At the final read-only boundary
  audit, the external CNA worktree had independently become clean on the newer `next` commit
  `39e84add777730969f58074fa1ffa010dc5c7d2c`; deliberate baseline review and requalification are
  recorded as the next task rather than silently mixed into this milestone.

### 2026-10-06 — CNA next 39e84add dependency baseline qualification

- Reviewed CNA `f243c63a1dc5..39e84add7777` read-only before consumption. The 302-file change retires
  89,195 lines of direct Win32, X11, and Wayland platform code and makes SDL3 CNA's graphical
  platform path. Island-simulator already selected SDL3 on all three required targets. The public
  graphics contracts and EasyGL renderer implementation are unchanged; the touched runtime and
  platform contract headers contain documentation-only changes relevant to this game.
- Advanced `dependencies.lock` and regenerated only island-simulator's reproducible project-owned
  CNA export at `39e84add777730969f58074fa1ffa010dc5c7d2c`. CNA and Sharp Runtime remained read-only;
  Sharp Runtime stays locked at `21c06910f7cf887cceebb45200e07b43dd45fef4`.
- Reconfigured and fully rebuilt the Linux/OpenGL 3.3, Web/WebGL 2, and arm64 Android/OpenGL ES 3
  targets with at most two jobs per build and four aggregate compiler workers. Native validation
  passes 37/37 in 51.01 seconds, including 29 real OpenGL scenes and real X11 input. The complete
  Chrome 152/SwiftShader WebGL 2 harness passes all 22 lifecycle, deployment, renderer, scene, and
  input stages. One immediately preceding run missed several synthetic key/mouse edges while all
  21 non-input stages passed; an isolated repeat passed, so this is retained as harness-timing
  evidence rather than misclassified as a framework defect.
- The Android debug APK assembles and application lint passes. It is 33,582,192 bytes; its stripped
  arm64 library is 15,849,816 bytes, and WebAssembly is 8,435,054 bytes. This does not upgrade the
  existing Android claim: the software AVD has provided rendering evidence, but touch, audio-focus,
  frame pacing, and thermals still require a physical device or stable KVM-accelerated emulator.
- The deterministic world report remains valid with fingerprint `d83ecd714e10fc9f`. The CNA XNB
  content-writer warnings observed on Android and Web are outside island-simulator's runtime asset
  path and predate this update; they do not materially affect the game and are not a `BUGS.md`
  entry. No confirmed CNA or Sharp Runtime defect was found during the advancement.

## Session-end checklist

- [ ] Keep source and docs coherent and in English.
- [ ] Build/test the relevant targets within the CPU policy when practical.
- [ ] Update task statuses, new discoveries, unresolved bugs, and exact next work here.
- [ ] Update `BUGS.md` for confirmed dependency defects; do not patch dependencies.
- [ ] Do not claim Web or Android runtime support from compilation alone.
- [ ] Inspect `git status` and `git diff --check`; stage only intended files.
- [ ] Measure project-owned disk use and remove only clearly obsolete owned artifacts.
- [ ] Record build directories used and maximum job count in the handoff/final report.
