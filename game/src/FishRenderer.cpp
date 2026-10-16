#include "IslandSimulator/FishRenderer.hpp"

#include "IslandSimulator/WildlifeRenderLighting.hpp"
#include "IslandSimulator/RuntimeDiagnostics.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <numbers>
#include <stdexcept>
#include <string_view>

#include "Microsoft/Xna/Framework/BoundingFrustum.hpp"
#include "Microsoft/Xna/Framework/BoundingSphere.hpp"
#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/Graphics/BasicEffect.hpp"
#include "Microsoft/Xna/Framework/Graphics/BlendState.hpp"
#include "Microsoft/Xna/Framework/Graphics/BufferUsage.hpp"
#include "Microsoft/Xna/Framework/Graphics/DepthStencilState.hpp"
#include "Microsoft/Xna/Framework/Graphics/DynamicVertexBuffer.hpp"
#include "Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp"
#include "Microsoft/Xna/Framework/Graphics/PrimitiveType.hpp"
#include "Microsoft/Xna/Framework/Graphics/RasterizerState.hpp"
#include "Microsoft/Xna/Framework/Graphics/SetDataOptions.hpp"
#include "Microsoft/Xna/Framework/Graphics/VertexPositionColor.hpp"

using namespace Microsoft::Xna::Framework;
using namespace Microsoft::Xna::Framework::Graphics;

namespace island
{
    namespace
    {
        constexpr float NearLodDistance = 72.0f;
        constexpr int MaximumVerticesPerFish = FishRenderer::NearVertexCount;
        constexpr int MaximumVertexCount = static_cast<int>(FishSchools::FishCount)
            * MaximumVerticesPerFish;

        struct FishPalette
        {
            Color back;
            Color body;
            Color belly;
            Color fin;
        };

        struct PoseAxes
        {
            Vector3 right;
            Vector3 up;
            Vector3 forward;
        };

        const std::array<FishPalette, 4> Palettes = {{
            {Color(66, 111, 117, 255), Color(126, 166, 166, 255),
             Color(207, 214, 184, 255),
             Color(105, 143, 132, 255)},
            {Color(21, 76, 91, 255), Color(41, 126, 138, 255),
             Color(125, 191, 175, 255),
             Color(58, 106, 118, 255)},
            {Color(102, 79, 51, 255), Color(174, 133, 74, 255),
             Color(223, 194, 129, 255),
             Color(143, 109, 64, 255)},
            {Color(28, 48, 91, 255), Color(59, 82, 126, 255),
             Color(140, 168, 180, 255),
             Color(82, 108, 137, 255)}
        }};

        struct BodySection
        {
            float z;
            float halfWidth;
            float halfHeight;
        };

        constexpr std::array<BodySection, 11> BodySections = {{
            {0.55f, 0.030f, 0.027f},
            {0.50f, 0.074f, 0.066f},
            {0.40f, 0.122f, 0.108f},
            {0.27f, 0.158f, 0.143f},
            {0.12f, 0.185f, 0.175f},
            {-0.04f, 0.195f, 0.185f},
            {-0.18f, 0.174f, 0.165f},
            {-0.30f, 0.135f, 0.123f},
            {-0.43f, 0.085f, 0.078f},
            {-0.52f, 0.048f, 0.045f},
            {-0.56f, 0.025f, 0.026f},
        }};
        constexpr int BodySegments = 16;
        constexpr std::string_view FishDetailDiagnostic =
            "island-simulator: near fish use eleven-station sixteen-sided bodies, closed swept fins, and fitted highlighted eyes";
        constexpr std::string_view FishFinDiagnostic =
            "island-simulator: fish fins inherit body-family pigment with dark fitted root edges";
        constexpr std::string_view FishPatternDiagnostic =
            "island-simulator: near fish use softly interpolated family flank lines, bars, and caudal marks";
        constexpr std::string_view FishTailDiagnostic =
            "island-simulator: fish use narrow caudal peduncles and shallow swept forked tails";
        constexpr std::string_view FishProfileDiagnostic =
            "island-simulator: fish profile pass uses six-facet oval eyes, tapered fork tips, and a lower rear-swept dorsal";
        constexpr std::string_view FishHeadDiagnostic =
            "island-simulator: near fish use blunt convex foreheads and rounded snouts instead of needle-point wedges";
        constexpr std::string_view FishSurfaceDiagnostic =
            "island-simulator: near fish use restrained upper-flank iridescence and palette-matched gill creases";
        constexpr std::string_view FishEyeDiagnostic =
            "island-simulator: near fish eyes use palette-tinted oval irises, inset dark pupils, and raised highlights";

