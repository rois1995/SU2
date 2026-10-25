/*!
 * \file CNativeBLComposition3D.hpp
 * \brief Native 3D remesher target: original sensor intersected with the geometric BL law at actual queries,
 *        and its chord-pinned edge geometry (one definition for the SU2 adapter and its tests).
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
#pragma once
#include <memory>
#include "CBoundaryLayerMetric.hpp"
#include "../../../Common/include/adaptation/CNativeField3D.hpp"

namespace SU2NativeBL3D {
/*! Logarithmic blending is not order preserving; restore every finer sensor direction afterwards. */
SU2Native3D::Tensor PreserveSensor(SU2Native3D::Tensor metric,SU2Native3D::Tensor sensor);
/*! sensor ∩ BL of every wall at p (ApplyPoint without the legacy tangential floor). With owners, each faceted
 *  wall evaluates the facet owning chord parameter t (ApplyPointOnChord). */
inline SU2Native3D::Tensor Compose(CBoundaryLayerMetric& layers, double core, SU2Native3D::Point p,
                                   SU2Native3D::Tensor sensor,
                                   const CBoundaryLayerMetric::ChordOwners* owners = nullptr, long double t = 0) {
  const su2double x[3] = {p.x, p.y, p.z};
  CBoundaryLayerMetric::Tensor m;
  m.m[0][0] = sensor.xx; m.m[0][1] = m.m[1][0] = sensor.xy; m.m[0][2] = m.m[2][0] = sensor.xz;
  m.m[1][1] = sensor.yy; m.m[1][2] = m.m[2][1] = sensor.yz; m.m[2][2] = sensor.zz;
  if (owners) layers.ApplyPointOnChord(x, m, core, *owners, t, false);
  else layers.ApplyPoint(x, m, core, nullptr, false);
  return SU2Native3D::Tensor{SU2_TYPE::GetValue(m.m[0][0]), SU2_TYPE::GetValue(m.m[0][1]),
                             SU2_TYPE::GetValue(m.m[0][2]), SU2_TYPE::GetValue(m.m[1][1]),
                             SU2_TYPE::GetValue(m.m[1][2]), SU2_TYPE::GetValue(m.m[2][2])};
}
/*! Pointwise composed target of the native 3D remesher. */
inline SU2Native3D::MetricComposition Composition(std::shared_ptr<CBoundaryLayerMetric> layers, double core) {
  return [layers, core](SU2Native3D::Point p, SU2Native3D::Tensor sensor) {
    return Compose(*layers, core, p, sensor);
  };
}
/*! Geometric cuts of each chord plus the owner-pinned integrand (WP-C): the facet owning each chord parameter is the
 *  one of the envelope piece that produced the cuts, so rounding-level ties never flip the owner inside a piece. The
 *  ownership is a deterministic function of the replicated wall geometry and the chord, identical on every rank. */
inline SU2Native3D::MetricEdgeBreakpoints EdgeGeometry(std::shared_ptr<CBoundaryLayerMetric> layers, double core) {
  return [layers, core](SU2Native3D::Point a, SU2Native3D::Point b) {
    auto owners = std::make_shared<CBoundaryLayerMetric::ChordOwners>();
    SU2Native3D::EdgeGeometry geometry(layers->EdgeBreakpoints({a.x, a.y, a.z}, {b.x, b.y, b.z}, 1.5, 16, owners.get()));
    geometry.pinned = [layers, core, owners](long double t, SU2Native3D::Point p, SU2Native3D::Tensor sensor) {
      return Compose(*layers, core, p, sensor, owners.get(), t);
    };
    return geometry;
  };
}
}  // namespace SU2NativeBL3D
