#include "IslandSimulator/MonkeyRenderer.hpp"

#include "IslandSimulator/EnvironmentLighting.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <functional>
#include <numbers>
#include <stdexcept>
#include <utility>

#include "Microsoft/Xna/Framework/BoundingFrustum.hpp"
#include "Microsoft/Xna/Framework/BoundingSphere.hpp"
#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/MathHelper.hpp"
#include "Microsoft/Xna/Framework/Graphics/AnimationPlayer.hpp"
#include "Microsoft/Xna/Framework/Graphics/BlendState.hpp"
#include "Microsoft/Xna/Framework/Graphics/BufferUsage.hpp"
#include "Microsoft/Xna/Framework/Graphics/DepthStencilState.hpp"
#include "Microsoft/Xna/Framework/Graphics/DirectionalLight.hpp"
#include "Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp"
#include "Microsoft/Xna/Framework/Graphics/IndexBuffer.hpp"
#include "Microsoft/Xna/Framework/Graphics/IndexElementSize.hpp"
#include "Microsoft/Xna/Framework/Graphics/PrimitiveType.hpp"
#include "Microsoft/Xna/Framework/Graphics/RasterizerState.hpp"
#include "Microsoft/Xna/Framework/Graphics/SkinnedEffect.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"
#include "Microsoft/Xna/Framework/Graphics/VertexBuffer.hpp"
#include "Microsoft/Xna/Framework/Graphics/VertexPositionNormalTextureSkinned.hpp"
#include "Microsoft/Xna/Framework/Quaternion.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Vector4.hpp"
#include "System/TimeSpan.hpp"

using namespace Microsoft::Xna::Framework;
using namespace Microsoft::Xna::Framework::Graphics;

namespace island
{
    namespace
    {
        using SkinnedVertex = VertexPositionNormalTextureSkinned;

        constexpr float NearLodDistance = 92.0f;
        constexpr float MaximumDistance = 235.0f;
        constexpr std::size_t BoneCount = 14U;

        enum Bone : std::uint8_t
        {
            Pelvis,
            Spine,
            Head,
            UpperArmLeft,
            ForearmLeft,
            UpperArmRight,
            ForearmRight,
            ThighLeft,
            ShinLeft,
            ThighRight,
            ShinRight,
            TailBase,
            TailMiddle,
            TailTip
        };

        struct RigDefinition
        {
            std::array<Vector3, BoneCount> heads;
            std::array<int, BoneCount> parents;
        };

        struct MeshData
        {
            std::vector<SkinnedVertex> vertices;
            std::vector<std::uint16_t> indices;
        };

        [[nodiscard]] RigDefinition CreateRig()
        {
            return RigDefinition{
                .heads = {
                    Vector3(0.0f, 0.56f, 0.0f),
                    Vector3(0.0f, 0.91f, 0.0f),
                    Vector3(0.0f, 1.27f, 0.015f),
                    Vector3(-0.27f, 1.04f, 0.0f),
                    Vector3(-0.43f, 0.67f, 0.055f),
                    Vector3(0.27f, 1.04f, 0.0f),
                    Vector3(0.43f, 0.67f, 0.055f),
                    Vector3(-0.15f, 0.57f, 0.0f),
                    Vector3(-0.20f, 0.28f, 0.035f),
                    Vector3(0.15f, 0.57f, 0.0f),
                    Vector3(0.20f, 0.28f, 0.035f),
                    Vector3(0.0f, 0.58f, -0.17f),
                    Vector3(0.03f, 0.48f, -0.58f),
                    Vector3(-0.05f, 0.66f, -0.96f)},
                .parents = {-1, Pelvis, Spine, Spine, UpperArmLeft, Spine, UpperArmRight,
                            Pelvis, ThighLeft, Pelvis, ThighRight, Pelvis, TailBase,
                            TailMiddle}};
        }

        [[nodiscard]] Vector3 RestTranslation(const RigDefinition& rig, const int bone)
        {
            const int parent = rig.parents[static_cast<std::size_t>(bone)];
            return rig.heads[static_cast<std::size_t>(bone)]
                - (parent < 0 ? Vector3::Zero : rig.heads[static_cast<std::size_t>(parent)]);
        }

