#include "IslandSimulator/SnakePopulation.hpp"
#include "IslandSimulator/CanopyProfile.hpp"
#include "IslandSimulator/CaveSite.hpp"
#include "IslandSimulator/JungleLayout.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <numbers>
#include <stdexcept>
#include <vector>

namespace island
{
    namespace
    {
        std::uint32_t Mix(std::uint32_t value) noexcept
        {
            value ^= value >> 16U; value *= 0x7feb352dU;
            value ^= value >> 15U; value *= 0x846ca68bU;
            return value ^ (value >> 16U);
        }
    }

    float SnakePopulation::RadiusAt(const std::size_t station) noexcept
    {
        // The same 41 authored radii are used in every body/body clearance pair.
        // Compute them once instead of thousands of sin/pow calls per animal tick.
        static const auto radii = [] {
            std::array<float, SnakeState::StationCount> result{};
            // A short full neck removes the pinched first-body segment that made the
            // close head look detached. The remaining authored taper is unchanged.
            result[0] = 0.040f;
            for (std::size_t i = 1; i < result.size(); ++i) {
                const float t = static_cast<float>(i) / static_cast<float>(result.size()-1);
                result[i] = 0.004f + 0.044f * std::pow(std::max(0.0f,
                    std::sin(std::numbers::pi_v<float> * t)), 0.45f);
            }
            result[1] = 0.035f;
            result[2] = 0.039f;
            result[3] = 0.042f;
            result[4] = 0.044f;
            return result;
        }();
        return radii[std::min(station, radii.size()-1)];
    }

    bool SnakePopulation::ValidPoint(const float x, const float z, const float radius) const
    {
        if (!std::isfinite(x) || !std::isfinite(z) || std::abs(x) > 300.0f || std::abs(z) > 300.0f
            || CaveSite::Reserves(x,z,radius))
            return false;
        const auto ground = surface_->Sample(x,z);
        return terrain_.SignedShoreDistance(x,z) >= 20.0f && ground.height >= 1.0f
            && ground.normal.y >= 0.913089f && !obstacles_.IntersectsCircle(x,z,radius);
    }

    bool SnakePopulation::IsUnderstoryMargin(
        const JungleLayout& jungle, const float x, const float z) noexcept
    {
        if (!std::isfinite(x) || !std::isfinite(z)) return false;
        float nearestSquared = 1000000.0f;
        int neighbours = 0;
        for (const auto& plant : jungle.Understory()) {
            const float dx = x - plant.x, dz = z - plant.z;
            const float distanceSquared = dx * dx + dz * dz;
            nearestSquared = std::min(nearestSquared, distanceSquared);
            if (distanceSquared <= 100.0f) ++neighbours;
        }
        for (const auto& bush : jungle.Bushes()) {
            const float dx = x - bush.x, dz = z - bush.z;
            if (dx * dx + dz * dz <= 64.0f) ++neighbours;
        }
        // Close enough to read as an understory edge, but not centred on a plant stem.
        return nearestSquared >= 1.20f * 1.20f
            && nearestSquared <= 5.25f * 5.25f && neighbours >= 4;
    }

    float SnakePopulation::GroundedY(const float x, const float z, const float radius) const
    {
        const float height = std::max({surface_->Sample(x,z).height,
            surface_->Sample(x-radius,z).height, surface_->Sample(x+radius,z).height,
            surface_->Sample(x,z-radius).height, surface_->Sample(x,z+radius).height});
        return height + radius + 0.004f;
    }

    bool SnakePopulation::ValidBody(const SnakeState& state) const
    {
        for (std::size_t i = 0; i < state.body.size(); ++i) {
            const auto& p = state.body[i];
            if (!std::isfinite(p.y) || !ValidPoint(p.x,p.z,MaximumRadius)
                || std::hypot(p.x-state.homeX,p.z-state.homeZ) > HomeRadius
                || p.y < surface_->Sample(p.x,p.z).height + RadiusAt(i)) return false;
            if (i != 0) {
                const auto& q = state.body[i-1];
                const float length = std::hypot(p.x-q.x,p.z-q.z);
                // A containing disc proves clearance of the whole short horizontal segment.
                if (std::abs(length-Spacing) > Spacing * 0.03f
                    || !ValidPoint((p.x+q.x)*0.5f,(p.z+q.z)*0.5f,MaximumRadius+length*0.5f))
                    return false;
            }
            for (std::size_t j=i+4; j<state.body.size(); ++j) {
                const auto& q = state.body[j];
                if (std::hypot(p.x-q.x,p.z-q.z) < RadiusAt(i)+RadiusAt(j)) return false;
            }
        }
        return true;
    }

