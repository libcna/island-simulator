#!/bin/sh

set -eu

if [ "$#" -ne 2 ]; then
    echo "usage: $0 <island-simulator> <xdotool>" >&2
    exit 2
fi

GAME=$1
XDOTOOL=$2

if [ -z "${DISPLAY:-}" ]; then
    echo "free-camera input smoke requires an isolated X11/Xwayland display" >&2
    exit 2
fi

WORK_DIR=$(mktemp -d)
LOG_FILE="$WORK_DIR/free-camera.log"
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

wait_for_log()
{
    PATTERN=$1
    DESCRIPTION=$2
    ATTEMPT=0
    while ! grep -q "$PATTERN" "$LOG_FILE"; do
        if ! kill -0 "$GAME_PID" 2>/dev/null; then
            cat "$LOG_FILE" >&2
            echo "free-camera input smoke: game exited while waiting for $DESCRIPTION" >&2
            exit 1
        fi
        if [ "$ATTEMPT" -ge 100 ]; then
            cat "$LOG_FILE" >&2
            echo "free-camera input smoke: timed out waiting for $DESCRIPTION" >&2
            exit 1
        fi
        ATTEMPT=$((ATTEMPT + 1))
        sleep 0.05
    done
}

"$GAME" --debug-overlay --input-diagnostics >"$LOG_FILE" 2>&1 &
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
        echo "free-camera input smoke: game exited before creating its window" >&2
        exit 1
    fi
    ATTEMPT=$((ATTEMPT + 1))
    sleep 0.05
done

if [ -z "$WINDOW" ]; then
    cat "$LOG_FILE" >&2
    echo "free-camera input smoke: window did not appear" >&2
    exit 1
fi

"$XDOTOOL" windowfocus --sync "$WINDOW"
wait_for_log "input sample" "the first game input sample"
"$XDOTOOL" keydown F3
sleep 0.3
"$XDOTOOL" keyup F3
wait_for_log "debug free camera enabled" "F3 activation"
wait_for_log "input sample.*free 1" "the first active-camera sample"
"$XDOTOOL" keydown w
"$XDOTOOL" keydown e
sleep 1.2
"$XDOTOOL" keyup e
"$XDOTOOL" keyup w
sleep 0.5
"$XDOTOOL" keydown F3
sleep 0.3
"$XDOTOOL" keyup F3
wait_for_log "debug free camera disabled" "the second F3 edge"
"$XDOTOOL" keydown Escape
sleep 0.3
"$XDOTOOL" keyup Escape

ATTEMPT=0
while kill -0 "$GAME_PID" 2>/dev/null && [ "$ATTEMPT" -lt 100 ]; do
    ATTEMPT=$((ATTEMPT + 1))
    sleep 0.05
done

if kill -0 "$GAME_PID" 2>/dev/null; then
    cat "$LOG_FILE" >&2
    echo "free-camera input smoke: game did not exit after Escape" >&2
    exit 1
fi

if ! wait "$GAME_PID"; then
    GAME_PID=
    cat "$LOG_FILE" >&2
    echo "free-camera input smoke: game returned failure" >&2
    exit 1
fi
GAME_PID=

if ! awk '
    /input sample/ && $15 == "free" && $16 == 1 {
        if (count == 0) {
            player_x = $9;
            player_z = $10;
            camera_x = $12;
            camera_y = $13;
            camera_z = $14;
        }
        last_player_x = $9;
        last_player_z = $10;
        last_camera_x = $12;
        last_camera_y = $13;
        last_camera_z = $14;
        count += 1;
    }
    END {
        camera_dx = last_camera_x - camera_x;
        camera_dy = last_camera_y - camera_y;
        camera_dz = last_camera_z - camera_z;
        player_dx = last_player_x - player_x;
        player_dz = last_player_z - player_z;
        camera_delta = sqrt(camera_dx ^ 2 + camera_dy ^ 2 + camera_dz ^ 2);
        player_delta = sqrt(player_dx ^ 2 + player_dz ^ 2);
        exit !(count >= 2 && camera_delta > 0.5 && player_delta < 0.01);
    }
' "$LOG_FILE"; then
    cat "$LOG_FILE" >&2
    echo "free-camera input smoke: flight did not move the camera while freezing the player" >&2
    exit 1
fi

echo "free-camera input smoke passed"
