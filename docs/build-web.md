# WebGL 2 Build and Browser Validation

## Current status

Focused Chrome `--scope materials` covers `litter`, `grass`, `bay-ground` and `wet-sand`,
60 intervals per view, with the existing diagnostic/GL/context gates. This targets all
terrain-normal consumers without replacing the terrain or full input/lifecycle suites.
Twelve CPU-only browser-launcher tests pass. Runtime `20261011T100522.314132Z-chrome-materials`
passes all four views (60 intervals, GL error zero each), images inspected. No host GPU fault;
final kernel delta contains unrelated AppArmor/audit notices. Package 10,347,083 raw /
3,528,841 gzip bytes. See `terrain-surface-frame.md` for scope and Android caveats.

Grass surface material passes contained Chrome/SwiftShader litter/grass scope
`20261009T130317.675153Z-chrome-litter`: 60 intervals each, zero GL errors/context
loss and empty kernel deltas. Grass screenshot inspected. Current package is
10,263,469 raw / 3,498,026 gzip bytes. See `grass-surface-material.md` for filtering
and remaining limits; no new moving-camera, full-input or hardware qualification.

Compact material packaging passes the contained SwiftShader litter/grass scope,
`20261009T123357.811353Z-chrome-litter`: 60 intervals each, zero GL errors/context
loss and empty kernel deltas. Earlier F1 harness failure and corrected acknowledged
gesture are preserved in `browser-input-ownership.md`. Then-validated package is
10,257,711 raw / 3,496,805 gzip bytes. See `prepared-materials.md` for pixel-equivalence
tests, host dependencies and scope limits; this is not full input/hardware qualification.

The fallen-leaf contact update passes contained SwiftShader focused scope `litter`,
`20261009T113543.634482Z-chrome-litter`: litter and grass, 60 intervals each, zero
GL errors/context loss and empty live/final kernel deltas. Both images inspected;
this scope does not capture post-simulation images or repeat input/lifecycle and
moving-camera qualification. Then-validated package: 13,910,123 raw / 6,973,507 gzip bytes.
See `fallen-leaf-contact.md` for shared resource lifetime and remaining visual limits.

The jungle root-contact update passes contained SwiftShader four-view matrix
`20261009T111538.624402Z-chrome-terrain` (overview, beach, east, jungle), 60 intervals
each, with zero GL errors/context loss and empty live/final kernel deltas. All logs
contain the root-contact marker; east/jungle images are inspected. No renewed full
input/lifecycle, moving-camera or eight-view vegetation qualification. Then-validated
package: 13,906,356 raw / 6,970,946 gzip bytes. See `jungle-root-contact.md`.

The forest-floor recruitment update passes contained SwiftShader eight-view matrix
`20261009T104814.928788Z-chrome-vegetation`: all scenes complete 60 intervals with
zero GL errors/context loss and empty kernel deltas. Runtime counts match native:
199 colonies / 866 companions. Jungle and ground-broadleaf frames are inspected.
Then-validated package: 13,902,136 raw / 6,970,160 gzip bytes. See `forest-floor-colonies.md`
for cost, preserved routes and modest trail-camera impact. Full input/lifecycle and
moving-camera qualification were not repeated for this placement pass.

The complete-outline shrub update passes contained SwiftShader real-input walk
`20261009T102949.809142Z-chrome-grass-motion`: 18.1906 m outward, 0.2444 m return
error, 108 distinct recorded frames, 28 shrub identities changing tiers. Zero GL
errors/context loss and empty live/final kernel deltas. Sampled moving images are
inspected; submission telemetry does not prove unobstructed image visibility.
Then-validated package: 13,895,553 raw / 6,968,513 gzip bytes. See `shrub-lod-continuity.md`
for measured geometry coverage/cost. Full browser input/lifecycle and the eight-view
matrix below were not repeated for this localized mesh change.

Supported-shoot shrubs pass all eight contained SwiftShader vegetation views in
`20261009T095827.308359Z-chrome-vegetation`, including the targeted `bush` camera.
Each has 60 presentation intervals, zero GL errors/context loss and empty kernel
deltas. Post-simulation bush and jungle images are inspected. Then-validated
package: 13,893,639 raw / 6,968,980 gzip bytes. See `shrub-structure.md` for mesh
cost and remaining coverage/motion limits. Full input/lifecycle gates were not repeated.

Connected split ground leaves pass all seven contained SwiftShader vegetation
views in `20261009T093110.666658Z-chrome-vegetation`, including `ground`. Each has
60 presentation intervals, zero GL errors and no context loss; kernel deltas are
empty. Post-simulation target and jungle images are inspected. Then-validated
package: 13,877,911 raw / 6,962,710 gzip bytes. See `split-ground-leaves.md` for
the tessellation refinement, extra geometry cost and remaining terrain/LOD limits.
Full browser/input matrices were not repeated.

The juvenile ground-broadleaf update passes all six contained SwiftShader vegetation
views in `20261009T090051.635173Z-chrome-vegetation`, including the new species-targeted
camera. Zero GL errors/context loss; empty kernel deltas; post-simulation plant image
inspected. Package at that milestone: 13,883,396 raw / 6,964,284 gzip bytes. See
`ground-broadleaf.md` for the limited species scope, mesh cost and remaining LOD/art work.
The complete browser/input matrices were not repeated for this geometry pass.

The grass distance-continuity update passes contained SwiftShader walking review
`20261009T083852.440443Z-chrome-grass-motion`: 18.1906 m outward, 1.1761 m return
error, 108 distinct frames, zero GL errors/context loss and empty kernel deltas.
Six sampled frames are inspected. Package at that milestone: 13,881,283 raw /
6,963,488 gzip bytes. See `grass-distance-continuity.md` for the measured cell-cutoff
defect, extra far-instance submissions, and remaining mesh-transition limitations.
Full browser matrices were not repeated for this localized shader/culling update.

Previous 4x MSAA evidence: contained SwiftShader walking review
`20261009T081101.528475Z-chrome-grass-motion` and complete llvmpipe Firefox gate
`20261009T081437.151151Z-firefox-full`, including canvas resize, an inspected resized PNG and
zero GL errors/context loss. Both have empty kernel deltas. Package at that milestone:
13,880,109 raw / 6,962,842 gzip bytes. See `edge-antialiasing.md` for exact scope and added
framebuffer cost. The complete Chrome matrix last passed before MSAA; the focused walking
test does not renew every Chrome gate. Physical GPU/browser performance remains unqualified.

### Earlier foliage-only evidence

The broadleaf-lamina build passes a contained SwiftShader outward/return walking review,
`build/browser-software/20261009T072607.224996Z-chrome-grass-motion/`: 18.3634 m outward, 100
distinct recorded frames, zero GL errors/context loss, empty kernel deltas. The first attempt
failed before travel because capture was not established; its event-order cause remains unproven.
The test now waits for fresh capture acknowledgement while holding the real mouse button and
records browser focus/hit/lock state. See `broadleaf-lamina.md` for both runs and bounded visual
scope. Package at that milestone: 13,871,273 raw / 6,960,777 gzip bytes. Full matrices were pending.

