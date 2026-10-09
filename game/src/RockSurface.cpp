#include "IslandSimulator/RockSurface.hpp"

#include <cmath>
#include <numbers>
#include <stdexcept>

namespace island
{
    RockSurface BuildRockSurface(const int variant, const bool farMesh)
    {
        if (variant < 0 || variant > 3) throw std::invalid_argument("Rock variant must be in [0, 3]");
        RockSurface result;
        const int rings = farMesh ? 4 : 10;
        const int slices = farMesh ? 7 : 24;
        const float height = 0.98f + 0.045f * variant;
        for (int ring = 0; ring <= rings; ++ring) {
            const float t = static_cast<float>(ring) / rings;
            const float radialBase = (0.72f + 0.38f * std::sin(t * std::numbers::pi_v<float>))
                * (1 - 0.55f * t);
            for (int slice = 0; slice < slices; ++slice) {
                const float angle = 2 * std::numbers::pi_v<float> * static_cast<float>(slice) / slices;
                const float crown = (farMesh ? 0 : t*t*t*t)
                    * (0.035f * std::sin(angle*(2+variant) + variant*0.7f)
                       + 0.018f * std::sin(angle*5 - variant*0.9f));
                const float radius = radialBase
                    * (0.88f + 0.11f * std::sin(angle*(2+variant) + t*7 + variant*1.3f));
                result.vertices.push_back({{std::cos(angle)*radius, t*height+crown,
                                             std::sin(angle)*radius}, {0,0,0}});
            }
        }
        for (int ring = 0; ring < rings; ++ring) {
            for (int slice = 0; slice < slices; ++slice) {
                const auto lower = static_cast<std::uint16_t>(ring*slices+slice);
                const auto lowerNext = static_cast<std::uint16_t>(ring*slices+(slice+1)%slices);
                const auto upper = static_cast<std::uint16_t>((ring+1)*slices+slice);
                const auto upperNext = static_cast<std::uint16_t>((ring+1)*slices+(slice+1)%slices);
                result.indices.insert(result.indices.end(), {lower,upper,lowerNext,lowerNext,upper,upperNext});
            }
        }
        const auto cap = static_cast<std::uint16_t>(result.vertices.size());
        result.vertices.push_back({{(variant-1.5f)*0.065f, height+(farMesh ? 0.045f : 0.105f),
                                     std::sin(variant*1.7f)*0.065f}, {0,0,0}});
        for (int slice = 0; slice < slices; ++slice) {
            result.indices.insert(result.indices.end(), {
                static_cast<std::uint16_t>(rings*slices+slice), cap,
                static_cast<std::uint16_t>(rings*slices+(slice+1)%slices)});
        }
        for (std::size_t i = 0; i < result.indices.size(); i += 3) {
            const auto& a = result.vertices[result.indices[i]].position;
            const auto& b = result.vertices[result.indices[i+1]].position;
            const auto& c = result.vertices[result.indices[i+2]].position;
            const float ux = b[0]-a[0], uy = b[1]-a[1], uz = b[2]-a[2];
            const float vx = c[0]-a[0], vy = c[1]-a[1], vz = c[2]-a[2];
            const std::array normal{uy*vz-uz*vy, uz*vx-ux*vz, ux*vy-uy*vx};
            for (std::size_t corner = 0; corner < 3; ++corner) {
                auto& sum = result.vertices[result.indices[i+corner]].normal;
                for (std::size_t axis = 0; axis < 3; ++axis) sum[axis] += normal[axis];
            }
        }
        for (auto& vertex : result.vertices) {
            auto& n = vertex.normal;
            const float length = std::sqrt(n[0]*n[0]+n[1]*n[1]+n[2]*n[2]);
            if (!std::isfinite(length) || length < 0.000001f) {
                throw std::runtime_error("Rock surface has an undefined vertex normal");
            }
            for (auto& component : n) component /= length;
        }
        return result;
    }
}
