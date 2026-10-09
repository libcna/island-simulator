#include "IslandSimulator/CanopyLightField.hpp"
#include "IslandSimulator/CanopyProfile.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace island
{
    namespace
    {
        bool Valid(const CanopyOccluder& c)
        {
            return std::isfinite(c.x) && std::isfinite(c.y) && std::isfinite(c.z)
                && std::abs(c.x) < 10000 && std::abs(c.y) < 10000 && std::abs(c.z) < 10000
                && std::isfinite(c.radiusX) && c.radiusX >= 0.01f && c.radiusX <= 100
                && std::isfinite(c.radiusY) && c.radiusY >= 0.01f && c.radiusY <= 100
                && std::isfinite(c.radiusZ) && c.radiusZ >= 0.01f && c.radiusZ <= 100
                && std::isfinite(c.yaw) && std::isfinite(c.density)
                && c.density >= 0 && c.density <= 10;
        }

        TerrainNormal UnitSun(TerrainNormal sun)
        {
            const float length = std::hypot(sun.x, sun.y, sun.z);
            if (!std::isfinite(length) || length < 0.001f) {
                throw std::invalid_argument("Canopy light requires a finite nonzero sun direction");
            }
            sun = {sun.x / length, sun.y / length, sun.z / length};
            if (sun.y < 0.1f) throw std::invalid_argument("Canopy light requires elevated fixed daylight");
            return sun;
        }

        struct RayCaster
        {
            CanopyOccluder shape;
            double cosine, sine, dx, dy, dz, a;

            RayCaster(const CanopyOccluder& caster, const TerrainNormal sun) : shape(caster)
            {
                cosine = std::cos(caster.yaw);
                sine = std::sin(caster.yaw);
                dx = (sun.x * cosine - sun.z * sine) / caster.radiusX;
                dy = sun.y / caster.radiusY;
                dz = (sun.x * sine + sun.z * cosine) / caster.radiusZ;
                a = dx * dx + dy * dy + dz * dz;
            }

            float Length(const float x, const float y, const float z) const
            {
                const double wx = x - shape.x, wz = z - shape.z;
                const double rx = (wx * cosine - wz * sine) / shape.radiusX;
                const double ry = (y - shape.y) / shape.radiusY;
                const double rz = (wx * sine + wz * cosine) / shape.radiusZ;
                const double b = rx * dx + ry * dy + rz * dz;
                const double c = rx * rx + ry * ry + rz * rz - 1.0;
                const double discriminant = b * b - a * c;
                if (discriminant <= 0) return 0;
                const double root = std::sqrt(discriminant);
                const double enter = std::max(0.0, (-b - root) / a);
                const double leave = (-b + root) / a;
                return static_cast<float>(std::max(0.0, leave - enter));
            }
        };
    }

    std::vector<CanopyOccluder> MakeCanopyOccluders(const std::span<const CanopyPlacement> trees)
    {
        std::vector<CanopyOccluder> result;
        result.reserve(trees.size() * 8U);
        for (const auto& tree : trees) {
            if (!std::isfinite(tree.scale) || tree.scale < 0.1f || tree.scale > 4.0f) {
                throw std::invalid_argument("Invalid canopy scale for light baking");
            }
            const float cosine = std::cos(tree.yawRadians), sine = std::sin(tree.yawRadians);
            for (int cluster = 0; cluster < 8; ++cluster) {
                const auto p = MakeCanopyCrownProfile(tree.variant, cluster);
                CanopyOccluder caster{
                    tree.x + (p.x * cosine + p.z * sine) * tree.scale,
                    tree.y + p.y * tree.scale,
                    tree.z + (-p.x * sine + p.z * cosine) * tree.scale,
                    p.width * tree.scale, p.height * tree.scale,
                    p.width * 0.91f * tree.scale, tree.yawRadians};
                if (!Valid(caster)) throw std::invalid_argument("Invalid canopy placement for light baking");
                result.push_back(caster);
            }
        }
        return result;
    }

    float CanopyRayLength(const CanopyOccluder& caster, const float x, const float y,
                          const float z, TerrainNormal toSun)
    {
        if (!Valid(caster) || !std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z)) {
            throw std::invalid_argument("Invalid canopy light ray");
        }
        toSun = UnitSun(toSun);
        return RayCaster(caster, toSun).Length(x, y, z);
    }

    CanopyLightTint CanopyTransmissionTint(float transmission) noexcept
    {
        transmission = std::isfinite(transmission) ? std::clamp(transmission, 0.0f, 1.0f) : 1.0f;
        return {std::lerp(CanopyAmbientTint.red, 1.0f, transmission),
                std::lerp(CanopyAmbientTint.green, 1.0f, transmission),
                std::lerp(CanopyAmbientTint.blue, 1.0f, transmission)};
    }

    float CanopyLightField::TexelCoordinate(const int index, const int resolution) noexcept
    {
        return -HalfExtent + (index + 0.5f) * (2.0f * HalfExtent / resolution);
    }

    CanopyLightField::CanopyLightField(const IslandTerrain& terrain,
        const std::span<const CanopyOccluder> casters, const int resolution, TerrainNormal toSun)
        : resolution_(resolution), toSun_(UnitSun(toSun))
    {
        if (resolution < 16 || resolution > DefaultResolution || (resolution & (resolution - 1))) {
            throw std::invalid_argument("Canopy field resolution must be a power of two from 16 to 1024");
        }
        toSun = UnitSun(toSun);
        for (const auto& caster : casters) {
            if (!Valid(caster)) throw std::invalid_argument("Invalid canopy light caster");
        }
        const std::size_t count = static_cast<std::size_t>(resolution) * resolution;
        texels_.assign(count, 255);
        if (casters.empty()) return;

        // One height query per texel, reused by all lobes; temporary storage dies after baking.
        std::vector<float> heights(count);
        std::vector<float> opticalDepth(count, 0.0f);
        float minimumHeight = std::numeric_limits<float>::max();
        float maximumHeight = std::numeric_limits<float>::lowest();
        for (int row = 0; row < resolution; ++row) {
            for (int column = 0; column < resolution; ++column) {
                const float h = terrain.Height(TexelCoordinate(column, resolution),
                                                TexelCoordinate(row, resolution));
                heights[static_cast<std::size_t>(row) * resolution + column] = h;
                minimumHeight = std::min(minimumHeight, h);
                maximumHeight = std::max(maximumHeight, h);
            }
        }
        const auto index = [resolution](const float world) {
            return std::clamp(static_cast<int>(std::floor((world + HalfExtent)
                * resolution / (2.0f * HalfExtent))), 0, resolution - 1);
        };
        for (const auto& caster : casters) {
            const RayCaster ray(caster, toSun);
            const float rx = static_cast<float>(std::hypot(caster.radiusX * ray.cosine,
                                                          caster.radiusZ * ray.sine));
            const float rz = static_cast<float>(std::hypot(caster.radiusX * ray.sine,
                                                          caster.radiusZ * ray.cosine));
            const float sx = toSun.x / toSun.y, sz = toSun.z / toSun.y;
            const float x0 = caster.x - sx * (caster.y - minimumHeight);
            const float x1 = caster.x - sx * (caster.y - maximumHeight);
            const float z0 = caster.z - sz * (caster.y - minimumHeight);
            const float z1 = caster.z - sz * (caster.y - maximumHeight);
            const float ex = rx + std::abs(sx) * caster.radiusY;
            const float ez = rz + std::abs(sz) * caster.radiusY;
            const int firstColumn = index(std::min(x0, x1) - ex);
            const int lastColumn = index(std::max(x0, x1) + ex);
            const int firstRow = index(std::min(z0, z1) - ez);
            const int lastRow = index(std::max(z0, z1) + ez);
            for (int row = firstRow; row <= lastRow; ++row) {
                for (int column = firstColumn; column <= lastColumn; ++column) {
                    const auto offset = static_cast<std::size_t>(row) * resolution + column;
                    opticalDepth[offset] += caster.density * ray.Length(
                        TexelCoordinate(column, resolution), heights[offset],
                        TexelCoordinate(row, resolution));
                }
            }
        }
        for (std::size_t i = 0; i < count; ++i) {
            texels_[i] = static_cast<std::uint8_t>(std::lround(255.0f * std::exp(-opticalDepth[i])));
        }
    }

    float CanopyLightField::SampleReceiver(const float x, const float y, const float z,
                                          const float groundHeight) const noexcept
    {
        if (!std::isfinite(y) || !std::isfinite(groundHeight)) return 1.0f;
        const float height = std::max(0.0f, y - groundHeight);
        return Sample(x - toSun_.x / toSun_.y * height, z - toSun_.z / toSun_.y * height);
    }

    float CanopyLightField::SampleReceiverFootprint(const float x, const float y, const float z,
                                                   const float groundHeight, const float radius) const noexcept
    {
        if (!std::isfinite(radius) || radius < 0) return 1.0f;
        return SampleReceiver(x, y, z, groundHeight) * 0.5f
            + (SampleReceiver(x - radius, y, z, groundHeight)
               + SampleReceiver(x + radius, y, z, groundHeight)
               + SampleReceiver(x, y, z - radius, groundHeight)
               + SampleReceiver(x, y, z + radius, groundHeight)) * 0.125f;
    }

    float CanopyLightField::Sample(const float x, const float z) const noexcept
    {
        if (!std::isfinite(x) || !std::isfinite(z) || std::abs(x) > HalfExtent
            || std::abs(z) > HalfExtent) return 1.0f;
        const float column = std::clamp((x + HalfExtent) * resolution_ / (2 * HalfExtent) - 0.5f,
                                         0.0f, static_cast<float>(resolution_ - 1));
        const float row = std::clamp((z + HalfExtent) * resolution_ / (2 * HalfExtent) - 0.5f,
                                      0.0f, static_cast<float>(resolution_ - 1));
        const int ix = static_cast<int>(column), iz = static_cast<int>(row);
        const auto value = [&](const int cx, const int cz) {
            return texels_[static_cast<std::size_t>(cz) * resolution_ + cx] / 255.0f;
        };
        return std::lerp(std::lerp(value(ix, iz), value(std::min(ix + 1, resolution_ - 1), iz), column - ix),
                         std::lerp(value(ix, std::min(iz + 1, resolution_ - 1)),
                                   value(std::min(ix + 1, resolution_ - 1), std::min(iz + 1, resolution_ - 1)),
                                   column - ix), row - iz);
    }
}
