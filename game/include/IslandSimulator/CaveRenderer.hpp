#pragma once
#include "IslandSimulator/CaveLayout.hpp"
#include "Microsoft/Xna/Framework/Matrix.hpp"
#include "Microsoft/Xna/Framework/BoundingBox.hpp"
#include <memory>
namespace Microsoft::Xna::Framework::Graphics {
class GraphicsDevice;class VertexBuffer;class ShaderEffect;
}
namespace island {
class CaveRenderer final {
public:
    CaveRenderer(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,const CaveLayout& cave);
    ~CaveRenderer();
    void Draw(const Microsoft::Xna::Framework::Matrix& view,const Microsoft::Xna::Framework::Matrix& projection);
    [[nodiscard]] int DrawCallCount() const noexcept {return draws_;}
private:
    Microsoft::Xna::Framework::Graphics::GraphicsDevice& device_;
    std::unique_ptr<Microsoft::Xna::Framework::Graphics::ShaderEffect> effect_;
    std::unique_ptr<Microsoft::Xna::Framework::Graphics::VertexBuffer> vertices_;
    Microsoft::Xna::Framework::BoundingBox bounds_;
    int count_=0,draws_=0;
};
}
