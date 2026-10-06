#!/usr/bin/env bash

set -euo pipefail

repository_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
cna_root_dir="${CNA_ROOT_DIR:-${repository_dir}/build/dependencies/cna-next}"
gradle_wrapper="${cna_root_dir}/modules/devices/examples/demo_devices/android/com.openeggbert.cna.demodevices/gradlew"

if [[ -z "${ANDROID_SDK_ROOT:-}" ]]; then
    echo "run_android_gradle.sh: ANDROID_SDK_ROOT must name the Android SDK." >&2
    exit 2
fi

if [[ -z "${ANDROID_NDK_ROOT:-}" ]]; then
    export ANDROID_NDK_ROOT="${ANDROID_SDK_ROOT}/ndk/29.0.14206865"
fi

if [[ ! -x "${gradle_wrapper}" ]]; then
    echo "run_android_gradle.sh: CNA's Gradle wrapper was not found at ${gradle_wrapper}." >&2
    echo "Run tools/materialize_cna_next.sh or set CNA_ROOT_DIR to the pinned CNA next tree." >&2
    exit 2
fi

if [[ ! -d "${ANDROID_NDK_ROOT}" ]]; then
    echo "run_android_gradle.sh: Android NDK was not found at ${ANDROID_NDK_ROOT}." >&2
    exit 2
fi

export CCACHE_DIR=/rv/cnaccache
export CCACHE_BASEDIR=/rv
export CMAKE_BUILD_PARALLEL_LEVEL=2

# Android Gradle Plugin's incremental debug packaging can retain the superseded uncompressed
# native library as unreachable ZIP data after a C++ relink. Recreate only the generated APK for
# an explicit debug assembly/package request; all expensive native intermediates stay incremental.
for task in "$@"; do
    case "${task}" in
        assembleDebug|:app:assembleDebug|app:assembleDebug|packageDebug|:app:packageDebug|app:packageDebug)
            debug_apk="${repository_dir}/android/app/build/outputs/apk/debug/app-debug.apk"
            if [[ -f "${debug_apk}" ]]; then
                rm -- "${debug_apk}"
                echo "run_android_gradle.sh: removed the previous debug APK before packaging."
            fi
            break
            ;;
    esac
done

exec "${gradle_wrapper}" -p "${repository_dir}/android" --max-workers=2 "$@"
