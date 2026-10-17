#pragma once

#include "IslandSimulator/DolphinPod.hpp"
#include "Microsoft/Xna/Framework/Matrix.hpp"
#include "Microsoft/Xna/Framework/Vector3.hpp"

#include <memory>
#include <span>
#include <vector>

namespace Microsoft::Xna::Framework::Graphics {
class BasicEffect;
class DynamicVertexBuffer;
class GraphicsDevice;
struct VertexPositionColor;
} // namespace Microsoft::Xna::Framework::Graphics

namespace island {
/** One portable dynamic batch for the offshore dolphin pod. */
class DolphinRenderer final {
public:
  static constexpr int MaximumNearVertexCount = 3390;
  static constexpr int MaximumFarVertexCount = 15;
  explicit DolphinRenderer(
      Microsoft::Xna::Framework::Graphics::GraphicsDevice &,
      float maximumDistance = 580.0f);
  ~DolphinRenderer();
  void Draw(const Microsoft::Xna::Framework::Matrix &,
            const Microsoft::Xna::Framework::Matrix &,
            const Microsoft::Xna::Framework::Vector3 &,
            std::span<const DolphinState>, float timeSeconds);
  [[nodiscard]] int VisibleCount() const noexcept { return visibleCount_; }
  [[nodiscard]] int VisibleNearCount() const noexcept {
    return visibleNearCount_;
  }
  [[nodiscard]] int VisibleFarCount() const noexcept {
    return visibleFarCount_;
  }
  [[nodiscard]] int DrawCallCount() const noexcept { return drawCalls_; }

private:
  Microsoft::Xna::Framework::Graphics::GraphicsDevice &device_;
  std::unique_ptr<Microsoft::Xna::Framework::Graphics::BasicEffect> effect_;
  std::unique_ptr<Microsoft::Xna::Framework::Graphics::DynamicVertexBuffer>
      vertices_;
  std::vector<Microsoft::Xna::Framework::Graphics::VertexPositionColor>
      staging_;
  int visibleCount_ = 0, visibleNearCount_ = 0, visibleFarCount_ = 0,
      drawCalls_ = 0;
  float maximumDistance_ = 580;
};
} // namespace island
