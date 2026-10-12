#include "IslandSimulator/EnvironmentLighting.hpp"
#include "IslandSimulator/BoarRenderer.hpp"
#include "IslandSimulator/BoarCoatTexture.hpp"
#include "IslandSimulator/BoarPose.hpp"
#include "IslandSimulator/BoarEarGeometry.hpp"
#include "IslandSimulator/BoarEyeGeometry.hpp"
#include "IslandSimulator/BoarMouthGeometry.hpp"
#include "IslandSimulator/BoarGroundPose.hpp"
#include "IslandSimulator/GeneratedTextureBudget.hpp"
#include "IslandSimulator/IslandTerrain.hpp"
#include "IslandSimulator/CanopyLightField.hpp"
#include "IslandSimulator/RuntimeDiagnostics.hpp"
#include <iostream>
#include "IslandSimulator/WildlifeRenderLighting.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>
#include <stdexcept>
#include "Microsoft/Xna/Framework/BoundingFrustum.hpp"
#include "Microsoft/Xna/Framework/BoundingSphere.hpp"
#include "Microsoft/Xna/Framework/Graphics/BasicEffect.hpp"
#include "Microsoft/Xna/Framework/Graphics/BlendState.hpp"
#include "Microsoft/Xna/Framework/Graphics/BufferUsage.hpp"
#include "Microsoft/Xna/Framework/Graphics/DepthStencilState.hpp"
#include "Microsoft/Xna/Framework/Graphics/DynamicVertexBuffer.hpp"
#include "Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp"
#include "Microsoft/Xna/Framework/Graphics/PrimitiveType.hpp"
#include "Microsoft/Xna/Framework/Graphics/RasterizerState.hpp"
#include "Microsoft/Xna/Framework/Graphics/SetDataOptions.hpp"
#include "Microsoft/Xna/Framework/Graphics/VertexPositionColorTexture.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"
#include "Microsoft/Xna/Framework/Graphics/SamplerState.hpp"

using namespace Microsoft::Xna::Framework;
using namespace Microsoft::Xna::Framework::Graphics;

namespace island
{
    namespace
    {
        constexpr int MaximumVertices = static_cast<int>(BoarHerd::BoarCount) * 6000;
        constexpr float Pi = std::numbers::pi_v<float>;
        const Vector2 WhiteUv(BoarCoatTexture::WhiteU, BoarCoatTexture::WhiteV);
        static_assert(BoarCoatTexture::Size == GeneratedTextureBudget::BoarCoatSize);

        struct ModelBuilder
        {
            std::vector<VertexPositionColorTexture>& vertices;
            Vector3 origin, right, up, forward;
            float scale;
            const CanopyLightField& canopyLight;

            Color CanopyColor(const Vector3& world, const Color& color) const
            {
                const auto tint = CanopyTransmissionTint(
                    canopyLight.SampleReceiver(world.X, world.Y, world.Z, origin.Y));
                return Color(static_cast<int>(std::lround(color.getRProperty() * tint.red)),
                             static_cast<int>(std::lround(color.getGProperty() * tint.green)),
                             static_cast<int>(std::lround(color.getBProperty() * tint.blue)),
                             static_cast<int>(color.getAProperty()));
            }

            Vector3 Point(const Vector3& local) const
            {
                return origin + (right * local.X + up * local.Y + forward * local.Z) * scale;
            }
            VertexPositionColorTexture SmoothVertex(const Vector3& point, const Vector3& normal,
                                             const Color& color, const Vector2& uv = WhiteUv) const
            {
                const auto worldNormal = Vector3::Normalize(
                    right * normal.X + up * normal.Y + forward * normal.Z);
                const float direct = std::max(0.0f, Vector3::Dot(
                    worldNormal, wildlife_render::FixedSunDirection));
                const float sky = std::max(0.0f, worldNormal.Y);
                const float underside = 0.83f + 0.17f * std::clamp(normal.Y + 0.55f, 0.0f, 1.0f);
                const float brightness = (0.61f + direct * 0.37f + sky * 0.10f) * underside;
                const auto world = Point(point);
                return VertexPositionColorTexture(world,
                    CanopyColor(world, wildlife_render::ScaleColor(color, brightness)), uv);
            }

