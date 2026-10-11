# User-reported freeze/abort during flight — 2026-10-11

Status: OPEN, high-priority stability investigation. Root cause unconfirmed.
This is not yet attributable to flight calculations, a specific shader, CNA, Mesa,
or hardware. Do not enter it as a confirmed dependency defect in BUGS.md.

## Report and verified evidence

The user launched `./island-simulator`, requested fullscreen, repeatedly toggled
Space flight on/off with successful dry-land returns, then flew until the game froze
and aborted. Last supplied diagnostics were a monkey entering canopy shade
(transmission 0.672338) and a palette probe at root (61.4029,17.363,26.0506), with
bone M44=1. These are last printed messages, NOT proof of the faulting draw.

The terminal reported:

```
amdgpu: The CS has cancelled because the context is lost. This context is guilty of a soft recovery.
Aborted (core dumped)
```

Read-only kernel inspection found, on 2026-10-11 at 21:00:21 CEST (19:00:21 UTC):

```
amdgpu 0000:c3:00.0: amdgpu: Dumping IP State
amdgpu 0000:c3:00.0: amdgpu: Dumping IP State Completed
amdgpu 0000:c3:00.0: amdgpu: ring gfx_0.0.0 timeout, but soft recovered
```

`coredumpctl info 423089` confirms PID423089, signal6/ABRT, timestamp21:00:22 CEST,
command `./island-simulator`, executable `build/island-simulator`. Core is present:
`/var/lib/systemd/coredump/core.island-simulato.1000.3808d1dccf7343bf91b26f5ce7bbdcb1.423089.1791745222000000.zst`
(27.9M compressed). No core was extracted, deleted or uploaded.

Aborting thread423277 has `abort` then frames in
`libgallium-25.0.7-2+deb13u1.so` (offsets0xa66824,0xa6a5f2,0x57b369,0x5afb8b).
Main thread423089 is in Gallium/EGL, `dri_flush`, SDL `Wayland_GLES_SwapWindow`,
CNA `Sdl3GlContext::SwapBuffers`, EasyGL `Present`, and Game `EndDraw`/`Tick`.
This establishes the failure/presentation context, not the originating GPU command.
Full text metadata retained locally at:
`build/gpu-incident-evidence/20261011T190021Z-user-flight/coredump-info.txt`.

## Investigation tasks and safety boundaries

### Read-only follow-up: runtime identity and termination path

Recovered the actual per-PID runtime log from
`/home/robertvokac/.local/state/island-simulator/runtime/island-runtime-423089.log`.
Byte-identical local evidence copy: `runtime-423089.log` beside the core metadata;
both SHA256 `bfac8da5e9c35b767d80aa1511c4003da0ecde765c0029368caadce187627092`.
The process started at18:59:24.835 UTC and logged its first frame at18:59:33.012.
It confirms OPENGL33/EasyGL/SDL3, native Wayland, AMD Radeon780M/radeonsi,
Mesa25.0.7-2+deb13u1, kernel6.12.111+deb13-amd64, requested/applied4x MSAA,
full quality and no software override. `ISLAND_GPU_DIAGNOSTICS` was unset:
there are no durable frame/pass markers from this run. Fullscreen/flight transition
messages were printed to stdout, but are not mirrored into this persistent log.
The last monkey palette message is at18:59:58.852 UTC, about22 seconds BEFORE the
19:00:21 GPU timeout. It cannot identify the last submitted or faulting draw.

The journal's archived process maps contain only4,024 bytes/57 lines, ending among
GPU mappings. They do not cover Gallium/CNA/SDL mappings and cannot establish all
loaded library identities. The executable mapping has inode37231455, matching the
current file; its modification time20:56:52.391 CEST precedes startup. Current file:
22,437,288 bytes, build ID `e37224286aeef8884283b5f2231b5983fc2231cd`, SHA256
`6e509871e84a624a0eb61dbbfa478eb4ac05b0616869ed29290d682bb822bf4a`.
This is strong corroboration, NOT an extracted core build-ID comparison. Do not rebuild
over this candidate before preserving or resolving loaded-image evidence.

Read-only source checkout `/home/robertvokac/deps/mesa` is clean at `mesa-25.0.7`.
`src/gallium/winsys/amdgpu/drm/amdgpu_cs.cpp:1997` maps `-ENODATA` to the exact
guilty-soft-recovery message and `PIPE_GUILTY_CONTEXT_RESET`.
`amdgpu_ctx_set_sw_reset_status` (line452) prints the error then calls `abort()`
when context loss is not allowed. This is a termination mechanism, not proof of the
initial reason for lost GPU execution; making the context robust alone does not fix it.

Offline disassembly of installed Gallium (build ID
`f34ab849e878d8ffbbb15533a597ca31ebd583ec`) corroborates both core return addresses:
at0xa6681a it calls formatted stderr output, at0xa6681f `abort@plt`, returning to
0xa66824 as recorded by the core. The caller at0xa6a5ed passes the string at0x18e05e0
and returns to0xa6a5f2; that string is exactly the reported guilty-soft-recovery text.
No code was executed by this disassembly and no driver binary/source was changed.
Loaded versus installed Gallium build-ID equality still requires core-note inspection.

Comparison with October8: same GPU/Mesa family and a gfx-ring soft recovery exist in
both records, but the earlier serial benchmark used X11/GLX and recorded GFXHUB/TCP
GPUVM faults at0x17000 before its timeout. The new observed interval contains no such
GPUVM address/protection record and uses Wayland/EGL. Same root cause is unproven;
flight is not necessary for the older failure, but remains a possible trigger here.

- [ ] Establish exact loaded executable/library identities from the core; a subsequently
  rebuilt file at the same path is NOT proof of the crashed process's version.
- [ ] Correlate runtime GPU pass/frame diagnostics, kernel timeline and core thread stacks.
  Do not blame the last printed monkey message or flight mode from ordering alone.
  Runtime log and termination path now recovered; exact triggering draw remains missing.
- [ ] Compare evidence with the earlier AMD incident; determine shared versus distinct
  signature without assuming either. Assess fullscreen, repeated flight transitions,
  aerial visibility/LOD and shader/resource workload changes as hypotheses only.
- [ ] Audit relevant CPU bounds/resource lifetimes and flight/visibility transitions.
  Use contained software reproduction only after reviewing the fresh incident evidence.
- [ ] Implement an evidence-supported fix and regression coverage; require matching-scope
  runtime qualification before claiming the crash fixed. Software success does not qualify AMD.

No new renderer was launched after the report. Previously running Web/ARM64 compilation
finished successfully and is terminal; compilation is not a GPU reproduction.
Keep the existing hardware qualification stop latch unchanged:
`build/gpu-incident-evidence/QUALIFICATION_STOP.json`, SHA256
`c0bd170489321c8d497eae0f1211e1553e30aef9ee02ac01774269300e6ee6f1`.
No hardware reproduction, driver reset/update, device unbind, emulator/KVM change,
process termination, or new hardware-qualification authorization is inferred.
