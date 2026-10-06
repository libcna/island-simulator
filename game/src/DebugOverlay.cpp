#include "IslandSimulator/DebugOverlay.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <locale>
#include <optional>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/Rectangle.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteEffects.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteFont.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"

using namespace Microsoft::Xna::Framework;
using namespace Microsoft::Xna::Framework::Graphics;

namespace island
{
    namespace
    {
        constexpr int GlyphWidth = 5;
        constexpr int GlyphHeight = 7;
        constexpr int GlyphCellWidth = 6;
        constexpr int GlyphCellHeight = 8;
        constexpr int AtlasColumns = 16;
        constexpr int FirstCharacter = 32;
        constexpr int LastCharacter = 90;

        using GlyphRows = std::array<std::uint8_t, GlyphHeight>;

        [[nodiscard]] constexpr GlyphRows Glyph(const char character) noexcept
        {
            switch (character) {
            case ' ': return {0, 0, 0, 0, 0, 0, 0};
            case '!': return {4, 4, 4, 4, 4, 0, 4};
            case '-': return {0, 0, 0, 31, 0, 0, 0};
            case '.': return {0, 0, 0, 0, 0, 0, 4};
            case '/': return {1, 2, 2, 4, 8, 8, 16};
            case ':': return {0, 4, 0, 0, 4, 0, 0};
            case '?': return {14, 17, 1, 2, 4, 0, 4};
            case '0': return {14, 17, 19, 21, 25, 17, 14};
            case '1': return {4, 12, 4, 4, 4, 4, 14};
            case '2': return {14, 17, 1, 2, 4, 8, 31};
            case '3': return {30, 1, 1, 14, 1, 1, 30};
            case '4': return {2, 6, 10, 18, 31, 2, 2};
            case '5': return {31, 16, 16, 30, 1, 1, 30};
            case '6': return {14, 16, 16, 30, 17, 17, 14};
            case '7': return {31, 1, 2, 4, 8, 8, 8};
            case '8': return {14, 17, 17, 14, 17, 17, 14};
            case '9': return {14, 17, 17, 15, 1, 1, 14};
            case 'A': return {14, 17, 17, 31, 17, 17, 17};
            case 'B': return {30, 17, 17, 30, 17, 17, 30};
            case 'C': return {14, 17, 16, 16, 16, 17, 14};
            case 'D': return {30, 17, 17, 17, 17, 17, 30};
            case 'E': return {31, 16, 16, 30, 16, 16, 31};
            case 'F': return {31, 16, 16, 30, 16, 16, 16};
            case 'G': return {14, 17, 16, 23, 17, 17, 14};
            case 'H': return {17, 17, 17, 31, 17, 17, 17};
            case 'I': return {14, 4, 4, 4, 4, 4, 14};
            case 'J': return {7, 2, 2, 2, 2, 18, 12};
            case 'K': return {17, 18, 20, 24, 20, 18, 17};
            case 'L': return {16, 16, 16, 16, 16, 16, 31};
            case 'M': return {17, 27, 21, 21, 17, 17, 17};
            case 'N': return {17, 25, 25, 21, 19, 19, 17};
            case 'O': return {14, 17, 17, 17, 17, 17, 14};
            case 'P': return {30, 17, 17, 30, 16, 16, 16};
            case 'Q': return {14, 17, 17, 17, 21, 18, 13};
            case 'R': return {30, 17, 17, 30, 20, 18, 17};
            case 'S': return {15, 16, 16, 14, 1, 1, 30};
            case 'T': return {31, 4, 4, 4, 4, 4, 4};
            case 'U': return {17, 17, 17, 17, 17, 17, 14};
            case 'V': return {17, 17, 17, 17, 17, 10, 4};
            case 'W': return {17, 17, 17, 21, 21, 21, 10};
            case 'X': return {17, 17, 10, 4, 10, 17, 17};
            case 'Y': return {17, 17, 10, 4, 4, 4, 4};
            case 'Z': return {31, 1, 2, 4, 8, 16, 31};
            default: return {14, 17, 1, 2, 4, 0, 4};
            }
        }

