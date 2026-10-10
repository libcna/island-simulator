# Sea turtle form pass

2026-10-10. The operator named the sea turtle as the worst-looking asset ("really terrible").
The previous near model was a flat ten-sided faceted lens with two-triangle fins: 204 vertices,
no dome, no scutes, no neck, flippers that pivoted rigidly.

## Shape (`SeaTurtleProfile.hpp`, CPU reference shared with tests)

- Carapace: teardrop outline widest ahead of the middle (`HalfWidth`), domed to 0.215 m above
  the rim with the ridge slightly forward (`CarapaceHeight`), closed by end fans so no facet is
  degenerate. The grid is scute-aligned: one vertebral column, two costal columns per side and a
  marginal rim column, over ten rows for the five vertebral scutes. Vertebrals are lighter,
  costals radiate dark outer / light inner, marginals use the pale underside colour, and
  alternating rows tone the seams. Seams are tonal only; there is no edge geometry.
- Plastron: shallow convex pale underside sharing the rim vertices, so the shell is closed.
- Head: neck ring, head ring and snout ring (six segments) with a nose point; lower snout facets
  use the pale beak colour; two dark eye triangles with a small highlight each.
- Front paddles (`FrontFlipper`): 0.74 m, root 0.095 m half-width, tapering to a nondegenerate
  tip; the stroke flaps about the shoulder with the tip lagging (bend), and the power stroke
  (negative) sweeps back while the recovery (positive) comes forward. Upper and lower faces are
  separate one-sided facets so the underside is shadowed.
- Rear paddles: short, delayed, reduced counter-stroke. Tail: short two-sided cone.
- Far tier: 12-facet domed teardrop with two paddles (36 vertices).

Near tier is 388 triangles / 1,164 vertices per turtle (was 68 / 204); five turtles fill at
most 93,120 bytes of the one dynamic `VertexPositionColor` batch. Still one draw, stock
`BasicEffect`, per-facet fixed-sun lighting with the existing underwater tint. The frustum
sphere grew from 1.08 to 1.22 × scale to contain the paddle reach; tests bound every reference
point inside 1.20 × scale.

## Evidence

Native llvmpipe `build/software-captures/20261010T134100.845143Z-turtles` (pass, image
inspected; before: `133435.489990Z-turtles`). The turtle now reads as a domed, scuted green
turtle with paddle strokes. Remaining defects: the six-segment head is still blocky, scute
boundaries are tonal, and the shared underwater tint flattens all colours to teal at depth.
Platform runs on the final Sharp Runtime pin c20373ba: native `20261010T143756.520012Z-turtles`
(pass), Chrome/SwiftShader full suite `142621.752717Z-chrome-full` (28/28 sections, turtle view
inspected, previous pin) plus litter scope `144151.050032Z-chrome-litter` on the final pin,
Android KVM/SwiftShader `144750.998688Z-turtles` (pass; app PID 1676 / entry TID 1822, first
draw 4,994.452 ms, total 10,885.404 ms, image inspected, empty kernel deltas). Rolling x86_64 APK
20,402,427 bytes (`98716929b85ca8d677fb88b5b786517b910d4695e7466b6129825af6241459eb`); Web
package 10,364,549 raw / 3,526,469 gzip bytes. Default ARM64 APK restored: 20,491,236 bytes
(`103b7bc1514ae06a61ebe3deb3b0160e17c90094008524da524103b48ae5e870`), runtime untested.
