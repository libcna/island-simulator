#include "IslandSimulator/HelicopterLayout.hpp"

#include "IslandSimulator/IslandTerrain.hpp"
#include "IslandSimulator/JungleLayout.hpp"
#include "IslandSimulator/HelicopterSite.hpp"
#include "IslandSimulator/RenderedTerrainSurface.hpp"
#include "IslandSimulator/StreamCourse.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>
#include <stdexcept>
#include <vector>

namespace island {
namespace {
constexpr float PilotDoorLow=.18f,PilotDoorHigh=1.48f;
constexpr float RearDoorLow=-1.70f,RearDoorHigh=-.30f;

bool DoorBand(const float along) noexcept {
    return (along>=PilotDoorLow&&along<=PilotDoorHigh)
        ||(along>=RearDoorLow&&along<=RearDoorHigh);
}

bool CircleRectangle(const float x,const float z,const float radius,
                     const float lowX,const float highX,
                     const float lowZ,const float highZ) noexcept {
    const float nearestX=std::clamp(x,lowX,highX);
    const float nearestZ=std::clamp(z,lowZ,highZ);
    const float dx=x-nearestX,dz=z-nearestZ;
    return dx*dx+dz*dz<radius*radius;
}

bool CircleCircle(const float x,const float z,const float radius,
                  const float centreX,const float centreZ,const float otherRadius) noexcept {
    const float dx=x-centreX,dz=z-centreZ,combined=radius+otherRadius;
    return dx*dx+dz*dz<combined*combined;
}
}

HelicopterPoint HelicopterPlacement::World(const float across,const float height,
                                            const float along) const noexcept {
    const float c=std::cos(yawRadians),s=std::sin(yawRadians);
    return {x+c*across+s*along,groundY+height,z-s*across+c*along};
}

HelicopterPoint HelicopterPlacement::Local(const float worldX,const float worldY,
                                            const float worldZ) const noexcept {
    const float c=std::cos(yawRadians),s=std::sin(yawRadians);
    const float dx=worldX-x,dz=worldZ-z;
    return {c*dx-s*dz,worldY-groundY,s*dx+c*dz};
}

float HelicopterPlacement::SupportHeight(const float worldX,const float worldZ,
                                          const float terrainHeight) const noexcept {
    if(!std::isfinite(worldX)||!std::isfinite(worldZ)||!std::isfinite(terrainHeight))
        return terrainHeight;
    const auto p=Local(worldX,groundY,worldZ);
    if(std::abs(p.x)<=.79f&&p.z>=-1.78f&&p.z<=1.48f)
        return std::max(terrainHeight,floorY);
    const float across=std::abs(p.x);
    if(DoorBand(p.z)&&across>.79f&&across<=1.23f)
        return std::max(terrainHeight,
            floorY-.21f*(across-.79f)/(1.23f-.79f));
    if(DoorBand(p.z)&&across>1.23f&&across<=1.58f)
        return std::max(terrainHeight,floorY-.21f
            -.18f*(across-1.23f)/(1.58f-1.23f));
    return terrainHeight;
}

bool HelicopterPlacement::IntersectsCapsule(const float worldX,const float worldZ,
                                             const float radius) const noexcept {
    if(!std::isfinite(worldX)||!std::isfinite(worldZ)||!std::isfinite(radius)||radius<=0)
        return true;
    const auto p=Local(worldX,groundY,worldZ);
    if(std::abs(p.x)>2.0f+radius||p.z<HelicopterLayout::BodyRear-radius
        ||p.z>HelicopterLayout::BodyFront+radius) return false;
    // Closed nose, rear bulkhead and side-shell pieces leave two capsule-wide doorways
    // on each side. The cabin itself is intentionally not represented by one solid disc.
    if(CircleRectangle(p.x,p.z,radius,-1.02f,1.02f,1.49f,2.72f)
        ||CircleRectangle(p.x,p.z,radius,-1.02f,1.02f,-2.10f,-1.78f)
        ||CircleRectangle(p.x,p.z,radius,.78f,1.05f,-1.78f,RearDoorLow)
        ||CircleRectangle(p.x,p.z,radius,.78f,1.05f,RearDoorHigh,PilotDoorLow)
        ||CircleRectangle(p.x,p.z,radius,.78f,1.05f,PilotDoorHigh,1.49f)) return true;
    // Left side carries matching openings, separated by the centre post.
    if(CircleRectangle(p.x,p.z,radius,-1.05f,-.78f,-1.78f,RearDoorLow)
        ||CircleRectangle(p.x,p.z,radius,-1.05f,-.78f,RearDoorHigh,PilotDoorLow)
        ||CircleRectangle(p.x,p.z,radius,-1.05f,-.78f,PilotDoorHigh,1.49f)) return true;
    // Two cockpit seats and a rear bench are real interior obstacles; both footwells
    // and the open central rear floor remain accessible through their own side doors.
    if(CircleCircle(p.x,p.z,radius,-.46f,.38f,.25f)
        ||CircleCircle(p.x,p.z,radius,.46f,.38f,.25f)
        ||CircleRectangle(p.x,p.z,radius,-.70f,.70f,-1.72f,-1.43f)) return true;
    return false;
}

float HelicopterPlacement::SafeTravelFraction(const float fromX,const float fromZ,
                                               const float toX,const float toZ,
                                               const float radius) const noexcept {
    if(!std::isfinite(fromX)||!std::isfinite(fromZ)||!std::isfinite(toX)
        ||!std::isfinite(toZ)||!std::isfinite(radius)||radius<=0
        ||IntersectsCapsule(fromX,fromZ,radius)) return 0.0f;
    const float distance=std::hypot(toX-fromX,toZ-fromZ);
    if(distance>1000.0f)return 0.0f;
    const int steps=std::max(1,static_cast<int>(std::ceil(distance/.08f)));
    float previous=0.0f;
    for(int i=1;i<=steps;++i) {
        const float current=static_cast<float>(i)/static_cast<float>(steps);
        const float px=std::lerp(fromX,toX,current),pz=std::lerp(fromZ,toZ,current);
        if(!IntersectsCapsule(px,pz,radius)){previous=current;continue;}
        float low=previous,high=current;
        for(int pass=0;pass<12;++pass) {
            const float middle=(low+high)*.5f;
            if(IntersectsCapsule(std::lerp(fromX,toX,middle),std::lerp(fromZ,toZ,middle),radius))
                high=middle;
            else low=middle;
        }
        return low;
    }
    return 1.0f;
}

bool HelicopterPlacement::InPilotFootwell(const float worldX,const float worldZ) const noexcept {
    const auto p=Local(worldX,groundY,worldZ);
    return std::abs(p.x)<=.72f&&p.z>=.78f&&p.z<=1.18f;
}

bool HelicopterPlacement::InRearCabin(const float worldX,const float worldZ) const noexcept {
    const auto p=Local(worldX,groundY,worldZ);
    return std::abs(p.x)<=.72f&&p.z>=-1.28f&&p.z<=-.48f;
}

std::optional<HelicopterPlacement> HelicopterLayout::FindInWorld(
    const IslandTerrain& terrain,const JungleLayout& jungle,
    const std::span<const CircularObstacle> solidObstacles) {
    std::vector<CircularObstacle> foliage;
    foliage.reserve(jungle.Understory().size()+jungle.Bushes().size()+jungle.GroundPlants().size());
    for(const auto& p:jungle.Understory())foliage.push_back({p.x,p.z,p.scale*1.8f});
    for(const auto& p:jungle.Bushes())foliage.push_back({p.x,p.z,p.scale*1.2f});
    for(const auto& p:jungle.GroundPlants())foliage.push_back({p.x,p.z,p.scale*.5f});
    return Find(terrain,RenderedTerrainSurface(terrain),solidObstacles,foliage);
}

std::optional<HelicopterPlacement> HelicopterLayout::Find(
    const IslandTerrain& terrain,const RenderedTerrainSurface& surface,
    const std::span<const CircularObstacle> solidObstacles,
    const std::span<const CircularObstacle> lowFoliage) {
    const auto validate=[](const std::span<const CircularObstacle> obstacles) {
        for(const auto& o:obstacles)if(!std::isfinite(o.x)||!std::isfinite(o.z)
            ||!std::isfinite(o.radius)||o.radius<0)
            throw std::invalid_argument("Invalid occupied helicopter site");
    };
    validate(solidObstacles);validate(lowFoliage);
    const StreamCourse stream(terrain);
    std::optional<HelicopterPlacement> best;
    float bestScore=std::numeric_limits<float>::infinity();
    for(int ring=0;ring<12;++ring)for(int sector=0;sector<144;++sector) {
        const float radius=MinimumCentreRadius+static_cast<float>(ring)*6.0f;
        if(radius>MaximumCentreRadius)continue;
        const float angle=.19f+2.0f*std::numbers::pi_v<float>*static_cast<float>(sector)/144.0f;
        const float x=radius*std::cos(angle),z=radius*std::sin(angle);
        const auto sample=terrain.Sample(x,z);
        if(sample.signedShoreDistance<MinimumShoreDistance||sample.height<2.0f
            ||sample.normal.y<.985f)continue;
        float streamDistance=std::numeric_limits<float>::infinity();
        for(std::size_t i=0;i<stream.Count();++i)streamDistance=std::min(streamDistance,
            std::hypot(x-stream.Station(i).x,z-stream.Station(i).z));
        if(streamDistance<18.0f)continue;
        bool blocked=false;
        for(const auto& o:solidObstacles)if(std::hypot(x-o.x,z-o.z)<SiteClearance+o.radius) {
            blocked=true;break;
        }
        if(blocked)continue;
        for(const auto& o:lowFoliage)if(std::hypot(x-o.x,z-o.z)<1.35f+o.radius) {
            blocked=true;break;
        }
        if(blocked)continue;
        const float yaw=angle+std::numbers::pi_v<float>*.5f;
        const float c=std::cos(yaw),s=std::sin(yaw);
        const float halfX=std::abs(c)*1.55f+std::abs(s)*3.05f;
        const float halfZ=std::abs(s)*1.55f+std::abs(c)*3.05f;
        const auto heights=surface.HeightRange(x-halfX,z-halfZ,x+halfX,z+halfZ);
        const float relief=heights[1]-heights[0];
        if(relief>MaximumRelief)continue;
        const float preferredDistance=std::hypot(x-HelicopterSite::X,z-HelicopterSite::Z);
        const float score=preferredDistance*100.0f+relief*22.0f+radius*.018f
            +streamDistance*.002f;
        if(score>=bestScore)continue;
        HelicopterPlacement candidate{x,z,yaw,heights[1],heights[1]+FloorClearance,{}};
        std::size_t contact=0;
        for(const float across:{-1.24f,1.24f})for(const float along:{-1.65f,1.50f}) {
            const auto p=candidate.World(across,0.0f,along);
            candidate.skidContacts[contact++]={p.x,surface.Sample(p.x,p.z).height,p.z};
        }
        best=candidate;bestScore=score;
    }
    return best;
}
}
