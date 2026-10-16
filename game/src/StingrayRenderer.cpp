#include "IslandSimulator/StingrayRenderer.hpp"

#include "IslandSimulator/EnvironmentLighting.hpp"
#include "IslandSimulator/RuntimeDiagnostics.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <numbers>
#include <stdexcept>
#include <string_view>

#include "Microsoft/Xna/Framework/BoundingFrustum.hpp"
#include "Microsoft/Xna/Framework/BoundingSphere.hpp"
#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/Graphics/BasicEffect.hpp"
#include "Microsoft/Xna/Framework/Graphics/BlendState.hpp"
#include "Microsoft/Xna/Framework/Graphics/BufferUsage.hpp"
#include "Microsoft/Xna/Framework/Graphics/DepthStencilState.hpp"
#include "Microsoft/Xna/Framework/Graphics/DynamicVertexBuffer.hpp"
#include "Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp"
#include "Microsoft/Xna/Framework/Graphics/PrimitiveType.hpp"
#include "Microsoft/Xna/Framework/Graphics/RasterizerState.hpp"
#include "Microsoft/Xna/Framework/Graphics/SetDataOptions.hpp"
#include "Microsoft/Xna/Framework/Graphics/VertexPositionColor.hpp"

using namespace Microsoft::Xna::Framework;
using namespace Microsoft::Xna::Framework::Graphics;

namespace island
{
    namespace
    {
        constexpr float NearDistance = 125.0f;
        constexpr int MaximumVerticesPerRay = 1100;
        constexpr int MaximumVertices = static_cast<int>(StingrayGroup::Count)
            * MaximumVerticesPerRay;
        constexpr std::string_view StingrayProfileDiagnostic =
            "island-simulator: near stingrays use a curved diamond disc, raised central body, wedge nose, and restrained dark rim";
        constexpr std::string_view StingrayGeometryDiagnostic =
            "island-simulator: stingray near/far geometry 1086/18 vertices with a continuous crossed-volume tapered tail";
        constexpr std::string_view StingrayAnatomyDiagnostic=
            "island-simulator: near stingrays add rounded eye mounds, circular dorsal spots, a feathered disc edge, and a continuous tail loft";
        static_assert(MaximumVerticesPerRay>=1086);

        struct Palette
        {
            Color dorsal, highlight, margin, underside, spot, eye, tail;
        };

        const std::array<Palette, 3> Palettes{{
            {Color(127,112,78,255),Color(157,143,104,255),Color(105,94,70,255),
             Color(183,176,143,255),Color(71,65,49,255),Color(18,16,12,255),Color(94,82,58,255)},
            {Color(111,102,78,255),Color(145,134,105,255),Color(93,87,71,255),
             Color(172,169,145,255),Color(67,63,54,255),Color(17,16,14,255),Color(82,76,61,255)},
            {Color(142,119,82,255),Color(172,148,106,255),Color(118,100,72,255),
             Color(193,180,145,255),Color(84,70,50,255),Color(20,17,12,255),Color(106,89,62,255)}
        }};

        constexpr int DiscSegments=32;

        void Triangle(std::vector<VertexPositionColor>& out, const Vector3& a,
                      const Vector3& b, const Vector3& c, const Color color)
        {
            out.emplace_back(a,color); out.emplace_back(b,color); out.emplace_back(c,color);
        }

        [[nodiscard]] Color ScaleColor(const Color& color,const float scale)
        {
            return Color(static_cast<int>(std::clamp(color.getRProperty()*scale,0.0f,255.0f)),
                         static_cast<int>(std::clamp(color.getGProperty()*scale,0.0f,255.0f)),
                         static_cast<int>(std::clamp(color.getBProperty()*scale,0.0f,255.0f)),
                         static_cast<int>(color.getAProperty()));
        }

        void LitTriangle(std::vector<VertexPositionColor>& out,const Vector3& a,
                         const Vector3& b,const Vector3& c,const Color color)
        {
            Vector3 normal=Vector3::Cross(b-a,c-a);
            if(normal.LengthSquared()>1e-8f) normal.Normalize(); else normal=Vector3::Up;
            if(normal.Y<0.0f) normal=-normal;
            const Vector3 sun=Vector3::Normalize(Vector3(.46f,.83f,-.31f));
            const float light=.72f+.28f*std::max(0.0f,Vector3::Dot(normal,sun));
            Triangle(out,a,b,c,ScaleColor(color,light));
        }

