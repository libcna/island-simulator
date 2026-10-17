#include "IslandSimulator/BearRenderer.hpp"

#include "IslandSimulator/CanopyLightField.hpp"
#include "IslandSimulator/EnvironmentLighting.hpp"
#include "IslandSimulator/GeneratedTextureBudget.hpp"
#include "IslandSimulator/OpaqueImageLoader.hpp"
#include "IslandSimulator/RuntimeDiagnostics.hpp"
#include "IslandSimulator/WildlifeRenderLighting.hpp"
#include "Microsoft/Xna/Framework/BoundingFrustum.hpp"
#include "Microsoft/Xna/Framework/BoundingSphere.hpp"
#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Graphics/BasicEffect.hpp"
#include "Microsoft/Xna/Framework/Graphics/BlendState.hpp"
#include "Microsoft/Xna/Framework/Graphics/BufferUsage.hpp"
#include "Microsoft/Xna/Framework/Graphics/DepthStencilState.hpp"
#include "Microsoft/Xna/Framework/Graphics/DynamicVertexBuffer.hpp"
#include "Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp"
#include "Microsoft/Xna/Framework/Graphics/PrimitiveType.hpp"
#include "Microsoft/Xna/Framework/Graphics/RasterizerState.hpp"
#include "Microsoft/Xna/Framework/Graphics/SamplerState.hpp"
#include "Microsoft/Xna/Framework/Graphics/SetDataOptions.hpp"
#include "Microsoft/Xna/Framework/Graphics/SurfaceFormat.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"
#include "Microsoft/Xna/Framework/Graphics/VertexPositionColorTexture.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <numbers>
#include <stdexcept>

using namespace Microsoft::Xna::Framework;
using namespace Microsoft::Xna::Framework::Graphics;

