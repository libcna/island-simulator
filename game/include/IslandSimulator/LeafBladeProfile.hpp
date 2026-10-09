#pragma once

namespace island
{
    /** @brief Canonical leaf point: side offset, height, and distance along the blade. */
    struct LeafBladePoint
    {
        float across;
        float height;
        float along;
    };

    /** @brief Curved, creased understory blade shared by portable mesh generation and CPU tests.
     * Coordinates are normalized (along 0..1, across -1..1); invalid input is rejected before
     * generating GPU data. The root is at zero and the supported tropical blades droop above it.
     */
    [[nodiscard]] LeafBladePoint SampleLeafBlade(float along, float across, float length,
                                                float halfWidth, float lift, float phase);
    /** @brief Unit normal in the same across/height/along basis; finite at tapered endpoints. */
    [[nodiscard]] LeafBladePoint SampleLeafBladeNormal(float along, float across, float length,
                                                      float halfWidth, float lift, float phase);
}
