#pragma once
#include "IslandSimulator/CampfireLayout.hpp"
#include "Microsoft/Xna/Framework/Matrix.hpp"
#include "Microsoft/Xna/Framework/BoundingBox.hpp"
#include <memory>
namespace Microsoft::Xna::Framework::Graphics {class GraphicsDevice;class VertexBuffer;class ShaderEffect;}
namespace island {
class CampfireRenderer final {
public:
    CampfireRenderer(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,const CampfireLayout& layout);
    ~CampfireRenderer();
    void Draw(const Microsoft::Xna::Framework::Matrix& view,const Microsoft::Xna::Framework::Matrix& projection,
        const Microsoft::Xna::Framework::Vector3& camera,float seconds,bool effects);
    [[nodiscard]] int DrawCallCount() const noexcept {return draws_;}
    [[nodiscard]] int EffectTriangleCount() const noexcept {return effectCount_/3;}
private:
    Microsoft::Xna::Framework::Graphics::GraphicsDevice& device_;
    std::unique_ptr<Microsoft::Xna::Framework::Graphics::ShaderEffect> effect_;
    std::unique_ptr<Microsoft::Xna::Framework::Graphics::VertexBuffer> solid_,moving_;
    Microsoft::Xna::Framework::BoundingBox bounds_;
    FirePoint centre_;
    int solidCount_=0,effectCount_=0,draws_=0;
};
}
