# island-simulator

`island-simulator` is a peaceful first-person tropical island walking and near-shore swimming
experience built in C++23 with CNA. The project is at the beginning of a long development campaign.
The current slice renders a deterministic approximately 500 m island with portable procedural
sand, wet-edge, soil, and slope material variation, including distance-faded interior substrate
microvariation, sparse curved leaf litter, and restrained fixed-sun jungle-floor dappling, plus an
animated tropical ocean with an exact
shore-following foam/wash ribbon,
330 varied procedural palms, 281 broadleaf canopy trees, 1,822 ferns/large-leaf plants, 565 low
bushes, 2,225 small forest-floor plants, and 18,556 mesh-grass clusters. It also composes 152
irregular land rocks, 14 emergent near-shore reef outcrops, 11 mossy fallen logs,
28 pieces of sun-bleached beach driftwood, 63 fallen palm fronds, and 265 near-field shell/pebble
clusters. Every dense near-island visual layer has spatial culling and appropriate distance
reduction, while vegetation uses a calm breeze. Five unreachable, haze-softened low islets and 11
simplified palm silhouettes break up the open-ocean horizon in one intentionally always-present
portable static batch.
Nearby mesh grass bends away from the player: a small stationary body radius expands smoothly with
walking speed and releases after movement stops, entirely inside the existing instanced shader.
The near mesh uses five curvature levels per blade, folded side colour, and portable two-sided
daylight/root/transmission shading; the cheaper far mesh uses three levels and still merges into
the terrain before culling.
Nearby palm crowns use nine paired tapered-leaflet stations on each of 10--12 curved fronds instead
of solid ribbon silhouettes. A cheaper widened ribbon preserves the crown envelope at distance,
while per-palm 125--145 m thresholds stagger the transition and avoid switching whole cells at once.
The near tier also adds texture-free leaf-scar rings to the trunks and restrained faceted coconut
clusters to two of the four variants without another draw submission.
Nearby broadleaf crowns use subtly irregular, separated lobes, visible tapered support branches,
and pointed perimeter leaves. A deterministic per-tree 105--129 m threshold staggers their
transition to a rounded 208-triangle far mesh instead of switching complete scene cells at once.
Nearby low bushes replace their largest smooth lobes with smaller overlapping volume and paired
folded, pointed leaves, preserving the inexpensive 48-triangle far tier and existing submissions.
One near forest-floor variant now forms six upright broad leaves on visible petioles instead of
another low strap-leaf rosette; its compact far representation and batching remain unchanged.
Near land and reef rocks gain subtle texture-free strata and restrained lichen flecks from their
portable material shader. The effect is deliberately limited to the near LOD so distant rock
silhouettes remain calm and avoid unnecessary fragment work.
Nearby fallen logs use the same portable material path for gently irregular cut-end growth rings
and longitudinal bark fissures. Both rock and wood detail fade out before their LOD transition.
Sun-bleached driftwood adds a still softer near-only longitudinal weathering treatment while
keeping its cut ends calm and its distant mesh plain.
Nearby palms, canopy trees, land rocks, fallen wood, driftwood, and monkeys receive subtle
terrain-conforming contact shadows. The twelve-segment soft ellipses use one portable
`BasicEffect` batch, fade before their bounded draw distances, and cache static terrain sampling
plus movement-thresholded uploads so this grounding cue does not become a per-frame procedural
terrain cost.
It supports
first-person terrain walking, near-shore swimming, trunk collision, a soft ocean-current boundary,
dual-zone touch input, a subtle depth-responsive near-surface water veil that cools and strengthens
as the offshore current develops, and speed/depth-responsive player ripples that expand across the
same animated ocean surface without another draw call. A fixed sunny gradient sky adds a sun disc
and sparse, layered horizontal high-cloud wisps without a weather simulation. A first
flock of 24 procedural birds follows varied shore, canopy, and high-altitude routes with animated
wing strokes, terrain clearance, visibility culling, and distance-throttled simulation. Eight
near-shore schools add 48 animated fish constrained between the seabed and water surface. One
non-hostile shark occasionally patrols deeper offshore water with a varied dive and tail cycle.
Seven original procedural monkeys roam the interior using a compact, obstacle-aware state machine,
three silhouette meshes, four material palettes, portable CNA GPU skeletal animation, and short
state-transition crossfades. The current audio pass adds original procedural shoreline surf, two
varied occasional stronger waves, light canopy breeze, a subtle daytime-jungle texture,
speed/depth-responsive movement through shallow or swimming water, and two restrained
water-entry/exit textures. The same continuous water layer now develops a subtle deeper undertow
bed as the 42--72 m offshore current strengthens, without adding another persistent source. Two
bird phrases and two monkey contact phrases share one positional
voice attached to actual nearby agents. All audio is generated without external sound files. The
complete scene runs on Linux/OpenGL 3.3 and in an actual
Chromium/WebGL 2 context. An Android API 35 x86_64 emulator rendered the earlier
terrain/palm/grass scene through OpenGL ES 3; the current complete arm64 APK builds and lints, but
its runtime and real-device touch feel are not yet qualified.

