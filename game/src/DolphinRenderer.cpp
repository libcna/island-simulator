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
  float z, rx, ry, centreY;
};
constexpr std::array<Section, 13> Body{{{-.76f, .052f, .052f, 0},
                                        {-.66f, .078f, .071f, 0},
                                        {-.54f, .115f, .103f, 0},
                                        {-.38f, .158f, .138f, 0},
                                        {-.18f, .203f, .171f, 0},
                                        {.04f, .240f, .195f, 0},
                                        {.24f, .245f, .198f, 0},
                                        {.43f, .235f, .194f, .002f},
                                        {.59f, .220f, .188f, .006f},
                                        {.71f, .200f, .178f, .014f},
                                        {.81f, .172f, .165f, .026f},
                                        {.89f, .136f, .150f, .040f},
                                        {.945f, .091f, .116f, .044f}}};
constexpr int Segments = 16;

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

template <std::size_t Count>
void ClosedPolygon(std::vector<VertexPositionColor> &out,
                   const std::array<Vector3, Count> &outline,
                   const Vector3 &halfThickness, const Color &face,
                   const Color &edge) {
  static_assert(Count >= 3);
  std::array<Vector3, Count> positive{}, negative{};
  for (std::size_t i = 0; i < Count; ++i) {
    positive[i] = outline[i] + halfThickness;
    negative[i] = outline[i] - halfThickness;
  }
  for (std::size_t i = 1; i + 1 < Count; ++i) {
    Triangle(out, positive[0], positive[i], positive[i + 1], face);
    Triangle(out, negative[0], negative[i + 1], negative[i], face);
  }
  for (std::size_t i = 0; i < Count; ++i) {
    const std::size_t next = (i + 1) % Count;
    Quad(out, positive[i], negative[i], negative[next], positive[next], edge);
  }
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
      {std::array{Color(86, 108, 118, 255), Color(69, 91, 102, 255),
                  Color(139, 154, 155, 255), Color(20, 28, 33, 255)},
       std::array{Color(94, 112, 118, 255), Color(75, 94, 102, 255),
                  Color(146, 157, 151, 255), Color(18, 27, 31, 255)},
       std::array{Color(77, 101, 115, 255), Color(61, 83, 97, 255),
                  Color(130, 150, 154, 255), Color(17, 25, 30, 255)}}};
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
          Point(centre, axes, section.rx * std::cos(angle),
                section.centreY + section.ry * std::sin(angle) + bend,
                section.z, scale);
    }
  }
  for (std::size_t row = 0; row + 1 < Body.size(); ++row)
    for (int s = 0; s < Segments; ++s) {
      const int n = (s + 1) % Segments;
      const float angle = 2 * std::numbers::pi_v<float> * (s + .5f) / Segments;
      const float vertical = std::sin(angle);
      const Color color = vertical < -.24f
                              ? colors[2]
                              : (vertical > .42f ? colors[1] : colors[0]);
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
    rostrumBase[segment] = Point(centre, axes, .090f * std::cos(angle),
        -.042f + .055f * std::sin(angle), .905f, scale);
    rostrumTip[segment] = Point(centre, axes, .045f * std::cos(angle),
        -.047f + .034f * std::sin(angle), 1.035f, scale);
  }
  const auto rostrumCap = Point(centre, axes, 0, -.047f, 1.065f, scale);
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
  const std::array dorsal{
      Point(centre, axes, 0, .225f, .13f, scale),
      Point(centre, axes, 0, .305f, .025f, scale),
      Point(centre, axes, 0, .355f, -.070f, scale),
      Point(centre, axes, 0, .272f, -.18f, scale),
      Point(centre, axes, 0, .183f, -.31f, scale)};
  ClosedPolygon(out, dorsal, axes.right * (.010f * scale), colors[0], colors[1]);
  for (float side : {-1.0f, 1.0f}) {
    const std::array fin{
        Point(centre, axes, side * .19f, -.055f, .30f, scale),
        Point(centre, axes, side * .34f, -.105f, .13f, scale),
        Point(centre, axes, side * .44f, -.13f, .015f, scale),
        Point(centre, axes, side * .34f, -.14f, -.095f, scale),
        Point(centre, axes, side * .14f, -.105f, -.18f, scale)};
    ClosedPolygon(out, fin, axes.up * (.009f * scale), colors[0], colors[1]);
  }
  const float tail = dolphin.tailBeat * .12f;
  for (float side : {-1.0f, 1.0f}) {
    const auto root = Point(centre, axes, 0, tail, -.68f, scale);
    const auto leading =
        Point(centre, axes, side * .30f, tail + .02f, -.80f, scale);
    const auto outer =
        Point(centre, axes, side * .50f, tail + .018f, -.91f, scale);
    const auto rear =
        Point(centre, axes, side * .12f, tail - .012f, -1.05f, scale);
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
             rear = Point(c, a, 0, tail, -.70f, s);
  Triangle(out, nose, top, rear, colors[0], true);
  Triangle(out, nose, rear, bottom, colors[2], true);
  Triangle(out, top, Point(c, a, 0, .43f, -.08f, s), rear, colors[1], true);
  Triangle(out, rear, Point(c, a, -.38f, tail + .02f, -.92f, s),
           Point(c, a, 0, tail, -1.0f, s), colors[1], true);
  Triangle(out, rear, Point(c, a, 0, tail, -1.0f, s),
           Point(c, a, .38f, tail + .02f, -.92f, s), colors[1], true);
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
  const char *silhouette =
      "island-simulator: near dolphins use a thirteen-station melon-to-peduncle profile, natural countershading, a compact falcate dorsal, and swept pectoral fins";
  std::cout << silhouette << '\n';
  MirrorRuntimeDiagnosticToPlatformLog(silhouette);
  const char *stroke =
      "island-simulator: dolphin tail stock bends vertically through horizontal fluke strokes";
  std::cout << stroke << '\n';
  MirrorRuntimeDiagnosticToPlatformLog(stroke);
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
