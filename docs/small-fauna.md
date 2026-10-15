# Small fauna

The first bounded stage of macro-ecosystem item 17 adds eight deterministic inland geckos. This is
not completion of the whole small-fauna list: skinks, hermit/coconut crabs, additional shorebirds,
fruit bats, dragonflies, cicadas and beetles remain candidates for later stages.

Each gecko owns a compact 5.2--6.1 m home range in one of the two clearings, near the seasonal
wetland, or on suitable inland ground. Placement rejects beach proximity, steep slopes and invalid
surface classes. The three-state simulation rests, briefly scurries away when the player comes
within 2.7 m, then disappears into cover before returning. It uses the shared wildlife activation
policy, including reduced and dormant update tiers.

The close model is deliberately small (roughly 37--47 cm including the tail) and has an eight-sided
lofted trunk and flattened head, tapered four-section tail, four articulated splayed legs, three
toes per foot, raised amber eyes with dark pupils, dorsal markings and four restrained habitat
palettes. All eight animals append to the existing butterfly dynamic vertex buffer and draw, so the
stage adds no renderer, GPU buffer, material switch or draw call. Hidden animals contribute no
vertices. The fixed `geckos` camera holds one animal visible for anatomy review; ordinary play and
the deterministic CPU tests retain the full rest/flee/hide cycle.

The world report fingerprints all initial states and home ranges, reports exactly eight geckos and
currently validates as `c4874b5d2f4ed411`.
Core tests cover deterministic construction and long simulation, inland grounding, home bounds,
finite invalid-input handling, all three behaviours, visibility fade, player-triggered flight and
complete activation-tier accounting.

Current evidence:

- native 1280x720 llvmpipe capture:
  `build-consumer/software-captures/20261015T013548.592879Z-geckos` (281 colour bins, 19.41
  luminance deviation and 73.04 spatial contrast; inspected at full resolution);
- Chrome/SwiftShader:
  `build/browser-software/20261015T013200.753923Z-chrome-geckos`, 60 finite positive intervals,
  GL0, live context, two distinct separated-time frames and no new kernel GPU fault;
- ARM64 assemble and lint pass without launching an emulator or touching KVM.
- the filtered non-display regression passes 60/60; its five display-producing tests remain
  intentionally disabled in this run.

The first native attempt exposed an endlessly reset flee timer and the test rejected it. Two later
browser attempts are retained as rejected evidence: the first lacked the second-frame harness path;
the second let the inspected animal complete its hide cycle before the slow software renderer took
the screenshot. Neither is accepted as visual proof.
