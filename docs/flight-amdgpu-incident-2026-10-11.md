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

- [ ] Establish exact loaded executable/library identities from the core; a subsequently
  rebuilt file at the same path is NOT proof of the crashed process's version.
- [ ] Correlate runtime GPU pass/frame diagnostics, kernel timeline and core thread stacks.
  Do not blame the last printed monkey message or flight mode from ordering alone.
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
