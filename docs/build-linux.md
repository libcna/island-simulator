# Linux Build and Validation

## Prerequisites

- CMake 3.23 or newer
- Ninja
- a C++23 compiler
- Python 3.10 or newer for the asset-manifest validator
- ccache
- `xdotool` for the optional synthetic free-camera input test
- Xvfb (`xvfb-run`) for that test when the desktop session is Wayland
- system SDL 3, SDL_image 3, and SDL_mixer 3 development packages
- read-only sibling checkouts of Sharp Runtime `next`, EasyGL `develop`, and MetaGL `develop`
- a CNA repository containing the locked `next` commit

The exact qualified revisions are in `dependencies.lock`. Dependency files and branches must not be
changed by this project.

## Configure and build

Use the shared compiler cache and the two-job ceiling required while compiling Sharp Runtime:

```bash
export CCACHE_DIR=/rv/cnaccache
export CCACHE_BASEDIR=/rv
tools/materialize_cna_next.sh
cmake --preset linux-debug
cmake --build --preset linux-debug
```

The preset deliberately builds only the game, core tests, and the two host report targets. CNA is
added with `EXCLUDE_FROM_ALL`, so a bare all-target build does not pull unrelated framework tools
into normal game iteration; explicit project targets remain preferable in automation.

The preset also builds the renderer-independent `island_world_report` and `island_audio_report`
host tools. Generate machine-readable deterministic content and audio inventories with:

```bash
build/island-world-report --output /tmp/island-world-report.json
build/island-audio-report --output /tmp/island-audio-report.json
python3 tools/validate_assets.py --output /tmp/island-asset-report.json
```

All three commands are registered as native CTests. See `docs/asset-pipeline.md` for content-report
semantics and `docs/audio-review.md` for source measurements and optional WAV preview export.

For a clean host-only memory and undefined-behaviour pass over all first-party core code and both
report tools, use the dedicated preset:

```bash
cmake --preset linux-sanitize
cmake --build --preset linux-sanitize
ctest --preset linux-sanitize
```

This configuration enables AddressSanitizer and UndefinedBehaviorSanitizer only on
island-simulator-owned targets, builds with at most two workers, and deliberately leaves the CNA
application/dependency graph out of the build. The test preset stops on the first
undefined-behaviour report. It disables LeakSanitizer because the managed development runner uses
process tracing, under which LSan exits before reporting project results. On an ordinary terminal
without a debugger or tracer, run a separate leak pass with:

```bash
ASAN_OPTIONS=detect_leaks=1:strict_string_checks=1 \
  build-asan/island_core_tests
```

The preset deliberately reuses the workspace-approved `build-asan/` directory. It is reproducible
and may be removed after a completed diagnostic pass when no other process owns it.
The 2026-10-06 requalification against the then-current dependency pins passed all six registered
tests in 73.98 seconds: core, world/audio reports, asset validation, capture-validator tests, and
the independently parsed WAV preview export. No AddressSanitizer or UndefinedBehaviorSanitizer
finding was emitted; the legacy 44 MB nonstandard tree was removed afterward and the preset was
corrected to the approved shared name.

The 2026-10-07 current-source requalification passed all seven registered tests in 50.51 seconds.
It additionally includes deterministic Web-release packaging validation and covers the current
terrain, wildlife, input, soundscape, and natural-prop core code. No AddressSanitizer or
UndefinedBehaviorSanitizer finding was emitted. The 45 MB `build-asan/` tree was removed after the
pass because it is fully reproducible from the preset.

The 2026-10-07 `9fbe34468c69` CNA `next` refresh compiles the selected OpenGL 3.3 framework and
game objects, and all six non-display gates pass. After the operator closed the preceding play
session, the complete current executable linked successfully and a live-desktop four-frame
overview smoke selected CNA `OPENGL33`, compiled every current scene shader, reported a 1280 by 720
logical backbuffer, and exited cleanly. The full 33-capture/input/benchmark display suite remains a
separate private-display qualification rather than being inferred from that short smoke.

To use a separate read-only CNA `next` checkout instead of the generated export:

```bash
cmake --preset linux-debug -DCNA_ROOT_DIR=/absolute/path/to/cna-next
```

CMake fails if a Git dependency is on the wrong branch or differs from `dependencies.lock`.
`ISLAND_STRICT_DEPENDENCY_REVISIONS=OFF` exists for deliberate update investigation only; do not
use it for release evidence.

