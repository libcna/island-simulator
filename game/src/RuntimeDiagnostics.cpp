#include "IslandSimulator/RuntimeDiagnostics.hpp"

#include "Microsoft/Xna/Framework/Graphics/GraphicsAdapter.hpp"
#include "Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp"

#ifdef __EMSCRIPTEN__
#include <GLES3/gl3.h>
#else
#include <SDL3/SDL.h>
#endif

#ifdef __ANDROID__
#include <android/log.h>
#endif

#if defined(__linux__) && !defined(__EMSCRIPTEN__)
#include <sys/syscall.h>
#include <unistd.h>
#endif

#include <algorithm>
#include <array>
#include <charconv>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <mutex>
#include <ranges>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

namespace island
{
    namespace
    {
        std::mutex diagnosticMutex;
        std::ofstream diagnosticLog;
        std::string diagnosticLogPath;

#if defined(__linux__) && !defined(__EMSCRIPTEN__)
        [[nodiscard]] std::uint64_t ProcfsId(const char* link, const std::uint64_t fallback) noexcept
        {
            // The test supervisor retains its procfs mount across PID-namespace containment.
            // Kernel reports use these outer IDs, not getpid()/gettid() inside the namespace.
            // On Android the procfs mount normally belongs to the application's PID namespace.
            std::array<char, 96> buffer{};
            const auto count = ::readlink(link, buffer.data(), buffer.size());
            if (count <= 0 || static_cast<std::size_t>(count) >= buffer.size()) return fallback;
            std::string_view value(buffer.data(), static_cast<std::size_t>(count));
            const auto slash = value.find_last_of('/');
            if (slash != std::string_view::npos) value.remove_prefix(slash + 1U);
            std::uint64_t result{};
            const auto parsed = std::from_chars(value.data(), value.data() + value.size(), result);
            return parsed.ec == std::errc{} && parsed.ptr == value.data() + value.size()
                && result != 0U ? result : fallback;
        }
#endif

        [[nodiscard]] std::uint64_t ProcessId() noexcept
        {
#if defined(__linux__) && !defined(__EMSCRIPTEN__)
            return ProcfsId("/proc/self", static_cast<std::uint64_t>(::getpid()));
#else
            return 0U;
#endif
        }

        [[nodiscard]] std::uint64_t CurrentThreadId() noexcept
        {
#if defined(__linux__) && !defined(__EMSCRIPTEN__)
            return ProcfsId("/proc/thread-self", static_cast<std::uint64_t>(::syscall(SYS_gettid)));
#else
            return static_cast<std::uint64_t>(
                std::hash<std::thread::id>{}(std::this_thread::get_id()));
#endif
        }

        [[nodiscard]] std::string Timestamp()
        {
            const auto now = std::chrono::system_clock::now();
            const std::time_t value = std::chrono::system_clock::to_time_t(now);
            std::tm utc{};
#if defined(_WIN32)
            gmtime_s(&utc, &value);
#else
            gmtime_r(&value, &utc);
#endif
            const auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
                now.time_since_epoch()) % std::chrono::seconds(1);
            std::ostringstream stream;
            stream << std::put_time(&utc, "%Y-%m-%dT%H:%M:%S") << '.'
                   << std::setw(3) << std::setfill('0') << milliseconds.count() << 'Z';
            return stream.str();
        }

        [[nodiscard]] std::string Quote(const std::string_view value)
        {
            std::string result;
            result.reserve(value.size() + 2U);
            result.push_back('"');
            for (const char character : value) {
                switch (character) {
                case '\\': result.append("\\\\"); break;
                case '"': result.append("\\\""); break;
                case '\n': result.append("\\n"); break;
                case '\r': result.append("\\r"); break;
                case '\t': result.append("\\t"); break;
                default: result.push_back(character); break;
                }
            }
            result.push_back('"');
            return result;
        }

        [[nodiscard]] std::filesystem::path DefaultLogDirectory()
        {
#if defined(__EMSCRIPTEN__) || defined(__ANDROID__)
            return {};
#else
            if (const char* explicitDirectory = std::getenv("ISLAND_RUNTIME_LOG_DIRECTORY");
                explicitDirectory != nullptr && explicitDirectory[0] != '\0') {
                return explicitDirectory;
            }
            if (const char* stateHome = std::getenv("XDG_STATE_HOME");
                stateHome != nullptr && stateHome[0] != '\0') {
                return std::filesystem::path(stateHome) / "island-simulator" / "runtime";
            }
            if (const char* userHome = std::getenv("HOME");
                userHome != nullptr && userHome[0] != '\0') {
                return std::filesystem::path(userHome) / ".local" / "state"
                    / "island-simulator" / "runtime";
            }
            return std::filesystem::temp_directory_path() / "island-simulator-runtime";
#endif
        }

        void AppendPersistentDiagnostic(const std::string_view message) noexcept
        {
            try {
                std::scoped_lock lock(diagnosticMutex);
                if (!diagnosticLog.is_open()) return;
                diagnosticLog << Timestamp() << " pid=" << ProcessId()
                              << " tid=" << CurrentThreadId() << ' ' << message << '\n';
                diagnosticLog.flush();
            } catch (...) {
                // Diagnostics must never replace the original application failure.
            }
        }

