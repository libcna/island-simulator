#include "IslandSimulator/SeaTurtleRenderer.hpp"

#include "IslandSimulator/RuntimeDiagnostics.hpp"
#include "IslandSimulator/SeaTurtleProfile.hpp"
#include "IslandSimulator/WildlifeRenderLighting.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <numbers>
#include <stdexcept>
#include <string_view>
#include <vector>

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
        constexpr float NearLodDistance = 74.0f;
        constexpr int MaximumVertexCount = static_cast<int>(SeaTurtleGroup::TurtleCount)
            * SeaTurtleRenderer::NearVertexCount;
        constexpr std::string_view TurtleDiagnostic =
            "island-simulator: sea turtles use faceted shells and articulated flippers in one culled batch";
        constexpr std::string_view TurtleFormDiagnostic =
            "island-simulator: sea turtles use domed scute carapaces, beaked heads and bending paddle strokes";
        constexpr std::string_view TurtleDetailDiagnostic =
            "island-simulator: near turtles use subdued scute seams, fuller heads, nostrils and mouth creases";
        constexpr std::string_view TurtleScaleDiagnostic =
            "island-simulator: near turtles use staggered scutes and closed tapered-volume paddles";

        struct TurtlePalette
        {
            Color shell;
            Color scute;
            Color underside;
            Color skin;
        };

        struct PoseAxes
        {
            Vector3 right;
            Vector3 up;
            Vector3 forward;
        };

        const std::array<TurtlePalette, 3> Palettes = {{
            {Color(75, 104, 69, 255), Color(113, 132, 75, 255),
             Color(171, 164, 105, 255), Color(91, 119, 82, 255)},
            {Color(73, 91, 65, 255), Color(132, 119, 72, 255),
             Color(183, 158, 101, 255), Color(103, 111, 75, 255)},
            {Color(57, 92, 81, 255), Color(102, 132, 91, 255),
             Color(162, 169, 121, 255), Color(72, 113, 97, 255)}
        }};

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

        void AddDoubleSidedTriangle(std::vector<VertexPositionColor>& vertices,
                                    const Vector3& a, const Vector3& b, const Vector3& c,
                                    const Color& color)
        {
            const Color lit = wildlife_render::FacetColor(a, b, c, color, true);
            AddTriangle(vertices, a, b, c, lit);
            AddTriangle(vertices, c, b, a, lit);
        }

        void AddQuad(std::vector<VertexPositionColor>& vertices,
                     const Vector3& a, const Vector3& b,
                     const Vector3& c, const Vector3& d, const Color& color)
        {
            AddLitTriangle(vertices, a, b, c, color);
            AddLitTriangle(vertices, a, c, d, color);
        }

        [[nodiscard]] PoseAxes MakePoseAxes(const SeaTurtleState& turtle)
        {
            const Vector3 levelForward(std::cos(turtle.headingRadians), 0.0f,
                                       std::sin(turtle.headingRadians));
            const Vector3 levelRight(-levelForward.Z, 0.0f, levelForward.X);
            const float pitchCosine = std::cos(turtle.pitchRadians);
            const float pitchSine = std::sin(turtle.pitchRadians);
            const Vector3 forward = levelForward * pitchCosine + Vector3::Up * pitchSine;
            const Vector3 pitchUp = Vector3::Up * pitchCosine - levelForward * pitchSine;
            const float bankCosine = std::cos(turtle.bankRadians);
            const float bankSine = std::sin(turtle.bankRadians);
            return PoseAxes{
                levelRight * bankCosine + pitchUp * bankSine,
                pitchUp * bankCosine - levelRight * bankSine,
                forward,
            };
        }

        [[nodiscard]] Vector3 LocalPoint(const Vector3& centre, const PoseAxes& axes,
                                         const float x, const float y, const float z,
                                         const float scale)
        {
            return centre + axes.right * (x * scale) + axes.up * (y * scale)
                + axes.forward * (z * scale);
        }

        [[nodiscard]] TurtlePalette LitPalette(const SeaTurtleState& turtle,
                                               const float timeSeconds)
        {
            const TurtlePalette& source = Palettes[turtle.colorVariant % Palettes.size()];
            const AquaticLightProfile light = AquaticLightAt(
                turtle.x, turtle.y, turtle.z, timeSeconds);
            return TurtlePalette{
                wildlife_render::ApplyUnderwaterLight(source.shell, light),
                wildlife_render::ApplyUnderwaterLight(source.scute, light),
                wildlife_render::ApplyUnderwaterLight(source.underside, light),
                wildlife_render::ApplyUnderwaterLight(source.skin, light),
            };
        }

        // One-sided facet lit so that its normal points away from a reference point.
        void AddOutwardTriangle(std::vector<VertexPositionColor>& vertices,
                                const Vector3& a, const Vector3& b, const Vector3& c,
                                const Color& color, const Vector3& inside,
                                const float sheen = 0.0f)
        {
            Vector3 normal = Vector3::Cross(b - a, c - a);
            const Vector3 centroid = (a + b + c) / 3.0f;
            const bool outward = Vector3::Dot(normal, centroid - inside) >= 0.0f;
            if (!outward) normal = -normal;
            Color lit = color;
            if (sheen > 0.0f && normal.LengthSquared() > 1.0e-12f) {
                // Wet carapace: a narrow glossy lobe toward the fixed sun, seen from above.
                const Vector3 halfVector = Vector3::Normalize(wildlife_render::FixedSunDirection + Vector3::Up);
                const float facing = std::max(0.0f, Vector3::Dot(Vector3::Normalize(normal), halfVector));
                lit = wildlife_render::ScaleColor(color, 1.0f + sheen * std::pow(facing, 12.0f));
            }
            if (outward) {
                AddLitTriangle(vertices, a, b, c, lit);
            } else {
                AddLitTriangle(vertices, a, c, b, lit);
            }
        }

        [[nodiscard]] Color ToneColor(const Color& color, const float factor)
        {
            return wildlife_render::ScaleColor(color, factor);
        }

        struct NearTurtleBudget
        {
            static constexpr int CarapaceRows = sea_turtle::CarapaceStations - 1;
            static constexpr int CarapaceColumns = static_cast<int>(sea_turtle::CarapaceColumns.size()) - 1;
            static constexpr int PlastronColumns = static_cast<int>(sea_turtle::PlastronColumns.size()) - 1;
            static constexpr int FrontSamples = 7, RearSamples = 5, HeadSegments = 12, HeadRings = 5;
            static constexpr int SeamRows = 4;                                // boundaries between the five scute rows
            static constexpr int SeamColumns = 4;                             // |s| = 0.22 and 0.78 on both sides
            static constexpr int Triangles =
                CarapaceRows * CarapaceColumns * 2 + CarapaceColumns * 2      // dome and end caps
                + SeamRows * CarapaceColumns * 2 + SeamColumns * CarapaceRows * 2 // scute seam strips
                + CarapaceRows * PlastronColumns * 2 + PlastronColumns * 2    // plastron and caps
                + HeadSegments * (HeadRings - 1) * 2 + HeadSegments * 2       // neck/skull/crown/brow/snout, nose, neck cap
                + 6                                                           // paired two-facet eyes and highlights
                + 4                                                           // paired nostrils and mouth creases
                + 2 * ((FrontSamples - 1) * 2 * 4)                            // front paddles, faces and edges
                + 2 * ((RearSamples - 1) * 2 * 4)                             // rear paddles, faces and edges
                + 2 * 4 * 2                                                   // closed roots and tips
                + 2 * ((FrontSamples - 2) * 2 + (RearSamples - 2) * 2)        // upper paddle scale seams
                + 4;                                                          // tail, two faces
        };
        static_assert(NearTurtleBudget::Triangles * 3 == SeaTurtleRenderer::NearVertexCount);

        void AppendNearTurtle(std::vector<VertexPositionColor>& vertices,
                              const SeaTurtleState& turtle, const float timeSeconds)
        {
            using namespace sea_turtle;
            const Vector3 centre(turtle.x, turtle.y, turtle.z);
            const PoseAxes axes = MakePoseAxes(turtle);
            const TurtlePalette palette = LitPalette(turtle, timeSeconds);
            const float scale = turtle.scale;
            const auto local = [&](const float x, const float y, const float z) {
                return LocalPoint(centre, axes, x, y, z, scale);
            };
            const Vector3 bodyInside = local(0.0f, 0.03f, -0.03f);

            // Carapace: a scute-aligned grid on the domed teardrop, closed by end fans so no
            // facet is degenerate. Vertebral scutes are lighter, costals radiate dark-to-light
            // from the outer edge, marginals form the pale rim; alternate rows tone the seams.
            constexpr int Rows = NearTurtleBudget::CarapaceRows;
            constexpr int Columns = NearTurtleBudget::CarapaceColumns;
            std::array<float, CarapaceStations> stations{};
            for (int i = 0; i < CarapaceStations; ++i) {
                stations[static_cast<std::size_t>(i)] = -0.96f + 1.92f * static_cast<float>(i)
                    / static_cast<float>(CarapaceStations - 1);
            }
            std::array<std::array<Vector3, CarapaceColumns.size()>, CarapaceStations> dome{};
            for (int i = 0; i < CarapaceStations; ++i) {
                for (std::size_t c = 0; c < CarapaceColumns.size(); ++c) {
                    const float sPos = CarapaceColumns[c];
                    float u = stations[static_cast<std::size_t>(i)];
                    if (i > 0 && i + 1 < CarapaceStations) {
                        if (i % 2 == 0) {
                            u = ScuteRowBoundaryU(i / 2, sPos);
                        } else {
                            const int lowerBoundary = i / 2;
                            const float lower = lowerBoundary == 0 ? stations.front()
                                : ScuteRowBoundaryU(lowerBoundary, sPos);
                            const float upper = lowerBoundary == 4 ? stations.back()
                                : ScuteRowBoundaryU(lowerBoundary + 1, sPos);
                            u = 0.5f * (lower + upper);
                        }
                    }
                    const float halfWidth = HalfWidth(u);
                    const float z = LengthToZ(u);
                    dome[static_cast<std::size_t>(i)][c] = local(sPos * halfWidth, CarapaceHeight(u, sPos), z);
                }
            }
            const auto scuteColor = [&](const float u, const float sMid, const int row, const int column) {
                const Scute scute = ScuteAt(sMid);
                const float rowTone = (ScuteRow(u) % 2 == 0) ? 1.0f : 0.93f;
                if (scute == Scute::Vertebral) return ToneColor(palette.scute, 1.04f * rowTone);
                if (scute == Scute::Marginal) return ToneColor(palette.underside, 0.84f * rowTone);
                const bool inner = column == 2 || column == 5;
                return ToneColor(palette.shell, (inner ? 1.14f : 0.90f) * rowTone * (row % 2 ? 1.0f : 0.97f));
            };
            for (int row = 0; row < Rows; ++row) {
                const float u = 0.5f * (stations[static_cast<std::size_t>(row)] + stations[static_cast<std::size_t>(row + 1)]);
                for (int column = 0; column < Columns; ++column) {
                    const auto c0 = static_cast<std::size_t>(column), c1 = c0 + 1U;
                    const auto r0 = static_cast<std::size_t>(row), r1 = r0 + 1U;
                    const float sMid = 0.5f * (CarapaceColumns[c0] + CarapaceColumns[c1]);
                    const Color color = scuteColor(u, sMid, row, column);
                    AddOutwardTriangle(vertices, dome[r0][c0], dome[r1][c0], dome[r1][c1], color, bodyInside, 0.35f);
                    AddOutwardTriangle(vertices, dome[r0][c0], dome[r1][c1], dome[r0][c1], color, bodyInside, 0.35f);
                }
            }
            const Vector3 rearPoint = local(0.0f, CarapaceRimY + 0.012f, LengthToZ(-1.0f) - 0.01f);
            const Vector3 frontPoint = local(0.0f, CarapaceRimY + 0.02f, LengthToZ(1.0f) + 0.01f);
            for (int column = 0; column < Columns; ++column) {
                const auto c0 = static_cast<std::size_t>(column), c1 = c0 + 1U;
                AddOutwardTriangle(vertices, rearPoint, dome[0][c0], dome[0][c1],
                                   ToneColor(palette.underside, 0.80f), bodyInside);
                AddOutwardTriangle(vertices, frontPoint, dome[CarapaceStations - 1][c0],
                                   dome[CarapaceStations - 1][c1], ToneColor(palette.underside, 0.84f), bodyInside);
            }

            // Scute seams: narrow dark strips lifted 3 mm off the dome. Alternating
            // transverse points and gently waving longitudinal edges avoid the old cage-like
            // rectangular grid while retaining a symmetric, deterministic shell pattern.
            {
                const Color seam = ToneColor(palette.shell, 0.82f);
                const auto surface = [&](const float u, const float sPos) {
                    return local(sPos * HalfWidth(u), CarapaceHeight(u, sPos), LengthToZ(u))
                        + axes.up * (0.003f * scale);
                };
                constexpr float seamHalfU = 0.0055f, seamHalfS = 0.0075f;
                for (int boundary = 1; boundary <= NearTurtleBudget::SeamRows; ++boundary) {
                    for (int column = 0; column < Columns; ++column) {
                        const auto c0 = static_cast<std::size_t>(column), c1 = c0 + 1U;
                        const float s0 = CarapaceColumns[c0], s1 = CarapaceColumns[c1];
                        const float u0 = ScuteRowBoundaryU(boundary, s0);
                        const float u1 = ScuteRowBoundaryU(boundary, s1);
                        AddOutwardTriangle(vertices, surface(u0 - seamHalfU, s0),
                                           surface(u0 + seamHalfU, s0), surface(u1 + seamHalfU, s1),
                                           seam, bodyInside);
                        AddOutwardTriangle(vertices, surface(u0 - seamHalfU, s0),
                                           surface(u1 + seamHalfU, s1), surface(u1 - seamHalfU, s1),
                                           seam, bodyInside);
                    }
                }
                for (const std::size_t c : {std::size_t{1}, std::size_t{3}, std::size_t{5}, std::size_t{7}}) {
                    for (int row = 0; row < Rows; ++row) {
                        const auto r0 = static_cast<std::size_t>(row), r1 = r0 + 1U;
                        const float u0 = stations[r0], u1 = stations[r1];
                        const float s0 = ScuteColumnBoundaryS(static_cast<int>(c), u0);
                        const float s1 = ScuteColumnBoundaryS(static_cast<int>(c), u1);
                        AddOutwardTriangle(vertices, surface(u0, s0 - seamHalfS),
                                           surface(u1, s1 - seamHalfS), surface(u1, s1 + seamHalfS),
                                           seam, bodyInside);
                        AddOutwardTriangle(vertices, surface(u0, s0 - seamHalfS),
                                           surface(u1, s1 + seamHalfS), surface(u0, s0 + seamHalfS),
                                           seam, bodyInside);
                    }
                }
            }

            // Plastron shares the rim so the shell is closed; pale with faint central seams.
            constexpr int PlastronCols = NearTurtleBudget::PlastronColumns;
            std::array<std::array<Vector3, PlastronColumns.size()>, CarapaceStations> plastron{};
            for (int i = 0; i < CarapaceStations; ++i) {
                const float u = stations[static_cast<std::size_t>(i)];
                const float halfWidth = HalfWidth(u);
                const float z = LengthToZ(u);
                for (std::size_t c = 0; c < PlastronColumns.size(); ++c) {
                    const float sPos = PlastronColumns[c];
                    const bool rim = c == 0 || c + 1 == PlastronColumns.size();
                    plastron[static_cast<std::size_t>(i)][c] = rim
                        ? dome[static_cast<std::size_t>(i)][sPos < 0.0f ? 0 : CarapaceColumns.size() - 1]
                        : local(sPos * halfWidth, PlastronY(u, sPos), z);
                }
            }
            for (int row = 0; row < Rows; ++row) {
                for (int column = 0; column < PlastronCols; ++column) {
                    const auto c0 = static_cast<std::size_t>(column), c1 = c0 + 1U;
                    const auto r0 = static_cast<std::size_t>(row), r1 = r0 + 1U;
                    const Color color = ToneColor(palette.underside, column == 1 || column == 2 ? 1.0f : 0.94f);
                    AddOutwardTriangle(vertices, plastron[r0][c0], plastron[r1][c1], plastron[r1][c0], color, bodyInside);
                    AddOutwardTriangle(vertices, plastron[r0][c0], plastron[r0][c1], plastron[r1][c1], color, bodyInside);
                }
            }
            for (int column = 0; column < PlastronCols; ++column) {
                const auto c0 = static_cast<std::size_t>(column), c1 = c0 + 1U;
                AddOutwardTriangle(vertices, rearPoint, plastron[0][c1], plastron[0][c0],
                                   ToneColor(palette.underside, 0.86f), bodyInside);
                AddOutwardTriangle(vertices, frontPoint, plastron[CarapaceStations - 1][c1],
                                   plastron[CarapaceStations - 1][c0], ToneColor(palette.underside, 0.9f), bodyInside);
            }

            // Neck, head and beaked snout: five rounded rings, split beak points and a neck cap.
            constexpr int HeadSegments = NearTurtleBudget::HeadSegments;
            struct Ring { float y, z, radiusX, radiusY; };
            // Narrow neck, rear skull, full crown, brow and a narrowing beaked snout.
            constexpr std::array<Ring, NearTurtleBudget::HeadRings> rings{{
                {0.020f, 0.48f, 0.073f, 0.064f}, {0.043f, 0.61f, 0.108f, 0.091f},
                {0.050f, 0.70f, 0.116f, 0.098f}, {0.035f, 0.79f, 0.094f, 0.077f},
                {0.004f, 0.88f, 0.064f, 0.048f}}};
            std::array<std::array<Vector3, HeadSegments>, NearTurtleBudget::HeadRings> head{};
            for (std::size_t r = 0; r < rings.size(); ++r) {
                for (int segment = 0; segment < HeadSegments; ++segment) {
                    const float angle = (static_cast<float>(segment) + 0.5f) * 2.0f
                        * std::numbers::pi_v<float> / static_cast<float>(HeadSegments);
                    head[r][static_cast<std::size_t>(segment)] = local(
                        std::cos(angle) * rings[r].radiusX, rings[r].y + std::sin(angle) * rings[r].radiusY,
                        rings[r].z);
                }
            }
            const auto headInside = [&](const std::size_t r) { return local(0.0f, rings[r].y, rings[r].z); };
            for (std::size_t r = 0; r + 1 < rings.size(); ++r) {
                for (int segment = 0; segment < HeadSegments; ++segment) {
                    const auto s0 = static_cast<std::size_t>(segment);
                    const auto s1 = static_cast<std::size_t>((segment + 1) % HeadSegments);
                    // Lower brow/snout facets form the pale horny beak; the skull keeps scaled skin.
                    const bool lower = segment >= HeadSegments / 2;
                    const Color color = r >= 2 && lower ? ToneColor(palette.underside, r == 3 ? 0.94f : 0.98f)
                        : ToneColor(palette.skin, (lower ? 0.92f : 1.0f) * (segment % 2 ? 0.97f : 1.0f));
                    const Vector3 inside = (headInside(r) + headInside(r + 1)) * 0.5f;
                    AddOutwardTriangle(vertices, head[r][s0], head[r + 1][s0], head[r + 1][s1], color, inside);
                    AddOutwardTriangle(vertices, head[r][s0], head[r + 1][s1], head[r][s1], color, inside);
                }
            }
            const Vector3 noseUpper = local(0.0f, 0.024f, 0.942f);
            const Vector3 noseLower = local(0.0f, -0.024f, 0.925f);
            const Vector3 neckBase = local(0.0f, 0.02f, 0.42f);
            for (int segment = 0; segment < HeadSegments; ++segment) {
                const auto s0 = static_cast<std::size_t>(segment);
                const auto s1 = static_cast<std::size_t>((segment + 1) % HeadSegments);
                const float midAngle = (static_cast<float>(segment) + 1.0f) * 2.0f
                    * std::numbers::pi_v<float> / static_cast<float>(HeadSegments);
                const Vector3& nose = std::sin(midAngle) >= 0.0f ? noseUpper : noseLower;
                AddOutwardTriangle(vertices, nose, head[4][s0], head[4][s1],
                                   ToneColor(palette.underside, 0.9f), headInside(4));
                AddOutwardTriangle(vertices, neckBase, head[0][s1], head[0][s0], palette.skin, headInside(0));
            }
            const Color eye(16, 22, 20, 255);
            const Color eyeShine(119, 137, 127, 255);
            for (int sideIndex = -1; sideIndex <= 1; sideIndex += 2) {
                const float side = static_cast<float>(sideIndex);
                const Vector3 eTop = local(side * 0.118f, 0.094f, 0.704f);
                const Vector3 eFront = local(side * 0.120f, 0.068f, 0.733f);
                const Vector3 eBottom = local(side * 0.120f, 0.043f, 0.700f);
                const Vector3 eRear = local(side * 0.118f, 0.066f, 0.671f);
                if (side > 0.0f) {
                    AddTriangle(vertices, eTop, eBottom, eFront, eye);
                    AddTriangle(vertices, eTop, eRear, eBottom, eye);
                } else {
                    AddTriangle(vertices, eTop, eFront, eBottom, eye);
                    AddTriangle(vertices, eTop, eBottom, eRear, eye);
                }
                const Vector3 h0 = local(side * 0.122f, 0.083f, 0.704f);
                const Vector3 h1 = local(side * 0.123f, 0.069f, 0.717f);
                const Vector3 h2 = local(side * 0.123f, 0.068f, 0.692f);
                AddTriangle(vertices, h0, side > 0.0f ? h2 : h1, side > 0.0f ? h1 : h2, eyeShine);
                const Vector3 n0 = local(side * 0.043f, 0.025f, 0.908f);
                const Vector3 n1 = local(side * 0.058f, 0.010f, 0.890f);
                const Vector3 n2 = local(side * 0.035f, 0.008f, 0.914f);
                AddTriangle(vertices, n0, side > 0.0f ? n2 : n1,
                            side > 0.0f ? n1 : n2, eye);
                const Vector3 m0 = local(side * 0.064f, -0.018f, 0.885f);
                const Vector3 m1 = local(side * 0.082f, -0.022f, 0.820f);
                const Vector3 m2 = local(side * 0.060f, -0.030f, 0.873f);
                AddTriangle(vertices, m0, side > 0.0f ? m2 : m1,
                            side > 0.0f ? m1 : m2, eye);
            }

            // Paddles: closed tapered volumes with a dark scaled upper face, pale underside
            // and shadowed edges. The old coincident top/bottom sheets looked paper-thin at
            // grazing angles and disconnected from the shell.
            const float stroke = turtle.flipperStroke;
            const auto appendPaddle = [&](const float side, const int samples,
                                          const auto& sampler, const float topTone) {
                std::vector<Vector3> upperLeading, upperTrailing;
                std::vector<Vector3> lowerLeading, lowerTrailing;
                for (int i = 0; i < samples; ++i) {
                    const float t = static_cast<float>(i) / static_cast<float>(samples - 1);
                    float halfWidth = 0.0f;
                    const FlipperPoint c = sampler(side, t, halfWidth);
                    const float halfThickness = (0.020f - 0.012f * t) * scale;
                    const Vector3 lift = axes.up * halfThickness;
                    upperLeading.push_back(local(c.x, c.y, c.z + halfWidth) + lift);
                    upperTrailing.push_back(local(c.x, c.y, c.z - halfWidth) + lift);
                    lowerLeading.push_back(local(c.x, c.y, c.z + halfWidth) - lift);
                    lowerTrailing.push_back(local(c.x, c.y, c.z - halfWidth) - lift);
                }
                for (int i = 0; i + 1 < samples; ++i) {
                    const auto a = static_cast<std::size_t>(i), b = a + 1U;
                    const Color top = ToneColor(palette.skin, topTone * (i % 2 ? 0.95f : 1.0f));
                    const Color under = ToneColor(palette.underside, 0.9f);
                    const Color edge = ToneColor(palette.skin, topTone * 0.72f);
                    const Vector3 above = (upperLeading[a] + upperTrailing[b]) * 0.5f + axes.up * 0.2f;
                    const Vector3 below = (lowerLeading[a] + lowerTrailing[b]) * 0.5f - axes.up * 0.2f;
                    AddOutwardTriangle(vertices, upperLeading[a], upperTrailing[a], upperTrailing[b], top, below);
                    AddOutwardTriangle(vertices, upperLeading[a], upperTrailing[b], upperLeading[b], top, below);
                    AddOutwardTriangle(vertices, lowerLeading[a], lowerTrailing[b], lowerTrailing[a], under, above);
                    AddOutwardTriangle(vertices, lowerLeading[a], lowerLeading[b], lowerTrailing[b], under, above);
                    AddQuad(vertices, upperLeading[a], upperLeading[b], lowerLeading[b], lowerLeading[a], edge);
                    AddQuad(vertices, upperTrailing[b], upperTrailing[a], lowerTrailing[a], lowerTrailing[b], edge);
                }
                const Color edge = ToneColor(palette.skin, topTone * 0.72f);
                AddQuad(vertices, upperTrailing.front(), upperLeading.front(),
                        lowerLeading.front(), lowerTrailing.front(), edge);
                AddQuad(vertices, upperLeading.back(), upperTrailing.back(),
                        lowerTrailing.back(), lowerLeading.back(), edge);
                const Color joint = ToneColor(palette.skin, topTone * 0.74f);
                for (int i = 1; i + 1 < samples; ++i) {
                    const auto before = static_cast<std::size_t>(i - 1);
                    const auto at = static_cast<std::size_t>(i);
                    const auto after = static_cast<std::size_t>(i + 1);
                    const Vector3 offset = axes.up * (0.0025f * scale);
                    const Vector3 l0 = Vector3::Lerp(upperLeading[before], upperLeading[at], 0.985f) + offset;
                    const Vector3 t0 = Vector3::Lerp(upperTrailing[before], upperTrailing[at], 0.985f) + offset;
                    const Vector3 l1 = Vector3::Lerp(upperLeading[at], upperLeading[after], 0.015f) + offset;
                    const Vector3 t1 = Vector3::Lerp(upperTrailing[at], upperTrailing[after], 0.015f) + offset;
                    const Vector3 below = (l0 + t0 + l1 + t1) * 0.25f - axes.up * 0.2f;
                    AddOutwardTriangle(vertices, l0, t0, t1, joint, below);
                    AddOutwardTriangle(vertices, l0, t1, l1, joint, below);
                }
            };
            for (int sideIndex = -1; sideIndex <= 1; sideIndex += 2) {
                const float side = static_cast<float>(sideIndex);
                appendPaddle(side, NearTurtleBudget::FrontSamples,
                    [stroke](const float s, const float t, float& w) { return FrontFlipper(s, t, stroke, w); }, 0.9f);
                appendPaddle(side, NearTurtleBudget::RearSamples,
                    [stroke](const float s, const float t, float& w) { return RearFlipper(s, t, -stroke * 0.6f, w); }, 0.88f);
            }

            const Vector3 tailRoot = local(0.0f, -0.035f, -0.54f);
            const Vector3 tailLeft = local(-0.035f, -0.045f, -0.62f);
            const Vector3 tailRight = local(0.035f, -0.045f, -0.62f);
            const Vector3 tailTip = local(0.0f, -0.05f, -0.70f);
            AddDoubleSidedTriangle(vertices, tailRoot, tailLeft, tailTip, ToneColor(palette.skin, 0.92f));
            AddDoubleSidedTriangle(vertices, tailRoot, tailTip, tailRight, ToneColor(palette.skin, 0.92f));
        }

        void AppendFarTurtle(std::vector<VertexPositionColor>& vertices,
                             const SeaTurtleState& turtle, const float timeSeconds)
        {
            // Low domed teardrop plus two paddles: 12 facets that keep the near silhouette.
            const Vector3 centre(turtle.x, turtle.y, turtle.z);
            const PoseAxes axes = MakePoseAxes(turtle);
            const TurtlePalette palette = LitPalette(turtle, timeSeconds);
            const float scale = turtle.scale;
            const auto local = [&](const float x, const float y, const float z) {
                return LocalPoint(centre, axes, x, y, z, scale);
            };
            const Vector3 inside = local(0.0f, 0.0f, -0.03f);
            const Vector3 top = local(0.0f, 0.20f, 0.0f);
            const Vector3 bottom = local(0.0f, -0.07f, -0.03f);
            const Vector3 front = local(0.0f, 0.0f, 0.62f);
            const Vector3 rear = local(0.0f, 0.0f, -0.56f);
            const Vector3 left = local(-0.42f, 0.0f, 0.02f);
            const Vector3 right = local(0.42f, 0.0f, 0.02f);
            const Color dome = palette.shell, ridge = palette.scute;
            AddOutwardTriangle(vertices, top, front, left, ridge, inside);
            AddOutwardTriangle(vertices, top, left, rear, dome, inside);
            AddOutwardTriangle(vertices, top, rear, right, dome, inside);
            AddOutwardTriangle(vertices, top, right, front, ridge, inside);
            AddOutwardTriangle(vertices, bottom, left, front, palette.underside, inside);
            AddOutwardTriangle(vertices, bottom, rear, left, palette.underside, inside);
            AddOutwardTriangle(vertices, bottom, right, rear, palette.underside, inside);
            AddOutwardTriangle(vertices, bottom, front, right, palette.underside, inside);
            const float stroke = turtle.flipperStroke;
            for (int sideIndex = -1; sideIndex <= 1; sideIndex += 2) {
                const float side = static_cast<float>(sideIndex);
                const Vector3 root = local(side * 0.36f, -0.01f, 0.30f);
                const Vector3 tip = local(side * 0.98f, -0.01f + stroke * 0.30f, 0.10f - stroke * 0.12f);
                const Vector3 back = local(side * 0.40f, -0.02f, 0.06f);
                AddDoubleSidedTriangle(vertices, root, tip, back, ToneColor(palette.skin, 0.92f));
            }
        }
    }

    SeaTurtleRenderer::SeaTurtleRenderer(GraphicsDevice& device, const float maximumDistance)
        : device_(device),
          effect_(std::make_unique<BasicEffect>(device)),
          vertices_(std::make_unique<DynamicVertexBuffer>(
              device, VertexPositionColor::getVertexDeclarationStatic(), MaximumVertexCount,
              BufferUsage::WriteOnly)),
          maximumDistance_(maximumDistance)
    {
        if (!std::isfinite(maximumDistance_) || maximumDistance_ <= 0.0f) {
            throw std::invalid_argument(
                "Sea-turtle maximum draw distance must be finite and positive");
        }
        effect_->setVertexColorEnabledProperty(true);
        effect_->setLightingEnabledProperty(false);
        effect_->setFogEnabledProperty(true);
        effect_->setFogColorProperty(Vector3(0.16f, 0.55f, 0.61f));
        effect_->setFogStartProperty(76.0f);
        effect_->setFogEndProperty(205.0f);
        stagingVertices_.reserve(static_cast<std::size_t>(MaximumVertexCount));
        std::cout << TurtleDiagnostic << '\n';
        MirrorRuntimeDiagnosticToPlatformLog(TurtleDiagnostic);
        std::cout << TurtleFormDiagnostic << '\n';
        MirrorRuntimeDiagnosticToPlatformLog(TurtleFormDiagnostic);
        std::cout << TurtleDetailDiagnostic << '\n';
        MirrorRuntimeDiagnosticToPlatformLog(TurtleDetailDiagnostic);
        std::cout << TurtleScaleDiagnostic << '\n';
        MirrorRuntimeDiagnosticToPlatformLog(TurtleScaleDiagnostic);
    }

    SeaTurtleRenderer::~SeaTurtleRenderer() = default;

    void SeaTurtleRenderer::Draw(const Matrix& view, const Matrix& projection,
                                 const Vector3& cameraPosition,
                                 const std::span<const SeaTurtleState> turtles,
                                 const float timeSeconds)
    {
        stagingVertices_.clear();
        visibleTurtleCount_ = 0;
        visibleNearTurtleCount_ = 0;
        visibleFarTurtleCount_ = 0;
        drawCallCount_ = 0;
        const BoundingFrustum frustum(view * projection);
        for (const SeaTurtleState& turtle : turtles) {
            const Vector3 centre(turtle.x, turtle.y, turtle.z);
            const float distanceSquared = Vector3::DistanceSquared(centre, cameraPosition);
            if (distanceSquared > maximumDistance_ * maximumDistance_
                || !frustum.Intersects(BoundingSphere(centre, turtle.scale * 1.22f))) {
                continue;
            }
            if (distanceSquared <= NearLodDistance * NearLodDistance) {
                AppendNearTurtle(stagingVertices_, turtle, timeSeconds);
                ++visibleNearTurtleCount_;
            } else {
                AppendFarTurtle(stagingVertices_, turtle, timeSeconds);
                ++visibleFarTurtleCount_;
            }
            ++visibleTurtleCount_;
        }
        if (stagingVertices_.empty()) return;

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
