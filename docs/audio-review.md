# Audio Source Audit and Listening Review

## Stream-entry runtime dropout — 2026-10-12

The operator heard all audio stop during two Release runs. The real-CNA regression
`IslandSoundscapeRuntimeTests` reproduced this without graphics or a physical audio output:
stream `Play()` occurred before its first `Apply3D()`. CNA correctly rejects changing a submitted
non-spatial packet to 3D, and the game's fatal-audio catch then destroys all audio instances.
Their recorded route also enters the stream's 34m sound region.

Spatial setup now precedes Play both at startup near the stream and upon entering its region.
The test exercises far/near startup, repeated approaches and departures, stream/swim arbitration,
mute and unmute. It checks the actual backend Playing state of at least four ambient tracks and
the six-source cap, not just an available flag or estimated owned-instance count. It requires
`SDL_AUDIODRIVER=dummy` and never opens a graphics context or the user's speakers. This is runtime
lifecycle evidence, not a listening or physical-device qualification.
Before-fix failure is retained in `build-consumer/audio-stream-before.log`; after-fix runs pass in
both Release and Debug (`build-consumer/audio-stream-after.log`, `build/audio-stream-after.log`).

Audio startup, mute and fatal errors are mirrored into persistent runtime logs; five-second frame
windows also record audio availability, mute and actual playing-source count. The existing safe
silent fallback remains for genuine device/runtime failures, now with retained evidence.

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

The gate requires all 40 sources to remain mono 22.05 kHz PCM16 at their intended duration, retain
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

The directory contains six loop beds and thirty-four one-shots as standard little-endian, mono,
22.05 kHz PCM16 WAV files. Their PCM payload totals 6,052,724 bytes plus WAV headers. Delete the directory after review;
the sources are reproducible from code.

Listen first to each source alone:

1. Loop `surf.wav`, `breeze.wav`, `jungle.wav`, `stream.wav`, and `water-movement.wav` for at least two minutes
   each. Listen specifically for the boundary, tonal whistles, obvious short repetition, harsh
   high-frequency noise, and low-frequency pressure.
2. Compare both stronger-wave and water-transition variants at matched playback volume. They should
   read as related events without sounding like pitch-shifted copies.
3. Compare `sand-footstep-a.wav`, `sand-footstep-b.wav`, `soil-footstep-a.wav`, and
   `soil-footstep-b.wav`. Sand should remain lighter and looser than soil, variants should reduce
   repetition, and none should sound like a click or an exaggerated boot impact.
4. Compare both bird and monkey phrases. Confirm that calls remain atmospheric rather than alarm-
   like, synthetic in an obtrusive way, or much louder than the beds.
5. Compare both `boar-breath`, `boar-snuffle` and `boar-grunt` variants. These are original
   synthesized prototypes, not recordings: verify that they actually read as a pig/boar,
   not a pitched oscillator or generic hiss. Test resting, foraging and moving animals,
   listener rotation, approaching/leaving range, multiple animals, mute and resume.
   Replace or refine the synthesis if listening does not support natural boar identity.
6. Compare `snake-hiss-a.wav` and `snake-hiss-b.wav`. They are original synthesized
   broadband prototypes, not recordings. Confirm they read as restrained close snake
   hisses rather than surf, static or steam; then verify direction, attenuation, retreat
   triggering, cooldowns and mute/resume beside several live snakes.
7. Compare both `goat-breath`, `goat-bleat` and `goat-hoof` variants. Confirm bleats
   remain recognisably caprine without becoming shrill or repetitive, and that the
   quieter close details remain audible only at suitable distances. Review grazing,
   resting, walking and retreating goats, listener rotation, mute/resume and competition
   with other wildlife calls.
8. Compare `shipwreck-creak-a.wav` and `shipwreck-creak-b.wav` at quiet playback level. They
   should read as loaded old hull timber rather than a door hinge, animal voice or tonal effect.
   In game, verify direction and falloff while circling the wreck and ensure intervals remain
   unobtrusive beneath surf on both headphones and ordinary speakers.

## Sparse shipwreck creaks (2026-10-12)