    SnakePopulation::SnakePopulation(const IslandTerrain& terrain,
        const std::span<const CircularObstacle> obstacles)
        : terrain_(terrain), obstacles_(obstacles), surface_(std::make_unique<RenderedTerrainSurface>(terrain))
    {
        Initialize(nullptr);
    }

    SnakePopulation::SnakePopulation(const IslandTerrain& terrain, const JungleLayout& jungle,
        const std::span<const CircularObstacle> obstacles)
        : terrain_(terrain), obstacles_(obstacles), surface_(std::make_unique<RenderedTerrainSurface>(terrain))
    {
        Initialize(&jungle);
    }

    void SnakePopulation::Initialize(const JungleLayout* jungle)
    {
        std::array<std::array<float,2>,Count> homes{};
        // The terrain-only constructor is used by focused motion tests. Give it the same
        // broad deterministic distribution instead of stacking expanded agents at (0,0).
        constexpr float goldenAngle=2.39996322973f;
        for(std::size_t index=0;index<Count;++index) {
            const float t=(static_cast<float>(index)+.5f)/static_cast<float>(Count);
            const float radius=28.0f+118.0f*std::sqrt(t);
            const float angle=static_cast<float>(index)*goldenAngle;
            homes[index]={radius*std::cos(angle),radius*std::sin(angle)};
        }
        if (jungle) {
            struct Candidate { float x, z; std::uint32_t rank; };
            std::vector<Candidate> candidates;
            // A deterministic coprime stride samples the full authored distribution without
            // turning startup into an all-pairs vegetation search.
            candidates.reserve((jungle->Understory().size() / 3U + 1U) * 4U);
            for (std::size_t plantIndex = 0; plantIndex < jungle->Understory().size(); plantIndex += 3U) {
                const auto& plant = jungle->Understory()[plantIndex];
                for (int direction = 0; direction < 4; ++direction) {
                    const std::uint32_t rank = Mix(0x736e616bU
                        + static_cast<std::uint32_t>(plantIndex * 4U + direction));
                    const float angle = static_cast<float>(direction)
                        * std::numbers::pi_v<float> * .5f
                        + static_cast<float>(rank & 1023U) / 1023.0f * .42f;
                    const float radius = 2.7f
                        + static_cast<float>((rank >> 10U) & 1023U) / 1023.0f * 1.5f;
                    const float x = plant.x + std::cos(angle) * radius;
                    const float z = plant.z + std::sin(angle) * radius;
                    if (!IsUnderstoryMargin(*jungle, x, z)) continue;
                    SnakeState probe;
                    probe.homeX = x; probe.homeZ = z;
                    for (std::size_t station = 0; station < probe.body.size(); ++station) {
                        const float px = x - static_cast<float>(station) * Spacing;
                        probe.body[station] = {px, GroundedY(px, z, RadiusAt(station)), z};
                    }
                    if (ValidBody(probe)) candidates.push_back({x, z, rank});
                }
            }
            std::stable_sort(candidates.begin(), candidates.end(), [](const auto& a, const auto& b) {
                return a.rank < b.rank;
            });
            std::size_t count = 0;
            for (const auto& candidate : candidates) {
                bool separated = true;
                for (std::size_t previous = 0; previous < count; ++previous) {
                    separated = separated && std::hypot(candidate.x - homes[previous][0],
                        candidate.z - homes[previous][1]) >= MinimumHomeSeparation;
                }
                if (!separated) continue;
                homes[count++] = {candidate.x, candidate.z};
                if (count == Count) break;
            }
            if (count != Count) {
                throw std::runtime_error("No complete separated understory-margin snake population");
            }
        }
        for (std::size_t index=0; index<Count; ++index) {
            bool found = false;
            auto& state = states_[index];
            for (int attempt=0; attempt<512 && !found; ++attempt) {
                const float angle = static_cast<float>(attempt)*2.399963f;
                const float radius = std::sqrt(static_cast<float>(attempt))*0.7f;
                state.homeX = homes[index][0]+radius*std::cos(angle);
                state.homeZ = homes[index][1]+radius*std::sin(angle);
                for (std::size_t j=0; j<state.body.size(); ++j) {
                    const float x = state.homeX-static_cast<float>(j)*Spacing;
                    const float z = state.homeZ;
                    state.body[j] = {x,GroundedY(x,z,RadiusAt(j)),z};
                }
                found = ValidBody(state);
            }
            if (!found) throw std::runtime_error("No safe whole-body ground snake habitat");
            trails_[index].assign(state.body.begin(),state.body.end());
            auto beyondTail = state.body.back();
            beyondTail.x -= Spacing;
            trails_[index].push_back(beyondTail);
            // Start QA and ordinary encounters in staggered active phases. Rest remains
            // part of the long cycle, but a newly discovered animal no longer resembles
            // a permanently static prop during the first few seconds.
            clocks_[index] = std::fmod(12.0f+static_cast<float>(index)*5.9f,60.0f);
        }
        if(jungle) {
            // A deliberately small arboreal population occupies separated mature broadleaf
            // trunks. Its body is sampled from the same triangle surface used by the canopy
            // renderer instead of floating on a guessed cylinder.
            for(std::size_t canopyIndex=0;canopyIndex<jungle->Canopy().size()
                    && treeCount_<TreeCount;++canopyIndex) {
                const auto& tree=jungle->Canopy()[canopyIndex];
                if(tree.variant>=NaturalCanopyFormCount || tree.scale<.88f || tree.scale>1.30f)continue;
                bool separated=true;
                for(std::size_t previous=0;previous<treeCount_;++previous)
                    separated=separated&&std::hypot(tree.x-treeHabitats_[previous].x,
                        tree.z-treeHabitats_[previous].z)>=32.0f;
                if(!separated)continue;
                const auto rank=Mix(0x74726565U+static_cast<std::uint32_t>(canopyIndex));
                auto& habitat=treeHabitats_[treeCount_];
                habitat={tree.x,tree.y,tree.z,tree.yawRadians,tree.scale,
                    3.5f+static_cast<float>(rank&255U)/255.0f*1.25f,
                    static_cast<float>((rank>>8U)&1023U)/1023.0f*2.0f*std::numbers::pi_v<float>,
                    tree.variant};
                treeClocks_[treeCount_]=static_cast<float>(treeCount_)*.81f+.35f;
                UpdateTreeSnake(treeCount_,treeClocks_[treeCount_]);
                ++treeCount_;
            }
            if(treeCount_!=TreeCount)
                throw std::runtime_error("No complete separated tree-snake habitat");
        }
    }

