#include "IslandSimulator/BushProfile.hpp"
#include "IslandSimulator/JungleWind.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>

void TestBushProfile()
{
    using namespace island;
    const auto require=[](bool condition,const char* message) {
        if (!condition) throw std::runtime_error(message);
    };
    int samples=0;
    for (int variant=0;variant<3;++variant) for (int shoot=0;shoot<BushShootCount(variant);++shoot) {
        const auto stem=MakeBushShoot(variant,shoot);
        require(SampleBushShoot(stem,0)==BushPoint{0,0,0} && BushShootWind(0)==0,
            "Shrub shoot root is not grounded and anchored");
        for (int leafIndex=0;leafIndex<BushLeavesPerShoot;++leafIndex) {
            const auto leaf=MakeBushLeaf(variant,shoot,leafIndex);
            const float attachment=leafIndex==8 ? 1.0f : BushShootKnots[1+leafIndex/2];
            require(leaf.root==SampleBushShoot(stem,attachment) && leaf.phase==stem.phase
                && BushLeafWind(leaf,0)==BushShootWind(attachment),
                "Shrub leaf is detached from its supporting shoot or wind phase");
            for (int row=0;row<=20;++row) for (int column=0;column<=10;++column) {
                const float t=row/20.0f,side=column/5.0f-1;
                const auto p=SampleBushLeaf(leaf,t,side),n=BushLeafNormal(leaf,t,side);
                require(std::all_of(p.begin(),p.end(),[](float v) { return std::isfinite(v); })
                    && p[1]>0 && p[1]<1.5f
                    && std::hypot(p[0],p[2])+JungleWind::HorizontalMargin<2.1f,
                    "Shrub leaf exceeds placement/wind/height bounds");
                require(std::abs(std::hypot(n[0],n[1],n[2])-1)<0.00001f,
                    "Shrub normal is not finite and unit length");
                if (row==0) require(p==leaf.root,"Shrub leaf root does not close on its stem");
                require(BushLeafWind(leaf,t)>=leaf.rootWind && BushLeafWind(leaf,t)<=0.61f,
                    "Shrub leaf wind exceeds its bounded range");
                if (row>1 && row<19) {
                    const auto a=SampleBushLeaf(leaf,t-0.002f,side),b=SampleBushLeaf(leaf,t+0.002f,side);
                    require(std::abs(n[0]*(b[0]-a[0])+n[1]*(b[1]-a[1])+n[2]*(b[2]-a[2]))<0.00001f,
                        "Shrub leaf normal is not tangent-orthogonal");
                }
                ++samples;
            }
            // Far retains every source leaf and its complete boundary; only the inner rib
            // is removed. Test actual triangles projected onto the leaf's local plane.
            std::array<float,2> projectedArea{};
            for (bool far:{false,true}) {
                const auto coordinates=BushLeafCoordinates(far);
                const auto indices=BushLeafIndices(far);
                require(coordinates.size()==static_cast<std::size_t>(far ? 12 : 17)
                    && indices.size()==static_cast<std::size_t>(far ? 30 : 60),
                    "Shrub closed-end mesh budget changed");
                for (std::size_t i=0;i<indices.size();i+=3) {
                    require(indices[i]<coordinates.size() && indices[i+1]<coordinates.size()
                        && indices[i+2]<coordinates.size(),"Shrub mesh index out of range");
                    const auto point=[&](std::uint16_t index) {
                        return SampleBushLeaf(leaf,coordinates[index][0],coordinates[index][1]);
                    };
                    const auto a=point(indices[i]),b=point(indices[i+1]),c=point(indices[i+2]);
                    const BushPoint u{b[0]-a[0],b[1]-a[1],b[2]-a[2]},v{c[0]-a[0],c[1]-a[1],c[2]-a[2]};
                    const float area=std::hypot(u[1]*v[2]-u[2]*v[1],u[2]*v[0]-u[0]*v[2],u[0]*v[1]-u[1]*v[0]);
                    require(std::isfinite(area) && area>0.000001f,"Shrub mesh contains a degenerate triangle");
                    const BushPoint normal{-std::cos(leaf.angle)*std::sin(leaf.pitch),
                        std::cos(leaf.pitch),-std::sin(leaf.angle)*std::sin(leaf.pitch)};
                    projectedArea[far ? 1 : 0]+=0.5f*std::abs(
                        normal[0]*(u[1]*v[2]-u[2]*v[1])+normal[1]*(u[2]*v[0]-u[0]*v[2])
                        +normal[2]*(u[0]*v[1]-u[1]*v[0]));
                }
            }
            require(projectedArea[0]>0 && std::abs(projectedArea[1]/projectedArea[0]-1)<0.00001f,
                "Shrub LOD changes leaf planform coverage");
            const auto nearCoordinates=BushLeafCoordinates(false),farCoordinates=BushLeafCoordinates(true);
            for (const auto& coordinate:nearCoordinates) {
                if (coordinate[1]==0 && coordinate[0]!=0 && coordinate[0]!=1) continue;
                const auto found=std::find(farCoordinates.begin(),farCoordinates.end(),coordinate);
                require(found!=farCoordinates.end(),"Shrub far LOD omitted a near outline vertex");
                require(SampleBushLeaf(leaf,coordinate[0],coordinate[1])==SampleBushLeaf(leaf,(*found)[0],(*found)[1])
                    && BushLeafNormal(leaf,coordinate[0],coordinate[1])==BushLeafNormal(leaf,(*found)[0],(*found)[1])
                    && BushLeafWind(leaf,coordinate[0])==BushLeafWind(leaf,(*found)[0]),
                    "Shrub LOD outline positions, normals or motion differ");
            }
        }
    }
    for (int field=0;field<10;++field) {
        auto leaf=MakeBushLeaf(0,0,0);
        bool rejected=false;
        try {
            switch (field) {
            case 0: (void)MakeBushShoot(-1,0); break;
            case 1: (void)MakeBushShoot(0,5); break;
            case 2: (void)MakeBushLeaf(0,0,9); break;
            case 3: (void)BushShootWind(-1); break;
            case 4: leaf.length=0; (void)SampleBushLeaf(leaf,0.5f,0); break;
            case 5: leaf.root[1]=std::numeric_limits<float>::quiet_NaN(); (void)SampleBushLeaf(leaf,0.5f,0); break;
            case 6: (void)BushLeafNormal(leaf,0.5f,2); break;
            case 7: leaf.phase=2; (void)BushLeafNormal(leaf,0.5f,0); break;
            case 8: leaf.rootWind=2; (void)BushLeafWind(leaf,0); break;
            case 9: (void)BushLeafNormal(leaf,std::numeric_limits<float>::infinity(),0); break;
            }
        } catch (const std::invalid_argument&) { rejected=true; }
        require(rejected,"Invalid shrub geometry accepted");
    }
    std::cout << "Bush profiles: " << samples << " bounded samples, supported roots and shared LOD identities\n";
}
