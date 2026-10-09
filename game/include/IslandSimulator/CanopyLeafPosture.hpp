#pragma once
#include "IslandSimulator/CanopyProfile.hpp"

#include <array>
#include <cmath>
#include <cstdint>
#include <stdexcept>

namespace island
{
    inline constexpr int CanopySprigCount = 36;

    struct CanopyLeafPosture
    {
        float pitch, roll, lengthScale, widthScale;
        bool operator==(const CanopyLeafPosture&) const = default;
    };

    inline bool RetainCanopySprig(int identity, bool far)
    {
        if (identity < 0 || identity >= CanopySprigCount)
            throw std::invalid_argument("Invalid canopy sprig identity");
        return far ? identity%9 == 3 : identity%3 != 2;
    }

    inline int CanopySprigLeafCount(int cluster, int identity)
    {
        if (cluster < 0 || cluster >= 8 || identity < 0 || identity >= CanopySprigCount)
            throw std::invalid_argument("Invalid canopy sprig membership");
        // Four paired shoots offset the perimeter blades' added outline triangles in each tier.
        return cluster%2 == 0 && identity == 30 ? 2 : 3;
    }

    // Identities 36/37 are the existing perimeter/lower blades. Pose depends on source identity,
    // never on the number of leaves retained in the chosen LOD.
    inline CanopyLeafPosture MakeCanopyLeafPosture(int variant, int cluster, int identity, int member)
    {
        if (variant < 0 || variant >= CanopyFormCount || cluster < 0 || cluster >= 8
            || identity < 0 || identity > CanopySprigCount+1 || member < 0 || member >= 3)
            throw std::invalid_argument("Invalid canopy leaf identity");
        const float shellY = identity < CanopySprigCount
            ? -1.0f + 2.0f*(identity+0.5f)/CanopySprigCount
            : (identity == CanopySprigCount ? -0.15f : -0.8f);
        const float phase = identity*2.39996323f+cluster*1.71f+variant*0.93f+member*2.17f;
        constexpr std::array<float,CanopyFormCount> pitchBias{-0.02f,0.12f,-0.10f,0.04f,-0.08f,0.10f,-0.10f,0.07f};
        constexpr std::array<float,CanopyFormCount> lengths{0.95f,1.04f,1.08f,0.90f,0.94f,1.01f,1.08f,0.92f};
        constexpr std::array<float,CanopyFormCount> widths{1.12f,0.85f,0.97f,1.16f,1.10f,0.86f,0.97f,1.10f};
        return {shellY*0.54f+std::sin(phase)*0.20f+pitchBias[variant],
                std::sin(phase*1.37f+member*0.7f)*0.52f,
                lengths[variant]*(0.94f+0.05f*(1+std::cos(phase))),
                widths[variant]*(0.92f+0.05f*(1+std::sin(phase*1.9f)))};
    }

    struct PointedLeafFrame
    {
        std::array<float,3> axis, side, normal;
    };

    inline PointedLeafFrame MakePointedLeafFrame(float yaw, float pitch, float roll)
    {
        if (!std::isfinite(yaw) || !std::isfinite(pitch) || !std::isfinite(roll)
            || std::abs(pitch) > 1.2f || std::abs(roll) > 1.2f)
            throw std::invalid_argument("Invalid pointed leaf orientation");
        const float c = std::cos(yaw), s = std::sin(yaw);
        const float cp = std::cos(pitch), sp = std::sin(pitch);
        const float cr = std::cos(roll), sr = std::sin(roll);
        const std::array<float,3> side{-s,0,c}, up{-c*sp,cp,-s*sp};
        PointedLeafFrame frame{{c*cp,sp,s*cp},{},{}};
        for (int i = 0; i < 3; ++i) {
            frame.side[i] = side[i]*cr+up[i]*sr;
            frame.normal[i] = up[i]*cr-side[i]*sr;
        }
        return frame;
    }

    // Root-relative five-vertex folded blade. Gravity droop remains world-down, while the
    // shoulder width and crease follow the pitched/rolled leaf frame. Zero posture preserves
    // legacy pointed leaves used by bushes and lianas.
    inline std::array<std::array<float,3>,5> PointedLeafPositions(
        float yaw, float length, float width, float droop, float ridge,
        float pitch = 0, float roll = 0)
    {
        for (float value : {length,width,droop,ridge})
            if (!std::isfinite(value)) throw std::invalid_argument("Non-finite pointed leaf size");
        if (length <= 0 || width <= 0 || droop < 0 || ridge < 0)
            throw std::invalid_argument("Invalid pointed leaf size");
        const auto frame = MakePointedLeafFrame(yaw,pitch,roll);
        std::array<std::array<float,3>,5> vertices{};
        for (int i = 0; i < 3; ++i) {
            const float shoulder = frame.axis[i]*(length*0.48f);
            vertices[1][i] = shoulder-frame.side[i]*width;
            vertices[2][i] = shoulder+frame.normal[i]*ridge-(i == 1 ? droop*0.18f : 0);
            vertices[3][i] = shoulder+frame.side[i]*width;
            vertices[4][i] = frame.axis[i]*length-(i == 1 ? droop : 0);
        }
        for (const auto& point : vertices)
            for (float value : point)
                if (!std::isfinite(value)) throw std::invalid_argument("Pointed leaf geometry overflow");
        return vertices;
    }

    inline constexpr std::array<std::uint16_t,18> CanopyBladeIndices{
        0,1,2, 1,4,2, 4,6,2, 6,5,2, 5,3,2, 3,0,2};
    inline constexpr std::array<std::array<float,2>,7> CanopyBladeCoordinates{{
        {0,0},{0.30f,-1},{0.48f,0},{0.30f,1},{0.72f,-1},{0.72f,1},{1,0}}};

    inline std::array<std::array<float,3>,7> CanopyBladePositions(
        float yaw, float length, float width, float droop, float ridge, float pitch, float roll)
    {
        // Reuse the legacy validator/root/crest/tip, not its diamond outline.
        const auto legacy = PointedLeafPositions(yaw,length,width,droop,ridge,pitch,roll);
        const auto frame = MakePointedLeafFrame(yaw,pitch,roll);
        std::array<std::array<float,3>,7> vertices{};
        vertices[2] = legacy[2]; vertices[6] = legacy[4];
        for (int index : {1,3,4,5}) {
            const float t = CanopyBladeCoordinates[index][0];
            const float side = CanopyBladeCoordinates[index][1];
            const float widthFactor = index <= 3 ? 0.90f : 0.82f;
            const float lift = index <= 3 ? 0.35f : 0.30f;
            for (int axis = 0; axis < 3; ++axis) {
                vertices[index][axis] = frame.axis[axis]*(length*t)
                    +frame.side[axis]*(width*widthFactor*side)+frame.normal[axis]*(ridge*lift)
                    -(axis == 1 ? droop*t*t : 0);
            }
        }
        for (const auto& point : vertices)
            for (float value : point)
                if (!std::isfinite(value)) throw std::invalid_argument("Canopy blade geometry overflow");
        return vertices;
    }
}
