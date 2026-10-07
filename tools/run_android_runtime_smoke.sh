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
DEBUG_ARGUMENTS=${ISLAND_ANDROID_SMOKE_ARGUMENTS:-}
DEBUG_ARGUMENTS_EXTRA="$PACKAGE.DEBUG_ARGUMENTS"

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
if [ "$REQUIRE_SWIMMING" -eq 1 ]; then
    case ",$DEBUG_ARGUMENTS," in
        *,--overview,*|*,--viewpoint,*|*,--smoke-test,*|*,--capture,*|*,--benchmark,*)
            echo "android runtime smoke: swimming traversal requires the interactive south-beach spawn" >&2
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
        && grep -q "island-simulator: viewport 1280x720 logical backbuffer" \
            "$LOG_FILE" \
        && grep -q "island-simulator: startup world" "$LOG_FILE" \
        && grep -q "island-simulator: grass biome tint follows coast, path, and authored clearings" \
            "$LOG_FILE" \
        && grep -q "island-simulator: selected near grass blades use a geometric V-fold ridge" \
            "$LOG_FILE" \
        && grep -q "island-simulator: all near driftwood variants use tapered three-dimensional branches" \
            "$LOG_FILE" \
        && grep -q "island-simulator: driftwood end caps match irregular body rings" \
            "$LOG_FILE" \
        && grep -q "island-simulator: clearing floors reuse existing noise fields for broad patch breakup" \
            "$LOG_FILE" \
        && grep -q "island-simulator: jungle per-instance foliage tint reuses existing transforms" \
            "$LOG_FILE" \
        && grep -q "island-simulator: palm per-instance frond tint reuses existing transforms" \
            "$LOG_FILE" \
        && grep -q "island-simulator: palm near leaflets use staggered asymmetric silhouettes" \
            "$LOG_FILE" \
        && grep -q "island-simulator: near fallen logs use irregular broken end contours" \
            "$LOG_FILE" \
        && grep -q "island-simulator: grass sparse near seed heads share blade wind and instancing" \
            "$LOG_FILE" \
        && grep -q "island-simulator: near rocks use irregular crown profiles and fixed-sun facet shading" \
            "$LOG_FILE" \
        && grep -q "island-simulator: rock per-instance tint reuses existing transforms" \
            "$LOG_FILE" \
        && grep -q "island-simulator: near birds use layered trailing flight-feather silhouettes" \
            "$LOG_FILE" \
        && grep -q "island-simulator: fallen fronds use curved asymmetric leaflet silhouettes" \
            "$LOG_FILE" \
        && grep -q "island-simulator: ocean sunward ripple crests reuse analytic slope field" \
            "$LOG_FILE" \
        && grep -q "island-simulator: ocean grazing reflection bands reuse analytic slope field" \
            "$LOG_FILE" \
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

if grep -Eiq "FATAL EXCEPTION|island-simulator: fatal error|shader.*(failed|error)" "$LOG_FILE"; then
    echo "android runtime smoke: fatal or shader error appeared in logcat" >&2
    tail -n 120 "$LOG_FILE" >&2
    exit 1
fi

