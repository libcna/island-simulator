#include "CNA/Platform/Entrypoint.hpp"

#include "IslandSimulator/IslandGame.hpp"
#include "IslandSimulator/RuntimeDiagnostics.hpp"
#ifdef __ANDROID__
#include "IslandSimulator/DiagnosticStreamMirror.hpp"
#endif

#include <exception>
#include <charconv>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>

namespace
{
    [[nodiscard]] const char* ViewpointName(const island::FixedViewpoint viewpoint) noexcept
    {
        using island::FixedViewpoint;
        switch (viewpoint) {
        case FixedViewpoint::None: return "interactive";
        case FixedViewpoint::Overview: return "overview";
        case FixedViewpoint::BayOverview: return "bay";
        case FixedViewpoint::BayBeachDetail: return "bay-ground";
        case FixedViewpoint::HeadlandDetail: return "headland";
        case FixedViewpoint::BeachPath: return "beach";
        case FixedViewpoint::JungleTrail: return "jungle";
        case FixedViewpoint::EastClearingDetail: return "east";
        case FixedViewpoint::ClearingEdgeDetail: return "clearing-edge";
        case FixedViewpoint::CanopyInspection: return "canopy";
        case FixedViewpoint::WestClearingDetail: return "west";
        case FixedViewpoint::GrassDetail: return "grass";
        case FixedViewpoint::GroundPlantDetail: return "ground";
        case FixedViewpoint::GroundBroadleafDetail: return "ground-broadleaf";
        case FixedViewpoint::BushDetail: return "bush";
        case FixedViewpoint::FallenLeafDetail: return "litter";
        case FixedViewpoint::GroundContactDetail: return "contact";
        case FixedViewpoint::MovementTraceDetail: return "movement";
        case FixedViewpoint::PalmDetail: return "palms";
        case FixedViewpoint::WaterlineDetail: return "water";
        case FixedViewpoint::WetSandDetail: return "wet-sand";
        case FixedViewpoint::CurrentBoundaryDetail: return "current";
        case FixedViewpoint::DistantSceneryDetail: return "distant";
        case FixedViewpoint::PropDetail: return "props";
        case FixedViewpoint::DriftwoodDetail: return "driftwood";
        case FixedViewpoint::ReefDetail: return "reef";
        case FixedViewpoint::FallenFrondDetail: return "fronds";
        case FixedViewpoint::BirdDetail: return "birds";
        case FixedViewpoint::FishDetail: return "fish";
        case FixedViewpoint::SharkDetail: return "shark";
        case FixedViewpoint::MonkeyDetail: return "monkeys";
        case FixedViewpoint::MonkeyTreeDetail: return "monkey-tree";
        case FixedViewpoint::MonkeyClimbDetail: return "monkey-climb";
        case FixedViewpoint::ShipwreckDetail: return "shipwreck";
        case FixedViewpoint::HutDetail: return "hut";
        case FixedViewpoint::HutMaterialDetail: return "hut-material";
        case FixedViewpoint::CrabDetail: return "crabs";
        case FixedViewpoint::BoarDetail: return "boars";
        case FixedViewpoint::SnakeDetail: return "snakes";
        case FixedViewpoint::SnakeHeadDetail: return "snake-head";
        case FixedViewpoint::ButterflyDetail: return "butterflies";
        case FixedViewpoint::SeaTurtleDetail: return "turtles";
        case FixedViewpoint::BenchmarkTraversal: return "traversal";
        }
        return "unknown";
    }
}

