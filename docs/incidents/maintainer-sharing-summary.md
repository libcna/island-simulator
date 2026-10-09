# Phoenix GPUVM fault: sharing summary for operator review

Prepared 2026-10-09. **Unsent; root cause unresolved.** This newly authored summary
omits local account names, hostnames and absolute workspace paths. It is not a
redacted copy of the original logs, and does not authorize sharing any attachment.
Hardware, software versions, timestamps and process IDs are retained intentionally
for technical correlation. The operator must approve publication separately.

## Suggested issue title

Radeon 780M / Phoenix: low-address GFXHUB faults in radeonsi OpenGL workload;
MODE2 recovery loses the graphical session

## Environment and impact

- Radeon 780M / Phoenix, PCI 1002:15bf, graphics IP 11.0.1.
- Debian kernel 6.12.107+deb13-amd64, DRM 3.61.
- Mesa 25.0.7-2+deb13u1 radeonsi, LLVM 19.1.7; reported OpenGL 4.6 core.
- C++ island-simulator using CNA OPENGL33 / EasyGL and SDL3 X11/GLX.
  The private Xwayland/headless Weston GL display still uses the desktop GPU.
- One incident caused a MODE2 reset, VRAM loss and termination of GNOME. A later
  single-client run faulted and reported a soft-recovered ring timeout.

## Observations, not root-cause conclusions

On 2026-10-08 at 01:12:34.474 CEST, an agent launched the historical command
`../cna/tools/platform/run_gpu_tests_private.sh build --output-on-failure -j8`.
It intentionally ran eight CTest clients in parallel. At 01:12:47, the kernel
attributed GFXHUB faults to PID 3497873 / TID 3497891 (`island-sim:cs0`, Mesa's
submission worker), including pages 0x7000, 0xf000, 0x2b000, 0x5f000 and 0x63000.
The original PID's exact workload is not directly retained; the jungle benchmark
is the strongest candidate, not a proven mapping. At 01:12:57 the GFX ring timed
out; MES recovery failed and MODE2 reset/VRAM loss followed at 01:13:01.
Other clients' later SwapBuffers/MakeCurrent aborts are not proof of the trigger.

At 18:31:41 CEST, a supervised **single** jungle benchmark started as PID 3593099
with submission TID 3593107. Its logged command shape was
`build/island-simulator --benchmark <evidence-directory>/benchmark-jungle.json --viewpoint jungle`.
It used OPENGL33/EasyGL/SDL3/X11/GLX/radeonsi, `MESA_DEBUG=context` and
`ISLAND_GPU_DIAGNOSTICS=1`; AMD_DEBUG, GALLIUM_DRIVER and LIBGL_ALWAYS_SOFTWARE
were unset. The benchmark requests 120 warm-up intervals and 600 samples, VSync off.
At 18:31:47.174644 it faulted at 0x17000, status 0x00101031, client TCP, PASID 16000.
The supervisor terminated the process group; at 18:31:57.214690 the kernel still
reported a GFX ring timeout, but soft recovered. CPU markers reach frame 448;
that is not an identification of the faulting GPU draw. No preceding GL error or
high-severity KHR_debug message was observed. Concurrency is not required.

These commands describe history, **not instructions to rerun**. Hardware testing
is stopped. The original worktree was dirty; no retained executable is proven
identical to the first incident's binary. No minimal deterministic reproducer or
technically supported fix has been established.

## Evidence retained locally

Kernel logs, serial per-PID/environment/pass logs, and a truncated devcoredump
are preserved with SHA-256 provenance. The dump contains 3,436,544 text bytes,
registers and the complete 2,048-dword graphics ring. It lacks the pointed-to
userspace indirect buffers and descriptor payloads. The sysfs dump expired;
we will not induce a new reset merely to replace it.

A separate llvmpipe API trace passes vertex byte-bound, skin palette and sampled
mip-chain checks. It has no non-shader GL object deletion during sampled frames,
but does contain frequent legal glBufferData storage replacement. These results
do **not** validate the failed radeonsi execution or establish driver responsibility.
Application/CNA misuse, Mesa command generation and kernel VM/recovery defects
remain possibilities. No CNA or Sharp Runtime files were modified.

## Assistance requested before any further hardware attempt

1. Is there a supported, bounded recorder for this driver that durably retains
   pre-fault IB contents, inline **and uploaded** vertex/shader descriptors, BO
   virtual-address ranges and correlated draw/build identity?
2. How can it be validated offline and configured without wave-halting debug
   actions or uncontrolled output? Inspection of GALLIUM_DDEBUG identified
   asynchronous persistence and unproven uploaded vertex-descriptor coverage;
   simply enabling it is not yet a qualified capture plan.
3. Do the fault/recovery signatures suggest a known Phoenix VM or MES issue,
   and which existing evidence would distinguish that from a userspace defect?
4. Which minimal next experiment would discriminate a hypothesis without merely
   repeating the unchanged faulting workload on the desktop GPU?

## Publication boundary

Review this text first. Original logs/archives contain private paths and host/account
identifiers and are **not approved attachments**. If maintainers request evidence,
prepare separate reviewed sharing copies, record transformations and new hashes,
and leave all originals untouched. No issue, log, trace or dump has been uploaded
as part of preparing this summary. No claim of hardware qualification is made.
