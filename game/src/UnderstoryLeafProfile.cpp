#include "IslandSimulator/UnderstoryLeafProfile.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>
#include <stdexcept>

namespace island
{
    int UnderstoryLeafCount(int variant, bool far)
    {
        if (variant < 0 || variant >= 4) throw std::invalid_argument("Invalid understory variant");
        return far ? 5 : 8 + variant;
    }

    UnderstoryLeafProfile UnderstoryLeaf(int variant, int index, bool far)
    {
        if (index < 0 || index >= UnderstoryLeafCount(variant, far))
            throw std::invalid_argument("Invalid understory leaf index");
        constexpr std::array<std::array<int, 5>, 4> sources{{
            {0, 2, 3, 5, 7}, {0, 2, 4, 6, 8}, {0, 2, 4, 7, 9}, {0, 2, 5, 7, 10}}};
        const int source = far ? sources[variant][index] : index;
        const bool broad = variant == 1 || variant == 3;
        const bool emerging = source % 4 == 1;
        const float age = static_cast<float>((source * 7 + variant * 3 + 2) % 13) / 12;
        const float posture = static_cast<float>((source * 5 + variant * 7 + 1) % 11) / 10;
        const float sign = source % 2 ? -1.0f : 1.0f;
        return {source,
            variant * 0.49f + source * 2.39996323f + 0.17f * std::sin(source * 1.73f + variant),
            (broad ? 2.05f : 1.72f) * (0.74f + 0.30f * age) * (emerging ? 0.60f : 1),
            (broad ? 0.48f : 0.25f) * (0.80f + 0.24f * age) * (emerging ? 0.64f : 1),
            (broad ? 1.08f : 0.74f) * (0.72f + 0.56f * posture) * (emerging ? 1.12f : 1),
            std::fmod(source / static_cast<float>(8 + variant) + variant * 0.19f, 1.0f),
            sign * (0.045f + 0.115f * posture),
            emerging ? 0.012f : 0.035f + 0.075f * age,
            sign * (0.025f + 0.055f * age),
            0.12f + 0.055f * posture,
            emerging ? 1.04f : 0.91f + 0.09f * posture,
            variant == 1 ? UnderstoryBladeShape::Paddle : (variant == 3 ? UnderstoryBladeShape::Lance
                : UnderstoryBladeShape::Narrow)};
    }

    LeafBladePoint SampleUnderstoryLeaf(const UnderstoryLeafProfile& leaf, float t, float side)
    {
        for (float value : {leaf.bow, leaf.tipDrop, leaf.edgeCurl, leaf.petiole})
            if (!std::isfinite(value)) throw std::invalid_argument("Non-finite understory deformation");
        if (std::abs(leaf.bow) > 0.3f || leaf.tipDrop < 0 || leaf.tipDrop > 0.2f
            || std::abs(leaf.edgeCurl) > 0.15f || leaf.petiole < 0 || leaf.petiole > 0.3f)
            throw std::invalid_argument("Unsupported understory deformation");
        float blade = std::clamp((t - leaf.petiole) / 0.20f, 0.0f, 1.0f);
        blade = blade * blade * (3 - 2 * blade);
        float halfWidth = leaf.width * (0.10f + 0.90f * blade);
        // Validate normalized coordinates before deriving the lamina remapping.
        auto point = SampleLeafBlade(t,side,leaf.length,halfWidth,leaf.lift,leaf.phase);
        const bool broad = leaf.shape != UnderstoryBladeShape::Narrow;
        if (broad && leaf.shape != UnderstoryBladeShape::Paddle && leaf.shape != UnderstoryBladeShape::Lance)
            throw std::invalid_argument("Invalid understory blade shape");
        const float arch = t == 0 || t == 1 ? 0 : std::sin(t * std::numbers::pi_v<float>);
        if (broad && arch > 0.000001f) {
            const float body = std::clamp((t-leaf.petiole)/(1-leaf.petiole),0.0f,1.0f);
            const float bias = leaf.shape == UnderstoryBladeShape::Paddle ? 0.72f : 0.95f;
            const float taper = std::pow(std::max(0.0f,std::sin(std::pow(body,bias)*std::numbers::pi_v<float>)),
                leaf.shape == UnderstoryBladeShape::Paddle ? 0.68f : 1.12f);
            const float originalTaper = std::pow(arch,0.70f);
            // The vanishing endpoint can round body to one before t reaches one.
            halfWidth = leaf.width*std::max(0.00001f,0.10f*(1-blade)+blade*taper/originalTaper);
            // Broad, low-frequency margin changes are represented by the existing eight rows.
            halfWidth *= 1+0.065f*side*std::sin(leaf.phase*6.2831853f+t*7.0f)
                +0.025f*side*side*std::sin(t*13.0f+leaf.phase*4.0f)*blade;
            point = SampleLeafBlade(t,side,leaf.length,halfWidth,leaf.lift,leaf.phase);
        }
        const float tip = std::clamp((t - 0.58f) / 0.42f, 0.0f, 1.0f);
        if (broad) {
            const float span = halfWidth*std::pow(arch,0.70f);
            point.height += span*(0.13f*(1-side*side)-0.18f*(1-std::abs(side)));
            point.height += 0.022f*side*side*side*arch*blade
                *std::sin(t*13.0f+leaf.phase*6.2831853f);
        }
        point.across += leaf.bow * t * t;
        point.height += leaf.edgeCurl * side * side * arch * blade - leaf.tipDrop * tip * tip;
        return point;
    }

