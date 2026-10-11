#include "IslandSimulator/DiagnosticStreamMirror.hpp"
#include "IslandSimulator/ControlTransitionDiagnostic.hpp"
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace
{
    std::vector<std::string> messages;
    void Capture(std::string_view message) noexcept { messages.emplace_back(message); }
}

int main()
{
    int failures = 0;
    const auto check = [&](bool passed) { if (!passed) ++failures; };
    const island::ControlTransitionSnapshot snapshot{2290,{-15.9166985f,49.44701f,-118.114937f},
        {-15.9166985f,49.44701f,-118.114937f},-.417899817f,-.296599835f,true,true};
    const auto record=island::FormatControlTransition(island::ControlTransitionEvent::FlightOn,snapshot);
    check(record.find("submitted-frames=2290")!=std::string::npos);
    check(record.find("camera=-15.9166985,49.44701,-118.114937")!=std::string::npos);
    check(record.find("flight-active=1")!=std::string::npos);
    check(record.ends_with("fullscreen-requested=1 finite=1"));
    for(const auto event:{island::ControlTransitionEvent::FlightOn,island::ControlTransitionEvent::FlightOff,
        island::ControlTransitionEvent::LandingRefused,island::ControlTransitionEvent::FullscreenOn,
        island::ControlTransitionEvent::FullscreenOff}) {
        check(island::ControlTransitionMessage(event).starts_with("island-simulator: "));
        check(island::FormatControlTransition(event,snapshot).find('\n')==std::string::npos);
    }
    auto invalid=snapshot; invalid.camera[1]=std::numeric_limits<float>::quiet_NaN();
    check(island::FormatControlTransition(island::ControlTransitionEvent::FlightOn,invalid).ends_with("finite=0"));
    invalid=snapshot; invalid.flightPosition[0]=std::numeric_limits<float>::infinity();
    check(island::FormatControlTransition(island::ControlTransitionEvent::FlightOff,invalid).ends_with("finite=0"));
    std::ostringstream stream;
    std::ostream& output = stream;
    auto* original = output.rdbuf();
    {
        island::DiagnosticStreamMirror mirror(stream, Capture);
        check(output.rdbuf() != original);
        stream << std::unitbuf << "shader " << "link failed" << '\n';
        check(messages.size() == 1 && messages[0] == "shader link failed");
        stream << "unterminated";
        check(messages.size() == 1);
    }
    check(output.rdbuf() == original);
    check(messages.size() == 2 && messages[1] == "unterminated");
    check(stream.str() == "shader link failed\nunterminated");
    stream << " restored";
    check(messages.size() == 2);
    messages.clear();
    std::ostringstream large;
    const std::string payload(island::DiagnosticStreamMirror::ChunkSize
                              * (island::DiagnosticStreamMirror::MaximumChunks + 2), 'x');
    {
        island::DiagnosticStreamMirror mirror(large, Capture);
        large << payload << '\n' << "still forwarded\n";
    }
    check(large.str() == payload + "\nstill forwarded\n");
    check(messages.size() == island::DiagnosticStreamMirror::MaximumChunks + 1);
    check(messages.front().size() == island::DiagnosticStreamMirror::ChunkSize);
    check(messages.back().find("limit reached") != std::string::npos);
    if (failures) std::cerr << failures << " diagnostic mirror checks failed\n";
    return failures ? 1 : 0;
}
