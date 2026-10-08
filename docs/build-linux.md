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

The 2026-10-07 current-source requalification passed all seven registered tests in 78.99 seconds.
It additionally includes deterministic Web-release packaging validation and covers the current
terrain, wildlife, input, soundscape, natural-prop, and persistent-settings core code. No
AddressSanitizer or UndefinedBehaviorSanitizer finding was emitted. The 47 MiB `build-asan/` tree
was removed after the pass because it is fully reproducible from the preset.

The 2026-10-07 `97783d1e090c` CNA `next` refresh compiles and links the selected OpenGL 3.3
framework and game. The complete private-display suite passes 46/46, selects CNA `OPENGL33`,
compiles every current scene shader, exercises all deterministic views, reports a 1280 by 720
logical backbuffer, and includes actual simultaneous movement/mouse input plus persistent Shift
run/walk toggling. Dedicated beach-side shipwreck and crab captures require the exact 20 m broken
hull/12 debris/large-ship detail/one wreck draw, a nonzero thirty-agent crab population in one wildlife draw, and a
nonzero eighteen-agent butterfly population in one clearing-edge wildlife draw, and one near
member of the five-agent sea-turtle population in one aquatic-wildlife draw.

To use a separate read-only CNA `next` checkout instead of the generated export:

```bash
cmake --preset linux-debug -DCNA_ROOT_DIR=/absolute/path/to/cna-next
```

CMake fails if a Git dependency is on the wrong branch or differs from `dependencies.lock`.
`ISLAND_STRICT_DEPENDENCY_REVISIONS=OFF` exists for deliberate update investigation only; do not
use it for release evidence.

## Relocatable release bundle

Configure and build the optimized OpenGL 3.3 target with the same shared cache and two-worker
ceiling:

```bash
export CCACHE_DIR=/rv/cnaccache
export CCACHE_BASEDIR=/rv
cmake --preset linux-release
cmake --build --preset linux-release
tools/package_linux_release.sh build-consumer
```

The packager refuses a non-Release or non-OpenGL 3.3 tree. It installs and strips the executable
and CNA shared runtime into `build-consumer/linux-release/`, sets the executable runpath to
`$ORIGIN/lib`, copies the project attribution and exact dependency pins, and includes the CNA,
Sharp Runtime, EasyGL, and MetaGL licences plus CNA notice. It emits a SHA-256/size manifest and a
deterministic `build-consumer/island-simulator-linux-x86_64.tar.gz`; two unchanged invocations must
produce byte-identical archives.

The current package contains 12,204,004 installed bytes, including the hash-gated 1280 by 720
overview image referenced by its README. Two unchanged invocations produced the same
5,074,648-byte archive with SHA-256
`c14e5ab64b8a6c058ef90acd3fa12a68e2f36d12d60991bb89c134d2b06825a3`.

The release preset deliberately reuses the workspace-approved `build-consumer/` tree. The optional
standalone Android CMake probe uses that same tree, so reconfigure it with the desired preset when
switching between those two qualifications. Normal Android Gradle builds use Gradle's managed
native intermediates instead.

The archive deliberately does not copy arbitrary libraries from the build host. A compatible
Linux system still needs glibc, libstdc++, SDL 3, SDL_mixer 3, and an OpenGL 3.3-capable driver.
After extraction, run:

```bash
./linux-release/island-simulator
```

