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

## Closer contact camera audit

The `monkeys` QA view now starts 0.6 model-world metres forward, 2.1 metres to the
opposite side and 1.1 metres above the subject, aimed at root + 0.52 metres. Translation
tracking and the separate twelve-metre simulation observer remain unchanged. No vegetation
is hidden or removed. Native build and four targeted monkey/command-line tests pass.

Retained candidate evidence:

- `20261011T123555.508886Z-monkeys`: image gate passes, but a foreground shrub hides the
  feet. Rejected for contact inspection despite the automated pass.
- `20261011T123834.120032Z-monkeys`: more visible limbs, but image gate fails luminance
  deviation (17.9727 vs 18). Retained as failed, not reclassified or accepted.
- `20261011T124224.778948Z-monkeys`: final lower side view passes unchanged image/graphics
  gates; inspected with both front palms clearly visible along the sloped ground. Rear
  contacts remain partly obscured by body/grass. Empty kernel delta in all three runs.

The closer view also exposes the still toy-like pale face mask and overly coarse/wavy fur
pattern. It is evidence for continuing the overhaul, not completed realism or moving stance
locking. Android contact-shader qualification is recorded after its actual outcome below.

## Android contact-transform qualification

`20261011T124529.157229Z-monkeys` passes on isolated x86_64 SwiftShader; screenshot
accepted on its first attempt and inspected. Front palms and a rear foot are visible
in the side view, with the current fur and tail. No shader/GL/reach-limit diagnostic,
empty kernel delta. This is a visible single pose, not a full contact/turning-cycle audit.
The previously intermittent black first presentation remains unresolved, not erased by this pass.

x86_64 assembly/lint passes. APK includes `uSoles[4]` and the W-tag selection path,
verified in the packaged library; retained as
`build/android-software/island-monkey-sole-frame-x86_64.apk`, 21,029,215 bytes,
SHA-256 `9d7a67601b9019b32329149f24e1d207894779b21d9d207556eeea6904ec1221`.
Default ARM64 assembly/lint restored (9 s), 21,107,272-byte APK,
SHA-256 `4a11520786a0bab0c1ab5e86c1c1ba1e85ae9fa7bf71a14590cae9a17137ee54`.
Three ARM64 libraries and all three prepared textures verified. Physical ARM64 and
lifecycle remain unqualified. No emulator/KVM/dependency edits. All jobs finished.
