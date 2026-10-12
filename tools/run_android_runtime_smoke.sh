#!/bin/sh

set -eu

if [ "$#" -lt 2 ] || [ "$#" -gt 3 ]; then
    echo "usage: $0 <adb> <apk> [output-directory]" >&2
    exit 2
fi

ADB=$1
APK=$2
OUTPUT_DIR=${3:-build/android-runtime-smoke}
PACKAGE=com.libcna.islandsimulator
COMPONENT="$PACKAGE/.IslandSimulatorActivity"
TIMEOUT_SECONDS=${ISLAND_ANDROID_SMOKE_TIMEOUT_SECONDS:-600}
REQUIRE_AUDIO=${ISLAND_ANDROID_SMOKE_REQUIRE_AUDIO:-0}
REQUIRE_FOOTSTEPS=${ISLAND_ANDROID_SMOKE_REQUIRE_FOOTSTEPS:-0}
REQUIRE_SWIMMING=${ISLAND_ANDROID_SMOKE_REQUIRE_SWIMMING:-0}
REQUIRE_TOUCH_SENSITIVITY=${ISLAND_ANDROID_SMOKE_REQUIRE_TOUCH_SENSITIVITY:-0}
REQUIRE_LIFECYCLE=${ISLAND_ANDROID_SMOKE_REQUIRE_LIFECYCLE:-0}
REQUIRE_SETTINGS_PERSISTENCE=${ISLAND_ANDROID_SMOKE_REQUIRE_SETTINGS_PERSISTENCE:-0}
DISABLE_USER_SETTINGS=${ISLAND_ANDROID_SMOKE_DISABLE_USER_SETTINGS:-1}
REQUESTED_ARGUMENTS=${ISLAND_ANDROID_SMOKE_ARGUMENTS:-}
if [ "$DISABLE_USER_SETTINGS" = 1 ]; then
    DEBUG_ARGUMENTS="--no-user-settings${REQUESTED_ARGUMENTS:+,$REQUESTED_ARGUMENTS}"
else
    DEBUG_ARGUMENTS=$REQUESTED_ARGUMENTS
fi
DEBUG_ARGUMENTS_EXTRA="$PACKAGE.DEBUG_ARGUMENTS"
GRAPHICS_ERROR_PATTERN='FATAL EXCEPTION|island-simulator: fatal error|shader.*(failed|error)|\[CNA EasyGL 3D\].*(VS|FS|link) failed|GL error (0x0*[1-9a-fA-F][0-9a-fA-F]*|[1-9][0-9]*)'

if [ ! -x "$ADB" ]; then
    echo "android runtime smoke: adb is not executable: $ADB" >&2
    exit 2
fi
if [ ! -f "$APK" ]; then
    echo "android runtime smoke: APK does not exist: $APK" >&2
    exit 2
fi
case "$TIMEOUT_SECONDS" in
    ''|*[!0-9]*)
        echo "android runtime smoke: timeout must be a positive integer" >&2
        exit 2
        ;;
esac
if [ "$TIMEOUT_SECONDS" -eq 0 ]; then
    echo "android runtime smoke: timeout must be greater than zero" >&2
    exit 2
fi
case "$REQUIRE_AUDIO" in
    0|1) ;;
    *)
        echo "android runtime smoke: ISLAND_ANDROID_SMOKE_REQUIRE_AUDIO must be 0 or 1" >&2
        exit 2
        ;;
esac
case "$REQUIRE_FOOTSTEPS" in
    0|1) ;;
    *)
        echo "android runtime smoke: ISLAND_ANDROID_SMOKE_REQUIRE_FOOTSTEPS must be 0 or 1" >&2
        exit 2
        ;;
esac
case "$REQUIRE_SWIMMING" in
    0|1) ;;
    *)
        echo "android runtime smoke: ISLAND_ANDROID_SMOKE_REQUIRE_SWIMMING must be 0 or 1" >&2
        exit 2
        ;;
esac
case "$REQUIRE_TOUCH_SENSITIVITY" in
    0|1) ;;
    *)
        echo "android runtime smoke: ISLAND_ANDROID_SMOKE_REQUIRE_TOUCH_SENSITIVITY must be 0 or 1" >&2
        exit 2
        ;;
esac
case "$REQUIRE_LIFECYCLE" in
    0|1) ;;
    *)
        echo "android runtime smoke: ISLAND_ANDROID_SMOKE_REQUIRE_LIFECYCLE must be 0 or 1" >&2
        exit 2
        ;;
esac
case "$REQUIRE_SETTINGS_PERSISTENCE" in
    0|1) ;;
    *)
        echo "android runtime smoke: ISLAND_ANDROID_SMOKE_REQUIRE_SETTINGS_PERSISTENCE must be 0 or 1" >&2
        exit 2
        ;;
esac
case "$DISABLE_USER_SETTINGS" in
    0|1) ;;
    *)
        echo "android runtime smoke: ISLAND_ANDROID_SMOKE_DISABLE_USER_SETTINGS must be 0 or 1" >&2
        exit 2
        ;;
