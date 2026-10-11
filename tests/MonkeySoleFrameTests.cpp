#include "IslandSimulator/MonkeySoleFrame.hpp"
#include "IslandSimulator/MonkeyGroundGait.hpp"
#include <iostream>
#include <stdexcept>

int main()
try {
    using Microsoft::Xna::Framework::Vector3;
    for (float slopeX : {-0.6f, 0.0f, 0.6f}) {
        for (float slopeZ : {-0.6f, 0.0f, 0.6f}) {
            const auto normal = Vector3::Normalize(Vector3(slopeX, 1, slopeZ));
            const Vector3 rest(.18f, .035f, .28f);
            const Vector3 target(0, island::MonkeyGroundGait::SoleHeight / normal.Y, 0);
            const auto skin = island::MonkeySoleSkin(rest, target, normal);
            const auto moved = Vector3::Transform(rest, skin);
            const auto up = Vector3::TransformNormal(Vector3(0,1,0), skin);
            const auto right = Vector3::TransformNormal(Vector3(1,0,0), skin);
            const auto forward = Vector3::TransformNormal(Vector3(0,0,1), skin);
            if (Vector3::Distance(moved,target) > 1e-5f || Vector3::Distance(up,normal) > 1e-5f
                || std::fabs(Vector3::Dot(right,up)) > 1e-5f
                || std::fabs(Vector3::Dot(forward,up)) > 1e-5f
                || std::fabs(right.Length()-1) > 1e-5f || std::fabs(forward.Length()-1) > 1e-5f)
                throw std::runtime_error("sole frame must be rigid, grounded and normal-aligned");
            // Conservative underside corners, including the distal digit extent.
            for (float x : {-0.048f, 0.0f, 0.048f})
                for (float z : {-0.049f, 0.027f, 0.138f}) {
                    const auto underside = Vector3::Transform(rest + Vector3(x,-.031f,z),skin);
                    if (std::fabs(Vector3::Dot(underside, normal)-.004f) > 1e-5f)
                        throw std::runtime_error("sole underside loses slope clearance");
                }
        }
    }
    std::cout << "Monkey sole frames: rigid contact, ground alignment and clearance pass\n";
    return 0;
} catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
}
