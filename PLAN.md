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

## Verified workspace baseline (2026-10-05)

- [x] Confirmed `island-simulator` is a new repository on unborn branch `develop`, with no source
  files or commits at campaign start.
- [x] Read the workspace build policy in `../AGENTS.md`.
- [x] Located CNA at `../cna` and inspected its repository instructions and current APIs.
- [x] Identified CNA `next` at commit `b0e97bb1bb87` (`docs: point release links at libcna`,
  2026-10-04).
- [x] Confirmed the main `../cna` worktree is on `samples` at `504bf0a06fad`, not `next`.
  `samples` contains `next` as an ancestor, but it is not accepted as the dependency identity.
- [x] Confirmed there is no existing CNA `next` worktree in `git worktree list`.
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
- [x] Confirmed Emscripten is not currently on `PATH`; CNA documentation says the SDK may exist at
  `~/emsdk` and must be checked before declaring Web work blocked.
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
- [ ] Inspect CNA `next` Android activity/entry-point requirements and the template's generated
  package boundary in detail.
- [x] Confirm an existing Emscripten SDK at `~/emsdk`; browser inventory and test tooling still
  need inspection before the Web build.
- [~] Confirm an Android SDK at `~/Android/Sdk` and `adb`; sandbox socket policy prevented the
  initial device query, so NDK/Gradle/API/device inventory remains open.
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
- [~] Add startup/render statistics and fatal exception diagnostics; structured subsystem logging
  and asset-specific failure context remain open.
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
- [~] Add broad-phase spatial partitioning for rocks/trees and capsule-vs-world collision. Palm
  trunks now use shared deterministic circular proxies with substepped collision and axis sliding;
  a spatial broad phase, depenetration, and future rock proxies remain open.
- [~] Add footprint-aware ground support, ground following, a 38-degree uphill limit, axis sliding,
  bounded step rejection, and palm-trunk collision. Rock collision, spatial broad phase, and
  depenetration remain open.
- [x] Prevent terrain tunnelling under low frame rates with a 100 ms time clamp and 0.14 m maximum
  horizontal substeps; keep swept prop tests open with prop collision.

### First-person controller

- [~] Implement keyboard movement and mouse look with pitch limits. Runtime configuration remains
  open.
- [~] Capture/release relative mouse input on Linux. Focus, Web, and mobile differences remain open.
- [~] Establish initial eye height, acceleration, deceleration, walking speed, and a 0.34 m capsule
  footprint. Full capsule-vs-prop collision and evidence-based tuning remain open.
- [ ] Add optional mild head motion only if it improves presence and can be disabled.
- [ ] Keep jumping absent unless traversal testing demonstrates a clear need.

### Swimming and ocean boundary

- [x] Define depth-based water-entry/exit hysteresis.
- [~] Implement near-surface camera height, reduced acceleration, swimming speed, and drag. Visual
  transition/bob remains open.
- [x] Define accessible shoreline distance from the authoritative island field, not world boxes.
- [~] Add progressive outward resistance and inward correction from 42 to 72 m offshore. Lateral
  current variation and audiovisual feedback remain open.
- [ ] Add subtle audiovisual warning before the hard safety boundary.
- [x] Add a tested 92 m last-resort radial safety cap that removes outward velocity.

### Tests and tuning

- [~] Unit-test terrain scale/determinism, shore distance, normals/slopes/surfaces, footprint ground
  support, movement integration, water hysteresis, long-frame substeps, and boundary forces.
- [ ] Add deterministic traversal scenarios for coves, rocks, steep banks, and shore entry/exit.
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
- [ ] Add sand, wet-sand, soil, grass/jungle-floor, and rock material transitions.
- [ ] Break the island silhouette with rocks, palms, shallow banks, and small offshore features.
- [ ] Keep routes traversable and provide alternating enclosed/revealing views.

### Ocean and shoreline

- [x] Build a bounded 25,921-vertex animated ocean grid large enough to hide the finite world
  boundary while retaining portable 16-bit indices.
- [x] Implement explicit GLSL 330 / GLSL ES 300 vertex wave motion and analytic layered
  normal/specular response through CNA's portable `ShaderEffect` package API.
- [x] Derive shallow/deep tropical colour and transparency from authoritative terrain depth and
  shore information baked into the ocean vertices.
- [~] Blend shoreline foam/readable moving wave edge and translucent water over the beach without
  visible z-fighting. Wet-sand material and close first-person tuning remain open.