esac
if [ "$REQUIRE_SETTINGS_PERSISTENCE" -eq 1 ] \
    && { [ "$DISABLE_USER_SETTINGS" -ne 0 ] \
         || [ "$REQUIRE_TOUCH_SENSITIVITY" -ne 1 ] \
         || [ "$REQUIRE_LIFECYCLE" -ne 1 ]; }; then
    echo "android runtime smoke: settings persistence requires enabled user settings, touch sensitivity, and lifecycle gates" >&2
    exit 2
fi
if [ "$REQUIRE_SWIMMING" -eq 1 ] || [ "$REQUIRE_FOOTSTEPS" -eq 1 ] \
    || [ "$REQUIRE_TOUCH_SENSITIVITY" -eq 1 ]; then
    case ",$DEBUG_ARGUMENTS," in
        *,--overview,*|*,--viewpoint,*|*,--smoke-test,*|*,--capture,*|*,--benchmark,*)
            echo "android runtime smoke: touch checks require the interactive south-beach spawn" >&2
            exit 2
            ;;
    esac
fi

if ! "$ADB" get-state 2>/dev/null | grep -qx device; then
    echo "android runtime smoke: no selected ADB device is ready" >&2
    exit 2
fi
BOOT_COMPLETED=$("$ADB" shell getprop sys.boot_completed 2>/dev/null | tr -d '\r')
if [ "$BOOT_COMPLETED" != "1" ]; then
    echo "android runtime smoke: selected device has not completed boot" >&2
    exit 2
fi

mkdir -p "$OUTPUT_DIR"
LOG_FILE="$OUTPUT_DIR/island-android-logcat.txt"
LAUNCH_FILE="$OUTPUT_DIR/island-android-launch.txt"
SCREENSHOT="$OUTPUT_DIR/island-android.png"
HOT_RESUME_LOG_FILE="$OUTPUT_DIR/island-android-hot-resume-logcat.txt"
HOT_RESUME_LAUNCH_FILE="$OUTPUT_DIR/island-android-hot-resume-launch.txt"
HOT_RESUME_SCREENSHOT="$OUTPUT_DIR/island-android-hot-resume.png"
COLD_RELAUNCH_LOG_FILE="$OUTPUT_DIR/island-android-cold-relaunch-logcat.txt"
COLD_RELAUNCH_LAUNCH_FILE="$OUTPUT_DIR/island-android-cold-relaunch-launch.txt"
COLD_RELAUNCH_SCREENSHOT="$OUTPUT_DIR/island-android-cold-relaunch.png"
: >"$LOG_FILE"

launch_game()
{
    launch_output=$1
    if [ -n "$DEBUG_ARGUMENTS" ]; then
        "$ADB" shell am start -W -n "$COMPONENT" \
            --esa "$DEBUG_ARGUMENTS_EXTRA" "$DEBUG_ARGUMENTS" >"$launch_output"
    else
        "$ADB" shell am start -W -n "$COMPONENT" >"$launch_output"
    fi
}

echo "android runtime smoke: installing $APK"
"$ADB" install -r "$APK"
"$ADB" logcat -c
"$ADB" shell am force-stop "$PACKAGE"
echo "android runtime smoke: launching $COMPONENT"
if [ -n "$DEBUG_ARGUMENTS" ]; then
    echo "android runtime smoke: debug arguments $DEBUG_ARGUMENTS"
fi
launch_game "$LAUNCH_FILE"

