#pragma once
#include "IslandSimulator/GoatHerd.hpp"
#include "Microsoft/Xna/Framework/Matrix.hpp"
#include "Microsoft/Xna/Framework/Vector3.hpp"
#include <memory>
#include <vector>
namespace Microsoft::Xna::Framework::Graphics {
class GraphicsDevice; class BasicEffect; class DynamicVertexBuffer; struct VertexPositionColor;
}
namespace island {
class CanopyLightField;
class GoatRenderer final {
public:
    GoatRenderer(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
        const IslandTerrain& terrain,const CanopyLightField& canopy,float maximumDistance);
    ~GoatRenderer();
    void Draw(const Microsoft::Xna::Framework::Matrix& view,
        const Microsoft::Xna::Framework::Matrix& projection,
        const Microsoft::Xna::Framework::Vector3& camera,std::span<const GoatState> goats);
    [[nodiscard]] int VisibleCount() const noexcept {return visible_;}
    [[nodiscard]] int DrawCallCount() const noexcept {return draws_;}
private:
    Microsoft::Xna::Framework::Graphics::GraphicsDevice& device_;
    const IslandTerrain& terrain_;
    RenderedTerrainSurface surface_;
    const CanopyLightField& canopy_;
    float maximumDistance_;
    std::unique_ptr<Microsoft::Xna::Framework::Graphics::BasicEffect> effect_;
    std::unique_ptr<Microsoft::Xna::Framework::Graphics::DynamicVertexBuffer> buffer_;
    std::vector<Microsoft::Xna::Framework::Graphics::VertexPositionColor> vertices_;
    int visible_=0,draws_=0;
};
}
