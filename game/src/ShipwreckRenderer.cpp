#include "IslandSimulator/ShipwreckRenderer.hpp"

#include "IslandSimulator/EnvironmentLighting.hpp"
#include "IslandSimulator/RuntimeDiagnostics.hpp"
#include "IslandSimulator/ShipwreckLayout.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <exception>
#include <iostream>
#include <limits>
#include <numbers>
#include <string>
#include <string_view>
#include <vector>

#include "CNA/Graphics/ShaderCodeEXT.hpp"
#include "CNA/Graphics/ShaderPackageEXT.hpp"
#include "CNA/GraphicsCapability.hpp"
#include "CNA/ShaderLanguageEXT.hpp"
#include "Microsoft/Xna/Framework/BoundingFrustum.hpp"
#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/ContainmentType.hpp"
#include "Microsoft/Xna/Framework/Graphics/BasicEffect.hpp"
#include "Microsoft/Xna/Framework/Graphics/BlendState.hpp"
#include "Microsoft/Xna/Framework/Graphics/BufferUsage.hpp"
#include "Microsoft/Xna/Framework/Graphics/DepthStencilState.hpp"
#include "Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp"
#include "Microsoft/Xna/Framework/Graphics/PrimitiveType.hpp"
#include "Microsoft/Xna/Framework/Graphics/RasterizerState.hpp"
#include "Microsoft/Xna/Framework/Graphics/ShaderEffect.hpp"
#include "Microsoft/Xna/Framework/Graphics/VertexBuffer.hpp"
#include "Microsoft/Xna/Framework/Graphics/VertexPositionColor.hpp"

using namespace Microsoft::Xna::Framework;
using namespace Microsoft::Xna::Framework::Graphics;

namespace island
{
    namespace
    {
        constexpr float MaximumDrawDistance = 310.0f;
        constexpr int BeamSegments = 8;
        constexpr std::string_view AgedWoodDiagnostic =
            "island-simulator: shipwreck portable aged wood, waterline, and salt-wear material";
        constexpr std::string_view LargeShipDetailDiagnostic =
            "island-simulator: shipwreck large-ship silhouette uses deck rails, hull openings, "
            "keel, rudder, and a broken foremast";
        constexpr int AgedWoodMaterial = 48;
        constexpr int InnerWoodMaterial = 80;
        constexpr int FreshBreakMaterial = 176;
        const Color OuterWood(112, 79, 45, AgedWoodMaterial);
        const Color DarkWood(77, 52, 33, AgedWoodMaterial);
        const Color InnerWood(137, 103, 66, InnerWoodMaterial);
        const Color FreshBreak(154, 121, 78, FreshBreakMaterial);
        const Color WetWood(63, 59, 47, AgedWoodMaterial);
        const Color DeepWood(43, 31, 22, AgedWoodMaterial);

        constexpr const char* VertexBody = R"(
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec4 aColor;

uniform mat4 uViewProjection;

out vec3 vColor;
out vec3 vWorld;
out float vMaterial;

void main()
{
    vColor = aColor.rgb;
    vWorld = aPosition;
    vMaterial = aColor.a;
    gl_Position = uViewProjection * vec4(aPosition, 1.0);
    gl_Position.z = 2.0 * gl_Position.z - gl_Position.w;
}
)";

