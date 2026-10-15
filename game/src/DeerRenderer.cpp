#include "IslandSimulator/DeerRenderer.hpp"

#include "IslandSimulator/CanopyLightField.hpp"
#include "IslandSimulator/EnvironmentLighting.hpp"
#include "IslandSimulator/RuntimeDiagnostics.hpp"
#include "Microsoft/Xna/Framework/BoundingFrustum.hpp"
#include "Microsoft/Xna/Framework/BoundingSphere.hpp"
#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/Graphics/BasicEffect.hpp"
#include "Microsoft/Xna/Framework/Graphics/BlendState.hpp"
#include "Microsoft/Xna/Framework/Graphics/BufferUsage.hpp"
#include "Microsoft/Xna/Framework/Graphics/DepthStencilState.hpp"
#include "Microsoft/Xna/Framework/Graphics/DynamicVertexBuffer.hpp"
#include "Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp"
#include "Microsoft/Xna/Framework/Graphics/PrimitiveType.hpp"
#include "Microsoft/Xna/Framework/Graphics/RasterizerState.hpp"
#include "Microsoft/Xna/Framework/Graphics/SetDataOptions.hpp"
#include "Microsoft/Xna/Framework/Graphics/VertexPositionColor.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <stdexcept>

using namespace Microsoft::Xna::Framework;
using namespace Microsoft::Xna::Framework::Graphics;

namespace island
{
    namespace
    {
        constexpr int MaximumVerticesPerDeer = 5800;
        constexpr int Capacity = static_cast<int>(DeerHerd::Count) * MaximumVerticesPerDeer;

        void Triangle(std::vector<VertexPositionColor>& out, const Vector3& a,
                      const Vector3& b, const Vector3& c, const Color color)
        {
            out.emplace_back(a, color);out.emplace_back(b, color);out.emplace_back(c, color);
        }

        Color Lit(const Color base, const Vector3& normal, const float transmission)
        {
            const Vector3 sun = Vector3::Normalize(Vector3(.46f, .83f, -.31f));
            const float light = (.55f + .40f * std::max(0.0f, Vector3::Dot(normal, sun)))
                * (.72f + .28f * transmission);
            return Color(static_cast<int>(std::clamp(
                             std::lround(base.getRProperty() * light), 0L, 255L)),
                         static_cast<int>(std::clamp(
                             std::lround(base.getGProperty() * light), 0L, 255L)),
                         static_cast<int>(std::clamp(
                             std::lround(base.getBProperty() * light), 0L, 255L)), 255);
        }

        void Ellipsoid(std::vector<VertexPositionColor>& out, const Vector3& centre,
                       const Vector3& right, const Vector3& up, const Vector3& forward,
                       const float rx, const float ry, const float rz, const Color color,
                       const float transmission, const int rings = 5, const int sides = 10)
        {
            const auto point = [&](const int ring, const int side) {
                const float latitude = -1.57079632679f
                    + 3.14159265359f * static_cast<float>(ring) / static_cast<float>(rings);
                const float longitude = 6.28318530718f * static_cast<float>(side)
                    / static_cast<float>(sides);
                return centre + right * (std::cos(latitude) * std::cos(longitude) * rx)
                    + up * (std::sin(latitude) * ry)
                    + forward * (std::cos(latitude) * std::sin(longitude) * rz);
            };
            for (int ring = 0; ring < rings; ++ring) for (int side = 0; side < sides; ++side) {
                const int next = (side + 1) % sides;
                const Vector3 a = point(ring, side), b = point(ring + 1, side);
                const Vector3 c = point(ring + 1, next), d = point(ring, next);
                const auto normal = [&](const int latitudeRing, const int longitudeSide) {
                    const float latitude = -1.57079632679f
                        + 3.14159265359f * static_cast<float>(latitudeRing)
                            / static_cast<float>(rings);
                    const float longitude = 6.28318530718f
                        * static_cast<float>(longitudeSide) / static_cast<float>(sides);
                    Vector3 result = right * (std::cos(latitude) * std::cos(longitude) / rx)
                        + up * (std::sin(latitude) / ry)
                        + forward * (std::cos(latitude) * std::sin(longitude) / rz);
                    if (result.LengthSquared() > 1e-8f) result.Normalize(); else result = up;
                    return result;
                };
                const Color ca=Lit(color,normal(ring,side),transmission);
                const Color cb=Lit(color,normal(ring+1,side),transmission);
                const Color cc=Lit(color,normal(ring+1,next),transmission);
                const Color cd=Lit(color,normal(ring,next),transmission);
                out.emplace_back(a,ca);out.emplace_back(b,cb);out.emplace_back(c,cc);
                out.emplace_back(a,ca);out.emplace_back(c,cc);out.emplace_back(d,cd);
            }
        }

