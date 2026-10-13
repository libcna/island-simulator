# island-simulator

`island-simulator` is a peaceful first-person tropical island walking and near-shore swimming
experience built in C++23 with CNA.

## Current GPU safety status

AMDGPU hardware qualification is suspended after a confirmed GPUVM fault and desktop-session
loss. The cause is still unresolved; do not treat successful software captures as a hardware fix.
See [the incident report](docs/incidents/2026-10-08-amdgpu-gpuvm-reset.md).
New wildlife and visual work is currently checked with CPU tests and forced llvmpipe/pixman.
After building, a repeatable software-only visual check is:

```sh
python3 tools/run_software_capture.py build jungle
```

This requires kernel-journal visibility. If group membership was granted without a new login,
use `sg systemd-journal -c 'python3 tools/run_software_capture.py build jungle'`.
The command preserves its image, logs and kernel delta under `build/software-captures/`, forces
at most eight llvmpipe workers, hides host GPU device nodes, and never clears the separate
hardware qualification failure latch.

![A sunny overview of the complete procedural tropical island](assets/presentation/island-overview.png)

The project remains under an active long-form development and polish campaign. The full
rendering profile requests 4x geometric-edge antialiasing on Linux/Web and 2x on Android through
CNA's existing backbuffer path, with explicit applied-sample diagnostics and device fallback.
Native Linux now defaults to the bounded `native-performance` profile without MSAA;
use the Release executable in `build-consumer/` for ordinary performance evaluation.
See [antialiasing evidence and cost](docs/edge-antialiasing.md); software results do not qualify
hardware performance or resolve the GPU incident. The current
playable slice renders a deterministic approximately 500 m island with
portable procedural
sand, wet-edge, soil, and slope material variation, including distance-faded interior substrate
microvariation, broad leaf-humus and cool-moss floor breakup, close analytic soil relief and
smooth pores faded by both distance and pixel footprint, sparse curved leaf litter, close
dry-sand granulation, soft-edged pebble/shell
flecks, subtle shoreline-raked ripples, restrained fixed-sun jungle-floor dappling, and a narrow
noise-mottled, broken-edge
earthen footpath that follows the same authored corridor reserved by vegetation. Two
authored clearings use distinct sunlit-dry and leaf-rich ground treatments with broad irregular
patch breakup derived from terrain noise that was already evaluated. Close jungle soil now adds
one project-owned floor image with a 512-square mip chain and edge-safe rotated source windows,
preserving the authored paths and clearing palettes. It is packaged on all targets; missing data
logs a diagnostic and retains procedural soil. See [material review](docs/forest-floor-material-review.md)
for provenance, software-only runtime evidence and remaining visual limitations.
A first-pass20m headland cave now has a real interior, replacement terrain, capsule collision
and flight roof containment. Its original mipmapped rock material uses mirrored triplanar
sampling, smooth connected-shell lighting and a stone apron on the replacement terrain. Six
small irregular high-clearance rock pendants break up the otherwise continuous interior arch
without narrowing the playable corridor.
Native and Chrome/SwiftShader exterior/interior captures plus a native live keyboard
entry/flight/exit test pass; natural-shape polish and Android qualification remain;
see [cave status](docs/cave.md).
A 101.879m carved western stream descends from the interior into the sheltered bay.
Its shallow transparent surface follows local bends with downstream ripples, broken
riffle highlights and tapered bank-edge sheen while retaining the visible wet bed. Shared
low-frequency pools and pinches break up its longitudinal width without separating water,
terrain, collision or habitat fields. A compact
collision-authoritative cross-section now provides visibly incised, slope-shaded earthen banks,
with sparse non-solid stones seated just beyond the carved bank blend.
The shared moisture/drainage field drives damp soil, bank plants, fallen wood, mangroves
and positional water sound; see [stream evidence](docs/stream-course.md).
A raised timber hut stands in a reserved western-interior jungle clearing, safely inland
and about57m from the stream. It has a layered palm-fibre roof, plank walls, framed windows
and door, a railed three-step veranda, an enclosed slatted gable with a four-pane attic light,
visible rafters and knee-braced posts. Its supports and contact patches follow rendered terrain;
see [hut evidence](docs/island-hut.md) for exact placement and remaining route/Android review.
A stone-ringed campfire burns in the eastern clearing, with animated flames, a pale
mottled ash bed, mixed glowing charcoal, subtle smoke/sparks, nearby flickering light
and positional wood crackling. A separate
stack of gathered firewood sits about6m away. See [campfire evidence](docs/campfire.md)
for the native first pass and remaining visual/portable checks.
Exactly one peaceful adult resident now follows a deterministic688m inland tour with pauses,
terrain-fitted planted feet and player yielding. The original procedural model has facial and
hair geometry, a bound-neck woven tunic, a short split thigh-following wrap, a small generated
material atlas, sandals and sole shadows. Use
`--viewpoint resident` for the fixed eight-stage review; [resident evidence](docs/resident.md)
records the native first pass and its remaining artistic and portable limitations.
Exactly one large peaceful bear now roams a bounded inland habitat with planted four-foot
terrain contact, quadrupedal idle/walk/trot/turn/forage/rest/withdraw behavior, near/far
procedural anatomy, a continuous rump-to-neck torso and shoulder-to-wrist limbs, five-clawed
broad paws, generated directional fur and sparse positional breath/snuffle/huff
audio. Use `--viewpoint bear`; [bear evidence](docs/bear.md) records the native first pass
and explicitly open visual, listening and portable work.
A grounded original procedural utility helicopter now occupies a vegetation-cleared inland
summit site. It cannot fly: both sides instead have open pilot and rear-cabin doors, stepped
thresholds, a supported walkable floor, seats and cockpit controls. Four outward-open framed and
glazed door panels make those entrances visually explicit without blocking them. Four stationary
main-rotor blades are closed tapered volumes rather than view-dependent flat sheets. The pilot
station has round gauges with needles, paired cyclic/collective controls and four rudder pedals,
without enabling flight. Its skid contacts follow
the rendered terrain while wildlife avoids the airframe. Use `--viewpoint helicopter` or
`--viewpoint helicopter-inside`; [helicopter evidence](docs/helicopter.md) records the native
first pass and remaining material, interaction and portable review work.
Live jungle leaves now use a small mipmapped image to vary vein and tissue colour,
preserving species tint and existing lighting; see [leaf material evidence](docs/live-leaf-material.md).
An animated tropical
ocean adds distance-faded crossing-wave colour structure, restrained shallow-ridge light and
sunward ripple crests plus view-aligned grazing reflection bands from its existing slope field,
with a shared nine-wave spectrum, gently irregular short-wave normals and per-wave pixel-footprint
filtering. Grazing sky reflections become less transparent than downward views into the lagoon.
Fine waves now vary in strength across slowly moving wind patches, with a wider spread of short
directions and analytically consistent normals; broad water displacement is unchanged.
Supplementary grazing contrast is bounded, and broad breaker foam is limited by shallow depth
and shore distance rather than appearing across deeper water. Fine wave repetition still needs polish.
The sea floor uses continuous sand colours across the waterline, with filtered shallow sand ripples;
water tint and opacity increase with local depth instead of baking turquoise into the substrate.
An exact shore-following foam/wash ribbon uses advancing, fading crests and
pixel-footprint-filtered porous foam instead of a continuous white stripe. The landscape contains
322 procedural coconut palms in coastal groves, 270 broadleaf canopy trees, 1,770 ferns/large-leaf plants, 582 low
bushes, 3,008 small forest-floor plants, and 16,809 mesh-grass clusters placed from a shared canopy-light and ground-cover field. It also composes 184
irregular land rocks, 14 emergent near-shore reef outcrops, 11 mossy fallen logs with irregular
near broken-end contours,
33 pieces of sun-bleached beach driftwood, 33 curved fallen palm fronds with asymmetric leaflets,
and 233 near-field
shell/pebble/dry-wrack clusters. Three varied driftwood silhouettes form a non-solid landmark arc
in one sheltered cove;
five rocks and two fallen logs form natural orientation landmarks around the outer rings of the
two authored clearings without blocking their central walking room. Every dense near-island
visual layer has spatial culling and appropriate distance
reduction, while vegetation uses a calm breeze. Five unreachable, haze-softened low islets and 11
simplified palm silhouettes break up the open-ocean horizon in one intentionally always-present
portable static batch.
One beach shallows now contains a large original wooden sailing-ship wreck rather than a small
boat. Its exact 20 m hull is grounded and heeled in the water with a central break, exposed ribs,
partial deck, an open broken stern deckhouse, snapped mast, bowsprit, and rigging. The deckhouse
uses separated uneven roof boards, framed window bays and one true through-opening instead of a
solid cabin box. Twelve planks and substantial fragments
form a separate trail toward the sand. Six conservative hull proxies keep the player out of the
major structure while the smaller debris remains non-solid and traversal-friendly. The complete
5,532-triangle landmark uses a finer curved hull section, true missing-plank bays with visible inner
ribs and 11cm closed inner edges, a split broken keel and rudder, broken deck rails, raked lower bow/stern profiles, dark hull
openings, and a splintered foremast stump. The debris
trail varies its angle strongly in both directions instead of repeating the ship axis. One portable
GLSL 330 / GLSL ES 300 material adds distance-faded
longitudinal grain, weathering, restrained cracks, wet/submerged darkening, salt wear, and a small
waterline sheen. A prepared 256-square weathered-timber albedo now adds photographic-scale fibre,
abrasion, salt and restrained algae detail. Per-piece UVs align its grain with individual hull
boards, deck surfaces, beams and scattered debris; mirrored interior sampling avoids relying on
the generated source's unqualified edge tiling. Sparse aged-iron hull straps, rivets, cabin hinges
and rudder braces use a separate bounded rust and sheen response rather than receiving wood grain.
It remains frustum/distance culled and is submitted as one batch before the
transparent ocean colours its submerged portions; it introduces no quest, pickup, or objective.
Nearby mesh grass bends away from the player: a small stationary body radius expands smoothly with
walking speed and releases after movement stops, entirely inside the existing instanced shader.
The 42-blade near mesh uses five curvature levels per blade, geometric V-folds, and portable
two-sided daylight/root/transmission shading. Shared surface normals follow rest-shape curvature
and instance scale; wind/player motion still uses approximate normal response. Pigment now stays
separate from fixed side lighting, with restrained filtered longitudinal grain; see
[grass surface material and evidence](docs/grass-surface-material.md). Its close blades are deliberately narrower than the
middle tier, use small deterministic left/right edge asymmetry, and occasionally bend and droop at
the tip instead of repeating broad symmetric ribbons. Their upper width now tapers progressively,
and seed stems attach to the actual curved terminal centre/ridge. A sparse juvenile underlayer, six restrained
seed-bearing stems in the near cluster, and deterministic per-instance blade stature, colour, and
wind variation
keep neighbouring tufts from repeating the shared mesh literally; the cheaper far mesh uses three
levels and still merges into the terrain before culling.
The 42 near blades form seven unequal loose tufts, with outward-facing leaves, short peripheral
blades and related wind phases. This reuses the existing geometry and draw budget.
Nearby palm crowns use nine paired, fuller tapered-leaflet stations on each of 10--12 curved fronds
instead of solid ribbon silhouettes. A cheaper widened ribbon preserves the crown envelope at
distance,
while per-palm 125--145 m thresholds stagger the transition and avoid switching whole cells at once.
The near tier also adds texture-free leaf-scar rings to the trunks and restrained faceted coconut
clusters to two of the four variants without another draw submission. Five irregular faceted root
wedges vary in angle, length, width, and height around each near trunk instead of ending it in a
flat cylinder or a regular stamped star. The palm shader also derives a restrained green-only
frond tint from each existing instance transform, so neighbouring crowns no longer share one
literal green while trunks, roots, coconuts, near/far identity, buffers, and draw count remain
unchanged. Two selected near variants also carry two or three irregular hanging dry-frond
remnants beneath the living crown. Their broken ochre-brown leaflets add sparse age variation
without turning every palm into a uniform dead skirt; the far silhouettes and submission count
remain unchanged.
Nearby broadleaf crowns use small irregular inner foliage masses, visible tapered support
branches, and layered shells of 584 curved canopy blades per tree. Golden-angle sprigs, unequal leaf
sizes and height-dependent tints break up smooth silhouettes without alpha blending or another
draw pass. Eight shared crown forms vary branch architecture as well as leaf posture. The near
meshes have 4,328–4,378 triangles; the 1,064-triangle far tier retains 104 original blades and
primary branches. The foliage is still procedurally stylized rather than photorealistic.
Terrain, grass and jungle ground layers share approximate fixed-sun shade baked from the placed broadleaf crowns,
replacing unrelated noise-based sun patches. The static 4 MiB field preserves ambient skylight;
it is not leaf-exact shadow mapping. Boars sample the same light per body vertex, retaining a
1 MiB CPU field without another GPU texture or draw. Monkeys reuse it through a smoothed body
sample and the existing skinned material, including reduced direct specular light in shade.
Other wildlife and palms are not yet integrated. Understory,
bushes and small plants project their elevated leaves along the same sun direction, retaining
ambient light beneath crowns instead of remaining uniformly bright against shaded soil.
The same near meshes give trunks a restrained secondary growth bend and slight ring/azimuth girth
variation, while each support branch tapers through a deterministic off-axis middle joint instead
of reading as one straight cylinder. Near/far crown anchors agree and collision proxies remain
unchanged; the expanded form set adds shared meshes and variant batches.
Two of the eight near variants also carry one sparse hanging liana
with a curved tapered strand and two small leaves, adding vertical jungle detail without another
instance or draw. Their near trunks now meet the forest floor through four or five
short irregular buttress roots instead of a uniform cylinder, using the same instanced mesh and
draw submissions. Close bark gains restrained procedural longitudinal grain and narrow fissures
that fade before the far tier, without a texture or new pass. A deterministic per-tree 105--129 m
threshold staggers their
transition to the far mesh instead of switching complete scene cells at once.
The shared portable jungle shader uses smooth normals on curved and pointed leaves, with
subtle footprint-filtered veins and a restrained waxy sun reflection nearby. Fine material detail
fades between 16 and 38 m; bark and other geometry retain their geometric two-sided lighting.
Leaf coordinates and normals share the existing vertex buffer; no texture or draw was added.
Its vertex stage also recombines each existing instance transform's scale/yaw coefficients into a
stable restrained foliage tint. The green-only mask leaves bark unchanged, and the shared transform
keeps the same tint identity across near/far LOD without another attribute, texture, or draw.
Low bushes use supported shoots and individual curved leaves. Both detail tiers retain
the complete leaf outlines; far removes inner rib vertices without extra draw submissions.
Two medium understory forms now use pinnate fern fronds, with tapered paired leaflets aligned
to a curved rachis and attached through shared wind vertices. Far meshes retain original leaflet
identities. This adds geometry but no material, texture or draw; mobile cost and moving LOD
quality remain unqualified. Two other forms use broad paddle/lance profiles with mildly
asymmetric margins and cupped surfaces, reusing the same mesh budget.
One near forest-floor variant forms six upright broad leaves on visible petioles, while another
uses five low folded split leaves instead of repeating the strap-leaf rosette. Their
far representations retain reduced original leaf subsets with unchanged batching. Understory, bush, and forest-floor
LOD and maximum-distance selection use actual instance positions after a conservative cell reject,
so first-person detail no longer changes early at a coarse cell boundary.
Land and reef rocks use shared smooth normals, scale/yaw-correct fixed-sun lighting and refined
near silhouettes. Texture-free mineral mottling, filtered fine grain and restrained lichen replace
the former triangle-facet lighting and repetitive strata. Fine detail fades before the far LOD;
broad colour and daylight remain consistent. A fixed mean-sea-level band darkens their wet bases.
This uses one additional normal attribute in the existing instanced pass, not another draw or
texture. A restrained warm/cool tint derived from each
existing instance transform varies neutral stone tones while preserving one identity across the
near/far LOD and leaving wood and foliage colours unchanged.
Nearby fallen logs use the same portable material path for gently irregular cut-end growth rings,
restrained radial drying cracks, and longitudinal bark fissures. Both rock and wood detail fade
out before their LOD transition.
Wood surface normals now follow each instance's rotation and nonuniform scale, with outward-lit
cut ends and separate hard rims. Far wood keeps the same daylight response without close detail.
Sun-bleached driftwood adds a still softer near-only longitudinal weathering treatment. Its third
near variant replaces a simple spike with three curved tapered fork limbs, while cut ends and all
far meshes stay calm and inexpensive. Rocks, fallen logs, and driftwood select range and LOD from
each instance after a conservative cell-bounds rejection rather than switching an entire cell.
Nearby palms, canopy trees, land rocks, fallen wood, driftwood, monkeys, and boars receive subtle
terrain-conforming contact shadows. The twelve-segment soft ellipses use one portable
`BasicEffect` batch, fade before their bounded draw distances, and cache static terrain sampling
plus movement-thresholded uploads so this grounding cue does not become a per-frame procedural
terrain cost.
Land-rock and wood ellipses are split at rendered terrain triangles to avoid coarse-surface
mismatches. Fallen-log and driftwood model heading now matches placement/support and collision
heading; terrain layout and walking routes are unchanged.
Reef outcrops, the two broken hull sections and twelve wreck fragments now also receive soft
grounding patches. These are clipped to the actual seabed/terrain triangles, drawn beneath the
water in the existing batch, and preserve the gap between the hull halves. They remain contact
approximations, not full mast/rigging or directional shadow silhouettes.
It supports
first-person terrain walking, near-shore swimming, trunk collision, and a soft ocean-current
boundary whose always-shoreward return includes mild coherent coast-parallel circulation,
dual-zone touch input, a subtle depth-responsive near-surface water veil with upper-surface light
scatter that cools and strengthens as the offshore current develops, and speed/depth-responsive
player ripples that expand across the
same animated ocean surface without another draw call. Moving entries and exits also trigger one
short speed-scaled pair of broken surface rings, with depth hysteresis and a cooldown that prevent
shoreline chatter. A fixed sunny gradient sky adds a sun disc
and sparse, layered horizontal high-cloud wisps without a weather simulation. A first
flock of 24 procedural birds follows varied shore, canopy, and high-altitude routes with animated
wing strokes, layered trailing flight feathers on the near silhouette, intermittent glides, banked
turns, slow radial route drift, terrain clearance, visibility culling, and distance-throttled
simulation. Six non-hostile wild boars now forage, rest, wander, and retreat from nearby players
in bounded inland habitats, with solid-prop avoidance, adult/juvenile procedural models,
continuous body/head profiles, folded ears, near nostrils and bent tusks, smooth fixed-sun shading
and terrain-following contact shadows. An original 128x128 opaque directional coat atlas adds
fine body strands through stock BasicEffect; a padded white region leaves eyes, tusks and hooves
unpatterned. The three coat tints and juvenile markings remain shared with the mesh colors.
Legs use connected tapered upper sections plus a continuous knee-to-cannon taper and split
near hooves, with a grounded stance and lifted return phase; short swept bristles follow the
same body profile. This is still approximate
procedural animation: individual soles fit the rendered terrain with fixed-length limbs,
but world-space foot locking and stride-speed synchronization remain unfinished.
Use `--viewpoint boars` for their dedicated inspection camera. This original asset still needs
visual refinement; its current runtime validation covers native llvmpipe and actual browser
WebGL 2 through SwiftShader, not post-incident AMD or Android device qualification.
The juvenile broadleaf ground plant now uses a curved leaf surface and retains real source
leaves in its reduced-detail model. Use `--viewpoint ground-broadleaf` for its inspection
camera; see [ground broadleaf validation](docs/ground-broadleaf.md) for exact scope and cost.
The split-leaf ground plant uses connected, curved lobes with shared near/far source
identities. Its `--viewpoint ground` camera and [validation notes](docs/split-ground-leaves.md)
track the remaining terrain-intersection and LOD limitations.
Shrubs now use rooted woody shoots and smaller individual curved leaves, retaining
all source leaves and outlines at far LOD. Use `--viewpoint bush` for the targeted camera;
[shrub validation](docs/shrub-lod-continuity.md) records the increased geometry cost and
remaining coverage/animation limitations.
The floor layer includes 199 irregular canopy-associated groups with 866 smaller
companion plants, while preserving paths and clearing accents. See
[placement validation](docs/forest-floor-colonies.md) for bounds, cost and visual limitations.
Jungle root origins now follow the actual rendered terrain triangles; see
[root-contact validation and remaining slope limits](docs/jungle-root-contact.md).
Near rosette litter also follows slopes through the shared terrain height map;
[contact measurements and remaining limits](docs/fallen-leaf-contact.md) document the change.
Thirty small beach crabs occupy six dry/wet-sand colonies and
alternate asynchronous
pause, forage, and lateral scuttle states. They stay inside terrain-validated home ranges, retreat
briefly from a nearby player, and use low, laterally broad panelled carapaces, sturdy volumetric
three-segment legs, short eye stalks and enlarged palm-backed solid split claws in one culled
near/far batch. See the
[close-form audit](docs/crab-form.md).
Eighteen butterflies occupy three sunny clearing-edge habitats, alternate rest, flutter, and
nectar-hover behaviour, and stay inside low terrain-relative altitude and home-range bounds. Five
palettes, hinged patterned near wings, a reduced far silhouette, and one culled batch make their
motion readable without filling every clearing. Eight
near-shore schools add 48 animated fish constrained between the seabed and water surface, with
depth-following pitch, banked turns, forked tails, CPU-lit body facets, restrained depth/caustic
colour response, and seven-station/ten-facet volumetric near bodies with rounded heads, caudal
peduncles, two-facet highlighted eyes, surface-fitted gills, lit shaped fins and progressive
rear-body motion. See
[the image-driven fish form review](docs/fish-form.md).
Five non-hostile sea turtles follow separate bounded near-shore routes between the seabed and
surface. They alternate slow cruising, seabed foraging, and occasional surfacing, remain outside
land and the outer-current region, and respond mildly to a nearby swimmer. Faceted scute-patterned
shells, articulated flippers, three natural palettes, near/far geometry, and one culled dynamic
batch keep them readable through the water without making them a constant attraction. The current
close pass uses subdued geometric scute seams, a fuller four-ring head, surface eyes, nostrils and
a mouth crease; see [the sea-turtle form review](docs/sea-turtle-form.md).
Twenty-four birds retain their three flight-route families while at most two use bounded approach,
perch and departure states on real tree branches, land rocks or the wreck beam. Perched near birds
fold their wings and place small feet on the support without another draw call.
One non-hostile shark
occasionally patrols deeper offshore water with matching dive pitch, turn banking, a forked tail,
depth-integrated colour, and an eight-station/ten-facet near body with rounded blunt snout, caudal
peduncle, heterocercal tail, lit shaped paired fins, faceted eyes and five gill slits per side. See
[the image-driven shark form review](docs/shark-form.md).
Four peaceful dolphins cruise beyond the western reef in bounded home ranges, alternate cruising,
surfacing and diving, and use pitch, banking plus a vertical tail-stock stroke through animated
horizontal flukes in one culled batch.
Their 1,476-vertex close form uses a thirteen-station, sixteen-sided slender body, a rounded melon
above a short closed beak, restrained countershading, shaped closed-volume fins, faceted eyes,
mouth/blowhole cues and horizontal flukes. The fixed `--viewpoint dolphins`
camera follows a live subject for review; see
[the dolphin implementation and validation notes](docs/dolphins.md).
Seven original procedural monkeys roam the interior using a compact, obstacle-aware state machine,
three silhouette meshes, four material palettes, portable CNA GPU skeletal animation, and short
state-transition crossfades. Near animals use generated directional fur, a sculpted rump/waist/chest
loft, paired fur cheek ruffs around the skin mask, a heavy hooked tail with raised fur clumps, and
bone-following volumetric palms/soles with four forward digits and a lateral opposable digit. A
shallow fitted dark nose, visible nostrils/mouth and slightly raised corneas break up the pale mask
without another texture, material or draw.
They approach qualified trees, grip actual wind-deformed bark, ascend to a supported summit and
descend without teleporting. Calm idle/observe animals gradually turn
their existing head bone
toward a nearby player within a restrained angle, then relax or retreat without becoming hostile.
The current audio pass adds original procedural shoreline surf, two
varied occasional stronger waves, light canopy breeze, a subtle daytime-jungle texture,
one positional flowing-stream layer following the shared watercourse,
speed/depth-responsive movement through shallow or swimming water, and two restrained
water-entry/exit textures. The same continuous water layer now develops a subtle deeper undertow
bed as the 42--72 m offshore current strengthens, without adding another persistent source. Two
bird phrases and two monkey contact phrases share one positional
voice attached to actual nearby agents. All six generated loops remove their measured mean
before PCM quantization, avoiding long-lived DC bias without altering one-shot silent endpoints.
Stream and player-water playback share one active voice, so no more than four ambient loops run
concurrently.
Behaviour-aware boar grunts/snuffles/breathing, close retreating-snake hisses, peaceful bear
breathing/snuffling/huffs, and goat bleats/breathing/hoof detail reuse the same bounded positional
wildlife voice. Calls follow the moving animal, attenuate by distance, vary deterministically and
respect mute without expanding the six-source ceiling. These are original synthesized prototypes;
the remaining headphone/speaker naturalness and portable lifecycle review is tracked separately.
Distance-scheduled player footsteps select paired dry-sand or interior-soil textures from actual
resolved travel, vary pitch/pan/gain, and remain silent while swimming or blocked.
All audio is generated without external sound files. The
complete scene runs on Linux/OpenGL 3.3, in an actual Chromium/WebGL 2 context, and in a
KVM-accelerated Android API 35 x86_64 emulator through OpenGL ES 3. The accelerated Android pass
also exercises movement and look gestures, AAudio startup, and background/foreground recovery.
Android uses a centralized mobile quality default that retains all content and close detail while
reducing distant vegetation/grass/shadow/wildlife work and the ocean grid. Web retains the full
portable profile. Native Linux defaults to a performance-first profile after a measured flight
regression; it preserves populations and close models while bounding distant vegetation, grass,
shadows, wildlife, ocean tessellation and multisampling.
Physical-device touch ergonomics, thermals, and sustained mobile-GPU performance are not yet
qualified.

