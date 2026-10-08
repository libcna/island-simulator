#include "IslandSimulator/GpuDiagnostics.hpp"

#include "IslandSimulator/RuntimeDiagnostics.hpp"

#ifndef __EMSCRIPTEN__
#include <SDL3/SDL.h>
#endif

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>

namespace island
{
    namespace
    {
#if defined(_WIN32)
#define ISLAND_GL_APIENTRY __stdcall
#else
#define ISLAND_GL_APIENTRY
#endif

        using DebugCallback = void (ISLAND_GL_APIENTRY *)(
            unsigned int, unsigned int, unsigned int, unsigned int, int, const char*, const void*);
        using EnableFunction = void (*)(unsigned int);
        using DebugMessageCallbackFunction = void (*)(DebugCallback, const void*);
        using PushDebugGroupFunction = void (*)(unsigned int, unsigned int, int, const char*);
        using PopDebugGroupFunction = void (*)();
        using GetErrorFunction = unsigned int (*)();

        constexpr unsigned int GlDebugOutput = 0x92E0U;
        constexpr unsigned int GlDebugOutputSynchronous = 0x8242U;
        constexpr unsigned int GlDebugSourceApplication = 0x824AU;
        constexpr unsigned int GlDebugTypePushGroup = 0x8269U;
        constexpr unsigned int GlDebugTypePopGroup = 0x826AU;
        constexpr unsigned int GlNoError = 0U;

        bool enabled = false;
        std::uint64_t currentFrame = 0U;
        EnableFunction glEnableFunction = nullptr;
        DebugMessageCallbackFunction glDebugMessageCallbackFunction = nullptr;
        PushDebugGroupFunction glPushDebugGroupFunction = nullptr;
        PopDebugGroupFunction glPopDebugGroupFunction = nullptr;
        GetErrorFunction glGetErrorFunction = nullptr;

        [[nodiscard]] bool DebugGroupsAvailable() noexcept
        {
            return glPushDebugGroupFunction != nullptr && glPopDebugGroupFunction != nullptr;
        }

        [[nodiscard]] bool Requested() noexcept
        {
            const char* value = std::getenv("ISLAND_GPU_DIAGNOSTICS");
            return value != nullptr && std::string_view(value) == "1";
        }

        template<typename Function>
        [[nodiscard]] Function Load(const char* name) noexcept
        {
#ifdef __EMSCRIPTEN__
            (void)name;
            return nullptr;
#else
            return reinterpret_cast<Function>(SDL_GL_GetProcAddress(name));
#endif
        }

        [[nodiscard]] const char* SeverityName(const unsigned int severity) noexcept
        {
            switch (severity) {
            case 0x9146U: return "high";
            case 0x9147U: return "medium";
            case 0x9148U: return "low";
            case 0x826BU: return "notification";
            default: return "unknown";
            }
        }

        void ISLAND_GL_APIENTRY DebugMessage(const unsigned int source,
                                             const unsigned int type,
                                             const unsigned int id,
                                             const unsigned int severity,
                                             const int length,
                                             const char* message,
                                             const void*)
        {
            // The persistent begin/end markers already record our own group boundaries. Mesa
            // echoes every push and pop as a notification; recording those again doubles the
            // incident log and needlessly perturbs the diagnostic workload.
            if (source == GlDebugSourceApplication
                && (type == GlDebugTypePushGroup || type == GlDebugTypePopGroup)) {
                return;
            }
            try {
                const char* safeMessage = message == nullptr ? "<null>" : message;
                const std::size_t size = message != nullptr && length > 0
                    ? static_cast<std::size_t>(length)
                    : std::char_traits<char>::length(safeMessage);
                std::ostringstream stream;
                stream << "island-simulator: gl-debug frame=" << currentFrame
                       << " severity=" << SeverityName(severity)
                       << " source=0x" << std::hex << source
                       << " type=0x" << type << " id=0x" << id << std::dec
                       << " message=" << std::string_view(safeMessage, size);
                const std::string text = stream.str();
                std::cerr << text << '\n';
                MirrorRuntimeDiagnosticToPlatformLog(text);
            } catch (...) {
            }
        }

