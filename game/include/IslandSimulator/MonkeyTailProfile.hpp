#pragma once

#include <algorithm>
#include <array>
#include <cmath>

namespace island::MonkeyTailProfile
{
    struct Point { float x, y, z; };
    inline Point operator+(Point a, Point b) { return {a.x+b.x, a.y+b.y, a.z+b.z}; }
    inline Point operator-(Point a, Point b) { return {a.x-b.x, a.y-b.y, a.z-b.z}; }
    inline Point operator*(Point a, float b) { return {a.x*b, a.y*b, a.z*b}; }
    struct Section {
        Point centre, tangent;
        float radius, secondWeight;
        int firstBone, secondBone; // relative to the three existing tail bones
    };

    // C1 centreline through the three bone anchors and the terminal tip. Smooth
    // adjacent weights share the same transform/tangent at each bone anchor.
    inline Section Sample(const std::array<Point, 4>& anchors, float t)
    {
        t = std::clamp(t, 0.0f, 1.0f);
        const int segment = std::min(2, static_cast<int>(t * 3.0f));
        const float u = t * 3.0f - segment, u2 = u*u, u3 = u2*u;
        const auto a = anchors[segment], b = anchors[segment+1];
        const auto ma = segment == 0 ? b-a : (b-anchors[segment-1])*0.5f;
        const auto mb = segment == 2 ? b-a : (anchors[segment+2]-a)*0.5f;
        const auto centre = a*(2*u3-3*u2+1) + ma*(u3-2*u2+u)
                          + b*(-2*u3+3*u2) + mb*(u3-u2);
        const auto tangent = (a*(6*u2-6*u) + ma*(3*u2-4*u+1)
                            + b*(-6*u2+6*u) + mb*(3*u2-2*u))*3.0f;
        // A strong furry base tapers progressively before the closed tip. The
        // near mesh adds sparse raised fur clumps, so this underlying skin need
        // not remain a uniform hose merely to preserve a furry silhouette.
        // A capuchin-like prehensile tail is muscular at the root and remains
        // visibly fur-bearing through the hook.  Keep the terminal closure rapid
        // so the larger profile does not end as a blunt open hose.
        const float radius=(.040f+.050f*(1-t))*std::sqrt(std::max(0.0f,1.0f-std::pow(t,16.0f)));
        return {centre, tangent, radius, u2*(3-2*u),
                segment, std::min(2, segment+1)};
    }
}
