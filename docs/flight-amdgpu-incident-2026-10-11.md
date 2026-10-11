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
(27.9M compressed). Initial triage did not extract it. Later offline GDB inspection
used coredumpctl's temporary expansion; no core was deleted or uploaded.

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
Loaded versus installed Gallium build-ID equality was unresolved at this stage;
the subsequent retained-page check below resolves it.

Comparison with October8: same GPU/Mesa family and a gfx-ring soft recovery exist in
both records, but the earlier serial benchmark used X11/GLX and recorded GFXHUB/TCP
GPUVM faults at0x17000 before its timeout. The new observed interval contains no such
GPUVM address/protection record and uses Wayland/EGL. Same root cause is unproven;
flight is not necessary for the older failure, but remains a possible trigger here.

### Offline core inspection: loaded IDs and game state

Preserved `island-simulator.candidate` (22,437,288 bytes, matching SHA above) in the
local incident directory before any further rebuild. Core inspection via batch GDB
used `-nx -nh`, disabled auto-load and debuginfod, and never ran/resumed the program.
`coredumpctl` temporarily expanded its stored dump and cleaned that temporary file;
the compressed system core remains intact. Warnings concern deleted PulseAudio/Mutter
memfd mappings, not an executable mismatch. No renderer or GPU workload was launched.

The [incident-specific helper](incidents/evidence/2026-10-11/core-review.py) reads GNU
build-ID notes only from bytes actually present in the core's PT_LOAD segments,
rejecting missing pages rather than substituting current file-backed data:

| Loaded object | Build ID retained in core |
| --- | --- |
| Game | `e37224286aeef8884283b5f2231b5983fc2231cd` |
| Gallium | `f34ab849e878d8ffbbb15533a597ca31ebd583ec` |
| CNA | `f8dc807d0df965e4dfb6881e3cae4b1269f70acf` |
| SDL3 | `c25c94cd74bd8f6413a1ab65fa4f5e662ab16689` |

All four match the corresponding current candidate/installed files, verified with
`readelf -n`. This resolves the build-ID matching question for these four objects,
not binary provenance for every driver dependency. The game candidate includes the
still-uncommitted snake/grass interaction work. Its tracked diff and new header are
preserved locally as `pending-worktree.patch` and `SnakeGrassContact.hpp.snapshot`;
presence of this work does not establish fault causation.

Matching debug symbols recover the main-thread game object at `0x7ffcef5dc0c0`:

- CPU drawn-frame counter2290; flight active.
- Flight, camera and rendered-camera positions agree:
  `(-15.9166985, 49.44701, -118.114937)`.
- Radius119.18255 m, rendered terrain height4.66093 m, height above terrain44.78608 m
  (computed offline from retained height-cache samples using the rendered triangle layout).
- Yaw−0.417899817, pitch−0.296599835; all view/projection matrix entries finite.
- Grass2838 patches:776 near/470 middle/1592 far,24 draws, custom shader active.
  Monkeys4 visible/4 draws; contact shadows20/240 triangles/1 draw.

These values are a CPU snapshot at abort, NOT the exact asynchronously faulting GPU
frame. They do not show a NaN camera or flight-radius/height violation in that snapshot;
they cannot exclude earlier invalid state, a bad resource, or a shader/driver issue.
Full compact result is `core-state-review.txt` in the local incident directory.

### Diagnostic gap addressed (not a crash fix)

Native fullscreen requests and flight on/off/refused-landing transitions now mirror
their existing console messages into the per-process incident log. A separate sparse
`control-transition` record includes submitted-frame count, camera/flight positions,
yaw/pitch, flight-active and fullscreen-requested flags, and an explicit finite-value
flag. Event IDs are0 flight-on,1 flight-off,2 landing-refused,3 fullscreen-on-request,
4 fullscreen-off-request. Values use locale-independent float round-trip precision;
nonfinite values are flagged, not silently repaired by the logger. Fullscreen request
is not treated as confirmed platform state. Web trusted-event fullscreen remains separate.
The existing log sink timestamps and flushes each record; unavailable logging remains
best effort and cannot replace a successful input operation with an exception.