The subsequent pinnate-fern update passes all five contained Chrome/SwiftShader vegetation
views at `build/browser-software/20261009T070325.041949Z-chrome-vegetation/`, with zero GL errors,
no context loss and empty kernel deltas. Post-simulation jungle image inspected. Web
package at that milestone: 13,870,094 raw / 6,960,362 gzip bytes. See `understory-ferns.md`; no hardware or
continuous-motion quality claim is implied by this focused test.

The latest eight-form canopy expansion passes the contained Chrome/SwiftShader terrain matrix
at `build/browser-software/20261009T064451.929486Z-chrome-terrain/`: four views, zero GL errors
or context loss, empty kernel deltas, inspected jungle PNG. Refreshed package: 13,856,503 raw /
6,957,042 gzip bytes. See `canopy-form-expansion.md` for geometry cost and limits. This focused
test does not requalify the full browser/input/audio suite or moving-camera LOD transitions.

The boar per-foot rendered-terrain contact pass runs on SwiftShader in Chrome PID 3902657:
jungle/boars/monkeys/east pass serially with 60 intervals each, no GL errors, lost context,
fallback or limited-reach warning, and nine empty kernel checks including browser exit.
The boar initial and post-simulation images are inspected. Animals retreat into foliage in the
latter; it is runtime evidence, not a close gait-quality certificate. Prior reports/images are
preserved with `.before-boar-ground-contact`. See `boar-ground-contact.md` for CPU/contact scope.

The broad browser/input/audio observations below include pre-incident evidence. Latest foliage,
shoreline and ocean changes have narrower serial SwiftShader qualification described below;
the complete Chrome/Firefox matrix has not been repeated for all those changes. AMD hardware
qualification remains stopped by the GPUVM incident.

The focused `vegetation` software scope includes grass, jungle, fronds, east,
clearing edge, ground broadleaf, split ground leaves (`ground`) and shrubs (`bush`). The targeted
cameras avoid mistaking neighboring bushes for the species being changed. All
eight require current geometry/material diagnostics, the existing shader/inventory
gates and clean kernel deltas. This scope is not hardware clearance.

The latest grass-material pass (Chrome PID 3830423) checks normal grass/jungle/fronds/east on
SwiftShader, 60 intervals each, with nine empty kernel checks and no GL/context/fallback errors.
Grass keeps foliage colour at distance and uses normalized per-fragment basal pigment from its
existing packed alpha byte, with opaque output. Full-resolution grass was inspected; mesh and
instance sizes, textures, population and draws are unchanged. This is software-only validation.
The subsequent CPU moving review (PID 3831033) passes a measured 18.15 m outward/return walk,
44 grass population states, seven stills and 104 distinct recorded frames, with ten empty kernel
checks and no GL/context/fallback errors. A six-frame sheet and the full-resolution farthest
stop were inspected; sparse coverage and geometry LOD changes remain. This is not a claim of
seamless transitions or physical-GPU qualification. Previous reports use `.before-grass-material`.

The latest terrain pass (Chrome PID 3828191) runs overview/beach/east/jungle serially on verified
SwiftShader, 60 frame intervals each. Persistent substrate and smooth slope-light markers are
required; all views pass without GL errors/context loss/fallbacks, with nine empty kernel checks.
The inspected east image retains close clods/leaves and the new 28-byte terrain data path.
This is stationary software shader evidence, not physical-device performance or fallback-path
runtime qualification. Previous terrain reports/stills use `.before-slope-light`.

The latest narrow vegetation pass includes RGB24 rendered-terrain grass grounding, warm
forest humus/filtered fallen leaves and shared eight-lobe canopy LOD with retained original
leaves plus conservative per-tree canopy frustum rejection: Chrome PID 3821108, verified SwiftShader, serial
grass/jungle/fronds/east scenes with 60 presentation
intervals each. All four pass without GL errors, context loss or fallback; all nine kernel
checks are empty, including after browser exit. Native jungle and browser east frames were
visually inspected during the geometry pass; four frozen native before/after culling images are
byte-identical. Mean software presentation intervals are 295.55/241.10/244.99/257.21 ms,
respectively; these shared-host CPU observations are not physical GPU benchmarks. Far canopy
meshes grow from 457 vertices/464 triangles to 997/952; draw counts and texture inventory stay
unchanged. The current normal grass/jungle/fronds/east views reject an additional 39/44/44/67
offscreen canopy instances and submit 236/65/226/116 trees. This is frustum rather than occlusion
culling, not a reduction in visible density. See `canopy-lod-review.md` and `canopy-culling.md`
for isolated image comparisons and geometry cost. Moving-camera
LOD transitions remain a separate review; the preceding soil pass also inspected near-shore sand.
This is software WebGL shader execution, not numerical GPU root-height readback, physical-GPU
performance, or a repeat of the full browser matrix. The first run (PID 3817329) failed the stale
mesh-inventory expectation, not GL/kernel checks; its report/images are retained with
`.failed-stale-canopy-inventory`. Earlier successful references have `.before-retained-canopy-leaves`;
the pre-culling result (PID 3817791) is preserved as `.before-per-tree-frustum`.
See `grass-grounding.md` and `PLAN.md`.

The terrain/traversal/jungle scene builds with Emscripten and runs in Chromium using CNA's `WEBGL2`
renderer. Automated evidence covers renderer initialization, clean short-run lifecycle, live
overview/beach/grass/jungle/prop/driftwood/reef/water/current/distant/shipwreck/crabs/butterflies/turtles rendering, first-click pointer
lock, canvas focus, WASD/arrow traversal, Shift run/walk toggling, simultaneous movement/relative
mouse look, the B camera-motion opt-out, F1 input, pointer release/reacquisition, and responsive
16:9 presentation. The Chrome gate also restarts the production runtime on the same origin and
profile and requires the B selection to survive through CNA's IDBFS storage. F2 world-line rendering is also toggled
through browser input, and F4 must execute its explicit confirmed/unavailable clipboard path. The
first click opens
CNA's 44.1 kHz mixer, starts all four procedural loops,
and prepares both wave/splash and sand/soil footstep pairs plus the pooled positional bird/monkey
call resources. The F2
path includes the same nine terrain-contact samples/normals, attempted/resolved movement trace,
fish-school/shark/monkey navigation guides, three bird route-family volumes, all 24 bird spawn
headings, nearby live bird radial targets, six terrain-following ambient-zone contours, and
positional-source markers.
Audible output on a visible hardware browser, long tab suspension/audio-focus recovery,
hardware-GPU performance, and extended play are not yet validated. Firefox ESR 140.15 now provides
a narrower independent second-engine gate for the production first frame, essential input/audio
contract, and dedicated live jungle, near-surface-water, shipwreck, beach-crab, butterfly, plus
sea-turtle
scenes; it does not duplicate the
complete Chromium scene matrix.
The F1 panel exposes the same authoritative seed rows as desktop. F4 attempts to copy their stable
text report, but browser clipboard permission and user-gesture policy can prevent immediate
readback; the panel then reports `UNAVAILABLE` instead of claiming success.

