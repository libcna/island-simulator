# 2026-10-08 AMDGPU GPUVM Fault and Desktop-Session Loss

## Status

**Open, critical.** The parent launch is reconstructed exactly. The jungle benchmark is the
strongest candidate for PID 3497873, but its PID-to-command mapping is not directly recorded in
the retained evidence. A serial jungle benchmark reproduced GPUVM faults under the required
kernel-log protocol at 18:31 CEST. The userspace operation that generated the invalid GPU access
is not yet proven. This document must
not be read as a root-cause closure.

A concise, unsent [upstream triage draft](upstream-triage-draft.md) lists the retained
attachments, known attribution limits, and specific recorder/recovery questions. It is
not a ready-to-run reproducer or a claim that Mesa has been proven responsible.

The October 9 [Debian patch-scope audit](debian-mesa-patch-audit.md) now checks the
published `25.0.7-2+deb13u1` patch series and build rules against the recorder audit.
None changes the reviewed ddebug/radeonsi recorder/descriptor paths. This does not
establish full source/binary equivalence or fill the missing pre-fault IB/descriptor
evidence, and does not justify another unchanged hardware reproduction.

No CNA or Sharp Runtime file was changed during this investigation. After journal access was
granted, a serial hardware reproduction triggered another GPUVM fault; see the update below.

## Serial reproduction at 18:31 CEST

The operator added `robertvokac` to `systemd-journal`. Although the running session retained its old
groups, `sg systemd-journal -c ...` granted read-only journal access without logging out. The
original incident's complete kernel output is now preserved as `evidence/2026-10-08/original-kernel.txt`.
Searching journal entries with `_PID=3497873` yielded no record, so the historical PID mapping
remains unproven.

Three workloads were run individually with the supervisor:

| Start (CEST) | Workload | PID | Result |
|---|---|---:|---|
| 18:31:05 | `smoke` | 3592624 | Four frames, zero new kernel records, no GL errors |
| 18:31:26 | `capture-jungle` | 3592878 | Completed, zero new kernel records, no GL errors |
| 18:31:41 | `benchmark-jungle` | 3593099 | GPUVM fault, supervisor stopped its process group |

The exact failing parent command was:

```sh
sg systemd-journal -c 'tools/run_gpu_incident_qualification.sh build benchmark-jungle'
```

Its evidence directory is
`build/gpu-incident-evidence/20261008T163141.853965Z-3593014-benchmark-jungle/`.
The per-PID log proves this time that the faulting process ran `--benchmark` with `--viewpoint
jungle`, using OPENGL33/EasyGL/SDL3/X11/GLX, Mesa 25.0.7 radeonsi on Radeon 780M. At
18:31:47.174644 the kernel reported a GFXHUB fault, PASID 16000, PID 3593099, submission TID
3593107. The first address was `0x17000`; the first protection status was `0x00101031`, client
TCP. The application completed CPU submission through frame 448 before termination; no GL error
or high-severity KHR_debug diagnostic preceded the fault. These asynchronous CPU markers do not
identify the exact GPU draw that faulted.

The supervisor finished at 18:31:48.334 and latched `QUALIFICATION_STOP.json`. Both wrapper and
game processes are confirmed absent. At 18:31:57.214690 the kernel nevertheless reported
`ring gfx_0.0.0 timeout, but soft recovered`. The preserved kernel interval through 18:33:00 has
no MODE2 reset or VRAM-loss message. Killing a process cannot cancel already-submitted GPU work.
No further hardware workload was launched after this fault. Multi-client concurrency is therefore
not necessary to reproduce the invalid workload, though its effect on full recovery is unresolved.

The kernel produced `/sys/class/devcoredump/devcd3/data`, readable only by root and subject to
automatic expiry. The operator's root read at approximately 18:36 returned `No such device`, but
it copied **3,436,544 bytes** before expiry. The file is ASCII text and ends mid-record; it is a
partial dump, not an empty or complete one. The retained prefix contains hardware/firmware
identity, the `0x17000` fault, protection status `0x101031`, graphics registers, and the complete
2,048-dword `gfx_0.0.0` ring before ending partway through the much larger MES ring. Register and
ring snapshots expose command-buffer addresses, but do not include the pointed-to userspace
indirect-buffer contents or prove which high-level draw generated the bad access.

