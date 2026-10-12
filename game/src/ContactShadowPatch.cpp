#include "IslandSimulator/ContactShadowPatch.hpp"

#include "IslandSimulator/EnvironmentLighting.hpp"
#include "IslandSimulator/IslandTerrain.hpp"
#include "IslandSimulator/NaturalPropLayout.hpp"
#include "IslandSimulator/ShipwreckLayout.hpp"
#include "IslandSimulator/HutLayout.hpp"
#include "IslandSimulator/TerrainMeshLayout.hpp"
#include "IslandSimulator/RenderedTerrainSurface.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <stdexcept>

namespace island
{
    std::array<ContactFootprint,7> HutContactFootprints(const HutPlacement& hut) noexcept
    {
        const float x=std::sin(hut.yawRadians),z=std::cos(hut.yawRadians);
        std::array<ContactFootprint,7> result{};
        // Broad soft grounding under the raised floor, not a directional roof silhouette.
        result[0]={hut.x,hut.z,x,z,HutLayout::Depth/2+HutLayout::VerandaDepth+0.18f,
            HutLayout::Width/2+0.3f,0.28f};
        for(std::size_t i=0;i<hut.footings.size();++i)
            result[i+1]={hut.footings[i].x,hut.footings[i].z,x,z,0.25f,0.25f,0.34f};
        return result;
    }
    namespace
    {
        struct Point { double x, z, opacity; };

        double Side(const Point& a, const Point& b, const Point& p)
        {
            return (b.x - a.x) * (p.z - a.z) - (b.z - a.z) * (p.x - a.x);
        }

        std::vector<Point> Clip(const std::vector<Point>& input, const Point& a, const Point& b)
        {
            std::vector<Point> result;
            if (input.empty()) return result;
            Point previous = input.back();
            double previousSide = Side(a, b, previous);
            for (const auto& current : input) {
                const double side = Side(a, b, current);
                if ((side >= 0) != (previousSide >= 0)) {
                    const double t = previousSide / (previousSide - side);
                    result.push_back({previous.x + (current.x - previous.x) * t,
                                      previous.z + (current.z - previous.z) * t,
                                      previous.opacity + (current.opacity - previous.opacity) * t});
                }
                if (side >= 0) result.push_back(current);
                previous = current;
                previousSide = side;
            }
            return result;
        }

        void BiasFromSun(ContactFootprint& footprint, const float height) noexcept
        {
            footprint.x -= environment::SunDirectionX / environment::SunDirectionY * height;
            footprint.z -= environment::SunDirectionZ / environment::SunDirectionY * height;
        }
    }

    ContactFootprint LandRockContactFootprint(const RockPlacement& rock) noexcept
    {
        const float dx = std::cos(rock.yawRadians), dz = -std::sin(rock.yawRadians);
        const float offset = 0.08f * std::max(rock.scaleX, rock.scaleZ);
        return {rock.x + dx*offset, rock.z + dz*offset, dx, dz,
                0.78f*rock.scaleX, 0.68f*rock.scaleZ, 52.0f/255.0f};
    }

    ContactFootprint LogContactFootprint(const FallenLogPlacement& log) noexcept
    {
        return {log.x, log.z, std::cos(log.yawRadians), std::sin(log.yawRadians),
                log.length*0.52f, log.radius*1.45f, 45.0f/255.0f};
    }

    ContactFootprint DriftwoodContactFootprint(const DriftwoodPlacement& wood) noexcept
    {
        return {wood.x, wood.z, std::cos(wood.yawRadians), std::sin(wood.yawRadians),
                wood.length*0.52f, wood.radius*1.70f, 30.0f/255.0f};
    }

    ContactFootprint RockContactFootprint(const RockPlacement& rock) noexcept
    {
        ContactFootprint result{rock.x, rock.z, std::cos(rock.yawRadians), -std::sin(rock.yawRadians),
                                rock.scaleX * 1.12f, rock.scaleZ * 1.08f, 0.30f};
        BiasFromSun(result, rock.scaleY * 0.22f);
        return result;
    }

    std::array<ContactFootprint, 2> HullContactFootprints(const ShipwreckPlacement& ship) noexcept
    {
        std::array<ContactFootprint, 2> result;
        const float dx = std::sin(ship.yawRadians), dz = std::cos(ship.yawRadians);
        for (std::size_t index = 0; index < result.size(); ++index) {
            const float along = (index == 0 ? -0.274f : 0.279f) * ship.length;
            result[index] = {ship.x + dx * along, ship.z + dz * along, dx, dz,
                             ship.length * 0.218f, ship.beam * 0.49f, 0.42f};
            BiasFromSun(result[index], 0.45f);
        }
        return result;
    }

    ContactFootprint DebrisContactFootprint(const ShipwreckDebrisPlacement& debris) noexcept
    {
        const float width = debris.width
            * (debris.kind == ShipwreckDebrisKind::HullFragment ? 1.32f : 0.75f);
        ContactFootprint result{debris.x, debris.z, std::sin(debris.yawRadians),
                                std::cos(debris.yawRadians), debris.length * 0.55f, width, 0.26f};
        BiasFromSun(result, 0.12f);
        return result;
    }