        void AddTriangle(std::vector<VertexPositionColor>& vertices,
                         const Vector3& a, const Vector3& b, const Vector3& c,
                         const Color& color)
        {
            vertices.emplace_back(a, color);
            vertices.emplace_back(b, color);
            vertices.emplace_back(c, color);
        }

        void AddLitTriangle(std::vector<VertexPositionColor>& vertices,
                            const Vector3& a, const Vector3& b, const Vector3& c,
                            const Color& color, const bool twoSided = false)
        {
            AddTriangle(vertices, a, b, c,
                        wildlife_render::FacetColor(a, b, c, color, twoSided));
        }

        void AddLitQuad(std::vector<VertexPositionColor>& vertices,
                        const Vector3& a, const Vector3& b,
                        const Vector3& c, const Vector3& d, const Color& color)
        {
            AddLitTriangle(vertices, a, b, c, color);
            AddLitTriangle(vertices, a, c, d, color);
        }

        template <std::size_t Count>
        void AppendClosedPolygon(std::vector<VertexPositionColor>& vertices,
                                 const std::array<Vector3, Count>& outline,
                                 const Vector3& halfThickness,
                                 const Color& face, const Color& edge)
        {
            static_assert(Count >= 3U);
            std::array<Vector3, Count> positive{}, negative{};
            for (std::size_t point = 0; point < Count; ++point) {
                positive[point] = outline[point] + halfThickness;
                negative[point] = outline[point] - halfThickness;
            }
            for (std::size_t point = 1; point + 1U < Count; ++point) {
                AddLitTriangle(vertices, positive[0], positive[point], positive[point + 1U], face);
                AddLitTriangle(vertices, negative[0], negative[point + 1U], negative[point], face);
            }
            for (std::size_t point = 0; point < Count; ++point) {
                const std::size_t next = (point + 1U) % Count;
                AddLitQuad(vertices, positive[point], negative[point], negative[next],
                           positive[next], edge);
            }
        }

        [[nodiscard]] Color BodyColor(const FishPalette& palette, const float vertical)
        {
            if (vertical < 0.0f)
                return Color::Lerp(palette.body, palette.belly,
                                   std::clamp(-vertical * .88f, 0.0f, 1.0f));
            return Color::Lerp(palette.body, palette.back,
                               std::clamp(vertical * .82f, 0.0f, 1.0f));
        }

        [[nodiscard]] float SoftBand(const float value, const float centre,
                                     const float halfWidth)
        {
            const float distance = std::abs(value - centre);
            return 1.0f - std::clamp((distance - halfWidth * .42f)
                / (halfWidth * .58f), 0.0f, 1.0f);
        }