        [[nodiscard]] Quaternion Pitch(const float radians)
        {
            return Quaternion::CreateFromAxisAngle(Vector3::Right, radians);
        }

        [[nodiscard]] Quaternion Yaw(const float radians)
        {
            return Quaternion::CreateFromAxisAngle(Vector3::Up, radians);
        }

        [[nodiscard]] Quaternion Roll(const float radians)
        {
            return Quaternion::CreateFromAxisAngle(Vector3(0.0f, 0.0f, 1.0f), radians);
        }

        [[nodiscard]] BoneTrackEXT MakeTrack(
            const RigDefinition& rig, const int bone, const float duration, const int keyCount,
            const std::function<Quaternion(float)>& rotation,
            const std::function<Vector3(float)>& translation = {})
        {
            BoneTrackEXT track;
            track.BoneIndex = bone;
            const Vector3 rest = RestTranslation(rig, bone);
            for (int index = 0; index <= keyCount; ++index) {
                const float phase = static_cast<float>(index) / static_cast<float>(keyCount);
                KeyframeEXT key;
                key.Time = System::TimeSpan::FromSeconds(
                    static_cast<double>(duration * phase));
                key.Translation = translation ? translation(phase) : rest;
                key.Rotation = rotation ? rotation(phase) : Quaternion::Identity;
                track.Keys.push_back(key);
            }
            return track;
        }

        [[nodiscard]] AnimationClip CreateIdleClip(const RigDefinition& rig)
        {
            constexpr float Duration = 5.4f;
            constexpr int Keys = 18;
            AnimationClip clip;
            clip.Duration = System::TimeSpan::FromSeconds(Duration);
            clip.Tracks.push_back(MakeTrack(rig, Pelvis, Duration, Keys,
                [](const float phase) {
                    return Roll(std::sin(phase * MathHelper::TwoPi) * 0.025f);
                },
                [&rig](const float phase) {
                    return RestTranslation(rig, Pelvis)
                        + Vector3(0.0f, std::sin(phase * MathHelper::TwoPi * 2.0f) * 0.008f,
                                  0.0f);
                }));
            clip.Tracks.push_back(MakeTrack(rig, Head, Duration, Keys,
                [](const float phase) {
                    return Yaw(std::sin(phase * MathHelper::TwoPi) * 0.22f)
                        * Pitch(std::sin(phase * MathHelper::TwoPi * 0.5f) * 0.05f);
                }));
            clip.Tracks.push_back(MakeTrack(rig, TailBase, Duration, Keys,
                [](const float phase) {
                    return Yaw(std::sin(phase * MathHelper::TwoPi + 0.7f) * 0.20f);
                }));
            clip.Tracks.push_back(MakeTrack(rig, TailMiddle, Duration, Keys,
                [](const float phase) {
                    return Yaw(std::sin(phase * MathHelper::TwoPi + 1.5f) * 0.28f);
                }));
            clip.Tracks.push_back(MakeTrack(rig, TailTip, Duration, Keys,
                [](const float phase) {
                    return Yaw(std::sin(phase * MathHelper::TwoPi + 2.3f) * 0.35f);
                }));
            return clip;
        }

        [[nodiscard]] AnimationClip CreateObserveClip(const RigDefinition& rig)
        {
            constexpr float Duration = 3.6f;
            constexpr int Keys = 14;
            AnimationClip clip = CreateIdleClip(rig);
            clip.Duration = System::TimeSpan::FromSeconds(Duration);
            for (BoneTrackEXT& track : clip.Tracks) {
                const std::size_t last = track.Keys.empty() ? 0U : track.Keys.size() - 1U;
                for (std::size_t index = 0; index < track.Keys.size(); ++index) {
                    const double phase = last == 0U ? 0.0
                        : static_cast<double>(index) / static_cast<double>(last);
                    track.Keys[index].Time = System::TimeSpan::FromSeconds(Duration * phase);
                }
            }
            clip.Tracks.push_back(MakeTrack(rig, Spine, Duration, Keys,
                [](const float phase) {
                    return Pitch(-0.10f + std::sin(phase * MathHelper::TwoPi) * 0.035f);
                }));
            clip.Tracks.push_back(MakeTrack(rig, Head, Duration, Keys,
                [](const float phase) {
                    return Yaw(std::sin(phase * MathHelper::TwoPi) * 0.48f)
                        * Pitch(-0.08f + std::sin(phase * MathHelper::TwoPi * 2.0f) * 0.04f);
                }));
            return clip;
        }

