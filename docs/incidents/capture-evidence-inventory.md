# GPU incident capture evidence and next decision

Updated 2026-10-09, operator follow-up at approximately 12:17 CEST.
Primary incident: [reconstruction and limitations](2026-10-08-amdgpu-gpuvm-reset.md).
**Root cause unresolved. No hardware reproduction authorized by this document.**

## What a useful next capture must add

The missing information is not another demonstration that the jungle benchmark can
fault. It already faulted with one game client. A diagnostic must connect the bad GPU
address to the submitted command and resource state from that same failing execution.

| Evidence | Already retained | Still missing for the faulting execution |
|---|---|---|
| Process and workload | Exact serial PID 3593099, jungle benchmark, backend and environment | Direct command-line attribution for original PID 3497873; the jungle assignment remains inferred |
| Build identity | Historical binaries and recorded hashes; source timeline | Proof that a retained executable is identical to the original overnight executable and dirty source state |
| Kernel event | Original and serial GPUVM faults, addresses, status and recovery sequence | Association of the fault with one submitted GPU operation |
| Ring and indirect buffers (IBs) | Complete 2,048-dword graphics ring in the partial serial dump | Contents of referenced userspace IBs, not just their addresses |
| Inline vertex descriptors | Register snapshot and partial command evidence | Complete descriptors associated with the faulting draw and their effective buffer bindings |
| Uploaded vertex descriptors | Driver source path identified | Actual uploaded bytes, their GPU addresses and retention at the fault |
| Shader descriptors and buffer address ranges | Software GL state and read-only driver inspection | Failing radeonsi descriptor bytes and corresponding live buffer virtual-address ranges |
| Draw identity and resource lifetime | CPU markers through frame 448; separate llvmpipe API trace | GPU/CPU correlation; CPU marker 448 is not the faulting draw number |

The software trace cannot fill the last column: it is a different execution on
llvmpipe. Valid API bounds and clean GL diagnostics do not prove valid radeonsi
descriptors, nor do missing descriptors prove that Mesa caused the fault.

## Recorder acceptance gate

Before proposing any supervised hardware experiment, a candidate recorder must:

1. Retain the IBs, descriptors, address ranges and draw/build identities listed above.
2. Demonstrate that the required data survives process termination and a GPU hang;
   asynchronous output after completion is not a pre-fault persistence guarantee.
3. Have a verified output limit and avoid uncontrolled debug actions such as halting
   waves. Preserve the original evidence without overwriting it.
4. Document how instrumentation changes resource lifetime, submission or timing.
5. Test a discriminating hypothesis rather than repeat the unchanged workload.
6. Be followed by explicit operator approval of the residual desktop-reset risk.
   A monitor can stop new work but cannot retract already-submitted GPU commands.

The [current Mesa recorder audit](mesa-recorder-readiness.md) does **not** satisfy
this gate. The [Debian patch audit](debian-mesa-patch-audit.md) found no patch that
closes its identified retention gaps. Do not clear the stop latch, repeat the expired
`devcd3` read, install another driver or induce another fault to replace the dump.

The next incident escalation is an operator-reviewed sharing copy of the
[unsent maintainer report](upstream-triage-draft.md), asking for a supported bounded
capture method. Review account/host identifiers and paths before any external
submission. No report or attachment has been sent by this follow-up.

A [standalone sharing summary](maintainer-sharing-summary.md) is now prepared for
operator review. It omits local account/host names and absolute workspace paths;
the original attachments remain unreviewed for publication and must not accompany
it automatically. No external submission has been performed.

## Follow-up checks performed

- All 15 attachment hashes in the [provenance table](evidence/2026-10-08/README.md)
  match. Original evidence was only read, not rewritten.
- Four targeted CTest groups pass in 1.55 seconds: GPU qualification supervisor,
  software capture preflight, Android software preflight and browser software
  preflight. These use CPU fixtures/synthetic faults, not graphics workloads.
- Read-only journal access through `sg systemd-journal` succeeds. The query since
  2026-10-09 11:42 CEST returns no entries at this check. This is not post-fix
  hardware qualification; the offending workload was not run.
- No game, emulator, Weston or Xvfb process appears in the host process-name check.
  No current devcoredump device is available. The stop-latch SHA-256 remains
  `c0bd170489321c8d497eae0f1211e1553e30aef9ee02ac01774269300e6ee6f1`.
- 359 GiB remains free. No evidence cleanup, framework edit, build or driver change
  was needed for this follow-up.

Safe feature work can continue separately using the existing GPU-device-isolated
software launchers. Consult PLAN.md for the current visual task; later software
progress must not be presented as AMD hardware acceptance.