Two original deterministic 1.8-second mono clips model low loaded-timber strain with swept
resonances, inharmonic partials and filtered fibre texture. Variant A/B peak at 0.63998/0.60999,
RMS is 0.23128/0.21377, normalized mean is 0.00051/-0.00019, endpoints are exact silence, and
fingerprints are `26ec888679d25c9e` / `a51318948291aa33`. The complete report and independent
WAV export now pass for 40 distinct clips and 6,052,724 PCM bytes. These measurements prove
bounded non-silent data, not that the synthesis sounds like real wreck timber.

`ShipwreckCreakScheduler` requires audible surf and a listener within 34m of the immutable wreck
emitter. Its first delay is 10--17s and subsequent intervals are 17--41s; gain is bounded to
0.035--0.16 and pitch to -0.075--0.045. Leaving range when due advances to a fresh short wait, so
missed creaks cannot accumulate. The creak reuses the stronger-wave `SoundEffectInstance`, and
therefore adds prepared effects but no seventh simultaneous source or persistent loop. Water
transitions retain their pre-emption priority; mute stops the shared instance immediately.

Core tests cover invalid input, distance, busy-slot holding, both variants and parameter bounds.
The real CNA runtime test observes actual creak playback, mute and distance suppression while
holding the four-persistent/two-transient limit. An isolated SDL disk-device run produced 249,856
stereo S16LE bytes, including 63,398 nonzero samples with peak 10,536, and logged the first creak;
this proves backend submission, not subjective spatial or naturalness acceptance. Web/Android
runtime playback and a human listening pass remain open.

Then perform the authoritative in-game pass with headphones and ordinary speakers. Walk a complete
beach circuit, both clearings, the main jungle trail, dense interior, shallow water, and the active
offshore-current field for at least 20 minutes. Record silence, abrupt zone changes, masking,
repetition, directional confusion, and fatigue in `PLAN.md`. Use F1 to observe layer gains and F2
to compare audible calls with exact source markers. Numerical success alone does not complete this
subjective task.

## Goat calls and native backend evidence (2026-10-12)

Six original deterministic 1.2-second mono clips add two variants each of breathing,
bleating and hoof detail; no recording or external audio asset is used. At the goat-integration
checkpoint, the then-38-clip report and independent WAV export passed with 5,893,964 PCM bytes. Breath A/B peak at
0.47998 with RMS 0.09644/0.08438; bleat A/B peak at 0.59998 with RMS
0.22678/0.20955; hoof A/B peak at 0.53998 with RMS 0.07758/0.07128. All have exact
silent endpoints and distinct fingerprints. These measurements prove bounded,
non-silent source data, not natural goat identity.

`GoatCallScheduler` uses the six live goats' actual behaviour. Each eligible animal
starts with a bleat; graze/rest may then produce close breathing, walk may produce
hoof detail, and retreat uses a bleat. A quiet detail below the near-listener gain
threshold is replaced with a bleat instead of becoming inaudible. Per-animal cooldowns,
a 2.8--5.0 second herd gap, deterministic round-robin selection, pitch/gain variation
and one pooled wildlife voice prevent a synchronized chorus. The emitter follows the
selected goat; invalid, removed, distant, paused or muted states cannot accumulate or
continue a call. Bear and snake requests retain priority, followed by goats and boars,
without exceeding four persistent plus two transient sources.

The real-CNA dummy-backend regression observes a playing goat source, mute teardown,
unmute recovery, distance rejection and the six-source cap. A supervised private
pixman/llvmpipe run at `build-consumer/software-goat-audio/20261012T220810.661476Z`
used SDL's disk device and a fixed live-goat view. It observed three goat calls,
wildlife type 6, a maximum of five sources, 991,232 exact-zero muted PCM bytes and
262,144 nonzero bytes after unmute; the kernel fault delta is empty. The diagnostic
screenshot was inspected, but is not goat-appearance evidence. Actual speaker/headphone
naturalness, spatial turn/distance, overlaps and Web/Android lifecycle remain open.

## Stream loop and local wet sound biome (2026-10-12)

One original deterministic thirteen-second loop combines several circularly filtered noise
bands with restrained pebble-scale modulation. `StreamSoundField` projects the listener onto 32
segments of the authoritative `StreamRoute`; gain is full within 4m and smoothly reaches silence
at 34m. The positional emitter follows that nearest route point, so one voice covers the whole
approximately 102m meandering watercourse without a chain of emitters. Near the stream, the
existing jungle bed gains at most 0.024 while canopy breeze falls by at most twelve percent.