There are deliberately no quests, combat, inventory, crafting, survival systems, score,
progression, dynamic weather, or day/night cycle.

## Dependency baseline

The project consumes read-only CNA `next` and Sharp Runtime `next` checkouts. Exact qualified
commits are stored in `dependencies.lock`. The current workspace checkouts are both on `next`; the
project still creates an ignored, unmodified CNA source export so its build input remains pinned:

```bash
tools/materialize_cna_next.sh
```

This does not switch or edit the CNA repository. A separate read-only CNA checkout on `next` can be
used instead with `-DCNA_ROOT_DIR=/path/to/cna`.

## Linux development build

The machine-wide ccache is required. Sharp Runtime's stricter build policy limits builds that
compile its sources to two parallel jobs.

```bash
export CCACHE_DIR=/rv/cnaccache
export CCACHE_BASEDIR=/rv
tools/materialize_cna_next.sh
cmake -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_C_COMPILER_LAUNCHER=ccache \
  -DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
  -DCNA_GRAPHICS_RENDERER=OPENGL33
cmake --build build --target island-simulator island_core_tests island_world_report \
  island_audio_report --parallel 2
ctest --preset linux-core
```

The graphical tests need an isolated display; use the command in `docs/build-linux.md` rather than
running the full CTest set on the owner's live desktop.

