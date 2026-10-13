#pragma once
namespace island {
inline constexpr const char* HutMaterialVertex=R"(
layout(location=0) in vec3 aPosition;
layout(location=1) in vec4 aColor;
layout(location=2) in vec2 aTextureCoordinate;
uniform mat4 uViewProjection;
out vec3 vWorld;
out vec4 vColor;
out vec2 vGrain;
void main() {
    vWorld=aPosition; vColor=aColor; vGrain=aTextureCoordinate;
    gl_Position=uViewProjection*vec4(aPosition,1.0);
    gl_Position.z=2.0*gl_Position.z-gl_Position.w;
}
)";
inline constexpr const char* HutMaterialFragment=R"(
in vec3 vWorld;
in vec4 vColor;
in vec2 vGrain;
uniform vec3 uCameraPosition;
uniform vec3 uHutCentre;
uniform sampler2D uThatchTexture;
uniform vec3 uThatchMean;
out vec4 fragColor;
// Fade unresolved cycles to their mean; no texture, loops or dependent sampling.
float resolvedWave(float phase) {
    return sin(phase)*(1.0-smoothstep(0.7,2.5,fwidth(phase)));
}
void main() {
    float u=vGrain.x,v=vGrain.y;
    float warp=sin(v*2.3+sin(u*5.1))*0.8+sin(v*0.81)*0.45;
    float grain=resolvedWave(u*210.0+warp);
    float fine=resolvedWave(u*970.0+warp*3.2);
    float weather=sin(u*14.0+v*0.6)*sin(v*2.7+u*2.0);
    vec3 timber=vColor.rgb*(0.98+0.09*grain+0.035*fine-0.08*max(weather,0.0));
    float grey=dot(timber,vec3(0.3,0.59,0.11));
    timber=mix(timber,vec3(grey)*vec3(1.04,1.02,0.96),0.18+0.08*weather);
    float rings=resolvedWave(length(vGrain)*230.0+sin(u*31.0)*0.35);
    if(vColor.a>0.4&&vColor.a<0.65) timber=vColor.rgb*(0.97+0.09*rings);
    float fibres=resolvedWave(u*460.0+sin(v*8.0)*0.28);
    float fineFibres=resolvedWave(u*1410.0+sin(v*13.0)*0.4);
    vec3 leafSample=texture(uThatchTexture,vGrain).rgb;
    vec3 leafVariation=clamp(leafSample/uThatchMean,vec3(0.5),vec3(1.55));
    vec3 thatch=vColor.rgb*mix(vec3(1.0),leafVariation,0.85)
        *(0.98+0.04*fibres+0.015*fineFibres);
    float glassClass=step(0.65,vColor.a)*(1.0-step(0.7,vColor.a));
    float reflectedSky=smoothstep(0.12,0.78,v)
        *(0.82+0.18*resolvedWave(u*7.0+v*2.0));
    float softGlint=0.5+0.5*resolvedWave((u-v)*18.0+vWorld.y*1.7);
    vec3 glass=mix(vColor.rgb*vec3(0.56,0.64,0.61),
        vec3(0.30,0.47,0.51),0.40*reflectedSky);
    glass+=vec3(0.055,0.075,0.072)*softGlint*reflectedSky;
    vec3 color=vColor.a>0.7?thatch:timber;
    color=mix(color,glass,glassClass);
    float detail=1.0-smoothstep(35.0,80.0,length(vWorld-uCameraPosition));
    color=mix(vColor.rgb,color,detail);
    float base=(1.0-smoothstep(-0.3,0.25,vWorld.y-uHutCentre.y))*(1.0-step(0.7,vColor.a));
    color*=1.0-base*0.12;
    color=mix(color,vec3(0.53,0.78,0.88),smoothstep(180.0,300.0,length(vWorld-uCameraPosition)));
    fragColor=vec4(color,1.0);
}
)";
}
