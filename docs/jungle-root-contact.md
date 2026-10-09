# Jungle root origins on the rendered terrain

Date: 2026-10-09. Game-side placement correction, unrelated to the unresolved
AMDGPU incident. No dependency, shader, mesh or collision data is changed.

## Measured discrepancy

Jungle layout uses the continuous analytic terrain height, including small positive
offsets for low plants. Terrain rendering instead interpolates a 241 by 241 vertex
grid as triangles. Those surfaces agree at grid vertices, not at arbitrary roots.
The world report now measures signed mesh-origin gaps against the drawn surface:

| Default layer | Count | Old minimum / maximum (m) | Old RMS (m) | Origins >2 cm above / below |
|---|---:|---:|---:|---:|
| Canopy | 281 | -0.0582 / +0.0721 | 0.0206 | 41 / 36 |
| Understory | 1,822 | -0.0592 / +0.1043 | 0.0273 | 811 / 55 |
| Shrubs | 578 | -0.0495 / +0.0849 | 0.0239 | 178 / 37 |
| Ground plants | 3,148 | -0.0768 / +0.1224 | 0.0219 | 819 / 203 |

These are origin-height measurements, not classifications of the entire visible
plant or its pixels. `build/island-root-contact-before.json` retains the baseline.

## Correction and ownership

`GroundJunglePlacement` returns a rendering-only placement copy, replacing y with
the exact piecewise-planar `RenderedTerrainSurface` height. `JungleRenderer` uses
that copy for instance translation **and** canopy/cell bounds, for both detail tiers
and both custom/stock-effect paths. The existing shader receives the same corrected
ground origin through its instance matrix. No arbitrary burial offset is added.

The original layout, x/z, scale, yaw, species, ordering, collision proxies and world
fingerprint remain unchanged (`edef1a1dc4f5eb51` for the current default world).
A startup-only 232,324-byte height cache is heap allocated and released after cell
construction. It adds no resident GPU resource, per-frame query, draw, triangle or
attribute. Its construction does add startup CPU work; no physical-GPU performance
claim is made. No CNA or Sharp Runtime modification is needed.

## Validation

- All 32 non-display CTest groups pass in 21.19 s. Extended rendered-terrain tests
  check 17,232 roots across three seeds, independently evaluating triangle planes
  from grid vertices with a 0.02 mm tolerance, plus unchanged non-height fields and
  idempotence. The old largest discrepancy is 0.122392 m.
- `build/island-root-contact-after.json` reports zero origin gaps using the shared
  placement policy. This CPU report is not GPU readback or a whole-footprint test.
- Contained native llvmpipe `ground-broadleaf` captures:
  `20261009T111015.733542Z-ground-broadleaf` before and
  `20261009T111326.983357Z-ground-broadleaf` after. Both pass image/shader checks
  with empty live/final kernel deltas. Converted before/after PNGs in `build-probe`
  are inspected. The change from this camera is small, not a dramatic realism pass.
  Wind is active; images are not frozen pixel-isolation measurements.
- Contained Chrome/SwiftShader `20261009T111538.624402Z-chrome-terrain` passes
  overview, beach, east and jungle, 60 intervals each, with zero GL errors/context
  loss and empty live/final kernel deltas. All four logs contain the root-contact
  marker; east/jungle post-simulation images are inspected. This does not repeat
  the eight-view vegetation, full input/lifecycle or moving-camera matrices.
- Web release package: 13,906,356 raw / 6,970,946 gzip bytes, regenerated from the
  tested build. This is not a renewed hardware browser qualification.
- Android x86_64/restored ARM64 assembly and lint pass (3 s / 4 s). Contained
  API 35 KVM/SwiftShader `20261009T112028.208485Z-ground-broadleaf` passes, app
  PID 1844 / entry TID 1901, first-draw total 12,042.214 ms. Root-contact marker
  is present, the 1280x720 image is inspected, live/final kernel deltas are empty
  and the owned emulator exits zero. APK hashes are in `build-android.md`.
  This does not repeat Android touch/lifecycle/traversal tests or qualify ARM64
  runtime/physical-phone performance. All graphics checks remain software-only.

Final 32 non-display groups pass in 21.91 s, including the regenerated Web package.
Script syntax and whitespace checks pass. No owned game/emulator/compositor/browser
process remains; the final host kernel interval since 13:09 CEST is empty. The
hardware stop-latch hash remains unchanged. Disk has 359 GiB free; only rolling
generated APKs were replaced and all original incident evidence was preserved.

## Remaining limits and next work

Matching a single root origin does not conform a broad trunk base, multiple shrub
shoot bases or hanging leaves to a slope. A subsequent [fallen-leaf contact pass](fallen-leaf-contact.md)
now drapes off-centre litter in the custom shader; the stock fallback remains flat. Local geometry may
still float or intersect the terrain away from its origin. Ground/canopy detail
transitions remain discrete, and vegetation/prop intersections still need review.
The coarse precomputed canopy-light field and terrain contact-shadow approximation
retain their existing analytic-layout assumptions; this is not an exact shadow
alignment pass. The first-person collision surface remains analytic.

Next, inspect close off-path colonies and their full footprints before increasing
density, and improve leaf surface character without obscuring geometry problems.
This correction does not qualify AMD hardware or explain the original GPU fault.
