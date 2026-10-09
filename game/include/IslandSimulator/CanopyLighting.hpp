#pragma once
#include <memory>

namespace Microsoft::Xna::Framework::Graphics
{
    class GraphicsDevice;
    class ShaderEffect;
    class Texture2D;
}
namespace island
{
    class IslandTerrain;
    class JungleLayout;
    class CanopyLightField;
    /** @brief Immutable transmission texture; its owner must outlive all receiving renderers. */
    class CanopyLighting final
    {
    public:
        CanopyLighting(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                       const IslandTerrain& terrain, const JungleLayout& layout);
        ~CanopyLighting();
        CanopyLighting(const CanopyLighting&) = delete;
        CanopyLighting& operator=(const CanopyLighting&) = delete;
        void Bind(Microsoft::Xna::Framework::Graphics::ShaderEffect& effect);
        [[nodiscard]] const CanopyLightField& Field() const noexcept { return *field_; }
        static constexpr const char* Shader = R"(
uniform sampler2D uCanopyLight;
uniform vec2 uCanopyMap;
uniform vec3 uCanopyAmbient;
vec2 canopyGroundReceiver(vec3 worldPosition, float groundHeight, vec3 toSun)
{
    // Ground-anchor approximation for low receivers; never use this for canopy self-shadowing.
    return worldPosition.xz - toSun.xz / toSun.y * max(0.0, worldPosition.y - groundHeight);
}
vec3 applyCanopyLight(vec3 color, vec2 groundPosition)
{
    vec2 uv = (groundPosition + vec2(uCanopyMap.x)) * uCanopyMap.y;
    vec2 inside = step(vec2(0.0), uv) * step(uv, vec2(1.0));
    float transmission = mix(1.0, texture(uCanopyLight, clamp(uv, 0.0, 1.0)).r,
                              inside.x * inside.y);
    // Retain cool skylight: this field attenuates direct daylight, not all illumination.
    return color * mix(uCanopyAmbient, vec3(1.0), transmission);
}
)";
    private:
        Microsoft::Xna::Framework::Graphics::GraphicsDevice& device_;
        std::unique_ptr<CanopyLightField> field_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::Texture2D> texture_;
    };
}
