# island-simulator Development Plan

## Document purpose

This is the living execution ledger for the original roughly 300-hour development campaign plus a
user-directed visual-realism extension of up to roughly 100 additional productive hours. It is not
a feature wish list: checked items require the validation named by their phase, and platform claims
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

## Verified workspace baseline (refreshed 2026-10-07)

- [x] Confirmed `island-simulator` is a new repository on unborn branch `develop`, with no source
  files or commits at campaign start.
- [x] Read the workspace build policy in `../AGENTS.md`.
- [x] Located CNA at `../cna` and inspected its repository instructions and current APIs.
- [x] Qualified and locked CNA `next` at commit `97783d1e090c` (`fix(Task RRC-029): name the
  renderer the benchmark ran, not the compile-time default`, 2026-10-07). The preceding qualified
  exports were `9fbe34468c69` and `fe20e3e9c23b`; the campaign started from `b0e97bb1bb87`.
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
  device is connected. The host loads `kvm`/`kvm_amd`; `/dev/kvm` is present and accessible to the
  configured `kvm` group, and Emulator 36.5.11 confirms KVM version 12 is usable.
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
- [x] Keep runtime content independent of the launch directory and absolute developer paths. All
  current runtime shaders, meshes, textures, and audio are embedded or generated, so no content
  root is required. `IslandGameRelocatableWorkingDirectory` launches the actual game from an
  otherwise empty build-owned directory and guards that contract without adding a dormant content
  locator.
- [x] Add a core test executable that avoids graphics initialization.

### Foundational code quality

- [x] Establish namespace, include, ownership, units, coordinate, handedness, and naming rules.
  `docs/architecture.md` now records the existing `island`/anonymous-namespace boundary, include
  order, RAII ownership policy, metre/second/radian units, right-handed +X/+Y/+Z world basis,
  transform order, identifier/file language rules, and the threshold for adding math helpers.
- [x] Add deterministic noise/terrain helpers with repeatability and scale tests.
- [x] Add small math/geometry helpers only where CNA's existing types do not already suffice.
  The core keeps CNA/XNA vector, matrix, frustum, colour, and bounding primitives; project helpers
  are limited to tested island-domain rules such as deterministic terrain/noise queries, aquatic
  lighting, and procedural triangle-mesh validation rather than a parallel math layer.
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
  exact narrow-phase checks retain substepped axis sliding. A bounded one-time full-set pass safely
  recovers explicit placements from overlapping static proxies; dynamic obstacles and continuous
  depenetration remain open.
- [~] Add footprint-aware ground support, ground following, a 38-degree uphill limit, axis sliding,
  bounded step rejection, and palm-trunk collision. Solid land-rock, reef-outcrop, and log
  collision is active while walking; the same horizontal proxies constrain swimming.
  Static placement/teleport depenetration is implemented; continuous dynamic-obstacle recovery and
  geometric step-up remain open.
- [x] Prevent terrain tunnelling under low frame rates with a 100 ms time clamp and 0.14 m maximum
  horizontal substeps; keep swept prop tests open with prop collision.

### First-person controller

- [~] Implement keyboard movement and mouse look with pitch limits. WASD/arrows, correct
  screen-relative strafing, Shift-toggled 4.3/8.0 m/s walk/run, and simultaneous forward/mouse turns
  are qualified on native Linux and Chromium. A 1.4-second normal-play notice now confirms each
  retained keyboard mode without becoming a permanent HUD. Walk/run, camera-motion, audio-mute,
  and touch-look preferences now persist portably; a general settings UI and remapping remain open.
- [x] Capture/release relative mouse input on Linux. Click and first-movement-key acquisition avoid
  pre-capture absolute cursor deltas; one Escape edge releases and a second exits. A real isolated
  SDL/XTest regression covers the complete sequence.
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
  depth-responsive near-surface haze/edge veil with restrained surface scatter, movement-driven
  surface ripples, and hysteretic speed-scaled entry/exit bursts are complete; a true geometric
  waterline and full underwater presentation remain open.
- [x] Define accessible shoreline distance from the authoritative island field, not world boxes.
- [x] Add progressive outward resistance and inward correction from 42 to 72 m offshore. Velocity
  now eases continuously from zero into a dominant shoreward return with restrained deterministic
  coast-parallel circulation; the water-view veil and undertow audio strengthen across the same
  normalized field. More complex cove-, rock-, or wave-responsive turbulence is intentionally
  outside the current boundary model.
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

- [~] Shape multiple coves, beach widths, low ridges, viewpoints, jungle pockets, clearings, and
  readable paths without an implausible mountain. The deterministic shoreline spans 209.7--252.1 m
  in radius and the first authored three-silhouette driftwood arc gives the `-2.35` radian cove a
  recognizable non-solid landmark; broader whole-route composition remains open.
- [~] Create biome/density masks with authored overrides and deterministic placement seeds. The
  authoritative shore distance, slope, height, and independent multi-scale cluster fields already
  form coast/interior density masks for palms, four jungle layers, grass, and each prop family.
  The shared primary corridor, two clearing rooms, spawn space, clearing landmarks, and cove
  driftwood provide authored overrides; fixed seeds and population/placement tests lock the result.
  A separately stored mask asset is not justified for this procedural source, while broader
  authored biome pockets and whole-route composition tuning remain open.
- [~] Add sand, wet-sand, soil, grass/jungle-floor, and rock material transitions. Terrain vertices
  now carry authoritative shore distance and slope into a portable texture-free material shader;
  two-scale value noise and fine breakup vary sand/interior soil, the wet edge darkens coherently
  and adds a bounded view-dependent fixed-sun sheen,
  a close-only octave adds dry-sand granulation, and a bounded signed-shore-distance term adds
  broken raked crests parallel to the water edge. Two soft-edged deterministic close-only grids
  add sparse dark pebbles and pale shell flecks to otherwise exposed dry sand without geometry or
  texture samples. Steeper interior ground exposes a muted soil
  tint, and a distance-bounded microvariation octave plus two sparse deterministic grids add
  curved pointed leaf litter to the interior. A restrained
  fixed-sun dapple reuses those noise fields on low-slope ground beyond the beach transition. The
  same portable shader now draws a narrower, noise-mottled broken-edge dry-earth footpath from the
  authoritative vegetation-reserved corridor, with short grass retained over it and the wider
  walking reservation left unchanged. The two shared authored clearing
  definitions now drive distinct warm-dry and darker leaf-rich surface treatments. Five authored
  rocks and one fallen log in each clearing add a natural landmark composition around their outer
  rings while retaining open centres. Authored biome masks, richer landmarks, decals, broader litter diversity,
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
- [x] Compose one 20 m long, 5.2 m beam wooden sailing-ship wreck in a chosen
  beach shallows. The landmark must read as a large ship rather than a boat: retain a recognizable
  broken hull, exposed frames, partial deck, grounded heel, and snapped mast, with a separately
  composed debris trail of planks and substantial fragments leading toward the beach. Keep the
  player clear of sharp snag traps, use only bounded solid proxies for major hull sections, and
  validate placement against the authoritative seabed/shore field. The implemented original
  wreck has a central break, cabin, bowsprit, rigging, six major-hull collision proxies, and a
  12-piece beachward debris trail; the smaller debris remains intentionally non-solid.
- [x] Give the shipwreck deterministic layout data, a portable culled renderer, fixed-sun material
  response, wet/submerged colour treatment, a dedicated first-person QA viewpoint, F1/F2 counts,
  tests for dimensions/depth/debris bounds/collision, and matched OpenGL 3.3, WebGL 2, and GLES 3
  runtime evidence. The 3,948-triangle one-draw batch is distance/frustum culled, provenance is
  manifest-gated, and native, Chrome, Firefox, and accelerated-Android captures were inspected.

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
  the wet-sand pass. A subtle generated near-surface depth haze, upper-surface scatter, and edge
  veil now fade with swimming state and support depth, while depth/speed-responsive player ripples
  and one short broken-ring burst on a moving wet/dry crossing reuse the animated ocean submission.
  The ocean now also reuses its existing two crossing-wave values for shallow-weighted subsurface
  colour structure that fades before the horizon. A restrained ridge-light cue reuses the
  already-computed five-wave analytic slope and the same distance/Fresnel/shallow bounds without a
  new sine, varying, texture, geometry, or draw. A narrow sun-facing window of that same slope now
  adds short directional crests from 85 to 245 m without another wave field or resource. A
  view-aligned signed projection of that slope now also adds shallow-weighted grazing reflection
  bands inside the existing 135--420 m detail fade without a new wave, varying, texture, resource,
  or draw; full underwater transitions and final
  wash/material tuning remain open.
- [~] Add underwater/near-surface tint, fog, and refraction-like cues only when portable and
  stable. The first tint is a renderer-portable, premultiplied 160 by 90 generated depth/edge veil
  with restrained upper-surface scatter and one conditional `SpriteBatch` draw; waterline
  geometry, distance fog, and refraction remain.
- [~] Provide an actionable stock `BasicEffect` fallback when portable custom effects are
  unavailable or fail compilation. WebGL 2 and Android GLES 3 use the full shader successfully,
  including inspected fixed water captures with an active transition burst. The fallback
  intentionally omits shader-only ripple/burst detail.

### Vegetation and props

- [~] Build multiple procedural palm variants with different height, lean, curvature, crown
  rotation, frond arrangement, scale, and clustering. Four curved tapered trunk variants, 330
  varied instances, fuller segmented near leaflets, widened-ribbon far crowns, irregular faceted
  near-root wedges, staggered per-instance near/far thresholds, near-only procedural leaf scars,
  two restrained coconut-bearing variants, transform-derived green-only per-instance frond tint,
  and portable shader wind now run on native OpenGL 3.3 and WebGL 2;
  broader bark/fruit variation, true LOD blending, and instrumented Android wind/LOD qualification
  remain open despite the first visible GLES3 scene.
- [~] Add broadleaf canopy, medium plants, bushes, ferns, ground plants, rocks, fallen wood, and
  sparse natural beach props. The first layer now provides 281 four-variant canopy trees and 1,822
  four-variant fern/large-leaf plants, 565 three-variant low bushes, 2,225 three-variant
  forest-floor plants, 157 land rocks, 14 emergent reef outcrops, 13 fallen logs, 31 three-variant
  driftwood pieces including a three-piece authored cove arc, 63 grove-associated fallen palm
  fronds, and 265 four-variant shell/pebble/dry-wrack
  clusters. The fourth beach-detail variant now uses low curved dry-wrack strands instead of
  repeating the other three shell/pebble compositions without changing population or draw budget.
  Near canopy meshes now add tapered support branches, irregular separated lobes,
  pointed edge leaves, four/five short irregular buttress roots, and distance-faded procedural
  bark grain/fissures without a texture or draw. Near bushes now use smaller
  lobes with paired folded pointed leaves while retaining
  their compact far tier. Near forest-floor variants now include six upright broad leaves with
  visible petioles and a separate five-leaf folded split form while retaining compact far
  tiers. One existing near ground-plant variant also carries three sparse folded fallen leaves,
  reusing its instance, buffer, culling, and draw submission instead of adding a litter batch.
  All four jungle layers now enforce range and near/far LOD per instance after conservative cell
  rejection. The shared portable shader now derives
  two-sided geometric facet light from the fixed sun for every jungle tier without another vertex
  stream or draw. It also derives a stable green-only tint from each existing instance transform,
  reducing repeated foliage colour while preserving bark and near/far LOD identity without a new
  instance field, buffer, texture, or draw. Near rocks now break their former flat crown line
  through bounded positions on the existing rings/cap and derive matching fixed-sun facet light
  without another normal stream, vertex, triangle, or draw. Their neutral stone-family colours
  now also receive restrained warm/cool per-instance tint derived from existing scale/yaw
  transform coefficients, preserving near/far LOD identity without another attribute, buffer,
  texture, geometry, upload, or draw. Near fallen logs now reuse their
  existing end-ring/cap vertices for irregular non-planar broken contours while retaining topology,
  collision, far LOD, and draw count; further floor-plant/bush material
  diversity and broader natural beach-prop variety remain.
- [~] Keep placement biome-aware, slope-aware, collision-aware, clustered, and reproducible. Palm,
  canopy, understory, bush, forest-floor, rock, log, driftwood, fallen-frond, and beach-detail
  placement is deterministic, shore-distance/slope/height constrained, multi-scale cluster-noise
  weighted, and reserves a winding primary path, two clearings, and spawn; final biome masks and
  prop separation remain open.
- [~] Add subtle coherent breeze phases without storm-scale motion. Palm fronds, grass, canopy
  crowns, understory leaves, low bushes, and forest-floor plants use bounded weighted motion on
  OpenGL 3.3 and WebGL 2; the current transform-tint-expanded shader also compiles and renders in a
  KVM/gfxstream GLES 3 jungle launch. Physical-device wind appearance and cost remain open.
- [~] Ensure collision is applied only where visually necessary; avoid a forest of snag hazards.
  One radius-matched circular proxy is active per palm and canopy trunk; substantial land rocks,
  emergent reef outcrops, and fallen logs are solid, while fronds, crowns, understory, ground
  plants, small rocks, and driftwood remain non-colliding. The same proxies now constrain walking
  and swimming. Broader play-tuning remains open; the shared static-proxy spatial broad phase is
  active.

### Grass quality campaign

- [~] Prototype clustered mesh blades and evaluate first-person silhouette/overdraw on all backends.
  A 42-blade, five-control-level near mesh and five-blade, three-level far mesh now run on OpenGL
  3.3, actual WebGL 2, and KVM/gfxstream GLES 3. Near blades have folded side colour, two-sided
  fixed-sun lighting, root darkening, restrained transmission, seven shorter juvenile blades, and
  per-instance blade stature/colour/wind variation. Six mature near blades now carry restrained
  seed-bearing stems that share the existing opaque instanced wind path. Measured physical-mobile
  overdraw remains.
- [x] Use density, colour, height, lean, and phase variation tied to terrain/biome data. The
  18,556-patch layout varies all five deterministically from terrain constraints and smooth cluster
  noise. A coherent cool-to-warm material field now differentiates coast, interior, short route
  turf, the sunny east clearing, and the leaf-rich west clearing without changing population or
  draw count; core tests lock its range, determinism, and clearing separation. Sparse deterministic
  blades also carry a height-weighted sun-cured tip in the shared mesh vertex colour, adding local
  age variation without another attribute, texture, instance, or draw. The same existing instance
  value now varies individual blade stature, colour, and wind amplitude inside each shared cluster.
- [x] Align clusters to authoritative terrain normals without flattening individual blade growth.
- [~] Add vertex wind with bounded amplitude and coherent gust-free sunny-day motion. The portable
  two-frequency wind path is verified on OpenGL 3.3, WebGL 2, and a current KVM/gfxstream GLES 3
  close-grass capture; physical-mobile behaviour remains unqualified.
- [x] Add bounded player interaction without a CPU per-patch update. Nearby blade tips now bend
  away from a 0.55 m stationary body radius, smoothly expand to 1.35 m at walking speed, and ease
  back after movement inside the existing portable instanced vertex shader.
- [~] Implement near/middle/far density and mesh LOD with dither/fade or masked transitions. Near
  (42-blade), middle (10-blade), and far (five-blade) meshes plus ground-colour distance merging
  exist. Deterministic adjacent-tier smoothstep splits stagger individual replacements across
  62--78 m and 86--102 m instead of switching an entire 37.5 m cell or jumping directly between
  the extreme meshes. OpenGL 3.3 and WebGL 2 validate the same mixed-cell populations. Masked
  blending, longer traversal review, and physical-mobile evidence remain.
- [~] Cull cells by frustum/distance and batch or instance where CNA's verified path permits. A
  256-cell grid uses one hardware-instanced draw for a pure-LOD cell and two only while that cell
  crosses either progressive LOD band. The close QA view uses 21 draws for 15 cells, with six
  mixed; native and WebGL 2 agree. Android counters and sustained performance remain open.
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
  population, path-family mix, deterministic initialization/long-run motion, wing extremes,
  intermittent glide reachability, opposed bank directions, terrain clearance, route radius, and
  tier accounting are covered through a simulated 20-minute flight. Fish population/school
  distribution, water validity, deterministic motion, tail extremes, bounded depth pitch, and
  opposed turn banks,
  meaningful route travel, and tier accounting are covered through a simulated ten-minute swim.
  Shark deterministic motion, water/depth validity, presence/absence cycles, body animation,
  bounded dive pitch, visible turn banking,
  meaningful route travel, and tier accounting are covered through a simulated 20-minute patrol.
  Monkey population, deterministic long-run state/motion, finite pose, terrain/slope validity,
  idle/observe/walk/run-away reachability, bounded player-aware head posing in both directions,
  meaningful travel, and tier accounting are covered through a simulated 20-minute session. The
  same run samples clearance against the complete
  world obstacle set and requires deterministic steering deflections.

### Birds

- [x] Add multiple path families above shore/interior with altitude, speed, turn, and group variation.
  Ten shore, eight canopy, and six high agents use distinct radii, altitudes, speeds, directions,
  vertical phases, two-frequency steering variation, and slow individual radial drift.
- [x] Avoid short obvious loops and synchronized animation. Per-agent incommensurate steering,
  altitude, flap, radial-drift, and glide phases remove shared cycles. Smooth intermittent glides
  and route/steering-derived bank angles break up perpetual level flapping without increasing the
  population, geometry, or one-draw submission budget. A long deterministic regression guards
  bounded nondegenerate motion, while subjective whole-island observation remains a polish task.
- [x] Add occasional near and distant appearances with bounded active counts. The fixed 24-agent
  budget uses individual frustum/distance culling, near/far procedural meshes, distance-throttled
  updates, and one dynamic-buffer draw submission. The near mesh now uses the complete existing
  54-vertex allocation for 18 triangles, adding paired trailing-flight-feather surfaces behind the
  wings; the 12-vertex/4-triangle far silhouette, buffer capacity, and draw count remain unchanged.
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
  culling, near/far meshes, and one dynamic-buffer draw submission. Smoothed depth pitch and turn
  banking now drive the existing portable CPU pose; both LODs have forked tails and the near tier
  adds restrained eye/gill detail without another draw. CPU face colours give the body fixed-sun
  faceting plus one depth-dependent tropical-water tint and moving shallow sunlight sample per
  animal, while the existing cull-none batch lets thin fins use one surface; near/far geometry is
  reduced from 66/24 to 51/12 vertices per fish.
- [x] Add one occasional offshore shark with credible silhouette, patrol variation, swimming
  animation, and no hostile behaviour. A single original procedural model uses near/far geometry,
  CPU body/tail motion plus smoothed dive pitch and turn banking, frustum/distance culling, and one
  portable dynamic-buffer draw. Both LODs have a forked tail and the near tier adds eye/gill detail.
  Shared fixed-sun CPU facet lighting, restrained depth tint/moving shallow sunlight, and cull-none
  single-surface fins reduce near/far geometry from 66/30 to 51/15 vertices without another draw.
- [x] Keep the shark out of immediate beach shallows and tune rarity for atmosphere, not tension.
  It patrols 34--88 m offshore for roughly 42--66 seconds, then remains below the presentation
  layer for roughly 92--160 seconds before the next appearance.

### Additional peaceful wildlife

- [x] Add a small deterministic population of beach crabs that scuttle laterally across suitable
  dry/wet sand, pause, and retreat from a nearby player without entering deep water or the jungle.
  Thirty crabs occupy six bounded colonies, share pause/forage/scuttle behaviour and the existing
  full/reduced/dormant activation policy, and render as articulated near/far silhouettes in one
  culled dynamic batch with no per-animal audio voice.
- [x] Add butterflies around selected sunny clearings and jungle edges. Eighteen butterflies use
  three terrain-validated low-altitude habitats, asynchronous rest/flutter/nectar-hover timing,
  mild player lift/retreat, five palettes, and hinged patterned near wings. They share the
  full/reduced/dormant activation policy and render through one culled near/far dynamic batch.
- [x] Add a few non-hostile sea turtles to suitable near-shore water. Constrain them between seabed
  and surface, keep them out of land and the offshore-current limit, give them slow flipper/body
  animation and occasional surfacing, and avoid duplicating the fish/shark steering code where the
  shared wildlife activation policy is sufficient. Five agents now occupy separate authoritative
  7--38 m offshore routes, alternate cruise/forage/surface states, and render articulated near/far
  silhouettes through one culled batch.
- [x] Test deterministic populations, biome/depth bounds, state reachability, player response,
  finite long-run motion, activation tiers, and land/water exclusion for all new species. Add
  dedicated debug counts and at least one fixed QA view that proves readable silhouettes without
  inflating invisible populations or the mobile draw-call budget. Crab determinism, habitat,
  12,000-frame finite motion, behaviour reachability, player response, activation tiers, F1 counts,
  benchmark ranges, and a matched `crabs` QA view are covered. Butterfly determinism, habitat and
  altitude bounds, 12,000-frame finite motion, all three states, observer tiers, player response,
  F1/benchmark counts, and a matched `butterflies` QA view are also covered. Turtle tests cover
  deterministic reconstruction, initial/12,000-frame habitat and finite-motion invariants, all
  three states including surfacing, activation tiers, mild player separation, F1/benchmark counts,
  and a matched `turtles` QA view on all three renderers.

### Monkeys and skeletal animation

- [x] Select or create a legally redistributable monkey/substitute model and record provenance.
  The first troop uses three original runtime-generated low-poly silhouettes, four palettes, and a
  fourteen-bone rig recorded in `ASSET_LICENSES.md`, so it has no external asset or
  disappearing-source dependency.
- [~] Validate skeleton, clips, scale, axes, material fallback, and all three renderers. Native
  OpenGL 3.3 and actual Chromium/WebGL 2 captures agree; a KVM/gfxstream GLES 3 fixed view matches
  the preceding near silhouette, and the revised arm64 package builds/lints. The newest near fur
  tufts plus bone-following finger/toe silhouettes still await a follow-up GLES 3 close capture
  and physical-device animation observation.
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
  within 7.5 m causes a brief bounded retreat followed by observation. Calm idle/observe animals
  between 7.5 and 30 m also ease their existing head bone toward the player within a restrained
  0.34-radian limit, then relax the pose during locomotion or retreat.
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
  four-second washes are selected at varied 14--32 second intervals near audible surf. The four
  persistent beds now use 10/11/29/7-second lengths, delaying their complete shared phase
  recurrence to 6 hours, 12 minutes, and 10 seconds. Subjective balance and long-session fatigue
  tuning remain.
- [~] Add wind-through-vegetation, jungle beds, birds, insects where tasteful, monkey calls, distant
  wildlife, and sparse water splashes. An eleven-second light-canopy-breeze loop and subtle
  twenty-nine-second daytime-jungle bed with sixteen irregularly spaced insect phrases increase
  inland, two cooldown-protected water entry/exit textures are active, and two bird plus two monkey
  phrases now play sparsely from real nearby agents; richer biome layers remain.
- [x] Add positional one-shots/emitters where direction and distance improve presence. One pooled
  mono wildlife voice uses CNA `AudioListener`/`AudioEmitter`/`Apply3D`, camera orientation, agent
  positions, 24 m reference falloff, distance gates, and species habitat gates.
- [x] Add player wading/swimming movement audio without repetitive rapid triggering. One continuous
  seven-second procedural loop follows authoritative support depth, locomotion mode, and horizontal
  speed with asymmetric fade smoothing and wading/swimming pitch variation. The same source now
  gains a deeper 0.08-maximum undertow bed across the 42--72 m current field, with separate 650 ms
  cue smoothing and no dry-state leakage.
- [x] Add land footsteps tied to authoritative resolved movement. Four original 0.3-second clips
  provide paired dry-sand and interior-soil families; a distance scheduler varies 2.05--2.75 m
  walk/run stride, pitch, pan, gain, and source variant while rejecting blocked movement,
  swimming, non-finite input, and teleport discontinuities.
- [x] Bound simultaneously active sources and virtualize/throttle inaudible emitters. The current
  system has exactly four persistent loops and at most two transient voices. Wildlife owns one
  pooled voice, footsteps own one reusable voice, and water crossings pre-empt lower-priority
  wildlife/wave/footstep playback as needed. Inland waves, distant/dormant wildlife, and footsteps
  requested while both other transient slots are occupied are virtualized.
- [~] Normalize source material, remove clicks at loops, and test mono/stereo spatial assumptions.
  Core tests cover deterministic duration/rate, peak headroom, RMS energy, loop-boundary delta,
  distinct source PCM, silent wildlife endpoints, and the mono-to-CNA positional output path. A
  native report now measures all 16 exact sources for DC offset, peak/RMS, loop seam versus normal
  internal steps, zero-crossing rate, duration, uniqueness, and PCM footprint. The four persistent
  beds now remove their generated mean before PCM quantization and enforce an absolute normalized
  mean no greater than 0.0001; the report can export standard PCM16 WAV previews. Headphone
  listening remains.
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
  clean click reacquisition. Escape cannot terminate the Web session even if a browser forwards
  the key after dropping pointer lock; fullscreen presentation remains open.
- [~] Verify keyboard, mouse, focus loss, resize, audio unlock, and browser pause/resume behaviour.
  Controlled Chromium input now proves focus, WASD/arrows, persistent Shift-toggled running and
  restoration of walking on the second press, captures the transient run confirmation, simultaneous
  forward movement/relative look, F1, actual-Escape pointer release, continued frames without
  exit/error, delayed click reacquisition, aspect-preserving resize, first-click CNA mixer plus
  four-loop startup, and actual distance-scheduled footstep playback. Chromium's synthetic CDP
  freeze/thaw remains inconclusive. Firefox ESR 140.15 now
  independently passes the production first frame, trusted-click pointer lock, 44.1 kHz audio
  initialization, both persistent Shift edges, forward traversal/footstep, Escape release,
  delayed pointer-lock reacquisition, responsive 16:9 sizing, and an actual second-tab activation
  with hidden/visible, animation-resume, and fresh-game-input assertions. Audible hardware output,
  extended suspension, and audio-focus recovery remain unverified.
- [x] Package runtime assets for Web with useful missing-asset diagnostics. The current game has
  no file-backed runtime asset: geometry, shaders, small textures, font, animation, and PCM are
  generated from compiled project code. The deployable package therefore intentionally contains
  only HTML, JavaScript, and WebAssembly rather than an empty Emscripten `.data`/VFS payload.
  Schema-2 provenance validation rejects undeclared future files, and any future external runtime
  asset must add both target packaging and an actionable load failure before this decision changes.
- [~] Reduce download/startup size through mesh/texture/audio choices and measured compression.
  The current Release package is 8,802,501 bytes uncompressed and 2,496,567 bytes through
  deterministic gzip, a measured 71.6% transfer reduction with round-trip/hash/manifest coverage.
  Procedural PCM and generated render textures add no separate downloads. Production-host header
  validation, hardware-browser cold-load timing, and decoded-memory measurement remain open.
- [~] Add quality defaults appropriate to browser GPU/memory limits. The first browser policy uses
  a stable 1280 by 720 render surface with aspect-preserving CSS scaling; measured density and
  memory tiers remain open.
- [~] Run the current terrain/ocean/palm/jungle scene in Chromium/SwiftShader with overview, beach,
  grass-detail, and jungle active-render captures; the grass view requires the full player-bend
  strength on the custom shader. Firefox ESR now supplies inspected second-engine production,
  dedicated jungle, and near-surface-water frames on its reported Radeon WebGL 2 backend with
  explicit live-state gates. A visible interactive hardware-accelerated pass and the remaining
  Firefox scene matrix remain open.
- [~] Capture browser console output, `WEBGL2` renderer identity, clean lifecycle, and an
  active-render screenshot. Startup time, memory, and representative frame-time evidence remain.
- [~] Document browser requirements and verified versions without claiming untested browsers.
  Chromium/SwiftShader and the narrower Firefox ESR 140.15 evidence plus explicit gaps are
  documented; record a visible interactive hardware run before completing this item.

Exit validation:

- A user can load, enter pointer lock, walk, swim, observe wildlife, hear audio, and recover focus
  in an actual browser.
- The environment remains recognizably the same game, with documented quality differences only.

## Phase 7 — Android / OpenGL ES 3 (28 h)

Dependencies: stable input abstraction, scalable UI/debug display, mobile quality tiers.

- [x] Inventory Android SDK, NDK, Gradle, Java, device/emulator, and supported ABI state. The
  host KVM modules, group membership, and `/dev/kvm` access now work; no physical device is
  connected.
- [x] Adapt the CNA template Android shell with island-specific package/application identities,
  API 24 minimum/API 35 target, a launcher icon, no-backup policy, landscape presentation, and an
  isolated read-only-source SDL Java bridge module.
- [x] Add explicit `OPENGLES3` CMake and Gradle configurations plus reproducible arm64-v8a and
  x86_64 packaging instructions. Both ABI APKs build; the default output is arm64-v8a.
- [~] Implement dual-zone touch controls: a floating movement stick, relative look region, stable
  contact ownership, and a minimal asset-free HUD. Deterministic interpreter tests pass; full
  movement travel selects the 8.0 m/s land pace through 92/78 percent hysteresis and a warm-gold
  ring/thumb cue while partial travel retains analog walking. A top-right three-dot selector
  cycles low/standard/high look response without consuming the look pointer. The
  premultiplied translucent HUD and independent movement/look gestures run on the accelerated
  GLES 3 AVD. Simultaneous real-finger and physical-device ergonomic validation remain open.
- [~] Handle pointer IDs, safe display margins, fixed-landscape orientation, resize, and missing or
  released contact cancellation. Core tests cover ownership/cancellation/resize plus non-finite,
  implausibly remote, and invalid-display inputs with immediate valid-contact recovery. The
  lifecycle gate proves both same-process Home/resume surface/audio restoration and a
  force-stop/new-process qualified GLES 3/audio launch; longer suspends, OS-initiated eviction,
  and physical-device recovery remain open.
- [~] Add configurable sensitivity and left/right layout considerations where practical. Runtime
  touch look now offers bounded 0.72/1.00/1.35 multipliers, selection feedback, hold debouncing,
  resize/hot-resume retention, and CNA-backed cold-process persistence. A handed layout remains
  open.
- [~] Verify audio focus/lifecycle and resource restoration after backgrounding. AAudio opened at
  44.1 kHz stereo, paused during Home, restarted after a same-process hot resume, and recreated
  all qualified game audio resources after a force-stop/new-process launch. External focus
  interruption, long suspension, OS-initiated eviction, and audible physical-device quality
  remain open.
- [x] Add mobile quality defaults for grass, shadows, wildlife distance, water, textures, and audio.
  Android now selects one validated game-owned profile: 112 m distant-grass cutoff, 72% contact-
  shadow range, 82% wildlife draw range, and a 128 by 128 ocean grid while retaining identical
  close meshes/shaders, all populations, the 143,624-byte texture inventory under a 160 KiB gate,
  and the six-source soundscape. Linux/Web retain the full profile. Core tests validate both
  profiles, native runtime output confirms the full selection, and Chrome/GLES3 gates require the
  corresponding diagnostics; physical-device tuning remains open.
- [~] Profile CPU/GPU frame time, memory, thermal behaviour, and loading on a real device when
  available; use emulator evidence only for claims it can support. The KVM/gfxstream AVD now has
  startup, memory, and SurfaceFlinger before/after samples; no physical GPU, thermal, or power
  budget is claimed.
- [~] Test install, first launch, relaunch, background/foreground, rotation policy, and clean exit.
  The x86_64 debug APK installs and starts on the KVM-accelerated API 35 `Medium_Phone` AVD,
  produces a 2400 by 1080 GLES3 surface, accepts movement/look gestures, and survives a same-process
  background/foreground cycle. The repeatable runtime gate installs, launches from cleared logcat,
  requires renderer/first-frame/current-grass, near-seed-head, near-rock crown/facet, near-bird-
  feather, ocean-crest, grazing-reflection, and rock-tint diagnostics, rejects fatal/shader errors,
  and records
  logcat plus a device screenshot. Its opt-in audio mode additionally requires bounded sequential
  source staging, persistent-loop DC centering, and slow non-amplifying gain variation through
  Android's platform log rather than unavailable SDLActivity stdout. Its separate footstep mode
  injects sustained movement-zone touch input and requires actual land-footstep playback. A
  separate swimming mode reverses through the interactive south-beach shallows, requires the
  authoritative walking-to-swimming transition, and also requires water-transition playback when
  the audio gate is active. Debug
  builds now accept a
  release-disabled intent argument array
  for repeatable fixed-view capture; inspected beach, close-grass, jungle, water, monkey, and
  reef GLES3 frames match the corresponding WebGL 2 composition and material paths. Rotation
  lifecycle mode now validates same-process Home/resume and force-stop/new-process relaunch with
  dedicated logs and screenshots. Rotation policy, long suspension, clean exit, OS-initiated
  eviction, and physical-device relaunch remain open.
- [x] Recheck the software AVD's 27 gfxstream `GL_INVALID_OPERATION` uniform-query reports on a
  stable KVM emulator before assigning them to the driver, framework, or game. They did not recur
  in the accelerated run, and no shader/fatal error appeared; no dependency bug is claimed.
- [~] Document device/API/ABI evidence and all unverified hardware classes. Build/install/runtime
  commands and KVM/gfxstream emulator limits are recorded; real-device evidence is absent.

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
  warm mean/median/p95/p99 frame intervals, exact 120/60/30 Hz miss counts, and scene counts; a
  total backend voice counter remains.
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
  baseline records frame distributions and tracked scene submissions. The schema-3 moving-route
  report adds population ranges and exact 120/60/30 Hz misses. Three isolated optimized-release
  suites now establish a first current-GPU Linux gate: every fixed and moving view retains p99
  below 16.667 ms and the instrumented run has no interval above 33.333 ms. A schema-1 first-draw
  report exposes five startup phases and is gated on actual OpenGL 3.3 plus the WebGL 2 timing-log
  path, but browser cold-load, Android hardware, memory, audio, and thermal budgets remain open.

### Optimization

- [~] Profile representative beach, dense jungle, hill vista, waterline, and wildlife scenes. The
  first fixed-view native suite covers overview, beach, dense jungle, shore detail, and monkeys;
  a deterministic 676 m native route now forces culling/LOD/contact-shadow refreshes across
  shallows, both coasts, trail, clearings, and dense jungle. Three clean optimized-release suites
  now cover the full matrix; hardware WebGL 2 and physical Android profiles remain open.
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
  trees select 776/208-triangle meshes per instance, and grass selects 42/10/5-blade
  clusters; the initial native scene baseline is recorded, while target budgets and true blended
  transitions remain open.
- [x] Throttle distant wildlife and audio updates without visible popping. Shared
  full/reduced/dormant simulation tiers bound agent work; positional calls select only nearby
  non-dormant sources, use one voice, and are held while transient slots are occupied.
- [~] Reduce transparency/overdraw hotspots, especially grass and layered shoreline effects. The
  complete pass audit confirms that grass and every vegetation/prop/wildlife mesh use opaque
  geometry. Transparency is limited to the depth-tested ocean, one 8 m shoreline ribbon, one
  frustum/distance-culled contact-shadow batch, conditional water/touch overlays, the opt-in debug
  panel, and the 1.4-second pace notice. Physical-mobile fill-rate profiling remains open.
- [x] Audit texture formats/sizes/mipmaps and create smaller variants only when justified. The game
  owns only six generated single-level `Color`/RGBA8 textures: a 96 by 32 debug atlas, two 96 by 96
  touch circles, one 160 by 90 water-view veil, and two one-pixel white textures. Their exact
  base-level payload is 35,906 pixels / 143,624 bytes, guarded by a compile-time inventory and core
  test. With no external texture asset or mip chain, separate Web/mobile variants are unjustified.
- [~] Measure loading and remove redundant runtime generation/conversion. Game-owned wall-clock
  phases now identify deterministic world construction and GPU/content creation separately through
  first draw. Procedural audio now generates, converts, uploads, and releases one source at a time,
  eliminating at least 4,008,688 bytes of simultaneous game-owned staging without changing source
  PCM. Repeat isolated release measurements and profile the dominant generators before changing
  the reproducible runtime-generation architecture further.

### Reliability and automated coverage

- [~] Test malformed/missing terrain, configuration, model, texture, animation, shader, and audio
  inputs for actionable failure diagnostics. `IslandCommandLineValidation` now launches the real
  executable for eight invalid option/value/conflict cases and requires exit code 2, specific
  stderr guidance, and no stdout before graphics initialization. The settings parser rejects
  oversize, embedded-NUL, duplicate, missing-schema, unsupported-schema, and invalid-value inputs;
  an actual native launch test also proves valid-backup recovery. Malformed content cases remain.
- [x] Add assertions for finite transforms, valid indices, terrain query domains, animation palette
  sizes, and impossible player/wildlife states in development builds. Player and all four wildlife
  simulations now reject non-finite frame/input values before they can poison persistent state and
  assert finite poses, bounded animation values, valid populations, and home/range invariants in
  debug builds. Monkey renderer startup validates all generated mesh indices, the complete
  fourteen-bone hierarchy/palette, four required clips, track palette indices, ordered key times,
  and finite key transforms before GPU upload. Public terrain queries now define a one-million-
  metre numeric domain and fail closed to a finite upward-facing deep-seabed sample outside it,
  preventing unsafe float-to-noise-cell conversion. One tested core validator now rejects empty,
  incomplete, oversized, or out-of-range 16-bit triangle meshes before every indexed procedural
  renderer upload.
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
- [~] Retune acceleration, deceleration, slope response, eye height, mouse/touch sensitivity,
  swimming, and water transitions from play evidence. Horizontal acceleration and braking are now
  vector-bounded, removing the former faster diagonal response while retaining the 4.3/8.0 m/s
  walk/run targets; longer subjective traversal tuning remains open.
