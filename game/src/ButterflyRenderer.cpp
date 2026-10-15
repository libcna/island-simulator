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
        constexpr int MaximumVerticesPerButterfly = 228;
        constexpr int MaximumFlowerVertices = 1800;
        constexpr int MaximumGeckoVertices = static_cast<int>(GeckoPopulation::Count) * 900;
        constexpr int MaximumSkinkVertices = static_cast<int>(SkinkPopulation::Count) * 900;
        constexpr int MaximumDragonflyVertices =
            static_cast<int>(DragonflySwarm::Count) * 720;
        constexpr int MaximumFrogVertices = static_cast<int>(FrogPopulation::Count) * 720;
        constexpr int MaximumRodentVertices = static_cast<int>(RodentPopulation::Count) * 720;
        // A close fly uses 348 vertices: 192 body, 24 head, 12 wings,
        // 48 eyes and 72 legs. Keep a small per-agent margin for later detail edits.
        constexpr int MaximumFlyVertices = static_cast<int>(FlySwarms::Count) * 360;
        constexpr int MaximumAntVertices = static_cast<int>(AntTrails::Count) * 360;
        // A close cicada uses at most 684 vertices including the broad lofted head,
        // segmented abdomen, wing veins and all six two-section legs.
        constexpr int MaximumCicadaVertices = static_cast<int>(CicadaColony::Count) * 720;
        constexpr int MaximumBeetleVertices = static_cast<int>(BeetlePopulation::Count) * 660;
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
        constexpr std::string_view ButterflyHabitatDiagnostic =
            "island-simulator: butterflies feed at three visible deterministic nectar-flower patches";
        constexpr std::string_view GeckoDiagnostic =
            "island-simulator: eight inland geckos rest, scurry from the player, and hide in the shared butterfly batch";
        constexpr std::string_view SkinkDiagnostic =
            "island-simulator: eight open-ground skinks bask, forage, and flee in the shared butterfly batch";
        constexpr std::string_view SkinkAnatomyDiagnostic =
            "island-simulator: skinks use smooth elongated trunks, short articulated limbs, tapered tails, longitudinal stripes, and highlighted eyes";
        constexpr std::string_view DragonflyDiagnostic=
            "island-simulator: nine stream and wetland dragonflies hover, patrol, and dart in the shared butterfly batch";
        constexpr std::string_view FrogDiagnostic =
            "island-simulator: ten stream and wetland frogs rest, hop, and retreat in the shared butterfly batch";
        constexpr std::string_view RodentDiagnostic =
            "island-simulator: twelve field mice and eight rats forage, scurry, and hide in the shared small-fauna batch";
        constexpr std::string_view RodentAnatomyDiagnostic =
            "island-simulator: near rodents use species-scaled lofted bodies, pointed muzzles, rounded ears, whiskers, paws, eyes, and long segmented tails";
        constexpr std::string_view FlyDiagnostic =
            "island-simulator: four close-range fly swarms use twenty-four hover, orbit, and dart agents in the shared small-fauna batch";
        constexpr std::string_view FlyAnatomyDiagnostic =
            "island-simulator: near flies use compact dark thoraxes, tapered abdomens, highlighted eyes, six legs, and two beating pale wings";
        constexpr std::string_view AntDiagnostic =
            "island-simulator: four close-range ant trails use sixty-four soil and trunk-climbing agents in the shared small-fauna batch";
        constexpr std::string_view AntAnatomyDiagnostic =
            "island-simulator: close ants use three-part bodies, six articulated legs, paired antennae, and visible carried leaf fragments";
        constexpr std::string_view CicadaDiagnostic =
            "island-simulator: twelve cicadas rest, call, and make short flights from real broadleaf trunks in the shared small-fauna batch";
        constexpr std::string_view CicadaAnatomyDiagnostic =
            "island-simulator: close cicadas use broad heads, highlighted eyes, stout tapered bodies, six legs, and two veined roof-folded wings";
        constexpr std::string_view BeetleDiagnostic =
            "island-simulator: fifteen tropical ground beetles rest, forage, and hide in three inland leaf-litter colonies";
        constexpr std::string_view BeetleAnatomyDiagnostic =
            "island-simulator: close beetles use domed paired elytra, a centre seam, pronotum, head, antennae, eyes, and six articulated legs";

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
            constexpr std::array<float,5> BodyStations={{-.23f,-.14f,.0f,.14f,.22f}};
            constexpr std::array<float,5> BodyWidths={{.045f,.105f,.13f,.115f,.075f}};
            constexpr std::array<float,5> BodyHeights={{.035f,.072f,.082f,.076f,.052f}};
            AppendGeckoLoft(vertices,body,right,up,forward,BodyStations,BodyWidths,
                            BodyHeights,scale,back,flank,underside);
            const Vector3 head=centre+forward*(.275f*scale)+up*(.018f*scale);
            constexpr std::array<float,4> HeadStations={{-.07f,.0f,.105f,.15f}};
            constexpr std::array<float,4> HeadWidths={{.070f,.115f,.105f,.055f}};
            constexpr std::array<float,4> HeadHeights={{.042f,.060f,.055f,.032f}};
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
                    AppendGeckoSegment(vertices, root, knee, .016f * scale, flank);
                    AppendGeckoSegment(vertices, knee, toe, .012f * scale, back);
                    for(int digit=-1;digit<=1;++digit) {
                        const Vector3 digitEnd=toe+right*(sideSign*.055f*scale)
                            +forward*((static_cast<float>(digit)*.030f
                                +(pair==0?.018f:-.018f))*scale);
                        AppendGeckoSegment(vertices,toe,digitEnd,.0055f*scale,underside);
                    }
                }
            }
            Vector3 tailRoot = body - forward * (.19f * scale);
            for (int segment = 0; segment < 4; ++segment) {
                const float phase = gecko.gaitPhase * .35f + static_cast<float>(segment) * .72f;
                const float length = (.15f - static_cast<float>(segment) * .018f) * scale;
                const Vector3 tailEnd = tailRoot - forward * length
                    + right * (std::sin(phase) * .035f * scale);
                AppendGeckoSegment(vertices, tailRoot, tailEnd,
                    (.036f - static_cast<float>(segment) * .006f) * scale, flank);
                tailRoot = tailEnd;
            }
            for (int sideIndex = -1; sideIndex <= 1; sideIndex += 2) {
                const float sideSign = static_cast<float>(sideIndex);
                const Vector3 eyeCentre = head + forward * (.065f * scale)
                    + right * (sideSign * .075f * scale) + up * (.035f * scale);
                AppendGeckoDiamond(vertices, eyeCentre, right, up, forward,
                                   .018f * scale, .018f * scale, .012f * scale, eye, eye);
                const Vector3 pupilCentre=eyeCentre+right*(sideSign*.015f*scale)
                    +up*(.006f*scale);
                AddDoubleSidedTriangle(vertices,pupilCentre+up*(.009f*scale),
                    pupilCentre-forward*(.010f*scale)-up*(.007f*scale),
                    pupilCentre+forward*(.010f*scale)-up*(.007f*scale),pupil);
            }
            for(int spot=0;spot<5;++spot) {
                const float z=(-.15f+static_cast<float>(spot)*.075f)*scale;
                const float width=(spot%2==0?.045f:.032f)*scale;
                const Vector3 spotCentre=body+forward*z+up*(.086f*scale);
                AddDoubleSidedTriangle(vertices,spotCentre+forward*(.018f*scale),
                    spotCentre-right*width-forward*(.014f*scale),
                    spotCentre+right*width-forward*(.014f*scale),marking);
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
                    AppendGeckoSegment(vertices, root, elbow, .013f * scale, flank);
                    AppendGeckoSegment(vertices, elbow, foot, .009f * scale, back);
                    for (int toe = -1; toe <= 1; ++toe) {
                        const Vector3 tip = foot + right * (side * .040f * scale)
                            + forward * ((static_cast<float>(toe) * .018f
                                + (pair == 0 ? .010f : -.010f)) * scale);
                        AppendGeckoSegment(vertices, foot, tip, .0038f * scale, underside);
                    }
                }
            }

            Vector3 tailRoot = centre - forward * (.275f * scale);
            for (int segment = 0; segment < 5; ++segment) {
                const float length = (.145f - static_cast<float>(segment) * .014f) * scale;
                const Vector3 tailEnd = tailRoot - forward * length
                    + right * (std::sin(skink.gaitPhase * .24f
                        + static_cast<float>(segment) * .66f) * .025f * scale);
                AppendGeckoSegment(vertices, tailRoot, tailEnd,
                    (.030f - static_cast<float>(segment) * .0045f) * scale, flank);
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
                    + right * (side * .056f * scale) + up * (.028f * scale);
                AppendGeckoDiamond(vertices, eyeCentre, right, up, forward,
                                   .012f * scale, .013f * scale, .010f * scale, eye, eye);
                const Vector3 highlight = eyeCentre + right * (side * .009f * scale)
                    + forward * (.006f * scale) + up * (.007f * scale);
                AddDoubleSidedTriangle(vertices, highlight + up * (.0035f * scale),
                    highlight - forward * (.0035f * scale) - up * (.002f * scale),
                    highlight + forward * (.0035f * scale) - up * (.002f * scale), glint);
            }
        }

        void AppendDragonfly(std::vector<VertexPositionColor>& vertices,
                             const DragonflyState& fly)
        {
            const std::array<Color, 3> bodies = {{
                Color(33, 116, 124, 255), Color(45, 89, 151, 255),
                Color(62, 126, 69, 255)}};
            const Color bodyColor = bodies[fly.colorVariant % bodies.size()];
            const Color dark = Color::Lerp(bodyColor, Color(10, 17, 18, 255), .68f);
            const Color abdomenBand = Color::Lerp(bodyColor, Color(174, 178, 93, 255), .18f);
            const Color wing(184, 202, 198, 255);
            const Color wingVein(83, 111, 108, 255);
            const Color eye(75, 117, 64, 255);
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

            constexpr std::array<float, 4> ThoraxStations = {{-.060f, -.020f, .035f, .075f}};
            constexpr std::array<float, 4> ThoraxWidths = {{.022f, .055f, .048f, .024f}};
            constexpr std::array<float, 4> ThoraxHeights = {{.018f, .044f, .038f, .018f}};
            AppendGeckoLoft(vertices, centre, right, up, forward, ThoraxStations,
                            ThoraxWidths, ThoraxHeights, s, bodyColor, dark, dark);

            const Vector3 head = centre + forward * (.098f * s) + up * (.006f * s);
            AppendGeckoDiamond(vertices, head, right, up, forward,
                               .052f * s, .039f * s, .036f * s, dark, dark);

            // Seven visibly separated, steadily tapering abdominal segments give
            // the insect its characteristic long needle silhouette.
            Vector3 root = centre - forward * (.052f * s);
            for (int segment = 0; segment < 7; ++segment) {
                const float length = (.058f - .0025f * static_cast<float>(segment)) * s;
                const Vector3 end = root - forward * length
                    + right * (std::sin(fly.wingPhase * .08f
                                        + static_cast<float>(segment) * .42f)
                               * .0025f * s);
                const Vector3 middle = (root + end) * .5f;
                const float radius = (.018f - .0017f * static_cast<float>(segment)) * s;
                AppendGeckoDiamond(vertices, middle, right, up, forward,
                                   radius, radius * .78f, length * .53f,
                                   segment % 2 == 0 ? bodyColor : abdomenBand, dark);
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
                    AppendGeckoSegment(vertices, hip, knee, .0030f * s, dark);
                    AppendGeckoSegment(vertices, knee, foot, .0022f * s, dark);
                }
            }

            for (const float side : {-1.0f, 1.0f}) {
                const Vector3 eyeCentre = head + right * (side * .040f * s)
                    + forward * (.020f * s) + up * (.014f * s);
                AppendGeckoDiamond(vertices, eyeCentre, right, up, forward,
                                   .019f * s, .022f * s, .016f * s, eye, eye);
                const Vector3 glint = eyeCentre + right * (side * .015f * s)
                    + forward * (.013f * s) + up * (.012f * s);
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
                AppendGeckoSegment(vertices, hip, knee, .055f * scale, flank);
                AppendGeckoSegment(vertices, knee, ankle, .039f * scale, back);
                AppendGeckoSegment(vertices, ankle, rearToe, .020f * scale, underside);
                for (int toe = -1; toe <= 1; ++toe) {
                    const Vector3 tip = rearToe + forward * (.095f * scale)
                        + right * ((side * .025f + static_cast<float>(toe) * .045f) * scale);
                    AppendGeckoSegment(vertices, rearToe, tip, .008f * scale, underside);
                }

                const Vector3 shoulder = head - forward * (.075f * scale)
                    + right * (side * .17f * scale) - up * (.04f * scale);
                const Vector3 wrist = shoulder + forward * ((.15f + extension * .10f) * scale)
                    + right * (side * .055f * scale) - up * (.11f * scale);
                AppendGeckoSegment(vertices, shoulder, wrist, .026f * scale, flank);
                for (int toe = -1; toe <= 1; ++toe) {
                    const Vector3 tip = wrist + forward * (.095f * scale)
                        + right * ((side * .025f + static_cast<float>(toe) * .025f) * scale);
                    AppendGeckoSegment(vertices, wrist, tip, .007f * scale, underside);
                }
            }

            // Raised dorsolateral folds and a few restrained spots break the smooth green shell.
            for (const float side : {-1.0f, 1.0f}) {
                const Vector3 foldA = centre - forward * (.16f * scale)
                    + right * (side * .13f * scale) + up * (.15f * scale);
                const Vector3 foldB = centre + forward * (.19f * scale)
                    + right * (side * .15f * scale) + up * (.13f * scale);
                AppendGeckoSegment(vertices, foldA, foldB, .010f * scale, marking);
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
                const Vector3 irisCentre = eyeCentre + right * (side * .050f * scale)
                    + forward * (.012f * scale) + up * (.004f * scale);
                AppendGeckoDiamond(vertices, irisCentre, right, up, forward,
                                   .023f * scale, .029f * scale, .014f * scale, iris, iris);
                const Vector3 pupilCentre = irisCentre + right * (side * .015f * scale)
                    + forward * (.005f * scale);
                AddDoubleSidedTriangle(vertices, pupilCentre + up * (.035f * scale),
                    pupilCentre - forward * (.012f * scale) - up * (.035f * scale),
                    pupilCentre + forward * (.012f * scale) - up * (.035f * scale), pupil);
                const Vector3 highlight = pupilCentre + up * (.020f * scale)
                    + right * (side * .004f * scale);
                AddDoubleSidedTriangle(vertices, highlight + up * (.007f * scale),
                    highlight - forward * (.006f * scale) - up * (.004f * scale),
                    highlight + forward * (.006f * scale) - up * (.004f * scale), glint);
            }
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
            const Color eye(78, 32, 25), wing(158, 170, 165), leg(31, 28, 24);
            const Vector3 centre(fly.x, fly.y, fly.z);
            constexpr std::array<float, 4> Stations = {{-.15f, -.04f, .12f, .23f}};
            constexpr std::array<float, 4> Widths = {{.035f, .085f, .080f, .025f}};
            constexpr std::array<float, 4> Heights = {{.035f, .075f, .070f, .025f}};
            AppendGeckoLoft(vertices, centre, right, up, forward, Stations, Widths,
                            Heights, scale, body, body, body);
            const Vector3 head = centre + forward * (.235f * scale)
                + up * (.012f * scale);
            AppendGeckoDiamond(vertices, head, right, up, forward,
                .075f * scale, .070f * scale, .060f * scale, body, body);
            const float beat = std::sin(fly.wingPhase);
            for (const float side : {-1.0f, 1.0f}) {
                const Vector3 root = centre - forward * (.015f * scale)
                    + right * (side * .055f * scale) + up * (.055f * scale);
                const Vector3 tip = root + right * (side * .23f * scale)
                    - forward * (.19f * scale) + up * (beat * .10f * scale);
                const Vector3 leading = root + forward * (.08f * scale)
                    + right * (side * .035f * scale);
                const Vector3 trailing = root - forward * (.12f * scale)
                    + right * (side * .02f * scale);
                AddDoubleSidedTriangle(vertices, leading, tip, trailing, wing);
                const Vector3 eyeCentre = head + forward * (.025f * scale)
                    + right * (side * .058f * scale) + up * (.010f * scale);
                AppendGeckoDiamond(vertices, eyeCentre, right, up, forward,
                    .026f * scale, .032f * scale, .022f * scale, eye, eye);
                for (int legIndex = 0; legIndex < 3; ++legIndex) {
                    const Vector3 hip = centre + forward * ((.09f - legIndex * .08f) * scale)
                        + right * (side * .05f * scale) - up * (.035f * scale);
                    const Vector3 foot = hip + right * (side * (.10f + .025f * legIndex) * scale)
                        - forward * ((.02f + .04f * legIndex) * scale)
                        - up * (.09f * scale);
                    AppendGeckoSegment(vertices, hip, foot, .009f * scale, leg);
                }
            }
        }

        void AppendAntBody(std::vector<VertexPositionColor>& vertices,const Vector3& centre,
                           const Vector3& right,const Vector3& up,const Vector3& forward,
                           const float halfWidth,const float halfHeight,const float halfLength,
                           const Color& dorsal,const Color& flank)
        {
            constexpr int Sides=8;
            const Vector3 front=centre+forward*halfLength,rear=centre-forward*halfLength;
            std::array<Vector3,Sides> ring{};
            for(int side=0;side<Sides;++side) {
                const float angle=6.28318530718f*static_cast<float>(side)/Sides;
                ring[side]=centre+right*(std::cos(angle)*halfWidth)
                    +up*(std::sin(angle)*halfHeight);
            }
            for(int side=0;side<Sides;++side) {
                const int next=(side+1)%Sides;
                const Color color=side>=1&&side<=3?dorsal:flank;
                AddTriangle(vertices,front,ring[side],ring[next],color);
                AddTriangle(vertices,rear,ring[next],ring[side],color);
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
            const Vector3 centre(ant.x,ant.y,ant.z);
            AppendAntBody(vertices,centre,right,up,forward,
                .34f*unit,.30f*unit,.40f*unit,body,flank);
            const Vector3 head=centre+forward*(.70f*unit)+up*(.02f*unit);
            AppendAntBody(vertices,head,right,up,forward,
                .31f*unit,.29f*unit,.34f*unit,body,flank);
            const Vector3 abdomen=centre-forward*(.82f*unit)+up*(.04f*unit);
            AppendAntBody(vertices,abdomen,right,up,forward,
                .48f*unit,.40f*unit,.58f*unit,body,flank);
            const float step=std::sin(ant.gaitPhase);
            for(const float side:{-1.0f,1.0f}) for(int legIndex=0;legIndex<3;++legIndex) {
                const float along=(.36f-static_cast<float>(legIndex)*.38f)*unit;
                const float swing=step*side*(legIndex%2==0?1.0f:-1.0f)*.18f*unit;
                const Vector3 hip=centre+forward*along+right*(side*.25f*unit);
                const Vector3 knee=hip+right*(side*.64f*unit)
                    +forward*((.18f-static_cast<float>(legIndex)*.18f)*unit+swing)
                    +up*(.10f*unit);
                const Vector3 foot=knee+right*(side*.54f*unit)
                    +forward*((.28f-static_cast<float>(legIndex)*.28f)*unit+swing)
                    -up*(.18f*unit);
                AppendGeckoSegment(vertices,hip,knee,.055f*unit,leg);
                AppendGeckoSegment(vertices,knee,foot,.040f*unit,leg);
            }
            for(const float side:{-1.0f,1.0f}) {
                const Vector3 root=head+forward*(.27f*unit)+right*(side*.16f*unit);
                const Vector3 elbow=root+forward*(.43f*unit)+right*(side*.24f*unit)
                    +up*(.12f*unit);
                const Vector3 tip=elbow+forward*(.34f*unit)+right*(side*.25f*unit)
                    -up*(.08f*unit);
                AppendGeckoSegment(vertices,root,elbow,.032f*unit,leg);
                AppendGeckoSegment(vertices,elbow,tip,.024f*unit,leg);
            }
            if(ant.behavior==AntBehavior::Carry) {
                const Color leaf(76,112,42);
                const Vector3 leafCentre=centre+up*(.92f*unit)-forward*(.10f*unit);
                AddDoubleSidedTriangle(vertices,leafCentre-forward*(1.18f*unit),
                    leafCentre+right*(.62f*unit),leafCentre+forward*(1.28f*unit),leaf);
                AddDoubleSidedTriangle(vertices,leafCentre-forward*(1.18f*unit),
                    leafCentre+forward*(1.28f*unit),leafCentre-right*(.62f*unit),
                    Color(58,91,34));
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
            const Color eye(54, 35, 24), glint(221, 194, 128);
            const Vector3 centre(cicada.x, cicada.y, cicada.z);
            const Vector3 thorax = centre + dorsal * (.34f * unit);
            AppendAntBody(vertices, thorax, right, dorsal, forward,
                .59f * unit, .45f * unit, .50f * unit,
                Color::Lerp(body, Color(155, 132, 64), .14f), flank);
            const Vector3 head = thorax + forward * (.63f * unit);
            constexpr std::array<float, 4> HeadStations = {{-.30f, -.10f, .18f, .34f}};
            constexpr std::array<float, 4> HeadWidths = {{.37f, .50f, .48f, .27f}};
            constexpr std::array<float, 4> HeadHeights = {{.23f, .32f, .30f, .16f}};
            AppendGeckoLoft(vertices, head, right, dorsal, forward, HeadStations,
                            HeadWidths, HeadHeights, unit, body, flank, flank);
            const std::array<float, 3> abdomenOffsets = {{-.48f, -.89f, -1.23f}};
            const std::array<float, 3> abdomenWidths = {{.46f, .40f, .29f}};
            for (std::size_t segment = 0; segment < abdomenOffsets.size(); ++segment) {
                const Vector3 abdomen = thorax + forward * (abdomenOffsets[segment] * unit);
                AppendAntBody(vertices, abdomen, right, dorsal, forward,
                    abdomenWidths[segment] * unit,
                    (.31f - .045f * static_cast<float>(segment)) * unit,
                    .34f * unit,
                    segment % 2U == 0U ? body : Color::Lerp(body, flank, .18f), flank);
            }

            for (const float side : {-1.0f, 1.0f}) {
                const float beat = cicada.behavior == CicadaBehavior::Flight
                    ? std::sin(cicada.wingPhase) * .35f : .08f;
                const Vector3 root = thorax + right * (side * .28f * unit)
                    + dorsal * (.31f * unit) - forward * (.04f * unit);
                const Vector3 leading = root + right * (side * .48f * unit)
                    - forward * (.18f * unit) + dorsal * (beat * unit);
                const Vector3 tip = root + right * (side * .82f * unit)
                    - forward * (1.48f * unit) + dorsal * (beat * .72f * unit);
                const Vector3 trailing = root + right * (side * .18f * unit)
                    - forward * (1.32f * unit) + dorsal * (.05f * unit);
                AddDoubleSidedTriangle(vertices, root, leading, tip, wing);
                AddDoubleSidedTriangle(vertices, root, tip, trailing,
                                       Color::Lerp(wing, body, .18f));
                AppendGeckoSegment(vertices, root, tip, .025f * unit, vein);
                AppendGeckoSegment(vertices, leading, trailing, .018f * unit, vein);
                AppendGeckoSegment(vertices,
                    root + (leading - root) * .62f,
                    root + (tip - root) * .76f, .014f * unit, vein);

                const Vector3 eyeCentre = head + forward * (.12f * unit)
                    + right * (side * .43f * unit) + dorsal * (.15f * unit);
                AppendGeckoDiamond(vertices, eyeCentre, right, dorsal, forward,
                    .095f * unit, .11f * unit, .085f * unit, eye, eye);
                AddDoubleSidedTriangle(vertices,
                    eyeCentre + dorsal * (.115f * unit),
                    eyeCentre + forward * (.055f * unit),
                    eyeCentre + right * (side * .045f * unit), glint);

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
                    AppendGeckoSegment(vertices, hip, knee, .075f * unit, legColor);
                    AppendGeckoSegment(vertices, knee, foot, .052f * unit, legColor);
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
            const std::array<Color, 4> shells = {{Color(43, 119, 81), Color(137, 72, 35),
                                                  Color(42, 78, 132), Color(133, 101, 29)}};
            const Color shell = shells[beetle.colorVariant % shells.size()];
            const Color shellLight = Color::Lerp(shell, Color(146, 143, 92), .28f);
            const Color underside = Color::Lerp(shell, Color(10, 11, 10), .58f);
            const Color limb = Color::Lerp(shell, Color(13, 12, 10), .48f);
            const Color eye(18, 12, 8), glint(205, 181, 112);
            const Vector3 centre(beetle.x, beetle.y, beetle.z);
            const Vector3 abdomen = centre + dorsal * (.34f * unit) - forward * (.20f * unit);
            constexpr std::array<float, 6> ElytronStations =
                {{-.82f, -.68f, -.28f, .20f, .55f, .70f}};
            constexpr std::array<float, 6> ElytronWidths =
                {{.10f, .26f, .34f, .35f, .28f, .10f}};
            constexpr std::array<float, 6> ElytronHeights =
                {{.08f, .22f, .34f, .38f, .28f, .09f}};
            for (const float side : {-1.0f, 1.0f}) {
                const Vector3 elytron = abdomen + right * (side * .28f * unit);
                AppendGeckoLoft(vertices, elytron, right, dorsal, forward,
                    ElytronStations, ElytronWidths, ElytronHeights, unit,
                    side < 0.0f ? shell : shellLight,
                    Color::Lerp(shell, underside, .28f), underside);
            }
            AppendGeckoSegment(vertices,
                abdomen + dorsal * (.36f * unit) + forward * (.50f * unit),
                abdomen + dorsal * (.36f * unit) - forward * (.88f * unit),
                .025f * unit, underside);
            const Vector3 pronotum = centre + forward * (.63f * unit)
                + dorsal * (.28f * unit);
            constexpr std::array<float, 4> PronotumStations = {{-.34f, -.18f, .18f, .34f}};
            constexpr std::array<float, 4> PronotumWidths = {{.32f, .52f, .50f, .25f}};
            constexpr std::array<float, 4> PronotumHeights = {{.15f, .28f, .27f, .12f}};
            AppendGeckoLoft(vertices, pronotum, right, dorsal, forward,
                PronotumStations, PronotumWidths, PronotumHeights, unit,
                shellLight, shell, underside);
            const Vector3 head = pronotum + forward * (.52f * unit) - dorsal * (.03f * unit);
            constexpr std::array<float, 4> HeadStations = {{-.28f, -.12f, .18f, .31f}};
            constexpr std::array<float, 4> HeadWidths = {{.22f, .34f, .32f, .15f}};
            constexpr std::array<float, 4> HeadHeights = {{.12f, .22f, .20f, .08f}};
            AppendGeckoLoft(vertices, head, right, dorsal, forward,
                HeadStations, HeadWidths, HeadHeights, unit, shell, shell, underside);
            const float gait = std::sin(beetle.gaitPhase);
            for (const float side : {-1.0f, 1.0f}) {
                for (int legIndex = 0; legIndex < 3; ++legIndex) {
                    const float along = (.48f - .52f * static_cast<float>(legIndex)) * unit;
                    const float swing = gait * (legIndex % 2 == 0 ? 1.0f : -1.0f) * .13f * unit;
                    const Vector3 hip = centre + forward * along
                        + right * (side * .38f * unit) + dorsal * (.18f * unit);
                    const Vector3 knee = hip + right * (side * .30f * unit)
                        + forward * swing - dorsal * (.16f * unit);
                    const Vector3 foot = knee + right * (side * .28f * unit)
                        + forward * ((.14f - .14f * legIndex) * unit + swing)
                        - dorsal * (.20f * unit);
                    AppendGeckoSegment(vertices, hip, knee, .080f * unit, limb);
                    AppendGeckoSegment(vertices, knee, foot, .055f * unit, limb);
                }
                const Vector3 eyeCentre = head + forward * (.20f * unit)
                    + right * (side * .27f * unit) + dorsal * (.10f * unit);
                AppendGeckoDiamond(vertices, eyeCentre, right, dorsal, forward,
                    .065f * unit, .072f * unit, .060f * unit, eye, eye);
                AddDoubleSidedTriangle(vertices, eyeCentre + dorsal * (.075f * unit),
                    eyeCentre + forward * (.035f * unit),
                    eyeCentre + right * (side * .030f * unit), glint);
                const Vector3 antennaRoot = head + forward * (.27f * unit)
                    + right * (side * .15f * unit);
                const Vector3 antennaKnee = antennaRoot + forward * (.42f * unit)
                    + right * (side * .22f * unit) + dorsal * (.10f * unit);
                const Vector3 antennaTip = antennaKnee + forward * (.32f * unit)
                    + right * (side * .24f * unit) - dorsal * (.04f * unit);
                AppendGeckoSegment(vertices, antennaRoot, antennaKnee, .025f * unit, limb);
                AppendGeckoSegment(vertices, antennaKnee, antennaTip, .018f * unit, limb);
            }
        }

        void AppendRodentEar(std::vector<VertexPositionColor>& vertices,
                             const Vector3& centre, const Vector3& horizontal,
                             const Vector3& vertical, const float radius,
                             const Color& outer, const Color& inner)
        {
            constexpr int Segments = 10;
            for (int segment = 0; segment < Segments; ++segment) {
                const float a = 6.28318530718f * static_cast<float>(segment) / Segments;
                const float b = 6.28318530718f * static_cast<float>(segment + 1) / Segments;
                const Vector3 edgeA = centre + horizontal * (std::cos(a) * radius)
                    + vertical * (std::sin(a) * radius * 1.08f);
                const Vector3 edgeB = centre + horizontal * (std::cos(b) * radius)
                    + vertical * (std::sin(b) * radius * 1.08f);
                AddDoubleSidedTriangle(vertices, centre, edgeA, edgeB,
                    segment % 2 == 0 ? inner : outer);
            }
        }

        void AppendRodent(std::vector<VertexPositionColor>& vertices,
                          const RodentState& rodent)
        {
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
            constexpr std::array<float, 6> BodyStations = {{-.18f, -.12f, -.02f, .10f, .19f, .24f}};
            constexpr std::array<float, 6> BodyWidths = {{.055f, .11f, .14f, .135f, .095f, .045f}};
            constexpr std::array<float, 6> BodyHeights = {{.045f, .085f, .105f, .10f, .075f, .035f}};
            AppendGeckoLoft(vertices, centre, right, up, forward, BodyStations, BodyWidths,
                            BodyHeights, unit, coat, flank, underside);
            const float sniff = std::sin(rodent.sniffPhase) * .008f * unit;
            const Vector3 head = centre + forward * (.245f * unit)
                + up * ((rat ? .018f : .028f) * unit + sniff);
            constexpr std::array<float, 5> MouseHeadStations = {{-.08f, -.01f, .075f, .145f, .20f}};
            constexpr std::array<float, 5> MouseHeadWidths = {{.075f, .11f, .095f, .055f, .018f}};
            constexpr std::array<float, 5> MouseHeadHeights = {{.055f, .092f, .080f, .045f, .018f}};
            constexpr std::array<float, 5> RatHeadStations = {{-.08f, -.01f, .09f, .18f, .24f}};
            constexpr std::array<float, 5> RatHeadWidths = {{.070f, .105f, .082f, .043f, .016f}};
            constexpr std::array<float, 5> RatHeadHeights = {{.052f, .086f, .068f, .038f, .016f}};
            AppendGeckoLoft(vertices, head, right, up, forward,
                            rat ? RatHeadStations : MouseHeadStations,
                            rat ? RatHeadWidths : MouseHeadWidths,
                            rat ? RatHeadHeights : MouseHeadHeights,
                            unit, coat, flank, underside);

            for (const float side : {-1.0f, 1.0f}) {
                const Vector3 earCentre = head - forward * (.035f * unit)
                    + right * (side * .085f * unit) + up * (.080f * unit);
                const float earSize = (rat ? .040f : .072f) * unit;
                AppendRodentEar(vertices, earCentre, forward, up, earSize, flank, ear);
                const Vector3 eyeCentre = head + forward * (.070f * unit)
                    + right * (side * .083f * unit) + up * (.026f * unit);
                const float eyeSize = (rat ? .016f : .022f) * unit;
                AppendGeckoDiamond(vertices, eyeCentre, right, up, forward,
                    eyeSize, eyeSize * 1.09f, eyeSize * .68f, eye, eye);
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
            AppendGeckoDiamond(vertices, nose, right, up, forward,
                .019f * unit, .018f * unit, .023f * unit, eye, eye);
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
                AppendGeckoSegment(vertices, hip, ankle, .014f * unit, flank);
                AppendGeckoSegment(vertices, ankle, paw, .009f * unit, underside);
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
                AppendGeckoSegment(vertices, start, end,
                    (.012f - .0025f * segment) * unit, tail);
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
            const Vector3 bodyFront=LocalPoint(
                centre,right,up,forward,0,.012f,.21f,scale);
            const Vector3 bodyRear=LocalPoint(
                centre,right,up,forward,0,-.012f,-.38f,scale);
            const auto bodyRing=[&](float angle,float z,float radius) {
                return LocalPoint(centre,right,up,forward,std::cos(angle)*radius,
                    std::sin(angle)*radius,z,scale);
            };
            for(int sideIndex=0;sideIndex<BodySides;++sideIndex) {
                const float a0=6.28318530718f*static_cast<float>(sideIndex)/BodySides;
                const float a1=6.28318530718f*static_cast<float>(sideIndex+1)/BodySides;
                const Vector3 front0=bodyRing(a0,.12f,.055f),front1=bodyRing(a1,.12f,.055f);
                const Vector3 rear0=bodyRing(a0,-.15f,.050f),rear1=bodyRing(a1,-.15f,.050f);
                AddTriangle(vertices,bodyFront,front0,front1,palette.body);
                AddTriangle(vertices,front0,rear0,rear1,palette.body);
                AddTriangle(vertices,front0,rear1,front1,palette.body);
                AddTriangle(vertices,bodyRear,rear1,rear0,palette.body);
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
                const Vector3 foreTip=wingPoint(.64f,.18f);
                const Vector3 outerTip=wingPoint(.68f,-.05f);
                const Vector3 hindTip=wingPoint(.52f,-.33f);
                const Vector3 rearTip=wingPoint(.28f,-.42f);
                AddDoubleSidedTriangle(
                    vertices,foreRoot,leadingTip,foreTip,palette.forewing);
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
                const Vector3 accentTip=wingPoint(.43f,-.02f,.008f);
                AddDoubleSidedTriangle(vertices,foreRoot,accentTip,hindRoot,
                    palette.accent);
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
        std::cout << ButterflyHabitatDiagnostic << '\n';
        std::cout << GeckoDiagnostic << '\n';
        std::cout << SkinkDiagnostic << '\n';
        std::cout << SkinkAnatomyDiagnostic << '\n';
        std::cout << DragonflyDiagnostic << '\n';
        std::cout << FrogDiagnostic << '\n';
        std::cout << RodentDiagnostic << '\n';
        std::cout << RodentAnatomyDiagnostic << '\n';
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
        MirrorRuntimeDiagnosticToPlatformLog(ButterflyHabitatDiagnostic);
        MirrorRuntimeDiagnosticToPlatformLog(GeckoDiagnostic);
        MirrorRuntimeDiagnosticToPlatformLog(SkinkDiagnostic);
        MirrorRuntimeDiagnosticToPlatformLog(SkinkAnatomyDiagnostic);
        MirrorRuntimeDiagnosticToPlatformLog(DragonflyDiagnostic);
        MirrorRuntimeDiagnosticToPlatformLog(FrogDiagnostic);
        MirrorRuntimeDiagnosticToPlatformLog(RodentDiagnostic);
        MirrorRuntimeDiagnosticToPlatformLog(RodentAnatomyDiagnostic);
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
            AppendAnt(stagingVertices_,ant);
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
            AppendBeetle(stagingVertices_, beetle);
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
