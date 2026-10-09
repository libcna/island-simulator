# AMDGPU / radeonsi incident: upstream triage draft

Prepared 2026-10-09. **Not submitted. Root cause unresolved.** This is an evidence
summary for a maintainer, not authorization to repeat a GPU-resetting workload.
The detailed [incident reconstruction](2026-10-08-amdgpu-gpuvm-reset.md) remains
authoritative. Application/CNA misuse, Mesa command generation and kernel VM
handling have not yet been distinguished.

## Suggested report title

Phoenix / Radeon 780M: low-address GFXHUB GPUVM faults during OpenGL instanced
rendering; one incident escalates through MES failure to MODE2 reset and session loss

## Observed environment

- Radeon 780M / Phoenix, PCI `1002:15bf`; graphics IP `11.0.1`.
- Kernel `6.12.107+deb13-amd64`, DRM 3.61.
- OpenGL `4.6 (Core Profile) Mesa 25.0.7-2+deb13u1`, radeonsi, LLVM 19.1.7.
- C++ island-simulator, CNA `OPENGL33` / EasyGL, SDL3, X11/GLX through a private
  Xwayland server and headless Weston **GL** compositor. A private display does
  not isolate this workload from the desktop's physical GPU.
- The serial dump records MES firmware `0x75` (feature 1), MES_KIQ `0x73`
  (feature 6), ME/MEC `0x28` (feature 35), PFP `0x31` (feature 35).
  These are captured firmware values, not a claim that firmware caused the fault.

The installed Debian package identity was checked again on October 9 with
`dpkg-query`. Its installed changelog includes distribution changes beyond the
clean local `mesa-25.0.7` tag. The published revision's complete nine-patch series
and build rules have now been inspected: none edits the reviewed ddebug/radeonsi
recorder/descriptor paths. See [patch scope and hashes](debian-mesa-patch-audit.md).
Full upstream-orig/binary equivalence is **not** established; this is not a
complete audit of the installed binary. No driver/package changes were made.

## Two distinct observed failures

1. October 8, 01:12:47 CEST: GFXHUB faults attributed to process 3497873, Mesa
   submission thread 3497891 (`island-sim:cs0`), at pages including `0x7000`,
   `0xf000`, `0x2b000`, `0x5f000`, `0x63000`. At 01:12:57 the GFX ring times out;
   MES reset/remove-queue operations fail, followed by MODE2 reset and VRAM loss
   at 01:13:01. GNOME terminates. Parallel game clients subsequently abort inside
   GLX/Mesa; their stacks are recovery consequences, not identified faulting draws.
2. October 8, 18:31:47.174644 CEST: one supervised jungle benchmark reproduces a
   GFXHUB fault at `0x17000`, status `0x00101031`, client TCP, PASID 16000,
   PID 3593099 / submission TID 3593107. The supervisor stops its process group.
   At 18:31:57.214690 the kernel reports a ring timeout **but soft recovered**.
   The retained interval through 18:33 contains no MODE2 or VRAM-loss record.

Concurrent game clients are not required for the GPUVM fault. The application
emits no preceding GL-error/high-severity KHR_debug diagnostic in the serial run.
CPU pass markers reach frame 448; asynchronous submission means that number is
not an identification of the faulting GPU operation.

## Historical launch, not instructions to rerun

The original parent command, recovered from the agent execution transcript, was
`../cna/tools/platform/run_gpu_tests_private.sh build --output-on-failure -j8`
from the island-simulator repository at 01:12:34.474 CEST. It deliberately ran
eight CTest clients concurrently. PID 3497873 most likely belongs to the fixed
jungle benchmark, but no retained direct PID-to-command record proves this.

The serial parent was
`sg systemd-journal -c 'tools/run_gpu_incident_qualification.sh build benchmark-jungle'`.
Its retained per-PID log proves `build/island-simulator --benchmark <evidence-directory>/benchmark-jungle.json --viewpoint jungle`.
It used `MESA_DEBUG=context` and `ISLAND_GPU_DIAGNOSTICS=1`; AMD_DEBUG,
GALLIUM_DRIVER and LIBGL_ALWAYS_SOFTWARE were unset. The fixed-view benchmark
requests 120 warm-up intervals and 600 samples with VSync disabled.

The original worktree contained uncommitted dry-frond geometry above commit
`424357e`; the later serial build had instrumentation and other changes. Neither
the current feature build nor the retained software trace is an exact preserved
original failing binary. There is not yet a minimal deterministic reproducer.

## Available evidence and limits

All small attachments are under [evidence/2026-10-08](evidence/2026-10-08/README.md),
with recorded SHA-256 provenance:

- `original-kernel.txt`: initial fault, recovery and collateral failures.
- `serial-jungle-kernel.txt` and `serial-jungle-reproduction.tar.gz`: direct
  PID/workload/backend attribution, environment, pass markers and monitored fault.
- `partial-amdgpu-devcd3.txt.gz`: 3,436,544 decompressed bytes, truncated during
  sysfs expiry. Includes registers and the complete 2,048-dword graphics ring;
  **does not include pointed-to userspace IB or descriptor payloads**. It is not
  a complete devcoredump. No new reset has been induced to replace it.
- `primary-ctest-output.txt`: original scheduling and failures; not a direct map
  from PID 3497873 to its command line.
- Software audit JSONs: 64,169 draws / 389,340 vertex fetch ranges pass byte-bound
  checks; 2,884 skin draws have valid indices/weights and finite required palette
  uploads; observed sampled mip chains are complete. These inspect an llvmpipe
  execution, **not** the failed radeonsi command stream.

An approximately 80 MB software apitrace is retained locally with its hash in
the main incident document. It has not been uploaded or hardware-replayed.
No non-shader GL object deletion occurs during its sampled frames, but frequent
legal `glBufferData` storage replacement remains a relevant lifetime path.
Read-only inspection of Mesa buffer renaming/rebinding found no proven defect.

## Specific assistance requested from maintainers

1. A bounded supported recorder for this installed driver that retains the
   **pre-fault** IB, inline and uploaded vertex descriptors, shader descriptors,
   BO virtual-address ranges, and draw identity. Current evidence cannot map the
   low fault VA to a particular resource or packet.
2. Whether the attached primary/serial kernel evidence identifies a known
   Phoenix VM or MES recovery issue. Recovery/session loss is a separate question
   from the source of the invalid GPU access.
3. A discriminating next experiment, preferably offline first. Another unchanged
   jungle run would only reconfirm the already reproduced fault.

The [recorder readiness audit](mesa-recorder-readiness.md) explains why merely
enabling GALLIUM_DDEBUG is not yet a verified solution: asynchronous persistence,
uploaded vertex-descriptor coverage, timing/lifetime changes, unbounded output,
and side effects in the hang-report path need resolution. No supported recorder
has yet been qualified for a supervised hardware attempt.

## Safety, privacy and closure

The hardware stop latch remains active. Software-only development is separate
from incident qualification. No upstream issue has been filed, and this draft
does not authorize uploading logs: attachments contain local paths, account/host
identifiers and device details. Review a sharing copy before external submission;
preserve original evidence unchanged.

Do not close this incident based on a software renderer passing, an application
exit-code improvement, or a quiet kernel interval without the offending workload.
Closure requires a technically supported fix and serial then supervised stress
qualification with zero new GPUVM faults, ring timeouts, resets or crashes.
