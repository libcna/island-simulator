# Dry sand microrelief

2026-10-11, native visual stage; Android/Web pending together with coastal turf.

The ground-level bay review exposed a smooth dry-sand plane. The terrain material
now adds wind-aligned ripples with a nominal 22 cm wavelength and millimetre-scale
height, warped phase and a varying amplitude envelope. Both phase and envelope
gradients contribute to the shading normal. Direction comes from `PrevailingWind`
through one vec2 uniform, not a separately authored direction.

A finer 85-cells/m noise field supplies tiny grain relief and pigment variation
only where the pixel footprint can resolve it. Ripples fade according to their
highest harmonic; grain fades separately. Dry-height/shore masks keep this out of
the existing wet shoreline and submerged sand treatment. No geometry, textures,
samplers or draw passes are added. This is shading relief, not collision geometry,
dunes, dynamic sand or a claim that individual grains are visible at eye height.

The first native candidate was rejected for a bright foreground. Its squared
normal-length floor of 1e-8 was too large for cross products of tiny world-space
pixel derivatives. The sand normal now uses a 1e-20 floor, preserving unit length
at those footprints. The next iteration varied amplitude/phase to reduce a combed
appearance. Rejected evidence is retained; unrelated soil/wet-normal code is unchanged.

## Evidence and limits

- Native build succeeds. The non-display suite passes 33/33 groups (50.10 s);
  final shader refinements additionally pass terrain-material, CLI and browser
  preflight groups, plus Web harness syntax/diff checks.
- Final native `20261011T092129.366965Z-bay-ground` and
  `20261011T092157.241700Z-bay` pass and were visually inspected. Both kernel deltas
  are empty; hardware latch unchanged. The distant composition remains intact.
- World geometry/placements unchanged from coastal turf: fingerprint
  `5d0b62ad43a25895`, 17,225 grass patches.
- Web terrain matrix now includes `bay-ground` (seven views total). Android and Web
  runtime checks are pending for this shader and the preceding coastal-turf change.
  Static captures alone do not establish temporal antialiasing or hardware performance.

Remaining realism work includes geological strata/talus, dune-scale relief, habitat
fields and more natural close sand debris. The broader environment/geography tasks
remain open. The existing soil/wet material normal-length floors also deserve a
separate review; they were not changed by this pass. Android lifecycle stays degraded.
