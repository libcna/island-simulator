# Canopy contact foundation for monkey climbing

The canopy renderer and future grip solver now share trunk ring heights and vertex positions
in `CanopyProfile`. The contact query samples the actual near/far mesh triangle, including
root flare, bend and irregular cross-section, rather than an ideal cylinder. It returns a
mesh-local position and outward triangle normal. No monkey currently calls this query:
approach, attachment, climbing and descent are still unimplemented.

An omitted wind time selects the rest mesh. Supplying the renderer's time deforms each
triangle vertex with the trunk's wind weight/phase before interpolating the contact. This
ordering matters: applying nonlinear wind directly to an interpolated point would miss the
rendered triangle plane. Triangle normals are recomputed after deformation. The CPU equation
mirrors the vertex shader and consumes `JungleWind` amplitudes; shader-equation changes must
update the CPU equation and its independent regression fixture together.

The caller must select the tree's actual LOD and apply its uniform scale, yaw and translation
to the contact (rotation only for the unit normal). The query is parameterized by rest-ring
height and angle, not by a world-space ray. Branches, separate buttress meshes and crowns are
not included. Crossing LOD boundaries during attachment needs an explicit continuity policy.
This API alone does not establish reachable grips, limb IK, collision-free body poses,
turning stance locks, a route to the tree or safe dismount terrain.

The initial rest-surface revision passed the native build and all 37 non-display test groups
(58.79 seconds). Tests cover both triangles in every trunk cell, all eight variants, both
LODs, angular wrapping, unit/perpendicular normals and invalid coordinates. Contained native
capture `20261011T130518.614818Z-jungle` passed and was inspected: trunks/crowns remain
present, grounded and continuous in this view; kernel delta is empty. This is a geometry
refactor check, not evidence of monkey climbing or an island-wide realism audit. The view
still exposes stylized flat leaf clusters and repetitive sparse ground cover.

Wind regression additionally samples three times and checks deformed triangle barycentres
and normals, plus independent vertex-shader-equation fixtures. Current Android/Web runtime
qualification, grip visualization and an actual complete climbing cycle remain pending.
The final native build also passes. Its contained jungle capture
`20261011T131221.540034Z-jungle` passes and was inspected with the same continuous trunk
silhouettes and empty kernel delta. Since monkeys do not yet use the query, this image
checks renderer integration, not animated hand-to-bark contact accuracy.
The final wind-enabled revision passes all 37 non-display test groups (102.14 seconds;
`build/canopy-grip-wind-tests.log`). Hardware qualification stop latch remains unchanged.
