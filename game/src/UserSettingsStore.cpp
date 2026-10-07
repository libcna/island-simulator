#include "IslandSimulator/UserSettingsStore.hpp"

#include <cstdint>
#include <exception>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "Microsoft/Xna/Framework/Storage/StorageContainer.hpp"
#include "Microsoft/Xna/Framework/Storage/StorageDevice.hpp"
#include "System/IO/FileMode.hpp"
#include "System/IO/Stream.hpp"

namespace island
{
    namespace Storage = Microsoft::Xna::Framework::Storage;

    namespace
    {
        constexpr std::string_view SettingsFileName = "settings.ini";
        constexpr std::string_view BackupFileName = "settings.ini.bak";
        constexpr std::string_view TemporaryFileName = "settings.ini.tmp";
        constexpr std::size_t MaximumReadBytes = UserSettings::MaximumSerializedBytes + 1U;

        [[nodiscard]] std::string ReadText(Storage::StorageContainer& container,
                                           const std::string& name)
        {
            auto stream = container.OpenFile(name, System::IO::FileMode::Open);
            std::string text;
            std::vector<std::uint8_t> chunk(1024U);
            while (text.size() <= MaximumReadBytes) {
                const int read = stream->Read(chunk.data(), 0,
                                              static_cast<int>(chunk.size()));
                if (read <= 0) break;
                text.append(reinterpret_cast<const char*>(chunk.data()),
                            static_cast<std::size_t>(read));
            }
            return text;
        }

        void WriteText(Storage::StorageContainer& container, const std::string& name,
                       const std::string_view text)
        {
            auto stream = container.CreateFile(name);
            stream->Write(reinterpret_cast<const std::uint8_t*>(text.data()), 0,
                          static_cast<int>(text.size()));
            stream->Flush();
        }

        void DeleteIfPresent(Storage::StorageContainer& container,
                             const std::string& name)
        {
            if (container.FileExists(name)) container.DeleteFile(name);
        }
    }

    class UserSettingsStore::Impl
    {
    public:
        std::unique_ptr<Storage::StorageDevice> device;
        std::unique_ptr<Storage::StorageContainer> container;
    };

    UserSettingsStore::UserSettingsStore(std::unique_ptr<Impl> impl, std::string location)
        : impl_(std::move(impl)), location_(std::move(location))
    {
    }

    UserSettingsStore::~UserSettingsStore() = default;

    std::unique_ptr<UserSettingsStore> UserSettingsStore::Open(std::string& error)
    {
        try {
            Storage::StorageDevice::SetAppNameEXT("island-simulator");
            auto impl = std::make_unique<Impl>();
            auto selector = Storage::StorageDevice::BeginShowSelector(nullptr, nullptr);
            impl->device = Storage::StorageDevice::EndShowSelector(selector.get());
            if (!impl->device || !impl->device->getIsConnectedProperty()) {
                error = "no persistent storage device is connected";
                return nullptr;
            }
            auto opening = impl->device->BeginOpenContainer(
                "IslandSimulator", nullptr, nullptr);
            impl->container = impl->device->EndOpenContainer(opening.get());
            if (!impl->container) {
                error = "the IslandSimulator settings container did not open";
                return nullptr;
            }
            std::string location = Storage::StorageDevice::GetStorageRootEXT()
                + "/IslandSimulator/AllPlayers/" + std::string(SettingsFileName);
            error.clear();
            return std::unique_ptr<UserSettingsStore>(
                new UserSettingsStore(std::move(impl), std::move(location)));
        } catch (const std::exception& exception) {
            error = exception.what();
            return nullptr;
        }
    }

    UserSettingsLoadResult UserSettingsStore::Load()
    {
        UserSettingsLoadResult result;
        try {
            if (!impl_->container->FileExists(std::string(SettingsFileName))) return result;
            const UserSettingsParseResult primary = UserSettings::Parse(
                ReadText(*impl_->container, std::string(SettingsFileName)));
            if (primary.IsValid()) {
                result.settings = primary.settings;
                result.loaded = true;
                return result;
            }
            result.warning = "primary settings are invalid: " + primary.error;
            if (!impl_->container->FileExists(std::string(BackupFileName))) return result;
            const UserSettingsParseResult backup = UserSettings::Parse(
                ReadText(*impl_->container, std::string(BackupFileName)));
            if (backup.IsValid()) {
                result.settings = backup.settings;
                result.loaded = true;
                result.usedBackup = true;
            } else {
                result.warning += "; backup is invalid: " + backup.error;
            }
        } catch (const std::exception& exception) {
            result.warning = std::string("settings could not be read: ") + exception.what();
        }
        return result;
    }

    bool UserSettingsStore::Save(const UserSettings& settings, std::string& error)
    {
        if (!settings.IsValid()) {
            error = "refused to save invalid settings";
            return false;
        }
        try {
            const std::string serialized = settings.ToText();
            WriteText(*impl_->container, std::string(TemporaryFileName), serialized);

            if (impl_->container->FileExists(std::string(SettingsFileName))) {
                const std::string previous = ReadText(
                    *impl_->container, std::string(SettingsFileName));
                if (UserSettings::Parse(previous).IsValid()) {
                    WriteText(*impl_->container, std::string(BackupFileName), previous);
                }
                impl_->container->DeleteFile(std::string(SettingsFileName));
            }
            WriteText(*impl_->container, std::string(SettingsFileName), serialized);
            DeleteIfPresent(*impl_->container, std::string(TemporaryFileName));
            error.clear();
            return true;
        } catch (const std::exception& exception) {
            error = exception.what();
            try {
                DeleteIfPresent(*impl_->container, std::string(TemporaryFileName));
            } catch (const std::exception&) {
                // Preserve the original actionable write error.
            }
            return false;
        }
    }
}
