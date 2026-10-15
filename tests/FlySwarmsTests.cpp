#include "IslandSimulator/FlySwarms.hpp"
#include "IslandSimulator/IslandTerrain.hpp"

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
{
    try {
        island::IslandTerrain terrain;
        island::FlySwarms flies(terrain), repeated(terrain);
        Check(flies.Flies().size() == island::FlySwarms::Count, "wrong fly count");
        Check(flies.Homes().size() == island::FlySwarms::Count, "wrong fly home count");
        bool hover = false, orbit = false, dart = false;
        for (int step = 0; step < 5400; ++step) {
            flies.Update(1.0f / 60.0f, 1000.0f, 1000.0f, 1000.0f);
            repeated.Update(1.0f / 60.0f, 1000.0f, 1000.0f, 1000.0f);
            for (std::size_t index = 0; index < flies.Flies().size(); ++index) {
                const auto& fly = flies.Flies()[index];
                const auto& copy = repeated.Flies()[index];
                Check(std::isfinite(fly.x) && std::isfinite(fly.y) && std::isfinite(fly.z),
                      "fly position is non-finite");
                Check(std::hypot(fly.x - flies.Homes()[index].x,
                                 fly.z - flies.Homes()[index].z) < 1.7f,
                      "fly escaped its close swarm");
                Check(fly.x == copy.x && fly.y == copy.y && fly.z == copy.z
                          && fly.behavior == copy.behavior,
                      "fly simulation is not deterministic");
                hover |= fly.behavior == island::FlyBehavior::Hover;
                orbit |= fly.behavior == island::FlyBehavior::Orbit;
                dart |= fly.behavior == island::FlyBehavior::Dart;
            }
        }
        Check(hover && orbit && dart, "fly swarm did not exercise all behaviours");
        flies.Update(std::numeric_limits<float>::quiet_NaN(),
                     std::numeric_limits<float>::quiet_NaN(),
                     std::numeric_limits<float>::infinity(),
                     std::numeric_limits<float>::quiet_NaN());
        for (const auto& fly : flies.Flies())
            Check(std::isfinite(fly.x) && std::isfinite(fly.y) && std::isfinite(fly.z),
                  "invalid input poisoned fly state");
        std::cout << "Four deterministic close-range fly swarms hover, orbit and dart\n";
        return 0;
    } catch (const std::exception& exception) {
        std::cerr << exception.what() << '\n';
        return 1;
    }
}
