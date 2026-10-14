#include "IslandSimulator/GroundBroadleaf.hpp"

#include <array>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>

void TestGroundBroadleaf()
{
    const auto require=[](bool condition, const char* message) {
        if (!condition) throw std::runtime_error(message);
    };
    std::array<bool,6> retained{};
    std::array<float,6> heights{},reaches{},pigments{};
    for (int i=0; i<4; ++i) {
        const auto far=island::GroundBroadleafForTier(i,true);
        require(far==island::GroundBroadleafForTier(far.source,false),"Ground leaf LOD identity changed");
        require(!retained.at(far.source),"Duplicate far ground leaf");
        retained[far.source]=true;
    }
    int samples=0;
    for (int i=0; i<6; ++i) {
        const auto leaf=island::GroundBroadleafForTier(i,false);
        heights[i]=leaf.height;
        reaches[i]=leaf.reach;
        pigments[i]=leaf.pigment;
        for (int row=0; row<=100; ++row) for (int column=0; column<=20; ++column) {
            const float t=row/100.0f, side=column/10.0f-1;
            const auto p=island::SampleGroundBroadleaf(leaf,t,side);
            const auto n=island::GroundBroadleafNormal(leaf,t,side);
            require(std::isfinite(p.across) && std::isfinite(p.height) && std::isfinite(p.along)
                && p.height>=0 && p.height<0.75f && std::hypot(p.across,p.along)<0.70f,
                "Ground broadleaf escaped its small-plant envelope");
            require(std::abs(std::hypot(n.across,n.height,n.along)-1)<0.00001f,
                "Ground broadleaf normal is not finite and unit length");
            if (row==0) require(p.across==0 && p.height==leaf.height*0.31f && p.along==leaf.reach*0.28f,
                "Ground broadleaf root does not meet its petiole");
            if (row==100) require(p.across==0 && std::abs(p.height-leaf.height)<0.000001f
                && p.along==leaf.reach,"Ground broadleaf tip is not tapered");
            if (row>1 && row<99 && column>0 && column<20) {
                // Independently sample a wider derivative stencil, not the normal helper's exact steps.
                const auto a=island::SampleGroundBroadleaf(leaf,t-0.002f,side);
                const auto b=island::SampleGroundBroadleaf(leaf,t+0.002f,side);
                const float dot=n.across*(b.across-a.across)+n.height*(b.height-a.height)
                    +n.along*(b.along-a.along);
                require(std::abs(dot)<0.00001f,"Ground broadleaf normal is not tangent-orthogonal");
            }
            ++samples;
        }
        const auto middle=island::SampleGroundBroadleaf(leaf,0.5f,0);
        const auto edge=island::SampleGroundBroadleaf(leaf,0.5f,1);
        require(middle.height>edge.height,"Ground broadleaf has no cupped lamina");
    }
    const auto spread=[](const auto& values) {
        const auto [low,high]=std::minmax_element(values.begin(),values.end());
        return *high-*low;
    };
    require(spread(heights)>0.15f && spread(reaches)>0.15f && spread(pigments)>0.15f,
        "Ground broadleaf rosette lost its layered stature or pigment variation");
    for (bool far : {false,true}) for (int bad : {-1,far?4:6}) {
        bool rejected=false;
        try { (void)island::GroundBroadleafForTier(bad,far); }
        catch (const std::invalid_argument&) { rejected=true; }
        require(rejected,"Invalid ground broadleaf source accepted");
    }
    for (int field=0; field<8; ++field) {
        auto leaf=island::GroundBroadleafForTier(0,false);
        float t=0.5f,side=0;
        switch (field) {
        case 0: t=-1; break;
        case 1: side=2; break;
        case 2: t=std::numeric_limits<float>::quiet_NaN(); break;
        case 3: leaf.height=0; break;
        case 4: leaf.reach=std::numeric_limits<float>::infinity(); break;
        case 5: leaf.width=1; break;
        case 6: leaf.phase=-1; break;
        case 7: leaf.height=2; break;
        }
        bool rejected=false;
        try { (void)island::GroundBroadleafNormal(leaf,t,side); }
        catch (const std::invalid_argument&) { rejected=true; }
        require(rejected,"Invalid ground broadleaf dimensions accepted");
    }
    std::cout << "Ground broadleaf: " << samples << " bounded samples, attached roots and shared LOD identities\n";
}
