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
        constexpr int MaximumDragonflyVertices =
            static_cast<int>(DragonflySwarm::Count) * 720;
        constexpr int MaximumVertexCount = static_cast<int>(ButterflyMeadow::ButterflyCount)
            * MaximumVerticesPerButterfly + MaximumFlowerVertices + MaximumGeckoVertices
            +MaximumDragonflyVertices;
        constexpr std::string_view ButterflyDiagnostic =
            "island-simulator: clearing butterflies use hinged patterned wings in one culled batch";
        constexpr std::string_view ButterflyAnatomyDiagnostic =
            "island-simulator: near butterflies use closed bodies, paired antennae, rounded wing lobes, and dark margins";
        constexpr std::string_view ButterflyHabitatDiagnostic =
            "island-simulator: butterflies feed at three visible deterministic nectar-flower patches";
        constexpr std::string_view GeckoDiagnostic =
            "island-simulator: eight inland geckos rest, scurry from the player, and hide in the shared butterfly batch";
        constexpr std::string_view DragonflyDiagnostic=
            "island-simulator: nine stream and wetland dragonflies hover, patrol, and dart in the shared butterfly batch";

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
        std::cout << DragonflyDiagnostic << '\n';
        MirrorRuntimeDiagnosticToPlatformLog(ButterflyDiagnostic);
        MirrorRuntimeDiagnosticToPlatformLog(ButterflyAnatomyDiagnostic);
        MirrorRuntimeDiagnosticToPlatformLog(ButterflyHabitatDiagnostic);
        MirrorRuntimeDiagnosticToPlatformLog(GeckoDiagnostic);
        MirrorRuntimeDiagnosticToPlatformLog(DragonflyDiagnostic);
    }

    ButterflyRenderer::~ButterflyRenderer() = default;

    void ButterflyRenderer::Draw(const Matrix& view, const Matrix& projection,
                                  const Vector3& cameraPosition,
                                  const std::span<const ButterflyState> butterflies,
                                  const std::span<const ButterflyFlowerPatch> flowerPatches,
                                  const std::span<const GeckoState> geckos,
                                  const std::span<const DragonflyState> dragonflies)
    {
        stagingVertices_.clear();
        visibleButterflyCount_ = 0;
        visibleNearButterflyCount_ = 0;
        visibleFarButterflyCount_ = 0;
        visibleGeckoCount_ = 0;
        visibleDragonflyCount_=0;
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
        for(const DragonflyState& fly:dragonflies){
            const Vector3 centre(fly.x,fly.y,fly.z);
            if(Vector3::DistanceSquared(centre,cameraPosition)>maximumDistance_*maximumDistance_
                ||!frustum.Intersects(BoundingSphere(centre,.46f*fly.scale)))continue;
            AppendDragonfly(stagingVertices_,fly);++visibleDragonflyCount_;
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
