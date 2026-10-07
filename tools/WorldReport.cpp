#include "IslandSimulator/BirdFlock.hpp"
#include "IslandSimulator/CrabColony.hpp"
#include "IslandSimulator/DistantSceneryLayout.hpp"
#include "IslandSimulator/FishSchools.hpp"
#include "IslandSimulator/GrassLayout.hpp"
#include "IslandSimulator/IslandTerrain.hpp"
#include "IslandSimulator/JungleLayout.hpp"
#include "IslandSimulator/MonkeyTroop.hpp"
#include "IslandSimulator/NaturalPropLayout.hpp"
#include "IslandSimulator/SharkPatrol.hpp"
#include "IslandSimulator/ShipwreckLayout.hpp"
#include "IslandSimulator/VegetationLayout.hpp"
#include "IslandSimulator/WorldCollisionGrid.hpp"
#include "IslandSimulator/WorldIdentity.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <locale>
#include <numbers>
#include <ostream>
#include <sstream>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace
{
    constexpr float TerrainReportSpacing = 8.0f;
    constexpr int TerrainReportExtent = 40;
    constexpr std::size_t TerrainReportSampleCount = static_cast<std::size_t>(
        (TerrainReportExtent * 2 + 1) * (TerrainReportExtent * 2 + 1));

    class StableHash final
    {
    public:
        void AddByte(const std::uint8_t value) noexcept
        {
            value_ ^= value;
            value_ *= 1099511628211ULL;
        }

        void AddUint32(const std::uint32_t value) noexcept
        {
            for (int shift = 0; shift < 32; shift += 8) {
                AddByte(static_cast<std::uint8_t>((value >> shift) & 0xffU));
            }
        }

        void AddSize(const std::size_t value) noexcept
        {
            const auto fixed = static_cast<std::uint64_t>(value);
            for (int shift = 0; shift < 64; shift += 8) {
                AddByte(static_cast<std::uint8_t>((fixed >> shift) & 0xffU));
            }
        }

        void AddFloat(const float value) noexcept
        {
            AddUint32(std::bit_cast<std::uint32_t>(value));
        }

        template<typename Enum>
        void AddEnum(const Enum value) noexcept
        {
            using Underlying = std::underlying_type_t<Enum>;
            AddUint32(static_cast<std::uint32_t>(static_cast<Underlying>(value)));
        }

        [[nodiscard]] std::uint64_t Value() const noexcept { return value_; }

    private:
        std::uint64_t value_ = 14695981039346656037ULL;
    };

    struct WorldSnapshot final
    {
        island::IslandTerrain terrain;
        island::DistantSceneryLayout distantScenery;
        island::BirdFlock birds{terrain};
        island::CrabColony crabs{terrain};
        island::FishSchools fish{terrain};
        island::SharkPatrol shark{terrain};
        island::MonkeyTroop monkeys{terrain};
        island::GrassLayout grass{terrain};
        island::VegetationLayout palms{terrain};
        island::JungleLayout jungle{terrain};
        island::NaturalPropLayout props{terrain, palms, jungle};
        island::ShipwreckLayout shipwreck{terrain};
        std::vector<island::CircularObstacle> worldObstacles;
        island::WorldCollisionGrid collisionGrid;

        WorldSnapshot()
        {
            worldObstacles.reserve(palms.Obstacles().size() + jungle.Obstacles().size()
                                   + props.Obstacles().size()
                                   + shipwreck.Obstacles().size());
            worldObstacles.insert(worldObstacles.end(), palms.Obstacles().begin(),
                                  palms.Obstacles().end());
            worldObstacles.insert(worldObstacles.end(), jungle.Obstacles().begin(),
                                  jungle.Obstacles().end());
            worldObstacles.insert(worldObstacles.end(), props.Obstacles().begin(),
                                  props.Obstacles().end());
            worldObstacles.insert(worldObstacles.end(), shipwreck.Obstacles().begin(),
                                  shipwreck.Obstacles().end());
            collisionGrid.Reset(worldObstacles);
            monkeys.SetObstacles(collisionGrid.Obstacles());
        }
    };

    struct TerrainStatistics final
    {
        float minimumShoreRadius = std::numeric_limits<float>::max();
        float maximumShoreRadius = std::numeric_limits<float>::lowest();
        float minimumHeight = std::numeric_limits<float>::max();
        float maximumHeight = std::numeric_limits<float>::lowest();
        std::array<std::size_t, 7U> surfaceCounts{};
        std::size_t sampleCount = 0U;
        bool finite = true;
    };

    void HashPosition(StableHash& hash, const float x, const float y, const float z) noexcept
    {
        hash.AddFloat(x);
        hash.AddFloat(y);
        hash.AddFloat(z);
    }

    template<typename Placement>
    void HashScaledPlacements(StableHash& hash,
                              const std::span<const Placement> placements) noexcept
    {
        hash.AddSize(placements.size());
        for (const Placement& placement : placements) {
            HashPosition(hash, placement.x, placement.y, placement.z);
            hash.AddFloat(placement.yawRadians);
            hash.AddFloat(placement.scale);
            hash.AddByte(placement.variant);
        }
    }

    void HashSnapshotTerrain(StableHash& hash, const island::IslandTerrain& terrain) noexcept
    {
        for (int z = -TerrainReportExtent; z <= TerrainReportExtent; ++z) {
            for (int x = -TerrainReportExtent; x <= TerrainReportExtent; ++x) {
                const island::TerrainSample sample = terrain.Sample(
                    static_cast<float>(x) * TerrainReportSpacing,
                    static_cast<float>(z) * TerrainReportSpacing);
                hash.AddFloat(sample.height);
                hash.AddFloat(sample.signedShoreDistance);
                HashPosition(hash, sample.normal.x, sample.normal.y, sample.normal.z);
                hash.AddFloat(sample.slopeRadians);
                hash.AddEnum(sample.surface);
            }
        }
    }

    [[nodiscard]] std::uint64_t Fingerprint(const WorldSnapshot& world) noexcept
    {
        StableHash hash;
        const island::WorldSeedSet seeds = island::AuthoritativeWorldSeeds();
        hash.AddUint32(seeds.palms);
        hash.AddUint32(seeds.grass);
        hash.AddUint32(seeds.jungle);
        hash.AddUint32(seeds.naturalProps);
        HashSnapshotTerrain(hash, world.terrain);

        HashScaledPlacements(hash, std::span(world.palms.Palms()));

        hash.AddSize(world.grass.Patches().size());
        for (const island::GrassPlacement& placement : world.grass.Patches()) {
            HashPosition(hash, placement.x, placement.y, placement.z);
            hash.AddFloat(placement.yawRadians);
            hash.AddFloat(placement.scale);
            hash.AddFloat(placement.spreadScale);
            hash.AddFloat(placement.windPhase);
            hash.AddFloat(placement.colorVariation);
            hash.AddFloat(placement.biomeTint);
            hash.AddByte(placement.pathScale ? 1U : 0U);
        }

        HashScaledPlacements(hash, std::span(world.jungle.Canopy()));
        HashScaledPlacements(hash, std::span(world.jungle.Understory()));
        HashScaledPlacements(hash, std::span(world.jungle.Bushes()));
        HashScaledPlacements(hash, std::span(world.jungle.GroundPlants()));

        hash.AddSize(world.props.Rocks().size());
        for (const island::RockPlacement& rock : world.props.Rocks()) {
            HashPosition(hash, rock.x, rock.y, rock.z);
            hash.AddFloat(rock.yawRadians);
            hash.AddFloat(rock.scaleX);
            hash.AddFloat(rock.scaleY);
            hash.AddFloat(rock.scaleZ);
            hash.AddByte(rock.variant);
            hash.AddByte(rock.solid ? 1U : 0U);
            hash.AddByte(rock.offshore ? 1U : 0U);
            hash.AddByte(rock.clearingLandmark ? 1U : 0U);
        }

        hash.AddSize(world.props.FallenLogs().size());
        for (const island::FallenLogPlacement& log : world.props.FallenLogs()) {
            HashPosition(hash, log.x, log.y, log.z);
            hash.AddFloat(log.yawRadians);
            hash.AddFloat(log.length);
            hash.AddFloat(log.radius);
            hash.AddByte(log.variant);
            hash.AddByte(log.clearingLandmark ? 1U : 0U);
        }

        hash.AddSize(world.props.Driftwood().size());
        for (const island::DriftwoodPlacement& wood : world.props.Driftwood()) {
            HashPosition(hash, wood.x, wood.y, wood.z);
            hash.AddFloat(wood.yawRadians);
            hash.AddFloat(wood.length);
            hash.AddFloat(wood.radius);
            hash.AddByte(wood.variant);
            hash.AddByte(wood.coveLandmark ? 1U : 0U);
        }
        HashScaledPlacements(hash, std::span(world.props.BeachDetails()));
        HashScaledPlacements(hash, std::span(world.props.FallenFronds()));

        const island::ShipwreckPlacement& ship = world.shipwreck.Ship();
        HashPosition(hash, ship.x, ship.y, ship.z);
        hash.AddFloat(ship.yawRadians);
        hash.AddFloat(ship.heelRadians);
        hash.AddFloat(ship.pitchRadians);
        hash.AddFloat(ship.length);
        hash.AddFloat(ship.beam);
        hash.AddFloat(ship.hullHeight);
        hash.AddSize(world.shipwreck.Debris().size());
        for (const island::ShipwreckDebrisPlacement& debris : world.shipwreck.Debris()) {
            HashPosition(hash, debris.x, debris.y, debris.z);
            hash.AddFloat(debris.yawRadians);
            hash.AddFloat(debris.length);
            hash.AddFloat(debris.width);
            hash.AddEnum(debris.kind);
        }

        hash.AddSize(world.distantScenery.Islets().size());
        for (const island::DistantIsletPlacement& islet : world.distantScenery.Islets()) {
            hash.AddFloat(islet.x);
            hash.AddFloat(islet.z);
            hash.AddFloat(islet.radiusX);
            hash.AddFloat(islet.radiusZ);
            hash.AddFloat(islet.height);
            hash.AddFloat(islet.yawRadians);
            hash.AddByte(islet.variant);
        }

        hash.AddSize(world.worldObstacles.size());
        for (const island::CircularObstacle& obstacle : world.worldObstacles) {
            hash.AddFloat(obstacle.x);
            hash.AddFloat(obstacle.z);
            hash.AddFloat(obstacle.radius);
        }

        hash.AddSize(world.birds.Birds().size());
        for (const island::BirdState& bird : world.birds.Birds()) {
            HashPosition(hash, bird.x, bird.y, bird.z);
            hash.AddFloat(bird.headingRadians);
            hash.AddFloat(bird.speed);
            hash.AddFloat(bird.scale);
            hash.AddEnum(bird.family);
            hash.AddByte(bird.colorVariant);
        }

        hash.AddSize(world.crabs.Crabs().size());
        for (std::size_t index = 0; index < world.crabs.Crabs().size(); ++index) {
            const island::CrabState& crab = world.crabs.Crabs()[index];
            const island::CrabHomeRange& home = world.crabs.Homes()[index];
            HashPosition(hash, crab.x, crab.y, crab.z);
            hash.AddFloat(crab.bodyYawRadians);
            hash.AddFloat(crab.travelHeadingRadians);
            hash.AddFloat(crab.scale);
            hash.AddEnum(crab.behavior);
            hash.AddByte(crab.colorVariant);
            hash.AddFloat(home.x);
            hash.AddFloat(home.z);
            hash.AddFloat(home.radius);
        }

        hash.AddSize(world.fish.Fish().size());
        for (const island::FishState& fish : world.fish.Fish()) {
            HashPosition(hash, fish.x, fish.y, fish.z);
            hash.AddFloat(fish.headingRadians);
            hash.AddFloat(fish.speed);
            hash.AddFloat(fish.scale);
            hash.AddByte(fish.schoolIndex);
            hash.AddByte(fish.colorVariant);
        }

        const island::SharkState& shark = world.shark.State();
        HashPosition(hash, shark.x, shark.y, shark.z);
        hash.AddFloat(shark.headingRadians);
        hash.AddFloat(shark.scale);
        hash.AddFloat(shark.targetX);
        hash.AddFloat(shark.targetZ);

        hash.AddSize(world.monkeys.Monkeys().size());
        for (const island::MonkeyState& monkey : world.monkeys.Monkeys()) {
            HashPosition(hash, monkey.x, monkey.y, monkey.z);
            hash.AddFloat(monkey.headingRadians);
            hash.AddFloat(monkey.scale);
            hash.AddFloat(monkey.homeX);
            hash.AddFloat(monkey.homeZ);
            hash.AddByte(monkey.colorVariant);
        }
        return hash.Value();
    }

    [[nodiscard]] TerrainStatistics MeasureTerrain(const island::IslandTerrain& terrain) noexcept
    {
        TerrainStatistics statistics;
        for (int index = 0; index < 720; ++index) {
            const float angle = static_cast<float>(index)
                * (2.0f * std::numbers::pi_v<float> / 720.0f);
            const float radius = terrain.ShoreRadius(angle);
            statistics.minimumShoreRadius = std::min(statistics.minimumShoreRadius, radius);
            statistics.maximumShoreRadius = std::max(statistics.maximumShoreRadius, radius);
            statistics.finite = statistics.finite && std::isfinite(radius);
        }

        for (int z = -TerrainReportExtent; z <= TerrainReportExtent; ++z) {
            for (int x = -TerrainReportExtent; x <= TerrainReportExtent; ++x) {
                const island::TerrainSample sample = terrain.Sample(
                    static_cast<float>(x) * TerrainReportSpacing,
                    static_cast<float>(z) * TerrainReportSpacing);
                statistics.minimumHeight = std::min(statistics.minimumHeight, sample.height);
                statistics.maximumHeight = std::max(statistics.maximumHeight, sample.height);
                const auto surfaceIndex = static_cast<std::size_t>(sample.surface);
                if (surfaceIndex < statistics.surfaceCounts.size()) {
                    ++statistics.surfaceCounts[surfaceIndex];
                } else {
                    statistics.finite = false;
                }
                statistics.finite = statistics.finite
                    && std::isfinite(sample.height)
                    && std::isfinite(sample.signedShoreDistance)
                    && std::isfinite(sample.normal.x)
                    && std::isfinite(sample.normal.y)
                    && std::isfinite(sample.normal.z)
                    && std::isfinite(sample.slopeRadians);
                ++statistics.sampleCount;
            }
        }
        return statistics;
    }

    template<typename Placement>
    [[nodiscard]] bool ScaledPlacementsAreFinite(
        const std::span<const Placement> placements) noexcept
    {
        for (const Placement& placement : placements) {
            if (!std::isfinite(placement.x) || !std::isfinite(placement.y)
                || !std::isfinite(placement.z) || !std::isfinite(placement.yawRadians)
                || !std::isfinite(placement.scale) || placement.scale <= 0.0f) {
                return false;
            }
        }
        return true;
    }

    template<typename... Values>
    [[nodiscard]] bool AllFinite(const Values... values) noexcept
    {
        return (... && std::isfinite(values));
    }

    [[nodiscard]] bool ValidateWorld(const WorldSnapshot& world,
                                     const TerrainStatistics& terrain,
                                     const std::uint64_t fingerprint,
                                     const std::uint64_t repeatedFingerprint) noexcept
    {
        const bool expectedPopulations = world.palms.Palms().size() >= 180U
            && world.palms.Palms().size() <= 520U
            && world.grass.Patches().size() >= 10000U
            && world.grass.Patches().size() <= 40000U
            && world.jungle.Canopy().size() >= 100U
            && world.jungle.Canopy().size() <= 600U
            && world.jungle.Understory().size() >= 800U
            && world.jungle.Understory().size() <= 4000U
            && world.jungle.Bushes().size() >= 150U
            && world.jungle.Bushes().size() <= 1400U
            && world.jungle.GroundPlants().size() >= 600U
            && world.jungle.GroundPlants().size() <= 5000U
            && world.props.Rocks().size() >= 30U
            && world.props.Rocks().size() <= 500U
            && world.props.FallenLogs().size() >= 6U
            && world.props.FallenLogs().size() <= 80U
            && world.props.Driftwood().size() >= 12U
            && world.props.Driftwood().size() <= 100U
            && world.props.BeachDetails().size() >= 120U
            && world.props.BeachDetails().size() <= 600U
            && world.props.FallenFronds().size() >= 30U
            && world.props.FallenFronds().size() <= 180U;
        const bool derivedCountsAgree = world.palms.Obstacles().size() == world.palms.Palms().size()
            && world.jungle.Obstacles().size() == world.jungle.Canopy().size()
            && world.collisionGrid.Obstacles().size() == world.worldObstacles.size()
            && world.birds.Birds().size() == island::BirdFlock::BirdCount
            && world.crabs.Crabs().size() == island::CrabColony::CrabCount
            && world.crabs.Homes().size() == island::CrabColony::CrabCount
            && world.fish.Fish().size() == island::FishSchools::FishCount
            && world.fish.Schools().size() == island::FishSchools::SchoolCount
            && world.monkeys.Monkeys().size() == island::MonkeyTroop::MonkeyCount
            && world.shipwreck.Debris().size() == island::ShipwreckLayout::DebrisCount
            && world.shipwreck.Obstacles().size()
                == island::ShipwreckLayout::CollisionProxyCount
            && world.distantScenery.Islets().size()
                == island::DistantSceneryLayout::IsletCount;
        const bool finitePlacements = ScaledPlacementsAreFinite(
                std::span(world.palms.Palms()))
            && ScaledPlacementsAreFinite(std::span(world.jungle.Canopy()))
            && ScaledPlacementsAreFinite(std::span(world.jungle.Understory()))
            && ScaledPlacementsAreFinite(std::span(world.jungle.Bushes()))
            && ScaledPlacementsAreFinite(std::span(world.jungle.GroundPlants()))
            && ScaledPlacementsAreFinite(std::span(world.props.BeachDetails()))
            && ScaledPlacementsAreFinite(std::span(world.props.FallenFronds()));
        bool finiteSpecializedData = true;
        for (const island::GrassPlacement& placement : world.grass.Patches()) {
            finiteSpecializedData = finiteSpecializedData
                && AllFinite(placement.x, placement.y, placement.z, placement.yawRadians,
                             placement.scale, placement.spreadScale, placement.windPhase,
                             placement.colorVariation, placement.biomeTint)
                && placement.scale > 0.0f && placement.spreadScale > 0.0f
                && placement.biomeTint >= 0.0f && placement.biomeTint <= 1.0f;
        }
        for (const island::RockPlacement& rock : world.props.Rocks()) {
            finiteSpecializedData = finiteSpecializedData
                && AllFinite(rock.x, rock.y, rock.z, rock.yawRadians,
                             rock.scaleX, rock.scaleY, rock.scaleZ)
                && rock.scaleX > 0.0f && rock.scaleY > 0.0f && rock.scaleZ > 0.0f;
        }
        for (const island::FallenLogPlacement& log : world.props.FallenLogs()) {
            finiteSpecializedData = finiteSpecializedData
                && AllFinite(log.x, log.y, log.z, log.yawRadians, log.length, log.radius)
                && log.length > 0.0f && log.radius > 0.0f;
        }
        for (const island::DriftwoodPlacement& wood : world.props.Driftwood()) {
            finiteSpecializedData = finiteSpecializedData
                && AllFinite(wood.x, wood.y, wood.z, wood.yawRadians,
                             wood.length, wood.radius)
                && wood.length > 0.0f && wood.radius > 0.0f;
        }
        const island::ShipwreckPlacement& ship = world.shipwreck.Ship();
        finiteSpecializedData = finiteSpecializedData
            && AllFinite(ship.x, ship.y, ship.z, ship.yawRadians, ship.heelRadians,
                         ship.pitchRadians, ship.length, ship.beam, ship.hullHeight)
            && ship.length == island::ShipwreckLayout::ShipLength
            && ship.beam == island::ShipwreckLayout::ShipBeam
            && ship.hullHeight == island::ShipwreckLayout::ShipHullHeight
            && world.terrain.SignedShoreDistance(ship.x, ship.z) < -3.0f
            && world.terrain.SignedShoreDistance(ship.x, ship.z) > -12.0f;
        for (const island::ShipwreckDebrisPlacement& debris : world.shipwreck.Debris()) {
            finiteSpecializedData = finiteSpecializedData
                && AllFinite(debris.x, debris.y, debris.z, debris.yawRadians,
                             debris.length, debris.width)
                && debris.length > 0.0f && debris.width > 0.0f;
        }
        for (const island::CircularObstacle& obstacle : world.worldObstacles) {
            finiteSpecializedData = finiteSpecializedData
                && AllFinite(obstacle.x, obstacle.z, obstacle.radius)
                && obstacle.radius > 0.0f;
        }
        for (const island::DistantIsletPlacement& islet : world.distantScenery.Islets()) {
            finiteSpecializedData = finiteSpecializedData
                && AllFinite(islet.x, islet.z, islet.radiusX, islet.radiusZ,
                             islet.height, islet.yawRadians)
                && islet.radiusX > 0.0f && islet.radiusZ > 0.0f && islet.height > 0.0f;
        }
        for (const island::BirdState& bird : world.birds.Birds()) {
            finiteSpecializedData = finiteSpecializedData
                && AllFinite(bird.x, bird.y, bird.z, bird.headingRadians,
                             bird.speed, bird.scale);
        }
        for (std::size_t index = 0; index < world.crabs.Crabs().size(); ++index) {
            const island::CrabState& crab = world.crabs.Crabs()[index];
            const island::CrabHomeRange& home = world.crabs.Homes()[index];
            const island::TerrainSample sample = world.terrain.Sample(crab.x, crab.z);
            finiteSpecializedData = finiteSpecializedData
                && AllFinite(crab.x, crab.y, crab.z, crab.bodyYawRadians,
                             crab.travelHeadingRadians, crab.speed, crab.legPhase,
                             crab.clawPose, crab.scale, home.x, home.z, home.radius)
                && crab.scale > 0.0f && home.radius > 0.0f
                && sample.signedShoreDistance >= island::CrabColony::MinimumShoreDistance
                && sample.signedShoreDistance <= island::CrabColony::MaximumShoreDistance;
        }
        for (const island::FishState& fish : world.fish.Fish()) {
            finiteSpecializedData = finiteSpecializedData
                && AllFinite(fish.x, fish.y, fish.z, fish.headingRadians,
                             fish.speed, fish.scale);
        }
        const island::SharkState& shark = world.shark.State();
        finiteSpecializedData = finiteSpecializedData
            && AllFinite(shark.x, shark.y, shark.z, shark.headingRadians, shark.scale,
                         shark.targetX, shark.targetZ);
        for (const island::MonkeyState& monkey : world.monkeys.Monkeys()) {
            finiteSpecializedData = finiteSpecializedData
                && AllFinite(monkey.x, monkey.y, monkey.z, monkey.headingRadians,
                             monkey.scale, monkey.homeX, monkey.homeZ);
        }
        return terrain.finite && terrain.sampleCount == TerrainReportSampleCount
            && terrain.minimumShoreRadius > 170.0f
            && terrain.maximumShoreRadius < 300.0f
            && expectedPopulations && derivedCountsAgree
            && finitePlacements && finiteSpecializedData
            && fingerprint == repeatedFingerprint;
    }

    [[nodiscard]] std::size_t CountOffshoreRocks(const WorldSnapshot& world) noexcept
    {
        std::size_t count = 0U;
        for (const island::RockPlacement& rock : world.props.Rocks()) {
            if (rock.offshore) ++count;
        }
        return count;
    }

    [[nodiscard]] std::string FingerprintString(const std::uint64_t fingerprint)
    {
        std::ostringstream output;
        output << std::hex << std::setfill('0') << std::setw(16) << fingerprint;
        return output.str();
    }

    void WriteReport(std::ostream& output, const WorldSnapshot& world,
                     const TerrainStatistics& terrain, const std::uint64_t fingerprint,
                     const bool valid)
    {
        const island::WorldSeedSet seeds = island::AuthoritativeWorldSeeds();
        output.imbue(std::locale::classic());
        output << std::fixed << std::setprecision(4)
               << "{\n"
               << "  \"schemaVersion\": 1,\n"
               << "  \"valid\": " << (valid ? "true" : "false") << ",\n"
               << "  \"fingerprint\": \"" << FingerprintString(fingerprint) << "\",\n"
               << "  \"seeds\": {\n"
               << "    \"palms\": \"0x" << std::hex << seeds.palms
               << "\",\n"
               << "    \"grass\": \"0x" << seeds.grass << "\",\n"
               << "    \"jungle\": \"0x" << seeds.jungle << "\",\n"
               << "    \"naturalProps\": \"0x" << seeds.naturalProps
               << "\"\n"
               << std::dec << "  },\n"
               << "  \"terrain\": {\n"
               << "    \"nominalDiameterMeters\": "
               << island::IslandTerrain::NominalDiameter << ",\n"
               << "    \"sampleSpacingMeters\": " << TerrainReportSpacing << ",\n"
               << "    \"sampleCount\": " << terrain.sampleCount << ",\n"
               << "    \"minimumShoreRadiusMeters\": " << terrain.minimumShoreRadius
               << ",\n"
               << "    \"maximumShoreRadiusMeters\": " << terrain.maximumShoreRadius
               << ",\n"
               << "    \"minimumHeightMeters\": " << terrain.minimumHeight << ",\n"
               << "    \"maximumHeightMeters\": " << terrain.maximumHeight << ",\n"
               << "    \"surfaceSamples\": {\n"
               << "      \"deepSeabed\": " << terrain.surfaceCounts[0] << ",\n"
               << "      \"shallowSeabed\": " << terrain.surfaceCounts[1] << ",\n"
               << "      \"wetSand\": " << terrain.surfaceCounts[2] << ",\n"
               << "      \"drySand\": " << terrain.surfaceCounts[3] << ",\n"
               << "      \"coastalGround\": " << terrain.surfaceCounts[4] << ",\n"
               << "      \"interiorGround\": " << terrain.surfaceCounts[5] << ",\n"
               << "      \"steepGround\": " << terrain.surfaceCounts[6] << "\n"
               << "    }\n"
               << "  },\n"
               << "  \"placements\": {\n"
               << "    \"palms\": " << world.palms.Palms().size() << ",\n"
               << "    \"grassPatches\": " << world.grass.Patches().size() << ",\n"
               << "    \"canopyTrees\": " << world.jungle.Canopy().size() << ",\n"
               << "    \"understoryPlants\": " << world.jungle.Understory().size() << ",\n"
               << "    \"bushes\": " << world.jungle.Bushes().size() << ",\n"
               << "    \"groundPlants\": " << world.jungle.GroundPlants().size() << ",\n"
               << "    \"rocks\": " << world.props.Rocks().size() << ",\n"
               << "    \"offshoreRocks\": " << CountOffshoreRocks(world) << ",\n"
               << "    \"fallenLogs\": " << world.props.FallenLogs().size() << ",\n"
               << "    \"driftwood\": " << world.props.Driftwood().size() << ",\n"
               << "    \"beachDetailClusters\": "
               << world.props.BeachDetails().size() << ",\n"
               << "    \"fallenFronds\": " << world.props.FallenFronds().size() << ",\n"
               << "    \"distantIslets\": " << world.distantScenery.Islets().size() << ",\n"
               << "    \"shipwrecks\": 1,\n"
               << "    \"shipwreckDebris\": " << world.shipwreck.Debris().size() << "\n"
               << "  },\n"
               << "  \"shipwreck\": {\n"
               << "    \"lengthMeters\": " << world.shipwreck.Ship().length << ",\n"
               << "    \"beamMeters\": " << world.shipwreck.Ship().beam << ",\n"
               << "    \"signedShoreDistanceMeters\": "
               << world.terrain.SignedShoreDistance(world.shipwreck.Ship().x,
                                                     world.shipwreck.Ship().z) << ",\n"
               << "    \"collisionProxies\": "
               << world.shipwreck.Obstacles().size() << "\n"
               << "  },\n"
               << "  \"collision\": {\n"
               << "    \"staticObstacles\": " << world.worldObstacles.size() << ",\n"
               << "    \"occupiedCells\": " << world.collisionGrid.CellCount() << ",\n"
               << "    \"cellSizeMeters\": " << island::WorldCollisionGrid::CellSize << "\n"
               << "  },\n"
               << "  \"wildlife\": {\n"
               << "    \"birds\": " << world.birds.Birds().size() << ",\n"
               << "    \"beachCrabs\": " << world.crabs.Crabs().size() << ",\n"
               << "    \"fishSchools\": " << world.fish.Schools().size() << ",\n"
               << "    \"fish\": " << world.fish.Fish().size() << ",\n"
               << "    \"sharks\": 1,\n"
               << "    \"monkeys\": " << world.monkeys.Monkeys().size() << "\n"
               << "  }\n"
               << "}\n";
    }
}