namespace island {
namespace {
constexpr int VertexCapacity=36000,ShadowCapacity=180;
constexpr float Pi=std::numbers::pi_v<float>;
enum class Material:std::uint8_t {Fur,DarkFur,Muzzle,Nose,Bare,Claw,EyeHighlight};

Vector3 Point(BearPoint p){return {p.x,p.y,p.z};}
BearPoint Point(Vector3 p){return {p.X,p.Y,p.Z};}
Vector3 SafeUnit(Vector3 p) {if(p.LengthSquared()<1e-10f)return Vector3::Up;p.Normalize();return p;}
Vector2 AtlasUv(Material material,float u,float v) {
    u=std::clamp(u,0.0f,1.0f);v=std::clamp(v,0.0f,1.0f);
    if(material==Material::Fur)return {.025f+.95f*u,.025f+.95f*v};
    if(material==Material::DarkFur)return {.975f-.95f*u,.025f+.95f*v};
    // Short facial surfaces use bounded source windows rather than stretching one
    // long guard-hair clump over a nose, eye or mouth mark.
    if(material==Material::Muzzle)return {.40f+.20f*u,.40f+.20f*v};
    if(material==Material::EyeHighlight)return {.70f+.02f*u,.18f+.02f*v};
    return {.18f+.025f*u,.86f+.025f*v};
}
Color BaseColor(Material material) {
    switch(material) {
    case Material::Fur:return Color(245,245,245);
    case Material::DarkFur:return Color(176,166,154);
    case Material::Muzzle:return Color(235,222,205);
    case Material::Nose:return Color(39,34,30);
    case Material::Bare:return Color(15,13,12);
    case Material::Claw:return Color(82,68,49);
    case Material::EyeHighlight:return Color(184,168,142);
    }
    return Color::White;
}
class BearMeshBuilder {
public:
    BearMeshBuilder(std::vector<VertexPositionColorTexture>& output,const BearPose& pose,
        CanopyLightTint canopy):output_(output),pose_(pose),canopy_(canopy){}
    void Ellipsoid(Vector3 centre,Vector3 right,Vector3 up,Vector3 forward,float rx,float ry,float rz,
        Material material,int longitude,int latitude) {
        right=SafeUnit(right);up=SafeUnit(up);forward=SafeUnit(forward);
        const auto point=[&](float a,float b){const float c=std::cos(a);return centre+right*(rx*c*std::cos(b))
            +up*(ry*std::sin(a))+forward*(rz*c*std::sin(b));};
        const auto normal=[&](float a,float b){const float c=std::cos(a);return SafeUnit(
            right*(c*std::cos(b)/rx)+up*(std::sin(a)/ry)+forward*(c*std::sin(b)/rz));};
        for(int y=0;y<latitude;++y)for(int x=0;x<longitude;++x) {
            const float a0=-Pi*.5f+Pi*y/latitude,a1=-Pi*.5f+Pi*(y+1)/latitude;
            const float b0=2*Pi*x/longitude,b1=2*Pi*(x+1)/longitude;
            Quad(point(a0,b0),normal(a0,b0),{static_cast<float>(x)/longitude,static_cast<float>(y)/latitude},
                 point(a0,b1),normal(a0,b1),{static_cast<float>(x+1)/longitude,static_cast<float>(y)/latitude},
                 point(a1,b1),normal(a1,b1),{static_cast<float>(x+1)/longitude,static_cast<float>(y+1)/latitude},
                 point(a1,b0),normal(a1,b0),{static_cast<float>(x)/longitude,static_cast<float>(y+1)/latitude},material);
        }
    }
    void Tube(Vector3 a,Vector3 b,float radiusA,float radiusB,Material material,int segments) {
        const Vector3 axis=SafeUnit(b-a);
        const Vector3 side=SafeUnit(Vector3::Cross(axis,std::abs(axis.Y)<.86f?Vector3::Up:Vector3::Right));
        const Vector3 other=SafeUnit(Vector3::Cross(side,axis));
        for(int i=0;i<segments;++i) {
            const float t0=2*Pi*i/segments,t1=2*Pi*(i+1)/segments;
            const Vector3 n0=SafeUnit(side*std::cos(t0)+other*std::sin(t0));
            const Vector3 n1=SafeUnit(side*std::cos(t1)+other*std::sin(t1));
            Quad(a+n0*radiusA,n0,{static_cast<float>(i)/segments,0},a+n1*radiusA,n1,{static_cast<float>(i+1)/segments,0},
                b+n1*radiusB,n1,{static_cast<float>(i+1)/segments,1},b+n0*radiusB,n0,{static_cast<float>(i)/segments,1},material);
        }
    }
    void LimbLoft(Vector3 root,Vector3 joint,Vector3 ankle,bool front,Material material,int segments) {
        // Keep one continuous surface through shoulder/haunch, knee and wrist.  A
        // polyline through the exact IK joint still made a mechanically sharp elbow
        // in profile, so round only the rendered centreline around that authoritative
        // point.  The root and ankle remain exact and the curve stays inside the two
        // solved bone segments; motion/contact therefore remain owned by BearMotion.
        constexpr std::array<float,9> fractions{0.0f,.10f,.25f,.42f,.55f,.68f,.82f,.94f,1.0f};
        const std::array<float,9> frontRadii{.235f,.232f,.216f,.190f,.166f,.151f,.137f,.124f,.116f};
        const std::array<float,9> rearRadii{.268f,.264f,.244f,.210f,.181f,.162f,.145f,.129f,.118f};
        const auto& radii=front?frontRadii:rearRadii;
        std::array<Vector3,fractions.size()> centres{};
        const Vector3 approach=joint+(root-joint)*.24f;
        const Vector3 departure=joint+(ankle-joint)*.24f;
        const auto roundedJoint=[&](float t) {
            const float one=1.0f-t;
            return approach*(one*one)+joint*(2.0f*one*t)+departure*(t*t);
        };
        for(std::size_t ring=0;ring<fractions.size();++ring) {
            const float t=fractions[ring];
            if(t<.42f)centres[ring]=root+(approach-root)*(t/.42f);
            else if(t>.68f)centres[ring]=departure+(ankle-departure)*((t-.68f)/.32f);
            else centres[ring]=roundedJoint((t-.42f)/.26f);
        }
        for(std::size_t ring=0;ring+1<fractions.size();++ring) {
            const auto ringFrame=[&](std::size_t index,float angle) {
                const Vector3 before=centres[index==0?0:index-1];
                const Vector3 after=centres[std::min(index+1,centres.size()-1)];
                const Vector3 axis=SafeUnit(after-before);
                const Vector3 across=SafeUnit(Vector3::Right-axis*axis.X);
                const Vector3 around=SafeUnit(Vector3::Cross(axis,across));
                const Vector3 radial=across*std::cos(angle)+around*std::sin(angle);
                const std::size_t prior=index==0?0:index-1;
                const std::size_t next=std::min(index+1,radii.size()-1);
                const float span=std::max((centres[next]-centres[prior]).Length(),.001f);
                const Vector3 normal=SafeUnit(radial-axis*((radii[next]-radii[prior])/span));
                return std::array<Vector3,2>{centres[index]+radial*radii[index],normal};
            };
            for(int segment=0;segment<segments;++segment) {
                const float a0=2*Pi*segment/segments,a1=2*Pi*(segment+1)/segments;
                const auto p00=ringFrame(ring,a0),p01=ringFrame(ring,a1);
                const auto p10=ringFrame(ring+1,a0),p11=ringFrame(ring+1,a1);
                const float u0=static_cast<float>(segment)/segments;
                const float u1=static_cast<float>(segment+1)/segments;
                Quad(p00[0],p00[1],{u0,fractions[ring]},
                    p01[0],p01[1],{u1,fractions[ring]},
                    p11[0],p11[1],{u1,fractions[ring+1]},
                    p10[0],p10[1],{u0,fractions[ring+1]},material);
            }
        }
    }
    template<std::size_t Count>
    void BodyLoft(const std::array<Vector3,Count>& centres,
                  const std::array<Vector2,Count>& radii,Material material,int segments) {
        static_assert(Count>=2);
        for(std::size_t ring=0;ring+1<Count;++ring) {
            const auto ringFrame=[&](std::size_t index,float angle) {
                const std::size_t prior=index==0?0:index-1;
                const std::size_t next=std::min(index+1,Count-1);
                const Vector3 axis=SafeUnit(centres[next]-centres[prior]);
                const Vector3 across=SafeUnit(Vector3::Right-axis*axis.X);
                const Vector3 around=SafeUnit(Vector3::Cross(axis,across));
                const float c=std::cos(angle),s=std::sin(angle);
                const Vector3 radial=across*(radii[index].X*c)+around*(radii[index].Y*s);
                const Vector3 circumferential=across*(-radii[index].X*s)
                    +around*(radii[index].Y*c);
                const Vector3 longitudinal=centres[next]-centres[prior]
                    +across*((radii[next].X-radii[prior].X)*c)
                    +around*((radii[next].Y-radii[prior].Y)*s);
                Vector3 normal=SafeUnit(Vector3::Cross(circumferential,longitudinal));
                if(Vector3::Dot(normal,radial)<0)normal*=-1.0f;
                return std::array<Vector3,2>{centres[index]+radial,normal};
            };
            for(int segment=0;segment<segments;++segment) {
                // Place the non-tileable source-window seam beneath the belly rather
                // than across either broad flank seen by the player.
                const float a0=2*Pi*segment/segments-Pi*.5f;
                const float a1=2*Pi*(segment+1)/segments-Pi*.5f;
                const auto p00=ringFrame(ring,a0),p01=ringFrame(ring,a1);
                const auto p10=ringFrame(ring+1,a0),p11=ringFrame(ring+1,a1);
                const float u0=static_cast<float>(segment)/segments;
                const float u1=static_cast<float>(segment+1)/segments;
                const float v0=static_cast<float>(ring)/(Count-1);
                const float v1=static_cast<float>(ring+1)/(Count-1);
                // The source guard hairs run along U. Map U down the animal and V
                // around the cross-section; the opposite assignment stretched whole
                // clumps into conspicuous horizontal bands along the new long loft.
                Quad(p00[0],p00[1],{v0,u0},p01[0],p01[1],{v0,u1},
                    p11[0],p11[1],{v1,u1},p10[0],p10[1],{v1,u0},material);
            }
        }
    }
    void FurTuft(Vector3 root,float halfWidth,float length,float sweep) {
        const Vector3 across=Vector3::Right;
        const Vector3 normal=SafeUnit(Vector3::Up+Vector3(0,0,-.12f));
        const Vector3 tip=root+Vector3(0,length,sweep);
        Quad(root-across*halfWidth,normal,{.16f,.88f},
            root+across*halfWidth,normal,{.28f,.88f},
            tip+across*halfWidth*.28f,normal,{.28f,.18f},
            tip-across*halfWidth*.28f,normal,{.16f,.18f},Material::Fur);
    }
private:
    Vector3 World(Vector3 local)const{return Point(pose_.ToWorld(Point(local)));}
    Vector3 WorldNormal(Vector3 local)const{return SafeUnit(Point(pose_.right)*local.X
        +Point(pose_.up)*local.Y+Point(pose_.forward)*local.Z);}
    void Vertex(Vector3 p,Vector3 normal,Vector2 uv,Material material) {
        const auto worldNormal=WorldNormal(normal);const auto base=BaseColor(material);
        const float light=.55f+.37f*std::max(0.0f,Vector3::Dot(worldNormal,wildlife_render::FixedSunDirection))
            +.10f*std::max(0.0f,worldNormal.Y);
        const auto channel=[&](std::uint8_t value,float tint){return static_cast<int>(std::clamp(
            std::lround(value*light*tint),0L,255L));};
        output_.emplace_back(World(p),Color(channel(base.getRProperty(),canopy_.red),
            channel(base.getGProperty(),canopy_.green),channel(base.getBProperty(),canopy_.blue),255),
            AtlasUv(material,uv.X,uv.Y));
    }
    void Triangle(Vector3 a,Vector3 na,Vector2 ua,Vector3 b,Vector3 nb,Vector2 ub,
        Vector3 c,Vector3 nc,Vector2 uc,Material material) {
        Vertex(a,na,ua,material);Vertex(b,nb,ub,material);Vertex(c,nc,uc,material);
    }
    void Quad(Vector3 a,Vector3 na,Vector2 ua,Vector3 b,Vector3 nb,Vector2 ub,
        Vector3 c,Vector3 nc,Vector2 uc,Vector3 d,Vector3 nd,Vector2 ud,Material material) {
        Triangle(a,na,ua,b,nb,ub,c,nc,uc,material);Triangle(a,na,ua,c,nc,uc,d,nd,ud,material);
    }
    std::vector<VertexPositionColorTexture>& output_;const BearPose& pose_;CanopyLightTint canopy_;
};

void BuildBear(std::vector<VertexPositionColorTexture>& vertices,const BearState& state,
    CanopyLightTint canopy,bool near) {
    BearMeshBuilder mesh(vertices,state.pose,canopy);const int lon=near?22:10,lat=near?14:7;
    const float breath=.014f*std::sin(state.breathPhase),forage=state.foragePose;
    const Vector3 right=Vector3::Right,up=Vector3::Up,forward(0,0,1);
    const Vector3 head{0,1.30f-forage*.48f,1.15f+forage*.18f};
    const float bob=state.pose.bodyBob;
    const std::array<Vector3,13> bodyCentres{{
        {0,1.08f+bob,-1.10f},{0,1.05f+bob,-.98f},{0,1.04f+bob,-.78f},
        {0,1.04f+bob,-.52f},{0,1.04f+bob,-.25f},{0,1.10f+bob,.02f},
        {0,1.25f+bob,.24f},{0,1.34f+bob,.42f},{0,1.32f+bob-forage*.06f,.58f},
        {0,1.22f-forage*.18f,.70f+forage*.04f},
        {0,head.Y,head.Z-.30f},{0,head.Y,head.Z-.20f},{0,head.Y,head.Z-.12f}}};
    const std::array<Vector2,13> bodyRadii{{
        {.12f+breath*.2f,.14f+breath*.2f},{.36f+breath*.5f,.38f+breath*.5f},
        {.50f+breath,.48f+breath},{.57f+breath,.53f+breath},
        {.59f+breath,.55f+breath},{.60f+breath,.54f+breath},
        {.625f,.525f},{.595f,.475f},{.525f,.445f},{.45f,.41f},
        {.405f,.37f},{.28f,.27f},{.14f,.14f}}};
    mesh.BodyLoft(bodyCentres,bodyRadii,Material::Fur,lon);
    // Keep the crown broad but take some height out of the old near-spherical head.
    // The flatter crown and deeper rear skull read as a heavy ursine head instead of
    // a round plush-toy ball, while retaining the established neck connection.
    mesh.Ellipsoid(head,right,up,forward,.44f,.355f,.425f,Material::Fur,lon,lat);
    if(near) {
        // Short overlapping wedges break the mathematically smooth top outline. Roots
        // follow the new continuous trunk loft and the overlapping head, so breathing,
        // foraging and the shoulder hump cannot expose floating fur cards.
        const auto ellipsoidTop=[](float z,float centreZ,float centreY,float radiusY,float radiusZ) {
            const float t=(z-centreZ)/radiusZ;
            return std::abs(t)<=1.0f?centreY+radiusY*std::sqrt(std::max(0.0f,1.0f-t*t)):-100.0f;
        };
        const auto trunkTop=[&](float z) {
            for(std::size_t section=0;section+1<bodyCentres.size();++section) {
                if(z<bodyCentres[section].Z||z>bodyCentres[section+1].Z)continue;
                const float span=bodyCentres[section+1].Z-bodyCentres[section].Z;
                const float t=span>.0001f?(z-bodyCentres[section].Z)/span:0.0f;
                const float centreY=bodyCentres[section].Y
                    +(bodyCentres[section+1].Y-bodyCentres[section].Y)*t;
                const float radiusY=bodyRadii[section].Y
                    +(bodyRadii[section+1].Y-bodyRadii[section].Y)*t;
                return centreY+radiusY;
            }
            return -100.0f;
        };
        for(int tuft=0;tuft<22;++tuft) {
            const float z=-1.02f+static_cast<float>(tuft)*.112f;
            const float y=std::max(trunkTop(z),
                ellipsoidTop(z,head.Z,head.Y,.35f,.39f));
            if(y>-50.0f)mesh.FurTuft({0,y-.004f,z},.020f+.003f*(tuft%3),
                .030f+.006f*(tuft%4),-.014f-.004f*(tuft%2));
        }
    }
    // A single four-ring fur bridge carries the skull into the muzzle.  This provides
    // the broad wedge seen from above without separate spherical cheeks, while the
    // progressively lower centres give the forehead, stop and jaw a clear side profile.
    const std::array<Vector3,4> faceCentres{{
        head+forward*.185f+up*.020f,
        head+forward*.285f-up*.010f,
        head+forward*.375f-up*.060f,
        head+forward*.445f-up*.095f}};
    const std::array<Vector2,4> faceRadii{{
        {.350f,.280f},{.305f,.235f},{.250f,.185f},{.190f,.143f}}};
    mesh.BodyLoft(faceCentres,faceRadii,Material::Fur,lon);

    // Keep the contrasting muzzle confined to the distal face rather than painting a
    // broad oval over the cheeks. Two nested volumes retain a short natural taper.
    const Vector3 muzzleRoot=head+forward*.420f-up*.105f;
    const Vector3 muzzle=muzzleRoot+forward*.145f-up*.008f;
    mesh.Ellipsoid(muzzleRoot,right,up,forward,.202f,.145f,.195f,
        Material::Muzzle,near?18:9,near?10:6);
    mesh.Ellipsoid(muzzle,right,up,forward,.162f,.112f,.146f,
        Material::Muzzle,near?16:9,near?9:6);
    const Vector3 nose=muzzle+forward*.137f+up*.028f;
    mesh.Ellipsoid(nose,right,up,forward,.094f,.057f,.041f,
        Material::Nose,near?14:8,near?8:5);
    mesh.Ellipsoid({0,1.10f,-1.08f},right,up,forward,.13f,.13f,.14f,Material::Fur,near?12:8,near?7:5);
    for(int side=-1;side<=1;side+=2) {
        const Vector3 ear=head+right*(side*.282f)+up*.285f-forward*.045f;
        mesh.Ellipsoid(ear,right,up,forward,.116f,.121f,.074f,Material::Fur,near?12:8,near?8:5);
        if(near) {
            mesh.Ellipsoid(ear+forward*.052f,right,up,forward,.068f,.074f,.019f,Material::DarkFur,10,7);

            // A broad brow and recessed dark socket surround a smaller visible eye.
            // Moving the pair inward also gives the face the forward-looking, deep-set
            // expression of a brown bear instead of two dots on the sides of a sphere.
            const Vector3 brow=head+right*(side*.202f)+up*.132f+forward*.292f;
            mesh.Ellipsoid(brow,right,up,forward,.132f,.066f,.102f,Material::Fur,12,7);
            const Vector3 socket=head+right*(side*.205f)+up*.068f+forward*.374f;
            mesh.Ellipsoid(socket,right,up,forward,.058f,.040f,.017f,Material::DarkFur,10,6);
            const Vector3 eye=socket+forward*.014f;
            mesh.Ellipsoid(eye,right,up,forward,.026f,.020f,.009f,Material::Bare,9,6);
            mesh.Ellipsoid(eye-right*(side*.004f)+up*.006f+forward*.008f,right,up,forward,
                .006f,.0045f,.0025f,Material::EyeHighlight,7,5);
        }
    }
    for(std::size_t leg=0;leg<4;++leg) {
        const auto& p=state.pose.legs[leg];const Vector3 root=Point(p.root),joint=Point(p.joint),ankle=Point(p.paw);
        const bool front=leg%2==1;
        mesh.LimbLoft(root,joint,ankle,front,Material::Fur,near?14:8);
        const Vector3 sole=ankle-up*BearLegPose::PawHeight;
        const Vector3 pawUp=SafeUnit(Point(p.soleNormal));
        const Vector3 pawRight=SafeUnit(Vector3::Cross(pawUp,forward));
        const Vector3 pawForward=SafeUnit(Vector3::Cross(pawRight,pawUp));
        const Vector3 paw=sole+pawUp*.080f+pawForward*.065f;
        mesh.Ellipsoid(paw,pawRight,pawUp,pawForward,.170f,.090f,.250f,
            Material::DarkFur,near?14:8,near?7:5);
        if(near)for(int claw=-2;claw<=2;++claw) {
            // Bury a broad horn-coloured root in the toe and bend the short tip
            // down. The previous single black needle read as a detached spike.
            const Vector3 base=paw+pawRight*(claw*.048f)+pawForward*.184f-pawUp*.018f;
            const float length=.052f-.003f*std::abs(claw);
            const Vector3 joint=base+pawForward*(length*.62f)-pawUp*.006f;
            const Vector3 tip=base+pawForward*length-pawUp*.022f;
            mesh.Tube(base,joint,.014f,.0085f,Material::Claw,5);
            mesh.Tube(joint,tip,.0088f,.0025f,Material::Claw,5);
        }
    }
    if(near) {
        // Paired nostrils, a short philtrum and divided mouth crease remain shallow facial marks,
        // rather than the previous thick horizontal rod projecting beyond the muzzle.
        for(int side=-1;side<=1;side+=2) {
            const Vector3 nostril=nose+right*(side*.035f)+up*.003f+forward*.037f;
            mesh.Ellipsoid(nostril,right,up,forward,.014f,.008f,.0050f,Material::Bare,8,5);
        }
        const Vector3 philtrumTop=nose+forward*.045f-up*.030f;
        const Vector3 mouth=muzzle+forward*.132f-up*.047f;
        mesh.Tube(philtrumTop,mouth,.0045f,.0035f,Material::Bare,7);
        mesh.Tube(mouth,mouth-right*.088f-up*.004f-forward*.010f,
            .0040f,.0025f,Material::Bare,7);
        mesh.Tube(mouth,mouth+right*.088f-up*.004f-forward*.010f,
            .0040f,.0025f,Material::Bare,7);
        mesh.Ellipsoid(mouth-up*.037f-forward*.025f,right,up,forward,
            .086f,.029f,.043f,Material::Muzzle,12,6);
    }
}
bool IsVisible(const Matrix& view,const Matrix& projection,const Vector3& camera,
    const BearState& state,float maximumDistance) {
    const Vector3 centre(state.x,state.y+1.0f,state.z);
    return Vector3::DistanceSquared(centre,camera)<=maximumDistance*maximumDistance
        &&BoundingFrustum(view*projection).Intersects(BoundingSphere(centre,1.8f));
}
}

BearRenderer::BearRenderer(GraphicsDevice& device,const CanopyLightField& canopy,float maximumDistance)
    :device_(device),canopy_(canopy),maximumDistance_(maximumDistance),effect_(std::make_unique<BasicEffect>(device)),
     buffer_(std::make_unique<DynamicVertexBuffer>(device,VertexPositionColorTexture::getVertexDeclarationStatic(),
        VertexCapacity,BufferUsage::WriteOnly)),
     shadowBuffer_(std::make_unique<DynamicVertexBuffer>(device,VertexPositionColorTexture::getVertexDeclarationStatic(),
        ShadowCapacity,BufferUsage::WriteOnly)),furTexture_(std::make_unique<Texture2D>(device,
        GeneratedTextureBudget::BearFurSize,GeneratedTextureBudget::BearFurSize,true,SurfaceFormat::Color)) {
    if(!std::isfinite(maximumDistance)||maximumDistance<=0)throw std::invalid_argument("Bear draw distance must be positive");
    vertices_.reserve(VertexCapacity);shadows_.reserve(ShadowCapacity);
    const auto levels=LoadOpaqueImageMips(device,"Content/wildlife/bear-fur.png",
        GeneratedTextureBudget::BearFurSize);
    for(std::size_t level=0;level<levels.size();++level) {
        const auto& pixels=levels[level].pixels;
        furTexture_->SetData(static_cast<int>(level),nullptr,
            reinterpret_cast<const std::uint8_t*>(pixels.data()),0,
            static_cast<int>(pixels.size()*4));
    }
    effect_->setVertexColorEnabledProperty(true);effect_->setLightingEnabledProperty(false);effect_->setFogEnabledProperty(true);
    effect_->setFogColorProperty(Vector3(environment::FogColorRed,environment::FogColorGreen,environment::FogColorBlue));
    effect_->setFogStartProperty(maximumDistance*.65f);effect_->setFogEndProperty(maximumDistance);
    const char* material="island-simulator: bear fur loaded Content/wildlife/bear-fur.png; 128x128, 8 mips, 87380 RGBA8 bytes";
    std::cout<<material<<'\n';MirrorRuntimeDiagnosticToPlatformLog(material);
    const char* diagnostic="island-simulator: one large peaceful bear uses original articulated quadruped anatomy and a photographic-detail directional-fur material";
    std::cout<<diagnostic<<'\n';MirrorRuntimeDiagnosticToPlatformLog(diagnostic);
    const char* faceDiagnostic="island-simulator: near bear uses a two-volume tapered muzzle, volumetric nose, paired nostrils, divided mouth crease, and highlighted eyes";
    std::cout<<faceDiagnostic<<'\n';MirrorRuntimeDiagnosticToPlatformLog(faceDiagnostic);
    const char* furDiagnostic="island-simulator: near bear uses twenty-two body-envelope fur wedges without another draw";
    std::cout<<furDiagnostic<<'\n';MirrorRuntimeDiagnosticToPlatformLog(furDiagnostic);
    const char* limbDiagnostic="island-simulator: bear legs use rounded shoulder-to-wrist lofts and five-clawed broad paws";
    std::cout<<limbDiagnostic<<'\n';MirrorRuntimeDiagnosticToPlatformLog(limbDiagnostic);
    const char* clawDiagnostic="island-simulator: near bear paws use short two-segment horn claws with buried roots and downward tips";
    std::cout<<clawDiagnostic<<'\n';MirrorRuntimeDiagnosticToPlatformLog(clawDiagnostic);
    const char* torsoDiagnostic="island-simulator: bear torso uses a continuous rump-to-neck loft without overlapping body shells";
    std::cout<<torsoDiagnostic<<'\n';MirrorRuntimeDiagnosticToPlatformLog(torsoDiagnostic);
    const char* headDiagnostic="island-simulator: near bear uses a flattened broad skull, compact rounded ears, a continuous tapered facial bridge, recessed eye sockets, and a short fitted muzzle";
    std::cout<<headDiagnostic<<'\n';MirrorRuntimeDiagnosticToPlatformLog(headDiagnostic);
}
BearRenderer::~BearRenderer()=default;

void BearRenderer::DrawShadow(const Matrix& view,const Matrix& projection,const Vector3& camera,const BearState& bear) {
    draws_=triangles_=shadowTriangles_=visible_=0;shadows_.clear();
    if(!IsVisible(view,projection,camera,bear,maximumDistance_))return;
    const Vector3 origin(bear.x,bear.y+.012f,bear.z),right=Point(bear.pose.right),forward=Point(bear.pose.forward);
    constexpr int segments=18;const Color centre(20,17,14,62),edge(20,17,14,0);
    for(int i=0;i<segments;++i) {
        const float a0=2*Pi*i/segments,a1=2*Pi*(i+1)/segments;
        shadows_.emplace_back(origin,centre,Vector2(.5f,.5f));
        shadows_.emplace_back(origin+right*(.64f*std::cos(a0))+forward*(1.18f*std::sin(a0)),edge,Vector2::Zero);
        shadows_.emplace_back(origin+right*(.64f*std::cos(a1))+forward*(1.18f*std::sin(a1)),edge,Vector2::Zero);
    }
    shadowTriangles_=static_cast<int>(shadows_.size()/3);
    shadowBuffer_->SetData(shadows_.data(),0,static_cast<int>(shadows_.size()),SetDataOptions::Discard);
    device_.setRasterizerStateProperty(RasterizerState::CullNone);device_.setBlendStateProperty(BlendState::AlphaBlend);
    device_.setDepthStencilStateProperty(DepthStencilState::DepthRead);device_.SetVertexBuffer(shadowBuffer_.get());
    device_.setIndicesProperty(nullptr);effect_->setTextureEnabledProperty(false);effect_->setWorldProperty(Matrix::getIdentityProperty());
    effect_->setViewProperty(view);effect_->setProjectionProperty(projection);effect_->Apply();
    device_.DrawPrimitives(PrimitiveType::TriangleList,0,shadowTriangles_);++draws_;
}
void BearRenderer::Draw(const Matrix& view,const Matrix& projection,const Vector3& camera,const BearState& bear) {
    vertices_.clear();visible_=0;triangles_=0;nearDetail_=false;
    if(!IsVisible(view,projection,camera,bear,maximumDistance_))return;
    const float distanceSquared=Vector3::DistanceSquared({bear.x,bear.y+1,bear.z},camera);
    nearDetail_=distanceSquared<55*55;
    const auto tint=CanopyTransmissionTint(canopy_.SampleReceiver(bear.x,bear.y+1,bear.z,bear.y));
    BuildBear(vertices_,bear,tint,nearDetail_);
    if(vertices_.empty()||vertices_.size()>VertexCapacity)throw std::length_error("Bear mesh exceeds bounded upload");
    triangles_=static_cast<int>(vertices_.size()/3);visible_=1;
    buffer_->SetData(vertices_.data(),0,static_cast<int>(vertices_.size()),SetDataOptions::Discard);
    device_.setRasterizerStateProperty(RasterizerState::CullNone);device_.setBlendStateProperty(BlendState::Opaque);
    device_.setDepthStencilStateProperty(DepthStencilState::Default);device_.SetVertexBuffer(buffer_.get());device_.setIndicesProperty(nullptr);
    device_.getSamplerStatesProperty()[0]=SamplerState::LinearClamp;effect_->setTextureEnabledProperty(true);
    effect_->setTextureProperty(furTexture_.get());effect_->setWorldProperty(Matrix::getIdentityProperty());
    effect_->setViewProperty(view);effect_->setProjectionProperty(projection);effect_->Apply();
    device_.DrawPrimitives(PrimitiveType::TriangleList,0,triangles_);++draws_;
}
}
