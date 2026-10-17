#include "IslandSimulator/SpiderContactShadow.hpp"
#include <cmath>
#include <stdexcept>
namespace island {
std::vector<ContactPatchVertex> BuildSpiderContactShadow(const SpiderState& spider,
    const IslandTerrain& terrain,const RenderedTerrainSurface& surface) {
    std::vector<ContactPatchVertex> result;
    const auto append=[&](const ContactFootprint& footprint) {
        const auto patch=BuildContactPatch(terrain,footprint,ContactPatchDetail::SmallAnimal,&surface);
        if(result.size()+patch.size()>SpiderShadowVertexLimit)
            throw std::length_error("Spider contact shadow exceeds vertex budget");
        result.insert(result.end(),patch.begin(),patch.end());
    };
    const auto centre=spider.pose.ToWorld({0,0,-.0156f});
    append({centre.x,centre.z,std::cos(spider.heading),std::sin(spider.heading),.078f,.0377f,.18f});
    for(std::size_t leg=0;leg<8;++leg) {
        const auto p=spider.pose.feet[leg];
        if(!std::isfinite(p.x)||!std::isfinite(p.y)||!std::isfinite(p.z))
            throw std::invalid_argument("Nonfinite spider shadow foot");
        if(!spider.pose.stance[leg])continue;
        append({p.x,p.z,1,0,.00455f,.00455f,.30f});
    }
    return result;
}
}
