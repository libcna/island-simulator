# Island resident — native and Web visual pass in progress

## Restrained nasal tip and separate lower lip — 2026-10-16

The renewed animal/resident comparison retained the ten-ring shoulder silhouette, tapered neck,
cheeks, jaw, eyelids, nostril wings and gait but selected the facial profile: the round nasal tip
still dominated the quarter view and the mouth ended as one dark upper chevron. The nasal tip and
wings are now narrower and shallower while preserving the bridge and paired nostrils. A separate
two-segment lower-lip contour sits beneath the existing peaked upper lip, producing a closed human
mouth instead of one painted line.

The lower lip adds 32 triangles, taking the body from 7,122 to 7,154 triangles beneath the unchanged
40,000-vertex stream capacity. It shares the existing material atlas and single opaque body draw;
the second draw remains the 24-triangle foot shadow. Exactly one peaceful resident, the route,
terrain-fitted gait, arm counter-swing and player avoidance are unchanged.

Five focused native groups and all 41 browser/capture harness tests pass. Same-camera native
baseline `20261015T215422.757419Z-resident` and accepted eight-stage evidence
`20261016T072111.165937Z-resident` were inspected side by side. The accepted 1280x720 frame passes
at 291 colour bins, 42.54 luminance deviation, 158.56 spatial contrast and kernel `[]`; its quarter
view shows the restrained profile and two lip contours without losing the facial silhouette.
Inspected Chrome 155/SwiftShader
`build/browser-software/20261016T072542.438718Z-chrome-resident` passes two distinct walking frames,
60 finite positive intervals, GL error zero, a live context, two draws and kernel `[]`.

## Foot-synchronised arm swing and honest quarter-view audit — 2026-10-16

The renewed nine-frame native audit showed that the existing ten-ring torso, trapezius neck,
layered face and shaped limbs were already materially better than the old mannequin, but the arms
remained nearly vertical while the feet stepped. The authoritative motion controller now increases
the foot-synchronised sagittal counter-swing from 0.24 to 0.52 radians and keeps a relaxed 0.42-radian
elbow offset. Bone lengths, shoulder roots, planted-foot IK, route, yielding and the single peaceful
actor are unchanged. The fixed QA camera moves from an almost frontal 18-degree offset to a
distance-matched 37-degree quarter view so arm depth, torso shape and facial profile can actually be
inspected rather than hidden by projection.

The eight-stage capture log now records both world-space wrists and each wrist's local forward
reach relative to its shoulder and current heading. Validation requires both arms to span at least
150mm; this cannot be satisfied merely by translating the actor. Final current-binary llvmpipe
`20261015T215422.757419Z-resident` passes at 1280x720 with 291 colour bins, 42.54 luminance
deviation, 158.58 spatial contrast and an empty kernel delta. It records 2.671m actor displacement,
2.804m foot motion, and independent local arm-swing ranges of 0.494m and 0.497m. All nine images
were inspected as a montage.

Focused Chrome/SwiftShader `20261015T220238.956562Z-chrome-resident` passes 60 finite positive
intervals, `glError=0`, a live context and an empty kernel delta. Its two distinct inspected images
show the counter-swing and quarter-view silhouette. The long deterministic resident test enforces
more than 350mm of local forward range for both arms while preserving exact upper/lower arm lengths
and elbow bend; 96 focused Python gates pass. Android runtime and continuous subjective traversal
remain open.

The operator requested exactly one dark-skinned adult fictional resident with believable
human proportions, textured skin/hair and practical woven-cloth/plant-fibre tropical
clothing. Natural walking/turning, pauses, terrain-aware foot placement and obstacle/player
avoidance are required. No combat, quests, trading or compulsory interaction is planned.

Exactly one resident is now simulated and visible in IslandGame. The native first pass has
an original procedural human mesh driven directly by the grounded pose below. It includes
an articulated torso, neck/head, facial features, close-cropped hair, hands, jointed limbs,
woven clothes with short open cap sleeves, belt and plant-fibre sandals. A generated128x128 mipmapped atlas
adds separate restrained skin grain, hair curl, cloth weave and fibre patterns. No downloaded
human model, cultural costume or third-party character art is used.