DEADLINE=$(( $(date +%s) + TIMEOUT_SECONDS ))
GAME_PID=
QUALIFIED=0
while [ "$(date +%s)" -lt "$DEADLINE" ]; do
    CURRENT_PID=$("$ADB" shell pidof "$PACKAGE" 2>/dev/null | tr -d '\r' | cut -d ' ' -f 1)
    if [ -z "$CURRENT_PID" ]; then
        if [ -n "$GAME_PID" ]; then
            echo "android runtime smoke: game process exited before the first qualified frame" >&2
            tail -n 120 "$LOG_FILE" >&2
            exit 1
        fi
        sleep 1
        continue
    fi
    if [ -n "$GAME_PID" ] && [ "$CURRENT_PID" != "$GAME_PID" ]; then
        echo "android runtime smoke: game process restarted before the first qualified frame" >&2
        tail -n 120 "$LOG_FILE" >&2
        exit 1
    fi
    if [ -z "$GAME_PID" ]; then
        GAME_PID=$CURRENT_PID
        echo "android runtime smoke: observing game process $GAME_PID"
    fi

    # Filtering at the device avoids transferring the entire system log from slow software AVDs.
    "$ADB" logcat --pid="$GAME_PID" -d -v threadtime >"$LOG_FILE"
    AUDIO_QUALIFIED=1
    if [ "$REQUIRE_AUDIO" -eq 1 ] \
        && ! grep -q "island-simulator: audio sources use sequential generation and upload with bounded staging memory" \
            "$LOG_FILE"; then
        AUDIO_QUALIFIED=0
    fi
    if [ "$REQUIRE_AUDIO" -eq 1 ] \
        && ! grep -q "island-simulator: ambient beds use slow non-amplifying gain drift" \
            "$LOG_FILE"; then
        AUDIO_QUALIFIED=0
    fi
    if [ "$REQUIRE_AUDIO" -eq 1 ] \
        && ! grep -q "island-simulator: persistent ambient PCM is DC-centered before quantization" \
            "$LOG_FILE"; then
        AUDIO_QUALIFIED=0
    fi
    if [ "$REQUIRE_FOOTSTEPS" -eq 1 ] \
        && ! grep -q "island-simulator: distance-scheduled sand and soil footsteps enabled" \
            "$LOG_FILE"; then
        AUDIO_QUALIFIED=0
    fi
    if grep -q "island-simulator: renderer OPENGLES3" "$LOG_FILE" \
        && grep -q "island-simulator: quality profile mobile grass 112m shadows 72% wildlife 82% ocean 128x128 textures 8846468/8866684 bytes audio 6/6 sources" \
            "$LOG_FILE" \
        && grep -q "island-simulator: viewport 1280x720 logical backbuffer" \
            "$LOG_FILE" \
        && grep -q "island-simulator: startup world" "$LOG_FILE" \
        && grep -q "island-simulator: grass biome tint follows coast, path, and authored clearings" \
            "$LOG_FILE" \
        && grep -q "island-simulator: grass near clusters use 42 blades for denser first-person cover" \
            "$LOG_FILE" \
        && grep -q "island-simulator: selected near grass blades use a geometric V-fold ridge" \
            "$LOG_FILE" \
        && grep -q "island-simulator: grass near blades use asymmetric edges and occasional bent tips" \
            "$LOG_FILE" \
        && grep -q "island-simulator: grass shared rest-shape normals follow curvature and instance scale" \
            "$LOG_FILE" \
        && grep -q "island-simulator: grass progressive near taper and seed roots share the sampled blade profile" \
            "$LOG_FILE" \
        && grep -q "island-simulator: grass near blades form seven unequal fanned tufts with coherent wind" \
            "$LOG_FILE" \
        && grep -q "island-simulator: grass near mesh adds 48 low curved turf blades with height-scaled motion" \
            "$LOG_FILE" \
        && grep -q "island-simulator: grass roots sample RGB24 rendered terrain triangles before wind" \
            "$LOG_FILE" \
        && grep -q "island-simulator: grass culling bounds cover measured meshes, slope and full shared motion limits" \
            "$LOG_FILE" \
        && grep -q "island-simulator: all near driftwood variants use tapered three-dimensional branches" \
            "$LOG_FILE" \
        && grep -q "island-simulator: driftwood end caps match irregular body rings" \
            "$LOG_FILE" \
        && grep -q "island-simulator: shipwreck portable aged wood, waterline, and salt-wear material" \
            "$LOG_FILE" \
        && grep -q "island-simulator: shipwreck 20m broken hull, 12 debris pieces, one portable culled batch" \
            "$LOG_FILE" \
        && grep -q "island-simulator: shipwreck broken hull exposes ribs, partial deck, and snapped mast" \
            "$LOG_FILE" \
        && grep -q "island-simulator: shipwreck large-ship silhouette uses deck rails, hull openings, keel, rudder, and a broken foremast" \
            "$LOG_FILE" \
        && grep -q "island-simulator: beach crabs 30 agents in deterministic sand-constrained colonies" \
            "$LOG_FILE" \
        && grep -q "island-simulator: beach crabs use articulated legs, claws, and one culled batch" \
            "$LOG_FILE" \
        && grep -q "island-simulator: clearing butterflies 18 agents in deterministic low-altitude habitats" \
            "$LOG_FILE" \
        && grep -q "island-simulator: clearing butterflies use hinged patterned wings in one culled batch" \
            "$LOG_FILE" \
        && grep -q "island-simulator: sea turtles 5 agents in bounded near-shore cruise, forage, and surface routes" \
            "$LOG_FILE" \
        && grep -q "island-simulator: sea turtles use faceted shells and articulated flippers in one culled batch" \
            "$LOG_FILE" \
        && grep -q "island-simulator: clearing floors reuse existing noise fields for broad patch breakup" \
            "$LOG_FILE" \
        && grep -q "island-simulator: terrain interior humus and moss reuse portable material fields" \
            "$LOG_FILE" \
        && grep -q "island-simulator: forest soil separates humus and moss with footprint-filtered fallen leaves" \
            "$LOG_FILE" \
        && grep -q "island-simulator: terrain substrate persists at distance with footprint-filtered noise" \
            "$LOG_FILE" \
        && grep -q "island-simulator: terrain palette overrides preserve smooth slope daylight; opaque 28-byte vertices" \
            "$LOG_FILE" \
        && grep -q "forest-floor material loaded Content/terrain/forest-floor.png; 512x512, 10 mips, 1398100 RGBA8 bytes; interior windows" \
            "$LOG_FILE" \
        && grep -q "leaf material loaded Content/vegetation/leaf-lamina.png; 256x256, 9 mips, 349524 RGBA8 bytes" \
            "$LOG_FILE" \
        && grep -q "monkey fur loaded Content/wildlife/monkey-fur.png; 512x512, 10 mips, 1398100 RGBA8 bytes" \
            "$LOG_FILE" \
        && grep -Eq "forest-floor filter (anisotropic, requested maximum 4x, device-clamped|trilinear, anisotropy unavailable)" \
            "$LOG_FILE" \
        && grep -q "island-simulator: grass retains foliage colour at distance with normalized-height basal tint" \
            "$LOG_FILE" \
        && grep -q "island-simulator: grass pigment uses separate height occlusion and filtered longitudinal veins" \
            "$LOG_FILE" \
        && grep -q "island-simulator: grass LOD proxies share seven tuft roots, profiles and wind phases" \
            "$LOG_FILE" \
        && grep -q "island-simulator: grass anchor-distance contraction uses nearest-bound cell culling" \
            "$LOG_FILE" \
        && grep -q "island-simulator: juvenile ground broadleaf uses curved tagged laminae and retained near/far leaf identities" \
            "$LOG_FILE" \
        && grep -q "island-simulator: split ground leaves use connected lobed surfaces with shared LOD knots and root-anchored wind" \
            "$LOG_FILE" \
        && grep -q "island-simulator: shrubs use supported shoots and curved individual leaves with shared LOD identities and complete leaf outlines" \
            "$LOG_FILE" \
        && grep -q "island-simulator: grass mesh vertices/triangles near=862/640 middle=60/40 far=42/28" \
            "$LOG_FILE" \
        && grep -q "island-simulator: canopy per-tree frustum boxes contain both LODs and wind" \
            "$LOG_FILE" \
        && grep -q "island-simulator: clearing-edge colonies respect full route footprints; bushes=" \
            "$LOG_FILE" \
        && grep -q "island-simulator: forest-floor colonies retain route footprints; groups=" \
            "$LOG_FILE" \
        && grep -q "island-simulator: jungle root origins and culling bounds follow rendered terrain triangles" \
            "$LOG_FILE" \
        && grep -q "island-simulator: fallen leaves conform to rendered triangles using the shared grass height texture" \
            "$LOG_FILE" \
        && grep -q "island-simulator: understory leaf identities share asymmetric posture and curled profiles across LOD" \
            "$LOG_FILE" \
        && grep -q "island-simulator: pinnate understory shares rachis attachment knots and retained leaflet wind across LOD" \
            "$LOG_FILE" \
        && grep -q "island-simulator: edge antialiasing requested 2x renderer-applied 2x" \
            "$LOG_FILE" \
        && grep -q "island-simulator: broadleaf paddle and lance laminae share cupped profiles and asymmetric margins across LOD" \
            "$LOG_FILE" \
        && grep -q "island-simulator: leaf daylight contrast and stable tip pigment; footprint-filtered leaf and bark detail" \
            "$LOG_FILE" \
        && grep -q "island-simulator: canopy wood separates albedo from baked shade with validated outward normals and one-sided daylight" \
            "$LOG_FILE" \
        && grep -q "island-simulator: canopy bark follows local wood axes with seam-safe circular footprint filtering" \
            "$LOG_FILE" \
        && grep -q "island-simulator: canopy bark has instance-stable warped grain and interrupted filtered fissures" \
            "$LOG_FILE" \
        && grep -q "island-simulator: canopy trunk pigment follows normalized height across LOD tiers" \
            "$LOG_FILE" \
        && grep -q "island-simulator: canopy blades use spatial pitch and roll with identical retained LOD poses" \
            "$LOG_FILE" \
        && grep -q "island-simulator: curved canopy blades retain source identities within original triangle budgets" \
            "$LOG_FILE" \
        && grep -q "canopy mesh variant=0 near-vertices=4818 near-triangles=4640 far-vertices=1071 far-triangles=1120" \
            "$LOG_FILE" \
        && grep -q "island-simulator: eight structural crown forms share eight LOD lobe anchors and supported trunk bends" \
            "$LOG_FILE" \
        && grep -q "island-simulator: jungle per-instance foliage tint reuses existing transforms" \
            "$LOG_FILE" \
        && grep -q "island-simulator: jungle near canopy uses irregular layered lower-crown edges" \
            "$LOG_FILE" \
        && grep -q "island-simulator: jungle near canopy uses irregular trunks and bent support branches" \
            "$LOG_FILE" \
        && grep -q "island-simulator: jungle ground layers receive shared canopy shade; crowns excluded" \
            "$LOG_FILE" \
        && grep -q "island-simulator: boars receive bilinear per-vertex canopy light in their existing batch" \
            "$LOG_FILE" \
        && grep -q "island-simulator: boar directional coat atlas 128x128, opaque stock BasicEffect" \
            "$LOG_FILE" \
        && grep -q "island-simulator: jungle leaves use smooth normals, filtered veins and restrained wax shading" \
            "$LOG_FILE" \
        && grep -q "island-simulator: boars use connected limb poses, stance contact and body-rooted bristles" \
            "$LOG_FILE" \
        && grep -q "island-simulator: boar soles fit rendered terrain independently with fixed-length limbs" \
            "$LOG_FILE" \
        && ! grep -q "boar terrain contact limited by limb reach" "$LOG_FILE" \
        && grep -q "island-simulator: monkeys receive footprint-averaged canopy light through 14-bone skinning" \
            "$LOG_FILE" \
        && grep -q "island-simulator: selected near canopy variants include hanging leafy lianas" \
            "$LOG_FILE" \
        && grep -q "island-simulator: palm per-instance frond tint reuses existing transforms" \
            "$LOG_FILE" \
        && grep -q "island-simulator: palm near leaflets use staggered asymmetric silhouettes" \
            "$LOG_FILE" \
        && grep -q "island-simulator: palm near geometry uses fixed-sun facet shading" \
            "$LOG_FILE" \
        && grep -q "island-simulator: selected palm near crowns include sparse hanging dry fronds" \
            "$LOG_FILE" \
        && grep -q "island-simulator: near fallen logs use irregular broken end contours" \
            "$LOG_FILE" \
        && grep -q "island-simulator: near fallen-log end grain uses restrained radial cracks" \
            "$LOG_FILE" \
        && grep -q "island-simulator: grass sparse near seed heads share blade wind and instancing" \
            "$LOG_FILE" \
        && grep -q "island-simulator: rocks use shared smooth normals, scale-correct daylight and filtered mineral weathering" \
            "$LOG_FILE" \
        && grep -q "island-simulator: rock per-instance tint reuses existing transforms" \
            "$LOG_FILE" \
        && grep -q "island-simulator: near birds use layered trailing flight-feather silhouettes" \
            "$LOG_FILE" \
        && grep -q "island-simulator: fallen fronds use curved asymmetric leaflet silhouettes" \
            "$LOG_FILE" \
        && grep -q "island-simulator: fallen frond leaflets use geometric central folds" \
            "$LOG_FILE" \
        && grep -q "island-simulator: ocean sunward ripple crests reuse analytic slope field" \
            "$LOG_FILE" \
        && grep -q "island-simulator: ocean grazing reflection bands reuse analytic slope field" \
            "$LOG_FILE" \
        && grep -q "island-simulator: shoreline advancing wash uses footprint-filtered porous foam" \
            "$LOG_FILE" \
        && grep -q "island-simulator: ocean shared nine-wave spectrum uses filtered short-wave normals" \
            "$LOG_FILE" \
        && grep -q "island-simulator: ocean shallow attenuation uses interpolated bed depth and contact fade" \
            "$LOG_FILE" \
        && grep -q "island-simulator: coastal sand stays continuous below water with filtered seabed ripples" \
            "$LOG_FILE" \
        && grep -q "island-simulator: coastal contact patches follow rendered seabed triangles in the existing batch" \
            "$LOG_FILE" \
        && grep -q "island-simulator: land prop shadows match model yaw and rendered terrain triangles" \
            "$LOG_FILE" \
        && grep -q "island-simulator: wood uses outward shared normals and scale-correct fixed-sun lighting" \
            "$LOG_FILE" \
        && grep -q "island-simulator: ocean bounded grazing contrast and depth-limited filtered breaker foam" \
            "$LOG_FILE" \
        && grep -q "island-simulator: ocean short waves use phased wind packets with analytic envelope gradients" \
            "$LOG_FILE" \
        && { [ "${ISLAND_ANDROID_SMOKE_ARGUMENTS:-}" != "--viewpoint,monkey-climb" ] \
             || grep -q "monkey climb held summit presented after eight cycle stages" "$LOG_FILE"; } \
        && { case "${ISLAND_ANDROID_SMOKE_ARGUMENTS:-}" in
                 --viewpoint,snakes|--viewpoint,snake-head)
                     grep -q "snake runtime sample frame=32 " "$LOG_FILE" ;;
                 --viewpoint,goats)
                     grep -Eq "goat render proof: goats [1-6]/6 visible, 2 draw;" "$LOG_FILE" ;;
                 --viewpoint,spiders)
                     grep -Eq "spider render proof: spiders ([1-9]|1[0-2])/12 visible, 2 draw; contact-shadows=([1-9]|1[0-2])([[:space:]]|$)" "$LOG_FILE" ;;
                 *) true ;;
             esac; } \
        && [ "$AUDIO_QUALIFIED" -eq 1 ]; then
        QUALIFIED=1
        break
    fi
    sleep 1
