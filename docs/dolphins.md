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
has a tapered fusiform body, pale underside, narrow rostrum below the forehead, paired eyes,
swept pectoral fins, a rear-curved dorsal fin and broad horizontal flukes. The body follows
smoothed dive pitch and turn bank, while the tail stock and flukes oscillate laterally. Three
restrained grey-blue palettes share the existing CPU depth tint and moving shallow-sun response.

The fixed budgets are 498 vertices for a close animal and 30 vertices for a distant silhouette,
below the renderer capacities of 540/42. Four animals still produce at most one draw call. There
is no dolphin texture, separate shadow, collision proxy or audio voice.

## Verification

- `IslandDolphinPodTests` simulates 18,000 frames (600 seconds), reconstructs a second pod in
  lockstep, and checks deterministic state, finite motion, shore/depth/home bounds, surface and
  dive travel, pitch in both directions, both tail extremes, all three behaviours and activation
  tiers.
- The deterministic world report records the four states and home ranges, validates their
  habitat, and currently has fingerprint `049597fca36b232e`.
- The contained llvmpipe/pixman capture
  `build-consumer/software-captures/20261012T205513.019794Z-dolphins` passed image, runtime,
  OpenGL and kernel-delta checks. The inspected 1280x720 image contains one close dolphin in one
  draw call. The fixed `--viewpoint dolphins` camera is available for repeat review.

## Known limitations

The first pass is intentionally compact for the 100-hour implementation target. The close animal
is still visibly faceted; the fuller forehead, mouth crease, blowhole and fin curvature are economical cues
rather than sculpted anatomy and need further art refinement.
There is no breach, leap, wake, exhalation plume, social pod interaction or dolphin sound. Actual
WebGL and Android runtime captures, continuous interactive observation, and measured integrated
frame cost remain open. These limits keep both the explicit aquatic-animal visual-polish item and
the dolphin backlog entry in progress rather than complete.