        [[nodiscard]] AnimationClip CreateLocomotionClip(const RigDefinition& rig,
                                                          const bool running)
        {
            const float duration = running ? 0.66f : 1.18f;
            constexpr int Keys = 16;
            const float legSwing = running ? 0.60f : 0.38f;
            const float armSwing = running ? 0.72f : 0.48f;
            AnimationClip clip;
            clip.Duration = System::TimeSpan::FromSeconds(duration);
            for (int side = 0; side < 2; ++side) {
                const float sidePhase = side == 0 ? 0.0f : std::numbers::pi_v<float>;
                const int thigh = side == 0 ? ThighLeft : ThighRight;
                const int shin = side == 0 ? ShinLeft : ShinRight;
                const int upperArm = side == 0 ? UpperArmLeft : UpperArmRight;
                const int forearm = side == 0 ? ForearmLeft : ForearmRight;
                clip.Tracks.push_back(MakeTrack(rig, thigh, duration, Keys,
                    [sidePhase, legSwing](const float phase) {
                        return Pitch(std::sin(phase * MathHelper::TwoPi + sidePhase) * legSwing);
                    }));
                clip.Tracks.push_back(MakeTrack(rig, shin, duration, Keys,
                    [sidePhase, running](const float phase) {
                        const float lift = std::max(0.0f,
                            std::sin(phase * MathHelper::TwoPi + sidePhase));
                        return Pitch(lift * (running ? 0.85f : 0.54f));
                    }));
                clip.Tracks.push_back(MakeTrack(rig, upperArm, duration, Keys,
                    [sidePhase, armSwing, side](const float phase) {
                        const float sideRoll = side == 0 ? -0.12f : 0.12f;
                        return Roll(sideRoll)
                            * Pitch(-std::sin(phase * MathHelper::TwoPi + sidePhase) * armSwing);
                    }));
                clip.Tracks.push_back(MakeTrack(rig, forearm, duration, Keys,
                    [sidePhase, running](const float phase) {
                        const float flex = 0.22f + std::max(0.0f,
                            -std::sin(phase * MathHelper::TwoPi + sidePhase))
                                * (running ? 0.62f : 0.36f);
                        return Pitch(-flex);
                    }));
            }
            clip.Tracks.push_back(MakeTrack(rig, Pelvis, duration, Keys,
                [](const float phase) {
                    return Yaw(std::sin(phase * MathHelper::TwoPi) * 0.055f);
                },
                [&rig, running](const float phase) {
                    const float bob = std::fabs(std::sin(phase * MathHelper::TwoPi))
                        * (running ? 0.045f : 0.024f);
                    return RestTranslation(rig, Pelvis) + Vector3(0.0f, bob, 0.0f);
                }));
            clip.Tracks.push_back(MakeTrack(rig, Spine, duration, Keys,
                [running](const float phase) {
                    return Pitch(running ? -0.18f : -0.09f)
                        * Yaw(-std::sin(phase * MathHelper::TwoPi) * 0.075f);
                }));
            clip.Tracks.push_back(MakeTrack(rig, TailBase, duration, Keys,
                [](const float phase) {
                    return Yaw(std::sin(phase * MathHelper::TwoPi + 1.1f) * 0.30f);
                }));
            clip.Tracks.push_back(MakeTrack(rig, TailMiddle, duration, Keys,
                [](const float phase) {
                    return Yaw(std::sin(phase * MathHelper::TwoPi + 1.8f) * 0.38f);
                }));
            clip.Tracks.push_back(MakeTrack(rig, TailTip, duration, Keys,
                [](const float phase) {
                    return Yaw(std::sin(phase * MathHelper::TwoPi + 2.5f) * 0.46f);
                }));
            return clip;
        }