- [~] Improve grass close-up shape, density transitions, colour variation, wind coherence, and
  aliasing until it holds up at first-person height. Five-level near curvature, folded blade-side
  colour, shared-sun two-sided lighting, root darkening, and restrained transmission now add
  first-person depth without textures or extra draws. Biome-aware instance colour and sparse
  height-weighted sun-cured tips now break up broad and blade-scale uniformity. Seven near juvenile
  blades plus per-instance stature/colour/wind variation further reduce repeated tuft silhouettes.
  Six sparse seed-bearing stems add restrained near-field vertical accents while leaving the
  middle/far meshes, population, buffers, and draw count unchanged. The near tier now uses 42
  blades, including 14 restrained geometric V-fold ridges with side-specific lighting. The shared
  near cluster is 622 vertices/496 triangles, while the middle/far meshes and all runtime
  resources/submissions remain unchanged.
  The light middle
  mesh and two adjacent-tier population splits remove the old whole-cell/direct-extreme swap;
  transition-motion review, aliasing, and physical-device tuning remain.
- [~] Break repeated palm/prop silhouettes and correct floating/sunken placements. Nine paired
  tapered-leaflet stations now replace each near-palm ribbon while widened far crowns preserve the
  envelope; the existing leaflet root chords are widened modestly after original-resolution review
  showed skeletal gaps, without adding geometry. Near-only procedural trunk scars and coconut
  clusters on two variants add a first
  material/content pass without new draws. Five deterministic near-root wedges now vary in angle,
  length, width, and height rather than reading as flat tabs or a radial stamp. Fallen-frond
  leaflet roots now follow their curved rachis; pair/side sweep, droop, and ageing variation break
  the former mirrored-comb silhouette without changing its mesh count or batches. Existing palm
  transform coefficients now also seed restrained green-only crown tint while preserving brown
  structural materials and near/far colour identity without a new resource or draw. Fallen-log
  near ends now replace the former planar circular cuts with deterministic irregular closed
  contours by repositioning existing vertices only. Prop repetition,
  broader palm variation, physical-device placement review, and true LOD blending remain.
- [~] Refine shoreline contours, wet edge, shallow colour, foam, wave scale, and water/terrain
  seams. The exact foam ribbon, wet terrain edge, tropical depth colour, and subtle movement-driven
  player ripples plus a bounded water-entry/exit surface burst are functional. The overlapping
  broad ocean foam and exact ribbon wash now share broken continuity shaping, removing the most
  obvious uninterrupted bright waterline from the low water view. A tightly shore-bounded
  derivative-normal sun sheen now distinguishes wet sand from the still-matte dry beach without
  another texture or pass; repeated whole-island composition tuning remains.
- [~] Refine jungle density versus traversability and strengthen landmarks/clearings/navigation
  cues. The existing open corridor now has a subtle authored footpath material tied to its exact
  placement curve; its visible wear was narrowed and given local shaded/sun-dry variation while the
  broader vegetation-free route remains walkable. The east/west clearing definitions drive
  distinct surface treatments plus a first natural landmark pass made from five rocks and one
  fallen log around their outer rings. Their open floors now reuse the existing broad, medium, and
  fine terrain fields for bounded dry/cool and leaf/moss patch breakup without another noise field.
  Broader landmark variety and whole-route density retuning remain.
- [~] Refine fixed lighting, shadow/contact cues, sky, haze, exposure, and distant scenery as one
  coherent permanent sunny art direction. Terrain/ocean/sky share one sun, grass has two-sided
  blade lighting, the jungle now has derivative-based two-sided facet light, and a bounded contact
  batch grounds major near objects; unified atmospheric/exposure tuning and broader material
  response remain.
- [~] Improve bird/fish/shark/monkey timing, animation transitions, path variation, and clipping.
  Bird paths now drift slowly around their authored radii and their CPU pose blends between wing
  strokes, glides, and banked turns; paired near trailing-feather surfaces broaden the close wing
  silhouette without another buffer or draw. Fish and shark now pitch with vertical motion, bank
  through turns, have clearer forked-tail/head silhouettes, and receive a shared depth/caustic
  colour profile that visually seats them in the water column. Calm monkeys now use bounded
  player-aware head posing on top of their existing clips, and three restrained near-only crown/
  cheek tufts break up the smooth head outline without another draw; long subjective tuning remains
  open.
- [~] Improve soundscape variation, spatial placement, transitions, balance, and long-session
  fatigue. The 10/11/29/7-second persistent beds already avoid whole-set realignment for more than
  six hours; three mismatched 44--131-second deterministic gain fields now add restrained,
  non-amplifying surf/breeze/jungle movement without new PCM or voices. Positional bird/monkey
  calls and two variants per transient class are active; subjective headphone/speaker fatigue and
  balance review remains open.
- [ ] Fix frame pacing, loading hitches, input focus issues, touch ergonomics, and platform-specific
  rendering differences found during real play.

### Release qualification

- [~] Run Linux release build and multiple fresh-user smoke sessions. A clean two-worker release
  tree now builds directly against the read-only sibling dependencies, passes the core/tool gates,
  starts four times on isolated OpenGL 3.3, and produces a visually inspected overview. A dedicated
  release preset now installs a stripped game plus `libcna.so` with a relative runpath, complete
  linked-dependency notices, an integrity manifest, and a deterministic archive. That archive also
  passes a private-display smoke run after extraction under `/tmp`; a longer subjective release
  traversal from a fresh user environment remains open.
- [ ] Run actual-browser release build, cold load, focus/pointer-lock, audio, resize/fullscreen, and
  extended-play tests.
- [ ] Run Android release-like package install, cold/warm launch, lifecycle, touch, audio, thermal,
  and extended-play tests.
- [x] Compare a representative view matrix across all three renderers and document unavoidable
  differences rather than hiding them. Native and Web fixed-view evidence is joined by inspected
  Android beach, close-grass, jungle, water, monkey, and reef captures; Android's centred viewport,
  touch HUD, and emulator-only performance scope are recorded explicitly.
- [x] Review every external and original distributable asset against `ASSET_LICENSES.md` and
  redistribution requirements. The tracked source tree has no external binary/content files; its
  one non-generated visual asset is the original Android vector launcher icon, now hash-gated by
  manifest schema 2 with a matching attribution row.
- [x] Review `BUGS.md`, known limitations, disabled fallbacks, warnings, and unverified claims. The
  latest source-only audit after the `97783d1e090c` baseline refresh found no qualifying dependency
  bug, retained explicit stock-effect/audio degradation diagnostics, corrected one stale Android
  sky-runtime claim, and preserved physical-device/browser gaps as unverified.
- [~] Review repository size and remove only project-owned obsolete artifacts after checking for
  live users/processes. The first maintenance pass removed a stale 1.1 GiB one-off arm64 consumer
  tree while preserving the live native tree, current Web/Android outputs, reusable SDL caches,
  and retained runtime evidence; repeat before final packaging.
- [~] Synchronize README, architecture, controls, Linux/Web/Android instructions, renderer
  differences, debugging, asset pipeline, performance notes, and known limitations. Current CNA
  identity, artifact sizes, Shift semantics, and Web control discoverability are synchronized;
  repeat after future runtime or visual changes.
- [~] Run formatting/static checks, unit tests, smoke tests, `git diff --check`, and a source-only
  repository inspection. Shell/Node syntax, executable modes, tracked-file types, approved build
  directories, local Markdown links, asset provenance, package determinism, full native 46/46, and
  full Web 27-stage Chrome plus Firefox second-engine gates currently pass; repeat the aggregate
  gate at final packaging.

Exit validation:

- A fresh user can follow the documentation and launch each verified target.
- The full island supports walking, jungle exploration, near-shore swimming with a natural limit,
  wildlife observation, and a cohesive soundscape.
- No documentation claims a platform, renderer, effect, control, or asset that was not actually
  validated.

## Phase 10 — Extended visual-realism campaign (up to 100 h)

Dependencies: stable feature-complete scene, current cross-renderer captures, representative
frame-time baselines, and the user-directed shipwreck/additional-wildlife scope above. This phase
improves the requested experience rather than adding quests, combat, survival, or progression.

### Realism target and art direction

- [~] Define a restrained tropical realism target that preserves the current bright permanent-day
  identity while replacing toy-like flatness. Use matched first-person reference boards for sand,
  wet shore, shallow water, palms, broadleaf jungle, grass, stone, aged wood, and atmospheric
  depth; do not copy copyrighted game assets. The first 12-view audit retained the bright fixed-
  daylight identity and ranked uniform interior ground above adding more features; broader
  reference and colour-script work remains open.
- [~] Audit every fixed QA view at original resolution and rank defects by visual impact, first-
  person frequency, portability cost, and measurable performance risk. Record before/after evidence
  for changes rather than increasing polygons or shader layers blindly. Beach, jungle, grass,
  palm, ground-plant, prop, driftwood, reef, water, distant, butterfly, and turtle frames were
  reviewed together; remaining fixed views and repeated in-motion review remain open.
- [~] Establish consistent real-world scale for terrain features, vegetation, rocks, wildlife, and
  the approximately 20 m shipwreck. Correct visibly implausible thicknesses, floating roots,
  intersections, horizon scale, and eye-height cues. The wreck now has exact 20.0 m length,
  5.2 m beam, 4.4 m hull height, shallow-water support sampling, broken deck rails, hull openings,
  keel/rudder, and two damaged-mast silhouettes reviewed from the first-person beach camera; the
  whole-island scale audit remains open.

### Materials and surface detail

- [~] Upgrade terrain response with portable multi-scale albedo variation, material-specific
  roughness/specular cues, and bounded micro-normal or derivative detail for dry sand, wet sand,
  soil, leaf-rich floor, grass substrate, rock, and path wear. Prefer shared procedural fields or a
  small reproducible texture set with mipmaps; avoid shimmer and mobile-unfriendly texture cost.
  The inland floor now desaturates its broad green substrate and classifies irregular humus/moss
  areas from already-evaluated portable fields with clearing, slope, and distance masks; broader
  roughness and micro-normal work remains open.
- [~] Add physically coherent fixed-sun wetness to the shoreline, submerged stone, driftwood, and
  shipwreck without making every surface glossy. Match foam, wet edge, shallow colour, and seabed
  visibility across OpenGL 3.3, WebGL 2, and GLES 3. The shipwreck now has a bounded sea-level
  wet/submerged band, salt line, and small view-dependent sheen on all three renderers; broader
  shoreline/material unification remains open.
- [~] Improve aged wood, bark, leaf, stone, shell, wrack, and shipwreck materials with readable
  scale, colour breakup, edge wear, and distance fade. Keep far LODs calmer than the near detail.
  The wreck now has distance-faded longitudinal grain, broad grey/green weathering, restrained
  cracks, differentiated fresh breaks, and calmer distant colour; other material families remain.

### Geometry, vegetation, and composition

- [~] Improve close palm trunks, roots, crowns, frond curvature, leaflet density, dead-frond/coconut
  variation, and transitions while breaking obvious clones. Preserve a cheaper distant silhouette
  and validate wind/aliasing on mobile. The current pass adds sparse hanging dry-frond remnants to
  selected near variants inside the existing instanced mesh. Matched OpenGL 3.3, Chrome/WebGL 2,
  Firefox/WebGL 2, and KVM/gfxstream GLES 3 views pass and were inspected; the fixed Release palm
  scene retained eight palm submissions and no frame-budget misses. Moving LOD/wind review and a
  physical-mobile pass remain open.
- [~] Improve broadleaf crown layering, branch hierarchy, trunk taper, buttress integration,
  understory leaf forms, vines, fallen leaves, and forest-floor occlusion. Keep the jungle dense but
  navigable and avoid replacing one repeated silhouette with another. The current pass targets the
  conspicuous rounded near-canopy shells with ring-varying contours and sparse secondary drooping
  edge leaves while preserving far LODs and instanced draw counts. Matched OpenGL 3.3,
  Chrome/WebGL 2, Firefox/WebGL 2, and KVM/gfxstream GLES 3 views pass and were inspected; the
  fixed Release jungle scene has no frame-budget misses. Branch hierarchy, broader understory
  variation, moving-wind inspection, and physical-mobile review remain open. A second pass adds
  near-only trunk growth irregularity and tapered bent two-segment support branches while
  preserving far meshes, collision proxies, and submissions; native, both browser engines, and
  KVM/gfxstream GLES 3 validation pass. Broader understory variation, moving-wind inspection, and
  physical-mobile review remain open.
- [~] Rework grass blade width/taper/curvature, cluster footprints, height/density fields, colour
  gradients, seed heads, wind phase, terrain-normal seating, and near/middle/far transitions until
  first-person gaps and repeated tufts are no longer distracting. The first realism pass narrows
  the shared near blades, adds stable edge/bow asymmetry, and bends a minority of tips without
  changing counts, LOD, shader work, or submissions; its matched OpenGL 3.3, WebGL 2, and GLES 3
  views pass. Profile overdraw and alpha-free geometry cost on all targets before increasing
  population; cluster-footprint and transition composition remain open.
- [~] Strengthen the island's authored composition with realistic erosion cues, beach-width
  variation, shallow sandbars, reef groupings, grove structure, denser/sparser jungle rooms,
  landmark sightlines, and controlled openings. Integrate the shipwreck and debris into one chosen
  cove without damaging the existing main traversal loop. The wreck and beachward debris now form
  a dedicated coastal landmark outside the reserved primary corridor. Ten of twelve major
  pieces now diverge at least 0.4 radians from the hull in both directions, removing the parallel-
  bar composition; broader erosion, grove, and sightline refinement remains open.

### Lighting and atmosphere

- [ ] Unify fixed-sun direct light, sky fill, material response, contact grounding, haze, water
  reflection, and exposure. Evaluate a baked/static sun-occlusion or tightly bounded portable
  shadow solution; accept it only if it improves first-person depth on WebGL 2 and GLES 3 within
  measured budgets.
- [ ] Improve sky colour gradient, sun/halo, cloud scale, horizon haze, distant-islet integration,
  and aerial perspective while retaining permanent pleasant sunny weather and no time-of-day
  system.
- [ ] Reduce visible aliasing, overbright edges, flat silhouettes, abrupt LOD changes, vegetation
  pop, depth-order artefacts, and colour discontinuities in motion, not only in fixed screenshots.

### Life, motion, and atmosphere polish

- [~] Integrate crabs, butterflies, sea turtles, existing birds/fish/shark/monkeys, vegetation wind,
  ocean motion, and positional sound into believable local activity rhythms. Avoid synchronized
  loops, clutter, and constant attention-seeking behaviour. Six sparse crab colonies and three
  small clearing-edge butterfly habitats now provide localized motion with asynchronous state
  timing. Five sparse near-shore turtles now add asynchronous cruise/forage/surface motion without
  entering the current region; whole-scene activity-rhythm critique remains.
- [ ] Add only subtle shipwreck ambience that can reuse the bounded soundscape budget, such as
  wave wash/wood creak scheduling from existing water layers; do not create a permanently loud
  global loop or imply a quest.
- [ ] Tune camera motion, movement acceleration, slope response, swimming, shoreline transitions,
  and collision against the improved scale/detail so the player remains grounded rather than
  moving through a static diorama.

### Performance and acceptance

- [~] Preserve the current Linux 60 Hz p99 gate on representative scenes, define a shipwreck/wildlife
  worst-case view, and compare draw calls, triangles, texture bytes, startup, and frame pacing
  before/after each major realism pass. Scale distance/LOD rather than removing core content on
  Web or Android. Two isolated Debug shipwreck runs measured 1.9153--1.9271 ms median,
  4.5776--4.6285 ms p95, and 9.4118--10.4471 ms p99 with zero 60 Hz misses; the additional-wildlife
  combined worst case remains to be defined. The isolated Release turtle view records 2.1252 ms
  mean, 3.2192 ms p95, 9.5893 ms p99, two 60 Hz misses, and no 30 Hz miss across 600 intervals.
  The current 3,948-triangle/material-shader shipwreck Release view recorded 1.4082 ms mean,
  1.8133 ms p95, 8.1463 ms p99, zero 60 Hz/30 Hz misses, 46 tracked submissions, and one wreck draw
  across 600 intervals.
- [ ] Run matched native, Chrome, Firefox, and accelerated-Android captures for every new shader or
  renderer path. Validate physical Android hardware when available; otherwise keep thermals,
  driver diversity, and real-device ergonomics explicitly unverified.
- [ ] Complete three recorded whole-island critique passes after the realism work: traversal/scale,
  material/lighting/composition, and wildlife/sound/motion. Fix high-impact defects found in those
  passes before declaring the extended campaign complete.

Exit validation:

- First-person beach, jungle, clearing, hill, water, wildlife, and shipwreck views form one
  coherent sunny tropical scene and no longer rely on flat colour or repeated silhouettes for
  their primary read.
- The approximately 20 m wreck is clearly a large broken ship with water-integrated debris, remains
  safely explorable, and does not become an objective or combat/story system.
- Additional wildlife reads naturally at intended distances and remains inside valid terrain/water
  regions without compromising the established platform budgets.
- Linux, WebGL 2, and GLES 3 still present the same complete island experience with documented,
  measured quality scaling only where necessary.

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
| Grass density/distance | Full, 135 m | Full, 135 m | Same density/near mesh, 112 m maximum |
| Dynamic shadows | Portable contact batch, full range | Same full range | Same batch, 72% range |
| Ocean detail | Full layers, 160 by 160 grid | Same layers/grid | Same layers, 128 by 128 grid |
| Wildlife range/count | Full | Full | Same counts/simulation, 82% draw range |
| Texture/audio sizes | 143,624 bytes / six sources | Same bounded inventory | Same bounded inventory |

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
  frame spacing, player/shore/current/water-ripple/water-view state, renderer/logical viewport,
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
  retained with depenetration/obstacle/terrain/axis-slide/safety-clamp classification; movement
  history, geometric step-up, and continuous dynamic-obstacle recovery remain.
- [~] Shore distance/current vector visualization. F2 draws the irregular shoreline, all three
  current thresholds, and the player's exact active inward/circulating-current arrow; a scalar
  field view remains.
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

- [~] `README.md`: concept, controls, quick start, and honest status exist. A current hash-gated
  native overview now provides the first actual release-facing screenshot and is packaged beside
  the README; a restrained first-person gallery and final release copy-edit remain.
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
  documented with functional accelerated-AVD gestures and three runtime touch-look response
  levels; persistent selection, handed layout, simultaneous real-finger use, and physical-device
  ergonomic notes remain open.
- [x] `docs/asset-pipeline.md`: current generation model, versioned manifest schema, validation,
  world report, external intake, deterministic conversion requirements, and limitations.
- [~] `docs/renderer-differences.md`: current visual/input/runtime evidence and explicit gaps are
  documented; seven KVM Android fixed views are matched against WebGL 2 and the established native
  suite, while representative physical-device measurements remain open.
- [~] `docs/debugging.md`: overlay fields, direct QA teleports, deterministic captures, and the
  native performance-report workflow are documented. Android debug packages can now receive the
  same fixed-view arguments through a release-disabled intent bridge; deeper world-space
  visualizers and common-failure guidance remain open.
- [~] `docs/performance.md`: native method, five representative fixed scenes, the initial Linux
  baseline, startup phase/report semantics, interpretation rules, and open budget work are
  documented. Android now has KVM startup, memory, and same-AVD terrain-batching evidence;
  release/Web/physical-Android, repeated cold loading, audio, and thermal budgets remain open.
- [x] `docs/known-limitations.md`: current environment, traversal, content, platform, and tooling
  limitations are recorded without claiming planned systems.

## Current execution queue

Work next, in order:

Immediate user-directed expansion:

1. Execute the extended visual-realism campaign above in evidence-driven passes: materials and
   lighting first, then vegetation/grass geometry and composition, then atmosphere/motion polish.
   Preserve current performance evidence and complete matched renderer reviews after each pass.
   The first ranked terrain pass is complete on all three renderers; continue with close grass and
   vegetation silhouette/composition improvements before adding more material cost.
2. Continue the large 20 m sailing-ship wreck realism pass with beach-clutter composition and
   subjective collision feel without reducing it to a boat, changing its established scale, or
   turning it into an objective. Its aged-wood breakup, waterline material, curved lower hull,
   localized missing planks, keel/rudder, broken rails, hull openings, second damaged mast, and
   bidirectionally scattered splintered debris are complete and portable. Review traversal/collision
   feel and add only restrained shore integration rather than accumulating props blindly.

Existing observation and qualification queue:

1. Tune the complete four-layer soundscape, current-responsive undertow mix, both accent pairs,
   paired sand/soil footsteps, and the pooled positional bird/monkey voice through a 20-minute
   headphone/speaker route.
   Numerical headroom, exact-source signal reports, disposable validated WAV previews, actual
   Linux playback, finite-input fail-closed control state, the six-source ceiling, and a 6-hour,
   12-minute aggregate loop phase period and bounded sequential upload staging are verified;
   subjective masking and fatigue are not.
2. Continue long subjective observation of the GPU-skinned monkey troop and its restrained
   27--50 second positional call cadence. Three silhouettes, four palettes, short clip crossfades,
   shared trunk/rock/log avoidance, two call phrases, a surface-visible near face/eye/nose pass,
   bone-following near fingers/toes, and restrained calm-state player-aware head posing are
   complete first passes. Bird flight now adds slow per-agent radial drift, banked turns,
   intermittent smooth glides, and layered trailing feathers on the near wing silhouette without
   another buffer or draw. Fish and shark movement now follows depth with
   pitch, banks through turns, and uses clearer forked-tail/near-head silhouettes without another
   draw; keep all four wildlife layers under subjective observation during play.
3. Improve ground/contact variation and enrich the forest-floor silhouettes and sparse driftwood
   layer only where composition review needs it. The first low-bush silhouette follow-up now adds
   paired folded leaves over smaller near lobes without touching its far tier; near ground-plant
   variants now include both an upright six-leaf form and a five-leaf folded split form. The
   dedicated cross-renderer QA camera targets the latter and verifies a nonzero per-instance near
   LOD population. One existing near variant now also carries three restrained folded fallen
   leaves in its existing mesh/draw, with a second dedicated cross-renderer camera for direct
   composition review.
   Near land and reef rocks now add restrained, texture-free strata, lichen, and a low
   fixed-sea-level wet band only in their near LOD. Bounded upper-ring height offsets plus a taller
   asymmetric cap break the former flat crown, and derivative fixed-sun light reveals the existing
   planes without changing vertex/triangle counts, textures, instances, buffers, or submissions.
   Their neutral stone tones now also use transform-derived per-instance warm/cool tint with stable
   near/far identity and no new instance data or submission.
   The grass close-up pass now gives one-third of the near blades a real V-fold ridge while keeping
   the remaining two-thirds broad and curved; continue reviewing aliasing and density transitions
   rather than increasing the mobile-visible population blindly.
   Near fallen logs now add distance-faded local-space bark fissures and subtle irregular growth
   rings through the same material path, also without new geometry, textures, or submissions.
   Near sun-bleached driftwood now adds a softer faded longitudinal grain while leaving cut ends
   and far meshes plain. Its third near variant uses a curved three-limb fork; the former flat
   branch fins on variants zero and one are now tapered, bent three-dimensional limbs through the
   same near meshes. Both end caps now reuse the exact irregular outer body rings rather than
   leaving smaller circular seams. The dedicated native/WebGL QA camera proves a nonzero per-
   instance near LOD.
   Rocks, logs, and
   driftwood now enforce range and LOD from their actual positions after conservative cell bounds.
   The portable sand/soil, wet-edge, and slope-material pass, the initial small-plant layer, a
   first near-field shell/pebble/dry-wrack substrate layer, a first grove-associated fallen-frond
   layer with curved asymmetric leaflets, shallow geometric central folds, and restrained
   two-sided fixed-sun response, and 14 emergent shallow-reef outcrops are complete.
   Texture-free near-field substrate microvariation and sparse
   curved interior leaf litter and a first fixed-sun floor-dapple treatment now break up exposed
   jungle-floor gaps. The vegetation-reserved primary corridor now has a subtle shared broken-edge
   dry-earth material cue under its short grass. The east/west clearings now share their placement,
   QA, and distinct warm-dry/leaf-rich terrain definitions. Their floors now reuse the same broad,
   medium, and fine fields for larger irregular dry/cool and leaf/moss patches. Five authored rocks
   and one fallen log in each clearing give their outer rings different natural silhouettes while
   preserving the central walking room. Three spaced non-solid driftwood pieces now use all existing silhouettes as a landmark
   arc in one sheltered cove; broader landmark variety, prop diversity, contact cues, authored
   biome variation, and material tuning remain.
   The portable near-surface swimming veil now adds depth haze and restrained upper-surface
   scatter without another draw; movement-driven surface ripples and a bounded speed-scaled
   broken-ring effect for moving water entry/exit are also complete. A clean six-view
   composition audit then identified overly uniform exposed dry sand
   and low-contrast close forest soil; sparse soft-edged pebble/shell flecks and a restrained
   contrast increase now address those specific gaps without new geometry, textures, instances,
   or submissions. A subsequent cove review found all four small beach-detail variants too
   similar, so the fourth now uses low dry-wrack strands while retaining the same 265 placements,
   buffers, culling, and maximum four submissions. The latest east-clearing composition review
   found its intended sunny/dry identity too close to surrounding forest soil, so the existing
   shared material now gives that mask a clearer warm olive cue without changing its geometry or
   walkable centre. A later side-by-side audit strengthened only that same existing warm/dry blend
   and restrained its competing cool patch; the east room now reads separately from the unchanged
   west floor without new shader inputs. Defer waterline geometry, underwater fog, and
   refraction until a similarly portable design is justified.
4. Extend the completed F2 shoreline/current, terrain-contact, nearby collision, shared-cell,
   wildlife-heading/navigation, bird route/spawn/live-target, and audio-zone/emitter visualization
   with individual culling/LOD/density fields only where they materially improve debugging. F1 now
   reports local static-collision candidates, nine terrain support samples, wildlife
   marker/target/home-range counts, bird route-family/spawn/target counts, and live audio sources
   against their complete populations/budgets. F1 is now independently toggleable from fixed QA
   cameras, and the browser matrix validates the populated panel before hiding it for clean visual
   captures; retain diagnostic screenshots only where the panel itself is the subject under test.
5. Continue Android qualification on physical hardware. The KVM-accelerated API 35 AVD now proves
   complete-scene GLES 3 rendering, independent movement/look gestures, translucent touch HUD,
   same-process surface restoration, AAudio pause/resume, startup/memory instrumentation, and a
   terrain-batching direction check. A debug-only fixed-view bridge now proves matched beach,
   close-grass, jungle, water, monkey, bird, and wet-reef views on GLES3. An automated touch route
   now crosses the south shoreline into the real swimming state and plays its transition accent.
   Simultaneous real-finger ergonomics, external audio focus, process
   death, thermals, and physical mobile-GPU performance remain open.
6. Perform a visible hardware-accelerated Web listening/play pass, fullscreen pass, and extended
   tab-suspension/audio-focus test; automated first-gesture audio initialization is complete.
   Firefox ESR now independently passes the production first frame, essential input/audio path,
   responsive 16:9 sizing, Escape release, delayed pointer-lock reacquisition, and fixed jungle/
   water live-scene gates. It also passes one actual short second-tab activation with visibility,
   animation-resume, and fresh-game-input evidence. The headless Chrome freeze/active probe remains
   diagnostic-only because the latest passing input run did not emit a post-resume game sample
   even after a three-second allowance.
7. Observe the irregular faceted palm roots and fuller segmented-leaflet versus widened-ribbon
   125--145 m transition during traversal and on physical mobile hardware. Current close-palm
   evidence justified widening the existing leaflet root chords and then breaking the remaining
   regular opposed-comb silhouette through vertex-only length/stagger/sweep/droop variation; the
   close-root, crown-silhouette, and near material/coconut passes are measured. A restrained
   near-only derivative-normal fixed-sun pass now gives the existing planes daylight response while
   leaving far fragments unchanged. Do not add geometry without new evidence.
8. Observe the retained branch-supported broadleaf crowns and staggered 105--129 m transition
   during traversal. A clear close-view cylindrical trunk-base defect justified a small near-only
   four/five-buttress pass; the far tier, culling, instances, and draws remain unchanged. Further
   canopy geometry should be justified by physical-mobile evidence or another clear defect. The
   fixed-sun facet-light and close distance-faded bark-fissure passes are visually matched on native
   OpenGL and actual WebGL 2; observe their readability, aliasing, and cost on physical mobile
   before extending the technique to more layers.
9. Add browser cold-load and physical-Android launch evidence before assigning cross-target startup
   budgets. Two isolated current-source Linux Debug and stripped Release repetitions are now
   recorded: Release reaches the first game-owned draw in 426--506 ms, with world construction at
   161--202 ms and GPU/procedural content at 203--234 ms. Debug reaches it in 1.76--1.98 seconds.
   The Release split shows no demonstrated bottleneck that justifies speculative restructuring.

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
  clamps magenta, and the resolved endpoint green. F1 now reports `MOVE D/O/T/S/C`, including
  later static-placement recovery; the normal presentation still pays no extra draw submission and
  the complete F2 view remains two draws.
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
  emitted. The reproducible sanitizer tree is removed after the pass to avoid retaining a
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

### 2026-10-06 — Web input-harness timing hardening

- Replaced repeated Chrome DevTools key-event sequences with one explicit press helper that holds
  both edges across multiple SwiftShader frames. Pointer-lock clicks now likewise hold the mouse
  button across game updates instead of assuming a 150 ms press will always be sampled. This
  hardens QA only; production game input and dependency code are unchanged.
- The complete real-Chrome WebGL 2 harness passes all 22 stages with the hardened timing. Its input
  evidence includes pointer lock and focus, CNA relative-mouse capture, keyboard traversal into
  water, the first water-transition accent, mouse look, camera-motion toggles, F1/F2/F4 debug
  controls, aspect-preserving resize, unlock/relock, and continued rendering without a browser
  exception.
- The synthetic DevTools freeze/active transition did not append a post-resume quarter-second game
  diagnostic in the latest passing run. It was already a reported diagnostic rather than a pass
  condition and is not equivalent to a user moving a real tab between foreground and background.
  Genuine visible-tab lifecycle and audio-focus validation remain in the execution queue; this
  observation is not classified as a CNA defect without that evidence.

### 2026-10-06 — Long-period persistent soundscape phasing

- Identified that the prior 10/10/12/6-second surf, breeze, jungle, and player-water loops returned
  to the same aggregate phase after 60 seconds. Retained the qualified ten-second surf and
  twelve-second jungle sources while extending breeze to eleven seconds and player water to seven.
  The 10/11/12/7-second set has a least common multiple of 4,620 seconds, so the complete persistent
  pattern now takes 77 minutes to realign. Core coverage locks that property.
- The change adds only 88,200 bytes of generated mono PCM16 memory and no packaged asset, active
  source, mixer voice, draw submission, or external license. The audio report validates all 12
  unique sources and a 2,533,544-byte PCM footprint. Peaks remain approximately 0.56--0.86,
  absolute DC remains below 0.0069, and the four loop seams span approximately 0.0036--0.0271,
  each below its own largest ordinary internal step. WAV export and independent parsing pass.
- The native suite passes 37/37 in 84.12 seconds under shared game/browser load, including 29 real
  OpenGL scenes and real X11 input. The complete Chrome 152/SwiftShader WebGL 2 harness passes all
  22 stages. The arm64 GLES 3 target builds, application lint passes, the 33,582,240-byte APK
  assembles, and its stripped library is 15,849,864 bytes; WebAssembly remains 8,435,054 bytes.
  Physical Android playback and the subjective 20-minute headphone/speaker route remain open.
- The world fingerprint remains `d83ecd714e10fc9f`. CNA and Sharp Runtime were not modified, and no
  dependency defect was found.

### 2026-10-06 — Surface-visible monkey face and shade readability

- Inspected a fresh deterministic native monkey capture and found that the nominal eye geometry
  sat behind the front surface of the head. The eye draw call existed, but the eyes were effectively
  inside the procedural skull. This was game-side mesh placement, not a CNA skinning defect.
- Added a shallow light upper-face mask to the existing face buffer, moved both eyes forward while
  keeping their backs embedded, and added a restrained dark nose to the existing eye-detail buffer.
  The dark eye material receives a modest glossy response, and all four body/face palettes are
  slightly brighter so shaded animals remain distinct from brown trunks. The accepted native
  close-up shows the mask, one profile eye, muzzle, and nose without intersection or z-fighting.
- The shared near-detail buffers add 86 vertices and 122 triangles per visible near monkey. There
  are no new textures, instances, animation bones, effects, active agents, or draw calls; all far
  meshes and the 92 m near/far threshold are unchanged.
- The native suite passes 37/37 in 82.73 seconds under shared game/browser load, including 29 real
  OpenGL scenes and real X11 input. The complete Chrome 152/SwiftShader WebGL 2 harness passes all
  22 stages and renders two near / one far monkey through the seven-draw existing path. The arm64
  GLES 3 target builds, application lint passes, the 33,582,736-byte APK assembles, and its stripped
  library is 15,850,360 bytes; WebAssembly is 8,435,697 bytes. Android appearance remains
  unqualified without stable hardware/KVM execution.
- CNA and Sharp Runtime were not modified, and no dependency defect was found.

### 2026-10-06 — Radially varied, banked, intermittently gliding bird flight

- Extended every authored shore/canopy/high bird route with a separate slow radial-drift phase,
  frequency, and family-scaled 7--17 m amplitude. The existing two-frequency heading wander,
  vertical clearance, speed variation, and bounded airspace remain authoritative; routes no longer
  converge on one fixed-radius orbit over long observation.
- Added smoothly approached route/steering-derived bank angles bounded to 0.34 radians and
  deterministic, smoothly entered intermittent glide windows. The flap clock continues through a
  glide, so resuming a wing stroke does not reset all birds to a shared phase. `BirdRenderer`
  rotates the existing near and far body/wing bases by the bank pose; population, triangles,
  dynamic-buffer capacity, culling, simulation tiers, and the one-draw ceiling are unchanged.
- Extended the simulated 20-minute regression to require finite bounded bank/glide state, a strong
  glide, both bank directions, both wing-stroke extremes, unchanged terrain clearance, bounded
  radii, and exact repeatability. Native and Chromium bird captures were inspected and show the
  banked silhouette. A new startup diagnostic is mandatory in the native bird capture and both the
  Web lifecycle and dedicated bird gates.
- A heavily loaded SwiftShader pass proved every renderer/scene gate, including the bird gate, but
  missed fixed-duration `B`, `F1`, `F4`, and shoreline input sampling. The browser harness now
  waits for authoritative game diagnostics with bounded retries and holds shoreline traversal until
  the actual first transition accent. The resulting complete Chrome 152/WebGL 2 harness passes all
  22 stages; genuine tab background/foreground recovery remains diagnostic-only and unqualified.
- The correctly isolated native suite passes 37/37 in 74.81 seconds, including 29 real OpenGL
  scenes and real X11 input. The arm64 GLES 3 target builds, the 33,585,008-byte APK assembles,
  application lint passes, its stripped library is 15,852,632 bytes, and WebAssembly is 8,437,245
  bytes. Android appearance remains unqualified without stable hardware/KVM execution. The world
  fingerprint remains `d83ecd714e10fc9f`.
- CNA and Sharp Runtime were not modified, and no dependency defect was found.

### 2026-10-06 — Depth-following, banked aquatic wildlife silhouettes

- Added bounded, smoothly approached vertical-velocity pitch and heading-error banking to every
  fish and to the occasional shark. Fish pitch stays within 0.22 radians and bank within 0.28;
  shark pitch stays within 0.18 and bank within 0.24. The CPU basis composes both poses with the
  existing yaw/body/tail animation through portable `BasicEffect`, so no renderer-specific shader
  path or new draw submission is required.
- Reworked both fish and shark tail fins into forked silhouettes at near and far LOD, then added
  restrained eye and gill triangles to the near LOD. The existing single dynamic buffer and one-draw
  ceiling per species remain unchanged; the exact near worst case is 66 vertices per animal. Fresh
  native close captures were inspected and show readable heads, tail forks, and varied non-level
  silhouettes without obvious intersection.
- Extended deterministic long-session core coverage to compare the new pose state exactly, reject
  non-finite/out-of-range values, and require upward/downward pitch plus both fish bank directions
  and visible shark banking. Native and Web lifecycle/fish/shark gates now require the explicit
  aquatic-pose startup diagnostic.
- The isolated native suite passes 37/37 in 70.92 seconds, including 29 real OpenGL scenes and real
  X11 input. The complete Chrome 152/SwiftShader WebGL 2 harness passes all 22 lifecycle, renderer,
  scene, and input stages; dedicated fish and shark captures were inspected. The arm64 GLES 3
  target builds, Gradle APK assembly and application lint pass, WebAssembly is 8,452,228 bytes,
  the stripped Android library is 15,857,496 bytes, and the APK is 33,589,872 bytes. Android
  appearance and timing remain unqualified without KVM or a physical device.
- The deterministic world fingerprint remains `d83ecd714e10fc9f`; population, placement, collision,
  activation tiers, and offshore constraints are unchanged. CNA and Sharp Runtime were not
  modified, and no dependency defect was found.

### 2026-10-06 — Calm monkey player-aware skeletal head posing

- Added a renderer-independent, distance-faded look angle for calm `Idle` and `Observe` monkeys
  when the player is 7.5--30 m away. It approaches at 0.75 radians per second, remains within
  0.34 radians, relaxes during locomotion, and yields to the existing brief retreat inside 7.5 m;
  no hostile or interaction system was introduced.
- `MonkeyRenderer` composes the look into the existing head-bone local transform after normal clip
  sampling or crossfade blending, then rebuilds the same fourteen-matrix world/skin palette. Agent
  count, geometry, clips, effects, textures, draw counts, and the portable `SkinnedEffect` path are
  unchanged. Core coverage requires smooth attention to both sides, exact determinism, finite pose,
  and the bound throughout the simulated 20-minute wildlife session.
