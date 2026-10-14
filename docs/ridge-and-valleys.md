# Asymmetric interior ridges and drainage

## 2026-10-14 measured-slope outcrop refinement

The high overview audit found that the preceding outcrop breakup still let broad noise move the
rock threshold far enough to produce large dark spots on gentle interior shoulders. Exposure now
follows measured slope more closely: broad and medium fields perturb the threshold only lightly,
and a stronger share of the existing medium projected mottle breaks each truly steep outcrop into
smaller connected mineral regions. Minimum inland exposure falls from 16% to 4.5%; the coastal
field still raises that same value to 88%, so the salt-worn headland remains a continuous cliff.

No texture, noise evaluation, vertex, terrain cell, collision shape or draw is added. Identical
native overview baseline/final evidence is `20261014T161807.844403Z-overview` and
`20261014T162029.661645Z-overview`; coastal regression evidence is
`20261014T161833.419961Z-headland` and `20261014T162057.573757Z-headland`. All four were inspected.
The revised overview keeps more soil/turf continuity on the ridge shoulders while retaining rock
on the actual high face; the close headland preserves its mineral wall, bedding and open-sky fill.
Focused Chrome/SwiftShader evidence `20261014T162348.342986Z-chrome-cliffs` passes inspected east
and headland frames, 60 finite positive intervals per view, GL error zero, an intact context and
an empty kernel delta. The default arm64 APK assembles and application lint passes through the
two-worker wrapper without starting an emulator; Android runtime appearance remains unqualified.

## 2026-10-14 irregular exposed-rock follow-up

A fresh overview/east/headland audit selected the broad inland ridge face as the weakest
terrain element. Although its noise already used a vertical face projection, the exposure
mask still blended that rock almost continuously across the hillside, producing one dark
brown curtain. The material now derives an irregular but connected outcrop coverage from
the existing projected macro/mottle/bedding fields. Inland thin soil can show between those
outcrops; the coastal field independently keeps salt-worn headlands predominantly mineral.
Medium bedding, seams and joints have slightly stronger contrast, and shaded steep faces
receive a bounded cool sky fill. No new noise sample, texture, vertex, terrain cell, collision
shape or draw is added.

Native baseline/final evidence is `20261014T135539.892693Z-east` and
`20261014T140025.714730Z-east`; the coastal regression pair is
`20261014T135547.506252Z-headland` / `20261014T140033.470550Z-headland`. All four were
inspected. The interior slope now alternates soil/moss and weathered outcrop rather than one
uniform mud tint, while the headland retains its continuous rock face. This is a material
improvement, not additional terrain tessellation or a claim that the height field has true
overhangs. Focused terrain/core tests pass; WebGL builds and Android ARM64 assemble/lint
passes without an emulator or KVM. Portable visual runtime remains unqualified.

2026-10-11, interior stage of the macro-geography rewrite (item 1).

The previous terrain was a low noise-driven dome, reaching about 19.4 m. Two offset elongated
ridges now establish a readable skyline: a main summit near (-48, 48) and a lower eastern
summit near (76, -30), with a saddle between them. Broad southern and western drainage cuts
break their shoulders and widen toward the coastal plain. Multiplicative erosion keeps the
surface continuous and above sea level without a clipped flat valley floor.

The existing shared terrain queries rebuild vegetation, prop placement, wildlife ground
contact, collision support and the rendered mesh over the new elevations. Slope limits leave
thinner vegetation on steep faces. Those faces now reveal mottled weathered rock with filtered
grain and narrow fractures, rather than falling back to an untextured green vertex colour.
The grid remains 58,081 vertices / 115,200 triangles, with the same terrain buffers and draw
passes. Rock detail adds two noise evaluations only on exposed ridge fragments.

## Wildlife and traversal

The taller terrain exposed two pre-existing bird altitude defects: spawn oscillation was added
after clearance clamping, and limited climb speed could lag rising terrain. Spawn now clamps
the complete altitude; flight anticipates terrain two seconds ahead, approaches its clearance
with the existing climb rate, and enforces a final minimum clearance floor. Existing deterministic
flight tests caught both cases. This protects terrain clearance; exact canopy envelope avoidance
is still a separate visual improvement.

The narrow trail's three-lane static obstacle regression remains in force. Existing player
slope/step tests, boar terrain contact and rendered root/contact tests must pass on this shape.

## Scope and next work

This stage changes the interior relief. The old radial shoreline and coastal shelf are still
present; the following stage must create distinct headlands, sheltered bay and dominant beach,
then a shared environment field for vegetation and material placement. Local terrain details,
rock outcrop geometry and route detours/forks remain open. Do not mark the complete macro-geography
rewrite finished on this evidence.

## Measured state and qualification

Default world fingerprint: `ccc1de2816f009d0`. Populations: 372 palms, 267 canopy trees,
1,786 understory plants, 579 bushes, 3,016 ground plants, 18,583 grass clusters, 173 rocks
(14 offshore), 10 logs, 32 driftwood pieces and 109 fallen fronds. Clearing accents:
13 bushes / 64 plants. Forest-floor companions: 190 colonies / 758 plants.

Two-metre regional sampling measures summits at 40.0192 m and 29.0938 m, and the saddle
station (10, 5) at 13.7758 m. The southern valley transect at z=-70 measures 9.76125 /
7.93494 / 16.0371 m (western shoulder / bed / eastern shoulder). Regression bounds require
35–45 m and 25–30 m summits, a lower saddle, two raised valley shoulders and descent coastward.

Initial native overview `20261011T072056.176221Z-overview` inspected: the skyline now has two
distinct ridges. Intermediate east views `072240.069046Z` and `072409.439101Z` exposed the
green-slope fallback and verified its replacement with weathered substrate. Final native east
`20261011T072605.084747Z-east` inspected with fractured rock detail; overview
`20261011T072739.811196Z-overview` passes and was inspected. Final 33/33 non-display
test groups pass (49.78 s), including root grounding and wildlife contact.
Android x86_64 build/lint and east run `20261011T072932.613664Z-east` pass; image
inspected and agrees with native. Interactive run `20261011T073115.062445Z-interactive`
passes touch/traversal but fails Home/resume with `EGL_BAD_SURFACE` during surface teardown.
Kernel delta is empty. See suspected framework issue `CNA-ISLAND-001` in `BUGS.md`.
Identical-APK repeat `20261011T073355.598497Z-interactive` passes all gates (hot PID 1756,
cold PID 3258), demonstrating intermittent behaviour. Android lifecycle remains degraded.
Web build succeeds; contained Chrome terrain suite `20261011T073720.044111Z-chrome-terrain`
passes overview, beach, east and jungle (60 CPU-rendered frame intervals each).
Overview/east/jungle screenshots inspected and agree with the native terrain. No new kernel
GPU faults; the kernel delta contains an unrelated perf sampling-rate notice. Web release
refreshed: 10,340,243 raw / 3,525,530 gzip bytes. Full Web input/lifecycle and Firefox were
not rerun for this milestone.
Tested x86_64 APK: 20,432,315 bytes, SHA-256
`f148c3c6eff22c0c2ab6354a0e49859d4cedb247ac391ede6792054c4e1697d2`.
Default ARM64 assembly/lint restored: 20,517,780 bytes, SHA-256
`30f70790abf20e6783d87cb9a2eb5973d63724d638aedee25619349beb1ac6d6`;
physical ARM64 runtime remains untested.
All graphics checks use contained software renderers; AMD hardware remains stopped.
