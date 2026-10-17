#include "IslandSimulator/BunkerRenderer.hpp"

#include "IslandSimulator/EnvironmentLighting.hpp"
#include "IslandSimulator/RuntimeDiagnostics.hpp"
#include "CNA/Graphics/ShaderCodeEXT.hpp"
#include "CNA/Graphics/ShaderPackageEXT.hpp"
#include "CNA/GraphicsCapability.hpp"
#include "CNA/ShaderLanguageEXT.hpp"
#include "Microsoft/Xna/Framework/BoundingFrustum.hpp"
#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/ContainmentType.hpp"
#include "Microsoft/Xna/Framework/Graphics/BasicEffect.hpp"
#include "Microsoft/Xna/Framework/Graphics/BlendState.hpp"
#include "Microsoft/Xna/Framework/Graphics/BufferUsage.hpp"
#include "Microsoft/Xna/Framework/Graphics/DepthStencilState.hpp"
#include "Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp"
#include "Microsoft/Xna/Framework/Graphics/PrimitiveType.hpp"
#include "Microsoft/Xna/Framework/Graphics/RasterizerState.hpp"
#include "Microsoft/Xna/Framework/Graphics/ShaderEffect.hpp"
#include "Microsoft/Xna/Framework/Graphics/VertexBuffer.hpp"
#include "Microsoft/Xna/Framework/Graphics/VertexPositionColor.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <limits>
#include <numbers>
#include <stdexcept>
#include <string>
#include <vector>

using namespace Microsoft::Xna::Framework;
using namespace Microsoft::Xna::Framework::Graphics;

namespace island {
namespace {
constexpr const char* BunkerVertex=R"(
layout(location=0) in vec3 aPosition;
layout(location=1) in vec4 aColor;
uniform mat4 uViewProjection;
out vec3 world;out vec4 pigment;
void main(){
    world=aPosition;pigment=aColor;
    gl_Position=uViewProjection*vec4(aPosition,1.0);
    gl_Position.z=2.0*gl_Position.z-gl_Position.w;
}
)";
constexpr const char* BunkerFragment=R"(
in vec3 world;in vec4 pigment;
out vec4 fragColor;
float hash(vec3 p){return fract(sin(dot(p,vec3(127.1,311.7,74.7)))*43758.5453);}
float noise(vec3 p){
    vec3 i=floor(p),f=fract(p);f=f*f*(3.0-2.0*f);
    return mix(mix(mix(hash(i),hash(i+vec3(1,0,0)),f.x),
                   mix(hash(i+vec3(0,1,0)),hash(i+vec3(1,1,0)),f.x),f.y),
               mix(mix(hash(i+vec3(0,0,1)),hash(i+vec3(1,0,1)),f.x),
                   mix(hash(i+vec3(0,1,1)),hash(i+vec3(1,1,1)),f.x),f.y),f.z);
}
void main(){
    float broad=noise(world*vec3(.42,.55,.37)+vec3(7.1,2.4,5.8));
    float grain=noise(world*4.6+vec3(1.7,9.3,3.2));
    float age=noise(world*vec3(.16,.72,.16)+vec3(13.0,4.0,8.0));
    vec3 color=pigment.rgb*(.79+broad*.27+grain*.075);
    float damp=smoothstep(.60,.86,age);
    color=mix(color,color*vec3(.67,.90,.87),damp*.24);
    float oxidation=smoothstep(.70,.90,
        noise(world*vec3(.31,.18,.35)+vec3(21.0,3.0,11.0)));
    color=mix(color,color*vec3(1.19,.76,.43),oxidation*.20);
    float jointWear=smoothstep(.80,.98,noise(world*1.25+vec3(2.0,17.0,6.0)));
    color+=vec3(.035,.027,.015)*jointWear;
    fragColor=vec4(color,1.0);
}
)";

std::string BunkerShaderSource(bool embedded,const char* body) {
    return std::string(embedded?"#version 300 es\nprecision highp float;\n":"#version 330 core\n")+body;
}
}