if [ "$REQUIRE_TOUCH_SENSITIVITY" -eq 1 ]; then
    echo "android runtime smoke: exercising touch look-sensitivity selector"
    WINDOW_DIMENSIONS=$(sed -n \
        's/.*Window size: \([1-9][0-9]*x[1-9][0-9]*\).*/\1/p' "$LOG_FILE" | tail -n 1)
    LOGICAL_DIMENSIONS=$(sed -n \
        's/.*viewport \([1-9][0-9]*x[1-9][0-9]*\) logical backbuffer.*/\1/p' \
        "$LOG_FILE" | tail -n 1)
    WINDOW_WIDTH=${WINDOW_DIMENSIONS%x*}
    WINDOW_HEIGHT=${WINDOW_DIMENSIONS#*x}
    LOGICAL_WIDTH=${LOGICAL_DIMENSIONS%x*}
    LOGICAL_HEIGHT=${LOGICAL_DIMENSIONS#*x}
    case "$WINDOW_WIDTH:$WINDOW_HEIGHT:$LOGICAL_WIDTH:$LOGICAL_HEIGHT" in
        *[!0-9:]*|:*|*::*|*:)
            echo "android runtime smoke: could not parse window/backbuffer sizes for sensitivity input" >&2
            exit 1
            ;;
    esac
    # CNA scales normalized SDL touch positions into the logical backbuffer. Map the logical HUD
    # target back through the aspect-fit viewport, including Android's pillarbox/letterbox margin.
    if [ $((WINDOW_WIDTH * LOGICAL_HEIGHT)) -le \
         $((WINDOW_HEIGHT * LOGICAL_WIDTH)) ]; then
        CONTENT_WIDTH=$WINDOW_WIDTH
        CONTENT_HEIGHT=$((WINDOW_WIDTH * LOGICAL_HEIGHT / LOGICAL_WIDTH))
    else
        CONTENT_HEIGHT=$WINDOW_HEIGHT
        CONTENT_WIDTH=$((WINDOW_HEIGHT * LOGICAL_WIDTH / LOGICAL_HEIGHT))
    fi
    CONTENT_X=$(((WINDOW_WIDTH - CONTENT_WIDTH) / 2))
    CONTENT_Y=$(((WINDOW_HEIGHT - CONTENT_HEIGHT) / 2))
    SELECTOR_RADIUS=$((LOGICAL_HEIGHT * 6 / 100))
    SELECTOR_LOGICAL_X=$((LOGICAL_WIDTH - SELECTOR_RADIUS * 7 / 5))
    SELECTOR_LOGICAL_Y=$((SELECTOR_RADIUS * 7 / 5))
    SELECTOR_X=$((CONTENT_X + SELECTOR_LOGICAL_X * CONTENT_WIDTH / LOGICAL_WIDTH))
    SELECTOR_Y=$((CONTENT_Y + SELECTOR_LOGICAL_Y * CONTENT_HEIGHT / LOGICAL_HEIGHT))
    # ADB's instantaneous tap can press and release between game frames. A stationary bounded
    # swipe keeps the contact down long enough to exercise the real Pressed ownership path.
    "$ADB" shell input swipe "$SELECTOR_X" "$SELECTOR_Y" \
        "$SELECTOR_X" "$SELECTOR_Y" 500
    SENSITIVITY_DEADLINE=$(( $(date +%s) + 10 ))
    SENSITIVITY_QUALIFIED=0
    while [ "$(date +%s)" -lt "$SENSITIVITY_DEADLINE" ]; do
        "$ADB" logcat --pid="$GAME_PID" -d -v threadtime >"$LOG_FILE"
        if grep -q "island-simulator: touch look sensitivity high (1.35x)" "$LOG_FILE" \
            && grep -q "island-simulator: touch sensitivity selector retained touch ownership without mouse capture" \
                "$LOG_FILE"; then
            SENSITIVITY_QUALIFIED=1
            break
        fi
        sleep 1
    done
    if [ "$SENSITIVITY_QUALIFIED" -ne 1 ]; then
        echo "android runtime smoke: touch sensitivity selector did not choose high" >&2
        tail -n 120 "$LOG_FILE" >&2
        exit 1
    fi
fi

if [ "$REQUIRE_FOOTSTEPS" -eq 1 ]; then
    echo "android runtime smoke: exercising distance-scheduled land footsteps"
    # The AVD presents the landscape surface at 2400 by 1080. This sustained gesture starts in
    # its left movement zone and travels upward far enough to request forward motion and running.
    "$ADB" shell input swipe 600 820 600 180 3000
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
    # movement zone therefore walks backward through the shallows and continues as swimming. When
    # the footstep gate ran first, the extra duration also covers its short inland displacement.
    "$ADB" shell input swipe 600 180 600 900 9000
    SWIMMING_DEADLINE=$(( $(date +%s) + 20 ))
    SWIMMING_QUALIFIED=0
    while [ "$(date +%s)" -lt "$SWIMMING_DEADLINE" ]; do
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
    if [ "$SWIMMING_QUALIFIED" -ne 1 ]; then
        echo "android runtime smoke: touch traversal did not enter the swimming state" >&2
        tail -n 120 "$LOG_FILE" >&2
        exit 1
    fi
fi

if grep -Eiq "FATAL EXCEPTION|island-simulator: fatal error|shader.*(failed|error)" "$LOG_FILE"; then
    echo "android runtime smoke: fatal or shader error appeared after traversal" >&2
    tail -n 120 "$LOG_FILE" >&2
    exit 1
fi

"$ADB" exec-out screencap -p >"$SCREENSHOT"
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
        echo "android runtime smoke: same-process Home/resume did not restore surface and focus" >&2
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
    if grep -Eiq "FATAL EXCEPTION|island-simulator: fatal error|shader.*(failed|error)" \
        "$HOT_RESUME_LOG_FILE"; then
        echo "android runtime smoke: fatal or shader error appeared during hot resume" >&2
        tail -n 120 "$HOT_RESUME_LOG_FILE" >&2
        exit 1
    fi
    "$ADB" exec-out screencap -p >"$HOT_RESUME_SCREENSHOT"
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
        if [ "$REQUIRE_AUDIO" -eq 1 ] \
            && ! grep -q "island-simulator: audio sources use sequential generation and upload with bounded staging memory" \
                "$COLD_RELAUNCH_LOG_FILE"; then
            COLD_AUDIO_QUALIFIED=0
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
            && [ "$COLD_AUDIO_QUALIFIED" -eq 1 ]; then
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
    if grep -Eiq "FATAL EXCEPTION|island-simulator: fatal error|shader.*(failed|error)" \
        "$COLD_RELAUNCH_LOG_FILE"; then
        echo "android runtime smoke: fatal or shader error appeared during cold relaunch" >&2
        tail -n 120 "$COLD_RELAUNCH_LOG_FILE" >&2
        exit 1
    fi
    "$ADB" exec-out screencap -p >"$COLD_RELAUNCH_SCREENSHOT"
    if [ ! -s "$COLD_RELAUNCH_SCREENSHOT" ]; then
        echo "android runtime smoke: cold-relaunch screenshot is empty" >&2
        exit 1
    fi
    echo "android runtime smoke: lifecycle passed (hot PID $LIFECYCLE_PID, cold PID $COLD_PID)"
    echo "android runtime smoke: hot-resume artifacts (log: $HOT_RESUME_LOG_FILE, screenshot: $HOT_RESUME_SCREENSHOT)"
    echo "android runtime smoke: cold-relaunch artifacts (log: $COLD_RELAUNCH_LOG_FILE, screenshot: $COLD_RELAUNCH_SCREENSHOT)"
fi

echo "android runtime smoke passed (log: $LOG_FILE, screenshot: $SCREENSHOT)"