- [ ] Add underwater/near-surface tint, fog, and refraction-like cues only when portable and stable.
- [~] Provide an actionable stock `BasicEffect` fallback when portable custom effects are
  unavailable or fail compilation. WebGL 2 uses the full shader successfully; GLES 3 remains
  untested and therefore unqualified.

### Vegetation and props

- [~] Build multiple procedural palm variants with different height, lean, curvature, crown
  rotation, frond arrangement, scale, and clustering. Four curved tapered trunk/ribbon-frond
  variants, 330 varied instances, near/far meshes, root contact wedges, and portable shader wind
  now run on native OpenGL 3.3 and WebGL 2; richer close-up materials, pop-free LOD blending, and
  Android GLES 3 qualification remain open.
- [ ] Add broadleaf canopy, medium plants, bushes, ferns, ground plants, rocks, fallen wood, and
  sparse natural beach props.
- [~] Keep placement biome-aware, slope-aware, collision-aware, clustered, and reproducible. Palm
  placement is deterministic, shore-distance/slope/height constrained, cluster-noise weighted,
  and reserves a winding primary path, two clearings, and spawn; final biome masks and prop
  separation remain open.
- [~] Add subtle coherent breeze phases without storm-scale motion. Palm fronds and grass use
  bounded two-frequency motion with deterministic phase variation on OpenGL 3.3 and WebGL 2;
  broadleaf vegetation and Android GLES 3 validation remain open.
- [~] Ensure collision is applied only where visually necessary; avoid a forest of snag hazards.
  One radius-matched circular proxy is active per palm trunk while fronds remain non-colliding;
  broader play-tuning is still required.

### Grass quality campaign

- [~] Prototype clustered mesh blades and evaluate first-person silhouette/overdraw on all backends.
  A 28-blade near mesh and five-blade far mesh now run on OpenGL 3.3 and actual WebGL 2; GLES 3,
  measured overdraw, and further close-up shaping remain open.
- [~] Use density, colour, height, lean, and phase variation tied to terrain/biome data. The first
  18,556-patch layout varies all five deterministically from terrain constraints and smooth cluster
  noise; final biome masks remain open.
- [x] Align clusters to authoritative terrain normals without flattening individual blade growth.
- [~] Add vertex wind with bounded amplitude and coherent gust-free sunny-day motion. The portable
  two-frequency wind path is verified on OpenGL 3.3 and WebGL 2; GLES 3 remains unqualified.
- [~] Implement near/middle/far density and mesh LOD with dither/fade or masked transitions. Near
  and far meshes plus ground-colour distance merging exist; a middle tier and pop-free transition
  still need visual/performance evidence.
- [~] Cull cells by frustum/distance and batch or instance where CNA's verified path permits. A
  256-cell grid and one hardware-instanced draw per visible cell are verified on native and WebGL 2;
  Android remains open.
- [ ] Test worst-case first-person grass views for aliasing, transparency, overdraw, and frame time.

### Lighting, sky, distance, and shadows

- [ ] Establish one fixed attractive sun direction, colour temperature, fill light, and exposure.
- [ ] Build a portable sky gradient/dome with sun disc and high, calm tropical cloud suggestion.
- [ ] Add distance haze/fog that enlarges perceived scale without washing out nearby jungle.
- [ ] Evaluate one bounded directional shadow solution or baked/vertex alternatives on all targets.
- [~] Provide contact/grounding cues for palms, rocks, props, and animals even when shadows degrade.
  Near-tier palms have procedural root wedges; shadow/ambient-occlusion alternatives and all other
  object classes remain open.
- [ ] Add distant low-cost scenery and vegetation cards/meshes without exposing the world edge.

Exit validation:

- Representative beach, jungle, clearing, hill, cove, and offshore views look intentionally
  composed from first-person height.
- Grass and palms do not read as uniform stamps.
- Linux frame timing is instrumented before density is increased further.

## Phase 4 — Wildlife and animation (42 h)

Dependencies: terrain/water/spawn queries, culling, and content/animation pipeline.

### Reusable wildlife framework

- [ ] Implement spawn regions, deterministic variation, simple steering, state timers, activation
  range, simulation LOD, animation state, and optional audio emitter.
- [ ] Validate positions against land/water/depth/slope constraints before and during movement.
- [ ] Add debug visualization for regions, paths, state, and activation tiers.
- [ ] Unit-test state transitions, bounds, spawn validity, avoidance, and deterministic seeds.

