# Android current monkey model qualification

Model revision: `ca6b042` (horizontal quadruped, actual raster fur, individual rendered
terrain targets and continuous tail skin). No dependency, emulator configuration or KVM
changes. This is an isolated x86_64 SwiftShader functional check, not physical ARM64
performance, lifecycle, full motion realism or climbing acceptance.

## Build and attribution

x86_64 debug assembly/lint passes. Tested APK retained at
`build/android-software/island-monkey-fur-contact-tail-x86_64.apk`, 21,026,335 bytes,
SHA-256 `f3f358c7e1d342d29407737241a1a9927ebc27982f51cb14fcc119736c53e895`.
APK validation checks exact hashes of all three prepared textures, including monkey fur.
Both shell startup and Python cold-start gates now require the fur-load marker with the
expected dimensions, mip count and byte budget. CPU launcher tests pass 22/22, including
missing/wrong fur diagnostics and preservation of fresh app-only logcat after failure.

Default ARM64 debug assembly/lint restored afterward (12 s): APK 21,105,312 bytes,
SHA-256 `e2419de5f7af6b15c8f439ad06ba0ef2a9b70e11d9e0321e41cddd3589a858c7`.
ZIP contains only the three ARM64 libraries and all three prepared textures. Physical
ARM64 execution remains untested. Hardware stop-latch hash remains unchanged.

## Both runtime outcomes, retained unchanged

1. `20261011T121437.574103Z-monkeys` FAILED image validation. Six capture attempts over
   thirty seconds were all black (zero luminance, one colour bin). Fur had loaded and the
   first-draw marker existed, but the retained app log preceded the presentation wait.
   Final failure screenshot also black. Empty kernel delta. Cause is not established.
2. After adding fresh failure-log retention and increasing the CLI presentation wait to
   sixty seconds (same image/graphics thresholds), identical APK run
   `20261011T121912.364377Z-monkeys` PASSES. Screenshot accepted on its FIRST attempt,
   visually inspected: textured quadruped and tapering tail visible, scene and touch controls
   rendered. No shader/GL errors or limb-reach diagnostic; final kernel delta empty.

The second run does **not** prove that the longer timeout fixed the first failure: it did
not need the extra attempts. Treat black first presentation as intermittent and unresolved.
Next failure will retain `game-failure-logcat.txt` after the wait, before owned teardown.
Do not erase the failed run or soften black-image rejection. All runtime jobs terminated;
the original 30-second in-process capture default stays available for explicit callers,
while the command-line software presentation wait is bounded at sixty seconds.

The front face still reads as a toy-like pale mask; palms/soles are not independently aligned,
stance locking/shoulder anatomy/climbing remain open. This frame proves visible current
geometry/material on Android, not completion of those requirements.