        [[nodiscard]] std::unique_ptr<SkinningData> CreateSkinningData(
            const RigDefinition& rig)
        {
            auto data = std::make_unique<SkinningData>();
            data->BoneCount = static_cast<int>(BoneCount);
            data->SkeletonHierarchy.assign(rig.parents.begin(), rig.parents.end());
            data->BindPose.reserve(BoneCount);
            data->InverseBindPose.reserve(BoneCount);
            for (std::size_t index = 0; index < BoneCount; ++index) {
                data->BindPose.push_back(Matrix::CreateTranslation(
                    RestTranslation(rig, static_cast<int>(index))));
                data->InverseBindPose.push_back(Matrix::CreateTranslation(-rig.heads[index]));
            }
            data->AnimationClips.emplace("idle", CreateIdleClip(rig));
            data->AnimationClips.emplace("observe", CreateObserveClip(rig));
            data->AnimationClips.emplace("walk", CreateLocomotionClip(rig, false));
            data->AnimationClips.emplace("run", CreateLocomotionClip(rig, true));
            return data;
        }

        void AddVertex(MeshData& mesh, const Vector3& position, const Vector3& normal,
                       const int bone)
        {
            mesh.vertices.emplace_back(position, normal, Vector2::Zero,
                Vector4(1.0f, 0.0f, 0.0f, 0.0f),
                std::array<std::uint8_t, 4>{static_cast<std::uint8_t>(bone), 0U, 0U, 0U});
        }

        void AddEllipsoid(MeshData& mesh, const Vector3& centre, const Vector3& radii,
                          const int bone, const int rings, const int segments)
        {
            const std::uint16_t base = static_cast<std::uint16_t>(mesh.vertices.size());
            for (int ring = 0; ring <= rings; ++ring) {
                const float latitude = -MathHelper::PiOver2
                    + MathHelper::Pi * static_cast<float>(ring) / static_cast<float>(rings);
                const float y = std::sin(latitude);
                const float ringRadius = std::cos(latitude);
                for (int segment = 0; segment <= segments; ++segment) {
                    const float longitude = MathHelper::TwoPi
                        * static_cast<float>(segment) / static_cast<float>(segments);
                    const Vector3 local(std::cos(longitude) * ringRadius, y,
                                        std::sin(longitude) * ringRadius);
                    const Vector3 position(centre.X + local.X * radii.X,
                                           centre.Y + local.Y * radii.Y,
                                           centre.Z + local.Z * radii.Z);
                    const Vector3 normal = Vector3::Normalize(Vector3(
                        local.X / std::max(radii.X, 0.001f),
                        local.Y / std::max(radii.Y, 0.001f),
                        local.Z / std::max(radii.Z, 0.001f)));
                    AddVertex(mesh, position, normal, bone);
                }
            }
            const int stride = segments + 1;
            for (int ring = 0; ring < rings; ++ring) {
                for (int segment = 0; segment < segments; ++segment) {
                    const auto a = static_cast<std::uint16_t>(base + ring * stride + segment);
                    const auto b = static_cast<std::uint16_t>(a + stride);
                    const auto c = static_cast<std::uint16_t>(b + 1U);
                    const auto d = static_cast<std::uint16_t>(a + 1U);
                    mesh.indices.insert(mesh.indices.end(), {a, b, c, a, c, d});
                }
            }
        }

