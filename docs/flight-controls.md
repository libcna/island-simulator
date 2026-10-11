# Bounded player flight

Space toggles flight in normal play (not fixed QA views). WASD/arrows and touch movement
remain horizontal and camera-yaw-relative; Q/E descend/ascend. Hold Shift for 36 m/s
instead of 12 m/s. Shift does not change the persisted walk/run selection during flight.
The existing debug F3 inspection camera is separate and is disabled when player flight
starts; F3 cannot activate while flying. Diagnostic teleports cancel flight.

The allowed region is a circle of radius 330 m: nominal island radius 250 m plus 80 m.
This is not exactly 80 m from every irregular shoreline point. It stays inside the
rendered terrain's 340 m square extent. Eye height remains at least 1.72 m above both
rendered terrain and mean sea level, and at most 100 m above rendered terrain, including
seabed. Thus the ocean ceiling is lower than 100 m above sea level. No tree/building
collision is claimed during flight; the requirement here is terrain non-penetration.

`FlightMotion` sanitizes activation coordinates and input, normalizes 3D motion, clamps
time to 0.1 s, and projects horizontal targets into the circular boundary. Each proposed
straight movement segment is split at all terrain grid lines and triangle diagonals.
Height-minus-terrain is affine between these points; intersecting a floor or ceiling
clips movement at the first crossing. A 1 mm numerical margin is retained. A blocked
horizontal direction requires ascending/descending, rather than silently flying through
a ridge. The lazy heap-owned rendered-height cache is shared by this controller's queries.

Turning flight off first tries dry walkable ground at the current horizontal position.
If unsuitable or offshore, a bounded 10 m grid search chooses a nearby valid land point.
Candidates use the existing player placement/depenetration and reject steep ground,
water, solid overlaps, out-of-bound points and below-rendered-terrain eye positions.
If no safe point exists, flight remains on with a diagnostic; it never places the player
inside a solid or abandons them offshore. The return is an immediate safe placement, not
a falling animation. No flight state is persisted across launches.

While flying, the ground player stays parked with zero velocity; camera bob, underwater
view, swimming ripple source and movement sounds are suppressed. Soundscape listener and
shore ambience follow the flight camera. Actual listening is still required; no audio
acceptance inferred from code. A transient on-screen notice identifies flight controls;
the diagnostics panel reports FLYING and camera position.

## Verification and remaining work

CPU tests exercise invalid activation/time, vertical motion, ceiling, radial edge,
normalized speed/stall limits and 48 routes × 120 steps with 33 independent interior
samples of every accepted segment. Landing tests use the actual vegetation, jungle,
props and wreck collision layouts, offshore/interior positions and a fully blocked world.
39/39 non-display groups pass (80.93 s); focused flight rerun passes (0.85 s) after adding
the explicit outward-boundary regression. Initial fixture build issues (missing explicit
preset target and mixed obstacle container types) were fixed before these passing runs.

Native `build/software-flight/20261011T171103.599867Z` passes actual held Space toggle,
Shift/E ascent to the 100 m terrain ceiling, repeated held ascent, mouse look, Q descent
and safe dry-land return. Aerial and landed PNGs inspected: correct elevated view and
ground return without clipping into terrain. Kernel delta empty. This also reruns the
native fullscreen/focus/mouse checks. Horizontal radial limits and ridge traversal are
CPU-tested, not yet reviewed in continuous interactive flight. Web/Android runtime
controls, focus-loss cases and audio lifecycle remain open.

The aerial image reveals conspicuous grass-density/LOD transitions and sparse-looking
upper slopes. Keep these in the island visual backlog; flight controls are not acceptance
of the island's final appearance.

Final native run `20261011T171850.676505Z` also passes after correcting rotated-diagonal
normalization and adding its regression (focused CPU rerun 1.08 s). All three actual
PNGs inspected: readable FLIGHT/Q-E/Space notice, aerial scene at the ceiling, and safe
ground return with WALK notice. The early notice capture in the preceding
`20261011T171521.041577Z` run had caught the preceding frame; the harness now waits
three flight diagnostic samples before capture. No gameplay timing or limits weakened.
Final native kernel delta is empty. Final Web Release build/package passes (11,082,700
raw / 4,168,979 gzip bytes); ARM64 assembleDebug/lintDebug passes (4 s). Build evidence
is recorded in handoff; these builds are not portable runtime flight qualification.
