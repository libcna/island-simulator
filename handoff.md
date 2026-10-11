# island-simulator handoff

## Update — Web surface-frame materials complete; Android stderr mirror prepared

Browser `20261011T100522.314132Z-chrome-materials` passes all four views, 60 intervals
each, GL error zero. All four images inspected; final kernel 12 AppArmor audit + one
audit-suppression notice, no GPU fault, latch unchanged. Session `21871` FINISHED exit zero.
See `docs/terrain-surface-frame.md`. No full input/lifecycle/Firefox qualification implied.

Added consumer-only Android stderr mirror before game construction: complete lines/chunks,
2 KiB buffer, at most 128 chunks plus a limit notice, original stderr always preserved.
No dependency/rendering changes. Native build and 34/34 non-display tests pass (52.34 s),
including chunking, fragmented unitbuf lines, trailing text, stream restoration and log cap.
New test added to explicit debug/sanitizer preset targets. Android build/runtime verification
is next; capture the actual shader/link message for suspected `CNA-ISLAND-002`.
Android x86_64 assembly/lint passes. Monkey close-view run is ACTIVE in session `51286`;
poll the same handle. Launcher log: `build/diagnostic-mirror-android-monkeys.log`.
After inspection, preserve the diagnostic x86 APK separately and restore default ARM64
assembly/lint. The rolling tested APK still holds the earlier material-qualified build.

## Update — Android guest uniform diagnostics investigated read-only

See suspected `CNA-ISLAND-002` in `BUGS.md`: stock CompileAndLink logs failure but continues
uniform queries; the vertex-lit skinned program has 24+3 queries, matching the 27-message
burst. This is a candidate, NOT confirmed program attribution or a driver/uniform-limit defect.
Next targeted evidence: contained monkey close view and actual link diagnostic, after Web ends.
Browser materials session `21871` is still live; litter and grass screenshots are now present.
Litter image inspected and consistent with native. Do not restart or run concurrent graphics.

## Update — surface-frame Android visual checks; Web material scope prepared

Game revision `3642ac1` / capture harness `90fc6a4`: x86_64 assembly/lint and litter
`20261011T095903.886091Z-litter`, wet-sand `20261011T100107.688712Z-wet-sand` gates pass.
Both images inspected, first screenshot attempt; empty final kernel deltas. ARM64 build/lint
restored, tested x86 APK retained; exact hashes in `docs/terrain-surface-frame.md`.
CAVEAT: litter has 27 guest gfxstream uniform-query 0x502 diagnostics, also seen in the
older `20261011T092539.275219Z-interactive`; wet-sand has none. Root cause/ownership unknown;
passed image/startup gates do not mean zero GL errors. Android lifecycle remains open.
Added Chrome `materials` scope: litter, grass, bay-ground, wet-sand, 60 intervals each;
12 browser-launcher tests and Node syntax check pass. Web runtime qualification pending.
Web build/package complete: 10,347,083 raw / 3,528,841 gzip bytes. Browser materials run
is ACTIVE in session `21871`; poll the same handle, do not restart on observation timeout.
Launcher output: `build/surface-frame-web-materials.log`. Build session `8078` is complete.


## Update — explicit shaded-floor capture profile verified

The litter image discrepancy below is resolved by an explicit `shaded-detail` profile,
not a lighting change or global gate reduction. See `docs/terrain-surface-frame.md`.
Fresh native `20261011T095340.833878Z-litter` passes and was inspected; empty kernel delta,
latch unchanged. The old failed run remains failed and untouched. Six validator and twenty
Android CPU-only tests pass; native/Android launchers select the profile from camera arguments.
Full non-display CTest also passes 33/33 (49.94 s); no jobs remain running.
NEXT: Android/Web material qualification of game revision `3642ac1` remains pending.
No emulator/KVM configuration change, hardware rendering or push is authorized.

## Update — surface normal fix built; litter image gate remains open

