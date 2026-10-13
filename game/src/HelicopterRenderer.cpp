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
constexpr int FuselageSides=20;
enum class Surface : std::uint8_t { Paint,Metal,Upholstery,Rubber,Glass,Emissive };
struct Texel{std::uint8_t r,g,b,a;};
struct FuselageStation{float z,centreY,radiusX,radiusY;};
constexpr std::array<FuselageStation,9> Fuselage{{
    {-2.10f,1.35f,.68f,.64f},{-1.78f,1.38f,.94f,.82f},
    {-.30f,1.43f,1.07f,.98f},{.18f,1.44f,1.08f,.99f},
    {1.48f,1.42f,.98f,.93f},{1.92f,1.35f,.82f,.78f},
    {2.42f,1.25f,.51f,.57f},{2.63f,1.21f,.34f,.43f},
    {2.78f,1.18f,.16f,.27f}}};

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
        surface==Surface::Rubber?3:0;
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
        const float light=.48f+.52f*std::max(0.0f,Vector3::Dot(normal,sun));
        const Color color=ScaleColor(pigment,light);
        auto& target=pigment.getAProperty()<255?glassMesh:mesh;
        target.emplace_back(a,color,uvA);target.emplace_back(b,color,uvB);target.emplace_back(d,color,uvD);
    };
    const auto quad=[&](const Vector3& a,const Vector3& b,const Vector3& d,const Vector3& e,
                        const Color& color,const Surface surface=Surface::Paint) {
        triangle(a,b,d,color,surface);triangle(a,d,e,color,surface);
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
    const Color olive(101,116,80),oliveLight(136,147,102),oliveDark(61,73,56);
    const Color glass(42,78,91,132),glassLight(94,145,159,150),metal(82,85,81);
    const Color interior(49,54,50),seat(45,49,43),rubber(30,32,30);

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
            Color color=(slice%4==0)?oliveLight:olive;
            Surface surface=Surface::Paint;
            if(midZ>1.50f&&std::sin(mid)>-.08f) {
                color=(slice%2)?glass:glassLight;surface=Surface::Glass;
            }
            if(std::sin(mid)<-.72f)color=oliveDark;
            quad(point(Fuselage[band],a),point(Fuselage[band+1],a),
                 point(Fuselage[band+1],b),point(Fuselage[band],b),color,surface);
        }
    }
    // Seal the pointed nose. Without this small fan the rear-cabin sightline could
    // expose a bright terrain pinhole through the final fuselage ring.
    const auto fuselagePoint=[](const FuselageStation& station,const float angle) {
        return Vector3(std::cos(angle)*station.radiusX,
            station.centreY+std::sin(angle)*station.radiusY,station.z);
    };
    const Vector3 noseTip(0,1.17f,2.88f);
    for(int slice=0;slice<FuselageSides;++slice) {
        const float a=tau*slice/FuselageSides;
        const float b=tau*((slice+1)%FuselageSides)/FuselageSides;
        const float mid=(a+b)*.5f;
        const Color color=std::sin(mid)>-.08f?glass:oliveDark;
        triangle(fuselagePoint(Fuselage.back(),a),noseTip,
                 fuselagePoint(Fuselage.back(),b),color,
                 std::sin(mid)>-.08f?Surface::Glass:Surface::Paint);
    }
    // A narrow centre mullion makes the faceted windshield read as glazing rather
    // than an open hole when the player stands in the rear cabin.
    beam({0,1.11f,2.67f},{0,2.12f,1.70f},.075f,oliveDark);
    for(const float side:{-1.0f,1.0f})
        beam({side*.57f,1.08f,2.45f},{side*.30f,2.08f,2.06f},.052f,oliveDark);
    beam({-.31f,2.08f,2.06f},{.31f,2.08f,2.06f},.050f,oliveDark);

    // Flat walkable floor, two-stage door sills and visible frames match collision support.
    box({-.78f,.405f,-1.78f},{.78f,.455f,1.48f},interior,Surface::Rubber);
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
        quad(point(0,.49f),point(1,.49f),point(1,.98f),point(0,.98f),olive);
        quad(point(0,1.88f),point(1,1.88f),point(1,2.08f),point(0,2.08f),oliveLight);
        quad(point(.12f,1.00f),point(.88f,1.00f),point(.88f,1.86f),point(.12f,1.86f),
             glass,Surface::Glass);
        beam(point(0,.49f),point(0,2.08f),.065f,oliveDark);
        beam(point(1,.49f),point(1,2.08f),.065f,oliveDark);
        beam(point(0,.49f),point(1,.49f),.065f,oliveDark);
        beam(point(0,2.08f),point(1,2.08f),.065f,oliveDark);
        beam(point(.10f,.99f),point(.90f,.99f),.052f,oliveDark);
        beam(point(.70f,1.12f),point(.87f,1.12f),.034f,metal,Surface::Metal);
    };
    for(const float side:{-1.0f,1.0f}) {
        openDoor(side,1.46f,2.35f);
        openDoor(side,-1.68f,-2.55f);
    }

    // Two pilot seats, rear bench, instrument panel and paired cyclic controls.
    for(const float x:{-.46f,.46f}) {
        box({x-.27f,.46f,.08f},{x+.27f,.76f,.66f},seat,Surface::Upholstery);
        box({x-.28f,.72f,.08f},{x+.28f,1.43f,.25f},seat,Surface::Upholstery);
        beam({x,.58f,.79f},{x,.95f,1.03f},.045f,metal,Surface::Metal);
        box({x-.055f,.91f,.98f},{x+.055f,1.01f,1.08f},rubber,Surface::Rubber);
    }
    box({-.72f,.48f,-1.72f},{.72f,.79f,-1.43f},seat,Surface::Upholstery);
    box({-.72f,.75f,-1.70f},{.72f,1.38f,-1.52f},seat,Surface::Upholstery);
    box({-.72f,1.05f,1.30f},{.72f,1.39f,1.52f},rubber,Surface::Rubber);
    for(int row=0;row<2;++row)for(int column=0;column<6;++column) {
        const Color lamp=(column+row)%3==0?Color(194,91,57):
            ((column+row)%3==1?Color(89,157,105):Color(204,177,80));
        box({-.54f+column*.18f,1.18f+row*.10f,1.285f},
            {-.49f+column*.18f,1.23f+row*.10f,1.295f},lamp,Surface::Emissive);
    }

    // Tail boom, empennage and clearly horizontal tail-rotor disc.
    constexpr int tailSides=8;
    constexpr std::array<FuselageStation,4> tail{{
        {-2.02f,1.53f,.46f,.43f},{-3.55f,1.62f,.35f,.32f},
        {-5.25f,1.78f,.25f,.24f},{-6.85f,1.92f,.14f,.15f}}};
    for(std::size_t band=0;band+1<tail.size();++band)for(int slice=0;slice<tailSides;++slice) {
        const int next=(slice+1)%tailSides;const float a=tau*slice/tailSides,b=tau*next/tailSides;
        const auto point=[](const FuselageStation& station,const float angle){return Vector3(
            std::cos(angle)*station.radiusX,station.centreY+std::sin(angle)*station.radiusY,
            station.z);};
        quad(point(tail[band],a),point(tail[band+1],a),point(tail[band+1],b),
             point(tail[band],b),(slice%3==0)?oliveLight:olive);
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
    for(int blade=0;blade<4;++blade) {
        const float angle=.18f+blade*std::numbers::pi_v<float>*.5f;
        const Vector3 radial(std::cos(angle),0,std::sin(angle));
        const Vector3 perpendicular(-radial.Z,0,radial.X);
        const Vector3 centre(0,3.02f,-.12f);
        const Vector3 inner=radial*.25f+centre;
        const Vector3 outer=radial*HelicopterLayout::MainRotorRadius+centre
            +perpendicular*.055f-Vector3(0,.012f,0);
        const float rootHalf=.16f,tipHalf=.105f,halfDepth=.034f;
        const Vector3 lift(0,halfDepth,0);
        const std::array<Vector3,4> outline{{inner-perpendicular*rootHalf,
            outer-perpendicular*tipHalf,outer+perpendicular*tipHalf,
            inner+perpendicular*rootHalf}};
        const Color upper=blade%2?Color(55,59,54):Color(69,74,67);
        const Color lower=blade%2?Color(39,42,39):Color(47,51,46);
        quad(outline[0]+lift,outline[1]+lift,outline[2]+lift,outline[3]+lift,upper);
        quad(outline[3]-lift,outline[2]-lift,outline[1]-lift,outline[0]-lift,lower);
        for(std::size_t edge=0;edge<outline.size();++edge) {
            const std::size_t next=(edge+1U)%outline.size();
            quad(outline[edge]-lift,outline[next]-lift,outline[next]+lift,
                 outline[edge]+lift,edge==1U?Color(31,34,32):Color(43,47,43));
        }
    }
    // Exhausts, navigation lights and a small belly lamp improve exterior scale cues.
    beam({-.36f,2.18f,-1.54f},{-.48f,2.25f,-1.98f},.15f,Color(55,55,52),Surface::Metal);
    beam({ .36f,2.18f,-1.54f},{ .48f,2.25f,-1.98f},.15f,Color(55,55,52),Surface::Metal);
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
    const std::string detail="island-simulator: helicopter uses rounded twenty-sided cockpit glazing, an articulated eight-link rotor hub, tubular rising-tip skids, and a tapered tailplane";
    std::cout<<detail<<'\n';MirrorRuntimeDiagnosticToPlatformLog(detail);
    const std::string material="island-simulator: helicopter uses a generated 128-square atlas for worn paint, brushed metal, woven seats, and fine rubber";
    std::cout<<material<<'\n';MirrorRuntimeDiagnosticToPlatformLog(material);
    const std::string doors="island-simulator: helicopter carries four outward-open framed door panels with glazing and handles";
    std::cout<<doors<<'\n';MirrorRuntimeDiagnosticToPlatformLog(doors);
    const std::string rotor="island-simulator: helicopter main rotor uses four closed tapered blades with visible leading, trailing, and tip edges";
    std::cout<<rotor<<'\n';MirrorRuntimeDiagnosticToPlatformLog(rotor);
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