        void DoubleTriangle(std::vector<VertexPositionColor>& out, const Vector3& a,
                            const Vector3& b, const Vector3& c, const Color color)
        {
            Triangle(out,a,b,c,color); Triangle(out,c,b,a,color);
        }

        struct Basis
        {
            Vector3 centre, right, up, forward;
            float scale = 1.0f;

            [[nodiscard]] Vector3 Point(const float x, const float y,
                                        const float z) const noexcept
            {
                return centre + right*(x*scale) + up*(y*scale) + forward*(z*scale);
            }
        };

        [[nodiscard]] Basis MakeBasis(const StingrayState& ray)
        {
            const Vector3 forward(std::cos(ray.headingRadians),0.0f,
                                  std::sin(ray.headingRadians));
            const Vector3 flatRight(-forward.Z,0.0f,forward.X);
            const float cosine=std::cos(ray.bankRadians),sine=std::sin(ray.bankRadians);
            return {{ray.x,ray.y,ray.z},flatRight*cosine+Vector3::Up*sine,
                    Vector3::Up*cosine-flatRight*sine,forward,ray.scale};
        }

        [[nodiscard]] Vector3 DiscPoint(const Basis& basis, const float x,
                                        const float z, const float wave, const float shell)
        {
            const float lateral=std::clamp(std::fabs(x)/1.34f,0.0f,1.0f);
            const float longitudinal=std::clamp(1.0f-std::fabs(z-.04f)/.96f,0.0f,1.0f);
            const float flap=wave*.19f*std::pow(lateral,1.05f);
            const float centralBody=std::pow(1.0f-lateral,1.55f)
                *std::pow(longitudinal,.72f);
            const float crown=shell*(.010f+.145f*centralBody);
            const float feather=-.030f*std::pow(lateral,2.7f)*longitudinal;
            return basis.Point(x,flap+crown+feather,z);
        }

        [[nodiscard]] std::array<float,2> Outline(const int index,const float radius)
        {
            const float angle=2.0f*std::numbers::pi_v<float>
                *static_cast<float>(index)/static_cast<float>(DiscSegments);
            const float cosine=std::cos(angle);
            const float sine=std::sin(angle);
            const float width=1.34f*(.97f+.03f*std::max(0.0f,-cosine));
            const float length=cosine>=0.0f?.98f:.72f;
            // A low-power superellipse keeps the wing tips and snout distinct instead of
            // projecting as the old oval carpet from a swimming-height camera.
            constexpr float exponent=1.22f;
            const float radial=std::pow(
                std::pow(std::fabs(sine)/width,exponent)
                    +std::pow(std::fabs(cosine)/length,exponent),
                -1.0f/exponent);
            return {{sine*radial*radius,cosine*radial*radius}};
        }

