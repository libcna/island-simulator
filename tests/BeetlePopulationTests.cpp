#include "IslandSimulator/BeetlePopulation.hpp"
#include "IslandSimulator/IslandTerrain.hpp"

#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace
{
    void Check(const bool value, const char* message)
    {
        if (!value) throw std::runtime_error(message);
    }
}

int main()
{
    try {
        island::IslandTerrain terrain;
        island::BeetlePopulation beetles(terrain), repeated(terrain);
        Check(beetles.Beetles().size() == island::BeetlePopulation::Count,
              "wrong beetle count");
        Check(beetles.Homes().size() == island::BeetlePopulation::ColonyCount,
              "wrong beetle colony count");
        bool rested = false, foraged = false, hidden = false;
        for (int step = 0; step < 10800; ++step) {
            beetles.Update(1.0f / 60.0f, 1000.0f, 1000.0f, 1000.0f);
            repeated.Update(1.0f / 60.0f, 1000.0f, 1000.0f, 1000.0f);
            for (std::size_t index = 0; index < beetles.Beetles().size(); ++index) {
                const auto& beetle = beetles.Beetles()[index];
                const auto& copy = repeated.Beetles()[index];
                const auto& home = beetles.Homes()[index / island::BeetlePopulation::PerColony];
                Check(std::isfinite(beetle.x) && std::isfinite(beetle.y)
                          && std::isfinite(beetle.z), "non-finite beetle state");
                Check(std::hypot(beetle.x - home.x, beetle.z - home.z) <= home.radius + .01f,
                      "beetle escaped its litter colony");
                Check(std::abs(beetle.y - terrain.Height(beetle.x, beetle.z) - .018f) < .002f,
                      "beetle is not grounded");
                Check(beetle.x == copy.x && beetle.z == copy.z
                          && beetle.behavior == copy.behavior,
                      "beetle simulation is not deterministic");
                rested |= beetle.behavior == island::BeetleBehavior::Rest;
                foraged |= beetle.behavior == island::BeetleBehavior::Forage;
            }
        }
        const auto subject = beetles.Beetles().front();
        for (int step = 0; step < 120; ++step) {
            beetles.Update(1.0f / 60.0f, subject.x, subject.y, subject.z);
            hidden |= beetles.Beetles().front().behavior == island::BeetleBehavior::Hide
                && beetles.Beetles().front().visibility < .25f;
        }
        Check(rested && foraged && hidden, "beetles did not exercise all behaviours");
        beetles.Update(0.0f, subject.x, subject.y, subject.z);
        Check(beetles.Beetles().front().simulationTier == island::WildlifeSimulationTier::Full,
              "near beetle did not use full simulation");
        beetles.Update(0.0f, subject.x + 35.0f, subject.y, subject.z);
        Check(beetles.Beetles().front().simulationTier == island::WildlifeSimulationTier::Reduced,
              "mid-distance beetle did not use reduced simulation");
        beetles.Update(std::numeric_limits<float>::quiet_NaN(),
                       std::numeric_limits<float>::quiet_NaN(), 0.0f,
                       std::numeric_limits<float>::infinity());
        for (const auto& beetle : beetles.Beetles())
            Check(std::isfinite(beetle.x) && std::isfinite(beetle.y),
                  "invalid input poisoned beetle state");
        std::cout << "BeetlePopulationTests: 15 deterministic leaf-litter beetles passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "BeetlePopulationTests: " << error.what() << '\n';
        return 1;
    }
}
