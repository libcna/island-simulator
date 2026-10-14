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
constexpr std::array<Section, 19> Body{{{-.80f, .042f, .040f, 0},
                                        {-.72f, .060f, .055f, 0},
                                        {-.62f, .082f, .074f, 0},
                                        {-.50f, .115f, .103f, 0},
                                        {-.37f, .155f, .137f, 0},
                                        {-.22f, .190f, .165f, 0},
                                        {-.06f, .222f, .185f, 0},
                                        {.10f, .242f, .198f, 0},
                                        {.25f, .246f, .200f, 0},
                                        {.40f, .238f, .197f, .002f},
                                        {.53f, .225f, .190f, .006f},
                                        {.64f, .210f, .182f, .014f},
                                        {.73f, .193f, .172f, .025f},
                                        {.81f, .174f, .160f, .038f},
                                        {.87f, .150f, .145f, .052f},
                                        {.92f, .122f, .130f, .064f},
                                        {.955f, .096f, .110f, .068f},
                                        {.978f, .070f, .085f, .064f},
                                        {.992f, .044f, .058f, .054f}}};
constexpr int Segments = 24;

void Triangle(std::vector<VertexPositionColor> &out, const Vector3 &a,
              const Vector3 &b, const Vector3 &c, const Color &color,
              bool twoSided = false) {
  const auto lit = wildlife_render::FacetColor(a, b, c, color, twoSided);
  out.emplace_back(a, lit);
  out.emplace_back(b, lit);
  out.emplace_back(c, lit);
}

void GradientTriangle(std::vector<VertexPositionColor> &out, const Vector3 &a,
                      const Vector3 &b, const Vector3 &c, const Color &colorA,
                      const Color &colorB, const Color &colorC) {
  out.emplace_back(a, colorA);
  out.emplace_back(b, colorB);
  out.emplace_back(c, colorC);
}

Color BodyColor(const std::array<Color, 4> &colors, const float vertical) {
  if (vertical < .05f)
    return Color::Lerp(colors[0], colors[2],
                       std::clamp((.05f - vertical) / .92f, 0.0f, 1.0f));
  return Color::Lerp(colors[0], colors[1],
                     std::clamp((vertical - .05f) / .88f, 0.0f, 1.0f));
}

Color SmoothBodyColor(const std::array<Color, 4> &colors, const Axes &axes,
                      const float angle) {
  const Vector3 normal = Vector3::Normalize(
      axes.right * std::cos(angle) + axes.up * std::sin(angle));
  const float direct = std::max(
      0.0f, Vector3::Dot(normal, wildlife_render::FixedSunDirection));
  const float sky = std::max(0.0f, normal.Y);
  return wildlife_render::ScaleColor(BodyColor(colors, std::sin(angle)),
                                     .76f + .25f * direct + .05f * sky);
}

