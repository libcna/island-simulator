#include "IslandSimulator/DeerRenderer.hpp"

#include "IslandSimulator/CanopyLightField.hpp"
#include "IslandSimulator/EnvironmentLighting.hpp"
#include "IslandSimulator/RuntimeDiagnostics.hpp"
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

#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <utility>

using namespace Microsoft::Xna::Framework;
using namespace Microsoft::Xna::Framework::Graphics;

namespace island
{
    namespace
    {
        constexpr int MaximumVerticesPerDeer = 6500;
        constexpr int Capacity = static_cast<int>(DeerHerd::Count) * MaximumVerticesPerDeer;

        void Triangle(std::vector<VertexPositionColor>& out, const Vector3& a,
                      const Vector3& b, const Vector3& c, const Color color)
        {
            out.emplace_back(a, color);out.emplace_back(b, color);out.emplace_back(c, color);
        }

        Color Lit(const Color base, const Vector3& normal, const float transmission)
        {
            const Vector3 sun = Vector3::Normalize(Vector3(.46f, .83f, -.31f));
            const float light = (.55f + .40f * std::max(0.0f, Vector3::Dot(normal, sun)))
                * (.72f + .28f * transmission);
            return Color(static_cast<int>(std::clamp(
                             std::lround(base.getRProperty() * light), 0L, 255L)),
                         static_cast<int>(std::clamp(
                             std::lround(base.getGProperty() * light), 0L, 255L)),
                         static_cast<int>(std::clamp(
                             std::lround(base.getBProperty() * light), 0L, 255L)), 255);
        }

        Color CoatTone(const Color base, const Vector3& normal, const float amount)
        {
            if (amount <= 0.0f) return base;
            const Color target = normal.Y >= 0.0f ? Color(65, 45, 30)
                                                   : Color(220, 190, 145);
            return Color::Lerp(base, target,
                               std::clamp(amount * std::abs(normal.Y), 0.0f, 1.0f));
        }

        void Ellipsoid(std::vector<VertexPositionColor>& out, const Vector3& centre,
                       const Vector3& right, const Vector3& up, const Vector3& forward,
                       const float rx, const float ry, const float rz, const Color color,
                       const float transmission, const int rings = 5, const int sides = 10,
                       const float coatTone = 0.0f)
        {
            const auto point = [&](const int ring, const int side) {
                const float latitude = -1.57079632679f
                    + 3.14159265359f * static_cast<float>(ring) / static_cast<float>(rings);
                const float longitude = 6.28318530718f * static_cast<float>(side)
                    / static_cast<float>(sides);
                return centre + right * (std::cos(latitude) * std::cos(longitude) * rx)
                    + up * (std::sin(latitude) * ry)
                    + forward * (std::cos(latitude) * std::sin(longitude) * rz);
            };
            for (int ring = 0; ring < rings; ++ring) for (int side = 0; side < sides; ++side) {
                const int next = (side + 1) % sides;
                const Vector3 a = point(ring, side), b = point(ring + 1, side);
                const Vector3 c = point(ring + 1, next), d = point(ring, next);
                const auto normal = [&](const int latitudeRing, const int longitudeSide) {
                    const float latitude = -1.57079632679f
                        + 3.14159265359f * static_cast<float>(latitudeRing)
                            / static_cast<float>(rings);
                    const float longitude = 6.28318530718f
                        * static_cast<float>(longitudeSide) / static_cast<float>(sides);
                    Vector3 result = right * (std::cos(latitude) * std::cos(longitude) / rx)
                        + up * (std::sin(latitude) / ry)
                        + forward * (std::cos(latitude) * std::sin(longitude) / rz);
                    if (result.LengthSquared() > 1e-8f) result.Normalize(); else result = up;
                    return result;
                };
                const Vector3 na=normal(ring,side),nb=normal(ring+1,side);
                const Vector3 nc=normal(ring+1,next),nd=normal(ring,next);
                const Color ca=Lit(CoatTone(color,na,coatTone),na,transmission);
                const Color cb=Lit(CoatTone(color,nb,coatTone),nb,transmission);
                const Color cc=Lit(CoatTone(color,nc,coatTone),nc,transmission);
                const Color cd=Lit(CoatTone(color,nd,coatTone),nd,transmission);
                out.emplace_back(a,ca);out.emplace_back(b,cb);out.emplace_back(c,cc);
                out.emplace_back(a,ca);out.emplace_back(c,cc);out.emplace_back(d,cd);
            }
        }

