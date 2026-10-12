#include "IslandSimulator/GoatMesh.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <iostream>
#include <locale>
#include <stdexcept>
#include <vector>

// CPU-only orthographic shape review, not the game's lighting/material/rendering path.
int main(int argc,char** argv) {
    using namespace island;
    if(argc!=2){std::cerr<<"usage: island-goat-mesh-preview output.svg\n";return 1;}
    std::ofstream out(argv[1]);out.imbue(std::locale::classic());
    if(!out)throw std::runtime_error("Cannot open goat preview output");
    out<<"<svg xmlns='http://www.w3.org/2000/svg' width='1000' height='640' viewBox='0 0 1000 640'>"
       <<"<rect width='1000' height='640' fill='#e7e5de'/>"
       <<"<text x='24' y='30' font-family='sans-serif' font-size='18'>Goat geometry only - CPU orthographic preview, no coat texture</text>";
    constexpr std::array<std::array<int,3>,7> colors{{{184,165,134},{145,116,92},{108,91,66},
        {58,54,45},{18,16,13},{63,57,48},{166,122,53}}};
    for(int panel=0;panel<2;++panel) {
        GoatGroundPose pose{};pose.scale=1;
        for(std::size_t leg=0;leg<4;++leg)pose.legs[leg]=EvaluateGoatLeg(0,0,0,leg);
        const auto mesh=BuildGoatMesh(pose,static_cast<float>(panel),true);
        struct Face {std::array<GoatVertex,3> vertices;float depth;};
        std::vector<Face> faces;
        for(std::size_t i=0;i<mesh.vertices.size();i+=3) {
            Face face{{mesh.vertices[i],mesh.vertices[i+1],mesh.vertices[i+2]},0};
            float facing=0;
            for(const auto& v:face.vertices) {
                face.depth+=.8f*v.position.x+.25f*v.position.y+.6f*v.position.z;
                facing+=.8f*v.normal.x+.25f*v.normal.y+.6f*v.normal.z;
            }
            if(facing>0)faces.push_back(face);
        }
        std::stable_sort(faces.begin(),faces.end(),[](const auto& a,const auto& b){return a.depth<b.depth;});
        for(const auto& face:faces) {
            GoatPoint n{};
            for(const auto& v:face.vertices){n.x+=v.normal.x;n.y+=v.normal.y;n.z+=v.normal.z;}
            const float length=std::hypot(n.x,n.y,n.z);
            const float light=.42f+.58f*std::max(0.0f,(.5f*n.x+.8f*n.y+.33f*n.z)/length);
            const auto color=colors[static_cast<std::size_t>(face.vertices[0].material)];
            out<<"<polygon fill='rgb("<<static_cast<int>(color[0]*light)<<','<<static_cast<int>(color[1]*light)
               <<','<<static_cast<int>(color[2]*light)<<")' points='";
            for(const auto& v:face.vertices) {
                const auto p=v.position;
                out<<260+panel*500+285*(.6f*p.x-.8f*p.z)<<','
                   <<550-285*(-.194029f*p.x+.970143f*p.y-.145521f*p.z)<<' ';
            }
            out<<"'/>";
        }
        out<<"<text x='"<<100+panel*500<<"' y='595' font-family='sans-serif' font-size='18'>"
           <<(panel==0?"Head up":"Grazing pose")<<"</text>";
    }
    out<<"</svg>\n";
    if(!out)throw std::runtime_error("Cannot write goat preview output");
}
