#include "IslandSimulator/EnvironmentLighting.hpp"
#include "IslandSimulator/BirdRenderer.hpp"
#include "IslandSimulator/WildlifeRenderLighting.hpp"

#include "IslandSimulator/RuntimeDiagnostics.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
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
        constexpr float NearLodDistance = 255.0f;
        constexpr int MaximumVerticesPerBird = 1602;
        constexpr int MaximumVertexCount = static_cast<int>(BirdFlock::BirdCount)
            * MaximumVerticesPerBird;
        constexpr std::string_view NearBirdFeatherDiagnostic =
            "island-simulator: near birds use layered trailing flight-feather silhouettes";
        constexpr std::string_view BirdPerchSilhouetteDiagnostic =
            "island-simulator: perched birds fold wings and add near feet silhouettes";
        constexpr std::string_view NearBirdHeadDiagnostic =
            "island-simulator: near birds use closed lofted bodies, multi-ring heads, fitted beaks, and lateral eyes";
        constexpr std::string_view TropicalBirdDiagnostic =
            "island-simulator: canopy flock includes hooked-beak long-tailed parrots and compact tropical fruit doves";
        constexpr std::string_view NearBirdProfileDiagnostic =
            "island-simulator: near birds use compact rounded heads, camera-facing six-facet eyes, fitted hooked beaks, solid swept wings, and split tail feathers";
        constexpr std::string_view GroundDoveAnatomyDiagnostic =
            "island-simulator: ground fruit doves use plump bodies, rounded folded wings, fitted eye rings, and solid planted toes";

        struct BirdPalette
        {
            Color body;
            Color upperWing;
            Color lowerWing;
            Color tail;
            Color head;
            Color beak;
            Color eye;
        };

        const std::array<BirdPalette, 3> Palettes = {{
            {Color(61, 70, 70, 255), Color(104, 113, 108, 255),
             Color(214, 218, 203, 255), Color(48, 57, 58, 255),
             Color(161, 168, 164, 255), Color(194, 145, 57, 255), Color(15, 14, 12, 255)},
            {Color(90, 72, 54, 255), Color(139, 117, 82, 255),
             Color(222, 204, 161, 255), Color(67, 56, 46, 255),
             Color(130, 105, 73, 255), Color(184, 127, 47, 255), Color(14, 12, 9, 255)},
            {Color(50, 62, 72, 255), Color(78, 99, 114, 255),
             Color(195, 211, 215, 255), Color(39, 50, 59, 255),
             Color(104, 124, 133, 255), Color(177, 122, 45, 255), Color(12, 13, 14, 255)}
        }};

        const std::array<BirdPalette, 2> ParrotPalettes = {{
            {Color(48, 130, 58, 255), Color(28, 96, 48, 255),
             Color(210, 176, 40, 255), Color(26, 82, 47, 255),
             Color(196, 54, 43, 255), Color(203, 187, 145, 255), Color(13, 12, 9, 255)},
            {Color(57, 140, 65, 255), Color(35, 92, 118, 255),
             Color(218, 182, 46, 255), Color(27, 84, 82, 255),
             Color(224, 174, 43, 255), Color(59, 53, 47, 255), Color(12, 11, 8, 255)}
        }};

        const std::array<BirdPalette, 2> DovePalettes = {{
            {Color(91, 137, 84, 255), Color(48, 102, 62, 255),
             Color(181, 191, 150, 255), Color(45, 88, 59, 255),
             Color(158, 86, 128, 255), Color(78, 70, 64, 255), Color(43, 20, 18, 255)},
            {Color(99, 143, 94, 255), Color(54, 108, 69, 255),
             Color(201, 196, 163, 255), Color(49, 92, 65, 255),
             Color(111, 126, 144, 255), Color(83, 72, 62, 255), Color(47, 21, 17, 255)}
        }};

        [[nodiscard]] const BirdPalette& PaletteForBird(const BirdState& bird)
        {
            if (bird.species == BirdSpecies::Parrot) {
                return ParrotPalettes[bird.colorVariant % ParrotPalettes.size()];
            }
            if (bird.species == BirdSpecies::FruitDove) {
                return DovePalettes[bird.colorVariant % DovePalettes.size()];
            }
            return Palettes[bird.colorVariant % Palettes.size()];
        }

        void AddTriangle(std::vector<VertexPositionColor>& vertices,
                         const Vector3& a, const Vector3& b, const Vector3& c,
                         const Color& color)
        {
            vertices.emplace_back(a, color);
            vertices.emplace_back(b, color);
            vertices.emplace_back(c, color);
        }

        void AddDoubleSidedTriangle(std::vector<VertexPositionColor>& vertices,
                                    const Vector3& a, const Vector3& b, const Vector3& c,
                                    const Color& color)
        {
            AddTriangle(vertices, a, b, c, color);
            AddTriangle(vertices, c, b, a, color);
        }

        void AppendEllipsoid(std::vector<VertexPositionColor>& vertices,
                             const Vector3& centre,const Vector3& right,
                             const Vector3& up,const Vector3& forward,
                             const float radiusX,const float radiusY,const float radiusZ,
                             const Color& upper,const Color& lower,
                             const int rings=4,const int segments=10)
        {
            constexpr float Pi=3.14159265359f;
            for(int ring=0;ring<rings;++ring) {
                const float v0=-.5f*Pi+Pi*static_cast<float>(ring)/rings;
                const float v1=-.5f*Pi+Pi*static_cast<float>(ring+1)/rings;
                for(int segment=0;segment<segments;++segment) {
                    const float u0=2.0f*Pi*static_cast<float>(segment)/segments;
                    const float u1=2.0f*Pi*static_cast<float>(segment+1)/segments;
                    const auto point=[&](const float u,const float v) {
                        return centre+right*(std::cos(v)*std::cos(u)*radiusX)
                            +up*(std::sin(v)*radiusY)
                            +forward*(std::cos(v)*std::sin(u)*radiusZ);
                    };
                    const Vector3 a=point(u0,v0),b=point(u0,v1);
                    const Vector3 c=point(u1,v1),d=point(u1,v0);
                    const Color base=(v0+v1)*.5f>0.0f?upper:lower;
                    AddTriangle(vertices,a,b,c,
                        wildlife_render::FacetColor(a,b,c,base,true));
                    AddTriangle(vertices,a,c,d,
                        wildlife_render::FacetColor(a,c,d,base,true));
                }
            }
        }

        void AppendTaperedSegment(std::vector<VertexPositionColor>& vertices,
                                  const Vector3& start,const Vector3& end,
                                  const float startRadius,const float endRadius,
                                  const Color& color,const int sides=5)
        {
            Vector3 axis=end-start;
            if(axis.LengthSquared()<1.0e-8f)return;
            axis.Normalize();
            Vector3 side=Vector3::Cross(axis,Vector3::Up);
            if(side.LengthSquared()<1.0e-6f)side=Vector3::Right;
            else side.Normalize();
            const Vector3 normal=Vector3::Normalize(Vector3::Cross(side,axis));
            constexpr float Pi=3.14159265359f;
            for(int index=0;index<sides;++index) {
                const float a0=2.0f*Pi*static_cast<float>(index)/sides;
                const float a1=2.0f*Pi*static_cast<float>(index+1)/sides;
                const Vector3 r0=side*std::cos(a0)+normal*std::sin(a0);
                const Vector3 r1=side*std::cos(a1)+normal*std::sin(a1);
                const Vector3 a=start+r0*startRadius,b=start+r1*startRadius;
                const Vector3 c=end+r1*endRadius,d=end+r0*endRadius;
                AddTriangle(vertices,a,b,c,
                    wildlife_render::FacetColor(a,b,c,color,true));
                AddTriangle(vertices,a,c,d,
                    wildlife_render::FacetColor(a,c,d,color,true));
            }
        }

        template<std::size_t Count>
        void AppendClosedSurface(std::vector<VertexPositionColor>& vertices,
                                 const std::array<Vector3, Count>& outline,
                                 const Vector3& halfThickness,
                                 const Color& upper, const Color& lower)
        {
            static_assert(Count>=3U);
            std::array<Vector3, Count> top{}, bottom{};
            for(std::size_t point=0;point<outline.size();++point) {
                top[point]=outline[point]+halfThickness;
                bottom[point]=outline[point]-halfThickness;
            }
            for(std::size_t point=1;point+1<outline.size();++point) {
                AddTriangle(vertices,top[0],top[point],top[point+1],
                    wildlife_render::FacetColor(top[0],top[point],top[point+1],upper,true));
                AddTriangle(vertices,bottom[0],bottom[point+1],bottom[point],
                    wildlife_render::FacetColor(bottom[0],bottom[point+1],bottom[point],lower,true));
            }
            const Color edge=wildlife_render::ScaleColor(upper,.78f);
            for(std::size_t point=0;point<outline.size();++point) {
                const std::size_t next=(point+1U)%outline.size();
                AddTriangle(vertices,top[point],bottom[point],bottom[next],
                    wildlife_render::FacetColor(top[point],bottom[point],bottom[next],edge,true));
                AddTriangle(vertices,top[point],bottom[next],top[next],
                    wildlife_render::FacetColor(top[point],bottom[next],top[next],edge,true));
            }
        }

        [[nodiscard]] Color Lit(const Color& base,const Vector3& normal)
        {
            const float light=.62f+.30f*std::max(0.0f,
                Vector3::Dot(normal,wildlife_render::FixedSunDirection))
                +.08f*std::max(0.0f,normal.Y);
            const auto channel=[light](int value) {
                return static_cast<int>(std::clamp(std::lround(value*light),0L,255L));
            };
            return Color(channel(base.getRProperty()),channel(base.getGProperty()),
                channel(base.getBProperty()),static_cast<int>(base.getAProperty()));
        }

        [[nodiscard]] Vector3 LocalPoint(const Vector3& centre, const Vector3& right,
                                         const Vector3& up, const Vector3& forward,
                                         const float localX,
                                         const float localY, const float localZ,
                                         const float scale)
        {
            return centre + right * (localX * scale) + up * (localY * scale)
                + forward * (localZ * scale);
        }

        void AppendNearBody(std::vector<VertexPositionColor>& vertices,
                            const Vector3& centre,const Vector3& right,
                            const Vector3& up,const Vector3& forward,const float scale,
                            const BirdPalette& palette,const BirdSpecies species)
        {
            constexpr int Sides=16;
            const bool dove=species==BirdSpecies::FruitDove;
            const Vector3 tail=LocalPoint(centre,right,up,forward,0,dove?.008f:0.0f,
                dove?-.45f:-.49f,scale);
            const Vector3 neck=LocalPoint(centre,right,up,forward,0,dove?.030f:.015f,
                dove?.365f:.34f,scale);
            const auto ring=[&](float angle,float xRadius,float yRadius,float z) {
                return LocalPoint(centre,right,up,forward,std::cos(angle)*xRadius,
                    std::sin(angle)*yRadius,z,scale);
            };
            for(int side=0;side<Sides;++side) {
                const float a0=6.28318530718f*static_cast<float>(side)/Sides;
                const float a1=6.28318530718f*static_cast<float>(side+1)/Sides;
                const Vector3 rear0=ring(a0,dove?.15f:.12f,dove?.13f:.105f,-.31f);
                const Vector3 rear1=ring(a1,dove?.15f:.12f,dove?.13f:.105f,-.31f);
                const Vector3 middle0=ring(a0,dove?.22f:.19f,dove?.18f:.16f,-.02f);
                const Vector3 middle1=ring(a1,dove?.22f:.19f,dove?.18f:.16f,-.02f);
                const Vector3 front0=ring(a0,dove?.15f:.115f,dove?.14f:.105f,.27f);
                const Vector3 front1=ring(a1,dove?.15f:.115f,dove?.14f:.105f,.27f);
                const float middleAngle=(a0+a1)*.5f;
                const Vector3 normal=Vector3::Normalize(
                    right*std::cos(middleAngle)+up*std::sin(middleAngle));
                const Color color=Lit(std::sin(middleAngle)<-.05f
                    ?palette.lowerWing:palette.body,normal);
                AddTriangle(vertices,tail,rear1,rear0,color);
                AddTriangle(vertices,rear0,rear1,middle1,color);
                AddTriangle(vertices,rear0,middle1,middle0,color);
                AddTriangle(vertices,middle0,middle1,front1,color);
                AddTriangle(vertices,middle0,front1,front0,color);
                AddTriangle(vertices,neck,front0,front1,color);
            }
        }

        void AppendNearHead(std::vector<VertexPositionColor>& vertices,
                            const Vector3& centre,const Vector3& right,
                            const Vector3& up,const Vector3& forward,const float scale,
                            const BirdPalette& palette, const BirdSpecies species,
                            const Vector3& cameraPosition)
        {
            constexpr int Sides=16;
            const float headScale = species == BirdSpecies::Parrot ? .88f
                : (species == BirdSpecies::FruitDove ? .82f : .90f);
            const bool parrot = species == BirdSpecies::Parrot;
            const bool dove = species == BirdSpecies::FruitDove;
            const Vector3 top=LocalPoint(centre,right,up,forward,0,.180f*headScale,.405f,scale);
            const Vector3 bottom=LocalPoint(centre,right,up,forward,0,-.072f*headScale,.395f,scale);
            const auto ring=[&](float angle,float xRadius,float zRadius,float y) {
                return LocalPoint(centre,right,up,forward,std::cos(angle)*xRadius*headScale,
                    y*headScale,.405f+std::sin(angle)*zRadius*headScale,scale);
            };
            if(dove) {
                const Vector3 doveHead=LocalPoint(centre,right,up,forward,0,.042f,.415f,scale);
                AppendEllipsoid(vertices,doveHead,right,up,forward,
                    .108f*scale,.126f*scale,.140f*scale,palette.head,
                    wildlife_render::ScaleColor(palette.head,.90f),6,12);
            } else {
                for(int side=0;side<Sides;++side) {
                    const float a0=6.28318530718f*static_cast<float>(side)/Sides;
                    const float a1=6.28318530718f*static_cast<float>(side+1)/Sides;
                    const Vector3 upper0=ring(a0,.098f,.115f,.112f),upper1=ring(a1,.098f,.115f,.112f);
                    const Vector3 middle0=ring(a0,.125f,.145f,.040f),middle1=ring(a1,.125f,.145f,.040f);
                    const Vector3 lower0=ring(a0,.100f,.112f,-.025f),lower1=ring(a1,.100f,.112f,-.025f);
                    const float middleAngle=(a0+a1)*.5f;
                    const Vector3 normal=Vector3::Normalize(
                        right*std::cos(middleAngle)+forward*std::sin(middleAngle));
                    const Color color=Lit(palette.head,normal);
                    AddTriangle(vertices,top,upper0,upper1,color);
                    AddTriangle(vertices,upper0,middle0,middle1,color);
                    AddTriangle(vertices,upper0,middle1,upper1,color);
                    AddTriangle(vertices,middle0,lower0,lower1,color);
                    AddTriangle(vertices,middle0,lower1,middle1,color);
                    AddTriangle(vertices,bottom,lower1,lower0,color);
                }
            }
            const float beakWidth = parrot ? .046f : (dove ? .026f : .038f);
            const float beakBase = parrot ? .526f : (dove ? .510f : .520f);
            const float beakReach = parrot ? .616f : (dove ? .584f : .625f);
            const float baseTop=parrot?.098f:.078f;
            const float baseBottom=parrot?.025f:.026f;
            const float midZ=beakBase+(beakReach-beakBase)*.58f;
            const float midWidth=beakWidth*(parrot?.72f:.62f);
            const std::array<Vector3,4> beakRoot{
                LocalPoint(centre,right,up,forward,-beakWidth,.050f,beakBase,scale),
                LocalPoint(centre,right,up,forward,0,baseTop,beakBase,scale),
                LocalPoint(centre,right,up,forward,beakWidth,.050f,beakBase,scale),
                LocalPoint(centre,right,up,forward,0,baseBottom,beakBase,scale)};
            const std::array<Vector3,4> beakMiddle{
                LocalPoint(centre,right,up,forward,-midWidth,parrot?.036f:.047f,midZ,scale),
                LocalPoint(centre,right,up,forward,0,parrot?.078f:.064f,midZ,scale),
                LocalPoint(centre,right,up,forward,midWidth,parrot?.036f:.047f,midZ,scale),
                LocalPoint(centre,right,up,forward,0,parrot?.010f:.033f,midZ,scale)};
            const Vector3 beakTip=LocalPoint(centre,right,up,forward,0,
                parrot?-.012f:.043f,beakReach,scale);
            for(std::size_t face=0;face<beakRoot.size();++face) {
                const std::size_t next=(face+1U)%beakRoot.size();
                const auto addBeakTriangle=[&](const Vector3& a,const Vector3& b,
                                               const Vector3& c) {
                    AddTriangle(vertices,a,b,c,
                        wildlife_render::FacetColor(a,b,c,palette.beak,true));
                };
                addBeakTriangle(beakRoot[face],beakMiddle[face],beakMiddle[next]);
                addBeakTriangle(beakRoot[face],beakMiddle[next],beakRoot[next]);
                addBeakTriangle(beakMiddle[face],beakTip,beakMiddle[next]);
            }
            constexpr int EyeSides=6;
            const float eyeSide=Vector3::Dot(cameraPosition-centre,right)>=0.0f?1.0f:-1.0f;
            const float eyeX=eyeSide*(dove?.109f:.106f*headScale);
            const Vector3 eyeCentre=LocalPoint(
                centre,right,up,forward,eyeX,.078f*headScale,.469f,scale);
            for(int segment=0;segment<EyeSides;++segment) {
                const float a0=6.28318530718f*static_cast<float>(segment)/EyeSides;
                const float a1=6.28318530718f*static_cast<float>(segment+1)/EyeSides;
                const auto eyeEdge=[&](const float angle) {
                    return LocalPoint(centre,right,up,forward,eyeX,
                        .078f*headScale+std::sin(angle)*.013f,
                        .469f+std::cos(angle)*.015f,scale);
                };
                if(dove) {
                    const float outerRadiusY=.017f,outerRadiusZ=.019f;
                    const float innerRadiusY=.008f,innerRadiusZ=.009f;
                    const auto ringPoint=[&](const float angle,const float ry,const float rz,
                                             const float outward) {
                        return LocalPoint(centre,right,up,forward,
                            eyeX+eyeSide*outward,
                            .078f*headScale+std::sin(angle)*ry,
                            .469f+std::cos(angle)*rz,scale);
                    };
                    const Vector3 outer0=ringPoint(a0,outerRadiusY,outerRadiusZ,.003f);
                    const Vector3 outer1=ringPoint(a1,outerRadiusY,outerRadiusZ,.003f);
                    const Vector3 inner0=ringPoint(a0,innerRadiusY,innerRadiusZ,.005f);
                    const Vector3 inner1=ringPoint(a1,innerRadiusY,innerRadiusZ,.005f);
                    const Color eyeRing(188,145,111,255);
                    if(eyeSide>0.0f) {
                        AddTriangle(vertices,outer0,outer1,inner1,eyeRing);
                        AddTriangle(vertices,outer0,inner1,inner0,eyeRing);
                        AddTriangle(vertices,eyeCentre,inner1,inner0,palette.eye);
                    } else {
                        AddTriangle(vertices,outer0,inner1,outer1,eyeRing);
                        AddTriangle(vertices,outer0,inner0,inner1,eyeRing);
                        AddTriangle(vertices,eyeCentre,inner0,inner1,palette.eye);
                    }
                } else if(eyeSide>0.0f)
                    AddTriangle(vertices,eyeCentre,eyeEdge(a1),eyeEdge(a0),palette.eye);
                else AddTriangle(vertices,eyeCentre,eyeEdge(a0),eyeEdge(a1),palette.eye);
                }
        }

        void AppendNearBird(std::vector<VertexPositionColor>& vertices,
                            const BirdState& bird,const Vector3& cameraPosition)
        {
            const std::size_t startingVertexCount=vertices.size();
            const BirdPalette& palette = PaletteForBird(bird);
            const Vector3 centre(bird.x, bird.y, bird.z);
            const Vector3 forward(std::cos(bird.headingRadians), 0.0f,
                                  std::sin(bird.headingRadians));
            const Vector3 levelRight(-forward.Z, 0.0f, forward.X);
            const float bankCosine = std::cos(bird.bankRadians);
            const float bankSine = std::sin(bird.bankRadians);
            const Vector3 right = levelRight * bankCosine + Vector3::Up * bankSine;
            const Vector3 up = Vector3::Up * bankCosine - levelRight * bankSine;
            const float scale = bird.scale;
            AppendNearBody(vertices,centre,right,up,forward,scale,palette,bird.species);
            AppendNearHead(vertices,centre,right,up,forward,scale,palette,bird.species,
                           cameraPosition);

            const float perch = std::clamp(bird.perchAmount, 0.0f, 1.0f);
            const bool groundDove = bird.behavior == BirdBehavior::GroundApproach
                || bird.behavior == BirdBehavior::GroundForage
                || bird.behavior == BirdBehavior::GroundDepart;
            const float wingLift = std::lerp(bird.wingPose * 0.38f, -0.025f, perch);
            const float wingSpan = std::lerp(.96f, 0.21f, perch);
            const float wingRear = std::lerp(-0.08f, -0.29f, perch);
            const Vector3 leftFront = LocalPoint(
                centre, right, up, forward, -0.06f, 0.035f, 0.20f, scale);
            const Vector3 leftBack = LocalPoint(
                centre, right, up, forward, -0.08f, 0.015f, -0.24f, scale);
            const Vector3 rightFront = LocalPoint(
                centre, right, up, forward, 0.06f, 0.035f, 0.20f, scale);
            const Vector3 rightBack = LocalPoint(
                centre, right, up, forward, 0.08f, 0.015f, -0.24f, scale);
            for(const float side:{-1.0f,1.0f}) {
                if(groundDove) {
                    const Vector3 foldedWing=LocalPoint(centre,right,up,forward,
                        side*.145f,.045f,-.055f,scale);
                    AppendEllipsoid(vertices,foldedWing,right,up,forward,
                        .105f*scale,.100f*scale,.345f*scale,
                        palette.upperWing,wildlife_render::ScaleColor(palette.upperWing,.82f));
                    continue;
                }
                const Vector3 front=side<0.0f?leftFront:rightFront;
                const Vector3 back=side<0.0f?leftBack:rightBack;
                const Vector3 elbow=LocalPoint(centre,right,up,forward,side*.40f,
                    wingLift*.30f+.035f,std::lerp(.055f,-.20f,perch),scale);
                const Vector3 tip=LocalPoint(
                    centre,right,up,forward,side*wingSpan,wingLift,wingRear,scale);
                const Vector3 outerTrailing=LocalPoint(centre,right,up,forward,
                    side*std::lerp(.62f,.17f,perch),wingLift*.62f-.018f,
                    std::lerp(-.39f,-.48f,perch),scale);
                const Vector3 innerTrailing=LocalPoint(centre,right,up,forward,
                    side*std::lerp(.25f,.11f,perch),wingLift*.20f-.010f,
                    std::lerp(-.34f,-.43f,perch),scale);
                const std::array<Vector3,6> wingOutline{
                    front,elbow,tip,outerTrailing,innerTrailing,back};
                AppendClosedSurface(vertices,wingOutline,up*(.014f*scale),
                                    palette.upperWing,palette.lowerWing);
            }

            const float tailHalfWidth = bird.species == BirdSpecies::Parrot ? .16f
                : (bird.species == BirdSpecies::FruitDove ? .21f : .34f);
            const float tailLength = bird.species == BirdSpecies::Parrot ? -1.08f
                : (bird.species == BirdSpecies::FruitDove ? -.64f : -.83f);
            const float featherRoot=bird.species==BirdSpecies::Parrot?.075f:tailHalfWidth*.42f;
            for(const float side:{-1.0f,1.0f}) {
                const float tipZ=tailLength-(side>0.0f&&bird.species==BirdSpecies::Parrot?.055f:0.0f);
                const std::array<Vector3,4> feather{
                    LocalPoint(centre,right,up,forward,side*.025f,0.0f,-.35f,scale),
                    LocalPoint(centre,right,up,forward,side*featherRoot,-.006f,-.47f,scale),
                    LocalPoint(centre,right,up,forward,side*tailHalfWidth,side*.012f,tipZ,scale),
                    LocalPoint(centre,right,up,forward,side*.018f,side*.018f,tipZ+.10f,scale)};
                AppendClosedSurface(vertices,feather,up*(.010f*scale),palette.tail,
                    wildlife_render::ScaleColor(palette.tail,.84f));
            }

            if (perch > 0.35f) {
                for (const float side : {-1.0f, 1.0f}) {
                    const Vector3 leg = LocalPoint(
                        centre, right, up, forward, side * 0.055f, -0.075f, 0.01f, scale);
                    const Vector3 foot = LocalPoint(
                        centre, right, up, forward, side * 0.055f,
                        groundDove ? -0.20f : -0.12f, 0.01f, scale);
                    const Vector3 toe = LocalPoint(
                        centre, right, up, forward, side * 0.055f,
                        groundDove ? -0.197f : -0.117f, 0.105f, scale);
                    const Vector3 rearToe = LocalPoint(
                        centre, right, up, forward, side * 0.055f,
                        groundDove ? -0.197f : -0.117f, -0.06f, scale);
                    if(groundDove) {
                        const Color footColor(132,82,67,255);
                        const Vector3 outerToe=LocalPoint(centre,right,up,forward,
                            side*.080f,-.197f,.090f,scale);
                        AppendTaperedSegment(vertices,leg,foot,.013f*scale,.011f*scale,
                                             footColor);
                        AppendTaperedSegment(vertices,foot,toe,.010f*scale,.004f*scale,
                                             footColor);
                        AppendTaperedSegment(vertices,foot,outerToe,.009f*scale,.004f*scale,
                                             footColor);
                        AppendTaperedSegment(vertices,foot,rearToe,.009f*scale,.004f*scale,
                                             footColor);
                    } else {
                        AddDoubleSidedTriangle(vertices, leg, foot, toe, palette.tail);
                        AddDoubleSidedTriangle(vertices, leg, rearToe, foot, palette.tail);
                    }
                }
            }
            std::size_t expected=perch>0.35f?846U:822U;
            if(bird.species==BirdSpecies::FruitDove)expected+=180U;
            if(groundDove) {
                expected+=360U;
                if(perch>0.35f)expected+=216U;
            }
            if(vertices.size()-startingVertexCount!=expected) {
                throw std::runtime_error("Bird near mesh does not match its fixed vertex budget");
            }
        }

        void AppendFarBird(std::vector<VertexPositionColor>& vertices,
                           const BirdState& bird)
        {
            const BirdPalette& palette = PaletteForBird(bird);
            const Vector3 centre(bird.x, bird.y, bird.z);
            const Vector3 forward(std::cos(bird.headingRadians), 0.0f,
                                  std::sin(bird.headingRadians));
            const Vector3 levelRight(-forward.Z, 0.0f, forward.X);
            const float bankCosine = std::cos(bird.bankRadians);
            const float bankSine = std::sin(bird.bankRadians);
            const Vector3 right = levelRight * bankCosine + Vector3::Up * bankSine;
            const Vector3 up = Vector3::Up * bankCosine - levelRight * bankSine;
            const float scale = bird.scale;
            const float perch = std::clamp(bird.perchAmount, 0.0f, 1.0f);
            const float wingLift = std::lerp(bird.wingPose * 0.30f, -0.02f, perch);
            const float wingSpan = std::lerp(0.98f, 0.20f, perch);
            const float wingRear = std::lerp(-0.04f, -0.28f, perch);
            const Vector3 nose = LocalPoint(
                centre, right, up, forward, 0.0f, 0.0f, 0.60f, scale);
            const Vector3 tail = LocalPoint(
                centre, right, up, forward, 0.0f, 0.0f, -0.55f, scale);
            const Vector3 leftTip = LocalPoint(
                centre, right, up, forward, -wingSpan, wingLift, wingRear, scale);
            const Vector3 rightTip = LocalPoint(
                centre, right, up, forward, wingSpan, wingLift, wingRear, scale);
            AddDoubleSidedTriangle(vertices, nose, leftTip, tail, palette.upperWing);
            AddDoubleSidedTriangle(vertices, nose, tail, rightTip, palette.upperWing);
        }
    }

    BirdRenderer::BirdRenderer(GraphicsDevice& device, const float maximumDistance)
        : device_(device),
          effect_(std::make_unique<BasicEffect>(device)),
          vertices_(std::make_unique<DynamicVertexBuffer>(
              device, VertexPositionColor::getVertexDeclarationStatic(), MaximumVertexCount,
              BufferUsage::WriteOnly)),
          maximumDistance_(maximumDistance)
    {
        if (!std::isfinite(maximumDistance_) || maximumDistance_ <= 0.0f) {
            throw std::invalid_argument("Bird maximum draw distance must be finite and positive");
        }
        effect_->setVertexColorEnabledProperty(true);
        effect_->setLightingEnabledProperty(false);
        effect_->setFogEnabledProperty(true);
        effect_->setFogColorProperty(Vector3(environment::FogColorRed, environment::FogColorGreen, environment::FogColorBlue));
        effect_->setFogStartProperty(310.0f);
        effect_->setFogEndProperty(700.0f);
        stagingVertices_.reserve(static_cast<std::size_t>(MaximumVertexCount));
        std::cout << NearBirdFeatherDiagnostic << '\n';
        std::cout << BirdPerchSilhouetteDiagnostic << '\n';
        std::cout << NearBirdHeadDiagnostic << '\n';
        MirrorRuntimeDiagnosticToPlatformLog(NearBirdFeatherDiagnostic);
        MirrorRuntimeDiagnosticToPlatformLog(BirdPerchSilhouetteDiagnostic);
        MirrorRuntimeDiagnosticToPlatformLog(NearBirdHeadDiagnostic);
        std::cout << TropicalBirdDiagnostic << '\n';
        MirrorRuntimeDiagnosticToPlatformLog(TropicalBirdDiagnostic);
        std::cout << NearBirdProfileDiagnostic << '\n';
        MirrorRuntimeDiagnosticToPlatformLog(NearBirdProfileDiagnostic);
        std::cout << GroundDoveAnatomyDiagnostic << '\n';
        MirrorRuntimeDiagnosticToPlatformLog(GroundDoveAnatomyDiagnostic);
    }

    BirdRenderer::~BirdRenderer() = default;

    void BirdRenderer::Draw(const Matrix& view, const Matrix& projection,
                            const Vector3& cameraPosition,
                            const std::span<const BirdState> birds)
    {
        stagingVertices_.clear();
        visibleBirdCount_ = 0;
        visibleNearBirdCount_ = 0;
        visibleFarBirdCount_ = 0;
        drawCallCount_ = 0;
        const BoundingFrustum frustum(view * projection);

        for (const BirdState& bird : birds) {
            const Vector3 centre(bird.x, bird.y, bird.z);
            const float distanceSquared = Vector3::DistanceSquared(centre, cameraPosition);
            if (distanceSquared > maximumDistance_ * maximumDistance_
                || !frustum.Intersects(BoundingSphere(centre, bird.scale * 1.25f))) {
                continue;
            }
            if (distanceSquared <= NearLodDistance * NearLodDistance) {
                AppendNearBird(stagingVertices_, bird, cameraPosition);
                ++visibleNearBirdCount_;
            } else {
                AppendFarBird(stagingVertices_, bird);
                ++visibleFarBirdCount_;
            }
            ++visibleBirdCount_;
        }

        if (stagingVertices_.empty()) {
            return;
        }

        device_.setRasterizerStateProperty(RasterizerState::CullNone);
        device_.setBlendStateProperty(BlendState::Opaque);
        device_.setDepthStencilStateProperty(DepthStencilState::Default);
        vertices_->SetData(stagingVertices_.data(), 0,
                           static_cast<int>(stagingVertices_.size()), SetDataOptions::Discard);
        device_.SetVertexBuffer(vertices_.get());
        device_.setIndicesProperty(nullptr);
        effect_->setWorldProperty(Matrix::getIdentityProperty());
        effect_->setViewProperty(view);
        effect_->setProjectionProperty(projection);
        effect_->Apply();
        device_.DrawPrimitives(PrimitiveType::TriangleList, 0,
                               static_cast<int>(stagingVertices_.size() / 3U));
        ++drawCallCount_;
    }
}