        void AddTube(MeshData& mesh, const Vector3& from, const Vector3& to,
                     const float radius, const int bone, const int segments)
        {
            const Vector3 axis = Vector3::Normalize(to - from);
            const Vector3 reference = std::fabs(axis.Y) < 0.88f ? Vector3::Up : Vector3::Right;
            const Vector3 right = Vector3::Normalize(Vector3::Cross(reference, axis));
            const Vector3 up = Vector3::Normalize(Vector3::Cross(axis, right));
            const std::uint16_t base = static_cast<std::uint16_t>(mesh.vertices.size());
            for (int end = 0; end < 2; ++end) {
                const Vector3 centre = end == 0 ? from : to;
                for (int segment = 0; segment <= segments; ++segment) {
                    const float angle = MathHelper::TwoPi
                        * static_cast<float>(segment) / static_cast<float>(segments);
                    const Vector3 normal = right * std::cos(angle) + up * std::sin(angle);
                    AddVertex(mesh, centre + normal * radius, normal, bone);
                }
            }
            const int stride = segments + 1;
            for (int segment = 0; segment < segments; ++segment) {
                const auto a = static_cast<std::uint16_t>(base + segment);
                const auto b = static_cast<std::uint16_t>(a + stride);
                const auto c = static_cast<std::uint16_t>(b + 1U);
                const auto d = static_cast<std::uint16_t>(a + 1U);
                mesh.indices.insert(mesh.indices.end(), {a, b, c, a, c, d});
            }
        }

        [[nodiscard]] MeshData CreateMonkeyMesh(const RigDefinition& rig, const bool near)
        {
            MeshData mesh;
            const int rings = near ? 6 : 3;
            const int segments = near ? 9 : 5;
            const int limbSegments = near ? 7 : 4;
            AddEllipsoid(mesh, Vector3(0.0f, 0.87f, 0.0f),
                         Vector3(0.30f, 0.40f, 0.22f), Spine, rings, segments);
            AddEllipsoid(mesh, Vector3(0.0f, 1.30f, 0.035f),
                         Vector3(0.235f, 0.225f, 0.21f), Head, rings, segments);
            AddEllipsoid(mesh, Vector3(0.0f, 1.22f, 0.205f),
                         Vector3(0.17f, 0.115f, 0.145f), Head,
                         std::max(3, rings - 1), segments);
            AddEllipsoid(mesh, Vector3(-0.215f, 1.32f, 0.015f),
                         Vector3(0.085f, 0.105f, 0.055f), Head, 3, segments);
            AddEllipsoid(mesh, Vector3(0.215f, 1.32f, 0.015f),
                         Vector3(0.085f, 0.105f, 0.055f), Head, 3, segments);

            AddTube(mesh, rig.heads[UpperArmLeft], rig.heads[ForearmLeft],
                    0.078f, UpperArmLeft, limbSegments);
            AddTube(mesh, rig.heads[ForearmLeft], Vector3(-0.47f, 0.34f, 0.10f),
                    0.063f, ForearmLeft, limbSegments);
            AddTube(mesh, rig.heads[UpperArmRight], rig.heads[ForearmRight],
                    0.078f, UpperArmRight, limbSegments);
            AddTube(mesh, rig.heads[ForearmRight], Vector3(0.47f, 0.34f, 0.10f),
                    0.063f, ForearmRight, limbSegments);
            AddEllipsoid(mesh, Vector3(-0.47f, 0.30f, 0.12f),
                         Vector3(0.075f, 0.10f, 0.065f), ForearmLeft, 3, segments);
            AddEllipsoid(mesh, Vector3(0.47f, 0.30f, 0.12f),
                         Vector3(0.075f, 0.10f, 0.065f), ForearmRight, 3, segments);

            AddTube(mesh, rig.heads[ThighLeft], rig.heads[ShinLeft],
                    0.105f, ThighLeft, limbSegments);
            AddTube(mesh, rig.heads[ShinLeft], Vector3(-0.20f, 0.06f, 0.08f),
                    0.075f, ShinLeft, limbSegments);
            AddTube(mesh, rig.heads[ThighRight], rig.heads[ShinRight],
                    0.105f, ThighRight, limbSegments);
            AddTube(mesh, rig.heads[ShinRight], Vector3(0.20f, 0.06f, 0.08f),
                    0.075f, ShinRight, limbSegments);
            AddEllipsoid(mesh, Vector3(-0.20f, 0.055f, 0.135f),
                         Vector3(0.10f, 0.055f, 0.16f), ShinLeft, 3, segments);
            AddEllipsoid(mesh, Vector3(0.20f, 0.055f, 0.135f),
                         Vector3(0.10f, 0.055f, 0.16f), ShinRight, 3, segments);

            AddTube(mesh, rig.heads[TailBase], rig.heads[TailMiddle],
                    0.062f, TailBase, limbSegments);
            AddTube(mesh, rig.heads[TailMiddle], rig.heads[TailTip],
                    0.053f, TailMiddle, limbSegments);
            AddTube(mesh, rig.heads[TailTip], Vector3(0.03f, 0.91f, -1.20f),
                    0.043f, TailTip, limbSegments);
            if (mesh.vertices.size() > 65535U) {
                throw std::runtime_error("procedural monkey mesh exceeds portable 16-bit indices");
            }
            return mesh;
        }

