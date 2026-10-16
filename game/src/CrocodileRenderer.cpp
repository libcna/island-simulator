#include "IslandSimulator/CrocodileRenderer.hpp"

#include "IslandSimulator/EnvironmentLighting.hpp"
#include "IslandSimulator/RuntimeDiagnostics.hpp"
#include "IslandSimulator/WildlifeRenderLighting.hpp"

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
        constexpr float NearLodDistance = 86.0f;
        // Rounded multi-station jaws and the low articulated armor stay bounded but
        // need deterministic headroom beyond the former wedge-based 5,200 budget.
        constexpr int MaximumVerticesPerCrocodile = 6500;
        constexpr int MaximumVertexCount = static_cast<int>(CrocodileGroup::CrocodileCount)
            * MaximumVerticesPerCrocodile;
        constexpr std::string_view FormDiagnostic =
            "island-simulator: crocodiles use long low armored bodies, broad skulls, muscular tails, and planted splayed limbs";
        constexpr std::string_view DetailDiagnostic =
            "island-simulator: near crocodiles use raised eyes, nostrils, separate jaws, teeth, dorsal osteoderms, and wet mottled facet shading, with multi-section rounded snouts and lateral eyes";
        constexpr std::string_view ArmorDiagnostic =
            "island-simulator: crocodile armor uses low paired trunk scutes that merge into a diminishing articulated tail keel";
        constexpr std::string_view SkinDiagnostic =
            "island-simulator: near crocodile torso, tail, skull, jaw, and limb roots use bounded reticulated facet pigment";
        constexpr std::string_view SurfaceDiagnostic =
            "island-simulator: near crocodiles use finer rounded torso, shoulder, skull, and nasal surfaces within the same draw";

        struct Palette
        {
            Color back, side, belly, scute, jaw, mouth, tooth, eye, pupil;
        };

        const std::array<Palette, 3> Palettes = {{
            {Color(74, 83, 48), Color(86, 91, 55), Color(150, 142, 92),
             Color(53, 61, 36), Color(105, 104, 63), Color(87, 37, 29),
             Color(224, 212, 166), Color(184, 157, 78), Color(18, 16, 10)},
            {Color(65, 76, 43), Color(82, 87, 49), Color(143, 133, 83),
             Color(45, 55, 31), Color(98, 98, 57), Color(81, 34, 27),
             Color(218, 207, 165), Color(171, 143, 70), Color(15, 14, 9)},
            {Color(82, 78, 44), Color(98, 91, 51), Color(157, 139, 83),
             Color(57, 55, 30), Color(113, 101, 57), Color(91, 38, 28),
             Color(225, 213, 169), Color(190, 159, 76), Color(17, 15, 8)}
        }};

        [[nodiscard]] Color ScalePigment(const Color& color,const float scale)
        {
            const auto channel=[scale](const int value) {
                return static_cast<int>(std::clamp(std::lround(value*scale),0L,255L));
            };
            return Color(channel(color.getRProperty()),channel(color.getGProperty()),
                         channel(color.getBProperty()),static_cast<int>(color.getAProperty()));
        }

        struct Axes { Vector3 right, up, forward; };

        [[nodiscard]] Axes MakeAxes(const CrocodileState& crocodile)
        {
            const Vector3 forward(std::cos(crocodile.headingRadians), 0.0f,
                                  std::sin(crocodile.headingRadians));
            return Axes{Vector3(-forward.Z, 0.0f, forward.X), Vector3::Up, forward};
        }

        [[nodiscard]] Vector3 Local(const Vector3& centre, const Axes& axes,
                                    const float x, const float y, const float z,
                                    const float scale)
        {
            return centre + axes.right * (x * scale) + axes.up * (y * scale)
                + axes.forward * (z * scale);
        }

        void AddTriangle(std::vector<VertexPositionColor>& vertices,
                         const Vector3& a, const Vector3& b, const Vector3& c,
                         const Color& color, const bool twoSided = false)
        {
            const Color lit = wildlife_render::FacetColor(a, b, c, color, twoSided);
            vertices.emplace_back(a, lit);
            vertices.emplace_back(b, lit);
            vertices.emplace_back(c, lit);
        }

        void AddQuad(std::vector<VertexPositionColor>& vertices,
                     const Vector3& a, const Vector3& b,
                     const Vector3& c, const Vector3& d, const Color& color)
        {
            AddTriangle(vertices, a, b, c, color);
            AddTriangle(vertices, a, c, d, color);
        }

        void AddDoubleTriangle(std::vector<VertexPositionColor>& vertices,
                               const Vector3& a, const Vector3& b, const Vector3& c,
                               const Color& color)
        {
            AddTriangle(vertices, a, b, c, color, true);
            AddTriangle(vertices, c, b, a, color, true);
        }

        void AppendEllipsoid(std::vector<VertexPositionColor>& vertices,
                             const Vector3& centre, const Axes& axes, const float scale,
                             const float radiusX, const float radiusY, const float radiusZ,
                             const Color& upper, const Color& lower,
                             const int rings = 5, const int segments = 12,
                             const float pigmentVariation = 0.0f,
                             const int pigmentSeed = 0)
        {
            for (int ring=0; ring<rings; ++ring) {
                const float v0=-.5f*std::numbers::pi_v<float>
                    +std::numbers::pi_v<float>*static_cast<float>(ring)/static_cast<float>(rings);
                const float v1=-.5f*std::numbers::pi_v<float>
                    +std::numbers::pi_v<float>*static_cast<float>(ring+1)/static_cast<float>(rings);
                for(int segment=0;segment<segments;++segment) {
                    const float u0=2.0f*std::numbers::pi_v<float>*static_cast<float>(segment)
                        /static_cast<float>(segments);
                    const float u1=2.0f*std::numbers::pi_v<float>*static_cast<float>(segment+1)
                        /static_cast<float>(segments);
                    const auto surface=[&](const float u,const float v) {
                        return Local(centre,axes,std::cos(v)*std::cos(u)*radiusX,
                            std::sin(v)*radiusY,std::cos(v)*std::sin(u)*radiusZ,scale);
                    };
                    const Color base=(v0+v1)*.5f>0?upper:lower;
                    // Two incommensurate row/column waves produce bounded irregular
                    // quadrilateral scale fields without another texture, vertex or draw.
                    const float patch=.68f*std::sin((ring+1)*2.17f+(segment+1)*3.71f
                                                    +pigmentSeed*.91f)
                        +.32f*std::cos((ring+1)*4.03f-(segment+1)*1.43f
                                      +pigmentSeed*1.27f);
                    const Color color=ScalePigment(base,1.0f+pigmentVariation*patch);
                    AddQuad(vertices,surface(u0,v0),surface(u0,v1),surface(u1,v1),
                            surface(u1,v0),color);
                }
            }
        }

        void AppendTaperedSegment(std::vector<VertexPositionColor>& vertices,
                                  const Vector3& a, const Vector3& b,
                                  const float radiusA, const float radiusB,
                                  const Color& color, const int sides = 7)
        {
            Vector3 axis=b-a;
            if(axis.LengthSquared()<1.0e-8f)return;
            axis.Normalize();
            Vector3 side=Vector3::Cross(axis,Vector3::Up);
            if(side.LengthSquared()<1.0e-6f)side=Vector3::Right;
            else side.Normalize();
            Vector3 up=Vector3::Normalize(Vector3::Cross(side,axis));
            for(int i=0;i<sides;++i) {
                const float a0=2.0f*std::numbers::pi_v<float>*static_cast<float>(i)/static_cast<float>(sides);
                const float a1=2.0f*std::numbers::pi_v<float>*static_cast<float>(i+1)/static_cast<float>(sides);
                const Vector3 r0=side*std::cos(a0)+up*std::sin(a0);
                const Vector3 r1=side*std::cos(a1)+up*std::sin(a1);
                AddQuad(vertices,a+r0*radiusA,a+r1*radiusA,b+r1*radiusB,b+r0*radiusB,color);
            }
        }

        void AppendTailSegment(std::vector<VertexPositionColor>& vertices,
                               const Vector3& a,const Vector3& b,const Axes& axes,
                               const float widthA,const float heightA,
                               const float widthB,const float heightB,
                               const Color& color,const float pigmentVariation=0.0f,
                               const int pigmentSeed=0)
        {
            constexpr int Sides=12;
            for(int i=0;i<Sides;++i) {
                const float angle0=2.0f*std::numbers::pi_v<float>*static_cast<float>(i)
                    /static_cast<float>(Sides);
                const float angle1=2.0f*std::numbers::pi_v<float>*static_cast<float>(i+1)
                    /static_cast<float>(Sides);
                const Vector3 a0=a+axes.right*(std::cos(angle0)*widthA)
                    +axes.up*(std::sin(angle0)*heightA);
                const Vector3 a1=a+axes.right*(std::cos(angle1)*widthA)
                    +axes.up*(std::sin(angle1)*heightA);
                const Vector3 b0=b+axes.right*(std::cos(angle0)*widthB)
                    +axes.up*(std::sin(angle0)*heightB);
                const Vector3 b1=b+axes.right*(std::cos(angle1)*widthB)
                    +axes.up*(std::sin(angle1)*heightB);
                const float patch=.66f*std::sin((i+1)*2.43f+pigmentSeed*.83f)
                    +.34f*std::cos((i+1)*4.19f-pigmentSeed*1.31f);
                AddQuad(vertices,a0,a1,b1,b0,
                        ScalePigment(color,1.0f+pigmentVariation*patch));
            }
        }

        void AppendNearCrocodile(std::vector<VertexPositionColor>& vertices,
                                 const CrocodileState& crocodile)
        {
            const Palette& palette=Palettes[crocodile.colorVariant%Palettes.size()];
            const Axes axes=MakeAxes(crocodile);
            const Vector3 centre(crocodile.x,crocodile.y-.04f*crocodile.submerged,crocodile.z);
            const float scale=crocodile.scale;
            // Low barrel torso and heavy shoulder mass.
            AppendEllipsoid(vertices,Local(centre,axes,0,.34f,-.18f,scale),axes,scale,
                            .68f,.37f,1.28f,palette.back,palette.belly,8,18,.12f,3);
            AppendEllipsoid(vertices,Local(centre,axes,0,.35f,.72f,scale),axes,scale,
                            .63f,.32f,.72f,palette.back,palette.side,5,16,.10f,11);

            // The tapering tail bends visibly during swimming instead of reading as a rigid cone.
            Vector3 previous=Local(centre,axes,0,.27f,-1.18f,scale);
            float previousWidth=.48f*scale,previousHeight=.29f*scale;
            for(int section=1;section<=12;++section) {
                const float t=static_cast<float>(section)/12.0f;
                const float wave=std::sin(t*4.2f+crocodile.gaitPhase)*crocodile.bodyWave
                    *t*t*.55f;
                const Vector3 next=Local(centre,axes,wave,.22f-.17f*t,
                                         -1.10f-2.55f*t,scale);
                const float width=(.47f*(1.0f-t)+.025f)*scale;
                const float height=(.28f*(1.0f-t)+.018f)*scale;
                AppendTailSegment(vertices,previous,next,axes,previousWidth,previousHeight,
                                  width,height,palette.side,.10f,section);
                previous=next;previousWidth=width;previousHeight=height;
            }

            // Broad, shallow saltwater-crocodile skull and independently opening lower jaw.
            const Vector3 skull=Local(centre,axes,0,.39f,1.25f,scale);
            AppendEllipsoid(vertices,skull,axes,scale,.58f,.29f,.70f,
                            palette.back,palette.jaw,6,18,.10f,17);
            // Four shallow elliptical stations replace the old rectangular upper
            // wedge. They keep the broad saltwater-crocodile proportions while
            // rounding the cheek-to-muzzle transition and tapering the nasal tip.
            constexpr std::array<float,4> upperZ{{1.30f,1.68f,2.08f,2.45f}};
            constexpr std::array<float,4> upperY{{.40f,.405f,.39f,.365f}};
            constexpr std::array<float,4> upperWidth{{.51f,.49f,.43f,.35f}};
            constexpr std::array<float,4> upperHeight{{.20f,.18f,.15f,.115f}};
            for(std::size_t section=0;section+1<upperZ.size();++section) {
                AppendTailSegment(vertices,
                    Local(centre,axes,0,upperY[section],upperZ[section],scale),
                    Local(centre,axes,0,upperY[section+1],upperZ[section+1],scale),axes,
                    upperWidth[section]*scale,upperHeight[section]*scale,
                    upperWidth[section+1]*scale,upperHeight[section+1]*scale,palette.side,
                    .09f,20+static_cast<int>(section));
            }
            AppendEllipsoid(vertices,Local(centre,axes,0,.36f,2.43f,scale),axes,scale,
                            .355f,.12f,.17f,palette.side,palette.jaw,4,16,.08f,23);
            const float gape=crocodile.jawOpen;
            const Axes jawAxes{axes.right,axes.up,axes.forward};
            const Vector3 lowerJawCentre=centre-axes.up*(gape*.18f*scale);
            constexpr std::array<float,4> jawZ{{1.20f,1.62f,2.06f,2.43f}};
            constexpr std::array<float,4> jawY{{.205f,.205f,.195f,.185f}};
            constexpr std::array<float,4> jawWidth{{.48f,.46f,.41f,.345f}};
            constexpr std::array<float,4> jawHeight{{.105f,.10f,.087f,.072f}};
            for(std::size_t section=0;section+1<jawZ.size();++section) {
                AppendTailSegment(vertices,
                    Local(lowerJawCentre,jawAxes,0,jawY[section],jawZ[section],scale),
                    Local(lowerJawCentre,jawAxes,0,jawY[section+1],jawZ[section+1],scale),
                    jawAxes,jawWidth[section]*scale,jawHeight[section]*scale,
                    jawWidth[section+1]*scale,jawHeight[section+1]*scale,palette.jaw,
                    .07f,30+static_cast<int>(section));
            }
            if(gape>.012f) {
                const auto mouthPoint=[&](const float x,const float z) {
                    return Local(centre,axes,x,.295f-gape*.04f,z,scale);
                };
                AddQuad(vertices,mouthPoint(-.41f,1.19f),mouthPoint(-.41f,2.38f),
                        mouthPoint(.41f,2.38f),mouthPoint(.41f,1.19f),palette.mouth);
            }
            for(int sideIndex=-1;sideIndex<=1;sideIndex+=2) {
                const float side=static_cast<float>(sideIndex);
                const Vector3 rear=Local(centre,axes,side*.493f,.285f,1.28f,scale);
                const Vector3 front=Local(centre,axes,side*.365f,.285f-gape*.04f,2.40f,scale);
                const Vector3 rearLow=rear-axes.up*(.025f*scale);
                const Vector3 frontLow=front-axes.up*(.025f*scale);
                AddQuad(vertices,rear,rearLow,frontLow,front,palette.mouth);
            }
            for(int sideIndex=-1;sideIndex<=1;sideIndex+=2) {
                const float side=static_cast<float>(sideIndex);
                for(int tooth=0;tooth<6;++tooth) {
                    const float z=1.32f+static_cast<float>(tooth)*.19f;
                    const Vector3 root=Local(centre,axes,side*(.44f-.025f*tooth),
                                             .29f-gape*.04f,z,scale);
                    const Vector3 tip=Local(centre,axes,side*(.42f-.024f*tooth),
                                            .19f-gape*.13f,z+.025f,scale);
                    AddDoubleTriangle(vertices,root-axes.forward*.035f*scale,
                                      root+axes.forward*.035f*scale,tip,palette.tooth);
                }
                const Vector3 eye=Local(centre,axes,side*.40f,.615f,1.24f,scale);
                AppendEllipsoid(vertices,eye,axes,scale,.10f,.058f,.12f,
                                palette.scute,palette.side,3,8);
                // Crocodile pupils face laterally, not forward along the snout.
                // Embedding the thin axis into the socket also removes the old
                // stalk-eyed toy silhouette in broadside views.
                const Vector3 iris=eye+axes.right*(side*.092f*scale)+axes.up*.006f*scale;
                AppendEllipsoid(vertices,iris,axes,scale,.018f,.036f,.038f,
                                palette.eye,palette.eye,3,8);
                const Vector3 pupil=iris+axes.right*(side*.017f*scale);
                AppendEllipsoid(vertices,pupil,axes,scale,.009f,.027f,.012f,
                                palette.pupil,palette.pupil,2,7);
                const Vector3 nostril=Local(centre,axes,side*.21f,.505f,2.32f,scale);
                AppendEllipsoid(vertices,nostril,axes,scale,.040f,.018f,.028f,
                                palette.pupil,palette.pupil,2,7);
            }

            // Low paired trunk osteoderms replace the old repeated saw-tooth peaks.
            // Four-sided keeled plates follow the changing back line; they remain
            // readable broadside without turning a crocodile into a spined dinosaur.
            for(int row=0;row<11;++row) {
                const float z=-1.04f+static_cast<float>(row)*.22f;
                const float width=.22f+.055f*std::sin(static_cast<float>(row)*.61f);
                const float backY=.665f+.024f*std::cos((z+.12f)*2.1f);
                for(int sideIndex=-1;sideIndex<=1;sideIndex+=2) {
                    const float x=static_cast<float>(sideIndex)*width;
                    const Vector3 rearLeft=Local(centre,axes,x-.070f,backY,z-.075f,scale);
                    const Vector3 rearRight=Local(centre,axes,x+.070f,backY,z-.075f,scale);
                    const Vector3 frontRight=Local(centre,axes,x+.064f,backY,z+.075f,scale);
                    const Vector3 frontLeft=Local(centre,axes,x-.064f,backY,z+.075f,scale);
                    const Vector3 peak=Local(centre,axes,x,backY+.066f,z+.006f,scale);
                    AddTriangle(vertices,rearLeft,rearRight,peak,palette.scute);
                    AddTriangle(vertices,rearRight,frontRight,peak,palette.scute);
                    AddTriangle(vertices,frontRight,frontLeft,peak,palette.scute);
                    AddTriangle(vertices,frontLeft,rearLeft,peak,palette.scute);
                }
            }
            // Behind the hips the paired rows merge into one progressively smaller
            // keel which follows the exact same animated tail wave as the volume.
            for(int row=0;row<8;++row) {
                const float t=(static_cast<float>(row)+.65f)/9.0f;
                const float z=-1.10f-2.55f*t;
                const float wave=std::sin(t*4.2f+crocodile.gaitPhase)*crocodile.bodyWave
                    *t*t*.55f;
                const float centreY=.22f-.17f*t;
                const float tailHeight=.28f*(1.0f-t)+.018f;
                const float halfWidth=std::lerp(.070f,.018f,t);
                const float halfLength=std::lerp(.090f,.038f,t);
                const float baseY=centreY+tailHeight*.88f;
                const Vector3 rear=Local(centre,axes,wave,baseY,z-halfLength,scale);
                const Vector3 right=Local(centre,axes,wave+halfWidth,baseY,z,scale);
                const Vector3 front=Local(centre,axes,wave,baseY,z+halfLength,scale);
                const Vector3 left=Local(centre,axes,wave-halfWidth,baseY,z,scale);
                const Vector3 peak=Local(centre,axes,wave,baseY+.055f*(1.0f-.65f*t),z,scale);
                AddTriangle(vertices,rear,right,peak,palette.scute);
                AddTriangle(vertices,right,front,peak,palette.scute);
                AddTriangle(vertices,front,left,peak,palette.scute);
                AddTriangle(vertices,left,rear,peak,palette.scute);
            }

            // Four short, muscular, splayed legs with alternating planted gait.
            for(int pair=0;pair<2;++pair) {
                const float rootZ=pair==0?.64f:-.66f;
                for(int sideIndex=-1;sideIndex<=1;sideIndex+=2) {
                    const float side=static_cast<float>(sideIndex);
                    const float stride=std::sin(crocodile.gaitPhase
                        +(pair*2+(sideIndex>0?1:0))*std::numbers::pi_v<float>)*.13f
                        *(1.0f-crocodile.submerged);
                    const Vector3 shoulder=Local(centre,axes,side*.52f,.34f,rootZ,scale);
                    const Vector3 elbow=Local(centre,axes,side*.83f,.18f,rootZ+stride,scale);
                    const Vector3 wrist=Local(centre,axes,side*.91f,.02f,rootZ+.24f+stride,scale);
                    // A low pectoral/hip mass hides the old tube-to-barrel seam and
                    // gives the splayed limb a muscular crocodilian root. It remains
                    // centred on the existing animated joint and adds no simulation state.
                    const float rootWidth=(pair==0?.24f:.27f)*scale;
                    const float rootHeight=(pair==0?.14f:.16f)*scale;
                    const float rootLength=(pair==0?.28f:.30f)*scale;
                    AppendEllipsoid(vertices,shoulder,axes,1.0f,rootWidth,rootHeight,rootLength,
                                    palette.back,palette.side,3,8,.09f,
                                    41+pair*2+(sideIndex>0?1:0));
                    AppendTaperedSegment(vertices,shoulder,elbow,.17f*scale,.125f*scale,palette.side,8);
                    AppendTaperedSegment(vertices,elbow,wrist,.125f*scale,.075f*scale,palette.side,8);
                    for(int toe=0;toe<3;++toe) {
                        const float toeSide=(static_cast<float>(toe)-1.0f)*.075f;
                        const Vector3 toeTip=wrist+axes.forward*(.25f*scale)
                            +axes.right*(toeSide*scale)+axes.up*(.015f*scale);
                        AppendTaperedSegment(vertices,wrist,toeTip,.035f*scale,.012f*scale,
                                             palette.scute,5);
                    }
                }
            }

            if(static_cast<int>(vertices.size())>MaximumVertexCount)
                throw std::logic_error("Crocodile geometry exceeded its shared vertex budget");
        }

        void AppendFarCrocodile(std::vector<VertexPositionColor>& vertices,
                                const CrocodileState& crocodile)
        {
            const Palette& palette=Palettes[crocodile.colorVariant%Palettes.size()];
            const Axes axes=MakeAxes(crocodile);
            const Vector3 centre(crocodile.x,crocodile.y,crocodile.z);
            const float s=crocodile.scale;
            const Vector3 nose=Local(centre,axes,0,.35f,2.35f,s);
            const Vector3 headLeft=Local(centre,axes,-.50f,.34f,1.25f,s);
            const Vector3 headRight=Local(centre,axes,.50f,.34f,1.25f,s);
            const Vector3 hipLeft=Local(centre,axes,-.55f,.30f,-.82f,s);
            const Vector3 hipRight=Local(centre,axes,.55f,.30f,-.82f,s);
            const Vector3 tail=Local(centre,axes,crocodile.bodyWave*.22f,.12f,-3.45f,s);
            AddDoubleTriangle(vertices,nose,headLeft,headRight,palette.back);
            AddDoubleTriangle(vertices,headLeft,hipLeft,hipRight,palette.back);
            AddDoubleTriangle(vertices,headLeft,hipRight,headRight,palette.back);
            AddDoubleTriangle(vertices,hipLeft,tail,hipRight,palette.side);
        }
    }

    CrocodileRenderer::CrocodileRenderer(GraphicsDevice& device,const float maximumDistance)
        :device_(device),effect_(std::make_unique<BasicEffect>(device)),
         vertices_(std::make_unique<DynamicVertexBuffer>(device,
             VertexPositionColor::getVertexDeclarationStatic(),MaximumVertexCount,
             BufferUsage::WriteOnly)),maximumDistance_(maximumDistance)
    {
        if(!std::isfinite(maximumDistance_)||maximumDistance_<=0)
            throw std::invalid_argument("Crocodile maximum draw distance must be finite and positive");
        effect_->setVertexColorEnabledProperty(true);
        effect_->setLightingEnabledProperty(false);
        effect_->setFogEnabledProperty(true);
        effect_->setFogColorProperty(Vector3(environment::FogColorRed,environment::FogColorGreen,
                                              environment::FogColorBlue));
        effect_->setFogStartProperty(135.0f);
        effect_->setFogEndProperty(260.0f);
        stagingVertices_.reserve(MaximumVertexCount);
        std::cout<<FormDiagnostic<<'\n'<<DetailDiagnostic<<'\n'<<ArmorDiagnostic<<'\n'
                 <<SkinDiagnostic<<'\n'<<SurfaceDiagnostic<<'\n';
        MirrorRuntimeDiagnosticToPlatformLog(FormDiagnostic);
        MirrorRuntimeDiagnosticToPlatformLog(DetailDiagnostic);
        MirrorRuntimeDiagnosticToPlatformLog(ArmorDiagnostic);
        MirrorRuntimeDiagnosticToPlatformLog(SkinDiagnostic);
        MirrorRuntimeDiagnosticToPlatformLog(SurfaceDiagnostic);
    }

    CrocodileRenderer::~CrocodileRenderer()=default;

    void CrocodileRenderer::Draw(const Matrix& view,const Matrix& projection,
                                  const Vector3& cameraPosition,
                                  const std::span<const CrocodileState> crocodiles)
    {
        stagingVertices_.clear();visibleCount_=visibleNearCount_=visibleFarCount_=drawCallCount_=0;
        const BoundingFrustum frustum(view*projection);
        for(const CrocodileState& crocodile:crocodiles) {
            const Vector3 centre(crocodile.x,crocodile.y+.35f*crocodile.scale,crocodile.z);
            const float distanceSquared=Vector3::DistanceSquared(centre,cameraPosition);
            if(distanceSquared>maximumDistance_*maximumDistance_
               ||!frustum.Intersects(BoundingSphere(centre,4.3f*crocodile.scale)))continue;
            const std::size_t first=stagingVertices_.size();
            if(distanceSquared<=NearLodDistance*NearLodDistance) {
                AppendNearCrocodile(stagingVertices_,crocodile);++visibleNearCount_;
            } else {AppendFarCrocodile(stagingVertices_,crocodile);++visibleFarCount_;}
            if(stagingVertices_.size()-first>MaximumVerticesPerCrocodile)
                throw std::logic_error("One crocodile exceeded its vertex budget");
            ++visibleCount_;
        }
        if(stagingVertices_.empty())return;
        device_.setRasterizerStateProperty(RasterizerState::CullNone);
        device_.setBlendStateProperty(BlendState::Opaque);
        device_.setDepthStencilStateProperty(DepthStencilState::Default);
        vertices_->SetData(stagingVertices_.data(),0,static_cast<int>(stagingVertices_.size()),
                           SetDataOptions::Discard);
        device_.SetVertexBuffer(vertices_.get());device_.setIndicesProperty(nullptr);
        effect_->setWorldProperty(Matrix::getIdentityProperty());effect_->setViewProperty(view);
        effect_->setProjectionProperty(projection);effect_->Apply();
        device_.DrawPrimitives(PrimitiveType::TriangleList,0,
                               static_cast<int>(stagingVertices_.size()/3U));
        ++drawCallCount_;
    }
}
