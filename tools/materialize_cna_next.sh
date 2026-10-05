#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
project_root="$(cd -- "${script_dir}/.." && pwd)"
cna_repository="${CNA_SOURCE_REPOSITORY:-${project_root}/../cna}"
destination="${1:-${project_root}/build/dependencies/cna-next}"
lock_file="${project_root}/dependencies.lock"

required_revision="$(sed -n 's/^CNA=//p' "${lock_file}")"
if [[ ! "${required_revision}" =~ ^[0-9a-f]{40}$ ]]; then
    echo "materialize_cna_next: dependencies.lock has no valid CNA commit" >&2
    exit 1
fi

actual_revision="$(git -C "${cna_repository}" rev-parse 'next^{commit}')"
if [[ "${actual_revision}" != "${required_revision}" ]]; then
    echo "materialize_cna_next: CNA next is ${actual_revision}, expected ${required_revision}" >&2
    echo "Update dependencies.lock only after deliberately qualifying the new baseline." >&2
    exit 1
fi

marker="${destination}/.island-cna-revision"
if [[ -f "${marker}" ]]; then
    existing_revision="$(sed -n '1p' "${marker}")"
    if [[ "${existing_revision}" == "${required_revision}" ]]; then
        echo "materialize_cna_next: reusing ${destination} at ${existing_revision}"
        exit 0
    fi
fi
if [[ -e "${destination}" ]]; then
    echo "materialize_cna_next: refusing to overwrite existing ${destination}" >&2
    echo "Remove that project-owned generated directory after checking it is unused, then retry." >&2
    exit 1
fi

parent="$(dirname -- "${destination}")"
pending="${parent}/cna-next.pending"
mkdir -p "${parent}"
if [[ -e "${pending}" ]]; then
    echo "materialize_cna_next: stale partial export exists at ${pending}" >&2
    echo "Inspect and remove it before retrying." >&2
    exit 1
fi
mkdir "${pending}"

cleanup_pending() {
    if [[ -d "${pending}" ]]; then
        rm -rf -- "${pending}"
    fi
}
trap cleanup_pending EXIT

git -C "${cna_repository}" archive --format=tar "${required_revision}" | tar -xf - -C "${pending}"

# Git archives intentionally omit submodule contents. CNA's Web and Android paths build their
# vendored SDL stack from these three pins, so copy the already-present, matching snapshots from
# the shared CNA worktree into this generated export. The containing CNA revisions must agree on
# every gitlink; this project never initializes or changes the source worktree's submodules.
for dependency in SDL SDL_image SDL_mixer; do
    dependency_path="third_party/${dependency}"
    required_gitlink="$(git -C "${cna_repository}" ls-tree "${required_revision}" \
        "${dependency_path}" | awk '{print $3}')"
    checkout_gitlink="$(git -C "${cna_repository}" ls-tree HEAD \
        "${dependency_path}" | awk '{print $3}')"
    if [[ -z "${required_gitlink}" || "${checkout_gitlink}" != "${required_gitlink}" ]]; then
        echo "materialize_cna_next: ${dependency_path} does not match CNA next" >&2
        exit 1
    fi
    if [[ ! -f "${cna_repository}/${dependency_path}/CMakeLists.txt" ]]; then
        echo "materialize_cna_next: populated ${dependency_path} snapshot is unavailable" >&2
        echo "The CNA owner must provide the pinned submodule contents without this project editing CNA." >&2
        exit 1
    fi
    tar -C "${cna_repository}" -cf - "${dependency_path}" | tar -xf - -C "${pending}"
done

printf '%s\n' "${required_revision}" > "${pending}/.island-cna-revision"
mv -- "${pending}" "${destination}"
trap - EXIT

echo "materialize_cna_next: exported CNA next ${required_revision} to ${destination}"