        void Tube(std::vector<VertexPositionColor>& out, const Vector3& a, const Vector3& b,
                  const float radiusA, const float radiusB, const Color color,
                  const float transmission, const int sides = 7)
        {
            Vector3 axis = b - a;
            const float length = axis.Length();
            if (length < .0001f) return;
            axis /= length;
            Vector3 side = Vector3::Cross(axis, std::abs(axis.Y) < .9f
                ? Vector3::Up : Vector3::Right);
            side.Normalize();
            const Vector3 other = Vector3::Cross(axis, side);
            for (int index = 0; index < sides; ++index) {
                const float first = 6.28318530718f * static_cast<float>(index) / sides;
                const float second = 6.28318530718f * static_cast<float>(index + 1) / sides;
                const Vector3 radialA = side * std::cos(first) + other * std::sin(first);
                const Vector3 radialB = side * std::cos(second) + other * std::sin(second);
                const Color shadeA = Lit(color, radialA, transmission);
                const Color shadeB = Lit(color, radialB, transmission);
                const Vector3 p0 = a + radialA * radiusA, p1 = b + radialA * radiusB;
                const Vector3 p2 = b + radialB * radiusB, p3 = a + radialB * radiusA;
                out.emplace_back(p0,shadeA);out.emplace_back(p1,shadeA);out.emplace_back(p2,shadeB);
                out.emplace_back(p0,shadeA);out.emplace_back(p2,shadeB);out.emplace_back(p3,shadeB);
            }
        }

        template<std::size_t RingCount>
        void LoftTube(std::vector<VertexPositionColor>& out,
                      const std::array<Vector3,RingCount>& centres,
                      const std::array<float,RingCount>& radii,
                      Vector3 right,const Color color,const float transmission,
                      const int sides,const float coatTone=0.0f)
        {
            static_assert(RingCount>=2);
            right.Normalize();
            const auto basis=[&](const std::size_t ring) {
                const std::size_t previous=ring==0?0:ring-1;
                const std::size_t next=std::min(ring+1,RingCount-1);
                Vector3 tangent=centres[next]-centres[previous];
                tangent.Normalize();
                Vector3 other=Vector3::Cross(tangent,right);
                other.Normalize();
                return other;
            };
            const auto radial=[&](const std::size_t ring,const int side) {
                const float angle=6.28318530718f*static_cast<float>(side)
                    /static_cast<float>(sides);
                return right*std::cos(angle)+basis(ring)*std::sin(angle);
            };
            for(std::size_t ring=0;ring+1<RingCount;++ring) {
                for(int side=0;side<sides;++side) {
                    const int next=(side+1)%sides;
                    const Vector3 radialA=radial(ring,side);
                    const Vector3 radialB=radial(ring+1,side);
                    const Vector3 radialC=radial(ring+1,next);
                    const Vector3 radialD=radial(ring,next);
                    const Vector3 a=centres[ring]+radialA*radii[ring];
                    const Vector3 b=centres[ring+1]+radialB*radii[ring+1];
                    const Vector3 c=centres[ring+1]+radialC*radii[ring+1];
                    const Vector3 d=centres[ring]+radialD*radii[ring];
                    out.emplace_back(a,Lit(CoatTone(color,radialA,coatTone),radialA,transmission));
                    out.emplace_back(b,Lit(CoatTone(color,radialB,coatTone),radialB,transmission));
                    out.emplace_back(c,Lit(CoatTone(color,radialC,coatTone),radialC,transmission));
                    out.emplace_back(a,Lit(CoatTone(color,radialA,coatTone),radialA,transmission));
                    out.emplace_back(c,Lit(CoatTone(color,radialC,coatTone),radialC,transmission));
                    out.emplace_back(d,Lit(CoatTone(color,radialD,coatTone),radialD,transmission));
                }
            }
        }

