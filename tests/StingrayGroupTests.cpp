#include "IslandSimulator/IslandTerrain.hpp"
#include "IslandSimulator/StingrayGroup.hpp"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <vector>

int main()
{
    const auto require = [](const bool condition, const char* message) {
        if (!condition) throw std::runtime_error(message);
    };
    island::IslandTerrain terrain;
    island::StingrayGroup rays(terrain), repeated(terrain);
    require(rays.Stingrays().size() == island::StingrayGroup::Count,
            "stingray count changed");
    require(rays.Homes().size() == island::StingrayGroup::Count,
            "stingray home count changed");
    float travel = 0.0f;
    for (std::size_t index = 0; index < rays.Stingrays().size(); ++index) {
        const auto& ray = rays.Stingrays()[index];
        const auto& same = repeated.Stingrays()[index];
        const auto sample = terrain.Sample(ray.x, ray.z);
        require(ray.x == same.x && ray.y == same.y && ray.z == same.z
                && ray.behavior == same.behavior,
                "stingray initialization is not deterministic");
        require(sample.surface == island::TerrainSurface::ShallowSeabed
                && sample.height < -.7f && sample.height > -6.3f,
                "stingray did not start above shallow seabed");
    }
    std::vector<island::StingrayState> previous(
        rays.Stingrays().begin(), rays.Stingrays().end());
    island::StingrayGroup invalid(terrain);
    for (int frame = 0; frame < 7200; ++frame) {
        rays.Update(1.0f / 60.0f, 0.0f, 4.0f, 0.0f);
        repeated.Update(1.0f / 60.0f, 0.0f, 4.0f, 0.0f);
        for (std::size_t index = 0; index < rays.Stingrays().size(); ++index) {
            const auto& ray = rays.Stingrays()[index];
            const auto& same = repeated.Stingrays()[index];
            require(ray.x == same.x && ray.y == same.y && ray.z == same.z
                    && ray.wingWave == same.wingWave,
                    "stingray simulation is not deterministic");
            require(std::hypot(ray.x - rays.Homes()[index].x,
                               ray.z - rays.Homes()[index].z)
                        <= rays.Homes()[index].radius + .02f,
                    "stingray escaped its sandy home");
            const float seabed = terrain.Height(ray.x, ray.z);
            require(ray.y > seabed + .04f && ray.y < -.10f,
                    "stingray clipped seabed or water surface");
            if (frame == 0) previous[index] = ray;
            if (frame == 7199) {
                travel += std::hypot(ray.x - previous[index].x, ray.z - previous[index].z);
            }
        }
    }
    require(travel > 1.0f, "stingrays did not move through their habitats");
    require(rays.FullSimulationCount() + rays.ReducedSimulationCount()
            + rays.DormantSimulationCount() == static_cast<int>(island::StingrayGroup::Count),
            "stingray activation tiers do not account for the group");
    invalid.Update(NAN, NAN, NAN, NAN);
    require(invalid.DormantSimulationCount() == static_cast<int>(island::StingrayGroup::Count),
            "stingrays did not fail dormant on invalid input");
    std::cout << "Stingrays: five deterministic shallow-sand agents remain bounded\n";
}
