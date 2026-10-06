#include "IslandSimulator/AudioAnalysis.hpp"
#include "IslandSimulator/BenchmarkTraversalRoute.hpp"
#include "IslandSimulator/BirdFlock.hpp"
#include "IslandSimulator/DebugFreeCamera.hpp"
#include "IslandSimulator/DistantSceneryLayout.hpp"
#include "IslandSimulator/FishSchools.hpp"
#include "IslandSimulator/FirstPersonCameraMotion.hpp"
#include "IslandSimulator/FirstPersonInput.hpp"
#include "IslandSimulator/GrassInteractionModel.hpp"
#include "IslandSimulator/GrassLayout.hpp"
#include "IslandSimulator/GrassLod.hpp"
#include "IslandSimulator/IslandTerrain.hpp"
#include "IslandSimulator/JungleLayout.hpp"
#include "IslandSimulator/MonkeyTroop.hpp"
#include "IslandSimulator/NaturalPropLayout.hpp"
#include "IslandSimulator/OceanMeshLayout.hpp"
#include "IslandSimulator/PerformanceStatistics.hpp"
#include "IslandSimulator/PlayerMotion.hpp"
#include "IslandSimulator/SharkPatrol.hpp"
#include "IslandSimulator/ShorelineMeshLayout.hpp"
#include "IslandSimulator/SoundscapeModel.hpp"
#include "IslandSimulator/TerrainMeshLayout.hpp"
#include "IslandSimulator/TouchControls.hpp"
#include "IslandSimulator/VegetationLayout.hpp"
#include "IslandSimulator/WaterRippleModel.hpp"
#include "IslandSimulator/WaterViewModel.hpp"
#include "IslandSimulator/WorldCollisionGrid.hpp"
#include "IslandSimulator/WorldIdentity.hpp"
#include "IslandSimulator/WildlifeSimulation.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <numeric>
#include <numbers>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace
{
    int failures = 0;

    void Check(const bool condition, const std::string_view description)
    {
        if (!condition) {
            std::cerr << "FAIL: " << description << '\n';
            ++failures;
        }
    }

    void CheckNear(const float actual, const float expected, const float tolerance,
                   const std::string_view description)
    {
        Check(std::abs(actual - expected) <= tolerance, description);
    }
}

