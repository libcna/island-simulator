#include "IslandSimulator/BirdFlock.hpp"
#include "IslandSimulator/DistantSceneryLayout.hpp"
#include "IslandSimulator/FishSchools.hpp"
#include "IslandSimulator/FirstPersonCameraMotion.hpp"
#include "IslandSimulator/GrassInteractionModel.hpp"
#include "IslandSimulator/GrassLayout.hpp"
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
    Check(surfLoop.samples.size() == 220500U && breezeLoop.samples.size() == 220500U,
          "procedural ambient loops are ten seconds long");
    Check(jungleLoop.sampleRate == 22050 && jungleLoop.samples.size() == 264600U,
          "daytime-jungle ambience has its intended twelve-second portable format");
    Check(waveAccent.sampleRate == 22050 && waveAccent.samples.size() == 88200U
              && waveAccentAlternate.sampleRate == 22050
              && waveAccentAlternate.samples.size() == 88200U,
          "both stronger-wave accents have the intended four-second portable format");
    Check(waterMovementLoop.sampleRate == 22050
              && waterMovementLoop.samples.size() == 132300U,
          "player water movement has its intended six-second portable format");
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
    Check(island::WildlifeActivationPolicy::UpdateDivisor(
              island::WildlifeSimulationTier::Full) == 1U
              && island::WildlifeActivationPolicy::UpdateDivisor(
                  island::WildlifeSimulationTier::Reduced) == 4U
              && island::WildlifeActivationPolicy::UpdateDivisor(
                  island::WildlifeSimulationTier::Dormant) == 30U,
          "wildlife tiers use the documented bounded update divisors");

    island::BirdFlock birds(terrain);
    island::BirdFlock repeatedBirds(terrain);
    Check(birds.Birds().size() == island::BirdFlock::BirdCount,
          "bird flock has the intended bounded population");
    int shoreBirds = 0;
    int canopyBirds = 0;
    int highBirds = 0;
    for (std::size_t index = 0; index < birds.Birds().size(); ++index) {
        const island::BirdState& bird = birds.Birds()[index];
        const island::BirdState& repeated = repeatedBirds.Birds()[index];
        Check(bird.x == repeated.x && bird.y == repeated.y && bird.z == repeated.z
                  && bird.headingRadians == repeated.headingRadians
                  && bird.speed == repeated.speed && bird.scale == repeated.scale
                  && bird.family == repeated.family
                  && bird.colorVariant == repeated.colorVariant,
              "bird initialization is deterministic");
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

    float maximumObservedBirdRadius = 0.0f;
    float minimumObservedBirdRadius = 1000.0f;
    bool observedWingUp = false;
    bool observedWingDown = false;
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
                      && bird.simulationTier == repeated.simulationTier,
                  "bird simulation remains deterministic over a long session");
            const float radius = std::hypot(bird.x, bird.z);
            maximumObservedBirdRadius = std::max(maximumObservedBirdRadius, radius);
            minimumObservedBirdRadius = std::min(minimumObservedBirdRadius, radius);
            Check(std::isfinite(bird.headingRadians) && std::isfinite(bird.wingPose),
                  "bird flight pose remains finite over a long session");
            Check(bird.y >= terrain.Height(bird.x, bird.z) + 14.8f,
                  "bird flight maintains its minimum terrain clearance");
            observedWingUp = observedWingUp || bird.wingPose > 0.55f;
            observedWingDown = observedWingDown || bird.wingPose < -0.55f;
        }
    }
    Check(minimumObservedBirdRadius > 35.0f && maximumObservedBirdRadius < 315.0f,
          "all bird route families remain in the authored island airspace");
    Check(observedWingUp && observedWingDown,
          "bird animation visits both wing stroke extremes");
    Check(birds.FullSimulationCount() + birds.ReducedSimulationCount()
              + birds.DormantSimulationCount() == static_cast<int>(island::BirdFlock::BirdCount),
          "bird simulation tiers account for the complete flock");

    island::FishSchools fishSchools(terrain);
    island::FishSchools repeatedFishSchools(terrain);
    Check(fishSchools.Fish().size() == island::FishSchools::FishCount,
          "fish schools have the intended bounded population");
    std::array<int, island::FishSchools::SchoolCount> fishPerSchool{};
    for (std::size_t index = 0; index < fishSchools.Fish().size(); ++index) {
        const island::FishState& fish = fishSchools.Fish()[index];
        const island::FishState& repeated = repeatedFishSchools.Fish()[index];
        const island::TerrainSample sample = terrain.Sample(fish.x, fish.z);
        Check(fish.x == repeated.x && fish.y == repeated.y && fish.z == repeated.z
                  && fish.headingRadians == repeated.headingRadians
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
    constexpr int fishSimulationFrames = 18000;
    for (int frame = 0; frame < fishSimulationFrames; ++frame) {
        const island::FishState observerFish = fishSchools.Fish().front();
        fishSchools.Update(1.0f / 30.0f, observerFish.x, observerFish.y, observerFish.z);
        repeatedFishSchools.Update(1.0f / 30.0f, observerFish.x,
                                   observerFish.y, observerFish.z);
        const auto fishStates = fishSchools.Fish();
        const auto repeatedStates = repeatedFishSchools.Fish();
        for (std::size_t index = 0; index < fishStates.size(); ++index) {
            const island::FishState& fish = fishStates[index];
            const island::FishState& repeated = repeatedStates[index];
            const island::TerrainSample sample = terrain.Sample(fish.x, fish.z);
            Check(fish.x == repeated.x && fish.y == repeated.y && fish.z == repeated.z
                      && fish.headingRadians == repeated.headingRadians
                      && fish.bodyWave == repeated.bodyWave
                      && fish.simulationTier == repeated.simulationTier,
                  "fish simulation remains deterministic over a long session");
            Check(std::isfinite(fish.x) && std::isfinite(fish.y)
                      && std::isfinite(fish.z) && std::isfinite(fish.headingRadians),
                  "fish pose remains finite over a long session");
            Check(sample.signedShoreDistance <= -2.8f
                      && sample.signedShoreDistance >= -48.2f,
                  "fish remain inside the authored near-shore water belt");
            Check(sample.height <= -0.5f && fish.y >= sample.height + 0.18f
                      && fish.y <= -0.18f,
                  "fish remain below the surface and above the seabed");
            observedFishTailLeft = observedFishTailLeft || fish.bodyWave > 0.65f;
            observedFishTailRight = observedFishTailRight || fish.bodyWave < -0.65f;
        }
    }
    const island::FishState finalLeadFish = fishSchools.Fish().front();
    Check(std::hypot(finalLeadFish.x - initialLeadFish.x,
                     finalLeadFish.z - initialLeadFish.z) > 20.0f,
          "fish schools traverse a meaningful section of shoreline over time");
    Check(observedFishTailLeft && observedFishTailRight,
          "fish animation visits both tail-stroke extremes");
    Check(fishSchools.FullSimulationCount() + fishSchools.ReducedSimulationCount()
              + fishSchools.DormantSimulationCount()
              == static_cast<int>(island::FishSchools::FishCount),
          "fish simulation tiers account for every school member");

    island::SharkPatrol shark(terrain);
    island::SharkPatrol repeatedShark(terrain);
    Check(shark.State().present && shark.State().visible
              && shark.State().activity == 1.0f,
          "shark begins with a bounded visible observation window");
    Check(shark.State().x == repeatedShark.State().x
              && shark.State().y == repeatedShark.State().y
              && shark.State().z == repeatedShark.State().z
              && shark.State().headingRadians == repeatedShark.State().headingRadians,
          "shark patrol initialization is deterministic");
    bool observedPresentShark = false;
    bool observedHiddenShark = false;
    bool observedSharkTailLeft = false;
    bool observedSharkTailRight = false;
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
                  && state.bodyWave == repeated.bodyWave
                  && state.activity == repeated.activity
                  && state.present == repeated.present && state.visible == repeated.visible
                  && state.simulationTier == repeated.simulationTier,
              "shark patrol remains deterministic over a long session");
        Check(std::isfinite(state.x) && std::isfinite(state.y)
                  && std::isfinite(state.z) && std::isfinite(state.headingRadians),
              "shark pose remains finite over a long session");
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
    }
    Check(shark.PresenceCycleCount() >= 8U,
          "shark alternates through several bounded presence cycles in twenty minutes");
    Check(observedPresentShark && observedHiddenShark,
          "shark reaches both fully present and fully hidden states");
    Check(observedSharkTailLeft && observedSharkTailRight,
          "shark animation visits both tail-stroke extremes");
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
                      && state.speed == repeated.speed
                      && state.animationSeconds == repeated.animationSeconds
                      && state.behavior == repeated.behavior
                      && state.simulationTier == repeated.simulationTier,
                  "monkey troop remains deterministic over a long session");
            Check(std::isfinite(state.x) && std::isfinite(state.y)
                      && std::isfinite(state.z) && std::isfinite(state.headingRadians)
                      && std::isfinite(state.animationSeconds),
                  "monkey pose remains finite over a long session");
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
    Check(grass.Patches().size() >= 10000U && grass.Patches().size() <= 40000U,
          "grass patch population stays inside the first geometry budget");
    Check(grass.Patches().size() == repeatedGrass.Patches().size(),
          "grass population is deterministic");
    for (std::size_t index = 0; index < grass.Patches().size(); ++index) {
        const island::GrassPlacement& patch = grass.Patches()[index];
        const island::GrassPlacement& repeated = repeatedGrass.Patches()[index];
        const island::TerrainSample sample = terrain.Sample(patch.x, patch.z);
        Check(patch.x == repeated.x && patch.y == repeated.y && patch.z == repeated.z
                  && patch.yawRadians == repeated.yawRadians && patch.scale == repeated.scale
                  && patch.spreadScale == repeated.spreadScale
                  && patch.windPhase == repeated.windPhase
                  && patch.colorVariation == repeated.colorVariation
                  && patch.pathScale == repeated.pathScale,
              "grass placement data is bitwise deterministic for one seed");
        Check(sample.signedShoreDistance >= 20.0f && sample.height >= 2.1f,
              "grass remains above the sand transition");
        Check(sample.slopeRadians <= 0.4886922f,
              "grass rejects terrain steeper than its placement limit");
        Check(patch.windPhase >= 0.0f && patch.windPhase < 1.0f
                  && patch.colorVariation >= 0.0f && patch.colorVariation < 1.0f,
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
    }

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
    for (std::size_t index = 0; index < props.Rocks().size(); ++index) {
        const island::RockPlacement& rock = props.Rocks()[index];
        const island::RockPlacement& repeated = repeatedProps.Rocks()[index];
        const island::TerrainSample sample = terrain.Sample(rock.x, rock.z);
        Check(rock.x == repeated.x && rock.y == repeated.y && rock.z == repeated.z
                  && rock.yawRadians == repeated.yawRadians
                  && rock.scaleX == repeated.scaleX && rock.scaleY == repeated.scaleY
                  && rock.scaleZ == repeated.scaleZ && rock.variant == repeated.variant
                  && rock.solid == repeated.solid && rock.offshore == repeated.offshore,
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
            Check(sample.signedShoreDistance >= 1.5f
                      && sample.signedShoreDistance <= 202.0f,
                  "land rocks stay inside their coastal/interior placement belt");
            Check(sample.height >= 0.15f && sample.slopeRadians <= 0.5759587f,
                  "land rocks remain on suitable unsubmerged ground");
            Check(!island::VegetationLayout::IsReservedRoute(rock.x, rock.z),
                  "land rocks keep authored routes and clearings open");
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
    for (std::size_t index = 0; index < props.FallenLogs().size(); ++index) {
        const island::FallenLogPlacement& log = props.FallenLogs()[index];
        const island::FallenLogPlacement& repeated = repeatedProps.FallenLogs()[index];
        Check(log.x == repeated.x && log.y == repeated.y && log.z == repeated.z
                  && log.yawRadians == repeated.yawRadians && log.length == repeated.length
                  && log.radius == repeated.radius && log.variant == repeated.variant,
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
            Check(!island::VegetationLayout::IsReservedRoute(sampleX, sampleZ),
                  "the complete fallen-log footprint stays outside authored routes");
            Check(terrain.Sample(sampleX, sampleZ).signedShoreDistance >= 30.0f,
                  "the complete fallen-log footprint stays in the island interior");
        }
    }
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
    Check(player.GroundHeight() >= terrain.Height(0.0f, 0.0f),
          "capsule footprint resolves at or above its centre terrain height");
    Check(player.Position().y >= player.GroundHeight() + island::PlayerMotion::EyeHeight - 0.0001f,
          "walking eye never starts below its resolved ground support");
    Check(player.GroundSlopeRadians() >= terrain.Sample(0.0f, 0.0f).slopeRadians,
          "capsule ground slope includes the centre sample");
    player.Update(terrain, 1.0f, 0.0f, 1.0f / 60.0f);
    Check(player.Position().x > start.x, "walking input accelerates the player");
    Check(player.VelocityX() > 0.0f && player.VelocityX() < island::PlayerMotion::WalkSpeed,
          "walking acceleration does not jump to full speed");
    Check(player.Mode() == island::LocomotionMode::Walking,
          "interior movement stays in walking mode");
    CheckNear(player.OceanCurrentStrength(terrain), 0.0f, 0.0001f,
              "ocean current remains inactive while walking");

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

    if (failures != 0) {
        std::cerr << failures << " core test assertion(s) failed\n";
        return EXIT_FAILURE;
    }
    std::cout << "IslandCoreTests: all checks passed\n";
    return EXIT_SUCCESS;
}