BunkerRenderer::BunkerRenderer(GraphicsDevice& device,const BunkerLayout& layout)
    :device_(device),effect_(std::make_unique<BasicEffect>(device)) {
    std::vector<VertexPositionColor> opaque,glow;
    opaque.reserve(12000);glow.reserve(1800);
    const auto world=[&](const Vector3& p) {
        const auto q=layout.World(p.X,p.Y,p.Z);return Vector3(q.x,q.y,q.z);
    };
    const auto triangle=[&](Vector3 a,Vector3 b,Vector3 c,const Color& color,bool lit=false) {
        auto& target=lit?glow:opaque;
        target.emplace_back(world(a),color);target.emplace_back(world(b),color);
        target.emplace_back(world(c),color);
    };
    const auto quad=[&](const Vector3& a,const Vector3& b,const Vector3& c,const Vector3& d,
                        const Color& color,bool lit=false) {
        triangle(a,b,c,color,lit);triangle(a,c,d,color,lit);
    };
    const auto shade=[](const Color& color,float scale) {
        return Color(static_cast<int>(std::clamp(color.getRProperty()*scale,0.0f,255.0f)),
                     static_cast<int>(std::clamp(color.getGProperty()*scale,0.0f,255.0f)),
                     static_cast<int>(std::clamp(color.getBProperty()*scale,0.0f,255.0f)),
                     static_cast<int>(color.getAProperty()));
    };
    const auto box=[&](const Vector3& low,const Vector3& high,const Color& color,bool lit=false) {
        const std::array<Vector3,8> p{{{low.X,low.Y,low.Z},{high.X,low.Y,low.Z},
            {high.X,high.Y,low.Z},{low.X,high.Y,low.Z},{low.X,low.Y,high.Z},
            {high.X,low.Y,high.Z},{high.X,high.Y,high.Z},{low.X,high.Y,high.Z}}};
        quad(p[0],p[3],p[2],p[1],shade(color,.78f),lit);
        quad(p[4],p[5],p[6],p[7],shade(color,.92f),lit);
        quad(p[0],p[1],p[5],p[4],shade(color,.67f),lit);
        quad(p[1],p[2],p[6],p[5],shade(color,.86f),lit);
        quad(p[2],p[3],p[7],p[6],shade(color,1.16f),lit);
        quad(p[3],p[0],p[4],p[7],shade(color,.82f),lit);
    };
    constexpr float tau=2.0f*std::numbers::pi_v<float>;
    const auto verticalCylinder=[&](const Vector3& centre,float radius,float height,int segments,
                                    const Color& side,const Color& top,bool lit=false) {
        for(int i=0;i<segments;++i) {
            const float a=tau*i/segments,b=tau*(i+1)/segments;
            const Vector3 p0=centre+Vector3(std::cos(a)*radius,0,std::sin(a)*radius);
            const Vector3 p1=centre+Vector3(std::cos(b)*radius,0,std::sin(b)*radius);
            const float facet=.76f+.25f*std::max(0.0f,std::cos((a+b)*.5f-.72f));
            quad(p0,p1,p1+Vector3(0,height,0),p0+Vector3(0,height,0),shade(side,facet),lit);
            triangle(centre+Vector3(0,height,0),p0+Vector3(0,height,0),
                     p1+Vector3(0,height,0),shade(top,1.12f),lit);
        }
    };
    const auto tube=[&](const Vector3& a,const Vector3& b,float radius,int segments,
                        const Color& color,bool capEnds=true,bool lit=false) {
        Vector3 axis=b-a;
        if(axis.LengthSquared()<1.0e-8f)return;
        axis.Normalize();
        Vector3 side=Vector3::Cross(axis,std::abs(axis.Y)<.85f?Vector3::Up:Vector3::Right);
        side.Normalize();
        const Vector3 up=Vector3::Normalize(Vector3::Cross(side,axis));
        for(int i=0;i<segments;++i) {
            const float angle0=tau*i/segments,angle1=tau*(i+1)/segments;
            const Vector3 radial0=side*std::cos(angle0)+up*std::sin(angle0);
            const Vector3 radial1=side*std::cos(angle1)+up*std::sin(angle1);
            const float facet=.77f+.25f*std::max(0.0f,
                Vector3::Dot((radial0+radial1)*.5f,Vector3(.62f,.71f,-.32f)));
            quad(a+radial0*radius,a+radial1*radius,b+radial1*radius,b+radial0*radius,
                 shade(color,facet),lit);
            if(capEnds) {
                triangle(a,a+radial1*radius,a+radial0*radius,shade(color,.72f),lit);
                triangle(b,b+radial0*radius,b+radial1*radius,shade(color,1.04f),lit);
            }
        }
    };
    const auto ellipsoid=[&](const Vector3& centre,const Vector3& radius,int rings,int slices,
                             const Color& color) {
        for(int ring=0;ring<rings;++ring) {
            const float latitude0=-std::numbers::pi_v<float>*.5f
                +std::numbers::pi_v<float>*ring/rings;
            const float latitude1=-std::numbers::pi_v<float>*.5f
                +std::numbers::pi_v<float>*(ring+1)/rings;
            for(int slice=0;slice<slices;++slice) {
                const float longitude0=tau*slice/slices,longitude1=tau*(slice+1)/slices;
                const auto point=[&](float latitude,float longitude) {
                    return centre+Vector3(std::cos(latitude)*std::cos(longitude)*radius.X,
                        std::sin(latitude)*radius.Y,
                        std::cos(latitude)*std::sin(longitude)*radius.Z);
                };
                const auto a=point(latitude0,longitude0),b=point(latitude1,longitude0),
                           c=point(latitude1,longitude1),d=point(latitude0,longitude1);
                const float latitude=(latitude0+latitude1)*.5f;
                const float longitude=(longitude0+longitude1)*.5f;
                const float light=.66f+.20f*(std::sin(latitude)*.5f+.5f)
                    +.22f*std::max(0.0f,std::cos(longitude-.72f));
                const auto facet=shade(color,std::clamp(light,.62f,1.12f));
                if(ring==0)triangle(a,b,c,facet);
                else if(ring==rings-1)triangle(a,b,d,facet);
                else quad(a,b,c,d,facet);
            }
        }
    };
    const Color concrete(62,67,65),concreteLight(82,88,84),concreteDark(39,43,42);
    const Color steel(76,83,84),steelDark(35,40,42),floor(48,52,51);
    const Color warm(224,205,151,255),cyan(73,205,221,215),screen(70,191,175,235);
    constexpr float roomFloor=-3.2f,roomCeiling=-.15f;
    const auto corridorCeiling=[&](float depth) {
        const float t=std::clamp((depth-8.0f)/2.0f,0.0f,1.0f);
        const float eased=t*t*(3.0f-2.0f*t);
        return layout.FloorRelative(depth)+2.75f+.30f*eased;
    };

    // Concealed rock-and-concrete portal, open blast door and drainage threshold.
    box({-2.25f,-.16f,-.75f},{-1.36f,3.08f,.65f},concreteDark);
    box({1.36f,-.16f,-.75f},{2.25f,3.08f,.65f},concreteDark);
    box({-2.25f,2.72f,-.75f},{2.25f,3.42f,.65f},concreteDark);
    box({-1.55f,-.06f,-.82f},{1.55f,.10f,.35f},steelDark);
    // A recessed steel portal survives between the camouflage stones: layered
    // jambs, lintel and bolt plates keep the opening readable in canopy shade.
    box({-1.51f,.06f,-.70f},{-1.35f,2.78f,.34f},steel);
    box({1.35f,.06f,-.70f},{1.51f,2.78f,.34f},steel);
    box({-1.51f,2.62f,-.70f},{1.51f,2.79f,.34f},steel);
    for(float y:{.40f,1.02f,1.64f,2.26f}) {
        box({-1.55f,y,-.74f},{-1.47f,y+.08f,-.64f},Color(129,134,126));
        box({1.47f,y,-.74f},{1.55f,y+.08f,-.64f},Color(129,134,126));
    }
    // Heavy door rests open against the right rock shoulder; its wheel and ribs are visible.
    box({2.28f,.08f,-.62f},{2.48f,2.76f,1.05f},steel);
    for(float y:{.42f,1.04f,1.66f,2.28f})box({2.20f,y,-.58f},{2.56f,y+.10f,1.01f},steelDark);
    verticalCylinder({2.18f,1.44f,.20f},.34f,.08f,12,steelDark,steel);
    // Closed overlapping stones replace the former infinitely thin portal slabs.
    // Their lower halves are deliberately buried into the authored ridge.
    const std::array<std::pair<Vector3,Vector3>,10> rocks{{
        std::pair{Vector3(-3.35f,.65f,-.20f),Vector3(1.55f,1.45f,1.55f)},
        std::pair{Vector3( 3.35f,.62f,-.12f),Vector3(1.62f,1.42f,1.48f)},
        std::pair{Vector3(-2.72f,2.25f,.18f),Vector3(1.35f,1.18f,1.32f)},
        std::pair{Vector3( 2.74f,2.27f,.20f),Vector3(1.38f,1.16f,1.28f)},
        std::pair{Vector3(-1.55f,3.03f,.34f),Vector3(1.30f,.78f,1.15f)},
        std::pair{Vector3( 0.00f,3.18f,.42f),Vector3(1.42f,.72f,1.12f)},
        std::pair{Vector3( 1.55f,3.01f,.31f),Vector3(1.28f,.76f,1.16f)},
        std::pair{Vector3(-4.28f,.18f,.70f),Vector3(1.25f,.82f,1.48f)},
        std::pair{Vector3( 4.24f,.14f,.74f),Vector3(1.24f,.80f,1.42f)},
        std::pair{Vector3( .12f,3.63f,1.04f),Vector3(2.62f,.54f,1.38f)}}};
    for(std::size_t i=0;i<rocks.size();++i)
        ellipsoid(rocks[i].first,rocks[i].second,4,9,
                  i%3==0?Color(76,76,61):(i%3==1?Color(65,68,57):Color(86,83,64)));
    // The terrain shader opens the last 80 cm before the portal. A matching packed
    // apron closes that excavation from below and carries the player smoothly onto
    // the concrete ramp instead of exposing the clear colour through a ground hole.
    quad({-1.48f,layout.FloorRelative(-.9f),-.9f},
         {1.48f,layout.FloorRelative(-.9f),-.9f},
         {1.48f,layout.FloorRelative(0),0},
         {-1.48f,layout.FloorRelative(0),0},Color(66,67,58));
    box({-1.50f,.13f,-.86f},{1.50f,.20f,-.72f},Color(181,150,57));

    // Descending access passage and main spine. Six sections keep the ramp welded
    // while allowing light pools to alternate along the otherwise restrained concrete.
    constexpr std::array<float,10> corridorStops{{0,5,10,14,19,24,27,30,33,36}};
    for(std::size_t i=0;i+1<corridorStops.size();++i) {
        const float d0=corridorStops[i],d1=corridorStops[i+1];
        const float f0=layout.FloorRelative(d0),f1=layout.FloorRelative(d1);
        const Color wall=i%2?concrete:concreteLight;
        quad({-1.36f,f0,d0},{1.36f,f0,d0},{1.36f,f1,d1},{-1.36f,f1,d1},floor);
        const float c0=corridorCeiling(d0),c1=corridorCeiling(d1);
        quad({-1.36f,c0,d0},{-1.36f,c1,d1},{1.36f,c1,d1},{1.36f,c0,d0},concreteDark);
        const bool rightDoor=(d0>=14&&d1<=19)||(d0>=30&&d1<=33);
        const bool leftDoor=d0>=27&&d1<=30;
        if(!rightDoor)quad({1.36f,f0,d0},{1.36f,c0,d0},
                           {1.36f,c1,d1},{1.36f,f1,d1},wall);
        if(!leftDoor)quad({-1.36f,f1,d1},{-1.36f,c1,d1},
                          {-1.36f,c0,d0},{-1.36f,f0,d0},wall);
    }
    for(float depth:{3.0f,8.0f,13.5f,21.5f,28.0f,34.0f}) {
        const float y=layout.FloorRelative(depth)+2.70f;
        box({-.58f,y-.085f,depth-.38f},{.58f,y-.035f,depth+.38f},steelDark);
        box({-.48f,y-.045f,depth-.28f},{.48f,y+.01f,depth+.28f},warm,true);
        quad({-1.12f,y-.08f,depth-.72f},{1.12f,y-.08f,depth-.72f},
             {1.12f,y-.08f,depth+.72f},{-1.12f,y-.08f,depth+.72f},
             Color(214,199,151,70),true);
    }
    // The descending access used to be a long unbroken concrete wedge in first-person
    // view. Recessed framed panels follow its actual sloping floor and ceiling, while
    // paired conduits and expansion joints carry the authored corridor rhythm all the
    // way to the entrance rather than beginning only after the ramp.
    const auto rampPanel=[&](float d0,float d1) {
        const float f0=layout.FloorRelative(d0),f1=layout.FloorRelative(d1);
        const float c0=corridorCeiling(d0),c1=corridorCeiling(d1);
        for(float side:{-1.0f,1.0f}) {
            const float face=side*1.345f;
            const float trim=side*1.337f;
            const auto panelQuad=[&](float low0,float low1,float high0,float high1,
                                     const Color& color,float x) {
                quad({x,low0,d0},{x,high0,d0},{x,high1,d1},{x,low1,d1},color);
            };
            panelQuad(f0+.43f,f1+.43f,c0-.43f,c1-.43f,Color(46,52,51),face);
            panelQuad(f0+.36f,f1+.36f,f0+.43f,f1+.43f,steel,trim);
            panelQuad(c0-.43f,c1-.43f,c0-.36f,c1-.36f,steel,trim);
            const float edge=.065f;
            const auto upright=[&](float depth,float floorAt,float ceilingAt) {
                quad({trim,floorAt+.36f,depth-edge},{trim,ceilingAt-.36f,depth-edge},
                     {trim,ceilingAt-.36f,depth+edge},{trim,floorAt+.36f,depth+edge},steel);
            };
            upright(d0,f0,c0);upright(d1,f1,c1);
            const float mid=(d0+d1)*.5f;
            const float floorMid=layout.FloorRelative(mid);
            for(float yOffset:{.54f,1.72f})
                box({std::min(face,trim)-.012f,floorMid+yOffset-.035f,mid-.045f},
                    {std::max(face,trim)+.012f,floorMid+yOffset+.035f,mid+.045f},
                    Color(118,124,117));
        }
    };
    rampPanel(.72f,2.62f);rampPanel(3.02f,5.28f);
    rampPanel(5.68f,7.72f);rampPanel(8.10f,9.72f);
    tube({-1.04f,corridorCeiling(.55f)-.21f,.55f},
         {-1.04f,corridorCeiling(9.78f)-.21f,9.78f},.038f,7,Color(54,91,96),false);
    tube({1.04f,corridorCeiling(.55f)-.21f,.55f},
         {1.04f,corridorCeiling(9.78f)-.21f,9.78f},.038f,7,Color(122,91,54),false);
    for(float depth:{2.5f,5.0f,7.5f,9.75f}) {
        const float d0=depth-.025f,d1=depth+.025f;
        const float f0=layout.FloorRelative(d0),f1=layout.FloorRelative(d1);
        quad({-1.30f,f0+.006f,d0},{1.30f,f0+.006f,d0},
             {1.30f,f1+.006f,d1},{-1.30f,f1+.006f,d1},Color(31,36,36));
        const float ceiling=corridorCeiling(depth);
        box({-1.12f,ceiling-.27f,depth-.045f},{-0.96f,ceiling-.14f,depth+.045f},steelDark);
        box({.96f,ceiling-.27f,depth-.045f},{1.12f,ceiling-.14f,depth+.045f},steelDark);
    }
    // Repeated structural bays, base rails, utility pipes and a centre safety stripe
    // give the corridor scale and rhythm without extra draw calls.
    for(float depth:{10.4f,14.0f,18.0f,22.0f,26.0f,30.0f,34.0f}) {
        box({-1.39f,roomFloor,depth-.055f},{-1.25f,roomCeiling,depth+.055f},steelDark);
        box({1.25f,roomFloor,depth-.055f},{1.39f,roomCeiling,depth+.055f},steelDark);
    }
    box({-1.38f,roomFloor+.12f,10.0f},{-1.25f,roomFloor+.30f,36.0f},steel);
    box({1.25f,roomFloor+.12f,10.0f},{1.38f,roomFloor+.30f,36.0f},steel);
    box({-1.29f,roomCeiling-.33f,10.0f},{-1.13f,roomCeiling-.18f,36.0f},Color(103,90,60));
    box({1.13f,roomCeiling-.49f,10.0f},{1.27f,roomCeiling-.36f,36.0f},Color(62,87,91));
    quad({-.09f,roomFloor+.006f,10},{.09f,roomFloor+.006f,10},
         {.09f,roomFloor+.006f,36},{-.09f,roomFloor+.006f,36},Color(177,147,49));
    // Reinforced colour-coded room portals.
    box({1.27f,roomFloor,14.0f},{1.48f,roomCeiling,14.16f},Color(106,116,113));
    box({1.27f,roomFloor,18.84f},{1.48f,roomCeiling,19.0f},Color(106,116,113));
    box({1.27f,roomCeiling-.18f,14.0f},{1.48f,roomCeiling,19.0f},Color(82,139,132));
    box({-1.48f,roomFloor,27.0f},{-1.27f,roomCeiling,27.16f},Color(118,112,91));
    box({-1.48f,roomFloor,29.84f},{-1.27f,roomCeiling,30.0f},Color(118,112,91));
    box({-1.48f,roomCeiling-.18f,27.0f},{-1.27f,roomCeiling,30.0f},Color(168,132,52));
    box({1.27f,roomFloor,30.0f},{1.48f,roomCeiling,30.16f},Color(105,111,107));
    box({1.27f,roomFloor,32.84f},{1.48f,roomCeiling,33.0f},Color(105,111,107));
    box({1.27f,roomCeiling-.18f,30.0f},{1.48f,roomCeiling,33.0f},Color(139,104,66));
    // Recessed lower panels, fasteners and restrained colour coding prevent the
    // long spine from reading as a single flat grey extrusion.
    for(float depth:{11.8f,15.8f,20.8f,25.2f,31.2f,34.5f}) {
        box({-1.355f,roomFloor+.38f,depth-.68f},{-1.315f,roomFloor+1.38f,depth+.68f},
            Color(51,58,57));
        box({1.315f,roomFloor+.38f,depth-.68f},{1.355f,roomFloor+1.38f,depth+.68f},
            Color(51,58,57));
        for(float z:{depth-.54f,depth+.54f}) {
            box({-1.31f,roomFloor+.47f,z-.025f},{-1.27f,roomFloor+.53f,z+.025f},steel);
            box({1.27f,roomFloor+.47f,z-.025f},{1.31f,roomFloor+.53f,z+.025f},steel);
        }
    }
    // A sealed maintenance bulkhead closes the spine; without it the camera could
    // see the underside of the island heightfield beyond the authored facility.
    box({-1.36f,roomFloor,35.92f},{1.36f,roomCeiling,36.10f},steelDark);
    box({-1.12f,roomFloor+.18f,35.88f},{1.12f,roomCeiling-.18f,35.94f},Color(69,77,76));
    // A proper wheel replaces the old gold rectangular cross, which looked like
    // signage rather than maintenance-door hardware. It remains entirely in front
    // of the already-solid bulkhead and therefore cannot reopen the terrain leak.
    const Color wheelMetal(153,126,56),wheelEdge(99,83,48);
    const Vector3 wheelCentre(0,roomFloor+1.43f,35.79f);
    constexpr int WheelSegments=16;
    for(int segment=0;segment<WheelSegments;++segment) {
        const float a0=tau*segment/WheelSegments,a1=tau*(segment+1)/WheelSegments;
        const Vector3 p0=wheelCentre+Vector3(std::cos(a0)*.48f,std::sin(a0)*.48f,0);
        const Vector3 p1=wheelCentre+Vector3(std::cos(a1)*.48f,std::sin(a1)*.48f,0);
        tube(p0,p1,.035f,7,wheelMetal,false);
    }
    for(int spoke=0;spoke<6;++spoke) {
        const float angle=tau*spoke/6;
        const Vector3 direction(std::cos(angle),std::sin(angle),0);
        tube(wheelCentre+direction*.11f,wheelCentre+direction*.44f,.027f,7,wheelEdge);
    }
    tube(wheelCentre-Vector3(0,0,.075f),wheelCentre+Vector3(0,0,.075f),.115f,10,wheelMetal);
    for(int bolt=0;bolt<8;++bolt) {
        const float angle=tau*(static_cast<float>(bolt)+.5f)/8;
        ellipsoid(wheelCentre+Vector3(std::cos(angle)*.82f,std::sin(angle)*.82f,-.015f),
                  Vector3(.045f,.045f,.024f),3,7,Color(126,132,123));
    }
    // Compact retro status panel breaks the otherwise monochrome door leaf and
    // communicates powered-but-sealed machinery without becoming an objective UI.
    box({-.98f,roomFloor+2.26f,35.77f},{-.42f,roomFloor+2.53f,35.84f},steelDark);
    const std::array<Color,3> IndicatorColors{{
        Color(185,61,42,235),Color(206,154,51,235),Color(67,170,126,235)}};
    for(int indicator=0;indicator<3;++indicator) {
        const float x=-.88f+indicator*.18f;
        box({x-.045f,roomFloor+2.34f,35.755f},{x+.045f,roomFloor+2.44f,35.77f},
            IndicatorColors[static_cast<std::size_t>(indicator)],true);
    }
    box({-.52f,roomCeiling-.42f,35.80f},{.52f,roomCeiling-.27f,35.89f},Color(160,55,43),true);
    box({-.028f,roomFloor+.18f,35.765f},{.028f,roomCeiling-.20f,35.79f},Color(47,54,54));
    for(int plate=0;plate<10;++plate) {
        const float x0=-1.04f+static_cast<float>(plate)*.208f;
        box({x0,roomFloor+.20f,35.755f},{x0+.174f,roomFloor+.32f,35.78f},
            plate%2==0?Color(185,144,40):Color(31,36,36));
    }
    // Near-ramp ribs and cable trays make the first descent as authored as the
    // deep spine; these sit against the shell and do not narrow capsule clearance.
    for(float depth:{2.0f,5.0f,8.0f}) {
        const float base=layout.FloorRelative(depth),top=corridorCeiling(depth);
        box({-1.355f,base,depth-.045f},{-1.24f,top,depth+.045f},steelDark);
        box({1.24f,base,depth-.045f},{1.355f,top,depth+.045f},steelDark);
        box({-1.25f,top-.10f,depth-.045f},{1.25f,top,depth+.045f},steelDark);
    }

    // Computer room: lit floor/ceiling shell, desks, four active terminals and server racks.
    quad({1.05f,roomFloor,11},{9.25f,roomFloor,11},{9.25f,roomFloor,23.2f},
         {1.05f,roomFloor,23.2f},floor);
    quad({1.05f,roomCeiling,23.2f},{9.25f,roomCeiling,23.2f},{9.25f,roomCeiling,11},
         {1.05f,roomCeiling,11},concreteDark);
    quad({9.25f,roomFloor,11},{9.25f,roomCeiling,11},{9.25f,roomCeiling,23.2f},
         {9.25f,roomFloor,23.2f},concrete);
    quad({1.05f,roomFloor,11},{1.05f,roomCeiling,11},{9.25f,roomCeiling,11},
         {9.25f,roomFloor,11},concreteLight);
    quad({9.25f,roomFloor,23.2f},{9.25f,roomCeiling,23.2f},{1.05f,roomCeiling,23.2f},
         {1.05f,roomFloor,23.2f},concreteLight);
    for(float depth:{13.0f,16.0f,19.0f,22.0f}) {
        box({5.25f,roomFloor+.64f,depth-.78f},{8.25f,roomFloor+.78f,depth+.78f},steel);
        box({6.25f,roomFloor+.78f,depth-.48f},{6.48f,roomFloor+1.82f,depth+.48f},steelDark);
        quad({6.235f,roomFloor+.91f,depth-.37f},{6.235f,roomFloor+1.69f,depth-.37f},
             {6.235f,roomFloor+1.69f,depth+.37f},{6.235f,roomFloor+.91f,depth+.37f},
             Color(13,31,32));
        for(int row=0;row<4;++row) {
            const float y=roomFloor+1.02f+row*.145f;
            const float width=.20f+.07f*((row+static_cast<int>(depth))%3);
            quad({6.225f,y,depth-width},{6.225f,y+.052f,depth-width},
                 {6.225f,y+.052f,depth+width},{6.225f,y,depth+width},
                 row==0?Color(91,210,184,235):(row==3?Color(211,157,64,235):screen),true);
        }
        box({5.48f,roomFloor+.79f,depth-.48f},{6.06f,roomFloor+.83f,depth+.48f},concreteDark);
        for(int key=0;key<6;++key) {
            const float z=depth-.36f+key*.145f;
            box({5.62f,roomFloor+.83f,z-.035f},{5.92f,roomFloor+.855f,z+.035f},
                key==5?Color(190,132,48):Color(85,123,116));
        }
        // Low wheeled operator chair, kept clear of the room's walking aisle.
        box({4.30f,roomFloor+.44f,depth-.40f},{4.92f,roomFloor+.56f,depth+.40f},Color(43,48,49));
        box({4.42f,roomFloor+.54f,depth-.34f},{4.54f,roomFloor+1.23f,depth+.34f},steelDark);
        box({4.18f,roomFloor+.10f,depth-.05f},{4.88f,roomFloor+.16f,depth+.05f},steelDark);
    }
    for(float depth:{12.2f,15.1f,18.0f,20.9f})
        box({8.53f,roomFloor+.05f,depth-.55f},{9.08f,roomCeiling-.12f,depth+.55f},steelDark);
    for(float depth:{14.0f,20.0f})box({3.2f,roomCeiling-.06f,depth-1.0f},
                                      {7.2f,roomCeiling-.01f,depth+1.0f},warm,true);
    box({8.98f,roomFloor+.08f,11.2f},{9.18f,roomFloor+.26f,22.9f},Color(86,94,91));
    for(float depth:{12.2f,15.1f,18.0f,20.9f})
        box({8.48f,roomFloor+.86f,depth-.04f},{9.14f,roomFloor+.93f,depth+.04f},Color(77,166,150),true);
    box({9.12f,roomFloor+1.98f,11.35f},{9.20f,roomFloor+2.12f,22.82f},Color(69,137,130));
    for(float depth:{12.2f,15.1f,18.0f,20.9f})
        box({8.42f,roomFloor+.08f,depth-.58f},{8.50f,roomCeiling-.12f,depth+.58f},Color(102,109,105));
    // Floor tile joints and a wall-mounted situation display add human scale.
    for(float across:{2.6f,4.2f,5.8f,7.4f})
        box({across-.018f,roomFloor+.004f,11.2f},{across+.018f,roomFloor+.018f,23.0f},
            Color(73,78,75));
    for(float depth:{12.5f,14.5f,16.5f,18.5f,20.5f,22.5f})
        box({1.18f,roomFloor+.004f,depth-.018f},{9.05f,roomFloor+.018f,depth+.018f},
            Color(73,78,75));
    box({9.18f,roomFloor+1.10f,14.2f},{9.23f,roomFloor+2.32f,19.9f},Color(24,39,39));
    quad({9.225f,roomFloor+1.24f,14.48f},{9.225f,roomFloor+2.17f,14.48f},
         {9.225f,roomFloor+2.17f,19.62f},{9.225f,roomFloor+1.24f,19.62f},Color(44,125,120),true);
    for(float z:{15.0f,15.8f,16.9f,18.1f,19.0f})
        box({9.23f,roomFloor+1.48f,z-.30f},{9.245f,roomFloor+1.54f,z+.30f},Color(137,218,192),true);

    // Reactor/generator room. The glowing core, concentric containment rings, pipes
    // and control cabinets are deliberately fictional scenery, not an operational design.
    quad({-10.1f,roomFloor,22.8f},{-1.05f,roomFloor,22.8f},{-1.05f,roomFloor,36.2f},
         {-10.1f,roomFloor,36.2f},floor);
    quad({-10.1f,roomCeiling,36.2f},{-1.05f,roomCeiling,36.2f},{-1.05f,roomCeiling,22.8f},
         {-10.1f,roomCeiling,22.8f},concreteDark);
    quad({-10.1f,roomFloor,36.2f},{-10.1f,roomCeiling,36.2f},{-10.1f,roomCeiling,22.8f},
         {-10.1f,roomFloor,22.8f},concrete);
    quad({-10.1f,roomFloor,22.8f},{-10.1f,roomCeiling,22.8f},{-1.05f,roomCeiling,22.8f},
         {-1.05f,roomFloor,22.8f},concreteLight);
    quad({-1.05f,roomFloor,36.2f},{-1.05f,roomCeiling,36.2f},{-10.1f,roomCeiling,36.2f},
         {-10.1f,roomFloor,36.2f},concreteLight);
    verticalCylinder({-5.55f,roomFloor+.30f,29.5f},.92f,2.18f,20,cyan,cyan,true);
    verticalCylinder({-5.55f,roomFloor+.42f,29.5f},.42f,1.94f,16,
                     Color(157,238,235,205),Color(199,247,233,220),true);
    for(int post=0;post<8;++post) {
        const float angle=tau*post/8;
        const float x=-5.55f+std::cos(angle)*1.42f,z=29.5f+std::sin(angle)*1.42f;
        box({x-.065f,roomFloor+.12f,z-.065f},{x+.065f,roomFloor+2.67f,z+.065f},steelDark);
    }
    // Open tubular containment hoops replace the former solid pancake-like discs.
    // They keep the luminous core visible while the vertical posts and radial braces
    // now read as one plausible protective cage from every walkable side.
    const auto containmentRing=[&](float y,float radius,float thickness,
                                   const Color& color,bool lit=false) {
        constexpr int ringSegments=16;
        for(int segment=0;segment<ringSegments;++segment) {
            const float a=tau*segment/ringSegments,b=tau*(segment+1)/ringSegments;
            const Vector3 p0(-5.55f+std::cos(a)*radius,y,29.5f+std::sin(a)*radius);
            const Vector3 p1(-5.55f+std::cos(b)*radius,y,29.5f+std::sin(b)*radius);
            tube(p0,p1,thickness,6,color,false,lit);
        }
    };
    for(float y:{roomFloor+.34f,roomFloor+.98f,roomFloor+1.62f,roomFloor+2.34f})
        containmentRing(y,1.72f,.075f,Color(121,130,124));
    for(float y:{roomFloor+.82f,roomFloor+1.92f})
        containmentRing(y,.945f,.025f,Color(108,225,226,185),true);
    for(float y:{roomFloor+.98f,roomFloor+1.62f}) {
        for(int brace=0;brace<8;++brace) {
            const float angle=tau*(static_cast<float>(brace)+.5f)/8.0f;
            const Vector3 direction(std::cos(angle),0,std::sin(angle));
            tube(Vector3(-5.55f,y,29.5f)+direction*.98f,
                 Vector3(-5.55f,y,29.5f)+direction*1.62f,
                 .035f,6,Color(91,100,98),false);
        }
    }
    verticalCylinder({-5.55f,roomFloor+.08f,29.5f},1.10f,.18f,20,
                     Color(82,90,88),steel);
    // Alternating floor caution bars remain outside the solid containment radius.
    for(int stripe=0;stripe<7;++stripe) {
        const float z=26.0f+stripe*.42f;
        quad({-8.95f,roomFloor+.008f,z},{-7.35f,roomFloor+.008f,z},
             {-7.35f,roomFloor+.008f,z+.20f},{-8.95f,roomFloor+.008f,z+.20f},
             stripe%2?Color(42,46,44):Color(184,143,42));
    }
    for(float across:{-9.35f,-8.45f})
        box({across-.24f,roomFloor+.1f,25.0f},{across+.24f,roomCeiling-.2f,34.0f},steelDark);
    for(float depth:{25.1f,33.9f})
        box({-3.0f,roomFloor+.1f,depth-.28f},{-1.65f,roomFloor+1.75f,depth+.28f},steel);
    box({-9.75f,roomFloor+.4f,35.72f},{-6.9f,roomFloor+1.65f,35.98f},steelDark);
    for(int i=0;i<5;++i)
        box({-9.48f+i*.52f,roomFloor+.72f,35.68f},
            {-9.18f+i*.52f,roomFloor+1.31f,35.70f},i%2?screen:Color(213,151,62),true);
    for(float depth:{25.8f,33.2f})box({-8.8f,roomCeiling-.06f,depth-.85f},
                                      {-3.0f,roomCeiling-.01f,depth+.85f},warm,true);
    box({-10.04f,roomFloor+.55f,24.5f},{-9.99f,roomFloor+2.45f,34.7f},Color(45,73,73));
    for(float depth:{25.2f,27.6f,30.0f,32.4f,34.2f})
        box({-10.02f,roomFloor+1.08f,depth-.54f},{-9.96f,roomFloor+1.18f,depth+.54f},
            Color(61,140,132),true);

    // Service/workshop room: a complete shell, cable cabinets, work bench and
    // amber task lighting. It closes the previously unrendered walkable branch.
    quad({1.05f,roomFloor,25.8f},{7.10f,roomFloor,25.8f},{7.10f,roomFloor,35.5f},
         {1.05f,roomFloor,35.5f},Color(53,57,55));
    quad({1.05f,roomCeiling,35.5f},{7.10f,roomCeiling,35.5f},{7.10f,roomCeiling,25.8f},
         {1.05f,roomCeiling,25.8f},concreteDark);
    quad({7.10f,roomFloor,25.8f},{7.10f,roomCeiling,25.8f},{7.10f,roomCeiling,35.5f},
         {7.10f,roomFloor,35.5f},concrete);
    quad({1.05f,roomFloor,25.8f},{1.05f,roomCeiling,25.8f},{7.10f,roomCeiling,25.8f},
         {7.10f,roomFloor,25.8f},concreteLight);
    quad({7.10f,roomFloor,35.5f},{7.10f,roomCeiling,35.5f},{1.05f,roomCeiling,35.5f},
         {1.05f,roomFloor,35.5f},concreteLight);
    for(float depth:{27.0f,29.1f,33.8f}) {
        box({6.46f,roomFloor+.08f,depth-.72f},{6.94f,roomCeiling-.16f,depth+.72f},steelDark);
        for(int lamp=0;lamp<4;++lamp) {
            const float y=roomFloor+.52f+lamp*.47f;
            box({6.42f,y,depth-.36f},{6.45f,y+.08f,depth+.36f},
                lamp==0?Color(201,145,49):Color(77,162,145),true);
        }
    }
    box({3.15f,roomFloor+.66f,34.25f},{5.95f,roomFloor+.82f,35.15f},steel);
    box({3.30f,roomFloor+.10f,34.38f},{3.50f,roomFloor+.66f,35.02f},steelDark);
    box({5.60f,roomFloor+.10f,34.38f},{5.80f,roomFloor+.66f,35.02f},steelDark);
    box({3.45f,roomFloor+.83f,34.52f},{5.55f,roomFloor+1.06f,34.90f},Color(103,78,48));
    for(float depth:{28.1f,33.0f})
        box({2.25f,roomCeiling-.06f,depth-.72f},{5.95f,roomCeiling-.01f,depth+.72f},
            Color(227,188,113),true);
    box({1.12f,roomFloor+.10f,26.0f},{1.24f,roomFloor+.26f,35.3f},Color(107,82,51));
    // A compact wall-mounted distribution board gives the service room a readable
    // purpose without placing non-colliding clutter in its walkable centre aisle.
    box({6.84f,roomFloor+.40f,29.92f},{7.04f,roomFloor+2.38f,32.92f},steelDark);
    box({6.80f,roomFloor+.34f,29.84f},{6.87f,roomFloor+2.45f,29.96f},steel);
    box({6.80f,roomFloor+.34f,32.88f},{6.87f,roomFloor+2.45f,33.00f},steel);
    box({6.80f,roomFloor+2.34f,29.88f},{6.87f,roomFloor+2.45f,32.96f},steel);
    const std::array<Color,3> dialFaces{{Color(21,36,37),Color(38,33,23),Color(28,31,45)}};
    const std::array<float,3> dialDepths{{30.48f,31.42f,32.36f}};
    for(std::size_t dial=0;dial<dialDepths.size();++dial) {
        const float depth=dialDepths[dial];
        const Vector3 centre(6.78f,roomFloor+1.88f,depth);
        constexpr int dialSegments=12;
        for(int segment=0;segment<dialSegments;++segment) {
            const float a=tau*segment/dialSegments,b=tau*(segment+1)/dialSegments;
            const Vector3 p0=centre+Vector3(0,std::sin(a)*.19f,std::cos(a)*.19f);
            const Vector3 p1=centre+Vector3(0,std::sin(b)*.19f,std::cos(b)*.19f);
            triangle(centre,p1,p0,shade(dialFaces[dial],.86f+.035f*segment));
            tube(p0,p1,.018f,6,Color(132,139,132),false);
        }
        tube(centre-Vector3(.025f,0,0),
             centre+Vector3(-.025f,.105f,-.065f),.014f,6,Color(214,174,65));
    }
    const std::array<Color,6> switchColors{{Color(58,159,143),Color(195,145,43),
        Color(177,67,48),Color(61,126,174),Color(176,184,146),Color(112,87,153)}};
    for(int bank=0;bank<3;++bank) {
        const float y=roomFloor+.62f+bank*.30f;
        box({6.76f,y,30.16f},{6.81f,y+.14f,32.66f},Color(42,49,49));
        for(int toggle=0;toggle<6;++toggle) {
            const float z=30.34f+toggle*.42f;
            box({6.72f,y+.035f,z-.065f},{6.77f,y+.11f,z+.065f},
                switchColors[static_cast<std::size_t>((toggle+bank*2)%6)]);
        }
    }
    for(std::size_t lamp=0;lamp<switchColors.size();++lamp) {
        const float z=30.25f+static_cast<float>(lamp)*.46f;
        const Color source=switchColors[lamp];
        box({6.70f,roomFloor+2.18f,z-.07f},{6.76f,roomFloor+2.29f,z+.07f},
            Color(static_cast<int>(source.getRProperty()),
                  static_cast<int>(source.getGProperty()),
                  static_cast<int>(source.getBProperty()),220),true);
    }
    tube({6.82f,roomFloor+2.42f,30.48f},{6.82f,roomCeiling-.12f,30.48f},
         .033f,6,Color(64,121,125),false);
    tube({6.82f,roomFloor+2.42f,32.36f},{6.82f,roomCeiling-.12f,32.36f},
         .033f,6,Color(132,91,53),false);

    if(opaque.empty()||glow.empty()||opaque.size()+glow.size()>24000)
        throw std::runtime_error("Bunker mesh exceeds bounded static budget");
    Vector3 low(std::numeric_limits<float>::max()),high(std::numeric_limits<float>::lowest());
    for(const auto& v:opaque){low=Vector3::Min(low,v.Position);high=Vector3::Max(high,v.Position);}
    for(const auto& v:glow){low=Vector3::Min(low,v.Position);high=Vector3::Max(high,v.Position);}
    bounds_=BoundingBox(low,high);centre_=(low+high)*.5f;
    opaqueTriangles_=static_cast<int>(opaque.size()/3);glowTriangles_=static_cast<int>(glow.size()/3);
    opaque_=std::make_unique<VertexBuffer>(device,VertexPositionColor::getVertexDeclarationStatic(),
        static_cast<int>(opaque.size()),BufferUsage::WriteOnly);
    opaque_->SetData(opaque.data(),static_cast<int>(opaque.size()));
    glow_=std::make_unique<VertexBuffer>(device,VertexPositionColor::getVertexDeclarationStatic(),
        static_cast<int>(glow.size()),BufferUsage::WriteOnly);
    glow_->SetData(glow.data(),static_cast<int>(glow.size()));
    effect_->setVertexColorEnabledProperty(true);effect_->setLightingEnabledProperty(false);
    effect_->setFogEnabledProperty(true);
    effect_->setFogColorProperty({environment::FogColorRed,environment::FogColorGreen,environment::FogColorBlue});
    effect_->setFogStartProperty(190);effect_->setFogEndProperty(330);
    if(device_.SupportsCapability(CNA::GraphicsCapability::CustomEffects)) {
        try {
            using CNA::Graphics::ShaderCodeEXT;
            const CNA::Graphics::ShaderPackageEXT package({
                ShaderCodeEXT(CNA::ShaderLanguageEXT::GlslEs,CNA::ShaderStageEXT::Vertex,
                              "main","bunker.es.vert",BunkerShaderSource(true,BunkerVertex)),
                ShaderCodeEXT(CNA::ShaderLanguageEXT::GlslEs,CNA::ShaderStageEXT::Fragment,
                              "main","bunker.es.frag",BunkerShaderSource(true,BunkerFragment)),
                ShaderCodeEXT(CNA::ShaderLanguageEXT::GlslDesktop,CNA::ShaderStageEXT::Vertex,
                              "main","bunker.vert",BunkerShaderSource(false,BunkerVertex)),
                ShaderCodeEXT(CNA::ShaderLanguageEXT::GlslDesktop,CNA::ShaderStageEXT::Fragment,
                              "main","bunker.frag",BunkerShaderSource(false,BunkerFragment))},
                {CNA::ShaderStageEXT::Vertex,CNA::ShaderStageEXT::Fragment},{});
            materialEffect_=std::make_unique<ShaderEffect>(device_,package);
            if(!materialEffect_->IsEffectValid()) {
                std::cerr<<"island-simulator: bunker material fallback: "
                         <<materialEffect_->GetCompileErrorEXT()<<'\n';
                materialEffect_.reset();
            }
        } catch(const std::exception& error) {
            std::cerr<<"island-simulator: bunker material fallback: "<<error.what()<<'\n';
            materialEffect_.reset();
        }
    }
    const std::string structure="island-simulator: concealed inland bunker has an open rock portal, descending access corridor, computer room, service room, and fictional reactor-generator chamber; triangles="+std::to_string(TriangleCount());
    std::cout<<structure<<'\n';MirrorRuntimeDiagnosticToPlatformLog(structure);
    const char* lighting="island-simulator: bunker uses bounded baked light pools, luminous ceiling fixtures, active terminal screens, and a contained cyan generator core in two static draws";
    std::cout<<lighting<<'\n';MirrorRuntimeDiagnosticToPlatformLog(lighting);
    const char* interior="island-simulator: bunker computer room has four desks and terminals, keyboard consoles, eight server cabinets, and a separate generator control bank";
    std::cout<<interior<<'\n';MirrorRuntimeDiagnosticToPlatformLog(interior);
    const char* material="island-simulator: bunker concrete and metal use portable world-space mottling, restrained age variation, and baked face shading";
    std::cout<<material<<'\n';MirrorRuntimeDiagnosticToPlatformLog(material);
    const char* closure="island-simulator: bunker deep spine ends at a sealed maintenance bulkhead and the service branch has a complete lit shell";
    std::cout<<closure<<'\n';MirrorRuntimeDiagnosticToPlatformLog(closure);
    const char* bulkhead="island-simulator: bunker end bulkhead uses a round six-spoke handwheel, central hub, eight perimeter bolts, and a lower caution band";
    std::cout<<bulkhead<<'\n';MirrorRuntimeDiagnosticToPlatformLog(bulkhead);
    const char* access="island-simulator: bunker access passage uses recessed ramp panels, paired ceiling conduits, transverse floor joints, and housed lights";
    std::cout<<access<<'\n';MirrorRuntimeDiagnosticToPlatformLog(access);
    const char* generator="island-simulator: bunker generator containment uses open tubular hoops, radial braces, luminous core bands, and a raised machinery plinth";
    std::cout<<generator<<'\n';MirrorRuntimeDiagnosticToPlatformLog(generator);
    const char* service="island-simulator: bunker service room adds a framed distribution board, three round gauges, paired conduits, and labeled switch banks";
    std::cout<<service<<'\n';MirrorRuntimeDiagnosticToPlatformLog(service);
}

