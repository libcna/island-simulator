#include "IslandSimulator/GrassMotionBounds.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <iomanip>
#include <limits>
#include <locale>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace island::GrassMotion
{
    static_assert(MinimumStature > 0 && MaximumStature >= MinimumStature && HeightExponent > 0);
    static_assert(MinimumPush >= 0 && MaximumPush >= MinimumPush
                  && MinimumLowering >= 0 && MaximumLowering >= MinimumLowering);
    static_assert(BroadWind >= 0 && DetailWind >= 0 && MinimumWindGain >= 0
                  && MaximumWindGain >= MinimumWindGain && WindLowering >= 0 && NormalFloor > 0);

    Bounds CalculateBounds(const float meshRadius, const float meshHeight, const float heightScale,
                           const float spreadScale, const float normalX, const float normalZ)
    {
        for (const float value : {meshRadius,meshHeight,heightScale,spreadScale,normalX,normalZ}) {
            if (!std::isfinite(value)) throw std::invalid_argument("Non-finite grass bound input");
        }
        if (meshRadius < 0 || meshHeight < 0 || heightScale <= 0 || spreadScale <= 0
            || std::abs(normalX) > 1 || std::abs(normalZ) > 1) {
            throw std::invalid_argument("Grass bound input outside supported range");
        }
        // Match the vertex shader's normal reconstruction, including its near-horizontal floor.
        const double horizontalNormalSquared = double(normalX)*normalX + double(normalZ)*normalZ;
        const double normalY = std::sqrt(std::max(double(NormalFloor),1.0-horizontalNormalSquared));
        const double normalLength = std::sqrt(horizontalNormalSquared+normalY*normalY);
        const double slopeSin = std::sqrt(horizontalNormalSquared)/normalLength;
        const double slopeCos = normalY/normalLength;
        const double radius = double(meshRadius)*spreadScale;
        const double height = double(meshHeight)*heightScale*MaximumStature;
        const double sway = (double(BroadWind)+DetailWind)*heightScale*MaximumWindGain;
        const double horizontalWind = sway*std::hypot(1.0,double(WindZ));
        // The terrain tangent disc projects by at most radius horizontally and radius*sin(slope)
        // vertically. Player direction and heightWeight each have magnitude at most one.
        const Bounds bounds{
            static_cast<float>(radius+height*slopeSin+heightScale*MaximumPush+horizontalWind+Padding),
            static_cast<float>(-radius*slopeSin-heightScale*MaximumLowering-sway*WindLowering-Padding),
            static_cast<float>(radius*slopeSin+std::max(height*slopeCos,double(meshHeight)*heightScale)+Padding)};
        if (!std::isfinite(bounds.horizontalRadius) || !std::isfinite(bounds.minimumY)
            || !std::isfinite(bounds.maximumY)) {
            throw std::invalid_argument("Grass bounds overflow supported geometry");
        }
        return bounds;
    }

    std::string ShaderConstants()
    {
        const std::array values{
            std::pair{"grassMinimumStature",MinimumStature},std::pair{"grassMaximumStature",MaximumStature},
            std::pair{"grassHeightExponent",HeightExponent},std::pair{"grassMinimumPush",MinimumPush},
            std::pair{"grassMaximumPush",MaximumPush},std::pair{"grassMinimumLowering",MinimumLowering},
            std::pair{"grassMaximumLowering",MaximumLowering},std::pair{"grassBroadWind",BroadWind},
            std::pair{"grassDetailWind",DetailWind},std::pair{"grassMinimumWindGain",MinimumWindGain},
            std::pair{"grassMaximumWindGain",MaximumWindGain},std::pair{"grassWindZ",WindZ},
            std::pair{"grassWindLowering",WindLowering},std::pair{"grassNormalFloor",NormalFloor}};
        std::ostringstream source;
        source.imbue(std::locale::classic());
        source << std::scientific << std::setprecision(std::numeric_limits<float>::max_digits10);
        for (const auto& [name,value] : values) source << "const float " << name << " = " << value << ";\n";
        return source.str();
    }
}
