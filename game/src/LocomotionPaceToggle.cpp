#include "IslandSimulator/LocomotionPaceToggle.hpp"

namespace island
{
    bool LocomotionPaceToggle::Update(const bool shiftDown,
                                      const bool togglingEnabled) noexcept
    {
        const bool changed = togglingEnabled && shiftDown && !shiftWasDown_;
        if (changed) {
            runningSelected_ = !runningSelected_;
        }
        shiftWasDown_ = shiftDown;
        return changed;
    }
}
