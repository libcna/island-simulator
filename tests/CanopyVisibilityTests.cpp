#include "IslandSimulator/CanopyVisibility.hpp"
#include "IslandSimulator/JungleWind.hpp"
#include "Microsoft/Xna/Framework/BoundingFrustum.hpp"
#include "Microsoft/Xna/Framework/ContainmentType.hpp"
#include "Microsoft/Xna/Framework/Matrix.hpp"

#include <array>
#include <iostream>
#include <limits>
#include <numbers>

namespace
{
    using namespace Microsoft::Xna::Framework;
    struct Comma final : std::numpunct<char>
    {
        char do_decimal_point() const override { return ','; }
    };

    void Require(bool condition, const char* message)
    {
        if (!condition) throw std::runtime_error(message);
    }

    bool InClip(const Vector3& p, const Matrix& matrix)
    {
        // Independent homogeneous test of the shader's pre-OpenGL depth conversion (0..w).
        const double x = p.X * double(matrix.M11) + p.Y * double(matrix.M21)
            + p.Z * double(matrix.M31) + matrix.M41;
        const double y = p.X * double(matrix.M12) + p.Y * double(matrix.M22)
            + p.Z * double(matrix.M32) + matrix.M42;
        const double z = p.X * double(matrix.M13) + p.Y * double(matrix.M23)
            + p.Z * double(matrix.M33) + matrix.M43;
        const double w = p.X * double(matrix.M14) + p.Y * double(matrix.M24)
            + p.Z * double(matrix.M34) + matrix.M44;
        return w > 0 && x >= -w && x <= w && y >= -w && y <= w && z >= 0 && z <= w;
    }
}

