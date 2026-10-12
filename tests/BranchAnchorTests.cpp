#include "IslandSimulator/CanopyBranchProfile.hpp"
#include "IslandSimulator/CanopyProfile.hpp"
#include "IslandSimulator/JungleWind.hpp"
#include <cmath>
#include <iostream>
#include <numbers>
#include <stdexcept>

namespace {
void Check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
island::BranchPoint ReferenceWind(const island::CanopyBranchVertex& vertex,float time) {
    auto p=vertex.position;
    const double phase=vertex.phase*2.0*std::numbers::pi;
    const double sway=(std::sin(time*.61+phase+p[1]*.075)*island::JungleWind::Broad
        +std::sin(time*1.07+phase*1.41+p[0]*.18-p[2]*.21)*island::JungleWind::Detail)*vertex.weight;
    p[0]+=static_cast<float>(sway);p[2]+=static_cast<float>(sway*island::JungleWind::DirectionZ);
    p[1]-=static_cast<float>(std::abs(sway)*island::JungleWind::Lowering*vertex.weight);
    return p;
}
}
int main() {
    using namespace island;
    std::size_t samples=0;
    for(int variant=0;variant<CanopyFormCount;++variant)for(int cluster=1;cluster<8;++cluster) {
        const auto branch=MakeCanopyBranchProfile(variant,cluster);
        for(std::size_t triangle=0;triangle<branch.triangles.size();++triangle) {
            const auto ids=branch.triangles[triangle];
            const auto a=branch.vertices.at(ids[0]),b=branch.vertices.at(ids[1]),c=branch.vertices.at(ids[2]);
            const BranchPoint ab{b.position[0]-a.position[0],b.position[1]-a.position[1],b.position[2]-a.position[2]};
            const BranchPoint ac{c.position[0]-a.position[0],c.position[1]-a.position[1],c.position[2]-a.position[2]};
            const BranchPoint normal{ab[1]*ac[2]-ab[2]*ac[1],ab[2]*ac[0]-ab[0]*ac[2],ab[0]*ac[1]-ab[1]*ac[0]};
            float dot=0;for(std::size_t k=0;k<3;++k)dot+=normal[k]*(a.outward[k]+b.outward[k]+c.outward[k]);
            Check(dot>0,"inward or degenerate support triangle");
            Check(SampleCanopyBranchAnchor(branch,triangle,0,0)==a.position,"vertex anchor moved at rest");
            for(float time:{0.0f,.7f,3.0f,11.5f,60.0f}) {
                const auto actual=SampleCanopyBranchAnchor(branch,triangle,.2f,.3f,time);
                const auto wa=ReferenceWind(a,time),wb=ReferenceWind(b,time),wc=ReferenceWind(c,time);
                for(std::size_t k=0;k<3;++k) {
                    const float expected=.5f*wa[k]+.2f*wb[k]+.3f*wc[k];
                    Check(std::isfinite(actual[k])&&std::abs(actual[k]-expected)<.00001f,
                        "anchor does not follow actual wind-deformed triangle");
                }
                ++samples;
            }
        }
    }
    const auto branch=MakeCanopyBranchProfile(0,1);
    for(int fault=0;fault<7;++fault) {
        bool rejected=false;
        try {
            if(fault<3)(void)MakeCanopyBranchProfile(fault==0?-1:0,fault==1?0:fault==2?8:1);
            else (void)SampleCanopyBranchAnchor(branch,fault==3?24:0,
                fault==4?-.1f:fault==5?.8f:.2f,.3f,fault==6?NAN:0.0f);
        } catch(const std::invalid_argument&){rejected=true;}
        Check(rejected,"invalid branch anchor accepted");
    }
    std::cout<<samples<<" branch triangle/wind anchors checked; webs not yet live\n";
}
