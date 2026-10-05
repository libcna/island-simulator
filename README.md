# island-simulator

`island-simulator` is a peaceful first-person tropical island walking and near-shore swimming
experience built in C++23 with CNA. The project is at the beginning of a long development campaign.
The current slice renders a deterministic approximately 500 m island, animated tropical ocean,
330 varied procedural palms, 281 broadleaf canopy trees, 1,822 ferns/large-leaf plants, 565 low
bushes, 2,225 small forest-floor plants, and 18,556 mesh-grass clusters. It also composes 152
irregular rocks, 11 mossy fallen logs,
and 28 pieces of sun-bleached beach driftwood. Every visual layer has spatial culling and distance
LOD, while vegetation uses a calm breeze. It supports
first-person terrain walking, near-shore swimming, trunk collision, a soft ocean-current boundary,
dual-zone touch input, and a fixed sunny gradient sky with a sun disc and calm high clouds. The
current audio pass adds original procedural shoreline surf, occasional stronger waves, light
canopy breeze, a subtle daytime-jungle texture, speed/depth-responsive movement through shallow or
swimming water, and restrained water-entry/exit accents without external sound files. The
complete scene runs on Linux/OpenGL 3.3 and in an actual
Chromium/WebGL 2 context. An Android API 35 x86_64 emulator rendered the earlier
terrain/palm/grass scene through OpenGL ES 3; the current jungle/prop/sky-inclusive APK builds, but
its runtime and real-device touch feel are not yet qualified.

There are deliberately no quests, combat, inventory, crafting, survival systems, score,
progression, dynamic weather, or day/night cycle.

## Dependency baseline

The project consumes read-only CNA `next` and Sharp Runtime `next` checkouts. Exact qualified
commits are stored in `dependencies.lock`. In the current workspace CNA's shared checkout is on a
different branch, so the project creates an ignored, unmodified source export of CNA `next`:

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
cmake --build build --target island-simulator island_core_tests --parallel 2
ctest --preset linux-core
```

The graphical tests need an isolated display; use the command in `docs/build-linux.md` rather than
running the full CTest set on the owner's live desktop.

Run `build/island-simulator`. Click inside the window to capture the mouse, use WASD to move, and
press Escape to release the mouse; press Escape again to exit. The controller follows the terrain,
rejects overly steep uphill motion, slides along trunk, solid-rock, and fallen-log proxies, and enters swimming
water with hysteresis. Rock/complex prop collision and geometric step-up are not implemented yet.
Press F1 to toggle the developer overlay with frame timing, player/water state, renderer identity,
culling counts, vegetation draw statistics, ambient mix levels, player-water gain, and wave count.
Press M to mute or unmute the soundscape. While the overlay is visible, F5 through F8 teleport among
four fixed QA locations. The overlay starts hidden in a normal run and can be enabled at launch
with `--debug-overlay` for automated QA.

The automated browser smoke test verifies renderer initialization, the portable animated-ocean
and fixed-sky shaders, hardware-instanced palm/jungle/prop LOD, portable instanced grass wind, four
frames, clean shutdown, active overview/beach/jungle/prop/driftwood screenshots, and portable
overlay text rendering.
The Chromium harness also drives first-click pointer lock and audio start, WASD, relative mouse
look, a deterministic beach-to-water crossing with successful transition-audio playback, F1,
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