done

if [ "$QUALIFIED" -ne 1 ]; then
    echo "android runtime smoke: timed out after $TIMEOUT_SECONDS seconds waiting for the qualified GLES 3 frame/audio state" >&2
    tail -n 120 "$LOG_FILE" >&2
    exit 1
fi

if grep -Eiq "$GRAPHICS_ERROR_PATTERN" "$LOG_FILE"; then
    echo "android runtime smoke: fatal or shader error appeared in logcat" >&2
    tail -n 120 "$LOG_FILE" >&2
    exit 1
fi

if [ "$REQUIRE_TOUCH_SENSITIVITY" -eq 1 ] || [ "$REQUIRE_FOOTSTEPS" -eq 1 ] \
    || [ "$REQUIRE_SWIMMING" -eq 1 ]; then
    SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
    TOUCH_GEOMETRY=$(python3 "$SCRIPT_DIR/android_touch_geometry.py" "$LOG_FILE")
    read -r SELECTOR_X SELECTOR_Y MOVE_X MOVE_LOW_Y MOVE_HIGH_Y MOVE_BOTTOM_Y <<EOF
$TOUCH_GEOMETRY
EOF
    echo "android runtime smoke: mapped touch coordinates $TOUCH_GEOMETRY"
fi

if [ "$REQUIRE_TOUCH_SENSITIVITY" -eq 1 ]; then
    echo "android runtime smoke: exercising touch look-sensitivity selector"
    # ADB's instantaneous tap can press and release between game frames. A stationary bounded
    # swipe keeps the contact down long enough to exercise the real Pressed ownership path.
    SENSITIVITY_QUALIFIED=0
    for _sensitivity_attempt in 1 2 3; do
        "$ADB" shell input swipe "$SELECTOR_X" "$SELECTOR_Y" \
            "$SELECTOR_X" "$SELECTOR_Y" 500
        SENSITIVITY_DEADLINE=$(( $(date +%s) + 10 ))
        while [ "$(date +%s)" -lt "$SENSITIVITY_DEADLINE" ]; do
            "$ADB" logcat --pid="$GAME_PID" -d -v threadtime >"$LOG_FILE"
            SETTINGS_WRITE_QUALIFIED=1
            if [ "$REQUIRE_SETTINGS_PERSISTENCE" -eq 1 ] \
                && ! grep -q "island-simulator: user settings saved" "$LOG_FILE"; then
                SETTINGS_WRITE_QUALIFIED=0
            fi
            if grep -q "island-simulator: touch look sensitivity high (1.35x)" "$LOG_FILE" \
                && grep -q "island-simulator: touch sensitivity selector retained touch ownership without mouse capture" \
                    "$LOG_FILE" \
                && [ "$SETTINGS_WRITE_QUALIFIED" -eq 1 ]; then
                SENSITIVITY_QUALIFIED=1
                break
            fi
            sleep 1
        done
        [ "$SENSITIVITY_QUALIFIED" -eq 1 ] && break
    done
    if [ "$SENSITIVITY_QUALIFIED" -ne 1 ]; then
        echo "android runtime smoke: touch sensitivity selector did not choose high" >&2
        tail -n 120 "$LOG_FILE" >&2
        exit 1
    fi
