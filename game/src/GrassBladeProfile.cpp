#include "IslandSimulator/GrassBladeProfile.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <numbers>
#include <stdexcept>

namespace island
{
    namespace
    {
        // The seven tallest source blades preserve each tuft's upper silhouette. Three extra
        // middle representatives cover the largest tufts; all far sources also exist in middle.
        constexpr std::array<int, MiddleGrassBladeCount> MiddleSources{0, 4, 10, 13, 16, 22, 28, 30, 34, 40};
        constexpr std::array<int, FarGrassBladeCount> FarSources{4, 10, 16, 22, 28, 34, 40};

        GrassBladeShape NearShape(int blade)
        {
            const bool juvenile = blade % 6 == 5;
            const float individual = 0.87f
                + 0.19f * static_cast<float>((blade * 5 + NearGrassBladeCount) % 7) / 6.0f;
            const float juvenileHeight = juvenile ? 0.56f + 0.06f * static_cast<float>(blade % 3) : 1.0f;
            const bool bentTip = ((blade * 5 + 1) % 13) < 3;
            const float tipDirection = (blade & 1) == 0 ? -1.0f : 1.0f;
            return {
                (0.40f + 0.045f * static_cast<float>(blade % 6)) * individual * juvenileHeight,
                (0.018f + 0.0035f * static_cast<float>(blade % 5)) * (juvenile ? 0.74f : 1.0f),
                (0.07f + 0.018f * static_cast<float>(blade % 6)) * (juvenile ? 0.62f : 1.0f),
                static_cast<float>((blade * 11 + 3) % 9 - 4) / 4.0f,
                static_cast<float>((blade * 13 + 5) % 11 - 5) / 5.0f,
                blade % 3 == 0 ? -1.0f : 1.0f, bentTip ? tipDirection * 0.030f : 0.0f,
                bentTip ? 0.018f : 0.0f, true, blade % 3 == 0};
        }
    }

    GrassBladePlacement NearGrassBladePlacement(const int blade)
    {
        if (blade < 0 || blade >= NearGrassBladeCount) {
            throw std::invalid_argument("Near grass blade index outside shared mesh");
        }
        struct Tuft { int count; float x, z, facing, spread, phase; };
        // A shared, deliberately asymmetric footprint. Instance yaw/scale/colour/wind still
        // vary in the world; unequal populations avoid seven identical radial bouquets.
        static constexpr std::array<Tuft,7> tufts{{
            {8,-0.62f,-0.39f,0.21f,0.150f,0.12f},
            {5, 0.19f,-0.77f,1.73f,0.110f,0.41f},
            {7, 0.74f,-0.12f,3.12f,0.190f,0.68f},
            {4, 0.40f, 0.61f,0.96f,0.100f,0.24f},
            {6,-0.37f, 0.69f,4.43f,0.140f,0.82f},
            {7,-0.76f, 0.22f,2.34f,0.175f,0.55f},
            {5, 0.02f,-0.01f,5.61f,0.125f,0.03f}}};
        static_assert([] { int count=0; for (auto t:tufts) count+=t.count; return count; }()
                      == NearGrassBladeCount);
        int local=blade;
        for (int group=0; group<static_cast<int>(tufts.size()); ++group) {
            const auto& tuft=tufts[group];
            if (local>=tuft.count) { local-=tuft.count; continue; }
            const float angle=tuft.facing+2.39996323f*local;
            // Short juvenile blades reach beyond the tuft core, softening bare inter-tuft gaps.
            const float radius=tuft.spread*std::sqrt((local+0.45f)/tuft.count)
                + (blade%6==5 ? 0.065f : 0.0f);
            return {tuft.x+std::cos(angle)*radius,tuft.z+std::sin(angle)*radius,
                angle+0.27f*std::sin(local*1.71f+group),
                tuft.phase+0.009f*(local-(tuft.count-1)*0.5f),group};
        }
        throw std::logic_error("Near grass tuft populations do not cover the mesh");
    }

    GrassTurfBlade NearGrassTurfBlade(const int blade)
    {
        if (blade < 0 || blade >= NearGrassTurfBladeCount) {
            throw std::invalid_argument("Near turf blade index outside shared mesh");
        }
        const float angle = 0.73f + 2.39996323f * blade;
        const float radius = 1.10f * std::sqrt((blade + 0.35f) / NearGrassTurfBladeCount);
        const float facing = angle + 0.65f * std::sin(blade * 1.37f);
        return {
            {std::cos(angle) * radius, std::sin(angle) * radius, facing,
             static_cast<float>((blade * 17 + 3) % 47) / 47.0f, -1},
            {0.16f + 0.022f * (blade % 6), 0.010f + 0.002f * (blade % 4),
             0.06f + 0.016f * (blade % 6),
             static_cast<float>((blade * 7 + 2) % 9 - 4) / 4.0f,
             static_cast<float>((blade * 11 + 1) % 7 - 3) / 3.0f,
             (blade & 1) ? -1.0f : 1.0f, 0.0f, 0.025f, true, false}};
    }

