#include "IslandSimulator/BoarPose.hpp"
#include "IslandSimulator/BoarGroundPose.hpp"
#include "IslandSimulator/BoarHerd.hpp"
#include "IslandSimulator/RenderedTerrainSurface.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <numbers>
#include <stdexcept>

int main()
{
    using namespace island;
    const auto check = [](bool ok, const char* text) { if (!ok) throw std::runtime_error(text); };
    const auto distance = [](const BoarJoint& a, const BoarJoint& b) {
        return std::hypot(a.y - b.y, a.z - b.z);
    };
    constexpr float Tau = 2.0f * std::numbers::pi_v<float>;
    for (bool front : {false, true}) for (bool left : {false, true}) {
        for (float movement : {0.0f, 0.1f, 0.5f, 1.0f}) {
            auto previous = EvaluateBoarLeg(-Tau / 4096, movement, 0.018f, front, left);
            int stanceSamples = 0;
            float maximumLift = 0.0f;
            for (int i = 0; i <= 4096; ++i) {
                const float phase = Tau * static_cast<float>(i) / 4096;
                const auto pose = EvaluateBoarLeg(phase, movement, 0.018f, front, left);
                check(std::isfinite(pose.knee.y) && std::isfinite(pose.knee.z), "finite knee");
                check(std::abs(distance(pose.root, pose.knee) - BoarLegPose::UpperLength) < 1e-5f,
                      "upper segment length must remain fixed");
                check(std::abs(distance(pose.knee, pose.hoof) - BoarLegPose::LowerLength) < 1e-5f,
                      "lower segment length must remain fixed");
                check(pose.hoof.y >= BoarLegPose::HoofHeight && pose.hoof.y < 0.171f,
                      "hoof must not penetrate the local ground plane");
                if (pose.stance) check(pose.hoof.y == BoarLegPose::HoofHeight, "stance contact");
                stanceSamples += pose.stance ? 1 : 0;
                maximumLift = std::max(maximumLift, pose.hoof.y - BoarLegPose::HoofHeight);
                check(distance(pose.hoof, previous.hoof) < 0.002f, "continuous cycle and wrap");
                previous = pose;
            }
            if (movement > 0.0f) {
                check(stanceSamples > 2600 && stanceSamples < 2640, "64 percent stance duty cycle");
                check(maximumLift > 0.104f * movement, "swing must actually lift the hoof");
            } else check(stanceSamples == 4097, "all feet remain planted when stationary");
            const auto first = EvaluateBoarLeg(0, movement, 0.018f, front, left);
            check(distance(previous.hoof, first.hoof) < 1e-5f, "periodic hoof pose");
        }
    }
    for (float dip : {0.0f, 0.15f, std::numeric_limits<float>::quiet_NaN()}) {
        const auto body = MakeBoarBodyProfile(dip);
        for (const auto& ring : body) {
            check(std::abs(BoarBackHeight(body, ring.z, 0) - ring.centreY - ring.radiusY) < 1e-5f,
                  "dorsal roots match shared body rings");
        }
        for (int i = 0; i < 1000; ++i) {
            const float z = -0.56f + static_cast<float>(i) * 0.00094f;
            const float y = BoarBackHeight(body, z, 0.03f);
            check(y > 0.90f && y < 1.12f, "bristle region stays on back");
        }
    }
    const float nan = std::numeric_limits<float>::quiet_NaN();
    const auto invalid = EvaluateBoarLeg(nan, nan, nan, true, true);
    check(std::isfinite(invalid.knee.y) && invalid.hoof.y == BoarLegPose::HoofHeight,
          "invalid pose input has a finite resting fallback");
    for (float ground : {-100.0f, -0.20f, 0.0f, 0.10f, 100.0f, nan}) {
        for (bool front : {false, true}) {
            const auto pose = EvaluateBoarLeg(0, 1, 0.018f, front, true, ground);
            check(std::abs(distance(pose.root, pose.knee) - BoarLegPose::UpperLength) < 1e-5f,
                  "terrain target cannot stretch upper limb");
            check(std::abs(distance(pose.knee, pose.hoof) - BoarLegPose::LowerLength) < 1e-5f,
                  "terrain target cannot stretch lower limb");
            check(pose.reachLimited == (!std::isfinite(ground) || ground < -0.19f || ground > 1),
                  "unreachable and invalid targets must be reported");
        }
    }

    const IslandTerrain terrain;
    const RenderedTerrainSurface surface(terrain);
    float maximumOldError = 0, maximumContactError = 0;
    double oldErrorSum = 0, fittedErrorSum = 0;
    int contacts = 0, poses = 0;
    for (int x = -150; x <= 150; x += 30) for (int z = -150; z <= 150; z += 30) {
        const auto ground = terrain.Sample(static_cast<float>(x), static_cast<float>(z));
        if (ground.slopeRadians > BoarHerd::MaximumSlope
            || ground.signedShoreDistance < BoarHerd::MinimumShoreDistance || ground.height < 0) continue;
        for (float scale : {0.65f, 1.15f}) for (float heading : {0.0f, 0.91f, 2.70f}) {
            for (int phase = 0; phase < 16; ++phase) {
                BoarState state;
                state.x = static_cast<float>(x);
                state.z = static_cast<float>(z);
                state.scale = scale;
                state.headingRadians = heading;
                state.gaitPhase = Tau * static_cast<float>(phase) / 16.0f;
                state.speed = phase == 0 ? 0.0f : 0.65f;
                const auto fitted = FitBoarGroundPose(terrain, surface, state);
                ++poses;
                check(std::abs(fitted.origin.y - surface.Sample(state.x, state.z).height) < 1e-6f,
                      "body origin must use the rendered surface");
                for (std::size_t foot = 0; foot < fitted.legs.size(); ++foot) {
                    const auto& pose = fitted.legs[foot];
                    check(!pose.reachLimited, "valid sampled habitat must fit limb reach");
                    check(std::abs(distance(pose.root, pose.knee) - BoarLegPose::UpperLength) < 1e-5f,
                          "ground fit changed upper limb length");
                    check(std::abs(distance(pose.knee, pose.hoof) - BoarLegPose::LowerLength) < 1e-5f,
                          "ground fit changed lower limb length");
                    const auto sole = fitted.ToWorld({pose.hoof.x,
                        pose.hoof.y - BoarLegPose::HoofHeight, pose.hoof.z + 0.025f});
                    const float residual = sole.y - surface.Sample(sole.x, sole.z).height;
                    check(residual >= -0.00015f, "sole must not penetrate the rendered surface");
                    if (!pose.stance) continue;
                    check(std::abs(residual) < 0.00015f, "stance sole contact exceeds 0.15 mm");
                    maximumContactError = std::max(maximumContactError, std::abs(residual));
                    fittedErrorSum += std::abs(residual);
                    auto oldFrame = fitted;
                    oldFrame.origin.y = ground.height;
                    const auto oldSole = oldFrame.ToWorld({pose.hoof.x, 0, pose.hoof.z + 0.025f});
                    const float oldError = std::abs(oldSole.y - surface.Sample(oldSole.x, oldSole.z).height);
                    oldErrorSum += oldError;
                    maximumOldError = std::max(maximumOldError, oldError);
                    ++contacts;
                }
            }
        }
    }
    check(contacts > 1000 && poses > 1000, "ground fit audit must cover representative poses");
    check(oldErrorSum > fittedErrorSum * 20, "contact fit must materially improve the old plane");
    // Cross triangle/grid boundaries with idle feet; retain smooth height, not triangle-normal snaps.
    BoarState crossing;
    crossing.scale = 0.7f;
    crossing.headingRadians = 0.8f;
    auto before = FitBoarGroundPose(terrain, surface, crossing);
    for (int step = 1; step <= 200; ++step) {
        crossing.x = static_cast<float>(step) * 0.002f - 0.2f;
        crossing.z = -crossing.x;
        const auto after = FitBoarGroundPose(terrain, surface, crossing);
        if (step > 1) for (std::size_t foot = 0; foot < 4; ++foot) {
            const auto a = before.ToWorld(before.legs[foot].hoof);
            const auto b = after.ToWorld(after.legs[foot].hoof);
            check(std::hypot(a.x-b.x, a.y-b.y, a.z-b.z) < 0.004f,
                  "triangle boundary must not pop the fitted hoof");
        }
        before = after;
    }
    for (int field = 0; field < 5; ++field) {
        BoarState bad;
        if (field == 0) bad.x = nan;
        if (field == 1) bad.scale = 0;
        if (field == 2) bad.speed = -1;
        if (field == 3) bad.headingRadians = nan;
        if (field == 4) bad.z = TerrainMeshLayout::Extent;
        bool rejected = false;
        try { (void)FitBoarGroundPose(terrain, surface, bad); }
        catch (const std::invalid_argument&) { rejected = true; }
        check(rejected, "invalid contact state must be rejected");
    }
    std::cout << "boar ground audit: poses=" << poses << " stance-contacts=" << contacts
              << " old-max-error=" << maximumOldError << " fitted-max-error=" << maximumContactError << '\n';
    std::cout << "boar pose: fixed joints, stance contact, cycle continuity and dorsal profile passed\n";
}
