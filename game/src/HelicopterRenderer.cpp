#include "IslandSimulator/HelicopterRenderer.hpp"

#include "IslandSimulator/EnvironmentLighting.hpp"
#include "IslandSimulator/GeneratedTextureBudget.hpp"
#include "IslandSimulator/RuntimeDiagnostics.hpp"
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
#include "Microsoft/Xna/Framework/Graphics/SamplerState.hpp"
#include "Microsoft/Xna/Framework/Graphics/SurfaceFormat.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"
#include "Microsoft/Xna/Framework/Graphics/VertexBuffer.hpp"
#include "Microsoft/Xna/Framework/Graphics/VertexPositionColorTexture.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <numbers>
#include <stdexcept>
#include <vector>

using namespace Microsoft::Xna::Framework;
using namespace Microsoft::Xna::Framework::Graphics;

namespace island {
namespace {
constexpr int FuselageSides=32;
enum class Surface : std::uint8_t { Paint,Metal,Upholstery,Rubber,Cabin,Glass,Emissive };
struct Texel{std::uint8_t r,g,b,a;};
struct FuselageStation{float z,centreY,radiusX,radiusY;};
constexpr std::array<FuselageStation,17> Fuselage{{
    {-2.10f,1.35f,.68f,.64f},{-1.78f,1.38f,.94f,.82f},
    {-.30f,1.43f,1.07f,.98f},{.18f,1.44f,1.08f,.99f},
    {.62f,1.45f,1.075f,.985f},{1.02f,1.45f,1.05f,.97f},
    {1.34f,1.43f,1.00f,.94f},{1.62f,1.40f,.94f,.90f},
    {1.86f,1.37f,.90f,.86f},{2.08f,1.34f,.83f,.80f},
    {2.30f,1.30f,.73f,.72f},{2.48f,1.26f,.61f,.63f},
    {2.64f,1.23f,.48f,.52f},{2.76f,1.20f,.34f,.40f},
    {2.86f,1.18f,.18f,.27f},{2.91f,1.16f,.11f,.19f},
    {2.95f,1.145f,.055f,.105f}}};

bool OpenDoorBand(const float z) {
    return (z>=.18f&&z<=1.48f)||(z>=-1.70f&&z<=-.30f);
}

Color ScaleColor(const Color& color,const float scale) {
    return Color(static_cast<int>(std::clamp(color.getRProperty()*scale,0.0f,255.0f)),
                 static_cast<int>(std::clamp(color.getGProperty()*scale,0.0f,255.0f)),
                 static_cast<int>(std::clamp(color.getBProperty()*scale,0.0f,255.0f)),
                 static_cast<int>(color.getAProperty()));
}

std::uint32_t Hash(const std::uint32_t x,const std::uint32_t y) noexcept {
    std::uint32_t value=x*0x9e3779b9U+y*0x85ebca6bU+0xc2b2ae35U;
    value^=value>>16U;value*=0x7feb352dU;value^=value>>15U;value*=0x846ca68bU;
    return value^(value>>16U);
}

std::vector<Texel> MakeMaterialAtlas() {
    constexpr int size=GeneratedTextureBudget::HelicopterMaterialSize;
    constexpr int half=size/2;
    std::vector<Texel> pixels(static_cast<std::size_t>(size*size));
    for(int y=0;y<size;++y)for(int x=0;x<size;++x) {
        const int localX=x%half,localY=y%half;
        const auto hash=Hash(static_cast<std::uint32_t>(x),static_cast<std::uint32_t>(y));
        int value=238;
        if(x<half&&y<half) {
            // Oxidized matte paint: broad restrained mottling and rare hairline scuffs.
            value=238+static_cast<int>((hash>>4U)%11U)-5
                +static_cast<int>(5.0f*std::sin(localX*.19f+localY*.11f));
            if((hash%347U)<3U)value-=30;
        } else if(x>=half&&y<half) {
            // Brushed aluminium/steel follows one coherent manufacturing direction.
            value=226+((localY%5)==0?13:0)+static_cast<int>((hash>>7U)%7U)-3;
        } else if(x<half) {
            // Woven seat cloth has unequal warp/weft so it does not resemble a checkerboard.
            value=226+((localX%7)==0?17:0)+((localY%5)==0?9:0)
                +static_cast<int>((hash>>9U)%5U)-2;
        } else {
            // Fine rubber/interior stipple, kept bright enough for vertex-colour modulation.
            value=222+static_cast<int>((hash>>12U)%17U)-8;
            if(((localX+2*localY)%19)==0)value-=11;
        }
        value=std::clamp(value,176,255);
        pixels[static_cast<std::size_t>(y*size+x)]={static_cast<std::uint8_t>(value),
            static_cast<std::uint8_t>(value),static_cast<std::uint8_t>(value),255};
    }
    return pixels;
}

std::vector<Texel> Downsample(const std::vector<Texel>& source,const int size) {
    const int next=std::max(1,size/2);std::vector<Texel> result(static_cast<std::size_t>(next*next));
    for(int y=0;y<next;++y)for(int x=0;x<next;++x) {
        unsigned value=0;
        for(int oy=0;oy<2;++oy)for(int ox=0;ox<2;++ox)
            value+=source[static_cast<std::size_t>(std::min(size-1,y*2+oy)*size+std::min(size-1,x*2+ox))].r;
        const auto shade=static_cast<std::uint8_t>(value/4U);
        result[static_cast<std::size_t>(y*next+x)]={shade,shade,shade,255};
    }
    return result;
}

std::unique_ptr<Texture2D> CreateMaterialAtlas(GraphicsDevice& device) {
    constexpr int size=GeneratedTextureBudget::HelicopterMaterialSize;
    auto texture=std::make_unique<Texture2D>(device,size,size,true,SurfaceFormat::Color);
    auto pixels=MakeMaterialAtlas();int levelSize=size,level=0;
    while(true) {
        texture->SetData(level,nullptr,reinterpret_cast<const std::uint8_t*>(pixels.data()),0,
            static_cast<int>(pixels.size()*sizeof(Texel)));
        if(levelSize==1)break;
        pixels=Downsample(pixels,levelSize);levelSize=std::max(1,levelSize/2);++level;
    }
    return texture;
}

Vector2 MaterialUv(const Vector3& point,const Vector3& normal,const Surface surface) {
    if(surface==Surface::Glass||surface==Surface::Emissive)return {.245f,.245f};
    float first=point.X,second=point.Y;
    const Vector3 absolute(std::abs(normal.X),std::abs(normal.Y),std::abs(normal.Z));
    if(absolute.Y>=absolute.X&&absolute.Y>=absolute.Z){first=point.X;second=point.Z;}
    else if(absolute.X>=absolute.Z){first=point.Z;second=point.Y;}
    const auto repeat=[](const float value){return value-std::floor(value);};
    const float u=repeat(first*1.35f),v=repeat(second*1.35f);
    const int quadrant=surface==Surface::Metal?1:surface==Surface::Upholstery?2:
        (surface==Surface::Rubber||surface==Surface::Cabin)?3:0;
    constexpr float pad=.035f,span=.430f;
    return {static_cast<float>(quadrant%2)*.5f+pad+u*span,
            static_cast<float>(quadrant/2)*.5f+pad+v*span};
}
}

HelicopterRenderer::HelicopterRenderer(GraphicsDevice& device,const HelicopterPlacement& placement)
    :device_(device),effect_(std::make_unique<BasicEffect>(device)),
     materialTexture_(CreateMaterialAtlas(device)),
     centre_(placement.x,placement.groundY+1.5f,placement.z) {
    std::vector<VertexPositionColorTexture> mesh,glassMesh;
    mesh.reserve(12000);glassMesh.reserve(2400);
    const float c=std::cos(placement.yawRadians),s=std::sin(placement.yawRadians);
    const auto world=[&](const Vector3& p){return Vector3(placement.x+c*p.X+s*p.Z,
        placement.groundY+p.Y,placement.z-s*p.X+c*p.Z);};
    const Vector3 sun(environment::SunDirectionX,environment::SunDirectionY,
                      environment::SunDirectionZ);
    const auto triangle=[&](Vector3 a,Vector3 b,Vector3 d,const Color& pigment,
                            const Surface surface=Surface::Paint) {
        Vector3 localNormal=Vector3::Cross(b-a,d-a);
        if(localNormal.LengthSquared()<1e-10f)throw std::runtime_error("Degenerate helicopter triangle");
        localNormal.Normalize();
        const Vector2 uvA=MaterialUv(a,localNormal,surface),uvB=MaterialUv(b,localNormal,surface),
                      uvD=MaterialUv(d,localNormal,surface);
        a=world(a);b=world(b);d=world(d);
        Vector3 normal=Vector3::Cross(b-a,d-a);
        normal.Normalize();
        // The open cabin receives broad sky/ground bounce from four doorways. Treating
        // every interior face like an exterior back face made the otherwise modelled
        // controls disappear into a nearly black void. Emissive instruments remain
        // self-lit; cabin trim and upholstery get restrained indirect light only.
        const float ambient=surface==Surface::Emissive?1.0f:
            surface==Surface::Cabin?.70f:surface==Surface::Upholstery?.60f:.48f;
        const float light=ambient+(1.0f-ambient)*std::max(0.0f,Vector3::Dot(normal,sun));
        const Color color=ScaleColor(pigment,light);
        auto& target=pigment.getAProperty()<255?glassMesh:mesh;
        target.emplace_back(a,color,uvA);target.emplace_back(b,color,uvB);target.emplace_back(d,color,uvD);
    };
    const auto smoothTriangle=[&](Vector3 a,Vector3 b,Vector3 d,
                                  Vector3 normalA,Vector3 normalB,Vector3 normalD,
                                  const Color& pigment,
                                  const Surface surface=Surface::Paint) {
        Vector3 localFaceNormal=Vector3::Cross(b-a,d-a);
        if(localFaceNormal.LengthSquared()<1e-10f)
            throw std::runtime_error("Degenerate smooth helicopter triangle");
        localFaceNormal.Normalize();
        const Vector2 uvA=MaterialUv(a,localFaceNormal,surface);
        const Vector2 uvB=MaterialUv(b,localFaceNormal,surface);
        const Vector2 uvD=MaterialUv(d,localFaceNormal,surface);
        a=world(a);b=world(b);d=world(d);
        const auto worldNormal=[&](Vector3 normal) {
            normal=Vector3(c*normal.X+s*normal.Z,normal.Y,-s*normal.X+c*normal.Z);
            normal.Normalize();
            return normal;
        };
        const auto litColor=[&](const Vector3& normal) {
            const float ambient=surface==Surface::Emissive?1.0f:
                surface==Surface::Cabin?.70f:surface==Surface::Upholstery?.60f:.48f;
            const float light=ambient+(1.0f-ambient)
                *std::max(0.0f,Vector3::Dot(worldNormal(normal),sun));
            return ScaleColor(pigment,light);
        };
        auto& target=pigment.getAProperty()<255?glassMesh:mesh;
        target.emplace_back(a,litColor(normalA),uvA);
        target.emplace_back(b,litColor(normalB),uvB);
        target.emplace_back(d,litColor(normalD),uvD);
    };
    const auto quad=[&](const Vector3& a,const Vector3& b,const Vector3& d,const Vector3& e,
                        const Color& color,const Surface surface=Surface::Paint) {
        triangle(a,b,d,color,surface);triangle(a,d,e,color,surface);
    };
    const auto smoothQuad=[&](const Vector3& a,const Vector3& b,
                              const Vector3& d,const Vector3& e,
                              const Vector3& normalA,const Vector3& normalB,
                              const Vector3& normalD,const Vector3& normalE,
                              const Color& color,const Surface surface=Surface::Paint) {
        smoothTriangle(a,b,d,normalA,normalB,normalD,color,surface);
        smoothTriangle(a,d,e,normalA,normalD,normalE,color,surface);
    };
    const auto box=[&](const Vector3& low,const Vector3& high,const Color& color,
                       const Surface surface=Surface::Paint) {
        const std::array<Vector3,8> p{{{low.X,low.Y,low.Z},{high.X,low.Y,low.Z},
            {high.X,high.Y,low.Z},{low.X,high.Y,low.Z},{low.X,low.Y,high.Z},
            {high.X,low.Y,high.Z},{high.X,high.Y,high.Z},{low.X,high.Y,high.Z}}};
        quad(p[0],p[3],p[2],p[1],color,surface);quad(p[4],p[5],p[6],p[7],color,surface);
        quad(p[0],p[1],p[5],p[4],color,surface);quad(p[1],p[2],p[6],p[5],color,surface);
        quad(p[2],p[3],p[7],p[6],color,surface);quad(p[3],p[0],p[4],p[7],color,surface);
    };
    constexpr float tau=2.0f*std::numbers::pi_v<float>;
    const auto beam=[&](const Vector3& a,const Vector3& b,const float width,
                        const Color& color,const Surface surface=Surface::Paint) {
        Vector3 along=b-a;along.Normalize();
        Vector3 across=Vector3::Cross(along,std::abs(along.Y)>.9f?Vector3(0,0,1):Vector3::Up);
        across.Normalize();Vector3 up=Vector3::Cross(across,along);up.Normalize();
        const Vector3 r=across*(width*.5f),sizedUp=up*(width*.5f);
        const std::array<Vector3,8> p{{a-r-sizedUp,a+r-sizedUp,a+r+sizedUp,a-r+sizedUp,
            b-r-sizedUp,b+r-sizedUp,b+r+sizedUp,b-r+sizedUp}};
        quad(p[0],p[3],p[2],p[1],color,surface);quad(p[4],p[5],p[6],p[7],color,surface);
        quad(p[0],p[1],p[5],p[4],color,surface);quad(p[1],p[2],p[6],p[5],color,surface);
        quad(p[2],p[3],p[7],p[6],color,surface);quad(p[3],p[0],p[4],p[7],color,surface);
    };
    const auto tube=[&](const Vector3& a,const Vector3& b,const float radius,
                        const Color& color,const Surface surface=Surface::Paint) {
        constexpr int sides=8;
        Vector3 along=b-a;along.Normalize();
        Vector3 tangent=Vector3::Cross(along,std::abs(along.Y)>.88f
            ?Vector3(0,0,1):Vector3::Up);
        tangent.Normalize();
        Vector3 bitangent=Vector3::Cross(along,tangent);bitangent.Normalize();
        std::array<Vector3,sides> first{},second{};
        for(int side=0;side<sides;++side) {
            const float angle=tau*side/sides;
            const Vector3 offset=(tangent*std::cos(angle)+bitangent*std::sin(angle))*radius;
            first[static_cast<std::size_t>(side)]=a+offset;
            second[static_cast<std::size_t>(side)]=b+offset;
        }
        for(int side=0;side<sides;++side) {
            const int next=(side+1)%sides;
            quad(first[static_cast<std::size_t>(side)],
                 second[static_cast<std::size_t>(side)],
                 second[static_cast<std::size_t>(next)],
                 first[static_cast<std::size_t>(next)],color,surface);
        }
    };
    const auto disc=[&](const Vector3& centre,const float radius,const int segments,
                        const Color& color,const Surface surface=Surface::Rubber) {
        for(int segment=0;segment<segments;++segment) {
            const float first=tau*segment/segments,next=tau*(segment+1)/segments;
            triangle(centre,
                centre+Vector3(std::cos(first)*radius,std::sin(first)*radius,0),
                centre+Vector3(std::cos(next)*radius,std::sin(next)*radius,0),color,surface);
        }
    };
    const auto ellipsoid=[&](const Vector3& centre,const Vector3& radius,
                             const int rings,const int slices,const Color& color,
                             const Surface surface=Surface::Paint) {
        for(int ring=0;ring<rings;++ring) {
            const float latitude0=-std::numbers::pi_v<float>*.5f
                +std::numbers::pi_v<float>*ring/rings;
            const float latitude1=-std::numbers::pi_v<float>*.5f
                +std::numbers::pi_v<float>*(ring+1)/rings;
            for(int slice=0;slice<slices;++slice) {
                const float longitude0=tau*slice/slices;
                const float longitude1=tau*(slice+1)/slices;
                const auto point=[&](const float latitude,const float longitude) {
                    return centre+Vector3(std::cos(latitude)*std::cos(longitude)*radius.X,
                        std::sin(latitude)*radius.Y,
                        std::cos(latitude)*std::sin(longitude)*radius.Z);
                };
                const auto normal=[&](const float latitude,const float longitude) {
                    Vector3 result(std::cos(latitude)*std::cos(longitude)/radius.X,
                        std::sin(latitude)/radius.Y,
                        std::cos(latitude)*std::sin(longitude)/radius.Z);
                    result.Normalize();
                    return result;
                };
                const Vector3 a=point(latitude0,longitude0),b=point(latitude1,longitude0),
                              d=point(latitude1,longitude1),e=point(latitude0,longitude1);
                const Vector3 normalA=normal(latitude0,longitude0);
                const Vector3 normalB=normal(latitude1,longitude0);
                const Vector3 normalD=normal(latitude1,longitude1);
                const Vector3 normalE=normal(latitude0,longitude1);
                if(ring==0)
                    smoothTriangle(a,b,d,normalA,normalB,normalD,color,surface);
                else if(ring==rings-1)
                    smoothTriangle(a,b,e,normalA,normalB,normalE,color,surface);
                else smoothQuad(a,b,d,e,normalA,normalB,normalD,normalE,color,surface);
            }
        }
    };
    const Color olive(105,118,78),oliveLight(142,149,98),oliveDark(54,64,47);
    // Muted grey-green glazing reads as thick aircraft glass in the sunny island
    // palette. The previous saturated blue made the whole cockpit resemble plastic.
    const Color glass(82,101,99,128),metal(91,96,92);
    const Color interior(73,80,71),seat(68,73,61),rubber(30,32,30);
    const auto loftNormal=[](const auto& stations,const std::size_t index,
                             const float angle) {
        const std::size_t previous=index==0?0:index-1;
        const std::size_t next=std::min(index+1,stations.size()-1);
        const float deltaZ=stations[next].z-stations[previous].z;
        const float radiusXSlope=(stations[next].radiusX-stations[previous].radiusX)/deltaZ;
        const float radiusYSlope=(stations[next].radiusY-stations[previous].radiusY)/deltaZ;
        const float centreYSlope=(stations[next].centreY-stations[previous].centreY)/deltaZ;
        const float cosine=std::cos(angle),sine=std::sin(angle);
        Vector3 normal(cosine*stations[index].radiusY,
                       sine*stations[index].radiusX,
                       -sine*stations[index].radiusX*(centreYSlope+sine*radiusYSlope)
                           -cosine*cosine*stations[index].radiusY*radiusXSlope);
        normal.Normalize();
        return normal;
    };
    const auto fuselagePaint=[&](const float z,const float angle) {
        // Broad continuous value variation describes sun-faded painted sheet metal
        // without alternating colours per polygon or introducing another material.
        const float upper=std::max(0.0f,std::sin(angle));
        const float side=1.0f-std::abs(std::cos(angle));
        const float longitudinal=.040f*std::sin(z*2.15f+.35f)
            +.022f*std::sin(z*5.1f+1.2f);
        return ScaleColor(olive,.935f+.080f*upper+.025f*side+longitudinal);
    };

    // Multi-ring fuselage with actual side openings for separate cockpit and rear doors.
    for(std::size_t band=0;band+1<Fuselage.size();++band) {
        const float midZ=(Fuselage[band].z+Fuselage[band+1].z)*.5f;
        for(int slice=0;slice<FuselageSides;++slice) {
            const int next=(slice+1)%FuselageSides;
            const float a=tau*slice/FuselageSides,b=tau*next/FuselageSides,mid=(a+b)*.5f;
            const bool sidePanel=std::abs(std::cos(mid))>.72f;
            if(sidePanel&&OpenDoorBand(midZ))continue;
            const auto point=[](const FuselageStation& station,const float angle) {
                return Vector3(std::cos(angle)*station.radiusX,
                    station.centreY+std::sin(angle)*station.radiusY,station.z);
            };
            Color color=fuselagePaint(midZ,mid);
            Surface surface=Surface::Paint;
            // The windshield wraps progressively below the belt toward the rounded
            // nose. A constant horizontal cutoff made the otherwise smooth loft read
            // as a dark wedge from the normal three-quarter walking view.
            const float windshieldWrap=std::clamp((midZ-1.50f)/1.30f,0.0f,1.0f);
            if(midZ>1.50f&&std::sin(mid)>-.08f-.22f*windshieldWrap) {
                // Consistent glazing pigment lets the denser curved shell and its
                // lighting describe the canopy instead of alternating dark/light
                // wedges that made the cockpit look deliberately polygonal.
                color=glass;surface=Surface::Glass;
            }
            if(std::sin(mid)<-.72f)color=oliveDark;
            smoothQuad(point(Fuselage[band],a),point(Fuselage[band+1],a),
                       point(Fuselage[band+1],b),point(Fuselage[band],b),
                       loftNormal(Fuselage,band,a),loftNormal(Fuselage,band+1,a),
                       loftNormal(Fuselage,band+1,b),loftNormal(Fuselage,band,b),
                       color,surface);
        }
    }
    // Seal the short rounded nose. Two small terminal rings now reduce the final fan
    // to a compact cap instead of letting a large ring converge visibly into one
    // angular point. The rear-cabin sightline remains sealed.
    const auto fuselagePoint=[](const FuselageStation& station,const float angle) {
        return Vector3(std::cos(angle)*station.radiusX,
            station.centreY+std::sin(angle)*station.radiusY,station.z);
    };
    const Vector3 noseTip(0,1.14f,2.975f);
    for(int slice=0;slice<FuselageSides;++slice) {
        const float a=tau*slice/FuselageSides;
        const float b=tau*((slice+1)%FuselageSides)/FuselageSides;
        const float mid=(a+b)*.5f;
        const Color color=std::sin(mid)>-.08f?glass:oliveDark;
        smoothTriangle(fuselagePoint(Fuselage.back(),a),noseTip,
                       fuselagePoint(Fuselage.back(),b),
                       loftNormal(Fuselage,Fuselage.size()-1,a),Vector3(0,0,1),
                       loftNormal(Fuselage,Fuselage.size()-1,b),color,
                       std::sin(mid)>-.08f?Surface::Glass:Surface::Paint);
    }
    // Slim dark structural frames divide the canopy without turning it into a pale
    // cage. The previous thick sunlit tubes dominated the glass and resembled white
    // cracks from the ordinary three-quarter player view.
    const Color windscreenFrame(38,43,36);
    beam({0,1.11f,2.67f},{0,2.12f,1.70f},.048f,windscreenFrame);
    for(const float side:{-1.0f,1.0f})
        beam({side*.57f,1.08f,2.45f},{side*.30f,2.08f,2.06f},.035f,windscreenFrame);
    beam({-.31f,2.08f,2.06f},{.31f,2.08f,2.06f},.034f,windscreenFrame);
    // A rounded chin fairing and slim lower windscreen rail keep the otherwise
    // transparent nose from collapsing into a pointed glass bubble at eye level.
    ellipsoid({0,.87f,2.36f},{.57f,.23f,.36f},4,16,olive);
    beam({-.59f,1.03f,2.47f},{.59f,1.03f,2.47f},.045f,windscreenFrame);

    // Parked paired wipers give the broad canopy a familiar mechanical scale cue.
    // Their articulated arms sit just ahead of the generated glass instead of being
    // painted into it, while two deliberately restrained translucent ribbons break
    // up the otherwise uniform pane without pretending to mirror the whole sky.
    for(const float side:{-1.0f,1.0f}) {
        const Vector3 pivot(side*.25f,1.095f,2.492f);
        const Vector3 elbow(side*.34f,1.32f,2.402f);
        const Vector3 tip(side*.13f,1.70f,2.225f);
        ellipsoid(pivot,{.041f,.033f,.029f},3,8,metal,Surface::Metal);
        tube(pivot,elbow,.016f,rubber,Surface::Rubber);
        tube(elbow,tip,.013f,rubber,Surface::Rubber);
        beam(tip-Vector3(.12f*side,.055f,-.018f),
             tip+Vector3(.12f*side,.055f,-.018f),.014f,rubber,Surface::Rubber);

        // A former broad cyan quadrilateral crossed several canopy facets and read
        // as a white crack in the windshield. Keep one narrow, low-alpha sky glint
        // per pane; the curved glass and frame now carry the main surface read.
        const Color reflection(128,151,148,18);
        quad({side*.18f,1.755f,2.198f},{side*.225f,1.765f,2.205f},
             {side*.395f,1.585f,2.314f},{side*.360f,1.565f,2.326f},
             reflection,Surface::Glass);
    }

    // Flat walkable floor, two-stage door sills and visible frames match collision support.
    box({-.78f,.405f,-1.78f},{.78f,.455f,1.48f},interior,Surface::Cabin);
    for(const float side:{-1.0f,1.0f})for(const auto& door:
        {std::pair{-1.70f,-.30f},std::pair{.18f,1.48f}}) {
        const float lowX=side>0?.78f:-1.23f,highX=side>0?1.23f:-.78f;
        box({lowX,.205f,door.first},{highX,.245f,door.second},oliveDark);
        const float stepInnerX=side*1.25f,stepOuterX=side*1.56f;
        const float stepRearZ=door.first+.12f,stepFrontZ=door.second-.12f;
        tube({stepInnerX,.055f,stepRearZ},{stepInnerX,.055f,stepFrontZ},.026f,
             metal,Surface::Metal);
        tube({stepOuterX,.055f,stepRearZ},{stepOuterX,.055f,stepFrontZ},.026f,
             metal,Surface::Metal);
        for(int rung=0;rung<2;++rung) {
            const float fraction=.28f+static_cast<float>(rung)*.44f;
            const float z=std::lerp(stepRearZ,stepFrontZ,fraction);
            tube({stepInnerX,.055f,z},{stepOuterX,.055f,z},.020f,
                 metal,Surface::Metal);
        }
        // Rounded jambs and a shallow oval roof shoulder continue the curved
        // fuselage around each genuinely open doorway. The former three square
        // beams formed a conspicuous rectangular cage in the broadside view.
        tube({side*.93f,.45f,door.first},{side*.93f,2.13f,door.first},.047f,
             oliveLight);
        tube({side*.93f,.45f,door.second},{side*.93f,2.13f,door.second},.047f,
             oliveLight);
        ellipsoid({side*.93f,2.13f,(door.first+door.second)*.5f},
                  {.105f,.075f,(door.second-door.first)*.5f+.045f},3,12,
                  oliveLight);
    }
    // A rounded belly and paired lower sills restore the continuous load-bearing
    // fuselage below the four open doors. The former perfectly flat floor edge made
    // the aircraft resemble a framed cabin sitting independently on skid rails.
    ellipsoid({0,.28f,-.10f},{.80f,.16f,1.92f},4,18,oliveDark);
    for(const float side:{-1.0f,1.0f})
        ellipsoid({side*.83f,.47f,-.10f},{.15f,.13f,1.90f},4,18,
                  ScaleColor(olive,.84f));

    // Four visibly open doors explain the otherwise skeletal side openings. Pilot
    // panels hinge forward and rear panels hinge aft, leaving the tested perpendicular
    // entry corridors unobstructed. Glass shares the existing depth-read second draw.
    const auto openDoor=[&](const float side,const float hingeZ,const float farZ) {
        const Color doorFrame=ScaleColor(olive,.72f);
        const Vector3 hingeLow(side*.99f,.49f,hingeZ),farLow(side*1.62f,.49f,farZ);
        const auto point=[&](const float across,const float y) {
            return Vector3::Lerp(hingeLow,farLow,across)+Vector3(0,y-.49f,0);
        };
        // A subtly raked belt and roof edge avoids a construction-site rectangle.
        quad(point(0,.49f),point(1,.49f),point(1,.96f),point(0,1.01f),olive);
        quad(point(0,1.91f),point(.82f,1.98f),point(1,2.04f),point(0,2.08f),oliveLight);
        quad(point(.12f,1.03f),point(.88f,1.00f),point(.80f,1.82f),point(.18f,1.88f),
             glass,Surface::Glass);
        beam(point(0,.49f),point(0,2.08f),.052f,doorFrame);
        beam(point(1,.49f),point(1,1.82f),.052f,doorFrame);
        beam(point(1,1.82f),point(.82f,1.98f),.052f,doorFrame);
        beam(point(.82f,1.98f),point(0,1.91f),.052f,doorFrame);
        beam(point(0,.49f),point(1,.49f),.052f,doorFrame);
        beam(point(.10f,1.01f),point(.90f,.98f),.042f,doorFrame);
        beam(point(.70f,1.12f),point(.87f,1.12f),.034f,metal,Surface::Metal);
        // A shallow inset lower skin panel breaks up the former featureless slab
        // while keeping the complete door in the shared mesh.
        const Vector3 skinOffset(side*.009f,0,0);
        quad(point(.14f,.58f)+skinOffset,point(.86f,.58f)+skinOffset,
             point(.86f,.90f)+skinOffset,point(.14f,.92f)+skinOffset,
             ScaleColor(olive,.82f));
        // Two compact barrels make the open panel's load path explicit. They remain
        // tight against the existing jamb and door edge, outside the authoritative
        // entry corridor, and fit the established bounded static mesh allocation.
        for(const float y:{.80f,1.68f}) {
            tube({side*.985f,y-.075f,hingeZ},{side*.985f,y+.075f,hingeZ},
                 .028f,metal,Surface::Metal);
        }
    };
    for(const float side:{-1.0f,1.0f}) {
        openDoor(side,1.46f,2.35f);
        openDoor(side,-1.68f,-2.55f);
    }

    // Two pilot seats, rear bench and articulated primary flight controls. Curved
    // cyclic shafts and separate collectives read as controls rather than seat braces.
    for(const float x:{-.46f,.46f}) {
        // Closed rounded cushions replace the old exposed rectangular seat blocks.
        // A slim frame carries their weight without filling the walkable footwell.
        ellipsoid({x,.70f,.40f},{.29f,.14f,.31f},5,12,
                  Color(72,76,64),Surface::Upholstery);
        ellipsoid({x,1.09f,.17f},{.27f,.36f,.12f},8,16,
                  Color(69,74,62),Surface::Upholstery);
        ellipsoid({x,1.42f,.17f},{.20f,.16f,.12f},4,10,
                  Color(65,69,59),Surface::Upholstery);
        for(const float frameSide:{-1.0f,1.0f}) {
            tube({x+frameSide*.19f,.48f,.18f},{x+frameSide*.19f,.61f,.38f},.022f,
                 metal,Surface::Metal);
            tube({x+frameSide*.19f,.57f,.18f},{x+frameSide*.19f,1.34f,.12f},.022f,
                 metal,Surface::Metal);
        }
        tube({x-.20f,.53f,.18f},{x+.20f,.53f,.18f},.022f,metal,Surface::Metal);
        // Muted harness webbing provides human scale without turning the empty
        // aircraft into an occupied vehicle.
        beam({x-.15f,1.37f,.06f},{x+.02f,.83f,.31f},.028f,Color(151,140,96),Surface::Upholstery);
        beam({x+.15f,1.37f,.06f},{x-.02f,.83f,.31f},.028f,Color(151,140,96),Surface::Upholstery);
        tube({x,.58f,.79f},{x,.74f,.91f},.026f,metal,Surface::Metal);
        tube({x,.74f,.91f},{x,.94f,1.03f},.026f,metal,Surface::Metal);
        tube({x-.085f,.95f,1.03f},{x+.085f,.95f,1.03f},.039f,rubber,Surface::Rubber);
        const float collectiveX=x-.32f; // A collective is conventionally left of each pilot.
        tube({collectiveX,.56f,.34f},{collectiveX,.78f,.48f},.024f,metal,Surface::Metal);
        tube({collectiveX,.78f,.48f},{collectiveX,.91f,.66f},.024f,metal,Surface::Metal);
        tube({collectiveX,.91f,.66f},{collectiveX,.91f,.82f},.038f,rubber,Surface::Rubber);
        for(const float pedalOffset:{-.11f,.11f}) {
            tube({x+pedalOffset,.47f,.92f},{x+pedalOffset,.58f,1.15f},.020f,
                 metal,Surface::Metal);
            box({x+pedalOffset-.070f,.545f,1.12f},{x+pedalOffset+.070f,.615f,1.25f},
                rubber,Surface::Rubber);
        }
    }
    // The rear bench uses two continuous rounded pads rather than an intersecting
    // pair of slabs. Thin recessed seams retain three readable seating positions.
    ellipsoid({0,.70f,-1.56f},{.73f,.14f,.29f},5,14,
              Color(72,76,64),Surface::Upholstery);
    ellipsoid({0,1.07f,-1.61f},{.73f,.34f,.13f},8,18,
              Color(67,72,61),Surface::Upholstery);
    for(const float seam:{-.24f,.24f})
        beam({seam,.80f,-1.477f},{seam,1.33f,-1.477f},.014f,
             Color(48,52,45),Surface::Upholstery);
    // A tapered, shallow instrument binnacle replaces the former full-width black box.
    // Its narrower crown follows the windscreen and leaves the door-side sightlines open.
    const std::array<Vector3,4> panelFront{{
        {-.70f,1.03f,1.30f},{.70f,1.03f,1.30f},{.61f,1.40f,1.30f},{-.61f,1.40f,1.30f}}};
    const std::array<Vector3,4> panelBack{{
        {-.70f,1.03f,1.50f},{.70f,1.03f,1.50f},{.61f,1.40f,1.50f},{-.61f,1.40f,1.50f}}};
    quad(panelFront[0],panelFront[1],panelFront[2],panelFront[3],
         Color(51,58,52),Surface::Cabin);
    quad(panelBack[3],panelBack[2],panelBack[1],panelBack[0],
         Color(43,49,44),Surface::Cabin);
    for(std::size_t edge=0;edge<panelFront.size();++edge) {
        const std::size_t next=(edge+1)%panelFront.size();
        quad(panelFront[edge],panelBack[edge],panelBack[next],panelFront[next],
             Color(48,54,49),Surface::Cabin);
    }
    // A low tapered centre pedestal rises into the panel instead of blocking the
    // footwell as one tall rectangular crate.  Its aft end stays narrow between the
    // cyclics and its sloping crown carries a small, readable switch console.
    const Vector3 pedestalAftLeft(-.14f,.47f,.87f);
    const Vector3 pedestalAftRight(.14f,.47f,.87f);
    const Vector3 pedestalForwardLeft(-.18f,.47f,1.27f);
    const Vector3 pedestalForwardRight(.18f,.47f,1.27f);
    const Vector3 pedestalCrownAftLeft(-.11f,.82f,1.02f);
    const Vector3 pedestalCrownAftRight(.11f,.82f,1.02f);
    const Vector3 pedestalCrownForwardLeft(-.17f,1.02f,1.27f);
    const Vector3 pedestalCrownForwardRight(.17f,1.02f,1.27f);
    const Color pedestalSide(49,56,50);
    quad(pedestalAftLeft,pedestalAftRight,pedestalCrownAftRight,
         pedestalCrownAftLeft,pedestalSide,Surface::Cabin);
    quad(pedestalForwardRight,pedestalForwardLeft,pedestalCrownForwardLeft,
         pedestalCrownForwardRight,pedestalSide,Surface::Cabin);
    quad(pedestalAftLeft,pedestalCrownAftLeft,pedestalCrownForwardLeft,
         pedestalForwardLeft,Color(46,53,48),Surface::Cabin);
    quad(pedestalAftRight,pedestalForwardRight,pedestalCrownForwardRight,
         pedestalCrownAftRight,Color(55,62,56),Surface::Cabin);
    quad(pedestalAftLeft,pedestalForwardLeft,pedestalForwardRight,
         pedestalAftRight,Color(42,48,44),Surface::Cabin);
    quad(pedestalCrownAftLeft,pedestalCrownAftRight,pedestalCrownForwardRight,
         pedestalCrownForwardLeft,Color(60,67,60),Surface::Cabin);
    // The plate follows the same crown slope so it cannot float above the pedestal.
    quad({-.088f,.865f,1.069f},{.088f,.865f,1.069f},
         {.126f,.975f,1.207f},{-.126f,.975f,1.207f},
         Color(27,32,29),Surface::Rubber);
    for(int control=0;control<3;++control) {
        const float x=-.060f+control*.060f;
        const Color controlColor=control==0?Color(184,154,67):
            (control==1?Color(74,139,96):Color(171,74,57));
        quad({x-.015f,.884f,1.091f},{x+.015f,.884f,1.091f},
             {x+.019f,.914f,1.129f},{x-.019f,.914f,1.129f},
             controlColor,Surface::Emissive);
    }
    quad({-.73f,1.38f,1.33f},{.73f,1.38f,1.33f},{.61f,1.48f,1.15f},
         {-.61f,1.48f,1.15f},Color(46,52,47),Surface::Cabin);
    box({-.29f,1.91f,.42f},{.29f,1.99f,1.03f},Color(62,69,61),Surface::Cabin);
    // Recessed round gauges, contrasting bezels, printed tick rings and individual
    // needles replace the former grid of twelve coloured squares. One instrument
    // carries a restrained blue/brown artificial horizon so the group reads as
    // actual flight instrumentation rather than four identical black discs.
    constexpr std::array<float,4> gaugeX{{-.47f,-.16f,.16f,.47f}};
    for(std::size_t gauge=0;gauge<gaugeX.size();++gauge) {
        const Vector3 bezel(gaugeX[gauge],1.27f,1.289f);
        disc(bezel,.095f,16,Color(116,119,111),Surface::Metal);
        disc(bezel-Vector3(0,0,.006f),.073f,16,Color(14,17,15),Surface::Rubber);
        if(gauge==1) {
            const Vector3 horizon=bezel-Vector3(0,0,.0085f);
            for(int segment=0;segment<8;++segment) {
                const float first=std::numbers::pi_v<float>*segment/8.0f;
                const float next=std::numbers::pi_v<float>*(segment+1)/8.0f;
                triangle(horizon,
                    horizon+Vector3(std::cos(first)*.061f,std::sin(first)*.061f,0),
                    horizon+Vector3(std::cos(next)*.061f,std::sin(next)*.061f,0),
                    Color(79,125,147),Surface::Emissive);
                triangle(horizon,
                    horizon+Vector3(std::cos(-first)*.061f,std::sin(-first)*.061f,0),
                    horizon+Vector3(std::cos(-next)*.061f,std::sin(-next)*.061f,0),
                    Color(126,91,61),Surface::Emissive);
            }
            quad(horizon+Vector3(-.060f,-.003f,-.001f),
                 horizon+Vector3(.060f,-.003f,-.001f),
                 horizon+Vector3(.060f,.003f,-.001f),
                 horizon+Vector3(-.060f,.003f,-.001f),
                 Color(218,207,168),Surface::Emissive);
        }
        for(int tick=0;tick<9;++tick) {
            const float tickAngle=-2.35f+4.70f*static_cast<float>(tick)/8.0f;
            const Vector3 radial(std::cos(tickAngle),std::sin(tickAngle),0);
            const Vector3 tangent(-radial.Y,radial.X,0);
            const float halfWidth=tick%4==0?.0045f:.0028f;
            const Vector3 inner=bezel+radial*.052f-Vector3(0,0,.0105f);
            const Vector3 outer=bezel+radial*.066f-Vector3(0,0,.0105f);
            quad(inner-tangent*halfWidth,outer-tangent*halfWidth,
                 outer+tangent*halfWidth,inner+tangent*halfWidth,
                 Color(194,196,181),Surface::Emissive);
        }
        const float angle=-1.03f+static_cast<float>(gauge)*.47f;
        const Vector3 centre=bezel-Vector3(0,0,.013f);
        const Vector3 along(std::cos(angle)*.058f,std::sin(angle)*.058f,0);
        const Vector3 across(-std::sin(angle)*.010f,std::cos(angle)*.010f,0);
        triangle(centre-across,centre+along,centre+across,
                 gauge==0?Color(210,91,66):Color(218,207,168),Surface::Emissive);
        disc(centre-Vector3(0,0,.002f),.010f,8,
             Color(158,160,148),Surface::Metal);
    }
    // Two recessed radio/navigation displays give the lower panel a readable centre
    // stack instead of an undifferentiated row of coloured squares. Dark raised frames
    // and three unequal luminous data rows keep the screens legible in the open cabin.
    box({-.255f,1.055f,1.276f},{-.025f,1.145f,1.296f},
        Color(56,126,105),Surface::Emissive);
    box({.025f,1.055f,1.276f},{.255f,1.145f,1.296f},
        Color(86,118,137),Surface::Emissive);
    for(const float centreX:{-.14f,.14f}) {
        quad({centreX-.130f,1.047f,1.273f},{centreX+.130f,1.047f,1.273f},
             {centreX+.130f,1.153f,1.273f},{centreX-.130f,1.153f,1.273f},
             Color(22,27,24),Surface::Cabin);
        const Color screen=centreX<0?Color(48,116,96):Color(70,105,126);
        quad({centreX-.108f,1.061f,1.270f},{centreX+.108f,1.061f,1.270f},
             {centreX+.108f,1.137f,1.270f},{centreX-.108f,1.137f,1.270f},
             screen,Surface::Emissive);
        for(int row=0;row<3;++row) {
            const float y=1.075f+row*.023f;
            const float halfLength=.082f-row*.014f;
            quad({centreX-halfLength,y-.002f,1.267f},
                 {centreX+halfLength,y-.002f,1.267f},
                 {centreX+halfLength,y+.002f,1.267f},
                 {centreX-halfLength,y+.002f,1.267f},
                 Color(172,206,180),Surface::Emissive);
        }
    }
    for(int column=0;column<6;++column) {
        const Color lamp=column%3==0?Color(194,91,57):
            (column%3==1?Color(89,157,105):Color(204,177,80));
        const float x=column<3?-.56f+column*.09f:.34f+(column-3)*.09f;
        box({x,1.055f,1.279f},{x+.045f,1.105f,1.293f},lamp,Surface::Emissive);
    }

    // A low faceted engine cowling fills the previously flat/skeletal roof behind
    // the mast and visually explains the paired exhaust outlets. Its lower half is
    // buried in the existing cabin shell, so it adds no false interior obstruction.
    constexpr std::array<FuselageStation,3> engineCowling{{
        {-.30f,2.26f,.58f,.30f},{-1.18f,2.31f,.53f,.35f},
        {-1.76f,2.23f,.40f,.27f}}};
    constexpr int cowlingSides=10;
    const auto cowlingPoint=[](const FuselageStation& station,const float angle) {
        return Vector3(std::cos(angle)*station.radiusX,
            station.centreY+std::sin(angle)*station.radiusY,station.z);
    };
    for(std::size_t band=0;band+1<engineCowling.size();++band)
        for(int slice=0;slice<cowlingSides;++slice) {
            const int next=(slice+1)%cowlingSides;
            const float a=tau*slice/cowlingSides,b=tau*next/cowlingSides;
            smoothQuad(cowlingPoint(engineCowling[band],a),
                       cowlingPoint(engineCowling[band+1],a),
                       cowlingPoint(engineCowling[band+1],b),
                       cowlingPoint(engineCowling[band],b),
                       loftNormal(engineCowling,band,a),
                       loftNormal(engineCowling,band+1,a),
                       loftNormal(engineCowling,band+1,b),
                       loftNormal(engineCowling,band,b),
                       ScaleColor(olive,.985f+.025f*std::sin(
                           (a+b)*.5f)+.012f*static_cast<float>(band)));
        }
    // Framed side access panels give the cowling a believable
    // removable skin rather than a single smooth olive lump.
    for(const float side:{-1.0f,1.0f}) {
        constexpr float panelX=.552f;
        const std::array<Vector3,4> panel{{{side*panelX,2.13f,-1.49f},
            {side*panelX,2.13f,-.64f},{side*panelX,2.43f,-.70f},
            {side*panelX,2.45f,-1.40f}}};
        for(std::size_t edge=0;edge<panel.size();++edge) {
            const Vector3 a=panel[edge],b=panel[(edge+1U)%panel.size()];
            Vector3 perpendicular(0,-(b.Z-a.Z),b.Y-a.Y);perpendicular.Normalize();
            perpendicular*=.010f;
            quad(a-perpendicular,b-perpendicular,b+perpendicular,a+perpendicular,
                 oliveDark);
        }
    }

    // Tail boom, empennage and clearly horizontal tail-rotor disc.
    constexpr int tailSides=12;
    constexpr std::array<FuselageStation,6> tail{{
        {-1.98f,1.50f,.50f,.46f},{-2.42f,1.52f,.45f,.40f},
        {-3.55f,1.62f,.35f,.32f},{-4.55f,1.70f,.29f,.27f},
        {-5.55f,1.81f,.22f,.21f},{-6.85f,1.92f,.14f,.15f}}};
    for(std::size_t band=0;band+1<tail.size();++band)for(int slice=0;slice<tailSides;++slice) {
        const int next=(slice+1)%tailSides;const float a=tau*slice/tailSides,b=tau*next/tailSides;
        const auto point=[](const FuselageStation& station,const float angle){return Vector3(
            std::cos(angle)*station.radiusX,station.centreY+std::sin(angle)*station.radiusY,
            station.z);};
        smoothQuad(point(tail[band],a),point(tail[band+1],a),point(tail[band+1],b),
                   point(tail[band],b),loftNormal(tail,band,a),
                   loftNormal(tail,band+1,a),loftNormal(tail,band+1,b),
                   loftNormal(tail,band,b),ScaleColor(olive,
                       .96f+.025f*std::sin((a+b)*.5f)+.012f*static_cast<float>(band)));
    }
    // Raised dorsal and ventral seams make the boom a manufactured tapered shell,
    // while a buried fairing blends it into the cabin instead of meeting it as a bar.
    ellipsoid({0,1.55f,-1.92f},{.61f,.55f,.68f},5,14,olive);
    for(const float z:{-2.44f,-3.56f,-4.56f,-5.56f}) {
        const float interpolation=std::clamp((-z-2.0f)/4.85f,0.0f,1.0f);
        const float halfWidth=std::lerp(.47f,.15f,interpolation);
        beam({-halfWidth,1.53f+.39f*interpolation,z},
             {halfWidth,1.53f+.39f*interpolation,z},.022f,oliveDark);
    }
    // Closed swept empennage replaces the old two-sided fin and rectangular
    // stabilizer slab. Thickness, taper and separate left/right roots make the
    // tail read as manufactured lifting surfaces rather than intersecting cards.
    const std::array<Vector3,4> finPort{{{-.065f,1.62f,-6.12f},{-.065f,3.18f,-6.62f},
        {-.065f,3.08f,-6.87f},{-.065f,1.80f,-6.98f}}};
    const std::array<Vector3,4> finStarboard{{{.065f,1.62f,-6.12f},{.065f,3.18f,-6.62f},
        {.065f,3.08f,-6.87f},{.065f,1.80f,-6.98f}}};
    quad(finPort[0],finPort[1],finPort[2],finPort[3],oliveLight);
    quad(finStarboard[3],finStarboard[2],finStarboard[1],finStarboard[0],olive);
    for(std::size_t edge=0;edge<finPort.size();++edge) {
        const std::size_t next=(edge+1U)%finPort.size();
        quad(finPort[edge],finStarboard[edge],finStarboard[next],finPort[next],
             edge==2?oliveDark:olive);
    }
    for(const float side:{-1.0f,1.0f}) {
        const std::array<Vector3,4> lower{{{side*.20f,1.70f,-5.22f},
            {side*1.28f,1.70f,-5.38f},{side*1.08f,1.70f,-5.72f},
            {side*.20f,1.70f,-5.63f}}};
        const Vector3 lift(0,.075f,0);
        quad(lower[0],lower[1],lower[2],lower[3],oliveDark);
        quad(lower[3]+lift,lower[2]+lift,lower[1]+lift,lower[0]+lift,oliveLight);
        for(std::size_t edge=0;edge<lower.size();++edge) {
            const std::size_t next=(edge+1U)%lower.size();
            quad(lower[edge],lower[edge]+lift,lower[next]+lift,lower[next],
                 edge==1?oliveDark:olive);
        }
    }
    // The anti-torque rotor turns about the lateral shaft, so its disc belongs in
    // the Y/Z plane. The former X/Y cross was effectively a front-facing propeller:
    // it almost disappeared in the normal broadside view and described the wrong
    // mechanism. A projecting gearbox and two closed tapered spans per blade now
    // give the parked rotor visible chord, thickness, sweep and muted tip markings.
    tube({-.05f,1.92f,-6.84f},{-.43f,1.92f,-6.84f},.090f,metal,Surface::Metal);
    ellipsoid({-.45f,1.92f,-6.84f},{.105f,.16f,.16f},3,10,
              Color(72,76,70),Surface::Metal);
    const Vector3 tailRotorCentre(-.50f,1.92f,-6.84f);
    for(int blade=0;blade<4;++blade) {
        const float angle=.20f+blade*std::numbers::pi_v<float>*.5f;
        const Vector3 radial(0,std::cos(angle),std::sin(angle));
        const Vector3 tangent(0,-radial.Z,radial.Y);
        const auto tailBladePoint=[&](const float distance,const float fraction) {
            return tailRotorCentre+radial*distance+tangent*(.08f*fraction*fraction);
        };
        const auto closedTailBlade=[&](const float innerDistance,const float outerDistance,
            const float innerHalfChord,const float outerHalfChord,
            const float innerFraction,const float outerFraction,
            const Color& upper,const Color& lower) {
            constexpr float halfThickness=.028f;
            const Vector3 depth(halfThickness,0,0);
            const Vector3 inner=tailBladePoint(innerDistance,innerFraction);
            const Vector3 outer=tailBladePoint(outerDistance,outerFraction);
            const std::array<Vector3,4> outline{{inner-tangent*innerHalfChord,
                outer-tangent*outerHalfChord,outer+tangent*outerHalfChord,
                inner+tangent*innerHalfChord}};
            quad(outline[0]+depth,outline[1]+depth,outline[2]+depth,
                 outline[3]+depth,upper);
            quad(outline[3]-depth,outline[2]-depth,outline[1]-depth,
                 outline[0]-depth,lower);
            for(std::size_t edge=0;edge<outline.size();++edge) {
                const std::size_t next=(edge+1U)%outline.size();
                quad(outline[edge]-depth,outline[next]-depth,outline[next]+depth,
                     outline[edge]+depth,Color(39,43,40));
            }
        };
        closedTailBlade(.15f,.69f,.105f,.074f,0,.74f,
            blade%2?Color(59,64,59):Color(70,75,68),
            blade%2?Color(40,44,41):Color(47,51,47));
        closedTailBlade(.69f,.88f,.074f,.052f,.74f,1.0f,
            Color(177,154,62),Color(118,101,43));
    }

    // Ground-following tubular skid rails and four struts. The two rising nose
    // segments create the characteristic curved ski profile without moving the
    // authoritative terrain contact points retained by the layout.
    for(int rail=0;rail<2;++rail) {
        const auto& rear=placement.skidContacts[static_cast<std::size_t>(rail*2)];
        const auto& front=placement.skidContacts[static_cast<std::size_t>(rail*2+1)];
        const float side=rail==0?-1.0f:1.0f;
        const float rearY=rear.groundY-placement.groundY+.11f;
        const float frontY=front.groundY-placement.groundY+.11f;
        const std::array<Vector3,5> railPoints{{
            {side*1.24f,rearY+.025f,-2.06f},
            {side*1.24f,rearY,-1.82f},
            {side*1.24f,frontY,1.55f},
            {side*1.24f,frontY+.11f,1.83f},
            {side*1.24f,frontY+.34f,2.03f}}};
        for(std::size_t segment=0;segment+1<railPoints.size();++segment)
            tube(railPoints[segment],railPoints[segment+1],.072f,metal,Surface::Metal);
        tube({side*.82f,.68f,-1.30f},{side*1.24f,rearY,-1.30f},.062f,metal,Surface::Metal);
        tube({side*.82f,.67f,1.18f},{side*1.24f,frontY,1.18f},.062f,metal,Surface::Metal);
    }

    // Stationary mast, swashplate, hub and four broad blades; there is deliberately
    // no flight state. The earlier square drive column and box hub dominated the
    // broadside silhouette. Concentric tubular shafts and flattened round housings
    // preserve the same parked mechanism while reading as helicopter hardware.
    tube({0,2.22f,-.12f},{0,2.82f,-.12f},.070f,metal,Surface::Metal);
    ellipsoid({0,2.82f,-.12f},{.29f,.045f,.29f},3,16,
              Color(77,82,76),Surface::Metal);
    ellipsoid({0,2.995f,-.12f},{.25f,.095f,.25f},4,12,
              oliveDark,Surface::Metal);
    constexpr int hubSegments=8;
    for(int segment=0;segment<hubSegments;++segment) {
        const float first=tau*segment/hubSegments,next=tau*(segment+1)/hubSegments;
        const Vector3 a(std::cos(first)*.34f,2.90f,std::sin(first)*.34f-.12f);
        const Vector3 b(std::cos(next)*.34f,2.90f,std::sin(next)*.34f-.12f);
        beam(a,b,.065f,metal,Surface::Metal);
        if(segment%2==0)
            beam(a,{a.X*.74f,3.02f,(a.Z+.12f)*.74f-.12f},.045f,oliveDark);
    }
    const Vector3 rotorCentre(0,3.02f,-.12f);
    // The fixed parked head still exposes real scale cues: a projecting mast cap,
    // four radial grip housings and paired-height pitch links from the lower hub.
    // These are purely static mechanical detail and do not imply a flight state.
    tube({0,2.74f,-.12f},{0,3.17f,-.12f},.075f,metal,Surface::Metal);
    for(int blade=0;blade<4;++blade) {
        const float angle=.50f+blade*std::numbers::pi_v<float>*.5f;
        const Vector3 radial(std::cos(angle),0,std::sin(angle));
        const Vector3 perpendicular(-radial.Z,0,radial.X);
        beam(rotorCentre+radial*.18f,rotorCentre+radial*.53f,.13f,
             Color(72,76,70),Surface::Metal);
        tube({radial.X*.24f,2.82f,radial.Z*.24f-.12f},
             {radial.X*.47f,3.045f,radial.Z*.47f-.12f},.020f,
             Color(104,108,98),Surface::Metal);
        const auto bladePoint=[&](const float distance,const float fraction) {
            const float sweep=.055f*fraction+.055f*fraction*fraction;
            const float droop=.18f*fraction*fraction;
            return rotorCentre+radial*distance+perpendicular*sweep
                -Vector3(0,droop,0);
        };
        const auto closedBlade=[&](const float innerDistance,const float outerDistance,
            const float innerHalf,const float outerHalf,const float innerFraction,
            const float outerFraction,const Color& upper,const Color& lower) {
            constexpr float halfDepth=.040f;
            const Vector3 inner=bladePoint(innerDistance,innerFraction);
            const Vector3 outer=bladePoint(outerDistance,outerFraction);
            const Vector3 lift(0,halfDepth,0);
            // A four-point cambered section replaces the old rectangular slab.
            // Leading and trailing edges close to a line while the maximum depth
            // sits ahead of mid-chord, so an eye-level broadside reads as a thin
            // parked airfoil rather than a uniformly black bar.
            const std::array<Vector3,4> innerSection{{
                inner-perpendicular*innerHalf,
                inner-perpendicular*(innerHalf*.18f)+lift,
                inner+perpendicular*innerHalf-Vector3(0,.004f,0),
                inner-perpendicular*(innerHalf*.18f)-lift}};
            const std::array<Vector3,4> outerSection{{
                outer-perpendicular*outerHalf,
                outer-perpendicular*(outerHalf*.18f)+lift,
                outer+perpendicular*outerHalf-Vector3(0,.004f,0),
                outer-perpendicular*(outerHalf*.18f)-lift}};
            quad(innerSection[0],outerSection[0],outerSection[1],innerSection[1],
                 ScaleColor(upper,.94f));
            quad(innerSection[1],outerSection[1],outerSection[2],innerSection[2],upper);
            quad(innerSection[2],outerSection[2],outerSection[3],innerSection[3],lower);
            quad(innerSection[3],outerSection[3],outerSection[0],innerSection[0],
                 ScaleColor(lower,1.12f));
            quad(innerSection[3],innerSection[2],innerSection[1],innerSection[0],
                 Color(61,65,60));
            quad(outerSection[0],outerSection[1],outerSection[2],outerSection[3],
                 Color(67,70,63));
        };
        const float tipStart=HelicopterLayout::MainRotorRadius-.28f;
        const float tipFraction=(tipStart-.48f)/(HelicopterLayout::MainRotorRadius-.48f);
        constexpr float firstJoint=2.00f;
        constexpr float secondJoint=3.70f;
        const float firstFraction=(firstJoint-.48f)/(HelicopterLayout::MainRotorRadius-.48f);
        const float secondFraction=(secondJoint-.48f)/(HelicopterLayout::MainRotorRadius-.48f);
        closedBlade(.48f,firstJoint,.175f,.160f,0,firstFraction,
            blade%2?Color(88,92,84):Color(101,105,95),
            blade%2?Color(48,52,48):Color(55,59,54));
        closedBlade(firstJoint,secondJoint,.160f,.138f,firstFraction,secondFraction,
            blade%2?Color(88,92,84):Color(101,105,95),
            blade%2?Color(48,52,48):Color(55,59,54));
        closedBlade(secondJoint,tipStart,.138f,.116f,secondFraction,tipFraction,
            blade%2?Color(88,92,84):Color(101,105,95),
            blade%2?Color(48,52,48):Color(55,59,54));
        // Muted yellow parked-blade tips improve scale/readability without suggesting
        // illuminated or moving rotors.
        closedBlade(tipStart,HelicopterLayout::MainRotorRadius,.110f,.098f,
            tipFraction,1.0f,Color(177,154,62),Color(119,101,42));
    }
    // Exhausts, navigation lights and a small belly lamp improve exterior scale cues.
    beam({-.36f,2.18f,-1.54f},{-.48f,2.25f,-1.98f},.15f,Color(55,55,52),Surface::Metal);
    beam({ .36f,2.18f,-1.54f},{ .48f,2.25f,-1.98f},.15f,Color(55,55,52),Surface::Metal);
    // Paired dark intake lips, access-panel seams and fastener heads add real
    // maintenance anatomy without another material batch.
    for(const float side:{-1.0f,1.0f}) {
        ellipsoid({side*.40f,2.35f,-.75f},{.20f,.105f,.30f},3,10,
                  Color(35,39,36),Surface::Rubber);
        beam({side*.76f,1.02f,-.26f},{side*.76f,1.93f,-.26f},.022f,oliveDark);
        for(const float y:{.72f,1.15f,1.58f})
            ellipsoid({side*.985f,y,-.24f},{.018f,.018f,.018f},2,6,
                      Color(178,179,153),Surface::Metal);
    }
    box({-1.10f,1.50f,-.20f},{-1.04f,1.61f,-.08f},Color(188,54,43),Surface::Emissive);
    box({ 1.04f,1.50f,-.20f},{ 1.10f,1.61f,-.08f},Color(55,162,91),Surface::Emissive);
    box({-.08f,.30f,.92f},{.08f,.39f,1.08f},Color(221,211,161),Surface::Emissive);

    const auto totalVertices=mesh.size()+glassMesh.size();
    if(mesh.empty()||glassMesh.empty()||totalVertices>19000)
        throw std::runtime_error("Helicopter mesh exceeds budget: "
            +std::to_string(totalVertices)+" vertices");
    Vector3 low(std::numeric_limits<float>::max()),high(std::numeric_limits<float>::lowest());
    for(const auto& vertex:mesh){low=Vector3::Min(low,vertex.Position);high=Vector3::Max(high,vertex.Position);}
    for(const auto& vertex:glassMesh){low=Vector3::Min(low,vertex.Position);high=Vector3::Max(high,vertex.Position);}
    bounds_=BoundingBox(low,high);triangles_=static_cast<int>(mesh.size()/3);
    glassTriangles_=static_cast<int>(glassMesh.size()/3);triangles_+=glassTriangles_;
    vertices_=std::make_unique<VertexBuffer>(device,
        VertexPositionColorTexture::getVertexDeclarationStatic(),static_cast<int>(mesh.size()),
        BufferUsage::WriteOnly);
    vertices_->SetData(mesh.data(),static_cast<int>(mesh.size()));
    glassVertices_=std::make_unique<VertexBuffer>(device,
        VertexPositionColorTexture::getVertexDeclarationStatic(),static_cast<int>(glassMesh.size()),
        BufferUsage::WriteOnly);
    glassVertices_->SetData(glassMesh.data(),static_cast<int>(glassMesh.size()));
    effect_->setVertexColorEnabledProperty(true);effect_->setLightingEnabledProperty(false);
    effect_->setTextureEnabledProperty(true);effect_->setTextureProperty(materialTexture_.get());
    effect_->setFogEnabledProperty(true);
    effect_->setFogColorProperty(Vector3(environment::FogColorRed,environment::FogColorGreen,
                                         environment::FogColorBlue));
    effect_->setFogStartProperty(190.0f);effect_->setFogEndProperty(330.0f);
    const std::string message="island-simulator: grounded helicopter original procedural shell, open pilot/rear doors, interior and stationary rotors; triangles="
        +std::to_string(triangles_);
    std::cout<<message<<'\n';MirrorRuntimeDiagnosticToPlatformLog(message);
    const std::string detail="island-simulator: helicopter uses a smoothly tapered thirty-two-sided seventeen-station cockpit canopy, an articulated eight-link rotor hub, tubular rising-tip skids, and a tapered tailplane";
    std::cout<<detail<<'\n';MirrorRuntimeDiagnosticToPlatformLog(detail);
    const std::string material="island-simulator: helicopter uses a generated 128-square atlas for worn paint, brushed metal, woven seats, and fine rubber";
    std::cout<<material<<'\n';MirrorRuntimeDiagnosticToPlatformLog(material);
    const std::string doors="island-simulator: helicopter carries four outward-open framed door panels with glazing and handles";
    std::cout<<doors<<'\n';MirrorRuntimeDiagnosticToPlatformLog(doors);
    const std::string doorHinges="island-simulator: helicopter open doors use two visible hinge barrels at every jamb";
    std::cout<<doorHinges<<'\n';MirrorRuntimeDiagnosticToPlatformLog(doorHinges);
    const std::string doorwayShell="island-simulator: helicopter doorway shell uses rounded jambs and low-profile oval roof shoulders around all four open entries";
    std::cout<<doorwayShell<<'\n';MirrorRuntimeDiagnosticToPlatformLog(doorwayShell);
    const std::string rotor="island-simulator: helicopter main rotor uses four closed tapered blades with visible leading, trailing, and tip edges";
    std::cout<<rotor<<'\n';MirrorRuntimeDiagnosticToPlatformLog(rotor);
    const std::string rotorDetail="island-simulator: helicopter rotor head adds four blade grips, pitch links, a mast cap, and muted yellow tip bands";
    std::cout<<rotorDetail<<'\n';MirrorRuntimeDiagnosticToPlatformLog(rotorDetail);
    const std::string cowling="island-simulator: helicopter raised tapered engine cowling feeds two rear exhaust outlets behind the rotor mast";
    std::cout<<cowling<<'\n';MirrorRuntimeDiagnosticToPlatformLog(cowling);
    const std::string cockpit="island-simulator: helicopter cockpit uses four round gauges, paired articulated cyclic and collective controls, and four pedals";
    std::cout<<cockpit<<'\n';MirrorRuntimeDiagnosticToPlatformLog(cockpit);
    const std::string instruments="island-simulator: helicopter instruments use recessed ticked dials, one artificial horizon, needle hubs, and framed multi-line radio displays";
    std::cout<<instruments<<'\n';MirrorRuntimeDiagnosticToPlatformLog(instruments);
    const std::string cabin="island-simulator: helicopter open cabin uses restrained indirect light, a tapered instrument binnacle, and twin radio displays";
    std::cout<<cabin<<'\n';MirrorRuntimeDiagnosticToPlatformLog(cabin);
    const std::string anatomy="island-simulator: helicopter realism pass adds a rounded chin and tail-root fairing, twelve-sided six-station tail boom, maintenance seams, engine intakes, shaped cushions, harnesses, centre pedestal, and overhead console";
    std::cout<<anatomy<<'\n';MirrorRuntimeDiagnosticToPlatformLog(anatomy);
    const std::string smoothSurfaces="island-simulator: helicopter curved shell uses analytic per-vertex lighting across the canopy, cowling, tail boom, fairings, and cushions without added geometry";
    std::cout<<smoothSurfaces<<'\n';MirrorRuntimeDiagnosticToPlatformLog(smoothSurfaces);
    const std::string bubble="island-simulator: helicopter cockpit uses a fuller eleven-station forward bubble, two compact nose rings, and a progressively wrapped lower windscreen";
    std::cout<<bubble<<'\n';MirrorRuntimeDiagnosticToPlatformLog(bubble);
    const std::string glazingDetail="island-simulator: helicopter divided windscreen carries paired articulated parked wipers and restrained translucent sky glints";
    std::cout<<glazingDetail<<'\n';MirrorRuntimeDiagnosticToPlatformLog(glazingDetail);
    const std::string bladeShape="island-simulator: parked main rotor uses three-span quadratic blade droop, progressive chord taper, and restrained tip sweep";
    std::cout<<bladeShape<<'\n';MirrorRuntimeDiagnosticToPlatformLog(bladeShape);
    const std::string bladeSection="island-simulator: parked main rotor uses closed cambered airfoil sections with fine leading and trailing edges";
    std::cout<<bladeSection<<'\n';MirrorRuntimeDiagnosticToPlatformLog(bladeSection);
    const std::string tailRotorShape="island-simulator: lateral tail-rotor shaft carries four closed swept blades with visible chord, thickness, and muted tip bands";
    std::cout<<tailRotorShape<<'\n';MirrorRuntimeDiagnosticToPlatformLog(tailRotorShape);
    const std::string empennage="island-simulator: helicopter empennage uses a closed swept vertical fin and separate tapered horizontal stabilizers";
    std::cout<<empennage<<'\n';MirrorRuntimeDiagnosticToPlatformLog(empennage);
    const std::string entrySteps="island-simulator: helicopter four open doorways use twin-rail tubular anti-slip entry steps instead of solid slabs";
    std::cout<<entrySteps<<'\n';MirrorRuntimeDiagnosticToPlatformLog(entrySteps);
    const std::string stepProfile="island-simulator: helicopter entry steps use two spaced cross-braces per doorway instead of repeated ladder rungs";
    std::cout<<stepProfile<<'\n';MirrorRuntimeDiagnosticToPlatformLog(stepProfile);
    const std::string seating="island-simulator: helicopter cabin uses rounded closed pilot cushions on exposed metal frames and a seam-divided rear bench";
    std::cout<<seating<<'\n';MirrorRuntimeDiagnosticToPlatformLog(seating);
    const std::string rotorMast="island-simulator: helicopter rotor mast uses concentric tubular shafts, a flattened round swashplate, and a rounded gearbox housing";
    std::cout<<rotorMast<<'\n';MirrorRuntimeDiagnosticToPlatformLog(rotorMast);
    const std::string lowerShell="island-simulator: helicopter open cabin is carried by a rounded belly and continuous lower sill fairings";
    std::cout<<lowerShell<<'\n';MirrorRuntimeDiagnosticToPlatformLog(lowerShell);
}

HelicopterRenderer::~HelicopterRenderer()=default;

void HelicopterRenderer::Draw(const Matrix& view,const Matrix& projection,const Vector3& camera) {
    drawCalls_=0;
    if((centre_-camera).LengthSquared()>360.0f*360.0f
        ||BoundingFrustum(view*projection).Contains(bounds_)==ContainmentType::Disjoint)return;
    device_.setRasterizerStateProperty(RasterizerState::CullNone);
    device_.setBlendStateProperty(BlendState::Opaque);
    device_.setDepthStencilStateProperty(DepthStencilState::Default);
    device_.getSamplerStatesProperty()[0]=SamplerState::LinearClamp;
    device_.SetVertexBuffer(vertices_.get());device_.setIndicesProperty(nullptr);
    effect_->setWorldProperty(Matrix::getIdentityProperty());effect_->setViewProperty(view);
    effect_->setProjectionProperty(projection);effect_->Apply();
    device_.DrawPrimitives(PrimitiveType::TriangleList,0,triangles_-glassTriangles_);
    device_.setBlendStateProperty(BlendState::AlphaBlend);
    device_.setDepthStencilStateProperty(DepthStencilState::DepthRead);
    device_.setRasterizerStateProperty(RasterizerState::CullCounterClockwise);
    device_.SetVertexBuffer(glassVertices_.get());effect_->Apply();
    device_.DrawPrimitives(PrimitiveType::TriangleList,0,glassTriangles_);drawCalls_=2;
}
}
