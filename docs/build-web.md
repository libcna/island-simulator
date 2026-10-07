# WebGL 2 Build and Browser Validation

## Current status

The terrain/traversal/jungle scene builds with Emscripten and runs in Chromium using CNA's `WEBGL2`
renderer. Automated evidence covers renderer initialization, clean short-run lifecycle, live
overview/beach/grass/jungle/prop/driftwood/reef/water/current/distant/shipwreck/crabs/butterflies/turtles rendering, first-click pointer
lock, canvas focus, WASD/arrow traversal, Shift run/walk toggling, simultaneous movement/relative
mouse look, the B camera-motion opt-out, F1 input, pointer release/reacquisition, and responsive
16:9 presentation. The Chrome gate also restarts the production runtime on the same origin and
profile and requires the B selection to survive through CNA's IDBFS storage. F2 world-line rendering is also toggled
through browser input, and F4 must execute its explicit confirmed/unavailable clipboard path. The
first click opens
CNA's 44.1 kHz mixer, starts all four procedural loops,
and prepares both wave/splash and sand/soil footstep pairs plus the pooled positional bird/monkey
call resources. The F2
path includes the same nine terrain-contact samples/normals, attempted/resolved movement trace,
fish-school/shark/monkey navigation guides, three bird route-family volumes, all 24 bird spawn
headings, nearby live bird radial targets, six terrain-following ambient-zone contours, and
positional-source markers.
Audible output on a visible hardware browser, long tab suspension/audio-focus recovery,
hardware-GPU performance, and extended play are not yet validated. Firefox ESR 140.15 now provides
a narrower independent second-engine gate for the production first frame, essential input/audio
contract, and dedicated live jungle, near-surface-water, shipwreck, beach-crab, butterfly, plus
sea-turtle
scenes; it does not duplicate the
complete Chromium scene matrix.
The F1 panel exposes the same authoritative seed rows as desktop. F4 attempts to copy their stable
text report, but browser clipboard permission and user-gesture policy can prevent immediate
readback; the panel then reports `UNAVAILABLE` instead of claiming success.

The renderer gate also requires the exact shore-following animated foam ribbon rather than its
stock-effect fallback. The close prop camera additionally requires a nonzero terrain-conforming
contact-shadow population in exactly one portable draw. The beach and grass cameras require the
five-level near-blade curvature, portable two-sided daylight path, selected near-blade geometric
V-fold ridge, and narrower asymmetric/occasional bent-tip close-blade geometry rather than only
accepting the older flat-colour shader. The lifecycle,
overview, beach, and grass cameras also require the
coherent coast/path/clearing grass biome-colour path. The lifecycle plus close grass and jungle
cameras also require the sparse near seed-head path. The lifecycle plus close grass and jungle
cameras require the near-field
terrain microvariation and procedural leaf-litter path. The lifecycle and dedicated jungle camera
also require the inland humus/moss material diagnostic, preventing a silent return to the former
uniform green interior substrate. The lifecycle and dedicated jungle camera
also require derivative-based, two-sided fixed-sun facet lighting on the instanced vegetation
shader plus sparse hanging leafy lianas on selected near-canopy variants rather than accepting its
unlit or less-layered fallback. The lifecycle, beach approach, and jungle camera
also require the authored primary-footpath terrain material diagnostic. Dedicated east/west
clearing cameras require their shared authored-material and reused-noise broad-patch diagnostics
plus exact QA-camera identity.
The east camera additionally requires the explicit warm-dry ground-cue diagnostic.
The lifecycle, beach approach, and jungle gates additionally require the narrow irregular-wear
diagnostic, preventing a silent return to the former broad, uniformly coloured path treatment.
The lifecycle and beach gates also require the sparse close sand pebble/shell diagnostic; dedicated
driftwood and fallen-frond captures expose that material at first-person scale.
The lifecycle and dedicated driftwood gates also require tapered three-dimensional branches on all
three near driftwood variants while retaining the established three-limb-fork diagnostic. They
also require the shared irregular-body/end-cap-ring diagnostic.
Both clearing cameras also require the shared five-rock/two-log authored-landmark diagnostic.

