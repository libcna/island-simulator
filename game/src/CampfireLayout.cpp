#include "IslandSimulator/CampfireLayout.hpp"
#include <algorithm>
#include <cmath>
#include <numbers>
#include <stdexcept>
namespace island {
namespace {
FirePoint Add(FirePoint a,FirePoint b){return {a.x+b.x,a.y+b.y,a.z+b.z};}
FirePoint Sub(FirePoint a,FirePoint b){return {a.x-b.x,a.y-b.y,a.z-b.z};}
FirePoint Mul(FirePoint a,float b){return {a.x*b,a.y*b,a.z*b};}
FirePoint Cross(FirePoint a,FirePoint b){return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};}
float Dot(FirePoint a,FirePoint b){return a.x*b.x+a.y*b.y+a.z*b.z;}
FirePoint Unit(FirePoint p){return Mul(p,1/std::sqrt(Dot(p,p)));}
constexpr float Tau=2*std::numbers::pi_v<float>;
}
float CampfireLayout::GroundY() {
    static const float value=RenderedTerrainSurface(IslandTerrain{}).Sample(CampfireSite::X,CampfireSite::Z).height;
    return value;
}
CampfireLayout::CampfireLayout(const IslandTerrain& terrain) {
    const RenderedTerrainSurface ground(terrain);
    centre_={CampfireSite::X,ground.Sample(CampfireSite::X,CampfireSite::Z).height,CampfireSite::Z};
    woodCentre_={CampfireSite::WoodX,ground.Sample(CampfireSite::WoodX,CampfireSite::WoodZ).height,CampfireSite::WoodZ};
    for(const auto point:{centre_,woodCentre_}) {
        const auto sample=terrain.Sample(point.x,point.z);
        if(sample.surface!=TerrainSurface::DrySand||sample.signedShoreDistance<7.0f
            ||sample.signedShoreDistance>23.0f||sample.beachSuitability<.70f)
            throw std::runtime_error("Campfire site is not on qualified dry beach sand");
    }
    obstacles_={CircularObstacle{centre_.x,centre_.z,CampfireSite::PitRadius},
                CircularObstacle{woodCentre_.x,woodCentre_.z,CampfireSite::WoodRadius}};
    const auto emit=[&](FirePoint a,FirePoint b,FirePoint c,FirePoint desired,FireMaterial material,
                        std::array<std::array<float,2>,3> uv) {
        auto n=Cross(Sub(b,a),Sub(c,a));
        if(Dot(n,desired)<0){std::swap(b,c);std::swap(uv[1],uv[2]);n=Mul(n,-1);}
        if(Dot(n,n)<1e-12f)throw std::runtime_error("Degenerate campfire surface");
        n=Unit(n);
        const std::array p{a,b,c};
        for(int i=0;i<3;++i)vertices_.push_back({p[i],n,uv[i][0],uv[i][1],material});
    };
    const auto stone=[&](FirePoint centre,FirePoint size,float phase,FireMaterial material) {
        constexpr int sides=10;
        std::array<std::array<FirePoint,sides>,3> rings{};
        for(int row=0;row<3;++row)for(int side=0;side<sides;++side) {
            const float a=Tau*side/sides,t=std::numbers::pi_v<float>*(row+1)/4;
            const float irregular=1+.07f*std::sin(side*2.7f+row*1.9f+phase);
            rings[row][side]=Add(centre,{size.x*std::sin(t)*std::cos(a)*irregular,
                size.y*std::cos(t),size.z*std::sin(t)*std::sin(a)*irregular});
        }
        const auto tri=[&](FirePoint a,FirePoint b,FirePoint c) {
            const auto desired=Sub(Mul(Add(Add(a,b),c),1.0f/3),centre);
            emit(a,b,c,desired,material,{{{0,0},{1,0},{.5f,1}}});
        };
        for(int side=0;side<sides;++side) {
            const int next=(side+1)%sides;
            tri(Add(centre,{0,size.y,0}),rings[0][side],rings[0][next]);
            tri(Add(centre,{0,-size.y,0}),rings[2][next],rings[2][side]);
            for(int row=0;row<2;++row) {
                tri(rings[row][side],rings[row+1][side],rings[row+1][next]);
                tri(rings[row][side],rings[row+1][next],rings[row][next]);
            }
        }
    };
    const auto log=[&](FirePoint a,FirePoint b,float radius,FireMaterial material,float phase) {
        constexpr int sides=10;
        const auto axis=Unit(Sub(b,a)),right=Unit(Cross(axis,{0,1,0})),up=Cross(right,axis);
        std::array<std::array<FirePoint,sides>,2> rings{};
        for(int end=0;end<2;++end)for(int side=0;side<sides;++side) {
            const float angle=Tau*side/sides,r=radius*(1+.06f*std::sin(side*2.8f+phase));
            rings[end][side]=Add(end?b:a,Add(Mul(right,std::cos(angle)*r),Mul(up,std::sin(angle)*r)));
        }
        const float length=std::sqrt(Dot(Sub(b,a),Sub(b,a)));
        for(int side=0;side<sides;++side) {
            const int next=(side+1)%sides;
            const float u=static_cast<float>(side)/sides,v=static_cast<float>(side+1)/sides;
            const auto desired=Sub(rings[0][side],a);
            emit(rings[0][side],rings[1][side],rings[1][next],desired,material,
                {{{u,0},{u,length},{v,length}}});
            emit(rings[0][side],rings[1][next],rings[0][next],desired,material,
                {{{u,0},{v,length},{v,0}}});
            for(int end=0;end<2;++end) {
                const float sa=Tau*side/sides,sb=Tau*(side+1)/sides;
                emit(end?b:a,rings[end][side],rings[end][next],Mul(axis,end?1.0f:-1.0f),
                    material==FireMaterial::Firewood?FireMaterial::EndGrain:material,
                    {{{0,0},{std::cos(sa),std::sin(sa)},{std::cos(sb),std::sin(sb)}}});
            }
        }
    };
    for(int i=0;i<32;++i) {
        const float a=Tau*i/32,b=Tau*(i+1)/32;
        FirePoint p{centre_.x+std::cos(a)*1.12f,0,centre_.z+std::sin(a)*1.12f};
        FirePoint q{centre_.x+std::cos(b)*1.12f,0,centre_.z+std::sin(b)*1.12f};
        p.y=ground.Sample(p.x,p.z).height+.012f;q.y=ground.Sample(q.x,q.z).height+.012f;
        emit(Add(centre_,{0,.012f,0}),p,q,{0,1,0},FireMaterial::Ash,
            {{{0,0},{std::cos(a),std::sin(a)},{std::cos(b),std::sin(b)}}});
    }
    for(int i=0;i<StoneCount;++i) {
        const float angle=Tau*i/StoneCount;
        FirePoint p{centre_.x+.96f*std::cos(angle),0,centre_.z+.96f*std::sin(angle)};
        p.y=ground.Sample(p.x,p.z).height+.11f;
        stone(p,{.225f,.145f,.19f},static_cast<float>(i),FireMaterial::Stone);
    }
    for(int i=0;i<BurningLogCount;++i) {
        const float angle=i*1.7f,offset=(i%2?-.13f:.13f);
        FirePoint a{centre_.x+std::cos(angle)*.67f,0,centre_.z+std::sin(angle)*.67f+offset};
        FirePoint b{centre_.x-std::cos(angle)*.56f,0,centre_.z-std::sin(angle)*.56f+offset};
        a.y=ground.Sample(a.x,a.z).height+.105f+.035f*i;
        b.y=ground.Sample(b.x,b.z).height+.105f+.035f*i;
        log(a,b,.10f,FireMaterial::CharredWood,static_cast<float>(i));
    }
    for(int i=0;i<FirewoodCount;++i) {
        const int row=i<5?0:i<9?1:2,column=i<5?i:i<9?i-5:i-9;
        const float offset=(column-(row==0?2.0f:row==1?1.5f:.5f))*.245f;
        const float length=1.65f+.08f*(i%4),radius=.103f+.008f*(i%3);
        FirePoint a{woodCentre_.x-length*.5f,0,woodCentre_.z+offset};
        FirePoint b{woodCentre_.x+length*.5f,0,woodCentre_.z+offset};
        a.y=ground.Sample(a.x,a.z).height+radius+.012f+row*.213f;
        b.y=ground.Sample(b.x,b.z).height+radius+.012f+row*.213f;
        log(a,b,radius,FireMaterial::Firewood,static_cast<float>(i)+1);
    }
    for(int i=0;i<CoalPieceCount;++i) {
        const float a=i*2.3999632f,r=.17f+.043f*i;
        FirePoint p{centre_.x+std::cos(a)*r,0,centre_.z+std::sin(a)*r};
        p.y=ground.Sample(p.x,p.z).height+.050f;
        const FirePoint size{.095f+.008f*(i%3),.043f+.004f*(i%2),.075f+.007f*((i+1)%3)};
        stone(p,size,static_cast<float>(i),i%3==0?FireMaterial::Ember:FireMaterial::Coal);
    }
    if(vertices_.size()>9000)throw std::runtime_error("Campfire geometry exceeds budget");
}
}