        [[nodiscard]] Color SmoothBodyColor(const FishPalette& palette,
                                            const PoseAxes& axes, const float angle,
                                            const float longitudinal,
                                            const std::uint8_t variant)
        {
            const Vector3 normal = Vector3::Normalize(
                axes.right * std::cos(angle) + axes.up * std::sin(angle));
            const float direct = std::max(
                0.0f, Vector3::Dot(normal, wildlife_render::FixedSunDirection));
            const float sky = std::max(0.0f, normal.Y);
            Color patterned = BodyColor(palette, std::sin(angle));
            // Pigment lives mainly on the two flanks and fades continuously over the back/
            // belly. Four bounded families prevent the school collapsing to one cyan mass
            // after shared underwater attenuation, without adding a texture or geometry.
            const float flank = 1.0f - std::clamp((std::abs(std::sin(angle)) - .18f) / .42f,
                                                  0.0f, 1.0f);
            switch (variant % 4U) {
            case 0U: {
                const float line = flank * SoftBand(std::sin(angle), 0.0f, .20f)
                    * (1.0f - SoftBand(longitudinal, .48f, .10f));
                patterned = Color::Lerp(patterned, palette.back, line * .30f);
                patterned = wildlife_render::ScaleColor(patterned, 1.0f - line * .13f);
                break;
            }
            case 1U: {
                const float bars = std::max(SoftBand(longitudinal, .18f, .095f),
                                            SoftBand(longitudinal, -.17f, .10f));
                patterned = Color::Lerp(patterned, palette.back, flank * bars * .27f);
                patterned = wildlife_render::ScaleColor(patterned,
                    1.0f - flank * bars * .16f);
                break;
            }
            case 2U: {
                const float centreFlank = flank * SoftBand(longitudinal, .04f, .30f);
                patterned = Color::Lerp(patterned, palette.belly, centreFlank * .22f);
                patterned = wildlife_render::ScaleColor(patterned,
                    1.0f + centreFlank * .10f);
                const float tailSpot = flank * SoftBand(longitudinal, -.34f, .075f);
                patterned = Color::Lerp(patterned, palette.back, tailSpot * .42f);
                patterned = wildlife_render::ScaleColor(patterned,
                    1.0f - tailSpot * .20f);
                break;
            }
            default: {
                const float bars = std::max(SoftBand(longitudinal, .28f, .075f),
                    std::max(SoftBand(longitudinal, .01f, .080f),
                             SoftBand(longitudinal, -.25f, .075f)));
                patterned = Color::Lerp(patterned, palette.back, flank * bars * .24f);
                patterned = wildlife_render::ScaleColor(patterned,
                    1.0f - flank * bars * .14f);
                break;
            }
            }
            // A broad, low-contrast shoulder sheen breaks up the otherwise uninterrupted
            // vertex-colour flank without painting a checkerboard across the coarse rings.
            // It follows the rounded surface, fades before the head and peduncle, and keeps
            // every family rooted in its existing underwater-adjusted palette.
            const float shoulder = flank * SoftBand(std::sin(angle), .27f, .24f)
                * SoftBand(longitudinal, .08f, .34f);
            const float sheenVariation = .78f + .22f * (
                .5f + .5f * std::sin((longitudinal + .56f) * 7.2f
                                     + static_cast<float>(variant % 4U) * 1.31f));
            const float sheen = shoulder * sheenVariation;
            patterned = Color::Lerp(patterned, palette.belly, sheen * .14f);
            patterned = wildlife_render::ScaleColor(patterned, 1.0f + sheen * .055f);
            return wildlife_render::ScaleColor(patterned,
                                               .76f + .25f * direct + .05f * sky);
        }

        void AddGradientTriangle(std::vector<VertexPositionColor>& vertices,
                                 const Vector3& a, const Vector3& b, const Vector3& c,
                                 const Color& colorA, const Color& colorB,
                                 const Color& colorC)
        {
            vertices.emplace_back(a, colorA);
            vertices.emplace_back(b, colorB);
            vertices.emplace_back(c, colorC);
        }

        [[nodiscard]] PoseAxes MakePoseAxes(const FishState& fish)
        {
            const Vector3 levelForward(std::cos(fish.headingRadians), 0.0f,
                                       std::sin(fish.headingRadians));
            const Vector3 levelRight(-levelForward.Z, 0.0f, levelForward.X);
            const float pitchCosine = std::cos(fish.pitchRadians);
            const float pitchSine = std::sin(fish.pitchRadians);
            const Vector3 forward = levelForward * pitchCosine + Vector3::Up * pitchSine;
            const Vector3 pitchUp = Vector3::Up * pitchCosine - levelForward * pitchSine;
            const float bankCosine = std::cos(fish.bankRadians);
            const float bankSine = std::sin(fish.bankRadians);
            return PoseAxes{
                levelRight * bankCosine + pitchUp * bankSine,
                pitchUp * bankCosine - levelRight * bankSine,
                forward,
            };
        }

        [[nodiscard]] Vector3 LocalPoint(const Vector3& centre, const PoseAxes& axes,
                                         const float localX,
                                         const float localY, const float localZ,
                                         const float scale)
        {
            return centre + axes.right * (localX * scale) + axes.up * (localY * scale)
                + axes.forward * (localZ * scale);
        }