The renderer gate also requires the exact shore-following animated foam ribbon rather than its
stock-effect fallback. The close prop camera additionally requires a nonzero terrain-conforming
contact-shadow population in exactly one portable draw. The beach and grass cameras require the
five-level near-blade curvature, portable two-sided daylight path, selected near-blade geometric
V-fold ridge, and narrower asymmetric/occasional bent-tip close-blade geometry rather than only
accepting the older flat-colour shader. The lifecycle,
overview, beach, and grass cameras also require the
coherent coast/path/clearing grass biome-colour path. The lifecycle plus close grass and jungle
cameras also require the sparse near seed-head path. The lifecycle plus close grass and jungle
cameras require the near-field
terrain microvariation and procedural leaf-litter path. The lifecycle and dedicated jungle camera
also require the inland humus/moss material diagnostic, preventing a silent return to the former
uniform green interior substrate. The lifecycle and dedicated jungle camera
also require derivative-based, two-sided fixed-sun facet lighting on the instanced vegetation
shader plus sparse hanging leafy lianas on selected near-canopy variants rather than accepting its
unlit or less-layered fallback. The lifecycle, beach approach, and jungle camera
also require the authored primary-footpath terrain material diagnostic. Dedicated east/west
clearing cameras require their shared authored-material and reused-noise broad-patch diagnostics
plus exact QA-camera identity.
The east camera additionally requires the explicit warm-dry ground-cue diagnostic.
The lifecycle, beach approach, and jungle gates additionally require the narrow irregular-wear
diagnostic, preventing a silent return to the former broad, uniformly coloured path treatment.
The lifecycle and beach gates also require the sparse close sand pebble/shell diagnostic; dedicated
driftwood and fallen-frond captures expose that material at first-person scale.
The lifecycle and dedicated driftwood gates also require tapered three-dimensional branches on all
three near driftwood variants while retaining the established three-limb-fork diagnostic. They
also require the shared irregular-body/end-cap-ring diagnostic.
Both clearing cameras also require the shared five-rock/two-log authored-landmark diagnostic.

The latest automated run used Chrome 152.0.7977.82 with SwiftShader and confirmed exact F4
clipboard readback on its focused localhost canvas. All 27 reported lifecycle, deployment,
renderer, scene, and input stages passed against CNA `next` `97783d1e090c`; the current rebuilt
WebAssembly is 8,818,462 bytes. The close-palm stage also requires the sparse hanging dry-frond
diagnostic, and the jungle stage requires the irregular layered lower-crown plus irregular-trunk/
bent-support-branch markers. The dedicated
shipwreck stage requires the exact 20 m broken-hull,
12-debris, large-ship-detail, one-draw diagnostics plus the portable aged-wood/waterline material
marker; its refined curved hull, broken rails/foremast, keel/rudder, side openings, and irregularly
scattered debris frame was inspected. A separate crab
stage requires the thirty-agent colony marker, a nonzero visible population, and exactly one draw;
its original-resolution frame was also inspected. The butterfly stage likewise requires the
eighteen-agent habitat marker, a visible near population, and exactly one draw. The turtle stage
requires all five agents, a visible near turtle, and exactly one draw. The independent Firefox ESR
gate requires and captures all four live wildlife/landmark scenes. The deployment
gate additionally requires the exact persistent-
ambient DC-centering and land-footstep resource diagnostics after browser-gesture audio startup.
The monkey scene requires the
near crown/cheek-tuft and bone-following finger/toe diagnostics, and its inspected frame retains
the complete facial-detail/crossfade path. The input
stage captures and visually validates the warm-gold `RUN MODE  8.0 M/S` confirmation while still
proving that releasing Shift retains running and a second press restores walking. The input path
also requires a successful distance-scheduled land-footstep playback diagnostic while
the player actually traverses the beach. The first-frame
gate also requires the reported 1280 by 720 logical
backbuffer rather than inferring render resolution from the browser surface. This records the
evidence environment; it is not a minimum-version or general browser-permission claim.

## Prerequisites

- host Python 3.10 or newer and Pillow (`python3-pil`) for incremental material preparation;
- the Emscripten SDK at `/home/robertvokac/emsdk`, or another installation exposed through `EMSDK`;
- Google Chrome or Chromium for the automated browser smoke test;
- Firefox ESR with its built-in WebDriver BiDi endpoint for the optional second-engine smoke test;
- the same exact read-only dependency revisions listed in `dependencies.lock`;
- the CNA SDL submodule snapshots already populated in the source CNA worktree;
- the shared compiler cache at `/rv/cnaccache` in this workspace.

`tools/materialize_cna_next.sh` copies the three pinned SDL snapshots into the ignored CNA source
export after verifying that the containing checkout and CNA `next` use identical SDL gitlinks. It
does not initialize, switch, or edit the CNA repository. Configure-time SDL build products go to
the writable project cache `build/cna-sdl-prebuilt-emscripten`, never into the CNA export.

## Configure and build

```bash
source /home/robertvokac/emsdk/emsdk_env.sh
export CCACHE_DIR=/rv/cnaccache
export CCACHE_BASEDIR=/rv
tools/materialize_cna_next.sh
cmake --preset web-webgl2
cmake --build --preset web-webgl2
```

The first configure builds CNA's pinned SDL, SDL_image, and SDL_mixer for Emscripten with a hard
two-job ceiling. Later configures reuse their content-fingerprinted cache. The game build also uses
two jobs because it compiles Sharp Runtime.

The deployable files are written to `build-probe/`:

- `island-simulator.html`
- `island-simulator.js`
- `island-simulator.wasm`
- `island-simulator.data`

The required `.data` file preloads the prepared material PNGs, including the hash-gated v1
forest-floor image at `/Content/terrain/forest-floor.png` and the board-local shipwreck wood at
`/Content/structures/shipwreck-weathered-wood.png`. Serve it alongside the other three files as
`application/octet-stream`; omitting it blocks Emscripten startup. The runtime material validates
the prepared dimensions and builds complete mip chains. Geometry, shaders, remaining small textures,
animation, font and PCM are generated from project code. Rejected studies and the original
Android launcher vector are not included. See `forest-floor-material-review.md` for evidence.

The floor integration passes the serial Chrome/SwiftShader terrain gate (PID 3910489):
overview, beach, east and jungle, 60 intervals each, no GL errors or context loss and nine
empty kernel checks. The actual jungle screenshot matches the native material. Reports and
images before this pass are retained with `.before-floor-runtime`. The four-file package totals
13,854,483 bytes or 6,954,937 gzip bytes; `.data` is 4,428,065 bytes and hashes identically to
the source PNG. This is a functional software-renderer result, not hardware performance.

