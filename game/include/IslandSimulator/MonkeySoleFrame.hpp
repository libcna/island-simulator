#pragma once

#include <algorithm>
#include <cmath>

#include "Microsoft/Xna/Framework/Matrix.hpp"
#include "Microsoft/Xna/Framework/Vector3.hpp"

namespace island
{
    // A rigid shortest-arc segment rotation, including parallel/opposite directions.
    inline Microsoft::Xna::Framework::Matrix MonkeySegmentRotation(
        const Microsoft::Xna::Framework::Vector3& restDirection,
        const Microsoft::Xna::Framework::Vector3& targetDirection)
    {
        using namespace Microsoft::Xna::Framework;
        const auto from = Vector3::Normalize(restDirection), to = Vector3::Normalize(targetDirection);
        const float cosine = std::clamp(Vector3::Dot(from,to),-1.0f,1.0f);
        Vector3 axis = Vector3::Cross(from,to);
        const float sine = axis.Length();
        if (sine < 1e-7f) {
            if (cosine > 0) return Matrix::getIdentityProperty();
            axis = Vector3::Cross(from,std::abs(from.X) < .8f ? Vector3(1,0,0) : Vector3(0,0,1));
        }
        axis.Normalize();
        return Matrix::CreateFromAxisAngle(axis,std::atan2(sine,cosine));
    }
    // Row-vector skin transform: rest-space ankle -> independently oriented palm/sole.
    // The sole forward direction follows the body's heading projected onto the ground.
    inline Microsoft::Xna::Framework::Matrix MonkeySoleSkin(
        const Microsoft::Xna::Framework::Vector3& rest,
        const Microsoft::Xna::Framework::Vector3& target,
        const Microsoft::Xna::Framework::Vector3& normal)
    {
        using namespace Microsoft::Xna::Framework;
        const Vector3 up = Vector3::Normalize(normal);
        Vector3 forward = Vector3(0, 0, 1) - up * up.Z;
        if (forward.LengthSquared() < 1e-8f) forward = Vector3(0,1,0)-up*up.Y;
        forward.Normalize();
        const Vector3 right = Vector3::Normalize(Vector3::Cross(up, forward));
        Matrix frame = Matrix::getIdentityProperty();
        frame.M11 = right.X; frame.M12 = right.Y; frame.M13 = right.Z;
        frame.M21 = up.X; frame.M22 = up.Y; frame.M23 = up.Z;
        frame.M31 = forward.X; frame.M32 = forward.Y; frame.M33 = forward.Z;
        return Matrix::CreateTranslation(rest * -1.0f) * frame * Matrix::CreateTranslation(target);
    }
}