The captured model remains a bounded procedural pass rather than final realism: its silhouette
and face read as an adult human, but the clothing remains angular and skin shading is coarse.
Native and Web close views are now inspected; Android appearance and representative hardware
performance are not qualified. Keep the resident PLAN entry in progress.

## Deterministic tour

ResidentRoute builds a closed tour using the current rendered terrain and actual world
collision proxies: palms, jungle trunks, props, wreck, campfire/woodpile and hut. It keeps
clear of the entire cave reserve and excavated stream. Five approaches connect the fire
clearing, northern walking corridor, western clearing, southern corridor and central trail.
Default world:25 simplified route points,688.249m total, five designated pause locations.
Construction takes about135ms in an isolated native Release test; this is setup cost,
not a frame-rate measurement or mobile timing claim.

A3m navigation grid caches static point/edge eligibility. Deterministic A* ties use node
index, and every simplified line is checked against actual ground and solid clearance.
Disconnected/fully blocked tours are refused rather than teleported or cleared of trees.
The route follows dry, walkable ground. It does not require a new island navigation engine.

## Contact and animation state

ResidentMotion owns the terrain cache and copied collision grid and refers to an immutable
IslandTerrain which must outlive it. The usual walking pace is0.95m/s, reduced on slopes;
heading turns are bounded. Named stops pause6–10s. A nearby grounded player makes the
resident pause/yield; movement resumes when the player leaves. The visible actor currently
stops rather than sidestepping, so stronger player separation still needs integration/review.

The controller keeps two world-space planted soles. One foot swings at a time, with a
bounded lift and a terrain-fitted landing. Heel/toe/side probes fit the emitted terrain
triangles and tilt the sole to its support. Two-segment IK preserves0.43m thigh and0.44m
shin lengths, including recovery on slopes; it never silently accepts stretched contact.
Arm counter-swing follows the stepping foot instead of a fixed-distance clock that could
drift after hill strides or interrupted steps. Pelvis support adjusts to planted feet.
This is physical posing, not evidence that the final human animation looks natural.

Large elapsed times are clamped to0.1s and subdivided into at most0.02s steps; invalid
times do not move the actor. No background worker, graphics context or audio source is used.

## Native CPU and visual evidence

IslandResidentTests runs two identical controllers through1200 simulated seconds against
the actual generated world. Their routes, state and poses are exactly equal. The resident
completes at least one full tour; the trace reaches847.907m travel with6 pauses. Tests
check both legs actually swing, planted feet do not slide, sole probes never penetrate or
float, full foot discs miss world obstacles, arm/leg lengths stay fixed, and movement never
teleports. They also exercise yield/resume, stall bounds and rejection of a blocked island.

The initial long test exposed a blocked body rolling back an airborne foot indefinitely.
Recovery now allows that foot to settle before the next step. The final tour has8 rejected
body microsteps which recover, rather than a frozen resident or a relaxed collision test.
The final tests pass in Release1.19s and Debug6.67s, including full foot-obstacle checks.
The independent world grid also verifies the body's clearance. Timing varies with the host;
these are CPU test durations, not game FPS measurements.

The dedicated `--viewpoint resident` run advances a fixed0.1s clock for64 updates and captures
eight ordered stages. Native llvmpipe evidence
`build-consumer/software-captures/20261012T191806.756957Z-resident` passed the isolated
kernel-monitored runner. It shows one visible actor, two bounded draws (body plus two sole
contact shadows),3638 body triangles and24 shadow triangles. The measured actor displacement
is1.979m and planted/swinging foot motion1.979m. All eight images and the final1280x720 image
were inspected; the second visual iteration reduced oversized eyes, side hair lumps, sleeve
bulk and hand/arm thickness. This is software-renderer evidence, not AMD hardware qualification.