For a release host, create an integrity-checked package containing both the original files and
deterministic gzip variants:

```bash
tools/package_web_release.sh build-probe
```

The package is written to `build-probe/web-release/`. Its `manifest.json` records the MIME type,
uncompressed size, SHA-256 digest, and gzip size of every artifact. The historical pre-floor package was
9,085,111 bytes uncompressed and 2,587,860 bytes through gzip, a 71.5% transfer reduction. HTML is
2,058 bytes (1,144 gzip), JavaScript is 264,591 bytes (59,252 gzip), and WebAssembly is 8,818,462
bytes (2,527,464 gzip). These
are payload sizes, not browser decoded-memory or cold-load timing measurements. The validation
test round-trips every compressed artifact, verifies the hashes and manifest totals, rejects a
missing input before creating partial output, and proves that repeated packaging is byte-stable:

```bash
ctest --test-dir build -R '^IslandWebPackageValidation$' --output-on-failure
```

A production host should serve the matching `.gz` file when the request accepts gzip and add
`Content-Encoding: gzip` plus `Vary: Accept-Encoding`. Preserve `text/html; charset=utf-8` for
HTML, `text/javascript; charset=utf-8` for JavaScript, and `application/wasm` for WebAssembly.
Never return a `.gz` file without its content-encoding header; clients without gzip support should
receive the uncompressed file. The following development server deliberately serves the original
files and does not qualify compressed production delivery.

Serve the directory over HTTP; browsers do not support this package through `file://`:

```bash
python3 -m http.server 8765 --bind 127.0.0.1 --directory build-probe
```

Then open `http://127.0.0.1:8765/island-simulator.html`.

The deployable HTML is produced from `tools/web_shell.html`. It supplies a 1280 by 720 render
surface, preserves 16:9 while fitting different browser windows, displays WASD/arrow movement and
the persistent Shift walk/run toggle alongside the current diagnostic/audio controls,
and requests pointer lock within the first mouse gesture instead of requiring a confusing second
click. The C++ controller detects pointer-lock loss and permits clean reacquisition. Web Escape
never terminates the game loop, including on browsers that forward the key after releasing the
lock; the tab remains under browser ownership.

## Automated browser smoke

The floor-filter update is qualified by
`build/browser-software/20261009T062644.751882Z-chrome-terrain/`: four views pass under Chrome
152 SwiftShader, host PID 3927812, 4x anisotropic selection, zero GL errors/context loss and
empty kernel deltas. The jungle PNG was inspected. Current Web package integrity checks pass
at 13,855,342 raw / 6,955,371 gzip bytes. This is software/static-view coverage; Firefox,
moving-floor temporal review and physical GPU performance remain separate outstanding gates.

Linux browser harnesses must now be launched through `tools/run_browser_software.py`, not by
calling their Node entry points directly. The gate requires bubblewrap, a Node runtime with
global WebSocket, and host kernel-journal permission. It shares the native/Android qualification
lock, caps affinity to eight available CPUs, continuously monitors the kernel, and contains the
whole browser/server tree in a PID namespace. A private `/dev` omits host GPU devices; display
variables are removed and software rendering is forced. There is no uncontained fallback.

Each run has a timestamped `build/browser-software/` evidence directory containing commands,
package/launcher hashes, host/namespace PID attribution, logs, kernel deltas, reports and captures.
Older `build-probe/island-browser-*` results are historical and are no longer overwritten.
Scene reports qualify browser assertions only; **the parent `run.json` and kernel evidence are
authoritative for overall acceptance**. Profiles are removed on ordinary exit; a forced shutdown
can retain a profile inside the owned evidence directory for diagnosis.

### Focused, software-only wildlife and foliage check

For terrain distance and close-material review, use `--scope terrain` with the
same software-only command below (a 240-second deadline is sufficient for this bounded scope).
It visits overview, beach path, east clearing and jungle serially, requires the current persistent
substrate and smooth-slope-light diagnostics, and records 60 frame intervals plus before/after-simulation stills per view.
Reports use the `island-browser-terrain` prefix. This checks portable shader execution, not a
continuous moving-camera sweep, hardware performance, or resolution of the AMD incident.

During the unresolved AMDGPU incident, prefer this bounded check for the current boar and canopy
changes. It is **not** the complete input/audio/browser suite or AMD qualification. With the
Emscripten environment active and kernel-journal permission in the current session:

```bash
python3 tools/run_browser_software.py build-probe --node "${EMSDK_NODE}" \
  --browser chrome --scope wildlife-foliage --timeout 240
```

If the session predates membership of `systemd-journal`, enter `sg systemd-journal` first, then
activate the Emscripten environment and run the command. It does not require root. Node must
provide global `WebSocket`; the workspace's system Node 20 does not, while Emscripten's Node 24
does. Unsupported Node is rejected before creating a profile or browser process.

The harness checks Chrome's actual SwiftShader renderer **before loading the game**, then checks
the WebGL 2 context's renderer too. It visits jungle, boars, monkeys, and the east clearing strictly
serially, closes each page/context, rejects shader fallbacks/GL errors/context loss, and records
60 animation-frame intervals per scene. These are software presentation intervals, not GPU
benchmarks. The outer supervisor polls kernel records throughout the run and checks a final
delta after cleanup; scene stage markers are not independent kernel checks. The gate never
clears the hardware qualification latch and does not qualify AMD hardware.

Results: `build-probe/island-browser-wildlife-foliage.json`,
`build-probe/island-browser-foliage-kernel.json`, and `island-browser-foliage-*.png`. Both JSON
reports must pass and the process must exit zero. Preserve useful comparison files before a
subsequent run, because the ordinary filenames are reused. The initial October 8 check passed
on Chrome 152 / SwiftShader and visually matched the native llvmpipe canopy and boar presentation;
it does not qualify physical Android or AMD hardware.

For focused shoreline validation, use `ISLAND_WEB_SMOKE_SCOPE=coast` and a 240-second outer
deadline with the same software-only flags, affinity and lock. It visits shipwreck, water, west
and reef serially, checks the current porous-foam diagnostic, samples 120 frame intervals per scene,
and saves a second screenshot after simulation. Reports are `island-browser-coast.json` and
`island-browser-coast-kernel.json`; images use `island-browser-coast-*.png`. Inspect both times,
not just an image-statistics pass: a crest should advance and dissipate, not flash at cycle wrapping.
This focused scope leaves previous wildlife/foliage evidence untouched and does not replace full
browser, input, audio or hardware qualification.