    void SnakePopulation::UpdateTreeSnake(const std::size_t index,const float clock)
    {
        auto& state=treeStates_[index];
        const auto& habitat=treeHabitats_[index];
        state.homeX=habitat.x;state.homeZ=habitat.z;state.arboreal=true;
        const float travel=std::sin(clock*.72f+static_cast<float>(index)*.63f);
        for(std::size_t station=0;station<state.body.size();++station) {
            const float path=travel-static_cast<float>(station)*.135f/habitat.scale;
            const float height=habitat.baseHeight+path*.39f
                +.08f*std::sin(path*1.7f+static_cast<float>(index));
            const float angle=habitat.baseAngle+path*.18f
                +.16f*std::sin(path*.9f+static_cast<float>(index)*.7f);
            const auto contact=SampleCanopyTrunkSurface(habitat.variant,height,angle,false);
            const float c=std::cos(habitat.yaw),s=std::sin(habitat.yaw);
            const float nx=c*contact.normal[0]-s*contact.normal[2];
            const float nz=s*contact.normal[0]+c*contact.normal[2];
            const float offset=RadiusAt(station)+.004f;
            state.body[station]={
                habitat.x+habitat.scale*(c*contact.position[0]-s*contact.position[2])+nx*offset,
                habitat.y+habitat.scale*contact.position[1]+contact.normal[1]*offset,
                habitat.z+habitat.scale*(s*contact.position[0]+c*contact.position[2])+nz*offset};
        }
        const auto& head=state.body[0];const auto& neck=state.body[1];
        state.heading=std::atan2(head.z-neck.z,head.x-neck.x);
        state.phase=std::fmod(clock*3.0f,2.0f*std::numbers::pi_v<float>);
    }

