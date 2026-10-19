/* Strict private geometry predicate; no mesh payload or public ABI change. */
#pragma once
#include "../../include/adaptation/CNativeMesh3D.hpp"
namespace SU2Native3D {
/*! Exact sign of dot(cross(reference edges),cross(actual edges)); zero includes a degenerate face. */
int SurfaceOrientationSign(const std::array<Point,3>& reference,const std::array<Point,3>& actual,KernelStats* stats);
}
