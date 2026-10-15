# Seasonal wetland

The island has one small damp dry-season depression centred at world position `(0, -140)`. It is
separate from the permanent west-flowing stream, more than 90 metres inside the shoreline and about
25 metres from the main footpath. The existing terrain already forms a shallow local basin there;
the implementation deliberately does not cut another channel or add a permanent water plane.

`IslandEnvironmentField::SeasonalWetlandPotential` is the shared finite elliptical control. It
feeds the CPU moisture/drainage sample and its generated GLSL counterpart with matching constants.
The centre reaches high moisture but only 0.60 drainage, distinguishing damp retained soil from the
stream's flowing channel. Existing grass and vegetation placement therefore respond to the same
field used by the terrain material.

The terrain shader adds dark humic earth, cool moss flecks and broad irregular drying breakup. Forty-two
deterministic local ground plants form a broken fern-rich patch using the existing ground-plant meshes,
instance cells, LODs and draw calls. Their placement is bounded by the wetland field, slope, route,
tree-root and existing-foliage clearance. No texture, mesh family, buffer or renderer was added.

The procedural jungle loop already contains sparse insect chirps. Near this depression the soundscape
raises that bed by at most 0.038 and slightly shelters the breeze while leaving stream volume unchanged.
This provides a localized insect character without another persistent audio source and without false
flowing-water sound.

## Evidence

- Native llvmpipe `build-consumer/software-captures/20261015T003306.759798Z-wetland` passes at
  1280 by 720 with 241 colour bins, 46.96 luminance deviation and 185.45 spatial contrast. The image
  was inspected at original resolution.
- Chrome/SwiftShader `build/browser-software/20261015T003955.406585Z-chrome-wetland` passes one
  60-interval view with `GL error 0`, a live context and no new kernel GPU fault. Its image was
  inspected and matches the native soil/fern composition.
- The Release consumer build and all 63 enabled non-display tests pass. Direct tests cover field
  bounds, non-flowing drainage, deterministic plant membership and soundscape separation.
- World-report fingerprint is `1b565d9d9e9ce490`; it records 2,945 ground plants including exactly
  42 seasonal-wetland plants.
- Default ARM64 assemble and application lint pass through the two-worker wrapper using the pinned
  Sharp Runtime checkout. The APK is 24,055,539 bytes with SHA-256
  `20b43c68e9dc7a74f073409ebf7ab14f7909021e759d7e3cfdf5709dee8ea265`. No emulator or KVM was used.

