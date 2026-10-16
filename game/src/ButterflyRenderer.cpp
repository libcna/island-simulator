#include "IslandSimulator/EnvironmentLighting.hpp"
#include "IslandSimulator/ButterflyRenderer.hpp"

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
        constexpr float NearLodDistance = 25.0f;
        constexpr int MaximumVerticesPerButterfly = 384;
        constexpr int MaximumFlowerVertices = 1800;
        constexpr int MaximumGeckoVertices = static_cast<int>(GeckoPopulation::Count) * 2300;
        constexpr int MaximumSkinkVertices = static_cast<int>(SkinkPopulation::Count) * 1700;
        constexpr int MaximumDragonflyVertices =
            static_cast<int>(DragonflySwarm::Count) * 1500;
        constexpr int MaximumFrogVertices = static_cast<int>(FrogPopulation::Count) * 1700;
        constexpr int RodentVertexCount = 4056;
        constexpr int MaximumRodentVertices =
            static_cast<int>(RodentPopulation::Count) * RodentVertexCount;
        // A close fly uses fewer than 1,400 vertices across three closed body volumes,
        // fitted eyes, swept wings and six articulated solid legs.
        constexpr int MaximumFlyVertices = static_cast<int>(FlySwarms::Count) * 1400;
        constexpr int AntVertexCount = 1692;
        constexpr int CarryingAntVertexCount = 1800;
        constexpr int MaximumAntVertices =
            static_cast<int>(AntTrails::Count) * CarryingAntVertexCount;
        // A close cicada uses 1,740 vertices including continuous body lofts,
        // compound eyes, roof-folded wing veins, antennae and solid jointed legs.
        constexpr int MaximumCicadaVertices = static_cast<int>(CicadaColony::Count) * 1800;
        constexpr int BeetleVertexCount = 3102;
        constexpr int MaximumBeetleVertices =
            static_cast<int>(BeetlePopulation::Count) * BeetleVertexCount;
        constexpr int MaximumVertexCount = static_cast<int>(ButterflyMeadow::ButterflyCount)
            * MaximumVerticesPerButterfly + MaximumFlowerVertices + MaximumGeckoVertices
            + MaximumSkinkVertices
            +MaximumDragonflyVertices + MaximumFrogVertices + MaximumRodentVertices
            + MaximumFlyVertices + MaximumAntVertices + MaximumCicadaVertices
            + MaximumBeetleVertices;
        constexpr std::string_view ButterflyDiagnostic =
            "island-simulator: clearing butterflies use hinged patterned wings in one culled batch";
        constexpr std::string_view ButterflyAnatomyDiagnostic =
            "island-simulator: near butterflies use closed bodies, paired antennae, rounded wing lobes, and dark margins";
        constexpr std::string_view ButterflyBodyDiagnostic =
            "island-simulator: near butterflies use four-ring rounded abdomens, six swept legs, and restrained wing-root spots";
        constexpr std::string_view ButterflyHabitatDiagnostic =
            "island-simulator: butterflies feed at three visible deterministic nectar-flower patches";
        constexpr std::string_view GeckoDiagnostic =
            "island-simulator: eight inland geckos rest, scurry from the player, and hide in the shared butterfly batch";
        constexpr std::string_view GeckoAnatomyDiagnostic =
            "island-simulator: geckos use rounded wedge heads, solid tapered limbs and tails, twelve expanded toe pads, raised eyes, and fitted dorsal spots";
        constexpr std::string_view SkinkDiagnostic =
            "island-simulator: eight open-ground skinks bask, forage, and flee in the shared butterfly batch";
        constexpr std::string_view SkinkAnatomyDiagnostic =
            "island-simulator: skinks use smooth elongated trunks, short articulated limbs, tapered tails, longitudinal stripes, and highlighted eyes";
        constexpr std::string_view SkinkSolidDiagnostic =
            "island-simulator: skinks use solid tapered leg, toe, and five-section tail volumes with surface-set highlighted eyes";
        constexpr std::string_view DragonflyDiagnostic=
            "island-simulator: nine stream and wetland dragonflies hover, patrol, and dart in the shared butterfly batch";
        constexpr std::string_view DragonflyAnatomyDiagnostic=
            "island-simulator: near dragonflies use connected tapered abdominal segments, rounded heads, fitted compound eyes, solid jointed legs, and subdued veined wings";
        constexpr std::string_view FrogDiagnostic =
            "island-simulator: ten stream and wetland frogs rest, hop, and retreat in the shared butterfly batch";
        constexpr std::string_view FrogAnatomyDiagnostic =
            "island-simulator: near frogs use solid folded legs, separated webbed toes, rounded eye sockets, fitted irises, nostrils, and a mouth line";
        constexpr std::string_view RodentDiagnostic =
            "island-simulator: twelve field mice and eight rats forage, scurry, and hide in the shared small-fauna batch";
        constexpr std::string_view RodentAnatomyDiagnostic =
            "island-simulator: near rodents use species-scaled lofted bodies, pointed muzzles, rounded ears, whiskers, paws, eyes, and long segmented tails";
        constexpr std::string_view RodentSurfaceDiagnostic =
            "island-simulator: mice use rounded compact shoulders while rats use long low twelve-facet body and head profiles";
        constexpr std::string_view FlyDiagnostic =
            "island-simulator: four close-range fly swarms use twenty-four hover, orbit, and dart agents in the shared small-fauna batch";
        constexpr std::string_view FlyAnatomyDiagnostic =
            "island-simulator: near flies use separate rounded thoraxes, tapered abdomens, fitted red compound eyes, jointed solid legs, antennae, and veined swept wings";
        constexpr std::string_view AntDiagnostic =
            "island-simulator: four close-range ant trails use sixty-four soil and trunk-climbing agents in the shared small-fauna batch";
        constexpr std::string_view AntAnatomyDiagnostic =
            "island-simulator: close ants use rounded three-part bodies, paired waist nodes, fitted eyes, mandibles, solid three-segment legs, bent antennae, and physically attached carried leaves";
        constexpr std::string_view CicadaDiagnostic =
            "island-simulator: twelve cicadas rest, call, and make short flights from real broadleaf trunks in the shared small-fauna batch";
        constexpr std::string_view CicadaAnatomyDiagnostic =
            "island-simulator: close cicadas use continuous thorax and abdomen lofts, rounded compound eyes, short antennae, solid jointed legs, and two veined roof-folded wings";
        constexpr std::string_view BeetleDiagnostic =
            "island-simulator: fifteen tropical ground beetles rest, forage, and hide in three inland leaf-litter colonies";
        constexpr std::string_view BeetleAnatomyDiagnostic =
            "island-simulator: close beetles use rounded striated elytra, a fitted pronotum and head, compound eyes, clubbed antennae, mandibles, and six solid three-segment legs";

        struct ButterflyPalette
        {
            Color forewing;
            Color hindwing;
            Color accent;
            Color body;
        };

        const std::array<ButterflyPalette, 5> Palettes = {{
            {Color(49, 111, 156, 255), Color(74, 153, 183, 255),
             Color(205, 225, 190, 255), Color(32, 37, 34, 255)},
            {Color(211, 110, 39, 255), Color(236, 159, 54, 255),
             Color(45, 39, 32, 255), Color(38, 32, 29, 255)},
            {Color(221, 202, 94, 255), Color(235, 220, 129, 255),
             Color(87, 69, 40, 255), Color(43, 39, 30, 255)},
            {Color(55, 143, 126, 255), Color(91, 181, 146, 255),
             Color(220, 219, 142, 255), Color(31, 42, 36, 255)},
            {Color(213, 217, 205, 255), Color(178, 194, 187, 255),
             Color(75, 93, 87, 255), Color(38, 42, 40, 255)}
        }};

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

        void AppendFlowerPatch(std::vector<VertexPositionColor>& vertices,
                               const ButterflyFlowerPatch& patch,
                               const std::size_t patchIndex)
        {
            constexpr int FlowerCount = 7;
            const std::array<Color, 3> petals = {{
                Color(224, 194, 76, 255), Color(218, 126, 153, 255),
                Color(184, 153, 220, 255)}};
            const Color petal = petals[patchIndex % petals.size()];
            const Color petalShade = Color::Lerp(petal, Color(112, 75, 91, 255), .22f);
            const Color stem(59, 111, 48, 255);
            const Color leaf(77, 133, 58, 255);
            const Color centreColor(232, 175, 45, 255);
            for (int flower = 0; flower < FlowerCount; ++flower) {
                const float angle = static_cast<float>(flower) * 2.39996323f
                    + static_cast<float>(patchIndex) * .61f;
                const float radius = flower == 0 ? 0.0f
                    : patch.radius * (.22f + .10f * static_cast<float>(flower));
                const float x = patch.x + std::cos(angle) * radius;
                const float z = patch.z + std::sin(angle) * radius;
                const float groundY = patch.y + patch.groundSlopeX * (x - patch.x)
                    + patch.groundSlopeZ * (z - patch.z);
                const float height = .32f + .035f * static_cast<float>(flower % 4);
                const float width = .018f;
                const Vector3 base(x, groundY + .012f, z);
                const Vector3 top(x, groundY + height, z);
                AddDoubleSidedTriangle(vertices, base + Vector3(-width, 0, 0),
                    top + Vector3(-width * .45f, 0, 0), top + Vector3(width * .45f, 0, 0), stem);
                AddDoubleSidedTriangle(vertices, base + Vector3(-width, 0, 0),
                    top + Vector3(width * .45f, 0, 0), base + Vector3(width, 0, 0), stem);
                const float leafSide = flower % 2 == 0 ? 1.0f : -1.0f;
                AddDoubleSidedTriangle(vertices,
                    Vector3(x, groundY + height * .46f, z),
                    Vector3(x + leafSide * .15f, groundY + height * .56f, z + .045f),
                    Vector3(x + leafSide * .055f, groundY + height * .62f, z - .035f), leaf);
                const Vector3 centre(x, groundY + height, z);
                constexpr int PetalCount = 5;
                for (int petalIndex = 0; petalIndex < PetalCount; ++petalIndex) {
                    const float petalAngle = angle + static_cast<float>(petalIndex)
                        * 2.0f * 3.14159265359f / static_cast<float>(PetalCount);
                    const Vector3 inner = centre + Vector3(
                        std::cos(petalAngle) * .035f, .006f,
                        std::sin(petalAngle) * .035f);
                    const Vector3 left = centre + Vector3(
                        std::cos(petalAngle - .43f) * .11f, -.006f,
                        std::sin(petalAngle - .43f) * .11f);
                    const Vector3 right = centre + Vector3(
                        std::cos(petalAngle + .43f) * .11f, -.006f,
                        std::sin(petalAngle + .43f) * .11f);
                    AddDoubleSidedTriangle(vertices, inner, left, right,
                        petalIndex % 2 == 0 ? petal : petalShade);
                }
                const Vector3 north = centre + Vector3(0, .025f, .035f);
                const Vector3 east = centre + Vector3(.035f, .025f, 0);
                const Vector3 south = centre + Vector3(0, .025f, -.035f);
                const Vector3 west = centre + Vector3(-.035f, .025f, 0);
                const Vector3 crown = centre + Vector3(0, .055f, 0);
                AddTriangle(vertices, crown, north, east, centreColor);
                AddTriangle(vertices, crown, east, south, centreColor);
                AddTriangle(vertices, crown, south, west, centreColor);
                AddTriangle(vertices, crown, west, north, centreColor);
            }
        }

        void AppendGeckoDiamond(std::vector<VertexPositionColor>& vertices,
                                const Vector3& centre, const Vector3& right,
                                const Vector3& up, const Vector3& forward,
                                const float halfX, const float halfY, const float halfZ,
                                const Color& top, const Color& side)
        {
            const Vector3 crown = centre + up * halfY;
            const Vector3 belly = centre - up * halfY;
            const Vector3 left = centre - right * halfX;
            const Vector3 rightPoint = centre + right * halfX;
            const Vector3 front = centre + forward * halfZ;
            const Vector3 rear = centre - forward * halfZ;
            AddTriangle(vertices, crown, front, rightPoint, top);
            AddTriangle(vertices, crown, rightPoint, rear, top);
            AddTriangle(vertices, crown, rear, left, top);
            AddTriangle(vertices, crown, left, front, top);
            AddTriangle(vertices, belly, rightPoint, front, side);
            AddTriangle(vertices, belly, rear, rightPoint, side);
            AddTriangle(vertices, belly, left, rear, side);
            AddTriangle(vertices, belly, front, left, side);
        }

        void AppendGeckoSegment(std::vector<VertexPositionColor>& vertices,
                                const Vector3& a, const Vector3& b,
                                const float halfWidth, const Color& color)
        {
            const Vector3 delta = b - a;
            const float length = std::max(.001f, std::hypot(delta.X, delta.Z));
            const Vector3 side(-delta.Z * halfWidth / length, 0.0f,
                               delta.X * halfWidth / length);
            AddDoubleSidedTriangle(vertices, a + side, b + side * .52f, b - side * .52f, color);
            AddDoubleSidedTriangle(vertices, a + side, b - side * .52f, a - side, color);
        }

        void AppendGeckoTube(std::vector<VertexPositionColor>& vertices,
                             const Vector3& a,const Vector3& b,
                             const float radiusA,const float radiusB,
                             const Color& color,const int sides=6)
        {
            Vector3 axis=b-a;
            const float length=axis.Length();
            if(length<.0001f) return;
            axis/=length;
            Vector3 side=Vector3::Cross(axis,std::abs(axis.Y)<.88f
                ?Vector3::Up:Vector3::Right);
            side.Normalize();
            const Vector3 other=Vector3::Cross(axis,side);
            const auto radial=[&](const int index) {
                const float angle=6.28318530718f*static_cast<float>(index)
                    /static_cast<float>(sides);
                return side*std::cos(angle)+other*std::sin(angle);
            };
            const auto shade=[&](const Vector3& normal) {
                return Color::Lerp(color,normal.Y>.25f?Color(220,225,170,255)
                                                        :Color(32,43,27,255),
                                   normal.Y>.25f?.10f:.07f);
            };
            for(int index=0;index<sides;++index) {
                const Vector3 first=radial(index),second=radial(index+1);
                const Vector3 p0=a+first*radiusA,p1=b+first*radiusB;
                const Vector3 p2=b+second*radiusB,p3=a+second*radiusA;
                AddTriangle(vertices,p0,p1,p2,shade(first));
                AddTriangle(vertices,p0,p2,p3,shade(second));
            }
        }

        void AppendGeckoSpot(std::vector<VertexPositionColor>& vertices,
                             const Vector3& centre,const Vector3& right,
                             const Vector3& forward,const float halfWidth,
                             const float halfLength,const Color& color)
        {
            constexpr int Sides=8;
            for(int edge=0;edge<Sides;++edge) {
                const float a=6.28318530718f*static_cast<float>(edge)/Sides;
                const float b=6.28318530718f*static_cast<float>(edge+1)/Sides;
                const Vector3 first=centre+right*(std::sin(a)*halfWidth)
                    +forward*(std::cos(a)*halfLength);
                const Vector3 second=centre+right*(std::sin(b)*halfWidth)
                    +forward*(std::cos(b)*halfLength);
                AddDoubleSidedTriangle(vertices,centre,first,second,color);
            }
        }

        template<std::size_t StationCount>
        void AppendGeckoLoft(std::vector<VertexPositionColor>& vertices,
                             const Vector3& centre,const Vector3& right,
                             const Vector3& up,const Vector3& forward,
                             const std::array<float,StationCount>& stations,
                             const std::array<float,StationCount>& halfWidths,
                             const std::array<float,StationCount>& halfHeights,
                             const float scale,const Color& dorsal,
                             const Color& flank,const Color& underside)
        {
            constexpr int Sides=8;
            const auto ringPoint=[&](const std::size_t station,const int side) {
                const float angle=6.28318530718f*static_cast<float>(side)/Sides;
                return centre+forward*(stations[station]*scale)
                    +right*(std::cos(angle)*halfWidths[station]*scale)
                    +up*(std::sin(angle)*halfHeights[station]*scale);
            };
            for(std::size_t station=0;station+1U<StationCount;++station) {
                for(int side=0;side<Sides;++side) {
                    const int next=(side+1)%Sides;
                    const float vertical=std::sin(6.28318530718f
                        *(static_cast<float>(side)+.5f)/Sides);
                    const Color color=vertical>.28f
                        ? Color::Lerp(dorsal,Color(194,204,139,255),.13f)
                        : (vertical<-.28f?underside:flank);
                    const Vector3 a=ringPoint(station,side);
                    const Vector3 b=ringPoint(station+1U,side);
                    const Vector3 c=ringPoint(station+1U,next);
                    const Vector3 d=ringPoint(station,next);
                    AddTriangle(vertices,a,b,c,color);
                    AddTriangle(vertices,a,c,d,color);
                }
            }
            const Vector3 rear=centre+forward*(stations.front()*scale);
            const Vector3 front=centre+forward*(stations.back()*scale);
            for(int side=0;side<Sides;++side) {
                const int next=(side+1)%Sides;
                AddTriangle(vertices,rear,ringPoint(0U,next),ringPoint(0U,side),flank);
                AddTriangle(vertices,front,ringPoint(StationCount-1U,side),
                            ringPoint(StationCount-1U,next),dorsal);
            }
        }

        template<std::size_t StationCount>
        void AppendBeetleLoft(std::vector<VertexPositionColor>& vertices,
                              const Vector3& centre, const Vector3& right,
                              const Vector3& up, const Vector3& forward,
                              const std::array<float, StationCount>& stations,
                              const std::array<float, StationCount>& halfWidths,
                              const std::array<float, StationCount>& halfHeights,
                              const float scale, const Color& dorsal,
                              const Color& flank, const Color& underside)
        {
            constexpr int Sides = 12;
            const auto ringPoint = [&](const std::size_t station, const int side) {
                const float angle = 6.28318530718f * static_cast<float>(side)
                    / static_cast<float>(Sides);
                return centre + forward * (stations[station] * scale)
                    + right * (std::cos(angle) * halfWidths[station] * scale)
                    + up * (std::sin(angle) * halfHeights[station] * scale);
            };
            for (std::size_t station = 0; station + 1U < StationCount; ++station) {
                for (int side = 0; side < Sides; ++side) {
                    const int next = (side + 1) % Sides;
                    const float vertical = std::sin(6.28318530718f
                        * (static_cast<float>(side) + .5f) / Sides);
                    const Color base = vertical > .22f
                        ? Color::Lerp(dorsal, Color(185, 205, 177, 255), .10f)
                        : (vertical < -.22f ? underside : flank);
                    const Color color = station % 2U == 0U
                        ? base : Color::Lerp(base, dorsal, .08f);
                    AddTriangle(vertices, ringPoint(station, side),
                                ringPoint(station + 1U, side),
                                ringPoint(station + 1U, next), color);
                    AddTriangle(vertices, ringPoint(station, side),
                                ringPoint(station + 1U, next),
                                ringPoint(station, next), color);
                }
            }
            const Vector3 rear = centre + forward * (stations.front() * scale);
            const Vector3 front = centre + forward * (stations.back() * scale);
            for (int side = 0; side < Sides; ++side) {
                const int next = (side + 1) % Sides;
                AddTriangle(vertices, rear, ringPoint(0U, next),
                            ringPoint(0U, side), underside);
                AddTriangle(vertices, front, ringPoint(StationCount - 1U, side),
                            ringPoint(StationCount - 1U, next), dorsal);
            }
        }

        void AppendGecko(std::vector<VertexPositionColor>& vertices, const GeckoState& gecko)
        {
            const std::array<Color, 4> backs = {{
                Color(104, 139, 62, 255), Color(146, 117, 67, 255),
                Color(73, 124, 89, 255), Color(157, 139, 77, 255)}};
            const Color back = backs[gecko.colorVariant % backs.size()];
            const Color flank = Color::Lerp(back, Color(49, 57, 36, 255), .32f);
            const Color underside=Color::Lerp(back,Color(208,192,139,255),.52f);
            const Color marking=Color::Lerp(back,Color(43,53,31,255),.58f);
            const Color eye(207, 171, 55, 255);
            const Color pupil(8, 11, 7, 255);
            const float scale = gecko.scale * std::clamp(gecko.visibility, 0.0f, 1.0f);
            const Vector3 centre(gecko.x, gecko.y + .072f * scale, gecko.z);
            const Vector3 forward(std::cos(gecko.headingRadians), 0,
                                  std::sin(gecko.headingRadians));
            const Vector3 right(-forward.Z, 0, forward.X);
            const Vector3 up = Vector3::Up;
            const Vector3 body=centre;
            constexpr std::array<float,6> BodyStations={{-.24f,-.16f,-.04f,.08f,.18f,.24f}};
            constexpr std::array<float,6> BodyWidths={{.045f,.110f,.135f,.140f,.120f,.065f}};
            constexpr std::array<float,6> BodyHeights={{.032f,.067f,.085f,.088f,.075f,.045f}};
            AppendGeckoLoft(vertices,body,right,up,forward,BodyStations,BodyWidths,
                            BodyHeights,scale,back,flank,underside);
            const Vector3 head=centre+forward*(.275f*scale)+up*(.018f*scale);
            constexpr std::array<float,6> HeadStations={{-.095f,-.050f,.025f,.095f,.155f,.188f}};
            constexpr std::array<float,6> HeadWidths={{.055f,.100f,.128f,.122f,.082f,.034f}};
            constexpr std::array<float,6> HeadHeights={{.038f,.063f,.076f,.071f,.050f,.024f}};
            AppendGeckoLoft(vertices,head,right,up,forward,HeadStations,HeadWidths,
                            HeadHeights,scale,back,flank,underside);
            const float stride = std::sin(gecko.gaitPhase) * .055f * scale;
            for (int sideIndex = -1; sideIndex <= 1; sideIndex += 2) {
                const float sideSign = static_cast<float>(sideIndex);
                for (int pair = 0; pair < 2; ++pair) {
                    const float along = pair == 0 ? .12f : -.12f;
                    const float opposed = (pair + (sideIndex > 0 ? 1 : 0)) % 2 == 0
                        ? stride : -stride;
                    const Vector3 root = body + forward * (along * scale)
                        + right * (sideSign * .07f * scale);
                    const Vector3 knee = root + right * (sideSign * .12f * scale)
                        + forward * opposed;
                    const Vector3 toe = knee + right * (sideSign * .10f * scale)
                        + forward * ((pair == 0 ? .055f : -.045f) * scale - opposed * .35f);
                    AppendGeckoTube(vertices,root,knee,.020f*scale,.015f*scale,flank);
                    AppendGeckoTube(vertices,knee,toe,.015f*scale,.010f*scale,back);
                    for(int digit=-1;digit<=1;++digit) {
                        const Vector3 digitEnd=toe+right*(sideSign*.055f*scale)
                            +forward*((static_cast<float>(digit)*.030f
                                +(pair==0?.018f:-.018f))*scale);
                        AppendGeckoTube(vertices,toe,digitEnd,.0065f*scale,
                                        .0040f*scale,underside);
                        AppendGeckoDiamond(vertices,digitEnd,right,up,forward,
                            .010f*scale,.0045f*scale,.016f*scale,underside,flank);
                    }
                }
            }
            Vector3 tailRoot = body - forward * (.205f * scale);
            for (int segment = 0; segment < 5; ++segment) {
                const float phase = gecko.gaitPhase * .35f + static_cast<float>(segment) * .72f;
                const float length = (.142f - static_cast<float>(segment) * .017f) * scale;
                const Vector3 tailEnd = tailRoot - forward * length
                    + right * (std::sin(phase) * (.032f+.004f*segment) * scale);
                const float rootRadius=(.040f-static_cast<float>(segment)*.0065f)*scale;
                const float tipRadius=std::max(.006f,
                    .0335f-static_cast<float>(segment)*.0065f)*scale;
                AppendGeckoTube(vertices,tailRoot,tailEnd,rootRadius,tipRadius,flank);
                tailRoot = tailEnd;
            }
            for (int sideIndex = -1; sideIndex <= 1; sideIndex += 2) {
                const float sideSign = static_cast<float>(sideIndex);
                const Vector3 eyeCentre = head + forward * (.065f * scale)
                    + right * (sideSign * .108f * scale) + up * (.050f * scale);
                AppendGeckoDiamond(vertices, eyeCentre, right, up, forward,
                                   .021f * scale, .021f * scale, .014f * scale, eye, eye);
                const Vector3 pupilCentre=eyeCentre+right*(sideSign*.018f*scale)
                    +up*(.007f*scale);
                AddDoubleSidedTriangle(vertices,pupilCentre+up*(.010f*scale),
                    pupilCentre-forward*(.011f*scale)-up*(.008f*scale),
                    pupilCentre+forward*(.011f*scale)-up*(.008f*scale),pupil);
            }
            for(int spot=0;spot<6;++spot) {
                const float z=(-.155f+static_cast<float>(spot)*.062f)*scale;
                const float width=(spot%2==0?.025f:.019f)*scale;
                const Vector3 spotCentre=body+forward*z+up*(.090f*scale);
                AppendGeckoSpot(vertices,spotCentre,right,forward,width,
                                (spot%2==0?.018f:.014f)*scale,marking);
            }
        }

        void AppendSkink(std::vector<VertexPositionColor>& vertices, const SkinkState& skink)
        {
            const std::array<Color, 4> backs = {{
                Color(87, 102, 72, 255), Color(112, 88, 61, 255),
                Color(70, 94, 94, 255), Color(126, 111, 73, 255)}};
            const Color back = backs[skink.colorVariant % backs.size()];
            const Color flank = Color::Lerp(back, Color(28, 36, 31, 255), .42f);
            const Color underside = Color::Lerp(back, Color(210, 192, 146, 255), .62f);
            const Color stripe = Color::Lerp(back, Color(225, 198, 105, 255), .47f);
            const Color eye(47, 35, 20, 255);
            const Color glint(215, 199, 151, 255);
            const float scale = skink.scale;
            const float breathing = 1.0f + .035f * std::sin(skink.breathPhase);
            const Vector3 centre(skink.x, skink.y + .060f * scale, skink.z);
            const Vector3 forward(std::cos(skink.headingRadians), 0.0f,
                                  std::sin(skink.headingRadians));
            const Vector3 right(-forward.Z, 0.0f, forward.X);
            const Vector3 up = Vector3::Up;
            constexpr std::array<float, 6> BodyStations =
                {{-.28f, -.20f, -.08f, .08f, .22f, .30f}};
            constexpr std::array<float, 6> BodyWidths =
                {{.045f, .080f, .105f, .108f, .085f, .048f}};
            constexpr std::array<float, 6> BodyHeights =
                {{.032f, .055f, .068f, .070f, .057f, .034f}};
            std::array<float, 6> breathingHeights = BodyHeights;
            for (float& height : breathingHeights) height *= breathing;
            AppendGeckoLoft(vertices, centre, right, up, forward, BodyStations,
                            BodyWidths, breathingHeights, scale, back, flank, underside);

            const Vector3 head = centre + forward * (.345f * scale) + up * (.006f * scale);
            constexpr std::array<float, 4> HeadStations = {{-.045f, .02f, .105f, .16f}};
            constexpr std::array<float, 4> HeadWidths = {{.050f, .078f, .070f, .036f}};
            constexpr std::array<float, 4> HeadHeights = {{.035f, .052f, .046f, .025f}};
            AppendGeckoLoft(vertices, head, right, up, forward, HeadStations,
                            HeadWidths, HeadHeights, scale, back, flank, underside);

            const float stride = std::sin(skink.gaitPhase) * .045f * scale;
            for (const float side : {-1.0f, 1.0f}) {
                for (int pair = 0; pair < 2; ++pair) {
                    const float along = pair == 0 ? .17f : -.15f;
                    const float phase = ((pair == 0) == (side > 0.0f)) ? stride : -stride;
                    const Vector3 root = centre + forward * (along * scale)
                        + right * (side * .065f * scale);
                    const Vector3 elbow = root + right * (side * .075f * scale)
                        + forward * phase - up * (.020f * scale);
                    const Vector3 foot = elbow + right * (side * .082f * scale)
                        + forward * ((pair == 0 ? .030f : -.026f) * scale - phase * .35f)
                        - up * (.012f * scale);
                    AppendGeckoTube(vertices,root,elbow,.016f*scale,.012f*scale,flank);
                    AppendGeckoTube(vertices,elbow,foot,.012f*scale,.008f*scale,back);
                    for (int toe = -1; toe <= 1; ++toe) {
                        const Vector3 tip = foot + right * (side * .040f * scale)
                            + forward * ((static_cast<float>(toe) * .018f
                                + (pair == 0 ? .010f : -.010f)) * scale);
                        AppendGeckoTube(vertices,foot,tip,.0048f*scale,
                                        .0028f*scale,underside,5);
                    }
                }
            }

            Vector3 tailRoot = centre - forward * (.275f * scale);
            for (int segment = 0; segment < 5; ++segment) {
                const float length = (.145f - static_cast<float>(segment) * .014f) * scale;
                const Vector3 tailEnd = tailRoot - forward * length
                    + right * (std::sin(skink.gaitPhase * .24f
                        + static_cast<float>(segment) * .66f) * .025f * scale);
                const float rootRadius=(.031f-static_cast<float>(segment)*.0048f)*scale;
                const float tipRadius=std::max(.0055f,
                    .0262f-static_cast<float>(segment)*.0048f)*scale;
                AppendGeckoTube(vertices,tailRoot,tailEnd,rootRadius,tipRadius,flank);
                tailRoot = tailEnd;
            }

            // Two narrow dorsal lines distinguish the smooth skink from the spotted,
            // broader-headed gecko while remaining readable at natural ground distance.
            for (const float side : {-1.0f, 1.0f}) {
                Vector3 stripeRoot = centre - forward * (.23f * scale)
                    + right * (side * .048f * scale) + up * (.071f * scale);
                for (int segment = 0; segment < 4; ++segment) {
                    const Vector3 stripeEnd = centre
                        + forward * ((-.23f + static_cast<float>(segment + 1) * .13f) * scale)
                        + right * (side * (.048f - static_cast<float>(segment) * .004f) * scale)
                        + up * (.073f * scale);
                    AppendGeckoSegment(vertices, stripeRoot, stripeEnd, .0050f * scale, stripe);
                    stripeRoot = stripeEnd;
                }
            }
            for (const float side : {-1.0f, 1.0f}) {
                const Vector3 eyeCentre = head + forward * (.065f * scale)
                    + right * (side * .071f * scale) + up * (.038f * scale);
                AppendGeckoDiamond(vertices, eyeCentre, right, up, forward,
                                   .014f * scale, .015f * scale, .011f * scale, eye, eye);
                const Vector3 highlight = eyeCentre + right * (side * .011f * scale)
                    + forward * (.006f * scale) + up * (.008f * scale);
                AddDoubleSidedTriangle(vertices, highlight + up * (.0040f * scale),
                    highlight - forward * (.0040f * scale) - up * (.0025f * scale),
                    highlight + forward * (.0040f * scale) - up * (.0025f * scale), glint);
            }
        }

        void AppendDragonfly(std::vector<VertexPositionColor>& vertices,
                             const DragonflyState& fly)
        {
            const std::array<Color, 3> bodies = {{
                Color(31, 96, 101, 255), Color(35, 73, 117, 255),
                Color(46, 103, 61, 255)}};
            const Color bodyColor = bodies[fly.colorVariant % bodies.size()];
            const Color dark = Color::Lerp(bodyColor, Color(10, 17, 18, 255), .68f);
            const Color abdomenBand = Color::Lerp(bodyColor, Color(174, 178, 93, 255), .18f);
            const Color wing(132, 153, 151, 255);
            const Color wingVein(65, 86, 84, 255);
            const Color eye(83, 111, 53, 255);
            const Color eyeGlint(185, 204, 151, 255);
            const float s = fly.scale;
            const Vector3 centre(fly.x, fly.y, fly.z);
            const Vector3 forward(std::cos(fly.headingRadians), 0,
                                  std::sin(fly.headingRadians));
            const Vector3 levelRight(-forward.Z, 0, forward.X);
            const float bankCos = std::cos(fly.bankRadians);
            const float bankSin = std::sin(fly.bankRadians);
            const Vector3 right = levelRight * bankCos + Vector3::Up * bankSin;
            const Vector3 up = Vector3::Up * bankCos - levelRight * bankSin;

            constexpr std::array<float, 5> ThoraxStations = {{-.065f, -.035f, .010f, .050f, .080f}};
            constexpr std::array<float, 5> ThoraxWidths = {{.022f, .050f, .057f, .046f, .020f}};
            constexpr std::array<float, 5> ThoraxHeights = {{.018f, .040f, .047f, .037f, .017f}};
            AppendGeckoLoft(vertices, centre, right, up, forward, ThoraxStations,
                            ThoraxWidths, ThoraxHeights, s, bodyColor, dark, dark);

            const Vector3 head = centre + forward * (.098f * s) + up * (.006f * s);
            constexpr std::array<float, 4> HeadStations = {{-.040f, -.010f, .028f, .052f}};
            constexpr std::array<float, 4> HeadWidths = {{.026f, .052f, .050f, .024f}};
            constexpr std::array<float, 4> HeadHeights = {{.022f, .040f, .038f, .019f}};
            AppendGeckoLoft(vertices, head, right, up, forward, HeadStations,
                            HeadWidths, HeadHeights, s, dark, dark, dark);

            // Seven visibly separated, steadily tapering abdominal segments give
            // the insect its characteristic long needle silhouette.
            Vector3 root = centre - forward * (.052f * s);
            for (int segment = 0; segment < 7; ++segment) {
                const float length = (.058f - .0025f * static_cast<float>(segment)) * s;
                const Vector3 end = root - forward * length
                    + right * (std::sin(fly.wingPhase * .08f
                                        + static_cast<float>(segment) * .42f)
                               * .0025f * s);
                const float radius = (.018f - .0017f * static_cast<float>(segment)) * s;
                const float tipRadius = std::max(.0045f,
                    .0163f - .0017f * static_cast<float>(segment)) * s;
                AppendGeckoTube(vertices, root, end, radius, tipRadius,
                    segment % 2 == 0 ? bodyColor : abdomenBand, 6);
                root = end;
            }

            // Opaque vertex colour is used by this shared portable batch, so use
            // slim four-corner wings and darker veins instead of broad pale planes.
            const float beat = std::sin(fly.wingPhase) * .030f * s;
            for (const float side : {-1.0f, 1.0f}) {
                for (int pair = 0; pair < 2; ++pair) {
                    const float rootForward = pair == 0 ? .030f : -.020f;
                    const float reach = pair == 0 ? .235f : .205f;
                    const float tipForward = pair == 0 ? .080f : -.095f;
                    const float rearForward = pair == 0 ? .035f : -.135f;
                    const Vector3 base = centre + forward * (rootForward * s)
                        + right * (side * .025f * s);
                    const Vector3 shoulder = base + right * (side * .050f * s)
                        + forward * ((pair == 0 ? .018f : -.012f) * s);
                    const Vector3 tip = base + right * (side * reach * s)
                        + forward * (tipForward * s)
                        + up * (beat * (side > 0 ? 1.0f : -1.0f));
                    const Vector3 trailing = base + right * (side * reach * .72f * s)
                        + forward * (rearForward * s)
                        + up * (beat * .72f * (side > 0 ? 1.0f : -1.0f));
                    AddDoubleSidedTriangle(vertices, base, shoulder, tip, wing);
                    AddDoubleSidedTriangle(vertices, base, tip, trailing, wing);
                    AppendGeckoSegment(vertices, base, tip, .0032f * s, wingVein);
                    AppendGeckoSegment(vertices, shoulder, trailing, .0024f * s, wingVein);
                }
            }

            // Six swept legs remain subordinate to the wing silhouette but stop a
            // close individual from looking like a finned vehicle.
            for (int leg = 0; leg < 3; ++leg) {
                for (const float side : {-1.0f, 1.0f}) {
                    const Vector3 hip = centre + forward * ((.035f - leg * .034f) * s)
                        + right * (side * .035f * s) - up * (.022f * s);
                    const Vector3 knee = hip + right * (side * (.040f + leg * .006f) * s)
                        - forward * ((.018f + leg * .018f) * s) - up * (.034f * s);
                    const Vector3 foot = knee + right * (side * .030f * s)
                        - forward * ((.020f + leg * .014f) * s) + up * (.007f * s);
                    AppendGeckoTube(vertices, hip, knee, .0038f * s,
                                    .0028f * s, dark, 5);
                    AppendGeckoTube(vertices, knee, foot, .0028f * s,
                                    .0016f * s, dark, 5);
                }
            }

            for (const float side : {-1.0f, 1.0f}) {
                const Vector3 eyeCentre = head + right * (side * .040f * s)
                    + forward * (.020f * s) + up * (.014f * s);
                for (int edge = 0; edge < 8; ++edge) {
                    const float firstAngle = 6.28318530718f * static_cast<float>(edge) / 8.0f;
                    const float secondAngle = 6.28318530718f * static_cast<float>(edge + 1) / 8.0f;
                    const Vector3 first = eyeCentre
                        + forward * (std::cos(firstAngle) * .022f * s)
                        + up * (std::sin(firstAngle) * .025f * s);
                    const Vector3 second = eyeCentre
                        + forward * (std::cos(secondAngle) * .022f * s)
                        + up * (std::sin(secondAngle) * .025f * s);
                    AddDoubleSidedTriangle(vertices, eyeCentre, first, second, eye);
                }
                const Vector3 glint = eyeCentre + right * (side * .001f * s)
                    + forward * (.010f * s) + up * (.013f * s);
                AddDoubleSidedTriangle(vertices, glint + up * (.005f * s),
                    glint - forward * (.005f * s) - up * (.003f * s),
                    glint + forward * (.005f * s) - up * (.003f * s), eyeGlint);
            }
        }

        void AppendFrog(std::vector<VertexPositionColor>& vertices, const FrogState& frog)
        {
            const std::array<Color, 4> backs = {{
                Color(76, 111, 49, 255), Color(102, 126, 55, 255),
                Color(68, 96, 63, 255), Color(117, 105, 55, 255)}};
            const Color back = backs[frog.colorVariant % backs.size()];
            const Color flank = Color::Lerp(back, Color(35, 50, 30, 255), .34f);
            const Color underside = Color::Lerp(back, Color(204, 190, 132, 255), .62f);
            const Color marking = Color::Lerp(back, Color(30, 48, 25, 255), .62f);
            const Color iris(176, 150, 54, 255);
            const Color pupil(9, 13, 7, 255);
            const Color glint(223, 218, 171, 255);
            const float scale = frog.scale;
            const float hop = std::sin(3.14159265359f * frog.hopPhase);
            const float extension = frog.behavior == FrogBehavior::Rest ? 0.0f : hop;
            const float breathing = 1.0f + .045f * std::sin(frog.breathPhase);
            const Vector3 forward(std::cos(frog.headingRadians), 0.0f,
                                  std::sin(frog.headingRadians));
            const Vector3 right(-forward.Z, 0.0f, forward.X);
            const Vector3 up = Vector3::Up;
            const Vector3 centre(frog.x, frog.y + .095f * scale, frog.z);

            constexpr std::array<float, 5> BodyStations = {{-.25f, -.14f, .02f, .17f, .25f}};
            constexpr std::array<float, 5> BodyWidths = {{.10f, .22f, .27f, .23f, .12f}};
            constexpr std::array<float, 5> BodyHeights = {{.07f, .15f, .18f, .15f, .08f}};
            std::array<float, 5> bodyHeights = BodyHeights;
            for (float& height : bodyHeights) height *= breathing;
            AppendGeckoLoft(vertices, centre, right, up, forward, BodyStations,
                            BodyWidths, bodyHeights, scale, back, flank, underside);

            const Vector3 head = centre + forward * (.265f * scale) + up * (.035f * scale);
            constexpr std::array<float, 4> HeadStations = {{-.08f, .0f, .12f, .19f}};
            constexpr std::array<float, 4> HeadWidths = {{.17f, .25f, .23f, .13f}};
            constexpr std::array<float, 4> HeadHeights = {{.10f, .16f, .14f, .07f}};
            AppendGeckoLoft(vertices, head, right, up, forward, HeadStations,
                            HeadWidths, HeadHeights, scale, back, flank, underside);

            // Long folded hind legs and shorter forelegs preserve the characteristic
            // crouched silhouette; the hop phase extends both pairs without a skeleton.
            for (const float side : {-1.0f, 1.0f}) {
                const Vector3 hip = centre - forward * (.10f * scale)
                    + right * (side * .18f * scale);
                const Vector3 knee = hip - forward * ((.18f + extension * .17f) * scale)
                    + right * (side * (.19f - extension * .05f) * scale)
                    - up * ((.035f - extension * .055f) * scale);
                const Vector3 ankle = knee + forward * ((.23f + extension * .24f) * scale)
                    + right * (side * .07f * scale) - up * (.035f * scale);
                const Vector3 rearToe = ankle + forward * (.18f * scale)
                    + right * (side * .025f * scale);
                AppendGeckoTube(vertices, hip, knee, .062f * scale, .047f * scale, flank);
                AppendGeckoTube(vertices, knee, ankle, .047f * scale, .032f * scale, back);
                AppendGeckoTube(vertices, ankle, rearToe, .030f * scale, .017f * scale, underside);
                std::array<Vector3, 3> rearTips;
                for (int toe = -1; toe <= 1; ++toe) {
                    const Vector3 tip = rearToe + forward * (.095f * scale)
                        + right * ((side * .025f + static_cast<float>(toe) * .045f) * scale);
                    rearTips[static_cast<std::size_t>(toe + 1)] = tip;
                    AppendGeckoTube(vertices, rearToe, tip, .010f * scale,
                                    .0045f * scale, underside, 5);
                }
                for (int web = 0; web < 2; ++web) {
                    AddDoubleSidedTriangle(vertices, rearToe,
                        rearTips[static_cast<std::size_t>(web)],
                        rearTips[static_cast<std::size_t>(web + 1)], underside);
                }

                const Vector3 shoulder = head - forward * (.075f * scale)
                    + right * (side * .17f * scale) - up * (.04f * scale);
                const Vector3 wrist = shoulder + forward * ((.15f + extension * .10f) * scale)
                    + right * (side * .055f * scale) - up * (.11f * scale);
                AppendGeckoTube(vertices, shoulder, wrist, .031f * scale,
                                .017f * scale, flank);
                for (int toe = -1; toe <= 1; ++toe) {
                    const Vector3 tip = wrist + forward * (.095f * scale)
                        + right * ((side * .025f + static_cast<float>(toe) * .025f) * scale);
                    AppendGeckoTube(vertices, wrist, tip, .0085f * scale,
                                    .0038f * scale, underside, 5);
                }
            }

            // Raised dorsolateral folds and a few restrained spots break the smooth green shell.
            for (const float side : {-1.0f, 1.0f}) {
                const Vector3 foldA = centre - forward * (.16f * scale)
                    + right * (side * .13f * scale) + up * (.15f * scale);
                const Vector3 foldB = centre + forward * (.19f * scale)
                    + right * (side * .15f * scale) + up * (.13f * scale);
                AppendGeckoTube(vertices, foldA, foldB, .011f * scale,
                                .008f * scale, marking, 5);
            }
            for (int spot = 0; spot < 3; ++spot) {
                const float side = spot == 1 ? -1.0f : 1.0f;
                const Vector3 spotCentre = centre + forward * ((-.12f + spot * .12f) * scale)
                    + right * (side * .09f * scale) + up * (.18f * scale);
                AddDoubleSidedTriangle(vertices, spotCentre + forward * (.025f * scale),
                    spotCentre - right * (.025f * scale) - forward * (.018f * scale),
                    spotCentre + right * (.025f * scale) - forward * (.018f * scale), marking);
            }

            for (const float side : {-1.0f, 1.0f}) {
                const Vector3 eyeCentre = head + forward * (.045f * scale)
                    + right * (side * .195f * scale) + up * (.115f * scale);
                AppendGeckoDiamond(vertices, eyeCentre, right, up, forward,
                                   .060f * scale, .058f * scale, .046f * scale, back, flank);
                const Vector3 irisCentre = eyeCentre + right * (side * .052f * scale)
                    + forward * (.012f * scale) + up * (.004f * scale);
                for (int edge = 0; edge < 8; ++edge) {
                    const float firstAngle = 6.28318530718f * static_cast<float>(edge) / 8.0f;
                    const float secondAngle = 6.28318530718f * static_cast<float>(edge + 1) / 8.0f;
                    const Vector3 first = irisCentre
                        + forward * (std::cos(firstAngle) * .025f * scale)
                        + up * (std::sin(firstAngle) * .030f * scale);
                    const Vector3 second = irisCentre
                        + forward * (std::cos(secondAngle) * .025f * scale)
                        + up * (std::sin(secondAngle) * .030f * scale);
                    AddDoubleSidedTriangle(vertices, irisCentre, first, second, iris);
                }
                const Vector3 pupilCentre = irisCentre + right * (side * .002f * scale);
                AddDoubleSidedTriangle(vertices, pupilCentre + up * (.035f * scale),
                    pupilCentre - forward * (.012f * scale) - up * (.035f * scale),
                    pupilCentre + forward * (.012f * scale) - up * (.035f * scale), pupil);
                const Vector3 highlight = pupilCentre + up * (.020f * scale)
                    + right * (side * .004f * scale);
                AddDoubleSidedTriangle(vertices, highlight + up * (.007f * scale),
                    highlight - forward * (.006f * scale) - up * (.004f * scale),
                    highlight + forward * (.006f * scale) - up * (.004f * scale), glint);
            }

            const Vector3 snout = head + forward * (.194f * scale);
            for (const float side : {-1.0f, 1.0f}) {
                const Vector3 nostril = snout + right * (side * .060f * scale)
                    + up * (.026f * scale);
                AddDoubleSidedTriangle(vertices, nostril + up * (.007f * scale),
                    nostril - right * (side * .007f * scale) - up * (.005f * scale),
                    nostril + right * (side * .007f * scale) - up * (.005f * scale), pupil);
            }
            const Vector3 mouthCentre = snout - up * (.054f * scale);
            const Vector3 mouthLeft = mouthCentre - right * (.115f * scale)
                + forward * (.001f * scale) + up * (.012f * scale);
            const Vector3 mouthRight = mouthCentre + right * (.115f * scale)
                + forward * (.001f * scale) + up * (.012f * scale);
            AppendGeckoTube(vertices, mouthLeft, mouthCentre, .006f * scale,
                            .004f * scale, marking, 5);
            AppendGeckoTube(vertices, mouthCentre, mouthRight, .004f * scale,
                            .006f * scale, marking, 5);
        }

        void AppendFly(std::vector<VertexPositionColor>& vertices, const FlyState& fly)
        {
            const Vector3 forward(std::cos(fly.headingRadians), 0.0f,
                                  std::sin(fly.headingRadians));
            const Vector3 right(-forward.Z, 0.0f, forward.X), up = Vector3::Up;
            const float scale = .12f * fly.scale;
            const std::array<Color, 3> bodies = {{Color(38, 35, 30), Color(52, 45, 34),
                                                  Color(29, 31, 29)}};
            const Color body = bodies[fly.colorVariant % bodies.size()];
            const Color bodyFlank = Color::Lerp(body, Color(13, 15, 13), .34f);
            const Color abdomen = Color::Lerp(body, Color(72, 76, 55), .24f);
            const Color underside = Color::Lerp(body, Color(87, 78, 61), .18f);
            const Color eye(126, 42, 31), eyeGlint(221, 174, 111);
            const Color wing(166, 178, 171), vein(76, 82, 76), leg(31, 28, 24);
            const Vector3 centre(fly.x, fly.y, fly.z);
            const Vector3 thorax = centre + forward * (.035f * scale);
            constexpr std::array<float, 5> ThoraxStations = {{-.12f, -.07f, .01f, .09f, .14f}};
            constexpr std::array<float, 5> ThoraxWidths = {{.045f, .088f, .105f, .094f, .047f}};
            constexpr std::array<float, 5> ThoraxHeights = {{.040f, .077f, .093f, .082f, .041f}};
            AppendGeckoLoft(vertices, thorax, right, up, forward, ThoraxStations,
                            ThoraxWidths, ThoraxHeights, scale, body, bodyFlank, underside);

            const Vector3 abdomenCentre = centre - forward * (.165f * scale)
                - up * (.008f * scale);
            constexpr std::array<float, 5> AbdomenStations = {{-.18f, -.11f, .0f, .11f, .17f}};
            constexpr std::array<float, 5> AbdomenWidths = {{.025f, .060f, .080f, .068f, .035f}};
            constexpr std::array<float, 5> AbdomenHeights = {{.022f, .052f, .067f, .058f, .029f}};
            AppendGeckoLoft(vertices, abdomenCentre, right, up, forward, AbdomenStations,
                            AbdomenWidths, AbdomenHeights, scale, abdomen, bodyFlank, underside);

            const Vector3 head = centre + forward * (.205f * scale)
                + up * (.004f * scale);
            constexpr std::array<float, 4> HeadStations = {{-.060f, -.015f, .045f, .085f}};
            constexpr std::array<float, 4> HeadWidths = {{.043f, .076f, .073f, .035f}};
            constexpr std::array<float, 4> HeadHeights = {{.038f, .067f, .064f, .031f}};
            AppendGeckoLoft(vertices, head, right, up, forward, HeadStations,
                            HeadWidths, HeadHeights, scale, body, bodyFlank, underside);
            const float beat = std::sin(fly.wingPhase);
            for (const float side : {-1.0f, 1.0f}) {
                const Vector3 root = centre - forward * (.015f * scale)
                    + right * (side * .065f * scale) + up * (.068f * scale);
                const Vector3 leading = root + forward * (.060f * scale)
                    + right * (side * .040f * scale);
                const Vector3 tip = root + right * (side * .245f * scale)
                    - forward * (.155f * scale) + up * (beat * .085f * scale);
                const Vector3 rear = root + right * (side * .145f * scale)
                    - forward * (.235f * scale) + up * (beat * .055f * scale);
                const Vector3 trailing = root - forward * (.105f * scale)
                    + right * (side * .030f * scale);
                AddDoubleSidedTriangle(vertices, leading, tip, rear, wing);
                AddDoubleSidedTriangle(vertices, leading, rear, trailing, wing);
                AppendGeckoSegment(vertices, root, tip, .0035f * scale, vein);
                AppendGeckoSegment(vertices, root, rear, .0028f * scale, vein);

                const Vector3 eyeCentre = head + forward * (.030f * scale)
                    + right * (side * .072f * scale) + up * (.008f * scale);
                for (int edge = 0; edge < 8; ++edge) {
                    const float firstAngle = 6.28318530718f * static_cast<float>(edge) / 8.0f;
                    const float secondAngle = 6.28318530718f * static_cast<float>(edge + 1) / 8.0f;
                    const Vector3 first = eyeCentre
                        + forward * (std::cos(firstAngle) * .037f * scale)
                        + up * (std::sin(firstAngle) * .044f * scale);
                    const Vector3 second = eyeCentre
                        + forward * (std::cos(secondAngle) * .037f * scale)
                        + up * (std::sin(secondAngle) * .044f * scale);
                    AddDoubleSidedTriangle(vertices, eyeCentre, first, second, eye);
                }
                const Vector3 highlight = eyeCentre + forward * (.015f * scale)
                    + up * (.020f * scale) + right * (side * .001f * scale);
                AddDoubleSidedTriangle(vertices, highlight + up * (.006f * scale),
                    highlight - forward * (.005f * scale) - up * (.004f * scale),
                    highlight + forward * (.005f * scale) - up * (.004f * scale), eyeGlint);
                for (int legIndex = 0; legIndex < 3; ++legIndex) {
                    const Vector3 hip = centre + forward * ((.09f - legIndex * .08f) * scale)
                        + right * (side * .05f * scale) - up * (.035f * scale);
                    const Vector3 knee = hip + right * (side * (.070f + .018f * legIndex) * scale)
                        + forward * ((.055f - .052f * legIndex) * scale)
                        - up * (.055f * scale);
                    const Vector3 foot = knee + right * (side * (.075f + .020f * legIndex) * scale)
                        - forward * ((.02f + .04f * legIndex) * scale)
                        - up * (.035f * scale);
                    AppendGeckoTube(vertices, hip, knee, .009f * scale,
                                    .006f * scale, leg, 5);
                    AppendGeckoTube(vertices, knee, foot, .006f * scale,
                                    .003f * scale, leg, 5);
                }

                const Vector3 antennaRoot = head + forward * (.078f * scale)
                    + right * (side * .023f * scale) + up * (.018f * scale);
                const Vector3 antennaTip = antennaRoot + forward * (.070f * scale)
                    + right * (side * .028f * scale) + up * (.018f * scale);
                AppendGeckoSegment(vertices, antennaRoot, antennaTip, .003f * scale, leg);
            }
        }

        void AppendAntBody(std::vector<VertexPositionColor>& vertices,const Vector3& centre,
                           const Vector3& right,const Vector3& up,const Vector3& forward,
                           const float halfWidth,const float halfHeight,const float halfLength,
                           const Color& dorsal,const Color& flank)
        {
            constexpr int Sides = 8;
            constexpr std::array<float, 5> Stations = {{-1.0f, -.64f, 0.0f, .64f, 1.0f}};
            constexpr std::array<float, 5> Radii = {{.08f, .76f, 1.0f, .76f, .08f}};
            const auto point = [&](const std::size_t station, const int side) {
                const float angle = 6.28318530718f * static_cast<float>(side) / Sides;
                return centre + forward * (Stations[station] * halfLength)
                    + right * (std::cos(angle) * Radii[station] * halfWidth)
                    + up * (std::sin(angle) * Radii[station] * halfHeight);
            };
            for (std::size_t station = 0; station + 1U < Stations.size(); ++station) {
                for (int side = 0; side < Sides; ++side) {
                    const int next = (side + 1) % Sides;
                    const float vertical = std::sin(6.28318530718f
                        * (static_cast<float>(side) + .5f) / Sides);
                    const Color color = vertical > .20f ? dorsal : flank;
                    AddTriangle(vertices, point(station, side),
                                point(station + 1U, side),
                                point(station + 1U, next), color);
                    AddTriangle(vertices, point(station, side),
                                point(station + 1U, next),
                                point(station, next), color);
                }
            }
            const Vector3 rear = centre - forward * halfLength;
            const Vector3 front = centre + forward * halfLength;
            for (int side = 0; side < Sides; ++side) {
                const int next = (side + 1) % Sides;
                AddTriangle(vertices, rear, point(0U, next), point(0U, side), flank);
                AddTriangle(vertices, front, point(4U, side), point(4U, next), dorsal);
            }
        }

        void AppendAnt(std::vector<VertexPositionColor>& vertices, const AntState& ant)
        {
            const Vector3 forward = Vector3::Normalize(Vector3(
                ant.forwardX,ant.forwardY,ant.forwardZ));
            const Vector3 up = Vector3::Normalize(Vector3(
                ant.normalX,ant.normalY,ant.normalZ));
            const Vector3 right = Vector3::Normalize(Vector3::Cross(up,forward));
            const float unit=.0115f*ant.scale;
            const std::array<Color,3> bodies{{Color(43,25,18),Color(59,31,20),Color(30,25,21)}};
            const Color body=bodies[ant.colorVariant%bodies.size()];
            const Color flank=Color::Lerp(body,Color(9,7,6),.32f);
            const Color leg=Color::Lerp(body,Color(12,10,8),.50f);
            const Color eye(12,9,7), eyeGlint(191,151,105);
            const Vector3 centre(ant.x,ant.y,ant.z);
            const Vector3 thorax = centre + up * (.31f * unit);
            AppendAntBody(vertices,thorax,right,up,forward,
                .35f*unit,.31f*unit,.40f*unit,body,flank);
            const Vector3 head=thorax+forward*(.70f*unit)+up*(.015f*unit);
            AppendAntBody(vertices,head,right,up,forward,
                .32f*unit,.29f*unit,.34f*unit,body,flank);
            const Vector3 abdomen=thorax-forward*(1.04f*unit)+up*(.04f*unit);
            AppendAntBody(vertices,abdomen,right,up,forward,
                .50f*unit,.43f*unit,.63f*unit,body,flank);
            AppendGeckoDiamond(vertices, thorax - forward * (.43f * unit),
                right, up, forward, .14f * unit, .13f * unit, .13f * unit,
                body, flank);
            AppendGeckoDiamond(vertices, thorax - forward * (.61f * unit),
                right, up, forward, .12f * unit, .12f * unit, .12f * unit,
                body, flank);
            const float step=std::sin(ant.gaitPhase);
            for(const float side:{-1.0f,1.0f}) for(int legIndex=0;legIndex<3;++legIndex) {
                const float along=(.36f-static_cast<float>(legIndex)*.38f)*unit;
                const float swing=step*side*(legIndex%2==0?1.0f:-1.0f)*.18f*unit;
                const Vector3 hip=thorax+forward*along+right*(side*.25f*unit);
                const Vector3 knee=hip+right*(side*.64f*unit)
                    +forward*((.18f-static_cast<float>(legIndex)*.18f)*unit+swing)
                    +up*(.10f*unit);
                const Vector3 ankle=knee+right*(side*.46f*unit)
                    +forward*((.28f-static_cast<float>(legIndex)*.28f)*unit+swing)
                    -up*(.18f*unit);
                const Vector3 foot=ankle+right*(side*.30f*unit)
                    +forward*((.15f-static_cast<float>(legIndex)*.14f)*unit)
                    -up*(.12f*unit);
                AppendGeckoTube(vertices,hip,knee,.060f*unit,.046f*unit,leg,5);
                AppendGeckoTube(vertices,knee,ankle,.046f*unit,.030f*unit,leg,5);
                AppendGeckoTube(vertices,ankle,foot,.030f*unit,.010f*unit,leg,5);
                AppendGeckoDiamond(vertices,knee,right,up,forward,
                    .052f*unit,.045f*unit,.052f*unit,leg,flank);
            }
            for(const float side:{-1.0f,1.0f}) {
                const Vector3 eyeCentre=head+forward*(.16f*unit)
                    +right*(side*.27f*unit)+up*(.08f*unit);
                AppendGeckoDiamond(vertices,eyeCentre,right,up,forward,
                    .050f*unit,.050f*unit,.038f*unit,eye,eye);
                AddDoubleSidedTriangle(vertices,eyeCentre+up*(.052f*unit),
                    eyeCentre+forward*(.025f*unit),
                    eyeCentre+right*(side*.018f*unit),eyeGlint);
                const Vector3 root=head+forward*(.27f*unit)+right*(side*.16f*unit);
                const Vector3 elbow=root+forward*(.43f*unit)+right*(side*.24f*unit)
                    +up*(.12f*unit);
                const Vector3 tip=elbow+forward*(.34f*unit)+right*(side*.25f*unit)
                    -up*(.08f*unit);
                AppendGeckoTube(vertices,root,elbow,.032f*unit,.022f*unit,leg,5);
                AppendGeckoTube(vertices,elbow,tip,.022f*unit,.009f*unit,leg,5);

                const Vector3 mandibleRoot=head+forward*(.29f*unit)
                    +right*(side*.13f*unit)-up*(.07f*unit);
                const Vector3 mandibleTip=mandibleRoot+forward*(.27f*unit)
                    -right*(side*.08f*unit)-up*(.04f*unit);
                AppendGeckoTube(vertices,mandibleRoot,mandibleTip,
                    .040f*unit,.010f*unit,leg,5);
            }
            if(ant.behavior==AntBehavior::Carry) {
                const Color leaf(76,112,42);
                const Color leafShade(58,91,34);
                const Vector3 longAxis=Vector3::Normalize(up*.97f+right*.16f);
                const Vector3 leafAcross=Vector3::Normalize(forward-right*.04f);
                const Vector3 leafCentre=head+forward*(.22f*unit)+up*(1.18f*unit);
                for(int edge=0;edge<8;++edge) {
                    const float a=6.28318530718f*static_cast<float>(edge)/8.0f;
                    const float b=6.28318530718f*static_cast<float>(edge+1)/8.0f;
                    const Vector3 first=leafCentre+leafAcross*(std::sin(a)*.62f*unit)
                        +longAxis*(std::cos(a)*1.24f*unit);
                    const Vector3 second=leafCentre+leafAcross*(std::sin(b)*.62f*unit)
                        +longAxis*(std::cos(b)*1.24f*unit);
                    AddDoubleSidedTriangle(vertices,leafCentre,first,second,
                        edge<4?leaf:leafShade);
                }
                const Vector3 leafBase=leafCentre-longAxis*(1.24f*unit);
                AppendGeckoTube(vertices,head+forward*(.30f*unit),leafBase,
                    .025f*unit,.015f*unit,leafShade,5);
                AppendGeckoTube(vertices,leafBase,
                    leafCentre+longAxis*(1.12f*unit),.015f*unit,.007f*unit,
                    leafShade,5);
            }
        }

        void AppendCicada(std::vector<VertexPositionColor>& vertices,
                          const CicadaState& cicada)
        {
            const Vector3 forward = Vector3::Normalize(Vector3(
                cicada.forwardX, cicada.forwardY, cicada.forwardZ));
            const Vector3 dorsal = Vector3::Normalize(Vector3(
                cicada.normalX, cicada.normalY, cicada.normalZ));
            const Vector3 right = Vector3::Normalize(Vector3::Cross(dorsal, forward));
            const float unit = .026f * cicada.scale;
            const std::array<Color, 3> bodies = {{Color(102, 88, 45), Color(69, 88, 48),
                                                  Color(91, 69, 46)}};
            const Color body = bodies[cicada.colorVariant % bodies.size()];
            const Color flank = Color::Lerp(body, Color(22, 20, 15), .45f);
            const Color wing = Color::Lerp(body, Color(133, 143, 94), .42f);
            const Color vein = Color::Lerp(body, Color(27, 31, 20), .61f);
            const Color legColor = Color::Lerp(body, Color(164, 142, 77), .20f);
            const Color eye(119, 57, 39), glint(232, 197, 127);
            const Vector3 centre(cicada.x, cicada.y, cicada.z);
            const Vector3 thorax = centre + dorsal * (.34f * unit);
            constexpr std::array<float,5> ThoraxStations={{-.50f,-.34f,-.10f,.24f,.47f}};
            constexpr std::array<float,5> ThoraxWidths={{.20f,.52f,.62f,.54f,.20f}};
            constexpr std::array<float,5> ThoraxHeights={{.18f,.38f,.46f,.38f,.16f}};
            AppendGeckoLoft(vertices,thorax,right,dorsal,forward,ThoraxStations,
                ThoraxWidths,ThoraxHeights,unit,
                Color::Lerp(body,Color(155,132,64),.14f),flank,flank);
            const Vector3 head = thorax + forward * (.63f * unit);
            constexpr std::array<float, 4> HeadStations = {{-.30f, -.11f, .17f, .33f}};
            constexpr std::array<float, 4> HeadWidths = {{.30f, .52f, .49f, .23f}};
            constexpr std::array<float, 4> HeadHeights = {{.20f, .33f, .30f, .14f}};
            AppendGeckoLoft(vertices, head, right, dorsal, forward, HeadStations,
                            HeadWidths, HeadHeights, unit, body, flank, flank);
            const Vector3 abdomen=thorax-forward*(.78f*unit)-dorsal*(.035f*unit);
            constexpr std::array<float,6> AbdomenStations={{-.65f,-.42f,-.15f,.13f,.40f,.58f}};
            constexpr std::array<float,6> AbdomenWidths={{.12f,.26f,.37f,.44f,.42f,.20f}};
            constexpr std::array<float,6> AbdomenHeights={{.11f,.22f,.28f,.31f,.28f,.14f}};
            AppendGeckoLoft(vertices,abdomen,right,dorsal,forward,AbdomenStations,
                AbdomenWidths,AbdomenHeights,unit,body,flank,
                Color::Lerp(body,Color(166,151,94),.18f));
            const Color marking=Color::Lerp(body,Color(32,29,20),.35f);
            AppendGeckoSpot(vertices,thorax+dorsal*(.47f*unit)+forward*(.04f*unit),
                right,forward,.23f*unit,.16f*unit,marking);
            constexpr std::array<float,4> BandOffsets={{-.35f,-.10f,.15f,.38f}};
            constexpr std::array<float,4> BandWidths={{.22f,.34f,.39f,.32f}};
            constexpr std::array<float,4> BandHeights={{.22f,.28f,.31f,.27f}};
            for(std::size_t band=0;band<BandOffsets.size();++band) {
                const Vector3 bandCentre=abdomen+forward*(BandOffsets[band]*unit)
                    +dorsal*((BandHeights[band]+.012f)*unit);
                const Vector3 leftFront=bandCentre-right*(BandWidths[band]*unit)
                    +forward*(.018f*unit);
                const Vector3 rightFront=bandCentre+right*(BandWidths[band]*unit)
                    +forward*(.018f*unit);
                const Vector3 leftRear=bandCentre-right*(BandWidths[band]*unit)
                    -forward*(.018f*unit);
                const Vector3 rightRear=bandCentre+right*(BandWidths[band]*unit)
                    -forward*(.018f*unit);
                AddDoubleSidedTriangle(vertices,leftFront,rightFront,rightRear,marking);
                AddDoubleSidedTriangle(vertices,leftFront,rightRear,leftRear,marking);
            }

            for (const float side : {-1.0f, 1.0f}) {
                const float beat = cicada.behavior == CicadaBehavior::Flight
                    ? std::sin(cicada.wingPhase) * .35f : .08f;
                const Vector3 root=thorax+right*(side*.22f*unit)
                    +dorsal*(.31f*unit)-forward*(.02f*unit);
                const Vector3 leading=root+right*(side*.24f*unit)
                    +forward*(.11f*unit)+dorsal*(beat*unit);
                const Vector3 outer=root+right*(side*.50f*unit)
                    -forward*(.30f*unit)+dorsal*(beat*.90f*unit);
                const Vector3 tip=root+right*(side*.38f*unit)
                    -forward*(1.46f*unit)+dorsal*(beat*.70f*unit);
                const Vector3 trailing=root+right*(side*.08f*unit)
                    -forward*(1.25f*unit)+dorsal*(.04f*unit);
                AddDoubleSidedTriangle(vertices,root,leading,outer,wing);
                AddDoubleSidedTriangle(vertices,root,outer,tip,wing);
                AddDoubleSidedTriangle(vertices,root,tip,trailing,
                    Color::Lerp(wing,body,.20f));
                AppendGeckoSegment(vertices, root, tip, .025f * unit, vein);
                AppendGeckoSegment(vertices,leading,tip,.018f*unit,vein);
                AppendGeckoSegment(vertices,outer,trailing,.016f*unit,vein);
                AppendGeckoSegment(vertices,root+(leading-root)*.58f,
                    root+(tip-root)*.72f,.013f*unit,vein);

                const Vector3 eyeCentre = head + forward * (.12f * unit)
                    + right * (side * .43f * unit) + dorsal * (.15f * unit);
                constexpr std::array<float,3> EyeStations={{-.09f,0.0f,.09f}};
                constexpr std::array<float,3> EyeWidths={{.045f,.105f,.045f}};
                constexpr std::array<float,3> EyeHeights={{.040f,.100f,.040f}};
                AppendGeckoLoft(vertices,eyeCentre,forward,dorsal,right,EyeStations,
                    EyeWidths,EyeHeights,unit,eye,eye,eye);
                AddDoubleSidedTriangle(vertices,
                    eyeCentre + dorsal * (.115f * unit),
                    eyeCentre + forward * (.055f * unit),
                    eyeCentre + right * (side * .045f * unit), glint);

                const Vector3 antennaRoot=head+forward*(.29f*unit)
                    +right*(side*.18f*unit)+dorsal*(.11f*unit);
                const Vector3 antennaTip=antennaRoot+forward*(.30f*unit)
                    +right*(side*.13f*unit)+dorsal*(.05f*unit);
                AppendGeckoTube(vertices,antennaRoot,antennaTip,.025f*unit,
                    .010f*unit,legColor,5);

                for (int legIndex = 0; legIndex < 3; ++legIndex) {
                    const float along = (.42f - .43f * static_cast<float>(legIndex)) * unit;
                    const Vector3 hip = thorax + forward * along
                        + right * (side * .34f * unit);
                    const Vector3 knee = centre + forward * (along - .12f * unit)
                        + right * (side * (.83f + .08f * legIndex) * unit)
                        + dorsal * (.04f * unit);
                    const Vector3 foot = centre + forward
                        * ((.54f - .58f * static_cast<float>(legIndex)) * unit)
                        + right * (side * (1.02f + .10f * legIndex) * unit)
                        - dorsal * (.02f * unit);
                    AppendGeckoTube(vertices,hip,knee,.072f*unit,.050f*unit,
                                    legColor,6);
                    AppendGeckoTube(vertices,knee,foot,.050f*unit,.024f*unit,
                                    legColor,6);
                }
            }
        }

        void AppendBeetle(std::vector<VertexPositionColor>& vertices,
                          const BeetleState& beetle)
        {
            Vector3 dorsal(beetle.normalX, beetle.normalY, beetle.normalZ);
            dorsal = Vector3::Normalize(dorsal);
            Vector3 forward(std::cos(beetle.headingRadians), 0.0f,
                            std::sin(beetle.headingRadians));
            forward = Vector3::Normalize(forward - dorsal * Vector3::Dot(forward, dorsal));
            const Vector3 right = Vector3::Normalize(Vector3::Cross(dorsal, forward));
            const float unit = .050f * beetle.scale;
            const std::array<Color, 4> shells = {{Color(37, 128, 91), Color(145, 75, 37),
                                                  Color(43, 91, 151), Color(145, 111, 31)}};
            const Color shell = shells[beetle.colorVariant % shells.size()];
            const Color shellLight = Color::Lerp(shell, Color(172, 172, 116), .24f);
            const Color underside = Color::Lerp(shell, Color(10, 11, 10), .58f);
            const Color limb = Color::Lerp(shell, Color(13, 12, 10), .48f);
            const Color ridge = Color::Lerp(shell, Color(18, 20, 17), .43f);
            const Color eye(18, 12, 8), glint(213, 197, 148);
            const Vector3 centre(beetle.x, beetle.y, beetle.z);
            const Vector3 abdomen = centre + dorsal * (.35f * unit) - forward * (.20f * unit);
            constexpr std::array<float, 6> ElytronStations =
                {{-.84f, -.70f, -.30f, .18f, .54f, .69f}};
            constexpr std::array<float, 6> ElytronWidths =
                {{.08f, .24f, .33f, .35f, .28f, .08f}};
            constexpr std::array<float, 6> ElytronHeights =
                {{.07f, .22f, .36f, .40f, .29f, .07f}};
            for (const float side : {-1.0f, 1.0f}) {
                const Vector3 elytron = abdomen + right * (side * .265f * unit);
                AppendBeetleLoft(vertices, elytron, right, dorsal, forward,
                    ElytronStations, ElytronWidths, ElytronHeights, unit,
                    side < 0.0f ? shell : shellLight,
                    Color::Lerp(shell, underside, .28f), underside);

                // Three shallow raised striae per wing case catch the light without
                // relying on a texture or a second material.
                for (int groove = 0; groove < 3; ++groove) {
                    const float lateral = side * (.12f + .13f * groove) * unit;
                    const Vector3 rear = abdomen + right * lateral
                        - forward * (.68f * unit) + dorsal * (.38f * unit);
                    const Vector3 front = abdomen + right * lateral
                        + forward * (.49f * unit) + dorsal * (.39f * unit);
                    AppendGeckoTube(vertices, rear, front, .018f * unit,
                                    .012f * unit, ridge, 5);
                }
            }
            AppendGeckoTube(vertices,
                abdomen + dorsal * (.36f * unit) + forward * (.50f * unit),
                abdomen + dorsal * (.36f * unit) - forward * (.88f * unit),
                .022f * unit, .012f * unit, underside, 5);
            const Vector3 pronotum = centre + forward * (.63f * unit)
                + dorsal * (.285f * unit);
            constexpr std::array<float, 5> PronotumStations =
                {{-.36f, -.22f, .02f, .24f, .37f}};
            constexpr std::array<float, 5> PronotumWidths =
                {{.24f, .46f, .52f, .44f, .20f}};
            constexpr std::array<float, 5> PronotumHeights =
                {{.13f, .27f, .31f, .25f, .11f}};
            AppendBeetleLoft(vertices, pronotum, right, dorsal, forward,
                PronotumStations, PronotumWidths, PronotumHeights, unit,
                shellLight, shell, underside);
            const Vector3 head = pronotum + forward * (.52f * unit) - dorsal * (.03f * unit);
            constexpr std::array<float, 5> HeadStations =
                {{-.30f, -.17f, .02f, .21f, .32f}};
            constexpr std::array<float, 5> HeadWidths =
                {{.18f, .32f, .35f, .29f, .12f}};
            constexpr std::array<float, 5> HeadHeights =
                {{.10f, .21f, .24f, .18f, .07f}};
            AppendBeetleLoft(vertices, head, right, dorsal, forward,
                HeadStations, HeadWidths, HeadHeights, unit, shell, shell, underside);

            const Vector3 scutellum = centre + forward * (.33f * unit)
                + dorsal * (.60f * unit);
            AppendGeckoDiamond(vertices, scutellum, right, dorsal, forward,
                .105f * unit, .025f * unit, .115f * unit,
                ridge, underside);

            const float gait = std::sin(beetle.gaitPhase);
            for (const float side : {-1.0f, 1.0f}) {
                for (int legIndex = 0; legIndex < 3; ++legIndex) {
                    const float along = (.48f - .52f * static_cast<float>(legIndex)) * unit;
                    const float swing = gait * (legIndex % 2 == 0 ? 1.0f : -1.0f) * .13f * unit;
                    const Vector3 hip = centre + forward * along
                        + right * (side * .38f * unit) + dorsal * (.18f * unit);
                    const Vector3 knee = hip + right * (side * .30f * unit)
                        + forward * swing + dorsal * (.015f * unit);
                    const Vector3 ankle = knee + right * (side * .28f * unit)
                        + forward * ((.14f - .14f * legIndex) * unit + swing)
                        - dorsal * (.22f * unit);
                    const Vector3 foot = ankle + right * (side * .22f * unit)
                        + forward * ((.09f - .09f * legIndex) * unit)
                        - dorsal * (.08f * unit);
                    AppendGeckoTube(vertices, hip, knee, .072f * unit,
                                    .056f * unit, limb, 6);
                    AppendGeckoTube(vertices, knee, ankle, .056f * unit,
                                    .035f * unit, limb, 6);
                    AppendGeckoTube(vertices, ankle, foot, .034f * unit,
                                    .012f * unit, underside, 5);
                    AppendGeckoDiamond(vertices, knee, right, dorsal, forward,
                        .065f * unit, .055f * unit, .065f * unit, limb, underside);
                }
                const Vector3 eyeCentre = head + forward * (.20f * unit)
                    + right * (side * .27f * unit) + dorsal * (.10f * unit);
                constexpr std::array<float, 3> EyeStations = {{-.06f, 0.0f, .06f}};
                constexpr std::array<float, 3> EyeWidths = {{.026f, .055f, .026f}};
                constexpr std::array<float, 3> EyeHeights = {{.022f, .050f, .022f}};
                AppendGeckoLoft(vertices, eyeCentre, forward, dorsal, right,
                    EyeStations, EyeWidths, EyeHeights, unit, eye, eye, eye);
                AddDoubleSidedTriangle(vertices, eyeCentre + dorsal * (.054f * unit),
                    eyeCentre + forward * (.015f * unit),
                    eyeCentre + right * (side * .012f * unit), glint);
                const Vector3 antennaRoot = head + forward * (.27f * unit)
                    + right * (side * .15f * unit);
                const Vector3 antennaKnee = antennaRoot + forward * (.42f * unit)
                    + right * (side * .22f * unit) + dorsal * (.10f * unit);
                const Vector3 antennaTip = antennaKnee + forward * (.32f * unit)
                    + right * (side * .24f * unit) - dorsal * (.04f * unit);
                AppendGeckoTube(vertices, antennaRoot, antennaKnee, .025f * unit,
                                .018f * unit, limb, 5);
                AppendGeckoTube(vertices, antennaKnee, antennaTip, .018f * unit,
                                .010f * unit, limb, 5);
                AppendGeckoDiamond(vertices, antennaTip, right, dorsal, forward,
                    .030f * unit, .026f * unit, .060f * unit, limb, underside);

                const Vector3 mandibleRoot = head + forward * (.30f * unit)
                    + right * (side * .10f * unit) - dorsal * (.06f * unit);
                const Vector3 mandibleTip = mandibleRoot + forward * (.22f * unit)
                    - right * (side * .06f * unit) - dorsal * (.03f * unit);
                AppendGeckoTube(vertices, mandibleRoot, mandibleTip,
                                .040f * unit, .012f * unit, underside, 5);
            }
        }

        void AppendRodentEar(std::vector<VertexPositionColor>& vertices,
                             const Vector3& centre, const Vector3& horizontal,
                             const Vector3& vertical, const float radius,
                             const Color& outer, const Color& inner)
        {
            constexpr int Segments = 12;
            const Vector3 recessed = centre - Vector3::Normalize(
                Vector3::Cross(horizontal, vertical)) * (radius * .045f);
            for (int segment = 0; segment < Segments; ++segment) {
                const float a = 6.28318530718f * static_cast<float>(segment) / Segments;
                const float b = 6.28318530718f * static_cast<float>(segment + 1) / Segments;
                const Vector3 edgeA = centre + horizontal * (std::cos(a) * radius)
                    + vertical * (std::sin(a) * radius * 1.08f);
                const Vector3 edgeB = centre + horizontal * (std::cos(b) * radius)
                    + vertical * (std::sin(b) * radius * 1.08f);
                const Vector3 innerA = recessed + horizontal * (std::cos(a) * radius * .67f)
                    + vertical * (std::sin(a) * radius * .72f);
                const Vector3 innerB = recessed + horizontal * (std::cos(b) * radius * .67f)
                    + vertical * (std::sin(b) * radius * .72f);
                AddDoubleSidedTriangle(vertices, edgeA, edgeB, innerB, outer);
                AddDoubleSidedTriangle(vertices, edgeA, innerB, innerA, outer);
                AddDoubleSidedTriangle(vertices, recessed, innerA, innerB, inner);
            }
        }

        void AppendRodentSegment(std::vector<VertexPositionColor>& vertices,
                                 const Vector3& a, const Vector3& b,
                                 const float radiusA, const float radiusB,
                                 const Color& color)
        {
            constexpr int Sides = 7;
            const Vector3 axis = Vector3::Normalize(b - a);
            const Vector3 reference = std::abs(Vector3::Dot(axis, Vector3::Up)) < .88f
                ? Vector3::Up : Vector3(1.0f, 0.0f, 0.0f);
            const Vector3 side = Vector3::Normalize(Vector3::Cross(axis, reference));
            const Vector3 binormal = Vector3::Normalize(Vector3::Cross(side, axis));
            for (int index = 0; index < Sides; ++index) {
                const float angleA = 6.28318530718f * static_cast<float>(index) / Sides;
                const float angleB = 6.28318530718f * static_cast<float>(index + 1) / Sides;
                const Vector3 radialA = side * std::cos(angleA) + binormal * std::sin(angleA);
                const Vector3 radialB = side * std::cos(angleB) + binormal * std::sin(angleB);
                const Vector3 a0 = a + radialA * radiusA;
                const Vector3 a1 = a + radialB * radiusA;
                const Vector3 b0 = b + radialA * radiusB;
                const Vector3 b1 = b + radialB * radiusB;
                AddTriangle(vertices, a0, b0, b1, color);
                AddTriangle(vertices, a0, b1, a1, color);
                AddTriangle(vertices, a, a1, a0, color);
                AddTriangle(vertices, b, b0, b1, color);
            }
        }

        template<std::size_t StationCount>
        void AppendRodentLoft(std::vector<VertexPositionColor>& vertices,
                              const Vector3& centre, const Vector3& right,
                              const Vector3& up, const Vector3& forward,
                              const std::array<float, StationCount>& stations,
                              const std::array<float, StationCount>& halfWidths,
                              const std::array<float, StationCount>& halfHeights,
                              const float scale, const Color& dorsal,
                              const Color& flank, const Color& underside,
                              const std::size_t colorVariant)
        {
            constexpr int Sides = 12;
            const auto ringPoint = [&](const std::size_t station, const int side) {
                const float angle = 6.28318530718f * static_cast<float>(side) / Sides;
                return centre + forward * (stations[station] * scale)
                    + right * (std::cos(angle) * halfWidths[station] * scale)
                    + up * (std::sin(angle) * halfHeights[station] * scale);
            };
            for (std::size_t station = 0; station + 1U < StationCount; ++station) {
                for (int side = 0; side < Sides; ++side) {
                    const int next = (side + 1) % Sides;
                    const float angle = 6.28318530718f
                        * (static_cast<float>(side) + .5f) / Sides;
                    const float vertical = std::sin(angle);
                    const float lateralLight = std::max(0.0f, std::cos(angle)) * .08f;
                    const float furVariation = .018f * std::sin(
                        static_cast<float>(station) * 2.17f
                        + static_cast<float>(colorVariant) * 1.31f);
                    Color color = vertical > .24f
                        ? Color::Lerp(dorsal, Color(211, 194, 157),
                                      .10f + lateralLight + furVariation)
                        : (vertical < -.24f ? underside
                            : Color::Lerp(flank, Color(194, 174, 139),
                                          lateralLight + std::max(0.0f, furVariation)));
                    const Vector3 a = ringPoint(station, side);
                    const Vector3 b = ringPoint(station + 1U, side);
                    const Vector3 c = ringPoint(station + 1U, next);
                    const Vector3 d = ringPoint(station, next);
                    AddTriangle(vertices, a, b, c, color);
                    AddTriangle(vertices, a, c, d, color);
                }
            }
            const Vector3 rear = centre + forward * (stations.front() * scale);
            const Vector3 front = centre + forward * (stations.back() * scale);
            for (int side = 0; side < Sides; ++side) {
                const int next = (side + 1) % Sides;
                AddTriangle(vertices, rear, ringPoint(0U, next), ringPoint(0U, side), flank);
                AddTriangle(vertices, front, ringPoint(StationCount - 1U, side),
                            ringPoint(StationCount - 1U, next), dorsal);
            }
        }

        void AppendRodent(std::vector<VertexPositionColor>& vertices,
                          const RodentState& rodent)
        {
            const std::size_t startingVertexCount = vertices.size();
            const bool rat = rodent.species == RodentSpecies::Rat;
            const std::array<Color, 4> mouseCoats = {{Color(139, 119, 92), Color(116, 105, 91),
                Color(154, 132, 100), Color(96, 91, 82)}};
            const std::array<Color, 4> ratCoats = {{Color(100, 89, 76), Color(83, 79, 72),
                Color(119, 101, 79), Color(72, 69, 65)}};
            const Color coat = rat ? ratCoats[rodent.colorVariant % ratCoats.size()]
                                   : mouseCoats[rodent.colorVariant % mouseCoats.size()];
            const Color flank = Color::Lerp(coat, Color(54, 48, 42), .28f);
            const Color underside = Color::Lerp(coat, Color(210, 194, 165), rat ? .48f : .62f);
            const Color ear = Color::Lerp(coat, Color(184, 116, 112), .46f);
            const Color tail = Color::Lerp(coat, Color(174, 126, 119), .55f);
            const Color eye(11, 10, 8), glint(229, 220, 188), whisker(177, 168, 148);
            const float visibility = std::clamp(rodent.visibility, 0.0f, 1.0f);
            const float unit = (rat ? 1.0f : .67f) * visibility;
            if (unit <= .04f) return;
            const Vector3 forward(std::cos(rodent.headingRadians), 0,
                                  std::sin(rodent.headingRadians));
            const Vector3 right(-forward.Z, 0, forward.X), up = Vector3::Up;
            const float bob = std::abs(std::sin(rodent.gaitPhase)) * .012f * unit;
            const Vector3 centre(rodent.x, rodent.y + (rat ? .095f : .072f) * unit + bob,
                                 rodent.z);
            constexpr std::array<float, 8> MouseBodyStations =
                {{-.22f, -.18f, -.10f, .015f, .12f, .20f, .25f, .27f}};
            constexpr std::array<float, 8> MouseBodyWidths =
                {{.026f, .102f, .145f, .155f, .145f, .112f, .062f, .022f}};
            constexpr std::array<float, 8> MouseBodyHeights =
                {{.024f, .078f, .112f, .120f, .108f, .078f, .043f, .020f}};
            constexpr std::array<float, 8> RatBodyStations =
                {{-.27f, -.225f, -.13f, -.005f, .115f, .205f, .255f, .275f}};
            constexpr std::array<float, 8> RatBodyWidths =
                {{.030f, .098f, .137f, .148f, .139f, .108f, .058f, .021f}};
            constexpr std::array<float, 8> RatBodyHeights =
                {{.026f, .074f, .103f, .112f, .101f, .073f, .040f, .019f}};
            AppendRodentLoft(vertices, centre, right, up, forward,
                rat ? RatBodyStations : MouseBodyStations,
                rat ? RatBodyWidths : MouseBodyWidths,
                rat ? RatBodyHeights : MouseBodyHeights,
                unit, coat, flank, underside, rodent.colorVariant);
            const float sniff = std::sin(rodent.sniffPhase) * .008f * unit;
            const Vector3 head = centre + forward * (.235f * unit)
                + up * ((rat ? .018f : .028f) * unit + sniff);
            constexpr std::array<float, 7> MouseHeadStations =
                {{-.12f, -.075f, -.01f, .065f, .13f, .18f, .215f}};
            constexpr std::array<float, 7> MouseHeadWidths =
                {{.040f, .090f, .112f, .106f, .080f, .046f, .014f}};
            constexpr std::array<float, 7> MouseHeadHeights =
                {{.037f, .076f, .095f, .087f, .064f, .036f, .013f}};
            constexpr std::array<float, 7> RatHeadStations =
                {{-.13f, -.082f, -.01f, .080f, .165f, .230f, .272f}};
            constexpr std::array<float, 7> RatHeadWidths =
                {{.035f, .080f, .105f, .096f, .064f, .034f, .013f}};
            constexpr std::array<float, 7> RatHeadHeights =
                {{.033f, .069f, .087f, .073f, .052f, .029f, .012f}};
            AppendRodentLoft(vertices, head, right, up, forward,
                            rat ? RatHeadStations : MouseHeadStations,
                            rat ? RatHeadWidths : MouseHeadWidths,
                            rat ? RatHeadHeights : MouseHeadHeights,
                            unit, coat, flank, underside, rodent.colorVariant + 3U);

            for (const float side : {-1.0f, 1.0f}) {
                const Vector3 earCentre = head - forward * (.035f * unit)
                    + right * (side * .085f * unit) + up * (.080f * unit);
                const float earSize = (rat ? .034f : .052f) * unit;
                AppendRodentEar(vertices, earCentre, forward, up, earSize, flank, ear);
                const Vector3 eyeCentre = head + forward * (.070f * unit)
                    + right * (side * .083f * unit) + up * (.026f * unit);
                const float eyeSize = (rat ? .016f : .022f) * unit;
                constexpr std::array<float, 4> EyeStations = {{-.55f, -.18f, .30f, .62f}};
                constexpr std::array<float, 4> EyeWidths = {{.32f, .52f, .46f, .18f}};
                constexpr std::array<float, 4> EyeHeights = {{.32f, .52f, .46f, .18f}};
                AppendGeckoLoft(vertices, eyeCentre, forward, up, right * side,
                    EyeStations, EyeWidths, EyeHeights, eyeSize * 1.32f,
                    eye, eye, eye);
                const Vector3 highlight = eyeCentre + right * (side * .016f * unit)
                    + up * (.009f * unit) + forward * (.004f * unit);
                AddDoubleSidedTriangle(vertices, highlight + up * (.004f * unit),
                    highlight - forward * (.004f * unit) - up * (.003f * unit),
                    highlight + forward * (.004f * unit) - up * (.003f * unit), glint);
                const Vector3 whiskerRoot = head + forward * (.17f * unit)
                    + right * (side * .035f * unit) - up * (.015f * unit);
                for (int whiskerIndex = -1; whiskerIndex <= 1; ++whiskerIndex) {
                    const Vector3 tip = whiskerRoot + forward * (.045f * unit)
                        + right * (side * (.16f + .018f * whiskerIndex) * unit)
                        + up * (.023f * whiskerIndex * unit);
                    AppendGeckoSegment(vertices, whiskerRoot, tip, .0022f * unit, whisker);
                }
            }
            const Vector3 nose = head + forward * ((rat ? .242f : .202f) * unit)
                - up * (.006f * unit);
            constexpr std::array<float, 4> NoseStations = {{-.55f, -.18f, .32f, .62f}};
            constexpr std::array<float, 4> NoseWidths = {{.35f, .58f, .50f, .20f}};
            constexpr std::array<float, 4> NoseHeights = {{.31f, .50f, .43f, .18f}};
            AppendGeckoLoft(vertices, nose, right, up, forward,
                NoseStations, NoseWidths, NoseHeights, .035f * unit, eye, eye, eye);
            const float stride = std::sin(rodent.gaitPhase) * .045f * unit;
            for (int leg = 0; leg < 4; ++leg) {
                const float side = leg % 2 == 0 ? -1.0f : 1.0f;
                const float along = leg < 2 ? .12f : -.10f;
                const float phaseStride = (leg == 0 || leg == 3) ? stride : -stride;
                const Vector3 hip = centre + forward * (along * unit)
                    + right * (side * .09f * unit) - up * (.045f * unit);
                const Vector3 ankle = hip + forward * phaseStride
                    + right * (side * .055f * unit) - up * (.055f * unit);
                const Vector3 paw = ankle + forward * (.055f * unit - phaseStride * .4f)
                    + right * (side * .025f * unit);
                AppendRodentSegment(vertices, hip, ankle, .019f * unit,
                                    .014f * unit, flank);
                AppendRodentSegment(vertices, ankle, paw, .014f * unit,
                                    .010f * unit, underside);
                AppendAntBody(vertices, paw + forward * (.014f * unit), right, up, forward,
                    .017f * unit, .009f * unit, .030f * unit, underside, underside);
            }
            // Three gently curved segments keep the tail volumetric enough to avoid a wire.
            Vector3 tailRoot = centre - forward * (.19f * unit) - up * (.015f * unit);
            const float tailLength = (rat ? .36f : .31f) * unit;
            for (int segment = 0; segment < 3; ++segment) {
                const float a = static_cast<float>(segment) / 3.0f;
                const float b = static_cast<float>(segment + 1) / 3.0f;
                const Vector3 start = tailRoot - forward * (tailLength * a)
                    + right * (std::sin(a * 2.7f + rodent.gaitPhase * .18f) * .045f * unit)
                    + up * (a * .025f * unit);
                const Vector3 end = tailRoot - forward * (tailLength * b)
                    + right * (std::sin(b * 2.7f + rodent.gaitPhase * .18f) * .045f * unit)
                    + up * (b * .025f * unit);
                AppendRodentSegment(vertices, start, end,
                    (.013f - .0025f * segment) * unit,
                    (.011f - .0025f * segment) * unit, tail);
            }
            if (vertices.size() - startingVertexCount != RodentVertexCount) {
                throw std::logic_error("Rodent geometry violated its exact vertex budget");
            }
        }

        [[nodiscard]] Vector3 LocalPoint(const Vector3& centre, const Vector3& right,
                                         const Vector3& up, const Vector3& forward,
                                         const float x, const float y, const float z,
                                         const float scale)
        {
            return centre + right * (x * scale) + up * (y * scale)
                + forward * (z * scale);
        }

        void AppendNearButterfly(std::vector<VertexPositionColor>& vertices,
                                 const ButterflyState& butterfly)
        {
            const std::size_t startingVertexCount = vertices.size();
            const ButterflyPalette& palette =
                Palettes[butterfly.colorVariant % Palettes.size()];
            const Vector3 centre(butterfly.x, butterfly.y, butterfly.z);
            const Vector3 forward(std::cos(butterfly.headingRadians), 0.0f,
                                  std::sin(butterfly.headingRadians));
            const Vector3 right(-forward.Z, 0.0f, forward.X);
            const Vector3 up = Vector3::Up;
            const float scale = butterfly.scale;
            const float foldAngle = butterfly.wingFold * 1.22f;
            const float horizontal = std::cos(foldAngle);
            const float lift = std::sin(foldAngle);

            constexpr int BodySides=6;
            const Color abdomen = Color::Lerp(palette.body, palette.accent, .42f);
            const Color abdomenTip = Color::Lerp(palette.body, palette.accent, .18f);
            const Vector3 bodyFront=LocalPoint(
                centre,right,up,forward,0,.012f,.22f,scale);
            const Vector3 bodyRear=LocalPoint(
                centre,right,up,forward,0,-.010f,-.382f,scale);
            const auto bodyRing=[&](float angle,float z,float radius) {
                return LocalPoint(centre,right,up,forward,std::cos(angle)*radius,
                    std::sin(angle)*radius,z,scale);
            };
            for(int sideIndex=0;sideIndex<BodySides;++sideIndex) {
                const float a0=6.28318530718f*static_cast<float>(sideIndex)/BodySides;
                const float a1=6.28318530718f*static_cast<float>(sideIndex+1)/BodySides;
                const Vector3 front0=bodyRing(a0,.12f,.064f);
                const Vector3 front1=bodyRing(a1,.12f,.064f);
                const Vector3 middle0=bodyRing(a0,-.05f,.060f);
                const Vector3 middle1=bodyRing(a1,-.05f,.060f);
                const Vector3 rear0=bodyRing(a0,-.22f,.043f);
                const Vector3 rear1=bodyRing(a1,-.22f,.043f);
                const Vector3 tail0=bodyRing(a0,-.36f,.018f);
                const Vector3 tail1=bodyRing(a1,-.36f,.018f);
                AddTriangle(vertices,bodyFront,front0,front1,palette.body);
                AddTriangle(vertices,front0,middle0,middle1,palette.body);
                AddTriangle(vertices,front0,middle1,front1,palette.body);
                AddTriangle(vertices,middle0,rear0,rear1,abdomen);
                AddTriangle(vertices,middle0,rear1,middle1,abdomen);
                AddTriangle(vertices,rear0,tail0,tail1,abdomenTip);
                AddTriangle(vertices,rear0,tail1,rear1,abdomenTip);
                AddTriangle(vertices,bodyRear,tail1,tail0,palette.body);
            }
            const Vector3 headCentre=LocalPoint(
                centre,right,up,forward,0,.012f,.255f,scale);
            const Vector3 headTop=headCentre+up*(.065f*scale);
            const Vector3 headBottom=headCentre-up*(.065f*scale);
            const Vector3 headFront=headCentre+forward*(.070f*scale);
            const Vector3 headBack=headCentre-forward*(.060f*scale);
            const Vector3 headLeft=headCentre-right*(.062f*scale);
            const Vector3 headRight=headCentre+right*(.062f*scale);
            AddTriangle(vertices,headTop,headFront,headLeft,palette.body);
            AddTriangle(vertices,headTop,headRight,headFront,palette.body);
            AddTriangle(vertices,headTop,headBack,headRight,palette.body);
            AddTriangle(vertices,headTop,headLeft,headBack,palette.body);
            AddTriangle(vertices,headBottom,headLeft,headFront,palette.body);
            AddTriangle(vertices,headBottom,headFront,headRight,palette.body);
            AddTriangle(vertices,headBottom,headRight,headBack,palette.body);
            AddTriangle(vertices,headBottom,headBack,headLeft,palette.body);
            for(const float side:{-1.0f,1.0f}) {
                const Vector3 antennaRoot=LocalPoint(
                    centre,right,up,forward,side*.025f,.045f,.30f,scale);
                const Vector3 antennaTip=LocalPoint(
                    centre,right,up,forward,side*.17f,.085f,.52f,scale);
                const Vector3 antennaEdge=LocalPoint(
                    centre,right,up,forward,side*.155f,.073f,.505f,scale);
                AddDoubleSidedTriangle(vertices,antennaRoot,antennaTip,antennaEdge,
                    palette.body);
            }

            // Butterflies carry three pairs of slender legs under the thorax.  A broad,
            // swept footprint remains readable from the close overhead audit without
            // turning the appendages into expensive solid tubes.
            constexpr std::array<float,3> LegRootsZ={.105f,-.015f,-.135f};
            constexpr std::array<float,3> LegTipsZ={.19f,-.015f,-.23f};
            constexpr std::array<float,3> LegSpans={.19f,.23f,.18f};
            for(const float side:{-1.0f,1.0f}) {
                for(std::size_t pair=0;pair<LegRootsZ.size();++pair) {
                    const Vector3 root=LocalPoint(centre,right,up,forward,
                        side*.028f,-.035f,LegRootsZ[pair],scale);
                    const Vector3 foot=LocalPoint(centre,right,up,forward,
                        side*LegSpans[pair],-.073f,LegTipsZ[pair],scale);
                    AppendGeckoSegment(vertices,root,foot,.0075f*scale,palette.body);
                }
            }

            for (int sideIndex = -1; sideIndex <= 1; sideIndex += 2) {
                const float side = static_cast<float>(sideIndex);
                const auto wingPoint=[&](float span,float z,float extraLift=0.0f) {
                    return LocalPoint(centre,right,up,forward,side*span*horizontal,
                        span*lift+extraLift,z,scale);
                };
                const Vector3 foreRoot = LocalPoint(
                    centre, right, up, forward, side * 0.035f, 0.0f, 0.12f, scale);
                const Vector3 hindRoot = LocalPoint(
                    centre, right, up, forward, side * 0.035f, 0.0f, -0.13f, scale);
                const Vector3 leadingTip=wingPoint(.40f,.31f);
                const Vector3 foreShoulder=wingPoint(.53f,.265f);
                const Vector3 foreTip=wingPoint(.65f,.16f);
                const Vector3 outerTip=wingPoint(.68f,-.05f);
                const Vector3 hindTip=wingPoint(.52f,-.33f);
                const Vector3 rearTip=wingPoint(.28f,-.42f);
                AddDoubleSidedTriangle(
                    vertices,foreRoot,leadingTip,foreShoulder,palette.forewing);
                AddDoubleSidedTriangle(
                    vertices,foreRoot,foreShoulder,foreTip,palette.forewing);
                AddDoubleSidedTriangle(
                    vertices,foreRoot,foreTip,outerTip,palette.forewing);
                AddDoubleSidedTriangle(
                    vertices,foreRoot,outerTip,hindRoot,palette.forewing);
                AddDoubleSidedTriangle(
                    vertices,hindRoot,outerTip,hindTip,palette.hindwing);
                AddDoubleSidedTriangle(
                    vertices,hindRoot,hindTip,rearTip,palette.hindwing);

                const Vector3 foreInner=wingPoint(.51f,.13f,.004f);
                const Vector3 outerInner=wingPoint(.55f,-.055f,.004f);
                const Vector3 hindInner=wingPoint(.41f,-.275f,.004f);
                AddDoubleSidedTriangle(
                    vertices,foreTip,outerTip,outerInner,palette.body);
                AddDoubleSidedTriangle(
                    vertices,foreTip,outerInner,foreInner,palette.body);
                AddDoubleSidedTriangle(
                    vertices,outerTip,hindTip,hindInner,palette.body);
                AddDoubleSidedTriangle(
                    vertices,outerTip,hindInner,outerInner,palette.body);
                const Vector3 accentFront=wingPoint(.29f,.065f,.008f);
                const Vector3 accentOuter=wingPoint(.43f,.015f,.008f);
                const Vector3 accentRear=wingPoint(.34f,-.105f,.008f);
                AddDoubleSidedTriangle(vertices,accentFront,accentOuter,accentRear,
                    palette.accent);
            }
            if (vertices.size() - startingVertexCount != MaximumVerticesPerButterfly) {
                throw std::runtime_error(
                    "Butterfly near mesh does not match its fixed vertex budget");
            }
        }

        void AppendFarButterfly(std::vector<VertexPositionColor>& vertices,
                                const ButterflyState& butterfly)
        {
            const ButterflyPalette& palette =
                Palettes[butterfly.colorVariant % Palettes.size()];
            const Vector3 centre(butterfly.x, butterfly.y, butterfly.z);
            const Vector3 forward(std::cos(butterfly.headingRadians), 0.0f,
                                  std::sin(butterfly.headingRadians));
            const Vector3 right(-forward.Z, 0.0f, forward.X);
            const Vector3 up = Vector3::Up;
            const float scale = butterfly.scale;
            const float foldAngle = butterfly.wingFold * 1.12f;
            const float horizontal = std::cos(foldAngle);
            const float lift = std::sin(foldAngle);
            const Vector3 front = LocalPoint(
                centre, right, up, forward, 0.0f, 0.0f, 0.22f, scale);
            const Vector3 back = LocalPoint(
                centre, right, up, forward, 0.0f, 0.0f, -0.25f, scale);
            const Vector3 leftTip = LocalPoint(
                centre, right, up, forward, -0.57f * horizontal,
                0.57f * lift, -0.02f, scale);
            const Vector3 rightTip = LocalPoint(
                centre, right, up, forward, 0.57f * horizontal,
                0.57f * lift, -0.02f, scale);
            AddDoubleSidedTriangle(vertices, front, leftTip, back, palette.forewing);
            AddDoubleSidedTriangle(vertices, front, back, rightTip, palette.hindwing);
        }
    }

    ButterflyRenderer::ButterflyRenderer(GraphicsDevice& device, const float maximumDistance)
        : device_(device),
          effect_(std::make_unique<BasicEffect>(device)),
          vertices_(std::make_unique<DynamicVertexBuffer>(
              device, VertexPositionColor::getVertexDeclarationStatic(), MaximumVertexCount,
              BufferUsage::WriteOnly)),
          maximumDistance_(maximumDistance)
    {
        if (!std::isfinite(maximumDistance_) || maximumDistance_ <= 0.0f) {
            throw std::invalid_argument(
                "Butterfly maximum draw distance must be finite and positive");
        }
        effect_->setVertexColorEnabledProperty(true);
        effect_->setLightingEnabledProperty(false);
        effect_->setFogEnabledProperty(true);
        effect_->setFogColorProperty(Vector3(environment::FogColorRed, environment::FogColorGreen, environment::FogColorBlue));
        effect_->setFogStartProperty(42.0f);
        effect_->setFogEndProperty(82.0f);
        stagingVertices_.reserve(static_cast<std::size_t>(MaximumVertexCount));
        std::cout << ButterflyDiagnostic << '\n';
        std::cout << ButterflyAnatomyDiagnostic << '\n';
        std::cout << ButterflyBodyDiagnostic << '\n';
        std::cout << ButterflyHabitatDiagnostic << '\n';
        std::cout << GeckoDiagnostic << '\n';
        std::cout << GeckoAnatomyDiagnostic << '\n';
        std::cout << SkinkDiagnostic << '\n';
        std::cout << SkinkAnatomyDiagnostic << '\n';
        std::cout << SkinkSolidDiagnostic << '\n';
        std::cout << DragonflyDiagnostic << '\n';
        std::cout << DragonflyAnatomyDiagnostic << '\n';
        std::cout << FrogDiagnostic << '\n';
        std::cout << FrogAnatomyDiagnostic << '\n';
        std::cout << RodentDiagnostic << '\n';
        std::cout << RodentAnatomyDiagnostic << '\n';
        std::cout << RodentSurfaceDiagnostic << '\n';
        std::cout << FlyDiagnostic << '\n';
        std::cout << FlyAnatomyDiagnostic << '\n';
        std::cout << AntDiagnostic << '\n';
        std::cout << AntAnatomyDiagnostic << '\n';
        std::cout << CicadaDiagnostic << '\n';
        std::cout << CicadaAnatomyDiagnostic << '\n';
        std::cout << BeetleDiagnostic << '\n';
        std::cout << BeetleAnatomyDiagnostic << '\n';
        MirrorRuntimeDiagnosticToPlatformLog(ButterflyDiagnostic);
        MirrorRuntimeDiagnosticToPlatformLog(ButterflyAnatomyDiagnostic);
        MirrorRuntimeDiagnosticToPlatformLog(ButterflyBodyDiagnostic);
        MirrorRuntimeDiagnosticToPlatformLog(ButterflyHabitatDiagnostic);
        MirrorRuntimeDiagnosticToPlatformLog(GeckoDiagnostic);
        MirrorRuntimeDiagnosticToPlatformLog(GeckoAnatomyDiagnostic);
        MirrorRuntimeDiagnosticToPlatformLog(SkinkDiagnostic);
        MirrorRuntimeDiagnosticToPlatformLog(SkinkAnatomyDiagnostic);
        MirrorRuntimeDiagnosticToPlatformLog(SkinkSolidDiagnostic);
        MirrorRuntimeDiagnosticToPlatformLog(DragonflyDiagnostic);
        MirrorRuntimeDiagnosticToPlatformLog(DragonflyAnatomyDiagnostic);
        MirrorRuntimeDiagnosticToPlatformLog(FrogDiagnostic);
        MirrorRuntimeDiagnosticToPlatformLog(FrogAnatomyDiagnostic);
        MirrorRuntimeDiagnosticToPlatformLog(RodentDiagnostic);
        MirrorRuntimeDiagnosticToPlatformLog(RodentAnatomyDiagnostic);
        MirrorRuntimeDiagnosticToPlatformLog(RodentSurfaceDiagnostic);
        MirrorRuntimeDiagnosticToPlatformLog(FlyDiagnostic);
        MirrorRuntimeDiagnosticToPlatformLog(FlyAnatomyDiagnostic);
        MirrorRuntimeDiagnosticToPlatformLog(AntDiagnostic);
        MirrorRuntimeDiagnosticToPlatformLog(AntAnatomyDiagnostic);
        MirrorRuntimeDiagnosticToPlatformLog(CicadaDiagnostic);
        MirrorRuntimeDiagnosticToPlatformLog(CicadaAnatomyDiagnostic);
        MirrorRuntimeDiagnosticToPlatformLog(BeetleDiagnostic);
        MirrorRuntimeDiagnosticToPlatformLog(BeetleAnatomyDiagnostic);
    }

    ButterflyRenderer::~ButterflyRenderer() = default;

    void ButterflyRenderer::Draw(const Matrix& view, const Matrix& projection,
                                  const Vector3& cameraPosition,
                                  const std::span<const ButterflyState> butterflies,
                                  const std::span<const ButterflyFlowerPatch> flowerPatches,
                                  const std::span<const GeckoState> geckos,
                                  const std::span<const SkinkState> skinks,
                                  const std::span<const DragonflyState> dragonflies,
                                  const std::span<const FrogState> frogs,
                                  const std::span<const RodentState> rodents,
                                  const std::span<const FlyState> flies,
                                  const std::span<const AntState> ants,
                                  const std::span<const CicadaState> cicadas,
                                  const std::span<const BeetleState> beetles)
    {
        stagingVertices_.clear();
        visibleButterflyCount_ = 0;
        visibleNearButterflyCount_ = 0;
        visibleFarButterflyCount_ = 0;
        visibleGeckoCount_ = 0;
        visibleSkinkCount_ = 0;
        visibleDragonflyCount_=0;
        visibleFrogCount_ = 0;
        visibleMouseCount_ = 0;
        visibleRatCount_ = 0;
        visibleFlyCount_ = 0;
        visibleAntCount_ = 0;
        visibleCicadaCount_ = 0;
        visibleBeetleCount_ = 0;
        drawCallCount_ = 0;
        const BoundingFrustum frustum(view * projection);
        for (std::size_t index = 0; index < flowerPatches.size(); ++index) {
            const ButterflyFlowerPatch& patch = flowerPatches[index];
            const Vector3 centre(patch.x, patch.y + .24f, patch.z);
            if (Vector3::DistanceSquared(centre, cameraPosition)
                    <= maximumDistance_ * maximumDistance_
                && frustum.Intersects(BoundingSphere(centre, patch.radius + .5f))) {
                AppendFlowerPatch(stagingVertices_, patch, index);
            }
        }
        for (const GeckoState& gecko : geckos) {
            if (gecko.visibility <= .035f) continue;
            const Vector3 centre(gecko.x, gecko.y + .08f, gecko.z);
            if (Vector3::DistanceSquared(centre, cameraPosition)
                    > maximumDistance_ * maximumDistance_
                || !frustum.Intersects(BoundingSphere(centre, gecko.scale * .55f))) {
                continue;
            }
            AppendGecko(stagingVertices_, gecko);
            ++visibleGeckoCount_;
        }
        for (const SkinkState& skink : skinks) {
            const Vector3 centre(skink.x, skink.y + .07f, skink.z);
            if (Vector3::DistanceSquared(centre, cameraPosition)
                    > maximumDistance_ * maximumDistance_
                || !frustum.Intersects(BoundingSphere(centre, skink.scale * .65f))) {
                continue;
            }
            AppendSkink(stagingVertices_, skink);
            ++visibleSkinkCount_;
        }
        for(const DragonflyState& fly:dragonflies){
            const Vector3 centre(fly.x,fly.y,fly.z);
            if(Vector3::DistanceSquared(centre,cameraPosition)>maximumDistance_*maximumDistance_
                ||!frustum.Intersects(BoundingSphere(centre,.46f*fly.scale)))continue;
            AppendDragonfly(stagingVertices_,fly);++visibleDragonflyCount_;
        }
        for (const FrogState& frog : frogs) {
            const Vector3 centre(frog.x, frog.y + .08f, frog.z);
            if (Vector3::DistanceSquared(centre, cameraPosition)
                    > maximumDistance_ * maximumDistance_
                || !frustum.Intersects(BoundingSphere(centre, frog.scale * .75f))) {
                continue;
            }
            AppendFrog(stagingVertices_, frog);
            ++visibleFrogCount_;
        }
        for (const RodentState& rodent : rodents) {
            if (rodent.visibility <= .035f) continue;
            const Vector3 centre(rodent.x, rodent.y + .08f, rodent.z);
            if (Vector3::DistanceSquared(centre, cameraPosition)
                    > maximumDistance_ * maximumDistance_
                || !frustum.Intersects(BoundingSphere(centre,
                    rodent.species == RodentSpecies::Rat ? .55f : .38f))) continue;
            AppendRodent(stagingVertices_, rodent);
            if (rodent.species == RodentSpecies::Mouse) ++visibleMouseCount_;
            else ++visibleRatCount_;
        }
        for (const FlyState& fly : flies) {
            const Vector3 centre(fly.x, fly.y, fly.z);
            if (Vector3::DistanceSquared(centre, cameraPosition) > 34.0f * 34.0f
                || !frustum.Intersects(BoundingSphere(centre, .08f))) continue;
            AppendFly(stagingVertices_, fly);
            ++visibleFlyCount_;
        }
        for(const AntState& ant:ants) {
            const Vector3 centre(ant.x,ant.y,ant.z);
            if(Vector3::DistanceSquared(centre,cameraPosition)>12.0f*12.0f
                ||!frustum.Intersects(BoundingSphere(centre,.045f)))continue;
            const std::size_t firstVertex = stagingVertices_.size();
            AppendAnt(stagingVertices_,ant);
            const std::size_t expectedVertexCount = ant.behavior == AntBehavior::Carry
                ? CarryingAntVertexCount : AntVertexCount;
            if (stagingVertices_.size() - firstVertex != expectedVertexCount) {
                throw std::logic_error("Ant geometry violated its exact vertex budget");
            }
            ++visibleAntCount_;
        }
        for (const CicadaState& cicada : cicadas) {
            const Vector3 centre(cicada.x, cicada.y, cicada.z);
            if (Vector3::DistanceSquared(centre, cameraPosition) > 26.0f * 26.0f
                || !frustum.Intersects(BoundingSphere(centre, .12f))) continue;
            AppendCicada(stagingVertices_, cicada);
            ++visibleCicadaCount_;
        }
        for (const BeetleState& beetle : beetles) {
            if (beetle.visibility <= .035f) continue;
            const Vector3 centre(beetle.x, beetle.y + .04f, beetle.z);
            if (Vector3::DistanceSquared(centre, cameraPosition) > 24.0f * 24.0f
                || !frustum.Intersects(BoundingSphere(centre, .16f))) continue;
            const std::size_t firstVertex = stagingVertices_.size();
            AppendBeetle(stagingVertices_, beetle);
            if (stagingVertices_.size() - firstVertex != BeetleVertexCount) {
                throw std::logic_error("Beetle geometry violated its exact vertex budget");
            }
            ++visibleBeetleCount_;
        }
        for (const ButterflyState& butterfly : butterflies) {
            const Vector3 centre(butterfly.x, butterfly.y, butterfly.z);
            const float distanceSquared = Vector3::DistanceSquared(centre, cameraPosition);
            if (distanceSquared > maximumDistance_ * maximumDistance_
                || !frustum.Intersects(BoundingSphere(centre, butterfly.scale * 0.82f))) {
                continue;
            }
            if (distanceSquared <= NearLodDistance * NearLodDistance) {
                AppendNearButterfly(stagingVertices_, butterfly);
                ++visibleNearButterflyCount_;
            } else {
                AppendFarButterfly(stagingVertices_, butterfly);
                ++visibleFarButterflyCount_;
            }
            ++visibleButterflyCount_;
        }
        if (stagingVertices_.empty()) return;

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