        [[nodiscard]] MeshData CreateFaceMesh()
        {
            MeshData mesh;
            AddEllipsoid(mesh, Vector3(0.0f, 1.215f, 0.218f),
                         Vector3(0.176f, 0.116f, 0.151f), Head, 5, 8);
            AddEllipsoid(mesh, Vector3(-0.216f, 1.32f, 0.047f),
                         Vector3(0.054f, 0.071f, 0.029f), Head, 3, 7);
            AddEllipsoid(mesh, Vector3(0.216f, 1.32f, 0.047f),
                         Vector3(0.054f, 0.071f, 0.029f), Head, 3, 7);
            return mesh;
        }

        [[nodiscard]] MeshData CreateEyeMesh()
        {
            MeshData mesh;
            AddEllipsoid(mesh, Vector3(-0.073f, 1.342f, 0.207f),
                         Vector3(0.026f, 0.032f, 0.026f), Head, 3, 6);
            AddEllipsoid(mesh, Vector3(0.073f, 1.342f, 0.207f),
                         Vector3(0.026f, 0.032f, 0.026f), Head, 3, 6);
            return mesh;
        }

        [[nodiscard]] const char* ClipName(const MonkeyBehavior behavior)
        {
            switch (behavior) {
            case MonkeyBehavior::Observe:
                return "observe";
            case MonkeyBehavior::Walk:
            case MonkeyBehavior::Wander:
                return "walk";
            case MonkeyBehavior::RunAway:
                return "run";
            case MonkeyBehavior::Idle:
                return "idle";
            }
            return "idle";
        }

        const std::array<Vector3, 4> MonkeyColors = {
            Vector3(0.30f, 0.20f, 0.13f),
            Vector3(0.39f, 0.27f, 0.17f),
            Vector3(0.24f, 0.18f, 0.14f),
            Vector3(0.46f, 0.32f, 0.20f)};
    }