            void SmoothTriangle(const VertexPositionColorTexture& a, const VertexPositionColorTexture& b,
                                const VertexPositionColorTexture& c)
            {
                if (Vector3::Cross(b.Position - a.Position, c.Position - a.Position).LengthSquared()
                    < 1.0e-12f) return;
                vertices.push_back(a);
                vertices.push_back(b);
                vertices.push_back(c);
            }
            void Triangle(const Vector3& a, const Vector3& b, const Vector3& c, const Color& color)
            {
                const auto pa = Point(a), pb = Point(b), pc = Point(c);
                const auto normal = Vector3::Cross(pb - pa, pc - pa);
                if (normal.LengthSquared() < 1.0e-12f) return;
                const auto lit = wildlife_render::FacetColor(pa, pb, pc, color);
                vertices.emplace_back(pa, CanopyColor(pa, lit), WhiteUv);
                vertices.emplace_back(pb, CanopyColor(pb, lit), WhiteUv);
                vertices.emplace_back(pc, CanopyColor(pc, lit), WhiteUv);
            }
            void Ellipsoid(const Vector3& centre, const Vector3& radius, const Color& color,
                           const int sides, const int rings)
            {
                const auto point = [&](const int latitude, const int longitude) {
                    const float a = static_cast<float>(latitude) * Pi / static_cast<float>(rings);
                    const float b = static_cast<float>(longitude) * 2.0f * Pi / static_cast<float>(sides);
                    const Vector3 unit(std::sin(a) * std::cos(b), std::cos(a),
                                       std::sin(a) * std::sin(b));
                    return SmoothVertex(centre + Vector3(radius.X * unit.X, radius.Y * unit.Y,
                                                          radius.Z * unit.Z),
                                        Vector3(unit.X / radius.X, unit.Y / radius.Y, unit.Z / radius.Z), color);
                };
                for (int row = 0; row < rings; ++row) {
                    for (int side = 0; side < sides; ++side) {
                        const auto a = point(row, side), b = point(row + 1, side);
                        const auto c = point(row + 1, side + 1), d = point(row, side + 1);
                        SmoothTriangle(a, c, b);
                        SmoothTriangle(a, d, c);
                    }
                }
            }

            void Limb(const Vector3& start, const Vector3& end, float startRadius,
                      float endRadius, const Color& color, int sides)
            {
                const Vector3 axis = Vector3::Normalize(end - start);
                const Vector3 across(1, 0, 0);
                const Vector3 around = Vector3::Normalize(Vector3::Cross(axis, across));
                const float taper = (startRadius - endRadius) / (end - start).Length();
                const auto vertex = [&](int side, bool lower) {
                    const float angle = static_cast<float>(side) * 2.0f * Pi / sides;
                    const Vector3 radial = across * std::cos(angle) + around * std::sin(angle);
                    return SmoothVertex((lower ? end : start) + radial * (lower ? endRadius : startRadius),
                                        radial + axis * taper, color);
                };
                for (int side = 0; side < sides; ++side) {
                    const auto a = vertex(side, false), b = vertex(side, true);
                    const auto c = vertex(side + 1, true), d = vertex(side + 1, false);
                    SmoothTriangle(a, b, c);
                    SmoothTriangle(a, c, d);
                }
            }

            void Body(const Color& color, const float dip, const float bob, const bool juvenile,
                      const int sides)
            {
                // One continuous neck/head/snout surface removes intersecting primitive seams.
                const auto profile = MakeBoarBodyProfile(dip);
                const auto vertex = [&](const std::size_t row, const int side) {
                    const auto& ring = profile[row];
                    const auto& before = profile[row == 0 ? 0 : row - 1];
                    const auto& after = profile[std::min(row + 1, profile.size() - 1)];
                    const float invLength = 1.0f / (after.z - before.z);
                    const float angle = static_cast<float>(side) * 2.0f * Pi / static_cast<float>(sides);
                    const float c = std::cos(angle), s = std::sin(angle);
                    const Vector3 point(c * ring.radiusX, ring.centreY + s * ring.radiusY + bob, ring.z);
                    const float nx = c / ring.radiusX, ny = s / ring.radiusY;
                    const float nz = -nx * c * (after.radiusX - before.radiusX) * invLength
                        - ny * ((after.centreY - before.centreY) + s * (after.radiusY - before.radiusY)) * invLength;
                    const float grain = 1.0f + 0.025f * std::sin(point.Z * 37.0f + point.Y * 13.0f);
                    const float dorsal = 0.90f + 0.10f * (1.0f - std::max(0.0f, s));
                    const float stripe = juvenile && ring.z < 0.45f
                        ? 1.0f + 0.16f * std::pow(std::max(0.0f, std::sin(angle * 6.0f)), 4.0f) : 1.0f;
                    const auto coat = wildlife_render::ScaleColor(color, grain * dorsal * stripe);
                    const Vector2 uv(BoarCoatTexture::BodyU((ring.z + 0.72f) / 1.78f),
                        BoarCoatTexture::BodyV(static_cast<float>(side) / sides));
                    return SmoothVertex(point, Vector3(nx, ny, nz), coat, uv);
                };
                const std::size_t rowStep=sides<16?2U:1U; // Far mesh retains authored anchor count.
                for (std::size_t row = 0; row + rowStep < profile.size(); row+=rowStep) {
                    for (int side = 0; side < sides; ++side) {
                        const auto a = vertex(row, side), b = vertex(row + rowStep, side);
                        const auto c = vertex(row + rowStep, side + 1), d = vertex(row, side + 1);
                        SmoothTriangle(a, b, c);
                        SmoothTriangle(a, c, d);
                    }
                }
            }
        };