int main(const int argc, char* argv[])
{
    island::InitializeRuntimeDiagnostics(argc, argv);
#ifdef __ANDROID__
    island::DiagnosticStreamMirror stderrMirror(
        std::cerr, island::MirrorRuntimeDiagnosticToPlatformLog);
#endif
    bool smokeTest = false;
    bool showDebugOverlay = false;
    bool showWorldDebug = false;
    bool inputDiagnostics = false;
    bool noUserSettings = false;
    island::FixedViewpoint viewpoint = island::FixedViewpoint::None;
    std::string capturePath;
    std::string benchmarkPath;
    std::string startupReportPath;
    std::optional<island::CanopyLodInspection> canopyLodInspection;
    for (int index = 1; index < argc; ++index) {
        const std::string_view argument(argv[index]);
        if (argument == "--canopy-lod-inspection") {
            island::CanopyLodInspection inspection{};
            const auto coordinate = [](const std::string_view text, int& value) {
                const auto result = std::from_chars(text.data(), text.data() + text.size(), value);
                return result.ec == std::errc{} && result.ptr == text.data() + text.size()
                    && value >= -500000 && value <= 500000;
            };
            if (canopyLodInspection || index + 3 >= argc
                || !coordinate(argv[index + 1], inspection.xMillimetres)
                || !coordinate(argv[index + 2], inspection.zMillimetres)
                || (std::string_view(argv[index + 3]) != "near"
                    && std::string_view(argv[index + 3]) != "far")) {
                std::cerr << "island-simulator: --canopy-lod-inspection requires unique "
                             "X_MM Z_MM near|far (integer coordinates within +/-500000)\n";
                return 2;
            }
            inspection.near = std::string_view(argv[index + 3]) == "near";
            canopyLodInspection = inspection;
            index += 3;
        } else if (argument == "--smoke-test") {
            smokeTest = true;
        } else if (argument == "--debug-overlay") {
            showDebugOverlay = true;
        } else if (argument == "--world-debug") {
            showDebugOverlay = true;
            showWorldDebug = true;
        } else if (argument == "--input-diagnostics") {
            inputDiagnostics = true;
        } else if (argument == "--no-user-settings") {
            noUserSettings = true;
        } else if (argument == "--overview") {
            viewpoint = island::FixedViewpoint::Overview;
        } else if (argument == "--viewpoint") {
            if (++index >= argc) {
                std::cerr << "island-simulator: --viewpoint requires overview, bay, bay-ground, beach, jungle, "
                             "east, headland, clearing-edge, canopy (inspection only), west, grass, ground, ground-broadleaf, bush, litter, contact, movement, palms, water, "
                             "wet-sand, current, distant, props, driftwood, reef, fronds, birds, fish, "
                             "shark, monkeys, monkey-tree, monkey-climb, shipwreck, hut, hut-material, crabs, boars, snakes, snake-head, butterflies, turtles, or traversal\n";
                return 2;
            }
            const std::string_view name(argv[index]);
            if (name == "overview") {
                viewpoint = island::FixedViewpoint::Overview;
            } else if (name == "bay") {
                viewpoint = island::FixedViewpoint::BayOverview;
            } else if (name == "headland") {
                viewpoint = island::FixedViewpoint::HeadlandDetail;
            } else if (name == "bay-ground") {
                viewpoint = island::FixedViewpoint::BayBeachDetail;
            } else if (name == "beach") {
                viewpoint = island::FixedViewpoint::BeachPath;
            } else if (name == "jungle") {
                viewpoint = island::FixedViewpoint::JungleTrail;
            } else if (name == "east") {
                viewpoint = island::FixedViewpoint::EastClearingDetail;
            } else if (name == "clearing-edge") {
                viewpoint = island::FixedViewpoint::ClearingEdgeDetail;
            } else if (name == "canopy") {
                viewpoint = island::FixedViewpoint::CanopyInspection;
            } else if (name == "west") {
                viewpoint = island::FixedViewpoint::WestClearingDetail;
            } else if (name == "grass") {
                viewpoint = island::FixedViewpoint::GrassDetail;
            } else if (name == "ground") {
                viewpoint = island::FixedViewpoint::GroundPlantDetail;
            } else if (name == "ground-broadleaf") {
                viewpoint = island::FixedViewpoint::GroundBroadleafDetail;
            } else if (name == "bush") {
                viewpoint = island::FixedViewpoint::BushDetail;
            } else if (name == "litter") {
                viewpoint = island::FixedViewpoint::FallenLeafDetail;
            } else if (name == "contact") {
                viewpoint = island::FixedViewpoint::GroundContactDetail;
            } else if (name == "movement") {
                viewpoint = island::FixedViewpoint::MovementTraceDetail;
            } else if (name == "palms") {
                viewpoint = island::FixedViewpoint::PalmDetail;
            } else if (name == "water") {
                viewpoint = island::FixedViewpoint::WaterlineDetail;
            } else if (name == "wet-sand") {
                viewpoint = island::FixedViewpoint::WetSandDetail;
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
            } else if (name == "monkey-tree") {
                viewpoint = island::FixedViewpoint::MonkeyTreeDetail;
            } else if (name == "monkey-climb") {
                viewpoint = island::FixedViewpoint::MonkeyClimbDetail;
            } else if (name == "shipwreck") {
                viewpoint = island::FixedViewpoint::ShipwreckDetail;
            } else if (name == "hut") {
                viewpoint = island::FixedViewpoint::HutDetail;
            } else if (name == "hut-material") {
                viewpoint = island::FixedViewpoint::HutMaterialDetail;
            } else if (name == "crabs") {
                viewpoint = island::FixedViewpoint::CrabDetail;
            } else if (name == "snakes") {
                viewpoint = island::FixedViewpoint::SnakeDetail;
            } else if (name == "snake-head") {
                viewpoint = island::FixedViewpoint::SnakeHeadDetail;
            } else if (name == "boars") {
                viewpoint = island::FixedViewpoint::BoarDetail;
            } else if (name == "butterflies") {
                viewpoint = island::FixedViewpoint::ButterflyDetail;
            } else if (name == "turtles") {
                viewpoint = island::FixedViewpoint::SeaTurtleDetail;
            } else if (name == "traversal") {
                viewpoint = island::FixedViewpoint::BenchmarkTraversal;
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
        } else if (argument == "--startup-report") {
            if (++index >= argc) {
                std::cerr << "island-simulator: --startup-report requires a JSON output path\n";
                return 2;
            }
            startupReportPath = argv[index];
        } else {
            std::cerr << "island-simulator: unknown argument '" << argument << "'\n";
            return 2;
        }
    }
    if (viewpoint == island::FixedViewpoint::CanopyInspection && !canopyLodInspection) {
        std::cerr << "island-simulator: canopy viewpoint requires --canopy-lod-inspection\n";
        return 2;
    }
    if (canopyLodInspection && (capturePath.empty() || viewpoint == island::FixedViewpoint::None
        || viewpoint == island::FixedViewpoint::BenchmarkTraversal || !benchmarkPath.empty()
        || !startupReportPath.empty() || showDebugOverlay || showWorldDebug || inputDiagnostics)) {
        std::cerr << "island-simulator: canopy LOD inspection requires --capture and a fixed "
                     "viewpoint, without benchmark, startup report or debug modes\n";
        return 2;
    }
    const std::string_view mode = canopyLodInspection ? "canopy-lod-inspection"
        : !benchmarkPath.empty() ? "benchmark"
        : !capturePath.empty() ? "capture"
        : !startupReportPath.empty() ? "startup-report"
        : inputDiagnostics ? "input-diagnostics"
        : smokeTest ? "smoke-test"
        : "interactive";
    if (!benchmarkPath.empty()
        && (smokeTest || showDebugOverlay || showWorldDebug || inputDiagnostics)) {
        std::cerr << "island-simulator: --benchmark cannot be combined with smoke, capture, "
                     "or debug modes\n";
        return 2;
    }
    if (viewpoint == island::FixedViewpoint::BenchmarkTraversal && benchmarkPath.empty()) {
        std::cerr << "island-simulator: the traversal viewpoint requires --benchmark\n";
        return 2;
    }
    island::RecordRuntimeWorkload(mode, ViewpointName(viewpoint), capturePath,
                                  benchmarkPath, startupReportPath);
    try {
        island::IslandGame game(smokeTest, std::move(capturePath), viewpoint, showDebugOverlay,
                                inputDiagnostics, showWorldDebug, std::move(benchmarkPath),
                                std::move(startupReportPath),
                                !noUserSettings && !smokeTest && !inputDiagnostics
                                    && viewpoint == island::FixedViewpoint::None,
                                canopyLodInspection);
        game.Run();
        return 0;
    } catch (const std::exception& error) {
        const std::string diagnostic =
            std::string("island-simulator: fatal error: ") + error.what();
        std::cerr << diagnostic << '\n';
        island::MirrorRuntimeDiagnosticToPlatformLog(diagnostic);
        return 1;
    } catch (...) {
        constexpr std::string_view diagnostic =
            "island-simulator: fatal error: unknown non-standard exception";
        std::cerr << diagnostic << '\n';
        island::MirrorRuntimeDiagnosticToPlatformLog(diagnostic);
        return 1;
    }
}