        void Tube(std::vector<VertexPositionColor>& out, const Vector3& a, const Vector3& b,
                  const float radiusA, const float radiusB, const Color color,
                  const float transmission, const int sides = 7)
        {
            Vector3 axis = b - a;
            const float length = axis.Length();
            if (length < .0001f) return;
            axis /= length;
            Vector3 side = Vector3::Cross(axis, std::abs(axis.Y) < .9f
                ? Vector3::Up : Vector3::Right);
            side.Normalize();
            const Vector3 other = Vector3::Cross(axis, side);
            for (int index = 0; index < sides; ++index) {
                const float first = 6.28318530718f * static_cast<float>(index) / sides;
                const float second = 6.28318530718f * static_cast<float>(index + 1) / sides;
                const Vector3 radialA = side * std::cos(first) + other * std::sin(first);
                const Vector3 radialB = side * std::cos(second) + other * std::sin(second);
                const Color shadeA = Lit(color, radialA, transmission);
                const Color shadeB = Lit(color, radialB, transmission);
                const Vector3 p0 = a + radialA * radiusA, p1 = b + radialA * radiusB;
                const Vector3 p2 = b + radialB * radiusB, p3 = a + radialB * radiusA;
                out.emplace_back(p0,shadeA);out.emplace_back(p1,shadeA);out.emplace_back(p2,shadeB);
                out.emplace_back(p0,shadeA);out.emplace_back(p2,shadeB);out.emplace_back(p3,shadeB);
            }
        }