        struct BodySection
        {
            float forward;
            float lift;
            float halfWidth;
            float halfHeight;
        };

        constexpr std::array DeerBodySections{
            BodySection{-.72f,.060f,.045f,.050f},
            BodySection{-.62f,.035f,.255f,.275f},
            BodySection{-.43f,.015f,.340f,.345f},
            BodySection{-.15f,.000f,.350f,.340f},
            BodySection{ .15f,.015f,.340f,.345f},
            BodySection{ .40f,.065f,.330f,.370f},
            BodySection{ .58f,.140f,.265f,.295f},
            BodySection{ .68f,.195f,.055f,.065f},
        };

        [[nodiscard]] float BodyHalfWidthAt(const float longitudinal,
                                            const float height) noexcept
        {
            std::size_t upper=1;
            while(upper+1<DeerBodySections.size()
                  && longitudinal>DeerBodySections[upper].forward) ++upper;
            const auto& a=DeerBodySections[upper-1];
            const auto& b=DeerBodySections[upper];
            const float t=std::clamp((longitudinal-a.forward)
                /std::max(b.forward-a.forward,.0001f),0.0f,1.0f);
            const float radius=a.halfWidth+(b.halfWidth-a.halfWidth)*t;
            const float verticalRadius=a.halfHeight+(b.halfHeight-a.halfHeight)*t;
            const float centreHeight=a.lift+(b.lift-a.lift)*t;
            const float normalized=(height-centreHeight)/std::max(verticalRadius,.0001f);
            return radius*std::sqrt(std::max(0.0f,1.0f-normalized*normalized));
        }

        void AppendBody(std::vector<VertexPositionColor>& out,const Vector3& centre,
                        const Vector3& right,const Vector3& up,const Vector3& forward,
                        const float scale,const Color coat,const float transmission,
                        const bool near)
        {
            const int sides=near?20:9;
            const Color shadowCoat=Color::Lerp(coat,Color(72,48,30),.32f);
            const auto point=[&](const std::size_t section,const int side) {
                const auto& profile=DeerBodySections[section];
                const float angle=6.28318530718f*static_cast<float>(side)
                    /static_cast<float>(sides);
                return centre+forward*(profile.forward*scale)+up*(profile.lift*scale)
                    +right*(std::cos(angle)*profile.halfWidth*scale)
                    +up*(std::sin(angle)*profile.halfHeight*scale);
            };
            const auto normal=[&](const std::size_t section,const int side) {
                const auto& profile=DeerBodySections[section];
                const float angle=6.28318530718f*static_cast<float>(side)
                    /static_cast<float>(sides);
                Vector3 result=right*(std::cos(angle)/profile.halfWidth)
                    +up*(std::sin(angle)/profile.halfHeight);
                result.Normalize();
                return result;
            };
            const auto colour=[&](const std::size_t section,const int side,const Vector3& n) {
                const float mottle=.5f+.5f*std::sin(
                    static_cast<float>(section)*2.17f+static_cast<float>(side)*1.31f);
                return Lit(CoatTone(Color::Lerp(coat,shadowCoat,.018f+.035f*mottle),n,.30f),
                           n,transmission);
            };
            for(std::size_t section=0;section+1<DeerBodySections.size();++section) {
                for(int side=0;side<sides;++side) {
                    const int next=(side+1)%sides;
                    const Vector3 a=point(section,side),b=point(section+1,side);
                    const Vector3 c=point(section+1,next),d=point(section,next);
                    const Vector3 na=normal(section,side),nb=normal(section+1,side);
                    const Vector3 nc=normal(section+1,next),nd=normal(section,next);
                    out.emplace_back(a,colour(section,side,na));
                    out.emplace_back(b,colour(section+1,side,nb));
                    out.emplace_back(c,colour(section+1,next,nc));
                    out.emplace_back(a,colour(section,side,na));
                    out.emplace_back(c,colour(section+1,next,nc));
                    out.emplace_back(d,colour(section,next,nd));
                }
            }
            for(const auto& [section,reverse]:{
                    std::pair<std::size_t,bool>{0U,true},
                    std::pair<std::size_t,bool>{DeerBodySections.size()-1U,false}}) {
                const Vector3 capCentre=centre+forward*(DeerBodySections[section].forward*scale)
                    +up*(DeerBodySections[section].lift*scale);
                const Vector3 capNormal=reverse?-forward:forward;
                const Color capColor=Lit(CoatTone(coat,capNormal,.30f),capNormal,transmission);
                for(int side=0;side<sides;++side) {
                    const int next=(side+1)%sides;
                    Triangle(out,capCentre,point(section,reverse?next:side),
                             point(section,reverse?side:next),capColor);
                }
            }
        }

