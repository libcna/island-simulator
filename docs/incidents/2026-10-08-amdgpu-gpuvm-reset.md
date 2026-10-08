# 2026-10-08 AMDGPU GPUVM Fault and Desktop-Session Loss

## Status

**Open, critical.** The initiating workload and process are reconstructed exactly. The first GPUVM
fault has not yet been reproduced under the required fail-closed kernel-log protocol, so the
userspace operation that generated the invalid GPU access is not yet proven. This document must
not be read as a root-cause closure.

No CNA or Sharp Runtime file was changed during this investigation. No new hardware-GPU workload
has been launched since the incident was reported.

## Impact

At 2026-10-08 01:12:47 CEST, AMDGPU attributed multiple GFXHUB virtual-memory faults to
`island-simulator` PID 3497873 and Mesa submission thread TID 3497891. The GFX ring timed out ten
seconds later. Per-queue recovery failed, the kernel performed a MODE2 reset, VRAM was lost, and
the GNOME graphical session terminated. The machine remained at the graphical login screen while
unattended for approximately sixteen hours.

Observed fault pages included `0x7000`, `0xf000`, `0x2b000`, `0x5f000`, and `0x63000`. These low
addresses are consistent with an invalid or unmapped GPU address, but the addresses alone do not
identify whether the source was application state, Mesa/radeonsi command generation, or an AMDGPU
VM/driver defect.

## Exact launch reconstruction

The primary Codex rollout records this tool call at `2026-10-07T23:12:34.474Z`, which is
2026-10-08 01:12:34.474 CEST:

```sh
cd /rv/data/development/github.com/libcna/island-simulator
../cna/tools/platform/run_gpu_tests_private.sh build --output-on-failure -j8
```

The CNA wrapper did the following:

1. Started a private headless Weston compositor using its GL renderer.
2. Started a rootful Xwayland server on `DISPLAY=:2`.
3. Exported `WAYLAND_DISPLAY=cna-weston-3497623` and the wrapper's private
   `XDG_RUNTIME_DIR`.
4. Ran `ctest --test-dir build --output-on-failure -j8`.

CTest intentionally launched eight processes at once. Its first scheduled tests were:

1. `IslandGameStartupReport`
2. `IslandGameCapture`
3. `IslandGameButterflyCapture`
4. `IslandGameSeaTurtleCapture`
5. `IslandGameCrabCapture`
6. `IslandGameShipwreckCapture`
7. `IslandCoreTests`
8. `IslandGameRelocatableWorkingDirectory`

As slots became available, it launched `IslandGameTraversalBenchmark`, then
`IslandGameBenchmark`, followed by further capture processes. The first kernel fault occurred
approximately 12.5 seconds after the wrapper launch and approximately 6.5 seconds after the jungle
benchmark started.

The preserved CTest output further constrains the primary process timeline. Test 42 started after
`IslandGameCapture` completed at 3.93 seconds. PID 3497873 completed framework and GPU-content
initialization, announced `benchmark jungle warming 120 intervals, sampling 600 intervals with
VSync disabled`, completed its first draw, and reported a 1280x720 backbuffer. It never printed the
required `benchmark completed 600 samples` marker. CTest eventually failed it at 22.14 seconds,
after the private X server had already disconnected. The primary process was therefore executing
benchmark frames when the fault occurred; there is no evidence that it had entered normal
renderer/resource teardown. The later `X connection to :2 broken` line is a consequence of the
GPU reset/private-display failure, not the initiating operation.

The same `-j8` suite had completed successfully once at approximately 00:55 CEST. Therefore the
failure is load- or timing-sensitive; eight-way concurrency is an important condition but is not,
by itself, proof of the faulty component.

### Immediately preceding GPU work

The Android emulator was not part of the failing interval. The agent sent `adb emu kill` at
00:45:06.998 CEST and confirmed it was no longer running. The exact GPU-heavy sequence immediately
before the native run was:

1. A complete Chrome/WebGL 2 regression using the same island build ended successfully at
   01:11:48.161 CEST.