The 2026-10-07 qualification extracted the final `build-consumer/` archive under `/tmp`, launched
it from that unrelated working directory on a private Weston/Xwayland display, selected CNA
`OPENGL33`, compiled every current scene shader, rendered four overview frames, and exited cleanly.
Its first-draw report was 497.137 ms total. This validates relocation and first rendering, not
compatibility with every Linux distribution or GPU driver.
The current settings-enabled package repeated this extraction/launch gate, required the full Linux
profile, retained the 25,921-vertex/51,200-triangle ocean, and completed first draw in 1058.594 ms
total. This one-run value is functional evidence, not a startup-performance regression claim.
The later shipwreck package repeated the unrelated-working-directory gate with
`--viewpoint shipwreck`, selected `OPENGL33`, rendered four frames with the landmark visible, and
exited cleanly in approximately 400 ms total.
The refined-shipwreck package repeated that exact archive extraction gate after adding the
portable aged-wood/waterline material, curved hull section, missing boards, and splintered debris.
It selected `OPENGL33`, retained one wreck draw with 3,392 triangles, passed 1280 by 720 capture
validation, and completed first draw in 402.776 ms total.
The final documentation-synchronized archive was then launched from a second unrelated directory;
it selected the same shader and one-draw scene and completed first draw in 437.103 ms total.
After the final wet-material selector correction, the exact `c836cce2` archive repeated this gate
and completed first draw in 629.566 ms total. These individual startup values are functional
evidence, not a performance trend.
The later inland-floor package repeated the unrelated-working-directory gate with the jungle
camera. The exact `363f2df1` archive selected `OPENGL33`, required the humus/moss material marker,
captured four frames, and completed first draw in 372.093 ms total. Its 1280 by 720 output passed
capture validation with 165 colour bins, 35.91 luminance deviation, and 185.13 spatial contrast;
the original-resolution frame was inspected.
The subsequent natural-grass package repeated the unrelated-working-directory gate with the close
grass camera. The exact `3cb1d12a` archive selected `OPENGL33`, required the asymmetric/bent-tip
marker, retained 2,827 visible clusters in 21 grass submissions, and completed first draw in
446.534 ms. Its inspected 1280 by 720 frame passed capture validation with 124 colour bins, 19.68
luminance deviation, and 42.29 spatial contrast.
The later large-ship package repeated the gate with the shipwreck camera. The exact `177e3498`
archive selected `OPENGL33`, required the large-ship marker, retained the 3,948-triangle wreck in
one draw, and completed first draw in 408.252 ms. Its inspected 1280 by 720 frame passed capture
validation with 183 colour bins, 36.34 luminance deviation, and 146.84 spatial contrast.
The subsequent palm-detail package repeated the unrelated-working-directory gate with the close
palm camera. The exact `08cbaddf` archive selected `OPENGL33`, required the sparse hanging dry-
frond marker, retained 293 palms in eight submissions, and completed first draw in 321.558 ms. Its
inspected 1280 by 720 frame passed capture validation with 199 colour bins, 34.84 luminance
deviation, and 146.03 spatial contrast.
The following near-canopy package repeated the gate with the jungle camera. The exact `c14e5ab6`
archive selected `OPENGL33`, required the irregular layered lower-crown marker, retained 109 canopy
trees (63 near and 46 far) in the unchanged 28 jungle submissions, and completed first draw in
506.911 ms. Its inspected 1280 by 720 frame passed capture validation with 166 colour bins, 35.88
luminance deviation, and 185.39 spatial contrast.
The subsequent trunk/branch package repeated the unrelated-working-directory gate with the same
jungle camera. Two packaging runs produced the identical 5,075,520-byte archive with SHA-256
`ccbfb6a7ea40189bcbde20d264dd20579232348653715af29ac280b4225f2489` and 12,204,685 installed
bytes. The exact archive selected `OPENGL33`, required the irregular-trunk/bent-support-branch
marker, retained the same 109 canopy trees and 28 jungle submissions, and completed first draw in
298.290 ms. Its inspected 1280 by 720 frame passed capture validation with 165 colour bins, 35.87
luminance deviation, and 185.38 spatial contrast.
The subsequent crab package repeated the gate from `/tmp` with `--viewpoint crabs`, selected
`OPENGL33`, rendered five of thirty agents in one draw, produced an inspected 1280 by 720 frame,
and exited cleanly after 456.854 ms to the first completed game draw.
The butterfly package repeated the unrelated-working-directory gate with `--viewpoint
butterflies`, selected `OPENGL33`, rendered eleven of eighteen agents (seven near, four far) in one
draw, and exited cleanly after an 823.393 ms first-draw total. Its Release frame was inspected.

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

GPU tests must not run on the owner's live desktop. The 2026-10-08 AMDGPU GPUVM incident also means
that a private compositor is not, by itself, a sufficient safety boundary: it still uses the same
physical GPU as GNOME. Until the critical incident is closed, do not invoke the aggregate CNA
runner directly. Obtain read-only kernel-journal access and qualify one workload at a time with:

```bash
tools/run_gpu_incident_qualification.sh build smoke
tools/run_gpu_incident_qualification.sh build capture-jungle
tools/run_gpu_incident_qualification.sh build benchmark-jungle
```

The harness refuses to launch unless it can read a real kernel record and acquire a journal cursor.
It preserves the command output, per-PID runtime log, private-compositor log, benchmark/capture
output, and post-run kernel delta. Any new GPUVM fault, GFX ring timeout, VRAM loss, or reset stops
qualification immediately. See `docs/incidents/2026-10-08-amdgpu-gpuvm-reset.md` for the exact
sequence and acceptance criteria.