        void AppendNear(std::vector<VertexPositionColor>& out, const StingrayState& ray)
        {
            const Palette& palette=Palettes[ray.colorVariant%Palettes.size()];
            const Basis basis=MakeBasis(ray);
            const Vector3 dorsalCentre=basis.Point(0,.105f,.04f);
            const Vector3 ventralCentre=basis.Point(0,-.050f,.01f);
            for(int index=0;index<DiscSegments;++index) {
                const auto innerA=Outline(index,.57f);
                const auto innerB=Outline((index+1)%DiscSegments,.57f);
                const auto middleA=Outline(index,.975f);
                const auto middleB=Outline((index+1)%DiscSegments,.975f);
                const auto outerA=Outline(index,1.0f);
                const auto outerB=Outline((index+1)%DiscSegments,1.0f);
                const Vector3 innerPointA=DiscPoint(
                    basis,innerA[0],innerA[1],ray.wingWave,1.0f);
                const Vector3 innerPointB=DiscPoint(
                    basis,innerB[0],innerB[1],ray.wingWave,1.0f);
                const Vector3 middlePointA=DiscPoint(
                    basis,middleA[0],middleA[1],ray.wingWave,1.0f);
                const Vector3 middlePointB=DiscPoint(
                    basis,middleB[0],middleB[1],ray.wingWave,1.0f);
                const Vector3 outerPointA=DiscPoint(
                    basis,outerA[0],outerA[1],ray.wingWave,1.0f);
                const Vector3 outerPointB=DiscPoint(
                    basis,outerB[0],outerB[1],ray.wingWave,1.0f);
                LitTriangle(out,dorsalCentre,innerPointA,innerPointB,palette.highlight);
                LitTriangle(out,innerPointA,middlePointA,middlePointB,palette.dorsal);
                LitTriangle(out,innerPointA,middlePointB,innerPointB,palette.dorsal);
                LitTriangle(out,middlePointA,outerPointA,outerPointB,palette.margin);
                LitTriangle(out,middlePointA,outerPointB,middlePointB,palette.margin);
                Triangle(out,ventralCentre,
                    DiscPoint(basis,outerB[0],outerB[1],ray.wingWave,-1.0f),
                    DiscPoint(basis,outerA[0],outerA[1],ray.wingWave,-1.0f),
                    palette.underside);
            }

            // The disc supplies flexible wings; a separate shallow loft gives the head and
            // shoulder girdle enough volume to remain readable from swimming height.
            constexpr int BodySegments=16;
            const Vector3 bodyCrown=basis.Point(0,.295f,.075f);
            for(int index=0;index<BodySegments;++index) {
                const float angleA=2.0f*std::numbers::pi_v<float>
                    *static_cast<float>(index)/static_cast<float>(BodySegments);
                const float angleB=2.0f*std::numbers::pi_v<float>
                    *static_cast<float>(index+1)/static_cast<float>(BodySegments);
                const auto bodyPoint=[&](const float angle,const bool upper) {
                    const float x=std::cos(angle)*(upper?.255f:.43f);
                    const float z=.075f+std::sin(angle)*(upper?.41f:.61f);
                    if(!upper)return DiscPoint(basis,x,z,ray.wingWave,1.0f)
                        +basis.up*(.006f*basis.scale);
                    const float y=.205f+.022f*std::sin(angle)
                        +.015f*std::cos(angle)*std::cos(angle);
                    return basis.Point(x,y,z);
                };
                const Vector3 baseA=bodyPoint(angleA,false),baseB=bodyPoint(angleB,false);
                const Vector3 topA=bodyPoint(angleA,true),topB=bodyPoint(angleB,true);
                LitTriangle(out,baseA,topA,topB,palette.dorsal);
                LitTriangle(out,baseA,topB,baseB,palette.dorsal);
                LitTriangle(out,topA,bodyCrown,topB,palette.highlight);
            }

            for(const auto& spot:std::array<std::array<float,3>,6>{{
                    {{-.52f,.18f,.055f}},{{.52f,.18f,.055f}},{{-.69f,-.14f,.044f}},
                    {{.69f,-.14f,.044f}},{{-.27f,-.32f,.040f}},{{.27f,-.32f,.040f}}}}) {
                const Vector3 centre=DiscPoint(basis,spot[0],spot[1],ray.wingWave,1.12f)
                    +basis.up*(.004f*basis.scale);
                constexpr int SpotSegments=6;
                for(int segment=0;segment<SpotSegments;++segment) {
                    const float angleA=2.0f*std::numbers::pi_v<float>*segment/SpotSegments;
                    const float angleB=2.0f*std::numbers::pi_v<float>*(segment+1)/SpotSegments;
                    const auto point=[&](float angle) {
                        return centre+basis.right*(std::cos(angle)*spot[2]*basis.scale)
                            +basis.forward*(std::sin(angle)*spot[2]*.72f*basis.scale);
                    };
                    Triangle(out,centre,point(angleA),point(angleB),palette.spot);
                }
            }

            for(const float side:{-1.0f,1.0f}) {
                const Vector3 eyeTop=basis.Point(side*.19f,.332f,.46f);
                constexpr int EyeSegments=6;
                for(int segment=0;segment<EyeSegments;++segment) {
                    const float angleA=2.0f*std::numbers::pi_v<float>*segment/EyeSegments;
                    const float angleB=2.0f*std::numbers::pi_v<float>*(segment+1)/EyeSegments;
                    const auto eyeRing=[&](float angle,bool middle) {
                        const float xRadius=middle?.044f:.066f;
                        const float zRadius=middle?.034f:.052f;
                        return basis.Point(side*.19f+std::cos(angle)*xRadius,
                                           middle?.310f:.268f,
                                           .46f+std::sin(angle)*zRadius);
                    };
                    const Vector3 baseA=eyeRing(angleA,false),baseB=eyeRing(angleB,false);
                    const Vector3 middleA=eyeRing(angleA,true),middleB=eyeRing(angleB,true);
                    LitTriangle(out,baseA,middleA,middleB,palette.margin);
                    LitTriangle(out,baseA,middleB,baseB,palette.margin);
                    LitTriangle(out,middleA,eyeTop,middleB,palette.margin);
                }
                const Vector3 pupilCentre=basis.Point(side*.19f,.335f,.472f);
                const Vector3 pupilRight=basis.right*(.020f*basis.scale);
                const Vector3 pupilForward=basis.forward*(.014f*basis.scale);
                Triangle(out,pupilCentre+pupilForward,pupilCentre+pupilRight,
                         pupilCentre-pupilForward,palette.eye);
                Triangle(out,pupilCentre+pupilForward,pupilCentre-pupilForward,
                         pupilCentre-pupilRight,palette.eye);
                const Vector3 spiracle=basis.Point(side*.22f,.222f,.285f);
                const Vector3 slotSide=basis.right*(side*.055f*basis.scale);
                const Vector3 slotFore=basis.forward*(.025f*basis.scale);
                Triangle(out,spiracle-slotSide-slotFore,spiracle+slotSide-slotFore,
                         spiracle+slotSide+slotFore,palette.margin);
                Triangle(out,spiracle-slotSide-slotFore,spiracle+slotSide+slotFore,
                         spiracle-slotSide+slotFore,palette.margin);
            }

            Vector3 tailRoot=basis.Point(0,.015f,-.61f);
            constexpr std::array<float,5> TailLengths{{.30f,.33f,.36f,.39f,.42f}};
            float tailZ=-.61f;
            for(std::size_t index=0;index<TailLengths.size();++index) {
                const float length=TailLengths[index];
                const float nextZ=tailZ-length;
                const float sway=std::sin(ray.wingWave*1.2f+static_cast<float>(index)*.65f)
                    *.025f*static_cast<float>(index);
                const Vector3 next=basis.Point(sway,.01f,nextZ);
                const float width=(.066f-static_cast<float>(index)*.009f)*basis.scale;
                const float nextWidth=std::max(.014f,
                    .066f-static_cast<float>(index+1)*.009f)*basis.scale;
                const Vector3 side=basis.right*width,nextSide=basis.right*nextWidth;
                DoubleTriangle(out,tailRoot+side,tailRoot-side,next+nextSide,palette.tail);
                DoubleTriangle(out,tailRoot-side,next-nextSide,next+nextSide,palette.tail);
                const Vector3 vertical=basis.up*(width*.72f);
                const Vector3 nextVertical=basis.up*(nextWidth*.72f);
                const Color lower=ScaleColor(palette.tail,.82f);
                DoubleTriangle(out,tailRoot+vertical,tailRoot-vertical,
                               next+nextVertical,lower);
                DoubleTriangle(out,tailRoot-vertical,next-nextVertical,
                               next+nextVertical,lower);
                tailRoot=next;tailZ=nextZ;
            }
            const Vector3 barb=basis.Point(.11f,.025f,-1.33f);
            DoubleTriangle(out,basis.Point(0,.02f,-1.22f),barb,
                           basis.Point(0,.02f,-1.47f),palette.margin);
        }

