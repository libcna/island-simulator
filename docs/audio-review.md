# Audio Source Audit and Listening Review

## Purpose

The runtime soundscape is generated from original deterministic code rather than stored audio
assets. Numerical checks can catch broken formats, silence, clipping risk, DC bias, clicks at loop
boundaries, accidental duplicate variants, and unstable regeneration. They cannot decide whether
the island sounds pleasant, natural, well balanced, or tiring. This document keeps those two kinds
of evidence separate.

## Deterministic report

Build and run the native host tool:

```bash
cmake --build build --target island_audio_report --parallel 2
build/island-audio-report --output /tmp/island-audio-report.json
```

The JSON records every source's:

- sample rate, sample count, and duration;
- normalized mean/DC offset, RMS energy, and peak magnitude;
- end-to-start seam step and largest internal adjacent-sample step;
- zero-crossing rate;
- stable PCM fingerprint and source identity;
- per-clip and overall validation state.

The gate requires all 16 sources to remain mono 22.05 kHz PCM16 at their intended duration, retain
audible energy and mix headroom, avoid material DC bias, and stay finite. A looping source's seam
must be both absolutely bounded and no worse than 125 percent of its largest ordinary internal
step, and its normalized absolute mean must remain at or below 0.0001 after PCM quantization. The
gate also rejects a full-scale adjacent jump and an implausible zero-crossing rate. Every
non-looping accent or wildlife phrase must begin and end at exact silence. Fingerprints must be
unique within the set; they are change detectors for the qualified host/toolchain rather than
cross-platform golden promises.

`IslandAudioReport` runs in native CTest. `IslandAudioPreviewExport` also creates all previews in a
temporary directory, parses them with an independent standard-library WAV reader, verifies channel,
sample-width, rate, frame count, and complete payload, then removes the directory. The ordinary
report test leaves only a small JSON file in the build tree. These development artifacts do not
enter WebAssembly, APKs, or release presentation.

## Disposable WAV previews

Export all exact runtime sources for listening without adding generated binaries to the repository:

```bash
build/island-audio-report \
  --output /tmp/island-audio-report.json \
  --wav-directory /tmp/island-audio-preview
```

The directory contains four loop beds and twelve one-shots as standard little-endian, mono,
22.05 kHz PCM16 WAV files. They total about 3.18 MiB plus headers. Delete the directory after review;
the sources are reproducible from code.

Listen first to each source alone:

1. Loop `surf.wav`, `breeze.wav`, `jungle.wav`, and `water-movement.wav` for at least two minutes
   each. Listen specifically for the boundary, tonal whistles, obvious short repetition, harsh
   high-frequency noise, and low-frequency pressure.
2. Compare both stronger-wave and water-transition variants at matched playback volume. They should
   read as related events without sounding like pitch-shifted copies.
3. Compare `sand-footstep-a.wav`, `sand-footstep-b.wav`, `soil-footstep-a.wav`, and
   `soil-footstep-b.wav`. Sand should remain lighter and looser than soil, variants should reduce
   repetition, and none should sound like a click or an exaggerated boot impact.
4. Compare both bird and monkey phrases. Confirm that calls remain atmospheric rather than alarm-
   like, synthetic in an obtrusive way, or much louder than the beds.

Then perform the authoritative in-game pass with headphones and ordinary speakers. Walk a complete
beach circuit, both clearings, the main jungle trail, dense interior, shallow water, and the active
offshore-current field for at least 20 minutes. Record silence, abrupt zone changes, masking,
repetition, directional confusion, and fatigue in `PLAN.md`. Use F1 to observe layer gains and F2
to compare audible calls with exact source markers. Numerical success alone does not complete this
subjective task.

## Current measured baseline

The current report validates 16 unique sources and 3,336,164 PCM payload bytes. Normalized peaks
range from approximately 0.56 to 0.86. The four persistent loops subtract their generated mean
before quantization; their measured normalized means are at or below approximately 0.00000002.
One-shots retain exact silent endpoints and the largest absolute mean across all sources remains
below 0.0069. The four loop seams range from approximately 0.0055 to 0.0268 and remain below their
own largest ordinary internal step. The persistent 10/11/29/7-second lengths align as a complete
pattern only every 22,330 seconds (6 hours, 12 minutes, 10 seconds); these figures establish
regression bounds, not a claim
of perceptual quality. Runtime initialization handles one source at a time, so it does not retain
both complete 3,336,164-byte PCM and conversion sets as game-owned staging. The largest current
source pair is 2,557,800 bytes for the jungle bed; this is a staging-memory bound, not total mixer
or process memory.

Each land-footstep source is 6,615 samples / 0.3 seconds and has exact silent endpoints. Sand A/B
measure RMS 0.1257/0.1292, peaks 0.58, and maximum internal steps 0.4476/0.4276. Soil A/B measure
RMS 0.1232/0.1378, peaks 0.62, and maximum internal steps 0.5833/0.5835. Those bounds validate the
generated signals; they are not a claim about perceptual material accuracy or mix balance.

Renderer-independent regressions additionally inject non-finite time, shoreline distance, water
depth, movement speed, current strength, and positional-audio coordinates. Those values must not
enter a mix, advance an event scheduler, fabricate a shoreline crossing, or request a wildlife
call; the next valid update must recover from finite state. This validates control-state safety,
not audible quality. Footstep regressions additionally prove distance-based walk/run cadence,
sand/soil selection, variant reachability, bounded playback parameters, teleport rejection, and
silence during swimming.

At runtime the three persistent environmental beds also receive deterministic, non-amplifying
gain drift with mismatched roughly 44--131 second component periods. Maximum downward movement is
3.5 percent for surf, 7.5 percent for breeze, and 5.5 percent for jungle. Tests prove bounded
movement, deterministic reset, and the unchanged twenty-minute mix ceilings. This reduces static
gain fatigue but does not replace the required in-game headphone and speaker review.
