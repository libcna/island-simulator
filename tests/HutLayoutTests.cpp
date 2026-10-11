#include "IslandSimulator/HutLayout.hpp"
#include "IslandSimulator/IslandTerrain.hpp"
#include "IslandSimulator/RenderedTerrainSurface.hpp"
#include "IslandSimulator/VegetationLayout.hpp"
#include "IslandSimulator/JungleLayout.hpp"
#include "IslandSimulator/NaturalPropLayout.hpp"
#include "IslandSimulator/ShipwreckLayout.hpp"
#include "IslandSimulator/PlayerMotion.hpp"
#include "IslandSimulator/ContactShadowPatch.hpp"
#include "IslandSimulator/HutThatch.hpp"
#include "HutReachability.hpp"
#include <cmath>
#include <algorithm>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>

static void Check(bool value,const char* message) { if(!value) throw std::runtime_error(message); }
int main() {
    float shortest=10,longest=0;
    for(int side:{-1,1}) for(int row=0;row<7;++row) for(int strip=0;strip<48;++strip) {
        const auto blade=island::BuildHutThatchBlade(side,row,strip),repeat=island::BuildHutThatchBlade(side,row,strip);
        Check(blade.pigment>=0&&blade.pigment<=1,"thatch pigment outside bounds");
        for(std::size_t station=0;station<blade.sections.size();++station) for(std::size_t edge=0;edge<3;++edge) {
            const auto p=blade.sections[station][edge],r=repeat.sections[station][edge];
            Check(std::isfinite(p.x)&&std::isfinite(p.y)&&std::isfinite(p.z),"nonfinite thatch");
            Check(p.x==r.x&&p.y==r.y&&p.z==r.z,"nondeterministic thatch");
            Check(std::abs(p.x)<=2.75f&&std::abs(p.z)<=2.32f&&p.y>=2.0f&&p.y<=3.81f,
                "thatch outside roof bounds");
            if(station>0) Check(side*(p.x-blade.sections[station-1][edge].x)>0,"thatch folded backwards");
        }
        const float length=std::abs(blade.sections.back()[1].x-blade.sections[0][1].x);
        Check(blade.sections.back()[2].z-blade.sections.back()[0].z<0.041f,
            "leaf ends retain square plank silhouette");
        shortest=std::min(shortest,length); longest=std::max(longest,length);
        if(row<6) {
            const auto next=island::BuildHutThatchBlade(side,row+1,strip);
            Check(side*(blade.sections.back()[1].x-next.sections[0][1].x)>0.08f,"roof courses do not overlap");
        }
        if(strip<47) {
            const auto next=island::BuildHutThatchBlade(side,row,strip+1);
            // Narrow tips are intentionally separate; the lower course covers them.
            for(std::size_t station=0;station+1<blade.sections.size();++station)
                Check(blade.sections[station][2].z>=next.sections[station][0].z-0.005f,"roof strips leave large gaps");
        }
    }
    Check(longest-shortest>0.08f,"thatch has uniform rigid lengths");
    // Check combined projected coverage, not just same-course station widths.
    // The final 20cm is an intentionally ragged eave outside the structural floor.
    for(int side:{-1,1}) for(int sample=0;sample<=255;++sample) {
        const float x=static_cast<float>(sample)*0.01f;
        std::vector<std::pair<float,float>> spans;
        for(int row=0;row<7;++row) for(int strip=0;strip<48;++strip) {
            const auto blade=island::BuildHutThatchBlade(side,row,strip);
            for(std::size_t i=0;i+1<blade.sections.size();++i) {
                const auto& a=blade.sections[i]; const auto& b=blade.sections[i+1];
                const float ax=side*a[1].x,bx=side*b[1].x;
                if(x<ax||x>bx) continue;
                const float t=(x-ax)/(bx-ax);
                spans.emplace_back(a[0].z+(b[0].z-a[0].z)*t,
                                   a[2].z+(b[2].z-a[2].z)*t);
            }
        }
        std::sort(spans.begin(),spans.end());
        float covered=-2.20f;
        for(const auto& [low,high]:spans) {
            if(high<covered) continue;
            Check(low<=covered+0.0001f,"layered roof has an internal projected hole");
            covered=std::max(covered,high);
            if(covered>=2.20f) break;
        }
        Check(covered>=2.20f,"layered roof does not cover structural interior");
    }
    const island::IslandTerrain terrain;
    const island::RenderedTerrainSurface surface(terrain);
    const island::VegetationLayout vegetation(terrain);
    const island::JungleLayout jungle(terrain);
    const island::NaturalPropLayout props(terrain,vegetation,jungle);
    const island::ShipwreckLayout ship(terrain);
    std::vector<island::CircularObstacle> occupied;
    for(const auto obstacles:{std::span<const island::CircularObstacle>(vegetation.Obstacles()),
        std::span<const island::CircularObstacle>(jungle.Obstacles()),
        std::span<const island::CircularObstacle>(props.Obstacles()),
        std::span<const island::CircularObstacle>(ship.Obstacles())})
        occupied.insert(occupied.end(),obstacles.begin(),obstacles.end());
    const auto live=island::HutLayout::FindInWorld(terrain,jungle,occupied);
    Check(live.has_value(),"live world has no hut");
    CheckHutReachability(terrain,*live,occupied);
    bool blockedRouteRejected=false;
    try { CheckHutReachability(terrain,*live,{{0,0,1000}}); }
    catch(const std::runtime_error&) { blockedRouteRejected=true; }
    Check(blockedRouteRejected,"global hut route accepted blocked world");
    for(const auto& p:jungle.Understory()) occupied.push_back({p.x,p.z,p.scale*1.8f});
    for(const auto& p:jungle.Bushes()) occupied.push_back({p.x,p.z,p.scale*1.2f});
    for(const auto& p:jungle.GroundPlants()) occupied.push_back({p.x,p.z,p.scale*0.5f});
    const auto found=island::HutLayout::Find(terrain,surface,occupied);
    Check(found.has_value(),"no hut site in current world");
    Check(live&&live->x==found->x&&live->z==found->z&&live->floorY==found->floorY
        &&live->yawRadians==found->yawRadians,"shared live/report placement differs from foliage fixture");
    const auto repeat=island::HutLayout::Find(terrain,surface,occupied);
    Check(repeat&&found->x==repeat->x&&found->z==repeat->z&&found->floorY==repeat->floorY
        &&found->yawRadians==repeat->yawRadians,"hut placement nondeterministic");
    for(const auto& o:occupied)
        Check(std::hypot(found->x-o.x,found->z-o.z)>=island::HutLayout::SiteClearance+o.radius,
            "hut site intersects existing solid");
    for(const auto& foot:found->footings) {
        Check(foot.groundY==surface.Sample(foot.x,foot.z).height,"post misses rendered ground");
        const float clearance=found->floorY-foot.groundY;
        Check(clearance>=island::HutLayout::FloorClearance-0.00001f
            &&clearance<=island::HutLayout::FloorClearance+island::HutLayout::MaximumRelief+0.00001f,
            "unsupported or buried floor");
    }
    for(std::size_t i=0;i<found->footings.size();++i)
        for(std::size_t j=i+1;j<found->footings.size();++j)
            Check(std::hypot(found->footings[i].x-found->footings[j].x,
                found->footings[i].z-found->footings[j].z)>1.6f,"duplicate or collapsed hut posts");
    const float c=std::cos(found->yawRadians),s=std::sin(found->yawRadians);
    const auto footprints=island::HutContactFootprints(*found);
    std::size_t shadowVertices=0;
    for(std::size_t index=0;index<footprints.size();++index) {
        const auto& footprint=footprints[index];
        Check(std::abs(footprint.directionX-s)<1e-6f&&std::abs(footprint.directionZ-c)<1e-6f,
            "hut shadow orientation differs from model");
        if(index>0) Check(footprint.x==found->footings[index-1].x&&footprint.z==found->footings[index-1].z,
            "post shadow not centered on footing");
        const auto patch=island::BuildContactPatch(terrain,footprint);
        Check(!patch.empty()&&patch.size()%3==0,"hut shadow patch missing triangles");
        shadowVertices+=patch.size();
        for(const auto& v:patch) {
            Check(std::abs(v.y-surface.Sample(v.x,v.z).height-island::ContactPatchSurfaceOffset)<0.00015f,
                "hut shadow not grounded on rendered triangles");
            Check(std::isfinite(v.opacity)&&v.opacity>=0&&v.opacity<=footprint.opacity+1e-6f,
                "hut shadow alpha outside bounds");
        }
        for(std::size_t i=0;i<patch.size();i+=3) {
            const float x=(patch[i].x+patch[i+1].x+patch[i+2].x)/3,
                z=(patch[i].z+patch[i+1].z+patch[i+2].z)/3,
                y=(patch[i].y+patch[i+1].y+patch[i+2].y)/3;
            Check(std::abs(y-surface.Sample(x,z).height-island::ContactPatchSurfaceOffset)<0.00015f,
                "hut shadow triangle crosses a terrain fold");
        }
    }
    Check(shadowVertices<=12000,"hut shadow exceeded fixed geometry budget");
    const std::array<island::CircularObstacle,1> hutSolid{{{found->x,found->z,island::HutLayout::CollisionRadius}}};
    Check(std::hypot(island::HutLayout::Width/2+0.08f,island::HutLayout::Depth/2+0.08f)
        <island::HutLayout::CollisionRadius,"hut posts extend outside collision proxy");
    for(int direction=0;direction<8;++direction) {
        const float a=static_cast<float>(direction)*0.785398163f,dx=std::cos(a),dz=std::sin(a);
        island::PlayerMotion player;
        player.SetPosition(terrain,found->x+dx*6,found->z+dz*6);
        for(int step=0;step<240;++step) {
            player.Update(terrain,-dx,-dz,1.0f/60,hutSolid);
            Check(std::hypot(player.Position().x-found->x,player.Position().z-found->z)
                >=island::HutLayout::CollisionRadius+island::PlayerMotion::CapsuleRadius-0.002f,
                "walking player entered hut solid");
        }
        Check(std::hypot(player.Position().x-found->x,player.Position().z-found->z)<4.0f,
            "hut approach test never reached building");
    }
    for(int ix=-30;ix<=30;++ix) for(int iz=-25;iz<=25;++iz) {
        const float u=static_cast<float>(ix)*0.08f,v=static_cast<float>(iz)*0.08f;
        const float x=found->x+c*u+s*v,z=found->z-s*u+c*v;
        Check(surface.Sample(x,z).height<found->floorY,"terrain clips hut floor");
    }
    const std::array<island::CircularObstacle,1> full{{{0,0,1000}}};
    Check(!island::HutLayout::Find(terrain,surface,full),"blocked world created overlapping hut");
    occupied.push_back({found->x,found->z,island::HutLayout::SiteClearance});
    const auto alternate=island::HutLayout::Find(terrain,surface,occupied);
    Check(alternate&&std::hypot(found->x-alternate->x,found->z-alternate->z)
        >=2*island::HutLayout::SiteClearance,"reserved site was not avoided");
    auto invalid=full; invalid[0].radius=std::numeric_limits<float>::quiet_NaN();
    bool rejected=false;
    try { (void)island::HutLayout::Find(terrain,surface,invalid); }
    catch(const std::invalid_argument&) { rejected=true; }
    Check(rejected,"invalid occupancy accepted");
    std::cout<<"Hut candidate "<<found->x<<','<<found->z<<" floor "<<found->floorY
        <<" yaw "<<found->yawRadians<<"; six grounded supports, eight collision approaches checked; "
        <<shadowVertices<<" shadow vertices\n";
}
