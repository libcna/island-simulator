# Island hut — implementation in progress

The hut now has a **first visible runtime model and collision**, but is not visually
complete or fully qualified. Placement uses the renderer-independent `HutLayout::Find`.

## Placement contract

- A 4.2 × 3.4 m level floor, six supports and 0.55 m roof overhang.
- At most 576 deterministic candidate sites, 18–38 m inland, with the front facing
  radially toward the coast. Stable score/order chooses a relatively flat site.
- A 5 m site-clearance circle must not intersect supplied occupied discs. Existing
  palm, jungle, rock and wreck solids are included, together with conservative discs
  for understory (1.8 × scale), bushes (1.2 × scale) and ground plants (0.5 × scale).
- The exact rendered terrain height range over the whole roof's enclosing rectangle
  must be above 1.2 m and have at most 0.45 m relief. The level floor is 0.45 m above
  that range's maximum; each support bottom independently samples rendered terrain.
- Sixteen points around the approach ring must be dry, inland and locally walkable.
  This is **not** a global path-connectivity proof.
- Invalid occupancy is rejected. A fully occupied world returns no placement instead
  of forcing an overlapping landmark or modifying terrain/vegetation.

Current solid-obstacle fixture selects approximately x=2.58166, z=204.115, floorY=
2.51744, yaw=0.0126473 radians. The same search is used by the live game.
Tests cover determinism, existing-solid clearance, independent support grounding,
dense floor terrain clearance, separate posts, reserving the chosen site, blocked
world and invalid occupancy. No new texture assets are used yet.

## First runtime version and evidence

`HutRenderer` builds 6,804 triangles (20,412 vertex-color vertices), a single static
buffer/draw, actual mesh bounds for frustum rejection and a 300m distance cutoff.
The mesh has six posts grounded 8cm into the rendered surface, braces, individually
separated deck boards, slatted walls/front opening, rafters and overlapping narrow
roof bundles. Fixed-sun CPU face lighting and portable BasicEffect fog are used;
this is provisional material work, not realistic textured wood or finished thatch.
Upload validates finite vertices, nondegenerate triangles and a30,000vertex limit.

A conservative2.83m solid disc encloses the floor and outside post corners. It enters
the existing shared collision grid for players and wildlife together with the model.
The building is currently decorative/non-enterable; the opening is visual, not a
new indoor navigation system. Eight radial player approaches test contact without
penetration and confirm the player can actually reach the proxy. Collider is deliberately
coarser than individual walls/posts; no detailed interior collision is claimed.

CLI `--viewpoint hut` and the native software supervisor support the new camera.
Actual contained llvmpipe `20261011T221344.265945Z-hut/capture.png` inspected: raised
timber hut clearly visible on dry sand, floor/posts grounded, no broadleaf intersection
visible. Capture gate PASS (393 color bins, luminanceSD50.51, spatial contrast196.80),
kernel-after[]. Visual shortcomings: roof looks too regular/slat-like, untextured
wood is plain, and missing contact/roof shadows make the hut look insufficiently
connected to the ground. These are open, not waived by the capture pass.

## Remaining implementation

1. Finish weathered wood and natural irregular thatch materials/geometry; the existing
   roof bundles are not visually convincing palm-leaf thatch yet.
2. Add grounded contact/roof shadows. Confirm grass/wind-driven foliage clearance
   through representative views; conservative site discs are not a triangle proof.
3. Audit global player-route reachability and refine the coarse building collider if
   needed. No inventory, crafting, quests or unrelated systems.
4. Add world-report data and Android/Web QA routing; perform actual portable image
   audits and budget checks. Build success is not runtime qualification.
