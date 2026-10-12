#pragma once
#include "IslandSimulator/IslandTerrain.hpp"
#include "IslandSimulator/StreamRoute.hpp"
#include <array>
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace island {
struct StreamStation {
    float x,z,distance,ground,water,bed,halfWidth;
};
// Authored route through the existing west valley. Construction inspects the
// uncarved terrain; no recursive terrain queries or whole-island hydrology.
class StreamCourse final {
public:
    static constexpr std::size_t Capacity=161;
    explicit StreamCourse(const IslandTerrain& terrain) {
        float previousWater=100;
        float distance=0;
        for(std::size_t i=0;i<Capacity;++i) {
            const float x=StreamRoute::SourceX-static_cast<float>(i);
            const float z=StreamRoute::CentreZ(x);
            const float stepLength=i==0?0.0f:std::hypot(
                x-stations_[i-1].x,z-stations_[i-1].z);
            distance+=stepLength;
            const float ground=terrain.Height(x,z);
            const float water=std::max(0.0f,std::min(ground-.15f,
                i==0?previousWater:previousWater-.002f*stepLength));
            const float bed=water-.25f;
            if(ground-bed>1.0f) throw std::runtime_error("Stream course requires excessive incision");
            stations_[count_++]={x,z,distance,ground,water,bed,0};
            previousWater=water;
            if(terrain.SignedShoreDistance(x,z)<=0)break;
        }
        if(count_<2 || terrain.SignedShoreDistance(stations_[count_-1].x,stations_[count_-1].z)>0)
            throw std::runtime_error("Stream course does not reach the sea");
        const float length=stations_[count_-1].distance;
        for(std::size_t i=0;i<count_;++i) {
            const float t=stations_[i].distance/length;
            stations_[i].halfWidth=std::lerp(.45f,1.5f,t*t*(3-2*t));
        }
    }
    [[nodiscard]] std::size_t Count() const noexcept {return count_;}
    [[nodiscard]] const StreamStation& Station(std::size_t i) const {return stations_.at(i);}
private:
    std::array<StreamStation,Capacity> stations_{};
    std::size_t count_=0;
};
}
