#pragma once

#include "IslandSimulator/WorldCollision.hpp"
#include "IslandSimulator/CanopyProfile.hpp"

#include <cstdint>
#include <vector>

namespace island
{
    class IslandTerrain;

    /** @brief One deterministic broadleaf canopy-tree placement. */
    struct CanopyPlacement
    {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
        float yawRadians = 0.0f;
        float scale = 1.0f;
        std::uint8_t variant = 0U;
    };

    /** @brief One deterministic fern or broadleaf understory placement. */
    struct UnderstoryPlacement
    {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
        float yawRadians = 0.0f;
        float scale = 1.0f;
        std::uint8_t variant = 0U;
    };

    /** @brief One low non-solid bush that bridges grass and medium understory height. */
    struct BushPlacement
    {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
        float yawRadians = 0.0f;
        float scale = 1.0f;
        std::uint8_t variant = 0U;
    };

    /** @brief One small non-solid rosette or forest-floor plant. */
    struct GroundPlantPlacement
    {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
        float yawRadians = 0.0f;
        float scale = 1.0f;
        std::uint8_t variant = 0U;
        bool wetBank = false;
    };

    /** @brief Bounded related floor plants near one canopy tree; indices refer to GroundPlants. */
    struct ForestFloorColony
    {
        std::size_t canopyIndex = 0;
        std::size_t firstPlant = 0;
        std::size_t plantCount = 0;
        float x = 0, z = 0, angle = 0, majorRadius = 0, minorRadius = 0;
        bool operator==(const ForestFloorColony&) const = default;
    };

    /** @brief Renderer-independent layout for the island's layered interior jungle. */
    class JungleLayout final
    {
    public:
        static constexpr std::uint32_t DefaultSeed = 0x43b0a7d1U;
        static constexpr int CanopyVariantCount = CanopyFormCount;
        static constexpr int UnderstoryVariantCount = 4;
        static constexpr int BushVariantCount = 3;
        static constexpr int GroundPlantVariantCount = 3;
        static constexpr float CanopyMinimumScale = 0.78f;
        static constexpr float CanopyMaximumScale = 1.28f;
        // A few emergents rise above the closed canopy only inside dense stands.
        static constexpr float CanopyEmergentMinimumScale = 1.60f;
        static constexpr float CanopyEmergentScale = 1.85f;
        static constexpr float CanopyEmergentChance = 0.09f;
        static constexpr float CanopyStandThreshold = 0.60f;
        static constexpr float UnderstoryMinimumScale = 0.72f;
        static constexpr float UnderstoryMaximumScale = 1.34f;
        static constexpr float BushMinimumScale = 0.68f;
        static constexpr float BushMaximumScale = 1.26f;
        static constexpr float GroundPlantMinimumScale = 0.58f;
        static constexpr float GroundPlantMaximumScale = 1.22f;
        // Includes the shared wind envelope; renderer startup validates both LOD meshes.
        static constexpr float BushFootprintRadius = 2.1f;
        static constexpr float GroundPlantFootprintRadius = 1.4f;
        static constexpr int ClearingColonyCandidates = 12;
        static constexpr int ClearingPlantsPerColony = 6;
        static constexpr int ForestPlantsPerColony = 8;
        static constexpr int StreamBankColonyCount = 20;
        static constexpr int StreamBankPlantsPerColony = 3;
        static constexpr float ForestRootSeparation = 0.72f;

        explicit JungleLayout(const IslandTerrain& terrain,
                              std::uint32_t seed = DefaultSeed);

        [[nodiscard]] const std::vector<CanopyPlacement>& Canopy() const noexcept
        {
            return canopy_;
        }

        [[nodiscard]] const std::vector<UnderstoryPlacement>& Understory() const noexcept
        {
            return understory_;
        }

        [[nodiscard]] const std::vector<BushPlacement>& Bushes() const noexcept
        {
            return bushes_;
        }

        [[nodiscard]] const std::vector<GroundPlantPlacement>& GroundPlants() const noexcept
        {
            return groundPlants_;
        }

        [[nodiscard]] const std::vector<CircularObstacle>& Obstacles() const noexcept
        {
            return obstacles_;
        }

        [[nodiscard]] std::size_t ClearingBushCount() const noexcept { return clearingBushCount_; }
        [[nodiscard]] std::size_t ClearingGroundPlantCount() const noexcept { return clearingGroundPlantCount_; }
        [[nodiscard]] std::size_t ForestGroundPlantCount() const noexcept { return forestGroundPlantCount_; }
        [[nodiscard]] std::size_t StreamBankGroundPlantCount() const noexcept
        {
            return streamBankGroundPlantCount_;
        }
        [[nodiscard]] const std::vector<ForestFloorColony>& ForestColonies() const noexcept { return forestColonies_; }

    private:
        [[nodiscard]] static std::uint32_t Hash(std::int32_t x, std::int32_t z,
                                                std::uint32_t seed) noexcept;
        [[nodiscard]] static float UnitRandom(std::uint32_t value) noexcept;
        [[nodiscard]] static float ClusterNoise(float x, float z,
                                                std::uint32_t seed,
                                                float cellSize) noexcept;

        std::vector<CanopyPlacement> canopy_;
        std::vector<UnderstoryPlacement> understory_;
        std::vector<BushPlacement> bushes_;
        std::vector<GroundPlantPlacement> groundPlants_;
        std::vector<CircularObstacle> obstacles_;
        std::size_t clearingBushCount_ = 0;
        std::size_t clearingGroundPlantCount_ = 0;
        std::size_t forestGroundPlantCount_ = 0;
        std::size_t streamBankGroundPlantCount_ = 0;
        std::vector<ForestFloorColony> forestColonies_;
    };
}
