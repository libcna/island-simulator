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
: >"$LOG_FILE"

echo "android runtime smoke: installing $APK"
"$ADB" install -r "$APK"
"$ADB" logcat -c
"$ADB" shell am force-stop "$PACKAGE"
echo "android runtime smoke: launching $COMPONENT"
if [ -n "$DEBUG_ARGUMENTS" ]; then
    echo "android runtime smoke: debug arguments $DEBUG_ARGUMENTS"
    "$ADB" shell am start -W -n "$COMPONENT" \
        --esa "$DEBUG_ARGUMENTS_EXTRA" "$DEBUG_ARGUMENTS" >"$LAUNCH_FILE"
else
    "$ADB" shell am start -W -n "$COMPONENT" >"$LAUNCH_FILE"
fi

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
        && grep -q "island-simulator: clearing floors reuse existing noise fields for broad patch breakup" \
            "$LOG_FILE" \
        && grep -q "island-simulator: jungle per-instance foliage tint reuses existing transforms" \
            "$LOG_FILE" \
        && grep -q "island-simulator: palm per-instance frond tint reuses existing transforms" \
            "$LOG_FILE" \
        && grep -q "island-simulator: grass sparse near seed heads share blade wind and instancing" \
            "$LOG_FILE" \
        && grep -q "island-simulator: near rocks use irregular crown profiles and fixed-sun facet shading" \
            "$LOG_FILE" \
        && grep -q "island-simulator: near birds use layered trailing flight-feather silhouettes" \
            "$LOG_FILE" \
        && grep -q "island-simulator: fallen fronds use curved asymmetric leaflet silhouettes" \
            "$LOG_FILE" \
        && grep -q "island-simulator: ocean sunward ripple crests reuse analytic slope field" \
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

"$ADB" exec-out screencap -p >"$SCREENSHOT"
if [ ! -s "$SCREENSHOT" ]; then
    echo "android runtime smoke: device screenshot is empty" >&2
    exit 1
fi

echo "android runtime smoke passed (log: $LOG_FILE, screenshot: $SCREENSHOT)"