        [[nodiscard]] std::string EnvironmentValue(const char* name)
        {
            const char* value = std::getenv(name);
            return value == nullptr ? "<unset>" : Quote(value);
        }

#ifndef __EMSCRIPTEN__
        [[nodiscard]] const char* SafeSdlString(const char* value) noexcept
        {
            return value == nullptr || value[0] == '\0' ? "<unavailable>" : value;
        }
#endif

        [[nodiscard]] const char* GlString(const unsigned int name) noexcept
        {
#ifdef __EMSCRIPTEN__
            const GLubyte* value = glGetString(static_cast<GLenum>(name));
#else
            using GetStringFunction = const unsigned char* (*)(unsigned int);
            const auto function = reinterpret_cast<GetStringFunction>(
                SDL_GL_GetProcAddress("glGetString"));
            if (function == nullptr) return "<unavailable>";
            const unsigned char* value = function(name);
#endif
            return value == nullptr ? "<unavailable>"
                                    : reinterpret_cast<const char*>(value);
        }

        [[nodiscard]] const char* PlatformName() noexcept
        {
#ifdef __EMSCRIPTEN__
            return "Emscripten";
#else
            return SafeSdlString(SDL_GetPlatform());
#endif
        }

        [[nodiscard]] const char* VideoDriverName() noexcept
        {
#ifdef __EMSCRIPTEN__
            return "browser/WebGL";
#else
            return SafeSdlString(SDL_GetCurrentVideoDriver());
#endif
        }