The original partial file remains at `build/gpu-incident-evidence/amdgpu-devcd3.bin`. A lossless
compressed copy and the complete supervisor/runtime evidence are versioned in the incident
evidence directory, with hashes and provenance in its README. No new GPU execution was needed
to inspect or archive them.

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
approximately 12.5 seconds after the wrapper launch. CTest's per-test durations do not establish
the exact wall-clock start time of each child after private-display startup.

The preserved CTest output further constrains the candidate workload timeline. Test 42 started after
`IslandGameCapture` completed in 3.93 seconds. The jungle benchmark completed framework and GPU-content
initialization, announced `benchmark jungle warming 120 intervals, sampling 600 intervals with
VSync disabled`, completed its first draw, and reported a 1280x720 backbuffer. It never printed the
required `benchmark completed 600 samples` marker. CTest eventually failed it at 22.14 seconds,
after the private X server had already disconnected. This is consistent with a fault during
benchmark frames, but cannot establish PID 3497873's exact operation without a direct PID mapping.
The later `X connection to :2 broken` line is a consequence of the
GPU reset/private-display failure, not the initiating operation.

The same `-j8` suite had completed successfully once at approximately 00:55 CEST, before the
dry-frond source change discussed below. Concurrency, timing, and that source change remain
isolation variables; the earlier pass alone does not prove a race or identify the faulty component.

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
| 3497873 | Most likely `island-simulator --benchmark build/island-benchmark-jungle.json --viewpoint jungle`; direct command-line record missing | Primary process named by the first AMDGPU GPUVM fault; no userspace coredump |
| 3497856 | `island-simulator --benchmark build/island-benchmark-traversal.json --viewpoint traversal` | SIGABRT at 01:13:01 in present/swap after the reset began |
| 3498035 | `island-simulator --capture build/island-ground-capture.ppm --viewpoint ground` | SIGABRT at 01:13:01 during GL context lease restoration after reset |
| 3498052 | `island-simulator --capture build/island-frond-capture.ppm --viewpoint fronds` | SIGABRT at 01:13:01 during GL context handling after reset |
| 3498053 | `island-simulator --capture build/island-grass-capture.ppm --viewpoint grass` | SIGABRT at 01:13:01 during GL context handling after reset |

CTest test 42, `IslandGameBenchmark`, is the leading candidate for PID 3497873. The scheduling
order and its position between the known traversal and later capture PIDs support this inference.
However, PID ordering is not a process-to-command record; the original application did not print
its PID, the CTest output does not contain 3497873, and its core is absent. Earlier revisions of
this report and progress messages stated this mapping as exact; that certainty was unsupported.
Kernel/audit/process-accounting evidence is still required to prove or correct the association.

Test 42 rendered the fixed jungle viewpoint, disabled vertical synchronization, warmed up for 120
frame intervals, and requested 600 benchmark samples. The suite used the `OPENGL33` CNA renderer
implemented by EasyGL, the SDL3 platform, X11/GLX through private Xwayland, and Mesa/radeonsi on the
AMD Radeon 780M. The benchmark startup output identified OpenGL 4.6 Core Profile, Mesa
`25.0.7-2+deb13u1`. The build cache records
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

The extracted primary CTest output and the post-reset retry's compositor log are also preserved
verbatim in `evidence/2026-10-08/`, with provenance and SHA-256 hashes. Their original temporary
copies were left intact.

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

Before journal access was granted, no further hardware-GPU workload was launched. The two
isolated completions and the `-j2` completion show that neither the fixed jungle scene nor the palm
geometry fails on every run. The source also evolved after the incident, so these runs are not an
exact historical-binary reproduction; they do not show that the kernel recorded no silent GPUVM fault.

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
- The jungle benchmark did not print its completion marker. This supports investigation of
  steady-state frame operations, but neither missing buffered output nor the inferred PID mapping
  rules out a resource teardown fault in the actual primary process.
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

### Full software benchmark and offline GL bounds audit

After the serial hardware failure, one full jungle benchmark was traced on **CPU-only llvmpipe**,
with `LIBGL_ALWAYS_SOFTWARE=1`, `GALLIUM_DRIVER=llvmpipe`, `LP_NUM_THREADS=8`, and a private Weston
pixman compositor. The per-process implementation string confirms llvmpipe. No further AMD
hardware workload was submitted. The run completed 600 measured frames after warm-up (721 swaps
in the trace including startup) and emitted no GL-error or warning-or-higher KHR_debug message.
The kernel interval after this software run contained no entries.

