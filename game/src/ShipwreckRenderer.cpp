#include "IslandSimulator/ShipwreckRenderer.hpp"

#include "IslandSimulator/EnvironmentLighting.hpp"
#include "IslandSimulator/OpaqueImageLoader.hpp"
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
#include "Microsoft/Xna/Framework/Graphics/SamplerState.hpp"
#include "Microsoft/Xna/Framework/Graphics/ShaderEffect.hpp"
#include "Microsoft/Xna/Framework/Graphics/SurfaceFormat.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"
#include "Microsoft/Xna/Framework/Graphics/VertexBuffer.hpp"
#include "Microsoft/Xna/Framework/Graphics/VertexPositionColorTexture.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"

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
        constexpr std::string_view BoardTextureDiagnostic =
            "island-simulator: shipwreck weathered wood follows per-piece board and beam UVs";
        constexpr std::string_view IronFittingDiagnostic =
            "island-simulator: shipwreck restrained rusted iron straps, rivets, hinges and rudder braces";
        constexpr std::string_view HullShapeDiagnostic =
            "island-simulator: shipwreck hull uses asymmetric rising sheer, curved keel, and irregular splintered break sections";
        constexpr std::string_view BroadsideDamageDiagnostic =
            "island-simulator: shipwreck beach broadside has true missing plank bays with exposed structural ribs";
        constexpr std::string_view EndProfileDiagnostic =
            "island-simulator: shipwreck bow and stern use raked lower profiles with thick end timbers";
        constexpr std::string_view PlankThicknessDiagnostic =
            "island-simulator: shipwreck missing plank bays expose eleven-centimetre inner edges";
        constexpr std::string_view DeckhouseDamageDiagnostic =
            "island-simulator: shipwreck stern deckhouse uses separated broken roof planks and framed window bays";
        constexpr std::string_view SettledDebrisDiagnostic =
            "island-simulator: shipwreck beach debris uses buried ends, restrained warp, unequal settling, and five genuinely split long planks";
        constexpr std::string_view BrokenDeckDiagnostic =
            "island-simulator: shipwreck uses separated deck planks, open fracture gaps, and collapsed break timbers";
        constexpr int AgedWoodMaterial = 48;
        constexpr int InnerWoodMaterial = 80;
        constexpr int FreshBreakMaterial = 176;
        constexpr int AgedIronMaterial = 232;
        const Color OuterWood(112, 79, 45, AgedWoodMaterial);
        const Color DarkWood(77, 52, 33, AgedWoodMaterial);
        const Color InnerWood(137, 103, 66, InnerWoodMaterial);
        const Color FreshBreak(132, 99, 64, FreshBreakMaterial);
        const Color WetWood(63, 59, 47, AgedWoodMaterial);
        const Color DeepWood(43, 31, 22, AgedWoodMaterial);
        const Color AgedIron(49, 47, 42, AgedIronMaterial);
        const Color RustedIron(77, 47, 31, AgedIronMaterial);

        constexpr const char* VertexBody = R"(
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec4 aColor;
layout(location = 2) in vec2 aWoodCoordinate;

uniform mat4 uViewProjection;

out vec3 vColor;
out vec3 vWorld;
out float vMaterial;
out vec2 vWoodCoordinate;

void main()
{
    vColor = aColor.rgb;
    vWorld = aPosition;
    vMaterial = aColor.a;
    vWoodCoordinate = aWoodCoordinate;
    gl_Position = uViewProjection * vec4(aPosition, 1.0);
    gl_Position.z = 2.0 * gl_Position.z - gl_Position.w;
}
)";

        constexpr const char* FragmentBody = R"(
in vec3 vColor;
in vec3 vWorld;
in float vMaterial;
in vec2 vWoodCoordinate;

uniform vec3 uCameraPosition;
uniform vec3 uShipCentre;
uniform vec3 uShipRight;
uniform vec3 uShipForward;
uniform vec3 uToSun;
uniform sampler2D uWoodTexture;
uniform vec3 uWoodMean;

out vec4 fragColor;

