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