The trace is retained at `build/gpu-incident-evidence/software-jungle-trace/jungle.trace`, SHA-256
`044785478efbf2b3b01df138676b01ef717f3a03debe67b51c83e11ca14ebf55`. The complete software evidence
directory occupies approximately 83 MB. This trace is useful unique incident evidence and must
not be removed as an obsolete build artifact. Its smaller logs/report are archived in the
versioned evidence directory.

`tools/audit_gl_vertex_trace.py` decodes the trace without executing GL. It tracks uploaded buffer
sizes, actual index values, VAO bindings, attribute strides/offsets, instance divisors, and base
vertices, and checks every enabled attribute's maximum fetch address. It refuses unsupported draw
or buffer-mutation APIs and missing index contents instead of silently certifying them. Eight
CPU-only regression tests exercise valid boundaries, bad indices, index/attribute overruns,
nonzero instance offsets/divisors, updated index contents, and unsupported draws.

The audit found zero out-of-range fetches in **64,169 draws and 389,340 attribute ranges**. This
narrows the straightforward application-buffer-overrun hypothesis for the software trace. It does
not validate shader semantics, textures, GL object lifetime, GPU synchronization, or radeonsi's
hardware descriptors, and is not proof that the hardware workload was valid.

Re-run only the offline analysis with:

```sh
python3 tools/audit_gl_vertex_trace.py \
  build/gpu-incident-evidence/software-jungle-trace/jungle.trace \
  --output build/gpu-incident-evidence/software-jungle-trace/vertex-audit.json
```

### Offline program, texture, and allocation inventory

`tools/inspect_gl_trace_state.py` now inspects the same trace without GL execution. All 28 observed
shader compile checks and 14 program link checks succeeded. All 64,169 draws use a successfully
linked program. The only sampled program/texture pair is program 42 / texture 2 (2,884 draws);
its sampler unit has an allocated level-zero image, and that texture is not attached to the draw
framebuffer. These checks do **not** establish mip completeness or shader arithmetic validity.
Eight CPU-only regressions cover missing/reused texture storage, sampler units, framebuffer
feedback versus a read-only framebuffer, failed linking, unsupported sampler declarations, and
deletion timing. The report is `evidence/2026-10-08/software-state-inspection.json`.

No non-shader GL deletion occurs between the first and last swaps. Initialization contains
capability-probe deletions; teardown deletes the remaining objects after swap 721. Deletion
timing alone is not a complete lifetime audit: OpenGL must retain backing storage referenced by
pending commands, and storage can change without deleting an object name.

There are **38,568 `glBufferData` calls requesting 320,319,980 bytes cumulatively**, not 320 MB of
simultaneously resident memory. Most calls are small 1–8 KiB uploads; 721 calls orphan a 20,736-byte
allocation and 466 orphan a 471,744-byte allocation. CNA's
`EasyGLVertexBufferRenderer::uploadWithOptions` uses legal `glBufferData` replacement/orphaning
followed by `glBufferSubData` for partial uploads. Churn is a relevant driver-lifetime path to
investigate, not evidence of incorrect application synchronization or a reason to add sleeps.
Mesa 25.0.7's `si_buffer.c:si_invalidate_buffer` and
`si_descriptors.c:si_rebind_buffer` are the corresponding read-only source-audit starting points;
no specific defect has yet been established there.

An offline `apitrace dump --thread-ids` check of `glXMakeCurrent|glXMakeContextCurrent` reports
2,132 calls, all on trace thread `@0`. This narrows the cross-thread context-ownership hypothesis
for this software execution; it does not inspect Mesa's internal submission worker or establish
the hardware failure's exact context sequence.

Reproduce the additional CPU-only inspection with:

```sh
python3 tools/inspect_gl_trace_state.py \
  build/gpu-incident-evidence/software-jungle-trace/jungle.trace \
  --output build/gpu-incident-evidence/software-jungle-trace/state-inspection.json
python3 tests/InspectGlTraceStateTests.py
```

The incremental native build passed with the shared ccache and `-j8`. The three CPU-only incident
CTest targets passed (15 supervisor, eight vertex-auditor, and eight state-inspector cases; 31
total). No graphics runtime was launched for this extension, and the existing failure latch was
not cleared. The preserved build evidence remains approximately 92 MB; nothing was deleted.

