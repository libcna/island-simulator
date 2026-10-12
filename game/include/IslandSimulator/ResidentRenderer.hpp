#pragma once

#include "IslandSimulator/ResidentMotion.hpp"
#include "Microsoft/Xna/Framework/Matrix.hpp"
#include "Microsoft/Xna/Framework/Vector3.hpp"

#include <memory>
#include <vector>

namespace Microsoft::Xna::Framework::Graphics {
class BasicEffect;
class DynamicVertexBuffer;
class GraphicsDevice;
class Texture2D;
struct VertexPositionColorTexture;
}

namespace island {
class CanopyLightField;

/** One terrain-lit adult resident built from an original procedural human mesh. */
class ResidentRenderer final {
public:
    ResidentRenderer(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
        const CanopyLightField& canopy,float maximumDistance);
    ~ResidentRenderer();
    ResidentRenderer(const ResidentRenderer&)=delete;
    ResidentRenderer& operator=(const ResidentRenderer&)=delete;

    void DrawShadow(const Microsoft::Xna::Framework::Matrix& view,
        const Microsoft::Xna::Framework::Matrix& projection,
        const Microsoft::Xna::Framework::Vector3& camera,const ResidentState& resident);
    void Draw(const Microsoft::Xna::Framework::Matrix& view,
        const Microsoft::Xna::Framework::Matrix& projection,
        const Microsoft::Xna::Framework::Vector3& camera,const ResidentState& resident);

    [[nodiscard]] int VisibleCount()const noexcept{return visible_;}
    [[nodiscard]] int DrawCallCount()const noexcept{return draws_;}
    [[nodiscard]] int TriangleCount()const noexcept{return triangles_;}
    [[nodiscard]] int ShadowTriangleCount()const noexcept{return shadowTriangles_;}
private:
    Microsoft::Xna::Framework::Graphics::GraphicsDevice& device_;
    const CanopyLightField& canopy_;
    float maximumDistance_;
    std::unique_ptr<Microsoft::Xna::Framework::Graphics::BasicEffect> effect_;
    std::unique_ptr<Microsoft::Xna::Framework::Graphics::DynamicVertexBuffer> buffer_;
    std::unique_ptr<Microsoft::Xna::Framework::Graphics::DynamicVertexBuffer> shadowBuffer_;
    std::unique_ptr<Microsoft::Xna::Framework::Graphics::Texture2D> materialAtlas_;
    std::vector<Microsoft::Xna::Framework::Graphics::VertexPositionColorTexture> vertices_;
    std::vector<Microsoft::Xna::Framework::Graphics::VertexPositionColorTexture> shadows_;
    int visible_=0,draws_=0,triangles_=0,shadowTriangles_=0;
};
}
