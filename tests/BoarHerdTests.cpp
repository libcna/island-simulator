#include "IslandSimulator/BoarHerd.hpp"
#include "IslandSimulator/IslandEnvironmentField.hpp"
#include "IslandSimulator/IslandTerrain.hpp"
#include "IslandSimulator/JungleLayout.hpp"
#include "IslandSimulator/NaturalPropLayout.hpp"
#include "IslandSimulator/VegetationLayout.hpp"

#include <array>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>

namespace
{
    int failures = 0;
    void Check(const bool condition, const char* message)
    {
        if (!condition) { ++failures; std::cerr << message << '\n'; }
    }
}

int main()
{
    island::IslandTerrain terrain;
    island::VegetationLayout palms(terrain);
    island::JungleLayout jungle(terrain);
    island::NaturalPropLayout props(terrain, palms, jungle);
    std::vector<island::CircularObstacle> obstacles(palms.Obstacles().begin(), palms.Obstacles().end());
    obstacles.insert(obstacles.end(), jungle.Obstacles().begin(), jungle.Obstacles().end());
    obstacles.insert(obstacles.end(), props.Obstacles().begin(), props.Obstacles().end());
    island::BoarHerd herd(terrain, obstacles), repeated(terrain, obstacles);
    std::array<bool, 4> behaviors{};
    for (int frame = 0; frame < 12000; ++frame) {
        const auto first = herd.Boars().front();
        const bool approach = frame % 1500 < 90;
        const float x = approach ? first.x - 3.0f : 0.0f;
        const float z = approach ? first.z : 0.0f;
        const float dt = frame % 300 == 0 ? 2.0f : 1.0f / 30.0f;
        herd.Update(dt, x, first.y + 1.7f, z);
        repeated.Update(dt, x, first.y + 1.7f, z);
        for (std::size_t index = 0; index < herd.Boars().size(); ++index) {
            const auto& boar = herd.Boars()[index];
            const auto& other = repeated.Boars()[index];
            Check(boar.x == other.x && boar.z == other.z && boar.gaitPhase == other.gaitPhase,
                  "boar simulation must be deterministic");
            Check(herd.IsValidGround(boar.x, boar.z), "boar left valid obstacle-free habitat");
            const auto inland = island::IslandEnvironmentField::SampleInland(
                boar.x, boar.z, terrain.SignedShoreDistance(boar.x, boar.z));
            Check(inland.moisture >= island::BoarHerd::MinimumHabitatMoisture
                      && inland.drainage <= island::BoarHerd::MaximumHabitatDrainage
                      && inland.saltExposure <= island::BoarHerd::MaximumHabitatSaltExposure,
                  "boar left the shared damp, non-channel interior habitat");
            Check(std::hypot(boar.x - boar.homeX, boar.z - boar.homeZ) <= island::BoarHerd::HomeRadius,
                  "boar exceeded its bounded home range");
            Check(boar.y == terrain.Height(boar.x, boar.z), "boar ground contact drifted");
            Check(std::isfinite(boar.headingRadians) && std::isfinite(boar.gaitPhase)
                      && boar.rootingPose >= 0.0f && boar.rootingPose <= 1.0f
                      && boar.speed >= 0.0f && boar.speed <= 2.801f,
                  "boar animation state left finite supported bounds");
            behaviors[static_cast<std::size_t>(boar.behavior)] = true;
        }
    }
    for (const bool observed : behaviors) Check(observed, "a boar behavior was never reached");
    const auto initial = herd.Boars().front();
    const float nan = std::numeric_limits<float>::quiet_NaN();
    herd.Update(nan, nan, nan, nan);
    Check(herd.Boars().front().x == initial.x, "non-finite dt changed a boar position");
    for (const auto& boar : herd.Boars()) {
        Check(boar.simulationTier == island::WildlifeSimulationTier::Dormant,
              "invalid observer must fail closed to dormant activation");
    }
    herd.Update(0.0f, initial.x + 150.0f, initial.y, initial.z);
    Check(herd.Boars().front().simulationTier == island::WildlifeSimulationTier::Reduced,
          "boar reduced activation is unreachable");
    herd.Update(0.0f, initial.x, initial.y, initial.z);
    Check(herd.Boars().front().simulationTier == island::WildlifeSimulationTier::Full,
          "nearby boar did not activate fully");
    const auto before = herd.Boars().front();
    try {
        const std::array blocked = {island::CircularObstacle{0.0f, 0.0f, 1000.0f}};
        herd.SetObstacles(blocked);
        Check(false, "impossible habitat must report an error rather than spawn inside an obstacle");
    } catch (const std::runtime_error&) { }
    Check(herd.Boars().front().x == before.x, "failed obstacle replacement mutated herd state");
    try {
        const std::array invalid = {island::CircularObstacle{nan, 0.0f, 1.0f}};
        herd.SetObstacles(invalid);
        Check(false, "invalid obstacles must be rejected");
    } catch (const std::invalid_argument&) { }
    obstacles.push_back({before.x, before.z, 3.0f});
    herd.SetObstacles(obstacles);
    for (const auto& boar : herd.Boars()) {
        Check(herd.IsValidGround(boar.x, boar.z), "obstacle replacement did not rebuild safe homes");
    }
    if (failures) return 1;
    std::cout << "BoarHerdTests: all checks passed (12000 frames with world obstacles)\n";
}
