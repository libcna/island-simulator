#include "IslandSimulator/DistantSceneryLayout.hpp"

namespace island
{
    DistantSceneryLayout::DistantSceneryLayout() noexcept
        : islets_({{
              // Nearer wooded islet, a mid companion, one treeless sandbar and two far
              // silhouettes on the diagonals, so contrast steps down with distance.
              {0.0f, -575.0f, 42.0f, 29.0f, 10.5f, 0.13f, 2U},
              {150.0f, -690.0f, 25.0f, 17.0f, 6.5f, -0.40f, 1U},
              {600.0f, 300.0f, 38.0f, 25.0f, 9.2f, 0.20f, 2U},
              {-700.0f, 720.0f, 31.0f, 22.0f, 7.8f, -0.30f, 0U},
              {-760.0f, -420.0f, 26.0f, 18.0f, 2.2f, 0.40f, 3U},
          }})
    {
    }
}
