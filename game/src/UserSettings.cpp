#include "IslandSimulator/UserSettings.hpp"

#include <charconv>
#include <cstddef>
#include <string>
#include <system_error>
#include <utility>

namespace island
{
    namespace
    {
        [[nodiscard]] std::string_view Trim(std::string_view value) noexcept
        {
            constexpr std::string_view whitespace = " \t\r";
            const std::size_t first = value.find_first_not_of(whitespace);
            if (first == std::string_view::npos) return {};
            const std::size_t last = value.find_last_not_of(whitespace);
            return value.substr(first, last - first + 1U);
        }

        [[nodiscard]] UserSettingsParseResult Invalid(const std::size_t line,
                                                       std::string message)
        {
            return {UserSettings{}, "line " + std::to_string(line) + ": "
                                        + std::move(message)};
        }

        [[nodiscard]] bool ParseEnabled(const std::string_view value,
                                        bool& destination) noexcept
        {
            if (value == "enabled") {
                destination = true;
                return true;
            }
            if (value == "disabled") {
                destination = false;
                return true;
            }
            return false;
        }
    }

    bool UserSettings::IsValid() const noexcept
    {
        switch (touchLookSensitivity) {
        case TouchLookSensitivityLevel::Low:
        case TouchLookSensitivityLevel::Standard:
        case TouchLookSensitivityLevel::High:
            return true;
        }
        return false;
    }

    std::string UserSettings::ToText() const
    {
        const char* touchSensitivity = "standard";
        switch (touchLookSensitivity) {
        case TouchLookSensitivityLevel::Low:
            touchSensitivity = "low";
            break;
        case TouchLookSensitivityLevel::Standard:
            break;
        case TouchLookSensitivityLevel::High:
            touchSensitivity = "high";
            break;
        }
        return "schema=1\nrun_mode=" + std::string(runningSelected ? "run" : "walk")
            + "\ncamera_motion=" + (cameraMotionEnabled ? "enabled" : "disabled")
            + "\naudio=" + (audioMuted ? "muted" : "enabled")
            + "\ntouch_look=" + touchSensitivity + "\n";
    }

    UserSettingsParseResult UserSettings::Parse(const std::string_view text)
    {
        if (text.size() > MaximumSerializedBytes) {
            return {UserSettings{}, "settings exceed the 4096-byte safety limit"};
        }
        if (text.find('\0') != std::string_view::npos) {
            return {UserSettings{}, "settings contain an embedded NUL byte"};
        }

        UserSettings parsed;
        bool schemaSeen = false;
        bool runModeSeen = false;
        bool cameraMotionSeen = false;
        bool audioSeen = false;
        bool touchLookSeen = false;
        std::size_t lineNumber = 0U;
        std::size_t cursor = 0U;
        while (cursor <= text.size()) {
            ++lineNumber;
            const std::size_t end = text.find('\n', cursor);
            const std::size_t length = end == std::string_view::npos
                ? text.size() - cursor : end - cursor;
            const std::string_view line = Trim(text.substr(cursor, length));
            cursor = end == std::string_view::npos ? text.size() + 1U : end + 1U;
            if (line.empty() || line.front() == '#') continue;

            const std::size_t separator = line.find('=');
            if (separator == std::string_view::npos) {
                return Invalid(lineNumber, "expected key=value");
            }
            const std::string_view key = Trim(line.substr(0U, separator));
            const std::string_view value = Trim(line.substr(separator + 1U));
            if (key.empty() || value.empty()) {
                return Invalid(lineNumber, "key and value must not be empty");
            }

            if (key == "schema") {
                if (schemaSeen) return Invalid(lineNumber, "duplicate schema key");
                schemaSeen = true;
                int schema = 0;
                const auto conversion = std::from_chars(
                    value.data(), value.data() + value.size(), schema);
                if (conversion.ec != std::errc{}
                    || conversion.ptr != value.data() + value.size()) {
                    return Invalid(lineNumber, "schema must be an integer");
                }
                if (schema != CurrentSchema) {
                    return Invalid(lineNumber, "unsupported settings schema "
                                                   + std::to_string(schema));
                }
            } else if (key == "run_mode") {
                if (runModeSeen) return Invalid(lineNumber, "duplicate run_mode key");
                runModeSeen = true;
                if (value == "walk") parsed.runningSelected = false;
                else if (value == "run") parsed.runningSelected = true;
                else return Invalid(lineNumber, "run_mode must be walk or run");
            } else if (key == "camera_motion") {
                if (cameraMotionSeen) {
                    return Invalid(lineNumber, "duplicate camera_motion key");
                }
                cameraMotionSeen = true;
                if (!ParseEnabled(value, parsed.cameraMotionEnabled)) {
                    return Invalid(lineNumber,
                                   "camera_motion must be enabled or disabled");
                }
            } else if (key == "audio") {
                if (audioSeen) return Invalid(lineNumber, "duplicate audio key");
                audioSeen = true;
                if (value == "enabled") parsed.audioMuted = false;
                else if (value == "muted") parsed.audioMuted = true;
                else return Invalid(lineNumber, "audio must be enabled or muted");
            } else if (key == "touch_look") {
                if (touchLookSeen) return Invalid(lineNumber, "duplicate touch_look key");
                touchLookSeen = true;
                if (value == "low") {
                    parsed.touchLookSensitivity = TouchLookSensitivityLevel::Low;
                } else if (value == "standard") {
                    parsed.touchLookSensitivity = TouchLookSensitivityLevel::Standard;
                } else if (value == "high") {
                    parsed.touchLookSensitivity = TouchLookSensitivityLevel::High;
                } else {
                    return Invalid(lineNumber,
                                   "touch_look must be low, standard, or high");
                }
            }
            // Unknown keys are deliberately ignored so a newer writer can preserve basic
            // compatibility while the schema still describes the same setting meanings.
        }

        if (!schemaSeen) return {UserSettings{}, "settings are missing the schema key"};
        if (!runModeSeen || !cameraMotionSeen || !audioSeen || !touchLookSeen) {
            return {UserSettings{}, "settings are missing one or more required keys"};
        }
        if (!parsed.IsValid()) return {UserSettings{}, "settings contain an invalid value"};
        return {parsed, {}};
    }
}