`docs/terrain-surface-frame.md`: scale-first CPU/GLSL normal helper now drives sand,
soil clods and wet sheen; new `wet-sand` camera and mappings. Native build and 33/33
non-display tests pass (42.70 s), including 1e-30..1e30 scale/winding invariance.
Fingerprint still `5d0b62ad43a25895`, grass 17,225. Native wet-sand
`20261011T094128.539261Z-wet-sand`, grass `20261011T094154.489156Z-grass`, bay-ground
`20261011T094225.864744Z-bay-ground`, water `20261011T094616.083800Z-water` pass and
were inspected; all kernel deltas empty, latch unchanged. No jobs running.

NEXT: resolve retained litter capture `20261011T094405.569362Z-litter`, which FAILED
the generic image gate: luminance deviation 16.8656 < 18, mean 58.8703, 240 bins,
spatial contrast 37.2447; other gates pass. Inspected image is complete textured shaded
floor, not blank. Do not call it passed. Validator unchanged. Evaluate appropriate
image criteria/reference evidence without adjusting lighting just to satisfy a threshold.
Then Android/Web material qualification for this fix; prior `9f1ee59` results do not
qualify it. Android lifecycle remains open and hardware latch must remain set.

## Update — sand/turf Android and Web qualification complete

Current game revision `9f1ee59`. x86_64 assembly/lint passes. Bay-ground
`20261011T092336.875730Z-bay-ground` passes the image gate and matches native visually.
Interactive `20261011T092539.275219Z-interactive` passes sensitivity/footsteps/swimming
and transition audio after one gesture; image inspected. Both kernel deltas empty,
latch unchanged. Lifecycle deliberately excluded, still degraded. Tested x86 APK
preserved, default ARM64 build/lint restored. Exact sizes/hashes in `docs/dry-sand-relief.md`.

Web build/package: 10,346,158 raw / 3,528,706 gzip bytes. Contained Chrome terrain
`20261011T092826.584773Z-chrome-terrain` passes seven views, 60 intervals each, GL error zero.
Bay-ground before/after and jungle images inspected. No GPU faults; final kernel delta
contains AppArmor audit and audit-suppression notices. Full input/lifecycle/Firefox not rerun.
Browser session `28597` and build `34267` completed; do not poll/restart them.
This qualifies game revision `9f1ee59`, not newer uncommitted normal/camera changes above.
These are software checks, not host AMD or physical-device acceptance.

## Update — dry sand relief native stage complete

`docs/dry-sand-relief.md`: wind-aligned warped ripples plus footprint-filtered grain
shading. Native build succeeds, non-display 33/33 pass (50.10 s); final shader refinements
pass material/CLI/browser-preflight checks. Initial bright foreground rejected and fixed
by correcting the sand normal-length floor. Final bay-ground
`20261011T092129.366965Z-bay-ground` and bay `20261011T092157.241700Z-bay` pass and
were inspected; empty kernel deltas, latch unchanged. No geometry/population changes.
Web terrain suite now has seven views, including `bay-ground`. No jobs running.
NEXT: qualify BOTH this shader and preceding coastal turf on Android (bay-ground and
focused touch/traversal, restore ARM64) and Web (build/package, seven-view terrain matrix).
Earlier beach qualification does not qualify these newer stages. Static native images
do not prove temporal stability. Soil/wet-normal length floors deserve separate review;
they were not changed here. Full environment/geology and Android lifecycle remain open.

## Update — coastal turf native stage complete

`docs/coastal-turf.md`: shared CPU/generated-GLSL sand/rock suppression for grass and
turf, plus hard sandy-apron exclusion before the density floor/route boost (parents and
companions). New `bay-ground` eye-height camera and platform mappings. Native build and
final 33/33 non-display groups pass (52.01 s); CLI help test updated for the new camera.
Captures bay-ground `20261011T091417.768738Z-bay-ground`, grass
`20261011T091436.406649Z-grass`, bay `20261011T091521.122820Z-bay` pass and were
inspected; all kernel deltas empty, latch unchanged. Fingerprint `5d0b62ad43a25895`,
17,225 grass patches (two fewer), other populations unchanged, rendered root gaps zero.
Browser grass count pin updated. No jobs remain running.

NEXT: Android/Web qualification of the new turf/camera revision. The previous beach
results below do not qualify it. Ground-level bay review also identified overly smooth
dry sand as the next material target (filtered wind ripples/grain); that is not implemented.
Full environment migration/geology still open. Android lifecycle remains degraded.

