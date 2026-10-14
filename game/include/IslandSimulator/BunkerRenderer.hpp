#pragma once

#include "IslandSimulator/BunkerLayout.hpp"
#include "Microsoft/Xna/Framework/BoundingBox.hpp"
#include "Microsoft/Xna/Framework/Matrix.hpp"
#include "Microsoft/Xna/Framework/Vector3.hpp"

#include <memory>

namespace Microsoft::Xna::Framework::Graphics {
class BasicEffect;
class GraphicsDevice;
class ShaderEffect;
class VertexBuffer;
}

namespace island {

/** Bounded two-batch procedural shell for the underground bunker and its lit rooms. */
class BunkerRenderer final {
public:
    BunkerRenderer(Microsoft::Xna::Framework::Graphics::GraphicsDevice&,const BunkerLayout&);
    ~BunkerRenderer();
    void Draw(const Microsoft::Xna::Framework::Matrix&,const Microsoft::Xna::Framework::Matrix&,
              const Microsoft::Xna::Framework::Vector3& camera);
    [[nodiscard]] int DrawCallCount()const noexcept{return drawCalls_;}
    [[nodiscard]] int TriangleCount()const noexcept{return opaqueTriangles_+glowTriangles_;}
private:
    Microsoft::Xna::Framework::Graphics::GraphicsDevice& device_;
    std::unique_ptr<Microsoft::Xna::Framework::Graphics::BasicEffect> effect_;
    std::unique_ptr<Microsoft::Xna::Framework::Graphics::ShaderEffect> materialEffect_;
    std::unique_ptr<Microsoft::Xna::Framework::Graphics::VertexBuffer> opaque_,glow_;
    Microsoft::Xna::Framework::BoundingBox bounds_;
    Microsoft::Xna::Framework::Vector3 centre_;
    int opaqueTriangles_=0,glowTriangles_=0,drawCalls_=0;
};

}
