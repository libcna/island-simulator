#include "IslandSimulator/MonkeyRenderer.hpp"
#include "IslandSimulator/CanopyLightField.hpp"
#include "IslandSimulator/RuntimeDiagnostics.hpp"
#include "IslandSimulator/MonkeySkinningShader.hpp"
#include "IslandSimulator/MonkeyGroundGait.hpp"
#include "IslandSimulator/OpaqueImageLoader.hpp"
#include "CNA/Graphics/ShaderPackageEXT.hpp"

#include "IslandSimulator/EnvironmentLighting.hpp"
#include "IslandSimulator/GeneratedTextureBudget.hpp"
#include "IslandSimulator/ProceduralMeshValidation.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <functional>
#include <iostream>
#include <numbers>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
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
#include "Microsoft/Xna/Framework/Graphics/ShaderEffect.hpp"
#include "Microsoft/Xna/Framework/Graphics/SamplerState.hpp"
#include "Microsoft/Xna/Framework/Graphics/SurfaceFormat.hpp"
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
        constexpr std::size_t BoneCount = MonkeySkinningShader::BoneCount;

        CNA::Graphics::ShaderPackageEXT MonkeyShaderPackage()
        {
            using CNA::Graphics::ShaderCodeEXT;
            return CNA::Graphics::ShaderPackageEXT({
                ShaderCodeEXT(CNA::ShaderLanguageEXT::GlslEs, CNA::ShaderStageEXT::Vertex,
                    "main", "monkey/skin.es.vert", MonkeySkinningShader::Source(true, true)),
                ShaderCodeEXT(CNA::ShaderLanguageEXT::GlslEs, CNA::ShaderStageEXT::Fragment,
                    "main", "monkey/skin.es.frag", MonkeySkinningShader::Source(true, false)),
                ShaderCodeEXT(CNA::ShaderLanguageEXT::GlslDesktop, CNA::ShaderStageEXT::Vertex,
                    "main", "monkey/skin.vert", MonkeySkinningShader::Source(false, true)),
                ShaderCodeEXT(CNA::ShaderLanguageEXT::GlslDesktop, CNA::ShaderStageEXT::Fragment,
                    "main", "monkey/skin.frag", MonkeySkinningShader::Source(false, false))},
                {CNA::ShaderStageEXT::Vertex, CNA::ShaderStageEXT::Fragment});
        }

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
        static_assert(static_cast<std::size_t>(TailTip) + 1U == BoneCount);

        enum class AnimationKind : std::uint8_t
        {
            Idle,
            Observe,
            Walk,
            Run
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

        struct SilhouetteParameters
        {
            float width;
            float headSize;
            float limbThickness;
            float tailLength;
        };

        const std::array<SilhouetteParameters, MonkeyRenderer::SilhouetteCount>
            SilhouetteParametersByVariant = {{{1.0f, 1.0f, 1.0f, 1.0f},
                                              {0.90f, 0.96f, 0.90f, 1.10f},
                                              {1.10f, 1.04f, 1.10f, 0.94f}}};

        Vector3 ToVector(const MonkeyGroundGait::Point& point)
        {
            return Vector3(point.x, point.y, point.z);
        }

        [[nodiscard]] RigDefinition CreateRig()
        {
            const auto rest = MonkeyGroundGait::Sample(0.0f, 0.0f);
            return RigDefinition{
                .heads = {
                    Vector3(0.0f, 0.57f, -0.34f),
                    Vector3(0.0f, 0.63f, 0.12f),
                    Vector3(0.0f, 0.79f, 0.39f),
                    ToVector(rest.limbs[0].root), ToVector(rest.limbs[0].joint),
                    ToVector(rest.limbs[1].root), ToVector(rest.limbs[1].joint),
                    ToVector(rest.limbs[2].root), ToVector(rest.limbs[2].joint),
                    ToVector(rest.limbs[3].root), ToVector(rest.limbs[3].joint),
                    Vector3(0.0f, 0.62f, -0.52f),
                    Vector3(0.0f, 0.70f, -0.91f),
                    Vector3(0.0f, 0.86f, -1.25f)},
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

        void ValidateSkinningDataOrThrow(const SkinningData& data)
        {
            if (data.BoneCount != static_cast<int>(BoneCount)
                || data.SkeletonHierarchy.size() != BoneCount
                || data.BindPose.size() != BoneCount
                || data.InverseBindPose.size() != BoneCount
                || (!data.SkeletonRootPrefix.empty()
                    && data.SkeletonRootPrefix.size() != BoneCount)) {
                throw std::runtime_error(
                    "procedural monkey skeleton has inconsistent 14-bone palette sizes");
            }
            for (std::size_t index = 0; index < BoneCount; ++index) {
                const int parent = data.SkeletonHierarchy[index];
                if ((index == 0U && parent != -1)
                    || (index > 0U
                        && (parent < 0 || parent >= static_cast<int>(index)))) {
                    throw std::runtime_error(
                        "procedural monkey skeleton is not in parent-before-child order");
                }
            }

            constexpr std::array<std::string_view, 4> RequiredClips = {
                "idle", "observe", "walk", "run"};
            for (const std::string_view name : RequiredClips) {
                const auto clipIterator = data.AnimationClips.find(std::string(name));
                if (clipIterator == data.AnimationClips.end()) {
                    throw std::runtime_error(
                        "procedural monkey skeleton is missing animation clip "
                        + std::string(name));
                }
                const AnimationClip& clip = clipIterator->second;
                const double duration = clip.Duration.getTotalSecondsProperty();
                if (!std::isfinite(duration) || duration <= 0.0
                    || clip.TargetSpace != ClipTargetSpaceEXT::JointPalette
                    || clip.Tracks.empty()) {
                    throw std::runtime_error(
                        "procedural monkey animation clip metadata is invalid: "
                        + std::string(name));
                }
                for (const BoneTrackEXT& track : clip.Tracks) {
                    if (track.BoneIndex < 0
                        || track.BoneIndex >= static_cast<int>(BoneCount)
                        || track.Keys.empty()) {
                        throw std::runtime_error(
                            "procedural monkey animation track has an invalid palette index");
                    }
                    double previousSeconds = -1.0;
                    for (const KeyframeEXT& key : track.Keys) {
                        const double seconds = key.Time.getTotalSecondsProperty();
                        const bool transformIsFinite =
                            std::isfinite(key.Translation.X)
                            && std::isfinite(key.Translation.Y)
                            && std::isfinite(key.Translation.Z)
                            && std::isfinite(key.Rotation.X)
                            && std::isfinite(key.Rotation.Y)
                            && std::isfinite(key.Rotation.Z)
                            && std::isfinite(key.Rotation.W)
                            && std::isfinite(key.Scale.X)
                            && std::isfinite(key.Scale.Y)
                            && std::isfinite(key.Scale.Z);
                        if (!std::isfinite(seconds) || seconds < previousSeconds
                            || seconds < 0.0 || seconds > duration + 0.000001
                            || !transformIsFinite) {
                            throw std::runtime_error(
                                "procedural monkey animation keyframes are invalid");
                        }
                        previousSeconds = seconds;
                    }
                }
            }
        }

        void AddVertex(MeshData& mesh, const Vector3& position, const Vector3& normal,
                       const int bone, const Vector2& uv)
        {
            if (bone < 0 || static_cast<std::size_t>(bone) >= BoneCount)
                throw std::invalid_argument("monkey vertex exceeds the 14-bone shader palette");
            mesh.vertices.emplace_back(position, normal, uv,
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
                    AddVertex(mesh, position, normal, bone,
                        Vector2(0.5f + 0.4f * local.X, 0.5f - 0.4f * local.Y));
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
                    AddVertex(mesh, centre + normal * radius, normal, bone,
                        Vector2(0.5f + 0.4f * std::cos(angle), 0.1f + 0.8f * end));
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

        struct BodySection
        {
            Vector3 centre;
            float radiusX, radiusY;
            int firstBone, secondBone;
            float secondWeight;
        };

        // A longitudinal, smoothly shaded skin surface rather than stacked torso balls.
        void AddBodyLoft(MeshData& mesh, const std::span<const BodySection> sections,
                         const int segments)
        {
            const auto base = static_cast<std::uint16_t>(mesh.vertices.size());
            for (std::size_t ring = 0; ring < sections.size(); ++ring) {
                const auto& section = sections[ring];
                const auto& before = sections[ring == 0 ? 0 : ring - 1];
                const auto& after = sections[std::min(ring + 1, sections.size() - 1)];
                for (int segment = 0; segment <= segments; ++segment) {
                    const float angle = MathHelper::TwoPi * static_cast<float>(segment) / segments;
                    const float c = std::cos(angle), s = std::sin(angle);
                    const Vector3 radial(section.radiusX * c, section.radiusY * s, 0.0f);
                    const Vector3 tangent(-section.radiusX * s, section.radiusY * c, 0.0f);
                    const Vector3 longitudinal = after.centre - before.centre
                        + Vector3((after.radiusX - before.radiusX) * c,
                                  (after.radiusY - before.radiusY) * s, 0.0f);
                    Vector3 normal = Vector3::Cross(tangent, longitudinal);
                    if (Vector3::Dot(normal, radial) < 0.0f) normal *= -1.0f;
                    normal.Normalize();
                    // Interior-only mirrored window: no unverified repeat seam. V follows
                    // the spine from neck to tail and stays attached to the rest mesh.
                    AddVertex(mesh, section.centre + radial, normal, section.firstBone,
                        Vector2(0.5f + 0.4f * c,
                            0.9f - 0.8f * static_cast<float>(ring) / (sections.size() - 1)));
                    auto& vertex = mesh.vertices.back();
                    vertex.BlendWeight = Vector4(1.0f - section.secondWeight,
                                                  section.secondWeight, 0.0f, 0.0f);
                    vertex.BlendIndices[1] = static_cast<std::uint8_t>(section.secondBone);
                }
            }
            const int stride = segments + 1;
            for (std::size_t ring = 1; ring < sections.size(); ++ring)
                for (int segment = 0; segment < segments; ++segment) {
                    const auto a = static_cast<std::uint16_t>(base + (ring - 1) * stride + segment);
                    const auto b = static_cast<std::uint16_t>(a + stride);
                    mesh.indices.insert(mesh.indices.end(), {a, b, static_cast<std::uint16_t>(b + 1),
                        a, static_cast<std::uint16_t>(b + 1), static_cast<std::uint16_t>(a + 1)});
                }
        }

        void AddLimbSkin(MeshData& mesh, const MonkeyGroundGait::Limb& limb,
                         int upperBone, int lowerBone, float thickness, int segments)
        {
            const auto base = static_cast<std::uint16_t>(mesh.vertices.size());
            constexpr std::array<float, 9> Radii{0.105f, 0.097f, 0.080f, 0.064f,
                                                0.055f, 0.048f, 0.042f, 0.037f, 0.034f};
            const Vector3 root = ToVector(limb.root), joint = ToVector(limb.joint);
            const Vector3 sole = ToVector(limb.sole);
            for (std::size_t ring = 0; ring < Radii.size(); ++ring) {
                const float t = static_cast<float>(ring) / 8.0f;
                const Vector3 centre = t < 0.5f ? Vector3::Lerp(root, joint, t * 2.0f)
                                                : Vector3::Lerp(joint, sole, t * 2.0f - 1.0f);
                const Vector3 axis = Vector3::Normalize(t < 0.5f ? joint - root : sole - joint);
                const Vector3 across = Vector3::Right;
                const Vector3 around = Vector3::Normalize(Vector3::Cross(axis, across));
                const float blend = std::clamp((t - 0.375f) * 4.0f, 0.0f, 1.0f);
                for (int segment = 0; segment <= segments; ++segment) {
                    const float angle = MathHelper::TwoPi * static_cast<float>(segment) / segments;
                    const Vector3 normal = across * std::cos(angle) + around * std::sin(angle);
                    AddVertex(mesh, centre + normal * Radii[ring] * thickness, normal, upperBone,
                        Vector2(0.5f + 0.4f * std::cos(angle), 0.1f + 0.8f * t));
                    mesh.vertices.back().BlendWeight = Vector4(1.0f - blend, blend, 0.0f, 0.0f);
                    mesh.vertices.back().BlendIndices[1] = static_cast<std::uint8_t>(lowerBone);
                }
            }
            const int stride = segments + 1;
            for (int ring = 0; ring < 8; ++ring)
                for (int segment = 0; segment < segments; ++segment) {
                    const auto a = static_cast<std::uint16_t>(base + ring * stride + segment);
                    const auto b = static_cast<std::uint16_t>(a + stride);
                    mesh.indices.insert(mesh.indices.end(), {a, b, static_cast<std::uint16_t>(b + 1),
                        a, static_cast<std::uint16_t>(b + 1), static_cast<std::uint16_t>(a + 1)});
                }
        }

        [[nodiscard]] MeshData CreateMonkeyMesh(const RigDefinition& rig, const bool near,
                                                const std::size_t silhouetteIndex)
        {
            MeshData mesh;
            const auto& shape = SilhouetteParametersByVariant[silhouetteIndex];
            const int segments = near ? 14 : 8;
            const float w = shape.width;
            const std::array<BodySection, 9> body{{
                {Vector3(0, .58f, -.57f), .008f, .015f, Pelvis, Spine, 0},
                {Vector3(0, .59f, -.48f), .15f*w, .17f, Pelvis, Spine, 0},
                {Vector3(0, .61f, -.31f), .205f*w, .205f, Pelvis, Spine, .15f},
                {Vector3(0, .62f, -.10f), .21f*w, .21f, Pelvis, Spine, .55f},
                {Vector3(0, .63f, .10f), .20f*w, .215f, Pelvis, Spine, 1},
                {Vector3(0, .67f, .27f), .175f*w, .20f, Spine, Head, .05f},
                {Vector3(0, .74f, .36f), .13f*w, .17f, Spine, Head, .45f},
                {Vector3(0, .80f, .45f), .105f*w, .13f, Spine, Head, 1},
                {Vector3(0, .82f, .51f), .008f, .015f, Spine, Head, 1}}};
            AddBodyLoft(mesh, body, segments);
            AddEllipsoid(mesh, Vector3(0, .85f, .46f),
                         Vector3(.15f, .16f, .165f) * shape.headSize, Head, near ? 8 : 4, segments);
            for (float side : {-1.0f, 1.0f})
                AddEllipsoid(mesh, Vector3(side * .147f, .84f, .425f),
                             Vector3(.042f, .057f, .023f), Head, 4, near ? 10 : 6);
            const auto rest = MonkeyGroundGait::Sample(0, 0);
            constexpr std::array<int, 4> upper{UpperArmLeft, UpperArmRight, ThighLeft, ThighRight};
            constexpr std::array<int, 4> lower{ForearmLeft, ForearmRight, ShinLeft, ShinRight};
            for (std::size_t limb = 0; limb < rest.limbs.size(); ++limb) {
                AddLimbSkin(mesh, rest.limbs[limb], upper[limb], lower[limb],
                            shape.limbThickness, near ? 10 : 6);
                const Vector3 sole = ToVector(rest.limbs[limb].sole);
                AddEllipsoid(mesh, sole + Vector3(0, 0, .027f),
                             Vector3(.048f, .031f, .076f), lower[limb], 3, near ? 10 : 6);
                if (near)
                    for (int digit = 0; digit < 4; ++digit)
                        AddEllipsoid(mesh, sole + Vector3((digit - 1.5f) * .021f, -.006f, .095f),
                                     Vector3(.010f, .012f, .043f - .003f * std::abs(digit - 1)),
                                     lower[limb], 3, 6);
            }
            AddTube(mesh, rig.heads[TailBase], rig.heads[TailMiddle], .036f, TailBase, segments);
            AddTube(mesh, rig.heads[TailMiddle], rig.heads[TailTip], .028f, TailMiddle, segments);
            AddTube(mesh, rig.heads[TailTip], Vector3(0, .97f, -1.55f * shape.tailLength),
                    .017f, TailTip, segments);
            return mesh;
        }

        [[nodiscard]] MeshData CreateFaceMesh()
        {
            MeshData mesh;
            // Narrow orbital skin and a projecting, low muzzle, not a teddy-bear mask.
            AddEllipsoid(mesh, Vector3(0, .86f, .605f), Vector3(.108f, .065f, .027f), Head, 6, 12);
            AddEllipsoid(mesh, Vector3(0, .787f, .628f), Vector3(.077f, .052f, .069f), Head, 6, 12);
            for (float side : {-1.0f, 1.0f})
                AddEllipsoid(mesh, Vector3(side * .151f, .84f, .445f),
                             Vector3(.026f, .037f, .013f), Head, 4, 8);
            return mesh;
        }

        [[nodiscard]] MeshData CreateEyeMesh()
        {
            MeshData mesh;
            for (float side : {-1.0f, 1.0f}) {
                AddEllipsoid(mesh, Vector3(side * .058f, .864f, .629f),
                             Vector3(.019f, .013f, .010f), Head, 4, 10);
                AddEllipsoid(mesh, Vector3(side * .018f, .800f, .694f),
                             Vector3(.009f, .006f, .004f), Head, 3, 6);
            }
            AddEllipsoid(mesh, Vector3(0, .762f, .679f),
                         Vector3(.046f, .003f, .006f), Head, 3, 10);
            return mesh;
        }

        [[nodiscard]] AnimationKind ClipForBehavior(const MonkeyBehavior behavior)
        {
            switch (behavior) {
            case MonkeyBehavior::Observe:
                return AnimationKind::Observe;
            case MonkeyBehavior::Walk:
            case MonkeyBehavior::Wander:
                return AnimationKind::Walk;
            case MonkeyBehavior::RunAway:
                return AnimationKind::Run;
            case MonkeyBehavior::Idle:
                return AnimationKind::Idle;
            }
            return AnimationKind::Idle;
        }

        [[nodiscard]] const char* ClipName(const AnimationKind kind)
        {
            switch (kind) {
            case AnimationKind::Observe:
                return "observe";
            case AnimationKind::Walk:
                return "walk";
            case AnimationKind::Run:
                return "run";
            case AnimationKind::Idle:
                return "idle";
            }
            return "idle";
        }

        [[nodiscard]] float CrossfadeDuration(const AnimationKind from,
                                              const AnimationKind to) noexcept
        {
            if (from == AnimationKind::Run || to == AnimationKind::Run) {
                return 0.14f;
            }
            if (from == AnimationKind::Walk || to == AnimationKind::Walk) {
                return 0.20f;
            }
            return 0.26f;
        }

        [[nodiscard]] Matrix BlendLocalTransform(const Matrix& from, const Matrix& to,
                                                 const float amount)
        {
            Vector3 fromScale;
            Vector3 fromTranslation;
            Quaternion fromRotation;
            Vector3 toScale;
            Vector3 toTranslation;
            Quaternion toRotation;
            if (!from.Decompose(fromScale, fromRotation, fromTranslation)
                || !to.Decompose(toScale, toRotation, toTranslation)) {
                return Matrix::Lerp(from, to, amount);
            }
            return Matrix::CreateScale(Vector3::Lerp(fromScale, toScale, amount))
                * Matrix::CreateFromQuaternion(
                    Quaternion::Slerp(fromRotation, toRotation, amount))
                * Matrix::CreateTranslation(
                    Vector3::Lerp(fromTranslation, toTranslation, amount));
        }

        void BuildBlendedLocalTransforms(const SkinningData& data,
                                         const std::span<const Matrix> source,
                                         const std::span<const Matrix> target,
                                         const float amount,
                                         std::vector<Matrix>& local)
        {
            const std::size_t boneCount = static_cast<std::size_t>(data.BoneCount);
            assert(source.size() == boneCount && target.size() == boneCount);
            assert(std::isfinite(amount) && amount >= 0.0f && amount <= 1.0f);
            local.resize(boneCount);
            const float eased = amount * amount * (3.0f - 2.0f * amount);
            for (std::size_t index = 0; index < boneCount; ++index) {
                local[index] = BlendLocalTransform(source[index], target[index], eased);
            }
        }

        void BuildSkinPalette(const SkinningData& data,
                              const std::span<const Matrix> local,
                              std::vector<Matrix>& world,
                              std::vector<Matrix>& skin)
        {
            const std::size_t boneCount = static_cast<std::size_t>(data.BoneCount);
            assert(local.size() == boneCount);
            assert(data.SkeletonHierarchy.size() == boneCount
                   && data.InverseBindPose.size() == boneCount);
            world.resize(boneCount);
            skin.resize(boneCount);
            const bool hasPrefix = data.SkeletonRootPrefix.size() == boneCount;
            for (std::size_t index = 0; index < boneCount; ++index) {
                const int parent = data.SkeletonHierarchy[index];
                world[index] = parent < 0
                    ? (hasPrefix ? local[index] * data.SkeletonRootPrefix[index]
                                 : local[index])
                    : local[index] * world[static_cast<std::size_t>(parent)];
                skin[index] = data.InverseBindPose[index] * world[index];
            }
        }

        const std::array<Vector3, 4> MonkeyColors = {
            Vector3(0.34f, 0.23f, 0.15f),
            Vector3(0.43f, 0.30f, 0.19f),
            Vector3(0.29f, 0.21f, 0.16f),
            Vector3(0.50f, 0.35f, 0.22f)};
        const std::array<Vector3, 4> FaceColors = {
            Vector3(0.68f, 0.51f, 0.34f),
            Vector3(0.76f, 0.59f, 0.39f),
            Vector3(0.60f, 0.46f, 0.33f),
            Vector3(0.80f, 0.63f, 0.42f)};
        const Vector3 FurSpecularColor(0.04f, 0.04f, 0.035f);
        const Vector3 EyeSpecularColor(0.42f, 0.37f, 0.28f);
        constexpr float FurSpecularPower = 8.0f;
        constexpr float EyeSpecularPower = 28.0f;
    }

    struct MonkeyRenderer::PlaybackState
    {
        std::unique_ptr<AnimationPlayer> player;
        AnimationKind activeClip = AnimationKind::Idle;
        float lastAnimationSeconds = 0.0f;
        float crossfadeStartSeconds = 0.0f;
        float crossfadeDuration = 0.0f;
        bool crossfading = false;
        std::vector<Matrix> sourceLocalTransforms;
        std::vector<Matrix> presentedLocalTransforms;
        std::vector<Matrix> blendedWorldTransforms;
        std::vector<Matrix> blendedSkinTransforms;
        std::vector<Matrix> attentiveLocalTransforms;
        std::vector<Matrix> attentiveWorldTransforms;
        std::vector<Matrix> attentiveSkinTransforms;
    };

    MonkeyRenderer::MonkeyRenderer(GraphicsDevice& device, const CanopyLightField& canopyLight,
                                   const float maximumDistance)
        : device_(device), canopyLight_(canopyLight),
          whiteTexture_(std::make_unique<Texture2D>(
              device, GeneratedTextureBudget::SolidWhiteTextureSize,
              GeneratedTextureBudget::SolidWhiteTextureSize)),
          effect_(std::make_unique<SkinnedEffect>(device)),
          maximumDistance_(maximumDistance)
    {
        if (!std::isfinite(maximumDistance_) || maximumDistance_ <= 0.0f) {
            throw std::invalid_argument("Monkey maximum draw distance must be finite and positive");
        }
        const Color whitePixel = Color::White;
        whiteTexture_->SetData(&whitePixel, 1);
        // Required material: a missing/invalid asset must not silently become plain brown.
        const auto furLevels = LoadOpaqueImageMips(device_, "Content/wildlife/monkey-fur.png", 512);
        for (std::size_t c = 0; c < furMean_.size(); ++c) {
            furMean_[c] = furLevels.back().pixels[0][c] / 255.0f;
            if (furMean_[c] < 0.01f) throw std::runtime_error("Monkey fur mean is too dark");
        }
        furTexture_ = std::make_unique<Texture2D>(device_, 512, 512, true, SurfaceFormat::Color);
        for (std::size_t level = 0; level < furLevels.size(); ++level) {
            const auto& pixels = furLevels[level].pixels;
            furTexture_->SetData(static_cast<int>(level), nullptr,
                reinterpret_cast<const std::uint8_t*>(pixels.data()), 0,
                static_cast<int>(pixels.size() * 4));
        }
        const char* furDiagnostic = "island-simulator: monkey fur loaded Content/wildlife/monkey-fur.png; 512x512, 10 mips, 1398100 RGBA8 bytes";
        std::cout << furDiagnostic << '\n';
        MirrorRuntimeDiagnosticToPlatformLog(furDiagnostic);
        const RigDefinition rig = CreateRig();
        skinningData_ = CreateSkinningData(rig);
        ValidateSkinningDataOrThrow(*skinningData_);
        MeshData faceMesh = CreateFaceMesh();
        MeshData eyeMesh = CreateEyeMesh();
        ValidateProceduralTriangleMeshOrThrow(
            faceMesh.vertices.size(), faceMesh.indices, "monkey face");
        ValidateProceduralTriangleMeshOrThrow(
            eyeMesh.vertices.size(), eyeMesh.indices, "monkey eye");
        faceVertexCount_ = static_cast<int>(faceMesh.vertices.size());
        facePrimitiveCount_ = static_cast<int>(faceMesh.indices.size() / 3U);
        eyeVertexCount_ = static_cast<int>(eyeMesh.vertices.size());
        eyePrimitiveCount_ = static_cast<int>(eyeMesh.indices.size() / 3U);

        for (std::size_t variant = 0; variant < SilhouetteCount; ++variant) {
            MeshData nearMesh = CreateMonkeyMesh(rig, true, variant);
            MeshData farMesh = CreateMonkeyMesh(rig, false, variant);
            ValidateProceduralTriangleMeshOrThrow(
                nearMesh.vertices.size(), nearMesh.indices, "monkey near LOD");
            ValidateProceduralTriangleMeshOrThrow(
                farMesh.vertices.size(), farMesh.indices, "monkey far LOD");
            nearVertexCounts_[variant] = static_cast<int>(nearMesh.vertices.size());
            nearPrimitiveCounts_[variant] = static_cast<int>(nearMesh.indices.size() / 3U);
            farVertexCounts_[variant] = static_cast<int>(farMesh.vertices.size());
            farPrimitiveCounts_[variant] = static_cast<int>(farMesh.indices.size() / 3U);

            nearVertices_[variant] = std::make_unique<VertexBuffer>(
                device_, SkinnedVertex::getVertexDeclarationStatic(), nearVertexCounts_[variant],
                BufferUsage::WriteOnly);
            nearVertices_[variant]->SetData(
                nearMesh.vertices.data(), nearVertexCounts_[variant]);
            nearIndices_[variant] = std::make_unique<IndexBuffer>(
                device_, IndexElementSize::SixteenBits,
                static_cast<int>(nearMesh.indices.size()), BufferUsage::WriteOnly);
            nearIndices_[variant]->SetData(
                nearMesh.indices.data(), static_cast<int>(nearMesh.indices.size()));

            farVertices_[variant] = std::make_unique<VertexBuffer>(
                device_, SkinnedVertex::getVertexDeclarationStatic(), farVertexCounts_[variant],
                BufferUsage::WriteOnly);
            farVertices_[variant]->SetData(
                farMesh.vertices.data(), farVertexCounts_[variant]);
            farIndices_[variant] = std::make_unique<IndexBuffer>(
                device_, IndexElementSize::SixteenBits,
                static_cast<int>(farMesh.indices.size()), BufferUsage::WriteOnly);
            farIndices_[variant]->SetData(
                farMesh.indices.data(), static_cast<int>(farMesh.indices.size()));
        }

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
        effect_->setSpecularColorProperty(FurSpecularColor);
        effect_->setSpecularPowerProperty(FurSpecularPower);
        effect_->setFogEnabledProperty(true);
        effect_->setFogColorProperty(Vector3(environment::FogColorRed, environment::FogColorGreen, environment::FogColorBlue));
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
        paletteEffect_ = std::make_unique<ShaderEffect>(device_, MonkeyShaderPackage());
        if (!paletteEffect_->IsEffectValid()) {
            throw std::runtime_error("monkey 14-bone shader failed: "
                                     + paletteEffect_->GetCompileErrorEXT());
        }
        constexpr const char* canopyDiagnostic =
            "island-simulator: monkeys receive footprint-averaged canopy light through 14-bone skinning";
        std::cout << canopyDiagnostic << '\n';
        MirrorRuntimeDiagnosticToPlatformLog(canopyDiagnostic);

        playbackStates_.reserve(MonkeyTroop::MonkeyCount);
        const AnimationClip& idle = skinningData_->AnimationClips.at("idle");
        for (std::size_t index = 0; index < MonkeyTroop::MonkeyCount; ++index) {
            auto playback = std::make_unique<PlaybackState>();
            playback->player = std::make_unique<AnimationPlayer>(*skinningData_);
            playback->player->StartClip(idle);
            playback->presentedLocalTransforms = playback->player->GetBoneTransforms();
            playbackStates_.push_back(std::move(playback));
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
        activeCrossfadeCount_ = 0;
        minimumCanopyTransmission_ = 1.0f;
        maximumCanopyTransmission_ = 0.0f;
        const BoundingFrustum frustum(view * projection);
        effect_->setViewProperty(view);
        effect_->setProjectionProperty(projection);
        float viewValues[16], viewProjectionValues[16];
        view.ToColumnMajor(viewValues);
        (view * projection).ToColumnMajor(viewProjectionValues);
        paletteEffect_->Apply();
        paletteEffect_->SetUniformMat4("uView", viewValues);
        paletteEffect_->SetUniformMat4("uViewProjection", viewProjectionValues);
        paletteEffect_->SetUniformVec3("uCameraPosition", cameraPosition.X, cameraPosition.Y, cameraPosition.Z);
        paletteEffect_->SetUniformVec3("uSunDirection", environment::SunDirectionX,
                                      environment::SunDirectionY, environment::SunDirectionZ);
        paletteEffect_->SetUniformVec3("uFogColor", environment::FogColorRed,
                                      environment::FogColorGreen, environment::FogColorBlue);
        // Keep SkinnedEffect only as the authored material state; never apply its 72-bone program.
        const auto applyMaterial = [&](float furAmount) {
            float worldValues[16];
            effect_->getWorldProperty().ToColumnMajor(worldValues);
            const auto diffuse = effect_->getDiffuseColorProperty();
            const auto specular = effect_->getSpecularColorProperty();
            const auto lightSpecular = effect_->getDirectionalLight0Property().getSpecularColorProperty();
            paletteEffect_->Apply();
            device_.getSamplerStatesProperty()[0] = SamplerState::LinearClamp;
            paletteEffect_->SetTexture(0, *furTexture_);
            paletteEffect_->SetUniformInt("uFurTexture", 0);
            paletteEffect_->SetUniformVec3("uFurMean", furMean_[0], furMean_[1], furMean_[2]);
            paletteEffect_->SetUniformFloat("uFurAmount", furAmount);
            paletteEffect_->SetUniformMat4("uWorld", worldValues);
            paletteEffect_->SetUniformVec3("uDiffuse", diffuse.X, diffuse.Y, diffuse.Z);
            paletteEffect_->SetUniformVec3("uSpecular", specular.X, specular.Y, specular.Z);
            paletteEffect_->SetUniformVec3("uLightSpecular", lightSpecular.X, lightSpecular.Y, lightSpecular.Z);
            paletteEffect_->SetUniformFloat("uSpecularPower", effect_->getSpecularPowerProperty());
        };
        device_.setRasterizerStateProperty(RasterizerState::CullNone);
        device_.setBlendStateProperty(BlendState::Opaque);
        device_.setDepthStencilStateProperty(DepthStencilState::Default);

        const std::size_t count = std::min(monkeys.size(), playbackStates_.size());
        for (std::size_t index = 0; index < count; ++index) {
            const MonkeyState& monkey = monkeys[index];
            PlaybackState& playback = *playbackStates_[index];
            const AnimationKind requestedClip = ClipForBehavior(monkey.behavior);
            if (playback.activeClip != requestedClip) {
                playback.sourceLocalTransforms = playback.presentedLocalTransforms;
                playback.crossfadeStartSeconds = monkey.animationSeconds;
                playback.crossfadeDuration = CrossfadeDuration(
                    playback.activeClip, requestedClip);
                playback.crossfading = true;
                playback.activeClip = requestedClip;
                playback.player->StartClip(
                    skinningData_->AnimationClips.at(ClipName(requestedClip)));
                playback.lastAnimationSeconds = monkey.animationSeconds;
            } else {
                const float elapsed = std::max(
                    0.0f, monkey.animationSeconds - playback.lastAnimationSeconds);
                if (elapsed > 0.0f) {
                    playback.player->Update(System::TimeSpan::FromSeconds(elapsed), true, true);
                }
                playback.lastAnimationSeconds = monkey.animationSeconds;
            }

            const std::vector<Matrix>* skinTransforms = &playback.player->GetSkinTransforms();
            assert(playback.player->GetBoneTransforms().size() == BoneCount);
            assert(skinTransforms->size() == BoneCount);
            if (playback.crossfading) {
                const float amount = std::clamp(
                    (monkey.animationSeconds - playback.crossfadeStartSeconds)
                        / playback.crossfadeDuration,
                    0.0f, 1.0f);
                if (amount < 1.0f) {
                    BuildBlendedLocalTransforms(
                        *skinningData_, playback.sourceLocalTransforms,
                        playback.player->GetBoneTransforms(), amount,
                        playback.presentedLocalTransforms);
                    BuildSkinPalette(*skinningData_, playback.presentedLocalTransforms,
                                     playback.blendedWorldTransforms,
                                     playback.blendedSkinTransforms);
                    skinTransforms = &playback.blendedSkinTransforms;
                    ++activeCrossfadeCount_;
                } else {
                    playback.crossfading = false;
                    playback.presentedLocalTransforms = playback.player->GetBoneTransforms();
                    ++completedCrossfadeCount_;
                }
            } else {
                playback.presentedLocalTransforms = playback.player->GetBoneTransforms();
            }

            {
                playback.attentiveLocalTransforms = playback.presentedLocalTransforms;
                static const RigDefinition groundRig = CreateRig();
                static const auto restPose = MonkeyGroundGait::Sample(0.0f, 0.0f);
                // The horizontal trunk supports four limbs; do not inherit the former
                // upright-walk pelvis bob/spine lean. Head/tail retain bounded animation.
                playback.attentiveLocalTransforms[Pelvis] = Matrix::CreateTranslation(
                    RestTranslation(groundRig, Pelvis));
                playback.attentiveLocalTransforms[Spine] = Matrix::CreateTranslation(
                    RestTranslation(groundRig, Spine));
                playback.attentiveLocalTransforms[Head]
                    = Matrix::CreateRotationY(-monkey.headLookRadians)
                        * playback.attentiveLocalTransforms[Head];
                BuildSkinPalette(*skinningData_, playback.attentiveLocalTransforms,
                                 playback.attentiveWorldTransforms,
                                 playback.attentiveSkinTransforms);
                const auto pose = MonkeyGroundGait::Sample(monkey.gaitPhase, monkey.gaitAmount);
                constexpr std::array<int, 4> upper{UpperArmLeft, UpperArmRight, ThighLeft, ThighRight};
                constexpr std::array<int, 4> lower{ForearmLeft, ForearmRight, ShinLeft, ShinRight};
                const auto applySegment = [&](int bone, const Vector3& restDirection,
                                               const Vector3& direction, const Vector3& origin) {
                    const float angle = std::atan2(direction.Z, direction.Y)
                        - std::atan2(restDirection.Z, restDirection.Y);
                    playback.attentiveWorldTransforms[bone] = Matrix::CreateRotationX(angle)
                        * Matrix::CreateTranslation(origin);
                    playback.attentiveSkinTransforms[bone] = skinningData_->InverseBindPose[bone]
                        * playback.attentiveWorldTransforms[bone];
                };
                for (std::size_t limb = 0; limb < pose.limbs.size(); ++limb) {
                    const auto& rest = restPose.limbs[limb];
                    const auto& current = pose.limbs[limb];
                    applySegment(upper[limb], ToVector(rest.joint) - ToVector(rest.root),
                        ToVector(current.joint) - ToVector(current.root), ToVector(current.root));
                    applySegment(lower[limb], ToVector(rest.sole) - ToVector(rest.joint),
                        ToVector(current.sole) - ToVector(current.joint), ToVector(current.joint));
                }
                skinTransforms = &playback.attentiveSkinTransforms;
            }
            assert(skinTransforms->size() == BoneCount);

            const Vector3 centre(monkey.x, monkey.y + 0.72f * monkey.scale, monkey.z);
            const float distanceSquared = Vector3::DistanceSquared(centre, cameraPosition);
            if (distanceSquared > maximumDistance_ * maximumDistance_
                || !frustum.Intersects(BoundingSphere(centre, 1.9f * monkey.scale))) {
                continue;
            }

            std::array<float, BoneCount * 16> palette{};
            for (std::size_t bone = 0; bone < BoneCount; ++bone)
                (*skinTransforms)[bone].ToColumnMajor(palette.data() + bone * 16);
            paletteEffect_->SetUniformMat4Array("uBones", palette.data(), static_cast<int>(BoneCount));
            // Spatial averaging is pose/LOD independent: no camera-driven brightness switches.
            const float transmission = canopyLight_.SampleReceiverFootprint(
                centre.X, centre.Y, centre.Z, monkey.y, 0.35f * monkey.scale);
            const auto tint = CanopyTransmissionTint(transmission);
            minimumCanopyTransmission_ = std::min(minimumCanopyTransmission_, transmission);
            maximumCanopyTransmission_ = std::max(maximumCanopyTransmission_, transmission);
            if (!reportedCanopyShade_ && transmission < 0.95f) {
                std::ostringstream message;
                message << "island-simulator: visible monkey entered canopy shade, transmission "
                        << transmission;
                std::cout << message.str() << '\n';
                MirrorRuntimeDiagnosticToPlatformLog(message.str().c_str());
                reportedCanopyShade_ = true;
            }
            const auto shaded = [tint](const Vector3& color) {
                return Vector3(color.X * tint.red, color.Y * tint.green, color.Z * tint.blue);
            };
            effect_->getDirectionalLight0Property().setSpecularColorProperty(
                Vector3(0.16f, 0.14f, 0.10f) * transmission);
            effect_->setDiffuseColorProperty(
                shaded(MonkeyColors[monkey.colorVariant % MonkeyColors.size()]));
            const float yaw = std::numbers::pi_v<float> * 0.5f - monkey.headingRadians;
            effect_->setWorldProperty(Matrix::CreateScale(monkey.scale)
                * Matrix::CreateRotationY(yaw)
                * Matrix::CreateTranslation(monkey.x, monkey.y, monkey.z));
            static bool reportedPaletteProbe = false;
            if (!reportedPaletteProbe && index == 0) {
                const auto probe = Vector3::Transform(Vector3(0.0f, 0.56f, 0.0f),
                    (*skinTransforms)[0] * effect_->getWorldProperty());
                std::ostringstream report;
                report << "island-simulator: monkey palette probe root " << monkey.x << ','
                    << monkey.y << ',' << monkey.z << "; skinned pelvis " << probe.X << ','
                    << probe.Y << ',' << probe.Z << "; bone M44 " << (*skinTransforms)[0].M44;
                std::cout << report.str() << '\n';
                MirrorRuntimeDiagnosticToPlatformLog(report.str());
                reportedPaletteProbe = true;
            }

            const bool near = distanceSquared <= NearLodDistance * NearLodDistance;
            const std::size_t silhouette = monkey.colorVariant % SilhouetteCount;
            VertexBuffer* vertices = near ? nearVertices_[silhouette].get()
                                          : farVertices_[silhouette].get();
            IndexBuffer* indices = near ? nearIndices_[silhouette].get()
                                        : farIndices_[silhouette].get();
            const int vertexCount = near ? nearVertexCounts_[silhouette]
                                         : farVertexCounts_[silhouette];
            const int primitiveCount = near ? nearPrimitiveCounts_[silhouette]
                                            : farPrimitiveCounts_[silhouette];
            device_.SetVertexBuffer(vertices);
            device_.setIndicesProperty(indices);
            applyMaterial(1.0f);
            device_.DrawIndexedPrimitives(PrimitiveType::TriangleList, 0, 0,
                                          vertexCount, 0, primitiveCount);
            if (near) {
                effect_->setDiffuseColorProperty(
                    shaded(FaceColors[monkey.colorVariant % FaceColors.size()]));
                device_.SetVertexBuffer(faceVertices_.get());
                device_.setIndicesProperty(faceIndices_.get());
                applyMaterial(0.0f);
                device_.DrawIndexedPrimitives(PrimitiveType::TriangleList, 0, 0,
                                              faceVertexCount_, 0, facePrimitiveCount_);
                effect_->setDiffuseColorProperty(shaded(Vector3(0.025f, 0.022f, 0.018f)));
                effect_->setSpecularColorProperty(EyeSpecularColor);
                effect_->setSpecularPowerProperty(EyeSpecularPower);
                device_.SetVertexBuffer(eyeVertices_.get());
                device_.setIndicesProperty(eyeIndices_.get());
                applyMaterial(0.0f);
                device_.DrawIndexedPrimitives(PrimitiveType::TriangleList, 0, 0,
                                              eyeVertexCount_, 0, eyePrimitiveCount_);
                effect_->setSpecularColorProperty(FurSpecularColor);
                effect_->setSpecularPowerProperty(FurSpecularPower);
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