2. A Firefox ESR/WebGL 2 smoke run ended successfully at 01:12:29.927 CEST.
3. The native eight-client command above was issued at 01:12:34.474 CEST, 4.547 seconds after the
   Firefox process completed.
4. The first kernel fault was recorded at approximately 01:12:47 CEST.

The failing native run was the first complete `-j8` run after the palm realism pass added sparse
hanging dry fronds to two near-palm variants. The new geometry had already passed the dedicated
single-process `IslandGamePalmCapture` at 01:07:45 CEST, its capture validator, and both WebGL 2
browser suites. The earlier successful `-j8` aggregate predated that change. This makes the palm
change a required isolation variable, but does not establish it as the cause: it adds only static,
validated indexed geometry to the same existing palm buffers/draw path, and later identical jungle
workloads completed successfully.

The repository `HEAD` at launch was `424357e` (`feat: strengthen large shipwreck silhouette`,
committed at 01:05:45 CEST). The dry-frond work was present as an uncommitted worktree change; its
eventual source commit is `5107d11`. Commits `5107d11`, `1eb0319`, and `7accd10` were made only
after the graphical login recovered and therefore must not be mistaken for the exact initiating
source state.

## PID and workload map

| PID | Exact workload | Disposition |
|---:|---|---|
| 3497873 | `island-simulator --benchmark build/island-benchmark-jungle.json --viewpoint jungle` | Primary process named by the first AMDGPU GPUVM fault; no userspace coredump |
| 3497856 | `island-simulator --benchmark build/island-benchmark-traversal.json --viewpoint traversal` | SIGABRT at 01:13:01 in present/swap after the reset began |
| 3498035 | `island-simulator --capture build/island-ground-capture.ppm --viewpoint ground` | SIGABRT at 01:13:01 during GL context lease restoration after reset |
| 3498052 | `island-simulator --capture build/island-frond-capture.ppm --viewpoint fronds` | SIGABRT at 01:13:01 during GL context handling after reset |
| 3498053 | `island-simulator --capture build/island-grass-capture.ppm --viewpoint grass` | SIGABRT at 01:13:01 during GL context handling after reset |

PID 3497873 was CTest test 42, `IslandGameBenchmark`. It rendered the fixed jungle viewpoint,
disabled vertical synchronization, warmed up for 120 frame intervals, and was collecting 600
benchmark samples. It used the `OPENGL33` CNA renderer implemented by EasyGL, the SDL3 platform,
X11/GLX through the private Xwayland server, and Mesa/radeonsi on the AMD Radeon 780M. Its own
startup output identified OpenGL 4.6 Core Profile, Mesa `25.0.7-2+deb13u1`. The build cache records
`CNA_GRAPHICS_RENDERER=OPENGL33` and `CNA_PLATFORM=SDL3`.

The launch command did not set Mesa, Gallium, or AMD debug/driver overrides. The wrapper changed
the display/runtime variables described above. The ordinary inherited session had `DISPLAY=:0`,
`WAYLAND_DISPLAY=wayland-0`, and `XDG_RUNTIME_DIR=/run/user/1000` before the wrapper isolated the
test display.

## Meaning of `island-sim:cs0`

`island-sim:cs0` is not a game thread and is not created by CNA. The game contains no
`std::thread`, `std::jthread`, or asynchronous render worker. In Mesa 25.0.7:

- radeonsi's AMDGPU winsys initializes one queue named `cs` in
  `src/gallium/winsys/amdgpu/drm/amdgpu_winsys.c`;
- Mesa's `src/util/u_queue.c` formats worker names from the process name, queue name, and worker
  index, subject to Linux's 15-character task-name limit;
- `src/gallium/winsys/amdgpu/drm/amdgpu_cs.cpp` queues command submission to that worker.

TID 3497891 was therefore Mesa/radeonsi command-submission worker zero for PID 3497873. Kernel
attribution proves that this process submitted the faulting command stream; it does not identify
the high-level game draw that produced it.

## Evidence and reconstruction sources

Evidence retained outside the repository:

