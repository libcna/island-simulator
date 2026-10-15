#include "IslandSimulator/BirdFlock.hpp"
#include "IslandSimulator/BearMotion.hpp"
#include "IslandSimulator/BoarHerd.hpp"
#include "IslandSimulator/GoatHerd.hpp"
#include "IslandSimulator/SpiderColony.hpp"
#include "IslandSimulator/SpiderWebLayout.hpp"
#include "IslandSimulator/ButterflyMeadow.hpp"
#include "IslandSimulator/GeckoPopulation.hpp"
#include "IslandSimulator/DragonflySwarm.hpp"
#include "IslandSimulator/CrabColony.hpp"
#include "IslandSimulator/DistantSceneryLayout.hpp"
#include "IslandSimulator/DolphinPod.hpp"
#include "IslandSimulator/FishSchools.hpp"
#include "IslandSimulator/CanopyLightField.hpp"
#include "IslandSimulator/GroundCoverField.hpp"
#include "IslandSimulator/JungleLayout.hpp"
#include "IslandSimulator/GrassLayout.hpp"
#include "IslandSimulator/IslandTerrain.hpp"
#include "IslandSimulator/IslandEnvironmentField.hpp"
#include "IslandSimulator/JungleLayout.hpp"
#include "IslandSimulator/JungleGrounding.hpp"
#include "IslandSimulator/MonkeyTroop.hpp"
#include "IslandSimulator/NaturalPropLayout.hpp"
#include "IslandSimulator/RenderedTerrainSurface.hpp"
#include "IslandSimulator/SeaTurtleGroup.hpp"
#include "IslandSimulator/SharkPatrol.hpp"
#include "IslandSimulator/SnakePopulation.hpp"
#include "IslandSimulator/ShipwreckLayout.hpp"
#include "IslandSimulator/VegetationLayout.hpp"
#include "IslandSimulator/WorldCollisionGrid.hpp"
#include "IslandSimulator/WorldIdentity.hpp"
#include "IslandSimulator/HutLayout.hpp"
#include "IslandSimulator/HelicopterLayout.hpp"
#include "IslandSimulator/CaveGroundPatch.hpp"
#include "IslandSimulator/CampfireLayout.hpp"
#include "IslandSimulator/ResidentMotion.hpp"

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
#include <memory>
#include <numbers>
#include <ostream>
#include <sstream>
#include <span>
#include <stdexcept>
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
        island::CaveLayout cave{terrain};
        island::RenderedTerrainSurface caveOriginal{terrain};
        island::CaveGroundPatch caveGround{cave,caveOriginal};
        island::CampfireLayout campfire{terrain};
        island::DistantSceneryLayout distantScenery;
        island::BirdFlock birds{terrain};
        island::ButterflyMeadow butterflies{terrain};
        island::GeckoPopulation geckos{terrain};
        island::DragonflySwarm dragonflies{terrain};
        island::CrabColony crabs{terrain};
        island::BoarHerd boars{terrain};
        island::FishSchools fish{terrain};
        island::SeaTurtleGroup seaTurtles{terrain};
        island::SharkPatrol shark{terrain};
        island::DolphinPod dolphins{terrain};
        island::MonkeyTroop monkeys{terrain};
        island::VegetationLayout palms{terrain};
        island::JungleLayout jungle{terrain};
        std::vector<island::SpiderWebSite> spiderWebs{
            island::SpiderWebLayout::Propose(jungle,caveOriginal)};
        const island::CanopyLightField canopyLight{terrain, island::MakeCanopyOccluders(jungle.Canopy())};
        const island::GroundCoverField groundCover;
        island::GrassLayout grass{terrain, canopyLight, groundCover};
        island::NaturalPropLayout props{terrain, palms, jungle};
        island::ShipwreckLayout shipwreck{terrain};
        std::vector<island::CircularObstacle> worldObstacles;
        island::WorldCollisionGrid collisionGrid;
        island::WorldCollisionGrid wildlifeCollisionGrid;
        std::optional<island::HutPlacement> hut;
        std::optional<island::HelicopterPlacement> helicopter;
        std::unique_ptr<island::SnakePopulation> snakes;
        std::unique_ptr<island::GoatHerd> goats;
        std::unique_ptr<island::SpiderColony> spiders;
        std::unique_ptr<island::ResidentMotion> resident;
        std::unique_ptr<island::BearMotion> bear;

        WorldSnapshot()
        {
            std::vector<island::CrabHabitatCue> crabDebrisCues;
            for (const auto& wood : props.Driftwood())
                crabDebrisCues.push_back({wood.x, wood.z});
            for (const auto& detail : props.BeachDetails())
                if (detail.variant == 3U) crabDebrisCues.push_back({detail.x, detail.z});
            crabs.SetDebrisHabitats(crabDebrisCues);
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
            const auto fireObstacles=campfire.Obstacles();
            worldObstacles.insert(worldObstacles.end(),fireObstacles.begin(),fireObstacles.end());
            hut=island::HutLayout::FindInWorld(terrain,jungle,worldObstacles);
            if(hut) worldObstacles.push_back({hut->x,hut->z,island::HutLayout::CollisionRadius});
            helicopter=island::HelicopterLayout::FindInWorld(terrain,jungle,worldObstacles);
            if(!helicopter)throw std::runtime_error("No valid central helicopter site");
            collisionGrid.Reset(worldObstacles);
            std::vector<island::CircularObstacle> wildlifeObstacles(worldObstacles);
            wildlifeObstacles.push_back({helicopter->x,helicopter->z,3.2f});
            wildlifeCollisionGrid.Reset(wildlifeObstacles);
            resident=std::make_unique<island::ResidentMotion>(terrain,wildlifeCollisionGrid.Obstacles());
            bear=std::make_unique<island::BearMotion>(terrain,wildlifeCollisionGrid.Obstacles());
            monkeys.SetObstacles(wildlifeCollisionGrid.Obstacles());
            monkeys.SetCanopy(jungle.Canopy());
            boars.SetObstacles(wildlifeCollisionGrid.Obstacles());
            // Match runtime ordering: wildlife avoids both the hut and helicopter.
            snakes=std::make_unique<island::SnakePopulation>(
                terrain,jungle,wildlifeCollisionGrid.Obstacles());
            goats=std::make_unique<island::GoatHerd>(terrain,jungle,wildlifeCollisionGrid.Obstacles());
            spiders=std::make_unique<island::SpiderColony>(terrain,jungle,wildlifeCollisionGrid.Obstacles());
        }
    };

    struct TerrainStatistics final
    {
        float minimumShoreRadius = std::numeric_limits<float>::max();
        float maximumShoreRadius = std::numeric_limits<float>::lowest();
        float minimumHeight = std::numeric_limits<float>::max();
        float maximumHeight = std::numeric_limits<float>::lowest();
        std::array<std::size_t, 8U> surfaceCounts{};
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
        hash.AddSize(world.cave.Triangles().size());
        for(const auto& triangle:world.cave.Triangles()) {
            hash.AddEnum(triangle.surface);
            for(const auto p:triangle.points)HashPosition(hash,p.x,p.y,p.z);
            HashPosition(hash,triangle.normal.x,triangle.normal.y,triangle.normal.z);
        }
        hash.AddSize(world.caveGround.Vertices().size());
        for(const auto p:world.caveGround.Vertices())HashPosition(hash,p.x,p.y,p.z);
        for(const auto index:world.caveGround.Indices())hash.AddUint32(index);
        hash.AddSize(world.campfire.Vertices().size());
        for(const auto v:world.campfire.Vertices()) {
            HashPosition(hash,v.position.x,v.position.y,v.position.z);
            HashPosition(hash,v.normal.x,v.normal.y,v.normal.z);
            hash.AddFloat(v.u);hash.AddFloat(v.v);hash.AddEnum(v.material);
        }

        HashScaledPlacements(hash, std::span(world.palms.Palms()));
        for (const island::PalmPlacement& placement : world.palms.Palms()) {
            hash.AddFloat(placement.damageYawRadians);
            hash.AddEnum(placement.condition);
        }

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
        HashScaledPlacements(hash, std::span(world.jungle.StandingSnags()));
        HashScaledPlacements(hash, std::span(world.jungle.Understory()));
        HashScaledPlacements(hash, std::span(world.jungle.Bushes()));
        HashScaledPlacements(hash, std::span(world.jungle.GroundPlants()));
        for (const island::GroundPlantPlacement& placement : world.jungle.GroundPlants()) {
            hash.AddByte(placement.wetBank ? 1U : 0U);
            hash.AddByte(placement.seasonalWetland ? 1U : 0U);
        }

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
            hash.AddByte(rock.tidePoolLandmark ? 1U : 0U);
            hash.AddByte(rock.streamBankLandmark ? 1U : 0U);
            hash.AddByte(rock.cliffTalus ? 1U : 0U);
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
        HashScaledPlacements(hash, std::span(world.props.ReefHabitat()));

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

        hash.AddByte(world.hut?1U:0U);
        if(world.hut) {
            HashPosition(hash,world.hut->x,world.hut->floorY,world.hut->z);
            hash.AddFloat(world.hut->yawRadians);
            for(const auto& foot:world.hut->footings) HashPosition(hash,foot.x,foot.groundY,foot.z);
        }
        hash.AddByte(world.helicopter?1U:0U);
        if(world.helicopter) {
            const auto& helicopter=*world.helicopter;
            HashPosition(hash,helicopter.x,helicopter.groundY,helicopter.z);
            hash.AddFloat(helicopter.yawRadians);
            hash.AddFloat(helicopter.floorY);
            for(const auto& contact:helicopter.skidContacts)
                HashPosition(hash,contact.x,contact.groundY,contact.z);
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

        hash.AddSize(world.butterflies.Butterflies().size());
        for (std::size_t index = 0;
             index < world.butterflies.Butterflies().size(); ++index) {
            const island::ButterflyState& butterfly =
                world.butterflies.Butterflies()[index];
            const island::ButterflyHomeRange& home = world.butterflies.Homes()[index];
            HashPosition(hash, butterfly.x, butterfly.y, butterfly.z);
            hash.AddFloat(butterfly.headingRadians);
            hash.AddFloat(butterfly.wingFold);
            hash.AddFloat(butterfly.scale);
            hash.AddEnum(butterfly.behavior);
            hash.AddByte(butterfly.colorVariant);
            hash.AddFloat(home.x);
            hash.AddFloat(home.z);
            hash.AddFloat(home.radius);
        }
        hash.AddSize(world.butterflies.FlowerPatches().size());
        for (const auto& patch : world.butterflies.FlowerPatches()) {
            HashPosition(hash, patch.x, patch.y, patch.z);
            hash.AddFloat(patch.radius);
            hash.AddFloat(patch.groundSlopeX);
            hash.AddFloat(patch.groundSlopeZ);
        }
        hash.AddSize(world.geckos.Geckos().size());
        for(std::size_t index=0;index<world.geckos.Geckos().size();++index) {
            const auto& gecko=world.geckos.Geckos()[index];
            const auto& home=world.geckos.Homes()[index];
            HashPosition(hash,gecko.x,gecko.y,gecko.z);
            hash.AddFloat(gecko.headingRadians);
            hash.AddFloat(gecko.scale);
            hash.AddEnum(gecko.behavior);
            hash.AddByte(gecko.colorVariant);
            hash.AddFloat(home.x);hash.AddFloat(home.z);hash.AddFloat(home.radius);
        }
        hash.AddSize(world.dragonflies.Dragonflies().size());
        for(std::size_t index=0;index<world.dragonflies.Dragonflies().size();++index) {
            const auto& dragonfly=world.dragonflies.Dragonflies()[index];
            const auto& home=world.dragonflies.Homes()[index];
            HashPosition(hash,dragonfly.x,dragonfly.y,dragonfly.z);
            hash.AddFloat(dragonfly.headingRadians);
            hash.AddFloat(dragonfly.scale);
            hash.AddEnum(dragonfly.behavior);
            hash.AddByte(dragonfly.colorVariant);
            hash.AddFloat(home.x);hash.AddFloat(home.z);hash.AddFloat(home.radius);
        }

        hash.AddSize(world.boars.Boars().size());
        for (const auto& boar : world.boars.Boars()) {
            HashPosition(hash, boar.x, boar.y, boar.z);
            hash.AddFloat(boar.homeX);
            hash.AddFloat(boar.homeZ);
            hash.AddFloat(boar.headingRadians);
            hash.AddFloat(boar.scale);
            hash.AddEnum(boar.behavior);
            hash.AddByte(boar.colorVariant);
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
        hash.AddSize(world.crabs.Burrows().size());
        for (const auto& burrow : world.crabs.Burrows()) {
            HashPosition(hash, burrow.x, burrow.y, burrow.z);
            hash.AddFloat(burrow.radius);
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

        hash.AddSize(world.seaTurtles.Turtles().size());
        for (std::size_t index = 0;
             index < world.seaTurtles.Turtles().size(); ++index) {
            const island::SeaTurtleState& turtle = world.seaTurtles.Turtles()[index];
            const island::SeaTurtleHomeRange& home = world.seaTurtles.Homes()[index];
            HashPosition(hash, turtle.x, turtle.y, turtle.z);
            hash.AddFloat(turtle.headingRadians);
            hash.AddFloat(turtle.pitchRadians);
            hash.AddFloat(turtle.bankRadians);
            hash.AddFloat(turtle.flipperStroke);
            hash.AddFloat(turtle.scale);
            hash.AddEnum(turtle.behavior);
            hash.AddByte(turtle.colorVariant);
            hash.AddFloat(home.x);
            hash.AddFloat(home.z);
            hash.AddFloat(home.radius);
        }

        const island::SharkState& shark = world.shark.State();
        HashPosition(hash, shark.x, shark.y, shark.z);
        hash.AddFloat(shark.headingRadians);
        hash.AddFloat(shark.scale);
        hash.AddFloat(shark.targetX);
        hash.AddFloat(shark.targetZ);

        hash.AddSize(world.dolphins.Dolphins().size());
        for(std::size_t index=0;index<world.dolphins.Dolphins().size();++index) {
            const auto& dolphin=world.dolphins.Dolphins()[index];
            const auto& home=world.dolphins.Homes()[index];
            HashPosition(hash,dolphin.x,dolphin.y,dolphin.z);
            hash.AddFloat(dolphin.headingRadians);
            hash.AddFloat(dolphin.pitchRadians);
            hash.AddFloat(dolphin.bankRadians);
            hash.AddFloat(dolphin.speed);
            hash.AddFloat(dolphin.tailBeat);
            hash.AddFloat(dolphin.scale);
            hash.AddEnum(dolphin.behavior);
            hash.AddByte(dolphin.colorVariant);
            hash.AddFloat(home.x);hash.AddFloat(home.z);hash.AddFloat(home.radius);
        }

        hash.AddSize(world.monkeys.Monkeys().size());
        for (const island::MonkeyState& monkey : world.monkeys.Monkeys()) {
            HashPosition(hash, monkey.x, monkey.y, monkey.z);
            hash.AddFloat(monkey.headingRadians);
            hash.AddFloat(monkey.scale);
            hash.AddFloat(monkey.homeX);
            hash.AddFloat(monkey.homeZ);
            hash.AddByte(monkey.colorVariant);
        }
        hash.AddSize(world.snakes->Snakes().size());
        hash.AddSize(world.goats->Goats().size());
        for(const auto& goat:world.goats->Goats()) {
            HashPosition(hash,goat.x,goat.y,goat.z);
            hash.AddFloat(goat.heading);hash.AddFloat(goat.scale);
        }
        for(const auto& snake:world.snakes->Snakes()) {
            hash.AddFloat(snake.homeX);hash.AddFloat(snake.homeZ);
            hash.AddFloat(snake.heading);hash.AddFloat(snake.phase);hash.AddFloat(snake.speed);
            hash.AddEnum(snake.behavior);hash.AddEnum(snake.tier);
            hash.AddSize(snake.body.size());
            for(const auto& point:snake.body) HashPosition(hash,point.x,point.y,point.z);
        }
        hash.AddSize(world.spiders->Spiders().size());
        for(std::size_t i=0;i<world.spiders->Spiders().size();++i) {
            const auto& spider=world.spiders->Spiders()[i];
            const auto home=world.spiders->Homes()[i];
            HashPosition(hash,home.x,home.y,home.z);hash.AddSize(home.canopyIndex);
            HashPosition(hash,spider.x,spider.y,spider.z);
            hash.AddFloat(spider.heading);hash.AddFloat(spider.speed);hash.AddEnum(spider.behavior);
            const auto& pose=spider.pose;
            for(const auto p:{pose.origin,pose.right,pose.up,pose.forward})HashPosition(hash,p.x,p.y,p.z);
            for(std::size_t leg=0;leg<8;++leg) {
                const auto p=pose.feet[leg];HashPosition(hash,p.x,p.y,p.z);
                hash.AddByte(pose.stance[leg]?1:0);
                const auto& chain=pose.legs[leg];
                hash.AddByte(chain.reachLimited?1:0);
                for(const auto joint:{chain.root,chain.hip,chain.knee,chain.foot})HashPosition(hash,joint.x,joint.y,joint.z);
            }
        }
        hash.AddSize(world.spiderWebs.size());
        for(const auto& web:world.spiderWebs) {
            hash.AddSize(web.canopyIndex);
            for(const auto& anchor:web.anchors) {
                hash.AddUint32(static_cast<std::uint32_t>(anchor.branch));
                hash.AddSize(anchor.triangle);hash.AddFloat(anchor.u);hash.AddFloat(anchor.v);
            }
            for(const auto point:island::SpiderWebLayout::Sample(
                web,world.jungle,world.caveOriginal,0))HashPosition(hash,point[0],point[1],point[2]);
        }
        const auto& resident=world.resident->State();
        HashPosition(hash,resident.x,resident.y,resident.z);hash.AddFloat(resident.heading);
        hash.AddSize(world.resident->Route().Points().size());
        for(const auto p:world.resident->Route().Points())HashPosition(hash,p.x,p.y,p.z);
        const auto& bear=world.bear->State();
        HashPosition(hash,bear.x,bear.y,bear.z);hash.AddFloat(bear.heading);
        hash.AddFloat(bear.homeX);hash.AddFloat(bear.homeZ);hash.AddEnum(bear.behavior);
        for(const auto& leg:bear.pose.legs) {
            for(const auto p:{leg.root,leg.joint,leg.paw})HashPosition(hash,p.x,p.y,p.z);
            hash.AddByte(leg.stance?1:0);hash.AddByte(leg.reachLimited?1:0);
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
            && world.jungle.StandingSnags().size()
                == static_cast<std::size_t>(island::JungleLayout::StandingSnagTargetCount)
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
            && world.props.FallenFronds().size() <= 180U
            && world.props.ReefHabitat().size() >= 180U
            && world.props.ReefHabitat().size() <= 380U;
        std::size_t fallenPalmCount = 0U;
        std::size_t brokenPalmCount = 0U;
        for (const island::PalmPlacement& palm : world.palms.Palms()) {
            fallenPalmCount += palm.condition == island::PalmCondition::Fallen ? 1U : 0U;
            brokenPalmCount += palm.condition == island::PalmCondition::Broken ? 1U : 0U;
        }
        const bool derivedCountsAgree = world.palms.Obstacles().size()
                == world.palms.Palms().size() + fallenPalmCount * 5U
            && fallenPalmCount == island::VegetationLayout::FallenPalmTargetCount
            && brokenPalmCount == island::VegetationLayout::BrokenPalmTargetCount
            && world.jungle.Obstacles().size()
                == world.jungle.Canopy().size() + world.jungle.StandingSnags().size()
            && world.collisionGrid.Obstacles().size() == world.worldObstacles.size()
            && world.wildlifeCollisionGrid.Obstacles().size() == world.worldObstacles.size()+1U
            && world.birds.Birds().size() == island::BirdFlock::BirdCount
            && world.butterflies.Butterflies().size()
                == island::ButterflyMeadow::ButterflyCount
            && world.butterflies.Homes().size()
                == island::ButterflyMeadow::ButterflyCount
            && world.butterflies.FlowerPatches().size()
                == island::ButterflyMeadow::HabitatCount
            && world.geckos.Geckos().size()==island::GeckoPopulation::Count
            && world.geckos.Homes().size()==island::GeckoPopulation::Count
            && world.dragonflies.Dragonflies().size()==island::DragonflySwarm::Count
            && world.dragonflies.Homes().size()==island::DragonflySwarm::Count
            && world.crabs.Crabs().size() == island::CrabColony::CrabCount
            && world.boars.Boars().size() == island::BoarHerd::BoarCount
            && world.snakes->Snakes().size() == island::SnakePopulation::Count
            && world.goats->Goats().size() == island::GoatHabitat::Count
            && world.spiders->Spiders().size() == island::SpiderHabitat::GroundCount
            && world.spiders->Homes().size() == island::SpiderHabitat::GroundCount
            && world.spiderWebs.size() == island::SpiderWebLayout::MaximumSites
            && world.crabs.Homes().size() == island::CrabColony::CrabCount
            && world.crabs.Burrows().size() == 6U
            && world.fish.Fish().size() == island::FishSchools::FishCount
            && world.fish.Schools().size() == island::FishSchools::SchoolCount
            && world.seaTurtles.Turtles().size()
                == island::SeaTurtleGroup::TurtleCount
            && world.seaTurtles.Homes().size()
                == island::SeaTurtleGroup::TurtleCount
            && world.dolphins.Dolphins().size()==island::DolphinPod::DolphinCount
            && world.dolphins.Homes().size()==island::DolphinPod::DolphinCount
            && world.monkeys.Monkeys().size() == island::MonkeyTroop::MonkeyCount
            && world.shipwreck.Debris().size() == island::ShipwreckLayout::DebrisCount
            && world.shipwreck.Obstacles().size()
                == island::ShipwreckLayout::CollisionProxyCount
            && world.distantScenery.Islets().size()
                == island::DistantSceneryLayout::IsletCount;
        const bool finitePlacements = ScaledPlacementsAreFinite(
                std::span(world.palms.Palms()))
            && ScaledPlacementsAreFinite(std::span(world.jungle.Canopy()))
            && ScaledPlacementsAreFinite(std::span(world.jungle.StandingSnags()))
            && ScaledPlacementsAreFinite(std::span(world.jungle.Understory()))
            && ScaledPlacementsAreFinite(std::span(world.jungle.Bushes()))
            && ScaledPlacementsAreFinite(std::span(world.jungle.GroundPlants()))
            && ScaledPlacementsAreFinite(std::span(world.props.BeachDetails()))
            && ScaledPlacementsAreFinite(std::span(world.props.FallenFronds()))
            && ScaledPlacementsAreFinite(std::span(world.props.ReefHabitat()));
        bool finiteSpecializedData = true;
        for (const island::PalmPlacement& placement : world.palms.Palms()) {
            finiteSpecializedData = finiteSpecializedData
                && std::isfinite(placement.damageYawRadians)
                && placement.condition >= island::PalmCondition::Living
                && placement.condition <= island::PalmCondition::Broken;
        }
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
        for (std::size_t index = 0;
             index < world.butterflies.Butterflies().size(); ++index) {
            const island::ButterflyState& butterfly =
                world.butterflies.Butterflies()[index];
            const island::ButterflyHomeRange& home = world.butterflies.Homes()[index];
            const island::TerrainSample sample =
                world.terrain.Sample(butterfly.x, butterfly.z);
            finiteSpecializedData = finiteSpecializedData
                && AllFinite(butterfly.x, butterfly.y, butterfly.z,
                             butterfly.headingRadians, butterfly.speed,
                             butterfly.wingFold, butterfly.scale,
                             home.x, home.z, home.radius)
                && butterfly.scale > 0.0f && home.radius > 0.0f
                && sample.signedShoreDistance
                    >= island::ButterflyMeadow::MinimumInlandDistance;
        }
        for (const auto& patch : world.butterflies.FlowerPatches()) {
            finiteSpecializedData = finiteSpecializedData
                && AllFinite(patch.x, patch.y, patch.z, patch.radius,
                             patch.groundSlopeX, patch.groundSlopeZ)
                && patch.radius > 0.0f
                && std::fabs(patch.y - world.terrain.Height(patch.x, patch.z)) < .001f;
        }
        for(std::size_t index=0;index<world.geckos.Geckos().size();++index) {
            const auto& gecko=world.geckos.Geckos()[index];
            const auto& home=world.geckos.Homes()[index];
            const auto sample=world.terrain.Sample(gecko.x,gecko.z);
            finiteSpecializedData=finiteSpecializedData
                &&AllFinite(gecko.x,gecko.y,gecko.z,gecko.headingRadians,
                             gecko.speed,gecko.gaitPhase,gecko.visibility,gecko.scale,
                             home.x,home.z,home.radius)
                &&gecko.scale>0.0f&&home.radius>0.0f
                &&std::hypot(gecko.x-home.x,gecko.z-home.z)<=home.radius
                &&sample.signedShoreDistance>=34.0f&&sample.slopeRadians<.52f
                &&std::fabs(gecko.y-sample.height-.038f)<.001f;
        }
        for(std::size_t index=0;index<world.dragonflies.Dragonflies().size();++index) {
            const auto& dragonfly=world.dragonflies.Dragonflies()[index];
            const auto& home=world.dragonflies.Homes()[index];
            const auto sample=world.terrain.Sample(dragonfly.x,dragonfly.z);
            const auto inland=island::IslandEnvironmentField::SampleInland(
                dragonfly.x,dragonfly.z,sample.signedShoreDistance);
            finiteSpecializedData=finiteSpecializedData
                &&AllFinite(dragonfly.x,dragonfly.y,dragonfly.z,
                             dragonfly.headingRadians,dragonfly.speed,
                             dragonfly.wingPhase,dragonfly.bankRadians,
                             dragonfly.scale,home.x,home.z,home.radius)
                &&dragonfly.scale>0.0f&&home.radius>0.0f
                &&std::hypot(dragonfly.x-home.x,dragonfly.z-home.z)<=home.radius
                &&sample.height>0.0f&&sample.signedShoreDistance>12.0f
                &&sample.slopeRadians<.55f&&inland.moisture>.34f
                &&dragonfly.y>=sample.height+.70f
                &&dragonfly.y<=sample.height+1.22f;
        }
        for (const auto& boar : world.boars.Boars()) {
            finiteSpecializedData = finiteSpecializedData
                && AllFinite(boar.x, boar.y, boar.z, boar.homeX, boar.homeZ,
                             boar.headingRadians, boar.scale, boar.speed, boar.rootingPose)
                && boar.scale > 0.0f && world.boars.IsValidGround(boar.x, boar.z)
                && std::hypot(boar.x - boar.homeX, boar.z - boar.homeZ) <= island::BoarHerd::HomeRadius;
        }
        std::vector<island::CrabHabitatCue> crabDebrisCues;
        for (const auto& wood : world.props.Driftwood())
            crabDebrisCues.push_back({wood.x, wood.z});
        for (const auto& detail : world.props.BeachDetails())
            if (detail.variant == 3U) crabDebrisCues.push_back({detail.x, detail.z});
        for (std::size_t index = 0; index < world.crabs.Crabs().size(); ++index) {
            const island::CrabState& crab = world.crabs.Crabs()[index];
            const island::CrabHomeRange& home = world.crabs.Homes()[index];
            const island::TerrainSample sample = world.terrain.Sample(crab.x, crab.z);
            finiteSpecializedData = finiteSpecializedData
                && AllFinite(crab.x, crab.y, crab.z, crab.bodyYawRadians,
                             crab.travelHeadingRadians, crab.speed, crab.legPhase,
                             crab.clawPose, crab.scale, home.x, home.z, home.radius)
                && crab.scale > 0.0f && home.radius > 0.0f
                && island::CrabColony::IsNearDebris(home, crabDebrisCues)
                && sample.signedShoreDistance >= island::CrabColony::MinimumShoreDistance
                && sample.signedShoreDistance <= island::CrabColony::MaximumShoreDistance;
        }
        for (const auto& burrow : world.crabs.Burrows()) {
            finiteSpecializedData = finiteSpecializedData
                && AllFinite(burrow.x, burrow.y, burrow.z, burrow.radius)
                && burrow.radius > 0.0f
                && std::fabs(burrow.y - world.terrain.Height(burrow.x, burrow.z) - .012f)
                    < .001f;
        }
        for (const island::FishState& fish : world.fish.Fish()) {
            finiteSpecializedData = finiteSpecializedData
                && AllFinite(fish.x, fish.y, fish.z, fish.headingRadians,
                             fish.speed, fish.scale);
        }
        for (std::size_t index = 0;
             index < world.seaTurtles.Turtles().size(); ++index) {
            const island::SeaTurtleState& turtle = world.seaTurtles.Turtles()[index];
            const island::SeaTurtleHomeRange& home = world.seaTurtles.Homes()[index];
            const island::TerrainSample sample = world.terrain.Sample(turtle.x, turtle.z);
            finiteSpecializedData = finiteSpecializedData
                && AllFinite(turtle.x, turtle.y, turtle.z,
                             turtle.headingRadians, turtle.pitchRadians,
                             turtle.bankRadians, turtle.speed,
                             turtle.flipperStroke, turtle.scale,
                             home.x, home.z, home.radius)
                && turtle.scale > 0.0f && home.radius > 0.0f
                && sample.signedShoreDistance
                    >= island::SeaTurtleGroup::MinimumShoreDistance
                && sample.signedShoreDistance
                    <= island::SeaTurtleGroup::MaximumShoreDistance
                && sample.height <= -0.72f;
        }
        const island::SharkState& shark = world.shark.State();
        finiteSpecializedData = finiteSpecializedData
            && AllFinite(shark.x, shark.y, shark.z, shark.headingRadians, shark.scale,
                         shark.targetX, shark.targetZ);
        for(std::size_t index=0;index<world.dolphins.Dolphins().size();++index) {
            const auto& dolphin=world.dolphins.Dolphins()[index];
            const auto& home=world.dolphins.Homes()[index];
            const auto sample=world.terrain.Sample(dolphin.x,dolphin.z);
            finiteSpecializedData=finiteSpecializedData
                && AllFinite(dolphin.x,dolphin.y,dolphin.z,dolphin.headingRadians,
                    dolphin.pitchRadians,dolphin.bankRadians,dolphin.speed,dolphin.tailBeat,
                    dolphin.scale,home.x,home.z,home.radius)
                && dolphin.scale>0&&home.radius>0
                && std::hypot(dolphin.x-home.x,dolphin.z-home.z)<=home.radius
                && sample.signedShoreDistance>=island::DolphinPod::MinimumShoreDistance
                && sample.signedShoreDistance<=island::DolphinPod::MaximumShoreDistance
                && sample.height<=-3.0f&&dolphin.y<0;
        }
        for (const island::MonkeyState& monkey : world.monkeys.Monkeys()) {
            finiteSpecializedData = finiteSpecializedData
                && AllFinite(monkey.x, monkey.y, monkey.z, monkey.headingRadians,
                             monkey.scale, monkey.homeX, monkey.homeZ);
        }
        for(const auto& snake:world.snakes->Snakes()) {
            finiteSpecializedData=finiteSpecializedData
                && AllFinite(snake.homeX,snake.homeZ,snake.heading,snake.phase,snake.speed)
                && world.snakes->ValidBody(snake);
        }
        std::size_t goatIndex=0;
        for(const auto& goat:world.goats->Goats()) {
            finiteSpecializedData=finiteSpecializedData
                && AllFinite(goat.x,goat.y,goat.z,goat.heading,goat.scale,goat.phase,goat.speed,goat.grazingPose)
                && world.goats->IsValidPosition(goatIndex++,goat.x,goat.z);
        }
        bool validHut=world.hut.has_value();
        {
            const island::RenderedTerrainSurface surface(world.terrain);
            for(std::size_t i=0;i<world.spiders->Spiders().size();++i) {
                const auto& s=world.spiders->Spiders()[i];const auto h=world.spiders->Homes()[i];
                finiteSpecializedData=finiteSpecializedData
                    && AllFinite(s.x,s.y,s.z,s.heading,s.speed)
                    && s.x==h.x&&s.y==h.y&&s.z==h.z
                    && island::SpiderHabitat::AcceptsPatch(world.terrain,surface,world.jungle,
                        world.wildlifeCollisionGrid,h.x,h.z,h.canopyIndex);
                for(std::size_t leg=0;leg<8;++leg) {
                    const auto foot=s.pose.feet[leg],actual=s.pose.ToWorld(s.pose.legs[leg].foot);
                    finiteSpecializedData=finiteSpecializedData
                        && AllFinite(foot.x,foot.y,foot.z)
                        && s.pose.stance[leg]&&!s.pose.legs[leg].reachLimited
                        && std::abs(foot.y-surface.Sample(foot.x,foot.z).height-.0001f)<.00002f
                        && std::hypot(actual.x-foot.x,actual.y-foot.y,actual.z-foot.z)<.00003f;
                }
            }
            for(const auto& web:world.spiderWebs) {
                const auto anchors=island::SpiderWebLayout::Sample(web,world.jungle,surface,0);
                finiteSpecializedData=finiteSpecializedData
                    && web.canopyIndex<world.jungle.Canopy().size();
                for(const auto point:anchors)
                    finiteSpecializedData=finiteSpecializedData
                        && AllFinite(point[0],point[1],point[2]);
            }
        }
        if(world.hut) {
            const auto& h=*world.hut;
            validHut=AllFinite(h.x,h.z,h.floorY,h.yawRadians)
                && !world.worldObstacles.empty()
                && world.worldObstacles.back().x==h.x && world.worldObstacles.back().z==h.z
                && world.worldObstacles.back().radius==island::HutLayout::CollisionRadius;
            const island::RenderedTerrainSurface surface(world.terrain);
            for(const auto& foot:h.footings) validHut=validHut
                && AllFinite(foot.x,foot.groundY,foot.z)
                && foot.groundY==surface.Sample(foot.x,foot.z).height
                && h.floorY-foot.groundY>=island::HutLayout::FloorClearance-0.00001f;
        }
        bool validHelicopter=world.helicopter.has_value();
        if(world.helicopter) {
            const auto& h=*world.helicopter;
            const float centreRadius=std::hypot(h.x,h.z);
            validHelicopter=AllFinite(h.x,h.z,h.groundY,h.floorY,h.yawRadians)
                && centreRadius>=island::HelicopterLayout::MinimumCentreRadius
                && centreRadius<=island::HelicopterLayout::MaximumCentreRadius
                && world.terrain.SignedShoreDistance(h.x,h.z)
                    >=island::HelicopterLayout::MinimumShoreDistance
                && h.floorY==h.groundY+island::HelicopterLayout::FloorClearance
                && !world.wildlifeCollisionGrid.Obstacles().empty()
                && world.wildlifeCollisionGrid.Obstacles().back().x==h.x
                && world.wildlifeCollisionGrid.Obstacles().back().z==h.z
                && world.wildlifeCollisionGrid.Obstacles().back().radius==3.2f;
            const island::RenderedTerrainSurface surface(world.terrain);
            for(const auto& contact:h.skidContacts) validHelicopter=validHelicopter
                && AllFinite(contact.x,contact.groundY,contact.z)
                && contact.groundY==surface.Sample(contact.x,contact.z).height;
            for(const auto& obstacle:world.worldObstacles) validHelicopter=validHelicopter
                && std::hypot(h.x-obstacle.x,h.z-obstacle.z)
                    >=island::HelicopterLayout::SiteClearance+obstacle.radius;
            const auto pilotOutside=h.World(1.72f,0.0f,1.02f);
            const auto pilotInside=h.World(0.0f,0.0f,1.02f);
            const auto rearOutside=h.World(1.72f,0.0f,-.92f);
            const auto rearInside=h.World(.45f,0.0f,-.92f);
            validHelicopter=validHelicopter
                && h.SafeTravelFraction(pilotOutside.x,pilotOutside.z,
                    pilotInside.x,pilotInside.z,.32f)==1.0f
                && h.SafeTravelFraction(rearOutside.x,rearOutside.z,
                    rearInside.x,rearInside.z,.32f)==1.0f
                && h.InPilotFootwell(pilotInside.x,pilotInside.z)
                && h.InRearCabin(rearInside.x,rearInside.z)
                && h.SupportHeight(pilotInside.x,pilotInside.z,-100.0f)==h.floorY
                && h.SupportHeight(rearInside.x,rearInside.z,-100.0f)==h.floorY;
        }
        const auto& resident=world.resident->State();
        const bool validResident=world.resident->Route().Points().size()>=5
            &&world.resident->Route().Length()>400
            &&AllFinite(resident.x,resident.y,resident.z,resident.heading,resident.pose.pelvisY)
            &&island::ResidentRoute::SafePoint(world.terrain,world.resident->Ground(),
                world.wildlifeCollisionGrid,resident.x,resident.z);
        const auto& bear=world.bear->State();
        bool validBear=world.bear->IsValidPosition(bear.x,bear.z)
            &&AllFinite(bear.x,bear.y,bear.z,bear.heading,bear.homeX,bear.homeZ);
        for(const auto& leg:bear.pose.legs)validBear=validBear&&!leg.reachLimited
            &&AllFinite(leg.root.x,leg.root.y,leg.root.z,leg.joint.x,leg.joint.y,leg.joint.z,
                leg.paw.x,leg.paw.y,leg.paw.z);
        return validHut && validHelicopter && terrain.finite
            && terrain.sampleCount == TerrainReportSampleCount
            && terrain.minimumShoreRadius > 150.0f
            && terrain.maximumShoreRadius < 300.0f
            && expectedPopulations && derivedCountsAgree
            && finitePlacements && finiteSpecializedData && validResident && validBear
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

    [[nodiscard]] std::size_t CountCliffTalus(const WorldSnapshot& world) noexcept
    {
        std::size_t count = 0U;
        for (const island::RockPlacement& rock : world.props.Rocks()) {
            if (rock.cliffTalus) ++count;
        }
        return count;
    }

    [[nodiscard]] std::string FingerprintString(const std::uint64_t fingerprint)
    {
        std::ostringstream output;
        output << std::hex << std::setfill('0') << std::setw(16) << fingerprint;
        return output.str();
    }

    template<typename Placements>
    void WriteRootGaps(std::ostream& output, const Placements& plants,
                       const island::RenderedTerrainSurface& surface, const bool grounded = false)
    {
        float minimum = std::numeric_limits<float>::max();
        float maximum = std::numeric_limits<float>::lowest();
        double squareSum = 0.0;
        std::size_t floating = 0, buried = 0;
        for (const auto& plant : plants) {
            // Signed translation of the mesh origin, not the whole plant footprint.
            const auto rendered = grounded ? island::GroundJunglePlacement(plant, surface) : plant;
            const float gap = rendered.y - surface.Sample(plant.x, plant.z).height;
            minimum = std::min(minimum, gap);
            maximum = std::max(maximum, gap);
            squareSum += static_cast<double>(gap) * gap;
            floating += gap > 0.02f;
            buried += gap < -0.02f;
        }
        output << "{\"count\":" << plants.size()
               << ",\"minimumMeters\":" << (plants.empty() ? 0.0f : minimum)
               << ",\"maximumMeters\":" << (plants.empty() ? 0.0f : maximum)
               << ",\"rmsMeters\":" << (plants.empty() ? 0.0 : std::sqrt(squareSum / plants.size()))
               << ",\"floatingOverTwoCentimeters\":" << floating
               << ",\"buriedOverTwoCentimeters\":" << buried << '}';
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
               << "  \"cave\": {\"entrance\": [" << world.cave.Entrance().x << ','
               << world.cave.Entrance().y << ',' << world.cave.Entrance().z
               << "], \"lengthMeters\": " << island::CaveLayout::Length
               << ", \"shellTriangles\": " << world.cave.Triangles().size()
               << ", \"groundVertices\": " << world.caveGround.Vertices().size()
               << ", \"groundTriangles\": " << world.caveGround.Indices().size()/3 << "},\n"
               << "  \"campfire\": {\"centre\": ["<<world.campfire.Centre().x<<','<<world.campfire.Centre().y<<','
               <<world.campfire.Centre().z<<"], \"woodCentre\": ["<<world.campfire.WoodCentre().x<<','
               <<world.campfire.WoodCentre().y<<','<<world.campfire.WoodCentre().z
               <<"], \"solidTriangles\": "<<world.campfire.Vertices().size()/3
               <<", \"burningLogs\": "<<island::CampfireLayout::BurningLogCount
               <<", \"storedLogs\": "<<island::CampfireLayout::FirewoodCount<<"},\n"
               << "  \"resident\": {\"count\": 1, \"position\": ["
               <<world.resident->State().x<<','<<world.resident->State().y<<','<<world.resident->State().z
               <<"], \"routePoints\": "<<world.resident->Route().Points().size()
               <<", \"routeLengthMeters\": "<<world.resident->Route().Length()<<"},\n"
               << "  \"bear\": {\"count\": 1, \"position\": ["
               <<world.bear->State().x<<','<<world.bear->State().y<<','<<world.bear->State().z
               <<"], \"home\": ["<<world.bear->State().homeX<<','<<world.bear->State().homeZ
               <<"], \"peaceful\": true},\n"
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
               << "      \"steepGround\": " << terrain.surfaceCounts[6] << ",\n"
               << "      \"rockyGround\": " << terrain.surfaceCounts[7] << "\n"
               << "    }\n"
               << "  },\n"
               << "  \"analyticJungleRootGaps\": {\n";
        const island::RenderedTerrainSurface renderedSurface(world.terrain);
        output << "    \"canopy\": ";
        WriteRootGaps(output, world.jungle.Canopy(), renderedSurface);
        output << ",\n    \"standingSnags\": ";
        WriteRootGaps(output, world.jungle.StandingSnags(), renderedSurface);
        output << ",\n    \"understory\": ";
        WriteRootGaps(output, world.jungle.Understory(), renderedSurface);
        output << ",\n    \"bushes\": ";
        WriteRootGaps(output, world.jungle.Bushes(), renderedSurface);
        output << ",\n    \"groundPlants\": ";
        WriteRootGaps(output, world.jungle.GroundPlants(), renderedSurface);
        output << "\n  },\n  \"renderedJungleRootGaps\": {\n    \"canopy\": ";
        WriteRootGaps(output, world.jungle.Canopy(), renderedSurface, true);
        output << ",\n    \"standingSnags\": ";
        WriteRootGaps(output, world.jungle.StandingSnags(), renderedSurface, true);
        output << ",\n    \"understory\": ";
        WriteRootGaps(output, world.jungle.Understory(), renderedSurface, true);
        output << ",\n    \"bushes\": ";
        WriteRootGaps(output, world.jungle.Bushes(), renderedSurface, true);
        output << ",\n    \"groundPlants\": ";
        WriteRootGaps(output, world.jungle.GroundPlants(), renderedSurface, true);
        output << "\n  },\n"
               << "  \"placements\": {\n"
               << "    \"palms\": " << world.palms.Palms().size() << ",\n"
               << "    \"fallenPalms\": " << std::count_if(world.palms.Palms().begin(),
                    world.palms.Palms().end(), [](const island::PalmPlacement& palm) {
                        return palm.condition == island::PalmCondition::Fallen;
                    }) << ",\n"
               << "    \"brokenPalms\": " << std::count_if(world.palms.Palms().begin(),
                    world.palms.Palms().end(), [](const island::PalmPlacement& palm) {
                        return palm.condition == island::PalmCondition::Broken;
                    }) << ",\n"
               << "    \"grassPatches\": " << world.grass.Patches().size() << ",\n"
               << "    \"canopyTrees\": " << world.jungle.Canopy().size() << ",\n";
        output << "    \"standingSnags\": " << world.jungle.StandingSnags().size() << ",\n";
        output << "    \"mangroves\": " << world.jungle.MangroveCount() << ",\n";
        output << "    \"reefHabitat\": " << world.props.ReefHabitat().size() << ",\n";
        // Outermost representatives reduce foreground canopy obstruction for the explicit
        // elevated LOD camera. Actual screenshots must still establish visible coverage.
        output << "    \"canopyReviewTargets\": [";
        for (int variant = 0; variant < island::JungleLayout::CanopyVariantCount; ++variant) {
            const island::CanopyPlacement* selected = nullptr;
            for (const auto& tree : world.jungle.Canopy())
                if (tree.variant == variant && (!selected
                    || std::hypot(tree.x, tree.z) > std::hypot(selected->x, selected->z))) selected = &tree;
            if (!selected) throw std::runtime_error("Missing canopy review variant");
            if (variant) output << ',';
            output << "{\"variant\":" << variant << ",\"xMillimetres\":" << std::lround(selected->x * 1000)
                   << ",\"zMillimetres\":" << std::lround(selected->z * 1000) << '}';
        }
        output << "],\n"
               << "    \"understoryPlants\": " << world.jungle.Understory().size() << ",\n"
               << "    \"bushes\": " << world.jungle.Bushes().size() << ",\n"
               << "    \"groundPlants\": " << world.jungle.GroundPlants().size() << ",\n"
               << "    \"streamBankGroundPlants\": " << world.jungle.StreamBankGroundPlantCount() << ",\n"
               << "    \"seasonalWetlandGroundPlants\": "
               << world.jungle.SeasonalWetlandGroundPlantCount() << ",\n"
               << "    \"clearingBushes\": " << world.jungle.ClearingBushCount() << ",\n"
               << "    \"clearingGroundPlants\": " << world.jungle.ClearingGroundPlantCount() << ",\n"
               << "    \"forestFloorColonies\": " << world.jungle.ForestColonies().size() << ",\n"
               << "    \"forestFloorCompanions\": " << world.jungle.ForestGroundPlantCount() << ",\n"
               << "    \"rocks\": " << world.props.Rocks().size() << ",\n"
               << "    \"offshoreRocks\": " << CountOffshoreRocks(world) << ",\n"
               << "    \"cliffTalusBlocks\": " << CountCliffTalus(world) << ",\n"
               << "    \"streamBankRocks\": " << world.props.StreamBankRockCount() << ",\n"
               << "    \"fallenLogs\": " << world.props.FallenLogs().size() << ",\n"
               << "    \"streamBankLogs\": " << world.props.StreamBankLogCount() << ",\n"
               << "    \"driftwood\": " << world.props.Driftwood().size() << ",\n"
               << "    \"beachDetailClusters\": "
               << world.props.BeachDetails().size() << ",\n"
               << "    \"fallenFronds\": " << world.props.FallenFronds().size() << ",\n"
               << "    \"distantIslets\": " << world.distantScenery.Islets().size() << ",\n"
               << "    \"huts\": " << (world.hut?1:0) << ",\n"
               << "    \"helicopters\": " << (world.helicopter?1:0) << ",\n"
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
               << "  \"hut\": ";
        if(world.hut) {
            const auto& h=*world.hut;
            output << "{\n    \"x\": " << h.x << ", \"z\": " << h.z
                   << ", \"floorY\": " << h.floorY << ", \"yawRadians\": " << h.yawRadians
                   << ",\n    \"collisionRadiusMeters\": " << island::HutLayout::CollisionRadius
                   << ",\n    \"enterable\": false,\n    \"footings\": [";
            for(std::size_t i=0;i<h.footings.size();++i) {
                const auto& foot=h.footings[i];
                if(i) output << ',';
                output << "{\"x\":" << foot.x << ",\"groundY\":" << foot.groundY << ",\"z\":" << foot.z << '}';
            }
            output << "]\n  }";
        } else output << "null";
        output << ",\n  \"helicopter\": ";
        if(world.helicopter) {
            const auto& h=*world.helicopter;
            output << "{\n    \"x\": " << h.x << ", \"z\": " << h.z
                   << ", \"groundY\": " << h.groundY << ", \"floorY\": " << h.floorY
                   << ", \"yawRadians\": " << h.yawRadians
                   << ",\n    \"mainRotorRadiusMeters\": "
                   << island::HelicopterLayout::MainRotorRadius
                   << ",\n    \"grounded\": true, \"canFly\": false,"
                   << " \"pilotEnterable\": true, \"rearCabinEnterable\": true,"
                   << "\n    \"skidContacts\": [";
            for(std::size_t i=0;i<h.skidContacts.size();++i) {
                const auto& contact=h.skidContacts[i];
                if(i)output << ',';
                output << "{\"x\":" << contact.x << ",\"groundY\":" << contact.groundY
                       << ",\"z\":" << contact.z << '}';
            }
            output << "]\n  }";
        } else output << "null";
        output << ",\n  \"collision\": {\n"
               << "    \"staticObstacles\": " << world.worldObstacles.size() << ",\n"
               << "    \"wildlifeAvoidanceObstacles\": "
               << world.wildlifeCollisionGrid.Obstacles().size() << ",\n"
               << "    \"occupiedCells\": " << world.collisionGrid.CellCount() << ",\n"
               << "    \"cellSizeMeters\": " << island::WorldCollisionGrid::CellSize << "\n"
               << "  },\n"
               << "  \"wildlife\": {\n"
               << "    \"birds\": " << world.birds.Birds().size() << ",\n"
               << "    \"butterflies\": "
               << world.butterflies.Butterflies().size() << ",\n"
               << "    \"butterflyFlowerPatches\": "
               << world.butterflies.FlowerPatches().size() << ",\n"
               << "    \"geckos\": " << world.geckos.Geckos().size() << ",\n"
               << "    \"dragonflies\": " << world.dragonflies.Dragonflies().size() << ",\n"
               << "    \"beachCrabs\": " << world.crabs.Crabs().size() << ",\n"
               << "    \"crabBurrows\": " << world.crabs.Burrows().size() << ",\n"
               << "    \"wildBoars\": " << world.boars.Boars().size() << ",\n"
               << "    \"groundSnakes\": " << world.snakes->Snakes().size() << ",\n"
               << "    \"goats\": " << world.goats->Goats().size() << ",\n"
               << "    \"groundSpiders\": " << world.spiders->Spiders().size() << ",\n"
               << "    \"canopyWebs\": " << world.spiderWebs.size() << ",\n"
               << "    \"fishSchools\": " << world.fish.Schools().size() << ",\n"
               << "    \"fish\": " << world.fish.Fish().size() << ",\n"
               << "    \"seaTurtles\": " << world.seaTurtles.Turtles().size() << ",\n"
               << "    \"sharks\": 1,\n"
               << "    \"dolphins\": " << world.dolphins.Dolphins().size() << ",\n"
               << "    \"monkeys\": " << world.monkeys.Monkeys().size() << "\n"
               << "  },\n"
               << "  \"groundSnakePopulation\": {\n"
               << "    \"snapshot\": \"initial\",\n"
               << "    \"treeHabitatImplemented\": false,\n"
               << "    \"stationsPerSnake\": " << island::SnakeState::StationCount << ",\n"
               << "    \"spacingMeters\": " << island::SnakePopulation::Spacing << ",\n"
               << "    \"snakes\": [\n";
        const auto snakes=world.snakes->Snakes();
        for(std::size_t i=0;i<snakes.size();++i) {
            const auto& snake=snakes[i];
            output << "      {\"home\":[" << snake.homeX << ',' << snake.homeZ
                   << "],\"headingRadians\":" << snake.heading << ",\"body\":[";
            for(std::size_t j=0;j<snake.body.size();++j) {
                const auto& p=snake.body[j];
                if(j) output << ',';
                output << '[' << p.x << ',' << p.y << ',' << p.z << ']';
            }
            output << "]}" << (i+1<snakes.size()?",":"") << '\n';
        }
        output << "    ]\n  },\n  \"goatPopulation\": {\n    \"snapshot\": \"initial\",\n    \"goats\": [\n";
        const auto goats=world.goats->Goats();
        for(std::size_t i=0;i<goats.size();++i) {
            const auto& goat=goats[i];
            output << "      {\"position\":[" << goat.x << ',' << goat.y << ',' << goat.z
                   << "],\"headingRadians\":" << goat.heading << ",\"scale\":" << goat.scale << '}'
                   << (i+1<goats.size()?",":"") << '\n';
        }
        output << R"json(    ]
  },
  "groundSpiderPopulation": {
    "snapshot": "initial",
    "websImplemented": true,
    "webCount": )json" << world.spiderWebs.size() << R"json(,
    "webs": [
)json";
        for(std::size_t i=0;i<world.spiderWebs.size();++i) {
            const auto& web=world.spiderWebs[i];
            const auto rest=island::SpiderWebLayout::Sample(web,world.jungle,world.caveOriginal,0);
            output << "      {\"canopyIndex\":" << web.canopyIndex << ",\"anchors\":[";
            for(std::size_t j=0;j<web.anchors.size();++j) {
                if(j)output << ',';
                const auto& a=web.anchors[j];
                output << '[' << a.branch << ',' << a.triangle << ',' << a.u << ',' << a.v << ']';
            }
            output << "],\"rest\":[";
            for(std::size_t j=0;j<rest.size();++j) {
                if(j)output << ',';
                output << '[' << rest[j][0] << ',' << rest[j][1] << ',' << rest[j][2] << ']';
            }
            output << "]}" << (i+1<world.spiderWebs.size()?",":"") << '\n';
        }
        output << R"json(    ],
    "spiders": [
)json";
        output << std::setprecision(7); // Submillimetre appendages need more than four decimals.
        const auto writePoint=[&output](island::SpiderPoint p) {
            output << '[' << p.x << ',' << p.y << ',' << p.z << ']';
        };
        const auto spiders=world.spiders->Spiders();
        for(std::size_t i=0;i<spiders.size();++i) {
            const auto& s=spiders[i];const auto home=world.spiders->Homes()[i];
            output << R"({"position":)";
            writePoint({s.x,s.y,s.z});
            output << R"(,"home":)";writePoint({home.x,home.y,home.z});
            output << R"(,"canopyIndex":)" << home.canopyIndex
                   << R"(,"headingRadians":)" << s.heading
                   << R"(,"speed":)" << s.speed
                   << R"(,"behavior":)" << static_cast<int>(s.behavior) << R"(,"basis":[)";
            writePoint(s.pose.right);output << ',';writePoint(s.pose.up);output << ',';writePoint(s.pose.forward);
            output << R"(],"feet":[)";
            for(std::size_t leg=0;leg<8;++leg) {
                if(leg)output << ',';
                output << R"({"position":)";writePoint(s.pose.feet[leg]);
                output << R"(,"stance":)" << (s.pose.stance[leg]?"true":"false")
                       << R"(,"reachLimited":)" << (s.pose.legs[leg].reachLimited?"true":"false")
                       << R"(,"joints":[)";
                const auto& p=s.pose.legs[leg];
                writePoint(p.root);output << ',';writePoint(p.hip);output << ',';
                writePoint(p.knee);output << ',';writePoint(p.foot);output << "]}";
            }
            output << "]}" << (i+1<spiders.size()?",":"") << std::endl;
        }
        output << R"json(    ]
  }
}
)json";
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
