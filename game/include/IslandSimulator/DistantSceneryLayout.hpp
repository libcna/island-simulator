#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace island
{
    /** @brief One low, unreachable horizon islet used only for distant composition. */
    struct DistantIsletPlacement
    {
        float x = 0.0f;
        float z = 0.0f;
        float radiusX = 0.0f;
        float radiusZ = 0.0f;
        float height = 0.0f;
        float yawRadians = 0.0f;
        std::uint8_t variant = 0U;
    };

    /** @brief Deterministic low-cost scenery placed far beyond the swimming boundary. */
    class DistantSceneryLayout final
    {
    public:
        static constexpr std::size_t IsletCount = 5U;
        static constexpr std::uint8_t VariantCount = 4U; // 3 is a low treeless sandbar
        static constexpr std::uint8_t SandbarVariant = 3U;

        DistantSceneryLayout() noexcept;

        [[nodiscard]] const std::array<DistantIsletPlacement, IsletCount>& Islets()
            const noexcept
        {
            return islets_;
        }

    private:
        std::array<DistantIsletPlacement, IsletCount> islets_;
    };
}