        void AppendNearFish(std::vector<VertexPositionColor>& vertices,
                            const FishState& fish, const float timeSeconds)
        {
            const std::size_t startingVertexCount = vertices.size();
            const Vector3 centre(fish.x, fish.y, fish.z);
            const FishPalette& sourcePalette = Palettes[fish.colorVariant % Palettes.size()];
            const AquaticLightProfile waterLight = AquaticLightAt(
                centre.X, centre.Y, centre.Z, timeSeconds);
            const FishPalette palette{
                wildlife_render::ApplyUnderwaterLight(sourcePalette.back, waterLight),
                wildlife_render::ApplyUnderwaterLight(sourcePalette.body, waterLight),
                wildlife_render::ApplyUnderwaterLight(sourcePalette.belly, waterLight),
                wildlife_render::ApplyUnderwaterLight(sourcePalette.fin, waterLight),
            };
            const PoseAxes axes = MakePoseAxes(fish);
            const float scale = fish.scale;
            const float widthScale = fish.colorVariant % 4U == 1U ? 0.88f : 1.0f;
            const float heightScale = fish.colorVariant % 4U == 2U ? 1.12f : 1.0f;
            std::array<std::array<Vector3, BodySegments>, BodySections.size()> rings{};
            for (std::size_t row = 0; row < BodySections.size(); ++row) {
                const BodySection& section = BodySections[row];
                const float rearWeight = std::clamp((0.20f - section.z) / 0.65f, 0.0f, 1.0f);
                const float bend = fish.bodyWave * 0.105f * rearWeight * rearWeight;
                for (int segment = 0; segment < BodySegments; ++segment) {
                    const float angle = 2.0f * std::numbers::pi_v<float>
                        * static_cast<float>(segment) / static_cast<float>(BodySegments);
                    rings[row][segment] = LocalPoint(
                        centre, axes,
                        bend + section.halfWidth * widthScale * std::cos(angle),
                        section.halfHeight * heightScale * std::sin(angle), section.z, scale);
                }
            }
            for (std::size_t row = 0; row + 1U < BodySections.size(); ++row) {
                for (int segment = 0; segment < BodySegments; ++segment) {
                    const int next = (segment + 1) % BodySegments;
                    const float angle = 2.0f * std::numbers::pi_v<float>
                        * static_cast<float>(segment) / static_cast<float>(BodySegments);
                    const float nextAngle = 2.0f * std::numbers::pi_v<float>
                        * static_cast<float>(next) / static_cast<float>(BodySegments);
                    const Color color = SmoothBodyColor(palette, axes, angle,
                        BodySections[row].z, fish.colorVariant);
                    const Color nextColor = SmoothBodyColor(palette, axes, nextAngle,
                        BodySections[row].z, fish.colorVariant);
                    const Color forwardColor = SmoothBodyColor(palette, axes, angle,
                        BodySections[row + 1U].z, fish.colorVariant);
                    const Color nextForwardColor = SmoothBodyColor(palette, axes, nextAngle,
                        BodySections[row + 1U].z, fish.colorVariant);
                    AddGradientTriangle(vertices, rings[row][segment], rings[row + 1U][segment],
                                        rings[row + 1U][next], color, forwardColor,
                                        nextForwardColor);
                    AddGradientTriangle(vertices, rings[row][segment], rings[row + 1U][next],
                                        rings[row][next], color, nextForwardColor, nextColor);
                }
            }

            const auto appendBodyCap = [&](const std::size_t row, const float z,
                                           const bool front) {
                const Vector3 cap = LocalPoint(centre, axes,
                    fish.bodyWave * 0.105f
                        * std::pow(std::clamp((0.20f - z) / 0.65f, 0.0f, 1.0f), 2.0f),
                    0.0f, z, scale);
                for (int segment = 0; segment < BodySegments; ++segment) {
                    const int next = (segment + 1) % BodySegments;
                    const float angle = 2.0f * std::numbers::pi_v<float>
                        * static_cast<float>(segment) / static_cast<float>(BodySegments);
                    const float nextAngle = 2.0f * std::numbers::pi_v<float>
                        * static_cast<float>(next) / static_cast<float>(BodySegments);
                    const Color color = SmoothBodyColor(palette, axes, angle, z,
                                                        fish.colorVariant);
                    const Color nextColor = SmoothBodyColor(palette, axes, nextAngle, z,
                                                            fish.colorVariant);
                    if (front)
                        AddGradientTriangle(vertices, rings[row][segment],
                                            rings[row][next], cap,
                                            color, nextColor, palette.body);
                    else
                        AddGradientTriangle(vertices, rings[row][segment], cap,
                                            rings[row][next],
                                            color, palette.body, nextColor);
                }
            };
            appendBodyCap(0U, .562f, true);
            appendBodyCap(BodySections.size() - 1U, -.575f, false);

            const float bend = fish.bodyWave * 0.105f;
            const Vector3 rear = LocalPoint(centre, axes, bend, 0.0f, -0.56f, scale);

            const float tailBend = fish.bodyWave * 0.17f;
            const auto tailPoint = [&](const float y, const float z, const float wave) {
                return LocalPoint(centre, axes, tailBend * wave, y, z, scale);
            };
            const std::array upperTail{rear, tailPoint(.045f,-.65f,1.02f),
                tailPoint(.135f,-.86f,1.09f), tailPoint(.080f,-.92f,1.11f),
                tailPoint(0,-.79f,1.07f)};
            const std::array lowerTail{rear, tailPoint(0,-.79f,1.07f),
                tailPoint(-.080f,-.92f,1.11f), tailPoint(-.135f,-.86f,1.09f),
                tailPoint(-.045f,-.65f,1.02f)};
            AppendClosedPolygon(vertices, upperTail, axes.right * (0.004f * scale),
                                palette.fin, palette.back);
            AppendClosedPolygon(vertices, lowerTail, axes.right * (0.004f * scale),
                                palette.fin, palette.back);

            const std::array dorsal{
                LocalPoint(centre, axes, 0, .14f, .15f, scale),
                LocalPoint(centre, axes, 0, .19f, .05f, scale),
                LocalPoint(centre, axes, 0, .235f, -.05f, scale),
                LocalPoint(centre, axes, bend*.18f, .25f, -.12f, scale),
                LocalPoint(centre, axes, bend*.38f, .18f, -.22f, scale),
                LocalPoint(centre, axes, bend*.50f, .12f, -.30f, scale)};
            AppendClosedPolygon(vertices, dorsal, axes.right * (0.006f * scale),
                                palette.fin, palette.back);

            for (const float side : {-1.0f, 1.0f}) {
                const std::array pectoral{
                    LocalPoint(centre, axes, side*.145f,-.015f,.13f,scale),
                    LocalPoint(centre, axes, side*.175f,-.038f,.035f,scale),
                    LocalPoint(centre, axes, side*.22f,-.058f,-.06f,scale),
                    LocalPoint(centre, axes, side*.205f,-.064f,-.115f,scale),
                    LocalPoint(centre, axes, side*.17f,-.055f,-.16f,scale),
                    LocalPoint(centre, axes, side*.125f,-.048f,-.19f,scale)};
                AppendClosedPolygon(vertices, pectoral, axes.up * (0.004f * scale),
                                    palette.fin, palette.back);
            }

            const Color iris = wildlife_render::ScaleColor(
                Color::Lerp(palette.belly, palette.fin, .42f), .84f);
            const Color pupil(19, 31, 33, 255);
            const Color eyeShine(132, 158, 151, 255);
            const Color gill = wildlife_render::ScaleColor(
                Color::Lerp(palette.back, palette.body, .38f), .76f);
            for (const float side : {-1.0f, 1.0f}) {
                constexpr int EyeSegments = 6;
                const Vector3 eyeCentre = LocalPoint(
                    centre, axes, side * .159f, .045f, .283f, scale);
                for (int segment = 0; segment < EyeSegments; ++segment) {
                    const float first = 2.0f * std::numbers::pi_v<float>
                        * static_cast<float>(segment) / static_cast<float>(EyeSegments);
                    const float next = 2.0f * std::numbers::pi_v<float>
                        * static_cast<float>(segment + 1) / static_cast<float>(EyeSegments);
                    const auto edge = [&](const float angle) {
                        return LocalPoint(centre, axes, side * .161f,
                            .045f + std::sin(angle) * .032f,
                            .283f + std::cos(angle) * .036f, scale);
                    };
                    if (side > 0.0f)
                        AddTriangle(vertices, eyeCentre, edge(next), edge(first), iris);
                    else AddTriangle(vertices, eyeCentre, edge(first), edge(next), iris);
                }
                const Vector3 pupilCentre = LocalPoint(
                    centre, axes, side * .163f, .045f, .283f, scale);
                for (int segment = 0; segment < EyeSegments; ++segment) {
                    const float first = 2.0f * std::numbers::pi_v<float>
                        * static_cast<float>(segment) / static_cast<float>(EyeSegments);
                    const float next = 2.0f * std::numbers::pi_v<float>
                        * static_cast<float>(segment + 1) / static_cast<float>(EyeSegments);
                    const auto pupilEdge = [&](const float angle) {
                        return LocalPoint(centre, axes, side * .164f,
                            .045f + std::sin(angle) * .018f,
                            .283f + std::cos(angle) * .020f, scale);
                    };
                    if (side > 0.0f)
                        AddTriangle(vertices, pupilCentre,
                                    pupilEdge(next), pupilEdge(first), pupil);
                    else AddTriangle(vertices, pupilCentre,
                                     pupilEdge(first), pupilEdge(next), pupil);
                }
                AddTriangle(vertices,
                    LocalPoint(centre, axes, side * 0.165f, 0.060f, 0.291f, scale),
                    LocalPoint(centre, axes, side * 0.166f, 0.051f, 0.297f, scale),
                    LocalPoint(centre, axes, side * 0.166f, 0.050f, 0.284f, scale), eyeShine);
            }
            AddTriangle(vertices,
                        LocalPoint(centre, axes, -0.190f, 0.070f, 0.145f, scale),
                        LocalPoint(centre, axes, -0.192f, -0.060f, 0.115f, scale),
                        LocalPoint(centre, axes, -0.193f, -0.042f, 0.158f, scale), gill);
            AddTriangle(vertices,
                        LocalPoint(centre, axes, 0.190f, 0.070f, 0.145f, scale),
                        LocalPoint(centre, axes, 0.193f, -0.042f, 0.158f, scale),
                        LocalPoint(centre, axes, 0.192f, -0.060f, 0.115f, scale), gill);
            for (const float side : {-1.0f, 1.0f}) {
                AddTriangle(vertices,
                            LocalPoint(centre, axes, side * 0.035f, -0.027f, 0.486f, scale),
                            LocalPoint(centre, axes, side * 0.110f, -0.034f, 0.405f, scale),
                            LocalPoint(centre, axes, side * 0.040f, -0.045f, 0.465f, scale),
                            gill);
            }
            if (vertices.size() - startingVertexCount != FishRenderer::NearVertexCount) {
                throw std::runtime_error("Fish near mesh does not match its fixed vertex budget");
            }
        }

