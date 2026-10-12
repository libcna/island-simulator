#pragma once
#include "IslandSimulator/SpiderHabitat.hpp"
#include "IslandSimulator/SpiderPose.hpp"
#include "IslandSimulator/WildlifeSimulation.hpp"
#include <cstdint>

namespace island {
struct SpiderGroundPose {
    SpiderPoint origin{},right{},up{},forward{};
    std::array<SpiderLegPose,8> legs{};
    std::array<SpiderPoint,8> feet{};
    std::array<bool,8> stance{};
    [[nodiscard]] SpiderPoint ToWorld(SpiderPoint local) const noexcept;
    [[nodiscard]] SpiderPoint ToLocal(SpiderPoint world) const noexcept;
    bool operator==(const SpiderGroundPose&) const = default;
};
enum class SpiderBehavior { Rest,Crawl,Retreat };
struct SpiderState {
    float x=0,y=0,z=0,heading=0,speed=0;
    SpiderBehavior behavior=SpiderBehavior::Rest;
    WildlifeSimulationTier tier=WildlifeSimulationTier::Dormant;
    SpiderGroundPose pose{};
    bool operator==(const SpiderState&) const = default;
};
// Ground population only. Web occupants and anchors remain a separate habitat.
class SpiderColony final {
public:
    static constexpr float CrawlSpeed=.035f,RetreatSpeed=.065f;
    SpiderColony(const IslandTerrain& terrain,const JungleLayout& jungle,
        std::span<const CircularObstacle> obstacles);
    void Update(float dt,SpiderPoint observer);
    [[nodiscard]] std::span<const SpiderState> Spiders() const noexcept {return states_;}
    [[nodiscard]] std::span<const SpiderGroundHome> Homes() const noexcept {return homes_;}
private:
    struct Foot {SpiderPoint position{},start{},target{};float progress=0;bool swinging=false;};
    using Feet=std::array<Foot,8>;
    struct Intent {float remaining=0,heading=0;std::uint32_t decisions=0;};
    [[nodiscard]] SpiderGroundPose Frame(float x,float z,float heading) const;
    [[nodiscard]] SpiderGroundPose Pose(const SpiderState& state,const Feet& feet) const;
    [[nodiscard]] bool Valid(const SpiderGroundPose& pose) const;
    void AdvanceFeet(std::size_t index,float dt);
    void Step(std::size_t index,float dt,SpiderPoint observer,bool observerValid);
    RenderedTerrainSurface surface_;
    std::array<SpiderGroundHome,SpiderHabitat::GroundCount> homes_{};
    std::array<SpiderState,SpiderHabitat::GroundCount> states_{};
    std::array<Feet,SpiderHabitat::GroundCount> feet_{};
    std::array<Intent,SpiderHabitat::GroundCount> intents_{};
    std::array<float,SpiderHabitat::GroundCount> accumulatedSeconds_{};
    std::uint64_t frameIndex_=0;
};
}
