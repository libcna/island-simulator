#include "IslandSimulator/NaturalPropRenderer.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <exception>
#include <iostream>
#include <numbers>
#include <string>

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
#include "Microsoft/Xna/Framework/Graphics/DynamicVertexBuffer.hpp"
#include "Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp"
#include "Microsoft/Xna/Framework/Graphics/IndexBuffer.hpp"
#include "Microsoft/Xna/Framework/Graphics/IndexElementSize.hpp"
#include "Microsoft/Xna/Framework/Graphics/PrimitiveType.hpp"
#include "Microsoft/Xna/Framework/Graphics/RasterizerState.hpp"
#include "Microsoft/Xna/Framework/Graphics/ShaderEffect.hpp"
#include "Microsoft/Xna/Framework/Graphics/VertexBuffer.hpp"
#include "Microsoft/Xna/Framework/Graphics/VertexBufferBinding.hpp"
#include "Microsoft/Xna/Framework/Graphics/VertexDeclaration.hpp"
#include "Microsoft/Xna/Framework/Graphics/VertexElement.hpp"
#include "Microsoft/Xna/Framework/Graphics/VertexElementFormat.hpp"
#include "Microsoft/Xna/Framework/Graphics/VertexElementUsage.hpp"
#include "Microsoft/Xna/Framework/Graphics/VertexPositionColor.hpp"

using namespace Microsoft::Xna::Framework;
using namespace Microsoft::Xna::Framework::Graphics;

namespace island
{
    namespace
    {
        constexpr float RockNearDistance = 92.0f;
        constexpr float RockMaximumDistance = 285.0f;
        constexpr float LogNearDistance = 72.0f;
        constexpr float LogMaximumDistance = 175.0f;
        constexpr float DriftwoodNearDistance = 58.0f;
        constexpr float DriftwoodMaximumDistance = 145.0f;
        constexpr float BeachDetailMaximumDistance = 52.0f;
        constexpr float FallenFrondMaximumDistance = 76.0f;

        struct PropGeometry
        {
            std::vector<VertexPositionColor> vertices;
            std::vector<std::uint16_t> indices;
        };

        constexpr const char* VertexBody = R"(
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec4 aColor;
layout(location = 2) in vec4 aInstance0;
layout(location = 3) in vec4 aInstance1;
layout(location = 4) in vec4 aInstance2;
layout(location = 5) in vec4 aInstance3;

uniform mat4 uViewProjection;

out vec3 vColor;
out vec3 vWorld;

void main()
{
    mat4 instanceTransform = mat4(aInstance0, aInstance1, aInstance2, aInstance3);
    vec4 world = instanceTransform * vec4(aPosition, 1.0);
    vColor = aColor.rgb;
    vWorld = world.xyz;
    gl_Position = uViewProjection * world;
    gl_Position.z = 2.0 * gl_Position.z - gl_Position.w;
}
)";