        void AppendFarFish(std::vector<VertexPositionColor>& vertices,
                           const FishState& fish, const float timeSeconds)
        {
            const Vector3 centre(fish.x, fish.y, fish.z);
            const FishPalette& sourcePalette = Palettes[fish.colorVariant % Palettes.size()];
            const AquaticLightProfile waterLight = AquaticLightAt(
                centre.X, centre.Y, centre.Z, timeSeconds);
            const FishPalette palette{
                wildlife_render::ApplyUnderwaterLight(sourcePalette.back, waterLight),
                wildlife_render::ApplyUnderwaterLight(sourcePalette.body, waterLight),
                wildlife_render::ApplyUnderwaterLight(sourcePalette.belly, waterLight),
                wildlife_render::ApplyUnderwaterLight(sourcePalette.fin, waterLight),
            };
            const PoseAxes axes = MakePoseAxes(fish);
            const float scale = fish.scale;
            const float bend = fish.bodyWave * 0.12f;
            const Vector3 nose = LocalPoint(centre, axes, 0.0f, 0.0f, 0.46f, scale);
            const Vector3 top = LocalPoint(centre, axes, 0.0f, 0.17f, 0.0f, scale);
            const Vector3 bottom = LocalPoint(centre, axes, 0.0f, -0.14f, 0.0f, scale);
            const Vector3 rear = LocalPoint(centre, axes, bend, 0.0f, -0.49f, scale);
            const Vector3 tailTop = LocalPoint(
                centre, axes, bend * 1.5f, 0.15f, -0.82f, scale);
            const Vector3 tailNotch = LocalPoint(
                centre, axes, bend * 1.62f, 0.0f, -0.73f, scale);
            const Vector3 tailBottom = LocalPoint(
                centre, axes, bend * 1.5f, -0.15f, -0.82f, scale);
            AddLitTriangle(vertices, nose, top, rear, palette.body, true);
            AddLitTriangle(vertices, nose, rear, bottom, palette.belly, true);
            AddTriangle(vertices, rear, tailTop, tailNotch, palette.fin);
            AddTriangle(vertices, rear, tailNotch, tailBottom, palette.fin);
        }
    }