void main()
{
    float distanceToCamera = length(vWorld - uCameraPosition);
    float detailFade = 1.0 - smoothstep(72.0, 132.0, distanceToCamera);
    vec3 relative = vWorld - uShipCentre;
    float along = dot(relative, uShipForward);
    float across = dot(relative, uShipRight);
    vec3 faceVector = cross(dFdx(vWorld), dFdy(vWorld));
    vec3 faceNormal = normalize(faceVector);
    vec2 mirrored = abs(fract(vWoodCoordinate * 0.5) * 2.0 - 1.0);
    vec2 atlasCoordinate = mix(vec2(0.075), vec2(0.925), mirrored);
    vec3 sampledWood = texture(uWoodTexture, atlasCoordinate).rgb;
    vec3 woodModulation = clamp(sampledWood / max(uWoodMean, vec3(0.08)),
                                vec3(0.48), vec3(1.52));

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

    float metal = smoothstep(0.84, 0.90, vMaterial);
    float freshBreak = smoothstep(0.52, 0.72, vMaterial) * (1.0 - metal);
    vec3 aged = vColor * (0.925 + 0.105 * longGrain + 0.035 * fineGrain);
    aged *= 1.0 - darkWeather * 0.14 - split * 0.055;
    aged = mix(aged, aged * vec3(0.78, 0.85, 0.82), darkWeather * 0.22);
    vec3 broken = vColor * (0.95 + 0.075 * fineGrain);
    broken = mix(broken, broken * vec3(1.07, 1.02, 0.90), 0.35 * longGrain);
    vec3 materialColor = mix(aged, broken, freshBreak);
    float textureInfluence = detailFade * (1.0 - freshBreak * 0.86) * (1.0 - metal);
    materialColor *= mix(vec3(1.0), woodModulation, textureInfluence * 0.58);
    materialColor = mix(vColor, materialColor, detailFade);

    float rustField = sin(along * 5.17 + vWorld.y * 3.41)
        * sin(across * 6.23 - vWorld.y * 4.09);
    float rust = smoothstep(0.36, 0.89, rustField) * detailFade;
    vec3 iron = vColor * (0.84 + 0.12 * fineGrain);
    iron = mix(iron, vec3(0.29, 0.13, 0.065), rust * 0.58);
    iron = mix(vColor, iron, detailFade);
    materialColor = mix(materialColor, iron, metal);

    float wetBand = (1.0 - smoothstep(-0.14, 0.38, vWorld.y))
        * (1.0 - freshBreak * 0.25);
    float submerged = 1.0 - smoothstep(-0.52, 0.05, vWorld.y);
    vec3 wetColor = materialColor * vec3(0.59, 0.68, 0.67);
    wetColor = mix(wetColor, vec3(dot(wetColor, vec3(0.30, 0.59, 0.11)))
        * vec3(0.77, 0.91, 0.79), submerged * 0.24);
    materialColor = mix(materialColor, wetColor, wetBand * (0.74 + 0.18 * detailFade));

    // Years of wave wash leave the lowest timbers stained and partly packed with
    // pale lagoon sand. This remains a bounded material cue rather than a second
    // seabed surface that could float when terrain placement changes.
    float sandField = 0.5 + 0.5 * sin(along * 2.37 + across * 3.11
        + sin(along * 0.83 - across * 1.29) * 1.2);
    float sandBurial = (1.0 - smoothstep(-0.70, -0.24, vWorld.y))
        * smoothstep(0.32, 0.78, sandField) * (1.0 - metal) * detailFade;
    materialColor = mix(materialColor, vec3(0.43, 0.36, 0.25), sandBurial * 0.34);

    float saltLine = exp(-abs(vWorld.y - 0.18) * 13.0) * detailFade
        * (1.0 - freshBreak) * (1.0 - metal * 0.72) * (0.68 + 0.32 * fineGrain);
    materialColor = mix(materialColor,
                        materialColor * vec3(1.13, 1.12, 1.05),
                        saltLine * 0.24);

    vec3 toViewer = normalize(uCameraPosition - vWorld);
    if (dot(faceNormal, toViewer) < 0.0) faceNormal = -faceNormal;
    vec3 reflectedSun = reflect(-normalize(uToSun), faceNormal);
    float wetSheen = pow(max(dot(reflectedSun, toViewer), 0.0), 42.0)
        * wetBand * (1.0 - submerged) * (0.12 + metal * 0.10);
    materialColor += vec3(1.00, 0.91, 0.70) * wetSheen;
    float ironSheen = pow(max(dot(reflectedSun, toViewer), 0.0), 22.0)
        * metal * detailFade * (1.0 - rust) * 0.085;
    materialColor += vec3(0.76, 0.78, 0.75) * ironSheen;

    float fog = smoothstep(190.0, 520.0, distanceToCamera);
    vec3 color = mix(materialColor, islandAtmosphereFog, fog);
    fragColor = vec4(color, 1.0);
}
)";

        [[nodiscard]] std::string ShaderSource(const bool embedded, const char* body)
        {
            if (embedded) {
                return std::string("#version 300 es\nprecision highp float;\n")
                    + environment::FogShaderDefinition + body;
            }
            return std::string("#version 330 core\n")
                + environment::FogShaderDefinition + body;
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
            // The two wreck halves have settled independently after the keel failed.
            // Apply a very small piece-local roll and sink here so every attached rib,
            // fitting, mast and rail follows its own hull instead of floating away.
            // The open fracture contains the smooth hand-off, where only broken debris
            // spans the discontinuity.
            const float pieceBlend = std::clamp((z + 0.78f) / 1.56f, 0.0f, 1.0f);
            const float localRoll = -0.018f + pieceBlend * 0.046f;
            const float localSink = 0.025f - pieceBlend * 0.105f;
            const float cosine = std::cos(localRoll);
            const float sine = std::sin(localRoll);
            const float settledX = x * cosine - y * sine;
            const float settledY = x * sine + y * cosine + localSink;
            return Vector3(ship.x, ship.y, ship.z) + axes.right * settledX
                + axes.up * settledY + axes.forward * z;
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

        enum class GrainAxis
        {
            Longest,
            AlongAB,
            AlongAD,
        };

        void AddTriangle(std::vector<VertexPositionColorTexture>& vertices,
                         const Vector3& a, const Vector3& b, const Vector3& c,
                         const Color& color, const Vector2& textureA,
                         const Vector2& textureB, const Vector2& textureC)
        {
            const Color lit = LitColor(a, b, c, color);
            vertices.emplace_back(a, lit, textureA);
            vertices.emplace_back(b, lit, textureB);
            vertices.emplace_back(c, lit, textureC);
        }

        void AddTriangle(std::vector<VertexPositionColorTexture>& vertices,
                         const Vector3& a, const Vector3& b, const Vector3& c,
                         const Color& color)
        {
            AddTriangle(vertices, a, b, c, color, Vector2(0.0f, 0.0f),
                        Vector2((b - a).Length() * 0.19f, 0.0f),
                        Vector2(0.0f, (c - a).Length() * 0.34f));
        }

        void AddQuad(std::vector<VertexPositionColorTexture>& vertices,
                     const Vector3& a, const Vector3& b, const Vector3& c,
                     const Vector3& d, const Color& color,
                     const GrainAxis grainAxis = GrainAxis::Longest)
        {
            const float lengthAB = (b - a).Length();
            const float lengthAD = (d - a).Length();
            const bool alongAB = grainAxis == GrainAxis::AlongAB
                || (grainAxis == GrainAxis::Longest && lengthAB >= lengthAD);
            const float along = (alongAB ? lengthAB : lengthAD) * 0.19f;
            const float acrossLength = (alongAB ? lengthAD : lengthAB) * 0.34f;
            const Vector2 textureA(0.0f, 0.0f);
            const Vector2 textureB = alongAB ? Vector2(along, 0.0f)
                                             : Vector2(0.0f, acrossLength);
            const Vector2 textureC(along, acrossLength);
            const Vector2 textureD = alongAB ? Vector2(0.0f, acrossLength)
                                             : Vector2(along, 0.0f);
            AddTriangle(vertices, a, b, c, color, textureA, textureB, textureC);
            AddTriangle(vertices, a, c, d, color, textureA, textureC, textureD);
        }

        void AppendBox(std::vector<VertexPositionColorTexture>& vertices, const Vector3& centre,
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

        void AppendBrokenPlank(std::vector<VertexPositionColorTexture>& vertices,
                               const Vector3& centre, const Axes& axes,
                               const float halfWidth, const float halfHeight,
                               const float halfLength, const Color& color,
                               const std::size_t shapeSeed,
                               const float settlingWarp = 0.0f)
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
                // Beach debris should follow years of wet/dry settling rather than
                // remain a perfectly straight carpenter's board. The terminal bias
                // tucks splintered ends into sand while the small alternating crown
                // remains below the plank thickness and cannot read as a bent beam.
                constexpr std::array<float, 5> Settling = {-0.62f, 0.18f, -0.24f, 0.12f, -0.48f};
                const float seededSettling = Settling[point]
                    + 0.08f * (static_cast<float>((shapeSeed + point * 3U) % 5U) - 2.0f);
                const Vector3 sectionCentre = centre
                    + axes.forward * (Along[point] * halfLength)
                    + axes.right * (centreOffset[point] * halfWidth)
                    + axes.up * (settlingWarp * seededSettling);
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

        void AppendTaperedBeam(std::vector<VertexPositionColorTexture>& vertices,
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

        [[nodiscard]] float HullSheer(const float z)
        {
            const float distance = std::clamp(std::abs(z) / 10.0f, 0.0f, 1.0f);
            // A sailing hull's deck edge rises toward both ends, more strongly at the bow.
            // Keeping this in local ship space also curves the keel instead of lifting the wreck.
            return (z >= 0.0f ? 0.64f : 0.34f) * std::pow(distance, 2.2f);
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

        [[nodiscard]] float HullEndRake(const float z, const float y)
        {
            const float endWeight = std::clamp((std::abs(z) - 8.55f) / 1.45f,
                                               0.0f, 1.0f);
            const float lowerWeight = std::clamp((1.33f - y) / 2.81f,
                                                 0.0f, 1.0f);
            // Keep the authored +/-10 m deck-edge endpoints while pulling the keel
            // inward. This replaces flat vertical end cuts with recognizable raked
            // bow/stern profiles without changing layout, collision, or landmark scale.
            return endWeight * lowerWeight * (z >= 0.0f ? -0.90f : 0.65f);
        }

        [[nodiscard]] std::array<Vector3, 11> HullSection(
            const ShipwreckPlacement& ship, const Axes& axes, const float z)
        {
            constexpr std::array<float, 5> SideHeights = {
                1.33f, 0.88f, 0.42f, -0.10f, -0.72f};
            std::array<Vector3, 11> points;
            const float sheer = HullSheer(z);
            for (std::size_t level = 0; level < SideHeights.size(); ++level) {
                const float height = SideHeights[level];
                const float width = HullWidthAtHeight(ship, z, height);
                const float rakedZ = z + HullEndRake(z, height);
                points[level] = ShipPoint(ship, axes, -width, height + sheer, rakedZ);
                points[points.size() - 1U - level] =
                    ShipPoint(ship, axes, width, height + sheer, rakedZ);
            }
            points[SideHeights.size()] = ShipPoint(
                ship, axes, 0.0f, -1.48f + sheer, z + HullEndRake(z, -1.48f));
            return points;
        }

        [[nodiscard]] std::array<Vector3, 11> BrokenHullSection(
            const ShipwreckPlacement& ship, const Axes& axes, const float z,
            const float outwardDirection)
        {
            auto points = HullSection(ship, axes, z);
            constexpr std::array<float, 11> SplinterOffset = {
                0.07f, -0.18f, 0.24f, -0.09f, 0.31f, 0.02f,
                -0.26f, 0.13f, -0.21f, 0.19f, -0.06f};
            for (std::size_t point = 0; point < points.size(); ++point)
                points[point] += axes.forward * (outwardDirection * SplinterOffset[point]);
            return points;
        }

        template<std::size_t SectionCount>
        void AppendHullPiece(std::vector<VertexPositionColorTexture>& vertices,
                             const ShipwreckPlacement& ship, const Axes& axes,
                             const std::array<float, SectionCount>& sectionZ,
                             const bool capFirst)
        {
            const auto sectionAt = [&](const std::size_t index) {
                const bool broken = capFirst ? index + 1U == sectionZ.size() : index == 0U;
                return broken
                    ? BrokenHullSection(ship, axes, sectionZ[index], capFirst ? -1.0f : 1.0f)
                    : HullSection(ship, axes, sectionZ[index]);
            };
            for (std::size_t section = 0; section + 1U < sectionZ.size(); ++section) {
                const auto first = sectionAt(section);
                const auto next = sectionAt(section + 1U);
                for (std::size_t band = 0; band + 1U < first.size(); ++band) {
                    const bool nearBrokenEnd = capFirst
                        ? section + 2U >= sectionZ.size()
                        : section < 2U;
                    const bool missingBoard = nearBrokenEnd
                        && ((capFirst && band == 2U && section % 2U == 0U)
                            || (!capFirst && band == 7U && section % 2U == 1U));
                    // The authored camera and beach approach see the negative-across
                    // broadside (bands 0..4). Remove real planking there rather than
                    // painting more black rectangles over an otherwise sealed hull.
                    const float centreZ = (sectionZ[section] + sectionZ[section + 1U]) * .5f;
                    const bool broadsideBreach =
                        (centreZ > -3.6f && centreZ < -2.4f && (band == 1U || band == 2U))
                        || (centreZ > 3.2f && centreZ < 4.8f && band == 1U)
                        || (centreZ > 6.3f && centreZ < 7.8f && band == 2U);
                    const float tone = 0.89f + 0.035f
                        * static_cast<float>((section * 3U + band * 2U) % 6U);
                    const Color color = ScaleColor(OuterWood, tone);
                    if (missingBoard) continue;
                    if (broadsideBreach) {
                        const float lowerCut = band % 2U == 0U ? .29f : .17f;
                        const float upperCut = band % 2U == 0U ? .86f : .73f;
                        const auto interpolate=[](const Vector3& a,const Vector3& b,
                                                  const float amount) {
                            return a+(b-a)*amount;
                        };
                        const Vector3 lowFirst=interpolate(first[band],next[band],lowerCut);
                        const Vector3 highFirst=interpolate(first[band+1U],next[band+1U],lowerCut);
                        const Vector3 lowSecond=interpolate(first[band],next[band],upperCut);
                        const Vector3 highSecond=interpolate(first[band+1U],next[band+1U],upperCut);
                        AddQuad(vertices,first[band],lowFirst,highFirst,first[band+1U],color);
                        AddQuad(vertices,lowSecond,next[band],next[band+1U],highSecond,color);
                        // The visible beach broadside is the negative-across shell, so
                        // positive ship-right points into the hull. Close all four sides
                        // of the missing bay with a modest real plank depth rather than
                        // leaving the retained boards as infinitely thin cut-outs.
                        const Vector3 inward = axes.right * 0.11f;
                        const Color brokenEdge = ScaleColor(FreshBreak, 0.86f);
                        AddQuad(vertices, lowFirst, lowFirst + inward,
                                highFirst + inward, highFirst, brokenEdge);
                        AddQuad(vertices, highSecond, highSecond + inward,
                                lowSecond + inward, lowSecond, brokenEdge);
                        AddQuad(vertices, lowSecond, lowSecond + inward,
                                lowFirst + inward, lowFirst, InnerWood);
                        AddQuad(vertices, highFirst, highFirst + inward,
                                highSecond + inward, highSecond, InnerWood);
                        continue;
                    }
                    AddQuad(vertices, first[band], next[band], next[band + 1U],
                            first[band + 1U], color);
                }

                const float firstWidth = HullHalfWidth(ship, sectionZ[section]) * 0.74f;
                const float nextWidth = HullHalfWidth(ship, sectionZ[section + 1U]) * 0.74f;
                const Vector3 deckFirstLeft = ShipPoint(
                    ship, axes, -firstWidth, 1.16f + HullSheer(sectionZ[section]),
                    sectionZ[section] + HullEndRake(sectionZ[section], 1.16f));
                const Vector3 deckFirstRight = ShipPoint(
                    ship, axes, firstWidth, 1.16f + HullSheer(sectionZ[section]),
                    sectionZ[section] + HullEndRake(sectionZ[section], 1.16f));
                const Vector3 deckNextLeft = ShipPoint(
                    ship, axes, -nextWidth, 1.16f + HullSheer(sectionZ[section + 1U]),
                    sectionZ[section + 1U]
                        + HullEndRake(sectionZ[section + 1U], 1.16f));
                const Vector3 deckNextRight = ShipPoint(
                    ship, axes, nextWidth, 1.16f + HullSheer(sectionZ[section + 1U]),
                    sectionZ[section + 1U]
                        + HullEndRake(sectionZ[section + 1U], 1.16f));
                // Keep the deck walkable-looking but no longer cast it as one pristine
                // sheet. Seven unequal lanes leave real dark seams and a few missing
                // boards beside each torn centre section. Small deterministic lifts
                // make the warped planks catch light individually from elevated views.
                constexpr std::size_t DeckLaneCount = 7U;
                const bool fractureSection = capFirst
                    ? section + 2U >= sectionZ.size()
                    : section < 2U;
                for (std::size_t lane = 0U; lane < DeckLaneCount; ++lane) {
                    const bool missingLane = fractureSection
                        && ((capFirst && (lane == 1U || lane == 5U))
                            || (!capFirst && (lane == 2U || lane == 6U)));
                    if (missingLane) continue;
                    const float firstFraction =
                        (static_cast<float>(lane) + 0.055f) / static_cast<float>(DeckLaneCount);
                    const float secondFraction =
                        (static_cast<float>(lane) + 0.945f) / static_cast<float>(DeckLaneCount);
                    const auto interpolate = [](const Vector3& a, const Vector3& b,
                                                const float amount) {
                        return a + (b - a) * amount;
                    };
                    const float lift = 0.006f
                        * static_cast<float>((section * 5U + lane * 3U) % 4U);
                    const Vector3 boardFirstLeft = interpolate(
                        deckFirstLeft, deckFirstRight, firstFraction) + axes.up * lift;
                    const Vector3 boardFirstRight = interpolate(
                        deckFirstLeft, deckFirstRight, secondFraction) + axes.up * lift;
                    const Vector3 boardNextLeft = interpolate(
                        deckNextLeft, deckNextRight, firstFraction) + axes.up * lift;
                    const Vector3 boardNextRight = interpolate(
                        deckNextLeft, deckNextRight, secondFraction) + axes.up * lift;
                    const Color deckColor = (section + lane) % 5U == 0U
                        ? InnerWood : ((section + lane) % 2U == 0U ? OuterWood : DarkWood);
                    AddQuad(vertices, boardFirstLeft, boardFirstRight, boardNextRight,
                            boardNextLeft, deckColor, GrainAxis::AlongAD);
                }

                const float centreZ = (sectionZ[section] + sectionZ[section + 1U]) * 0.5f;
                const float halfLength = (sectionZ[section + 1U] - sectionZ[section]) * 0.5f;
                const float centreWidth = HullHalfWidth(ship, centreZ) * 0.92f;
                AppendBox(vertices, ShipPoint(ship, axes, -centreWidth, 1.38f + HullSheer(centreZ), centreZ),
                          axes, 0.10f, 0.10f, halfLength, DarkWood);
                AppendBox(vertices, ShipPoint(ship, axes, centreWidth, 1.38f + HullSheer(centreZ), centreZ),
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
                                                    seamY - 0.030f + HullSheer(sectionZ[section]),
                                                    sectionZ[section]
                                                        + HullEndRake(sectionZ[section], seamY));
                        const Vector3 b = ShipPoint(ship, axes, side * nextSide,
                                                    seamY - 0.030f + HullSheer(sectionZ[section + 1U]),
                                                    sectionZ[section + 1U]
                                                        + HullEndRake(sectionZ[section + 1U], seamY));
                        const Vector3 c = ShipPoint(ship, axes, side * nextSide,
                                                    seamY + 0.030f + HullSheer(sectionZ[section + 1U]),
                                                    sectionZ[section + 1U]
                                                        + HullEndRake(sectionZ[section + 1U], seamY));
                        const Vector3 d = ShipPoint(ship, axes, side * firstSide,
                                                    seamY + 0.030f + HullSheer(sectionZ[section]),
                                                    sectionZ[section]
                                                        + HullEndRake(sectionZ[section], seamY));
                        AddQuad(vertices, a, b, c, d, DarkWood);
                    }
                }
            }

            const auto first = sectionAt(0U);
            const auto last = sectionAt(sectionZ.size() - 1U);
            if (capFirst) {
                for (std::size_t band = 0; band + 1U < first.size(); ++band) {
                    AddTriangle(
                        vertices,
                        ShipPoint(ship, axes, 0.0f,
                                  0.38f + HullSheer(sectionZ.front()),
                                  sectionZ.front()
                                      + HullEndRake(sectionZ.front(), 0.38f)),
                        first[band + 1U], first[band], DarkWood);
                }
            } else {
                for (std::size_t band = 0; band + 1U < first.size(); ++band) {
                    AppendTaperedBeam(vertices, first[band], first[band + 1U],
                                      0.080f, 0.055f, FreshBreak);
                }
            }
            for (std::size_t band = 0; band + 1U < last.size(); ++band) {
                AppendTaperedBeam(vertices, last[band], last[band + 1U],
                                  0.080f, 0.055f, FreshBreak);
            }
        }

        void AppendRib(std::vector<VertexPositionColorTexture>& vertices,
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

        void AppendIronFittings(std::vector<VertexPositionColorTexture>& vertices,
                                const ShipwreckPlacement& ship, const Axes& axes)
        {
            constexpr std::array<float, 4> StrapPositions = {
                -7.25f, -4.25f, 4.15f, 7.15f};
            for (std::size_t strap = 0; strap < StrapPositions.size(); ++strap) {
                const float z = StrapPositions[strap];
                const float width = HullWidthAtHeight(ship, z, 0.28f);
                for (const float side : {-1.0f, 1.0f}) {
                    const Color& iron = (strap + (side > 0.0f ? 1U : 0U)) % 3U == 0U
                        ? RustedIron : AgedIron;
                    AppendBox(vertices,
                              ShipPoint(ship, axes, side * (width + 0.035f),
                                        0.28f + HullSheer(z), z),
                              axes, 0.030f, 0.67f, 0.070f, iron);
                    for (const float rivetY : {-0.20f, 0.76f}) {
                        const float innerX = side * (width + 0.060f);
                        const float outerX = side * (width + 0.135f);
                        AppendTaperedBeam(
                            vertices, ShipPoint(ship, axes, innerX, rivetY + HullSheer(z), z),
                            ShipPoint(ship, axes, outerX, rivetY + HullSheer(z), z),
                            0.047f, 0.042f, AgedIron);
                    }
                }
            }

            for (const float side : {-1.0f, 1.0f}) {
                for (const float hingeZ : {-7.30f, -6.55f}) {
                    AppendBox(vertices,
                              ShipPoint(ship, axes, side * 1.525f,
                                        2.08f + HullSheer(hingeZ), hingeZ),
                              axes, 0.030f, 0.045f, 0.19f, RustedIron);
                }
            }
            for (const float braceY : {-0.84f, 0.06f}) {
                AppendBox(vertices, ShipPoint(ship, axes, 0.0f,
                                              braceY + HullSheer(-10.37f), -10.37f), axes,
                          0.50f, 0.055f, 0.030f, AgedIron);
            }
        }

        void AppendShip(std::vector<VertexPositionColorTexture>& vertices,
                        const ShipwreckPlacement& ship)
        {
            const Axes axes = ShipAxes(ship);
            constexpr std::array aftSections = {
                -10.0f, -8.7f, -7.1f, -5.4f, -3.8f, -2.4f, -1.3f, -0.95f};
            constexpr std::array bowSections = {
                1.15f, 2.0f, 3.25f, 4.8f, 6.35f, 7.65f, 8.7f, 9.4f, 10.0f};
            AppendHullPiece(vertices, ship, axes, aftSections, true);
            AppendHullPiece(vertices, ship, axes, bowSections, false);
            AppendIronFittings(vertices, ship, axes);

            AppendRib(vertices, ship, axes, -0.88f, -0.12f);
            AppendRib(vertices, ship, axes, 0.05f, 0.18f);
            AppendRib(vertices, ship, axes, 1.12f, -0.08f);
            // These frames sit directly behind the missing broadside plank bays.
            // Their darker inner wood gives the damage depth in the beach view.
            AppendRib(vertices, ship, axes, -3.18f, 0.06f);
            AppendRib(vertices, ship, axes, 4.04f, -0.05f);
            AppendRib(vertices, ship, axes, 6.92f, 0.04f);

            // The two hull halves were once one vessel. Broken deck beams now reach
            // into the fracture at unequal heights, with a few collapsed boards below
            // them, so the centre reads as structural failure rather than a clean gap.
            AppendTaperedBeam(
                vertices, ShipPoint(ship, axes, -1.42f, 1.31f, -0.92f),
                ShipPoint(ship, axes, -0.62f, 0.46f, 0.22f),
                0.12f, 0.045f, FreshBreak);
            AppendTaperedBeam(
                vertices, ShipPoint(ship, axes, 0.58f, 1.27f, -0.89f),
                ShipPoint(ship, axes, 1.12f, 0.71f, 0.48f),
                0.105f, 0.040f, FreshBreak);
            AppendTaperedBeam(
                vertices, ShipPoint(ship, axes, -0.15f, 1.22f, 1.13f),
                ShipPoint(ship, axes, 0.34f, 0.30f, -0.12f),
                0.11f, 0.038f, FreshBreak);
            const Axes fallenDeckAxes{
                Vector3::Normalize(axes.right + axes.forward * 0.18f), axes.up,
                Vector3::Normalize(axes.forward - axes.right * 0.18f)};
            AppendBrokenPlank(
                vertices, ShipPoint(ship, axes, -0.70f, 0.56f, -0.12f),
                fallenDeckAxes, 0.20f, 0.055f, 0.68f, InnerWood, 401U, 0.028f);
            AppendBrokenPlank(
                vertices, ShipPoint(ship, axes, 0.42f, 0.30f, 0.16f),
                fallenDeckAxes, 0.18f, 0.050f, 0.55f, FreshBreak, 402U, 0.022f);

            AppendTaperedBeam(
                vertices, ShipPoint(ship, axes, 0.0f, -1.42f + HullSheer(-9.55f), -9.55f),
                ShipPoint(ship, axes, 0.0f, -1.45f + HullSheer(-1.05f), -1.05f),
                0.14f, 0.11f, DarkWood);
            AppendTaperedBeam(
                vertices, ShipPoint(ship, axes, 0.0f, -1.45f + HullSheer(1.15f), 1.15f),
                ShipPoint(ship, axes, 0.0f, -1.45f + HullSheer(9.45f), 9.45f),
                0.12f, 0.10f, DarkWood);
            AppendTaperedBeam(
                vertices, ShipPoint(ship, axes, 0.0f, -1.36f + HullSheer(-9.40f), -9.40f),
                ShipPoint(ship, axes, 0.0f, 1.26f + HullSheer(-10.0f), -10.0f),
                0.15f, 0.11f, DarkWood);
            AppendBox(vertices, ShipPoint(ship, axes, 0.0f,
                                          -0.46f + HullSheer(-10.28f), -10.28f), axes,
                      0.46f, 0.74f, 0.075f, OuterWood);

            AppendTaperedBeam(
                vertices, ShipPoint(ship, axes, 0.0f, 1.18f + HullSheer(-3.25f), -3.25f),
                ShipPoint(ship, axes, 1.65f, 7.15f + HullSheer(-1.75f), -1.75f),
                0.34f, 0.20f, DarkWood);
            AppendTaperedBeam(
                vertices, ShipPoint(ship, axes, -2.1f, 5.45f + HullSheer(-2.15f), -2.15f),
                ShipPoint(ship, axes, 3.0f, 5.85f + HullSheer(-1.35f), -1.35f),
                0.13f, 0.09f, DarkWood);

            AppendTaperedBeam(
                vertices, ShipPoint(ship, axes, 0.0f, 1.08f + HullSheer(9.1f), 9.1f),
                ShipPoint(ship, axes, 0.0f, 2.12f + HullSheer(13.2f), 13.2f),
                0.18f, 0.10f, DarkWood);
            AppendTaperedBeam(
                vertices, ShipPoint(ship, axes, 1.65f, 7.15f + HullSheer(-1.75f), -1.75f),
                ShipPoint(ship, axes, 0.0f, 1.92f + HullSheer(8.8f), 8.8f),
                0.045f, 0.035f, DarkWood);
            AppendTaperedBeam(
                vertices, ShipPoint(ship, axes, 1.65f, 7.15f + HullSheer(-1.75f), -1.75f),
                ShipPoint(ship, axes, -1.05f, 1.75f + HullSheer(-8.4f), -8.4f),
                0.045f, 0.035f, DarkWood);

            for (const float z : {-7.9f, -5.9f, -3.7f, 3.4f, 5.8f, 7.8f}) {
                const float railX = HullHalfWidth(ship, z) * 0.88f;
                for (const float side : {-1.0f, 1.0f}) {
                    AppendBox(vertices, ShipPoint(ship, axes, side * railX,
                                                  1.70f + HullSheer(z), z),
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
                        vertices, ShipPoint(ship, axes, side * firstX,
                                            2.05f + HullSheer(section[0]), section[0]),
                        ShipPoint(ship, axes, side * secondX,
                                  2.05f + HullSheer(section[1]), section[1]),
                        0.065f, 0.055f, DarkWood);
                }
            }

            for (const float z : {-4.9f, 6.0f}) {
                constexpr float openingY = 0.53f;
                const float openingX = HullWidthAtHeight(ship, z, openingY) + 0.022f;
                for (const float side : {-1.0f, 1.0f}) {
                    AppendBox(vertices,
                              ShipPoint(ship, axes, side * openingX,
                                        openingY + HullSheer(z), z), axes,
                              0.035f, 0.21f, 0.29f, DeepWood);
                }
            }

            const Vector3 foremastBreak = ShipPoint(
                ship, axes, -0.34f, 4.08f + HullSheer(4.78f), 4.78f);
            AppendTaperedBeam(
                vertices, ShipPoint(ship, axes, 0.0f, 1.18f + HullSheer(4.35f), 4.35f),
                foremastBreak,
                0.25f, 0.15f, DarkWood);
            AppendTaperedBeam(
                vertices, ShipPoint(ship, axes, -0.28f, 3.98f + HullSheer(4.73f), 4.73f),
                ShipPoint(ship, axes, -0.58f, 4.48f + HullSheer(4.92f), 4.92f),
                0.08f, 0.025f, FreshBreak);
            AppendTaperedBeam(
                vertices, ShipPoint(ship, axes, -0.25f, 4.02f + HullSheer(4.74f), 4.74f),
                ShipPoint(ship, axes, 0.08f, 4.38f + HullSheer(4.56f), 4.56f),
                0.07f, 0.022f, FreshBreak);

            for (const float side : {-1.0f, 1.0f}) {
                AppendTaperedBeam(
                    vertices, ShipPoint(ship, axes, side * 1.45f,
                                        1.17f + HullSheer(-0.92f), -0.92f),
                    ShipPoint(ship, axes, side * 1.78f,
                              2.65f + HullSheer(0.25f), 0.25f),
                    0.13f, 0.08f, FreshBreak);
                AppendTaperedBeam(
                    vertices, ShipPoint(ship, axes, side * 1.50f,
                                        1.08f + HullSheer(1.12f), 1.12f),
                    ShipPoint(ship, axes, side * 2.25f,
                              2.05f + HullSheer(2.30f), 2.30f),
                    0.12f, 0.07f, FreshBreak);
            }

            for (int post = 0; post < 4; ++post) {
                const float x = post < 2 ? -1.35f : 1.35f;
                const float z = post % 2 == 0 ? -8.1f : -5.8f;
                AppendBox(vertices, ShipPoint(ship, axes, x,
                                              2.0f + HullSheer(z), z), axes,
                          0.11f, 0.80f, 0.11f, DarkWood);
            }
            const float cabinSheer = HullSheer(-6.9f);
            // The surviving stern deckhouse is open, framed wreckage rather than one
            // roof slab over two solid box walls. Longitudinal roof boards retain small
            // gaps, uneven ends and one central fracture that reads from the beach.
            for (int plank = 0; plank < 10; ++plank) {
                const float x = -1.35f + static_cast<float>(plank) * .30f;
                const float lift = static_cast<float>((plank * 5) % 4) * .012f;
                const float length = 1.24f + static_cast<float>((plank * 3) % 5) * .027f;
                if (plank == 2) {
                    AppendBrokenPlank(vertices,
                        ShipPoint(ship, axes, x, 2.62f + cabinSheer + lift, -7.56f),
                        axes, .135f, .065f, .48f, OuterWood, 301U);
                    AppendBrokenPlank(vertices,
                        ShipPoint(ship, axes, x, 2.62f + cabinSheer + lift, -6.22f),
                        axes, .135f, .065f, .48f, InnerWood, 302U);
                } else {
                    AppendBrokenPlank(vertices,
                        ShipPoint(ship, axes, x, 2.62f + cabinSheer + lift,
                                  -6.9f + (plank % 3 - 1) * .025f),
                        axes, .135f, .065f, length,
                        plank % 4 == 0 ? InnerWood : OuterWood,
                        310U + static_cast<std::size_t>(plank));
                }
            }
            // Lower/upper plank bands and four uprights form three side window bays.
            // The middle bay is genuinely open on both sides; the two surviving dark
            // panes are recessed behind their frames rather than painted on a wall.
            constexpr std::array<float,4> cabinPosts{{-8.02f,-7.28f,-6.54f,-5.78f}};
            constexpr std::array<float,3> cabinWindows{{-7.65f,-6.91f,-6.16f}};
            for (const float side : {-1.0f, 1.0f}) {
                AppendBox(vertices, ShipPoint(ship, axes, side * 1.43f,
                    1.70f + cabinSheer, -6.9f), axes, .08f, .21f, 1.22f, OuterWood);
                AppendBox(vertices, ShipPoint(ship, axes, side * 1.43f,
                    2.44f + cabinSheer, -6.9f), axes, .08f, .17f, 1.22f, OuterWood);
                for (const float z : cabinPosts)
                    AppendBox(vertices, ShipPoint(ship, axes, side * 1.43f,
                        2.07f + cabinSheer, z), axes, .08f, .37f, .06f, DarkWood);
                for (std::size_t window = 0; window < cabinWindows.size(); ++window) {
                    if (window == 1U) continue;
                    AppendBox(vertices, ShipPoint(ship, axes, side * 1.515f,
                        2.08f + cabinSheer, cabinWindows[window]), axes,
                        .018f, .235f, .27f, DeepWood);
                }
            }
        }

        [[nodiscard]] Axes HorizontalAxes(const float yaw)
        {
            return Axes{Vector3(std::cos(yaw), 0.0f, -std::sin(yaw)), Vector3::Up,
                        Vector3(std::sin(yaw), 0.0f, std::cos(yaw))};
        }

        void AppendDebris(std::vector<VertexPositionColorTexture>& vertices,
                          const ShipwreckDebrisPlacement& debris, const std::size_t index)
        {
            Axes axes = HorizontalAxes(debris.yawRadians);
            const float tilt = index % 3U == 0U ? 0.075f : -0.035f;
            const float pitch = (static_cast<float>((index * 7U) % 5U) - 2.0f) * 0.014f;
            axes.up = Vector3::Normalize(Vector3::Up + axes.right * tilt
                                         + axes.forward * pitch);
            axes.forward = Vector3::Normalize(Vector3::Cross(axes.right, axes.up));
            axes.right = Vector3::Normalize(Vector3::Cross(axes.up, axes.forward));
            // Layout stores the sampled surface plus a 55 mm placement guard. Planks
            // deliberately consume that guard and a further 5 mm so their lower edge
            // is sand-buried rather than hovering above its detached contact shadow.
            const float lift = debris.kind == ShipwreckDebrisKind::Spar ? 0.105f : -0.005f;
            const Vector3 centre(debris.x, debris.y + lift, debris.z);
            if (debris.kind == ShipwreckDebrisKind::Plank) {
                const Color plankColor=index % 2U == 0U ? OuterWood : InnerWood;
                if (debris.length >= 3.0f) {
                    // The five longest beach boards are storm-broken pairs, not
                    // pristine full-length lumber. Unequal spans, a real sand gap and
                    // a small lateral slip preserve the authored trail while breaking
                    // the strongest repeated straight-line silhouettes.
                    const float side=index % 2U == 0U ? 1.0f : -1.0f;
                    AppendBrokenPlank(vertices,
                        centre - axes.forward * (debris.length * 0.27f),axes,
                        debris.width * 0.48f,0.065f,debris.length * 0.21f,
                        plankColor,index * 2U,0.040f);
                    AppendBrokenPlank(vertices,
                        centre + axes.forward * (debris.length * 0.25f)
                            + axes.right * (side * debris.width * 0.11f),axes,
                        debris.width * 0.43f,0.060f,debris.length * 0.19f,
                        index % 2U == 0U ? InnerWood : OuterWood,index * 2U + 1U,0.035f);
                } else {
                    AppendBrokenPlank(vertices, centre, axes, debris.width * 0.5f, 0.065f,
                                      debris.length * 0.5f,plankColor,index,0.040f);
                }
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
                        index * 3U + static_cast<std::size_t>(plank + 1), 0.032f);
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
        const auto woodLevels = LoadOpaqueImageMips(
            device_, "Content/structures/shipwreck-weathered-wood.png", 256);
        const auto& mean = woodLevels.back().pixels[0];
        woodMean_ = Vector3(mean[0] / 255.0f, mean[1] / 255.0f, mean[2] / 255.0f);
        woodTexture_ = std::make_unique<Texture2D>(
            device_, 256, 256, true, SurfaceFormat::Color);
        for (std::size_t level = 0; level < woodLevels.size(); ++level) {
            const auto& pixels = woodLevels[level].pixels;
            woodTexture_->SetData(static_cast<int>(level), nullptr,
                                  reinterpret_cast<const std::uint8_t*>(pixels.data()),
                                  0, static_cast<int>(pixels.size() * 4U));
        }
        constexpr std::string_view WoodTextureDiagnostic =
            "island-simulator: shipwreck wood loaded Content/structures/"
            "shipwreck-weathered-wood.png; 256x256, 9 mips, 349524 RGBA8 bytes; "
            "mirrored interior atlas";
        std::cout << WoodTextureDiagnostic << '\n';
        MirrorRuntimeDiagnosticToPlatformLog(WoodTextureDiagnostic);
        std::vector<VertexPositionColorTexture> vertices;
        vertices.reserve(4800U);
        AppendShip(vertices, layout.Ship());
        for (std::size_t index = 0; index < layout.Debris().size(); ++index) {
            AppendDebris(vertices, layout.Debris()[index], index);
        }

        vertexCount_ = static_cast<int>(vertices.size());
        primitiveCount_ = vertexCount_ / 3;
        debrisCount_ = static_cast<int>(layout.Debris().size());
        vertices_ = std::make_unique<VertexBuffer>(
            device_, VertexPositionColorTexture::getVertexDeclarationStatic(), vertexCount_,
            BufferUsage::WriteOnly);
        vertices_->SetData(vertices.data(), vertexCount_);

        Vector3 minimum(std::numeric_limits<float>::max());
        Vector3 maximum(std::numeric_limits<float>::lowest());
        for (const VertexPositionColorTexture& vertex : vertices) {
            minimum = Vector3::Min(minimum, vertex.Position);
            maximum = Vector3::Max(maximum, vertex.Position);
        }
        bounds_ = BoundingBox(minimum, maximum);

        fallbackEffect_->setVertexColorEnabledProperty(true);
        fallbackEffect_->setLightingEnabledProperty(false);
        fallbackEffect_->setFogEnabledProperty(true);
        fallbackEffect_->setFogColorProperty(Vector3(environment::FogColorRed, environment::FogColorGreen, environment::FogColorBlue));
        fallbackEffect_->setFogStartProperty(190.0f);
        fallbackEffect_->setFogEndProperty(520.0f);

        if (device_.SupportsCapability(CNA::GraphicsCapability::CustomEffects)) {
            try {
                agedWoodEffect_ = std::make_unique<ShaderEffect>(device_, CreateShaderPackage());
                if (agedWoodEffect_->IsEffectValid()) {
                    agedWoodShaderReady_ = true;
                    std::cout << AgedWoodDiagnostic << '\n';
                    std::cout << BoardTextureDiagnostic << '\n';
                    std::cout << IronFittingDiagnostic << '\n';
                    MirrorRuntimeDiagnosticToPlatformLog(AgedWoodDiagnostic);
                    MirrorRuntimeDiagnosticToPlatformLog(BoardTextureDiagnostic);
                    MirrorRuntimeDiagnosticToPlatformLog(IronFittingDiagnostic);
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
        std::cout << HullShapeDiagnostic << '\n';
        std::cout << BroadsideDamageDiagnostic << '\n';
        std::cout << EndProfileDiagnostic << '\n';
        std::cout << PlankThicknessDiagnostic << '\n';
        std::cout << DeckhouseDamageDiagnostic << '\n';
        std::cout << SettledDebrisDiagnostic << '\n';
        std::cout << BrokenDeckDiagnostic << '\n';
        MirrorRuntimeDiagnosticToPlatformLog(diagnostic);
        MirrorRuntimeDiagnosticToPlatformLog(
            "island-simulator: shipwreck broken hull exposes ribs, partial deck, and snapped mast");
        MirrorRuntimeDiagnosticToPlatformLog(LargeShipDetailDiagnostic);
        MirrorRuntimeDiagnosticToPlatformLog(HullShapeDiagnostic);
        MirrorRuntimeDiagnosticToPlatformLog(BroadsideDamageDiagnostic);
        MirrorRuntimeDiagnosticToPlatformLog(EndProfileDiagnostic);
        MirrorRuntimeDiagnosticToPlatformLog(PlankThicknessDiagnostic);
        MirrorRuntimeDiagnosticToPlatformLog(DeckhouseDamageDiagnostic);
        MirrorRuntimeDiagnosticToPlatformLog(SettledDebrisDiagnostic);
        MirrorRuntimeDiagnosticToPlatformLog(BrokenDeckDiagnostic);
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
            device_.getSamplerStatesProperty()[0] = SamplerState::LinearClamp;
            agedWoodEffect_->SetTexture(0, *woodTexture_);
            agedWoodEffect_->SetUniformInt("uWoodTexture", 0);
            agedWoodEffect_->SetUniformVec3("uWoodMean", woodMean_.X,
                                            woodMean_.Y, woodMean_.Z);
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
