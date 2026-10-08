# Incident Evidence Provenance

These are verbatim copies, retained in version control so temporary-file cleanup cannot erase the
available evidence. No new hardware GPU workload was used to obtain them.

| File | Source | SHA-256 |
|---|---|---|
| `primary-ctest-output.txt` | CTest output extracted from the original Codex rollout into `/tmp/island-incident-ctest-output.txt` | `6493203c2b0cb6a319375c5aeb4786709041a4b1b9ef4e4fb2701dee94e231a6` |
| `post-reset-retry-compositor.txt` | `/tmp/cna-wl-sfF758/compositor.log` | `6a1af532bf3e3ed6bf0b34cd828cf10a6a2c4f39e17d1b9b5d1fbc790b448479` |

The compositor log is from the unsafe retry **after** the reset. It must not be used to infer the
operation that preceded the initial GPUVM fault. The primary run's compositor log was removed by
its wrapper, and no primary-process core exists. Consult the parent incident document for the
rollout path, process map, timeline, and interpretation limits.
