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
    && [ -z "${ISLAND_PLAYER_INPUT_SMOKE_XVFB:-}" ]; then
    XVFB_RUN=$(command -v xvfb-run || true)
    if [ -z "$XVFB_RUN" ]; then
        echo "player input smoke requires xvfb-run under a Wayland session" >&2
        exit 2
    fi
    export ISLAND_PLAYER_INPUT_SMOKE_XVFB=1
    exec "$XVFB_RUN" -a -s '-screen 0 1280x720x24' "$0" "$GAME" "$XDOTOOL"
fi

if [ -z "${DISPLAY:-}" ]; then
    echo "player input smoke requires an isolated X11/Xwayland display" >&2
    exit 2
fi

WORK_DIR=$(mktemp -d)
LOG_FILE="$WORK_DIR/player-input.log"
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

wait_for_log_after()
{
    START_LINE=$1
    PATTERN=$2
    DESCRIPTION=$3
    ATTEMPT=0
    while ! tail -n "+$((START_LINE + 1))" "$LOG_FILE" | grep -q "$PATTERN"; do
        if ! kill -0 "$GAME_PID" 2>/dev/null; then
            cat "$LOG_FILE" >&2
            echo "player input smoke: game exited while waiting for $DESCRIPTION" >&2
            exit 1
        fi
        if [ "$ATTEMPT" -ge 120 ]; then
            cat "$LOG_FILE" >&2
            echo "player input smoke: timed out waiting for $DESCRIPTION" >&2
            exit 1
        fi
        ATTEMPT=$((ATTEMPT + 1))
        sleep 0.05
    done
}

tap_key()
{
    "$XDOTOOL" keydown "$1"
    sleep 0.12
    "$XDOTOOL" keyup "$1"
}

reset_to_south_beach()
{
    RESET_LINE=$(wc -l < "$LOG_FILE")
    tap_key F5
    wait_for_log_after "$RESET_LINE" "debug teleport south beach" "south-beach reset"
    sleep 0.3
}

capture_since()
{
    START_LINE=$1
    OUTPUT_FILE=$2
    tail -n "+$((START_LINE + 1))" "$LOG_FILE" > "$OUTPUT_FILE"
}

SDL_VIDEODRIVER=x11 "$GAME" --debug-overlay --input-diagnostics >"$LOG_FILE" 2>&1 &
GAME_PID=$!

WINDOW=
ATTEMPT=0
while [ "$ATTEMPT" -lt 100 ]; do
    WINDOW=$("$XDOTOOL" search --onlyvisible --pid "$GAME_PID" --name '^island-simulator$' \
        2>/dev/null | sed -n '1p' || true)
    if [ -n "$WINDOW" ]; then
        break
    fi
    if ! kill -0 "$GAME_PID" 2>/dev/null; then
        cat "$LOG_FILE" >&2
        echo "player input smoke: game exited before creating its window" >&2
        exit 1
    fi
    ATTEMPT=$((ATTEMPT + 1))
    sleep 0.05
done

if [ -z "$WINDOW" ]; then
    cat "$LOG_FILE" >&2
    echo "player input smoke: window did not appear" >&2
    exit 1
fi

"$XDOTOOL" windowfocus --sync "$WINDOW"
"$XDOTOOL" mousemove --sync --window "$WINDOW" 640 360
INITIAL_LINE=$(wc -l < "$LOG_FILE")
wait_for_log_after "$INITIAL_LINE" "input sample captured 0 relative 0" "initial input sample"

# Prove that the first movement-key edge captures the native desktop mouse. Keep W held while
# sending two relative turns so the same samples must show both forward progress and changing yaw.
LOOK_LINE=$(wc -l < "$LOG_FILE")
"$XDOTOOL" keydown w
wait_for_log_after "$LOOK_LINE" "input sample captured 1 relative 1" "keyboard mouse capture"
sleep 0.3
"$XDOTOOL" mousemove_relative --sync -- 90 0
sleep 0.4
"$XDOTOOL" mousemove_relative --sync -- 90 0
sleep 0.4
"$XDOTOOL" keyup w
sleep 0.35
capture_since "$LOOK_LINE" "$WORK_DIR/w-mouse.log"
if ! awk '
    /input sample/ && $15 == "free" && $16 == 0 && $5 == 1 && $7 == 1 {
        if (count == 0) {
            first_x = $9;
            first_z = $10;
            first_yaw = $18;
        }
        last_x = $9;
        last_z = $10;
        last_yaw = $18;
        count += 1;
    }
    END {
        distance = sqrt((last_x - first_x) ^ 2 + (last_z - first_z) ^ 2);
        yaw_delta = last_yaw - first_yaw;
        if (yaw_delta < 0) yaw_delta = -yaw_delta;
        exit !(count >= 3 && distance > 0.35 && yaw_delta > 0.08);
    }