        void AppendFar(std::vector<VertexPositionColor>& out, const StingrayState& ray)
        {
            const Palette& palette=Palettes[ray.colorVariant%Palettes.size()];
            const Basis basis=MakeBasis(ray);
            const Vector3 nose=basis.Point(0,.08f,.94f);
            const Vector3 left=basis.Point(-1.18f,ray.wingWave*.16f,-.02f);
            const Vector3 rear=basis.Point(0,.03f,-.70f);
            const Vector3 right=basis.Point(1.18f,ray.wingWave*.16f,-.02f);
            DoubleTriangle(out,nose,left,rear,palette.dorsal);
            DoubleTriangle(out,nose,rear,right,palette.dorsal);
            DoubleTriangle(out,rear,basis.Point(.035f,0,-1.72f),
                           basis.Point(-.035f,0,-1.72f),palette.tail);
        }
    }

    StingrayRenderer::StingrayRenderer(GraphicsDevice& device, const float maximumDistance)
        : device_(device),effect_(std::make_unique<BasicEffect>(device)),
          vertices_(std::make_unique<DynamicVertexBuffer>(device,
              VertexPositionColor::getVertexDeclarationStatic(),MaximumVertices,
              BufferUsage::WriteOnly)),maximumDistance_(maximumDistance)
    {
        if(!std::isfinite(maximumDistance_)||maximumDistance_<=0.0f)
            throw std::invalid_argument("Stingray maximum distance must be finite and positive");
        effect_->setVertexColorEnabledProperty(true);
        effect_->setLightingEnabledProperty(false);
        effect_->setFogEnabledProperty(true);
        effect_->setFogColorProperty(Vector3(environment::FogColorRed,
            environment::FogColorGreen,environment::FogColorBlue));
        effect_->setFogStartProperty(165.0f);effect_->setFogEndProperty(340.0f);
        staging_.reserve(MaximumVertices);
        constexpr auto diagnostic=
            "island-simulator: stingrays use broad undulating discs, raised eyes, dorsal spots, pale undersides, and long tapered tails in one draw";
        std::cout<<diagnostic<<'\n';MirrorRuntimeDiagnosticToPlatformLog(diagnostic);
        std::cout<<StingrayProfileDiagnostic<<'\n';
        MirrorRuntimeDiagnosticToPlatformLog(StingrayProfileDiagnostic);
        std::cout<<StingrayGeometryDiagnostic<<'\n';
        MirrorRuntimeDiagnosticToPlatformLog(StingrayGeometryDiagnostic);
        std::cout<<StingrayAnatomyDiagnostic<<'\n';
        MirrorRuntimeDiagnosticToPlatformLog(StingrayAnatomyDiagnostic);
    }

