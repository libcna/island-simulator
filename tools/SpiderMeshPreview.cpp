#include "IslandSimulator/SpiderMesh.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <iostream>
#include <locale>
#include <stdexcept>

// Offline painter-sorted shape review only, not the game renderer or a depth test.
int main(int argc,char** argv) {
    using namespace island;
    if(argc!=2){std::cerr<<"usage: island-spider-mesh-preview output.svg\n";return 1;}
    std::ofstream out(argv[1]);out.imbue(std::locale::classic());
    if(!out)throw std::runtime_error("Cannot open spider preview");
    out<<"<svg xmlns='http://www.w3.org/2000/svg' width='1000' height='560'>"
       <<"<rect width='1000' height='560' fill='#ddd9cb'/>"
       <<"<text x='24' y='30' font-family='sans-serif' font-size='18'>Spider geometry - CPU shape preview, NOT in-game lighting/materials</text>";
    constexpr std::array<std::array<int,3>,4> colors{{{100,77,51},{169,139,93},{15,13,11},{87,71,51}}};
    const auto mesh=BuildSpiderMesh(SpiderPose::Rest(),true);
    for(int panel=0;panel<2;++panel) {
        const SpiderPoint view=panel==0?SpiderPoint{0,.8660254f,.5f}:SpiderPoint{.8660254f,.5f,0};
        const SpiderPoint right=panel==0?SpiderPoint{1,0,0}:SpiderPoint{0,0,-1};
        const SpiderPoint up=panel==0?SpiderPoint{0,.5f,-.8660254f}:SpiderPoint{-.5f,.8660254f,0};
        const auto dot=[](SpiderPoint a,SpiderPoint b){return a.x*b.x+a.y*b.y+a.z*b.z;};
        struct Face{std::array<SpiderVertex,3> vertices;float depth;};
        std::vector<Face> faces;
        for(std::size_t i=0;i<mesh.vertices.size();i+=3) {
            Face face{{mesh.vertices[i],mesh.vertices[i+1],mesh.vertices[i+2]},0};
            float facing=0;
            for(const auto& v:face.vertices){face.depth+=dot(v.position,view);facing+=dot(v.normal,view);}
            if(facing>0)faces.push_back(face);
        }
        std::stable_sort(faces.begin(),faces.end(),[](const auto& a,const auto& b){return a.depth<b.depth;});
        for(const auto& face:faces) {
            SpiderPoint normal{};
            for(const auto& v:face.vertices){normal.x+=v.normal.x;normal.y+=v.normal.y;normal.z+=v.normal.z;}
            const float light=.4f+.6f*std::max(0.0f,dot(normal,{.3f,.9f,.3f})/std::hypot(normal.x,normal.y,normal.z));
            const auto color=colors[static_cast<std::size_t>(face.vertices[0].material)];
            out<<"<polygon fill='rgb("<<static_cast<int>(color[0]*light)<<','<<static_cast<int>(color[1]*light)
               <<','<<static_cast<int>(color[2]*light)<<")' points='";
            for(const auto& v:face.vertices)out<<250+panel*500+1750*dot(v.position,right)<<','<<300-1750*dot(v.position,up)<<' ';
            out<<"'/>";
        }
        out<<"<text x='"<<160+panel*500<<"' y='510' font-family='sans-serif' font-size='18'>"
           <<(panel==0?"Elevated front":"Elevated side")<<"</text>";
    }
    out<<"</svg>\n";
    if(!out)throw std::runtime_error("Cannot write spider preview");
}