For a stripped, relocatable Linux release bundle, use the dedicated two-worker preset and
packager:

```bash
export CCACHE_DIR=/rv/cnaccache
export CCACHE_BASEDIR=/rv
cmake --preset linux-release
cmake --build --preset linux-release
tools/package_linux_release.sh build-consumer
```

This produces `build-consumer/island-simulator-linux-x86_64.tar.gz`. The archive contains the game,
the exact built `libcna.so`, dependency licences/notices, attribution, dependency pins, and a
SHA-256 manifest. The executable resolves CNA through `$ORIGIN/lib`, so the extracted directory
can be moved and launched from an unrelated working directory. Compatible system SDL 3,
SDL_mixer 3, glibc, libstdc++, and an OpenGL 3.3 driver remain explicit host dependencies rather
than silently copied distribution-specific libraries.

For ordinary play, run `build-consumer/island-simulator` after the Release build above.
`build/island-simulator` is the unoptimized Debug version for development, not a representative
performance build. Runtime logs identify the build configuration. Click inside the window or press a movement key to capture the mouse,
use WASD or the arrow keys to move, and press Shift to toggle between walking and running on land.
Each keyboard toggle briefly confirms `WALK MODE  4.3 M/S` or `RUN MODE  8.0 M/S` near the bottom
of the view; no key must remain held.
The selected walk/run mode, camera-motion toggle, audio mute state, and Android touch-look
sensitivity are saved automatically through CNA's platform storage and restored on the next
ordinary launch. Isolated smoke/capture/benchmark runs do not read or change player preferences;
`--no-user-settings` provides the same isolation for an explicit diagnostic launch.
The executable may be launched from any working directory; current runtime content is embedded or
generated and does not depend on a repository-relative asset path.
On Linux, press Escape once to release the mouse and again to exit. In a browser, Escape only
releases pointer lock; close the tab through the browser UI when finished. Mouse look remains
active while a movement key is held. The controller follows the terrain,
rejects overly steep uphill motion, slides along trunk, solid-rock, and fallen-log proxies, and
enters swimming water with hysteresis. Swimming retains collision with solid emergent reef rocks.
A static 16 m collision grid limits each capsule test to nearby proxies; bounded one-time recovery
prevents a placement or debug teleport from leaving the player embedded in a static proxy.
Geometric step-up and continuous recovery from moving obstacles are not implemented yet.
Walking and swimming apply restrained, speed-responsive positional camera motion; press B to
smoothly disable or restore it.
Press F1 to toggle the developer overlay with frame timing, player/water state, renderer identity
and the actual logical backbuffer size,
culling counts, contact-shadow class/draw statistics, vegetation draw statistics, ambient mix
levels, grass-interaction strength, player-ripple and transition-burst intensity/phase, player-water
gain, wave/splash counts, undertow cue strength, and positional bird/monkey call activity.
The wave counter is joined by a separate wreck-creak count; sparse positional hull creaks reuse
the stronger-wave transient voice only near the grounded ship and never add a persistent source.
While F1 is visible, F2 toggles an x-ray world diagnostic: the authoritative shoreline and three
ocean-current thresholds, nearby collision proxies, the player footprint, shared 8 by 8 scene
cells, all nine terrain-contact samples and normals, wildlife headings, simulation-tier colours,
nearby fish-school centres, the shark route target, monkey home ranges and active movement targets,
three bird-family route volumes, all 24 authored bird spawn headings, and nearby live radial targets,
the last exact attempted/resolved player step with rejection classification,
the exact active inward/circulating-current vector, all six ambient crossfade boundaries, the nearest bird/monkey
audio candidates, and any active positional wildlife emitter. The panel also reports active audio
sources against the six-source ceiling.
In debug builds, F3 toggles a bounded inspection camera while F1 is visible. Mouse look remains
unchanged; WASD flies horizontally, Q/E moves down/up, and Shift increases speed. The player stays
fixed, and hiding F1 immediately restores the first-person camera.
The overlay also shows the four authoritative procedural-layout seeds. Press F4 while it is visible
to copy a stable seed report; the panel reports `COPIED` only after reading back the same text and
otherwise reports `UNAVAILABLE`.
Press M to mute or unmute the soundscape. Press B to toggle locomotion camera motion. While the
overlay is visible, F5–F10, J, K, L, O, and F12 teleport among QA locations (K cycles through boars,
L goes to the cave entrance and O to the campfire), including views toward
the south fish shallows, offshore shark, monkey troop, and the active offshore-current field.
F11 toggles fullscreen on desktop/Web (subject to browser permissions); Android stays immersive.
Tab toggles player flight: WASD stays horizontal, Space ascends and either Shift descends.
Flight stops at terrain, a 100 m above-terrain ceiling and a 330 m island-centred
radius. Tab again returns to safe dry ground. See [flight controls](docs/flight-controls.md)
for exact limits and the remaining portable/runtime verification scope.
The overlay starts
hidden in a normal run and can be enabled at launch with `--debug-overlay` for automated QA.
Use `--world-debug` to enable both the panel and world visualization at launch.
The 2026-10-08 AMDGPU GPUVM incident currently requires the fail-closed serial procedure in
`docs/incidents/2026-10-08-amdgpu-gpuvm-reset.md`; a private compositor still shares the physical
GPU with the desktop. After that incident is closed, `tools/run_native_benchmarks.sh` provides
warm, uncapped JSON frame-time reports across five fixed scenes plus a 676 m moving traversal. Reports include
min/max/mean scene-population ranges, timing distributions, and exact 120/60/30 Hz budget-miss
counts; methodology and the current Linux release baseline are in `docs/performance.md`.
Use `--startup-report <path>` to write a machine-readable first-draw report. It separates
deterministic world construction, framework initialization, GPU/content creation, the interval
before the first draw, and the first submitted draw. Combine it with `--smoke-test --overview` for
an unattended run; the measurement ends before CNA presents the frame and is therefore not a
GPU-completion timer. The native CTest suite validates the report after an actual OpenGL 3.3 draw.
Use `python3 tools/validate_assets.py` to enforce the asset provenance gate and
`build/island-world-report --output <path>` for deterministic terrain, placement, collision,
wildlife, seed, and fingerprint statistics. Native overview captures are followed by
`tools/validate_capture.py`, which rejects truncated, black/white, flat, or spatially degenerate
P6 output and writes machine-readable metrics. See `docs/asset-pipeline.md` and
`docs/debugging.md`.
Use `build/island-audio-report --output <path> --wav-directory <directory>` to audit every
procedural source and export disposable PCM16 WAV previews for focused listening. See
`docs/audio-review.md`.

