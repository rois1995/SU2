/*!
 * \file CNativeCurvedAtlas3D.hpp
 * \brief Transaction-local exact correspondence through certified convex PL sheet charts.
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#pragma once
#include "CNativeBoundary3D.hpp"
#include <memory>
#include <optional>

namespace SU2NativeGeometry {
/*! Fixed simultaneous remapping locals plus exact arithmetic arrays; SIZE_MAX on overflow.
 * Callers derive the integer-bit reservation from admitted scratch before preparing a transaction. */
size_t CurvedAtlasWorkspaceBytes(size_t maximum_integer_bits);
/*! Private construction data borrow the reference for one transaction. Original controls and
 * accepted correspondence never change. No synthetic refinement point becomes a volume node. */
class CurvedAtlas {
 public:
  struct Data;
  std::shared_ptr<const Data> data;
  size_t ResidentBytes()const;
  size_t ReferenceTriangleCount()const;
};
/*! moving_vertex releases a noncorner curve sample on each incident sheet's macroperimeter.
 * nullopt retains the accepted perimeter vertices. Holes/non-disks refuse. */
bool PrepareCurvedAtlas(const SU2Native3D::Patch&,const SU2Native3D::PlanarReference&,
                        std::optional<SU2Native3D::Id> moving_vertex,CurvedAtlas&,AtlasBudget&,std::string&);
/*! Quantize a proposal to an immutable forward binding, then map that actual binding exactly
 * into every incident sheet. new_node supplies only the new current identity. */
bool ProposeAtlasBinding(const CurvedAtlas&,SU2Native3D::Id old_vertex,SU2Native3D::Node new_node,
                         SU2Native3D::BoundaryPoint&,AtlasBudget&,std::string&);
/*! All candidate endpoints are actual forward bindings, including unchanged endpoints.
 * Produces local explicit records; replacement, sheet membership and shared traces are certified. */
bool RemapAtlasSurface(const CurvedAtlas&,const std::vector<SU2Native3D::SurfaceFace>&,
                       const SU2Native3D::BoundaryPoints&,BoundaryCoverage&,AtlasBudget&,std::string&);
}  // namespace SU2NativeGeometry