## Update — Web wider-beach validation complete

Web build and package for committed `53a2442` complete: 10,343,092 raw / 3,528,055
gzip bytes. Contained Chrome terrain suite `20261011T085703.088366Z-chrome-terrain`
finished with all six views passing, 60 intervals each. Overview/bay/headland/east/jungle
images inspected; no new GPU faults, Wi-Fi bandwidth/EXT4 unmount/USB disconnect notices.
Full Web input/lifecycle
and Firefox not rerun. No browser job remains running; session `38693` completed.

Do not attribute this completed Web test to the newer coastal-turf revision above.

## Update — dominant bay beach native stage complete

`docs/dominant-bay-beach.md`: shared approximately 36 m sheltered bay / 6.5 m eastern
headland width now controls physical profile and sand material. Final 33/33 non-display
tests pass (53.06 s). Native bay `20261011T084410.221147Z-bay`, headland
`20261011T084654.624120Z-headland`, beach `20261011T084740.315381Z-beach` pass and
were inspected, all empty kernel deltas. Existing beach camera looks inland through
vegetation, so it is not a close sand review. Fingerprint `08ced4994845ae68`; measured
inventory pins updated in core and browser tests. Hardware latch unchanged.
Android x86_64 build/lint and interactive `20261011T085027.124944Z-interactive` pass
touch/footsteps/swimming after two gestures, image inspected. Initial bay run had a black
PNG despite old log-gate success. Added `tools/android_capture.py`: bounded 30-second
image-content validation for initial/hot/cold screenshots; 19 CPU-only Android tests pass.
Repeat `20261011T085244.623709Z-bay` passes strengthened gate and visually matches native.
No new GPU faults (Wi-Fi-only kernel deltas). Tested x86 APK preserved; ARM64 build/lint
restored. See `docs/dominant-bay-beach.md` for hashes. Web work is tracked above.
Earlier rocky-coast runs do not
qualify this newer code. A dedicated ground-level bay view is useful follow-up work.
Android lifecycle remains degraded; do not close the known issue from focused visual runs.
No Android/KVM system configuration changes are needed or requested. No push.

## Update — rocky coast Web qualification complete

Web build/package refreshed: 10,343,035 raw / 3,528,104 gzip bytes. Contained Chrome
terrain suite `20261011T083120.118782Z-chrome-terrain` passes all six views: overview,
bay, headland, beach, east, jungle, 60 intervals each and GL error zero. Bay/headland/east/
jungle images inspected. No new GPU faults; kernel delta contains USB/storage, PCIe and
perf notices. Full Web input/lifecycle and Firefox not rerun. This qualifies `2a660b7`.

Historical source preparation (now completed in the native stage above): changes in
`IslandEnvironmentField.hpp`, `IslandTerrain.cpp`, `TerrainRenderer.cpp` and `CoreTests.cpp`
add variable beach width, approximately 36 m in the sheltered bay and 6.5 m at the eastern
headland, shared by geometry and sand material. The completed browser suite used the immutable
rocky-coast build from commit `2a660b7`; it does NOT qualify this newer source preparation.
Native build/tests/review are now complete; affected Android/Web platforms still need requalification.

## Update 2026-10-11 — integrated rocky coast (native and focused Android qualified)

See `docs/coastal-environment.md`. The first `IslandEnvironmentField` coastal subset drives
actual up-to-7.5 m headland relief, RockyGround classification, palm/canopy/understory/grass
exclusions and shader rock/sand controls. Reuses canopy texture blue/alpha; red/green and
texture/mesh/draw budgets unchanged. This is not the full environment model.
Crab starts now find nearby valid sandy habitat; tests preserve their population/determinism.
Rocky faces intentionally block climbing, with separately tested adjacent beach access.
Final 33/33 non-display groups pass (55.12 s). Bay `20261011T082154.597630Z-bay` and final
headland `20261011T082316.930694Z-headland` pass/inspected, empty kernel deltas.
Fingerprint `b0f79ae14adeb8cd`; inventory pins updated from report. `headland` camera is
available in native/Android launchers and Web template; terrain suite now has six views.
Android x86_64 build/lint and headland `20261011T082539.891561Z-headland` pass, image
inspected. Interactive `20261011T082658.311623Z-interactive` passes touch/footsteps/swimming
after two gestures; lifecycle not rerun and still degraded. Empty kernel deltas. ARM64
assembly/lint restored; physical-device runtime untested. NEXT: finish the running Web
visual/runtime qualification for this new terrain/shader, then
extend environmental controls/dominant beach. Prior coast-shape qualification below is not
current evidence. Android lifecycle remains degraded. No push or hardware rendering authorized.

