#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace Microsoft::Xna::Framework::Graphics
{
    class GraphicsDevice;
}

namespace island
{
    /** @brief Opens the per-process incident log and records the exact command line. */
    void InitializeRuntimeDiagnostics(int argc, char* const argv[]) noexcept;

    /** @brief Records the parsed application mode after command-line validation succeeds. */
    void RecordRuntimeWorkload(std::string_view mode, std::string_view viewpoint,
                               std::string_view capturePath,
                               std::string_view benchmarkPath,
                               std::string_view startupReportPath) noexcept;

    /** @brief Records CNA, SDL, OpenGL, Mesa, and physical-adapter identity. */
    void RecordGraphicsRuntimeDiagnostics(
        Microsoft::Xna::Framework::Graphics::GraphicsDevice& device) noexcept;

    /** @brief Records Linux task names so kernel TIDs can be assigned to their owners. */
    void RecordRuntimeThreadInventory() noexcept;

    /** @brief Returns the persistent per-PID log path, or an empty string when unavailable. */
    [[nodiscard]] std::string RuntimeDiagnosticLogPath() noexcept;

    /**
     * @brief Mirrors a high-value runtime diagnostic to the platform log when stdout is hidden.
     *
     * Android applications launched through SDLActivity do not expose the process stdout stream
     * through logcat. Every platform also appends the message to the persistent per-PID incident
     * log when that log could be opened.
     */
    void MirrorRuntimeDiagnosticToPlatformLog(std::string_view message) noexcept;
}