    StingrayRenderer::~StingrayRenderer()=default;

    void StingrayRenderer::Draw(const Matrix& view,const Matrix& projection,
                                const Vector3& cameraPosition,
                                const std::span<const StingrayState> rays)
    {
        staging_.clear();visibleCount_=nearCount_=farCount_=drawCallCount_=0;
        const BoundingFrustum frustum(view*projection);
        for(const StingrayState& ray:rays) {
            const Vector3 centre(ray.x,ray.y,ray.z);
            const float distanceSquared=Vector3::DistanceSquared(centre,cameraPosition);
            if(distanceSquared>maximumDistance_*maximumDistance_
                ||!frustum.Intersects(BoundingSphere(centre,2.4f*ray.scale))) continue;
            if(distanceSquared<=NearDistance*NearDistance){AppendNear(staging_,ray);++nearCount_;}
            else{AppendFar(staging_,ray);++farCount_;}
            ++visibleCount_;
        }
        if(staging_.empty())return;
        device_.setRasterizerStateProperty(RasterizerState::CullNone);
        device_.setBlendStateProperty(BlendState::Opaque);
        device_.setDepthStencilStateProperty(DepthStencilState::Default);
        vertices_->SetData(staging_.data(),0,static_cast<int>(staging_.size()),
                           SetDataOptions::Discard);
        device_.SetVertexBuffer(vertices_.get());device_.setIndicesProperty(nullptr);
        effect_->setWorldProperty(Matrix::getIdentityProperty());effect_->setViewProperty(view);
        effect_->setProjectionProperty(projection);effect_->Apply();
        device_.DrawPrimitives(PrimitiveType::TriangleList,0,
                               static_cast<int>(staging_.size()/3U));
        ++drawCallCount_;
    }
}
