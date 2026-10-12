#include "IslandSimulator/CanopyBranchProfile.hpp"
#include "IslandSimulator/CanopyProfile.hpp"
#include "IslandSimulator/JungleWind.hpp"
#include <cmath>
#include <numbers>
#include <stdexcept>

namespace island {
namespace {
BranchPoint Add(BranchPoint a,BranchPoint b){return {a[0]+b[0],a[1]+b[1],a[2]+b[2]};}
BranchPoint Mul(BranchPoint a,float s){return {a[0]*s,a[1]*s,a[2]*s};}
BranchPoint Sub(BranchPoint a,BranchPoint b){return Add(a,Mul(b,-1));}
float Length(BranchPoint p){return std::sqrt(p[0]*p[0]+p[1]*p[1]+p[2]*p[2]);}
BranchPoint Unit(BranchPoint p){return Mul(p,1/Length(p));}
BranchPoint Cross(BranchPoint a,BranchPoint b){return {a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]};}
}
CanopyBranchProfile MakeCanopyBranchProfile(int variant,int cluster) {
    if(cluster<1||cluster>7)throw std::invalid_argument("Invalid canopy support branch");
    const auto crown=MakeCanopyCrownProfile(variant,cluster);
    const auto attachment=SampleCanopyTrunk(variant,.65f+.065f*static_cast<float>(cluster/2));
    const BranchPoint start{attachment[0]+std::cos(crown.angle)*.10f,attachment[1],attachment[2]+std::sin(crown.angle)*.10f};
    const BranchPoint end{crown.x-std::cos(crown.angle)*.48f,crown.y-.62f,crown.z-std::sin(crown.angle)*.48f};
    const auto axis=Unit(Sub(end,start));
    const auto right=Unit(Cross(std::abs(axis[1])<.88f?BranchPoint{0,1,0}:BranchPoint{1,0,0},axis));
    const auto up=Unit(Cross(axis,right));
    const float phaseAngle=crown.phase*2*std::numbers::pi_v<float>;
    const auto bend=Add(Add(Add(start,Mul(Sub(end,start),.56f)),
        Mul(right,.14f+.075f*std::sin(phaseAngle))),Mul(up,.065f*std::cos(phaseAngle*1.31f)));
    const std::array<BranchPoint,3> centres{start,bend,end};
    constexpr std::array<float,3> radii{.17f,.115f,.070f},weights{.12f,.34f,.58f};
    CanopyBranchProfile result{};
    for(std::size_t ring=0;ring<3;++ring)for(std::size_t side=0;side<6;++side) {
        const float angle=2*std::numbers::pi_v<float>*static_cast<float>(side)/6;
        const auto outward=Add(Mul(right,std::cos(angle)),Mul(up,std::sin(angle)));
        const float along=ring==0?0:Length(Sub(bend,start))+(ring==2?Length(Sub(end,bend)):0);
        result.vertices[ring*6+side]={Add(centres[ring],Mul(outward,radii[ring])),outward,
            weights[ring],crown.phase,angle,along};
    }
    for(std::size_t segment=0;segment<2;++segment)for(std::size_t side=0;side<6;++side) {
        const auto lower=segment*6+side,next=segment*6+(side+1)%6,upper=lower+6,upperNext=next+6;
        result.triangles[(segment*6+side)*2]={lower,next,upper};
        result.triangles[(segment*6+side)*2+1]={next,upperNext,upper};
    }
    return result;
}
BranchPoint CanopyBranchPosition(const CanopyBranchVertex& vertex,std::optional<float> windTime) {
    auto p=vertex.position;
    if(windTime) {
        if(!std::isfinite(*windTime))throw std::invalid_argument("Invalid branch wind time");
        const float phase=vertex.phase*2*std::numbers::pi_v<float>;
        const float broad=std::sin(*windTime*.61f+phase+p[1]*.075f);
        const float detail=std::sin(*windTime*1.07f+phase*1.41f+p[0]*.18f-p[2]*.21f);
        const float sway=(broad*JungleWind::Broad+detail*JungleWind::Detail)*vertex.weight;
        p[0]+=sway;p[2]+=sway*JungleWind::DirectionZ;
        p[1]-=std::abs(sway)*JungleWind::Lowering*vertex.weight;
    }
    return p;
}
BranchPoint SampleCanopyBranchAnchor(const CanopyBranchProfile& branch,std::size_t triangle,
    float u,float v,std::optional<float> windTime) {
    if(triangle>=branch.triangles.size()||!std::isfinite(u)||!std::isfinite(v)||u<0||v<0||u+v>1)
        throw std::invalid_argument("Invalid branch attachment coordinate");
    const auto indices=branch.triangles[triangle];
    const auto a=CanopyBranchPosition(branch.vertices.at(indices[0]),windTime);
    const auto b=CanopyBranchPosition(branch.vertices.at(indices[1]),windTime);
    const auto c=CanopyBranchPosition(branch.vertices.at(indices[2]),windTime);
    return Add(Add(Mul(a,1-u-v),Mul(b,u)),Mul(c,v));
}
}
