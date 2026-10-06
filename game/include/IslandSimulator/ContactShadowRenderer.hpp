#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <vector>

#include "Microsoft/Xna/Framework/Matrix.hpp"
#include "Microsoft/Xna/Framework/Vector3.hpp"

namespace Microsoft::Xna::Framework::Graphics
{
    class BasicEffect;
    class DynamicVertexBuffer;
    class GraphicsDevice;
    struct VertexPositionColor;
}

namespace island
{
    class IslandTerrain;
    class JungleLayout;
    class NaturalPropLayout;
    class VegetationLayout;
    struct MonkeyState;

    /**
     * @brief One-batch terrain-conforming grounding shadows for nearby scene objects.
     *
     * These intentionally compact contact shadows are not a replacement for full directional
     * shadow maps. They provide stable grounding on every required renderer without textures,
     * depth-map sampling, or renderer-specific shaders.
     */
    class ContactShadowRenderer final
    {
    public:
        ContactShadowRenderer(
            Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
            const IslandTerrain& terrain, const VegetationLayout& vegetation,
            const JungleLayout& jungle, const NaturalPropLayout& naturalProps);
        ~ContactShadowRenderer();

        ContactShadowRenderer(const ContactShadowRenderer&) = delete;
        ContactShadowRenderer& operator=(const ContactShadowRenderer&) = delete;
        ContactShadowRenderer(ContactShadowRenderer&&) = delete;
        ContactShadowRenderer& operator=(ContactShadowRenderer&&) = delete;

        void Draw(const Microsoft::Xna::Framework::Matrix& view,
                  const Microsoft::Xna::Framework::Matrix& projection,
                  const Microsoft::Xna::Framework::Vector3& cameraPosition,
                  std::span<const MonkeyState> monkeys);

        [[nodiscard]] int VisibleShadowCount() const noexcept { return visibleShadowCount_; }
        [[nodiscard]] int VisiblePalmShadowCount() const noexcept
        {
            return visiblePalmShadowCount_;
        }
        [[nodiscard]] int VisibleCanopyShadowCount() const noexcept
        {
            return visibleCanopyShadowCount_;
        }
        [[nodiscard]] int VisibleRockShadowCount() const noexcept
        {
            return visibleRockShadowCount_;
        }
        [[nodiscard]] int VisibleWoodShadowCount() const noexcept
        {
            return visibleWoodShadowCount_;
        }
        [[nodiscard]] int VisibleMonkeyShadowCount() const noexcept
        {
            return visibleMonkeyShadowCount_;
        }
        [[nodiscard]] int PrimitiveCount() const noexcept { return primitiveCount_; }
        [[nodiscard]] int DrawCallCount() const noexcept { return drawCallCount_; }

    private:
        enum class StaticShadowKind : std::uint8_t
        {
            Palm,
            Canopy,
            Rock,
            Wood
        };

        struct StaticShadow
        {
            Microsoft::Xna::Framework::Vector3 centre;
            float radius = 0.0f;
            float maximumDistance = 0.0f;
            std::size_t firstVertex = 0U;
            std::uint8_t centreAlpha = 0U;
            StaticShadowKind kind = StaticShadowKind::Palm;
        };

        Microsoft::Xna::Framework::Graphics::GraphicsDevice& device_;
        const IslandTerrain& terrain_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::BasicEffect> effect_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::DynamicVertexBuffer> vertices_;
        std::vector<StaticShadow> staticShadows_;
        std::vector<Microsoft::Xna::Framework::Graphics::VertexPositionColor> staticVertices_;
        std::vector<Microsoft::Xna::Framework::Graphics::VertexPositionColor> stagingVertices_;
        Microsoft::Xna::Framework::Matrix cachedView_;
        Microsoft::Xna::Framework::Matrix cachedProjection_;
        Microsoft::Xna::Framework::Vector3 cachedCameraPosition_;
        std::vector<Microsoft::Xna::Framework::Vector3> cachedMonkeyPositions_;
        bool cacheValid_ = false;
        int visibleShadowCount_ = 0;
        int visiblePalmShadowCount_ = 0;
        int visibleCanopyShadowCount_ = 0;
        int visibleRockShadowCount_ = 0;
        int visibleWoodShadowCount_ = 0;
        int visibleMonkeyShadowCount_ = 0;
        int primitiveCount_ = 0;
        int drawCallCount_ = 0;
    };
}