There are deliberately no quests, combat, inventory, crafting, survival systems, score,
progression, dynamic weather, or day/night cycle.

## Dependency baseline

The project consumes read-only CNA `next` and Sharp Runtime `next` checkouts. Exact qualified
commits are stored in `dependencies.lock`. The current workspace checkouts are both on `next`; the
project still creates an ignored, unmodified CNA source export so its build input remains pinned:

```bash
tools/materialize_cna_next.sh
```

This does not switch or edit the CNA repository. A separate read-only CNA checkout on `next` can be
used instead with `-DCNA_ROOT_DIR=/path/to/cna`.

## Linux development build

The machine-wide ccache is required. Sharp Runtime's stricter build policy limits builds that
compile its sources to two parallel jobs.

```bash
export CCACHE_DIR=/rv/cnaccache
export CCACHE_BASEDIR=/rv
tools/materialize_cna_next.sh
cmake -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_C_COMPILER_LAUNCHER=ccache \
  -DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
  -DCNA_GRAPHICS_RENDERER=OPENGL33
cmake --build build --target island-simulator island_core_tests island_world_report \
  island_audio_report --parallel 2
ctest --preset linux-core
```

The graphical tests need an isolated display; use the command in `docs/build-linux.md` rather than
running the full CTest set on the owner's live desktop.

