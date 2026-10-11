#include "IslandSimulator/DiagnosticStreamMirror.hpp"
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