        void AppendDeer(std::vector<VertexPositionColor>& out, const DeerState& deer,
                        const float transmission, const bool near)
        {
            const std::array<Color, 3> coats = {{Color(151, 103, 58), Color(130, 82, 48),
                                                 Color(170, 121, 69)}};
            const Color coat = coats[deer.coatVariant % coats.size()];
            const Color dark = Color::Lerp(coat, Color(42, 31, 24), .58f);
            const Color pale = Color::Lerp(coat, Color(231, 214, 177), .68f);
            const Color eye(24, 17, 11), antler(103, 82, 57), hoof(31, 27, 24);
            const float scale = deer.scale;
            const Vector3 forward(std::cos(deer.headingRadians), 0.0f,
                                  std::sin(deer.headingRadians));
            const Vector3 right(-forward.Z, 0.0f, forward.X), up = Vector3::Up;
            const Vector3 body(deer.x, deer.y + 1.03f * scale, deer.z);
            Ellipsoid(out, body, right, up, forward, .33f * scale, .37f * scale,
                      .76f * scale, coat, transmission, near ? 10 : 4, near ? 20 : 8);
            // A higher shoulder and slightly rounder rump avoid the single barrel primitive
            // while keeping the complete silhouette inexpensive and contiguous.
            Ellipsoid(out, body + forward * (.43f * scale) + up * (.08f * scale),
                      right, up, forward, .34f * scale, .39f * scale, .37f * scale,
                      Color::Lerp(coat, dark, .06f), transmission, near ? 8 : 3,
                      near ? 16 : 7);
            Ellipsoid(out, body - forward * (.43f * scale) + up * (.015f * scale),
                      right, up, forward, .35f * scale, .39f * scale, .40f * scale,
                      coat, transmission, near ? 8 : 3, near ? 16 : 7);
            const float graze = deer.grazeAmount;
            const Vector3 neckBase = body + forward * (.62f * scale) + up * (.20f * scale);
            const Vector3 neckTop = neckBase + forward * (.30f * scale)
                + up * ((.57f - .68f * graze) * scale);
            const Vector3 neckMid = neckBase + forward * (.10f * scale)
                + up * ((.31f - .34f * graze) * scale);
            Tube(out, neckBase, neckMid, .25f * scale, .22f * scale, coat, transmission,
                 near ? 9 : 6);
            Tube(out, neckMid, neckTop, .22f * scale, .19f * scale, coat, transmission,
                 near ? 9 : 6);
            const Vector3 head = neckTop + forward * (.25f * scale)
                + up * ((.06f - .12f * graze) * scale);
            Ellipsoid(out, head, right, up, forward, .18f * scale, .22f * scale,
                      .34f * scale, coat, transmission, near ? 7 : 3, near ? 16 : 7);
            const Vector3 muzzle = head + forward * (.29f * scale) - up * (.055f * scale);
            Ellipsoid(out, muzzle, right, up, forward, .13f * scale, .12f * scale,
                      .19f * scale, dark, transmission, near ? 6 : 2, near ? 12 : 6);
            // Long pointed ears establish a deer rather than goat silhouette.
            for (const float sideSign : {-1.0f, 1.0f}) {
                const Vector3 earRoot = head - forward * (.06f * scale)
                    + right * (sideSign * .13f * scale) + up * (.14f * scale);
                const Vector3 earTip = earRoot - forward * (.09f * scale)
                    + right * (sideSign * .19f * scale) + up * (.24f * scale);
                Triangle(out, earRoot - right * (sideSign * .055f * scale), earTip,
                         earRoot + forward * (.12f * scale), Lit(dark, up, transmission));
                Triangle(out, earRoot + forward * (.12f * scale), earTip,
                         earRoot - right * (sideSign * .055f * scale), Lit(pale, up, transmission));
                if (near) {
                    const Vector3 eyeCentre = head + forward * (.14f * scale)
                        + right * (sideSign * .17f * scale) + up * (.035f * scale);
                    Ellipsoid(out, eyeCentre, right, up, forward, .025f * scale,
                              .030f * scale, .018f * scale, eye, transmission, 4, 10);
                }
            }
            // Four articulated legs; diagonal phase offsets make walk and retreat readable.
            for (int leg = 0; leg < 4; ++leg) {
                const float fore = leg < 2 ? .48f : -.47f;
                const float sideSign = leg % 2 == 0 ? -1.0f : 1.0f;
                const float phase = deer.gaitPhase + ((leg == 0 || leg == 3) ? 0.0f : 3.14159265359f);
                const float swing = std::sin(phase) * std::min(1.0f, deer.speed / .8f);
                const Vector3 hip = body + forward * (fore * scale)
                    + right * (sideSign * .255f * scale) - up * (.23f * scale);
                const bool hind = leg >= 2;
                const Vector3 knee = hip + forward * (((hind ? .16f : -.07f)
                    + swing * .13f) * scale) - up * (.37f * scale)
                    + right * (sideSign * .018f * scale);
                const Vector3 hock = knee + forward * (((hind ? -.22f : .08f)
                    - swing * .15f) * scale) - up * (.34f * scale);
                const Vector3 hoofPoint = hock + forward * (((hind ? .18f : .04f)
                    - swing * .08f) * scale);
                Tube(out, hip, knee, .082f * scale, .057f * scale, coat, transmission,
                     near ? 9 : 7);
                Tube(out, knee, hock, .054f * scale, .038f * scale, dark, transmission,
                     near ? 9 : 7);
                Tube(out, hock, Vector3(hoofPoint.X, deer.y + .060f * scale, hoofPoint.Z),
                     .037f * scale, .028f * scale, dark, transmission, near ? 9 : 7);
                const Vector3 groundedHoof(hoofPoint.X, deer.y + .060f * scale, hoofPoint.Z);
                Tube(out, groundedHoof - forward * (.045f * scale),
                     groundedHoof + forward * (.075f * scale), .038f * scale,
                     .028f * scale, hoof, transmission, near ? 8 : 6);
            }
            const Vector3 tailRoot = body - forward * (.77f * scale) + up * (.17f * scale);
            const Vector3 tailTip = tailRoot - forward * (.31f * scale) + up * (.08f * scale);
            Tube(out, tailRoot, tailTip, .105f * scale, .025f * scale, pale, transmission,
                 near ? 10 : 8);
            if (near) {
                // A restrained row of flank spots breaks the flat coat without a texture draw.
                for (int spot = 0; spot < 5; ++spot) for (const float sideSign : {-1.0f, 1.0f}) {
                    const Vector3 c = body + forward * ((-.43f + .20f * spot) * scale)
                        + right * (sideSign * .395f * scale) + up * (.12f * scale);
                    const Vector3 a = c + up * (.035f * scale);
                    const Vector3 b = c - up * (.025f * scale) - forward * (.030f * scale);
                    const Vector3 d = c - up * (.025f * scale) + forward * (.030f * scale);
                    Triangle(out, a, b, d, Lit(pale, right * sideSign, transmission));
                    Triangle(out, d, b, a, Lit(pale, right * -sideSign, transmission));
                }
            }
            if (near && deer.antlered) {
                for (const float sideSign : {-1.0f, 1.0f}) {
                    const Vector3 base = head - forward * (.12f * scale)
                        + right * (sideSign * .075f * scale) + up * (.18f * scale);
                    const Vector3 beam = base - forward * (.07f * scale)
                        + right * (sideSign * .07f * scale) + up * (.39f * scale);
                    const Vector3 crown = beam + forward * (.22f * scale) + up * (.19f * scale);
                    Tube(out, base, beam, .026f * scale, .018f * scale, antler, transmission, 8);
                    Tube(out, beam, crown, .019f * scale, .010f * scale, antler, transmission, 8);
                    Tube(out, beam + forward * (.05f * scale),
                         beam + forward * (.18f * scale) + up * (.10f * scale),
                         .015f * scale, .008f * scale, antler, transmission, 8);
                    Tube(out, crown - forward * (.06f * scale),
                         crown - forward * (.02f * scale) + up * (.16f * scale),
                         .012f * scale, .006f * scale, antler, transmission, 8);
                }
            }
        }
    }