fi

if [ "$REQUIRE_FOOTSTEPS" -eq 1 ]; then
    echo "android runtime smoke: exercising distance-scheduled land footsteps"
    # Sustain a drag in the aspect-fitted left movement zone, independent of device resolution.
    "$ADB" shell input swipe "$MOVE_X" "$MOVE_LOW_Y" "$MOVE_X" "$MOVE_HIGH_Y" 3000
    FOOTSTEP_DEADLINE=$(( $(date +%s) + 15 ))
    FOOTSTEP_QUALIFIED=0
    while [ "$(date +%s)" -lt "$FOOTSTEP_DEADLINE" ]; do
        "$ADB" logcat --pid="$GAME_PID" -d -v threadtime >"$LOG_FILE"
        if grep -q "island-simulator: first distance-scheduled land footstep played" \
            "$LOG_FILE"; then
            FOOTSTEP_QUALIFIED=1
            break
        fi
        sleep 1
    done
    if [ "$FOOTSTEP_QUALIFIED" -ne 1 ]; then
        echo "android runtime smoke: touch traversal did not play a land footstep" >&2
        tail -n 120 "$LOG_FILE" >&2
        exit 1
    fi
fi

if [ "$REQUIRE_SWIMMING" -eq 1 ]; then
    echo "android runtime smoke: exercising shoreline entry with touch traversal"
    # The interactive spawn faces inland from the south beach. A long downward drag in the left
    # movement zone therefore walks backward through the shallows and continues as swimming.
    # SwiftShader frames can exceed 0.7 s while PlayerMotion caps simulation steps at 0.1 s.
    # One fixed-duration swipe then covers too little simulated distance; an idle wait cannot
    # complete that movement. Repeat actual touch gestures within a bounded deadline, retaining
    # the same swimming/audio evidence gates (a blocked route must still fail).
    SWIMMING_DEADLINE=$(( $(date +%s) + 100 ))
    SWIMMING_QUALIFIED=0
    SWIMMING_GESTURES=0
    while [ "$(date +%s)" -lt "$SWIMMING_DEADLINE" ]; do
        "$ADB" shell input swipe "$MOVE_X" "$MOVE_HIGH_Y" "$MOVE_X" "$MOVE_BOTTOM_Y" 9000
        SWIMMING_GESTURES=$((SWIMMING_GESTURES + 1))
        "$ADB" logcat --pid="$GAME_PID" -d -v threadtime >"$LOG_FILE"
        WATER_AUDIO_QUALIFIED=1
        if [ "$REQUIRE_AUDIO" -eq 1 ] \
            && ! grep -q "island-simulator: first water-transition accent played" \
                "$LOG_FILE"; then
            WATER_AUDIO_QUALIFIED=0
        fi
        if grep -q "island-simulator: first traversal entered swimming state" "$LOG_FILE" \
            && [ "$WATER_AUDIO_QUALIFIED" -eq 1 ]; then
            SWIMMING_QUALIFIED=1
            break
        fi
        sleep 1
    done
    echo "android runtime smoke: shoreline gestures $SWIMMING_GESTURES, swimming qualified $SWIMMING_QUALIFIED"
    if [ "$SWIMMING_QUALIFIED" -ne 1 ]; then
        echo "android runtime smoke: touch traversal did not enter the swimming state" >&2
        tail -n 120 "$LOG_FILE" >&2
        exit 1
    fi