        [[nodiscard]] std::string_view GraphicsApiName(
            const std::string_view rendererName) noexcept
        {
            if (rendererName.find("WEBGL") != std::string_view::npos) {
                return "WebGL 2";
            }
            if (rendererName.find("OPENGLES") != std::string_view::npos) {
                return "OpenGL ES 3";
            }
            return "OpenGL";
        }
    }

    void InitializeRuntimeDiagnostics(const int argc, char* const argv[]) noexcept
    {
        try {
            const std::filesystem::path directory = DefaultLogDirectory();
            if (!directory.empty()) {
                std::error_code error;
                std::filesystem::create_directories(directory, error);
                if (!error) {
                    const std::filesystem::path path = directory /
                        ("island-runtime-" + std::to_string(ProcessId()) + ".log");
                    std::scoped_lock lock(diagnosticMutex);
                    diagnosticLog.open(path, std::ios::out | std::ios::trunc);
                    if (diagnosticLog.is_open()) diagnosticLogPath = path.string();
                }
            }

            std::ostringstream command;
            command << "island-simulator: runtime-start pid=" << ProcessId()
                    << " main-tid=" << CurrentThreadId();
#if defined(__linux__) && !defined(__EMSCRIPTEN__)
            command << " namespace-pid=" << ::getpid()
                    << " namespace-main-tid=" << ::syscall(SYS_gettid)
                    << " pid-source=procfs-or-namespace-fallback";
#endif
            command << " command-line=";
            for (int index = 0; index < argc; ++index) {
                if (index != 0) command << ' ';
                command << Quote(argv[index] == nullptr ? "<null>" : argv[index]);
            }
            std::error_code currentPathError;
            const std::filesystem::path currentPath =
                std::filesystem::current_path(currentPathError);
            command << " cwd="
                    << (currentPathError ? "<unavailable>" : Quote(currentPath.string()))
                    << " log="
                    << (diagnosticLogPath.empty() ? "<unavailable>" : Quote(diagnosticLogPath));
            const std::string text = command.str();
            std::cerr << text << '\n';
            MirrorRuntimeDiagnosticToPlatformLog(text);
        } catch (...) {
            // Startup diagnostics are best-effort and must not make the game unlaunchable.
        }
    }

    void RecordRuntimeWorkload(const std::string_view mode, const std::string_view viewpoint,
                               const std::string_view capturePath,
                               const std::string_view benchmarkPath,
                               const std::string_view startupReportPath) noexcept
    {
        try {
            std::ostringstream stream;
            stream << "island-simulator: workload mode=" << Quote(mode)
                   << " viewpoint=" << Quote(viewpoint)
                   << " capture=" << Quote(capturePath)
                   << " benchmark=" << Quote(benchmarkPath)
                   << " startup-report=" << Quote(startupReportPath);
            const std::string text = stream.str();
            std::cout << text << '\n';
            MirrorRuntimeDiagnosticToPlatformLog(text);
        } catch (...) {
        }
    }

    void RecordGraphicsRuntimeDiagnostics(
        Microsoft::Xna::Framework::Graphics::GraphicsDevice& device) noexcept
    {
        try {
            constexpr unsigned int GlVendor = 0x1F00U;
            constexpr unsigned int GlRenderer = 0x1F01U;
            constexpr unsigned int GlVersion = 0x1F02U;
            constexpr unsigned int GlShadingLanguageVersion = 0x8B8CU;
            auto& adapter = device.getAdapterProperty();
            const std::string_view rendererName = device.GetGraphicsRendererName();

            std::ostringstream stream;
            stream << "island-simulator: graphics renderer="
                   << Quote(rendererName)
                   << " graphics-api=" << Quote(GraphicsApiName(rendererName))
                   << " implementation=" << Quote("CNA EasyGL")
                   << " platform-api=" << Quote("SDL3")
                   << " os=" << Quote(PlatformName())
                   << " video-driver=" << Quote(VideoDriverName())
                   << " gl-vendor=" << Quote(GlString(GlVendor))
                   << " gl-renderer=" << Quote(GlString(GlRenderer))
                   << " gl-version=" << Quote(GlString(GlVersion))
                   << " glsl-version=" << Quote(GlString(GlShadingLanguageVersion))
                   << " adapter-description=" << Quote(adapter.getDescriptionProperty())
                   << " adapter-device=" << Quote(adapter.getDeviceNameProperty())
                   << " pci-vendor=0x" << std::hex << adapter.getVendorIdProperty()
                   << " pci-device=0x" << adapter.getDeviceIdProperty() << std::dec;
            const std::string graphicsText = stream.str();
            std::cout << graphicsText << '\n';
            MirrorRuntimeDiagnosticToPlatformLog(graphicsText);

            constexpr std::array Names{
                "DISPLAY", "WAYLAND_DISPLAY", "XDG_RUNTIME_DIR", "SDL_VIDEODRIVER",
                "MESA_LOADER_DRIVER_OVERRIDE", "MESA_GL_VERSION_OVERRIDE", "MESA_EXTENSION_OVERRIDE",
                "MESA_GLSL_VERSION_OVERRIDE", "LIBGL_ALWAYS_SOFTWARE", "LIBGL_DRI3_DISABLE",
                "GALLIUM_DRIVER", "GALLIUM_THREAD", "MESA_DEBUG", "AMD_DEBUG", "R600_DEBUG",
                "RADEON_DEBUG", "DRI_PRIME", "CNA_GRAPHICS_RENDERER", "CNA_PLATFORM",
                "ISLAND_GPU_DIAGNOSTICS"};
            std::ostringstream environment;
            environment << "island-simulator: graphics-environment";
            for (const char* name : Names) {
                environment << ' ' << name << '=' << EnvironmentValue(name);
            }
            const std::string environmentText = environment.str();
            std::cout << environmentText << '\n';
            MirrorRuntimeDiagnosticToPlatformLog(environmentText);

            constexpr std::string_view ownership =
                "island-simulator: thread-ownership main thread owns game update, rendering, "
                "content creation, and resource destruction; no game worker threads are created; "
                "when using Mesa/radeonsi, a task named island-sim:cs0 is its asynchronous AMD "
                "command-submission worker";
            std::cout << ownership << '\n';
            MirrorRuntimeDiagnosticToPlatformLog(ownership);
        } catch (const std::exception& error) {
            const std::string text = std::string("island-simulator: graphics diagnostic failed: ")
                + error.what();
            std::cerr << text << '\n';
            MirrorRuntimeDiagnosticToPlatformLog(text);
        } catch (...) {
            MirrorRuntimeDiagnosticToPlatformLog(
                "island-simulator: graphics diagnostic failed with an unknown error");
        }
    }

    void RecordRuntimeThreadInventory() noexcept
    {
#if defined(__linux__) && !defined(__EMSCRIPTEN__)
        try {
            std::vector<std::pair<std::uint64_t, std::string>> tasks;
            const std::filesystem::path directory = "/proc/self/task";
            for (const std::filesystem::directory_entry& entry
                 : std::filesystem::directory_iterator(directory)) {
                if (!entry.is_directory()) continue;
                const std::string idText = entry.path().filename().string();
                std::uint64_t id = 0U;
                try {
                    id = std::stoull(idText);
                } catch (...) {
                    continue;
                }
                std::ifstream nameFile(entry.path() / "comm");
                std::string name;
                std::getline(nameFile, name);
                tasks.emplace_back(id, std::move(name));
            }
            std::ranges::sort(tasks);
            std::ostringstream stream;
            stream << "island-simulator: thread-inventory";
            for (const auto& [id, name] : tasks) {
                stream << " tid=" << id << ':' << Quote(name);
            }
            const std::string text = stream.str();
            std::cout << text << '\n';
            MirrorRuntimeDiagnosticToPlatformLog(text);
        } catch (const std::exception& error) {
            const std::string text = std::string("island-simulator: thread inventory failed: ")
                + error.what();
            std::cerr << text << '\n';
            MirrorRuntimeDiagnosticToPlatformLog(text);
        }
#else
        MirrorRuntimeDiagnosticToPlatformLog(
            "island-simulator: thread inventory is unavailable on this platform");
#endif
    }

    std::string RuntimeDiagnosticLogPath() noexcept
    {
        try {
            std::scoped_lock lock(diagnosticMutex);
            return diagnosticLogPath;
        } catch (...) {
            return {};
        }
    }

    void MirrorRuntimeDiagnosticToPlatformLog(const std::string_view message) noexcept
    {
        AppendPersistentDiagnostic(message);
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
