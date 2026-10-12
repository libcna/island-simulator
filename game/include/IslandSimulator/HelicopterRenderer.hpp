#pragma once

#include "IslandSimulator/HelicopterLayout.hpp"
#include "Microsoft/Xna/Framework/BoundingBox.hpp"
#include "Microsoft/Xna/Framework/Matrix.hpp"
#include "Microsoft/Xna/Framework/Vector3.hpp"

#include <memory>

namespace Microsoft::Xna::Framework::Graphics {
class BasicEffect;
class GraphicsDevice;
class VertexBuffer;
}

namespace island {
/** One-batch original procedural grounded utility helicopter, including accessible interior. */
class HelicopterRenderer final {
public:
    HelicopterRenderer(Microsoft::Xna::Framework::Graphics::GraphicsDevice&,
                       const HelicopterPlacement&);
    ~HelicopterRenderer();
    void Draw(const Microsoft::Xna::Framework::Matrix& view,
              const Microsoft::Xna::Framework::Matrix& projection,
              const Microsoft::Xna::Framework::Vector3& camera);
    [[nodiscard]] int DrawCallCount() const noexcept{return drawCalls_;}
    [[nodiscard]] int TriangleCount() const noexcept{return triangles_;}
private:
    Microsoft::Xna::Framework::Graphics::GraphicsDevice& device_;
    std::unique_ptr<Microsoft::Xna::Framework::Graphics::BasicEffect> effect_;
    std::unique_ptr<Microsoft::Xna::Framework::Graphics::VertexBuffer> vertices_;
    Microsoft::Xna::Framework::BoundingBox bounds_;
    Microsoft::Xna::Framework::Vector3 centre_;
    int triangles_=0,drawCalls_=0;
};
}
