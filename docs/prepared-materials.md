# Compact material packaging

2026-10-09. This changes packaging and startup work, not rendering features or
the unresolved AMDGPU incident. Original source images and their hashes remain
unchanged. No generated asset is committed under `assets/`.

## Implementation and dependencies

Host Python 3.10+ and Pillow (`python3-pil` on Debian; validated here with Python
3.13.5 / Pillow 11.1.0) are required. No new target/runtime image library is added.
`tools/prepare_material_texture.py` uses Pillow only for PNG decoding/encoding.
It retains RGB code values, rejects alpha, animation, non-RGB8 input, non-square
or oversized images and limits files to 8 MiB. IHDR dimensions are checked before
opening the decoder. Output goes through a same-directory temporary file and
atomic replacement; failure does not truncate an existing usable output. Input
and output cannot be the same resolved path. Generated PNGs are readable files
with no inherited metadata or timestamps.

The area integration, evaluation order and rounding match
`forest_floor::PrepareTexture` in C++. Generic Pillow resize and colour transforms
are deliberately not used. Once base RGB bytes agree, the existing deterministic
runtime mip builder receives the same RGBA8 pixels, so every lower level agrees.
The final texture inventory stays 6,383,412 bytes; shaders, UVs, colour response,
geometry and draw counts are unchanged. This is not GPU-format compression.

Native and Web CMake generate `Content/terrain/forest-floor.png` and
`Content/vegetation/leaf-lamina.png` in the existing binary directory, with source
and script dependencies. Native installation and Emscripten preload use those
outputs. No cross-compiled host executable is needed. Android uses incremental
`prepareIslandFloor` / `prepareIslandLeaf` Exec tasks before its existing asset
Sync, with generated files under the existing app build tree. Set
`-PislandPython=/path/to/python3` for a non-default host interpreter.

Originals remain the attribution/reproduction authority. Rebuild generated PNGs
after intentionally updating the host codec/toolchain; compressed bytes can differ
across codec versions while decoded pixels remain identical. Same-host repeated
conversion is byte-identical and tested. There is no new persistent asset cache.

## Measured outputs

| Material | Original PNG bytes | Prepared size | Prepared PNG bytes | SHA-256 |
|---|---:|---|---:|---|
| Forest floor v1 | 4,428,065 | 512x512 | 650,495 | `79cfe4fc0648c0da3b54ef8fc7db57c17280390db7bc62956e9384ad9d8696e8` |
| Leaf lamina v1 | 3,003,277 | 256x256 | 115,766 | `81ce7b38d179c5065455c8488d029f9a5e48accf6bda8ca297cae49eeb9bed76` |

Combined PNG payload falls from 7,431,342 to 766,261 bytes (6,665,081 fewer bytes,
89.69% reduction). Package-wide compression savings and startup timings require
separate measurements; these numbers do not establish physical-device performance.

## Validation ledger

- Native build passes. Initial 33/33 non-display groups pass in 23.10 s.
- New `IslandPreparedMaterialTests` compares every RGB pixel of both actual
  images against the unchanged C++ runtime filter exposed through the existing
  preview tool's `--reduce` mode. It also tests noninteger reduction, enlargement,
  constant fields, idempotence, deterministic encoding, rejection and source/output
  preservation. CPU-only; no graphics context.
- Final pre-decode guard tests pass (focused three groups, 3.98 s). A second native
  build reports `ninja: no work to do`; no asset regeneration or compiler work.
- Native isolated llvmpipe `20261009T122627.472020Z-ground-broadleaf` passes, both
  load markers and empty kernel deltas. Image inspected against the original-size
  package's `20261009T120122.533676Z-ground-broadleaf`: consistent leaf/floor detail.
  Full images are not pixel-identical (normalized MAE 0.00135078); these animated
  scene captures are not a synchronized pixel-golden test. The independent source
  reduction tests, not this screenshot similarity, establish pixel equivalence.
- In that one native comparison, floor decode/reduce/upload is 39.3618 versus
  175.26 ms, and startup total 2,382.135 versus 2,747.731 ms. These are single-run
  observations on a shared machine, not a controlled performance benchmark.
- Web build and package complete: 10,257,711 raw / 3,496,805 gzip bytes, versus
  16,922,796 / 9,919,804 before reduction. The `.data` file is exactly 766,261 bytes;
  both prepared PNG hashes match native. Initial focused run
  `20261009T122907.762729Z-chrome-litter` passes litter but fails grass overlay F1
  control, with empty kernel deltas. See `browser-input-ownership.md`; no scene or
  graphics assertion was removed. Corrected run
  `20261009T123357.811353Z-chrome-litter` passes both views / 60 intervals each,
  zero GL errors/context loss and empty kernel deltas. Litter image inspected.
  This does not repeat the full input/lifecycle or eight-view vegetation suite.
- Android x86_64 assembly/lint passes (4 s); prepared PNG hashes match native/Web.
  First runtime attempt was refused before emulator launch by the old gate's
  original-source-size assertion. Updated `validate_apk` verifies **both** prepared
  materials, exactly one ZIP entry each, exact size and SHA-256, and records source
  identities in run metadata. Its 15 CPU tests pass, including missing/duplicate,
  wrong-size and same-size-corruption cases for each material. This is a packaging
  contract update, not relaxed runtime acceptance.
- Android API 35 KVM/SwiftShader `20261009T123933.093621Z-ground-broadleaf` passes,
  app PID 1835 / entry TID 1922. Image inspected; empty live/final kernel deltas,
  owned emulator exits zero. Startup total 11,274.129 ms versus 12,245.743 ms in
  the preceding original-source run, again a single uncontrolled observation.
  No touch/lifecycle/traversal or physical-phone performance claim.
- Default ARM64 assembly/lint restored (2 s); three ARM64 libraries and both
  compact assets verified. Preparation and Sync tasks are UP-TO-DATE. ARM64 runtime
  remains untested. CNA's existing GLES platform warning remains unchanged.
- Final 33/33 non-display groups pass in 22.64 s. Owned graphics processes are
  gone; the AMD stop latch is unchanged and 359 GiB remains free. No incident
  evidence or original artwork was removed. Failure logs remain available.

Then-validated rolling APKs (since replaced by the grass-surface pass; incident evidence retained):

| Package | Bytes | SHA-256 |
|---|---:|---|
| `build/android-software/island-x86_64.apk` | 20,345,947 | `4900591f6e7557e4c395b757c93c4b8b954da8a0129dd48d08c0dd750cbaab65` |
| `android/app/build/outputs/apk/debug/app-debug.apk` (ARM64) | 20,437,764 | `db15e05f2a5125c51471518e563d80a8ae3184b3482886462c7d77f9351f4f34` |

Useful commands (shared ccache remains required for any build):

```sh
export CCACHE_DIR=/rv/cnaccache
export CCACHE_BASEDIR=/rv
cmake --build --preset linux-debug -j2
ctest --test-dir build -R 'PreparedMaterial|ForestFloor' --output-on-failure -j2
sg systemd-journal -c 'python3 tools/run_software_capture.py build ground-broadleaf'
```

Missing-material fallback testing, grass appearance and hardware incident
diagnosis remain separate tasks. This preparation is not a bypass of loading
errors or a claim that the original GPU workload is safe.
