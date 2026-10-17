#pragma once
#include "IslandSimulator/CaveLayout.hpp"
#include "IslandSimulator/CaveGroundPatch.hpp"
#include "Microsoft/Xna/Framework/Matrix.hpp"
#include "Microsoft/Xna/Framework/BoundingBox.hpp"
#include <memory>
namespace Microsoft::Xna::Framework::Graphics {
class GraphicsDevice;class VertexBuffer;class ShaderEffect;class Texture2D;
}
namespace island {
class CaveRenderer final {
public:
    CaveRenderer(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                 const CaveLayout& cave,const CaveGroundPatch& ground);
    ~CaveRenderer();
    void Draw(const Microsoft::Xna::Framework::Matrix& view,const Microsoft::Xna::Framework::Matrix& projection);
    [[nodiscard]] int DrawCallCount() const noexcept {return draws_;}
private:
    Microsoft::Xna::Framework::Graphics::GraphicsDevice& device_;
    std::unique_ptr<Microsoft::Xna::Framework::Graphics::ShaderEffect> effect_;
    std::unique_ptr<Microsoft::Xna::Framework::Graphics::VertexBuffer> vertices_;
    std::unique_ptr<Microsoft::Xna::Framework::Graphics::Texture2D> rockTexture_;
    Microsoft::Xna::Framework::BoundingBox bounds_;
    float rockMean_[3]={1,1,1};
    int count_=0,draws_=0;
};
}
