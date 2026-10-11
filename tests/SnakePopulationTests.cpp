#include "IslandSimulator/SnakePopulation.hpp"
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
        behaviors[static_cast<std::size_t>(snake.behavior)]=true;
        const float travel = std::hypot(snake.body[0].x-head.x,snake.body[0].z-head.z);
        totalTravel+=travel;
        Check(travel<=1.151f*std::min(dt,0.1f)+0.0001f,"snake head teleported");
        if (threat) Check(std::hypot(snake.body[0].x-x,snake.body[0].z-z)>=2.9999f,
            "threatened snake advanced toward observer");
    }
    Check(totalTravel>3.0f && behaviors[0] && behaviors[1] && behaviors[2],
        "snake behavior/movement coverage missing");
    const auto before=population.Snakes().front();
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
