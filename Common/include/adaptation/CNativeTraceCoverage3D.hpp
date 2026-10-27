/*!
 * \file CNativeTraceCoverage3D.hpp
 * \brief Fresh face correspondence from prescribed shared reference traces.
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#pragma once
#include "CNativeBoundaryParameters3D.hpp"

namespace SU2NativeGeometry {
/*! Shared traces are oriented by sorted current endpoint IDs and never modified.
 * A fresh constrained reference mesh is labelled by candidate face boundaries, then mapped
 * to each active simplex with its prescribed edge parameters. Failure preserves output. */
bool RebuildTraceCoverage(const MappedFacetReference&,const std::vector<CoveragePiece>& accepted_region,
    const std::vector<SU2Native3D::SurfaceFace>& candidate,const std::map<SU2Native3D::Id,SurfaceBinding>& bindings,
    const std::map<SU2Native3D::Edge,EdgeTrace>& traces,std::map<SU2Native3D::Face,FaceCoverage>& out,
    AtlasBudget&,std::string& reason);
}  // namespace SU2NativeGeometry