        void AppendDeer(std::vector<VertexPositionColor>& out, const DeerState& deer,
                        const float transmission, const bool near)
        {
            const std::array<Color, 3> coats = {{Color(151, 103, 58), Color(130, 82, 48),
                                                 Color(170, 121, 69)}};
            const Color coat = coats[deer.coatVariant % coats.size()];
            const Color dark = Color::Lerp(coat, Color(42, 31, 24), .58f);
            const Color pale = Color::Lerp(coat, Color(231, 214, 177), .68f);
            const Color eye(24, 17, 11), antler(103, 82, 57), hoof(31, 27, 24);
            const float scale = deer.scale;
            const Vector3 forward(std::cos(deer.headingRadians), 0.0f,
                                  std::sin(deer.headingRadians));
            const Vector3 right(-forward.Z, 0.0f, forward.X), up = Vector3::Up;
            const Vector3 body(deer.x, deer.y + 1.03f * scale, deer.z);
            AppendBody(out,body,right,up,forward,scale,coat,transmission,near);
            const float graze = deer.grazeAmount;
            const Vector3 neckBase = body + forward * (.58f * scale) + up * (.24f * scale);
            const Vector3 neckTop = neckBase + forward * (.27f * scale)
                + up * ((.50f - .62f * graze) * scale);
            const Vector3 neckMid = neckBase + forward * (.12f * scale)
                + up * ((.27f - .31f * graze) * scale);
            const std::array<Vector3,5> neckStations{{
                body+forward*(.48f*scale)+up*(.17f*scale),
                neckBase,
                neckMid,
                Vector3::Lerp(neckMid,neckTop,.56f),
                neckTop}};
            const std::array<float,5> neckRadii{{.245f*scale,.225f*scale,.185f*scale,
                                                 .155f*scale,.132f*scale}};
            LoftTube(out,neckStations,neckRadii,right,coat,transmission,near?10:7,.24f);
            const Vector3 head = neckTop + forward * (.22f * scale)
                + up * ((.045f - .105f * graze) * scale);
            Ellipsoid(out, head, right, up, forward, .155f * scale, .19f * scale,
                      .29f * scale, coat, transmission, near ? 8 : 3, near ? 18 : 7, .22f);
            const Vector3 muzzle = head + forward * (.255f * scale) - up * (.055f * scale);
            Ellipsoid(out, muzzle, right, up, forward, .112f * scale, .098f * scale,
                      .17f * scale, Color::Lerp(coat,dark,.25f), transmission,
                      near ? 6 : 2, near ? 12 : 6);
            const Vector3 nose=muzzle+forward*(.16f*scale)-up*(.010f*scale);
            Ellipsoid(out,nose,right,up,forward,.086f*scale,.058f*scale,.052f*scale,
                      dark,transmission,near?5:2,near?12:6);
            // Long pointed ears establish a deer rather than goat silhouette.
            for (const float sideSign : {-1.0f, 1.0f}) {
                const Vector3 earRoot = head - forward * (.07f * scale)
                    + right * (sideSign * .105f * scale) + up * (.13f * scale);
                const Vector3 earTip = earRoot - forward * (.075f * scale)
                    + right * (sideSign * .16f * scale) + up * (.23f * scale);
                const Vector3 earFront=earRoot+forward*(.105f*scale)
                    +right*(sideSign*.018f*scale);
                const Vector3 earBack=earRoot-forward*(.070f*scale)
                    -right*(sideSign*.014f*scale);
                const Color earOuter=Lit(Color::Lerp(coat,dark,.22f),up,transmission);
                Triangle(out,earBack,earTip,earFront,earOuter);
                Triangle(out,earFront,earTip,earBack,earOuter);
                const Vector3 innerRoot=Vector3::Lerp(earBack,earFront,.52f)+up*(.018f*scale);
                const Vector3 innerTip=Vector3::Lerp(earRoot,earTip,.78f);
                Triangle(out,innerRoot-right*(sideSign*.022f*scale),innerTip,
                         innerRoot+right*(sideSign*.022f*scale),Lit(pale,up,transmission));
                if (near) {
                    const Vector3 eyeCentre = head + forward * (.12f * scale)
                        + right * (sideSign * .148f * scale) + up * (.030f * scale);
                    Ellipsoid(out, eyeCentre, right, up, forward, .022f * scale,
                              .026f * scale, .016f * scale, eye, transmission, 4, 10);
                    const Vector3 glint=eyeCentre+forward*(.010f*scale)
                        +right*(sideSign*.014f*scale)+up*(.008f*scale);
                    Ellipsoid(out,glint,right,up,forward,.005f*scale,.006f*scale,
                              .004f*scale,Color(225,205,145),transmission,3,8);
                }
            }
            // Four articulated legs; diagonal phase offsets make walk and retreat readable.
            for (int leg = 0; leg < 4; ++leg) {
                const float fore = leg < 2 ? .48f : -.47f;
                const float sideSign = leg % 2 == 0 ? -1.0f : 1.0f;
                const float phase = deer.gaitPhase + ((leg == 0 || leg == 3) ? 0.0f : 3.14159265359f);
                const float swing = std::sin(phase) * std::min(1.0f, deer.speed / .8f);
                const Vector3 hip = body + forward * (fore * scale)
                    + right * (sideSign * .255f * scale) - up * (.23f * scale);
                const bool hind = leg >= 2;
                const Vector3 knee = hip + forward * (((hind ? .16f : -.07f)
                    + swing * .13f) * scale) - up * (.37f * scale)
                    + right * (sideSign * .018f * scale);
                const Vector3 hock = knee + forward * (((hind ? -.22f : .08f)
                    - swing * .15f) * scale) - up * (.34f * scale);
                const Vector3 hoofPoint = hock + forward * (((hind ? .18f : .04f)
                    - swing * .08f) * scale);
                const float upperRootRadius = hind ? .142f : .128f;
                const std::array<Vector3,4> upperLeg{{hip,Vector3::Lerp(hip,knee,.28f),
                    Vector3::Lerp(hip,knee,.62f),knee}};
                const std::array<float,4> upperLegRadii{{upperRootRadius*scale,
                    (hind?.132f:.118f)*scale,.091f*scale,.069f*scale}};
                LoftTube(out,upperLeg,upperLegRadii,right,coat,transmission,near?9:7);
                Tube(out, knee, hock, .060f * scale, .043f * scale, dark, transmission,
                     near ? 9 : 7);
                Tube(out, hock, Vector3(hoofPoint.X, deer.y + .060f * scale, hoofPoint.Z),
                     .043f * scale, .031f * scale, dark, transmission, near ? 9 : 7);
                const Vector3 groundedHoof(hoofPoint.X, deer.y + .060f * scale, hoofPoint.Z);
                for(const float toeSide:{-.018f,.018f}) {
                    const Vector3 offset=right*(toeSide*scale);
                    Tube(out,groundedHoof-forward*(.042f*scale)+offset,
                         groundedHoof+forward*(.085f*scale)+offset,
                         .032f*scale,.022f*scale,hoof,transmission,near?8:6);
                }
            }
            const Vector3 tailRoot = body - forward * (.77f * scale) + up * (.17f * scale);
            const Vector3 tailTip = tailRoot - forward * (.31f * scale) + up * (.08f * scale);
            Tube(out, tailRoot, tailTip, .085f * scale, .022f * scale, pale, transmission,
                 near ? 10 : 8);
            if (near) {
                // Fitted oval patches replace the old double-sided triangles, which
                // projected beyond the coat like small pale spikes. Each centre uses
                // the outermost of the overlapping barrel, shoulder and rump ellipsoids.
                constexpr std::array<std::array<float,2>,7> spots{{
                    {{-.36f,.15f}},{{-.13f,.15f}},{{.10f,.15f}},{{.33f,.15f}},
                    {{-.25f,.015f}},{{-.02f,.015f}},{{.21f,.015f}}}};
                for (const auto& spot : spots) for (const float sideSign : {-1.0f, 1.0f}) {
                    const float z=spot[0],y=spot[1];
                    const float halfWidth=BodyHalfWidthAt(z,y)+.004f;
                    const Vector3 centre=body+forward*(z*scale)+up*(y*scale)
                        +right*(sideSign*halfWidth*scale);
                    constexpr int SpotSides=6;
                    const Color spotColor=Color::Lerp(coat,pale,.70f);
                    for(int edge=0;edge<SpotSides;++edge) {
                        const float a=6.28318530718f*static_cast<float>(edge)/SpotSides;
                        const float b=6.28318530718f*static_cast<float>(edge+1)/SpotSides;
                        const Vector3 first=centre+up*(std::sin(a)*.024f*scale)
                            +forward*(std::cos(a)*.034f*scale);
                        const Vector3 second=centre+up*(std::sin(b)*.024f*scale)
                            +forward*(std::cos(b)*.034f*scale);
                        Triangle(out,centre,sideSign>0?first:second,
                                 sideSign>0?second:first,
                                 Lit(spotColor,right*sideSign,transmission));
                    }
                }
            }
            if (near && deer.antlered) {
                for (const float sideSign : {-1.0f, 1.0f}) {
                    const Vector3 base = head - forward * (.12f * scale)
                        + right * (sideSign * .075f * scale) + up * (.18f * scale);
                    const Vector3 lowerBeam=base-forward*(.055f*scale)
                        +right*(sideSign*.045f*scale)+up*(.22f*scale);
                    const Vector3 upperBeam=lowerBeam-forward*(.018f*scale)
                        +right*(sideSign*.040f*scale)+up*(.20f*scale);
                    const Vector3 crown=upperBeam+forward*(.18f*scale)
                        +right*(sideSign*.025f*scale)+up*(.16f*scale);
                    Tube(out,base,lowerBeam,.030f*scale,.024f*scale,antler,transmission,8);
                    Tube(out,lowerBeam,upperBeam,.025f*scale,.018f*scale,antler,transmission,8);
                    Tube(out,upperBeam,crown,.019f*scale,.010f*scale,antler,transmission,8);
                    Tube(out,lowerBeam+forward*(.010f*scale),
                         lowerBeam+forward*(.16f*scale)+up*(.09f*scale),
                         .017f*scale,.008f*scale,antler,transmission,8);
                    Tube(out,upperBeam+forward*(.035f*scale),
                         upperBeam+forward*(.15f*scale)+up*(.12f*scale),
                         .014f*scale,.006f*scale,antler,transmission,8);
                    Tube(out,crown-forward*(.035f*scale),
                         crown+forward*(.015f*scale)+up*(.14f*scale),
                         .011f*scale,.005f*scale,antler,transmission,8);
                }
            }
        }
    }