The stream instance and player-water instance are both allocated, but only one plays: dry land
near the stream selects the stream, while swimming or leaving its audible field restores player
water. This preserves the existing four-active-ambient plus two-transient, six-source ceiling and
therefore does not starve footsteps or animal calls. At this integration checkpoint the report
validated 25 unique clips and
4,315,184 PCM bytes. Core tests cover route projection, falloff, invalid input, biome mix,
determinism, DC centering, signal energy and a combined five-loop phase period of 290,290 seconds.
The independent WAV export passes. These are implementation and numerical results; actual spatial
listening, full route walking, switch/mute behavior and Web/Android lifecycle remain open.

## Snake hiss implementation and unverified listening (2026-10-12)

Two original one-second mono procedural hisses use filtered broadband noise with
different sustained/double-pulse envelopes. The 24-clip report and independent WAV
export pass at 3,741,884 PCM bytes. Variant A/B fingerprints are
`587c135aae098175` / `968b26b9548758cd`, peaks are both 0.56 and RMS is
0.18884 / 0.17045. These numerical values prove bounded non-silent distinct PCM, not
that the result sounds like a real snake.

The renderer-independent scheduler only admits non-dormant snakes whose current
behaviour is Retreat. Gain falls quadratically to silence at 16m. Each snake receives
a randomized 7–15s cooldown and the whole population a 3–5s cooldown; variant, pitch
and gain vary deterministically. Hisses use the existing single wildlife voice, after
due bird/monkey calls and before boars, so the six-source maximum is unchanged and
multiple snakes cannot hiss simultaneously. While playing, the emitter follows the
selected head and stops when the snake is dormant or no longer retreating.

Core tests cover deterministic PCM, exact endpoints, variants, attenuation, fairness,
cooldowns, busy voice, invalid/paused input, range, dormant and non-retreat states.
F1 and the input-sample diagnostic expose the played hiss count. A supervised private-
llvmpipe native run in `build/software-snake-audio/20261012T133332.612520Z` used SDL's
disk backend with a close retreating live snake. It observed wildlife type 4, one hiss
before mute and another after resume, 1,482,752 exact-zero muted bytes, 2,277,376 nonzero
resumed bytes, at most five of six sources, and no new kernel GPU fault. That run also
exposed and fixed a real integration defect: fixed inspection cameras did not advance
the soundscape or process M, because both lived inside interactive camera movement.
Soundscape advancement and mute input now run from the common update path. This is
backend/playback evidence only; subjective listening, spatial turn/distance and
Web/Android runtime lifecycle have not yet been accepted.

## Boar audio implementation and unverified listening (2026-10-11)

### Quiet-behaviour grunts — operator silence report follow-up (2026-10-12)

Source inspection found a concrete limitation: Rest could only breathe and Forage
could only snuffle; neither could ever grunt without changing behaviour. Rest breath
gain at seven metres was only about0.0069 before clip amplitude/spatial mixing.
This can explain absent grunting/very weak nearby detail, but does not establish the
cause of all silence on the operator's device.

Each animal now starts its scheduled vocal sequence with a grunt, then cycles through
two behaviour-specific details. When a breath/snuffle would have model gain below0.03,
it uses a grunt instead, retaining the same behaviour cooldown and42m grunt cutoff.
Per-animal phase resets deterministically and advances only on a returned request;
mute/busy slots cannot accumulate bursts. Voice count, PCM clips, gain curves and
bird/monkey arbitration remain unchanged. This is not simply global volume boosting.

