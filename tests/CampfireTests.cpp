#include "IslandSimulator/CampfireLayout.hpp"
#include "IslandSimulator/VegetationLayout.hpp"
#include "IslandSimulator/JungleLayout.hpp"
#include "IslandSimulator/NaturalPropLayout.hpp"
#include "IslandSimulator/StreamSoundField.hpp"
#include "IslandSimulator/IslandEnvironmentField.hpp"
#include "IslandSimulator/AudioAnalysis.hpp"
#include "IslandSimulator/PlayerMotion.hpp"
#include "IslandSimulator/WorldCollisionGrid.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>
namespace {void Check(bool value,const char* message){if(!value)throw std::runtime_error(message);}}
int main() {
    using namespace island;
    const IslandTerrain terrain;
    const CampfireLayout fire(terrain),repeat(terrain);
    const RenderedTerrainSurface ground(terrain);
    const auto centre=fire.Centre(),wood=fire.WoodCentre();
    for(const auto point:{centre,wood}) {
        const auto sample=terrain.Sample(point.x,point.z);
        Check(sample.surface==TerrainSurface::DrySand
            &&sample.signedShoreDistance>=7&&sample.signedShoreDistance<=23
            &&sample.beachSuitability>.70f&&point.y>.25f,
            "fire or stored wood is not on qualified dry beach sand");
    }
    Check(std::hypot(wood.x-centre.x,wood.z-centre.z)>5.5f,"stored firewood is too close to flames");
    Check(StreamSoundField::Sample(centre.x,centre.z).distance>58,"fire/stream audible regions overlap");
    Check(fire.Vertices().size()==5976&&fire.Vertices().size()==repeat.Vertices().size(),"fire geometry budget changed");
    std::array<int,7> materials{};
    for(std::size_t i=0;i<fire.Vertices().size();i+=3) {
        const auto a=fire.Vertices()[i],b=fire.Vertices()[i+1],c=fire.Vertices()[i+2];
        const auto p=a.position,q=b.position,r=c.position;
        const float nx=(q.y-p.y)*(r.z-p.z)-(q.z-p.z)*(r.y-p.y);
        const float ny=(q.z-p.z)*(r.x-p.x)-(q.x-p.x)*(r.z-p.z);
        const float nz=(q.x-p.x)*(r.y-p.y)-(q.y-p.y)*(r.x-p.x);
        Check(nx*a.normal.x+ny*a.normal.y+nz*a.normal.z>1e-9f,"fire winding or normal is invalid");
        ++materials[static_cast<int>(a.material)];
    }
    for(std::size_t i=0;i<fire.Vertices().size();++i) {
        const auto v=fire.Vertices()[i],r=repeat.Vertices()[i];
        Check(std::isfinite(v.position.x)&&std::isfinite(v.position.y)&&std::isfinite(v.position.z)
            &&std::isfinite(v.u)&&std::isfinite(v.v),"nonfinite fire mesh");
        Check(std::abs(std::hypot(v.normal.x,v.normal.y,v.normal.z)-1)<1e-5f,"fire normal not unit");
        Check(v.position.x==r.position.x&&v.position.y==r.position.y&&v.position.z==r.position.z,
            "fire geometry is not deterministic");
        if(v.material==FireMaterial::Ash)
            Check(std::abs(v.position.y-ground.Sample(v.position.x,v.position.z).height-.012f)<.00002f,
                "ash border is not grounded on the emitted terrain");
        if(v.material==FireMaterial::Firewood||v.material==FireMaterial::EndGrain)
            Check(std::hypot(v.position.x-wood.x,v.position.z-wood.z)<CampfireSite::WoodRadius,
                "woodpile collision does not contain its mesh");
        else Check(std::hypot(v.position.x-centre.x,v.position.z-centre.z)<CampfireSite::PitRadius,
                "firepit collision does not contain its mesh");
    }
    Check(std::all_of(materials.begin(),materials.end(),[](int n){return n>0;}),"missing fire material group");
    Check(materials[static_cast<int>(FireMaterial::Ember)]==240
        &&materials[static_cast<int>(FireMaterial::Coal)]==360,"ember/charcoal split changed");
    const WorldCollisionGrid fireSolids(fire.Obstacles());
    Check(!fireSolids.IntersectsCircle(VegetationLayout::PlayerSpawnX,
                                      VegetationLayout::PlayerSpawnZ,
                                      PlayerMotion::CapsuleRadius)
              && !fireSolids.IntersectsCircle(VegetationLayout::PlayerSpawnX,
                                              VegetationLayout::PlayerSpawnZ + 1.0f,
                                              PlayerMotion::CapsuleRadius),
          "beach campfire or woodpile blocks the player spawn corridor");
    for(const auto obstacle:fire.Obstacles()) {
        PlayerMotion player;
        player.SetPosition(terrain,obstacle.x-4,obstacle.z);
        for(int step=0;step<100;++step)player.Update(terrain,1,0,.1f,fireSolids);
        const float remaining=obstacle.x-player.Position().x;
        Check(remaining>=obstacle.radius+PlayerMotion::CapsuleRadius-.001f
            &&remaining<obstacle.radius+PlayerMotion::CapsuleRadius+.2f,
            "player cannot approach safely or walks through fire/woodpile collision");
    }
    const VegetationLayout palms(terrain);
    const JungleLayout jungle(terrain);
    const NaturalPropLayout props(terrain,palms,jungle);
    for(const auto& p:palms.Palms())Check(!CampfireSite::Reserves(p.x,p.z,3*p.scale),"palm crown too close to fire");
    for(const auto& p:jungle.Canopy())Check(!CampfireSite::Reserves(p.x,p.z,5*p.scale),"tree crown too close to fire");
    for(const auto& p:jungle.Understory())Check(!CampfireSite::Reserves(p.x,p.z,1.8f*p.scale),"understory too close to fire");
    for(const auto& p:props.Obstacles()) {
        Check(std::hypot(p.x-centre.x,p.z-centre.z)>p.radius+CampfireSite::PitRadius,"fire intersects a prop");
        Check(std::hypot(p.x-wood.x,p.z-wood.z)>p.radius+CampfireSite::WoodRadius,"woodpile intersects a prop");
    }
    Check(CampfireSite::SoundPresence(centre.x,centre.y,centre.z,centre.y)==1
        &&CampfireSite::SoundPresence(centre.x+24,centre.y,centre.z,centre.y)==0
        &&CampfireSite::SoundPresence(centre.x,centre.y+25,centre.z,centre.y)==0
        &&CampfireSite::SoundPresence(NAN,0,0,0)==0,"fire sound range is not bounded in 3D");
    const auto audio=GenerateCampfireLoop(),second=GenerateCampfireLoop();
    const auto stats=AnalyzeAudioSignal(audio);
    Check(audio.samples==second.samples&&stats.sampleCount==418950&&stats.sampleRate==22050
        &&std::abs(stats.mean)<.0001&&stats.rootMeanSquare>.015&&stats.peak>.6&&stats.peak<.7
        &&stats.seamStep<.25,"campfire crackle PCM is invalid or not deterministic/seamless");
    std::cout<<"campfire centre "<<centre.x<<','<<centre.y<<','<<centre.z<<" wood "<<wood.x<<','<<wood.y<<','<<wood.z
        <<" triangles "<<fire.Vertices().size()/3<<"; ground, materials, clearance and collision passed\n";
}