The October 9 bounded-surface response passes this scope in Chrome PID 3753708, with 120
intervals for each of the four views, zero GL errors/context loss/fallback and nine empty kernel
checks. Earlier coastal artifacts use `.before-banding`. Water/wreck frame pairs were inspected;
water changes over time but still has conspicuous wave-family regularity. See `PLAN.md` for the
next spectral-quality work; this pass is not AMD incident closure or mobile performance evidence.

The subsequent wind-packet/direction-spread/broad-normal-Fresnel pass succeeds in Chrome PID
3759513 on SwiftShader: all four views, 120 intervals each, zero GL errors/context loss and nine
empty kernel checks. Earlier images/reports use `.before-packets`. Inspected water/wreck images
show reduced strong crosshatch, not final realistic water. The water-only paired-frame mean
absolute difference is 0.0103374 on [0,1], confirming continued motion. Mean presentation
intervals are 90.97/198.19/241.52/95.97 ms for wreck/water/west/reef; shared-host software timing
does not isolate shader cost or predict physical GPU performance. Android is cross-built only.

For close vegetation review, use the monitored, device-isolated supervisor:

```sh
sg systemd-journal -c 'python3 tools/run_browser_software.py build-probe --node /home/robertvokac/emsdk/node/24.19.0_64bit/bin/node --browser chrome --scope vegetation --timeout 660'
```

The absolute Node path above is this workspace's runtime; other developers should
pass their Emscripten Node executable. The eight views listed above run serially,
with 60 presentation intervals and before/after-simulation images each. Only one
game context is live at a time. Evidence is retained under the timestamped
`build/browser-software/` directory, including `island-browser-vegetation.json`,
the parent result, and live/final kernel deltas. This is visual/animation coverage,
not a hardware performance benchmark or the full browser/input suite.

The following four-view results are historical, before this scope was extended:
The October 9 rest-shape grass-normal pass succeeds in Chrome PID 3762631 / SwiftShader:
all four scenes, 60 intervals each, zero GL errors/context loss and nine empty kernel checks.
Grass and jungle post-simulation images were inspected; broad/angular blade silhouettes remain
a known visual limitation despite corrected rest-shape lighting.
The following progressive-taper/seed-attachment pass also succeeds (Chrome PID 3765621): all
four scenes, 60 intervals each, zero GL errors/context loss and nine empty kernel checks. The
grass post-simulation image was inspected. Earlier vegetation images/reports remain with the
`.before-taper` suffix. This refines silhouette and attachment, not physical-GPU qualification.
The subsequent unequal-tuft layout passes the same scope in Chrome PID 3768713: four scenes,
60 intervals each, zero GL errors/context loss and nine empty kernel checks. Close grass and
wider jungle images were inspected. Pre-tuft evidence uses `.before-tufts`; moving-camera LOD
continuity and physical-GPU performance are not established by this stationary software scope.
The measured-mesh/shared-motion culling correction also passes (Chrome PID 3771308): four
scenes, 60 intervals each, zero GL errors/context loss and nine empty kernel checks. Prior reports
and images use `.before-motion-bounds`. The new bounds have independent CPU containment tests;
this browser run verifies shader portability and scene regression, not moving-edge continuity.
The final low-turf/shared-tip version passes in Chrome PID 3780632: the same four scenes and
60 intervals each, no GL errors/context loss/fallback and nine empty kernel checks. Near grass
adds 48 short curved blades (144 triangles per near instance, no additional draw); middle/far
meshes are unchanged. Close grass/east screenshots were inspected. Pre-layer evidence uses
`.before-low-turf` and the higher-cost initial tip topology uses `.before-shared-turf-tips`.
This is only a modest near-field coverage improvement; sparse ground and exact terrain-root
conformance remain open, and current physical-GPU/mobile performance is not qualified.

`ISLAND_WEB_SMOKE_SCOPE=grass-motion` uses the same software-only command and a 240-second
deadline for a measured walking review. It launches interactive input diagnostics, uses the
existing F7 diagnostic teleport once to set up the east clearing, hides the overlay, acquires
pointer lock, and drives the normal player with W then S toward projected distances
0/6/12/18/12/6/0 m. Position feedback, not key-hold wall time, determines each stop. The driver
waits for a fresh stationary sample before each of seven PNG captures and releases held keys
even when a movement stage fails. Each movement/stop wait has a 15-second deadline. The driver
learns release-to-stop travel from measured progress, releases early on later legs, and permits
at most two corrective attempts per target. It uses actual W/S input throughout; the original
2.5 m capture/return tolerances and 17 m minimum outward coverage are unchanged.

`island-browser-grass-motion-traversal.json` preserves telemetry, stages and any partial failure;
`island-browser-grass-motion-output.log` retains the complete game log after route success/failure.
`island-browser-grass-motion.json` contains the scene result and 30 subsequent presentation
intervals. The separate `island-browser-grass-motion-kernel.json` includes browser-exit checks.
Successful coverage requires maintained pointer lock, no free camera, heading/lateral bounds,
distance targets within 2.5 m, return within 2.5 m and at least four distinct visible grass LOD
population tuples. Opt-in diagnostics additionally serialize the previous draw's actual canopy
batches using millimetre X/Z IDs and near/far tiers. Duplicate IDs and mismatched populations are
errors. The review requires a changed tier for at least one tree present in adjacent samples,
ignoring entries/exits. Whole-cell frustum rejection means submitted trees are not necessarily
visible pixels. These are sampled previous-render states, not the exact screenshot frame or
every transition. Inspect the PNGs: these telemetry gates alone cannot establish
smooth LOD blending, absence of transient popping or island-wide collision/culling correctness.

After the canopy outline, retained far branches and height-based trunk pigment passes, the
current route succeeds with Chrome PID 3898134: 94 samples, 44 grass LOD population states,
19 observed canopy switches across ten trees, 18.22 m outward progress and 1.0033 m return error.
All ten kernel checks are empty and no context loss/GL error is observed. The recording retains
101 distinct compositor frames (2,765,421 bytes), untruncated. Frames 003/004, 005/006 and 087/088
were inspected in `island-trunk-pigment-motion-review-3898134.png`, along with stationary step 3.
There is no gross scene/culling failure in those samples, but their 640x360 JPEG resolution and
approximate telemetry association cannot prove that individual detail switches are imperceptible.
Previous reports, log and stationary PNGs are preserved with `.before-trunk-pigment` suffixes;
PID-named frame recordings remain separate. Current source still needs broader real-GPU and
physical-device qualification after the unresolved AMD incident is safely addressed.
The pure parser and route gates also have CPU-only Node tests registered with CTest when Node
is available. No AMD hardware or Android runtime qualification follows from this software scope.
The corrected October 9 run (Chrome PID 3775097) passes: 97 samples, 50 aggregate LOD tuples,
19.582 m maximum recorded outward distance and 1.648 m return error, seven inspected images,
30 final frame intervals, no GL errors/context loss/fallback and ten empty kernel checks.
Earlier `.failed-fixed-camera` artifacts preserve the harness setup failure, not a driver fault.
Sparse grass and smooth substrate remain visible; this pass does not close continuous LOD review.

