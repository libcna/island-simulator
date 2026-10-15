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
constexpr std::array<FuselageStation,15> Fuselage{{
    {-2.10f,1.35f,.68f,.64f},{-1.78f,1.38f,.94f,.82f},
    {-.30f,1.43f,1.07f,.98f},{.18f,1.44f,1.08f,.99f},
    {.62f,1.45f,1.075f,.985f},{1.02f,1.45f,1.05f,.97f},
    {1.34f,1.43f,1.00f,.94f},{1.62f,1.40f,.94f,.88f},
    {1.86f,1.36f,.84f,.81f},{2.08f,1.32f,.73f,.73f},
    {2.30f,1.28f,.61f,.64f},{2.48f,1.24f,.49f,.55f},
    {2.64f,1.21f,.35f,.44f},{2.76f,1.19f,.22f,.33f},
    {2.84f,1.17f,.10f,.20f}}};

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
    const Color olive(108,121,82),oliveLight(149,155,105),oliveDark(56,67,49);
    const Color glass(60,105,120,148),metal(91,96,92);
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
            Color color=olive;
            Surface surface=Surface::Paint;
            if(midZ>1.50f&&std::sin(mid)>-.08f) {
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
    // Seal the pointed nose. Without this small fan the rear-cabin sightline could
    // expose a bright terrain pinhole through the final fuselage ring.
    const auto fuselagePoint=[](const FuselageStation& station,const float angle) {
        return Vector3(std::cos(angle)*station.radiusX,
            station.centreY+std::sin(angle)*station.radiusY,station.z);
    };
    const Vector3 noseTip(0,1.16f,2.92f);
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
    // A narrow centre mullion makes the faceted windshield read as glazing rather
    // than an open hole when the player stands in the rear cabin.
    beam({0,1.11f,2.67f},{0,2.12f,1.70f},.075f,oliveDark);
    for(const float side:{-1.0f,1.0f})
        beam({side*.57f,1.08f,2.45f},{side*.30f,2.08f,2.06f},.052f,oliveDark);
    beam({-.31f,2.08f,2.06f},{.31f,2.08f,2.06f},.050f,oliveDark);
    // A rounded chin fairing and slim lower windscreen rail keep the otherwise
    // transparent nose from collapsing into a pointed glass bubble at eye level.
    ellipsoid({0,.87f,2.36f},{.57f,.23f,.36f},4,16,olive);
    beam({-.59f,1.03f,2.47f},{.59f,1.03f,2.47f},.055f,oliveDark);

    // Flat walkable floor, two-stage door sills and visible frames match collision support.
    box({-.78f,.405f,-1.78f},{.78f,.455f,1.48f},interior,Surface::Cabin);
    for(const float side:{-1.0f,1.0f})for(const auto& door:
        {std::pair{-1.70f,-.30f},std::pair{.18f,1.48f}}) {
        const float lowX=side>0?.78f:-1.23f,highX=side>0?1.23f:-.78f;
        box({lowX,.205f,door.first},{highX,.245f,door.second},oliveDark);
        const float outerLow=side>0?1.23f:-1.58f,outerHigh=side>0?1.58f:-1.23f;
        box({outerLow,.025f,door.first+.08f},{outerHigh,.065f,door.second-.08f},metal,Surface::Metal);
        beam({side*.93f,.45f,door.first},{side*.93f,2.17f,door.first},.075f,oliveLight);
        beam({side*.93f,.45f,door.second},{side*.93f,2.17f,door.second},.075f,oliveLight);
        beam({side*.93f,2.15f,door.first},{side*.93f,2.15f,door.second},.075f,oliveLight);
    }

    // Four visibly open doors explain the otherwise skeletal side openings. Pilot
    // panels hinge forward and rear panels hinge aft, leaving the tested perpendicular
    // entry corridors unobstructed. Glass shares the existing depth-read second draw.
    const auto openDoor=[&](const float side,const float hingeZ,const float farZ) {
        const Vector3 hingeLow(side*.99f,.49f,hingeZ),farLow(side*1.62f,.49f,farZ);
        const auto point=[&](const float across,const float y) {
            return Vector3::Lerp(hingeLow,farLow,across)+Vector3(0,y-.49f,0);
        };
        // A subtly raked belt and roof edge avoids a construction-site rectangle.
        quad(point(0,.49f),point(1,.49f),point(1,.96f),point(0,1.01f),olive);
        quad(point(0,1.91f),point(.82f,1.98f),point(1,2.04f),point(0,2.08f),oliveLight);
        quad(point(.12f,1.03f),point(.88f,1.00f),point(.80f,1.82f),point(.18f,1.88f),
             glass,Surface::Glass);
        beam(point(0,.49f),point(0,2.08f),.065f,oliveDark);
        beam(point(1,.49f),point(1,1.82f),.065f,oliveDark);
        beam(point(1,1.82f),point(.82f,1.98f),.065f,oliveDark);
        beam(point(.82f,1.98f),point(0,1.91f),.065f,oliveDark);
        beam(point(0,.49f),point(1,.49f),.065f,oliveDark);
        beam(point(.10f,1.01f),point(.90f,.98f),.052f,oliveDark);
        beam(point(.70f,1.12f),point(.87f,1.12f),.034f,metal,Surface::Metal);
    };
    for(const float side:{-1.0f,1.0f}) {
        openDoor(side,1.46f,2.35f);
        openDoor(side,-1.68f,-2.55f);
    }

    // Two pilot seats, rear bench and articulated primary flight controls. Curved
    // cyclic shafts and separate collectives read as controls rather than seat braces.
    for(const float x:{-.46f,.46f}) {
        box({x-.27f,.46f,.08f},{x+.27f,.76f,.66f},seat,Surface::Upholstery);
        box({x-.28f,.72f,.08f},{x+.28f,1.43f,.25f},seat,Surface::Upholstery);
        ellipsoid({x,.77f,.43f},{.29f,.13f,.31f},4,10,
                  Color(72,76,64),Surface::Upholstery);
        ellipsoid({x,1.42f,.17f},{.20f,.16f,.12f},4,10,
                  Color(65,69,59),Surface::Upholstery);
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
    box({-.72f,.48f,-1.72f},{.72f,.79f,-1.43f},seat,Surface::Upholstery);
    box({-.72f,.75f,-1.70f},{.72f,1.38f,-1.52f},seat,Surface::Upholstery);
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
    // The central pedestal and glareshield break up the panel into real cockpit volumes.
    box({-.17f,.47f,.87f},{.17f,1.02f,1.28f},Color(52,59,53),Surface::Cabin);
    quad({-.73f,1.38f,1.33f},{.73f,1.38f,1.33f},{.61f,1.48f,1.15f},
         {-.61f,1.48f,1.15f},Color(46,52,47),Surface::Cabin);
    box({-.29f,1.91f,.42f},{.29f,1.99f,1.03f},Color(62,69,61),Surface::Cabin);
    // Recessed round gauges, contrasting bezels and individual needles replace the
    // former grid of twelve coloured squares. Their layered planes face the cabin.
    constexpr std::array<float,4> gaugeX{{-.47f,-.16f,.16f,.47f}};
    for(std::size_t gauge=0;gauge<gaugeX.size();++gauge) {
        const Vector3 bezel(gaugeX[gauge],1.27f,1.289f);
        disc(bezel,.095f,16,Color(116,119,111),Surface::Metal);
        disc(bezel-Vector3(0,0,.006f),.073f,16,Color(14,17,15),Surface::Rubber);
        const float angle=-1.03f+static_cast<float>(gauge)*.47f;
        const Vector3 centre=bezel-Vector3(0,0,.010f);
        const Vector3 along(std::cos(angle)*.058f,std::sin(angle)*.058f,0);
        const Vector3 across(-std::sin(angle)*.010f,std::cos(angle)*.010f,0);
        triangle(centre-across,centre+along,centre+across,
                 gauge==0?Color(210,91,66):Color(218,207,168),Surface::Emissive);
    }
    // Two recessed radio/navigation displays give the lower panel a readable centre
    // stack instead of an undifferentiated row of coloured squares.
    box({-.255f,1.055f,1.276f},{-.025f,1.145f,1.296f},
        Color(56,126,105),Surface::Emissive);
    box({.025f,1.055f,1.276f},{.255f,1.145f,1.296f},
        Color(86,118,137),Surface::Emissive);
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
                       slice%3==0?oliveLight:olive);
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
                   loftNormal(tail,band,b),(slice%3==0)?oliveLight:olive);
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
    // Vertical fin and horizontal stabilizer.
    quad({-.06f,1.82f,-6.35f},{-.06f,3.18f,-6.72f},{-.06f,1.93f,-6.94f},
         {-.06f,1.62f,-6.18f},oliveLight);
    quad({.06f,1.62f,-6.18f},{.06f,1.93f,-6.94f},{.06f,3.18f,-6.72f},
         {.06f,1.82f,-6.35f},olive);
    // A thin tapered stabilizer replaces the old rectangular slab. The swept tips
    // remain broad enough to read from the authored exterior camera.
    const std::array<Vector3,4> stabilizer{{
        {-1.22f,1.73f,-5.57f},{-1.22f,1.73f,-5.30f},
        { 1.22f,1.73f,-5.30f},{ 1.22f,1.73f,-5.57f}}};
    const Vector3 stabilizerLift(0,.075f,0);
    quad(stabilizer[0],stabilizer[1],stabilizer[2],stabilizer[3],oliveDark);
    quad(stabilizer[3]+stabilizerLift,stabilizer[2]+stabilizerLift,
         stabilizer[1]+stabilizerLift,stabilizer[0]+stabilizerLift,oliveLight);
    for(int side=0;side<4;++side) {
        const int next=(side+1)%4;
        quad(stabilizer[static_cast<std::size_t>(side)],
             stabilizer[static_cast<std::size_t>(side)]+stabilizerLift,
             stabilizer[static_cast<std::size_t>(next)]+stabilizerLift,
             stabilizer[static_cast<std::size_t>(next)],olive);
    }
    beam({.18f,1.92f,-6.84f},{.30f,1.92f,-6.84f},.10f,metal,Surface::Metal);
    for(int blade=0;blade<4;++blade) {
        const float angle=blade*std::numbers::pi_v<float>*.5f;
        const Vector3 radial(std::cos(angle),std::sin(angle),0);
        beam({.31f,1.92f,-6.84f},{.31f+radial.X*.78f,1.92f+radial.Y*.78f,-6.84f},
             .075f,Color(49,52,48));
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

    // Stationary mast, hub and four broad blades; there is deliberately no flight state.
    beam({0,2.22f,-.12f},{0,2.98f,-.12f},.17f,metal,Surface::Metal);
    box({-.25f,2.91f,-.37f},{.25f,3.08f,.13f},oliveDark);
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
        const float angle=.18f+blade*std::numbers::pi_v<float>*.5f;
        const Vector3 radial(std::cos(angle),0,std::sin(angle));
        const Vector3 perpendicular(-radial.Z,0,radial.X);
        beam(rotorCentre+radial*.18f,rotorCentre+radial*.53f,.13f,
             Color(72,76,70),Surface::Metal);
        tube({radial.X*.24f,2.82f,radial.Z*.24f-.12f},
             {radial.X*.47f,3.045f,radial.Z*.47f-.12f},.020f,
             Color(104,108,98),Surface::Metal);
        const auto bladePoint=[&](const float distance,const float fraction) {
            return rotorCentre+radial*distance+perpendicular*(.055f*fraction)
                -Vector3(0,.012f*fraction,0);
        };
        const auto closedBlade=[&](const float innerDistance,const float outerDistance,
            const float innerHalf,const float outerHalf,const float innerFraction,
            const float outerFraction,const Color& upper,const Color& lower) {
            constexpr float halfDepth=.034f;
            const Vector3 lift(0,halfDepth,0);
            const Vector3 inner=bladePoint(innerDistance,innerFraction);
            const Vector3 outer=bladePoint(outerDistance,outerFraction);
            const std::array<Vector3,4> outline{{inner-perpendicular*innerHalf,
                outer-perpendicular*outerHalf,outer+perpendicular*outerHalf,
                inner+perpendicular*innerHalf}};
            quad(outline[0]+lift,outline[1]+lift,outline[2]+lift,outline[3]+lift,upper);
            quad(outline[3]-lift,outline[2]-lift,outline[1]-lift,outline[0]-lift,lower);
            for(std::size_t edge=0;edge<outline.size();++edge) {
                const std::size_t next=(edge+1U)%outline.size();
                quad(outline[edge]-lift,outline[next]-lift,outline[next]+lift,
                     outline[edge]+lift,edge==1U?Color(31,34,32):Color(43,47,43));
            }
        };
        const float tipStart=HelicopterLayout::MainRotorRadius-.28f;
        const float tipFraction=(tipStart-.48f)/(HelicopterLayout::MainRotorRadius-.48f);
        closedBlade(.48f,tipStart,.155f,.110f,0,tipFraction,
            blade%2?Color(62,67,60):Color(76,82,73),
            blade%2?Color(42,46,42):Color(50,55,49));
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

    if(mesh.empty()||glassMesh.empty()||mesh.size()+glassMesh.size()>18000)
        throw std::runtime_error("Helicopter mesh exceeds budget");
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
    const std::string detail="island-simulator: helicopter uses a smoothly tapered thirty-two-sided cockpit canopy, an articulated eight-link rotor hub, tubular rising-tip skids, and a tapered tailplane";
    std::cout<<detail<<'\n';MirrorRuntimeDiagnosticToPlatformLog(detail);
    const std::string material="island-simulator: helicopter uses a generated 128-square atlas for worn paint, brushed metal, woven seats, and fine rubber";
    std::cout<<material<<'\n';MirrorRuntimeDiagnosticToPlatformLog(material);
    const std::string doors="island-simulator: helicopter carries four outward-open framed door panels with glazing and handles";
    std::cout<<doors<<'\n';MirrorRuntimeDiagnosticToPlatformLog(doors);
    const std::string rotor="island-simulator: helicopter main rotor uses four closed tapered blades with visible leading, trailing, and tip edges";
    std::cout<<rotor<<'\n';MirrorRuntimeDiagnosticToPlatformLog(rotor);
    const std::string rotorDetail="island-simulator: helicopter rotor head adds four blade grips, pitch links, a mast cap, and muted yellow tip bands";
    std::cout<<rotorDetail<<'\n';MirrorRuntimeDiagnosticToPlatformLog(rotorDetail);
    const std::string cowling="island-simulator: helicopter raised tapered engine cowling feeds two rear exhaust outlets behind the rotor mast";
    std::cout<<cowling<<'\n';MirrorRuntimeDiagnosticToPlatformLog(cowling);
    const std::string cockpit="island-simulator: helicopter cockpit uses four round gauges, paired articulated cyclic and collective controls, and four pedals";
    std::cout<<cockpit<<'\n';MirrorRuntimeDiagnosticToPlatformLog(cockpit);
    const std::string cabin="island-simulator: helicopter open cabin uses restrained indirect light, a tapered instrument binnacle, and twin radio displays";
    std::cout<<cabin<<'\n';MirrorRuntimeDiagnosticToPlatformLog(cabin);
    const std::string anatomy="island-simulator: helicopter realism pass adds a rounded chin and tail-root fairing, twelve-sided six-station tail boom, maintenance seams, engine intakes, shaped cushions, harnesses, centre pedestal, and overhead console";
    std::cout<<anatomy<<'\n';MirrorRuntimeDiagnosticToPlatformLog(anatomy);
    const std::string smoothSurfaces="island-simulator: helicopter curved shell uses analytic per-vertex lighting across the canopy, cowling, tail boom, fairings, and cushions without added geometry";
    std::cout<<smoothSurfaces<<'\n';MirrorRuntimeDiagnosticToPlatformLog(smoothSurfaces);
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
    device_.SetVertexBuffer(glassVertices_.get());effect_->Apply();
    device_.DrawPrimitives(PrimitiveType::TriangleList,0,glassTriangles_);drawCalls_=2;
}
}
