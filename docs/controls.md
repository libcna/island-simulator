# Controls

## Linux desktop (current)

| Input | Action |
|---|---|
| Left click | Capture the mouse for first-person look |
| Mouse | Look around while captured |
| W / A / S / D | Move with acceleration and deceleration |
| F1 | Show or hide the developer diagnostics overlay |
| F2 | Toggle world-space shoreline/current, collision, terrain-contact, cell, wildlife, and audio-zone/emitter diagnostics; visible only with F1 |
| F5 / F6 / F7 / F8 / F9 / F10 / F11 / F12 | With the overlay visible, teleport to eight QA locations; F9 faces the fish shallows, F10 the offshore shark, repeated F11 presses cycle through all seven monkeys, and F12 enters the active offshore current for feedback QA |
| B | Smoothly disable or restore restrained walking/swimming camera motion |
| M | Mute or unmute the ambient soundscape |
| Escape | Release captured mouse; press again to exit |

Movement follows terrain, refuses uphill motion steeper than 38 degrees, and switches to swimming
in sufficiently deep water. Swimming is slower. An offshore current progressively resists outward
movement and carries the player back toward the island. A subtle peripheral tint and deeper
undertow texture strengthen with that same current field. Moving through shallow or swimming water
also produces subtle expanding surface ripples around the player. On land, nearby mesh-grass tips
bend smoothly farther aside as walking speed rises and relax after the player stops.

Jumping is intentionally absent. The game is not a platformer.

The developer overlay starts hidden during ordinary play. Launching with `--debug-overlay` enables
it immediately for captures and QA; `--world-debug` also enables its F2 world view. It reports
wall-clock frame timing, renderer identity, player and ocean-current state, visible
terrain/vegetation cells, LOD populations, draw counts, and active audio sources against the
six-source ceiling. See `docs/debugging.md` for interpretation and limitations.

## Web (current)

The Web build uses the same WASD, mouse-look, F1, F2, B, M, and Escape controls. Its production page
displays click-to-play instructions and requests pointer lock directly from the first left-button
gesture, as required by browser security policy. The same gesture starts audio after browser
activation. Escape releases pointer lock; clicking the canvas acquires it again. Automated
Chromium/SwiftShader coverage verifies those transitions, audio-mixer/track initialization,
keyboard movement, relative mouse look, focus, and responsive 16:9 sizing. Audible quality on a
visible hardware-accelerated browser and a second browser engine remain to be checked manually.

## Android

The current Android build uses two simultaneous touch zones:

| Input | Action |
|---|---|
| Touch and drag in the left half | Move with a floating analog stick anchored at the first contact |
| Touch and drag in the right half | Look relative to the preceding touch position |
| Release a contact | Stop the role owned by that pointer without disturbing the other role |

Subtle generated circles show the movement origin/thumb and the current look contact. The control
interpreter preserves pointer ownership if a finger crosses the screen centre, clamps movement to
a unit vector, applies a dead zone, and cancels missing contacts or contacts invalidated by a
display-size change. The HUD remains inside conservative display-edge margins.

The implementation and deterministic multi-touch tests are complete, and the same HUD rendering
code compiles for OpenGL 3.3, WebGL 2, and OpenGL ES 3. The available API 35 emulator rendered the
game through GLES3 but is too unstable without KVM to establish touch feel or pause/resume quality.
Sensitivity, optional handedness, and real-device ergonomic tuning remain open.
