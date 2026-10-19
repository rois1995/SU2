/* Strict private geometry predicate; no mesh payload or public ABI change. */
#pragma once
#include "../../include/adaptation/CNativePointConstruction3D.hpp"
namespace SU2Native3D {
/*! Exact sign of dot(cross(reference edges),cross(actual edges)); zero includes a degenerate face. */
int SurfaceOrientationSign(const std::array<Point,3>& reference,const std::array<Point,3>& actual,KernelStats* stats);
int SurfaceNormalSign(Point normal,const std::array<Point,3>& actual,KernelStats* stats);
/*! Exact sign of normal dot(ideal point-origin), including cancellation. */
int ParametricPlaneSide(const ParametricPoint& point,Point origin,Point normal);
/*! Ideal chart predicates; no rounded reconstruction of centroid/intersection coordinates. */
int ParametricCentroidSide(const ParametricPoint& a,const ParametricPoint& b,
                          const std::array<ParametricPoint,3>& triangle,unsigned axis,KernelStats* stats=nullptr);
bool ParametricSegmentEntersTriangle(const ParametricPoint& a,const ParametricPoint& b,
                                    const std::array<ParametricPoint,3>& triangle,unsigned axis,KernelStats* stats=nullptr);
bool ParametricOnProjectedSegment(const ParametricPoint& a,const ParametricPoint& b,const ParametricPoint& point,
                                 unsigned axis,KernelStats* stats=nullptr);
}
