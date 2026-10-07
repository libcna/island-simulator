#!/bin/sh

set -eu

if [ "$#" -ne 2 ]; then
    echo "usage: $0 <island-simulator> <xdotool>" >&2
    exit 2
fi

GAME=$1
XDOTOOL=$2

if [ "${XDG_SESSION_TYPE:-}" = "wayland" ] \
    && [ -z "${CNA_WAYLAND_TEST_DISPLAY:-}" ] \
    && [ -z "${ISLAND_SETTINGS_SMOKE_XVFB:-}" ]; then
    XVFB_RUN=$(command -v xvfb-run || true)
    if [ -z "$XVFB_RUN" ]; then
        echo "settings smoke requires xvfb-run under a Wayland session" >&2
        exit 2
    fi
    export ISLAND_SETTINGS_SMOKE_XVFB=1
    exec "$XVFB_RUN" -a -s '-screen 0 1280x720x24' "$0" "$GAME" "$XDOTOOL"
fi

if [ -z "${DISPLAY:-}" ]; then
    echo "settings smoke requires an isolated X11/Xwayland display" >&2
    exit 2
fi

WORK_DIR=$(mktemp -d)
DATA_DIR="$WORK_DIR/data"
SETTINGS_FILE="$DATA_DIR/island-simulator/IslandSimulator/AllPlayers/settings.ini"
EXPECTED_FILE="$WORK_DIR/expected.ini"
GAME_PID=

cleanup()
{
    if [ -n "$GAME_PID" ] && kill -0 "$GAME_PID" 2>/dev/null; then
        kill "$GAME_PID" 2>/dev/null || true
        wait "$GAME_PID" 2>/dev/null || true
    fi
    rm -rf "$WORK_DIR"
}
trap cleanup EXIT INT TERM

tap_key()
{
    "$XDOTOOL" keydown "$1"
    # A heavily loaded software/translated display can take longer than one nominal frame to
    # publish a keyboard snapshot. Hold long enough for the real update loop to observe the edge.
    sleep 0.65
    "$XDOTOOL" keyup "$1"
    sleep 0.35
}

start_instance()
{
    LOG_FILE=$1
    XDG_DATA_HOME="$DATA_DIR" SDL_VIDEODRIVER=x11 \
        "$GAME" --debug-overlay >"$LOG_FILE" 2>&1 &
    GAME_PID=$!
    WINDOW=
    ATTEMPT=0
    while [ "$ATTEMPT" -lt 160 ]; do
        WINDOW=$("$XDOTOOL" search --onlyvisible --pid "$GAME_PID" \
            --name '^island-simulator$' 2>/dev/null | sed -n '1p' || true)
        if [ -n "$WINDOW" ]; then
            "$XDOTOOL" windowfocus --sync "$WINDOW"
            READY_ATTEMPT=0
            while ! grep -q "island-simulator: startup world" "$LOG_FILE"; do
                if ! kill -0 "$GAME_PID" 2>/dev/null; then
                    cat "$LOG_FILE" >&2
                    echo "settings smoke: game exited before its first rendered frame" >&2
                    exit 1
                fi
                if [ "$READY_ATTEMPT" -ge 240 ]; then
                    cat "$LOG_FILE" >&2
                    echo "settings smoke: game did not report its first rendered frame" >&2
                    exit 1
                fi
                READY_ATTEMPT=$((READY_ATTEMPT + 1))
                sleep 0.05
            done
            sleep 0.25
            return
        fi
        if ! kill -0 "$GAME_PID" 2>/dev/null; then
            cat "$LOG_FILE" >&2
            echo "settings smoke: game exited before creating its window" >&2
            exit 1
        fi
        ATTEMPT=$((ATTEMPT + 1))
        sleep 0.05
    done
    cat "$LOG_FILE" >&2
    echo "settings smoke: game window did not appear" >&2
    exit 1
}

finish_instance()
{
    LOG_FILE=$1
    tap_key Escape
    ATTEMPT=0
    while kill -0 "$GAME_PID" 2>/dev/null; do
        if [ "$ATTEMPT" -ge 160 ]; then
            cat "$LOG_FILE" >&2
            echo "settings smoke: game did not exit after Escape" >&2
            exit 1
        fi
        ATTEMPT=$((ATTEMPT + 1))
        sleep 0.05
    done
    if ! wait "$GAME_PID"; then
        cat "$LOG_FILE" >&2
        echo "settings smoke: game exited unsuccessfully" >&2
        exit 1
    fi
    GAME_PID=
}

FIRST_LOG="$WORK_DIR/first.log"
start_instance "$FIRST_LOG"
tap_key Shift_L
tap_key b
tap_key m
finish_instance "$FIRST_LOG"

if ! grep -q "user settings defaults run disabled camera motion enabled audio enabled touch standard" \
        "$FIRST_LOG" \
    || ! grep -q "run mode enabled (8.0 m/s)" "$FIRST_LOG" \
    || ! grep -q "camera motion disabled" "$FIRST_LOG" \
    || ! grep -q "soundscape muted" "$FIRST_LOG"; then
    cat "$FIRST_LOG" >&2
    echo "settings smoke: first run did not apply all preference changes" >&2
    exit 1
fi

printf '%s\n' \
    'schema=1' \
    'run_mode=run' \
    'camera_motion=disabled' \
    'audio=muted' \
    'touch_look=standard' >"$EXPECTED_FILE"
if [ ! -f "$SETTINGS_FILE" ] || ! cmp -s "$EXPECTED_FILE" "$SETTINGS_FILE"; then
    cat "$FIRST_LOG" >&2
    [ ! -f "$SETTINGS_FILE" ] || diff -u "$EXPECTED_FILE" "$SETTINGS_FILE" >&2
    echo "settings smoke: first run did not persist the exact expected settings" >&2
    exit 1
fi

SECOND_LOG="$WORK_DIR/second.log"
start_instance "$SECOND_LOG"
finish_instance "$SECOND_LOG"
if ! grep -q "user settings loaded run enabled camera motion disabled audio muted touch standard" \
        "$SECOND_LOG"; then
    cat "$SECOND_LOG" >&2
    echo "settings smoke: second run did not restore the primary settings" >&2
    exit 1
fi

# Corrupt only the private test profile. The prior valid copy must remain recoverable.
printf '%s\n' 'schema=1' 'audio=invalid' >"$SETTINGS_FILE"
THIRD_LOG="$WORK_DIR/third.log"
start_instance "$THIRD_LOG"
finish_instance "$THIRD_LOG"
if ! grep -q "primary settings are invalid" "$THIRD_LOG" \
    || ! grep -q "user settings recovered run enabled camera motion disabled audio enabled touch standard" \
        "$THIRD_LOG"; then
    cat "$THIRD_LOG" >&2
    echo "settings smoke: invalid primary settings did not recover from the valid backup" >&2
    exit 1
fi

echo "settings persistence smoke passed"