## Update 2026-10-11 — Web coast qualification complete

Coast Web build and release package refreshed (10,340,739 raw / 3,525,638 gzip bytes).
Contained Chrome terrain suite `20261011T080318.835613Z-chrome-terrain` passes all five
views: overview, bay, beach, east, jungle (60 frame intervals each). Overview/bay/east/jungle
screenshots inspected. No new GPU faults; kernel delta contains perf and Bluetooth/USB
messages. No game changes since coast commit `623c314`. Web input/lifecycle and Firefox
were not rerun. Android lifecycle remains degraded, separately from the passed visual checks.
Read-only SDL inspection found explicit pending pause/resume coalescing, which can explain
missing AAudio calls during rapid Home/resume; see `BUGS.md`, not a confirmed root cause.
Next implementation: `docs/environment-field-design.md` (design only), integrating shared
coastal rockiness/beach suitability through relief, vegetation placement and terrain materials.

## Update 2026-10-11 — bay/headland silhouette (native qualified)

See `docs/coastal-silhouette.md`. Western bay between two headlands plus eastern promontory;
shore radii 158.994–258.680 m. New `bay` QA camera. The first shallow design was rejected
visually; final geometry has world-space recession checks. New coast exposed turtle bank
steering at the home centre, now fixed without weakening its movement regression.
Final 33/33 non-display groups pass (37.13 s). Native final bay
`20261011T074941.874456Z-bay` and turtles `20261011T075118.375540Z-turtles` pass/inspected.
Fingerprint `9dc988ef0ffd9f67`; population and shadow-count pins updated from the report.
Android x86_64 build/lint and bay `20261011T075311.442314Z-bay` pass, image inspected.
Interactive first fails a too-short fixed swimming gesture; bounded repeated-touch gate
added with a device-free shell regression. Repeat `20261011T075739.370693Z-interactive`
passes touch/footsteps/swimming after two gestures but lacks AAudio pause/start evidence
on Home/resume, despite restored surface/focus/same PID. Lifecycle remains degraded;
see the distinct observation under `CNA-ISLAND-001`. ARM64 assembly/lint restored;
physical-device runtime untested. Relevant harness regressions and syntax checks pass.
Web qualification is recorded above. Next is shared coastal
environment/material placement to replace the continuous sand band with rocky headlands
and a dominant beach. Macro-geography items 1–3 remain incomplete. Android lifecycle issue
`CNA-ISLAND-001` remains open. No hardware rendering or push is authorized.

## Update 2026-10-11 — ridge/valley interior

See `docs/ridge-and-valleys.md`. Two offset ridges now reach 40.0192 / 29.0938 m, with a
13.7758 m saddle station and southern/western drainage. Steep faces reveal filtered rock
grain/fractures. Bird clearance now anticipates terrain and clamps the complete altitude.
Final 33/33 non-display tests pass (49.78 s), including summit/valley, trail slope, roots,
wildlife contact and bird clearance. Native final overview `20261011T072739.811196Z-overview`
and east `20261011T072605.084747Z-east` pass and were inspected.
Android x86_64 build/lint and east `20261011T072932.613664Z-east` pass, image inspected.
Interactive `20261011T073115.062445Z-interactive` fails Home/resume with `EGL_BAD_SURFACE`;
identical-APK repeat `20261011T073355.598497Z-interactive` passes. Suspected lifecycle race
is open as `CNA-ISLAND-001` in `BUGS.md`; do not call Android lifecycle reliably qualified.
ARM64 assembly/lint restored. Web build/package refreshed; Chrome terrain suite
`20261011T073720.044111Z-chrome-terrain` passes all four views (60 frame intervals each).
Overview/east/jungle screenshots inspected; no new kernel GPU faults.
The world fingerprint and new inventory pins are in the ridge document.
Next is the shoreline/headland/bay stage, followed by the shared environment field. The full
macro-geography rewrite remains open. No hardware rendering or push is authorized.

