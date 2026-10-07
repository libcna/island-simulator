# island-simulator

`island-simulator` is a peaceful first-person tropical island walking and near-shore swimming
experience built in C++23 with CNA. The project remains under an active long-form development and
polish campaign. The current playable slice renders a deterministic approximately 500 m island with
portable procedural
sand, wet-edge, soil, and slope material variation, including distance-faded interior substrate
microvariation, sparse curved leaf litter, close dry-sand granulation, soft-edged pebble/shell
flecks, subtle shoreline-raked ripples, restrained fixed-sun jungle-floor dappling, and a narrow
noise-mottled, broken-edge
earthen footpath that follows the same authored corridor reserved by vegetation. Two
authored clearings use distinct sunlit-dry and leaf-rich ground treatments with broad irregular
patch breakup derived from terrain noise that was already evaluated. An animated tropical
ocean adds distance-faded crossing-wave colour structure, restrained shallow-ridge light and
sunward ripple crests plus view-aligned grazing reflection bands from its existing slope field,
and an exact shore-following foam/wash ribbon,
330 varied procedural palms, 281 broadleaf canopy trees, 1,822 ferns/large-leaf plants, 565 low
bushes, 2,225 small forest-floor plants, and 18,556 mesh-grass clusters. It also composes 157
irregular land rocks, 14 emergent near-shore reef outcrops, 13 mossy fallen logs with irregular
near broken-end contours,
31 pieces of sun-bleached beach driftwood, 63 curved fallen palm fronds with asymmetric leaflets,
and 265 near-field
shell/pebble/dry-wrack clusters. Three varied driftwood silhouettes form a non-solid landmark arc
in one sheltered cove;
five rocks and two fallen logs form natural orientation landmarks around the outer rings of the
two authored clearings without blocking their central walking room. Every dense near-island
visual layer has spatial culling and appropriate distance
reduction, while vegetation uses a calm breeze. Five unreachable, haze-softened low islets and 11
simplified palm silhouettes break up the open-ocean horizon in one intentionally always-present
portable static batch.
Nearby mesh grass bends away from the player: a small stationary body radius expands smoothly with
walking speed and releases after movement stops, entirely inside the existing instanced shader.
The near mesh uses five curvature levels per blade, folded side colour, and portable two-sided
daylight/root/transmission shading. A sparse juvenile underlayer, four restrained seed-bearing
stems in the near cluster, and deterministic per-instance blade stature, colour, and wind variation
keep neighbouring tufts from repeating the shared mesh literally; the cheaper far mesh uses three
levels and still merges into the terrain before culling.
Nearby palm crowns use nine paired, fuller tapered-leaflet stations on each of 10--12 curved fronds
instead of solid ribbon silhouettes. A cheaper widened ribbon preserves the crown envelope at
distance,
while per-palm 125--145 m thresholds stagger the transition and avoid switching whole cells at once.
The near tier also adds texture-free leaf-scar rings to the trunks and restrained faceted coconut
clusters to two of the four variants without another draw submission. Five irregular faceted root
wedges vary in angle, length, width, and height around each near trunk instead of ending it in a
flat cylinder or a regular stamped star. The palm shader also derives a restrained green-only
frond tint from each existing instance transform, so neighbouring crowns no longer share one
literal green while trunks, roots, coconuts, near/far identity, buffers, and draw count remain
unchanged.
Nearby broadleaf crowns use subtly irregular, separated lobes, visible tapered support branches,
and pointed perimeter leaves. Two of the four near variants also carry one sparse hanging liana
with a curved tapered strand and two small leaves, adding vertical jungle detail without another
instance or draw. Their near trunks now meet the forest floor through four or five
short irregular buttress roots instead of a uniform cylinder, using the same instanced mesh and
draw submissions. Close bark gains restrained procedural longitudinal grain and narrow fissures
that fade before the far tier, without a texture or new pass. A deterministic per-tree 105--129 m
threshold staggers their
transition to a rounded 208-triangle far mesh instead of switching complete scene cells at once.
The shared portable jungle shader reconstructs a two-sided geometric face normal from world-space
derivatives, so canopy, understory, bush, and ground-plant facets respond coherently to the fixed
sun without normal buffers, textures, or another draw.
Its vertex stage also recombines each existing instance transform's scale/yaw coefficients into a
stable restrained foliage tint. The green-only mask leaves bark unchanged, and the shared transform
keeps the same tint identity across near/far LOD without another attribute, texture, or draw.
Nearby low bushes replace their largest smooth lobes with smaller overlapping volume and paired
folded, pointed leaves, preserving the inexpensive 48-triangle far tier and existing submissions.
One near forest-floor variant forms six upright broad leaves on visible petioles, while another
uses five low folded split leaves instead of repeating the strap-leaf rosette. Their
compact far representations and batching remain unchanged. Understory, bush, and forest-floor
LOD and maximum-distance selection use actual instance positions after a conservative cell reject,
so first-person detail no longer changes early at a coarse cell boundary.
Near land and reef rocks gain subtle texture-free strata and restrained lichen flecks from their
portable material shader. The effect is deliberately limited to the near LOD so distant rock
silhouettes remain calm and avoid unnecessary fragment work. A fixed mean-sea-level band subtly
darkens and cools their lowest near fragments so emergent outcrops no longer read as uniformly dry
where they meet the water. Near-rock crown heights and asymmetric caps now break the former flat
top line, while derivative-based fixed-sun facet shading makes the deliberately low-poly planes
read clearly without another normal stream or draw. A restrained warm/cool tint derived from each
existing instance transform varies neutral stone tones while preserving one identity across the
near/far LOD and leaving wood and foliage colours unchanged.
Nearby fallen logs use the same portable material path for gently irregular cut-end growth rings,
restrained radial drying cracks, and longitudinal bark fissures. Both rock and wood detail fade
out before their LOD transition.
Sun-bleached driftwood adds a still softer near-only longitudinal weathering treatment. Its third
near variant replaces a simple spike with three curved tapered fork limbs, while cut ends and all
far meshes stay calm and inexpensive. Rocks, fallen logs, and driftwood select range and LOD from
each instance after a conservative cell-bounds rejection rather than switching an entire cell.
Nearby palms, canopy trees, land rocks, fallen wood, driftwood, and monkeys receive subtle
terrain-conforming contact shadows. The twelve-segment soft ellipses use one portable
`BasicEffect` batch, fade before their bounded draw distances, and cache static terrain sampling
plus movement-thresholded uploads so this grounding cue does not become a per-frame procedural
terrain cost.
It supports
first-person terrain walking, near-shore swimming, trunk collision, and a soft ocean-current
boundary whose always-shoreward return includes mild coherent coast-parallel circulation,
dual-zone touch input, a subtle depth-responsive near-surface water veil with upper-surface light
scatter that cools and strengthens as the offshore current develops, and speed/depth-responsive
player ripples that expand across the
same animated ocean surface without another draw call. Moving entries and exits also trigger one
short speed-scaled pair of broken surface rings, with depth hysteresis and a cooldown that prevent
shoreline chatter. A fixed sunny gradient sky adds a sun disc
and sparse, layered horizontal high-cloud wisps without a weather simulation. A first
flock of 24 procedural birds follows varied shore, canopy, and high-altitude routes with animated
wing strokes, layered trailing flight feathers on the near silhouette, intermittent glides, banked
turns, slow radial route drift, terrain clearance, visibility culling, and distance-throttled
simulation. Eight
near-shore schools add 48 animated fish constrained between the seabed and water surface, with
depth-following pitch, banked turns, forked tails, CPU-lit body facets, restrained depth/caustic
colour response, and readable near heads.
One non-hostile shark
occasionally patrols deeper offshore water with matching dive pitch, turn banking, a forked tail,
depth-integrated colour, and restrained near eye/gill detail.
Seven original procedural monkeys roam the interior using a compact, obstacle-aware state machine,
three silhouette meshes, four material palettes, portable CNA GPU skeletal animation, and short
state-transition crossfades. Near animals add restrained crown/cheek fur tufts plus bone-following
finger and toe silhouettes in the existing body draw. Calm idle/observe animals gradually turn
their existing head bone
toward a nearby player within a restrained angle, then relax or retreat without becoming hostile.
The current audio pass adds original procedural shoreline surf, two
varied occasional stronger waves, light canopy breeze, a subtle daytime-jungle texture,
speed/depth-responsive movement through shallow or swimming water, and two restrained
water-entry/exit textures. The same continuous water layer now develops a subtle deeper undertow
bed as the 42--72 m offshore current strengthens, without adding another persistent source. Two
bird phrases and two monkey contact phrases share one positional
voice attached to actual nearby agents. All four persistent loops remove their measured mean
before PCM quantization, avoiding long-lived DC bias without altering one-shot silent endpoints.
Distance-scheduled player footsteps select paired dry-sand or interior-soil textures from actual
resolved travel, vary pitch/pan/gain, and remain silent while swimming or blocked.
All audio is generated without external sound files. The
complete scene runs on Linux/OpenGL 3.3, in an actual Chromium/WebGL 2 context, and in a
KVM-accelerated Android API 35 x86_64 emulator through OpenGL ES 3. The accelerated Android pass
also exercises movement and look gestures, AAudio startup, and background/foreground recovery.
Physical-device touch ergonomics, thermals, and sustained mobile-GPU performance are not yet
qualified.

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

