# Ecosystem integration stage — user priorities, 2026-10-12

This is a new requested execution order, not a claim that the supplied project
inventory has been independently verified. Recheck actual source and runtime
before implementing each item. Preserve the roughly 500m island, current system
boundaries and peaceful walking/swimming character; improve the connections among
existing features, not a broad redesign.

## Ordered work ledger

1. [~] **A small real stream.** Inspect `southValley` / `westValley` and use a
   suitable existing drainage corridor: spring → narrow upper stream → wider lower
   reach → protected western bay. Locally lower terrain by tens of centimetres to
   about a metre where appropriate, with a narrow static water mesh and slow flow.
   Check continuous downhill drainage, banks, mouth and traversal; no hydrology engine.
   Survey/layout foundation: west corridor99stations/100.559m, no sampled uphill
   step, candidate incision<=0.4m and sea-level-connected outlet. The live shared
   route now curves by -2.16..+1.45m, spans101.879m, carves a matching local terrain
   patch and carries the rendered water to the bay; source/outlet remain fixed.
   CPU geometry/water/player survey and actual llvmpipe image pass. Flow-motion,
   richer bank relief and portable/performance review remain. See
   [stream-course.md](stream-course.md).
2. [~] **Shared environmental moisture and drainage.** Extend
   `IslandEnvironmentField` with `moisture` and `drainage`, and evaluate
   `saltExposure`. Derive deterministic values from terrain, valleys, stream and
   shoreline. Feed existing `JungleLayout`, `GrassLayout`, `GroundCoverField` and
   `NaturalPropLayout`; keep one coherent source rather than separate biome hacks.
   First live pass now provides normalized moisture/drainage/salt exposure and
   feeds terrain turf, grass, palms, jungle layers and fallen-log probability.
   Broader wildlife/prop consumers and portable qualification remain.
3. [~] **Small wet stream-side microhabitat.** A roughly 10–25m band around the
   lower reach: darker soil, more ferns/moss/broadleaf plants/fallen branches, less
   grass. Reuse current assets with appropriate distribution/material variation.
   Verify a visibly distinct transition from dry beach, not an island-wide swamp.
   First pass adds damp earth/moss/stone, suppresses channel grass and clears
   trunks/dense plants from flowing water. A second pass adds 39 irregularly
   clustered marked fern/broadleaf plants plus three supported darker-bark fallen
   logs; actual llvmpipe review rejected and replaced an initially regular paired
   layout. Further bank shape/material tuning and portable/motion review remain.
4. [~] **Localized mangroves.** About 15–30 low-crowned, multi-stem trees with
   recognizable stilt roots in one small brackish mouth/protected-bay habitat.
   Prefer a variant of existing jungle/broadleaf geometry and rendering; do not
   spread them around the whole coast or create a separate engine by default.
   The first native pass now places exactly 15 environment-field-selected trees
   along the final stream reach. Canopy variant 8 adds a low broad crown, multiple
   stems, curved stilt roots and lighter grey bark in both LODs. Collision covers
   the visible roots and tests reject palm overlap. The first real capture caught
   an inverted procedural support frame that caused the operator's startup failure;
   it is fixed and `20261012T095538.769586Z-mangroves` passes and was inspected.
   Portable, performance and moving-view review remain; see [mangroves.md](mangroves.md).
5. [~] **Spatial reef habitat.** Extend natural props with cheap branching, rounded
   and plate coral variants plus underwater rocks. A bounded few hundred instances
   in appropriate shallow reef zones, with culling/LOD and actual underwater review.
   Connect existing fish schools and turtles to reef/seagrass habitat. The native
   first pass now has 314 seabed-fitted non-solid instances across four near/far LOD
   forms in the existing natural-prop renderer. Shared reef/seagrass suitability
   anchors eight fish schools and five turtle home ranges, including long-route
   regression coverage. Two early captures were visually rejected and iterated;
   inspected `20261012T101426.680059Z-reef-habitat` submits 51 habitat instances,
   three fish and one turtle. A later macro-material pass adds an explicitly pale
   inner lagoon, habitat-field-aligned reef patches, a broken 62–86m crest and a
   restrained deep-shelf drop-off without new geometry or draws. The elevated reef
   camera plus inspected native and Chrome/SwiftShader coast evidence now establish
   distant readability. Android, measured performance and crest-foam coupling remain.
