#include "IslandSimulator/SnakePopulation.hpp"
#include "IslandSimulator/SnakeHeadProfile.hpp"
#include "IslandSimulator/SnakeContactShadow.hpp"
#include "IslandSimulator/SnakeGrassContact.hpp"
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
        if(frame%60==0) {
            const auto contact=island::SnakeGrassContact::Build(population.Snakes());
            Check(contact.count==population.Count,"grass contact lost snake population");
            // Whole-cell rejection must preserve every pressure sample, including soft edges.
            for(const auto& s:population.Snakes()) {
                const auto h=s.body.front();
                for(int cx=-4;cx<=4;++cx) for(int cz=-4;cz<=4;++cz) {
                    const float left=h.x+cx*0.7f, back=h.z+cz*0.7f;
                    const auto mask=contact.CellMask(left,back,left+0.7f,back+0.7f);
                    for(int ix=0;ix<=3;++ix) for(int iz=0;iz<=3;++iz) {
                        const float px=left+0.7f*ix/3, pz=back+0.7f*iz/3;
                        Check(contact.Pressure(px,pz,mask)==contact.Pressure(px,pz),
                            "grass cell mask lost snake contact or soft edge");
                    }
                }
            }
            for(const auto& s:population.Snakes()) for(std::size_t j=1;j<s.body.size();++j) {
                const auto a=s.body[j-1],b=s.body[j];
                for(const float t:{0.0f,0.25f,0.5f,0.75f,1.0f})
                    Check(contact.Pressure(a.x+(b.x-a.x)*t,a.z+(b.z-a.z)*t)>0.999f,
                        "grass contact has a gap along snake body");
            }
            Check(contact.Pressure(299,299)==0,"snake contact flattened distant grass");
            Check(contact.Pressure(std::numeric_limits<float>::quiet_NaN(),0)==0,
                "invalid grass sample did not fail closed");
        }
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
    {
        auto s=before;
        for(std::size_t j=0;j<s.body.size();++j) s.body[j]={static_cast<float>(j)*population.Spacing,2,0};
        const std::array<island::SnakeState,1> fixture{s};
        const auto contact=island::SnakeGrassContact::Build(fixture);
        Check(contact.Pressure(1.1f,0.05f)>0.999f,"snake body pressure insufficient");
        Check(contact.Pressure(1.1f,0.12f)>0 && contact.Pressure(1.1f,0.12f)<1,
            "grass pressure lacks soft edge");
        Check(contact.Pressure(1.1f,0.3f)==0,"grass contact exceeds local footprint");
        Check(contact.CellMask(100,100,101,101)==std::array<float,4>{},
            "distant grass cells still run snake contact loops");
        const auto b=contact.bounds[0];
        Check(contact.CellMask(b[2],b[3],b[2]+1,b[3]+1)[0]==1,
            "touching contact bounds rejected");
        Check(contact.CellMask(b[2]+0.01f,b[3]+0.01f,b[2]+1,b[3]+1)[0]==0,
            "separated contact bounds retained");
        for(const auto query:std::array<std::array<float,4>,3>{{{1,0,0,1},{0,1,1,0},
            {0,0,std::numeric_limits<float>::infinity(),1}}}) {
            bool invalid=false;
            try { (void)contact.CellMask(query[0],query[1],query[2],query[3]); }
            catch(const std::invalid_argument&) { invalid=true; }
            Check(invalid,"invalid cell bounds accepted");
        }
        Check(island::SnakeGrassContact::Build({}).Pressure(0,0)==0,"empty contact flattened grass");
        bool rejected=false;
        auto bad=fixture; bad[0].body[0].z=std::numeric_limits<float>::quiet_NaN();
        try { (void)island::SnakeGrassContact::Build(bad); } catch(const std::invalid_argument&) { rejected=true; }
        Check(rejected,"invalid snake uploaded to grass shader");
    }
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
