/* Native BL vector operations shared by the target and construction.
 * SPDX-License-Identifier: LGPL-2.1-or-later */
#pragma once
#include "../../../Common/include/adaptation/CNativeMesh3D.hpp"
#include <cmath>
#include <stdexcept>
namespace junction_wp0 {
using namespace SU2Native3D;
struct V { long double x=0,y=0,z=0; };
inline V Of(Point p){return {p.x,p.y,p.z};}
inline Point Pt(V v){return {double(v.x),double(v.y),double(v.z)};}
inline V operator+(V a,V b){return {a.x+b.x,a.y+b.y,a.z+b.z};}
inline V operator-(V a,V b){return {a.x-b.x,a.y-b.y,a.z-b.z};}
inline V operator*(long double s,V a){return {s*a.x,s*a.y,s*a.z};}
inline long double Dot(V a,V b){return a.x*b.x+a.y*b.y+a.z*b.z;}
inline V Cross(V a,V b){return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};}
inline long double Norm(V a){return std::sqrt(Dot(a,a));}
inline V Unit(V a){const auto n=Norm(a);if(!(n>0))throw std::runtime_error("WP0B degenerate direction.");return (1/n)*a;}
}