int main()
try
{
    using namespace Microsoft::Xna::Framework;
    using island::CanopyInstanceBounds;
    constexpr float horizontalWindMargin = island::JungleWind::HorizontalMargin;
    constexpr float verticalWindMargin = island::JungleWind::VerticalMargin;
    // VertexBody has independent unit sine terms, weights in [0,1], then uniform scale/yaw.
    constexpr double maximumSway = double(island::JungleWind::Broad) + island::JungleWind::Detail;
    constexpr double windZ = island::JungleWind::DirectionZ;
    constexpr double lowering = island::JungleWind::Lowering;
    Require(std::hypot(maximumSway, maximumSway * windZ) < horizontalWindMargin
        && maximumSway * lowering < verticalWindMargin, "Local margin must cover shader motion");
    const auto shader = island::JungleWind::ShaderConstants();
    const auto previousLocale = std::locale();
    std::locale::global(std::locale(previousLocale, new Comma));
    const auto localizedShader = island::JungleWind::ShaderConstants();
    std::locale::global(previousLocale);
    Require(shader == localizedShader, "Shader constants must be independent of decimal locale");
    std::istringstream input(shader);
    input.imbue(std::locale::classic());
    constexpr std::array values{island::JungleWind::Broad, island::JungleWind::Detail,
                               island::JungleWind::DirectionZ, island::JungleWind::Lowering};
    constexpr std::array names{"jungleBroadWind", "jungleDetailWind", "jungleWindZ", "jungleWindLowering"};
    for (std::size_t i = 0; i < values.size(); ++i) {
        std::string qualifier, type, name, equal;
        float value = 0;
        char terminator = 0;
        input >> qualifier >> type >> name >> equal >> value >> terminator;
        Require(input.good() && qualifier == "const" && type == "float" && name == names[i]
            && equal == "=" && value == values[i] && terminator == ';',
            "CPU wind values must round-trip through valid GLSL literals");
    }
    std::size_t sampled = 0, visible = 0, rejected = 0;
    for (float cameraYaw : {-1.5f, -0.7f, 0.0f, 0.8f, 1.6f}) {
        const Vector3 camera(5, 3, 8);
        const auto matrix = Matrix::CreateLookAt(camera,
            camera + Vector3(std::sin(cameraYaw), -0.1f, -std::cos(cameraYaw)), Vector3::Up)
            * Matrix::CreatePerspectiveFieldOfView(1.1f, 16.0f/9.0f, 0.1f, 250.0f);
        const BoundingFrustum frustum(matrix);
        for (float x : {-60.0f, -20.0f, 0.0f, 20.0f, 60.0f}) {
            for (float z : {-100.0f, -40.0f, -5.0f, 25.0f}) {
                for (float scale : {0.78f, 1.0f, 1.46f}) {
                    const Vector3 anchor(x, 2, z);
                    const auto box = CanopyInstanceBounds(6 + horizontalWindMargin,
                        -verticalWindMargin, 17 + verticalWindMargin, anchor, scale);
                    const bool outside = frustum.Contains(box) == ContainmentType::Disjoint;
                    rejected += outside;
                    for (float yaw : {0.0f, 0.63f, 2.1f, 4.4f}) {
                        for (int angle = 0; angle < 16; ++angle) {
                            const double a = angle * std::numbers::pi / 8;
                            for (double height : {0.0, 8.5, 17.0}) {
                                for (double wave : {-1.0, 0.0, 1.0}) {
                                    const double sway = wave * maximumSway;
                                    const double lx = std::cos(a) * 6 + sway;
                                    const double lz = std::sin(a) * 6 + sway * windZ;
                                    const Vector3 p(
                                        static_cast<float>(anchor.X + (lx*std::cos(yaw)+lz*std::sin(yaw))*scale),
                                        static_cast<float>(anchor.Y + (height-std::abs(sway)*lowering)*scale),
                                        static_cast<float>(anchor.Z + (-lx*std::sin(yaw)+lz*std::cos(yaw))*scale));
                                    Require(p.X >= box.Min.X && p.X <= box.Max.X
                                        && p.Y >= box.Min.Y && p.Y <= box.Max.Y
                                        && p.Z >= box.Min.Z && p.Z <= box.Max.Z,
                                        "Transformed animated vertex escaped instance bounds");
                                    if (InClip(p, matrix)) {
                                        ++visible;
                                        Require(!outside, "Frustum rejected an instance with an on-screen vertex");
                                    }
                                    ++sampled;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    Require(visible > 1000 && rejected > 50, "Visibility sampling must exercise both outcomes");
    const BoundingFrustum forward(Matrix::CreatePerspectiveFieldOfView(
        std::numbers::pi_v<float>/2, 1, 0.1f, 100));
    Require(forward.Contains(CanopyInstanceBounds(2, -1, 1, {20,0,-10}, 1))
        == ContainmentType::Disjoint, "Definitely off-screen box should be rejected");
    Require(forward.Contains(CanopyInstanceBounds(2, -1, 1, {11,0,-10}, 1))
        != ContainmentType::Disjoint, "Keep intersecting crown when its trunk is off-screen");
    Require(forward.Contains(CanopyInstanceBounds(2, -1, 1, {0,0,1}, 1))
        != ContainmentType::Disjoint, "Keep a crown crossing the eye and near planes");
    for (int invalid = 0; invalid < 7; ++invalid) {
        float radius = 6, minimum = 0, maximum = 17, scale = 1;
        Vector3 anchor(0,0,0);
        if (invalid == 0) radius = -1;
        if (invalid == 1) minimum = 18;
        if (invalid == 2) scale = 0;
        if (invalid == 3) maximum = std::numeric_limits<float>::quiet_NaN();
        if (invalid == 4) anchor.Z = std::numeric_limits<float>::infinity();
        if (invalid == 5) radius = scale = std::numeric_limits<float>::max();
        if (invalid == 6) scale = -1;
        bool refused = false;
        try { (void)CanopyInstanceBounds(radius, minimum, maximum, anchor, scale); }
        catch (const std::invalid_argument&) { refused = true; }
        Require(refused, "Invalid instance envelope was accepted");
    }
    std::cout << "Canopy visibility tests passed: " << sampled << " transformed vertices, "
              << visible << " clip-visible, " << rejected << " rejected boxes; no graphics context\n";
    return 0;
}
catch (const std::exception& error)
{
    std::cerr << error.what() << '\n';
    return 1;
}