Texture accounting now includes the complete87,380-byte resident atlas mip chain. Runtime
generation is deterministic and the material source is the project-owned algorithm in
`ResidentRenderer.cpp`. The static mesh budget is40,000 streamed vertices; the current model
uses15,270 vertices per frame in one body upload/draw, plus72 shadow vertices.

## Required next work

- Refine cloth drape and skin response beyond the visibly angular procedural pass.
- Add real player sidestepping/separation and review gait/turn transitions and performance.
- Perform consolidated native/Web/Android model, motion and lifecycle qualification.

Exactly one actor, native visibility and deterministic movement are now real game behavior.
Final artistic quality and portable runtime acceptance are not claimed.

The Emscripten Release target also compiles and links the new renderer without native-only
branches. That is only a portability build result; no browser frame, WebGL lifecycle or motion
quality claim is based on it.

## Torso, limb and portable close-view pass

A new image-driven pass addresses the most obvious mannequin silhouette without changing the
safe route, foot planting or cultural presentation. The woven upper garment now uses seven
body-profile rings and18 facets: a narrower waist expands through the ribcage and chest, slopes
from the shoulders into a compact collar, and remains genuinely sleeveless. Lower legs split at
the calf maximum instead of tapering as one straight rod. Forearms retain their exact0.26m length
but now keep a bounded relaxed elbow bend (cosine.85–.97) throughout idle and walking. The body
mesh grows only3638→3786 triangles, retains one opaque body draw plus the existing24-triangle
sole shadows, and uses the same87,380-byte generated atlas.

Two intermediate shoulder experiments were rejected after actual images: separate ellipsoids
looked like attached balls and an inward-rooted tube produced pointed shoulder wedges. Neither
remains. Final native llvmpipe evidence `20261013T065442.376759Z-resident` passes the eight ordered
stages, keeps the actor framed across1.979m body/foot motion and has an empty kernel delta. The
new focused Web harness initially exposed a fixed-camera error: the actor left the second frame.
The retained camera now follows and re-aims at the same live resident. Chrome/SwiftShader evidence
`20261013T065137.967386Z-chrome-resident` passes two distinct inspected images,60 finite intervals,
GL0, an intact context and an empty kernel delta. This is a bounded silhouette and portability
improvement, not final human realism; face, cloth drape, hands, sidestepping, continuous gait,
Android runtime and representative performance remain open.

## Layered face, split hands and covered shoulder joint

The next inspected pass addresses three close-view failures without changing movement or adding
a draw. Eyes are smaller; the nose has separate bridge, tip and paired nostrils; two shaped lip
halves and a chin replace the single line mouth. Each hand now has a smaller palm, four distinct
fingers and a thumb. The upper torso uses an eighth transition ring, animated shoulder joints sit
under the cloth shoulder line, and short open woven cap sleeves cover the unavoidable procedural
arm-root seam. A first attempt that left the brown deltoid root exposed above the tunic was visible
in WebGL and rejected before this pass was accepted.

The body is now5,090 triangles /15,270 streamed vertices, still below the40,000-vertex allocation
and still one body draw plus the existing24-triangle fitted sole-shadow draw. The generated87,380-
byte atlas and deterministic route are unchanged. Release and Debug `IslandResidentTests` pass;
the final native llvmpipe evidence `20261013T121822.689255Z-resident` passes all eight ordered gait
stages, measures1.978805m actor displacement and1.979076m foot motion, and has an empty kernel
delta. Both the final image and gait stages were inspected.

Chrome/SwiftShader evidence `20261013T122110.669273Z-chrome-resident` passes60 finite intervals,
GL0, an intact context and an empty kernel delta. Its two distinct close images were inspected and
show the same covered shoulder transition, separated fingers and layered face. This qualifies the
bounded native/Web visual change, not photorealism, continuous gait, Android runtime or hardware
performance.

## Bound neckline and moving split wrap

The next clothing-silhouette pass retains the culturally nonspecific practical woven outfit but
removes two more mannequin cues. The upper torso shoulder ring is narrower and the cap sleeves are
smaller and slope farther down the arms. Four thin cloth panels now overlap the existing fitted
walking shorts as a short front/rear wrap. Each outside lower corner follows its authoritative
thigh, while the inside corners remain near the pelvis; the centre split therefore opens visibly
during a stride without introducing a cloth simulation. A four-segment plant-fibre trim follows
the garment surface as a continuous bound V neckline.