- primary agent rollout:
  `/home/robertvokac/.codex/sessions/2026/10/05/rollout-2026-10-05T16-58-43-01a10c93-00dd-7150-b100-cc25f7f7b4dd.jsonl`;
- coredumps under `/var/lib/systemd/coredump/` for PIDs 3497856, 3498035, 3498052, 3498053,
  Xwayland 3497684, Weston 3497633, and GNOME Shell 2922;
- the original primary process has no coredump;
- `/tmp/cna-wl-sfF758/compositor.log` belongs to the unsafe **post-reset** `-j2` retry, not the
  initiating run. It records Radeon 780M/Phoenix, Mesa 25.0.7, kernel 6.12.107, and
  `amdgpu_cs_ctx_create2 failed (-13)` after VRAM/context loss;
- the initiating wrapper removed its temporary compositor directory during cleanup because no
  keep-log path had been configured;
- the user-supplied kernel excerpt is authoritative until this account obtains read-only kernel
  journal access.

The coredump stacks in `SwapBuffers`, `MakeCurrent`, `RestoreBinding`, and `ClearCurrent` begin at
the MODE2 reset timestamp. They are evidence of collateral context loss, not evidence that those
functions triggered the first fault.

### Unmonitored runs after the graphical login recovered

Before the operator reported the overnight kernel evidence at 17:21:31 CEST, the continuing agent
did more GPU work after the new login session appeared. These commands and results are preserved
because they constrain reproducibility, but they had no before/after kernel-journal gate and are
**not** incident qualification:

- `../cna/tools/platform/run_gpu_tests_private.sh build --output-on-failure -j2` began at
  17:10:00 CEST and completed all 46 tests successfully at 17:11:38 CEST;
- the Release jungle benchmark then completed twice in isolation, each with 120 warm-up and 600
  sampled intervals, at 17:11:49 and 17:12:04 CEST. Both rendered the same stable scene counts as
  PID 3497873: 89 submissions, 120 palms (37 near), 109 canopy trees, and 2,932 grass clusters.

No further hardware-GPU workload has been launched since the incident report arrived. The two
isolated completions and the `-j2` completion show that neither the fixed jungle scene nor the palm
geometry fails deterministically; they do not show that the kernel recorded no silent GPUVM fault.

## Unsafe post-fault action

At 01:13:09 CEST the agent incorrectly characterized the event as an infrastructure/context reset
and asserted that it was not geometry-related without evidence. At 01:13:30 it launched:

```sh
../cna/tools/platform/run_gpu_tests_private.sh build --output-on-failure -j2
```

The retry began on `WAYLAND_DISPLAY=cna-weston-3498567`. The session disappeared before the run
completed. This retry was unsafe because the kernel had just reset the GPU and no kernel-log gate
was in place. It must not be treated as validation.

## Investigation findings so far

### Application resource and threading audit

- All update, render, content creation, and renderer destruction occurs on the main game thread.
- No island-simulator code acquires CNA EasyGL thread-context leases from a worker thread.
- The fixed jungle benchmark does not intentionally destroy scene renderers or their static GPU
  resources while sampling.
- Generated indexed terrain, ocean, shoreline, sky, palms, jungle, natural props, and monkey
  geometry is checked before upload for a non-empty complete triangle list, the 16-bit vertex
  limit, and every index being in range.
- Wildlife dynamic vertex buffers are sized from the maximum population multiplied by the maximum
  near-LOD geometry size. Their uploads use CNA's checked `SetData` API.
- Palm, jungle, and natural-prop instance buffers grow before upload, are rebound for each draw,
  and are explicitly unbound after each subsystem's draw sequence.
- Grass is the only instanced path using a non-zero instance-buffer offset. Its LOD split is
  unit-tested to keep every count non-negative and make offset plus count equal the cell's exact
  patch count. Palm, jungle, and natural props use offset zero and draw no more than the capacity
  allocated before upload.
- CNA EasyGL returns `false` from `RequiresManagedBufferedDrawRangeValidationEXT()`, so CNA forwards
  buffered instance ranges to GL rather than checking them. island-simulator now validates every
  one of its four instanced paths immediately before upload/draw and throws before GL if a capacity,
  offset, or count would read out of bounds. This is defensive regression protection; the audited
  incident ranges are valid, so it is not presented as the root-cause fix.