The latest automated run used Chrome 152.0.7977.82 with SwiftShader and confirmed exact F4
clipboard readback on its focused localhost canvas. All 27 reported lifecycle, deployment,
renderer, scene, and input stages passed against CNA `next` `97783d1e090c`; the current rebuilt
WebAssembly is 8,801,515 bytes. The dedicated shipwreck stage requires the exact 20 m broken-hull,
12-debris, large-ship-detail, one-draw diagnostics plus the portable aged-wood/waterline material
marker; its refined curved hull, broken rails/foremast, keel/rudder, side openings, and irregularly
scattered debris frame was inspected. A separate crab
stage requires the thirty-agent colony marker, a nonzero visible population, and exactly one draw;
its original-resolution frame was also inspected. The butterfly stage likewise requires the
eighteen-agent habitat marker, a visible near population, and exactly one draw. The turtle stage
requires all five agents, a visible near turtle, and exactly one draw. The independent Firefox ESR
gate requires and captures all four live wildlife/landmark scenes. The deployment
gate additionally requires the exact persistent-
ambient DC-centering and land-footstep resource diagnostics after browser-gesture audio startup.
The monkey scene requires the
near crown/cheek-tuft and bone-following finger/toe diagnostics, and its inspected frame retains
the complete facial-detail/crossfade path. The input
stage captures and visually validates the warm-gold `RUN MODE  8.0 M/S` confirmation while still
proving that releasing Shift retains running and a second press restores walking. The input path
also requires a successful distance-scheduled land-footstep playback diagnostic while
the player actually traverses the beach. The first-frame
gate also requires the reported 1280 by 720 logical
backbuffer rather than inferring render resolution from the browser surface. This records the
evidence environment; it is not a minimum-version or general browser-permission claim.

## Prerequisites

- the Emscripten SDK at `/home/robertvokac/emsdk`, or another installation exposed through `EMSDK`;
- Google Chrome or Chromium for the automated browser smoke test;
- Firefox ESR with its built-in WebDriver BiDi endpoint for the optional second-engine smoke test;
- the same exact read-only dependency revisions listed in `dependencies.lock`;
- the CNA SDL submodule snapshots already populated in the source CNA worktree;
- the shared compiler cache at `/rv/cnaccache` in this workspace.

`tools/materialize_cna_next.sh` copies the three pinned SDL snapshots into the ignored CNA source
export after verifying that the containing checkout and CNA `next` use identical SDL gitlinks. It
does not initialize, switch, or edit the CNA repository. Configure-time SDL build products go to
the writable project cache `build/cna-sdl-prebuilt-emscripten`, never into the CNA export.

## Configure and build

```bash
source /home/robertvokac/emsdk/emsdk_env.sh
export CCACHE_DIR=/rv/cnaccache
export CCACHE_BASEDIR=/rv
tools/materialize_cna_next.sh
cmake --preset web-webgl2
cmake --build --preset web-webgl2
```

The first configure builds CNA's pinned SDL, SDL_image, and SDL_mixer for Emscripten with a hard
two-job ceiling. Later configures reuse their content-fingerprinted cache. The game build also uses
two jobs because it compiles Sharp Runtime.

The deployable files are written to `build-probe/`:

- `island-simulator.html`
- `island-simulator.js`
- `island-simulator.wasm`

There is intentionally no Emscripten `.data` or virtual-filesystem payload. All current runtime
geometry, shaders, small textures, animation, font, and PCM are generated from compiled project
code; the original Android launcher vector is not part of the Web product. The schema-2 asset
manifest rejects undeclared source assets. If a future file-backed runtime asset is introduced, it
must add explicit Web packaging and an actionable load-failure diagnostic rather than relying on a
developer's local filesystem.

For a release host, create an integrity-checked package containing both the original files and
deterministic gzip variants:

```bash
tools/package_web_release.sh build-probe
```

The package is written to `build-probe/web-release/`. Its `manifest.json` records the MIME type,
uncompressed size, SHA-256 digest, and gzip size of every artifact. The current package is
9,068,164 bytes uncompressed and 2,582,366 bytes through gzip, a 71.5% transfer reduction. HTML is
2,058 bytes (1,144 gzip), JavaScript is 264,591 bytes (59,260 gzip), and WebAssembly is 8,801,515
bytes (2,521,962 gzip). These
are payload sizes, not browser decoded-memory or cold-load timing measurements. The validation
test round-trips every compressed artifact, verifies the hashes and manifest totals, rejects a
missing input before creating partial output, and proves that repeated packaging is byte-stable:

