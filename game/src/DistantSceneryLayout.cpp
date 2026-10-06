#include "IslandSimulator/DistantSceneryLayout.hpp"

namespace island
{
    DistantSceneryLayout::DistantSceneryLayout() noexcept
        : islets_({{
              {0.0f, -610.0f, 42.0f, 29.0f, 10.5f, 0.13f, 2U},
              {105.0f, -670.0f, 25.0f, 17.0f, 6.5f, -0.40f, 1U},
              {590.0f, 260.0f, 38.0f, 25.0f, 9.2f, 0.20f, 2U},
              {-430.0f, 510.0f, 31.0f, 22.0f, 7.8f, -0.30f, 0U},
              {-660.0f, -80.0f, 24.0f, 17.0f, 5.9f, 0.40f, 1U},
          }})
    {
    }
}