The automated browser smoke test verifies renderer initialization, the portable terrain-material,
animated-ocean, and fixed-sky shaders, hardware-instanced palm/jungle/prop LOD, portable instanced
grass wind, a finite first-draw startup timing line, four frames, clean shutdown, active
overview/beach/jungle/prop/driftwood/reef/shipwreck/crabs/butterflies/turtles/water/
current/grass/frond/distant/bird/fish/shark/monkey screenshots, the transient run-mode confirmation,
and portable overlay text rendering.
The close prop view additionally requires a nonzero one-draw contact-shadow batch.
The driftwood view frames the authored three-silhouette cove group, including the forked third
variant, and requires a nonzero per-instance near-LOD population.
The Chromium harness also drives first-click pointer lock and initialization of all ambient and
positional-wildlife audio resources, WASD, arrow-key movement, Shift run/walk toggling,
simultaneous forward movement and relative mouse look, a deterministic beach-to-water crossing with successful
transition-audio playback, successful land-footstep playback, F2 world
lines, F4 seed-copy execution with explicit clipboard status, F1,
real-Escape pointer release, continued live frames, delayed click reacquisition, canvas focus, and
responsive 16:9 sizing. A narrower Firefox ESR 140.15 WebDriver BiDi gate independently checks the
production shell's first frame, reported WebGL 2 Radeon backend, trusted-click pointer lock,
44.1 kHz audio initialization, persistent Shift enable/disable edges, forward traversal with an
actual footstep, Escape release, delayed pointer-lock reacquisition, and responsive 16:9 sizing.
It also loads dedicated 1280 by 720 jungle, near-surface-water, 20 m shipwreck, beach-crab,
butterfly, and sea-turtle scenes, requires
their live state, and captures each. All Firefox scene frames were visually inspected. A visible
interactive listening pass, the remaining fixed-scene matrix, long tab suspension, audio-focus
behaviour, and fullscreen remain open. The automated Firefox path now does one actual second-tab
activation cycle and requires `visible` to `hidden` to `visible`, resumed animation frames, and
fresh game input after foregrounding.

