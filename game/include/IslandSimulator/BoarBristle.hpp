#pragma once
#include "IslandSimulator/BoarPose.hpp"
#include <algorithm>
#include <cmath>

namespace island {
struct BoarBristle { BoarJoint left,tip,right; float tone; };
inline constexpr int BoarBristleCount=135;
// Three staggered rows, seated in the same deformed loft used by the body renderer.
inline BoarBristle BuildBoarBristle(const BoarBodyProfile& profile,int index,bool juvenile,float bob) {
    const int row=index/45;const float t=static_cast<float>(index%45);
    const float z=-.55f+t*.021f+std::sin(t*2.7f+row)*.007f;
    const float x=static_cast<float>(row-1)*.035f+std::sin(t*2.1f+row)*.014f;
    const float shoulder=std::clamp((z+.3f)/.65f,0.0f,1.0f);
    const float length=(juvenile?.020f:.034f)+shoulder*.019f
        +(.5f+.5f*std::sin(t*1.7f+row))*.014f;
    const float lean=.018f+.009f*std::sin(t*.9f+row);
    const float y=BoarBackHeight(profile,z,x)+bob;
    return {{x-.0025f,BoarBackHeight(profile,z,x-.0025f)+bob-.006f,z},
        {x+.006f*std::sin(t),y+length,z-lean},
        {x+.0025f,BoarBackHeight(profile,z+.006f,x+.0025f)+bob-.006f,z+.006f},
        .72f+.22f*(.5f+.5f*std::sin(t*2.3f+row))};
}
}
