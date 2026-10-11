# Current wildlife visual audit — 2026-10-12

Actual current native build after `5fc2a94`, contained llvmpipe/private pixman,
one renderer at a time. Both captures passed their technical image/runtime gates,
and both complete1280x720 PNGs were actually inspected. Kernel deltas are empty.
This is not anatomy, animation, audio or overall wildlife acceptance.

## Wild boars

Evidence: `build/software-captures/20261011T233059.789662Z-boars/capture.png`.
Technical capture PASS:295colour bins, luminance deviation56.24, spatialcontrast275.31.

The primary animal is partly hidden behind foreground shrubs; a large diagonal leaf
occupies the right foreground. Most legs and much of the torso are not available
for a meaningful anatomy/contact review. The visible head/back outline is still
coarsely angular, with a steep wedge-like face and tiny eye. The image does not show
enough of the coat to claim natural bristles, nor does a still prove gait quality.

Next actions, in order:

- [ ] Establish an unobscured close three-quarter/side inspection view of the live
  animal. Change QA camera framing, not population or vegetation to hide problems.
- [ ] Inspect/refine head-to-shoulder continuity, muzzle/nose/ear anatomy and silhouette
  using that view; retain the shared collision and fitted-leg behavior.
- [ ] Improve close bristle/coat readability where the unobscured image supports it.
- [ ] Inspect multiple walking poses and audible grunts/snuffles separately. Existing
  sole-centre numerical accuracy and audio counters do not establish natural motion/sound.

## Monkeys

Evidence: `build/software-captures/20261011T233132.293700Z-monkeys/capture.png`.
Technical capture PASS:326colour bins, luminance deviation33.35, spatialcontrast180.72.

The subject is recognizable with directional fur and a horizontal trunk, so this is
not the old plain-brown primitive-only material. Remaining defects are visible:
flat mask-like face with very small dark eyes, coarse limb/joint transitions, an
upright sharply bending tail, and grass obscuring parts of the hands/feet. The trunk
still reads somewhat box-like. No continuous walking or climbing sequence was captured
in this pass; previous separate climbing evidence does not close these appearance issues.

- [ ] Add facial relief/eye sockets/lids and soften cheek/muzzle transitions.
- [ ] Refine shoulder/hip contours and tail curvature within measured mesh/bone budgets.
- [ ] Recheck unobscured feet and a sequence of walking/climbing poses on all software
  targets before marking natural anatomy or motion accepted.

The full requested species/resident backlog remains open. These new images neither
add missing species nor diagnose/fix the unresolved AMD flight incident.