fi

if grep -Eiq "$GRAPHICS_ERROR_PATTERN" "$LOG_FILE"; then
    echo "android runtime smoke: fatal or shader error appeared after traversal" >&2
    tail -n 120 "$LOG_FILE" >&2
    exit 1
fi

CAPTURE_SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
python3 "$CAPTURE_SCRIPT_DIR/android_capture.py" "$ADB" "$SCREENSHOT" "$DEBUG_ARGUMENTS"
if [ ! -s "$SCREENSHOT" ]; then
    echo "android runtime smoke: device screenshot is empty" >&2
    exit 1
fi

if [ "$REQUIRE_LIFECYCLE" -eq 1 ]; then
    echo "android runtime smoke: exercising same-process Home/resume"
    LIFECYCLE_PID=$GAME_PID
    "$ADB" logcat -c
    "$ADB" shell input keyevent KEYCODE_HOME
    sleep 2
    launch_game "$HOT_RESUME_LAUNCH_FILE"
    HOT_RESUME_DEADLINE=$(( $(date +%s) + 20 ))
    HOT_RESUME_QUALIFIED=0
    while [ "$(date +%s)" -lt "$HOT_RESUME_DEADLINE" ]; do
        HOT_PID=$("$ADB" shell pidof "$PACKAGE" 2>/dev/null | tr -d '\r' | cut -d ' ' -f 1)
        "$ADB" logcat --pid="$LIFECYCLE_PID" -d -v threadtime >"$HOT_RESUME_LOG_FILE"
        HOT_AUDIO_QUALIFIED=1
        if [ "$REQUIRE_AUDIO" -eq 1 ] \
            && { ! grep -q "AAudioStream_requestPause" "$HOT_RESUME_LOG_FILE" \
                 || ! grep -q "AAudioStream_requestStart" "$HOT_RESUME_LOG_FILE"; }; then
            HOT_AUDIO_QUALIFIED=0
        fi
        if [ "$HOT_PID" = "$LIFECYCLE_PID" ] \
            && grep -q "onPause()" "$HOT_RESUME_LOG_FILE" \
            && grep -q "surfaceDestroyed()" "$HOT_RESUME_LOG_FILE" \
            && grep -q "onResume()" "$HOT_RESUME_LOG_FILE" \
            && grep -q "surfaceCreated()" "$HOT_RESUME_LOG_FILE" \
            && grep -Eq "Window size: [1-9][0-9]*x[1-9][0-9]*" "$HOT_RESUME_LOG_FILE" \
            && grep -q "onWindowFocusChanged(): true" "$HOT_RESUME_LOG_FILE" \
            && [ "$HOT_AUDIO_QUALIFIED" -eq 1 ]; then
            HOT_RESUME_QUALIFIED=1
            break
        fi
        sleep 1
    done
    if [ "$HOT_RESUME_QUALIFIED" -ne 1 ]; then
        echo "android runtime smoke: same-process Home/resume qualification failed (process/surface/focus/audio); audio qualified $HOT_AUDIO_QUALIFIED, original PID $LIFECYCLE_PID, current PID $HOT_PID" >&2
        tail -n 120 "$HOT_RESUME_LOG_FILE" >&2
        exit 1
    fi
    # Android can report the recreated landscape surface before system screenshot orientation has
    # caught up. Allow presentation to settle, then confirm the resumed process is still alive.
    sleep 2
    HOT_PID=$("$ADB" shell pidof "$PACKAGE" 2>/dev/null | tr -d '\r' | cut -d ' ' -f 1)
    "$ADB" logcat --pid="$LIFECYCLE_PID" -d -v threadtime >"$HOT_RESUME_LOG_FILE"
    if [ "$HOT_PID" != "$LIFECYCLE_PID" ]; then
        echo "android runtime smoke: process changed while the resumed surface was settling" >&2
        tail -n 120 "$HOT_RESUME_LOG_FILE" >&2
        exit 1
    fi
    if grep -Eiq "$GRAPHICS_ERROR_PATTERN" \
        "$HOT_RESUME_LOG_FILE"; then
        echo "android runtime smoke: fatal or shader error appeared during hot resume" >&2
        tail -n 120 "$HOT_RESUME_LOG_FILE" >&2
        exit 1
    fi
    python3 "$CAPTURE_SCRIPT_DIR/android_capture.py" "$ADB" "$HOT_RESUME_SCREENSHOT" "$DEBUG_ARGUMENTS"
    if [ ! -s "$HOT_RESUME_SCREENSHOT" ]; then
        echo "android runtime smoke: hot-resume screenshot is empty" >&2
        exit 1
    fi

    echo "android runtime smoke: exercising force-stop/cold relaunch"
    "$ADB" logcat -c
    "$ADB" shell am force-stop "$PACKAGE"
    PROCESS_STOPPED=0
    for _attempt in 1 2 3 4 5 6 7 8 9 10; do
        STOPPED_PID=$("$ADB" shell pidof "$PACKAGE" 2>/dev/null | tr -d '\r' | cut -d ' ' -f 1)
        if [ -z "$STOPPED_PID" ]; then
            PROCESS_STOPPED=1
            break
        fi
        sleep 1
    done
    if [ "$PROCESS_STOPPED" -ne 1 ]; then
        echo "android runtime smoke: force-stop did not terminate the original process" >&2
        exit 1
    fi
    launch_game "$COLD_RELAUNCH_LAUNCH_FILE"
    COLD_RELAUNCH_DEADLINE=$(( $(date +%s) + TIMEOUT_SECONDS ))
    COLD_RELAUNCH_QUALIFIED=0
    COLD_PID=
    while [ "$(date +%s)" -lt "$COLD_RELAUNCH_DEADLINE" ]; do
        COLD_PID=$("$ADB" shell pidof "$PACKAGE" 2>/dev/null | tr -d '\r' | cut -d ' ' -f 1)
        if [ -z "$COLD_PID" ] || [ "$COLD_PID" = "$LIFECYCLE_PID" ]; then
            sleep 1
            continue
        fi
        "$ADB" logcat --pid="$COLD_PID" -d -v threadtime >"$COLD_RELAUNCH_LOG_FILE"
        COLD_AUDIO_QUALIFIED=1
        COLD_SETTINGS_QUALIFIED=1
        if [ "$REQUIRE_AUDIO" -eq 1 ] \
            && ! grep -q "island-simulator: audio sources use sequential generation and upload with bounded staging memory" \
                "$COLD_RELAUNCH_LOG_FILE"; then
            COLD_AUDIO_QUALIFIED=0
        fi
        if [ "$REQUIRE_SETTINGS_PERSISTENCE" -eq 1 ] \
            && ! grep -q "island-simulator: user settings loaded run disabled camera motion enabled audio enabled touch high" \
                "$COLD_RELAUNCH_LOG_FILE"; then
            COLD_SETTINGS_QUALIFIED=0
        fi
        if [ "$REQUIRE_AUDIO" -eq 1 ] \
            && ! grep -q "island-simulator: ambient beds use slow non-amplifying gain drift" \
                "$COLD_RELAUNCH_LOG_FILE"; then
            COLD_AUDIO_QUALIFIED=0
        fi
        if [ "$REQUIRE_AUDIO" -eq 1 ] \
            && ! grep -q "island-simulator: persistent ambient PCM is DC-centered before quantization" \
                "$COLD_RELAUNCH_LOG_FILE"; then
            COLD_AUDIO_QUALIFIED=0
        fi
        if grep -q "island-simulator: renderer OPENGLES3" "$COLD_RELAUNCH_LOG_FILE" \
            && grep -q "island-simulator: viewport 1280x720 logical backbuffer" \
                "$COLD_RELAUNCH_LOG_FILE" \
            && grep -q "island-simulator: startup world" "$COLD_RELAUNCH_LOG_FILE" \
            && [ "$COLD_AUDIO_QUALIFIED" -eq 1 ] \
            && [ "$COLD_SETTINGS_QUALIFIED" -eq 1 ]; then
            COLD_RELAUNCH_QUALIFIED=1
            break
        fi
        sleep 1
    done
    if [ "$COLD_RELAUNCH_QUALIFIED" -ne 1 ]; then
        echo "android runtime smoke: cold relaunch did not reach a new qualified GLES 3 frame" >&2
        tail -n 120 "$COLD_RELAUNCH_LOG_FILE" >&2
        exit 1
    fi
    if grep -Eiq "$GRAPHICS_ERROR_PATTERN" \
        "$COLD_RELAUNCH_LOG_FILE"; then
        echo "android runtime smoke: fatal or shader error appeared during cold relaunch" >&2
        tail -n 120 "$COLD_RELAUNCH_LOG_FILE" >&2
        exit 1
    fi
    python3 "$CAPTURE_SCRIPT_DIR/android_capture.py" "$ADB" "$COLD_RELAUNCH_SCREENSHOT" "$DEBUG_ARGUMENTS"
    if [ ! -s "$COLD_RELAUNCH_SCREENSHOT" ]; then
        echo "android runtime smoke: cold-relaunch screenshot is empty" >&2
        exit 1
    fi
    echo "android runtime smoke: lifecycle passed (hot PID $LIFECYCLE_PID, cold PID $COLD_PID)"
    echo "android runtime smoke: hot-resume artifacts (log: $HOT_RESUME_LOG_FILE, screenshot: $HOT_RESUME_SCREENSHOT)"
    echo "android runtime smoke: cold-relaunch artifacts (log: $COLD_RELAUNCH_LOG_FILE, screenshot: $COLD_RELAUNCH_SCREENSHOT)"
fi

echo "android runtime smoke passed (log: $LOG_FILE, screenshot: $SCREENSHOT)"