void Quad(std::vector<VertexPositionColor> &out, const Vector3 &a,
          const Vector3 &b, const Vector3 &c, const Vector3 &d,
          const Color &color) {
  Triangle(out, a, b, c, color);
  Triangle(out, a, c, d, color);
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
      {std::array{Color(105, 124, 132, 255), Color(87, 107, 117, 255),
                  Color(154, 168, 168, 255), Color(20, 28, 33, 255)},
       std::array{Color(112, 128, 133, 255), Color(92, 110, 118, 255),
                  Color(160, 171, 165, 255), Color(18, 27, 31, 255)},
       std::array{Color(96, 118, 132, 255), Color(78, 101, 116, 255),
                  Color(148, 166, 171, 255), Color(17, 25, 30, 255)}}};
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
      const float angleS = 2 * std::numbers::pi_v<float> * s / Segments;
      const float angleN = 2 * std::numbers::pi_v<float> * n / Segments;
      const Color colorS = SmoothBodyColor(colors, axes, angleS);
      const Color colorN = SmoothBodyColor(colors, axes, angleN);
      GradientTriangle(out, rings[row][s], rings[row + 1][s], rings[row + 1][n],
                       colorS, colorS, colorN);
      GradientTriangle(out, rings[row][s], rings[row + 1][n], rings[row][n],
                       colorS, colorN, colorN);
    }
  // Close the melon rather than leaving the foremost body ring as a visible cut plane.
  const auto melonCap = Point(centre, axes, 0, .050f, 1.010f, scale);
  for (int segment = 0; segment < Segments; ++segment) {
    const int next = (segment + 1) % Segments;
    const float angle = 2 * std::numbers::pi_v<float> * segment / Segments;
    const float nextAngle = 2 * std::numbers::pi_v<float> * next / Segments;
    GradientTriangle(out, rings.back()[segment], melonCap, rings.back()[next],
                     SmoothBodyColor(colors, axes, angle),
                     wildlife_render::ScaleColor(colors[0], .88f),
                     SmoothBodyColor(colors, axes, nextAngle));
  }
  // A stout closed rostrum grows out of the lower forehead. Earlier radii made the
  // beak collapse into a paper-thin plate in the authored broadside camera.
  constexpr int RostrumSegments = 12;
  std::array<Vector3, RostrumSegments> rostrumBase{};
  std::array<Vector3, RostrumSegments> rostrumTip{};
  for (int segment = 0; segment < RostrumSegments; ++segment) {
    const float angle = 2.0f * std::numbers::pi_v<float>
        * static_cast<float>(segment) / static_cast<float>(RostrumSegments);
    rostrumBase[segment] = Point(centre, axes, .108f * std::cos(angle),
        -.032f + .068f * std::sin(angle), .882f, scale);
    rostrumTip[segment] = Point(centre, axes, .058f * std::cos(angle),
        -.043f + .043f * std::sin(angle), 1.135f, scale);
  }
  const auto rostrumCap = Point(centre, axes, 0, -.043f, 1.180f, scale);
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
      Point(centre, axes, 0, .205f, .16f, scale),
      Point(centre, axes, 0, .258f, .075f, scale),
      Point(centre, axes, 0, .302f, -.020f, scale),
      Point(centre, axes, 0, .286f, -.105f, scale),
      Point(centre, axes, 0, .246f, -.190f, scale),
      Point(centre, axes, 0, .207f, -.280f, scale),
      Point(centre, axes, 0, .180f, -.350f, scale)};
  ClosedPolygon(out, dorsal, axes.right * (.010f * scale), colors[0], colors[1]);
  for (float side : {-1.0f, 1.0f}) {
    const std::array fin{
        Point(centre, axes, side * .19f, -.055f, .30f, scale),
        Point(centre, axes, side * .27f, -.080f, .22f, scale),
        Point(centre, axes, side * .35f, -.110f, .10f, scale),
        Point(centre, axes, side * .47f, -.145f, -.060f, scale),
        Point(centre, axes, side * .34f, -.145f, -.170f, scale),
        Point(centre, axes, side * .23f, -.125f, -.150f, scale),
        Point(centre, axes, side * .14f, -.105f, -.18f, scale)};
    ClosedPolygon(out, fin, axes.up * (.009f * scale), colors[0], colors[1]);
  }
  const float tail = dolphin.tailBeat * .12f;
  for (float side : {-1.0f, 1.0f}) {
    const std::array fluke{
        Point(centre, axes, 0, tail, -.68f, scale),
        Point(centre, axes, side * .18f, tail + .018f, -.75f, scale),
        Point(centre, axes, side * .43f, tail + .020f, -.84f, scale),
        Point(centre, axes, side * .52f, tail + .012f, -.92f, scale),
        Point(centre, axes, side * .38f, tail - .008f, -1.00f, scale),
        Point(centre, axes, side * .10f, tail - .014f, -1.05f, scale)};
    ClosedPolygon(out, fluke, axes.up * (.007f * scale), colors[0], colors[1]);
  }
  const Color eyeShine(153, 173, 176, 255);
  for (float side : {-1.0f, 1.0f}) {
    constexpr int EyeSegments = 6;
    const auto eyeCentre = Point(centre, axes, side * .218f, .058f, .642f, scale);
    for (int segment = 0; segment < EyeSegments; ++segment) {
      const float first = 2.0f * std::numbers::pi_v<float> * segment / EyeSegments;
      const float next = 2.0f * std::numbers::pi_v<float> * (segment + 1) / EyeSegments;
      const auto edge = [&](const float angle) {
        return Point(centre, axes, side * .221f,
                     .058f + std::sin(angle) * .014f,
                     .642f + std::cos(angle) * .020f, scale);
      };
      if (side > 0) Triangle(out, eyeCentre, edge(next), edge(first), colors[3]);
      else Triangle(out, eyeCentre, edge(first), edge(next), colors[3]);
    }
    Triangle(out, Point(centre, axes, side * .223f, .066f, .649f, scale),
             Point(centre, axes, side * .224f, .061f, .653f, scale),
             Point(centre, axes, side * .224f, .060f, .646f, scale), eyeShine);

    const auto mouthFront = Point(centre, axes, side * .027f, -.078f, 1.150f, scale);
    const auto mouthOuter = Point(centre, axes, side * .092f, -.078f, .905f, scale);
    const auto mouthKnee = Point(centre, axes, side * .070f, -.088f, 1.020f, scale);
    const auto mouthInner = Point(centre, axes, side * .025f, -.086f, 1.125f, scale);
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
      "island-simulator: near dolphins use a nineteen-station closed melon profile, twenty-four-sided body, and vertex-blended countershading";
  std::cout << silhouette << '\n';
  MirrorRuntimeDiagnosticToPlatformLog(silhouette);
  const char *stroke =
      "island-simulator: dolphin tail stock bends vertically through horizontal fluke strokes";
  std::cout << stroke << '\n';
  MirrorRuntimeDiagnosticToPlatformLog(stroke);
  const char *profile =
      "island-simulator: near dolphins use an extended fitted rostrum, lower swept fins, and readable grey countershading";
  std::cout << profile << '\n';
  MirrorRuntimeDiagnosticToPlatformLog(profile);
  const char *face =
      "island-simulator: dolphin facial pass uses a stout tapered rostrum and small six-facet oval eyes instead of flat diamond marks";
  std::cout << face << '\n';
  MirrorRuntimeDiagnosticToPlatformLog(face);
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