6. [~] **Ecological placement of existing wildlife first.** Crabs on sand/wrack;
   butterflies in sunny clearings; snakes on dense-understory margins; spiders in
   forest/shrubs; boars in wetter interior; goats on dry open slopes; monkeys under
   more closed canopy; fish around reef/seagrass; turtles in calmer bay water.
   Use the shared environment field and preserve bounded valid routes/populations.
   The first audited pass confirms the existing sand-constrained crab colonies,
   clearing butterflies and canopy-adjacent spiders; fish already use the shared
   reef/seagrass field. Boars now require moist, low-salt ground outside the live
   drainage channel, goats require dry low-salt open slopes, eight snake homes are
   deterministically selected on separated dense-understory margins, and three of
   five turtle ranges are based in the sheltered western bay. Long deterministic
   habitat tests pass. Inspected llvmpipe `20261012T102822.874894Z-boars` and
   `20261012T102941.088316Z-turtles` pass validation. The inspected snake image is
   useful but rejected by the generic scene gate for low luminance variation, so it
   is not accepted evidence. Monkey tree selection now also rejects mangroves and
   isolated trees, requiring at least two neighbouring natural canopy trees within
   20m. Seven scale-specific homes selected by exhaustive attachment and full-route
   clearance checks are at least 90m inland beside qualified trees and at least 7m
   apart. The deterministic long session retains over 900m of ground travel plus
   natural climb, summit and interruption coverage. Inspected llvmpipe
   `20261012T111900.787421Z-monkeys` passes and proves dense interior habitat, but its
   long-tailed silhouette still looks rat-like and is explicitly rejected as visual
   monkey acceptance. All six crab colonies are now
   transactionally anchored beside actual non-solid driftwood or dry-wrack sites;
   all 30 homes remain within 3.25m of a debris cue and inside valid sand. Inspected
   llvmpipe `20261012T110201.712430Z-crabs` passes and visibly shows the colony around
   beach debris. Monkey visual redesign, portable review and a stronger snake habitat
   camera remain.
7. [~] **Four distinct coastal experiences.** Strengthen the existing broad beach,
   sheltered lagoon/bay, exposed rocky windward shore and mangrove stream mouth.
   Keep island outline/scale; make a coastal walk visibly vary in materials,
   vegetation, props, water and sound instead of increasing map size. A first shared
   coastal-identity field now distinguishes all four regions. Existing props consume
   it without a new render path: exposed sectors receive denser rock placement, the
   main beach selects only shell/pebble detail, and sheltered bay/mouth sites favor
   existing dry wrack. With the tide-pool accents, Core measures 36/41/0/3/2 rocky rocks, main-beach shells,
   main-beach wrack, bay wrack and mouth wrack. Inspected llvmpipe bay/headland/
   bay-ground captures `113137.288194Z`, `113202.517664Z` and `113228.784262Z` pass.
   The `113111.792460Z-beach` image is rejected because its inland-facing camera does
   not show the beach. Close detail, water/sound differentiation and portable review remain.
8. [~] **Rocky-shore tide pools.** A few selected depressions with calm static water,
   rocks/shells and optionally small crabs. Fit water to closed basins and terrain;
   no physical tidal simulation or disconnected water decals. Five separated basins
   now deform the real terrain on the exposed rock bench; calm water is below each
   original rim and shares the shoreline draw. Ten non-solid stones and five shell/
   pebble accents reuse existing prop batches. Long Core tests pass. Inspected llvmpipe
   `20261012T115636.611276Z-tide-pools` passes and visibly separates the foreground
   pool from the sea. Three earlier camera/location attempts were rejected. Further
   rim refinement, optional crab use, Web/Android and performance review remain.
9. [~] **Wind/wave-organized wrack.** Use `PrevailingWind` and shelter to accumulate
   existing driftwood, fallen fronds, shells and wrack in the leeward bay, with much
   less on exposed headlands. Reuse models; verify deterministic placement/clearance.
   A shared wind/shelter field now drives all four existing prop groups. A bounded
   western-bay deposition belt adds ten driftwood and 24 shell/wrack placements after
   support, route, tree and spacing checks. Core measures sheltered/exposed counts of
   11/4 driftwood, 47/26 beach details and 2/0 coastal fronds; all crab debris habitats
   remain valid. Inspected llvmpipe `140703.574060Z-bay-ground` visibly shows the
   accumulation while `140806.665876Z-headland` remains sparse and rocky. Deterministic
   native evidence passes; Web/Android, interactive walking and performance remain.
