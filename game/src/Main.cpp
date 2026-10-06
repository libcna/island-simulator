#include "CNA/Platform/Entrypoint.hpp"

#include "IslandSimulator/IslandGame.hpp"

#include <exception>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>

int main(const int argc, char* argv[])
{
    bool smokeTest = false;
    bool showDebugOverlay = false;
    bool showWorldDebug = false;
    bool inputDiagnostics = false;
    island::FixedViewpoint viewpoint = island::FixedViewpoint::None;
    std::string capturePath;
    std::string benchmarkPath;
    for (int index = 1; index < argc; ++index) {
        const std::string_view argument(argv[index]);
        if (argument == "--smoke-test") {
            smokeTest = true;
        } else if (argument == "--debug-overlay") {
            showDebugOverlay = true;
        } else if (argument == "--world-debug") {
            showDebugOverlay = true;
            showWorldDebug = true;
        } else if (argument == "--input-diagnostics") {
            inputDiagnostics = true;
        } else if (argument == "--overview") {
            viewpoint = island::FixedViewpoint::Overview;
        } else if (argument == "--viewpoint") {
            if (++index >= argc) {
                std::cerr << "island-simulator: --viewpoint requires overview, beach, jungle, "
                             "water, current, distant, props, driftwood, reef, fronds, birds, fish, shark, or "
                             "monkeys\n";
                return 2;
            }
            const std::string_view name(argv[index]);
            if (name == "overview") {
                viewpoint = island::FixedViewpoint::Overview;
            } else if (name == "beach") {
                viewpoint = island::FixedViewpoint::BeachPath;
            } else if (name == "jungle") {
                viewpoint = island::FixedViewpoint::JungleTrail;
            } else if (name == "water") {
                viewpoint = island::FixedViewpoint::WaterlineDetail;
            } else if (name == "current") {
                viewpoint = island::FixedViewpoint::CurrentBoundaryDetail;
            } else if (name == "distant") {
                viewpoint = island::FixedViewpoint::DistantSceneryDetail;
            } else if (name == "props") {
                viewpoint = island::FixedViewpoint::PropDetail;
            } else if (name == "driftwood") {
                viewpoint = island::FixedViewpoint::DriftwoodDetail;
            } else if (name == "reef") {
                viewpoint = island::FixedViewpoint::ReefDetail;
            } else if (name == "fronds") {
                viewpoint = island::FixedViewpoint::FallenFrondDetail;
            } else if (name == "birds") {
                viewpoint = island::FixedViewpoint::BirdDetail;
            } else if (name == "fish") {
                viewpoint = island::FixedViewpoint::FishDetail;
            } else if (name == "shark") {
                viewpoint = island::FixedViewpoint::SharkDetail;
            } else if (name == "monkeys") {
                viewpoint = island::FixedViewpoint::MonkeyDetail;
            } else {
                std::cerr << "island-simulator: unknown viewpoint '" << name << "'\n";
                return 2;
            }
        } else if (argument == "--capture") {
            if (++index >= argc) {
                std::cerr << "island-simulator: --capture requires a path\n";
                return 2;
            }
            capturePath = argv[index];
            smokeTest = true;
        } else if (argument == "--benchmark") {
            if (++index >= argc) {
                std::cerr << "island-simulator: --benchmark requires a JSON output path\n";
                return 2;
            }
            benchmarkPath = argv[index];
        } else {
            std::cerr << "island-simulator: unknown argument '" << argument << "'\n";
            return 2;
        }
    }
    if (!benchmarkPath.empty()
        && (smokeTest || showDebugOverlay || showWorldDebug || inputDiagnostics)) {
        std::cerr << "island-simulator: --benchmark cannot be combined with smoke, capture, "
                     "or debug modes\n";
        return 2;
    }
    try {
        island::IslandGame game(smokeTest, std::move(capturePath), viewpoint, showDebugOverlay,
                                inputDiagnostics, showWorldDebug, std::move(benchmarkPath));
        game.Run();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "island-simulator: fatal error: " << error.what() << '\n';
        return 1;
    }
}
