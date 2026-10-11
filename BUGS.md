# CNA and Sharp Runtime Issues Affecting island-simulator

This file is the authoritative record of confirmed or strongly evidenced framework defects that
materially affect `island-simulator`. CNA and Sharp Runtime are read-only dependencies. Framework
issues are worked around only inside this repository and are never patched in their worktrees.

No qualifying CNA or Sharp Runtime defect has been confirmed yet. One Android lifecycle
failure has strong framework-boundary evidence and is recorded as suspected below.

Consumer integration constraints, unsupported features, renderer limitations, and incorrect game
code do not belong here unless investigation demonstrates a framework defect. Suspected entries
must say `Status: Suspected` and must not present a guessed cause as fact.

### CNA-ISLAND-001 — Android presentation races surface destruction during Home/resume

**Component:** CNA SDL3 platform/runtime presentation boundary

**Branch/revision:** next, `97783d1e090c6d84f4b6a6d7746516e65184569e`

**Platform/renderer:** Android API 35 x86_64 emulator, GLES3 on SwiftShader

**Severity:** High

**Status:** Suspected; observed lifecycle failure, exact ownership/root cause unconfirmed

**Date discovered:** 2026-10-11

#### Symptoms and evidence

Contained run `build/android-software/20261011T073115.062445Z-interactive` passes touch
selection, land footsteps and shoreline traversal, then fails same-process Home/resume.
`game-workload.log` records `surfaceDestroyed()` / `nativePause()` at 09:32:28.542,
`eglSwapBuffersWithDamageKHRImpl` returning `EGL_BAD_SURFACE` at 09:32:29.258 and
`CNA::Platform: GlContext::SwapBuffers failed` terminating the game at 09:32:29.913.
The Java surface is recreated in between, at 09:32:29.385. Kernel delta is empty and the
owned emulator exits zero. This is not an AMD GPU fault. An earlier lifecycle run on the
footpath tree (`20261011T071027.731091Z-interactive`) passed; timing dependence is plausible.

#### Reproduction

Use the contained launcher with the x86_64 ridge-stage APK, `--viewpoint interactive
--touch-selector --lifecycle --traversal`; see `docs/ridge-and-valleys.md`. A single failed
run establishes the failure, not deterministic reproduction. Retain its evidence even if
a subsequent attempt passes.

Identical-APK repeat `20261011T073355.598497Z-interactive` passes every gate, including
hot resume (PID 1756) and cold relaunch (PID 3258). This demonstrates intermittent behaviour
on the same artifact; it does not resolve the failure or qualify reliable Android lifecycle.

#### Suspected cause and consumer impact

Read-only source inspection: `Sdl3Platform::LifecycleWatch` queues lifecycle events for a
later poll; `Game` calls `Draw` then `EndDraw`; `GraphicsDeviceManager::EndDraw` presents
and rethrows failures; `Sdl3GlContext::SwapBuffers` throws on a failed SDL swap. A surface
loss during a long software-rendered frame may reach presentation before lifecycle handling
can defer drawing. The SDL/EGL surface ownership sequence needs upstream investigation;
no claim is made that the terrain shader itself causes the race.

Android lifecycle qualification remains degraded until resolved. The game must survive
surface destruction/recreation during backgrounding rather than exit with a fatal error.
There is no consumer workaround: swallowing presentation exceptions or delaying the Home
test would hide the defect. Dependencies remain untouched; nothing has been reported externally.

#### Additional lifecycle qualification observation (not proven to share this cause)

Coast-stage run `20261011T075739.370693Z-interactive` restores the same PID 1869, surface,
window dimensions and focus, and continues rendering without the fatal `EGL_BAD_SURFACE`.
It still fails the lifecycle gate because neither `AAudioStream_requestPause` nor
`AAudioStream_requestStart` appears in the hot-resume log. Surface destruction/nativePause
occur at 09:59:54.097 / 09:59:54.221, nativeResume at 09:59:55.476. The short background
interval and queued lifecycle handling merit investigation, but missing log evidence alone
does not prove an audio engine defect or establish the same root cause as the earlier crash.
Kernel delta is empty. The prior generic failure message incorrectly suggested that surface
or focus failed; the harness now identifies the process/surface/focus/audio gate and prints
audio qualification and both PIDs. No acceptance condition was removed.

Read-only follow-up identifies a concrete mechanism: bundled SDL's
`third_party/SDL/src/core/android/SDL_android.c`, `Android_SendLifecycleEvent`, removes a
pending PAUSE when RESUME arrives before the event pump consumes it (and vice versa).
`SDL_androidevents.c` calls `Android_PauseAudio` only from the consumed pause handler;
`Android_ResumeAudio` acts only if audio was paused. Therefore a short Home/resume can
legitimately produce Java surface/focus logs without AAudio pause/start calls. This code
path is verified; its execution in the failed run is not instrumented and remains an inference.
The audio assertion currently combines a fixed two-second background interval with a demand
for completed suspension. A future suspension-specific test should await observable pause
before requesting resume, with a bounded deadline; retain a separate rapid-transition stress
case and do not weaken the fatal-swap/surface checks. Missing audio log calls alone must not
be promoted to a confirmed framework defect.

## Entry template

<!-- Copy this section only after a qualifying issue has been investigated. -->

### CNA-ISLAND-NNN — Concise title

**Component:** CNA or Sharp Runtime

**Branch:** next

**Revision:** commit hash

**Platform:** affected platform(s)

**Renderer:** affected renderer(s), or N/A

**Severity:** Low / Medium / High / Critical

**Status:** Suspected / Confirmed / Workaround active / Fixed upstream, workaround removed

**Date discovered:** YYYY-MM-DD

#### Symptoms

Describe the observable failure.

#### Expected behaviour

Describe the supported behaviour the framework should provide.

#### Actual behaviour

Describe the measured behaviour.

#### Reproduction

Give the smallest practical reproduction and its prerequisites.

#### Suspected cause

Name relevant framework locations and distinguish evidence from inference.

#### Impact on island-simulator

Describe affected game systems and platforms.

#### Workaround

Describe the isolated game-side workaround and link its source location.

#### Workaround limitations

Describe compromises, platform differences, or residual risk.

#### Workaround removal

Explain how to detect an upstream fix and remove the workaround safely.

#### Validation

List regression tests, builds, and runtime checks performed.
