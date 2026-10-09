# Detached-process containment — 2026-10-09

## Defect and scope

The October 8 serial-incident compositor survived because it belonged to a new session, outside
the process group signalled by the game-side supervisor. Ownership and original logs are in the
[GPU incident](2026-10-08-amdgpu-gpuvm-reset.md). This is a confirmed supervision defect, **not**
the identified cause of the GPUVM fault. CNA and Sharp Runtime were not modified.

`tools/gpu_incident_qualification.py::contained_command` now wraps each monitored workload:

```text
/usr/bin/bwrap --unshare-pid --die-with-parent --bind / / --dev-bind /dev /dev --cap-drop ALL -- <original command>
```

The original shell, compositor, game and detached descendants belong to the same new PID
namespace. Bubblewrap's initial-command exit monitor and parent-death chain terminate its init;
namespace-init death kills remaining members regardless of session/process group. This follows
the [Linux PID namespace semantics](https://man7.org/linux/man-pages/man7/pid_namespaces.7.html)
and the inspected [bubblewrap 0.12.0 implementation](https://github.com/containers/bubblewrap/blob/v0.12.0/bubblewrap.c)
(`handle_die_with_parent`, `monitor_child`, `do_init`). No ancestry polling or delay is used to
discover children. Existing bounded TERM/KILL cleanup targets the owned launcher group.
Missing bubblewrap or namespace setup failure cannot fall back to an uncontained command.

This outer wrapper is **lifetime containment only**: it retains filesystem, network and device
access. It does not make a hardware workload safe, retract submitted GPU commands, or recover a
hung driver. Android retains its separate nested read-only-root/private-device/network sandbox.
The shared software monitor writes `process-containment.json` before launch, recording the exact
outer command and launcher hash; the hardware supervisor records these in run metadata.

The outer wrapper retains the caller's procfs mount. `RuntimeDiagnostics.cpp` obtains PID/TID
from `/proc/self` and `/proc/thread-self` for correspondence with host kernel reports and thread
inventory, while also logging namespace-local getpid/gettid values. Unreadable procfs falls back
to local IDs; startup explicitly identifies that policy. A namespace mounted with its own procfs
(such as Android's inner sandbox) still reports that mount's IDs, not an invented host mapping.

## CPU validation

Exact commands, with no graphics context creation:

```sh
python3 tests/ValidateGpuQualificationTests.py
python3 tests/SoftwareCaptureTests.py
python3 tests/AndroidSoftwareTests.py
export CCACHE_DIR=/rv/cnaccache CCACHE_BASEDIR=/rv
cmake --build --preset linux-debug
python3 tests/ValidateCommandLineTests.py build/island-simulator
ctest --test-dir build -LE display --output-on-failure -j2
sg systemd-journal -c 'python3 tools/run_android_software.py --sdk /home/robertvokac/Android/Sdk'
```

- Supervisor: 38 tests pass both in the tool sandbox (1.483 s) and host context (1.510 s).
  Real CPU fixtures ignore TERM, detach with setsid, double-fork and outlive their initial
  parent. Tests cover clean exit, synthetic journal fault, timeout, interruption, abrupt
  SIGKILL of an established supervisor and survival of an unrelated sentinel process.
  Synthetic faults are strings only: no kernel fault or GPU work is injected.
- Software monitor: 23 tests; Android preflight/asset checks: 15 tests, all pass.
- Native incremental shared-cache build passes. Command-line tests run 24 invalid invocations
  plus an exec-preserved PID fixture in a fresh namespace. Actual startup PID/TID/log filename
  match the fixture's independently recorded outer IDs; argument rejection occurs before GL.
- All 31 non-display CTest groups pass in 20.04 s. Existing screenshot/report validators are
  not new hardware runs.
- Nested Android preflight: `build/android-software/20261009T060433.012207Z-preflight/` passes;
  private network, no DRM/display access, usable KVM, at most eight CPUs, final kernel `[]`.
  No emulator was booted. This does not requalify the Android native diagnostic change.

## One software runtime check

```sh
sg systemd-journal -c 'python3 tools/run_software_capture.py build jungle'
```

Evidence: `build/software-captures/20261009T060515.360852Z-jungle/`. PASS, 1280x720,
207 colour bins, luminance deviation 44.22, spatial contrast 210.88. The PNG was inspected:
the complete jungle and file-backed floor are present; its still-stylized appearance is not
claimed as finished realism. Runtime confirms OPENGL33/EasyGL/SDL3/X11 and Mesa llvmpipe,
with private pixman compositing. Host PID/main TID **3919288** correspond to namespace ID **41**;
the persistent log filename uses 3919288. `kernel-live.jsonl` is empty and `kernel-after.json`
is `[]`. A host process inspection after exit found no owned Weston, game, emulator or CPU
fixture still running. No user-session process was terminated.

## Remaining work and boundaries

- The primary AMD GPUVM cause and hardware acceptance remain unresolved. The stop latch hash
  remains `c0bd170489321c8d497eae0f1211e1553e30aef9ee02ac01774269300e6ee6f1`.
- This validation covers established workloads, not every instruction-level parent-death race
  during bubblewrap startup. It is not a hostile-process security sandbox.
- Node browser entry points now require the contained launcher described below. Direct CNA
  display-test invocations are not wrapped: do not use them while the incident is unresolved.
- Cross-build the diagnostic change when next validating Web/Android; only native was rebuilt
  in this step. Preserve historical Linux Release binaries/archive before updating that target.
- Resume visual/material and wildlife polish with software validation. No root command or
  deliberate GPU fault to obtain another dump is needed from the operator.

## Browser containment follow-up

### Native software device isolation follow-up (2026-10-09)

The native capture path formerly forced llvmpipe/pixman but retained the outer
host `/dev` bind. It was software-renderer validation, not device isolation.
The shared software monitor now defaults to `private_devices=True` (`--dev /dev`),
and its CPU regression checks assert this policy. Native capture therefore hides
host GPU device nodes as well. Browser already requested this policy explicitly.
Android explicitly retains the outer bind because its inner audited sandbox needs
to expose `/dev/kvm`; its inner sandbox still hides host DRM devices. The hardware
qualification launcher and its stop latch are unchanged. This is defense in depth,
not the root cause of the historical GPUVM fault. Native captures
`20261009T120122.533676Z-ground-broadleaf` and `20261009T120211.824931Z-bush` pass
with `private_devices: true`, `--dev /dev`, confirmed llvmpipe and empty kernel
deltas. This does not retroactively make earlier native captures device-isolated.

`tools/run_browser_software.py` owns the Node worker, HTTP server and browser tree through the
same monitored namespace mechanism. Its `private_devices=True` option uses `--dev /dev`, not
the default host device bind. Host DRM/NVIDIA devices are absent. Software driver overrides
and an eight-available-CPU affinity limit are enforced, and display variables are removed.
Native/Android keep their previous device policy. This is still not a hostile-code sandbox.

Both Node entry points call `browser_containment.mjs::requireBrowserContainment` **before**
creating profiles or processes. Direct invocation, incorrect software overrides, a visible
GPU device, or missing outer/local PID separation fails closed. The helper records host/local
worker IDs. `recordBrowserProcess` maps the immediate browser launcher through the worker's
own procfs child list and `NSpid`, not an unrelated-process scan, and preserves both IDs.
CPU fixtures independently confirm that this mapping agrees with the child's own procfs ID.

The parent, outside the user namespace, now owns continuous kernel monitoring for both focused
and full browser suites; the child no longer relies on supplementary journal group membership.
Browser stage markers are explicitly not kernel acceptance. `run.json`, `kernel-live.jsonl` and
`kernel-final.json` remain authoritative even when a child fails before writing its scene report.
Each run writes reports/images into its own evidence directory. Old build-root reports are
untouched. Profiles are removed normally; forced-failure profiles, if any, remain owned evidence.

Validation commands:

```sh
python3 tests/BrowserSoftwareTests.py
python3 tests/SoftwareCaptureTests.py
python3 tests/ValidateGpuQualificationTests.py
node --check tools/run_web_smoke.mjs
node --check tools/run_firefox_web_smoke.mjs
export CCACHE_DIR=/rv/cnaccache CCACHE_BASEDIR=/rv
cmake --build --preset linux-debug
ctest --test-dir build -LE display --output-on-failure -j2
sg systemd-journal -c 'python3 tools/run_browser_software.py build-probe --node /home/robertvokac/emsdk/node/24.19.0_64bit/bin/node --browser chrome --scope terrain --timeout 240'
```

Nine CPU browser checks, 23 software-monitor tests and 38 supervisor tests pass. All 32 CTest
groups pass in 26.22 s (the browser group then had seven tests; the two added PID-attribution
tests subsequently pass in the nine-test run). Retained CTest log:
`build/gpu-incident-evidence/20261009-browser-containment-ctest.log`.

The browser command was run twice, serially:

1. `build/browser-software/20261009T061147.539233Z-chrome-terrain/`: all four views rendered,
   but the modified scene report still referenced removed `kernelFailed`. Node exited 1.
   This was an introduced harness reporting defect, not a GPU failure or accepted test.
   Fixed the reference and made the report explicitly defer kernel acceptance to the parent.
   Logs/images remain preserved; live/final kernel deltas are empty. Host inspection confirmed
   no owned browser/server survived before the repeat.
2. `build/browser-software/20261009T061502.364252Z-chrome-terrain/`: PASS. Chrome host PID
   **3923492**, namespace PID **16**, Node worker host PID **3923477**. Actual renderer:
   ANGLE Vulkan SwiftShader Subzero. Overview, beach, east and jungle each pass 60 CPU-rendered
   frame intervals, with zero GL errors/context loss. Live kernel file is empty; final delta
   is `[]`. Jungle screenshot inspected. No owned browser/server/compositor/test process or
   profile remained after completion. This is software correctness, not hardware performance.

The new Firefox path inherits the affinity cap (no hard-coded CPU indices) and proves a software
WebGL renderer on a blank page before loading the game. The subsequent contained full gate
passes at `20261009T075434.226844Z-firefox-full`, using an owned automatically allocated Xvfb
display because GPU-free Linux headless Firefox refuses WebGL2. The display is also inside
the lifetime/device namespace and has separate PID evidence; no desktop display is used.
See `../browser-input-ownership.md` for failed attempts, short-click correction and scope. The Web
package itself was not rebuilt during this harness change; exact tested package hashes are in
the per-run metadata. The native-only diagnostics change still needs the next Android build.
The AMD stop latch and read-only dependency boundary are unchanged.
