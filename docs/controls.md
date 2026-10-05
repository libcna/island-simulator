# Controls

## Linux desktop (current)

| Input | Action |
|---|---|
| Left click | Capture the mouse for first-person look |
| Mouse | Look around while captured |
| W / A / S / D | Move with acceleration and deceleration |
| F1 | Show or hide the developer diagnostics overlay |
| F5 / F6 / F7 / F8 / F9 / F10 | With the overlay visible, teleport to six fixed QA locations; F9 faces the south fish shallows and F10 faces the offshore shark patrol |
| M | Mute or unmute the ambient soundscape |
| Escape | Release captured mouse; press again to exit |

Movement follows terrain, refuses uphill motion steeper than 38 degrees, and switches to swimming
in sufficiently deep water. Swimming is slower. An offshore current progressively resists outward
movement and carries the player back toward the island.

Jumping is intentionally absent. The game is not a platformer.

The developer overlay starts hidden during ordinary play. Launching with `--debug-overlay` enables
it immediately for captures and QA. It reports wall-clock frame timing, renderer identity, player
and ocean-current state, visible terrain/vegetation cells, LOD populations, and draw counts. See
`docs/debugging.md` for interpretation and limitations.

## Web (current)

The Web build uses the same WASD, mouse-look, F1, M, and Escape controls. Its production page
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
