# Controls

## Linux desktop (current)

| Input | Action |
|---|---|
| Left click or first movement-key press | Capture the mouse for first-person look |
| Mouse | Look around while captured |
| W / A / S / D or arrow keys | Move with acceleration and deceleration |
| Press either Shift | Toggle the 4.3 m/s walk and 8.0 m/s run modes; swimming remains unchanged |
| F1 | Show or hide the developer diagnostics overlay |
| F2 | Toggle world-space shoreline/current, collision, terrain-contact, cell, wildlife navigation, bird route/spawn, and audio-zone/emitter diagnostics; visible only with F1 |
| F3 | In debug builds with F1 visible, toggle the bounded inspection camera; use WASD, Q/E, and Shift to fly |
| F4 | With F1 visible, copy the four authoritative procedural-layout seeds and show whether clipboard readback confirmed the copy |
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
six-source ceiling. Its world-navigation line separately reports wildlife markers/targets/home
ranges and bird route-family/spawn/live-target counts. See `docs/debugging.md` for interpretation
and limitations.

The two `SEEDS` rows are always visible with the panel. F4 copies a stable English text block
containing the palm, grass, jungle, and natural-prop seeds. Desktop and Android expose a native
clipboard. Browser permission or user-gesture policy may reject the operation; in that case the
panel says `UNAVAILABLE` and the seed rows remain available for manual transcription.

The F3 camera is compiled into debug builds only. It starts at the rendered player-eye position,
keeps mouse look, moves at 12 m/s or 36 m/s with Shift, and is bounded to the island inspection
volume. It freezes player movement and locomotion bob while wildlife visibility continues to use
the inspection camera. Press F3 again, or hide F1, to return exactly to the stationary player.

## Web (current)

The Web release build uses the same WASD/arrow movement, Shift run/walk toggle, mouse-look, F1,
F2, F4, B, M, and Escape controls. Browser security still requires a click before pointer lock. Its production page
displays click-to-play instructions and requests pointer lock directly from the first left-button
gesture, as required by browser security policy. The same gesture starts audio after browser
activation. Escape releases pointer lock; clicking the canvas acquires it again. Automated
Chromium/SwiftShader coverage verifies those transitions, audio-mixer/track initialization,
correct screen-relative strafing, arrow parity, running, simultaneous movement/mouse look, focus,
and responsive 16:9 sizing. Audible quality on a
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
code runs on OpenGL 3.3, WebGL 2, and OpenGL ES 3. A KVM-accelerated API 35 emulator visibly
validated the translucent HUD and separately injected left-zone movement and right-zone look
gestures. The game also survived a same-process background/foreground cycle with its audio stream
paused and resumed. Simultaneous real-finger feel, sensitivity, optional handedness, and
physical-device ergonomic tuning remain open.
