#include "IslandSimulator/SnakePopulation.hpp"
#include <algorithm>
#include <cmath>
#include <numbers>
#include <stdexcept>

namespace island
{
    float SnakePopulation::RadiusAt(const std::size_t station) noexcept
    {
        if (station == 0) return 0.043f;
        const float t = static_cast<float>(std::min(station, SnakeState::StationCount-1))
            / static_cast<float>(SnakeState::StationCount-1);
        return 0.004f + 0.044f * std::pow(std::max(0.0f,
            std::sin(std::numbers::pi_v<float> * t)), 0.45f);
    }

    bool SnakePopulation::ValidPoint(const float x, const float z, const float radius) const
    {
        if (!std::isfinite(x) || !std::isfinite(z) || std::abs(x) > 300.0f || std::abs(z) > 300.0f)
            return false;
        const auto ground = surface_->Sample(x,z);
        return terrain_.SignedShoreDistance(x,z) >= 20.0f && ground.height >= 1.0f
            && ground.normal.y >= 0.913089f && !obstacles_.IntersectsCircle(x,z,radius);
    }

    float SnakePopulation::GroundedY(const float x, const float z, const float radius) const
    {
        const float height = std::max({surface_->Sample(x,z).height,
            surface_->Sample(x-radius,z).height, surface_->Sample(x+radius,z).height,
            surface_->Sample(x,z-radius).height, surface_->Sample(x,z+radius).height});
        return height + radius + 0.004f;
    }

    bool SnakePopulation::ValidBody(const SnakeState& state) const
    {
        for (std::size_t i = 0; i < state.body.size(); ++i) {
            const auto& p = state.body[i];
            if (!std::isfinite(p.y) || !ValidPoint(p.x,p.z,MaximumRadius)
                || std::hypot(p.x-state.homeX,p.z-state.homeZ) > HomeRadius
                || p.y < surface_->Sample(p.x,p.z).height + RadiusAt(i)) return false;
            if (i != 0) {
                const auto& q = state.body[i-1];
                const float length = std::hypot(p.x-q.x,p.z-q.z);
                // A containing disc proves clearance of the whole short horizontal segment.
                if (std::abs(length-Spacing) > Spacing * 0.03f
                    || !ValidPoint((p.x+q.x)*0.5f,(p.z+q.z)*0.5f,MaximumRadius+length*0.5f))
                    return false;
            }
            for (std::size_t j=i+4; j<state.body.size(); ++j) {
                const auto& q = state.body[j];
                if (std::hypot(p.x-q.x,p.z-q.z) < RadiusAt(i)+RadiusAt(j)) return false;
            }
        }
        return true;
    }

    SnakePopulation::SnakePopulation(const IslandTerrain& terrain,
        const std::span<const CircularObstacle> obstacles)
        : terrain_(terrain), obstacles_(obstacles), surface_(std::make_unique<RenderedTerrainSurface>(terrain))
    {
        constexpr std::array<std::array<float,2>,Count> homes{{{-44,56},{70,-38},{-72,-70},{42,64}}};
        for (std::size_t index=0; index<Count; ++index) {
            bool found = false;
            auto& state = states_[index];
            for (int attempt=0; attempt<512 && !found; ++attempt) {
                const float angle = static_cast<float>(attempt)*2.399963f;
                const float radius = std::sqrt(static_cast<float>(attempt))*0.7f;
                state.homeX = homes[index][0]+radius*std::cos(angle);
                state.homeZ = homes[index][1]+radius*std::sin(angle);
                for (std::size_t j=0; j<state.body.size(); ++j) {
                    const float x = state.homeX-static_cast<float>(j)*Spacing;
                    const float z = state.homeZ;
                    state.body[j] = {x,GroundedY(x,z,RadiusAt(j)),z};
                }
                found = ValidBody(state);
            }
            if (!found) throw std::runtime_error("No safe whole-body ground snake habitat");
            clocks_[index] = static_cast<float>(index)*4.3f;
        }
    }

    void SnakePopulation::Update(const float deltaSeconds, const float observerX,
        const float observerY, const float observerZ)
    {
        if (!std::isfinite(deltaSeconds) || deltaSeconds <= 0.0f) return;
        const float dt = std::min(deltaSeconds,0.1f);
        const bool observerValid = std::isfinite(observerX) && std::isfinite(observerY) && std::isfinite(observerZ);
        for (std::size_t index=0; index<Count; ++index) {
            auto& state = states_[index];
            const auto head = state.body.front();
            const float distance = observerValid ? std::hypot(std::hypot(head.x-observerX,
                head.z-observerZ),head.y-observerY) : 1000.0f;
            state.tier = WildlifeActivationPolicy::Classify(distance,65.0f,120.0f);
            state.speed = 0.0f;
            if (state.tier == WildlifeSimulationTier::Dormant) continue;
            clocks_[index] = std::fmod(clocks_[index]+dt,60.0f);
            const bool retreat = distance < 8.0f;
            state.behavior = retreat ? SnakeBehavior::Retreat
                : clocks_[index] < 9.0f ? SnakeBehavior::Rest : SnakeBehavior::Explore;
            if (state.behavior == SnakeBehavior::Rest) continue;
            float target = 0.7f*std::sin(clocks_[index]*0.17f+static_cast<float>(index));
            if (std::hypot(head.x-state.homeX,head.z-state.homeZ) > HomeRadius-3.0f)
                target = std::atan2(state.homeZ-head.z,state.homeX-head.x);
            if (retreat) target = std::atan2(head.z-observerZ,head.x-observerX);
            constexpr float tau = 2.0f*std::numbers::pi_v<float>;
            state.heading = std::remainder(state.heading + std::clamp(
                std::remainder(target-state.heading,tau),-1.8f*dt,1.8f*dt),tau);
            const float speed = retreat ? 1.15f : 0.38f;
            const int steps = static_cast<int>(std::ceil(speed*dt/0.012f));
            float traveled = 0.0f;
            for (int step=0; step<steps; ++step) {
                auto candidate = state;
                const float advance = speed*dt/static_cast<float>(steps);
                const float direction = state.heading + 0.35f*std::sin(state.phase);
                auto& nextHead = candidate.body.front();
                nextHead.x += std::cos(direction)*advance;
                nextHead.z += std::sin(direction)*advance;
                if (!ValidPoint(nextHead.x,nextHead.z,MaximumRadius)) {
                    state.heading = std::remainder(state.heading+0.25f,tau);
                    break;
                }
                if (retreat && std::hypot(nextHead.x-observerX,nextHead.z-observerZ)
                        < std::hypot(state.body[0].x-observerX,state.body[0].z-observerZ)) break;
                nextHead.y = GroundedY(nextHead.x,nextHead.z,RadiusAt(0));
                for (std::size_t j=1; j<candidate.body.size(); ++j) {
                    auto& p = candidate.body[j];
                    const auto& leader = candidate.body[j-1];
                    const float dx = p.x-leader.x, dz = p.z-leader.z;
                    const float length = std::hypot(dx,dz);
                    if (length > 0.000001f) {
                        p.x = leader.x+dx*Spacing/length;
                        p.z = leader.z+dz*Spacing/length;
                    }
                    p.y = GroundedY(p.x,p.z,RadiusAt(j));
                }
                if (!ValidBody(candidate)) {
                    state.heading = std::remainder(state.heading+0.25f,tau);
                    break;
                }
                state.body = candidate.body;
                state.phase = std::fmod(state.phase+advance*8.0f,tau);
                traveled += advance;
            }
            state.speed = traveled/dt;
        }
    }
}
