# Inland research bunker

The bunker is an original project-owned procedural asset. It uses no downloaded model,
texture or third-party licence. The entrance sits in the inland ridge at `(92, 24)` and
opens into a descending access passage, a 36 m main spine, a computer room, a service
workshop and a deliberately fictional reactor-generator room. It is scenery for quiet
exploration, not an operational reactor design or a mission system.

## Traversal and terrain integration

`BunkerLayout` owns the deterministic local plan, floor support and player-capsule
collision. The exterior heightfield remains intact over the buried facility, while the
terrain shader suppresses only fragments below the bunker ceiling. This keeps soil and
vegetation above the roof without filling the playable rooms. The final corridor wall is
a full sealed maintenance bulkhead, so the player can no longer see the underside of the
island beyond the authored interior.

The portal threshold uses a packed-earth apron and a gentle floor transition. A native
controller regression starts outside, walks through the portal and down the ramp, reaches
the room branches and deep corridor, then returns to dry exterior ground. Grass and
ordinary vegetation reserve the entrance and facility footprint.

## Visual construction

The current static mesh is 3,876 triangles in two culled draws. It includes:

- a rock-concealed reinforced portal, open blast door, jamb bolts and drainage threshold;
- repeated structural corridor bays, recessed lower panels, safety stripe, cable trays,
  utility rails and six warm ceiling-light pools;
- four active computer desks with keyboards and chairs, eight server cabinets, a wall
  situation display, tiled floor and cable details;
- a complete service-room shell with cabinets, workbench and amber task lighting;
- a cyan fictional generator core, containment rings, support posts, control cabinets,
  pipes and caution markings;
- a reinforced sealed end bulkhead that replaces the former view beyond the facility.

Opaque concrete, steel and rock use one portable desktop-GL/GLES shader with subtle
world-space mottling, damp ageing, restrained oxidation and baked per-face shading.
Luminous fixtures, displays and the generator core share the second alpha draw. If custom
effects are unavailable, the geometry retains a `BasicEffect` fallback and logs it; the
qualified runs below used the custom material without fallback.

## Verification — 2026-10-14

Native Release `island_bunker_tests` passes real `PlayerMotion` entry, descent, room
access, collision and exit. Inspected native software captures include the computer room
and the sealed-end view `20261014T184005.204770Z-bunker-end`; the latter shows the complete
bulkhead with no exposed island underside.

Focused Chrome/SwiftShader evidence is
`build/browser-software/20261014T190856.457455Z-chrome-bunker`. It contains six distinct
inspected views (`bunker`, corridor, computers, reactor, service and end), each with twelve
finite post-capture intervals, `glError=0`, an intact context, the current material/closure
diagnostics and an empty kernel GPU-fault delta. Twelve intervals are intentional here:
the scope owns six static artistic cameras and the CPU-only renderer can take more than
ten seconds for one full-island frame. This is a shader/visual portability gate, not a
WebGL performance claim.

Android ARM64 `assembleDebug` and `lintDebug` pass with two workers. The generated debug
APK is 23,919,699 bytes. No emulator, KVM setting or device runtime was touched, so GLES
runtime traversal and touch-control review remain open.

## Remaining work

The facility is intentionally compact and still visibly procedural at close range.
Moving first-person artistic review, richer prop variation, localized audio, Android
device runtime and real-GPU performance qualification remain future polish. They do not
reopen the fixed entrance collision or the sealed-end terrain leak.
