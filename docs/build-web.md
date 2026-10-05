# WebGL 2 Build and Browser Validation

## Current status

The terrain/traversal/jungle scene builds with Emscripten and runs in Chromium using CNA's `WEBGL2`
renderer. Automated evidence covers renderer initialization, clean short-run lifecycle, live
overview/beach/jungle/prop/driftwood rendering, first-click pointer lock, canvas focus, WASD
traversal, relative mouse look, F1 input, pointer release/reacquisition, and responsive 16:9
presentation. The first click also opens CNA's 44.1 kHz mixer and starts both procedural ambient
tracks. Audible output on a visible hardware browser, genuine tab background/foreground recovery,
hardware-GPU performance, extended play, and a second browser engine are not yet validated.

The latest automated run used Chrome 152.0.7977.82 with SwiftShader. This records the evidence
environment; it is not a minimum-version claim.

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
game with `--smoke-test` and requires a clean exit after four frames. It also requires the portable
sky/ocean shaders and the hardware-instanced palm, grass, jungle, and natural-prop shaders rather
than their fallbacks. It then opens live fixed overview, first-person beach, jungle, prop, and
driftwood instances at a 1280 x 720 viewport. Every scene asserts the fixed-sky shader diagnostic.
The
captures are saved as `build-probe/island-browser-smoke.png`,
`build-probe/island-browser-beach.png`, `build-probe/island-browser-jungle.png`, and
`build-probe/island-browser-props.png`, and `build-probe/island-browser-driftwood.png`. The beach
view must report nonzero grass, the jungle view must report nonzero canopy, understory, and low
bushes, the prop view must report visible fallen logs, and the driftwood view must report visible
beach wood. The
first-person runs enable the runtime-generated debug font and overlay, covering the WebGL 2
`SpriteBatch` composition path.
The machine-readable result is `build-probe/island-browser-smoke.json`.

The harness then opens a live controllable instance and uses Chromium's input protocol to click
the canvas, assert pointer lock/focus and the audio-start diagnostic, hold W long enough to measure
world movement, move the
relative pointer, toggle F1, resize to a non-16:9 browser window, explicitly unlock, and click to
reacquire. It records the initial/final player and viewport samples in the JSON report. Chromium's
synthetic page-freeze command did not yield reliable resume evidence in this environment, so real
tab background/foreground behaviour remains unqualified rather than being inferred.

This smoke test proves browser/WebAssembly/WebGL execution and first-gesture CNA mixer/track
initialization, but it is not a substitute for a visible interactive listening pass using hardware
acceleration. Do not infer audible quality, real tab lifecycle, frame pacing, thermal behaviour, or
non-Chromium compatibility from it.