## Update 2026-10-11 — narrow footpath

See `docs/natural-footpath.md`: vegetation corridor 12.4 m -> 2.8 m, narrower dirt shading,
full trunk/rock footprint rejection and three-lane collision regression. Native overview/jungle
inspected; all non-display test groups pass (core rerun after the rock fix).
Android x86_64 touch/traversal/lifecycle run `20261011T071027.731091Z-interactive` passes;
ARM64 build/lint restored. Web build and package refreshed. First grass-motion run failed a
15 s wall wait at 5.99/6 m; bounded game-sample wait added. Retry
`20261011T071524.062886Z-chrome-grass-motion` passes seven stages, 93 distinct frames,
GL error zero and no new GPU faults. Full Web lifecycle was not rerun.
New inventory and world fingerprint are in the footpath document; browser pins updated.
The next substantial realism target is the asymmetric ridge/valley terrain
and coast from macro-geography items 1–3. Local path forks/detours remain open too.

## Update 2026-10-11 — reef/seabed material pass

Continued the suggested items 5/16 in `TerrainRenderer.cpp`: depth-limited olive seagrass,
mottled ochre reef patches, pale radial sand channels and a broken outer material crest.
See `docs/reef-seabed.md` for the design, limitations, captures and package identities.
This is a material-only first pass; geometric reef relief and the larger terrain rewrite
remain open. No dependency pins or population/mesh counts changed.

Linux build and 33/33 non-display tests pass. Native before/after overview and Android
SwiftShader reef/overview captures pass and were visually inspected. ARM64 assembly/lint
restored; physical runtime untested. Chrome/SwiftShader terrain suite
`20261011T065909.248161Z-chrome-terrain` passes all four views and the web release is refreshed.
Nothing from this
session has been pushed. The AMD hardware stop latch remains unchanged.

The reef pass is complete as a first material milestone. The trail step selected after it is
recorded above.

The following is the historical 2026-10-10 handoff; use the update above and `PLAN.md` for
the current state.

