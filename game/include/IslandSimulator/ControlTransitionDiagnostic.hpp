#pragma once
#include <array>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <limits>
#include <locale>
#include <sstream>
#include <string>
#include <string_view>

namespace island {
enum class ControlTransitionEvent { FlightOn, FlightOff, LandingRefused, FullscreenOn, FullscreenOff };
inline std::string_view ControlTransitionMessage(ControlTransitionEvent event) noexcept {
    switch(event) {
    case ControlTransitionEvent::FlightOn: return "island-simulator: flight on; Q/E down/up, 100m terrain ceiling, 330m radius";
    case ControlTransitionEvent::FlightOff: return "island-simulator: flight off; safe dry-land return";
    case ControlTransitionEvent::LandingRefused: return "island-simulator: flight remains on; no safe landing found";
    case ControlTransitionEvent::FullscreenOn: return "island-simulator: fullscreen on requested";
    case ControlTransitionEvent::FullscreenOff: return "island-simulator: fullscreen off requested";
    }
    return "island-simulator: unknown control transition";
}
struct ControlTransitionSnapshot {
    std::uint64_t frame=0;
    std::array<float,3> camera{},flightPosition{};
    float yaw=0,pitch=0;
    bool flightActive=false,fullscreenRequested=false;
};
inline std::string FormatControlTransition(ControlTransitionEvent event,const ControlTransitionSnapshot& s) {
    bool finite=std::isfinite(s.yaw)&&std::isfinite(s.pitch);
    for(const float v:s.camera) finite=finite&&std::isfinite(v);
    for(const float v:s.flightPosition) finite=finite&&std::isfinite(v);
    std::ostringstream out; out.imbue(std::locale::classic());
    out << std::setprecision(std::numeric_limits<float>::max_digits10)
        << "island-simulator: control-transition event=" << static_cast<int>(event)
        << " submitted-frames=" << s.frame << " camera=" << s.camera[0] << ',' << s.camera[1] << ',' << s.camera[2]
        << " yaw=" << s.yaw << " pitch=" << s.pitch
        << " flight-active=" << s.flightActive << " flight-position="
        << s.flightPosition[0] << ',' << s.flightPosition[1] << ',' << s.flightPosition[2]
        << " fullscreen-requested=" << s.fullscreenRequested << " finite=" << finite;
    return out.str();
}
}
