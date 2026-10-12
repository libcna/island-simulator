# Underwater diving — work in progress

Required controls: Space ascends; either Shift descends while swimming. Opposing
keys cancel vertical movement. Ground pace selection must not change in water;
flight remains separately controlled by Tab. Actual below-surface camera motion,
safe seabed clearance, transitions, underwater rendering and audio are required.

## Current foundation

`SwimDepth.hpp` owns a vertical eye height starting at the existing 0.58m swimming
surface. Vertical speed is 1.6m/s, each update is limited to 0.1s, and minimum eye
clearance above supplied seabed support is 0.45m. Released keys hold depth; reset
returns to the surface. Nonfinite support rejects the update rather than inventing
a safe floor. Invalid time produces no input displacement. A changed valid support
still resolves an existing penetration even with zero elapsed time.

The vertical controller is now wired into `PlayerMotion` and normal-play input.
CPU tests cover descent/floor,
ascent/surface, opposing/released keys, invalid inputs, bounded stall steps, rising
support, dry support and reset. They do not establish underwater collision or visuals.
Native incremental build and the complete CoreTests executable passed (41.96s);
no renderer was launched for this controller-only change.

## Integration still required

`PlayerMotion` accepts a caller-owned immutable rendered surface and refreshable
vertical input. The game lazily allocates its 232324-byte height cache on the first
vertical swimming input; it outlives the player controller. Ground movement remains
unchanged. Submerged horizontal substeps query the maximum vertex height over every
quad intersecting the swept capsule rectangle; this conservatively bounds all
rendered triangles in that footprint, but may stop slightly early near grid edges.
Final vertical motion uses the footprint support. Teleports/land reset depth, and
normal flight activation already resets the player. Shift no longer toggles running
while swimming. Camera bob is suppressed when the eye is below mean sea level.
Native build and CoreTests passed40.77s, including a real terrain route which dives,
holds depth, attempts inward swimming until blocked by rising seabed, resurfaces
and resets via teleport. No graphics or audio validation has been run for this stage.

Validate world-boundary safety corrections, prop-depenetration paths and focus
transitions while diving, including deeper open-water routes and shore exits.
The renderer must handle the underwater side of the ocean and depth-dependent
visibility, and sound must respond to actual immersion. Add a controlled deep-water
QA route with actual key events and inspect below-surface images. No oxygen/survival
mechanic is implied by the diving request.