        void CheckErrors(const std::string_view pass) noexcept
        {
            if (!enabled || glGetErrorFunction == nullptr) return;
            try {
                for (int count = 0; count < 32; ++count) {
                    const unsigned int error = glGetErrorFunction();
                    if (error == GlNoError) return;
                    std::ostringstream stream;
                    stream << "island-simulator: gl-error frame=" << currentFrame
                           << " pass=" << pass << " code=0x" << std::hex << error;
                    const std::string text = stream.str();
                    std::cerr << text << '\n';
                    MirrorRuntimeDiagnosticToPlatformLog(text);
                }
                MirrorRuntimeDiagnosticToPlatformLog(
                    "island-simulator: gl-error checkpoint stopped after 32 errors");
            } catch (...) {
                // Incident diagnostics must not replace the original application failure.
            }
        }

#undef ISLAND_GL_APIENTRY
    }

    void InitializeGpuDiagnostics() noexcept
    {
        if (!Requested()) return;
        try {
            glEnableFunction = Load<EnableFunction>("glEnable");
            glDebugMessageCallbackFunction =
                Load<DebugMessageCallbackFunction>("glDebugMessageCallback");
            glPushDebugGroupFunction = Load<PushDebugGroupFunction>("glPushDebugGroup");
            glPopDebugGroupFunction = Load<PopDebugGroupFunction>("glPopDebugGroup");
            glGetErrorFunction = Load<GetErrorFunction>("glGetError");
            enabled = glGetErrorFunction != nullptr;

            if (glEnableFunction != nullptr && glDebugMessageCallbackFunction != nullptr) {
                glEnableFunction(GlDebugOutput);
                glEnableFunction(GlDebugOutputSynchronous);
                glDebugMessageCallbackFunction(DebugMessage, nullptr);
            }

            std::ostringstream stream;
            stream << "island-simulator: gpu diagnostics enabled=" << (enabled ? 1 : 0)
                   << " khr-debug=" << (glDebugMessageCallbackFunction != nullptr ? 1 : 0)
                   << " debug-groups=" << (DebugGroupsAvailable() ? 1 : 0)
                   << " synchronization=unchanged";
            const std::string text = stream.str();
            std::cout << text << '\n';
            MirrorRuntimeDiagnosticToPlatformLog(text);
            CheckErrors("diagnostic-initialization");
        } catch (...) {
            enabled = false;
            MirrorRuntimeDiagnosticToPlatformLog(
                "island-simulator: GPU diagnostic initialization failed");
        }
    }

    void BeginGpuDiagnosticFrame(const std::uint64_t frame) noexcept
    {
        if (!enabled) return;
        currentFrame = frame;
        try {
            MirrorRuntimeDiagnosticToPlatformLog(
                "island-simulator: gpu-frame begin frame=" + std::to_string(currentFrame));
        } catch (...) {
        }
    }

    void EndGpuDiagnosticFrame() noexcept
    {
        if (!enabled) return;
        CheckErrors("frame-end");
        try {
            MirrorRuntimeDiagnosticToPlatformLog(
                "island-simulator: gpu-frame end frame=" + std::to_string(currentFrame));
        } catch (...) {
        }
    }

    GpuDiagnosticScope::GpuDiagnosticScope(const std::string_view name) noexcept
        : name_(name), active_(enabled)
    {
        if (!active_) return;
        try {
            MirrorRuntimeDiagnosticToPlatformLog(
                "island-simulator: gpu-pass begin frame=" + std::to_string(currentFrame)
                + " name=" + std::string(name_));
            if (DebugGroupsAvailable()) {
                glPushDebugGroupFunction(GlDebugSourceApplication, 0,
                                         static_cast<int>(name_.size()), name_.data());
                groupPushed_ = true;
            }
        } catch (...) {
        }
    }

    GpuDiagnosticScope::~GpuDiagnosticScope() noexcept
    {
        if (!active_) return;
        try {
            CheckErrors(name_);
            if (groupPushed_ && DebugGroupsAvailable()) glPopDebugGroupFunction();
            MirrorRuntimeDiagnosticToPlatformLog(
                "island-simulator: gpu-pass end frame=" + std::to_string(currentFrame)
                + " name=" + std::string(name_));
        } catch (...) {
        }
    }
}
