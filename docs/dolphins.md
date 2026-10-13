# Offshore dolphin pod

## Scope

The island now has four peaceful dolphins beyond the western reef. `DolphinPod` owns a
deterministic bounded route and three-state `Cruise -> Surface/Dive -> Cruise` cycle for each
animal. Homes remain 34--105 m offshore, over seabed at least 3 m below the surface, and every
animal remains within its 17--21 m home radius. The existing wildlife activation policy reduces
or stops simulation outside 190/520 m rather than updating an invisible ocean-wide population.

This is an original procedural implementation. It does not use a downloaded model, texture,
animation or sound recording.

## Form and rendering

`DolphinRenderer` emits all visible animals into one dynamic `BasicEffect` batch. The close form
has a tapered fusiform body, pale underside, short closed eight-facet beak below the melon,
two-facet highlighted eyes, shaped swept pectoral fins, a rear-curved dorsal fin and broad
horizontal flukes. The dorsal, both pectorals and both flukes are thin closed volumes with separate
faces and darker perimeter edges rather than coincident zero-thickness sheets. A two-part mouth
crease and two-facet blowhole retain cetacean head cues. The
body follows smoothed dive pitch and turn bank, while the tail stock and flukes oscillate laterally. Three
restrained grey-blue palettes share the existing CPU depth tint and moving shallow-sun response.

The fixed budgets are 708 vertices for a close animal and 15 vertices for a distant silhouette.
The five closed fins add 150 near vertices while retaining `CullNone`, the same buffer and the same
single batch. Four animals still produce at most one draw call. There
is no dolphin texture, separate shadow, collision proxy or audio voice.

## Verification

- `IslandDolphinPodTests` simulates 18,000 frames (600 seconds), reconstructs a second pod in
  lockstep, and checks deterministic state, finite motion, shore/depth/home bounds, surface and
  dive travel, pitch in both directions, both tail extremes, all three behaviours and activation
  tiers.
- The deterministic world report records the four states and home ranges, validates their
  habitat, and currently has fingerprint `049597fca36b232e`.
- The contained llvmpipe/pixman capture
  `build-consumer/software-captures/20261013T032529.689761Z-dolphins` passed image, runtime,
  OpenGL and kernel-delta checks. The inspected 1280x720 image contains one close dolphin in one
  draw call. The closer `--viewpoint dolphins` camera follows subject translation without freezing
  pitch, bank, fluke beat or behaviour.
- Inspected focused Chrome/SwiftShader evidence
  `build/browser-software/20261013T032653.774852Z-chrome-dolphins` retains the animal in two
  different-hash frames over 60 positive intervals with GL error zero, an intact context and an
  empty kernel delta.

The current closed-fin native pass is
`build-consumer/software-captures/20261013T112745.515773Z-dolphins`: 240 colour bins, 28.30
luminance deviation, 149.09 spatial contrast and an empty kernel delta. The first
`20261013T112616.939803Z-dolphins` tuning image retained an excessively dark fin treatment and was
not accepted; the final pass uses the body grey-blue on broad faces and the darker tone only around
their solid edges. Current Android execution remains pending.

Focused Chrome/SwiftShader evidence `20261013T113206.342870Z-chrome-dolphins` passes and both
inspected frames retain the animal in distinct pitch/tail poses. The PNG SHA-256 identities differ,
all 60 intervals are finite, `glError` is zero, the context remains live and the kernel delta is
empty. This qualifies current Web execution; Android, continuous interactive observation and
hardware performance remain open.

## Known limitations

The close animal remains visibly faceted; its melon, mouth crease, blowhole and fin curvature are
economical procedural cues rather than sculpted anatomy and can receive further art refinement.
There is no breach, leap, wake, exhalation plume, social pod interaction or dolphin sound. Actual
Android execution, continuous interactive observation, and measured integrated frame cost remain
open. These limits keep both the explicit aquatic-animal visual-polish item and the dolphin backlog
entry in progress rather than complete.