### Birds

- [ ] Add multiple path families above shore/interior with altitude, speed, turn, and group variation.
- [ ] Avoid short obvious loops and synchronized animation.
- [ ] Add occasional near and distant appearances with bounded active counts.
- [ ] Add sparse positional calls without attaching a sound to every visible bird.

### Fish and shark

- [ ] Add shallow-water fish schools with depth bands, body motion, separation/cohesion, and land
  avoidance based on authoritative water queries.
- [ ] Activate/cull schools spatially; never simulate large invisible populations.
- [ ] Add one occasional offshore shark with credible silhouette, patrol variation, swimming
  animation, and no hostile behaviour.
- [ ] Keep the shark out of immediate beach shallows and tune rarity for atmosphere, not tension.

### Monkeys and skeletal animation

- [ ] Select or create a legally redistributable monkey/substitute model and record provenance.
- [ ] Validate glTF skeleton, clips, scale, axes, material fallback, and all three renderers.
- [ ] Integrate CNA skeletal animation without framework modification.
- [ ] Implement compact `Idle`, `Observe`, `Wander`, `Walk`, and `RunAway` states.
- [ ] Blend or transition clips without visible snapping; synchronize movement with animation speed.
- [ ] Constrain monkeys to valid land/terrain regions and avoid water, steep slopes, and props.
- [ ] Add mild player awareness without enemies, attacks, quests, or interaction systems.
- [ ] Distance-throttle animation and decisions while preserving nearby quality.

Exit validation:

- All four wildlife categories appear naturally during an ordinary island circuit.
- No frequent land/water violations, obvious repeated loops, animation explosions, or dead crowds.
- Monkeys remain environmental life, not a gameplay objective or threat.

## Phase 5 — Atmosphere and audio (24 h)

Dependencies: stable zones, wildlife emitters, and representative locations.

- [ ] Confirm CNA audio format, streaming, lifetime, listener/emitter, and platform constraints.
- [ ] Build a layered mixer/soundscape controller with beach, ocean, jungle, clearing, and water
  zones and smooth hysteretic crossfades.
- [ ] Add non-repetitive surf beds and occasional stronger waves tied to shoreline proximity.
- [ ] Add wind-through-vegetation, jungle beds, birds, insects where tasteful, monkey calls, distant
  wildlife, and sparse water splashes.
- [ ] Add positional one-shots/emitters where direction and distance improve presence.
- [ ] Add player wading/swimming movement audio without repetitive rapid triggering.
- [ ] Bound simultaneously active sources and virtualize/throttle inaudible emitters.
- [ ] Normalize source material, remove clicks at loops, and test mono/stereo spatial assumptions.
- [ ] Provide graceful no-audio diagnostics/fallback without crashing the visual experience.
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
- [~] Verify stock `BasicEffect` terrain fallback and explicit GLSL ES 300 ocean/palm/grass shaders
  through actual WebGL 2 browser compilation and execution. The current 330-palm and 18,556-patch
  grass hardware-instancing paths, including both wind shaders and palm/grass LOD selection, are
  verified; later broadleaf, wildlife, and material variants remain open.
- [ ] Implement pointer-lock acquisition/release and clear click-to-focus/fullscreen instructions.
- [ ] Verify keyboard, mouse, focus loss, resize, audio unlock, and browser pause/resume behaviour.
- [ ] Package assets into the virtual filesystem with useful missing-asset diagnostics.
- [ ] Reduce download/startup size through mesh/texture/audio choices and measured compression.
- [ ] Add quality defaults appropriate to browser GPU/memory limits.
- [~] Run the current terrain/ocean/palm scene in Chromium/SwiftShader with an active-render capture; a
  visible hardware-accelerated interactive pass and another browser engine remain open.
- [~] Capture browser console output, `WEBGL2` renderer identity, clean lifecycle, and an
  active-render screenshot. Startup time, memory, and representative frame-time evidence remain.
- [ ] Document browser requirements and verified versions without claiming untested browsers.

Exit validation:

- A user can load, enter pointer lock, walk, swim, observe wildlife, hear audio, and recover focus
  in an actual browser.
- The environment remains recognizably the same game, with documented quality differences only.

## Phase 7 — Android / OpenGL ES 3 (28 h)

Dependencies: stable input abstraction, scalable UI/debug display, mobile quality tiers.

