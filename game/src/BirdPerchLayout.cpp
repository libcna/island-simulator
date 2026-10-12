#include "IslandSimulator/BirdPerchLayout.hpp"

#include "IslandSimulator/CanopyBranchProfile.hpp"
#include "IslandSimulator/IslandTerrain.hpp"
#include "IslandSimulator/JungleLayout.hpp"
#include "IslandSimulator/NaturalPropLayout.hpp"
#include "IslandSimulator/RockSurface.hpp"
#include "IslandSimulator/ShipwreckLayout.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>
#include <stdexcept>

namespace island
{
    namespace
    {
        [[nodiscard]] std::array<float, 3> TransformTreePoint(
            const CanopyPlacement& tree, const BranchPoint& point) noexcept
        {
            const float cosine = std::cos(tree.yawRadians);
            const float sine = std::sin(tree.yawRadians);
            return {
                tree.x + tree.scale * (point[0] * cosine + point[2] * sine),
                tree.y + tree.scale * point[1],
                tree.z + tree.scale * (-point[0] * sine + point[2] * cosine),
            };
        }

        [[nodiscard]] bool Separated(const std::vector<BirdPerchSite>& sites,
                                     const float x, const float z,
                                     const float minimumDistance) noexcept
        {
            return std::ranges::all_of(sites, [=](const BirdPerchSite& site) {
                return std::hypot(site.x - x, site.z - z) >= minimumDistance;
            });
        }

        struct ShipAxes
        {
            std::array<float, 3> right;
            std::array<float, 3> up;
            std::array<float, 3> forward;
        };

        [[nodiscard]] ShipAxes MakeShipAxes(const ShipwreckPlacement& ship) noexcept
        {
            const float yawCosine = std::cos(ship.yawRadians);
            const float yawSine = std::sin(ship.yawRadians);
            const std::array levelRight{yawCosine, 0.0f, -yawSine};
            const std::array levelForward{yawSine, 0.0f, yawCosine};
            const float heelCosine = std::cos(ship.heelRadians);
            const float heelSine = std::sin(ship.heelRadians);
            const std::array rolledRight{
                levelRight[0] * heelCosine,
                heelSine,
                levelRight[2] * heelCosine,
            };
            const std::array rolledUp{
                -levelRight[0] * heelSine,
                heelCosine,
                -levelRight[2] * heelSine,
            };
            const float pitchCosine = std::cos(ship.pitchRadians);
            const float pitchSine = std::sin(ship.pitchRadians);
            ShipAxes result;
            result.right = rolledRight;
            for (std::size_t axis = 0; axis < 3U; ++axis) {
                result.up[axis] = rolledUp[axis] * pitchCosine
                    - levelForward[axis] * pitchSine;
                result.forward[axis] = levelForward[axis] * pitchCosine
                    + rolledUp[axis] * pitchSine;
            }
            return result;
        }

        [[nodiscard]] std::array<float, 3> ShipPoint(
            const ShipwreckPlacement& ship, const ShipAxes& axes,
            const float x, const float y, const float z) noexcept
        {
            return {
                ship.x + axes.right[0] * x + axes.up[0] * y + axes.forward[0] * z,
                ship.y + axes.right[1] * x + axes.up[1] * y + axes.forward[1] * z,
                ship.z + axes.right[2] * x + axes.up[2] * y + axes.forward[2] * z,
            };
        }
    }

    BirdPerchLayout::BirdPerchLayout(const IslandTerrain& terrain,
                                     const JungleLayout& jungle,
                                     const NaturalPropLayout& props,
                                     const ShipwreckLayout& shipwreck)
    {
        sites_.reserve(SiteCount);
        std::size_t acceptedBranches = 0U;
        const auto& trees = jungle.Canopy();
        for (std::size_t index = 0; index < trees.size()
             && acceptedBranches < BranchSiteCount; ++index) {
            const CanopyPlacement& tree = trees[index];
            const float radius = std::hypot(tree.x, tree.z);
            if (tree.variant >= 8U || radius < 48.0f || radius > 155.0f
                || tree.scale > 1.48f || index % 5U != 1U) {
                continue;
            }
            const int cluster = 1 + static_cast<int>((index * 3U) % 7U);
            const CanopyBranchProfile branch = MakeCanopyBranchProfile(tree.variant, cluster);
            const auto highest = std::max_element(
                branch.vertices.begin() + 12, branch.vertices.end(),
                [](const CanopyBranchVertex& left, const CanopyBranchVertex& right) {
                    return left.position[1] < right.position[1];
                });
            const auto point = TransformTreePoint(tree, highest->position);
            if (!Separated(sites_, point[0], point[2], 34.0f)) {
                continue;
            }
            const auto middle = TransformTreePoint(tree, branch.vertices[6].position);
            sites_.push_back({point[0], point[1], point[2],
                              std::atan2(point[2] - middle[2], point[0] - middle[0]),
                              BirdPerchKind::Branch});
            ++acceptedBranches;
        }

        std::size_t acceptedRocks = 0U;
        for (const RockPlacement& rock : props.Rocks()) {
            if (acceptedRocks >= RockSiteCount) {
                break;
            }
            if (!rock.solid || rock.offshore || rock.scaleY < 1.15f
                || std::hypot(rock.x, rock.z) > 175.0f
                || !Separated(sites_, rock.x, rock.z, 28.0f)) {
                continue;
            }
            const float localTop = 1.085f + 0.045f * static_cast<float>(rock.variant);
            const float capX = (static_cast<float>(rock.variant) - 1.5f) * 0.065f;
            const float capZ = std::sin(static_cast<float>(rock.variant) * 1.7f) * 0.065f;
            const float cosine = std::cos(rock.yawRadians);
            const float sine = std::sin(rock.yawRadians);
            const float x = rock.x + rock.scaleX * (capX * cosine + capZ * sine);
            const float z = rock.z + rock.scaleZ * (-capX * sine + capZ * cosine);
            sites_.push_back({x, rock.y + rock.scaleY * localTop, z,
                              rock.yawRadians + std::numbers::pi_v<float> * 0.5f,
                              BirdPerchKind::Rock});
            ++acceptedRocks;
        }

        const ShipwreckPlacement& ship = shipwreck.Ship();
        const ShipAxes axes = MakeShipAxes(ship);
        // Midpoint of the rendered cross-tree beam, plus its 0.13 m surface radius.
        const auto wreckPoint = ShipPoint(ship, axes, 0.45f, 5.78f, -1.75f);
        sites_.push_back({wreckPoint[0], wreckPoint[1], wreckPoint[2],
                          std::atan2(axes.forward[2], axes.forward[0]),
                          BirdPerchKind::Wreck});

        if (sites_.size() != SiteCount) {
            throw std::runtime_error("Unable to derive the complete bounded bird perch layout");
        }
        for (const BirdPerchSite& site : sites_) {
            if (!std::isfinite(site.x) || !std::isfinite(site.y)
                || !std::isfinite(site.z) || !std::isfinite(site.headingRadians)
                || site.y < terrain.Height(site.x, site.z) + 0.35f) {
                throw std::runtime_error("Bird perch is not finite and clear of terrain");
            }
        }
    }
}
