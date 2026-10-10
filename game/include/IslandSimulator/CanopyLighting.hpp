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
    class CanopyLightField;
    class GroundCoverField;
    /**
     * @brief Immutable ground-field texture: red/blue direct transmission, green cover patch.
     *
     * The CPU fields and this owner must outlive all receiving renderers.
     */
    class CanopyLighting final
    {
    public:
        CanopyLighting(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                       const CanopyLightField& field, const GroundCoverField& cover);
        ~CanopyLighting();
        CanopyLighting(const CanopyLighting&) = delete;
        CanopyLighting& operator=(const CanopyLighting&) = delete;
        void Bind(Microsoft::Xna::Framework::Graphics::ShaderEffect& effect);
        [[nodiscard]] const CanopyLightField& Field() const noexcept { return field_; }
        static constexpr const char* Shader = R"(
uniform sampler2D uCanopyLight;
uniform vec2 uCanopyMap;
uniform vec3 uCanopyAmbient;
vec2 canopyGroundReceiver(vec3 worldPosition, float groundHeight, vec3 toSun)
{
    // Ground-anchor approximation for low receivers; never use this for canopy self-shadowing.
    return worldPosition.xz - toSun.xz / toSun.y * max(0.0, worldPosition.y - groundHeight);
}
// x: direct-sun transmission, y: ground-cover patch potential. Outside: open sun, no cover.
vec2 canopyGroundField(vec2 groundPosition)
{
    vec2 uv = (groundPosition + vec2(uCanopyMap.x)) * uCanopyMap.y;
    vec2 inside = step(vec2(0.0), uv) * step(uv, vec2(1.0));
    // The field has one level: explicit LOD keeps this valid inside per-primitive branches.
    return mix(vec2(1.0, 0.0), textureLod(uCanopyLight, clamp(uv, 0.0, 1.0), 0.0).rg,
               inside.x * inside.y);
}
float canopyTransmission(vec2 groundPosition)
{
    return canopyGroundField(groundPosition).x;
}
vec3 applyCanopyLight(vec3 color, vec2 groundPosition)
{
    // Retain cool skylight: this field attenuates direct daylight, not all illumination.
    return color * mix(uCanopyAmbient, vec3(1.0), canopyTransmission(groundPosition));
}
)";
    private:
        Microsoft::Xna::Framework::Graphics::GraphicsDevice& device_;
        const CanopyLightField& field_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::Texture2D> texture_;
    };
}
