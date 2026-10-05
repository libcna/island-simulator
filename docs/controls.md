# Controls

## Linux desktop (current)

| Input | Action |
|---|---|
| Left click | Capture the mouse for first-person look |
| Mouse | Look around while captured |
| W / A / S / D | Move with acceleration and deceleration |
| F1 | Show or hide the developer diagnostics overlay |
| Escape | Release captured mouse; press again to exit |

Movement follows terrain, refuses uphill motion steeper than 38 degrees, and switches to swimming
in sufficiently deep water. Swimming is slower. An offshore current progressively resists outward
movement and carries the player back toward the island.

Jumping is intentionally absent. The game is not a platformer.

The developer overlay starts hidden during ordinary play. Launching with `--debug-overlay` enables
it immediately for captures and QA. It reports wall-clock frame timing, renderer identity, player
and ocean-current state, visible terrain/vegetation cells, LOD populations, and draw counts. See
`docs/debugging.md` for interpretation and limitations.

## Web and Android

Web pointer-lock behaviour and Android touch controls are planned but not implemented or validated.
This section will be updated only when those paths work in their real environments.
