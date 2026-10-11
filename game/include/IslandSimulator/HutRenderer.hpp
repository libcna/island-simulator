#pragma once
#include "IslandSimulator/HutLayout.hpp"
#include "Microsoft/Xna/Framework/BoundingBox.hpp"
#include "Microsoft/Xna/Framework/Matrix.hpp"
#include "Microsoft/Xna/Framework/Vector3.hpp"
#include <memory>
namespace Microsoft::Xna::Framework::Graphics { class GraphicsDevice; class BasicEffect; class VertexBuffer; }
namespace island {
class HutRenderer final {
public:
    HutRenderer(Microsoft::Xna::Framework::Graphics::GraphicsDevice&,const HutPlacement&);
    ~HutRenderer();
    void Draw(const Microsoft::Xna::Framework::Matrix&,const Microsoft::Xna::Framework::Matrix&,
              const Microsoft::Xna::Framework::Vector3&);
    [[nodiscard]] int DrawCallCount() const noexcept { return drawCalls_; }
private:
    Microsoft::Xna::Framework::Graphics::GraphicsDevice& device_;
    std::unique_ptr<Microsoft::Xna::Framework::Graphics::BasicEffect> effect_;
    std::unique_ptr<Microsoft::Xna::Framework::Graphics::VertexBuffer> vertices_;
    Microsoft::Xna::Framework::BoundingBox bounds_;
    Microsoft::Xna::Framework::Vector3 centre_;
    int triangles_=0,drawCalls_=0;
};
}
