#include "IslandSimulator/SurfaceNormals.hpp"

#include <cmath>
#include <stdexcept>
#include <string>

namespace island
{
    std::vector<std::array<float, 3>> BuildOutwardSurfaceNormals(
        std::span<const std::array<float, 3>> positions,
        std::span<const std::uint16_t> indices,
        std::span<const std::array<float, 3>> outwardReferences)
    {
        if (positions.size() != outwardReferences.size())
            throw std::invalid_argument("Outward normals require one reference per vertex");
        auto normals = BuildSurfaceNormals(positions, indices);
        for (std::size_t i = 0; i < normals.size(); ++i) {
            const auto& r = outwardReferences[i];
            const double length = std::hypot(r[0], r[1], r[2]);
            const double dot = normals[i][0]*r[0] + normals[i][1]*r[1] + normals[i][2]*r[2];
            if (!std::isfinite(length) || length < 1e-12 || !std::isfinite(dot)
                || dot <= length * 1e-6)
                throw std::invalid_argument("Procedural surface normal is not outward at vertex " + std::to_string(i));
        }
        return normals;
    }

    std::vector<std::array<float, 3>> BuildSurfaceNormals(
        std::span<const std::array<float, 3>> positions,
        std::span<const std::uint16_t> indices)
    {
        if (positions.empty() || indices.empty() || indices.size() % 3 != 0) {
            throw std::invalid_argument("Surface normals require a nonempty triangle mesh");
        }
        for (const auto& p : positions) {
            for (float v : p) {
                if (!std::isfinite(v)) throw std::invalid_argument("Non-finite normal input");
            }
        }
        std::vector<std::array<double, 3>> sums(positions.size());
        for (std::size_t i = 0; i < indices.size(); i += 3) {
            for (int corner = 0; corner < 3; ++corner) {
                if (indices[i+corner] >= positions.size()) {
                    throw std::invalid_argument("Surface normal index out of range");
                }
            }
            const auto& a = positions[indices[i]];
            const auto& b = positions[indices[i+1]];
            const auto& c = positions[indices[i+2]];
            std::array<double, 3> u{}, v{};
            for (int axis = 0; axis < 3; ++axis) {
                u[axis] = static_cast<double>(b[axis]) - a[axis];
                v[axis] = static_cast<double>(c[axis]) - a[axis];
            }
            const std::array face{u[1]*v[2]-u[2]*v[1], u[2]*v[0]-u[0]*v[2],
                                  u[0]*v[1]-u[1]*v[0]};
            if (std::hypot(face[0],face[1],face[2]) <= 1e-15) {
                throw std::invalid_argument("Degenerate normal triangle");
            }
            for (int corner = 0; corner < 3; ++corner) {
                for (int axis = 0; axis < 3; ++axis) sums[indices[i+corner]][axis] += face[axis];
            }
        }
        std::vector<std::array<float, 3>> result;
        result.reserve(positions.size());
        for (const auto& sum : sums) {
            const double length = std::hypot(sum[0],sum[1],sum[2]);
            if (!std::isfinite(length) || length <= 1e-15) {
                throw std::invalid_argument("Undefined normal at unused or cancelling vertex");
            }
            result.push_back({static_cast<float>(sum[0]/length), static_cast<float>(sum[1]/length),
                              static_cast<float>(sum[2]/length)});
        }
        return result;
    }
}