## Tests

Core tests require no display:

```bash
ctest --preset linux-core
```

The core suite includes deterministic controller round trips across the deepest cove, furthest
headland, and four additional shore transects, plus an automatically selected steep-bank approach
and a real generated land-rock collision route. These scenarios exercise the authoritative terrain
and layout rather than maintaining a second test-only world description.

`IslandCommandLineValidation` is also display-independent. It starts the real executable for eight
invalid invocations and requires exit code 2, an actionable stderr diagnostic, and no stdout before
graphics initialization. The cases cover an unknown option, missing option values, an unknown
viewpoint, a benchmark/debug conflict, and traversal without a benchmark destination.

GPU tests must not run on the owner's live desktop. In this workspace use CNA's private Weston and
Xwayland runner:

```bash
../cna/tools/platform/run_gpu_tests_private.sh \
  build --output-on-failure -j2
```

`IslandGameSmoke` requires the layered-horizontal-wisp sky diagnostic in addition to renderer
startup and a clean four-frame exit.

`IslandGameStartupReport` performs an actual overview draw and writes
`build/island-startup-report.json`; the dependent `IslandGameStartupReportValidation` checks its
renderer/viewpoint/resolution identity, smoke-mode audio state, finite nonnegative phase timings,
and coherent total. Generate the same schema manually with:

```bash
build/island-simulator --smoke-test --overview \
  --startup-report /tmp/island-startup.json
```

Run this through the private GPU runner when it must not open on the live desktop. The first-draw
phase ends when game-owned draw submission returns; CNA presents afterward.

`IslandGameRelocatableWorkingDirectory` runs the same real graphics startup from the empty
`build/isolated-runtime-cwd/` directory. It guards the current asset model: runtime shaders are
embedded and meshes, textures, and audio are generated, so launching the executable does not
depend on the repository being the working directory. This test must be extended when an external
runtime asset is introduced rather than weakening the working-directory contract.

`IslandGameCapture` writes the fixed overview to `build/island-capture.ppm`;
`IslandGameCaptureValidation` then checks its exact 1280 by 720 P6 payload, luminance spread,
quantized colour diversity, and coarse spatial contrast. It writes
`build/island-capture-report.json`; this catches empty, truncated, flat, overwhelmingly dark, and
overwhelmingly bright output without pretending to be a pixel-golden comparison. The parser and
failure thresholds have separate `IslandCaptureValidatorTests` coverage.
`IslandGameBeachCapture` writes a first-person path view to `build/island-beach-capture.ppm` and
requires both the authored primary-footpath material and its narrow irregular-wear path.
`IslandGameJungleCapture` writes a side-on trail view to `build/island-jungle-capture.ppm` and
requires the folded pointed-leaf near-bush path; the same startup also reports the
branch-supported/pointed-leaf near-canopy path and distance-faded procedural bark fissures.
`IslandGameEastClearingCapture` and `IslandGameWestClearingCapture` write player-eye views from the
two authoritative clearing centres. The east view requires their shared distinct-material shader
path, while the west view requires the shared five-rock/two-log landmark-layout diagnostic.
`IslandGameGroundPlantCapture` writes a cleared first-person folded split-leaf detail to
`build/island-ground-capture.ppm` and requires a nonzero per-instance near-LOD population.
`IslandGameGrassCapture` writes a close first-person grass-interaction view to
`build/island-grass-capture.ppm` and requires full movement strength on the custom instanced shader.
The grass startup also reports the five-level near curvature, portable two-sided daylight path, and
coherent coast/path/clearing biome-colour field. Sparse deterministic sun-cured tips add restrained
blade-scale age variation baked into the shared mesh vertex colours, without another texture,
instance attribute, or draw. The near tier additionally carries a sparse juvenile underlayer, and
one existing instance value produces deterministic per-blade stature and small colour/wind
variation so neighbouring clusters do not repeat the shared mesh literally. The dedicated view
must also contain nonzero near,
middle, and far populations plus a mixed cell from the adjacent-tier 62--78 m / 86--102 m LOD
transitions.
`IslandGamePalmCapture` writes an isolated near/far crown-composition view to
`build/island-palm-capture.ppm` and requires the fuller segmented near-leaflet plus procedural
bark-scar/coconut path.
`IslandGamePropCapture` writes a close fallen-log view to `build/island-prop-capture.ppm` and
requires a nonzero one-draw terrain-contact-shadow batch.
`IslandGameDriftwoodCapture` writes a close beach-composition view to
`build/island-driftwood-capture.ppm`.
`IslandGameReefCapture` writes a close emergent shallow-water outcrop view to
`build/island-reef-capture.ppm`; the current material should show a restrained dark/cool band at
the mean water contact while leaving the upper rock dry.
`IslandGameWaterViewCapture` writes a near-surface swimming view to
`build/island-water-capture.ppm` and requires the procedural depth/scatter/edge cue's nonzero
intensity and
single conditional draw plus a nonzero movement-driven player ripple and shoreline-transition
burst in the ocean shader. Ocean startup also reports the reused crossing-wave subsurface-colour
path.
`IslandGameCurrentViewCapture` writes a 62 m offshore view to
`build/island-current-capture.ppm` and requires nonzero current-modulated veil feedback plus a
nonzero renderer-independent undertow-audio target. Capture mode intentionally leaves the audio
device disabled; normal runtime playback is qualified separately.
`IslandGameDistantSceneryCapture` writes the south-horizon composition to
`build/island-distant-capture.ppm` and requires five islets in one scenery draw.
`IslandGameFallenFrondCapture` writes a close dry-beach/palm-grove transition view to
`build/island-frond-capture.ppm`.
`IslandGameBirdCapture` writes a close airborne wildlife view to
`build/island-bird-capture.ppm`.
`IslandGameFishCapture` writes a close shallow-water school view to
`build/island-fish-capture.ppm` and requires the exact 51/12-vertex, single-surface-fin, CPU-faceted
daylight path plus the shared depth/sun-band lighting diagnostic.
`IslandGameSharkCapture` writes a close offshore patrol view to
`build/island-shark-capture.ppm` and requires the same water-column lighting diagnostic.
`IslandGameMonkeyCapture` writes a close GPU-skinned jungle wildlife view to
`build/island-monkey-capture.ppm`; it waits 24 frames so startup crossfades settle before capture.
They are reproducible QA outputs and are ignored by Git. Convert or inspect them only during a
visual review.

