#include "IslandSimulator/DolphinPod.hpp"
#include "IslandSimulator/IslandTerrain.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>

static void Check(bool value, const char *message) {
  if (!value)
    throw std::runtime_error(message);
}

int main() {
  const island::IslandTerrain terrain;
  island::DolphinPod pod(terrain), repeat(terrain), invalid(terrain);
  Check(pod.Dolphins().size() == island::DolphinPod::DolphinCount,
        "wrong dolphin count");
  invalid.Update(std::numeric_limits<float>::quiet_NaN(), NAN, NAN, NAN);
  bool cruise = false, surface = false, dive = false, left = false,
       right = false, up = false, down = false;
  bool observedFullTier = false;
  float minimumDepth = 0, maximumDepth = -100;
  float travel = 0;
  auto previous = pod.Dolphins().front();
  for (int frame = 0; frame < 18000; ++frame) {
    const auto observer = pod.Dolphins().front();
    pod.Update(1.0f / 30, observer.x, observer.y, observer.z);
    repeat.Update(1.0f / 30, observer.x, observer.y, observer.z);
    for (std::size_t i = 0; i < pod.Dolphins().size(); ++i) {
      const auto &a = pod.Dolphins()[i];
      const auto &b = repeat.Dolphins()[i];
      Check(a.x == b.x && a.y == b.y && a.z == b.z && a.tailBeat == b.tailBeat,
            "dolphin pod is not deterministic");
      const auto sample = terrain.Sample(a.x, a.z);
      Check(std::isfinite(a.x) && std::isfinite(a.y) &&
                std::isfinite(a.headingRadians),
            "nonfinite dolphin state");
      Check(sample.signedShoreDistance >=
                    island::DolphinPod::MinimumShoreDistance - .01f &&
                sample.signedShoreDistance <=
                    island::DolphinPod::MaximumShoreDistance + .01f &&
                sample.height <= -3,
            "dolphin left offshore habitat");
      Check(std::hypot(a.x - pod.Homes()[i].x, a.z - pod.Homes()[i].z) <=
                pod.Homes()[i].radius + .01f,
            "dolphin left home range");
      Check(a.y >= sample.height + 1.24f * a.scale - .02f && a.y <= -.20f,
            "dolphin crossed seabed or water surface");
      cruise |= a.behavior == island::DolphinBehavior::Cruise;
      surface |= a.behavior == island::DolphinBehavior::Surface;
      dive |= a.behavior == island::DolphinBehavior::Dive;
      minimumDepth = std::min(minimumDepth, a.y);
      maximumDepth = std::max(maximumDepth, a.y);
      left |= a.tailBeat < -.8f;
      right |= a.tailBeat > .8f;
      up |= a.pitchRadians > .03f;
      down |= a.pitchRadians < -.03f;
    }
    observedFullTier |= pod.FullSimulationCount() > 0;
    travel += std::hypot(pod.Dolphins().front().x - previous.x,
                         pod.Dolphins().front().z - previous.z);
    previous = pod.Dolphins().front();
  }
  Check(cruise && surface && dive, "dolphin behavior cycle incomplete");
  Check(maximumDepth > -.40f && minimumDepth < -2.20f,
        "dolphin did not complete surface and dive travel");
  Check(left && right && up && down,
        "dolphin articulated swim pose incomplete");
  Check(travel > 100, "dolphin pod did not travel");
  Check(observedFullTier, "near dolphin did not enter full simulation tier");
  Check(pod.FullSimulationCount() + pod.ReducedSimulationCount() +
                pod.DormantSimulationCount() ==
            static_cast<int>(island::DolphinPod::DolphinCount),
        "dolphin tiers do not partition pod");
  pod.Update(1.0f / 30, 10000, 10000, 10000);
  Check(pod.DormantSimulationCount() ==
            static_cast<int>(island::DolphinPod::DolphinCount),
        "distant dolphin pod did not become dormant");
  std::cout << "Dolphin pod " << pod.Dolphins().size() << " agents; travel "
            << travel << "m; cruise/surface/dive and tail/depth motion pass\n";
}
