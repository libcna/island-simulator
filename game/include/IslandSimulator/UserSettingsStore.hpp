#pragma once

#include "IslandSimulator/UserSettings.hpp"

#include <memory>
#include <string>

namespace island
{
    /** @brief Result of loading a primary settings file and its recovery copy. */
    struct UserSettingsLoadResult
    {
        UserSettings settings;
        bool loaded = false;
        bool usedBackup = false;
        std::string warning;
    };

    /** @brief CNA StorageDevice adapter for portable, recoverable user preferences. */
    class UserSettingsStore final
    {
    public:
        [[nodiscard]] static std::unique_ptr<UserSettingsStore> Open(std::string& error);

        ~UserSettingsStore();
        UserSettingsStore(const UserSettingsStore&) = delete;
        UserSettingsStore& operator=(const UserSettingsStore&) = delete;
        UserSettingsStore(UserSettingsStore&&) = delete;
        UserSettingsStore& operator=(UserSettingsStore&&) = delete;

        [[nodiscard]] UserSettingsLoadResult Load();
        [[nodiscard]] bool Save(const UserSettings& settings, std::string& error);
        [[nodiscard]] const std::string& Location() const noexcept { return location_; }

    private:
        class Impl;
        UserSettingsStore(std::unique_ptr<Impl> impl, std::string location);

        std::unique_ptr<Impl> impl_;
        std::string location_;
    };
}