int main()
{
    island::AmbientLoop knownSignal;
    knownSignal.sampleRate = 4;
    knownSignal.samples = {0, 16384, -16384, 0};
    const island::AudioSignalStatistics knownStatistics =
        island::AnalyzeAudioSignal(knownSignal);
    Check(knownStatistics.finite && knownStatistics.sampleCount == 4U
              && knownStatistics.sampleRate == 4,
          "audio analysis accepts a finite PCM16 signal");
    CheckNear(static_cast<float>(knownStatistics.durationSeconds), 1.0f, 0.0001f,
              "audio analysis reports duration from sample count and rate");
    CheckNear(static_cast<float>(knownStatistics.mean), 0.0f, 0.0001f,
              "audio analysis reports normalized DC offset");
    CheckNear(static_cast<float>(knownStatistics.rootMeanSquare), 0.353553f, 0.0001f,
              "audio analysis reports normalized RMS energy");
    CheckNear(static_cast<float>(knownStatistics.peak), 0.5f, 0.0001f,
              "audio analysis reports normalized peak magnitude");
    CheckNear(static_cast<float>(knownStatistics.seamStep), 0.0f, 0.0001f,
              "audio analysis reports the loop seam step");
    CheckNear(static_cast<float>(knownStatistics.maximumInternalStep), 1.0f, 0.0001f,
              "audio analysis reports the largest adjacent PCM step");
    Check(!island::AnalyzeAudioSignal({}).finite,
          "audio analysis rejects an empty signal without a sample rate");

    const island::WorldSeedSet worldSeeds = island::AuthoritativeWorldSeeds();
    Check(worldSeeds == island::WorldSeedSet{0x7a11d5e3U, 0x6b4d2f91U,
                                             0x43b0a7d1U, 0xa7816c39U},
          "world identity exposes the four authoritative layout seeds");
    Check(island::WorldSeedOverlayHex(0x00001a2bU) == "00001A2B",
          "world seed overlay formatting is fixed-width uppercase hexadecimal");
    Check(island::WorldSeedClipboardText(worldSeeds)
              == "island-simulator world seeds\n"
                 "palms=0x7a11d5e3\n"
                 "grass=0x6b4d2f91\n"
                 "jungle=0x43b0a7d1\n"
                 "natural-props=0xa7816c39\n",
          "world seed clipboard text is stable and report-friendly");

    const island::BenchmarkTraversalRoute benchmarkRoute;
    const island::BenchmarkRouteSample routeStart = benchmarkRoute.Sample(0.0f);
    const island::BenchmarkRouteSample routeEnd = benchmarkRoute.Sample(1.0f);
    CheckNear(routeStart.x, benchmarkRoute.Waypoints().front().x, 0.0001f,
              "benchmark route starts at its first authored waypoint");
    CheckNear(routeStart.z, benchmarkRoute.Waypoints().front().z, 0.0001f,
              "benchmark route start retains its authored depth");
    CheckNear(routeEnd.x, benchmarkRoute.Waypoints().back().x, 0.0001f,
              "benchmark route ends at its last authored waypoint");
    CheckNear(routeEnd.z, benchmarkRoute.Waypoints().back().z, 0.0001f,
              "benchmark route end retains its authored depth");
    CheckNear(benchmarkRoute.Sample(-1.0f).z, routeStart.z, 0.0001f,
              "benchmark route clamps negative progress");
    CheckNear(benchmarkRoute.Sample(2.0f).z, routeEnd.z, 0.0001f,
              "benchmark route clamps progress beyond its end");
    CheckNear(benchmarkRoute.Sample(std::numeric_limits<float>::quiet_NaN()).z,
              routeStart.z, 0.0001f,
              "benchmark route fails closed on non-finite progress");
    const float benchmarkRouteLength = benchmarkRoute.ApproximateLength();
    Check(benchmarkRouteLength > 600.0f && benchmarkRouteLength < 850.0f,
          "benchmark route has a sustained but island-scale horizontal length");
    float minimumRouteShoreDistance = std::numeric_limits<float>::max();
    float maximumRouteShoreDistance = std::numeric_limits<float>::lowest();
    const island::IslandTerrain routeTerrain;
    for (int index = 0; index <= 600; ++index) {
        const island::BenchmarkRouteSample sample = benchmarkRoute.Sample(
            static_cast<float>(index) / 600.0f);
        Check(std::isfinite(sample.x) && std::isfinite(sample.z)
                  && std::isfinite(sample.forwardX) && std::isfinite(sample.forwardZ),
              "benchmark route samples remain finite");
        CheckNear(std::hypot(sample.forwardX, sample.forwardZ), 1.0f, 0.0001f,
                  "benchmark route look direction remains normalized");
        Check(std::hypot(sample.x, sample.z) < 270.0f,
              "benchmark route stays within the island and near-shore profiling envelope");
        const float shoreDistance = routeTerrain.SignedShoreDistance(sample.x, sample.z);
        minimumRouteShoreDistance = std::min(minimumRouteShoreDistance, shoreDistance);
        maximumRouteShoreDistance = std::max(maximumRouteShoreDistance, shoreDistance);
    }
    Check(minimumRouteShoreDistance < -3.0f,
          "benchmark route includes shallow-water renderer coverage");
    Check(maximumRouteShoreDistance > 120.0f,
          "benchmark route includes deep-interior renderer coverage");

    const std::array<double, 5> frameTimes = {50.0, 10.0, 30.0, 20.0, 40.0};
    const island::FrameTimeSummary frameSummary =
        island::CalculateFrameTimeSummary(frameTimes);
    Check(frameSummary.sampleCount == frameTimes.size(),
          "frame-time statistics retain their sample count");
    CheckNear(static_cast<float>(frameSummary.minimumMilliseconds), 10.0f, 0.0001f,
              "frame-time statistics report the minimum");
    CheckNear(static_cast<float>(frameSummary.maximumMilliseconds), 50.0f, 0.0001f,
              "frame-time statistics report the maximum");
    CheckNear(static_cast<float>(frameSummary.meanMilliseconds), 30.0f, 0.0001f,
              "frame-time statistics report the arithmetic mean");
    CheckNear(static_cast<float>(frameSummary.medianMilliseconds), 30.0f, 0.0001f,
              "frame-time statistics report the interpolated median");
    CheckNear(static_cast<float>(frameSummary.percentile95Milliseconds), 48.0f, 0.0001f,
              "frame-time statistics report the interpolated 95th percentile");
    CheckNear(static_cast<float>(frameSummary.percentile99Milliseconds), 49.6f, 0.0001f,
              "frame-time statistics report the interpolated 99th percentile");
    CheckNear(static_cast<float>(frameSummary.meanFramesPerSecond), 33.3333f, 0.001f,
              "frame-time statistics derive mean frames per second");
    bool rejectedEmptyFrameTimes = false;
    try {
        (void)island::CalculateFrameTimeSummary(std::span<const double>());
    } catch (const std::invalid_argument&) {
        rejectedEmptyFrameTimes = true;
    }
    Check(rejectedEmptyFrameTimes, "frame-time statistics reject an empty sample set");
    bool rejectedInvalidFrameTime = false;
    const std::array<double, 1> invalidFrameTime = {
        std::numeric_limits<double>::quiet_NaN()};
    try {
        (void)island::CalculateFrameTimeSummary(invalidFrameTime);
    } catch (const std::invalid_argument&) {
        rejectedInvalidFrameTime = true;
    }
    Check(rejectedInvalidFrameTime,
          "frame-time statistics reject non-finite sample durations");

    const island::FirstPersonMovementIntent forwardIntent =
        island::FirstPersonInput::Resolve(
            island::FirstPersonDigitalInput{true, false, false, false, false},
            0.0f, 0.0f, 0.0f);
    CheckNear(forwardIntent.worldX, 0.0f, 0.0001f,
              "first-person forward input has no lateral drift at zero yaw");
    CheckNear(forwardIntent.worldZ, 1.0f, 0.0001f,
              "first-person forward input follows zero yaw");
    const island::FirstPersonMovementIntent rightIntent =
        island::FirstPersonInput::Resolve(
            island::FirstPersonDigitalInput{false, false, false, true, false},
            0.0f, 0.0f, 0.0f);
    CheckNear(rightIntent.worldX, -1.0f, 0.0001f,
              "first-person right input follows screen-right in the view convention");
    CheckNear(rightIntent.worldZ, 0.0f, 0.0001f,
              "first-person right input has no forward drift at zero yaw");
    const island::FirstPersonMovementIntent leftIntent =
        island::FirstPersonInput::Resolve(
            island::FirstPersonDigitalInput{false, false, true, false, false},
            0.0f, 0.0f, 0.0f);
    CheckNear(leftIntent.worldX, 1.0f, 0.0001f,
              "first-person left input opposes screen-right in the view convention");
    const island::FirstPersonMovementIntent rotatedRunIntent =
        island::FirstPersonInput::Resolve(
            island::FirstPersonDigitalInput{true, false, false, false, true},
            0.0f, 0.0f, -std::numbers::pi_v<float> * 0.5f);
    CheckNear(rotatedRunIntent.worldX, -1.0f, 0.0001f,
              "first-person forward intent immediately follows changed mouse yaw");
    CheckNear(rotatedRunIntent.worldZ, 0.0f, 0.0001f,
              "first-person changed-yaw intent remains normalized");
    Check(rotatedRunIntent.run, "first-person intent preserves the selected run mode");
    const island::FirstPersonMovementIntent invalidAnalogIntent =
        island::FirstPersonInput::Resolve(
            island::FirstPersonDigitalInput{},
            std::numeric_limits<float>::quiet_NaN(),
            std::numeric_limits<float>::infinity(),
            std::numeric_limits<float>::quiet_NaN());
    CheckNear(invalidAnalogIntent.worldX, 0.0f, 0.0001f,
              "first-person input rejects non-finite lateral values");
    CheckNear(invalidAnalogIntent.worldZ, 0.0f, 0.0001f,
              "first-person input rejects non-finite forward values");

    island::DebugFreeCamera freeCamera;
    freeCamera.Update(0.0f, island::DebugFreeCameraInput{1.0f, 0.0f, 0.0f, false}, 0.1f);
    Check(!freeCamera.IsActive()
              && freeCamera.Position().x == 0.0f
              && freeCamera.Position().y == 0.0f
              && freeCamera.Position().z == 0.0f,
          "inactive debug free camera ignores movement input");
    freeCamera.Activate(10.0f, 20.0f, 30.0f);
    freeCamera.Update(0.0f, island::DebugFreeCameraInput{1.0f, 0.0f, 0.0f, false}, 1.0f);
    CheckNear(freeCamera.Position().x, 10.0f, 0.0001f,
              "debug free camera forward input follows yaw without lateral drift");
    CheckNear(freeCamera.Position().z, 31.2f, 0.0001f,
              "debug free camera clamps long frames before movement");
    const island::DebugFreeCameraPosition beforeDiagonal = freeCamera.Position();
    freeCamera.Update(
        0.0f, island::DebugFreeCameraInput{1.0f, 1.0f, 1.0f, false}, 0.1f);
    CheckNear(std::hypot(
                  freeCamera.Position().y - beforeDiagonal.y,
                  std::hypot(freeCamera.Position().x - beforeDiagonal.x,
                             freeCamera.Position().z - beforeDiagonal.z)),
              island::DebugFreeCamera::BaseSpeed * 0.1f, 0.0001f,
              "debug free camera normalizes three-axis input");
    freeCamera.Activate(10.0f, 20.0f, 30.0f);
    freeCamera.Update(0.0f, island::DebugFreeCameraInput{0.0f, 1.0f, 0.0f, false}, 0.1f);
    CheckNear(freeCamera.Position().x, 8.8f, 0.0001f,
              "debug free camera right input follows the visible view convention");
    CheckNear(freeCamera.Position().z, 30.0f, 0.0001f,
              "debug free camera right input has no forward drift");
    const float beforeFastX = freeCamera.Position().x;
    freeCamera.Update(
        std::numbers::pi_v<float> * 0.5f,
        island::DebugFreeCameraInput{1.0f, 0.0f, 0.0f, true}, 0.1f);
    CheckNear(freeCamera.Position().x - beforeFastX,
              island::DebugFreeCamera::BaseSpeed
                  * island::DebugFreeCamera::FastMultiplier * 0.1f,
              0.0001f, "debug free camera fast mode preserves yaw-relative motion");
    freeCamera.Activate(900.0f, -90.0f, -900.0f);
    CheckNear(freeCamera.Position().x, island::DebugFreeCamera::HorizontalExtent, 0.0001f,
              "debug free camera activation clamps positive world extent");
    CheckNear(freeCamera.Position().y, island::DebugFreeCamera::MinimumHeight, 0.0001f,
              "debug free camera activation clamps minimum inspection height");
    CheckNear(freeCamera.Position().z, -island::DebugFreeCamera::HorizontalExtent, 0.0001f,
              "debug free camera activation clamps negative world extent");
    freeCamera.Deactivate();
    const island::DebugFreeCameraPosition retainedFreePosition = freeCamera.Position();
    freeCamera.Update(0.0f, island::DebugFreeCameraInput{1.0f, 1.0f, 1.0f, true}, 0.1f);
    Check(!freeCamera.IsActive()
              && freeCamera.Position().x == retainedFreePosition.x
              && freeCamera.Position().y == retainedFreePosition.y
              && freeCamera.Position().z == retainedFreePosition.z,
          "deactivated debug free camera retains its last bounded position");

    island::FirstPersonCameraMotion cameraMotion;
    CheckNear(cameraMotion.Offset().lateralMeters, 0.0f, 0.000001f,
              "camera motion starts centred laterally");
    CheckNear(cameraMotion.Offset().verticalMeters, 0.0f, 0.000001f,
              "camera motion starts centred vertically");
    float largestWalkingLateral = 0.0f;
    float largestWalkingVertical = 0.0f;
    for (int frame = 0; frame < 240; ++frame) {
        const island::CameraMotionOffset offset = cameraMotion.Update(
            island::PlayerMotion::WalkSpeed, island::LocomotionMode::Walking, 1.0f / 60.0f);
        largestWalkingLateral = std::max(largestWalkingLateral,
                                         std::abs(offset.lateralMeters));
        largestWalkingVertical = std::max(largestWalkingVertical,
                                          std::abs(offset.verticalMeters));
        Check(std::abs(offset.lateralMeters)
                  <= island::FirstPersonCameraMotion::MaximumLateralOffset + 0.000001f,
              "walking camera motion stays inside its lateral comfort bound");
        Check(std::abs(offset.verticalMeters)
                  <= island::FirstPersonCameraMotion::MaximumVerticalOffset + 0.000001f,
              "walking camera motion stays inside its vertical comfort bound");
    }
    Check(largestWalkingLateral > 0.005f && largestWalkingVertical > 0.009f,
          "full-speed walking produces restrained but nonzero camera motion");
    island::FirstPersonCameraMotion runningCameraMotion;
    float largestRunningLateral = 0.0f;
    float largestRunningVertical = 0.0f;
    for (int frame = 0; frame < 240; ++frame) {
        const island::CameraMotionOffset offset = runningCameraMotion.Update(
            island::PlayerMotion::RunSpeed, island::LocomotionMode::Walking, 1.0f / 60.0f);
        largestRunningLateral = std::max(largestRunningLateral,
                                        std::abs(offset.lateralMeters));
        largestRunningVertical = std::max(largestRunningVertical,
                                         std::abs(offset.verticalMeters));
        Check(std::abs(offset.lateralMeters)
                  <= island::FirstPersonCameraMotion::MaximumLateralOffset + 0.000001f
                  && std::abs(offset.verticalMeters)
                  <= island::FirstPersonCameraMotion::MaximumVerticalOffset + 0.000001f,
              "running camera motion stays inside both comfort bounds");
    }
    Check(largestRunningLateral > largestWalkingLateral
              && largestRunningVertical > largestWalkingVertical,
          "running has a stronger camera-motion signature than walking");
    for (int frame = 0; frame < 180; ++frame) {
        (void)cameraMotion.Update(island::PlayerMotion::SwimSpeed,
                                  island::LocomotionMode::Swimming, 1.0f / 60.0f);
    }
    Check(cameraMotion.SwimBlend() > 0.99f,
          "camera motion smoothly reaches its swimming style");
    Check(std::abs(cameraMotion.Offset().lateralMeters)
              <= island::FirstPersonCameraMotion::MaximumLateralOffset + 0.000001f
              && std::abs(cameraMotion.Offset().verticalMeters)
              <= island::FirstPersonCameraMotion::MaximumVerticalOffset + 0.000001f,
          "swimming camera motion stays inside both comfort bounds");
    cameraMotion.SetEnabled(false);
    for (int frame = 0; frame < 180; ++frame) {
        (void)cameraMotion.Update(island::PlayerMotion::WalkSpeed,
                                  island::LocomotionMode::Walking, 1.0f / 60.0f);
    }
    Check(cameraMotion.MotionStrength() < 0.00001f
              && std::abs(cameraMotion.Offset().lateralMeters) < 0.000001f
              && std::abs(cameraMotion.Offset().verticalMeters) < 0.000001f,
          "disabling camera motion smoothly returns to the physical eye point");
    island::FirstPersonCameraMotion clampedCameraMotion;
    island::FirstPersonCameraMotion referenceCameraMotion;
    const island::CameraMotionOffset clampedOffset = clampedCameraMotion.Update(
        island::PlayerMotion::WalkSpeed, island::LocomotionMode::Walking, 5.0f);
    const island::CameraMotionOffset referenceOffset = referenceCameraMotion.Update(
        island::PlayerMotion::WalkSpeed, island::LocomotionMode::Walking, 0.1f);
    CheckNear(clampedOffset.lateralMeters, referenceOffset.lateralMeters, 0.000001f,
              "camera motion clamps a long frame before phase integration");
    CheckNear(clampedOffset.verticalMeters, referenceOffset.verticalMeters, 0.000001f,
              "camera motion long-frame clamp is deterministic");
    cameraMotion.Reset();
    CheckNear(cameraMotion.MotionStrength(), 0.0f, 0.000001f,
              "camera motion reset clears residual movement strength");

    CheckNear(island::GrassInteractionModel::TargetStrength(0.0f), 0.0f, 0.000001f,
              "stationary grass interaction retains only the shader's body-radius bend");
    CheckNear(island::GrassInteractionModel::TargetStrength(-4.0f), 0.0f, 0.000001f,
              "negative grass interaction speed fails closed");
    CheckNear(island::GrassInteractionModel::TargetStrength(
                  std::numeric_limits<float>::quiet_NaN()),
              0.0f, 0.000001f,
              "non-finite grass interaction speed fails closed");
    const float walkingGrassInteraction =
        island::GrassInteractionModel::TargetStrength(island::PlayerMotion::WalkSpeed);
    CheckNear(walkingGrassInteraction, 1.0f, 0.000001f,
              "full walking speed reaches the bounded grass interaction envelope");
    island::GrassInteractionModel grassInteraction;
    const float firstGrassInteractionStep = grassInteraction.Update(
        island::PlayerMotion::WalkSpeed, 1.0f / 60.0f);
    Check(firstGrassInteractionStep > 0.0f && firstGrassInteractionStep < 1.0f,
          "grass interaction eases in instead of appearing in one frame");
    const float beforeLongGrassFrame = grassInteraction.Strength();
    (void)grassInteraction.Update(island::PlayerMotion::WalkSpeed, 5.0f);
    Check(grassInteraction.Strength() - beforeLongGrassFrame
              <= island::GrassInteractionModel::EntryRatePerSecond
                    * island::GrassInteractionModel::MaximumStepSeconds + 0.000001f,
          "grass interaction clamps a long frame");
    grassInteraction.Reset(island::PlayerMotion::WalkSpeed);
    CheckNear(grassInteraction.Strength(), 1.0f, 0.000001f,
              "grass interaction reset supports deterministic QA placement");
    const float beforeInvalidGrassDelta = grassInteraction.Strength();
    (void)grassInteraction.Update(0.0f, std::numeric_limits<float>::quiet_NaN());
    CheckNear(grassInteraction.Strength(), beforeInvalidGrassDelta, 0.000001f,
              "grass interaction rejects a non-finite frame duration safely");
    for (int frame = 0; frame < 180; ++frame) {
        (void)grassInteraction.Update(0.0f, 1.0f / 60.0f);
    }
    CheckNear(grassInteraction.Strength(), 0.0f, 0.000001f,
              "grass interaction releases fully after movement stops");

    CheckNear(island::WaterViewModel::TargetIntensity(false, -2.0f, 1.0f), 0.0f, 0.000001f,
              "water view remains absent outside swimming mode");
    const float shallowWaterView = island::WaterViewModel::TargetIntensity(true, -0.48f, 0.0f);
    const float deepWaterView = island::WaterViewModel::TargetIntensity(true, -2.5f, 0.0f);
    const float currentWaterView = island::WaterViewModel::TargetIntensity(true, -2.5f, 1.0f);
    Check(shallowWaterView >= 0.60f && shallowWaterView < 0.70f,
          "near-shore swimming receives a restrained minimum water-view cue");
    Check(deepWaterView > shallowWaterView && deepWaterView < 0.80f,
          "deeper swimming strengthens the restrained near-shore water-view cue");
    CheckNear(currentWaterView, 1.0f, 0.000001f,
              "full offshore current makes the peripheral warning unambiguous but bounded");
    CheckNear(island::WaterViewModel::TargetIntensity(
                  true, -2.5f, std::numeric_limits<float>::quiet_NaN()),
              deepWaterView, 0.000001f,
              "non-finite current input falls back to the calm-water presentation");
    island::WaterViewModel waterView;
    const float firstWaterViewStep = waterView.Update(true, -0.48f, 0.0f, 1.0f / 60.0f);
    Check(firstWaterViewStep > 0.0f && firstWaterViewStep < shallowWaterView,
          "water-view entry fades instead of appearing in one frame");
    const float beforeLongWaterFrame = waterView.Intensity();
    (void)waterView.Update(true, -2.5f, 1.0f, 5.0f);
    Check(waterView.Intensity() - beforeLongWaterFrame
              <= island::WaterViewModel::EntryRatePerSecond
                    * island::WaterViewModel::MaximumStepSeconds + 0.000001f,
          "water-view entry clamps long frames");
    waterView.Reset(true, -2.5f, 1.0f);
    CheckNear(waterView.Intensity(), currentWaterView, 0.000001f,
              "water-view reset snaps deterministic QA placement to its target");
    for (int frame = 0; frame < 180; ++frame) {
        (void)waterView.Update(false, -2.5f, 1.0f, 1.0f / 60.0f);
    }
    CheckNear(waterView.Intensity(), 0.0f, 0.000001f,
              "water-view exit fades fully back to a clear dry view");
    const float beforeInvalidWaterDelta = waterView.Intensity();
    (void)waterView.Update(true, -1.0f, 0.0f,
                           std::numeric_limits<float>::quiet_NaN());
    CheckNear(waterView.Intensity(), beforeInvalidWaterDelta, 0.000001f,
              "water-view transition rejects a non-finite frame duration safely");

    CheckNear(island::WaterRippleModel::TargetIntensity(false, 1.0f, 3.0f),
              0.0f, 0.000001f,
              "dry movement does not create an ocean-surface ripple");
    CheckNear(island::WaterRippleModel::TargetIntensity(true, -1.0f, 0.0f),
              0.0f, 0.000001f,
              "stationary swimming does not create a perpetual ripple");
    const float wadingRipple = island::WaterRippleModel::TargetIntensity(
        false, -0.20f, 1.0f);
    const float swimmingRipple = island::WaterRippleModel::TargetIntensity(
        true, -1.0f, 2.0f);
    Check(wadingRipple > 0.15f && wadingRipple < 0.35f,
          "moving through shallow water creates a restrained wading ripple");
    CheckNear(swimmingRipple, 1.0f, 0.000001f,
              "active swimming can reach the bounded full ripple intensity");
    CheckNear(island::WaterRippleModel::TargetIntensity(
                  true, -1.0f, std::numeric_limits<float>::quiet_NaN()),
              0.0f, 0.000001f,
              "non-finite ripple input fails closed");
    island::WaterRippleModel waterRipple;
    const float firstRippleStep = waterRipple.Update(true, -1.0f, 2.0f, 1.0f / 60.0f);
    Check(firstRippleStep > 0.0f && firstRippleStep < swimmingRipple,
          "player ripple intensity eases in instead of appearing in one frame");
    const float firstRipplePhase = waterRipple.Phase();
    Check(firstRipplePhase > 0.0f && firstRipplePhase < 1.0f,
          "active player ripples advance a wrapped radial phase");
    const float beforeLongRippleFrame = waterRipple.Intensity();
    (void)waterRipple.Update(true, -1.0f, 2.0f, 5.0f);
    Check(waterRipple.Intensity() - beforeLongRippleFrame
              <= island::WaterRippleModel::EntryRatePerSecond
                    * island::WaterRippleModel::MaximumStepSeconds + 0.000001f,
          "player ripple intensity clamps a long frame");
    waterRipple.Reset(true, -1.0f, 2.0f);
    CheckNear(waterRipple.Intensity(), 1.0f, 0.000001f,
              "player ripple reset snaps deterministic QA placement to its target");
    CheckNear(waterRipple.Phase(), 0.0f, 0.000001f,
              "player ripple reset returns to a deterministic phase");
    for (int frame = 0; frame < 600; ++frame) {
        (void)waterRipple.Update(true, -1.0f, 2.0f, 1.0f / 60.0f);
        Check(waterRipple.Phase() >= 0.0f && waterRipple.Phase() < 1.0f,
              "player ripple phase remains wrapped during sustained movement");
    }
    for (int frame = 0; frame < 180; ++frame) {
        (void)waterRipple.Update(true, -1.0f, 0.0f, 1.0f / 60.0f);
    }
    CheckNear(waterRipple.Intensity(), 0.0f, 0.000001f,
              "stationary water movement releases the ripple fully");

    const island::AmbientLoop surfLoop = island::GenerateSurfLoop();
    const island::AmbientLoop repeatedSurfLoop = island::GenerateSurfLoop();
    const island::AmbientLoop breezeLoop = island::GenerateBreezeLoop();
    const island::AmbientLoop jungleLoop = island::GenerateJungleLoop();
    const island::AmbientLoop repeatedJungleLoop = island::GenerateJungleLoop();
    const island::AmbientLoop waveAccent = island::GenerateWaveAccent();
    const island::AmbientLoop waveAccentAlternate = island::GenerateWaveAccentAlternate();
    const island::AmbientLoop waterMovementLoop = island::GenerateWaterMovementLoop();
    const island::AmbientLoop repeatedWaterMovementLoop = island::GenerateWaterMovementLoop();
    const island::AmbientLoop waterTransitionAccent = island::GenerateWaterTransitionAccent();
    const island::AmbientLoop waterTransitionAccentAlternate =
        island::GenerateWaterTransitionAccentAlternate();
    const island::AmbientLoop birdCall = island::GenerateBirdCall();
    const island::AmbientLoop birdCallAlternate = island::GenerateBirdCallAlternate();
    const island::AmbientLoop monkeyCall = island::GenerateMonkeyCall();
    const island::AmbientLoop monkeyCallAlternate = island::GenerateMonkeyCallAlternate();
    const island::AmbientLoop repeatedBirdCall = island::GenerateBirdCall();
    const island::AmbientLoop repeatedBirdCallAlternate = island::GenerateBirdCallAlternate();
    const island::AmbientLoop repeatedMonkeyCall = island::GenerateMonkeyCall();
    const island::AmbientLoop repeatedMonkeyCallAlternate = island::GenerateMonkeyCallAlternate();
    Check(surfLoop.sampleRate == 22050 && breezeLoop.sampleRate == 22050,
          "procedural ambience uses the intended portable sample rate");
    Check(surfLoop.samples.size() == 220500U && breezeLoop.samples.size() == 242550U,
          "procedural ambient loops have their intended staggered portable lengths");
    Check(jungleLoop.sampleRate == 22050 && jungleLoop.samples.size() == 264600U,
          "daytime-jungle ambience has its intended twelve-second portable format");
    Check(waveAccent.sampleRate == 22050 && waveAccent.samples.size() == 88200U
              && waveAccentAlternate.sampleRate == 22050
              && waveAccentAlternate.samples.size() == 88200U,
          "both stronger-wave accents have the intended four-second portable format");
    Check(waterMovementLoop.sampleRate == 22050
              && waterMovementLoop.samples.size() == 154350U,
          "player water movement has its intended seven-second portable format");
    const std::array<std::size_t, 4> persistentLoopSeconds = {
        surfLoop.samples.size() / 22050U,
        breezeLoop.samples.size() / 22050U,
        jungleLoop.samples.size() / 22050U,
        waterMovementLoop.samples.size() / 22050U,
    };
    const std::size_t combinedLoopPeriod = std::accumulate(
        persistentLoopSeconds.begin(), persistentLoopSeconds.end(), std::size_t{1U},
        [](const std::size_t left, const std::size_t right) {
            return std::lcm(left, right);
        });
    Check(combinedLoopPeriod == 4620U,
          "persistent ambience avoids a complete repeated phase alignment for 77 minutes");
    Check(waterTransitionAccent.sampleRate == 22050
              && waterTransitionAccent.samples.size() == 33075U
              && waterTransitionAccentAlternate.sampleRate == 22050
              && waterTransitionAccentAlternate.samples.size() == 33075U,
          "both water transitions have the intended one-and-a-half-second portable format");
    Check(birdCall.sampleRate == 22050 && birdCall.samples.size() == 26460U
              && birdCallAlternate.sampleRate == 22050
              && birdCallAlternate.samples.size() == 29767U,
          "both bird phrases have their intended short portable formats");
    Check(monkeyCall.sampleRate == 22050 && monkeyCall.samples.size() == 39690U
              && monkeyCallAlternate.sampleRate == 22050
              && monkeyCallAlternate.samples.size() == 46305U,
          "both monkey phrases have their intended short portable formats");
    Check(surfLoop.samples == repeatedSurfLoop.samples,
          "procedural surf generation is deterministic");
    Check(waterMovementLoop.samples == repeatedWaterMovementLoop.samples,
          "procedural player water movement generation is deterministic");
    Check(jungleLoop.samples == repeatedJungleLoop.samples,
          "procedural daytime-jungle generation is deterministic");
    Check(waveAccent.samples != waveAccentAlternate.samples,
          "stronger-wave variants contain genuinely different source PCM");
    Check(waterTransitionAccent.samples != waterTransitionAccentAlternate.samples,
          "water-transition variants contain genuinely different source PCM");
    Check(birdCall.samples != birdCallAlternate.samples
              && monkeyCall.samples != monkeyCallAlternate.samples,
          "wildlife variants contain genuinely different source PCM");
    Check(birdCall.samples == repeatedBirdCall.samples
              && birdCallAlternate.samples == repeatedBirdCallAlternate.samples
              && monkeyCall.samples == repeatedMonkeyCall.samples
              && monkeyCallAlternate.samples == repeatedMonkeyCallAlternate.samples,
          "all procedural wildlife phrases regenerate deterministically");
    const auto peakMagnitude = [](const island::AmbientLoop& loop) {
        int peak = 0;
        for (const std::int16_t sample : loop.samples) {
            peak = std::max(peak, std::abs(static_cast<int>(sample)));
        }
        return peak;
    };
    const auto rootMeanSquare = [](const island::AmbientLoop& loop) {
        double squaredSum = 0.0;
        for (const std::int16_t sample : loop.samples) {
            const double value = static_cast<double>(sample);
            squaredSum += value * value;
        }
        return std::sqrt(squaredSum / static_cast<double>(loop.samples.size()));
    };
    Check(peakMagnitude(surfLoop) < 32767 && peakMagnitude(breezeLoop) < 32767,
          "procedural ambience retains headroom before runtime mixing");
    Check(rootMeanSquare(surfLoop) > 1200.0 && rootMeanSquare(breezeLoop) > 900.0,
          "procedural ambience contains meaningful audible energy");
    Check(peakMagnitude(jungleLoop) < 32767 && rootMeanSquare(jungleLoop) > 700.0,
          "daytime-jungle ambience is audible while retaining mixing headroom");
    Check(peakMagnitude(waveAccent) < 32767 && rootMeanSquare(waveAccent) > 900.0,
          "stronger-wave accent is audible while retaining mixing headroom");
    Check(peakMagnitude(waveAccentAlternate) < 32767
              && rootMeanSquare(waveAccentAlternate) > 900.0,
          "alternate stronger wave is audible while retaining mixing headroom");
    Check(waveAccent.samples.front() == 0 && waveAccent.samples.back() == 0
              && waveAccentAlternate.samples.front() == 0
              && waveAccentAlternate.samples.back() == 0,
          "both stronger waves start and end at silence without a boundary click");
    Check(peakMagnitude(waterMovementLoop) < 32767
              && rootMeanSquare(waterMovementLoop) > 900.0,
          "player water movement is audible while retaining mixing headroom");
    Check(peakMagnitude(waterTransitionAccent) < 32767
              && rootMeanSquare(waterTransitionAccent) > 700.0,
          "water transition accent is audible while retaining mixing headroom");
    Check(peakMagnitude(waterTransitionAccentAlternate) < 32767
              && rootMeanSquare(waterTransitionAccentAlternate) > 700.0,
          "alternate water transition is audible while retaining mixing headroom");
    Check(waterTransitionAccent.samples.front() == 0
              && waterTransitionAccent.samples.back() == 0
              && waterTransitionAccentAlternate.samples.front() == 0
              && waterTransitionAccentAlternate.samples.back() == 0,
          "both water transition accents start and end at silence");
    Check(peakMagnitude(birdCall) < 32767 && rootMeanSquare(birdCall) > 1500.0
              && peakMagnitude(birdCallAlternate) < 32767
              && rootMeanSquare(birdCallAlternate) > 1500.0,
          "bird phrases carry audible energy while retaining headroom");
    Check(peakMagnitude(monkeyCall) < 32767 && rootMeanSquare(monkeyCall) > 1500.0
              && peakMagnitude(monkeyCallAlternate) < 32767
              && rootMeanSquare(monkeyCallAlternate) > 1500.0,
          "monkey phrases carry audible energy while retaining headroom");
    Check(birdCall.samples.front() == 0 && birdCall.samples.back() == 0
              && birdCallAlternate.samples.front() == 0
              && birdCallAlternate.samples.back() == 0
              && monkeyCall.samples.front() == 0 && monkeyCall.samples.back() == 0
              && monkeyCallAlternate.samples.front() == 0
              && monkeyCallAlternate.samples.back() == 0,
          "all wildlife phrases begin and end at silence");
    Check(std::abs(static_cast<int>(surfLoop.samples.front())
                   - static_cast<int>(surfLoop.samples.back())) < 8000,
          "surf loop boundary does not contain a large discontinuity");
    Check(std::abs(static_cast<int>(breezeLoop.samples.front())
                   - static_cast<int>(breezeLoop.samples.back())) < 8000,
          "breeze loop boundary does not contain a large discontinuity");
    Check(std::abs(static_cast<int>(jungleLoop.samples.front())
                   - static_cast<int>(jungleLoop.samples.back())) < 8000,
          "daytime-jungle loop boundary does not contain a large discontinuity");
    Check(std::abs(static_cast<int>(waterMovementLoop.samples.front())
                   - static_cast<int>(waterMovementLoop.samples.back())) < 8000,
          "player water movement loop has no large boundary discontinuity");

    const island::SoundscapeMix beachMix = island::SoundscapeModel::TargetMix(2.0f, false);
    const island::SoundscapeMix jungleMix = island::SoundscapeModel::TargetMix(140.0f, false);
    const island::SoundscapeMix swimmingMix = island::SoundscapeModel::TargetMix(-8.0f, true);
    const island::SoundscapeMix forcedInlandSwimmingMix =
        island::SoundscapeModel::TargetMix(140.0f, true);
    Check(beachMix.surfVolume > jungleMix.surfVolume,
          "surf is louder at the beach than deep inland");
    Check(jungleMix.breezeVolume > beachMix.breezeVolume,
          "canopy breeze grows toward the island interior");
    CheckNear(beachMix.jungleVolume, 0.0f, 0.0001f,
              "daytime-jungle texture stays out of the beach mix");
    Check(jungleMix.jungleVolume >= 0.075f
              && forcedInlandSwimmingMix.jungleVolume < jungleMix.jungleVolume * 0.1f,
          "jungle texture is audible inland and explicitly attenuated while swimming");
    Check(swimmingMix.surfVolume >= 0.35f && swimmingMix.breezeVolume < beachMix.breezeVolume,
          "swimming emphasizes nearby water while softening canopy breeze");
    island::SoundscapeModel soundscapeModel;
    soundscapeModel.Reset(2.0f, false);
    const float initialSurfVolume = soundscapeModel.Mix().surfVolume;
    for (int step = 0; step < 300; ++step) {
        (void)soundscapeModel.Update(140.0f, false, 1.0f / 60.0f);
    }
    Check(soundscapeModel.Mix().surfVolume < initialSurfVolume
              && soundscapeModel.Mix().surfVolume > jungleMix.surfVolume,
          "soundscape transitions smoothly toward the new zone instead of snapping");
    CheckNear(soundscapeModel.Mix().surfVolume, jungleMix.surfVolume, 0.025f,
              "soundscape approaches the inland surf target over several seconds");
    Check(soundscapeModel.Mix().jungleVolume > 0.045f
              && soundscapeModel.Mix().jungleVolume < jungleMix.jungleVolume,
          "the slower jungle layer fades in without a hard biome boundary");

    const island::WaterMovementMix dryMovement =
        island::WaterMovementModel::TargetMix(0.0f, false, 4.3f);
    const island::WaterMovementMix idleWater =
        island::WaterMovementModel::TargetMix(0.5f, true, 0.0f);
    const island::WaterMovementMix wadingMovement =
        island::WaterMovementModel::TargetMix(0.24f, false, 3.0f);
    const island::WaterMovementMix swimmingMovement =
        island::WaterMovementModel::TargetMix(0.7f, true, 2.0f);
    const island::WaterMovementMix idleCurrent =
        island::WaterMovementModel::TargetMix(2.5f, true, 0.0f, 1.0f);
    const island::WaterMovementMix dryCurrent =
        island::WaterMovementModel::TargetMix(0.0f, false, 4.3f, 1.0f);
    const island::WaterMovementMix invalidCurrent =
        island::WaterMovementModel::TargetMix(
            2.5f, true, 0.0f, std::numeric_limits<float>::quiet_NaN());
    CheckNear(dryMovement.volume, 0.0f, 0.0001f,
              "dry walking never activates the player water layer");
    CheckNear(idleWater.volume, 0.0f, 0.0001f,
              "standing still in water does not emit continuous movement noise");
    Check(wadingMovement.volume > 0.0f && swimmingMovement.volume > wadingMovement.volume,
          "wading is audible and active swimming carries the stronger water mix");
    Check(swimmingMovement.pitch < wadingMovement.pitch,
          "swimming shifts the shared water texture below the wading pitch");
    Check(idleCurrent.currentCue == 1.0f
              && idleCurrent.volume >= 0.075f && idleCurrent.volume <= 0.081f
              && idleCurrent.pitch < swimmingMovement.pitch,
          "full offshore current reuses the water loop as a bounded deeper undertow cue");
    CheckNear(dryCurrent.currentCue, 0.0f, 0.0001f,
              "ocean-current audio remains disabled outside swimming");
    CheckNear(invalidCurrent.currentCue, 0.0f, 0.0001f,
              "non-finite ocean-current audio input degrades to the calm mix");
    island::WaterMovementModel waterMovementModel;
    waterMovementModel.Reset(0.0f, false, 0.0f);
    const island::WaterMovementMix firstWadingFrame =
        waterMovementModel.Update(0.24f, false, 3.0f, 1.0f / 60.0f);
    Check(firstWadingFrame.volume > 0.0f
              && firstWadingFrame.volume < wadingMovement.volume,
          "water movement fades in instead of starting with a hard gain edge");
    for (int step = 0; step < 180; ++step) {
        (void)waterMovementModel.Update(0.24f, false, 3.0f, 1.0f / 60.0f);
    }
    CheckNear(waterMovementModel.Mix().volume, wadingMovement.volume, 0.002f,
              "water movement converges to its speed and depth target");
    const float movingWaterVolume = waterMovementModel.Mix().volume;
    const island::WaterMovementMix firstDryFrame =
        waterMovementModel.Update(0.0f, false, 0.0f, 1.0f / 60.0f);
    Check(firstDryFrame.volume > 0.0f && firstDryFrame.volume < movingWaterVolume,
          "water movement fades out without rapid stop/start chatter");
    waterMovementModel.Reset(2.5f, true, 0.0f, 0.0f);
    const island::WaterMovementMix firstCurrentFrame = waterMovementModel.Update(
        2.5f, true, 0.0f, 1.0f / 60.0f, 1.0f);
    Check(firstCurrentFrame.currentCue > 0.0f && firstCurrentFrame.currentCue < 0.03f
              && firstCurrentFrame.volume > 0.0f && firstCurrentFrame.volume < 0.001f,
          "undertow audio fades in instead of snapping at the current threshold");
    for (int step = 0; step < 240; ++step) {
        (void)waterMovementModel.Update(2.5f, true, 0.0f, 1.0f / 60.0f, 1.0f);
    }
    CheckNear(waterMovementModel.Mix().currentCue, 1.0f, 0.003f,
              "undertow audio converges to the full-current target smoothly");
    const island::WaterMovementMix firstCalmFrame = waterMovementModel.Update(
        2.5f, true, 0.0f, 1.0f / 60.0f, 0.0f);
    Check(firstCalmFrame.currentCue > 0.0f && firstCalmFrame.currentCue < 1.0f,
          "undertow audio releases smoothly after leaving the current field");
    for (int step = 0; step < 360; ++step) {
        (void)waterMovementModel.Update(2.5f, true, 0.0f, 1.0f / 60.0f, 0.0f);
    }
    CheckNear(waterMovementModel.Mix().currentCue, 0.0f, 0.0002f,
              "undertow cue fully returns to the calm stationary-water state");
    CheckNear(waterMovementModel.Mix().volume, 0.0f, 0.0002f,
              "undertow audio leaves no residual gain in calm stationary water");

    island::WaterTransitionScheduler waterTransitionScheduler;
    waterTransitionScheduler.Reset(0.0f, false);
    const island::WaterTransitionRequest waterEntry =
        waterTransitionScheduler.Update(0.12f, false, 3.0f, 1.0f / 60.0f);
    Check(waterEntry.type == island::WaterTransitionType::Enter,
          "crossing the wet threshold emits one water-entry accent");
    Check(waterEntry.variant <= 1U
              && waterEntry.volume >= 0.055f && waterEntry.volume <= 0.14f
              && waterEntry.pitch >= -0.055f && waterEntry.pitch <= -0.005f
              && waterEntry.pan >= -0.18f && waterEntry.pan <= 0.18f,
          "water-entry variation remains inside conservative playback bounds");
    const island::WaterTransitionRequest rapidExit =
        waterTransitionScheduler.Update(0.0f, false, 3.0f, 0.1f);
    Check(rapidExit.type == island::WaterTransitionType::None,
          "water transition cooldown prevents overlapping shoreline chatter");
    for (int step = 0; step < 7; ++step) {
        (void)waterTransitionScheduler.Update(0.0f, false, 0.0f, 0.25f);
    }
    const island::WaterTransitionRequest secondEntry =
        waterTransitionScheduler.Update(0.12f, false, 2.0f, 0.01f);
    Check(secondEntry.type == island::WaterTransitionType::Enter,
          "water entry becomes eligible again after the authored cooldown");
    waterTransitionScheduler.Reset(0.2f, false);
    const island::WaterTransitionRequest waterExit =
        waterTransitionScheduler.Update(0.0f, false, 2.0f, 0.01f);
    Check(waterExit.type == island::WaterTransitionType::Exit
              && waterExit.pitch >= 0.025f && waterExit.pitch <= 0.075f,
          "leaving shallow water emits the distinct higher-pitched exit accent");
    waterTransitionScheduler.Reset(0.0f, false);
    const island::WaterTransitionRequest stationaryEntry =
        waterTransitionScheduler.Update(0.12f, false, 0.0f, 0.01f);
    Check(stationaryEntry.type == island::WaterTransitionType::None,
          "stationary water-state changes do not emit a teleport-like splash");
    island::WaterTransitionScheduler transitionVariantScheduler;
    transitionVariantScheduler.Reset(0.0f, false);
    std::array<bool, 2> observedTransitionVariants = {false, false};
    bool transitionVariantOutOfRange = false;
    for (int crossing = 0; crossing < 10; ++crossing) {
        const bool entering = crossing % 2 == 0;
        const island::WaterTransitionRequest transition = transitionVariantScheduler.Update(
            entering ? 0.12f : 0.0f, false, 2.5f, 0.01f);
        if (transition.type != island::WaterTransitionType::None) {
            if (transition.variant < observedTransitionVariants.size()) {
                observedTransitionVariants[transition.variant] = true;
            } else {
                transitionVariantOutOfRange = true;
            }
        }
        for (int cooldownStep = 0; cooldownStep < 7; ++cooldownStep) {
            (void)transitionVariantScheduler.Update(
                entering ? 0.12f : 0.0f, false, 0.0f, 0.25f);
        }
    }
    Check(!transitionVariantOutOfRange
              && observedTransitionVariants[0] && observedTransitionVariants[1],
          "deterministic shoreline scheduling reaches both source variants");

    island::WaveAccentScheduler waveScheduler;
    waveScheduler.Reset();
    island::WaveAccentRequest firstWave;
    for (int step = 0; step < 60 && !firstWave.trigger; ++step) {
        firstWave = waveScheduler.Update(0.25f, beachMix.surfVolume);
    }
    Check(firstWave.trigger, "shoreline scheduler emits its first wave within the authored window");
    Check(firstWave.variant <= 1U
              && firstWave.volume >= 0.06f && firstWave.volume <= 0.22f
              && firstWave.pitch >= -0.06f && firstWave.pitch <= 0.06f
              && firstWave.pan >= -0.32f && firstWave.pan <= 0.32f,
          "wave request variation remains inside conservative playback bounds");
    bool overlappedWave = false;
    for (int step = 0; step < 52; ++step) {
        overlappedWave = overlappedWave
            || waveScheduler.Update(0.25f, beachMix.surfVolume).trigger;
    }
    Check(!overlappedWave,
          "wave scheduler cannot overlap a four-second accent or repeat it too quickly");
    waveScheduler.Reset();
    bool inlandWave = false;
    for (int step = 0; step < 80; ++step) {
        inlandWave = inlandWave || waveScheduler.Update(0.25f, jungleMix.surfVolume).trigger;
    }
    Check(!inlandWave, "quiet inland ambience virtualizes stronger-wave accents");
    Check(waveScheduler.Update(0.25f, beachMix.surfVolume).trigger,
          "returning to audible surf permits a waiting wave accent");
    island::WaveAccentScheduler waveVariantScheduler;
    waveVariantScheduler.Reset();
    std::array<bool, 2> observedWaveVariants = {false, false};
    bool waveVariantOutOfRange = false;
    for (int step = 0; step < 1600; ++step) {
        const island::WaveAccentRequest wave =
            waveVariantScheduler.Update(0.25f, beachMix.surfVolume);
        if (wave.trigger) {
            if (wave.variant < observedWaveVariants.size()) {
                observedWaveVariants[wave.variant] = true;
            } else {
                waveVariantOutOfRange = true;
            }
        }
    }
    Check(!waveVariantOutOfRange && observedWaveVariants[0] && observedWaveVariants[1],
          "deterministic stronger-wave scheduling reaches both source variants");

    const island::WildlifeAudioSource nearbyBird{true, 28.0f, 16.0f, -8.0f};
    const island::WildlifeAudioSource nearbyMonkey{true, -18.0f, 2.0f, 21.0f};
    island::WildlifeCallScheduler heldWildlifeScheduler;
    heldWildlifeScheduler.Reset();
    bool callEscapedBusySlots = false;
    for (int step = 0; step < 80; ++step) {
        callEscapedBusySlots = callEscapedBusySlots
            || heldWildlifeScheduler.Update(0.25f, false, 90.0f, 0.0f, 2.0f, 0.0f,
                                            nearbyBird, nearbyMonkey, false).type
                != island::WildlifeCallType::None;
    }
    Check(!callEscapedBusySlots,
          "wildlife scheduling never exceeds the transient playback-slot budget");
    const island::WildlifeCallRequest releasedCall = heldWildlifeScheduler.Update(
        0.01f, false, 90.0f, 0.0f, 2.0f, 0.0f, nearbyBird, nearbyMonkey, true);
    Check(releasedCall.type != island::WildlifeCallType::None,
          "a due wildlife call resumes when a transient slot becomes available");

    island::WildlifeCallScheduler swimmingMonkeyScheduler;
    swimmingMonkeyScheduler.Reset();
    bool monkeyCalledWhileSwimming = false;
    const island::WildlifeAudioSource noBird;
    for (int step = 0; step < 160; ++step) {
        monkeyCalledWhileSwimming = monkeyCalledWhileSwimming
            || swimmingMonkeyScheduler.Update(
                   0.25f, true, -4.0f, 0.0f, 2.0f, 0.0f, noBird, nearbyMonkey, true).type
                == island::WildlifeCallType::Monkey;
    }
    Check(!monkeyCalledWhileSwimming,
          "interior monkey calls are suppressed while the player is swimming");

    island::WildlifeCallScheduler distantWildlifeScheduler;
    distantWildlifeScheduler.Reset();
    const island::WildlifeAudioSource distantBird{true, 440.0f, 80.0f, -310.0f};
    bool distantCallTriggered = false;
    for (int step = 0; step < 240; ++step) {
        distantCallTriggered = distantCallTriggered
            || distantWildlifeScheduler.Update(
                   0.25f, false, 90.0f, 0.0f, 2.0f, 0.0f,
                   distantBird, noBird, true).type != island::WildlifeCallType::None;
    }
    Check(!distantCallTriggered,
          "distant wildlife is virtualized instead of consuming an inaudible voice");

    island::WildlifeCallScheduler longWildlifeScheduler;
    longWildlifeScheduler.Reset();
    int longBirdCallCount = 0;
    int longMonkeyCallCount = 0;
    std::array<bool, 2> longBirdCallVariants = {false, false};
    std::array<bool, 2> longMonkeyCallVariants = {false, false};
    bool wildlifeRequestBoundsValid = true;
    for (int step = 0; step < 20 * 60 * 4; ++step) {
        const island::WildlifeCallRequest call = longWildlifeScheduler.Update(
            0.25f, false, 90.0f, 0.0f, 2.0f, 0.0f,
            nearbyBird, nearbyMonkey, true);
        if (call.type == island::WildlifeCallType::None) {
            continue;
        }
        wildlifeRequestBoundsValid = wildlifeRequestBoundsValid
            && call.variant <= 1U && call.source.available
            && call.volume >= 0.36f && call.volume <= 0.60f
            && call.pitch >= -0.08f && call.pitch <= 0.09f;
        if (call.type == island::WildlifeCallType::Bird) {
            ++longBirdCallCount;
            if (call.variant < longBirdCallVariants.size()) {
                longBirdCallVariants[call.variant] = true;
            }
        } else if (call.type == island::WildlifeCallType::Monkey) {
            ++longMonkeyCallCount;
            if (call.variant < longMonkeyCallVariants.size()) {
                longMonkeyCallVariants[call.variant] = true;
            }
        }
    }
    Check(wildlifeRequestBoundsValid
              && longBirdCallVariants[0] && longBirdCallVariants[1]
              && longMonkeyCallVariants[0] && longMonkeyCallVariants[1],
          "twenty-minute wildlife scheduling keeps playback values bounded and varies sources");
    Check(longBirdCallCount >= 20 && longBirdCallCount <= 70
              && longMonkeyCallCount >= 5 && longMonkeyCallCount <= 30,
          "wildlife calls remain sparse over a twenty-minute session");

    island::SoundscapeModel longSoundscapeModel;
    island::WaterMovementModel longWaterMovementModel;
    island::WaveAccentScheduler longWaveScheduler;
    island::WaterTransitionScheduler longTransitionScheduler;
    longSoundscapeModel.Reset(2.0f, false);
    longWaterMovementModel.Reset(0.0f, false, 0.0f);
    longWaveScheduler.Reset();
    longTransitionScheduler.Reset(0.0f, false);
    constexpr float longStepSeconds = 1.0f / 60.0f;
    constexpr int longSessionSteps = 20 * 60 * 60;
    constexpr int biomeBlockSteps = 20 * 60;
    bool longSessionStable = true;
    int longWaveCount = 0;
    int longTransitionCount = 0;
    std::array<bool, 2> longWaveVariants = {false, false};
    std::array<bool, 2> longTransitionVariants = {false, false};
    for (int step = 0; step < longSessionSteps; ++step) {
        const int block = (step / biomeBlockSteps) % 4;
        const int blockStep = step % biomeBlockSteps;
        const float signedShoreDistance = block == 0 ? 2.0f
            : block == 1 ? 140.0f
            : block == 2 ? -8.0f : 8.0f;
        const bool swimming = block == 2;
        const float waterDepth = block == 2 ? 0.7f
            : block == 3 && blockStep < biomeBlockSteps / 2 ? 0.12f : 0.0f;
        const float speed = block == 1 ? 2.8f : swimming ? 1.8f : 3.2f;
        const float oceanCurrentStrength = swimming
            ? static_cast<float>(blockStep) / static_cast<float>(biomeBlockSteps - 1)
            : 0.0f;
        const island::SoundscapeMix longMix = longSoundscapeModel.Update(
            signedShoreDistance, swimming, longStepSeconds);
        const island::WaterMovementMix longWaterMix = longWaterMovementModel.Update(
            waterDepth, swimming, speed, longStepSeconds, oceanCurrentStrength);
        const island::WaveAccentRequest longWave = longWaveScheduler.Update(
            longStepSeconds, longMix.surfVolume);
        const island::WaterTransitionRequest longTransition = longTransitionScheduler.Update(
            waterDepth, swimming, speed, longStepSeconds);

        longSessionStable = longSessionStable
            && std::isfinite(longMix.surfVolume) && longMix.surfVolume >= 0.0f
            && longMix.surfVolume <= 0.40f
            && std::isfinite(longMix.breezeVolume) && longMix.breezeVolume >= 0.0f
            && longMix.breezeVolume <= 0.20f
            && std::isfinite(longMix.jungleVolume) && longMix.jungleVolume >= 0.0f
            && longMix.jungleVolume <= 0.08f
            && std::isfinite(longWaterMix.volume) && longWaterMix.volume >= 0.0f
            && longWaterMix.volume <= 0.18f
            && std::isfinite(longWaterMix.pitch) && longWaterMix.pitch >= -0.23f
            && longWaterMix.pitch <= 0.15f
            && std::isfinite(longWaterMix.currentCue)
            && longWaterMix.currentCue >= 0.0f && longWaterMix.currentCue <= 1.0f;
        if (longWave.trigger) {
            ++longWaveCount;
            if (longWave.variant < longWaveVariants.size()) {
                longWaveVariants[longWave.variant] = true;
            } else {
                longSessionStable = false;
            }
        }
        if (longTransition.type != island::WaterTransitionType::None) {
            ++longTransitionCount;
            if (longTransition.variant < longTransitionVariants.size()) {
                longTransitionVariants[longTransition.variant] = true;
            } else {
                longSessionStable = false;
            }
        }
    }
    Check(longSessionStable,
          "twenty-minute soundscape simulation remains finite and inside mix bounds");
    Check(longWaveCount >= 20 && longWaveCount <= 80
              && longWaveVariants[0] && longWaveVariants[1],
          "long soundscape simulation keeps wave activity bounded and varied");
    Check(longTransitionCount == 30
              && longTransitionVariants[0] && longTransitionVariants[1],
          "long soundscape simulation emits exactly one varied accent per water crossing");

    island::TouchControls touchControls;
    std::array<island::TouchContact, 2> touchContacts = {{
        {11, 180.0f, 520.0f, island::TouchPhase::Pressed},
        {23, 1000.0f, 360.0f, island::TouchPhase::Pressed},
    }};
    island::TouchControlState touchState = touchControls.Update(touchContacts, 1280.0f, 720.0f);
    Check(touchState.movementActive && touchState.lookActive,
          "touch controls assign simultaneous left movement and right look contacts");
    CheckNear(touchState.moveForward, 0.0f, 0.0001f,
              "movement touch begins at a neutral floating-stick anchor");
    CheckNear(touchState.lookDeltaX, 0.0f, 0.0001f,
              "look touch begins without a camera jump");
    CheckNear(touchState.lookTouchX, 1000.0f, 0.0001f,
              "look presentation tracks its initial contact position");

    touchContacts[0] = {11, 230.0f, 440.0f, island::TouchPhase::Moved};
    touchContacts[1] = {23, 1032.0f, 344.0f, island::TouchPhase::Moved};
    touchState = touchControls.Update(touchContacts, 1280.0f, 720.0f);
    Check(touchState.moveForward > 0.4f && touchState.moveRight > 0.2f,
          "left touch produces proportional forward and strafe movement");
    CheckNear(touchState.lookDeltaX, 32.0f, 0.0001f,
              "right touch produces relative horizontal look delta");
    CheckNear(touchState.lookDeltaY, -16.0f, 0.0001f,
              "right touch produces relative vertical look delta");
    CheckNear(touchState.lookTouchX, 1032.0f, 0.0001f,
              "look presentation follows the owned contact");
    CheckNear(touchState.lookTouchY, 344.0f, 0.0001f,
              "look presentation preserves vertical contact position");

    touchContacts[0] = {11, 900.0f, -200.0f, island::TouchPhase::Moved};
    touchState = touchControls.Update(touchContacts, 1280.0f, 720.0f);
    Check(std::hypot(touchState.moveRight, touchState.moveForward) <= 1.0001f,
          "movement stick clamps extreme drags to unit magnitude");
    Check(touchState.movementActive && touchState.lookActive,
          "touch roles remain owned after contacts cross the screen centre");

    const std::array<island::TouchContact, 1> releasedMovement = {{
        {11, 900.0f, -200.0f, island::TouchPhase::Released},
    }};
    touchState = touchControls.Update(releasedMovement, 1280.0f, 720.0f);
    Check(!touchState.movementActive && !touchState.lookActive,
          "released and missing contacts cancel both touch roles");
    CheckNear(touchState.moveForward, 0.0f, 0.0001f,
              "cancelled movement returns neutral output");

    const std::array<island::TouchContact, 1> tinyMovement = {{
        {31, 160.0f, 500.0f, island::TouchPhase::Pressed},
    }};
    (void)touchControls.Update(tinyMovement, 1280.0f, 720.0f);
    const std::array<island::TouchContact, 1> tinyMovementMoved = {{
        {31, 164.0f, 496.0f, island::TouchPhase::Moved},
    }};
    touchState = touchControls.Update(tinyMovementMoved, 1280.0f, 720.0f);
    CheckNear(touchState.moveForward, 0.0f, 0.0001f,
              "movement dead zone rejects small finger jitter");
    touchState = touchControls.Update(tinyMovementMoved, 1000.0f, 600.0f);
    Check(touchState.movementActive && touchState.moveForward == 0.0f,
          "display resize safely re-anchors an active movement contact");

    const island::IslandTerrain terrain;

    const island::DistantSceneryLayout distantScenery;
    const island::DistantSceneryLayout repeatedDistantScenery;
    Check(distantScenery.Islets().size() == island::DistantSceneryLayout::IsletCount,
          "distant scenery keeps its intentionally small bounded population");
    for (std::size_t index = 0; index < distantScenery.Islets().size(); ++index) {
        const island::DistantIsletPlacement& islet = distantScenery.Islets()[index];
        const island::DistantIsletPlacement& repeated = repeatedDistantScenery.Islets()[index];
        Check(islet.x == repeated.x && islet.z == repeated.z
                  && islet.radiusX == repeated.radiusX
                  && islet.radiusZ == repeated.radiusZ
                  && islet.height == repeated.height
                  && islet.yawRadians == repeated.yawRadians
                  && islet.variant == repeated.variant,
              "distant scenery placement is exactly deterministic");
        Check(std::hypot(islet.x, islet.z) >= 570.0f,
              "distant islets remain far beyond the playable island");
        Check(terrain.SignedShoreDistance(islet.x, islet.z)
                  < -(island::PlayerMotion::SafetyDistance + 150.0f),
              "distant islets remain well beyond the swimming safety boundary");
        Check(std::abs(islet.x) + islet.radiusX
                      < island::OceanMeshLayout::Extent - 20.0f
                  && std::abs(islet.z) + islet.radiusZ
                      < island::OceanMeshLayout::Extent - 20.0f,
              "distant islets remain inside the finite ocean surface");
        Check(islet.radiusX >= 24.0f && islet.radiusX <= 42.0f
                  && islet.radiusZ >= 17.0f && islet.radiusZ <= 29.0f
                  && islet.height >= 5.9f && islet.height <= 10.5f
                  && islet.variant < island::DistantSceneryLayout::VariantCount,
              "distant islet dimensions and variants stay within the authored low silhouette");
        for (std::size_t other = index + 1U;
             other < distantScenery.Islets().size(); ++other) {
            const island::DistantIsletPlacement& neighbour = distantScenery.Islets()[other];
            Check(std::hypot(islet.x - neighbour.x, islet.z - neighbour.z)
                      > islet.radiusX + neighbour.radiusX + 20.0f,
                  "distant islet silhouettes remain clearly separated on the horizon");
        }
    }

    Check(island::WildlifeActivationPolicy::Classify(25.0f, 100.0f, 250.0f)
              == island::WildlifeSimulationTier::Full,
          "near wildlife receives full-rate simulation");
    Check(island::WildlifeActivationPolicy::Classify(175.0f, 100.0f, 250.0f)
              == island::WildlifeSimulationTier::Reduced,
          "mid-distance wildlife receives reduced-rate simulation");
    Check(island::WildlifeActivationPolicy::Classify(275.0f, 100.0f, 250.0f)
              == island::WildlifeSimulationTier::Dormant,
          "far wildlife receives dormant-rate simulation");
    const float quietNan = std::numeric_limits<float>::quiet_NaN();
    Check(island::WildlifeActivationPolicy::Classify(quietNan, 100.0f, 250.0f)
              == island::WildlifeSimulationTier::Dormant,
          "non-finite observer distance fails closed to dormant wildlife simulation");
    Check(island::WildlifeActivationPolicy::Classify(1.0f, quietNan, quietNan)
              == island::WildlifeSimulationTier::Dormant,
          "non-finite wildlife thresholds fail closed to a zero-distance envelope");
    Check(island::WildlifeActivationPolicy::UpdateDivisor(
              island::WildlifeSimulationTier::Full) == 1U
              && island::WildlifeActivationPolicy::UpdateDivisor(
                  island::WildlifeSimulationTier::Reduced) == 4U
              && island::WildlifeActivationPolicy::UpdateDivisor(
                  island::WildlifeSimulationTier::Dormant) == 30U,
          "wildlife tiers use the documented bounded update divisors");

    island::BirdFlock birds(terrain);
    island::BirdFlock repeatedBirds(terrain);
    island::BirdFlock invalidInputBirds(terrain);
    invalidInputBirds.Update(quietNan, quietNan, quietNan, quietNan);
    Check(invalidInputBirds.DormantSimulationCount()
              == static_cast<int>(island::BirdFlock::BirdCount)
              && std::ranges::all_of(invalidInputBirds.Birds(), [](const auto& state) {
                     return std::isfinite(state.x) && std::isfinite(state.y)
                         && std::isfinite(state.z) && std::isfinite(state.headingRadians);
                 }),
          "bird simulation rejects non-finite frame and observer input without state poisoning");
    Check(birds.Birds().size() == island::BirdFlock::BirdCount,
          "bird flock has the intended bounded population");
    Check(birds.Routes().size() == island::BirdFlock::BirdCount,
          "bird flock exposes one authoritative route per agent");
    Check(birds.RouteRegions().size() == 3U,
          "bird flock exposes the three authored route-family regions");
    int shoreBirds = 0;
    int canopyBirds = 0;
    int highBirds = 0;
    for (std::size_t index = 0; index < birds.Birds().size(); ++index) {
        const island::BirdState& bird = birds.Birds()[index];
        const island::BirdState& repeated = repeatedBirds.Birds()[index];
        const island::BirdRouteState& route = birds.Routes()[index];
        const island::BirdRouteState& repeatedRoute = repeatedBirds.Routes()[index];
        Check(bird.x == repeated.x && bird.y == repeated.y && bird.z == repeated.z
                  && bird.headingRadians == repeated.headingRadians
                  && bird.speed == repeated.speed && bird.scale == repeated.scale
                  && bird.bankRadians == repeated.bankRadians
                  && bird.glideAmount == repeated.glideAmount
                  && bird.family == repeated.family
                  && bird.colorVariant == repeated.colorVariant,
              "bird initialization is deterministic");
        Check(route.spawnX == repeatedRoute.spawnX
                  && route.spawnY == repeatedRoute.spawnY
                  && route.spawnZ == repeatedRoute.spawnZ
                  && route.spawnHeadingRadians == repeatedRoute.spawnHeadingRadians
                  && route.preferredRadius == repeatedRoute.preferredRadius
                  && route.baseAltitude == repeatedRoute.baseAltitude
                  && route.minimumClearance == repeatedRoute.minimumClearance
                  && route.verticalAmplitude == repeatedRoute.verticalAmplitude
                  && route.direction == repeatedRoute.direction
                  && route.family == repeatedRoute.family,
              "bird route diagnostics initialize deterministically");
        Check(route.spawnX == bird.x && route.spawnY == bird.y && route.spawnZ == bird.z
                  && route.spawnHeadingRadians == bird.headingRadians,
              "bird route diagnostics retain the exact authored spawn pose");
        Check(route.family == bird.family
                  && route.preferredRadius >= 70.0f && route.preferredRadius <= 248.0f
                  && route.baseAltitude >= 28.0f && route.baseAltitude <= 95.0f
                  && route.minimumClearance >= 15.0f
                  && route.verticalAmplitude >= 2.0f
                  && (route.direction == -1 || route.direction == 1),
              "bird route diagnostics retain bounded steering inputs");
        Check(std::isfinite(bird.x) && std::isfinite(bird.y) && std::isfinite(bird.z),
              "bird initial positions are finite");
        Check(bird.y >= terrain.Height(bird.x, bird.z) + 14.9f,
              "bird initial positions clear the terrain and canopy");
        Check(bird.scale >= 0.7f && bird.scale <= 1.2f,
              "bird scale variation stays within the authored range");
        switch (bird.family) {
        case island::BirdPathFamily::Shore:
            ++shoreBirds;
            break;
        case island::BirdPathFamily::Canopy:
            ++canopyBirds;
            break;
        case island::BirdPathFamily::High:
            ++highBirds;
            break;
        }
    }
    Check(shoreBirds == 10 && canopyBirds == 8 && highBirds == 6,
          "bird flock combines shore, canopy, and high route families");
    const std::array<std::size_t, 3> expectedBirdRegionPopulations = {10U, 8U, 6U};
    for (std::size_t index = 0; index < birds.RouteRegions().size(); ++index) {
        const island::BirdRouteRegion& region = birds.RouteRegions()[index];
        const island::BirdRouteRegion& repeated = repeatedBirds.RouteRegions()[index];
        Check(region.minimumRadius == repeated.minimumRadius
                  && region.maximumRadius == repeated.maximumRadius
                  && region.minimumAltitude == repeated.minimumAltitude
                  && region.maximumAltitude == repeated.maximumAltitude
                  && region.population == repeated.population
                  && region.family == repeated.family,
              "bird route-family regions initialize deterministically");
        Check(region.population == expectedBirdRegionPopulations[index]
                  && region.minimumRadius <= region.maximumRadius
                  && region.minimumAltitude < region.maximumAltitude,
              "bird route-family regions account for bounded authored volumes");
    }
    CheckNear(birds.RouteRegions()[0].minimumRadius, 218.0f, 0.0001f,
              "shore-bird route region begins at the authored inner orbit");
    CheckNear(birds.RouteRegions()[0].maximumRadius, 248.0f, 0.0001f,
              "shore-bird route region ends at the authored outer orbit");
    CheckNear(birds.RouteRegions()[1].minimumRadius, 70.0f, 0.0001f,
              "canopy-bird route region begins at the authored inner orbit");
    CheckNear(birds.RouteRegions()[1].maximumRadius, 150.0f, 0.0001f,
              "canopy-bird route region ends at the authored outer orbit");
    CheckNear(birds.RouteRegions()[2].minimumRadius, 125.0f, 0.0001f,
              "high-bird route region begins at the authored inner orbit");
    CheckNear(birds.RouteRegions()[2].maximumRadius, 233.0f, 0.0001f,
              "high-bird route region ends at the authored outer orbit");

    float maximumObservedBirdRadius = 0.0f;
    float minimumObservedBirdRadius = 1000.0f;
    bool observedWingUp = false;
    bool observedWingDown = false;
    bool observedStrongGlide = false;
    bool observedLeftBank = false;
    bool observedRightBank = false;
    constexpr int birdSimulationFrames = 36000;
    for (int frame = 0; frame < birdSimulationFrames; ++frame) {
        birds.Update(1.0f / 30.0f, 0.0f, 15.0f, 0.0f);
        repeatedBirds.Update(1.0f / 30.0f, 0.0f, 15.0f, 0.0f);
        const auto birdStates = birds.Birds();
        const auto repeatedStates = repeatedBirds.Birds();
        for (std::size_t index = 0; index < birdStates.size(); ++index) {
            const island::BirdState& bird = birdStates[index];
            const island::BirdState& repeated = repeatedStates[index];
            Check(bird.x == repeated.x && bird.y == repeated.y && bird.z == repeated.z
                      && bird.headingRadians == repeated.headingRadians
                      && bird.wingPose == repeated.wingPose
                      && bird.bankRadians == repeated.bankRadians
                      && bird.glideAmount == repeated.glideAmount
                      && bird.simulationTier == repeated.simulationTier,
                  "bird simulation remains deterministic over a long session");
            const float radius = std::hypot(bird.x, bird.z);
            maximumObservedBirdRadius = std::max(maximumObservedBirdRadius, radius);
            minimumObservedBirdRadius = std::min(minimumObservedBirdRadius, radius);
            Check(std::isfinite(bird.headingRadians) && std::isfinite(bird.wingPose)
                      && std::isfinite(bird.bankRadians) && std::isfinite(bird.glideAmount),
                  "bird flight pose remains finite over a long session");
            Check(std::fabs(bird.bankRadians) <= 0.341f
                      && bird.glideAmount >= 0.0f && bird.glideAmount <= 1.0f,
                  "bird banking and glide blend remain inside portable pose bounds");
            Check(bird.y >= terrain.Height(bird.x, bird.z) + 14.8f,
                  "bird flight maintains its minimum terrain clearance");
            observedWingUp = observedWingUp || bird.wingPose > 0.55f;
            observedWingDown = observedWingDown || bird.wingPose < -0.55f;
            observedStrongGlide = observedStrongGlide || bird.glideAmount > 0.92f;
            observedLeftBank = observedLeftBank || bird.bankRadians < -0.055f;
            observedRightBank = observedRightBank || bird.bankRadians > 0.055f;
        }
    }
    Check(minimumObservedBirdRadius > 35.0f && maximumObservedBirdRadius < 315.0f,
          "all bird route families remain in the authored island airspace");
    Check(observedWingUp && observedWingDown,
          "bird animation visits both wing stroke extremes");
    Check(observedStrongGlide,
          "bird animation enters sustained intermittent glide periods");
    Check(observedLeftBank && observedRightBank,
          "opposed route directions produce visible banking in both directions");
    Check(birds.FullSimulationCount() + birds.ReducedSimulationCount()
              + birds.DormantSimulationCount() == static_cast<int>(island::BirdFlock::BirdCount),
          "bird simulation tiers account for the complete flock");

    island::FishSchools fishSchools(terrain);
    island::FishSchools repeatedFishSchools(terrain);
    island::FishSchools invalidInputFish(terrain);
    invalidInputFish.Update(quietNan, quietNan, quietNan, quietNan);
    Check(invalidInputFish.DormantSimulationCount()
              == static_cast<int>(island::FishSchools::FishCount)
              && std::ranges::all_of(invalidInputFish.Fish(), [](const auto& state) {
                     return std::isfinite(state.x) && std::isfinite(state.y)
                         && std::isfinite(state.z) && std::isfinite(state.headingRadians);
                 }),
          "fish simulation rejects non-finite frame and observer input without state poisoning");
    Check(fishSchools.Fish().size() == island::FishSchools::FishCount,
          "fish schools have the intended bounded population");
    Check(fishSchools.Schools().size() == island::FishSchools::SchoolCount,
          "fish navigation exposes every authoritative school centre");
    for (std::size_t index = 0; index < fishSchools.Schools().size(); ++index) {
        const island::FishSchoolState& school = fishSchools.Schools()[index];
        const island::FishSchoolState& repeated = repeatedFishSchools.Schools()[index];
        const island::TerrainSample sample = terrain.Sample(school.centreX, school.centreZ);
        Check(school.centreX == repeated.centreX && school.centreY == repeated.centreY
                  && school.centreZ == repeated.centreZ
                  && school.direction == repeated.direction,
              "fish school centres initialize deterministically");
        Check(std::isfinite(school.centreX) && std::isfinite(school.centreY)
                  && std::isfinite(school.centreZ),
              "fish school centre diagnostics remain finite");
        Check((school.direction == -1 || school.direction == 1)
                  && sample.signedShoreDistance <= -2.8f
                  && sample.signedShoreDistance >= -48.2f,
              "fish school centres stay in the authored water belt");
    }
    std::array<int, island::FishSchools::SchoolCount> fishPerSchool{};
    for (std::size_t index = 0; index < fishSchools.Fish().size(); ++index) {
        const island::FishState& fish = fishSchools.Fish()[index];
        const island::FishState& repeated = repeatedFishSchools.Fish()[index];
        const island::TerrainSample sample = terrain.Sample(fish.x, fish.z);
        Check(fish.x == repeated.x && fish.y == repeated.y && fish.z == repeated.z
                  && fish.headingRadians == repeated.headingRadians
                  && fish.pitchRadians == repeated.pitchRadians
                  && fish.bankRadians == repeated.bankRadians
                  && fish.bodyWave == repeated.bodyWave && fish.speed == repeated.speed
                  && fish.scale == repeated.scale && fish.schoolIndex == repeated.schoolIndex
                  && fish.colorVariant == repeated.colorVariant,
              "fish initialization is deterministic");
        Check(fish.schoolIndex < island::FishSchools::SchoolCount,
              "fish school indices stay in range");
        ++fishPerSchool[fish.schoolIndex];
        Check(sample.signedShoreDistance <= -2.8f
                  && sample.signedShoreDistance >= -48.2f,
              "fish initialize inside the authored near-shore water belt");
        Check(sample.height <= -0.5f && fish.y >= sample.height + 0.20f
                  && fish.y <= -0.20f,
              "fish initialize between the seabed and water surface");
    }
    for (const int population : fishPerSchool) {
        Check(population == static_cast<int>(island::FishSchools::FishPerSchool),
              "each deterministic fish school has the intended population");
    }

    const island::FishState initialLeadFish = fishSchools.Fish().front();
    bool observedFishTailLeft = false;
    bool observedFishTailRight = false;
    bool observedFishPitchUp = false;
    bool observedFishPitchDown = false;
    bool observedFishLeftBank = false;
    bool observedFishRightBank = false;
    constexpr int fishSimulationFrames = 18000;
    for (int frame = 0; frame < fishSimulationFrames; ++frame) {
        const island::FishState observerFish = fishSchools.Fish().front();
        fishSchools.Update(1.0f / 30.0f, observerFish.x, observerFish.y, observerFish.z);
        repeatedFishSchools.Update(1.0f / 30.0f, observerFish.x,
                                   observerFish.y, observerFish.z);
        const auto fishStates = fishSchools.Fish();
        const auto repeatedStates = repeatedFishSchools.Fish();
        const auto schoolStates = fishSchools.Schools();
        const auto repeatedSchoolStates = repeatedFishSchools.Schools();
        for (std::size_t index = 0; index < schoolStates.size(); ++index) {
            const island::FishSchoolState& school = schoolStates[index];
            const island::FishSchoolState& repeated = repeatedSchoolStates[index];
            Check(school.centreX == repeated.centreX
                      && school.centreY == repeated.centreY
                      && school.centreZ == repeated.centreZ
                      && school.direction == repeated.direction
                      && school.simulationTier == repeated.simulationTier,
                  "fish school navigation remains deterministic over a long session");
        }
        for (std::size_t index = 0; index < fishStates.size(); ++index) {
            const island::FishState& fish = fishStates[index];
            const island::FishState& repeated = repeatedStates[index];
            const island::TerrainSample sample = terrain.Sample(fish.x, fish.z);
            Check(fish.x == repeated.x && fish.y == repeated.y && fish.z == repeated.z
                      && fish.headingRadians == repeated.headingRadians
                      && fish.pitchRadians == repeated.pitchRadians
                      && fish.bankRadians == repeated.bankRadians
                      && fish.bodyWave == repeated.bodyWave
                      && fish.simulationTier == repeated.simulationTier,
                  "fish simulation remains deterministic over a long session");
            Check(std::isfinite(fish.x) && std::isfinite(fish.y)
                      && std::isfinite(fish.z) && std::isfinite(fish.headingRadians)
                      && std::isfinite(fish.pitchRadians)
                      && std::isfinite(fish.bankRadians),
                  "fish pose remains finite over a long session");
            Check(std::fabs(fish.pitchRadians) <= 0.221f
                      && std::fabs(fish.bankRadians) <= 0.281f,
                  "fish pitch and bank remain inside restrained pose bounds");
            Check(sample.signedShoreDistance <= -2.8f
                      && sample.signedShoreDistance >= -48.2f,
                  "fish remain inside the authored near-shore water belt");
            Check(sample.height <= -0.5f && fish.y >= sample.height + 0.18f
                      && fish.y <= -0.18f,
                  "fish remain below the surface and above the seabed");
            observedFishTailLeft = observedFishTailLeft || fish.bodyWave > 0.65f;
            observedFishTailRight = observedFishTailRight || fish.bodyWave < -0.65f;
            observedFishPitchUp = observedFishPitchUp || fish.pitchRadians > 0.035f;
            observedFishPitchDown = observedFishPitchDown || fish.pitchRadians < -0.035f;
            observedFishLeftBank = observedFishLeftBank || fish.bankRadians < -0.035f;
            observedFishRightBank = observedFishRightBank || fish.bankRadians > 0.035f;
        }
    }
    const island::FishState finalLeadFish = fishSchools.Fish().front();
    Check(std::hypot(finalLeadFish.x - initialLeadFish.x,
                     finalLeadFish.z - initialLeadFish.z) > 20.0f,
          "fish schools traverse a meaningful section of shoreline over time");
    Check(observedFishTailLeft && observedFishTailRight,
          "fish animation visits both tail-stroke extremes");
    Check(observedFishPitchUp && observedFishPitchDown,
          "fish schools pitch into both rising and descending depth changes");
    Check(observedFishLeftBank && observedFishRightBank,
          "fish schools bank through turns in both directions");
    Check(fishSchools.FullSimulationCount() + fishSchools.ReducedSimulationCount()
              + fishSchools.DormantSimulationCount()
              == static_cast<int>(island::FishSchools::FishCount),
          "fish simulation tiers account for every school member");

    island::SharkPatrol shark(terrain);
    island::SharkPatrol repeatedShark(terrain);
    island::SharkPatrol invalidInputShark(terrain);
    invalidInputShark.Update(quietNan, quietNan, quietNan, quietNan);
    Check(invalidInputShark.State().simulationTier
              == island::WildlifeSimulationTier::Dormant
              && std::isfinite(invalidInputShark.State().x)
              && std::isfinite(invalidInputShark.State().y)
              && std::isfinite(invalidInputShark.State().z)
              && std::isfinite(invalidInputShark.State().headingRadians),
          "shark simulation rejects non-finite frame and observer input without state poisoning");
    Check(shark.State().present && shark.State().visible
              && shark.State().activity == 1.0f,
          "shark begins with a bounded visible observation window");
    Check(shark.State().x == repeatedShark.State().x
              && shark.State().y == repeatedShark.State().y
              && shark.State().z == repeatedShark.State().z
              && shark.State().headingRadians == repeatedShark.State().headingRadians
              && shark.State().pitchRadians == repeatedShark.State().pitchRadians
              && shark.State().bankRadians == repeatedShark.State().bankRadians
              && shark.State().targetX == repeatedShark.State().targetX
              && shark.State().targetZ == repeatedShark.State().targetZ,
          "shark patrol initialization is deterministic");
    bool observedPresentShark = false;
    bool observedHiddenShark = false;
    bool observedSharkTailLeft = false;
    bool observedSharkTailRight = false;
    bool observedSharkPitchUp = false;
    bool observedSharkPitchDown = false;
    bool observedSharkBank = false;
    float sharkTravelDistance = 0.0f;
    island::SharkState previousShark = shark.State();
    constexpr int sharkSimulationFrames = 36000;
    for (int frame = 0; frame < sharkSimulationFrames; ++frame) {
        const island::SharkState observerShark = shark.State();
        shark.Update(1.0f / 30.0f, observerShark.x, observerShark.y, observerShark.z);
        repeatedShark.Update(1.0f / 30.0f, observerShark.x,
                             observerShark.y, observerShark.z);
        const island::SharkState& state = shark.State();
        const island::SharkState& repeated = repeatedShark.State();
        const island::TerrainSample sample = terrain.Sample(state.x, state.z);
        Check(state.x == repeated.x && state.y == repeated.y && state.z == repeated.z
                  && state.headingRadians == repeated.headingRadians
                  && state.pitchRadians == repeated.pitchRadians
                  && state.bankRadians == repeated.bankRadians
                  && state.bodyWave == repeated.bodyWave
                  && state.activity == repeated.activity
                  && state.targetX == repeated.targetX
                  && state.targetZ == repeated.targetZ
                  && state.present == repeated.present && state.visible == repeated.visible
                  && state.simulationTier == repeated.simulationTier,
              "shark patrol remains deterministic over a long session");
        Check(std::isfinite(state.x) && std::isfinite(state.y)
                  && std::isfinite(state.z) && std::isfinite(state.headingRadians)
                  && std::isfinite(state.pitchRadians)
                  && std::isfinite(state.bankRadians)
                  && std::isfinite(state.targetX) && std::isfinite(state.targetZ),
              "shark pose remains finite over a long session");
        Check(std::fabs(state.pitchRadians) <= 0.181f
                  && std::fabs(state.bankRadians) <= 0.241f,
              "shark pitch and bank remain inside restrained pose bounds");
        const island::TerrainSample targetSample = terrain.Sample(
            state.targetX, state.targetZ);
        Check(targetSample.signedShoreDistance <= -33.8f
                  && targetSample.signedShoreDistance >= -88.2f,
              "shark route target remains in the authored offshore patrol belt");
        Check(sample.signedShoreDistance <= -33.8f
                  && sample.signedShoreDistance >= -88.2f,
              "shark remains in the authored offshore patrol belt");
        Check(sample.height <= -2.95f && state.y >= sample.height + 0.70f
                  && state.y <= -1.0f,
              "shark remains below the surface and above the offshore seabed");
        Check(state.activity >= 0.0f && state.activity <= 1.0f,
              "shark activity blend remains normalized");
        sharkTravelDistance += std::hypot(state.x - previousShark.x,
                                          state.z - previousShark.z);
        previousShark = state;
        observedPresentShark = observedPresentShark
            || (state.present && state.activity > 0.95f && state.visible);
        observedHiddenShark = observedHiddenShark
            || (!state.present && state.activity < 0.035f && !state.visible);
        observedSharkTailLeft = observedSharkTailLeft || state.bodyWave > 0.65f;
        observedSharkTailRight = observedSharkTailRight || state.bodyWave < -0.65f;
        observedSharkPitchUp = observedSharkPitchUp || state.pitchRadians > 0.035f;
        observedSharkPitchDown = observedSharkPitchDown || state.pitchRadians < -0.035f;
        observedSharkBank = observedSharkBank || std::fabs(state.bankRadians) > 0.018f;
    }
    Check(shark.PresenceCycleCount() >= 8U,
          "shark alternates through several bounded presence cycles in twenty minutes");
    Check(observedPresentShark && observedHiddenShark,
          "shark reaches both fully present and fully hidden states");
    Check(observedSharkTailLeft && observedSharkTailRight,
          "shark animation visits both tail-stroke extremes");
    Check(observedSharkPitchUp && observedSharkPitchDown,
          "shark pitches through both dive and return transitions");
    Check(observedSharkBank,
          "shark visibly banks while following its varied offshore patrol");
    Check(sharkTravelDistance > 700.0f,
          "shark patrol covers a meaningful offshore route over time");

    const island::VegetationLayout vegetation(terrain);
    const island::JungleLayout jungle(terrain);
    const island::NaturalPropLayout props(terrain, vegetation, jungle);
    std::vector<island::CircularObstacle> worldObstacles;
    worldObstacles.reserve(vegetation.Obstacles().size() + jungle.Obstacles().size()
                           + props.Obstacles().size());
    worldObstacles.insert(worldObstacles.end(), vegetation.Obstacles().begin(),
                          vegetation.Obstacles().end());
    worldObstacles.insert(worldObstacles.end(), jungle.Obstacles().begin(),
                          jungle.Obstacles().end());
    worldObstacles.insert(worldObstacles.end(), props.Obstacles().begin(),
                          props.Obstacles().end());
    const island::WorldCollisionGrid worldCollisionGrid(worldObstacles);
    Check(worldCollisionGrid.Obstacles().size() == worldObstacles.size()
              && worldCollisionGrid.CellCount() > 0U,
          "world collision grid indexes the complete static obstacle set");
    bool collisionGridMatchesBruteForce = true;
    std::size_t collisionCandidateTotal = 0U;
    std::size_t collisionQueryCount = 0U;
    for (int row = -20; row <= 20; ++row) {
        for (int column = -20; column <= 20; ++column) {
            const float queryX = static_cast<float>(column) * 13.7f + 0.19f;
            const float queryZ = static_cast<float>(row) * 13.1f - 0.23f;
            const float queryRadius = island::PlayerMotion::CapsuleRadius;
            const bool bruteForceHit = std::ranges::any_of(
                worldObstacles, [queryX, queryZ, queryRadius](const auto& obstacle) {
                    const float combinedRadius = queryRadius
                        + std::max(0.0f, obstacle.radius);
                    const float dx = queryX - obstacle.x;
                    const float dz = queryZ - obstacle.z;
                    return dx * dx + dz * dz < combinedRadius * combinedRadius;
                });
            collisionGridMatchesBruteForce = collisionGridMatchesBruteForce
                && worldCollisionGrid.IntersectsCircle(queryX, queryZ, queryRadius)
                    == bruteForceHit;
            collisionCandidateTotal += worldCollisionGrid.CandidateCount(
                queryX, queryZ, queryRadius);
            ++collisionQueryCount;
        }
    }
    Check(collisionGridMatchesBruteForce,
          "collision broad phase exactly matches exhaustive circle tests across the island");
    Check(collisionCandidateTotal / collisionQueryCount < worldObstacles.size() / 8U,
          "collision broad phase materially reduces the average candidate population");
    island::MonkeyTroop monkeys(terrain);
    island::MonkeyTroop repeatedMonkeys(terrain);
    island::MonkeyTroop invalidInputMonkeys(terrain);
    invalidInputMonkeys.Update(quietNan, quietNan, quietNan, quietNan);
    Check(invalidInputMonkeys.DormantSimulationCount()
              == static_cast<int>(island::MonkeyTroop::MonkeyCount)
              && std::ranges::all_of(invalidInputMonkeys.Monkeys(), [](const auto& state) {
                     return std::isfinite(state.x) && std::isfinite(state.y)
                         && std::isfinite(state.z) && std::isfinite(state.animationSeconds);
                 }),
          "monkey simulation rejects non-finite frame and observer input without state poisoning");
    monkeys.SetObstacles(worldObstacles);
    repeatedMonkeys.SetObstacles(worldObstacles);
    Check(monkeys.Monkeys().size() == island::MonkeyTroop::MonkeyCount,
          "monkey troop has the intended bounded population");
    Check(monkeys.Monkeys().size() == repeatedMonkeys.Monkeys().size(),
          "repeated monkey troop has the same population");
    std::array<bool, 4> observedMonkeyVariants{};
    float minimumMonkeyScale = 10.0f;
    float maximumMonkeyScale = 0.0f;
    for (const island::MonkeyState& monkey : monkeys.Monkeys()) {
        Check(monkey.colorVariant < observedMonkeyVariants.size(),
              "monkey material variant remains inside the renderer palette");
        if (monkey.colorVariant < observedMonkeyVariants.size()) {
            observedMonkeyVariants[monkey.colorVariant] = true;
        }
        minimumMonkeyScale = std::min(minimumMonkeyScale, monkey.scale);
        maximumMonkeyScale = std::max(maximumMonkeyScale, monkey.scale);
    }
    Check(std::ranges::all_of(observedMonkeyVariants, [](const bool value) { return value; }),
          "monkey troop exercises all four material palettes");
    Check(maximumMonkeyScale - minimumMonkeyScale >= 0.20f,
          "monkey troop has meaningful individual scale variation");

    island::MonkeyTroop attentiveMonkeys(terrain);
    attentiveMonkeys.SetObstacles(worldObstacles);
    const island::MonkeyState attentionStart = attentiveMonkeys.Monkeys().front();
    const float leftObserverAngle = attentionStart.headingRadians + 0.90f;
    for (int frame = 0; frame < 30; ++frame) {
        attentiveMonkeys.Update(
            1.0f / 30.0f,
            attentionStart.x + std::cos(leftObserverAngle) * 12.0f,
            attentionStart.y + 1.6f,
            attentionStart.z + std::sin(leftObserverAngle) * 12.0f);
    }
    Check(attentiveMonkeys.Monkeys().front().headLookRadians > 0.20f,
          "a calm monkey gradually attends toward a nearby observer on its left");
    const float rightObserverAngle = attentionStart.headingRadians - 0.90f;
    for (int frame = 0; frame < 50; ++frame) {
        attentiveMonkeys.Update(
            1.0f / 30.0f,
            attentionStart.x + std::cos(rightObserverAngle) * 12.0f,
            attentionStart.y + 1.6f,
            attentionStart.z + std::sin(rightObserverAngle) * 12.0f);
    }
    Check(attentiveMonkeys.Monkeys().front().headLookRadians < -0.20f,
          "a calm monkey smoothly redirects attention toward the opposite side");

    bool observedMonkeyIdle = false;
    bool observedMonkeyObserve = false;
    bool observedMonkeyWalk = false;
    bool observedMonkeyRunAway = false;
    bool observedMonkeyFullTier = false;
    bool observedMonkeyReducedTier = false;
    bool observedMonkeyDormantTier = false;
    bool observedMonkeyContinuousClipClock = false;
    float monkeyTravelDistance = 0.0f;
    std::vector<island::MonkeyState> previousMonkeys(
        monkeys.Monkeys().begin(), monkeys.Monkeys().end());
    constexpr int monkeySimulationFrames = 36000;
    for (int frame = 0; frame < monkeySimulationFrames; ++frame) {
        const island::MonkeyState followed = monkeys.Monkeys().front();
        const float observerX = frame < 240 ? followed.x
            : (frame < 12000 ? 0.0f : (frame < 18000 ? 1000.0f : -40.0f));
        const float observerY = frame < 240 ? followed.y + 1.0f : 4.0f;
        const float observerZ = frame < 240 ? followed.z
            : (frame < 12000 ? 0.0f : (frame < 18000 ? 1000.0f : 35.0f));
        monkeys.Update(1.0f / 30.0f, observerX, observerY, observerZ);
        repeatedMonkeys.Update(1.0f / 30.0f, observerX, observerY, observerZ);
        Check(monkeys.FullSimulationCount() + monkeys.ReducedSimulationCount()
                  + monkeys.DormantSimulationCount()
                  == static_cast<int>(island::MonkeyTroop::MonkeyCount),
              "monkey simulation tiers account for the whole troop");
        observedMonkeyFullTier = observedMonkeyFullTier
            || monkeys.FullSimulationCount() > 0;
        observedMonkeyReducedTier = observedMonkeyReducedTier
            || monkeys.ReducedSimulationCount() > 0;
        observedMonkeyDormantTier = observedMonkeyDormantTier
            || monkeys.DormantSimulationCount() > 0;
        for (std::size_t index = 0; index < monkeys.Monkeys().size(); ++index) {
            const island::MonkeyState& state = monkeys.Monkeys()[index];
            const island::MonkeyState& repeated = repeatedMonkeys.Monkeys()[index];
            const island::TerrainSample sample = terrain.Sample(state.x, state.z);
            Check(state.x == repeated.x && state.y == repeated.y
                      && state.z == repeated.z
                      && state.headingRadians == repeated.headingRadians
                      && state.headLookRadians == repeated.headLookRadians
                      && state.speed == repeated.speed
                      && state.animationSeconds == repeated.animationSeconds
                      && state.homeX == repeated.homeX && state.homeZ == repeated.homeZ
                      && state.targetX == repeated.targetX
                      && state.targetZ == repeated.targetZ
                      && state.behavior == repeated.behavior
                      && state.simulationTier == repeated.simulationTier,
                  "monkey troop remains deterministic over a long session");
            Check(std::isfinite(state.x) && std::isfinite(state.y)
                      && std::isfinite(state.z) && std::isfinite(state.headingRadians)
                      && std::isfinite(state.headLookRadians)
                      && std::isfinite(state.animationSeconds)
                      && std::isfinite(state.homeX) && std::isfinite(state.homeZ)
                      && std::isfinite(state.targetX) && std::isfinite(state.targetZ),
                  "monkey pose remains finite over a long session");
            Check(std::fabs(state.headLookRadians)
                      <= island::MonkeyTroop::MaximumHeadLookRadians + 0.001f,
                  "monkey head attention remains inside its restrained skeletal bound");
            Check(std::hypot(state.x - state.homeX, state.z - state.homeZ)
                      <= island::MonkeyTroop::MaximumHomeDistance + 0.001f
                      && std::hypot(state.targetX - state.homeX,
                                    state.targetZ - state.homeZ)
                          <= island::MonkeyTroop::MaximumHomeDistance + 0.001f,
                  "monkey pose and active target remain inside the authoritative home range");
            Check(state.animationSeconds >= previousMonkeys[index].animationSeconds,
                  "monkey animation clock remains monotonic across behaviour changes");
            observedMonkeyContinuousClipClock = observedMonkeyContinuousClipClock
                || (state.behavior != previousMonkeys[index].behavior
                    && state.animationSeconds > previousMonkeys[index].animationSeconds);
            Check(sample.signedShoreDistance >= 47.8f && sample.height >= 2.75f
                      && sample.slopeRadians <= 0.465f
                      && std::fabs(state.y - sample.height) < 0.001f,
                  "monkeys remain on supported traversable interior terrain");
            if (frame % 30 == 0) {
                for (const island::CircularObstacle& obstacle : worldObstacles) {
                    const float minimumDistance = island::MonkeyTroop::CollisionRadius
                        + std::max(0.0f, obstacle.radius);
                    Check(std::hypot(state.x - obstacle.x, state.z - obstacle.z)
                              >= minimumDistance - 0.001f,
                          "monkeys remain outside trunk, rock, and log collision proxies");
                }
            }
            monkeyTravelDistance += std::hypot(
                state.x - previousMonkeys[index].x,
                state.z - previousMonkeys[index].z);
            previousMonkeys[index] = state;
            observedMonkeyIdle = observedMonkeyIdle
                || state.behavior == island::MonkeyBehavior::Idle;
            observedMonkeyObserve = observedMonkeyObserve
                || state.behavior == island::MonkeyBehavior::Observe;
            observedMonkeyWalk = observedMonkeyWalk
                || state.behavior == island::MonkeyBehavior::Walk;
            observedMonkeyRunAway = observedMonkeyRunAway
                || state.behavior == island::MonkeyBehavior::RunAway;
        }
    }
    Check(observedMonkeyIdle && observedMonkeyObserve && observedMonkeyWalk
              && observedMonkeyRunAway,
          "monkey troop visits idle, observe, walk, and mild avoidance states");
    Check(observedMonkeyFullTier && observedMonkeyReducedTier && observedMonkeyDormantTier,
          "monkey troop exercises every simulation quality tier");
    Check(monkeyTravelDistance > 900.0f,
          "monkey troop covers meaningful valid ground during a long session");
    Check(monkeys.ObstacleDeflectionCount() > 0U
              && monkeys.ObstacleDeflectionCount()
                  == repeatedMonkeys.ObstacleDeflectionCount(),
          "monkey obstacle steering is exercised and remains deterministic");
    Check(observedMonkeyContinuousClipClock,
          "monkey behaviour changes preserve a continuous renderer animation clock");

    Check(island::OceanMeshLayout::VertexCount == 25921U,
          "ocean grid remains within the portable 16-bit vertex budget");
    Check(island::OceanMeshLayout::TriangleCount == 51200U,
          "ocean grid triangle count is deterministic");
    CheckNear(island::OceanMeshLayout::Coordinate(0), -island::OceanMeshLayout::Extent,
              0.0001f, "ocean grid begins at its documented extent");
    CheckNear(island::OceanMeshLayout::Coordinate(island::OceanMeshLayout::QuadsPerAxis),
              island::OceanMeshLayout::Extent, 0.0001f,
              "ocean grid ends at its documented extent");
    Check(island::ShorelineMeshLayout::VertexCount == 2695U,
          "shoreline ribbon stays inside its compact portable vertex budget");
    Check(island::ShorelineMeshLayout::TriangleCount == 4608U,
          "shoreline ribbon triangle count is deterministic");
    CheckNear(island::ShorelineMeshLayout::RadialOffset(0), -2.5f, 0.0001f,
              "shoreline ribbon begins landward of the authoritative contour");
    CheckNear(island::ShorelineMeshLayout::RadialOffset(
                  island::ShorelineMeshLayout::AcrossQuadCount),
              5.5f, 0.0001f,
              "shoreline ribbon ends in the shallow offshore wash band");

    Check(island::TerrainMeshLayout::CellCount == 64U,
          "terrain layout has the expected bounded cell count");
    std::size_t partitionTriangleCount = 0;
    for (std::size_t index = 0; index < island::TerrainMeshLayout::CellCount; ++index) {
        const island::TerrainCellLayout cell = island::TerrainMeshLayout::Cell(index);
        Check(cell.quadColumns == island::TerrainMeshLayout::QuadsPerCell
                  && cell.quadRows == island::TerrainMeshLayout::QuadsPerCell,
              "terrain cells use a uniform portable grid");
        Check(cell.minimumX < cell.maximumX && cell.minimumZ < cell.maximumZ,
              "terrain cell bounds have positive area");
        partitionTriangleCount += static_cast<std::size_t>(cell.quadColumns * cell.quadRows * 2);
        if (cell.column + 1 < island::TerrainMeshLayout::CellsPerAxis) {
            const island::TerrainCellLayout right = island::TerrainMeshLayout::Cell(index + 1U);
            CheckNear(cell.maximumX, right.minimumX, 0.0001f,
                      "horizontal terrain cell boundaries share an exact seam");
        }
        if (cell.row + 1 < island::TerrainMeshLayout::CellsPerAxis) {
            const island::TerrainCellLayout front = island::TerrainMeshLayout::Cell(
                index + island::TerrainMeshLayout::CellsPerAxis);
            CheckNear(cell.maximumZ, front.minimumZ, 0.0001f,
                      "vertical terrain cell boundaries share an exact seam");
        }
    }
    Check(partitionTriangleCount
              == static_cast<std::size_t>(island::TerrainMeshLayout::QuadsPerAxis
                                           * island::TerrainMeshLayout::QuadsPerAxis * 2),
          "terrain cells cover every source-grid triangle exactly once");
    Check(island::TerrainMeshLayout::Resolution * island::TerrainMeshLayout::Resolution
              == 58081,
          "the shared terrain vertex grid remains inside the portable 16-bit index budget");
    std::array<bool, island::TerrainMeshLayout::CellCount> noVisibleTerrainCells{};
    Check(island::TerrainMeshLayout::CoalesceVisibleCells(noVisibleTerrainCells).count == 0U,
          "terrain batching emits no range when every cell is culled");
    std::array<bool, island::TerrainMeshLayout::CellCount> allVisibleTerrainCells{};
    allVisibleTerrainCells.fill(true);
    const island::TerrainMeshLayout::CellRuns oneTerrainRun =
        island::TerrainMeshLayout::CoalesceVisibleCells(allVisibleTerrainCells);
    Check(oneTerrainRun.count == 1U && oneTerrainRun.values[0].firstCell == 0U
              && oneTerrainRun.values[0].cellCount == island::TerrainMeshLayout::CellCount,
          "terrain batching merges one fully visible island into one index range");
    std::array<bool, island::TerrainMeshLayout::CellCount> sparseTerrainCells{};
    sparseTerrainCells[0] = true;
    sparseTerrainCells[1] = true;
    sparseTerrainCells[3] = true;
    sparseTerrainCells[4] = true;
    sparseTerrainCells[5] = true;
    sparseTerrainCells.back() = true;
    const island::TerrainMeshLayout::CellRuns sparseTerrainRuns =
        island::TerrainMeshLayout::CoalesceVisibleCells(sparseTerrainCells);
    Check(sparseTerrainRuns.count == 3U
              && sparseTerrainRuns.values[0].firstCell == 0U
              && sparseTerrainRuns.values[0].cellCount == 2U
              && sparseTerrainRuns.values[1].firstCell == 3U
              && sparseTerrainRuns.values[1].cellCount == 3U
              && sparseTerrainRuns.values[2].firstCell
                  == island::TerrainMeshLayout::CellCount - 1U
              && sparseTerrainRuns.values[2].cellCount == 1U,
          "terrain batching preserves gaps while merging adjacent visible cells");

    Check(terrain.IsLand(0.0f, 0.0f), "island centre is land");
    Check(!terrain.IsLand(420.0f, 0.0f), "far offshore point is water");
    Check(terrain.Height(420.0f, 0.0f) < -3.0f, "offshore seabed descends");
    Check(terrain.Height(0.0f, 0.0f) > 8.0f, "interior has useful elevation");

    const island::TerrainSample centreSample = terrain.Sample(0.0f, 0.0f);
    const float centreNormalLength = std::hypot(
        centreSample.normal.y, std::hypot(centreSample.normal.x, centreSample.normal.z));
    CheckNear(centreNormalLength, 1.0f, 0.0001f, "terrain sample normal is normalized");
    Check(centreSample.normal.y > 0.0f, "terrain sample normal points upward");
    Check(centreSample.surface == island::TerrainSurface::InteriorGround
              || centreSample.surface == island::TerrainSurface::SteepGround,
          "island centre has an interior ground classification");

    const island::TerrainSample deepWaterSample = terrain.Sample(420.0f, 0.0f);
    Check(deepWaterSample.IsSubmerged(), "deep-water sample reports submerged terrain");
    Check(deepWaterSample.surface == island::TerrainSurface::DeepSeabed,
          "far offshore terrain is classified as deep seabed");

    float minimumRadius = 1000.0f;
    float maximumRadius = 0.0f;
    for (int step = 0; step < 720; ++step) {
        const float angle = 2.0f * std::numbers::pi_v<float> * static_cast<float>(step) / 720.0f;
        const float radius = terrain.ShoreRadius(angle);
        minimumRadius = std::min(minimumRadius, radius);
        maximumRadius = std::max(maximumRadius, radius);
        const float x = std::cos(angle) * radius;
        const float z = std::sin(angle) * radius;
        CheckNear(terrain.SignedShoreDistance(x, z), 0.0f, 0.001f,
                  "shore parameterization agrees with shore distance");
        Check(std::isfinite(terrain.Height(x, z)), "shore height is finite");
    }
    Check(minimumRadius > 185.0f, "shoreline never collapses inward");
    Check(maximumRadius < 275.0f, "shoreline remains near the 500 metre target");
    Check(maximumRadius - minimumRadius > 20.0f, "shoreline silhouette has variation");

    const island::VegetationLayout repeatedVegetation(terrain);
    Check(vegetation.Palms().size() >= 180U && vegetation.Palms().size() <= 520U,
          "palm population stays within the intended first performance budget");
    Check(vegetation.Palms().size() == repeatedVegetation.Palms().size(),
          "palm population is deterministic");
    Check(vegetation.Obstacles().size() == vegetation.Palms().size(),
          "every palm has exactly one trunk collision proxy");
    CheckNear(island::VegetationLayout::PrimaryPathCenterX(
                  -island::VegetationLayout::PrimaryPathZOffset),
              island::VegetationLayout::PrimaryPathBaseX, 0.0001f,
              "primary path shape constants identify the authored centre crossing");
    Check(island::VegetationLayout::PrimaryPathMinimumZ
                  < island::VegetationLayout::PrimaryPathMaximumZ
              && island::VegetationLayout::PrimaryPathReservedHalfWidth > 4.0f,
          "primary path material range remains ordered inside a walkable reserve");
    Check(island::VegetationLayout::EastClearing.radius > 20.0f
              && island::VegetationLayout::WestClearing.radius > 20.0f,
          "authored clearings retain useful first-person room dimensions");
    Check(island::VegetationLayout::IsReservedRoute(
              island::VegetationLayout::EastClearing.x,
              island::VegetationLayout::EastClearing.z)
              && island::VegetationLayout::IsReservedRoute(
                  island::VegetationLayout::WestClearing.x,
                  island::VegetationLayout::WestClearing.z),
          "authored clearing material centres remain vegetation-reserved");
    for (int z = -216; z <= 180; z += 18) {
        const float pathX = island::VegetationLayout::PrimaryPathCenterX(static_cast<float>(z));
        Check(island::VegetationLayout::IsReservedRoute(pathX, static_cast<float>(z)),
              "the authored primary path centre remains reserved from vegetation");
    }
    for (std::size_t index = 0; index < vegetation.Palms().size(); ++index) {
        const island::PalmPlacement& palm = vegetation.Palms()[index];
        const island::PalmPlacement& repeated = repeatedVegetation.Palms()[index];
        const island::TerrainSample sample = terrain.Sample(palm.x, palm.z);
        Check(palm.x == repeated.x && palm.y == repeated.y && palm.z == repeated.z
                  && palm.yawRadians == repeated.yawRadians && palm.scale == repeated.scale
                  && palm.variant == repeated.variant,
              "palm placement data is bitwise deterministic for one seed");
        Check(sample.signedShoreDistance >= 7.0f && sample.signedShoreDistance <= 158.0f,
              "palms remain inside the authored coastal/interior belt");
        Check(sample.slopeRadians <= 0.4712389f,
              "palms reject terrain steeper than the placement limit");
        Check(!island::VegetationLayout::IsReservedRoute(palm.x, palm.z),
              "palms keep paths, clearings, and player spawn open");
        Check(palm.variant < island::VegetationLayout::PalmVariantCount,
              "palm variant index remains in range");
        Check(palm.scale >= island::VegetationLayout::PalmMinimumScale
                  && palm.scale <= island::VegetationLayout::PalmMaximumScale,
              "palm scale remains in the authored range");
        const island::CircularObstacle& obstacle = vegetation.Obstacles()[index];
        CheckNear(obstacle.x, palm.x, 0.0001f, "palm obstacle shares its visual x coordinate");
        CheckNear(obstacle.z, palm.z, 0.0001f, "palm obstacle shares its visual z coordinate");
        Check(obstacle.radius > 0.20f && obstacle.radius < 0.45f,
              "palm collision radius stays close to the visible trunk");
    }

    const island::GrassLayout grass(terrain);
    const island::GrassLayout repeatedGrass(terrain);
    const island::GrassLodSplit emptyGrassLod = island::GrassLod::Split(0, 82.0f);
    Check(emptyGrassLod.nearCount == 0 && emptyGrassLod.middleCount == 0
              && emptyGrassLod.farCount == 0,
          "grass LOD ignores an empty population");
    const island::GrassLodSplit invalidGrassLod = island::GrassLod::Split(
        3, std::numeric_limits<float>::quiet_NaN());
    const island::GrassLodSplit infiniteGrassLod = island::GrassLod::Split(
        3, std::numeric_limits<float>::infinity());
    Check(invalidGrassLod.nearCount == 3 && invalidGrassLod.middleCount == 0
              && invalidGrassLod.farCount == 0 && infiniteGrassLod.nearCount == 0
              && infiniteGrassLod.middleCount == 0 && infiniteGrassLod.farCount == 3,
          "grass LOD handles invalid and unbounded distances conservatively");
    const island::GrassLodSplit nearGrassLod = island::GrassLod::Split(
        101, island::GrassLod::NearOnlyDistance);
    const island::GrassLodSplit nearMiddleGrassLod = island::GrassLod::Split(101, 70.0f);
    const island::GrassLodSplit middleGrassLod = island::GrassLod::Split(101, 82.0f);
    const island::GrassLodSplit middleFarGrassLod = island::GrassLod::Split(101, 94.0f);
    const island::GrassLodSplit farGrassLod = island::GrassLod::Split(
        101, island::GrassLod::FarOnlyDistance);
    Check(nearGrassLod.nearCount == 101 && nearGrassLod.middleCount == 0
              && nearGrassLod.farCount == 0 && nearMiddleGrassLod.nearCount == 50
              && nearMiddleGrassLod.middleCount == 51 && nearMiddleGrassLod.farCount == 0
              && middleGrassLod.nearCount == 0 && middleGrassLod.middleCount == 101
              && middleGrassLod.farCount == 0 && middleFarGrassLod.nearCount == 0
              && middleFarGrassLod.middleCount == 50 && middleFarGrassLod.farCount == 51
              && farGrassLod.nearCount == 0 && farGrassLod.middleCount == 0
              && farGrassLod.farCount == 101,
          "grass LOD covers near, middle, far, and adjacent mixed populations");
    int previousNearGrassCount = 101;
    int previousFarGrassCount = 0;
    for (int step = 0; step <= 160; ++step) {
        const float distance = island::GrassLod::NearOnlyDistance
            + static_cast<float>(step) * 0.25f;
        const island::GrassLodSplit split = island::GrassLod::Split(101, distance);
        Check(split.nearCount >= 0 && split.middleCount >= 0 && split.farCount >= 0
                  && split.nearCount + split.middleCount + split.farCount == 101,
              "grass LOD conserves every patch through both transitions");
        Check(split.nearCount <= previousNearGrassCount,
              "grass near-LOD population decreases monotonically with distance");
        Check(split.farCount >= previousFarGrassCount,
              "grass far-LOD population increases monotonically with distance");
        Check(!(split.nearCount > 0 && split.farCount > 0),
              "grass never mixes non-adjacent near and far meshes");
        previousNearGrassCount = split.nearCount;
        previousFarGrassCount = split.farCount;
    }
    Check(grass.Patches().size() >= 10000U && grass.Patches().size() <= 40000U,
          "grass patch population stays inside the first geometry budget");
    Check(grass.Patches().size() == repeatedGrass.Patches().size(),
          "grass population is deterministic");
    double eastClearingTintSum = 0.0;
    double westClearingTintSum = 0.0;
    std::size_t eastClearingPatchCount = 0U;
    std::size_t westClearingPatchCount = 0U;
    for (std::size_t index = 0; index < grass.Patches().size(); ++index) {
        const island::GrassPlacement& patch = grass.Patches()[index];
        const island::GrassPlacement& repeated = repeatedGrass.Patches()[index];
        const island::TerrainSample sample = terrain.Sample(patch.x, patch.z);
        Check(patch.x == repeated.x && patch.y == repeated.y && patch.z == repeated.z
                  && patch.yawRadians == repeated.yawRadians && patch.scale == repeated.scale
                  && patch.spreadScale == repeated.spreadScale
                  && patch.windPhase == repeated.windPhase
                  && patch.colorVariation == repeated.colorVariation
                  && patch.biomeTint == repeated.biomeTint
                  && patch.pathScale == repeated.pathScale,
              "grass placement data is bitwise deterministic for one seed");
        Check(sample.signedShoreDistance >= 20.0f && sample.height >= 2.1f,
              "grass remains above the sand transition");
        Check(sample.slopeRadians <= 0.4886922f,
              "grass rejects terrain steeper than its placement limit");
        Check(patch.windPhase >= 0.0f && patch.windPhase < 1.0f
                  && patch.colorVariation >= 0.0f && patch.colorVariation < 1.0f
                  && patch.biomeTint >= 0.0f && patch.biomeTint <= 1.0f,
              "grass shader variation values stay normalized");
        Check(patch.spreadScale >= island::GrassLayout::MinimumSpreadScale
                  && patch.spreadScale <= island::GrassLayout::MaximumSpreadScale,
              "grass horizontal spread remains in the authored range");
        if (patch.pathScale) {
            Check(island::VegetationLayout::IsReservedRoute(patch.x, patch.z),
                  "short grass is confined to authored route regions");
            Check(patch.scale < island::GrassLayout::MinimumScale,
                  "authored route grass remains shorter than ordinary grass");
        } else {
            Check(patch.scale >= island::GrassLayout::MinimumScale
                      && patch.scale <= island::GrassLayout::MaximumScale,
                  "ordinary grass scale remains in the authored range");
        }

        const auto accumulateClearingTint = [&patch](
            const island::ClearingDefinition& clearing, double& sum,
            std::size_t& count) {
            const float offsetX = patch.x - clearing.x;
            const float offsetZ = patch.z - clearing.z;
            if (offsetX * offsetX + offsetZ * offsetZ
                <= clearing.radius * clearing.radius) {
                sum += static_cast<double>(patch.biomeTint);
                ++count;
            }
        };
        accumulateClearingTint(island::VegetationLayout::EastClearing,
                               eastClearingTintSum, eastClearingPatchCount);
        accumulateClearingTint(island::VegetationLayout::WestClearing,
                               westClearingTintSum, westClearingPatchCount);
    }
    Check(eastClearingPatchCount >= 100U && westClearingPatchCount >= 100U,
          "both authored clearings retain enough grass to express their material identity");
    const double eastClearingAverageTint = eastClearingTintSum
        / static_cast<double>(eastClearingPatchCount);
    const double westClearingAverageTint = westClearingTintSum
        / static_cast<double>(westClearingPatchCount);
    Check(eastClearingAverageTint >= 0.70 && westClearingAverageTint <= 0.30
              && eastClearingAverageTint - westClearingAverageTint >= 0.45,
          "east and west clearing grass keep distinct warm and cool biome identities");

    const island::JungleLayout repeatedJungle(terrain);
    Check(jungle.Canopy().size() >= 100U && jungle.Canopy().size() <= 600U,
          "broadleaf canopy population stays inside its first performance budget");
    Check(jungle.Understory().size() >= 800U && jungle.Understory().size() <= 4000U,
          "understory population stays inside its first performance budget");
    Check(jungle.Bushes().size() >= 150U && jungle.Bushes().size() <= 1400U,
          "low-bush population stays inside its first performance budget");
    Check(jungle.GroundPlants().size() >= 600U && jungle.GroundPlants().size() <= 5000U,
          "forest-floor plant population stays inside its first performance budget");
    Check(jungle.Canopy().size() == repeatedJungle.Canopy().size()
              && jungle.Understory().size() == repeatedJungle.Understory().size()
              && jungle.Bushes().size() == repeatedJungle.Bushes().size()
              && jungle.GroundPlants().size() == repeatedJungle.GroundPlants().size(),
          "jungle populations are deterministic");
    Check(jungle.Obstacles().size() == jungle.Canopy().size(),
          "every broadleaf canopy trunk has one collision proxy");
    for (std::size_t index = 0; index < jungle.Canopy().size(); ++index) {
        const island::CanopyPlacement& tree = jungle.Canopy()[index];
        const island::CanopyPlacement& repeated = repeatedJungle.Canopy()[index];
        const island::TerrainSample sample = terrain.Sample(tree.x, tree.z);
        Check(tree.x == repeated.x && tree.y == repeated.y && tree.z == repeated.z
                  && tree.yawRadians == repeated.yawRadians && tree.scale == repeated.scale
                  && tree.variant == repeated.variant,
              "broadleaf canopy placement is bitwise deterministic for one seed");
        Check(sample.signedShoreDistance >= 43.0f
                  && sample.signedShoreDistance <= 238.0f,
              "broadleaf trees remain in the interior jungle belt");
        Check(sample.height >= 3.8f && sample.slopeRadians <= 0.4712389f,
              "broadleaf trees remain on suitable interior ground");
        Check(!island::VegetationLayout::IsReservedRoute(tree.x, tree.z),
              "broadleaf trees keep paths, clearings, and player spawn open");
        Check(tree.variant < island::JungleLayout::CanopyVariantCount,
              "broadleaf canopy variant index remains in range");
        Check(tree.scale >= island::JungleLayout::CanopyMinimumScale
                  && tree.scale <= island::JungleLayout::CanopyMaximumScale,
              "broadleaf canopy scale remains in the authored range");
        const island::CircularObstacle& obstacle = jungle.Obstacles()[index];
        CheckNear(obstacle.x, tree.x, 0.0001f,
                  "broadleaf obstacle shares its visual x coordinate");
        CheckNear(obstacle.z, tree.z, 0.0001f,
                  "broadleaf obstacle shares its visual z coordinate");
        Check(obstacle.radius > 0.30f && obstacle.radius < 0.56f,
              "broadleaf collision radius stays close to the visible trunk");
    }
    for (std::size_t index = 0; index < jungle.Understory().size(); ++index) {
        const island::UnderstoryPlacement& plant = jungle.Understory()[index];
        const island::UnderstoryPlacement& repeated = repeatedJungle.Understory()[index];
        const island::TerrainSample sample = terrain.Sample(plant.x, plant.z);
        Check(plant.x == repeated.x && plant.y == repeated.y && plant.z == repeated.z
                  && plant.yawRadians == repeated.yawRadians && plant.scale == repeated.scale
                  && plant.variant == repeated.variant,
              "understory placement is bitwise deterministic for one seed");
        Check(sample.signedShoreDistance >= 31.0f
                  && sample.signedShoreDistance <= 242.0f,
              "understory remains inside its interior placement belt");
        Check(sample.height >= 2.8f && sample.slopeRadians <= 0.5410521f,
              "understory remains on suitable interior ground");
        Check(!island::VegetationLayout::IsReservedRoute(plant.x, plant.z),
              "understory keeps paths, clearings, and player spawn open");
        Check(plant.variant < island::JungleLayout::UnderstoryVariantCount,
              "understory variant index remains in range");
        Check(plant.scale >= island::JungleLayout::UnderstoryMinimumScale
                  && plant.scale <= island::JungleLayout::UnderstoryMaximumScale,
              "understory scale remains in the authored range");
    }
    for (std::size_t index = 0; index < jungle.Bushes().size(); ++index) {
        const island::BushPlacement& bush = jungle.Bushes()[index];
        const island::BushPlacement& repeated = repeatedJungle.Bushes()[index];
        const island::TerrainSample sample = terrain.Sample(bush.x, bush.z);
        Check(bush.x == repeated.x && bush.y == repeated.y && bush.z == repeated.z
                  && bush.yawRadians == repeated.yawRadians && bush.scale == repeated.scale
                  && bush.variant == repeated.variant,
              "low-bush placement is bitwise deterministic for one seed");
        Check(sample.signedShoreDistance >= 38.0f
                  && sample.signedShoreDistance <= 232.0f,
              "low bushes remain inside their interior placement belt");
        Check(sample.height >= 3.0f && sample.slopeRadians <= 0.5235988f,
              "low bushes remain on suitable interior ground");
        Check(!island::VegetationLayout::IsReservedRoute(bush.x, bush.z),
              "low bushes keep paths, clearings, and player spawn open");
        Check(bush.variant < island::JungleLayout::BushVariantCount,
              "low-bush variant index remains in range");
        Check(bush.scale >= island::JungleLayout::BushMinimumScale
                  && bush.scale <= island::JungleLayout::BushMaximumScale,
              "low-bush scale remains in the authored range");
    }
    for (std::size_t index = 0; index < jungle.GroundPlants().size(); ++index) {
        const island::GroundPlantPlacement& plant = jungle.GroundPlants()[index];
        const island::GroundPlantPlacement& repeated = repeatedJungle.GroundPlants()[index];
        const island::TerrainSample sample = terrain.Sample(plant.x, plant.z);
        Check(plant.x == repeated.x && plant.y == repeated.y && plant.z == repeated.z
                  && plant.yawRadians == repeated.yawRadians && plant.scale == repeated.scale
                  && plant.variant == repeated.variant,
              "forest-floor plant placement is bitwise deterministic for one seed");
        Check(sample.signedShoreDistance >= 27.0f
                  && sample.signedShoreDistance <= 238.0f,
              "forest-floor plants remain inside their coastal-to-interior belt");
        Check(sample.height >= 2.5f && sample.slopeRadians <= 0.5585054f,
              "forest-floor plants remain on suitable ground");
        Check(!island::VegetationLayout::IsReservedRoute(plant.x, plant.z),
              "forest-floor plants keep paths, clearings, and player spawn open");
        Check(plant.variant < island::JungleLayout::GroundPlantVariantCount,
              "forest-floor plant variant index remains in range");
        Check(plant.scale >= island::JungleLayout::GroundPlantMinimumScale
                  && plant.scale <= island::JungleLayout::GroundPlantMaximumScale,
              "forest-floor plant scale remains in the authored range");
    }

    const island::NaturalPropLayout repeatedProps(terrain, vegetation, jungle);
    Check(props.Rocks().size() >= 30U && props.Rocks().size() <= 500U,
          "rock population stays inside its first performance budget");
    Check(props.FallenLogs().size() >= 6U && props.FallenLogs().size() <= 80U,
          "fallen-log population stays inside its first composition budget");
    Check(props.Driftwood().size() >= 12U && props.Driftwood().size() <= 100U,
          "driftwood population stays inside its sparse beach-composition budget");
    Check(props.BeachDetails().size() >= 120U && props.BeachDetails().size() <= 600U,
          "shell/pebble clusters stay inside their near-field beach-detail budget");
    Check(props.FallenFronds().size() >= 30U && props.FallenFronds().size() <= 180U,
          "fallen palm fronds stay inside their near-field composition budget");
    Check(props.Rocks().size() == repeatedProps.Rocks().size()
              && props.FallenLogs().size() == repeatedProps.FallenLogs().size()
              && props.Driftwood().size() == repeatedProps.Driftwood().size()
              && props.BeachDetails().size() == repeatedProps.BeachDetails().size()
              && props.FallenFronds().size() == repeatedProps.FallenFronds().size(),
          "natural-prop populations are deterministic");
    std::size_t solidRockCount = 0U;
    std::size_t reefRockCount = 0U;
    std::size_t clearingLandmarkRockCount = 0U;
    for (std::size_t index = 0; index < props.Rocks().size(); ++index) {
        const island::RockPlacement& rock = props.Rocks()[index];
        const island::RockPlacement& repeated = repeatedProps.Rocks()[index];
        const island::TerrainSample sample = terrain.Sample(rock.x, rock.z);
        Check(rock.x == repeated.x && rock.y == repeated.y && rock.z == repeated.z
                  && rock.yawRadians == repeated.yawRadians
                  && rock.scaleX == repeated.scaleX && rock.scaleY == repeated.scaleY
                  && rock.scaleZ == repeated.scaleZ && rock.variant == repeated.variant
                  && rock.solid == repeated.solid && rock.offshore == repeated.offshore
                  && rock.clearingLandmark == repeated.clearingLandmark,
              "rock placement is bitwise deterministic for one seed");
        if (rock.offshore) {
            ++reefRockCount;
            Check(sample.signedShoreDistance <= -1.8f
                      && sample.signedShoreDistance >= -9.4f,
                  "reef outcrops remain inside the authored shallow-water belt");
            Check(sample.height <= -0.04f && rock.solid,
                  "reef outcrops originate on submerged support and remain solid");
            Check(rock.y + rock.scaleY * 1.03f >= 0.39f,
                  "reef outcrops remain visibly emergent above the ocean surface");
            Check(rock.scaleX > 0.90f && rock.scaleX < 3.55f
                      && rock.scaleY > 0.70f && rock.scaleY < 3.40f
                      && rock.scaleZ > 0.90f && rock.scaleZ < 3.60f,
                  "reef-outcrop axis scales remain inside their authored bounds");
        } else {
            if (rock.clearingLandmark) {
                Check(sample.signedShoreDistance >= 30.0f,
                      "authored clearing rocks remain on deep interior ground");
            } else {
                Check(sample.signedShoreDistance >= 1.5f
                          && sample.signedShoreDistance <= 202.0f,
                      "procedural land rocks stay inside their placement belt");
            }
            Check(sample.height >= 0.15f && sample.slopeRadians <= 0.5759587f,
                  "land rocks remain on suitable unsubmerged ground");
            if (rock.clearingLandmark) {
                ++clearingLandmarkRockCount;
                const float eastDistance = std::hypot(
                    rock.x - island::VegetationLayout::EastClearing.x,
                    rock.z - island::VegetationLayout::EastClearing.z);
                const float westDistance = std::hypot(
                    rock.x - island::VegetationLayout::WestClearing.x,
                    rock.z - island::VegetationLayout::WestClearing.z);
                const island::ClearingDefinition& clearing = eastDistance < westDistance
                    ? island::VegetationLayout::EastClearing
                    : island::VegetationLayout::WestClearing;
                const float clearingDistance = std::min(eastDistance, westDistance);
                Check(clearingDistance > clearing.radius * 0.70f
                          && clearingDistance < clearing.radius * 0.92f,
                      "authored clearing rocks stay in a traversable outer landmark ring");
                Check(island::VegetationLayout::IsInsideAuthoredClearing(rock.x, rock.z),
                      "authored clearing rocks remain associated with a clearing");
            } else {
                Check(!island::VegetationLayout::IsReservedRoute(rock.x, rock.z),
                      "procedural land rocks keep authored routes and clearings open");
            }
            Check(rock.scaleX > 0.35f && rock.scaleX < 2.30f
                      && rock.scaleY > 0.24f && rock.scaleY < 1.70f
                      && rock.scaleZ > 0.35f && rock.scaleZ < 2.35f,
                  "land-rock axis scales remain inside conservative visual bounds");
        }
        Check(rock.variant < island::NaturalPropLayout::RockVariantCount,
              "rock variant index remains in range");
        if (rock.solid) ++solidRockCount;
    }
    Check(reefRockCount >= 12U && reefRockCount <= 34U,
          "reef outcrops stay inside their sparse shoreline-composition budget");
    Check(clearingLandmarkRockCount
              == static_cast<std::size_t>(
                  island::NaturalPropLayout::AuthoredClearingRockCount),
          "both clearing landmark rock groups remain complete");
    std::size_t clearingLandmarkLogCount = 0U;
    for (std::size_t index = 0; index < props.FallenLogs().size(); ++index) {
        const island::FallenLogPlacement& log = props.FallenLogs()[index];
        const island::FallenLogPlacement& repeated = repeatedProps.FallenLogs()[index];
        Check(log.x == repeated.x && log.y == repeated.y && log.z == repeated.z
                  && log.yawRadians == repeated.yawRadians && log.length == repeated.length
                  && log.radius == repeated.radius && log.variant == repeated.variant
                  && log.clearingLandmark == repeated.clearingLandmark,
              "fallen-log placement is bitwise deterministic for one seed");
        Check(log.length >= island::NaturalPropLayout::LogMinimumLength
                  && log.length <= island::NaturalPropLayout::LogMaximumLength,
              "fallen-log length remains in the authored range");
        Check(log.radius >= 0.24f && log.radius <= 0.43f,
              "fallen-log radius remains in the authored range");
        Check(log.variant < island::NaturalPropLayout::LogVariantCount,
              "fallen-log variant index remains in range");
        const float directionX = std::cos(log.yawRadians);
        const float directionZ = std::sin(log.yawRadians);
        for (int sampleIndex = 0; sampleIndex < 7; ++sampleIndex) {
            const float t = static_cast<float>(sampleIndex) / 6.0f - 0.5f;
            const float sampleX = log.x + directionX * log.length * t;
            const float sampleZ = log.z + directionZ * log.length * t;
            if (log.clearingLandmark) {
                Check(island::VegetationLayout::IsInsideAuthoredClearing(sampleX, sampleZ),
                      "the complete landmark-log footprint stays inside its clearing");
                const island::ClearingDefinition& clearing =
                    island::VegetationLayout::WestClearing;
                Check(std::hypot(sampleX - clearing.x, sampleZ - clearing.z)
                          > clearing.radius * 0.56f,
                      "the landmark log preserves the clearing's central walking room");
            } else {
                Check(!island::VegetationLayout::IsReservedRoute(sampleX, sampleZ),
                      "procedural fallen logs stay outside authored routes");
            }
            Check(terrain.Sample(sampleX, sampleZ).signedShoreDistance >= 30.0f,
                  "the complete fallen-log footprint stays in the island interior");
        }
        if (log.clearingLandmark) ++clearingLandmarkLogCount;
    }
    Check(clearingLandmarkLogCount
              == static_cast<std::size_t>(
                  island::NaturalPropLayout::AuthoredClearingLogCount),
          "the west-clearing fallen-log landmark remains present");
    std::array<bool, island::NaturalPropLayout::DriftwoodVariantCount>
        observedDriftwoodVariants{};
    for (std::size_t index = 0; index < props.Driftwood().size(); ++index) {
        const island::DriftwoodPlacement& wood = props.Driftwood()[index];
        const island::DriftwoodPlacement& repeated = repeatedProps.Driftwood()[index];
        Check(wood.x == repeated.x && wood.y == repeated.y && wood.z == repeated.z
                  && wood.yawRadians == repeated.yawRadians && wood.length == repeated.length
                  && wood.radius == repeated.radius && wood.variant == repeated.variant,
              "driftwood placement is bitwise deterministic for one seed");
        Check(wood.length >= island::NaturalPropLayout::DriftwoodMinimumLength
                  && wood.length <= island::NaturalPropLayout::DriftwoodMaximumLength,
              "driftwood length remains in the authored range");
        Check(wood.radius >= 0.09f && wood.radius <= 0.18f,
              "driftwood radius remains in the authored range");
        Check(wood.variant < island::NaturalPropLayout::DriftwoodVariantCount,
              "driftwood variant index remains in range");
        if (wood.variant < observedDriftwoodVariants.size()) {
            observedDriftwoodVariants[wood.variant] = true;
        }
        const float directionX = std::cos(wood.yawRadians);
        const float directionZ = std::sin(wood.yawRadians);
        for (int sampleIndex = 0; sampleIndex < 5; ++sampleIndex) {
            const float t = static_cast<float>(sampleIndex) / 4.0f - 0.5f;
            const float sampleX = wood.x + directionX * wood.length * t;
            const float sampleZ = wood.z + directionZ * wood.length * t;
            const island::TerrainSample support = terrain.Sample(sampleX, sampleZ);
            Check(support.signedShoreDistance >= 1.5f
                      && support.signedShoreDistance <= 27.0f,
                  "the complete driftwood footprint stays in the dry beach belt");
            Check(!island::VegetationLayout::IsReservedRoute(sampleX, sampleZ),
                  "the complete driftwood footprint stays outside authored routes");
        }
    }
    for (const bool observed : observedDriftwoodVariants) {
        Check(observed, "every authored driftwood silhouette remains represented");
    }
    std::array<bool, island::NaturalPropLayout::BeachDetailVariantCount>
        observedBeachDetailVariants{};
    for (std::size_t index = 0; index < props.BeachDetails().size(); ++index) {
        const island::BeachDetailPlacement& detail = props.BeachDetails()[index];
        const island::BeachDetailPlacement& repeated = repeatedProps.BeachDetails()[index];
        const island::TerrainSample sample = terrain.Sample(detail.x, detail.z);
        Check(detail.x == repeated.x && detail.y == repeated.y
                  && detail.z == repeated.z
                  && detail.yawRadians == repeated.yawRadians
                  && detail.scale == repeated.scale && detail.variant == repeated.variant,
              "shell/pebble placement is bitwise deterministic for one seed");
        Check(sample.signedShoreDistance >= 3.0f
                  && sample.signedShoreDistance <= 18.0f,
              "shell/pebble clusters remain in the authored dry-sand belt");
        Check(sample.height >= 0.22f && sample.slopeRadians <= 0.2617994f,
              "shell/pebble clusters stay on dry low-slope support");
        CheckNear(detail.y, sample.height + 0.012f, 0.0001f,
                  "shell/pebble clusters rest just above authoritative terrain");
        Check(!island::VegetationLayout::IsReservedRoute(detail.x, detail.z),
              "shell/pebble clusters keep authored routes and spawn clear");
        Check(detail.variant < observedBeachDetailVariants.size(),
              "shell/pebble cluster variant index remains in range");
        if (detail.variant < observedBeachDetailVariants.size()) {
            observedBeachDetailVariants[detail.variant] = true;
        }
        Check(detail.scale >= island::NaturalPropLayout::BeachDetailMinimumScale
                  && detail.scale <= island::NaturalPropLayout::BeachDetailMaximumScale,
              "shell/pebble cluster scale remains in the authored range");
    }
    Check(std::ranges::all_of(observedBeachDetailVariants,
                              [](const bool value) { return value; }),
          "shell/pebble population exercises all four mesh variants");
    std::array<bool, island::NaturalPropLayout::FallenFrondVariantCount>
        observedFallenFrondVariants{};
    for (std::size_t index = 0; index < props.FallenFronds().size(); ++index) {
        const island::FallenFrondPlacement& frond = props.FallenFronds()[index];
        const island::FallenFrondPlacement& repeated = repeatedProps.FallenFronds()[index];
        Check(frond.x == repeated.x && frond.y == repeated.y && frond.z == repeated.z
                  && frond.yawRadians == repeated.yawRadians
                  && frond.scale == repeated.scale && frond.variant == repeated.variant,
              "fallen-frond placement is bitwise deterministic for one seed");
        Check(frond.scale >= island::NaturalPropLayout::FallenFrondMinimumScale
                  && frond.scale <= island::NaturalPropLayout::FallenFrondMaximumScale,
              "fallen-frond scale remains in the authored range");
        Check(frond.variant < observedFallenFrondVariants.size(),
              "fallen-frond variant index remains in range");
        if (frond.variant < observedFallenFrondVariants.size()) {
            observedFallenFrondVariants[frond.variant] = true;
        }
        const float directionX = std::cos(frond.yawRadians);
        const float directionZ = std::sin(frond.yawRadians);
        float maximumSupportHeight = -std::numeric_limits<float>::infinity();
        for (int sampleIndex = 0; sampleIndex < 5; ++sampleIndex) {
            const float t = static_cast<float>(sampleIndex) / 4.0f - 0.5f;
            const float sampleX = frond.x + directionX * frond.scale * t;
            const float sampleZ = frond.z + directionZ * frond.scale * t;
            const island::TerrainSample support = terrain.Sample(sampleX, sampleZ);
            maximumSupportHeight = std::max(maximumSupportHeight, support.height);
            Check(support.signedShoreDistance >= 6.0f
                      && support.signedShoreDistance <= 190.0f,
                  "the complete fallen-frond footprint stays on coastal/interior ground");
            Check(support.slopeRadians <= 0.34906585f,
                  "the complete fallen-frond footprint stays on gentle support");
            Check(!island::VegetationLayout::IsReservedRoute(sampleX, sampleZ),
                  "the complete fallen-frond footprint keeps authored routes open");
        }
        CheckNear(frond.y, maximumSupportHeight + 0.022f, 0.0001f,
                  "fallen fronds rest on the highest sampled terrain support");
        for (std::size_t other = 0; other < index; ++other) {
            const island::FallenFrondPlacement& previous = props.FallenFronds()[other];
            Check(std::hypot(frond.x - previous.x, frond.z - previous.z) >= 2.899f,
                  "fallen fronds preserve their minimum centre spacing");
        }
    }
    Check(std::ranges::all_of(observedFallenFrondVariants,
                              [](const bool value) { return value; }),
          "fallen-frond population exercises all three colour/shape variants");
    Check(props.Obstacles().size() == solidRockCount + props.FallenLogs().size() * 7U,
          "natural-prop collision excludes small decorative props and matches solid spans");

    for (int z = -360; z <= 360; z += 12) {
        for (int x = -360; x <= 360; x += 12) {
            const float first = terrain.Height(static_cast<float>(x), static_cast<float>(z));
            const float second = terrain.Height(static_cast<float>(x), static_cast<float>(z));
            Check(std::isfinite(first), "terrain height is finite across the world grid");
            Check(first == second, "terrain generation is deterministic");
            Check(first >= -22.01f && first < 40.0f, "terrain height stays in the authored scale");
            const island::TerrainSample sample = terrain.Sample(static_cast<float>(x),
                                                                 static_cast<float>(z));
            Check(std::isfinite(sample.slopeRadians), "terrain slope is finite across the world grid");
            Check(sample.slopeRadians >= 0.0f
                      && sample.slopeRadians <= std::numbers::pi_v<float> * 0.5f,
                  "terrain slope remains in the upward-normal range");
        }
    }

    island::PlayerMotion player;
    player.SetPosition(terrain, 0.0f, 0.0f);
    const island::PlayerPosition start = player.Position();
    Check(island::PlayerMotion::GroundContactOffsets.size() == 9U,
          "capsule ground query exposes all nine diagnostic samples");
    CheckNear(island::PlayerMotion::GroundContactOffsets.front().x, 0.0f, 0.0001f,
              "capsule ground query begins at the footprint centre x");
    CheckNear(island::PlayerMotion::GroundContactOffsets.front().z, 0.0f, 0.0001f,
              "capsule ground query begins at the footprint centre z");
    float maximumGroundSampleHeight = -std::numeric_limits<float>::infinity();
    for (const island::PlayerGroundOffset& offset
         : island::PlayerMotion::GroundContactOffsets) {
        Check(std::hypot(offset.x, offset.z) <= 1.0001f,
              "capsule ground sample stays inside the unit footprint");
        const island::TerrainSample sample = terrain.Sample(
            start.x + offset.x * island::PlayerMotion::CapsuleRadius,
            start.z + offset.z * island::PlayerMotion::CapsuleRadius);
        Check(std::isfinite(sample.height) && std::isfinite(sample.slopeRadians),
              "capsule diagnostic sample is finite");
        maximumGroundSampleHeight = std::max(maximumGroundSampleHeight, sample.height);
    }
    CheckNear(maximumGroundSampleHeight, player.GroundHeight(), 0.0001f,
              "capsule diagnostics reproduce the authoritative support height");
    Check(player.GroundHeight() >= terrain.Height(0.0f, 0.0f),
          "capsule footprint resolves at or above its centre terrain height");
    Check(player.Position().y >= player.GroundHeight() + island::PlayerMotion::EyeHeight - 0.0001f,
          "walking eye never starts below its resolved ground support");
    Check(player.GroundSlopeRadians() >= terrain.Sample(0.0f, 0.0f).slopeRadians,
          "capsule ground slope includes the centre sample");
    player.Update(terrain, 1.0f, 0.0f, 1.0f / 60.0f);
    Check(player.Position().x > start.x, "walking input accelerates the player");
    const island::PlayerMovementTrace firstMovementTrace = player.MovementTrace();
    Check(firstMovementTrace.valid && firstMovementTrace.startX == start.x
              && firstMovementTrace.attemptedX > firstMovementTrace.startX,
          "movement diagnostics retain the exact nonzero attempted step");
    CheckNear(firstMovementTrace.resolvedX, player.Position().x, 0.0001f,
              "movement diagnostics retain the exact resolved player position");
    Check(firstMovementTrace.obstacleRejections == 0
              && firstMovementTrace.terrainRejections == 0
              && firstMovementTrace.slideSubsteps == 0
              && !firstMovementTrace.safetyClamped,
          "unobstructed movement diagnostics report no rejection or clamp");
    Check(player.VelocityX() > 0.0f && player.VelocityX() < island::PlayerMotion::WalkSpeed,
          "walking acceleration does not jump to full speed");
    Check(player.Mode() == island::LocomotionMode::Walking,
          "interior movement stays in walking mode");
    CheckNear(player.OceanCurrentStrength(terrain), 0.0f, 0.0001f,
              "ocean current remains inactive while walking");

    island::PlayerMotion invalidInputPlayer;
    invalidInputPlayer.SetPosition(terrain, 12.0f, -9.0f);
    const island::PlayerPosition validPlayerPosition = invalidInputPlayer.Position();
    invalidInputPlayer.Update(terrain, quietNan,
                              std::numeric_limits<float>::infinity(), quietNan);
    Check(invalidInputPlayer.Position().x == validPlayerPosition.x
              && invalidInputPlayer.Position().y == validPlayerPosition.y
              && invalidInputPlayer.Position().z == validPlayerPosition.z
              && invalidInputPlayer.VelocityX() == 0.0f
              && invalidInputPlayer.VelocityZ() == 0.0f,
          "player movement rejects non-finite frame and direction input without state poisoning");
    invalidInputPlayer.SetPosition(
        terrain, quietNan, std::numeric_limits<float>::infinity());
    Check(invalidInputPlayer.Position().x == validPlayerPosition.x
              && invalidInputPlayer.Position().z == validPlayerPosition.z
              && std::isfinite(invalidInputPlayer.Position().y),
          "player placement retains the last valid coordinates for non-finite input");

    island::PlayerMotion runningPlayer;
    runningPlayer.SetPosition(terrain, 0.0f, 0.0f);
    runningPlayer.SetRunning(true);
    for (int frame = 0; frame < 120; ++frame) {
        runningPlayer.Update(terrain, 0.0f, 1.0f, 1.0f / 60.0f);
    }
    Check(island::PlayerMotion::RunSpeed >= 8.0f,
          "selected run mode provides the faster requested land pace");
    Check(runningPlayer.IsRunning(), "selected run mode activates the running land pace");
    CheckNear(std::hypot(runningPlayer.VelocityX(), runningPlayer.VelocityZ()),
              island::PlayerMotion::RunSpeed, 0.001f,
              "running acceleration reaches the bounded running speed");
    runningPlayer.SetRunning(false);
    for (int frame = 0; frame < 120; ++frame) {
        runningPlayer.Update(terrain, 0.0f, 1.0f, 1.0f / 60.0f);
    }
    Check(!runningPlayer.IsRunning(), "selecting walk mode restores the walking land pace");
    CheckNear(std::hypot(runningPlayer.VelocityX(), runningPlayer.VelocityZ()),
              island::PlayerMotion::WalkSpeed, 0.001f,
              "selected walk mode settles at the bounded walking speed");

    float coveAngle = 0.0f;
    float headlandAngle = 0.0f;
    float minimumShoreRadius = std::numeric_limits<float>::infinity();
    float maximumShoreRadius = 0.0f;
    for (int sampleIndex = 0; sampleIndex < 720; ++sampleIndex) {
        const float angle = -std::numbers::pi_v<float>
            + 2.0f * std::numbers::pi_v<float>
                * static_cast<float>(sampleIndex) / 720.0f;
        const float radius = terrain.ShoreRadius(angle);
        if (radius < minimumShoreRadius) {
            minimumShoreRadius = radius;
            coveAngle = angle;
        }
        if (radius > maximumShoreRadius) {
            maximumShoreRadius = radius;
            headlandAngle = angle;
        }
    }
    const std::array<float, 6> shorelineTransects = {
        coveAngle, headlandAngle, -2.35f, -0.78f, 0.69f, 2.18f};
    int completedShorelineRoundTrips = 0;
    bool shorelineTraversalStayedFinite = true;
    for (const float angle : shorelineTransects) {
        const float outwardX = std::cos(angle);
        const float outwardZ = std::sin(angle);
        const float shoreRadius = terrain.ShoreRadius(angle);
        island::PlayerMotion transectPlayer;
        transectPlayer.SetPosition(
            terrain, outwardX * (shoreRadius - 14.0f),
            outwardZ * (shoreRadius - 14.0f));
        Check(transectPlayer.Mode() == island::LocomotionMode::Walking,
              "shoreline traversal begins on walkable dry ground");

        int outwardWaterEntries = 0;
        float maximumOffshoreDistance = 0.0f;
        island::LocomotionMode previousMode = transectPlayer.Mode();
        for (int step = 0; step < 900; ++step) {
            transectPlayer.Update(terrain, outwardX, outwardZ, 1.0f / 60.0f);
            if (previousMode == island::LocomotionMode::Walking
                && transectPlayer.Mode() == island::LocomotionMode::Swimming) {
                ++outwardWaterEntries;
            }
            previousMode = transectPlayer.Mode();
            maximumOffshoreDistance = std::max(
                maximumOffshoreDistance, transectPlayer.OffshoreDistance(terrain));
            shorelineTraversalStayedFinite = shorelineTraversalStayedFinite
                && std::isfinite(transectPlayer.Position().x)
                && std::isfinite(transectPlayer.Position().y)
                && std::isfinite(transectPlayer.Position().z);
            if (transectPlayer.Mode() == island::LocomotionMode::Swimming
                && maximumOffshoreDistance >= 20.0f) {
                break;
            }
        }

        int inwardLandExits = 0;
        previousMode = transectPlayer.Mode();
        for (int step = 0; step < 1200; ++step) {
            transectPlayer.Update(terrain, -outwardX, -outwardZ, 1.0f / 60.0f);
            if (previousMode == island::LocomotionMode::Swimming
                && transectPlayer.Mode() == island::LocomotionMode::Walking) {
                ++inwardLandExits;
            }
            previousMode = transectPlayer.Mode();
            shorelineTraversalStayedFinite = shorelineTraversalStayedFinite
                && std::isfinite(transectPlayer.Position().x)
                && std::isfinite(transectPlayer.Position().y)
                && std::isfinite(transectPlayer.Position().z);
            if (transectPlayer.Mode() == island::LocomotionMode::Walking
                && terrain.SignedShoreDistance(
                    transectPlayer.Position().x, transectPlayer.Position().z) >= 5.0f) {
                break;
            }
        }

        Check(outwardWaterEntries == 1 && maximumOffshoreDistance >= 20.0f,
              "each authored shoreline transect enters stable near-shore swimming once");
        Check(inwardLandExits == 1
                  && transectPlayer.Mode() == island::LocomotionMode::Walking
                  && terrain.SignedShoreDistance(
                      transectPlayer.Position().x, transectPlayer.Position().z) >= 5.0f,
              "each authored shoreline transect returns to stable dry-ground walking once");
        if (outwardWaterEntries == 1 && inwardLandExits == 1) {
            ++completedShorelineRoundTrips;
        }
    }
    Check(completedShorelineRoundTrips
              == static_cast<int>(shorelineTransects.size()),
          "cove, headland, and varied-coast traversal scenarios all complete");
    Check(shorelineTraversalStayedFinite,
          "all shoreline traversal positions remain finite");

    bool foundSteepBankScenario = false;
    float steepBankSlope = 0.0f;
    float steepBankStartX = 0.0f;
    float steepBankStartZ = 0.0f;
    float steepBankUphillX = 0.0f;
    float steepBankUphillZ = 0.0f;
    for (int z = -220; z <= 220; z += 2) {
        for (int x = -220; x <= 220; x += 2) {
            const float targetX = static_cast<float>(x);
            const float targetZ = static_cast<float>(z);
            const island::TerrainSample target = terrain.Sample(targetX, targetZ);
            if (target.signedShoreDistance < 24.0f
                || target.height < 0.2f
                || target.slopeRadians <= steepBankSlope) {
                continue;
            }
            const float gradientX = terrain.Height(targetX + 0.5f, targetZ)
                - terrain.Height(targetX - 0.5f, targetZ);
            const float gradientZ = terrain.Height(targetX, targetZ + 0.5f)
                - terrain.Height(targetX, targetZ - 0.5f);
            const float gradientLength = std::hypot(gradientX, gradientZ);
            if (gradientLength < 0.05f) continue;
            const float uphillX = gradientX / gradientLength;
            const float uphillZ = gradientZ / gradientLength;
            const float startX = targetX - uphillX * 2.4f;
            const float startZ = targetZ - uphillZ * 2.4f;
            const island::TerrainSample bankStart = terrain.Sample(startX, startZ);
            if (bankStart.height < 0.2f
                || bankStart.slopeRadians > island::PlayerMotion::MaximumWalkSlopeRadians
                || bankStart.signedShoreDistance < 20.0f) {
                continue;
            }
            foundSteepBankScenario = true;
            steepBankSlope = target.slopeRadians;
            steepBankStartX = startX;
            steepBankStartZ = startZ;
            steepBankUphillX = uphillX;
            steepBankUphillZ = uphillZ;
        }
    }
    Check(foundSteepBankScenario && steepBankSlope >= 0.30f,
          "deterministic interior search finds a representative steep-bank approach");
    if (foundSteepBankScenario) {
        island::PlayerMotion bankWalker;
        bankWalker.SetPosition(terrain, steepBankStartX, steepBankStartZ);
        const float initialGroundHeight = bankWalker.GroundHeight();
        float minimumGroundHeight = initialGroundHeight;
        float maximumGroundHeight = initialGroundHeight;
        int rejectionCount = 0;
        bool bankTraversalStayedFinite = true;
        for (int step = 0; step < 360; ++step) {
            bankWalker.Update(
                terrain, steepBankUphillX, steepBankUphillZ, 1.0f / 60.0f);
            rejectionCount += bankWalker.MovementTrace().terrainRejections;
            minimumGroundHeight = std::min(minimumGroundHeight, bankWalker.GroundHeight());
            maximumGroundHeight = std::max(maximumGroundHeight, bankWalker.GroundHeight());
            bankTraversalStayedFinite = bankTraversalStayedFinite
                && std::isfinite(bankWalker.Position().x)
                && std::isfinite(bankWalker.Position().y)
                && std::isfinite(bankWalker.Position().z);
        }
        const float uphillProgress =
            (bankWalker.Position().x - steepBankStartX) * steepBankUphillX
            + (bankWalker.Position().z - steepBankStartZ) * steepBankUphillZ;
        Check(uphillProgress > 0.5f || rejectionCount > 0,
              "steep-bank traversal either climbs valid support or exercises slope rejection");
        Check(maximumGroundHeight - minimumGroundHeight > 0.20f || rejectionCount > 0,
              "steep-bank traversal observes elevation change or an authoritative limit");
        Check(bankWalker.Mode() == island::LocomotionMode::Walking,
              "steep-bank traversal cannot spuriously enter swimming mode");
        Check(bankTraversalStayedFinite,
              "steep-bank traversal remains numerically stable");
    }

    const float eastShore = terrain.ShoreRadius(0.0f);
    player.SetPosition(terrain, eastShore + 24.0f, 0.0f);
    Check(player.Mode() == island::LocomotionMode::Swimming,
          "sufficiently deep near-shore water enters swimming mode");
    CheckNear(player.Position().y, 0.58f, 0.001f, "swimming eye rests near the water surface");
    CheckNear(player.OceanCurrentStrength(terrain), 0.0f, 0.0001f,
              "ocean current remains inactive inside the near-shore swimming area");

    const island::CircularObstacle testReef{eastShore + 25.4f, 0.0f, 0.62f};
    island::PlayerMotion reefSwimmer;
    reefSwimmer.SetPosition(terrain, eastShore + 24.0f, 0.0f);
    for (int step = 0; step < 180; ++step) {
        reefSwimmer.Update(
            terrain, 1.0f, 0.0f, 1.0f / 60.0f,
            std::span<const island::CircularObstacle>(&testReef, 1U));
    }
    const float reefClearance = island::PlayerMotion::CapsuleRadius + testReef.radius;
    Check(reefSwimmer.Position().x <= testReef.x - reefClearance + 0.001f,
          "a near-shore swimmer cannot pass through an emergent-rock proxy");
    Check(reefSwimmer.Mode() == island::LocomotionMode::Swimming,
          "emergent-rock collision is exercised while the controller remains in swimming mode");
    const island::WorldCollisionGrid reefCollisionGrid(
        std::span<const island::CircularObstacle>(&testReef, 1U));
    island::PlayerMotion indexedReefSwimmer;
    indexedReefSwimmer.SetPosition(terrain, eastShore + 24.0f, 0.0f);
    for (int step = 0; step < 180; ++step) {
        indexedReefSwimmer.Update(terrain, 1.0f, 0.0f, 1.0f / 60.0f,
                                  reefCollisionGrid);
    }
    CheckNear(indexedReefSwimmer.Position().x, reefSwimmer.Position().x, 0.0001f,
              "indexed and exhaustive swimming collision stop at the same position");
    CheckNear(indexedReefSwimmer.Position().z, reefSwimmer.Position().z, 0.0001f,
              "indexed and exhaustive swimming collision preserve identical axis sliding");

    player.SetPosition(terrain, eastShore + island::PlayerMotion::FullCurrentDistance, 0.0f);
    CheckNear(player.OceanCurrentStrength(terrain), 1.0f, 0.0001f,
              "ocean current reaches full normalized strength at its documented distance");

    player.SetPosition(terrain, eastShore + 80.0f, 0.0f);
    const float offshoreBeforeCurrent = player.OffshoreDistance(terrain);
    for (int step = 0; step < 120; ++step) {
        player.Update(terrain, 0.0f, 0.0f, 1.0f / 60.0f);
    }
    Check(player.OffshoreDistance(terrain) < offshoreBeforeCurrent,
          "outer-ocean current gently carries an idle swimmer shoreward");

    player.SetPosition(terrain, eastShore + 88.0f, 0.0f);
    for (int step = 0; step < 900; ++step) {
        player.Update(terrain, 1.0f, 0.0f, 1.0f / 60.0f);
        Check(player.OffshoreDistance(terrain) <= island::PlayerMotion::SafetyDistance + 0.01f,
              "swimmer cannot escape the ocean safety boundary");
    }
    Check(player.VelocityX() <= 0.01f,
          "safety boundary removes residual outward velocity");

    island::PlayerMotion safetyTracePlayer;
    safetyTracePlayer.SetPosition(
        terrain, eastShore + island::PlayerMotion::SafetyDistance + 4.0f, 0.0f);
    safetyTracePlayer.Update(terrain, 0.0f, 0.0f, 1.0f / 60.0f);
    Check(safetyTracePlayer.MovementTrace().safetyClamped
              && safetyTracePlayer.OffshoreDistance(terrain)
                  <= island::PlayerMotion::SafetyDistance + 0.01f,
          "movement diagnostics record an explicit ocean safety-boundary correction");

    player.SetPosition(terrain, 0.0f, 0.0f);
    player.Update(terrain, 1.0f, 0.0f, 5.0f);
    Check(player.Position().x - 0.0f < 0.2f,
          "long frame is clamped and cannot teleport the player");

    player.SetPosition(terrain, 35.0f, -20.0f);
    for (int step = 0; step < 600; ++step) {
        const float inputX = std::sin(static_cast<float>(step) * 0.071f);
        const float inputZ = std::cos(static_cast<float>(step) * 0.053f);
        player.Update(terrain, inputX, inputZ, step % 37 == 0 ? 0.1f : 1.0f / 60.0f);
        if (player.Mode() == island::LocomotionMode::Walking) {
            Check(player.Position().y
                      >= player.GroundHeight() + island::PlayerMotion::EyeHeight - 0.0001f,
                  "walking camera remains above the capsule terrain support");
        }
    }

    player.SetPosition(terrain, 0.0f, 0.0f);
    const island::CircularObstacle testTrunk{1.2f, 0.0f, 0.42f};
    for (int step = 0; step < 180; ++step) {
        player.Update(terrain, 1.0f, 0.0f, 1.0f / 60.0f,
                      std::span<const island::CircularObstacle>(&testTrunk, 1U));
    }
    const float trunkDistance = std::hypot(player.Position().x - testTrunk.x,
                                            player.Position().z - testTrunk.z);
    Check(trunkDistance >= island::PlayerMotion::CapsuleRadius + testTrunk.radius - 0.001f,
          "player capsule cannot enter a palm trunk proxy");
    Check(player.Position().x < testTrunk.x,
          "blocked movement does not tunnel through a trunk");
    Check(player.MovementTrace().obstacleRejections > 0
              && player.MovementTrace().terrainRejections == 0
              && player.MovementTrace().slideSubsteps > 0
              && player.MovementTrace().attemptedX > player.MovementTrace().resolvedX,
          "movement diagnostics distinguish blocked direct motion and axis sliding");

    const island::WorldCollisionGrid testCollisionGrid(
        std::span<const island::CircularObstacle>(&testTrunk, 1U));
    island::PlayerMotion indexedPlayer;
    indexedPlayer.SetPosition(terrain, 0.0f, 0.0f);
    for (int step = 0; step < 180; ++step) {
        indexedPlayer.Update(terrain, 1.0f, 0.0f, 1.0f / 60.0f,
                             testCollisionGrid);
    }
    CheckNear(indexedPlayer.Position().x, player.Position().x, 0.0001f,
              "indexed and exhaustive obstacle paths stop at the same position");
    CheckNear(indexedPlayer.Position().z, player.Position().z, 0.0001f,
              "indexed and exhaustive obstacle paths preserve identical axis sliding");

    const island::CircularObstacle* selectedRockObstacle = nullptr;
    float rockApproachX = 0.0f;
    float rockApproachZ = 0.0f;
    float rockStartX = 0.0f;
    float rockStartZ = 0.0f;
    for (const island::RockPlacement& rock : props.Rocks()) {
        if (!rock.solid || rock.offshore
            || terrain.SignedShoreDistance(rock.x, rock.z) < 28.0f
            || terrain.Sample(rock.x, rock.z).slopeRadians > 0.30f) {
            continue;
        }
        const auto obstacle = std::ranges::find_if(
            props.Obstacles(), [&rock](const island::CircularObstacle& candidate) {
                return std::abs(candidate.x - rock.x) < 0.0001f
                    && std::abs(candidate.z - rock.z) < 0.0001f;
            });
        if (obstacle == props.Obstacles().end()) continue;
        const float radiusFromCentre = std::hypot(rock.x, rock.z);
        if (radiusFromCentre < 1.0f) continue;
        const float outwardX = rock.x / radiusFromCentre;
        const float outwardZ = rock.z / radiusFromCentre;
        const float startX = rock.x - outwardX * (obstacle->radius + 2.4f);
        const float startZ = rock.z - outwardZ * (obstacle->radius + 2.4f);
        const island::TerrainSample approachStart = terrain.Sample(startX, startZ);
        if (approachStart.height < 0.2f
            || approachStart.slopeRadians > island::PlayerMotion::MaximumWalkSlopeRadians
            || approachStart.signedShoreDistance < 18.0f) {
            continue;
        }
        selectedRockObstacle = &*obstacle;
        rockApproachX = outwardX;
        rockApproachZ = outwardZ;
        rockStartX = startX;
        rockStartZ = startZ;
        break;
    }
    Check(selectedRockObstacle != nullptr,
          "deterministic natural-prop layout supplies a walkable land-rock approach");
    if (selectedRockObstacle != nullptr) {
        island::PlayerMotion rockWalker;
        rockWalker.SetPosition(terrain, rockStartX, rockStartZ);
        int rockObstacleRejections = 0;
        int rockTerrainRejections = 0;
        for (int step = 0; step < 360; ++step) {
            rockWalker.Update(
                terrain, rockApproachX, rockApproachZ, 1.0f / 60.0f,
                std::span<const island::CircularObstacle>(selectedRockObstacle, 1U));
            rockObstacleRejections += rockWalker.MovementTrace().obstacleRejections;
            rockTerrainRejections += rockWalker.MovementTrace().terrainRejections;
        }
        const float finalClearance = std::hypot(
            rockWalker.Position().x - selectedRockObstacle->x,
            rockWalker.Position().z - selectedRockObstacle->z);
        const float requiredClearance = island::PlayerMotion::CapsuleRadius
            + selectedRockObstacle->radius;
        const float approachProgress =
            (rockWalker.Position().x - rockStartX) * rockApproachX
            + (rockWalker.Position().z - rockStartZ) * rockApproachZ;
        Check(approachProgress > 0.5f,
              "player advances along the deterministic land-rock approach");
        Check(finalClearance >= requiredClearance - 0.001f,
              "player capsule cannot tunnel through an authored land-rock proxy");
        Check(rockObstacleRejections > 0 && rockTerrainRejections == 0,
              "authored land-rock traversal is rejected by collision rather than terrain");
    }

    if (failures != 0) {
        std::cerr << failures << " core test assertion(s) failed\n";
        return EXIT_FAILURE;
    }
    std::cout << "IslandCoreTests: all checks passed\n";
    return EXIT_SUCCESS;
}
