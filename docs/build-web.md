# WebGL 2 Build and Browser Validation

## Current status

The terrain/traversal/jungle scene builds with Emscripten and runs in Chromium using CNA's `WEBGL2`
renderer. Automated evidence covers renderer initialization, clean short-run lifecycle, live
overview/beach/grass/jungle/prop/driftwood/reef/water/current/distant rendering, first-click pointer lock,
canvas focus, WASD traversal, relative mouse look, the B camera-motion opt-out, F1 input, pointer
release/reacquisition, and responsive 16:9 presentation. F2 world-line rendering is also toggled
through browser input, and F4 must execute its explicit confirmed/unavailable clipboard path. The
first click opens
CNA's 44.1 kHz mixer, starts all four procedural loops,
and prepares both wave/splash pairs plus the pooled positional bird/monkey call resources. The F2
path includes the same nine terrain-contact samples/normals, attempted/resolved movement trace,
fish-school/shark/monkey navigation guides, three bird route-family volumes, all 24 bird spawn
headings, nearby live bird radial targets, six terrain-following ambient-zone contours, and
positional-source markers.
Audible output on a visible hardware browser, genuine tab background/foreground recovery,
hardware-GPU performance, extended play, and a second browser engine are not yet validated.
The F1 panel exposes the same authoritative seed rows as desktop. F4 attempts to copy their stable
text report, but browser clipboard permission and user-gesture policy can prevent immediate
readback; the panel then reports `UNAVAILABLE` instead of claiming success.

The renderer gate also requires the exact shore-following animated foam ribbon rather than its
stock-effect fallback. The close prop camera additionally requires a nonzero terrain-conforming
contact-shadow population in exactly one portable draw. The beach and grass cameras require the
five-level near-blade curvature and portable two-sided daylight path rather than only accepting the
older flat-colour shader. The lifecycle plus close grass and jungle cameras require the near-field
terrain microvariation and procedural leaf-litter path. The lifecycle and dedicated jungle camera
also require derivative-based, two-sided fixed-sun facet lighting on the instanced vegetation
shader rather than accepting its unlit fallback. The lifecycle, beach approach, and jungle camera
also require the authored primary-footpath terrain material diagnostic. Dedicated east/west
clearing cameras require their shared authored-material diagnostic and exact QA-camera identity.
Both clearing cameras also require the shared five-rock/one-log authored-landmark diagnostic.

The latest automated run used Chrome 152.0.7977.82 with SwiftShader and confirmed exact F4
clipboard readback on its focused localhost canvas. All 22 reported lifecycle, deployment,
renderer, scene, and input stages passed. This records the evidence environment; it is not a
minimum-version or general browser-permission claim.

## Prerequisites

- the Emscripten SDK at `/home/robertvokac/emsdk`, or another installation exposed through `EMSDK`;
- Google Chrome or Chromium for the automated browser smoke test;
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

Serve the directory over HTTP; browsers do not support this package through `file://`:

```bash
python3 -m http.server 8765 --bind 127.0.0.1 --directory build-probe
```

Then open `http://127.0.0.1:8765/island-simulator.html`.

The deployable HTML is produced from `tools/web_shell.html`. It supplies a 1280 by 720 render
surface, preserves 16:9 while fitting different browser windows, displays the current controls,
and requests pointer lock within the first mouse gesture instead of requiring a confusing second
click. The C++ controller detects pointer-lock loss and permits clean reacquisition.

## Automated browser smoke

With the Emscripten environment active, run:

```bash
"${EMSDK_NODE}" tools/run_web_smoke.mjs build-probe
```