    FishRenderer::FishRenderer(GraphicsDevice& device, const float maximumDistance)
        : device_(device),
          effect_(std::make_unique<BasicEffect>(device)),
          vertices_(std::make_unique<DynamicVertexBuffer>(
              device, VertexPositionColor::getVertexDeclarationStatic(), MaximumVertexCount,
              BufferUsage::WriteOnly)),
          maximumDistance_(maximumDistance)
    {
        if (!std::isfinite(maximumDistance_) || maximumDistance_ <= 0.0f) {
            throw std::invalid_argument("Fish maximum draw distance must be finite and positive");
        }
        effect_->setVertexColorEnabledProperty(true);
        effect_->setLightingEnabledProperty(false);
        effect_->setFogEnabledProperty(true);
        effect_->setFogColorProperty(Vector3(0.16f, 0.55f, 0.61f));
        effect_->setFogStartProperty(70.0f);
        effect_->setFogEndProperty(220.0f);
        stagingVertices_.reserve(static_cast<std::size_t>(MaximumVertexCount));
        std::cout << FishDetailDiagnostic << '\n';
        MirrorRuntimeDiagnosticToPlatformLog(FishDetailDiagnostic);
        std::cout << FishFinDiagnostic << '\n';
        MirrorRuntimeDiagnosticToPlatformLog(FishFinDiagnostic);
        std::cout << FishPatternDiagnostic << '\n';
        MirrorRuntimeDiagnosticToPlatformLog(FishPatternDiagnostic);
        std::cout << FishTailDiagnostic << '\n';
        MirrorRuntimeDiagnosticToPlatformLog(FishTailDiagnostic);
        std::cout << FishProfileDiagnostic << '\n';
        MirrorRuntimeDiagnosticToPlatformLog(FishProfileDiagnostic);
        std::cout << FishHeadDiagnostic << '\n';
        MirrorRuntimeDiagnosticToPlatformLog(FishHeadDiagnostic);
        std::cout << FishSurfaceDiagnostic << '\n';
        MirrorRuntimeDiagnosticToPlatformLog(FishSurfaceDiagnostic);
        std::cout << FishEyeDiagnostic << '\n';
        MirrorRuntimeDiagnosticToPlatformLog(FishEyeDiagnostic);
    }

