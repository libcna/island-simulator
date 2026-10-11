#include "IslandSimulator/FlightMotion.hpp"
#include "IslandSimulator/WorldCollisionGrid.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

namespace island
{
    namespace
    {
        constexpr float Skin = .001f;
        float Finite(float value) { return std::isfinite(value) ? value : 0.0f; }
        PlayerPosition BoundedHorizontal(PlayerPosition point)
        {
            point.x = Finite(point.x); point.z = Finite(point.z);
            const double radius = std::hypot(double(point.x),double(point.z));
            if (radius > FlightMotion::Radius) {
                point.x = static_cast<float>(point.x * (FlightMotion::Radius / radius));
                point.z = static_cast<float>(point.z * (FlightMotion::Radius / radius));
            }
            return point;
        }
    }

    void FlightMotion::Activate(const IslandTerrain& terrain, PlayerPosition point)
    {
        if (!surface_) surface_ = std::make_unique<RenderedTerrainSurface>(terrain);
        point = BoundedHorizontal(point);
        const float ground = surface_->Sample(point.x,point.z).height;
        point.y = std::clamp(Finite(point.y),std::max(0.0f,ground)+Clearance+Skin,
                             ground+MaximumHeight-Skin);
        position_ = point;
        active_ = true;
    }

    void FlightMotion::Update(float x, float y, float z, bool fast, float seconds)
    {
        if (!active_) return;
        x=Finite(x); y=Finite(y); z=Finite(z);
        // Normalize after yaw rotation without per-axis clipping, which would bend
        // diagonal directions. Double intermediates also handle large finite input.
        const double norm=std::max(1.0,std::hypot(double(y),std::hypot(double(x),double(z))));
        const float distance=Speed*(fast ? 3.0f : 1.0f)*std::clamp(Finite(seconds),0.0f,.1f);
        const auto start=position_;
        const auto end=BoundedHorizontal({start.x+static_cast<float>(x/norm)*distance,
            start.y+static_cast<float>(y/norm)*distance,start.z+static_cast<float>(z/norm)*distance});
        const auto point=[&](float t) {
            return PlayerPosition{std::lerp(start.x,end.x,t),std::lerp(start.y,end.y,t),std::lerp(start.z,end.z,t)};
        };
        // Terrain is piecewise affine. Trace every grid edge and triangle diagonal,
        // not just endpoints or fixed-distance samples that could miss a ridge.
        std::array<float,32> knots{};
        std::size_t count=2;
        knots[0]=0; knots[1]=1;
        constexpr double grid=2.0*TerrainMeshLayout::Extent/TerrainMeshLayout::QuadsPerAxis;
        const auto crossings=[&](double a,double b) {
            if (a==b) return;
            const int first=static_cast<int>(std::floor(std::min(a,b)/grid))+1;
            const int last=static_cast<int>(std::floor(std::max(a,b)/grid));
            for (int edge=first;edge<=last;++edge) {
                const float t=static_cast<float>((edge*grid-a)/(b-a));
                if (t>0 && t<1) knots.at(count++)=t;
            }
        };
        crossings(double(start.x)+TerrainMeshLayout::Extent,double(end.x)+TerrainMeshLayout::Extent);
        crossings(double(start.z)+TerrainMeshLayout::Extent,double(end.z)+TerrainMeshLayout::Extent);
        crossings(double(start.x)+start.z+2*TerrainMeshLayout::Extent,double(end.x)+end.z+2*TerrainMeshLayout::Extent);
        std::sort(knots.begin(),knots.begin()+count);
        const auto gaps=[&](PlayerPosition p) {
            const float ground=surface_->Sample(p.x,p.z).height;
            return std::array<float,3>{p.y-ground-Clearance-Skin,p.y-Clearance-Skin,
                                      ground+MaximumHeight-Skin-p.y};
        };
        auto previous=gaps(start);
        float previousT=0;
        for (std::size_t i=1;i<count;++i) {
            const float t=knots[i];
            const auto current=gaps(point(t));
            float stop=t;
            bool blocked=false;
            for (std::size_t limit=0;limit<current.size();++limit) {
                if (current[limit]<0) {
                    const float a=std::max(0.0f,previous[limit]);
                    const float fraction=a/(a-current[limit]);
                    stop=std::min(stop,std::lerp(previousT,t,fraction));
                    blocked=true;
                }
            }
            if (blocked) {
                position_=point(std::max(0.0f,stop-.00001f));
                return;
            }
            previous=current; previousT=t;
        }
        position_=end;
    }

    std::optional<PlayerPosition> FlightMotion::FindLanding(
        const IslandTerrain& terrain,const WorldCollisionGrid& obstacles) const
    {
        if (!surface_ || !active_) return {};
        std::optional<PlayerPosition> best;
        float bestDistance=std::numeric_limits<float>::infinity();
        const auto candidate=[&](float x,float z) {
            const float distance=std::hypot(x-position_.x,z-position_.z);
            if (distance>=bestDistance || std::hypot(x,z)>Radius || terrain.Height(x,z)<.1f) return;
            PlayerMotion landed;
            landed.SetPosition(terrain,x,z);
            landed.Update(terrain,0,0,0,obstacles);
            const auto p=landed.Position();
            if (landed.Mode()!=LocomotionMode::Walking
                || landed.GroundSlopeRadians()>PlayerMotion::MaximumWalkSlopeRadians
                || std::hypot(p.x,p.z)>Radius || terrain.Height(p.x,p.z)<.1f
                || p.y<surface_->Sample(p.x,p.z).height+.2f) return;
            if (obstacles.IntersectsCircle(p.x,p.z,PlayerMotion::CapsuleRadius)) return;
            const float resolvedDistance=std::hypot(p.x-position_.x,p.z-position_.z);
            if (resolvedDistance<bestDistance) { best=p; bestDistance=resolvedDistance; }
        };
        candidate(position_.x,position_.z);
        if (best && bestDistance<.01f) return best;
        // Bounded deterministic dry-land fallback (including launches from water).
        // Choose the nearest valid grid candidate, not an offshore swimming placement.
        for (int z=-330;z<=330;z+=10)
            for (int x=-330;x<=330;x+=10) candidate(static_cast<float>(x),static_cast<float>(z));
        return best;
    }
}
