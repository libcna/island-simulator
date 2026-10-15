#pragma once
#include <memory>
#include <span>
#include <vector>
#include "IslandSimulator/OctopusGroup.hpp"
#include "Microsoft/Xna/Framework/Matrix.hpp"
#include "Microsoft/Xna/Framework/Vector3.hpp"
namespace Microsoft::Xna::Framework::Graphics{class BasicEffect;class DynamicVertexBuffer;class GraphicsDevice;struct VertexPositionColor;}
namespace island{
class OctopusRenderer final{
public:explicit OctopusRenderer(Microsoft::Xna::Framework::Graphics::GraphicsDevice&,float maximumDistance=180);~OctopusRenderer();
OctopusRenderer(const OctopusRenderer&)=delete;OctopusRenderer& operator=(const OctopusRenderer&)=delete;
void Draw(const Microsoft::Xna::Framework::Matrix&,const Microsoft::Xna::Framework::Matrix&,const Microsoft::Xna::Framework::Vector3&,std::span<const OctopusState>);
[[nodiscard]] int VisibleCount()const noexcept{return visible_;}[[nodiscard]] int NearCount()const noexcept{return near_;}[[nodiscard]] int FarCount()const noexcept{return far_;}[[nodiscard]] int DrawCallCount()const noexcept{return draws_;}
private:Microsoft::Xna::Framework::Graphics::GraphicsDevice& device_;std::unique_ptr<Microsoft::Xna::Framework::Graphics::BasicEffect> effect_;std::unique_ptr<Microsoft::Xna::Framework::Graphics::DynamicVertexBuffer> vertices_;std::vector<Microsoft::Xna::Framework::Graphics::VertexPositionColor> staging_;float maximumDistance_;int visible_=0,near_=0,far_=0,draws_=0;};}
