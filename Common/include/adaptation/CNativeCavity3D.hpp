/*!
 * \file CNativeCavity3D.hpp
 * \brief Bounded geometric closure for native tetrahedral private proposals.
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
#pragma once
#include "CNativeTopology3D.hpp"
#include <string>

namespace SU2Native3D {
constexpr size_t MaximumCavityCells = 64;
constexpr size_t MaximumReplacementCells = 128;
/*! Throws unless bounded triangles form a connected manifold sphere/disk with valid vertex links.
 * Optional closure identifies a nonmanifold edge or pinched vertex whose full star must be admitted. */
void ValidateSurfaceTopology(const std::vector<Face>& triangles, bool closed,
                             size_t maximum = 4 * MaximumReplacementCells, Feature* closure = nullptr);
/*! Closed oriented manifold domain skin; components and genus are unrestricted.
 * Caller admits global reference storage and supplies an embedded accepted boundary. */
void ValidateDomainSurfaceTopology(const std::vector<Face>& triangles);
/*! Local ball topology and an unchanged oriented interface. Requires an embedded accepted source cavity.
 * Metric gates, external collisions, geometry references and distributed dependency completeness are separate.
 * Constructed private inputs may explicitly use the replacement bound; accepted-cavity admission stays64. */
bool ValidateFixedInterface(const std::vector<Cell>& old, const std::vector<Cell>& fresh, std::string& reason,
                            KernelStats* stats = nullptr, size_t maximum_old_cells = MaximumCavityCells);
/*! Checks a caller-proved boundary edit against its prescribed oriented skin and ball topology.
 * Caller must establish physical geometry/marker and signed-volume closure; no arbitrary changed-skin certificate. */
bool ValidatePrescribedInterface(const std::vector<Cell>& old, const std::vector<Cell>& fresh,
                                 const std::vector<Face>& expected_skin, std::string& reason,
                                 KernelStats* stats = nullptr, size_t maximum_old_cells = MaximumCavityCells);
/*! Private insertion/movement/reconnection by coning the cavity skin to an apex. Does not commit a mesh.
 * Existing boundary apex faces are omitted; an outside/degenerate fan is rejected without changing fresh.
 * first_cell is a caller-reserved ID range; external ID collisions require the distributed directory. */
bool Cone(const std::vector<Cell>& old, Node apex, Id first_cell, std::vector<Cell>& fresh, std::string& reason,
          KernelStats* stats = nullptr);
}  // namespace SU2Native3D