Next: correlate the preserved GFX ring with Mesa's descriptor/storage-renaming paths, and audit
shader input values (especially skinning indices and finite transforms) not covered by byte-range
checks. A useful hardware experiment must discriminate a specific hypothesis and capture the
driver's pre-fault command state, rather than merely reproduce another kernel error. Mesa's
`AMD_DEBUG=check_vm` implementation reads `dmesg`; journal-group membership alone does not provide
that access, so it must not be assumed to capture the necessary state. Keep the hardware
qualification failure latch in place. No hardware test or new root command is required for the
offline work above; the incident is still unresolved.

### Bounded follow-up and operator-authorized development continuation

A further offline scan of the original llvmpipe trace checked 1,152,158 floating-point uniform
values; none was NaN or infinite. The traced skinning vertex shader declares `uBones[72]` and
indexes it through the four vertex bone indices. The game constructs a 14-bone rig and validates
hierarchy/clip sizes; the uniform scan is **not** a complete proof of skin-index validity.
Read-only inspection of Mesa's `si_alloc_resource` shows that allocation failure returns before
replacing the old backing buffer, while `si_invalidate_buffer` calls `si_rebind_buffer` after
replacement. No concrete null-address defect was established by this source review.

The operator subsequently authorized bounded diagnosis followed by continued wildlife and visual
work if no cause emerged. New feature work therefore proceeds with CPU tests and forced llvmpipe
plus pixman capture checks only. The failure latch remains, and the original trace, partial dump,
pre-change binaries/log provenance, and kernel evidence must not be discarded. Feature builds do
not constitute an incident fix or post-incident hardware qualification. A future reproduction
must identify its exact source revision and distinguish new scene changes from the preserved
pre-feature incident workload.

### Offline skinning input follow-up

`tools/audit_gl_skin_trace.py` now decodes the retained software trace without replaying any GL
commands. It tracks buffer contents, storage generations, vertex layouts and the actual element
selection for the explicit-layout `uBones` shader. All four index/weight lanes are checked,
including zero-weight lanes. Unknown layouts, normalization, uninitialized input and unsupported
buffer operations fail closed. Ten synthetic regression cases cover valid input, storage
replacement, invalid indices, non-finite/unnormalized weights, missing data and state changes.

The 721-swap trace contains 64,169 draws, of which 2,884 use skinning. Its 1,445 distinct fetched
skin vertex versions have valid weights and integral bone indices; the highest index is 13,
inside both the game's 14-bone rig and the shader's 72-matrix array. No input violation was found.
This is evidence about the software trace only, not a proof about the failing hardware command
stream, completeness of uploaded palettes, shader outputs, or driver storage renaming.

```sh
python3 tests/AuditGlSkinTraceTests.py
python3 tools/audit_gl_skin_trace.py \
  build/gpu-incident-evidence/software-jungle-trace/jungle.trace \
  --output build/gpu-incident-evidence/software-jungle-trace/skin-audit.json
```

At the 20:45 CEST follow-up, kernel journal access through `sg systemd-journal` was verified again.
`/sys/class/devcoredump` no longer contained `devcd3`; no new root read can retrieve that expired
instance. No hardware workload was launched, no latch was cleared, and no original evidence was
removed. A replacement dump is not justification for intentionally triggering another GPU fault.
Next safe analysis is palette upload completeness and the descriptor/storage-renaming path;
hardware reproduction still requires a specific hypothesis and pre-armed evidence plan.

The follow-up incremental Linux build used shared ccache and two workers. All 18 non-display
CTest targets passed; capture/startup validators in that set can read earlier artifacts and do
not imply a new runtime pass. The post-test kernel journal interval since 20:45 CEST had no
entries. No graphics process was launched during this follow-up, so this result is not the
required clean-hardware acceptance test. The incident remains open.

The subsequent palette extension checks observed `uBones` uploads and requires every referenced
slot to have received finite matrix data before each draw, including when vertex data is cached.
All 2,884 palette uploads (646,016 floats) pass, and no skin draw references a slot without a
finite upload. Four additional regressions cover missing/partial palettes, a non-finite update
after a cached draw, and invalid upload length; all 14 skin-auditor tests pass. The first test
attempt exposed a keyword collision in the test helper, corrected before running the trace.
The separate `software-skin-palette-audit.json` preserves the extended result without replacing
the earlier report. This still does not establish hardware matrix values, post-transform
arithmetic, Mesa descriptor correctness, or the cause of the GPUVM fault. The bounded offline
checks have not justified another AMD reproduction; continue software-qualified visual work
while retaining the evidence and hardware stop latch.

