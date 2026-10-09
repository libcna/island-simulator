#include "IslandSimulator/OpaqueImageLoader.hpp"
#include "Microsoft/Xna/Framework/TitleContainer.hpp"
#include "Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp"
#include "Microsoft/Xna/Framework/Graphics/SurfaceFormat.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"
#include <array>
#include <stdexcept>
#include <type_traits>

namespace island
{
    std::vector<forest_floor::MipLevel> LoadOpaqueImageMips(
        Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
        const char* path, const int targetSize)
    {
        using namespace Microsoft::Xna::Framework;
        using namespace Microsoft::Xna::Framework::Graphics;
        static_assert(sizeof(forest_floor::Pixel)==4);
        static_assert(std::is_same_v<std::uint8_t,unsigned char>);
        auto stream=TitleContainer::OpenStream(path);
        if (!stream || stream->getLengthProperty()<33 || stream->getLengthProperty()>8*1024*1024)
            throw std::runtime_error("PNG stream is absent or outside the 33-byte..8-MiB limit");
        std::array<std::uint8_t,24> header{};
        int read=0;
        while (read<static_cast<int>(header.size())) {
            const int count=stream->Read(header.data(),read,static_cast<int>(header.size())-read);
            if (count<=0) throw std::runtime_error("PNG header is truncated");
            read+=count;
        }
        const int size=forest_floor::ValidateSourceHeader(header);
        stream->setPositionProperty(0);
        auto decoded=Texture2D::FromStream(device,*stream);
        if (decoded.getWidthProperty()!=size || decoded.getHeightProperty()!=size
            || decoded.getFormatProperty()!=SurfaceFormat::Color)
            throw std::runtime_error("PNG decoder dimensions/format differ from bounded RGBA8 input");
        std::vector<forest_floor::Pixel> pixels(static_cast<std::size_t>(size)*size);
        // CNA's public transfer uses its CPU shadow; no raw GL readback or Color-layout cast.
        decoded.GetData(reinterpret_cast<std::uint8_t*>(pixels.data()),
                        static_cast<int>(pixels.size()*sizeof(forest_floor::Pixel)));
        return forest_floor::PrepareTexture(pixels,size,targetSize);
        // Stream, decoder texture and source pixels die before the final material allocation.
    }
}
