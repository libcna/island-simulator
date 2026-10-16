#pragma once

#include <algorithm>
#include <cmath>

namespace island {

/** Authored inland footprint for the concealed underground research bunker. */
struct BunkerSite final {
    static constexpr float X=92.0f;
    static constexpr float Z=24.0f;
    // The entrance faces north; the descending spine runs south into the second ridge.
    static constexpr float Angle=1.57079632679f;
    static constexpr float MaximumAcross=12.0f;
    static constexpr float MinimumDepth=-7.0f;
    static constexpr float MaximumDepth=39.0f;

    [[nodiscard]] static bool Reserves(float x,float z,float margin=0.0f) noexcept {
        if(!std::isfinite(x)||!std::isfinite(z)||!std::isfinite(margin))return false;
        margin=std::max(0.0f,margin);
        const float dx=x-X,dz=z-Z;
        const float across=-std::sin(Angle)*dx+std::cos(Angle)*dz;
        const float depth=-std::cos(Angle)*dx-std::sin(Angle)*dz;
        return std::abs(across)<=MaximumAcross+margin
            && depth>=MinimumDepth-margin&&depth<=MaximumDepth+margin;
    }

    /** Smooth, finite listener presence for the built underground rooms, not the ridge above. */
    [[nodiscard]] static float SoundPresence(float x,float y,float z,
                                             float entranceGroundY) noexcept {
        if(!std::isfinite(x)||!std::isfinite(y)||!std::isfinite(z)
            ||!std::isfinite(entranceGroundY))return 0.0f;
        const float dx=x-X,dz=z-Z;
        const float across=-std::sin(Angle)*dx+std::cos(Angle)*dz;
        const float depth=-std::cos(Angle)*dx-std::sin(Angle)*dz;
        const bool corridor=std::abs(across)<=1.55f&&depth>=-.75f&&depth<=36.3f;
        const bool computer=across>=.9f&&across<=9.35f&&depth>=10.8f&&depth<=23.4f;
        const bool reactor=across>=-10.25f&&across<=-.9f&&depth>=22.6f&&depth<=36.35f;
        const bool service=across>=.9f&&across<=7.25f&&depth>=25.6f&&depth<=35.65f;
        if(!(corridor||computer||reactor||service))return 0.0f;
        const float floorRelative=depth<=0.0f?.11f*depth:depth<10.0f?-.32f*depth:-3.2f;
        // Match the walking camera rather than accepting a listener standing on the ridge roof.
        const float verticalError=std::abs(y-(entranceGroundY+floorRelative+1.72f));
        const float verticalT=std::clamp((verticalError-.7f)/1.35f,0.0f,1.0f);
        const float verticalGate=1.0f-verticalT*verticalT*(3.0f-2.0f*verticalT);
        const float depthT=std::clamp((depth-.25f)/4.5f,0.0f,1.0f);
        return depthT*depthT*(3.0f-2.0f*depthT)*verticalGate;
    }
};

}