- The benchmark scene uses deterministic placement and a fixed camera, so visible instance counts
  stabilize rather than continually reallocating resources.
- The primary process did not reach its benchmark-completion marker, which rules out normal
  end-of-benchmark destruction as the operation immediately preceding the first fault.
- CNA EasyGL's frame lease is acquired by `GraphicsDeviceManager::BeginDraw` and released after
  `Present` by `EndDraw`; no game-side cross-thread lease use was found.

This audit has not found application undefined behaviour or a resource-lifetime race that explains
the fault. That is a negative finding, not proof that the application is correct.

### CPU, sanitizer, and software-GL evidence

- `island_core_tests` passed under the native AddressSanitizer/UndefinedBehaviorSanitizer build;
  LeakSanitizer alone had to be disabled because the sandbox prohibits its ptrace mechanism. No
  ASan or UBSan finding occurred.
- Six additional non-rendering report, asset, and command-line tests passed under that sanitizer
  configuration.
- Cppcheck completed over all 62 first-party game translation units. Its only first-party lifetime
  report was a false positive around references obtained from `std::span` accessors whose backing
  wildlife vectors remain alive; no explanatory ownership, bounds, or concurrency defect was
  found.
- An instrumented four-frame fixed-jungle smoke run completed on Mesa llvmpipe 25.0.7 with
  `LIBGL_ALWAYS_SOFTWARE=1`, `GALLIUM_DRIVER=llvmpipe`, and KHR_debug enabled. Every render pass
  completed, no GL error was returned, and no high/medium/low KHR_debug message was emitted. A
  second run after adding the instance-range guards produced the same clean result with llvmpipe
  capped at eight threads. These runs did not access the AMD GPU and are not hardware
  qualification.

### Native EasyGL context-loss recovery audit

This recovery path is separate from the first fault. Current CNA EasyGL cannot reliably detect and
cleanly terminate after an unsolicited native desktop GPU reset:

- `RequestedGlContext()` in
  `modules/renderers/easygl/src/EasyGLRenderer.cpp` leaves `robustAccess` and
  `loseContextOnReset` false.
- `Sdl3GlContext::CreateContext()` faithfully maps those false values to no
  `SDL_GL_CONTEXT_ROBUST_ACCESS_FLAG` and no `SDL_GL_CONTEXT_RESET_LOSE_CONTEXT` request.
- On WebGL, EasyGL installs browser context-loss callbacks. On native desktop GL, the only code
  that calls `metagl::NotifyContextLost()` is the explicit debug simulation; no production path
  polls `glGetGraphicsResetStatus()` or translates a native reset into MetaGL's lost state.
- `EasyGLRenderer::Present()` skips work only after `metagl::IsContextLost()` has already become
  true. It otherwise calls `SDL_GL_SwapWindow`. The post-reset coredumps show Mesa aborting inside
  GLX operations before CNA could receive and translate a normal error return.
- `contextRecoveryEnabled` retains/rebuilds registered resources for a known loss; it does not
  detect an unsolicited native loss.

Therefore the collateral clients could not use EasyGL's existing recovery registry after MODE2
VRAM loss. This is currently classified as an unsupported native recovery capability rather than
a confirmed CNA implementation bug, so it is documented here rather than in `BUGS.md`. The public
game API exposes no supported way for island-simulator to change EasyGL's native context creation
attributes, and a process cannot recover gracefully if Mesa aborts inside GLX before returning.
CNA remains read-only under this project's dependency boundary.

### Current fault model

The evidence supports at least two distinct problems:

1. **Primary invalid GPU access — unresolved.** PID 3497873 submitted an AMD GFX command stream
   that accessed low unmapped GPU addresses. Serial reproduction with pass markers and kernel-log
   correlation is required to decide between application/CNA state and Mesa/radeonsi/AMDGPU VM
   behaviour.