BunkerRenderer::~BunkerRenderer()=default;

void BunkerRenderer::Draw(const Matrix& view,const Matrix& projection,const Vector3& camera) {
    drawCalls_=0;
    if((centre_-camera).LengthSquared()>380.0f*380.0f
        ||BoundingFrustum(view*projection).Contains(bounds_)==ContainmentType::Disjoint)return;
    device_.setRasterizerStateProperty(RasterizerState::CullNone);
    device_.setDepthStencilStateProperty(DepthStencilState::Default);
    device_.setBlendStateProperty(BlendState::Opaque);
    effect_->setWorldProperty(Matrix::getIdentityProperty());effect_->setViewProperty(view);
    effect_->setProjectionProperty(projection);
    device_.SetVertexBuffer(opaque_.get());device_.setIndicesProperty(nullptr);
    if(materialEffect_) {
        float viewProjection[16];(view*projection).ToColumnMajor(viewProjection);
        materialEffect_->Apply();materialEffect_->SetUniformMat4("uViewProjection",viewProjection);
    } else effect_->Apply();
    device_.DrawPrimitives(PrimitiveType::TriangleList,0,opaqueTriangles_);++drawCalls_;
    device_.setDepthStencilStateProperty(DepthStencilState::DepthRead);
    device_.setBlendStateProperty(BlendState::AlphaBlend);
    device_.SetVertexBuffer(glow_.get());effect_->Apply();
    device_.DrawPrimitives(PrimitiveType::TriangleList,0,glowTriangles_);++drawCalls_;
}

}