    DeerRenderer::DeerRenderer(GraphicsDevice& device, const CanopyLightField& canopy,
                               const float maximumDistance)
        : device_(device), canopy_(canopy), maximumDistance_(maximumDistance),
          effect_(std::make_unique<BasicEffect>(device)),
          buffer_(std::make_unique<DynamicVertexBuffer>(device,
              VertexPositionColor::getVertexDeclarationStatic(), Capacity, BufferUsage::WriteOnly))
    {
        staging_.reserve(Capacity);
        effect_->setVertexColorEnabledProperty(true);effect_->setLightingEnabledProperty(false);
        effect_->setFogEnabledProperty(true);
        effect_->setFogColorProperty(Vector3(environment::FogColorRed,
            environment::FogColorGreen, environment::FogColorBlue));
        effect_->setFogStartProperty(maximumDistance_ * .70f);
        effect_->setFogEndProperty(maximumDistance_);
        constexpr auto diagnostic =
            "island-simulator: four inland deer use long legs, pointed ears, spotted coats and one antlered silhouette";
        std::cout << diagnostic << '\n';MirrorRuntimeDiagnosticToPlatformLog(diagnostic);
        constexpr auto anatomyDiagnostic =
            "island-simulator: near deer use vertex-blended ellipsoid light, curved tapered necks, articulated hocks, fitted muzzles, and smooth antler beams";
        std::cout << anatomyDiagnostic << '\n';MirrorRuntimeDiagnosticToPlatformLog(anatomyDiagnostic);
    }

    DeerRenderer::~DeerRenderer() = default;

    void DeerRenderer::Draw(const Matrix& view, const Matrix& projection,
                            const Vector3& camera, const std::span<const DeerState> deer)
    {
        if (deer.size() > DeerHerd::Count) throw std::length_error("Deer render capacity exceeded");
        staging_.clear();visibleCount_ = drawCallCount_ = 0;
        const BoundingFrustum frustum(view * projection);
        for (const DeerState& animal : deer) {
            const Vector3 centre(animal.x, animal.y + 1.1f * animal.scale, animal.z);
            const float distanceSquared = Vector3::DistanceSquared(centre, camera);
            if (distanceSquared > maximumDistance_ * maximumDistance_
                || !frustum.Intersects(BoundingSphere(centre, 2.1f * animal.scale))) continue;
            const float transmission = canopy_.SampleReceiver(
                animal.x, animal.y + 1.0f, animal.z, animal.y);
            AppendDeer(staging_, animal, transmission, distanceSquared < 48.0f * 48.0f);
            ++visibleCount_;
        }
        if (staging_.empty()) return;
        if (staging_.size() > static_cast<std::size_t>(Capacity))
            throw std::length_error("Deer vertex upload exceeds capacity");
        device_.setRasterizerStateProperty(RasterizerState::CullNone);
        device_.setBlendStateProperty(BlendState::Opaque);
        device_.setDepthStencilStateProperty(DepthStencilState::Default);
        buffer_->SetData(staging_.data(), 0, static_cast<int>(staging_.size()),
                         SetDataOptions::Discard);
        device_.SetVertexBuffer(buffer_.get());device_.setIndicesProperty(nullptr);
        effect_->setWorldProperty(Matrix::getIdentityProperty());effect_->setViewProperty(view);
        effect_->setProjectionProperty(projection);effect_->Apply();
        device_.DrawPrimitives(PrimitiveType::TriangleList, 0,
                               static_cast<int>(staging_.size() / 3U));
        drawCallCount_ = 1;
    }
}