10. [~] **Very slow fair-weather cloud drift.** Move existing cumulus and wisps
    gently along prevailing wind, retaining a permanent sunny day. Avoid conspicuous
    wrapping/popping on long walks; no new weather or volumetric cloud system.
    The portable sky shader now advects both layers west at 0.00035 rad/s using a
    complete-turn periodic phase. CPU tests pin ten-minute speed, wrap and invalid-clock
    handling; inspected native llvmpipe overview `20261012T144704.173888Z-overview`
    passes and retains the sunny composition. A separated-time motion comparison and
    actual Web/Android shader runs remain before completion.
11. [~] **Wind-aware ocean and ambient wind.** Somewhat greater waves/foam/breeze
    sound on the exposed east side and calmer western bay; roughly ±20–30% modulation
    of existing systems is a starting point, to be tuned from actual observation.
    One shared bounded response now drives ocean displacement/slopes, ocean and shoreline
    foam, surf gain and breeze gain. East reaches 1.22/1.25/1.18 wave/foam/ambient scale;
    the western bay reaches roughly 0.89/0.88/0.91, returning to neutral away from shore.
    CPU tests and three inspected native llvmpipe views pass. Controlled paired motion,
    audible review and actual Web/Android runtime remain.
12. [~] **Stream audio and local sound biomes.** One distance-attenuated procedural
    stream loop; stronger existing jungle ambience in wet interior, breeze/surf on
    headlands and less wind in sheltered bay. Preserve audio budgets/mute/lifecycle;
    no replacement audio engine. A deterministic thirteen-second stream loop now follows
    the nearest point on the shared meandering route, fades through a 4–34m field and
    modestly strengthens jungle texture while softening canopy breeze in that wet corridor.
    It dynamically shares the otherwise inaudible land-side player-water voice, retaining
    four active ambient sources plus at most two transients. Wind-aware headland/bay surf
    and breeze remain integrated through item 11. Core signal/field/mix tests and the
    independent 25-file WAV export pass; actual spatial listening, route walk, mute/
    swim switching and Web/Android lifecycle evidence remain.
13. [~] **Bird landing/perching.** Before adding more birds/species, implement
    bounded Fly → Approach → Perched → Depart behavior on real branches, rocks or
    the wreck's broken mast, with verified feet/perch contact and takeoff/landing.
    The first native pass derives five real branch, two rock and one wreck-beam anchor,
    limits landing to two birds, folds wings/adds near feet, and completes all four
    deterministic states. Core contact/motion tests and an inspected wreck-perch
    llvmpipe frame pass. A moving visual sequence, branch-wind following and actual
    Web/Android runtime remain; see [bird-perching.md](bird-perching.md).
14. [ ] **One turtle nesting beach.** Subtle procedural/static shore-to-nest tracks
    and one or two disturbed sand areas on a suitable beach, fitting terrain and
    existing dry-sand material. No breeding simulation required.
15. [ ] **Small walkable headland cave.** Retain the requested cave, approximately
    15–25m deep: a mesh shell, local terrain opening and collision proxies. Verify
    actual entry, interior roof/walls/floor and exit; no heightfield sealing the
    entrance, voxel/CSG replacement or black-decal substitute.
16. [ ] **Hut farther inland.** Preserve the user's visual reference requirement;
    relocate/rebuild along an inland jungle path near fresh water. Beach emphasis
    stays on wreck/driftwood and traces of habitation. Check access and placement.
17. [~] **Enterable helicopter last.** Keep the previously requested realistic,
    grounded helicopter near the island centre, with accessible pilot and rear
    spaces and licensed/original asset provenance, but defer until ecosystem work.
    No flight capability. A native reduced-detail procedural first pass now supplies
    four open doors, walkable cockpit/rear floor, terrain-fitted skids, original-asset
    provenance, CPU entry checks and inspected exterior/interior software captures.
    Material/geometry polish and portable qualification remain; see
    [helicopter.md](helicopter.md).

## Next implementation stage and constraints

Prioritize **stream + moisture/drainage + wet microhabitat + localized mangroves +
habitat-driven reef/fauna**. Steps can be incrementally integrated through existing
terrain/environment/layout/wildlife interfaces. Inspect the implementation rather
than assuming the user's reported counts or limitations are still current.

No dynamic weather/storms, day/night cycle, simulated tides, volumetric clouds,
voxel cave, replacement vegetation engine, physically simulated river or survival
systems. Do not expand the island simply to add space. Previously requested animal
species remain deferred behind improving existing fauna; do not add a new batch
of species as a substitute for ecosystem integration. Diving, campfire, visual
repairs, stability and platform qualification remain in the full plan.

Each item needs actual visual/audio/traversal evidence appropriate to its scope,
deterministic placement tests where relevant, and portable resource/performance
checks. Technical gates alone do not establish a convincing ecosystem.
