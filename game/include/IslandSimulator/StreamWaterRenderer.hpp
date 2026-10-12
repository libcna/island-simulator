#pragma once
#include "IslandSimulator/StreamWaterMesh.hpp"
#include "Microsoft/Xna/Framework/Matrix.hpp"
#include "Microsoft/Xna/Framework/BoundingBox.hpp"
#include <memory>
namespace Microsoft::Xna::Framework::Graphics {
class GraphicsDevice;class VertexBuffer;class IndexBuffer;class ShaderEffect;
}
namespace island {
class StreamWaterRenderer final {
public:
    StreamWaterRenderer(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
        const IslandTerrain& terrain,const StreamBedPatch& bed);
    ~StreamWaterRenderer();
    void Draw(const Microsoft::Xna::Framework::Matrix& view,const Microsoft::Xna::Framework::Matrix& projection,
        const Microsoft::Xna::Framework::Vector3& camera,float seconds);
    [[nodiscard]] int DrawCallCount() const noexcept{return draws_;}
private:
    Microsoft::Xna::Framework::Graphics::GraphicsDevice& device_;
    std::unique_ptr<Microsoft::Xna::Framework::Graphics::ShaderEffect> effect_;
    std::unique_ptr<Microsoft::Xna::Framework::Graphics::VertexBuffer> vertices_;
    std::unique_ptr<Microsoft::Xna::Framework::Graphics::IndexBuffer> indices_;
    Microsoft::Xna::Framework::BoundingBox bounds_;
    int count_=0,triangles_=0,draws_=0;
};
}
