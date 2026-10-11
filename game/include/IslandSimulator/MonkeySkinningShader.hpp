#pragma once

#include <cstddef>
#include <string>

namespace island::MonkeySkinningShader
{
    inline constexpr std::size_t BoneCount = 14;
    // 56 palette vectors + 12 transform vectors + conservatively one vector per scalar/vec3.
    inline constexpr std::size_t VertexUniformVectorBudget = BoneCount * 4 + 12 + 8;

    inline constexpr const char* VertexBody = R"GLSL(
layout(location=0) in vec3 aPosition;
layout(location=1) in vec3 aNormal;
layout(location=3) in vec4 aBoneWeights;
layout(location=4) in vec4 aBoneIndices;
uniform mat4 uWorld;
uniform mat4 uView;
uniform mat4 uViewProjection;
uniform vec3 uCameraPosition;
uniform vec3 uSunDirection;
uniform vec3 uDiffuse;
uniform vec3 uSpecular;
uniform vec3 uLightSpecular;
uniform float uSpecularPower;
out vec3 vDiffuse;
out vec3 vSpecular;
out float vFogKeep;
void main() {
    mat4 skin = uBones[int(aBoneIndices.x)] * aBoneWeights.x
              + uBones[int(aBoneIndices.y)] * aBoneWeights.y;
    vec4 local = skin * vec4(aPosition, 1.0);
    vec4 world = uWorld * local;
    gl_Position = uViewProjection * world;
    // XNA projection depth is [0,w]; OpenGL / GLES clip depth is [-w,w].
    gl_Position.z = 2.0 * gl_Position.z - gl_Position.w;
    vec3 transformed = mat3(skin) * aNormal;
    float normalLength = length(transformed);
    vec3 boneNormal = normalLength > 1e-6 ? transformed / normalLength : aNormal;
    // Monkey worlds contain positive UNIFORM scale, yaw and translation only.
    vec3 normal = normalize(mat3(uWorld) * boneNormal);
    vec3 eye = normalize(uCameraPosition - world.xyz);
    float sunDot = dot(normal, uSunDirection);
    vec3 halfDirection = normalize(eye + uSunDirection);
    float specular = pow(max(dot(halfDirection, normal), 0.0) * step(0.0, sunDot),
                         uSpecularPower);
    // Same one-light Gouraud material as the authored stock effect, white texture and alpha 1.
    vDiffuse = clamp(uDiffuse * (vec3(0.47, 0.50, 0.42)
                     + vec3(0.78, 0.74, 0.62) * max(sunDot, 0.0)), 0.0, 1.0);
    vSpecular = clamp(uSpecular * uLightSpecular * specular, 0.0, 1.0);
    float depth = -(uView * world).z;
    vFogKeep = 1.0 - clamp((depth - 210.0) / (620.0 - 210.0), 0.0, 1.0);
}
)GLSL";

    inline constexpr const char* FragmentBody = R"GLSL(
in vec3 vDiffuse;
in vec3 vSpecular;
in float vFogKeep;
uniform vec3 uFogColor;
out vec4 fragmentColor;
void main() {
    fragmentColor = vec4(mix(uFogColor, vDiffuse + vSpecular, vFogKeep), 1.0);
}
)GLSL";

    inline std::string Source(bool embedded, bool vertex)
    {
        std::string source = embedded ? "#version 300 es\nprecision highp float;\n"
                                      : "#version 330 core\n";
        if (vertex) source += "uniform mat4 uBones[" + std::to_string(BoneCount) + "];\n";
        source += vertex ? VertexBody : FragmentBody;
        return source;
    }
}
