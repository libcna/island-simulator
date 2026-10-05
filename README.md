# island-simulator

`island-simulator` is a peaceful first-person tropical island walking and near-shore swimming
experience built in C++23 with CNA. The project is at the beginning of a long development campaign.
The current Linux/OpenGL 3.3 slice renders a deterministic approximately 500 m island, animated
tropical ocean, 330 varied procedural palms with a light breeze and near/far mesh LOD, and 18,556
deterministic mesh-grass clusters with wind and distance LOD. It supports first-person terrain
walking, near-shore swimming, palm-trunk
collision, and a soft ocean-current boundary. The same current rendering slice has also run in an
actual Chromium WebGL 2 context.

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
ctest --test-dir build --output-on-failure -j2
```

Run `build/island-simulator`. Click inside the window to capture the mouse, use WASD to move, and
press Escape to release the mouse; press Escape again to exit. The controller follows the terrain,
rejects overly steep uphill motion, slides along palm-trunk collision proxies, and enters swimming
water with hysteresis. Rock/complex prop collision, geometric step-up, and touch controls are not
implemented yet.

The automated browser smoke test verifies renderer initialization, the portable animated-ocean
shader, hardware-instanced palm wind and near/far LOD, portable instanced grass wind, four frames,
clean shutdown, and active overview/beach screenshots.
Browser pointer-lock/input and audio behaviour still need interactive QA. Android OpenGL ES 3 is
required but is not yet built or runtime-validated. See `PLAN.md` for the evidence required before
broader platform claims.

## Project records

- `PLAN.md` is the living development ledger.
- `BUGS.md` records confirmed CNA or Sharp Runtime defects and game-side workarounds.
- `ASSET_LICENSES.md` records all asset provenance before assets enter a runtime build.