        void AppendBoar(ModelBuilder& mesh, const BoarState& boar,
                        const BoarGroundPose& grounded, const bool near)
        {
            const std::array<Color, 3> coats = {Color(100, 76, 51), Color(79, 66, 51), Color(115, 91, 62)};
            const auto coat = coats[boar.colorVariant % coats.size()];
            const auto dark = wildlife_render::ScaleColor(coat, 0.63f);
            const int sides = near ? 16 : 6;
            const float bob = grounded.bodyBob;
            const float dip = boar.rootingPose * 0.15f;
            mesh.Body(coat, dip, bob, boar.scale < 0.8f, sides);
            mesh.Ellipsoid(Vector3(0, 0.42f - dip + bob, 1.047f), Vector3(0.114f, 0.09f, 0.025f),
                           Color(51, 44, 39), near ? 12 : 6, 4);
            for (int side : {-1, 1}) {
                const float x = static_cast<float>(side);
                for (int leg = 0; leg < 2; ++leg) {
                    const auto& pose = grounded.legs[(side < 0 ? 0 : 2) + leg];
                    const Vector3 root(pose.root.x, pose.root.y, pose.root.z);
                    const Vector3 knee(pose.knee.x, pose.knee.y, pose.knee.z);
                    const Vector3 hoof(pose.hoof.x, pose.hoof.y, pose.hoof.z);
                    mesh.Limb(root, knee, 0.105f, 0.057f, coat, near ? 8 : 5);
                    mesh.Limb(knee, hoof, 0.057f, 0.034f, dark, near ? 8 : 5);
                    if (near) {
                        mesh.Ellipsoid(knee, Vector3(0.059f, 0.059f, 0.059f), dark, 6, 3);
                        for (const float toe : {-1.0f, 1.0f}) {
                            mesh.Ellipsoid(hoof + Vector3(toe * 0.032f, 0, 0.025f),
                                           Vector3(0.028f, BoarLegPose::HoofHeight, 0.085f),
                                           Color(41, 36, 30), 6, 3);
                        }
                    } else {
                        mesh.Ellipsoid(hoof + Vector3(0, 0, 0.025f),
                                       Vector3(0.06f, BoarLegPose::HoofHeight, 0.085f),
                                       Color(41, 36, 30), 5, 3);
                    }
                }
                const auto ear=BuildBoarEar(side,dip,bob,near);
                for(std::size_t i=0;i<ear.count;++i) {
                    const auto& face=ear.triangles[i];
                    const auto point=[](BoarJoint p){return Vector3(p.x,p.y,p.z);};
                    const auto vertex=[&](std::size_t v) {
                        const auto p=face.points[v];
                        const float radius=std::hypot((p.y-(0.935f-dip*0.35f+bob))/0.16f,
                            (p.z-0.59f)/0.10f);
                        const float tone=face.surface==BoarEarSurface::Back?0.94f:
                            0.88f+0.12f*std::clamp(radius,0.0f,1.0f);
                        return mesh.SmoothVertex(point(p),point(face.normals[v]),
                            wildlife_render::ScaleColor(coat,tone));
                    };
                    mesh.SmoothTriangle(vertex(0),vertex(1),vertex(2));
                }
                if (near) {
                    const auto mouth=BuildBoarMouth(side,dip,bob);
                    const auto mouthPoint=[&](std::size_t i) {
                        const auto p=mouth[i];return Vector3(p.x,p.y,p.z);
                    };
                    const auto mouthTriangle=[&](std::size_t a,std::size_t b,std::size_t c) {
                        if(side<0) std::swap(b,c);
                        mesh.Triangle(mouthPoint(a),mouthPoint(b),mouthPoint(c),
                            wildlife_render::ScaleColor(coat,0.48f));
                    };
                    for(std::size_t i=0;i+3<mouth.size();i+=2) {
                        mouthTriangle(i,i+1,i+3);mouthTriangle(i,i+3,i+2);
                    }
                    const auto eye=BuildBoarEye(side,dip,bob);
                    const auto eyePoint=[&](std::size_t i) {
                        const auto p=eye[i];return Vector3(p.x,p.y,p.z);
                    };
                    const auto eyeTriangle=[&](std::size_t a,std::size_t b,std::size_t c,const Color& tint) {
                        // Ring angles run around Y/Z with outward -X winding.
                        if(side>0) std::swap(b,c);
                        mesh.Triangle(eyePoint(a),eyePoint(b),eyePoint(c),tint);
                    };
                    const auto lid=wildlife_render::ScaleColor(coat,0.74f);
                    for(std::size_t i=0;i<8;++i) {
                        const auto j=(i+1)%8;
                        eyeTriangle(i,j,8+j,lid);eyeTriangle(i,8+j,8+i,lid);
                        eyeTriangle(16,8+i,8+j,Color(18,15,11));
                    }
                    mesh.Ellipsoid(Vector3(x * 0.046f, 0.435f - dip + bob, 1.068f),
                                   Vector3(0.021f, 0.013f, 0.009f), Color(17, 15, 13), 6, 3);
                    if (boar.scale > 0.8f) {
                        const Vector3 root(x * 0.125f, 0.375f - dip + bob, 0.91f);
                        const Vector3 bend(x * 0.195f, 0.43f - dip + bob, 0.945f);
                        const Vector3 tip(x * 0.205f, 0.505f - dip + bob, 0.92f);
                        const Vector3 edge(0.0f, 0.0f, -0.025f);
                        const Color ivory(185, 171, 134);
                        mesh.Triangle(root, bend, root + edge, ivory);
                        mesh.Triangle(root + edge, bend, bend + edge * 0.5f, ivory);
                        mesh.Triangle(bend, tip, bend + edge * 0.5f, ivory);
                    }
                }
            }
            if (near) {
                // Short swept fibers rooted slightly inside the shared body, not a row of spikes.
                const auto profile = MakeBoarBodyProfile(dip);
                for (int tuft = 0; tuft < 45; ++tuft) {
                    const float t = static_cast<float>(tuft);
                    const float z = -0.55f + t * 0.021f + std::sin(t * 2.7f) * 0.007f;
                    const float x = std::sin(t * 2.1f) * 0.030f;
                    const float height = BoarBackHeight(profile, z, x) + bob - 0.007f;
                    const float length = (boar.scale < 0.8f ? 0.017f : 0.026f)
                        + (0.5f + 0.5f * std::sin(t * 1.7f)) * 0.014f;
                    mesh.Triangle(Vector3(x - 0.005f, height, z),
                                  Vector3(x + 0.003f, height + length, z - 0.013f),
                                  Vector3(x + 0.005f, height, z + 0.009f), dark);
                }
                mesh.Ellipsoid(Vector3(0.015f, 0.66f, -0.76f), Vector3(0.025f, 0.035f, 0.17f), dark, 6, 3);
            }
        }
    }