        constexpr const char* FragmentBody = R"(
in vec3 vColor;
in vec3 vWorld;
in float vMaterial;

uniform vec3 uCameraPosition;
uniform vec3 uShipCentre;
uniform vec3 uShipRight;
uniform vec3 uShipForward;
uniform vec3 uToSun;

out vec4 fragColor;

void main()
{
    float distanceToCamera = length(vWorld - uCameraPosition);
    float detailFade = 1.0 - smoothstep(72.0, 132.0, distanceToCamera);
    vec3 relative = vWorld - uShipCentre;
    float along = dot(relative, uShipForward);
    float across = dot(relative, uShipRight);

    float longGrain = 0.5 + 0.5 * sin(along * 7.4
        + sin(along * 1.31 + across * 0.73) * 0.82
        + sin(vWorld.y * 5.7) * 0.28);
    float fineGrain = 0.5 + 0.5 * sin(along * 18.3
        + across * 2.1 + sin(vWorld.y * 11.0) * 0.32);
    float broadWeather = sin(along * 0.71 + across * 1.37)
        * sin(vWorld.y * 2.41 - across * 0.62);
    float darkWeather = smoothstep(0.47, 0.91, broadWeather);
    float split = 1.0 - smoothstep(0.028, 0.15,
        abs(sin(along * 2.82 + sin(across * 1.71 + vWorld.y) * 0.44)));

    float freshBreak = smoothstep(0.52, 0.72, vMaterial);
    vec3 aged = vColor * (0.925 + 0.105 * longGrain + 0.035 * fineGrain);
    aged *= 1.0 - darkWeather * 0.14 - split * 0.055;
    aged = mix(aged, aged * vec3(0.78, 0.85, 0.82), darkWeather * 0.22);
    vec3 broken = vColor * (0.95 + 0.075 * fineGrain);
    broken = mix(broken, broken * vec3(1.07, 1.02, 0.90), 0.35 * longGrain);
    vec3 materialColor = mix(aged, broken, freshBreak);
    materialColor = mix(vColor, materialColor, detailFade);

    float wetBand = (1.0 - smoothstep(-0.14, 0.38, vWorld.y))
        * (1.0 - freshBreak * 0.25);
    float submerged = 1.0 - smoothstep(-0.52, 0.05, vWorld.y);
    vec3 wetColor = materialColor * vec3(0.59, 0.68, 0.67);
    wetColor = mix(wetColor, vec3(dot(wetColor, vec3(0.30, 0.59, 0.11)))
        * vec3(0.77, 0.91, 0.79), submerged * 0.24);
    materialColor = mix(materialColor, wetColor, wetBand * (0.74 + 0.18 * detailFade));

    float saltLine = exp(-abs(vWorld.y - 0.18) * 13.0) * detailFade
        * (1.0 - freshBreak) * (0.68 + 0.32 * fineGrain);
    materialColor = mix(materialColor,
                        materialColor * vec3(1.13, 1.12, 1.05),
                        saltLine * 0.24);

    vec3 faceVector = cross(dFdx(vWorld), dFdy(vWorld));
    vec3 faceNormal = normalize(faceVector);
    vec3 toViewer = normalize(uCameraPosition - vWorld);
    if (dot(faceNormal, toViewer) < 0.0) faceNormal = -faceNormal;
    vec3 reflectedSun = reflect(-normalize(uToSun), faceNormal);
    float wetSheen = pow(max(dot(reflectedSun, toViewer), 0.0), 42.0)
        * wetBand * (1.0 - submerged) * 0.12;
    materialColor += vec3(1.00, 0.91, 0.70) * wetSheen;

    float fog = smoothstep(190.0, 520.0, distanceToCamera);
    vec3 color = mix(materialColor, vec3(0.53, 0.78, 0.88), fog);
    fragColor = vec4(color, 1.0);
}
)";

        [[nodiscard]] std::string ShaderSource(const bool embedded, const char* body)
        {
            if (embedded) {
                return std::string("#version 300 es\nprecision highp float;\n") + body;
            }
            return std::string("#version 330 core\n") + body;
        }

        [[nodiscard]] CNA::Graphics::ShaderPackageEXT CreateShaderPackage()
        {
            using CNA::Graphics::ShaderCodeEXT;
            return CNA::Graphics::ShaderPackageEXT(
                {
                    ShaderCodeEXT(CNA::ShaderLanguageEXT::GlslEs,
                                  CNA::ShaderStageEXT::Vertex, "main",
                                  "shipwreck/shipwreck.es.vert.glsl",
                                  ShaderSource(true, VertexBody)),
                    ShaderCodeEXT(CNA::ShaderLanguageEXT::GlslEs,
                                  CNA::ShaderStageEXT::Fragment, "main",
                                  "shipwreck/shipwreck.es.frag.glsl",
                                  ShaderSource(true, FragmentBody)),
                    ShaderCodeEXT(CNA::ShaderLanguageEXT::GlslDesktop,
                                  CNA::ShaderStageEXT::Vertex, "main",
                                  "shipwreck/shipwreck.desktop.vert.glsl",
                                  ShaderSource(false, VertexBody)),
                    ShaderCodeEXT(CNA::ShaderLanguageEXT::GlslDesktop,
                                  CNA::ShaderStageEXT::Fragment, "main",
                                  "shipwreck/shipwreck.desktop.frag.glsl",
                                  ShaderSource(false, FragmentBody)),
                },
                {CNA::ShaderStageEXT::Vertex, CNA::ShaderStageEXT::Fragment});
        }

        struct Axes
        {
            Vector3 right;
            Vector3 up;
            Vector3 forward;
        };

        [[nodiscard]] Axes ShipAxes(const ShipwreckPlacement& ship)
        {
            const float yawCosine = std::cos(ship.yawRadians);
            const float yawSine = std::sin(ship.yawRadians);
            const Vector3 levelRight(yawCosine, 0.0f, -yawSine);
            const Vector3 levelForward(yawSine, 0.0f, yawCosine);
            const float heelCosine = std::cos(ship.heelRadians);
            const float heelSine = std::sin(ship.heelRadians);
            const Vector3 rolledRight = levelRight * heelCosine + Vector3::Up * heelSine;
            const Vector3 rolledUp = Vector3::Up * heelCosine - levelRight * heelSine;
            const float pitchCosine = std::cos(ship.pitchRadians);
            const float pitchSine = std::sin(ship.pitchRadians);
            return Axes{
                rolledRight,
                rolledUp * pitchCosine - levelForward * pitchSine,
                levelForward * pitchCosine + rolledUp * pitchSine,
            };
        }

        [[nodiscard]] Vector3 ShipPoint(const ShipwreckPlacement& ship, const Axes& axes,
                                        const float x, const float y, const float z)
        {
            return Vector3(ship.x, ship.y, ship.z) + axes.right * x + axes.up * y
                + axes.forward * z;
        }

        [[nodiscard]] Color ScaleColor(const Color& color, const float scale)
        {
            const auto channel = [scale](const std::uint8_t value) {
                return static_cast<int>(std::lround(std::clamp(
                    static_cast<float>(value) * scale, 0.0f, 255.0f)));
            };
            return Color(channel(color.getRProperty()), channel(color.getGProperty()),
                         channel(color.getBProperty()),
                         static_cast<int>(color.getAProperty()));
        }

        [[nodiscard]] Color LitColor(const Vector3& a, const Vector3& b, const Vector3& c,
                                     const Color& source)
        {
            const Vector3 normal = Vector3::Normalize(Vector3::Cross(b - a, c - a));
            const Vector3 sun = Vector3::Normalize(Vector3(
                environment::SunDirectionX, environment::SunDirectionY,
                environment::SunDirectionZ));
            const float direct = std::max(0.0f, Vector3::Dot(normal, sun));
            const float sky = std::max(0.0f, normal.Y);
            Color base = source;
            const float averageHeight = (a.Y + b.Y + c.Y) / 3.0f;
            if (averageHeight < 0.12f) {
                const float wet = std::clamp((0.12f - averageHeight) / 1.1f, 0.0f, 1.0f);
                const auto blend = [wet](const std::uint8_t first, const std::uint8_t second) {
                    return static_cast<int>(std::lround(
                        static_cast<float>(first)
                        + (static_cast<float>(second) - static_cast<float>(first)) * wet));
                };
                base = Color(blend(base.getRProperty(), WetWood.getRProperty()),
                             blend(base.getGProperty(), WetWood.getGProperty()),
                             blend(base.getBProperty(), WetWood.getBProperty()),
                             static_cast<int>(source.getAProperty()));
            }
            return ScaleColor(base, 0.66f + direct * 0.34f + sky * 0.08f);
        }

        void AddTriangle(std::vector<VertexPositionColor>& vertices,
                         const Vector3& a, const Vector3& b, const Vector3& c,
                         const Color& color)
        {
            const Color lit = LitColor(a, b, c, color);
            vertices.emplace_back(a, lit);
            vertices.emplace_back(b, lit);
            vertices.emplace_back(c, lit);
        }

        void AddQuad(std::vector<VertexPositionColor>& vertices,
                     const Vector3& a, const Vector3& b, const Vector3& c,
                     const Vector3& d, const Color& color)
        {
            AddTriangle(vertices, a, b, c, color);
            AddTriangle(vertices, a, c, d, color);
        }

        void AppendBox(std::vector<VertexPositionColor>& vertices, const Vector3& centre,
                       const Axes& axes, const float halfWidth, const float halfHeight,
                       const float halfLength, const Color& color)
        {
            const Vector3 r = axes.right * halfWidth;
            const Vector3 u = axes.up * halfHeight;
            const Vector3 f = axes.forward * halfLength;
            const std::array<Vector3, 8> point = {
                centre - r - u - f, centre + r - u - f,
                centre + r + u - f, centre - r + u - f,
                centre - r - u + f, centre + r - u + f,
                centre + r + u + f, centre - r + u + f,
            };
            AddQuad(vertices, point[0], point[1], point[2], point[3], color);
            AddQuad(vertices, point[5], point[4], point[7], point[6], color);
            AddQuad(vertices, point[4], point[0], point[3], point[7], color);
            AddQuad(vertices, point[1], point[5], point[6], point[2], color);
            AddQuad(vertices, point[3], point[2], point[6], point[7], color);
            AddQuad(vertices, point[4], point[5], point[1], point[0], color);
        }

        void AppendBrokenPlank(std::vector<VertexPositionColor>& vertices,
                               const Vector3& centre, const Axes& axes,
                               const float halfWidth, const float halfHeight,
                               const float halfLength, const Color& color,
                               const std::size_t shapeSeed)
        {
            constexpr std::array<float, 5> Along = {-1.0f, -0.82f, -0.04f, 0.84f, 1.0f};
            const float phase = static_cast<float>(shapeSeed % 7U);
            const std::array<float, 5> widthScale = {
                0.58f + 0.035f * phase,
                0.94f,
                0.91f + 0.012f * phase,
                1.0f,
                0.62f + 0.025f * static_cast<float>((shapeSeed + 3U) % 5U),
            };
            const std::array<float, 5> centreOffset = {
                -0.15f + 0.025f * phase,
                0.035f,
                -0.045f,
                0.02f,
                0.13f - 0.021f * phase,
            };
            std::array<Vector3, 5> leftTop;
            std::array<Vector3, 5> rightTop;
            std::array<Vector3, 5> leftBottom;
            std::array<Vector3, 5> rightBottom;
            for (std::size_t point = 0; point < Along.size(); ++point) {
                const Vector3 sectionCentre = centre
                    + axes.forward * (Along[point] * halfLength)
                    + axes.right * (centreOffset[point] * halfWidth);
                const Vector3 halfSpan = axes.right * (widthScale[point] * halfWidth);
                leftTop[point] = sectionCentre - halfSpan + axes.up * halfHeight;
                rightTop[point] = sectionCentre + halfSpan + axes.up * halfHeight;
                leftBottom[point] = sectionCentre - halfSpan - axes.up * halfHeight;
                rightBottom[point] = sectionCentre + halfSpan - axes.up * halfHeight;
            }
            for (std::size_t section = 0; section + 1U < Along.size(); ++section) {
                AddQuad(vertices, leftTop[section], rightTop[section],
                        rightTop[section + 1U], leftTop[section + 1U], color);
                AddQuad(vertices, rightBottom[section], leftBottom[section],
                        leftBottom[section + 1U], rightBottom[section + 1U], color);
                AddQuad(vertices, leftBottom[section], leftTop[section],
                        leftTop[section + 1U], leftBottom[section + 1U], color);
                AddQuad(vertices, rightTop[section], rightBottom[section],
                        rightBottom[section + 1U], rightTop[section + 1U], color);
            }
            AddQuad(vertices, leftBottom.front(), rightBottom.front(), rightTop.front(),
                    leftTop.front(), FreshBreak);
            AddQuad(vertices, rightBottom.back(), leftBottom.back(), leftTop.back(),
                    rightTop.back(), FreshBreak);
        }

        void AppendTaperedBeam(std::vector<VertexPositionColor>& vertices,
                               const Vector3& start, const Vector3& end,
                               const float startRadius, const float endRadius,
                               const Color& color)
        {
            const Vector3 axis = Vector3::Normalize(end - start);
            const Vector3 reference = std::abs(axis.Y) < 0.82f
                ? Vector3::Up : Vector3(1.0f, 0.0f, 0.0f);
            const Vector3 right = Vector3::Normalize(Vector3::Cross(reference, axis));
            const Vector3 up = Vector3::Normalize(Vector3::Cross(axis, right));
            for (int segment = 0; segment < BeamSegments; ++segment) {
                const float angle = 2.0f * std::numbers::pi_v<float>
                    * static_cast<float>(segment) / static_cast<float>(BeamSegments);
                const float nextAngle = 2.0f * std::numbers::pi_v<float>
                    * static_cast<float>(segment + 1) / static_cast<float>(BeamSegments);
                const Vector3 startPoint = start + right * (std::cos(angle) * startRadius)
                    + up * (std::sin(angle) * startRadius);
                const Vector3 startNext = start + right * (std::cos(nextAngle) * startRadius)
                    + up * (std::sin(nextAngle) * startRadius);
                const Vector3 endPoint = end + right * (std::cos(angle) * endRadius)
                    + up * (std::sin(angle) * endRadius);
                const Vector3 endNext = end + right * (std::cos(nextAngle) * endRadius)
                    + up * (std::sin(nextAngle) * endRadius);
                AddQuad(vertices, startPoint, endPoint, endNext, startNext, color);
                AddTriangle(vertices, start, startNext, startPoint, color);
                AddTriangle(vertices, end, endPoint, endNext, color);
            }
        }

        [[nodiscard]] float HullHalfWidth(const ShipwreckPlacement& ship, const float z)
        {
            const float t = std::clamp(z / ship.length + 0.5f, 0.0f, 1.0f);
            const float curved = std::pow(std::max(0.0f,
                std::sin(t * std::numbers::pi_v<float>)), 0.38f);
            return ship.beam * 0.5f * (curved * 0.78f + (1.0f - t) * 0.35f);
        }

        [[nodiscard]] float HullWidthAtHeight(const ShipwreckPlacement& ship,
                                              const float z, const float y)
        {
            const float fullWidth = HullHalfWidth(ship, z);
            if (y <= -0.12f) {
                return fullWidth * std::clamp((y + 1.48f) / 1.36f, 0.0f, 1.0f);
            }
            return fullWidth * (1.0f - 0.09f
                * std::clamp((y + 0.12f) / 1.45f, 0.0f, 1.0f));
        }

        [[nodiscard]] std::array<Vector3, 11> HullSection(
            const ShipwreckPlacement& ship, const Axes& axes, const float z)
        {
            constexpr std::array<float, 5> SideHeights = {
                1.33f, 0.88f, 0.42f, -0.10f, -0.72f};
            std::array<Vector3, 11> points;
            for (std::size_t level = 0; level < SideHeights.size(); ++level) {
                const float height = SideHeights[level];
                const float width = HullWidthAtHeight(ship, z, height);
                points[level] = ShipPoint(ship, axes, -width, height, z);
                points[points.size() - 1U - level] =
                    ShipPoint(ship, axes, width, height, z);
            }
            points[SideHeights.size()] = ShipPoint(ship, axes, 0.0f, -1.48f, z);
            return points;
        }

        template<std::size_t SectionCount>
        void AppendHullPiece(std::vector<VertexPositionColor>& vertices,
                             const ShipwreckPlacement& ship, const Axes& axes,
                             const std::array<float, SectionCount>& sectionZ,
                             const bool capFirst)
        {
            for (std::size_t section = 0; section + 1U < sectionZ.size(); ++section) {
                const auto first = HullSection(ship, axes, sectionZ[section]);
                const auto next = HullSection(ship, axes, sectionZ[section + 1U]);
                for (std::size_t band = 0; band + 1U < first.size(); ++band) {
                    const bool nearBrokenEnd = capFirst
                        ? section + 2U >= sectionZ.size()
                        : section < 2U;
                    const bool missingBoard = nearBrokenEnd
                        && ((capFirst && band == 2U && section % 2U == 0U)
                            || (!capFirst && band == 7U && section % 2U == 1U));
                    if (missingBoard) continue;
                    const float tone = 0.89f + 0.035f
                        * static_cast<float>((section * 3U + band * 2U) % 6U);
                    const Color color = ScaleColor(OuterWood, tone);
                    AddQuad(vertices, first[band], next[band], next[band + 1U],
                            first[band + 1U], color);
                }

                const float firstWidth = HullHalfWidth(ship, sectionZ[section]) * 0.74f;
                const float nextWidth = HullHalfWidth(ship, sectionZ[section + 1U]) * 0.74f;
                const Vector3 deckFirstLeft = ShipPoint(
                    ship, axes, -firstWidth, 1.16f, sectionZ[section]);
                const Vector3 deckFirstRight = ShipPoint(
                    ship, axes, firstWidth, 1.16f, sectionZ[section]);
                const Vector3 deckNextLeft = ShipPoint(
                    ship, axes, -nextWidth, 1.16f, sectionZ[section + 1U]);
                const Vector3 deckNextRight = ShipPoint(
                    ship, axes, nextWidth, 1.16f, sectionZ[section + 1U]);
                AddQuad(vertices, deckFirstLeft, deckFirstRight, deckNextRight,
                        deckNextLeft, section % 2U == 0U ? InnerWood : OuterWood);

                const float centreZ = (sectionZ[section] + sectionZ[section + 1U]) * 0.5f;
                const float halfLength = (sectionZ[section + 1U] - sectionZ[section]) * 0.5f;
                const float centreWidth = HullHalfWidth(ship, centreZ) * 0.92f;
                AppendBox(vertices, ShipPoint(ship, axes, -centreWidth, 1.38f, centreZ),
                          axes, 0.10f, 0.10f, halfLength, DarkWood);
                AppendBox(vertices, ShipPoint(ship, axes, centreWidth, 1.38f, centreZ),
                          axes, 0.10f, 0.10f, halfLength, DarkWood);

                constexpr std::array<float, 4> SeamHeights = {
                    -0.72f, -0.18f, 0.40f, 0.92f};
                for (const float seamY : SeamHeights) {
                    const float firstSide = HullWidthAtHeight(
                        ship, sectionZ[section], seamY) + 0.025f;
                    const float nextSide = HullWidthAtHeight(
                        ship, sectionZ[section + 1U], seamY) + 0.025f;
                    for (const float side : {-1.0f, 1.0f}) {
                        const Vector3 a = ShipPoint(ship, axes, side * firstSide,
                                                    seamY - 0.030f, sectionZ[section]);
                        const Vector3 b = ShipPoint(ship, axes, side * nextSide,
                                                    seamY - 0.030f,
                                                    sectionZ[section + 1U]);
                        const Vector3 c = ShipPoint(ship, axes, side * nextSide,
                                                    seamY + 0.030f,
                                                    sectionZ[section + 1U]);
                        const Vector3 d = ShipPoint(ship, axes, side * firstSide,
                                                    seamY + 0.030f, sectionZ[section]);
                        AddQuad(vertices, a, b, c, d, DarkWood);
                    }
                }
            }

            const auto first = HullSection(ship, axes, sectionZ.front());
            const auto last = HullSection(ship, axes, sectionZ.back());
            if (capFirst) {
                for (std::size_t band = 0; band + 1U < first.size(); ++band) {
                    AddTriangle(
                        vertices,
                        ShipPoint(ship, axes, 0.0f, 0.38f, sectionZ.front()),
                        first[band + 1U], first[band], DarkWood);
                }
            } else {
                for (std::size_t band = 0; band + 1U < first.size(); ++band) {
                    AppendTaperedBeam(vertices, first[band], first[band + 1U],
                                      0.11f, 0.09f, FreshBreak);
                }
            }
            for (std::size_t band = 0; band + 1U < last.size(); ++band) {
                AppendTaperedBeam(vertices, last[band], last[band + 1U],
                                  0.11f, 0.09f, FreshBreak);
            }
        }

        void AppendRib(std::vector<VertexPositionColor>& vertices,
                       const ShipwreckPlacement& ship, const Axes& axes, const float z,
                       const float damageLean)
        {
            const float width = std::max(0.8f, HullHalfWidth(ship, z));
            const Vector3 keel = ShipPoint(ship, axes, damageLean, -1.35f, z);
            const Vector3 leftBilge = ShipPoint(ship, axes, -width * 0.92f, -0.15f, z);
            const Vector3 rightBilge = ShipPoint(ship, axes, width * 0.92f, -0.15f, z);
            const Vector3 leftTop = ShipPoint(ship, axes, -width * 0.82f, 1.72f, z);
            const Vector3 rightTop = ShipPoint(ship, axes, width * 0.82f, 1.58f, z);
            AppendTaperedBeam(vertices, keel, leftBilge, 0.14f, 0.13f, InnerWood);
            AppendTaperedBeam(vertices, leftBilge, leftTop, 0.13f, 0.10f, InnerWood);
            AppendTaperedBeam(vertices, keel, rightBilge, 0.14f, 0.13f, InnerWood);
            AppendTaperedBeam(vertices, rightBilge, rightTop, 0.13f, 0.10f, InnerWood);
        }

        void AppendShip(std::vector<VertexPositionColor>& vertices,
                        const ShipwreckPlacement& ship)
        {
            const Axes axes = ShipAxes(ship);
            constexpr std::array aftSections = {
                -10.0f, -8.7f, -7.1f, -5.4f, -3.8f, -2.4f, -1.3f, -0.95f};
            constexpr std::array bowSections = {
                1.15f, 2.0f, 3.25f, 4.8f, 6.35f, 7.65f, 8.7f, 9.4f, 10.0f};
            AppendHullPiece(vertices, ship, axes, aftSections, true);
            AppendHullPiece(vertices, ship, axes, bowSections, false);

            AppendRib(vertices, ship, axes, -0.88f, -0.12f);
            AppendRib(vertices, ship, axes, 0.05f, 0.18f);
            AppendRib(vertices, ship, axes, 1.12f, -0.08f);

            AppendTaperedBeam(
                vertices, ShipPoint(ship, axes, 0.0f, -1.42f, -9.55f),
                ShipPoint(ship, axes, 0.0f, -1.45f, -1.05f), 0.14f, 0.11f, DarkWood);
            AppendTaperedBeam(
                vertices, ShipPoint(ship, axes, 0.0f, -1.45f, 1.15f),
                ShipPoint(ship, axes, 0.0f, -1.45f, 9.45f), 0.12f, 0.10f, DarkWood);
            AppendTaperedBeam(
                vertices, ShipPoint(ship, axes, 0.0f, -1.36f, -9.92f),
                ShipPoint(ship, axes, 0.0f, 1.26f, -9.92f), 0.15f, 0.11f, DarkWood);
            AppendBox(vertices, ShipPoint(ship, axes, 0.0f, -0.46f, -10.28f), axes,
                      0.46f, 0.74f, 0.075f, OuterWood);

            AppendTaperedBeam(
                vertices, ShipPoint(ship, axes, 0.0f, 1.18f, -3.25f),
                ShipPoint(ship, axes, 1.65f, 7.15f, -1.75f), 0.34f, 0.20f, DarkWood);
            AppendTaperedBeam(
                vertices, ShipPoint(ship, axes, -2.1f, 5.45f, -2.15f),
                ShipPoint(ship, axes, 3.0f, 5.85f, -1.35f), 0.13f, 0.09f, DarkWood);

            AppendTaperedBeam(
                vertices, ShipPoint(ship, axes, 0.0f, 1.08f, 9.1f),
                ShipPoint(ship, axes, 0.0f, 2.12f, 13.2f), 0.18f, 0.10f, DarkWood);
            AppendTaperedBeam(
                vertices, ShipPoint(ship, axes, 1.65f, 7.15f, -1.75f),
                ShipPoint(ship, axes, 0.0f, 1.92f, 8.8f), 0.045f, 0.035f, DarkWood);
            AppendTaperedBeam(
                vertices, ShipPoint(ship, axes, 1.65f, 7.15f, -1.75f),
                ShipPoint(ship, axes, -1.05f, 1.75f, -8.4f), 0.045f, 0.035f, DarkWood);

            for (const float z : {-7.9f, -5.9f, -3.7f, 3.4f, 5.8f, 7.8f}) {
                const float railX = HullHalfWidth(ship, z) * 0.88f;
                for (const float side : {-1.0f, 1.0f}) {
                    AppendBox(vertices, ShipPoint(ship, axes, side * railX, 1.70f, z),
                              axes, 0.075f, 0.37f, 0.075f, DarkWood);
                }
            }

            constexpr std::array<std::array<float, 2>, 4> RailSections = {{
                {-7.9f, -5.9f}, {-5.9f, -3.7f}, {3.4f, 5.8f}, {5.8f, 7.8f},
            }};
            for (const auto& section : RailSections) {
                for (const float side : {-1.0f, 1.0f}) {
                    const float firstX = HullHalfWidth(ship, section[0]) * 0.88f;
                    const float secondX = HullHalfWidth(ship, section[1]) * 0.88f;
                    AppendTaperedBeam(
                        vertices, ShipPoint(ship, axes, side * firstX, 2.05f, section[0]),
                        ShipPoint(ship, axes, side * secondX, 2.05f, section[1]),
                        0.065f, 0.055f, DarkWood);
                }
            }

            for (const float z : {-4.9f, -2.8f, 3.7f, 6.0f}) {
                constexpr float openingY = 0.53f;
                const float openingX = HullWidthAtHeight(ship, z, openingY) + 0.022f;
                for (const float side : {-1.0f, 1.0f}) {
                    AppendBox(vertices,
                              ShipPoint(ship, axes, side * openingX, openingY, z), axes,
                              0.035f, 0.21f, 0.29f, DeepWood);
                }
            }

            const Vector3 foremastBreak = ShipPoint(ship, axes, -0.34f, 4.08f, 4.78f);
            AppendTaperedBeam(
                vertices, ShipPoint(ship, axes, 0.0f, 1.18f, 4.35f), foremastBreak,
                0.25f, 0.15f, DarkWood);
            AppendTaperedBeam(
                vertices, ShipPoint(ship, axes, -0.28f, 3.98f, 4.73f),
                ShipPoint(ship, axes, -0.58f, 4.48f, 4.92f),
                0.08f, 0.025f, FreshBreak);
            AppendTaperedBeam(
                vertices, ShipPoint(ship, axes, -0.25f, 4.02f, 4.74f),
                ShipPoint(ship, axes, 0.08f, 4.38f, 4.56f),
                0.07f, 0.022f, FreshBreak);

            for (const float side : {-1.0f, 1.0f}) {
                AppendTaperedBeam(
                    vertices, ShipPoint(ship, axes, side * 1.45f, 1.17f, -0.92f),
                    ShipPoint(ship, axes, side * 1.78f, 2.65f, 0.25f),
                    0.13f, 0.08f, FreshBreak);
                AppendTaperedBeam(
                    vertices, ShipPoint(ship, axes, side * 1.50f, 1.08f, 1.12f),
                    ShipPoint(ship, axes, side * 2.25f, 2.05f, 2.30f),
                    0.12f, 0.07f, FreshBreak);
            }

            for (int post = 0; post < 4; ++post) {
                const float x = post < 2 ? -1.35f : 1.35f;
                const float z = post % 2 == 0 ? -8.1f : -5.8f;
                AppendBox(vertices, ShipPoint(ship, axes, x, 2.0f, z), axes,
                          0.11f, 0.80f, 0.11f, DarkWood);
            }
            AppendBox(vertices, ShipPoint(ship, axes, 0.0f, 2.62f, -6.9f), axes,
                      1.55f, 0.09f, 1.35f, OuterWood);
            AppendBox(vertices, ShipPoint(ship, axes, -1.43f, 2.05f, -6.9f), axes,
                      0.08f, 0.56f, 1.22f, OuterWood);
            AppendBox(vertices, ShipPoint(ship, axes, 1.43f, 2.05f, -6.9f), axes,
                      0.08f, 0.56f, 1.22f, OuterWood);
            AppendBox(vertices, ShipPoint(ship, axes, -1.53f, 2.10f, -6.9f), axes,
                      0.025f, 0.23f, 0.34f, DarkWood);
            AppendBox(vertices, ShipPoint(ship, axes, 1.53f, 2.10f, -6.9f), axes,
                      0.025f, 0.23f, 0.34f, DarkWood);
        }

        [[nodiscard]] Axes HorizontalAxes(const float yaw)
        {
            return Axes{Vector3(std::cos(yaw), 0.0f, -std::sin(yaw)), Vector3::Up,
                        Vector3(std::sin(yaw), 0.0f, std::cos(yaw))};
        }

        void AppendDebris(std::vector<VertexPositionColor>& vertices,
                          const ShipwreckDebrisPlacement& debris, const std::size_t index)
        {
            Axes axes = HorizontalAxes(debris.yawRadians);
            const float tilt = index % 3U == 0U ? 0.075f : -0.035f;
            axes.up = Vector3::Normalize(Vector3::Up + axes.right * tilt);
            axes.right = Vector3::Normalize(Vector3::Cross(axes.up, axes.forward));
            const Vector3 centre(debris.x, debris.y + 0.08f, debris.z);
            if (debris.kind == ShipwreckDebrisKind::Plank) {
                AppendBrokenPlank(vertices, centre, axes, debris.width * 0.5f, 0.065f,
                                  debris.length * 0.5f,
                                  index % 2U == 0U ? OuterWood : InnerWood, index);
            } else if (debris.kind == ShipwreckDebrisKind::Spar) {
                AppendTaperedBeam(vertices,
                                  centre - axes.forward * (debris.length * 0.5f),
                                  centre + axes.forward * (debris.length * 0.5f),
                                  debris.width * 0.52f, debris.width * 0.37f, DarkWood);
            } else {
                for (int plank = -1; plank <= 1; ++plank) {
                    const float spread = static_cast<float>(plank) * debris.width * 0.82f;
                    const Vector3 plankCentre = centre + axes.right * spread
                        + axes.up * (0.08f * static_cast<float>(plank + 1));
                    AppendBrokenPlank(
                        vertices, plankCentre, axes, debris.width * 0.34f, 0.075f,
                        debris.length * (0.40f + 0.035f * static_cast<float>(plank)),
                        plank == 0 ? FreshBreak : OuterWood,
                        index * 3U + static_cast<std::size_t>(plank + 1));
                }
            }
        }
    }

    ShipwreckRenderer::ShipwreckRenderer(GraphicsDevice& device,
                                         const ShipwreckLayout& layout)
        : device_(device), fallbackEffect_(std::make_unique<BasicEffect>(device)),
          centre_(layout.Ship().x, layout.Ship().y, layout.Ship().z)
    {
        const Axes shipAxes = ShipAxes(layout.Ship());
        woodRight_ = shipAxes.right;
        woodForward_ = shipAxes.forward;
        std::vector<VertexPositionColor> vertices;
        vertices.reserve(4200U);
        AppendShip(vertices, layout.Ship());
        for (std::size_t index = 0; index < layout.Debris().size(); ++index) {
            AppendDebris(vertices, layout.Debris()[index], index);
        }

        vertexCount_ = static_cast<int>(vertices.size());
        primitiveCount_ = vertexCount_ / 3;
        debrisCount_ = static_cast<int>(layout.Debris().size());
        vertices_ = std::make_unique<VertexBuffer>(
            device_, VertexPositionColor::getVertexDeclarationStatic(), vertexCount_,
            BufferUsage::WriteOnly);
        vertices_->SetData(vertices.data(), vertexCount_);

        Vector3 minimum(std::numeric_limits<float>::max());
        Vector3 maximum(std::numeric_limits<float>::lowest());
        for (const VertexPositionColor& vertex : vertices) {
            minimum = Vector3::Min(minimum, vertex.Position);
            maximum = Vector3::Max(maximum, vertex.Position);
        }
        bounds_ = BoundingBox(minimum, maximum);

        fallbackEffect_->setVertexColorEnabledProperty(true);
        fallbackEffect_->setLightingEnabledProperty(false);
        fallbackEffect_->setFogEnabledProperty(true);
        fallbackEffect_->setFogColorProperty(Vector3(0.53f, 0.78f, 0.88f));
        fallbackEffect_->setFogStartProperty(190.0f);
        fallbackEffect_->setFogEndProperty(520.0f);

        if (device_.SupportsCapability(CNA::GraphicsCapability::CustomEffects)) {
            try {
                agedWoodEffect_ = std::make_unique<ShaderEffect>(device_, CreateShaderPackage());
                if (agedWoodEffect_->IsEffectValid()) {
                    agedWoodShaderReady_ = true;
                    std::cout << AgedWoodDiagnostic << '\n';
                    MirrorRuntimeDiagnosticToPlatformLog(AgedWoodDiagnostic);
                } else {
                    std::cerr << "island-simulator: shipwreck material fallback: "
                              << agedWoodEffect_->GetCompileErrorEXT() << '\n';
                    agedWoodEffect_.reset();
                }
            } catch (const std::exception& exception) {
                std::cerr << "island-simulator: shipwreck material fallback: "
                          << exception.what() << '\n';
                agedWoodEffect_.reset();
            }
        } else {
            std::cerr << "island-simulator: shipwreck material fallback: "
                         "custom effects unavailable\n";
        }

        const std::string diagnostic = "island-simulator: shipwreck 20m broken hull, "
            + std::to_string(debrisCount_) + " debris pieces, one portable culled batch, "
            + std::to_string(primitiveCount_) + " triangles";
        std::cout << diagnostic << '\n';
        std::cout << "island-simulator: shipwreck broken hull exposes ribs, partial deck, "
                     "and snapped mast\n";
        std::cout << LargeShipDetailDiagnostic << '\n';
        MirrorRuntimeDiagnosticToPlatformLog(diagnostic);
        MirrorRuntimeDiagnosticToPlatformLog(
            "island-simulator: shipwreck broken hull exposes ribs, partial deck, and snapped mast");
        MirrorRuntimeDiagnosticToPlatformLog(LargeShipDetailDiagnostic);
    }

    ShipwreckRenderer::~ShipwreckRenderer() = default;

    void ShipwreckRenderer::Draw(const Matrix& view, const Matrix& projection,
                                 const Vector3& cameraPosition)
    {
        drawCallCount_ = 0;
        visible_ = false;
        const float deltaX = centre_.X - cameraPosition.X;
        const float deltaZ = centre_.Z - cameraPosition.Z;
        if (deltaX * deltaX + deltaZ * deltaZ
            > MaximumDrawDistance * MaximumDrawDistance) {
            return;
        }
        const BoundingFrustum frustum(view * projection);
        if (frustum.Contains(bounds_) == ContainmentType::Disjoint) return;

        device_.setRasterizerStateProperty(RasterizerState::CullNone);
        device_.setBlendStateProperty(BlendState::Opaque);
        device_.setDepthStencilStateProperty(DepthStencilState::Default);
        device_.SetVertexBuffer(vertices_.get());
        device_.setIndicesProperty(nullptr);
        if (agedWoodShaderReady_) {
            float viewProjection[16];
            (view * projection).ToColumnMajor(viewProjection);
            agedWoodEffect_->Apply();
            agedWoodEffect_->SetUniformMat4("uViewProjection", viewProjection);
            agedWoodEffect_->SetUniformVec3("uCameraPosition", cameraPosition.X,
                                            cameraPosition.Y, cameraPosition.Z);
            agedWoodEffect_->SetUniformVec3("uShipCentre", centre_.X, centre_.Y, centre_.Z);
            agedWoodEffect_->SetUniformVec3("uShipRight", woodRight_.X,
                                            woodRight_.Y, woodRight_.Z);
            agedWoodEffect_->SetUniformVec3("uShipForward", woodForward_.X,
                                            woodForward_.Y, woodForward_.Z);
            agedWoodEffect_->SetUniformVec3(
                "uToSun", environment::SunDirectionX, environment::SunDirectionY,
                environment::SunDirectionZ);
        } else {
            fallbackEffect_->setWorldProperty(Matrix::getIdentityProperty());
            fallbackEffect_->setViewProperty(view);
            fallbackEffect_->setProjectionProperty(projection);
            fallbackEffect_->Apply();
        }
        device_.DrawPrimitives(PrimitiveType::TriangleList, 0, primitiveCount_);
        drawCallCount_ = 1;
        visible_ = true;
    }
}