    float GrassUprightArea(const GrassBladeShape& shape, const int levels)
    {
        if (levels < 2 || levels > 16) throw std::invalid_argument("Unsupported grass area sampling levels");
        float area = 0, previousWidth = 0, previousHeight = 0;
        for (int level = 0; level < levels; ++level) {
            const float t = static_cast<float>(level) / static_cast<float>(levels - 1);
            const auto left = SampleGrassBlade(shape, t, -1);
            const auto right = SampleGrassBlade(shape, t, 1);
            const float width = right.across - left.across;
            if (level) area += 0.5f * (previousWidth + width) * std::abs(right.height - previousHeight);
            previousWidth = width;
            previousHeight = right.height;
        }
        return area;
    }

    GrassLodBlade GrassBladeForTier(const GrassMeshTier tier, const int blade)
    {
        const int count = tier == GrassMeshTier::Near ? NearGrassBladeCount
            : tier == GrassMeshTier::Middle ? MiddleGrassBladeCount
            : tier == GrassMeshTier::Far ? FarGrassBladeCount : 0;
        if (blade < 0 || blade >= count) throw std::invalid_argument("Grass blade outside requested LOD");
        const auto sourceAt = [tier](int index) {
            return tier == GrassMeshTier::Middle ? MiddleSources[static_cast<std::size_t>(index)]
                                                : FarSources[static_cast<std::size_t>(index)];
        };
        const int source = tier == GrassMeshTier::Near ? blade : sourceAt(blade);
        GrassLodBlade result{NearGrassBladePlacement(source), NearShape(source), source};
        if (tier == GrassMeshTier::Near) return result;

        float assignedArea = 0;
        for (int near = 0; near < NearGrassBladeCount; ++near) {
            const auto root = NearGrassBladePlacement(near);
            if (root.tuft != result.placement.tuft) continue;
            float closest = std::numeric_limits<float>::infinity();
            int representative = -1;
            for (int candidate = 0; candidate < count; ++candidate) {
                const auto proxy = NearGrassBladePlacement(sourceAt(candidate));
                if (proxy.tuft != root.tuft) continue;
                const float dx = root.x - proxy.x, dz = root.z - proxy.z;
                const float distance = dx * dx + dz * dz;
                if (distance < closest) { closest = distance; representative = candidate; }
            }
            if (representative == blade) assignedArea += GrassUprightArea(NearShape(near), 5);
        }
        result.shape.folded = false;
        const float originalArea = GrassUprightArea(result.shape, 3);
        if (!(assignedArea > 0 && originalArea > 0)) throw std::logic_error("Empty grass proxy assignment");
        result.shape.halfWidth *= GrassProxyAreaRetention * assignedArea / originalArea;
        return result;
    }

    GrassBladePoint SampleGrassBlade(const GrassBladeShape& shape, const float t, const float side)
    {
        for (const float value : {shape.height, shape.halfWidth, shape.lean, shape.lateralBow,
                                  shape.edgeAsymmetry, shape.sideSign, shape.tipDeflection,
                                  shape.tipDrop, t, side}) {
            if (!std::isfinite(value)) throw std::invalid_argument("Non-finite grass blade input");
        }
        if (t < 0 || t > 1 || std::abs(side) > 1 || shape.height <= 0 || shape.halfWidth <= 0
            || shape.lean < 0 || std::abs(shape.lateralBow) > 1 || std::abs(shape.edgeAsymmetry) > 1
            || std::abs(shape.sideSign) > 1 || shape.tipDrop < 0 || shape.tipDrop >= shape.height) {
            throw std::invalid_argument("Grass blade input outside supported range");
        }
        const float arch = (t == 0 || t == 1) ? 0.0f : std::sin(t * std::numbers::pi_v<float>);
        // Refined profiles narrow continuously through the upper half. LOD representatives
        // reuse the same profile; the legacy option remains available for explicit callers.
        const float taper = shape.refined ? 0.018f + (1.0f-t) * (0.68f + 0.72f*arch)
                                         : (t == 1 ? 0.018f : 0.58f + 0.42f*arch);
        const float tipWeight = std::clamp((t-0.52f)/0.48f,0.0f,1.0f);
        const float tipCurve = tipWeight*tipWeight;
        const float asymmetry = shape.refined ? shape.edgeAsymmetry*0.14f*arch : 0.0f;
        const float centreAcross = 0.012f*arch*shape.sideSign
            + (shape.refined ? shape.lateralBow*0.016f*t*t : 0.0f)
            + shape.tipDeflection*tipCurve;
        const float centreAlong = shape.lean*t*t
            + (shape.refined ? std::abs(shape.lateralBow)*0.010f*arch : 0.0f);
        const GrassBladePoint point{
            centreAcross + side*shape.halfWidth*taper*(1.0f-side*asymmetry),
            shape.height*t - shape.tipDrop*tipCurve,
            centreAlong + (shape.folded ? (1.0f-std::abs(side))*shape.halfWidth*taper*0.48f : 0.0f)};
        if (!std::isfinite(point.across) || !std::isfinite(point.height) || !std::isfinite(point.along)) {
            throw std::invalid_argument("Grass blade dimensions overflow geometry");
        }
        return point;
    }
}
