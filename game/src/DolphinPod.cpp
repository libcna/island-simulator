#include "IslandSimulator/DolphinPod.hpp"

#include "IslandSimulator/IslandTerrain.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <limits>
#include <numbers>

namespace island {
namespace {
constexpr float TwoPi = 2 * std::numbers::pi_v<float>;
constexpr std::array<float, DolphinPod::DolphinCount> HomeAngles{
    -2.48f, -2.34f, -2.62f, -2.20f};

float Wrap(float value) noexcept {
  while (value > std::numbers::pi_v<float>)
    value -= TwoPi;
  while (value < -std::numbers::pi_v<float>)
    value += TwoPi;
  return value;
}

float MoveAngle(float current, float target, float maximum) noexcept {
  return Wrap(current + std::clamp(Wrap(target - current), -maximum, maximum));
}
} // namespace

DolphinPod::DolphinPod(const IslandTerrain &terrain) : terrain_(terrain) {
  agents_.resize(DolphinCount);
  states_.resize(DolphinCount);
  homes_.resize(DolphinCount);
  for (std::size_t i = 0; i < DolphinCount; ++i) {
    Initialize(i);
    states_[i] = agents_[i].state;
    homes_[i] = agents_[i].home;
  }
}

void DolphinPod::Initialize(const std::size_t index) {
  auto &agent = agents_[index];
  const float angle = HomeAngles[index];
  float offshore = 58.0f + static_cast<float>(index % 2) * 7.0f;
  for (int attempt = 0; attempt < 12; ++attempt) {
    const float radius = terrain_.ShoreRadius(angle) + offshore;
    agent.home.x = std::cos(angle) * radius;
    agent.home.z = std::sin(angle) * radius;
    const auto sample = terrain_.Sample(agent.home.x, agent.home.z);
    if (sample.signedShoreDistance >= MinimumShoreDistance &&
        sample.signedShoreDistance <= MaximumShoreDistance &&
        sample.height <= -3.0f)
      break;
    offshore += 3.0f;
  }
  agent.home.radius = 17.0f + static_cast<float>(index % 3) * 2.0f;
  agent.routePhase = static_cast<float>(index) * TwoPi / DolphinCount + .35f;
  agent.depthPhase = static_cast<float>(index) * 1.37f + .4f;
  agent.tailPhase = static_cast<float>(index) * 1.91f;
  agent.state.x = agent.home.x + std::cos(agent.routePhase) * 5.5f;
  agent.state.z = agent.home.z + std::sin(agent.routePhase) * 5.5f;
  if (!ValidHabitat(agent.state.x, agent.state.z, agent.home)) {
    agent.state.x = agent.home.x;
    agent.state.z = agent.home.z;
  }
  agent.state.headingRadians =
      Wrap(agent.routePhase + std::numbers::pi_v<float> * .5f);
  agent.state.scale = .92f + static_cast<float>(index) * .055f;
  agent.state.colorVariant = static_cast<std::uint8_t>(index % 3);
  agent.state.behavior = index == 1 ? DolphinBehavior::Surface
                                    : (index == 3 ? DolphinBehavior::Dive
                                                  : DolphinBehavior::Cruise);
  agent.behaviorSeconds = 4.0f + static_cast<float>(index) * 2.7f;
  agent.state.speed =
      agent.state.behavior == DolphinBehavior::Cruise ? 2.15f : 1.55f;
  agent.state.y =
      DesiredDepth(agent, terrain_.Height(agent.state.x, agent.state.z));
  agent.state.tailBeat = std::sin(agent.tailPhase);
}

bool DolphinPod::ValidHabitat(const float x, const float z,
                              const DolphinHomeRange &home) const noexcept {
  const auto sample = terrain_.Sample(x, z);
  return sample.signedShoreDistance >= MinimumShoreDistance &&
         sample.signedShoreDistance <= MaximumShoreDistance &&
         sample.height <= -3.0f &&
         std::hypot(x - home.x, z - home.z) <= home.radius;
}

float DolphinPod::DesiredDepth(const Agent &agent,
                               const float seabed) const noexcept {
  const float lower = seabed + 1.25f * agent.state.scale;
  constexpr float surface = -.24f;
  if (agent.state.behavior == DolphinBehavior::Surface)
    return std::max(lower, surface);
  if (agent.state.behavior == DolphinBehavior::Dive)
    return std::max(lower, -4.6f);
  const float cruise =
      -1.75f + .32f * std::sin(elapsedSeconds_ * .24f + agent.depthPhase);
  return std::max(lower, std::min(surface, cruise));
}

void DolphinPod::Update(const float deltaSeconds, const float observerX,
                        const float observerY, const float observerZ) noexcept {
  const float delta =
      std::isfinite(deltaSeconds) ? std::clamp(deltaSeconds, 0.0f, .1f) : 0.0f;
  const bool observerValid = std::isfinite(observerX) &&
                             std::isfinite(observerY) &&
                             std::isfinite(observerZ);
  elapsedSeconds_ += delta;
  ++frameIndex_;
  fullCount_ = reducedCount_ = dormantCount_ = 0;
  for (std::size_t i = 0; i < agents_.size(); ++i) {
    auto &agent = agents_[i];
    const float distance =
        observerValid ? std::hypot(agent.state.y - observerY,
                                   std::hypot(agent.state.x - observerX,
                                              agent.state.z - observerZ))
                      : std::numeric_limits<float>::infinity();
    agent.state.simulationTier = WildlifeActivationPolicy::Classify(
        distance, FullSimulationDistance, ReducedSimulationDistance);
    switch (agent.state.simulationTier) {
    case WildlifeSimulationTier::Full:
      ++fullCount_;
      break;
    case WildlifeSimulationTier::Reduced:
      ++reducedCount_;
      break;
    case WildlifeSimulationTier::Dormant:
      ++dormantCount_;
      break;
    }
    agent.accumulatedSeconds += delta;
    const auto divisor =
        WildlifeActivationPolicy::UpdateDivisor(agent.state.simulationTier);
    if ((frameIndex_ + i) % divisor == 0) {
      UpdateAgent(agent, std::min(agent.accumulatedSeconds, .5f));
      agent.accumulatedSeconds = 0;
    }
    states_[i] = agent.state;
  }
#ifndef NDEBUG
  assert(fullCount_ + reducedCount_ + dormantCount_ ==
         static_cast<int>(DolphinCount));
  for (const auto &dolphin : states_) {
    assert(std::isfinite(dolphin.x) && std::isfinite(dolphin.y) &&
           std::isfinite(dolphin.z));
    assert(std::isfinite(dolphin.headingRadians) &&
           std::isfinite(dolphin.pitchRadians));
    assert(std::fabs(dolphin.pitchRadians) <= .361f &&
           std::fabs(dolphin.bankRadians) <= .291f);
    assert(std::fabs(dolphin.tailBeat) <= 1.001f && dolphin.speed >= 0 &&
           dolphin.scale > 0);
  }
#endif
}

void DolphinPod::UpdateAgent(Agent &agent, const float delta) noexcept {
  if (delta <= 0)
    return;
  agent.behaviorSeconds -= delta;
  if (agent.behaviorSeconds <= 0) {
    ++agent.transitionCount;
    if (agent.state.behavior == DolphinBehavior::Cruise)
      agent.state.behavior = agent.transitionCount % 3 == 0
                                 ? DolphinBehavior::Dive
                                 : DolphinBehavior::Surface;
    else
      agent.state.behavior = DolphinBehavior::Cruise;
    agent.behaviorSeconds = agent.state.behavior == DolphinBehavior::Surface
                                ? 3.2f + .45f * (agent.transitionCount % 3)
                            : agent.state.behavior == DolphinBehavior::Dive
                                ? 5.0f + .7f * (agent.transitionCount % 4)
                                : 11.0f + 1.3f * (agent.transitionCount % 5);
  }
  agent.routePhase = std::fmod(agent.routePhase + delta * .105f, TwoPi);
  const float targetRadius =
      7.0f + 2.4f * std::sin(elapsedSeconds_ * .07f + agent.depthPhase);
  const float targetX =
      agent.home.x + std::cos(agent.routePhase) * targetRadius;
  const float targetZ =
      agent.home.z + std::sin(agent.routePhase) * targetRadius;
  const float desired =
      std::atan2(targetZ - agent.state.z, targetX - agent.state.x);
  const float before = agent.state.headingRadians;
  agent.state.headingRadians = MoveAngle(before, desired, .72f * delta);
  const float turn = Wrap(agent.state.headingRadians - before) / delta;
  agent.state.bankRadians +=
      std::clamp(std::clamp(turn * .32f, -.28f, .28f) - agent.state.bankRadians,
                 -.55f * delta, .55f * delta);
  const float targetSpeed =
      agent.state.behavior == DolphinBehavior::Cruise
          ? 2.25f
          : (agent.state.behavior == DolphinBehavior::Surface ? 1.75f : 2.05f);
  agent.state.speed +=
      std::clamp(targetSpeed - agent.state.speed, -.7f * delta, .7f * delta);
  const float x = agent.state.x + std::cos(agent.state.headingRadians) *
                                      agent.state.speed * delta;
  const float z = agent.state.z + std::sin(agent.state.headingRadians) *
                                      agent.state.speed * delta;
  if (ValidHabitat(x, z, agent.home)) {
    agent.state.x = x;
    agent.state.z = z;
  } else
    agent.state.headingRadians = MoveAngle(
        agent.state.headingRadians,
        std::atan2(agent.home.z - agent.state.z, agent.home.x - agent.state.x),
        1.4f * delta);
  const float targetY =
      DesiredDepth(agent, terrain_.Height(agent.state.x, agent.state.z));
  const float oldY = agent.state.y;
  agent.state.y +=
      std::clamp(targetY - agent.state.y, -.85f * delta, .85f * delta);
  const float targetPitch =
      std::clamp(std::atan2((agent.state.y - oldY) / delta,
                            std::max(.4f, agent.state.speed)),
                 -.36f, .36f);
  agent.state.pitchRadians += std::clamp(targetPitch - agent.state.pitchRadians,
                                         -.65f * delta, .65f * delta);
  agent.tailPhase = std::fmod(
      agent.tailPhase + delta * (5.0f + agent.state.speed * .9f), TwoPi);
  agent.state.tailBeat = std::sin(agent.tailPhase);
}
} // namespace island