- [ ] Inventory Android SDK, NDK, Gradle, Java, device/emulator, and supported ABI state.
- [ ] Adapt the CNA template Android shell with island-specific package/application identities.
- [ ] Add an `OPENGLES3` Android configuration and reproducible packaging instructions.
- [ ] Implement dual-zone touch controls: movement stick, look region, and minimal unobtrusive UI.
- [ ] Handle pointer IDs, safe areas, orientation, resize, pause/resume, and touch cancellation.
- [ ] Add configurable sensitivity and left/right layout considerations where practical.
- [ ] Verify audio focus/lifecycle and resource restoration after backgrounding.
- [ ] Add mobile quality defaults for grass, shadows, wildlife distance, water, textures, and audio.
- [ ] Profile CPU/GPU frame time, memory, thermal behaviour, and loading on a real device when
  available; use emulator evidence only for claims it can support.
- [ ] Test install, first launch, relaunch, background/foreground, rotation policy, and clean exit.
- [ ] Document device/API/ABI evidence and all unverified hardware classes.

Exit validation:

- On an actual device or explicitly identified emulator, touch traversal and swimming are usable,
  lifecycle transitions do not corrupt rendering/audio, and the complete environment is present.

## Phase 8 — Performance, diagnostics, and robustness (28 h)

Dependencies: representative complete scene on all targets.

### Instrumentation and budgets

- [~] Add debug overlay: FPS, frame time, player coordinates/state, renderer/backend, visible
  terrain/vegetation cells, draw counts if available, wildlife active/simulated counts, and audio
  source counts. The asset-free F1 overlay now covers all existing terrain/player/water/vegetation
  fields on OpenGL 3.3 and WebGL 2; future wildlife and audio fields remain dependent on those
  systems.
- [ ] Add toggles for collision, shoreline/current field, vegetation cells, spawn regions, wildlife
  states, LOD, and culling; keep them disabled in normal release presentation.
- [ ] Add debug teleport locations and a debug-only free camera.
- [ ] Establish measured frame-time, draw-call, triangle, texture, mesh, audio, and startup budgets
  per target rather than one desktop-only target.

### Optimization

- [ ] Profile representative beach, dense jungle, hill vista, waterline, and wildlife scenes.
- [~] Implement terrain/vegetation cell frustum and distance culling. Terrain and palms share a
  64-cell partition; grass uses 256 finer cells. All are frustum culled, palms/grass have distance
  cutoffs, and grass selects near/far geometry by cell distance. Per-instance wildlife policy and
  pop-free vegetation transitions remain open.
- [~] Tune static batching/instancing and verify it does not increase memory or Web/mobile stalls.
  CNA hardware instancing reduces the 330-palm overview to eight near/far variant draws on native
  OpenGL 3.3 and actual WebGL 2; GLES 3 and dense grass still require validation.
- [~] Add mesh/material/animation LOD only where profiles show a worthwhile return. Palms select
  reduced trunk/frond geometry by cell distance and grass selects 28-blade/five-blade clusters;
  measured budgets and pop-free transitions remain open.
- [ ] Throttle distant wildlife and audio updates without visible popping.
- [ ] Reduce transparency/overdraw hotspots, especially grass and layered shoreline effects.
- [ ] Audit texture formats/sizes/mipmaps and create smaller variants only when justified.
- [ ] Measure loading and remove redundant runtime generation/conversion.

### Reliability and automated coverage

- [ ] Test malformed/missing terrain, configuration, model, texture, animation, shader, and audio
  inputs for actionable failure diagnostics.
- [ ] Add assertions for finite transforms, valid indices, terrain query domains, animation palette
  sizes, and impossible player/wildlife states in development builds.
- [ ] Run ASan and UBSan selectively on first-party core/tests; do not build unused sanitizer sets.
- [x] Add deterministic smoke mode plus fixed overview and first-person beach-path camera/capture
  modes for renderer evidence.
- [ ] Rebuild from the documented clean state to detect undeclared files and absolute paths.
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
- [ ] Improve grass close-up shape, density transitions, colour variation, wind coherence, and
  aliasing until it holds up at first-person height.
- [ ] Break repeated palm/prop silhouettes and correct floating/sunken placements.
- [ ] Refine shoreline contours, wet edge, shallow colour, foam, wave scale, and water/terrain seams.
- [ ] Refine jungle density versus traversability and strengthen landmarks/clearings/navigation cues.
- [ ] Refine fixed lighting, shadow/contact cues, sky, haze, exposure, and distant scenery as one
  coherent permanent sunny art direction.
