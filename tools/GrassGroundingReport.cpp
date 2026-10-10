#include "IslandSimulator/GrassBladeProfile.hpp"
#include "IslandSimulator/CanopyLightField.hpp"
#include "IslandSimulator/GroundCoverField.hpp"
#include "IslandSimulator/JungleLayout.hpp"
#include "IslandSimulator/GrassLayout.hpp"
#include "IslandSimulator/GrassMotionBounds.hpp"
#include "IslandSimulator/RenderedTerrainSurface.hpp"
#include "IslandSimulator/VegetationLayout.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <locale>
#include <stdexcept>

namespace
{
    using Vector = std::array<double,3>;
    Vector Unit(Vector p)
    {
        const double length=std::hypot(p[0],p[1],p[2]);
        for(auto& value:p) value/=length;
        return p;
    }
    struct Basis { Vector x,z; };
    Basis ShaderBasis(island::TerrainNormal normal)
    {
        // Match the renderer's packed X/Z normal reconstruction, not an arbitrary yaw basis.
        const auto n=Unit({normal.x,std::sqrt(std::max(double(island::GrassMotion::NormalFloor),
            1.0-double(normal.x)*normal.x-double(normal.z)*normal.z)),normal.z});
        const auto x=Unit({n[1],-n[0],0});
        const auto z=Unit({x[1]*n[2]-x[2]*n[1],x[2]*n[0]-x[0]*n[2],x[0]*n[1]-x[1]*n[0]});
        return {x,z};
    }
    struct Example { Vector root{}; std::size_t anchor=0; int blade=0,edge=0; };
    struct Statistics
    {
        std::size_t count=0, buried1cm=0, buried3cm=0, floating1cm=0, floating3cm=0;
        double minimum=0,maximum=0,absoluteSum=0,squaredSum=0;
        Example deepest,highest;
        void Add(double error, Example example)
        {
            if(!std::isfinite(error)) throw std::runtime_error("Non-finite grass grounding error");
            if(count==0 || error<minimum) { minimum=error; deepest=example; }
            if(count==0 || error>maximum) { maximum=error; highest=example; }
            ++count;
            buried1cm+=error < -0.01; buried3cm+=error < -0.03;
            floating1cm+=error > 0.01; floating3cm+=error > 0.03;
            absoluteSum+=std::abs(error); squaredSum+=error*error;
        }
        void Write(std::ostream& out) const
        {
            if(!count) throw std::runtime_error("Empty grass grounding report scope");
            out << "{\"samples\":" << count << ",\"minimum_error_m\":" << minimum
                << ",\"maximum_error_m\":" << maximum << ",\"mean_absolute_error_m\":" << absoluteSum/count
                << ",\"rms_error_m\":" << std::sqrt(squaredSum/count)
                << ",\"buried_over_1cm\":" << buried1cm << ",\"buried_over_3cm\":" << buried3cm
                << ",\"floating_over_1cm\":" << floating1cm << ",\"floating_over_3cm\":" << floating3cm;
            const auto example=[&](const char* name,const Example& e) {
                out << ",\"" << name << "\":{\"anchor_index\":" << e.anchor << ",\"blade\":" << e.blade
                    << ",\"edge\":" << e.edge << ",\"root\":[" << e.root[0] << ',' << e.root[1] << ',' << e.root[2] << "]}";
            };
            example("deepest",deepest); example("highest",highest);
            out << '}';
        }
    };
}