The newer per-tree-culling walk (Chrome PID 3821826) passes the same gates: 91 telemetry samples,
43 grass populations and 18 observed LOD switches across nine submitted trees. Progress is
0/7.5545/12.1706/17.9334/10.5222/5.7628/0.6450 m; all stages finish without corrective attempts.
It retains 99 distinct frames/2,681,082 bytes over 26.700 s with verified hashes and no truncation
or recording errors. Ten empty kernel checks and zero GL/context/fallback failures; 30 final
intervals average 243.33 ms in software. The farthest stop and six-frame edge/transition sheet
`island-canopy-culling-motion-review-3821826.png` were inspected without gross disappearances in
that portion. This is not exhaustive edge, all-variant or seamless-LOD qualification. Previous
walk reports/stills are `.before-per-tree-frustum`; PID-named recordings remain intact.

The earlier canopy/soil walk (Chrome PID 3808640) passed: 105 telemetry samples, 53 grass population
tuples and 44 observed tier switches across 18 submitted trees. Seven measured stops are
0/5.3033/13.0011/17.8322/11.5678/5.7333/-2.2511 m. One corrective reversal at the first target
accounts for extra switches on some trees; four switches are not alone evidence of LOD jitter.
The final 30 presentation intervals average 254.43 ms under SwiftShader, not physical-GPU timing.
All ten kernel checks are empty and GL/context/fallback gates pass.

Between the first and last stop, CDP compositor capture saves up to 240 JPEGs/16 MiB, at up to
640x360 and quality 65. Frames use PID-qualified filenames, SHA-256 hashes, compositor timestamps
and an explicitly approximate link to the most recently read telemetry. The report records
truncation and write/acknowledgement errors; stopped writes are drained before reporting. This is
not lossless video, every game frame, or a 60 fps review. The successful run retains 111 distinct
frames/2,967,511 bytes over 31.695 seconds, without truncation/errors; all file hashes were checked.
The six-frame sheet `island-canopy-motion-review-3808640.png` inspects one portion of that sequence.
Coarse distant crown shapes remain visible. The subsequent full-resolution isolated comparison
in `canopy-lod-review.md` improves one partly occluded tree; broader moving-camera comparisons remain
needed before claiming seamless changes. The short route does not fully traverse the far soil fade.

The first recorded run, PID 3807576, failed only the unchanged return-distance gate (2.6517 m).
`.failed-open-loop-release` reports/log/stills and its 105 PID-qualified frames preserve that
harness failure; no GPU error occurred. The earlier walk is preserved as `.before-canopy-trace`.

For land-prop orientation/grounding review, `ISLAND_WEB_SMOKE_SCOPE=grounding` uses the same
software-only flags, lock, eight-core affinity and 240-second deadline. It visits props, driftwood,
contact and reef serially, samples 60 intervals each, requires the current land-shadow diagnostic
and a nonempty single shadow draw, and retains before/after PNGs. Reports use
`island-browser-grounding.json` and `island-browser-grounding-kernel.json`. Its GL/runtime gate
does not run the native PPM luminance threshold: passing it does not supersede a failed native
image-statistics test. The first October 8 native grounding-only driftwood view failed that
threshold at 17.9037 versus 18.0; its retained evidence and follow-up are in `PLAN.md`.
The corrected grounding route passed in Chrome PID 3741656: four scenes, 60 intervals each,
zero GL errors/context loss/fallback and nine empty kernel checks. The earlier incomplete run
remains as `.before-contact-routing`. Post-simulation props/driftwood images were inspected and
match native orientation. A later native wood-normal correction passes the unchanged threshold
at 18.49; the failed image remains available for comparison, and neither metric proves final art quality.
The wood-normal version subsequently passed the same four-view gate in Chrome PID 3745118,
including reef regression coverage, zero GL errors/context loss/fallback and nine empty kernel
checks. Previous grounding reports and images are retained with `.before-wood` suffixes. This
does not requalify the full browser input/audio matrix or hardware rendering.

The October 8 porous-foam pass passed this coastal scope in Chrome PID 3713011: four scenes,
120 intervals each, no GL errors/context loss/fallback, and nine empty kernel deltas. Paired wreck
images show changed crest position and porous coverage. This observation does not prove every
animation phase is artifact-free or measure physical GPU performance.

The subsequent shared ocean-spectrum pass passed the same coastal gate in Chrome PID 3717051
(four scenes, 120 intervals each, no GL errors/context loss/fallback and nine empty kernel deltas).
Earlier porous-foam-only reports and paired images are retained with `.before-spectrum` suffixes.
Current paired wreck/water frames show the added normal detail; repeated wave trains still look
stylized. The full browser input/audio matrix and hardware performance were not requalified.

The following coastal-optics pass also passed this scope in Chrome PID 3720281, with four scenes,
120 intervals each, zero GL errors/context loss/fallback and nine empty kernel deltas. Paired
shipwreck/water images show the continuous sand substrate through shallow water. Spectrum-only
comparison artifacts carry `.before-optics`; current reports again use the ordinary coastal names.

Coastal contact patches passed the same gate in Chrome PID 3724748: shipwreck/water/west/reef,
120 intervals each, zero GL errors, fallback or context loss and nine empty kernel deltas.
Inspected post-simulation wreck/reef images show restrained grounding below the opaque props and
water. Previous optics-only reports/images are retained with `.before-contact`; ordinary coast
names now refer to this pass. The mean CPU-rendered intervals were 68.47/186.52/292.07/110.41 ms;
these are not a controlled performance comparison or a physical-GPU qualification. Native Android,
the full browser input/audio matrix and AMD hardware remain unqualified for this change.

The smooth-stone pass also passes in Chrome PID 3734479, including the derivative-safe final
filter: four coastal scenes, 120 intervals each, no GL errors/fallback/context loss and nine empty
kernel deltas. The inspected after-simulation reef frame has the same smooth daylight/mineral
surface as native llvmpipe. CPU means: 85.14/207.91/243.19/87.50 ms; hardware cost remains unknown.
Contact-only comparison files use `.before-stone`; the first stone shader's passing evidence is
retained as `.before-stone-filter`. Current ordinary coast files describe the final filter.

The subsequent placed-canopy transmission field also passed this focused gate (Chrome PID
3661024), with inspected jungle/boar/clearing images and seven empty kernel deltas. Both terrain
and grass use the same 4 MiB field on WebGL 2. Earlier noise-only ground lighting was removed;
the former pre-canopy reports/images are retained with `before-canopy` suffixes. The complete
input/audio suite has not yet been rerun for this revision.

The low-jungle receiver extension also passed the three-scene gate (PID 3666026). Understory,
bushes and ground plants share the texture without shading the crown draws. Inspected native
and browser woodland images agree; seven kernel checkpoints were empty. Previous comparison
reports/images use the `before-understory-shade` suffix.

