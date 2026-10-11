#pragma once
#include "IslandSimulator/PlayerMotion.hpp"
#include "IslandSimulator/WorldCollisionGrid.hpp"
#include "IslandSimulator/HutLayout.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <functional>
#include <queue>
#include <stdexcept>
#include <vector>

// Test-only conservative route search. Acceptance is continuous PlayerMotion replay,
// not a claim that a coarse occupancy grid by itself proves traversability.
inline void CheckHutReachability(const island::IslandTerrain& terrain,
    const island::HutPlacement& hut, std::vector<island::CircularObstacle> solids)
{
    solids.push_back({hut.x,hut.z,island::HutLayout::CollisionRadius});
    const island::WorldCollisionGrid grid(solids);
    constexpr int width=301,origin=150,count=width*width;
    const auto coordinate=[](int cell) { return static_cast<float>((cell-origin)*2); };
    const auto index=[](float x,float z) {
        return (static_cast<int>(std::lround(z/2))+origin)*width
            +static_cast<int>(std::lround(x/2))+origin;
    };
    const int start=index(0,-224);
    const int goal=index(hut.x+std::sin(hut.yawRadians)*6,
                         hut.z+std::cos(hut.yawRadians)*6);
    if(goal<0||goal>=count) throw std::runtime_error("hut route target outside bounded search");
    std::vector<signed char> walkable(count,-1);
    const auto allowed=[&](int node) {
        auto& cache=walkable[static_cast<std::size_t>(node)];
        if(cache>=0) return cache==1;
        const float x=coordinate(node%width),z=coordinate(node/width);
        bool clear=!grid.IntersectsCircle(x,z,island::PlayerMotion::CapsuleRadius+1.05f);
        // Sample a larger footprint than the player to cover the 2m grid edges.
        for(const auto& offset:island::PlayerMotion::GroundContactOffsets) {
            const auto sample=terrain.Sample(x+offset.x*1.4f,z+offset.z*1.4f);
            clear=clear&&sample.height>0.8f&&sample.slopeRadians<0.60f;
        }
        cache=clear?1:0;
        return clear;
    };
    if(!allowed(start)||!allowed(goal)) throw std::runtime_error("hut route endpoints blocked");
    const auto heuristic=[&](int n) { return std::abs(n%width-goal%width)+std::abs(n/width-goal/width); };
    using Entry=std::pair<int,int>;
    std::priority_queue<Entry,std::vector<Entry>,std::greater<Entry>> open;
    std::vector<int> cost(count,count),parent(count,-1);
    cost[start]=0;open.emplace(heuristic(start),start);
    while(!open.empty()) {
        const auto [score,node]=open.top();open.pop();
        if(score!=cost[node]+heuristic(node)) continue;
        if(node==goal) break;
        for(const int delta:{-width,width,-1,1}) {
            const int next=node+delta;
            if(next<0||next>=count||std::abs(next%width-node%width)+std::abs(next/width-node/width)!=1
                ||cost[next]<=cost[node]+1||!allowed(next)) continue;
            cost[next]=cost[node]+1;parent[next]=node;
            open.emplace(cost[next]+heuristic(next),next);
        }
    }
    if(parent[goal]<0) throw std::runtime_error("no dry land route from spawn to hut");
    std::vector<int> route;
    for(int n=goal;n>=0;n=parent[n]) route.push_back(n);
    std::reverse(route.begin(),route.end());
    island::PlayerMotion player;
    player.SetPosition(terrain,0,-225); // Actual normal game spawn, not a QA teleport.
    float travelled=0;
    int frames=0;
    for(const int node:route) {
        const float x=coordinate(node%width),z=coordinate(node/width);
        int step=0;
        while(std::hypot(player.Position().x-x,player.Position().z-z)>0.12f&&step++<240) {
            const auto before=player.Position();
            const float dx=x-before.x,dz=z-before.z,length=std::hypot(dx,dz);
            player.Update(terrain,dx/length,dz/length,1.0f/60,grid);
            const auto& p=player.Position();
            const float distance=std::hypot(p.x-before.x,p.z-before.z);
            travelled+=distance;++frames;
            if(player.Mode()!=island::LocomotionMode::Walking
                ||grid.IntersectsCircle(p.x,p.z,island::PlayerMotion::CapsuleRadius-0.002f)
                ||!std::isfinite(p.x)||!std::isfinite(p.y)||!std::isfinite(p.z)
                ||distance>island::PlayerMotion::WalkSpeed/60+0.001f
                ||player.MovementTrace().obstacleDepenetrations!=0||player.MovementTrace().safetyClamped)
                throw std::runtime_error("hut route replay swam, penetrated or used safety clamp");
        }
        if(step>=240) throw std::runtime_error("hut route replay stalled before a waypoint");
    }
    if(std::hypot(player.Position().x-hut.x,player.Position().z-hut.z)>7.5f)
        throw std::runtime_error("hut route did not reach the front approach");
    std::cout<<"Hut dry walking route from spawn: "<<travelled<<"m, "<<frames
             <<" continuous player steps, "<<route.size()<<" waypoints\n";
}
