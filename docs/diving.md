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

This controller is **not wired into the game yet**. CPU tests cover descent/floor,
ascent/surface, opposing/released keys, invalid inputs, bounded stall steps, rising
support, dry support and reset. They do not establish underwater collision or visuals.
Native incremental build and the complete CoreTests executable passed (41.96s);
no renderer was launched for this controller-only change.

## Integration still required

Current `PlayerMotion::ResolveModeAndHeight` always targets `SwimmingEyeHeight`;
its support sampling uses the analytic field, not the exact rendered triangles.
Do not simply subtract an offset from the camera: that would allow penetration
of sloping seabed during horizontal movement and leave visual/audio state incorrect.

Wire depth into the authoritative movement state, validate the swept horizontal
footprint against rendered seabed (including triangle boundaries), preserve current
prop collision and offshore current limits, and reset on teleports/land/flight.
The renderer must handle the underwater side of the ocean and depth-dependent
visibility, and sound must respond to actual immersion. Add a controlled deep-water
QA route with actual key events and inspect below-surface images. No oxygen/survival
mechanic is implied by the diving request.
