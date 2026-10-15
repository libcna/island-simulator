#include "IslandSimulator/RenderedTerrainSurface.hpp"
#include "IslandSimulator/JungleGrounding.hpp"
#include "IslandSimulator/JungleLayout.hpp"
#include "IslandSimulator/GroundLitter.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>

int main()
{
    using L = island::TerrainMeshLayout;
    const island::IslandTerrain terrain;
    const island::RenderedTerrainSurface surface(terrain);
    int failures = 0;
    const auto check = [&](bool value, const char* message) {
        if (!value) { if (failures < 12) std::cerr << message << '\n'; ++failures; }
    };
    // Search the uploaded coordinates independently at each representable grid edge.
    // An arithmetic cell lookup must preserve which triangle owns the edge, including
    // its normal, rather than merely return a continuous-looking height.
    std::array<float, L::Resolution> coordinates{};
    for (int i=0;i<L::Resolution;++i) coordinates[i]=L::Coordinate(i);
    const auto auditBoundary = [&](float x,float z) {
        const auto quad = [&](float p) {
            return std::clamp(static_cast<int>(std::upper_bound(
                coordinates.begin(),coordinates.end(),p)-coordinates.begin())-1,0,L::QuadsPerAxis-1);
        };
        const int col=quad(x),row=quad(z),index=row*L::Resolution+col;
        const double width=double(coordinates[col+1])-coordinates[col];
        const double depth=double(coordinates[row+1])-coordinates[row];
        const double u=(double(x)-coordinates[col])/width,v=(double(z)-coordinates[row])/depth;
        const auto heights=surface.Heights();
        const double tl=heights[index],tr=heights[index+1];
        const double bl=heights[index+L::Resolution],br=heights[index+L::Resolution+1];
        const bool upper=u+v<=1.0;
        const double h=upper?tl+(tr-tl)*u+(bl-tl)*v:br+(bl-br)*(1-u)+(tr-br)*(1-v);
        const double dx=upper?(tr-tl)/width:(br-bl)/width;
        const double dz=upper?(bl-tl)/depth:(br-tr)/depth;
        const double length=std::hypot(dx,1.0,dz);
        const auto actual=surface.Sample(x,z);
        check(actual.height==static_cast<float>(h)
            &&actual.normal.x==static_cast<float>(-dx/length)
            &&actual.normal.y==static_cast<float>(1.0/length)
            &&actual.normal.z==static_cast<float>(-dz/length),
            "Fast grid lookup changed boundary triangle ownership or height");
    };
    for(float edge:coordinates) {
        for(float value:{std::nextafter(edge,-INFINITY),edge,std::nextafter(edge,INFINITY)}) {
            if(std::abs(value)>L::Extent)continue;
            auditBoundary(value,17.123f);
            auditBoundary(-31.234f,value);
            auditBoundary(value,value);
        }
    }
    for (int row = 0; row < L::Resolution; ++row) for (int col = 0; col < L::Resolution; ++col) {
        const float x = L::Coordinate(col), z = L::Coordinate(row);
        check(std::abs(surface.Sample(x, z).height - terrain.Height(x, z)) < 0.000005f,
              "Rendered grid vertex height differs from the uploaded terrain");
        const float h=terrain.Height(x,z);
        check(std::abs(island::RenderedTerrainSurface::DecodeHeight(
            island::RenderedTerrainSurface::EncodeHeight(h))-h)<0.00002f,"RGB24 height encoding lost precision");
        check(std::abs(surface.SamplePacked(x,z)-h)<0.0001f,"Packed shader emulation missed a grid vertex");
    }
    // Independent cross-product plane evaluation against the exact three uploaded vertices.
    for (int row = 0; row < L::QuadsPerAxis; row += 3) for (int col = 0; col < L::QuadsPerAxis; col += 3) {
        using P = std::array<double, 3>;
        const auto point = [&](int c, int r) -> P {
            const float x = L::Coordinate(c), z = L::Coordinate(r);
            return {x, terrain.Height(x, z), z};
        };
        const auto tl = point(col, row), tr = point(col+1, row);
        const auto bl = point(col, row+1), br = point(col+1, row+1);
        for (const auto& triangle : {std::array{tl, bl, tr}, std::array{tr, bl, br}}) {
            const auto a = triangle[0], b = triangle[1], c = triangle[2];
            const P ab{b[0]-a[0], b[1]-a[1], b[2]-a[2]};
            const P ac{c[0]-a[0], c[1]-a[1], c[2]-a[2]};
            const P n{ab[1]*ac[2]-ab[2]*ac[1], ab[2]*ac[0]-ab[0]*ac[2], ab[0]*ac[1]-ab[1]*ac[0]};
            const double length = std::hypot(n[0], n[1], n[2]);
            const float x = static_cast<float>(a[0]*0.21+b[0]*0.33+c[0]*0.46);
            const float z = static_cast<float>(a[2]*0.21+b[2]*0.33+c[2]*0.46);
            const double expected = a[1] - (n[0]*(x-a[0])+n[2]*(z-a[2]))/n[1];
            const auto actual = surface.Sample(x,z);
            check(std::abs(actual.height-expected)<0.00001, "Triangle plane height mismatch");
            check(std::abs(surface.SamplePacked(x,z)-actual.height)<0.0001f,
                  "Packed triangle reconstruction exceeds 0.1 mm tolerance");
            check(std::abs(actual.normal.x-n[0]/length)<0.000002
                  && std::abs(actual.normal.y-n[1]/length)<0.000002
                  && std::abs(actual.normal.z-n[2]/length)<0.000002, "Triangle normal mismatch");
        }
        // The shared diagonal and axis-aligned edges must remain continuous from either side.
        for (const auto& edge : {std::array{tr,bl}, std::array{tl,tr}, std::array{tl,bl}}) {
            const float x = static_cast<float>((edge[0][0]+edge[1][0])*0.5);
            const float z = static_cast<float>((edge[0][2]+edge[1][2])*0.5);
            const float h = static_cast<float>((edge[0][1]+edge[1][1])*0.5);
            check(std::abs(surface.Sample(x,z).height-h)<0.0001f, "Shared edge height mismatch");
            for (float offset : {-0.0001f,0.0001f}) {
                if (std::abs(x+offset)<=L::Extent && std::abs(z+offset)<=L::Extent) {
                    check(std::abs(surface.Sample(x+offset,z+offset).height-h)<0.001f,
                          "Discontinuous shared terrain edge");
                }
            }
        }
    }
    for(int row=10;row<230;row+=17) for(int col=10;col<230;col+=19) {
        const float x=L::Coordinate(col)+0.371f,z=L::Coordinate(row)+0.529f;
        const auto range=surface.HeightRange(x-1.7f,z-1.7f,x+1.7f,z+1.7f);
        for(int ix=0;ix<9;++ix) for(int iz=0;iz<9;++iz) {
            const float h=surface.Sample(x-1.7f+ix*0.425f,z-1.7f+iz*0.425f).height;
            check(h>=range[0]-0.00001f && h<=range[1]+0.00001f,
                  "Footprint height range omitted a rendered triangle point");
        }
    }
    for(float h : {-64.0f,0.0f,64.0f}) {
        check(std::abs(island::RenderedTerrainSurface::DecodeHeight(
            island::RenderedTerrainSurface::EncodeHeight(h))-h)<0.00002f,"Encoding endpoint mismatch");
    }
    for(float invalid : {-65.0f,65.0f,std::numeric_limits<float>::infinity(),
                        std::numeric_limits<float>::quiet_NaN()}) {
        bool rejected=false;
        try { (void)island::RenderedTerrainSurface::EncodeHeight(invalid); }
        catch(const std::invalid_argument&) { rejected=true; }
        check(rejected,"Invalid height encoding accepted");
    }
    bool reversedRejected=false;
    try { (void)surface.HeightRange(1,0,-1,0); }
    catch(const std::invalid_argument&) { reversedRejected=true; }
    check(reversedRejected,"Reversed terrain footprint accepted");
    std::size_t auditedRoots = 0;
    float largestOldGap = 0;
    // Three independently generated layouts, all four instance types. Evaluate the
    // triangle plane from its grid vertices rather than repeating Sample at the root.
    for (const auto seed : {island::JungleLayout::DefaultSeed, 0x12345U, 0xa55aU}) {
        const island::JungleLayout layout(terrain, seed);
        const auto audit = [&](const auto& plants) {
            for (const auto& source : plants) {
                const auto grounded = island::GroundJunglePlacement(source, surface);
                check(grounded.x == source.x && grounded.z == source.z
                      && grounded.scale == source.scale && grounded.variant == source.variant
                      && grounded.yawRadians == source.yawRadians,
                      "Grounding changed plant identity, position, scale or yaw");
                int col = 0, row = 0;
                while (col < L::QuadsPerAxis-1 && L::Coordinate(col+1) <= source.x) ++col;
                while (row < L::QuadsPerAxis-1 && L::Coordinate(row+1) <= source.z) ++row;
                const double x0 = L::Coordinate(col), x1 = L::Coordinate(col+1);
                const double z0 = L::Coordinate(row), z1 = L::Coordinate(row+1);
                const double tx = (source.x-x0)/(x1-x0), tz = (source.z-z0)/(z1-z0);
                const double tr = terrain.Height(static_cast<float>(x1), static_cast<float>(z0));
                const double bl = terrain.Height(static_cast<float>(x0), static_cast<float>(z1));
                const double corner = terrain.Height(static_cast<float>(tx+tz<=1 ? x0 : x1),
                                                      static_cast<float>(tx+tz<=1 ? z0 : z1));
                const double expected = tx+tz<=1 ? corner*(1-tx-tz)+tr*tx+bl*tz
                    : corner*(tx+tz-1)+bl*(1-tx)+tr*(1-tz);
                check(std::abs(grounded.y-expected)<0.00002,
                      "Jungle root origin differs from the rendered triangle plane");
                check(island::GroundJunglePlacement(grounded,surface).y == grounded.y,
                      "Grounding is not idempotent");
                largestOldGap = std::max(largestOldGap, std::abs(source.y-grounded.y));
                ++auditedRoots;
            }
        };
        audit(layout.Canopy()); audit(layout.Understory());
        audit(layout.Bushes()); audit(layout.GroundPlants());
    }
    check(auditedRoots>15000 && largestOldGap>0.10f,
          "Root-contact regression no longer covers the observed analytic-height discrepancy");
    double oldMinimum=1000,oldMaximum=-1000,conformedMinimum=1000,conformedMaximum=-1000;
    std::size_t litterSamples=0,oldBuriedSamples=0,conformedBuriedSamples=0;
    for (int leaf=0;leaf<island::GroundLitterCount;++leaf) {
        const auto points=island::MakeGroundLitterLeaf(leaf);
        for (const auto& point:points) {
            check(std::hypot(point[0],point[2])<=island::GroundLitterRadius
                  && point[1]>=0.021f && point[1]<=0.052f,
                  "Litter profile escaped its documented culling envelope");
        }
        for (std::size_t t=0;t<island::GroundLitterIndices.size();t+=3) {
            const auto a=points.at(island::GroundLitterIndices[t]);
            const auto b=points.at(island::GroundLitterIndices[t+1]);
            const auto c=points.at(island::GroundLitterIndices[t+2]);
            check(std::abs((b[0]-a[0])*(c[2]-a[2])-(b[2]-a[2])*(c[0]-a[0]))>0.0001f,
                  "Degenerate litter triangle planform");
        }
    }
    for (const int index : {-1,island::GroundLitterCount}) {
        bool rejected=false;
        try { (void)island::MakeGroundLitterLeaf(index); }
        catch (const std::invalid_argument&) { rejected=true; }
        check(rejected,"Invalid litter index accepted");
    }
    for (const auto seed : {island::JungleLayout::DefaultSeed, 0x12345U, 0xa55aU}) {
        const island::JungleLayout layout(terrain,seed);
        for (const auto& source:layout.GroundPlants()) {
            if (source.variant!=0) continue;
            const auto plant=island::GroundJunglePlacement(source,surface);
            const float c=std::cos(plant.yawRadians),s=std::sin(plant.yawRadians);
            for (int leaf=0;leaf<island::GroundLitterCount;++leaf) {
                const auto local=island::MakeGroundLitterLeaf(leaf);
                std::array<island::LitterPoint,island::GroundLitterVertexCount> old{},conformed{};
                const float radius=island::GroundLitterRadius*plant.scale;
                const auto heightBounds=surface.HeightRange(plant.x-radius,plant.z-radius,
                                                             plant.x+radius,plant.z+radius);
                for (std::size_t v=0;v<local.size();++v) {
                    const float x=plant.x+(c*local[v][0]+s*local[v][2])*plant.scale;
                    const float z=plant.z+(-s*local[v][0]+c*local[v][2])*plant.scale;
                    old[v]={x,plant.y+local[v][1]*plant.scale,z};
                    conformed[v]={x,surface.SamplePacked(x,z)+local[v][1]*plant.scale,z};
                    check(conformed[v][1]>=heightBounds[0]-0.0001f
                          && conformed[v][1]<=heightBounds[1]+0.052f*plant.scale+0.0001f,
                          "Terrain-draped litter vertex escaped renderer height bounds");
                }
                for (std::size_t tri=0;tri<island::GroundLitterIndices.size();tri+=3) {
                    for (int a=0;a<=4;++a) for (int b=0;b<=4-a;++b) {
                        const std::array<float,3> weights{a/4.0f,b/4.0f,1-(a+b)/4.0f};
                        island::LitterPoint point{};
                        float newY=0;
                        for (int v=0;v<3;++v) {
                            const auto index=island::GroundLitterIndices[tri+v];
                            for (int axis=0;axis<3;++axis) point[axis]+=old[index][axis]*weights[v];
                            newY+=conformed[index][1]*weights[v];
                        }
                        const float ground=surface.Sample(point[0],point[2]).height;
                        const double oldGap=point[1]-ground,newGap=newY-ground;
                        oldMinimum=std::min(oldMinimum,oldGap); oldMaximum=std::max(oldMaximum,oldGap);
                        conformedMinimum=std::min(conformedMinimum,newGap);
                        conformedMaximum=std::max(conformedMaximum,newGap);
                        oldBuriedSamples+=oldGap<0; conformedBuriedSamples+=newGap<0;
                        ++litterSamples;
                    }
                }
            }
        }
    }
    std::cout << "Litter samples " << litterSamples << ", origin-only gaps " << oldMinimum << ".."
              << oldMaximum << " m (buried " << oldBuriedSamples << "), conformed gaps "
              << conformedMinimum << ".." << conformedMaximum << " m (buried "
              << conformedBuriedSamples << ")\n";
    check(litterSamples>500000 && oldBuriedSamples>100000 && oldMinimum<-0.15,
          "Litter audit missed the known sloped-plane defect");
    check(conformedBuriedSamples==0 && conformedMinimum>0.009 && conformedMaximum<0.07,
          "Conformed folded litter intersects terrain or exceeds its folded-leaf height envelope");
    for (float invalid : {-341.0f,341.0f,std::numeric_limits<float>::infinity(),
                          std::numeric_limits<float>::quiet_NaN()}) {
        for (bool swap : {false,true}) {
            bool rejected = false;
            try { (void)surface.Sample(swap ? 0 : invalid, swap ? invalid : 0); }
            catch (const std::invalid_argument&) { rejected = true; }
            check(rejected,"Invalid rendered-surface coordinate was accepted");
        }
    }
    if (failures) return 1;
    std::cout << "Rendered terrain vertices, triangle planes, normals, seams and input bounds passed; "
              << auditedRoots << " jungle root origins checked, old maximum gap " << largestOldGap << " m\n";
}
