#pragma once

namespace island
{
    /** @brief Horizontal circular solid used for inexpensive trunk/rock collision. */
    struct CircularObstacle
    {
        float x = 0.0f;
        float z = 0.0f;
        float radius = 0.0f;
    };
}