Boar per-vertex CPU light sampling and the shared CPU/GPU ambient tint passed the same gate
(PID 3670400), again with inspected frames and seven empty kernel deltas. Its prior comparison
files use `before-boar-shade`. GPU texture storage is unchanged; the CPU retains 1 MiB of
transmission samples. These focused checks do not replace hardware or full input/audio tests.

The monkey material extension adds a fourth scene and records `shadedMonkeyPathObserved`
separately from basic WebGL success. The first four-scene run (PID 3679560) passed, with nine
empty kernel deltas and natural visible-monkey shade events in every scene (transmission
approximately 0.86–0.89). Initial fixed captures were fully sunlit; later runtime events establish
active shade coverage. Previous reports/images use `before-monkey-shade`. Monkeys reuse the
existing CPU field and stock skinning; no extra texture, geometry or draw pass was added.

### Complete browser suite

With the Emscripten environment active, run:

```bash
python3 tools/run_browser_software.py build-probe --node "${EMSDK_NODE}" \
  --browser chrome --scope full --timeout 1200
```

The test uses headless Chrome with SwiftShader to create a real WebGL 2 context. It first runs the
game with `--smoke-test`, requires finite timings for all first-draw startup phases, and requires a
clean exit after four frames. It also requires the portable
terrain-material, sky, and ocean shaders and the hardware-instanced palm, grass, jungle, and
natural-prop shaders rather than their fallbacks. Lifecycle, overview, beach, near-surface-water,
and offshore-current stages additionally require the distance-faded reused crossing-wave colour
plus reused-slope shallow-ridge, sunward-crest, and grazing-reflection diagnostics. The harness
then opens live fixed
overview, first-person
beach, grass interaction, folded-split-ground-plant, sparse fallen-leaf detail, segmented-palm and
bark/coconut detail, jungle,
east clearing, west clearing, prop, driftwood, reef, near-surface water, offshore-current,
fallen-frond, distant-horizon, bird, fish, shark, monkey, 20 m shipwreck, beach-crab, butterfly,
and sea-turtle
instances
at a 1280 x 720 viewport.
Every scene asserts the
layered-horizontal-wisp fixed-sky shader diagnostic.
The
captures are saved as `build-probe/island-browser-smoke.png`,
`build-probe/island-browser-beach.png`, `build-probe/island-browser-jungle.png`,
`build-probe/island-browser-grass.png`,
`build-probe/island-browser-ground.png`,
`build-probe/island-browser-litter.png`,
`build-probe/island-browser-palms.png`,
`build-probe/island-browser-east-clearing.png`,
`build-probe/island-browser-west-clearing.png`,
`build-probe/island-browser-props.png`, `build-probe/island-browser-driftwood.png`,
`build-probe/island-browser-reef.png`, `build-probe/island-browser-water.png`,
`build-probe/island-browser-current.png`, `build-probe/island-browser-fronds.png`,
`build-probe/island-browser-distant.png`,
`build-probe/island-browser-birds.png`, `build-probe/island-browser-fish.png`,
`build-probe/island-browser-shark.png`, `build-probe/island-browser-monkeys.png`,
`build-probe/island-browser-shipwreck.png`, `build-probe/island-browser-crabs.png`,
`build-probe/island-browser-butterflies.png`, `build-probe/island-browser-turtles.png`, and
`build-probe/island-browser-run-mode.png`. The beach
view must report nonzero grass; the dedicated grass view must report full smoothed player
interaction on the custom shader, and both views require its curved two-sided daylight path. The
grass view also requires the per-instance blade-stature and near juvenile-underlayer diagnostic,
preventing the close tier from silently reverting to the more repetitive 28-blade cluster. The
same stage requires the sparse near seed-head diagnostic, which proves that the restrained
seed-bearing stems use the portable shared-mesh path. Run and walk speed sampling polls observed
movement until the strict 7.0/3.5 m/s thresholds are reached, avoiding a fixed-duration
SwiftShader scheduling assumption while preserving the speed and persistent-mode checks. The
production input stage then disables camera motion, waits for the settings write, closes that
runtime, and opens a second instance on the same origin and browser profile. The second instance
must report the stored disabled state, proving an actual IDBFS-backed cross-runtime round trip
rather than same-process state retention. Fixed-view and short-run QA pages pass
`--no-user-settings`, so scene validation cannot overwrite player preferences. The grass view
also requires nonzero near, middle, and far populations in at least one mixed cell,
proving that WebGL 2 honours the nonzero instance-buffer offsets used by the adjacent-tier
62--78 m / 86--102 m LOD splits. The
dedicated ground view requires the upright/split-leaf diagnostic, exact variant-2 QA target, and a
nonzero per-instance near-LOD forest-floor population. The
palm view requires the fuller segmented-leaflet, bark/coconut, sparse hanging dry-frond, and
transform-derived frond-tint diagnostics plus nonzero near/far populations. The jungle
view requires the branch-supported canopy, irregular layered lower-crown edge, irregular near
trunks with bent two-segment support branches, sparse hanging
leafy lianas, procedural-bark, and pointed-leaf near-bush diagnostics,
nonzero near/far
canopy populations, understory, low bushes, and forest-floor plants. The prop view must report
visible fallen logs, restrained radial end-grain cracks, plus a nonzero one-draw contact-shadow
batch. The driftwood view frames the
authored cove group and must report its exact three-piece landmark diagnostic, the three-limb
silhouette diagnostic, visible beach wood plus shell/pebble/dry-wrack clusters, and a nonzero
per-instance near-LOD population. The reef view must report a visible emergent outcrop plus the
near-rock fixed-sea-level wet-band, irregular-crown/facet-light, and transform-derived stone-tint
diagnostics. The water view must
report a nonzero generated
depth/scatter/edge cue, exactly one overlay draw, and a
nonzero player ripple plus a nonzero finite shoreline-transition burst integrated into the existing
ocean submission, the
current view must additionally report nonzero normalized current strength and undertow-audio
target, the frond view must report visible grove-associated fallen fronds and require their curved
asymmetric-leaflet diagnostic, the distant view must report all five unreachable
islets in exactly one portable batch, and the bird/fish views must report nonzero
visible populations from the 24-agent flock and 48-agent school population. The bird view also
requires the layered near trailing-flight-feather diagnostic. The fish view also
requires the exact 414/12-vertex, single-surface-fin, CPU-faceted daylight diagnostic, the
seven-station/ten-facet anatomy marker, and the shared depth/sun-band lighting diagnostic. A
focused `--scope fish` run additionally requires two different-hash separated-time frames, 60
positive intervals, SwiftShader, an intact context and GL error zero. The shark view must report
the active offshore patrol, exact 834/15-vertex inventory, eight-station/ten-facet anatomy marker
and the same lighting diagnostic. A focused `--scope shark` run additionally requires two
different-hash separated-time frames, 60 positive intervals, SwiftShader, an intact context and GL
error zero. A focused `--scope dolphins` run requires the 708/15-vertex inventory and current
short-beak/faceted-eye/shaped-fin marker, a visible near member of the four-animal pod, two
different-hash separated-time frames, 60 positive intervals, SwiftShader, an intact context and GL
error zero. The monkey view must report a visible member of the
seven-agent GPU-skinned troop. The shipwreck view requires the exact 20 m hull, twelve-piece debris
trail, portable aged-wood/waterline shader, and one visible draw; the crab view requires a nonzero population from all thirty agents in
one draw. The butterfly view requires a nonzero population from all eighteen agents, nonzero near
hinged-wing detail, and one draw. The turtle view requires the five-agent near-shore route marker,
a visible near turtle, and one draw. The
monkey startup diagnostics must also prove that the local-pose crossfade path and both near-only
silhouette details are active. The
first-person runs enable the runtime-generated debug font and overlay, covering the WebGL 2
`SpriteBatch` composition path; the input-stage image records the short normal-play confirmation.
The machine-readable result is `build-probe/island-browser-smoke.json`.