    DeerRenderer::DeerRenderer(GraphicsDevice& device, const CanopyLightField& canopy,
                               const float maximumDistance)
        : device_(device), canopy_(canopy), maximumDistance_(maximumDistance),
          effect_(std::make_unique<BasicEffect>(device)),
          buffer_(std::make_unique<DynamicVertexBuffer>(device,
              VertexPositionColor::getVertexDeclarationStatic(), Capacity, BufferUsage::WriteOnly))
    {
        staging_.reserve(Capacity);
        effect_->setVertexColorEnabledProperty(true);effect_->setLightingEnabledProperty(false);
        effect_->setFogEnabledProperty(true);
        effect_->setFogColorProperty(Vector3(environment::FogColorRed,
            environment::FogColorGreen, environment::FogColorBlue));
        effect_->setFogStartProperty(maximumDistance_ * .70f);
        effect_->setFogEndProperty(maximumDistance_);
        constexpr auto diagnostic =
            "island-simulator: four inland deer use long legs, pointed ears, spotted coats and one antlered silhouette";
        std::cout << diagnostic << '\n';MirrorRuntimeDiagnosticToPlatformLog(diagnostic);
        constexpr auto anatomyDiagnostic =
            "island-simulator: near deer use vertex-blended ellipsoid light, curved tapered necks, articulated hocks, fitted muzzles, and smooth antler beams, with continuous five-ring necks, separate nasal pads, and surface-fitted oval coat spots";
        std::cout << anatomyDiagnostic << '\n';MirrorRuntimeDiagnosticToPlatformLog(anatomyDiagnostic);
        constexpr auto proportionDiagnostic =
            "island-simulator: deer use separated chest and haunch masses, muscle-profiled upper legs, and normal-driven dorsal-to-belly coat tone";
        std::cout << proportionDiagnostic << '\n';
        MirrorRuntimeDiagnosticToPlatformLog(proportionDiagnostic);
        constexpr auto silhouetteDiagnostic =
            "island-simulator: near deer use a continuous sectioned torso, slender shoulder-grown neck, compact tapered face, leaf-shaped ears, split hooves, and curved multi-tine antlers";
        std::cout << silhouetteDiagnostic << '\n';
        MirrorRuntimeDiagnosticToPlatformLog(silhouetteDiagnostic);
    }