' "$WORK_DIR/w-mouse.log"; then
    cat "$WORK_DIR/w-mouse.log" >&2
    echo "player input smoke: simultaneous W and relative mouse did not move and turn" >&2
    exit 1
fi

# At the reset yaw, both D and Right must travel toward visible screen-right, which is negative
# world X in the current CreateLookAt convention.
reset_to_south_beach
D_LINE=$(wc -l < "$LOG_FILE")
"$XDOTOOL" keydown d
sleep 0.9
"$XDOTOOL" keyup d
sleep 0.35
capture_since "$D_LINE" "$WORK_DIR/d-strafe.log"
if ! awk '
    /input sample/ && $15 == "free" && $16 == 0 {
        if (count == 0) first_x = $9;
        last_x = $9;
        count += 1;
    }
    END { exit !(count >= 2 && last_x < first_x - 0.25); }
' "$WORK_DIR/d-strafe.log"; then
    cat "$WORK_DIR/d-strafe.log" >&2
    echo "player input smoke: D did not strafe toward visible screen-right" >&2
    exit 1
fi

reset_to_south_beach
RIGHT_LINE=$(wc -l < "$LOG_FILE")
"$XDOTOOL" keydown Right
sleep 0.9
"$XDOTOOL" keyup Right
sleep 0.35
capture_since "$RIGHT_LINE" "$WORK_DIR/right-strafe.log"
if ! awk '
    /input sample/ && $15 == "free" && $16 == 0 {
        if (count == 0) first_x = $9;
        last_x = $9;
        count += 1;
    }
    END { exit !(count >= 2 && last_x < first_x - 0.25); }
' "$WORK_DIR/right-strafe.log"; then
    cat "$WORK_DIR/right-strafe.log" >&2
    echo "player input smoke: Right arrow did not match D" >&2
    exit 1
fi

# One Shift press must select running after release; a second press must restore walking.
reset_to_south_beach
tap_key Shift_L
sleep 0.25
RUN_LINE=$(wc -l < "$LOG_FILE")
"$XDOTOOL" keydown Up
sleep 1.2
"$XDOTOOL" keyup Up
sleep 0.35
capture_since "$RUN_LINE" "$WORK_DIR/arrow-run.log"
if ! awk '
    /input sample/ && $15 == "free" && $16 == 0 {
        if (count == 0) first_z = $10;
        last_z = $10;
        speed = sqrt($27 ^ 2 + $28 ^ 2);
        if ($24 == "run" && $25 == 1 && speed > 7.0) fast = 1;
        count += 1;
    }
    END { exit !(count >= 3 && last_z > first_z + 0.35 && fast == 1); }
' "$WORK_DIR/arrow-run.log"; then
    cat "$WORK_DIR/arrow-run.log" >&2
    echo "player input smoke: a released Shift press did not retain the faster run mode" >&2
    exit 1
fi

tap_key Shift_L
sleep 0.25
reset_to_south_beach
WALK_LINE=$(wc -l < "$LOG_FILE")
"$XDOTOOL" keydown Up
sleep 1.0
"$XDOTOOL" keyup Up
sleep 0.35
capture_since "$WALK_LINE" "$WORK_DIR/arrow-walk.log"
if ! awk '
    /input sample/ && $15 == "free" && $16 == 0 {
        if (count == 0) first_z = $10;
        last_z = $10;
        speed = sqrt($27 ^ 2 + $28 ^ 2);
        if ($24 != "run" || $25 != 0 || speed > 4.35) invalid = 1;
        if (speed > 3.5) walking = 1;
        count += 1;
    }
    END { exit !(count >= 3 && last_z > first_z + 0.35 && walking == 1 && invalid != 1); }
' "$WORK_DIR/arrow-walk.log"; then
    cat "$WORK_DIR/arrow-walk.log" >&2
    echo "player input smoke: the second Shift press did not restore walking" >&2
    exit 1
fi

# One Escape edge releases capture; the next edge exits.
tap_key Escape
sleep 0.2
tap_key Escape

ATTEMPT=0
while kill -0 "$GAME_PID" 2>/dev/null && [ "$ATTEMPT" -lt 100 ]; do
    ATTEMPT=$((ATTEMPT + 1))
    sleep 0.05
done

if kill -0 "$GAME_PID" 2>/dev/null; then
    cat "$LOG_FILE" >&2
    echo "player input smoke: game did not exit after two Escape edges" >&2
    exit 1
fi

if ! wait "$GAME_PID"; then
    GAME_PID=
    cat "$LOG_FILE" >&2
    echo "player input smoke: game returned failure" >&2
    exit 1
fi
GAME_PID=

echo "player input smoke passed"
