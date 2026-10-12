#include "IslandSimulator/RenderedTerrainSurface.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace island
{
    namespace
    {
        constexpr auto Coordinates = [] {
            std::array<float, TerrainMeshLayout::Resolution> result{};
            for (int i = 0; i < TerrainMeshLayout::Resolution; ++i) {
                result[i] = TerrainMeshLayout::Coordinate(i);
            }
            return result;
        }();

        int Quad(const float position)
        {
            // The regular grid gives an O(1) candidate. Correct against the actual
            // rounded vertex coordinates so exact edges still select the same quad
            // as upper_bound, including negative coordinates and the outer extent.
            int index = std::clamp(static_cast<int>((double(position) + TerrainMeshLayout::Extent)
                * (TerrainMeshLayout::QuadsPerAxis / (2.0 * TerrainMeshLayout::Extent))),
                0, TerrainMeshLayout::QuadsPerAxis - 1);
            while (index > 0 && position < Coordinates[index]) --index;
            while (index < TerrainMeshLayout::QuadsPerAxis - 1 && position >= Coordinates[index+1]) ++index;
            return index;
        }
    }

    RenderedTerrainSurface::RenderedTerrainSurface(const IslandTerrain& terrain)
    {
        for (int z = 0; z < TerrainMeshLayout::Resolution; ++z) {
            for (int x = 0; x < TerrainMeshLayout::Resolution; ++x) {
                heights_[z * TerrainMeshLayout::Resolution + x] = terrain.Height(Coordinates[x], Coordinates[z]);
            }
        }
    }

    RenderedTerrainSample RenderedTerrainSurface::Sample(const float x, const float z) const
    {
        if (!std::isfinite(x) || !std::isfinite(z)
            || std::abs(x) > TerrainMeshLayout::Extent || std::abs(z) > TerrainMeshLayout::Extent) {
            throw std::invalid_argument("Rendered terrain query outside finite mesh");
        }
        const int column = Quad(x), row = Quad(z);
        const double width = double(Coordinates[column + 1]) - Coordinates[column];
        const double depth = double(Coordinates[row + 1]) - Coordinates[row];
        const double u = (double(x) - Coordinates[column]) / width;
        const double v = (double(z) - Coordinates[row]) / depth;
        const int index = row * TerrainMeshLayout::Resolution + column;
        const double tl = heights_[index], tr = heights_[index + 1];
        const double bl = heights_[index + TerrainMeshLayout::Resolution];
        const double br = heights_[index + TerrainMeshLayout::Resolution + 1];
        double height, dx, dz;
        // TerrainRenderer emits TL/BL/TR and TR/BL/BR, never bilinear interpolation.
        if (u + v <= 1.0) {
            height = tl + (tr - tl) * u + (bl - tl) * v;
            dx = (tr - tl) / width;
            dz = (bl - tl) / depth;
        } else {
            height = br + (bl - br) * (1.0 - u) + (tr - br) * (1.0 - v);
            dx = (br - bl) / width;
            dz = (br - tr) / depth;
        }
        const double length = std::hypot(dx, 1.0, dz);
        return {static_cast<float>(height), {static_cast<float>(-dx / length),
            static_cast<float>(1.0 / length), static_cast<float>(-dz / length)}};
    }

    std::array<float,2> RenderedTerrainSurface::HeightRange(float minX,float minZ,float maxX,float maxZ) const
    {
        (void)Sample(minX,minZ); (void)Sample(maxX,maxZ);
        if(minX>maxX || minZ>maxZ) throw std::invalid_argument("Reversed terrain footprint");
        std::array<float,2> range{64.0f,-64.0f};
        // Include every vertex of every intersecting quad; linear triangles cannot overshoot.
        for(int row=Quad(minZ);row<=Quad(maxZ)+1;++row) {
            for(int col=Quad(minX);col<=Quad(maxX)+1;++col) {
                const float height=heights_[row*TerrainMeshLayout::Resolution+col];
                range[0]=std::min(range[0],height); range[1]=std::max(range[1],height);
            }
        }
        return range;
    }

    std::array<std::uint8_t,4> RenderedTerrainSurface::EncodeHeight(const float height)
    {
        if(!std::isfinite(height) || height<MinimumEncodedHeight
            || height>MinimumEncodedHeight+EncodedHeightRange) {
            throw std::invalid_argument("Terrain height outside RGB24 encoding range");
        }
        const auto code=static_cast<std::uint32_t>(std::llround(
            (double(height)-MinimumEncodedHeight)*16777215.0/EncodedHeightRange));
        return {static_cast<std::uint8_t>(code&255U),static_cast<std::uint8_t>((code>>8U)&255U),
                static_cast<std::uint8_t>((code>>16U)&255U),255};
    }

    float RenderedTerrainSurface::DecodeHeight(const std::array<std::uint8_t,4> pixel) noexcept
    {
        const auto code=std::uint32_t(pixel[0])+(std::uint32_t(pixel[1])<<8U)+(std::uint32_t(pixel[2])<<16U);
        return MinimumEncodedHeight+static_cast<float>(code)*(EncodedHeightRange/16777215.0f);
    }

    float RenderedTerrainSurface::SamplePacked(const float x,const float z) const
    {
        (void)Sample(x,z); // Reject invalid/out-of-mesh coordinates just like the exact sampler.
        const float gx=(x+TerrainMeshLayout::Extent)*(TerrainMeshLayout::QuadsPerAxis/(2.0f*TerrainMeshLayout::Extent));
        const float gz=(z+TerrainMeshLayout::Extent)*(TerrainMeshLayout::QuadsPerAxis/(2.0f*TerrainMeshLayout::Extent));
        const int col=std::clamp(static_cast<int>(std::floor(gx)),0,TerrainMeshLayout::QuadsPerAxis-1);
        const int row=std::clamp(static_cast<int>(std::floor(gz)),0,TerrainMeshLayout::QuadsPerAxis-1);
        const float u=std::clamp(gx-col,0.0f,1.0f),v=std::clamp(gz-row,0.0f,1.0f);
        const auto height=[&](int c,int r) { return DecodeHeight(EncodeHeight(heights_[r*TerrainMeshLayout::Resolution+c])); };
        const float tr=height(col+1,row),bl=height(col,row+1);
        if(u+v<=1) { const float tl=height(col,row); return tl+(tr-tl)*u+(bl-tl)*v; }
        const float br=height(col+1,row+1); return br+(bl-br)*(1-u)+(tr-br)*(1-v);
    }
}
