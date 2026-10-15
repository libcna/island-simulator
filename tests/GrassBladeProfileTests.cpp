#include "IslandSimulator/GrassBladeProfile.hpp"
#include "IslandSimulator/GrassMaterial.hpp"
#include "IslandSimulator/SurfaceNormals.hpp"

#include <array>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <locale>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace
{
    int failures=0;
    void Check(bool value, std::string_view message)
    {
        if (!value) { std::cerr << "FAIL: " << message << '\n'; ++failures; }
    }
    bool Near(float a,float b) { return std::abs(a-b)<0.000001f; }
}

int TestGrassMotionBounds();
int TestGrassVisibility();

int main()
{
    failures += TestGrassMotionBounds();
    failures += TestGrassVisibility();
    {
        float maximumStretch = 0.0f;
        bool observedVariation = false;
        for (int blade = 0; blade < island::NearGrassBladeCount; ++blade) {
            const auto placement = island::NearGrassBladePlacement(blade);
            island::GrassFootprintPoint first{};
            for (int variant = 0; variant <= 20; ++variant) {
                const float unit = variant / 20.0f;
                const auto varied = island::VaryGrassFootprint(
                    placement.x, placement.z, placement.x, placement.z,
                    unit, 1.0f - unit);
                const auto repeated = island::VaryGrassFootprint(
                    placement.x, placement.z, placement.x, placement.z,
                    unit, 1.0f - unit);
                Check(varied.x == repeated.x && varied.z == repeated.z,
                      "grass footprint variation is deterministic");
                const float sourceRadius = std::hypot(placement.x, placement.z);
                const float variedRadius = std::hypot(varied.x, varied.z);
                if (sourceRadius > 0.0001f) {
                    maximumStretch = std::max(maximumStretch, variedRadius / sourceRadius);
                    Check(variedRadius <= sourceRadius * island::GrassMaximumFootprintStretch
                              + 0.00001f,
                          "asymmetric footprint stays inside its conservative radial bound");
                }
                if (variant == 0) first = varied;
                else observedVariation = observedVariation
                    || std::hypot(varied.x - first.x, varied.z - first.z) > 0.001f;
            }
        }
        Check(observedVariation && maximumStretch > 1.02f,
              "instance identity produces visibly different area-preserving footprints");
        for (int invalid = 0; invalid < 4; ++invalid) {
            float x = 0.4f, z = -0.2f, phase = 0.5f, variation = 0.5f;
            if (invalid == 0) x = std::numeric_limits<float>::quiet_NaN();
            if (invalid == 1) z = std::numeric_limits<float>::infinity();
            if (invalid == 2) phase = -0.01f;
            if (invalid == 3) variation = 1.01f;
            bool rejected = false;
            try { (void)island::VaryGrassFootprint(x, z, 0.2f, 0.1f, phase, variation); }
            catch (const std::invalid_argument&) { rejected = true; }
            Check(rejected, "invalid footprint variation is rejected before shader use");
        }
        const std::string constants = island::GrassFootprintShaderConstants();
        const auto previousLocale = std::locale();
        struct Comma final : std::numpunct<char> {
            char do_decimal_point() const override { return ','; }
        };
        std::locale::global(std::locale(previousLocale, new Comma));
        const std::string localized = island::GrassFootprintShaderConstants();
        std::locale::global(previousLocale);
        Check(constants == localized && constants.find(',') == std::string::npos,
              "footprint GLSL constants ignore the process decimal locale");
    }
    {
        float previous = 0;
        for (int step = 0; step <= 1000; ++step) {
            const float height = step / 1000.0f;
            const float occlusion = island::GrassHeightOcclusion(height);
            Check(occlusion >= previous && occlusion >= island::GrassRootOcclusion && occlusion <= 1,
                  "separate basal occlusion is bounded and monotonic");
            previous = occlusion;
        }
        Check(island::GrassHeightOcclusion(0) == island::GrassRootOcclusion
              && island::GrassHeightOcclusion(1) == 1, "root and seed occlusion endpoints are exact");
        for (int column = 0; column <= 200; ++column) {
            const float across = column / 100.0f - 1;
            for (int phase = 0; phase <= 10; ++phase) {
                for (float footprint : {0.0f, 0.01f, 0.05f, 0.1f, 0.25f, 0.5f, 1.0f, 100.0f}) {
                    const float surface = island::GrassSurfaceResponse(across, phase / 10.0f, footprint);
                    Check(std::isfinite(surface) && surface >= 0.929999f && surface <= 1.105001f,
                          "grain and midrib modulation remain within the analytic bound");
                    if (footprint >= 0.5f) Check(surface == 1,
                        "unresolved blade detail is exactly neutral, independent of phase");
                    if (footprint >= 0.5f / island::GrassGrainCycles) {
                        Check(surface == island::GrassSurfaceResponse(across, 0, footprint),
                              "subpixel longitudinal veins cannot alias with phase");
                    }
                }
            }
        }
        for (float invalid : {-0.01f, 1.01f, std::numeric_limits<float>::infinity(),
                              std::numeric_limits<float>::quiet_NaN()}) {
            bool rejected = false;
            try { (void)island::GrassHeightOcclusion(invalid); }
            catch (const std::invalid_argument&) { rejected = true; }
            Check(rejected, "invalid occlusion height is rejected");
            rejected = false;
            try { (void)island::GrassSurfaceResponse(0, invalid, 0); }
            catch (const std::invalid_argument&) { rejected = true; }
            Check(rejected, "invalid surface phase is rejected");
        }
        for (const auto invalid : {std::array<float, 3>{-1.01f, 0, 0}, {1.01f, 0, 0},
                                  {2, 0, 0}, {0, 0, -0.01f},
                                  {std::numeric_limits<float>::quiet_NaN(), 0, 0},
                                  {0, 0, std::numeric_limits<float>::infinity()}}) {
            bool rejected = false;
            try { (void)island::GrassSurfaceResponse(invalid[0], invalid[1], invalid[2]); }
            catch (const std::invalid_argument&) { rejected = true; }
            Check(rejected, "invalid surface coordinate/footprint and seed tag cannot enter blade reference");
        }
    }
    {
        std::array<float, 7> nearArea{}, middleArea{}, farArea{};
        std::array<bool, island::NearGrassBladeCount> middleSources{};
        float widest = 0;
        for (int blade = 0; blade < island::NearGrassBladeCount; ++blade) {
            const auto profile = island::GrassBladeForTier(island::GrassMeshTier::Near, blade);
            // Preserve the former renderer formula independently of the new shared builder.
            const bool juvenile = blade % 6 == 5;
            const float individual = 0.87f + 0.19f * float((blade * 5 + 42) % 7) / 6.0f;
            const float juvenileHeight = juvenile ? 0.56f + 0.06f * float(blade % 3) : 1.0f;
            const bool bent = ((blade * 5 + 1) % 13) < 3;
            const island::GrassBladeShape legacy{
                (0.40f + 0.045f * float(blade % 6)) * individual * juvenileHeight,
                (0.020f + 0.0038f * float(blade % 5)) * (juvenile ? 0.76f : 1.0f),
                (0.07f + 0.018f * float(blade % 6)) * (juvenile ? 0.62f : 1.0f),
                float((blade * 11 + 3) % 9 - 4) / 4.0f,
                float((blade * 13 + 5) % 11 - 5) / 5.0f,
                blade % 3 == 0 ? -1.0f : 1.0f,
                bent ? ((blade & 1) == 0 ? -1.0f : 1.0f) * 0.040f : 0.0f,
                bent ? 0.022f : 0.0f, true, blade % 3 == 0};
            Check(profile.sourceBlade == blade && profile.shape.folded == legacy.folded,
                  "near source identity and folded topology remain unchanged");
            for (int level = 0; level < 5; ++level) {
                for (float edge : {-1.0f, 0.0f, 1.0f}) {
                    const auto actual = island::SampleGrassBlade(profile.shape, level * 0.25f, edge);
                    const auto expected = island::SampleGrassBlade(legacy, level * 0.25f, edge);
                    Check(actual.across == expected.across && actual.height == expected.height
                          && actual.along == expected.along, "near sampled mesh positions are legacy-identical");
                }
            }
            nearArea[profile.placement.tuft] += island::GrassUprightArea(profile.shape, 5);
        }
        for (auto tier : {island::GrassMeshTier::Middle, island::GrassMeshTier::Far}) {
            const int count = tier == island::GrassMeshTier::Middle ? island::MiddleGrassBladeCount
                                                                  : island::FarGrassBladeCount;
            std::array<int, 7> tuftCounts{};
            std::array<bool, island::NearGrassBladeCount> seen{};
            for (int blade = 0; blade < count; ++blade) {
                const auto proxy = island::GrassBladeForTier(tier, blade);
                auto source = island::GrassBladeForTier(island::GrassMeshTier::Near, proxy.sourceBlade);
                const auto& a = proxy.placement;
                const auto& b = source.placement;
                Check(a.x == b.x && a.z == b.z && a.facing == b.facing && a.windPhase == b.windPhase
                      && a.tuft == b.tuft, "proxy root, facing, phase and tuft match an actual near blade");
                Check(!seen[proxy.sourceBlade], "each tier has unique source representatives");
                seen[proxy.sourceBlade] = true;
                ++tuftCounts[a.tuft];
                widest = std::max(widest, proxy.shape.halfWidth);
                Check(proxy.shape.halfWidth > 0 && proxy.shape.halfWidth < 0.20f && !proxy.shape.folded,
                      "proxy widths are bounded and retain two-column topology");
                source.shape.folded = false; // Fold ridge is intentionally omitted in proxies.
                for (int level = 0; level <= 32; ++level) {
                    const auto p = island::SampleGrassBlade(proxy.shape, level / 32.0f, 0);
                    const auto q = island::SampleGrassBlade(source.shape, level / 32.0f, 0);
                    Check(p.across == q.across && p.height == q.height && p.along == q.along,
                          "proxy curvature and top remain on the source blade centreline");
                }
                const float area = island::GrassUprightArea(proxy.shape, 3);
                if (tier == island::GrassMeshTier::Middle) {
                    middleSources[proxy.sourceBlade] = true;
                    middleArea[a.tuft] += area;
                } else {
                    Check(middleSources[proxy.sourceBlade], "far source is nested in middle tier");
                    farArea[a.tuft] += area;
                }
            }
            for (int countInTuft : tuftCounts) Check(countInTuft > 0,
                "every near tuft is represented at every distance tier");
        }
        for (int tuft = 0; tuft < 7; ++tuft) {
            const float target = nearArea[tuft] * island::GrassProxyAreaRetention;
            Check(Near(middleArea[tuft], target) && Near(farArea[tuft], target),
                  "both proxy tiers preserve the same assigned upright area per tuft");
        }
        for (auto tier : {island::GrassMeshTier::Near, island::GrassMeshTier::Middle,
                           island::GrassMeshTier::Far, static_cast<island::GrassMeshTier>(-1)}) {
            const int end = tier == island::GrassMeshTier::Near ? island::NearGrassBladeCount
                : tier == island::GrassMeshTier::Middle ? island::MiddleGrassBladeCount
                : tier == island::GrassMeshTier::Far ? island::FarGrassBladeCount : 0;
            for (int invalid : {-1, end, 42}) {
                bool rejected = false;
                try { (void)island::GrassBladeForTier(tier, invalid); }
                catch (const std::invalid_argument&) { rejected = true; }
                Check(rejected, "invalid grass tier/source cannot generate GPU geometry");
            }
        }
        for (int invalid : {0, 1, 17}) {
            bool rejected = false;
            try { (void)island::GrassUprightArea(
                island::GrassBladeForTier(island::GrassMeshTier::Near, 0).shape, invalid); }
            catch (const std::invalid_argument&) { rejected = true; }
            Check(rejected, "unsupported area sample counts are rejected");
        }
        std::cout << "Grass shared LOD: seven tufts, 42/10/7 blades, widest proxy " << widest << '\n';
    }
    {
        auto previous = island::GrassBasalTint(0);
        Check(Near(previous[0], 1.22f) && Near(previous[1], 0.87f) && Near(previous[2], 0.79f),
              "basal pigment is bounded, warm and subdued");
        for (int step = 0; step <= 1000; ++step) {
            const float t = static_cast<float>(step) / 1000;
            const auto tint = island::GrassBasalTint(t);
            Check(tint[0] <= previous[0] && tint[1] >= previous[1] && tint[2] >= previous[2],
                  "basal colour converges monotonically to the original leaf");
            Check(tint[0] >= 1 && tint[0] <= 1.22f && tint[1] >= 0.87f && tint[1] <= 1
                  && tint[2] >= 0.79f && tint[2] <= 1, "basal colour remains finite and bounded");
            if (t >= 0.24f) Check(tint == std::array<float, 3>{1, 1, 1},
                                  "upper blade material remains exactly unchanged");
            const float decoded = island::EncodeGrassMaterialHeight(t) / 255.0f;
            Check(std::abs(decoded - t) <= 1.0f / 510.0f + 0.000001f,
                  "material-height byte has at most half a quantization-step error");
            for (int levels : {3, 5}) {
                const float section = t * (levels - 1);
                const int lower = std::min(levels - 2, static_cast<int>(section));
                const float a = island::EncodeGrassMaterialHeight(float(lower) / (levels - 1)) / 255.0f;
                const float b = island::EncodeGrassMaterialHeight(float(lower + 1) / (levels - 1)) / 255.0f;
                const auto interpolated = island::GrassBasalTint(std::lerp(a, b, section - lower));
                for (int c = 0; c < 3; ++c) Check(std::abs(interpolated[c] - tint[c]) < 0.0028f,
                    "three/five-level meshes share the same per-fragment basal profile");
            }
            // Normalized geometric height, not wind weighting: identical treatment for turf.
            for (const float height : {0.16f, 0.40f, 0.65f}) {
                const auto shortTint = island::GrassBasalTint((t * height) / height);
                for (int c = 0; c < 3; ++c) Check(Near(tint[c], shortTint[c]),
                    "root material is independent of blade stature");
            }
            previous = tint;
        }
        for (float invalid : {-0.01f, 1.01f, std::numeric_limits<float>::infinity(),
                              std::numeric_limits<float>::quiet_NaN()}) {
            bool rejected = false;
            try { (void)island::GrassBasalTint(invalid); }
            catch (const std::invalid_argument&) { rejected = true; }
            Check(rejected, "invalid basal tint input is rejected before mesh upload");
            rejected = false;
            try { (void)island::EncodeGrassMaterialHeight(invalid); }
            catch (const std::invalid_argument&) { rejected = true; }
            Check(rejected, "invalid normalized material height cannot enter the alpha byte");
        }
        Check(island::EncodeGrassMaterialHeight(0) == 0 && island::EncodeGrassMaterialHeight(1) == 255,
              "root/seed-tip material heights have exact endpoints");
        const auto shader = island::GrassMaterialShaderConstants();
        struct Comma final : std::numpunct<char> { char do_decimal_point() const override { return ','; } };
        const auto previousLocale = std::locale();
        std::locale::global(std::locale(previousLocale, new Comma));
        const auto localizedShader = island::GrassMaterialShaderConstants();
        std::locale::global(previousLocale);
        Check(shader == localizedShader, "grass material GLSL literals ignore process decimal locale");
        std::istringstream scalar(shader.substr(shader.find('=') + 1));
        scalar.imbue(std::locale::classic());
        float endpoint = 0;
        scalar >> endpoint;
        Check(endpoint == island::GrassBasalEnd, "grass material endpoint round-trips into GLSL");
        std::istringstream vector(shader.substr(shader.find('(') + 1));
        vector.imbue(std::locale::classic());
        for (int c = 0; c < 3; ++c) {
            float value = 0;
            char separator = 0;
            vector >> value >> separator;
            Check(value == island::GrassBasalShift[c] && separator == (c == 2 ? ')' : ','),
                  "grass material channel shift round-trips into GLSL");
        }
    }
    {
        float minimumRadius = 2, maximumRadius = 0;
        for (int blade = 0; blade < island::NearGrassTurfBladeCount; ++blade) {
            const auto turf = island::NearGrassTurfBlade(blade);
            const auto repeated = island::NearGrassTurfBlade(blade);
            const auto& p = turf.placement;
            Check(p.x == repeated.placement.x && p.z == repeated.placement.z
                  && p.facing == repeated.placement.facing
                  && turf.shape.height == repeated.shape.height, "low turf is deterministic");
            const float radius = std::hypot(p.x, p.z);
            minimumRadius = std::min(minimumRadius, radius);
            maximumRadius = std::max(maximumRadius, radius);
            Check(radius < 1.10f && p.windPhase >= 0 && p.windPhase < 1,
                  "low turf roots and wind phase are bounded");
            Check(turf.shape.height >= 0.16f && turf.shape.height <= 0.270001f,
                  "turf remains shorter than the tall tuft layer");
            for (int other = 0; other < blade; ++other) {
                const auto q = island::NearGrassTurfBlade(other).placement;
                Check(std::hypot(p.x - q.x, p.z - q.z) > 0.10f,
                      "low turf roots are distributed rather than stacked");
            }
            std::vector<std::array<float, 3>> positions;
            const float c = std::cos(p.facing), s = std::sin(p.facing);
            for (int vertex = 0; vertex < 5; ++vertex) {
                const float t = (vertex / 2) * 0.5f;
                const float edge = vertex == 4 ? 0.0f : (vertex % 2 ? 1.0f : -1.0f);
                const auto point = island::SampleGrassBlade(turf.shape, t, edge);
                const float x = p.x - s * point.across + c * point.along;
                const float z = p.z + c * point.across + s * point.along;
                Check(std::hypot(x, z) < 1.31f && point.height >= 0 && point.height < 0.28f,
                      "low turf geometry has a bounded above-ground envelope");
                positions.push_back({x, point.height, z});
            }
            const std::vector<std::uint16_t> indices{0, 2, 1, 1, 2, 3, 2, 4, 3};
            const auto tip = island::SampleGrassBlade(turf.shape, 1, 0);
            const auto left = island::SampleGrassBlade(turf.shape, 1, -1);
            const auto right = island::SampleGrassBlade(turf.shape, 1, 1);
            Check(Near(tip.across, (left.across + right.across) * 0.5f)
                  && Near(tip.height, left.height) && std::abs(left.across-right.across) < 0.0006f,
                  "shared turf tip collapses only the submillimetre terminal edge");
            const auto normals = island::BuildSurfaceNormals(positions, indices);
            for (const auto n : normals) Check(Near(std::hypot(n[0], n[1], n[2]), 1),
                "every short curved strip has valid shared normals");
        }
        Check(minimumRadius < 0.1f && maximumRadius > 1.08f,
              "low turf spans the centre and outer footprint");
        for (int invalid : {-1, island::NearGrassTurfBladeCount, std::numeric_limits<int>::max()}) {
            bool rejected = false;
            try { (void)island::NearGrassTurfBlade(invalid); }
            catch (const std::invalid_argument&) { rejected = true; }
            Check(rejected, "invalid low turf index is rejected");
        }
    }
    {
        std::array<int,7> populations{};
        std::array<float,7> phaseMin{},phaseMax{};
        phaseMin.fill(1);
        float nearestSum=0;
        for (int blade=0; blade<island::NearGrassBladeCount; ++blade) {
            const auto p=island::NearGrassBladePlacement(blade);
            const auto repeated=island::NearGrassBladePlacement(blade);
            Check(p.x==repeated.x && p.z==repeated.z && p.facing==repeated.facing
                  && p.windPhase==repeated.windPhase && p.tuft==repeated.tuft,
                  "near grass placement is deterministic");
            Check(p.tuft>=0 && p.tuft<7,"near grass has a valid tuft");
            if (p.tuft<0 || p.tuft>=7) continue;
            ++populations[p.tuft];
            phaseMin[p.tuft]=std::min(phaseMin[p.tuft],p.windPhase);
            phaseMax[p.tuft]=std::max(phaseMax[p.tuft],p.windPhase);
            Check(std::hypot(p.x,p.z)<1.02f,"tuft roots stay inside the old near cluster envelope");
            Check(p.windPhase>=0 && p.windPhase<1 && std::isfinite(p.facing),
                  "wind phase and fanning are finite and bounded");
            float nearest=10;
            for (int other=0; other<island::NearGrassBladeCount; ++other) {
                if (other==blade) continue;
                const auto q=island::NearGrassBladePlacement(other);
                const float distance=std::hypot(p.x-q.x,p.z-q.z);
                Check(distance>0.015f,"tuft roots are not coincident");
                nearest=std::min(nearest,distance);
            }
            nearestSum+=nearest;
        }
        Check(populations==std::array<int,7>{8,5,7,4,6,7,5},"tufts have unequal populations totalling 42");
        Check(nearestSum/island::NearGrassBladeCount<0.16f
              && nearestSum/island::NearGrassBladeCount>0.09f,
              "root neighbourhoods form loose tufts, not tight identical bouquets");
        for (int group=0; group<7; ++group) {
            Check(phaseMax[group]-phaseMin[group]<0.064f && phaseMax[group]>phaseMin[group],
                  "tuft wind is coherent but not identical for every blade");
        }
        for (int invalid:{-1,island::NearGrassBladeCount,std::numeric_limits<int>::max()}) {
            bool rejected=false;
            try { (void)island::NearGrassBladePlacement(invalid); }
            catch (const std::invalid_argument&) { rejected=true; }
            Check(rejected,"invalid tuft blade index is rejected");
        }
    }
    for (int blade=0; blade<42; ++blade) {
        for (bool refined : {false,true}) {
            const bool bent=refined && ((blade*5+1)%13)<3;
            const island::GrassBladeShape shape{0.40f+0.045f*(blade%6),0.020f+0.0038f*(blade%5),
                0.07f+0.018f*(blade%6), static_cast<float>((blade*11+3)%9-4)/4,
                static_cast<float>((blade*13+5)%11-5)/5,blade%3==0?-1.0f:1.0f,
                bent?((blade&1)?0.040f:-0.040f):0.0f,bent?0.022f:0.0f,refined,refined && blade%3==0};
            float previousWidth=1;
            for (int step=0; step<=100; ++step) {
                const float t=static_cast<float>(step)/100;
                const auto left=island::SampleGrassBlade(shape,t,-1);
                const auto right=island::SampleGrassBlade(shape,t,1);
                const auto centre=island::SampleGrassBlade(shape,t,0);
                if (refined) {
                    const auto placement=island::NearGrassBladePlacement(blade);
                    const float c=std::cos(placement.facing),s=std::sin(placement.facing);
                    for (auto point:{left,centre,right}) {
                        const float x=placement.x-s*point.across+c*point.along;
                        const float z=placement.z+c*point.across+s*point.along;
                        Check(std::hypot(x,z)<1.20f,"tuft blades fit the existing undeformed mesh envelope");
                    }
                }
                const float width=right.across-left.across;
                Check(width>0 && width<2.1f*shape.halfWidth,"blade width stays positive and bounded");
                Check(centre.height>=0 && centre.height<=shape.height,"blade stays in its vertical bounds");
                Check(centre.along>=0 && centre.along<=shape.lean+shape.halfWidth,
                      "fold and centreline stay within existing lean bounds");
                Check(std::abs(centre.across)<=0.058f,"lateral bow stays within existing bounds");
                Check(Near(left.height,right.height),"both edges follow the same centreline height");
                Check(centre.along>=left.along && centre.along>=right.along,
                      "fold ridge never inverts below its sides");
                if (refined && step>=35) Check(width<previousWidth,"upper near blade tapers progressively");
                previousWidth=width;
            }
            const auto tip=island::SampleGrassBlade(shape,1,0);
            Check(Near(tip.height,shape.height-shape.tipDrop),"seed anchor includes terminal drop");
            Check(Near(tip.across,(refined?shape.lateralBow*0.020f:0)+shape.tipDeflection),
                  "seed anchor includes terminal bow and bend");
            Check(Near(tip.along,shape.lean+(shape.folded?shape.halfWidth*0.018f*0.48f:0)),
                  "seed anchor lies on terminal fold ridge");
            const auto leftTip=island::SampleGrassBlade(shape,1,-1);
            const auto rightTip=island::SampleGrassBlade(shape,1,1);
            Check(Near(tip.across,(leftTip.across+rightTip.across)*0.5f),
                  "seed attachment is centred on the terminal edge");

            std::vector<std::array<float,3>> points;
            std::vector<std::uint16_t> indices;
            const int columns=shape.folded?3:2;
            for (int level=0; level<5; ++level) {
                for (int col=0; col<columns; ++col) {
                    const auto p=island::SampleGrassBlade(shape,level/4.0f,2.0f*col/(columns-1)-1);
                    points.push_back({p.across,p.height,p.along});
                }
            }
            for (int level=0; level<4; ++level) for(int col=0; col<columns-1; ++col) {
                const auto a=static_cast<std::uint16_t>(level*columns+col);
                const auto b=static_cast<std::uint16_t>(a+1);
                const auto c=static_cast<std::uint16_t>(a+columns);
                const auto d=static_cast<std::uint16_t>(c+1);
                indices.insert(indices.end(),{a,c,b,b,c,d});
            }
            const auto normals=island::BuildSurfaceNormals(points,indices);
            for (const auto n:normals) Check(Near(std::hypot(n[0],n[1],n[2]),1),
                "sampled strips have finite nondegenerate shared normals");
            if (!refined) {
                const auto middle=island::SampleGrassBlade(shape,0.5f,1);
                Check(Near(middle.across,0.012f*shape.sideSign+shape.halfWidth),
                      "distant blade retains its previous middle width");
            }
        }
    }
    for (int invalid=0; invalid<14; ++invalid) {
        island::GrassBladeShape shape{0.5f,0.02f,0.1f,0,0,1,0,0,true,false};
        float t=0.5f,side=0;
        switch (invalid) {
        case 0: t=-0.1f; break;
        case 1: t=1.1f; break;
        case 2: side=1.1f; break;
        case 3: shape.height=0; break;
        case 4: shape.halfWidth=-1; break;
        case 5: shape.lean=-1; break;
        case 6: shape.lateralBow=2; break;
        case 7: shape.edgeAsymmetry=2; break;
        case 8: shape.sideSign=2; break;
        case 9: shape.tipDrop=shape.height; break;
        case 10: shape.tipDrop=-1; break;
        case 11: shape.tipDeflection=std::numeric_limits<float>::infinity(); break;
        case 12: t=std::numeric_limits<float>::quiet_NaN(); break;
        case 13:
            t=1;
            shape.folded=true;
            shape.lean=shape.halfWidth=std::numeric_limits<float>::max();
            break;
        }
        bool rejected=false;
        try { (void)island::SampleGrassBlade(shape,t,side); }
        catch (const std::invalid_argument&) { rejected=true; }
        Check(rejected,"invalid blade input is rejected before GPU upload");
    }
    if (failures) return 1;
    std::cout << "Grass blade profiles, progressive taper, seed anchors and strip normals passed\n";
}