- The close-review viewpoint now supplies a virtual observer at the same bearing but outside the
  retreat radius, so the QA camera can remain near the face without making its subject immediately
  run away. The deployable Web test page uses the fixed monkey view without the F1 panel because
  first-draw scene populations are logged independently. Fresh native OpenGL 3.3 and actual
  Chrome/WebGL 2 captures were inspected and match on the calm frontal pose and readable face.
- The isolated native suite passes 37/37 in 71.66 seconds, including 29 real OpenGL scenes and real
  X11 input. The complete Chrome 152/SwiftShader WebGL 2 harness passes all 22 stages. The arm64
  GLES 3 target builds, Gradle APK assembly and application lint pass, WebAssembly is 8,453,927
  bytes, the stripped Android library is 15,858,992 bytes, and the APK is 33,591,368 bytes. Android
  appearance and attention timing remain unqualified without KVM or a physical device.
- The deterministic world fingerprint remains `d83ecd714e10fc9f`; placements, collision, home
  ranges, and animation clip data are unchanged. CNA and Sharp Runtime were not modified, and no
  dependency defect was found.

### 2026-10-06 — Coherent grass biome colour and Wayland-safe input QA

- A fresh native beach/jungle/east-clearing/west-clearing/water audit found that broad interior
  grass fields retained one colour even where the terrain material already distinguished routes
  and authored clearings. Added one deterministic 0--1 cool-to-warm value per grass anchor. Shore
  distance now transitions warm coastal turf into cooler interior growth, non-clearing reserved
  routes remain lightly sun-dried, the east clearing is olive-gold, and the leaf-rich west clearing
  is deeper and cooler. A smooth inner/outer clearing blend avoids a material ring.
- Extended the existing portable instanced shader with one 32-bit attribute and coherent near/far
  palettes. The far-ground merge consumes the same value, so identity does not disappear at the
  82--125 m transition. Population remains exactly 18,556; placement, geometry, cell culling,
  near/far LOD, effects, and draw counts are unchanged. Core coverage locks finite normalized
  values, exact repeatability, minimum clearing populations, and a strong east/west average-colour
  separation. The intentional world-data fingerprint change is `d83ecd714e10fc9f` to
  `b6aea3886cdfe525`.
- Native before/after captures of close grass, jungle, and both clearings were inspected. The
  accepted tuning is visible without making the scene neon or introducing a hard circular edge.
  Actual Chrome/SwiftShader WebGL 2 clearing and close-grass captures match the OpenGL 3.3 result;
  lifecycle, overview, beach, and grass gates now require the new diagnostic. One isolated native
  six-view safety suite measured 1.2069--2.3128 ms mean and 1.6855--3.2074 ms p95 across its fixed
  scenes; this is a gross-regression guard rather than a before/after GPU-cost claim.
- The full native run exposed an environment-sensitive QA problem rather than a game or framework
  regression: SDL correctly selected native Wayland, while the deliberately X11-only `xdotool`
  harness could neither discover nor focus that surface. The game-side script now explicitly uses
  SDL X11 and, under a Wayland session, automatically nests the synthetic-input test in Xvfb rather
  than attempting to steal the operator's real keyboard focus. The final sequential native suite
  passes 37/37 in 62.43 seconds, including 29 real OpenGL scenes and the isolated real SDL/XTest
  input path.
- The complete Chrome 152/SwiftShader WebGL 2 harness passes all 22 stages; WebAssembly is
  8,455,363 bytes. The arm64 GLES 3 target builds, the 33,592,792-byte APK assembles, application
  lint passes, and its stripped library is 15,860,416 bytes. `/dev/kvm` is absent and no ADB device
  is attached, so current Android runtime appearance and timing remain unqualified; root access
  would not create a host KVM device. CNA and Sharp Runtime remain unmodified, and no dependency
  defect was found.

### 2026-10-06 — Current Android GLES 3 first-frame qualification

- Added a project-owned Android runtime gate that verifies device readiness, installs the selected
  APK, clears logcat, launches the activity, follows only the game PID, requires the `OPENGLES3`,
  biome-aware grass, and first-draw diagnostics, rejects fatal/shader failures, and records both
  logcat and a screenshot. PID filtering avoids repeatedly transferring the entire system log from
  an already CPU-bound software AVD. A configurable ten-minute deadline covers this environment
  without weakening failure detection on faster devices.
- SDLActivity does not expose C++ stdout through Android logcat. Added a narrow game-side diagnostic
  mirror using Android's public log API for the three high-value milestones; Linux and Web retain
  their existing stdout behaviour. The implementation and Android `liblog` linkage live entirely
  in island-simulator. CNA and Sharp Runtime remain clean, read-only, and unmodified.
- Rebuilt the x86_64 debug APK and ran it on the API 35 `Medium_Phone` AVD under QEMU TCG plus
  SwiftShader GLES, with no host `/dev/kvm`. A fresh cold boot took 229 seconds. At a temporary
  800 by 360 guest resolution, SDL loaded both native libraries, created an RGBA8888 surface, and
  the current complete scene reported biome grass, renderer `OPENGLES3`, and a completed first
  draw. Measured application phases were 157126.852 ms world generation, 256204.401 ms content
  creation, 7708.990 ms first draw, and 421449.067 ms total.
- The saved image visibly contains the current island and touch HUD behind Android's one-time
  fullscreen help overlay. It is valid integrated GLES 3 evidence but not a subjective per-layer,
  input, audio, lifecycle, frame-pacing, memory, or thermal qualification. System/System UI ANRs
  remained frequent, and attempting to restore full resolution caused the guest `system_server`
  to lose its window/activity services. A clean headless boot subsequently restored and verified
  the original 1080 by 2400 size and 420 dpi before shutdown. Guest root would not supply missing
  host virtualization.
- Gfxstream logged 27 `GL_INVALID_OPERATION` results at uniform-location queries while the correct
  frame still completed and no shader compile/link failure appeared. The observation remains a
  renderer-environment triage item, not a CNA/Sharp Runtime bug entry, until it can be reproduced
  on stable KVM-backed or physical GLES 3 hardware.
- Post-change validation passes all 37 native tests in 71.80 seconds, including 29 real OpenGL 3.3
  scenes and isolated SDL/XTest input. The complete Chrome 152/SwiftShader WebGL 2 harness passes
  against the rebuilt 8,458,494-byte WebAssembly module. The default single-ABI arm64 package was
  restored after emulator testing; application lint passes, its stripped `libmain.so` is
  15,861,744 bytes, and the APK is 18,672,028 bytes. Removed approximately 1 GB of superseded
  x86_64 NDK intermediates while retaining the runtime log/screenshot evidence and final arm64
  artifact. The project-owned workspace now occupies approximately 3.7 GB.

### 2026-10-06 — Sparse sun-cured grass-tip variation

- A close native and browser audit found that the broader biome-aware colour field worked, but
  individual blade tips remained too uniform at first-person height. Added a sparse deterministic
  sun-cured subset to the existing shared near/far mesh vertex colours. The tint increases smoothly
  toward each selected tip while preserving green roots and folded-side lighting.
- A first shader-based experiment was rejected because its subtle result did not justify permanent
  per-vertex runtime arithmetic. The accepted CPU mesh-build path changes only static packed
  colours: geometry, textures, instance records, populations, culling, shader work, and draw counts
  remain unchanged. Native close and full-scene captures plus the actual Chrome/SwiftShader WebGL 2
  grass capture were inspected and agree on the restrained result.
- A native six-view gross-regression suite recorded 1.4789--2.9033 ms mean and 1.8064--3.7133 ms
  p95 across its fixed scenes while the visible operator game shared the GPU. Differences from the
  immediately preceding experimental run were mixed, as expected for a static colour-only change;
  no runtime performance claim is inferred. The complete native suite passes 37/37 in 71.86
  seconds, including 29 OpenGL scenes and isolated SDL/XTest input. The complete Chrome 152 WebGL 2
  harness passes and requires the new diagnostic.
- A clean arm64-v8a Gradle build and app lint pass produced an 18,672,364-byte v2-signed APK with a
  15,862,080-byte stripped `libmain.so`; an immediate no-change rebuild stayed byte-for-byte the
  same size. WebAssembly is 8,458,810 bytes. The earlier 34,544,728-byte incremental APK was a stale
  packaging artifact and was superseded by the clean reproducible package. Android visual tuning
  of the new static colours remains open for physical or KVM-backed hardware.
- CNA and Sharp Runtime remain unmodified. The deterministic world fingerprint, all populations,
  and placement data are unchanged, and no qualifying dependency defect was found.

### 2026-10-06 — Progressive grass LOD and compact Android packaging

- Replaced the single 82 m whole-cell grass mesh switch with a renderer-independent 70--94 m
  smoothstep population split. Each cell's existing instance records are deterministically ordered
  once, then a near prefix and far suffix are rendered through the same buffer. Individual patches
  change mesh progressively as the viewer moves; no instance field, shader branch, geometry,
  texture, placement, population, culling radius, or world fingerprint changed.
- Pure cells retain one submission. Only a cell inside the transition band uses two submissions.
  The dedicated close view has 2,827 visible patches across 15 cells: 1,318 near, 1,509 far, four
  mixed cells, and 19 grass draws. F1 now exposes near/far/mixed counts. Core coverage checks empty,
  endpoint, midpoint, conservation, and monotonic split behaviour across 97 transition samples.
- Native OpenGL 3.3 and actual Chrome 152/SwiftShader WebGL 2 captures were inspected and match
  those exact counts. This also validates the nonzero per-instance `VertexBufferBinding` offset on
  both renderers. The Web harness requires the explicit LOD diagnostic and nonzero near, far, and
  mixed populations. The full browser harness passes all 22 stages.
- The six-view native debug safety suite recorded 1.4942--2.8983 ms mean and 1.8951--3.6413 ms p95.
  Representative fixed views add zero to five total scene submissions; comparisons with the prior
  colour-only run are mixed and approximately within one percent. The full native suite passes
  37/37 in 41.20 seconds with two-way test parallelism, including 29 actual OpenGL scenes and
  isolated SDL/XTest input. WebAssembly is 8,468,671 bytes.
- The arm64-v8a GLES 3 target builds and app lint passes. Its stripped `libmain.so` is 15,888,768
  bytes. Reproducing the native-source rebuild showed that incremental APK packaging retained one
  superseded native library as unreachable archive data, inflating the signed file from 18,699,052
  to 34,571,416 bytes. The project Gradle wrapper now removes only the generated debug APK before
  explicit debug packaging; native intermediates remain incremental, repackaging takes under a
  second, and repeated output is a valid v2-signed 18,699,052-byte single-ABI APK.
- The software AVD was not rerun: its already-qualified current GLES 3 first-frame path took about
  seven minutes without host KVM, while this milestone changes buffer selection rather than shader
  code. Physical or KVM-backed Android visual/performance evidence remains required. CNA and Sharp
  Runtime remain unmodified, and no qualifying dependency defect was found.

### 2026-10-06 — Lightweight middle grass LOD

- Added a ten-blade, three-control-level, 40-triangle middle cluster between the existing
  28-blade/224-triangle near and five-blade/20-triangle far meshes. The same deterministically
  ordered instance buffer now transitions near-to-middle across 62--78 m, remains wholly middle
  through 86 m, and transitions middle-to-far through 102 m. Non-adjacent near/far meshes are
  never active together, and a cell still needs at most two grass submissions.
- An initial 12-blade/four-level, 72-triangle middle prototype was rejected after two six-view
  suites repeatedly made the traversal mean 12--18 percent slower. The accepted mesh reduced both
  geometry and transition widths. In the close QA view it renders 957 near, 637 middle, and 1,233
  far patches across 15 cells, with six mixed cells and 21 draws. Its 264,508 visible grass
  triangles are about 19 percent below the preceding two-tier view's 325,412 despite the smoother
  two-step silhouette progression.
- Two accepted-mesh six-view debug suites recorded 1.3194--3.0860 ms mean and 1.6323--4.3729 ms
  p95 across their fixed scenes while the operator game shared the GPU. Noise is substantial, but
  neither traversal repeated the rejected regression: both improved from the preceding 2.8983 ms
  mean to 2.4887--2.6431 ms as its visible grass geometry fell by about 32 percent. These are
  safety results rather than a hardware performance budget.
- Core tests cover empty/invalid/unbounded input, exact near/middle/far and both midpoint splits,
  population conservation, monotonic extreme-tier movement, and exclusion of simultaneous
  non-adjacent tiers across 161 distance samples. The exact-source native suite passes 37/37 in
  40.55 seconds, including 29 real OpenGL scenes and isolated SDL/XTest input.
- The complete Chrome 152/SwiftShader WebGL 2 harness passes all 22 stages. Its inspected grass
  capture exactly matches native populations, mixed-cell count, and draw count, and requires the
  three-tier diagnostic. WebAssembly is 8,470,486 bytes. The arm64 GLES 3 target builds and app
  lint passes; the v2-signed compact single-ABI APK is 18,700,484 bytes and its stripped
  `libmain.so` is 15,890,200 bytes. Physical-Android appearance and cost remain open.
- Population, placement, instance layout, culling radius, shaders, and the deterministic world
  fingerprint are unchanged. CNA and Sharp Runtime remain unmodified, and no qualifying
  dependency defect was found.

### 2026-10-06 — Narrow irregular primary-footpath wear

- A comparative eight-view visual audit found that the main interior path still read as an overly
  broad, materially flat strip. Reduced its varying visible half-width from 2.05--2.77 m to
  1.52--2.16 m while retaining the 6.2 m vegetation reservation, traversal, collision, and short
  grass. Stronger fine edge breakup, a subtly offset compacted centre, and shaded-to-sun-dry earth
  variation now return the sides to ordinary substrate without turning the route into a road.
- Rejected an initial separate path-noise evaluation: the extra four-corner field was almost
  indistinguishable in captures and not justified for mobile fragment work. The accepted path
  reuses the terrain shader's existing medium and fine fields, adding no noise octave, geometry,
  texture, CPU placement, instance data, or draw submission. Two final traversal checks measured
  2.5100--2.7317 ms mean / 3.4736--3.6927 ms p95 while the operator game shared the GPU; this is a
  gross-regression guard, not an isolated speedup claim.
- Native OpenGL 3.3 before/after captures and the actual Chrome 152/SwiftShader WebGL 2 result were
  inspected at 1280 by 720 and agree on the narrower treatment. The Web harness now requires its
  explicit diagnostic in lifecycle, beach, and jungle stages and passes all 22 stages. The final
  native suite passes 37/37 in 39.03 seconds, including 29 real OpenGL scenes and isolated
  SDL/XTest input.
- The arm64 GLES 3 target builds and application lint passes. The v2-signed, single-ABI APK is
  18,700,980 bytes, its stripped `libmain.so` is 15,890,696 bytes, and WebAssembly is 8,471,044
  bytes. The software AVD was not rerun for a material-only shader change after its current GLES 3
  first frame had already required about seven minutes; physical-device appearance and cost remain
  open. CNA and Sharp Runtime remain unmodified, and no qualifying dependency defect was found.

### 2026-10-06 — Reused crossing-wave ocean colour structure

- The post-path sixteen-view audit found the surrounding ocean coherent but too uniform through
  broad near/middle-distance areas. Promoted the two crossing-wave fields already evaluated for
  broad foam into a restrained bright/dark subsurface colour term. It favours shallow,
  non-reflective water and fades from 135 to 420 m, preserving the calm hazed horizon while making
  animated surface movement easier to read from shore, swimming, fish, and reef views.
- The accepted GLSL 330 / GLSL ES 300 path adds no sinusoid, texture, vertex, index, instance, CPU
  update, allocation, or draw submission. It moves the existing crossing-wave calculation earlier,
  reuses the already-required camera distance, and adds bounded colour arithmetic plus one
  smoothstep. Two weaker amplitudes were inspected and rejected because their static result was
  almost imperceptible.
- Six fixed native OpenGL 3.3 ocean/wildlife captures and actual Chrome 152/SwiftShader WebGL 2
  near-surface/current captures were inspected at 1280 by 720 and retain the same composition. The
  Web harness requires the new diagnostic in lifecycle, overview, beach, water, and current stages
  and passes all 22 stages. The final native suite passes 37/37 in 36.30 seconds, including 29 real
  OpenGL scenes and isolated SDL/XTest input.
- One shared-GPU six-view safety suite measured 1.0604--2.3070 ms mean / 1.3832--3.4943 ms p95.
  Two ocean-dominant overview checks closely repeated at 1.2114--1.2183 ms mean /
  1.4792--1.5122 ms p95. These results reject a gross regression but are not an isolated speedup
  claim. The arm64 GLES 3 target builds and app lint passes; the v2-signed single-ABI APK is
  18,701,412 bytes, its stripped `libmain.so` is 15,891,128 bytes, and WebAssembly is 8,471,519
  bytes. The software AVD was not rerun; physical-device appearance/cost remain open. CNA and Sharp
  Runtime remain unmodified, and no qualifying dependency defect was found.

### 2026-10-06 — Reduced and CPU-faceted fish geometry

- The wildlife audit found that close fish retained an overly flat body read despite their existing
  forked tails, dorsal/pectoral fins, eyes, gills, pitch, and banking. Added fixed-sun CPU face
  colours to the closed body triangles, producing readable low-poly volume through the existing
  `BasicEffect` vertex-colour path without a shader, normal stream, texture, or additional draw.
- Removed the exact reverse-winding copies of every thin fin/tail triangle. `FishRenderer` already
  selects `RasterizerState::CullNone`, so one surface is visible from both sides and the coincident
  copy only duplicated transform/raster work. Near geometry falls from 66 to 51 vertices
  (22 to 17 triangles), far geometry from 24 to 12 vertices (eight to four triangles), and the
  48-fish worst-case dynamic buffer from 3,168 to 2,448 vertices. Population, simulation, culling,
  LOD distances, animation, palettes, and the one-draw ceiling are unchanged.
- Native OpenGL 3.3 before/after and actual Chrome 152/SwiftShader WebGL 2 close-school captures were
  inspected at 1280 by 720 and retain every fin/tail silhouette with clearer body faceting. Native
  and browser lifecycle/fish gates require the exact 51/12-vertex diagnostic. The complete Web
  harness passes all 22 stages; the final native suite passes 37/37 in 36.72 seconds, including 29
  actual OpenGL scenes and isolated SDL/XTest input.
- Two shared-GPU fish-view safety runs, each with six near fish in one draw, measured
  1.6764--1.7648 ms mean / 2.1861--2.3527 ms p95. These are functional cost checks rather than a
  before/after speedup claim. The arm64 GLES 3 target builds and app lint passes; the v2-signed
  single-ABI APK is 18,703,060 bytes, its stripped `libmain.so` is 15,892,776 bytes, and
  WebAssembly is 8,472,951 bytes. Physical-Android appearance/cost remain open. CNA and Sharp
  Runtime remain unmodified, and no qualifying dependency defect was found.

### 2026-10-06 — Correct first-person controls and reduced shark geometry

- Direct operator play found three real usability defects: A/D followed the opposite visible side,
  arrows were unmapped, and starting with W before clicking left the mouse uncaptured so forward
  movement could not turn. Added renderer-independent `FirstPersonInput`: WASD and arrows share
  digital axes, touch remains additive, yaw is applied before physics, and the perpendicular basis
  now matches visible screen-right under the existing `CreateLookAt` convention. The debug free
  camera uses the same sign. Core tests lock forward/right/left/changed-yaw results and reject
  non-finite analog inputs.
- Either Shift now requests a 6.5 m/s land run above the existing 4.3 m/s walk, with a modestly
  higher acceleration ceiling; swimming remains 2.15 m/s. Linux can acquire relative mouse mode
  on the first movement-key edge as well as click, suppresses the stale absolute cursor sample on
  the acquisition frame, and applies mouse yaw before resolving held forward input. Escape now
  reacts to edges, so one press releases capture and a second press exits as documented.
- Added `IslandGamePlayerInput`, a real isolated SDL/XTest gate that requires keyboard mouse
  capture, simultaneous W plus two relative turns, correct D and Right-arrow strafing, Shift+Up
  speed above the walking ceiling, and distinct release/exit edges. The complete native suite
  passes 38/38 in 51.55 seconds, including 30 display-backed tests and two isolated input tests.
  Actual Chrome 152/SwiftShader WebGL 2 passes all 22 stages; its JSON report independently marks
  `simultaneousMoveAndLook`, `rightArrowMovedRight`, and `arrowRunWorked` true.
- Applied the fish renderer's proven cull-none cleanup to the shark and extracted shared
  `WildlifeRenderLighting`. Single-surface fins reduce near/far shark geometry from 66/30 to 51/15
  vertices while fixed-sun CPU body facets add no attribute, texture, effect, or draw. Native
  before/after captures retain the complete tail/dorsal/pectoral silhouette, and the actual Web
  shark gate requires the exact diagnostic. Two one-shark/one-draw safety runs measured
  1.4463--1.4549 ms mean, 1.9150--2.0607 ms p95, and 2.2383--2.3108 ms p99 across 85 tracked
  submissions; these are functional checks, not an isolated speedup claim.
- The arm64 GLES 3 target builds and app lint passes. The v2-signed, single-ABI APK is 18,705,724
  bytes, its stripped `libmain.so` is 15,895,440 bytes, and WebAssembly is 8,473,165 bytes. An
  immediate no-change APK assembly reproduced the same size. Host `kvm`/`kvm_amd` modules are
  loaded and the account is listed in the `kvm` group, but this shell has not inherited that group
  and `/dev/kvm` remains absent, so accelerated Android runtime qualification is still open. CNA
  and Sharp Runtime remain unmodified, and no qualifying dependency defect was found.

### 2026-10-06 — Close sand granulation and shoreline-raked ripples

- Added a close-only sand octave to the portable terrain material and a restrained broken-crest
  term driven by the authoritative signed shore distance. The latter follows the irregular beach
  instead of a world-axis pattern, fades by 92 m, and ends 14 m inland. It changes material colour
  only: no texture, mesh, instance, draw, allocation, collision, or gameplay state was added.
- Native OpenGL 3.3 and actual Chrome/SwiftShader WebGL 2 waterline and close-driftwood captures
  agree on the retained subtle sand breakup. The Chrome suite passes all 22 stages and requires
  the exact feature diagnostic. Two isolated 600-sample shore-detail safety runs retained 54
  tracked submissions and measured 1.0062--1.0528 ms mean, 1.2434--1.4131 ms p95, and
  1.5616--1.8055 ms p99; the ranges overlap the preceding integrated observation and are not an
  isolated speedup claim.
- The arm64 GLES 3 APK assembles through the shared `/rv/cnaccache`; it is 18,706,876 bytes, its
  stripped `libmain.so` is 15,896,592 bytes, and WebAssembly is 8,474,383 bytes. Actual Android
  appearance/cost remains open because `/dev/kvm` is still absent. CNA and Sharp Runtime remain
  unmodified, and no qualifying dependency defect was found.

### 2026-10-06 — Irregular faceted near-palm roots

- Replaced each near palm's five flat single-triangle root tabs with five six-triangle ridge
  wedges. Root angle, length, width, and ridge height vary deterministically by root and variant,
  avoiding both the former nearly invisible contact and the rejected regular five-point star.
  Near variants rise from 469--541 to 494--566 triangles; the far mesh, instance layout,
  population, culling, LOD thresholds, collision proxies, shaders, and draw submissions do not
  change.
- Native OpenGL 3.3 and actual Chrome/SwiftShader WebGL 2 close-palm captures agree on the retained
  grounded bases, and the Chrome suite passes all 22 stages with an exact root-wedge diagnostic.
  Two isolated 600-sample palm-view checks retained 47 near and 246 far palms in 111 tracked scene
  submissions and measured 1.6128--1.6437 ms mean, 2.1310--2.2256 ms p95, and
  2.5902--2.8770 ms p99. These guard against a gross regression rather than isolating geometry
  cost.
- The arm64 GLES 3 APK assembles through the shared `/rv/cnaccache`; it is 18,708,476 bytes, its
  stripped `libmain.so` is 15,898,192 bytes, and WebAssembly is 8,477,780 bytes. Physical-Android
  appearance/cost remains open because `/dev/kvm` is still absent. CNA and Sharp Runtime remain
  unmodified, and no qualifying dependency defect was found.

### 2026-10-06 — Accelerated Android qualification and shared terrain buffers

- The operator restored `/dev/kvm` as the host misc device `10:232`. The developer account has
  read/write access through group `kvm`, and `emulator -accel-check` reports KVM version 12 usable.
  Android Emulator 36.5.11 runs the API 35 `Medium_Phone` x86_64 AVD with `-accel on`, host-GPU
  gfxstream on the AMD Radeon 780M, and its original 1080 by 2400 / 420 dpi guest configuration.
- The accelerated x86_64 APK renders the complete current GLES 3 scene at a 2400 by 1080 landscape
  surface. Startup fell from the historical, invalid-for-performance QEMU TCG total of
  421449.067 ms to 1666.782 ms: 487.895 ms world, 3.421 ms framework, 1107.472 ms content,
  20.058 ms pre-draw, and 47.937 ms first draw. A settled snapshot reports 87.3 MiB PSS,
  167.3 MiB RSS, and 0.5 MiB swap PSS. These are identified emulator observations, not physical
  mobile budgets.
- The first accelerated capture exposed opaque white rectangles behind the touch controls.
  `TouchOverlay` had supplied straight-alpha white pixels and tints to CNA `SpriteBatch`'s default
  premultiplied blend contract. The game now premultiplies both generated circle texels and tint
  colours. Rebuilt GLES 3 captures show clean translucent rings; independently injected left-zone
  movement and right-zone relative-look gestures change player position and view. This was
  game-owned code, not a CNA defect, so `BUGS.md` remains unchanged.
- One Home/resume cycle retained the same process, destroyed and recreated the surface, paused and
  restarted the 44.1 kHz stereo AAudio stream, and reported a 77 ms hot launch / 80 ms wait. No
  fatal, shader, `GL_INVALID_OPERATION`, or earlier software-AVD uniform-query error recurred.
  Simultaneous real-finger ergonomics, swimming, audio-focus interruption, process death, thermals,
  and physical-device driver/performance coverage remain open.
- Replaced 64 duplicated-edge terrain vertex/index buffer pairs with one shared 58,081-vertex,
  115,200-triangle 16-bit mesh. The original 64 independent cell bounds and cell-ordered index
  ranges remain; consecutive visible cells coalesce into draw runs. The overview now renders 58
  visible cells in three terrain submissions. Core tests cover empty, full, and sparse run
  construction plus the portable index budget, and F1/benchmark JSON expose the actual terrain
  draw count.
- At the same accelerated Android spawn, isolated SurfaceFlinger presentation cadence changed from
  40.903 ms mean / 24.45 frames/s with 71 of 126 intervals above 40 ms to two post-change samples
  of 34.163 and 32.590 ms / 29.27 and 30.68 frames/s with 6 and 1 intervals above 40 ms. This is
  useful same-AVD directional evidence, not a physical mobile GPU claim.
- A fresh isolated Radeon 780M/OpenGL 3.3 six-view suite passes. Fixed views use 3--8 terrain draws
  for 9--58 visible cells; the 675.9635 m traversal ranges over 6--47 cells in only 2--7 terrain
  submissions. Its mean is 3.7348 ms and p95 is 7.7032 ms in the current debug scene. The actual
  Chrome 152/SwiftShader WebGL 2 harness passes all 22 stages, reports 58 cells in three draws, and
  produces an inspected seam-free overview; WebAssembly is 8,481,359 bytes.
- The final arm64-v8a APK was rebuilt through the required shared `/rv/cnaccache`, passes
  application lint, contains only arm64-v8a, and verifies with APK signature scheme v2. It is
  18,710,140 bytes and its stripped `libmain.so` is 15,899,856 bytes. The final incremental native
  build is current and `IslandCoreTests` passes in 19.19 seconds. CNA and Sharp Runtime remain clean,
  on `next`, read-only, and unmodified; no qualifying dependency defect was found.

### 2026-10-06 — Persistent Shift run toggle and faster running pace

- Replaced hold-to-run with an edge-triggered land-pace mode: either Shift key toggles running on
  the first press and restores walking on the next press. Releasing Shift no longer cancels the
  selected mode, both Shift keys share one edge state, swimming remains fixed at 2.15 m/s, and the
  debug inspection camera deliberately retains its separate hold-Shift fast-flight behaviour.
- Increased the land run ceiling from 6.5 to 8.0 m/s while retaining the 4.3 m/s walk ceiling.
  Running now adds a bounded cadence and amplitude blend to first-person camera motion so the
  faster pace remains readable instead of appearing to glide; all offsets remain inside the
  existing 10 mm lateral and 18 mm vertical comfort bounds. Core tests cover the new speed and
  stronger-but-bounded running presentation.
- The incremental Linux/OpenGL 3.3 build used the required shared `/rv/cnaccache`; core tests pass
  in 16.34 seconds. The final isolated real SDL/XTest input gate passes in 12.55 seconds and proves
  that running remains active above 7.0 m/s after Shift is released, then that a second Shift
  press restores a maximum 4.3 m/s walking pace.
- The complete actual Chrome 152/SwiftShader WebGL 2 harness passes all 22 stages. Its input report
  independently marks `runModeEnabled`, `retainedRunWorked`, `runModeDisabled`, and
  `restoredWalkWorked` true; WebAssembly is 8,482,000 bytes. The arm64-v8a GLES 3 APK rebuilds and
  application lint passes; it is 18,710,652 bytes and its stripped `libmain.so` is 15,900,368
  bytes. The updated x86_64 APK then passed the actual accelerated GLES 3 first-frame gate. Two
  separated external-keyboard Shift events produced the expected Android platform diagnostics for
  8.0 m/s run enabled followed by 4.3 m/s run disabled, qualifying the same toggle path on the
  touch-oriented target without claiming physical-device keyboard ergonomics. CNA and Sharp
  Runtime remain read-only and unmodified; no qualifying dependency defect was found.

### 2026-10-06 — Pre-graphics command-line failure validation

- Added `IslandCommandLineValidation`, a display-independent test that invokes the real executable
  for eight invalid command lines. It requires exit code 2, the specific actionable stderr text,
  and an empty stdout stream for unknown options, three missing option values, an unknown
  viewpoint, a benchmark/debug conflict, and traversal requested without a benchmark destination.
- The gate passes in 0.08 seconds and proves every case fails before constructing `IslandGame` or
  initializing graphics. This advances malformed-configuration coverage without pretending that
  malformed procedural content, shader packages, animation data, or audio inputs are now covered.
  CNA and Sharp Runtime remain read-only and unmodified; no dependency defect was involved.

### 2026-10-06 — Finite simulation and procedural animation validation

- Hardened `PlayerMotion`, bird, fish, shark, and monkey updates against non-finite time, movement,
  placement, and observer values. Invalid time is a zero-length step, invalid direction is neutral,
  invalid player placement retains the last valid coordinate, and invalid wildlife distance fails
  closed to the dormant tier. Debug builds assert finite player/wildlife poses, bounded pose values,
  complete population accounting, valid palette/school indices, and monkey home-range invariants.
- Added fail-fast startup validation for the original procedural monkey assets. The generated
  fourteen-bone hierarchy, bind/inverse-bind arrays, optional root prefix, required clips, joint-
  palette target space, track indices, ordered key times, and finite key transforms must agree.
  Face/eye plus all three near/far silhouette meshes must be nonempty triangle lists whose every
  16-bit index addresses an uploaded vertex. Crossfade and recomposed skin palettes retain debug
  size assertions at their unchecked indexing boundaries.
- Core and command-line tests pass in 15.44 seconds, including explicit poisoned-input regressions.
  Isolated actual OpenGL 3.3 smoke and monkey capture pass. The complete Chrome 152/SwiftShader
  WebGL 2 harness passes all 22 stages and WebAssembly is 8,488,549 bytes.
- The arm64 GLES 3 target and application lint pass. A fresh x86_64 APK then constructed the full
  scene and passed the actual KVM/host-GPU GLES 3 first-frame gate at 1465.892 ms total startup; its
  2400 by 1080 screenshot was inspected and retains clean touch-overlay blending. The emulator was
  stopped afterward and the default v2-signed, arm64-only APK restored at 18,731,100 bytes with a
  15,920,816-byte stripped `libmain.so`. CNA and Sharp Runtime remain clean, on `next`, read-only,
  and unmodified; no qualifying dependency defect was found.

### 2026-10-06 — Bounded terrain-query domain

- Defined a public one-million-metre absolute horizontal query limit, far outside the approximately
  500 m island and all distant scenery. `Height`, `Sample`, `SignedShoreDistance`, and `IsLand` now
  fail closed for non-finite or larger coordinates to finite deep seabed, a negative shore distance,
  an upward normal, zero slope, and a water classification. This prevents `floor(NaN/Inf)` or an
  out-of-range floating-point value from reaching the procedural noise cell's `int32_t` conversion.
  Non-finite shoreline angles use the deterministic zero-angle profile; ordinary `atan2` angles
  avoid an unnecessary range-reduction call.
- Core regression coverage exercises `NaN`, infinity, maximum float, and the explicit domain edge;
  the debug suite passes in 18.60 seconds with private noise-domain assertions active. Linux
  OpenGL 3.3, WebGL 2, and arm64 GLES 3 targets compile from the same implementation using the
  required shared ccache and two workers. WebAssembly is 8,488,514 bytes; the v2-signed arm64-only
  APK is 18,732,292 bytes with a 15,922,008-byte stripped `libmain.so`, and application lint passes.
  Runtime behaviour for valid world coordinates is unchanged. CNA and Sharp Runtime remain clean,
  read-only, and unmodified.

### 2026-10-06 — Complete procedural index validation

- Added renderer-independent `ValidateProceduralTriangleMeshOrThrow` to reject zero vertices,
  empty/incomplete triangle lists, more than 65,536 addressable vertices, graphics-API-sized index
  overflow, and every out-of-range 16-bit vertex index with a subsystem-labelled diagnostic.
  Unit coverage proves both the maximum legal index and all practical malformed boundaries.
- Applied it immediately before GPU buffer creation for the terrain, ocean, shoreline ribbon, sky
  dome, three grass LODs, both palm LODs, all canopy/understory/bush/ground-plant LODs, every
  rock/log/driftwood/beach-detail/frond mesh, and monkey body/face/eye meshes. Non-indexed dynamic
  wildlife/contact batches are outside this index contract. The validation runs only during
  content creation and adds no frame-loop work.
- The core suite passes in 16.92 seconds and actual isolated OpenGL 3.3 startup constructs every
  current mesh successfully. The complete Chrome 152/SwiftShader WebGL 2 harness again passes all
  22 stages; WebAssembly is 8,491,201 bytes. The arm64 GLES 3 target and app lint pass, and the
  v2-signed arm64-only APK is 18,736,332 bytes with a 15,926,048-byte stripped `libmain.so`.
  The immediately preceding KVM/GLES 3 complete-scene run remains the Android runtime evidence;
  the emulator was not needlessly restarted for this platform-independent startup check. CNA and
  Sharp Runtime remain clean, read-only, and unmodified.

### 2026-10-06 — Sparse near-field dry-beach substrate

- A clean six-view composition audit found that exposed dry sand remained nearly uniform at
  first-person scale even after broad variation, close granulation, and shore-following ripples.
  Added two deterministic jittered grids of soft-edged elliptical dark-pebble and pale-shell
  flecks. They run only on dry sand within 58 m, fade with distance and shore position, and add no
  texture, mesh, instance, allocation, collision rule, gameplay state, or draw submission.
- The same audit found the existing close jungle-floor microvariation and leaf litter too faint in
  broad gaps. Raised only their bounded near-field contrast while retaining the original density,
  material palette, distance/slope gates, and zero-submission design.
- Core tests pass. Seven actual OpenGL 3.3 material captures pass; the driftwood
  and fallen-frond views were inspected at 1280 by 720 and show sparse irregular substrate without
  a visible grid. The complete Chrome 152/SwiftShader WebGL 2 harness passes all 22 stages, requires
  the exact new feature diagnostic, and its corresponding captures visually match native.
  WebAssembly is 8,492,759 bytes.
- The arm64 GLES 3 APK assembles through the required shared `/rv/cnaccache`, application lint
  passes, and the v2-signed arm64-only package is 18,737,820 bytes with a 15,927,536-byte stripped
  `libmain.so`. Close material appearance and fragment cost on a physical Android device remain
  explicitly unqualified. CNA and Sharp Runtime remain clean, read-only, and unmodified; no
  qualifying dependency defect was found.

### 2026-10-06 — Authored cove driftwood landmark

- Composed three non-solid driftwood pieces into a recognizable arc at the shared `-2.35` radian
  cove transect. The group uses all three existing silhouettes with varied length, radius, angle,
  and shore-relative orientation. Exact terrain support keeps each piece grounded without adding
  assets, geometry variants, shader branches, collision proxies, or a new draw type.
- Core coverage locks the exact landmark population and silhouettes, deterministic state,
  dry-beach placement and full-footprint constraints, reserved-route clearance, tree clearance,
  and 2.4 m separation from every other driftwood piece. The world report remains valid with
  fingerprint `97a64d24063a5a74`; only driftwood population changes from 28 to 31, while the
  820-primitive / 481-cell collision field is unchanged.
- Reframed the deterministic driftwood QA camera around the landmark centroid. Repeated isolated
  Radeon 780M/OpenGL 3.3 captures pass and the inspected image shows all three pieces without a
  blocked viewpoint. The complete actual Chrome 152/SwiftShader WebGL 2 suite passes all 22 stages
  and its inspected cove composition matches native. The browser input gate now confirms each
  synthetic Shift toggle from game output, preventing a short edge from falling between heavily
  loaded SwiftShader frames without weakening the run/walk assertions.
