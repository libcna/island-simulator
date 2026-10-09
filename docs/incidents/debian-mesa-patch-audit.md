# Installed Debian Mesa patch-scope audit

Date: 2026-10-09. Read-only analysis; no driver installation, rebuild, debugging
option or hardware execution. The AMDGPU incident remains open.

## Inputs and provenance

`dpkg-query` identifies the installed `libgl1-mesa-dri` source package as
`mesa 25.0.7-2+deb13u1`. Its Debian changelog agrees. The matching official
[source manifest](https://deb.debian.org/debian/pool/main/m/mesa/mesa_25.0.7-2+deb13u1.dsc)
and [Debian patch archive](https://deb.debian.org/debian/pool/main/m/mesa/mesa_25.0.7-2+deb13u1.debian.tar.xz)
were downloaded over HTTPS to `build/gpu-incident-evidence/`, without downloading
another upstream source tree or modifying the existing Mesa checkout.

- Manifest: 6,755 bytes, SHA-256
  `c3605bc68cd140db19523c1b80ecd4aa1d77d5b528f15902995ef9ddbbc8f07e`.
- Debian archive: 112,628 bytes, SHA-256
  `7ea51db6f4c6a39d28269975b7f652e37333a283819557c5901cbd3eccee446a`.
  Size/hash match the manifest's `Checksums-Sha256` entry. This check is not a
  separate verification of the manifest's OpenPGP signature or reproducible binary.

Read the complete `debian/patches/series`, all nine listed patches,
`debian/rules` and source format (`3.0 (quilt)`) directly with `tar -xOf`.
No patch was applied. The upstream orig archive named by the manifest was not
downloaded or byte-compared with the local `mesa-25.0.7` Git tag in this audit.

## Patch scope

| Patch | Actual edited scope |
|---|---|
| `path_max.diff` | Utility tests and GNU/Hurd pipe-loader path constant |
| `src_glx_dri_common.h.diff` | GLX drawable forward typedef |
| `disable_ppc64el_assembly.diff` | Meson ppc64 little-endian assembly selection |
| `etnaviv-add-support-for-texelfetch.patch` | Etnaviv instruction/compiler lowering/tests |
| `Revert-hasvk-elk-stop-turning-load_push_constants-in.patch` | Intel ELK/Haswell Vulkan lowering |
| `kopper-Revert-kopper-Explicitly-choose-zink.patch` | Kopper loader's explicit-driver argument |
| `backport_STACK_ARRAY.patch` | Adds utility stack/heap array macro |
| `CVE-2026-40393-part1.patch` | SPIR-V structure temporary arrays |
| `CVE-2026-40393-part2.patch` | NIR function-inline temporary arrays |

None edits `src/gallium/auxiliary/driver_ddebug/`, radeonsi's `si_debug.c`,
`si_state_draw.cpp` or `si_descriptors.c`, or AMD winsys submission. Debian rules
select LLVM 19, radeonsi and llvmpipe for amd64, disable LTO and request
`b_ndebug=true`; no additional rewrite of those recorder/descriptor files is
present in the inspected build rules. Shared compiler/loader patches are not
declared globally irrelevant to rendering merely because their paths differ.

## Consequence for the next experiment

The distribution patch set supplies **no identified correction** to the
asynchronous output, uploaded vertex-descriptor coverage or output-bound gaps
in [the recorder audit](mesa-recorder-readiness.md). Thus this check does not
make an unchanged hardware rerun with `GALLIUM_DDEBUG` justified.

This closes the narrower question of what the published Debian patch series
changes. It is not a byte-for-byte installed-source/binary equivalence proof,
runtime recorder qualification, attribution of the low GPU address to a draw,
or exclusion of a Mesa/kernel/application defect. The missing pre-fault IB and
descriptor payloads remain the decisive evidence gap. Use the existing unsent
upstream draft for recorder guidance if no supported bounded mechanism can be
established offline; do not trigger another reset just to refresh sysfs data.
