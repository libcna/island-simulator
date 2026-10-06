#include "IslandSimulator/RuntimeDiagnostics.hpp"

#ifdef __ANDROID__
#include <android/log.h>
#endif

#include <limits>

namespace island
{
    void MirrorRuntimeDiagnosticToPlatformLog(const std::string_view message) noexcept
    {
#ifdef __ANDROID__
        const std::size_t boundedSize = message.size() > static_cast<std::size_t>(
            std::numeric_limits<int>::max())
            ? static_cast<std::size_t>(std::numeric_limits<int>::max())
            : message.size();
        __android_log_print(ANDROID_LOG_INFO, "island-simulator", "%.*s",
                            static_cast<int>(boundedSize), message.data());
#else
        (void)message;
#endif
    }
}
