#pragma once
#include "IslandSimulator/ResidentRoute.hpp"
#include <array>
namespace island {
struct ResidentFootFrame {
    ResidentPoint centre{},right{1,0,0},up{0,1,0},forward{0,0,1};
    float heading=0;
    bool operator==(const ResidentFootFrame&)const=default;
};
struct ResidentLegPose {
    static constexpr float UpperLength=.43f,LowerLength=.44f,AnkleHeight=.065f;
    ResidentPoint hip{},knee{},ankle{};
    ResidentFootFrame foot;
    bool stance=true,reachLimited=false;
    bool operator==(const ResidentLegPose&)const=default;
};
[[nodiscard]] ResidentLegPose SolveResidentLeg(ResidentPoint hip,const ResidentFootFrame& foot,
    ResidentPoint bendForward,bool stance);
struct ResidentPose {
    ResidentPoint origin{};
    float pelvisY=0,heading=0;
    std::array<ResidentLegPose,2> legs{};
    std::array<ResidentPoint,2> shoulders{},elbows{},wrists{};
    bool operator==(const ResidentPose&)const=default;
};
enum class ResidentMode {Walking,Pausing,Turning,Yielding};
struct ResidentState {
    float x=0,y=0,z=0,heading=0,speed=0,phase=0;
    float distanceTravelled=0;
    std::size_t nextPoint=1,completedLaps=0,pauseCount=0,rejectedMoves=0;
    ResidentMode mode=ResidentMode::Pausing;
    ResidentPose pose;
    bool operator==(const ResidentState&)const=default;
};
// Exactly one peaceful adult. This CPU controller does not install a visible character;
// rendering and model provenance must be integrated before claiming a resident in the game.
class ResidentMotion final {
public:
    static constexpr float WalkingSpeed=.95f,SwingSeconds=.45f;
    ResidentMotion(const IslandTerrain& terrain,std::span<const CircularObstacle> solids);
    void Update(float seconds,ResidentPoint observer);
    [[nodiscard]] const ResidentState& State()const noexcept {return state_;}
    [[nodiscard]] const ResidentRoute& Route()const noexcept {return route_;}
    [[nodiscard]] const RenderedTerrainSurface& Ground()const noexcept {return ground_;}
private:
    struct Foot {
        ResidentFootFrame current,start,target;
        float progress=1,duration=SwingSeconds;
        bool swinging=false;
    };
    [[nodiscard]] ResidentFootFrame GroundFoot(float x,float z,float heading)const;
    [[nodiscard]] bool FitPose(float x,float z,float heading,float speed,ResidentPose& pose)const;
    void Step(float seconds,ResidentPoint observer);
    void AdvanceFeet(float seconds);
    void StartStep(float targetX,float targetZ,float heading,std::size_t leg,float duration);
    const IslandTerrain& terrain_;
    RenderedTerrainSurface ground_;
    WorldCollisionGrid solids_;
    ResidentRoute route_;
    std::array<Foot,2> feet_{};
    ResidentState state_;
    float pauseRemaining_=3;
};
}
