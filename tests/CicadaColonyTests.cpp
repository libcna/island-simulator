#include "IslandSimulator/CicadaColony.hpp"
#include "IslandSimulator/IslandTerrain.hpp"
#include "IslandSimulator/JungleLayout.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace
{
    void Check(const bool condition, const char* message)
    {
        if (!condition) throw std::runtime_error(message);
    }
}

int main()
try {
    const island::IslandTerrain terrain;
    const island::JungleLayout jungle(terrain);
    island::CicadaColony colony(jungle), repeated(jungle);
    Check(colony.Cicadas().size() == island::CicadaColony::Count,
          "wrong cicada count");
    bool rested = false, called = false, flew = false;
    float maximumExcursion = 0.0f;
    for (int frame = 0; frame < 60 * 180; ++frame) {
        colony.Update(1.0f / 60.0f, 0.0f, 8.0f, -70.0f);
        repeated.Update(1.0f / 60.0f, 0.0f, 8.0f, -70.0f);
        for (std::size_t index = 0; index < colony.Cicadas().size(); ++index) {
            const auto& cicada = colony.Cicadas()[index];
            const auto& copy = repeated.Cicadas()[index];
            const auto& home = colony.Homes()[index];
            Check(cicada.x == copy.x && cicada.y == copy.y && cicada.z == copy.z
                && cicada.behavior == copy.behavior,
                "cicada simulation is not deterministic");
            Check(std::isfinite(cicada.x) && std::isfinite(cicada.y)
                && std::isfinite(cicada.z) && std::isfinite(cicada.wingPhase),
                "cicada pose is not finite");
            const float forwardLength = std::hypot(std::hypot(cicada.forwardX,
                cicada.forwardZ), cicada.forwardY);
            const float normalLength = std::hypot(std::hypot(cicada.normalX,
                cicada.normalZ), cicada.normalY);
            Check(std::abs(forwardLength - 1.0f) < 0.001f
                && std::abs(normalLength - 1.0f) < 0.001f,
                "cicada frame is not normalized");
            rested |= cicada.behavior == island::CicadaBehavior::Rest;
            called |= cicada.behavior == island::CicadaBehavior::Call;
            flew |= cicada.behavior == island::CicadaBehavior::Flight;
            maximumExcursion = std::max(maximumExcursion,
                std::hypot(cicada.x - home.x, cicada.z - home.z) - home.radius);
            if (cicada.behavior != island::CicadaBehavior::Flight) {
                Check(std::abs(std::hypot(cicada.x - home.x, cicada.z - home.z)
                    - home.radius) < 0.001f && std::abs(cicada.y - home.y) < 0.001f,
                    "perched cicada left its host trunk");
            }
        }
    }
    Check(rested && called && flew && maximumExcursion > 0.8f,
          "cicada behavior cycle was not covered");
    const auto subject = colony.Cicadas()[1];
    colony.Update(0.0f, subject.x, subject.y, subject.z);
    Check(colony.Cicadas()[1].simulationTier == island::WildlifeSimulationTier::Full,
          "near cicada did not enter full simulation");
    colony.Update(0.0f, subject.x + 44.0f, subject.y, subject.z);
    Check(colony.Cicadas()[1].simulationTier == island::WildlifeSimulationTier::Reduced,
          "mid-distance cicada did not enter reduced simulation");
    colony.Update(std::numeric_limits<float>::quiet_NaN(),
                  std::numeric_limits<float>::quiet_NaN(), 0.0f, 0.0f);
    for (const auto& cicada : colony.Cicadas()) {
        Check(std::isfinite(cicada.x) && std::isfinite(cicada.wingPhase),
              "invalid cicada input poisoned state");
    }
    std::cout << "CicadaColonyTests: 12 deterministic trunk cicadas passed\n";
    return 0;
} catch (const std::exception& error) {
    std::cerr << "CicadaColonyTests: " << error.what() << '\n';
    return 1;
}
