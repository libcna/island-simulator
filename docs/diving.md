# Underwater diving — work in progress

## Runtime audit

The supervised native tool supports `--diving`: actual F12 offshore placement,
Shift descent, Space ascent, screenshots, finite surface/dive/resurface traces and
disk-backend PCM capture. It rejects accidental flight activation or ground-pace
toggles. The focused run
`software-fullscreen/20261013T001121.596107Z` descended from the 0.58 m swimming
eye height to -1.86399 m, held that depth, and returned to 0.58 m. Both captured
views were inspected: the submerged frame shows the seabed, depth haze, surface
scatter and distant island through the water, while the resurfaced frame returns
cleanly to the normal water view. `kernel-final.json` is empty.

The run also exercised real CNA audio through SDL's disk backend. At full immersion,
airborne ambience reached its bounded 0.18 gain multiplier, the existing water bed
reached 1.22, and its pitch offset reached -0.12. The response returned continuously
to the surface values after ascent. The recording contains 1,413,120 bytes of
nonzero PCM. This establishes runtime routing and signal production, not subjective
speaker quality.

The earlier `software-fullscreen/20261012T073057.137680Z` report remains useful as
evidence of an unrelated private-window-manager fullscreen timeout before diving.
Focused diving qualification therefore does not pre-run F11 cycles; those retain a
separate input gate.

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
CPU tests cover descent/floor, ascent/surface, opposing/released keys, invalid inputs,
bounded stall steps, rising support, dry support and reset. Pure soundscape tests cover
surface, shallow, deep and nonfinite depth responses; the CNA runtime test verifies
the same deep mix while its real loop instances are active.

## Integrated behaviour and remaining scope

`PlayerMotion` accepts a caller-owned immutable rendered surface and refreshable
vertical input. The game lazily allocates its 232324-byte height cache on the first
vertical swimming input; it outlives the player controller. Ground movement remains
unchanged. Submerged horizontal substeps query the maximum vertex height over every
quad intersecting the swept capsule rectangle; this conservatively bounds all
rendered triangles in that footprint, but may stop slightly early near grid edges.
Final vertical motion uses the footprint support. Teleports/land reset depth, and
normal flight activation already resets the player. Shift no longer toggles running
while swimming. Camera bob is suppressed when the eye is below mean sea level.
Native build and CoreTests passed 40.77s, including a real terrain route which dives,
holds depth, attempts inward swimming until blocked by rising seabed, resurfaces
and resets via teleport.

`UnderwaterAudioResponse` maps actual submerged camera depth into one finite,
bounded mix. Between 0.08 m and 1.2 m it progressively muffles surf, breeze, jungle,
stream, fire and terrestrial wildlife voices; it slightly reinforces and lowers the
pitch of the already active player-water loop. It allocates no PCM and adds no source,
so the six-source ceiling is unchanged. Flight always supplies zero submerged depth.

Still validate longer deep-water routes, world-boundary safety corrections,
prop-depenetration, shore exits and focus transitions while submerged. Web and
Android need actual input/runtime qualification, and the depth mix still needs a
speaker/headphone listening pass. Full volumetric underwater fog, refraction and
particles remain outside this compact stage. No oxygen/survival mechanic is implied
by the diving request.
