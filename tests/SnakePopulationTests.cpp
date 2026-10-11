#include "IslandSimulator/SnakePopulation.hpp"
#include "IslandSimulator/SnakeHeadProfile.hpp"
#include "IslandSimulator/SnakeContactShadow.hpp"
#include "IslandSimulator/VegetationLayout.hpp"
#include "IslandSimulator/JungleLayout.hpp"
#include "IslandSimulator/NaturalPropLayout.hpp"
#include <cmath>
#include <algorithm>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>

static void Check(bool pass, const char* message) { if (!pass) throw std::runtime_error(message); }
int main()
{
    for(const auto& section:island::SnakeHeadSections) {
        Check(section.forward>=0 && section.width>0 && section.height>0,
            "invalid snake muzzle section");
        Check(std::hypot(section.forward,section.width)<=island::SnakePopulation::MaximumRadius,
            "muzzle exceeds horizontal collision envelope");
        Check(section.centreY-section.height>=-island::SnakePopulation::RadiusAt(0),
            "muzzle lower surface falls below supported head ring");
        for (int angle=0;angle<360;++angle) {
            const float theta=static_cast<float>(angle)*0.01745329252f;
            Check(std::hypot(section.forward,std::hypot(section.width*std::cos(theta),
                section.centreY+section.height*std::sin(theta)))+0.0003f
                <=island::SnakePopulation::MaximumRadius,
                "terrain-rotated muzzle exceeds spherical clearance envelope");
        }
    }
    Check(island::SnakeHeadSections.front().width==island::SnakePopulation::RadiusAt(0)
        && island::SnakeNoseForward<=island::SnakePopulation::MaximumRadius,
        "snake head does not join body or nose exceeds clearance");
    Check(std::hypot(island::SnakeEyeForward,std::hypot(island::SnakeEyeSide,island::SnakeEyeHeight))+island::SnakeEyeRadius
        <=island::SnakePopulation::MaximumRadius,"eyes exceed head horizontal clearance");
    const island::IslandTerrain terrain;
    const island::VegetationLayout vegetation(terrain);
    const island::JungleLayout jungle(terrain);
    const island::NaturalPropLayout props(terrain,vegetation,jungle);
    std::vector<island::CircularObstacle> obstacles(vegetation.Obstacles().begin(),vegetation.Obstacles().end());
    obstacles.insert(obstacles.end(),jungle.Obstacles().begin(),jungle.Obstacles().end());
    obstacles.insert(obstacles.end(),props.Obstacles().begin(),props.Obstacles().end());
    island::SnakePopulation population(terrain,obstacles), repeated(terrain,obstacles);
    for (std::size_t i=0; i<island::SnakeState::StationCount+2; ++i) {
        const float radius = island::SnakePopulation::RadiusAt(i);
        Check(std::isfinite(radius) && radius>=0.004f && radius<=island::SnakePopulation::MaximumRadius,
            "snake radius is nonfinite or outside tapered profile");
    }
    const island::RenderedTerrainSurface surface(terrain);
    std::array<bool,3> behaviors{};
    float totalTravel=0.0f;
    int propagatedWaveFrames=0;
    for (int frame=0; frame<2400; ++frame) {
        const auto old = population.Snakes().front();
        const auto head = old.body.front();
        const bool threat = frame%600 < 90;
        const float x = head.x+(threat ? 3.0f : 25.0f), z = head.z;
        const float dt = frame%200==0 ? 5.0f : 1.0f/30.0f;
        population.Update(dt,x,head.y+1.7f,z);
        repeated.Update(dt,x,head.y+1.7f,z);
        for (std::size_t index=0; index<population.Count; ++index) {
            const auto& snake = population.Snakes()[index];
            const auto& other = repeated.Snakes()[index];
            Check(population.ValidBody(snake),"snake left whole-body habitat/spacing/clearance bounds");
            Check(snake.speed>=0 && snake.speed<=1.151f,"snake speed not bounded");
            if(frame%60==0) {
                const auto shadow=island::BuildSnakeContactShadow(snake,surface);
                Check(!shadow.empty() && shadow.size()%3==0 && shadow.size()<=island::SnakeShadowVertexLimit,
                    "snake shadow geometry budget/triangles invalid");
                for(const auto& v:shadow) {
                    Check(std::isfinite(v.opacity) && v.opacity>=0 && v.opacity<=0.230001f,
                        "snake shadow opacity invalid");
                    Check(std::abs(v.y-surface.Sample(v.x,v.z).height-island::SnakeShadowSurfaceOffset)<0.00002f,
                        "snake shadow vertex not on rendered terrain");
                }
                for(std::size_t j=0;j<shadow.size();j+=3) {
                    const auto a=shadow[j],b=shadow[j+1],c=shadow[j+2];
                    for(const auto weights:{std::array{0.2f,0.3f,0.5f},std::array{0.6f,0.2f,0.2f}}) {
                        const float x0=a.x*weights[0]+b.x*weights[1]+c.x*weights[2];
                        const float z0=a.z*weights[0]+b.z*weights[1]+c.z*weights[2];
                        const float y0=a.y*weights[0]+b.y*weights[1]+c.y*weights[2];
                        Check(std::abs(y0-surface.Sample(x0,z0).height-island::SnakeShadowSurfaceOffset)<0.00003f,
                            "snake shadow triangle bridges rendered terrain crease");
                    }
                }
            }
            for (std::size_t j=0; j<snake.body.size(); ++j) {
                const auto a=snake.body[j], b=other.body[j];
                Check(a.x==b.x && a.y==b.y && a.z==b.z,"snake trajectory not deterministic");
                if (j>0) for (const float t : {0.25f,0.5f,0.75f}) {
                    const auto p=snake.body[j-1];
                    const float x0=p.x+(a.x-p.x)*t, z0=p.z+(a.z-p.z)*t;
                    const float radius=population.RadiusAt(j-1)*(1-t)+population.RadiusAt(j)*t;
                    Check(p.y+(a.y-p.y)*t-radius >= surface.Sample(x0,z0).height-0.0001f,
                        "snake segment underside crosses rendered terrain");
                }
                if (j>0 && frame%60==0) {
                    const auto p=snake.body[j-1];
                    const float dx=a.x-p.x, dz=a.z-p.z, length2=dx*dx+dz*dz;
                    for (const auto& obstacle : obstacles) {
                        const float t=std::clamp(((obstacle.x-p.x)*dx+(obstacle.z-p.z)*dz)/length2,0.0f,1.0f);
                        const float ox=p.x+t*dx-obstacle.x, oz=p.z+t*dz-obstacle.z;
                        const float radius=obstacle.radius+island::SnakePopulation::MaximumRadius;
                        Check(ox*ox+oz*oz>=radius*radius-0.00001f,
                            "independent segment oracle found obstacle penetration");
                    }
                }
            }
        }
        const auto& snake = population.Snakes().front();
        float leftTurn=0.0f, rightTurn=0.0f;
        for (std::size_t j=1;j+1<snake.body.size();++j) {
            const auto a=snake.body[j-1], b=snake.body[j], c=snake.body[j+1];
            const float ax=b.x-a.x, az=b.z-a.z, bx=c.x-b.x, bz=c.z-b.z;
            const float turn=std::atan2(ax*bz-az*bx,ax*bx+az*bz);
            if(j>=4) { leftTurn+=std::max(0.0f,turn); rightTurn+=std::max(0.0f,-turn); }
        }
        if(leftTurn>1.0f && rightTurn>1.0f) ++propagatedWaveFrames;
        for(std::size_t j=0;j<snake.body.size();++j)
            Check(std::hypot(snake.body[j].x-old.body[j].x,snake.body[j].z-old.body[j].z)
                <=1.151f*std::min(dt,0.1f)+0.001f,"snake body station teleported");
        behaviors[static_cast<std::size_t>(snake.behavior)]=true;
        const float travel = std::hypot(snake.body[0].x-head.x,snake.body[0].z-head.z);
        totalTravel+=travel;
        Check(travel<=1.151f*std::min(dt,0.1f)+0.0001f,"snake head teleported");
        if (threat) Check(std::hypot(snake.body[0].x-x,snake.body[0].z-z)>=2.9999f,
            "threatened snake advanced toward observer");
    }
    Check(totalTravel>3.0f && behaviors[0] && behaviors[1] && behaviors[2],
        "snake behavior/movement coverage missing");
    Check(propagatedWaveFrames>10,"opposing body waves did not propagate beyond the neck");
    // Sub-millimetre updates must accumulate into the bounded trail, not append
    // unbounded tiny samples or lose the body's path as old samples are pruned.
    island::SnakePopulation fineSteps(terrain), coarseSteps(terrain);
    for(int frame=0;frame<2000;++frame) {
        const auto h=fineSteps.Snakes().front().body.front();
        fineSteps.Update(0.001f,h.x-3.0f,h.y+1.7f,h.z);
        if(frame%10==0) {
            const auto c=coarseSteps.Snakes().front().body.front();
            coarseSteps.Update(0.01f,c.x-3.0f,c.y+1.7f,c.z);
        }
        Check(fineSteps.ValidBody(fineSteps.Snakes().front()),"fine-step trail lost body validity");
    }
    for(std::size_t j=0;j<island::SnakeState::StationCount;++j) {
        const auto a=fineSteps.Snakes().front().body[j], b=coarseSteps.Snakes().front().body[j];
        Check(std::hypot(a.x-b.x,a.z-b.z)<0.025f,"trail sampling depends excessively on update step");
    }
    const auto before=population.Snakes().front();
    for(int invalid=0;invalid<3;++invalid) {
        auto snake=before;
        if(invalid==0) snake.body[0].x=std::numeric_limits<float>::quiet_NaN();
        if(invalid==1) snake.body[2]=snake.body[0];
        if(invalid==2) snake.body[0].x+=2.0f;
        bool rejected=false;
        try { (void)island::BuildSnakeContactShadow(snake,surface); }
        catch(const std::invalid_argument&) { rejected=true; }
        Check(rejected,"invalid snake shadow geometry accepted");
    }
    for (float dt : {0.0f,-1.0f,std::numeric_limits<float>::quiet_NaN()})
        population.Update(dt,0,0,0);
    Check(before.body[0].x==population.Snakes()[0].body[0].x,"invalid time moved snake");
    population.Update(0.1f,0,10000,0);
    Check(population.Snakes()[0].tier==island::WildlifeSimulationTier::Dormant
        && population.Snakes()[0].body[0].x==before.body[0].x,"far snake was not dormant");
    population.Update(0.1f,std::numeric_limits<float>::quiet_NaN(),0,0);
    Check(population.Snakes()[0].tier==island::WildlifeSimulationTier::Dormant,
        "invalid observer activated snake");
    bool refused=false;
    try {
        const std::array<island::CircularObstacle,1> blocked{{{0,0,1000}}};
        island::SnakePopulation impossible(terrain,blocked);
    } catch(const std::runtime_error&) { refused=true; }
    Check(refused,"impossible whole-body habitat was accepted");
    std::cout << "Snake population deterministic retreat/body/terrain/obstacle tests passed\n";
}
