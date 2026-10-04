/*!
 * \file CTransferAdmissibility.cpp
 * \brief The admissibility predicate of the solution transfers.
 * \version 8.5.0 "Harrier"
 *
 * SU2 Project Website: https://su2code.github.io
 *
 * The SU2 Project is maintained by the SU2 Foundation
 * (http://su2foundation.org)
 *
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md)
 *
 * SU2 is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * SU2 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with SU2. If not, see <http://www.gnu.org/licenses/>.
 */

#include "../../include/adaptation/CTransferAdmissibility.hpp"

#include <cmath>

#include "../../include/adaptation/CBarycentricTransfer.hpp"
#include "../../../Common/include/adaptation/TransferTolerances.hpp"
#include "../../include/solvers/CTurbSolver.hpp"

namespace {
bool IsFinite(const su2double& value) { return std::isfinite(SU2_TYPE::GetValue(value)); }
}  // namespace

CTransferAdmissibility::CTransferAdmissibility(CFluidModel& fluidModel, unsigned short nDim,
                                               const CTurbSolver* turbSolver, bool sst)
    : fluidModel(&fluidModel),
      nDim(nDim),
      nVarFlow(nDim + 2),
      nVarTurb(turbSolver ? turbSolver->GetnVar() : 0),
      turbSolver(turbSolver),
      sst(sst && turbSolver != nullptr) {}

su2double CTransferAdmissibility::Lower(unsigned short iVar) const { return turbSolver->GetLowerLimit(iVar); }
su2double CTransferAdmissibility::Upper(unsigned short iVar) const { return turbSolver->GetUpperLimit(iVar); }

su2double CTransferAdmissibility::Bounded(unsigned short iVar, su2double value, unsigned long* counter) const {
  const su2double lower = Lower(iVar), upper = Upper(iVar);
  if (value < lower || value > upper) {
    const su2double limit = (value < lower) ? lower : upper;
    if (counter != nullptr && fabs(value - limit) > DiagnosticTol(1e-10) * fabs(limit)) (*counter)++;
    value = limit;
  }
  return value;
}

bool CTransferAdmissibility::Admissible(const su2double* flow, const su2double* turbulence) const {
  for (unsigned short iVar = 0; iVar < nVarTurb; ++iVar) {
    const su2double value = turbulence[iVar];
    if (!IsFinite(value)) return false;
    if (value < Lower(iVar) || value > Upper(iVar)) return false;
  }
  return CBarycentricTransfer::AdmissibleState(*fluidModel, nDim, flow, sst ? turbulence[0] : su2double(0.0));
}

bool CTransferAdmissibility::AdmissibleConservative(const su2double* fields) const {
  constexpr unsigned short kMaxTurb = 8;
  su2double turbulence[kMaxTurb] = {};
  const su2double density = fields[0];
  if (nVarTurb > 0 && sst) {
    if (!IsFinite(density) || !(density > 0.0)) return false;
  }
  for (unsigned short iVar = 0; iVar < nVarTurb; ++iVar)
    turbulence[iVar] = sst ? su2double(fields[nVarFlow + iVar] / density) : fields[nVarFlow + iVar];
  return Admissible(fields, turbulence);
}

bool CTransferAdmissibility::AdmissibleStage1(const su2double* fields) const {
  for (unsigned short iVar = 0; iVar < nVarFlow + nVarTurb; ++iVar)
    if (!IsFinite(fields[iVar])) return false;
  const su2double density = fields[0];
  if (!(density > 0.0)) return false;
  su2double k = 0.0;
  if (sst) {
    const su2double kLower = Lower(0);
    const su2double rhoK = fields[nVarFlow];
    const su2double floor = kLower * density;
    k = ((rhoK > floor) ? rhoK : floor) / density;
  }
  return CBarycentricTransfer::AdmissibleState(*fluidModel, nDim, fields, k);
}