- WebAssembly is 8,494,145 bytes. The arm64 GLES 3 APK builds through the required shared
  `/rv/cnaccache`, passes application lint, contains only arm64-v8a libraries, and verifies with
  APK signature scheme v2. It is 18,740,172 bytes with a 15,929,888-byte stripped `libmain.so`.
  A current x86_64 package then passed the actual KVM/host-GPU GLES 3 gate in 1569.704 ms total and
  its 2400 by 1080 overview was inspected before restoring the arm64 package. This qualifies
  complete-scene startup but not the cove's close appearance or cost on physical Android hardware.
  The pinned CNA and Sharp Runtime exports used by the game remained read-only and unchanged; no
  qualifying dependency defect was found.

### 2026-10-06 — CNA `next` baseline refresh

- Advanced the reproducible CNA pin from `39e84add777730969f58074fa1ffa010dc5c7d2c` to the then-current
  `next` commit `fe20e3e9c23b5353a5b63bbda36f236d799d6612`. The 11-commit delta is confined to
  Windows CI, Direct3D 11 validation, platform documentation, and test-only access; it does not
  change an island-simulator renderer implementation or public API. Sharp Runtime remains pinned
  to its current `next` commit `21c06910f7cf887cceebb45200e07b43dd45fef4`.
- Materialized CNA from the exact committed tree rather than its shared checkout. That checkout
  acquired concurrent uncommitted renderer-retirement work during qualification; island-simulator
  neither consumed nor modified those files. The game-owned export and every build stayed on the
  reviewed commit, preserving the dependency boundary and reproducibility.
- The complete isolated Weston/Xwayland Linux/OpenGL 3.3 suite passes 39/39 tests in 59.67 seconds,
  including every render capture, both benchmarks, core/asset/report validation, free-camera input,
  and real SDL/XTest player input. The actual Chrome 152/SwiftShader WebGL 2 harness passes all 22
  lifecycle, scene, visual, audio-initialization, and interaction stages. WebAssembly remains
  8,494,145 bytes.
- The arm64 GLES 3 APK rebuilds and application lint passes. A fresh x86_64 package then installed
  on the API 35 KVM/host-GPU emulator and completed a current `OPENGLES3` first frame in
  2003.189 ms total; its 2400 by 1080 screenshot was inspected and retained clean translucent touch
  controls. The restored arm64-only APK remains 18,740,172 bytes with a 15,929,888-byte stripped
  `libmain.so` and verifies with signature scheme v2. All compilation used `/rv/cnaccache` and no
  more than two workers per build. No game workaround or qualifying dependency defect was found.

### 2026-10-06 — Finite-safe soundscape control state

- Hardened every renderer-independent soundscape model and scheduler against non-finite control
  input. Invalid time is a zero-length update; invalid shoreline position produces a finite silent
  target; invalid water depth, speed, or current produces the finite dry/calm mix; invalid crossing
  state emits no accent; and invalid listener/source coordinates cannot request a positional call.
  The next valid update resumes without a poisoned mix or corrupted water-transition state.
- Added focused core regressions for ambient and water fades, entry/exit hysteresis, wave timing,
  wildlife timing, and positional eligibility. The core suite passes in 16.50 seconds. The audio
  report still validates the same 12 distinct procedural clips and 2,533,544 PCM payload bytes;
  generation and normal finite-input mix behaviour are unchanged.
- The actual Chrome 152/SwiftShader WebGL 2 harness passes all 22 renderer, lifecycle, scene,
  input, and first-gesture audio stages after the change; WebAssembly is 8,495,122 bytes. The
  arm64-v8a GLES 3 APK rebuilds through `/rv/cnaccache`, passes application lint, contains only the
  requested ABI, and verifies with signature scheme v2. It is 18,740,844 bytes with a
  15,930,560-byte stripped `libmain.so`. Android runtime was not needlessly repeated for this
  renderer-independent control change; the immediately preceding current-CNA-pin KVM/GLES 3
  complete-scene run remains the runtime evidence. CNA and Sharp Runtime remained read-only and
  unchanged; no qualifying dependency defect or game-side framework workaround was involved.

### 2026-10-06 — Varied near-grass silhouette and juvenile underlayer

- A six-view visual audit identified the close grass as the clearest remaining repeated ground
  silhouette. Expanded only the near tier from 28 to 34 curved blades and made five of them a
  shorter, narrower juvenile underlayer. The existing instance colour value now also seeds bounded
  per-blade stature, colour, and wind-amplitude variation, so adjacent instances no longer repeat
  the shared cluster literally. Population, instance layout, attributes, middle/far geometry, LOD
  distances, culling, textures, allocations, and draw submissions are unchanged.
- The dedicated view retains 957 near, 637 middle, and 1,233 far clusters in 21 grass draws. Its
  visible grass geometry rises from 264,508 to 310,444 triangles, a deliberate 17.4 percent close-
  view increase. Two isolated 600-sample OpenGL 3.3 debug checks retained 98 total tracked scene
  submissions and measured 6.4599--7.2119 ms mean, 8.2017--8.8514 ms p95, and
  9.1263--10.4740 ms p99. These are gross-regression checks, not an isolated cost or mobile budget.
- The OpenGL 3.3 grass capture passes and inspected before/after frames show a fuller, less uniform
  tuft edge. Core tests pass in 38.48 seconds. The complete actual Chrome 152/SwiftShader WebGL 2
  harness passes all 22 stages, requires the new exact diagnostic, and its inspected grass/beach
  captures match native. WebAssembly is 8,495,690 bytes.
- The arm64-v8a GLES 3 APK builds through the required shared `/rv/cnaccache`, passes application
  lint, contains only the selected ABI, and verifies with APK signature scheme v2. A fresh x86_64
  package then ran on the API 35 KVM/host-GPU emulator: the revised GLSL ES 300 shader compiled,
  the complete scene reached a clean first frame in 3946.689 ms total, and the inspected 2400 by
  1080 screenshot retained varied grass plus clean translucent touch controls. The emulator was
  stopped and the default arm64 package restored at 18,741,500 bytes with a 15,931,216-byte
  stripped `libmain.so`. Physical-mobile close-grass quality, density cost, thermals, and sustained
  performance remain unqualified. CNA and Sharp Runtime remained read-only and unchanged; no
  qualifying dependency defect or workaround was involved. All compilation used at most two
  workers and the single shared ccache.

### 2026-10-06 — Current-pin ASan/UBSan requalification

- Recreated the dedicated host-only sanitizer preset against the pinned CNA `next` export and
  current Sharp Runtime `next` revision. Only island-simulator-owned core/tests and report tools
  were instrumented; the renderer/dependency graph remained deliberately outside this diagnostic.
- All 6/6 gates passed in 73.98 seconds with strict undefined-behaviour failure enabled: the full
  core suite, deterministic world report, all 12-source audio report, asset manifest validation,
  capture-validator unit tests, and independently parsed disposable WAV export. No AddressSanitizer
  or UndefinedBehaviorSanitizer finding was emitted.
- Configuration and compilation used the required `/rv/cnaccache` and two workers. The reproducible
  44 MB legacy sanitizer tree was removed after the pass rather than retained as obsolete output.
  CNA and Sharp Runtime remained read-only and unchanged; no dependency defect was observed.

### 2026-10-06 — Workspace-compliant sanitizer build directory

- Corrected the existing `linux-sanitize` preset from its legacy nonstandard directory to the
  shared approved `build-asan/` directory. The preset name and its
  combined first-party AddressSanitizer/UndefinedBehaviorSanitizer behaviour remain unchanged.
- Updated the direct leak-check command and cleanup guidance. Future diagnostics now reuse one of
  the closed-list workspace build directories and continue to use the single `/rv/cnaccache`.
  No compilation was repeated merely to validate this path-only correction; CMake preset parsing
  confirms the revised configuration. No dependency repository was touched.

### 2026-10-06 — Finite-safe touch ownership and camera deltas

- Hardened the renderer-independent dual-zone touch interpreter against invalid platform data.
  Non-finite, negative-ID, or implausibly remote contacts cannot acquire or retain movement/look
  ownership; non-finite, nonpositive, or implausible display extents release both roles; and look
  deltas are capped to one display span. Valid out-of-bounds dragging near an edge and deliberate
  centre-line crossing continue to preserve ownership and analog movement.
- Added core regressions proving that poisoned coordinates remain finite, cancel an owned look
  contact without a camera jump, reject extreme finite coordinates, release safely on invalid
  display geometry, and accept the next valid contact. The complete core suite passes in
  18.47 seconds.
- The shared implementation compiles for Linux/OpenGL 3.3, WebGL 2, and arm64 GLES 3. The Android
  package and application lint pass through `/rv/cnaccache`; the v2-signed, arm64-only APK is
  18,742,188 bytes with a 15,931,904-byte stripped `libmain.so`. WebAssembly is 8,496,251 bytes.
  Android runtime was not repeated for impossible normal-platform values; the immediately
  preceding KVM run remains the current valid-input GLES 3 evidence. CNA and Sharp Runtime stayed
  read-only and unchanged, and no dependency workaround was introduced.

### 2026-10-06 — Direction-independent player acceleration

- Replaced component-wise horizontal velocity changes with one vector-bounded response. Cardinal
  and diagonal walking now accelerate and brake at the same physical rate, while the selected
  4.3 m/s walking, 8.0 m/s running, and 2.15 m/s swimming targets remain unchanged. This also
  removes the former diagonal turn surge without changing input axes, collision, or slope rules.
- Added deterministic regressions comparing cardinal and diagonal response after both acceleration
  and release. The complete native core suite passes in 18.52 seconds. The shared implementation
  compiles for WebGL 2 and arm64 GLES 3; current WebAssembly is 8,496,213 bytes and the rebuilt
  arm64 package is 18,742,444 bytes with a 15,932,160-byte stripped `libmain.so`.
- The native core library was rebuilt through `/rv/cnaccache`. The visible OpenGL 3.3 game process
  was deliberately left running for the operator, so the final native executable relink and a
  subjective movement pass remain the immediate follow-up. No dependency repository was touched.

### 2026-10-06 — Original-asset provenance audit

- Audited every tracked source file and confirmed that the game ships no external mesh, texture,
  animation, font, or audio data. The audit identified one previously implicit distributable
  visual: the hand-authored Android vector launcher icon.
- Upgraded the asset manifest to schema 2 with a distinct `projectFiles` collection. Original
  distributable files now require a stable name, safe repository-relative path, exact SHA-256, and
  matching `ASSET_LICENSES.md` row, while externally sourced files retain their fuller provenance
  contract. Duplicate project/external paths, missing files, unsafe paths, stale hashes, and
  orphaned attribution rows fail validation.
- Recorded the launcher icon as original project artwork and hash-gated its exact XML. The updated
  validator passes with 17 procedural records, 32 generator sources, one original project file,
  18 documented attribution records, and zero external files. No dependency repository or runtime
  package was changed.

### 2026-10-06 — Analog touch running and GLES 3 validation

- Added a mobile path to the existing faster land pace without another button: movement-stick
  output at or above 92 percent requests 8.0 m/s running, while easing below 78 percent returns the
  touch selection to analog walking. Hysteresis prevents threshold chatter, release/reset cancels
  the request, swimming retains 2.15 m/s, and a hardware Shift toggle remains independent.
- Reused the generated touch HUD to communicate the state. Its movement ring and thumb change from
  green to restrained warm gold while touch running is selected, with no new textures, controls,
  draw submissions, or external assets. Deterministic tests cover engage, retained hysteresis,
  release, and cancellation; the complete native core suite passes in 43.56 seconds.
- The same sources compile for WebGL 2 and arm64 GLES 3. WebAssembly is 8,496,519 bytes; the
  arm64-only APK is 18,742,396 bytes with a 15,932,112-byte stripped `libmain.so`. An actual
  x86_64 API 35 KVM/gfxstream run reached a clean GLES 3 frame; a sustained full-travel gesture
  moved the player and an inspected 2400 by 1080 capture shows the clean warm-gold active cue.
  No fatal, shader, or `GL_INVALID_OPERATION` diagnostic appeared. The headless AVD was stopped
  and the default arm64 package restored. Physical-device ergonomics remain open. CNA and Sharp
  Runtime remained read-only and unchanged.

### 2026-10-06 — Static placement depenetration

- Added a bounded one-time recovery pass to the renderer-independent player controller. The first
  collision-aware update after explicit placement checks the static obstacle set, pushes an
  overlapping capsule out with a 1 mm separation skin, removes only velocity directed back into
  the obstacle, and stops after eight passes. Exact-centre overlaps choose a deterministic
  island-relative direction. Normal movement still uses the 16 m broad phase and pays no full-set
  scan; moving obstacles and continuous depenetration remain explicitly out of scope.
- Extended the retained movement trace and F1/F2 diagnostics with a separate depenetration count.
  Orange obstacle-contact visualization now covers both a rejected move and a placement
  correction, while the overlay reports `MOVE D/O/T/S/C`. Deterministic core regressions prove
  finite clearance, stable exact-centre direction, no idle drift, diagnostic classification, and
  matching exhaustive/indexed results. The complete core test passes in 40.61 seconds.
- The shared sources compile for WebGL 2 and arm64 GLES 3. The Android application build and lint
  pass; the APK contains only arm64-v8a native libraries and verifies with signature scheme v2.
  WebAssembly is 8,497,359 bytes; the APK is 18,744,052 bytes with a 15,933,768-byte stripped
  `libmain.so`. The actual Chrome 152/SwiftShader WebGL 2 suite subsequently passes all 22 stages,
  including every fixed scene, simultaneous forward movement and mouse look, screen-right arrow
  movement, retained run after releasing the first Shift press, and return to walking after the
  second press. The operator's visible OpenGL 3.3 process was deliberately left running, so the
  native executable relink and an interactive placement pass remain follow-up work. Android
  runtime was not repeated for this renderer-independent correction; the immediately preceding
  KVM/GLES 3 touch-running pass remains current. CNA and Sharp Runtime remained read-only and
  unchanged; no qualifying dependency defect or workaround was involved.

### 2026-10-06 — Emergent-rock wet contact band

- A current 19-view WebGL 2 composition audit identified the reef-detail outcrop as uniformly dry
  down to its water intersection. Added a restrained dark/cool wet band below 0.32 m world height
  to the existing near-rock material branch, modulated by the same distance fade as strata and
  lichen. Far rocks and contact disks retain the plain path. The change adds no texture, vertex,
  instance data, CPU update, collision work, or draw submission.
- Added an explicit startup diagnostic and made the browser lifecycle, overview, and dedicated
  reef gates require it. The complete actual Chrome 152/SwiftShader WebGL 2 suite passes all 22
  stages; the new 1280 by 720 reef capture was inspected and shows a subtle darker wet base while
  retaining the dry upper-rock treatment. WebAssembly is 8,497,747 bytes.
- The arm64 GLES 3 package rebuilds and application lint passes through `/rv/cnaccache`; the APK
  contains only arm64-v8a libraries and verifies with signature scheme v2. It is 18,744,372 bytes
  with a 15,934,088-byte stripped `libmain.so`. The native OpenGL 3.3 renderer object compiles, but
  the actual desktop shader runtime and capture remain deferred so the operator's visible game is
  not relinked or interrupted. Android appearance and physical-GPU fragment cost likewise remain
  unqualified. CNA and Sharp Runtime remained read-only and unchanged.

### 2026-10-06 — Android fixed-view QA bridge and reef qualification

- Added a game-owned Android activity override that forwards a string-array intent extra to SDL's
  existing public `getArguments()` hook only when Android marks the package debuggable. Release
  packages return an empty argument array even if an external caller supplies the extra. Extended
  the runtime-smoke helper with the optional comma-separated
  `ISLAND_ANDROID_SMOKE_ARGUMENTS` input, keeping the normal no-argument gate unchanged.
- KVM version 12 booted the API 35 `Medium_Phone` AVD in 22.240 seconds with host AMD Radeon 780M
  gfxstream GLES. A temporary x86_64 APK installed and launched with
  `--viewpoint reef --debug-overlay`; the actual 2400 by 1080 frame reports `OPENGLES3`, two visible
  reef outcrops from the 14-object population, and visibly matches the WebGL 2
  dry-upper/dark-wet-base treatment. First draw completed in 3879.191 ms total. No fatal,
  shader-compilation, or `GL_INVALID_OPERATION` diagnostic appeared. This is functional emulator
  evidence, not a physical-GPU performance claim.
- Android application lint passes with the bridge. The headless AVD was stopped cleanly and no ADB
  target remains. The default arm64-v8a APK was restored, contains only the requested ABI, and
  verifies with signature scheme v2 at 18,744,372 bytes with a 15,934,088-byte stripped
  `libmain.so`. The retained ignored reef screenshot/log are about 0.6 MB and provide useful QA
  evidence. CNA and Sharp Runtime remained read-only and unchanged; no dependency workaround was
  required.

### 2026-10-06 — Android GLES 3 representative view matrix

- Reused the debug-only, release-disabled Android argument bridge to capture fixed beach,
  close-grass, jungle, water, and monkey views on the API 35 x86_64 KVM/gfxstream AVD. Every launch
  reported `OPENGLES3`, passed the first-frame/grass runtime gate, and emitted no fatal,
  shader-compilation, or `GL_INVALID_OPERATION` diagnostic. First draw completed in
  1606.499--2217.599 ms across these five runs; the figures identify successful startup on this
  emulator and are not a shipping performance baseline.
- Inspected a labelled side-by-side matrix of those frames plus the preceding reef frame against
  their WebGL 2 counterparts. Scene geometry, broad terrain materials, close grass, jungle layers,
  palm presentation, sky, water treatment, near skinned-monkey silhouette, and the reef wet band
  remain visually consistent. Android deliberately adds its translucent touch HUD and centres the
  scaled 1920 by 1080 presentation region inside the AVD's 2400 by 1080 landscape surface. Exact pixel identity,
  animated-effect timing, automated image comparison, and physical-device quality/cost remain out
  of scope for this evidence.
- Stopped the headless emulator cleanly and confirmed that ADB has no remaining target. Restored
  and linted the default arm64-v8a package through the required shared `/rv/cnaccache`; it contains
  only the selected ABI and is 18,744,372 bytes with a 15,934,088-byte stripped `libmain.so`.
  Retained the small ignored screenshots/logs as reproducible QA evidence and removed the temporary
  comparison montage. CNA and Sharp Runtime remained read-only and unchanged; no dependency bug or
  workaround was involved.

### 2026-10-07 — Shoreline-transition surface bursts

- Extended the renderer-independent water-ripple state with a finite wet/dry crossing detector.
  Moving entry uses a 7.5 cm depth threshold, exit uses 2.5 cm, and a 1.6-second cooldown matches
  the existing audio scheduler so shallow-boundary correction cannot chatter. Each accepted
  crossing starts one 1.6-second, speed-scaled phase; exit is deliberately gentler, stationary or
  non-finite input fails closed, and reset/teleport placement never creates a false burst.
- The existing portable ocean fragment shader consumes two additional bounded uniforms and draws
  a spatially broken leading/trailing ring pair around the player. It reuses the existing ocean
  mesh and submission, allocates no texture or geometry, and skips the added radial/angle work
  while inactive. F1 and first-frame diagnostics expose burst strength/phase. Deterministic tests
  cover entry strength, phase advance, hysteresis, finite lifetime, gentler exit, cooldown, and
  invalid input; the complete core suite passes.
- The complete Chrome 152/SwiftShader WebGL 2 harness passes with all 22 stages and the revised
  interaction/Shift gates. Its inspected 1280 by 720 water capture reports a 0.6-strength burst at
  phase 0.8 and visibly shows the restrained expanding crest; WebAssembly is 8,500,585 bytes. The
  revised desktop OpenGL 3.3 consumer objects compile through the shared ccache, but the running
  operator-visible game was intentionally not relinked or interrupted, so the post-change desktop
  shader capture remains follow-up work.
- A temporary x86_64 APK then ran the same fixed water view on the API 35 KVM/host-GPU emulator.
  The actual 2400 by 1080 frame reports `OPENGLES3`, a 0.6-strength burst at phase 0.6, matching
  water/shore composition, and clean translucent touch controls. First draw completed in
  4247.442 ms total; no fatal, shader-compilation, or `GL_INVALID_OPERATION` diagnostic appeared.
  The emulator was stopped and ADB is empty. The restored/linted v2-signed arm64-only APK is
  18,747,868 bytes with a 15,937,584-byte stripped `libmain.so`. The functional emulator run is
  not a physical-GPU cost claim. All compilation used at most two workers and `/rv/cnaccache`;
  CNA and Sharp Runtime remained read-only and unchanged.

### 2026-10-07 — Coherent outer-ocean circulation

- Replaced the old step-like purely radial return speed with one renderer-independent velocity
  function. The field now eases continuously from zero at the 42 m soft boundary, reaches a
  2.1 m/s shoreward component at 72 m, and adds only a restrained positive coast-parallel drift
  with gentle three-lobe spatial variation. Outward input suppression and the independent 92 m
  safety recovery remain intact, so circulation cannot weaken the guaranteed return to shore.
- Made the exact velocity authoritative for movement and diagnostics. F1 reports both normalized
  current strength and world-space `DRIFT x/z`; the existing two-draw F2 pass points its yellow
  arrow along that same velocity rather than reconstructing a purely radial approximation. The
  release scene adds no draw, allocation, asset, shader work, or audio source.
- Added deterministic regressions for dominant shoreward velocity, restrained lateral drift,
  continuous zero-speed onset, idle-swimmer shoreward progress, and meaningful coast-parallel
  displacement while retaining the existing long-run escape-prevention tests. All six non-display
  gates pass. The changed OpenGL 3.3 consumer objects compile through `/rv/cnaccache`; the visible
  operator game remains on the prior executable so it was not interrupted or unsafely relinked.
- The complete Chrome 152/SwiftShader WebGL 2 renderer/lifecycle/input/audio harness passes all 22
  stages. Its inspected 1280 by 720 current capture reports a nonzero exact drift while preserving
  the previous scene and water presentation; WebAssembly is 8,500,109 bytes. The arm64-v8a GLES 3
  package builds and application lint passes, contains only the requested ABI, and verifies with
  APK signature scheme v2 at 18,748,788 bytes with a 15,938,504-byte stripped `libmain.so`. A new
  Android runtime pass was not claimed: `/dev/kvm` is presently absent from this execution
  namespace even though earlier KVM/gfxstream captures remain valid evidence. CNA and Sharp
  Runtime remained read-only and unchanged; no dependency defect or workaround was involved.

### 2026-10-07 — CNA `next` renderer-retirement baseline refresh

- Advanced the reproducible CNA pin from `fe20e3e9c23b5353a5b63bbda36f236d799d6612` to the
  workspace's clean current `next` commit `9fbe34468c69872204536bfe3dd6f106404d9403` after reviewing
  both intervening commits. They deliberately retire DIRECTX12, CANVAS, OPENGLES2, and WEBGL1;
  island-simulator's required OPENGL33, WEBGL2, and OPENGLES3 EasyGL profiles remain supported.
  Sharp Runtime stays clean on `next` at `21c06910f7cf887cceebb45200e07b43dd45fef4`.
- Removed the obsolete 323 MiB generated CNA export only after confirming that no build process
  held it, then materialized one replacement at the exact new commit. The new 321 MiB export
  retains the matching vendored SDL snapshots and avoids a duplicate dependency tree. Neither CNA
  nor Sharp Runtime was edited.
- Reconfigured the existing Linux tree for explicit OPENGL33, compiled the revised 160,614,552-byte
  `libcna.so` and game consumer objects, and passed all six non-display gates. The complete native
  display suite and executable relink remain deferred because the operator-visible prior build is
  still running; this is recorded as an evidence boundary rather than silently claiming the new
  pin's OpenGL runtime.
- Reconfigured and rebuilt the Web package for explicit WEBGL2, then passed the complete actual
  Chrome 152/SwiftShader 22-stage renderer, lifecycle, scene, audio-initialization, and input
  harness. The inspected current-field frame retains the intended island/ocean presentation and
  reports its exact drift. WebAssembly is 8,464,191 bytes.
- Rebuilt and linted the arm64-v8a OPENGLES3 Android package. It contains only the requested ABI,
  verifies with APK signature scheme v2, and is 18,690,996 bytes with a 15,880,712-byte stripped
  `libmain.so`. The restricted sandbox hid `/dev/kvm`, so runtime was initially left unclaimed;
  an explicitly escalated host check and the follow-up milestone below then qualified the current
  pin. All compilation used `/rv/cnaccache` and no more than two workers. No framework defect
  affecting island-simulator was found, so no `BUGS.md` entry or workaround was added.

### 2026-10-07 — Current-pin Android GLES 3 requalification

- Confirmed outside the restricted filesystem sandbox that `/dev/kvm` is device 10:232, the
  developer belongs to group `kvm`, both read/write probes pass, and Emulator 36.5.11 reports KVM
  version 12 usable. No new device node, root-run emulator, or `sg` workaround is required.
- Built the current `9fbe34468c69` sources for x86_64 and booted the existing API 35 `Medium_Phone`
  AVD in 15.876 seconds using `-accel on` and host AMD Radeon 780M gfxstream. The ordinary overview
  reported `OPENGLES3`, opened its 44.1 kHz AAudio stream, and completed first draw in 2179.350 ms
  total. Its inspected 2400 by 1080 frame retains the complete sunny island, dense grass/jungle,
  and clean translucent dual-zone controls.
- Ran a second debug-argument launch at the fixed offshore-current view. It completed first draw in
  1555.412 ms and the inspected frame reports normalized current 0.7, exact `DRIFT 0.3/1.2`, water
  view intensity 1.0 in one draw, undertow 0.9, and `OPENGLES3`. Neither launch emitted a fatal,
  shader-compilation, or `GL_INVALID_OPERATION` diagnostic.
- Stopped the emulator cleanly, confirmed ADB empty, retained the two small ignored screenshot/log
  evidence sets, and restored the linted v2-signed arm64-only APK at 18,690,996 bytes with a
  15,880,712-byte stripped `libmain.so`. These are functional KVM/gfxstream results, not physical
  mobile GPU, thermal, battery, or real-finger ergonomics claims.

### 2026-10-07 — Portable near-surface depth and light-scatter cue

- A cross-renderer composition review found that the fish and shark silhouettes were readable but
  the water column below the surface remained too visually uniform. Refined the existing generated
  160 by 90 premultiplied water-view texture with a restrained vertical depth haze, two crossing
  upper-surface scatter fields, and a few broad light streaks. The same current modulation,
  smoothed intensity, single conditional `SpriteBatch` draw, dry zero-draw path, and external-asset
  count are retained. This is deliberately a near-surface cue, not a claim of distance fog,
  refraction, caustic simulation, or a geometric waterline.
- Compiled the changed OpenGL 3.3 consumer object through `/rv/cnaccache` and passed the complete
  renderer-independent core test binary. The operator-visible Linux game was intentionally not
  relinked or interrupted, so native visual requalification remains deferred until that process is
  closed.
- Rebuilt WebGL 2 and passed the complete real Chrome 152/SwiftShader 22-stage renderer,
  lifecycle, input, audio-initialization, and visual harness twice during refinement. Inspected
  calm-water, offshore-current, fish, and shark frames retain wildlife and shoreline readability
  while gaining a subtle top-to-bottom water-column separation. The final WebAssembly is
  8,464,649 bytes.
- Built a temporary x86_64 package and ran the fixed calm-water view on the KVM/API 35 host-GPU
  emulator. The inspected 2400 by 1080 frame reports `OPENGLES3`, water-view intensity 0.6 in one
  draw, and 29.5 FPS; first draw completed in 1804.148 ms with no fatal, shader-error, or
  `GL_INVALID_OPERATION` diagnostic. The emulator was stopped cleanly and ADB is empty. The
  restored arm64-only APK passes application lint, verifies with signature scheme v2, and is
  18,691,524 bytes with a 15,881,240-byte stripped `libmain.so`. All builds used at most two
  workers and the required shared ccache. CNA and Sharp Runtime remain read-only and unchanged;
  no dependency defect or workaround was involved.

### 2026-10-07 — Near-canopy buttress grounding pass

- The same clean jungle/monkey composition audit exposed an abrupt cylindrical termination on
  nearby broadleaf trunks. Added four or five short, deterministically irregular buttress wedges
  to each shared near-canopy variant. Length, angle, width, crest height, and facet shade vary by
  variant/root; the static bases carry zero wind weight. The far meshes, placements, culling,
  contact shadows, instance streams, and draw submissions are unchanged.
- Each shared near mesh gains only 24 or 30 vertices and the same number of triangles. The maximum
  authored tip/width radius remains inside the existing combined player-capsule/trunk collision
  footprint at every 0.78--1.28 tree scale, avoiding a new visible snag outside the proxy. The
  runtime mesh validator covers every new index before upload.
- The OpenGL 3.3 consumer object compiles through `/rv/cnaccache` without relinking the still-live
  operator game. The complete Chrome 152/SwiftShader 22-stage WebGL 2 harness passes and requires
  the new exact diagnostic; inspected jungle and monkey frames show the grounded base without a
  new silhouette problem. Final WebAssembly is 8,471,341 bytes.
- A temporary x86_64 package then rendered the fixed monkey view on the KVM/API 35 host-GPU AVD.
  The inspected 2400 by 1080 frame shows the new tree base beside the monkey, reports `OPENGLES3`
  and 23.0 FPS, and completed first draw in 2421.371 ms without a fatal, shader-error, or
  `GL_INVALID_OPERATION` diagnostic. After a clean emulator shutdown, the restored arm64-only APK
  passes application lint, verifies with signature scheme v2, and is 18,694,612 bytes with a
  15,884,328-byte stripped `libmain.so`. Compilation used two workers and the required shared
  ccache; CNA and Sharp Runtime remain read-only and unchanged.

### 2026-10-07 — Cross-renderer logical-viewport qualification

- Added the actual `GraphicsDevice` logical viewport to the opt-in F1 renderer row and to a
  one-shot first-frame diagnostic. The latter is mirrored through the existing Android platform
  log bridge so it is not lost with the following long population diagnostic. Normal presentation
  gains no draw, allocation, or recurring log operation; the F1 panel simply extends an existing
  text row.
- Strengthened both real-browser and Android GLES 3 smoke gates to require exactly
  `1280x720 logical backbuffer`. The complete Chrome
  152/SwiftShader 22-stage WebGL 2 suite passes, including lifecycle, renderer, scene, input, audio
  initialization, and all fixed captures. The final WebAssembly is 8,482,838 bytes.
- Built x86_64 through the required shared ccache and ran the fixed monkey camera on the API 35
  KVM/host-GPU AVD. Both logcat and the inspected 2400 by 1080 physical screenshot report
  `OPENGLES3` with a 1280 by 720 logical backbuffer; CNA scales it into the centred 1920 by 1080
  presentation region. The captured F1 sample reports 31.5 FPS / 29.1 ms and first draw completed
  in 1439.657 ms without a fatal, shader-error, or `GL_INVALID_OPERATION` diagnostic. These are
  functional emulator observations, not a physical-mobile performance claim.
- Stopped the emulator cleanly and confirmed ADB empty. Restored/linted the default arm64-only APK,
  verified its signature scheme v2, and measured 18,697,540 bytes with a 15,887,256-byte stripped
  `libmain.so`. After the operator closed the preceding Linux session, the complete current
  executable linked through `/rv/cnaccache`, all six renderer-independent gates passed, and a
  live-desktop four-frame overview smoke selected `OPENGL33`, compiled every current scene shader,
  reported the same 1280 by 720 logical backbuffer, and exited cleanly. CNA and Sharp Runtime
  stayed read-only and no dependency defect or workaround was involved.

### 2026-10-07 — Near-monkey fur-silhouette polish

- Review of the current fixed monkey frame found that the face was readable but the near head
  retained an overly smooth toy-like outline. Added one small crown tuft and two cheek tufts to each
  shared near body variant. The original procedural geometry is rigidly attached to the existing
  head bone, follows every clip/crossfade/attention pose, and uses the existing fur material.
- Each near variant gains 27 vertices and nine triangles. The fixed view currently shows two near
  monkeys and one far monkey, adding only 54 transformed vertices and 18 triangles. Far geometry,
  face/eye detail passes, animation palettes, culling, LOD thresholds, collision, behaviour,
  textures, allocations, and draw counts remain unchanged. Procedural mesh validation still runs
  before every upload.
- The changed OpenGL 3.3 objects compile through `/rv/cnaccache` without relinking or interrupting
  the currently visible operator game. The complete Chrome 152/SwiftShader 22-stage WebGL 2 suite
  passes and requires the exact new diagnostic; its inspected fixed frame shows restrained crown
  and cheek breakup while retaining face readability and the two-near/one-far population. Final
  WebAssembly is 8,486,871 bytes.
- The default arm64-only GLES 3 APK builds, passes application lint, verifies with signature scheme
  v2, and is 18,699,724 bytes with a 15,889,440-byte stripped `libmain.so`. A new Android runtime
  capture is intentionally deferred while the user plays the visible Linux build; the preceding
  fixed-monkey GLES 3 evidence therefore does not claim the new tufts. CNA and Sharp Runtime remain
  read-only, and no dependency defect or workaround was involved.

### 2026-10-07 — Persistent run/walk mode feedback

- Added a short normal-play confirmation for the persistent keyboard Shift toggle. `RUN MODE  8.0
  M/S` uses a restrained warm-gold accent and `WALK MODE  4.3 M/S` uses seafoam; both reuse the
  existing generated bitmap font/one-pixel texture, appear near the lower centre for 1.4 seconds,
  and fade during the final 0.32 seconds. The fully expired path submits nothing, and analog touch
  travel deliberately keeps its direct hold-style request without repeatedly triggering the text.
- Kept debug input semantics independent: holding Shift for the debug-only F3 inspection camera
  still selects 36 m/s flight but no longer changes the retained player mode or creates a normal
  gameplay notice. Strengthened `IslandGameFreeCameraInput` to hold Shift during its real X11
  flight and reject any player run-mode diagnostic; executing that revised native gate is deferred
  until the operator closes the visible pre-change Linux process.
- The complete actual Chrome 152/SwiftShader WebGL 2 suite passes all 22 stages. It still proves
  that the first Shift edge persists after release, running exceeds 7.0 m/s, and the second edge
  restores a maximum 4.3 m/s pace. A new machine-recorded screenshot visibly shows the correctly
  premultiplied gold notice without a rectangular blend artifact. Final WebAssembly is 8,490,159
  bytes.
- Changed OpenGL 3.3 objects compile through `/rv/cnaccache` without relinking the visible game.
  The arm64-only GLES 3 APK builds, passes application lint, verifies with signature scheme v2, and
  is 18,701,940 bytes with a 15,891,656-byte stripped `libmain.so`. Android runtime appearance is
  not newly claimed. CNA and Sharp Runtime remain read-only, and no dependency defect or workaround
  was involved.

### 2026-10-07 — Unobstructed fixed-camera visual evidence

- A review of the browser evidence found that the opt-in F1 panel covered more than half of most
  fixed-camera captures. The statistics were useful, but those same images were poor evidence for
  composition, LOD transitions, silhouettes, and full-frame rendering defects. Kept the panel as
  an initial validation gate and changed the WebGL 2 matrix to focus the canvas, send a real F1
  edge, require the exact disabled diagnostic, wait for a newly presented frame, and only then
  save each visual reference.
- Moved only the F1 edge handler out of the normal player-camera update. Fixed QA cameras still
  reject movement, look, Shift locomotion changes, teleports, and free-camera controls, while their
  diagnostic panel can now be hidden interactively. The ordinary first-person path retains the
  same input state and behaviour. The browser helper fails explicitly if the panel was not active,
  the canvas could not be focused, or the game did not acknowledge its removal.
- The complete Chrome 152/SwiftShader 22-stage WebGL 2 suite passes without a synthetic click or
  timing retry beyond the established bounded key helper. Manual inspection of regenerated beach,
  jungle, east-clearing, and near-surface-water frames confirms a full unobstructed 1280 by 720
  image with the expected scene and no residual panel. The final WebAssembly is 8,492,424 bytes.
- The changed OpenGL 3.3 object compiles through `/rv/cnaccache`; the native executable is
  intentionally not relinked while the operator's visible game process remains active. The default
  arm64-only GLES 3 APK builds with two workers, passes application lint, verifies with signature
  scheme v2, and is 18,702,516 bytes with a 15,892,232-byte stripped `libmain.so`. No new Android
  runtime claim is made. CNA and Sharp Runtime remain read-only, and no dependency defect or
  workaround was involved.

### 2026-10-07 — Long-period daytime-jungle bed

- Audio-source review found that the four persistent beds did not realign for 77 minutes, but the
  recognizable daytime-jungle insect sequence itself still repeated every 12 seconds. Extended
  that existing source to 29 seconds and authored sixteen low-level chirps as irregular singles and
  pairs with varied frequency, sweep, duration, pulse, and amplitude. The same source, interior
  gain, fade, mute, and swimming attenuation remain; the persistent-source count stays four and the
  complete six-source ceiling is unchanged.
- The 10/11/29/7-second bed lengths now realign after 22,330 seconds: 6 hours, 12 minutes, and 10
  seconds. The change adds 749,700 bytes of generated mono PCM, taking all twelve procedural
  sources from 2,533,544 to 3,283,244 bytes (about 3.13 MiB) before backend bookkeeping. No binary
  audio asset or external license was added.
- Renderer-independent core tests, the exact-source signal report, WAV export validation, asset
  provenance validation, and deterministic regeneration pass. The new source is exactly 639,450
  frames at 22.05 kHz, with RMS 0.11069326, peak 0.55999756, mean -0.00293074, seam step
  0.01110840, maximum internal step 0.22473145, and fingerprint `f459687300acca64`.
- The complete Chrome 152/SwiftShader 22-stage WebGL 2 suite passes, including first-gesture
  creation and start of every persistent source. First-draw content time in that run was 235.0 ms
  and final WebAssembly is 8,492,625 bytes. The default arm64-only GLES 3 APK builds with two
  workers, passes application lint, verifies with signature scheme v2, and is 18,702,612 bytes with
  a 15,892,328-byte stripped `libmain.so`. Android runtime playback and the required long
  headphone/speaker fatigue pass remain unclaimed. The native core/report tools are current, while
  the graphical executable is intentionally not relinked beneath the operator's visible process.
  CNA and Sharp Runtime remain read-only, and no dependency defect or workaround was involved.

