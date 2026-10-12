#pragma once

#include "IslandSimulator/RenderedTerrainSurface.hpp"
#include "IslandSimulator/WorldCollisionGrid.hpp"

#include <array>
#include <span>

namespace island {
class IslandTerrain;

struct BearPoint {float x=0,y=0,z=0;bool operator==(const BearPoint&)const=default;};
struct BearLegPose {
    static constexpr float UpperLength=.48f,LowerLength=.46f,PawHeight=.09f;
    BearPoint root{},joint{},paw{},soleNormal{0,1,0};
    bool stance=true,reachLimited=false;
    bool operator==(const BearLegPose&)const=default;
};
struct BearPose {
    BearPoint origin{},right{1,0,0},up{0,1,0},forward{0,0,1};
    float bodyBob=0;
    std::array<BearLegPose,4> legs{}; // left rear, left front, right rear, right front
    [[nodiscard]] BearPoint ToWorld(BearPoint local)const noexcept;
    [[nodiscard]] BearPoint ToLocal(BearPoint world)const noexcept;
    bool operator==(const BearPose&)const=default;
};
enum class BearBehavior {Idle,Walk,Trot,Forage,Rest,Withdraw};
struct BearState {
    float x=0,y=0,z=0,homeX=0,homeZ=0,heading=0,speed=0,phase=0;
    float foragePose=0,breathPhase=0,distanceTravelled=0;
    BearBehavior behavior=BearBehavior::Idle;
    BearPose pose{};
    bool operator==(const BearState&)const=default;
};

/** Exactly one peaceful heavy quadruped with world-locked terrain contact. */
class BearMotion final {
public:
    static constexpr float HomeRadius=34.0f,BodyClearance=1.55f,MaximumSpeed=1.65f;
    BearMotion(const IslandTerrain& terrain,std::span<const CircularObstacle> obstacles);
    void Update(float seconds,BearPoint observer);
    [[nodiscard]] const BearState& State()const noexcept{return state_;}
    [[nodiscard]] bool IsValidPosition(float x,float z)const noexcept;
    [[nodiscard]] const RenderedTerrainSurface& Ground()const noexcept{return ground_;}
private:
    struct Foot {BearPoint position{},start{},target{};float progress=1,duration=.34f;bool swinging=false;};
    [[nodiscard]] bool AcceptsGround(float x,float z)const noexcept;
    [[nodiscard]] BearPose BodyFrame(const BearState& state)const;
    [[nodiscard]] BearPose ContactPose(const BearState& state,const std::array<Foot,4>& feet)const;
    [[nodiscard]] BearPoint NeutralFoot(const BearState& state,std::size_t leg,float ahead=0)const;
    void Step(float seconds,BearPoint observer,bool observerValid);
    void AdvanceFeet(float seconds);
    bool TryMove(float heading,float distance,bool& waitingForStep);
    void StartFoot(std::size_t leg,BearPoint target,float duration);
    const IslandTerrain& terrain_;
    RenderedTerrainSurface ground_;
    WorldCollisionGrid solids_;
    BearState state_;
    std::array<Foot,4> feet_{};
    float remaining_=2.5f,targetHeading_=0,avoidance_=0;
    std::uint32_t decisions_=0;
};
}
