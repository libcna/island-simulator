#!/usr/bin/env bash
set -euo pipefail

project_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
build_directory=${1:-"${project_root}/build-release"}
build_directory=$(cd "${build_directory}" && pwd)
output_directory="${build_directory}/linux-release"
archive_path="${build_directory}/island-simulator-linux-x86_64.tar.gz"

for command_name in awk cmake grep gzip mktemp mv readelf sha256sum stat tar; do
    if ! command -v "${command_name}" >/dev/null 2>&1; then
        echo "package_linux_release.sh: required command '${command_name}' is unavailable." >&2
        exit 1
    fi
done

cache_path="${build_directory}/CMakeCache.txt"
if [[ ! -f "${cache_path}" ]]; then
    echo "package_linux_release.sh: missing configured build '${cache_path}'." >&2
    exit 1
fi
if ! grep -q '^CMAKE_BUILD_TYPE:STRING=Release$' "${cache_path}"; then
    echo "package_linux_release.sh: '${build_directory}' is not a Release build." >&2
    exit 1
fi
if ! grep -q '^CNA_GRAPHICS_RENDERER:STRING=OPENGL33$' "${cache_path}"; then
    echo "package_linux_release.sh: '${build_directory}' is not configured for OPENGL33." >&2
    exit 1
fi

staging_directory=$(mktemp -d "${build_directory}/.linux-release.XXXXXX")
cleanup() {
    cmake -E remove_directory "${staging_directory}"
}
trap cleanup EXIT

cmake --install "${build_directory}" --prefix "${staging_directory}" --component Runtime --strip

readonly packaged_files=(
    island-simulator
    lib/libcna.so
    share/island-simulator/README.md
    share/island-simulator/ASSET_LICENSES.md
    share/island-simulator/dependencies.lock
    share/island-simulator/licenses/CNA/LICENSE
    share/island-simulator/licenses/CNA/NOTICE.md
    share/island-simulator/licenses/Sharp-Runtime/LICENSE
    share/island-simulator/licenses/EasyGL/LICENSE
    share/island-simulator/licenses/MetaGL/LICENSE
)

for relative_path in "${packaged_files[@]}"; do
    if [[ ! -f "${staging_directory}/${relative_path}" ]]; then
        echo "package_linux_release.sh: install omitted '${relative_path}'." >&2
        exit 1
    fi
done

if ! readelf -d "${staging_directory}/island-simulator" \
        | grep -Fq 'Library runpath: [$ORIGIN/lib]'; then
    echo "package_linux_release.sh: executable does not use the relocatable CNA runpath." >&2
    exit 1
fi

manifest_entries=()
total_bytes=0
for relative_path in "${packaged_files[@]}"; do
    packaged_path="${staging_directory}/${relative_path}"
    byte_count=$(stat -c '%s' "${packaged_path}")
    digest=$(sha256sum "${packaged_path}" | awk '{print $1}')
    total_bytes=$((total_bytes + byte_count))
    manifest_entries+=("    {\"name\": \"${relative_path}\", \"bytes\": ${byte_count}, \"sha256\": \"${digest}\"}")
done

manifest_path="${staging_directory}/manifest.json"
{
    echo '{'
    echo '  "schema": 1,'
    echo '  "platform": "linux-x86_64",'
    echo '  "renderer": "OPENGL33",'
    echo '  "runtimeDependencies": ["glibc", "libstdc++", "SDL 3", "SDL_mixer 3", "OpenGL 3.3 driver"],'
    echo '  "files": ['
    for index in "${!manifest_entries[@]}"; do
        suffix=','
        if (( index + 1 == ${#manifest_entries[@]} )); then
            suffix=''
        fi
        printf '%s%s\n' "${manifest_entries[index]}" "${suffix}"
    done
    echo '  ],'
    printf '  "totalBytes": %s\n' "${total_bytes}"
    echo '}'
} >"${manifest_path}"
chmod 0644 "${manifest_path}"

# Only this build-owned, fixed-name output is replaced. The staging tree is complete before the
# old package is touched, so a failed install never leaves a partial advertised release.
cmake -E remove_directory "${output_directory}"
mv "${staging_directory}" "${output_directory}"
trap - EXIT

archive_temporary="${archive_path}.tmp"
tar --sort=name --mtime='@0' --owner=0 --group=0 --numeric-owner \
    -C "${build_directory}" -cf - linux-release \
    | gzip -9 -n >"${archive_temporary}"
gzip -t "${archive_temporary}"
mv "${archive_temporary}" "${archive_path}"

archive_bytes=$(stat -c '%s' "${archive_path}")
archive_sha256=$(sha256sum "${archive_path}" | awk '{print $1}')
echo "Linux release package: ${output_directory}"
echo "Installed payload: ${total_bytes} bytes"
echo "Archive: ${archive_path} (${archive_bytes} bytes)"
echo "Archive SHA-256: ${archive_sha256}"
