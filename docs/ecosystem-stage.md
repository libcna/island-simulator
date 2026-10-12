# Ecosystem integration stage — user priorities, 2026-10-12

This is a new requested execution order, not a claim that the supplied project
inventory has been independently verified. Recheck actual source and runtime
before implementing each item. Preserve the roughly 500m island, current system
boundaries and peaceful walking/swimming character; improve the connections among
existing features, not a broad redesign.

## Ordered work ledger

1. [ ] **A small real stream.** Inspect `southValley` / `westValley` and use a
   suitable existing drainage corridor: spring → narrow upper stream → wider lower
   reach → protected western bay. Locally lower terrain by tens of centimetres to
   about a metre where appropriate, with a narrow static water mesh and slow flow.
   Check continuous downhill drainage, banks, mouth and traversal; no hydrology engine.
2. [ ] **Shared environmental moisture and drainage.** Extend
   `IslandEnvironmentField` with `moisture` and `drainage`, and evaluate
   `saltExposure`. Derive deterministic values from terrain, valleys, stream and
   shoreline. Feed existing `JungleLayout`, `GrassLayout`, `GroundCoverField` and
   `NaturalPropLayout`; keep one coherent source rather than separate biome hacks.
3. [ ] **Small wet stream-side microhabitat.** A roughly 10–25m band around the
   lower reach: darker soil, more ferns/moss/broadleaf plants/fallen branches, less
   grass. Reuse current assets with appropriate distribution/material variation.
   Verify a visibly distinct transition from dry beach, not an island-wide swamp.
4. [ ] **Localized mangroves.** About 15–30 low-crowned, multi-stem trees with
   recognizable stilt roots in one small brackish mouth/protected-bay habitat.
   Prefer a variant of existing jungle/broadleaf geometry and rendering; do not
   spread them around the whole coast or create a separate engine by default.
5. [ ] **Spatial reef habitat.** Extend natural props with cheap branching, rounded
   and plate coral variants plus underwater rocks. A bounded few hundred instances
   in appropriate shallow reef zones, with culling/LOD and actual underwater review.
   Connect existing fish schools and turtles to reef/seagrass habitat.
6. [ ] **Ecological placement of existing wildlife first.** Crabs on sand/wrack;
   butterflies in sunny clearings; snakes on dense-understory margins; spiders in
   forest/shrubs; boars in wetter interior; goats on dry open slopes; monkeys under
   more closed canopy; fish around reef/seagrass; turtles in calmer bay water.
   Use the shared environment field and preserve bounded valid routes/populations.
7. [ ] **Four distinct coastal experiences.** Strengthen the existing broad beach,
   sheltered lagoon/bay, exposed rocky windward shore and mangrove stream mouth.
   Keep island outline/scale; make a coastal walk visibly vary in materials,
   vegetation, props, water and sound instead of increasing map size.
8. [ ] **Rocky-shore tide pools.** A few selected depressions with calm static water,
   rocks/shells and optionally small crabs. Fit water to closed basins and terrain;
   no physical tidal simulation or disconnected water decals.
9. [ ] **Wind/wave-organized wrack.** Use `PrevailingWind` and shelter to accumulate
   existing driftwood, fallen fronds, shells and wrack in the leeward bay, with much
   less on exposed headlands. Reuse models; verify deterministic placement/clearance.
10. [ ] **Very slow fair-weather cloud drift.** Move existing cumulus and wisps
    gently along prevailing wind, retaining a permanent sunny day. Avoid conspicuous
    wrapping/popping on long walks; no new weather or volumetric cloud system.
11. [ ] **Wind-aware ocean and ambient wind.** Somewhat greater waves/foam/breeze
    sound on the exposed east side and calmer western bay; roughly ±20–30% modulation
    of existing systems is a starting point, to be tuned from actual observation.
12. [ ] **Stream audio and local sound biomes.** One distance-attenuated procedural
    stream loop; stronger existing jungle ambience in wet interior, breeze/surf on
    headlands and less wind in sheltered bay. Preserve audio budgets/mute/lifecycle;
    no replacement audio engine.
13. [ ] **Bird landing/perching.** Before adding more birds/species, implement
    bounded Fly → Approach → Perched → Depart behavior on real branches, rocks or
    the wreck's broken mast, with verified feet/perch contact and takeoff/landing.
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
17. [ ] **Enterable helicopter last.** Keep the previously requested realistic,
    grounded helicopter near the island centre, with accessible pilot and rear
    spaces and licensed/original asset provenance, but defer until ecosystem work.
    No flight capability. This is a retained requirement, not cancellation.

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