        [[nodiscard]] std::unique_ptr<SpriteFont> CreateDebugFont(GraphicsDevice& device)
        {
            constexpr int characterCount = LastCharacter - FirstCharacter + 1;
            constexpr int atlasRows = (characterCount + AtlasColumns - 1) / AtlasColumns;
            constexpr int atlasWidth = AtlasColumns * GlyphCellWidth;
            constexpr int atlasHeight = atlasRows * GlyphCellHeight;
            std::vector<Color> pixels(static_cast<std::size_t>(atlasWidth * atlasHeight),
                                      Color(0, 0, 0, 0));
            std::vector<Rectangle> glyphBounds;
            std::vector<Rectangle> cropping;
            std::vector<SharpRuntime::charcs> characters;
            std::vector<Vector3> kerning;
            glyphBounds.reserve(characterCount);
            cropping.reserve(characterCount);
            characters.reserve(characterCount);
            kerning.reserve(characterCount);

            for (int code = FirstCharacter; code <= LastCharacter; ++code) {
                const int index = code - FirstCharacter;
                const int originX = (index % AtlasColumns) * GlyphCellWidth;
                const int originY = (index / AtlasColumns) * GlyphCellHeight;
                const GlyphRows rows = Glyph(static_cast<char>(code));
                for (int row = 0; row < GlyphHeight; ++row) {
                    for (int column = 0; column < GlyphWidth; ++column) {
                        const std::uint8_t mask = static_cast<std::uint8_t>(1U
                            << static_cast<unsigned>(GlyphWidth - 1 - column));
                        if ((rows[static_cast<std::size_t>(row)] & mask) != 0U) {
                            pixels[static_cast<std::size_t>((originY + row) * atlasWidth
                                                            + originX + column)] = Color::White;
                        }
                    }
                }
                glyphBounds.emplace_back(originX, originY, GlyphWidth, GlyphHeight);
                cropping.emplace_back(0, 0, GlyphWidth, GlyphHeight);
                characters.push_back(static_cast<SharpRuntime::charcs>(code));
                kerning.emplace_back(0.0f, static_cast<float>(GlyphWidth), 1.0f);
            }

            Texture2D atlas(device, atlasWidth, atlasHeight);
            atlas.SetData(pixels.data(), static_cast<int>(pixels.size()));
            return std::make_unique<SpriteFont>(
                std::move(atlas), std::move(glyphBounds), std::move(cropping),
                std::move(characters), GlyphCellHeight, 0.0f, std::move(kerning),
                static_cast<SharpRuntime::charcs>('?'));
        }

        [[nodiscard]] std::ostringstream DiagnosticLine()
        {
            std::ostringstream stream;
            stream.imbue(std::locale::classic());
            stream << std::fixed << std::setprecision(1);
            return stream;
        }
    }

    DebugOverlay::DebugOverlay(GraphicsDevice& device)
        : spriteBatch_(std::make_unique<SpriteBatch>(device)),
          font_(CreateDebugFont(device)),
          whitePixel_(std::make_unique<Texture2D>(device, 1, 1))
    {
        const Color pixel = Color::White;
        whitePixel_->SetData(&pixel, 1);
    }

    DebugOverlay::~DebugOverlay() = default;

    void DebugOverlay::UpdateFrameTiming() noexcept
    {
        const auto now = std::chrono::steady_clock::now();
        if (!hasPreviousFrameTime_) {
            previousFrameTime_ = now;
            hasPreviousFrameTime_ = true;
            return;
        }

        const float sampleSeconds = std::chrono::duration<float>(now - previousFrameTime_).count();
        previousFrameTime_ = now;
        if (!(sampleSeconds > 0.0f && sampleSeconds < 1.0f)) {
            return;
        }
        const float sampleMilliseconds = sampleSeconds * 1000.0f;
        frameMilliseconds_ = frameMilliseconds_ <= 0.0f
            ? sampleMilliseconds
            : frameMilliseconds_ + (sampleMilliseconds - frameMilliseconds_) * 0.12f;
        timingWindowSeconds_ += sampleSeconds;
        ++timingWindowFrames_;
        if (timingWindowSeconds_ >= 0.5f) {
            framesPerSecond_ = static_cast<float>(timingWindowFrames_) / timingWindowSeconds_;
            timingWindowSeconds_ = 0.0f;
            timingWindowFrames_ = 0;
        }
    }