    bool RetainFernPair(int pair, bool far)
    {
        if (pair < 0 || pair >= FernPairCount) throw std::invalid_argument("Invalid fern pair");
        return !far || pair % 2 == 0;
    }

    float FernWindWeight(float attachmentT, float along)
    {
        if (!std::isfinite(attachmentT) || !std::isfinite(along)
            || attachmentT < 0 || attachmentT > 1 || along < 0 || along > 1)
            throw std::invalid_argument("Invalid fern wind coordinate");
        return std::lerp(attachmentT*attachmentT,
            std::min(1.0f,attachmentT*attachmentT+0.12f),along);
    }

    FernLeafletProfile FernLeaflet(const UnderstoryLeafProfile& frond, int pair, int side)
    {
        (void)RetainFernPair(pair, false);
        if (side != -1 && side != 1) throw std::invalid_argument("Invalid fern leaflet side");
        const float t = FernRachisKnots[pair+1];
        // Validate the source profile before producing derived geometry.
        (void)SampleUnderstoryLeaf(frond,t,0);
        if (!std::isfinite(frond.angle)) throw std::invalid_argument("Invalid fern angle");
        const float taper = std::sin(t*std::numbers::pi_v<float>);
        const float variation = 0.94f+0.06f*std::sin(pair*2.13f+frond.phase*6.28f+side);
        const float length = frond.width*1.75f*taper*(1.08f-0.50f*t)*variation;
        const auto a = SampleUnderstoryLeaf(frond,t-0.001f,0);
        const auto b = SampleUnderstoryLeaf(frond,t+0.001f,0);
        const float norm = std::hypot(b.across-a.across,b.height-a.height,b.along-a.along);
        if (!std::isfinite(norm) || norm <= 0 || !std::isfinite(length) || length <= 0)
            throw std::invalid_argument("Unrepresentable fern frame");
        const float tx = (b.across-a.across)/norm, ty = (b.height-a.height)/norm;
        const float tz = (b.along-a.along)/norm;
        const float spread = 1.02f+0.16f*t, forward = std::cos(spread);
        const float x = tx*forward+side*std::sin(spread), y = ty*forward, z = tz*forward;
        const float c = std::cos(frond.angle), s = std::sin(frond.angle);
        const float worldX = c*z-s*x, worldZ = s*z+c*x;
        const float yaw = std::atan2(worldZ,worldX);
        const float pitch = std::atan2(y,std::hypot(worldX,worldZ));
        // Lateral axis crossed with the rachis tangent gives the upward frond-plane normal.
        // Roll the leaflet into that plane instead of leaving every blade horizontal.
        const float nx = -c*ty, ny = tz, nz = -s*ty;
        const float sideDot = -nx*std::sin(yaw)+nz*std::cos(yaw);
        const float upDot = -nx*std::cos(yaw)*std::sin(pitch)+ny*std::cos(pitch)
            -nz*std::sin(yaw)*std::sin(pitch);
        // Emerging fronds can be almost vertical. Limit the secondary blade's roll to the
        // supported pointed-leaf range while preserving the rachis-derived direction.
        const float roll = std::clamp(std::atan2(-sideDot,upDot),-1.15f,1.15f);
        const float width = length*(0.21f+0.025f*std::cos(pair+frond.phase));
        if (!std::isfinite(width) || width <= 0)
            throw std::invalid_argument("Unrepresentable fern leaflet width");
        return {t,yaw,length,width,pitch,roll,length*0.10f};
    }

    LeafBladePoint UnderstoryLeafNormal(const UnderstoryLeafProfile& leaf, float t, float side)
    {
        (void)SampleUnderstoryLeaf(leaf, t, side);
        t = std::clamp(t, 0.002f, 0.998f);
        const auto a = SampleUnderstoryLeaf(leaf, t - 0.001f, side);
        const auto b = SampleUnderstoryLeaf(leaf, t + 0.001f, side);
        const auto c = SampleUnderstoryLeaf(leaf, t, std::max(-1.0f, side - 0.002f));
        const auto d = SampleUnderstoryLeaf(leaf, t, std::min(1.0f, side + 0.002f));
        const float tx = b.across-a.across, ty = b.height-a.height, tz = b.along-a.along;
        const float sx = d.across-c.across, sy = d.height-c.height, sz = d.along-c.along;
        const float nx = ty*sz-tz*sy, ny = tz*sx-tx*sz, nz = tx*sy-ty*sx;
        const float magnitude = std::hypot(nx, ny, nz);
        if (!std::isfinite(magnitude) || magnitude <= 0)
            throw std::invalid_argument("Unrepresentable understory normal");
        return {nx/magnitude, ny/magnitude, nz/magnitude};
    }
}