2. **Kernel/driver recovery failure — independently real.** The per-queue reset failed through
   MES, forcing a MODE2 reset and VRAM loss. Other innocent GL clients and GNOME then lost their
   contexts. A userspace client should not be able to end the entire desktop session this way,
   even when it submits a workload that exposes a driver bug.

There is no evidence yet for a CNA or Sharp Runtime defect, so `BUGS.md` has no incident entry. If
controlled reproduction isolates a framework defect, it must be recorded there; CNA and Sharp
Runtime remain read-only.

## Containment and attribution added

- Every process now writes a flushed per-PID runtime log containing PID, main TID, exact command
  line, parsed mode/viewpoint/outputs, CNA renderer, graphics API and implementation strings,
  SDL platform/video backend, adapter identity, relevant Mesa/AMD/display environment, and the
  Linux task inventory after first present.
- `ISLAND_GPU_DIAGNOSTICS=1` enables KHR_debug where available, a GL-error checkpoint, and flushed
  begin/end markers for every high-level render pass. It does not add waits, fences, or `glFinish`.
- CTest display tests share the `island_gpu_display` resource lock by default. CPU tests still run
  in parallel. Deliberate stress qualification can configure
  `ISLAND_ALLOW_PARALLEL_GPU_TESTS=ON` only after serial qualification is clean.
- `tools/run_gpu_incident_qualification.sh` launches exactly one named workload, preserves its
  runtime/compositor/kernel evidence, and refuses to start unless the invoking account can read an
  actual kernel journal record and acquire a cursor. A live 200 ms journal poll terminates only the
  harness-owned process group on the first new AMDGPU GPUVM fault, GFX ring timeout, VRAM loss, or
  GPU reset; a final cursor scan closes startup/exit races and stops the campaign.

## Required safe qualification sequence

The current account cannot read kernel journal entries: `journalctl -k` returns no kernel record,
and membership in `systemd-journal`/`adm` is absent. Hardware-GPU qualification remains blocked
until read-only access is granted and a new login session picks up the group membership.

On this Debian host the required group exists. The operator can grant the narrow read-only access
with:

```sh
sudo usermod -aG systemd-journal robertvokac
```

A complete logout/login (or a new development-agent session created after group membership takes
effect) is required. The qualification harness will continue to refuse hardware work until it can
read an actual `_TRANSPORT=kernel` record and obtain a journal cursor; a successful but empty
`journalctl` invocation is deliberately insufficient.

Once access exists, run each step separately and inspect its preserved evidence before proceeding:

```sh
tools/run_gpu_incident_qualification.sh build smoke
tools/run_gpu_incident_qualification.sh build capture-jungle
tools/run_gpu_incident_qualification.sh build benchmark-jungle
tools/run_gpu_incident_qualification.sh build benchmark-traversal
tools/run_gpu_incident_qualification.sh build capture-ground
tools/run_gpu_incident_qualification.sh build capture-fronds
tools/run_gpu_incident_qualification.sh build capture-grass
```

Repeat the original jungle benchmark enough times to cover the timing-sensitive window. Then run
the other fixed viewpoints serially. Only after every serial result has zero new GPUVM faults,
ring timeouts, resets, context-loss crashes, and high-severity GL diagnostics may concurrency be
increased to two and eventually to the original eight clients. The final eight-client run must use
an explicit incident stress configuration and the same kernel-log gate; an ordinary developer
`ctest -j8` remains contained.

## Resolution criteria

Do not mark this incident resolved until all of the following are true:

- the pre-fault operation is identified by a controlled reproducer or sufficient driver evidence;
- the responsible application defect is fixed, or a proven upstream Mesa/AMDGPU defect has a
  narrowly justified game-side avoidance and an upstream-ready report;
- the original jungle benchmark repeatedly passes on hardware;
- every related capture and the traversal benchmark passes serially;
- the relevant EasyGL tests pass;
- the former concurrency level passes a controlled stress run;
- every run has a preserved kernel-log delta with zero GPUVM faults, GFX ring timeouts, and resets;
- collateral context-loss behaviour is reported separately from the primary trigger.
