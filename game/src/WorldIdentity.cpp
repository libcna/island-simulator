#include "IslandSimulator/WorldIdentity.hpp"

#include "IslandSimulator/GrassLayout.hpp"
#include "IslandSimulator/JungleLayout.hpp"
#include "IslandSimulator/NaturalPropLayout.hpp"
#include "IslandSimulator/VegetationLayout.hpp"

#include <iomanip>
#include <locale>
#include <sstream>

namespace island
{
    namespace
    {
        void WriteHexSeed(std::ostream& output, const std::uint32_t seed,
                          const bool uppercase)
        {
            output << (uppercase ? std::uppercase : std::nouppercase)
                   << std::hex << std::setfill('0') << std::setw(8) << seed;
        }
    }

    WorldSeedSet AuthoritativeWorldSeeds() noexcept
    {
        return WorldSeedSet{
            VegetationLayout::DefaultSeed,
            GrassLayout::DefaultSeed,
            JungleLayout::DefaultSeed,
            NaturalPropLayout::DefaultSeed};
    }

    std::string WorldSeedOverlayHex(const std::uint32_t seed)
    {
        std::ostringstream output;
        output.imbue(std::locale::classic());
        WriteHexSeed(output, seed, true);
        return output.str();
    }

    std::string WorldSeedClipboardText(const WorldSeedSet& seeds)
    {
        std::ostringstream output;
        output.imbue(std::locale::classic());
        output << "island-simulator world seeds\n";
        output << "palms=0x";
        WriteHexSeed(output, seeds.palms, false);
        output << "\ngrass=0x";
        WriteHexSeed(output, seeds.grass, false);
        output << "\njungle=0x";
        WriteHexSeed(output, seeds.jungle, false);
        output << "\nnatural-props=0x";
        WriteHexSeed(output, seeds.naturalProps, false);
        output << '\n';
        return output.str();
    }
}