Run `build/island-simulator`. Click inside the window or press a movement key to capture the mouse,
use WASD or the arrow keys to move, and press Shift to toggle between walking and running on land.
Each keyboard toggle briefly confirms `WALK MODE  4.3 M/S` or `RUN MODE  8.0 M/S` near the bottom
of the view; no key must remain held.
The executable may be launched from any working directory; current runtime content is embedded or
generated and does not depend on a repository-relative asset path.
On Linux, press Escape once to release the mouse and again to exit. In a browser, Escape only
releases pointer lock; close the tab through the browser UI when finished. Mouse look remains
active while a movement key is held. The controller follows the terrain,
rejects overly steep uphill motion, slides along trunk, solid-rock, and fallen-log proxies, and
enters swimming water with hysteresis. Swimming retains collision with solid emergent reef rocks.
A static 16 m collision grid limits each capsule test to nearby proxies; bounded one-time recovery
prevents a placement or debug teleport from leaving the player embedded in a static proxy.
Geometric step-up and continuous recovery from moving obstacles are not implemented yet.
Walking and swimming apply restrained, speed-responsive positional camera motion; press B to
smoothly disable or restore it.
Press F1 to toggle the developer overlay with frame timing, player/water state, renderer identity
and the actual logical backbuffer size,
culling counts, contact-shadow class/draw statistics, vegetation draw statistics, ambient mix
levels, grass-interaction strength, player-ripple and transition-burst intensity/phase, player-water
gain, wave/splash counts, undertow cue strength, and positional bird/monkey call activity.
While F1 is visible, F2 toggles an x-ray world diagnostic: the authoritative shoreline and three
ocean-current thresholds, nearby collision proxies, the player footprint, shared 8 by 8 scene
cells, all nine terrain-contact samples and normals, wildlife headings, simulation-tier colours,
nearby fish-school centres, the shark route target, monkey home ranges and active movement targets,
three bird-family route volumes, all 24 authored bird spawn headings, and nearby live radial targets,
the last exact attempted/resolved player step with rejection classification,
the exact active inward/circulating-current vector, all six ambient crossfade boundaries, the nearest bird/monkey
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
current/grass/frond/distant/bird/fish/shark/monkey screenshots, the transient run-mode confirmation,
and portable overlay text rendering.
The close prop view additionally requires a nonzero one-draw contact-shadow batch.
The driftwood view frames the authored three-silhouette cove group, including the forked third
variant, and requires a nonzero per-instance near-LOD population.
The Chromium harness also drives first-click pointer lock and initialization of all ambient and
positional-wildlife audio resources, WASD, arrow-key movement, Shift run/walk toggling,
simultaneous forward movement and relative mouse look, a deterministic beach-to-water crossing with successful
transition-audio playback, successful land-footstep playback, F2 world
lines, F4 seed-copy execution with explicit clipboard status, F1,
real-Escape pointer release, continued live frames, delayed click reacquisition, canvas focus, and
responsive 16:9 sizing. A narrower Firefox ESR 140.15 WebDriver BiDi gate independently checks the
production shell's first frame, reported WebGL 2 Radeon backend, trusted-click pointer lock,
44.1 kHz audio initialization, persistent Shift enable/disable edges, forward traversal with an
actual footstep, Escape release, delayed pointer-lock reacquisition, and responsive 16:9 sizing.
It also loads dedicated 1280 by 720 jungle and near-surface-water scenes, requires their live
LOD/water state, and captures both. All four Firefox frames were visually inspected. A visible
interactive listening pass, the remaining fixed-scene matrix, long tab suspension, audio-focus
behaviour, and fullscreen remain open. The automated Firefox path now does one actual second-tab
activation cycle and requires `visible` to `hidden` to `visible`, resumed animation frames, and
fresh game input after foregrounding.