int main(const int argc, char* argv[])
{
    std::string outputPath;
    if (argc == 3 && std::string_view(argv[1]) == "--output") {
        outputPath = argv[2];
    } else if (argc != 1) {
        std::cerr << "usage: island-world-report [--output report.json]\n";
        return 2;
    }

    const WorldSnapshot world;
    const WorldSnapshot repeatedWorld;
    const TerrainStatistics terrain = MeasureTerrain(world.terrain);
    const std::uint64_t fingerprint = Fingerprint(world);
    const bool valid = ValidateWorld(
        world, terrain, fingerprint, Fingerprint(repeatedWorld));

    if (outputPath.empty()) {
        WriteReport(std::cout, world, terrain, fingerprint, valid);
    } else {
        std::ofstream output(outputPath, std::ios::binary | std::ios::trunc);
        if (!output) {
            std::cerr << "island-simulator: cannot write world report to " << outputPath << '\n';
            return 1;
        }
        WriteReport(output, world, terrain, fingerprint, valid);
        if (!output) {
            std::cerr << "island-simulator: failed while writing world report to "
                      << outputPath << '\n';
            return 1;
        }
    }

    std::ostream& status = outputPath.empty() ? std::cerr : std::cout;
    status << "island-simulator: world report " << (valid ? "valid" : "invalid")
           << ", fingerprint " << FingerprintString(fingerprint) << '\n';
    return valid ? 0 : 1;
}