### Offline texture mip-completeness follow-up

The extended `tools/inspect_gl_trace_state.py` now tracks mutable 2D image dimensions/formats,
texture base/max levels, minification filters, and separate sampler-object overrides. Deleting
and reusing a texture or sampler name clears the corresponding state. Unsupported storage and
parameter APIs fail closed. The deliberately narrow normalized-color path is not a general GL
validator: it does not validate texel contents, hardware descriptors, shader arithmetic, or
context/device lifetime.

All 2,884 textured draws in the retained 721-swap software trace use texture 2 and sampler 1.
The sampler uses `GL_LINEAR_MIPMAP_LINEAR` (9987), but the texture has base level 0 and max level
0. Its required image chain is complete. A mipmap filter alone is therefore not evidence of a
missing-mipmap defect. Sampler state overrides texture sampling parameters, while texture
base/max levels still bound the image chain; see the
[Khronos sampler documentation](https://wikis.khronos.org/opengl/Sampler_Object).

The new report is `evidence/2026-10-08/software-mip-state-inspection.json`; earlier reports remain
unchanged. Twenty-one CPU-only state-inspector tests pass, including missing/wrong-sized/wrong-format
mips, non-power-of-two chains, nonzero base levels, max-level truncation, sampler overrides and
deletion, and the distinction between a floating LOD clamp and an image max-level limit.

```sh
python3 tests/InspectGlTraceStateTests.py
python3 tools/inspect_gl_trace_state.py \
  build/gpu-incident-evidence/software-jungle-trace/jungle.trace \
  --output build/gpu-incident-evidence/software-jungle-trace/mip-state-inspection.json
```

No graphics workload or rebuild was needed. The kernel journal query for 21:30–21:39 CEST
returned no entries at the time of this follow-up. This is not a hardware acceptance test and
does not resolve the original incident. The failure latch remains intact. Next incident work is
to correlate storage replacement and descriptor rebinding in the pinned CNA/Mesa paths against
the preserved ring evidence. Another hardware experiment is not justified until it tests a
specific hypothesis with pre-armed evidence collection; merely obtaining a replacement for the
expired `devcd3` is insufficient reason to risk another desktop reset.

### Bounded buffer-storage source follow-up

Read-only inspection of the pinned CNA export and the clean local Mesa `mesa-25.0.7` checkout
followed the actual replacement path further. `EasyGLVertexBufferRenderer::uploadWithOptions`
(`EasyGLRenderer.cpp:8951`) binds the VBO, allocates capacity-sized storage for `Discard`, and
uploads the written prefix at offset zero. The retained vertex audit already checked the fetched
ranges, so no application out-of-range fetch was established here.

Mesa `si_buffer.c:si_alloc_resource` installs the newly allocated BO/address before dropping its
resource reference to the old BO. `si_invalidate_buffer` calls `si_rebind_buffer`; the latter's
vertex-buffer branch (`si_descriptors.c:1652`) marks vertex descriptors dirty and adds the current
BO to the submission resource list. The dirty branch in `si_state_draw.cpp:1828` regenerates the
descriptors and checks failure of descriptor-upload allocation before emitting its address.
These are expected lifetime mechanisms, not proof that every concurrent path is correct.
`si_invalidate_buffer` does not propagate a failed `si_alloc_resource`, but the allocator retains
the old BO on failure; no allocation-failure evidence ties that path to this incident. Do not
label it the cause or silently patch around it.

The preserved ring lacks the pointed-to userspace indirect buffers and descriptor bytes, so this
review cannot correlate the low fault address with an actual emitted vertex descriptor. No
framework/driver change or new hardware experiment is justified by this bounded review. Retain
the unresolved incident and resume operator-authorized software-qualified visual work.

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
   that accessed low unmapped GPU addresses. The serial reproduction establishes that concurrent
   game clients are not required. Pass markers and a partial kernel device dump are now preserved;
   neither yet identifies the exact draw or decides between application/CNA state and
   Mesa/radeonsi/AMDGPU VM behaviour.
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
  actual kernel journal record and acquire a cursor. Its Python supervisor parses JSON journal
  records, preserving their cursors, boot identity, and timestamps. A live poll every 200 ms (plus
  journal command time) terminates the harness-owned process group on the first GPU fault, loss of
  journal access/continuity, operator interruption, or the 120-second workload deadline. Journal
  reads themselves have a three-second deadline. A final scan preserves the complete delta.
- The initial shell implementation was found to use `kill -TERM -- -PGID`, which Debian's dash
  rejects as `Illegal number: -`. Its polling path also swallowed journal errors. Both defects
  were discovered before hardware qualification and replaced with `os.killpg` supervision, bounded
  TERM/KILL cleanup, explicit monitoring-failure handling, and 15 CPU-only regression cases. Those
  tests verify termination of parent and child processes, including clients left behind by an
  exited wrapper, and retention of a live fault even when the final synthetic delta is empty.
- Failed qualification writes `gpu-incident-evidence/QUALIFICATION_STOP.json`; subsequent runs
  refuse to launch until the evidence is reviewed and the stop file is deliberately archived out
  of that location. An advisory build lock rejects concurrent supervisor invocations. GL errors
  and high-severity KHR_debug messages also fail qualification. This containment does not identify
  or fix the original GPUVM defect.

## Required safe qualification sequence

### Operator status check at 23:01 CEST

The no-launch preflight is available from the repository root:

```sh
sg systemd-journal -c 'tools/run_gpu_incident_qualification.sh build preflight'
```

At `2026-10-08T21:01:31.024Z`, it verified journal access, reported the existing serial-jungle
stop latch as active, and found no `devcd` entries. It did not launch graphics or read a device
dump. No new root action is needed. The stop latch SHA-256 remains
`c0bd170489321c8d497eae0f1211e1553e30aef9ee02ac01774269300e6ee6f1`.

Preflight exit zero means inspection completed, not GPU safety. It never clears a latch, acquires
a workload lock, starts a compositor/game, or authorizes hardware, even if the latch is absent.
Unreadable/invalid state is reported and returns nonzero. Dump entry names do not prove payload
availability. Seven additional CPU-only tests cover no-launch dispatch, payload-read prohibition,
latch preservation/invalidity/absence, journal denial and dump-interface absence/access failure.
All 30 supervisor tests and 21 non-display CTest groups pass (24.78 s for CTest). Existing capture
validators in that suite inspect saved artifacts; these are not new graphics runs. The Linux
instructions no longer recommend restarting the failed benchmark during the incident.

An additional read-only Mesa source check identified Gallium's driver debugger as a candidate
evidence path, **not a ready-to-run remedy**. In the local `mesa-25.0.7` source:

- `src/gallium/auxiliary/driver_ddebug/dd_screen.c:560` documents per-draw dump modes;
  `dd_draw.c:982` writes saved draw/log records, independent of the legacy kernel fault parser.
- `dd_draw.c:1135` detects hangs using driver/GPU fences; the default timeout path can lose a
  race with our stop-on-first-kernel-fault supervisor. It is not a guaranteed pre-fault capture.
- `dd_draw.c:1218` and `:1261` add deferred fences when hang detection is enabled; `flush` adds
  submission changes. These modes alter timing and must not be mistaken for a defect correction.
- The hang-report path calls `si_dump_debug_state` with device-status flags. In
  `src/gallium/drivers/radeonsi/si_debug.c:1030`, this includes external `umr` halt-wave commands.
  Do not enable that path casually on the owner's live GPU.
- Per-draw `always` output is potentially large; targeted draw attribution, actual installed-driver
  support, descriptor/IB content completeness and output bounds still need offline verification.

No debugger option, driver setting or hardware workload was activated. The next diagnostic task is
to determine whether a bounded pre-fault capture can retain the required command/descriptor state
without relying on expired sysfs data or the incompatible `check_vm` message parser. The primary
fault remains unexplained and no source-level GPU fix is claimed.

### Continuation checkpoint: evidence before another hardware experiment

On the 22:22 CEST continuation, `devcd3` was still absent and the persistent hardware stop latch
still pointed to the 18:31 jungle fault. No hardware workload was launched. The kernel journal
query since 22:18 CEST returned no entries; this CPU-only observation is not incident qualification.
The completed software beach capture at `build/software-captures/20261008T201808.445983Z-beach/`
passed image validation and had an empty kernel delta. It does not exercise radeonsi.

Future supervisor metadata now includes SHA-256, resolved path, size, inode/device and modification
time for the game executable, private-display runner, supervisor and optional CMake cache. A missing
optional cache is explicit rather than fabricated. Unreadable artifacts or replacement/modification
during hashing refuse launch. Hashing uses bounded memory and Python 3.10-compatible APIs. These
identities help distinguish a later feature build from the preserved incident workload, but do not
pin executable contents after the check or identify every dynamically loaded library. Do not build
concurrently with qualification. Per-process runtime diagnostics remain necessary.

The next hardware diagnostic is **not scheduled or authorized by this document**. Before proposing
one, record all of the following:

1. A specific hypothesis and how its possible outcomes discriminate application state from driver
   command generation. Repeating the unchanged benchmark only proves an already known fault.
2. The exact historical executable/source/configuration and launch environment; later wildlife and
   material changes must not silently substitute for the preserved incident workload.
3. A pre-armed, verified method to retain the failing userspace indirect buffers and descriptor
   state. The partial kernel ring alone lacks those referenced bytes. Inspect the exact Mesa debug
   path and its permissions before relying on it; journal access is not `dmesg` access.
4. A supervised single-process run with kernel monitoring, preserved evidence and the existing
   stop-on-first-fault policy. Killing the client cannot guarantee that submitted GPU work will not
   subsequently hang or reset the desktop; explain that residual risk before requesting permission.

No operator root command is currently needed. Do not repeat the expired sysfs read, clear the latch,
or provoke another GPU fault merely to obtain a fresh device dump. CNA and Sharp Runtime stay
read-only; the root cause and the hardware acceptance criteria remain unresolved.

Read-only preflight was repeated on 2026-10-09 at 02:08:03 UTC using
`sg systemd-journal -c 'tools/run_gpu_incident_qualification.sh build preflight'`.
Journal access is verified, the stop latch is active, and `/sys/class/devcoredump` has no
device-dump entries. No workload was launched. This verifies access/state only, not GPU safety.

#### Mesa diagnostic preflight finding: kernel-message format mismatch

Read-only inspection of the clean local `mesa-25.0.7` tag found a second obstacle to relying on
`AMD_DEBUG=check_vm`, beyond `dmesg` permissions. In `src/amd/common/ac_debug.c:103`,
`ac_vm_fault_occurred()` invokes `dmesg` and, for GFX9 and newer, requires the literal header
`VMC page fault`, then the address prefix `   at page` in the immediately following message.
The preserved original and serial kernel logs instead contain `[gfxhub] page fault`, an intervening
process attribution message, and `in page starting at address`. Even if timestamp parsing and
permissions succeed, those messages cannot satisfy the inspected parser's string/state checks.

The tag's `si_debug.c:1071` returns without producing its VM report when that parser returns false.
`si_gfx_cs.c:130,214` also changes submission timing for `CHECK_VM`: it clears `PIPE_FLUSH_ASYNC`
and waits on the submission fence for up to 800 ms before checking. Thus this switch is not a
passive recorder and a clean run with it would not alone exonerate the original asynchronous path.
This source observation does not prove the installed Debian Mesa binary lacks distribution
patches; inspect the matching package source before asserting its exact parser implementation.
An offline `strings` check of the installed amd64
`/usr/lib/x86_64-linux-gnu/libgallium-25.0.7-2+deb13u1.so` also found `VMC page fault`,
`   at page`, and `VM fault report.`; it did not find `in page starting`. This corroborates the
source-level concern but string presence alone does not prove all installed control-flow paths.

This is a diagnostic limitation, **not the primary GPUVM cause**, not a CNA defect, and not a
reason to change Mesa or kernel permissions during this task. Before another hardware run, verify
the actual installed driver's evidence path against these saved messages or choose a supported
pre-fault capture mechanism which does not depend on this parser. No hardware run is justified
merely to test a parser that can be checked offline.

Journal access initially blocked qualification. The operator has now granted membership with:

```sh
sudo usermod -aG systemd-journal robertvokac
```

A new login picks up this group. The existing session can already use
`sg systemd-journal -c '<qualification command>'`, as verified during the serial reproduction.
The supervisor still requires an actual kernel record and cursor, not just a successful empty
`journalctl` invocation.

**Hardware qualification is currently stopped by the reproduced fault and its persistent latch.**
Do not clear the latch or repeat the workload unchanged to collect more failures. The remaining
sequence below is pending diagnosis and a justified correction; the first three steps have been
attempted and the third failed:

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

### Software-only supervision hardening (2026-10-09)

The native visual-capture helper previously checked the kernel only after its bounded workload
finished. It now additionally checks before launch, incrementally during rendering, and after
owned-child cleanup. It flushes each observed record to `kernel-live.jsonl` before fault handling,
terminates its process group on the first fault or loss of journal visibility, and preserves the
first failure even if cleanup also fails. SIGINT/SIGTERM restore the caller's handlers after owned
process cleanup. The exact command and baseline are written before launch.

Twenty-one CPU-only tests cover configuration and monitor behavior, including pre-launch/live/
cleanup faults, cursor advancement, boot changes, journal failure, timeout, signal interruption,
nonzero exit and cleanup-error precedence. The serial llvmpipe east capture at
`build/software-captures/20261009T004917.838194Z-east/` passes with an empty live and final kernel
delta and no GL error or material fallback. This closes a harness safety gap, not the original
GPUVM defect; no AMD workload was launched and the hardware stop latch remains unchanged.

### Hardware acceptance (still pending)

#### 2026-10-09 — detached incident compositor discovered and stopped

Final host-process inspection found Weston PID 3593026 still running, with desktop-shell child
3593070. Both started on October 8 at 18:31:41 CEST. Its socket `cna-weston-3593017` and
`/tmp/cna-wl-ZmR7Ni` configuration/runtime directory exactly match the retained serial-jungle
workload's environment. This establishes ownership; it is not a user desktop or another project.
The earlier statement that the game/wrapper were absent did **not** establish complete cleanup.

Weston had PGID/SID 3593026, unlike the workload group 3593017; the shell had its own PGID/SID
3593070. The game supervisor's `stop_process_group` only signals the original PGID.
The consumed CNA `tools/platform/wayland_test_server.sh:121` launches Weston with `setsid`;
its own cleanup trap is not a substitute for supervisor containment when the wrapper is killed.
The exact reason its cleanup did not complete is not proven. The group-boundary gap itself is
confirmed by the live processes and source. This is a game-side supervision defect, not proof
of the primary GPUVM cause and not grounds for modifying CNA.

Preserved the compositor log and configuration before sending SIGTERM only to the verified
Weston PID. Its log records signal 15 at 07:51:59.386 CEST. Both exact PIDs were subsequently
absent; a wider process check found no remaining private Weston/game/emulator. GNOME's
Xwayland :0 was untouched. The preserved before-cursor/after-cursor kernel check in
`build/gpu-incident-evidence/orphan-cleanup-kernel-{before,after}.json` has an empty delta.
No new hardware workload, debugger, reset or kernel setting was activated. Temporary incident
directories were preserved, not deleted. Evidence hashes are in the evidence README.

**Secondary containment fix verified on October 9:** both Python qualification paths now use
a bubblewrap PID namespace and parent-death chain, including `setsid` descendants. Thirty-eight
CPU-only supervisor tests pass, along with 31 non-display CTest groups, a nested Android
isolation preflight and one serial llvmpipe jungle capture. The latter has empty kernel deltas;
host process inspection finds no owned compositor/game/emulator survivors. Runtime diagnostics
preserve the outer PID/TID independently of namespace-local IDs. See the exact commands, scope,
evidence and remaining browser-harness task in [process containment](process-containment.md).
No CNA wrapper was edited. The hardware stop latch remains unchanged; this secondary fix does
not resolve the GPUVM fault or authorize another hardware reproduction.

Read-only preflight on 2026-10-09 at 05:31:32 UTC again verifies journal access,
an active stop latch, and no device-dump entries. No workload was launched and no
root command is required from the operator. The subsequent
[Mesa recorder readiness audit](mesa-recorder-readiness.md) establishes why merely
enabling Gallium's debugger is insufficient: asynchronous file writing, unverified
uploaded vertex-descriptor coverage, instrumentation-induced lifetime changes and
no verified bounded output. These are diagnostic limitations, not a primary-cause
finding. The expired dump must not be replaced by deliberately provoking another fault.

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
