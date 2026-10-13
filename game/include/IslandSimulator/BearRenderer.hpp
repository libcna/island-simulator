#pragma once

#include "IslandSimulator/BearMotion.hpp"
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

/** A single articulated adult bear with a required project-owned directional-fur material. */
class BearRenderer final {
public:
    BearRenderer(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
        const CanopyLightField& canopy,float maximumDistance);
    ~BearRenderer();
    BearRenderer(const BearRenderer&)=delete;
    BearRenderer& operator=(const BearRenderer&)=delete;

    void DrawShadow(const Microsoft::Xna::Framework::Matrix& view,
        const Microsoft::Xna::Framework::Matrix& projection,
        const Microsoft::Xna::Framework::Vector3& camera,const BearState& bear);
    void Draw(const Microsoft::Xna::Framework::Matrix& view,
        const Microsoft::Xna::Framework::Matrix& projection,
        const Microsoft::Xna::Framework::Vector3& camera,const BearState& bear);

    [[nodiscard]] int VisibleCount()const noexcept{return visible_;}
    [[nodiscard]] int DrawCallCount()const noexcept{return draws_;}
    [[nodiscard]] int TriangleCount()const noexcept{return triangles_;}
    [[nodiscard]] int ShadowTriangleCount()const noexcept{return shadowTriangles_;}
    [[nodiscard]] bool UsesNearDetail()const noexcept{return nearDetail_;}
private:
    Microsoft::Xna::Framework::Graphics::GraphicsDevice& device_;
    const CanopyLightField& canopy_;
    float maximumDistance_;
    std::unique_ptr<Microsoft::Xna::Framework::Graphics::BasicEffect> effect_;
    std::unique_ptr<Microsoft::Xna::Framework::Graphics::DynamicVertexBuffer> buffer_;
    std::unique_ptr<Microsoft::Xna::Framework::Graphics::DynamicVertexBuffer> shadowBuffer_;
    std::unique_ptr<Microsoft::Xna::Framework::Graphics::Texture2D> furTexture_;
    std::vector<Microsoft::Xna::Framework::Graphics::VertexPositionColorTexture> vertices_,shadows_;
    int visible_=0,draws_=0,triangles_=0,shadowTriangles_=0;
    bool nearDetail_=false;
};
}