```bash
ctest --test-dir build -R '^IslandWebPackageValidation$' --output-on-failure
```

A production host should serve the matching `.gz` file when the request accepts gzip and add
`Content-Encoding: gzip` plus `Vary: Accept-Encoding`. Preserve `text/html; charset=utf-8` for
HTML, `text/javascript; charset=utf-8` for JavaScript, and `application/wasm` for WebAssembly.
Never return a `.gz` file without its content-encoding header; clients without gzip support should
receive the uncompressed file. The following development server deliberately serves the original
files and does not qualify compressed production delivery.

Serve the directory over HTTP; browsers do not support this package through `file://`:

```bash
python3 -m http.server 8765 --bind 127.0.0.1 --directory build-probe
```

Then open `http://127.0.0.1:8765/island-simulator.html`.

The deployable HTML is produced from `tools/web_shell.html`. It supplies a 1280 by 720 render
surface, preserves 16:9 while fitting different browser windows, displays WASD/arrow movement and
the persistent Shift walk/run toggle alongside the current diagnostic/audio controls,
and requests pointer lock within the first mouse gesture instead of requiring a confusing second
click. The C++ controller detects pointer-lock loss and permits clean reacquisition. Web Escape
never terminates the game loop, including on browsers that forward the key after releasing the
lock; the tab remains under browser ownership.

## Automated browser smoke

With the Emscripten environment active, run:

```bash
"${EMSDK_NODE}" tools/run_web_smoke.mjs build-probe
```