The 2026-10-06 startup-instrumentation run reported 134.8 ms deterministic world generation,
246.8 ms content creation, 15.7 ms game-owned first draw, and 438.0 ms total in SwiftShader. This
qualifies the portable timing path, not hardware-browser cold-load performance or GPU completion.

The harness then opens a live controllable instance and uses Chromium's input protocol to click
the canvas, assert pointer lock/focus, initialization of all four looping audio layers, preparation
of both wave variants, both shoreline-transition variants, all four sand/soil footsteps, and positional
bird/monkey calls. It also requires the slow non-amplifying ambient-gain path after the bounded
sequential source upload, then holds W while applying repeated pointer movements. The same
diagnostic segment
must show both player displacement and changing yaw. It resets to the deterministic south-beach QA
location, proves that Right moves toward visible screen-right, proves that Shift+Up exceeds the
walking ceiling after Shift has been released, proves that a second Shift press restores walking,
captures the first toggle's transient HUD, requires an actual land-footstep playback, and then
walks backward into the ocean. The shoreline crossing must produce a successful water-transition
one-shot and then enter the authoritative swimming state. The harness then enables the portable F2
world line view, toggles F1, resizes to a
non-16:9 browser window, sends a real Escape key, proves that pointer lock is gone while new game
frames continue without exit/error, waits through Chromium's immediate-relock security cooldown,
and clicks to reacquire. It records the initial/final player and viewport samples plus the Escape
state in the JSON report. Chromium's synthetic page-freeze command did not yield reliable resume
evidence in this environment, so the Chrome result does not infer tab recovery. The independent
Firefox gate below now qualifies one short actual active-tab switch through WebDriver BiDi.

Discrete input checks wait for the corresponding authoritative game diagnostic and retry a bounded
number of times. The backward shoreline traversal likewise remains held until the real
walking-to-swimming transition is reported, up to a bounded timeout; the preceding water accent
remains independently required. This keeps SwiftShader frame-time spikes from turning
brief DevTools key edges into false failures while still requiring the game to observe every tested
control and state transition.

This smoke test proves browser/WebAssembly/WebGL execution, bounded one-source-at-a-time staging,
first-gesture CNA mixer/track initialization, activation of the slow ambient-gain path, acceptance
of the shoreline transition one-shot, and a resolved transition into swimming,
but it is not a substitute for
a visible interactive listening pass using hardware acceleration. Do not infer audible quality,
long tab suspension, frame pacing, thermal behaviour, or compatibility beyond the separately
qualified Firefox subset from it.

## Firefox ESR second-engine smoke

Run the narrower production-shell gate with the same Node executable:

```bash
python3 tools/run_browser_software.py build-probe --node "${EMSDK_NODE}" \
  --browser firefox --scope full --timeout 1200
```

The Linux tool requires `Xvfb`, starts an owned automatically allocated virtual display,
a local-only HTTP server and a clean Firefox ESR profile, inherits the
supervisor's limit of eight available CPUs, and talks directly to Firefox's built-in WebDriver BiDi endpoint. No
geckodriver, browser extension, npm package, or network service is required. Override the browser
binary with `ISLAND_FIREFOX` when the installation is named `firefox` rather than `firefox-esr`.
The contained gate additionally requires proof of a software WebGL renderer on a blank page
before loading the game. Masked/unavailable renderer identity is rejected, not guessed. Earlier
Firefox milestones below predate containment. The October 9 contained llvmpipe full gate
passes at `075434.226844Z-firefox-full`, including short-click regression checks; see
`browser-input-ownership.md` for the pre-fix failures, application correction, evidence and
remaining scope. The Linux headless Firefox backend refused WebGL2 without a display, so
the tool uses its own Xvfb instead; it never attaches to the operator's desktop. PID and
device containment, software-renderer proof and kernel monitoring are unchanged.

The gate requires the production page to create WebGL 2, emit the current renderer/viewport,
first-draw, clearing-material, ocean-material, and visible-scene diagnostics, and render a 1280 by
720 frame. A trusted click must acquire pointer lock and initialize the complete 44.1 kHz audio
resource path. The test then requires one released Shift press to select persistent 8.0 m/s running,
forward motion to play a real distance-scheduled footstep, a second Shift press to restore 4.3 m/s
walking, and Escape to release pointer lock. A 1000 by 600 resize must retain an undistorted 16:9
canvas; after Firefox's security cooldown, a delayed trusted click must reacquire pointer lock and
a second Escape must release it again. The gate installs a document-visibility/animation probe,
activates a real second tab, requires the game tab to report `hidden`, reactivates it, requires
`visible` plus resumed animation frames, and proves resumed game input with another complete pair
of persistent Shift edges. It then loads the test-only fixed jungle and water cameras,
requires nonzero near/far populations across all four jungle layers plus an active water-view/
ripple/splash state. It also loads the fixed shipwreck, crab, butterfly, and turtle cameras and
requires their exact live population/draw diagnostics. The tool writes eight PNGs plus a schema-1 JSON
report under the ignored build directory and removes its profile and processes even on failure.

The pre-incident Firefox ESR 140.15 run reported `WebGL 2.0` on a `Radeon R9 200 Series, or similar`
backend and completed the first rendered frame in 289--401 ms total across fifteen clean tool runs.
All four original-resolution frames were inspected; the input frame visibly retained the world
and the warm-gold `RUN MODE  8.0 M/S` notice, while the jungle and water frames match the Chromium
composition and expose their expected live state. These one-run startup values are not a network
cold-load or frame-time benchmark. The gate does not qualify the remaining Chrome scene matrix,
simultaneous mouse look, clipboard behaviour, fullscreen, extended tab suspension/audio focus, or
audible sound quality.