The body grows from5,090 to5,154 triangles /15,462 streamed vertices, still one opaque draw below
the40,000-vertex allocation, plus the unchanged24-triangle sole-shadow draw. No texture, route,
collision, animation state or population changes. Native llvmpipe
`20261013T221102.938781Z-resident` passes all eight ordered stages,213 colour bins,35.47 luminance
deviation,134.29 spatial contrast and empty kernel-fault evidence. The final close frame and gait
sequence were inspected; the neckline is continuous and the hem opens with the walking pose.
Chrome/SwiftShader `20261013T221420.747958Z-chrome-resident` passes two different inspected frames,
60 finite intervals,GL0, intact context and empty kernel reports. ARM64 assembleDebug+lintDebug
passes without emulator/KVM; Release and Debug resident motion tests and88 focused Python gate
tests pass. Skin response, sidestepping, continuous gait, Android runtime and representative
hardware performance remain open.

## Shoulder, neck and facial-volume repair — 2026-10-14

The close audit still showed a cylindrical neck, abrupt shoulder corners and facial marks on a
single oval. The retained body now has a ten-ring shoulder/chest transition; each cap sleeve is
an arm-axis-aligned oval joined to a shaped biceps and two-stage forearm. A five-ring unequal-oval
trapezius/neck loft broadens below the collar and narrows under the jaw. Separate cheek pads, jaw
corners, nostril wings and upper eyelids augment the existing bridge, tip, eyes, brows, lips and
chin. One peaceful actor, the deterministic route, grounded gait, pauses/yielding and two-draw
body/shadow contract are unchanged.

Relocating the campfire back to the east inland clearing changes the generated safe tour, not its
movement rules: the current route has24 simplified points and692.043m total length, with a safe
approach around the fire and woodpile.

The current body is7122 triangles and remains under the fixed40000-vertex stream allocation.
Inspected llvmpipe evidence `20261014T004313.678113Z-resident` passes all eight ordered gait
stages at1280x720 with353 colour bins,47.65 luminance deviation and186.44 spatial contrast.
The final and stride frames show sloping shoulders, a tapered neck and the same walking actor;
the new beach route also places the resident near, but outside, the campfire/wood collisions.

## Fine broken woven-cloth atlas — 2026-10-16

A fresh current-binary eight-stage audit retained the resident's corrected sloping shoulders,
tapered neck, face volumes, split hands, moving wrap and counter-swing gait. It selected the cloth
atlas instead: bright complete lines every five and seven texels created a large square grid over
the tunic and lower wrap, reinforcing the impression of a rigid procedural mannequin.

The cloth quadrant now combines two independently wandering high-frequency warp/weft phases with
bounded irregular slub variation, low-amplitude deterministic grain and sparse fibre highlights.
No complete high-contrast line spans the material. Skin, hair, plant fibre, UV insets and mip
generation remain unchanged. This replaces pixels in the existing deterministic 128x128 atlas;
the 7,154-triangle body, 40,000-vertex stream allocation, one body draw plus fitted foot-shadow
draw, route, collision and animation are identical.

Four focused native groups and all 97 browser/capture harness tests pass. Same-stage native
baseline `20261016T105645.076574Z-resident` and accepted
`20261016T110752.908894Z-resident` were inspected side by side. The accepted 1280x720 frame removes
the graph-paper pattern while retaining a fine fabric response and passes at 293 colour bins, 42.62
luminance deviation, 158.60 spatial contrast and an empty kernel delta. Focused Chrome
155/SwiftShader evidence `build/browser-software/20261016T111259.627192Z-chrome-resident` contains
two distinct inspected live poses and passes 60 finite positive intervals, GL error zero, a live
context and kernel `[]`. Physical Android/AMD appearance and continuous all-angle gait remain part
of consolidated qualification.
