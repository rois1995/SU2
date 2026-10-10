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
/*! Local ball topology and an unchanged oriented interface. Requires an embedded accepted source cavity.
 * Metric gates, external collisions, geometry references and distributed dependency completeness are separate. */
bool ValidateFixedInterface(const std::vector<Cell>& old, const std::vector<Cell>& fresh, std::string& reason,
                            KernelStats* stats = nullptr);
/*! Checks a caller-proved boundary edit against its prescribed oriented skin and ball topology.
 * Caller must establish physical geometry/marker and signed-volume closure; no arbitrary changed-skin certificate. */
bool ValidatePrescribedInterface(const std::vector<Cell>& old, const std::vector<Cell>& fresh,
                                 const std::vector<Face>& expected_skin, std::string& reason,
                                 KernelStats* stats = nullptr);
/*! Private insertion/movement/reconnection by coning the cavity skin to an apex. Does not commit a mesh.
 * Existing boundary apex faces are omitted; an outside/degenerate fan is rejected without changing fresh.
 * first_cell is a caller-reserved ID range; external ID collisions require the distributed directory. */
bool Cone(const std::vector<Cell>& old, Node apex, Id first_cell, std::vector<Cell>& fresh, std::string& reason,
          KernelStats* stats = nullptr);
}  // namespace SU2Native3D