        constexpr const char* FragmentBody = R"(
in vec3 vColor;
in vec3 vWorld;

uniform vec3 uCameraPosition;

out vec4 fragColor;

void main()
{
    float distanceToCamera = length(vWorld - uCameraPosition);
    float fog = smoothstep(235.0, 610.0, distanceToCamera);
    vec3 color = mix(vColor, vec3(0.53, 0.78, 0.88), fog);
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
                                  CNA::ShaderStageEXT::Vertex, "main", "props/props.es.vert.glsl",
                                  ShaderSource(true, VertexBody)),
                    ShaderCodeEXT(CNA::ShaderLanguageEXT::GlslEs,
                                  CNA::ShaderStageEXT::Fragment, "main", "props/props.es.frag.glsl",
                                  ShaderSource(true, FragmentBody)),
                    ShaderCodeEXT(CNA::ShaderLanguageEXT::GlslDesktop,
                                  CNA::ShaderStageEXT::Vertex, "main", "props/props.desktop.vert.glsl",
                                  ShaderSource(false, VertexBody)),
                    ShaderCodeEXT(CNA::ShaderLanguageEXT::GlslDesktop,
                                  CNA::ShaderStageEXT::Fragment, "main", "props/props.desktop.frag.glsl",
                                  ShaderSource(false, FragmentBody)),
                },
                {CNA::ShaderStageEXT::Vertex, CNA::ShaderStageEXT::Fragment});
        }

        [[nodiscard]] Color ShadeColor(const Color& color, const float factor)
        {
            const auto channel = [factor](const std::uint8_t value) {
                return static_cast<std::uint8_t>(std::lround(std::clamp(
                    static_cast<float>(value) * factor, 0.0f, 255.0f)));
            };
            return Color(channel(color.getRProperty()), channel(color.getGProperty()),
                         channel(color.getBProperty()), color.getAProperty());
        }

        void AppendContactDisk(PropGeometry& geometry, const int sides)
        {
            const auto centre = static_cast<std::uint16_t>(geometry.vertices.size());
            geometry.vertices.emplace_back(Vector3(0.0f, 0.055f, 0.0f),
                                           Color(38, 63, 37, 255));
            for (int side = 0; side < sides; ++side) {
                const float angle = 2.0f * std::numbers::pi_v<float>
                    * static_cast<float>(side) / static_cast<float>(sides);
                geometry.vertices.emplace_back(
                    Vector3(std::cos(angle) * 0.76f, 0.055f, std::sin(angle) * 0.76f),
                    Color(38, 63, 37, 255));
            }
            for (int side = 0; side < sides; ++side) {
                geometry.indices.insert(geometry.indices.end(),
                                        {centre,
                                         static_cast<std::uint16_t>(centre + 1U + side),
                                         static_cast<std::uint16_t>(
                                             centre + 1U + (side + 1) % sides)});
            }
        }

        [[nodiscard]] PropGeometry CreateRockGeometry(const int variant, const bool farMesh)
        {
            PropGeometry geometry;
            const int rings = farMesh ? 3 : 5;
            const int slices = farMesh ? 6 : 9;
            geometry.vertices.reserve(static_cast<std::size_t>((rings + 1) * slices + slices + 1));
            geometry.indices.reserve(static_cast<std::size_t>(rings * slices * 6 + slices * 3));
            AppendContactDisk(geometry, slices);
            const auto base = static_cast<std::uint16_t>(geometry.vertices.size());
            const Color rockBase = variant == 0 ? Color(104, 105, 92, 255)
                : (variant == 1 ? Color(118, 108, 87, 255)
                   : (variant == 2 ? Color(88, 104, 93, 255)
                                   : Color(112, 116, 105, 255)));
            for (int ring = 0; ring <= rings; ++ring) {
                const float t = static_cast<float>(ring) / static_cast<float>(rings);
                const float radialBase = std::sin(t * std::numbers::pi_v<float>);
                const float y = t * (1.03f + 0.05f * variant);
                for (int slice = 0; slice < slices; ++slice) {
                    const float angle = 2.0f * std::numbers::pi_v<float>
                        * static_cast<float>(slice) / static_cast<float>(slices);
                    const float roughness = 0.88f + 0.11f
                        * std::sin(angle * (2.0f + variant) + t * 7.0f + variant * 1.3f);
                    const float radial = radialBase * roughness;
                    const Vector3 normal(std::cos(angle) * radialBase,
                                         std::sin((t - 0.5f) * std::numbers::pi_v<float>),
                                         std::sin(angle) * radialBase);
                    const float light = 0.63f + 0.34f * std::max(
                        0.0f, normal.X * 0.54f + normal.Y * 0.77f - normal.Z * 0.18f);
                    geometry.vertices.emplace_back(
                        Vector3(std::cos(angle) * radial, y, std::sin(angle) * radial),
                        ShadeColor(rockBase, light));
                }
            }
            for (int ring = 0; ring < rings; ++ring) {
                for (int slice = 0; slice < slices; ++slice) {
                    const int next = (slice + 1) % slices;
                    const auto lower = static_cast<std::uint16_t>(base + ring * slices + slice);
                    const auto lowerNext = static_cast<std::uint16_t>(base + ring * slices + next);
                    const auto upper = static_cast<std::uint16_t>(base + (ring + 1) * slices + slice);
                    const auto upperNext = static_cast<std::uint16_t>(base + (ring + 1) * slices + next);
                    geometry.indices.insert(geometry.indices.end(),
                                            {lower, upper, lowerNext,
                                             lowerNext, upper, upperNext});
                }
            }
            return geometry;
        }

        [[nodiscard]] PropGeometry CreateLogGeometry(const int variant, const bool farMesh)
        {
            PropGeometry geometry;
            const int sections = farMesh ? 2 : 5;
            const int sides = farMesh ? 6 : 9;
            geometry.vertices.reserve(static_cast<std::size_t>((sections + 1) * sides + sides * 2 + 5));
            geometry.indices.reserve(static_cast<std::size_t>(sections * sides * 6 + sides * 6 + 12));
            const Color bark = variant == 0 ? Color(101, 69, 42, 255)
                : (variant == 1 ? Color(87, 67, 45, 255) : Color(112, 77, 44, 255));
            const auto sideBase = static_cast<std::uint16_t>(geometry.vertices.size());
            for (int section = 0; section <= sections; ++section) {
                const float t = static_cast<float>(section) / static_cast<float>(sections);
                const float x = t - 0.5f;
                const float bend = std::sin(t * std::numbers::pi_v<float>)
                    * (0.035f + 0.012f * variant);
                for (int side = 0; side < sides; ++side) {
                    const float angle = 2.0f * std::numbers::pi_v<float>
                        * static_cast<float>(side) / static_cast<float>(sides);
                    const float ring = 0.92f + 0.07f
                        * std::sin(angle * (2.0f + variant) + section * 1.7f);
                    const float light = 0.60f + 0.36f
                        * std::max(0.0f, std::sin(angle) * 0.78f - std::cos(angle) * 0.22f);
                    geometry.vertices.emplace_back(
                        Vector3(x, std::sin(angle) * ring + bend,
                                std::cos(angle) * ring),
                        ShadeColor(bark, light));
                }
            }
            for (int section = 0; section < sections; ++section) {
                for (int side = 0; side < sides; ++side) {
                    const int next = (side + 1) % sides;
                    const auto lower = static_cast<std::uint16_t>(sideBase + section * sides + side);
                    const auto lowerNext = static_cast<std::uint16_t>(sideBase + section * sides + next);
                    const auto upper = static_cast<std::uint16_t>(sideBase + (section + 1) * sides + side);
                    const auto upperNext = static_cast<std::uint16_t>(sideBase + (section + 1) * sides + next);
                    geometry.indices.insert(geometry.indices.end(),
                                            {lower, upper, lowerNext,
                                             lowerNext, upper, upperNext});
                }
            }

            const Color cut(151 + variant * 4, 112 + variant * 3, 66, 255);
            for (int end = 0; end < 2; ++end) {
                const float x = end == 0 ? -0.505f : 0.505f;
                const auto centre = static_cast<std::uint16_t>(geometry.vertices.size());
                geometry.vertices.emplace_back(Vector3(x, 0.0f, 0.0f), cut);
                for (int side = 0; side < sides; ++side) {
                    const float angle = 2.0f * std::numbers::pi_v<float>
                        * static_cast<float>(side) / static_cast<float>(sides);
                    geometry.vertices.emplace_back(
                        Vector3(x, std::sin(angle) * 0.86f, std::cos(angle) * 0.86f),
                        ShadeColor(cut, 0.82f + 0.16f * std::max(0.0f, std::sin(angle))));
                }
                for (int side = 0; side < sides; ++side) {
                    const auto first = static_cast<std::uint16_t>(centre + 1U + side);
                    const auto next = static_cast<std::uint16_t>(
                        centre + 1U + (side + 1) % sides);
                    if (end == 0) geometry.indices.insert(geometry.indices.end(), {centre, next, first});
                    else geometry.indices.insert(geometry.indices.end(), {centre, first, next});
                }
            }

            const auto mossBase = static_cast<std::uint16_t>(geometry.vertices.size());
            const Color moss(54, 94 + variant * 5, 43, 255);
            geometry.vertices.emplace_back(Vector3(-0.42f, 0.87f, -0.28f), moss);
            geometry.vertices.emplace_back(Vector3(0.42f, 0.87f, -0.28f), moss);
            geometry.vertices.emplace_back(Vector3(-0.42f, 0.87f, 0.28f), ShadeColor(moss, 1.12f));
            geometry.vertices.emplace_back(Vector3(0.42f, 0.87f, 0.28f), ShadeColor(moss, 1.12f));
            geometry.indices.insert(geometry.indices.end(),
                                    {mossBase, static_cast<std::uint16_t>(mossBase + 1U),
                                     static_cast<std::uint16_t>(mossBase + 2U),
                                     static_cast<std::uint16_t>(mossBase + 2U),
                                     static_cast<std::uint16_t>(mossBase + 1U),
                                     static_cast<std::uint16_t>(mossBase + 3U)});

            const auto contactBase = static_cast<std::uint16_t>(geometry.vertices.size());
            const Color contact(37, 57, 34, 255);
            geometry.vertices.emplace_back(Vector3(-0.47f, -0.76f, -0.72f), contact);
            geometry.vertices.emplace_back(Vector3(0.47f, -0.76f, -0.72f), contact);
            geometry.vertices.emplace_back(Vector3(-0.47f, -0.76f, 0.72f), contact);
            geometry.vertices.emplace_back(Vector3(0.47f, -0.76f, 0.72f), contact);
            geometry.indices.insert(geometry.indices.end(),
                                    {contactBase, static_cast<std::uint16_t>(contactBase + 2U),
                                     static_cast<std::uint16_t>(contactBase + 1U),
                                     static_cast<std::uint16_t>(contactBase + 1U),
                                     static_cast<std::uint16_t>(contactBase + 2U),
                                     static_cast<std::uint16_t>(contactBase + 3U)});
            return geometry;
        }

        [[nodiscard]] PropGeometry CreateDriftwoodGeometry(const int variant,
                                                           const bool farMesh)
        {
            PropGeometry geometry;
            const int sections = farMesh ? 2 : 4;
            const int sides = farMesh ? 5 : 7;
            const Color wood = variant == 0 ? Color(157, 137, 103, 255)
                : (variant == 1 ? Color(171, 148, 109, 255)
                                : Color(142, 126, 98, 255));
            const auto sideBase = static_cast<std::uint16_t>(geometry.vertices.size());
            for (int section = 0; section <= sections; ++section) {
                const float t = static_cast<float>(section) / static_cast<float>(sections);
                const float x = t - 0.5f;
                const float bendY = std::sin(t * std::numbers::pi_v<float>)
                    * (0.08f + 0.025f * static_cast<float>(variant));
                const float bendZ = std::sin(t * 2.0f * std::numbers::pi_v<float>
                                             + static_cast<float>(variant)) * 0.055f;
                for (int side = 0; side < sides; ++side) {
                    const float angle = 2.0f * std::numbers::pi_v<float>
                        * static_cast<float>(side) / static_cast<float>(sides);
                    const float ring = 0.82f + 0.12f
                        * std::sin(angle * (2.0f + static_cast<float>(variant))
                                   + static_cast<float>(section) * 1.9f);
                    const float light = 0.72f + 0.27f
                        * std::max(0.0f, std::sin(angle) * 0.86f
                                            - std::cos(angle) * 0.18f);
                    geometry.vertices.emplace_back(
                        Vector3(x, std::sin(angle) * ring + bendY,
                                std::cos(angle) * ring + bendZ),
                        ShadeColor(wood, light));
                }
            }
            for (int section = 0; section < sections; ++section) {
                for (int side = 0; side < sides; ++side) {
                    const int next = (side + 1) % sides;
                    const auto lower = static_cast<std::uint16_t>(
                        sideBase + section * sides + side);
                    const auto lowerNext = static_cast<std::uint16_t>(
                        sideBase + section * sides + next);
                    const auto upper = static_cast<std::uint16_t>(
                        sideBase + (section + 1) * sides + side);
                    const auto upperNext = static_cast<std::uint16_t>(
                        sideBase + (section + 1) * sides + next);
                    geometry.indices.insert(geometry.indices.end(),
                                            {lower, upper, lowerNext,
                                             lowerNext, upper, upperNext});
                }
            }

            const Color endColor = ShadeColor(wood, 1.08f);
            for (int end = 0; end < 2; ++end) {
                const float x = end == 0 ? -0.505f : 0.505f;
                const auto centre = static_cast<std::uint16_t>(geometry.vertices.size());
                geometry.vertices.emplace_back(Vector3(x, 0.0f, 0.0f), endColor);
                for (int side = 0; side < sides; ++side) {
                    const float angle = 2.0f * std::numbers::pi_v<float>
                        * static_cast<float>(side) / static_cast<float>(sides);
                    geometry.vertices.emplace_back(
                        Vector3(x, std::sin(angle) * 0.76f,
                                std::cos(angle) * 0.76f), endColor);
                }
                for (int side = 0; side < sides; ++side) {
                    const auto first = static_cast<std::uint16_t>(centre + 1U + side);
                    const auto next = static_cast<std::uint16_t>(
                        centre + 1U + (side + 1) % sides);
                    if (end == 0) {
                        geometry.indices.insert(geometry.indices.end(), {centre, next, first});
                    } else {
                        geometry.indices.insert(geometry.indices.end(), {centre, first, next});
                    }
                }
            }

            if (!farMesh) {
                const int branchCount = 1 + variant % 2;
                for (int branch = 0; branch < branchCount; ++branch) {
                    const float branchX = branch == 0
                        ? -0.18f + 0.04f * static_cast<float>(variant)
                        : 0.21f;
                    const float branchSide = branch == 0 ? 1.0f : -1.0f;
                    const auto base = static_cast<std::uint16_t>(geometry.vertices.size());
                    const Color branchColor = ShadeColor(wood, 0.88f);
                    geometry.vertices.emplace_back(
                        Vector3(branchX - 0.035f, 0.42f, branchSide * 0.38f), branchColor);
                    geometry.vertices.emplace_back(
                        Vector3(branchX + 0.035f, 0.42f, branchSide * 0.38f), branchColor);
                    geometry.vertices.emplace_back(
                        Vector3(branchX + 0.035f, 0.72f, branchSide * 0.58f), branchColor);
                    geometry.vertices.emplace_back(
                        Vector3(branchX - 0.035f, 0.72f, branchSide * 0.58f), branchColor);
                    geometry.vertices.emplace_back(
                        Vector3(branchX + (branch == 0 ? 0.10f : -0.11f),
                                1.85f + 0.18f * static_cast<float>(variant),
                                branchSide * 1.45f),
                        ShadeColor(wood, 1.02f));
                    const auto tip = static_cast<std::uint16_t>(base + 4U);
                    geometry.indices.insert(
                        geometry.indices.end(),
                        {base, static_cast<std::uint16_t>(base + 1U), tip,
                         static_cast<std::uint16_t>(base + 1U),
                         static_cast<std::uint16_t>(base + 2U), tip,
                         static_cast<std::uint16_t>(base + 2U),
                         static_cast<std::uint16_t>(base + 3U), tip,
                         static_cast<std::uint16_t>(base + 3U), base, tip});
                }
            }
            return geometry;
        }

        void AppendBeachPebble(PropGeometry& geometry, const float centreX,
                               const float centreZ, const float radiusX,
                               const float radiusZ, const float height,
                               const float rotation, const Color& baseColor,
                               const bool scalloped)
        {
            constexpr int sides = 7;
            const Color contact = ShadeColor(baseColor, 0.48f);
            const auto contactCentre = static_cast<std::uint16_t>(geometry.vertices.size());
            geometry.vertices.emplace_back(Vector3(centreX, 0.006f, centreZ), contact);
            for (int side = 0; side < sides; ++side) {
                const float angle = rotation + 2.0f * std::numbers::pi_v<float>
                    * static_cast<float>(side) / static_cast<float>(sides);
                geometry.vertices.emplace_back(
                    Vector3(centreX + std::cos(angle) * radiusX * 1.12f,
                            0.006f,
                            centreZ + std::sin(angle) * radiusZ * 1.12f),
                    contact);
            }
            for (int side = 0; side < sides; ++side) {
                geometry.indices.insert(
                    geometry.indices.end(),
                    {contactCentre,
                     static_cast<std::uint16_t>(contactCentre + 1U + side),
                     static_cast<std::uint16_t>(contactCentre + 1U + (side + 1) % sides)});
            }

            const auto top = static_cast<std::uint16_t>(geometry.vertices.size());
            geometry.vertices.emplace_back(
                Vector3(centreX - std::sin(rotation) * radiusZ * 0.12f,
                        height, centreZ + std::cos(rotation) * radiusX * 0.12f),
                ShadeColor(baseColor, 1.16f));
            for (int side = 0; side < sides; ++side) {
                const float angle = rotation + 2.0f * std::numbers::pi_v<float>
                    * static_cast<float>(side) / static_cast<float>(sides);
                const float ridge = scalloped && side % 2 == 0 ? 1.0f : 0.82f;
                const float light = 0.70f + 0.26f
                    * std::max(0.0f, std::sin(angle) * 0.82f - std::cos(angle) * 0.18f);
                geometry.vertices.emplace_back(
                    Vector3(centreX + std::cos(angle) * radiusX * ridge,
                            0.012f,
                            centreZ + std::sin(angle) * radiusZ * ridge),
                    ShadeColor(baseColor, light));
            }
            for (int side = 0; side < sides; ++side) {
                geometry.indices.insert(
                    geometry.indices.end(),
                    {top,
                     static_cast<std::uint16_t>(top + 1U + side),
                     static_cast<std::uint16_t>(top + 1U + (side + 1) % sides)});
            }
        }

        [[nodiscard]] PropGeometry CreateBeachDetailGeometry(const int variant)
        {
            PropGeometry geometry;
            geometry.vertices.reserve(64U);
            geometry.indices.reserve(168U);
            const float phase = static_cast<float>(variant) * 0.73f;
            const std::array<Color, 4> colors = {
                Color(216, 195, 153, 255),
                Color(190, 132, 103, 255),
                Color(146, 145, 126, 255),
                Color(229, 214, 178, 255),
            };
            for (int item = 0; item < 4; ++item) {
                const float angle = phase + static_cast<float>(item) * 1.83f;
                const float distance = item == 0 ? 0.08f
                    : 0.19f + 0.055f * static_cast<float>((item + variant) % 3);
                const float size = 0.075f
                    + 0.018f * static_cast<float>((item * 3 + variant) % 4);
                const bool shell = (item + variant) % 3 == 0;
                AppendBeachPebble(
                    geometry, std::cos(angle) * distance, std::sin(angle) * distance,
                    size * (shell ? 1.28f : 1.0f),
                    size * (shell ? 0.82f : 0.92f),
                    size * (shell ? 0.66f : 0.52f), angle * 0.61f,
                    colors[static_cast<std::size_t>((item + variant) % colors.size())], shell);
            }
            return geometry;
        }

        [[nodiscard]] PropGeometry CreateFallenFrondGeometry(const int variant)
        {
            PropGeometry geometry;
            geometry.vertices.reserve(136U);
            geometry.indices.reserve(228U);
            const Color rachis = variant == 2 ? Color(105, 76, 39, 255)
                : Color(102, 112, 48, 255);
            const Color leaf = variant == 0 ? Color(52, 113, 55, 255)
                : (variant == 1 ? Color(91, 112, 49, 255)
                                : Color(133, 100, 48, 255));
            constexpr int stemSections = 9;
            const auto stemBase = static_cast<std::uint16_t>(geometry.vertices.size());
            for (int section = 0; section <= stemSections; ++section) {
                const float t = static_cast<float>(section)
                    / static_cast<float>(stemSections);
                const float x = t - 0.5f;
                const float curve = std::sin(t * std::numbers::pi_v<float>) * 0.032f;
                const float sideways = std::sin(
                    t * std::numbers::pi_v<float> * 2.0f + variant * 0.7f) * 0.012f;
                const float halfWidth = 0.014f - t * 0.006f;
                geometry.vertices.emplace_back(
                    Vector3(x, 0.018f + curve, sideways - halfWidth),
                    ShadeColor(rachis, 0.82f + t * 0.18f));
                geometry.vertices.emplace_back(
                    Vector3(x, 0.018f + curve, sideways + halfWidth),
                    ShadeColor(rachis, 1.05f + t * 0.10f));
            }
            for (int section = 0; section < stemSections; ++section) {
                const auto left = static_cast<std::uint16_t>(stemBase + section * 2);
                const auto right = static_cast<std::uint16_t>(left + 1U);
                const auto nextLeft = static_cast<std::uint16_t>(left + 2U);
                const auto nextRight = static_cast<std::uint16_t>(left + 3U);
                geometry.indices.insert(geometry.indices.end(),
                                        {left, nextLeft, right,
                                         right, nextLeft, nextRight});
            }

            constexpr int leafletPairs = 14;
            for (int pair = 0; pair < leafletPairs; ++pair) {
                const float t = 0.055f + static_cast<float>(pair)
                    / static_cast<float>(leafletPairs - 1) * 0.855f;
                const float x = t - 0.5f;
                const float envelope = 0.055f
                    + std::sin(t * std::numbers::pi_v<float>) * 0.155f;
                const float centreHeight = 0.020f
                    + std::sin(t * std::numbers::pi_v<float>) * 0.032f;
                for (int sideIndex = 0; sideIndex < 2; ++sideIndex) {
                    const float side = sideIndex == 0 ? -1.0f : 1.0f;
                    const float alternate = ((pair + sideIndex + variant) % 2 == 0)
                        ? 0.018f : -0.012f;
                    const float sweep = 0.055f + 0.010f * static_cast<float>(variant);
                    const auto base = static_cast<std::uint16_t>(geometry.vertices.size());
                    const Color inner = ShadeColor(leaf, 0.80f + 0.020f * pair);
                    const Color outer = ShadeColor(leaf, 0.92f + 0.014f * pair);
                    geometry.vertices.emplace_back(
                        Vector3(x, centreHeight, side * 0.010f), inner);
                    geometry.vertices.emplace_back(
                        Vector3(x - 0.028f, centreHeight + 0.002f,
                                side * envelope * 0.23f),
                        ShadeColor(inner, 0.92f));
                    geometry.vertices.emplace_back(
                        Vector3(x + sweep + alternate, centreHeight + 0.007f,
                                side * envelope), ShadeColor(outer, 1.05f));
                    geometry.vertices.emplace_back(
                        Vector3(x + 0.040f, centreHeight + 0.003f,
                                side * envelope * 0.24f), outer);
                    geometry.indices.insert(
                        geometry.indices.end(),
                        {base, static_cast<std::uint16_t>(base + 1U),
                         static_cast<std::uint16_t>(base + 2U),
                         base, static_cast<std::uint16_t>(base + 2U),
                         static_cast<std::uint16_t>(base + 3U)});
                }
            }
            return geometry;
        }

        [[nodiscard]] std::size_t CellIndex(const float x, const float z) noexcept
        {
            constexpr float cellWidth = 2.0f * TerrainMeshLayout::Extent
                / static_cast<float>(TerrainMeshLayout::CellsPerAxis);
            const int column = std::clamp(static_cast<int>(
                std::floor((x + TerrainMeshLayout::Extent) / cellWidth)),
                0, TerrainMeshLayout::CellsPerAxis - 1);
            const int row = std::clamp(static_cast<int>(
                std::floor((z + TerrainMeshLayout::Extent) / cellWidth)),
                0, TerrainMeshLayout::CellsPerAxis - 1);
            return static_cast<std::size_t>(row * TerrainMeshLayout::CellsPerAxis + column);
        }
    }

    const VertexDeclaration& NaturalPropRenderer::InstanceDeclaration()
    {
        static const VertexDeclaration declaration{
            VertexElement(0, VertexElementFormat::Vector4,
                          VertexElementUsage::TextureCoordinate, 1),
            VertexElement(16, VertexElementFormat::Vector4,
                          VertexElementUsage::TextureCoordinate, 2),
            VertexElement(32, VertexElementFormat::Vector4,
                          VertexElementUsage::TextureCoordinate, 3),
            VertexElement(48, VertexElementFormat::Vector4,
                          VertexElementUsage::TextureCoordinate, 4)};
        return declaration;
    }

    NaturalPropRenderer::NaturalPropRenderer(GraphicsDevice& device,
                                             const NaturalPropLayout& layout)
        : device_(device)
    {
        instancingSupported_ = device_.SupportsCapability(CNA::GraphicsCapability::Instancing);
        fallbackEffect_ = std::make_unique<BasicEffect>(device_);
        fallbackEffect_->setVertexColorEnabledProperty(true);
        fallbackEffect_->setLightingEnabledProperty(false);
        fallbackEffect_->setFogEnabledProperty(true);
        fallbackEffect_->setFogColorProperty(Vector3(0.53f, 0.78f, 0.88f));
        fallbackEffect_->setFogStartProperty(235.0f);
        fallbackEffect_->setFogEndProperty(610.0f);

        if (!instancingSupported_) {
            fallbackReason_ = "the active renderer does not support hardware instancing";
        } else if (!device_.SupportsCapability(CNA::GraphicsCapability::CustomEffects)) {
            fallbackReason_ = "the active renderer does not support custom effects";
        } else {
            try {
                customEffect_ = std::make_unique<ShaderEffect>(device_, CreateShaderPackage());
                if (customEffect_->IsEffectValid()) {
                    customShaderReady_ = true;
                } else {
                    fallbackReason_ = "the natural-prop shader did not compile: "
                        + customEffect_->GetCompileErrorEXT();
                    customEffect_.reset();
                }
            } catch (const std::exception& exception) {
                fallbackReason_ = std::string("the portable natural-prop shader could not be created: ")
                    + exception.what();
                customEffect_.reset();
            }
        }

        CreateMeshes();
        CreateCells(layout);
        std::cout << "island-simulator: natural props " << totalRockCount_ << " rocks, "
                  << totalLogCount_ << " fallen logs, " << totalDriftwoodCount_
                  << " driftwood pieces, " << totalFallenFrondCount_
                  << " fallen palm fronds, "
                  << (instancingSupported_ ? "hardware instancing" : "per-prop fallback") << '\n';
        std::cout << "island-simulator: beach detail " << totalBeachDetailCount_
                  << " shell/pebble clusters, near-field culled\n";
        if (customShaderReady_) {
            std::cout << "island-simulator: natural-prop portable shader with near/far LOD\n";
        } else {
            std::cerr << "island-simulator: natural-prop stock-effect fallback: "
                      << fallbackReason_ << '\n';
        }
    }

    NaturalPropRenderer::~NaturalPropRenderer() = default;

    void NaturalPropRenderer::CreateMeshes()
    {
        const auto upload = [this](const PropGeometry& geometry, PropMesh& mesh) {
            mesh.vertexCount = static_cast<int>(geometry.vertices.size());
            mesh.primitiveCount = static_cast<int>(geometry.indices.size() / 3U);
            mesh.vertices = std::make_unique<VertexBuffer>(
                device_, VertexPositionColor::getVertexDeclarationStatic(),
                mesh.vertexCount, BufferUsage::WriteOnly);
            mesh.vertices->SetData(geometry.vertices.data(), mesh.vertexCount);
            mesh.indices = std::make_unique<IndexBuffer>(
                device_, IndexElementSize::SixteenBits, static_cast<int>(geometry.indices.size()),
                BufferUsage::WriteOnly);
            mesh.indices->SetData(geometry.indices.data(), static_cast<int>(geometry.indices.size()));
        };
        for (std::size_t variant = 0; variant < rockNearMeshes_.size(); ++variant) {
            upload(CreateRockGeometry(static_cast<int>(variant), false), rockNearMeshes_[variant]);
            upload(CreateRockGeometry(static_cast<int>(variant), true), rockFarMeshes_[variant]);
        }
        for (std::size_t variant = 0; variant < logNearMeshes_.size(); ++variant) {
            upload(CreateLogGeometry(static_cast<int>(variant), false), logNearMeshes_[variant]);
            upload(CreateLogGeometry(static_cast<int>(variant), true), logFarMeshes_[variant]);
        }
        for (std::size_t variant = 0; variant < driftwoodNearMeshes_.size(); ++variant) {
            upload(CreateDriftwoodGeometry(static_cast<int>(variant), false),
                   driftwoodNearMeshes_[variant]);
            upload(CreateDriftwoodGeometry(static_cast<int>(variant), true),
                   driftwoodFarMeshes_[variant]);
        }
        for (std::size_t variant = 0; variant < beachDetailMeshes_.size(); ++variant) {
            upload(CreateBeachDetailGeometry(static_cast<int>(variant)),
                   beachDetailMeshes_[variant]);
        }
        for (std::size_t variant = 0; variant < fallenFrondMeshes_.size(); ++variant) {
            upload(CreateFallenFrondGeometry(static_cast<int>(variant)),
                   fallenFrondMeshes_[variant]);
        }
    }

    void NaturalPropRenderer::CreateCells(const NaturalPropLayout& layout)
    {
        const auto extendBounds = [](PropCell& cell, const Vector3& minimum,
                                     const Vector3& maximum) {
            if (!cell.occupied) {
                cell.bounds = BoundingBox(minimum, maximum);
                cell.occupied = true;
            } else {
                cell.bounds = BoundingBox(Vector3::Min(cell.bounds.Min, minimum),
                                          Vector3::Max(cell.bounds.Max, maximum));
            }
        };
        for (const RockPlacement& rock : layout.Rocks()) {
            PropCell& cell = cells_[CellIndex(rock.x, rock.z)];
            cell.rocks.push_back({Matrix::CreateScale(rock.scaleX, rock.scaleY, rock.scaleZ)
                                      * Matrix::CreateRotationY(rock.yawRadians)
                                      * Matrix::CreateTranslation(rock.x, rock.y, rock.z),
                                  rock.variant});
            const float radius = std::max(rock.scaleX, rock.scaleZ);
            extendBounds(cell, Vector3(rock.x - radius, rock.y - 0.1f, rock.z - radius),
                         Vector3(rock.x + radius, rock.y + rock.scaleY * 1.2f, rock.z + radius));
            ++totalRockCount_;
        }
        for (const FallenLogPlacement& log : layout.FallenLogs()) {
            PropCell& cell = cells_[CellIndex(log.x, log.z)];
            cell.logs.push_back({Matrix::CreateScale(log.length, log.radius, log.radius)
                                     * Matrix::CreateRotationY(log.yawRadians)
                                     * Matrix::CreateTranslation(log.x, log.y, log.z),
                                 log.variant});
            const float boundRadius = log.length * 0.55f;
            extendBounds(cell, Vector3(log.x - boundRadius, log.y - log.radius, log.z - boundRadius),
                         Vector3(log.x + boundRadius, log.y + log.radius, log.z + boundRadius));
            ++totalLogCount_;
        }
        for (const DriftwoodPlacement& wood : layout.Driftwood()) {
            PropCell& cell = cells_[CellIndex(wood.x, wood.z)];
            cell.driftwood.push_back({Matrix::CreateScale(wood.length, wood.radius, wood.radius)
                                          * Matrix::CreateRotationY(wood.yawRadians)
                                          * Matrix::CreateTranslation(wood.x, wood.y, wood.z),
                                      wood.variant});
            const float boundRadius = wood.length * 0.56f;
            extendBounds(cell,
                         Vector3(wood.x - boundRadius, wood.y - wood.radius,
                                 wood.z - boundRadius),
                         Vector3(wood.x + boundRadius, wood.y + wood.radius * 2.2f,
                                 wood.z + boundRadius));
            ++totalDriftwoodCount_;
        }
        for (const BeachDetailPlacement& detail : layout.BeachDetails()) {
            PropCell& cell = cells_[CellIndex(detail.x, detail.z)];
            cell.beachDetails.push_back({
                Matrix::CreateScale(detail.scale)
                    * Matrix::CreateRotationY(detail.yawRadians)
                    * Matrix::CreateTranslation(detail.x, detail.y, detail.z),
                detail.variant, detail.x, detail.z});
            const float radius = detail.scale * 0.58f;
            extendBounds(cell,
                         Vector3(detail.x - radius, detail.y - 0.02f,
                                 detail.z - radius),
                         Vector3(detail.x + radius, detail.y + detail.scale * 0.13f,
                                 detail.z + radius));
            ++totalBeachDetailCount_;
        }
        for (const FallenFrondPlacement& frond : layout.FallenFronds()) {
            PropCell& cell = cells_[CellIndex(frond.x, frond.z)];
            cell.fallenFronds.push_back({
                Matrix::CreateScale(frond.scale)
                    * Matrix::CreateRotationY(frond.yawRadians)
                    * Matrix::CreateTranslation(frond.x, frond.y, frond.z),
                frond.variant, frond.x, frond.z});
            const float radius = frond.scale * 0.58f;
            extendBounds(cell,
                         Vector3(frond.x - radius, frond.y - 0.02f,
                                 frond.z - radius),
                         Vector3(frond.x + radius, frond.y + frond.scale * 0.12f,
                                 frond.z + radius));
            ++totalFallenFrondCount_;
        }
        for (auto& transforms : visibleRockNear_) transforms.reserve(layout.Rocks().size());
        for (auto& transforms : visibleRockFar_) transforms.reserve(layout.Rocks().size());
        for (auto& transforms : visibleLogNear_) transforms.reserve(layout.FallenLogs().size());
        for (auto& transforms : visibleLogFar_) transforms.reserve(layout.FallenLogs().size());
        for (auto& transforms : visibleDriftwoodNear_) {
            transforms.reserve(layout.Driftwood().size());
        }
        for (auto& transforms : visibleDriftwoodFar_) {
            transforms.reserve(layout.Driftwood().size());
        }
        for (auto& transforms : visibleBeachDetails_) {
            transforms.reserve(layout.BeachDetails().size());
        }
        for (auto& transforms : visibleFallenFronds_) {
            transforms.reserve(layout.FallenFronds().size());
        }
    }

    void NaturalPropRenderer::DrawInstances(PropMesh& mesh,
                                            const std::vector<Matrix>& transforms)
    {
        if (transforms.empty()) return;
        device_.setIndicesProperty(mesh.indices.get());
        if (instancingSupported_) {
            const int count = static_cast<int>(transforms.size());
            if (mesh.instances == nullptr || count > mesh.instanceCapacity) {
                int capacity = std::max(16, mesh.instanceCapacity);
                while (capacity < count) capacity *= 2;
                mesh.instances = std::make_unique<DynamicVertexBuffer>(
                    device_, InstanceDeclaration(), capacity, BufferUsage::WriteOnly);
                mesh.instanceCapacity = capacity;
            }
            mesh.instances->SetDataRaw(transforms.data(), count, static_cast<int>(sizeof(Matrix)));
            device_.SetVertexBuffers({VertexBufferBinding(mesh.vertices.get(), 0, 0),
                                      VertexBufferBinding(mesh.instances.get(), 0, 1)});
            if (!customShaderReady_) {
                fallbackEffect_->setWorldProperty(Matrix::getIdentityProperty());
                fallbackEffect_->Apply();
            }
            device_.DrawInstancedPrimitives(PrimitiveType::TriangleList, 0, 0,
                                            mesh.vertexCount, 0, mesh.primitiveCount, count);
            ++drawCallCount_;
            return;
        }
        device_.SetVertexBuffer(mesh.vertices.get());
        for (const Matrix& transform : transforms) {
            fallbackEffect_->setWorldProperty(transform);
            fallbackEffect_->Apply();
            device_.DrawIndexedPrimitives(PrimitiveType::TriangleList, 0, 0,
                                          mesh.vertexCount, 0, mesh.primitiveCount);
            ++drawCallCount_;
        }
    }

    void NaturalPropRenderer::Draw(const Matrix& view, const Matrix& projection,
                                   const Vector3& cameraPosition)
    {
        for (auto& transforms : visibleRockNear_) transforms.clear();
        for (auto& transforms : visibleRockFar_) transforms.clear();
        for (auto& transforms : visibleLogNear_) transforms.clear();
        for (auto& transforms : visibleLogFar_) transforms.clear();
        for (auto& transforms : visibleDriftwoodNear_) transforms.clear();
        for (auto& transforms : visibleDriftwoodFar_) transforms.clear();
        for (auto& transforms : visibleBeachDetails_) transforms.clear();
        for (auto& transforms : visibleFallenFronds_) transforms.clear();
        visibleRockCount_ = 0;
        visibleLogCount_ = 0;
        visibleDriftwoodCount_ = 0;
        visibleBeachDetailCount_ = 0;
        visibleFallenFrondCount_ = 0;
        visibleCellCount_ = 0;
        drawCallCount_ = 0;

        const BoundingFrustum frustum(view * projection);
        for (const PropCell& cell : cells_) {
            if (!cell.occupied || frustum.Contains(cell.bounds) == ContainmentType::Disjoint) {
                continue;
            }
            const Vector3 centre = (cell.bounds.Min + cell.bounds.Max) * 0.5f;
            const float deltaX = centre.X - cameraPosition.X;
            const float deltaZ = centre.Z - cameraPosition.Z;
            const float distanceSquared = deltaX * deltaX + deltaZ * deltaZ;
            bool cellVisible = false;
            if (distanceSquared <= RockMaximumDistance * RockMaximumDistance) {
                const bool nearLod = distanceSquared <= RockNearDistance * RockNearDistance;
                for (const DrawInstance& instance : cell.rocks) {
                    (nearLod ? visibleRockNear_ : visibleRockFar_)[instance.variant]
                        .push_back(instance.transform);
                    ++visibleRockCount_;
                    cellVisible = true;
                }
            }
            if (distanceSquared <= LogMaximumDistance * LogMaximumDistance) {
                const bool nearLod = distanceSquared <= LogNearDistance * LogNearDistance;
                for (const DrawInstance& instance : cell.logs) {
                    (nearLod ? visibleLogNear_ : visibleLogFar_)[instance.variant]
                        .push_back(instance.transform);
                    ++visibleLogCount_;
                    cellVisible = true;
                }
            }
            if (distanceSquared <= DriftwoodMaximumDistance * DriftwoodMaximumDistance) {
                const bool nearLod = distanceSquared
                    <= DriftwoodNearDistance * DriftwoodNearDistance;
                for (const DrawInstance& instance : cell.driftwood) {
                    (nearLod ? visibleDriftwoodNear_ : visibleDriftwoodFar_)[instance.variant]
                        .push_back(instance.transform);
                    ++visibleDriftwoodCount_;
                    cellVisible = true;
                }
            }
            for (const DrawInstance& instance : cell.beachDetails) {
                const float detailDeltaX = instance.x - cameraPosition.X;
                const float detailDeltaZ = instance.z - cameraPosition.Z;
                if (detailDeltaX * detailDeltaX + detailDeltaZ * detailDeltaZ
                    > BeachDetailMaximumDistance * BeachDetailMaximumDistance) {
                    continue;
                }
                visibleBeachDetails_[instance.variant].push_back(instance.transform);
                ++visibleBeachDetailCount_;
                cellVisible = true;
            }
            for (const DrawInstance& instance : cell.fallenFronds) {
                const float frondDeltaX = instance.x - cameraPosition.X;
                const float frondDeltaZ = instance.z - cameraPosition.Z;
                if (frondDeltaX * frondDeltaX + frondDeltaZ * frondDeltaZ
                    > FallenFrondMaximumDistance * FallenFrondMaximumDistance) {
                    continue;
                }
                visibleFallenFronds_[instance.variant].push_back(instance.transform);
                ++visibleFallenFrondCount_;
                cellVisible = true;
            }
            if (cellVisible) ++visibleCellCount_;
        }

        device_.setRasterizerStateProperty(RasterizerState::CullNone);
        device_.setBlendStateProperty(BlendState::Opaque);
        device_.setDepthStencilStateProperty(DepthStencilState::Default);
        if (customShaderReady_) {
            float viewProjection[16];
            (view * projection).ToColumnMajor(viewProjection);
            customEffect_->Apply();
            customEffect_->SetUniformMat4("uViewProjection", viewProjection);
            customEffect_->SetUniformVec3("uCameraPosition", cameraPosition.X,
                                          cameraPosition.Y, cameraPosition.Z);
        } else {
            fallbackEffect_->setViewProperty(view);
            fallbackEffect_->setProjectionProperty(projection);
        }

        for (std::size_t variant = 0; variant < rockNearMeshes_.size(); ++variant) {
            DrawInstances(rockNearMeshes_[variant], visibleRockNear_[variant]);
            DrawInstances(rockFarMeshes_[variant], visibleRockFar_[variant]);
        }
        for (std::size_t variant = 0; variant < logNearMeshes_.size(); ++variant) {
            DrawInstances(logNearMeshes_[variant], visibleLogNear_[variant]);
            DrawInstances(logFarMeshes_[variant], visibleLogFar_[variant]);
        }
        for (std::size_t variant = 0; variant < driftwoodNearMeshes_.size(); ++variant) {
            DrawInstances(driftwoodNearMeshes_[variant], visibleDriftwoodNear_[variant]);
            DrawInstances(driftwoodFarMeshes_[variant], visibleDriftwoodFar_[variant]);
        }
        for (std::size_t variant = 0; variant < beachDetailMeshes_.size(); ++variant) {
            DrawInstances(beachDetailMeshes_[variant], visibleBeachDetails_[variant]);
        }
        for (std::size_t variant = 0; variant < fallenFrondMeshes_.size(); ++variant) {
            DrawInstances(fallenFrondMeshes_[variant], visibleFallenFronds_[variant]);
        }
        device_.SetVertexBuffers({});
        device_.setIndicesProperty(nullptr);
        fallbackEffect_->setWorldProperty(Matrix::getIdentityProperty());
    }
}
