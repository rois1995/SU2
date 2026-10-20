/*!
 * \file CIntegrationFactory.cpp
 * \brief Main subroutines for CIntegrationFactory .
 * \author T. Albring
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

#include "../../include/integration/CIntegrationFactory.hpp"
#include "../../include/integration/CSingleGridIntegration.hpp"
#include "../../include/integration/CMultiGridIntegration.hpp"
#include "../../include/integration/CNewtonLegacyIntegration.hpp"
#include "../../include/integration/CNewtonIntegration.hpp"
#include "../../include/integration/CStructuralIntegration.hpp"
#include "../../include/integration/CFEM_DG_Integration.hpp"

CIntegration** CIntegrationFactory::CreateIntegrationContainer(CConfig* config,
                                                               const CSolver* const* solver_container){
  SU2_ZONE_SCOPED

  auto **integration = new CIntegration* [MAX_SOLS]();

  for (unsigned int iSol = 0; iSol < MAX_SOLS; iSol++){
    if (solver_container[iSol] != nullptr){
      const SolverMetaData &solverInfo = CSolverFactory::GetSolverMeta(solver_container[iSol]);
      integration[iSol] = CreateIntegration(solverInfo.integrationType);
    }
  }

  /*--- Preserve legacy NEWTON_KRYLOV behavior unless the common equations are explicitly selected. ---*/
  const auto selected = config->GetNewtonEquations();
  std::shared_ptr<CNewtonEquationSet::ResidualWorkspace> workspace;
  if (!selected.empty()) workspace = std::make_shared<CNewtonEquationSet::ResidualWorkspace>();
  const auto options = config->GetNewtonOptions().Passive([](const su2double& x) { return SU2_TYPE::GetValue(x); });
  /*--- Native iteration order determines the group's first dispatch, independently of its packing order. ---*/
  const std::pair<unsigned short, const char*> order[] = {{RUNTIME_FLOW_SYS, "flow"}, {RUNTIME_TRANS_SYS, "trans"},
                                                         {RUNTIME_TURB_SYS, "turb"}, {RUNTIME_SPECIES_SYS, "species"}};
  /*--- An assembled coupled group leaves the transition model out: it is solved as its own group after the coupled
   *    group (loose coupling, Mosahebi and Laurendeau, AIAA J. 53(5), 2015). In one group its clipped variables
   *    share the line search and CFL, which held back the flow and turbulence updates. ---*/
  auto coupled = config->GetNewtonCoupled() ? selected : std::vector<unsigned short>{};
  const auto trans = std::find(coupled.begin(), coupled.end(), RUNTIME_TRANS_SYS);
  if (config->GetNewtonAssembled() && trans != coupled.end() && coupled.size() > 1) coupled.erase(trans);
  bool coupledCreated = false;
  for (const auto& equation : order) {
    const auto runtime = equation.first;
    if (std::find(selected.begin(), selected.end(), runtime) == selected.end()) continue;
    const auto slot = config->GetContainerPosition(runtime);
    if (!solver_container[slot]) SU2_MPI::Error("Unavailable equation in NEWTON_EQUATIONS", CURRENT_FUNCTION);
    const bool inCoupled = std::find(coupled.begin(), coupled.end(), runtime) != coupled.end();
    if (inCoupled && coupledCreated) continue;
    const auto group = inCoupled ? coupled : std::vector<unsigned short>{runtime};
    const bool flowControls = config->GetNewtonKrylov() &&
                              std::find(group.begin(), group.end(), RUNTIME_FLOW_SYS) != group.end();
    delete integration[slot];
    integration[slot] = new CNewtonIntegration(group, options, config->GetNewtonBlockLower(),
        inCoupled ? "newton_coupled" : std::string("newton_") + equation.second, flowControls,
        workspace, config->GetNewtonAssembled());
    coupledCreated = coupledCreated || inCoupled;
  }

  return integration;
}

CIntegration* CIntegrationFactory::CreateIntegration(INTEGRATION_TYPE integrationType){
  SU2_ZONE_SCOPED

  CIntegration *integration = nullptr;

  switch(integrationType){
    case INTEGRATION_TYPE::DEFAULT:
      integration = new CIntegration();
      break;
    case INTEGRATION_TYPE::SINGLEGRID:
      integration = new CSingleGridIntegration();
      break;
    case INTEGRATION_TYPE::MULTIGRID:
      integration = new CMultiGridIntegration();
      break;
    case INTEGRATION_TYPE::NEWTON:
      /*--- NEWTON_EQUATIONS is the opt-in for replacing this legacy flow integration. ---*/
      integration = new CNewtonLegacyIntegration();
      break;
    case INTEGRATION_TYPE::STRUCTURAL:
      integration = new CStructuralIntegration();
      break;
    case INTEGRATION_TYPE::FEM_DG:
      integration = new CFEM_DG_Integration();
      break;
    case INTEGRATION_TYPE::NONE:
      integration = nullptr;
      break;
  }

  return integration;

}