After the WebGL 2 release build, `tools/package_web_release.sh build-probe` creates
`build-probe/web-release/` with verified uncompressed artifacts, deterministic gzip variants, and
a SHA-256 manifest. The current gzip payload is 2,498,912 bytes, 71.6% below the 8,812,570-byte
uncompressed package. See `docs/build-web.md` for required server content types and
content-encoding headers; the quick Python development server does not exercise compressed
delivery.

The Android project produces arm64-v8a device and x86_64 emulator APKs with an explicit GLES 3
renderer. The left half of the screen supplies a floating movement stick and the right half uses
relative drag-to-look, with subtle generated premultiplied-alpha control indicators. Near-full
movement-stick travel selects the faster land pace with a warm-gold visual cue; easing inward
returns to analog walking. A small three-dot control at top right cycles low, standard, and high
look sensitivity (0.72/1.00/1.35 times); the highlighted dot shows the current choice. Android
Emulator 36.5.11 runs the API 35 x86_64 AVD with KVM CPU acceleration and host-GPU gfxstream.
The complete current scene, independent movement/look gestures, all sensitivity levels, a
same-process hot resume, force-stop/new-process relaunch, and AAudio pause/resume are validated
there. This is meaningful emulator qualification, but it does not replace physical-device
ergonomics, thermal, driver, and sustained-performance testing. See
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
