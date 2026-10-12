#pragma once
#include "IslandSimulator/CampfireLayout.hpp"
#include "Microsoft/Xna/Framework/Graphics/ShaderEffect.hpp"
#include <cmath>
namespace island {
struct CampfireLighting final {
    static constexpr const char* Shader=R"(
uniform vec4 uCampfire;
float campfirePulse(float t){return .83+.10*sin(t*8.7)+.07*sin(t*17.1+1.3);}
vec3 applyCampfireLight(vec3 color,vec3 world){
    vec3 offset=world-uCampfire.xyz;float squared=dot(offset,offset);
    if(squared>=36.0)return color;
    float falloff=max(0.0,1.0-squared/36.0);
    return min(vec3(1),color+vec3(.16,.072,.018)*falloff*falloff*campfirePulse(uCampfire.w));
}
)";
    static void Bind(Microsoft::Xna::Framework::Graphics::ShaderEffect& effect,float seconds) {
        effect.SetUniformVec4("uCampfire",CampfireSite::X,CampfireLayout::GroundY()+.45f,CampfireSite::Z,
            std::isfinite(seconds)?std::remainder(seconds,628.31853f):0);
    }
};
}
