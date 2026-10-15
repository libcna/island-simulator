# Bounded large-object sun shadows

Lighting item 11 uses a deliberately portable fixed-day solution rather than a general shadow-map
engine. The island has one permanent sun direction, while CNA targets desktop GL, WebGL2 and GLES3;
a depth render target, extra scene pass and platform-specific sampling fallback would be a large
cost for three static landmarks. The existing terrain-conforming contact-shadow batch already
provides alpha blending, culling, quality-distance scaling and one portable draw.

The batch now contains one broad projected-mass footprint for each of the shipwreck, inland hut and
grounded helicopter. Each footprint starts from the object's actual authored dimensions and yaw.
Its centre moves opposite the fixed horizontal sun by half the height-derived projection length,
and its two radii expand modestly to cover the union between base and projected top. Every vertex
samples authoritative terrain height, so shadows follow hills and the wreck's shallow seabed.
They use low centre alpha and a transparent twelve-segment edge, retaining the smaller existing
contact patches beneath supports. Palm and broadleaf crown shade remains in the shared baked canopy
transmission field; this stage adds no texture, render target, renderer or draw call.

Evidence:

- native llvmpipe `20261015T014613.722242Z-helicopter` passes at 1280x720 with 305 colour bins,
  55.67 luminance deviation and 234.94 spatial contrast; the projected fuselage mass was inspected;
- native hut `20261015T014214.728276Z-hut` and shipwreck
  `20261015T014411.416135Z-shipwreck` were also inspected for restrained terrain/seabed integration;
- Chrome/SwiftShader `20261015T014752.722690Z-chrome-helicopter` passes exterior and interior views,
  60 finite positive intervals per view, GL0, live contexts, distinct frames and clean kernel
  monitoring; the exterior shadow was inspected;
- ARM64 assemble/lint passes without emulator/KVM (24,088,611-byte APK, SHA-256
  `b97097bfd39510ab0a679d7ba8b8f52a9f43ce2ce1c80d0b8a03cbe55a66e52d`, 19,046,040-byte
  `libmain.so`).
- the filtered non-display regression passes 60/60; five display-producing tests remain disabled.

This is bounded mass projection, not leaf-exact, rotor-blade-exact or self-shadowing. That limitation
is intentional and named; it avoids presenting a contact ellipse as a full dynamic shadow map.
