#!/usr/bin/env bash
set -euo pipefail

project_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
build_directory=${1:-"${project_root}/build-probe"}
output_directory="${build_directory}/web-release"

readonly artifact_names=(
    island-simulator.html
    island-simulator.js
    island-simulator.wasm
)
readonly content_types=(
    'text/html; charset=utf-8'
    'text/javascript; charset=utf-8'
    'application/wasm'
)

for command_name in gzip install sha256sum stat awk; do
    if ! command -v "${command_name}" >/dev/null 2>&1; then
        echo "package_web_release.sh: required command '${command_name}' is unavailable." >&2
        exit 1
    fi
done

for artifact_name in "${artifact_names[@]}"; do
    source_path="${build_directory}/${artifact_name}"
    if [[ ! -f "${source_path}" ]]; then
        echo "package_web_release.sh: missing release artifact '${source_path}'." >&2
        exit 1
    fi
done

install -d -m 0755 "${output_directory}"

total_bytes=0
total_gzip_bytes=0
manifest_entries=()
for index in "${!artifact_names[@]}"; do
    artifact_name=${artifact_names[index]}
    content_type=${content_types[index]}
    source_path="${build_directory}/${artifact_name}"
    packaged_path="${output_directory}/${artifact_name}"
    compressed_path="${packaged_path}.gz"

    install -m 0644 "${source_path}" "${packaged_path}"
    gzip -9 -n -c "${source_path}" >"${compressed_path}"
    chmod 0644 "${compressed_path}"
    gzip -t "${compressed_path}"

    source_sha256=$(sha256sum "${source_path}" | awk '{print $1}')
    packaged_sha256=$(sha256sum "${packaged_path}" | awk '{print $1}')
    restored_sha256=$(gzip -dc "${compressed_path}" | sha256sum | awk '{print $1}')
    if [[ "${source_sha256}" != "${packaged_sha256}"
          || "${source_sha256}" != "${restored_sha256}" ]]; then
        echo "package_web_release.sh: integrity check failed for '${artifact_name}'." >&2
        exit 1
    fi

    byte_count=$(stat -c '%s' "${packaged_path}")
    gzip_byte_count=$(stat -c '%s' "${compressed_path}")
    total_bytes=$((total_bytes + byte_count))
    total_gzip_bytes=$((total_gzip_bytes + gzip_byte_count))
    manifest_entries+=("    {\"name\": \"${artifact_name}\", \"contentType\": \"${content_type}\", \"bytes\": ${byte_count}, \"sha256\": \"${source_sha256}\", \"gzipBytes\": ${gzip_byte_count}}")
done

reduction_percent=$(awk -v original="${total_bytes}" -v compressed="${total_gzip_bytes}" \
    'BEGIN { if (original == 0) print "0.0"; else printf "%.1f", (1.0 - compressed / original) * 100.0 }')

manifest_path="${output_directory}/manifest.json"
{
    echo '{'
    echo '  "schema": 1,'
    echo '  "contentEncoding": "gzip",'
    echo '  "files": ['
    for index in "${!manifest_entries[@]}"; do
        suffix=','
        if (( index + 1 == ${#manifest_entries[@]} )); then
            suffix=''
        fi
        printf '%s%s\n' "${manifest_entries[index]}" "${suffix}"
    done
    echo '  ],'
    printf '  "totalBytes": %s,\n' "${total_bytes}"
    printf '  "totalGzipBytes": %s,\n' "${total_gzip_bytes}"
    printf '  "transferReductionPercent": %s\n' "${reduction_percent}"
    echo '}'
} >"${manifest_path}"
chmod 0644 "${manifest_path}"

echo "Web release package: ${output_directory}"
echo "Uncompressed payload: ${total_bytes} bytes"
echo "Gzip payload: ${total_gzip_bytes} bytes (${reduction_percent}% reduction)"
