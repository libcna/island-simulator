# Incident Evidence Provenance

These are verbatim copies retained alongside project sources so build/temporary-file cleanup cannot
erase the available evidence. Newly added reports may still be uncommitted in the working tree;
their presence here is not a remote backup. The primary output and original kernel log concern the overnight incident;
the serial reproduction and partial device dump concern the single supervised 18:31 attempt.

| File | Source | SHA-256 |
|---|---|---|
| `serial-jungle-orphan-compositor.txt` | Surviving serial-jungle Weston's `/tmp/cna-wl-ZmR7Ni/compositor.log`, copied October 9 before cleanup | `0cb171f6dc4b189afdd3a965bb2eaa08ad2ba8805792bfc02d3f35e8beaed413` |
| `serial-jungle-orphan-compositor-after.txt` | Same log after verified PID 3593026 received SIGTERM at 07:51:59 CEST | `46c35442dbc5ef1f4ad09234b7478a50c42d996ea3f169a62a29730413a03c1f` |
| `serial-jungle-orphan-weston.ini` | Exact compositor configuration for that same process | `a416f1f26bdc6e250e10984516b8757b41ecb1c33a2a91272e4aecd1bd14a018` |
| `primary-ctest-output.txt` | CTest output extracted from the original Codex rollout into `/tmp/island-incident-ctest-output.txt` | `6493203c2b0cb6a319375c5aeb4786709041a4b1b9ef4e4fb2701dee94e231a6` |
| `post-reset-retry-compositor.txt` | `/tmp/cna-wl-sfF758/compositor.log` | `6a1af532bf3e3ed6bf0b34cd828cf10a6a2c4f39e17d1b9b5d1fbc790b448479` |
| `original-kernel.txt` | Kernel journal, 01:12:35–01:14:00 CEST | `87258020606266dc87b5db8c7e7cc4e206effeaea96fe9b14b367291030752a3` |
| `serial-jungle-kernel.txt` | Kernel journal, 18:31:40–18:33:00 CEST | `958e9b1420ef4937651c446df5992f782ecf3c91069256f18b0f6e93cf098abe` |
| `serial-jungle-reproduction.tar.gz` | Entire `build/gpu-incident-evidence/20261008T163141.853965Z-3593014-benchmark-jungle/` | `edbbdca2bf1be5c8af0942bc4a1dabc1e7ff0fe0e939942a826bca90732cab69` |
| `partial-amdgpu-devcd3.txt.gz` | Operator's partial root read of `/sys/class/devcoredump/devcd3/data` | `a3a6f071ca3fcf1f661bd1707e07fed81c0bb11daff4add540b5f289f9467d10` |
| `software-vertex-audit.json` | Offline bounds audit of the llvmpipe GL trace | `63c7ff40dff0e4bf934a76ad08e6e266847632c4049977160e897399ebd5440a` |
| `software-state-inspection.json` | Offline program/texture checks and allocation/deletion inventory of the same llvmpipe trace | `c75c8148dca4dce8ef00bcb534db4dc2f230b5b5a703f494a83ac487d744152a` |
| `software-mip-state-inspection.json` | Extended offline texture check including base/max mip levels and separate sampler overrides; same retained llvmpipe trace | `bd6bb51d2b5c04e83313f42159e8cbf34b16a04794cc5346682a8821b07c17ee` |
| `software-skin-audit.json` | Offline fetched bone-index/weight audit of the same llvmpipe trace, using `tools/audit_gl_skin_trace.py` | `622e46c5968d24ea744e273bf5135561be9fecdafd473d759c45178aff3258e8` |
| `software-skin-palette-audit.json` | Extended skin auditor, also checking finite uploads for all referenced palette slots before every draw | `05265be8d3c2e22a79d630581a636df2b23a9ab8e53338586790ef84a7caf5ee` |
| `software-jungle-diagnostics.tar.gz` | Software benchmark logs, kernel checks, and audit report; excludes the larger trace retained locally | `9fe71ab9bd5ed402304c9fa0627931f126f2de7ba9c193976c6aef38fac61d2d` |

The decompressed partial device dump is 3,436,544 bytes, SHA-256
`11ad14cb540dfc446b96a13cab3e6690f0b0ee89551e23c9203f7352dbb30b34`. Its root read ended
with `No such device` during automatic expiry; its last line is incomplete. Preserve that
truncation rather than appending or manufacturing a terminator. It includes the complete graphics
ring but only part of the MES ring. The original `.bin` filename is retained under the build
evidence directory even though the content is text.

The compositor log is from the unsafe retry **after** the reset. It must not be used to infer the
operation that preceded the initial GPUVM fault. The primary run's compositor log was removed by
its wrapper, and no primary-process core exists. Consult the parent incident document for the
rollout path, process map, timeline, and interpretation limits.
