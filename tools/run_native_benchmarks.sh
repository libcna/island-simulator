#!/bin/sh

set -eu

project_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
output_directory=${1:-"$project_root/build/benchmarks"}
executable=${ISLAND_BENCHMARK_EXECUTABLE:-"$project_root/build/island-simulator"}

if [ ! -x "$executable" ]; then
    echo "island-simulator: benchmark executable is missing: $executable" >&2
    exit 2
fi

mkdir -p "$output_directory"
for viewpoint in overview beach jungle driftwood monkeys; do
    "$executable" --benchmark "$output_directory/$viewpoint.json" --viewpoint "$viewpoint"
done

echo "island-simulator: benchmark suite wrote reports to $output_directory"