    void DebugOverlay::Draw(const DebugOverlayState& state)
    {
        std::vector<std::string> lines;
        lines.reserve(19U);
        lines.emplace_back("F1 DEBUG  F2 WORLD  F5-F12 TELEPORT  M MUTE");

        auto timing = DiagnosticLine();
        timing << "FPS " << framesPerSecond_ << "  FRAME " << frameMilliseconds_ << " MS";
        lines.push_back(timing.str());

        auto position = DiagnosticLine();
        position << "POS X:" << state.playerPosition.X << " Y:" << state.playerPosition.Y
                 << " Z:" << state.playerPosition.Z;
        lines.push_back(position.str());

        auto locomotion = DiagnosticLine();
        locomotion << "STATE " << (state.swimming ? "SWIMMING" : "WALKING")
                   << "  GROUND " << state.groundHeight
                   << "  SLOPE " << state.groundSlopeDegrees << " DEG";
        lines.push_back(locomotion.str());

        auto water = DiagnosticLine();
        water << "OFFSHORE " << state.offshoreDistance
              << " M  CURRENT " << state.oceanCurrentStrength
              << "  RIPPLE " << state.waterRippleIntensity
              << " PHASE " << state.waterRipplePhase
              << "  VIEW " << state.waterViewIntensity
              << " DRAWS " << state.waterViewDraws;
        lines.push_back(water.str());

        auto worldDebug = DiagnosticLine();
        worldDebug << "WORLD DEBUG " << (state.worldDebugVisible ? "ON" : "OFF")
                   << "  OBSTACLES " << state.worldDebugObstacles
                   << "  COLLISION " << state.collisionCandidates << '/'
                   << state.collisionObstacles
                   << " CELLS " << state.worldDebugCells
                   << " WILDLIFE " << state.worldDebugWildlife
                   << " LINES " << state.worldDebugLines
                   << " DRAWS " << state.worldDebugDraws;
        lines.push_back(worldDebug.str());

        lines.push_back("RENDERER " + state.renderer);

        auto terrain = DiagnosticLine();
        terrain << "TERRAIN CELLS " << state.visibleTerrainCells << '/'
                << state.totalTerrainCells << "  DISTANT " << state.distantIslets
                << " ISLETS " << state.distantSceneryDraws << " DRAW";
        lines.push_back(terrain.str());

        auto shadows = DiagnosticLine();
        shadows << "CONTACT SHADOWS " << state.visibleContactShadows
                << "  P " << state.visiblePalmShadows
                << " C " << state.visibleCanopyShadows
                << " R " << state.visibleRockShadows
                << " W " << state.visibleWoodShadows
                << " M " << state.visibleMonkeyShadows
                << "  DRAWS " << state.contactShadowDraws;
        lines.push_back(shadows.str());

        auto palms = DiagnosticLine();
        palms << "PALMS " << state.visiblePalms << '/' << state.totalPalms
              << "  NEAR " << state.nearPalms << " FAR " << state.farPalms
              << "  CELLS " << state.palmCells << " DRAWS " << state.palmDraws;
        lines.push_back(palms.str());

        auto jungle = DiagnosticLine();
        jungle << "JUNGLE C " << state.visibleCanopy << '/' << state.totalCanopy
               << " U " << state.visibleUnderstory << '/' << state.totalUnderstory
               << " B " << state.visibleBushes << '/' << state.totalBushes
               << " G " << state.visibleGroundPlants << '/' << state.totalGroundPlants
               << "  CELLS " << state.jungleCells << " DRAWS " << state.jungleDraws;
        lines.push_back(jungle.str());

        auto props = DiagnosticLine();
        props << "PROPS R " << state.visibleRocks << '/' << state.totalRocks
              << " REEF " << state.visibleReefRocks << '/' << state.totalReefRocks
              << " L " << state.visibleLogs << '/' << state.totalLogs
              << " D " << state.visibleDriftwood << '/' << state.totalDriftwood
              << " F " << state.visibleFallenFronds << '/' << state.totalFallenFronds
              << " S " << state.visibleBeachDetails << '/' << state.totalBeachDetails
              << "  CELLS " << state.propCells << " DRAWS " << state.propDraws;
        lines.push_back(props.str());

        auto grass = DiagnosticLine();
        grass << "GRASS " << state.visibleGrassPatches << '/' << state.totalGrassPatches
              << "  CELLS " << state.grassCells << " DRAWS " << state.grassDraws;
        lines.push_back(grass.str());

        auto birds = DiagnosticLine();
        birds << "BIRDS " << state.visibleBirds << '/' << state.totalBirds
              << "  NEAR " << state.nearBirds << " FAR " << state.farBirds
              << "  SIM " << state.fullSimulationBirds << '/'
              << state.reducedSimulationBirds << '/' << state.dormantSimulationBirds
              << "  DRAWS " << state.birdDraws;
        lines.push_back(birds.str());

        auto fish = DiagnosticLine();
        fish << "FISH " << state.visibleFish << '/' << state.totalFish
             << "  NEAR " << state.nearFish << " FAR " << state.farFish
             << "  SIM " << state.fullSimulationFish << '/'
             << state.reducedSimulationFish << '/' << state.dormantSimulationFish
             << "  DRAWS " << state.fishDraws;
        lines.push_back(fish.str());

        const char* sharkSimulation = state.sharkSimulationTier == 0
            ? "FULL" : (state.sharkSimulationTier == 1 ? "REDUCED" : "DORMANT");
        auto shark = DiagnosticLine();
        shark << "SHARK " << (state.sharkVisible ? "VISIBLE" : "CULLED")
              << ' ' << (state.sharkPresent ? "PRESENT" : "DIVING")
              << "  ACT " << state.sharkActivity
              << " LOD " << (state.sharkNear ? "NEAR" : "FAR")
              << " SIM " << sharkSimulation
              << " CYCLES " << state.sharkPresenceCycles
              << " DRAWS " << state.sharkDraws;
        lines.push_back(shark.str());

        auto monkeys = DiagnosticLine();
        monkeys << "MONKEYS " << state.visibleMonkeys << '/' << state.totalMonkeys
                << "  NEAR " << state.nearMonkeys << " FAR " << state.farMonkeys
                << " SIM " << state.fullSimulationMonkeys << '/'
                << state.reducedSimulationMonkeys << '/' << state.dormantSimulationMonkeys
                << " ST " << state.idleMonkeys << '/' << state.observeMonkeys << '/'
                << state.walkingMonkeys << '/' << state.runningMonkeys
                << " DRAWS " << state.monkeyDraws
                << " AVOID " << state.monkeyObstacleDeflections
                << " XFADE " << state.activeMonkeyCrossfades << '/'
                << state.completedMonkeyCrossfades;
        lines.push_back(monkeys.str());

        auto audio = DiagnosticLine();
        audio << "AUDIO " << (state.audioAvailable ? (state.audioMuted ? "MUTED" : "ACTIVE")
                                                       : "UNAVAILABLE")
              << "  SURF " << state.surfVolume << " BREEZE " << state.breezeVolume
              << " JUNGLE " << state.jungleVolume;
        audio << " WATER " << state.waterMovementVolume
              << " UNDERTOW " << state.oceanCurrentAudioCue
              << " WAVES " << state.playedWaveAccents
              << " SPLASHES " << state.playedWaterTransitions;
        lines.push_back(audio.str());

        auto wildlifeAudio = DiagnosticLine();
        wildlifeAudio << "POSITIONAL CALLS  BIRD " << state.playedBirdCalls
                      << " MONKEY " << state.playedMonkeyCalls
                      << "  VOICE " << (state.wildlifeCallActive ? "ACTIVE" : "IDLE");
        lines.push_back(wildlifeAudio.str());

        constexpr float scale = 1.45f;
        constexpr float lineHeight = 13.0f;
        constexpr int panelX = 10;
        constexpr int panelY = 10;
        constexpr int padding = 10;
        float maximumWidth = 0.0f;
        for (const std::string& line : lines) {
            maximumWidth = std::max(maximumWidth, font_->MeasureString(line).X * scale);
        }
        const int panelWidth = static_cast<int>(std::ceil(maximumWidth)) + padding * 2;
        const int panelHeight = static_cast<int>(lines.size() * lineHeight) + padding * 2;
        const Rectangle panel(panelX, panelY, panelWidth, panelHeight);

        spriteBatch_->Begin();
        spriteBatch_->Draw(*whitePixel_, panel, Color(4, 20, 22, 218));
        spriteBatch_->Draw(*whitePixel_, Rectangle(panelX, panelY, panelWidth, 2),
                           Color(92, 217, 176, 255));
        float y = static_cast<float>(panelY + padding);
        for (std::size_t index = 0; index < lines.size(); ++index) {
            const Color color = index == 0U
                ? Color(121, 238, 195, 255)
                : Color(232, 246, 238, 255);
            spriteBatch_->DrawString(*font_, lines[index],
                                     Vector2(static_cast<float>(panelX + padding), y), color,
                                     0.0f, Vector2::Zero, scale, SpriteEffects::None, 0.0f);
            y += lineHeight;
        }
        spriteBatch_->End();
    }
}
