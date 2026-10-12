#pragma once
#include "IslandSimulator/GoatHabitat.hpp"
#include "IslandSimulator/GoatPose.hpp"

namespace island {
enum class GoatBehavior { Graze,Walk,Rest,Retreat };
struct GoatState {
    float x=0,y=0,z=0,heading=0,speed=0,phase=0,grazingPose=0;
    float scale=1;
    GoatBehavior behavior=GoatBehavior::Graze;
    GoatGroundPose pose{}; // Authoritative planted-foot pose shared with rendering.
    bool operator==(const GoatState&) const = default;
};
// Deterministic peaceful herd; simulation remains independent of the renderer.
class GoatHerd final {
public:
    static constexpr float HomeRadius=14,MaximumSpeed=1.2f;
    GoatHerd(const IslandTerrain& terrain,const JungleLayout& jungle,
        std::span<const CircularObstacle> obstacles);
    void Update(float dt,GoatPoint observer);
    [[nodiscard]] std::span<const GoatState> Goats() const noexcept { return states_; }
    [[nodiscard]] bool IsValidPosition(std::size_t index,float x,float z) const;
private:
    void Step(std::size_t index,float dt,GoatPoint observer,bool observerValid);
    bool TryMove(std::size_t index,float heading,float distance,bool& waitingForStep);
    struct Foot { GoatPoint position{},start{},target{};float progress=0;bool swinging=false; };
    using Feet=std::array<Foot,4>;
    [[nodiscard]] GoatGroundPose ContactPose(const GoatState& state,const Feet& feet) const;
    void AdvanceFeet(std::size_t index,float dt);
    const IslandTerrain& terrain_;
    RenderedTerrainSurface surface_;
    WorldCollisionGrid clearance_;
    std::array<GoatHome,GoatHabitat::Count> homes_{};
    std::array<GoatState,GoatHabitat::Count> states_{};
    std::array<Feet,GoatHabitat::Count> feet_{};
    struct Intent { float remaining=0,targetHeading=0,avoidance=0; std::uint32_t decisions=0; };
    std::array<Intent,GoatHabitat::Count> intents_{};
};
}