### 2026-10-07 — Depth-integrated aquatic wildlife colour

- Added one shared CPU lighting sample per visible fish or shark. It combines a smooth
  0.35--8.0 m depth response with two low-frequency moving sunlight bands, then applies a
  restrained tropical-water tint to the existing body, belly, and fin palettes before their
  fixed-sun facet lighting. Non-finite inputs fall back to an unchanged neutral profile. Eye and
  gill accents remain unmodified so near silhouettes retain their small identifying details.
- The change adds no texture, shader, vertex, buffer, draw submission, transparency, or new asset;
  it only adds two sine evaluations for each animal that was already accepted by distance/frustum
  culling and already rebuilt in the dynamic CPU batch. Fish retain 51/12 near/far vertices and
  one draw, while the single shark retains 51/15 vertices and one draw.
- Changed OpenGL 3.3 objects compile through `/rv/cnaccache` without relinking or interrupting the
  operator's visible game, and all six renderer-independent native gates pass. The complete real
  Chrome 152/SwiftShader 22-stage WebGL 2 suite passes and now requires the exact feature diagnostic
  in its lifecycle, fish, and shark gates. Direct before/after inspection of both regenerated fixed
  views found the animals still readable while the deeper shark is less detached from the water
  column. Final WebAssembly is 8,496,414 bytes.
- The default arm64-only GLES 3 APK builds with two workers, passes application lint, verifies with
  signature scheme v2, and is 18,705,692 bytes with a 15,895,408-byte stripped `libmain.so`. A new
  Android runtime appearance claim is intentionally deferred while the operator plays the visible
  Linux build. CNA and Sharp Runtime remain read-only, and no dependency defect or workaround was
  involved.
- Follow-up regression work moved the numeric profile into renderer-independent
  `AquaticLightingModel` while leaving CNA colour conversion in the renderer helper. Core tests now
  cover the shallow/deep tint bounds, monotonic depth response, suppressed deep caustics, bounded
  moving shallow light, and neutral fail-closed handling for non-finite input. The test passes,
  OpenGL 3.3 renderer objects compile, WebGL 2 links at 8,495,452 bytes, and the two-worker Android
  build/lint passes at 18,705,644 bytes with a 15,895,360-byte stripped library. The visible result
  is unchanged from the already inspected browser frames, so no duplicate runtime claim is made.

### 2026-10-07 — Bounded procedural-audio startup staging

- Refactored game-side `Soundscape::Start` to generate, convert, upload, and release each of its
  twelve deterministic sources before beginning the next. Previously all 3,283,244 PCM bytes and
  all 3,283,244 converted byte copies remained live together while effects were created. The
  largest remaining source/conversion pair is the 29-second jungle bed at 2,557,800 bytes, removing
  at least 4,008,688 bytes (about 3.82 MiB) of simultaneous game-owned staging. This is not a total
  RSS claim because generator filter arrays and backend-owned decoded audio are separate.
- Source generation code, PCM fingerprints, loop points, four persistent instances, eight one-shot
  effects, event timing, gains, and the six-live-source ceiling are unchanged. Core, exact audio
  report, independent WAV export, and asset provenance gates pass. The OpenGL 3.3 audio consumer
  object compiles through `/rv/cnaccache` without relinking the operator's live executable.
- The complete real Chrome 152/SwiftShader 22-stage WebGL 2 suite passes. Its first-gesture
  deployment gate now requires an explicit bounded-staging diagnostic in addition to proving Web
  Audio creation, every persistent bed, both wave/splash variant pairs, and the pooled wildlife
  voice. Final WebAssembly is 8,490,148 bytes.
- The default arm64-only GLES 3 APK builds with two workers, passes application lint, verifies with
  signature scheme v2, and is 18,705,180 bytes with a 15,894,896-byte stripped `libmain.so`. A new
  Android runtime-memory claim is intentionally deferred while the visible Linux game remains
  active. The Android runtime helper now accepts `ISLAND_ANDROID_SMOKE_REQUIRE_AUDIO=1` to require
  the same post-upload diagnostic on the next emulator/device run; shell syntax and invalid-value
  rejection pass, while the opt-in device path remains deliberately unclaimed. CNA and Sharp
  Runtime remain read-only and unchanged; no dependency defect or workaround was involved.

### 2026-10-07 — Isolated persistent-pace regression model

- Moved the keyboard walk/run edge state into the renderer- and platform-independent
  `LocomotionPaceToggle`. One Shift edge selects the persistent 8.0 m/s run pace, holding and
  releasing the key retain it, and a second edge returns to the 4.3 m/s walking pace. The player
  update, normal-play confirmation, and touch combination consume that single authoritative
  selection; movement speeds and presentation are otherwise unchanged.
- The model always observes the physical Shift state even while player toggling is disabled. This
  preserves the debug free camera's hold-Shift fast flight and prevents a key pressed there from
  becoming a delayed player toggle if the camera is closed before the key is released.
- Added deterministic core coverage for initial walking, first edge, held key, release, second
  edge, disabled/debug-camera input, held-key re-entry, rearming, and the next valid edge. The
  complete renderer-independent core test passes in 16.99 seconds, and the changed OpenGL 3.3
  consumer object compiles through the required shared `/rv/cnaccache` without relinking or
  interrupting the operator's live game.
- The complete Chrome 152/SwiftShader WebGL 2 22-stage suite passes with the extracted model. Its
  real input report independently confirms run selection, 8.0 m/s movement after Shift release,
  second-edge deselection, restored 4.3 m/s walking, correct right-arrow direction, and concurrent
  forward movement/mouse look. Final WebAssembly is 8,490,180 bytes. The arm64-only GLES 3 APK
  builds with two workers, passes application lint, contains the expected arm64 `libmain.so`, and
  verifies with APK signature scheme v2; it is 18,705,876 bytes with a 15,895,592-byte stripped
  library. Android runtime behaviour is not newly claimed while the operator's visible Linux game
  remains active. No dependency modification or workaround is involved.

### 2026-10-07 — Generated texture and transparency budget audit

- Inventoried all game-side texture creation and exposed one shared compile-time budget for the
  exact dimensions used by the debug atlas, touch controls, swimming veil, and two solid-white
  helper textures. The six generated, single-level `Color`/RGBA8 textures total 35,906 pixels or
  143,624 base-level bytes. A static assertion and renderer-independent core checks prevent this
  accounting from silently drifting as dimensions change.
- Confirmed there are no imported texture assets or mip chains and no alpha-card vegetation.
  Grass, palms, jungle, props, terrain, sky, and wildlife are opaque geometry. The remaining
  transparent work is bounded to the required ocean, one narrow shoreline ribbon, one culled
  contact-shadow batch, conditional water/touch overlays, opt-in diagnostics, and a transient pace
  notice. Separate mobile/web textures are not justified at roughly 140.3 KiB; physical-device
  fill-rate profiling remains open rather than being inferred from code inspection.
- The complete native core test passes in 17.44 seconds, all four OpenGL 3.3 consumer objects
  compile without relinking the live game, and WebGL 2 links at 8,490,180 bytes. The arm64 GLES 3
  APK builds and passes lint with two workers, verifies with signature scheme v2, and is
  18,705,980 bytes with a 15,895,696-byte stripped library. These are inventory and compilation
  checks; no new visual/runtime claim is made because all generated pixel dimensions and content
  remain unchanged. Every build used `/rv/cnaccache`; CNA and Sharp Runtime remained read-only.

### 2026-10-07 — Slow non-amplifying ambient-bed variation

- Added deterministic long-period gain movement after the authored shoreline/interior zone mix.
  Surf combines two components with about 44/95-second periods and can dip at most 3.5 percent;
  breeze uses about 57/131 seconds at 7.5 percent; jungle uses about 50/108 seconds at 5.5 percent.
  All gains start at unity, only move downward, and pass through the existing 1.8/2.6-second
  smoothing, preserving established volume ceilings and avoiding hard changes.
- The change adds no PCM, effect, playback instance, allocation, generator work, download payload,
  or source overlap. All twelve fingerprints and the 3,283,244-byte PCM total remain unchanged, as
  do the four persistent and six-maximum-live-source budgets. Core tests now prove bounded drift,
  deterministic reset, invalid-time stability, and the existing accelerated twenty-minute mix
  ceilings; exact audio report, WAV export, and provenance gates pass.
- Added an exact startup diagnostic to the browser deployment gate and the opt-in Android audio
  runtime gate. The complete Chrome 152/SwiftShader 22-stage suite passes; its post-gesture state
  reports audio start, bounded staging, and ambient-bed variation enabled while the input suite
  still proves persistent running, walk restore, and simultaneous movement/look. WebAssembly is
  8,490,627 bytes. The two-worker arm64 GLES 3 build/lint passes, and its v2-signed APK is
  18,706,644 bytes with a 15,896,360-byte stripped library. Native graphical relinking, live
  listening, and the opt-in Android runtime audio gate remain deferred while the operator's game
  process stays open; CNA and Sharp Runtime remain read-only.

### 2026-10-07 — Broken shoreline-foam continuity polish

- An unobstructed fixed WebGL 2 water capture exposed a broad, nearly continuous bright line where
  the ocean foam and exact ribbon wash overlapped. The ribbon wash now reuses its existing broken
  crest mask at 28--100 percent strength, while the broad ocean foam reuses its existing spatial
  sine through a squared 16--100 percent continuity mask. The centre still communicates an active
  shore, but the line naturally weakens or disappears across the left and right portions of the
  reviewed frame.
- This reuses existing shader inputs and evaluations: there is no additional sine, texture,
  geometry, buffer, allocation, or draw submission. Both OpenGL 3.3 renderer objects compile
  through the shared cache. The complete Chrome 152/SwiftShader 22-stage WebGL 2 suite requires
  both exact continuity diagnostics and passes; the accepted final fixed-water capture was
  compared at original resolution against the baseline. WebAssembly is 8,491,085 bytes.
- The two-worker arm64 GLES 3 build and lint pass, the APK verifies with signature scheme v2, and
  the 18,706,996-byte package contains a 15,896,712-byte stripped library. Android runtime
  appearance is not newly claimed. The operator's visible Linux game remained uninterrupted, so
  it still runs the preceding linked executable; CNA and Sharp Runtime remained read-only.

### 2026-10-07 — Bounded wet-sand sun sheen

- The cross-renderer composition review found that the existing wet sand darkened correctly but
  retained the same matte response as the dry beach. The terrain shader now derives a geometric
  normal from world-position derivatives only inside the authoritative wet-edge band and adds a
  narrow fixed-sun half-vector highlight plus a much weaker grazing response. Original-resolution
  water, driftwood, and beach-ground comparisons retain a deliberately subtle edge and show no
  new uninterrupted bright shoreline.
- The sheen adds no texture, varying, vertex/index data, geometry, buffer, allocation, or draw.
  Its extra fragment work is bounded to wet coverage. The OpenGL 3.3 consumer object compiles via
  `/rv/cnaccache` without relinking the visible game. The complete Chrome 152/SwiftShader WebGL 2
  22-stage suite requires the exact diagnostic, compiles the shared GLSL ES 300 shader, and passes;
  WebAssembly is 8,492,140 bytes.
- The two-worker arm64 GLES 3 build and lint pass, and the 18,707,956-byte APK verifies with
  signature scheme v2 and contains a 15,897,672-byte stripped library. Android runtime appearance
  and physical-device fragment cost are not newly claimed. The KVM host preflight separately
  reports version 12 installed and usable, but no AVD was started beside the operator's visible
  Linux game. CNA and Sharp Runtime remained read-only.

### 2026-10-07 — Browser Escape and pointer-lock recovery regression

- Removed the remaining Web-only ambiguity between Escape-to-unlock and Escape-to-exit. An
  Emscripten build now treats every delivered Escape edge as pointer-lock release only, so browser
  event ordering cannot convert a release into game termination. Linux retains its established
  first-Escape release and second-Escape exit behaviour; Android behaviour is unchanged.
- Replaced the harness's JavaScript `document.exitPointerLock()` shortcut with a real Chrome Escape
  key. Chrome consumes that key at browser level rather than forwarding it to SDL, so the portable
  regression checks the observable contract: pointer lock is absent, new game output continues,
  and `exited`/`error` remain clear. It then waits 1.25 seconds through Chrome's immediate-relock
  security cooldown, clicks the canvas, and requires pointer lock again. An initial no-delay run
  correctly exposed Chrome's `SecurityError`; the accepted complete 22-stage run passes without
  relying on a game-side diagnostic that Chrome cannot emit.
- The OpenGL 3.3 `IslandGame` object compiles through `/rv/cnaccache` without relinking the visible
  game. WebAssembly is 8,492,253 bytes. The two-worker arm64 GLES 3 build/lint and APK v2-signature
  check pass; because the Web branch is compiled out there, the APK remains 18,707,956 bytes with
  a 15,897,672-byte stripped library. No new Android runtime claim is made, and CNA and Sharp
  Runtime remained read-only.

### 2026-10-07 — Project-owned disk maintenance pass

- Audited the project-owned build trees after confirming the operator-visible game remained alive.
  The 1.1 GiB `build-consumer` directory was a stale, unused, reproducible one-off arm64 consumer
  check last built on 2026-10-06; it contained no source or current deliverable and no process used
  it. Removed that directory only, reducing the workspace total from 6.1 GiB to 5.0 GiB.
- Preserved the 2.6 GiB native tree that owns the running executable and shared CNA objects, the
  72 MiB current Web tree, the current Android package/intermediates, project-owned reusable SDL
  cross-build caches, and the small retained Android runtime evidence directories. No dependency,
  user data, licensed asset, cache outside the project, or uncertain artifact was deleted.
- Re-ran the renderer-independent core preset after cleanup; it passes 1/1 in 18.15 seconds. The
  working tree remained coherent and the visible game stayed active. A second disk review remains
  scheduled before final packaging, when the active build trees can be judged against their live
  use rather than removed prematurely.

### 2026-10-07 — Deterministic precompressed Web release package

- Added a project-owned release packager for the existing WebGL 2 Release outputs. It preflights
  all three inputs before creating output, copies the HTML/JavaScript/WebAssembly artifacts into
  `build-probe/web-release`, emits reproducible `gzip -9 -n` siblings, round-trips every stream,
  and verifies source, copy, and decompressed SHA-256 identity.
- The schema-1 manifest records exact content types, byte counts, hashes, gzip sizes, totals, and
  reduction. The current HTML/JavaScript/WebAssembly package is 8,756,777 bytes uncompressed and
  2,482,746 bytes through gzip, a 71.6% transfer reduction. Runtime-generated PCM and textures are
  not shipped as separate payloads.
- Added a focused CTest that validates content, MIME metadata, hashes, totals, deterministic repeat
  output, and clean missing-input failure without leaving a partial package. It passes in 0.18
  seconds. Documented the required production `Content-Encoding`, `Vary`, and MIME headers while
  leaving CDN/server verification, decoded-memory cost, and repeated hardware cold-load timing
  explicitly open.
- Reused the current `build-probe` Release outputs; no renderer source changed and no executable
  was rebuilt. The visible Linux game remained alive. CNA and Sharp Runtime stayed read-only.

### 2026-10-07 — Dry-wrack beach-detail silhouette pass

- An original-resolution cove review found that all four near-field beach-detail meshes repeated
  similar shell/pebble silhouettes across otherwise open sand. Replaced the fourth existing mesh
  with four low curved dry-wrack strands and two small holdfast forms in muted olive/brown tones.
  The 265 deterministic placements, four variant buffers, 52 m culling distance, and maximum four
  nearby beach-detail submissions are unchanged; no texture, transparency, collision proxy,
  simulation, or extra draw class was added.
- The new 80-vertex/68-triangle mesh replaces a 64-vertex/56-triangle composition. Core tests pass
  in 16.74 seconds and still prove exact placement, support, route clearance, scale, and all four
  variants. The OpenGL 3.3 renderer objects compile through `/rv/cnaccache` without relinking the
  operator-visible game.
- The complete actual Chrome 152/SwiftShader WebGL 2 suite passes all 22 stages, requires the
  explicit dry-wrack startup diagnostic, and produces an inspected cove frame. A bounded wait for
  the real overlay-ready message removes a discovered slow-frame race without weakening the gate.
  WebAssembly is 8,493,389 bytes; the updated deterministic release package is 8,758,232 bytes
  uncompressed and 2,483,492 bytes through gzip.
- The two-worker arm64 GLES 3 build and lint pass through the shared ccache, and APK v2 signature
  verification passes. The APK is 18,710,020 bytes with a 15,899,736-byte stripped `libmain.so`.
  No new Android runtime or physical-device appearance claim is made. CNA and Sharp Runtime
  remained read-only.

### 2026-10-07 — Reused-slope shallow-water relief

- Original-resolution water and current frames showed that the tropical shallow colour and shore
  transition were coherent, but large calm regions remained visually flat at low viewing angles.
  Added a restrained shallow-ridge colour cue derived from the length of the analytic five-wave
  slope already calculated for normals and foam.
- The cue is bounded by existing 135--420 m detail fade, Fresnel reflection, and shallow-water
  interpolation. It adds one length, two smooth steps, and scalar colour shaping to existing
  ocean fragments, with no new sine, texture, varying, geometry, allocation, or draw submission.
- The OpenGL 3.3 ocean object compiles through `/rv/cnaccache` without relinking the visible game.
  The complete actual Chrome 152/SwiftShader WebGL 2 suite passes all 22 stages, requires the new
  diagnostic, and produces inspected calm-water/current frames without a bright band or offshore
  colour shift. WebAssembly is 8,493,776 bytes; the release package is 8,758,619 bytes
  uncompressed and 2,483,497 bytes through deterministic gzip.
- The two-worker arm64 GLES 3 build/lint and APK v2-signature verification pass. The APK is
  18,710,388 bytes with a 15,900,104-byte stripped `libmain.so`. No new Android runtime or
  physical-device cost claim is made. CNA and Sharp Runtime remained read-only.

### 2026-10-07 — Bone-following near-monkey digits

- Close monkey review found that the established face, fur tufts, and animation read clearly, but
  the hand and foot ends remained undifferentiated ellipsoids. Added three short rounded fingers
  per hand and three toes per foot only to each shared near body variant, weighted to the existing
  forearm and shin bones.
- The selected near mesh gains 216 vertices and 240 triangles. No bone, animation clip/palette,
  material, texture, allocation, draw submission, behaviour, collision, or far-LOD geometry was
  added. The fixed view continues to render two near agents and one far agent through the existing
  main/face/eye paths.
- The first actual WebGL 2 frame exposed overly long thin fingers, so their reach was reduced and
  cross-section widened before acceptance. The final Chrome 152/SwiftShader 22-stage suite passes,
  requires the exact diagnostic in lifecycle and monkey gates, and produces an inspected frame
  with restrained digits following the skinned limbs. OpenGL 3.3 objects compile through the
  shared cache without relinking the visible game.
- The two-worker arm64 GLES 3 build/lint and APK v2-signature verification pass. WebAssembly is
  8,497,758 bytes; the APK is 18,711,460 bytes with a 15,901,176-byte stripped library. The
  deterministic release package is 8,762,601 bytes uncompressed and 2,484,224 bytes through gzip.
  Physical-device appearance and animation cost remain unqualified; CNA and Sharp Runtime stayed
  read-only.

### 2026-10-07 — Launch-directory independence regression

- Audited every current runtime-content path. Portable shader text is embedded in its owning
  renderer; meshes, small textures, and PCM audio are generated at startup. The source asset
  manifest and Web shell/package templates are development inputs and are not opened by the game.
- Added `IslandGameRelocatableWorkingDirectory`, which starts the actual OpenGL game from an
  otherwise empty build-owned working directory and requires a completed four-frame smoke run.
  This protects relocatable launches without adding an unused content-root abstraction.
- Reconfigured the pinned OpenGL 3.3 build with `/rv/cnaccache` active and ran the new test through
  an isolated X11 display. It passes in 1.45 seconds. No executable relink was required, so the
  operator-visible game remained open. CNA and Sharp Runtime stayed read-only.

### 2026-10-07 — East-clearing fallen-log landmark

- Original-resolution clearing review showed that the east room was visually sparse relative to
  the west room. Added one 7.1 m cut-ended log to the east clearing's outer ring, reusing an
  existing mesh variant, shader, LOD, culling, instancing, contact-shadow, and collision path.
- The first actual WebGL 2 capture placed the log nearly broadside and made it read as a horizontal
  beam. Rotated it into the view depth while retaining a visible cut end. The final east frame is
  compositionally balanced, and the central walking room remains open.
- The deterministic world now has 13 fallen logs, 827 static proxies, 482 occupied collision
  cells, and fingerprint `56f46a1dd86ebf89`. Core tests prove both authored log footprints remain
  in their respective clearings and outside the central room. No mesh, texture, shader, allocation
  class, render pass, or draw submission was added.
- The actual Chrome 152/SwiftShader suite passes all 22 stages and the east/west frames were
  inspected. A bounded overlay-ready wait was raised from 5 to 20 seconds after one slow reef
  startup exposed a harness race; the required diagnostic was not weakened. The two-worker arm64
  GLES 3 build/lint and APK v2 verification pass. WebAssembly is 8,498,615 bytes; the APK is
  18,712,004 bytes with a 15,901,720-byte stripped library. The verified Web package is 8,763,458
  bytes uncompressed and 2,484,397 bytes through gzip. Physical-device appearance and cost remain
  unqualified; CNA and Sharp Runtime stayed read-only.

### 2026-10-07 — Current-source ASan/UBSan requalification

- Recreated the host-only `linux-sanitize` preset against the current source while keeping the
  graphical application and all dependency targets out of the instrumented build. Configuration
  and the two-worker build used the required shared `/rv/cnaccache`.
- All seven registered tests pass in 50.51 seconds: core simulation/invariants, deterministic
  world and audio reports, asset provenance, capture-validator unit cases, deterministic Web
  package round-trip/integrity, and independently parsed WAV preview export. AddressSanitizer and
  UndefinedBehaviorSanitizer reported no finding.
- LeakSanitizer remains intentionally disabled under the managed process tracer and retains its
  separately documented ordinary-terminal command. The 45 MB reproducible `build-asan/` tree was
  removed after qualification. CNA and Sharp Runtime stayed read-only.

### 2026-10-07 — Persistent ambient-loop DC centering

- The exact-source report showed that all four long-lived beds had small nonzero means, with the
  canopy breeze reaching 0.00646980. Added an allocation-free mean measurement/subtraction before
  peak normalization and PCM16 quantization only for persistent loops. Silent-ended one-shots keep
  their existing path and bit-identical PCM.
- The normalized loop means are now at most approximately 0.00000002. Peak magnitudes remain
  0.56--0.82 for the persistent sources, RMS energy remains valid, and seam steps remain
  0.0055--0.0268. The report and core test independently enforce an absolute loop mean no greater
  than 0.0001. PCM footprint, source count, voice ceiling, allocations, and per-frame work are
  unchanged.
- Core/audio/report/WAV validation passes 3/3. The complete actual Chrome 152/SwiftShader suite
  passes all 22 stages and requires the exact post-gesture DC-centering diagnostic. The two-worker
  arm64 GLES 3 build/lint and APK v2 verification pass. WebAssembly is 8,498,907 bytes; the APK is
  18,712,372 bytes with a 15,902,088-byte stripped library. The deterministic Web package is
  8,763,750 bytes uncompressed and 2,484,733 bytes through gzip. Subjective headphone/speaker
  balance remains open; CNA and Sharp Runtime stayed read-only.

### 2026-10-07 — KVM Android audio-gate requalification

- Confirmed the operator's repaired `/dev/kvm` node is device `10:232`, the current non-root shell
  belongs to group `kvm`, and Emulator 36.5.11 reports KVM version 12 usable. The first launch
  exposed a stale Quick Boot snapshot crash (exit 139); a non-destructive retry with snapshot
  loading disabled booted the API 35 `Medium_Phone` AVD in 14.908 seconds with host AMD Radeon 780M
  gfxstream.
- The first strict audio run exposed a game-side validation gap: SDLActivity does not place native
  stdout in logcat, so the existing audio gate could not observe successful `Soundscape` startup.
  `Soundscape` now mirrors the three bounded high-value success diagnostics through the existing
  Android-only platform logger, while desktop and Web retain their unchanged stdout path. The gate
  now also requires the DC-centering diagnostic. No CNA or Sharp Runtime defect is claimed.
- A rebuilt x86_64 APK passed the actual GLES 3 plus audio runtime gate. Logcat proves 44.1 kHz
  stereo AAudio startup, bounded sequential source staging, DC-centred persistent PCM, ambient gain
  variation, `OPENGLES3`, and the exact 1280 by 720 logical viewport. Startup measured 515.618 ms
  world, 1.996 ms framework, 1099.860 ms content, 19.109 ms pre-draw, 45.149 ms first draw, and
  1681.732 ms total. The inspected 2400 by 1080 screenshot retains the complete scene and clean
  translucent touch HUD with no fatal or shader diagnostic. The emulator was stopped cleanly and
  the default arm64 package was restored afterward; it builds/lints, verifies with APK v2, and is
  18,712,724 bytes with a 15,902,440-byte stripped library. The native OpenGL 3.3 object compiles,
  and the complete actual Chrome/SwiftShader WebGL 2 suite passes all 22 stages. The final
  8,498,883-byte WebAssembly package is 8,763,726 bytes uncompressed and 2,484,599 bytes through
  deterministic gzip. Physical-device audio quality remains open.

### 2026-10-07 — East-clearing warm-ground distinction

- The current original-resolution clearing pair showed that the east room's intended sunny/dry
  ground still read nearly as dark and green as surrounding forest soil, while the west room's
  leaf-rich identity was already clear. Strengthened only the existing east-clearing colour blend
  and reused both already-computed broad and medium noise fields for restrained mottling. The mask,
  shared clearing definition, soft edge, dapple suppression, litter reduction, short grass,
  walkable centre, geometry, textures, collision, instances, and draw submissions are unchanged.
- The same 500 by 260 review crop changed from normalized mean RGB 0.2482/0.3774/0.1687 to
  0.2794/0.3817/0.1654. Original-resolution before/after inspection shows a clearer warm olive room
  without a hard circular decal or sand-like yellow cast; the west room remains visibly cooler and
  leaf-richer.
- The OpenGL 3.3 terrain object compiles through `/rv/cnaccache`. The complete actual Chrome 152/
  SwiftShader suite passes all 22 stages, and its east gate requires the new exact warm-dry cue
  diagnostic. The two-worker arm64 GLES 3 package builds/lints and verifies with APK v2. The APK is
  18,712,820 bytes with a 15,902,536-byte stripped library; WebAssembly is 8,499,046 bytes and its
  deterministic package is 8,763,889 bytes uncompressed / 2,484,496 bytes through gzip. Physical-
  mobile appearance/cost remain open; CNA and Sharp Runtime stayed read-only.

### 2026-10-07 — Fuller near-palm leaflet silhouettes

- Original-resolution close-palm review showed that the segmented near crowns remained too
  skeletal from first-person height. Extended only each existing leaflet's two root vertices
  modestly along the rachis and outward; tips, wind, colours, placement, collision, and the far
  widened-ribbon tier are unchanged.
- Before/after frames were inspected at original resolution. The retained crown has smaller empty
  gaps and a fuller feathered silhouette while every tapered leaflet remains visibly separate; it
  does not return to the former solid paddle-shaped ribbon. The 494--566-triangle near variants,
  instance data, buffers, draws, culling, and 125--145 m per-palm LOD policy are unchanged.
- The OpenGL 3.3 palm object compiles through `/rv/cnaccache` without relinking the operator-visible
  game. The complete actual Chrome 152/SwiftShader WebGL 2 suite passes all 22 stages and requires
  the exact fuller-leaflet diagnostic. The two-worker arm64 GLES 3 package builds/lints and verifies
  with APK signature scheme v2. The APK remains 18,712,820 bytes with a 15,902,536-byte stripped
  library; WebAssembly is 8,499,057 bytes and its deterministic package is 8,763,900 bytes
  uncompressed / 2,484,361 bytes through gzip. Physical-mobile appearance and moving LOD review
  remain open; CNA and Sharp Runtime stayed read-only.

### 2026-10-07 — Near-canopy procedural bark fissures

- Original-resolution jungle review showed that the closest broadleaf trunks remained uniformly
  brown and repeated despite their varied geometry. Added restrained local-space longitudinal
  grain and narrow winding fissures to the existing shared jungle fragment shader. An existing
  colour relationship isolates brown bark, and a 38--68 m fade removes the detail before it can
  alias at the far tier.
- The pass adds one interpolated local position and bounded fragment math. It adds no texture,
  vertex, index, instance, buffer, CPU update, collision change, draw submission, or change to the
  105--129 m canopy LOD policy. Original-resolution full-frame and close-crop review shows readable
  variation on the nearest trunk without a strong repeated stripe pattern; distant trunks remain
  calm.
- The OpenGL 3.3 jungle object compiles through `/rv/cnaccache` without relinking the operator-
  visible game. The complete actual Chrome 152/SwiftShader WebGL 2 suite passes all 22 stages and
  requires the exact bark diagnostic. The two-worker arm64 GLES 3 package builds/lints and verifies
  with APK signature scheme v2. The APK is 18,713,604 bytes with a 15,903,320-byte stripped library;
  WebAssembly is 8,499,873 bytes and its deterministic package is 8,764,716 bytes uncompressed /
  2,484,634 bytes through gzip. Physical-mobile appearance, aliasing, and fragment cost remain
  open; CNA and Sharp Runtime stayed read-only.

### 2026-10-07 — Distance-scheduled sand and soil footsteps

- Added four original deterministic 0.3-second/6,615-sample PCM16 clips: paired dry-sand and
  interior-soil footsteps. The renderer-independent scheduler measures actual resolved horizontal
  player travel rather than requested input or elapsed-frame cadence, interpolates stride from
  2.05 m walking to 2.75 m running, selects sand within the 18 m beach belt and soil inland, and
  applies bounded variant/pitch/pan/gain variation. Swimming, blocked travel, invalid input, and
  teleport-sized discontinuities cannot fabricate a step.
- Reused one transient instance and preserved the hard four-persistent-plus-two-transient source
  ceiling. Water crossings retain priority; footsteps defer under full transient load and cannot
  grow the six-source budget. F1 now reports `STEPS`. The native audit validates 16 unique clips
  and 3,336,164 PCM bytes. Sand A/B measure 0.1257/0.1292 RMS with 0.58 peaks; soil A/B measure
  0.1232/0.1378 RMS with 0.62 peaks; all four have exact silent endpoints.
- Core scheduler/signal regressions, the audio report, independently parsed WAV export, asset
  provenance, and deterministic Web package tests pass 5/5. The final Chrome 152/SwiftShader
  WebGL 2 suite passes all 22 stages and requires both footstep-resource initialization and actual
  playback during real W/mouse traversal. The harness now explicitly focuses the input tab,
  measures yaw during the simultaneous-input segment, and polls lifecycle recovery instead of
  relying on fragile end-state or fixed-delay assumptions; no game requirement was weakened.
- A current x86_64 APK passed the actual KVM/host-GPU GLES 3 plus strict audio/footstep gate. A
  sustained 2400 by 1080 movement-zone gesture advanced the player and produced the first-footstep
  platform-log diagnostic while 44.1 kHz stereo AAudio remained active. The inspected screenshot
  is clean and no fatal, shader, or `GL_INVALID_OPERATION` diagnostic appeared. The emulator was
  stopped and the default arm64-only APK restored, built/linted through the required shared
  `/rv/cnaccache`, and verified with APK signature scheme v2.
- Final artifacts are 8,512,564 bytes WebAssembly, 8,777,407 bytes for the deterministic Web
  payload, 2,488,587 bytes through gzip, 15,913,048 bytes for stripped arm64 `libmain.so`, and
  18,723,332 bytes for the arm64 APK. Subjective sand/soil identity, balance, and long-session
  fatigue remain open. CNA and Sharp Runtime stayed read-only.

### 2026-10-07 — Reused-slope sunward ocean crests

- Original-resolution calm-water review found that the broad near field still had little
  directional definition. Added a narrow sun-facing window of the five-wave analytic slope already
  calculated for the ocean normal. It fades from 85 to 245 m and is attenuated by Fresnel plus
  shallow-water weighting, so nearby water gains short subdued crests while the horizon remains
  quiet.
- The accepted fragment path adds one planar normalization, one dot product, three smooth steps,
  and bounded colour shaping. It adds no wave sine, texture, varying, vertex, index, allocation,
  CPU update, or draw submission. OpenGL 3.3, GLSL ES 300/WebGL 2, and GLES 3 use the same bounded
  design through explicit shader-language variants.
- The exact-current Chrome 152/SwiftShader suite passes all 22 stages and requires the feature in
  lifecycle, calm-water, and offshore-current gates. Its original-resolution water/current frames
  were inspected without a repeated bright band. The current native OpenGL 3.3 water capture was
  also inspected, and the complete isolated-X11 suite passes 41/41, including real simultaneous
  movement/mouse input and persistent Shift run/walk toggling.
- A current x86_64 APK passed a KVM/host-gfxstream GLES 3 water-view gate that requires the exact
  platform-log diagnostic. Its inspected 2400 by 1080 frame retains the subdued crest, calm
  horizon, broken transition ring, and clean touch HUD. It reports `OPENGLES3`, 1280 by 720 logical
  rendering, and 3997.896 ms cold first draw total without fatal, shader, or
  `GL_INVALID_OPERATION` diagnostics. The emulator was stopped cleanly.
- Full-suite execution exposed three stale CTest regexes rather than runtime failures: native
  capture may validly observe a nonzero transition at phase zero, the current report includes the
  intervening splash fields, and movement diagnostics now name the leading depenetration counter.
  The gates now match the full emitted schemas while still requiring nonzero water effects,
  current/undertow, obstacle resolution, and sliding.
- The default arm64-only APK was restored, builds/lints through `/rv/cnaccache`, and verifies with
  APK signature scheme v2. Final artifacts are 8,513,315 bytes WebAssembly, 8,778,158 bytes for the
  deterministic Web payload, 2,489,351 bytes through gzip, 15,913,688 bytes for stripped arm64
  `libmain.so`, and 18,723,972 bytes for the arm64 APK. Physical-mobile appearance and isolated
  fragment cost remain open. CNA and Sharp Runtime stayed read-only.

### 2026-10-07 — Sparse near-grass seed-head silhouettes

- Original-resolution close-grass review found that the improved curved blades and juvenile
  underlayer still ended in a mostly uniform fine silhouette. Four of the 29 mature blades in the
  shared near cluster now carry a narrow stem and six sparse single-triangle spikelets. Their
  restrained green/gold variation is deterministic and they inherit the existing packed facing,
  per-blade phase, wind displacement, opaque material, and instanced draw.
- The shared near mesh rises from 340 vertices/272 triangles to 428 vertices/304 triangles. Middle
  and far meshes remain unchanged. The fixed close view still uses 21 grass submissions and adds
  30,624 triangles across its 957 visible near clusters, taking total visible grass geometry from
  310,444 to 341,068 triangles, or 9.86 percent. No texture, transparency, instance attribute,
  buffer, population, allocation, culling distance, or draw submission was added.
- The current OpenGL 3.3 grass frame was inspected and the complete isolated-X11 suite passes
  41/41, including actual simultaneous movement/mouse input and persistent Shift run/walk toggling.
  The exact Chrome 152/SwiftShader WebGL 2 suite passes all 22 stages, requires the new diagnostic,
  and its original-resolution grass frame was inspected. A loaded SwiftShader run exposed a fixed-
  duration input-test assumption: the old key hold sometimes sampled acceleration before reaching
  the target. The harness now polls observed run/walk samples while retaining strict persistent-
  mode and 7.0/3.5 m/s speed requirements.
- A current x86_64 APK passed the KVM/host-gfxstream GLES 3 grass-view gate with the exact platform-
  log diagnostic. Its inspected 2400 by 1080 frame retains sparse heads, varied blades, and clean
  touch controls; it reports `OPENGLES3`, 1280 by 720 logical rendering, and 7148.253 ms cold first
  draw total without fatal, shader, or `GL_INVALID_OPERATION` diagnostics. The slow one-run startup
  is emulator functional evidence, not a performance baseline.
- The default arm64-only APK was restored, builds/lints through `/rv/cnaccache`, and verifies with
  APK signature scheme v2. Final artifacts are 8,518,672 bytes WebAssembly, 8,783,515 bytes for the
  deterministic Web payload, 2,490,073 bytes through gzip, 15,917,208 bytes for stripped arm64
  `libmain.so`, and 18,727,492 bytes for the arm64 APK. Physical-mobile density cost, thermals, and
  sustained performance remain open. CNA and Sharp Runtime stayed read-only.

### 2026-10-07 — Irregular near-rock crowns and portable facet light

- A current twelve-view composition audit identified the close reef outcrop as a clear weak point:
  its constant-height upper ring read as a clipped plateau, and its broad planes were too uniform
  beside the more developed water and vegetation. Added bounded height variation to the existing
  upper ring plus a taller asymmetric cap. Each shared near variant remains exactly 110 vertices
  and 192 triangles; far meshes, contact disks, placement, collision, buffers, LOD, and draws are
  unchanged.
- The near-rock material branch now derives a two-sided face normal from world-position
  derivatives and applies restrained light from the same fixed environment sun. The visible
  contribution uses the existing full-strength-to-58 m, faded-to-90 m detail envelope. No normal
  stream, texture, varying, instance field, allocation, or submission was added.
- The current OpenGL 3.3 reef capture was inspected and matches the intended broken crown/readable
  planar treatment. The complete isolated-X11 suite passes 41/41, including actual simultaneous
  movement/mouse input and persistent Shift run/walk toggling. The exact Chrome 152/SwiftShader
  WebGL 2 suite passes all 22 stages, requires the crown/facet diagnostic in lifecycle, overview,
  and reef gates, and its original-resolution reef frame was inspected.