    DeerRenderer::~DeerRenderer() = default;

    void DeerRenderer::Draw(const Matrix& view, const Matrix& projection,
                            const Vector3& camera, const std::span<const DeerState> deer)
    {
        if (deer.size() > DeerHerd::Count) throw std::length_error("Deer render capacity exceeded");
        staging_.clear();visibleCount_ = drawCallCount_ = 0;
        const BoundingFrustum frustum(view * projection);
        for (const DeerState& animal : deer) {
            const Vector3 centre(animal.x, animal.y + 1.1f * animal.scale, animal.z);
            const float distanceSquared = Vector3::DistanceSquared(centre, camera);
            if (distanceSquared > maximumDistance_ * maximumDistance_
                || !frustum.Intersects(BoundingSphere(centre, 2.1f * animal.scale))) continue;
            const float transmission = canopy_.SampleReceiver(
                animal.x, animal.y + 1.0f, animal.z, animal.y);
            AppendDeer(staging_, animal, transmission, distanceSquared < 48.0f * 48.0f);
            ++visibleCount_;
        }
        if (staging_.empty()) return;
        if (staging_.size() > static_cast<std::size_t>(Capacity))
            throw std::length_error("Deer vertex upload exceeds capacity");
        device_.setRasterizerStateProperty(RasterizerState::CullNone);
        device_.setBlendStateProperty(BlendState::Opaque);
        device_.setDepthStencilStateProperty(DepthStencilState::Default);
        buffer_->SetData(staging_.data(), 0, static_cast<int>(staging_.size()),
                         SetDataOptions::Discard);
        device_.SetVertexBuffer(buffer_.get());device_.setIndicesProperty(nullptr);
        effect_->setWorldProperty(Matrix::getIdentityProperty());effect_->setViewProperty(view);
        effect_->setProjectionProperty(projection);effect_->Apply();
        device_.DrawPrimitives(PrimitiveType::TriangleList, 0,
                               static_cast<int>(staging_.size() / 3U));
        drawCallCount_ = 1;
    }
}
