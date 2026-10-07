#pragma once

#include "IslandSimulator/TouchControls.hpp"

#include <cstddef>
#include <string>
#include <string_view>

namespace island
{
    struct UserSettingsParseResult;

    /** @brief Small, portable set of preferences exposed by the current in-game controls. */
    struct UserSettings
    {
        static constexpr int CurrentSchema = 1;
        static constexpr std::size_t MaximumSerializedBytes = 4096U;

        bool runningSelected = false;
        bool cameraMotionEnabled = true;
        bool audioMuted = false;
        TouchLookSensitivityLevel touchLookSensitivity =
            TouchLookSensitivityLevel::Standard;

        [[nodiscard]] bool IsValid() const noexcept;
        [[nodiscard]] std::string ToText() const;
        [[nodiscard]] static UserSettingsParseResult Parse(std::string_view text);

        [[nodiscard]] bool operator==(const UserSettings&) const noexcept = default;
    };

    /** @brief Non-throwing parse result with an actionable validation diagnostic. */
    struct UserSettingsParseResult
    {
        UserSettings settings;
        std::string error;

        [[nodiscard]] bool IsValid() const noexcept { return error.empty(); }
    };
}