int main(int argc,char** argv)
{
    try {
        if(argc!=2) throw std::invalid_argument("Usage: island-grass-grounding-report OUTPUT.json");
        const island::IslandTerrain terrain;
        const island::RenderedTerrainSurface surface(terrain);
        const island::JungleLayout jungle(terrain);
        const island::CanopyLightField light(terrain, island::MakeCanopyOccluders(jungle.Canopy()));
        const island::GroundCoverField cover;
        const island::GrassLayout layout(terrain, light, cover);
        std::array<std::array<Statistics,2>,4> stats{};
        constexpr std::array names{"analytic", "mesh_height_only", "mesh_height_and_normal", "rgb24_surface_reprojection"};
        std::size_t anchor=0;
        for(const auto& patch:layout.Patches()) {
            const auto analytic=terrain.Sample(patch.x,patch.z);
            const auto mesh=surface.Sample(patch.x,patch.z);
            const std::array<Basis,4> bases{ShaderBasis(analytic.normal),ShaderBasis(analytic.normal),
                ShaderBasis(mesh.normal),ShaderBasis(analytic.normal)};
            const std::array<double,4> heights{patch.y,mesh.height,mesh.height,patch.y};
            const bool east=std::hypot(patch.x-island::VegetationLayout::EastClearing.x,
                patch.z-island::VegetationLayout::EastClearing.z)<island::VegetationLayout::EastClearing.radius;
            const double c=std::cos(patch.yawRadians),s=std::sin(patch.yawRadians);
            for(int blade=0;blade<island::NearGrassTurfBladeCount;++blade) {
                const auto turf=island::NearGrassTurfBlade(blade);
                const double facingC=std::cos(turf.placement.facing),facingS=std::sin(turf.placement.facing);
                for(int edge:{-1,1}) {
                    const auto root=island::SampleGrassBlade(turf.shape,0,float(edge));
                    const double x=turf.placement.x-facingS*root.across+facingC*root.along;
                    const double z=turf.placement.z+facingC*root.across+facingS*root.along;
                    const double lx=(x*c-z*s)*patch.spreadScale,lz=(x*s+z*c)*patch.spreadScale;
                    for(std::size_t mode=0;mode<stats.size();++mode) {
                        Vector world{patch.x,heights[mode],patch.z};
                        for(int axis=0;axis<3;++axis) world[axis]+=bases[mode].x[axis]*lx+bases[mode].z[axis]*lz;
                        if(mode==3) world[1]=surface.SamplePacked(static_cast<float>(world[0]),static_cast<float>(world[2]));
                        const double error=world[1]-surface.Sample(static_cast<float>(world[0]),static_cast<float>(world[2])).height;
                        const Example example{world,anchor,blade,edge};
                        stats[mode][0].Add(error,example);
                        if(east) stats[mode][1].Add(error,example);
                    }
                }
            }
            ++anchor;
        }
        std::ofstream out;
        out.exceptions(std::ios::failbit|std::ios::badbit);
        out.open(argv[1]); out.imbue(std::locale::classic()); out << std::setprecision(10);
        out << "{\n\"scope\":\"all low-turf root edges; CPU shader-basis reconstruction; no wind at roots\",\n"
            << "\"anchor_alternatives_are_counterfactuals\":true,\n\"rgb24_is_cpu_shader_emulation_not_gpu_readback\":true,\n\"grass_seed\":" << island::GrassLayout::DefaultSeed
            << ",\n\"anchors\":" << anchor << ",\n\"modes\":{\n";
        for(std::size_t mode=0;mode<stats.size();++mode) {
            if(mode) out << ",\n";
            out << '"' << names[mode] << "\":{\"all\":"; stats[mode][0].Write(out);
            out << ",\"east_clearing\":"; stats[mode][1].Write(out); out << '}';
            std::cout << names[mode] << ": root error " << stats[mode][0].minimum << ".."
                << stats[mode][0].maximum << " m, mean absolute " << stats[mode][0].absoluteSum/stats[mode][0].count << " m\n";
        }
        out << "\n}}\n"; out.close();
        if(std::max(std::abs(stats[3][0].minimum),std::abs(stats[3][0].maximum))>0.0001) {
            throw std::runtime_error("RGB24 root reprojection exceeds 0.1 mm tolerance");
        }
        std::cout << "grass grounding report written (not a conformance pass)\n";
    } catch(const std::exception& error) {
        std::cerr << "grass grounding report failed: " << error.what() << '\n'; return 1;
    }
}
