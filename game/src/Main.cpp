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
    bool inputDiagnostics = false;
    island::FixedViewpoint viewpoint = island::FixedViewpoint::None;
    std::string capturePath;
    for (int index = 1; index < argc; ++index) {
        const std::string_view argument(argv[index]);
        if (argument == "--smoke-test") {
            smokeTest = true;
        } else if (argument == "--debug-overlay") {
            showDebugOverlay = true;
        } else if (argument == "--input-diagnostics") {
            inputDiagnostics = true;
        } else if (argument == "--overview") {
            viewpoint = island::FixedViewpoint::Overview;
        } else if (argument == "--viewpoint") {
            if (++index >= argc) {
                std::cerr << "island-simulator: --viewpoint requires overview, beach, jungle, props, driftwood, birds, or fish\n";
                return 2;
            }
            const std::string_view name(argv[index]);
            if (name == "overview") {
                viewpoint = island::FixedViewpoint::Overview;
            } else if (name == "beach") {
                viewpoint = island::FixedViewpoint::BeachPath;
            } else if (name == "jungle") {
                viewpoint = island::FixedViewpoint::JungleTrail;
            } else if (name == "props") {
                viewpoint = island::FixedViewpoint::PropDetail;
            } else if (name == "driftwood") {
                viewpoint = island::FixedViewpoint::DriftwoodDetail;
            } else if (name == "birds") {
                viewpoint = island::FixedViewpoint::BirdDetail;
            } else if (name == "fish") {
                viewpoint = island::FixedViewpoint::FishDetail;
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
        } else {
            std::cerr << "island-simulator: unknown argument '" << argument << "'\n";
            return 2;
        }
    }
    try {
        island::IslandGame game(smokeTest, std::move(capturePath), viewpoint, showDebugOverlay,
                                inputDiagnostics);
        game.Run();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "island-simulator: fatal error: " << error.what() << '\n';
        return 1;
    }
}
