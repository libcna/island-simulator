# Island hut — implementation in progress

The requested hut is **not rendered or installed in the live world yet**. The first
component is a renderer-independent placement search, `HutLayout::Find`.

## Placement contract

- A 4.2 × 3.4 m level floor, six supports and 0.55 m roof overhang.
- At most 576 deterministic candidate sites, 18–38 m inland, with the front facing
  radially toward the coast. Stable score/order chooses a relatively flat site.
- A 5 m site-clearance circle must not intersect supplied occupied discs. Existing
  palm, jungle, rock and wreck solids are included in the current regression fixture.
- The exact rendered terrain height range over the whole roof's enclosing rectangle
  must be above 1.2 m and have at most 0.45 m relief. The level floor is 0.45 m above
  that range's maximum; each support bottom independently samples rendered terrain.
- Sixteen points around the approach ring must be dry, inland and locally walkable.
  This is **not** a global path-connectivity proof.
- Invalid occupancy is rejected. A fully occupied world returns no placement instead
  of forcing an overlapping landmark or modifying terrain/vegetation.

Current solid-obstacle fixture selects approximately x=2.58166, z=204.115, floorY=
2.51744, yaw=0.0126473 radians. This is a tested candidate, not a saved live-world hut.
Tests cover determinism, existing-solid clearance, independent support grounding,
dense floor terrain clearance, separate posts, reserving the chosen site, blocked
world and invalid occupancy. No new textures or runtime graphics resources exist yet.

## Remaining implementation

1. Integrate placement with non-solid vegetation reservations too: the current solid
   fixture does not establish that foliage/grass cannot penetrate the structure.
2. Build a recognizable raised timber hut with posts, braces, weathered plank/slatted
   walls, an entrance/porch and a layered thatched roof. Reuse portable wood/material
   conventions; avoid adding inventory, crafting, quests or unrelated systems.
3. Install matching collision only together with visible geometry, preserving access
   around the landmark. Confirm actual player-route reachability.
4. Add grounded contact shadows, distance/frustum culling, world reporting and a QA
   viewpoint. Audit real images on native, Android and Web; test material/geometry
   budgets and portable renderer behavior. Do not mark the hut complete on builds.
