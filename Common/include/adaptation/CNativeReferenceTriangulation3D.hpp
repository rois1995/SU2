/*!
 * \file CNativeReferenceTriangulation3D.hpp
 * \brief Fresh bounded reference triangulation from current constraints, independent of old interior cuts.
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#pragma once
#include "CNativeAffineReference.hpp"

namespace SU2NativeGeometry {
/*! Original chart corners, reduced accepted-region boundary, current bindings and retained
 * trace knots are the only vertices. Trace path/t records are borrowed and never modified.
 * Per-chart holes/components are classified exactly; the caller certifies the glued sheet disk.
 * Output reference triangles have identity active barycentrics; failure preserves output. */
bool RebuildReferenceTriangulation(const MappedFacetReference&,
    const std::vector<CoveragePiece>& accepted_region,const std::map<SU2Native3D::Id,SurfaceBinding>& current_bindings,
    const std::vector<EdgeTrace>& retained_traces,std::vector<CoveragePiece>& fresh,
    AtlasBudget&,std::string& reason);
}  // namespace SU2NativeGeometry
