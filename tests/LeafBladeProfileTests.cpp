#include "IslandSimulator/LeafBladeProfile.hpp"
#include "IslandSimulator/UnderstoryLeafProfile.hpp"
#include "IslandSimulator/JungleMaterial.hpp"
#include "IslandSimulator/BarkMaterial.hpp"
#include "IslandSimulator/CanopyLeafPosture.hpp"
#include "IslandSimulator/SurfaceNormals.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <limits>
#include <numbers>
#include <stdexcept>

void TestGroundBroadleaf();
void TestSplitGroundLeaf();
void TestBushProfile();

int main() try
{
    TestGroundBroadleaf();
    TestSplitGroundLeaf();
    TestBushProfile();
    unsigned checks = 0;
    {
        using namespace island;
        if (BroadleafColumns != std::array<float,5>{-1,-0.5f,0,0.5f,1})
            throw std::runtime_error("Broadleaf columns must sample both lamina wings, not only the midrib");
        auto paddle = UnderstoryLeaf(1,0,false);
        auto lance = paddle;
        lance.shape = UnderstoryBladeShape::Lance;
        float maximumPaddle = 0, maximumLance = 0, peakPaddle = 0, peakLance = 0;
        for (int i = 1; i < 100; ++i) {
            const float t = i/100.0f;
            const auto width = [t](const auto& leaf) {
                return SampleUnderstoryLeaf(leaf,t,1).across-SampleUnderstoryLeaf(leaf,t,-1).across;
            };
            const float p = width(paddle), l = width(lance);
            if (p > maximumPaddle) { maximumPaddle = p; peakPaddle = t; }
            if (l > maximumLance) { maximumLance = l; peakLance = t; }
        }
        if (peakPaddle >= peakLance-0.025f || maximumPaddle <= 0 || maximumLance <= 0)
            throw std::runtime_error("Paddle and lance forms must have distinct longitudinal width distributions");
        for (auto shape : {UnderstoryBladeShape::Paddle,UnderstoryBladeShape::Lance}) {
            auto leaf = paddle;
            leaf.shape = shape;
            for (float t : {0.0f,std::nextafter(0.0f,1.0f),std::nextafter(1.0f,0.0f),1.0f}) {
                for (float side : BroadleafColumns) {
                    const auto point = SampleUnderstoryLeaf(leaf,t,side);
                    if (!std::isfinite(point.across) || !std::isfinite(point.height))
                        throw std::runtime_error("Broadleaf endpoint remapping generated invalid geometry");
                }
            }
        }
        auto broken = paddle;
        broken.shape = static_cast<UnderstoryBladeShape>(42);
        try { (void)SampleUnderstoryLeaf(broken,0.5f,0); throw std::runtime_error("Invalid blade shape accepted"); }
        catch (const std::invalid_argument&) { }
        for (int variant : {0,2}) {
            for (int index = 0; index < UnderstoryLeafCount(variant,false); ++index) {
                const auto leaf = UnderstoryLeaf(variant,index,false);
                if (leaf.shape != UnderstoryBladeShape::Narrow)
                    throw std::runtime_error("Fern source inherited a broadleaf lamina shape");
                for (float t : {0.0f,0.2f,0.5f,0.8f,1.0f}) {
                    for (float side : {-1.0f,0.0f,1.0f}) {
                        float blend = std::clamp((t-leaf.petiole)/0.20f,0.0f,1.0f);
                        blend = blend*blend*(3-2*blend);
                        auto old = SampleLeafBlade(t,side,leaf.length,leaf.width*(0.10f+0.90f*blend),
                            leaf.lift,leaf.phase);
                        const float arch = t == 0 || t == 1 ? 0 : std::sin(t*std::numbers::pi_v<float>);
                        const float tip = std::clamp((t-0.58f)/0.42f,0.0f,1.0f);
                        old.across += leaf.bow*t*t;
                        old.height += leaf.edgeCurl*side*side*arch*blend-leaf.tipDrop*tip*tip;
                        const auto actual = SampleUnderstoryLeaf(leaf,t,side);
                        if (std::abs(old.across-actual.across) > 0.000001f
                            || std::abs(old.height-actual.height) > 0.000001f || old.along != actual.along)
                            throw std::runtime_error("Broadleaf refinement changed the fern source curve");
                    }
                }
            }
        }
    }
    {
        using namespace island;
        const auto require = [](bool value, const char* message) {
            if (!value) throw std::runtime_error(message);
        };
        constexpr std::array<std::uint16_t,12> triangles{0,1,2,0,2,3,1,4,2,2,4,3};
        int retained = 0;
        for (int pair = 0; pair < FernPairCount; ++pair) retained += RetainFernPair(pair,true);
        require(retained == 4,"Far fern must retain four original leaflet pairs");
        require(std::ranges::is_sorted(FernRachisKnots) && FernRachisKnots.front() == 0
            && FernRachisKnots.back() == 1,"Rachis knots must cover the complete rooted frond");
        for (int variant : {0,2}) {
            for (int leaf = 0; leaf < UnderstoryLeafCount(variant,false); ++leaf) {
                const auto frond = UnderstoryLeaf(variant,leaf,false);
                for (int pair = 0; pair < FernPairCount; ++pair) {
                    for (int sign : {-1,1}) {
                        const auto p = FernLeaflet(frond,pair,sign);
                        require(p == FernLeaflet(frond,pair,sign)
                            && p.t == FernRachisKnots[pair+1] && p.length > 0 && p.width > 0,
                            "Fern profile must be deterministic and rooted on an exact stem knot");
                        const auto root = SampleUnderstoryLeaf(frond,p.t,0);
                        const auto before = SampleUnderstoryLeaf(frond,p.t-0.001f,0);
                        const auto after = SampleUnderstoryLeaf(frond,p.t+0.001f,0);
                        const float dy = after.height-before.height, dz = after.along-before.along;
                        const float c = std::cos(frond.angle), s = std::sin(frond.angle);
                        const auto frame = MakePointedLeafFrame(p.angle,p.pitch,p.roll);
                        const float alignment = (-c*dy*frame.normal[0]+dz*frame.normal[1]
                            -s*dy*frame.normal[2])/std::hypot(dy,dz);
                        require(alignment > 0.95f,
                            "Fern leaflet plane must follow its curved rachis, not a global horizontal plane");
                        const std::array origin{c*root.along-s*root.across,root.height,
                            s*root.along+c*root.across};
                        const auto blade = PointedLeafPositions(p.angle,p.length,p.width,p.droop,
                            p.width*0.15f,p.pitch,p.roll);
                        require(blade[0] == std::array<float,3>{}
                            && FernWindWeight(p.t,0) == p.t*p.t,
                            "Fern leaflet root must share the stem position and exact wind weight");
                        for (const auto& v : blade) {
                            require(std::hypot(origin[0]+v[0],origin[2]+v[2]) < 2.4f
                                && origin[1]+v[1] > 0 && origin[1]+v[1] < 2.2f,
                                "Fern leaflet leaves its bounded above-ground envelope");
                        }
                        const auto normals = BuildSurfaceNormals(blade,triangles);
                        for (const auto& n : normals)
                            require(std::abs(std::hypot(n[0],n[1],n[2])-1) < 0.00001f,
                                "Fern leaflet has a degenerate normal");
                        for (float t : {0.0f,0.48f,1.0f})
                            require(FernWindWeight(p.t,t) >= p.t*p.t && FernWindWeight(p.t,t) <= 1,
                                "Fern wind weight escapes the shared culling envelope");
                    }
                }
            }
            for (int leaf = 0; leaf < UnderstoryLeafCount(variant,true); ++leaf) {
                const auto far = UnderstoryLeaf(variant,leaf,true);
                const auto near = UnderstoryLeaf(variant,far.source,false);
                for (int pair = 0; pair < FernPairCount; ++pair) {
                    if (!RetainFernPair(pair,true)) continue;
                    for (int side : {-1,1})
                        require(FernLeaflet(near,pair,side) == FernLeaflet(far,pair,side),
                            "LOD must not re-space or re-pose retained fern leaflets");
                }
            }
        }
        const auto invalid = [](auto action) {
            try { action(); } catch (const std::invalid_argument&) { return; }
            throw std::runtime_error("Invalid fern input accepted");
        };
        const auto frond = UnderstoryLeaf(0,0,false);
        auto broken = frond;
        broken.width = std::numeric_limits<float>::max();
        invalid([&] { (void)FernLeaflet(broken,2,1); });
        broken = frond;
        broken.angle = std::numeric_limits<float>::infinity();
        invalid([&] { (void)FernLeaflet(broken,0,1); });
        for (int pair : {-1,FernPairCount}) invalid([&] { (void)FernLeaflet(frond,pair,1); });
        for (int side : {-2,0,2}) invalid([&] { (void)FernLeaflet(frond,0,side); });
        for (float v : {-0.1f,1.1f,std::numeric_limits<float>::quiet_NaN()}) {
            invalid([&] { (void)FernWindWeight(v,0); });
            invalid([&] { (void)FernWindWeight(0,v); });
        }
    }
    {
        using namespace island::JungleMaterial;
        for (int i = 0; i <= 100; ++i) {
            const float a = i / 100.0f;
            for (int j = i; j <= 100; ++j) {
                const float b = j / 100.0f;
                for (float weight : {0.0f, 0.17f, 0.5f, 0.83f, 1.0f}) {
                    const float expected = TrunkPigment(std::lerp(a,b,weight));
                    const float interpolated = std::lerp(TrunkPigment(a),TrunkPigment(b),weight);
                    if (expected < TrunkPigmentBase || expected > TrunkPigmentTip
                        || std::abs(expected-interpolated) > 0.000001f)
                        throw std::runtime_error("Trunk pigment depends on ring segmentation");
                    for (int channel : {48, 79, 104, 116}) {
                        const float packed = std::lerp(std::round(channel*TrunkPigment(a)),
                            std::round(channel*TrunkPigment(b)), weight);
                        if (std::abs(packed-std::round(channel*expected)) > 1.00001f)
                            throw std::runtime_error("Packed trunk pigment exceeds one-byte interpolation error");
                    }
                }
            }
        }
        for (float value : {-0.01f, 1.01f, std::numeric_limits<float>::infinity(),
                            std::numeric_limits<float>::quiet_NaN()}) {
            try { (void)TrunkPigment(value); throw std::runtime_error("Invalid trunk pigment height accepted"); }
            catch (const std::invalid_argument&) { }
        }
        std::cout << "Trunk pigment: bounded height field independent of ring segmentation passed\n";
    }
    {
        using namespace island;
        const auto require = [](bool condition, const char* text) {
            if (!condition) throw std::runtime_error(text);
        };
        const auto dot = [](const auto& a, const auto& b) {
            return a[0]*b[0]+a[1]*b[1]+a[2]*b[2];
        };
        constexpr std::array<std::uint16_t,12> triangles{0,1,2,0,2,3,1,4,2,2,4,3};
        unsigned retained = 0, nearRetained = 0, upward = 0, downward = 0, positiveRoll = 0, negativeRoll = 0;
        for (int identity = 0; identity < CanopySprigCount; ++identity) {
            require(!RetainCanopySprig(identity,true) || RetainCanopySprig(identity,false),
                "Far canopy sprig has no near source identity");
            retained += RetainCanopySprig(identity,true);
            nearRetained += RetainCanopySprig(identity,false);
        }
        require(retained == 4 && nearRetained == 24, "Canopy outline sprig budget changed");
        for (bool far : {false,true}) {
            unsigned triangles = 12*6; // Eight perimeter and four lower blades.
            for (int cluster = 0; cluster < 8; ++cluster)
                for (int identity = 0; identity < CanopySprigCount; ++identity)
                    if (RetainCanopySprig(identity,far)) triangles += CanopySprigLeafCount(cluster,identity)*6;
            require(triangles == (far ? 624U : 3504U), "Curved canopy leaves exceed original triangle budget");
        }
        for (int variant = 0; variant < CanopyFormCount; ++variant) {
            for (int cluster = 0; cluster < 8; ++cluster) {
                for (int identity = 0; identity < CanopySprigCount+2; ++identity) {
                    for (int member = 0; member < 3; ++member) {
                        const auto pose = MakeCanopyLeafPosture(variant,cluster,identity,member);
                        require(pose == MakeCanopyLeafPosture(variant,cluster,identity,member), "Unstable canopy leaf identity");
                        require(std::abs(pose.pitch) < 0.87f && std::abs(pose.roll) <= 0.52001f
                            && pose.lengthScale >= 0.84599f && pose.lengthScale <= 1.12321f
                            && pose.widthScale >= 0.78199f && pose.widthScale <= 1.18321f,
                            "Canopy leaf posture/proportions escaped envelope");
                        upward += pose.pitch > 0.2f; downward += pose.pitch < -0.2f;
                        positiveRoll += pose.roll > 0.2f; negativeRoll += pose.roll < -0.2f;
                        const float yaw = cluster*0.8f+member*1.2f;
                        const auto frame = MakePointedLeafFrame(yaw,pose.pitch,pose.roll);
                        require(std::abs(dot(frame.axis,frame.axis)-1) < 0.000001f
                            && std::abs(dot(frame.side,frame.side)-1) < 0.000001f
                            && std::abs(dot(frame.normal,frame.normal)-1) < 0.000001f
                            && std::abs(dot(frame.axis,frame.side)) < 0.000001f
                            && std::abs(dot(frame.axis,frame.normal)) < 0.000001f
                            && std::abs(dot(frame.side,frame.normal)) < 0.000001f,
                            "Canopy leaf frame is not orthonormal");
                        for (const auto dimensions : {std::array{0.68f,0.19f,0.29f,0.075f},
                                                     std::array{0.98f,0.255f,0.13f,0.075f},
                                                     std::array{1.81f,0.50f,0.58f,0.16f},
                                                     std::array{1.28f,0.39f,0.84f,0.11f}}) {
                            const float length = dimensions[0]*pose.lengthScale, width = dimensions[1]*pose.widthScale;
                            const auto positions = PointedLeafPositions(yaw,length,width,dimensions[2],dimensions[3],pose.pitch,pose.roll);
                            require(positions[0] == std::array<float,3>{}, "Leaf root detached from its sprig");
                            for (const auto& p : positions)
                                require(std::hypot(p[0],p[1],p[2]) <= length+width+dimensions[2]+dimensions[3],
                                    "Pointed leaf escaped conservative local extent");
                            const auto normals = BuildSurfaceNormals(positions,triangles);
                            require(normals.size() == 5, "Pointed leaf topology changed");
                            for (const auto& n : normals)
                                require(std::abs(dot(n,n)-1) < 0.000001f, "Invalid posed leaf surface normal");
                            const auto curved = CanopyBladePositions(yaw,length,width,dimensions[2],dimensions[3],pose.pitch,pose.roll);
                            require(curved[0] == positions[0] && curved[2] == positions[2] && curved[6] == positions[4],
                                "Curved outline moved source root, crease or drip tip");
                            for (const auto& p : curved)
                                require(std::hypot(p[0],p[1],p[2]) <= length+width+dimensions[2]+dimensions[3],
                                    "Curved canopy blade escaped its conservative extent");
                            const auto curvedNormals = BuildSurfaceNormals(curved,CanopyBladeIndices);
                            require(curvedNormals.size() == 7, "Curved canopy topology changed");
                            for (const auto& n : curvedNormals)
                                require(std::abs(dot(n,n)-1) < 0.000001f, "Invalid curved blade normal");
                        }
                    }
                }
            }
        }
        require(upward > 600 && downward > 600 && positiveRoll > 900 && negativeRoll > 900,
            "Canopy leaf orientation still collapses into repeated horizontal layers");
        for (float yaw : {0.0f,0.7f,2.4f}) {
            const auto p = PointedLeafPositions(yaw,1.4f,0.3f,0.25f,0.16f);
            const float c = std::cos(yaw), s = std::sin(yaw);
            const std::array<std::array<float,3>,5> legacy{{{},
                {c*1.4f*0.48f+s*0.3f,0,s*1.4f*0.48f-c*0.3f},
                {c*1.4f*0.48f,0.16f-0.25f*0.18f,s*1.4f*0.48f},
                {c*1.4f*0.48f-s*0.3f,0,s*1.4f*0.48f+c*0.3f},
                {c*1.4f,-0.25f,s*1.4f}}};
            for (std::size_t i = 0; i < p.size(); ++i)
                for (int axis = 0; axis < 3; ++axis)
                    require(std::abs(p[i][axis]-legacy[i][axis]) < 0.000001f, "Default pointed leaf shape changed");
        }
        for (auto args : {std::array{-1,0,0,0},std::array{CanopyFormCount,0,0,0},std::array{0,8,0,0},
                          std::array{0,0,-1,0},std::array{0,0,38,0},std::array{0,0,0,3}}) {
            try { (void)MakeCanopyLeafPosture(args[0],args[1],args[2],args[3]); throw std::runtime_error("Invalid leaf identity accepted"); }
            catch (const std::invalid_argument&) { }
        }
        for (float invalid : {1.3f,std::numeric_limits<float>::infinity(),std::numeric_limits<float>::quiet_NaN()}) {
            try { (void)MakePointedLeafFrame(0,invalid,0); throw std::runtime_error("Invalid leaf pitch accepted"); }
            catch (const std::invalid_argument&) { }
            try { (void)MakePointedLeafFrame(0,0,invalid); throw std::runtime_error("Invalid leaf roll accepted"); }
            catch (const std::invalid_argument&) { }
        }
        std::cout << "Canopy leaf posture: bounded orthonormal frames, nondegenerate geometry and retained source identities passed\n";
    }
    {
        using namespace island::BarkMaterial;
        float darkest = 2, lightest = 0;
        const float unresolved = 1-island::JungleMaterial::FissureMean*0.55f*0.16f;
        for (std::uint32_t seed : {0U,17U,351U,1023U}) {
            for (int y = 0; y <= 32; ++y) {
                const float along = y*0.37f;
                for (int x = -80; x <= 80; ++x) {
                    const float angle = x*Tau/160;
                    for (float footprint : {0.0f,0.03f,0.4f,10.0f}) {
                        const float value = Sample(angle,along,footprint,footprint,seed);
                        if (!std::isfinite(value) || value < 0.75999f || value > 1.08001f
                            || value != Sample(angle,along,footprint,footprint,seed))
                            throw std::runtime_error("Bark material escapes its envelope or is nondeterministic");
                        if (std::abs(value-Sample(angle+Tau,along,footprint,footprint,seed)) > 0.0001f)
                            throw std::runtime_error("Bark warp, grain or fissure breaks angular periodicity");
                        if (footprint == 10 && std::abs(value-unresolved) > 0.000001f)
                            throw std::runtime_error("Unresolved bark does not converge to its stable reference");
                        darkest = std::min(darkest,value); lightest = std::max(lightest,value);
                    }
                }
                for (int cells : {5,8,13}) {
                    const float a = Noise(-0.00001f,along,cells,seed), b = Noise(0.00001f,along,cells,seed);
                    if (std::abs(a-b) > 0.00001f)
                        throw std::runtime_error("Bark lattice wraps discontinuously at zero angle");
                }
            }
        }
        if (lightest-darkest < 0.1f) throw std::runtime_error("Bark material lacks intended close-range variation");
        unsigned seedDifferences = 0;
        for (unsigned seed = 0; seed < 1024; ++seed) {
            const float tag = -1.0f-static_cast<float>(seed);
            if (static_cast<unsigned>(std::floor(-tag-1+0.5f)) != seed)
                throw std::runtime_error("Bark seed cannot round-trip through material tag");
            if (std::abs(Sample(0.3f,2.1f,0,0,seed)-Sample(0.3f,2.1f,0,0,0)) > 0.005f) ++seedDifferences;
        }
        if (seedDifferences < 600) throw std::runtime_error("Tree bark seeds produce insufficient material diversity");
        for (float invalid : {-1.0f,std::numeric_limits<float>::infinity(),std::numeric_limits<float>::quiet_NaN()}) {
            try { (void)Sample(0,1,invalid,0,0); throw std::runtime_error("Invalid angular bark footprint accepted"); }
            catch (const std::invalid_argument&) { }
            try { (void)Sample(0,1,0,invalid,0); throw std::runtime_error("Invalid axial bark footprint accepted"); }
            catch (const std::invalid_argument&) { }
            try { (void)Noise(0,invalid,8,0); throw std::runtime_error("Invalid axial noise coordinate accepted"); }
            catch (const std::invalid_argument&) { }
        }
        try { (void)Sample(0,1,0,0,1024); throw std::runtime_error("Invalid bark seed accepted"); }
        catch (const std::invalid_argument&) { }
        std::cout << "Bark material: bounded periodic variation, stable seed and unresolved-detail checks passed\n";
    }
    {
        using namespace island::JungleMaterial;
        for (int sides : {4, 5, 8}) {
            for (int side = 0; side < sides; ++side) {
                const float angle = 2.0f*std::numbers::pi_v<float>*side/sides;
                const auto a = WoodCoordinates(angle, 0);
                const auto b = WoodCoordinates(angle+2.0f*std::numbers::pi_v<float>/sides, 4);
                if (!ValidWoodCoordinates(a[0],a[1],a[2]) || !ValidWoodCoordinates(b[0],b[1],b[2]))
                    throw std::runtime_error("Generated wood coordinate rejected");
                for (int sample = 0; sample <= 100; ++sample) {
                    const float t = sample/100.0f;
                    const float x = std::lerp(a[0],b[0],t), y = std::lerp(a[1],b[1],t);
                    if (x*x+y*y < 0.49999f)
                        throw std::runtime_error("Wood strip interpolation approaches angular singularity");
                    // Analytic atan derivative must agree with the local finite difference,
                    // including interpolation across the negative-X branch cut.
                    const std::array<float,2> dx{(b[0]-a[0])*0.001f,(b[1]-a[1])*0.001f};
                    const float step = std::remainder(std::atan2(y+dx[1],x+dx[0])-std::atan2(y,x),
                        2.0f*std::numbers::pi_v<float>);
                    const float footprint = WoodAngularFootprint({x,y},dx,{0,0});
                    if (std::abs(footprint-std::abs(step)) > 0.000005f)
                        throw std::runtime_error("Wood angular footprint disagrees at circular seam");
                }
            }
        }
        for (int frequency : {5,9}) {
            const float a = -std::numbers::pi_v<float>*frequency+0.47f;
            const float b = std::numbers::pi_v<float>*frequency+0.47f;
            if (std::abs(std::sin(a)-std::sin(b)) > 0.00001f)
                throw std::runtime_error("Bark angular frequencies are not periodic");
        }
        if (WoodAngularFootprint({0,0},{1,0},{0,1}) != 0
            || ValidWoodCoordinates(0,0,1) || ValidWoodCoordinates(1,1,0)
            || ValidWoodCoordinates(1,0,-1))
            throw std::runtime_error("Invalid wood coordinate or cap singularity handling");
        for (float invalid : {std::numeric_limits<float>::infinity(), std::numeric_limits<float>::quiet_NaN()}) {
            if (ValidWoodCoordinates(1,0,invalid)) throw std::runtime_error("Invalid wood distance accepted");
            try { (void)WoodCoordinates(invalid, 0); throw std::runtime_error("Invalid wood angle accepted"); }
            catch (const std::invalid_argument&) { }
            try { (void)WoodCoordinates(0, invalid); throw std::runtime_error("Invalid wood distance accepted"); }
            catch (const std::invalid_argument&) { }
            try { (void)WoodAngularFootprint({1,0},{invalid,0},{0,0}); throw std::runtime_error("Invalid wood footprint accepted"); }
            catch (const std::invalid_argument&) { }
        }
        try { (void)WoodCoordinates(0, -1); throw std::runtime_error("Negative wood distance accepted"); }
        catch (const std::invalid_argument&) { }
        float previous = 1;
        for (int i = 0; i <= 1000; ++i) {
            const float x = i / 1000.0f;
            const float filter = DetailWeight(x);
            if (filter < 0 || filter > previous || (x >= 0.5f && filter != 0)
                || (x <= 0.18f && filter != 1)) throw std::runtime_error("Material filter is not bounded/monotone or misses cutoff");
            previous = filter;
            for (float up : {-1.0f, -0.3f, 0.0f, 0.3f, 1.0f}) {
                const float light = LeafDaylight(x, up);
                if (light < 0.48f || light > 1.000001f || light != LeafDaylight(-x, -up))
                    throw std::runtime_error("Leaf daylight is unbounded or not two-sided");
                const float wood = WoodDaylight(x, up);
                if (wood < WoodFill || wood > 1.000001f || wood < WoodDaylight(-x, up)
                    || WoodDaylight(-x, -std::abs(up)) != WoodFill)
                    throw std::runtime_error("Opaque wood daylight is unbounded or lights its back side");
            }
            for (float phase : {0.0f, 0.16f, 0.29f, 0.42f, 1.0f}) {
                const auto tint = LeafAgeTint(x, phase);
                if (tint[0] < 1 || tint[0] > 1.120001f || tint[1] < 0.959999f || tint[1] > 1
                    || tint[2] < 0.729999f || tint[2] > 1
                    || ((x <= 0.62f || phase >= 0.42f) && tint != std::array<float,3>{1,1,1}))
                    throw std::runtime_error("Leaf age tint escapes bounds or affects unaged blade regions");
            }
        }
        if (LeafDaylight(0, 0) >= LeafDaylight(0.82f, 1)) throw std::runtime_error("Leaf sun response lacks contrast");
        double mean = 0;
        for (int i = 0; i < 100000; ++i) {
            const float wave = static_cast<float>(std::abs(std::sin((i+0.5)*3.141592653589793/100000)));
            mean += Smooth(FissureStart, FissureEnd, wave);
        }
        if (std::abs(mean/100000 - FissureMean) > 0.000001) {
            std::cerr << "Unresolved bark fissures must retain their area mean\n"; return 1;
        }
        const auto shader = Shader();
        struct Comma final : std::numpunct<char> { char do_decimal_point() const override { return ','; } };
        const auto original = std::locale();
        std::locale::global(std::locale(original, new Comma));
        const auto localized = Shader();
        std::locale::global(original);
        if (shader != localized) throw std::runtime_error("Material GLSL depends on numeric locale");
        for (float invalid : {-0.01f, std::numeric_limits<float>::infinity(), std::numeric_limits<float>::quiet_NaN()}) {
            try { (void)DetailWeight(invalid); throw std::runtime_error("Invalid material footprint accepted"); }
            catch (const std::invalid_argument&) { }
            try { (void)LeafAgeTint(invalid, 0); throw std::runtime_error("Invalid pigment coordinate accepted"); }
            catch (const std::invalid_argument&) { }
        }
        for (float invalid : {1.01f, std::numeric_limits<float>::infinity(), std::numeric_limits<float>::quiet_NaN()}) {
            try { (void)LeafDaylight(invalid, 0); throw std::runtime_error("Invalid light direction accepted"); }
            catch (const std::invalid_argument&) { }
            try { (void)WoodDaylight(invalid, 0); throw std::runtime_error("Invalid wood light direction accepted"); }
            catch (const std::invalid_argument&) { }
        }
        std::cout << "Jungle material: bounded daylight/tip tint, Nyquist fade and bark mean passed\n";
    }
    for (int variant = 0; variant < 4; ++variant) {
        std::array<bool, 11> seen{};
        float minimumHeight = 10, maximumHeight = 0;
        for (int leaf = 0; leaf < island::UnderstoryLeafCount(variant, true); ++leaf) {
            const auto far = island::UnderstoryLeaf(variant, leaf, true);
            if (seen[far.source] || far != island::UnderstoryLeaf(variant, far.source, false)) {
                std::cerr << "Far understory must select unique, unchanged near profiles\n";
                return 1;
            }
            seen[far.source] = true;
        }
        for (int leaf = 0; leaf < island::UnderstoryLeafCount(variant, false); ++leaf) {
            const auto profile = island::UnderstoryLeaf(variant, leaf, false);
            minimumHeight = std::min(minimumHeight, profile.lift);
            maximumHeight = std::max(maximumHeight, profile.lift);
            for (int row = 0; row <= 100; ++row) {
                for (int column = -10; column <= 10; ++column) {
                    const float t = row / 100.0f, side = column / 10.0f;
                    const auto p = island::SampleUnderstoryLeaf(profile, t, side);
                    const auto n = island::UnderstoryLeafNormal(profile, t, side);
                    if (!std::isfinite(p.height) || !std::isfinite(p.across) || !std::isfinite(p.along)
                        || !std::isfinite(n.height) || !std::isfinite(n.across) || !std::isfinite(n.along)
                        || p.height < -0.00001f || p.height > 2.2f
                        || std::hypot(p.along, p.across) > 2.4f
                        || std::abs(std::hypot(n.across, n.height, n.along) - 1) > 0.0001f
                        || n.height <= 0) {
                        std::cerr << "Understory geometry/normal envelope violated: variant " << variant
                            << " leaf " << leaf << " at " << t << ',' << side << '\n';
                        return 1;
                    }
                    if (row == 0 && (p.across != 0 || p.height != 0 || p.along != 0)) {
                        std::cerr << "Understory petioles must remain rooted\n";
                        return 1;
                    }
                    ++checks;
                }
            }
            for (float t : {0.13f, 0.31f, 0.57f, 0.79f, 0.91f}) {
                for (float side : {-0.8f, -0.3f, 0.3f, 0.8f}) {
                    const auto n = island::UnderstoryLeafNormal(profile, t, side);
                    const auto a = island::SampleUnderstoryLeaf(profile, t - 0.0005f, side);
                    const auto b = island::SampleUnderstoryLeaf(profile, t + 0.0005f, side);
                    const auto c = island::SampleUnderstoryLeaf(profile, t, side - 0.001f);
                    const auto d = island::SampleUnderstoryLeaf(profile, t, side + 0.001f);
                    const auto perpendicular = [&n](const auto& p, const auto& q) {
                        const float x = q.across-p.across, y = q.height-p.height, z = q.along-p.along;
                        return std::abs(n.across*x + n.height*y + n.along*z) < 0.007f * std::hypot(x,y,z);
                    };
                    if (!perpendicular(a,b) || !perpendicular(c,d)) {
                        std::cerr << "Understory normal misses deformed profile tangents\n";
                        return 1;
                    }
                }
            }
        }
        if (maximumHeight - minimumHeight < 0.2f) {
            std::cerr << "Understory variant needs distinct leaf postures\n";
            return 1;
        }
    }
    for (bool far : {false, true}) {
        for (int variant : {-1, 0, 1, 2, 3, 4}) {
            const int end = variant >= 0 && variant < 4 ? island::UnderstoryLeafCount(variant, far) : 0;
            for (int index : {-1, end}) {
                try {
                    (void)island::UnderstoryLeaf(variant, index, far);
                    std::cerr << "Invalid understory variant/index accepted\n";
                    return 1;
                } catch (const std::invalid_argument&) { }
            }
        }
    }
    auto invalidLeaf = island::UnderstoryLeaf(0, 0, false);
    invalidLeaf.bow = std::numeric_limits<float>::quiet_NaN();
    try {
        (void)island::SampleUnderstoryLeaf(invalidLeaf, 0.5f, 0);
        std::cerr << "Non-finite understory deformation accepted\n";
        return 1;
    } catch (const std::invalid_argument&) { }
    const auto leafProfile = island::UnderstoryLeaf(0, 0, false);
    for (float invalid : {-0.1f, 1.1f, std::numeric_limits<float>::infinity(),
                          std::numeric_limits<float>::quiet_NaN()}) {
        try {
            (void)island::UnderstoryLeafNormal(leafProfile, invalid, 0);
            std::cerr << "Invalid understory profile coordinate accepted\n";
            return 1;
        } catch (const std::invalid_argument&) { }
    }
    for (int variant = 0; variant < 7; ++variant) {
        const bool ground = variant >= 4;
        const int groundVariant = variant - 4;
        const bool broad = variant == 1 || variant == 3;
        for (int leaf = 0; leaf < 11; ++leaf) {
            const float variation = (ground ? 0.86f : 0.88f) + (ground ? 0.18f : 0.16f)
                * static_cast<float>((leaf * 3 + (ground ? groundVariant : variant)) % 5) / 4.0f;
            const float length = (ground ? 0.62f + 0.06f * groundVariant
                                          : broad ? 2.05f : 1.72f) * variation;
            const float width = (ground ? 0.13f + 0.025f * groundVariant
                                         : broad ? 0.48f : 0.25f) * variation;
            const float lift = ground ? 0.43f + 0.055f * groundVariant : broad ? 1.08f : 0.74f;
            const float phase = static_cast<float>(leaf) / 11.0f + static_cast<float>(variant) * 0.19f;
            for (int row = 0; row <= 100; ++row) {
                for (int column = -10; column <= 10; ++column) {
                    const auto p = island::SampleLeafBlade(static_cast<float>(row) / 100.0f,
                        static_cast<float>(column) / 10.0f, length, width, lift, phase);
                    if (!std::isfinite(p.across) || !std::isfinite(p.height) || !std::isfinite(p.along)
                        || p.height < -0.0001f || p.height > 2.2f
                        || std::hypot(p.across, p.along) > 2.4f) {
                        std::cerr << "Leaf sample violated finite geometry/culling envelope\n";
                        return 1;
                    }
                    ++checks;
                    const auto n = island::SampleLeafBladeNormal(static_cast<float>(row) / 100.0f,
                        static_cast<float>(column) / 10.0f, length, width, lift, phase);
                    if (!std::isfinite(n.across) || !std::isfinite(n.height) || !std::isfinite(n.along)
                        || std::abs(std::hypot(n.across, n.height, n.along) - 1.0f) > 0.0001f
                        || n.height <= 0.0f) {
                        std::cerr << "Leaf normal must remain finite, unit length and upper-facing\n";
                        return 1;
                    }
                }
            }
            const auto root = island::SampleLeafBlade(0, 0, length, width, lift, phase);
            const auto tip = island::SampleLeafBlade(1, 0, length, width, lift, phase);
            if (root.across != 0 || root.height != 0 || root.along != 0 || tip.height <= 0.15f) {
                std::cerr << "Leaf root or above-ground tip invariant failed\n";
                return 1;
            }
        }
    }
    const auto centre = island::SampleLeafBlade(0.5f, 0, 2, 0.4f, 1, 0);
    for (float t : {0.12f, 0.37f, 0.66f, 0.89f}) {
        for (float s : {-0.75f, -0.25f, 0.25f, 0.75f}) {
            const auto n = island::SampleLeafBladeNormal(t, s, 2, 0.4f, 1, 0.3f);
            const auto a = island::SampleLeafBlade(t - 0.0005f, s, 2, 0.4f, 1, 0.3f);
            const auto b = island::SampleLeafBlade(t + 0.0005f, s, 2, 0.4f, 1, 0.3f);
            const auto c = island::SampleLeafBlade(t, s - 0.001f, 2, 0.4f, 1, 0.3f);
            const auto d = island::SampleLeafBlade(t, s + 0.001f, 2, 0.4f, 1, 0.3f);
            const auto perpendicular = [&n](const auto& p, const auto& q) {
                const float x = q.across - p.across, y = q.height - p.height, z = q.along - p.along;
                return std::abs(n.across * x + n.height * y + n.along * z)
                    < 0.005f * std::hypot(x, y, z);
            };
            if (!perpendicular(a, b) || !perpendicular(c, d)) {
                std::cerr << "Leaf normal must be perpendicular to both surface tangents\n";
                return 1;
            }
        }
    }
    const auto left = island::SampleLeafBlade(0.5f, -1, 2, 0.4f, 1, 0);
    const auto right = island::SampleLeafBlade(0.5f, 1, 2, 0.4f, 1, 0);
    if (centre.height <= (left.height + right.height) * 0.5f) {
        std::cerr << "Leaf midrib must rise above its margins\n";
        return 1;
    }
    for (const float invalid : {-0.1f, 1.1f, std::numeric_limits<float>::quiet_NaN()}) {
        try {
            (void)island::SampleLeafBlade(invalid, 0, 2, 0.4f, 1, 0);
            std::cerr << "Invalid leaf coordinates accepted\n";
            return 1;
        } catch (const std::invalid_argument&) { }
        try {
            (void)island::SampleLeafBladeNormal(invalid, 0, 2, 0.4f, 1, 0);
            std::cerr << "Invalid normal coordinates accepted\n";
            return 1;
        } catch (const std::invalid_argument&) { }
    }
    try {
        (void)island::SampleLeafBlade(0.5f, 0, 2, 0.4f, std::numeric_limits<float>::max(), 0);
        std::cerr << "Overflowing leaf dimensions accepted\n";
        return 1;
    } catch (const std::invalid_argument&) { }
    std::cout << "LeafBladeProfileTests: " << checks << " bounded samples passed\n";
}
catch (const std::exception& error)
{
    std::cerr << "Leaf/material test failure: " << error.what() << '\n';
    return 1;
}