No per-frame log stream or GPU synchronization was added. Native build and focused
flight/diagnostic CPU tests pass (2/2,0.72s); supervisor35 and asset checks pass. Tests
cover formatting, float precision, all event kinds and nonfinite fields.
After the offline investigation, contained llvmpipe runtime verification passed in
`build/software-flight/20261011T195901.992700Z`. The actual persistent runtime file
contains events3/4/3/4/0/1 at submitted frames3/8/10/14/22/51: two fullscreen cycles,
flight activation and safe return after ascent to the100m ceiling and descent.
The validator requires finite state, expected flags/order and increasing frame counts;
it reads the owned persistent file rather than stdout. Runtime filenames use outer
procfs PID, so selection matches the startup namespace PID to the supervised child.
CPU supervisor suite:37 tests pass, including missing/ambiguous log identity and
missing/duplicate/reordered/nonfinite transition rejection.

Two preceding captures (`20261011T195708.323678Z` and `20261011T195829.747964Z`)
completed the interactive scenario but failed new log selection (first namespace/outer
PID mismatch, then an omitted main-tid field in the startup matcher). Both remain
failed evidence, not retroactively relabeled. All three kernel-final deltas are empty.
Final ceiling and landed PNGs actually inspected: island/landing visible, upper-slope
vegetation sparsity and grass transitions remain visual work, not final acceptance.
No AMD GPU, emulator or KVM was used or modified; no game rebuild was needed.
This closes an implementation gap in future evidence collection; it neither restores
missing history for PID423089 nor proves the GPU timeout fixed.

### Offline grass upload-range audit

Extended the core helper to inspect actual retained CPU upload shadows, without
calling buffer `GetData`, rendering, or any method in the dead process. All256 cells
are enumerated from the debug-symbol array bounds (after stripping its typedef),
not an assumed terrain-grid count. Final evidence: `core-grass-all-cells-review.txt`
in the local incident directory.

| Grass mesh | Vertices | Vertex stride | Indices | Highest index |
| --- | ---: | ---: | ---: | ---: |
| Near | 862 | 40 bytes | 1920 | 861 |
| Middle | 60 | 40 bytes | 120 | 59 |
| Far | 42 | 40 bytes | 84 | 41 |

For each mesh, retained byte sizes match declared buffer counts, vertex positions
are finite, index count equals three times the primitive count, and all16-bit indices
are below the vertex count. Across114 occupied cells, all17,225 instance records have
finite float fields, stride44 bytes, matching upload byte lengths and matching cell/
buffer counts. The sum agrees with the retained renderer's totalPatchCount_.

Source inspection also confirms grass instanced draws already validate the offset/
count against cell.patchCount. The new snake-contact shader's fixed s<4/j<41 loops
index at most163 in its164-element vec3 array; the upload supplies492 floats through
the typed vec3-array path (count164, not a scalar-array setter). No off-by-one or
type/count mismatch was found in these reviewed paths. This is not a performance,
GPU-descriptor, bound-resource, compiled-shader or all-renderer correctness proof.
CPU shadows do not show what an asynchronous GPU actually fetched.

The first helper extension inspected only64 cells and is retained as partial evidence;
an initial full-array attempt could not unwrap a typedef. Only the final all256-cell
result supports the complete instance audit above. After diagnostic rebuilds, GDB
initially warns that the recorded executable path differs; explicitly selecting the
preserved candidate and reloading the core restores matching symbols. The helper now
requires that candidate and rechecks retained build IDs before interpreting data.
No new game build, GPU workload, core deletion or dependency modification occurred
during this upload audit. The original timeout remains unresolved.

- [x] Match retained build-ID notes for game/Gallium/CNA/SDL3 and preserve the game candidate.
  A subsequently rebuilt file at the same path alone is not evidence of identity.
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

No hardware renderer was launched after the report. The later contained llvmpipe
diagnostic verification above is not a reproduction on AMD. Previously running
Web/ARM64 compilation finished successfully; compilation is not a GPU reproduction.
Keep the existing hardware qualification stop latch unchanged:
`build/gpu-incident-evidence/QUALIFICATION_STOP.json`, SHA256
`c0bd170489321c8d497eae0f1211e1553e30aef9ee02ac01774269300e6ee6f1`.
No hardware reproduction, driver reset/update, device unbind, emulator/KVM change,
process termination, or new hardware-qualification authorization is inferred.