    BoarRenderer::BoarRenderer(GraphicsDevice& device, const IslandTerrain& terrain,
                               const CanopyLightField& canopyLight,
                               const float maximumDistance)
        : device_(device), terrain_(terrain), canopyLight_(canopyLight),
          renderedTerrain_(terrain),
          coatTexture_(std::make_unique<Texture2D>(device, BoarCoatTexture::Size, BoarCoatTexture::Size)),
          effect_(std::make_unique<BasicEffect>(device)),
          buffer_(std::make_unique<DynamicVertexBuffer>(device,
              VertexPositionColorTexture::getVertexDeclarationStatic(), MaximumVertices, BufferUsage::WriteOnly)),
          maximumDistance_(maximumDistance)
    {
        if (!std::isfinite(maximumDistance) || maximumDistance <= 0.0f) {
            throw std::invalid_argument("Boar render distance must be finite and positive");
        }
        staging_.reserve(MaximumVertices);
        const auto texels = BoarCoatTexture::Generate();
        std::vector<Color> pixels;
        pixels.reserve(texels.size());
        for (const auto value : texels) {
            pixels.emplace_back(value, value, value, static_cast<std::uint8_t>(255));
        }
        coatTexture_->SetData(pixels.data(), static_cast<int>(pixels.size()));
        effect_->setTextureEnabledProperty(true);
        effect_->setTextureProperty(coatTexture_.get());
        effect_->setVertexColorEnabledProperty(true);
        effect_->setLightingEnabledProperty(false);
        effect_->setFogEnabledProperty(true);
        effect_->setFogColorProperty(Vector3(environment::FogColorRed, environment::FogColorGreen, environment::FogColorBlue));
        effect_->setFogStartProperty(90.0f);
        effect_->setFogEndProperty(180.0f);
        constexpr const char* diagnostic =
            "island-simulator: boars receive bilinear per-vertex canopy light in their existing batch";
        std::cout << diagnostic << '\n';
        MirrorRuntimeDiagnosticToPlatformLog(diagnostic);
        constexpr const char* coatDiagnostic =
            "island-simulator: boar directional coat atlas 128x128, opaque stock BasicEffect";
        std::cout << coatDiagnostic << '\n';
        MirrorRuntimeDiagnosticToPlatformLog(coatDiagnostic);
        constexpr const char* poseDiagnostic =
            "island-simulator: boars use connected limb poses, stance contact and body-rooted bristles";
        std::cout << poseDiagnostic << '\n';
        MirrorRuntimeDiagnosticToPlatformLog(poseDiagnostic);
        constexpr const char* contactDiagnostic =
            "island-simulator: boar soles fit rendered terrain independently with fixed-length limbs";
        std::cout << contactDiagnostic << '\n';
        MirrorRuntimeDiagnosticToPlatformLog(contactDiagnostic);
    }
    BoarRenderer::~BoarRenderer() = default;

