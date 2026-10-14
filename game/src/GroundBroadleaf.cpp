#include "IslandSimulator/GroundBroadleaf.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>
#include <stdexcept>

namespace island
{
    GroundBroadleaf GroundBroadleafForTier(int index, bool far)
    {
        if (index < 0 || index >= (far ? 4 : 6)) throw std::invalid_argument("Invalid ground broadleaf index");
        constexpr std::array sources{0,1,3,4};
        constexpr std::array heights{0.52f,0.66f,0.57f,0.61f,0.49f,0.68f};
        constexpr std::array reaches{0.62f,0.50f,0.58f,0.54f,0.64f,0.47f};
        constexpr std::array widths{0.31f,0.27f,0.34f,0.29f,0.33f,0.25f};
        const int source = far ? sources[index] : index;
        return {source, 0.73f + 2*std::numbers::pi_v<float>*source/6
                + 0.15f*std::sin(source*2.3f),
            heights[source], reaches[source], widths[source],
            std::fmod(source/6.0f+0.27f,1.0f),
            source%3 == 0 ? 1.10f : (source%3 == 1 ? 0.94f : 1.02f)};
    }

    LeafBladePoint SampleGroundBroadleaf(const GroundBroadleaf& leaf, float t, float side)
    {
        if (!std::isfinite(t) || !std::isfinite(side) || t < 0 || t > 1 || std::abs(side) > 1
            || !std::isfinite(leaf.height) || !std::isfinite(leaf.reach) || !std::isfinite(leaf.width)
            || !std::isfinite(leaf.phase) || leaf.phase < 0 || leaf.phase > 1
            || leaf.height <= 0 || leaf.height > 1 || leaf.reach <= 0 || leaf.reach > 1
            || leaf.width <= 0 || leaf.width > 0.5f)
            throw std::invalid_argument("Invalid ground broadleaf profile or coordinate");
        const float arch = t == 0 || t == 1 ? 0 : std::sin(t*std::numbers::pi_v<float>);
        const float phase = leaf.phase*2*std::numbers::pi_v<float>;
        const float span = leaf.width*std::pow(arch,0.66f)*(1-0.14f*t);
        const float margin = 1+0.055f*side*std::sin(t*7+phase);
        return {side*span*margin,
            leaf.height*(0.31f+0.69f*t) + arch*(0.075f-0.025f*t)
                + span*(0.17f*(1-side*side)+0.075f*side*std::sin(t*4+phase)),
            leaf.reach*(0.28f+0.72f*t)};
    }

    LeafBladePoint GroundBroadleafNormal(const GroundBroadleaf& leaf, float t, float side)
    {
        (void)SampleGroundBroadleaf(leaf,t,side);
        t=std::clamp(t,0.002f,0.998f);
        const auto a=SampleGroundBroadleaf(leaf,t-0.001f,side);
        const auto b=SampleGroundBroadleaf(leaf,t+0.001f,side);
        const auto c=SampleGroundBroadleaf(leaf,t,std::max(-1.0f,side-0.002f));
        const auto d=SampleGroundBroadleaf(leaf,t,std::min(1.0f,side+0.002f));
        const float tx=b.across-a.across,ty=b.height-a.height,tz=b.along-a.along;
        const float sx=d.across-c.across,sy=d.height-c.height,sz=d.along-c.along;
        const float nx=ty*sz-tz*sy,ny=tz*sx-tx*sz,nz=tx*sy-ty*sx;
        const float length=std::hypot(nx,ny,nz);
        if (!std::isfinite(length) || length<=0) throw std::invalid_argument("Unrepresentable ground leaf normal");
        return {nx/length,ny/length,nz/length};
    }
}
