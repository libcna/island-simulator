#pragma once

#include "Microsoft/Xna/Framework/Matrix.hpp"
#include "Microsoft/Xna/Framework/Vector3.hpp"

namespace island
{
    // Row-vector skin transform: rest-space ankle -> independently oriented palm/sole.
    // The sole forward direction follows the body's heading projected onto the ground.
    inline Microsoft::Xna::Framework::Matrix MonkeySoleSkin(
        const Microsoft::Xna::Framework::Vector3& rest,
        const Microsoft::Xna::Framework::Vector3& target,
        const Microsoft::Xna::Framework::Vector3& normal)
    {
        using namespace Microsoft::Xna::Framework;
        const Vector3 up = Vector3::Normalize(normal);
        const Vector3 forward = Vector3::Normalize(Vector3(0, 0, 1) - up * up.Z);
        const Vector3 right = Vector3::Normalize(Vector3::Cross(up, forward));
        Matrix frame = Matrix::getIdentityProperty();
        frame.M11 = right.X; frame.M12 = right.Y; frame.M13 = right.Z;
        frame.M21 = up.X; frame.M22 = up.Y; frame.M23 = up.Z;
        frame.M31 = forward.X; frame.M32 = forward.Y; frame.M33 = forward.Z;
        return Matrix::CreateTranslation(rest * -1.0f) * frame * Matrix::CreateTranslation(target);
    }
}
