#pragma once

#include <string_view>

namespace island
{
    /**
     * @brief Mirrors a high-value runtime diagnostic to the platform log when stdout is hidden.
     *
     * Android applications launched through SDLActivity do not expose the process stdout stream
     * through logcat. Other platforms retain their existing stdout diagnostics and this function
     * is intentionally a no-op there.
     */
    void MirrorRuntimeDiagnosticToPlatformLog(std::string_view message) noexcept;
}
