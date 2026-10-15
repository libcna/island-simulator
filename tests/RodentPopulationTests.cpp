#include "IslandSimulator/IslandTerrain.hpp"
#include "IslandSimulator/RodentPopulation.hpp"

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
        island::RodentPopulation rodents(terrain), repeated(terrain);
        Check(rodents.Rodents().size() == island::RodentPopulation::Count,
              "wrong rodent count");
        Check(rodents.Homes().size() == island::RodentPopulation::Count,
              "wrong rodent home count");
        for (std::size_t index = 0; index < rodents.Rodents().size(); ++index) {
            const auto& rodent = rodents.Rodents()[index];
            Check(rodent.species == (index < island::RodentPopulation::MouseCount
                      ? island::RodentSpecies::Mouse : island::RodentSpecies::Rat),
                  "rodent species partition changed");
            Check(rodents.IsValidPosition(index, rodent.x, rodent.z),
                  "initial rodent habitat invalid");
            Check(rodent.x == repeated.Rodents()[index].x
                      && rodent.z == repeated.Rodents()[index].z
                      && rodent.headingRadians == repeated.Rodents()[index].headingRadians,
                  "rodent construction is not deterministic");
        }

        bool rested = false, foraged = false;
        for (int step = 0; step < 7200; ++step) {
            rodents.Update(1.0f / 60.0f, 1000.0f, 1000.0f, 1000.0f);
            repeated.Update(1.0f / 60.0f, 1000.0f, 1000.0f, 1000.0f);
            for (std::size_t index = 0; index < rodents.Rodents().size(); ++index) {
                const auto& rodent = rodents.Rodents()[index];
                const auto& copy = repeated.Rodents()[index];
                Check(rodents.IsValidPosition(index, rodent.x, rodent.z),
                      "rodent left its bounded dry habitat");
                Check(rodent.x == copy.x && rodent.y == copy.y && rodent.z == copy.z
                          && rodent.behavior == copy.behavior,
                      "rodent simulation is not deterministic");
                rested |= rodent.behavior == island::RodentBehavior::Rest;
                foraged |= rodent.behavior == island::RodentBehavior::Forage;
            }
        }
        Check(rested && foraged, "rodents did not exercise rest and forage states");

        island::RodentPopulation reactive(terrain);
        const auto initial = reactive.Rodents().front();
        bool scurried = false, hidden = false;
        for (int step = 0; step < 300; ++step) {
            reactive.Update(1.0f / 60.0f, initial.x, initial.y + .2f, initial.z);
            scurried |= reactive.Rodents().front().behavior == island::RodentBehavior::Scurry;
            hidden |= reactive.Rodents().front().behavior == island::RodentBehavior::Hidden;
        }
        Check(scurried && hidden, "near observer did not trigger scurry and hide states");

        reactive.Update(std::numeric_limits<float>::quiet_NaN(),
                        std::numeric_limits<float>::quiet_NaN(),
                        std::numeric_limits<float>::infinity(),
                        std::numeric_limits<float>::quiet_NaN());
        for (const auto& rodent : reactive.Rodents()) {
            Check(std::isfinite(rodent.x) && std::isfinite(rodent.y)
                      && std::isfinite(rodent.z) && std::isfinite(rodent.headingRadians),
                  "invalid update input poisoned rodent state");
        }
        std::cout << "Twelve mice and eight rats forage, scurry and hide deterministically\n";
        return 0;
    } catch (const std::exception& exception) {
        std::cerr << exception.what() << '\n';
        return 1;
    }
}
