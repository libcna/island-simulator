#include "IslandSimulator/MonkeySkinningShader.hpp"
#include "IslandSimulator/MonkeyTroop.hpp"
#include "IslandSimulator/RenderedTerrainSurface.hpp"
#include <iostream>
#include <stdexcept>

int main()
try {
    using namespace island::MonkeySkinningShader;
    static_assert(BoneCount == 14);
    static_assert(VertexUniformVectorBudget < 128);
    const island::IslandTerrain terrain;
    const island::RenderedTerrainSurface rendered(terrain);
    const island::MonkeyTroop troop(terrain);
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
        require(vertex.find("[72]") == std::string::npos);
        require(fragment.find("uBones") == std::string::npos);
        require(vertex.find("layout(location=3) in vec4 aBoneWeights") != std::string::npos);
        require(vertex.find("layout(location=4) in vec4 aBoneIndices") != std::string::npos);
        require(vertex.find("uBones[int(aBoneIndices.x)] * aBoneWeights.x") != std::string::npos);
        require(vertex.find("gl_Position.z = 2.0 * gl_Position.z - gl_Position.w") != std::string::npos);
        require(vertex.find("vDiffuse = clamp") != std::string::npos);
        require(vertex.find("vSpecular = clamp") != std::string::npos);
        require(vertex.find("float depth = -(uView * world).z") != std::string::npos);
        require(fragment.find("mix(uFogColor, vDiffuse + vSpecular, vFogKeep)") != std::string::npos);
    }
    std::cout << "Monkey shader: 14-bone palette, budget and both dialect contracts pass\n";
    return 0;
} catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
}
