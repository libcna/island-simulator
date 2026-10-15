# Whole-island wildlife, sound and motion pass

## Recorded wildlife matrix

The third post-realism pass on 2026-10-15 compared representative close and habitat views:

- deer: `20261015T123301.629519Z-deer`
- bear: `20261015T123305.888491Z-bear`
- boars: `20261015T123314.938751Z-boars`
- goats: `20261015T123324.413950Z-goats`
- crocodiles: `20261015T123333.265345Z-crocodiles`
- fish school: `20261015T123337.523501Z-fish`

The bear, boar and goat have coherent volume/material response at their intended distances;
crocodiles and fish remain seated in their water habitats. Deer were visibly the weakest family:
large constant-colour facets split the torso into horizontal bands, the neck read as one rigid
tube, and the low-sided legs/paroh beams lagged behind the newer animal models.

## Deer correction

Near deer now light ellipsoid vertices from their analytic surface normals and interpolate those
colours across the existing triangles instead of assigning one colour to an entire face. Tubes
similarly interpolate their radial endpoint light. The neck follows two tapered segments, while
the near-only torso, shoulder/rump, head, muzzle, eye, leg, hoof, tail and antler tessellation is
raised within a bounded 5,800-vertex-per-animal staging allowance. The four deer still share one
dynamic batch and one draw; their far mesh, population, habitat and deterministic graze/walk/rest/
retreat simulation are unchanged.

The accepted current native frame is `20261015T124054.189699Z-deer` (233 colour bins, 33.78
luminance deviation, 140.28 spatial contrast). Chrome 155/SwiftShader WebGL 2 evidence
`build/browser-software/20261015T124406.991532Z-chrome-deer` passes with a live context and clean
kernel monitoring; its deer PNG was inspected. The Web package is 11,537,041 bytes of Wasm,
2,230,701 bytes of data and 280,771 bytes of JavaScript. Android ARM64 assemble/lint passes without
emulator/KVM. The 24,535,363-byte APK has SHA-256
`3b6832bfb2ff8fb46f1b15ab944e89550ba6de6da4efa354dfc06e7c12285395` and contains a
19,492,792-byte `libmain.so`.

## Sound boundary

The current automated soundscape runtime gate passes on the isolated dummy backend. It covers
startup, mute/unmute, water transitions, surf/wind/jungle/stream/campfire layers and the shared
bird, monkey, boar, goat, snake, bear, frog and cicada voice. The accelerated 72,000-frame route
already checks twenty simulated minutes of finite bounded mixes and beach/jungle/swim/current/
wading transitions. This is long-session scheduling/silence evidence, not human headphone or
speaker naturalness. A real listening walk and external audio-focus/device lifecycle remain open.

This completes the recorded wildlife visual/motion and automated sound portion of the third pass.
It does not close subjective audio acceptance, moving-edge aliasing, physical-device performance
or real-device lifecycle qualification.