Ordinary CTest display tests now share the `island_gpu_display` resource lock even under
`ctest -j8`; independent CPU-only tests can still run in parallel. The
`ISLAND_ALLOW_PARALLEL_GPU_TESTS=ON` CMake option exists only for the final controlled incident
stress stage after every serial gate is clean. It is not a normal developer setting.

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
requires sparse hanging leafy lianas on selected near-canopy variants; the same startup also
reports the folded pointed-leaf near-bush path,
branch-supported/pointed-leaf near-canopy path and distance-faded procedural bark fissures.
`IslandGameEastClearingCapture` and `IslandGameWestClearingCapture` write player-eye views from the
two authoritative clearing centres. The east view requires their shared reused-noise broad-patch
shader path and visibly warmer sun-dry floor identity, while the west view requires the shared
five-rock/two-log landmark-layout diagnostic.
`IslandGameGroundPlantCapture` writes a cleared first-person folded split-leaf detail to
`build/island-ground-capture.ppm` and requires a nonzero per-instance near-LOD population.
`IslandGameGrassCapture` writes a close first-person grass-interaction view to
`build/island-grass-capture.ppm` and requires full movement strength on the custom instanced shader.
The grass startup also reports the five-level near curvature, portable two-sided daylight path,
selected one-third near-blade geometric V-fold ridge, narrower asymmetric edges with occasional
bent tips, and coherent coast/path/clearing biome-colour field. Sparse deterministic sun-cured tips add restrained
blade-scale age variation baked into the shared mesh vertex colours, without another texture,
instance attribute, or draw. The near tier additionally carries a sparse juvenile underlayer, and
one existing instance value produces deterministic per-blade stature and small colour/wind
variation so neighbouring clusters do not repeat the shared mesh literally. Six mature near
blades additionally carry restrained seed-bearing stems that share the opaque instanced mesh and
wind path; the capture requires their exact diagnostic. The dedicated view
must also contain nonzero near,
middle, and far populations plus a mixed cell from the adjacent-tier 62--78 m / 86--102 m LOD
transitions.
The current natural-silhouette qualification passed all 46 tests on the private accelerated
display. Its Release fixed-grass benchmark retained 2,827 clusters, 21 grass submissions, and 99
total tracked submissions while recording 2.6461 ms mean, 6.3492 ms p95, and 11.7514 ms p99 across
600 intervals; one interval exceeded 16.667 ms and none exceeded 33.333 ms.
`IslandGamePalmCapture` writes an isolated near/far crown-composition view to
`build/island-palm-capture.ppm` and requires the fuller segmented near-leaflet plus procedural
bark-scar/coconut path, fixed-sun facet response, and sparse hanging dry-frond variation.
`IslandGamePropCapture` writes a close fallen-log view to `build/island-prop-capture.ppm` and
requires the restrained radial end-grain crack path. The broader Web prop gate additionally
requires a nonzero one-draw terrain-contact-shadow batch.
`IslandGameDriftwoodCapture` writes a close beach-composition view to
`build/island-driftwood-capture.ppm`; its startup gate requires tapered three-dimensional branches
on every near silhouette rather than accepting the former flat branch fins, plus end caps whose
perimeters exactly match the irregular bent-body rings.
`IslandGameReefCapture` writes a close emergent shallow-water outcrop view to
`build/island-reef-capture.ppm`; the current material should show a restrained dark/cool band at
the mean water contact while leaving the upper rock dry. Startup also reports the irregular near-
crown and derivative fixed-sun facet-light diagnostic.
`IslandGameWaterViewCapture` writes a near-surface swimming view to
`build/island-water-capture.ppm` and requires the procedural depth/scatter/edge cue's nonzero
intensity and
single conditional draw plus a nonzero movement-driven player ripple and shoreline-transition
burst in the ocean shader. Ocean startup also reports the reused crossing-wave subsurface-colour,
shallow-ridge, and sunward-crest paths.
`IslandGameCurrentViewCapture` writes a 62 m offshore view to
`build/island-current-capture.ppm` and requires nonzero current-modulated veil feedback plus a
nonzero renderer-independent undertow-audio target. Capture mode intentionally leaves the audio
device disabled; normal runtime playback is qualified separately.
`IslandGameDistantSceneryCapture` writes the south-horizon composition to
`build/island-distant-capture.ppm` and requires five islets in one scenery draw.
`IslandGameFallenFrondCapture` writes a close dry-beach/palm-grove transition view to
`build/island-frond-capture.ppm` and requires the curved asymmetric-leaflet diagnostic plus a
nonzero visible population.
`IslandGameBirdCapture` writes a close airborne wildlife view to
`build/island-bird-capture.ppm`; startup reports the layered near trailing-flight-feather path.
`IslandGameButterflyCapture` writes a low first-person clearing view to
`build/island-butterfly-capture.ppm` and requires a visible population from all eighteen agents,
nonzero near detail, and exactly one culled wildlife draw.
`IslandGameSeaTurtleCapture` writes a close near-surface-water view to
`build/island-turtle-capture.ppm` and requires the five-agent population marker, one visible near
turtle, and exactly one culled aquatic-wildlife draw.
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

For repeatable frame-time reports during the current GPU incident qualification:

```bash
tools/run_gpu_incident_qualification.sh build benchmark-jungle
tools/run_gpu_incident_qualification.sh build benchmark-traversal
```

The development executable dynamically loads `build/CNA/libcna.so` through its generated
build-tree runtime path. Use the release bundle above for a movable executable with its own pinned
CNA runtime.

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