`IslandGameBenchmark` validates the uncapped 600-sample JSON reporting path at the dense-jungle
view. `IslandGameTraversalBenchmark` validates the 676 m moving rail and its 600 scene-range
samples. They are functional tests, not stable performance thresholds. Run the six-report
measurement suite and interpret its limitations as documented in `docs/performance.md`.

When `xdotool` is available, Debug configurations also register `IslandGameFreeCameraInput`. It
activates F3, supplies horizontal/vertical/Shift-fast flight input, observes the active camera
diagnostic, rejects any accidental player run-mode toggle from that debug-only Shift use, restores
player control, and exits. On a native X11 session it uses the selected private display.
On Wayland it automatically nests itself in Xvfb because Xwayland correctly prevents an unattended
process from stealing the real user's keyboard focus; this remains a real SDL X11/XTest input path,
not a Wayland-native qualification.

`IslandGamePlayerInput` separately uses real SDL/XTest input to require keyboard-edge native mouse
capture, simultaneous W plus relative mouse turning, correct visible-right motion from both D and
Right, a released Shift press retaining the 8.0 m/s run mode, a second Shift press restoring the
4.3 m/s walk mode, and two distinct Escape edges for release and exit. The deterministic core suite
independently locks the camera-basis signs and 4.3/8.0 m/s land pace limits.

## Run interactively

```bash
build/island-simulator
```

For repeatable frame-time reports on a private accelerated display:

```bash
../cna/tools/platform/run_gpu_tests_private.sh \
  --exec ./tools/run_native_benchmarks.sh /tmp/island-benchmarks
```

The executable dynamically loads `build/CNA/libcna.so` through its generated build-tree runtime
path. Packaging rules will be added before a release build is claimed reproducible.

## Troubleshooting

- A shared-ccache write failure inside a restricted sandbox is an execution-environment permission
  issue; do not create a second cache. Run the build with permission to use `/rv/cnaccache`.
- A missing CNA tree means `tools/materialize_cna_next.sh` has not been run or `CNA_ROOT_DIR` is
  wrong.
- A dependency revision/branch failure is intentional. Update the baseline only after reviewing
  active `BUGS.md` workarounds and requalifying targets.
- The first configure disables unused network, video, Draco, source-font, and CNB compression
  components. This avoids unrelated optional dependencies; it does not remove runtime audio,
  glTF, skeletal animation, or the required renderers.