The test uses headless Chrome with SwiftShader to create a real WebGL 2 context. It first runs the
game with `--smoke-test`, requires finite timings for all first-draw startup phases, and requires a
clean exit after four frames. It also requires the portable
terrain-material, sky, and ocean shaders and the hardware-instanced palm, grass, jungle, and
natural-prop shaders rather than their fallbacks. Lifecycle, overview, beach, near-surface-water,
and offshore-current stages additionally require the distance-faded reused crossing-wave colour
plus reused-slope shallow-ridge, sunward-crest, and grazing-reflection diagnostics. The harness
then opens live fixed
overview, first-person
beach, grass interaction, folded-split-ground-plant, sparse fallen-leaf detail, segmented-palm and
bark/coconut detail, jungle,
east clearing, west clearing, prop, driftwood, reef, near-surface water, offshore-current,
fallen-frond, distant-horizon, bird, fish, shark, monkey, 20 m shipwreck, beach-crab, butterfly,
and sea-turtle
instances
at a 1280 x 720 viewport.
Every scene asserts the
layered-horizontal-wisp fixed-sky shader diagnostic.
The
captures are saved as `build-probe/island-browser-smoke.png`,
`build-probe/island-browser-beach.png`, `build-probe/island-browser-jungle.png`,
`build-probe/island-browser-grass.png`,
`build-probe/island-browser-ground.png`,
`build-probe/island-browser-litter.png`,
`build-probe/island-browser-palms.png`,
`build-probe/island-browser-east-clearing.png`,
`build-probe/island-browser-west-clearing.png`,
`build-probe/island-browser-props.png`, `build-probe/island-browser-driftwood.png`,
`build-probe/island-browser-reef.png`, `build-probe/island-browser-water.png`,
`build-probe/island-browser-current.png`, `build-probe/island-browser-fronds.png`,
`build-probe/island-browser-distant.png`,
`build-probe/island-browser-birds.png`, `build-probe/island-browser-fish.png`,
`build-probe/island-browser-shark.png`, `build-probe/island-browser-monkeys.png`,
`build-probe/island-browser-shipwreck.png`, `build-probe/island-browser-crabs.png`,
`build-probe/island-browser-butterflies.png`, `build-probe/island-browser-turtles.png`, and
`build-probe/island-browser-run-mode.png`. The beach
view must report nonzero grass; the dedicated grass view must report full smoothed player
interaction on the custom shader, and both views require its curved two-sided daylight path. The
grass view also requires the per-instance blade-stature and near juvenile-underlayer diagnostic,
preventing the close tier from silently reverting to the more repetitive 28-blade cluster. The
same stage requires the sparse near seed-head diagnostic, which proves that the restrained
seed-bearing stems use the portable shared-mesh path. Run and walk speed sampling polls observed
movement until the strict 7.0/3.5 m/s thresholds are reached, avoiding a fixed-duration
SwiftShader scheduling assumption while preserving the speed and persistent-mode checks. The
production input stage then disables camera motion, waits for the settings write, closes that
runtime, and opens a second instance on the same origin and browser profile. The second instance
must report the stored disabled state, proving an actual IDBFS-backed cross-runtime round trip
rather than same-process state retention. Fixed-view and short-run QA pages pass
`--no-user-settings`, so scene validation cannot overwrite player preferences. The grass view
also requires nonzero near, middle, and far populations in at least one mixed cell,
proving that WebGL 2 honours the nonzero instance-buffer offsets used by the adjacent-tier
62--78 m / 86--102 m LOD splits. The
dedicated ground view requires the upright/split-leaf diagnostic, exact variant-2 QA target, and a
nonzero per-instance near-LOD forest-floor population. The
palm view requires the fuller segmented-leaflet, bark/coconut, and transform-derived frond-tint
diagnostics plus nonzero near/far populations. The jungle
view requires the branch-supported canopy, sparse hanging leafy lianas, procedural-bark, and
pointed-leaf near-bush diagnostics,
nonzero near/far
canopy populations, understory, low bushes, and forest-floor plants. The prop view must report
visible fallen logs, restrained radial end-grain cracks, plus a nonzero one-draw contact-shadow
batch. The driftwood view frames the
authored cove group and must report its exact three-piece landmark diagnostic, the three-limb
silhouette diagnostic, visible beach wood plus shell/pebble/dry-wrack clusters, and a nonzero
per-instance near-LOD population. The reef view must report a visible emergent outcrop plus the
near-rock fixed-sea-level wet-band, irregular-crown/facet-light, and transform-derived stone-tint
diagnostics. The water view must
report a nonzero generated
depth/scatter/edge cue, exactly one overlay draw, and a
nonzero player ripple plus a nonzero finite shoreline-transition burst integrated into the existing
ocean submission, the
current view must additionally report nonzero normalized current strength and undertow-audio
target, the frond view must report visible grove-associated fallen fronds and require their curved
asymmetric-leaflet diagnostic, the distant view must report all five unreachable
islets in exactly one portable batch, and the bird/fish views must report nonzero
visible populations from the 24-agent flock and 48-agent school population. The bird view also
requires the layered near trailing-flight-feather diagnostic. The fish view also
requires the exact 51/12-vertex, single-surface-fin, CPU-faceted daylight diagnostic and the shared
depth/sun-band lighting diagnostic. The shark view must report the active offshore patrol and the
same lighting diagnostic, and the monkey view must report a visible member of the
seven-agent GPU-skinned troop. The shipwreck view requires the exact 20 m hull, twelve-piece debris
trail, portable aged-wood/waterline shader, and one visible draw; the crab view requires a nonzero population from all thirty agents in
one draw. The butterfly view requires a nonzero population from all eighteen agents, nonzero near
hinged-wing detail, and one draw. The turtle view requires the five-agent near-shore route marker,
a visible near turtle, and one draw. The
monkey startup diagnostics must also prove that the local-pose crossfade path and both near-only
silhouette details are active. The
first-person runs enable the runtime-generated debug font and overlay, covering the WebGL 2
`SpriteBatch` composition path; the input-stage image records the short normal-play confirmation.
The machine-readable result is `build-probe/island-browser-smoke.json`.

The 2026-10-06 startup-instrumentation run reported 134.8 ms deterministic world generation,
246.8 ms content creation, 15.7 ms game-owned first draw, and 438.0 ms total in SwiftShader. This
qualifies the portable timing path, not hardware-browser cold-load performance or GPU completion.