After the WebGL 2 release build, `tools/package_web_release.sh build-probe` creates
`build-probe/web-release/` with verified uncompressed artifacts, deterministic gzip variants, and
a SHA-256 manifest. The current gzip payload is 2,587,860 bytes, 71.5% below the 9,085,111-byte
uncompressed package. See `docs/build-web.md` for required server content types and
content-encoding headers; the quick Python development server does not exercise compressed
delivery.

The Android project produces arm64-v8a device and x86_64 emulator APKs with an explicit GLES 3
renderer. The left half of the screen supplies a floating movement stick and the right half uses
relative drag-to-look, with subtle generated premultiplied-alpha control indicators. Near-full
movement-stick travel selects the faster land pace with a warm-gold visual cue; easing inward
returns to analog walking. A small three-dot control at top right cycles low, standard, and high
look sensitivity (0.72/1.00/1.35 times); the highlighted dot shows the current choice. Android
Emulator 36.5.11 runs the API 35 x86_64 AVD with KVM CPU acceleration and host-GPU gfxstream.
The complete current scene, independent movement/look gestures, all sensitivity levels, a
same-process hot resume, force-stop/new-process relaunch, and AAudio pause/resume are validated
there. Dedicated current-source GLES 3 views also validate the inland humus/moss floor material,
the layered near-canopy edge plus irregular trunks and bent support branches, clearing butterflies,
a near-shore sea turtle, and sparse hanging dry-frond variation through the mobile quality profile. The
persistence-qualified run also
requires a touch sensitivity change to survive the cold relaunch. The default arm64-v8a APK is
19,082,004 bytes, contains a 16,271,720-byte stripped
`libmain.so`, passes app lint, and verifies with APK signature scheme v2. This is meaningful
emulator qualification, but it does not replace physical-device
ergonomics, thermal, driver, and sustained-performance testing. See
`docs/build-android.md` and `PLAN.md` for exact evidence and commands.

## Project records

- `PLAN.md` is the living development ledger.
- `BUGS.md` records confirmed CNA or Sharp Runtime defects and game-side workarounds.
- `ASSET_LICENSES.md` records all asset provenance before assets enter a runtime build.
- `docs/asset-pipeline.md` defines the manifest, validation gate, deterministic world report, and
  external-asset intake procedure.
- `docs/audio-review.md` defines deterministic source analysis, WAV preview export, and the
  remaining subjective listening protocol.
- `docs/performance.md` defines the repeatable benchmark method, current evidence, and open budgets.
