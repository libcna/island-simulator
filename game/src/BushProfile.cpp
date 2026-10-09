#include "IslandSimulator/BushProfile.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <stdexcept>

namespace island
{
    namespace
    {
        void CheckCoordinate(float t)
        {
            if (!std::isfinite(t) || t<0 || t>1)
                throw std::invalid_argument("Invalid shrub profile coordinate");
        }
    }

    int BushShootCount(int variant)
    {
        if (variant<0 || variant>2) throw std::invalid_argument("Invalid shrub variant");
        return 5+variant;
    }

    BushShoot MakeBushShoot(int variant, int shoot)
    {
        if (shoot<0 || shoot>=BushShootCount(variant))
            throw std::invalid_argument("Invalid shrub shoot");
        return {variant*0.61f+shoot*2.39996323f,
            0.48f+0.085f*((shoot*3+variant)%5),
            0.76f+0.075f*((shoot*2+variant)%5),
            0.05f*std::sin(shoot*2.17f+variant),
            std::fmod(shoot*0.173f+variant*0.21f,1.0f)};
    }

    BushPoint SampleBushShoot(const BushShoot& shoot, float t)
    {
        CheckCoordinate(t);
        if (!std::isfinite(shoot.angle) || !std::isfinite(shoot.reach)
            || !std::isfinite(shoot.height) || !std::isfinite(shoot.bend)
            || shoot.reach<=0 || shoot.reach>1 || shoot.height<=0 || shoot.height>1.2f
            || std::abs(shoot.bend)>0.1f)
            throw std::invalid_argument("Invalid shrub shoot profile");
        const float radial=shoot.reach*t*t;
        const float bend=shoot.bend*std::sin(t*std::numbers::pi_v<float>);
        return {std::cos(shoot.angle)*radial-std::sin(shoot.angle)*bend,
            shoot.height*t,std::sin(shoot.angle)*radial+std::cos(shoot.angle)*bend};
    }

    float BushShootWind(float t)
    {
        CheckCoordinate(t);
        return t*0.48f;
    }

    BushLeaf MakeBushLeaf(int variant, int shoot, int leaf)
    {
        const auto stem=MakeBushShoot(variant,shoot);
        if (leaf<0 || leaf>=BushLeavesPerShoot) throw std::invalid_argument("Invalid shrub leaf");
        const bool terminal=leaf==8;
        const float t=terminal ? 1.0f : BushShootKnots[1+leaf/2];
        const float length=(0.39f+0.04f*((leaf*3+shoot+variant)%5))*(terminal ? 0.8f : 1.0f);
        const float widthRatio=variant==0 ? 0.32f : (variant==1 ? 0.40f : 0.24f);
        return {leaf,SampleBushShoot(stem,t),
            stem.angle+(terminal ? 0.0f : (leaf%2==0 ? -1.12f : 1.12f))
                +0.12f*std::sin(shoot*1.7f+leaf*2.3f),
            terminal ? 0.38f : -0.14f+0.08f*(leaf/2)+0.09f*std::sin(shoot*2.2f+leaf),
            length,length*widthRatio,stem.phase,
            0.91f+0.055f*((leaf+shoot*2+variant)%4),BushShootWind(t)};
    }

    BushPoint SampleBushLeaf(const BushLeaf& leaf, float t, float side)
    {
        CheckCoordinate(t);
        if (!std::isfinite(side) || std::abs(side)>1 || !std::isfinite(leaf.angle)
            || !std::isfinite(leaf.pitch) || !std::isfinite(leaf.length)
            || !std::isfinite(leaf.width) || !std::isfinite(leaf.phase)
            || leaf.length<=0 || leaf.length>0.7f || leaf.width<=0 || leaf.width>0.3f
            || std::abs(leaf.pitch)>1 || leaf.phase<0 || leaf.phase>1
            || !std::all_of(leaf.root.begin(),leaf.root.end(),[](float value) { return std::isfinite(value); }))
            throw std::invalid_argument("Invalid shrub leaf profile");
        const float arch=t==0 || t==1 ? 0 : std::sin(t*std::numbers::pi_v<float>);
        const float across=side*leaf.width*std::pow(arch,0.8f)
            *(1+0.05f*side*std::sin(t*5+leaf.phase*6.2831853f));
        const float lift=leaf.length*(0.075f*arch-0.035f*t*t)
            +leaf.width*0.12f*(1-side*side)*arch;
        const float along=leaf.length*t;
        const float c=std::cos(leaf.angle),s=std::sin(leaf.angle);
        const float cp=std::cos(leaf.pitch),sp=std::sin(leaf.pitch);
        return {leaf.root[0]+c*cp*along-s*across-c*sp*lift,
            leaf.root[1]+sp*along+cp*lift,
            leaf.root[2]+s*cp*along+c*across-s*sp*lift};
    }

