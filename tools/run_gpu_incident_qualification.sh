#!/bin/sh

# Serial, fail-closed qualification harness for the 2026-10-08 AMDGPU GPUVM incident.
# This script intentionally launches exactly one island-simulator process. It refuses to run when
# the invoking account cannot read the kernel journal because absence of evidence is not evidence
# that a workload is safe.

set -eu

if [ "$#" -ne 2 ]; then
    echo "usage: $0 <build-directory> <workload>" >&2
    echo "workloads: smoke, benchmark-jungle, benchmark-traversal, capture-<viewpoint>" >&2
    exit 2
fi

project_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
build_directory=$(CDPATH= cd -- "$1" 2>/dev/null && pwd) || {
    echo "GPU qualification: build directory does not exist: $1" >&2
    exit 2
}
workload=$2
game="$build_directory/island-simulator"
private_runner="$project_root/build/dependencies/cna-next/tools/platform/run_gpu_tests_private.sh"

if [ ! -x "$game" ]; then
    echo "GPU qualification: game executable is missing: $game" >&2
    exit 2
fi
if [ ! -x "$private_runner" ]; then
    echo "GPU qualification: CNA private-display runner is missing: $private_runner" >&2
    exit 2
fi

# A successful journalctl exit is insufficient: without systemd-journal/adm membership it can
# return no kernel records. Require a real JSON kernel record and a cursor before touching the GPU.
kernel_probe=$(journalctl -k -n 1 -o json --no-pager 2>/dev/null || true)
case "$kernel_probe" in
    *'"_TRANSPORT":"kernel"'*) ;;
    *)
        echo "GPU qualification REFUSED: this account cannot read a kernel journal record." >&2
        echo "Grant read-only journal access, start a new login session, and retry." >&2
        exit 3
        ;;
esac

cursor_output=$(journalctl -k -n 0 --show-cursor --no-pager 2>/dev/null || true)
kernel_cursor=$(printf '%s\n' "$cursor_output" | sed -n 's/^-- cursor: //p' | tail -n 1)
if [ -z "$kernel_cursor" ]; then
    echo "GPU qualification REFUSED: no kernel-journal cursor was available." >&2
    exit 3
fi

timestamp=$(date -u +%Y%m%dT%H%M%SZ)
evidence_directory="$build_directory/gpu-incident-evidence/$timestamp-$$-$workload"
mkdir -p "$evidence_directory/runtime-logs"

case "$workload" in
    smoke)
        set -- "$game" --smoke-test
        ;;
    benchmark-jungle)
        set -- "$game" --benchmark "$evidence_directory/benchmark-jungle.json" \
            --viewpoint jungle
        ;;
    benchmark-traversal)
        set -- "$game" --benchmark "$evidence_directory/benchmark-traversal.json" \
            --viewpoint traversal
        ;;
    capture-*)
        viewpoint=${workload#capture-}
        case "$viewpoint" in
            overview|beach|jungle|east|west|grass|ground|litter|contact|movement|palms|water|current|distant|props|driftwood|reef|fronds|birds|fish|shark|monkeys|shipwreck|crabs|butterflies|turtles)
                ;;
            *)
                echo "GPU qualification: unsupported capture viewpoint: $viewpoint" >&2
                exit 2
                ;;
        esac
        set -- "$game" --capture "$evidence_directory/capture-$viewpoint.ppm" \
            --viewpoint "$viewpoint"
        ;;
    *)
        echo "GPU qualification: unknown workload: $workload" >&2
        exit 2
        ;;
esac

{
    echo "started_utc=$timestamp"
    echo "kernel_cursor=$kernel_cursor"
    printf 'command='
    printf ' %s' "$@"
    printf '\n'
    echo "serialization=one-game-process"
} > "$evidence_directory/run-metadata.txt"

export ISLAND_RUNTIME_LOG_DIRECTORY="$evidence_directory/runtime-logs"
export ISLAND_GPU_DIAGNOSTICS=1
export MESA_DEBUG=context
export CNA_WAYLAND_TEST_KEEP_LOG="$evidence_directory/compositor.log"

echo "GPU qualification: running one workload; evidence: $evidence_directory"
fault_pattern='amdgpu.*(page fault|timeout|GPU reset|VRAM is lost)|gfxhub.*page fault|ring gfx.*timeout|GPU reset|VRAM is lost'

# Poll from the pre-run cursor while the workload is alive. A private compositor is not a GPU
# safety boundary, so terminate this harness's own process group as soon as the first matching
# kernel record is visible. The final cursor scan below remains authoritative and closes races at
# process startup/exit. Polling cannot undo a command stream already executing on the GPU, but it
# avoids continuing with later frames after a reportable first fault.
monitor_kernel()
{
    monitor_cursor=$kernel_cursor
    : > "$evidence_directory/kernel-live.log"
    while kill -0 "$workload_pid" 2>/dev/null; do
        poll_output=$(journalctl -k --after-cursor "$monitor_cursor" --show-cursor \
            --no-pager -o short-iso-precise 2>/dev/null || true)
        poll_cursor=$(printf '%s\n' "$poll_output" \
            | sed -n 's/^-- cursor: //p' | tail -n 1)
        poll_records=$(printf '%s\n' "$poll_output" | sed '/^-- cursor: /d')
        if [ -n "$poll_records" ]; then
            printf '%s\n' "$poll_records" >> "$evidence_directory/kernel-live.log"
            if printf '%s\n' "$poll_records" | grep -E -i "$fault_pattern" \
                    > "$evidence_directory/kernel-live-trigger.log"; then
                printf '%s\n' "fault_detected_utc=$(date -u +%Y%m%dT%H%M%SZ)" \
                    >> "$evidence_directory/run-metadata.txt"
                kill -TERM -- "-$workload_pid" 2>/dev/null || true
                return
            fi
        fi
        if [ -n "$poll_cursor" ]; then
            monitor_cursor=$poll_cursor
        fi
        sleep 0.2
    done
}

set +e
setsid "$private_runner" --exec "$@" > "$evidence_directory/workload.log" 2>&1 &
workload_pid=$!
echo "workload_process_group=$workload_pid" >> "$evidence_directory/run-metadata.txt"
monitor_kernel &
monitor_pid=$!
wait "$workload_pid"
workload_status=$?
wait "$monitor_pid" 2>/dev/null
set -e

journalctl -k --after-cursor "$kernel_cursor" --no-pager -o short-iso-precise \
    > "$evidence_directory/kernel-after.log"

if grep -E -i "$fault_pattern" "$evidence_directory/kernel-after.log" \
        > "$evidence_directory/kernel-fault-matches.log"; then
    echo "GPU qualification FAILED: a new AMDGPU fault/timeout/reset was recorded." >&2
    echo "No further GPU workload may be launched; inspect $evidence_directory" >&2
    exit 4
fi
: > "$evidence_directory/kernel-fault-matches.log"

if [ "$workload_status" -ne 0 ]; then
    echo "GPU qualification FAILED: workload exited with status $workload_status." >&2
    echo "Kernel fault scan was clean; inspect $evidence_directory/workload.log" >&2
    exit "$workload_status"
fi

echo "GPU qualification passed: no new AMDGPU GPUVM fault, ring timeout, or reset."
echo "Evidence preserved at $evidence_directory"
