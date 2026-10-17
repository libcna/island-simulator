#pragma once
#include "IslandSimulator/CaveLayout.hpp"
#include "IslandSimulator/RenderedTerrainSurface.hpp"
#include <cstdint>

namespace island {
// Grid-aligned replacement, not an overlay. Remove coarse terrain quads whose
// centres satisfy Contains(), then render these triangles with the terrain material.
class CaveGroundPatch final {
public:
    static constexpr int Subdivision=4;
    static constexpr int CoverColumns=57;
    static constexpr int CoverRows=49;
    CaveGroundPatch(const CaveLayout& cave,const RenderedTerrainSurface& original);
    [[nodiscard]] bool Contains(float x,float z) const noexcept;
    [[nodiscard]] float SampleHeight(float x,float z) const;
    // All changes of the piecewise-linear support along a horizontal segment.
    // Includes the rectangular patch boundary, exact rounded grid edges and diagonals.
    [[nodiscard]] std::vector<float> TraceParameters(float x0,float z0,float x1,float z1) const;
    [[nodiscard]] std::span<const CavePoint> Vertices() const noexcept {return vertices_;}
    [[nodiscard]] std::span<const std::uint16_t> Indices() const noexcept {return indices_;}
    // Render-only second surface: terrain material over the closed rock crown.  It is
    // deliberately separate from the support grid because one X/Z location needs a
    // low cave floor for walking and a high soil roof for rendering.
    [[nodiscard]] std::span<const CavePoint> CoverVertices() const noexcept {return coverVertices_;}
    [[nodiscard]] std::span<const std::uint16_t> CoverIndices() const noexcept {return coverIndices_;}
    [[nodiscard]] int Columns() const noexcept {return static_cast<int>(xs_.size());}
    [[nodiscard]] int Rows() const noexcept {return static_cast<int>(zs_.size());}
private:
    std::vector<float> xs_,zs_;
    std::vector<CavePoint> vertices_;
    std::vector<std::uint16_t> indices_;
    std::vector<CavePoint> coverVertices_;
    std::vector<std::uint16_t> coverIndices_;
};
}