- A current x86_64 APK passed the KVM/host-gfxstream GLES 3 reef-view gate with the exact platform-
  log diagnostic. Its clean inspected 2400 by 1080 frame matches native/WebGL, reports `OPENGLES3`
  and 1280 by 720 logical rendering, and reached cold first draw in 1648.283 ms total without fatal,
  shader, or `GL_INVALID_OPERATION` diagnostics. This is functional emulator evidence, not a
  physical-mobile fragment-cost baseline. The emulator was stopped cleanly.
- The default arm64-only APK was restored, builds/lints through `/rv/cnaccache`, contains only
  `arm64-v8a`, and verifies with APK signature scheme v2. Final artifacts are 8,519,608 bytes
  WebAssembly, 8,784,451 bytes for the deterministic Web payload, 2,490,426 bytes through gzip,
  15,918,152 bytes for stripped arm64 `libmain.so`, and 18,728,436 bytes for the arm64 APK.
  Isolated native timing and physical-mobile fragment cost remain open. CNA and Sharp Runtime
  stayed read-only.

### 2026-10-07 — Layered near-bird trailing feathers

- A current monkey/bird/fish/shark capture audit found the monkey and aquatic silhouettes readable,
  while the bird view still reduced most agents to points and gave its target a very thin single-
  triangle wing outline. Added one double-sided trailing-flight-feather surface behind each near
  wing and moved only the deterministic QA camera from 17 m to 12 m for meaningful inspection.
  Runtime routes, actual bird scale, and player-facing camera behaviour are unchanged.
- The near bird grows from 42 vertices/14 triangles to 54 vertices/18 triangles. The renderer
  already reserved exactly 54 vertices per agent, so the maximum dynamic buffer and staging
  reservation do not grow. The 12-vertex/4-triangle far silhouette, population, 255 m near-LOD
  threshold, simulation tiers, culling, allocations, and one dynamic draw remain unchanged. Four
  near overview birds add only 48 streamed vertices and 16 triangles.
- The current OpenGL 3.3 bird capture was inspected and visibly retains the broader light/dark wing
  silhouette. The complete isolated-X11 suite passes 41/41, including actual simultaneous movement/
  mouse input and persistent Shift run/walk toggling. The exact Chrome 152/SwiftShader WebGL 2
  suite passes all 22 stages, requires the new diagnostic in lifecycle and bird gates, and its
  original-resolution bird frame was inspected.
- A current x86_64 APK passed the KVM/host-gfxstream GLES 3 bird-view gate with the exact platform-
  log diagnostic. Its clean inspected 2400 by 1080 frame matches native/WebGL, reports `OPENGLES3`
  and 1280 by 720 logical rendering, and reached cold first draw in 2192.963 ms total without fatal,
  shader, or `GL_INVALID_OPERATION` diagnostics. This is functional emulator evidence, not a
  physical-mobile animation-cost baseline. The emulator was stopped cleanly.
- The default arm64-only APK was restored, builds/lints through `/rv/cnaccache`, contains only
  `arm64-v8a`, and verifies with APK signature scheme v2. Final artifacts are 8,520,946 bytes
  WebAssembly, 8,785,789 bytes for the deterministic Web payload, 2,490,886 bytes through gzip,
  15,918,616 bytes for stripped arm64 `libmain.so`, and 18,728,900 bytes for the arm64 APK.
  Isolated native timing and physical-mobile animation cost remain open. CNA and Sharp Runtime
  stayed read-only.

### 2026-10-07 — Curved asymmetric fallen-frond silhouettes

- A current detail-matrix review identified the 63 grove-associated fallen fronds as overly flat
  and mirror-regular at first-person distance. Their 28 leaflet roots now follow the existing
  curved rachis. Bounded deterministic pair/side variation changes leaflet span, forward sweep,
  tip droop, and vertex-colour ageing while retaining all three broad fresh/olive/dry palettes.
- Every shared variant remains 132 vertices and 74 triangles. Population, placement, instance
  matrices, three buffers, opaque material, 76 m culling, collision policy, and maximum three
  submissions are unchanged. Original-resolution before/after inspection shows a less mechanical
  silhouette without increasing geometry or transparency.
- The complete private-display OpenGL 3.3 suite passes 41/41, including real simultaneous movement/
  mouse input and persistent Shift run/walk toggling. The actual Chrome 152/SwiftShader WebGL 2
  suite passes all 22 stages, requires the asymmetric-frond diagnostic, and its inspected frame
  matches native. The arm64 GLES 3 APK builds/lints through `/rv/cnaccache`, contains only
  `arm64-v8a`, and verifies with APK signature scheme v2. The revised fronds have not been inspected
  on physical Android hardware, so mobile appearance and cost remain open.
- Final artifacts are 8,521,483 bytes WebAssembly, 8,786,326 bytes for the deterministic Web
  payload, 2,490,885 bytes through gzip, 15,919,256 bytes for stripped arm64 `libmain.so`, and
  18,729,540 bytes for the arm64 APK. Build trees remain 2.6 GiB native, 84 MiB Web, and 360 MiB
  Android with approximately 375 GiB free. CNA and Sharp Runtime stayed read-only.

### 2026-10-07 — CNA `next` 97783d1e baseline refresh

- Advanced the reproducible CNA pin from `9fbe34468c69872204536bfe3dd6f106404d9403` to the
  workspace's clean current `next` commit `97783d1e090c6d84f4b6a6d7746516e65184569e` after reviewing
  all 11 intervening commits. The changes extend CNA's multi-renderer CMake, examples, tests, C API,
  and benchmark renderer identity; they do not change island-simulator's exercised public/runtime
  API. Sharp Runtime remains clean and locked to `next` at
  `21c06910f7cf887cceebb45200e07b43dd45fef4`. The active `BUGS.md` review found no dependency
  defect or workaround requiring re-evaluation.
- Confirmed that no process held the obsolete reproducible 321 MiB CNA export, removed only that
  generated copy, and materialized the replacement at the same `build/dependencies/cna-next` path.
  CNA, Sharp Runtime, EasyGL, and MetaGL remained read-only; no dependency source was modified.
- Reconfigured the existing `build/` tree for explicit OPENGL33, rebuilt with two workers and the
  shared `/rv/cnaccache`, and passed the complete private-display OpenGL 3.3 suite 41/41 in 51.37
  seconds. This includes actual simultaneous movement/mouse input, persistent Shift run/walk
  toggling, all deterministic captures, and renderer-independent gates.
- Reconfigured the existing `build-probe/` tree for explicit WEBGL2, rebuilt and repackaged it,
  then passed the complete actual Chrome 152/SwiftShader renderer, lifecycle, scene, audio, and
  input suite 22/22. WebAssembly is 8,521,483 bytes; the deterministic Web payload is 8,786,326
  bytes and 2,490,885 bytes through gzip. The functional SwiftShader first draw reached 443.200 ms
  total in this run; it is not a hardware-GPU or network cold-load benchmark.
- Rebuilt and linted the default arm64-v8a OPENGLES3 Android package through the same shared cache.
  It contains only `arm64-v8a`, verifies with APK signature scheme v2, and remains 18,729,540 bytes
  with a 15,919,256-byte stripped `libmain.so`. The latest Android runtime evidence remains the
  preceding `9fbe34468c69` KVM/gfxstream matrix because the current framework-only refresh was not
  rerun in the emulator; physical-device performance also remains open. All build operations used
  no more than two workers, and Chrome was capped to eight CPU cores.

### 2026-10-07 — Web control-discoverability follow-up

- A release-documentation audit found that the browser's visible pre-pointer-lock help still named
  only WASD and omitted the now-persistent Shift pace control. The English Web shell now advertises
  `WASD / arrows move` and `Shift toggles walk/run` alongside the existing mouse, camera-motion,
  diagnostics, mute, and Escape guidance. Runtime input semantics are unchanged.
- Relinked only the WEBGL2 artifact through the shared `/rv/cnaccache` with two workers while the
  operator's current native build remained open. The complete actual Chrome 152/SwiftShader suite
  again passes all 22 stages, including pointer lock, both Shift edges, simultaneous forward motion
  and relative mouse look, audio initialization, lifecycle, and all visual gates. The functional
  first draw reached 337.700 ms total in this run; it is not a hardware-browser performance claim.
- The package validation test and asset-provenance gate pass. WebAssembly remains 8,521,483 bytes;
  the longer HTML hint makes the deterministic package 8,786,361 bytes uncompressed and 2,490,911
  bytes through gzip, a 71.7 percent reduction. This Web-only presentation change does not require
  a new native or Android build. CNA and Sharp Runtime remained read-only.

### 2026-10-07 — Source-only release consistency audit

- Audited all current control descriptions, dependency-pin claims, fallback/unverified statements,
  TODO-style markers, absolute paths, CMake build-directory names, tracked file types and modes,
  and repository-local Markdown links while leaving the operator-visible native process running.
  Historical validation-ledger evidence remains historical rather than being rewritten as current.
- All project shell scripts pass `bash -n`; the Web harness passes Node syntax validation. No
  unexpected tracked binary, invalid local Markdown target, forbidden build directory, disabled
  shared-cache instruction, or source TODO/FIXME/HACK marker was found. The apparent temporary and
  placeholder matches are test-owned temporary directories or explicit limitation/history text.
- Corrected the current architecture text that still described Android sunny-sky runtime as open:
  the KVM/API 35 gfxstream matrix visibly executes that full path, while physical-device appearance
  and cost remain explicitly unqualified. `BUGS.md` still has no confirmed CNA or Sharp Runtime
  defect, all game-side fallbacks retain diagnostics, and both dependencies remained read-only.

### 2026-10-07 — Reused-noise clearing-floor patch breakup

- A current first-person composition review found that the open east/west clearing floors still
  read as broad uniform colour fields between grass clusters. The terrain shader now derives
  complementary dry/cool and leaf/moss patch masks from its already-computed broad, medium, and
  fine fields. Four bounded colour blends add no texture, new noise evaluation, varying, geometry,
  allocation, collision rule, or draw submission.
- Inspected three successive original-resolution east/west WebGL 2 pairs and rejected the first two
  as too subtle. The retained pass remains softly irregular rather than becoming a hard decal. The
  final actual Chrome 152/SwiftShader suite passes all 22 stages and requires the exact diagnostic
  in lifecycle plus both clearing gates. WebAssembly is 8,522,600 bytes; the deterministic package
  is 8,787,478 bytes uncompressed and 2,491,353 bytes through gzip.
- The changed OpenGL 3.3 consumer object compiles through `/rv/cnaccache` with first-party warnings
  as errors, without relinking or interrupting the operator-visible prior executable. Core, asset,
  and Web-package gates pass 3/3. Native runtime appearance remains deferred until that process is
  closed.
- The default arm64-v8a OPENGLES3 APK builds and passes application lint through the shared cache,
  contains only `arm64-v8a`, and verifies with APK signature scheme v2. It is 18,730,596 bytes with
  a 15,920,312-byte stripped `libmain.so`. Android runtime appearance and physical-device fragment
  cost remain unclaimed. All builds used no more than two workers; Chrome was capped to eight CPU
  cores. CNA and Sharp Runtime stayed read-only.

### 2026-10-07 — Current-pin Android clearing, touch, and lifecycle qualification

- Rebuilt the current `97783d1e090c` source as x86_64 through `/rv/cnaccache`, cold-booted the API
  35 `Medium_Phone` AVD in 14.457 seconds with KVM and host-GPU gfxstream, and passed the strict
  GLES 3/audio gate at the fixed east clearing. The inspected 2400 by 1080 frame reports
  `OPENGLES3`, the 1280 by 720 logical viewport, 33.6 FPS at capture, clean translucent touch
  controls, and the retained non-decal-like clearing treatment. First draw completed in 1594.265
  ms total; this is functional emulator evidence, not a physical-device performance result.
- The first attempt exposed a game-owned validation gap: ordinary C++ stdout reaches desktop/Web
  tests but is not an authoritative Android platform log. The clearing marker now uses the
  existing `MirrorRuntimeDiagnosticToPlatformLog` game-side abstraction, matching the established
  ocean, grass, prop, bird, audio, renderer, and viewport gates. The repeated Android run passed;
  no CNA or Sharp Runtime change, workaround, or `BUGS.md` entry was required.
- A second ordinary launch passed the strict audio/footstep gate. Its injected full-travel touch
  moved the player to `z=-204.9`, reached interaction strength 1.0, and played eight footsteps.
  A subsequent Home/resume retained PID 4218, recreated the GLES surface in a 35 ms hot launch,
  and moved the same AAudio stream from started to paused and back to started without a fatal,
  shader, or `GL_INVALID_OPERATION` diagnostic.
- Stopped the emulator cleanly and restored the default arm64-only APK. The package builds/lints,
  contains only `arm64-v8a`, verifies with APK signature scheme v2, is 18,730,692 bytes, and
  contains a 15,920,408-byte stripped `libmain.so`. The complete current Chrome 152/SwiftShader
  suite also passes 22/22 after the portable diagnostic call; WebAssembly is 8,522,727 bytes and
  the deterministic package is 8,787,605 bytes uncompressed or 2,491,600 bytes through gzip.
  The OpenGL 3.3 object compiles with first-party warnings as errors, but the native executable was
  deliberately not relinked while the operator-visible game remained open. Native runtime review
  of the retained clearing pass and physical-device Android qualification remain open. All builds
  used two workers or fewer, Chrome was limited to eight CPU cores, and both dependencies remained
  read-only.

### 2026-10-07 — Firefox ESR second-engine qualification

- Added `tools/run_firefox_web_smoke.mjs`, a dependency-free Linux gate for the generated production
  page. It starts a local-only server, a disposable Firefox profile and the browser's built-in
  WebDriver BiDi endpoint; Firefox is capped to CPU cores 0--7. Bounded cleanup closes the session,
  terminates both child processes, and removes the profile on success or failure. `ISLAND_FIREFOX`
  supports installations named `firefox` rather than `firefox-esr`.
- Six clean Firefox ESR 140.15 runs passed on current source. The gate requires `WEBGL2`, the 1280
  by 720 logical viewport, first-draw/visible-scene diagnostics and current clearing/ocean shader
  paths. Firefox reported `WebGL 2.0` on a `Radeon R9 200 Series, or similar` backend; first draw
  completed in 299--360 ms total across the six tool runs. These are functional local-load
  values, not network, sustained-frame, or portable hardware benchmarks.
- The trusted production-canvas click acquired pointer lock and initialized the full 44.1 kHz
  audio resource path. A released Shift edge selected persistent 8.0 m/s running, forward movement
  produced a real land footstep, a second Shift edge restored 4.3 m/s walking, and Escape released
  pointer lock. The original-resolution before/input frames were inspected; the latter visibly
  retains the world plus the warm-gold `RUN MODE  8.0 M/S` confirmation. No fatal or shader
  diagnostic appeared.
- The complete 22-stage Chrome/SwiftShader matrix remains authoritative for all fixed scenes,
  simultaneous mouse look, clipboard, and shoreline audio. Firefox qualifies the production
  first-frame and essential input/audio subset plus responsive sizing and pointer-lock
  reacquisition; visible interactive listening, fullscreen, real tab lifecycle, simultaneous
  mouse look, and broader Firefox scene coverage stay open. Added `/.aws` to `.gitignore` because
  the managed sandbox exposes that name as an
  environment-owned read-only credential mount, preventing it from appearing as project content.
  CNA and Sharp Runtime remained read-only.

### 2026-10-07 — Firefox responsive-shell and pointer reacquisition follow-up

- Extended the dependency-free Firefox gate through the production page's responsive shell. A
  1000 by 600 browser viewport retains the 1280 by 720 render backing store and presents a 1000 by
  563 client canvas, preserving 16:9 without overflow or distortion.
- Firefox released pointer lock on Escape, reacquired it from a delayed trusted click without a
  reload, and released the reacquired lock on a second Escape. The sixth clean current-source run
  passes every earlier renderer, audio, persistent-Shift, traversal, footstep, and screenshot gate
  plus these new assertions. Fullscreen, real tab suspension/resume, visible listening, sustained
  frame pacing, and the complete second-engine scene matrix remain explicitly unqualified.

### 2026-10-07 — Web runtime-asset packaging closure

- Resolved the stale virtual-filesystem task against the actual source model: all current runtime
  content is generated from compiled project code, so the complete Web product is the existing
  HTML, JavaScript, and WebAssembly trio. Shipping an empty Emscripten `.data` file would add a
  request and failure mode without packaging content. The Web instructions now make this boundary
  and the required future file-backed-asset path explicit.
- The schema-2 provenance gate passes with 17 procedural records, 32 generator sources, one hashed
  original Android-only project file, and zero external files. The deterministic Web packaging
  test passes and still requires exactly the three deployable artifacts with integrity and gzip
  round-trip coverage. No runtime, dependency, or build output changed.

### 2026-10-07 — Transform-derived jungle foliage tint

- A current jungle/ground/clearing capture review found that the four geometry families still
  shared overly uniform greens across repeated instances. The portable jungle vertex shader now
  recombines four scale/yaw coefficients from each existing instance transform into a stable
  bounded tint seed. A green-only mask preserves brown bark, while the shared transform preserves
  colour identity across near/far LOD. Placement, collision, geometry, instance stride, buffers,
  textures, culling, populations, and draw submissions are unchanged.
- Original-resolution Chrome/WebGL 2 before/after jungle, ground, and east-clearing pairs were
  inspected. The retained pass is deliberately restrained but visibly breaks up repeated foliage
  colour without spatial striping, bark contamination, or composition changes. The complete
  Chrome 152/SwiftShader suite passes all 22 stages and requires the new diagnostic in lifecycle
  and jungle gates. The sixth Firefox ESR 140.15 run independently compiled and rendered the same
  GLSL ES 300 path and passed production sizing, pointer-lock/reacquisition, persistent Shift,
  traversal/footstep, audio, and double-Escape release checks.
- A current x86_64 APK passed the strict KVM/host-gfxstream GLES 3 plus audio gate at the fixed
  jungle camera. Its inspected 2400 by 1080 frame retains all four vegetation layers, unchanged
  brown trunks, clean translucent touch controls, and the 1280 by 720 logical viewport. First draw
  completed in 1586.194 ms total without fatal, shader, or `GL_INVALID_OPERATION` diagnostics.
  This is functional emulator evidence, not a physical-mobile vertex-cost baseline. The emulator
  was stopped cleanly.
- Restored the default arm64-only APK, built/linted it through `/rv/cnaccache`, confirmed only
  `arm64-v8a`, and verified APK signature scheme v2. WebAssembly is 8,523,762 bytes; the
  deterministic Web payload is 8,788,640 bytes or 2,491,885 bytes through gzip. The stripped arm64
  `libmain.so` is 15,921,272 bytes and the APK is 18,731,556 bytes. The changed OpenGL 3.3 object
  compiles with first-party warnings as errors, but native runtime appearance remains deferred
  while the operator-visible earlier executable is open. All compilation used two workers or
  fewer and the shared cache; CNA and Sharp Runtime stayed read-only.

### 2026-10-07 — Firefox jungle, water, and tab-resume coverage

- Extended the dependency-free Firefox/WebDriver BiDi gate beyond the production first-frame and
  essential-input path. The tool now materializes the same ignored test-only shell as the Chrome
  harness, navigates the existing context to deterministic jungle and near-surface-water cameras,
  waits for their complete live visibility state, and writes two additional 1280 by 720 frames.
- The jungle gate requires nonzero near/far populations for canopy, understory, bushes, and ground
  plants plus the portable wind, facet-light, and transform-derived tint diagnostics. The water
  gate requires the sunward-crest and generated near-surface-view paths plus simultaneous nonzero
  view, ripple, and entry/exit-burst state. Both scenes reject test-shell-reported, fatal, and
  shader errors.
- Three consecutive expanded Firefox runs passed. In the ninth clean run, WebDriver BiDi activated
  a real second tab: the game document reported `visible` to `hidden` to `visible`, its bounded
  animation probe advanced from 12 frames before backgrounding to 38 after foregrounding, and two
  fresh Shift edges proved that the game update/input path resumed. This qualifies one short tab
  cycle, not long suspension or audio-focus recovery.
- The final run retained the earlier production sizing, trusted pointer-lock/reacquisition,
  traversal/footstep, audio, double-Escape, jungle, and water gates. Firefox again reported
  `WebGL 2.0` on a `Radeon R9 200 Series, or similar` backend; first draw was 308 ms total, within
  the retained 289--360 ms functional range. The four original-resolution production/input/
  jungle/water frames were inspected and match the established composition. The complete 22-stage
  Chrome suite remains authoritative for the other fixed scenes, simultaneous mouse look,
  clipboard, and shoreline audio. Fullscreen, visible listening, extended tab suspension/audio
  focus, and sustained hardware-GPU frame pacing remain open.

### 2026-10-07 — Transform-derived palm-frond tint

- A close-palm capture review found that four geometry variants still shared one literal crown
  green. The portable palm vertex shader now recombines four existing scale/yaw transform
  coefficients into a stable bounded tint seed. A green-dominance mask preserves brown trunks,
  roots, and coconuts; the shared transform preserves one tint identity through near/far LOD.
  Instance stride, buffers, textures, geometry, placement, collision, culling, populations, and
  draw submissions are unchanged.
- Rejected the first weaker amplitude after an original-resolution before/after WebGL 2 review.
  The retained restrained pass visibly reduces cloned crown colour without overpowering the four
  silhouettes or contaminating structural material. The complete Chrome 152/SwiftShader suite
  passes 22/22 and requires the exact tint diagnostic in lifecycle and palm gates. The eleventh
  clean Firefox ESR 140.15 run independently compiled and rendered the current GLSL ES 300 path
  while
  retaining production pointer-lock/reacquisition, persistent Shift, traversal/footstep, audio,
  short tab hidden/visible/resumed-input, jungle, and water gates; first draw was 302 ms total.
- A current x86_64 APK passed the strict fixed-palm KVM/host-gfxstream GLES 3 and audio gate. The
  inspected 2400 by 1080 frame reports `OPENGLES3`, the 1280 by 720 logical viewport, varied green
  crowns, unchanged brown structural material, and clean touch controls. First draw completed in
  1605.876 ms total without fatal, shader, or `GL_INVALID_OPERATION` diagnostics. This is portable
  functional evidence, not physical-device appearance or cost qualification.
- Restored and linted the default arm64-only APK through `/rv/cnaccache`, confirmed only
  `arm64-v8a`, and verified APK signature scheme v2. The stripped `libmain.so` is 15,922,104 bytes
  and the APK is 18,732,388 bytes. WebAssembly is 8,524,788 bytes; the deterministic Web payload
  is 8,789,665 bytes or 2,491,968 bytes through gzip. Seven renderer-independent/package gates
  pass. The changed OpenGL 3.3 object compiles with first-party warnings as errors, but the native
  executable remains deliberately unlinked while the operator-visible earlier build is open.
  All compilation used two workers or fewer and the shared cache; CNA and Sharp Runtime stayed
  read-only.

### 2026-10-07 — Irregular fallen-log broken ends

- An original-resolution close-prop review found that the three near fallen logs still ended in
  visibly perfect planar circles, making the natural objects read as clean cylinders. Their
  existing nine-sided side rings and duplicated cut-cap rims now share the same deterministic
  radius and receive bounded variant/end/side axial offsets. This spatially closes each irregular
  non-planar contour while retaining the established growth-ring material.
- No vertex, index, material selector, instance data, buffer, texture, collision proxy, placement,
  culling rule, population, upload, or draw submission was added; the far LOD remains unchanged.
  The OpenGL 3.3 object compiles through `/rv/cnaccache` with first-party warnings as errors, but
  the executable remains deliberately unlinked while the operator-visible prior build is open.
- The complete Chrome 152/SwiftShader suite passes 22/22 and requires the new marker in lifecycle
  and prop gates. Original-resolution before/after prop frames were inspected; the retained result
  breaks the artificial circular silhouette without losing readable rings or the existing contact
  shadow. The twelfth clean Firefox ESR 140.15 run independently compiled the GLSL ES 300 path and
  retained all production input/audio, short tab-resume, jungle, and water gates; first draw was
  401 ms total.
- A current fixed-prop x86_64 APK passed the strict KVM/host-gfxstream GLES 3 plus audio gate. Its
  inspected 2400 by 1080 frame reports `OPENGLES3`, the 1280 by 720 logical viewport, and visibly
  matches the WebGL contour; first draw completed in 1567.410 ms without fatal, shader, or
  `GL_INVALID_OPERATION` diagnostics. The emulator was stopped cleanly. The restored arm64-only
  APK builds/lints, contains only `arm64-v8a`, verifies with signature scheme v2, is 18,733,620
  bytes, and contains a 15,923,336-byte stripped `libmain.so`. WebAssembly is 8,524,663 bytes;
  the deterministic Web package is 8,789,541 bytes or 2,492,117 bytes through gzip. This is
  functional translated-host evidence, not physical-device performance qualification. All builds
  used two workers or fewer and the shared cache; CNA and Sharp Runtime stayed read-only.

### 2026-10-07 — Reused-slope grazing ocean reflections

- Original-resolution offshore-current, near-surface-water, and reef review found that the calm
  broad surface still read too uniformly at low viewing angles. The ocean fragment shader now
  projects its already-computed analytic slope toward the horizontal eye direction, smoothly
  saturates that signed value, and adds a bounded cyan reflection variation only at grazing angles.
  Existing 135--420 m detail fade and shallow weighting keep the horizon stable and the lagoon
  restrained. A weaker first tuning was inspected and rejected before retaining the final result.
- The change adds only fragment arithmetic. It introduces no sine, wave field, texture lookup,
  varying, uniform, geometry, allocation, resource, or draw submission. The OpenGL 3.3 object
  compiles through `/rv/cnaccache` with first-party warnings as errors but is deliberately not
  relinked while the operator-visible prior executable remains open.
- The complete Chrome 152/SwiftShader WebGL 2 suite passes 22/22 and requires the exact reflection
  marker in lifecycle, water, and current gates. Before/after current, water, and reef frames were
  inspected at original resolution. The thirteenth clean Firefox ESR 140.15 run independently
  compiles the GLSL ES 300 path and retains pointer-lock/relock, persistent Shift, footstep/audio,
  tab-resume, jungle, and water gates; first draw was 357 ms total.
- A current fixed-current x86_64 APK passed the strict KVM 12/host-gfxstream GLES 3 plus audio gate.
  Its inspected 2400 by 1080 frame reports `OPENGLES3`, the 1280 by 720 logical viewport, current
  0.7 and drift 0.3/1.2, and visibly retains the low-angle surface breakup. First draw completed in
  1677.445 ms without fatal, shader, or `GL_INVALID_OPERATION` diagnostics. This is functional
  translated-host evidence, not physical-device performance qualification.
- The emulator was stopped cleanly. The restored arm64-only APK builds/lints through the shared
  cache, contains only `arm64-v8a`, verifies with APK signature scheme v2, is 18,734,196 bytes, and
  contains a 15,923,912-byte stripped `libmain.so`. WebAssembly is 8,525,401 bytes; the
  deterministic Web package is 8,790,279 bytes or 2,493,211 bytes through gzip. Seven
  renderer-independent/package gates pass. Build trees remain approximately 2.6 GiB native,
  87 MiB Web, and 361 MiB Android with 374 GiB free; shared ccache is 3.7 GiB with 99.67 percent
  cacheable calls. All builds used two workers or fewer; CNA and Sharp Runtime stayed read-only.

### 2026-10-07 — Transform-derived per-instance rock tint

- Current clearing and reef review found that repeated copies of each shared rock variant retained
  one literal palette. The natural-prop vertex shader now selects neutral stone-family colours by
  their existing relative RGB channel span and recombines four existing scale/yaw transform coefficients
  into a bounded signed warm/cool tint. Red-dominant wood and green foliage remain outside the
  mask, and the shared transform preserves one tint identity through near/far LOD.
- The change adds only vertex arithmetic. It adds no attribute, instance stride, buffer, texture,
  geometry, population, collision proxy, upload, culling rule, or draw submission. The OpenGL 3.3
  object compiles through `/rv/cnaccache` with first-party warnings as errors but remains
  deliberately unlinked while the operator-visible prior executable is open.
- The complete Chrome 152/SwiftShader WebGL 2 suite passes 22/22 with the exact marker required in
  lifecycle, overview, prop, and reef gates. Original-resolution before/after prop and reef frames
  were inspected; the retained result adds a restrained warm/olive stone identity without changing
  the fixed log material. The fifteenth clean Firefox ESR 140.15 run independently compiles the
  GLSL ES 300 path and retains production input/audio, tab-resume, jungle, and water gates; first
  draw was 304 ms total.
- A fixed-reef x86_64 APK passed the strict KVM 12/host-gfxstream GLES 3 plus audio gate with the
  exact tint marker. Its inspected 2400 by 1080 frame reports `OPENGLES3`, the 1280 by 720 logical
  viewport, and visibly retains the warm/olive rock tone, facet light, wet band, and translucent
  touch controls. First draw completed in 1604.367 ms without fatal, shader, or
  `GL_INVALID_OPERATION` diagnostics. This is functional translated-host evidence, not physical-
  device performance qualification.
- The emulator was stopped cleanly. The restored arm64-only APK builds/lints through the shared
  cache, contains only `arm64-v8a`, verifies with APK signature scheme v2, is 18,735,316 bytes, and
  contains a 15,925,032-byte stripped `libmain.so`. WebAssembly is 8,526,577 bytes; the
  deterministic Web package is 8,791,455 bytes or 2,493,652 bytes through gzip. Seven
  renderer-independent/package gates pass. Build trees remain approximately 2.6 GiB native,
  87 MiB Web, and 361 MiB Android with 374 GiB free; shared ccache is 3.7 GiB with 99.67 percent
  cacheable calls. All builds used two workers or fewer; CNA and Sharp Runtime stayed read-only.

### 2026-10-07 — Android touch shoreline-to-swimming regression

- Added a one-time game-owned diagnostic only when resolved player movement changes from walking
  to swimming. Teleporting or merely entering shallow wading depth cannot satisfy it. Android also
  mirrors the already-authoritative first water-transition playback diagnostic to platform log;
  CNA and Sharp Runtime remain untouched.
- Extended the Android runtime helper with opt-in `ISLAND_ANDROID_SMOKE_REQUIRE_SWIMMING=1`.
  Fixed QA viewpoints plus smoke/capture/benchmark modes are rejected because they deliberately
  suppress player movement. The interactive gate uses a reverse left-zone gesture from the
  deterministic south-beach spawn,
  requires the actual swimming-state transition, and additionally requires water-transition
  playback whenever strict audio qualification is selected. It composes with the existing
  forward footstep route in one process.
- Built a temporary x86_64 APK with two workers and `/rv/cnaccache`, then booted the API 35
  `Medium_Phone` AVD in 14.432 seconds using KVM 12 and host AMD Radeon 780M gfxstream. The combined
  strict GLES 3/audio/footstep/swimming gate passed: a land footstep played, the water accent
  appeared in logcat, and the real swimming transition followed 1.224 seconds later. Startup was
  1679.190 ms total (574.107 ms world, 2.586 ms framework, 1043.779 ms content, 20.960 ms
  pre-draw, 37.759 ms first draw). The inspected 2400 by 1080 screenshot visibly shows the player
  in near-shore water facing the beach and jungle with clean translucent touch controls.
- Stopped the emulator cleanly and restored the default arm64-only package. `:app:lintDebug`
  reports no issues, the APK contains only `arm64-v8a`, and APK signature scheme v2 verifies. The
  stripped `libmain.so` is 15,925,496 bytes and the APK is 18,735,780 bytes. The two changed native
  objects compile with first-party warnings as errors; full native relink/runtime remains deferred
  only because the operator-visible prior executable is still open. Project-owned build trees
  remain approximately 2.6 GiB native, 87 MiB Web, and 361 MiB Android with 374 GiB free; shared
  ccache remains 3.7 GiB and 99.67 percent cacheable.
- Rebuilt the shared WebGL 2 source and tightened Chrome's existing shoreline input stage to hold
  `S` until the same authoritative swimming diagnostic instead of stopping at shallow-water audio.
  The complete Chrome 152/SwiftShader matrix passes 22/22 with both the water accent and swimming
  state true; the final player sample rests at swimming eye height 0.58 m. Firefox ESR 140.15 also
  passes its independent production input/audio, short tab-resume, jungle, and water gates with a
  305 ms first draw. The deterministic Web package is 8,791,948 bytes or 2,493,751 bytes through
  gzip; WebAssembly is 8,527,070 bytes or 2,433,547 bytes through gzip.

### 2026-10-07 — Android hot-resume and cold-relaunch regression gate

- Extended the Android runtime helper with opt-in
  `ISLAND_ANDROID_SMOKE_REQUIRE_LIFECYCLE=1`. After its qualified initial frame and optional
  traversal, the gate sends Home, requires SDL pause/surface destruction followed by resume,
  surface recreation, nonzero window dimensions, focus reacquisition, the unchanged PID, and
  AAudio pause/start when strict audio is enabled. It allows presentation to settle before taking
  the hot-resume screenshot because Android can expose the recreated landscape surface one frame
  before system screenshot orientation catches up.
- The same gate then force-stops the package, requires the original process to disappear, launches
  a new PID, and requires a fresh `OPENGLES3` frame, 1280 by 720 logical viewport, startup report,
  and all three qualified audio generation/mix diagnostics. Hot and cold phases retain separate
  launch reports, logs, and screenshots. Invalid lifecycle configuration fails closed, and shell
  syntax validation passes.
- Rebuilt the temporary x86_64 package through `/rv/cnaccache` and passed the combined strict
  audio, footstep, shoreline-to-swimming, hot-resume, and cold-relaunch test on the API 35 KVM
  12/host-gfxstream AVD. PID 4073 survived Home/resume with surface and AAudio restoration; the
  force-stop relaunch used PID 4269. The initial and cold first draws completed in 1998.424 ms and
  1665.509 ms respectively. All three inspected captures are 2400 by 1080, retain the rendered
  world and translucent touch controls, and show no fatal, shader, or `GL_INVALID_OPERATION`
  diagnostic. This is deterministic emulator recovery evidence, not qualification of long
  suspension, OS-initiated eviction, external audio focus, or a physical Android device.
- Stopped the emulator cleanly and restored the default arm64-only APK. The package builds/lints
  with two workers through the shared cache, contains only `arm64-v8a`, verifies with APK signature
  scheme v2, is 18,735,780 bytes, and contains a 15,925,496-byte stripped `libmain.so`. Project
  build trees remain approximately 2.6 GiB native, 87 MiB Web, and 361 MiB Android with 374 GiB
  free; shared ccache remains 3.7 GiB and 99.67 percent cacheable. CNA and Sharp Runtime remained
  read-only. All seven renderer-independent core/report/asset/package tests pass.

### 2026-10-07 — Runtime-selectable touch-look sensitivity

- Added a small asset-free three-dot selector at the top right of the touch HUD. A bounded contact
  cycles standard (1.00), high (1.35), and low (0.72) look response without acquiring the look
  role; holding the selector cannot cycle repeatedly. Pointer cancellation/resize preserves the
  current choice, same-process pause/resume retains it, and a new process deliberately starts at
  standard until persistent game settings exist. The active dot is visibly brighter. Existing
  generated ring/disc textures and the same `SpriteBatch` pass are reused, so this adds no texture,
  buffer, shader, audio source, or render pass.
- Renderer-independent tests cover selector placement, role isolation, hold debouncing, the
  bounded high/low multipliers, a high-to-low cycle, synthetic-mouse suppression ownership, and
  resize retention. The core suite passes,
  and the OpenGL 3.3 `IslandGame`/`TouchOverlay` objects compile with first-party warnings as
  errors through `/rv/cnaccache`; the live operator-visible older executable was not relinked.
- Added opt-in `ISLAND_ANDROID_SMOKE_REQUIRE_TOUCH_SENSITIVITY=1`. The first attempt exposed a QA
  coordinate error: ADB's physical 2400 by 1080 surface must be mapped through CNA's centred 1280
  by 720 aspect-fit content region, and an instantaneous ADB tap can complete between game frames.
  The helper now parses both dimensions, accounts for pillarbox/letterbox margins, holds a
  stationary 500 ms contact, and requires the exact high/1.35 plus no-mouse-capture platform
  diagnostics. This was test
  automation, not a CNA/Sharp Runtime defect; no dependency change or `BUGS.md` entry was needed.
- The combined KVM 12/host-gfxstream strict audio, sensitivity, footstep, swimming, hot-resume, and
  cold-relaunch gate passed. PID 3654 selected high, entered swimming, and retained the rightmost
  highlighted dot across Home/resume; PID 3867 started cold with the standard centre dot. All
  three inspected frames are clean 2400 by 1080 landscapes. A follow-up actual input sequence
  logged high, low, and standard in order. A final focused run on PID 3332 required both the high
  selection and explicit retained-touch/no-mouse-capture diagnostics and produced another clean
  inspected 2400 by 1080 frame. Initial/cold startup totals were 1605.987 and 2179.292 ms; these
  are functional emulator observations, not physical-device performance claims.
- The rebuilt WebGL 2 game passes the complete Chrome 152/SwiftShader suite and another clean
  Firefox ESR 140.15 production/input/audio/tab-lifecycle/jungle/water gate (505 ms first draw).
  All seven renderer-independent core/report/asset/package tests pass. The deterministic Web
  package is 8,794,948 bytes or 2,495,383 bytes through gzip; WebAssembly is 8,530,070 bytes or
  2,435,185 bytes through gzip.
- Stopped the emulator cleanly and restored the default arm64-only APK. Android lint has zero
  issues, the APK contains only `arm64-v8a`, signature scheme v2 verifies, the package is
  18,739,740 bytes, and its stripped `libmain.so` is 15,929,456 bytes. Shared ccache is 3.7 GiB
  with 99.66 percent cacheable calls; project build trees remain approximately 2.6 GiB native,
  87 MiB Web, and 361 MiB Android with 374 GiB free. Every build used no more than two workers;
  CNA and Sharp Runtime remained read-only.

