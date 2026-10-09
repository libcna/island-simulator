#include "IslandSimulator/GrassMotionBounds.hpp"
#include "IslandSimulator/GrassBladeProfile.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <limits>
#include <locale>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace
{
    using Vector = std::array<double,3>;
    Vector Unit(Vector v)
    {
        const double length=std::hypot(v[0],v[1],v[2]);
        for (auto& value:v) value/=length;
        return v;
    }
    Vector Cross(const Vector& a,const Vector& b)
    {
        return {a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]};
    }
    struct Comma final : std::numpunct<char> { char do_decimal_point() const override { return ','; } };
}

int TestGrassMotionBounds()
{
    using namespace island::GrassMotion;
    int failures=0;
    const auto check=[&](bool value,std::string_view message) {
        if (!value) { ++failures; std::cerr << "FAIL: " << message << '\n'; }
    };
    std::size_t samples=0,oldBoxEscapes=0;
    // Independently transform a cylinder containing every LOD. Sample the full displacement
    // superset (even combinations more extreme than the actual height-dependent shader).
    for (float scale:{0.4176f,0.72f,1.24f}) for (float spread:{0.88f,1.12f}) {
        for (double slope:{0.0,0.25,0.4886922,1.55}) for (double azimuth:{0.0,1.1,2.5,4.2}) {
            const float nx=static_cast<float>(std::sin(slope)*std::cos(azimuth));
            const float nz=static_cast<float>(std::sin(slope)*std::sin(azimuth));
            const auto bounds=CalculateBounds(1.2f,0.75f,scale,spread,nx,nz);
            const Vector n=Unit({nx,std::sqrt(std::max(double(NormalFloor),1.0-double(nx)*nx-double(nz)*nz)),nz});
            const Vector tx=Unit({n[1],-n[0],0});
            const Vector tz=Unit(Cross(tx,n));
            for (int angle=0; angle<16; ++angle) for (double height:{0.0,0.75}) {
                const double a=angle*0.392699081698724;
                for (float stature:{MinimumStature,MaximumStature}) for (int wave:{-1,0,1}) {
                    for (int push=0; push<4; ++push) for (double lowering:{0.0,double(MaximumLowering)}) {
                        Vector world{};
                        for (int axis=0; axis<3; ++axis) {
                            world[axis]=tx[axis]*std::cos(a)*1.2*spread
                                +tz[axis]*std::sin(a)*1.2*spread+n[axis]*height*scale*stature;
                        }
                        const double pushAngle=push*1.570796326794897;
                        world[0]+=std::cos(pushAngle)*MaximumPush*scale;
                        world[2]+=std::sin(pushAngle)*MaximumPush*scale;
                        world[1]-=lowering*scale;
                        const double sway=wave*(double(BroadWind)+DetailWind)*scale*MaximumWindGain;
                        world[0]+=sway;
                        world[2]+=sway*WindZ;
                        world[1]-=std::abs(sway)*WindLowering;
                        check(std::abs(world[0])<=bounds.horizontalRadius
                              && std::abs(world[2])<=bounds.horizontalRadius
                              && world[1]>=bounds.minimumY && world[1]<=bounds.maximumY,
                              "animated grass displacement is contained by derived bounds");
                        oldBoxEscapes+=std::abs(world[0])>1.4 || std::abs(world[2])>1.4
                            || world[1]<-0.08 || world[1]>1.35;
                        ++samples;
                    }
                }
            }
            // The unlit static fallback has no slope/stature/wind; its box must also fit.
            check(bounds.horizontalRadius>=1.2f*spread && bounds.minimumY<=0
                  && bounds.maximumY>=0.75f*scale,"derived bounds also contain the stock fallback");
        }
    }
    check(oldBoxEscapes>0,"regression sampling exposes the old fixed-box under-bound");
    // A concrete real tuft root on an allowed 28-degree slope already violates old minimum Y.
    const auto root=island::NearGrassBladePlacement(1);
    const double radius=std::hypot(root.x,root.z);
    const double rootY=-radius*1.12*std::sin(0.4886922);
    const auto rootBounds=CalculateBounds(1.2f,0.75f,1.24f,1.12f,std::sin(0.4886922f),0);
    check(rootY < -0.08 && rootY >= rootBounds.minimumY,
          "a yaw-aligned actual tuft root is outside the old box but inside the new box");

    for (int invalid=0; invalid<8; ++invalid) {
        float r=1.2f,h=0.75f,scale=1,spread=1,nx=0,nz=0;
        if (invalid==0) r=-1;
        if (invalid==1) h=-1;
        if (invalid==2) scale=0;
        if (invalid==3) spread=0;
        if (invalid==4) nx=1.1f;
        if (invalid==5) nz=std::numeric_limits<float>::quiet_NaN();
        if (invalid==6) h=std::numeric_limits<float>::infinity();
        if (invalid==7) r=spread=std::numeric_limits<float>::max();
        bool rejected=false;
        try { (void)CalculateBounds(r,h,scale,spread,nx,nz); }
        catch (const std::invalid_argument&) { rejected=true; }
        check(rejected,"invalid or overflowing grass bounds are rejected");
    }
    const std::string shader=ShaderConstants();
    const auto previous=std::locale();
    std::locale::global(std::locale(previous,new Comma));
    const std::string localized=ShaderConstants();
    std::locale::global(previous);
    check(shader==localized && shader.find(',')==std::string::npos,"grass GLSL constants ignore decimal locale");
    std::istringstream lines(shader);
    lines.imbue(std::locale::classic());
    std::string line;
    int count=0;
    const std::map<std::string,float> expected{
        {"grassMinimumStature",MinimumStature},{"grassMaximumStature",MaximumStature},
        {"grassHeightExponent",HeightExponent},{"grassMinimumPush",MinimumPush},
        {"grassMaximumPush",MaximumPush},{"grassMinimumLowering",MinimumLowering},
        {"grassMaximumLowering",MaximumLowering},{"grassBroadWind",BroadWind},
        {"grassDetailWind",DetailWind},{"grassMinimumWindGain",MinimumWindGain},
        {"grassMaximumWindGain",MaximumWindGain},{"grassWindZ",WindZ},
        {"grassWindLowering",WindLowering},{"grassNormalFloor",NormalFloor}};
    std::map<std::string,float> observed;
    while (std::getline(lines,line)) {
        std::istringstream input(line);
        input.imbue(std::locale::classic());
        std::string qualifier,type,name,equal;
        float value=0;
        input>>qualifier>>type>>name>>equal>>value;
        check(qualifier=="const" && type=="float" && equal=="=" && std::isfinite(value),
              "grass shader tuning uses finite float constants");
        check(observed.emplace(name,value).second,"grass shader constants have unique names");
        ++count;
    }
    check(count==14 && observed==expected,"all shader motion constants round-trip to CPU bounds tuning");
    std::cout << "Grass motion bounds: " << samples << " transformed states, "
              << oldBoxEscapes << " old-box escapes\n";
    return failures;
}
