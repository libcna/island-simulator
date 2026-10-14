#include "IslandSimulator/SplitGroundLeaf.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <numbers>
#include <stdexcept>

namespace island
{
    float SplitGroundLeafRow(int row, bool far)
    {
        if (row<0 || row>=SplitGroundLeafRows(far)) throw std::invalid_argument("Invalid split-leaf row");
        if (far) return SplitGroundLeafKnots[row];
        const int interval=row/SplitGroundLeafNearSubdivisions;
        const int part=row%SplitGroundLeafNearSubdivisions;
        return part == 0 ? SplitGroundLeafKnots[interval]
            : std::lerp(SplitGroundLeafKnots[interval],SplitGroundLeafKnots[interval+1],
                        static_cast<float>(part)/SplitGroundLeafNearSubdivisions);
    }

    SplitGroundLeaf SplitGroundLeafForTier(int index, bool far)
    {
        if (index<0 || index>=(far ? 4 : 5)) throw std::invalid_argument("Invalid split ground leaf index");
        constexpr std::array sources{0,1,3,4};
        constexpr std::array lengths{0.74f,0.65f,0.77f,0.68f,0.72f};
        constexpr std::array widths{0.29f,0.32f,0.26f,0.31f,0.28f};
        constexpr std::array lifts{0.49f,0.56f,0.50f,0.53f,0.49f};
        const int source=far ? sources[index] : index;
        return {source,1.46f+2*std::numbers::pi_v<float>*source/5
                +0.18f*std::sin(source*2.7f), lengths[source],widths[source],lifts[source],
            std::fmod(source/5.0f+0.54f,1.0f),
            source%3 == 0 ? 1.10f : (source%3 == 1 ? 0.93f : 1.02f)};
    }

    LeafBladePoint SampleSplitGroundLeaf(const SplitGroundLeaf& leaf, float t, float side)
    {
        if (!std::isfinite(t) || !std::isfinite(side) || t<0 || t>1 || std::abs(side)>1
            || !std::isfinite(leaf.length) || leaf.length<=0 || leaf.length>1
            || !std::isfinite(leaf.width) || leaf.width<=0 || leaf.width>0.5f
            || !std::isfinite(leaf.lift) || leaf.lift<=0 || leaf.lift>1
            || leaf.lift*0.30f<=leaf.length*0.07f
            || !std::isfinite(leaf.phase) || leaf.phase<0 || leaf.phase>1)
            throw std::invalid_argument("Invalid split ground leaf profile or coordinate");
        const std::array spans{SplitGroundPetioleHalfWidth,SplitGroundPetioleHalfWidth,
            leaf.width,leaf.width*0.27f,leaf.width*0.82f,leaf.width*0.23f,
            leaf.width*0.61f,leaf.width*0.18f,0.0f};
        const auto upper=std::upper_bound(SplitGroundLeafKnots.begin(),SplitGroundLeafKnots.end(),t);
        const auto interval=static_cast<std::size_t>(std::clamp<std::ptrdiff_t>(
            upper-SplitGroundLeafKnots.begin()-1,0,SplitGroundLeafKnots.size()-2));
        float u=(t-SplitGroundLeafKnots[interval])
            /(SplitGroundLeafKnots[interval+1]-SplitGroundLeafKnots[interval]);
        u=u*u*(3-2*u);
        const float span=std::lerp(spans[interval],spans[interval+1],u);
        const float phase=leaf.phase*2*std::numbers::pi_v<float>;
        float blade=std::clamp((t-0.22f)/0.12f,0.0f,1.0f);
        blade=blade*blade*(3-2*blade);
        const float arch=t==0 || t==1 ? 0 : std::sin(t*std::numbers::pi_v<float>);
        return {side*span*(1+0.075f*side*std::sin(t*7+phase)*blade),
            leaf.lift*(0.30f*t+0.78f*arch)-0.07f*leaf.length*t*t
                +span*blade*(0.14f*(1-side*side)+0.075f*side*std::sin(t*4+phase)),
            leaf.length*t};
    }

    LeafBladePoint SplitGroundLeafNormal(const SplitGroundLeaf& leaf, float t, float side)
    {
        (void)SampleSplitGroundLeaf(leaf,t,side);
        t=std::clamp(t,0.002f,0.998f);
        const auto a=SampleSplitGroundLeaf(leaf,t-0.001f,side);
        const auto b=SampleSplitGroundLeaf(leaf,t+0.001f,side);
        const auto c=SampleSplitGroundLeaf(leaf,t,std::max(-1.0f,side-0.002f));
        const auto d=SampleSplitGroundLeaf(leaf,t,std::min(1.0f,side+0.002f));
        const float tx=b.across-a.across,ty=b.height-a.height,tz=b.along-a.along;
        const float sx=d.across-c.across,sy=d.height-c.height,sz=d.along-c.along;
        const float nx=ty*sz-tz*sy,ny=tz*sx-tx*sz,nz=tx*sy-ty*sx;
        const float length=std::hypot(nx,ny,nz);
        if (!std::isfinite(length) || length<=0) throw std::invalid_argument("Unrepresentable split ground leaf normal");
        return {nx/length,ny/length,nz/length};
    }

    std::vector<std::uint16_t> SplitGroundLeafIndices(bool far)
    {
        const int rows=SplitGroundLeafRows(far),columns=SplitGroundLeafColumns(far);
        std::vector<std::uint16_t> indices;
        indices.reserve((rows-1)*(columns-1)*6);
        for (int row=0;row<rows-1;++row) for (int col=0;col<columns-1;++col) {
            const auto a=static_cast<std::uint16_t>(row*columns+col);
            const auto b=static_cast<std::uint16_t>(a+1),c=static_cast<std::uint16_t>(a+columns);
            const auto d=static_cast<std::uint16_t>(c+1);
            indices.insert(indices.end(),{a,c,b,b,c,d});
        }
        return indices;
    }
}
