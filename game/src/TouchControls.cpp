#include "IslandSimulator/TouchControls.hpp"

#include <algorithm>
#include <cmath>

namespace island
{
    namespace
    {
        constexpr float MaximumDisplayExtent = 65536.0f;

        [[nodiscard]] bool IsUsableContact(const TouchContact& contact,
                                           const float displayWidth,
                                           const float displayHeight) noexcept
        {
            return contact.id >= 0 && std::isfinite(contact.x) && std::isfinite(contact.y)
                && contact.x >= -displayWidth && contact.x <= displayWidth * 2.0f
                && contact.y >= -displayHeight && contact.y <= displayHeight * 2.0f;
        }
    }

    const TouchContact* TouchControls::FindContact(
        const std::span<const TouchContact> contacts, const int id) noexcept
    {
        for (const TouchContact& contact : contacts) {
            if (contact.id == id && contact.id >= 0
                && std::isfinite(contact.x) && std::isfinite(contact.y)) {
                return &contact;
            }
        }
        return nullptr;
    }

    void TouchControls::Reset() noexcept
    {
        state_ = {};
        movementId_ = -1;
        lookId_ = -1;
        lastLookX_ = 0.0f;
        lastLookY_ = 0.0f;
    }

    void TouchControls::AssignNewContacts(const std::span<const TouchContact> contacts,
                                          const float displayWidth,
                                          const float displayHeight) noexcept
    {
        for (const TouchContact& contact : contacts) {
            if (!IsUsableContact(contact, displayWidth, displayHeight)
                || contact.phase == TouchPhase::Released || contact.id == movementId_
                || contact.id == lookId_) {
                continue;
            }
            if (contact.x < displayWidth * 0.5f && movementId_ < 0) {
                movementId_ = contact.id;
                state_.movementAnchorX = contact.x;
                state_.movementAnchorY = contact.y;
                state_.movementTouchX = contact.x;
                state_.movementTouchY = contact.y;
                state_.movementActive = true;
            } else if (contact.x >= displayWidth * 0.5f && lookId_ < 0) {
                lookId_ = contact.id;
                lastLookX_ = contact.x;
                lastLookY_ = contact.y;
                state_.lookTouchX = contact.x;
                state_.lookTouchY = contact.y;
                state_.lookActive = true;
            }
        }
    }

    void TouchControls::UpdateMovement(const TouchContact& contact,
                                       const float displayHeight) noexcept
    {
        state_.movementTouchX = contact.x;
        state_.movementTouchY = contact.y;
        const float radius = std::max(1.0f, displayHeight * MovementRadiusFraction);
        float right = (contact.x - state_.movementAnchorX) / radius;
        float forward = (state_.movementAnchorY - contact.y) / radius;
        const float magnitude = std::hypot(right, forward);
        if (magnitude <= MovementDeadZone) {
            state_.moveRight = 0.0f;
            state_.moveForward = 0.0f;
            return;
        }

        const float normalizedMagnitude = std::min(
            1.0f, (magnitude - MovementDeadZone) / (1.0f - MovementDeadZone));
        right /= magnitude;
        forward /= magnitude;
        state_.moveRight = right * normalizedMagnitude;
        state_.moveForward = forward * normalizedMagnitude;
    }

    TouchControlState TouchControls::Update(const std::span<const TouchContact> contacts,
                                            const float displayWidth,
                                            const float displayHeight) noexcept
    {
        if (!std::isfinite(displayWidth) || !std::isfinite(displayHeight)
            || displayWidth <= 0.0f || displayHeight <= 0.0f
            || displayWidth > MaximumDisplayExtent
            || displayHeight > MaximumDisplayExtent) {
            Reset();
            displayWidth_ = 0.0f;
            displayHeight_ = 0.0f;
            return state_;
        }

        if (displayWidth_ > 0.0f
            && (std::abs(displayWidth - displayWidth_) > 0.5f
                || std::abs(displayHeight - displayHeight_) > 0.5f)) {
            Reset();
        }
        displayWidth_ = displayWidth;
        displayHeight_ = displayHeight;

        state_.lookDeltaX = 0.0f;
        state_.lookDeltaY = 0.0f;

        const TouchContact* movement = FindContact(contacts, movementId_);
        if (movement == nullptr
            || !IsUsableContact(*movement, displayWidth, displayHeight)
            || movement->phase == TouchPhase::Released) {
            movementId_ = -1;
            state_.moveRight = 0.0f;
            state_.moveForward = 0.0f;
            state_.movementActive = false;
        }
        const TouchContact* look = FindContact(contacts, lookId_);
        if (look == nullptr || !IsUsableContact(*look, displayWidth, displayHeight)
            || look->phase == TouchPhase::Released) {
            lookId_ = -1;
            state_.lookActive = false;
        }

        AssignNewContacts(contacts, displayWidth, displayHeight);
        movement = FindContact(contacts, movementId_);
        if (movement != nullptr && movement->phase != TouchPhase::Released) {
            state_.movementActive = true;
            UpdateMovement(*movement, displayHeight);
        }

        look = FindContact(contacts, lookId_);
        if (look != nullptr && look->phase != TouchPhase::Released) {
            state_.lookActive = true;
            state_.lookTouchX = look->x;
            state_.lookTouchY = look->y;
            if (look->phase != TouchPhase::Pressed) {
                state_.lookDeltaX = std::clamp(
                    look->x - lastLookX_, -displayWidth, displayWidth);
                state_.lookDeltaY = std::clamp(
                    look->y - lastLookY_, -displayHeight, displayHeight);
            }
            lastLookX_ = look->x;
            lastLookY_ = look->y;
        }
        return state_;
    }
}
