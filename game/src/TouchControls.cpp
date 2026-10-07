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

    TouchSensitivitySelectorLayout TouchControls::SensitivitySelectorLayout(
        const float displayWidth, const float displayHeight) noexcept
    {
        if (!std::isfinite(displayWidth) || !std::isfinite(displayHeight)
            || displayWidth <= 0.0f || displayHeight <= 0.0f) {
            return {};
        }
        const float radius = std::max(1.0f,
                                      displayHeight * SensitivitySelectorRadiusFraction);
        return {displayWidth - radius * 1.4f, radius * 1.4f, radius};
    }

    void TouchControls::Reset() noexcept
    {
        const TouchLookSensitivityLevel sensitivity = state_.lookSensitivity;
        state_ = {};
        state_.lookSensitivity = sensitivity;
        movementId_ = -1;
        lookId_ = -1;
        sensitivitySelectorId_ = -1;
        lastLookX_ = 0.0f;
        lastLookY_ = 0.0f;
    }

    void TouchControls::SetLookSensitivity(
        const TouchLookSensitivityLevel sensitivity) noexcept
    {
        Reset();
        switch (sensitivity) {
        case TouchLookSensitivityLevel::Low:
        case TouchLookSensitivityLevel::Standard:
        case TouchLookSensitivityLevel::High:
            state_.lookSensitivity = sensitivity;
            break;
        }
    }

    void TouchControls::AssignNewContacts(const std::span<const TouchContact> contacts,
                                          const float displayWidth,
                                          const float displayHeight) noexcept
    {
        for (const TouchContact& contact : contacts) {
            if (!IsUsableContact(contact, displayWidth, displayHeight)
                || contact.phase == TouchPhase::Released || contact.id == movementId_
                || contact.id == lookId_ || contact.id == sensitivitySelectorId_) {
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

    void TouchControls::UpdateSensitivitySelector(
        const std::span<const TouchContact> contacts, const float displayWidth,
        const float displayHeight) noexcept
    {
        state_.sensitivitySelectorActive = false;
        const TouchContact* selectorContact = FindContact(contacts, sensitivitySelectorId_);
        if (selectorContact == nullptr
            || !IsUsableContact(*selectorContact, displayWidth, displayHeight)
            || selectorContact->phase == TouchPhase::Released) {
            sensitivitySelectorId_ = -1;
        }
        if (sensitivitySelectorId_ >= 0) {
            state_.sensitivitySelectorActive = true;
            return;
        }

        const TouchSensitivitySelectorLayout selector = SensitivitySelectorLayout(
            displayWidth, displayHeight);
        for (const TouchContact& contact : contacts) {
            if (contact.phase != TouchPhase::Pressed
                || contact.id == movementId_ || contact.id == lookId_
                || !IsUsableContact(contact, displayWidth, displayHeight)
                || std::hypot(contact.x - selector.x, contact.y - selector.y)
                    > selector.radius) {
                continue;
            }
            sensitivitySelectorId_ = contact.id;
            switch (state_.lookSensitivity) {
            case TouchLookSensitivityLevel::Low:
                state_.lookSensitivity = TouchLookSensitivityLevel::Standard;
                break;
            case TouchLookSensitivityLevel::Standard:
                state_.lookSensitivity = TouchLookSensitivityLevel::High;
                break;
            case TouchLookSensitivityLevel::High:
                state_.lookSensitivity = TouchLookSensitivityLevel::Low;
                break;
            }
            state_.sensitivitySelectorActive = true;
            state_.lookSensitivityChanged = true;
            return;
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
            state_.runRequested = false;
            return;
        }

        const float normalizedMagnitude = std::min(
            1.0f, (magnitude - MovementDeadZone) / (1.0f - MovementDeadZone));
        right /= magnitude;
        forward /= magnitude;
        state_.moveRight = right * normalizedMagnitude;
        state_.moveForward = forward * normalizedMagnitude;
        if (state_.runRequested) {
            state_.runRequested = normalizedMagnitude > RunReleaseMagnitude;
        } else {
            state_.runRequested = normalizedMagnitude >= RunEngageMagnitude;
        }
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
        state_.lookSensitivityChanged = false;

        const TouchContact* movement = FindContact(contacts, movementId_);
        if (movement == nullptr
            || !IsUsableContact(*movement, displayWidth, displayHeight)
            || movement->phase == TouchPhase::Released) {
            movementId_ = -1;
            state_.moveRight = 0.0f;
            state_.moveForward = 0.0f;
            state_.movementActive = false;
            state_.runRequested = false;
        }
        const TouchContact* look = FindContact(contacts, lookId_);
        if (look == nullptr || !IsUsableContact(*look, displayWidth, displayHeight)
            || look->phase == TouchPhase::Released) {
            lookId_ = -1;
            state_.lookActive = false;
        }

        UpdateSensitivitySelector(contacts, displayWidth, displayHeight);
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