    MonkeyRenderer::MonkeyRenderer(GraphicsDevice& device)
        : device_(device),
          whiteTexture_(std::make_unique<Texture2D>(device, 1, 1)),
          effect_(std::make_unique<SkinnedEffect>(device))
    {
        const Color whitePixel = Color::White;
        whiteTexture_->SetData(&whitePixel, 1);
        const RigDefinition rig = CreateRig();
        skinningData_ = CreateSkinningData(rig);
        MeshData nearMesh = CreateMonkeyMesh(rig, true);
        MeshData farMesh = CreateMonkeyMesh(rig, false);
        MeshData faceMesh = CreateFaceMesh();
        MeshData eyeMesh = CreateEyeMesh();
        nearVertexCount_ = static_cast<int>(nearMesh.vertices.size());
        nearPrimitiveCount_ = static_cast<int>(nearMesh.indices.size() / 3U);
        farVertexCount_ = static_cast<int>(farMesh.vertices.size());
        farPrimitiveCount_ = static_cast<int>(farMesh.indices.size() / 3U);
        faceVertexCount_ = static_cast<int>(faceMesh.vertices.size());
        facePrimitiveCount_ = static_cast<int>(faceMesh.indices.size() / 3U);
        eyeVertexCount_ = static_cast<int>(eyeMesh.vertices.size());
        eyePrimitiveCount_ = static_cast<int>(eyeMesh.indices.size() / 3U);

        nearVertices_ = std::make_unique<VertexBuffer>(
            device_, SkinnedVertex::getVertexDeclarationStatic(), nearVertexCount_,
            BufferUsage::WriteOnly);
        nearVertices_->SetData(nearMesh.vertices.data(), nearVertexCount_);
        nearIndices_ = std::make_unique<IndexBuffer>(
            device_, IndexElementSize::SixteenBits, static_cast<int>(nearMesh.indices.size()),
            BufferUsage::WriteOnly);
        nearIndices_->SetData(nearMesh.indices.data(), static_cast<int>(nearMesh.indices.size()));

        farVertices_ = std::make_unique<VertexBuffer>(
            device_, SkinnedVertex::getVertexDeclarationStatic(), farVertexCount_,
            BufferUsage::WriteOnly);
        farVertices_->SetData(farMesh.vertices.data(), farVertexCount_);
        farIndices_ = std::make_unique<IndexBuffer>(
            device_, IndexElementSize::SixteenBits, static_cast<int>(farMesh.indices.size()),
            BufferUsage::WriteOnly);
        farIndices_->SetData(farMesh.indices.data(), static_cast<int>(farMesh.indices.size()));

        faceVertices_ = std::make_unique<VertexBuffer>(
            device_, SkinnedVertex::getVertexDeclarationStatic(), faceVertexCount_,
            BufferUsage::WriteOnly);
        faceVertices_->SetData(faceMesh.vertices.data(), faceVertexCount_);
        faceIndices_ = std::make_unique<IndexBuffer>(
            device_, IndexElementSize::SixteenBits, static_cast<int>(faceMesh.indices.size()),
            BufferUsage::WriteOnly);
        faceIndices_->SetData(faceMesh.indices.data(), static_cast<int>(faceMesh.indices.size()));

        eyeVertices_ = std::make_unique<VertexBuffer>(
            device_, SkinnedVertex::getVertexDeclarationStatic(), eyeVertexCount_,
            BufferUsage::WriteOnly);
        eyeVertices_->SetData(eyeMesh.vertices.data(), eyeVertexCount_);
        eyeIndices_ = std::make_unique<IndexBuffer>(
            device_, IndexElementSize::SixteenBits, static_cast<int>(eyeMesh.indices.size()),
            BufferUsage::WriteOnly);
        eyeIndices_->SetData(eyeMesh.indices.data(), static_cast<int>(eyeMesh.indices.size()));

        effect_->setWeightsPerVertexProperty(1);
        effect_->setTextureProperty(whiteTexture_.get());
        effect_->setAmbientLightColorProperty(Vector3(0.47f, 0.50f, 0.42f));
        effect_->setSpecularColorProperty(Vector3(0.04f, 0.04f, 0.035f));
        effect_->setSpecularPowerProperty(8.0f);
        effect_->setFogEnabledProperty(true);
        effect_->setFogColorProperty(Vector3(0.53f, 0.78f, 0.88f));
        effect_->setFogStartProperty(210.0f);
        effect_->setFogEndProperty(620.0f);
        auto& light = effect_->getDirectionalLight0Property();
        light.setEnabledProperty(true);
        light.setDirectionProperty(Vector3(-environment::SunDirectionX,
                                           -environment::SunDirectionY,
                                           -environment::SunDirectionZ));
        light.setDiffuseColorProperty(Vector3(0.78f, 0.74f, 0.62f));
        light.setSpecularColorProperty(Vector3(0.16f, 0.14f, 0.10f));
        effect_->getDirectionalLight1Property().setEnabledProperty(false);
        effect_->getDirectionalLight2Property().setEnabledProperty(false);

        players_.reserve(MonkeyTroop::MonkeyCount);
        activeBehaviors_.assign(MonkeyTroop::MonkeyCount, MonkeyBehavior::Idle);
        const AnimationClip& idle = skinningData_->AnimationClips.at("idle");
        for (std::size_t index = 0; index < MonkeyTroop::MonkeyCount; ++index) {
            auto player = std::make_unique<AnimationPlayer>(*skinningData_);
            player->StartClip(idle);
            players_.push_back(std::move(player));
        }
    }

