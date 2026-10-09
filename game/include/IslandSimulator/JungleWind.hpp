#pragma once

#include <iomanip>
#include <limits>
#include <locale>
#include <sstream>
#include <string>

namespace island::JungleWind
{
    inline constexpr float Broad = 0.105f;
    inline constexpr float Detail = 0.028f;
    inline constexpr float DirectionZ = 0.43f;
    inline constexpr float Lowering = 0.035f;
    inline constexpr float HorizontalMargin = 0.16f;
    inline constexpr float VerticalMargin = 0.01f;
    static_assert(Broad >= 0 && Detail >= 0 && Lowering >= 0
        && HorizontalMargin > 0 && VerticalMargin > 0);
    static_assert((Broad + Detail) * (Broad + Detail) * (1 + DirectionZ * DirectionZ)
        < HorizontalMargin * HorizontalMargin);
    static_assert((Broad + Detail) * Lowering < VerticalMargin);

    // Both GL dialects and CPU envelope tests consume the same amplitudes. Wind weights are
    // checked in [0,1] when meshes are built. No locale-dependent shader literals are allowed.
    [[nodiscard]] inline std::string ShaderConstants()
    {
        std::ostringstream source;
        source.imbue(std::locale::classic());
        source << std::scientific << std::setprecision(std::numeric_limits<float>::max_digits10)
               << "const float jungleBroadWind = " << Broad << ";\n"
               << "const float jungleDetailWind = " << Detail << ";\n"
               << "const float jungleWindZ = " << DirectionZ << ";\n"
               << "const float jungleWindLowering = " << Lowering << ";\n";
        return source.str();
    }
}