    void BoarRenderer::Draw(const Matrix& view, const Matrix& projection,
                            const Vector3& camera, const std::span<const BoarState> boars)
    {
        staging_.clear();
        visibleCount_ = drawCalls_ = 0;
        const BoundingFrustum frustum(view * projection);
        for (const auto& boar : boars) {
            const Vector3 origin(boar.x, renderedTerrain_.Sample(boar.x, boar.z).height, boar.z);
            const Vector3 centre = origin + Vector3(0, 0.55f * boar.scale, 0);
            const float distance = Vector3::DistanceSquared(centre, camera);
            if (distance > maximumDistance_ * maximumDistance_
                || !frustum.Intersects(BoundingSphere(centre, 1.5f * boar.scale))) continue;
            const auto grounded = FitBoarGroundPose(terrain_, renderedTerrain_, boar);
            const auto vector = [](BoarJoint p) { return Vector3(p.x, p.y, p.z); };
            if (!reportedContactLimit_ && std::any_of(grounded.legs.begin(), grounded.legs.end(),
                                                     [](const auto& leg) { return leg.reachLimited; })) {
                constexpr const char* warning =
                    "island-simulator: warning: boar terrain contact limited by limb reach";
                std::cerr << warning << '\n';
                MirrorRuntimeDiagnosticToPlatformLog(warning);
                reportedContactLimit_ = true;
            }
            ModelBuilder mesh{staging_, vector(grounded.origin), vector(grounded.right),
                              vector(grounded.up), vector(grounded.forward), boar.scale, canopyLight_};
            const auto vertexStart=staging_.size();
            AppendBoar(mesh, boar, grounded, distance < 55.0f * 55.0f);
            if(staging_.size()-vertexStart>6000U)
                throw std::runtime_error("Single boar exceeds per-animal geometry budget");
            ++visibleCount_;
        }
        if (staging_.empty()) return;
        if (staging_.size() > static_cast<std::size_t>(MaximumVertices)) {
            throw std::runtime_error("Boar geometry exceeds its bounded GPU allocation");
        }
        device_.setRasterizerStateProperty(RasterizerState::CullNone);
        device_.setBlendStateProperty(BlendState::Opaque);
        device_.setDepthStencilStateProperty(DepthStencilState::Default);
        device_.getSamplerStatesProperty()[0] = SamplerState::LinearClamp;
        buffer_->SetData(staging_.data(), 0, static_cast<int>(staging_.size()), SetDataOptions::Discard);
        device_.SetVertexBuffer(buffer_.get());
        device_.setIndicesProperty(nullptr);
        effect_->setWorldProperty(Matrix::getIdentityProperty());
        effect_->setViewProperty(view);
        effect_->setProjectionProperty(projection);
        effect_->Apply();
        device_.DrawPrimitives(PrimitiveType::TriangleList, 0, static_cast<int>(staging_.size() / 3));
        drawCalls_ = 1;
    }
}
