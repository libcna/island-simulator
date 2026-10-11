# Independent monkey palm/sole alignment — native first pass

Hands and feet previously inherited forearm/shin rotation, causing the entire palm/sole
to tilt with a bending limb. They now use four independent skin transforms: the rest-space
ankle maps to its solved target, local up follows the rendered terrain triangle normal,
and forward is the body-forward direction projected onto that plane. Normal sampling
undoes world yaw but not uniform instance scale. Sole-height compensation preserves the
normal-direction underside clearance on slopes. The limb chains still meet their original
ankle targets and preserve their segment lengths.

No skeleton bones, draw calls or textures were added. The existing fourteen-bone palette
is supplemented by four contact matrices (`uSoles`); previously unused blend-index W tags
the hand/foot vertices 1–4. Other vertices retain tag zero. The conservative vertex uniform
budget is now 92 vectors (below the 128-vector portability target), not the old 76.

`IslandMonkeySoleFrameTests` uses CNA's actual matrix/vector math without creating a graphics
device. Nine combined slopes test target mapping, rigid orthonormal axes, ground-normal
alignment and conservative underside corners with 0.004 model-metre plane clearance.
Shader tests cover both dialects, separate contact matrix selection and the updated budget.
The new test is included in the explicit native build preset; core-only tests stay CNA-free.

Native build and 37/37 non-display groups pass (81.08 s). Capture
`20261011T123046.817709Z-monkeys` passes and was inspected: model/fur remain visible, no
exploded geometry or shader failure. Kernel delta contains only one perf-sampling throttle
message, no GPU fault. Some digits are obscured by grass; this image cannot establish full
moving contact. Asset validation passes with 66 generator sources.

Remaining requirements: world-space stance locking through turns, smooth orientation across
terrain-triangle edges, multi-point footprint support where one foot spans multiple triangles,
body pitch/roll, transition/swing articulation and actual tree gripping/climbing. This is not
complete foot planting. Android/Web must requalify the changed shader/vertex convention;
previous successful captures are not evidence for this new contact-transform path.
