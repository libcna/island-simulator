# Linux Build and Validation

## Prerequisites

- CMake 3.23 or newer
- Ninja
- a C++23 compiler
- ccache
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

The preset deliberately builds only the game and core-test targets. CNA is added with
`EXCLUDE_FROM_ALL`, so a bare all-target build does not pull unrelated framework tools into normal
game iteration; explicit project targets remain preferable in automation.

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

GPU tests must not run on the owner's live desktop. In this workspace use CNA's private Weston and
Xwayland runner:

```bash
../cna/tools/platform/run_gpu_tests_private.sh \
  build --output-on-failure -j2
```

`IslandGameCapture` writes the fixed overview to `build/island-capture.ppm`;
`IslandGameBeachCapture` writes a first-person path view to `build/island-beach-capture.ppm`.
`IslandGameJungleCapture` writes a side-on trail view to `build/island-jungle-capture.ppm`.
`IslandGamePropCapture` writes a close fallen-log view to `build/island-prop-capture.ppm` and
requires a nonzero one-draw terrain-contact-shadow batch.
`IslandGameDriftwoodCapture` writes a close beach-composition view to
`build/island-driftwood-capture.ppm`.
`IslandGameReefCapture` writes a close emergent shallow-water outcrop view to
`build/island-reef-capture.ppm`.
`IslandGameWaterViewCapture` writes a near-surface swimming view to
`build/island-water-capture.ppm` and requires the procedural edge veil's nonzero intensity and
single conditional draw plus a nonzero movement-driven player ripple in the ocean shader.
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
`build/island-fish-capture.ppm`.
`IslandGameSharkCapture` writes a close offshore patrol view to
`build/island-shark-capture.ppm`.
`IslandGameMonkeyCapture` writes a close GPU-skinned jungle wildlife view to
`build/island-monkey-capture.ppm`; it waits 24 frames so startup crossfades settle before capture.
They are reproducible QA outputs and are ignored by Git. Convert or inspect them only during a
visual review.

`IslandGameBenchmark` validates the uncapped 600-sample JSON reporting path at the dense-jungle
view. It is a functional test, not a stable performance threshold. Run the five-view measurement
suite and interpret its limitations as documented in `docs/performance.md`.

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
