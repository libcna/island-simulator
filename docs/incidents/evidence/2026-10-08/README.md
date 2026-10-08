# Incident Evidence Provenance

These are verbatim copies, retained in version control so temporary-file cleanup cannot erase the
available evidence. The primary output and original kernel log concern the overnight incident;
the serial reproduction and partial device dump concern the single supervised 18:31 attempt.

| File | Source | SHA-256 |
|---|---|---|
| `primary-ctest-output.txt` | CTest output extracted from the original Codex rollout into `/tmp/island-incident-ctest-output.txt` | `6493203c2b0cb6a319375c5aeb4786709041a4b1b9ef4e4fb2701dee94e231a6` |
| `post-reset-retry-compositor.txt` | `/tmp/cna-wl-sfF758/compositor.log` | `6a1af532bf3e3ed6bf0b34cd828cf10a6a2c4f39e17d1b9b5d1fbc790b448479` |
| `original-kernel.txt` | Kernel journal, 01:12:35–01:14:00 CEST | `87258020606266dc87b5db8c7e7cc4e206effeaea96fe9b14b367291030752a3` |
| `serial-jungle-kernel.txt` | Kernel journal, 18:31:40–18:33:00 CEST | `958e9b1420ef4937651c446df5992f782ecf3c91069256f18b0f6e93cf098abe` |
| `serial-jungle-reproduction.tar.gz` | Entire `build/gpu-incident-evidence/20261008T163141.853965Z-3593014-benchmark-jungle/` | `edbbdca2bf1be5c8af0942bc4a1dabc1e7ff0fe0e939942a826bca90732cab69` |
| `partial-amdgpu-devcd3.txt.gz` | Operator's partial root read of `/sys/class/devcoredump/devcd3/data` | `a3a6f071ca3fcf1f661bd1707e07fed81c0bb11daff4add540b5f289f9467d10` |
| `software-vertex-audit.json` | Offline bounds audit of the llvmpipe GL trace | `63c7ff40dff0e4bf934a76ad08e6e266847632c4049977160e897399ebd5440a` |
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
