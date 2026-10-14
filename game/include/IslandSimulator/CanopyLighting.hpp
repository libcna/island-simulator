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
    class IslandTerrain;
    /**
     * @brief Ground texture: red transmission, green cover, blue coastal rock, alpha beach potential.
     *
     * The CPU fields and this owner must outlive all receiving renderers.
     */
    class CanopyLighting final
    {
    public:
        CanopyLighting(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                       const CanopyLightField& field, const GroundCoverField& cover,
                       const IslandTerrain& terrain);
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
// x: coastal rockiness, y: beach suitability; same grid and interpolation as lighting.
vec2 coastalGroundField(vec2 groundPosition)
{
    vec2 uv = (groundPosition + vec2(uCanopyMap.x)) * uCanopyMap.y;
    vec2 inside = step(vec2(0.0), uv) * step(uv, vec2(1.0));
    return textureLod(uCanopyLight, clamp(uv, 0.0, 1.0), 0.0).ba * inside.x * inside.y;
}
float canopyTransmission(vec2 groundPosition)
{
    return canopyGroundField(groundPosition).x;
}
float canopyDappleTransmission(vec2 groundPosition,float transmission)
{
    // Broad, world-anchored openings suggest sun passing between leaf clusters without
    // introducing an unfiltered high-frequency shadow texture or per-frame shadow map.
    float primary=sin(groundPosition.x*0.71+groundPosition.y*0.37
        +sin(groundPosition.y*0.13)*0.85);
    float crossing=sin(groundPosition.x*-0.29+groundPosition.y*0.83
        +sin(groundPosition.x*0.17)*0.62);
    float openings=smoothstep(0.40,0.79,0.5+0.25*primary+0.25*crossing);
    float deepShade=1.0-smoothstep(0.62,0.96,transmission);
    return clamp(transmission+openings*deepShade*0.22,0.0,1.0);
}
vec3 applyCanopyLight(vec3 color, vec2 groundPosition)
{
    // Retain cool skylight: this field attenuates direct daylight, not all illumination.
    float transmission=canopyTransmission(groundPosition);
    transmission=canopyDappleTransmission(groundPosition,transmission);
    return color * mix(uCanopyAmbient, vec3(1.0), transmission);
}
)";
    private:
        Microsoft::Xna::Framework::Graphics::GraphicsDevice& device_;
        const CanopyLightField& field_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::Texture2D> texture_;
    };
}
