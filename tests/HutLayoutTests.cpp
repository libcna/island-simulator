#include "IslandSimulator/HutLayout.hpp"
#include "IslandSimulator/IslandTerrain.hpp"
#include "IslandSimulator/RenderedTerrainSurface.hpp"
#include "IslandSimulator/VegetationLayout.hpp"
#include "IslandSimulator/JungleLayout.hpp"
#include "IslandSimulator/NaturalPropLayout.hpp"
#include "IslandSimulator/ShipwreckLayout.hpp"
#include "IslandSimulator/PlayerMotion.hpp"
#include "IslandSimulator/ContactShadowPatch.hpp"
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>

static void Check(bool value,const char* message) { if(!value) throw std::runtime_error(message); }
int main() {
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
    for(const auto& p:jungle.Understory()) occupied.push_back({p.x,p.z,p.scale*1.8f});
    for(const auto& p:jungle.Bushes()) occupied.push_back({p.x,p.z,p.scale*1.2f});
    for(const auto& p:jungle.GroundPlants()) occupied.push_back({p.x,p.z,p.scale*0.5f});
    const auto found=island::HutLayout::Find(terrain,surface,occupied);
    Check(found.has_value(),"no hut site in current world");
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