Written 2026-10-10 (last update at the end of that day's session) for the next agent
(expected: Codex Astra High) starting on a fresh context. The working tree is clean and
everything below is committed and pushed to `origin/develop`. Everything you need to continue is here or linked; `PLAN.md` is the detailed
ledger (newest sections at the top, under "2026-10-10"). All repository text is English.

## 1. Read first: hardware safety boundary (unchanged, still in force)

**Do not run this game on the host AMD GPU. Do not clear the hardware stop latch.**
The 2026-10-08 AMDGPU GPUVM faults caused a MODE2 reset, VRAM loss and a GNOME logout; the
root cause is not identified or fixed. Clean software-renderer runs are not evidence that the
hardware workload is safe.

- Latch: `build/gpu-incident-evidence/QUALIFICATION_STOP.json`, SHA-256
  `c0bd170489321c8d497eae0f1211e1553e30aef9ee02ac01774269300e6ee6f1` (ignored local state; a
  fresh clone lacking it is not hardware authorization).
- Only run the game through the contained software launchers listed in section 5. Never run
  unrestricted display CTest, never replay GL traces on AMD, never launch the binary directly.
- Incident documentation: `docs/incidents/2026-10-08-amdgpu-gpuvm-reset.md`,
  `capture-evidence-inventory.md`, `mesa-recorder-readiness.md`, `process-containment.md`,
  `maintainer-sharing-summary.md` and `upstream-triage-draft.md` (drafts only; nothing has
  been submitted upstream and pushing this repository is not permission to do so).
- Stop on any new GPU fault, including unrelated host activity. Do not restart a running
  tool job just because an output poll timed out.

## 2. Operator direction (2026-10-10)

1. **The island must look noticeably closer to reality**, for existing and all future
   content. Stylized or crude assets are defects. The operator named the sea turtle as
   "really terrible"; a first pass is done, more is expected (see section 4).
2. Product boundaries are unchanged: peaceful first-person exploration of a ~500 m tropical
   island, fixed sunny daytime, no combat, quests, survival, crafting, inventory, weather or
   day/night. Deterministic simulation. Portable GLSL 330 / ES 300. Measured budgets.
3. Backlogs the operator asked for are in `PLAN.md` (section "2026-10-10 — operator
   direction"): a 16-species nonhostile wildlife list (snakes, spiders, parrots, pigeons,
   crocodiles, frogs, bats, mice, rats, deer, goats, ants, flies, stingrays, dolphins,
   octopuses), a decorative island hut, and a 20-item macro-geography realism list from the
   operator-supplied analysis (Czech original kept outside the repository at
   `../island_improvements.md`). **Monkeys and wild boars stay on the island** by operator
   decision.
4. Order: visual realism of existing content before new species; a new species is not done
   until silhouette, material, motion and contact look believable in close software captures.

## 3. What was done on 2026-10-10 (all committed on `develop`)

- Dependency pins advanced twice to the clean shared checkouts (`dependencies.lock`): Sharp
  Runtime `next` c20373ba, EasyGL f4d6afaf, MetaGL dd3b3005, CNA 97783d1e (unchanged). The
  strict check in `cmake/VerifyDependencies.cmake` refuses to configure when a shared
  checkout moves; **other agents commit to those checkouts during the day**, so if a build
  refuses, review the delta, update the lock deliberately and requalify. Never switch or
  edit the shared worktrees.
- `docs/ground-cover-and-trunks.md`: `GroundCoverField` + `GroundCover::Density` shared by
  grass placement and the terrain turf undercoat (patch field in the canopy-light texture's
  green channel), 20,523 grass clusters; grey-brown canopy trunks with root flare, rounded
  buttresses, moss/lichen, contact occlusion and canopy-shaded wood; slender flared palm
  stems with smooth normals.
- `docs/sea-turtle-form.md`: domed scute-aligned carapace, plastron, beaked head, bending
  paddle strokes (`SeaTurtleProfile.hpp`, tested); still blocky head and tonal-only seams.
- `docs/sky-atmosphere.md`: shared-haze horizon, warm sun glow, eleven fixed distant cumulus
  masses; `environment::FogColor*` constants back the C++ fog sites (GLSL literals are still
  hand-kept at `vec3(0.53, 0.78, 0.88)`).
- `docs/forest-stands.md`: closed canopy stands with a sparse scatter and 22 emergents
  (277 trees). Counts that changed downstream: 579 bushes (14 clearing), 3,116 ground plants
  (64 clearing, 2,225 island-wide), 199 colonies, 159 land rocks, 12 logs.
- Evening, after the operator chose to continue the backlog: sea turtle second pass (ten-segment
  four-ring head, geometric scute seams, wet sheen; `docs/sea-turtle-form.md`) and the first
  prevailing-wind pass (`docs/prevailing-wind.md`: shared east trade wind, coastal windward
  exposure, palms lean up to ten degrees; ocean/foam/crowns/wisps/audio still to do) and
  depth on the horizon (`docs/distant-islets.md`: nearer wooded islet, sandbar, hazed far
  silhouettes), and coconut palm groves with young palms (`docs/palm-groves.md`, 370 palms).
- Harness fixes (`docs/browser-input-ownership.md`): GLSL ES reserves `patch` (a CPU test now
  scans generated GLSL for ES-reserved identifiers); the Web harness holds keys until the game
  logs a new acknowledgement and waits for the release to be observed; the swim walk and
  resume waits are paced by game output because frames on SwiftShader exceed 1 s and
  `PlayerMotion` clamps each step to 0.1 s.

## 4. Open work, in the intended order

1. **Keep going on visual realism** per the `PLAN.md` 2026-10-10 backlog. Highest-impact open
   items: terrain macro-geography rewrite (item 1), varied coast types (2), environment
   field replacing shore-distance bands (3), prevailing wind (4), readable reef (5), natural
   footpath (6), palm distribution (9), directional shadows (11), coastal micro-topography
   (15), underwater habitat (16), small fauna (17), geology (19), wet biome (20).
   Partially done: wind (4), stands (7), emergents (8), palm groves (9), trunks (10), sky (12),
   atmosphere (13); done: islets (14).
2. Sea turtle third pass: hexagonal scute layout, surface reflections, underwater tint.
   Suggested next realism step (designed, not started): readable reef/seabed (items 5/16) in the
   terrain fragment shader only. Measured profile: the bed is 0.7 m deep 20 m offshore, 3 m at
   70 m, 5 m at 110 m, 16 m at 250 m; water alpha is `0.42/m` extinction, so seabed patterns
   read to about 3 m depth. Plan: seagrass meadows (0.5-2.5 m depth, dark olive, broad/medium
   noise), patch reefs 28-78 m offshore (ochre-brown mottled blobs) cut by pale radial sand
   channels (noise along arc length), and a broken reef crest band 70-84 m offshore. Keep it
   straight-line GLSL (see the sky lesson below) and verify on Android.
3. Then species from the wildlife backlog and the island hut, each with deterministic
   simulation, budget, capture viewpoint, regression tests and close-capture review.
4. Requalify Web input/lifecycle and Android touch/lifecycle after relevant changes. ARM64
   physical-device runtime and Linux AMD hardware acceptance remain open. Never promote
   pre-incident results to current qualification.
5. Incident work is a separate approval-gated track (section 1).

**Resolved 2026-10-10 evening:** the Android emulator crash (qemu host RenderThread SIGSEGV
in `jungle`/`east`/`turtles`) was caused by the sky's cumulus loop over a constant array, not
by the forest stands. Bisected file by file and fixed by unrolling (`docs/sky-atmosphere.md`).
Lesson for future shaders: avoid constant arrays indexed by loop counters (and `continue`)
in fragment shaders; the Android emulator's gfxstream/SwiftShader host can crash on them even
when Mesa and ANGLE accept them. Many earlier "colony" experiments in `PLAN.md` are void.

Known soft spots to watch: distant turf undercoat reads as flat olive in open clearings;
cumulus masses are smooth blobs; `lifecycleResumed` in the full Web suite is false (reported
only, not a gate); `IslandGpuQualificationSupervisorTests` has a 4 s fixture timeout and can
fail under `ctest -j4` load while passing at `-j2` or alone.

## 5. Build, test and capture commands (run from the repository root)

```sh
export CCACHE_DIR=/rv/cnaccache CCACHE_BASEDIR=/rv     # mandatory, see ../AGENTS.md
cmake --build --preset linux-debug -j2                 # existing build/ tree, do not reconfigure
ctest --test-dir build -LE display --output-on-failure -j2

# Contained native llvmpipe captures (viewpoints: overview beach jungle east clearing-edge
# west grass ground ground-broadleaf bush litter contact movement palms water current distant
# props driftwood reef fronds birds fish shark monkeys shipwreck crabs butterflies turtles)
sg systemd-journal -c 'python3 tools/run_software_capture.py build grass'

# Web (existing build-probe/ tree)
source /home/robertvokac/emsdk/emsdk_env.sh
export EMCC_CORES=2 BINARYEN_CORES=2
cmake --build --preset web-webgl2 -j2
sg systemd-journal -c 'python3 tools/run_browser_software.py build-probe --node /home/robertvokac/emsdk/node/24.19.0_64bit/bin/node --browser chrome --scope litter --timeout 300'
#   scopes: full terrain wildlife-foliage coast grounding vegetation grass-motion litter
#   (the turtle view exists only in --scope full --timeout 1700, about 25 minutes)
tools/package_web_release.sh build-probe

# Android (x86_64 emulator run, then restore the default ARM64 package)
export ANDROID_SDK_ROOT=/home/robertvokac/Android/Sdk
tools/run_android_gradle.sh :app:assembleDebug :app:lintDebug -PislandAbi=x86_64
sg systemd-journal -c 'python3 tools/run_android_software.py --sdk /home/robertvokac/Android/Sdk --boot --apk android/app/build/outputs/apk/debug/app-debug.apk --viewpoint grass'
cp android/app/build/outputs/apk/debug/app-debug.apk build/android-software/island-x86_64.apk
tools/run_android_gradle.sh :app:assembleDebug :app:lintDebug
```

Rules that bit this session:

- Every launcher writes `run.json` with `"result"`; a `tail` of its stdout can look like
  success while the gate failed. Always read `run.json` and keep failed-run directories.
- The owned emulator crashed once (exit -11) on 2026-10-10 and its stale AVD lock files made
  the next launch fail before boot; the harness now removes such locks when no emulator
  process exists. Browser scopes with eight views need `--timeout 1700` at current frame cost.
- Harnesses pin exact diagnostics and counts (`tools/run_software_capture.py`,
  `tools/run_web_smoke.mjs`, `tools/run_firefox_web_smoke.mjs`,
  `tools/run_android_runtime_smoke.sh`). When you change a mesh inventory, population count or
  a diagnostic string, update the pins in the same change, otherwise runs fail with
  "check for a stale executable" or silently on a regex.
- The full Web suite verifies 28 sections; if a run fails, test the harness' `includes`
  literals and regexes against the retained `island-browser-smoke.json` outputs to find the
  stale one quickly (see the PLAN entry for the 2026-10-10 turtle requalification).
- Do not overlap heavy jobs (Web build, browser suite, emulator, Gradle). Two build workers
  for Sharp-Runtime-governed work; `-j4` is fine for the island build itself.
- Never build in `/tmp`; reuse `build/`, `build-probe/`; keep `build-consumer/` and
  `build/gpu-incident-evidence/` untouched.

## 6. Evidence locations (ignored local data, not in Git)

- Native: `build/software-captures/<timestamp>-<viewpoint>/` (PPM, logs, `run.json`,
  kernel deltas). Latest (palm groves): `20261010T174808.840978Z-overview`,
  `174754.658042Z-palms`, `174801.794546Z-beach`; before: `20261010T172806.329415Z-overview`,
  `172812.566297Z-distant`, `172642.513830Z-palms`, `172648.540756Z-jungle`. Earlier:
  `20261010T150916.889020Z-jungle`, `150924.874906Z-overview`,
  `150932.482781Z-east` (forest stands); `145219Z-beach`, `145227Z-overview`,
  `145234Z-palms` (sky); `143756Z-turtles`; `130435Z-grass`, `131112Z-litter` (ground cover).
- Web: `build/browser-software/`. Full suite `20261010T180146.003280Z-chrome-full` (28/28,
  final tree with palm groves; earlier `172921.618192Z`, `142621.752717Z`);
  vegetation scope for the forest stands `152203.104225Z-chrome-vegetation`; litter scopes `145543Z`
  (sky) and `131626Z` (ground cover); grass-motion `132225Z`. Failed runs from the same day
  are retained on purpose (reserved word, key pulses, stale pins).
- Android: `build/android-software/`. Final tree (palm groves), all pass:
  `20261010T175715.080427Z-palms`, `175828.715762Z-jungle`, `175941.257408Z-east`; before the
  groves: `173721.178620Z-jungle`, `173833.723258Z-east`, `174128.122762Z-turtles`. Earlier:
  `145908Z-palms` (sky), `144750Z-turtles`, `133014Z-grass`; the many failed `jungle`/`east`
  runs of the evening are the sky-shader crash bisection. Rolling tested x86_64 APK:
  `build/android-software/island-x86_64.apk`.
- Package sizes and hashes are in the `docs/*.md` file of each pass. ARM64 runtime untested.
- Incident evidence: `build/gpu-incident-evidence/` and historical `build-consumer/`.
  Never clean these as disposable build output.

## 7. Repository and dependency boundaries

- CNA `next` export at `build/dependencies/cna-next` and Sharp Runtime `next`, EasyGL and
  MetaGL `develop` under `../` are strictly read-only. Record actual framework defects in
  `BUGS.md`; do not patch them.
- Original artwork lives in `assets/art-studies/` with provenance in `ASSET_LICENSES.md`
  and `assets/manifest.json`; prepared PNGs are generated at build time (Python 3.10+ and
  Pillow on the host only).
- Commit on `develop`; push only when the operator asks. The push on 2026-10-10 included all
  work up to the forest-stands pass.