Run `build/island-simulator`. Click inside the window to capture the mouse, use WASD to move, and
press Escape to release the mouse; press Escape again to exit. The controller follows the terrain,
rejects overly steep uphill motion, slides along trunk, solid-rock, and fallen-log proxies, and
enters swimming water with hysteresis. Swimming retains collision with solid emergent reef rocks.
A static 16 m collision grid limits each capsule test to nearby proxies;
geometric step-up and collision depenetration are not implemented yet.
Walking and swimming apply restrained, speed-responsive positional camera motion; press B to
smoothly disable or restore it.
Press F1 to toggle the developer overlay with frame timing, player/water state, renderer identity,
culling counts, contact-shadow class/draw statistics, vegetation draw statistics, ambient mix
levels, grass-interaction strength, player-ripple intensity/phase, player-water gain, wave/splash
counts, undertow cue strength, and positional bird/monkey call activity.
While F1 is visible, F2 toggles an x-ray world diagnostic: the authoritative shoreline and three
ocean-current thresholds, nearby collision proxies, the player footprint, shared 8 by 8 scene
cells, all nine terrain-contact samples and normals, wildlife headings, simulation-tier colours,
nearby fish-school centres, the shark route target, monkey home ranges and active movement targets,
three bird-family route volumes, all 24 authored bird spawn headings, and nearby live radial targets,
the last exact attempted/resolved player step with rejection classification,
the active inward-current vector, all six ambient crossfade boundaries, the nearest bird/monkey
audio candidates, and any active positional wildlife emitter. The panel also reports active audio
sources against the six-source ceiling.
In debug builds, F3 toggles a bounded inspection camera while F1 is visible. Mouse look remains
unchanged; WASD flies horizontally, Q/E moves down/up, and Shift increases speed. The player stays
fixed, and hiding F1 immediately restores the first-person camera.
The overlay also shows the four authoritative procedural-layout seeds. Press F4 while it is visible
to copy a stable seed report; the panel reports `COPIED` only after reading back the same text and
otherwise reports `UNAVAILABLE`.
Press M to mute or unmute the soundscape. Press B to toggle locomotion camera motion. While the
overlay is visible, F5 through F12 teleport among eight fixed QA locations, including views toward
the south fish shallows, offshore shark, monkey troop, and the active offshore-current field.
The overlay starts
hidden in a normal run and can be enabled at launch with `--debug-overlay` for automated QA.
Use `--world-debug` to enable both the panel and world visualization at launch.
Use `tools/run_native_benchmarks.sh` through the private GPU runner for warm, uncapped JSON
frame-time reports across five fixed scenes plus a 676 m moving traversal. Reports include
min/max/mean scene-population ranges as well as timing distributions; methodology and the initial
Linux baseline are in `docs/performance.md`.
Use `--startup-report <path>` to write a machine-readable first-draw report. It separates
deterministic world construction, framework initialization, GPU/content creation, the interval
before the first draw, and the first submitted draw. Combine it with `--smoke-test --overview` for
an unattended run; the measurement ends before CNA presents the frame and is therefore not a
GPU-completion timer. The native CTest suite validates the report after an actual OpenGL 3.3 draw.
Use `python3 tools/validate_assets.py` to enforce the asset provenance gate and
`build/island-world-report --output <path>` for deterministic terrain, placement, collision,
wildlife, seed, and fingerprint statistics. Native overview captures are followed by
`tools/validate_capture.py`, which rejects truncated, black/white, flat, or spatially degenerate
P6 output and writes machine-readable metrics. See `docs/asset-pipeline.md` and
`docs/debugging.md`.
Use `build/island-audio-report --output <path> --wav-directory <directory>` to audit every
procedural source and export disposable PCM16 WAV previews for focused listening. See
`docs/audio-review.md`.

The automated browser smoke test verifies renderer initialization, the portable terrain-material,
animated-ocean, and fixed-sky shaders, hardware-instanced palm/jungle/prop LOD, portable instanced
grass wind, a finite first-draw startup timing line, four frames, clean shutdown, active
overview/beach/jungle/prop/driftwood/reef/water/
current/grass/frond/distant/bird/fish/shark/monkey screenshots, and portable overlay text rendering.
The close prop view additionally requires a nonzero one-draw contact-shadow batch.
The Chromium harness also drives first-click pointer lock and initialization of all ambient and
positional-wildlife audio resources, WASD, relative mouse
look, a deterministic beach-to-water crossing with successful transition-audio playback, F2 world
lines, F4 seed-copy execution with explicit clipboard status, F1,
pointer release/reacquisition, canvas focus, and responsive 16:9 sizing. A visible hardware-
accelerated browser listening pass, genuine tab background/foreground behaviour, and another
browser engine remain open.

The Android project produces arm64-v8a device and x86_64 emulator APKs with an explicit GLES 3
renderer. The left half of the screen supplies a floating movement stick and the right half uses
relative drag-to-look, with subtle generated control indicators. The first software-emulated GLES3
frame is validated, but the available host has no KVM and its Android System UI is unstable under
software emulation; lifecycle, audio, touch ergonomics, and mobile performance remain open. See
`docs/build-android.md` and `PLAN.md` for exact evidence and commands.

## Project records

- `PLAN.md` is the living development ledger.
- `BUGS.md` records confirmed CNA or Sharp Runtime defects and game-side workarounds.
- `ASSET_LICENSES.md` records all asset provenance before assets enter a runtime build.
- `docs/asset-pipeline.md` defines the manifest, validation gate, deterministic world report, and
  external-asset intake procedure.
- `docs/audio-review.md` defines deterministic source analysis, WAV preview export, and the
  remaining subjective listening protocol.
- `docs/performance.md` defines the repeatable benchmark method, current evidence, and open budgets.