    std::vector<ContactPatchVertex> BuildContactPatch(const IslandTerrain& terrain,
                                                     const ContactFootprint& footprint,
                                                     const ContactPatchDetail detail,
                                                     const RenderedTerrainSurface* cachedSurface)
    {
        const auto& f = footprint;
        const bool small=detail==ContactPatchDetail::SmallAnimal;
        const float minimumRadius=small?.001f:.05f,maximumRadius=small?.2f:8;
        for (float value : {f.x, f.z, f.directionX, f.directionZ, f.alongRadius,
                           f.acrossRadius, f.opacity}) {
            if (!std::isfinite(value)) throw std::invalid_argument("Non-finite contact footprint");
        }
        if (std::abs(f.x) > TerrainMeshLayout::Extent || std::abs(f.z) > TerrainMeshLayout::Extent
            || std::abs(std::hypot(f.directionX, f.directionZ) - 1) > 0.0001f
            || f.alongRadius < minimumRadius || f.alongRadius > maximumRadius || f.acrossRadius < minimumRadius
            || f.acrossRadius > maximumRadius || f.opacity < 0 || f.opacity > 1) {
            throw std::invalid_argument("Contact footprint exceeds its geometry contract");
        }
        std::vector<ContactPatchVertex> result;
        if (detail != ContactPatchDetail::Compact && detail != ContactPatchDetail::Coastal && !small) {
            throw std::invalid_argument("Unknown contact patch detail profile");
        }
        // Small land props retain their old twelve-sector linear falloff; only terrain clipping
        // introduces extra triangles. Larger coastal patches use a rounder, smoother profile.
        const int segments = detail == ContactPatchDetail::Coastal ? 24 : 12;
        const int rings = detail == ContactPatchDetail::Coastal ? 3 : 1;
        const auto point = [&](int ring, int segment) {
            const double radius = static_cast<double>(ring) / rings;
            const double angle = static_cast<double>(segment) * 2 * std::numbers::pi / segments;
            const double along = std::cos(angle) * f.alongRadius * radius;
            const double across = std::sin(angle) * f.acrossRadius * radius;
            const double fade = 1 - radius * radius * (3 - 2 * radius);
            return Point{f.x + f.directionX * along - f.directionZ * across,
                         f.z + f.directionZ * along + f.directionX * across, f.opacity * fade};
        };
        const auto grid = [](double position) {
            return std::clamp(static_cast<int>(std::floor((position + TerrainMeshLayout::Extent)
                * TerrainMeshLayout::QuadsPerAxis / (2 * TerrainMeshLayout::Extent))),
                0, TerrainMeshLayout::QuadsPerAxis - 1);
        };
        const auto triangle = [&](Point a, Point b, Point c) {
            const int firstX = grid(std::min({a.x, b.x, c.x}));
            const int lastX = grid(std::max({a.x, b.x, c.x}));
            const int firstZ = grid(std::min({a.z, b.z, c.z}));
            const int lastZ = grid(std::max({a.z, b.z, c.z}));
            for (int row = firstZ; row <= lastZ; ++row) {
                for (int column = firstX; column <= lastX; ++column) {
                    const double x0 = TerrainMeshLayout::Coordinate(column);
                    const double x1 = TerrainMeshLayout::Coordinate(column + 1);
                    const double z0 = TerrainMeshLayout::Coordinate(row);
                    const double z1 = TerrainMeshLayout::Coordinate(row + 1);
                    const Point tl{x0,z0,0}, tr{x1,z0,0}, bl{x0,z1,0}, br{x1,z1,0};
                    // Positive 2D winding; same top-right/bottom-left diagonal as TerrainRenderer.
                    for (const std::array<Point,3>& cell : {std::array{tl,tr,bl}, std::array{tr,br,bl}}) {
                        std::vector<Point> polygon{a,b,c};
                        for (int edge = 0; edge < 3; ++edge) {
                            polygon = Clip(polygon, cell[edge], cell[(edge + 1) % 3]);
                        }
                        const double area = Side(cell[0], cell[1], cell[2]);
                        std::array<float,3> heights;
                        for (int corner = 0; corner < 3; ++corner) {
                            const float x=static_cast<float>(cell[corner].x),z=static_cast<float>(cell[corner].z);
                            heights[corner] = cachedSurface ? cachedSurface->Sample(x,z).height : terrain.Height(x,z);
                        }
                        const auto emit = [&](const Point& p) {
                            const double w0 = Side(cell[1],cell[2],p) / area;
                            const double w1 = Side(cell[2],cell[0],p) / area;
                            const double height = heights[0]*w0 + heights[1]*w1 + heights[2]*(1-w0-w1);
                            result.push_back({static_cast<float>(p.x),
                                static_cast<float>(height) + (small?SmallContactSurfaceOffset:ContactPatchSurfaceOffset),
                                static_cast<float>(p.z), static_cast<float>(std::clamp(p.opacity,0.0,1.0))});
                        };
                        for (std::size_t index = 1; index + 1 < polygon.size(); ++index) {
                            if (std::abs(Side(polygon[0], polygon[index], polygon[index+1])) < 1e-9) continue;
                            emit(polygon[0]); emit(polygon[index]); emit(polygon[index+1]);
                        }
                    }
                }
            }
        };
        for (int segment = 0; segment < segments; ++segment) {
            triangle(point(0,0), point(1,segment), point(1,segment+1));
            for (int ring = 1; ring < rings; ++ring) {
                triangle(point(ring,segment), point(ring+1,segment), point(ring,segment+1));
                triangle(point(ring,segment+1), point(ring+1,segment), point(ring+1,segment+1));
            }
        }
        return result;
    }
}
