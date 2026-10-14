#include "IslandSimulator/MonkeySkinningShader.hpp"
#include "IslandSimulator/MonkeyTroop.hpp"
#include "IslandSimulator/RenderedTerrainSurface.hpp"
#include "IslandSimulator/MonkeyGroundGait.hpp"
#include <iostream>
#include <stdexcept>

int main()
try {
    using namespace island::MonkeySkinningShader;
    static_assert(BoneCount == 14);
    static_assert(SoleCount == 4);
    static_assert(VertexUniformVectorBudget == 92);
    static_assert(VertexUniformVectorBudget < 128);
    const island::IslandTerrain terrain;
    const island::RenderedTerrainSurface rendered(terrain);
    const island::MonkeyTroop troop(terrain);
    // Exercise the renderer's yaw/scale/terrain-space conversion on actual island
    // mesh triangles, not just synthetic flat-ground fixtures.
    for (const auto& monkey : troop.Monkeys()) {
        for (int direction = 0; direction < 8; ++direction) {
            const float yaw = direction * std::numbers::pi_v<float> / 4.0f;
            for (int step = 0; step < 40; ++step) {
                const auto flat = island::MonkeyGroundGait::Sample(step / 40.0f);
                std::array<float, 4> heights{};
                for (std::size_t i = 0; i < heights.size(); ++i) {
                    const auto sole = flat.limbs[i].sole;
                    const float x = monkey.x + monkey.scale * (sole.x * std::cos(yaw) + sole.z * std::sin(yaw));
                    const float z = monkey.z + monkey.scale * (-sole.x * std::sin(yaw) + sole.z * std::cos(yaw));
                    heights[i] = (rendered.Sample(x, z).height - monkey.y) / monkey.scale;
                }
                const auto fitted = island::MonkeyGroundGait::FitGround(flat, heights);
                if (fitted.reachLimited) throw std::runtime_error("initial island monkey footprint is unreachable");
                for (std::size_t i = 0; i < heights.size(); ++i)
                    if (std::fabs(fitted.limbs[i].sole.y - flat.limbs[i].sole.y - heights[i]) > 1e-5f)
                        throw std::runtime_error("island monkey contact differs from rendered ground");
            }
        }
    }
    for (const auto& monkey : troop.Monkeys())
        std::cout << "monkey root " << monkey.x << ',' << monkey.y << ',' << monkey.z
                  << " rendered floor " << rendered.Sample(monkey.x, monkey.z).height << '\n';
    for (bool embedded : {false, true}) {
        const auto vertex = Source(embedded, true);
        const auto fragment = Source(embedded, false);
        const auto require = [](bool condition) {
            if (!condition) throw std::runtime_error("monkey shader contract failed");
        };
        const std::string prefix = embedded ? "#version 300 es" : "#version 330 core";
        require(vertex.starts_with(prefix) && fragment.starts_with(prefix));
        require(vertex.find("uniform mat4 uBones[14];") != std::string::npos);
        require(vertex.find("uniform mat4 uSoles[4];") != std::string::npos);
        require(vertex.find("skin = uSoles[int(aBoneIndices.w) - 1]") != std::string::npos);
        require(vertex.find("[72]") == std::string::npos);
        require(fragment.find("uBones") == std::string::npos);
        require(vertex.find("layout(location=3) in vec4 aBoneWeights") != std::string::npos);
        require(vertex.find("layout(location=4) in vec4 aBoneIndices") != std::string::npos);
        require(vertex.find("uBones[int(aBoneIndices.x)] * aBoneWeights.x") != std::string::npos);
        require(vertex.find("uBones[int(aBoneIndices.y)] * aBoneWeights.y") != std::string::npos);
        require(vertex.find("gl_Position.z = 2.0 * gl_Position.z - gl_Position.w") != std::string::npos);
        require(vertex.find("vDiffuse = clamp") != std::string::npos);
        require(vertex.find("aTexCoord.y < 0.0 ? clamp(-aTexCoord.y, 0.8, 1.0) : 1.0") != std::string::npos);
        require(vertex.find("aTexCoord.x < -0.9 ? 0.43") != std::string::npos);
        require(vertex.find("aTexCoord.x < -0.6 ? 0.76 : 1.0") != std::string::npos);
        require(vertex.find("vec3(0.47, 0.50, 0.42) * faceAmbient") != std::string::npos);
        require(vertex.find("vSpecular = clamp") != std::string::npos);
        require(vertex.find("float depth = -(uView * world).z") != std::string::npos);
        require(vertex.find("layout(location=2) in vec2 aTexCoord") != std::string::npos);
        require(vertex.find("vFurUv = aTexCoord") != std::string::npos);
        require(fragment.find("texture(uFurTexture, vFurUv)") != std::string::npos);
        require(fragment.find("mix(vec3(1.0), fur, uFurAmount)") != std::string::npos);
        require(fragment.find("mix(uFogColor, diffuse + vSpecular, vFogKeep)") != std::string::npos);
    }
    std::cout << "Monkey shader: 14-bone palette, budget and both dialect contracts pass\n";
    return 0;
} catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
}
