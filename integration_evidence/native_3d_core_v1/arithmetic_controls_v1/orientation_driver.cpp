#include "Common/include/adaptation/CNativeMesh3D.hpp"
#include <iostream>
int main() { SU2Native3D::Tetrahedron t; SU2Native3D::KernelStats s;
while(std::cin >> t[0].x >> t[0].y >> t[0].z >> t[1].x >> t[1].y >> t[1].z
 >> t[2].x >> t[2].y >> t[2].z >> t[3].x >> t[3].y >> t[3].z) {
auto d=SU2Native3D::Orientation(t[0],t[1],t[2],t[3],&s);std::cout << (d>0)-(d<0) << "\n"; }
std::cerr << "filtered " << s.filtered_orientations << " exact " << s.exact_orientations << "\n"; }