    void SnakePopulation::Update(const float deltaSeconds, const float observerX,
        const float observerY, const float observerZ)
    {
        if (!std::isfinite(deltaSeconds) || deltaSeconds <= 0.0f) return;
        const float frameDt = std::min(deltaSeconds,0.1f);
        const bool observerValid = std::isfinite(observerX) && std::isfinite(observerY) && std::isfinite(observerZ);
        ++frameIndex_;
        for (std::size_t index=0; index<Count; ++index) {
            auto& state = states_[index];
            const auto head = state.body.front();
            const float distance = observerValid ? std::hypot(std::hypot(head.x-observerX,
                head.z-observerZ),head.y-observerY) : 1000.0f;
            state.tier = WildlifeActivationPolicy::Classify(distance,65.0f,120.0f);
            state.speed = 0.0f;
            if (state.tier == WildlifeSimulationTier::Dormant) {
                accumulatedSeconds_[index] = 0.0f;
                continue;
            }
            accumulatedSeconds_[index] = std::min(accumulatedSeconds_[index] + frameDt, 0.1f);
            const std::uint32_t divisor = WildlifeActivationPolicy::UpdateDivisor(state.tier);
            if ((frameIndex_ + index) % divisor != 0U) continue;
            const float dt = accumulatedSeconds_[index];
            accumulatedSeconds_[index] = 0.0f;
            clocks_[index] = std::fmod(clocks_[index]+dt,60.0f);
            const bool retreat = distance < 8.0f;
            state.behavior = retreat ? SnakeBehavior::Retreat
                : clocks_[index] < 5.0f ? SnakeBehavior::Rest : SnakeBehavior::Explore;
            if (state.behavior == SnakeBehavior::Rest) continue;
            float target = 0.7f*std::sin(clocks_[index]*0.17f+static_cast<float>(index));
            if (std::hypot(head.x-state.homeX,head.z-state.homeZ) > HomeRadius-3.0f)
                target = std::atan2(state.homeZ-head.z,state.homeX-head.x);
            if (retreat) target = std::atan2(head.z-observerZ,head.x-observerX);
            constexpr float tau = 2.0f*std::numbers::pi_v<float>;
            state.heading = std::remainder(state.heading + std::clamp(
                std::remainder(target-state.heading,tau),-1.8f*dt,1.8f*dt),tau);
            const float speed = retreat ? 1.15f : 0.38f;
            const int steps = static_cast<int>(std::ceil(speed*dt/0.012f));
            float traveled = 0.0f;
            const auto turnAside = [&](const float advance) {
                if(!retreat) return false;
                const auto h=state.body[0],neck=state.body[1];
                const float length=std::hypot(h.x-neck.x,h.z-neck.z);
                if(length<0.000001f) return false;
                const float sideX=-(h.z-neck.z)/length,sideZ=(h.x-neck.x)/length;
                const float sign=(sideX*(h.x-observerX)+sideZ*(h.z-observerZ)>=0)?1.0f:-1.0f;
                const float headAdvance=advance*0.5f;
                auto turned=state;
                turned.body[0].x+=sideX*sign*headAdvance;
                turned.body[0].z+=sideZ*sign*headAdvance;
                // An articulated lateral turn, not a rigid translation or reversal.
                // Spread the bend over the front 66 cm, then restore joint lengths slowly.
                // Half the movement budget is reserved for length correction; every final
                // station displacement is checked below instead of trusting the projection.
                for(std::size_t j=1;j<turned.body.size();++j) {
                    const float oldLength=std::hypot(state.body[j].x-state.body[j-1].x,
                                               state.body[j].z-state.body[j-1].z);
                    const float correctionBudget=advance*0.5f/static_cast<float>(SnakeState::StationCount-1);
                    const float rest=oldLength+std::clamp(Spacing-oldLength,-correctionBudget,correctionBudget);
                    const auto previous=turned.body[j-1];
                    const float neckWeight=std::max(0.0f,1.0f-static_cast<float>(j)/12.0f);
                    const float dx=state.body[j].x+sideX*sign*headAdvance*neckWeight*neckWeight-previous.x;
                    const float dz=state.body[j].z+sideZ*sign*headAdvance*neckWeight*neckWeight-previous.z;
                    const float distance=std::hypot(dx,dz);
                    if(distance<0.000001f) return false;
                    turned.body[j].x=previous.x+dx*(rest/distance);
                    turned.body[j].z=previous.z+dz*(rest/distance);
                }
                for(std::size_t j=0;j<turned.body.size();++j) {
                    auto& p=turned.body[j];
                    if(std::hypot(p.x-state.body[j].x,p.z-state.body[j].z)>advance+0.00005f) return false;
                    p.y=GroundedY(p.x,p.z,RadiusAt(j));
                }
                if(std::hypot(turned.body[0].x-observerX,turned.body[0].z-observerZ)
                    <std::hypot(h.x-observerX,h.z-observerZ) || !ValidBody(turned)) return false;
                state.body=turned.body;
                state.heading=std::atan2(state.body[0].z-state.body[1].z,state.body[0].x-state.body[1].x);
                state.phase=std::fmod(state.phase+headAdvance*6.0f,tau);
                // Reseed only after an accepted articulated escape. Ordinary locomotion
                // continues using its retained head trail; rejected turns change nothing.
                auto& trail=trails_[index];
                trail.assign(state.body.begin(),state.body.end());
                auto tail=state.body.back();
                const auto beforeTail=state.body[state.body.size()-2];
                const float tailLength=std::hypot(tail.x-beforeTail.x,tail.z-beforeTail.z);
                for(int extra=0;extra<2;++extra) {
                    tail.x+=(state.body.back().x-beforeTail.x)*Spacing/tailLength;
                    tail.z+=(state.body.back().z-beforeTail.z)*Spacing/tailLength;
                    trail.push_back(tail);
                }
                traveled+=headAdvance;
                return true;
            };
            for (int step=0; step<steps; ++step) {
                auto candidate = state;
                const float advance = speed*dt/static_cast<float>(steps);
                const float direction = state.heading + 0.8f*std::sin(state.phase);
                auto& nextHead = candidate.body.front();
                nextHead.x += std::cos(direction)*advance;
                nextHead.z += std::sin(direction)*advance;
                if (!ValidPoint(nextHead.x,nextHead.z,MaximumRadius)) {
                    if(turnAside(advance)) continue;
                    state.heading = std::remainder(state.heading+0.25f,tau);
                    break;
                }
                if (retreat && std::hypot(nextHead.x-observerX,nextHead.z-observerZ)
                        < std::hypot(state.body[0].x-observerX,state.body[0].z-observerZ)) {
                    if(turnAside(advance)) continue;
                    break;
                }
                nextHead.y = GroundedY(nextHead.x,nextHead.z,RadiusAt(0));
                const auto& trail = trails_[index];
                auto segmentStart = nextHead;
                std::size_t trailIndex = 0;
                float distanceBefore = 0.0f;
                bool completeTrail = true;
                for (std::size_t j=1; j<candidate.body.size(); ++j) {
                    const float wanted = static_cast<float>(j)*Spacing;
                    bool sampled = false;
                    while (trailIndex < trail.size()) {
                        const auto end = trail[trailIndex];
                        const float length = std::hypot(end.x-segmentStart.x,end.z-segmentStart.z);
                        if (length > 0.000001f && distanceBefore+length >= wanted) {
                            const float t = (wanted-distanceBefore)/length;
                            auto& p = candidate.body[j];
                            p.x = segmentStart.x+(end.x-segmentStart.x)*t;
                            p.z = segmentStart.z+(end.z-segmentStart.z)*t;
                            p.y = GroundedY(p.x,p.z,RadiusAt(j));
                            sampled = true;
                            break;
                        }
                        distanceBefore += length;
                        segmentStart = end;
                        ++trailIndex;
                    }
                    if (!sampled) { completeTrail = false; break; }
                }
                if (!completeTrail || !ValidBody(candidate)) {
                    if(turnAside(advance)) continue;
                    state.heading = std::remainder(state.heading+0.25f,tau);
                    break;
                }
                state.body = candidate.body;
                // Commit only accepted motion. Keep one full segment beyond the tail.
                auto& committed = trails_[index];
                while (committed.size() > trailIndex+2) committed.pop_back();
                if (std::hypot(nextHead.x-committed.front().x,nextHead.z-committed.front().z) >= 0.006f)
                    committed.push_front(nextHead);
                if (committed.size() > 512) throw std::length_error("Snake trail budget exceeded");
                state.phase = std::fmod(state.phase+advance*6.0f,tau);
                traveled += advance;
            }
            state.speed = traveled/dt;
        }
        for(std::size_t index=0;index<treeCount_;++index) {
            auto& state=treeStates_[index];
            const auto previous=state.body.front();
            const float distance=observerValid ? std::hypot(std::hypot(previous.x-observerX,
                previous.z-observerZ),previous.y-observerY) : 1000.0f;
            state.tier=WildlifeActivationPolicy::Classify(distance,75.0f,135.0f);
            state.speed=0;
            if(state.tier==WildlifeSimulationTier::Dormant)continue;
            const bool retreat=distance<5.0f;
            state.behavior=retreat?SnakeBehavior::Retreat:SnakeBehavior::Explore;
            treeClocks_[index]+=frameDt*(retreat?.52f:.22f);
            UpdateTreeSnake(index,treeClocks_[index]);
            const auto head=state.body.front();
            state.speed=std::hypot(std::hypot(head.x-previous.x,head.z-previous.z),
                head.y-previous.y)/frameDt;
        }
    }
}