The harness then opens a live controllable instance and uses Chromium's input protocol to click
the canvas, assert pointer lock/focus, initialization of all four looping audio layers, preparation
of both wave variants, both shoreline-transition variants, all four sand/soil footsteps, and positional
bird/monkey calls. It also requires the slow non-amplifying ambient-gain path after the bounded
sequential source upload, then holds W while applying repeated pointer movements. The same
diagnostic segment
must show both player displacement and changing yaw. It resets to the deterministic south-beach QA
location, proves that Right moves toward visible screen-right, proves that Shift+Up exceeds the
walking ceiling after Shift has been released, proves that a second Shift press restores walking,
captures the first toggle's transient HUD, requires an actual land-footstep playback, and then
walks backward into the ocean. The shoreline crossing must produce a successful water-transition
one-shot and then enter the authoritative swimming state. The harness then enables the portable F2
world line view, toggles F1, resizes to a
non-16:9 browser window, sends a real Escape key, proves that pointer lock is gone while new game
frames continue without exit/error, waits through Chromium's immediate-relock security cooldown,
and clicks to reacquire. It records the initial/final player and viewport samples plus the Escape
state in the JSON report. Chromium's synthetic page-freeze command did not yield reliable resume
evidence in this environment, so the Chrome result does not infer tab recovery. The independent
Firefox gate below now qualifies one short actual active-tab switch through WebDriver BiDi.

Discrete input checks wait for the corresponding authoritative game diagnostic and retry a bounded
number of times. The backward shoreline traversal likewise remains held until the real
walking-to-swimming transition is reported, up to a bounded timeout; the preceding water accent
remains independently required. This keeps SwiftShader frame-time spikes from turning
brief DevTools key edges into false failures while still requiring the game to observe every tested
control and state transition.

This smoke test proves browser/WebAssembly/WebGL execution, bounded one-source-at-a-time staging,
first-gesture CNA mixer/track initialization, activation of the slow ambient-gain path, acceptance
of the shoreline transition one-shot, and a resolved transition into swimming,
but it is not a substitute for
a visible interactive listening pass using hardware acceleration. Do not infer audible quality,
long tab suspension, frame pacing, thermal behaviour, or compatibility beyond the separately
qualified Firefox subset from it.

## Firefox ESR second-engine smoke

Run the narrower production-shell gate with the same Node executable:

```bash
"${EMSDK_NODE}" tools/run_firefox_web_smoke.mjs build-probe
```

The Linux tool starts a local-only HTTP server and a clean Firefox ESR profile, caps Firefox to CPU
cores 0--7 with `taskset`, and talks directly to Firefox's built-in WebDriver BiDi endpoint. No
geckodriver, browser extension, npm package, or network service is required. Override the browser
binary with `ISLAND_FIREFOX` when the installation is named `firefox` rather than `firefox-esr`.

The gate requires the production page to create WebGL 2, emit the current renderer/viewport,
first-draw, clearing-material, ocean-material, and visible-scene diagnostics, and render a 1280 by
720 frame. A trusted click must acquire pointer lock and initialize the complete 44.1 kHz audio
resource path. The test then requires one released Shift press to select persistent 8.0 m/s running,
forward motion to play a real distance-scheduled footstep, a second Shift press to restore 4.3 m/s
walking, and Escape to release pointer lock. A 1000 by 600 resize must retain an undistorted 16:9
canvas; after Firefox's security cooldown, a delayed trusted click must reacquire pointer lock and
a second Escape must release it again. The gate installs a document-visibility/animation probe,
activates a real second tab, requires the game tab to report `hidden`, reactivates it, requires
`visible` plus resumed animation frames, and proves resumed game input with another complete pair
of persistent Shift edges. It then loads the test-only fixed jungle and water cameras,
requires nonzero near/far populations across all four jungle layers plus an active water-view/
ripple/splash state. It also loads the fixed shipwreck, crab, butterfly, and turtle cameras and
requires their exact live population/draw diagnostics. The tool writes eight PNGs plus a schema-1 JSON
report under the ignored build directory and removes its profile and processes even on failure.

The current Firefox ESR 140.15 run reports `WebGL 2.0` on a `Radeon R9 200 Series, or similar`
backend and completed the first rendered frame in 289--401 ms total across fifteen clean tool runs.
All four original-resolution frames were inspected; the input frame visibly retained the world
and the warm-gold `RUN MODE  8.0 M/S` notice, while the jungle and water frames match the Chromium
composition and expose their expected live state. These one-run startup values are not a network
cold-load or frame-time benchmark. The gate does not qualify the remaining Chrome scene matrix,
simultaneous mouse look, clipboard behaviour, fullscreen, extended tab suspension/audio focus, or
audible sound quality.