    MonkeyRenderer::~MonkeyRenderer() = default;

    void MonkeyRenderer::Draw(const Matrix& view, const Matrix& projection,
                              const Vector3& cameraPosition,
                              const std::span<const MonkeyState> monkeys)
    {
        visibleMonkeyCount_ = 0;
        visibleNearMonkeyCount_ = 0;
        visibleFarMonkeyCount_ = 0;
        drawCallCount_ = 0;
        const BoundingFrustum frustum(view * projection);
        effect_->setViewProperty(view);
        effect_->setProjectionProperty(projection);
        device_.setRasterizerStateProperty(RasterizerState::CullNone);
        device_.setBlendStateProperty(BlendState::Opaque);
        device_.setDepthStencilStateProperty(DepthStencilState::Default);

        const std::size_t count = std::min(monkeys.size(), players_.size());
        for (std::size_t index = 0; index < count; ++index) {
            const MonkeyState& monkey = monkeys[index];
            const Vector3 centre(monkey.x, monkey.y + 0.72f * monkey.scale, monkey.z);
            const float distanceSquared = Vector3::DistanceSquared(centre, cameraPosition);
            if (distanceSquared > MaximumDistance * MaximumDistance
                || !frustum.Intersects(BoundingSphere(centre, 1.2f * monkey.scale))) {
                continue;
            }

            const char* clipName = ClipName(monkey.behavior);
            if (activeBehaviors_[index] != monkey.behavior) {
                players_[index]->StartClip(skinningData_->AnimationClips.at(clipName));
                activeBehaviors_[index] = monkey.behavior;
            }
            players_[index]->Update(System::TimeSpan::FromSeconds(monkey.animationSeconds),
                                    false, true);
            effect_->SetBoneTransforms(players_[index]->GetSkinTransforms());
            effect_->setDiffuseColorProperty(
                MonkeyColors[monkey.colorVariant % MonkeyColors.size()]);
            const float yaw = std::numbers::pi_v<float> * 0.5f - monkey.headingRadians;
            effect_->setWorldProperty(Matrix::CreateScale(monkey.scale)
                * Matrix::CreateRotationY(yaw)
                * Matrix::CreateTranslation(monkey.x, monkey.y, monkey.z));

            const bool near = distanceSquared <= NearLodDistance * NearLodDistance;
            VertexBuffer* vertices = near ? nearVertices_.get() : farVertices_.get();
            IndexBuffer* indices = near ? nearIndices_.get() : farIndices_.get();
            const int vertexCount = near ? nearVertexCount_ : farVertexCount_;
            const int primitiveCount = near ? nearPrimitiveCount_ : farPrimitiveCount_;
            device_.SetVertexBuffer(vertices);
            device_.setIndicesProperty(indices);
            effect_->Apply();
            device_.DrawIndexedPrimitives(PrimitiveType::TriangleList, 0, 0,
                                          vertexCount, 0, primitiveCount);
            if (near) {
                effect_->setDiffuseColorProperty(Vector3(0.62f, 0.46f, 0.30f));
                device_.SetVertexBuffer(faceVertices_.get());
                device_.setIndicesProperty(faceIndices_.get());
                effect_->Apply();
                device_.DrawIndexedPrimitives(PrimitiveType::TriangleList, 0, 0,
                                              faceVertexCount_, 0, facePrimitiveCount_);
                effect_->setDiffuseColorProperty(Vector3(0.025f, 0.022f, 0.018f));
                device_.SetVertexBuffer(eyeVertices_.get());
                device_.setIndicesProperty(eyeIndices_.get());
                effect_->Apply();
                device_.DrawIndexedPrimitives(PrimitiveType::TriangleList, 0, 0,
                                              eyeVertexCount_, 0, eyePrimitiveCount_);
                drawCallCount_ += 2;
            }
            ++visibleMonkeyCount_;
            ++drawCallCount_;
            if (near) {
                ++visibleNearMonkeyCount_;
            } else {
                ++visibleFarMonkeyCount_;
            }
        }
    }
}