### 2026-10-07 — Sparse geometric forest-floor litter

- Added three small folded fallen leaves to only ground-plant variant 0's near mesh. Brown material
  colours remain outside the shader's foliage-tint mask and zero wind weights keep the litter on
  the floor. The leaves reuse the plant's existing instance transform, cell/range culling, vertex/
  index buffers, and draw submission, so the 2,225-instance population, collision, resources, and
  draw-call ceiling are unchanged. The other two ground-plant silhouettes and all far LODs remain
  untouched.
- Preserved the established split-leaf diagnostic contract after the first Chrome run correctly
  rejected a renamed marker, then added a separate exact fallen-leaf marker. A new `litter` fixed
  viewpoint deterministically selects variant 0 at player-eye detail distance without replacing
  the existing variant-2 `ground` view. Native CLI, Emscripten smoke shell, debugging documentation,
  result JSON, and screenshot naming expose the same camera.
- The changed OpenGL 3.3 objects compile with first-party warnings as errors through the shared
  cache without relinking the operator-visible running executable. The core suite and final seven
  renderer-independent core/report/asset/package tests pass. The complete Chrome 152/SwiftShader
  WebGL 2 matrix now passes 23/23 stages; original-resolution `ground`, `litter`, and jungle frames
  were inspected, and the new leaf remains a restrained contact cue rather than a repeated carpet.
- The final deterministic Web package is 8,800,498 bytes or 2,496,160 bytes through gzip;
  WebAssembly is 8,535,620 bytes or 2,435,957 bytes through gzip. JavaScript remains 262,820 bytes
  (59,059 gzip) and HTML remains 2,058 bytes (1,144 gzip).
- A temporary x86_64 package built through `/rv/cnaccache` and passed the strict GLES 3/audio gate
  at the new fixed view on the API 35 KVM 12/host-gfxstream AVD. PID 3305 reported the exact reuse
  marker, `OPENGLES3`, 1280 by 720 logical rendering, and a 1699.166 ms first-draw total. The
  inspected 2400 by 1080 frame visibly retains the subtle leaf, surrounding vegetation, and clean
  touch HUD without fatal, shader, or `GL_INVALID_OPERATION` diagnostics.
- Stopped the emulator cleanly and restored/linted the default arm64-only APK. Lint reports zero
  issues, signature scheme v2 verifies, the APK is 18,742,572 bytes, and its stripped `libmain.so`
  is 15,932,288 bytes. Shared ccache remains 3.7 GiB and 99.66 percent cacheable; project build
  trees remain approximately 2.6 GiB native, 88 MiB Web, and 361 MiB Android with 374 GiB free.
  Every build used no more than two workers; CNA and Sharp Runtime remained read-only.

### 2026-10-07 — Selected near-grass V-fold geometry

- Added a restrained geometric centre ridge to 12 of the 34 blades in the shared near-grass mesh.
  Each selected five-level blade uses left/ridge/right vertices and four triangles per segment;
  side-specific encoded facing produces a visible fixed-sun fold while the other two-thirds retain
  the established broad curved silhouette. The near cluster grows from 428 to 488 vertices and
  from 304 to 400 triangles. Middle/far meshes, 18,556 instances, cell culling, transitions,
  buffers, textures, shaders, collision, and draw submissions are unchanged.
- The changed OpenGL 3.3 object compiles with first-party warnings as errors through the shared
  cache without relinking the operator-visible running executable. The complete Chrome
  152/SwiftShader WebGL 2 matrix passes 23/23 stages with an exact V-fold diagnostic, and the
  original-resolution close-grass frame was inspected for clean tips, restrained facet response,
  and preservation of the surrounding density.
- A temporary x86_64 package built through `/rv/cnaccache` and passed the strict GLES 3/audio gate
  at the fixed grass view on the API 35 KVM 12/host-gfxstream AVD. PID 3476 reported the exact
  V-fold marker, `OPENGLES3`, a 1280 by 720 logical backbuffer, low-latency audio, and a 1791.332 ms
  first-draw total. The inspected 2400 by 1080 frame matches the Web composition without fatal,
  shader, or `GL_INVALID_OPERATION` diagnostics; this is functional emulator evidence, not a
  physical-mobile performance claim.
- The deterministic Web package is now 8,803,308 bytes or 2,496,531 bytes through gzip;
  WebAssembly is 8,538,430 bytes or 2,436,329 bytes through gzip. JavaScript remains 262,820 bytes
  (59,058 gzip) and HTML remains 2,058 bytes (1,144 gzip).
- Stopped the emulator cleanly and restored/linted the default arm64-only APK. Lint reports zero
  issues, signature scheme v2 verifies, the APK is 18,744,028 bytes, and stripped `libmain.so` is
  15,933,744 bytes. Removed only the reproducible temporary x86_64 CMake/object intermediates,
  reducing the Android tree by about 1.1 GiB. Shared ccache remains 3.7 GiB and 99.66 percent
  cacheable; all builds used no more than two workers, and CNA/Sharp Runtime remained read-only.

### 2026-10-07 — Three-dimensional driftwood branches

- Replaced the flat branch fins on near driftwood variants zero and one with the same bounded
  three-ring tapered construction already proven on variant two. Variant zero now has one bent
  limb, variant one has two opposing limbs, and variant two retains its larger three-limb fork.
  Near meshes are respectively 70/100, 89/130, and 108/160 vertices/triangles; far meshes remain
  27 vertices/30 triangles. The 31 placements, authored three-piece cove, transforms, shader,
  materials, LOD distances, culling, buffers, collision, and draw submissions are unchanged.
- Added an exact shared diagnostic to the Chrome lifecycle/driftwood gates and Android first-frame
  gate while preserving the established variant-two marker. The OpenGL 3.3 object compiles with
  first-party warnings as errors through the shared cache without relinking the operator-visible
  running executable. The full Chrome 152/SwiftShader WebGL 2 matrix passes 23/23; its original-
  resolution cove frame was inspected and retains three distinct restrained silhouettes.
- A temporary x86_64 build passed the strict GLES 3/audio gate at the fixed driftwood view on the
  API 35 KVM 12/host-gfxstream AVD. PID 3524 reported the exact all-variants diagnostic,
  `OPENGLES3`, low-latency audio, a 1280 by 720 logical backbuffer, and a 1549.511 ms first-draw
  total. The inspected 2400 by 1080 cove frame shows all three branches without fatal, shader, or
  `GL_INVALID_OPERATION` diagnostics; this remains emulator functional evidence only.
- The deterministic Web package is 8,801,831 bytes or 2,496,370 bytes through gzip; WebAssembly is
  8,536,953 bytes or 2,436,167 bytes through gzip. JavaScript remains 262,820 bytes (59,059 gzip)
  and HTML remains 2,058 bytes (1,144 gzip).
- Stopped the emulator, restored/linted the default arm64-only APK, and verified its sole ABI plus
  signature scheme v2. The APK is 18,743,708 bytes and stripped `libmain.so` is 15,933,424 bytes.
  Removed only the reproducible x86_64 CMake/object intermediates again, leaving the Android tree
  at approximately 1.3 GiB. Shared ccache remains the only compiler cache; CNA and Sharp Runtime
  stayed read-only and every build used no more than two workers.

### 2026-10-07 — Closed irregular driftwood ends

- Reused one local bend-offset and ring-radius definition for both every driftwood body ring and
  both end caps. The cap centres now follow the bent body axis and their perimeter vertices exactly
  match the first/last irregular rings, removing the former smaller 0.76-radius circular seam.
  Vertex/triangle counts, branch silhouettes, buffers, transforms, materials, instances, LOD,
  culling, collision, and draw submissions are unchanged for all near and far variants.
- Added an exact diagnostic to Chrome lifecycle/driftwood gates and the next Android runtime gate.
  The OpenGL 3.3 object compiles with first-party warnings as errors through `/rv/cnaccache`
  without relinking the operator-visible process. The complete Chrome 152/SwiftShader WebGL 2
  matrix passes 23/23 and the original-resolution cove frame was inspected without cap cracks,
  inverted silhouettes, or composition regressions.
- A batched temporary x86_64 build passed the strict GLES 3/audio gate at the fixed driftwood view
  on the API 35 KVM/host-gfxstream AVD. PID 3602 reported the exact end-cap marker, `OPENGLES3`,
  low-latency audio, a 1280 by 720 logical backbuffer, and a 1522.465 ms first-draw total. The
  inspected 2400 by 1080 frame retains three distinct closed silhouettes without fatal, shader, or
  `GL_INVALID_OPERATION` diagnostics; this is emulator functional evidence, not physical-mobile
  performance qualification.
- The deterministic Web package is 8,802,501 bytes or 2,496,562 bytes through gzip; WebAssembly is
  8,537,623 bytes or 2,436,354 bytes through gzip. JavaScript is 262,820 bytes (59,064 gzip) and
  HTML is 2,058 bytes (1,144 gzip). CNA and Sharp Runtime remained read-only; builds used no more
  than two workers.

### 2026-10-07 — Stronger sun-dry east-clearing identity

- Retuned only the east clearing's existing portable terrain-material constants: its warm ground
  blend rises from 0.46 to 0.58, dry patches are modestly warmer/stronger, and the competing cool
  patch is restrained. The existing irregular clearing mask and already-computed broad/medium/fine
  fields remain the sole inputs, so no texture lookup, noise evaluation, uniform, geometry,
  resource, draw, collision, or world-layout cost was added. The west clearing is unchanged.
- The OpenGL 3.3 terrain object compiles with first-party warnings as errors through shared ccache
  without relinking the operator-visible game. The complete Chrome 152/SwiftShader WebGL 2 matrix
  passes 23/23; original-resolution east/west frames were reviewed together, and the east floor now
  reads as a restrained warm dry opening instead of blending into the surrounding green substrate.
- The same batched x86_64 qualification passed the strict GLES 3/audio gate at the fixed east
  clearing. PID 5305 reported `OPENGLES3`, low-latency audio, a 1280 by 720 logical backbuffer, and
  a 1521.558 ms first-draw total. Its inspected 2400 by 1080 frame retains a restrained but clearly
  warmer olive-gold floor without fatal, shader, or `GL_INVALID_OPERATION` diagnostics. This is
  translated-host functional evidence, not a physical-mobile fragment benchmark.
- The deterministic Web package remains 8,802,501 bytes uncompressed and is 2,496,567 bytes through
  gzip; WebAssembly remains 8,537,623 bytes and is 2,436,359 bytes through gzip. JavaScript remains
  262,820 bytes (59,064 gzip) and HTML remains 2,058 bytes (1,144 gzip). CNA and Sharp Runtime
  remained read-only, and no build used more than two workers. After both Android views passed, the
  emulator was stopped, the default arm64-only APK was restored, and application lint again found
  zero issues. The restored 18,744,428-byte APK contains only `arm64-v8a`, verifies with signature
  scheme v2, and contains a 15,934,144-byte stripped `libmain.so`; about 1.1 GiB of reproducible
  x86_64 CMake/object intermediates were removed.

### 2026-10-07 — Staggered asymmetric near-palm leaflets

- A new original-resolution close-palm audit found that the existing fuller nine-station fronds
  still read as mechanically opposed combs. Reused only their existing vertices to add bounded
  deterministic per-frond/station/side length, longitudinal stagger, forward-sweep, and droop
  variation. Maximum leaflet span rises from 0.74 to 0.84 local metres to match the unchanged far
  crown envelope more closely, while retaining separated tapered silhouettes.
- Near meshes remain 494--566 triangles and far meshes remain 102 triangles. Population, vertex
  and index counts, variant buffers, transforms, wind data, 125--145 m LOD, culling, collision,
  textures, uploads, and maximum eight mixed near/far submissions are unchanged. Added one exact
  diagnostic to native, browser, and next Android gates.
- The OpenGL 3.3 object compiles with first-party warnings as errors through `/rv/cnaccache`
  without relinking the operator-visible process. The complete Chrome 152/SwiftShader WebGL 2
  matrix passes 23/23; its original-resolution palm frame was inspected and retains distinct
  leaflets with a fuller, less repeated crown silhouette.
- The deterministic Web package is 8,803,353 bytes or 2,496,238 bytes through gzip; WebAssembly is
  8,538,475 bytes or 2,436,034 bytes through gzip. JavaScript remains 262,820 bytes (59,060 gzip)
  and HTML remains 2,058 bytes (1,144 gzip).
- Rebuilt/linted the default arm64-only APK through the shared cache; lint has zero issues, only
  `arm64-v8a` is packaged, and signature scheme v2 verifies. The APK is 18,745,116 bytes and its
  stripped `libmain.so` is 15,934,832 bytes. GLES 3 runtime appearance was deliberately batched
  rather than recreating about 1.1 GiB of temporary x86_64 intermediates for this vertex-only pass
  and is qualified by the later batch below.
  CNA and Sharp Runtime remained read-only; no build used more than two workers.

### 2026-10-07 — Near-palm fixed-sun facet lighting

- The post-silhouette close-palm audit found that near crowns and trunks remained visually flatter
  than surrounding grass, broadleaf vegetation, and rocks. The existing near material-detail branch
  now reconstructs two-sided face normals from world-position derivatives and applies a restrained
  0.88--1.08 fixed-sun diffuse/upward-fill multiplier before bark detail. Far palms skip the new
  derivative path.
- Added one fixed sun-direction uniform and fragment arithmetic only. Geometry, vertex format,
  textures, instance data, buffers, uploads, wind/placement updates, 125--145 m LOD, culling,
  collision, population, and draw submissions are unchanged. Native, browser, and next Android
  gates require an exact diagnostic.
- The OpenGL 3.3 object compiles with first-party warnings as errors through shared ccache without
  relinking the operator-visible process. The complete Chrome 152/SwiftShader WebGL 2 matrix passes
  23/23; the original-resolution close-palm frame retains restrained colours and gains legible
  trunk/frond planes without blown highlights.
- The deterministic Web package is 8,804,145 bytes or 2,496,534 bytes through gzip; WebAssembly is
  8,539,267 bytes or 2,436,331 bytes through gzip. JavaScript remains 262,820 bytes (59,059 gzip)
  and HTML remains 2,058 bytes (1,144 gzip).
- The default arm64-only GLES 3 APK rebuild and lint pass through `/rv/cnaccache`; lint has zero
  issues, only `arm64-v8a` is packaged, and signature scheme v2 verifies. The APK is 18,745,740
  bytes and stripped `libmain.so` is 15,935,456 bytes. Runtime appearance was batched with the
  preceding palm follow-up and is qualified by the later batch below; physical-mobile fragment
  cost remains open. CNA and Sharp Runtime
  remained read-only and builds used at most two workers.

### 2026-10-07 — Folded fallen-frond leaflets

- The latest original-resolution natural-detail audit found that the 63 curved/asymmetric fallen
  fronds retained a flat-card appearance at first-person distance. Reused every leaflet's existing
  root-to-tip diagonal as a shallow raised central fold and assigned only this class a private
  material selector for restrained 0.86--1.06 two-sided fixed-sun facet response.
- All three variants remain exactly 132 vertices and 74 triangles. Population, placement, vertex
  and index counts, variant/instance buffers, opaque rendering, 76 m culling, collision policy,
  transforms, uploads, and maximum three submissions are unchanged. Beach detail and every other
  natural-prop material remain outside the new shader branch.
- The first build correctly rejected an ambiguous mixed integer/byte `Color` constructor call under
  both GCC and Emscripten warnings-as-errors; the game-side material tag now follows the existing
  integer selector convention. No dependency defect or `BUGS.md` entry was involved.
- The corrected OpenGL 3.3 object compiles through `/rv/cnaccache` without relinking the
  operator-visible process. The complete Chrome 152/SwiftShader WebGL 2 matrix passes 23/23; its
  original-resolution frond frame was inspected and retains a subtle organic crease without a
  rigid or over-bright ridge.
- The deterministic Web package is 8,804,853 bytes or 2,496,704 bytes through gzip; WebAssembly is
  8,539,975 bytes or 2,436,494 bytes through gzip. JavaScript remains 262,820 bytes (59,066 gzip)
  and HTML remains 2,058 bytes (1,144 gzip).
- Rebuilt/linted the default arm64-only APK through the shared cache; lint has zero issues, only
  `arm64-v8a` is packaged, and signature scheme v2 verifies. The APK is 18,746,380 bytes and its
  stripped `libmain.so` is 15,936,096 bytes. GLES 3 runtime appearance was batched with the palm
  passes rather than recreating about 1.1 GiB of temporary x86_64 intermediates and is qualified
  by the later batch below. CNA and Sharp
  Runtime remained read-only; no build used more than two workers.

### 2026-10-07 — Restrained fallen-log end-grain cracks

- The latest original-resolution close-prop audit found that the irregular near-log outline and
  softened growth rings still left the exposed wood unusually pristine. Added three thin,
  slightly curved radial drying cracks in the existing cut-end fragment branch. Their strength
  rises gently from the centre toward the perimeter and remains only 8.5 percent at full mask, so
  the result preserves the rings instead of becoming a high-contrast starburst.
- This is fragment arithmetic only. Geometry, vertex/index counts, material selectors, instance
  data, buffers, uploads, textures, 72 m LOD, culling, collision proxies, population, and draw
  submissions are unchanged. Native, Chrome, Firefox, Android, and the dedicated native prop gate
  now carry the exact diagnostic.
- The OpenGL 3.3 object compiles with first-party warnings as errors through `/rv/cnaccache`
  without relinking the operator-visible process. The complete Chrome 152/SwiftShader WebGL 2
  matrix passes 23/23; the original-resolution close-log frame and a nearest-neighbour detail crop
  were inspected and show restrained readable cracks without a shader seam or artificial radial
  star. Firefox ESR 140.0 independently passes its production-shell first-frame, lifecycle,
  input/audio, jungle, and water gate while requiring the same diagnostic.
- The deterministic Web package is 8,805,713 bytes or 2,497,211 bytes through gzip; WebAssembly is
  8,540,835 bytes or 2,437,001 bytes through gzip. JavaScript remains 262,820 bytes (59,066 gzip)
  and HTML remains 2,058 bytes (1,144 gzip).
- Rebuilt/linted the default arm64-only APK through the shared cache; lint has zero issues, only
  `arm64-v8a` is packaged, and signature scheme v2 verifies. The APK is 18,747,212 bytes and its
  stripped `libmain.so` is 15,936,928 bytes. GLES 3 runtime appearance was batched with the palm
  and fallen-frond passes and is qualified by the later batch below. CNA and Sharp Runtime
  remained read-only; no build used
  more than two workers. All seven renderer-independent tests pass; project build trees remain
  bounded at 2.6 GiB native, 88 MiB Web, and 1.3 GiB Android, while the sole shared ccache remains
  3.7 GiB and 99.69 percent cacheable.

### 2026-10-07 — Sparse hanging near-canopy lianas

- A fresh original-resolution jungle audit found that the ground and crown layers were strong but
  the vertical space between them still read as a clean open grove. Near canopy variants one and
  three now each include one hanging leafy liana: a gently curved six-section tapered triangular
  strand with two small folded pointed leaves. The other two near variants remain unadorned, so the
  repeated instanced trees do not all gain the same dangling silhouette.
- Each selected shared mesh adds only 31 vertices/48 triangles. The geometry is opaque, uses the
  existing jungle vertex format/material/wind shader, and is deliberately non-solid. Population,
  placement, instance data, buffers, upload cadence, textures, culling, 105--129 m staggered LOD,
  every far mesh, collision, shader work, and the 28 fixed-jungle submissions are unchanged.
- The OpenGL 3.3 object compiles with warnings as errors through `/rv/cnaccache` without relinking
  the operator-visible game. Chrome 152/SwiftShader passes the complete 23/23 WebGL 2 matrix; the
  original-resolution jungle frame and a nearest-neighbour crop were inspected and show sparse
  readable vertical accents without obvious repetition or sky-edge artifacts. Firefox ESR 140.0
  independently passes first-frame, lifecycle, input/audio, jungle, and water gates while
  requiring the same exact diagnostic.
- The deterministic Web package is 8,812,570 bytes or 2,498,912 bytes through gzip; WebAssembly is
  8,547,692 bytes or 2,438,710 bytes through gzip. JavaScript remains 262,820 bytes (59,058 gzip)
  and HTML remains 2,058 bytes (1,144 gzip).
- The default arm64-only APK builds/lints through the shared cache, contains only `arm64-v8a`, and
  verifies with signature scheme v2. It is 18,750,124 bytes with a 15,939,840-byte stripped
  `libmain.so`. CNA and Sharp Runtime remain read-only and no build used more than two workers. All seven
  renderer-independent tests pass; the project-owned build trees remain bounded at 2.6 GiB native,
  88 MiB Web, and 1.3 GiB Android, while the sole shared ccache remains 3.8 GiB and 99.71 percent
  cacheable.

### 2026-10-07 — Batched Android palm/frond/prop/liana qualification

- Built one temporary x86_64 APK through `/rv/cnaccache` and used the API 35 `Medium_Phone` AVD
  with KVM 12 plus host AMD Radeon 780M gfxstream to close five deliberately batched visual gates:
  staggered palm leaflets, palm fixed-sun facets, folded fallen-frond leaflets, radial log-end
  cracks, and hanging leafy lianas.
- Fixed `palms`, `fronds`, `props`, and `jungle` runs used PIDs 3334, 3529, 3678, and 3826 and
  completed first draw in 1581.837, 1589.120, 1548.131, and 1571.279 ms total. Every strict gate
  required all five markers, reported `OPENGLES3`, opened a running 44.1 kHz stereo AAudio stream,
  and reported the 1280 by 720 logical backbuffer.
- Inspected every original 2400 by 1080 capture. The palms retain clean varied crowns and brown
  structural material; the frond retains its shallow fold; the log end shows subordinate radial
  cracks; and the jungle retains sparse vertical liana silhouettes. No fatal, shader, or
  `GL_INVALID_OPERATION` diagnostic occurred. This is translated-host functional/composition
  evidence, not sustained or physical-mobile performance qualification.
- Stopped the emulator cleanly, restored/linted the default 18,750,124-byte arm64-only APK,
  reconfirmed its sole ABI and v2 signature plus the 15,939,840-byte stripped `libmain.so`, and
  removed only five explicitly identified reproducible x86_64 directories totalling about 1.1 GiB.
  Compact logs and screenshots remain as evidence; Android is again approximately 1.3 GiB.

### 2026-10-07 — Current native relink and full OpenGL 3.3 requalification

- After the operator-visible older process exited normally, relinked the current Linux executable
  through the required shared `/rv/cnaccache` with two build workers. This closes the deferred
  native integration gate for the folded fallen frond, radial log-end cracks, and sparse hanging
  canopy lianas without editing CNA, Sharp Runtime, EasyGL, or MetaGL.
- The renderer-independent core test passes in 16.84 seconds. The complete private Weston/Xwayland
  OpenGL 3.3 suite then passes 41/41 in 57.15 seconds, including every deterministic scene,
  startup/package validation, both real-input scenarios, and the current visual diagnostics.
- Inspected the new original-resolution native jungle and prop captures. The jungle retains sparse
  vertical liana accents without turning every crown into a repeated curtain; the close fallen log
  keeps readable rings and an irregular outline while the restrained cracks remain subordinate.
  No shader failure, graphics-profile refusal, or test skip occurred.

### 2026-10-07 — Relocatable deterministic Linux release package

- Added a dedicated `linux-release` CMake configure/build preset using the workspace-approved
  reusable `build-consumer/` tree with the locked OpenGL 3.3
  renderer, Release optimization, project tools/tests excluded from the distributable build, the
  mandatory shared `/rv/cnaccache`, and a hard two-worker build ceiling. The clean first build
  completed all 626 steps against the unchanged read-only dependency revisions.
- Added native component-install rules plus `tools/package_linux_release.sh`. The installed game
  uses `$ORIGIN/lib`, carries the exact stripped `libcna.so`, project attribution/dependency pins,
  CNA's Ms-PL/notice, and the Sharp Runtime/EasyGL/MetaGL MIT licences. SDL 3, SDL_mixer 3, glibc,
  libstdc++, and the OpenGL driver remain honest host requirements rather than copied system files.
- After adding the frame-budget instrumentation below, the fixed-file schema-1 manifest records
  11,535,534 installed bytes and each file's size/SHA-256. Two unchanged packaging runs produced
  byte-identical 4,570,422-byte archives with SHA-256
  `a729334d189905caaa779fd27cabf7dbae4304fcf0288bf4fd65206c5388a8c4`.
- Extracted the archive beneath `/tmp`, launched it from that unrelated working directory on the
  private Weston/Xwayland display, selected `OPENGL33`, compiled the complete current shader set,
  rendered four overview frames, and exited cleanly. After moving the reusable release tree from
  the non-approved `build-release/` name to `build-consumer/`, the final archive repeated that gate
  at 497.137 ms total and the obsolete tree was removed. A deliberate attempt to package the Debug
  tree failed before creating any output with the expected actionable diagnostic.

### 2026-10-07 — Release frame-pacing baseline and budget-miss instrumentation

- Ran three isolated six-view suites from the stripped relocatable Release package on the AMD
  Radeon 780M/Mesa 25.0.7 OpenGL path. Across 1,800 samples per view, fixed-scene means remained
  1.5310--2.6794 ms, p95 remained 2.2126--4.9502 ms, and p99 remained 5.4131--13.4599 ms. The
  675.9635 m traversal remained 2.2811--2.5136 ms mean, 4.6850--5.7565 ms p95, and
  9.9770--12.8821 ms p99.
- Extended renderer-independent `FrameTimeSummary` and benchmark schema 3 with exact sample counts
  above 8.333, 16.667, and 33.333 ms. Unit tests lock the strict-greater-than thresholds. In the
  instrumented 600-sample suite, the six views had 6/20/17/8/19/22 intervals above the 120 Hz
  budget, only 1/5/0/0/1/3 above the 60 Hz budget, and zero above the 30 Hz budget.
- This establishes a current-machine Linux release gate of p99 below the 60 Hz interval and no
  repeated 30 Hz miss across the representative matrix. It is not a minimum hardware claim;
  hardware-browser and physical-Android budgets remain open. The isolated compositor intentionally
  lacked audio, so these timings exclude active mixer-device cost.
- The current source passes the full native 41/41 private-display matrix in 77.00 seconds, the
  complete Chrome
  WebGL 2 interaction/render suite, and the independent Firefox ESR production-shell lifecycle,
  pointer-lock, input/audio, jungle, and water gate. The later 42-blade Web release is 8,814,075
  bytes or 2,499,452 bytes through deterministic gzip; WebAssembly is 8,549,197 bytes or 2,439,244
  through gzip, JavaScript is 262,820 bytes (59,064 gzip), and HTML is 2,058 bytes (1,144 gzip).
- The default arm64-v8a GLES 3 APK builds through the shared cache with two workers, passes
  application lint with no errors or warnings, contains only the 15,941,512-byte `libmain.so`, and
  verifies with APK signature scheme v2. The APK is 18,751,796 bytes. Because benchmark mode is
  host QA and normal presentation is unchanged, the prior current GLES 3 runtime matrix remains
  the relevant device-side functional evidence.

### 2026-10-07 — Denser first-person near-grass tier

- Original-resolution native beach/jungle/grass/water review found that the curved near blades
  still exposed too much bare floor at first-person height. Increased only the shared near cluster
  from 34 to 42 blades. It now contains seven shorter juvenile blades, six sparse seed-bearing
  stems, and 14 selected V-fold ridges in 622 vertices/496 triangles. The 10/5-blade middle/far
  meshes, 18,556 placements, instance buffers, culling, transition distances, opacity, collision,
  textures, and draw submissions are unchanged.
- Side-by-side native before/after review shows visibly more continuous near cover while retaining
  individually separated silhouettes. The native detail capture passes. A stripped Release run of
  the deliberately heavy 2,827-cluster/99-submission grass view measured 3.1653 ms mean, 6.3239 ms
  p95, and 13.9728 ms p99 across 600 intervals; only four exceeded 16.667 ms and none exceeded
  33.333 ms.
- Added an exact 42-blade runtime marker to the Chrome and Android gates. The complete Chrome
  WebGL 2 matrix passes and its original-resolution frame is clean. Firefox ESR 140 independently
  passes production first frame, WebGL 2, pointer lock/relock, resize, tab lifecycle, resumed
  animation/input, audio, run toggling, footsteps, and dedicated jungle/water scenes. The Web package
  is 8,814,075 bytes or 2,499,452 bytes through gzip; WebAssembly is 8,549,197 bytes (2,439,244
  gzip), JavaScript is 262,820 bytes (59,064 gzip), and HTML is 2,058 bytes (1,144 gzip).
- Built a temporary x86_64 APK and booted the API 35 `Medium_Phone` AVD with KVM 12 and host AMD
  Radeon 780M gfxstream. Two fixed close-grass GLES 3 gates passed; the overlay-free 2400 by 1080
  frame visibly retains clean denser blades and translucent touch controls. The second run reported
  `OPENGLES3`, 1280 by 720 logical rendering, and 2937.204 ms total to first draw, with no fatal,
  shader, or `GL_INVALID_OPERATION` diagnostic. This is functional translated-GPU evidence, not a
  physical-device performance claim.
- Stopped the emulator cleanly and restored the default arm64-only APK. It is 18,751,796 bytes,
  contains a 15,941,512-byte `libmain.so`, passes app lint with no issues, and verifies with APK
  signature scheme v2. The optimized relocatable Linux payload is 11,535,670 bytes; two unchanged
  packaging runs produced byte-identical 4,570,530-byte archives with SHA-256
  `01376a7326c56418009e2890ad21fa49d78f06e5cda790984a04361901c98443`.
- The complete current native private-display suite passes 41/41 in 77.00 seconds with no graphics-
  profile refusal. Removed the five identified reproducible x86_64 Android intermediate trees,
  returning Android from 2.3 GiB to 1.3 GiB, and removed the documented temporary comparison/
  benchmark copies and superseded archives. The compact GLES logs/screenshots remain as evidence.

### 2026-10-07 — Centralized Android mobile quality default

- Added renderer-independent `RuntimeQualityProfile` policy with validated full and mobile values.
  Linux/Web keep 135 m grass, full contact-shadow/wildlife distances, and the 160 by 160 ocean
  grid. Android retains every population, simulation, shader, 42-blade near-grass mesh, the full
  143,624-byte generated texture inventory, and six-source soundscape while selecting 112 m grass,
  72% shadow range, 82% wildlife draw range, and a 128 by 128 ocean grid.
- Passed values explicitly into the grass, contact-shadow, bird, fish, shark, monkey, and ocean
  renderers. Runtime guards reject invalid distances/grids and texture/audio inventory overruns.
  Extended the ocean layout with tested dynamic counts and exact unsigned 16-bit capacity limits;
  Android ocean geometry is 16,641 vertices/32,768 triangles instead of 25,921/51,200.
- Core tests pass and the native `IslandGameSmoke` plus fixed grass capture pass on the private
  OpenGL display. Their exact full-profile diagnostic and unchanged 160-grid counts confirm that
  desktop quality did not regress. The complete 23-stage Chrome/WebGL 2 gate passes and now
  requires the same full-profile marker.
- Built the temporary x86_64 APK through the shared ccache with two Gradle/native workers. KVM 12
  and host AMD Radeon 780M gfxstream booted the API 35 `Medium_Phone` AVD in 19.235 seconds. Two
  fixed-grass GLES 3 gates required the mobile marker and passed without fatal, shader, or
  `GL_INVALID_OPERATION` diagnostics. The clean 2400 by 1080 frame retains dense close grass and
  translucent controls; first draw completed in 2703.072 ms at the 1280 by 720 logical backbuffer.
- The mobile debug view reported 2,222 visible grass clusters and 16 shadows versus 2,827 and 27 in
  the full fixed view, while preserving all 957 near clusters. This is bounded workload evidence,
  not a physical-device performance claim. Stopped the emulator cleanly, restored the arm64-only
  APK, passed lint with no issues, and verified its v2 signature. It is 18,757,052 bytes with a
  15,946,768-byte stripped `libmain.so`. Removed the verified x86_64 intermediates, reducing the
  Android tree from 2.4 GiB to 1.3 GiB. No dependency bug was found or added to `BUGS.md`.
- The final Web relink/package validation passes. HTML is 2,058 bytes (1,144 gzip), JavaScript is
  263,453 bytes (59,114 gzip), and WebAssembly is 8,556,884 bytes (2,442,705 gzip), for a total
  8,822,395-byte payload or 2,502,963 bytes through deterministic gzip (71.6% reduction).
- The stripped Linux Release package contains 11,541,525 installed bytes. Two unchanged packaging
  runs produced identical 4,573,055-byte archives with SHA-256
  `952dbe5aab1409ff98638e62d4a509e2391c28a79f1255a10823d73ce5527ee4`. An extraction beneath
  `/tmp` launched from that unrelated working directory, selected `OPENGL33`, required the full
  quality profile, rendered four overview frames, and exited cleanly in 623.924 ms total.
- The exact final Web artifacts pass the complete Chrome matrix. A repeated Firefox run exposed a
  harness race after tab foregrounding: the next synthetic Shift press could arrive before a
  resumed game frame observed the previous key release, so a valid running state never received a
  second edge. Added a 250 ms WebDriver action pause after each resumed Shift release, matching the
  already stable pre-lifecycle input sequence; this changes only QA automation, not game input.
  The exact final Firefox artifact then passed first frame, pointer lock/release/relock, responsive
  sizing, audio, both pre- and post-lifecycle Shift edges, footstep, tab lifecycle, resumed game
  input, and dedicated jungle/water scenes. The final native private-display matrix passes 41/41
  in 80.97 seconds without a graphics-profile refusal.

### 2026-10-07 — Portable persistent player preferences

- Added schema-1 `UserSettings` parsing/serialization and a CNA `StorageDevice`-backed
  `UserSettingsStore`. Ordinary launches now restore the selected Shift walk/run pace, B camera-
  motion state, M mute state, and Android low/standard/high touch-look response. The parser is
  deliberately bounded and rejects oversize or NUL-containing input, duplicate or missing current
  keys, unsupported schemas, and invalid values while ignoring documented future keys.
- Settings use the platform-native CNA container (`settings.ini`, a last-valid backup, and a
  temporary replacement): XDG user storage on Linux, IDBFS with automatic persistence on Web, and
  app-private storage on Android. Smoke, capture, benchmark, and fixed-view QA launches are
  isolated automatically; `--no-user-settings` provides explicit isolation. Load/save diagnostics
  are mirrored to Android logcat. The implementation is entirely game-owned and no dependency
  source was changed.
- Added core round-trip/malformed-input tests plus `IslandGameSettingsPersistence`, which launches
  three real native sessions under an isolated XDG root: the first writes exact toggles, the second
  restores them, and the third corrupts the primary file and recovers the valid backup. Increased
  synthetic key dwell times for slow frames and taught native input tests to reuse CNA's private
  rootful Xwayland when available instead of nesting a software Xvfb server on a Wayland host.
- An initial parallel full-suite run exposed only shared-display starvation: four frame-driven
  tests timed out but passed independently. The final serial private-display OpenGL 3.3 suite
  passes 42/42 in 153.44 seconds, including real simultaneous movement/look and the three-process
  persistence scenario. The Linux QA guide now makes serial aggregate GPU qualification explicit.
  All seven first-party AddressSanitizer/UndefinedBehaviorSanitizer tests pass in 78.99 seconds
  with no finding; the reproducible 47 MiB sanitizer tree was removed afterward.
- The exact Web release passes the complete Chrome 23-stage WebGL 2 gate, including an IDBFS-backed
  close/relaunch persistence check, and independently passes the Firefox ESR production-shell
  lifecycle, pointer-lock, input/audio, jungle, and water gate. HTML is 2,058 bytes (1,144 gzip),
  JavaScript is 264,001 bytes (59,212 gzip), and WebAssembly is 8,621,912 bytes (2,464,390 gzip),
  for 8,887,971 bytes uncompressed or 2,524,746 bytes through deterministic gzip.
- A temporary x86_64 APK passed the API 35 KVM 12/host-gfxstream GLES 3 settings gate across hot
  resume and a force-stop/new-process launch; exact logcat evidence shows high touch response
  restored in the new process. Both clean 2400 by 1080 frames were inspected. The emulator was
  stopped and the default arm64-only APK restored: it passes lint, verifies with APK signature
  scheme v2, is 18,861,028 bytes, and contains a 16,050,744-byte stripped `libmain.so`.
- Rebuilt the stripped Linux Release target through the mandatory shared `/rv/cnaccache`. The
  final package contains 11,585,955 installed bytes; two unchanged packaging runs produced
  byte-identical 4,594,448-byte archives with SHA-256
  `6b8ad11a9ead9d9414c472255aae7d6e91a7478605f951b1dfc51eb1fdcc0b07`. The unchanged game binary
  had already been extracted beneath `/tmp`, launched from an unrelated working directory,
  selected `OPENGL33`, rendered four frames, and exited cleanly with settings enabled.
- Removed verified temporary x86_64 Android intermediates and the sanitizer tree. The retained
  reproducible trees are approximately 2.5 GiB (`build`), 88 MiB (`build-probe`), 107 MiB
  (`build-consumer`), and 1.3 GiB (`android`). CNA `next` remains at `97783d1e090c`, Sharp Runtime
  `next` at `21c06910f7cf`, and their worktrees remain read-only and clean. No dependency defect was
  found, so `BUGS.md` requires no new entry.

### 2026-10-07 — Release presentation and isolated startup baseline

