#include "IslandSimulator/SplitGroundLeaf.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>

void TestSplitGroundLeaf()
{
    using namespace island;
    const auto require=[](bool value,const char* message) {
        if (!value) throw std::runtime_error(message);
    };
    for (bool far : {false,true}) {
        const int rows=SplitGroundLeafRows(far),columns=SplitGroundLeafColumns(far);
        const auto indices=SplitGroundLeafIndices(far);
        require(indices.size()==static_cast<std::size_t>((rows-1)*(columns-1)*6),"Split-leaf triangle count changed");
        std::vector<std::vector<int>> adjacency(rows*columns);
        for (std::size_t i=0;i<indices.size();i+=3) for (int edge=0;edge<3;++edge) {
            const auto a=indices[i+edge],b=indices[i+(edge+1)%3];
            require(a<rows*columns && b<rows*columns && a!=b,"Invalid split-leaf triangle indices");
            adjacency[a].push_back(b); adjacency[b].push_back(a);
        }
        std::vector<bool> visited(rows*columns);
        std::vector<int> pending{0};
        visited[0]=true;
        while (!pending.empty()) {
            const int current=pending.back(); pending.pop_back();
            for (int neighbor:adjacency[current]) if (!visited[neighbor]) {
                visited[neighbor]=true; pending.push_back(neighbor);
            }
        }
        require(std::all_of(visited.begin(),visited.end(),[](bool v) { return v; }),
                "Split-leaf lobes or petiole are disconnected from the indexed surface");
        require(SplitGroundLeafRow(0,far)==0 && SplitGroundLeafRow(rows-1,far)==1,
                "Split-leaf mesh omits root or tip");
        for (int row=1;row<rows;++row) require(SplitGroundLeafRow(row,far)>SplitGroundLeafRow(row-1,far),
                "Split-leaf rows are not ordered");
        for (int bad : {-1,rows}) {
            bool rejected=false;
            try { (void)SplitGroundLeafRow(bad,far); } catch (const std::invalid_argument&) { rejected=true; }
            require(rejected,"Invalid split-leaf row accepted");
        }
    }
    std::array<bool,5> retained{};
    for (int index=0;index<4;++index) {
        const auto far=SplitGroundLeafForTier(index,true);
        const auto near=SplitGroundLeafForTier(far.source,false);
        require(far==near && !retained.at(far.source),"Split-leaf far identity mismatch or duplicate");
        retained[far.source]=true;
        for (int row=0;row<9;++row) for (int column=0;column<3;++column) {
            const float nearT=SplitGroundLeafRow(row*SplitGroundLeafNearSubdivisions,false);
            const float farT=SplitGroundLeafRow(row,true);
            const auto a=SampleSplitGroundLeaf(near,nearT,static_cast<float>(column-1));
            const auto b=SampleSplitGroundLeaf(far,farT,static_cast<float>(column-1));
            require(nearT==farT && a.across==b.across && a.height==b.height && a.along==b.along,
                "Shared split-leaf LOD knots disagree in actual generated positions");
        }
    }
    std::array<float,5> lengths{},lifts{},pigments{};
    int samples=0;
    for (int index=0;index<5;++index) {
        const auto leaf=SplitGroundLeafForTier(index,false);
        lengths[index]=leaf.length;
        lifts[index]=leaf.lift;
        pigments[index]=leaf.pigment;
        // Check the generated mesh, not merely the continuous profile: a midpoint-only
        // grid misses smoothstep curvature and retains visibly pointed lobe shoulders.
        const float lower=SampleSplitGroundLeaf(leaf,SplitGroundLeafRow(4,false),1).across;
        const float upper=SampleSplitGroundLeaf(leaf,SplitGroundLeafRow(8,false),1).across;
        const float quarter=SampleSplitGroundLeaf(leaf,SplitGroundLeafRow(5,false),1).across;
        const float threeQuarter=SampleSplitGroundLeaf(leaf,SplitGroundLeafRow(7,false),1).across;
        require(quarter<std::lerp(lower,upper,0.25f)-0.015f
            && threeQuarter>std::lerp(lower,upper,0.75f)+0.015f,
            "Actual split-leaf mesh no longer samples rounded lobe shoulders");
        for (int row=0;row<=200;++row) for (int column=0;column<=20;++column) {
            const float t=row/200.0f,side=column/10.0f-1;
            const auto p=SampleSplitGroundLeaf(leaf,t,side);
            const auto n=SplitGroundLeafNormal(leaf,t,side);
            require(std::isfinite(p.across) && std::isfinite(p.height) && std::isfinite(p.along)
                && p.height>=0 && p.height<0.65f && std::hypot(p.across,p.along)<0.85f,
                "Split ground leaf escaped its local above-ground envelope");
            require(std::abs(std::hypot(n.across,n.height,n.along)-1)<0.00001f,
                "Split ground leaf normal is not finite and unit length");
            if (row==0) require(p.height==0 && p.along==0
                && p.across==side*SplitGroundPetioleHalfWidth,"Split ground leaf root moved");
            if (row==200) require(p.across==0 && p.height>0.09f && p.along==leaf.length,
                "Split ground leaf tip is not closed or still droops below its origin");
            if (row>2 && row<198 && column>0 && column<20) {
                const auto a=SampleSplitGroundLeaf(leaf,t-0.0015f,side);
                const auto b=SampleSplitGroundLeaf(leaf,t+0.0015f,side);
                require(std::abs(n.across*(b.across-a.across)+n.height*(b.height-a.height)
                    +n.along*(b.along-a.along))<0.00001f,"Split-leaf normal is not tangent-orthogonal");
            }
            ++samples;
        }
        const float peak=SampleSplitGroundLeaf(leaf,0.34f,1).across;
        for (float valley:{0.45f,0.67f,0.88f}) require(SampleSplitGroundLeaf(leaf,valley,1).across<peak*0.30f,
            "Split-leaf outline lost its deep connected lobes");
    }
    const auto spread=[](const auto& values) {
        const auto [low,high]=std::minmax_element(values.begin(),values.end());
        return *high-*low;
    };
    require(spread(lengths)>0.10f && spread(lifts)>0.06f && spread(pigments)>0.15f,
        "Split ground leaf rosette lost its layered stature or pigment variation");
    for (bool far:{false,true}) for (int invalid:{-1,far?4:5}) {
        bool rejected=false;
        try { (void)SplitGroundLeafForTier(invalid,far); } catch (const std::invalid_argument&) { rejected=true; }
        require(rejected,"Invalid split-leaf source accepted");
    }
    for (int field=0;field<8;++field) {
        auto leaf=SplitGroundLeafForTier(0,false);
        float t=0.5f,side=0;
        switch (field) {
        case 0: t=-1; break;
        case 1: side=2; break;
        case 2: t=std::numeric_limits<float>::quiet_NaN(); break;
        case 3: leaf.length=0; break;
        case 4: leaf.width=std::numeric_limits<float>::infinity(); break;
        case 5: leaf.phase=-1; break;
        case 6: leaf.lift=2; break;
        case 7: leaf.lift=0.001f; break;
        }
        bool rejected=false;
        try { (void)SplitGroundLeafNormal(leaf,t,side); } catch (const std::invalid_argument&) { rejected=true; }
        require(rejected,"Invalid split-leaf parameters accepted");
    }
    std::cout << "Split ground leaf: " << samples << " above-ground samples, connected topology and shared LOD knots\n";
}
