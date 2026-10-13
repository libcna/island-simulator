#pragma once

#include "IslandSimulator/SpiderWebGeometry.hpp"
#include "IslandSimulator/SpiderWebLayout.hpp"
#include "IslandSimulator/RenderedTerrainSurface.hpp"
#include "Microsoft/Xna/Framework/Matrix.hpp"
#include "Microsoft/Xna/Framework/Vector3.hpp"
#include <memory>
#include <span>
#include <vector>

namespace Microsoft::Xna::Framework::Graphics {
class BasicEffect;
class DynamicVertexBuffer;
class GraphicsDevice;
struct VertexPositionColor;
}

namespace island {

// Six bounded canopy webs share one dynamic upload and one translucent draw.
// The CPU graph remains authoritative; only camera-facing silk ribbons are rebuilt.
class SpiderWebRenderer final {
public:
    SpiderWebRenderer(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
        const IslandTerrain& terrain,const JungleLayout& jungle,
        std::span<const SpiderWebSite> sites);
    ~SpiderWebRenderer();
    void Draw(const Microsoft::Xna::Framework::Matrix& view,
        const Microsoft::Xna::Framework::Matrix& projection,
        const Microsoft::Xna::Framework::Vector3& camera,float timeSeconds);
    [[nodiscard]] int VisibleCount() const noexcept{return visible_;}
    [[nodiscard]] int StrandCount() const noexcept{return strands_;}
    [[nodiscard]] int DrawCallCount() const noexcept{return draws_;}
    [[nodiscard]] std::size_t SiteCount() const noexcept{return sites_.size();}
private:
    Microsoft::Xna::Framework::Graphics::GraphicsDevice& device_;
    const JungleLayout& jungle_;
    RenderedTerrainSurface surface_;
    std::vector<SpiderWebSite> sites_;
    std::vector<SpiderWebGeometry> geometry_;
    std::unique_ptr<Microsoft::Xna::Framework::Graphics::BasicEffect> effect_;
    std::unique_ptr<Microsoft::Xna::Framework::Graphics::DynamicVertexBuffer> buffer_;
    std::vector<Microsoft::Xna::Framework::Graphics::VertexPositionColor> vertices_;
    int visible_=0,strands_=0,draws_=0;
};

}