Regression simulates Rest and Forage separately for120seconds at2/7/15/30m: each
must produce repeated grunts, first within11seconds with an available voice slot;
at2m it must also retain breathing/snuffling variation. Existing deterministic herd,
range, dormant, mute/busy-slot and cooldown tests remain. A busy real soundscape may
delay a due call; the CPU deadline is not a runtime scheduling guarantee.
Naturalness, actual speaker audibility and portable listening remain unqualified.
Final native incremental build and all55 CPU groups PASS87.77s
(`build/boar-grunts-{build,full-tests}.log`). Actual contained llvmpipe/SDL-disk run
`build/software-boar-audio/20261012T062340.029253Z` parent/worker PASS, kernel-final[].
First boar call logs animal0/kind3 (grunt); three calls total across101 diagnostic
samples, maximum6/6 sources. Mute produced991232 exact-zero bytes; resumed output
contained114688 bytes with nonzero samples. Actual diagnostic screenshot inspected;
foliage obscures the animals, so it is not boar appearance evidence. Separate current
boar capture062020.501241Z was inspected in `build/boar-current-review.png`: plain
smooth torso/long muzzle remain visual defects; no model/material change this step.

Six original synthesized 1.2-second mono clips now join the live Soundscape: two each
for breath, snuffle and grunt. Filtered deterministic noise, irregular low harmonic
voicing and separate exhalation envelopes generate the clips; no third-party recordings
are used. Provenance is included in ASSET_LICENSES.md. Total PCM is now 3,653,684 bytes
across 22 unique sources, +317,520 bytes for boars; sequential startup staging remains.
Measured boar peaks are 0.48/0.62, RMS 0.0623–0.1303, maximum absolute mean 0.00456,
with exact silent endpoints. Report/export tests pass without weakening audio gates.

Rest uses breath within 9 m, forage snuffle within 23 m, wander/retreat grunt within
42 m (retreat uses shorter cooldown, not an aggressive attack sound). The common
distance-gain function now serves both scheduling and live emitter updates; this
changes the initial scheduler's wander-only 38 m range to 42 m. Each animal has a
cooldown, the herd has a 2.5–4.5 s gap, and candidate selection rotates fairly.
Due birds/monkeys have first refusal of the existing single wildlife voice, followed
by boars. Four persistent plus two transient maximum voices remain unchanged.
While playing, boar source position and volume follow that animal/listener; dormant,
invalid or removed sources stop. Existing mute destroys transient voices. F1 now
reports a BOAR call count and the first actual playback logs a bounded diagnostic.

Native build and all 39 non-display groups pass (83.36 s). WAV previews exported to
`build/boar-audio-preview/`; numerical report is `build/boar-audio-report.json`.
Web Release build/package and ARM64 assembleDebug/lintDebug also pass (Android 6 s).
These establish implementation/format, not naturalness or actual audible playback.
No listening-capable tool is available in this session. In-game playback, spatial
listening, source arbitration, mute/resume and Web/Android audio lifecycle are still
unqualified. Keep the user task open until those checks are actually performed.

### Actual native disk-device playback and mute

`build/software-boar-audio/20261011T174853.374413Z` passes the owned display/PID/device
supervisor with llvmpipe and the SDL disk audio backend. The installed SDL headers and
source define this backend as writing real playback buffers to a file; the actual
log identifies S16LE, stereo, 44100 Hz. Unlike earlier display tests that reported
ALSA unavailable, this run reports audio available and confirms each wildlife voice
enters the backend Playing state before incrementing its playback count.

Actual F1/K/M input navigates near the live herd, observes two boar calls, mutes for
24 diagnostic intervals, then unmutes and observes a third call. Across 130 audio
samples, at most five of six allowed sources are allocated. Muted samples retain
four silent bed voices, no wildlife voice and an unchanged boar count. After draining
the audio pipeline, 1,048,576 captured bytes are exactly zero; after unmute 884,736
captured bytes contain nonzero audio. Kernel delta is empty; parent and worker pass.
34 supervisor/trace CPU tests pass, including negative budget, missing playback and
mute fixtures. The final strengthened validator also passes the retained real trace.

The actual screenshot was inspected: F1 text is readable, but foliage obscures the
boars. It is diagnostic evidence, not animal appearance acceptance. K now cycles
through all six boars with F1 visible for further audio/visual review. Exported
`game-audio.wav` wraps the captured bytes without altering the signal. It is the
mixed game output, not an isolated boar stem or proof of perceptual boar identity.
This test does not qualify hardware speakers, spatial listening, focus/suspension,
Web/Android runtime audio or subjective naturalness. Those requirements remain open.

## Earlier measured baseline (before boar clips)

The pre-boar report validates 16 unique sources and 3,336,164 PCM payload bytes. Normalized peaks
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
