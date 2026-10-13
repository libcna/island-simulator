#include "IslandSimulator/DolphinRenderer.hpp"

#include "IslandSimulator/RuntimeDiagnostics.hpp"
#include "IslandSimulator/WildlifeRenderLighting.hpp"

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
#include <numbers>
#include <stdexcept>

using namespace Microsoft::Xna::Framework;
using namespace Microsoft::Xna::Framework::Graphics;

namespace island {
namespace {
constexpr float NearDistance = 175.0f;
struct Axes {
  Vector3 right, up, forward;
};
struct Section {
  float z, rx, ry;
};
constexpr std::array<Section, 8> Body{{{-.72f, .055f, .055f},
                                       {-.54f, .12f, .11f},
                                       {-.28f, .21f, .18f},
                                       {.06f, .27f, .225f},
                                       {.40f, .245f, .215f},
                                       {.65f, .225f, .205f},
                                       {.82f, .17f, .16f},
                                       {.93f, .09f, .10f}}};
constexpr int Segments = 10;

void Triangle(std::vector<VertexPositionColor> &out, const Vector3 &a,
              const Vector3 &b, const Vector3 &c, const Color &color,
              bool twoSided = false) {
  const auto lit = wildlife_render::FacetColor(a, b, c, color, twoSided);
  out.emplace_back(a, lit);
  out.emplace_back(b, lit);
  out.emplace_back(c, lit);
}

void Quad(std::vector<VertexPositionColor> &out, const Vector3 &a,
          const Vector3 &b, const Vector3 &c, const Vector3 &d,
          const Color &color) {
  Triangle(out, a, b, c, color);
  Triangle(out, a, c, d, color);
}

void ClosedQuad(std::vector<VertexPositionColor> &out, const Vector3 &a,
                const Vector3 &b, const Vector3 &c, const Vector3 &d,
                const Vector3 &halfThickness, const Color &face,
                const Color &edge) {
  const Vector3 ap = a + halfThickness, bp = b + halfThickness,
                cp = c + halfThickness, dp = d + halfThickness;
  const Vector3 am = a - halfThickness, bm = b - halfThickness,
                cm = c - halfThickness, dm = d - halfThickness;
  Quad(out, ap, bp, cp, dp, face);
  Quad(out, am, dm, cm, bm, face);
  Quad(out, ap, am, bm, bp, edge);
  Quad(out, bp, bm, cm, cp, edge);
  Quad(out, cp, cm, dm, dp, edge);
  Quad(out, dp, dm, am, ap, edge);
}

Axes Pose(const DolphinState &dolphin) {
  const Vector3 flat(std::cos(dolphin.headingRadians), 0,
                     std::sin(dolphin.headingRadians));
  const Vector3 right(-flat.Z, 0, flat.X);
  const float cp = std::cos(dolphin.pitchRadians),
              sp = std::sin(dolphin.pitchRadians);
  const Vector3 forward = flat * cp + Vector3::Up * sp;
  const Vector3 pitchUp = Vector3::Up * cp - flat * sp;
  const float cb = std::cos(dolphin.bankRadians),
              sb = std::sin(dolphin.bankRadians);
  return {right * cb + pitchUp * sb, pitchUp * cb - right * sb, forward};
}

Vector3 Point(const Vector3 &centre, const Axes &axes, float x, float y,
              float z, float scale) {
  return centre + axes.right * (x * scale) + axes.up * (y * scale) +
         axes.forward * (z * scale);
}

std::array<Color, 4> Palette(const DolphinState &dolphin, float time) {
  const std::array<std::array<Color, 4>, 3> colors{
      {std::array{Color(77, 104, 119, 255), Color(55, 77, 91, 255),
                  Color(173, 184, 181, 255), Color(20, 28, 33, 255)},
       std::array{Color(88, 108, 116, 255), Color(60, 79, 88, 255),
                  Color(181, 187, 177, 255), Color(18, 27, 31, 255)},
       std::array{Color(67, 94, 111, 255), Color(47, 69, 84, 255),
                  Color(158, 178, 181, 255), Color(17, 25, 30, 255)}}};
  auto result = colors[dolphin.colorVariant % colors.size()];
  const auto light = AquaticLightAt(dolphin.x, dolphin.y, dolphin.z, time);
  for (auto &color : result)
    color = wildlife_render::ApplyUnderwaterLight(color, light);
  return result;
}

void Near(std::vector<VertexPositionColor> &out, const DolphinState &dolphin,
          float time) {
  const auto start = out.size();
  const Vector3 centre(dolphin.x, dolphin.y, dolphin.z);
  const auto axes = Pose(dolphin);
  const auto colors = Palette(dolphin, time);
  const float scale = 1.75f * dolphin.scale;
  std::array<std::array<Vector3, Segments>, Body.size()> rings{};
  for (std::size_t row = 0; row < Body.size(); ++row) {
    const auto &section = Body[row];
    const float rear = std::max(0.0f, -section.z - .08f);
    const float bend = dolphin.tailBeat * .16f * rear * rear;
    for (int segment = 0; segment < Segments; ++segment) {
      const float angle = 2 * std::numbers::pi_v<float> * segment / Segments;
      rings[row][segment] =
          Point(centre, axes, bend + section.rx * std::cos(angle),
                section.ry * std::sin(angle), section.z, scale);
    }
  }
  for (std::size_t row = 0; row + 1 < Body.size(); ++row)
    for (int s = 0; s < Segments; ++s) {
      const int n = (s + 1) % Segments;
      const float angle = 2 * std::numbers::pi_v<float> * (s + .5f) / Segments;
      const Color color =
          std::sin(angle) < -.18f
              ? colors[2]
              : (std::cos(angle) < -.1f ? colors[1] : colors[0]);
      Triangle(out, rings[row][s], rings[row + 1][s], rings[row + 1][n], color);
      Triangle(out, rings[row][s], rings[row + 1][n], rings[row][n], color);
    }
  // A short closed beak beneath the rounded melon avoids a fish-like needle profile.
  constexpr int RostrumSegments = 8;
  std::array<Vector3, RostrumSegments> rostrumBase{};
  std::array<Vector3, RostrumSegments> rostrumTip{};
  for (int segment = 0; segment < RostrumSegments; ++segment) {
    const float angle = 2.0f * std::numbers::pi_v<float>
        * static_cast<float>(segment) / static_cast<float>(RostrumSegments);
    rostrumBase[segment] = Point(centre, axes, .102f * std::cos(angle),
        -.040f + .066f * std::sin(angle), .90f, scale);
    rostrumTip[segment] = Point(centre, axes, .052f * std::cos(angle),
        -.043f + .044f * std::sin(angle), 1.075f, scale);
  }
  const auto rostrumCap = Point(centre, axes, 0, -.043f, 1.105f, scale);
  for (int segment = 0; segment < RostrumSegments; ++segment) {
    const int next = (segment + 1) % RostrumSegments;
    const float angle = 2.0f * std::numbers::pi_v<float>
        * (static_cast<float>(segment) + .5f) / static_cast<float>(RostrumSegments);
    const Color color = std::sin(angle) < -.12f ? colors[2] : colors[0];
    Triangle(out, rostrumBase[segment], rostrumTip[segment], rostrumTip[next], color);
    Triangle(out, rostrumBase[segment], rostrumTip[next], rostrumBase[next], color);
    Triangle(out, rostrumCap, rostrumTip[next], rostrumTip[segment], color);
  }
  // A thin closed swept-back dorsal retains the falcate profile without vanishing
  // edge-on like the former coincident sheet.
  const auto dorsalFront = Point(centre, axes, 0, .205f, .18f, scale);
  const auto dorsalTip = Point(centre, axes, 0, .405f, -.10f, scale);
  const auto dorsalKnee = Point(centre, axes, 0, .285f, -.22f, scale);
  const auto dorsalRear = Point(centre, axes, 0, .15f, -.38f, scale);
  ClosedQuad(out, dorsalFront, dorsalTip, dorsalKnee, dorsalRear,
             axes.right * (.010f * scale), colors[0], colors[1]);
  for (float side : {-1.0f, 1.0f}) {
    const auto finFront = Point(centre, axes, side * .18f, -.04f, .35f, scale);
    const auto finTip = Point(centre, axes, side * .54f, -.14f, .02f, scale);
    const auto finElbow = Point(centre, axes, side * .38f, -.145f, -.10f, scale);
    const auto finRear = Point(centre, axes, side * .13f, -.11f, -.22f, scale);
    ClosedQuad(out, finFront, finTip, finElbow, finRear,
               axes.up * (.009f * scale), colors[0], colors[1]);
  }
  const float tail = dolphin.tailBeat * .12f;
  for (float side : {-1.0f, 1.0f}) {
    const auto root = Point(centre, axes, tail, 0, -.68f, scale);
    const auto leading =
        Point(centre, axes, tail + side * .30f, .02f, -.80f, scale);
    const auto outer =
        Point(centre, axes, tail + side * .50f, .018f, -.91f, scale);
    const auto rear =
        Point(centre, axes, tail + side * .12f, -.012f, -1.05f, scale);
    ClosedQuad(out, root, leading, outer, rear,
               axes.up * (.007f * scale), colors[0], colors[1]);
  }
  const Color eyeShine(126, 148, 151, 255);
  for (float side : {-1.0f, 1.0f}) {
    const auto eyeTop = Point(centre, axes, side * .210f, .085f, .645f, scale);
    const auto eyeFront = Point(centre, axes, side * .217f, .055f, .672f, scale);
    const auto eyeBottom = Point(centre, axes, side * .220f, .025f, .642f, scale);
    const auto eyeRear = Point(centre, axes, side * .216f, .055f, .610f, scale);
    if (side > 0) {
      Triangle(out, eyeTop, eyeBottom, eyeFront, colors[3]);
      Triangle(out, eyeTop, eyeRear, eyeBottom, colors[3]);
    } else {
      Triangle(out, eyeTop, eyeFront, eyeBottom, colors[3]);
      Triangle(out, eyeTop, eyeBottom, eyeRear, colors[3]);
    }
    Triangle(out, Point(centre, axes, side * .222f, .073f, .650f, scale),
             Point(centre, axes, side * .223f, .060f, .660f, scale),
             Point(centre, axes, side * .223f, .059f, .640f, scale), eyeShine);

    const auto mouthFront = Point(centre, axes, side * .030f, -.078f, 1.075f, scale);
    const auto mouthOuter = Point(centre, axes, side * .090f, -.078f, .905f, scale);
    const auto mouthKnee = Point(centre, axes, side * .074f, -.088f, .970f, scale);
    const auto mouthInner = Point(centre, axes, side * .026f, -.087f, 1.045f, scale);
    Triangle(out, mouthFront, mouthOuter, mouthKnee, colors[3]);
    Triangle(out, mouthFront, mouthKnee, mouthInner, colors[3]);
  }
  // A small two-facet dorsal blowhole completes the cetacean head cues.
  const auto blowholeFront = Point(centre, axes, 0, .205f, .745f, scale);
  const auto blowholeRight = Point(centre, axes, .032f, .195f, .700f, scale);
  const auto blowholeRear = Point(centre, axes, 0, .205f, .665f, scale);
  const auto blowholeLeft = Point(centre, axes, -.032f, .195f, .700f, scale);
  Triangle(out, blowholeFront, blowholeRight, blowholeRear, colors[3], true);
  Triangle(out, blowholeFront, blowholeRear, blowholeLeft, colors[3], true);
  if (out.size() - start != DolphinRenderer::MaximumNearVertexCount)
    throw std::runtime_error("Dolphin near mesh does not match fixed budget");
}

void Far(std::vector<VertexPositionColor> &out, const DolphinState &dolphin,
         float time) {
  const auto start = out.size();
  const Vector3 c(dolphin.x, dolphin.y, dolphin.z);
  const auto a = Pose(dolphin);
  const auto colors = Palette(dolphin, time);
  const float s = 1.75f * dolphin.scale;
  const float tail = dolphin.tailBeat * .10f;
  const auto nose = Point(c, a, 0, 0, .95f, s),
             top = Point(c, a, 0, .21f, .15f, s),
             bottom = Point(c, a, 0, -.18f, .10f, s),
             rear = Point(c, a, tail, 0, -.70f, s);
  Triangle(out, nose, top, rear, colors[0], true);
  Triangle(out, nose, rear, bottom, colors[2], true);
  Triangle(out, top, Point(c, a, 0, .43f, -.08f, s), rear, colors[1], true);
  Triangle(out, rear, Point(c, a, -.38f, .02f, -.92f, s),
           Point(c, a, 0, 0, -1.0f, s), colors[1], true);
  Triangle(out, rear, Point(c, a, 0, 0, -1.0f, s),
           Point(c, a, .38f, .02f, -.92f, s), colors[1], true);
  if (out.size() - start != DolphinRenderer::MaximumFarVertexCount)
    throw std::runtime_error("Dolphin far mesh does not match fixed budget");
}
} // namespace

DolphinRenderer::DolphinRenderer(GraphicsDevice &device,
                                 const float maximumDistance)
    : device_(device), effect_(std::make_unique<BasicEffect>(device)),
      vertices_(std::make_unique<DynamicVertexBuffer>(
          device, VertexPositionColor::getVertexDeclarationStatic(),
          static_cast<int>(DolphinPod::DolphinCount) * MaximumNearVertexCount,
          BufferUsage::WriteOnly)),
      maximumDistance_(maximumDistance) {
  if (!std::isfinite(maximumDistance) || maximumDistance <= 0)
    throw std::invalid_argument("Invalid dolphin draw distance");
  staging_.reserve(DolphinPod::DolphinCount * MaximumNearVertexCount);
  effect_->setVertexColorEnabledProperty(true);
  effect_->setLightingEnabledProperty(false);
  effect_->setFogEnabledProperty(true);
  effect_->setFogColorProperty({.16f, .55f, .61f});
  effect_->setFogStartProperty(210);
  effect_->setFogEndProperty(570);
  const char *message =
      "island-simulator: dolphins use fusiform bodies, rostrums, curved dorsal "
      "fins and horizontal animated flukes";
  std::cout << message << '\n';
  MirrorRuntimeDiagnosticToPlatformLog(message);
  const char *detail =
      "island-simulator: near dolphins use closed-volume fins and flukes, short beaks, faceted eyes, mouth creases, and a blowhole";
  std::cout << detail << '\n';
  MirrorRuntimeDiagnosticToPlatformLog(detail);
}
DolphinRenderer::~DolphinRenderer() = default;

void DolphinRenderer::Draw(const Matrix &view, const Matrix &projection,
                           const Vector3 &camera,
                           const std::span<const DolphinState> dolphins,
                           const float time) {
  staging_.clear();
  visibleCount_ = visibleNearCount_ = visibleFarCount_ = drawCalls_ = 0;
  const BoundingFrustum frustum(view * projection);
  for (const auto &dolphin : dolphins) {
    const Vector3 centre(dolphin.x, dolphin.y, dolphin.z);
    const float distance2 = Vector3::DistanceSquared(centre, camera);
    if (distance2 > maximumDistance_ * maximumDistance_ ||
        !frustum.Intersects(BoundingSphere(centre, 2.2f * dolphin.scale)))
      continue;
    if (distance2 <= NearDistance * NearDistance) {
      Near(staging_, dolphin, time);
      ++visibleNearCount_;
    } else {
      Far(staging_, dolphin, time);
      ++visibleFarCount_;
    }
    ++visibleCount_;
  }
  if (staging_.empty())
    return;
  device_.setRasterizerStateProperty(RasterizerState::CullNone);
  device_.setBlendStateProperty(BlendState::Opaque);
  device_.setDepthStencilStateProperty(DepthStencilState::Default);
  vertices_->SetData(staging_.data(), 0, static_cast<int>(staging_.size()),
                     SetDataOptions::Discard);
  device_.SetVertexBuffer(vertices_.get());
  device_.setIndicesProperty(nullptr);
  effect_->setWorldProperty(Matrix::getIdentityProperty());
  effect_->setViewProperty(view);
  effect_->setProjectionProperty(projection);
  effect_->Apply();
  device_.DrawPrimitives(PrimitiveType::TriangleList, 0,
                         static_cast<int>(staging_.size() / 3));
  drawCalls_ = 1;
}
} // namespace island