    BushPoint BushLeafNormal(const BushLeaf& leaf, float t, float side)
    {
        (void)SampleBushLeaf(leaf,t,side);
        t=std::clamp(t,0.002f,0.998f);
        const auto a=SampleBushLeaf(leaf,t-0.001f,side),b=SampleBushLeaf(leaf,t+0.001f,side);
        const auto c=SampleBushLeaf(leaf,t,std::max(-1.0f,side-0.002f));
        const auto d=SampleBushLeaf(leaf,t,std::min(1.0f,side+0.002f));
        const BushPoint tangent{b[0]-a[0],b[1]-a[1],b[2]-a[2]};
        const BushPoint lateral{d[0]-c[0],d[1]-c[1],d[2]-c[2]};
        BushPoint n{lateral[1]*tangent[2]-lateral[2]*tangent[1],
            lateral[2]*tangent[0]-lateral[0]*tangent[2],
            lateral[0]*tangent[1]-lateral[1]*tangent[0]};
        const float length=std::hypot(n[0],n[1],n[2]);
        if (!std::isfinite(length) || length<=0) throw std::invalid_argument("Invalid shrub leaf normal");
        for (auto& value:n) value/=length;
        return n;
    }

    float BushLeafWind(const BushLeaf& leaf, float t)
    {
        CheckCoordinate(t);
        if (!std::isfinite(leaf.rootWind) || leaf.rootWind<0 || leaf.rootWind>0.48f)
            throw std::invalid_argument("Invalid shrub leaf wind attachment");
        return leaf.rootWind+0.12f*t;
    }

    std::vector<std::array<float,2>> BushLeafCoordinates(bool far)
    {
        const int rows=BushLeafRows(),columns=BushLeafColumns(far);
        std::vector<std::array<float,2>> coordinates{{0,0}};
        for (int row=1;row<rows-1;++row) for (int column=0;column<columns;++column)
            coordinates.push_back({static_cast<float>(row)/(rows-1),
                -1.0f+2.0f*column/(columns-1)});
        coordinates.push_back({1,0});
        return coordinates;
    }

    std::vector<std::uint16_t> BushLeafIndices(bool far)
    {
        const int rows=BushLeafRows(),columns=BushLeafColumns(far);
        // Closed root/tip have one vertex each: avoid four zero-area triangles per leaf.
        std::vector<std::uint16_t> indices;
        for (int col=0;col<columns-1;++col)
            indices.insert(indices.end(),{0,static_cast<std::uint16_t>(1+col),
                static_cast<std::uint16_t>(2+col)});
        for (int row=0;row<rows-3;++row) for (int col=0;col<columns-1;++col) {
            const auto a=static_cast<std::uint16_t>(1+row*columns+col);
            const auto b=static_cast<std::uint16_t>(a+1),c=static_cast<std::uint16_t>(a+columns);
            const auto d=static_cast<std::uint16_t>(c+1);
            indices.insert(indices.end(),{a,c,b,b,c,d});
        }
        const auto last=static_cast<std::uint16_t>(1+(rows-3)*columns);
        const auto tip=static_cast<std::uint16_t>(1+(rows-2)*columns);
        for (int col=0;col<columns-1;++col)
            indices.insert(indices.end(),{static_cast<std::uint16_t>(last+col),tip,
                static_cast<std::uint16_t>(last+col+1)});
        return indices;
    }
}
