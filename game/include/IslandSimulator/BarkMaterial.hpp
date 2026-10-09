#pragma once

#include "IslandSimulator/JungleMaterial.hpp"
#include <cstdint>
#include <numbers>

namespace island::BarkMaterial
{
    inline constexpr float Tau = 2.0f * std::numbers::pi_v<float>;

    inline float Cell(std::uint32_t x, std::uint32_t y, std::uint32_t seed)
    {
        std::uint32_t h = x*374761393U + y*668265263U + seed*1442695041U;
        h = (h ^ (h >> 13U))*1274126177U;
        return static_cast<float>((h ^ (h >> 16U)) & 65535U)/65535.0f;
    }

    // The angular lattice wraps at an integer cell count; no duplicated seam vertices.
    inline float Noise(float angle, float along, int cells, std::uint32_t seed)
    {
        if (!std::isfinite(angle) || !std::isfinite(along) || std::abs(angle) > 100
            || along < 0 || along > 10000 || cells < 1 || cells > 32)
            throw std::invalid_argument("Bark noise coordinates outside supported local range");
        const float turns = angle/Tau;
        const float x = (turns-std::floor(turns))*cells;
        const int ix = static_cast<int>(std::floor(x)), iy = static_cast<int>(std::floor(along));
        const auto wrap = [cells](int value) { return static_cast<std::uint32_t>((value%cells+cells)%cells); };
        const float u = JungleMaterial::Smooth(0,1,x-std::floor(x));
        const float v = JungleMaterial::Smooth(0,1,along-std::floor(along));
        return std::lerp(std::lerp(Cell(wrap(ix),iy,seed),Cell(wrap(ix+1),iy,seed),u),
                         std::lerp(Cell(wrap(ix),iy+1,seed),Cell(wrap(ix+1),iy+1,seed),u),v);
    }

    // CPU reference; footprints are radians/pixel and local metres/pixel respectively.
    inline float Sample(float angle, float along, float angularFootprint, float axialFootprint,
                        std::uint32_t seed)
    {
        using namespace JungleMaterial;
        if (!std::isfinite(angularFootprint) || !std::isfinite(axialFootprint)
            || angularFootprint < 0 || axialFootprint < 0 || seed > 1023)
            throw std::invalid_argument("Invalid bark material footprint or seed");
        const float warpFootprint = angularFootprint*(8/Tau)+axialFootprint*0.65f;
        const float warp = (Noise(angle,along*0.65f,8,seed)-0.5f)*2.8f*DetailWeight(warpFootprint);
        const float phase = static_cast<float>(seed%32)*(Tau/32);
        const float grainPhase = angle*5+along*0.42f+phase+std::sin(along*1.31f+phase)*0.32f+warp*0.5f;
        const float fissurePhase = angle*9+along*0.19f+phase*2+std::sin(along*0.77f+phase)*0.55f+warp;
        // Smoothstep lattice interpolation has maximum slope 1.5 per cell. Include the warp
        // bound even when the warp is being filtered; this deliberately errs toward less detail.
        const float grainFootprint = angularFootprint*5+axialFootprint*(0.42f+1.31f*0.32f)+2.1f*warpFootprint;
        const float fissureFootprint = angularFootprint*9+axialFootprint*(0.19f+0.77f*0.55f)+4.2f*warpFootprint;
        const float grain = std::sin(grainPhase)*DetailWeight(grainFootprint/Tau);
        const float fissure = std::lerp(FissureMean,Smooth(FissureStart,FissureEnd,std::abs(std::sin(fissurePhase))),
                                        DetailWeight(fissureFootprint/(Tau/2)));
        const float breaks = std::lerp(0.5f,Noise(angle,along*2.3f,13,seed^91U),
                                       DetailWeight(angularFootprint*(13/Tau)+axialFootprint*2.3f));
        const float plates = (Noise(angle,along*1.2f,5,seed^2136U)-0.5f)*2
            *DetailWeight(angularFootprint*(5/Tau)+axialFootprint*1.2f);
        return 1+grain*0.035f+plates*0.045f-fissure*(0.1f+0.9f*breaks)*0.16f;
    }

    // GLSL 330 / ES 300: same integer hash and reference coefficients. No sine hash whose
    // instability depends on implementation-specific transcendental precision.
    inline constexpr const char* Shader = R"(
float barkCell(uvec2 cell,uint seed) {
    uint h=cell.x*374761393u+cell.y*668265263u+seed*1442695041u;
    h=(h^(h>>13u))*1274126177u;
    return float((h^(h>>16u))&65535u)/65535.0;
}
float barkNoise(float angle,float along,int cells,uint seed) {
    vec2 p=vec2(fract(angle/6.28318530718)*float(cells),along);
    ivec2 cell=ivec2(floor(p));
    // Nonnegative operands: GLSL does not define remainder for negative operands.
    int x=cell.x%cells, next=(x+1)%cells;
    vec2 t=fract(p); t=t*t*(3.0-2.0*t);
    return mix(mix(barkCell(uvec2(x,cell.y),seed),barkCell(uvec2(next,cell.y),seed),t.x),
               mix(barkCell(uvec2(x,cell.y+1),seed),barkCell(uvec2(next,cell.y+1),seed),t.x),t.y);
}
float barkMaterial(vec4 coordinate,uint seed) {
    float angle=coordinate.x, along=coordinate.y;
    float angularFootprint=coordinate.z, axialFootprint=coordinate.w;
    float warpFootprint=angularFootprint*(8.0/6.28318530718)+axialFootprint*0.65;
    float warp=(barkNoise(angle,along*0.65,8,seed)-0.5)*2.8*materialDetailWeight(warpFootprint);
    float phase=float(seed%32u)*(6.28318530718/32.0);
    float grainPhase=angle*5.0+along*0.42+phase+sin(along*1.31+phase)*0.32+warp*0.5;
    float fissurePhase=angle*9.0+along*0.19+phase*2.0+sin(along*0.77+phase)*0.55+warp;
    float grainFootprint=angularFootprint*5.0+axialFootprint*(0.42+1.31*0.32)+2.1*warpFootprint;
    float fissureFootprint=angularFootprint*9.0+axialFootprint*(0.19+0.77*0.55)+4.2*warpFootprint;
    float grain=sin(grainPhase)*materialDetailWeight(grainFootprint/6.28318530718);
    float fissure=mix(barkFissureTerms.z,smoothstep(barkFissureTerms.x,barkFissureTerms.y,abs(sin(fissurePhase))),
        materialDetailWeight(fissureFootprint/3.14159265359));
    float breaks=mix(0.5,barkNoise(angle,along*2.3,13,seed^91u),
        materialDetailWeight(angularFootprint*(13.0/6.28318530718)+axialFootprint*2.3));
    float plates=(barkNoise(angle,along*1.2,5,seed^2136u)-0.5)*2.0
        *materialDetailWeight(angularFootprint*(5.0/6.28318530718)+axialFootprint*1.2);
    return 1.0+grain*0.035+plates*0.045-fissure*(0.1+0.9*breaks)*0.16;
}
)";
}
