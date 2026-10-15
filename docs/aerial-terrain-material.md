# Aerial mid-scale terrain material

The 2026-10-15 whole-island audit found that close soil, litter and turf detail filtered correctly,
but this left only the roughly sixteen-metre broad material field in a high overview. Inland hills
therefore read as large smooth green and brown regions even though first-person terrain was varied.

`TerrainRenderer` now evaluates a second footprint-filtered value field at approximately eight
metres. One shared value modestly affects inland material variation, turf dryness, exposed-soil
grouping and the edge of sustained-slope rock. Correlating these responses makes the result read as
one substrate rather than unrelated colour noise. The field fades before its frequency becomes
undersampled. It changes no height, collision, environment classification, vegetation placement,
texture, varying, mesh, buffer or draw call.

Accepted inspected native evidence is
`build-consumer/software-captures/20261015T114359.515158Z-overview` and
`build-consumer/software-captures/20261015T114452.820037Z-headland`. Chrome/SwiftShader overview
evidence `build/browser-software/20261015T114632.228673Z-chrome-sky` passes 60 finite positive
intervals, GL0, live context and clean kernel monitoring. ARM64 assemble/lint passes without
emulator/KVM; the APK is 24,532,083 bytes with SHA-256
`ca45a625db23690728b523ec85e240974d7c374c466a341cef8003b82c38e78e` and packages a
19,489,512-byte `libmain.so`. Open hills remain ecologically intentional and are not filled with
distant billboard trees.