- Added one unmodified, losslessly converted 1280 by 720 native OpenGL 3.3 overview to the top of
  the README. The 399,673-byte original project image is tracked at
  `assets/presentation/island-overview.png`, hash-gated in manifest schema 2, attributed in
  `ASSET_LICENSES.md`, installed beside the packaged README, and visually inspected at original
  resolution. Asset validation now passes 17 procedural records, 32 generator sources, two
  project files, and zero external files.
- Extended the fixed-file Linux packager to require the presentation image, preventing a broken
  relative README link. Two unchanged packaging runs produced identical 11,986,092-byte installed
  payloads and 4,995,503-byte archives with SHA-256
  `f6009f1c9c0c5b0f7734c0a3430001a89eb3de55c482adb7f27ac63ee6752262`.
- Recorded two isolated Debug and two isolated stripped-Release startup reports on the private
  OpenGL 3.3 Radeon/Mesa display after closing other GPU clients. Release completed its first
  game-owned draw in 426.087--506.104 ms: world construction was 161.497--202.003 ms and GPU/
  procedural content was 202.621--233.506 ms. Debug completed in 1762.530--1981.890 ms. The
  current Release split shows no demonstrated loading bottleneck worth a speculative rewrite;
  hardware-browser cold load and physical-Android launch budgets remain open.
- Expanded this living plan at the user's direction with an approximately 20 m broken wooden ship
  and debris landmark, beach crabs, clearing butterflies, near-shore sea turtles, and an additional
  evidence-driven visual-realism campaign of up to roughly 100 productive hours. No implementation
  or platform claim is recorded for those new items yet.

### 2026-10-07 — Large shallow-water shipwreck landmark

- Added a deterministic original wooden sailing-ship wreck at an authored shoreline-relative
  location. It is exactly 20.0 m long, 5.2 m wide, and 4.4 m high: a grounded, heeled hull with a
  central break, exposed ribs, partial deck, cabin, snapped mast, bowsprit, rail remnants, rigging,
  and twelve planks/substantial fragments trailing toward the beach. It is deliberately a large
  ship, not a small boat, and remains environmental scenery rather than an objective.
- Added a renderer-independent layout and tested authoritative shore/seabed support, deterministic
  reconstruction, debris type/bounds, and six conservative major-hull collision proxies. Those
  proxies join the shared 16 m broad phase; the world report now validates 833 obstacles in 485
  occupied cells and produces fingerprint `9b0c6d1b3b0f806a`. Smaller debris stays non-solid to
  avoid snagging the player.
- Added one portable static `VertexPositionColor`/`BasicEffect` batch with 1,924 triangles,
  fixed-sun facet colour, wet/submerged darkening, fog, frustum rejection, and a 310 m distance
  bound. The wreck renders before the transparent ocean so the same water colours its submerged
  structure. A dedicated `--viewpoint shipwreck` camera and exact one-draw/12-debris diagnostics
  make the landmark independently reviewable.
- The native private-display OpenGL 3.3 matrix passes 43/43. The 24-stage Chrome/WebGL 2 matrix and
  independent Firefox ESR gate both pass with dedicated original-resolution shipwreck captures.
  Two accelerated API 35 x86_64 GLES 3 runs passed the strict shipwreck marker gate; the clean
  2400 by 1080 frame was inspected and first draw completed in 6204.880 ms on the KVM/gfxstream
  functional environment. This is not physical-mobile performance qualification.
- Two isolated native Debug shipwreck benchmarks reported 2.3154--2.3745 ms mean,
  1.9153--1.9271 ms median, 4.5776--4.6285 ms p95, and 9.4118--10.4471 ms p99 across the recorded
  runs, with zero intervals above 16.667 ms and 46 tracked submissions. The new ship itself remains
  one visible draw in the fixed view.
- The current Web package is 8,941,239 bytes uncompressed or 2,537,582 bytes through deterministic
  gzip. The default arm64 APK was restored, passes lint, contains a 16,086,656-byte stripped
  `libmain.so`, is 18,896,940 bytes, and verifies with APK signature scheme v2. Five verified
  reproducible x86_64 intermediate directories (about 1.1 GiB) were removed after the emulator
  stopped.
- Two unchanged Linux Release packaging runs produced byte-identical 5,024,595-byte archives with
  SHA-256 `0ad6f6d50bcaacf147f7ea25c305839dca6ca2386545d707646adbb36bdd918d` and
  12,071,978 installed bytes. An extracted package launched from an unrelated directory, selected
  `OPENGL33`, rendered the shipwreck view, and exited cleanly. Asset validation passes 18
  procedural records, 34 generator sources, two project files, and zero external files. CNA and
  Sharp Runtime remained read-only and clean; no qualifying dependency defect was found.

### 2026-10-07 — Deterministic beach-crab colonies

- Added thirty original procedural beach crabs in six authored-procedural colonies. Their compact
  renderer-independent state machine alternates asynchronous pause, forage, and lateral scuttle
  behaviour, responds non-hostilely to a player within 5.2 m, and shares the established
  full/reduced/dormant wildlife activation policy.
- Crab habitat is constrained by the authoritative terrain to a 1.4--17.0 m signed shore band,
  walkable sand, and a bounded individual home range. Core coverage proves deterministic
  construction, initial validity, non-finite-input safety, behaviour reachability, observer-tier
  transitions, player response, and 12,000 update frames without leaving valid habitat.
- Added a single culled dynamic `BasicEffect` batch with near/far models, four natural palettes,
  faceted shells, articulated bent legs, small claws, eye stalks, and asynchronous gait/claw pose.
  The dedicated `--viewpoint crabs` composition shows five animals at first-person beach scale;
  F1 and schema-3 benchmark output expose visible/near/far, simulation-tier, draw, and range counts.
- The native OpenGL 3.3 crab capture, 25-stage Chrome/WebGL 2 matrix, independent Firefox ESR crab
  scene, and strict API 35 KVM/gfxstream GLES 3 crab run all pass. The inspected 1280 by 720 native
  and browser frames and centred 2400 by 1080 Android frame agree on the five-crab composition and
  retain one wildlife draw. Android first draw completed in 2336.217 ms in that emulator run;
  physical-device performance and ergonomics remain unqualified.
- An isolated native Debug crab-view benchmark recorded 2.4998 ms mean, 4.8406 ms p95,
  11.0887 ms p99, zero 60 Hz misses, 26 tracked submissions, and five visible crabs in one draw.
  Asset validation now covers 19 procedural records, 36 generator sources, two project files, and
  zero external files; the deterministic world fingerprint is `9b80b94634506ebb`. The final Web
  package is 8,972,829 bytes uncompressed or 2,549,100 bytes through deterministic gzip.
- The temporary x86_64 Android build products were removed after the emulator stopped and the
  default arm64-only package was restored, rebuilt, linted, and signature-verified. It is
  18,947,852 bytes and contains a 16,137,568-byte stripped `libmain.so`. CNA and Sharp Runtime
  remained read-only and clean; no qualifying dependency defect was found.
- The stripped Linux Release package was rebuilt through shared ccache. Two packaging runs produced
  the same 5,035,707-byte archive with SHA-256
  `b5a17bd4e84f481597ee5db62c8aed66ed7aa6c6f6f7bab94e87086f30f918e2` and 12,105,759
  installed bytes. The installed executable launched from `/tmp` on the private OpenGL 3.3
  display, rendered the five-crab view in one draw, and exited cleanly; the frame was inspected.

### 2026-10-07 — Clearing-edge butterflies

- Added eighteen original procedural butterflies across three sunny-clearing and jungle-edge
  habitats. Their renderer-independent state machine alternates asynchronous rest, flutter, and
  nectar-hover behaviour, varies height and turning without reusing bird flight, and applies a
  mild non-hostile lift/retreat response to a nearby player.
- Every proposed pose is constrained to authoritative coastal/interior ground, at least 28 m
  inland, below the configured slope limit, within its individual home range, and inside a narrow
  terrain-relative altitude envelope. Tests cover deterministic reconstruction, invalid inputs,
  initial habitat validity, all three states, observer-distance tiers, player response, and 12,000
  update frames without non-finite or out-of-habitat motion.
- Added one culled dynamic `BasicEffect` batch with five natural palettes, hinged patterned wings,
  small bodies, double-sided opaque near geometry, and a reduced far silhouette. The dedicated
  `--viewpoint butterflies` composition shows 10--11 of 18 agents, normally seven near and four
  far, in one draw. F1 and schema-3 benchmark output expose render LOD, simulation tiers, draw
  count, and observed population ranges.
- The complete private-display OpenGL 3.3 suite passes 45/45. The final inspected native 1280 by
  720 capture retains readable clearing motion after reducing maximum wingspan to approximately
  0.27 m. The 26-stage Chrome/SwiftShader WebGL 2 matrix passes, and Firefox ESR independently
  reports 11 visible butterflies (seven near, four far) in one draw. One earlier Chrome attempt
  encountered a transient page-navigation readiness miss at the unrelated fish stage; the full
  unchanged rerun passed.
- An isolated native Debug butterfly-view benchmark recorded 3.5798 ms mean, 2.8551 ms median,
  7.6164 ms p95, 10.5271 ms p99, 13.1143 ms maximum, 29 intervals above 8.333 ms, and zero above
  16.667 ms. It reported 85 tracked submissions and a 10--11 visible-butterfly range. This is an
  integrated Debug baseline rather than isolated species cost attribution.
- Asset validation now covers 20 procedural records, 38 generator sources, two project files, and
  zero external files; the deterministic world fingerprint is `db257bb9333e17ad`. The final Web
  package is 9,000,114 bytes uncompressed or 2,556,727 bytes through deterministic gzip: HTML is
  2,058/1,144 bytes, JavaScript 264,849/59,282 bytes, and WebAssembly 8,733,207/2,496,301 bytes.
- The arm64-only Android package rebuild and lint pass. The APK is 18,999,812 bytes, contains a
  16,189,528-byte stripped `libmain.so`, and verifies with APK signature scheme v2. Butterfly
  runtime appearance on GLES 3 remains explicitly unqualified; its accelerated x86_64 runtime pass
  is intentionally combined with the next sea-turtle milestone to avoid regenerating roughly
  1.1 GiB of temporary native intermediates twice. CNA and Sharp Runtime remained read-only.
- The stripped Linux Release package was rebuilt through shared ccache. Two packaging runs produced
  the same 5,046,860-byte archive with SHA-256
  `815a1fe1b62b42ec8ab2731ba05a220bbba0589523be4a93f1eec2bea7a6cb67` and 12,131,223
  installed bytes. From `/tmp`, the packaged executable selected `OPENGL33`, rendered eleven
  butterflies (seven near, four far) in one draw, and reached its first completed game draw in
  823.393 ms; the Release frame was inspected.

### 2026-10-07 — Bounded near-shore sea turtles

- Added five original procedural sea turtles in separate deterministic near-shore route regions.
  Their renderer-independent state machine alternates asynchronous cruise, seabed-forage, and
  occasional surface behaviour, mildly separates from a nearby player, and shares the established
  full/reduced/dormant activation policy.
- Every candidate pose is constrained by authoritative shore and seabed queries to 7--38 m
  offshore, at least 0.72 m water depth, the animal's individual home range, seabed clearance, and
  the water surface. This keeps the complete population out of land and inside the player's 42 m
  offshore-current onset. Core tests cover deterministic reconstruction, invalid input, initial
  validity, all three states including surfacing, player response, activation tiers, and 12,000
  update frames without non-finite or out-of-habitat motion.
- Added one culled dynamic `BasicEffect` batch with three palettes and shared aquatic depth/sun-band
  colour response. The 204-vertex near model has a faceted scute-patterned shell, head, eyes, tail,
  and four articulated flippers; a 24-vertex far model keeps a complete silhouette. The dedicated
  `--viewpoint turtles` camera exposes one near animal in one draw, while F1 and schema-3 benchmark
  output expose render LOD, simulation tiers, behaviour states, draws, and observed ranges.
- The complete private-display OpenGL 3.3 suite passes 46/46. The 27-stage Chrome/SwiftShader
  WebGL 2 matrix and independent Firefox ESR gate both pass with dedicated original-resolution
  turtle captures. The inspected frames retain a clear shell/flipper silhouette beneath the
  transparent ocean. One initial unchanged Chrome run received an external termination during the
  long matrix; the complete immediate rerun passed without a game or JavaScript error.
- An isolated stripped-Release turtle benchmark recorded 2.1252 ms mean, 1.7856 ms median,
  3.2192 ms p95, 9.5893 ms p99, and 19.3897 ms maximum across 600 intervals. Eleven exceeded
  8.333 ms, two exceeded 16.667 ms, and none exceeded 33.333 ms. The fixed view held 75 tracked
  submissions and one turtle draw, so this is an integrated scene baseline rather than isolated
  species cost.
- Built one temporary x86_64 APK through `/rv/cnaccache` with two workers. The API 35
  `Medium_Phone` AVD booted in 21.494 seconds with KVM 12 and host AMD Radeon 780M gfxstream. Strict
  turtle and butterfly gates passed; the clean 2400 by 1080 turtle frame shows one near animal in
  one draw, and the butterfly overlay reports 11/18 visible (seven near, four far) in one draw.
  The repeated turtle run completed first draw in 2527.896 ms with no fatal, shader, or
  `GL_INVALID_OPERATION` diagnostic. This is translated-host functional evidence, not a
  physical-device performance claim.
- Stopped the emulator cleanly and restored/linted the default arm64-only APK through the shared
  cache. It is 19,055,612 bytes, contains a 16,245,328-byte stripped `libmain.so`, packages only
  `arm64-v8a`, and verifies with signature scheme v2. Removed only the five identified reproducible
  x86_64 intermediate directories (about 1.1 GiB), returning the Android tree to approximately
  1.3 GiB.
- Asset validation now covers 21 procedural records, 40 generator sources, two project files, and
  zero external files. The deterministic world fingerprint is `cff48b815e501c4f`. The final Web
  payload is 9,038,860 bytes uncompressed or 2,570,077 bytes through deterministic gzip; its
  WebAssembly is 8,771,952 bytes or 2,509,652 bytes through gzip.
- Two unchanged Linux Release packaging runs produced the same 5,061,333-byte archive with SHA-256
  `aa3463ba268e22effa44e936620f1dd582dcee91e87131aec6e4b3bea97f61ae` and 12,175,075 installed
  bytes. An extraction beneath `/tmp` launched from that unrelated working directory, selected
  `OPENGL33`, rendered one near turtle in one draw, passed capture validation, and reached first
  draw in 418.872 ms; the Release frame was inspected. CNA and Sharp Runtime remained read-only
  and clean, and no qualifying dependency defect was found.

### 2026-10-07 — Refined shipwreck geometry and aged-waterline material

- Replaced the wreck's broad five-point cross-section with eleven points and more longitudinal
  sections, giving the lower hull a curved keel-to-gunwale transition without changing its exact
  20.0 m by 5.2 m authored footprint. Two localized missing-board openings reinforce the central
  break. All plank and major-fragment debris now uses deterministic five-section splintered prisms
  with tapered, offset ends instead of regular boxes.
- Added one texture-free portable GLSL 330 / GLSL ES 300 material. It projects distance-faded grain
  along the authored ship axis, layers broad weathering and restrained cracks, distinguishes fresh
  breaks, darkens/cools wet and submerged wood, adds a narrow salt-wear line, and limits waterline
  sheen to a small sun/view response. Prelit vertex colour remains an actionable fallback. The
  complete refined landmark grows from 1,924 to 3,392 triangles but remains one static draw.
- Strengthened the native, Chrome, Firefox, and Android gates with the exact aged-wood/waterline
  diagnostic. The OpenGL 3.3 capture passes at 1280 by 720. Both complete 27-stage Chrome/WebGL 2
  and independent Firefox ESR/WebGL 2 runs pass, compile the GLSL ES path, retain one wreck draw,
  and produced inspected matching shipwreck frames. The complete private-display native suite
  passed 46/46; after the final wet-material selector correction, the targeted native capture and
  both complete browser gates passed again.
- Built a temporary x86_64 APK through shared `/rv/cnaccache` with two workers. The API 35
  `Medium_Phone` AVD booted with KVM 12 and host AMD Radeon 780M gfxstream in 18.606 seconds. Its
  strict clean `shipwreck` run passed the shader/construction markers, retained one draw, reached
  first draw in 3592.612 ms, and produced an inspected 2400 by 1080 GLES 3 frame without fatal,
  shader, or `GL_INVALID_OPERATION` diagnostics. This is translated-host functional evidence, not
  a physical-device performance or colour claim.
- Restored and linted the arm64-only package. The APK is 19,070,196 bytes, contains a 16,259,912-
  byte stripped `libmain.so`, packages only `arm64-v8a`, and verifies with APK signature scheme v2.
  Removed exactly the five reproducible x86_64 intermediate directories after stopping the
  emulator, returning the Android tree from 2.4 GiB to 1.3 GiB.
- One isolated Linux Release benchmark recorded 1.9559 ms mean, 1.3566 ms median, 5.5216 ms p95,
  11.6363 ms p99, and 17.4985 ms maximum across 600 intervals. Twenty exceeded 8.333 ms, one
  exceeded 16.667 ms, and none exceeded 33.333 ms; the scene retained 46 tracked submissions and
  one wreck draw.
- The current Web package is 9,057,532 bytes uncompressed or 2,578,613 bytes through deterministic
  gzip. Two Linux packaging runs produced the same 5,068,477-byte archive with SHA-256
  `c836cce25cc93ec7af0f82eec14447420305a1f55cc2427aff5db8b039094fd1` and 12,191,011 installed
  bytes. The exact archive launched from an unrelated `/tmp` directory, selected `OPENGL33`,
  compiled the new shader, rendered the refined one-draw wreck, passed capture validation, and
  completed first draw in 629.566 ms after the final wet-material selector correction. CNA and Sharp
  Runtime remained read-only and clean; no
  qualifying dependency defect was found.

### 2026-10-08 — Portable inland humus and moss material breakup

- Reviewed twelve current fixed native views together at original resolution. The most frequent
  high-impact defect was the broad, uniformly saturated green substrate beneath otherwise layered
  interior vegetation. Terrain/material work ranked above adding another feature or increasing
  geometry blindly; the bright permanent-day art direction remains unchanged.
- Extended the existing portable terrain shader with a warm-desaturated inland base plus separate
  leaf-humus and cool-moss classifications. The pass reuses the already-evaluated broad, medium,
  and fine fields and is bounded by signed shore distance, slope, clearing suppression, and the
  existing camera-detail fade. It adds no noise lookup, texture, varying, geometry, allocation,
  collision rule, or draw submission. Two strength passes were inspected; the weaker first pass
  was rejected because it was effectively invisible at first-person height.
- Added one exact runtime diagnostic and required it in the complete Chrome, independent Firefox,
  and strict Android gates. Targeted OpenGL 3.3 jungle/east/west/ground captures pass and were
  inspected. The complete 27-stage Chrome/WebGL 2 matrix and Firefox ESR WebGL 2 gate both pass;
  their current jungle frames retain the restrained breakup without seams or shader fallback.
  The complete native suite then passed as seven non-display plus 39 private-display tests,
  covering all 46 current tests without a failure.
- One isolated Release OpenGL 3.3 jungle run recorded 2.8044 ms mean, 2.0553 ms median, 7.8039 ms
  p95, 12.4736 ms p99, and 24.0792 ms maximum over 600 intervals. Twenty-five exceeded 8.333 ms,
  one exceeded 16.667 ms, and none exceeded 33.333 ms. This is an integrated 89-submission scene
  baseline rather than isolated fragment-cost attribution.
- Built a temporary x86_64 APK through shared `/rv/cnaccache` with two workers. KVM 12 and host AMD
  Radeon 780M gfxstream were active. The strict fixed-jungle GLES 3 run passed, reached first draw
  in 2755.377 ms, and retained an inspected clean 2400 by 1080 frame. An unrelated Pixel Launcher
  ANR dialog covered the helper's first screenshot after game qualification; selecting `Wait` and
  recapturing the still-focused game produced the retained unobstructed evidence. No game fatal,
  shader, or `GL_INVALID_OPERATION` diagnostic appeared.
- Stopped the emulator cleanly, restored and linted the arm64-only package, and verified APK
  signature scheme v2. The APK is 19,071,332 bytes with a 16,261,048-byte stripped `libmain.so`.
  Removed exactly five inactive reproducible x86_64 intermediate directories, returning the
  Android tree to approximately 1.3 GiB while retaining the log and screenshot evidence.
- The current deterministic Web package is 9,058,713 bytes uncompressed or 2,578,728 bytes through
  gzip: HTML is 2,058/1,144 bytes, JavaScript 264,591/59,251 bytes, and WebAssembly
  8,792,064/2,518,333 bytes. CNA and Sharp Runtime remain on their pinned `next` revisions,
  read-only and clean; no qualifying dependency defect was found.
- Two unchanged Linux Release packaging runs produced the same 5,069,942-byte archive with
  SHA-256 `363f2df11859c51c485f76694546cb255b38f82421887f1d6a03eeb4f4451f68` and
  12,192,120 installed bytes. The exact archive launched from an unrelated `/tmp` directory,
  selected `OPENGL33`, required the new material marker, captured the jungle view, and completed
  first draw in 372.093 ms. Its inspected 1280 by 720 frame passed capture validation.

### 2026-10-08 — Natural near-grass blade silhouettes

- Reviewed the deterministic close-grass view before and after the change. The former mature
  blades were broad, highly symmetric ribbons at first-person range. Narrowed only the shared near
  profile, derived bounded lateral bow and unequal left/right edges from the stable blade index,
  and gave a minority of tips a small sideways bend plus drop. The shared mesh remains 42 blades,
  622 vertices, and 496 triangles; middle/far geometry, instance records, shader arithmetic,
  buffers, population, culling, LOD transitions, and submissions remain unchanged.
- Added one exact diagnostic and required it in the Chrome, Firefox, and Android runtime gates.
  The complete 27-stage Chrome/WebGL 2 matrix and independent Firefox ESR WebGL 2 gate pass. Their
  current close-grass frame and the native OpenGL 3.3 before/after pair were inspected; the result
  retains cover while replacing the most conspicuous repeated ribbon silhouette.
- KVM 12 and the host AMD Radeon 780M gfxstream path were active for a strict API 35 x86_64 fixed-
  grass run. It selected `OPENGLES3`, required the new marker, reached first draw in 2943.958 ms,
  and produced an inspected 2400 by 1080 frame without fatal, shader, or
  `GL_INVALID_OPERATION` diagnostics. This is translated-host functional evidence, not physical-
  device performance qualification.
- Stopped the emulator, restored/linted the default arm64-only APK through shared `/rv/cnaccache`,
  verified its v2 signature and sole `arm64-v8a` ABI, and removed exactly five reproducible x86_64
  intermediate directories (about 1.1 GiB). The APK is 19,072,372 bytes and its stripped
  `libmain.so` is 16,262,088 bytes; the Android tree returned to approximately 1.3 GiB.
- The complete current native private-display suite passes 46/46. One isolated Linux Release
  close-grass benchmark retained 2,827 clusters, 957 near instances, 21 grass submissions, and 99
  tracked submissions. Its 600 intervals measured 2.6461 ms mean, 2.1421 ms median, 6.3492 ms p95,
  11.7514 ms p99, and 16.8790 ms maximum; twelve exceeded 8.333 ms, one exceeded 16.667 ms, and
  none exceeded 33.333 ms.
- The deterministic Web package is 9,059,477 bytes uncompressed or 2,579,291 bytes through gzip:
  HTML is 2,058/1,144 bytes, JavaScript 264,591/59,253 bytes, and WebAssembly
  8,792,828/2,518,894 bytes. CNA and Sharp Runtime remain on their pinned `next` revisions,
  read-only and clean; no qualifying dependency defect was found.
- Two unchanged Linux Release packaging runs produced the same 5,069,785-byte archive with
  SHA-256 `3cb1d12a739f3d2c612a4eba0ca1d7e029427e706a9b8ebd25cc0776a5756279` and
  12,192,773 installed bytes. The exact archive launched from an unrelated `/tmp` directory,
  selected `OPENGL33`, required the new marker, retained 2,827 visible grass clusters in 21 grass
  submissions, and completed first draw in 446.534 ms. Its inspected 1280 by 720 frame passed
  capture validation with 124 colour bins, 19.68 luminance deviation, and 42.29 spatial contrast.

### 2026-10-08 — Large-ship scale cues and scattered wreckage

- Confirmed and retained the exact 20.0 m length, 5.2 m beam, and 4.4 m hull height at the user's
  direction; this landmark is a large grounded wooden sailing ship, never a small boat. Added a
  split broken lower keel, sternpost/rudder silhouette, four paired broken rail spans, eight dark side
  openings, and a splintered broken foremast. The main mast, cabin, bowsprit, rigging, exposed ribs,
  partial deck, and central structural break remain recognizable.
- Increased the one static batch from 3,392 to 3,948 triangles. The 556 added triangles require no
  new draw, buffer, material, texture, collision proxy, update loop, or external asset. Reauthored
  the twelve existing debris yaws so ten substantial pieces diverge by at least 0.4 radians in both
  directions rather than reading as parallel bars. Added a deterministic core regression for
  bounded bidirectional scatter while keeping all debris non-solid and traversal-friendly.
- Added one exact large-ship diagnostic and required it in the native, Chrome, Firefox, and Android
  gates. The complete 27-stage Chrome/WebGL 2 matrix, independent Firefox ESR/WebGL 2 gate, and
  complete 46-test native private-display suite pass. The current native and browser 1280 by 720
  frames were inspected and clearly retain the new scale cues and scattered wreckage.
- A strict API 35 x86_64 KVM 12/host-gfxstream run selected `OPENGLES3`, required the large-ship
  marker, booted the AVD in 18.752 seconds, retained one 3,948-triangle wreck draw, and reached
  first draw in 2436.391 ms. Its clean
  inspected 2400 by 1080 frame visibly preserves the same composition and touch controls without
  fatal, shader, or `GL_INVALID_OPERATION` diagnostics. This is translated-host functional
  evidence rather than physical-device performance qualification.
- Stopped the emulator, restored/linted the default arm64-only APK through shared `/rv/cnaccache`,
  verified its sole `arm64-v8a` ABI and v2 signature, and removed exactly five reproducible x86_64
  intermediate directories (about 1.1 GiB). The APK is 19,075,204 bytes and its stripped
  `libmain.so` is 16,264,920 bytes; the Android tree returned to approximately 1.3 GiB.
- One isolated Linux Release shipwreck run retained 46 tracked submissions and one wreck draw. Its
  600 intervals measured 1.4082 ms mean, 1.1270 ms median, 1.8133 ms p95, 8.1463 ms p99, and
  16.2879 ms maximum; six exceeded 8.333 ms and none exceeded 16.667 or 33.333 ms.
- The deterministic Web package is 9,068,164 bytes uncompressed or 2,582,366 bytes through gzip:
  HTML is 2,058/1,144 bytes, JavaScript 264,591/59,260 bytes, and WebAssembly
  8,801,515/2,521,962 bytes. Two unchanged Linux Release packaging runs produced the same
  5,071,543-byte archive with SHA-256
  `177e3498a85ce8b3bf55699f41c87158b0823adac155657160e9509a4102d3d0` and 12,195,138
  installed bytes.
- The exact archive launched from an unrelated `/tmp` working directory, selected `OPENGL33`,
  required the large-ship marker, retained one wreck draw, and completed first draw in 408.252 ms.
  Its inspected 1280 by 720 frame passed capture validation with 183 colour bins, 36.34 luminance
  deviation, and 146.84 spatial contrast. CNA and Sharp Runtime remained read-only and clean; no
  qualifying dependency defect was found.

### 2026-10-08 — Sparse hanging palm dry-frond variation

- Added two downward-curving dry fronds to near palm variant zero and three to variant two. Four
  potential leaflet stations use deterministic omissions, unequal lengths, droop, and subdued
  ochre-brown shading, so only selected crowns show incomplete aged remnants rather than every
  shared silhouette receiving a regular skirt.
- The four stored near meshes gain 156 vertices and 82 triangles in total. The additions reuse the
  existing per-variant vertex/index buffer objects, wind shader, instance transforms, cells,
  culling, and near draw. No new material, texture, buffer object, instance field, collision proxy,
  population, far-LOD work, or submission was introduced. The current near variants span 526--607
  triangles; far variants are byte-for-byte unchanged.
- Added one exact startup diagnostic and required it in the native close-palm capture, both browser
  gates, and strict Android runtime gate. The complete native private-display suite passed 46/46
  with two workers. The complete 27-stage Chrome/SwiftShader WebGL 2 matrix and independent Firefox
  ESR/WebGL 2 gate passed; native and both browser close-palm frames were inspected.
- A strict API 35 x86_64 KVM 12/host-gfxstream fixed-palm run selected `OPENGLES3`, required the new
  marker, and reached first draw in 2413.984 ms. Its clean inspected 2400 by 1080 frame retains the
  sparse dry silhouettes and touch controls without fatal, shader, or `GL_INVALID_OPERATION`
  diagnostics. This is translated-host functional evidence, not physical-device performance.
- Restored and linted the arm64-only Android package through shared `/rv/cnaccache`, verified its
  sole `arm64-v8a` ABI and v2 signature, and removed the five reproducible x86_64 intermediate
  directories (about 1.1 GiB). The APK is 19,079,172 bytes and its stripped `libmain.so` is
  16,268,888 bytes; the Android tree returned to approximately 1.3 GiB.
- One isolated Linux Release palm benchmark retained 293 visible palms (47 near, 246 far), eight
  palm submissions, and 78 total tracked submissions. Its 600 intervals measured 1.0120 ms mean,
  0.9467 ms median, 1.4861 ms p95, 1.6594 ms p99, and 2.1476 ms maximum with zero 120/60/30 Hz
  budget misses.
- The deterministic Web package is 9,075,669 bytes uncompressed or 2,583,916 bytes through gzip:
  HTML is 2,058/1,144 bytes, JavaScript 264,591/59,251 bytes, and WebAssembly
  8,809,020/2,523,521 bytes. Its round-trip/hash/repeatability validation test passes.
- Two unchanged Linux packaging runs produced the same 5,075,420-byte archive with SHA-256
  `08cbaddf82e48bc20fb2cd429439d6bf96c91b60cee2d239e96a6b84988b0365` and 12,202,500 installed
  bytes. The exact archive launched from an unrelated `/tmp` working directory, selected
  `OPENGL33`, required the new marker, retained 293 palms in eight submissions, and completed first
  draw in 321.558 ms. Its inspected 1280 by 720 frame passed capture validation with 199 colour
  bins, 34.84 luminance deviation, and 146.03 spatial contrast. CNA and Sharp Runtime remained
  read-only and clean; no qualifying dependency defect was found.

### 2026-10-08 — Layered irregular near-canopy edges

- Reworked only the near broadleaf crown silhouettes after the fixed-view audit identified their
  repeated rounded lower edge beside the newly refined palms. Existing cluster rings now receive a
  bounded phase offset and ring/azimuth contour variation. Four alternating lobes per variant add
  one smaller pointed leaf with a lower root and stronger droop, preserving open gaps instead of
  simply inflating every crown.
- The four stored near variants gain 80 vertices and 64 triangles in total. They reuse the existing
  per-variant buffer objects, portable wind/facet/bark shader, instance transforms, cells, culling,
  and near draws. No material, texture, buffer object, instance field, collision proxy, population,
  far-LOD work, or submission was added; every far mesh remains byte-for-byte unchanged.
- Added one exact runtime marker and required it in the native jungle capture, both WebGL 2 gates,
  and strict Android runtime gate. The complete OpenGL 3.3 private-display suite passed 46/46 with
  two workers. The complete 27-stage Chrome/SwiftShader matrix and independent Firefox ESR gate
  passed; native and both browser jungle frames were inspected at 1280 by 720.
- A strict API 35 x86_64 KVM 12/host-gfxstream fixed-jungle run selected `OPENGLES3`, required the
  new marker, and reached first draw in 3070.691 ms. Its clean inspected 2400 by 1080 frame retains
  the irregular lower edges, secondary drooping leaves, ground layers, and touch controls without
  fatal, shader, or `GL_INVALID_OPERATION` diagnostics. This remains translated-host functional
  evidence, not physical-device performance.
- Restored and linted the arm64-only Android package through shared `/rv/cnaccache`, verified its
  sole `arm64-v8a` ABI and v2 signature, and removed the five reproducible x86_64 intermediate
  directories (about 1.1 GiB). The APK is 19,080,676 bytes and its stripped `libmain.so` is
  16,270,392 bytes; the Android tree returned to approximately 1.3 GiB.
- One isolated Linux Release jungle benchmark retained 109 visible canopy trees (63 near, 46 far),
  28 jungle submissions, and 89 total tracked submissions. Its 600 intervals measured 2.1422 ms
  mean, 2.0077 ms median, 2.9928 ms p95, 5.4067 ms p99, and 6.2373 ms maximum with zero 120/60/30
  Hz budget misses.
- The deterministic Web package is 9,081,790 bytes uncompressed or 2,586,476 bytes through gzip:
  HTML is 2,058/1,144 bytes, JavaScript 264,591/59,253 bytes, and WebAssembly
  8,815,141/2,526,079 bytes. Its round-trip/hash/repeatability validation test passes.
- Two unchanged Linux packaging runs produced the same 5,074,648-byte archive with SHA-256
  `c14e5ab64b8a6c058ef90acd3fa12a68e2f36d12d60991bb89c134d2b06825a3` and 12,204,004 installed
  bytes. The exact archive launched from an unrelated `/tmp` working directory, selected
  `OPENGL33`, required the new marker, retained the unchanged 28 jungle submissions, and completed
  first draw in 506.911 ms. Its inspected 1280 by 720 frame passed capture validation with 166
  colour bins, 35.88 luminance deviation, and 185.39 spatial contrast. CNA and Sharp Runtime
  remained read-only and clean; no qualifying dependency defect was found.

### 2026-10-08 — Irregular near trunks and bent support branches

- Refined only the four stored near-canopy meshes after the layered-crown review showed that the
  supporting structure still read as regular cylinders. Each trunk retains the same ground and
  crown anchors while receiving a restrained secondary growth bend, small ring-to-ring girth
  variation, and asymmetric radial variation. Each of seven support branches now tapers through a
  deterministic off-axis middle ring, yielding two connected segments instead of one straight
  tube.
- The branch change adds 28 vertices and 56 triangles per stored variant, or 112 vertices and 224
  triangles total; trunk vertex counts do not change. Existing per-variant buffers, the portable
  wind/facet/bark shader, transforms, cells, culling, and near draw are reused. No material,
  texture, buffer object, instance field, collision proxy, population, far-LOD work, simulation,
  or submission was introduced; every far mesh remains byte-for-byte unchanged.
- Added one exact runtime marker and required it in the native jungle capture, both WebGL 2 gates,
  and strict Android runtime gate. The complete OpenGL 3.3 private-display suite passed 46/46 with
  two workers. The complete 27-stage Chrome/SwiftShader matrix and independent Firefox ESR gate
  passed; native and both browser jungle frames were inspected at 1280 by 720 and preserve the
  same composition.
- A strict API 35 x86_64 KVM 12/host-gfxstream fixed-jungle run selected `OPENGLES3`, required the
  new marker, and reached first draw in 2255.331 ms (694.128 ms world, 2.885 ms framework,
  1484.222 ms content, 20.835 ms pre-draw, and 53.261 ms first draw). Its clean inspected 2400 by
  1080 frame retains the restrained structure, vegetation layers, and touch controls without
  fatal, shader, or `GL_INVALID_OPERATION` diagnostics. This remains translated-host functional
  evidence, not physical-device performance.
- Restored and linted the arm64-only Android package through shared `/rv/cnaccache`, verified its
  sole `arm64-v8a` ABI and v2 signature, and removed the five reproducible x86_64 intermediate
  directories (about 1.1 GiB). The APK is 19,082,004 bytes and its stripped `libmain.so` is
  16,271,720 bytes; the Android tree returned to approximately 1.3 GiB.
- Two isolated Linux Release jungle runs retained 109 visible canopy trees (63 near, 46 far), 28
  jungle submissions, and 89 total tracked submissions. The first measured 2.3036 ms mean,
  2.0550 ms median, 3.4578 ms p95, 7.0479 ms p99, and 10.9335 ms maximum with two 120 Hz misses;
  the repeat measured 2.2655/1.9760/3.9955/6.4305/7.3328 ms with zero misses. Neither run missed
  the 60 or 30 Hz budgets; both results are retained rather than discarding the noisier run.
- The deterministic Web package is 9,085,111 bytes uncompressed or 2,587,860 bytes through gzip:
  HTML is 2,058/1,144 bytes, JavaScript 264,591/59,252 bytes, and WebAssembly
  8,818,462/2,527,464 bytes. Its round-trip/hash/repeatability validation test passes.
- Two unchanged Linux packaging runs produced the same 5,075,520-byte archive with SHA-256
  `ccbfb6a7ea40189bcbde20d264dd20579232348653715af29ac280b4225f2489` and 12,204,685 installed
  bytes. The exact archive launched from an unrelated `/tmp` working directory, selected
  `OPENGL33`, required the new marker, retained the unchanged 28 jungle submissions, and completed
  first draw in 298.290 ms. Its inspected 1280 by 720 frame passed capture validation with 165
  colour bins, 35.87 luminance deviation, and 185.38 spatial contrast. CNA and Sharp Runtime
  remained on their required `next` revisions, read-only and clean; no qualifying dependency
  defect was found.

## Session-end checklist

- [ ] Keep source and docs coherent and in English.
- [ ] Build/test the relevant targets within the CPU policy when practical.
- [ ] Update task statuses, new discoveries, unresolved bugs, and exact next work here.
- [ ] Update `BUGS.md` for confirmed dependency defects; do not patch dependencies.
- [ ] Do not claim Web or Android runtime support from compilation alone.
- [ ] Inspect `git status` and `git diff --check`; stage only intended files.
- [ ] Measure project-owned disk use and remove only clearly obsolete owned artifacts.
- [ ] Record build directories used and maximum job count in the handoff/final report.