    FishRenderer::~FishRenderer() = default;

    void FishRenderer::Draw(const Matrix& view, const Matrix& projection,
                            const Vector3& cameraPosition,
                            const std::span<const FishState> fish,
                            const float timeSeconds)
    {
        stagingVertices_.clear();
        visibleFishCount_ = 0;
        visibleNearFishCount_ = 0;
        visibleFarFishCount_ = 0;
        drawCallCount_ = 0;
        const BoundingFrustum frustum(view * projection);

        for (const FishState& state : fish) {
            const Vector3 centre(state.x, state.y, state.z);
            const float distanceSquared = Vector3::DistanceSquared(centre, cameraPosition);
            if (distanceSquared > maximumDistance_ * maximumDistance_
                || !frustum.Intersects(BoundingSphere(centre, state.scale * 0.85f))) {
                continue;
            }
            if (distanceSquared <= NearLodDistance * NearLodDistance) {
                AppendNearFish(stagingVertices_, state, timeSeconds);
                ++visibleNearFishCount_;
            } else {
                AppendFarFish(stagingVertices_, state, timeSeconds);
                ++visibleFarFishCount_;
            }
            ++visibleFishCount_;
        }

        if (stagingVertices_.empty()) {
            return;
        }
        device_.setRasterizerStateProperty(RasterizerState::CullNone);
        device_.setBlendStateProperty(BlendState::Opaque);
        device_.setDepthStencilStateProperty(DepthStencilState::Default);
        vertices_->SetData(stagingVertices_.data(), 0,
                           static_cast<int>(stagingVertices_.size()), SetDataOptions::Discard);
        device_.SetVertexBuffer(vertices_.get());
        device_.setIndicesProperty(nullptr);
        effect_->setWorldProperty(Matrix::getIdentityProperty());
        effect_->setViewProperty(view);
        effect_->setProjectionProperty(projection);
        effect_->Apply();
        device_.DrawPrimitives(PrimitiveType::TriangleList, 0,
                               static_cast<int>(stagingVertices_.size() / 3U));
        ++drawCallCount_;
    }
}