- [ ] Improve bird/fish/shark/monkey timing, animation transitions, path variation, and clipping.
- [ ] Improve soundscape variation, spatial placement, transitions, balance, and long-session fatigue.
- [ ] Fix frame pacing, loading hitches, input focus issues, touch ergonomics, and platform-specific
  rendering differences found during real play.

### Release qualification

- [ ] Run Linux release build and multiple fresh-user smoke sessions.
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
| Player | Integration, collision, water transition tests | Feel/tuning sessions at varied frame rates |
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
  frame spacing, player/shore/current state, renderer, terrain/vegetation visibility, LOD counts,
  and draws; later wildlife/audio counters and warning budgets remain open.
- [ ] Deterministic seed display/copy.
- [ ] Teleports for beach, cove, jungle, clearing, hill, deep-water limit, fish, and monkeys.
- [ ] Terrain/capsule/contact visualization.
- [ ] Shore distance/current vector visualization.
- [ ] Vegetation cell, culling, LOD, and density visualization.
- [ ] Wildlife spawn region, state, steering, and simulation-tier visualization.
- [ ] Audio zone/emitter/source-count visualization.
- [~] Debug-only free camera and fixed screenshot viewpoints. Command-line overview and
  first-person beach-path views are implemented and used by native/browser captures; free camera
  and more landmark views remain open.
- [ ] Asset validation command and deterministic world-statistics report.
- [ ] Frame-time capture and representative scene benchmark route.

## Documentation deliverables

- [~] `README.md`: concept, controls, quick start, and honest status exist; representative
  screenshots and release-facing presentation remain.
- [x] `PLAN.md`: this living ledger.
- [x] `BUGS.md`: defect schema and current no-confirmed-defects record exist; update whenever
  evidence qualifies a dependency defect.
- [~] `ASSET_LICENSES.md`: original procedural baseline is recorded; it remains a gate and must be
  extended before any external asset enters a runtime build.
- [x] `docs/build-linux.md`: prerequisites, exact dependencies, configure/build/run/test.
- [x] `docs/build-web.md`: SDK, package, serve, browser requirements, validation, and explicit
  unverified interaction/performance scope.
- [ ] `docs/build-android.md`: SDK/NDK/Gradle, ABI, install/run/logging, validation.
- [~] `docs/architecture.md`: current subsystem boundaries, ownership, update/render flow, and
  coordinates exist; extend as systems are added.
- [~] `docs/controls.md`: current desktop controls are accurate; touch and configuration notes
  remain dependent on their implementations.
- [ ] `docs/asset-pipeline.md`: source formats, generation/conversion, manifests, validation.
- [ ] `docs/renderer-differences.md`: measured visual/performance differences and fallbacks.
- [~] `docs/debugging.md`: overlay fields and deterministic capture workflows are documented;
  teleports, world-space visualizers, performance capture, and common-failure guidance remain open.
- [ ] `docs/performance.md`: budgets, representative scenes, measurements, decisions.
- [ ] `docs/known-limitations.md`: only current, verified limitations.

## Current execution queue

Work next, in order:

1. Validate Web pointer lock, keyboard focus, resizing, pause/resume, and active hardware rendering
   before declaring the browser traversal experience usable.
2. Audit Android NDK/Gradle/device availability and establish the first GLES 3 package shell before
   designing touch input around assumptions.
3. Begin the broadleaf canopy/medium-plant layer and preserve the authored route/clearing structure.
4. Add debug teleports and shoreline/current plus vegetation-cell visualization before traversal
   and density tuning expands.
5. Revisit palm silhouette/material richness and pop-free LOD transition after measured frame-time
   instrumentation exists; do not raise geometry cost blindly.

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

## Session-end checklist

- [ ] Keep source and docs coherent and in English.
- [ ] Build/test the relevant targets within the CPU policy when practical.
- [ ] Update task statuses, new discoveries, unresolved bugs, and exact next work here.
- [ ] Update `BUGS.md` for confirmed dependency defects; do not patch dependencies.
- [ ] Do not claim Web or Android runtime support from compilation alone.
- [ ] Inspect `git status` and `git diff --check`; stage only intended files.
- [ ] Measure project-owned disk use and remove only clearly obsolete owned artifacts.
- [ ] Record build directories used and maximum job count in the handoff/final report.
