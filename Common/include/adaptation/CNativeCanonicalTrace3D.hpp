/*!
 * \file CNativeCanonicalTrace3D.hpp
 * \brief History-independent surface paths through immutable original charts.
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#pragma once
#include "CNativeAffineReference.hpp"

namespace SU2NativeGeometry {
/*! Canonical oriented surface path from original authority and endpoint associations only.
 * Uses the first common original vertex-star component, otherwise one deterministic shortest
 * dual-facet strip, cut at protected edges. No alternate path after certificate/resource refusal.
 * Feature-curve paths and whole path-network/face embedding admission remain caller duties.
 * Failure preserves output; work/storage are shared with the caller's transaction reservation. */
bool CanonicalSurfaceTrace(const MappedFacetReference&,const MobilityPolicy&,Id sheet,
    const SurfaceBinding& first,const SurfaceBinding& last,EdgeTrace& oriented_first_to_last,
    AtlasBudget&,std::string& reason);
} // namespace SU2NativeGeometry
