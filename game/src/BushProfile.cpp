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
            0.58f+0.075f*((shoot*3+variant)%5),
            0.78f+0.075f*((shoot*2+variant)%5),
            0.085f*std::sin(shoot*2.17f+variant),
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
        const float radial=shoot.reach*t*(0.28f+0.72f*t);
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
        const float length=(0.39f+0.04f*((leaf*3+shoot+variant)%5))*(terminal ? 0.92f : 1.0f);
        const float widthRatio=variant==0 ? 0.36f : (variant==1 ? 0.42f : 0.28f);
        const float sideSign=leaf%2==0 ? -1.0f : 1.0f;
        const float roll=terminal ? 0.12f*std::sin(shoot*1.37f+variant)
            : sideSign*(0.20f+0.045f*(leaf/2))
                +0.075f*std::sin(shoot*1.31f+leaf*0.87f+variant*0.61f);
        return {leaf,SampleBushShoot(stem,t),
            stem.angle+(terminal ? 0.0f : sideSign*1.12f)
                +0.12f*std::sin(shoot*1.7f+leaf*2.3f),
            terminal ? 0.50f : -0.08f+0.11f*(leaf/2)
                +(leaf%2==0 ? -0.045f : 0.045f)+0.08f*std::sin(shoot*2.2f+leaf),
            roll,length,length*widthRatio,stem.phase,
            0.84f+0.075f*((leaf+shoot*2+variant)%4),BushShootWind(t)};
    }

    bool BushLeafPresent(int variant, int shoot, int leaf)
    {
        (void)MakeBushShoot(variant,shoot);
        if (leaf<0 || leaf>=BushLeavesPerShoot)
            throw std::invalid_argument("Invalid shrub leaf");
        if (leaf==BushLeavesPerShoot-1) return true;
        // Preserve the terminal crown while leaving two or three deterministic gaps
        // among paired lateral leaves. Near and far call the same predicate, so a
        // moving LOD transition cannot grow or lose a retained leaf.
        return (shoot*7+leaf*5+variant*3)%8!=0;
    }

    BushPoint SampleBushLeaf(const BushLeaf& leaf, float t, float side)
    {
        CheckCoordinate(t);
        if (!std::isfinite(side) || std::abs(side)>1 || !std::isfinite(leaf.angle)
            || !std::isfinite(leaf.pitch) || !std::isfinite(leaf.roll)
            || !std::isfinite(leaf.length)
            || !std::isfinite(leaf.width) || !std::isfinite(leaf.phase)
            || leaf.length<=0 || leaf.length>0.7f || leaf.width<=0 || leaf.width>0.3f
            || std::abs(leaf.pitch)>1 || std::abs(leaf.roll)>0.6f
            || leaf.phase<0 || leaf.phase>1
            || !std::all_of(leaf.root.begin(),leaf.root.end(),[](float value) { return std::isfinite(value); }))
            throw std::invalid_argument("Invalid shrub leaf profile");
        const float arch=t==0 || t==1 ? 0 : std::sin(t*std::numbers::pi_v<float>);
        const float across=side*leaf.width*std::pow(arch,0.8f)
            *(1+0.05f*side*std::sin(t*5+leaf.phase*6.2831853f));
        const float lift=leaf.length*(0.10f*arch-0.032f*t*t)
            +leaf.width*0.16f*(1-side*side)*arch;
        const float along=leaf.length*t;
        const float c=std::cos(leaf.angle),s=std::sin(leaf.angle);
        const float cp=std::cos(leaf.pitch),sp=std::sin(leaf.pitch);
        const float cr=std::cos(leaf.roll),sr=std::sin(leaf.roll);
        const BushPoint direction{c*cp,sp,s*cp};
        const BushPoint lateral{-s*cr-c*sp*sr,cp*sr,c*cr-s*sp*sr};
        const BushPoint normal{s*sr-c*sp*cr,cp*cr,-c*sr-s*sp*cr};
        return {leaf.root[0]+direction[0]*along+lateral[0]*across+normal[0]*lift,
            leaf.root[1]+direction[1]*along+lateral[1]*across+normal[1]*lift,
            leaf.root[2]+direction[2]*along+lateral[2]*across+normal[2]*lift};
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
