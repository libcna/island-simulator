#include "IslandSimulator/SpiderColony.hpp"
#include "IslandSimulator/SnakePopulation.hpp"
#include "IslandSimulator/GoatHerd.hpp"
#include "IslandSimulator/MonkeyTroop.hpp"
#include "IslandSimulator/NaturalPropLayout.hpp"
#include "IslandSimulator/ShipwreckLayout.hpp"
#include "IslandSimulator/HutLayout.hpp"
#include "IslandSimulator/VegetationLayout.hpp"
#include "IslandSimulator/PerformanceStatistics.hpp"

#include <chrono>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <vector>

// CPU-only diagnostic: no window, graphics context, audio or driver access.
int main()
{
    using namespace island;
    const IslandTerrain terrain;
    const VegetationLayout palms(terrain);
    const JungleLayout jungle(terrain);
    const NaturalPropLayout props(terrain, palms, jungle);
    const ShipwreckLayout wreck(terrain);
    std::vector<CircularObstacle> obstacles;
    for (const auto items : std::array<std::span<const CircularObstacle>, 4>{
            palms.Obstacles(), jungle.Obstacles(), props.Obstacles(), wreck.Obstacles()})
        obstacles.insert(obstacles.end(), items.begin(), items.end());
    const auto hut = HutLayout::FindInWorld(terrain, jungle, obstacles);
    if (!hut) throw std::runtime_error("Reference hut absent");
    obstacles.push_back({hut->x, hut->z, HutLayout::CollisionRadius});
    const SpiderColony initial(terrain, jungle, obstacles);
    const auto first = initial.Spiders().front();
    struct Scenario { const char* name; SpiderPoint observer; };
    const std::array scenarios{
        Scenario{"ground", {29.6189f, 7.11978f, -160.266f}},
        Scenario{"flight", {43.8888f, 60.4394f, -113.693f}},
        Scenario{"close", {first.x, first.y + 1.65f, first.z}},
    };
    std::cout << "CPU-only wildlife update: build=" ISLAND_BUILD_CONFIGURATION
              << " spiders=" << initial.Spiders().size()
              << " warmup=600 samples=300 dt=1/60; excludes world creation and rendering\n";
    for (const auto& scenario : scenarios) {
        auto colony = initial;
        SnakePopulation snakes(terrain, jungle, obstacles);
        GoatHerd goats(terrain, jungle, obstacles);
        MonkeyTroop monkeys(terrain);
        monkeys.SetObstacles(obstacles);
        monkeys.SetCanopy(jungle.Canopy());
        std::array<std::vector<double>, 4> durations;
        for (auto& samples : durations) samples.reserve(300);
        for (int step = 0; step < 900; ++step) {
            const auto measure = [&](std::size_t species, auto&& update) {
                const auto start = std::chrono::steady_clock::now();
                update();
                const double ms = std::chrono::duration<double, std::milli>(
                    std::chrono::steady_clock::now() - start).count();
                if (step >= 600) durations[species].push_back(ms);
            };
            const auto o = scenario.observer;
            measure(0, [&] { colony.Update(1.0f / 60.0f, o); });
            measure(1, [&] { snakes.Update(1.0f / 60.0f, o.x, o.y, o.z); });
            measure(2, [&] { goats.Update(1.0f / 60.0f, {o.x, o.y, o.z}); });
            measure(3, [&] { monkeys.Update(1.0f / 60.0f, o.x, o.y, o.z,
                                            static_cast<float>(step) / 60.0f); });
        }
        std::array<int, 3> tiers{};
        for (const auto& spider : colony.Spiders()) ++tiers[static_cast<int>(spider.tier)];
        constexpr std::array names{"spiders", "snakes", "goats", "monkeys"};
        for (std::size_t species = 0; species < names.size(); ++species) {
            const auto summary = CalculateFrameTimeSummary(durations[species]);
            std::cout << std::fixed << std::setprecision(4) << scenario.name << ' ' << names[species]
                  << " mean_ms=" << summary.meanMilliseconds
                  << " p95_ms=" << summary.percentile95Milliseconds
                  << " max_ms=" << summary.maximumMilliseconds
                  << " spider-tiers=" << tiers[0] << '/' << tiers[1] << '/' << tiers[2]
                  << std::endl;
        }
    }
}
