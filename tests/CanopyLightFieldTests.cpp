#include "IslandSimulator/CanopyLightField.hpp"
#include "IslandSimulator/CanopyProfile.hpp"
#include "IslandSimulator/CanopyLodInspection.hpp"
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iostream>
#include <limits>
#include <numbers>
#include <stdexcept>
#include <string>

namespace
{
    void Require(const bool condition, const char* message)
    {
        if (!condition) throw std::runtime_error(message);
    }
    template<class F> void Invalid(F&& action)
    {
        try { action(); } catch (const std::invalid_argument&) { return; }
        throw std::runtime_error("Invalid canopy input was accepted");
    }
}

int main(int argc, char** argv)
try
{
    using namespace island;
    const CanopyLodInspection inspection{-87120, -15369, true};
    Require(inspection.Matches(-87.1201f, -15.3691f)
        && !inspection.Matches(-87.121f, -15.3691f)
        && !inspection.Matches(-87.1201f, -15.368f),
        "Capture inspection must match the rounded telemetry identity, not a nearby tree");
    const IslandTerrain terrain;
    for (const auto site : {std::array{120.0f, 80.0f}, std::array{-90.0f, 130.0f},
                            std::array{-40.0f, -180.0f}, std::array{0.0f, 0.0f}}) {
        const std::array minimum{site[0]-5, 10.0f, site[1]-5};
        const std::array maximum{site[0]+5, 30.0f, site[1]+5};
        const auto camera = MakeCanopyInspectionCamera(site[0], site[1], 117, minimum, maximum);
        Require(std::abs(std::hypot(camera.position[0]-site[0], camera.position[2]-site[1])-117) < 0.0001f,
            "Inspection camera changed the actual horizontal LOD distance");
        Require(camera.position[1] == 25 && camera.target == std::array{site[0], 25.0f, site[1]},
            "Inspection camera must aim at the measured crown at equal height");
        Require(std::hypot(camera.position[0],camera.position[2]) > std::hypot(site[0],site[1]),
            "Inspection camera must look inward from outside the selected tree");
    }
    for (float distance : {0.0f, -1.0f, 476.0f, std::numeric_limits<float>::infinity()})
        Invalid([&] { (void)MakeCanopyInspectionCamera(10, 20, distance, {0,0,0}, {1,1,1}); });
    Invalid([&] { (void)MakeCanopyInspectionCamera(10, 20, 110, {0,2,0}, {1,1,1}); });
    Invalid([&] { (void)MakeCanopyInspectionCamera(std::numeric_limits<float>::quiet_NaN(), 20,
        110, {0,0,0}, {1,1,1}); });
    const CanopyOccluder sphere{0, 10, 0, 3, 3, 3, 0};
    Require(std::abs(CanopyRayLength(sphere, 0, 0, 0, {0, 1, 0}) - 6) < 1e-5f,
            "Central ray must cross the full sphere diameter");
    Require(std::abs(CanopyRayLength(sphere, 0, 10, 0, {0, 1, 0}) - 3) < 1e-5f,
            "Inside ray must start at receiver, not back of canopy");
    Require(CanopyRayLength(sphere, 0, 20, 0, {0, 1, 0}) == 0,
            "Canopy behind receiver must not occlude the sun");
    Require(CanopyRayLength(sphere, 3, 0, 0, {0, 1, 0}) == 0,
            "A tangent ray must have zero optical thickness");
    Require(CanopyRayLength(sphere, 3.01f, 0, 0, {0, 1, 0}) == 0,
            "Ray outside silhouette must remain lit");
    const CanopyOccluder oval{0, 10, 0, 4, 2, 1, std::numbers::pi_v<float> * 0.5f};
    Require(CanopyRayLength(oval, 0, 0, 3, {0, 1, 0}) > 0,
            "Yaw must rotate the long ellipsoid axis into world Z");
    Require(CanopyRayLength(oval, 3, 0, 0, {0, 1, 0}) == 0,
            "Yaw must rotate the narrow ellipsoid axis into world X");
    Invalid([&] { (void)CanopyRayLength(sphere, 0, 0, 0, {0, 0, 0}); });
    Invalid([&] { (void)CanopyRayLength(sphere, 0, 0, 0, {0, -1, 0}); });
    Invalid([&] { (void)CanopyRayLength(sphere, 0, 0, 0,
        {std::numeric_limits<float>::quiet_NaN(), 1, 0}); });
    auto broken = sphere;
    broken.radiusX = 0;
    Invalid([&] { (void)CanopyRayLength(broken, 0, 0, 0, {0, 1, 0}); });
    Invalid([&] { (void)MakeCanopyCrownProfile(CanopyFormCount, 0); });
    Invalid([&] { (void)MakeCanopyCrownProfile(0, 8); });
    Invalid([&] { (void)MakeCanopyCrownProfile(0, -1); });
    Invalid([&] { (void)MakeCanopyTrunkProfile(-1); });
    Invalid([&] { (void)SampleCanopyTrunk(CanopyFormCount, 0.5f); });
    for(float invalid : {-0.01f,1.01f,std::numeric_limits<float>::infinity(),
                         std::numeric_limits<float>::quiet_NaN()}) {
        Invalid([&] { (void)SampleCanopyTrunk(0, invalid); });
    }
    std::array<float,CanopyFormCount> crownSpans{}, crownHeights{}, crownOffsets{};
    for(int variant=0;variant<CanopyFormCount;++variant) {
        const auto trunk=MakeCanopyTrunkProfile(variant);
        Require(SampleCanopyTrunk(variant,0)==std::array<float,3>{0,0,0},
                "Canopy root must stay on its collision axis");
        const auto tip=SampleCanopyTrunk(variant,1);
        Require(std::abs(tip[0]-std::cos(trunk.leanAngle)*trunk.lean)<1e-5f
            && std::abs(tip[2]-std::sin(trunk.leanAngle)*trunk.lean)<1e-5f,
            "Trunk tip must agree with the crown profile anchor");
        for(int step=0;step<=200;++step) {
            const float t=step/200.0f;
            const auto p=SampleCanopyTrunk(variant,t);
            Require(p==SampleCanopyTrunk(variant,t) && std::isfinite(p[0]) && std::isfinite(p[2])
                && std::abs(p[1]-trunk.height*t)<1e-5f && std::hypot(p[0],p[2])<1.8f,
                "Trunk centreline must be deterministic, finite, bounded and monotone in height");
            if(p[1]<=3.0f) Require(p[0]==0 && p[2]==0,
                "Lower three metres must not bend outside the collision-axis envelope");
        }
        float minX=100,maxX=-100,minY=100,maxY=-100,offset=0;
        for(int cluster=0;cluster<8;++cluster) {
            const auto p=MakeCanopyCrownProfile(variant,cluster);
            const auto q=MakeCanopyCrownProfile(variant,cluster);
            Require(p.x==q.x && p.y==q.y && p.z==q.z && p.width==q.width && p.height==q.height,
                    "Crown profiles must be deterministic");
            Require(std::isfinite(p.x) && std::isfinite(p.z) && p.width>0 && p.height>0
                && p.y-p.height>5 && p.y+p.height<17 && p.phase>=0 && p.phase<1,
                    "Crown profile escaped its positive bounded canopy region");
            minX=std::min(minX,p.x-p.width); maxX=std::max(maxX,p.x+p.width);
            minY=std::min(minY,p.y-p.height); maxY=std::max(maxY,p.y+p.height);
            offset+=p.x-tip[0];
            if(cluster) {
                const auto attachment=SampleCanopyTrunk(variant,0.65f+0.065f*(cluster/2));
                const float length=std::hypot(p.x-std::cos(p.angle)*0.48f-attachment[0],
                    p.y-0.62f-attachment[1],p.z-std::sin(p.angle)*0.48f-attachment[2]);
                Require(length>0.5f && length<8,
                        "Crown support branch must have a nondegenerate bounded span");
            }
        }
        crownSpans[variant]=maxX-minX; crownHeights[variant]=maxY-minY; crownOffsets[variant]=offset/8;
    }
    Require(crownSpans[0]>crownSpans[1]*1.3f && crownHeights[1]>crownHeights[0]*1.3f
        && crownOffsets[2]>1.0f && crownOffsets[0]<0.5f && crownHeights[3]>crownHeights[0]*1.2f,
        "Crown forms must differ structurally, not only by a uniform height/scale change");
    Require(crownSpans[4]>crownSpans[5]*1.7f && crownHeights[5]>crownHeights[4]*1.2f
        && crownHeights[7]>crownHeights[4]*1.25f,
        "New low, narrow and open forms must have distinct crown aspect ratios");
    Invalid([&] { CanopyLightField invalid(terrain, {}, 100); });
    Invalid([&] { CanopyLightField invalid(terrain, {}, 2048); });

    const CanopyLightField empty(terrain, {}, 32);
    const auto darkTint = CanopyTransmissionTint(0);
    const auto litTint = CanopyTransmissionTint(1);
    const auto invalidTint = CanopyTransmissionTint(std::numeric_limits<float>::quiet_NaN());
    Require(darkTint.red == CanopyAmbientTint.red && darkTint.green == CanopyAmbientTint.green
        && darkTint.blue == CanopyAmbientTint.blue && litTint.red == 1 && litTint.green == 1
        && litTint.blue == 1 && invalidTint.red == 1,
        "CPU receivers must retain the same ambient tint as GPU receivers");
    Require(CanopyTransmissionTint(-1).red == darkTint.red
        && CanopyTransmissionTint(2).blue == 1,
        "Out-of-range transmission must not create negative or amplified light");
    Require(std::ranges::all_of(empty.Texels(), [](auto x) { return x == 255; }),
            "Empty field must be fully sunlit");
    Require(empty.Sample(0, 0) == 1 && empty.Sample(301, 0) == 1
        && empty.Sample(std::numeric_limits<float>::infinity(), 0) == 1,
        "Empty/outside/non-finite samples must not wrap into a shadow");

    const std::array<CanopyPlacement, 1> rotatedTree = {{{10, 3, -8,
        std::numbers::pi_v<float> * 0.5f, 1.2f, 2}}};
    const auto transformed = MakeCanopyOccluders(rotatedTree);
    const auto local = MakeCanopyCrownProfile(2, 0);
    Require(transformed.size() == 8
        && std::abs(transformed[0].x - (10 + local.z * 1.2f)) < 1e-4f
        && std::abs(transformed[0].z - (-8 - local.x * 1.2f)) < 1e-4f,
        "Shadow lobes must use the renderer's scaled/yawed crown placement");
    auto invalidTree = rotatedTree;
    invalidTree[0].scale = 0;
    Invalid([&] { (void)MakeCanopyOccluders(invalidTree); });

    // Analytic oracle at every texel for one caster: exercises projected bounds, slopes,
    // sun direction, quantization, half-texel centres and row orientation together.
    auto single = sphere;
    single.x = 23; single.z = -17;
    single.y = terrain.Height(single.x, single.z) + 12;
    single.radiusX = 12; single.radiusY = 5; single.radiusZ = 9;
    single.yaw = 0.71f;
    constexpr TerrainNormal sun{-0.4f, 0.8f, 0.3f};
    const std::array casters{single};
    const CanopyLightField small(terrain, casters, 128, sun);
    std::size_t shadowed = 0;
    for (int row = 0; row < small.Resolution(); ++row) {
        for (int column = 0; column < small.Resolution(); ++column) {
            const float x = CanopyLightField::TexelCoordinate(column, small.Resolution());
            const float z = CanopyLightField::TexelCoordinate(row, small.Resolution());
            const auto expected = static_cast<unsigned>(std::lround(255.0f * std::exp(
                -single.density * CanopyRayLength(single, x, terrain.Height(x, z), z, sun))));
            const auto actual = small.Texels()[static_cast<std::size_t>(row) * small.Resolution() + column];
            Require(actual == expected, "Conservative projected bounds excluded a valid shadow ray");
            Require(std::abs(small.Sample(x, z) - actual / 255.0f) < 1e-5f,
                    "CPU bilinear mapping differs at the GPU texel centre");
            const float ground = terrain.Height(x, z);
            Require(std::abs(small.SampleReceiver(x, ground + 2, z, ground)
                - small.Sample(x + 1.0f, z - 0.75f)) < 1e-5f,
                "Elevated receivers must project along the same oblique sunlight ray");
            Require(std::abs(small.SampleReceiver(x, ground - 1, z, ground)
                - small.Sample(x, z)) < 1e-5f,
                "Below-ground vertices must not project behind their ground anchor");
            Require(std::abs(small.Sample(x + 0.001f, z) - small.Sample(x, z)) < 0.0003f,
                "Bilinear light must stay continuous when a receiver moves across texels");
            const float footprint = small.SampleReceiverFootprint(x, ground + 2, z, ground, 0.6f);
            const float expectedFootprint = small.Sample(x + 1, z - 0.75f) * 0.5f
                + (small.Sample(x + 0.4f, z - 0.75f) + small.Sample(x + 1.6f, z - 0.75f)
                   + small.Sample(x + 1, z - 1.35f) + small.Sample(x + 1, z - 0.15f)) * 0.125f;
            Require(std::abs(footprint - expectedFootprint) < 1e-5f && footprint >= 0 && footprint <= 1,
                "Body footprint must average projected samples with normalized positive weights");
            Require(std::abs(small.SampleReceiverFootprint(x + 0.001f, ground + 2, z, ground, 0.6f)
                - footprint) < 0.0003f,
                "Moving footprint must not introduce discontinuous whole-body lighting");
            shadowed += actual < 255;
        }
    }
    Require(shadowed > 10, "The oblique test canopy did not cast a measurable shadow");
    Require(small.SampleReceiver(0, std::numeric_limits<float>::infinity(), 0, 0) == 1,
            "Non-finite receiver height must not propagate invalid light");
    Require(small.SampleReceiverFootprint(0, 0, 0, 0, -1) == 1
        && small.SampleReceiverFootprint(0, 0, 0, 0, std::numeric_limits<float>::infinity()) == 1,
        "Invalid body footprints must not propagate invalid light");
    Require(std::abs(small.SampleReceiverFootprint(23, single.y - 10, -17, single.y - 12, 0)
        - small.SampleReceiver(23, single.y - 10, -17, single.y - 12)) < 1e-6f,
        "Zero-radius footprint must match the point receiver");

    const auto start = std::chrono::steady_clock::now();
    const JungleLayout layout(terrain);
    const auto worldCasters = MakeCanopyOccluders(layout.Canopy());
    const CanopyLightField world(terrain, worldCasters);
    const auto baked = std::chrono::steady_clock::now();
    const CanopyLightField repeated(terrain, worldCasters);
    Require(std::ranges::equal(world.Texels(), repeated.Texels()), "Canopy bake must be deterministic");
    const auto darkCount = std::ranges::count_if(world.Texels(), [](auto x) { return x < 240; });
    Require(darkCount > 10000 && darkCount < 300000,
            "World canopy shadow coverage is outside the plausible bounded range");
    if (argc == 3 && std::string(argv[1]) == "--preview") {
        std::ofstream output(argv[2], std::ios::binary);
        output << "P5\n" << world.Resolution() << ' ' << world.Resolution() << "\n255\n";
        output.write(reinterpret_cast<const char*>(world.Texels().data()),
                     static_cast<std::streamsize>(world.Texels().size()));
        if (!output) throw std::runtime_error("Could not write canopy light preview");
    } else if (argc != 1) {
        throw std::invalid_argument("Usage: island_canopy_tests [--preview output.pgm]");
    }
    std::cout << "CanopyLightFieldTests: " << worldCasters.size() << " crown lobes, "
              << world.Texels().size() << " bytes, " << darkCount << " shaded texels, bake "
              << std::chrono::duration<double, std::milli>(baked - start).count() << " ms; passed\n";
    return 0;
}
catch (const std::exception& error)
{
    std::cerr << "CanopyLightFieldTests: " << error.what() << '\n';
    return 1;
}