The test uses headless Chrome with SwiftShader to create a real WebGL 2 context. It first runs the
game with `--smoke-test`, requires finite timings for all first-draw startup phases, and requires a
clean exit after four frames. It also requires the portable
terrain-material, sky, and ocean shaders and the hardware-instanced palm, grass, jungle, and
natural-prop shaders rather than their fallbacks. It then opens live fixed overview, first-person
beach, grass interaction, folded-split-ground-plant, segmented-palm and bark/coconut detail, jungle,
east clearing, west clearing, prop, driftwood, reef, near-surface water, offshore-current,
fallen-frond, distant-horizon, bird, fish, shark, and monkey instances at a 1280 x 720 viewport.
Every scene asserts the
layered-horizontal-wisp fixed-sky shader diagnostic.
The
captures are saved as `build-probe/island-browser-smoke.png`,
`build-probe/island-browser-beach.png`, `build-probe/island-browser-jungle.png`,
`build-probe/island-browser-grass.png`,
`build-probe/island-browser-ground.png`,
`build-probe/island-browser-palms.png`,
`build-probe/island-browser-east-clearing.png`,
`build-probe/island-browser-west-clearing.png`,
`build-probe/island-browser-props.png`, `build-probe/island-browser-driftwood.png`,
`build-probe/island-browser-reef.png`, `build-probe/island-browser-water.png`,
`build-probe/island-browser-current.png`, `build-probe/island-browser-fronds.png`,
`build-probe/island-browser-distant.png`,
`build-probe/island-browser-birds.png`, `build-probe/island-browser-fish.png`,
`build-probe/island-browser-shark.png`, and `build-probe/island-browser-monkeys.png`. The beach
view must report nonzero grass; the dedicated grass view must report full smoothed player
interaction on the custom shader, and both views require its curved two-sided daylight path. The
dedicated ground view requires the upright/split-leaf diagnostic, exact variant-2 QA target, and a
nonzero per-instance near-LOD forest-floor population. The
palm view requires the segmented-leaflet and bark/coconut diagnostics plus nonzero near/far
populations. The jungle
view requires the branch-supported canopy and pointed-leaf near-bush diagnostics, nonzero near/far
canopy populations, understory, low bushes, and forest-floor plants. The prop view must report visible fallen logs plus
a nonzero one-draw contact-shadow batch, the driftwood view must
report visible beach wood plus shell/pebble clusters, the reef view must report a visible emergent
outcrop, the water view must report a nonzero procedural veil, exactly one overlay draw, and a
nonzero player ripple integrated into the existing ocean submission, the
current view must additionally report nonzero normalized current strength and undertow-audio
target, the frond view must
report visible grove-associated fallen fronds, the distant view must report all five unreachable
islets in exactly one portable batch, and the bird/fish views must report nonzero
visible populations from the 24-agent flock and 48-agent school population, the shark view must
report the active offshore patrol, and the monkey view must report a visible member of the
seven-agent GPU-skinned troop. The
monkey startup diagnostics must also prove that the local-pose crossfade path is active. The
first-person runs enable the runtime-generated debug font and overlay, covering the WebGL 2
`SpriteBatch` composition path.
The machine-readable result is `build-probe/island-browser-smoke.json`.

The 2026-10-06 startup-instrumentation run reported 134.8 ms deterministic world generation,
246.8 ms content creation, 15.7 ms game-owned first draw, and 438.0 ms total in SwiftShader. This
qualifies the portable timing path, not hardware-browser cold-load performance or GPU completion.

The harness then opens a live controllable instance and uses Chromium's input protocol to click
the canvas, assert pointer lock/focus, initialization of all four looping audio layers, preparation
of both wave and both shoreline-transition variants, and preparation of positional bird/monkey
calls, then hold W
long enough to measure world movement, reset to the deterministic south-beach QA location, and
walk backward into the ocean. The shoreline crossing must produce a successful water-transition
one-shot diagnostic. The harness then moves the relative pointer, enables the portable F2 world
line view, toggles F1, resizes to a
non-16:9 browser window, explicitly unlocks, and clicks to reacquire. It records the initial/final
player and viewport samples in the JSON report. Chromium's
synthetic page-freeze command did not yield reliable resume evidence in this environment, so real
tab background/foreground behaviour remains unqualified rather than being inferred.

This smoke test proves browser/WebAssembly/WebGL execution, first-gesture CNA mixer/track
initialization, and acceptance of the shoreline transition one-shot, but it is not a substitute for
a visible interactive listening pass using hardware acceleration. Do not infer audible quality,
real tab lifecycle, frame pacing, thermal behaviour, or non-Chromium compatibility from it.
