#include "IslandSimulator/AudioAnalysis.hpp"
#include "IslandSimulator/AquaticLightingModel.hpp"
#include "IslandSimulator/BenchmarkTraversalRoute.hpp"
#include "IslandSimulator/BirdFlock.hpp"
#include "IslandSimulator/BirdPerchLayout.hpp"
#include "IslandSimulator/ButterflyMeadow.hpp"
#include "IslandSimulator/GeckoPopulation.hpp"
#include "IslandSimulator/SkinkPopulation.hpp"
#include "IslandSimulator/DragonflySwarm.hpp"
#include "IslandSimulator/FrogPopulation.hpp"
#include "IslandSimulator/HermitCrabGroup.hpp"
#include "IslandSimulator/CoconutCrabGroup.hpp"
#include "IslandSimulator/CrabColony.hpp"
#include "IslandSimulator/CoastalMaterial.hpp"
#include "IslandSimulator/CoastalWindResponse.hpp"
#include "IslandSimulator/IslandEnvironmentField.hpp"
#include "IslandSimulator/ContactShadowPatch.hpp"
#include "IslandSimulator/DebugFreeCamera.hpp"
#include "IslandSimulator/DistantSceneryLayout.hpp"
#include "IslandSimulator/EnvironmentLighting.hpp"
#include "IslandSimulator/FishSchools.hpp"
#include "IslandSimulator/FirstPersonCameraMotion.hpp"
#include "IslandSimulator/FirstPersonInput.hpp"
#include "IslandSimulator/GeneratedTextureBudget.hpp"
#include "IslandSimulator/GrassInteractionModel.hpp"
#include "IslandSimulator/CanopyLightField.hpp"
#include "IslandSimulator/GrassLayout.hpp"
#include "IslandSimulator/GroundCoverField.hpp"
#include "IslandSimulator/PrevailingWind.hpp"
#include "IslandSimulator/SkyCloudDrift.hpp"
#include "IslandSimulator/TrunkSurfaceMaterial.hpp"
#include "IslandSimulator/GrassLod.hpp"
#include "IslandSimulator/IslandTerrain.hpp"
#include "IslandSimulator/JungleLayout.hpp"
#include "IslandSimulator/LocomotionPaceToggle.hpp"
#include "IslandSimulator/MonkeyTroop.hpp"
#include "IslandSimulator/MonkeyTreeReach.hpp"
#include "IslandSimulator/MonkeyAttachmentSite.hpp"
#include "IslandSimulator/MonkeyTreeExcursion.hpp"
#include "IslandSimulator/MonkeyExcursionClearance.hpp"
#include <chrono>
#include "IslandSimulator/NaturalPropLayout.hpp"
#include "IslandSimulator/OceanMeshLayout.hpp"
#include "IslandSimulator/PerformanceStatistics.hpp"
#include "IslandSimulator/PlayerMotion.hpp"
#include "IslandSimulator/SwimDepth.hpp"
#include "IslandSimulator/ProceduralMeshValidation.hpp"
#include "IslandSimulator/RockSurface.hpp"
#include "IslandSimulator/SurfaceNormals.hpp"
#include "IslandSimulator/RuntimeQualityProfile.hpp"
#include "IslandSimulator/SeaTurtleGroup.hpp"
#include "IslandSimulator/SeaTurtleProfile.hpp"
#include "IslandSimulator/SharkPatrol.hpp"
#include "IslandSimulator/ShipwreckLayout.hpp"
#include "IslandSimulator/ShorelineMeshLayout.hpp"
#include "IslandSimulator/SoundscapeModel.hpp"
#include "IslandSimulator/StreamRoute.hpp"
#include "IslandSimulator/StreamSoundField.hpp"
#include "IslandSimulator/TerrainMeshLayout.hpp"
#include "IslandSimulator/TidePoolLayout.hpp"
#include "IslandSimulator/TouchControls.hpp"
#include "IslandSimulator/UserSettings.hpp"
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
#include <regex>
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
    {
        const std::string_view fogShader = island::environment::FogShaderDefinition;
        Check(island::environment::FogColorRed == 0.53f
            && island::environment::FogColorGreen == 0.78f
            && island::environment::FogColorBlue == 0.88f
            && fogShader == "const vec3 islandAtmosphereFog=vec3(0.53,0.78,0.88);\n",
            "CPU and portable shader atmosphere fog share one authored colour");
    }
    {
        island::SwimDepth dive;
        Check(dive.EyeHeight()==.58f,"diving starts at the swimming surface");
        for(int step=0;step<200;++step) dive.Update(-12.0f,false,true,.1f);
        Check(std::abs(dive.EyeHeight()+11.55f)<1e-5f,"descent stops above the seabed");
        Check(dive.SubmergedDepth()>11.5f,"diving records actual submerged depth");
        const float held=dive.EyeHeight();
        dive.Update(-12,true,true,.1f);
        dive.Update(-12,false,false,.1f);
        Check(dive.EyeHeight()==held,"opposed or released keys hold depth");
        Check(!dive.Update(std::numeric_limits<float>::quiet_NaN(),true,false,.1f)
            &&dive.EyeHeight()==held,"invalid seabed rejects motion");
        for(float dt:{-1.0f,std::numeric_limits<float>::infinity(),
                std::numeric_limits<float>::quiet_NaN()}) {
            dive.Update(-12,true,false,dt);
            Check(dive.EyeHeight()==held,"invalid dive time is neutral");
        }
        dive.Update(-12,true,false,1000);
        Check(std::abs(dive.EyeHeight()-held-.16f)<1e-5f,"dive stalls clamp to one bounded step");
        dive.Update(-2,false,false,0);
        Check(std::abs(dive.EyeHeight()+1.55f)<1e-5f,"rising support cannot bury the swimmer");
        for(int step=0;step<200;++step) dive.Update(-12,true,false,.1f);
        Check(dive.EyeHeight()==.58f&&dive.SubmergedDepth()==0,"ascent cannot leave the surface");
        dive.Update(1,false,true,.1f);
        Check(dive.EyeHeight()==.58f,"dry support does not allow diving");
        dive.Update(-12,false,true,.1f); dive.Reset();
        Check(dive.EyeHeight()==.58f,"teleport/reset clears diving depth");
    }
    {
        // Bent, tapered V-fold strip; three shared vertices per level, including a centre ridge.
        const std::vector<std::array<float,3>> strip{
            {-0.03f,0,0}, {0,0,0.014f}, {0.03f,0,0},
            {-0.025f,0.3f,0.04f}, {0,0.3f,0.052f}, {0.025f,0.3f,0.04f},
            {-0.002f,0.55f,0.15f}, {0,0.55f,0.151f}, {0.002f,0.55f,0.15f}};
        const std::vector<std::uint16_t> indices{0,3,1,1,3,4,1,4,2,2,4,5,
                                               3,6,4,4,6,7,4,7,5,5,7,8};
        const auto rest = island::BuildSurfaceNormals(strip,indices);
        Check(std::abs(rest[1][1]-rest[7][1]) > 0.1f,
              "grass normal changes along the bent centreline");
        Check(rest[0][0]*rest[2][0] < 0,
              "grass fold sides have opposing transverse normal components");
        for (float spread : {0.4f,1.0f,1.6f}) {
            for (float stature : {0.3f,0.84f,1.16f,1.5f}) {
                for (float yaw : {-2.4f,0.0f,1.2f}) {
                    const float c=std::cos(yaw), s=std::sin(yaw);
                    const auto rotate = [c,s](std::array<float,3> v) {
                        return std::array<float,3>{c*v[0]-s*v[2],v[1],s*v[0]+c*v[2]};
                    };
                    std::vector<std::array<float,3>> transformed;
                    for (auto p : strip) transformed.push_back(rotate({p[0]*spread,p[1]*stature,p[2]*spread}));
                    const auto reference = island::BuildSurfaceNormals(transformed,indices);
                    for (std::size_t i=0; i<rest.size(); ++i) {
                        auto n=rotate({rest[i][0]/spread,rest[i][1]/stature,rest[i][2]/spread});
                        const float length=std::hypot(n[0],n[1],n[2]);
                        for (int axis=0; axis<3; ++axis) CheckNear(n[axis]/length,reference[i][axis],
                            0.000003f,"scaled grass normals match independently transformed triangles");
                    }
                }
            }
        }
    }
    {
        const std::vector<std::array<float,3>> positions{{0,0,0},{2,0,0},{0,1,0},{0,0,0.5f}};
        const std::vector<std::uint16_t> indices{0,1,2,0,3,1};
        const auto normals = island::BuildSurfaceNormals(positions,indices);
        CheckNear(normals[0][1], 1/std::sqrt(5.0f), 0.000001f,
                  "shared normal weights the smaller triangle by its area");
        CheckNear(normals[0][2], 2/std::sqrt(5.0f), 0.000001f,
                  "shared normal weights the larger triangle by its area");
        const std::vector<std::array<float,3>> split{positions[0],positions[1],positions[2],
                                                   positions[0],positions[3],positions[1]};
        const auto seam = island::BuildSurfaceNormals(split,std::array<std::uint16_t,6>{0,1,2,3,4,5});
        Check(seam[0] == std::array<float,3>{0,0,1} && seam[3] == std::array<float,3>{0,1,0},
              "duplicate cap/material vertices preserve a hard normal seam");
        auto reversed = indices;
        for (std::size_t i=0; i<reversed.size(); i+=3) std::swap(reversed[i+1],reversed[i+2]);
        const auto reversedNormals = island::BuildSurfaceNormals(positions,reversed);
        Check(island::BuildOutwardSurfaceNormals(positions, indices, normals) == normals,
              "outward validation preserves the computed unit normals");
        const std::array<std::array<float, 3>, 6> wedge{{
            {.25f,.025f,-.13f}, {.25f,.025f,.13f}, {.265f,.64f,0},
            {.60f,.018f,-.037f}, {.60f,.018f,.037f}, {.575f,.06f,0}}};
        const std::array<std::array<float, 3>, 6> wedgeReferences{{
            {0,0,-1}, {0,0,1}, {0,1,0}, {0,0,-1}, {0,0,1}, {0,1,0}}};
        std::array<std::uint16_t, 18> wedgeIndices{0,3,2,2,3,5,2,5,1,1,5,4,0,2,1,3,4,5};
        bool inwardWedgeRejected = false;
        try { (void)island::BuildOutwardSurfaceNormals(wedge, wedgeIndices, wedgeReferences); }
        catch (const std::invalid_argument&) { inwardWedgeRejected = true; }
        Check(inwardWedgeRejected, "historical inward root wedge is rejected");
        for (std::size_t i = 0; i < wedgeIndices.size(); i += 3)
            std::swap(wedgeIndices[i+1], wedgeIndices[i+2]);
        Check(island::BuildOutwardSurfaceNormals(wedge, wedgeIndices, wedgeReferences).size() == 6,
              "open tapered root wedge accepts semantic side and crest references");
        for (int invalid = 0; invalid < 5; ++invalid) {
            auto references = normals;
            if (invalid == 0) references.pop_back();
            if (invalid == 1) references[0] = {0, 0, 0};
            if (invalid == 2) references[0][0] = std::numeric_limits<float>::quiet_NaN();
            if (invalid == 3) references[0] = {1, 0, 0}; // Tangent is not outward.
            bool rejected = false;
            try { (void)island::BuildOutwardSurfaceNormals(positions, invalid == 4 ? reversed : indices, references); }
            catch (const std::invalid_argument&) { rejected = true; }
            Check(rejected, "outward normals reject missing, invalid, tangent and inward references");
        }
        for (std::size_t i=0; i<normals.size(); ++i) {
            CheckNear(std::hypot(normals[i][0],normals[i][1],normals[i][2]),1,0.000001f,
                      "surface normals are finite and unit length");
            for (int axis=0; axis<3; ++axis) CheckNear(reversedNormals[i][axis],-normals[i][axis],
                0.000001f,"normal direction follows triangle winding");
        }
        for (int invalid=0; invalid<7; ++invalid) {
            auto p=positions;
            auto i=indices;
            if (invalid==0) p.clear();
            if (invalid==1) i.pop_back();
            if (invalid==2) i[0]=99;
            if (invalid==3) p[0][0]=std::numeric_limits<float>::infinity();
            if (invalid==4) i[1]=i[0];
            if (invalid==5) p.push_back({3,3,3});
            if (invalid==6) i.insert(i.end(),reversed.begin(),reversed.end());
            bool rejected=false;
            try { (void)island::BuildSurfaceNormals(p,i); }
            catch (const std::invalid_argument&) { rejected=true; }
            Check(rejected,"invalid or undefined normal input is rejected before upload");
        }
    }
    for (int variant = 0; variant < 4; ++variant) {
        for (bool farMesh : {false, true}) {
            const auto rock = island::BuildRockSurface(variant, farMesh);
            std::vector<std::array<float,3>> normalPositions;
            for (const auto& vertex : rock.vertices) normalPositions.push_back(vertex.position);
            const auto sharedNormals = island::BuildSurfaceNormals(normalPositions,rock.indices);
            Check(rock.vertices.size() == (farMesh ? 36U : 265U), "rock LOD has a fixed bounded vertex count");
            Check(rock.indices.size() == (farMesh ? 189U : 1512U), "rock LOD has a fixed bounded index count");
            island::ValidateProceduralTriangleMeshOrThrow(rock.vertices.size(), rock.indices, "rock CPU test");
            std::vector<std::array<double,3>> expected(rock.vertices.size());
            for (std::size_t i = 0; i < rock.indices.size(); i += 3) {
                const auto& a = rock.vertices[rock.indices[i]].position;
                const auto& b = rock.vertices[rock.indices[i+1]].position;
                const auto& c = rock.vertices[rock.indices[i+2]].position;
                const double ux=b[0]-a[0], uy=b[1]-a[1], uz=b[2]-a[2];
                const double vx=c[0]-a[0], vy=c[1]-a[1], vz=c[2]-a[2];
                const std::array n{uy*vz-uz*vy,uz*vx-ux*vz,ux*vy-uy*vx};
                Check(std::hypot(n[0],n[1],n[2]) > 0.00001, "rock triangle is not degenerate");
                for (int corner = 0; corner < 3; ++corner) {
                    for (int axis = 0; axis < 3; ++axis) expected[rock.indices[i+corner]][axis] += n[axis];
                }
            }
            for (std::size_t i = 0; i < rock.vertices.size(); ++i) {
                const auto& p = rock.vertices[i].position;
                const auto& n = rock.vertices[i].normal;
                for (int axis=0; axis<3; ++axis) CheckNear(sharedNormals[i][axis],n[axis],0.000001f,
                    "wood normal builder matches independently generated rock normals");
                Check(std::hypot(p[0],p[2]) <= 1 && p[1] >= 0 && p[1] <= island::RockSurfaceMaximumHeight,
                      "refined rock fits existing collision/culling extents");
                CheckNear(std::hypot(n[0],n[1],n[2]), 1, 0.000001f, "rock normals are finite and unit length");
                const auto& e = expected[i];
                for (int axis=0; axis<3; ++axis) CheckNear(n[axis], e[axis]/std::hypot(e[0],e[1],e[2]),
                    0.000001f, "shared normals match independent area-weighted face accumulation");
                Check(n[0]*p[0] + n[2]*p[2] + n[1]*std::max(0.0f,p[1]-0.6f) > 0,
                      "rock normals face away from the interior");
                for (float yaw : {-2.3f, 0.0f, 1.7f}) {
                    const std::array<double,3> scale{0.4, 3.2, 1.7};
                    const std::array<double,3> tangent{n[1], -n[0], 0};
                    const auto rotate = [yaw](const std::array<double,3>& v) {
                        return std::array{std::cos(yaw)*v[0]+std::sin(yaw)*v[2], v[1],
                                          -std::sin(yaw)*v[0]+std::cos(yaw)*v[2]};
                    };
                    const auto transformedNormal = rotate({n[0]/scale[0],n[1]/scale[1],n[2]/scale[2]});
                    const auto transformedTangent = rotate({tangent[0]*scale[0],tangent[1]*scale[1],tangent[2]*scale[2]});
                    double dot=0;
                    for (int axis=0; axis<3; ++axis) dot += transformedNormal[axis]*transformedTangent[axis];
                    Check(std::abs(dot) < 0.000001, "inverse-scale yaw normal remains perpendicular to transformed surface");
                }
            }
        }
    }
    for (int invalid : {-1,4}) {
        bool rejected=false;
        try { (void)island::BuildRockSurface(invalid,false); }
        catch (const std::invalid_argument&) { rejected=true; }
        Check(rejected,"invalid rock variant is rejected before upload");
    }
    for (float knot : {-22.0f, -3.0f, 0.0f, 1.4f}) {
        const auto lower = island::CoastalSandColor(knot - 0.00001f);
        const auto upper = island::CoastalSandColor(knot + 0.00001f);
        for (std::size_t channel = 0; channel < lower.size(); ++channel) {
            CheckNear(lower[channel], upper[channel], 0.001f,
                      "coastal sand palette has no discontinuity at its depth knots");
        }
    }
    float previousOpacity = 0, previousLagoon = 1, previousCoverage = 0;
    float previousReflection = -1;
    for (int step=0; step<=10000; ++step) {
        const float slope = (static_cast<float>(step)/10000-0.5f)*0.42f;
        const float response = island::CoastalSurfaceAppearance::ReflectionModulation(slope);
        Check(response >= previousReflection && std::abs(response) < 0.54f,
              "supplementary grazing response is monotonic and restrained over the wave slope bound");
        CheckNear(response,-island::CoastalSurfaceAppearance::ReflectionModulation(-slope),0.000001f,
                  "grazing response has no brightness bias");
        previousReflection=response;
    }
    for (float shore : {-20.0f,-4.0f,0.0f,4.0f,20.0f}) {
        float previousFoam=1;
        for (int step=1; step<=3000; ++step) {
            const float depth=step*0.001f;
            const float coverage=island::CoastalSurfaceAppearance::BreakingCoverage(depth,shore);
            Check(coverage >= 0 && coverage <= previousFoam,
                  "calm breaker coverage decreases with depth");
            if (depth >= 1.25f) Check(coverage == 0,"deep water does not generate broad breaker foam");
            CheckNear(coverage,island::CoastalSurfaceAppearance::BreakingCoverage(depth,-shore),0.000001f,
                      "breaker envelope is symmetric about the shoreline");
            previousFoam=coverage;
        }
    }
    Check(island::CoastalSurfaceAppearance::BreakingCoverage(0,0)==0
              && island::CoastalSurfaceAppearance::BreakingCoverage(-1,0)==0
              && island::CoastalSurfaceAppearance::BreakingCoverage(0.1f,0)==1
              && island::CoastalSurfaceAppearance::BreakingCoverage(0.1f,20)<0.007f,
          "breaker foam remains in a bounded wet near-shore region");
    for (int step = 0; step <= 10000; ++step) {
        const float depth = static_cast<float>(step) * 0.003f;
        const float opacity = island::CoastalWaterOptics::BodyOpacity(depth);
        const float lagoon = island::CoastalWaterOptics::LagoonWeight(depth);
        const float coverage = island::CoastalWaterOptics::ContactCoverage(depth);
        Check(opacity >= previousOpacity && opacity <= 0.92f
                  && lagoon <= previousLagoon && lagoon >= 0
                  && coverage >= previousCoverage && coverage <= 1,
              "coastal attenuation, colour and contact coverage are bounded and monotonic");
        previousOpacity = opacity;
        previousLagoon = lagoon;
        previousCoverage = coverage;
        for (float channel : island::CoastalSandColor(-depth)) {
            Check(std::isfinite(channel) && channel >= 0 && channel <= 255,
                  "coastal sand colour stays in byte range");
        }
    }
    Check(island::CoastalWaterOptics::BodyOpacity(0) == 0
              && island::CoastalWaterOptics::ContactCoverage(-1) == 0
              && island::CoastalWaterOptics::ContactCoverage(0.12f) == 1
              && island::CoastalWaterOptics::BodyOpacity(0.1f) < 0.04f
              && island::CoastalWaterOptics::BodyOpacity(10) > 0.9f,
          "thin water reveals sand while deep water attenuates the bottom");
    Check(island::ReefMacroAppearance::LagoonWeight(20, 1.5f) > .98f
              && island::ReefMacroAppearance::LagoonWeight(55, 1.5f) == 0
              && island::ReefMacroAppearance::CrestWeight(74, 3) > .98f
              && island::ReefMacroAppearance::CrestWeight(40, 3) == 0
              && island::ReefMacroAppearance::DropoffWeight(110, 9) == 1
              && island::ReefMacroAppearance::DropoffWeight(60, 9) == 0,
          "reef macro zones separate lagoon, crest, and deep shelf drop-off");
    float maximumReefBreaker = 0;
    float minimumReefBreaker = 1;
    for (int sample = 0; sample < 360; ++sample) {
        const float angle = static_cast<float>(sample)
            * std::numbers::pi_v<float> / 180.0f;
        const float breaker = island::ReefMacroAppearance::BreakerWeight(
            std::cos(angle) * 320.0f, std::sin(angle) * 320.0f, 74.0f, 3.0f);
        maximumReefBreaker = std::max(maximumReefBreaker, breaker);
        minimumReefBreaker = std::min(minimumReefBreaker, breaker);
    }
    Check(maximumReefBreaker > .98f && minimumReefBreaker == 0,
          "reef breaker envelope forms separated angular patches instead of a white ring");
    Check(island::ReefMacroAppearance::BreakerWeight(320, 0, 40, 3) == 0
              && island::ReefMacroAppearance::BreakerWeight(320, 0, 95, 3) == 0,
          "reef breakers remain confined to the shared 62-86 metre crest");
    for (float invalid : {std::numeric_limits<float>::quiet_NaN(),
                           std::numeric_limits<float>::infinity()}) {
        Check(island::CoastalSurfaceAppearance::ReflectionModulation(invalid)==0
                  && island::CoastalSurfaceAppearance::BreakingCoverage(invalid,0)==0
                  && island::CoastalSurfaceAppearance::BreakingCoverage(1,invalid)==0,
              "invalid surface appearance input contributes no artificial contrast");
        Check(std::isfinite(island::CoastalWaterOptics::BodyOpacity(invalid))
                  && island::CoastalWaterOptics::ContactCoverage(invalid) == 0
                  && island::CoastalWaterOptics::LagoonWeight(invalid) == 0,
              "invalid optical inputs produce bounded conservative values");
        Check(island::CoastalSandColor(invalid) == island::CoastalSandColor(-22),
              "invalid coastal height uses the finite deep palette");
        Check(island::ReefMacroAppearance::LagoonWeight(invalid, 1) == 0
                  && island::ReefMacroAppearance::CrestWeight(72, invalid) == 0
                  && island::ReefMacroAppearance::DropoffWeight(invalid, invalid) == 0
                  && island::ReefMacroAppearance::BreakerWeight(invalid, 0, 74, 3) == 0,
              "invalid reef macro inputs contribute no seabed contrast");
    }
    const island::UserSettings defaultSettings;
    Check(defaultSettings.IsValid(), "default user settings are valid");
    Check(defaultSettings.ToText()
              == "schema=1\nrun_mode=walk\ncamera_motion=enabled\n"
                 "audio=enabled\ntouch_look=standard\n",
          "default user settings use a stable human-readable representation");
    const island::UserSettingsParseResult parsedSettings = island::UserSettings::Parse(
        "# portable preferences\n"
        "schema = 1\nunknown_future_key = retained_elsewhere\n"
        "touch_look=high\naudio=muted\ncamera_motion=disabled\nrun_mode=run\n");
    Check(parsedSettings.IsValid()
              && parsedSettings.settings.runningSelected
              && !parsedSettings.settings.cameraMotionEnabled
              && parsedSettings.settings.audioMuted
              && parsedSettings.settings.touchLookSensitivity
                  == island::TouchLookSensitivityLevel::High,
          "settings parser accepts reordered values and ignores forward-compatible keys");
    Check(island::UserSettings::Parse(parsedSettings.settings.ToText()).settings
              == parsedSettings.settings,
          "settings serialization round-trips every current preference");
    Check(!island::UserSettings::Parse("run_mode=run\n").IsValid(),
          "settings parser requires an explicit schema");
    Check(!island::UserSettings::Parse("schema=2\n").IsValid(),
          "settings parser rejects unsupported schemas");
    Check(!island::UserSettings::Parse(
              "schema=1\nrun_mode=walk\ncamera_motion=enabled\naudio=enabled\n").IsValid(),
          "settings parser rejects truncated records with missing required keys");
    Check(!island::UserSettings::Parse(
              "schema=1\nrun_mode=walk\nrun_mode=run\n").IsValid(),
          "settings parser rejects duplicate known keys");
    Check(!island::UserSettings::Parse("schema=1\naudio=loud\n").IsValid(),
          "settings parser rejects invalid preference values");
    Check(!island::UserSettings::Parse(std::string(
              island::UserSettings::MaximumSerializedBytes + 1U, 'x')).IsValid(),
          "settings parser rejects oversized input before scanning it");
    std::string settingsWithNul = "schema=1\n";
    settingsWithNul.push_back('\0');
    Check(!island::UserSettings::Parse(settingsWithNul).IsValid(),
          "settings parser rejects embedded NUL bytes");

    Check(island::GeneratedTextureBudget::TotalPixelCount == 1195811U,
          "generated texture inventory retains its exact base-level pixel count");
    Check(island::GeneratedTextureBudget::TotalRgba8Bytes == 4783244U,
          "generated texture inventory stays below its audited RGBA8 payload");
    constexpr island::RuntimeQualityProfile fullQuality =
        island::RuntimeQualityProfile::Full();
    constexpr island::RuntimeQualityProfile mobileQuality =
        island::RuntimeQualityProfile::Mobile();
    constexpr island::RuntimeQualityProfile nativeQuality =
        island::RuntimeQualityProfile::NativePerformance();
    Check(fullQuality.IsValid() && mobileQuality.IsValid() && nativeQuality.IsValid(),
          "runtime quality profiles expose valid renderer settings");
    Check(fullQuality.name == "full" && mobileQuality.name == "mobile"
              && nativeQuality.name == "native-performance",
          "runtime quality profiles retain stable diagnostic names");
    CheckNear(fullQuality.grassMaximumDistance, 135.0f, 0.0f,
              "full quality retains the authored grass distance");
    CheckNear(mobileQuality.grassMaximumDistance, 112.0f, 0.0f,
              "mobile quality reduces only distant grass work");
    CheckNear(nativeQuality.grassMaximumDistance, 72.0f, 0.0f,
              "native performance quality strongly bounds distant grass work");
    Check(nativeQuality.vegetationMaximumDistanceScale
              < mobileQuality.vegetationMaximumDistanceScale,
          "native performance quality bounds elevated-view vegetation");
    Check(mobileQuality.contactShadowDistanceScale < fullQuality.contactShadowDistanceScale
              && mobileQuality.wildlifeMaximumDistanceScale
                  < fullQuality.wildlifeMaximumDistanceScale,
          "mobile quality bounds distant shadows and wildlife");
    Check(island::GeneratedTextureBudget::TotalRuntimeRgba8Bytes == 10091644U,
          "runtime texture budget must include floor, leaf, wildlife, hut, cave, resident, helicopter and bear mip chains");
    Check(mobileQuality.textureByteBudget
              >= island::GeneratedTextureBudget::TotalRuntimeRgba8Bytes,
          "mobile quality admits the compact generated texture inventory");
    Check(mobileQuality.maximumPlayingAudioSources == 6,
          "mobile quality preserves the bounded full soundscape");
    Check(fullQuality.preferredBackBufferSamples == 4 && mobileQuality.preferredBackBufferSamples == 2,
          "portable antialiasing requests bound full and mobile sample counts");
    Check(nativeQuality.preferredBackBufferSamples == 0
              && nativeQuality.oceanQuadsPerAxis == 96,
          "native performance quality removes multisampling and reduces the ocean grid");
    for (const int samples : {-1, 1, 3, 8, 1024}) {
        auto invalid = fullQuality;
        invalid.preferredBackBufferSamples = samples;
        Check(!invalid.IsValid(), "quality policy rejects unsupported or unbounded sample requests");
    }
    auto noAntialiasing = fullQuality;
    noAntialiasing.preferredBackBufferSamples = 0;
    Check(noAntialiasing.IsValid(), "quality policy admits explicit single-sample fallback");

    const island::AquaticLightProfile shallowLight = island::AquaticLightAt(
        7.0f, -0.35f, -4.0f, 2.0f);
    const island::AquaticLightProfile middleLight = island::AquaticLightAt(
        7.0f, -4.0f, -4.0f, 2.0f);
    const island::AquaticLightProfile deepLight = island::AquaticLightAt(
        7.0f, -8.0f, -4.0f, 2.0f);
    CheckNear(shallowLight.waterTint, 0.04f, 0.0001f,
              "shallow aquatic lighting retains only the minimum water tint");
    Check(middleLight.waterTint > shallowLight.waterTint
              && deepLight.waterTint > middleLight.waterTint,
          "aquatic water tint increases smoothly with depth");
    CheckNear(deepLight.waterTint, 0.26f, 0.0001f,
              "deep aquatic lighting reaches its bounded water tint");
    CheckNear(deepLight.brightness, 0.88f, 0.0001f,
              "deep aquatic lighting suppresses shallow sunlight bands");
    const island::AquaticLightProfile movingShallowLight = island::AquaticLightAt(
        7.0f, -0.35f, -4.0f, 8.0f);
    Check(std::abs(movingShallowLight.brightness - shallowLight.brightness) > 0.001f,
          "shallow aquatic sunlight changes over time");
    Check(shallowLight.brightness >= 0.88f && shallowLight.brightness <= 1.12f
              && middleLight.brightness >= 0.88f && middleLight.brightness <= 1.12f,
          "aquatic sunlight brightness remains inside its renderer colour budget");
    const island::AquaticLightProfile invalidLight = island::AquaticLightAt(
        std::numeric_limits<float>::quiet_NaN(), -1.0f, 0.0f, 1.0f);
    CheckNear(invalidLight.waterTint, 0.0f, 0.0f,
              "non-finite aquatic position fails closed to neutral tint");
    CheckNear(invalidLight.brightness, 1.0f, 0.0f,
              "non-finite aquatic position fails closed to neutral brightness");
    const island::AquaticLightProfile invalidTimeLight = island::AquaticLightAt(
        0.0f, -1.0f, 0.0f, std::numeric_limits<float>::infinity());
    CheckNear(invalidTimeLight.waterTint, 0.0f, 0.0f,
              "non-finite aquatic clock fails closed to neutral tint");
    CheckNear(invalidTimeLight.brightness, 1.0f, 0.0f,
              "non-finite aquatic clock fails closed to neutral brightness");

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

    constexpr std::array<std::uint16_t, 3> validTriangle = {0U, 1U, 2U};
    bool validProceduralMeshAccepted = true;
    try {
        island::ValidateProceduralTriangleMeshOrThrow(
            3U, validTriangle, "test-valid");
        island::ValidateProceduralTriangleMeshOrThrow(
            65'536U, std::array<std::uint16_t, 3>{65'535U, 1U, 0U},
            "test-maximum");
    } catch (const std::runtime_error&) {
        validProceduralMeshAccepted = false;
    }
    Check(validProceduralMeshAccepted,
          "procedural mesh validation accepts complete in-range 16-bit triangle lists");

    const auto rejectsProceduralMesh = [](const std::size_t vertexCount,
                                           const std::span<const std::uint16_t> indices,
                                           const std::string_view expectedDiagnostic) {
        try {
            island::ValidateProceduralTriangleMeshOrThrow(
                vertexCount, indices, "test-invalid");
        } catch (const std::runtime_error& exception) {
            return std::string_view(exception.what()).find(expectedDiagnostic)
                != std::string_view::npos;
        }
        return false;
    };
    Check(rejectsProceduralMesh(0U, validTriangle, "no vertices")
              && rejectsProceduralMesh(3U, {}, "no triangle indices")
              && rejectsProceduralMesh(
                  3U, std::array<std::uint16_t, 2>{0U, 1U}, "complete triangle list")
              && rejectsProceduralMesh(
                  3U, std::array<std::uint16_t, 3>{0U, 1U, 3U}, "out-of-range")
              && rejectsProceduralMesh(65'537U, validTriangle, "16-bit vertex limit"),
          "procedural mesh validation rejects every malformed portable index boundary");

    bool validInstanceRangesAccepted = true;
    try {
        island::ValidateInstancedDrawRangeOrThrow(8, 0, 8, "test-full-instance");
        island::ValidateInstancedDrawRangeOrThrow(8, 3, 5, "test-sliced-instance");
    } catch (const std::runtime_error&) {
        validInstanceRangesAccepted = false;
    }
    Check(validInstanceRangesAccepted,
          "instanced draw validation accepts complete and sliced in-range buffers");

    const auto rejectsInstanceRange = [](const int capacity, const int offset,
                                         const int count,
                                         const std::string_view expectedDiagnostic) {
        try {
            island::ValidateInstancedDrawRangeOrThrow(
                capacity, offset, count, "test-invalid-instance");
        } catch (const std::runtime_error& exception) {
            return std::string_view(exception.what()).find(expectedDiagnostic)
                != std::string_view::npos;
        }
        return false;
    };
    Check(rejectsInstanceRange(-1, 0, 1, "negative instance-buffer capacity")
              && rejectsInstanceRange(8, -1, 1, "negative instance-buffer offset")
              && rejectsInstanceRange(8, 0, 0, "non-positive instance count")
              && rejectsInstanceRange(8, 7, 2, "beyond the instance buffer")
              && rejectsInstanceRange(8, 9, 1, "beyond the instance buffer"),
          "instanced draw validation rejects every out-of-buffer range shape");

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
    Check(frameSummary.framesOver120HzBudget == 5U,
          "frame-time statistics count samples outside a 120 Hz budget");
    Check(frameSummary.framesOver60HzBudget == 4U,
          "frame-time statistics count samples outside a 60 Hz budget");
    Check(frameSummary.framesOver30HzBudget == 2U,
          "frame-time statistics count samples outside a 30 Hz budget");
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

    island::LocomotionPaceToggle paceToggle;
    Check(!paceToggle.IsRunningSelected(),
          "locomotion pace begins in walking mode");
    Check(paceToggle.Update(true) && paceToggle.IsRunningSelected(),
          "first Shift edge selects persistent running mode");
    Check(!paceToggle.Update(true) && paceToggle.IsRunningSelected(),
          "holding Shift does not repeatedly toggle the selected pace");
    Check(!paceToggle.Update(false) && paceToggle.IsRunningSelected(),
          "releasing Shift retains the selected running mode");
    Check(paceToggle.Update(true) && !paceToggle.IsRunningSelected(),
          "second Shift edge restores persistent walking mode");
    Check(!paceToggle.Update(false) && !paceToggle.IsRunningSelected(),
          "walking mode remains selected after the second release");
    Check(!paceToggle.Update(true, false) && !paceToggle.IsRunningSelected(),
          "debug free-camera Shift does not change player pace");
    Check(!paceToggle.Update(true, true) && !paceToggle.IsRunningSelected(),
          "a held debug-camera Shift cannot leak into player pace selection");
    Check(!paceToggle.Update(false, true) && !paceToggle.IsRunningSelected(),
          "releasing the debug-camera Shift rearms the player toggle");
    Check(paceToggle.Update(true, true) && paceToggle.IsRunningSelected(),
          "a fresh Shift edge works after leaving the debug camera");
    island::LocomotionPaceToggle restoredPaceToggle;
    restoredPaceToggle.SetRunningSelected(true);
    Check(restoredPaceToggle.IsRunningSelected()
              && !restoredPaceToggle.Update(false),
          "persisted run selection restores without synthesizing a Shift edge");

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

    island::WaterRippleModel waterSplash;
    waterSplash.Reset(false, 0.0f, 0.0f);
    (void)waterSplash.Update(false, -0.10f, 3.0f, 1.0f / 60.0f);
    const float entrySplashIntensity = waterSplash.SplashIntensity();
    Check(entrySplashIntensity > 0.70f && entrySplashIntensity <= 1.0f,
          "moving across the wet threshold starts a bounded entry burst");
    Check(waterSplash.SplashPhase() > 0.0f && waterSplash.SplashPhase() < 0.05f,
          "entry burst begins with a smoothly advancing normalized phase");
    const float hysteresisPhase = waterSplash.SplashPhase();
    (void)waterSplash.Update(false, -0.04f, 3.0f, 1.0f / 60.0f);
    Check(waterSplash.SplashPhase() > hysteresisPhase,
          "water-depth hysteresis does not retrigger a burst in the transition band");
    const float cooldownPhase = waterSplash.SplashPhase();
    (void)waterSplash.Update(false, 0.0f, 3.0f, 1.0f / 60.0f);
    Check(waterSplash.SplashPhase() > cooldownPhase,
          "an opposite crossing during cooldown cannot restart the active burst");
    for (int frame = 0; frame < 120; ++frame) {
        (void)waterSplash.Update(false, -0.10f, 0.0f, 1.0f / 60.0f);
    }
    CheckNear(waterSplash.SplashIntensity(), 0.0f, 0.000001f,
              "shoreline burst retires completely after its bounded lifetime");
    (void)waterSplash.Update(false, 0.0f, 2.0f, 1.0f / 60.0f);
    Check(waterSplash.SplashIntensity() > 0.0f
              && waterSplash.SplashIntensity() < entrySplashIntensity,
          "leaving the water starts a gentler exit burst after cooldown");
    waterSplash.Reset(false, 0.0f, 0.0f);
    (void)waterSplash.Update(false, -0.10f,
                             std::numeric_limits<float>::quiet_NaN(), 1.0f / 60.0f);
    CheckNear(waterSplash.SplashIntensity(), 0.0f, 0.000001f,
              "non-finite crossing input cannot start a surface burst");

    const island::AmbientLoop surfLoop = island::GenerateSurfLoop();
    const island::AmbientLoop repeatedSurfLoop = island::GenerateSurfLoop();
    const island::AmbientLoop breezeLoop = island::GenerateBreezeLoop();
    const island::AmbientLoop jungleLoop = island::GenerateJungleLoop();
    const island::AmbientLoop repeatedJungleLoop = island::GenerateJungleLoop();
    const island::AmbientLoop streamLoop = island::GenerateStreamLoop();
    const island::AmbientLoop repeatedStreamLoop = island::GenerateStreamLoop();
    const island::AmbientLoop waveAccent = island::GenerateWaveAccent();
    const island::AmbientLoop waveAccentAlternate = island::GenerateWaveAccentAlternate();
    const island::AmbientLoop shipwreckCreak = island::GenerateShipwreckCreak();
    const island::AmbientLoop shipwreckCreakAlternate =
        island::GenerateShipwreckCreakAlternate();
    const island::AmbientLoop repeatedShipwreckCreak = island::GenerateShipwreckCreak();
    const island::AmbientLoop waterMovementLoop = island::GenerateWaterMovementLoop();
    const island::AmbientLoop repeatedWaterMovementLoop = island::GenerateWaterMovementLoop();
    const island::AmbientLoop waterTransitionAccent = island::GenerateWaterTransitionAccent();
    const island::AmbientLoop waterTransitionAccentAlternate =
        island::GenerateWaterTransitionAccentAlternate();
    const island::AmbientLoop sandFootstep = island::GenerateSandFootstep();
    const island::AmbientLoop sandFootstepAlternate =
        island::GenerateSandFootstepAlternate();
    const island::AmbientLoop soilFootstep = island::GenerateSoilFootstep();
    const island::AmbientLoop soilFootstepAlternate =
        island::GenerateSoilFootstepAlternate();
    const island::AmbientLoop birdCall = island::GenerateBirdCall();
    const island::AmbientLoop birdCallAlternate = island::GenerateBirdCallAlternate();
    const island::AmbientLoop parrotCall = island::GenerateParrotCall();
    const island::AmbientLoop parrotCallAlternate = island::GenerateParrotCallAlternate();
    const island::AmbientLoop monkeyCall = island::GenerateMonkeyCall();
    const island::AmbientLoop monkeyCallAlternate = island::GenerateMonkeyCallAlternate();
    const island::AmbientLoop frogCall = island::GenerateFrogCall(0U);
    const island::AmbientLoop frogCallAlternate = island::GenerateFrogCall(1U);
    const island::AmbientLoop repeatedBirdCall = island::GenerateBirdCall();
    const island::AmbientLoop repeatedBirdCallAlternate = island::GenerateBirdCallAlternate();
    const island::AmbientLoop repeatedParrotCall = island::GenerateParrotCall();
    const island::AmbientLoop repeatedParrotCallAlternate = island::GenerateParrotCallAlternate();
    const island::AmbientLoop repeatedMonkeyCall = island::GenerateMonkeyCall();
    const island::AmbientLoop repeatedMonkeyCallAlternate = island::GenerateMonkeyCallAlternate();
    const island::AmbientLoop repeatedFrogCall = island::GenerateFrogCall(0U);
    const island::AmbientLoop cicadaCall = island::GenerateCicadaCall(0U);
    const island::AmbientLoop cicadaCallAlternate = island::GenerateCicadaCall(1U);
    const island::AmbientLoop repeatedCicadaCall = island::GenerateCicadaCall(0U);
    Check(surfLoop.sampleRate == 22050 && breezeLoop.sampleRate == 22050,
          "procedural ambience uses the intended portable sample rate");
    Check(frogCall.sampleRate==22050&&frogCall.samples.size()==24255U
              &&frogCallAlternate.sampleRate==22050&&frogCallAlternate.samples.size()==24255U
              &&frogCall.samples!=frogCallAlternate.samples
              &&frogCall.samples==repeatedFrogCall.samples
              &&frogCall.samples.front()==0&&frogCall.samples.back()==0
              &&frogCallAlternate.samples.front()==0&&frogCallAlternate.samples.back()==0,
          "frog croaks are deterministic, distinct and have bounded silent endpoints");
    Check(cicadaCall.sampleRate==22050&&cicadaCall.samples.size()==30870U
              &&cicadaCallAlternate.sampleRate==22050
              &&cicadaCallAlternate.samples.size()==30870U
              &&cicadaCall.samples!=cicadaCallAlternate.samples
              &&cicadaCall.samples==repeatedCicadaCall.samples
              &&cicadaCall.samples.front()==0&&cicadaCall.samples.back()==0
              &&cicadaCallAlternate.samples.front()==0&&cicadaCallAlternate.samples.back()==0,
          "cicada buzzes are deterministic, distinct and have bounded silent endpoints");
    Check(surfLoop.samples.size() == 220500U && breezeLoop.samples.size() == 242550U,
          "procedural ambient loops have their intended staggered portable lengths");
    Check(jungleLoop.sampleRate == 22050 && jungleLoop.samples.size() == 639450U,
          "daytime-jungle ambience has its intended twenty-nine-second portable format");
    Check(streamLoop.sampleRate == 22050 && streamLoop.samples.size() == 286650U,
          "stream ambience has its intended thirteen-second portable format");
    Check(waveAccent.sampleRate == 22050 && waveAccent.samples.size() == 88200U
              && waveAccentAlternate.sampleRate == 22050
              && waveAccentAlternate.samples.size() == 88200U,
          "both stronger-wave accents have the intended four-second portable format");
    Check(shipwreckCreak.sampleRate == 22050
              && shipwreckCreak.samples.size() == 39690U
              && shipwreckCreakAlternate.sampleRate == 22050
              && shipwreckCreakAlternate.samples.size() == 39690U,
          "both shipwreck creaks have the intended 1.8-second portable format");
    Check(waterMovementLoop.sampleRate == 22050
              && waterMovementLoop.samples.size() == 154350U,
          "player water movement has its intended seven-second portable format");
    const std::array<std::size_t, 5> persistentLoopSeconds = {
        surfLoop.samples.size() / 22050U,
        breezeLoop.samples.size() / 22050U,
        jungleLoop.samples.size() / 22050U,
        waterMovementLoop.samples.size() / 22050U,
        streamLoop.samples.size() / 22050U,
    };
    const std::size_t combinedLoopPeriod = std::accumulate(
        persistentLoopSeconds.begin(), persistentLoopSeconds.end(), std::size_t{1U},
        [](const std::size_t left, const std::size_t right) {
            return std::lcm(left, right);
        });
    Check(combinedLoopPeriod == 290290U,
          "persistent ambience avoids complete phase realignment for over eighty hours");
    Check(waterTransitionAccent.sampleRate == 22050
              && waterTransitionAccent.samples.size() == 33075U
              && waterTransitionAccentAlternate.sampleRate == 22050
              && waterTransitionAccentAlternate.samples.size() == 33075U,
          "both water transitions have the intended one-and-a-half-second portable format");
    Check(sandFootstep.sampleRate == 22050 && sandFootstep.samples.size() == 6615U
              && sandFootstepAlternate.sampleRate == 22050
              && sandFootstepAlternate.samples.size() == 6615U
              && soilFootstep.sampleRate == 22050 && soilFootstep.samples.size() == 6615U
              && soilFootstepAlternate.sampleRate == 22050
              && soilFootstepAlternate.samples.size() == 6615U,
          "all land footsteps have the intended 300-millisecond portable format");
    Check(birdCall.sampleRate == 22050 && birdCall.samples.size() == 26460U
              && birdCallAlternate.sampleRate == 22050
              && birdCallAlternate.samples.size() == 29767U,
          "both bird phrases have their intended short portable formats");
    Check(parrotCall.sampleRate == 22050 && parrotCall.samples.size() == 31972U
              && parrotCallAlternate.sampleRate == 22050
              && parrotCallAlternate.samples.size() == 35280U,
          "both parrot squawks have their intended short portable formats");
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
    Check(streamLoop.samples == repeatedStreamLoop.samples,
          "procedural stream generation is deterministic");
    Check(waveAccent.samples != waveAccentAlternate.samples,
          "stronger-wave variants contain genuinely different source PCM");
    Check(shipwreckCreak.samples != shipwreckCreakAlternate.samples
              && shipwreckCreak.samples == repeatedShipwreckCreak.samples,
          "shipwreck creaks are distinct variants and regenerate deterministically");
    Check(waterTransitionAccent.samples != waterTransitionAccentAlternate.samples,
          "water-transition variants contain genuinely different source PCM");
    Check(sandFootstep.samples != sandFootstepAlternate.samples
              && soilFootstep.samples != soilFootstepAlternate.samples
              && sandFootstep.samples != soilFootstep.samples,
          "footstep variants and terrain surfaces contain distinct source PCM");
    Check(birdCall.samples != birdCallAlternate.samples
              && parrotCall.samples != parrotCallAlternate.samples
              && parrotCall.samples != birdCall.samples
              && monkeyCall.samples != monkeyCallAlternate.samples,
          "wildlife variants contain genuinely different source PCM");
    Check(birdCall.samples == repeatedBirdCall.samples
              && birdCallAlternate.samples == repeatedBirdCallAlternate.samples
              && parrotCall.samples == repeatedParrotCall.samples
              && parrotCallAlternate.samples == repeatedParrotCallAlternate.samples
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
    Check(peakMagnitude(streamLoop) < 32767 && rootMeanSquare(streamLoop) > 700.0,
          "stream ambience is audible while retaining mixing headroom");
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
    Check(peakMagnitude(sandFootstep) < 32767 && rootMeanSquare(sandFootstep) > 700.0
              && peakMagnitude(sandFootstepAlternate) < 32767
              && rootMeanSquare(sandFootstepAlternate) > 700.0
              && peakMagnitude(soilFootstep) < 32767
              && rootMeanSquare(soilFootstep) > 700.0
              && peakMagnitude(soilFootstepAlternate) < 32767
              && rootMeanSquare(soilFootstepAlternate) > 700.0,
          "land footsteps are audible while retaining mixing headroom");
    Check(sandFootstep.samples.front() == 0 && sandFootstep.samples.back() == 0
              && sandFootstepAlternate.samples.front() == 0
              && sandFootstepAlternate.samples.back() == 0
              && soilFootstep.samples.front() == 0 && soilFootstep.samples.back() == 0
              && soilFootstepAlternate.samples.front() == 0
              && soilFootstepAlternate.samples.back() == 0,
          "all land footsteps begin and end at exact silence");
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
    Check(std::abs(static_cast<int>(streamLoop.samples.front())
                   - static_cast<int>(streamLoop.samples.back())) < 8000,
          "stream loop has no large boundary discontinuity");
    for (const island::AmbientLoop* loop : {
             &surfLoop, &breezeLoop, &jungleLoop, &waterMovementLoop, &streamLoop}) {
        Check(std::abs(island::AnalyzeAudioSignal(*loop).mean) <= 0.0001,
              "persistent ambience removes material DC before PCM quantization");
    }

    const island::SoundscapeMix beachMix = island::SoundscapeModel::TargetMix(2.0f, false);
    const island::SoundscapeMix jungleMix = island::SoundscapeModel::TargetMix(140.0f, false);
    const island::SoundscapeMix swimmingMix = island::SoundscapeModel::TargetMix(-8.0f, true);
    const island::SoundscapeMix forcedInlandSwimmingMix =
        island::SoundscapeModel::TargetMix(140.0f, true);
    const island::SoundscapeMix wetInteriorMix =
        island::SoundscapeModel::TargetMix(80.0f, false, 0.5f, 1.0f);
    const island::SoundscapeMix dryInteriorMix =
        island::SoundscapeModel::TargetMix(80.0f, false, 0.5f, 0.0f);
    const island::SoundscapeMix seasonalWetlandMix =
        island::SoundscapeModel::TargetMix(80.0f, false, 0.5f, 0.0f, 1.0f);
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
    Check(seasonalWetlandMix.streamVolume==dryInteriorMix.streamVolume
            &&seasonalWetlandMix.jungleVolume>dryInteriorMix.jungleVolume
            &&seasonalWetlandMix.breezeVolume<dryInteriorMix.breezeVolume,
        "seasonal wetland emphasizes insect-rich jungle ambience without false flowing water");
    const auto surfaceAudio=island::UnderwaterAudioResponse(0.0f);
    const auto shallowAudio=island::UnderwaterAudioResponse(.55f);
    const auto deepAudio=island::UnderwaterAudioResponse(2.0f);
    const auto invalidDepthAudio=island::UnderwaterAudioResponse(
        std::numeric_limits<float>::quiet_NaN());
    Check(surfaceAudio.immersion==0&&surfaceAudio.airGain==1&&surfaceAudio.surfGain==1
          &&surfaceAudio.waterGain==1&&surfaceAudio.waterPitchOffset==0,
          "surface swimming leaves the existing sound mix unchanged");
    Check(shallowAudio.immersion>0&&shallowAudio.immersion<1
          &&shallowAudio.airGain<1&&shallowAudio.surfGain<1
          &&shallowAudio.waterGain>1&&shallowAudio.waterPitchOffset<0,
          "submersion progressively muffles air and deepens the water layer");
    Check(deepAudio.immersion==1&&deepAudio.airGain==.18f&&deepAudio.surfGain==.72f
          &&deepAudio.waterGain==1.22f&&deepAudio.waterPitchOffset==-.12f,
          "deep-water audio response stays inside its authored bounds");
    Check(invalidDepthAudio.immersion==0&&invalidDepthAudio.airGain==1,
          "invalid submersion depth fails safely to the surface mix");
    Check(wetInteriorMix.streamVolume >= 0.22f
              && dryInteriorMix.streamVolume == 0.0f
              && wetInteriorMix.jungleVolume > dryInteriorMix.jungleVolume
              && wetInteriorMix.breezeVolume < dryInteriorMix.breezeVolume,
          "the local stream biome adds flowing water and denser damp-jungle ambience");
    const auto nearStream = island::StreamSoundField::Sample(
        island::StreamRoute::SourceX - 48.0f,
        island::StreamRoute::CentreZ(island::StreamRoute::SourceX - 48.0f));
    const auto farFromStream = island::StreamSoundField::Sample(120.0f, 120.0f);
    const auto invalidStream = island::StreamSoundField::Sample(
        std::numeric_limits<float>::quiet_NaN(), 0.0f);
    Check(nearStream.valid && nearStream.distance < 0.01f && nearStream.presence > 0.999f
              && farFromStream.valid && farFromStream.presence == 0.0f
              && !invalidStream.valid && invalidStream.presence == 0.0f,
          "stream sound field follows the shared route and fails safely for invalid listeners");
    Check(island::StreamSoundField::ShouldUseStreamPlayback(false, 0.02f, false)
              && island::StreamSoundField::ShouldUseStreamPlayback(false, 0.01f, true)
              && !island::StreamSoundField::ShouldUseStreamPlayback(false, 0.01f, false)
              && !island::StreamSoundField::ShouldUseStreamPlayback(true, 1.0f, true)
              && !island::StreamSoundField::ShouldUseStreamPlayback(false,
                  std::numeric_limits<float>::quiet_NaN(), true),
          "stream and player-water share one voice with finite hysteretic arbitration");
    const float eastExposure=island::CoastalWindResponse::Exposure(230.0f,0.0f,2.0f);
    const float westExposure=island::CoastalWindResponse::Exposure(-180.0f,0.0f,2.0f);
    const auto eastCoastMix=island::SoundscapeModel::TargetMix(2.0f,false,eastExposure);
    const auto westBayMix=island::SoundscapeModel::TargetMix(2.0f,false,westExposure);
    Check(eastExposure>.98f&&westExposure<.27f
        &&island::CoastalWindResponse::WaveScale(eastExposure)>1.20f
        &&island::CoastalWindResponse::WaveScale(westExposure)<.90f
        &&island::CoastalWindResponse::FoamScale(eastExposure)>1.24f
        &&eastCoastMix.surfVolume>westBayMix.surfVolume*1.25f
        &&eastCoastMix.breezeVolume>westBayMix.breezeVolume*1.25f,
        "windward east coast has stronger waves, foam, surf and breeze than the west bay");
    CheckNear(island::CoastalWindResponse::Exposure(0.0f,0.0f,500.0f),.5f,.0001f,
        "coastal wind modulation returns to neutral away from the shore");
    CheckNear(island::CoastalWindResponse::AmbientScale(
        std::numeric_limits<float>::quiet_NaN()),1.0f,.0001f,
        "invalid coastal wind exposure fails safely to the neutral mix");
    const island::SoundscapeMix invalidPositionMix = island::SoundscapeModel::TargetMix(
        std::numeric_limits<float>::quiet_NaN(), false);
    Check(std::isfinite(invalidPositionMix.surfVolume)
              && std::isfinite(invalidPositionMix.breezeVolume)
              && std::isfinite(invalidPositionMix.jungleVolume)
              && invalidPositionMix.surfVolume == 0.0f
              && invalidPositionMix.breezeVolume == 0.0f
              && invalidPositionMix.jungleVolume == 0.0f
              && invalidPositionMix.streamVolume == 0.0f,
          "non-finite soundscape positions fail closed to a finite silent target");
    island::SoundscapeModel soundscapeModel;
    soundscapeModel.Reset(2.0f, false);
    const island::SoundscapeMix beforeInvalidDelta = soundscapeModel.Mix();
    const island::SoundscapeMix afterInvalidDelta = soundscapeModel.Update(
        140.0f, false, std::numeric_limits<float>::infinity());
    Check(afterInvalidDelta.surfVolume == beforeInvalidDelta.surfVolume
              && afterInvalidDelta.breezeVolume == beforeInvalidDelta.breezeVolume
              && afterInvalidDelta.jungleVolume == beforeInvalidDelta.jungleVolume
              && afterInvalidDelta.streamVolume == beforeInvalidDelta.streamVolume,
          "non-finite soundscape time cannot poison or advance the current mix");
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

    constexpr float ambientVariationDistance = 70.0f;
    const island::SoundscapeMix ambientVariationBase =
        island::SoundscapeModel::TargetMix(ambientVariationDistance, false);
    island::SoundscapeModel ambientVariationModel;
    ambientVariationModel.Reset(ambientVariationDistance, false);
    for (int step = 0; step < 240; ++step) {
        (void)ambientVariationModel.Update(
            ambientVariationDistance, false, 0.25f);
    }
    const island::SoundscapeMix variedAmbient = ambientVariationModel.Mix();
    Check(variedAmbient.surfVolume < ambientVariationBase.surfVolume
              && variedAmbient.surfVolume >= ambientVariationBase.surfVolume * 0.965f,
          "slow surf-bed variation remains a restrained downward gain drift");
    Check(variedAmbient.breezeVolume < ambientVariationBase.breezeVolume
              && variedAmbient.breezeVolume >= ambientVariationBase.breezeVolume * 0.925f,
          "slow canopy-bed variation remains inside its authored gain depth");
    Check(variedAmbient.jungleVolume < ambientVariationBase.jungleVolume
              && variedAmbient.jungleVolume >= ambientVariationBase.jungleVolume * 0.945f,
          "slow jungle-bed variation remains inside its authored gain depth");
    island::SoundscapeModel repeatedAmbientVariationModel;
    repeatedAmbientVariationModel.Reset(ambientVariationDistance, false);
    for (int step = 0; step < 240; ++step) {
        (void)repeatedAmbientVariationModel.Update(
            ambientVariationDistance, false, 0.25f);
    }
    Check(variedAmbient.surfVolume == repeatedAmbientVariationModel.Mix().surfVolume
              && variedAmbient.breezeVolume
                  == repeatedAmbientVariationModel.Mix().breezeVolume
              && variedAmbient.jungleVolume
                  == repeatedAmbientVariationModel.Mix().jungleVolume,
          "slow ambient-bed variation is deterministic after reset");

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
    const island::WaterMovementMix invalidWaterInputs =
        island::WaterMovementModel::TargetMix(
            std::numeric_limits<float>::quiet_NaN(), false,
            std::numeric_limits<float>::infinity(),
            std::numeric_limits<float>::quiet_NaN());
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
    Check(std::isfinite(invalidWaterInputs.volume)
              && std::isfinite(invalidWaterInputs.pitch)
              && std::isfinite(invalidWaterInputs.currentCue)
              && invalidWaterInputs.volume == 0.0f
              && invalidWaterInputs.currentCue == 0.0f,
          "non-finite water depth and speed fail closed to a finite dry mix");
    island::WaterMovementModel waterMovementModel;
    waterMovementModel.Reset(0.0f, false, 0.0f);
    const island::WaterMovementMix beforeInvalidWaterMixDelta = waterMovementModel.Mix();
    const island::WaterMovementMix afterInvalidWaterDelta = waterMovementModel.Update(
        std::numeric_limits<float>::quiet_NaN(), false,
        std::numeric_limits<float>::infinity(),
        std::numeric_limits<float>::quiet_NaN(),
        std::numeric_limits<float>::quiet_NaN());
    Check(afterInvalidWaterDelta.volume == beforeInvalidWaterMixDelta.volume
              && afterInvalidWaterDelta.pitch == beforeInvalidWaterMixDelta.pitch
              && afterInvalidWaterDelta.currentCue == beforeInvalidWaterMixDelta.currentCue,
          "non-finite water-mix time cannot poison or advance the current mix");
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
    island::WaterTransitionScheduler invalidTransitionScheduler;
    invalidTransitionScheduler.Reset(0.0f, false);
    const island::WaterTransitionRequest invalidDepthTransition =
        invalidTransitionScheduler.Update(
            std::numeric_limits<float>::quiet_NaN(), false, 3.0f, 0.01f);
    const island::WaterTransitionRequest invalidSpeedTransition =
        invalidTransitionScheduler.Update(
            0.12f, false, std::numeric_limits<float>::infinity(), 0.01f);
    const island::WaterTransitionRequest recoveredTransition =
        invalidTransitionScheduler.Update(0.12f, false, 3.0f, 0.01f);
    Check(invalidDepthTransition.type == island::WaterTransitionType::None
              && invalidSpeedTransition.type == island::WaterTransitionType::None
              && recoveredTransition.type == island::WaterTransitionType::Enter,
          "invalid shoreline audio inputs do not mutate state or block the next valid crossing");
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

    island::FootstepScheduler footstepScheduler;
    footstepScheduler.Reset();
    Check(footstepScheduler.Update(1.0f / 60.0f, false, 6.0f,
                                   island::PlayerMotion::WalkSpeed, 0.0f, 0.0f).surface
              == island::FootstepSurface::None,
          "footstep scheduling latches its first resolved player position silently");
    int walkingFootsteps = 0;
    std::array<bool, 2> sandFootstepVariants = {false, false};
    bool footstepPlaybackBoundsValid = true;
    float walkingPosition = 0.0f;
    for (int frame = 0; frame < 20 * 60; ++frame) {
        walkingPosition += island::PlayerMotion::WalkSpeed / 60.0f;
        const island::FootstepRequest step = footstepScheduler.Update(
            1.0f / 60.0f, false, 6.0f, island::PlayerMotion::WalkSpeed,
            walkingPosition, 0.0f);
        if (step.surface == island::FootstepSurface::None) {
            continue;
        }
        ++walkingFootsteps;
        if (step.variant < sandFootstepVariants.size()) {
            sandFootstepVariants[step.variant] = true;
        } else {
            footstepPlaybackBoundsValid = false;
        }
        footstepPlaybackBoundsValid = footstepPlaybackBoundsValid
            && step.surface == island::FootstepSurface::Sand
            && step.volume >= 0.12f && step.volume <= 0.203f
            && step.pitch >= -0.015f && step.pitch <= 0.10f
            && step.pan >= -0.070f && step.pan <= 0.070f;
    }
    Check(walkingFootsteps >= 40 && walkingFootsteps <= 44
              && sandFootstepVariants[0] && sandFootstepVariants[1]
              && footstepPlaybackBoundsValid,
          "walk footsteps follow resolved distance with bounded varied sand playback");

    island::FootstepScheduler runningFootstepScheduler;
    runningFootstepScheduler.Reset();
    (void)runningFootstepScheduler.Update(
        1.0f / 60.0f, false, 80.0f, island::PlayerMotion::RunSpeed, 0.0f, 0.0f);
    int runningFootsteps = 0;
    std::array<bool, 2> soilFootstepVariants = {false, false};
    float runningPosition = 0.0f;
    for (int frame = 0; frame < 20 * 60; ++frame) {
        runningPosition += island::PlayerMotion::RunSpeed / 60.0f;
        const island::FootstepRequest step = runningFootstepScheduler.Update(
            1.0f / 60.0f, false, 80.0f, island::PlayerMotion::RunSpeed,
            runningPosition, 0.0f);
        if (step.surface == island::FootstepSurface::None) {
            continue;
        }
        ++runningFootsteps;
        Check(step.surface == island::FootstepSurface::Soil,
              "interior travel selects the dry-soil footstep family");
        if (step.variant < soilFootstepVariants.size()) {
            soilFootstepVariants[step.variant] = true;
        }
    }
    Check(runningFootsteps >= 57 && runningFootsteps <= 60
              && runningFootsteps > walkingFootsteps
              && soilFootstepVariants[0] && soilFootstepVariants[1],
          "running increases cadence while retaining both interior footstep variants");

    island::FootstepScheduler guardedFootstepScheduler;
    guardedFootstepScheduler.Reset();
    (void)guardedFootstepScheduler.Update(
        1.0f / 60.0f, false, 50.0f, 3.0f, 0.0f, 0.0f);
    const island::FootstepRequest invalidFootstep = guardedFootstepScheduler.Update(
        std::numeric_limits<float>::quiet_NaN(), false, 50.0f, 3.0f, 0.1f, 0.0f);
    const island::FootstepRequest teleportedFootstep = guardedFootstepScheduler.Update(
        1.0f / 60.0f, false, 50.0f, 3.0f, 100.0f, 0.0f);
    const island::FootstepRequest swimmingFootstep = guardedFootstepScheduler.Update(
        1.0f / 60.0f, true, -5.0f, 2.0f, 100.02f, 0.0f);
    Check(invalidFootstep.surface == island::FootstepSurface::None
              && teleportedFootstep.surface == island::FootstepSurface::None
              && swimmingFootstep.surface == island::FootstepSurface::None,
          "invalid time, teleports, and swimming cannot fabricate land footsteps");

    island::WaveAccentScheduler waveScheduler;
    waveScheduler.Reset();
    bool invalidDeltaWave = false;
    for (int step = 0; step < 100; ++step) {
        invalidDeltaWave = invalidDeltaWave || waveScheduler.Update(
            std::numeric_limits<float>::quiet_NaN(), beachMix.surfVolume).trigger;
    }
    Check(!invalidDeltaWave,
          "non-finite time cannot advance the stronger-wave scheduler");
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

    island::ShipwreckCreakScheduler creakScheduler;
    creakScheduler.Reset();
    bool distantCreak = false;
    for (int step = 0; step < 240; ++step) {
        distantCreak = distantCreak || creakScheduler.Update(
            0.25f, beachMix.surfVolume, 60.0f, true).trigger;
    }
    Check(!distantCreak, "shipwreck creaks remain inaudible away from the wreck");
    bool busyCreak = false;
    for (int step = 0; step < 120; ++step) {
        busyCreak = busyCreak || creakScheduler.Update(
            0.25f, beachMix.surfVolume, 8.0f, false).trigger;
    }
    Check(!busyCreak,
          "shipwreck creaks cannot escape a busy shared shoreline voice");
    const island::ShipwreckCreakRequest releasedCreak = creakScheduler.Update(
        0.01f, beachMix.surfVolume, 8.0f, true);
    Check(releasedCreak.trigger && releasedCreak.variant <= 1U
              && releasedCreak.volume >= 0.035f && releasedCreak.volume <= 0.16f
              && releasedCreak.pitch >= -0.075f && releasedCreak.pitch <= 0.045f,
          "a due nearby wreck creak resumes with bounded playback parameters");
    island::ShipwreckCreakScheduler creakVariantScheduler;
    creakVariantScheduler.Reset();
    std::array<bool, 2> observedCreakVariants = {false, false};
    bool invalidCreak = false;
    for (int step = 0; step < 4000; ++step) {
        const island::ShipwreckCreakRequest creak = creakVariantScheduler.Update(
            0.25f, beachMix.surfVolume, 7.0f, true);
        if (creak.trigger && creak.variant < observedCreakVariants.size()) {
            observedCreakVariants[creak.variant] = true;
        }
        invalidCreak = invalidCreak || creakVariantScheduler.Update(
            std::numeric_limits<float>::quiet_NaN(), beachMix.surfVolume,
            std::numeric_limits<float>::infinity(), true).trigger;
    }
    Check(!invalidCreak && observedCreakVariants[0] && observedCreakVariants[1],
          "wreck scheduling rejects invalid input and reaches both sparse variants");

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

    island::WildlifeCallScheduler invalidWildlifeScheduler;
    invalidWildlifeScheduler.Reset();
    bool invalidWildlifeCallTriggered = false;
    for (int step = 0; step < 100; ++step) {
        invalidWildlifeCallTriggered = invalidWildlifeCallTriggered
            || invalidWildlifeScheduler.Update(
                   std::numeric_limits<float>::quiet_NaN(), false, 90.0f,
                   0.0f, 2.0f, 0.0f, nearbyBird, nearbyMonkey, true).type
                != island::WildlifeCallType::None;
    }
    Check(!invalidWildlifeCallTriggered,
          "non-finite time cannot advance the positional wildlife-call scheduler");
    bool invalidListenerCallTriggered = false;
    for (int step = 0; step < 80; ++step) {
        invalidListenerCallTriggered = invalidListenerCallTriggered
            || invalidWildlifeScheduler.Update(
                   0.25f, false, 90.0f,
                   std::numeric_limits<float>::quiet_NaN(), 2.0f, 0.0f,
                   nearbyBird, nearbyMonkey, true).type
                != island::WildlifeCallType::None;
    }
    Check(!invalidListenerCallTriggered,
          "non-finite listener positions cannot emit a positional wildlife call");

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

    // Boar audio scheduling is independent of rendering and of the playback device.
    for (const auto kind : {island::BoarCallKind::Breath, island::BoarCallKind::Snuffle,
                           island::BoarCallKind::Grunt}) {
        const auto a = island::GenerateBoarCall(kind, 0);
        const auto b = island::GenerateBoarCall(kind, 1);
        Check(a.sampleRate == 22050 && a.samples.size() == 26460U
            && a.samples.front() == 0 && a.samples.back() == 0
            && b.samples.front() == 0 && b.samples.back() == 0
            && a.samples == island::GenerateBoarCall(kind, 0).samples
            && a.samples != b.samples
            && std::any_of(a.samples.begin(), a.samples.end(), [](auto s) { return s > 1000; })
            && std::all_of(a.samples.begin(), a.samples.end(), [](auto s) { return std::abs(s) < 21000; }),
            "boar PCM clips are deterministic, distinct, bounded and silent at endpoints");
        if (kind == island::BoarCallKind::Breath) {
            int zeroCrossings = 0;
            for (std::size_t sample = 1; sample < a.samples.size(); ++sample)
                zeroCrossings += (a.samples[sample - 1] < 0 && a.samples[sample] >= 0)
                    || (a.samples[sample - 1] > 0 && a.samples[sample] <= 0);
            Check(zeroCrossings < 4000,
                "boar breath synthesis regressed into high-frequency hiss");
        }
        Check(island::BoarCallVolume(kind, 0.0f) > island::BoarCallVolume(kind, 5.0f)
            && island::BoarCallVolume(kind, 50.0f) == 0.0f
            && island::BoarCallVolume(kind, -1.0f) == 0.0f
            && island::BoarCallVolume(kind, std::numeric_limits<float>::quiet_NaN()) == 0.0f,
            "boar attenuation is bounded and rejects invalid distances");
    }
    std::array<island::BoarState, island::BoarHerd::BoarCount> audioBoars{};
    for (std::size_t i = 0; i < audioBoars.size(); ++i) {
        audioBoars[i].x = static_cast<float>(i);
        audioBoars[i].simulationTier = island::WildlifeSimulationTier::Full;
        audioBoars[i].behavior = static_cast<island::BoarBehavior>(i % 4U);
    }
    island::BoarCallScheduler boarCalls, repeatedBoarCalls;
    boarCalls.Reset(); repeatedBoarCalls.Reset();
    std::array<int, island::BoarHerd::BoarCount> boarCallCounts{};
    std::array<int, island::BoarHerd::BoarCount> previousBoarCall{};
    previousBoarCall.fill(-1000);
    std::array<bool, 2> boarVariants{};
    int previousHerdCall = -1000;
    bool boarBounds = true, boarDeterminism = true;
    for (int step = 0; step < 2400; ++step) {
        const auto call = boarCalls.Update(0.25f, 0.0f, 1.72f, 0.0f, audioBoars, true);
        const auto repeated = repeatedBoarCalls.Update(0.25f, 0.0f, 1.72f, 0.0f, audioBoars, true);
        boarDeterminism = boarDeterminism && call.kind == repeated.kind
            && call.animalIndex == repeated.animalIndex && call.variant == repeated.variant
            && call.volume == repeated.volume && call.pitch == repeated.pitch;
        if (call.kind == island::BoarCallKind::None) continue;
        const std::size_t i = call.animalIndex;
        const auto behavior = audioBoars[i].behavior;
        const auto expected = behavior == island::BoarBehavior::Rest ? island::BoarCallKind::Breath
            : behavior == island::BoarBehavior::Forage ? island::BoarCallKind::Snuffle
            : island::BoarCallKind::Grunt;
        boarBounds = boarBounds && (call.kind == expected || call.kind == island::BoarCallKind::Grunt)
            && step - previousHerdCall >= 10
            && step - previousBoarCall[i] >= 24 && call.volume > 0.0f && call.volume <= 0.40f
            && call.pitch >= -0.07f && call.pitch <= 0.07f && call.variant < 2U
            && call.source.available && call.source.x == audioBoars[i].x
            && std::abs(call.source.y - 0.55f) < 0.0001f;
        previousHerdCall = step; previousBoarCall[i] = step;
        ++boarCallCounts[i]; boarVariants[call.variant] = true;
    }
    Check(boarBounds && boarDeterminism && boarVariants[0] && boarVariants[1]
        && std::all_of(boarCallCounts.begin(), boarCallCounts.end(), [](int n) { return n > 5; }),
        "boar calls match behavior, vary, remain bounded and fairly service each animal");
    for(const auto behavior:{island::BoarBehavior::Rest,island::BoarBehavior::Forage}) {
        for(float distance:{2.0f,7.0f,15.0f,30.0f}) {
            std::array<island::BoarState,1> animals{};
            animals[0].scale=1;animals[0].behavior=behavior;
            animals[0].simulationTier=island::WildlifeSimulationTier::Full;
            island::BoarCallScheduler scheduler;scheduler.Reset();
            int grunts=0,details=0,firstGrunt=-1;
            for(int step=0;step<480;++step) {
                const auto call=scheduler.Update(.25f,distance,.55f,0,animals,true);
                if(call.kind==island::BoarCallKind::Grunt){++grunts;if(firstGrunt<0)firstGrunt=step;}
                if(call.kind==island::BoarCallKind::Breath||call.kind==island::BoarCallKind::Snuffle)++details;
            }
            Check(grunts>=2&&firstGrunt>=0&&firstGrunt<44,
                "resting/foraging boar must grunt without requiring movement or player approach");
            if(distance==2)Check(details>=2,"near boar lost its breathing/snuffling variation");
        }
    }
    const auto noBoarCall = [](const auto& animals, float x, float dt, bool slot) {
        island::BoarCallScheduler scheduler;
        scheduler.Reset();
        bool silent = true;
        for (int step = 0; step < 200; ++step) {
            const auto call = scheduler.Update(dt, x, 1.72f, 0.0f, animals, slot);
            silent = silent && call.kind == island::BoarCallKind::None;
        }
        return silent;
    };
    Check(noBoarCall(audioBoars, 0.0f, 0.25f, false)
        && noBoarCall(audioBoars, 300.0f, 0.25f, true)
        && noBoarCall(audioBoars, std::numeric_limits<float>::quiet_NaN(), 0.25f, true)
        && noBoarCall(audioBoars, 0.0f, std::numeric_limits<float>::quiet_NaN(), true)
        && noBoarCall(audioBoars, 0.0f, -1.0f, true)
        && noBoarCall(audioBoars, 0.0f, 0.0f, true),
        "boar calls respect unavailable slots, range, invalid listener and paused/invalid time");
    for (auto& boar : audioBoars) boar.simulationTier = island::WildlifeSimulationTier::Dormant;
    Check(noBoarCall(audioBoars, 0.0f, 0.25f, true), "dormant boars remain silent");
    for (auto& boar : audioBoars) {
        boar.simulationTier = island::WildlifeSimulationTier::Full;
        boar.x = std::numeric_limits<float>::infinity();
    }
    Check(noBoarCall(audioBoars, 0.0f, 0.25f, true), "invalid boar positions remain silent");
    Check(noBoarCall(std::span<const island::BoarState>{}, 0.0f, 0.25f, true),
        "empty boar population remains silent");
    std::array<island::BoarState, 1> loneBoar{};
    loneBoar[0].behavior = island::BoarBehavior::Wander;
    loneBoar[0].simulationTier = island::WildlifeSimulationTier::Full;
    island::BoarCallScheduler nearBoar, farBoar, heldBoar;
    nearBoar.Reset(); farBoar.Reset(); heldBoar.Reset();
    bool quieterAtDistance = false;
    for (int step = 0; step < 200; ++step) {
        const auto near = nearBoar.Update(0.25f, 0.0f, 1.72f, 0.0f, loneBoar, true);
        const auto far = farBoar.Update(0.25f, 20.0f, 1.72f, 0.0f, loneBoar, true);
        (void)heldBoar.Update(0.25f, 0.0f, 1.72f, 0.0f, loneBoar, false);
        if (near.kind != island::BoarCallKind::None) {
            quieterAtDistance = far.kind == near.kind && far.volume < near.volume * 0.3f;
        }
    }
    Check(quieterAtDistance, "boar volume attenuates with listener distance");
    Check(heldBoar.Update(0.0f, 0.0f, 1.72f, 0.0f, loneBoar, true).kind
        == island::BoarCallKind::None, "paused overdue boar calls cannot start playback");
    Check(heldBoar.Update(0.25f, 0.0f, 1.72f, 0.0f, loneBoar, true).kind
        == island::BoarCallKind::Grunt, "one due boar call resumes when a slot opens");
    Check(heldBoar.Update(0.25f, 0.0f, 1.72f, 0.0f, loneBoar, true).kind
        == island::BoarCallKind::None, "resuming boar audio cannot replay a backlog burst");

    for (const auto kind : {island::GoatCallKind::Breath,island::GoatCallKind::Bleat,
                           island::GoatCallKind::Hoof}) {
        const auto a=island::GenerateGoatCall(kind,0),b=island::GenerateGoatCall(kind,1);
        Check(a.sampleRate==22050&&a.samples.size()==26460U&&b.samples.size()==26460U
            &&a.samples.front()==0&&a.samples.back()==0&&b.samples.front()==0
            &&b.samples.back()==0&&a.samples==island::GenerateGoatCall(kind,0).samples
            &&a.samples!=b.samples
            &&std::any_of(a.samples.begin(),a.samples.end(),[](auto sample){return sample>1000;})
            &&std::all_of(a.samples.begin(),a.samples.end(),[](auto sample){return std::abs(sample)<21000;}),
            "goat PCM clips are deterministic, distinct, bounded and silent at endpoints");
        if(kind==island::GoatCallKind::Breath) {
            int zeroCrossings=0;
            for(std::size_t sample=1;sample<a.samples.size();++sample)
                zeroCrossings+=(a.samples[sample-1]<0&&a.samples[sample]>=0)
                    ||(a.samples[sample-1]>0&&a.samples[sample]<=0);
            Check(zeroCrossings<4000,
                "goat breath synthesis regressed into high-frequency hiss");
        }
        Check(island::GoatCallVolume(kind,0)>island::GoatCallVolume(kind,5)
            &&island::GoatCallVolume(kind,60)==0&&island::GoatCallVolume(kind,-1)==0
            &&island::GoatCallVolume(kind,std::numeric_limits<float>::quiet_NaN())==0,
            "goat attenuation is bounded and rejects invalid distances");
    }
    std::array<island::GoatState,island::GoatHabitat::Count> audioGoats{};
    for(std::size_t i=0;i<audioGoats.size();++i) {
        audioGoats[i].x=static_cast<float>(i)*1.25f;
        audioGoats[i].scale=.82f+static_cast<float>(i)*.04f;
        audioGoats[i].behavior=static_cast<island::GoatBehavior>(i%4U);
    }
    island::GoatCallScheduler goatCalls,repeatedGoatCalls;
    goatCalls.Reset();repeatedGoatCalls.Reset();
    std::array<int,island::GoatHabitat::Count> goatCounts{};
    std::array<bool,4> goatKinds{};
    std::array<bool,2> goatVariants{};
    int previousGoatCall=-1000;
    bool goatDeterministic=true,goatBounds=true;
    for(int step=0;step<2400;++step) {
        const auto call=goatCalls.Update(.25f,0,1.72f,0,audioGoats,true);
        const auto repeated=repeatedGoatCalls.Update(.25f,0,1.72f,0,audioGoats,true);
        goatDeterministic=goatDeterministic&&call.kind==repeated.kind
            &&call.animalIndex==repeated.animalIndex&&call.variant==repeated.variant
            &&call.volume==repeated.volume&&call.pitch==repeated.pitch;
        if(call.kind==island::GoatCallKind::None)continue;
        goatBounds=goatBounds&&call.animalIndex<audioGoats.size()
            &&step-previousGoatCall>=11&&call.volume>0&&call.volume<=.40f
            &&call.pitch>=-.07f&&call.pitch<=.08f&&call.variant<2U
            &&call.source.available&&call.source.x==audioGoats[call.animalIndex].x;
        previousGoatCall=step;++goatCounts[call.animalIndex];
        goatKinds[static_cast<std::size_t>(call.kind)]=true;goatVariants[call.variant]=true;
    }
    Check(goatDeterministic&&goatBounds&&goatVariants[0]&&goatVariants[1]
        &&goatKinds[static_cast<std::size_t>(island::GoatCallKind::Breath)]
        &&goatKinds[static_cast<std::size_t>(island::GoatCallKind::Bleat)]
        &&goatKinds[static_cast<std::size_t>(island::GoatCallKind::Hoof)]
        &&std::all_of(goatCounts.begin(),goatCounts.end(),[](int count){return count>4;}),
        "goat calls match behavior, vary, remain bounded and fairly service the herd");
    const auto noGoatCall=[](const auto& goats,float listenerX,float dt,bool slot) {
        island::GoatCallScheduler scheduler;scheduler.Reset();
        for(int step=0;step<200;++step)
            if(scheduler.Update(dt,listenerX,1.72f,0,goats,slot).kind
                !=island::GoatCallKind::None)return false;
        return true;
    };
    Check(noGoatCall(audioGoats,0,.25f,false)&&noGoatCall(audioGoats,300,.25f,true)
        &&noGoatCall(audioGoats,std::numeric_limits<float>::quiet_NaN(),.25f,true)
        &&noGoatCall(audioGoats,0,0,true)
        &&noGoatCall(std::span<const island::GoatState>{},0,.25f,true),
        "goat calls respect shared slots, range, valid listeners, paused time and empty herds");

    const auto hissA=island::GenerateSnakeHiss(0),hissB=island::GenerateSnakeHiss(1);
    Check(hissA.sampleRate==22050 && hissA.samples.size()==22050U
        && hissB.sampleRate==22050 && hissB.samples.size()==22050U
        && hissA.samples!=hissB.samples && hissA.samples.front()==0
        && hissA.samples.back()==0 && hissB.samples.front()==0 && hissB.samples.back()==0
        && hissA.samples==island::GenerateSnakeHiss(0).samples,
        "snake hiss PCM is deterministic, distinct, bounded and silent at endpoints");
    Check(island::SnakeHissVolume(0)>.25f
        && island::SnakeHissVolume(8)<island::SnakeHissVolume(2)
        && island::SnakeHissVolume(16)==0 && island::SnakeHissVolume(-1)==0
        && island::SnakeHissVolume(std::numeric_limits<float>::quiet_NaN())==0,
        "snake hiss attenuation is local and rejects invalid distances");
    std::array<island::SnakeState,island::SnakePopulation::Count> audioSnakes{};
    for(std::size_t i=0;i<audioSnakes.size();++i) {
        auto& snake=audioSnakes[i];
        const float angle=2.0f*std::numbers::pi_v<float>*static_cast<float>(i)
            / static_cast<float>(audioSnakes.size());
        snake.body.front()={6.0f*std::cos(angle),.05f,6.0f*std::sin(angle)};
        snake.behavior=island::SnakeBehavior::Retreat;
        snake.tier=island::WildlifeSimulationTier::Full;
    }
    island::SnakeHissScheduler snakeHisses,repeatedSnakeHisses;
    snakeHisses.Reset();repeatedSnakeHisses.Reset();
    std::array<int,island::SnakePopulation::Count> hissCounts{};
    std::array<bool,2> hissVariants{};
    int lastHiss=-1000;
    bool hissDeterministic=true,hissBounded=true;
    for(int step=0;step<1600;++step) {
        const auto hiss=snakeHisses.Update(.25f,0,1.7f,0,audioSnakes,true);
        const auto repeat=repeatedSnakeHisses.Update(.25f,0,1.7f,0,audioSnakes,true);
        hissDeterministic=hissDeterministic&&hiss.trigger==repeat.trigger
            &&hiss.animalIndex==repeat.animalIndex&&hiss.variant==repeat.variant
            &&hiss.volume==repeat.volume&&hiss.pitch==repeat.pitch;
        if(!hiss.trigger)continue;
        hissBounded=hissBounded&&hiss.animalIndex<audioSnakes.size()
            &&step-lastHiss>=12&&hiss.volume>0&&hiss.volume<=.31f
            &&hiss.pitch>=-.08f&&hiss.pitch<=.08f&&hiss.source.available
            &&hiss.source.x==audioSnakes[hiss.animalIndex].body.front().x;
        lastHiss=step;++hissCounts[hiss.animalIndex];hissVariants[hiss.variant]=true;
    }
    Check(hissDeterministic&&hissBounded&&hissVariants[0]&&hissVariants[1]
        &&std::all_of(hissCounts.begin(),hissCounts.end(),[](int count){return count>0;}),
        "snake hisses are deterministic, varied, sparse and fairly service nearby snakes");
    const auto noSnakeHiss=[](auto snakes,float listenerX,float dt,bool slot) {
        island::SnakeHissScheduler scheduler;scheduler.Reset();
        for(int step=0;step<80;++step)
            if(scheduler.Update(dt,listenerX,1.7f,0,snakes,slot).trigger)return false;
        return true;
    };
    Check(noSnakeHiss(audioSnakes,0,.25f,false)
        &&noSnakeHiss(audioSnakes,100,.25f,true)
        &&noSnakeHiss(audioSnakes,std::numeric_limits<float>::quiet_NaN(),.25f,true)
        &&noSnakeHiss(audioSnakes,0,0,true),
        "snake hisses respect shared slots, local range, valid listeners and paused time");
    for(auto& snake:audioSnakes)snake.behavior=island::SnakeBehavior::Explore;
    Check(noSnakeHiss(audioSnakes,0,.25f,true),"non-retreating snakes hissed without close awareness");
    auto oneEligibleSnake=audioSnakes;
    oneEligibleSnake.front().behavior=island::SnakeBehavior::Retreat;
    oneEligibleSnake.front().tier=island::WildlifeSimulationTier::Full;
    for(std::size_t i=1;i<oneEligibleSnake.size();++i)
        oneEligibleSnake[i].tier=island::WildlifeSimulationTier::Dormant;
    island::SnakeHissScheduler oneSnakeScheduler;
    oneSnakeScheduler.Reset();
    bool oneSnakeTriggered=false;
    for(int step=0;step<100&&!oneSnakeTriggered;++step)
        oneSnakeTriggered=oneSnakeScheduler.Update(.1f,0,1.7f,0,oneEligibleSnake,true).trigger;
    Check(oneSnakeTriggered,"one close retreating snake never received a hiss request");
    for(auto& snake:audioSnakes) {
        snake.behavior=island::SnakeBehavior::Retreat;
        snake.tier=island::WildlifeSimulationTier::Dormant;
    }
    Check(noSnakeHiss(audioSnakes,0,.25f,true),"dormant snakes hissed");

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
    Check(touchState.runRequested,
          "a near-full movement-stick throw requests the faster land pace");

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
    Check(!touchState.runRequested,
          "releasing the movement contact cancels its run request");

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

    island::TouchControls touchRunControls;
    std::array<island::TouchContact, 1> touchRunContact = {{
        {41, 150.0f, 500.0f, island::TouchPhase::Pressed},
    }};
    (void)touchRunControls.Update(touchRunContact, 1280.0f, 720.0f);
    touchRunContact[0] = {41, 250.0f, 500.0f, island::TouchPhase::Moved};
    island::TouchControlState touchRunState = touchRunControls.Update(
        touchRunContact, 1280.0f, 720.0f);
    Check(touchRunState.runRequested,
          "full touch-stick travel engages touch running");
    touchRunContact[0] = {41, 235.0f, 500.0f, island::TouchPhase::Moved};
    touchRunState = touchRunControls.Update(touchRunContact, 1280.0f, 720.0f);
    Check(touchRunState.runRequested,
          "touch running remains engaged inside its release hysteresis");
    touchRunContact[0] = {41, 220.0f, 500.0f, island::TouchPhase::Moved};
    touchRunState = touchRunControls.Update(touchRunContact, 1280.0f, 720.0f);
    Check(!touchRunState.runRequested,
          "easing the stick inward restores touch walking");

    island::TouchControls touchSensitivityControls;
    const island::TouchSensitivitySelectorLayout sensitivitySelector =
        island::TouchControls::SensitivitySelectorLayout(1280.0f, 720.0f);
    std::array<island::TouchContact, 1> sensitivityContact = {{
        {51, sensitivitySelector.x, sensitivitySelector.y, island::TouchPhase::Pressed},
    }};
    island::TouchControlState sensitivityState = touchSensitivityControls.Update(
        sensitivityContact, 1280.0f, 720.0f);
    Check(sensitivityState.lookSensitivity == island::TouchLookSensitivityLevel::High
              && sensitivityState.lookSensitivityChanged
              && sensitivityState.sensitivitySelectorActive
              && !sensitivityState.lookActive,
          "touch sensitivity selector chooses high without stealing the look role");
    CheckNear(island::TouchControls::LookSensitivityScale(
                  sensitivityState.lookSensitivity), 1.35f, 0.0001f,
              "high touch sensitivity exposes the documented bounded multiplier");
    sensitivityContact[0].phase = island::TouchPhase::Moved;
    sensitivityState = touchSensitivityControls.Update(
        sensitivityContact, 1280.0f, 720.0f);
    Check(sensitivityState.lookSensitivity == island::TouchLookSensitivityLevel::High
              && !sensitivityState.lookSensitivityChanged
              && sensitivityState.sensitivitySelectorActive
              && !sensitivityState.lookActive,
          "holding the sensitivity selector cannot cycle repeatedly or rotate the camera");
    sensitivityContact[0].phase = island::TouchPhase::Released;
    sensitivityState = touchSensitivityControls.Update(
        sensitivityContact, 1280.0f, 720.0f);
    Check(!sensitivityState.sensitivitySelectorActive,
          "releasing the sensitivity selector clears touch-interaction ownership");
    sensitivityContact[0] = {52, sensitivitySelector.x, sensitivitySelector.y,
                             island::TouchPhase::Pressed};
    sensitivityState = touchSensitivityControls.Update(
        sensitivityContact, 1280.0f, 720.0f);
    Check(sensitivityState.lookSensitivity == island::TouchLookSensitivityLevel::Low
              && sensitivityState.lookSensitivityChanged,
          "a second sensitivity tap cycles from high to low");
    (void)touchSensitivityControls.Update({}, 1000.0f, 600.0f);
    Check(touchSensitivityControls.State().lookSensitivity
              == island::TouchLookSensitivityLevel::Low,
          "display resize releases touch ownership without losing sensitivity selection");
    CheckNear(island::TouchControls::LookSensitivityScale(
                  island::TouchLookSensitivityLevel::Low), 0.72f, 0.0001f,
              "low touch sensitivity exposes the documented bounded multiplier");
    island::TouchControls restoredTouchControls;
    restoredTouchControls.SetLookSensitivity(
        island::TouchLookSensitivityLevel::High);
    Check(restoredTouchControls.State().lookSensitivity
              == island::TouchLookSensitivityLevel::High
              && !restoredTouchControls.State().movementActive
              && !restoredTouchControls.State().lookActive,
          "persisted touch sensitivity restores without retaining pointer ownership");

    const std::array<island::TouchContact, 1> invalidTouch = {{
        {44, std::numeric_limits<float>::quiet_NaN(), 300.0f,
         island::TouchPhase::Pressed},
    }};
    touchState = touchControls.Update(invalidTouch, 1280.0f, 720.0f);
    Check(!touchState.movementActive && !touchState.lookActive
              && std::isfinite(touchState.moveRight)
              && std::isfinite(touchState.moveForward)
              && std::isfinite(touchState.lookDeltaX)
              && std::isfinite(touchState.lookDeltaY),
          "non-finite touch coordinates cannot acquire a role or poison output");

    const std::array<island::TouchContact, 1> validLook = {{
        {45, 1000.0f, 360.0f, island::TouchPhase::Pressed},
    }};
    touchState = touchControls.Update(validLook, 1280.0f, 720.0f);
    Check(touchState.lookActive,
          "a valid look contact acquires its role after an invalid contact");
    const std::array<island::TouchContact, 1> poisonedOwnedLook = {{
        {45, std::numeric_limits<float>::infinity(), 360.0f,
         island::TouchPhase::Moved},
    }};
    touchState = touchControls.Update(poisonedOwnedLook, 1280.0f, 720.0f);
    Check(!touchState.lookActive && touchState.lookDeltaX == 0.0f
              && touchState.lookDeltaY == 0.0f,
          "an invalid owned look contact is cancelled without a camera jump");

    const std::array<island::TouchContact, 1> implausibleTouch = {{
        {46, std::numeric_limits<float>::max(), 300.0f,
         island::TouchPhase::Pressed},
    }};
    touchState = touchControls.Update(implausibleTouch, 1280.0f, 720.0f);
    Check(!touchState.movementActive && !touchState.lookActive,
          "implausibly distant finite touch coordinates are rejected");
    touchState = touchControls.Update(validLook,
                                      std::numeric_limits<float>::quiet_NaN(), 720.0f);
    Check(!touchState.movementActive && !touchState.lookActive
              && touchState.moveRight == 0.0f && touchState.moveForward == 0.0f,
          "non-finite display geometry releases all touch ownership safely");

    const island::IslandTerrain terrain;
    {
        const auto seabed=std::make_unique<island::RenderedTerrainSurface>(terrain);
        island::PlayerMotion swimmer;
        bool found=false;
        for(int angle=0;angle<32&&!found;++angle) {
            const float a=angle*6.2831853f/32;
            const float r=terrain.ShoreRadius(a)+38;
            const float x=std::cos(a)*r,z=std::sin(a)*r;
            if(std::abs(x)>330||std::abs(z)>330)continue;
            if(seabed->HeightRange(x-.34f,z-.34f,x+.34f,z+.34f)[1]<-1.3f) {
                swimmer.SetPosition(terrain,x,z); found=true;
            }
        }
        Check(found,"diving test locates actual water with room below the surface");
        if(found) {
            swimmer.SetSwimInput(seabed.get(),false,true);
            for(int i=0;i<5;++i) swimmer.Update(terrain,0,0,.1f);
            Check(swimmer.Position().y<-.2f,"player movement now reaches below the water surface");
            auto before=swimmer.Position();
            swimmer.SetSwimInput(seabed.get(),true,true);
            swimmer.Update(terrain,0,0,.1f);
            Check(std::abs(swimmer.Position().y-before.y)<1e-5f,"opposed dive input holds live player depth");
            swimmer.SetSwimInput(seabed.get(),false,false);
            int rejected=0;
            for(int i=0;i<300;++i) {
                before=swimmer.Position();
                const float r=std::hypot(before.x,before.z);
                swimmer.Update(terrain,-before.x/r,-before.z/r,.1f);
                const auto p=swimmer.Position();
                const auto support=seabed->HeightRange(std::min(before.x,p.x)-.34f,
                    std::min(before.z,p.z)-.34f,std::max(before.x,p.x)+.34f,
                    std::max(before.z,p.z)+.34f)[1];
                Check(p.y>=support+island::SwimDepth::BottomClearance-.0001f,
                    "submerged movement cannot cross rendered seabed along its swept footprint");
                rejected+=swimmer.MovementTrace().terrainRejections;
            }
            Check(rejected>0,"seabed ascent blocks underwater horizontal motion");
            swimmer.SetSwimInput(seabed.get(),true,false);
            for(int i=0;i<200;++i)swimmer.Update(terrain,0,0,.1f);
            Check(swimmer.Position().y==island::PlayerMotion::SwimmingEyeHeight,"player resurfaces");
            swimmer.SetSwimInput(seabed.get(),false,true);
            swimmer.Update(terrain,0,0,.1f);
            const auto p=swimmer.Position();
            swimmer.SetPosition(terrain,p.x,p.z);
            Check(swimmer.SubmergedDepth()==0&&swimmer.Position().y==.58f,"teleport clears immersion");
        }
    }

    const island::DistantSceneryLayout distantScenery;
    const island::DistantSceneryLayout repeatedDistantScenery;
    Check(distantScenery.Islets().size() == island::DistantSceneryLayout::IsletCount,
          "distant scenery keeps its intentionally small bounded population");
    {
        // Depth on the horizon: at least one islet within 600 m, at least one beyond 950 m, and
        // exactly one treeless sandbar.
        float nearest = 1.0e9f, farthest = 0.0f; int sandbars = 0;
        for (const auto& islet : distantScenery.Islets()) {
            nearest = std::min(nearest, std::hypot(islet.x, islet.z));
            farthest = std::max(farthest, std::hypot(islet.x, islet.z));
            sandbars += islet.variant == island::DistantSceneryLayout::SandbarVariant ? 1 : 0;
        }
        Check(nearest < 600.0f && farthest > 950.0f && sandbars == 1,
              "distant islets step from a nearer wooded islet to far silhouettes with one sandbar");
    }
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
        const bool sandbar = islet.variant == island::DistantSceneryLayout::SandbarVariant;
        Check(islet.radiusX >= 24.0f && islet.radiusX <= 42.0f
                  && islet.radiusZ >= 17.0f && islet.radiusZ <= 29.0f
                  && (sandbar ? (islet.height >= 1.5f && islet.height <= 3.0f)
                              : (islet.height >= 5.9f && islet.height <= 10.5f))
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
                  && bird.species == repeated.species
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
    Check(std::ranges::count_if(birds.Birds(), [](const auto& bird) {
              return bird.species == island::BirdSpecies::Parrot;
          }) == 4
              && std::ranges::count_if(birds.Birds(), [](const auto& bird) {
                     return bird.species == island::BirdSpecies::FruitDove;
                 }) == 4,
          "canopy flock retains four parrots and four tropical fruit doves");
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

    const std::array<island::BirdPerchSite, 8> testPerches = {{
        {80.0f, terrain.Height(80.0f, 0.0f) + 10.0f, 0.0f, 0.2f,
         island::BirdPerchKind::Branch},
        {0.0f, terrain.Height(0.0f, 80.0f) + 10.0f, 80.0f, 1.7f,
         island::BirdPerchKind::Branch},
        {-80.0f, terrain.Height(-80.0f, 0.0f) + 10.0f, 0.0f, -2.8f,
         island::BirdPerchKind::Branch},
        {0.0f, terrain.Height(0.0f, -80.0f) + 10.0f, -80.0f, -1.4f,
         island::BirdPerchKind::Branch},
        {57.0f, terrain.Height(57.0f, 57.0f) + 10.0f, 57.0f, 0.8f,
         island::BirdPerchKind::Branch},
        {205.0f, terrain.Height(205.0f, 0.0f) + 2.0f, 0.0f, 1.1f,
         island::BirdPerchKind::Rock},
        {-205.0f, terrain.Height(-205.0f, 0.0f) + 2.0f, 0.0f, -1.9f,
         island::BirdPerchKind::Rock},
        {0.0f, terrain.Height(0.0f, 205.0f) + 5.0f, 205.0f, -0.7f,
         island::BirdPerchKind::Wreck},
    }};
    birds.SetPerchSites(testPerches);
    repeatedBirds.SetPerchSites(testPerches);

    float maximumObservedBirdRadius = 0.0f;
    float minimumObservedBirdRadius = 1000.0f;
    bool observedWingUp = false;
    bool observedWingDown = false;
    bool observedStrongGlide = false;
    bool observedLeftBank = false;
    bool observedRightBank = false;
    bool observedBirdApproach = false;
    bool observedPerchedBird = false;
    bool observedBirdDeparture = false;
    bool observedFoldedPerchWings = false;
    bool observedGroundDoveApproach = false;
    bool observedGroundDoveForage = false;
    bool observedGroundDoveDeparture = false;
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
                      && bird.perchAmount == repeated.perchAmount
                      && bird.behavior == repeated.behavior
                      && bird.perchSiteIndex == repeated.perchSiteIndex
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
            if (bird.behavior == island::BirdBehavior::Fly) {
                Check(bird.y >= terrain.Height(bird.x, bird.z) + 14.8f,
                      "free bird flight maintains its minimum terrain clearance");
            } else if (bird.behavior == island::BirdBehavior::GroundApproach
                       || bird.behavior == island::BirdBehavior::GroundForage
                       || bird.behavior == island::BirdBehavior::GroundDepart) {
                Check(bird.species == island::BirdSpecies::FruitDove,
                      "only fruit doves enter the dedicated ground-foraging states");
                Check(bird.y >= terrain.Height(bird.x, bird.z)
                          + bird.scale * 0.195f,
                      "ground dove keeps its feet on or above authoritative terrain");
            } else {
                Check(bird.y >= terrain.Height(bird.x, bird.z) + 0.34f,
                      "landing bird remains clear of terrain");
            }
            if (bird.behavior == island::BirdBehavior::Approach) {
                observedBirdApproach = true;
            } else if (bird.behavior == island::BirdBehavior::Perched) {
                observedPerchedBird = true;
                const auto& site = testPerches[bird.perchSiteIndex];
                CheckNear(bird.x, site.x, 0.0001f,
                          "perched bird remains on the selected support x coordinate");
                CheckNear(bird.z, site.z, 0.0001f,
                          "perched bird remains on the selected support z coordinate");
                CheckNear(bird.y, site.y + bird.scale * 0.12f, 0.0001f,
                          "perched bird body places its feet on the support surface");
                observedFoldedPerchWings = observedFoldedPerchWings
                    || (bird.perchAmount > 0.999f && std::abs(bird.wingPose) < 0.04f);
            } else if (bird.behavior == island::BirdBehavior::Depart) {
                observedBirdDeparture = true;
            } else if (bird.behavior == island::BirdBehavior::GroundApproach) {
                observedGroundDoveApproach = true;
            } else if (bird.behavior == island::BirdBehavior::GroundForage) {
                observedGroundDoveForage = true;
                Check(bird.perchAmount > 0.999f && std::abs(bird.wingPose) < 0.03f,
                      "foraging ground dove walks with folded wings");
            } else if (bird.behavior == island::BirdBehavior::GroundDepart) {
                observedGroundDoveDeparture = true;
            }
            observedWingUp = observedWingUp || bird.wingPose > 0.55f;
            observedWingDown = observedWingDown || bird.wingPose < -0.55f;
            observedStrongGlide = observedStrongGlide || bird.glideAmount > 0.92f;
            observedLeftBank = observedLeftBank || bird.bankRadians < -0.055f;
            observedRightBank = observedRightBank || bird.bankRadians > 0.055f;
        }
        Check(birds.ApproachingCount() + birds.PerchedCount() + birds.DepartingCount() <= 2,
              "bird landing limits the flock to two simultaneous non-flight agents");
    }
    Check(minimumObservedBirdRadius > 35.0f && maximumObservedBirdRadius < 315.0f,
          "all bird route families remain in the authored island airspace");
    Check(observedWingUp && observedWingDown,
          "bird animation visits both wing stroke extremes");
    Check(observedStrongGlide,
          "bird animation enters sustained intermittent glide periods");
    Check(observedLeftBank && observedRightBank,
          "opposed route directions produce visible banking in both directions");
    Check(observedBirdApproach && observedPerchedBird && observedBirdDeparture
              && observedFoldedPerchWings,
          "bird simulation completes approach, contacted perch and departure states");
    Check(observedGroundDoveApproach && observedGroundDoveForage
              && observedGroundDoveDeparture,
          "fruit doves complete approach, ground forage and departure states");
    Check(birds.FullSimulationCount() + birds.ReducedSimulationCount()
              + birds.DormantSimulationCount() == static_cast<int>(island::BirdFlock::BirdCount),
          "bird simulation tiers account for the complete flock");

    island::ButterflyMeadow butterflies(terrain);
    island::ButterflyMeadow repeatedButterflies(terrain);
    island::ButterflyMeadow invalidInputButterflies(terrain);
    Check(butterflies.Butterflies().size() == island::ButterflyMeadow::ButterflyCount
              && butterflies.Homes().size() == island::ButterflyMeadow::ButterflyCount
              && butterflies.FlowerPatches().size()
                  == island::ButterflyMeadow::HabitatCount,
          "butterfly simulation exposes the intended bounded population and habitats");
    for (std::size_t patchIndex = 0;
         patchIndex < butterflies.FlowerPatches().size(); ++patchIndex) {
        const auto& patch = butterflies.FlowerPatches()[patchIndex];
        const auto& repeatedPatch = repeatedButterflies.FlowerPatches()[patchIndex];
        const auto sample = terrain.Sample(patch.x, patch.z);
        Check(patch.x == repeatedPatch.x && patch.y == repeatedPatch.y
                  && patch.z == repeatedPatch.z && patch.radius == repeatedPatch.radius
                  && patch.groundSlopeX == repeatedPatch.groundSlopeX
                  && patch.groundSlopeZ == repeatedPatch.groundSlopeZ,
              "butterfly nectar patches derive deterministically");
        Check(std::isfinite(patch.x) && std::isfinite(patch.y)
                  && std::isfinite(patch.z) && patch.radius >= 1.8f
                  && patch.radius <= 2.25f && std::isfinite(patch.groundSlopeX)
                  && std::isfinite(patch.groundSlopeZ)
                  && std::fabs(patch.y - sample.height) < 0.001f,
              "visible nectar flowers are grounded inside each clearing habitat");
        for (std::size_t member = 0; member < 6U; ++member) {
            const auto& home = butterflies.Homes()[patchIndex * 6U + member];
            Check(home.x == patch.x && home.z == patch.z,
                  "each butterfly home is anchored to its concrete nectar patch");
        }
    }
    for (std::size_t index = 0; index < butterflies.Butterflies().size(); ++index) {
        const island::ButterflyState& butterfly = butterflies.Butterflies()[index];
        const island::ButterflyState& repeated = repeatedButterflies.Butterflies()[index];
        const island::ButterflyHomeRange& home = butterflies.Homes()[index];
        const island::TerrainSample sample = terrain.Sample(butterfly.x, butterfly.z);
        Check(butterfly.x == repeated.x && butterfly.y == repeated.y
                  && butterfly.z == repeated.z
                  && butterfly.headingRadians == repeated.headingRadians
                  && butterfly.behavior == repeated.behavior
                  && butterfly.colorVariant == repeated.colorVariant,
              "butterfly construction is deterministic");
        Check(std::hypot(butterfly.x - home.x, butterfly.z - home.z) <= home.radius
                  && home.radius >= 7.5f && home.radius <= 11.5f,
              "each butterfly begins inside one bounded clearing habitat");
        Check(sample.signedShoreDistance
                      >= island::ButterflyMeadow::MinimumInlandDistance
                  && sample.slopeRadians < 0.48f
                  && (sample.surface == island::TerrainSurface::CoastalGround
                      || sample.surface == island::TerrainSurface::InteriorGround),
              "each butterfly begins above suitable non-beach vegetation ground");
        Check(butterfly.y >= sample.height + 0.14f
                  && butterfly.y <= sample.height + 1.8f,
              "each butterfly begins inside the low-altitude presentation envelope");
    }
    invalidInputButterflies.Update(quietNan, quietNan, quietNan, quietNan);
    Check(invalidInputButterflies.DormantSimulationCount()
              == static_cast<int>(island::ButterflyMeadow::ButterflyCount)
              && std::ranges::all_of(
                  invalidInputButterflies.Butterflies(), [](const auto& butterfly) {
                      return std::isfinite(butterfly.x) && std::isfinite(butterfly.y)
                          && std::isfinite(butterfly.z)
                          && std::isfinite(butterfly.headingRadians);
                  }),
          "butterflies reject non-finite frame and observer input without state poisoning");
    bool observedButterflyRest = false;
    bool observedButterflyFlutter = false;
    bool observedButterflyHover = false;
    for (int frame = 0; frame < 12000; ++frame) {
        butterflies.Update(1.0f / 30.0f, 0.0f, 100.0f, 0.0f);
        repeatedButterflies.Update(1.0f / 30.0f, 0.0f, 100.0f, 0.0f);
        for (std::size_t index = 0; index < butterflies.Butterflies().size(); ++index) {
            const island::ButterflyState& butterfly = butterflies.Butterflies()[index];
            const island::ButterflyState& repeated =
                repeatedButterflies.Butterflies()[index];
            const island::ButterflyHomeRange& home = butterflies.Homes()[index];
            const island::TerrainSample sample = terrain.Sample(butterfly.x, butterfly.z);
            Check(butterfly.x == repeated.x && butterfly.y == repeated.y
                      && butterfly.z == repeated.z
                      && butterfly.headingRadians == repeated.headingRadians
                      && butterfly.wingFold == repeated.wingFold
                      && butterfly.behavior == repeated.behavior,
                  "butterfly simulation remains deterministic over a long session");
            Check(std::hypot(butterfly.x - home.x, butterfly.z - home.z)
                          <= home.radius + 0.01f
                      && sample.signedShoreDistance
                          >= island::ButterflyMeadow::MinimumInlandDistance - 0.01f
                      && sample.slopeRadians < 0.481f
                      && butterfly.y >= sample.height + 0.13f
                      && butterfly.y <= sample.height + 1.81f,
                  "butterflies remain in bounded low-altitude clearing habitats");
            Check(std::isfinite(butterfly.speed) && butterfly.speed >= 0.0f
                      && std::isfinite(butterfly.wingFold)
                      && butterfly.wingFold >= 0.0f && butterfly.wingFold <= 1.0f,
                  "butterfly movement and wing pose remain finite and bounded");
            observedButterflyRest = observedButterflyRest
                || butterfly.behavior == island::ButterflyBehavior::Rest;
            observedButterflyFlutter = observedButterflyFlutter
                || butterfly.behavior == island::ButterflyBehavior::Flutter;
            observedButterflyHover = observedButterflyHover
                || butterfly.behavior == island::ButterflyBehavior::NectarHover;
        }
    }
    Check(observedButterflyRest && observedButterflyFlutter && observedButterflyHover,
          "butterfly habitats reach rest, flutter, and nectar-hover activity states");
    Check(butterflies.FullSimulationCount() + butterflies.ReducedSimulationCount()
              + butterflies.DormantSimulationCount()
              == static_cast<int>(island::ButterflyMeadow::ButterflyCount),
          "butterfly simulation tiers account for the complete population");
    island::ButterflyMeadow reactiveButterflies(terrain);
    const island::ButterflyState initialReactiveButterfly =
        reactiveButterflies.Butterflies().front();
    for (int frame = 0; frame < 120; ++frame) {
        reactiveButterflies.Update(
            1.0f / 60.0f, initialReactiveButterfly.x, initialReactiveButterfly.y,
            initialReactiveButterfly.z);
    }
    const island::ButterflyState& escapedButterfly =
        reactiveButterflies.Butterflies().front();
    Check(escapedButterfly.behavior == island::ButterflyBehavior::Flutter
              && std::hypot(escapedButterfly.x - initialReactiveButterfly.x,
                            escapedButterfly.z - initialReactiveButterfly.z) > 0.25f
              && escapedButterfly.y > terrain.Height(
                     escapedButterfly.x, escapedButterfly.z) + 0.35f,
          "a nearby observer prompts a mild airborne butterfly retreat");

    island::GeckoPopulation geckos(terrain);
    island::GeckoPopulation repeatedGeckos(terrain);
    island::GeckoPopulation invalidInputGeckos(terrain);
    Check(geckos.Geckos().size() == island::GeckoPopulation::Count
              && geckos.Homes().size() == island::GeckoPopulation::Count,
          "gecko simulation exposes eight bounded inland animals");
    for (std::size_t index=0;index<geckos.Geckos().size();++index) {
        const auto& gecko=geckos.Geckos()[index];
        const auto& repeated=repeatedGeckos.Geckos()[index];
        const auto& home=geckos.Homes()[index];
        const auto sample=terrain.Sample(gecko.x,gecko.z);
        Check(gecko.x==repeated.x && gecko.y==repeated.y && gecko.z==repeated.z
                  && gecko.headingRadians==repeated.headingRadians
                  && gecko.behavior==repeated.behavior
                  && gecko.colorVariant==repeated.colorVariant,
              "gecko construction is deterministic");
        Check(std::hypot(gecko.x-home.x,gecko.z-home.z)<=home.radius
                  && home.radius>=5.2f && home.radius<=6.2f,
              "each gecko begins inside its compact home range");
        Check(sample.signedShoreDistance>=34.0f && sample.slopeRadians<.52f
                  && (sample.surface==island::TerrainSurface::CoastalGround
                      || sample.surface==island::TerrainSurface::InteriorGround
                      || sample.surface==island::TerrainSurface::RockyGround)
                  && std::fabs(gecko.y-sample.height-.038f)<.001f,
              "each gecko begins grounded in suitable inland habitat");
    }
    invalidInputGeckos.Update(quietNan,quietNan,quietNan,quietNan);
    Check(invalidInputGeckos.DormantSimulationCount()
                  ==static_cast<int>(island::GeckoPopulation::Count)
              && std::ranges::all_of(invalidInputGeckos.Geckos(),[](const auto& gecko) {
                     return std::isfinite(gecko.x)&&std::isfinite(gecko.y)
                         &&std::isfinite(gecko.z)&&std::isfinite(gecko.headingRadians)
                         &&std::isfinite(gecko.visibility);
                 }),
          "geckos reject non-finite frame and observer input without state poisoning");
    bool observedGeckoRest=false;
    bool observedGeckoScurry=false;
    bool observedGeckoHidden=false;
    bool observedInvisibleGecko=false;
    for(int frame=0;frame<4000;++frame) {
        geckos.Update(1.0f/30.0f,0.0f,100.0f,0.0f);
        repeatedGeckos.Update(1.0f/30.0f,0.0f,100.0f,0.0f);
        for(std::size_t index=0;index<geckos.Geckos().size();++index) {
            const auto& gecko=geckos.Geckos()[index];
            const auto& repeated=repeatedGeckos.Geckos()[index];
            const auto& home=geckos.Homes()[index];
            const auto sample=terrain.Sample(gecko.x,gecko.z);
            Check(gecko.x==repeated.x && gecko.y==repeated.y && gecko.z==repeated.z
                      && gecko.headingRadians==repeated.headingRadians
                      && gecko.visibility==repeated.visibility
                      && gecko.behavior==repeated.behavior,
                  "gecko simulation remains deterministic over a long session");
            Check(std::hypot(gecko.x-home.x,gecko.z-home.z)<=home.radius+.01f
                      && sample.signedShoreDistance>=33.99f
                      && sample.slopeRadians<.521f
                      && std::fabs(gecko.y-sample.height-.038f)<.002f,
                  "geckos remain grounded in bounded inland habitats");
            Check(std::isfinite(gecko.speed)&&gecko.speed>=0.0f
                      && gecko.visibility>=0.0f&&gecko.visibility<=1.0f,
                  "gecko motion and visibility remain finite and bounded");
            observedGeckoRest|=gecko.behavior==island::GeckoBehavior::Rest;
            observedGeckoScurry|=gecko.behavior==island::GeckoBehavior::Scurry;
            observedGeckoHidden|=gecko.behavior==island::GeckoBehavior::Hidden;
            observedInvisibleGecko|=gecko.visibility<.04f;
        }
    }
    Check(observedGeckoRest&&observedGeckoScurry&&observedGeckoHidden
              &&observedInvisibleGecko,
          "geckos complete visible rest, scurry, and hidden activity states");
    Check(geckos.FullSimulationCount()+geckos.ReducedSimulationCount()
                  +geckos.DormantSimulationCount()
              ==static_cast<int>(island::GeckoPopulation::Count),
          "gecko simulation tiers account for the complete population");
    island::GeckoPopulation reactiveGeckos(terrain);
    const auto initialReactiveGecko=reactiveGeckos.Geckos()[1U];
    for(int frame=0;frame<75;++frame) {
        reactiveGeckos.Update(1.0f/60.0f,initialReactiveGecko.x,
                             initialReactiveGecko.y,initialReactiveGecko.z);
    }
    const auto& hiddenGecko=reactiveGeckos.Geckos()[1U];
    Check(hiddenGecko.behavior==island::GeckoBehavior::Hidden
              && hiddenGecko.visibility<.3f
              && std::hypot(hiddenGecko.x-initialReactiveGecko.x,
                            hiddenGecko.z-initialReactiveGecko.z)>.2f,
          "a nearby observer prompts a gecko to scurry away and hide");

    island::SkinkPopulation skinks(terrain);
    island::SkinkPopulation repeatedSkinks(terrain);
    island::SkinkPopulation invalidInputSkinks(terrain);
    Check(skinks.Skinks().size()==island::SkinkPopulation::Count
              &&skinks.Homes().size()==island::SkinkPopulation::Count,
          "skink simulation exposes eight bounded open-ground animals");
    for(std::size_t index=0;index<skinks.Skinks().size();++index) {
        const auto& skink=skinks.Skinks()[index];
        const auto& repeated=repeatedSkinks.Skinks()[index];
        const auto& home=skinks.Homes()[index];
        const auto sample=terrain.Sample(skink.x,skink.z);
        Check(skink.x==repeated.x&&skink.y==repeated.y&&skink.z==repeated.z
                  &&skink.headingRadians==repeated.headingRadians
                  &&skink.behavior==repeated.behavior
                  &&skink.colorVariant==repeated.colorVariant,
              "skink construction is deterministic");
        Check(std::hypot(skink.x-home.x,skink.z-home.z)<=home.radius
                  &&home.radius>=6.4f&&home.radius<=7.51f,
              "each skink begins inside its compact home range");
        Check(sample.height>.15f&&sample.signedShoreDistance>=34.0f
                  &&sample.slopeRadians<.52f
                  &&std::fabs(skink.y-sample.height-.034f)<.001f,
              "each skink begins grounded in open inland habitat");
        Check(std::ranges::all_of(geckos.Homes(),[&](const auto& geckoHome) {
                  return std::hypot(home.x-geckoHome.x,home.z-geckoHome.z)>=4.0f;
              }),
              "skink homes remain visually separated from gecko homes");
    }
    invalidInputSkinks.Update(quietNan,quietNan,quietNan,quietNan);
    Check(invalidInputSkinks.DormantSimulationCount()
                  ==static_cast<int>(island::SkinkPopulation::Count)
              &&std::ranges::all_of(invalidInputSkinks.Skinks(),[](const auto& skink) {
                     return std::isfinite(skink.x)&&std::isfinite(skink.y)
                         &&std::isfinite(skink.z)&&std::isfinite(skink.headingRadians)
                         &&std::isfinite(skink.gaitPhase)&&std::isfinite(skink.breathPhase);
                 }),
          "skinks reject non-finite frame and observer input without state poisoning");
    bool observedSkinkBask=false;
    bool observedSkinkForage=false;
    for(int frame=0;frame<3000;++frame) {
        skinks.Update(1.0f/30.0f,0.0f,100.0f,0.0f);
        repeatedSkinks.Update(1.0f/30.0f,0.0f,100.0f,0.0f);
        for(std::size_t index=0;index<skinks.Skinks().size();++index) {
            const auto& skink=skinks.Skinks()[index];
            const auto& repeated=repeatedSkinks.Skinks()[index];
            const auto& home=skinks.Homes()[index];
            const auto sample=terrain.Sample(skink.x,skink.z);
            Check(skink.x==repeated.x&&skink.y==repeated.y&&skink.z==repeated.z
                      &&skink.headingRadians==repeated.headingRadians
                      &&skink.gaitPhase==repeated.gaitPhase
                      &&skink.behavior==repeated.behavior,
                  "skink simulation remains deterministic over a long session");
            Check(std::hypot(skink.x-home.x,skink.z-home.z)<=home.radius+.01f
                      &&sample.signedShoreDistance>=33.99f
                      &&sample.slopeRadians<.521f
                      &&std::fabs(skink.y-sample.height-.034f)<.002f,
                  "skinks remain grounded in bounded open habitats");
            Check(std::isfinite(skink.speed)&&skink.speed>=0.0f
                      &&std::isfinite(skink.gaitPhase)&&std::isfinite(skink.breathPhase),
                  "skink movement phases remain finite");
            observedSkinkBask|=skink.behavior==island::SkinkBehavior::Bask;
            observedSkinkForage|=skink.behavior==island::SkinkBehavior::Forage;
        }
    }
    Check(observedSkinkBask&&observedSkinkForage,
          "skinks complete bask and forage activity states");
    Check(skinks.FullSimulationCount()+skinks.ReducedSimulationCount()
                  +skinks.DormantSimulationCount()
              ==static_cast<int>(island::SkinkPopulation::Count),
          "skink simulation tiers account for the complete population");
    island::SkinkPopulation reactiveSkinks(terrain);
    const auto initialReactiveSkink=reactiveSkinks.Skinks()[1U];
    bool observedSkinkFlee=false;
    for(int frame=0;frame<70;++frame) {
        reactiveSkinks.Update(1.0f/60.0f,initialReactiveSkink.x,
                              initialReactiveSkink.y,initialReactiveSkink.z);
        observedSkinkFlee|=reactiveSkinks.Skinks()[1U].behavior
            ==island::SkinkBehavior::Flee;
    }
    const auto& escapedSkink=reactiveSkinks.Skinks()[1U];
    Check(observedSkinkFlee
              &&std::hypot(escapedSkink.x-initialReactiveSkink.x,
                            escapedSkink.z-initialReactiveSkink.z)>.25f,
          "a nearby observer prompts a skink to flee across the ground");

    island::DragonflySwarm dragonflies(terrain);
    island::DragonflySwarm repeatedDragonflies(terrain);
    island::DragonflySwarm invalidInputDragonflies(terrain);
    Check(dragonflies.Dragonflies().size()==island::DragonflySwarm::Count
              &&dragonflies.Homes().size()==island::DragonflySwarm::Count,
          "dragonfly simulation exposes nine bounded wet-habitat animals");
    for(std::size_t index=0;index<dragonflies.Dragonflies().size();++index) {
        const auto& dragonfly=dragonflies.Dragonflies()[index];
        const auto& repeated=repeatedDragonflies.Dragonflies()[index];
        const auto& home=dragonflies.Homes()[index];
        const auto sample=terrain.Sample(dragonfly.x,dragonfly.z);
        const auto inland=island::IslandEnvironmentField::SampleInland(
            dragonfly.x,dragonfly.z,sample.signedShoreDistance);
        Check(dragonfly.x==repeated.x&&dragonfly.y==repeated.y
                  &&dragonfly.z==repeated.z
                  &&dragonfly.headingRadians==repeated.headingRadians
                  &&dragonfly.behavior==repeated.behavior
                  &&dragonfly.colorVariant==repeated.colorVariant,
              "dragonfly construction is deterministic");
        Check(std::hypot(dragonfly.x-home.x,dragonfly.z-home.z)<=home.radius
                  &&home.radius>=8.5f&&home.radius<=10.5f,
              "each dragonfly begins inside a compact stream or wetland range");
        Check(sample.height>0.0f&&sample.signedShoreDistance>12.0f
                  &&sample.slopeRadians<.55f&&inland.moisture>.34f
                  &&dragonfly.y>=sample.height+.70f
                  &&dragonfly.y<=sample.height+1.22f,
              "each dragonfly begins above suitable moist inland habitat");
        Check(dragonfly.scale>=.169f&&dragonfly.scale<=.191f,
              "dragonflies retain a plausible sub-ten-centimetre body scale");
    }
    invalidInputDragonflies.Update(quietNan,quietNan,quietNan,quietNan);
    Check(invalidInputDragonflies.DormantSimulationCount()
                  ==static_cast<int>(island::DragonflySwarm::Count)
              &&std::ranges::all_of(invalidInputDragonflies.Dragonflies(),
                    [](const auto& dragonfly) {
                        return std::isfinite(dragonfly.x)
                            &&std::isfinite(dragonfly.y)
                            &&std::isfinite(dragonfly.z)
                            &&std::isfinite(dragonfly.headingRadians)
                            &&std::isfinite(dragonfly.wingPhase);
                    }),
          "dragonflies reject non-finite frame and observer input without state poisoning");
    bool observedDragonflyHover=false;
    bool observedDragonflyPatrol=false;
    bool observedDragonflyDart=false;
    for(int frame=0;frame<4000;++frame) {
        dragonflies.Update(1.0f/30.0f,0.0f,100.0f,0.0f);
        repeatedDragonflies.Update(1.0f/30.0f,0.0f,100.0f,0.0f);
        for(std::size_t index=0;index<dragonflies.Dragonflies().size();++index) {
            const auto& dragonfly=dragonflies.Dragonflies()[index];
            const auto& repeated=repeatedDragonflies.Dragonflies()[index];
            const auto& home=dragonflies.Homes()[index];
            const auto sample=terrain.Sample(dragonfly.x,dragonfly.z);
            const auto inland=island::IslandEnvironmentField::SampleInland(
                dragonfly.x,dragonfly.z,sample.signedShoreDistance);
            Check(dragonfly.x==repeated.x&&dragonfly.y==repeated.y
                      &&dragonfly.z==repeated.z
                      &&dragonfly.headingRadians==repeated.headingRadians
                      &&dragonfly.wingPhase==repeated.wingPhase
                      &&dragonfly.behavior==repeated.behavior,
                  "dragonfly simulation remains deterministic over a long session");
            Check(std::hypot(dragonfly.x-home.x,dragonfly.z-home.z)<=home.radius+.01f
                      &&sample.height>0.0f&&sample.signedShoreDistance>12.0f
                      &&sample.slopeRadians<.55f&&inland.moisture>.34f
                      &&dragonfly.y>=sample.height+.45f
                      &&dragonfly.y<=sample.height+1.42f,
                  "dragonflies remain airborne in bounded moist habitats");
            Check(std::isfinite(dragonfly.speed)&&dragonfly.speed>=0.0f
                      &&std::isfinite(dragonfly.wingPhase)
                      &&std::isfinite(dragonfly.bankRadians),
                  "dragonfly flight state remains finite");
            observedDragonflyHover|=
                dragonfly.behavior==island::DragonflyBehavior::Hover;
            observedDragonflyPatrol|=
                dragonfly.behavior==island::DragonflyBehavior::Patrol;
            observedDragonflyDart|=
                dragonfly.behavior==island::DragonflyBehavior::Dart;
        }
    }
    Check(observedDragonflyHover&&observedDragonflyPatrol&&observedDragonflyDart,
          "dragonflies complete hover, patrol, and dart activity states");
    Check(dragonflies.FullSimulationCount()+dragonflies.ReducedSimulationCount()
                  +dragonflies.DormantSimulationCount()
              ==static_cast<int>(island::DragonflySwarm::Count),
          "dragonfly simulation tiers account for the complete population");
    island::DragonflySwarm reactiveDragonflies(terrain);
    const auto initialReactiveDragonfly=reactiveDragonflies.Dragonflies()[1U];
    bool reactiveDart=false;
    for(int frame=0;frame<50;++frame) {
        reactiveDragonflies.Update(1.0f/60.0f,initialReactiveDragonfly.x,
                                   initialReactiveDragonfly.y,
                                   initialReactiveDragonfly.z);
        reactiveDart|=reactiveDragonflies.Dragonflies()[1U].behavior
            ==island::DragonflyBehavior::Dart;
    }
    const auto& escapedDragonfly=reactiveDragonflies.Dragonflies()[1U];
    Check(reactiveDart
              &&std::hypot(escapedDragonfly.x-initialReactiveDragonfly.x,
                            escapedDragonfly.z-initialReactiveDragonfly.z)>.25f,
          "a nearby observer prompts a short dragonfly escape dart");

    island::FrogPopulation frogs(terrain);
    island::FrogPopulation repeatedFrogs(terrain);
    island::FrogPopulation invalidInputFrogs(terrain);
    Check(frogs.Frogs().size()==island::FrogPopulation::Count
              &&frogs.Homes().size()==island::FrogPopulation::Count,
          "frog population and homes have exact deterministic counts");
    for(std::size_t index=0;index<frogs.Frogs().size();++index) {
        const auto& frog=frogs.Frogs()[index];
        const auto& repeated=repeatedFrogs.Frogs()[index];
        const auto& home=frogs.Homes()[index];
        const auto sample=terrain.Sample(frog.x,frog.z);
        const auto inland=island::IslandEnvironmentField::SampleInland(
            frog.x,frog.z,sample.signedShoreDistance);
        Check(std::isfinite(frog.x)&&std::isfinite(frog.y)&&std::isfinite(frog.z)
                  &&std::hypot(frog.x-home.x,frog.z-home.z)<=home.radius+.001f,
              "frogs start finite inside bounded homes");
        Check(std::abs(frog.y-terrain.Height(frog.x,frog.z)-.045f)<.001f
                  &&inland.moisture>.52f&&sample.slopeRadians<.48f,
              "frogs start grounded in moist stream or wetland habitat");
        Check(frog.x==repeated.x&&frog.y==repeated.y&&frog.z==repeated.z
                  &&frog.headingRadians==repeated.headingRadians
                  &&frog.colorVariant==repeated.colorVariant,
              "frog construction is deterministic");
    }
    invalidInputFrogs.Update(quietNan,quietNan,quietNan,quietNan);
    Check(invalidInputFrogs.DormantSimulationCount()
                  ==static_cast<int>(island::FrogPopulation::Count),
          "frogs reject non-finite frame and observer input without state poisoning");
    bool observedFrogRest=false,observedFrogHop=false;
    for(int frame=0;frame<1800;++frame) {
        frogs.Update(1.0f/30.0f,0.0f,100.0f,0.0f);
        repeatedFrogs.Update(1.0f/30.0f,0.0f,100.0f,0.0f);
        for(std::size_t index=0;index<frogs.Frogs().size();++index) {
            const auto& frog=frogs.Frogs()[index];
            const auto& repeated=repeatedFrogs.Frogs()[index];
            const auto& home=frogs.Homes()[index];
            observedFrogRest|=frog.behavior==island::FrogBehavior::Rest;
            observedFrogHop|=frog.behavior==island::FrogBehavior::Hop;
            Check(std::hypot(frog.x-home.x,frog.z-home.z)<=home.radius+.02f
                      &&std::isfinite(frog.y)&&frog.y>=terrain.Height(frog.x,frog.z)+.044f,
                  "frogs stay finite, above ground, and inside wet homes");
            Check(frog.x==repeated.x&&frog.y==repeated.y&&frog.z==repeated.z
                      &&frog.behavior==repeated.behavior,
                  "frog rest and hopping remain deterministic");
        }
    }
    Check(observedFrogRest&&observedFrogHop,
          "frogs complete both resting and ordinary hopping states");
    island::FrogPopulation reactiveFrogs(terrain);
    const auto initialReactiveFrog=reactiveFrogs.Frogs()[1U];
    bool observedFrogFlee=false;
    for(int frame=0;frame<120;++frame) {
        reactiveFrogs.Update(1.0f/60.0f,initialReactiveFrog.x,
                            initialReactiveFrog.y,initialReactiveFrog.z);
        observedFrogFlee|=reactiveFrogs.Frogs()[1U].behavior
            ==island::FrogBehavior::Flee;
    }
    Check(observedFrogFlee
              &&std::hypot(reactiveFrogs.Frogs()[1U].x-initialReactiveFrog.x,
                            reactiveFrogs.Frogs()[1U].z-initialReactiveFrog.z)>.20f,
          "nearby observer triggers a visible bounded frog retreat");

    island::HermitCrabGroup hermitCrabs(terrain);
    island::HermitCrabGroup repeatedHermitCrabs(terrain);
    island::HermitCrabGroup invalidInputHermitCrabs(terrain);
    Check(hermitCrabs.Crabs().size()==island::HermitCrabGroup::Count
              &&hermitCrabs.Homes().size()==island::HermitCrabGroup::Count,
          "hermit-crab simulation exposes ten bounded shell-bearing animals");
    for(std::size_t index=0;index<hermitCrabs.Crabs().size();++index) {
        const auto& crab=hermitCrabs.Crabs()[index];
        const auto& repeated=repeatedHermitCrabs.Crabs()[index];
        const auto& home=hermitCrabs.Homes()[index];
        const auto sample=terrain.Sample(crab.x,crab.z);
        Check(crab.x==repeated.x&&crab.y==repeated.y&&crab.z==repeated.z
                  &&crab.headingRadians==repeated.headingRadians
                  &&crab.behavior==repeated.behavior
                  &&crab.shellVariant==repeated.shellVariant,
              "hermit-crab construction is deterministic");
        Check(std::hypot(crab.x-home.x,crab.z-home.z)<=home.radius
                  &&home.radius>=3.8f&&home.radius<=4.71f,
              "each hermit crab begins inside a compact beach home range");
        Check(sample.height>=-.01f
                  &&sample.signedShoreDistance>=island::HermitCrabGroup::MinimumShoreDistance
                  &&sample.signedShoreDistance<=island::HermitCrabGroup::MaximumShoreDistance
                  &&sample.slopeRadians<.34f
                  &&std::fabs(crab.y-sample.height-.038f)<.001f,
              "each hermit crab begins grounded on qualified sandy coast");
    }
    invalidInputHermitCrabs.Update(quietNan,quietNan,quietNan,quietNan);
    Check(invalidInputHermitCrabs.DormantSimulationCount()
                  ==static_cast<int>(island::HermitCrabGroup::Count)
              &&std::ranges::all_of(invalidInputHermitCrabs.Crabs(),[](const auto& crab) {
                     return std::isfinite(crab.x)&&std::isfinite(crab.y)
                         &&std::isfinite(crab.z)&&std::isfinite(crab.headingRadians)
                         &&std::isfinite(crab.legPhase)&&std::isfinite(crab.antennaPhase);
                 }),
          "hermit crabs reject non-finite frame and observer input without state poisoning");
    bool observedHermitPause=false;
    bool observedHermitForage=false;
    for(int frame=0;frame<4000;++frame) {
        hermitCrabs.Update(1.0f/30.0f,0.0f,100.0f,0.0f);
        repeatedHermitCrabs.Update(1.0f/30.0f,0.0f,100.0f,0.0f);
        for(std::size_t index=0;index<hermitCrabs.Crabs().size();++index) {
            const auto& crab=hermitCrabs.Crabs()[index];
            const auto& repeated=repeatedHermitCrabs.Crabs()[index];
            const auto& home=hermitCrabs.Homes()[index];
            const auto sample=terrain.Sample(crab.x,crab.z);
            Check(crab.x==repeated.x&&crab.y==repeated.y&&crab.z==repeated.z
                      &&crab.headingRadians==repeated.headingRadians
                      &&crab.legPhase==repeated.legPhase
                      &&crab.behavior==repeated.behavior,
                  "hermit-crab simulation remains deterministic over a long session");
            Check(std::hypot(crab.x-home.x,crab.z-home.z)<=home.radius+.01f
                      &&sample.height>=-.01f
                      &&sample.signedShoreDistance>=island::HermitCrabGroup::MinimumShoreDistance
                      &&sample.signedShoreDistance<=island::HermitCrabGroup::MaximumShoreDistance
                      &&sample.slopeRadians<.34f
                      &&std::fabs(crab.y-sample.height-.038f)<.002f,
                  "hermit crabs remain grounded inside sandy coastal habitats");
            Check(std::isfinite(crab.speed)&&crab.speed>=0.0f
                      &&std::isfinite(crab.legPhase)&&std::fabs(crab.legPhase)<=1.001f,
                  "hermit-crab movement and gait remain finite and bounded");
            observedHermitPause|=crab.behavior==island::HermitCrabBehavior::Pause;
            observedHermitForage|=crab.behavior==island::HermitCrabBehavior::Forage;
        }
    }
    Check(observedHermitPause&&observedHermitForage,
          "hermit crabs complete pause and forage activity states");
    Check(hermitCrabs.FullSimulationCount()+hermitCrabs.ReducedSimulationCount()
                  +hermitCrabs.DormantSimulationCount()
              ==static_cast<int>(island::HermitCrabGroup::Count),
          "hermit-crab simulation tiers account for the complete population");
    island::HermitCrabGroup reactiveHermitCrabs(terrain);
    const auto initialReactiveHermit=reactiveHermitCrabs.Crabs()[3U];
    bool observedRetreat=false;
    for(int frame=0;frame<50;++frame) {
        reactiveHermitCrabs.Update(1.0f/60.0f,initialReactiveHermit.x,
                                   initialReactiveHermit.y,initialReactiveHermit.z);
        observedRetreat|=reactiveHermitCrabs.Crabs()[3U].behavior
            ==island::HermitCrabBehavior::Retreat;
    }
    const auto& escapedHermit=reactiveHermitCrabs.Crabs()[3U];
    Check(observedRetreat
              &&std::hypot(escapedHermit.x-initialReactiveHermit.x,
                            escapedHermit.z-initialReactiveHermit.z)>.12f,
          "a nearby observer prompts a hermit crab to retreat with its shell");

    island::CoconutCrabGroup coconutCrabs(terrain);
    island::CoconutCrabGroup repeatedCoconutCrabs(terrain);
    island::CoconutCrabGroup invalidInputCoconutCrabs(terrain);
    Check(coconutCrabs.Crabs().size()==island::CoconutCrabGroup::Count
              &&coconutCrabs.Homes().size()==island::CoconutCrabGroup::Count,
          "coconut-crab simulation exposes six bounded terrestrial animals");
    for(std::size_t index=0;index<coconutCrabs.Crabs().size();++index) {
        const auto& crab=coconutCrabs.Crabs()[index];
        const auto& repeated=repeatedCoconutCrabs.Crabs()[index];
        const auto& home=coconutCrabs.Homes()[index];
        const auto sample=terrain.Sample(crab.x,crab.z);
        Check(crab.x==repeated.x&&crab.y==repeated.y&&crab.z==repeated.z
                  &&crab.headingRadians==repeated.headingRadians
                  &&crab.behavior==repeated.behavior
                  &&crab.colorVariant==repeated.colorVariant,
              "coconut-crab construction is deterministic");
        Check(std::hypot(crab.x-home.x,crab.z-home.z)<=home.radius
                  &&home.radius>=7.5f&&home.radius<=9.11f,
              "each coconut crab begins inside a coastal home range");
        Check(sample.height>.15f
                  &&sample.signedShoreDistance>=island::CoconutCrabGroup::MinimumShoreDistance
                  &&sample.signedShoreDistance<=island::CoconutCrabGroup::MaximumShoreDistance
                  &&sample.slopeRadians<.42f
                  &&std::fabs(crab.y-sample.height-.080f)<.001f,
              "each coconut crab begins grounded in coastal palm-zone terrain");
    }
    invalidInputCoconutCrabs.Update(quietNan,quietNan,quietNan,quietNan);
    Check(invalidInputCoconutCrabs.DormantSimulationCount()
                  ==static_cast<int>(island::CoconutCrabGroup::Count)
              &&std::ranges::all_of(invalidInputCoconutCrabs.Crabs(),[](const auto& crab) {
                     return std::isfinite(crab.x)&&std::isfinite(crab.y)
                         &&std::isfinite(crab.z)&&std::isfinite(crab.headingRadians)
                         &&std::isfinite(crab.legPhase)&&std::isfinite(crab.clawPose);
                 }),
          "coconut crabs reject non-finite frame and observer input without state poisoning");
    bool observedCoconutRest=false;
    bool observedCoconutForage=false;
    for(int frame=0;frame<4000;++frame) {
        coconutCrabs.Update(1.0f/30.0f,0.0f,100.0f,0.0f);
        repeatedCoconutCrabs.Update(1.0f/30.0f,0.0f,100.0f,0.0f);
        for(std::size_t index=0;index<coconutCrabs.Crabs().size();++index) {
            const auto& crab=coconutCrabs.Crabs()[index];
            const auto& repeated=repeatedCoconutCrabs.Crabs()[index];
            const auto& home=coconutCrabs.Homes()[index];
            const auto sample=terrain.Sample(crab.x,crab.z);
            Check(crab.x==repeated.x&&crab.y==repeated.y&&crab.z==repeated.z
                      &&crab.headingRadians==repeated.headingRadians
                      &&crab.legPhase==repeated.legPhase
                      &&crab.behavior==repeated.behavior,
                  "coconut-crab simulation remains deterministic over a long session");
            Check(std::hypot(crab.x-home.x,crab.z-home.z)<=home.radius+.01f
                      &&sample.signedShoreDistance>=island::CoconutCrabGroup::MinimumShoreDistance
                      &&sample.signedShoreDistance<=island::CoconutCrabGroup::MaximumShoreDistance
                      &&sample.slopeRadians<.42f
                      &&std::fabs(crab.y-sample.height-.080f)<.002f,
                  "coconut crabs remain grounded in bounded coastal habitats");
            Check(std::isfinite(crab.speed)&&crab.speed>=0.0f
                      &&std::isfinite(crab.legPhase)&&std::fabs(crab.legPhase)<=1.001f
                      &&crab.clawPose>=.13f&&crab.clawPose<=.51f,
                  "coconut-crab movement and claw state remain finite and bounded");
            observedCoconutRest|=crab.behavior==island::CoconutCrabBehavior::Rest;
            observedCoconutForage|=crab.behavior==island::CoconutCrabBehavior::Forage;
        }
    }
    Check(observedCoconutRest&&observedCoconutForage,
          "coconut crabs complete rest and forage activity states");
    Check(coconutCrabs.FullSimulationCount()+coconutCrabs.ReducedSimulationCount()
                  +coconutCrabs.DormantSimulationCount()
              ==static_cast<int>(island::CoconutCrabGroup::Count),
          "coconut-crab simulation tiers account for the complete population");
    island::CoconutCrabGroup reactiveCoconutCrabs(terrain);
    const auto initialReactiveCoconut=reactiveCoconutCrabs.Crabs()[2U];
    bool observedCoconutRetreat=false;
    for(int frame=0;frame<70;++frame) {
        reactiveCoconutCrabs.Update(1.0f/60.0f,initialReactiveCoconut.x,
                                    initialReactiveCoconut.y,initialReactiveCoconut.z);
        observedCoconutRetreat|=reactiveCoconutCrabs.Crabs()[2U].behavior
            ==island::CoconutCrabBehavior::Retreat;
    }
    const auto& escapedCoconut=reactiveCoconutCrabs.Crabs()[2U];
    Check(observedCoconutRetreat
              &&std::hypot(escapedCoconut.x-initialReactiveCoconut.x,
                            escapedCoconut.z-initialReactiveCoconut.z)>.20f,
          "a nearby observer prompts a coconut crab to retreat");

    island::SeaTurtleGroup seaTurtles(terrain);
    island::SeaTurtleGroup repeatedSeaTurtles(terrain);
    island::SeaTurtleGroup invalidInputSeaTurtles(terrain);
    Check(seaTurtles.Turtles().size() == island::SeaTurtleGroup::TurtleCount
              && seaTurtles.Homes().size() == island::SeaTurtleGroup::TurtleCount,
          "sea-turtle simulation exposes the intended small bounded population");
    std::size_t shelteredBayTurtleHomes = 0;
    for (std::size_t index = 0; index < seaTurtles.Turtles().size(); ++index) {
        const island::SeaTurtleState& turtle = seaTurtles.Turtles()[index];
        const island::SeaTurtleState& repeated = repeatedSeaTurtles.Turtles()[index];
        const island::SeaTurtleHomeRange& home = seaTurtles.Homes()[index];
        const island::TerrainSample sample = terrain.Sample(turtle.x, turtle.z);
        Check(turtle.x == repeated.x && turtle.y == repeated.y
                  && turtle.z == repeated.z
                  && turtle.headingRadians == repeated.headingRadians
                  && turtle.behavior == repeated.behavior
                  && turtle.colorVariant == repeated.colorVariant,
              "sea-turtle construction is deterministic");
        Check(std::hypot(turtle.x - home.x, turtle.z - home.z) <= home.radius
                  && home.radius >= 11.5f && home.radius <= 15.1f,
              "each sea turtle begins inside one bounded near-shore range");
        Check(sample.signedShoreDistance
                      >= island::SeaTurtleGroup::MinimumShoreDistance
                  && sample.signedShoreDistance
                      <= island::SeaTurtleGroup::MaximumShoreDistance
                  && sample.height <= -0.72f,
              "each sea turtle begins in submerged water inside the current limit");
        Check(island::IslandEnvironmentField::AquaticHabitatSuitability(
                  turtle.x, turtle.z, sample.signedShoreDistance, sample.height) >= .025f,
              "each sea turtle begins in shared reef or seagrass habitat");
        Check(turtle.y >= sample.height + 0.39f && turtle.y <= -0.09f,
              "each sea turtle begins between seabed clearance and the water surface");
        if (home.x < 0.0f && std::abs(home.z) < -home.x * .72f) {
            ++shelteredBayTurtleHomes;
        }
    }
    Check(shelteredBayTurtleHomes >= 3,
          "most sea-turtle homes must occupy the calmer sheltered western bay");
    invalidInputSeaTurtles.Update(quietNan, quietNan, quietNan, quietNan);
    Check(invalidInputSeaTurtles.DormantSimulationCount()
              == static_cast<int>(island::SeaTurtleGroup::TurtleCount)
              && std::ranges::all_of(
                  invalidInputSeaTurtles.Turtles(), [](const auto& turtle) {
                      return std::isfinite(turtle.x) && std::isfinite(turtle.y)
                          && std::isfinite(turtle.z)
                          && std::isfinite(turtle.headingRadians);
                  }),
          "sea turtles reject non-finite frame and observer input without state poisoning");
    bool observedTurtleCruise = false;
    bool observedTurtleForage = false;
    bool observedTurtleSurface = false;
    bool observedNearSurfaceTurtle = false;
    for (int frame = 0; frame < 12000; ++frame) {
        seaTurtles.Update(1.0f / 30.0f, 0.0f, 20.0f, 0.0f);
        repeatedSeaTurtles.Update(1.0f / 30.0f, 0.0f, 20.0f, 0.0f);
        for (std::size_t index = 0; index < seaTurtles.Turtles().size(); ++index) {
            const island::SeaTurtleState& turtle = seaTurtles.Turtles()[index];
            const island::SeaTurtleState& repeated = repeatedSeaTurtles.Turtles()[index];
            const island::SeaTurtleHomeRange& home = seaTurtles.Homes()[index];
            const island::TerrainSample sample = terrain.Sample(turtle.x, turtle.z);
            Check(turtle.x == repeated.x && turtle.y == repeated.y
                      && turtle.z == repeated.z
                      && turtle.headingRadians == repeated.headingRadians
                      && turtle.pitchRadians == repeated.pitchRadians
                      && turtle.bankRadians == repeated.bankRadians
                      && turtle.flipperStroke == repeated.flipperStroke
                      && turtle.behavior == repeated.behavior,
                  "sea-turtle simulation remains deterministic over a long session");
            Check(std::hypot(turtle.x - home.x, turtle.z - home.z)
                          <= home.radius + 0.01f
                      && sample.signedShoreDistance
                          >= island::SeaTurtleGroup::MinimumShoreDistance - 0.01f
                      && sample.signedShoreDistance
                          <= island::SeaTurtleGroup::MaximumShoreDistance + 0.01f
                      && sample.height <= -0.719f
                      && turtle.y >= sample.height + 0.38f
                      && turtle.y <= -0.09f,
                  "sea turtles remain between seabed and surface inside near-shore ranges");
            Check(island::IslandEnvironmentField::AquaticHabitatSuitability(
                      turtle.x, turtle.z, sample.signedShoreDistance, sample.height) >= .025f,
                  "sea turtles remain inside shared reef or seagrass habitat");
            Check(std::isfinite(turtle.speed) && turtle.speed >= 0.0f
                      && std::isfinite(turtle.flipperStroke)
                      && std::fabs(turtle.flipperStroke) <= 1.001f
                      && std::fabs(turtle.pitchRadians) <= 0.241f
                      && std::fabs(turtle.bankRadians) <= 0.221f,
                  "sea-turtle motion and articulated flipper pose remain finite and bounded");
            observedTurtleCruise = observedTurtleCruise
                || turtle.behavior == island::SeaTurtleBehavior::Cruise;
            observedTurtleForage = observedTurtleForage
                || turtle.behavior == island::SeaTurtleBehavior::Forage;
            observedTurtleSurface = observedTurtleSurface
                || turtle.behavior == island::SeaTurtleBehavior::Surface;
            observedNearSurfaceTurtle = observedNearSurfaceTurtle
                || (turtle.behavior == island::SeaTurtleBehavior::Surface
                    && turtle.y > -0.18f);
        }
    }
    {
        // Shared turtle shape reference: bounded inside the culling sphere, teardrop outline,
        // a dome above the rim, paddles that bend and lift with the stroke, invalid input refused.
        using namespace island::sea_turtle;
        float reach = 0.0f, widest = 0.0f, widestU = 0.0f, tallest = 0.0f, tallestU = 0.0f;
        for (int i = 0; i <= 40; ++i) {
            const float u = -1.0f + 0.05f * static_cast<float>(i);
            const float w = HalfWidth(u);
            if (w > widest) { widest = w; widestU = u; }
            for (int j = 0; j <= 20; ++j) {
                const float sPos = -1.0f + 0.1f * static_cast<float>(j);
                const float h = CarapaceHeight(u, sPos), b = PlastronY(u, sPos);
                Check(std::isfinite(h) && h >= CarapaceRimY && b <= CarapaceRimY,
                      "turtle carapace stays above its rim and the plastron below it");
                if (h > tallest) { tallest = h; tallestU = u; }
                reach = std::max(reach, std::hypot(sPos * w, h, LengthToZ(u)));
            }
        }
        Check(widest > 0.35f && widestU > 0.0f && widestU < 0.5f && HalfWidth(-1.0f) == 0.0f
                  && HalfWidth(1.0f) == 0.0f,
              "turtle carapace is a closed teardrop widest ahead of its middle");
        Check(tallest > 0.18f && tallest < 0.26f && std::abs(tallestU) < 0.3f,
              "turtle carapace dome peaks near the centre at a realistic height");
        float maximumFrontPaddleHalfWidth = 0.0f, maximumRearPaddleHalfWidth = 0.0f;
        for (const float stroke : {-1.0f, -0.5f, 0.0f, 0.5f, 1.0f}) {
            float rootWidth = 0.0f, tipWidth = 0.0f;
            const auto root = FrontFlipper(1.0f, 0.0f, stroke, rootWidth);
            const auto tip = FrontFlipper(1.0f, 1.0f, stroke, tipWidth);
            float rearWidth = 0.0f;
            const auto rearTip = RearFlipper(-1.0f, 1.0f, stroke, rearWidth);
            Check(rootWidth > tipWidth && tipWidth > 0.02f && rearWidth > 0.02f,
                  "turtle paddles taper to a nondegenerate tip");
            Check(std::hypot(tip.x - root.x, tip.y - root.y, tip.z - root.z) > 0.6f
                      && std::hypot(rearTip.x, rearTip.z) < 0.9f,
                  "front paddles are long and rear paddles short");
            for (int i = 0; i <= 10; ++i) {
                float frontWidth = 0.0f, rearSampleWidth = 0.0f;
                const auto f = FrontFlipper(-1.0f, 0.1f * static_cast<float>(i), stroke, frontWidth);
                const auto r = RearFlipper(1.0f, 0.1f * static_cast<float>(i), stroke, rearSampleWidth);
                maximumFrontPaddleHalfWidth=std::max(maximumFrontPaddleHalfWidth,frontWidth);
                maximumRearPaddleHalfWidth=std::max(maximumRearPaddleHalfWidth,rearSampleWidth);
                reach = std::max({reach, std::hypot(f.x, f.y, std::abs(f.z) + frontWidth),
                                  std::hypot(r.x, r.y, std::abs(r.z) + rearSampleWidth)});
            }
        }
        Check(maximumFrontPaddleHalfWidth>0.11f&&maximumFrontPaddleHalfWidth<0.135f
                  &&maximumRearPaddleHalfWidth>0.07f&&maximumRearPaddleHalfWidth<0.09f,
              "turtle paddles retain a narrow hydrodynamic planform rather than broad leaves");
        float upWidth = 0.0f, downWidth = 0.0f;
        const auto up = FrontFlipper(1.0f, 1.0f, 1.0f, upWidth);
        const auto down = FrontFlipper(1.0f, 1.0f, -1.0f, downWidth);
        Check(up.y > down.y + 0.3f && down.z < up.z,
              "front paddle tip lifts on the recovery stroke and sweeps back on the power stroke");
        Check(reach <= ReachLimit, "every turtle vertex stays inside the culling reach limit");
        Check(ScuteAt(0.0f) == Scute::Vertebral && ScuteAt(0.5f) == Scute::Costal
                  && ScuteAt(-0.9f) == Scute::Marginal && ScuteRow(-1.0f) == 0 && ScuteRow(1.0f) == 4,
              "turtle scute bands and rows are assigned as authored");
        for (int boundary = 1; boundary <= 4; ++boundary) {
            const float left = ScuteRowBoundaryU(boundary, -0.35f);
            const float centre = ScuteRowBoundaryU(boundary, 0.0f);
            const float right = ScuteRowBoundaryU(boundary, 0.35f);
            CheckNear(left, right, 1.0e-6f,
                      "turtle transverse scute boundaries remain bilaterally symmetric");
            Check(std::abs(centre - left) > 0.015f,
                  "turtle transverse scute boundaries form visible alternating points");
        }
        for (const int column : {1, 3, 5, 7}) {
            const float a = ScuteColumnBoundaryS(column, -0.85f);
            const float b = ScuteColumnBoundaryS(column, -0.75f);
            Check(std::abs(a - b) > 0.01f,
                  "turtle longitudinal scute boundaries visibly interlock between rows");
            CheckNear(ScuteColumnBoundaryS(column, 0.25f),
                      -ScuteColumnBoundaryS(8 - column, 0.25f), 1.0e-6f,
                      "turtle longitudinal scute boundaries remain bilaterally symmetric");
        }
        for (const float invalid : {1.5f, -1.5f, std::numeric_limits<float>::quiet_NaN()}) {
            bool rejected = 0;
            try { (void)HalfWidth(invalid); } catch (const std::invalid_argument&) { rejected = true; }
            Check(rejected, "turtle profile rejects out-of-range coordinates");
            rejected = false;
            float w = 0.0f;
            try { (void)FrontFlipper(1.0f, 0.5f, invalid, w); } catch (const std::invalid_argument&) { rejected = true; }
            Check(rejected, "turtle paddle rejects an invalid stroke");
        }
        for (const int invalidColumn : {0, 2, 4, 6, 8}) {
            bool rejected = false;
            try { (void)ScuteColumnBoundaryS(invalidColumn, 0.0f); }
            catch (const std::invalid_argument&) { rejected = true; }
            Check(rejected, "turtle scute profile rejects non-boundary columns");
        }
    }
    Check(observedTurtleCruise && observedTurtleForage && observedTurtleSurface
              && observedNearSurfaceTurtle,
          "sea turtles reach cruise, forage, and visibly near-surface states");
    Check(seaTurtles.FullSimulationCount() + seaTurtles.ReducedSimulationCount()
              + seaTurtles.DormantSimulationCount()
              == static_cast<int>(island::SeaTurtleGroup::TurtleCount),
          "sea-turtle simulation tiers account for the complete population");
    island::SeaTurtleGroup reactiveSeaTurtles(terrain);
    const island::SeaTurtleState initialReactiveTurtle = reactiveSeaTurtles.Turtles().front();
    for (int frame = 0; frame < 240; ++frame) {
        reactiveSeaTurtles.Update(
            1.0f / 60.0f, initialReactiveTurtle.x, initialReactiveTurtle.y,
            initialReactiveTurtle.z);
    }
    const island::SeaTurtleState& retreatingTurtle = reactiveSeaTurtles.Turtles().front();
    Check(std::hypot(retreatingTurtle.x - initialReactiveTurtle.x,
                     retreatingTurtle.z - initialReactiveTurtle.z) > 0.25f,
          "a nearby observer produces mild sea-turtle separation without hostility");

    island::CrabColony crabs(terrain);
    island::CrabColony repeatedCrabs(terrain);
    island::CrabColony invalidInputCrabs(terrain);
    Check(crabs.Crabs().size() == island::CrabColony::CrabCount
              && crabs.Homes().size() == island::CrabColony::CrabCount,
          "beach-crab simulation exposes the intended bounded population and homes");
    for (std::size_t index = 0; index < crabs.Crabs().size(); ++index) {
        const island::CrabState& crab = crabs.Crabs()[index];
        const island::CrabState& repeated = repeatedCrabs.Crabs()[index];
        const island::CrabHomeRange& home = crabs.Homes()[index];
        const island::TerrainSample sample = terrain.Sample(crab.x, crab.z);
        Check(crab.x == repeated.x && crab.y == repeated.y && crab.z == repeated.z
                  && crab.bodyYawRadians == repeated.bodyYawRadians
                  && crab.colorVariant == repeated.colorVariant,
              "beach-crab construction is deterministic");
        Check(std::hypot(crab.x - home.x, crab.z - home.z) < 0.001f
                  && home.radius >= 5.8f && home.radius <= 8.0f,
              "each beach crab begins inside its bounded home range");
        Check(sample.signedShoreDistance >= island::CrabColony::MinimumShoreDistance
                  && sample.signedShoreDistance <= island::CrabColony::MaximumShoreDistance
                  && sample.height >= -0.02f && sample.slopeRadians < 0.38f,
              "each beach crab begins in suitable shoreline habitat");
    }
    invalidInputCrabs.Update(quietNan, quietNan, quietNan, quietNan);
    Check(invalidInputCrabs.DormantSimulationCount()
              == static_cast<int>(island::CrabColony::CrabCount)
              && std::ranges::all_of(invalidInputCrabs.Crabs(), [](const auto& crab) {
                     return std::isfinite(crab.x) && std::isfinite(crab.y)
                         && std::isfinite(crab.z) && std::isfinite(crab.bodyYawRadians);
                 }),
          "beach crabs reject non-finite frame and observer input without state poisoning");
    bool observedPause = false;
    bool observedForage = false;
    for (int frame = 0; frame < 12000; ++frame) {
        crabs.Update(1.0f / 30.0f, 0.0f, 100.0f, 0.0f);
        repeatedCrabs.Update(1.0f / 30.0f, 0.0f, 100.0f, 0.0f);
        const auto states = crabs.Crabs();
        const auto repeatedStates = repeatedCrabs.Crabs();
        for (std::size_t index = 0; index < states.size(); ++index) {
            const island::CrabState& crab = states[index];
            const island::CrabState& repeated = repeatedStates[index];
            const island::CrabHomeRange& home = crabs.Homes()[index];
            const island::TerrainSample sample = terrain.Sample(crab.x, crab.z);
            Check(crab.x == repeated.x && crab.y == repeated.y && crab.z == repeated.z
                      && crab.bodyYawRadians == repeated.bodyYawRadians
                      && crab.travelHeadingRadians == repeated.travelHeadingRadians
                      && crab.behavior == repeated.behavior,
                  "beach-crab simulation remains deterministic over a long session");
            Check(std::hypot(crab.x - home.x, crab.z - home.z) <= home.radius + 0.01f
                      && sample.signedShoreDistance
                          >= island::CrabColony::MinimumShoreDistance - 0.01f
                      && sample.signedShoreDistance
                          <= island::CrabColony::MaximumShoreDistance + 0.01f
                      && sample.height >= -0.021f && sample.slopeRadians < 0.381f,
                  "beach crabs remain inside bounded dry shoreline habitat");
            Check(std::isfinite(crab.speed) && std::isfinite(crab.legPhase)
                      && std::isfinite(crab.clawPose)
                      && crab.speed >= 0.0f && std::fabs(crab.legPhase) <= 1.001f
                      && crab.clawPose >= 0.0f && crab.clawPose <= 1.0f,
                  "beach-crab animation remains finite and bounded");
            observedPause = observedPause || crab.behavior == island::CrabBehavior::Pause;
            observedForage = observedForage || crab.behavior == island::CrabBehavior::Forage;
        }
        if (frame % 30 == 0) {
            for (std::size_t first = 0; first < states.size(); ++first) {
                for (std::size_t second = first + 1; second < states.size(); ++second) {
                    Check(std::hypot(states[first].x - states[second].x,
                                     states[first].z - states[second].z)
                              >= island::CrabColony::MinimumAgentSeparation - 0.001f,
                          "live beach crabs overlap while foraging");
                }
            }
        }
    }
    Check(observedPause && observedForage,
          "beach-crab colonies alternate between quiet and foraging behaviour");
    Check(crabs.FullSimulationCount() + crabs.ReducedSimulationCount()
              + crabs.DormantSimulationCount()
              == static_cast<int>(island::CrabColony::CrabCount),
          "beach-crab simulation tiers account for the complete population");
    island::CrabColony reactiveCrabs(terrain);
    const island::CrabState initialReactiveCrab = reactiveCrabs.Crabs().front();
    for (int frame = 0; frame < 90; ++frame) {
        reactiveCrabs.Update(1.0f / 60.0f,
                             initialReactiveCrab.x, initialReactiveCrab.y,
                             initialReactiveCrab.z);
    }
    const island::CrabState& escapedCrab = reactiveCrabs.Crabs().front();
    Check(escapedCrab.behavior == island::CrabBehavior::Scuttle
              && std::hypot(escapedCrab.x - initialReactiveCrab.x,
                            escapedCrab.z - initialReactiveCrab.z) > 0.5f,
          "a nearby observer provokes a visible non-hostile crab scuttle response");

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
        Check(island::IslandEnvironmentField::AquaticHabitatSuitability(
                  school.centreX, school.centreZ,
                  sample.signedShoreDistance, sample.height) >= .04f,
              "fish school centres initialize in shared reef or seagrass habitat");
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
            const auto schoolSample = terrain.Sample(school.centreX, school.centreZ);
            Check(island::IslandEnvironmentField::AquaticHabitatSuitability(
                      school.centreX, school.centreZ, schoolSample.signedShoreDistance,
                      schoolSample.height) >= .04f,
                  "fish school routes remain tied to shared reef or seagrass habitat");
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
    const island::CanopyLightField canopyLight(terrain, island::MakeCanopyOccluders(jungle.Canopy()));
    const island::GroundCoverField groundCover;
    const island::NaturalPropLayout props(terrain, vegetation, jungle);
    std::vector<island::CrabHabitatCue> crabDebrisCues;
    for (const auto& wood : props.Driftwood()) crabDebrisCues.push_back({wood.x, wood.z});
    for (const auto& detail : props.BeachDetails())
        if (detail.variant == 3U) crabDebrisCues.push_back({detail.x, detail.z});
    island::CrabColony debrisCrabs(terrain), repeatedDebrisCrabs(terrain);
    debrisCrabs.SetDebrisHabitats(crabDebrisCues);
    repeatedDebrisCrabs.SetDebrisHabitats(crabDebrisCues);
    Check(debrisCrabs.Burrows().size() == 6U
              && repeatedDebrisCrabs.Burrows().size() == 6U,
          "six crab colonies expose one visible shared burrow each");
    for (std::size_t colony = 0; colony < debrisCrabs.Burrows().size(); ++colony) {
        const auto& burrow = debrisCrabs.Burrows()[colony];
        const auto& repeated = repeatedDebrisCrabs.Burrows()[colony];
        Check(burrow.x == repeated.x && burrow.y == repeated.y
                  && burrow.z == repeated.z && burrow.radius == repeated.radius,
              "crab burrows derive deterministically from the debris-linked homes");
        Check(std::isfinite(burrow.x) && std::isfinite(burrow.y)
                  && std::isfinite(burrow.z) && burrow.radius >= .24f
                  && burrow.radius <= .31f
                  && std::fabs(burrow.y - terrain.Height(burrow.x, burrow.z) - .012f)
                      < .001f,
              "crab burrows remain small and grounded on their beach section");
        for (std::size_t member = 0; member < 5U; ++member) {
            const auto& home = debrisCrabs.Homes()[colony * 5U + member];
            Check(std::hypot(home.x - burrow.x, home.z - burrow.z) < 2.5f,
                  "each crab home remains beside its colony burrow");
        }
    }
    for (std::size_t index = 0; index < debrisCrabs.Homes().size(); ++index) {
        const auto home = debrisCrabs.Homes()[index];
        Check(home.x == repeatedDebrisCrabs.Homes()[index].x
                  && home.z == repeatedDebrisCrabs.Homes()[index].z,
              "debris-guided crab colony placement is deterministic");
        Check(island::CrabColony::IsNearDebris(home, crabDebrisCues),
              "crab colony home is not beside driftwood or dry wrack");
        const std::size_t first=(index/5U)*5U;
        for(std::size_t previous=first;previous<index;++previous)
            Check(std::hypot(home.x-debrisCrabs.Homes()[previous].x,
                             home.z-debrisCrabs.Homes()[previous].z)>=1.119f,
                  "debris-guided crab homes keep broad carapaces separated");
    }
    const island::ShipwreckLayout shipwreck(terrain);
    const island::ShipwreckLayout repeatedShipwreck(terrain);
    const island::BirdPerchLayout birdPerches(terrain, jungle, props, shipwreck);
    const island::BirdPerchLayout repeatedBirdPerches(
        terrain, jungle, props, repeatedShipwreck);
    Check(birdPerches.Sites().size() == island::BirdPerchLayout::SiteCount
              && repeatedBirdPerches.Sites().size() == birdPerches.Sites().size(),
          "bird perch layout derives the bounded requested world-anchor inventory");
    std::array<int, 3> perchKinds{};
    for (std::size_t index = 0; index < birdPerches.Sites().size(); ++index) {
        const auto& site = birdPerches.Sites()[index];
        const auto& repeatedSite = repeatedBirdPerches.Sites()[index];
        Check(site.x == repeatedSite.x && site.y == repeatedSite.y
                  && site.z == repeatedSite.z
                  && site.headingRadians == repeatedSite.headingRadians
                  && site.kind == repeatedSite.kind,
              "bird world perch anchors derive deterministically");
        Check(site.y >= terrain.Height(site.x, site.z) + 0.35f,
              "bird world perch anchor remains above terrain");
        ++perchKinds[static_cast<std::size_t>(site.kind)];
    }
    Check(perchKinds[0] == 5 && perchKinds[1] == 2 && perchKinds[2] == 1,
          "bird perch layout includes five branches, two rocks and one wreck beam");
    const island::ShipwreckPlacement& ship = shipwreck.Ship();
    const island::ShipwreckPlacement& repeatedShip = repeatedShipwreck.Ship();
    std::vector<island::ContactFootprint> coastalFootprints;
    for (const auto& rock : props.Rocks()) {
        if (!rock.offshore) continue;
        const auto footprint = island::RockContactFootprint(rock);
        CheckNear(footprint.directionZ, -std::sin(rock.yawRadians), 0.00001f,
                  "rock shadow follows the mesh's local X rotation convention");
        coastalFootprints.push_back(footprint);
    }
    const auto hullFootprints = island::HullContactFootprints(ship);
    const auto& aftShadow = hullFootprints[0];
    const auto& bowShadow = hullFootprints[1];
    const float hullGap = std::hypot(aftShadow.x - bowShadow.x, aftShadow.z - bowShadow.z)
        - aftShadow.alongRadius - bowShadow.alongRadius;
    Check(hullGap > 2.0f && hullGap < 2.5f, "contact shadows preserve the open hull break");
    CheckNear(aftShadow.directionX, std::sin(ship.yawRadians), 0.00001f,
              "hull contact shadow follows the mesh's local Z convention");
    coastalFootprints.insert(coastalFootprints.end(), hullFootprints.begin(), hullFootprints.end());
    for (const auto& debris : shipwreck.Debris()) {
        coastalFootprints.push_back(island::DebrisContactFootprint(debris));
    }
    Check(coastalFootprints.size() == 28, "all 14 reef rocks, two hull halves and 12 fragments cast grounding patches");
    std::vector<island::ContactFootprint> landFootprints;
    for (const auto& rock : props.Rocks()) {
        if (!rock.offshore) landFootprints.push_back(island::LandRockContactFootprint(rock));
    }
    for (const auto& log : props.FallenLogs()) landFootprints.push_back(island::LogContactFootprint(log));
    for (const auto& wood : props.Driftwood()) landFootprints.push_back(island::DriftwoodContactFootprint(wood));
    Check(landFootprints.size() == static_cast<std::size_t>(std::count_if(
              props.Rocks().begin(), props.Rocks().end(), [](const auto& rock) { return !rock.offshore; }))
              + props.FallenLogs().size() + props.Driftwood().size(),
          "all land rocks, logs and driftwood pieces retain their shadows");
    for (float sign : {-1.0f, 1.0f}) {
        island::RockPlacement rock;
        island::FallenLogPlacement log;
        island::DriftwoodPlacement wood;
        rock.yawRadians = log.yawRadians = wood.yawRadians = sign*std::numbers::pi_v<float>/4;
        for (const auto& footprint : {island::LogContactFootprint(log), island::DriftwoodContactFootprint(wood)}) {
            CheckNear(footprint.directionX, std::sqrt(0.5f), 0.000001f,
                      "positive local-X shadow axis follows the model's 45-degree rotation");
            CheckNear(footprint.directionZ, sign*std::sqrt(0.5f), 0.000001f,
                      "wood shadows retain placement and collision heading");
            CheckNear(footprint.directionZ, -std::sin(island::WoodModelYaw(log.yawRadians)), 0.000001f,
                      "wood renderer converts its heading to the matrix rotation convention");
        }
        const auto rockFootprint = island::LandRockContactFootprint(rock);
        CheckNear(rockFootprint.directionZ, -sign*std::sqrt(0.5f), 0.000001f,
                  "land rock shadow retains its distinct model rotation convention");
        CheckNear(rockFootprint.z, -sign*std::sqrt(0.5f)*0.08f, 0.000001f,
                  "land rock shadow offset rotates with its corrected axis");
    }
    for (const auto& log : props.FallenLogs()) {
        const float modelYaw = island::WoodModelYaw(log.yawRadians);
        for (int sample = 0; sample < island::NaturalPropLayout::LogCollisionSamples; ++sample) {
            const float t = static_cast<float>(sample) / (island::NaturalPropLayout::LogCollisionSamples-1) - 0.5f;
            const float x = log.x + std::cos(modelYaw)*log.length*t;
            const float z = log.z - std::sin(modelYaw)*log.length*t;
            Check(std::any_of(props.Obstacles().begin(), props.Obstacles().end(), [&](const auto& obstacle) {
                return std::hypot(obstacle.x-x,obstacle.z-z) < 0.00001f
                    && std::abs(obstacle.radius-log.radius*0.92f) < 0.00001f;
            }), "every rendered log centreline collision sample matches an existing physical proxy");
        }
    }
    // Independent rendered-grid interpolation, including the top-right/bottom-left diagonal.
    const auto renderedHeight = [&](float x, float z) {
        const auto coordinate = [](float value) {
            return std::clamp(static_cast<int>(std::floor((value + island::TerrainMeshLayout::Extent)
                * island::TerrainMeshLayout::QuadsPerAxis / (2 * island::TerrainMeshLayout::Extent))),
                0, island::TerrainMeshLayout::QuadsPerAxis - 1);
        };
        const int column = coordinate(x), row = coordinate(z);
        const float x0 = island::TerrainMeshLayout::Coordinate(column);
        const float x1 = island::TerrainMeshLayout::Coordinate(column+1);
        const float z0 = island::TerrainMeshLayout::Coordinate(row);
        const float z1 = island::TerrainMeshLayout::Coordinate(row+1);
        const float u = (x-x0)/(x1-x0), v = (z-z0)/(z1-z0);
        if (u+v <= 1) return terrain.Height(x0,z0)*(1-u-v) + terrain.Height(x1,z0)*u + terrain.Height(x0,z1)*v;
        return terrain.Height(x1,z1)*(u+v-1) + terrain.Height(x1,z0)*(1-v) + terrain.Height(x0,z1)*(1-u);
    };
    const auto validateContactPatches = [&](const auto& footprints, const island::ContactPatchDetail detail) {
      std::size_t vertexCount = 0;
      for (const auto& footprint : footprints) {
        const auto patch = island::BuildContactPatch(terrain, footprint, detail);
        Check(!patch.empty() && patch.size() % 3 == 0 && patch.size() < 9000,
              "coastal contact geometry is nonempty, triangular and bounded");
        vertexCount += patch.size();
        double area = 0;
        float maximumOpacity = 0;
        for (const auto& vertex : patch) {
            Check(std::isfinite(vertex.y) && vertex.opacity >= 0 && vertex.opacity <= footprint.opacity + 0.000001f,
                  "coastal patch upload is finite with bounded opacity");
            maximumOpacity = std::max(maximumOpacity, vertex.opacity);
            const float dx = vertex.x - footprint.x, dz = vertex.z - footprint.z;
            const float u = (dx*footprint.directionX + dz*footprint.directionZ) / footprint.alongRadius;
            const float v = (-dx*footprint.directionZ + dz*footprint.directionX) / footprint.acrossRadius;
            Check(u*u+v*v <= 1.001f, "clipped geometry stays inside its grounding footprint");
            CheckNear(vertex.y, renderedHeight(vertex.x,vertex.z) + island::ContactPatchSurfaceOffset,
                      0.0001f, "coastal patch vertices follow the rendered terrain, not analytic-height mismatches");
        }
        for (std::size_t index = 0; index < patch.size(); index += 3) {
            const auto& a = patch[index]; const auto& b = patch[index+1]; const auto& c = patch[index+2];
            area += std::abs((static_cast<double>(b.x)-a.x)*(c.z-a.z) - (static_cast<double>(b.z)-a.z)*(c.x-a.x)) * 0.5;
            CheckNear((a.y+b.y+c.y)/3, renderedHeight((a.x+b.x+c.x)/3,(a.z+b.z+c.z)/3)
                          + island::ContactPatchSurfaceOffset, 0.00015f,
                      "shadow triangle interiors remain on one terrain plane");
        }
        const int sectors = detail == island::ContactPatchDetail::Compact ? 12 : 24;
        const double expectedArea = sectors * 0.5 * std::sin(2 * std::numbers::pi / sectors)
            * footprint.alongRadius * footprint.acrossRadius;
        Check(std::abs(area - expectedArea) < expectedArea * 0.001,
              "terrain clipping preserves footprint area without holes or duplicate coverage");
        CheckNear(maximumOpacity, footprint.opacity, 0.000001f, "every grounding patch retains its contact core");
      }
      return vertexCount;
    };
    const auto coastalVertexCount = validateContactPatches(coastalFootprints, island::ContactPatchDetail::Coastal);
    const auto landVertexCount = validateContactPatches(landFootprints, island::ContactPatchDetail::Compact);
    Check(landVertexCount < 70000, "the complete clipped land-shadow geometry stays within its static budget");
    Check(coastalVertexCount == 23772, "coastal patch geometry matches the authored shoreline layout");
    std::cout << "Land contact patches: " << landFootprints.size() << ", vertices: " << landVertexCount << '\n';
    Check(coastalVertexCount < 50000, "the complete coastal shadow addition fits the static vertex budget");
    std::cout << "Coastal contact patches: " << coastalFootprints.size() << ", vertices: " << coastalVertexCount << '\n';
    for (int invalidCase = 0; invalidCase < 5; ++invalidCase) {
        auto invalid = aftShadow;
        if (invalidCase == 0) invalid.x = std::numeric_limits<float>::quiet_NaN();
        if (invalidCase == 1) invalid.alongRadius = 8.1f;
        if (invalidCase == 2) invalid.acrossRadius = 0;
        if (invalidCase == 3) invalid.directionX = 4;
        if (invalidCase == 4) invalid.opacity = -1;
        bool rejected = false;
        try { (void)island::BuildContactPatch(terrain, invalid); }
        catch (const std::invalid_argument&) { rejected = true; }
        Check(rejected, "invalid contact footprint is rejected before GPU upload");
    }
    Check(ship.length == 20.0f && ship.beam == 5.2f && ship.hullHeight == 4.4f,
          "shipwreck retains the authored large-ship dimensions");
    Check(ship.x == repeatedShip.x && ship.y == repeatedShip.y
              && ship.z == repeatedShip.z && ship.yawRadians == repeatedShip.yawRadians
              && ship.heelRadians == repeatedShip.heelRadians
              && ship.pitchRadians == repeatedShip.pitchRadians,
          "shipwreck placement is bitwise deterministic");
    Check(std::isfinite(ship.x) && std::isfinite(ship.y) && std::isfinite(ship.z)
              && std::isfinite(ship.yawRadians) && std::isfinite(ship.heelRadians)
              && std::isfinite(ship.pitchRadians),
          "shipwreck placement remains finite");
    const float shipShoreDistance = terrain.SignedShoreDistance(ship.x, ship.z);
    Check(shipShoreDistance < -3.0f && shipShoreDistance > -12.0f,
          "large shipwreck is grounded in accessible beach shallows");
    Check(shipwreck.Debris().size() == island::ShipwreckLayout::DebrisCount
              && shipwreck.Obstacles().size()
                  == island::ShipwreckLayout::CollisionProxyCount,
          "shipwreck exposes its complete bounded debris and collision inventory");
    std::array<int, 3> shipwreckDebrisKinds{};
    bool shipwreckDebrisReachesWater = false;
    bool shipwreckDebrisReachesBeach = false;
    int stronglyAngledShipwreckDebris = 0;
    bool shipwreckDebrisAnglesClockwise = false;
    bool shipwreckDebrisAnglesCounterClockwise = false;
    for (std::size_t index = 0; index < shipwreck.Debris().size(); ++index) {
        const island::ShipwreckDebrisPlacement& debris = shipwreck.Debris()[index];
        const island::ShipwreckDebrisPlacement& repeated = repeatedShipwreck.Debris()[index];
        Check(debris.x == repeated.x && debris.y == repeated.y && debris.z == repeated.z
                  && debris.yawRadians == repeated.yawRadians
                  && debris.length == repeated.length && debris.width == repeated.width
                  && debris.kind == repeated.kind,
              "shipwreck debris placement is bitwise deterministic");
        Check(std::isfinite(debris.x) && std::isfinite(debris.y)
                  && std::isfinite(debris.z) && std::isfinite(debris.yawRadians)
                  && debris.length >= 2.0f && debris.length <= 5.6f
                  && debris.width >= 0.20f && debris.width <= 0.50f,
              "shipwreck debris remains finite and visibly substantial");
        const auto kind = static_cast<std::size_t>(debris.kind);
        Check(kind < shipwreckDebrisKinds.size(),
              "shipwreck debris kind remains in renderer range");
        if (kind < shipwreckDebrisKinds.size()) ++shipwreckDebrisKinds[kind];
        const float shoreDistance = terrain.SignedShoreDistance(debris.x, debris.z);
        shipwreckDebrisReachesWater = shipwreckDebrisReachesWater || shoreDistance < -1.0f;
        shipwreckDebrisReachesBeach = shipwreckDebrisReachesBeach || shoreDistance > 2.0f;
        Check(std::hypot(debris.x - ship.x, debris.z - ship.z) < 30.0f,
              "shipwreck debris stays in a coherent beach-directed trail");
        const float debrisAngle = debris.yawRadians - ship.yawRadians;
        Check(std::abs(debrisAngle) <= 0.85f,
              "shipwreck debris remains plausibly aligned with the wreck event");
        if (std::abs(debrisAngle) >= 0.40f) ++stronglyAngledShipwreckDebris;
        shipwreckDebrisAnglesClockwise = shipwreckDebrisAnglesClockwise
            || debrisAngle <= -0.40f;
        shipwreckDebrisAnglesCounterClockwise = shipwreckDebrisAnglesCounterClockwise
            || debrisAngle >= 0.40f;
    }
    Check(shipwreckDebrisKinds[0] == 9 && shipwreckDebrisKinds[1] == 2
              && shipwreckDebrisKinds[2] == 1,
          "shipwreck debris mixes planks, major hull fragments, and one spar");
    Check(shipwreckDebrisReachesWater && shipwreckDebrisReachesBeach,
          "shipwreck debris trail connects the shallow wreck to the beach");
    Check(stronglyAngledShipwreckDebris >= 8 && shipwreckDebrisAnglesClockwise
              && shipwreckDebrisAnglesCounterClockwise,
          "shipwreck debris breaks parallel repetition in both directions");
    for (std::size_t index = 0; index < shipwreck.Obstacles().size(); ++index) {
        const island::CircularObstacle& obstacle = shipwreck.Obstacles()[index];
        const island::CircularObstacle& repeated = repeatedShipwreck.Obstacles()[index];
        Check(obstacle.x == repeated.x && obstacle.z == repeated.z
                  && obstacle.radius == repeated.radius,
              "shipwreck collision proxies are bitwise deterministic");
        Check(std::isfinite(obstacle.x) && std::isfinite(obstacle.z)
                  && obstacle.radius >= 1.3f && obstacle.radius <= 1.8f
                  && std::hypot(obstacle.x - ship.x, obstacle.z - ship.z) <= 8.1f,
              "shipwreck collision proxies cover only the major grounded hull");
    }
    std::vector<island::CircularObstacle> worldObstacles;
    worldObstacles.reserve(vegetation.Obstacles().size() + jungle.Obstacles().size()
                           + props.Obstacles().size() + shipwreck.Obstacles().size());
    worldObstacles.insert(worldObstacles.end(), vegetation.Obstacles().begin(),
                          vegetation.Obstacles().end());
    worldObstacles.insert(worldObstacles.end(), jungle.Obstacles().begin(),
                          jungle.Obstacles().end());
    worldObstacles.insert(worldObstacles.end(), props.Obstacles().begin(),
                          props.Obstacles().end());
    worldObstacles.insert(worldObstacles.end(), shipwreck.Obstacles().begin(),
                          shipwreck.Obstacles().end());
    const island::WorldCollisionGrid worldCollisionGrid(worldObstacles);
    // Narrowing the vegetation reserve must preserve a continuous walking corridor,
    // including collision footprints of rocks, logs and promoted emergent trunks.
    for (int z = -215; z <= 180; ++z) {
        const float centre = island::VegetationLayout::PrimaryPathCenterX(static_cast<float>(z));
        for (const float offset : {-0.65f, 0.0f, 0.65f}) {
            const auto trailSurface = terrain.Sample(centre + offset, static_cast<float>(z));
            if (trailSurface.slopeRadians >= island::PlayerMotion::MaximumWalkSlopeRadians - 0.025f)
                std::cerr << "Trail slope at z=" << z << " offset=" << offset
                          << " is " << trailSurface.slopeRadians << '\n';
            Check(trailSurface.slopeRadians < island::PlayerMotion::MaximumWalkSlopeRadians - 0.025f,
                  "primary trail stays below the player's slope limit with contact margin");
            for (const auto& obstacle : worldObstacles) {
                const bool clear = std::hypot(centre + offset - obstacle.x,
                                              static_cast<float>(z) - obstacle.z)
                    > obstacle.radius + island::PlayerMotion::CapsuleRadius;
                if (!clear) std::cerr << "Trail blocked at z=" << z << " offset=" << offset
                    << " by obstacle " << obstacle.x << ',' << obstacle.z
                    << " radius=" << obstacle.radius << '\n';
                Check(clear,
                      "narrow trail leaves a continuous player-width corridor beside its centre");
            }
        }
    }
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
    const island::RenderedTerrainSurface monkeyAttachmentTerrain(terrain);
    std::array<bool,island::MonkeyTroop::MonkeyCount> attachmentAttempted{};
    unsigned attachmentCandidates=0,attachmentAccepted=0;
    unsigned excursionAccepted=0;
    unsigned excursionClear=0;
    double attachmentMillis=0,attachmentMaxMillis=0,clearanceMillis=0,clearanceMaxMillis=0;
    double clearanceChunkMaxMillis=0;
    double attachmentChunkMaxMillis=0;
    bool observedPreparing=false;
    std::array<unsigned,island::MonkeyTroop::MonkeyCount> preparationFrames{};
    bool observedLiveClimb=false,observedLiveSummit=false,checkedLiveReturn=false;
    std::array<unsigned,11> attachmentFailures{};
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
    monkeys.SetCanopy(jungle.Canopy());
    repeatedMonkeys.SetCanopy(jungle.Canopy());
    Check(monkeys.Monkeys().size() == island::MonkeyTroop::MonkeyCount,
          "monkey troop has the intended bounded population");
    Check(monkeys.Monkeys().size() == repeatedMonkeys.Monkeys().size(),
          "repeated monkey troop has the same population");
    std::array<bool, 4> observedMonkeyVariants{};
    float minimumMonkeyScale = 10.0f;
    float maximumMonkeyScale = 0.0f;
    for (const island::MonkeyState& monkey : monkeys.Monkeys()) {
        Check(island::MonkeyTroop::HomeHasClosedCanopy(
                  jungle.Canopy(),monkey.homeX,monkey.homeZ),
              "monkey home is not beside a clearance-qualified closed-canopy tree");
        Check(terrain.SignedShoreDistance(monkey.homeX,monkey.homeZ)>=90.0f,
              "monkey home is not in the deep interior canopy");
        Check(monkey.colorVariant < observedMonkeyVariants.size(),
              "monkey material variant remains inside the renderer palette");
        if (monkey.colorVariant < observedMonkeyVariants.size()) {
            observedMonkeyVariants[monkey.colorVariant] = true;
        }
        minimumMonkeyScale = std::min(minimumMonkeyScale, monkey.scale);
        maximumMonkeyScale = std::max(maximumMonkeyScale, monkey.scale);
    }
    for (std::size_t first=0;first<monkeys.Monkeys().size();++first) {
        for (std::size_t second=first+1;second<monkeys.Monkeys().size();++second) {
            const auto& a=monkeys.Monkeys()[first];
            const auto& b=monkeys.Monkeys()[second];
            Check(std::hypot(a.homeX-b.homeX,a.homeZ-b.homeZ)>=7.0f,
                  "deep-interior monkey homes are not sufficiently separated");
        }
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
    bool observedMonkeyTreeApproach = false;
    bool observedMonkeyTreeArrival = false;
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
        Check(monkeys.PreflightSamplesLastUpdate()<=32,"full-excursion preflight has a troop-wide per-update work bound");
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
            if (state.treePreparing) {
                observedPreparing=true;
                ++preparationFrames[index];
                Check(!state.treeExcursion,"pending collision preflight cannot activate tree climbing early");
            } else if (!state.treeExcursion) preparationFrames[index]=0;
            if (state.treeExcursion) {
                Check(preparationFrames[index]>=215,"live climb waits for all attachment and excursion samples");
                observedLiveClimb=true;
                observedLiveSummit=observedLiveSummit||state.treeClock.seconds>=18;
                Check(state.treeIndex>=0 && repeated.treeExcursion
                    && state.treeClock.seconds==repeated.treeClock.seconds,
                    "live monkey climbing preserves tree identity and deterministic clock");
                if (!checkedLiveReturn && state.treeClock.seconds>=12) {
                    checkedLiveReturn=true;
                    auto interrupted=monkeys;
                    interrupted.Update(.1f,state.x,state.y,state.z);
                    Check(interrupted.Monkeys()[index].treeExcursion
                        && interrupted.Monkeys()[index].treeClock.returning,
                        "player proximity requests descent instead of clearing airborne tree grips");
                    bool refused=false;
                    try { interrupted.SetCanopy({}); } catch (const std::logic_error&) { refused=true; }
                    Check(refused && interrupted.Monkeys()[index].treeExcursion,
                        "canopy replacement cannot delete an attached monkey's supporting tree");
                    for (int step=0;step<500 && interrupted.Monkeys()[index].treeIndex>=0;++step)
                        interrupted.Update(.1f,state.x,state.y,state.z);
                    Check(!interrupted.Monkeys()[index].treeExcursion
                        && !interrupted.Monkeys()[index].treeReleasing
                        && interrupted.Monkeys()[index].treeIndex<0,
                        "interrupted live monkey descends and releases grounded grips before fleeing");
                }
            }
            if (state.behavior!=island::MonkeyBehavior::InspectTree) attachmentAttempted[index]=false;
            if (state.behavior==island::MonkeyBehavior::InspectTree && state.treeInspectionSeconds>=3.2f
                && !attachmentAttempted[index] && attachmentCandidates<64) {
                attachmentAttempted[index]=true; ++attachmentCandidates;
                const island::MonkeyAttachmentSite::GroundFrame frame{{state.x,state.y,state.z},state.headingRadians,state.scale};
                const auto attachmentStart=std::chrono::steady_clock::now();
                const auto prepared=island::MonkeyAttachmentSite::Prepare(state.selectedTree,frame,
                    monkeyAttachmentTerrain,worldObstacles,3.0f);
                const double attachmentTime=std::chrono::duration<double,std::milli>(
                    std::chrono::steady_clock::now()-attachmentStart).count();
                attachmentMillis+=attachmentTime;
                attachmentMaxMillis=std::max(attachmentMaxMillis,attachmentTime);
                island::MonkeyAttachmentSite::Result ranged{};
                int siteChecked=0;
                while (siteChecked<island::MonkeyAttachmentSite::SampleCount) {
                    const int count=std::min(16,island::MonkeyAttachmentSite::SampleCount-siteChecked);
                    const auto chunkStart=std::chrono::steady_clock::now();
                    ranged=island::MonkeyAttachmentSite::PrepareRange(state.selectedTree,frame,
                        monkeyAttachmentTerrain,worldObstacles,3.0f,false,siteChecked,count);
                    attachmentChunkMaxMillis=std::max(attachmentChunkMaxMillis,
                        std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-chunkStart).count());
                    siteChecked+=count;
                    if (!ranged.plan) break;
                }
                Check(ranged.failure==prepared.failure && ranged.plan.has_value()==prepared.plan.has_value(),
                    "incremental attachment preserves full validator decisions and rejection categories");
                if (ranged.plan) {
                    Check(siteChecked==1502,"attachment chunks retain 1001 reach and 501 fixed-wind clearance samples");
                    for (std::size_t key=0;key<prepared.plan->keys.size();++key)
                        for (std::size_t limb=0;limb<4;++limb) {
                            const auto a=ranged.plan->keys[key].contacts[limb];
                            const auto b=prepared.plan->keys[key].contacts[limb];
                            Check(a.x==b.x && a.y==b.y && a.z==b.z,
                                "provisional attachment chunks preserve exactly the same plan contacts");
                        }
                }
                if (prepared.plan) {
                    ++attachmentAccepted;
                    const auto& plan=*prepared.plan;
                    const auto excursion=island::MonkeyTreeExcursion::Build(plan,frame,
                        island::GroundJunglePlacement(state.selectedTree,monkeyAttachmentTerrain),3.0f);
                    const auto clearanceStart=std::chrono::steady_clock::now();
                    const auto clearance=island::MonkeyExcursionClearance::Validate(excursion,
                        monkeyAttachmentTerrain,worldObstacles,3.0f);
                    const double clearanceTime=std::chrono::duration<double,std::milli>(
                        std::chrono::steady_clock::now()-clearanceStart).count();
                    clearanceMillis+=clearanceTime;
                    clearanceMaxMillis=std::max(clearanceMaxMillis,clearanceTime);
                    if (clearance.failure==island::MonkeyExcursionClearance::Failure::None) {
                        ++excursionClear;
                        if (excursionClear==1) {
                            int checkedSamples=0;
                            while (checkedSamples<island::MonkeyExcursionClearance::SampleCount) {
                                const int count=std::min(16,island::MonkeyExcursionClearance::SampleCount-checkedSamples);
                                const auto chunkStart=std::chrono::steady_clock::now();
                                const auto chunk=island::MonkeyExcursionClearance::ValidateRange(excursion,
                                    monkeyAttachmentTerrain,worldObstacles,3.0f,checkedSamples,count);
                                clearanceChunkMaxMillis=std::max(clearanceChunkMaxMillis,
                                    std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-chunkStart).count());
                                Check(chunk.failure==island::MonkeyExcursionClearance::Failure::None,
                                    "incremental clearance checks exactly the same passing poses as full validation");
                                checkedSamples+=count;
                            }
                            Check(checkedSamples==1951,"incremental clearance retains every original sample");
                            const auto summit=island::MonkeyTreeExcursion::Sample(excursion,19.0f,22.0f,false);
                            const auto foot=frame.World(summit.body.contacts[2]);
                            auto obstructed=worldObstacles;
                            obstructed.push_back({foot.x,foot.z,.025f});
                            Check(island::MonkeyExcursionClearance::Validate(excursion,monkeyAttachmentTerrain,
                                obstructed,3.0f).failure==island::MonkeyExcursionClearance::Failure::Obstacle,
                                "full excursion preflight rejects an additional obstacle along its bark-contact path");
                            auto sunken=excursion;
                            sunken.frame.origin.y-=.02f; sunken.tree.y-=.02f;
                            Check(island::MonkeyExcursionClearance::Validate(sunken,monkeyAttachmentTerrain,
                                worldObstacles,3.0f).failure==island::MonkeyExcursionClearance::Failure::Terrain,
                                "full excursion preflight rejects sole footprints penetrating actual terrain");
                            auto invalid=excursion; invalid.frame.scale=0;
                            Check(island::MonkeyExcursionClearance::Validate(invalid,monkeyAttachmentTerrain,
                                worldObstacles,3.0f).failure==island::MonkeyExcursionClearance::Failure::InvalidInput,
                                "full excursion clearance rejects invalid frames before querying geometry");
                        }
                    }
                    else std::cout << "Monkey excursion clearance rejected reason=" << static_cast<int>(clearance.failure)
                        << " time=" << clearance.seconds << " probe=" << clearance.probe << '\n';
                    if (island::MonkeyTreeExcursion::IsReachable(excursion)) {
                        ++excursionAccepted;
                        const auto distance=[](auto a,auto b) {
                            return std::hypot(a.y-b.y,std::hypot(a.x-b.x,a.z-b.z));
                        };
                        if (excursionAccepted==1) {
                            bool movingBarkObserved=false;
                            for (float wind:{0.0f,3.0f,17.0f}) for (bool far:{false,true}) {
                                // A coarse trunk can move the requested palm beyond arm
                                // reach. Establish that independently before retargeting;
                                // refusal is correct, clamping and reporting a grip is not.
                                bool initialLimited=false;
                                auto initial=plan.ground;
                                for (std::size_t hand=0;hand<2;++hand) {
                                    const auto target=island::MonkeyTreeReach::Target(excursion.tree,frame.origin,
                                        frame.heading,frame.scale,initial.limbs[hand].root.y,hand==0,wind,far);
                                    Check(target.has_value(),"retarget fixture has defined bark coordinates");
                                    if (target) initial.limbs[hand]=island::MonkeyGroundGait::SolveLimb(
                                        initial.limbs[hand].root,target->position,island::MonkeyTreeReach::BendHint(1),
                                        true,initialLimited);
                                }
                                initialLimited=initialLimited||!island::MonkeyTreeReach::RaiseTorso(initial,.22f);
                                if (initialLimited) {
                                    bool refused=false;
                                    try { (void)island::MonkeyTreeExcursion::Retarget(excursion,wind,far); }
                                    catch (const std::invalid_argument&) { refused=true; }
                                    Check(refused,"unreachable mesh/wind retarget is rejected instead of clamping bark grips");
                                    continue;
                                }
                                const auto retargeted=island::MonkeyTreeExcursion::Retarget(excursion,wind,far);
                                for (std::size_t limb=2;limb<4;++limb) {
                                    Check(distance(retargeted.attachment.keys[0].contacts[limb],plan.keys[0].contacts[limb])<1e-6f
                                        && distance(retargeted.attachment.keys[5].contacts[limb],plan.keys[5].contacts[limb])<1e-6f,
                                        "wind/mesh retargeting leaves planted and staged ground feet fixed");
                                }
                                for (std::size_t hand=0;hand<2;++hand) {
                                    const auto expected=island::MonkeyTreeReach::Target(excursion.tree,frame.origin,
                                        frame.heading,frame.scale,plan.ground.limbs[hand].root.y,hand==0,wind,far);
                                    Check(expected && distance(expected->position,retargeted.attachment.keys[0].contacts[hand])<1e-6f,
                                        "retargeted forepalms use actual current wind and trunk mesh queries");
                                    if (!far && wind!=3)
                                        movingBarkObserved=movingBarkObserved
                                            ||distance(retargeted.attachment.keys[0].contacts[hand],plan.keys[0].contacts[hand])>1e-6f;
                                }
                                for (float boundary:{10.0f,18.0f,21.0f,29.0f}) {
                                    const auto before=island::MonkeyTreeExcursion::Sample(excursion,boundary-.0001f,wind-.0001f,far);
                                    const auto after=island::MonkeyTreeExcursion::Sample(excursion,boundary+.0001f,wind+.0001f,far);
                                    for (std::size_t limb=0;limb<4;++limb) {
                                        const auto& a=before.body.limbs.limbs[limb];
                                        const auto& b=after.body.limbs.limbs[limb];
                                        Check(distance(a.root,b.root)<.001f && distance(a.joint,b.joint)<.001f
                                            && distance(a.sole,b.sole)<.001f,
                                            "stage transitions stay continuous with advancing wind on either trunk mesh");
                                    }
                                }
                            }
                            Check(movingBarkObserved,"wind test must move the bark, not silently reuse frozen grips");
                            for (int step=0;step<=390;++step) {
                                const float time=step*.1f;
                                const auto moving=island::MonkeyTreeExcursion::Sample(excursion,time,time+3,false);
                                Check(!moving.body.limbs.reachLimited,"selected real excursion stays reachable with advancing wind");
                                unsigned supports=0;
                                for (const auto& limb:moving.body.limbs.limbs) {
                                    supports+=limb.supporting;
                                    Check(std::abs(distance(limb.root,limb.joint)-.31f)<1e-4f
                                        && std::abs(distance(limb.joint,limb.sole)-.33f)<1e-4f,
                                        "moving-wind excursion preserves limb lengths");
                                }
                                Check(supports>=3,"moving-wind excursion retains three supports");
                            }
                            for (float interruptAt:{.25f,5.0f,12.0f,17.0f,19.0f,24.0f,33.0f}) {
                                island::MonkeyTreeExcursion::Clock clock;
                                while (clock.seconds<interruptAt) clock.Advance(.05f);
                                const auto before=island::MonkeyTreeExcursion::Sample(excursion,clock.seconds);
                                clock.Advance(.001f,true);
                                const auto after=island::MonkeyTreeExcursion::Sample(excursion,clock.seconds);
                                for (std::size_t limb=0;limb<4;++limb)
                                    Check(distance(before.body.contacts[limb],after.body.contacts[limb])<.005f,
                                        "an excursion interruption starts returning without teleporting contacts");
                                for (int step=0;step<1000 && !clock.finished;++step) clock.Advance(.05f);
                                Check(clock.finished && clock.returning,"interrupted excursion returns in bounded time");
                                const auto returned=island::MonkeyTreeExcursion::Sample(excursion,clock.seconds);
                                const auto planted=island::MonkeyTreeExcursion::Sample(excursion,0);
                                for (std::size_t limb=0;limb<4;++limb)
                                    Check(distance(returned.body.contacts[limb],planted.body.contacts[limb])<1e-5f,
                                        "interruption finishes on the original planted ground/bark grips before release");
                            }
                            island::MonkeyTreeExcursion::Clock clock;
                            clock.Advance(quietNan); clock.Advance(-1);
                            Check(clock.seconds==0 && !clock.finished,"invalid excursion time step does not advance the clock");
                        }
                        for (float boundary:{10.0f,18.0f,21.0f,29.0f}) {
                            const auto before=island::MonkeyTreeExcursion::Sample(excursion,boundary-.0001f);
                            const auto after=island::MonkeyTreeExcursion::Sample(excursion,boundary+.0001f);
                            for (std::size_t limb=0;limb<4;++limb) {
                                const auto& a=before.body.limbs.limbs[limb];
                                const auto& b=after.body.limbs.limbs[limb];
                                Check(distance(a.root,b.root)<.001f && distance(a.joint,b.joint)<.001f
                                    && distance(a.sole,b.sole)<.001f,
                                    "whole tree excursion has continuous roots, joints and soles at stage boundaries");
                            }
                        }
                        for (int step=0;step<=390;++step) {
                            const auto pose=island::MonkeyTreeExcursion::Sample(excursion,step*.1f);
                            const auto windy=island::MonkeyTreeExcursion::Sample(excursion,step*.1f,3+step*.1f,false);
                            Check(!windy.body.limbs.reachLimited,
                                "every accepted natural site retains full-cycle reach with advancing wind");
                            const auto reverse=island::MonkeyTreeExcursion::Sample(excursion,39-step*.1f);
                            unsigned supports=0;
                            for (std::size_t limb=0;limb<4;++limb) {
                                const auto& current=pose.body.limbs.limbs[limb];
                                const auto& windLimb=windy.body.limbs.limbs[limb];
                                Check(std::abs(distance(windLimb.root,windLimb.joint)-.31f)<1e-4f
                                    && std::abs(distance(windLimb.joint,windLimb.sole)-.33f)<1e-4f,
                                    "moving-wind contacts do not stretch limbs at natural sites");
                                supports+=current.supporting;
                                Check(distance(current.sole,reverse.body.limbs.limbs[limb].sole)<.0001f,
                                    "descent and detachment retrace the ascent contacts without ground teleport");
                                Check(std::abs(distance(current.root,current.joint)-.31f)<1e-4f
                                    && std::abs(distance(current.joint,current.sole)-.33f)<1e-4f,
                                    "whole excursion preserves fixed limb lengths");
                            }
                            Check(supports>=3,"whole excursion retains at least three supports");
                            if (pose.stage==island::MonkeyTreeExcursion::Stage::Rest)
                                Check(supports==4,"summit rest holds all four bark contacts");
                        }
                    }
                    if (attachmentAccepted==1) {
                        for (const auto& [first,count]:std::array<std::pair<int,int>,5>{{
                                {-1,1},{0,0},{0,-1},{1502,1},{1501,2}}})
                            Check(island::MonkeyAttachmentSite::PrepareRange(state.selectedTree,frame,
                                monkeyAttachmentTerrain,worldObstacles,3.0f,false,first,count).failure
                                    ==island::MonkeyAttachmentSite::Failure::InvalidInput,
                                "attachment range rejects negative, empty and overflowing sample intervals");
                        for (const auto& [first,count]:std::array<std::pair<int,int>,5>{{
                                {-1,1},{0,0},{0,-1},{1001,1},{1000,2}}})
                            Check(!island::MonkeyTreeAttachment::IsReachableRange(plan,first,count),
                                "reach range rejects negative, empty and overflowing sample intervals");
                        auto displacedTree=state.selectedTree;
                        displacedTree.y+=100;
                        const auto regrounded=island::MonkeyAttachmentSite::Prepare(displacedTree,frame,
                            monkeyAttachmentTerrain,worldObstacles,3.0f);
                        Check(regrounded.plan.has_value() && regrounded.plan->keys[10].contacts[0].y==plan.keys[10].contacts[0].y,
                            "attachment uses rendered tree grounding rather than stale layout Y");
                        auto wrongFrame=frame; wrongFrame.heading+=.2f;
                        Check(island::MonkeyAttachmentSite::Prepare(state.selectedTree,wrongFrame,
                            monkeyAttachmentTerrain,worldObstacles,3.0f).failure==island::MonkeyAttachmentSite::Failure::Heading,
                            "attachment does not silently snap a monkey that is not facing its tree");
                        wrongFrame=frame; wrongFrame.scale=0;
                        Check(island::MonkeyAttachmentSite::Prepare(state.selectedTree,wrongFrame,
                            monkeyAttachmentTerrain,worldObstacles,3.0f).failure==island::MonkeyAttachmentSite::Failure::InvalidInput,
                            "invalid monkey scale is rejected before surface queries");
                    }
                    for (std::size_t foot=2;foot<4;++foot) {
                        const auto point=frame.World(plan.keys[5].contacts[foot]);
                        const auto ground=monkeyAttachmentTerrain.Sample(point.x,point.z);
                        Check(std::abs(point.y-ground.height-island::MonkeyGroundGait::SoleHeight*state.scale/ground.normal.y)<1e-4f,
                            "attachment staging feet follow actual rendered terrain, not a flat plane");
                    }
                    const auto blocker=frame.World(plan.keys[5].contacts[2]);
                    auto blocked=worldObstacles;
                    blocked.push_back({blocker.x,blocker.z,.09f});
                    const auto denied=island::MonkeyAttachmentSite::Prepare(state.selectedTree,frame,
                        monkeyAttachmentTerrain,blocked,3.0f);
                    Check(!denied.plan && denied.failure==island::MonkeyAttachmentSite::Failure::Obstacle,
                        "an extra obstacle at a staging foot rejects attachment before lifting supports");
                } else ++attachmentFailures[static_cast<std::size_t>(prepared.failure)];
            }
            Check(state.treeIndex == repeated.treeIndex,
                  "repeated monkey simulation selects the same actual canopy tree");
            if (state.behavior == island::MonkeyBehavior::ApproachTree
                || state.behavior == island::MonkeyBehavior::InspectTree) {
                Check(state.treeIndex >= 0 && static_cast<std::size_t>(state.treeIndex) < jungle.Canopy().size(),
                      "tree behaviour references an installed canopy placement");
                if (state.treeIndex >= 0) {
                    Check(island::MonkeyTroop::IsClosedCanopyTree(
                              jungle.Canopy(), static_cast<std::size_t>(state.treeIndex)),
                          "monkey selected an isolated or mangrove tree outside closed canopy");
                }
                observedMonkeyTreeApproach = true;
                if (state.behavior == island::MonkeyBehavior::InspectTree) {
                    if (!observedMonkeyTreeArrival) {
                        auto interrupted = monkeys;
                        interrupted.Update(.1f,state.x,state.y,state.z);
                        Check(interrupted.Monkeys()[index].behavior == island::MonkeyBehavior::RunAway
                            && interrupted.Monkeys()[index].treeIndex == -1,
                            "player proximity interrupts tree inspection and releases the tree");
                        auto removed = monkeys;
                        removed.SetCanopy({});
                        Check(removed.Monkeys()[index].treeIndex == -1
                            && removed.Monkeys()[index].behavior == island::MonkeyBehavior::Idle,
                            "replacing canopy placements clears obsolete tree identities");
                        auto invalidTree = jungle.Canopy().front();
                        invalidTree.scale = quietNan;
                        bool rejected = false;
                        try { removed.SetCanopy(std::span<const island::CanopyPlacement>(&invalidTree,1)); }
                        catch (const std::invalid_argument&) { rejected = true; }
                        Check(rejected,"non-finite canopy placement is rejected before installation");
                    }
                    observedMonkeyTreeArrival = true;
                    const auto& tree = jungle.Canopy()[static_cast<std::size_t>(state.treeIndex)];
                    Check(std::hypot(state.x-tree.x,state.z-tree.z)
                        >= island::MonkeyTreeReach::StandOff(tree.scale,state.scale)-.121f,
                        "tree staging remains outside the flared root with body clearance");
                    Check(std::hypot(state.x-state.targetX,state.z-state.targetZ) < .121f
                        && state.speed == 0,
                        "tree inspection starts only after reaching the staging point and stopping");
                }
            } else {
                Check(state.treeIndex == -1,"ordinary ground behaviour releases its selected tree");
            }
            const island::TerrainSample sample = terrain.Sample(state.x, state.z);
            Check(state.x == repeated.x && state.y == repeated.y
                      && state.z == repeated.z
                      && state.headingRadians == repeated.headingRadians
                      && state.headLookRadians == repeated.headLookRadians
                      && state.speed == repeated.speed
                      && state.animationSeconds == repeated.animationSeconds
                      && state.gaitPhase == repeated.gaitPhase
                      && state.gaitAmount == repeated.gaitAmount
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
            Check(state.gaitPhase >= 0.0f && state.gaitPhase < 1.0f
                      && state.gaitAmount >= 0.0f && state.gaitAmount <= 1.0f,
                  "distance-driven monkey gait remains bounded over all simulation tiers");
            if (state.x == previousMonkeys[index].x && state.z == previousMonkeys[index].z) {
                Check(state.gaitPhase == previousMonkeys[index].gaitPhase,
                      "stationary monkeys do not advance the ground stride");
            }
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
                    const auto& previous = previousMonkeys[index];
                    const float dx = state.x-previous.x, dz = state.z-previous.z;
                    const float lengthSquared = dx*dx+dz*dz;
                    const float t = lengthSquared > 0 ? std::clamp(
                        ((obstacle.x-previous.x)*dx+(obstacle.z-previous.z)*dz)/lengthSquared,
                        0.0f,1.0f) : 0.0f;
                    Check(std::hypot(previous.x+t*dx-obstacle.x,previous.z+t*dz-obstacle.z)
                              >= minimumDistance-.001f,
                          "sampled monkey movement segments do not cross solid proxies");
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
    std::cout << "Monkey attachment sites " << attachmentAccepted << '/' << attachmentCandidates << "; rejections";
    for (std::size_t reason=0;reason<attachmentFailures.size();++reason)
        std::cout << ' ' << reason << ':' << attachmentFailures[reason];
    std::cout << '\n';
    Check(attachmentAccepted>0,"naturally reached real-island trees include a candidate passing terrain/contact/body-probe checks");
    std::cout << "Monkey full excursion reach " << excursionAccepted << '/' << attachmentAccepted << '\n';
    Check(excursionAccepted>0,"actual island attachment sites include reachable ascent and descent, not only a first grip");
    std::cout << "Monkey full excursion clearance " << excursionClear << '/' << attachmentAccepted << '\n';
    std::cout << "Monkey preflight milliseconds: attachment mean/max "
        << attachmentMillis/std::max(1U,attachmentCandidates) << '/' << attachmentMaxMillis
        << "; excursion mean/max " << clearanceMillis/std::max(1U,attachmentAccepted)
        << '/' << clearanceMaxMillis << '\n';
    std::cout << "Monkey preflight 16-pose chunk maximum milliseconds " << clearanceChunkMaxMillis << '\n';
    std::cout << "Monkey attachment 16-pose chunk maximum milliseconds " << attachmentChunkMaxMillis << '\n';
    Check(observedPreparing,"live preparation is exercised across updates instead of blocking on the whole trajectory");
    Check(excursionClear>0,"natural island arrivals include an excursion passing full-cycle clearance probes");
    Check(observedLiveClimb && observedLiveSummit && checkedLiveReturn,
        "ordinary island simulation actually climbs, reaches summit and exercises safe interruption");
    Check(observedMonkeyTreeApproach && observedMonkeyTreeArrival,
          "real-island simulation approaches and reaches actual canopy trees");

    Check(island::OceanMeshLayout::VertexCount == 25921U,
          "ocean grid remains within the portable 16-bit vertex budget");
    Check(island::OceanMeshLayout::TriangleCount == 51200U,
          "ocean grid triangle count is deterministic");
    Check(island::OceanMeshLayout::IsPortable(fullQuality.oceanQuadsPerAxis)
              && island::OceanMeshLayout::IsPortable(mobileQuality.oceanQuadsPerAxis),
          "both runtime ocean grids fit portable 16-bit indices");
    Check(!island::OceanMeshLayout::IsPortable(0)
              && island::OceanMeshLayout::IsPortable(255)
              && !island::OceanMeshLayout::IsPortable(256)
              && !island::OceanMeshLayout::IsPortable(std::numeric_limits<int>::max()),
          "runtime ocean validation accepts the exact 16-bit grid boundary");
    Check(island::OceanMeshLayout::VertexCountFor(mobileQuality.oceanQuadsPerAxis) == 16641U
              && island::OceanMeshLayout::TriangleCountFor(
                     mobileQuality.oceanQuadsPerAxis) == 32768U,
          "mobile ocean grid has the audited reduced geometry budget");
    CheckNear(island::OceanMeshLayout::Coordinate(
                  mobileQuality.oceanQuadsPerAxis, mobileQuality.oceanQuadsPerAxis),
              island::OceanMeshLayout::Extent, 0.0001f,
              "mobile ocean grid reaches the shared outer extent");
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
    const island::TerrainSample invalidTerrainSample = terrain.Sample(
        quietNan, std::numeric_limits<float>::infinity());
    Check(invalidTerrainSample.height == -22.0f
              && invalidTerrainSample.signedShoreDistance
                  == -island::IslandTerrain::MaximumQueryCoordinate
              && invalidTerrainSample.normal.x == 0.0f
              && invalidTerrainSample.normal.y == 1.0f
              && invalidTerrainSample.normal.z == 0.0f
              && invalidTerrainSample.slopeRadians == 0.0f
              && invalidTerrainSample.surface == island::TerrainSurface::DeepSeabed,
          "non-finite terrain coordinates fail closed to a finite deep-seabed sample");
    Check(terrain.Height(std::numeric_limits<float>::max(), 0.0f) == -22.0f
              && terrain.SignedShoreDistance(
                     0.0f, -island::IslandTerrain::MaximumQueryCoordinate - 1.0f)
                  == -island::IslandTerrain::MaximumQueryCoordinate
              && !terrain.IsLand(quietNan, 0.0f),
          "out-of-domain terrain queries avoid procedural-noise conversion and remain water");
    CheckNear(terrain.ShoreRadius(quietNan), terrain.ShoreRadius(0.0f), 0.0001f,
              "non-finite shoreline angles use the deterministic zero-angle profile");

    const island::TerrainSample centreSample = terrain.Sample(0.0f, 0.0f);
    const float centreNormalLength = std::hypot(
        centreSample.normal.y, std::hypot(centreSample.normal.x, centreSample.normal.z));
    CheckNear(centreNormalLength, 1.0f, 0.0001f, "terrain sample normal is normalized");
    Check(centreSample.normal.y > 0.0f, "terrain sample normal points upward");
    Check(centreSample.surface == island::TerrainSurface::InteriorGround
              || centreSample.surface == island::TerrainSurface::SteepGround,
          "island centre has an interior ground classification");

    const auto regionalSummit = [&](int minX, int maxX, int minZ, int maxZ) {
        float highest = 0.0f;
        for (int z = minZ; z <= maxZ; z += 2)
            for (int x = minX; x <= maxX; x += 2)
                highest = std::max(highest, terrain.Height(static_cast<float>(x), static_cast<float>(z)));
        return highest;
    };
    const float mainSummit = regionalSummit(-90, -20, 20, 86);
    const float secondarySummit = regionalSummit(44, 104, -60, 0);
    const float saddleHeight = terrain.Height(10.0f, 5.0f);
    std::cout << "Terrain summits/saddle: " << mainSummit << '/' << secondarySummit
              << '/' << saddleHeight << '\n';
    Check(mainSummit >= 35.0f && mainSummit <= 45.0f,
          "main ridge has an island-scale 35 to 45 metre summit");
    Check(secondarySummit >= 25.0f && secondarySummit <= 30.0f,
          "second ridge has a distinct 25 to 30 metre summit");
    Check(saddleHeight > 6.0f && saddleHeight < secondarySummit - 6.0f,
          "a lower inland saddle separates the two ridges");
    std::cout << "South valley left/bed/right: " << terrain.Height(-31.0f, -70.0f)
              << '/' << terrain.Height(-6.0f, -70.0f) << '/' << terrain.Height(19.0f, -70.0f) << '\n';
    Check(terrain.Height(-6.0f, -70.0f) + 1.0f < terrain.Height(-31.0f, -70.0f)
              && terrain.Height(-6.0f, -70.0f) + 1.0f < terrain.Height(19.0f, -70.0f),
          "southern drainage has shoulders on both sides rather than a uniform dome");
    Check(terrain.Height(-6.0f, -70.0f) > terrain.Height(5.0f, -175.0f) + 2.0f,
          "southern drainage descends toward the coastal plain");

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
    Check(minimumRadius > 150.0f, "sheltered bay preserves a broad connected island interior");
    Check(maximumRadius < 275.0f, "shoreline remains near the 500 metre target");
    Check(maximumRadius - minimumRadius > 20.0f, "shoreline silhouette has variation");
    const float westBayRadius = terrain.ShoreRadius(std::numbers::pi_v<float>);
    Check(terrain.ShoreRadius(2.48f) > westBayRadius + 35.0f
              && terrain.ShoreRadius(-2.48f) > westBayRadius + 35.0f,
          "western sheltered bay sits between two pronounced headlands");
    Check(terrain.ShoreRadius(0.35f) > terrain.ShoreRadius(1.0f) + 12.0f,
          "windward coast has a separate eastern promontory");
    CheckNear(terrain.ShoreRadius(-std::numbers::pi_v<float>), westBayRadius, 0.001f,
              "sheltered bay is continuous across the polar seam");
    for (const float headlandAngle : {2.48f, -2.48f}) {
        const float tipX = std::cos(headlandAngle) * terrain.ShoreRadius(headlandAngle);
        Check(-westBayRadius > tipX + 25.0f,
              "bay shore is recessed at least 25 metres behind both headland tips in world space");
    }
    std::cout << "Coast radius min/max/bay: " << minimumRadius << '/' << maximumRadius
              << '/' << westBayRadius << '\n';
    const auto coastalStation = [&](float angle, float inland) {
        const float radius = terrain.ShoreRadius(angle) - inland;
        return terrain.Sample(std::cos(angle) * radius, std::sin(angle) * radius);
    };
    const auto rockyStation = coastalStation(0.35f, 12.0f);
    const auto bayStation = coastalStation(std::numbers::pi_v<float>, 12.0f);
    Check(rockyStation.coastalRockiness > 0.90f && bayStation.coastalRockiness < 0.05f,
          "coastal environment distinguishes headland and bay at the same shore distance");
    Check(rockyStation.height > bayStation.height + 6.0f,
          "rocky headland has actual elevated relief, not only a material tint");
    Check(rockyStation.surface == island::TerrainSurface::RockyGround,
          "rocky headland is not classified as sandy habitat");
    Check(rockyStation.beachSuitability < 0.1f && bayStation.beachSuitability > 0.9f,
          "sheltered bay and exposed rock have distinct beach suitability");
    const float bayWidth = island::IslandEnvironmentField::BeachWidth(std::numbers::pi_v<float>);
    const float easternWidth = island::IslandEnvironmentField::BeachWidth(0.35f);
    Check(bayWidth > 34.0f && bayWidth < 37.0f && easternWidth < 8.0f,
          "dominant bay beach is substantially wider than the exposed rocky margin");
    const auto backBayBeach = coastalStation(std::numbers::pi_v<float>, 25.0f);
    Check(backBayBeach.beachSuitability > 0.45f && backBayBeach.height < 2.0f,
          "wide bay beach has both sandy material potential and low physical relief");
    const auto coastalHeight = [&](const float angle, const float inlandDistance) {
        const float radius = terrain.ShoreRadius(angle) - inlandDistance;
        return terrain.Height(std::cos(angle) * radius, std::sin(angle) * radius);
    };
    const float southBeachAngle = -std::numbers::pi_v<float> * 0.5f;
    const float lowerSouthBeach = coastalHeight(southBeachAngle, 3.0f);
    const float bermSouthBeach = coastalHeight(southBeachAngle, 9.0f);
    const float upperSouthBeach = coastalHeight(southBeachAngle, 15.0f);
    Check(bermSouthBeach > 0.5f * (lowerSouthBeach + upperSouthBeach) + 0.25f,
          "broad beach has a physical sub-metre berm and dune shoulder");
    const float lowerBay = coastalHeight(std::numbers::pi_v<float>, 1.0f);
    const float washBay = coastalHeight(std::numbers::pi_v<float>, 3.0f);
    const float upperBay = coastalHeight(std::numbers::pi_v<float>, 5.0f);
    Check(washBay + 0.01f < std::min(lowerBay, upperBay),
          "sheltered beach has a shallow terrain-authored wash channel");
    Check(coastalHeight(0.35f, 12.0f) > coastalHeight(0.35f, 15.0f) + 0.04f,
          "rocky headland has a walkable sub-metre shelf step");
    CheckNear(coastalHeight(southBeachAngle, 0.0f), 0.0f, 0.0001f,
              "coastal micro-relief preserves the exact swimming shoreline");
    CheckNear(island::IslandEnvironmentField::BeachWidth(-std::numbers::pi_v<float>), bayWidth,
              0.0001f, "bay beach width stays continuous across the polar seam");
    const auto coastalExperienceStation = [&](const float angle, const float inland) {
        const float radius=terrain.ShoreRadius(angle)-inland;
        const float x=std::cos(angle)*radius,z=std::sin(angle)*radius;
        return island::IslandEnvironmentField::SampleCoastalExperience(
            x,z,terrain.SignedShoreDistance(x,z));
    };
    const auto broadBeachExperience=coastalExperienceStation(
        -std::numbers::pi_v<float>*.5f,12.0f);
    const auto bayExperience=coastalExperienceStation(std::numbers::pi_v<float>,12.0f);
    const auto headlandExperience=coastalExperienceStation(.35f,12.0f);
    const float mouthX=island::StreamRoute::SourceX-island::StreamRoute::NominalLength;
    const float mouthZ=island::StreamRoute::CentreZ(mouthX);
    const auto mouthExperience=island::IslandEnvironmentField::SampleCoastalExperience(
        mouthX,mouthZ,terrain.SignedShoreDistance(mouthX,mouthZ));
    const auto broadBeachDebris=island::IslandEnvironmentField::SampleCoastalDebris(
        0.0f,-terrain.ShoreRadius(-std::numbers::pi_v<float>*.5f)+12.0f,
        coastalStation(-std::numbers::pi_v<float>*.5f,12.0f).signedShoreDistance);
    const float bayDebrisRadius=terrain.ShoreRadius(std::numbers::pi_v<float>)-12.0f;
    const auto bayDebris=island::IslandEnvironmentField::SampleCoastalDebris(
        -bayDebrisRadius,0.0f,bayStation.signedShoreDistance);
    const float headlandDebrisRadius=terrain.ShoreRadius(.35f)-12.0f;
    const auto headlandDebris=island::IslandEnvironmentField::SampleCoastalDebris(
        std::cos(.35f)*headlandDebrisRadius,std::sin(.35f)*headlandDebrisRadius,
        rockyStation.signedShoreDistance);
    Check(broadBeachExperience.broadBeach>.90f
            && broadBeachExperience.broadBeach>broadBeachExperience.shelteredBay,
        "southern Robinson beach has a dominant shared coastal identity");
    Check(bayExperience.shelteredBay>.75f
            && bayExperience.shelteredBay>bayExperience.mangroveMouth,
        "protected western bay remains distinct from its localized stream mouth");
    Check(headlandExperience.rockyShore>.90f,
        "exposed eastern headland has a dominant rocky-shore identity");
    Check(mouthExperience.mangroveMouth>.80f
            && mouthExperience.mangroveMouth>mouthExperience.shelteredBay,
        "stream outlet has a localized mangrove-mouth coastal identity");
    Check(bayDebris.windExposure<headlandDebris.windExposure*.45f
            && bayDebris.shelter>.70f && headlandDebris.shelter<.10f,
        "shared debris field distinguishes leeward bay shelter from windward headland exposure");
    Check(bayDebris.accumulation>.70f
            && broadBeachDebris.accumulation>.10f
            && broadBeachDebris.accumulation<bayDebris.accumulation
            && headlandDebris.accumulation<.05f,
        "shared debris accumulation is strongest in the bay and minimal on rocky headlands");
    Check(island::TidePoolLayout::Definitions.size()==5U,
        "rocky shore has a small bounded tide-pool set");
    for (std::size_t index=0;index<island::TidePoolLayout::Definitions.size();++index) {
        const auto& pool=island::TidePoolLayout::Definitions[index];
        const auto [x,z]=island::TidePoolLayout::Centre(terrain,index);
        const auto centre=terrain.Sample(x,z);
        const float water=island::TidePoolLayout::WaterLevel(terrain,index);
        Check(centre.signedShoreDistance>=23.5f && centre.signedShoreDistance<=34.5f
                && centre.coastalRockiness>.70f,
            "tide-pool basin remains on the authored rocky shore");
        CheckNear(island::TidePoolLayout::Depression(terrain,x,z),pool.depth,.0001f,
            "tide-pool centre receives its complete authored depression");
        Check(water>centre.height+.15f && water<centre.height+pool.depth-.12f,
            "tide-pool water lies above its floor and below the undepressed rim");
        Check(island::TidePoolLayout::WaterRadius(index)>1.0f
                && island::TidePoolLayout::WaterRadius(index)<pool.radius,
            "tide-pool water disc remains visibly broad and inside its basin");
        for (std::size_t other=index+1;other<island::TidePoolLayout::Definitions.size();++other) {
            const auto [otherX,otherZ]=island::TidePoolLayout::Centre(terrain,other);
            Check(std::hypot(x-otherX,z-otherZ)>pool.radius
                    +island::TidePoolLayout::Definitions[other].radius+3.0f,
                "tide-pool basins remain separated rocky-shore landmarks");
        }
    }
    for (const auto& palm : vegetation.Palms())
        Check(terrain.Sample(palm.x, palm.z).coastalRockiness <= 0.45f,
              "adult and juvenile palms avoid exposed coastal rock");
    const auto checkCoastalRoots = [&](const auto& plants, float maximumRockiness) {
        for (const auto& plant : plants)
            Check(terrain.Sample(plant.x, plant.z).coastalRockiness <= maximumRockiness,
                  "jungle roots respect shared coastal rock exclusions");
    };
    checkCoastalRoots(jungle.Canopy(), 0.45f);
    checkCoastalRoots(jungle.Understory(), 0.60f);
    checkCoastalRoots(jungle.Bushes(), 0.60f);
    checkCoastalRoots(jungle.GroundPlants(), 0.60f);
    const auto invalidCoast = island::IslandEnvironmentField::SampleCoast(quietNan, 0.0f, 0.0f);
    Check(invalidCoast.rockiness == 0.0f && invalidCoast.beachSuitability == 0.0f,
          "non-finite environment query has safe empty controls");
    const auto invalidExperience=island::IslandEnvironmentField::SampleCoastalExperience(
        quietNan,0.0f,0.0f);
    Check(invalidExperience.broadBeach==0.0f && invalidExperience.shelteredBay==0.0f
            && invalidExperience.rockyShore==0.0f && invalidExperience.mangroveMouth==0.0f,
        "non-finite coastal-experience query has safe empty controls");
    const auto invalidDebris=island::IslandEnvironmentField::SampleCoastalDebris(
        quietNan,0.0f,0.0f);
    Check(invalidDebris.windExposure==0.0f && invalidDebris.shelter==0.0f
            && invalidDebris.accumulation==0.0f,
        "non-finite coastal-debris query has safe empty controls");
    const float streamX=-110.0f,streamZ=island::StreamRoute::CentreZ(streamX);
    CheckNear(island::StreamRoute::CentreZ(island::StreamRoute::SourceX), -40.05f,
        .0001f, "stream meander preserves the authored source connection");
    Check(std::abs(streamZ-(-55.0f-.23f*streamX))>1.0f,
        "shared stream centreline has a material mid-course meander");
    CheckNear(island::StreamRoute::HalfWidth(island::StreamRoute::SourceX), .45f,
        .0001f, "stream source retains its narrow authored connection");
    CheckNear(island::StreamRoute::HalfWidth(
        island::StreamRoute::SourceX-island::StreamRoute::NominalLength), 1.5f,
        .0001f, "stream mouth retains its broad authored connection");
    float positiveWidthVariation=0.0f,negativeWidthVariation=0.0f;
    float positiveMeander=0.0f,negativeMeander=0.0f;
    float minimumLeftBankScale=1.0f,maximumLeftBankScale=1.0f;
    float minimumRightBankScale=1.0f,maximumRightBankScale=1.0f;
    for(int along=0;along<=98;++along) {
        const float routeX=island::StreamRoute::SourceX-static_cast<float>(along);
        const float routeOffset=island::StreamRoute::CentreZ(routeX)-(-55.0f-.23f*routeX);
        positiveMeander=std::max(positiveMeander,routeOffset);
        negativeMeander=std::min(negativeMeander,routeOffset);
        const float t=static_cast<float>(along)/island::StreamRoute::NominalLength;
        const float shaped=t*t*(3.0f-2.0f*t);
        const float baseline=std::lerp(.45f,1.5f,shaped);
        const float width=island::StreamRoute::HalfWidth(
            island::StreamRoute::SourceX-static_cast<float>(along));
        const float leftScale=island::StreamRoute::BankWidthScale(
            static_cast<float>(along),-1.0f);
        const float rightScale=island::StreamRoute::BankWidthScale(
            static_cast<float>(along),1.0f);
        Check(std::isfinite(width)&&width>=.45f&&width<=1.56f,
            "stream pools and pinches stay inside the authored width budget");
        Check(std::isfinite(leftScale)&&std::isfinite(rightScale)
                &&leftScale>=.78f&&leftScale<=1.22f
                &&rightScale>=.78f&&rightScale<=1.22f,
            "independent stream-bank width rhythms stay bounded");
        positiveWidthVariation=std::max(positiveWidthVariation,width-baseline);
        negativeWidthVariation=std::min(negativeWidthVariation,width-baseline);
        minimumLeftBankScale=std::min(minimumLeftBankScale,leftScale);
        maximumLeftBankScale=std::max(maximumLeftBankScale,leftScale);
        minimumRightBankScale=std::min(minimumRightBankScale,rightScale);
        maximumRightBankScale=std::max(maximumRightBankScale,rightScale);
    }
    Check(positiveWidthVariation>.16f&&negativeWidthVariation<-.31f,
        "stream profile contains material low-frequency pools and pinches");
    Check(positiveMeander>2.0f&&negativeMeander<-2.8f,
        "stream route contains broad visible bends on both sides of its valley axis");
    Check(minimumLeftBankScale<.90f&&maximumLeftBankScale>1.09f
              &&minimumRightBankScale<.91f&&maximumRightBankScale>1.08f
              &&std::abs(island::StreamRoute::BankWidthScale(42.0f,-1.0f)
                          -island::StreamRoute::BankWidthScale(42.0f,1.0f))>.02f,
        "stream banks retain independent width rhythms instead of mirrored edges");
    const auto streamCentre=island::IslandEnvironmentField::SampleInland(
        streamX,streamZ,terrain.SignedShoreDistance(streamX,streamZ));
    const auto streamBank=island::IslandEnvironmentField::SampleInland(
        streamX,streamZ+14.0f,terrain.SignedShoreDistance(streamX,streamZ+14.0f));
    const auto dryInterior=island::IslandEnvironmentField::SampleInland(
        105.0f,80.0f,terrain.SignedShoreDistance(105.0f,80.0f));
    const float wetlandShore=terrain.SignedShoreDistance(
        island::IslandEnvironmentField::SeasonalWetlandX,
        island::IslandEnvironmentField::SeasonalWetlandZ);
    const auto seasonalWetland=island::IslandEnvironmentField::SampleInland(
        island::IslandEnvironmentField::SeasonalWetlandX,
        island::IslandEnvironmentField::SeasonalWetlandZ,wetlandShore);
    Check(streamCentre.drainage>0.95f && streamCentre.moisture>0.95f
        && streamBank.moisture>streamBank.drainage && streamBank.moisture>0.35f
        && dryInterior.moisture<0.15f && dryInterior.drainage<0.15f,
        "shared inland field distinguishes wet channel, damp bank and dry interior");
    Check(island::IslandEnvironmentField::SeasonalWetlandPotential(
              island::IslandEnvironmentField::SeasonalWetlandX,
              island::IslandEnvironmentField::SeasonalWetlandZ,wetlandShore)>.95f
            && seasonalWetland.moisture>.90f
            && seasonalWetland.drainage>.55f&&seasonalWetland.drainage<.70f,
        "seasonal inland depression has damp non-flowing environmental controls");
    Check(island::IslandEnvironmentField::SeasonalWetlandPotential(
              island::IslandEnvironmentField::SeasonalWetlandX+26.0f,
              island::IslandEnvironmentField::SeasonalWetlandZ,wetlandShore)==0.0f,
        "seasonal wetland stays local instead of becoming a second valley");
    const auto invalidInland=island::IslandEnvironmentField::SampleInland(quietNan,0,0);
    Check(invalidInland.moisture==0 && invalidInland.drainage==0
        && invalidInland.saltExposure==0,"non-finite inland environment query is safely empty");
    for(int x=-170;x<=130;x+=10)for(int z=-190;z<=170;z+=10) {
        const auto field=island::IslandEnvironmentField::SampleInland(
            static_cast<float>(x),static_cast<float>(z),terrain.SignedShoreDistance(x,z));
        Check(std::isfinite(field.moisture)&&field.moisture>=0&&field.moisture<=1
            && std::isfinite(field.drainage)&&field.drainage>=0&&field.drainage<=1
            && std::isfinite(field.saltExposure)&&field.saltExposure>=0&&field.saltExposure<=1,
            "inland environment controls stay finite and normalized across the island");
    }
    for (int angularIndex = 0; angularIndex < 180; ++angularIndex) {
        const float angle = angularIndex * std::numbers::pi_v<float> / 90.0f;
        const float width = island::IslandEnvironmentField::BeachWidth(angle);
        Check(std::isfinite(width) && width >= 6.0f && width <= 36.0f,
              "authored beach width stays finite and within coastal scale");
        for (float inland : {-20.0f, 0.0f, 12.0f, 40.0f, 80.0f}) {
            const auto station = coastalStation(angle, inland);
            Check(std::isfinite(station.coastalRockiness) && station.coastalRockiness >= 0.0f
                      && station.coastalRockiness <= 1.0f && station.beachSuitability >= 0.0f
                      && station.beachSuitability <= 1.0f,
                  "coastal environment controls remain finite and normalized");
            if (inland == 80.0f)
                Check(station.coastalRockiness == 0.0f,
                      "coastal rock field does not override interior ridge geology");
            const float radius=terrain.ShoreRadius(angle)-inland;
            const auto experience=island::IslandEnvironmentField::SampleCoastalExperience(
                std::cos(angle)*radius,std::sin(angle)*radius,
                station.signedShoreDistance);
            for (const float weight : {experience.broadBeach,experience.shelteredBay,
                                      experience.rockyShore,experience.mangroveMouth}) {
                Check(std::isfinite(weight) && weight>=0.0f && weight<=1.0f,
                    "coastal experience weights remain finite and normalized");
            }
        }
    }

    const island::VegetationLayout repeatedVegetation(terrain);
    Check(vegetation.Palms().size() >= 180U && vegetation.Palms().size() <= 520U,
          "palm population stays within the intended first performance budget");
    Check(vegetation.Palms().size() == repeatedVegetation.Palms().size(),
          "palm population is deterministic");
    Check(vegetation.Obstacles().size() >= vegetation.Palms().size(),
          "every palm has a trunk collision proxy and fallen stems may add visible-log proxies");
    CheckNear(island::VegetationLayout::PrimaryPathCenterX(
                  -island::VegetationLayout::PrimaryPathZOffset),
              island::VegetationLayout::PrimaryPathBaseX, 0.0001f,
              "primary path shape constants identify the authored centre crossing");
    Check(island::VegetationLayout::PrimaryPathMinimumZ
                  < island::VegetationLayout::PrimaryPathMaximumZ
              && island::VegetationLayout::PrimaryPathReservedHalfWidth >= 0.75f
              && island::VegetationLayout::PrimaryPathReservedHalfWidth <= 1.5f,
          "primary path keeps an ordered range and a 1.5 to 3 metre walking reserve");
    Check(island::VegetationLayout::IsReservedRoute(
              island::VegetationLayout::PlayerSpawnX,
              island::VegetationLayout::PlayerSpawnZ)
              && island::VegetationLayout::PlayerSpawnReserveRadius >= 12.0f,
          "authored beach spawn retains a useful vegetation and prop clearance");
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
    std::size_t youngPalms = 0U, fallenPalms = 0U, brokenPalms = 0U;
    for (std::size_t index = 0; index < vegetation.Palms().size(); ++index) {
        const island::PalmPlacement& palm = vegetation.Palms()[index];
        const island::PalmPlacement& repeated = repeatedVegetation.Palms()[index];
        const island::TerrainSample sample = terrain.Sample(palm.x, palm.z);
        Check(palm.x == repeated.x && palm.y == repeated.y && palm.z == repeated.z
                  && palm.yawRadians == repeated.yawRadians && palm.scale == repeated.scale
                  && palm.damageYawRadians == repeated.damageYawRadians
                  && palm.variant == repeated.variant && palm.condition == repeated.condition,
              "palm placement data is bitwise deterministic for one seed");
        Check(sample.signedShoreDistance >= 7.0f && sample.signedShoreDistance <= 158.0f,
              "palms remain inside the authored coastal/interior belt");
        Check(sample.slopeRadians <= 0.4712389f,
              "palms reject terrain steeper than the placement limit");
        Check(!island::VegetationLayout::IsReservedRoute(palm.x, palm.z),
              "palms keep paths, clearings, and player spawn open");
        Check(island::IslandEnvironmentField::SampleInland(
                  palm.x,palm.z,sample.signedShoreDistance).drainage<=0.68f,
              "palm trunks stay outside the active stream channel");
        Check(palm.variant < island::VegetationLayout::PalmVariantCount,
              "palm variant index remains in range");
        const bool youngPalm = palm.scale < island::VegetationLayout::PalmMinimumScale;
        Check(youngPalm
                  ? palm.scale >= island::VegetationLayout::PalmJuvenileMinimumScale
                      && palm.scale <= island::VegetationLayout::PalmJuvenileMaximumScale
                  : palm.scale <= island::VegetationLayout::PalmMaximumScale,
              "palm scale remains in the authored adult or juvenile range");
        youngPalms += youngPalm ? 1U : 0U;
        fallenPalms += palm.condition == island::PalmCondition::Fallen ? 1U : 0U;
        brokenPalms += palm.condition == island::PalmCondition::Broken ? 1U : 0U;
        Check(!youngPalm || palm.condition == island::PalmCondition::Living,
              "juvenile palms remain upright while only established adults carry storm history");
        Check(std::isfinite(palm.damageYawRadians),
              "palm storm orientation remains finite");
        if (palm.condition == island::PalmCondition::Fallen) {
            const float length = (8.6f + 0.45f * static_cast<float>(palm.variant)) * palm.scale;
            for (int step = 1; step <= 6; ++step) {
                const float distance = length * static_cast<float>(step) / 6.0f;
                const float x = palm.x + std::sin(palm.damageYawRadians) * distance;
                const float z = palm.z + std::cos(palm.damageYawRadians) * distance;
                const island::TerrainSample stemSample = terrain.Sample(x, z);
                Check(stemSample.signedShoreDistance >= 5.0f
                          && stemSample.slopeRadians <= 0.31f
                          && std::abs(stemSample.height - palm.y) <= 0.85f,
                      "fallen palm stem remains on stable dry terrain");
                Check(!island::VegetationLayout::OverlapsReservedRoute(x, z, 0.65f),
                      "fallen palm stem keeps the complete authored route open");
                for (const island::PalmPlacement& other : vegetation.Palms()) {
                    if (&other == &palm) continue;
                    const float clearance = step == 6 ? 2.2f : 0.85f;
                    Check(std::hypot(other.x - x, other.z - z) >= clearance,
                          "fallen palm stem and crown avoid standing palm trunks");
                }
            }
        }
        const island::CircularObstacle& obstacle = vegetation.Obstacles()[index];
        CheckNear(obstacle.x, palm.x, 0.0001f, "palm obstacle shares its visual x coordinate");
        CheckNear(obstacle.z, palm.z, 0.0001f, "palm obstacle shares its visual z coordinate");
        Check(obstacle.radius > (youngPalm ? 0.12f : 0.20f) && obstacle.radius < 0.45f,
              "palm collision radius stays close to the visible trunk");
    }
    Check(fallenPalms == island::VegetationLayout::FallenPalmTargetCount,
          "the default groves contain the bounded fallen-palm target");
    Check(brokenPalms == island::VegetationLayout::BrokenPalmTargetCount,
          "the default groves contain the bounded storm-broken-palm target");
    Check(vegetation.Obstacles().size()
              == vegetation.Palms().size() + fallenPalms * 5U,
          "each fallen stem adds five collision samples along its visible length");
    {
        // Groves on the coast, few palms inland, young palms beside adults.
        std::size_t grouped = 0U, inland = 0U;
        for (const island::PalmPlacement& palm : vegetation.Palms()) {
            int neighbours = 0;
            for (const island::PalmPlacement& other : vegetation.Palms()) {
                if (&other != &palm && std::hypot(other.x - palm.x, other.z - palm.z) < 12.0f) ++neighbours;
            }
            grouped += neighbours >= 3 ? 1U : 0U;
            inland += palm.shoreDistance > 90.0f ? 1U : 0U;
        }
        const double total = static_cast<double>(vegetation.Palms().size());
        Check(grouped / total >= 0.40 && inland / total <= 0.05,
              "palms form coastal groves with only a few old palms inland");
        Check(youngPalms >= 30U && youngPalms / total <= 0.35,
              "groves recruit young palms without replacing the adults");
        for (std::size_t a = 0; a < vegetation.Palms().size(); ++a) {
            for (std::size_t b = a + 1; b < vegetation.Palms().size(); ++b) {
                const auto& p = vegetation.Palms()[a];
                const auto& q = vegetation.Palms()[b];
                if (p.scale < island::VegetationLayout::PalmMinimumScale
                    || q.scale < island::VegetationLayout::PalmMinimumScale) {
                    Check(std::hypot(p.x - q.x, p.z - q.z) >= 2.2f, "young palms keep their trunks clear");
                }
            }
        }
    }

    {
        // Prevailing wind: exposure is bounded, strongest on the windward coast, fades inland,
        // never negative on the lee shore, and the palm lean stays within ten degrees.
        using namespace island::wind;
        Check(Exposure(230.0f, 0.0f, 5.0f) > 0.95f && Exposure(-230.0f, 0.0f, 5.0f) < 0.3f
                  && Exposure(-230.0f, 0.0f, 5.0f) > 0.2f && Exposure(0.0f, 0.0f, 120.0f) == 0.0f
                  && Exposure(0.0f, 230.0f, 5.0f) > Exposure(-230.0f, 0.0f, 5.0f),
              "wind exposure peaks on the windward east coast and vanishes inland");
        float leeMax = 0.0f, windwardMax = 0.0f;
        for (const island::PalmPlacement& palm : vegetation.Palms()) {
            const float lean = PalmLeanRadians(Exposure(palm.x, palm.z, palm.shoreDistance));
            Check(lean >= 0.0f && lean <= PalmMaximumLeanRadians, "palm lean stays within the authored limit");
            Check(std::abs(palm.shoreDistance - terrain.Sample(palm.x, palm.z).signedShoreDistance) < 0.001f,
                  "palm placements retain their shore distance");
            (palm.x > 0.0f ? windwardMax : leeMax) = std::max(palm.x > 0.0f ? windwardMax : leeMax, lean);
        }
        Check(windwardMax > leeMax && windwardMax > 0.12f, "east-coast palms lean more than west-coast palms");
        bool rejected = false;
        try { (void)Exposure(std::numeric_limits<float>::quiet_NaN(), 0.0f, 1.0f); }
        catch (const std::invalid_argument&) { rejected = true; }
        Check(rejected, "wind exposure rejects non-finite input");
        using island::sky_cloud::DriftAngle;
        CheckNear(DriftAngle(0.0f),0.0f,0.000001f,
                  "cloud drift begins at the authored fair-weather phase");
        CheckNear(DriftAngle(600.0f),0.21f,0.00001f,
                  "cloud drift remains very slow over a ten-minute walk");
        const float turnSeconds=2.0f*std::numbers::pi_v<float>
            / island::sky_cloud::AngularSpeedRadiansPerSecond;
        CheckNear(DriftAngle(turnSeconds),0.0f,0.00001f,
                  "cloud drift wraps on a shader-periodic complete turn");
        CheckNear(DriftAngle(std::numeric_limits<float>::quiet_NaN()),0.0f,0.000001f,
                  "cloud drift sanitizes non-finite clocks");
    }
    const island::GrassLayout grass(terrain, canopyLight, groundCover);
    const island::GrassLayout repeatedGrass(terrain, canopyLight, groundCover);
    {
        // The cached field bake and the direct reference agree exactly; cover stays bounded,
        // rises with light and patch potential, and vanishes on sand and steep slopes.
        const island::GroundCoverField repeatedCover;
        Check(std::ranges::equal(groundCover.PatchTexels(), repeatedCover.PatchTexels()),
              "ground-cover patch field is deterministic");
        int mismatches = 0;
        std::array<int, 2> extremes{};
        for (int row = 0; row < groundCover.Resolution(); row += 17) {
            for (int column = 0; column < groundCover.Resolution(); column += 13) {
                const float x = island::CanopyLightField::TexelCoordinate(column, groundCover.Resolution());
                const float z = island::CanopyLightField::TexelCoordinate(row, groundCover.Resolution());
                const auto expected = static_cast<std::uint8_t>(std::lround(255.0f
                    * island::GroundCover::PatchFromNoise(island::GroundCoverField::Noise(
                        x, z, island::GroundCoverField::DefaultSeed))));
                const auto texel = groundCover.PatchTexels()[static_cast<std::size_t>(row)
                    * groundCover.Resolution() + column];
                mismatches += texel != expected;
                extremes[0] += texel < 26;
                extremes[1] += texel > 229;
            }
        }
        Check(mismatches == 0, "cached ground-cover bake matches the direct noise reference");
        // GLSL ES 3.00 reserves identifiers that desktop Mesa GLSL 330 accepts; a reserved
        // parameter name compiled natively but sent the Web terrain to its stock fallback.
        for (const std::string& shader : {island::GroundCover::Shader(), island::TrunkSurface::Shader()}) {
            for (const char* reserved : {"patch", "sample", "filter", "input", "output", "active",
                                         "common", "partition", "resource", "subroutine"}) {
                const std::regex word("\\b" + std::string(reserved) + "\\b");
                Check(!std::regex_search(shader, word),
                      "generated GLSL avoids identifiers reserved by GLSL ES 3.00");
            }
        }
        Check(extremes[0] > 200 && extremes[1] > 200,
              "ground-cover patches include both sparse and rich ground");
        float previous = -1.0f;
        for (int step = 0; step <= 20; ++step) {
            const float value = static_cast<float>(step) / 20.0f;
            const float lit = island::GroundCover::Density(value, 1.0f, 80.0f, 0.1f);
            const float rich = island::GroundCover::Density(1.0f, value, 80.0f, 0.1f);
            Check(lit >= previous && lit >= 0.0f && lit <= 1.0f && rich >= 0.0f && rich <= 1.0f,
                  "ground cover is bounded and rises with canopy transmission");
            previous = lit;
        }
        Check(island::GroundCover::Density(1.0f, 1.0f, 10.0f, 0.1f) == 0.0f
                  && island::GroundCover::Density(1.0f, 1.0f, 80.0f, 0.6f) == 0.0f
                  && island::GroundCover::Density(0.0f, 1.0f, 80.0f, 0.1f)
                      < 0.3f * island::GroundCover::Density(1.0f, 1.0f, 80.0f, 0.1f),
              "ground cover vanishes on sand and steep slopes and thins in deep shade");
        float previousCoastalCover = 1.0f;
        for (int step = 0; step <= 20; ++step) {
            const float beach = static_cast<float>(step) / 20.0f;
            const float density = island::GroundCover::Density(1.0f, 1.0f, 40.0f, 0.1f, 0.0f, beach);
            Check(density >= 0.0f && density <= previousCoastalCover,
                  "shared grass and turf response decreases monotonically with sandy substrate");
            previousCoastalCover = density;
        }
        Check(island::GroundCover::Density(1.0f, 1.0f, 40.0f, 0.1f, 0.0f, 0.8f) == 0.0f
                  && island::GroundCover::Density(1.0f, 1.0f, 40.0f, 0.1f, 1.0f, 0.0f) == 0.0f
                  && island::GroundCover::Density(1.0f, 1.0f, 40.0f, 0.1f, 0.0f, 0.0f) == 1.0f,
              "sandy apron and bare rock suppress turf independently of distance from shore");
        const float dryCover=island::GroundCover::Density(1,1,80,.1f,0,0,0,0);
        const float dampCover=island::GroundCover::Density(1,1,80,.1f,0,0,1,0);
        const float channelCover=island::GroundCover::Density(1,1,80,.1f,0,0,1,1);
        Check(dampCover>=dryCover && dampCover<=1
            && channelCover<dryCover*.25f,
            "shared moisture supports bank cover while concentrated drainage suppresses channel grass");
        const auto wideBeach = coastalStation(std::numbers::pi_v<float>, 25.0f);
        const auto ordinaryBeach = coastalStation(-1.5f, 25.0f);
        const float bayCover = island::GroundCover::Density(1.0f, 1.0f, 25.0f, 0.1f,
            wideBeach.coastalRockiness, wideBeach.beachSuitability);
        const float ordinaryCover = island::GroundCover::Density(1.0f, 1.0f, 25.0f, 0.1f,
            ordinaryBeach.coastalRockiness, ordinaryBeach.beachSuitability);
        Check(bayCover < ordinaryCover * 0.5f,
              "wide bay suppresses turf more than ordinary coast at the same shore distance");
        for (const float invalid : {std::numeric_limits<float>::quiet_NaN(), -0.1f, 1.1f}) {
            bool rejected = false;
            try { (void)island::GroundCover::Density(invalid, 0.5f, 50.0f, 0.1f); }
            catch (const std::invalid_argument&) { rejected = true; }
            Check(rejected, "ground cover rejects invalid transmission");
            for (bool invalidRock : {false, true}) {
                rejected = false;
                try {
                    (void)island::GroundCover::Density(0.5f, 0.5f, 50.0f, 0.1f,
                        invalidRock ? invalid : 0.0f, invalidRock ? 0.0f : invalid);
                } catch (const std::invalid_argument&) { rejected = true; }
                Check(rejected, "ground cover rejects invalid coastal controls");
            }
        }
    }
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
    {
        // Redistribution, not inflation: rich sunlit ground carries several times the tufts
        // per candidate of patch-poor or shaded ground.
        double richSites = 0.0, poorSites = 0.0;
        int richPatches = 0, poorPatches = 0;
        for (float z = -240.0f; z <= 240.0f; z += 2.0f) {
            for (float x = -240.0f; x <= 240.0f; x += 2.0f) {
                const auto sample = terrain.Sample(x, z);
                if (sample.signedShoreDistance < 40.0f || sample.slopeRadians > 0.3f) continue;
                const auto environment=island::IslandEnvironmentField::SampleInland(
                    x,z,sample.signedShoreDistance);
                const float cover = island::GroundCover::Density(canopyLight.Sample(x, z),
                    groundCover.SamplePatch(x, z), sample.signedShoreDistance, sample.slopeRadians,
                    sample.coastalRockiness,sample.beachSuitability,
                    environment.moisture,environment.drainage);
                if (cover > 0.7f) richSites += 4.0;
                else if (cover < 0.2f) poorSites += 4.0;
            }
        }
        for (const auto& patch : grass.Patches()) {
            const auto sample = terrain.Sample(patch.x, patch.z);
            Check(sample.beachSuitability < island::GroundCover::BeachSuppressionEnd,
                  "grass parents and companions avoid the sandy apron even on reserved routes");
            const auto environment=island::IslandEnvironmentField::SampleInland(
                patch.x,patch.z,sample.signedShoreDistance);
            Check(environment.drainage<0.82f,"grass roots stay outside flowing stream water");
            if (sample.signedShoreDistance < 40.0f || sample.slopeRadians > 0.3f) continue;
            const float cover = island::GroundCover::Density(canopyLight.Sample(patch.x, patch.z),
                groundCover.SamplePatch(patch.x, patch.z), sample.signedShoreDistance, sample.slopeRadians,
                sample.coastalRockiness,sample.beachSuitability,
                environment.moisture,environment.drainage);
            if (cover > 0.7f) ++richPatches;
            else if (cover < 0.2f) ++poorPatches;
        }
        Check(richSites > 1000.0 && poorSites > 1000.0
                  && richPatches / richSites > 4.0 * (poorPatches / poorSites),
              "grass forms dense swards on rich ground and isolated tufts on poor ground");
    }
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
    Check(jungle.ForestColonies() == repeatedJungle.ForestColonies()
        && jungle.ForestGroundPlantCount() == repeatedJungle.ForestGroundPlantCount()
        && jungle.StreamBankGroundPlantCount() == repeatedJungle.StreamBankGroundPlantCount()
        && jungle.SeasonalWetlandGroundPlantCount()
            == repeatedJungle.SeasonalWetlandGroundPlantCount(),
        "forest colony membership and shape are deterministic for one seed");
    Check(jungle.Bushes().size() - jungle.ClearingBushCount() == 557
        && jungle.GroundPlants().size() - jungle.ClearingGroundPlantCount()
            - jungle.ForestGroundPlantCount() - jungle.StreamBankGroundPlantCount()
            - jungle.SeasonalWetlandGroundPlantCount() == 2134,
        "wet-biome, cave, bunker and helicopter reservations preserve measured deterministic populations");
    Check(jungle.StreamBankGroundPlantCount() >= 20
        && jungle.StreamBankGroundPlantCount() <= static_cast<std::size_t>(
            island::JungleLayout::StreamBankColonyCount
                * island::JungleLayout::StreamBankPlantsPerColony),
        "stream banks retain a bounded mixed fern and broadleaf population");
    const auto checkColonies = [&](const island::JungleLayout& layout) {
        Check(layout.ClearingBushCount() > 0 && layout.ClearingBushCount() <= 24
            && layout.ClearingGroundPlantCount() > 0 && layout.ClearingGroundPlantCount() <= 144,
            "clearing accents stay within the authored local population budget");
        const auto checkLayer = [&](const auto& plants, std::size_t count, float radius, float separation) {
            std::array<int, 2> clearingCounts{};
            for (std::size_t i = plants.size() - count; i < plants.size(); ++i) {
                const auto& plant = plants[i];
                const float footprint = radius * plant.scale;
                Check(!island::VegetationLayout::OverlapsReservedRoute(plant.x, plant.z, footprint),
                    "full animated clearing-plant footprint avoids reserved walking space");
                for (int step = 0; step < 64; ++step) {
                    const float a = step * 2 * std::numbers::pi_v<float> / 64;
                    Check(!island::VegetationLayout::IsReservedRoute(plant.x + footprint * std::cos(a),
                        plant.z + footprint * std::sin(a)), "independent footprint samples remain outside routes");
                }
                for (std::size_t j = 0; j < i; ++j)
                    Check(std::hypot(plant.x - plants[j].x, plant.z - plants[j].z) >= separation,
                        "colony roots avoid duplicate roots within their layer");
                bool inBand = false;
                int index = 0;
                for (auto clearing : {island::VegetationLayout::EastClearing, island::VegetationLayout::WestClearing}) {
                    const float distance = std::hypot(plant.x - clearing.x, plant.z - clearing.z) - clearing.radius;
                    if (distance >= footprint && distance <= 15.4f) { inBand = true; ++clearingCounts[index]; }
                    ++index;
                }
                Check(inBand, "colony changes remain confined to clearing-edge bands");
                for (const auto& tree : layout.Obstacles())
                    Check(std::hypot(plant.x - tree.x, plant.z - tree.z) >= footprint + tree.radius,
                        "clearing plant footprints avoid canopy trunks");
            }
            Check(clearingCounts[0] > 0 && clearingCounts[1] > 0,
                "both clearings receive accents without filling their centres");
        };
        checkLayer(layout.Bushes(), layout.ClearingBushCount(), island::JungleLayout::BushFootprintRadius, 2.8f);
        checkLayer(layout.GroundPlants(), layout.ClearingGroundPlantCount(), island::JungleLayout::GroundPlantFootprintRadius, 0.72f);
    };
    const auto checkForestColonies = [&](const island::JungleLayout& layout) {
        Check(!layout.ForestColonies().empty() && layout.ForestColonies().size()<layout.Canopy().size(),
            "forest-floor recruitment leaves some canopy trees without a colony");
        Check(layout.ForestGroundPlantCount()>200
            && layout.ForestGroundPlantCount()<=layout.Canopy().size()*island::JungleLayout::ForestPlantsPerColony,
            "forest companions have useful density within their per-tree candidate budget");
        Check(layout.GroundPlants().size()<=5000,"forest companions preserve the existing global floor budget");
        std::size_t next=layout.GroundPlants().size()-layout.ClearingGroundPlantCount()-layout.ForestGroundPlantCount();
        std::size_t previousTree=0;
        std::array<int,3> species{};
        std::size_t groups=0;
        for (const auto& colony:layout.ForestColonies()) {
            Check(colony.firstPlant==next && colony.plantCount>0
                && colony.plantCount<=island::JungleLayout::ForestPlantsPerColony,
                "forest colony membership is contiguous, nonempty and bounded");
            Check(colony.canopyIndex<layout.Canopy().size()
                && (groups==0 || colony.canopyIndex>previousTree),
                "forest colonies have one unique ordered canopy parent each");
            previousTree=colony.canopyIndex;
            const auto& tree=layout.Canopy()[colony.canopyIndex];
            const float parentDistance=std::hypot(colony.x-tree.x,colony.z-tree.z)/tree.scale;
            Check(parentDistance>=3.1999f && parentDistance<=7.2001f
                && colony.majorRadius>=2.5f && colony.majorRadius<=3.8f
                && colony.minorRadius>=1.1f && colony.minorRadius<=2.4f,
                "forest colonies remain associated with their parent's nearby floor");
            for (std::size_t index=colony.firstPlant;index<colony.firstPlant+colony.plantCount;++index) {
                const auto& plant=layout.GroundPlants()[index];
                const float dx=plant.x-colony.x,dz=plant.z-colony.z;
                const float u=(std::cos(colony.angle)*dx+std::sin(colony.angle)*dz)/colony.majorRadius;
                const float v=(-std::sin(colony.angle)*dx+std::cos(colony.angle)*dz)/colony.minorRadius;
                Check(u*u+v*v<=1.0001f,"forest recruitment stays in its authored irregular ellipse");
                const float footprint=island::JungleLayout::GroundPlantFootprintRadius*plant.scale;
                const auto sample=terrain.Sample(plant.x,plant.z);
                Check(sample.signedShoreDistance>=43 && sample.signedShoreDistance<=238
                    && sample.slopeRadians<=0.41887902f && sample.height>=2.5f
                    && plant.scale>=island::JungleLayout::GroundPlantMinimumScale && plant.scale<=0.94f,
                    "forest companions have suitable inland ground and restrained stature");
                Check(!island::VegetationLayout::OverlapsReservedRoute(plant.x,plant.z,footprint),
                    "forest companion full footprint leaves reserved routes open");
                for (int step=0;step<64;++step) {
                    const float angle=step*2*std::numbers::pi_v<float>/64;
                    Check(!island::VegetationLayout::IsReservedRoute(plant.x+footprint*std::cos(angle),
                        plant.z+footprint*std::sin(angle)),"independent forest footprint samples avoid paths");
                }
                for (const auto& clearing:{island::VegetationLayout::EastClearing,island::VegetationLayout::WestClearing})
                    Check(std::hypot(plant.x-clearing.x,plant.z-clearing.z)>=clearing.radius+16+footprint,
                        "forest companions do not overwrite clearing-edge composition");
                for (std::size_t other=0;other<index;++other)
                    Check(std::hypot(plant.x-layout.GroundPlants()[other].x,plant.z-layout.GroundPlants()[other].z)
                        >=island::JungleLayout::ForestRootSeparation,"forest companion roots have minimum separation");
                for (const auto& trunk:layout.Obstacles())
                    Check(std::hypot(plant.x-trunk.x,plant.z-trunk.z)>=footprint+trunk.radius,
                        "forest companions avoid solid trunks with their full footprints");
                for (const auto& bush:layout.Bushes())
                    Check(std::hypot(plant.x-bush.x,plant.z-bush.z)>=0.9f,"forest companions avoid shrub roots");
                for (const auto& plantAbove:layout.Understory())
                    Check(std::hypot(plant.x-plantAbove.x,plant.z-plantAbove.z)>=0.85f,
                        "forest companions avoid medium-plant roots");
                ++species[plant.variant];
            }
            next+=colony.plantCount;
            ++groups;
        }
        Check(next==layout.GroundPlants().size()-layout.ClearingGroundPlantCount(),
            "forest colony metadata covers every added plant exactly once");
        Check(species[0]>species[1] && species[1]>0 && species[2]>0,
            "forest groups retain all species with low rosettes predominant");
    };
    checkColonies(jungle);
    checkForestColonies(jungle);
    Check(jungle.ForestGroundPlantCount() >= 780U
        && jungle.ForestGroundPlantCount() <= 900U,
        "default forest-floor recruitment has the reviewed local density budget");
    Check(jungle.ClearingBushCount()==12 && jungle.ClearingGroundPlantCount()==60,
        "forest recruitment preserves the existing default clearing accents");
    for (const auto seed:{0x12345U,0xa55aU}) {
        const island::JungleLayout alternate(terrain,seed);
        checkColonies(alternate);
        checkForestColonies(alternate);
    }
    for (float z = -230; z < 196; z += 0.75f) {
        for (float offset : {-9.0f, -7.5f, 7.5f, 9.0f}) {
            const float x = island::VegetationLayout::PrimaryPathCenterX(z) + offset;
            constexpr float radius = 2;
            if (island::VegetationLayout::OverlapsReservedRoute(x, z, radius)) continue;
            for (int i = 0; i < 64; ++i) {
                const float a = i * 2 * std::numbers::pi_v<float> / 64;
                Check(!island::VegetationLayout::IsReservedRoute(x + radius * std::cos(a), z + radius * std::sin(a)),
                    "conservative route envelope covers bends and path ends");
            }
        }
    }
    Check(island::VegetationLayout::OverlapsReservedRoute(0, 0, -1)
        && island::VegetationLayout::OverlapsReservedRoute(std::numeric_limits<float>::quiet_NaN(), 0, 1),
        "invalid placement footprints fail closed");
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
    Check(jungle.Obstacles().size()
              == jungle.Canopy().size() + jungle.StandingSnags().size(),
          "every broadleaf canopy and standing snag has one collision proxy");
    std::array<std::size_t, island::JungleLayout::CanopyVariantCount> canopyFormCounts{};
    std::size_t emergentCount = 0U;
    std::size_t juvenileCount = 0U;
    std::size_t mangroveCount = 0U;
    for (std::size_t index = 0; index < jungle.Canopy().size(); ++index) {
        const island::CanopyPlacement& tree = jungle.Canopy()[index];
        const island::CanopyPlacement& repeated = repeatedJungle.Canopy()[index];
        const island::TerrainSample sample = terrain.Sample(tree.x, tree.z);
        const bool mangrove = tree.variant == island::MangroveCanopyVariant;
        Check(tree.x == repeated.x && tree.y == repeated.y && tree.z == repeated.z
                  && tree.yawRadians == repeated.yawRadians && tree.scale == repeated.scale
                  && tree.variant == repeated.variant,
              "broadleaf canopy placement is bitwise deterministic for one seed");
        Check(sample.signedShoreDistance >= (mangrove ? .4f : 43.0f)
                  && sample.signedShoreDistance <= (mangrove ? 24.0f : 238.0f),
              "canopy trees remain in their authored coastal or interior belt");
        Check(sample.height >= (mangrove ? -.05f : 3.8f)
                  && sample.height <= (mangrove ? 3.4f : 100.0f)
                  && sample.slopeRadians <= (mangrove ? .38f : .4712389f),
              "canopy trees remain on suitable ground for their habitat");
        Check(!island::VegetationLayout::IsReservedRoute(tree.x, tree.z),
              "broadleaf trees keep paths, clearings, and player spawn open");
        const auto environment = island::IslandEnvironmentField::SampleInland(
            tree.x, tree.z, sample.signedShoreDistance);
        Check(environment.drainage <= (mangrove ? .76f : .68f),
              "canopy trunks stay outside the active stream channel");
        if (mangrove) {
            Check(environment.moisture >= .30f && environment.saltExposure >= .06f
                      && sample.coastalRockiness <= .38f,
                  "mangroves remain in the localized damp brackish mouth habitat");
            for (const island::CircularObstacle& palm : vegetation.Obstacles()) {
                Check(std::hypot(tree.x - palm.x, tree.z - palm.z)
                          >= jungle.Obstacles()[index].radius + palm.radius,
                      "mangrove stilt roots do not intersect a palm collision footprint");
            }
            ++mangroveCount;
        }
        Check(tree.variant < island::JungleLayout::CanopyVariantCount,
              "broadleaf canopy variant index remains in range");
        ++canopyFormCounts.at(tree.variant);
        const bool emergent = !mangrove
            && tree.scale > island::JungleLayout::CanopyMaximumScale;
        const bool juvenile = !mangrove
            && tree.scale < island::JungleLayout::CanopyMinimumScale;
        Check(mangrove
                  ? tree.scale >= island::JungleLayout::MangroveMinimumScale
                      && tree.scale <= island::JungleLayout::MangroveMaximumScale
                  : juvenile
                      ? tree.scale >= island::JungleLayout::CanopyJuvenileMinimumScale
                          && tree.scale <= island::JungleLayout::CanopyJuvenileMaximumScale
                      : tree.scale >= island::JungleLayout::CanopyMinimumScale
                          && (tree.scale <= island::JungleLayout::CanopyMaximumScale
                              || (tree.scale >= island::JungleLayout::CanopyEmergentMinimumScale
                                  && tree.scale <= island::JungleLayout::CanopyEmergentScale)),
              "canopy scale remains in its authored juvenile, adult or emergent range");
        emergentCount += emergent ? 1U : 0U;
        juvenileCount += juvenile ? 1U : 0U;
        if (juvenile) {
            const float nominalHeight = island::CanopyNominalHeight(tree.variant) * tree.scale;
            Check(nominalHeight >= 2.0f && nominalHeight <= 5.0f,
                  "juvenile full crown envelope remains inside the requested 2-5m stature band");
        }
        if (emergent) {
            int standNeighbours = 0;
            for (const island::CanopyPlacement& other : jungle.Canopy()) {
                if (&other != &tree && std::hypot(other.x - tree.x, other.z - tree.z) < 20.0f) ++standNeighbours;
            }
            Check(standNeighbours >= 2, "emergent trees rise only inside closed stands");
        }
        const island::CircularObstacle& obstacle = jungle.Obstacles()[index];
        CheckNear(obstacle.x, tree.x, 0.0001f,
                  "broadleaf obstacle shares its visual x coordinate");
        CheckNear(obstacle.z, tree.z, 0.0001f,
                  "broadleaf obstacle shares its visual z coordinate");
        if (mangrove) {
            CheckNear(obstacle.radius, 1.08f * tree.scale, .0001f,
                "mangrove collision covers its visible stilt-root spread");
        } else {
            Check(obstacle.radius >= (juvenile ? .12f : .30f)
                      && obstacle.radius < 0.43f * island::JungleLayout::CanopyEmergentScale + 0.01f
                      && (emergent || obstacle.radius < 0.56f),
                  "broadleaf collision radius stays close to the visible trunk");
        }
    }
    Check(mangroveCount == jungle.MangroveCount() && mangroveCount >= 15
              && mangroveCount <= 30,
          "one localized mangrove colony stays inside its requested population budget");
    Check(jungle.StandingSnags().size()
              == static_cast<std::size_t>(island::JungleLayout::StandingSnagTargetCount)
              && jungle.StandingSnags().size() == repeatedJungle.StandingSnags().size(),
          "standing dead and broken trunks meet their bounded deterministic population");
    for (std::size_t index = 0; index < jungle.StandingSnags().size(); ++index) {
        const auto& snag = jungle.StandingSnags()[index];
        const auto& repeated = repeatedJungle.StandingSnags()[index];
        const auto sample = terrain.Sample(snag.x, snag.z);
        const auto environment = island::IslandEnvironmentField::SampleInland(
            snag.x, snag.z, sample.signedShoreDistance);
        const float rootRadius = .76f * snag.scale;
        Check(snag.x == repeated.x && snag.y == repeated.y && snag.z == repeated.z
                  && snag.yawRadians == repeated.yawRadians && snag.scale == repeated.scale
                  && snag.variant == repeated.variant,
              "standing snag placement is bitwise deterministic for one seed");
        Check(snag.variant < island::JungleLayout::StandingSnagVariantCount
                  && snag.scale >= island::JungleLayout::StandingSnagMinimumScale
                  && snag.scale <= island::JungleLayout::StandingSnagMaximumScale,
              "standing snag silhouette and scale remain inside the authored range");
        Check(sample.signedShoreDistance >= 58.0f && sample.signedShoreDistance <= 232.0f
                  && sample.height >= 4.0f && sample.slopeRadians <= .42f
                  && sample.coastalRockiness <= .46f && environment.drainage <= .67f,
              "standing snags remain on stable dry inland forest ground");
        Check(!island::VegetationLayout::OverlapsReservedRoute(
                  snag.x, snag.z, rootRadius),
              "standing snag collision footprint leaves routes and clearings open");
        int matureNeighbours = 0;
        for (const auto& tree : jungle.Canopy()) {
            const float distance = std::hypot(snag.x - tree.x, snag.z - tree.z);
            if (tree.variant != island::MangroveCanopyVariant
                && tree.scale >= island::JungleLayout::CanopyMinimumScale
                && distance >= 3.0f && distance <= 25.0f) ++matureNeighbours;
        }
        Check(matureNeighbours >= 2,
              "standing snags occur inside established multi-tree stands");
        for (std::size_t treeIndex = 0; treeIndex < jungle.Canopy().size(); ++treeIndex) {
            const auto& treeObstacle = jungle.Obstacles()[treeIndex];
            Check(std::hypot(snag.x - treeObstacle.x, snag.z - treeObstacle.z)
                      >= rootRadius + treeObstacle.radius + .85f,
                  "standing snag roots do not intersect living canopy trunks");
        }
        const auto& obstacle = jungle.Obstacles()[jungle.Canopy().size() + index];
        CheckNear(obstacle.x, snag.x, .0001f,
                  "standing snag obstacle shares its visual x coordinate");
        CheckNear(obstacle.z, snag.z, .0001f,
                  "standing snag obstacle shares its visual z coordinate");
        CheckNear(obstacle.radius, rootRadius, .0001f,
                  "standing snag collision covers its visible root flare");
    }
    {
        // Forest masses, not a park: most trees stand in closed groups, a few stand alone.
        std::size_t grouped = 0U, solitary = 0U;
        for (const island::CanopyPlacement& tree : jungle.Canopy()) {
            int neighbours = 0;
            for (const island::CanopyPlacement& other : jungle.Canopy()) {
                if (&other != &tree && std::hypot(other.x - tree.x, other.z - tree.z) < 20.0f) ++neighbours;
            }
            grouped += neighbours >= 3 ? 1U : 0U;
            solitary += neighbours == 0 ? 1U : 0U;
        }
        const double total = static_cast<double>(jungle.Canopy().size());
        Check(grouped / total >= 0.60 && solitary >= 3U && solitary / total <= 0.15,
              "canopy trees form closed stands with a sparse scatter of solitary trees");
        Check(emergentCount / total >= 0.03 && emergentCount / total <= 0.15,
              "a small fraction of canopy trees is emergent");
        Check(juvenileCount / total >= 0.04 && juvenileCount / total <= 0.15
                  && juvenileCount == jungle.JuvenileCanopyCount()
                  && juvenileCount == repeatedJungle.JuvenileCanopyCount(),
              "a small deterministic fraction of canopy trees is juvenile");
        for (const island::CanopyPlacement& tree : jungle.Canopy()) {
            if (tree.variant == island::MangroveCanopyVariant
                || tree.scale >= island::JungleLayout::CanopyMinimumScale) continue;
            const bool nearAdult = std::ranges::any_of(jungle.Canopy(), [&](const auto& other) {
                return &other != &tree && other.variant != island::MangroveCanopyVariant
                    && other.scale >= island::JungleLayout::CanopyMinimumScale
                    && std::hypot(other.x-tree.x,other.z-tree.z)<18.0f;
            });
            Check(nearAdult, "juvenile canopy trees remain beside established forest");
        }
    }
    Check(std::ranges::all_of(canopyFormCounts, [](std::size_t count) { return count >= 10; }),
          "every shared canopy form is represented across the island");
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
        Check(island::IslandEnvironmentField::SampleInland(
                  plant.x,plant.z,sample.signedShoreDistance).drainage<=0.82f,
              "understory stays outside flowing stream water");
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
        Check(island::IslandEnvironmentField::SampleInland(
                  bush.x,bush.z,sample.signedShoreDistance).drainage<=0.78f,
              "low bushes stay outside flowing stream water");
        Check(bush.variant < island::JungleLayout::BushVariantCount,
              "low-bush variant index remains in range");
        Check(bush.scale >= island::JungleLayout::BushMinimumScale
                  && bush.scale <= island::JungleLayout::BushMaximumScale,
              "low-bush scale remains in the authored range");
    }
    std::size_t wetBankPlantCount = 0;
    std::size_t seasonalWetlandPlantCount = 0;
    std::size_t wetBankNegativeSideCount = 0;
    std::size_t wetBankPositiveSideCount = 0;
    for (std::size_t index = 0; index < jungle.GroundPlants().size(); ++index) {
        const island::GroundPlantPlacement& plant = jungle.GroundPlants()[index];
        const island::GroundPlantPlacement& repeated = repeatedJungle.GroundPlants()[index];
        const island::TerrainSample sample = terrain.Sample(plant.x, plant.z);
        Check(plant.x == repeated.x && plant.y == repeated.y && plant.z == repeated.z
                  && plant.yawRadians == repeated.yawRadians && plant.scale == repeated.scale
                  && plant.variant == repeated.variant && plant.wetBank == repeated.wetBank
                  && plant.seasonalWetland == repeated.seasonalWetland,
              "forest-floor plant placement is bitwise deterministic for one seed");
        Check(sample.signedShoreDistance >= (plant.wetBank ? 8.0f : 27.0f)
                  && sample.signedShoreDistance <= 238.0f,
              "forest-floor plants remain inside their coastal-to-interior belt");
        Check(sample.height >= (plant.wetBank ? .7f : 2.5f)
                  && sample.slopeRadians <= 0.5585054f,
              "forest-floor plants remain on suitable ground");
        Check(!island::VegetationLayout::IsReservedRoute(plant.x, plant.z),
              "forest-floor plants keep paths, clearings, and player spawn open");
        Check(island::IslandEnvironmentField::SampleInland(
                  plant.x,plant.z,sample.signedShoreDistance).drainage<=0.84f,
              "forest-floor plants stay outside flowing stream water");
        if (plant.wetBank) {
            const auto field = island::IslandEnvironmentField::SampleInland(
                plant.x, plant.z, sample.signedShoreDistance);
            Check(field.moisture >= .38f && field.drainage <= .72f,
                "authored stream-bank plants remain damp without entering water");
            if (plant.z < island::StreamRoute::CentreZ(plant.x)) ++wetBankNegativeSideCount;
            else ++wetBankPositiveSideCount;
            ++wetBankPlantCount;
        }
        if (plant.seasonalWetland) {
            const float potential=island::IslandEnvironmentField::SeasonalWetlandPotential(
                plant.x,plant.z,sample.signedShoreDistance);
            const auto field=island::IslandEnvironmentField::SampleInland(
                plant.x,plant.z,sample.signedShoreDistance);
            Check(!plant.wetBank&&potential>=.18f&&field.moisture>=.42f
                    &&field.drainage<=.72f,
                "seasonal-wetland plants remain in the damp non-flowing basin");
            ++seasonalWetlandPlantCount;
        }
        Check(plant.variant < island::JungleLayout::GroundPlantVariantCount,
              "forest-floor plant variant index remains in range");
        Check(plant.scale >= island::JungleLayout::GroundPlantMinimumScale
                  && plant.scale <= island::JungleLayout::GroundPlantMaximumScale,
              "forest-floor plant scale remains in the authored range");
    }
    Check(wetBankPlantCount == jungle.StreamBankGroundPlantCount(),
        "stream-bank plant inventory matches marked ground plants");
    Check(seasonalWetlandPlantCount==jungle.SeasonalWetlandGroundPlantCount()
            &&seasonalWetlandPlantCount>=30
            &&seasonalWetlandPlantCount<=static_cast<std::size_t>(
                island::JungleLayout::SeasonalWetlandPlantTargetCount),
        "seasonal depression retains a bounded fern-rich local population");
    Check(wetBankNegativeSideCount >= 5 && wetBankPositiveSideCount >= 5,
        "stream-bank colonies occupy both sides without a single-sided strip");

    const island::NaturalPropLayout repeatedProps(terrain, vegetation, jungle);
    Check(props.Rocks().size() >= 30U && props.Rocks().size() <= 500U,
          "rock population stays inside its first performance budget");
    Check(props.FallenLogs().size() >= 6U && props.FallenLogs().size() <= 80U,
          "fallen-log population stays inside its first composition budget");
    Check(props.Driftwood().size() >= 12U && props.Driftwood().size() <= 100U,
          "driftwood population stays inside its sparse beach-composition budget");
    Check(props.BeachDetails().size() >= 120U && props.BeachDetails().size() <= 600U,
          "shell/pebble/wrack clusters stay inside their near-field beach-detail budget");
    Check(props.FallenFronds().size() >= 30U && props.FallenFronds().size() <= 180U,
          "fallen palm fronds stay inside their near-field composition budget");
    Check(props.ReefHabitat().size() >= 180U && props.ReefHabitat().size() <= 380U,
          "spatial reef habitat stays inside its bounded instance budget");
    Check(props.Rocks().size() == repeatedProps.Rocks().size()
              && props.FallenLogs().size() == repeatedProps.FallenLogs().size()
              && props.Driftwood().size() == repeatedProps.Driftwood().size()
              && props.BeachDetails().size() == repeatedProps.BeachDetails().size()
              && props.FallenFronds().size() == repeatedProps.FallenFronds().size()
              && props.ReefHabitat().size() == repeatedProps.ReefHabitat().size(),
          "natural-prop populations are deterministic");
    std::array<std::size_t, island::NaturalPropLayout::ReefHabitatVariantCount>
        reefHabitatVariants{};
    for (std::size_t index = 0; index < props.ReefHabitat().size(); ++index) {
        const auto& habitat = props.ReefHabitat()[index];
        const auto& repeated = repeatedProps.ReefHabitat()[index];
        const auto support = terrain.Sample(habitat.x, habitat.z);
        Check(habitat.x == repeated.x && habitat.y == repeated.y
                  && habitat.z == repeated.z && habitat.yawRadians == repeated.yawRadians
                  && habitat.scale == repeated.scale && habitat.variant == repeated.variant,
              "reef habitat placement is bitwise deterministic for one seed");
        CheckNear(habitat.y, support.height + .025f, .0001f,
                  "reef habitat roots remain fitted to the seabed");
        Check(island::IslandEnvironmentField::ReefSuitability(
                  habitat.x, habitat.z, support.signedShoreDistance, support.height) >= .12f,
              "reef habitat remains inside the shared shallow-water suitability field");
        Check(habitat.scale >= island::NaturalPropLayout::ReefHabitatMinimumScale
                  && habitat.scale <= island::NaturalPropLayout::ReefHabitatMaximumScale,
              "reef habitat scale remains inside the portable geometry budget");
        Check(habitat.variant < island::NaturalPropLayout::ReefHabitatVariantCount,
              "reef habitat variant index remains in range");
        ++reefHabitatVariants.at(habitat.variant);
    }
    Check(std::all_of(reefHabitatVariants.begin(), reefHabitatVariants.end(),
                      [](const std::size_t count) { return count >= 30U; }),
          "branching, rounded, plate coral and underwater-rock forms all populate the reef");
    std::size_t solidRockCount = 0U;
    std::size_t reefRockCount = 0U;
    std::size_t clearingLandmarkRockCount = 0U;
    std::size_t rockyShoreRockCount = 0U;
    std::size_t tidePoolRockCount = 0U;
    std::size_t streamBankRockCount = 0U;
    std::size_t cliffTalusRockCount = 0U;
    for (std::size_t index = 0; index < props.Rocks().size(); ++index) {
        const island::RockPlacement& rock = props.Rocks()[index];
        const island::RockPlacement& repeated = repeatedProps.Rocks()[index];
        const island::TerrainSample sample = terrain.Sample(rock.x, rock.z);
        Check(rock.x == repeated.x && rock.y == repeated.y && rock.z == repeated.z
                  && rock.yawRadians == repeated.yawRadians
                  && rock.scaleX == repeated.scaleX && rock.scaleY == repeated.scaleY
                  && rock.scaleZ == repeated.scaleZ && rock.variant == repeated.variant
                  && rock.solid == repeated.solid && rock.offshore == repeated.offshore
                  && rock.clearingLandmark == repeated.clearingLandmark
                  && rock.tidePoolLandmark == repeated.tidePoolLandmark
                  && rock.streamBankLandmark == repeated.streamBankLandmark
                  && rock.cliffTalus == repeated.cliffTalus,
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
            if (rock.tidePoolLandmark) {
                ++tidePoolRockCount;
                Check(!rock.solid && !rock.clearingLandmark
                        && sample.coastalRockiness>.50f,
                    "tide-pool accent rocks remain non-solid on rocky support");
            } else if (rock.streamBankLandmark) {
                ++streamBankRockCount;
                const float across=std::abs(rock.z-island::StreamRoute::CentreZ(rock.x));
                Check(!rock.solid&&!rock.clearingLandmark
                        && across>island::StreamRoute::HalfWidth(rock.x)+.45f
                        && across<island::StreamRoute::HalfWidth(rock.x)+2.4f,
                    "stream stones remain non-solid and just beyond the carved bank");
                Check(island::IslandEnvironmentField::SampleInland(
                        rock.x,rock.z,sample.signedShoreDistance).moisture>.55f,
                    "stream stones remain inside the damp bank microhabitat");
            } else if (rock.clearingLandmark) {
                Check(sample.signedShoreDistance >= 30.0f,
                      "authored clearing rocks remain on deep interior ground");
            } else if (rock.cliffTalus) {
                ++cliffTalusRockCount;
                Check(rock.solid && sample.coastalRockiness > .90f
                          && sample.signedShoreDistance >= 1.4f
                          && sample.signedShoreDistance <= 2.8f,
                      "authored talus blocks remain solid at the foot of the rocky headland");
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
                if (island::IslandEnvironmentField::SampleCoastalExperience(
                        rock.x,rock.z,sample.signedShoreDistance).rockyShore>.50f) {
                    ++rockyShoreRockCount;
                }
            }
            if (rock.cliffTalus) {
                Check(rock.scaleX >= 1.5f && rock.scaleX <= 3.4f
                          && rock.scaleY >= 1.1f && rock.scaleY <= 2.3f
                          && rock.scaleZ >= 1.8f && rock.scaleZ <= 3.0f,
                      "cliff talus retains varied large-block proportions");
            } else {
                Check(rock.scaleX > 0.35f && rock.scaleX < 2.30f
                          && rock.scaleY > 0.24f && rock.scaleY < 1.70f
                          && rock.scaleZ > 0.35f && rock.scaleZ < 2.35f,
                      "land-rock axis scales remain inside conservative visual bounds");
            }
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
    Check(rockyShoreRockCount>=8U,
        "exposed rocky-shore experience retains a visible procedural rock composition");
    Check(tidePoolRockCount==island::TidePoolLayout::Definitions.size()*2U,
        "each tide pool retains two lightweight rim-rock accents");
    Check(streamBankRockCount==props.StreamBankRockCount()&&streamBankRockCount>=10U
            &&streamBankRockCount<=island::NaturalPropLayout::StreamBankRockCandidateCount,
        "stream retains a bounded deterministic non-solid bank-stone composition");
    Check(cliffTalusRockCount
              == static_cast<std::size_t>(island::NaturalPropLayout::AuthoredCliffTalusCount),
          "the eastern cliff retains its complete authored talus group");
    std::size_t clearingLandmarkLogCount = 0U;
    std::size_t wetBankLogCount = 0U;
    bool eastClearingLogObserved = false;
    bool westClearingLogObserved = false;
    for (std::size_t index = 0; index < props.FallenLogs().size(); ++index) {
        const island::FallenLogPlacement& log = props.FallenLogs()[index];
        const island::FallenLogPlacement& repeated = repeatedProps.FallenLogs()[index];
        Check(log.x == repeated.x && log.y == repeated.y && log.z == repeated.z
                  && log.yawRadians == repeated.yawRadians && log.length == repeated.length
                  && log.radius == repeated.radius && log.variant == repeated.variant
                  && log.clearingLandmark == repeated.clearingLandmark
                  && log.wetBank == repeated.wetBank,
              "fallen-log placement is bitwise deterministic for one seed");
        Check(log.length >= island::NaturalPropLayout::LogMinimumLength
                  && log.length <= island::NaturalPropLayout::LogMaximumLength,
              "fallen-log length remains in the authored range");
        Check(log.radius >= 0.24f && log.radius <= 0.43f,
              "fallen-log radius remains in the authored range");
        Check(log.variant < island::NaturalPropLayout::LogVariantCount,
              "fallen-log variant index remains in range");
        if (log.wetBank) {
            const auto sample = terrain.Sample(log.x, log.z);
            const auto field = island::IslandEnvironmentField::SampleInland(
                log.x, log.z, sample.signedShoreDistance);
            Check(!log.clearingLandmark && field.moisture >= .38f
                      && field.drainage <= .72f,
                "damp-bank logs stay beside rather than across the stream");
            ++wetBankLogCount;
        }
        const float directionX = std::cos(log.yawRadians);
        const float directionZ = std::sin(log.yawRadians);
        for (int sampleIndex = 0; sampleIndex < 7; ++sampleIndex) {
            const float t = static_cast<float>(sampleIndex) / 6.0f - 0.5f;
            const float sampleX = log.x + directionX * log.length * t;
            const float sampleZ = log.z + directionZ * log.length * t;
            if (log.clearingLandmark) {
                Check(island::VegetationLayout::IsInsideAuthoredClearing(sampleX, sampleZ),
                      "the complete landmark-log footprint stays inside its clearing");
                const float eastDistance = std::hypot(
                    log.x - island::VegetationLayout::EastClearing.x,
                    log.z - island::VegetationLayout::EastClearing.z);
                const float westDistance = std::hypot(
                    log.x - island::VegetationLayout::WestClearing.x,
                    log.z - island::VegetationLayout::WestClearing.z);
                const bool usesEastClearing = eastDistance < westDistance;
                const island::ClearingDefinition& clearing = usesEastClearing
                    ? island::VegetationLayout::EastClearing
                    : island::VegetationLayout::WestClearing;
                Check(std::hypot(sampleX - clearing.x, sampleZ - clearing.z)
                          > clearing.radius * 0.56f,
                      "the landmark log preserves the clearing's central walking room");
                eastClearingLogObserved = eastClearingLogObserved || usesEastClearing;
                westClearingLogObserved = westClearingLogObserved || !usesEastClearing;
            } else {
                Check(!island::VegetationLayout::IsReservedRoute(sampleX, sampleZ),
                      "procedural fallen logs stay outside authored routes");
            }
            Check(terrain.Sample(sampleX, sampleZ).signedShoreDistance
                      >= (log.wetBank ? 8.0f : 30.0f),
                  "the complete fallen-log footprint stays on suitable island ground");
        }
        if (log.clearingLandmark) ++clearingLandmarkLogCount;
    }
    Check(wetBankLogCount == props.StreamBankLogCount() && wetBankLogCount >= 2
        && wetBankLogCount <= island::NaturalPropLayout::StreamBankLogCandidateCount,
        "stream has a bounded deterministic fallen-wood composition");
    Check(clearingLandmarkLogCount
              == static_cast<std::size_t>(
                  island::NaturalPropLayout::AuthoredClearingLogCount),
          "both clearing fallen-log landmarks remain present");
    Check(eastClearingLogObserved && westClearingLogObserved,
          "each authored clearing retains one fallen-log landmark");
    std::array<bool, island::NaturalPropLayout::DriftwoodVariantCount>
        observedDriftwoodVariants{};
    std::size_t coveLandmarkDriftwoodCount = 0U;
    std::size_t shelteredDriftwoodCount=0U,exposedDriftwoodCount=0U;
    std::array<bool, island::NaturalPropLayout::DriftwoodVariantCount>
        coveLandmarkVariants{};
    for (std::size_t index = 0; index < props.Driftwood().size(); ++index) {
        const island::DriftwoodPlacement& wood = props.Driftwood()[index];
        const island::DriftwoodPlacement& repeated = repeatedProps.Driftwood()[index];
        Check(wood.x == repeated.x && wood.y == repeated.y && wood.z == repeated.z
                  && wood.yawRadians == repeated.yawRadians && wood.length == repeated.length
                  && wood.radius == repeated.radius && wood.variant == repeated.variant
                  && wood.coveLandmark == repeated.coveLandmark,
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
        const auto woodExperience=island::IslandEnvironmentField::SampleCoastalExperience(
            wood.x,wood.z,terrain.SignedShoreDistance(wood.x,wood.z));
        if (std::max(woodExperience.shelteredBay,woodExperience.mangroveMouth)>.35f)
            ++shelteredDriftwoodCount;
        if (woodExperience.rockyShore>.35f) ++exposedDriftwoodCount;
        if (wood.coveLandmark) {
            ++coveLandmarkDriftwoodCount;
            const float angle = std::atan2(wood.z, wood.x);
            Check(std::abs(std::remainder(
                      angle - island::NaturalPropLayout::AuthoredCoveAngle,
                      2.0f * std::numbers::pi_v<float>)) < 0.05f,
                  "authored driftwood remains inside its sheltered cove arc");
            Check(terrain.SignedShoreDistance(wood.x, wood.z) >= 6.8f
                      && terrain.SignedShoreDistance(wood.x, wood.z) <= 10.7f,
                  "authored cove driftwood centres remain on dry beach");
            const auto centre = terrain.Sample(wood.x, wood.z);
            const auto coast = island::IslandEnvironmentField::SampleCoast(
                wood.x, wood.z, centre.signedShoreDistance);
            Check(coast.beachSuitability >= 0.72f,
                  "authored cove driftwood remains on visibly sandy coast");
            if (wood.variant < coveLandmarkVariants.size()) {
                coveLandmarkVariants[wood.variant] = true;
            }
            for (const island::PalmPlacement& palm : vegetation.Palms()) {
                Check(std::hypot(wood.x - palm.x, wood.z - palm.z) >= 2.0f,
                      "authored cove driftwood remains clear of palm trunks");
            }
            for (const island::CanopyPlacement& tree : jungle.Canopy()) {
                Check(std::hypot(wood.x - tree.x, wood.z - tree.z) >= 2.0f,
                      "authored cove driftwood remains clear of canopy trunks");
            }
            for (const island::DriftwoodPlacement& other : props.Driftwood()) {
                if (&other == &wood) continue;
                Check(std::hypot(wood.x - other.x, wood.z - other.z) >= 2.4f,
                      "authored cove driftwood remains visually separated");
            }
        }
        const float directionX = std::cos(wood.yawRadians);
        const float directionZ = std::sin(wood.yawRadians);
        float minimumSupportHeight = terrain.Height(wood.x, wood.z);
        float maximumSupportHeight = minimumSupportHeight;
        float maximumSupportSlope = terrain.Sample(wood.x, wood.z).slopeRadians;
        for (int sampleIndex = 0; sampleIndex < 5; ++sampleIndex) {
            const float t = static_cast<float>(sampleIndex) / 4.0f - 0.5f;
            const float sampleX = wood.x + directionX * wood.length * t;
            const float sampleZ = wood.z + directionZ * wood.length * t;
            const island::TerrainSample support = terrain.Sample(sampleX, sampleZ);
            minimumSupportHeight = std::min(minimumSupportHeight, support.height);
            maximumSupportHeight = std::max(maximumSupportHeight, support.height);
            maximumSupportSlope = std::max(maximumSupportSlope, support.slopeRadians);
            Check(support.signedShoreDistance >= 1.5f
                      && support.signedShoreDistance <= 27.0f,
                  "the complete driftwood footprint stays in the dry beach belt");
            Check(!island::VegetationLayout::IsReservedRoute(sampleX, sampleZ),
                  "the complete driftwood footprint stays outside authored routes");
        }
        if (wood.coveLandmark) {
            Check(maximumSupportSlope <= 0.17f,
                  "authored cove driftwood lies on a level beach footprint");
            Check(maximumSupportHeight - minimumSupportHeight <= 0.22f,
                  "authored cove driftwood has no buried or hovering end");
        }
    }
    for (const bool observed : observedDriftwoodVariants) {
        Check(observed, "every authored driftwood silhouette remains represented");
    }
    Check(coveLandmarkDriftwoodCount
              == static_cast<std::size_t>(
                  island::NaturalPropLayout::AuthoredCoveDriftwoodCount),
          "the authored cove driftwood landmark remains complete");
    Check(std::ranges::all_of(coveLandmarkVariants,
                              [](const bool value) { return value; }),
          "the authored cove landmark uses all three driftwood silhouettes");
    std::array<bool, island::NaturalPropLayout::BeachDetailVariantCount>
        observedBeachDetailVariants{};
    std::size_t broadBeachShellDetailCount=0U,broadBeachWrackDetailCount=0U;
    std::size_t shelteredBayWrackDetailCount=0U,mangroveMouthWrackDetailCount=0U;
    std::size_t tidePoolShellDetailCount=0U;
    std::size_t shelteredBeachDetailCount=0U,exposedBeachDetailCount=0U;
    std::size_t turtleTrackMarkCount=0U,turtleNestCount=0U;
    std::vector<std::array<float,2>> turtleNestPositions;
    for (std::size_t index = 0; index < props.BeachDetails().size(); ++index) {
        const island::BeachDetailPlacement& detail = props.BeachDetails()[index];
        const island::BeachDetailPlacement& repeated = repeatedProps.BeachDetails()[index];
        const island::TerrainSample sample = terrain.Sample(detail.x, detail.z);
        Check(detail.x == repeated.x && detail.y == repeated.y
                  && detail.z == repeated.z
                  && detail.yawRadians == repeated.yawRadians
                  && detail.scale == repeated.scale && detail.variant == repeated.variant
                  && detail.tidePoolLandmark == repeated.tidePoolLandmark
                  && detail.turtleTrace == repeated.turtleTrace
                  && detail.turtleNest == repeated.turtleNest,
              "shell/pebble/wrack placement is bitwise deterministic for one seed");
        if (detail.tidePoolLandmark) {
            ++tidePoolShellDetailCount;
            Check(sample.signedShoreDistance>=20.0f
                    && sample.signedShoreDistance<=40.0f
                    && sample.coastalRockiness>.50f && detail.variant<3U,
                "tide-pool shell accents remain on rocky rims and never use wrack");
        } else if (detail.turtleTrace) {
            Check(sample.signedShoreDistance >= 3.0f
                      && sample.signedShoreDistance <= 18.0f
                      && sample.height >= 0.22f
                      && sample.slopeRadians <= 0.20943951f
                      && detail.x < -100.0f,
                  "turtle nesting traces stay on one dry low-slope western beach");
            Check((detail.turtleNest && detail.variant == 5U)
                      || (!detail.turtleNest && detail.variant == 4U),
                  "turtle trace kind selects its dedicated track or nest silhouette");
            if (detail.turtleNest) {
                ++turtleNestCount;
                turtleNestPositions.push_back({detail.x,detail.z});
            } else {
                ++turtleTrackMarkCount;
            }
        } else {
            Check(sample.signedShoreDistance >= 3.0f
                      && sample.signedShoreDistance <= 18.0f,
                  "shell/pebble/wrack clusters remain in the authored dry-sand belt");
            Check(sample.height >= 0.22f && sample.slopeRadians <= 0.2617994f,
                  "shell/pebble/wrack clusters stay on dry low-slope support");
        }
        CheckNear(detail.y, sample.height + (detail.turtleTrace ? 0.008f : 0.012f),
                  0.0001f,
                  "shell/pebble/wrack clusters rest just above authoritative terrain");
        Check(!island::VegetationLayout::IsReservedRoute(detail.x, detail.z),
              "shell/pebble/wrack clusters keep authored routes and spawn clear");
        Check(detail.variant < observedBeachDetailVariants.size(),
              "shell/pebble/wrack cluster variant index remains in range");
        if (detail.variant < observedBeachDetailVariants.size()) {
            observedBeachDetailVariants[detail.variant] = true;
        }
        const auto experience=island::IslandEnvironmentField::SampleCoastalExperience(
            detail.x,detail.z,sample.signedShoreDistance);
        if (!detail.turtleTrace && experience.broadBeach>.50f) {
            if (detail.variant==3U) ++broadBeachWrackDetailCount;
            else ++broadBeachShellDetailCount;
        }
        if (!detail.turtleTrace && experience.shelteredBay>.50f && detail.variant==3U)
            ++shelteredBayWrackDetailCount;
        if (!detail.turtleTrace && experience.mangroveMouth>.35f && detail.variant==3U)
            ++mangroveMouthWrackDetailCount;
        if (!detail.tidePoolLandmark && !detail.turtleTrace
                && std::max(experience.shelteredBay,experience.mangroveMouth)>.35f)
            ++shelteredBeachDetailCount;
        if (!detail.tidePoolLandmark && !detail.turtleTrace && experience.rockyShore>.35f)
            ++exposedBeachDetailCount;
        Check(detail.scale >= island::NaturalPropLayout::BeachDetailMinimumScale
                  && detail.scale <= island::NaturalPropLayout::BeachDetailMaximumScale,
              "shell/pebble/wrack cluster scale remains in the authored range");
    }
    Check(std::ranges::all_of(observedBeachDetailVariants,
                              [](const bool value) { return value; }),
          "beach detail population exercises shell, wrack, track and nest variants");
    Check(turtleTrackMarkCount == static_cast<std::size_t>(
              island::NaturalPropLayout::TurtleNestingTrackCount
                  * island::NaturalPropLayout::TurtleTrackSegmentsPerNest)
              && turtleNestCount
                  == static_cast<std::size_t>(island::NaturalPropLayout::TurtleNestingTrackCount),
          "one western nesting beach has two complete turtle approaches and nests");
    Check(turtleNestPositions.size()==2U
              && std::hypot(turtleNestPositions[0][0]-turtleNestPositions[1][0],
                            turtleNestPositions[0][1]-turtleNestPositions[1][1])>=18.0f,
          "the two subtle turtle nests remain separately readable on the same beach");
    std::cout << "Coastal identity props rocky/south-shell/south-wrack/bay-wrack/mouth-wrack: "
        << rockyShoreRockCount << '/' << broadBeachShellDetailCount << '/'
        << broadBeachWrackDetailCount << '/' << shelteredBayWrackDetailCount << '/'
        << mangroveMouthWrackDetailCount << '\n';
    Check(broadBeachShellDetailCount>=12U
            && broadBeachShellDetailCount>broadBeachWrackDetailCount*3U,
        "broad Robinson beach is visually biased toward shell and pebble detail");
    Check(shelteredBayWrackDetailCount>=3U && mangroveMouthWrackDetailCount>=1U,
        "sheltered bay and localized mangrove mouth retain distinct wrack detail");
    Check(tidePoolShellDetailCount==island::TidePoolLayout::Definitions.size(),
        "each tide pool retains one lightweight shell/pebble rim accent");
    std::array<bool, island::NaturalPropLayout::FallenFrondVariantCount>
        observedFallenFrondVariants{};
    std::size_t shelteredCoastalFrondCount=0U,exposedCoastalFrondCount=0U;
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
        const auto frondSample=terrain.Sample(frond.x,frond.z);
        const auto frondExperience=island::IslandEnvironmentField::SampleCoastalExperience(
            frond.x,frond.z,frondSample.signedShoreDistance);
        if (frondSample.signedShoreDistance<68.0f
                && std::max(frondExperience.shelteredBay,frondExperience.mangroveMouth)>.35f)
            ++shelteredCoastalFrondCount;
        if (frondSample.signedShoreDistance<68.0f && frondExperience.rockyShore>.35f)
            ++exposedCoastalFrondCount;
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
    std::cout << "Wind-sorted coastal debris sheltered/exposed wood,detail,fronds: "
        << shelteredDriftwoodCount << '/' << exposedDriftwoodCount << ", "
        << shelteredBeachDetailCount << '/' << exposedBeachDetailCount << ", "
        << shelteredCoastalFrondCount << '/' << exposedCoastalFrondCount << '\n';
    Check(shelteredDriftwoodCount>=island::NaturalPropLayout::AuthoredCoveDriftwoodCount
            && shelteredDriftwoodCount>=exposedDriftwoodCount*2U,
        "driftwood is more frequent in the leeward bay than on exposed rocky headlands");
    Check(shelteredBeachDetailCount*3U>=exposedBeachDetailCount*5U,
        "shell, pebble and wrack detail accumulates more strongly in sheltered shore sectors");
    Check(shelteredCoastalFrondCount>exposedCoastalFrondCount,
        "coastal fallen fronds accumulate more strongly in shelter than on rocky headlands");
    Check(props.Obstacles().size() == solidRockCount + props.FallenLogs().size() * 7U,
          "natural-prop collision excludes small decorative props and matches solid spans");

    for (int z = -360; z <= 360; z += 12) {
        for (int x = -360; x <= 360; x += 12) {
            const float first = terrain.Height(static_cast<float>(x), static_cast<float>(z));
            const float second = terrain.Height(static_cast<float>(x), static_cast<float>(z));
            Check(std::isfinite(first), "terrain height is finite across the world grid");
            Check(first == second, "terrain generation is deterministic");
            Check(first >= -22.01f && first <= 45.0f, "terrain height stays in the authored scale");
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
              && firstMovementTrace.obstacleDepenetrations == 0
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

    island::PlayerMotion cardinalAccelerationPlayer;
    island::PlayerMotion diagonalAccelerationPlayer;
    cardinalAccelerationPlayer.SetPosition(terrain, 0.0f, 0.0f);
    diagonalAccelerationPlayer.SetPosition(terrain, 0.0f, 0.0f);
    cardinalAccelerationPlayer.Update(terrain, 0.0f, 1.0f, 1.0f / 60.0f);
    diagonalAccelerationPlayer.Update(terrain, 1.0f, 1.0f, 1.0f / 60.0f);
    const float cardinalAccelerationSpeed = std::hypot(
        cardinalAccelerationPlayer.VelocityX(), cardinalAccelerationPlayer.VelocityZ());
    const float diagonalAccelerationSpeed = std::hypot(
        diagonalAccelerationPlayer.VelocityX(), diagonalAccelerationPlayer.VelocityZ());
    CheckNear(diagonalAccelerationSpeed, cardinalAccelerationSpeed, 0.0001f,
              "walking acceleration is independent of movement direction");
    cardinalAccelerationPlayer.Update(terrain, 0.0f, 0.0f, 1.0f / 60.0f);
    diagonalAccelerationPlayer.Update(terrain, 0.0f, 0.0f, 1.0f / 60.0f);
    CheckNear(std::hypot(diagonalAccelerationPlayer.VelocityX(),
                         diagonalAccelerationPlayer.VelocityZ()),
              std::hypot(cardinalAccelerationPlayer.VelocityX(),
                         cardinalAccelerationPlayer.VelocityZ()),
              0.0001f,
              "walking deceleration is independent of movement direction");

    island::PlayerMotion runningPlayer;
    // Speed selection needs an unobstructed gentle slope; the island centre is now a saddle
    // approach whose bank correctly rejects some northward movement. Steep-bank behaviour
    // has its own independent test below.
    for (int z = -170; z <= -140; ++z) {
        const auto runningSurface = terrain.Sample(0.0f, static_cast<float>(z));
        Check(runningSurface.height > 0.5f && runningSurface.slopeRadians < 0.25f,
              "run-speed fixture remains dry and gently sloped");
    }
    runningPlayer.SetPosition(terrain, 0.0f, -170.0f);
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
    for (const float authoredAngle : shorelineTransects) {
        // Rocky headlands now intentionally block climbing. Test their adjacent sandy
        // access here, and the actual cliff barrier independently below.
        const float angle = island::IslandEnvironmentField::CoastalRockPotential(authoredAngle) > 0.55f
            ? authoredAngle + 0.65f : authoredAngle;
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

    island::PlayerMotion cliffPlayer;
    constexpr float cliffAngle = 0.35f;
    const float cliffRadius = terrain.ShoreRadius(cliffAngle);
    cliffPlayer.SetPosition(terrain, std::cos(cliffAngle) * (cliffRadius - 0.5f),
                           std::sin(cliffAngle) * (cliffRadius - 0.5f));
    for (int frame = 0; frame < 600; ++frame)
        cliffPlayer.Update(terrain, -std::cos(cliffAngle), -std::sin(cliffAngle), 1.0f / 60.0f);
    const float cliffReached = terrain.SignedShoreDistance(cliffPlayer.Position().x, cliffPlayer.Position().z);
    Check(cliffReached > 1.0f && cliffReached < 7.0f,
          "player can approach rocky headland but cannot climb through its steep face");
    Check(cliffPlayer.Mode() == island::LocomotionMode::Walking,
          "cliff rejection leaves player on stable dry footing");

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
    const island::OceanCurrentDrift fullCurrent = player.OceanCurrentVelocity(terrain);
    Check(fullCurrent.x < -2.05f && fullCurrent.z > 0.35f
              && std::hypot(fullCurrent.x, fullCurrent.z) < 2.25f,
          "full current remains strongly shoreward with restrained coherent circulation");
    bool currentFieldRemainsSafe = true;
    for (int step = 0; step < 16; ++step) {
        const float angle = 2.0f * std::numbers::pi_v<float>
            * static_cast<float>(step) / 16.0f;
        const float outwardX = std::cos(angle);
        const float outwardZ = std::sin(angle);
        const float currentRadius = terrain.ShoreRadius(angle)
            + island::PlayerMotion::FullCurrentDistance;
        player.SetPosition(terrain, outwardX * currentRadius, outwardZ * currentRadius);
        const island::OceanCurrentDrift current = player.OceanCurrentVelocity(terrain);
        const float inwardSpeed = -(current.x * outwardX + current.z * outwardZ);
        const float circulationSpeed = -current.x * outwardZ + current.z * outwardX;
        currentFieldRemainsSafe = currentFieldRemainsSafe
            && std::isfinite(current.x) && std::isfinite(current.z)
            && inwardSpeed > 2.09f && inwardSpeed < 2.11f
            && circulationSpeed > 0.25f && circulationSpeed < 0.61f
            && inwardSpeed > circulationSpeed;
    }
    Check(currentFieldRemainsSafe,
          "full current stays finite, shoreward-dominant, and coherently circulating "
          "around the island");
    player.SetPosition(terrain, eastShore + island::PlayerMotion::SoftCurrentStart + 0.25f,
                       0.0f);
    const island::OceanCurrentDrift thresholdCurrent =
        player.OceanCurrentVelocity(terrain);
    Check(std::hypot(thresholdCurrent.x, thresholdCurrent.z) < 0.002f,
          "current velocity eases continuously from zero at the soft boundary");

    player.SetPosition(terrain, eastShore + 80.0f, 0.0f);
    const float offshoreBeforeCurrent = player.OffshoreDistance(terrain);
    for (int step = 0; step < 120; ++step) {
        player.Update(terrain, 0.0f, 0.0f, 1.0f / 60.0f);
    }
    Check(player.OffshoreDistance(terrain) < offshoreBeforeCurrent,
          "outer-ocean current gently carries an idle swimmer shoreward");
    Check(player.Position().z > 0.20f,
          "outer-ocean current gives an idle swimmer a mild coast-parallel drift");

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

    const island::CircularObstacle embeddedObstacle{0.0f, 0.0f, 0.42f};
    const std::span<const island::CircularObstacle> embeddedObstacles(
        &embeddedObstacle, 1U);
    island::PlayerMotion embeddedPlayer;
    embeddedPlayer.SetPosition(terrain, embeddedObstacle.x, embeddedObstacle.z);
    embeddedPlayer.Update(terrain, 0.0f, 0.0f, 1.0f / 60.0f, embeddedObstacles);
    const float embeddedClearance = island::PlayerMotion::CapsuleRadius
        + embeddedObstacle.radius;
    const float recoveredDistance = std::hypot(
        embeddedPlayer.Position().x - embeddedObstacle.x,
        embeddedPlayer.Position().z - embeddedObstacle.z);
    Check(std::isfinite(embeddedPlayer.Position().x)
              && std::isfinite(embeddedPlayer.Position().z)
              && recoveredDistance >= embeddedClearance,
          "placement inside a static obstacle recovers to finite capsule clearance");
    Check(embeddedPlayer.Position().x > embeddedObstacle.x
              && std::abs(embeddedPlayer.Position().z - embeddedObstacle.z) < 0.0001f,
          "exact-centre placement recovery uses its deterministic outward direction");
    Check(embeddedPlayer.MovementTrace().valid
              && embeddedPlayer.MovementTrace().obstacleDepenetrations == 1
              && embeddedPlayer.MovementTrace().obstacleRejections == 0,
          "movement diagnostics distinguish placement recovery from blocked movement");
    const island::PlayerPosition recoveredPosition = embeddedPlayer.Position();
    embeddedPlayer.Update(terrain, 0.0f, 0.0f, 1.0f / 60.0f, embeddedObstacles);
    CheckNear(embeddedPlayer.Position().x, recoveredPosition.x, 0.0001f,
              "static placement recovery does not drift on later idle updates along x");
    CheckNear(embeddedPlayer.Position().z, recoveredPosition.z, 0.0001f,
              "static placement recovery does not drift on later idle updates along z");

    const island::WorldCollisionGrid embeddedCollisionGrid(embeddedObstacles);
    island::PlayerMotion indexedEmbeddedPlayer;
    indexedEmbeddedPlayer.SetPosition(terrain, embeddedObstacle.x, embeddedObstacle.z);
    indexedEmbeddedPlayer.Update(
        terrain, 0.0f, 0.0f, 1.0f / 60.0f, embeddedCollisionGrid);
    CheckNear(indexedEmbeddedPlayer.Position().x, recoveredPosition.x, 0.0001f,
              "indexed and exhaustive placement recovery agree along x");
    CheckNear(indexedEmbeddedPlayer.Position().z, recoveredPosition.z, 0.0001f,
              "indexed and exhaustive placement recovery agree along z");
    Check(indexedEmbeddedPlayer.MovementTrace().obstacleDepenetrations == 1
              && !embeddedCollisionGrid.IntersectsCircle(
                  indexedEmbeddedPlayer.Position().x, indexedEmbeddedPlayer.Position().z,
                  island::PlayerMotion::CapsuleRadius),
          "indexed placement recovery ends outside the obstacle and reports its correction");

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
