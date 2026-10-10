# Sky volume and shared haze

2026-10-10, from the macro-geography analysis items 12 and 13: the sky was a clean two-colour
gradient with sinusoidal wisps and read as a shader, and every renderer fogged toward its own
copy of one literal, so distant terrain, water and islets could sit in front of the sky instead
of dissolving into it.

## What changed (`SkyRenderer.cpp`, portable GLSL 330 / ES 300)

- Horizon haze: an `exp(-9 y)` band toward the shared fog colour `(0.53, 0.78, 0.88)`, plus a
  pale sea-level band below the horizon, over a deeper zenith `(0.12, 0.42, 0.74)`.
- Warm sun surroundings: a broad `pow(sunDot, 5)` glow in addition to the existing halos and disc.
- Eleven fixed distant fair-weather cumulus masses, authored as azimuth / elevation / angular
  half-width / half-height ellipses eroded by two octaves of hashed value noise, mostly from
  above so bases stay flatter than tops. Tops are sunlit white, bases grey-blue, the sunward side
  brighter; masses near the horizon soften toward the haze. Nine sit low over the sea, two a
  little higher. They are fixed: still a sunny daytime, no weather.
- The high horizontal wisps and their diagnostic are unchanged; a new diagnostic names the sky
  volume and is pinned by the native capture harness.
- The stock-effect fallback dome uses the new horizon/zenith colours.
- `environment::FogColorRed/Green/Blue` now back the 14 C++ `setFogColorProperty` sites that
  previously repeated the literal; GLSL bodies still repeat the same literal and must be kept in
  step by hand (documented in the header). Underwater renderers keep their own water fog.

## Cost

The cumulus loop runs only for directions between -0.02 and 0.45 elevation and skips a mass by
its bounding ellipse before any noise, so most sky pixels evaluate no noise; at most two octaves
per mass inside it. No new texture, buffer or draw.

## Evidence and limitations

Native llvmpipe `build/software-captures/20261010T145219.907592Z-beach`, `145227.215759Z-overview`,
`145234.350022Z-palms` (pass; images inspected; before: `145055.141034Z-beach`). From the
overview the island now sits under a hazy horizon with cloud masses; from the beach the masses
show between trunks. Masses are smooth ellipse blobs without cauliflower edges, cast no light or
shadow on the world, and the fog literal is not yet generated from one source for GLSL.

Platform runs: Chrome/SwiftShader litter scope `build/browser-software/20261010T145543.719907Z-chrome-litter`
(pass, both views, empty kernel deltas), Android KVM/SwiftShader
`build/android-software/20261010T145908.469206Z-palms` (pass; app PID 1822 / entry TID 1890,
first draw 5,989.730 ms, total 16,349.383 ms, image inspected, empty kernel deltas). Rolling
packages: Web 10,368,543 raw / 3,527,432 gzip bytes; x86_64 APK 20,406,235 bytes
(`87617ee62e675cb38b8290d020d31c1be52cf87cd60f6e134e1a0c096acd12d6`); ARM64 APK 20,495,028 bytes
(`c4bf396f138198cf78140e382549c9af76483cce479b8e40ad453f7bf61a58dd`), runtime untested.
