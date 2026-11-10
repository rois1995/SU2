/*!
 * \file CEulerSolver.cpp
 * \brief Main subroutines for solving Finite-Volume Euler flow problems.
 * \author F. Palacios, T. Economon
 * \version 8.6.0 "Harrier"
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

#include <cstring>
#include "../../include/solvers/CEulerSolver.hpp"
#include "../../include/variables/CNSVariable.hpp"
#include "../../../Common/include/toolboxes/geometry_toolbox.hpp"
#include "../../../Common/include/toolboxes/printing_toolbox.hpp"
#include "../../../Common/include/toolboxes/p1_simplex.hpp"
#include "../../include/numerics/flow/p1_viscous_entropy.hpp"
#include "../../include/numerics/flow/p1_ale_entropy.hpp"
#include "../../include/numerics/flow/p1_positive_update.hpp"
#include "../../include/numerics/flow/p1_boundary_ghost.hpp"
#include "../../include/numerics/flow/jst_weak_wall.hpp"
#include "../../include/numerics/flow/jst_entropy.hpp"
#include "../../include/numerics/flow/p1_sst_matched.hpp"
#include "../../include/fluid/CIdealGas.hpp"
#include "../../include/fluid/CVanDerWaalsGas.hpp"
#include "../../include/fluid/CPengRobinson.hpp"
#include "../../include/fluid/CDataDrivenFluid.hpp"
#include "../../include/fluid/CCoolProp.hpp"
#include "../../include/numerics_simd/CNumericsSIMD.hpp"
#include "../../include/limiters/CLimiterDetails.hpp"
#include "../../include/limiters/computeLimiters.hpp"
#include "../../include/numerics/flow/tecno_adjoint.hpp"
#include "../../include/output/COutput.hpp"
#include "../../include/output/CTurboOutput.hpp"

CEulerSolver::CEulerSolver(CGeometry *geometry, CConfig *config,
                           unsigned short iMesh, const bool navier_stokes) :
  CFVMFlowSolverBase<CEulerVariable, ENUM_REGIME::COMPRESSIBLE>(*geometry, *config) {
  SU2_ZONE_SCOPED

  /*--- Based on the navier_stokes boolean, determine if this constructor is
   *    being called by itself, or by its derived class CNSSolver. ---*/
  string description;
  unsigned short nSecVar;
  if (navier_stokes) {
    description = "Navier-Stokes";
    nSecVar = 8;
  }
  else {
    description = "Euler";
    nSecVar = 2;
  }

  const auto nZone = geometry->GetnZone();
  const bool restart = (config->GetRestart() || config->GetRestart_Flow());
  const bool rans = (config->GetKind_Turb_Model() != TURB_MODEL::NONE);
  const auto direct_diff = config->GetDirectDiff();
  const bool dual_time = (config->GetTime_Marching() == TIME_MARCHING::DT_STEPPING_1ST) ||
                         (config->GetTime_Marching() == TIME_MARCHING::DT_STEPPING_2ND);
  const bool time_stepping = (config->GetTime_Marching() == TIME_MARCHING::TIME_STEPPING);
  const bool adjoint = config->GetContinuous_Adjoint() || config->GetDiscrete_Adjoint();
  const bool tkeNeeded = (rans && config->GetKind_Turb_Model() == TURB_MODEL::SST);

  int Unst_RestartIter = 0;
  unsigned long iPoint, iMarker, counter_local = 0, counter_global = 0;
  unsigned short iDim;
  su2double StaticEnergy, Density, Velocity2, Pressure, Temperature;

  /*--- A grid is defined as dynamic if there's rigid grid movement or grid deformation AND the problem is time domain ---*/
  dynamic_grid = config->GetDynamic_Grid();

  /*--- Store the multigrid level. ---*/
  MGLevel = iMesh;

  /*--- Check for a restart file to evaluate if there is a change in the angle of attack
   before computing all the non-dimesional quantities. ---*/

  if (restart && (iMesh == MESH_0) && nZone <= 1 && config->GetFixed_CL_Mode()) {

    /*--- Modify file name for a dual-time unsteady restart ---*/

    if (dual_time) {
      if (adjoint) Unst_RestartIter = SU2_TYPE::Int(config->GetUnst_AdjointIter())-1;
      else if (config->GetTime_Marching() == TIME_MARCHING::DT_STEPPING_1ST)
        Unst_RestartIter = SU2_TYPE::Int(config->GetRestart_Iter())-1;
      else Unst_RestartIter = SU2_TYPE::Int(config->GetRestart_Iter())-2;
    }

    /*--- Modify file name for a time stepping unsteady restart ---*/

    if (time_stepping) {
      if (adjoint) Unst_RestartIter = SU2_TYPE::Int(config->GetUnst_AdjointIter())-1;
      else Unst_RestartIter = SU2_TYPE::Int(config->GetRestart_Iter())-1;
    }

    /*--- Read and store the restart metadata. ---*/

    string filename_ = "flow";
    filename_ = config->GetFilename(filename_, ".meta", Unst_RestartIter);
    Read_SU2_Restart_Metadata(geometry, config, adjoint, filename_);

  }

  /*--- Set the gamma value ---*/

  Gamma = config->GetGamma();
  Gamma_Minus_One = Gamma - 1.0;

  /*--- Define geometry constants in the solver structure
   Compressible flow, primitive variables (T, vx, vy, vz, P, rho, h, c, lamMu, EddyMu, ThCond, Cp).
   ---*/

  nDim = geometry->GetnDim();

  nVar = nDim + 2;
  nPrimVar = nDim + 9;
  /*--- Centered schemes only need gradients for viscous fluxes (T and v). ---*/
  nPrimVarGrad = EulerNPrimVarGrad(config, nDim);
  nSecondaryVar = nSecVar;
  nAuxGradAdap = (config->GetGoal_Oriented_Metric())? nDim+3 : config->GetnAdap_Sensor();

  /*--- Initialize nVarGrad for deallocation ---*/

  nVarGrad = nPrimVarGrad;

  nMarker      = config->GetnMarker_All();
  nPoint       = geometry->GetnPoint();
  nPointDomain = geometry->GetnPointDomain();

  /*--- Store the number of vertices on each marker for deallocation later ---*/

  nVertex.resize(nMarker);
  for (iMarker = 0; iMarker < nMarker; iMarker++)
    nVertex[iMarker] = geometry->nVertex[iMarker];

  /*--- Perform the non-dimensionalization for the flow equations using the
   specified reference values. ---*/

  SetNondimensionalization(config, iMesh);

  /*--- Check if we are executing a verification case. If so, the
   VerificationSolution object will be instantiated for a particular
   option from the available library of verification solutions. Note
   that this is done after SetNondim(), as problem-specific initial
   parameters are needed by the solution constructors. ---*/

  SetVerificationSolution(nDim, nVar, config);

  /*--- Allocate base class members. ---*/

  Allocate(*config);

  /*--- MPI + OpenMP initialization. ---*/

  HybridParallelInitialization(*config, *geometry);

  /*--- Jacobians and vector structures for implicit computations ---*/

  if (config->GetKind_TimeIntScheme_Flow() == EULER_IMPLICIT) {

    if (rank == MASTER_NODE)
      cout << "Initialize Jacobian structure (" << description << "). MG level: " << iMesh <<"." << endl;

    Jacobian.Initialize(nPoint, nPointDomain, nVar, nVar, true, geometry, config, ReducerStrategy, true);
  }
  else {
    if (rank == MASTER_NODE)
      cout << "Explicit scheme. No Jacobian structure (" << description << "). MG level: " << iMesh <<"." << endl;
  }

  /*--- Store the value of the primitive variables + 2 turb variables at the boundaries,
   used for IO with a donor cell ---*/

  AllocVectorOfMatrices(nVertex, (rans? nPrimVar+2 : nPrimVar), DonorPrimVar);

  /*--- Store the value of the characteristic primitive variables index at the boundaries ---*/

  DonorGlobalIndex.resize(nMarker);
  for (iMarker = 0; iMarker < nMarker; iMarker++)
    DonorGlobalIndex[iMarker].resize(nVertex[iMarker],0);

  /*--- Actuator Disk Radius allocation ---*/
  ActDisk_R.resize(nMarker);

  /*--- Actuator Disk Center allocation ---*/
  ActDisk_C.resize(nMarker, MAXNDIM);

  /*--- Actuator Disk Axis allocation ---*/
  ActDisk_Axis.resize(nMarker, MAXNDIM);

  /*--- Actuator Disk Fa, Fx, Fy and Fz allocations ---*/
  AllocVectorOfVectors(nVertex, ActDisk_Fa);
  AllocVectorOfVectors(nVertex, ActDisk_Fx);
  AllocVectorOfVectors(nVertex, ActDisk_Fy);
  AllocVectorOfVectors(nVertex, ActDisk_Fz);

  /*--- Actuator Disk BEM Fa, Fx, Fy and Fz allocations ---*/

  AllocVectorOfVectors(nVertex, ActDisk_Fa_BEM);
  AllocVectorOfVectors(nVertex, ActDisk_Fx_BEM);
  AllocVectorOfVectors(nVertex, ActDisk_Fy_BEM);
  AllocVectorOfVectors(nVertex, ActDisk_Fz_BEM);

  /*--- Store the value of the Delta P at the Actuator Disk ---*/

  AllocVectorOfVectors(nVertex, ActDisk_DeltaP);

  /*--- Store the value of DeltaP_r, Thrust_r and Torque_r at the Actuator Disk for BEM ---*/

  AllocVectorOfVectors(nVertex, ActDisk_DeltaP_r);
  AllocVectorOfVectors(nVertex, ActDisk_Thrust_r);
  AllocVectorOfVectors(nVertex, ActDisk_Torque_r);

  /*--- Store the value of the Delta T at the Actuator Disk ---*/

  AllocVectorOfVectors(nVertex, ActDisk_DeltaT);

  /*--- Supersonic coefficients ---*/

  CEquivArea_Inv.resize(nMarker);

  /*--- Engine simulation ---*/

  Inflow_MassFlow.resize(nMarker);
  Inflow_Pressure.resize(nMarker);
  Inflow_Mach.resize(nMarker);
  Inflow_Area.resize(nMarker);

  Exhaust_Temperature.resize(nMarker);
  Exhaust_MassFlow.resize(nMarker);
  Exhaust_Pressure.resize(nMarker);
  Exhaust_Area.resize(nMarker);

  /*--- Turbomachinery simulation ---*/
  AverageMassFlowRate.resize(nMarker);

  /*--- Read farfield conditions from config ---*/

  Temperature_Inf = config->GetTemperature_FreeStreamND();
  Velocity_Inf = config->GetVelocity_FreeStreamND();
  Pressure_Inf = config->GetPressure_FreeStreamND();
  Density_Inf = config->GetDensity_FreeStreamND();
  Energy_Inf = config->GetEnergy_FreeStreamND();
  Mach_Inf = config->GetMach();
  const su2double TKE_Inf = config->GetTke_FreeStreamND();

  /*--- Initialize the secondary values for direct derivative approximations ---*/

  switch(direct_diff) {
    case NO_DERIVATIVE:
      /*--- Default ---*/
      break;
    case D_DENSITY:
      SU2_TYPE::SetDerivative(Density_Inf, 1.0);
      break;
    case D_PRESSURE:
      SU2_TYPE::SetDerivative(Pressure_Inf, 1.0);
      break;
    case D_TEMPERATURE:
      SU2_TYPE::SetDerivative(Temperature_Inf, 1.0);
      break;
    case D_MACH: case D_AOA:
    case D_SIDESLIP: case D_REYNOLDS:
    case D_TURB2LAM: case D_DESIGN:
      /*--- Already done in postprocessing of config ---*/
      break;
    default:
      break;
  }

  SetReferenceValues(*config);

  /*--- Initialize fan face pressure, fan face mach number, and mass flow rate ---*/

  for (iMarker = 0; iMarker < nMarker; iMarker++) {
    Inflow_MassFlow[iMarker]     = 0.0;
    Inflow_Mach[iMarker]         = Mach_Inf;
    Inflow_Pressure[iMarker]     = Pressure_Inf;
    Inflow_Area[iMarker]         = 0.0;

    Exhaust_MassFlow[iMarker]    = 0.0;
    Exhaust_Temperature[iMarker] = Temperature_Inf;
    Exhaust_Pressure[iMarker]    = Pressure_Inf;
    Exhaust_Area[iMarker]        = 0.0;

    AverageMassFlowRate[iMarker] = 0.0;
  }

  /*--- Initialize the solution to the far-field state everywhere. ---*/

  if (navier_stokes) {
    nodes = new CNSVariable(Density_Inf, Velocity_Inf, Energy_Inf, nPoint, nDim, nVar, config);
  } else {
    nodes = new CEulerVariable(Density_Inf, Velocity_Inf, Energy_Inf, nPoint, nDim, nVar, config);
  }
  SetBaseClassPointerToNodes();

  if (iMesh == MESH_0) {
    nodes->NonPhysicalEdgeCounter.resize(geometry->GetnEdge()) = 0;
    if (config->GetEdge_Form_P1() && config->GetEdge_Form_ES_Blend()) {
      nodes->EdgeBlendLambda.resize(geometry->GetnEdge()) = su2double(0.0);
      if (config->GetVisc_P1_Entropy_Moved()) P1MovedThetaV.resize(geometry->GetnElem()) = su2double(0.0);
    }
    if (config->GetJST_Weak_Wall_Diss()) {
      /*--- JST_WEAK_WALL_DISSIPATION: the weak no-slip walls of BC_WALL_ENTROPY_P1, flags on owned and halo nodes. ---*/
      auto isWeakWall = [&](unsigned short iMarker) {
        const auto kind = config->GetMarker_All_KindBC(iMarker);
        return (kind == HEAT_FLUX) || (kind == ISOTHERMAL);
      };
      JSTWeakWall::WallNodeFlags(*geometry, *config, isWeakWall, nodes->JSTWallNode);
    }
    if (config->GetEdge_Form_TeCNO_Adjoint()) {
      /*--- Simplex meshes only for now (P1_TECNO_CONVERGENCE.md, 5.1). ---*/
      unsigned long nonSimplex = 0, nonSimplexGlobal = 0;
      for (auto iElem = 0ul; iElem < geometry->GetnElem(); ++iElem) {
        nonSimplex += (geometry->elem[iElem]->GetnNodes() != nDim + 1);
      }
      SU2_MPI::Allreduce(&nonSimplex, &nonSimplexGlobal, 1, MPI_UNSIGNED_LONG, MPI_SUM, SU2_MPI::GetComm());
      if (nonSimplexGlobal > 0) {
        SU2_MPI::Error("EDGE_FORM_TECNO_ADJOINT: simplex meshes only (triangles, tetrahedra) for now.", CURRENT_FUNCTION);
      }
      nodes->TeCNOAdjGradient.resize(nPoint, nVar, nDim, 0.0);
      nodes->TeCNOAdjPhi.resize(nPoint) = su2double(1.0);
      /*--- TECNO_ADJOINT_LIMITER: B^T q of both edge ends (P1_TECNO_ADJ_LIMITER.md), else q. ---*/
      const bool edgeEnds = (config->GetKind_TeCNO_Adj_Limiter() != TECNO_ADJ_LIMITER::NONE);
      nodes->TeCNOAdjEdgeQ.resize(geometry->GetnEdge(), edgeEnds ? 2 * nVar : nVar) = su2double(0.0);
      if (config->GetKind_TeCNO_Adj_Limiter() == TECNO_ADJ_LIMITER::EDGE_ENTROPY_TVD) {
        /*--- EDGE_ENTROPY_TVD: ingredients, own clip, node factors and the per-edge-end values (TeCNOAdjointTVDLimiter). ---*/
        nodes->TeCNOAdjEdgeTVD.resize(geometry->GetnEdge(), TeCNOAdjoint::TVDWidth(nDim)) = su2double(0.0);
        nodes->TeCNOAdjEdgePhi.resize(geometry->GetnEdge(), 2 * TeCNOAdjoint::TVD_NFIELD) = su2double(1.0);
        TeCNOAdjTVDClip.resize(geometry->GetnEdge(), TeCNOAdjoint::TVD_CLIP_WIDTH) = su2double(0.0);
        TeCNOAdjTVDRho.resize(nPoint, TeCNOAdjoint::TVD_NFIELD) = su2double(1.0);
      }
      if (config->GetKind_TeCNO_Adj_Limiter() == TECNO_ADJ_LIMITER::EDGE_ENTROPY_FCT) {
        /*--- EDGE_ENTROPY_FCT: ingredients with the FCT columns and the store of phi_c, alpha, gamma (TeCNOAdjointTVDLimiter,
         * initial values of the full-ramp AD term: 1, 1, 0); the work arrays are sized with the setup. ---*/
        nodes->TeCNOAdjEdgeTVD.resize(geometry->GetnEdge(), TeCNOAdjoint::FCTWidth(nDim)) = su2double(0.0);
        nodes->TeCNOAdjEdgePhi.resize(geometry->GetnEdge(), TeCNOAdjoint::FCT_STORE_WIDTH) = su2double(1.0);
        for (auto iEdge = 0ul; iEdge < geometry->GetnEdge(); ++iEdge)
          for (auto f = 0u; f < TeCNOAdjoint::TVD_NFIELD; ++f) nodes->TeCNOAdjEdgePhi(iEdge, TeCNOAdjoint::FCT_GAMMA + f) = 0.0;
      }
      TeCNOAdjW.resize(nPoint, nVar) = su2double(0.0);
      TeCNOAdjY.resize(nPoint, nVar, nDim, 0.0);
      if (config->GetEdge_Form_TeCNO_Adjoint_Limiter()) {
        TeCNOAdjLimiter.resize(nPoint, nVar) = su2double(1.0);
        TeCNOAdjMin.resize(nPoint, nVar) = su2double(0.0);
        TeCNOAdjMax.resize(nPoint, nVar) = su2double(0.0);
      }
      if (config->GetEdge_Form_TeCNO_A4_Jacobian()) {
        nodes->TeCNOAdjEdgeD.resize(geometry->GetnEdge(), nVar * nVar) = su2double(0.0);
        TeCNOAdjP.resize(nPoint, nVar * nVar) = su2double(0.0);
      }
      if (config->GetKind_TeCNO_Adj_Jacobian() != TECNO_ADJ_JACOBIAN::ROE1) {
        nodes->TeCNOAdjEdgeD.resize(geometry->GetnEdge(), nVar * nVar) = su2double(0.0);
        TeCNOAdjP.resize(nPoint, nVar * nVar) = su2double(0.0);
        /*--- FIRST_RING: (s, 1/V, T); FIRST_RING_JUMPS: (1/V, E, T, M, margin). ---*/
        const bool jumps = (config->GetKind_TeCNO_Adj_Jacobian() == TECNO_ADJ_JACOBIAN::FIRST_RING_JUMPS);
        TeCNOAdjS.resize(nPoint, jumps ? 3 + 2 * nDim * nDim : nDim + 1 + nDim * nDim) = su2double(0.0);
      }
      if (config->GetKind_TeCNO_Adj_Gradient() == TECNO_ADJ_GRADIENT::P1_AVERAGE) {
        /*--- P1 average (P1_TECNO_ADJ_GRADIENT.md, G2): edge weights a and volumes Vhat, fixed (no grid motion). ---*/
        geometry->SetTeCNOAdjointP1Weights(TeCNOAdjEdgeA, TeCNOAdjVol);
      }
    }
  }

  /*--- Check that the initial solution is physical, report any non-physical nodes ---*/

  counter_local = 0;

  for (iPoint = 0; iPoint < nPoint; iPoint++) {

    Density = nodes->GetDensity(iPoint);

    Velocity2 = 0.0;
    for (iDim = 0; iDim < nDim; iDim++)
      Velocity2 += pow(nodes->GetSolution(iPoint,iDim+1)/Density,2);

    StaticEnergy= nodes->GetEnergy(iPoint) - 0.5*Velocity2;
    if (tkeNeeded) StaticEnergy -= TKE_Inf;

    GetFluidModel()->SetTDState_rhoe(Density, StaticEnergy);
    Pressure= GetFluidModel()->GetPressure();
    Temperature= GetFluidModel()->GetTemperature();

    /*--- Use the values at the infinity ---*/

    su2double Solution[MAXNVAR] = {0.0};
    if ((Pressure < 0.0) || (Density < 0.0) || (Temperature < 0.0)) {
      Solution[0] = Density_Inf;
      for (iDim = 0; iDim < nDim; iDim++)
        Solution[iDim+1] = Velocity_Inf[iDim]*Density_Inf;
      Solution[nDim+1] = Energy_Inf*Density_Inf;
      nodes->SetSolution(iPoint,Solution);
      nodes->SetSolution_Old(iPoint,Solution);
      counter_local++;
    }

  }

  /*--- Warning message about non-physical points ---*/

  if (config->GetComm_Level() == COMM_FULL) {

    SU2_MPI::Reduce(&counter_local, &counter_global, 1, MPI_UNSIGNED_LONG, MPI_SUM, MASTER_NODE, SU2_MPI::GetComm());

    if ((rank == MASTER_NODE) && (counter_global != 0))
      cout << "Warning. The original solution contains " << counter_global << " points that are not physical." << endl;
  }

  /*--- Initial comms. ---*/

  CommunicateInitialState(geometry, config);

  /*--- Sizing edge mass flux array, padded to a multiple of the SIMD size since the vectorized
   *    numerics write to it with contiguous (unmasked) SIMD stores, same as CEdge's Normal. ---*/
  /*--- Matched k of SST with the P1 options: the P1 element terms and the closure k rule are written for simplices. ---*/
  if (config->GetTke_Matched_P1() && !config->GetEdge_Form_P1_Mixed() && !config->GetSplit_Edge_Flux()) {
    unsigned long nonSimplex = 0;
    for (unsigned long iElem = 0; iElem < geometry->GetnElem(); ++iElem)
      nonSimplex += (geometry->elem[iElem]->GetnNodes() != nDim + 1);
    if (nonSimplex > 0)
      SU2_MPI::Error("The P1 options with SST (matched k) need a mesh of triangles or tetrahedra.", CURRENT_FUNCTION);
  }

  /*--- JST_ENTROPY_BUDGET (fine grid): edge coefficients of the kernel, node theta and the correction mass fluxes. ---*/
  if (config->GetJST_Entropy_Budget() && (MGLevel == MESH_0)) {
    nodes->JSTEdgeCoeff.resize(geometry->GetnEdge(), 2) = su2double(0.0);
    JSTTheta.resize(nPoint) = su2double(0.0);
    JSTLaplacian.resize(nPoint, nVar + 1) = su2double(0.0);
    /*--- N* of the matched term from the markers of CPhysicalGeometry::SetPhysicalBoundary, owners' flags on the halos,
     * so both ranks of an edge use the same chi (a halo node can lie on a boundary with no boundary face on this rank). ---*/
    auto isPhysical = [&](unsigned short iMarker) {
      const auto kind = config->GetMarker_All_KindBC(iMarker);
      return (kind != SEND_RECEIVE) && (kind != INTERNAL_BOUNDARY) && (kind != NEARFIELD_BOUNDARY) && (kind != PERIODIC_BOUNDARY);
    };
    JSTWeakWall::WallNodeFlags(*geometry, *config, isPhysical, JSTBoundaryNode);
    if (config->GetTke_Matched_P1()) JSTMassCorrection.resize(geometry->GetnEdge()) = su2double(0.0);
  }

  if (config->GetBounded_Scalar() || config->GetTke_Matched_P1()) {
    EdgeMassFluxes.resize(nextMultiple(geometry->GetnEdge(), simd::preferredLen<su2double>())) = su2double(0.0);
  }
  /*--- Matched k with MUSCL_TURB: the face k of each edge, same layout (SSTPATHS_PLAN.md 2). ---*/
  if (config->GetTke_Matched_P1() && config->GetMUSCL_Turb()) {
    nodes->EdgeMatchedKine.resize(nextMultiple(geometry->GetnEdge(), simd::preferredLen<su2double>())) = su2double(0.0);
  }

  /*--- Add the solver name.. ---*/
  SolverName = "C.FLOW";

  /*--- Finally, check that the static arrays will be large enough (keep this
   *    check at the bottom to make sure we consider the "final" values). ---*/
  if((nDim > MAXNDIM) || (nPrimVar > MAXNVAR) || (nSecondaryVar > MAXNVAR))
    SU2_MPI::Error("Oops! The CEulerSolver static array sizes are not large enough.",CURRENT_FUNCTION);
}

CEulerSolver::~CEulerSolver() {
  for(auto& model : FluidModel) delete model;
}

void CEulerSolver::InstantiateEdgeNumerics(const CSolver* const* solver_container, const CConfig* config) {
  SU2_ZONE_SCOPED

  BEGIN_SU2_OMP_SAFE_GLOBAL_ACCESS
  {

  if (config->Low_Mach_Correction())
    SU2_MPI::Error("Low-Mach correction is not supported with vectorization.", CURRENT_FUNCTION);

  if (solver_container[TURB_SOL])
    edgeNumerics = CNumericsSIMD::CreateNumerics(*config, nDim, MGLevel, solver_container[TURB_SOL]->GetNodes());
  else
    edgeNumerics = CNumericsSIMD::CreateNumerics(*config, nDim, MGLevel);

  if (!edgeNumerics)
    SU2_MPI::Error("The numerical scheme + gas model in use do not "
                   "support vectorization.", CURRENT_FUNCTION);

  }
  END_SU2_OMP_SAFE_GLOBAL_ACCESS
}

void CEulerSolver::InitTurboContainers(CGeometry *geometry, CConfig **config_container, unsigned short iZone){

  auto config = config_container[iZone];

  /*--- Initialize quantities for the average process for internal flow ---*/
  const auto nSpanWiseSections = config->GetnSpanWiseSections();

  AverageVelocity.resize(nMarker);
  AverageTurboVelocity.resize(nMarker);
  OldAverageTurboVelocity.resize(nMarker);
  AverageFlux.resize(nMarker);
  SpanTotalFlux.resize(nMarker);
  AveragePressure.resize(nMarker,nSpanWiseSections+1) = su2double(0.0);
  OldAveragePressure = AveragePressure;
  RadialEquilibriumPressure = AveragePressure;
  AverageDensity = AveragePressure;
  OldAverageDensity = AveragePressure;
  AverageNu = AveragePressure;
  AverageKine = AveragePressure;
  AverageOmega = AveragePressure;
  AverageRelTangVelocity = AveragePressure;

  for (unsigned long iMarker = 0; iMarker < nMarker; iMarker++) {
    AverageVelocity[iMarker].resize(nSpanWiseSections+1,nDim) = su2double(0.0);
    AverageTurboVelocity[iMarker].resize(nSpanWiseSections+1,nDim) = su2double(0.0);
    OldAverageTurboVelocity[iMarker].resize(nSpanWiseSections+1,nDim) = su2double(0.0);
    AverageFlux[iMarker].resize(nSpanWiseSections+1,nVar) = su2double(0.0);
    SpanTotalFlux[iMarker].resize(nSpanWiseSections+1,nVar) = su2double(0.0);
  }

  /*--- Initialize primitive quantities for turboperformace ---*/

  const auto nMarkerTurboPerf = config->GetnMarker_TurboPerformance();
  const auto nSpanMax = config->GetnSpanMaxAllZones();

  DensityIn.resize(nMarkerTurboPerf,nSpanMax+1) = su2double(0.0);
  PressureIn = DensityIn;
  TurboVelocityIn.resize(nMarkerTurboPerf);
  DensityOut = DensityIn;
  PressureOut = DensityIn;
  TurboVelocityOut.resize(nMarkerTurboPerf);
  KineIn = DensityIn;
  OmegaIn = DensityIn;
  NuIn = DensityIn;
  KineOut = DensityIn;
  OmegaOut = DensityIn;
  NuOut = DensityIn;
  RelTangVelocityIn = DensityIn;
  RelTangVelocityOut = DensityIn;

  for (unsigned long iMarker = 0; iMarker < nMarkerTurboPerf; iMarker++) {
    TurboVelocityIn[iMarker].resize(nSpanMax+1,nDim) = su2double(0.0);
    TurboVelocityOut[iMarker].resize(nSpanMax+1,nDim) = su2double(0.0);
  }

  /*--- Initialize quantities for NR BC ---*/

  if(config->GetBoolGiles()){

    CkInflow.resize(nMarker);
    CkOutflow1.resize(nMarker);
    CkOutflow2.resize(nMarker);

    for (unsigned long iMarker = 0; iMarker < nMarker; iMarker++) {
      CkInflow[iMarker].resize(nSpanWiseSections,2*geometry->GetnFreqSpanMax(INFLOW)+1) = complex<su2double>(0.0,0.0);
      CkOutflow1[iMarker].resize(nSpanWiseSections,2*geometry->GetnFreqSpanMax(OUTFLOW)+1) = complex<su2double>(0.0,0.0);
      CkOutflow2[iMarker] = CkOutflow1[iMarker];
    }
  }

  /*--- Initialise donor quantities ---*/
  ExtAverageDensity = su2double(0.0);
  ExtAveragePressure = su2double(0.0);
  ExtAverageTurboVelocity.resize(nDim);
  for (auto iDim = 0u; iDim < nDim; iDim++) ExtAverageTurboVelocity[iDim] = su2double(0.0);

  MixingState.resize(nMarker);

  for (auto iMarkerInt = 1; iMarkerInt < config->GetnMarker_MixingPlaneInterface()/2 + 1; iMarkerInt++) {
    auto iMarkerMP = config->FindMixingPlaneInterfaceMarker(geometry->GetnMarker(), iMarkerInt);
    if (iMarkerMP != -1) {
      MixingState[iMarkerMP].resize(nSpanWiseSections+1, nMixingStateVars) = su2double(0.0);
    }
  }

  TurbomachineryPerformance = std::make_shared<CTurboOutput>(config_container, *geometry, *GetFluidModel(), iZone);
}

void CEulerSolver::Set_MPI_ActDisk(CSolver **solver_container, CGeometry *geometry, CConfig *config) {
  SU2_ZONE_SCOPED

  unsigned long iter,  iPoint, iVertex, jVertex, iPointTotal,
  Buffer_Send_nPointTotal = 0;
  long iGlobalIndex, iGlobal;
  unsigned short iVar, iMarker, jMarker;
  long nDomain = 0, iDomain, jDomain;
  //bool ActDisk_Perimeter;
  bool rans = (config->GetKind_Turb_Model() != TURB_MODEL::NONE) && (solver_container[TURB_SOL] != nullptr);

  unsigned short nPrimVar_ = nPrimVar;
  if (rans) nPrimVar_ += 2; // Add two extra variables for the turbulence.

  /*--- MPI status and request arrays for non-blocking communications ---*/

  SU2_MPI::Status status;
  SU2_MPI::Request req;

  /*--- Define buffer vector interior domain ---*/

  su2double *Buffer_Send_PrimVar = nullptr;
  long      *Buffer_Send_Data    = nullptr;

  auto *nPointTotal_s = new unsigned long[size];
  auto *nPointTotal_r = new unsigned long[size];
  auto *iPrimVar = new su2double [nPrimVar_];

  unsigned long Buffer_Size_PrimVar = 0;
  unsigned long Buffer_Size_Data    = 0;

  unsigned long PointTotal_Counter = 0;

  /*--- Allocate the memory that we only need if we have MPI support ---*/

  su2double *Buffer_Receive_PrimVar = nullptr;
  long      *Buffer_Receive_Data    = nullptr;

  /*--- Basic dimensionalization ---*/

  nDomain = size;

  /*--- This loop gets the array sizes of points for each
   rank to send to each other rank. ---*/

  for (iDomain = 0; iDomain < nDomain; iDomain++) {

    /*--- Loop over the markers to perform the dimensionalizaton
     of the domain variables ---*/

    Buffer_Send_nPointTotal = 0;

    /*--- Loop over all of the markers and count the number of each
     type of point and element that needs to be sent. ---*/

    for (iMarker = 0; iMarker < config->GetnMarker_All(); iMarker++) {
      if ((config->GetMarker_All_KindBC(iMarker) == ACTDISK_INLET) ||
          (config->GetMarker_All_KindBC(iMarker) == ACTDISK_OUTLET)) {
        for (iVertex = 0; iVertex < geometry->nVertex[iMarker]; iVertex++) {
          //ActDisk_Perimeter = geometry->vertex[iMarker][iVertex]->GetActDisk_Perimeter();
          iPoint = geometry->vertex[iMarker][iVertex]->GetNode();
          jDomain = geometry->vertex[iMarker][iVertex]->GetDonorProcessor();
//          if ((iDomain == jDomain) && (geometry->nodes->GetDomain(iPoint)) && (!ActDisk_Perimeter)) {
          if ((iDomain == jDomain) && (geometry->nodes->GetDomain(iPoint))) {
            Buffer_Send_nPointTotal++;
          }
        }
      }
    }

    /*--- Store the counts on a partition by partition basis. ---*/

    nPointTotal_s[iDomain] = Buffer_Send_nPointTotal;

    /*--- Total counts for allocating send buffers below ---*/

    Buffer_Size_PrimVar += nPointTotal_s[iDomain]*(nPrimVar_);
    Buffer_Size_Data += nPointTotal_s[iDomain]*(3);

  }

  /*--- Allocate the buffer vectors in the appropiate domain (master, iDomain) ---*/

  Buffer_Send_PrimVar = new su2double[Buffer_Size_PrimVar];
  Buffer_Send_Data    = new long[Buffer_Size_Data];

  /*--- Now that we know the sizes of the point, we can
   allocate and send the information in large chunks to all processors. ---*/

  for (iDomain = 0; iDomain < nDomain; iDomain++) {

    /*--- A rank does not communicate with itself through MPI ---*/

    if (rank != iDomain) {

      /*--- Communicate the counts to iDomain with non-blocking sends ---*/

      SU2_MPI::Isend(&nPointTotal_s[iDomain], 1, MPI_UNSIGNED_LONG, iDomain, iDomain, SU2_MPI::GetComm(), &req);
      SU2_MPI::Request_free(&req);

    } else {

      /*--- If iDomain = rank, we simply copy values into place in memory ---*/

      nPointTotal_r[iDomain] = nPointTotal_s[iDomain];

    }

    /*--- Receive the counts. All processors are sending their counters to
     iDomain up above, so only iDomain needs to perform the recv here from
     all other ranks. ---*/

    if (rank == iDomain) {

      for (jDomain = 0; jDomain < size; jDomain++) {

        /*--- A rank does not communicate with itself through MPI ---*/

        if (rank != jDomain) {

          /*--- Recv the data by probing for the current sender, jDomain,
           first and then receiving the values from it. ---*/

          SU2_MPI::Recv(&nPointTotal_r[jDomain], 1, MPI_UNSIGNED_LONG, jDomain, rank, SU2_MPI::GetComm(), &status);

        }
      }

    }
  }

  /*--- Wait for the non-blocking sends to complete. ---*/

  SU2_MPI::Barrier(SU2_MPI::GetComm());

  /*--- Initialize the counters for the larger send buffers (by domain) ---*/

  PointTotal_Counter  = 0;

  for (iDomain = 0; iDomain < nDomain; iDomain++) {

    /*--- Set the value of the interior geometry. Initialize counters. ---*/

    iPointTotal = 0;

    /*--- Load up the actual values into the buffers for sending. ---*/

    for (iMarker = 0; iMarker < config->GetnMarker_All(); iMarker++) {

      if ((config->GetMarker_All_KindBC(iMarker) == ACTDISK_INLET) ||
          (config->GetMarker_All_KindBC(iMarker) == ACTDISK_OUTLET)) {
        for (iVertex = 0; iVertex < geometry->nVertex[iMarker]; iVertex++) {
          iPoint = geometry->vertex[iMarker][iVertex]->GetNode();

          jDomain = geometry->vertex[iMarker][iVertex]->GetDonorProcessor();
          //ActDisk_Perimeter = geometry->vertex[iMarker][iVertex]->GetActDisk_Perimeter();

//          if ((iDomain == jDomain) && (geometry->nodes->GetDomain(iPoint)) && (!ActDisk_Perimeter)) {
          if ((iDomain == jDomain) && (geometry->nodes->GetDomain(iPoint))) {

            for (iVar = 0; iVar < nPrimVar; iVar++) {
              Buffer_Send_PrimVar[(nPrimVar_)*(PointTotal_Counter+iPointTotal)+iVar] = nodes->GetPrimitive(iPoint,iVar);
            }
            if (rans) {
              Buffer_Send_PrimVar[(nPrimVar_)*(PointTotal_Counter+iPointTotal)+nPrimVar] = solver_container[TURB_SOL]->GetNodes()->GetSolution(iPoint,0);
              Buffer_Send_PrimVar[(nPrimVar_)*(PointTotal_Counter+iPointTotal)+(nPrimVar+1)] = 0.0;
            }

            iGlobalIndex = geometry->nodes->GetGlobalIndex(iPoint);
            jVertex = geometry->vertex[iMarker][iVertex]->GetDonorVertex();
            jMarker = geometry->vertex[iMarker][iVertex]->GetDonorMarker();

            Buffer_Send_Data[(3)*(PointTotal_Counter+iPointTotal)+(0)]  = iGlobalIndex;
            Buffer_Send_Data[(3)*(PointTotal_Counter+iPointTotal)+(1)] = jVertex;
            Buffer_Send_Data[(3)*(PointTotal_Counter+iPointTotal)+(2)]  = jMarker;

            iPointTotal++;

          }

        }

      }

    }

    /*--- Send the buffers with the geometrical information ---*/

    if (iDomain != rank) {

      /*--- Communicate the coordinates, global index, colors, and element
       date to iDomain with non-blocking sends. ---*/

      SU2_MPI::Isend(&Buffer_Send_PrimVar[PointTotal_Counter*(nPrimVar_)],
                     nPointTotal_s[iDomain]*(nPrimVar_), MPI_DOUBLE, iDomain,
                     iDomain,  SU2_MPI::GetComm(), &req);
      SU2_MPI::Request_free(&req);

      SU2_MPI::Isend(&Buffer_Send_Data[PointTotal_Counter*(3)],
                     nPointTotal_s[iDomain]*(3), MPI_LONG, iDomain,
                     iDomain+nDomain,  SU2_MPI::GetComm(), &req);
      SU2_MPI::Request_free(&req);
    }

    else {

      /*--- Allocate local memory for the local recv of the elements ---*/

      Buffer_Receive_PrimVar            = new su2double[nPointTotal_s[iDomain]*(nPrimVar_)];
      Buffer_Receive_Data               = new long[nPointTotal_s[iDomain]*(3)];

      for (iter = 0; iter < nPointTotal_s[iDomain]*(nPrimVar_); iter++)
        Buffer_Receive_PrimVar[iter] = Buffer_Send_PrimVar[PointTotal_Counter*(nPrimVar_)+iter];

      for (iter = 0; iter < nPointTotal_s[iDomain]*(3); iter++)
        Buffer_Receive_Data[iter] = Buffer_Send_Data[PointTotal_Counter*(3)+iter];


      /*--- Recv the point data from ourselves (same procedure as above) ---*/

      for (iPoint = 0; iPoint < nPointTotal_r[iDomain]; iPoint++) {

        for (iVar = 0; iVar < nPrimVar_; iVar++)
          iPrimVar[iVar] = Buffer_Receive_PrimVar[iPoint*(nPrimVar_)+iVar];

        iGlobal       =  Buffer_Receive_Data[iPoint*(3)+(0)];
        iVertex      =  Buffer_Receive_Data[iPoint*(3)+(1)];
        iMarker      = Buffer_Receive_Data[iPoint*(3)+(2)];

        for (iVar = 0; iVar < nPrimVar_; iVar++)
          DonorPrimVar[iMarker][iVertex][iVar] = iPrimVar[iVar];

        SetDonorGlobalIndex(iMarker, iVertex, iGlobal);

      }

      /*--- Delete memory for recv the point stuff ---*/

      delete [] Buffer_Receive_PrimVar;
      delete [] Buffer_Receive_Data;

    }

    /*--- Increment the counters for the send buffers (iDomain loop) ---*/

    PointTotal_Counter += iPointTotal;

  }

  /*--- Wait for the non-blocking sends to complete. ---*/

  SU2_MPI::Barrier(SU2_MPI::GetComm());

  /*--- The next section begins the recv of all data for the interior
   points/elements in the mesh. First, create the domain structures for
   the points on this rank. First, we recv all of the point data ---*/

  for (iDomain = 0; iDomain < size; iDomain++) {

    if (rank != iDomain) {

#ifdef HAVE_MPI

      /*--- Allocate the receive buffer vector. Send the colors so that we
       know whether what we recv is an owned or halo node. ---*/

      Buffer_Receive_PrimVar            = new su2double [nPointTotal_r[iDomain]*(nPrimVar_)];
      Buffer_Receive_Data               = new long [nPointTotal_r[iDomain]*(3)];

      /*--- Receive the buffers with the coords, global index, and colors ---*/

      SU2_MPI::Recv(Buffer_Receive_PrimVar, nPointTotal_r[iDomain]*(nPrimVar_) , MPI_DOUBLE,
                    iDomain, rank, SU2_MPI::GetComm(), &status);

      SU2_MPI::Recv(Buffer_Receive_Data, nPointTotal_r[iDomain]*(3) , MPI_LONG,
                    iDomain, rank+nDomain, SU2_MPI::GetComm(), &status);

      /*--- Loop over all of the points that we have recv'd and store the
       coords, global index vertex and markers ---*/

      for (iPoint = 0; iPoint < nPointTotal_r[iDomain]; iPoint++) {

        iGlobal      = Buffer_Receive_Data[iPoint*(3)+(0)];
        iVertex      = Buffer_Receive_Data[iPoint*(3)+(1)];
        iMarker      = Buffer_Receive_Data[iPoint*(3)+(2)];

        for (iVar = 0; iVar < nPrimVar_; iVar++)
          iPrimVar[iVar] = Buffer_Receive_PrimVar[iPoint*(nPrimVar_)+iVar];

        for (iVar = 0; iVar < nPrimVar_; iVar++) {
          DonorPrimVar[iMarker][iVertex][iVar] = iPrimVar[iVar];
        }

        SetDonorGlobalIndex(iMarker, iVertex, iGlobal);

      }

      /*--- Delete memory for recv the point stuff ---*/

      delete [] Buffer_Receive_PrimVar;
      delete [] Buffer_Receive_Data;

#endif

    }

  }

  /*--- Wait for the non-blocking sends to complete. ---*/

  SU2_MPI::Barrier(SU2_MPI::GetComm());

  /*--- Free all of the memory used for communicating points and elements ---*/

  delete[] Buffer_Send_PrimVar;
  delete[] Buffer_Send_Data;

  /*--- Release all of the temporary memory ---*/

  delete [] nPointTotal_s;
  delete [] nPointTotal_r;
  delete [] iPrimVar;

}

void CEulerSolver::SetNondimensionalization(CConfig *config, unsigned short iMesh) {
  SU2_ZONE_SCOPED

  su2double Temperature_FreeStream = 0.0, Mach2Vel_FreeStream = 0.0, ModVel_FreeStream = 0.0,
  Energy_FreeStream = 0.0, ModVel_FreeStreamND = 0.0, Velocity_Reynolds = 0.0,
  Omega_FreeStream = 0.0, Omega_FreeStreamND = 0.0, Viscosity_FreeStream = 0.0,
  Thermal_Conductivity_FreeStream = 0.0, SpecificHeat_Cp_FreeStream = 0.0,
  Density_FreeStream = 0.0, Pressure_FreeStream = 0.0, Tke_FreeStream = 0.0, Re_ThetaT_FreeStream = 0.0,
  Length_Ref = 0.0, Density_Ref = 0.0, Pressure_Ref = 0.0, Velocity_Ref = 0.0,
  Temperature_Ref = 0.0, Time_Ref = 0.0, Omega_Ref = 0.0, Force_Ref = 0.0,
  Gas_Constant_Ref = 0.0, Viscosity_Ref = 0.0, Conductivity_Ref = 0.0, Energy_Ref= 0.0,
  Froude = 0.0, Pressure_FreeStreamND = 0.0, Density_FreeStreamND = 0.0,
  Temperature_FreeStreamND = 0.0, Gas_ConstantND = 0.0,
  Velocity_FreeStreamND[3] = {0.0, 0.0, 0.0}, Viscosity_FreeStreamND = 0.0,
  Thermal_Conductivity_FreeStreamND = 0.0, SpecificHeat_Cp_FreeStreamND = 0.0,
  Tke_FreeStreamND = 0.0, Energy_FreeStreamND = 0.0,
  Total_UnstTimeND = 0.0, Delta_UnstTimeND = 0.0, TgammaR = 0.0, Heat_Flux_Ref = 0.0;

  unsigned short iDim;

  /*--- Local variables ---*/

  su2double Alpha         = config->GetAoA()*PI_NUMBER/180.0;
  su2double Beta          = config->GetAoS()*PI_NUMBER/180.0;
  su2double Mach          = config->GetMach();
  su2double Reynolds      = config->GetReynolds();
  bool unsteady           = (config->GetTime_Marching() != TIME_MARCHING::STEADY);
  bool viscous            = config->GetViscous();
  bool gravity            = config->GetGravityForce();
  bool turbulent          = (config->GetKind_Turb_Model() != TURB_MODEL::NONE);
  bool tkeNeeded          = (turbulent && config->GetKind_Turb_Model() == TURB_MODEL::SST);
  bool free_stream_temp   = (config->GetKind_FreeStreamOption() == FREESTREAM_OPTION::TEMPERATURE_FS);
  bool reynolds_init      = (config->GetKind_InitOption() == REYNOLDS);
  bool aeroelastic        = config->GetAeroelastic_Simulation();

  /*--- Set temperature via the flutter speed index ---*/
  if (aeroelastic) {
    su2double vf             = config->GetAeroelastic_Flutter_Speed_Index();
    su2double w_alpha        = config->GetAeroelastic_Frequency_Pitch();
    su2double b              = config->GetLength_Reynolds()/2.0; // airfoil semichord, Reynolds length is by defaul 1.0
    su2double mu             = config->GetAeroelastic_Airfoil_Mass_Ratio();
    // The temperature times gamma times the gas constant. Depending on the FluidModel temp is calculated below.
    TgammaR = ((vf*vf)*(b*b)*(w_alpha*w_alpha)*mu) / (Mach*Mach);
  }

  /*--- Compressible non dimensionalization ---*/

  /*--- Compute the Free Stream velocity, using the Mach number ---*/

  Pressure_FreeStream = config->GetPressure_FreeStream();
  Density_FreeStream  = config->GetDensity_FreeStream();
  Temperature_FreeStream = config->GetTemperature_FreeStream();

  /*--- The dimensional viscosity is needed to determine the free-stream conditions.
        To accomplish this, simply set the non-dimensional coefficients to the
        dimensional ones. This will be overruled later.---*/

  config->SetTemperature_Ref(1.0);
  config->SetViscosity_Ref(1.0);
  config->SetConductivity_Ref(1.0);

  CFluidModel* auxFluidModel = nullptr;

  switch (config->GetKind_FluidModel()) {

    case STANDARD_AIR:

      switch (config->GetSystemMeasurements()) {
        case SI: config->SetGas_Constant(287.058); break;
        case US: config->SetGas_Constant(1716.49); break;
      }

      auxFluidModel = new CIdealGas(1.4, config->GetGas_Constant());

      if (free_stream_temp && aeroelastic) {
        Temperature_FreeStream = TgammaR / (config->GetGas_Constant()*1.4);
        config->SetTemperature_FreeStream(Temperature_FreeStream);
      }
      break;

    case IDEAL_GAS:

      auxFluidModel = new CIdealGas(Gamma, config->GetGas_Constant());
      break;

    case VW_GAS:

      auxFluidModel = new CVanDerWaalsGas(Gamma, config->GetGas_Constant(),
                 config->GetPressure_Critical(), config->GetTemperature_Critical());
      break;

    case PR_GAS:

      auxFluidModel = new CPengRobinson(Gamma, config->GetGas_Constant(), config->GetPressure_Critical(),
                                        config->GetTemperature_Critical(), config->GetAcentric_Factor());
      break;

    case DATADRIVEN_FLUID:

      auxFluidModel = new CDataDrivenFluid(config);

      break;
    case COOLPROP:

      auxFluidModel = new CCoolProp(config->GetFluid_Name());
      break;

    default:
      SU2_MPI::Error("Unknown fluid model.", CURRENT_FUNCTION);
      break;
  }

  if (free_stream_temp) {
    auxFluidModel->SetTDState_PT(Pressure_FreeStream, Temperature_FreeStream);
    Density_FreeStream = auxFluidModel->GetDensity();
    config->SetDensity_FreeStream(Density_FreeStream);
  }
  else {
    auxFluidModel->SetTDState_Prho(Pressure_FreeStream, Density_FreeStream );
    Temperature_FreeStream = auxFluidModel->GetTemperature();
    config->SetTemperature_FreeStream(Temperature_FreeStream);
  }

  Mach2Vel_FreeStream = auxFluidModel->GetSoundSpeed();

  /*--- Compute the Free Stream velocity, using the Mach number ---*/

  if (nDim == 2) {
    config->GetVelocity_FreeStream()[0] = cos(Alpha)*Mach*Mach2Vel_FreeStream;
    config->GetVelocity_FreeStream()[1] = sin(Alpha)*Mach*Mach2Vel_FreeStream;
  }
  if (nDim == 3) {
    config->GetVelocity_FreeStream()[0] = cos(Alpha)*cos(Beta)*Mach*Mach2Vel_FreeStream;
    config->GetVelocity_FreeStream()[1] = sin(Beta)*Mach*Mach2Vel_FreeStream;
    config->GetVelocity_FreeStream()[2] = sin(Alpha)*cos(Beta)*Mach*Mach2Vel_FreeStream;
  }

  /*--- Compute the modulus of the free stream velocity ---*/

  ModVel_FreeStream = 0.0;
  for (iDim = 0; iDim < nDim; iDim++)
    ModVel_FreeStream += config->GetVelocity_FreeStream()[iDim]*config->GetVelocity_FreeStream()[iDim];
  ModVel_FreeStream = sqrt(ModVel_FreeStream); config->SetModVel_FreeStream(ModVel_FreeStream);

  /*--- Viscous initialization ---*/

  if (viscous) {

    /*--- Check if there is mesh motion. If yes, use the Mach
       number relative to the body to initialize the flow. ---*/

    if (dynamic_grid) Velocity_Reynolds = config->GetMach_Motion()*Mach2Vel_FreeStream;
    else Velocity_Reynolds = ModVel_FreeStream;

    /*--- Reynolds based initialization ---*/

    if (reynolds_init) {

      /*--- For viscous flows, pressure will be computed from a density
            that is found from the Reynolds number. The viscosity is computed
            from the dimensional version of Sutherland's law or the constant
            viscosity, depending on the input option.---*/

      auxFluidModel->SetLaminarViscosityModel(config);
      auxFluidModel->SetThermalConductivityModel(config);

      Viscosity_FreeStream = auxFluidModel->GetLaminarViscosity();
      Thermal_Conductivity_FreeStream = auxFluidModel->GetThermalConductivity();
      SpecificHeat_Cp_FreeStream = auxFluidModel->GetCp();

      config->SetViscosity_FreeStream(Viscosity_FreeStream);
      config->SetThermalConductivity_FreeStream(Thermal_Conductivity_FreeStream);
      config->SetSpecificHeatCp_FreeStream(SpecificHeat_Cp_FreeStream);

      Density_FreeStream = Reynolds*Viscosity_FreeStream/(Velocity_Reynolds*config->GetLength_Reynolds());
      config->SetDensity_FreeStream(Density_FreeStream);
      auxFluidModel->SetTDState_rhoT(Density_FreeStream, Temperature_FreeStream);
      Pressure_FreeStream = auxFluidModel->GetPressure();
      config->SetPressure_FreeStream(Pressure_FreeStream);
      Energy_FreeStream = auxFluidModel->GetStaticEnergy() + 0.5*ModVel_FreeStream*ModVel_FreeStream;

    }

    /*--- Thermodynamics quantities based initialization ---*/

    else {

      auxFluidModel->SetLaminarViscosityModel(config);
      auxFluidModel->SetThermalConductivityModel(config);
      Viscosity_FreeStream = auxFluidModel->GetLaminarViscosity();
      Thermal_Conductivity_FreeStream = auxFluidModel->GetThermalConductivity();
      SpecificHeat_Cp_FreeStream = auxFluidModel->GetCp();
      config->SetViscosity_FreeStream(Viscosity_FreeStream);
      config->SetThermalConductivity_FreeStream(Thermal_Conductivity_FreeStream);
      config->SetSpecificHeatCp_FreeStream(SpecificHeat_Cp_FreeStream);
      Energy_FreeStream = auxFluidModel->GetStaticEnergy() + 0.5*ModVel_FreeStream*ModVel_FreeStream;

      /*--- Compute Reynolds number ---*/
      Reynolds = (Density_FreeStream*Velocity_Reynolds*config->GetLength_Reynolds())/Viscosity_FreeStream;
      config->SetReynolds(Reynolds);
    }

    /*--- Turbulence kinetic energy ---*/

    Tke_FreeStream  = 3.0/2.0*(ModVel_FreeStream*ModVel_FreeStream*config->GetTurbulenceIntensity_FreeStream()*config->GetTurbulenceIntensity_FreeStream());

  }
  else {

    /*--- For inviscid flow, energy is calculated from the specified
       FreeStream quantities using the proper gas law. ---*/

    Energy_FreeStream = auxFluidModel->GetStaticEnergy() + 0.5*ModVel_FreeStream*ModVel_FreeStream;

  }

  /*-- Compute the freestream energy. ---*/

  if (tkeNeeded) { Energy_FreeStream += Tke_FreeStream; }; config->SetEnergy_FreeStream(Energy_FreeStream);

  /*--- Compute non dimensional quantities. By definition,
     Lref is one because we have converted the grid to meters. ---*/

  if (config->GetRef_NonDim() == DIMENSIONAL) {
    Pressure_Ref      = 1.0;
    Density_Ref       = 1.0;
    Temperature_Ref   = 1.0;
  }
  else if (config->GetRef_NonDim() == FREESTREAM_PRESS_EQ_ONE) {
    Pressure_Ref      = Pressure_FreeStream;     // Pressure_FreeStream = 1.0
    Density_Ref       = Density_FreeStream;      // Density_FreeStream = 1.0
    Temperature_Ref   = Temperature_FreeStream;  // Temperature_FreeStream = 1.0
  }
  else if (config->GetRef_NonDim() == FREESTREAM_VEL_EQ_MACH) {
    Pressure_Ref      = Gamma*Pressure_FreeStream; // Pressure_FreeStream = 1.0/Gamma
    Density_Ref       = Density_FreeStream;        // Density_FreeStream = 1.0
    Temperature_Ref   = Temperature_FreeStream;    // Temp_FreeStream = 1.0
  }
  else if (config->GetRef_NonDim() == FREESTREAM_VEL_EQ_ONE) {
    Pressure_Ref      = Mach*Mach*Gamma*Pressure_FreeStream; // Pressure_FreeStream = 1.0/(Gamma*(M_inf)^2)
    Density_Ref       = Density_FreeStream;        // Density_FreeStream = 1.0
    Temperature_Ref   = Temperature_FreeStream;    // Temp_FreeStream = 1.0
  }
  config->SetPressure_Ref(Pressure_Ref);
  config->SetDensity_Ref(Density_Ref);
  config->SetTemperature_Ref(Temperature_Ref);

  Length_Ref        = 1.0;                                                         config->SetLength_Ref(Length_Ref);
  Velocity_Ref      = sqrt(config->GetPressure_Ref()/config->GetDensity_Ref());    config->SetVelocity_Ref(Velocity_Ref);
  Time_Ref          = Length_Ref/Velocity_Ref;                                     config->SetTime_Ref(Time_Ref);
  Omega_Ref         = Velocity_Ref/Length_Ref;                                     config->SetOmega_Ref(Omega_Ref);
  Force_Ref         = config->GetDensity_Ref()*Velocity_Ref*Velocity_Ref*Length_Ref*Length_Ref; config->SetForce_Ref(Force_Ref);
  Heat_Flux_Ref     = Density_Ref*Velocity_Ref*Velocity_Ref*Velocity_Ref;           config->SetHeat_Flux_Ref(Heat_Flux_Ref);
  Gas_Constant_Ref  = Velocity_Ref*Velocity_Ref/config->GetTemperature_Ref();      config->SetGas_Constant_Ref(Gas_Constant_Ref);
  Viscosity_Ref     = config->GetDensity_Ref()*Velocity_Ref*Length_Ref;            config->SetViscosity_Ref(Viscosity_Ref);
  Conductivity_Ref  = Viscosity_Ref*Gas_Constant_Ref;                              config->SetConductivity_Ref(Conductivity_Ref);
  Froude            = ModVel_FreeStream/sqrt(STANDARD_GRAVITY*Length_Ref);         config->SetFroude(Froude);

  /*--- Divide by reference values, to compute the non-dimensional free-stream values ---*/

  Pressure_FreeStreamND = Pressure_FreeStream/config->GetPressure_Ref();  config->SetPressure_FreeStreamND(Pressure_FreeStreamND);
  Density_FreeStreamND  = Density_FreeStream/config->GetDensity_Ref();    config->SetDensity_FreeStreamND(Density_FreeStreamND);

  for (iDim = 0; iDim < nDim; iDim++) {
    Velocity_FreeStreamND[iDim] = config->GetVelocity_FreeStream()[iDim]/Velocity_Ref; config->SetVelocity_FreeStreamND(Velocity_FreeStreamND[iDim], iDim);
  }

  Temperature_FreeStreamND = Temperature_FreeStream/config->GetTemperature_Ref(); config->SetTemperature_FreeStreamND(Temperature_FreeStreamND);

  Gas_ConstantND = config->GetGas_Constant()/Gas_Constant_Ref;    config->SetGas_ConstantND(Gas_ConstantND);


  ModVel_FreeStreamND = 0.0;
  for (iDim = 0; iDim < nDim; iDim++) ModVel_FreeStreamND += Velocity_FreeStreamND[iDim]*Velocity_FreeStreamND[iDim];
  ModVel_FreeStreamND    = sqrt(ModVel_FreeStreamND); config->SetModVel_FreeStreamND(ModVel_FreeStreamND);

  Viscosity_FreeStreamND = Viscosity_FreeStream / Viscosity_Ref;   config->SetViscosity_FreeStreamND(Viscosity_FreeStreamND);
  Thermal_Conductivity_FreeStreamND = Thermal_Conductivity_FreeStream / Conductivity_Ref;
  config->SetThermalConductivity_FreeStreamND(Thermal_Conductivity_FreeStreamND);
  SpecificHeat_Cp_FreeStreamND = SpecificHeat_Cp_FreeStream / Gas_Constant_Ref;
  config->SetSpecificHeatCp_FreeStreamND(SpecificHeat_Cp_FreeStreamND);

  Tke_FreeStream  = 3.0/2.0*(ModVel_FreeStream*ModVel_FreeStream*config->GetTurbulenceIntensity_FreeStream()*config->GetTurbulenceIntensity_FreeStream());
  config->SetTke_FreeStream(Tke_FreeStream);

  Tke_FreeStreamND  = 3.0/2.0*(ModVel_FreeStreamND*ModVel_FreeStreamND*config->GetTurbulenceIntensity_FreeStream()*config->GetTurbulenceIntensity_FreeStream());
  config->SetTke_FreeStreamND(Tke_FreeStreamND);

  Omega_FreeStream = Density_FreeStream*Tke_FreeStream/max(Viscosity_FreeStream*config->GetTurb2LamViscRatio_FreeStream(), EPS);
  config->SetOmega_FreeStream(Omega_FreeStream);

  Omega_FreeStreamND = Density_FreeStreamND*Tke_FreeStreamND/max(Viscosity_FreeStreamND*config->GetTurb2LamViscRatio_FreeStream(), EPS);
  config->SetOmega_FreeStreamND(Omega_FreeStreamND);

  if (config->GetTurbulenceIntensity_FreeStream() *100 <= 1.3) {
    if (config->GetTurbulenceIntensity_FreeStream() *100 >=0.027) {
        Re_ThetaT_FreeStream = (1173.51-589.428*config->GetTurbulenceIntensity_FreeStream() *100+0.2196/
        (config->GetTurbulenceIntensity_FreeStream() *100*config->GetTurbulenceIntensity_FreeStream() *100));
      }
    else {
      Re_ThetaT_FreeStream = (1173.51-589.428*config->GetTurbulenceIntensity_FreeStream() *100+0.2196/(0.27*0.27));
    }
  }
  else {
    Re_ThetaT_FreeStream = 331.5*pow(config->GetTurbulenceIntensity_FreeStream() *100-0.5658,-0.671);
  }
  config->SetReThetaT_FreeStream(Re_ThetaT_FreeStream);

  const su2double MassDiffusivityND = config->GetDiffusivity_Constant() / (Velocity_Ref * Length_Ref);
  config->SetDiffusivity_ConstantND(MassDiffusivityND);

  /*--- Initialize the dimensionless Fluid Model that will be used to solve the dimensionless problem ---*/

  /*--- Auxilary (dimensional) FluidModel no longer needed. ---*/
  delete auxFluidModel;

  /*--- Create one final fluid model object per OpenMP thread to be able to use them in parallel.
   *    GetFluidModel() should be used to automatically access the "right" object of each thread. ---*/

  assert(FluidModel.empty() && "Potential memory leak!");
  FluidModel.resize(omp_get_max_threads());

  SU2_OMP_PARALLEL
  {
    const int thread = omp_get_thread_num();

    switch (config->GetKind_FluidModel()) {

      case STANDARD_AIR:
        FluidModel[thread] = new CIdealGas(1.4, Gas_ConstantND);
        break;

      case IDEAL_GAS:
        FluidModel[thread] = new CIdealGas(Gamma, Gas_ConstantND);
        break;

      case VW_GAS:
        FluidModel[thread] = new CVanDerWaalsGas(Gamma, Gas_ConstantND,
                                                 config->GetPressure_Critical() / config->GetPressure_Ref(),
                                                 config->GetTemperature_Critical() / config->GetTemperature_Ref());
        break;

      case PR_GAS:
        FluidModel[thread] = new CPengRobinson(Gamma, Gas_ConstantND,
                                               config->GetPressure_Critical() / config->GetPressure_Ref(),
                                               config->GetTemperature_Critical() / config->GetTemperature_Ref(),
                                               config->GetAcentric_Factor());
        break;

      case DATADRIVEN_FLUID:
        FluidModel[thread] = new CDataDrivenFluid(config, false);
        break;

      case COOLPROP:
        FluidModel[thread] = new CCoolProp(config->GetFluid_Name());
        break;
    }

    GetFluidModel()->SetEnergy_Prho(Pressure_FreeStreamND, Density_FreeStreamND);
    if (viscous) {
      GetFluidModel()->SetLaminarViscosityModel(config);
      GetFluidModel()->SetThermalConductivityModel(config);
      GetFluidModel()->SetMassDiffusivityModel(config);
    }

  }
  END_SU2_OMP_PARALLEL

  Energy_FreeStreamND = GetFluidModel()->GetStaticEnergy() + 0.5*ModVel_FreeStreamND*ModVel_FreeStreamND;

  if (tkeNeeded) Energy_FreeStreamND += Tke_FreeStreamND;

  config->SetEnergy_FreeStreamND(Energy_FreeStreamND);

  Energy_Ref = Energy_FreeStream/Energy_FreeStreamND; config->SetEnergy_Ref(Energy_Ref);

  Total_UnstTimeND = config->GetTotal_UnstTime() / Time_Ref;    config->SetTotal_UnstTimeND(Total_UnstTimeND);
  Delta_UnstTimeND = config->GetDelta_UnstTime() / Time_Ref;    config->SetDelta_UnstTimeND(Delta_UnstTimeND);

  /*--- Write output to the console if this is the master node and first domain ---*/

  if ((rank == MASTER_NODE) && (MGLevel == MESH_0)) {

    cout.precision(6);

    if (viscous) {
      cout << "Viscous flow: Computing pressure using the ideal gas law" << endl;
      cout << "based on the free-stream temperature and a density computed" << endl;
      cout << "from the Reynolds number." << endl;
    } else {
      cout << "Inviscid flow: Computing density based on free-stream" << endl;
      cout << "temperature and pressure using the ideal gas law." << endl;
    }

    if (dynamic_grid) cout << "Force coefficients computed using MACH_MOTION." << endl;
    else cout << "Force coefficients computed using free-stream values." << endl;

    stringstream NonDimTableOut, ModelTableOut;
    stringstream Unit;

    cout << endl;
    PrintingToolbox::CTablePrinter ModelTable(&ModelTableOut);
    ModelTableOut <<"-- Models:"<< endl;

    ModelTable.AddColumn("Viscosity Model", 25);
    ModelTable.AddColumn("Conductivity Model", 26);
    ModelTable.AddColumn("Fluid Model", 25);
    ModelTable.SetAlign(PrintingToolbox::CTablePrinter::RIGHT);
    ModelTable.PrintHeader();

    PrintingToolbox::CTablePrinter NonDimTable(&NonDimTableOut);
    NonDimTable.AddColumn("Name", 22);
    NonDimTable.AddColumn("Dim. value", 14);
    NonDimTable.AddColumn("Ref. value", 14);
    NonDimTable.AddColumn("Unit", 10);
    NonDimTable.AddColumn("Non-dim. value", 14);
    NonDimTable.SetAlign(PrintingToolbox::CTablePrinter::RIGHT);

    NonDimTableOut <<"-- Fluid properties:"<< endl;

    NonDimTable.PrintHeader();

    if (viscous) {

      switch(config->GetKind_ViscosityModel()){
      case VISCOSITYMODEL::CONSTANT:
        ModelTable << "CONSTANT_VISCOSITY";
        if      (config->GetSystemMeasurements() == SI) Unit << "N.s/m^2";
        else if (config->GetSystemMeasurements() == US) Unit << "lbf.s/ft^2";
        NonDimTable << "Viscosity" << config->GetMu_Constant() << config->GetMu_Constant()/config->GetMu_ConstantND() << Unit.str() << config->GetMu_ConstantND();
        Unit.str("");
        NonDimTable.PrintFooter();
        break;

      case VISCOSITYMODEL::COOLPROP:
        ModelTable << "COOLPROP";
        if      (config->GetSystemMeasurements() == SI) Unit << "N.s/m^2";
        else if (config->GetSystemMeasurements() == US) Unit << "lbf.s/ft^2";
        NonDimTable << "Viscosity" << "--" << "--" << Unit.str() << config->GetMu_ConstantND();
        Unit.str("");
        NonDimTable.PrintFooter();
        break;

      case VISCOSITYMODEL::SUTHERLAND:
        ModelTable << "SUTHERLAND";
        if      (config->GetSystemMeasurements() == SI) Unit << "N.s/m^2";
        else if (config->GetSystemMeasurements() == US) Unit << "lbf.s/ft^2";
        NonDimTable << "Ref. Viscosity" <<  config->GetMu_Ref() <<  config->GetViscosity_Ref() << Unit.str() << config->GetMu_RefND();
        Unit.str("");
        if      (config->GetSystemMeasurements() == SI) Unit << "K";
        else if (config->GetSystemMeasurements() == US) Unit << "R";
        NonDimTable << "Sutherland Temp." << config->GetMu_Temperature_Ref() <<  config->GetTemperature_Ref() << Unit.str() << config->GetMu_Temperature_RefND();
        Unit.str("");
        if      (config->GetSystemMeasurements() == SI) Unit << "K";
        else if (config->GetSystemMeasurements() == US) Unit << "R";
        NonDimTable << "Sutherland Const." << config->GetMu_S() << config->GetTemperature_Ref() << Unit.str() << config->GetMu_SND();
        Unit.str("");
        NonDimTable.PrintFooter();
        break;

      default:
        break;

      }
      switch(config->GetKind_ConductivityModel()){
      case CONDUCTIVITYMODEL::CONSTANT_PRANDTL:
        ModelTable << "CONSTANT_PRANDTL";
        NonDimTable << "Prandtl (Lam.)"  << "-" << "-" << "-" << config->GetPrandtl_Lam();
        Unit.str("");
        NonDimTable << "Prandtl (Turb.)" << "-" << "-" << "-" << config->GetPrandtl_Turb();
        Unit.str("");
        NonDimTable.PrintFooter();
        break;

      case CONDUCTIVITYMODEL::CONSTANT:
        ModelTable << "CONSTANT";
        Unit << "W/m^2.K";
        NonDimTable << "Molecular Cond." << config->GetThermal_Conductivity_Constant() << config->GetThermal_Conductivity_Constant()/config->GetThermal_Conductivity_ConstantND() << Unit.str() << config->GetThermal_Conductivity_ConstantND();
        Unit.str("");
        NonDimTable.PrintFooter();
        break;

      case CONDUCTIVITYMODEL::COOLPROP:
        ModelTable << "COOLPROP";
        Unit << "W/m^2.K";
        NonDimTable << "Molecular Cond." << "--" << "--" << Unit.str() << config->GetThermal_Conductivity_ConstantND();
        Unit.str("");
        NonDimTable.PrintFooter();
        break;

      default:
        break;

      }
    } else {
      ModelTable << "-" << "-";
    }

    if      (config->GetSystemMeasurements() == SI) Unit << "N.m/kg.K";
    else if (config->GetSystemMeasurements() == US) Unit << "lbf.ft/slug.R";
    if (config->GetKind_FluidModel() == COOLPROP) {
      CCoolProp auxFluidModel(config->GetFluid_Name());
      NonDimTable << "Gas Constant" << auxFluidModel.GetGas_Constant() << config->GetGas_Constant_Ref() << Unit.str() << auxFluidModel.GetGas_Constant()/config->GetGas_Constant_Ref();
    }
    else {
        NonDimTable << "Gas Constant" << config->GetGas_Constant() << config->GetGas_Constant_Ref() << Unit.str() << config->GetGas_ConstantND();
    }
    Unit.str("");
    if      (config->GetSystemMeasurements() == SI) Unit << "N.m/kg.K";
    else if (config->GetSystemMeasurements() == US) Unit << "lbf.ft/slug.R";
    if (config->GetKind_FluidModel() == COOLPROP) {
      NonDimTable << "Spec. Heat Ratio" << "-" << "-" << "-" << "-";
    }
    else {
        NonDimTable << "Spec. Heat Ratio" << "-" << "-" << "-" << Gamma;
    }
    Unit.str("");

    switch(config->GetKind_FluidModel()){
    case STANDARD_AIR:
      ModelTable << "STANDARD_AIR";
      break;
    case IDEAL_GAS:
      ModelTable << "IDEAL_GAS";
      break;
    case VW_GAS:
      ModelTable << "VW_GAS";
      break;
    case PR_GAS:
      ModelTable << "PR_GAS";
      break;
    case COOLPROP:
      ModelTable << "CoolProp library";
      break;
    }

    if (config->GetKind_FluidModel() == VW_GAS || config->GetKind_FluidModel() == PR_GAS){
        NonDimTable << "Critical Pressure" << config->GetPressure_Critical() << config->GetPressure_Ref() << Unit.str() << config->GetPressure_Critical() /config->GetPressure_Ref();
        Unit.str("");
        Unit << "K";
        NonDimTable << "Critical Temperature" << config->GetTemperature_Critical() << config->GetTemperature_Ref() << Unit.str() << config->GetTemperature_Critical() /config->GetTemperature_Ref();
        Unit.str("");
    }
    if (config->GetKind_FluidModel() == COOLPROP) {
      CCoolProp auxFluidModel(config->GetFluid_Name());
      NonDimTable << "Critical Pressure" << auxFluidModel.GetPressure_Critical() << config->GetPressure_Ref() << Unit.str() << auxFluidModel.GetPressure_Critical() /config->GetPressure_Ref();
      Unit.str("");
      Unit << "K";
      NonDimTable << "Critical Temperature" << auxFluidModel.GetTemperature_Critical() << config->GetTemperature_Ref() << Unit.str() << auxFluidModel.GetTemperature_Critical() /config->GetTemperature_Ref();
      Unit.str("");
    }
    NonDimTable.PrintFooter();

    NonDimTableOut <<"-- Initial and free-stream conditions:"<< endl;

    NonDimTable.PrintHeader();

    if      (config->GetSystemMeasurements() == SI) Unit << "Pa";
    else if (config->GetSystemMeasurements() == US) Unit << "psf";
    NonDimTable << "Static Pressure" << config->GetPressure_FreeStream() << config->GetPressure_Ref() << Unit.str() << config->GetPressure_FreeStreamND();
    Unit.str("");
    if      (config->GetSystemMeasurements() == SI) Unit << "kg/m^3";
    else if (config->GetSystemMeasurements() == US) Unit << "slug/ft^3";
    NonDimTable << "Density" << config->GetDensity_FreeStream() << config->GetDensity_Ref() << Unit.str() << config->GetDensity_FreeStreamND();
    Unit.str("");
    if      (config->GetSystemMeasurements() == SI) Unit << "K";
    else if (config->GetSystemMeasurements() == US) Unit << "R";
    NonDimTable << "Temperature" << config->GetTemperature_FreeStream() << config->GetTemperature_Ref() << Unit.str() << config->GetTemperature_FreeStreamND();
    Unit.str("");
    if      (config->GetSystemMeasurements() == SI) Unit << "m^2/s^2";
    else if (config->GetSystemMeasurements() == US) Unit << "ft^2/s^2";
    NonDimTable << "Total Energy" << config->GetEnergy_FreeStream() << config->GetEnergy_Ref() << Unit.str() << config->GetEnergy_FreeStreamND();
    Unit.str("");
    if      (config->GetSystemMeasurements() == SI) Unit << "m/s";
    else if (config->GetSystemMeasurements() == US) Unit << "ft/s";
    NonDimTable << "Velocity-X" << config->GetVelocity_FreeStream()[0] << config->GetVelocity_Ref() << Unit.str() << config->GetVelocity_FreeStreamND()[0];
    NonDimTable << "Velocity-Y" << config->GetVelocity_FreeStream()[1] << config->GetVelocity_Ref() << Unit.str() << config->GetVelocity_FreeStreamND()[1];
    if (nDim == 3){
      NonDimTable << "Velocity-Z" << config->GetVelocity_FreeStream()[2] << config->GetVelocity_Ref() << Unit.str() << config->GetVelocity_FreeStreamND()[2];
    }
    NonDimTable << "Velocity Magnitude" << config->GetModVel_FreeStream() << config->GetVelocity_Ref() << Unit.str() << config->GetModVel_FreeStreamND();
    Unit.str("");

    if (viscous){
      NonDimTable.PrintFooter();
      if      (config->GetSystemMeasurements() == SI) Unit << "N.s/m^2";
      else if (config->GetSystemMeasurements() == US) Unit << "lbf.s/ft^2";
      NonDimTable << "Viscosity" << config->GetViscosity_FreeStream() << config->GetViscosity_Ref() << Unit.str() << config->GetViscosity_FreeStreamND();
      Unit.str("");
      if      (config->GetSystemMeasurements() == SI) Unit << "W/m^2.K";
      else if (config->GetSystemMeasurements() == US) Unit << "lbf/ft.s.R";
      NonDimTable << "Conductivity" << "-" << config->GetThermal_Conductivity_Ref() << Unit.str() << "-";
      Unit.str("");
      if (turbulent){
        if      (config->GetSystemMeasurements() == SI) Unit << "m^2/s^2";
        else if (config->GetSystemMeasurements() == US) Unit << "ft^2/s^2";
        NonDimTable << "Turb. Kin. Energy" << config->GetTke_FreeStream() << config->GetTke_FreeStream()/config->GetTke_FreeStreamND() << Unit.str() << config->GetTke_FreeStreamND();
        Unit.str("");
        if      (config->GetSystemMeasurements() == SI) Unit << "1/s";
        else if (config->GetSystemMeasurements() == US) Unit << "1/s";
        NonDimTable << "Spec. Dissipation" << config->GetOmega_FreeStream() << config->GetOmega_FreeStream()/config->GetOmega_FreeStreamND() << Unit.str() << config->GetOmega_FreeStreamND();
        Unit.str("");
        if (config-> GetKind_Trans_Model() == TURB_TRANS_MODEL::LM) {
          NonDimTable << "Intermittency"  << "-" << "-" << "-" << config->GetIntermittency_FreeStream();
          Unit.str("");
          NonDimTable << "Moment. Thick. Re"  << "-" << "-" << "-" << config->GetReThetaT_FreeStream();
          Unit.str("");
        }
      }
      if (config->GetKind_Species_Model() != SPECIES_MODEL::NONE) {
        if      (config->GetSystemMeasurements() == SI) Unit << "m^2/s";
        else if (config->GetSystemMeasurements() == US) Unit << "ft^2/s";
        NonDimTable << "Mass Diffusivity" << config->GetDiffusivity_Constant() << config->GetDiffusivity_Constant()/config->GetDiffusivity_ConstantND() << Unit.str() << config->GetDiffusivity_ConstantND();
        Unit.str("");
      }
    }

    NonDimTable.PrintFooter();
    NonDimTable << "Mach Number" << "-" << "-" << "-" << config->GetMach();
    if (viscous){
      NonDimTable << "Reynolds Number" << "-" << "-" << "-" << config->GetReynolds();
    }
    if (gravity) {
      NonDimTable << "Froude Number" << "-" << "-" << "-" << Froude;
      NonDimTable << "Wave Length"   << "-" << "-" << "-" << 2.0*PI_NUMBER*Froude*Froude;
    }
    NonDimTable.PrintFooter();
    ModelTable.PrintFooter();

    if (unsteady){
      NonDimTableOut << "-- Unsteady conditions" << endl;
      NonDimTable.PrintHeader();
      NonDimTable << "Total Time" << config->GetMax_Time() << config->GetTime_Ref() << "s" << config->GetMax_Time()/config->GetTime_Ref();
      Unit.str("");
      NonDimTable << "Time Step" << config->GetTime_Step() << config->GetTime_Ref() << "s" << config->GetDelta_UnstTimeND();
      Unit.str("");
      NonDimTable.PrintFooter();
    }

    cout << ModelTableOut.str();
    cout << NonDimTableOut.str();

  }

}

void CEulerSolver::SetReferenceValues(const CConfig& config) {
  SU2_ZONE_SCOPED

  /*--- Evaluate reference values for non-dimensionalization. For dynamic meshes,
   use the motion Mach number as a reference value for computing the force coefficients.
   Otherwise, use the freestream values, which is the standard convention. ---*/

  su2double RefVel2;

  if (dynamic_grid && !config.GetFSI_Simulation()) {
    su2double Gas_Constant = config.GetGas_ConstantND();
    su2double Mach2Vel = sqrt(Gamma * Gas_Constant * Temperature_Inf);
    su2double Mach_Motion = config.GetMach_Motion();
    RefVel2 = pow(Mach_Motion * Mach2Vel, 2);
  }
  else {
    RefVel2 = GeometryToolbox::SquaredNorm(nDim, Velocity_Inf);
  }

  DynamicPressureRef = 0.5 * Density_Inf * RefVel2;

  if (DynamicPressureRef < EPS) {
    DynamicPressureRef = 1.0;
  }

  AeroCoeffForceRef =  DynamicPressureRef * config.GetRefArea();

}

void CEulerSolver::SetInitialCondition(CGeometry **geometry, CSolver ***solver_container, CConfig *config, unsigned long TimeIter) {
  SU2_ZONE_SCOPED

  const bool restart = (config->GetRestart() || config->GetRestart_Flow());
  const bool SubsonicEngine = config->GetSubsonicEngine();

  /*--- Use default implementation, then add solver-specifics. ---*/

  BaseClass::SetInitialCondition(geometry, solver_container, config, TimeIter);

  /*--- Set subsonic initial condition for engine intakes at iteration 0 ---*/

  if (!SubsonicEngine || (TimeIter != 0) || restart) return;

  /*--- Start OpenMP parallel region. ---*/

  SU2_OMP_PARALLEL {

    unsigned long iPoint;
    unsigned short iMesh, iDim;
    su2double X0[MAXNDIM] = {0.0}, X1[MAXNDIM] = {0.0}, X2[MAXNDIM] = {0.0},
    X1_X0[MAXNDIM] = {0.0}, X2_X0[MAXNDIM] = {0.0}, X2_X1[MAXNDIM] = {0.0},
    CP[MAXNDIM] = {0.0}, Distance, DotCheck, Radius;

    su2double Velocity_Cyl[MAXNDIM] = {0.0}, Velocity_CylND[MAXNDIM] = {0.0}, Viscosity_Cyl,
    Density_Cyl, Density_CylND, Pressure_CylND, ModVel_Cyl, ModVel_CylND, Energy_CylND,
    T_ref = 0.0, S = 0.0, Mu_ref = 0.0;
    const su2double *Coord, *SubsonicEngine_Cyl, *SubsonicEngine_Values;

    SubsonicEngine_Values = config->GetSubsonicEngine_Values();
    su2double Mach_Cyl        = SubsonicEngine_Values[0];
    su2double Alpha_Cyl       = SubsonicEngine_Values[1];
    su2double Beta_Cyl        = SubsonicEngine_Values[2];
    su2double Pressure_Cyl    = SubsonicEngine_Values[3];
    su2double Temperature_Cyl = SubsonicEngine_Values[4];

    su2double Alpha = Alpha_Cyl*PI_NUMBER/180.0;
    su2double Beta  = Beta_Cyl*PI_NUMBER/180.0;

    su2double Gamma_Minus_One = Gamma - 1.0;
    su2double Gas_Constant = config->GetGas_Constant();

    su2double Mach2Vel_Cyl = sqrt(Gamma*Gas_Constant*Temperature_Cyl);

    for (iMesh = 0; iMesh <= config->GetnMGLevels(); iMesh++) {

      auto FlowNodes = solver_container[iMesh][FLOW_SOL]->GetNodes();

      SU2_OMP_FOR_STAT(omp_chunk_size)
      for (iPoint = 0; iPoint < geometry[iMesh]->GetnPoint(); iPoint++) {

        Velocity_Cyl[0] = cos(Alpha)*cos(Beta)*Mach_Cyl*Mach2Vel_Cyl;
        Velocity_Cyl[1] = sin(Beta)*Mach_Cyl*Mach2Vel_Cyl;
        Velocity_Cyl[2] = sin(Alpha)*cos(Beta)*Mach_Cyl*Mach2Vel_Cyl;

        ModVel_Cyl = GeometryToolbox::Norm(3, Velocity_Cyl);

        if (config->GetViscous()) {
          if (config->GetSystemMeasurements() == SI) { T_ref = 273.15; S = 110.4; Mu_ref = 1.716E-5; }
          if (config->GetSystemMeasurements() == US) {
            T_ref = (273.15 - 273.15) * 1.8 + 491.67;
            S = (110.4 - 273.15) * 1.8 + 491.67;
            Mu_ref = 1.716E-5/47.88025898;
          }
          Viscosity_Cyl = Mu_ref*(pow(Temperature_Cyl/T_ref, 1.5) * (T_ref+S)/(Temperature_Cyl+S));
          Density_Cyl   = config->GetReynolds()*Viscosity_Cyl/(ModVel_Cyl*config->GetLength_Reynolds());
          Pressure_Cyl  = Density_Cyl*Gas_Constant*Temperature_Cyl;
        }
        else {
          Density_Cyl = Pressure_Cyl/(Gas_Constant*Temperature_Cyl);
        }

        Density_CylND  = Density_Cyl/config->GetDensity_Ref();
        Pressure_CylND = Pressure_Cyl/config->GetPressure_Ref();

        for (iDim = 0; iDim < nDim; iDim++) {
          Velocity_CylND[iDim] = Velocity_Cyl[iDim]/config->GetVelocity_Ref();
        }

        ModVel_CylND = GeometryToolbox::Norm(nDim, Velocity_CylND);

        Energy_CylND = Pressure_CylND/(Density_CylND*Gamma_Minus_One)+0.5*ModVel_CylND*ModVel_CylND;

        Coord = geometry[iMesh]->nodes->GetCoord(iPoint);

        SubsonicEngine_Cyl = config->GetSubsonicEngine_Cyl();

        X0[0] = Coord[0];               X0[1] = Coord[1];               if (nDim==3) X0[2] = Coord[2];
        X1[0] = SubsonicEngine_Cyl[0];  X1[1] = SubsonicEngine_Cyl[1];  X1[2] = SubsonicEngine_Cyl[2];
        X2[0] = SubsonicEngine_Cyl[3];  X2[1] = SubsonicEngine_Cyl[4];  X2[2] = SubsonicEngine_Cyl[5];
        Radius = SubsonicEngine_Cyl[6];

        GeometryToolbox::Distance(3, X1, X2, X2_X1);
        GeometryToolbox::Distance(3, X0, X1, X1_X0);
        GeometryToolbox::Distance(3, X0, X2, X2_X0);

        GeometryToolbox::CrossProduct(X2_X1, X1_X0, CP);

        Distance = sqrt(GeometryToolbox::SquaredNorm(3,CP) / GeometryToolbox::SquaredNorm(3,X2_X1));

        DotCheck = -GeometryToolbox::DotProduct(3, X1_X0, X2_X1);
        if (DotCheck < 0.0) Distance = GeometryToolbox::Norm(3, X1_X0);

        DotCheck = GeometryToolbox::DotProduct(3, X2_X0, X2_X1);
        if (DotCheck < 0.0) Distance = GeometryToolbox::Norm(3, X2_X0);

        if (Distance < Radius) {
          FlowNodes->SetSolution(iPoint, 0, Density_CylND);
          for (iDim = 0; iDim < nDim; iDim++)
            FlowNodes->SetSolution(iPoint, iDim+1, Density_CylND*Velocity_CylND[iDim]);
          FlowNodes->SetSolution(iPoint, nVar-1, Density_CylND*Energy_CylND);
        }

      }
      END_SU2_OMP_FOR

      FlowNodes->Set_OldSolution();

    }

  }
  END_SU2_OMP_PARALLEL

}

void CEulerSolver::CommonPreprocessing(CGeometry *geometry, CSolver **solver_container, CConfig *config, unsigned short iMesh,
                                       unsigned short iRKStep, unsigned short RunTime_EqSystem, bool Output) {
  SU2_ZONE_SCOPED

  bool cont_adjoint     = config->GetContinuous_Adjoint();
  bool disc_adjoint     = config->GetDiscrete_Adjoint();
  bool implicit         = (config->GetKind_TimeIntScheme() == EULER_IMPLICIT);
  bool center           = (config->GetKind_ConvNumScheme_Flow() == SPACE_CENTERED);
  bool center_jst       = (config->GetKind_Centered_Flow() == CENTERED::JST) && (iMesh == MESH_0);
  bool center_jst_ke    = (config->GetKind_Centered_Flow() == CENTERED::JST_KE) && (iMesh == MESH_0);
  bool center_jst_mat   = (config->GetKind_Centered_Flow() == CENTERED::JST_MAT) && (iMesh == MESH_0);
  bool engine           = ((config->GetnMarker_EngineInflow() != 0) || (config->GetnMarker_EngineExhaust() != 0));
  bool actuator_disk    = ((config->GetnMarker_ActDiskInlet() != 0) || (config->GetnMarker_ActDiskOutlet() != 0));
  bool fixed_cl         = config->GetFixed_CL_Mode();
  unsigned short kind_row_dissipation = config->GetKind_RoeLowDiss();
  bool roe_low_dissipation  = (kind_row_dissipation != NO_ROELOWDISS) &&
                              (config->GetKind_Upwind_Flow() == UPWIND::ROE ||
                               config->GetKind_Upwind_Flow() == UPWIND::SLAU ||
                               config->GetKind_Upwind_Flow() == UPWIND::SLAU2);

  /*--- Set the primitive variables ---*/

  ompMasterAssignBarrier(ErrorCounter, 0);

  SU2_OMP_ATOMIC
  ErrorCounter += SetPrimitive_Variables(solver_container, config);

  BEGIN_SU2_OMP_SAFE_GLOBAL_ACCESS
  { /*--- Ops that are not OpenMP parallel go in this block. ---*/

    if ((iMesh == MESH_0) && (config->GetComm_Level() == COMM_FULL)) {
      unsigned long tmp = ErrorCounter;
      SU2_MPI::Allreduce(&tmp, &ErrorCounter, 1, MPI_UNSIGNED_LONG, MPI_SUM, SU2_MPI::GetComm());
      config->SetNonphysical_Points(ErrorCounter);
    }

    /*--- Update the angle of attack at the far-field for fixed CL calculations (only direct problem). ---*/

    if (fixed_cl && !disc_adjoint && !cont_adjoint) {
      SetFarfield_AoA(geometry, solver_container, config, iMesh, Output);
    }

    /*--- Compute the engine properties ---*/

    if (engine) GetPower_Properties(geometry, config, iMesh, Output);

    /*--- Compute the actuator disk properties and distortion levels ---*/

    if (actuator_disk) {
      Set_MPI_ActDisk(solver_container, geometry, config);
      GetPower_Properties(geometry, config, iMesh, Output);
      SetActDisk_BCThrust(geometry, solver_container, config, iMesh, Output);
    }

  }
  END_SU2_OMP_SAFE_GLOBAL_ACCESS

  /*--- Artificial dissipation ---*/

  if (center && !Output) {
    if (!center_jst_mat) SetMax_Eigenvalue(geometry, config);
    if (center_jst || center_jst_ke || center_jst_mat) {
      SetCentered_Dissipation_Sensor(geometry, config);
      if (!center_jst_ke) SetUndivided_Laplacian(geometry, config);
    }
  }
  if (config->GetKind_Upwind_Flow() == UPWIND::MSW && !Output) {
    SetCentered_Dissipation_Sensor(geometry, config);
  }

  /*--- Roe Low Dissipation Sensor ---*/

  if (roe_low_dissipation) {
    SetRoe_Dissipation(geometry, config);
    if (kind_row_dissipation == FD_DUCROS || kind_row_dissipation == NTS_DUCROS){
      SetUpwind_Ducros_Sensor(geometry, config);
    }
  }

  /*--- Initialize the Jacobian matrix and residual, not needed for the reducer strategy
   *    as we set blocks (including diagonal ones) and completely overwrite. ---*/

  if (!ReducerStrategy && !Output) {
    LinSysRes.SetValZero();
    if (implicit) Jacobian.SetValDiagonalZero();
    else {SU2_OMP_BARRIER} // because of "nowait" in LinSysRes
  }
 /*--- Set all points as physical before iteration. ---*/

 if (!Output) {
    for (auto iPoint = 0; iPoint < nPoint; iPoint++) {
      nodes->ResetNon_Physical(iPoint);
    }
  }

}

void CEulerSolver::Preprocessing(CGeometry *geometry, CSolver **solver_container, CConfig *config, unsigned short iMesh,
                                 unsigned short iRKStep, unsigned short RunTime_EqSystem, bool Output) {
  SU2_ZONE_SCOPED
  const auto InnerIter = config->GetInnerIter();
  const bool muscl = config->GetMUSCL_Flow() && (iMesh == MESH_0);
  const bool center = (config->GetKind_ConvNumScheme_Flow() == SPACE_CENTERED);
  const bool limiter = (config->GetKind_SlopeLimit_Flow() != LIMITER::NONE) && (InnerIter <= config->GetLimiterIter());
  const bool van_albada = (config->GetKind_SlopeLimit_Flow() == LIMITER::VAN_ALBADA_EDGE);

  /*--- Common preprocessing steps. ---*/

  CommonPreprocessing(geometry, solver_container, config, iMesh, iRKStep, RunTime_EqSystem, Output);

  /*--- Upwind second order reconstruction ---*/

  if (!Output && muscl && !center) {

    /*--- Gradient computation for MUSCL reconstruction. ---*/

    switch (config->GetKind_Gradient_Method_Recon()) {
      case GREEN_GAUSS:
        SetPrimitive_Gradient_GG(geometry, config, true); break;
      case LEAST_SQUARES:
      case WEIGHTED_LEAST_SQUARES:
        SetPrimitive_Gradient_LS(geometry, config, true); break;
      default: break;
    }

    /*--- Limiter computation ---*/

    if (limiter && !van_albada) SetPrimitive_Limiter(geometry, config);

    if (config->GetEdge_Form_TeCNO_Adjoint()) SetTeCNOAdjointGradient(geometry, config);
  }
}

unsigned long CEulerSolver::SetPrimitive_Variables(CSolver **solver_container, const CConfig *config) {
  SU2_ZONE_SCOPED

  /*--- Number of non-physical points, local to the thread, needs
   *    further reduction if function is called in parallel ---*/
  unsigned long nonPhysicalPoints = 0;

  AD::StartNoSharedReading();

  SU2_OMP_FOR_STAT(omp_chunk_size)
  for (unsigned long iPoint = 0; iPoint < nPoint; iPoint ++) {

    /*--- Compressible flow, primitive variables nDim+9, (T, vx, vy, vz, P, rho, h, c, lamMu, eddyMu, ThCond, Cp) ---*/

    bool physical = nodes->SetPrimVar(iPoint, GetFluidModel());
    nodes->SetSecondaryVar(iPoint, GetFluidModel());

    /* Check for non-realizable states for reporting. */

    if (!physical) nonPhysicalPoints++;
  }
  END_SU2_OMP_FOR

  AD::EndNoSharedReading();

  return nonPhysicalPoints;
}

void CEulerSolver::SetTime_Step(CGeometry *geometry, CSolver **solver_container, CConfig *config,
                                unsigned short iMesh, unsigned long Iteration) {
  SU2_ZONE_SCOPED

  /*--- Define an object to compute the speed of sound. ---*/
  struct SoundSpeed {
    FORCEINLINE su2double operator() (const CEulerVariable& nodes, unsigned long iPoint, unsigned long jPoint) const {
      return 0.5 * (nodes.GetSoundSpeed(iPoint) + nodes.GetSoundSpeed(jPoint));
    }

    FORCEINLINE su2double operator() (const CEulerVariable& nodes, unsigned long iPoint) const {
      return nodes.GetSoundSpeed(iPoint);
    }

  } soundSpeed;

  /*--- Define an object to compute the viscous eigenvalue. ---*/
  struct LambdaVisc {
    const su2double gamma, prandtlTurb;

    LambdaVisc(su2double g, su2double pt) : gamma(g), prandtlTurb(pt) {}

    FORCEINLINE su2double lambda(su2double laminarVisc, su2double eddyVisc, su2double density, su2double cp,
                                 su2double thermalCond) const {
      su2double Lambda_1 = (4.0 / 3.0) * (laminarVisc + eddyVisc);
      /// TODO: (REAL_GAS) removing gamma as it cannot work with FLUIDPROP
      su2double Lambda_2 = (thermalCond / cp + eddyVisc / prandtlTurb) * gamma;
      return (Lambda_1 + Lambda_2) / density;
    }

    FORCEINLINE su2double operator() (const CEulerVariable& nodes, unsigned long iPoint, unsigned long jPoint) const {
      su2double laminarVisc = 0.5*(nodes.GetLaminarViscosity(iPoint) + nodes.GetLaminarViscosity(jPoint));
      su2double eddyVisc = 0.5*(nodes.GetEddyViscosity(iPoint) + nodes.GetEddyViscosity(jPoint));
      su2double density = 0.5*(nodes.GetDensity(iPoint) + nodes.GetDensity(jPoint));
      su2double thermalCond = 0.5*(nodes.GetThermalConductivity(iPoint) + nodes.GetThermalConductivity(jPoint));
      su2double cp = 0.5*(nodes.GetSpecificHeatCp(iPoint) + nodes.GetSpecificHeatCp(jPoint));
      return lambda(laminarVisc, eddyVisc, density, cp, thermalCond);
    }

    FORCEINLINE su2double operator() (const CEulerVariable& nodes, unsigned long iPoint) const {
      su2double laminarVisc = nodes.GetLaminarViscosity(iPoint);
      su2double eddyVisc = nodes.GetEddyViscosity(iPoint);
      su2double density = nodes.GetDensity(iPoint);
      su2double thermalCond = nodes.GetThermalConductivity(iPoint);
      su2double cp = nodes.GetSpecificHeatCp(iPoint);
      return lambda(laminarVisc, eddyVisc, density, cp, thermalCond);
    }

  } lambdaVisc(Gamma, Prandtl_Turb);

  /*--- Now instantiate the generic implementation with the two functors above. ---*/

  SetTime_Step_impl(soundSpeed, lambdaVisc, geometry, solver_container, config, iMesh, Iteration);

}

void CEulerSolver::Centered_Residual(CGeometry *geometry, CSolver **solver_container, CNumerics **numerics_container,
                                     CConfig *config, unsigned short iMesh, unsigned short iRKStep) {
  SU2_ZONE_SCOPED

  /*--- Matched k (SST with the P1 options): the blend pre-pass takes the hat states, as the edge loop. ---*/
  const CVariable* turbMatched = ((iMesh == MESH_0) && config->GetTke_Matched_P1()) ? solver_container[TURB_SOL]->GetNodes() : nullptr;
  if ((iMesh == MESH_0) && config->GetEdge_Form_ES_Blend()) SetEdgeBlendLambda(geometry, config, turbMatched);
  /*--- Matched k: the k rows of the non-edge mass terms of this residual (split pairs, diagonals, nodal terms, the
   * axisymmetric source), filled by the passes below and by Source_Residual, added by the SST (MatchedNodalKine). ---*/
  if (turbMatched) {
    SU2_OMP_SAFE_GLOBAL_ACCESS(P1KNodalRes.resize(nPoint) = su2double(0.0); P1KNodalJac.resize(nPoint) = su2double(0.0);)
    /*--- MUSCL_TURB: the bounds of the turbulence reconstruction for the face k of the edge kernel. ---*/
    if (config->GetMUSCL_Turb()) {
      SU2_OMP_SAFE_GLOBAL_ACCESS(
      nodes->MatchedKineBounds[0] = solver_container[TURB_SOL]->GetScalarLowerLimit(0);
      nodes->MatchedKineBounds[1] = solver_container[TURB_SOL]->GetScalarUpperLimit(0);)
    }
  }
  EdgeFluxResidual(geometry, solver_container, config);
  if ((iMesh == MESH_0) && config->GetJST_Entropy_Budget()) JSTEntropyResidual(geometry, solver_container, config);
  if ((iMesh == MESH_0) && (config->GetSplit_Edge_Flux() || config->GetEdge_Form_P1_Mixed())) SplitEdgeResidual(geometry, config, turbMatched);
  if ((iMesh == MESH_0) && config->GetGCL_Exact_P1()) SweptBoundaryResidual(geometry, config, turbMatched);
}

void CEulerSolver::SetBoundaryHatStates(CNumerics* numerics, const su2double* V_domain, const su2double* V_ghost,
                                        su2double kNode, su2double (*Vhat)[MAXNVAR + 8]) const {
  if (nPrimVar > MAXNVAR + 8) SU2_MPI::Error("Matched k: too many primitive variables.", CURRENT_FUNCTION);
  for (unsigned short iVar = 0; iVar < nPrimVar; ++iVar) {
    Vhat[0][iVar] = V_domain[iVar];
    Vhat[1][iVar] = V_ghost[iVar];
  }
  /*--- Both states carry the k of the node in their total enthalpy (BC_Far_Field, BC_Inlet, BC_Outlet). ---*/
  Vhat[0][nDim + 3] -= kNode;
  Vhat[1][nDim + 3] -= kNode;
  numerics->SetPrimitive(Vhat[0], Vhat[1]);
  numerics->SetTurbKineticEnergy(0.0, 0.0);
}

void CEulerSolver::AddBoundaryMatchedKine(unsigned long iPoint, su2double kNode, su2double kBc, su2double* res,
                                          su2double (*jac)[5], bool implicit) {
  /*--- Closure C k rule (P1_SST_COUPLED.md M2, SSTPATHS_PLAN.md revision 1): the hat boundary flux plus k_b m, the SST
   * k row gets the same k_b m (MatchedNodalKine) and skips its own convective boundary flux (ConvectiveBCOptions). ---*/
  const su2double m = res[0];
  P1KNodalRes[iPoint] += P1SSTMatched::BoundaryNodeKine(nVar, kNode, kBc, res, implicit ? jac : nullptr);
  P1KNodalJac[iPoint] += fmax(m, 0.0) / nodes->GetDensity(iPoint);
}

void CEulerSolver::SweptBoundaryResidual(const CGeometry *geometry, const CConfig *config, const CVariable* turbMatched) {
  SU2_ZONE_SCOPED

  /*--- -U_i beta_i, the nodal boundary term of the grid flux of GCL_EXACT_P1. No Jacobian, as for the split pairs.
   * Matched k (SSTPATHS_PLAN.md 3): its mass part -rho_i beta_i carries the node's k, -(rho k)_i beta_i, which is the k
   * part of -rhoE_i beta_i in the energy row; the same number goes to the SST k row (P1KNodalRes). ---*/

  const auto& boundSwept = geometry->edges->GetBoundSwept();
  auto rhoK = [&](unsigned long iPoint) { return nodes->GetDensity(iPoint) * turbMatched->GetSolution(iPoint, 0); };

  if (!config->GetGrid_Entropy_P1()) {
    SU2_OMP_FOR_STAT(omp_chunk_size)
    for (auto iPoint = 0ul; iPoint < nPointDomain; ++iPoint) {
      for (auto iVar = 0u; iVar < nVar; ++iVar) {
        LinSysRes(iPoint, iVar) -= boundSwept[iPoint] * nodes->GetSolution(iPoint, iVar);
      }
      if (turbMatched) P1KNodalRes[iPoint] -= boundSwept[iPoint] * rhoK(iPoint);
    }
    END_SU2_OMP_FOR
    return;
  }

  /*--- GRID_ENTROPY_P1 (P1_ALE.md, 6.3): -U_i sigma_i at a wall adds mass Sum rho_i sigma_i and entropy U_i sigma_i.
   * The remainder sigma_i is the grid flux of the boundary: with BC_WEAK_P1 it is sum_{f ni i} kappa_f over the
   * boundary faces of the node (CGeometry::BoundaryFaceKappa). The faces of closure C (BC_ENTROPY_P1 walls, symmetry
   * planes and far field, weak no-slip walls of BC_WALL_ENTROPY_P1) put kappa_f into the grid fluxes of their face
   * points, so their share is removed here. What is left is lumped: at nodes that are only on walls it is the
   * pressure work p_i r_i of a wall with zero relative mass flux, elsewhere -U_i r_i as before. No Jacobian. ---*/

  BEGIN_SU2_OMP_SAFE_GLOBAL_ACCESS {
    const bool closureC = config->GetBC_Entropy_P1();
    const bool weakNoSlip = config->GetBC_Wall_Entropy_P1();

    auto isWall = [](unsigned short kind) {
      return (kind == EULER_WALL) || (kind == SYMMETRY_PLANE) || (kind == HEAT_FLUX) || (kind == ISOTHERMAL);
    };
    auto inClosure = [&](unsigned short kind) {
      return (closureC && ((kind == EULER_WALL) || (kind == SYMMETRY_PLANE) || (kind == FAR_FIELD))) ||
             (weakNoSlip && ((kind == HEAT_FLUX) || (kind == ISOTHERMAL)));
    };

    vector<su2double> remainder(nPointDomain);
    for (auto iPoint = 0ul; iPoint < nPointDomain; ++iPoint) remainder[iPoint] = boundSwept[iPoint];

    for (auto iMarker = 0u; iMarker < geometry->GetnMarker(); ++iMarker) {
      if (!inClosure(config->GetMarker_All_KindBC(iMarker))) continue;
      for (auto iElem = 0ul; iElem < geometry->GetnElem_Bound(iMarker); ++iElem) {
        const auto* face = geometry->bound[iMarker][iElem];
        const auto nFaceNodes = face->GetnNodes();
        unsigned long points[3] = {0, 0, 0};
        for (unsigned short k = 0; k < nFaceNodes; ++k) points[k] = face->GetNode(k);
        const su2double kappa = geometry->BoundaryFaceKappa(config, nFaceNodes, points, face->getNormal());
        for (unsigned short k = 0; k < nFaceNodes; ++k) {
          if (geometry->nodes->GetDomain(points[k])) remainder[points[k]] -= kappa;
        }
      }
    }

    for (auto iPoint = 0ul; iPoint < nPointDomain; ++iPoint) {
      if (!geometry->nodes->GetPhysicalBoundary(iPoint)) continue;
      bool wallOnly = true;
      for (auto iMarker = 0u; iMarker < geometry->GetnMarker(); ++iMarker) {
        const auto kind = config->GetMarker_All_KindBC(iMarker);
        if ((kind == SEND_RECEIVE) || (geometry->nodes->GetVertex(iPoint, iMarker) < 0)) continue;
        wallOnly = wallOnly && isWall(kind);
      }
      if (wallOnly) {
        LinSysRes(iPoint, nDim + 1) += remainder[iPoint] * nodes->GetPressure(iPoint);
      } else {
        for (auto iVar = 0u; iVar < nVar; ++iVar) LinSysRes(iPoint, iVar) -= remainder[iPoint] * nodes->GetSolution(iPoint, iVar);
        if (turbMatched) P1KNodalRes[iPoint] -= remainder[iPoint] * rhoK(iPoint);
      }
    }
  }
  END_SU2_OMP_SAFE_GLOBAL_ACCESS
}

void CEulerSolver::SplitEdgeResidual(const CGeometry *geometry, const CConfig *config, const CVariable* turbMatched) {
  SU2_ZONE_SCOPED

  const auto& splitNodes = geometry->edges->GetSplitNodes();
  const auto& splitNormal = geometry->edges->GetSplitNormal();
  const auto& splitE = geometry->edges->GetSplitE();
  const auto& boundNormal = geometry->edges->GetSplitBoundNormal();
  const bool sweptFlux = config->GetGCL_Exact_P1();

  /*--- Matched k of SST (SSTPATHS_PLAN.md 1, 4): the pairs act on the hat states (H - k, rhoE - rho k) and carry k_up m
   * in the energy row; the same numbers go to the SST k row through P1KNodalRes (pairs: + at i, - at k; the 3D boundary
   * nodal term: k_i m_b at i). ---*/
  auto tke = [&](unsigned long iPoint) { return turbMatched->GetSolution(iPoint, 0); };
  auto hatSolution = [&](unsigned long iPoint, unsigned short iVar) {
    su2double U = nodes->GetSolution(iPoint, iVar);
    if (turbMatched && (iVar == nDim + 1)) U -= nodes->GetDensity(iPoint) * tke(iPoint);
    return U;
  };
  auto addPairKine = [&](unsigned long iPoint, unsigned long kPoint, su2double* flux) {
    const su2double kFlux = P1SSTMatched::PairKine(flux[0], tke(iPoint), tke(kPoint));
    su2double diag_i = 0.0, diag_k = 0.0;
    P1SSTMatched::PairKineDiag(flux[0], nodes->GetDensity(iPoint), nodes->GetDensity(kPoint), diag_i, diag_k);
    flux[nDim + 1] += kFlux;
    if (geometry->nodes->GetDomain(iPoint)) { P1KNodalRes[iPoint] += kFlux; P1KNodalJac[iPoint] += diag_i; }
    if (geometry->nodes->GetDomain(kPoint)) { P1KNodalRes[kPoint] -= kFlux; P1KNodalJac[kPoint] += diag_k; }
  };

  /*--- Convective flux of a point through an area vector, added with a factor. ---*/

  auto addFlux = [&](unsigned long iPoint, const su2double* normal, su2double factor, su2double* flux) {
    const su2double density = nodes->GetDensity(iPoint);
    const su2double pressure = nodes->GetPressure(iPoint);
    su2double projVel = 0.0;
    for (auto iDim = 0u; iDim < nDim; ++iDim) projVel += nodes->GetVelocity(iPoint, iDim) * normal[iDim];

    flux[0] += factor * density * projVel;
    for (auto iDim = 0u; iDim < nDim; ++iDim) {
      flux[iDim + 1] += factor * (density * projVel * nodes->GetVelocity(iPoint, iDim) + pressure * normal[iDim]);
    }
    if (turbMatched) flux[nDim + 1] += factor * density * projVel * (nodes->GetEnthalpy(iPoint) - tke(iPoint));
    else flux[nDim + 1] += factor * density * projVel * nodes->GetEnthalpy(iPoint);

    if (dynamic_grid) {
      const su2double projGridVel = GeometryToolbox::DotProduct(nDim, geometry->nodes->GetGridVel(iPoint), normal);
      for (auto iVar = 0u; iVar < nVar; ++iVar) flux[iVar] -= factor * projGridVel * hatSolution(iPoint, iVar);
    }
  };

  /*--- EDGE_FORM_P1_MIXED (P1_MIXED.md, 8.3): the face and body diagonals of the split averages carry only the central
   * part along n_ik, E = 0 (W = |t|/(d+1)). With EDGE_FORM_EC_FLUX it is the Ismail-Roe flux, so that the whole central
   * part is entropy conservative at interior nodes (Theorem X4). No dissipation, no Jacobian (the diagonals are not in
   * the sparsity pattern), static meshes only. ---*/
  if (config->GetEdge_Form_P1_Mixed()) {
    const bool ecFlux = config->GetEdge_Form_EC_Flux();
    auto state = [&](unsigned long iPoint) {
      P1AleEntropy::State S;
      S.rho = nodes->GetDensity(iPoint);
      S.p = nodes->GetPressure(iPoint);
      for (auto iDim = 0u; iDim < nDim; ++iDim) S.vel[iDim] = nodes->GetVelocity(iPoint, iDim);
      return S;
    };
    BEGIN_SU2_OMP_SAFE_GLOBAL_ACCESS {
      for (auto iSplit = 0ul; iSplit < splitNodes.size(); ++iSplit) {
        const auto iPoint = splitNodes[iSplit][0], kPoint = splitNodes[iSplit][1];
        su2double flux[MAXNVAR] = {};
        if (ecFlux) {
          P1AleEntropy::IsmailRoeFlux(nDim, Gamma, state(iPoint), state(kPoint), splitNormal[iSplit], flux);
        } else {
          addFlux(iPoint, splitNormal[iSplit], 0.5, flux);
          addFlux(kPoint, splitNormal[iSplit], 0.5, flux);
        }
        if (turbMatched) addPairKine(iPoint, kPoint, flux);
        if (geometry->nodes->GetDomain(iPoint)) LinSysRes.AddBlock(iPoint, flux);
        if (geometry->nodes->GetDomain(kPoint)) LinSysRes.SubtractBlock(kPoint, flux);
      }
    }
    END_SU2_OMP_SAFE_GLOBAL_ACCESS
    return;
  }

  BEGIN_SU2_OMP_SAFE_GLOBAL_ACCESS {
    for (auto iSplit = 0ul; iSplit < splitNodes.size(); ++iSplit) {
      const auto iPoint = splitNodes[iSplit][0], kPoint = splitNodes[iSplit][1];
      su2double flux[MAXNVAR] = {};

      /*--- 0.5(F_i+F_k).n_ik + (F_k-F_i).E_ik with F - U v, these pairs have no dissipation and no Jacobian. ---*/

      addFlux(iPoint, splitNormal[iSplit], 0.5, flux);
      addFlux(kPoint, splitNormal[iSplit], 0.5, flux);
      addFlux(kPoint, splitE[iSplit], 1.0, flux);
      addFlux(iPoint, splitE[iSplit], -1.0, flux);

      if (sweptFlux) {
        const su2double omega = geometry->edges->GetSplitSwept()[iSplit];
        for (auto iVar = 0u; iVar < nVar; ++iVar) {
          flux[iVar] -= 0.5 * omega * (hatSolution(iPoint, iVar) + hatSolution(kPoint, iVar));
        }
      }
      if (turbMatched) addPairKine(iPoint, kPoint, flux);

      if (geometry->nodes->GetDomain(iPoint)) LinSysRes.AddBlock(iPoint, flux);
      if (geometry->nodes->GetDomain(kPoint)) LinSysRes.SubtractBlock(kPoint, flux);
    }

    if (nDim == 3) {
      for (auto iPoint = 0ul; iPoint < nPointDomain; ++iPoint) {
        su2double flux[MAXNVAR] = {};
        addFlux(iPoint, boundNormal[iPoint], 1.0, flux);
        if (turbMatched) {
          /*--- A nodal mass term: the k of the node itself. ---*/
          const su2double kFlux = tke(iPoint) * flux[0];
          flux[nDim + 1] += kFlux;
          P1KNodalRes[iPoint] += kFlux;
          P1KNodalJac[iPoint] += fmax(flux[0], 0.0) / nodes->GetDensity(iPoint);
        }
        LinSysRes.AddBlock(iPoint, flux);
      }
    }
  }
  END_SU2_OMP_SAFE_GLOBAL_ACCESS
}

void CEulerSolver::SetEdgeBlendLambda(const CGeometry *geometry, const CConfig *config, const CVariable* turbMatched) {
  SU2_ZONE_SCOPED

  /*--- EDGE_FORM_ES_BLEND, element theta blend of the Lemma 3 dissipation of the E part (P1_SECOND_ORDER.md, B2).
   * In a simplex T with shape gradients g_a and delta_a = W_a - |T|/(d+1), the E part of the pair ab is
   * E^T_ab = 0.5 (delta_a g_b + delta_b g_a), and sum_T E^T_ab = E_ab. With w the entropy variables of
   * U = -rho s/(gamma-1), the E part produces exactly P_T = sum_{a<b} (w_b - w_a).(F_b - F_a).E^T_ab in T (METHOD 2),
   * and lam^T_ab (U_b - U_a) produces -D_T, D_T = sum_{a<b} lam^T_ab (w_b - w_a).(U_b - U_a) >= 0, with
   * lam^T_ab = max(0, max(u_a.E, u_b.E) + max(c_a, c_b) |E|), E = E^T_ab (Lemma 3 on the piece). Lemma 3 gives
   * P_T <= D_T, so theta_T = P_T^+ / D_T is in [0,1] and P_T - theta_T D_T = min(P_T, 0) <= 0 for any states.
   * The sum over T of the pair production is the edge production, so the edge gets sum_T theta_T lam^T_ab on the
   * nodal jump, with theta frozen in the Jacobian. All terms use the nodal states of the edge loop.
   * MPI: an edge with an owned node has all its simplices on this rank (they contain that node), with halo primitives
   * that are up to date, so its coefficient equals the serial one. Edges between halo nodes are incomplete, as is
   * their residual.
   * VISC_P1_ENTROPY_MOVED (P1_VISCOUS_MOVED.md, MV5-MV7, MV13): one budget per simplex with the viscous defect of the
   * moved test vectors, E_T = P_T + P^v_T - D^gal_T + W_T, theta_v = min(1, E_T/P^v_T) first, then
   * theta_T = (E_T - theta_v P^v_T)^+ / D_T <= P_T^+ / D_T. theta_v is stored for the viscous element loop of the same
   * residual, frozen in its Jacobian. An element with an owned node is complete on this rank (halo primitives, all
   * its wall faces), so theta_v and theta_T equal the serial ones; elements without one only reach halo edges and
   * keep theta_v = 0 and the B2 value. ---*/

  const auto& delta = geometry->edges->GetBlendDelta();
  const auto& blendEdges = geometry->edges->GetBlendEdges();
  auto& lamBlend = nodes->EdgeBlendLambda;
  const unsigned short nNodes = nDim + 1;
  const bool movedVisc = config->GetVisc_P1_Entropy_Moved();

  /*--- GRID_ENTROPY_P1 (P1_ALE.md, Lemma A12 and 5): the E part of the pair is that of the relative flux
   * F - U vbar^T, vbar = {{v}}_ab, so P_T gets -(vbar.E)(w_b - w_a).(U_b - U_a) and lam the relative speed
   * max(u_a.E, u_b.E) - vbar.E + max(c_a, c_b)|E|. theta_T is computed on the current geometry (the coefficients of
   * SetEdgeFormBlend follow the mesh), so the bound P_T <= D_T holds at every instant. ---*/
  const bool relative = config->GetGrid_Entropy_P1() && dynamic_grid;

  BEGIN_SU2_OMP_SAFE_GLOBAL_ACCESS {
    lamBlend = su2double(0.0);

    /*--- The wall faces of the budget (W_T), built on first use (collective). ---*/
    if (movedVisc && config->GetBC_Wall_Entropy_P1()) UpdateP1WallFaces(geometry, config);

    for (auto iElem = 0ul; iElem < geometry->GetnElem(); ++iElem) {
      const auto* element = geometry->elem[iElem];

      /*--- EDGE_FORM_P1_MIXED: the split averages have E = 0, so P_C = 0 and no theta (P1_MIXED.md, X5 with M0). ---*/
      if (element->GetnNodes() != nNodes) continue;

      std::array<const su2double*, MAXNDIM + 1> coord;
      unsigned long point[MAXNDIM + 1];
      for (unsigned short a = 0; a < nNodes; ++a) {
        point[a] = element->GetNode(a);
        coord[a] = geometry->nodes->GetCoord(point[a]);
      }
      su2double grad[MAXNDIM + 1][3];
      P1Simplex::ShapeGradients(nDim, coord, grad);

      /*--- Nodal states as in the edge loop, from the primitives (T, u, p, rho, h): U from them as compressibleConservatives,
       * w = ((gamma-s)/(gamma-1) - beta|u|^2, 2 beta u, -2 beta) with beta = rho/(2p) and s = ln p - gamma ln rho. ---*/

      su2double density[MAXNDIM + 1], pressure[MAXNDIM + 1], enthalpy[MAXNDIM + 1], soundSpeed2[MAXNDIM + 1];
      su2double velocity[MAXNDIM + 1][MAXNDIM], U[MAXNDIM + 1][MAXNDIM + 2], w[MAXNDIM + 1][MAXNDIM + 2];

      for (unsigned short a = 0; a < nNodes; ++a) {
        density[a] = nodes->GetPrimitive(point[a], nDim + 2);
        pressure[a] = nodes->GetPrimitive(point[a], nDim + 1);
        enthalpy[a] = nodes->GetPrimitive(point[a], nDim + 3);
        if (turbMatched) enthalpy[a] -= turbMatched->GetSolution(point[a], 0);  // hat state, as the edge loop
        soundSpeed2[a] = Gamma * pressure[a] / density[a];

        const su2double beta = 0.5 * density[a] / pressure[a];
        const su2double entropy = log(pressure[a]) - Gamma * log(density[a]);
        su2double velocity2 = 0.0;
        U[a][0] = density[a];
        for (unsigned short iDim = 0; iDim < nDim; ++iDim) {
          velocity[a][iDim] = nodes->GetPrimitive(point[a], iDim + 1);
          velocity2 += velocity[a][iDim] * velocity[a][iDim];
          U[a][iDim + 1] = density[a] * velocity[a][iDim];
          w[a][iDim + 1] = 2.0 * beta * velocity[a][iDim];
        }
        U[a][nDim + 1] = density[a] * enthalpy[a] - pressure[a];
        w[a][0] = (Gamma - entropy) / Gamma_Minus_One - beta * velocity2;
        w[a][nDim + 1] = -2.0 * beta;
      }

      /*--- P_T and D_T over the pairs, pair order as in CPhysicalGeometry::SetEdgeFormBlend. ---*/

      su2double lam[MAXNDIM * (MAXNDIM + 1) / 2];
      su2double production = 0.0, dissipation = 0.0;
      unsigned short pair = 0;

      for (unsigned short a = 0; a < nNodes; ++a) {
        for (unsigned short b = a + 1; b < nNodes; ++b) {
          su2double E[MAXNDIM] = {0.0};
          for (unsigned short iDim = 0; iDim < nDim; ++iDim) {
            E[iDim] = 0.5 * (delta(iElem, a) * grad[b][iDim] + delta(iElem, b) * grad[a][iDim]);
          }
          const su2double areaE = GeometryToolbox::Norm(nDim, E);
          const su2double projVel_a = GeometryToolbox::DotProduct(nDim, velocity[a], E);
          const su2double projVel_b = GeometryToolbox::DotProduct(nDim, velocity[b], E);

          su2double projGridE = 0.0;
          if (relative) {
            const su2double* gridVel_a = geometry->nodes->GetGridVel(point[a]);
            const su2double* gridVel_b = geometry->nodes->GetGridVel(point[b]);
            for (unsigned short iDim = 0; iDim < nDim; ++iDim) projGridE += 0.5 * (gridVel_a[iDim] + gridVel_b[iDim]) * E[iDim];
          }

          const su2double maxProjVel = relative ? max(projVel_a, projVel_b) - projGridE : max(projVel_a, projVel_b);
          const su2double maxSoundSpeed = sqrt(max(soundSpeed2[a], soundSpeed2[b]));
          const su2double speed = maxProjVel + maxSoundSpeed * areaE;
          lam[pair] = max(su2double(0.0), speed);

          /*--- (w_b - w_a).(F_b - F_a).E with F.E = (rho u.E, rho u.E u + p E, rho u.E H), as inviscidProjFlux. ---*/
          const su2double mdot_a = density[a] * projVel_a;
          const su2double mdot_b = density[b] * projVel_b;
          su2double prodPair = (w[b][0] - w[a][0]) * (mdot_b - mdot_a);
          for (unsigned short iDim = 0; iDim < nDim; ++iDim) {
            prodPair += (w[b][iDim + 1] - w[a][iDim + 1]) *
                        (mdot_b * velocity[b][iDim] + pressure[b] * E[iDim] - mdot_a * velocity[a][iDim] - pressure[a] * E[iDim]);
          }
          prodPair += (w[b][nDim + 1] - w[a][nDim + 1]) * (mdot_b * enthalpy[b] - mdot_a * enthalpy[a]);

          su2double dissPair = 0.0;
          for (unsigned short iVar = 0; iVar < nDim + 2; ++iVar) dissPair += (w[b][iVar] - w[a][iVar]) * (U[b][iVar] - U[a][iVar]);
          if (relative) prodPair -= projGridE * dissPair;

          production += prodPair;
          dissipation += lam[pair] * dissPair;
          ++pair;
        }
      }

      /*--- VISC_P1_ENTROPY_MOVED: one budget with the viscous terms. ---*/

      if (movedVisc) {
        su2double viscProduction = 0.0, viscDissipation = 0.0, thetaV = 0.0, thetaC = 0.0;
        P1MovedThetaV[iElem] = 0.0;
        if (P1MovedViscousBudget(geometry, config, iElem, viscProduction, viscDissipation)) {
          P1ViscousEntropy::OneBudgetBlend(production, dissipation, viscProduction, viscDissipation, thetaV, thetaC);
          P1MovedThetaV[iElem] = thetaV;
          if (thetaC <= 0.0) continue;
          for (unsigned short iPair = 0; iPair < pair; ++iPair) {
            lamBlend(blendEdges(iElem, iPair)) += thetaC * lam[iPair];
          }
          continue;
        }
      }

      /*--- P_T <= D_T holds exactly, min(.,1) only guards round-off. theta = 0 if P_T <= 0 or D_T = 0. ---*/

      if ((production <= 0.0) || (dissipation <= 0.0)) continue;
      const su2double ratio = production / dissipation;
      const su2double theta = min(ratio, su2double(1.0));

      for (unsigned short iPair = 0; iPair < pair; ++iPair) {
        lamBlend(blendEdges(iElem, iPair)) += theta * lam[iPair];
      }
    }
  }
  END_SU2_OMP_SAFE_GLOBAL_ACCESS
}

namespace {
/*!
 * \brief Copy count values per point of a contiguous row major array (rows = points) from the owners to the halos,
 *        as CSolver::InitiateComms/CompleteComms for the solver quantities. Called by all threads.
 */
void CommPointData(CGeometry* geometry, const CConfig* config, su2double* data, unsigned short count) {
  geometry->AllocateP2PComms(count);
  su2double* bufDSend = geometry->bufD_P2PSend;

  if (geometry->nP2PSend > 0) {
    geometry->PostP2PRecvs(geometry, config, COMM_TYPE::DOUBLE, count, false);
    for (int iMessage = 0; iMessage < geometry->nP2PSend; iMessage++) {
      const auto msg_offset = geometry->nPoint_P2PSend[iMessage];
      const auto nSend = geometry->nPoint_P2PSend[iMessage + 1] - geometry->nPoint_P2PSend[iMessage];
      SU2_OMP_FOR_STAT(OMP_MIN_SIZE)
      for (long iSend = 0; iSend < static_cast<long>(nSend); iSend++) {
        const auto iPoint = geometry->Local_Point_P2PSend[msg_offset + iSend];
        const auto buf_offset = (msg_offset + iSend) * count;
        for (unsigned short k = 0; k < count; ++k) bufDSend[buf_offset + k] = data[iPoint * count + k];
      }
      END_SU2_OMP_FOR
      geometry->PostP2PSends(geometry, config, COMM_TYPE::DOUBLE, count, iMessage, false);
    }
  }

  static SU2_MPI::Status status;
  const su2double* bufDRecv = geometry->bufD_P2PRecv;

  if (geometry->nP2PRecv > 0) {
    for (int iMessage = 0; iMessage < geometry->nP2PRecv; iMessage++) {
      int ind = 0;
      SU2_OMP_SAFE_GLOBAL_ACCESS(SU2_MPI::Waitany(geometry->nP2PRecv, geometry->req_P2PRecv, &ind, &status);)
      const int jRecv = geometry->P2PRecv2Neighbor[status.MPI_SOURCE];
      const auto msg_offset = geometry->nPoint_P2PRecv[jRecv];
      const auto nRecv = geometry->nPoint_P2PRecv[jRecv + 1] - geometry->nPoint_P2PRecv[jRecv];
      SU2_OMP_FOR_STAT(OMP_MIN_SIZE)
      for (long iRecv = 0; iRecv < static_cast<long>(nRecv); iRecv++) {
        const auto iPoint = geometry->Local_Point_P2PRecv[msg_offset + iRecv];
        const auto buf_offset = (msg_offset + iRecv) * count;
        for (unsigned short k = 0; k < count; ++k) data[iPoint * count + k] = bufDRecv[buf_offset + k];
      }
      END_SU2_OMP_FOR
    }
#ifdef HAVE_MPI
    SU2_OMP_SAFE_GLOBAL_ACCESS(SU2_MPI::Waitall(geometry->nP2PSend, geometry->req_P2PSend, MPI_STATUS_IGNORE);)
#endif
  }
}
}  // namespace

void CEulerSolver::SetTeCNOAdjointGradient(CGeometry *geometry, const CConfig *config) {
  SU2_ZONE_SCOPED

  /*--- EDGE_FORM_TECNO_ADJOINT (TeCNOAdjoint, numerics/flow/tecno_adjoint.hpp): w on all nodes from the primitives
   * (halo primitives are up to date), G w on the owned nodes, copied to the halos. The limiter of w (optional) is
   * computed on the owned nodes and copied; phi_i is its minimum over the variables. Theorem A1 holds for any phi,
   * which is frozen in the residual (no derivative of phi is used anywhere). ---*/

  SU2_OMP_FOR_STAT(omp_chunk_size)
  for (auto iPoint = 0ul; iPoint < nPoint; ++iPoint) {
    /*--- The primitives of the edge loop, (T, u, p, rho, ...). ---*/
    su2double velocity[MAXNDIM] = {0.0};
    for (auto iDim = 0u; iDim < nDim; ++iDim) velocity[iDim] = nodes->GetPrimitive(iPoint, iDim + 1);
    TeCNOAdjoint::EntropyVariables(nDim, Gamma, nodes->GetPrimitive(iPoint, nDim + 2), velocity,
                                   nodes->GetPrimitive(iPoint, nDim + 1), TeCNOAdjW[iPoint]);
  }
  END_SU2_OMP_FOR

  auto& gradient = nodes->TeCNOAdjGradient;
  if (config->GetKind_TeCNO_Adj_Gradient() == TECNO_ADJ_GRADIENT::P1_AVERAGE) {
    TeCNOAdjoint::GradientP1(*geometry, nDim, nVar, TeCNOAdjEdgeA, TeCNOAdjVol, TeCNOAdjW, gradient);
  } else {
    TeCNOAdjoint::Gradient(*geometry, nDim, nVar, TeCNOAdjW, gradient);
  }
  CommPointData(geometry, config, gradient.data(), nVar * nDim);

  if (config->GetEdge_Form_TeCNO_Adjoint_Limiter()) {
    if (config->GetInnerIter() <= config->GetLimiterIter()) {
      computeLimiters(config->GetKind_SlopeLimit_Flow(), nullptr, MPI_QUANTITIES::SOLUTION_LIMITER, PERIODIC_NONE,
                      PERIODIC_NONE, *geometry, *config, 0, nVar, config->GetMUSCL_Kappa_Flow(), TeCNOAdjW, gradient,
                      TeCNOAdjMin, TeCNOAdjMax, TeCNOAdjLimiter);
      CommPointData(geometry, config, TeCNOAdjLimiter.data(), nVar);
      SU2_OMP_FOR_STAT(omp_chunk_size)
      for (auto iPoint = 0ul; iPoint < nPoint; ++iPoint) {
        su2double phi = TeCNOAdjLimiter(iPoint, 0);
        for (auto iVar = 1u; iVar < nVar; ++iVar) phi = min(phi, TeCNOAdjLimiter(iPoint, iVar));
        nodes->TeCNOAdjPhi(iPoint) = phi;
      }
      END_SU2_OMP_FOR
    }
  }
}

void CEulerSolver::JSTEntropyResidual(CGeometry *geometry, CSolver **solver_container, const CConfig *config) {
  SU2_ZONE_SCOPED

  namespace je = JSTEntropy;

  /*--- P1_JST_ENTROPY.md, Theorem J7. The edge loop stored c2 = eps2 lam and c4 = eps4 lam of every local edge (halo data
   * are the owners', so both ranks of an edge store the same numbers) and added -c4 (L_i - L_k), L the undivided
   * Laplacian of Ut (SetUndivided_Laplacian). Here: the matched Laplacian Lh_i = sum_{N*(i)} (c4/cb_i)(Ut_k - Ut_i),
   * cb_i the mean of c4 over N*(i), theta_i = B_i^+/(Pi4_i + D_i) with Pi4_i = -Lh_i . sum_{N*(i)} c4 (w_k - w_i), and on
   * every edge the correction to -c4 (chi_ik (1-theta_i) Lh_i - chi_ki (1-theta_k) Lh_k) + floor (U_i - U_k).
   * Ut = (rho, m, rho H), U conservative; with the matched SST form all of them of the hat state (H - k), as the edge
   * flux. N* (chi) uses JSTBoundaryNode, the owners' physical-boundary flags. ---*/

  const bool implicit = (config->GetKind_TimeIntScheme() == EULER_IMPLICIT);
  const su2double fixFactor = config->GetCent_Jac_Fix_Factor();
  const bool matched = config->GetTke_Matched_P1();
  const CVariable* turb = matched ? solver_container[TURB_SOL]->GetNodes() : nullptr;
  const auto& coeff = nodes->JSTEdgeCoeff;
  const auto& lapl = nodes->GetUndivided_Laplacian();

  auto nodeState = [&](unsigned long iPoint, je::NodeState& S) {
    const su2double* V = nodes->GetPrimitive(iPoint);
    su2double h = V[nDim + 3];
    if (turb) h -= turb->GetSolution(iPoint, 0);
    je::SetState(nDim, Gamma, V[nDim + 2], &V[1], V[nDim + 1], h, S);
  };
  /*--- The kernel's L (hat energy entry with the matched form), cancelled exactly by the correction. ---*/
  auto laplacian = [&](unsigned long iPoint, su2double* L) {
    for (unsigned short iVar = 0; iVar < nVar; ++iVar) L[iVar] = lapl(iPoint, iVar);
    if (matched) L[nVar - 1] -= lapl(iPoint, nVar);
  };
  /*--- chi_ik = [k in N*(i)]: all neighbours, or the boundary neighbours if i is on a physical boundary. ---*/
  auto inRing = [&](unsigned long iPoint, unsigned long kPoint) {
    return !((JSTBoundaryNode(iPoint) > 0.5) && !(JSTBoundaryNode(kPoint) > 0.5));
  };

  /*--- Node states once per residual (owned and halo nodes), allocated by the master only (all its uses are master-only). ---*/
  std::vector<je::NodeState> state;
  SU2_OMP_MASTER
  {
    state.resize(nPoint);
    for (unsigned long iPoint = 0; iPoint < nPoint; ++iPoint) nodeState(iPoint, state[iPoint]);
  }
  END_SU2_OMP_MASTER
  SU2_OMP_BARRIER

  /*--- a. Lh_i and cb_i of the owned nodes, then the owners' values on the halos. ---*/
  SU2_OMP_MASTER
  for (unsigned long iPoint = 0; iPoint < nPoint; ++iPoint) {
    for (unsigned short iVar = 0; iVar <= nVar; ++iVar) JSTLaplacian(iPoint, iVar) = 0.0;
    if (!geometry->nodes->GetDomain(iPoint)) continue;
    const auto& Si = state[iPoint];
    su2double sum[je::MAXNVAR] = {0.0}, cSum = 0.0;
    unsigned short nRing = 0;
    for (unsigned short iNeigh = 0; iNeigh < geometry->nodes->GetnPoint(iPoint); ++iNeigh) {
      const auto kPoint = geometry->nodes->GetPoint(iPoint, iNeigh);
      if (!inRing(iPoint, kPoint)) continue;
      const su2double c4 = coeff(geometry->nodes->GetEdge(iPoint, iNeigh), 1);
      const auto& Sk = state[kPoint];
      for (unsigned short iVar = 0; iVar < nVar; ++iVar) sum[iVar] += c4 * (Sk.Ut[iVar] - Si.Ut[iVar]);
      cSum += c4;
      ++nRing;
    }
    je::MatchedLaplacian(nVar, sum, cSum, nRing, JSTLaplacian[iPoint], JSTLaplacian(iPoint, nVar));
  }
  END_SU2_OMP_MASTER
  SU2_OMP_BARRIER

  CommPointData(geometry, config, JSTLaplacian.data(), nVar + 1);

  /*--- b. theta of the owned nodes: P2_i and D_i over all edges of i, Lam_i over N*(i), Pi4_i = -Lh_i . Lam_i. ---*/
  SU2_OMP_MASTER
  for (unsigned long iPoint = 0; iPoint < nPoint; ++iPoint) {
    JSTTheta(iPoint) = 0.0;
    if (!geometry->nodes->GetDomain(iPoint)) continue;
    const su2double ni = geometry->nodes->GetnNeighbor(iPoint);
    su2double P2 = 0.0, D = 0.0, Lam[je::MAXNVAR] = {0.0};
    for (unsigned short iNeigh = 0; iNeigh < geometry->nodes->GetnPoint(iPoint); ++iNeigh) {
      const auto kPoint = geometry->nodes->GetPoint(iPoint, iNeigh);
      const auto iEdge = geometry->nodes->GetEdge(iPoint, iNeigh);
      const su2double nk = geometry->nodes->GetnNeighbor(kPoint);
      const su2double c4 = coeff(iEdge, 1);
      je::AddEdge(nVar, state[iPoint], state[kPoint], coeff(iEdge, 0), c4, 0.5 * c4 * (ni + nk + 2.0),
                  inRing(iPoint, kPoint), P2, Lam, D);
    }
    JSTTheta(iPoint) = je::NodeTheta(nVar, P2, JSTLaplacian[iPoint], Lam, D);
  }
  END_SU2_OMP_MASTER
  SU2_OMP_BARRIER

  /*--- The owners' theta on the halos. ---*/
  CommPointData(geometry, config, JSTTheta.data(), 1);

  /*--- c. Correction edge fluxes on every edge with an owned end (R_i += f, R_k -= f). Jacobian with theta frozen in SU2's
   * surrogate convention (CorrectionJacobianFactors) through dUt/dU, plus the exact floor +-1/2 (theta_i + theta_k) mu I. ---*/
  SU2_OMP_MASTER
  {
  if (matched) JSTMassCorrection = su2double(0.0);
  su2activematrix Jii(nVar, nVar), Jik(nVar, nVar), Jki(nVar, nVar), Jkk(nVar, nVar);
  auto dUtdU = [&](const su2double* V, const su2double& factor, su2activematrix& J) {
    su2double vel2 = 0.0;
    for (unsigned short iDim = 0; iDim < nDim; ++iDim) vel2 += V[iDim + 1] * V[iDim + 1];
    for (unsigned short iVar = 0; iVar < nVar; ++iVar)
      for (unsigned short jVar = 0; jVar < nVar; ++jVar) J(iVar, jVar) = 0.0;
    for (unsigned short iVar = 0; iVar < nVar - 1; ++iVar) J(iVar, iVar) = factor;
    J(nVar - 1, nVar - 1) = factor * Gamma;
    J(nVar - 1, 0) = 0.5 * factor * Gamma_Minus_One * vel2;
    for (unsigned short iDim = 0; iDim < nDim; ++iDim) J(nVar - 1, iDim + 1) = -factor * Gamma_Minus_One * V[iDim + 1];
  };
  for (unsigned long iEdge = 0; iEdge < geometry->GetnEdge(); ++iEdge) {
    const auto iPoint = geometry->edges->GetNode(iEdge, 0);
    const auto kPoint = geometry->edges->GetNode(iEdge, 1);
    const bool own_i = geometry->nodes->GetDomain(iPoint), own_k = geometry->nodes->GetDomain(kPoint);
    if (!own_i && !own_k) continue;
    const su2double c4 = coeff(iEdge, 1);
    const su2double theta_i = JSTTheta(iPoint), theta_k = JSTTheta(kPoint);
    if ((c4 == 0.0) && (theta_i == 0.0) && (theta_k == 0.0)) {
      continue;  // no eps4 term and no floor on this edge
    }

    const auto& Si = state[iPoint];
    const auto& Sk = state[kPoint];
    const bool chi_ik = inRing(iPoint, kPoint), chi_ki = inRing(kPoint, iPoint);
    su2double L_i[je::MAXNVAR], L_k[je::MAXNVAR], f[je::MAXNVAR];
    laplacian(iPoint, L_i);
    laplacian(kPoint, L_k);
    const su2double ni = geometry->nodes->GetnNeighbor(iPoint), nk = geometry->nodes->GetnNeighbor(kPoint);
    const su2double mu = 0.5 * c4 * (ni + nk + 2.0);
    je::CorrectionFlux(nVar, c4, mu, theta_i, theta_k, chi_ik, chi_ki, JSTLaplacian[iPoint], JSTLaplacian[kPoint], L_i,
                       L_k, Si.U, Sk.U, f);

    /*--- Matched SST form: the mass part of the correction carries its upwind k, the same numbers in the SST k row. ---*/
    su2double kUp = 0.0, k_i = 0.0, k_k = 0.0;
    if (matched) {
      k_i = turb->GetSolution(iPoint, 0);
      k_k = turb->GetSolution(kPoint, 0);
      kUp = P1SSTMatched::KUpwind(f[0], k_i, k_k);
      f[nVar - 1] += kUp * f[0];
      JSTMassCorrection(iEdge) = f[0];
    }

    if (own_i) LinSysRes.AddBlock(iPoint, f);
    if (own_k) LinSysRes.SubtractBlock(kPoint, f);
    if (!implicit) continue;

    /*--- d f/dU_i and d f/dU_k (hat), then L(k_up) J T(k) with the matched form. ---*/
    su2double fii, fik;
    je::CorrectionJacobianFactors(c4, theta_i, theta_k, chi_ik, chi_ki, ni, nk, JSTLaplacian(iPoint, nVar),
                                  JSTLaplacian(kPoint, nVar), fii, fik);
    const su2double floor = 0.5 * (theta_i + theta_k) * mu;
    dUtdU(nodes->GetPrimitive(iPoint), fixFactor * fii, Jii);
    dUtdU(nodes->GetPrimitive(kPoint), fixFactor * fik, Jik);
    for (unsigned short iVar = 0; iVar < nVar; ++iVar) {
      Jii(iVar, iVar) += floor;
      Jik(iVar, iVar) -= floor;
    }
    if (matched) {
      P1SSTMatched::TransformJacobian(nVar, Jii, kUp, k_i);
      P1SSTMatched::TransformJacobian(nVar, Jik, kUp, k_k);
    }
    for (unsigned short iVar = 0; iVar < nVar; ++iVar) {
      for (unsigned short jVar = 0; jVar < nVar; ++jVar) {
        Jki(iVar, jVar) = -Jii(iVar, jVar);
        Jkk(iVar, jVar) = -Jik(iVar, jVar);
      }
    }
    if (own_i) {
      Jacobian.AddBlock2Diag(iPoint, Jii);
      Jacobian.AddBlock(iPoint, kPoint, Jik);
    }
    if (own_k) {
      Jacobian.AddBlock(kPoint, iPoint, Jki);
      Jacobian.AddBlock2Diag(kPoint, Jkk);
    }
  }
  }
  END_SU2_OMP_MASTER
  SU2_OMP_BARRIER
}

void CEulerSolver::TeCNOAdjointResidual(CGeometry *geometry, const CConfig *config) {
  SU2_ZONE_SCOPED

  /*--- EDGE_FORM_TECNO_ADJOINT: gradient part of R_m = sum_e c_{e,m} q_e (TeCNOAdjoint). The edge loop added the
   * Delta part and stored q_e. Y = Z/V on the owned nodes (all their edges are here), copied to the halos, then
   * R_m += -gam/2 sum_k n_mk.(Y_m + Y_k), gam = -1/2 rho (1-kappa) with the ramp of the edge loop (EDGE_ENTROPY_FCT:
   * gam = -1/2, the ramp is applied to the blend weights in the stored moments, tecnoAdjointEdgeFCT). ---*/

  const su2double ramp = config->GetMUSCLRampValue() * config->GetNewtonKrylovRelaxation();
  const bool fct = (config->GetKind_TeCNO_Adj_Limiter() == TECNO_ADJ_LIMITER::EDGE_ENTROPY_FCT);
  const su2double gam = fct ? TeCNOAdjoint::GradientCoefficient(1.0, 0.0)
                            : TeCNOAdjoint::GradientCoefficient(ramp, config->GetMUSCL_Kappa_Flow());
  const auto& phi = nodes->TeCNOAdjPhi;

  /*--- TECNO_ADJOINT_GRADIENT= P1_AVERAGE: Y = Z/Vhat and R_m += gam sum_k (a_km.Y_k - a_mk.Y_m) (G3). ---*/
  const bool p1Average = (config->GetKind_TeCNO_Adj_Gradient() == TECNO_ADJ_GRADIENT::P1_AVERAGE);

  /*--- TECNO_ADJOINT_LIMITER: Z_a = sum_e e_e (x) (B^T q)_{e,a} from the edge-end stores (no nodal phi). ---*/
  const bool edgeEnds = (config->GetKind_TeCNO_Adj_Limiter() != TECNO_ADJ_LIMITER::NONE);

  if (edgeEnds && p1Average) {
    TeCNOAdjoint::NodalMomentEnds(*geometry, nDim, nVar, nodes->TeCNOAdjEdgeQ, TeCNOAdjVol, TeCNOAdjY);
  } else if (edgeEnds) {
    auto volume = [&](unsigned long iPoint) { return geometry->nodes->GetVolume(iPoint); };
    TeCNOAdjoint::NodalMomentEnds(*geometry, nDim, nVar, nodes->TeCNOAdjEdgeQ, volume, TeCNOAdjY);
  } else if (p1Average) {
    TeCNOAdjoint::NodalMomentP1(*geometry, nDim, nVar, nodes->TeCNOAdjEdgeQ, phi, TeCNOAdjVol, TeCNOAdjY);
  } else {
    TeCNOAdjoint::NodalMoment(*geometry, nDim, nVar, nodes->TeCNOAdjEdgeQ, phi, TeCNOAdjY);
  }
  CommPointData(geometry, config, TeCNOAdjY.data(), nVar * nDim);
  if (p1Average) {
    TeCNOAdjoint::AddTransposeP1(*geometry, nDim, nVar, TeCNOAdjEdgeA, TeCNOAdjY, gam, LinSysRes);
  } else {
    TeCNOAdjoint::AddTranspose(*geometry, nDim, nVar, TeCNOAdjY, gam, LinSysRes);
  }

  /*--- EDGE_FORM_TECNO_A4_JACOBIAN: diagonal blocks 3/2 [sum_k |n_mk|/2 Phat_k + |sum_k n_mk|/2 Phat_m] dw/dU_m of
   * M#, the edge loop added the Delta blocks. ---*/

  if (config->GetEdge_Form_TeCNO_A4_Jacobian() && (config->GetKind_TimeIntScheme() == EULER_IMPLICIT)) {
    if (p1Average) {
      TeCNOAdjoint::MajorantNodalP1(*geometry, nDim, nVar, TeCNOAdjEdgeA, TeCNOAdjVol, nodes->TeCNOAdjEdgeD, phi, gam,
                                    TeCNOAdjP);
    } else {
      TeCNOAdjoint::MajorantNodal(*geometry, nDim, nVar, nodes->TeCNOAdjEdgeD, phi, gam, TeCNOAdjP);
    }
    CommPointData(geometry, config, TeCNOAdjP.data(), nVar * nVar);

    SU2_OMP_FOR_STAT(omp_chunk_size)
    for (auto iPoint = 0ul; iPoint < nPointDomain; ++iPoint) {
      TeCNOAdjoint::Block blockW, H, block;
      if (p1Average) {
        TeCNOAdjoint::MajorantDiagonalP1(*geometry, nDim, nVar, TeCNOAdjEdgeA, TeCNOAdjP, iPoint, blockW);
      } else {
        TeCNOAdjoint::MajorantDiagonal(*geometry, nDim, nVar, TeCNOAdjP, iPoint, blockW);
      }
      su2double velocity[MAXNDIM] = {0.0};
      for (auto iDim = 0u; iDim < nDim; ++iDim) velocity[iDim] = nodes->GetPrimitive(iPoint, iDim + 1);
      const su2double density = nodes->GetPrimitive(iPoint, nDim + 2);
      const su2double pressure = nodes->GetPrimitive(iPoint, nDim + 1);
      TeCNOAdjoint::EntropyHessian(nDim, Gamma, density, velocity, pressure, H);
      for (auto iVar = 0u; iVar < nVar; ++iVar) {
        for (auto jVar = 0u; jVar < nVar; ++jVar) {
          su2double d = 0.0;
          for (auto kVar = 0u; kVar < nVar; ++kVar) d += blockW(iVar, kVar) * H(kVar, jVar);
          block(iVar, jVar) = d;
        }
      }
      Jacobian.AddBlock2Diag(iPoint, block.v);
    }
    END_SU2_OMP_FOR
  }

  /*--- TECNO_ADJOINT_JACOBIAN= FIRST_RING (TeCNOAdjoint::FirstRingRow): rows of the owned nodes, J_w(m, n) dw/dU_n with
   * the edge blocks beta_{e,m} beta_{e,n} D_e and the diagonal majorant of the second-ring part, 2 J >= C^T D C;
   * FIRST_RING_JUMPS (TeCNOAdjoint::JumpRow): the edge-jump majorant, J >= C^T D C and J 1 = 0. The edge loop added
   * the central part and stored D_e. ---*/

  if ((config->GetKind_TeCNO_Adj_Jacobian() != TECNO_ADJ_JACOBIAN::ROE1) &&
      (config->GetKind_TimeIntScheme() == EULER_IMPLICIT)) {
    if ((config->GetKind_TeCNO_Adj_Limiter() == TECNO_ADJ_LIMITER::EDGE_ENTROPY_TVD) ||
        (config->GetKind_TeCNO_Adj_Limiter() == TECNO_ADJ_LIMITER::EDGE_ENTROPY_FCT)) {
      TeCNOAdjointJumpsTVDJacobian(geometry, config);
    } else {
      TeCNOAdjointFirstRingJacobian(geometry, config);
    }
  }
}

void CEulerSolver::TeCNOAdjointFirstRingJacobian(CGeometry *geometry, const CConfig *config) {
  SU2_ZONE_SCOPED

  /*--- Same coefficients as TeCNOAdjointResidual and the edge kernel. Not inlined into TeCNOAdjointResidual, so that
   * its default code generation is unchanged. ---*/
  const su2double ramp = config->GetMUSCLRampValue() * config->GetNewtonKrylovRelaxation();
  const su2double gam = TeCNOAdjoint::GradientCoefficient(ramp, config->GetMUSCL_Kappa_Flow());
  const auto& phi = nodes->TeCNOAdjPhi;
  const bool p1Average = (config->GetKind_TeCNO_Adj_Gradient() == TECNO_ADJ_GRADIENT::P1_AVERAGE);
  const su2double coefDelta = 1.0 - ramp * config->GetMUSCL_Kappa_Flow();
  auto hessian = [&](unsigned long iPoint, TeCNOAdjoint::Block& H) {
    su2double velocity[MAXNDIM] = {0.0};
    for (auto iDim = 0u; iDim < nDim; ++iDim) velocity[iDim] = nodes->GetPrimitive(iPoint, iDim + 1);
    TeCNOAdjoint::EntropyHessian(nDim, Gamma, nodes->GetPrimitive(iPoint, nDim + 2), velocity,
                                 nodes->GetPrimitive(iPoint, nDim + 1), H);
  };
  const bool jumps = (config->GetKind_TeCNO_Adj_Jacobian() == TECNO_ADJ_JACOBIAN::FIRST_RING_JUMPS);
  auto assemble = [&](const auto& weights, const auto& volume) {
    if (jumps) {
      /*--- FIRST_RING_JUMPS: S = (1/V, E, T, M) and Q on the owned nodes, copied to the halos. Edge weights of E:
       * w_f = trace(D_f H_f), H_f the mean entropy Hessian of the two nodes. ---*/
      auto edgeWeight = [&](unsigned long iEdge) {
        TeCNOAdjoint::Block H0, H1;
        hessian(geometry->edges->GetNode(iEdge, 0), H0);
        hessian(geometry->edges->GetNode(iEdge, 1), H1);
        su2double w = 0.0;
        for (auto iVar = 0u; iVar < nVar; ++iVar)
          for (auto jVar = 0u; jVar < nVar; ++jVar)
            w += nodes->TeCNOAdjEdgeD(iEdge, iVar * nVar + jVar) * (H0(jVar, iVar) + H1(jVar, iVar));
        return 0.5 * w;
      };
      TeCNOAdjoint::JumpNodal(*geometry, nDim, weights, volume, edgeWeight, TeCNOAdjS);
      CommPointData(geometry, config, TeCNOAdjS.data(), 3 + 2 * nDim * nDim);
      TeCNOAdjoint::JumpNodalQ(*geometry, nDim, nVar, weights, TeCNOAdjS, phi, coefDelta, gam, nodes->TeCNOAdjEdgeD,
                               TeCNOAdjP);
    } else {
      TeCNOAdjoint::FirstRingNodal(*geometry, nDim, weights, volume, TeCNOAdjS);
      CommPointData(geometry, config, TeCNOAdjS.data(), nDim + 1 + nDim * nDim);
      TeCNOAdjoint::FirstRingNodalP(*geometry, nDim, nVar, nodes->TeCNOAdjEdgeD, TeCNOAdjS, phi, gam, TeCNOAdjP);
    }
    CommPointData(geometry, config, TeCNOAdjP.data(), nVar * nVar);

    SU2_OMP_FOR_STAT(omp_chunk_size)
    for (auto iPoint = 0ul; iPoint < nPointDomain; ++iPoint) {
      su2double flat[TeCNOAdjoint::MAXNVAR * TeCNOAdjoint::MAXNVAR];
      auto toCons = [&](const TeCNOAdjoint::Block& blockW, unsigned long jPoint) {
        TeCNOAdjoint::Block H;
        hessian(jPoint, H);
        for (auto iVar = 0u; iVar < nVar; ++iVar) {
          for (auto jVar = 0u; jVar < nVar; ++jVar) {
            su2double d = 0.0;
            for (auto kVar = 0u; kVar < nVar; ++kVar) d += blockW(iVar, kVar) * H(kVar, jVar);
            flat[iVar * nVar + jVar] = d;
          }
        }
      };
      TeCNOAdjoint::Block diagW;
      auto addOff = [&](unsigned long jPoint, const TeCNOAdjoint::Block& offW) {
        toCons(offW, jPoint);
        Jacobian.AddBlock(iPoint, jPoint, flat);
      };
      if (jumps) {
        TeCNOAdjoint::JumpRow(*geometry, nDim, nVar, weights, TeCNOAdjS, TeCNOAdjP, nodes->TeCNOAdjEdgeD, phi,
                              coefDelta, gam, iPoint, diagW, addOff);
      } else {
        TeCNOAdjoint::FirstRingRow(*geometry, nDim, nVar, weights, TeCNOAdjS, TeCNOAdjP, nodes->TeCNOAdjEdgeD, phi,
                                   coefDelta, gam, iPoint, diagW, addOff);
      }
      toCons(diagW, iPoint);
      Jacobian.AddBlock(iPoint, iPoint, flat);
    }
    END_SU2_OMP_FOR
  };
  if (p1Average) {
    assemble(TeCNOAdjEdgeA, TeCNOAdjVol);
  } else {
    auto volume = [&](unsigned long iPoint) { return geometry->nodes->GetVolume(iPoint); };
    assemble(TeCNOAdjoint::GreenGaussWeights<CGeometry>{*geometry, nDim}, volume);
  }
}

void CEulerSolver::TeCNOAdjointJumpsTVDJacobian(CGeometry *geometry, const CConfig *config) {
  SU2_ZONE_SCOPED

  /*--- TeCNOAdjointFirstRingJacobian (FIRST_RING_JUMPS branch) with JumpNodalQEnds / JumpRowEnds: alpha and phi of each
   * edge end from the stored Phi of EDGE_ENTROPY_TVD (TeCNOAdjoint, FIRST_RING_JUMPS with EDGE_ENTROPY_TVD). With
   * EDGE_ENTROPY_FCT: JumpNodalQEndsFCT / JumpRowEndsFCT, the full-ramp constituents (coefDelta 1, gam -1/2) with the
   * stored phi_c and alphaBar >= 1. ---*/
  const bool fct = (config->GetKind_TeCNO_Adj_Limiter() == TECNO_ADJ_LIMITER::EDGE_ENTROPY_FCT);
  const su2double ramp = config->GetMUSCLRampValue() * config->GetNewtonKrylovRelaxation();
  const su2double gam = TeCNOAdjoint::GradientCoefficient(ramp, config->GetMUSCL_Kappa_Flow());
  const auto& edgePhi = nodes->TeCNOAdjEdgePhi;
  const bool p1Average = (config->GetKind_TeCNO_Adj_Gradient() == TECNO_ADJ_GRADIENT::P1_AVERAGE);
  const su2double coefDelta = 1.0 - ramp * config->GetMUSCL_Kappa_Flow();
  auto hessian = [&](unsigned long iPoint, TeCNOAdjoint::Block& H) {
    su2double velocity[MAXNDIM] = {0.0};
    for (auto iDim = 0u; iDim < nDim; ++iDim) velocity[iDim] = nodes->GetPrimitive(iPoint, iDim + 1);
    TeCNOAdjoint::EntropyHessian(nDim, Gamma, nodes->GetPrimitive(iPoint, nDim + 2), velocity,
                                 nodes->GetPrimitive(iPoint, nDim + 1), H);
  };
  auto assemble = [&](const auto& weights, const auto& volume) {
    auto edgeWeight = [&](unsigned long iEdge) {
      TeCNOAdjoint::Block H0, H1;
      hessian(geometry->edges->GetNode(iEdge, 0), H0);
      hessian(geometry->edges->GetNode(iEdge, 1), H1);
      su2double w = 0.0;
      for (auto iVar = 0u; iVar < nVar; ++iVar)
        for (auto jVar = 0u; jVar < nVar; ++jVar)
          w += nodes->TeCNOAdjEdgeD(iEdge, iVar * nVar + jVar) * (H0(jVar, iVar) + H1(jVar, iVar));
      return 0.5 * w;
    };
    TeCNOAdjoint::JumpNodal(*geometry, nDim, weights, volume, edgeWeight, TeCNOAdjS);
    CommPointData(geometry, config, TeCNOAdjS.data(), 3 + 2 * nDim * nDim);
    if (fct) {
      TeCNOAdjoint::JumpNodalQEndsFCT(*geometry, nDim, nVar, weights, TeCNOAdjS, edgePhi, nodes->TeCNOAdjEdgeD, TeCNOAdjP);
    } else {
      TeCNOAdjoint::JumpNodalQEnds(*geometry, nDim, nVar, weights, TeCNOAdjS, edgePhi, coefDelta, gam,
                                   nodes->TeCNOAdjEdgeD, TeCNOAdjP);
    }
    CommPointData(geometry, config, TeCNOAdjP.data(), nVar * nVar);

    SU2_OMP_FOR_STAT(omp_chunk_size)
    for (auto iPoint = 0ul; iPoint < nPointDomain; ++iPoint) {
      su2double flat[TeCNOAdjoint::MAXNVAR * TeCNOAdjoint::MAXNVAR];
      auto toCons = [&](const TeCNOAdjoint::Block& blockW, unsigned long jPoint) {
        TeCNOAdjoint::Block H;
        hessian(jPoint, H);
        for (auto iVar = 0u; iVar < nVar; ++iVar) {
          for (auto jVar = 0u; jVar < nVar; ++jVar) {
            su2double d = 0.0;
            for (auto kVar = 0u; kVar < nVar; ++kVar) d += blockW(iVar, kVar) * H(kVar, jVar);
            flat[iVar * nVar + jVar] = d;
          }
        }
      };
      TeCNOAdjoint::Block diagW;
      auto addOff = [&](unsigned long jPoint, const TeCNOAdjoint::Block& offW) {
        toCons(offW, jPoint);
        Jacobian.AddBlock(iPoint, jPoint, flat);
      };
      if (fct) {
        TeCNOAdjoint::JumpRowEndsFCT(*geometry, nDim, nVar, weights, TeCNOAdjS, TeCNOAdjP, nodes->TeCNOAdjEdgeD, edgePhi,
                                     iPoint, diagW, addOff);
      } else {
        TeCNOAdjoint::JumpRowEnds(*geometry, nDim, nVar, weights, TeCNOAdjS, TeCNOAdjP, nodes->TeCNOAdjEdgeD, edgePhi,
                                  coefDelta, gam, iPoint, diagW, addOff);
      }
      toCons(diagW, iPoint);
      Jacobian.AddBlock(iPoint, iPoint, flat);
    }
    END_SU2_OMP_FOR
  };
  if (p1Average) {
    assemble(TeCNOAdjEdgeA, TeCNOAdjVol);
  } else {
    auto volume = [&](unsigned long iPoint) { return geometry->nodes->GetVolume(iPoint); };
    assemble(TeCNOAdjoint::GreenGaussWeights<CGeometry>{*geometry, nDim}, volume);
  }
}

void CEulerSolver::TeCNOAdjointTVDLimiter(CGeometry *geometry, CSolver **solver_container, const CConfig *config) {
  SU2_ZONE_SCOPED

  /*--- TECNO_ADJOINT_LIMITER= EDGE_ENTROPY_TVD (TeCNOAdjoint, EDGE_ENTROPY_TVD block). MPI: (A) ingredients and (B) the
   * own clip on every local edge, from the current primitives and the copied gradients of w at both ends; (C) rho on the
   * owned nodes, whose incident edges, gradient weights and volumes are all local, copied to the halos; (D) Phi on every
   * local edge (a cut edge takes the owner's rho of its halo end). The values of the edges of owned nodes then equal the
   * serial ones. While TVDRecompute is false the store is frozen; it is compared bitwise with a snapshot at every call. ---*/

  const auto nEdge = geometry->GetnEdge();
  /*--- EDGE_ENTROPY_FCT (TeCNOAdjoint, EDGE_ENTROPY_FCT block): the same ingredients pass and freeze rule, the passes of
   * FCTPasses with the star exchanges through CommPointData, the store phi_c, alpha, gamma (12 values per edge). ---*/
  const bool fct = (config->GetKind_TeCNO_Adj_Limiter() == TECNO_ADJ_LIMITER::EDGE_ENTROPY_FCT);
  const string limName = fct ? "TECNO_ADJOINT_LIMITER= EDGE_ENTROPY_FCT" : "TECNO_ADJOINT_LIMITER= EDGE_ENTROPY_TVD";
  const unsigned short width = fct ? TeCNOAdjoint::FCT_STORE_WIDTH : 2 * TeCNOAdjoint::TVD_NFIELD;
  auto checksum = [&]() {
    su2double sum = 0.0;
    for (auto iEdge = 0ul; iEdge < nEdge; ++iEdge)
      for (auto k = 0u; k < width; ++k) sum += nodes->TeCNOAdjEdgePhi(iEdge, k);
    return sum;
  };

  if (!TeCNOAdjoint::TVDRecompute(TeCNOAdjTVDValid, config->GetInnerIter(), config->GetLimiterIter())) {
    SU2_OMP_MASTER {
      /*--- The frozen store must be bitwise equal to the snapshot of its last computation. ---*/
      bool same = true;
      for (auto iEdge = 0ul; (iEdge < nEdge) && same; ++iEdge) {
        for (auto k = 0u; k < width; ++k) {
          const passivedouble a = SU2_TYPE::GetValue(nodes->TeCNOAdjEdgePhi(iEdge, k));
          const passivedouble b = SU2_TYPE::GetValue(TeCNOAdjTVDSnapshot(iEdge, k));
          if (std::memcmp(&a, &b, sizeof(passivedouble)) != 0) same = false;
        }
      }
      if (!same) SU2_MPI::Error(limName + ": the frozen store changed.", CURRENT_FUNCTION);
      if (!TeCNOAdjTVDReported) {
        const su2double local = checksum();
        su2double global = 0.0;
        SU2_MPI::Allreduce(&local, &global, 1, MPI_DOUBLE, MPI_SUM, SU2_MPI::GetComm());
        if (rank == MASTER_NODE) {
          cout << limName << ": Phi computed " << TeCNOAdjTVDCount
               << " times, frozen after inner iteration " << TeCNOAdjTVDLastIter << ", checksum "
               << setprecision(17) << global << setprecision(6) << endl;
        }
        TeCNOAdjTVDReported = true;
      }
    }
    END_SU2_OMP_MASTER
    SU2_OMP_BARRIER
    return;
  }

  SU2_OMP_MASTER
  if (!TeCNOAdjTVDNumerics) {
    const CVariable* turbVars = solver_container[TURB_SOL] ? solver_container[TURB_SOL]->GetNodes() : nullptr;
    TeCNOAdjTVDNumerics.reset(CNumericsSIMD::CreateTeCNOAdjExtNumerics(*config, nDim, MESH_0, turbVars));
    if (!TeCNOAdjTVDNumerics)
      SU2_MPI::Error(limName + ": no extended Roe kernel for this gas model.", CURRENT_FUNCTION);
  }
  END_SU2_OMP_MASTER
  SU2_OMP_BARRIER

  /*--- (A) Ingredients of every local edge (per-edge store, no scattering), masked lanes repeat an edge. ---*/
  SU2_OMP_FOR_STAT(omp_chunk_size)
  for (auto k = 0ul; k < nEdge; k += Double::Size) {
    Int iEdge;
    Double mask;
    for (auto j = 0ul; j < Double::Size; ++j) {
      const bool in = (k + j < nEdge);
      mask[j] = in;
      iEdge[j] = in ? k + j : k;
    }
    TeCNOAdjTVDNumerics->TeCNOAdjEdgeIngredients(iEdge, *config, *geometry, *nodes, mask);
  }
  END_SU2_OMP_FOR

  if (fct) {
    TeCNOAdjointFCTPasses(geometry, config);
  } else {
    /*--- (B) Own clip, (C) node factors, (D) per-edge-end values. ---*/
    TeCNOAdjoint::TVDEdgeClip(*geometry, nDim, nodes->TeCNOAdjEdgeTVD, TeCNOAdjTVDClip);
    if (config->GetKind_TeCNO_Adj_Gradient() == TECNO_ADJ_GRADIENT::P1_AVERAGE) {
      TeCNOAdjoint::TVDNodeFactor(*geometry, nDim, TeCNOAdjEdgeA, TeCNOAdjVol, nodes->TeCNOAdjEdgeTVD, TeCNOAdjTVDClip,
                                  TeCNOAdjTVDRho);
    } else {
      auto volume = [&](unsigned long iPoint) { return geometry->nodes->GetVolume(iPoint); };
      TeCNOAdjoint::TVDNodeFactor(*geometry, nDim, TeCNOAdjoint::GreenGaussWeights<CGeometry>{*geometry, nDim}, volume,
                                  nodes->TeCNOAdjEdgeTVD, TeCNOAdjTVDClip, TeCNOAdjTVDRho);
    }
    CommPointData(geometry, config, TeCNOAdjTVDRho.data(), TeCNOAdjoint::TVD_NFIELD);
    TeCNOAdjoint::TVDEdgePhi(*geometry, TeCNOAdjTVDClip, TeCNOAdjTVDRho, nodes->TeCNOAdjEdgePhi);
  }

  SU2_OMP_MASTER {
    TeCNOAdjTVDSnapshot = nodes->TeCNOAdjEdgePhi;
    TeCNOAdjTVDLastIter = config->GetInnerIter();
    TeCNOAdjTVDCount++;
    TeCNOAdjTVDValid = true;
  }
  END_SU2_OMP_MASTER
  SU2_OMP_BARRIER
}

void CEulerSolver::TeCNOAdjointFCTPasses(CGeometry *geometry, const CConfig *config) {
  SU2_ZONE_SCOPED

  /*--- TECNO_ADJOINT_LIMITER= EDGE_ENTROPY_FCT: setup once (static mesh: degrees and sorted neighbour lists of the owned
   * nodes copied to the halos, star slots, upwind elements), then TeCNOAdjoint::FCTPasses on the ingredients of the
   * kernel and the entropy variables of SetTeCNOAdjointGradient (halos current), and the activity statistics of the
   * store (master: iterations 0, 1, 2, every 100 and LIMITER_ITER). ---*/
  auto comm = [&](su2activematrix& data) { CommPointData(geometry, config, data.data(), data.cols()); };
  if (!TeCNOAdjFCTSetup.ready) {
    SU2_OMP_MASTER
    TeCNOAdjoint::FCTSetupRows(*geometry, TeCNOAdjFCTSetup, TeCNOAdjFCTRows);
    END_SU2_OMP_MASTER
    SU2_OMP_BARRIER
    comm(TeCNOAdjFCTRows);
    SU2_OMP_MASTER {
      TeCNOAdjoint::FCTSetupFinish(*geometry, nDim, TeCNOAdjFCTRows, TeCNOAdjFCTSetup);
      TeCNOAdjFCTRows.resize(0, 0);
      TeCNOAdjoint::FCTWorkResize(*geometry, TeCNOAdjFCTSetup, TeCNOAdjFCTWork,
                                  config->GetKind_TeCNO_Adj_FCT_Bound() != TECNO_ADJ_FCT_BOUND::TWO_NODE);
    }
    END_SU2_OMP_MASTER
    SU2_OMP_BARRIER
  }
  const bool nodeInput = (config->GetKind_TeCNO_Adj_FCT_Bound() != TECNO_ADJ_FCT_BOUND::TWO_NODE);
  const bool lens = (config->GetKind_TeCNO_Adj_FCT_Bound() == TECNO_ADJ_FCT_BOUND::ENTROPY_LENS);
  if (config->GetKind_TeCNO_Adj_Gradient() == TECNO_ADJ_GRADIENT::P1_AVERAGE) {
    TeCNOAdjoint::FCTPasses(*geometry, nDim, TeCNOAdjEdgeA, TeCNOAdjVol, TeCNOAdjFCTSetup, nodes->TeCNOAdjEdgeTVD,
                            TeCNOAdjW, TeCNOAdjFCTWork, nodes->TeCNOAdjEdgePhi, comm, config->GetTeCNO_Adj_FCT_Passes(),
                            nodeInput, lens);
  } else {
    auto volume = [&](unsigned long iPoint) { return geometry->nodes->GetVolume(iPoint); };
    TeCNOAdjoint::FCTPasses(*geometry, nDim, TeCNOAdjoint::GreenGaussWeights<CGeometry>{*geometry, nDim}, volume,
                            TeCNOAdjFCTSetup, nodes->TeCNOAdjEdgeTVD, TeCNOAdjW, TeCNOAdjFCTWork,
                            nodes->TeCNOAdjEdgePhi, comm, config->GetTeCNO_Adj_FCT_Passes(), nodeInput, lens);
  }

  const auto iter = config->GetInnerIter();
  if ((iter <= 2) || (iter % 100 == 0) || (iter == config->GetLimiterIter())) {
    SU2_OMP_MASTER {
      su2double local[10], global[10];
      TeCNOAdjoint::FCTStatistics(*geometry, nDim, nodes->TeCNOAdjEdgeTVD, TeCNOAdjFCTWork.edge, nodes->TeCNOAdjEdgePhi,
                                  local);
      SU2_MPI::Allreduce(local, global, 10, MPI_DOUBLE, MPI_SUM, SU2_MPI::GetComm());
      if (rank == MASTER_NODE) {
        const su2double n = fmax(global[0], su2double(1.0));
        cout << "TECNO_ADJOINT_LIMITER= EDGE_ENTROPY_FCT: inner iteration " << iter << ", valid entries " << global[0]
             << ", Phi_e = alpha phi_c: act " << 100.0 * global[1] / n << " %, zeros " << 100.0 * global[2] / n
             << " %, mean " << global[3] / n << ", wmean " << global[4] / fmax(global[5], EPS)
             << "; Phi_eq: act " << 100.0 * global[6] / n << " %, zeros " << 100.0 * global[7] / n << " %, mean "
             << global[8] / n << ", wmean " << global[9] / fmax(global[5], EPS);
        if (config->GetTeCNO_Adj_FCT_Passes() > 1) cout << " (" << config->GetTeCNO_Adj_FCT_Passes() << " passes)";
        if (nodeInput) cout << (lens ? " (ENTROPY_LENS)" : " (NODE_INPUT)");
        cout << endl;
      }
    }
    END_SU2_OMP_MASTER
    SU2_OMP_BARRIER
  }
}

void CEulerSolver::Upwind_Residual(CGeometry *geometry, CSolver **solver_container,
                                   CNumerics **numerics_container, CConfig *config, unsigned short iMesh) {
  SU2_ZONE_SCOPED

  const bool ideal_gas = (config->GetKind_FluidModel() == STANDARD_AIR) ||
                         (config->GetKind_FluidModel() == IDEAL_GAS);
  const bool low_mach_corr = config->Low_Mach_Correction();
  const bool fluxCorrection = config->GetFluxCorrection();

  /*--- Use vectorization if the scheme supports it. ---*/
  if (ideal_gas && !low_mach_corr) {
    if (config->GetKind_Upwind_Flow() == UPWIND::ROE || config->GetKind_Upwind_Flow() == UPWIND::MSW ||
        config->GetKind_Upwind_Flow() == UPWIND::ROENEW) {
      if ((iMesh == MESH_0) && config->GetEdge_Form_ES_Blend()) SetEdgeBlendLambda(geometry, config);
      if ((iMesh == MESH_0) && config->GetEdge_Form_TeCNO_Adjoint() && config->GetMUSCL_Flow() &&
          ((config->GetKind_TeCNO_Adj_Limiter() == TECNO_ADJ_LIMITER::EDGE_ENTROPY_TVD) ||
           (config->GetKind_TeCNO_Adj_Limiter() == TECNO_ADJ_LIMITER::EDGE_ENTROPY_FCT))) {
        TeCNOAdjointTVDLimiter(geometry, solver_container, config);
      }
      EdgeFluxResidual(geometry, solver_container, config);
      if ((iMesh == MESH_0) && config->GetEdge_Form_TeCNO_Adjoint() && config->GetMUSCL_Flow()) {
        TeCNOAdjointResidual(geometry, config);
      }
      if ((iMesh == MESH_0) && config->GetSplit_Edge_Flux()) SplitEdgeResidual(geometry, config);
      if ((iMesh == MESH_0) && config->GetGCL_Exact_P1()) SweptBoundaryResidual(geometry, config);
      return;
    }
  }

  const bool implicit         = (config->GetKind_TimeIntScheme() == EULER_IMPLICIT);
  const bool bounded_scalar   = config->GetBounded_Scalar();

  const bool msw              = (config->GetKind_Upwind_Flow() == UPWIND::MSW);
  const bool roe_turkel       = (config->GetKind_Upwind_Flow() == UPWIND::TURKEL);
  const auto kind_dissipation = config->GetKind_RoeLowDiss();

  const bool muscl            = (config->GetMUSCL_Flow() && (iMesh == MESH_0));
  const bool limiter          = (config->GetKind_SlopeLimit_Flow() != LIMITER::NONE);
  /*--- SST: the total enthalpy of the cells contains k. ---*/
  const bool tkeInEnergy      = (config->GetKind_Turb_Model() == TURB_MODEL::SST);
  const CVariable* turbNodes  = tkeInEnergy ? solver_container[TURB_SOL]->GetNodes() : nullptr;
  const bool van_albada       = (config->GetKind_SlopeLimit_Flow() == LIMITER::VAN_ALBADA_EDGE);

  const su2double kappa       = config->GetMUSCL_Kappa_Flow();
  const su2double musclRamp   = config->GetMUSCLRampValue() * config->GetNewtonKrylovRelaxation();

  /*--- Non-physical counter. ---*/
  unsigned long counter_local = 0;
  SU2_OMP_MASTER
  ErrorCounter = 0;
  END_SU2_OMP_MASTER

  /*--- Pick one numerics object per thread. ---*/
  CNumerics* numerics = numerics_container[CONV_TERM + omp_get_thread_num()*MAX_TERMS];

  /*--- Static arrays of MUSCL-reconstructed primitives and secondaries (thread safety). ---*/
  su2double Primitive_i[MAXNVAR] = {0.0}, Primitive_j[MAXNVAR] = {0.0};
  su2double Secondary_i[MAXNVAR] = {0.0}, Secondary_j[MAXNVAR] = {0.0};

  /*--- For hybrid parallel AD, pause preaccumulation if there is shared reading of
  * variables, otherwise switch to the faster adjoint evaluation mode. ---*/
  bool pausePreacc = false;
  if (ReducerStrategy) pausePreacc = AD::PausePreaccumulation();
  else AD::StartNoSharedReading();

  /*--- Loop over edge colors. ---*/
  for (auto color : EdgeColoring)
  {
  /*--- Chunk size is at least OMP_MIN_SIZE and a multiple of the color group size. ---*/
  SU2_OMP_FOR_DYN(nextMultiple(OMP_MIN_SIZE, color.groupSize))
  for(auto k = 0ul; k < color.size; ++k) {

    auto iEdge = color.indices[k];

    /*--- Points in edge and normal vectors ---*/

    auto iPoint = geometry->edges->GetNode(iEdge,0);
    auto jPoint = geometry->edges->GetNode(iEdge,1);

    numerics->SetNormal(geometry->edges->GetNormal(iEdge));

    su2double tke_i = 0.0, tke_j = 0.0;
    if (tkeInEnergy) {
      tke_i = turbNodes->GetSolution(iPoint, 0);
      tke_j = turbNodes->GetSolution(jPoint, 0);
      numerics->SetTurbKineticEnergy(tke_i, tke_j);
    }

    auto Coord_i = geometry->nodes->GetCoord(iPoint);
    auto Coord_j = geometry->nodes->GetCoord(jPoint);

    /*--- Roe Turkel preconditioning ---*/

    if (roe_turkel) {
      numerics->SetVelocity2_Inf(GeometryToolbox::SquaredNorm(nDim, config->GetVelocity_FreeStream()));
    }

    /*--- Grid movement ---*/

    if (dynamic_grid) {
      numerics->SetGridVel(geometry->nodes->GetGridVel(iPoint),
                           geometry->nodes->GetGridVel(jPoint));
    }

    /*--- Get primitive and secondary variables ---*/

    auto V_i = nodes->GetPrimitive(iPoint);
    auto V_j = nodes->GetPrimitive(jPoint);

    const su2double* S_i = ideal_gas? nullptr : nodes->GetSecondary(iPoint);
    const su2double* S_j = ideal_gas? nullptr : nodes->GetSecondary(jPoint);

    /*--- Set them with or without high order reconstruction using MUSCL strategy. ---*/

    if (!muscl) {

      numerics->SetPrimitive(V_i, V_j);
      numerics->SetSecondary(S_i, S_j);

    }
    else {
      /*--- Reconstruction ---*/

      su2double Vector_ij[MAXNDIM] = {0.0};
      GeometryToolbox::Distance(nDim, Coord_j, Coord_i, Vector_ij);

      auto Gradient_i = nodes->GetGradient_Reconstruction(iPoint);
      auto Gradient_j = nodes->GetGradient_Reconstruction(jPoint);

      for (auto iVar = 0u; iVar < nPrimVarGrad; iVar++) {
        const su2double V_ij = V_j[iVar] - V_i[iVar];

        const su2double Project_Grad_i = MUSCL_Reconstruction(Gradient_i[iVar], Vector_ij, V_ij, kappa, musclRamp);
        const su2double Project_Grad_j = MUSCL_Reconstruction(Gradient_j[iVar], Vector_ij, V_ij, kappa, musclRamp);

        su2double lim_i = 1.0;
        su2double lim_j = 1.0;
        if (van_albada) {
          lim_i = LimiterHelpers<>::vanAlbadaFunction(Project_Grad_i, V_ij, EPS);
          lim_j = LimiterHelpers<>::vanAlbadaFunction(Project_Grad_j, V_ij, EPS);
        } else if (limiter) {
          lim_i = nodes->GetLimiter_Primitive(iPoint, iVar);
          lim_j = nodes->GetLimiter_Primitive(jPoint, iVar);
        }
        lim_i *= (1 - static_cast<passivedouble>(nodes->OutlierMitigation(iPoint)) / CEulerVariable::MAX_OUTLIER_MITIGATION);
        lim_j *= (1 - static_cast<passivedouble>(nodes->OutlierMitigation(jPoint)) / CEulerVariable::MAX_OUTLIER_MITIGATION);

        Primitive_i[iVar] = V_i[iVar] + 0.5 * lim_i * Project_Grad_i;
        Primitive_j[iVar] = V_j[iVar] - 0.5 * lim_j * Project_Grad_j;

      }

      /*--- Check for non-physical solutions after reconstruction. If found, use the
       cell-average value of the solution. This is a locally 1st order approximation,
       which is typically only active during the start-up of a calculation.
       Pressure and density are checked before the fluid model is evaluated with them. ---*/

      auto nonPositive = [](su2double value) { return !(value > 0.0) || !std::isfinite(SU2_TYPE::GetValue(value)); };
      bool neg_pres_or_rho_i = nonPositive(Primitive_i[prim_idx.Pressure()]) || nonPositive(Primitive_i[prim_idx.Density()]);
      bool neg_pres_or_rho_j = nonPositive(Primitive_j[prim_idx.Pressure()]) || nonPositive(Primitive_j[prim_idx.Density()]);

      if (!neg_pres_or_rho_i && !neg_pres_or_rho_j) {
        /*--- Recompute the reconstructed quantities in a thermodynamically consistent way.
         With SST the total enthalpy contains k, taken from the cells (as the vectorized reconstruction does). ---*/

        if (!ideal_gas || low_mach_corr || tkeInEnergy) {
          ComputeConsistentExtrapolation(GetFluidModel(), nDim, Primitive_i, Secondary_i, tke_i);
          ComputeConsistentExtrapolation(GetFluidModel(), nDim, Primitive_j, Secondary_j, tke_j);
        }

        /*--- Low-Mach number correction. ---*/

        if (low_mach_corr) {
          LowMachPrimitiveCorrection(GetFluidModel(), nDim, Primitive_i, Primitive_j, tke_i, tke_j);
        }

        /*--- The correction changes the face states, check them again. ---*/

        neg_pres_or_rho_i = nonPositive(Primitive_i[prim_idx.Pressure()]) || nonPositive(Primitive_i[prim_idx.Density()]);
        neg_pres_or_rho_j = nonPositive(Primitive_j[prim_idx.Pressure()]) || nonPositive(Primitive_j[prim_idx.Density()]);
      }

      su2double R = 1.0;
      if (!neg_pres_or_rho_i && !neg_pres_or_rho_j)
        R = sqrt(Primitive_j[prim_idx.Density()]/Primitive_i[prim_idx.Density()]);
      su2double sq_vel = 0.0;
      for (auto iDim = 0u; iDim < nDim; iDim++) {
        su2double RoeVelocity = (R * Primitive_j[iDim + prim_idx.Velocity()] +
                                 Primitive_i[iDim + prim_idx.Velocity()]) / (R+1);
        sq_vel += pow(RoeVelocity, 2);
      }
      su2double RoeEnthalpy = (R * Primitive_j[prim_idx.Enthalpy()] + Primitive_i[prim_idx.Enthalpy()]) / (R+1);

      const su2double RoeTke = (R * tke_j + tke_i) / (R+1);  // SST: the total enthalpy contains k
      /*--- Speed of sound of the Roe average and of each side (schemes such as HLLC use both). ---*/
      auto negSoundSpeed2 = [&](const su2double* prim, su2double tke) {
        const su2double vel2 = GeometryToolbox::SquaredNorm(nDim, &prim[prim_idx.Velocity()]);
        return (Gamma-1)*(prim[prim_idx.Enthalpy()] - 0.5*vel2 - tke) < 0.0;
      };
      const bool neg_sound_speed = ((Gamma-1)*(RoeEnthalpy-0.5*sq_vel-RoeTke) < 0.0) ||
                                   negSoundSpeed2(Primitive_i, tke_i) || negSoundSpeed2(Primitive_j, tke_j);
      bool bad_recon = neg_sound_speed || neg_pres_or_rho_i || neg_pres_or_rho_j;
      bad_recon = nodes->UpdateNonPhysicalEdgeCounter(iEdge, bad_recon);
      counter_local += bad_recon;

      numerics->SetPrimitive(bad_recon? V_i : Primitive_i,  bad_recon? V_j : Primitive_j);
      numerics->SetSecondary(bad_recon? S_i : Secondary_i,  bad_recon? S_j : Secondary_j);

      if (fluxCorrection) {
        numerics->SetCorrection(geometry->edges->GetCorrection_X(iEdge), geometry->edges->GetCorrection_Y(iEdge),
                                geometry->edges->GetCorrection_Z(iEdge));
        numerics->SetPrimVarGradient(Gradient_i, Gradient_j);
      }
//      /** average reconstructed prim state **/
//      su2double *Prim_i = bad_recon ? V_i : Primitive_i;
//      su2double *Prim_j = bad_recon ? V_j : Primitive_j;
//
//      su2double avgPrim[MAXNVAR] = {0.0};
//
//      auto** fa = new su2double* [nVar];
//      auto** fb = new su2double* [nVar];
//      auto** fc = new su2double* [nVar];
//      for (iVar = 0; iVar < nVar; ++iVar) {
//        fa[iVar] = new su2double [nVar];
//        fb[iVar] = new su2double [nVar];
//        fc[iVar] = new su2double [nVar];
//      }
//      for (iVar = 0; iVar < nPrimVar; ++iVar) {
//        avgPrim[iVar] = 0.5 * (Prim_i[iVar] + Prim_j[iVar]);
//      }
//
//      su2double a[MAXNDIM], b[MAXNDIM], c[MAXNDIM];
//      geometry->edges->GetCoorrection_X(iEdge,a);
//      geometry->edges->GetCoorrection_Y(iEdge,b);
//      geometry->edges->GetCoorrection_Z(iEdge,c);
//
//      GetInviscidProjJacPrim(&avgPrim[prim_idx.Velocity()],&avgPrim[prim_idx.Density()],
//                             &avgPrim[prim_idx.Enthalpy()],a,0.5, fa);
//
//      GetInviscidProjJacPrim(&avgPrim[prim_idx.Velocity()],&avgPrim[prim_idx.Density()],
//                             &avgPrim[prim_idx.Enthalpy()],b,0.5, fb);
//
//      GetInviscidProjJacPrim(&avgPrim[prim_idx.Velocity()],&avgPrim[prim_idx.Density()],
//                             &avgPrim[prim_idx.Enthalpy()],c,0.5, fc);
//
//
//
//      vector<unsigned short> index = {prim_idx.Density(), prim_idx.Velocity(), prim_idx.Pressure(), prim_idx.Pressure(), prim_idx.Pressure()};
//      index[2] = prim_idx.Velocity() + 1;
//      if (nDim == 3) index[3] = prim_idx.Velocity() + 2;
//      for (iVar = 0; iVar < nVar; ++iVar) {
//        fluxCorrection[iVar] = 0.0;
//        for (int jVar = 0; jVar < nVar; ++jVar) {
//          fluxCorrection[iVar] += fa[iVar][jVar] * (Gradient_i[index[jVar]][0] + Gradient_j[index[jVar]][0]) +
//                                  fb[iVar][jVar] * (Gradient_i[index[jVar]][1] + Gradient_j[index[jVar]][1]) +
//                                  (nDim == 3 ? fc[iVar][jVar] * (Gradient_i[index[jVar]][2] + Gradient_j[index[jVar]][2]) : 0.0);
//        }
//      }
//
////      if (ReducerStrategy) {
////        EdgeFluxes.SetBlock(iEdge, fluxCorrection);
////      }
////      else {
////        LinSysRes.AddBlock(iPoint, fluxCorrection);
////        LinSysRes.SubtractBlock(jPoint, fluxCorrection);
////      }
//
//      for (iVar = 0; iVar < nVar; ++iVar) {
//        delete [] fa[iVar];
//        delete [] fb[iVar];
//        delete [] fc[iVar];
//      }
//      delete [] fa;
//      delete [] fb;
//      delete [] fc;

    }

    /*--- Roe Low Dissipation Scheme ---*/

    if (kind_dissipation != NO_ROELOWDISS) {
      numerics->SetDissipation(nodes->GetRoe_Dissipation(iPoint),
                               nodes->GetRoe_Dissipation(jPoint));
    }
    if (msw || kind_dissipation == FD_DUCROS || kind_dissipation == NTS_DUCROS){
      numerics->SetSensor(nodes->GetSensor(iPoint),
                          nodes->GetSensor(jPoint));
    }
    if (kind_dissipation == NTS || kind_dissipation == NTS_DUCROS){
      numerics->SetCoord(Coord_i, Coord_j);
    }

    /*--- Compute the residual ---*/

    auto conv_residual = numerics->ComputeResidual(config);

    if (bounded_scalar) EdgeMassFluxes[iEdge] = conv_residual[0];

    /*--- Set the final value of the Roe dissipation coefficient ---*/

    if ((kind_dissipation != NO_ROELOWDISS) && (MGLevel != MESH_0)) {
      nodes->SetRoe_Dissipation(iPoint,numerics->GetDissipation());
      nodes->SetRoe_Dissipation(jPoint,numerics->GetDissipation());
    }

    /*--- Update residual value ---*/

    if (ReducerStrategy) {
      EdgeFluxes.SetBlock(iEdge, conv_residual);
    } else {
      LinSysRes.AddBlock(iPoint, conv_residual);
      LinSysRes.SubtractBlock(jPoint, conv_residual);
    }

    /*--- Viscous contribution, returns its Jacobians so that the matrix is updated once. ---*/

    const auto visc_residual = Viscous_Residual(
        iEdge, geometry, solver_container, numerics_container[VISC_TERM + omp_get_thread_num()*MAX_TERMS], config);

    if (implicit) UpdateJacobian(iEdge, iPoint, jPoint, conv_residual, visc_residual);
  }
  END_SU2_OMP_FOR
  } // end color loop

  FinalizeResidualComputation(geometry, pausePreacc, counter_local, config);
}

void CEulerSolver::ComputeConsistentExtrapolation(CFluidModel *fluidModel, unsigned short nDim,
                                                  su2double *primitive, su2double *secondary, su2double tke) {
  SU2_ZONE_SCOPED
  const CEulerVariable::CIndices<unsigned short> prim_idx(nDim, 0);
  const su2double density = primitive[prim_idx.Density()];
  const su2double pressure = primitive[prim_idx.Pressure()];
  const su2double velocity2 = GeometryToolbox::SquaredNorm(nDim, &primitive[prim_idx.Velocity()]);

  fluidModel->SetTDState_Prho(pressure, density);

  primitive[prim_idx.Temperature()] = fluidModel->GetTemperature();
  primitive[prim_idx.Enthalpy()] = fluidModel->GetStaticEnergy() + pressure / density + 0.5*velocity2 + tke;
  primitive[prim_idx.SoundSpeed()] = fluidModel->GetSoundSpeed();
  secondary[0] = fluidModel->GetdPdrho_e();
  secondary[1] = fluidModel->GetdPde_rho();

}

void CEulerSolver::LowMachPrimitiveCorrection(CFluidModel *fluidModel, unsigned short nDim,
                                              su2double *primitive_i, su2double *primitive_j,
                                              su2double tke_i, su2double tke_j) {
  SU2_ZONE_SCOPED
  unsigned short iDim;

  su2double velocity2_i = 0.0;
  su2double velocity2_j = 0.0;

  for (iDim = 0; iDim < nDim; iDim++) {
    velocity2_i += pow(primitive_i[iDim+1], 2);
    velocity2_j += pow(primitive_j[iDim+1], 2);
  }
  su2double mach_i = sqrt(velocity2_i)/primitive_i[nDim+4];
  su2double mach_j = sqrt(velocity2_j)/primitive_j[nDim+4];

  su2double z = min(max(mach_i,mach_j),1.0);
  velocity2_i = 0.0;
  velocity2_j = 0.0;
  for (iDim = 0; iDim < nDim; iDim++) {
    su2double vel_i_corr = ( primitive_i[iDim+1] + primitive_j[iDim+1] )/2.0
                     + z * ( primitive_i[iDim+1] - primitive_j[iDim+1] )/2.0;
    su2double vel_j_corr = ( primitive_i[iDim+1] + primitive_j[iDim+1] )/2.0
                     + z * ( primitive_j[iDim+1] - primitive_i[iDim+1] )/2.0;

    velocity2_i += pow(vel_i_corr, 2);
    velocity2_j += pow(vel_j_corr, 2);

    primitive_i[iDim+1] = vel_i_corr;
    primitive_j[iDim+1] = vel_j_corr;
  }

  fluidModel->SetEnergy_Prho(primitive_i[nDim+1], primitive_i[nDim+2]);
  primitive_i[nDim+3]= fluidModel->GetStaticEnergy() + primitive_i[nDim+1]/primitive_i[nDim+2] + 0.5*velocity2_i + tke_i;

  fluidModel->SetEnergy_Prho(primitive_j[nDim+1], primitive_j[nDim+2]);
  primitive_j[nDim+3]= fluidModel->GetStaticEnergy() + primitive_j[nDim+1]/primitive_j[nDim+2] + 0.5*velocity2_j + tke_j;

}

void CEulerSolver::Source_Residual(CGeometry *geometry, CSolver **solver_container,
                                   CNumerics **numerics_container, CConfig *config, unsigned short iMesh) {
  SU2_ZONE_SCOPED

  const bool implicit         = config->GetKind_TimeIntScheme() == EULER_IMPLICIT;
  const bool viscous          = config->GetViscous();
  const bool rotating_frame   = config->GetRotating_Frame();
  const bool axisymmetric     = config->GetAxisymmetric();
  const bool gravity          = (config->GetGravityForce() == YES);
  const bool harmonic_balance = (config->GetTime_Marching() == TIME_MARCHING::HARMONIC_BALANCE);
  const bool body_force       = config->GetBody_Force();
  const bool vorticity_confinement = config->GetVorticityConfinement();
  const bool ideal_gas        = (config->GetKind_FluidModel() == STANDARD_AIR) ||
                                (config->GetKind_FluidModel() == IDEAL_GAS);
  const bool rans             = (config->GetKind_Turb_Model() != TURB_MODEL::NONE);

  /*--- Pick one numerics object per thread. ---*/
  CNumerics* numerics = numerics_container[SOURCE_FIRST_TERM + omp_get_thread_num()*MAX_TERMS];

  if (body_force) {

    /*--- Loop over all points ---*/
    AD::StartNoSharedReading();
    SU2_OMP_FOR_STAT(omp_chunk_size)
    for (auto iPoint = 0ul; iPoint < nPointDomain; iPoint++) {

      /*--- Load the conservative variables ---*/
      numerics->SetConservative(nodes->GetSolution(iPoint),
                                nodes->GetSolution(iPoint));

      /*--- Load the volume of the dual mesh cell ---*/
      numerics->SetVolume(geometry->nodes->GetVolume(iPoint));

      /*--- Compute the rotating frame source residual ---*/
      auto residual = numerics->ComputeResidual(config);

      /*--- Add the source residual to the total ---*/
      LinSysRes.AddBlock(iPoint, residual);

    }
    END_SU2_OMP_FOR
    AD::EndNoSharedReading();
  }

  if (rotating_frame) {

    /*--- Include the residual contribution from GCL due to the static
     mesh movement that is set for rotating frame. ---*/

    SetRotatingFrame_GCL(geometry, config);

    /*--- Loop over all points ---*/
    AD::StartNoSharedReading();
    SU2_OMP_FOR_DYN(omp_chunk_size)
    for (auto iPoint = 0ul; iPoint < nPointDomain; iPoint++) {

      /*--- Load the conservative variables ---*/
      numerics->SetConservative(nodes->GetSolution(iPoint),
                                nodes->GetSolution(iPoint));

      /*--- Load the volume of the dual mesh cell ---*/
      numerics->SetVolume(geometry->nodes->GetVolume(iPoint));

      /*--- Compute the rotating frame source residual ---*/
      auto residual = numerics->ComputeResidual(config);

      /*--- Add the source residual to the total ---*/
      LinSysRes.AddBlock(iPoint, residual);

      /*--- Add the implicit Jacobian contribution ---*/
      if (implicit) Jacobian.AddBlock2Diag(iPoint, residual.jacobian_i);

    }
    END_SU2_OMP_FOR
    AD::EndNoSharedReading();
  }

  if (axisymmetric) {

    const bool axiMatchedK = (iMesh == MESH_0) && config->GetTke_Matched_P1();
    if (axiMatchedK && (P1KNodalRes.size() != nPoint))
      SU2_MPI::Error("Matched k: the nodal k sums are not allocated (CONV_NUM_METHOD_FLOW= JST needed).", CURRENT_FUNCTION);

    /*--- SST_ENERGY_CONSISTENT: the k diffusion of the energy flux has the axisymmetric part (1/y) (-D_k dk/dy) of
     * div(-D_k grad k) in cylindrical coordinates, the same term as the k row of CSourcePieceWise_TurbSST
     * (ResidualAxisymmetricConvectionDiffusion: D_k = mu + sigma_k(F1) mu_t, the turbulence gradient of k), so the energy
     * row holds the same discrete k diffusion (M3) node by node. ---*/
    const bool axiKineDiffusion = (iMesh == MESH_0) && viscous && config->GetSST_Energy_Consistent() &&
                                  (config->GetKind_Turb_Model() == TURB_MODEL::SST);
    const CVariable* axiTurbNodes = axiKineDiffusion ? solver_container[TURB_SOL]->GetNodes() : nullptr;
    const su2double* sstConstants = axiKineDiffusion ? solver_container[TURB_SOL]->GetConstants() : nullptr;

    /*--- For viscous problems, we need an additional gradient. ---*/
    if (viscous) {
      ComputeAxisymmetricAuxGradients(geometry, config);
    }

    /*--- loop over points ---*/
    AD::StartNoSharedReading();
    SU2_OMP_FOR_DYN(omp_chunk_size)
    for (auto iPoint = 0ul; iPoint < nPointDomain; iPoint++) {

      /*--- Set solution  ---*/
      numerics->SetConservative(nodes->GetSolution(iPoint), nodes->GetSolution(iPoint));

      /*--- Set control volume ---*/
      numerics->SetVolume(geometry->nodes->GetVolume(iPoint));

      /*--- Set y coordinate ---*/
      numerics->SetCoord(geometry->nodes->GetCoord(iPoint), geometry->nodes->GetCoord(iPoint));

      /*--- Set primitive variables for viscous terms and/or generalised source ---*/
      if (!ideal_gas || viscous) numerics->SetPrimitive(nodes->GetPrimitive(iPoint), nodes->GetPrimitive(iPoint));

      /*--- Set secondary variables for generalised source ---*/
      if (!ideal_gas) numerics->SetSecondary(nodes->GetSecondary(iPoint), nodes->GetSecondary(iPoint));

      if (viscous) {

        /*--- Set gradient of primitive variables ---*/
        numerics->SetPrimVarGradient(nodes->GetGradient_Primitive(iPoint), nodes->GetGradient_Primitive(iPoint));

        /*--- Set gradient of auxillary variables ---*/
        numerics->SetAuxVarGrad(nodes->GetAuxVarGradient(iPoint), nullptr);

        /*--- Set turbulence kinetic energy ---*/
        if (config->GetKind_Turb_Model() == TURB_MODEL::SST){
          CVariable* turbNodes = solver_container[TURB_SOL]->GetNodes();
          numerics->SetTurbKineticEnergy(turbNodes->GetSolution(iPoint,0), turbNodes->GetSolution(iPoint,0));
        }
      }

      /*--- Compute Source term Residual ---*/
      auto residual = numerics->ComputeResidual(config);

      /*--- Add Residual ---*/
      LinSysRes.AddBlock(iPoint, residual);

      /*--- Matched k (SSTPATHS_PLAN.md 5): S_E = Shat_E + k_i S_rho,i (H = Hhat + k, both blend branches), the same
       * k_i S_rho,i goes to the SST k row, which drops its own convective axisymmetric k term. ---*/
      /*--- Same guards as the SST source: no source at the wall nodes (dist <= 1e-10, strong k) or below y = EPS. ---*/
      if (axiKineDiffusion && (geometry->nodes->GetWall_Distance(iPoint) > 1e-10) &&
          !(geometry->nodes->GetCoord(iPoint, 1) < EPS)) {
        const su2double yinv = 1.0 / geometry->nodes->GetCoord(iPoint, 1);
        const su2double F1 = axiTurbNodes->GetF1blending(iPoint);
        const su2double sigma_k = F1 * sstConstants[0] + (1.0 - F1) * sstConstants[1];
        const su2double cdk_axi = -(nodes->GetLaminarViscosity(iPoint) + sigma_k * nodes->GetEddyViscosity(iPoint)) *
                                  axiTurbNodes->GetGradient(iPoint, 0, 1);
        LinSysRes(iPoint, nVar - 1) += yinv * geometry->nodes->GetVolume(iPoint) * cdk_axi;
      }

      if (axiMatchedK) {
        const su2double kNode = solver_container[TURB_SOL]->GetNodes()->GetSolution(iPoint, 0);
        P1KNodalRes[iPoint] += kNode * residual[0];
        P1KNodalJac[iPoint] += fmax(residual[0] / nodes->GetDensity(iPoint), 0.0);
      }

      /*--- Implicit part ---*/
      if (implicit)
        Jacobian.AddBlock2Diag(iPoint, residual.jacobian_i);
    }
    END_SU2_OMP_FOR

    AD::EndNoSharedReading();
  }

  AD::StartNoSharedReading();

  if (gravity) {

    /*--- loop over points ---*/
    SU2_OMP_FOR_DYN(omp_chunk_size)
    for (auto iPoint = 0ul; iPoint < nPointDomain; iPoint++) {

      /*--- Set solution  ---*/
      numerics->SetConservative(nodes->GetSolution(iPoint), nodes->GetSolution(iPoint));

      /*--- Set control volume ---*/
      numerics->SetVolume(geometry->nodes->GetVolume(iPoint));

      /*--- Compute Source term Residual ---*/
      auto residual = numerics->ComputeResidual(config);

      /*--- Add Residual ---*/
      LinSysRes.AddBlock(iPoint, residual);

    }
    END_SU2_OMP_FOR

  }

  if (harmonic_balance) {

    /*--- loop over points ---*/
    SU2_OMP_FOR_STAT(omp_chunk_size)
    for (auto iPoint = 0ul; iPoint < nPointDomain; iPoint++) {

      /*--- Get control volume ---*/
      su2double Volume = geometry->nodes->GetVolume(iPoint);

      /*--- Get stored time spectral source term and add to residual ---*/
      for (auto iVar = 0ul; iVar < nVar; iVar++) {
        LinSysRes(iPoint,iVar) += Volume * nodes->GetHarmonicBalance_Source(iPoint,iVar);
      }
    }
    END_SU2_OMP_FOR
  }

  if (vorticity_confinement) {

    CNumerics* second_numerics = numerics_container[SOURCE_SECOND_TERM + omp_get_thread_num()*MAX_TERMS];

    /*--- calculate and set the average volume ---*/
    const su2double AvgVolume = config->GetDomainVolume() / geometry->GetGlobal_nPointDomain();
    second_numerics->SetAvgVolume(AvgVolume);

    /*--- set vorticity magnitude as auxilliary variable ---*/
    SU2_OMP_FOR_STAT(omp_chunk_size)
    for (auto iPoint = 0ul; iPoint < nPoint; iPoint++) {
      const su2double VorticityMag = max(GeometryToolbox::Norm(3, nodes->GetVorticity(iPoint)), 1e-12);
      nodes->SetAuxVar(iPoint, 0, VorticityMag);
    }
    END_SU2_OMP_FOR

    /*--- calculate the gradient of the vorticity magnitude (AuxVarGradient) ---*/
    SetAuxVar_Gradient_GG(geometry, config);

    SU2_OMP_FOR_DYN(omp_chunk_size)
    for (auto iPoint = 0ul; iPoint < nPointDomain; iPoint++) {
      second_numerics->SetPrimitive(nodes->GetPrimitive(iPoint), nullptr);
      second_numerics->SetVorticity(nodes->GetVorticity(iPoint), nullptr);
      second_numerics->SetAuxVarGrad(nodes->GetAuxVarGradient(iPoint), nullptr);
      second_numerics->SetDistance(geometry->nodes->GetWall_Distance(iPoint), 0);
      second_numerics->SetVolume(geometry->nodes->GetVolume(iPoint));
      auto residual = second_numerics->ComputeResidual(config);

      LinSysRes.AddBlock(iPoint, residual);

      if (implicit) Jacobian.AddBlock2Diag(iPoint, residual.jacobian_i);
    }
    END_SU2_OMP_FOR

  }

  /*--- Check if a verification solution is to be computed. ---*/

  if ( VerificationSolution ) {
    if ( VerificationSolution->IsManufacturedSolution() ) {

      /*--- Get the physical time. ---*/
      su2double time = 0.0;
      if (config->GetTime_Marching() != TIME_MARCHING::STEADY) time = config->GetPhysicalTime();

      /*--- Loop over points ---*/
      SU2_OMP_FOR_DYN(omp_chunk_size)
      for (auto iPoint = 0ul; iPoint < nPointDomain; iPoint++) {

        /*--- Get control volume size. ---*/
        su2double Volume = geometry->nodes->GetVolume(iPoint);

        /*--- Get the current point coordinates. ---*/
        const su2double *coor = geometry->nodes->GetCoord(iPoint);

        /*--- Get the MMS source term. ---*/
        vector<su2double> sourceMan(nVar,0.0);
        VerificationSolution->GetMMSSourceTerm(coor, time, sourceMan.data());

        /*--- Compute the residual for this control volume and subtract. ---*/
        for (auto iVar = 0ul; iVar < nVar; iVar++) {
          LinSysRes(iPoint,iVar) -= sourceMan[iVar]*Volume;
        }
      }
      END_SU2_OMP_FOR
    }
  }

  AD::EndNoSharedReading();

  /*--- Custom user defined source term (from the python wrapper) ---*/
  if (config->GetPyCustomSource() ) {
    CustomSourceResidual(geometry, solver_container, numerics_container, config, iMesh);
  }

}

void CEulerSolver::Source_Template(CGeometry *geometry, CSolver **solver_container, CNumerics *numerics,
                                   CConfig *config, unsigned short iMesh) {
  SU2_ZONE_SCOPED

  /* This method should be used to call any new source terms for a particular problem*/
  /* This method calls the new child class in CNumerics, where the new source term should be implemented.  */

  /* Next we describe how to get access to some important quanties for this method */
  /* Access to all points in the current geometric mesh by saying: nPointDomain */
  /* Get the vector of conservative variables at some point iPoint = nodes->GetSolution(iPoint) */
  /* Get the volume (or area in 2D) associated with iPoint = nodes->GetVolume(iPoint) */
  /* Get the vector of geometric coordinates of point iPoint = nodes->GetCoord(iPoint) */

}

void CEulerSolver::SetMax_Eigenvalue(CGeometry *geometry, const CConfig *config) {
  SU2_ZONE_SCOPED

  /*--- Define an object to compute the speed of sound. ---*/
  struct SoundSpeed {
    FORCEINLINE su2double operator() (const CEulerVariable& nodes, unsigned long iPoint, unsigned long jPoint) const {
      return 0.5 * (nodes.GetSoundSpeed(iPoint) + nodes.GetSoundSpeed(jPoint));
    }

    FORCEINLINE su2double operator() (const CEulerVariable& nodes, unsigned long iPoint) const {
      return nodes.GetSoundSpeed(iPoint);
    }

  } soundSpeed;

  /*--- Instantiate generic implementation. ---*/

  SetMax_Eigenvalue_impl(soundSpeed, geometry, config);

}

void CEulerSolver::SetUndivided_Laplacian(CGeometry *geometry, const CConfig *config) {
  SU2_ZONE_SCOPED

  /*--- Loop domain points. ---*/

  SU2_OMP_FOR_DYN(omp_chunk_size)
  for (unsigned long iPoint = 0; iPoint < nPointDomain; ++iPoint) {

    const bool boundary_i = geometry->nodes->GetPhysicalBoundary(iPoint);
    const su2double Pressure_i = nodes->GetPressure(iPoint);

    /*--- Initialize. ---*/
    for (unsigned short iVar = 0; iVar < nVar; iVar++) nodes->SetUnd_Lapl(iPoint, iVar, 0.0);
    if (nodes->GetUndivided_Laplacian().cols() > nVar) nodes->SetUnd_Lapl(iPoint, nVar, 0.0);

    /*--- Loop over the neighbors of point i. ---*/
    for (auto jPoint : geometry->nodes->GetPoints(iPoint)) {
      bool boundary_j = geometry->nodes->GetPhysicalBoundary(jPoint);

      /*--- If iPoint is boundary it only takes contributions from other boundary points. ---*/
      if (boundary_i && !boundary_j) continue;

      /*--- Add solution differences, with correction for compressible flows which use the enthalpy. ---*/

      for (unsigned short iVar = 0; iVar < nVar; iVar++)
        nodes->AddUnd_Lapl(iPoint, iVar, nodes->GetSolution(jPoint,iVar)-nodes->GetSolution(iPoint,iVar));

      su2double Pressure_j = nodes->GetPressure(jPoint);
      nodes->AddUnd_Lapl(iPoint, nVar - 1, Pressure_j - Pressure_i);
      if (nodes->GetUndivided_Laplacian().cols() > nVar)
        nodes->AddUnd_Lapl(iPoint, nVar, nodes->GetRhoTke(jPoint) - nodes->GetRhoTke(iPoint));
    }
  }
  END_SU2_OMP_FOR

  /*--- Correct the Laplacian across any periodic boundaries. ---*/

  for (unsigned short iPeriodic = 1; iPeriodic <= config->GetnMarker_Periodic()/2; iPeriodic++) {
    InitiatePeriodicComms(geometry, config, iPeriodic, PERIODIC_LAPLACIAN);
    CompletePeriodicComms(geometry, config, iPeriodic, PERIODIC_LAPLACIAN);
  }

  /*--- MPI parallelization ---*/

  InitiateComms(geometry, config, MPI_QUANTITIES::UNDIVIDED_LAPLACIAN);
  CompleteComms(geometry, config, MPI_QUANTITIES::UNDIVIDED_LAPLACIAN);

}

void CEulerSolver::SetCentered_Dissipation_Sensor(CGeometry *geometry, const CConfig *config) {
  SU2_ZONE_SCOPED

  /*--- Define an object for the sensor variable, pressure. ---*/
  struct SensVar {
    FORCEINLINE su2double operator() (const CEulerVariable& nodes, unsigned long iPoint) const {
      return nodes.GetPressure(iPoint);
    }
  } sensVar;

  /*--- Instantiate generic implementation. ---*/
  SetCentered_Dissipation_Sensor_impl(sensVar, geometry, config);
}

void CEulerSolver::SetUpwind_Ducros_Sensor(CGeometry *geometry, CConfig *config){
  SU2_ZONE_SCOPED

  SU2_OMP_FOR_STAT(omp_chunk_size)
  for (unsigned long iPoint = 0; iPoint < geometry->GetnPoint(); iPoint++) {

    /*---- Ducros sensor for iPoint and its neighbor points to avoid lower dissipation near shocks. ---*/

    su2double Ducros_i = 0.0;
    const auto nNeigh = geometry->nodes->GetnPoint(iPoint);

    for (unsigned short iNeigh = 0; iNeigh <= nNeigh; iNeigh++) {

      auto jPoint = iPoint; // when iNeigh == nNeigh
      if (iNeigh < nNeigh) jPoint = geometry->nodes->GetPoint(iPoint, iNeigh);

      /*---- Dilatation for jPoint ---*/

      su2double uixi=0.0;
      for(unsigned short iDim = 0; iDim < nDim; iDim++){
        uixi += nodes->GetGradient_Primitive(jPoint,iDim+1, iDim);
      }

      /*--- Compute norm of vorticity ---*/

      const su2double* Vorticity = nodes->GetVorticity(jPoint);
      su2double Omega = 0.0;
      for (unsigned short iDim = 0; iDim < nDim; iDim++) {
        Omega += pow(Vorticity[iDim], 2);
      }
      Omega = sqrt(Omega);

      su2double Ducros_j = 0.0;

      if (config->GetKind_RoeLowDiss() == FD_DUCROS) {
        Ducros_j = -uixi / (fabs(uixi) + Omega + 1e-20);
      }
      else if (config->GetKind_RoeLowDiss() == NTS_DUCROS) {
        Ducros_j = pow(uixi,2.0) /(pow(uixi,2.0)+ pow(Omega,2.0) + 1e-20);
      }
      Ducros_i = max(Ducros_i, Ducros_j);
    }

    nodes->SetSensor(iPoint, Ducros_i);
  }
  END_SU2_OMP_FOR

  InitiateComms(geometry, config, MPI_QUANTITIES::SENSOR);
  CompleteComms(geometry, config, MPI_QUANTITIES::SENSOR);

}

void CEulerSolver::ExplicitRK_Iteration(CGeometry *geometry, CSolver **solver_container,
                                        CConfig *config, unsigned short iRKStep) {
  SU2_ZONE_SCOPED

  Explicit_Iteration<RUNGE_KUTTA_EXPLICIT>(geometry, solver_container, config, iRKStep);
}

void CEulerSolver::ClassicalRK4_Iteration(CGeometry *geometry, CSolver **solver_container,
                                        CConfig *config, unsigned short iRKStep) {
  SU2_ZONE_SCOPED

  Explicit_Iteration<CLASSICAL_RK4_EXPLICIT>(geometry, solver_container, config, iRKStep);
}

void CEulerSolver::ExplicitEuler_Iteration(CGeometry *geometry, CSolver **solver_container, CConfig *config) {
  SU2_ZONE_SCOPED

  Explicit_Iteration<EULER_EXPLICIT>(geometry, solver_container, config, 0);
}

void CEulerSolver::PrepareImplicitIteration(CGeometry *geometry, CSolver**, CConfig *config) {
  SU2_ZONE_SCOPED

  struct LowMachPrec {
    const CEulerSolver* solver;
    const bool active;
    su2activematrix matrix;

    LowMachPrec(const CEulerSolver* s, bool a, unsigned short nVar) : solver(s), active(a) {
      if (active) matrix.resize(nVar,nVar);
    }

    FORCEINLINE const su2activematrix& operator() (const CConfig* config, unsigned long iPoint, su2double delta) {
      solver->SetPreconditioner(config, iPoint, delta, matrix);
      return matrix;
    }

  } precond(this, config->Low_Mach_Preconditioning() || (config->GetKind_Upwind_Flow() == UPWIND::TURKEL), nVar);

  PrepareImplicitIteration_impl(precond, geometry, config);
}

void CEulerSolver::ComputeUnderRelaxationFactor(const CConfig* config, CSolver** solver_container) {
  SU2_ZONE_SCOPED

  /*--- Loop over the solution update given by relaxing the linear system for this
   * nonlinear iteration and impose a limit on the maximum percentage that the
   * density and static energy can change. */

  const su2double allowableRatio = config->GetMaxUpdateFractionFlow();

  SU2_OMP_FOR_STAT(omp_chunk_size)
  for (unsigned long iPoint = 0; iPoint < nPointDomain; iPoint++) {
    su2double ratio = fabs(LinSysSol(iPoint, 0)) / max(nodes->GetSolution(iPoint, 0), EPS);
    su2double e_old = nodes->GetSolution(iPoint, nVar - 1);
    su2double e_new = e_old + LinSysSol(iPoint, nVar - 1);
    /*--- rho*e = rho*E - |rho*u|^2 / (2*rho), with the old and the proposed new density. ---*/
    /*--- Materialize CoDi expressions: GetSolution returns temporary active scalars. ---*/
    const su2double rho_old = max(nodes->GetSolution(iPoint, 0), EPS);
    const su2double rho_new = max(rho_old + LinSysSol(iPoint, 0), EPS);
    for (auto jVar = 1u; jVar <= nDim; jVar++) {
      e_old -= 0.5 * pow(nodes->GetSolution(iPoint, jVar), 2) / rho_old;
      e_new -= 0.5 * pow(nodes->GetSolution(iPoint, jVar) + LinSysSol(iPoint, jVar), 2) / rho_new;
    }
    if (config->GetKind_Turb_Model() == TURB_MODEL::SST) {
      const su2double tke = solver_container[TURB_SOL]->GetNodes()->GetSolution(iPoint, 0);
      e_old -= rho_old * tke;
      e_new -= rho_new * tke;
    }
    ratio = fmax(ratio, fabs(e_new - e_old) / max(e_old, EPS));

    su2double localUnderRelaxation = fmin(allowableRatio / fmax(ratio, EPS), 1);

    /* Threshold the relaxation factor in the event that there is
     a very small value. This helps avoid catastrophic crashes due
     to non-realizable states by canceling the update. */

    if (localUnderRelaxation < 1e-10) localUnderRelaxation = 0.0;

    nodes->SetUnderRelaxation(iPoint, localUnderRelaxation);
  }
  END_SU2_OMP_FOR
}

void CEulerSolver::LimitUnderRelaxationPositivity(CSolver** solver_container, CConfig* config) {
  SU2_ZONE_SCOPED

  /*--- POSITIVE_UPDATE_FLOW (P1_POSITIVITY.md): reduce the factor of ComputeUnderRelaxationFactor to the largest step
   * with rho >= eps_r rho_old and q >= eps_e q_old, q = rhoE - |m|^2/(2 rho) - rho k. With SST the pressure of
   * CNSVariable::SetPrimVar subtracts the current k of the turbulence solver, which advances after the flow. ---*/

  const bool sst = (config->GetKind_Turb_Model() == TURB_MODEL::SST) && (solver_container != nullptr) &&
                   (solver_container[TURB_SOL] != nullptr);
  const CVariable* turbNodes = sst ? solver_container[TURB_SOL]->GetNodes() : nullptr;

  ompMasterAssignBarrier(PositiveLimited_Counter, 0, PositiveFailed_Counter, 0);

  unsigned long limitedPoints = 0, failedPoints = 0;
  const su2double zero = 0.0;

  SU2_OMP_FOR_STAT(omp_chunk_size)
  for (unsigned long iPoint = 0; iPoint < nPointDomain; iPoint++) {
    su2double U0[P1PositiveUpdate::MAXNVAR] = {0.0}, dU[P1PositiveUpdate::MAXNVAR] = {0.0};
    for (unsigned short iVar = 0; iVar < nVar; iVar++) {
      U0[iVar] = nodes->GetSolution(iPoint, iVar);
      dU[iVar] = LinSysSol(iPoint, iVar);
    }
    const su2double k = sst ? turbNodes->GetSolution(iPoint, 0) : zero;
    const su2double alpha0 = nodes->GetUnderRelaxation(iPoint);

    const su2double alpha = P1PositiveUpdate::FlowStepLimit(nDim, U0, dU, k, alpha0);
    if (alpha < alpha0) limitedPoints++;

    /*--- A nonfinite increment is cancelled: 0 * NaN would still reach the solution through AddSolution. ---*/
    bool finite = true;
    for (unsigned short iVar = 0; iVar < nVar; iVar++) {
      if (!P1PositiveUpdate::IsFinite(dU[iVar])) finite = false;
    }
    if (!finite) {
      for (unsigned short iVar = 0; iVar < nVar; iVar++) {
        LinSysSol(iPoint, iVar) = 0.0;
        dU[iVar] = 0.0;
      }
    }

    /*--- Safety net: the accepted state, 0 failures by construction from admissible starts. ---*/
    su2double U[P1PositiveUpdate::MAXNVAR] = {0.0};
    for (unsigned short iVar = 0; iVar < nVar; iVar++) U[iVar] = U0[iVar] + alpha * dU[iVar];
    if (!P1PositiveUpdate::Admissible(nDim, U, k, zero, zero)) failedPoints++;

    nodes->SetUnderRelaxation(iPoint, alpha);
  }
  END_SU2_OMP_FOR

  atomicAdd(limitedPoints, PositiveLimited_Counter);
  atomicAdd(failedPoints, PositiveFailed_Counter);

  if ((MGLevel == MESH_0) && (config->GetComm_Level() == COMM_FULL)) {
    BEGIN_SU2_OMP_SAFE_GLOBAL_ACCESS {
      unsigned long localCounts[2] = {PositiveLimited_Counter, PositiveFailed_Counter};
      unsigned long globalCounts[2] = {0, 0};
      SU2_MPI::Allreduce(localCounts, globalCounts, 2, MPI_UNSIGNED_LONG, MPI_SUM, SU2_MPI::GetComm());
      config->SetPositive_Update_Points(globalCounts[0], globalCounts[1]);
    }
    END_SU2_OMP_SAFE_GLOBAL_ACCESS
  }
}

void CEulerSolver::IdentifySolutionOutliers(const CConfig *config, unsigned long iter) {
  SU2_ZONE_SCOPED

  const unsigned long startIteration = config->GetOutlierMitigationParam()[0];
  const unsigned long updateFrequency = config->GetOutlierMitigationParam()[1];
  const unsigned long printFrequency = config->GetOutlierMitigationParam()[2];
  const int nSigma = config->GetOutlierMitigationParam()[3];

  if (iter < startIteration) return;

  auto DetermineBinAndUpdatePoint = [&](const unsigned long iPoint) {
    const su2double t = nodes->GetTemperature(iPoint);
    const auto i = nSigma + min(max(-nSigma, SU2_TYPE::Int((t - MeanTemperature) / max(StdDevTemperature, EPS))), nSigma);
    if (i == 0 || i == 2 * nSigma) {
      if (nodes->OutlierMitigation(iPoint) == 0) {
        /*--- Start mitigating with maximum strength if the point was just identified as outlier. ---*/
        nodes->OutlierMitigation(iPoint) = CEulerVariable::MAX_OUTLIER_MITIGATION;
      } else if (nodes->OutlierMitigation(iPoint) < CEulerVariable::MAX_OUTLIER_MITIGATION) {
        /*--- The point became an outlier again after reducing mitigations (below), increase them slowly. ---*/
        ++nodes->OutlierMitigation(iPoint);
      }
    } else if (nodes->OutlierMitigation(iPoint) > 0) {
      /*--- Not an outlier anymore, try to slowly reduce the mitigations. ---*/
      --nodes->OutlierMitigation(iPoint);
    }
    return i;
  };

  /*--- Recompute mean and std deviation of temperature or use the stored values. ---*/
  if (iter == startIteration || iter % updateFrequency == 0) {
    su2double localSum = 0;
    static unsigned long nPointGlobal;
    SU2_OMP_MASTER
    MeanTemperature = 0;
    END_SU2_OMP_MASTER

    SU2_OMP_FOR_STAT(omp_chunk_size)
    for (unsigned long iPoint = 0; iPoint < nPointDomain; ++iPoint) {
      localSum += nodes->GetTemperature(iPoint);
    }
    END_SU2_OMP_FOR

    atomicAdd(localSum, MeanTemperature);

    BEGIN_SU2_OMP_SAFE_GLOBAL_ACCESS {
      su2double tmp[2] = {MeanTemperature, static_cast<su2double>(nPointDomain)}, global[2];
      SU2_MPI::Allreduce(tmp, global, 2, MPI_DOUBLE, MPI_SUM, SU2_MPI::GetComm());
      nPointGlobal = static_cast<unsigned long>(SU2_TYPE::GetValue(global[1]));
      MeanTemperature = global[0] / global[1];
    }
    END_SU2_OMP_SAFE_GLOBAL_ACCESS

    localSum = 0;
    SU2_OMP_MASTER
    StdDevTemperature = 0;
    END_SU2_OMP_MASTER

    SU2_OMP_FOR_STAT(omp_chunk_size)
    for (unsigned long iPoint = 0; iPoint < nPointDomain; iPoint ++) {
      localSum += pow(MeanTemperature - nodes->GetTemperature(iPoint), 2);
    }
    END_SU2_OMP_FOR

    atomicAdd(localSum, StdDevTemperature);

    BEGIN_SU2_OMP_SAFE_GLOBAL_ACCESS {
      SU2_MPI::Allreduce(&StdDevTemperature, &localSum, 1, MPI_DOUBLE, MPI_SUM, SU2_MPI::GetComm());
      StdDevTemperature = sqrt(localSum / nPointGlobal);
    }
    END_SU2_OMP_SAFE_GLOBAL_ACCESS

    const auto nBins = 2 * nSigma + 2;
    std::vector<unsigned long> bins(nBins, 0);
    unsigned long nPointLocal = 0;
    SU2_OMP_MASTER
    nPointGlobal = 0;
    END_SU2_OMP_MASTER

    SU2_OMP_FOR_STAT(omp_chunk_size)
    for (unsigned long iPoint = 0; iPoint < nPoint; ++iPoint) {
      const auto i = DetermineBinAndUpdatePoint(iPoint);
      if (iPoint < nPointDomain) {
        nPointLocal += static_cast<unsigned long>(nodes->OutlierMitigation(iPoint) > 0);

        SU2_OMP_ATOMIC
        ++bins[i];
      }
    }
    END_SU2_OMP_FOR

    atomicAdd(nPointLocal, nPointGlobal);

    BEGIN_SU2_OMP_SAFE_GLOBAL_ACCESS
    if (iter == 0 || iter % (updateFrequency * printFrequency) == 0) {
      bins.back() = nPointGlobal;
      std::vector<unsigned long> global(nBins);
      SU2_MPI::Reduce(bins.data(), global.data(), nBins, MPI_UNSIGNED_LONG, MPI_SUM, MASTER_NODE, SU2_MPI::GetComm());
      if (rank == MASTER_NODE) {
        PrintingToolbox::CTablePrinter outlierTable(&std::cout);
        outlierTable.AddColumn("Mean T", 12);
        outlierTable.AddColumn("StdDev", 12);
        outlierTable.AddColumn("< -" + std::to_string(nSigma), 12);
        for (int i = -nSigma; i < nSigma; ++i) {
          const int jump = (i == -1);
          outlierTable.AddColumn(std::to_string(i) + " to " + std::to_string(i + 1 + jump), 12);
          i += jump;
        }
        outlierTable.AddColumn("> " + std::to_string(nSigma), 12);
        outlierTable.AddColumn("#Mitig.", 12);
        outlierTable.SetAlign(PrintingToolbox::CTablePrinter::RIGHT);
        outlierTable.PrintHeader();
        outlierTable << MeanTemperature << StdDevTemperature;
        for (const auto n : global) outlierTable << n;
        outlierTable.PrintFooter();
      }
    }
    END_SU2_OMP_SAFE_GLOBAL_ACCESS

  } else {
    SU2_OMP_FOR_STAT(omp_chunk_size)
    for (unsigned long iPoint = 0; iPoint < nPoint; ++iPoint) {
      (void)DetermineBinAndUpdatePoint(iPoint);
    }
    END_SU2_OMP_FOR
  }
}

void CEulerSolver::SetPreconditioner(const CConfig *config, unsigned long iPoint,
                                     su2double delta, su2activematrix& preconditioner) const {
  SU2_ZONE_SCOPED

  unsigned short iDim, jDim, iVar, jVar;
  su2double local_Mach, rho, enthalpy, soundspeed, sq_vel;
  su2double *U_i = nullptr;
  su2double Beta_max = config->GetmaxTurkelBeta();
  su2double Mach_infty2, Mach_lim2, aux, parameter;

  /*--- Variables to calculate the preconditioner parameter Beta ---*/
  local_Mach = sqrt(nodes->GetVelocity2(iPoint))/nodes->GetSoundSpeed(iPoint);

  /*--- Weiss and Smith Preconditioning---*/
  Mach_infty2 = pow(config->GetMach(),2.0);
  Mach_lim2 = pow(0.00001,2.0);
  aux = max(pow(local_Mach,2.0),Mach_lim2);
  parameter = min(1.0, max(aux,Beta_max*Mach_infty2));

  U_i = nodes->GetSolution(iPoint);

  rho = U_i[0];
  enthalpy = nodes->GetEnthalpy(iPoint);
  soundspeed = nodes->GetSoundSpeed(iPoint);
  sq_vel = nodes->GetVelocity2(iPoint);

  const su2double tke = config->GetKind_Turb_Model() == TURB_MODEL::SST ? nodes->GetRhoTke(iPoint) / rho : 0.0;

  /*---Calculating the inverse of the preconditioning matrix that multiplies the time derivative  */
  preconditioner[0][0] = 0.5 * sq_vel - tke;
  preconditioner[0][nVar - 1] = 1.0;
  for (iDim = 0; iDim < nDim; iDim++) preconditioner[0][1 + iDim] = -1.0 * U_i[iDim + 1] / rho;

  for (iDim = 0; iDim < nDim; iDim++) {
    preconditioner[iDim + 1][0] = (0.5 * sq_vel - tke) * U_i[iDim + 1] / rho;
    preconditioner[iDim + 1][nVar - 1] = U_i[iDim + 1] / rho;
    for (jDim = 0; jDim < nDim; jDim++) {
      preconditioner[iDim + 1][1 + jDim] = -1.0 * U_i[jDim + 1] / rho * U_i[iDim + 1] / rho;
    }
  }

  preconditioner[nVar - 1][0] = (0.5 * sq_vel - tke) * enthalpy;
  preconditioner[nVar - 1][nVar - 1] = enthalpy;
  for (iDim = 0; iDim < nDim; iDim++) preconditioner[nVar - 1][1 + iDim] = -1.0 * U_i[iDim + 1] / rho * enthalpy;

  for (iVar = 0; iVar < nVar; iVar ++ ) {
    for (jVar = 0; jVar < nVar; jVar ++ ) {
      preconditioner[iVar][jVar] = (parameter - 1.0) * ((Gamma-1.0)/(soundspeed*soundspeed))*preconditioner[iVar][jVar];
      if (iVar == jVar)
        preconditioner[iVar][iVar] += 1.0;

      preconditioner[iVar][jVar] *= delta;
    }
  }

}

void CEulerSolver::GetPower_Properties(CGeometry *geometry, CConfig *config, unsigned short iMesh, bool Output) {
  SU2_ZONE_SCOPED

  unsigned short iDim, iMarker, jMarker;
  unsigned long iVertex, iPoint;
  su2double  *V_inlet = nullptr, *V_outlet = nullptr, Pressure, Temperature, Velocity[3], Vn,
  Velocity2, Density, Area, SoundSpeed, TotalPressure, Vel_Infty2, RamDrag,
  TotalTemperature, VelocityJet,
  Vel_Infty, MaxPressure, MinPressure, MFR, InfVel2;
  su2double DeltaPressure = 0.0, DeltaThrust = 0.0, DeltaTorque = 0.0;
  unsigned short iMarker_Inlet, iMarker_Outlet, nMarker_Inlet, nMarker_Outlet;
  string Inlet_TagBound, Outlet_TagBound;
  su2double DeltaPress = 0.0, DeltaTemp = 0.0, TotalPressRatio = 0.0, TotalTempRatio = 0.0, StaticPressRatio = 0.0, StaticTempRatio = 0.0,
  NetThrust = 0.0, GrossThrust = 0.0, Power = 0.0, MassFlow = 0.0, Mach = 0.0, Force = 0.0;
  bool ReverseFlow, Engine = false, Pair = true;
  su2double Vector[MAXNDIM] = {0.0};

  su2double Gas_Constant = config->GetGas_ConstantND();
  su2double Cp = Gas_Constant*Gamma / (Gamma-1.0);
  su2double Alpha = config->GetAoA()*PI_NUMBER/180.0;
  su2double Beta = config->GetAoS()*PI_NUMBER/180.0;
  bool write_heads = ((((config->GetInnerIter() % (config->GetScreen_Wrt_Freq(2)*40)) == 0) && (config->GetInnerIter()!= 0)) || (config->GetInnerIter() == 1));
  bool Evaluate_BC = ((((config->GetInnerIter() % (config->GetBc_Eval_Freq())) == 0)) || (config->GetInnerIter() == 1) || (config->GetDiscrete_Adjoint()));

  if ((config->GetnMarker_EngineInflow() != 0) || (config->GetnMarker_EngineExhaust() != 0)) Engine = true;
  if ((config->GetnMarker_ActDiskInlet() != 0) || (config->GetnMarker_ActDiskOutlet() != 0)) Engine = false;
  if ((config->GetnMarker_EngineInflow()) != (config->GetnMarker_EngineExhaust())) Pair = false;

  if (Engine) { nMarker_Inlet  = config->GetnMarker_EngineInflow(); nMarker_Outlet = config->GetnMarker_EngineExhaust(); }
  else  { nMarker_Inlet = config->GetnMarker_ActDiskInlet(); nMarker_Outlet  = config->GetnMarker_ActDiskOutlet(); }

  /*--- Evaluate the MPI for the actuator disk IO ---*/

  if (Evaluate_BC) {

    auto *Inlet_MassFlow         = new su2double [config->GetnMarker_All()]();
    auto *Inlet_ReverseMassFlow  = new su2double [config->GetnMarker_All()]();
    auto *Inlet_Pressure         = new su2double [config->GetnMarker_All()]();
    auto *Inlet_Mach             = new su2double [config->GetnMarker_All()]();
    auto *Inlet_MaxPressure      = new su2double [config->GetnMarker_All()]();
    auto *Inlet_MinPressure      = new su2double [config->GetnMarker_All()]();
    auto *Inlet_TotalPressure    = new su2double [config->GetnMarker_All()]();
    auto *Inlet_Temperature      = new su2double [config->GetnMarker_All()]();
    auto *Inlet_TotalTemperature = new su2double [config->GetnMarker_All()]();
    auto *Inlet_Area             = new su2double [config->GetnMarker_All()]();
    auto *Inlet_RamDrag          = new su2double [config->GetnMarker_All()]();
    auto *Inlet_Force            = new su2double [config->GetnMarker_All()]();
    auto *Inlet_Power            = new su2double [config->GetnMarker_All()]();
    auto *Inlet_XCG              = new su2double [config->GetnMarker_All()]();
    auto *Inlet_YCG              = new su2double [config->GetnMarker_All()]();
    auto *Inlet_ZCG              = new su2double [config->GetnMarker_All()]();

    auto *Outlet_MassFlow         = new su2double [config->GetnMarker_All()]();
    auto *Outlet_Pressure         = new su2double [config->GetnMarker_All()]();
    auto *Outlet_TotalPressure    = new su2double [config->GetnMarker_All()]();
    auto *Outlet_Temperature      = new su2double [config->GetnMarker_All()]();
    auto *Outlet_TotalTemperature = new su2double [config->GetnMarker_All()]();
    auto *Outlet_Area             = new su2double [config->GetnMarker_All()]();
    auto *Outlet_GrossThrust      = new su2double [config->GetnMarker_All()]();
    auto *Outlet_Force            = new su2double [config->GetnMarker_All()]();
    auto *Outlet_Power            = new su2double [config->GetnMarker_All()]();

    auto *Outlet_DeltaP           = new su2double [config->GetnMarker_All()]();
    auto *Outlet_Thrust           = new su2double [config->GetnMarker_All()]();
    auto *Outlet_Torque           = new su2double [config->GetnMarker_All()]();

    /*--- Comute MassFlow, average temp, press, etc. ---*/

    for (iMarker = 0; iMarker < config->GetnMarker_All(); iMarker++) {

      MinPressure = +1E10; MaxPressure = -1E10;

      if ((config->GetMarker_All_KindBC(iMarker) == ACTDISK_INLET) ||
          (config->GetMarker_All_KindBC(iMarker) == ENGINE_INFLOW)) {

        for (iVertex = 0; iVertex < geometry->nVertex[iMarker]; iVertex++) {

          iPoint = geometry->vertex[iMarker][iVertex]->GetNode();

          if (geometry->nodes->GetDomain(iPoint)) {

            V_inlet = nodes->GetPrimitive(iPoint);

            geometry->vertex[iMarker][iVertex]->GetNormal(Vector);

            Temperature = V_inlet[0];
            Pressure = V_inlet[nDim+1];

            Density = V_inlet[nDim+2];
            SoundSpeed = sqrt(Gamma*Pressure/Density);

            Velocity2 = 0.0; MassFlow = 0.0; Vel_Infty2 =0.0;
            for (iDim = 0; iDim < nDim; iDim++) {
              Velocity[iDim] = V_inlet[iDim+1];
              Velocity2 += Velocity[iDim]*Velocity[iDim];
              Vel_Infty2 += GetVelocity_Inf(iDim)*GetVelocity_Inf(iDim);
              MassFlow -= Vector[iDim]*Velocity[iDim]*Density;
            }

            Area = GeometryToolbox::Norm(nDim, Vector);
            Vn = 0.0; ReverseFlow = false;
            for (iDim = 0; iDim < nDim; iDim++) {  Vn -= Velocity[iDim]*Vector[iDim]/Area; }
            if (Vn < 0.0) { ReverseFlow = true; }

            Vel_Infty = sqrt (Vel_Infty2);
            Mach = sqrt(Velocity2)/SoundSpeed;
            TotalPressure = Pressure * pow( 1.0 + Mach * Mach * 0.5 * (Gamma - 1.0), Gamma    / (Gamma - 1.0));
            TotalTemperature = Temperature * (1.0 + Mach * Mach * 0.5 * (Gamma - 1.0));
            MinPressure = min(MinPressure, TotalPressure);
            MaxPressure = max(MaxPressure, TotalPressure);

            RamDrag = MassFlow * Vel_Infty;

            Inlet_MassFlow[iMarker]         += MassFlow;
            Inlet_Pressure[iMarker]         += Pressure*MassFlow;
            Inlet_Mach[iMarker]             += Mach*MassFlow;
            Inlet_MinPressure[iMarker]       = min (MinPressure, Inlet_MinPressure[iMarker]);
            Inlet_MaxPressure[iMarker]       = max(MaxPressure, Inlet_MaxPressure[iMarker]);
            Inlet_TotalPressure[iMarker]    += TotalPressure*MassFlow;
            Inlet_Temperature[iMarker]      += Temperature*MassFlow;
            Inlet_TotalTemperature[iMarker] += TotalTemperature*MassFlow;
            Inlet_Area[iMarker]             += Area;
            Inlet_RamDrag[iMarker]          += RamDrag;
            Inlet_Power[iMarker] += MassFlow*Cp*TotalTemperature;
            if (ReverseFlow) Inlet_ReverseMassFlow[iMarker]  += MassFlow;

            su2double Inlet_ForceX = -(Pressure - Pressure_Inf)*Vector[0] + MassFlow*Velocity[0];
            su2double Inlet_ForceY = -(Pressure - Pressure_Inf)*Vector[1] + MassFlow*Velocity[1];
            su2double Inlet_ForceZ = 0.0;
            if (nDim == 3) Inlet_ForceZ = -(Pressure - Pressure_Inf)*Vector[2] + MassFlow*Velocity[2];
            Inlet_Force[iMarker] +=  Inlet_ForceX*cos(Alpha)*cos(Beta) + Inlet_ForceY*sin(Beta) +Inlet_ForceZ*sin(Alpha)*cos(Beta);

            Inlet_XCG[iMarker] += geometry->nodes->GetCoord(iPoint, 0)*Area;
            Inlet_YCG[iMarker] += geometry->nodes->GetCoord(iPoint, 1)*Area;
            if (nDim == 3) Inlet_ZCG[iMarker] += geometry->nodes->GetCoord(iPoint, 2)*Area;

          }
        }

      }

      if ((config->GetMarker_All_KindBC(iMarker) == ACTDISK_OUTLET) ||
          (config->GetMarker_All_KindBC(iMarker) == ENGINE_EXHAUST)) {

        for (iVertex = 0; iVertex < geometry->nVertex[iMarker]; iVertex++) {

          iPoint = geometry->vertex[iMarker][iVertex]->GetNode();

          if (geometry->nodes->GetDomain(iPoint)) {

            V_outlet = nodes->GetPrimitive(iPoint);

            geometry->vertex[iMarker][iVertex]->GetNormal(Vector);

            Temperature = V_outlet[0];
            Pressure = V_outlet[nDim+1];

            Density = V_outlet[nDim+2];
            SoundSpeed  = sqrt(Gamma*Pressure/Density);

            Velocity2 = 0.0; MassFlow = 0.0; Vel_Infty2 = 0.0;
            for (iDim = 0; iDim < nDim; iDim++) {
              Velocity[iDim] = V_outlet[iDim+1];
              Velocity2 += Velocity[iDim]*Velocity[iDim];
              Vel_Infty2 += GetVelocity_Inf(iDim)*GetVelocity_Inf(iDim);
              MassFlow += Vector[iDim]*Velocity[iDim]*Density;
            }

            Vel_Infty = sqrt (Vel_Infty2);
            Area = GeometryToolbox::Norm(nDim, Vector);
            Mach = sqrt(Velocity2)/SoundSpeed;
            TotalPressure = Pressure * pow( 1.0 + Mach * Mach * 0.5 * (Gamma - 1.0), Gamma / (Gamma - 1.0));
            TotalTemperature = Temperature * (1.0 + Mach * Mach * 0.5 * (Gamma - 1.0));
            VelocityJet = sqrt(Velocity2) ;
            DeltaPressure = ActDisk_DeltaP_r[iMarker][iVertex];
            DeltaThrust = ActDisk_Thrust_r[iMarker][iVertex];
            DeltaTorque = ActDisk_Torque_r[iMarker][iVertex];

            GrossThrust = MassFlow * VelocityJet;

            Outlet_MassFlow[iMarker] += MassFlow;
            Outlet_Pressure[iMarker] += Pressure*MassFlow;
            Outlet_TotalPressure[iMarker] += TotalPressure*MassFlow;
            Outlet_Temperature[iMarker] += Temperature*MassFlow;
            Outlet_TotalTemperature[iMarker] += TotalTemperature*MassFlow;
            Outlet_Area[iMarker] += Area;
            Outlet_GrossThrust[iMarker] += GrossThrust;
            Outlet_Power[iMarker] += MassFlow*Cp*TotalTemperature;
            Outlet_DeltaP[iMarker] += DeltaPressure * Area;
            Outlet_Thrust[iMarker] += DeltaThrust * Area;
            Outlet_Torque[iMarker] += DeltaTorque * Area;

            su2double Outlet_ForceX = -(Pressure - Pressure_Inf)*Vector[0] -MassFlow*Velocity[0];
            su2double Outlet_ForceY = -(Pressure - Pressure_Inf)*Vector[1] -MassFlow*Velocity[1];
            su2double Outlet_ForceZ = 0.0;
            if (nDim == 3) Outlet_ForceZ = -(Pressure - Pressure_Inf)*Vector[2] -MassFlow*Velocity[2];

            if (nDim == 2) Outlet_Force[iMarker] +=  Outlet_ForceX*cos(Alpha) + Outlet_ForceY*sin(Alpha);
            if (nDim == 3) Outlet_Force[iMarker] +=  Outlet_ForceX*cos(Alpha)*cos(Beta) + Outlet_ForceY*sin(Beta) + Outlet_ForceZ*sin(Alpha)*cos(Beta);

          }
        }

      }

    }

    /*--- Copy to the appropriate structure ---*/

    auto *Inlet_MassFlow_Local             = new su2double [nMarker_Inlet]();
    auto *Inlet_ReverseMassFlow_Local      = new su2double [nMarker_Inlet]();
    auto *Inlet_Temperature_Local          = new su2double [nMarker_Inlet]();
    auto *Inlet_TotalTemperature_Local     = new su2double [nMarker_Inlet]();
    auto *Inlet_Pressure_Local             = new su2double [nMarker_Inlet]();
    auto *Inlet_Mach_Local                 = new su2double [nMarker_Inlet]();
    auto *Inlet_MinPressure_Local          = new su2double [nMarker_Inlet]();
    auto *Inlet_MaxPressure_Local          = new su2double [nMarker_Inlet]();
    auto *Inlet_Power_Local                = new su2double [nMarker_Inlet]();
    auto *Inlet_TotalPressure_Local        = new su2double [nMarker_Inlet]();
    auto *Inlet_RamDrag_Local              = new su2double [nMarker_Inlet]();
    auto *Inlet_Force_Local                = new su2double [nMarker_Inlet]();
    auto *Inlet_Area_Local                 = new su2double [nMarker_Inlet]();
    auto *Inlet_XCG_Local                  = new su2double [nMarker_Inlet]();
    auto *Inlet_YCG_Local                  = new su2double [nMarker_Inlet]();
    auto *Inlet_ZCG_Local                  = new su2double [nMarker_Inlet]();

    auto *Inlet_MassFlow_Total             = new su2double [nMarker_Inlet]();
    auto *Inlet_ReverseMassFlow_Total      = new su2double [nMarker_Inlet]();
    auto *Inlet_Pressure_Total             = new su2double [nMarker_Inlet]();
    auto *Inlet_Mach_Total                 = new su2double [nMarker_Inlet]();
    auto *Inlet_MinPressure_Total          = new su2double [nMarker_Inlet]();
    auto *Inlet_MaxPressure_Total          = new su2double [nMarker_Inlet]();
    auto *Inlet_Power_Total                = new su2double [nMarker_Inlet]();
    auto *Inlet_TotalPressure_Total        = new su2double [nMarker_Inlet]();
    auto *Inlet_Temperature_Total          = new su2double [nMarker_Inlet]();
    auto *Inlet_TotalTemperature_Total     = new su2double [nMarker_Inlet]();
    auto *Inlet_RamDrag_Total              = new su2double [nMarker_Inlet]();
    auto *Inlet_Force_Total                = new su2double [nMarker_Inlet]();
    auto *Inlet_Area_Total                 = new su2double [nMarker_Inlet]();
    auto *Inlet_XCG_Total                  = new su2double [nMarker_Inlet]();
    auto *Inlet_YCG_Total                  = new su2double [nMarker_Inlet]();
    auto *Inlet_ZCG_Total                  = new su2double [nMarker_Inlet]();

    auto *Outlet_MassFlow_Local            = new su2double [nMarker_Outlet]();
    auto *Outlet_Pressure_Local            = new su2double [nMarker_Outlet]();
    auto *Outlet_TotalPressure_Local       = new su2double [nMarker_Outlet]();
    auto *Outlet_Temperature_Local         = new su2double [nMarker_Outlet]();
    auto *Outlet_TotalTemperature_Local    = new su2double [nMarker_Outlet]();
    auto *Outlet_GrossThrust_Local         = new su2double [nMarker_Outlet]();
    auto *Outlet_Force_Local               = new su2double [nMarker_Outlet]();
    auto *Outlet_Power_Local               = new su2double [nMarker_Outlet]();
    auto *Outlet_Area_Local                = new su2double [nMarker_Outlet]();
    auto *Outlet_DeltaP_Local              = new su2double [nMarker_Outlet]();
    auto *Outlet_Thrust_Local              = new su2double [nMarker_Outlet]();
    auto *Outlet_Torque_Local              = new su2double [nMarker_Outlet]();

    auto *Outlet_MassFlow_Total            = new su2double [nMarker_Outlet]();
    auto *Outlet_Pressure_Total            = new su2double [nMarker_Outlet]();
    auto *Outlet_TotalPressure_Total       = new su2double [nMarker_Outlet]();
    auto *Outlet_Temperature_Total         = new su2double [nMarker_Outlet]();
    auto *Outlet_TotalTemperature_Total    = new su2double [nMarker_Outlet]();
    auto *Outlet_GrossThrust_Total         = new su2double [nMarker_Outlet]();
    auto *Outlet_Force_Total               = new su2double [nMarker_Outlet]();
    auto *Outlet_Power_Total               = new su2double [nMarker_Outlet]();
    auto *Outlet_Area_Total                = new su2double [nMarker_Outlet]();
    auto *Outlet_DeltaP_Total              = new su2double [nMarker_Outlet]();
    auto *Outlet_Thrust_Total              = new su2double [nMarker_Outlet]();
    auto *Outlet_Torque_Total              = new su2double [nMarker_Outlet]();

    /*--- Copy the values to the local array for MPI ---*/

    for (iMarker = 0; iMarker < config->GetnMarker_All(); iMarker++) {

      if ((config->GetMarker_All_KindBC(iMarker) == ACTDISK_INLET) ||  (config->GetMarker_All_KindBC(iMarker) == ENGINE_INFLOW)) {
        for (iMarker_Inlet = 0; iMarker_Inlet < nMarker_Inlet; iMarker_Inlet++) {

          if (config->GetMarker_All_KindBC(iMarker) == ACTDISK_INLET) Inlet_TagBound = config->GetMarker_ActDiskInlet_TagBound(iMarker_Inlet);
          if (config->GetMarker_All_KindBC(iMarker) == ENGINE_INFLOW) Inlet_TagBound = config->GetMarker_EngineInflow_TagBound(iMarker_Inlet);

          if (config->GetMarker_All_TagBound(iMarker) == Inlet_TagBound) {
            Inlet_MassFlow_Local[iMarker_Inlet]             += Inlet_MassFlow[iMarker];
            Inlet_ReverseMassFlow_Local[iMarker_Inlet]      += Inlet_ReverseMassFlow[iMarker];
            Inlet_Pressure_Local[iMarker_Inlet]             += Inlet_Pressure[iMarker];
            Inlet_Mach_Local[iMarker_Inlet]                 += Inlet_Mach[iMarker];
            Inlet_MinPressure_Local[iMarker_Inlet]          += Inlet_MinPressure[iMarker];
            Inlet_MaxPressure_Local[iMarker_Inlet]          += Inlet_MaxPressure[iMarker];
            Inlet_TotalPressure_Local[iMarker_Inlet]        += Inlet_TotalPressure[iMarker];
            Inlet_Temperature_Local[iMarker_Inlet]          += Inlet_Temperature[iMarker];
            Inlet_TotalTemperature_Local[iMarker_Inlet]     += Inlet_TotalTemperature[iMarker];
            Inlet_RamDrag_Local[iMarker_Inlet]              += Inlet_RamDrag[iMarker];
            Inlet_Force_Local[iMarker_Inlet]                += Inlet_Force[iMarker];
            Inlet_Power_Local[iMarker_Inlet]                += Inlet_Power[iMarker];
            Inlet_Area_Local[iMarker_Inlet]                 += Inlet_Area[iMarker];
            Inlet_XCG_Local[iMarker_Inlet]                  += Inlet_XCG[iMarker];
            Inlet_YCG_Local[iMarker_Inlet]                  += Inlet_YCG[iMarker];
            if (nDim == 3) Inlet_ZCG_Local[iMarker_Inlet]   += Inlet_ZCG[iMarker];
          }

        }
      }

      if ((config->GetMarker_All_KindBC(iMarker) == ACTDISK_OUTLET) || (config->GetMarker_All_KindBC(iMarker) == ENGINE_EXHAUST)) {
        for (iMarker_Outlet= 0; iMarker_Outlet < nMarker_Outlet; iMarker_Outlet++) {

          if (config->GetMarker_All_KindBC(iMarker) == ACTDISK_OUTLET) Outlet_TagBound = config->GetMarker_ActDiskOutlet_TagBound(iMarker_Outlet);
          if (config->GetMarker_All_KindBC(iMarker) == ENGINE_EXHAUST) Outlet_TagBound = config->GetMarker_EngineExhaust_TagBound(iMarker_Outlet);

          if (config->GetMarker_All_TagBound(iMarker) == Outlet_TagBound) {
            Outlet_MassFlow_Local[iMarker_Outlet]               += Outlet_MassFlow[iMarker];
            Outlet_Pressure_Local[iMarker_Outlet]               += Outlet_Pressure[iMarker];
            Outlet_TotalPressure_Local[iMarker_Outlet]          += Outlet_TotalPressure[iMarker];
            Outlet_Temperature_Local[iMarker_Outlet]            += Outlet_Temperature[iMarker];
            Outlet_TotalTemperature_Local[iMarker_Outlet]       += Outlet_TotalTemperature[iMarker];
            Outlet_GrossThrust_Local[iMarker_Outlet]            += Outlet_GrossThrust[iMarker];
            Outlet_Force_Local[iMarker_Outlet]                  += Outlet_Force[iMarker];
            Outlet_Power_Local[iMarker_Outlet]                  += Outlet_Power[iMarker];
            Outlet_Area_Local[iMarker_Outlet]                   += Outlet_Area[iMarker];
            Outlet_DeltaP_Local[iMarker_Outlet]                 += Outlet_DeltaP[iMarker];
            Outlet_Thrust_Local[iMarker_Outlet]                 += Outlet_Thrust[iMarker];
            Outlet_Torque_Local[iMarker_Outlet]                 += Outlet_Torque[iMarker];
          }

        }
      }

    }

    /*--- Correct the min max values for the MPI ---*/

    bool ActDisk  = false;
    for (iMarker = 0; iMarker < config->GetnMarker_All(); iMarker++) {
      if ((config->GetMarker_All_KindBC(iMarker) == ACTDISK_INLET) ||
          (config->GetMarker_All_KindBC(iMarker) == ENGINE_INFLOW)) { ActDisk  = true; break; }
    }

    if (!ActDisk) {
      for (iMarker_Inlet = 0; iMarker_Inlet < nMarker_Inlet; iMarker_Inlet++) {
        Inlet_MinPressure_Local[iMarker_Inlet]      =  1E10;
        Inlet_MaxPressure_Local[iMarker_Inlet]      = -1E10;
      }
    }

    /*--- All the ranks to compute the total value ---*/

    SU2_MPI::Allreduce(Inlet_MassFlow_Local, Inlet_MassFlow_Total, nMarker_Inlet, MPI_DOUBLE, MPI_SUM, SU2_MPI::GetComm());
    SU2_MPI::Allreduce(Inlet_ReverseMassFlow_Local, Inlet_ReverseMassFlow_Total, nMarker_Inlet, MPI_DOUBLE, MPI_SUM, SU2_MPI::GetComm());
    SU2_MPI::Allreduce(Inlet_Pressure_Local, Inlet_Pressure_Total, nMarker_Inlet, MPI_DOUBLE, MPI_SUM, SU2_MPI::GetComm());
    SU2_MPI::Allreduce(Inlet_Mach_Local, Inlet_Mach_Total, nMarker_Inlet, MPI_DOUBLE, MPI_SUM, SU2_MPI::GetComm());
    SU2_MPI::Allreduce(Inlet_MinPressure_Local, Inlet_MinPressure_Total, nMarker_Inlet, MPI_DOUBLE, MPI_MIN, SU2_MPI::GetComm());
    SU2_MPI::Allreduce(Inlet_MaxPressure_Local, Inlet_MaxPressure_Total, nMarker_Inlet, MPI_DOUBLE, MPI_MAX, SU2_MPI::GetComm());
    SU2_MPI::Allreduce(Inlet_TotalPressure_Local, Inlet_TotalPressure_Total, nMarker_Inlet, MPI_DOUBLE, MPI_SUM, SU2_MPI::GetComm());
    SU2_MPI::Allreduce(Inlet_Temperature_Local, Inlet_Temperature_Total, nMarker_Inlet, MPI_DOUBLE, MPI_SUM, SU2_MPI::GetComm());
    SU2_MPI::Allreduce(Inlet_TotalTemperature_Local, Inlet_TotalTemperature_Total, nMarker_Inlet, MPI_DOUBLE, MPI_SUM, SU2_MPI::GetComm());
    SU2_MPI::Allreduce(Inlet_RamDrag_Local, Inlet_RamDrag_Total, nMarker_Inlet, MPI_DOUBLE, MPI_SUM, SU2_MPI::GetComm());
    SU2_MPI::Allreduce(Inlet_Force_Local, Inlet_Force_Total, nMarker_Inlet, MPI_DOUBLE, MPI_SUM, SU2_MPI::GetComm());
    SU2_MPI::Allreduce(Inlet_Power_Local, Inlet_Power_Total, nMarker_Inlet, MPI_DOUBLE, MPI_SUM, SU2_MPI::GetComm());
    SU2_MPI::Allreduce(Inlet_Area_Local, Inlet_Area_Total, nMarker_Inlet, MPI_DOUBLE, MPI_SUM, SU2_MPI::GetComm());
    SU2_MPI::Allreduce(Inlet_XCG_Local, Inlet_XCG_Total, nMarker_Inlet, MPI_DOUBLE, MPI_SUM, SU2_MPI::GetComm());
    SU2_MPI::Allreduce(Inlet_YCG_Local, Inlet_YCG_Total, nMarker_Inlet, MPI_DOUBLE, MPI_SUM, SU2_MPI::GetComm());
    if (nDim == 3) SU2_MPI::Allreduce(Inlet_ZCG_Local, Inlet_ZCG_Total, nMarker_Inlet, MPI_DOUBLE, MPI_SUM, SU2_MPI::GetComm());

    SU2_MPI::Allreduce(Outlet_MassFlow_Local, Outlet_MassFlow_Total, nMarker_Outlet, MPI_DOUBLE, MPI_SUM, SU2_MPI::GetComm());
    SU2_MPI::Allreduce(Outlet_Pressure_Local, Outlet_Pressure_Total, nMarker_Outlet, MPI_DOUBLE, MPI_SUM, SU2_MPI::GetComm());
    SU2_MPI::Allreduce(Outlet_TotalPressure_Local, Outlet_TotalPressure_Total, nMarker_Outlet, MPI_DOUBLE, MPI_SUM, SU2_MPI::GetComm());
    SU2_MPI::Allreduce(Outlet_Temperature_Local, Outlet_Temperature_Total, nMarker_Outlet, MPI_DOUBLE, MPI_SUM, SU2_MPI::GetComm());
    SU2_MPI::Allreduce(Outlet_TotalTemperature_Local, Outlet_TotalTemperature_Total, nMarker_Outlet, MPI_DOUBLE, MPI_SUM, SU2_MPI::GetComm());
    SU2_MPI::Allreduce(Outlet_GrossThrust_Local, Outlet_GrossThrust_Total, nMarker_Outlet, MPI_DOUBLE, MPI_SUM, SU2_MPI::GetComm());
    SU2_MPI::Allreduce(Outlet_Force_Local, Outlet_Force_Total, nMarker_Outlet, MPI_DOUBLE, MPI_SUM, SU2_MPI::GetComm());
    SU2_MPI::Allreduce(Outlet_Power_Local, Outlet_Power_Total, nMarker_Outlet, MPI_DOUBLE, MPI_SUM, SU2_MPI::GetComm());
    SU2_MPI::Allreduce(Outlet_Area_Local, Outlet_Area_Total, nMarker_Outlet, MPI_DOUBLE, MPI_SUM, SU2_MPI::GetComm());
    SU2_MPI::Allreduce(Outlet_DeltaP_Local, Outlet_DeltaP_Total, nMarker_Outlet, MPI_DOUBLE, MPI_SUM, SU2_MPI::GetComm());
    SU2_MPI::Allreduce(Outlet_Thrust_Local, Outlet_Thrust_Total, nMarker_Outlet, MPI_DOUBLE, MPI_SUM, SU2_MPI::GetComm());
    SU2_MPI::Allreduce(Outlet_Torque_Local, Outlet_Torque_Total, nMarker_Outlet, MPI_DOUBLE, MPI_SUM, SU2_MPI::GetComm());

    /*--- Compute the value of the average surface temperature and pressure and
     set the value in the config structure for future use ---*/

    for (iMarker_Inlet = 0; iMarker_Inlet < nMarker_Inlet; iMarker_Inlet++) {
      if (Inlet_Area_Total[iMarker_Inlet] != 0.0) {
        Inlet_Pressure_Total[iMarker_Inlet] /= Inlet_MassFlow_Total[iMarker_Inlet];
        Inlet_Mach_Total[iMarker_Inlet] /= Inlet_MassFlow_Total[iMarker_Inlet];
        Inlet_TotalPressure_Total[iMarker_Inlet] /= Inlet_MassFlow_Total[iMarker_Inlet];
        Inlet_Temperature_Total[iMarker_Inlet] /= Inlet_MassFlow_Total[iMarker_Inlet];
        Inlet_TotalTemperature_Total[iMarker_Inlet] /= Inlet_MassFlow_Total[iMarker_Inlet];
        Inlet_XCG_Total[iMarker_Inlet] /= Inlet_Area_Total[iMarker_Inlet];
        Inlet_YCG_Total[iMarker_Inlet] /= Inlet_Area_Total[iMarker_Inlet];
        if (nDim == 3) Inlet_ZCG_Total[iMarker_Inlet] /= Inlet_Area_Total[iMarker_Inlet];
      }
      else {
        Inlet_Pressure_Total[iMarker_Inlet] = 0.0;
        Inlet_Mach_Total[iMarker_Inlet] = 0.0;
        Inlet_TotalPressure_Total[iMarker_Inlet] = 0.0;
        Inlet_Temperature_Total[iMarker_Inlet] = 0.0;
        Inlet_TotalTemperature_Total[iMarker_Inlet] = 0.0;
        Inlet_XCG_Total[iMarker_Inlet] = 0.0;
        Inlet_YCG_Total[iMarker_Inlet] = 0.0;
        if (nDim == 3) Inlet_ZCG_Total[iMarker_Inlet] = 0.0;
      }

      if (iMesh == MESH_0) {

        if (Engine) {
          config->SetInflow_MassFlow(iMarker_Inlet, Inlet_MassFlow_Total[iMarker_Inlet]);
          config->SetInflow_ReverseMassFlow(iMarker_Inlet, Inlet_ReverseMassFlow_Total[iMarker_Inlet]);
          config->SetInflow_Pressure(iMarker_Inlet, Inlet_Pressure_Total[iMarker_Inlet]);
          config->SetInflow_TotalPressure(iMarker_Inlet, Inlet_TotalPressure_Total[iMarker_Inlet]);
          config->SetInflow_Temperature(iMarker_Inlet, Inlet_Temperature_Total[iMarker_Inlet]);
          config->SetInflow_TotalTemperature(iMarker_Inlet, Inlet_TotalTemperature_Total[iMarker_Inlet]);
          config->SetInflow_RamDrag(iMarker_Inlet, Inlet_RamDrag_Total[iMarker_Inlet]);
          config->SetInflow_Force(iMarker_Inlet, Inlet_Force_Total[iMarker_Inlet]);
          config->SetInflow_Power(iMarker_Inlet, Inlet_Power_Total[iMarker_Inlet]);
          config->SetInflow_Mach(iMarker_Inlet, Inlet_Mach_Total[iMarker_Inlet]);
        }
        else {
          config->SetActDiskInlet_MassFlow(iMarker_Inlet, Inlet_MassFlow_Total[iMarker_Inlet]);
          config->SetActDiskInlet_ReverseMassFlow(iMarker_Inlet, Inlet_ReverseMassFlow_Total[iMarker_Inlet]);
          config->SetActDiskInlet_Pressure(iMarker_Inlet, Inlet_Pressure_Total[iMarker_Inlet]);
          config->SetActDiskInlet_TotalPressure(iMarker_Inlet, Inlet_TotalPressure_Total[iMarker_Inlet]);
          config->SetActDiskInlet_Temperature(iMarker_Inlet, Inlet_Temperature_Total[iMarker_Inlet]);
          config->SetActDiskInlet_TotalTemperature(iMarker_Inlet, Inlet_TotalTemperature_Total[iMarker_Inlet]);
          config->SetActDiskInlet_RamDrag(iMarker_Inlet, Inlet_RamDrag_Total[iMarker_Inlet]);
          config->SetActDiskInlet_Force(iMarker_Inlet, Inlet_Force_Total[iMarker_Inlet]);
          config->SetActDiskInlet_Power(iMarker_Inlet, Inlet_Power_Total[iMarker_Inlet]);
        }

      }

    }

    for (iMarker_Outlet = 0; iMarker_Outlet < nMarker_Outlet; iMarker_Outlet++) {
      if (Outlet_Area_Total[iMarker_Outlet] != 0.0) {
        Outlet_Pressure_Total[iMarker_Outlet] /= Outlet_MassFlow_Total[iMarker_Outlet];
        Outlet_TotalPressure_Total[iMarker_Outlet] /= Outlet_MassFlow_Total[iMarker_Outlet];
        Outlet_Temperature_Total[iMarker_Outlet] /= Outlet_MassFlow_Total[iMarker_Outlet];
        Outlet_TotalTemperature_Total[iMarker_Outlet] /= Outlet_MassFlow_Total[iMarker_Outlet];
      }
      else {
        Outlet_Pressure_Total[iMarker_Outlet] = 0.0;
        Outlet_TotalPressure_Total[iMarker_Outlet] = 0.0;
        Outlet_Temperature_Total[iMarker_Outlet] = 0.0;
        Outlet_TotalTemperature_Total[iMarker_Outlet] = 0.0;
      }

      if (iMesh == MESH_0) {

        if (Engine) {
          config->SetExhaust_MassFlow(iMarker_Outlet, Outlet_MassFlow_Total[iMarker_Outlet]);
          config->SetExhaust_Pressure(iMarker_Outlet, Outlet_Pressure_Total[iMarker_Outlet]);
          config->SetExhaust_TotalPressure(iMarker_Outlet, Outlet_TotalPressure_Total[iMarker_Outlet]);
          config->SetExhaust_Temperature(iMarker_Outlet, Outlet_Temperature_Total[iMarker_Outlet]);
          config->SetExhaust_TotalTemperature(iMarker_Outlet, Outlet_TotalTemperature_Total[iMarker_Outlet]);
          config->SetExhaust_GrossThrust(iMarker_Outlet, Outlet_GrossThrust_Total[iMarker_Outlet]);
          config->SetExhaust_Force(iMarker_Outlet, Outlet_Force_Total[iMarker_Outlet]);
          config->SetExhaust_Power(iMarker_Outlet, Outlet_Power_Total[iMarker_Outlet]);
        }
        else {
          config->SetActDiskOutlet_MassFlow(iMarker_Outlet, Outlet_MassFlow_Total[iMarker_Outlet]);
          config->SetActDiskOutlet_Pressure(iMarker_Outlet, Outlet_Pressure_Total[iMarker_Outlet]);
          config->SetActDiskOutlet_TotalPressure(iMarker_Outlet, Outlet_TotalPressure_Total[iMarker_Outlet]);
          config->SetActDiskOutlet_Temperature(iMarker_Outlet, Outlet_Temperature_Total[iMarker_Outlet]);
          config->SetActDiskOutlet_TotalTemperature(iMarker_Outlet, Outlet_TotalTemperature_Total[iMarker_Outlet]);
          config->SetActDiskOutlet_GrossThrust(iMarker_Outlet, Outlet_GrossThrust_Total[iMarker_Outlet]);
          config->SetActDiskOutlet_Force(iMarker_Outlet, Outlet_Force_Total[iMarker_Outlet]);
          config->SetActDiskOutlet_Power(iMarker_Outlet, Outlet_Power_Total[iMarker_Outlet]);
          config->SetActDiskOutlet_Thrust_BEM(iMarker_Outlet, Outlet_Thrust_Total[iMarker_Outlet]);
          config->SetActDiskOutlet_Torque_BEM(iMarker_Outlet, Outlet_Torque_Total[iMarker_Outlet]);
        }

      }

    }


    if (Pair) {

      /*--- Store delta pressure, temperature, thrust, and area ---*/

      for (iMarker_Inlet = 0; iMarker_Inlet < nMarker_Inlet; iMarker_Inlet++) {

        if (Engine) {
          Inlet_TagBound = config->GetMarker_EngineInflow_TagBound(iMarker_Inlet);
          jMarker = config->GetMarker_CfgFile_EngineExhaust(Inlet_TagBound);
          Outlet_TagBound = config->GetMarker_CfgFile_TagBound(jMarker);
        }
        else {
          Inlet_TagBound = config->GetMarker_ActDiskInlet_TagBound(iMarker_Inlet);
          jMarker = config->GetMarker_CfgFile_ActDiskOutlet(Inlet_TagBound);
          Outlet_TagBound = config->GetMarker_CfgFile_TagBound(jMarker);
        }


        su2double DeltaPress = 0.0, DeltaTemp = 0.0, NetThrust = 0.0, GrossThrust = 0.0, TotalPressRatio = 0.0, TotalTempRatio = 0.0, StaticPressRatio = 0.0, StaticTempRatio = 0.0;

        if (Engine) {
          DeltaPress   = config->GetExhaust_Pressure(Outlet_TagBound) - config->GetInflow_Pressure(Inlet_TagBound);
          DeltaTemp         = config->GetExhaust_Temperature(Outlet_TagBound) - config->GetInflow_Temperature(Inlet_TagBound);
          NetThrust    = config->GetExhaust_GrossThrust(Outlet_TagBound) - config->GetInflow_RamDrag(Inlet_TagBound);
          GrossThrust   = config->GetExhaust_GrossThrust(Outlet_TagBound);
          TotalPressRatio   = config->GetExhaust_TotalPressure(Outlet_TagBound)/config->GetInflow_TotalPressure(Inlet_TagBound);
          TotalTempRatio    = config->GetExhaust_TotalTemperature(Outlet_TagBound)/config->GetInflow_TotalTemperature(Inlet_TagBound);
          StaticPressRatio   = config->GetExhaust_Pressure(Outlet_TagBound)/config->GetInflow_Pressure(Inlet_TagBound);
          StaticTempRatio    = config->GetExhaust_Temperature(Outlet_TagBound)/config->GetInflow_Temperature(Inlet_TagBound);
          Force = config->GetInflow_Force(Inlet_TagBound) + config->GetExhaust_Force(Outlet_TagBound);
          Power = config->GetExhaust_Power(Outlet_TagBound) - config->GetInflow_Power(Inlet_TagBound);
        }
        else {
          DeltaPress   = config->GetActDiskOutlet_Pressure(Outlet_TagBound) - config->GetActDiskInlet_Pressure(Inlet_TagBound);
          DeltaTemp         = config->GetActDiskOutlet_Temperature(Outlet_TagBound) - config->GetActDiskInlet_Temperature(Inlet_TagBound);
          NetThrust    = config->GetActDiskOutlet_GrossThrust(Outlet_TagBound) - config->GetActDiskInlet_RamDrag(Inlet_TagBound);
          GrossThrust   = config->GetActDiskOutlet_GrossThrust(Outlet_TagBound);
          TotalPressRatio   = config->GetActDiskOutlet_TotalPressure(Outlet_TagBound)/config->GetActDiskInlet_TotalPressure(Inlet_TagBound);
          TotalTempRatio    = config->GetActDiskOutlet_TotalTemperature(Outlet_TagBound)/config->GetActDiskInlet_TotalTemperature(Inlet_TagBound);
          StaticPressRatio   = config->GetActDiskOutlet_Pressure(Outlet_TagBound)/config->GetActDiskInlet_Pressure(Inlet_TagBound);
          StaticTempRatio    = config->GetActDiskOutlet_Temperature(Outlet_TagBound)/config->GetActDiskInlet_Temperature(Inlet_TagBound);
          Force = config->GetActDiskInlet_Force(Inlet_TagBound) + config->GetActDiskOutlet_Force(Outlet_TagBound);
          Power =  config->GetActDiskOutlet_Power(Outlet_TagBound) - config->GetActDiskInlet_Power(Inlet_TagBound);
          MassFlow =  config->GetActDiskInlet_MassFlow(Inlet_TagBound);
        }

        Mach        = Inlet_Mach_Total[iMarker_Inlet];
        Area              = Inlet_Area_Total[iMarker_Inlet];

        if (Engine) {
          config->SetEngine_Mach(iMarker_Inlet, Mach);
          config->SetEngine_Force(iMarker_Inlet, Force);
          config->SetEngine_Power(iMarker_Inlet, Power);
          config->SetEngine_NetThrust(iMarker_Inlet, NetThrust);
          config->SetEngine_GrossThrust(iMarker_Inlet, GrossThrust);
          config->SetEngine_Area(iMarker_Inlet, Area);
        }
        else {
          config->SetActDisk_DeltaPress(iMarker_Inlet, DeltaPress);
          config->SetActDisk_DeltaTemp(iMarker_Inlet, DeltaTemp);
          config->SetActDisk_Mach(iMarker_Inlet, Mach);
          config->SetActDisk_Force(iMarker_Inlet, Force);
          config->SetActDisk_Power(iMarker_Inlet, Power);
          config->SetActDisk_MassFlow(iMarker_Inlet, MassFlow);
          config->SetActDisk_TotalPressRatio(iMarker_Inlet, TotalPressRatio);
          config->SetActDisk_TotalTempRatio(iMarker_Inlet, TotalTempRatio);
          config->SetActDisk_StaticPressRatio(iMarker_Inlet, StaticPressRatio);
          config->SetActDisk_StaticTempRatio(iMarker_Inlet, StaticTempRatio);
          config->SetActDisk_NetThrust(iMarker_Inlet, NetThrust);
          config->SetActDisk_GrossThrust(iMarker_Inlet, GrossThrust);
          config->SetActDisk_Area(iMarker_Inlet, Area);
        }

      }

      /*--- Screen output using the values already stored in the config container ---*/

      if ((rank == MASTER_NODE) && (iMesh == MESH_0) ) {

        cout.precision(5);
        cout.setf(ios::fixed, ios::floatfield);

        if (write_heads && Output && !config->GetDiscrete_Adjoint()) {
          if (Engine) cout << endl   << "---------------------------- Engine properties --------------------------" << endl;
          else cout << endl   << "------------------------ Actuator Disk properties -----------------------" << endl;
        }

        for (iMarker_Inlet = 0; iMarker_Inlet < nMarker_Inlet; iMarker_Inlet++) {

          if (Engine) {
            Inlet_TagBound = config->GetMarker_EngineInflow_TagBound(iMarker_Inlet);
            jMarker = config->GetMarker_CfgFile_EngineExhaust(Inlet_TagBound);
            Outlet_TagBound = config->GetMarker_CfgFile_TagBound(jMarker);
          }
          else {
            Inlet_TagBound = config->GetMarker_ActDiskInlet_TagBound(iMarker_Inlet);
            jMarker = config->GetMarker_CfgFile_ActDiskOutlet(Inlet_TagBound);
            Outlet_TagBound = config->GetMarker_CfgFile_TagBound(jMarker);
          }


          if (Engine) {
            NetThrust             =  config->GetEngine_NetThrust(iMarker_Inlet);
            GrossThrust   = config->GetEngine_GrossThrust(iMarker_Inlet);
            Power               = config->GetEngine_Power(iMarker_Inlet);
            Mach                  = config->GetEngine_Mach(iMarker_Inlet);
            Force               = config->GetEngine_Force(iMarker_Inlet);
          }
          else {
            DeltaPress      = config->GetActDisk_DeltaPress(iMarker_Inlet);
            DeltaTemp         = config->GetActDisk_DeltaTemp(iMarker_Inlet);
            TotalPressRatio       = config->GetActDisk_TotalPressRatio(iMarker_Inlet);
            TotalTempRatio        = config->GetActDisk_TotalTempRatio(iMarker_Inlet);
            StaticPressRatio      = config->GetActDisk_StaticPressRatio(iMarker_Inlet);
            StaticTempRatio           = config->GetActDisk_StaticTempRatio(iMarker_Inlet);
            NetThrust             =  config->GetActDisk_NetThrust(iMarker_Inlet);
            GrossThrust   = config->GetActDisk_GrossThrust(iMarker_Inlet);
            Power               = config->GetActDisk_Power(iMarker_Inlet);
            Mach                  = config->GetActDisk_Mach(iMarker_Inlet);
            Force               = config->GetActDisk_Force(iMarker_Inlet);
          }

          su2double Mach_Inf                  = config->GetMach();
          su2double Pressure_Inf  = config->GetPressure_FreeStreamND();

          su2double TotalPressure_Inf  = Pressure_Inf * pow( 1.0 + Mach_Inf * Mach_Inf * 0.5 * (Gamma - 1.0), Gamma     / (Gamma - 1.0));

          su2double MinPressure = Inlet_MinPressure_Total[iMarker_Inlet]/TotalPressure_Inf;
          su2double MaxPressure = Inlet_MaxPressure_Total[iMarker_Inlet]/TotalPressure_Inf;
          su2double AvePressure = Inlet_TotalPressure_Total[iMarker_Inlet]/TotalPressure_Inf;

          su2double RefDensity  = Density_Inf;
          su2double RefArea     = config->GetRefArea();
          su2double RefVel2 = 0.0;  for (iDim = 0; iDim < nDim; iDim++) RefVel2  += Velocity_Inf[iDim]*Velocity_Inf[iDim];

          su2double Factor = (0.5*RefDensity*RefArea*RefVel2);
          su2double Ref = config->GetDensity_Ref() * config->GetVelocity_Ref() * config->GetVelocity_Ref() * 1.0 * 1.0;
          su2double DmT = GetTotal_CD() * Factor;

//          su2double ModDmT = 0.0;
//          if (nDim == 2) ModDmT = sqrt(GetTotal_CFx()*GetTotal_CFx() +
//                                       GetTotal_CFy()*GetTotal_CFy());
//
//          if (nDim == 3) ModDmT = sqrt(GetTotal_CFx()*GetTotal_CFx() +
//                                       GetTotal_CFy()*GetTotal_CFy() +
//                                       GetTotal_CFz()*GetTotal_CFz());
//
//          DmTVector[0] = GetTotal_CFx()/ModDmT;
//          DmTVector[1] = GetTotal_CFy()/ModDmT;
//          if (nDim == 3)  DmTVector[2] = GetTotal_CFz()/ModDmT;

          /*--- Set the aero drag ---*/

          su2double Aero_Drag = DmT - Force;
          su2double Aero_CD = Aero_Drag / Factor;

          SetTotal_AeroCD(Aero_CD);

          /*--- Set the solid surface drag ---*/

          su2double Solid_Drag = DmT - Force;
          su2double Solid_CD = Solid_Drag / Factor;

          SetTotal_SolidCD(Solid_CD);

          /*--- Set the net thrust value---*/

          su2double CT = NetThrust / Factor;

          SetTotal_NetThrust(CT);

          /*--- Set the total power ---*/

          su2double PowerHP = Power * Ref *  config->GetVelocity_Ref() / 550.0;

          SetTotal_Power(PowerHP);

          /*--- Set the total ReverseFlow ---*/

          su2double ReverseFlow;
          if (Engine) ReverseFlow = fabs(config->GetInflow_ReverseMassFlow(iMarker_Inlet)  / config->GetInflow_MassFlow(Inlet_TagBound));
          else ReverseFlow = fabs(config->GetActDisk_ReverseMassFlow(iMarker_Inlet)  / config->GetActDiskInlet_MassFlow(Inlet_TagBound));

          SetTotal_ReverseFlow(ReverseFlow);

          /*--- Set the total mass flow ratio ---*/

          InfVel2 = 0.0;  for (iDim = 0; iDim < nDim; iDim++) InfVel2  += Velocity_Inf[iDim]*Velocity_Inf[iDim];
          if (Engine) MFR =fabs(config->GetInflow_MassFlow(Inlet_TagBound)) / (Density_Inf * sqrt(InfVel2) * config->GetHighlite_Area());
          else MFR = fabs(config->GetActDiskInlet_MassFlow(Inlet_TagBound)) / (Density_Inf * sqrt(InfVel2) * config->GetHighlite_Area());
          SetTotal_MFR(MFR);

          /*--- Evaluate shaft power and adiabatic efficiency (average) ---*/

          su2double Pstatic1, P1, P2, T1, T2;
          if (Engine) {
            Pstatic1 = config->GetInflow_Pressure(Inlet_TagBound);
            P1 = config->GetInflow_TotalPressure(Inlet_TagBound);
            P2 = config->GetExhaust_TotalPressure(Outlet_TagBound);
            T1 = config->GetInflow_TotalTemperature(Inlet_TagBound);
            T2 = config->GetExhaust_TotalTemperature(Outlet_TagBound);
          }
          else {
            Pstatic1 = config->GetActDiskInlet_Pressure(Inlet_TagBound);
            P1 = config->GetActDiskInlet_TotalPressure(Inlet_TagBound);
            P2 = config->GetActDiskOutlet_TotalPressure(Outlet_TagBound);
            T1 = config->GetActDiskInlet_TotalTemperature(Inlet_TagBound);
            T2 = config->GetActDiskOutlet_TotalTemperature(Outlet_TagBound);
          }

          /*-- Set the propulsive efficiency ---*/

          su2double mu_prop = fabs(DmT)*sqrt(RefVel2)/Power;
          SetTotal_Prop_Eff(mu_prop);

          /*-- Set the bypass propulsive efficiency ---*/

          su2double mu_bypass_prop = NetThrust*sqrt(RefVel2)/Power;
          SetTotal_ByPassProp_Eff(mu_bypass_prop);

          /*-- Set the fan adiabatic efficiency ---*/

          su2double mu_isentropic = 0.0;
          if ((P2/P1) > 0.0) mu_isentropic =    (T1/(T2-T1))*(pow((P2/P1),(Gamma-1.0)/Gamma)-1.0);
          SetTotal_Adiab_Eff(mu_isentropic);

          /*-- Set the polytropic efficiency ---*/

          su2double poly_coeff = 1.0/(1.0-log(T2/T1)/log(P2/P1));
          su2double mu_polytropic = ((Gamma-1.0)/Gamma)/((poly_coeff-1.0)/poly_coeff);
          SetTotal_Poly_Eff(mu_polytropic);

          if (write_heads && Output && !config->GetDiscrete_Adjoint()) {

            if (iMarker_Inlet > 0) cout << endl;

            /*--- Geometry defintion ---*/

            if (Engine) cout <<"Engine surfaces: " << Inlet_TagBound << ", " << Outlet_TagBound << "." << endl;
            else cout <<"Actuator disk surfaces: " << Inlet_TagBound << ", " << Outlet_TagBound << "." << endl;

            if (nDim == 2) {
              if (config->GetSystemMeasurements() == SI)
                cout <<"CG (m): (" << Inlet_XCG_Total[iMarker_Inlet] <<", " << Inlet_YCG_Total[iMarker_Inlet] << "). Length (m): " << Inlet_Area_Total[iMarker_Inlet] << "." << endl;
              else if (config->GetSystemMeasurements() == US)
                cout <<"CG (in): (" << Inlet_XCG_Total[iMarker_Inlet]*12.0 <<", " << Inlet_YCG_Total[iMarker_Inlet]*12.0 << "). Length (in): " << Inlet_Area_Total[iMarker_Inlet]*12.0 << "." << endl;
              cout << endl;
            }

            if (nDim ==3) {
              if (config->GetSystemMeasurements() == SI)
                cout <<"CG (m): (" << Inlet_XCG_Total[iMarker_Inlet] <<", " << Inlet_YCG_Total[iMarker_Inlet] <<", " << Inlet_ZCG_Total[iMarker_Inlet] << "). Area (m^2): " << Inlet_Area_Total[iMarker_Inlet] << ". Radius (m): " << sqrt(Inlet_Area_Total[iMarker_Inlet]/PI_NUMBER) << "." << endl;
              else if (config->GetSystemMeasurements() == US)
                cout <<"CG (in): (" << Inlet_XCG_Total[iMarker_Inlet]*12.0 <<", " << Inlet_YCG_Total[iMarker_Inlet]*12.0 <<", " << Inlet_ZCG_Total[iMarker_Inlet]*12.0 << "). Area (in^2): " << Inlet_Area_Total[iMarker_Inlet]*12.0*12.0 << "." << endl;
              cout << endl;
            }


            /*--- Flow field descritption  ---*/

            if (config->GetSystemMeasurements() == SI) {
              cout << setprecision(2) << "Inlet Ave. P (Pa): " << Pstatic1*config->GetPressure_Ref() << setprecision(3) <<  ". Inlet Ave. Mach: " << Mach << "." << endl;
              cout << setprecision(2) << "Outlet Ave. PT (Pa): " << P2*config->GetPressure_Ref() << ". Outlet Ave. TT (K): " << T2*config->GetTemperature_Ref() << "." << endl;
            }
            else if (config->GetSystemMeasurements() == US) {
              cout << setprecision(2) << "Inlet Ave. P (psf): " << Pstatic1*config->GetPressure_Ref() << setprecision(3) <<  ". Inlet Ave. Mach: " << Mach << "." << endl;
              cout << setprecision(2) << "Outlet Ave. PT (psf): " << P2*config->GetPressure_Ref() << ". Outlet Ave. TT (R): " << T2*config->GetTemperature_Ref() << "." << endl;
            }

            cout << "Inlet min. PT/PTinf: " << MinPressure << ". Inlet max. PT/PTinf: " << MaxPressure << ". Inlet Ave. PT/PTinf: " << AvePressure << endl;

            su2double InfVel2, Inlet_MassFlow, Outlet_MassFlow;

            if (Engine) Inlet_MassFlow = fabs(config->GetInflow_MassFlow(Inlet_TagBound)) * config->GetDensity_Ref() * config->GetVelocity_Ref();
            else Inlet_MassFlow = fabs(config->GetActDiskInlet_MassFlow(Inlet_TagBound)) * config->GetDensity_Ref() * config->GetVelocity_Ref();

            if (config->GetSystemMeasurements() == SI) { cout << "Inlet mass flow (kg/s): "; cout << setprecision(2) << Inlet_MassFlow; }
            else if (config->GetSystemMeasurements() == US) { cout << "Inlet mass flow (lbs/s): "; cout << setprecision(2) << Inlet_MassFlow * 32.174; }

            if (Engine) Outlet_MassFlow = fabs(config->GetExhaust_MassFlow(Outlet_TagBound)) * config->GetDensity_Ref() * config->GetVelocity_Ref();
            else Outlet_MassFlow = fabs(config->GetActDiskOutlet_MassFlow(Outlet_TagBound)) * config->GetDensity_Ref() * config->GetVelocity_Ref();

            //          if (config->GetSystemMeasurements() == SI) { cout << ". Outlet mass flow (kg/s): "; cout << setprecision(2) << Outlet_MassFlow; }
            //          else if (config->GetSystemMeasurements() == US) { cout << ". Outlet mass flow (lbs/s): "; cout << setprecision(2) << Outlet_MassFlow * 32.174; }

            if (Inlet_MassFlow > Outlet_MassFlow) cout << ". I/O diff.: " << setprecision(2) << 100.0*fabs(1.0-(Outlet_MassFlow/Inlet_MassFlow)) << "%";
            else cout << ". I/O diff.: " << setprecision(2) << -100.0*fabs(1.0-(Inlet_MassFlow/Outlet_MassFlow)) << "%";

            InfVel2 = 0.0;  for (iDim = 0; iDim < nDim; iDim++) InfVel2  += Velocity_Inf[iDim]*Velocity_Inf[iDim];
            cout << setprecision(2) << ". MFR: " << MFR << "." << endl;

            if (!Engine) {

              cout << setprecision(3) << "PT in/out ratio: " << TotalPressRatio << ". TT in/out ratio: " << TotalTempRatio  <<  "." << endl;

              if (config->GetActDisk_Jump() == VARIABLES_JUMP) {
                if (config->GetSystemMeasurements() == SI) cout << setprecision(3) << "P in/out jump (Pa): ";
                else if (config->GetSystemMeasurements() == US) cout << setprecision(3) << "P in/out jump (psf): ";
                cout << setprecision(3) << DeltaPress * config->GetPressure_Ref();
                if (config->GetSystemMeasurements() == SI) cout << setprecision(3) << ". T in/out jump (K): ";
                else if (config->GetSystemMeasurements() == US) cout << setprecision(3) << ". T in/out jump (R): ";
                cout << setprecision(3) << DeltaTemp * config->GetTemperature_Ref() <<"."<< endl;
              }
              else  if (config->GetActDisk_Jump() == RATIO) {
                cout << setprecision(3) << "P in/out ratio: ";
                cout << setprecision(3) << StaticPressRatio;
                cout  << setprecision(3) <<". T in/out ratio: ";
                cout << setprecision(3) << StaticTempRatio <<"."<< endl;
              }
            }

            cout << setprecision(1) << "\nProp. eff. (D-T.V/Shaft P): " << 100*mu_prop << "%. By-pass prop. eff. (NetT.V/Shaft P): " << 100*mu_bypass_prop <<   "%." << endl;
            cout << setprecision(1) << "Fan adiabatic eff.: " << 100*mu_isentropic  << "%. Fan poly. eff.: " << 100*mu_polytropic << "%. Poly coeff. (n): " << setprecision(4) << poly_coeff << "." << endl;

            cout << endl;


            /*--- Forces descritption  ---*/

            if (config->GetSystemMeasurements() == SI) cout << setprecision(1) << "Ram Drag (N): ";
            else if (config->GetSystemMeasurements() == US) cout << setprecision(1) << "Ram Drag (lbf): ";
            cout << (GrossThrust-NetThrust) * Ref;

            if (config->GetSystemMeasurements() == SI) cout << setprecision(1) << ". Gross Thrust (N): ";
            else if (config->GetSystemMeasurements() == US) cout << setprecision(1) << ". Gross Thrust (lbf): ";
            cout << -GrossThrust * Ref  << "." << endl;

            if (config->GetSystemMeasurements() == SI) cout << setprecision(1) << "Open surfaces Thurst (N): ";
            else if (config->GetSystemMeasurements() == US) cout  << setprecision(1) << "Open surfaces Thrust (lbf): ";
            cout<< setprecision(1) << Force * Ref << ". Open surfaces CT: " << setprecision(5) << -Force / Factor << "." << endl;

            if (config->GetSystemMeasurements() == SI) cout << "Solid surfaces Drag (N): ";
            else if (config->GetSystemMeasurements() == US) cout << "Solid surfaces Drag (lbf): ";
            cout << setprecision(1) << Solid_Drag * Ref << ". Solid surfaces CD: " << setprecision(5) << Solid_CD << "." << endl;

            if (config->GetSystemMeasurements() == SI) cout << setprecision(1) <<"Net Thrust (N): ";
            else if (config->GetSystemMeasurements() == US) cout << setprecision(1) << "Net Thrust (lbf): ";
            cout << setprecision(5) << -NetThrust * Ref  << ". Net CT: " << CT;

            if (config->GetSystemMeasurements() == SI) {
              cout << ". Power (W): ";
              cout << setprecision(1) << Power * Ref *  config->GetVelocity_Ref()  << "." << endl;
            }
            else if (config->GetSystemMeasurements() == US) {
              cout << ". Power (HP): ";
              cout << setprecision(1) << Power * Ref *  config->GetVelocity_Ref() / 550.0 << "." << endl;
            }

            if (config->GetKind_ActDisk() == BLADE_ELEMENT) {
              cout << setprecision(5);
              cout << "Thrust_BEM (N): " << config->GetActDiskOutlet_Thrust_BEM(Outlet_TagBound) << ". ";
              cout << "Torque_BEM (N-m): " << config->GetActDiskOutlet_Torque_BEM(Outlet_TagBound) << "." << endl;
            }
          }

        }

        if (write_heads && Output && !config->GetDiscrete_Adjoint()) cout << "-------------------------------------------------------------------------" << endl << endl;

      }

    }

    delete [] Outlet_MassFlow_Local;
    delete [] Outlet_Temperature_Local;
    delete [] Outlet_TotalTemperature_Local;
    delete [] Outlet_Pressure_Local;
    delete [] Outlet_TotalPressure_Local;
    delete [] Outlet_Area_Local;
    delete [] Outlet_GrossThrust_Local;
    delete [] Outlet_Force_Local;
    delete [] Outlet_Power_Local;
    delete [] Outlet_DeltaP_Local;
    delete [] Outlet_Thrust_Local;
    delete [] Outlet_Torque_Local;

    delete [] Outlet_MassFlow_Total;
    delete [] Outlet_Temperature_Total;
    delete [] Outlet_TotalTemperature_Total;
    delete [] Outlet_Pressure_Total;
    delete [] Outlet_TotalPressure_Total;
    delete [] Outlet_Area_Total;
    delete [] Outlet_GrossThrust_Total;
    delete [] Outlet_Force_Total;
    delete [] Outlet_Power_Total;
    delete [] Outlet_DeltaP_Total;
    delete [] Outlet_Thrust_Total;
    delete [] Outlet_Torque_Total;

    delete [] Inlet_MassFlow_Local;
    delete [] Inlet_ReverseMassFlow_Local;
    delete [] Inlet_Temperature_Local;
    delete [] Inlet_TotalTemperature_Local;
    delete [] Inlet_Pressure_Local;
    delete [] Inlet_Mach_Local;
    delete [] Inlet_MinPressure_Local;
    delete [] Inlet_MaxPressure_Local;
    delete [] Inlet_TotalPressure_Local;
    delete [] Inlet_Area_Local;
    delete [] Inlet_RamDrag_Local;
    delete [] Inlet_Force_Local;
    delete [] Inlet_Power_Local;
    delete [] Inlet_XCG_Local;
    delete [] Inlet_YCG_Local;
    delete [] Inlet_ZCG_Local;

    delete [] Inlet_MassFlow_Total;
    delete [] Inlet_ReverseMassFlow_Total;
    delete [] Inlet_Temperature_Total;
    delete [] Inlet_TotalTemperature_Total;
    delete [] Inlet_Pressure_Total;
    delete [] Inlet_Mach_Total;
    delete [] Inlet_MinPressure_Total;
    delete [] Inlet_MaxPressure_Total;
    delete [] Inlet_TotalPressure_Total;
    delete [] Inlet_Area_Total;
    delete [] Inlet_RamDrag_Total;
    delete [] Inlet_Force_Total;
    delete [] Inlet_Power_Total;
    delete [] Inlet_XCG_Total;
    delete [] Inlet_YCG_Total;
    delete [] Inlet_ZCG_Total;

    delete [] Inlet_MassFlow;
    delete [] Inlet_Mach;
    delete [] Inlet_MinPressure;
    delete [] Inlet_MaxPressure;
    delete [] Inlet_ReverseMassFlow;
    delete [] Inlet_Pressure;
    delete [] Inlet_TotalPressure;
    delete [] Inlet_Temperature;
    delete [] Inlet_TotalTemperature;
    delete [] Inlet_Area;
    delete [] Inlet_RamDrag;
    delete [] Inlet_Force;
    delete [] Inlet_Power;
    delete [] Inlet_XCG;
    delete [] Inlet_YCG;
    delete [] Inlet_ZCG;

    delete [] Outlet_MassFlow;
    delete [] Outlet_Pressure;
    delete [] Outlet_TotalPressure;
    delete [] Outlet_Temperature;
    delete [] Outlet_TotalTemperature;
    delete [] Outlet_Area;
    delete [] Outlet_GrossThrust;
    delete [] Outlet_Force;
    delete [] Outlet_Power;
    delete [] Outlet_DeltaP;
    delete [] Outlet_Thrust;
    delete [] Outlet_Torque;

  }

}

void CEulerSolver::SetActDisk_BCThrust(CGeometry *geometry, CSolver **solver_container,
                                       CConfig *config, unsigned short iMesh, bool Output) {
  SU2_ZONE_SCOPED

  su2double Massflow = 0.0 , Target_Massflow = 0.0, DragMinusThrust = 0.0 ,
  Target_DragMinusThrust = 0.0, Target_NetThrust = 0.0, BCThrust = 0.0, BCThrust_inc = 0.0;
  unsigned short iDim, iMarker;
  unsigned long iVertex, iPoint;
  su2double  *V_inlet = nullptr, Pressure,
  Density, T0_Ti, ATerm, BTerm, LHS, RHS, RHS_PDelta, RHS_MDelta, F, DF_DLa, CTerm_, DTerm_,
  ETerm, La, La_old, TotalArea, To_Ti, DeltaT, Po_Pi, DeltaP, Area, Velocity_Normal,
  SoundSpeed2, Force_Normal,
  RefDensity, RefArea, RefVel2, Factor, Ref;
  unsigned short iter;
  string Marker_Tag;
  su2double Target_Force, Force, Target_Power, Power, NetThrust, BCThrust_old, Initial_BCThrust;
  bool ActDisk_Info;
  su2double MyBCThrust, BCThrust_Init;
  su2double Vector[MAXNDIM] = {0.0};

  su2double dNetThrust_dBCThrust        = config->GetdNetThrust_dBCThrust();
  unsigned short Kind_ActDisk           = config->GetKind_ActDisk();
  bool ratio                            = (config->GetActDisk_Jump() == RATIO);
  unsigned long Update_BCThrust         = config->GetUpdate_BCThrust();
  unsigned long Iter_Fixed_NetThrust    = config->GetIter_Fixed_NetThrust();
  unsigned long InnerIter                 = config->GetInnerIter();
  bool Update_BCThrust_Bool             = false;
  bool restart                          = (config->GetRestart() || config->GetRestart_Flow());
  su2double Fan_Poly_Eff                = config->GetFan_Poly_Eff();
  su2double PolyCoeff                   = 1.0/(1.0-((Gamma-1.0)/Gamma)/Fan_Poly_Eff);

  RefDensity   = Density_Inf;
  RefArea = config->GetRefArea();
  RefVel2 = 0.0;  for (iDim = 0; iDim < nDim; iDim++) RefVel2  += Velocity_Inf[iDim]*Velocity_Inf[iDim];

  Factor = (0.5*RefDensity*RefArea*RefVel2);
  Ref = config->GetDensity_Ref() * config->GetVelocity_Ref() * config->GetVelocity_Ref() * 1.0 * 1.0;

  /*--- Blade element distribution is in input file. ---*/
  if (Kind_ActDisk == BLADE_ELEMENT) {
    SetActDisk_BEM_VLAD(geometry, solver_container, config, iMesh, Output);
  }

  /*--- Variable load distribution is in input file. ---*/
  if (Kind_ActDisk == VARIABLE_LOAD) {
    if(InnerIter == 0) {
      ReadActDisk_InputFile(geometry, solver_container, config, iMesh, Output);
    }
  }
  /*--- Delta P and delta T are inputs ---*/

  else if (Kind_ActDisk == VARIABLES_JUMP) {

    for (iMarker = 0; iMarker < config->GetnMarker_All(); iMarker++) {

      if ((config->GetMarker_All_KindBC(iMarker) == ACTDISK_INLET) ||
          (config->GetMarker_All_KindBC(iMarker) == ACTDISK_OUTLET)) {

        Marker_Tag = config->GetMarker_All_TagBound(iMarker);

        if (ratio) {
          if (config->GetMach()  < 0.5) {
            DeltaP       = config->GetActDisk_PressJump(Marker_Tag, 0);
            DeltaT       = config->GetActDisk_TempJump(Marker_Tag, 0);
          }
          else {
            DeltaP       = config->GetActDisk_PressJump(Marker_Tag, 1);
            DeltaT       = config->GetActDisk_TempJump(Marker_Tag, 1);
          }
        }
        else {
          if (config->GetMach()  < 0.5) {
            DeltaP       = max(0.0, config->GetActDisk_PressJump(Marker_Tag, 0) / config->GetPressure_Ref());
            DeltaT       = max(0.0, config->GetActDisk_TempJump(Marker_Tag, 0) / config->GetTemperature_Ref());
          }
          else {
            DeltaP       = max(0.0, config->GetActDisk_PressJump(Marker_Tag, 1) / config->GetPressure_Ref());
            DeltaT       = max(0.0, config->GetActDisk_TempJump(Marker_Tag, 1) / config->GetTemperature_Ref());
          }
        }

        /*--- Set the Delta P, Delta T values at each discrete point (uniform distribution)  ---*/

        for (iVertex = 0; iVertex < geometry->nVertex[iMarker]; iVertex++) {
          iPoint = geometry->vertex[iMarker][iVertex]->GetNode();
          ActDisk_DeltaP[iMarker][iVertex] = DeltaP;
          ActDisk_DeltaT[iMarker][iVertex] = DeltaT;
        }
      }
    }
  }

  /*--- Iteration using BCThrust ---*/

  else {

    if (InnerIter == 0) BCThrust_Counter = 0;

    /*--- Only the fine mesh level should check the convergence criteria ---*/

    if ((iMesh == MESH_0) && Output) {

      /*--- Initialize the update flag to false ---*/

      Update_BCThrust_Bool = false;

      /*--- Reevaluate BCThrust at a fix number of iterations ---*/

      if ((InnerIter % Iter_Fixed_NetThrust == 0) && (InnerIter != 0)) {
        BCThrust_Counter++;
        Update_BCThrust_Bool = (BCThrust_Counter != 0) &&
            (BCThrust_Counter != 1) &&
            (BCThrust_Counter != Update_BCThrust) &&
            (BCThrust_Counter != Update_BCThrust + 2) &&
            (BCThrust_Counter != Update_BCThrust + 4);
      }

      /*--- Store the update boolean for use on other mesh levels in the MG ---*/

      config->SetUpdate_BCThrust_Bool(Update_BCThrust_Bool);

    }

    else {
      Update_BCThrust_Bool = config->GetUpdate_BCThrust_Bool();
    }


    /*--- If it is the first iteration, set the BCThrust to a meaning full target value,
     * this can be done at an initialization level, for the time being it is OK here ---*/

    if (InnerIter == 0) {
      for (iMarker = 0; iMarker < config->GetnMarker_All(); iMarker++) {
        if ((config->GetMarker_All_KindBC(iMarker) == ACTDISK_INLET) ||
            (config->GetMarker_All_KindBC(iMarker) == ACTDISK_OUTLET)) {
          Marker_Tag = config->GetMarker_All_TagBound(iMarker);

          if (Kind_ActDisk == NET_THRUST) {
            if (restart)
              Initial_BCThrust = config->GetInitial_BCThrust() / Ref;
            else {
              if (config->GetMach() < 0.5) Initial_BCThrust = fabs( config->GetActDisk_PressJump(Marker_Tag, 0) / Ref);
              else Initial_BCThrust = fabs( config->GetActDisk_PressJump(Marker_Tag, 1) / Ref);
            }
            config->SetActDisk_BCThrust(Marker_Tag, Initial_BCThrust);
            config->SetActDisk_BCThrust_Old(Marker_Tag, Initial_BCThrust);
          }

          if (Kind_ActDisk == BC_THRUST) {
            if (restart)
              Initial_BCThrust = config->GetInitial_BCThrust() / Ref;
            else {
              if (config->GetMach() < 0.5) Initial_BCThrust = fabs( config->GetActDisk_PressJump(Marker_Tag, 0) / Ref);
              else Initial_BCThrust = fabs( config->GetActDisk_PressJump(Marker_Tag, 1) / Ref);
            }
            config->SetActDisk_BCThrust(Marker_Tag, Initial_BCThrust);
            config->SetActDisk_BCThrust_Old(Marker_Tag, Initial_BCThrust);
          }

          if (Kind_ActDisk == POWER) {
            Initial_BCThrust = config->GetInitial_BCThrust() / Ref;
            config->SetActDisk_BCThrust(Marker_Tag, Initial_BCThrust);
            config->SetActDisk_BCThrust_Old(Marker_Tag, Initial_BCThrust);
          }

          if (Kind_ActDisk == DRAG_MINUS_THRUST) {
            Initial_BCThrust = config->GetInitial_BCThrust() / Ref;
            config->SetActDisk_BCThrust(Marker_Tag, Initial_BCThrust);
            config->SetActDisk_BCThrust_Old(Marker_Tag, Initial_BCThrust);
          }

          if (Kind_ActDisk == MASSFLOW) {
            Initial_BCThrust = config->GetInitial_BCThrust() / Ref;
            config->SetActDisk_BCThrust(Marker_Tag, Initial_BCThrust);
            config->SetActDisk_BCThrust_Old(Marker_Tag, Initial_BCThrust);
          }

        }
      }
    }

    /*--- Typical iteration to set the value of BC Thrust at each actuator disk ---*/

    if (Update_BCThrust_Bool && Output) {

      for (iMarker = 0; iMarker < config->GetnMarker_All(); iMarker++) {

        if ((config->GetMarker_All_KindBC(iMarker) == ACTDISK_INLET) ||
            (config->GetMarker_All_KindBC(iMarker) == ACTDISK_OUTLET)) {

          Marker_Tag = config->GetMarker_All_TagBound(iMarker);

          if (Kind_ActDisk == NET_THRUST) {

            if (config->GetMach() < 0.5) Target_NetThrust = fabs( config->GetActDisk_PressJump(Marker_Tag, 0) / Ref);
            else Target_NetThrust = fabs( config->GetActDisk_PressJump(Marker_Tag, 1) / Ref);
            NetThrust    = config->GetActDisk_NetThrust(Marker_Tag);
            BCThrust_old = config->GetActDisk_BCThrust_Old(Marker_Tag);
            BCThrust_inc = (1.0/dNetThrust_dBCThrust)*(Target_NetThrust - NetThrust);

            if (iMesh == MESH_0) BCThrust = max(0.0,(BCThrust_old + BCThrust_inc));
            else BCThrust = config->GetActDisk_BCThrust(Marker_Tag);

            if (iMesh == MESH_0) {
              config->SetActDisk_BCThrust(Marker_Tag, BCThrust);
              BCThrust_Init = BCThrust*Ref;
              config->SetInitial_BCThrust(BCThrust_Init);
            }

          }

          if (Kind_ActDisk == BC_THRUST) {

            if (config->GetMach() < 0.5) Target_Force =  fabs( config->GetActDisk_PressJump(Marker_Tag, 0) / Ref);
            else Target_Force =  fabs( config->GetActDisk_PressJump(Marker_Tag, 1) / Ref);
            Force        = -config->GetActDisk_Force(Marker_Tag);
            BCThrust_old = config->GetActDisk_BCThrust_Old(Marker_Tag);
            BCThrust_inc = (1.0/dNetThrust_dBCThrust)*(Target_Force - Force);

            if (iMesh == MESH_0) BCThrust = max(0.0,(BCThrust_old + BCThrust_inc));
            else BCThrust = config->GetActDisk_BCThrust(Marker_Tag);

            if (iMesh == MESH_0) {
              config->SetActDisk_BCThrust(Marker_Tag, BCThrust);
              BCThrust_Init = BCThrust*Ref;
              config->SetInitial_BCThrust(BCThrust_Init);
            }

          }

          if (Kind_ActDisk == POWER) {

            if (config->GetMach() < 0.5) Target_Power =  fabs( config->GetActDisk_PressJump(Marker_Tag, 0) / (Ref * config->GetVelocity_Ref() /  550.0));
            else Target_Power =  fabs( config->GetActDisk_PressJump(Marker_Tag, 1) / (Ref * config->GetVelocity_Ref() /  550.0));
            Power        = config->GetActDisk_Power(Marker_Tag);
            BCThrust_old = config->GetActDisk_BCThrust_Old(Marker_Tag);
            BCThrust_inc = (1.0/dNetThrust_dBCThrust)*(Target_Power - Power);

            if (iMesh == MESH_0) BCThrust = max(0.0,(BCThrust_old + BCThrust_inc));
            else BCThrust = config->GetActDisk_BCThrust(Marker_Tag);

            if (iMesh == MESH_0) {
              config->SetActDisk_BCThrust(Marker_Tag, BCThrust);
              BCThrust_Init = BCThrust*Ref;
              config->SetInitial_BCThrust(BCThrust_Init);
            }

          }

          if (Kind_ActDisk == DRAG_MINUS_THRUST) {

            if (config->GetMach() < 0.5) Target_DragMinusThrust  =  -fabs(config->GetActDisk_PressJump(Marker_Tag, 0)) * Factor;
            else Target_DragMinusThrust  =  -fabs(config->GetActDisk_PressJump(Marker_Tag, 1)) * Factor;
            DragMinusThrust = GetTotal_CD() * Factor;
            BCThrust_old    = config->GetActDisk_BCThrust_Old(Marker_Tag);
            BCThrust_inc    = -(1.0/dNetThrust_dBCThrust)*(Target_DragMinusThrust - DragMinusThrust);

            if (iMesh == MESH_0) BCThrust = max(0.0,(BCThrust_old + BCThrust_inc));
            else BCThrust = config->GetActDisk_BCThrust(Marker_Tag);

            if (iMesh == MESH_0) {
              config->SetActDisk_BCThrust(Marker_Tag, BCThrust);
              BCThrust_Init = BCThrust*Ref;
              config->SetInitial_BCThrust(BCThrust_Init);
            }

          }

          if (Kind_ActDisk == MASSFLOW) {

            if (config->GetMach() < 0.5) {
              Target_Massflow  =  fabs(config->GetActDisk_PressJump(Marker_Tag, 0) / (config->GetDensity_Ref() * config->GetVelocity_Ref()));
              if (config->GetSystemMeasurements() == US) Target_Massflow /= 32.174;
            }
            else {
              Target_Massflow  =  fabs(config->GetActDisk_PressJump(Marker_Tag, 1) / (config->GetDensity_Ref() * config->GetVelocity_Ref()));
              if (config->GetSystemMeasurements() == US) Target_Massflow /= 32.174;
            }

            Massflow = config->GetActDisk_MassFlow(Marker_Tag);
            BCThrust_old    = config->GetActDisk_BCThrust_Old(Marker_Tag);
            BCThrust_inc    = (1.0/dNetThrust_dBCThrust)*(Target_Massflow - Massflow);
            if (iMesh == MESH_0) BCThrust = max(0.0,(BCThrust_old + BCThrust_inc));
            else BCThrust = config->GetActDisk_BCThrust(Marker_Tag);
            if (iMesh == MESH_0) {
              config->SetActDisk_BCThrust(Marker_Tag, BCThrust);
              BCThrust_Init = BCThrust*Ref;
              config->SetInitial_BCThrust(BCThrust_Init);
            }

          }

        }

      }

      /*--- After a complete update of BC_Thrust
       update the value of BC Thrust (old) for future iterations ---*/

      for (iMarker = 0; iMarker < config->GetnMarker_All(); iMarker++) {
        if ((config->GetMarker_All_KindBC(iMarker) == ACTDISK_INLET) ||
            (config->GetMarker_All_KindBC(iMarker) == ACTDISK_OUTLET)) {
          Marker_Tag = config->GetMarker_All_TagBound(iMarker);
          if ((Kind_ActDisk == NET_THRUST) || (Kind_ActDisk == BC_THRUST) ||
              (Kind_ActDisk == POWER) || (Kind_ActDisk == DRAG_MINUS_THRUST) ||
              (Kind_ActDisk == MASSFLOW)) {
            BCThrust = config->GetActDisk_BCThrust(Marker_Tag);
            config->SetActDisk_BCThrust_Old(Marker_Tag, BCThrust);
          }
        }
      }
    }

    /*--- Evaluate the pressure jump at each node using the total thrust ---*/

    if ((Update_BCThrust_Bool && Output) || (InnerIter == 0)) {

      for (iMarker = 0; iMarker < config->GetnMarker_All(); iMarker++) {

        if ((config->GetMarker_All_KindBC(iMarker) == ACTDISK_INLET) ||
            (config->GetMarker_All_KindBC(iMarker) == ACTDISK_OUTLET)) {

          Marker_Tag = config->GetMarker_All_TagBound(iMarker);
          RefDensity  = Density_Inf;
          RefArea = config->GetRefArea();
          RefVel2 = 0.0; for (iDim = 0; iDim < nDim; iDim++) RefVel2  += Velocity_Inf[iDim]*Velocity_Inf[iDim];

          Factor = (0.5*RefDensity*RefArea*RefVel2);
          Ref = config->GetDensity_Ref() * config->GetVelocity_Ref() * config->GetVelocity_Ref() * 1.0 * 1.0;
          BCThrust = config->GetActDisk_BCThrust(Marker_Tag);

          for (iVertex = 0; iVertex < geometry->nVertex[iMarker]; iVertex++) {

            iPoint = geometry->vertex[iMarker][iVertex]->GetNode();

            if (geometry->nodes->GetDomain(iPoint)) {

              geometry->vertex[iMarker][iVertex]->GetNormal(Vector);

              if (config->GetMarker_All_KindBC(iMarker) == ACTDISK_INLET) {
                for (iDim = 0; iDim < nDim; iDim++) { Vector[iDim] = -Vector[iDim]; }
              }

              Area = GeometryToolbox::Norm(nDim, Vector);

              /*--- Use the inlet state to compute the Pressure and Temperature jumps ---*/

              if (config->GetMarker_All_KindBC(iMarker) == ACTDISK_INLET)
                V_inlet = nodes->GetPrimitive(iPoint);
              if (config->GetMarker_All_KindBC(iMarker) == ACTDISK_OUTLET)
                V_inlet = DonorPrimVar[iMarker][iVertex];

              Density       = V_inlet[nDim+2];
              Pressure      = V_inlet[nDim+1];
              SoundSpeed2   = Pressure*Gamma/Density;
              TotalArea     = config->GetActDisk_Area(Marker_Tag);
              Force_Normal  = Area*(BCThrust/TotalArea);


              Velocity_Normal = 0.0;
              for (iDim = 0; iDim < nDim; iDim++) {
                Velocity_Normal += V_inlet[iDim+1]*Vector[iDim]/Area;
              }


              if (Velocity_Normal > EPS) {

                /*--- Ratio of the total temperature to the temperature at the inflow ---*/

                T0_Ti = 1.0 + ((Gamma-1.0)/SoundSpeed2)*(0.5*Velocity_Normal*Velocity_Normal + Force_Normal/(Density*Area));


                ATerm = 2.0*T0_Ti/(Gamma+1.0);
                BTerm = 0.5*(Gamma+1.0)/(Gamma-1.0);
                LHS = fabs(Velocity_Normal)/(sqrt(SoundSpeed2)*pow(ATerm,BTerm));

                CTerm_ = (PolyCoeff-1.0)/(PolyCoeff+1.0);
                DTerm_ = 1.0/(PolyCoeff-1.0);

                La = EPS; La_old = EPS;

                for (iter = 0; iter < 100; iter++) {

                  ETerm = ((1.0-CTerm_*La*La)/(1.0-CTerm_+EPS));

                  RHS = La*pow(ETerm, DTerm_);

                  ETerm = ((1.0-CTerm_*(La+1E-6)*(La+1E-6))/(1.0-CTerm_+EPS));
                  RHS_PDelta = (La+1E-6)*pow(ETerm, DTerm_);

                  ETerm = ((1.0-CTerm_*(La-1E-6)*(La-1E-6))/(1.0-CTerm_+EPS));
                  RHS_MDelta = (La-1E-6)*pow(ETerm, DTerm_);

                  /*--- Objective function and finitte differences derivative ---*/

                  F = RHS - LHS;
                  DF_DLa = (RHS_PDelta - RHS_MDelta)/2E-6;

                  /*--- Newton's step ---*/

                  La_old = La;
                  La = La_old - 0.75*(F/DF_DLa);

                  if (fabs(F) < 1E-10) break;

                }

                if (iter == 99) cout << "The laval number evaluation is not converging." << endl;

                /*--- Laval is bounded ---*/

                La = min(La, sqrt(6.0));  La = max(La, 0.0);

                To_Ti = max(1.0, T0_Ti*(1.0-CTerm_*La*La));
                ActDisk_DeltaT[iMarker][iVertex] = To_Ti;

                Po_Pi = max(1.0, pow(To_Ti, PolyCoeff*DTerm_));
                ActDisk_DeltaP[iMarker][iVertex] = Po_Pi;

              }
              else {
                ActDisk_DeltaT[iMarker][iVertex] = 1.0;
                ActDisk_DeltaP[iMarker][iVertex] = 1.0;
              }
            }
          }
        }
      }
    }
  }

  /*--- Broadcast some information to the master node ---*/

  ActDisk_Info = false;
  for (iMarker = 0; iMarker < config->GetnMarker_All(); iMarker++) {
    if ((config->GetMarker_All_KindBC(iMarker) == ACTDISK_INLET) ||
        (config->GetMarker_All_KindBC(iMarker) == ACTDISK_OUTLET)) {
      ActDisk_Info = true;
    }
  }
  if (!ActDisk_Info) config->SetInitial_BCThrust(0.0);

  MyBCThrust = config->GetInitial_BCThrust();
  SU2_MPI::Allreduce(&MyBCThrust, &BCThrust, 1, MPI_DOUBLE, MPI_MAX, SU2_MPI::GetComm());
  config->SetInitial_BCThrust(BCThrust);

}

void CEulerSolver::ReadActDisk_InputFile(CGeometry *geometry, CSolver **solver_container,
                                       CConfig *config, unsigned short iMesh, bool Output) {
  SU2_ZONE_SCOPED
  /*--- Input file provides force coefficients distributions along disk radius. Initialization
        necessary only at initial iteration. ---*/

  unsigned short iDim, iMarker;
  unsigned long iVertex, iPoint;
  string Marker_Tag;
  int iRow, nRow, iEl;
  std::vector<su2double> rad_v, dCt_v, dCp_v, dCr_v;
  su2double r_ = 0.0, r[MAXNDIM] = {0.0},
  AD_Center[MAXNDIM] = {0.0}, AD_Axis[MAXNDIM] = {0.0}, AD_Radius = 0.0, AD_J = 0.0;
  std::vector<su2double> Fa, Ft, Fr;
  su2double Fx = 0.0, Fy = 0.0, Fz = 0.0,
  Fx_inf = 0.0, Fy_inf = 0.0, Fz_inf = 0.0, Fx_sup = 0.0, Fy_sup = 0.0, Fz_sup = 0.0, h = 0.0;
  const su2double *P = nullptr;

  su2double Dens_FreeStream = config->GetDensity_FreeStream();
  const su2double *Vel_FreeStream = config->GetVelocity_FreeStream();

  /*--- Get the file name that contains the propeller data. ---*/
  string ActDisk_filename = config->GetActDisk_FileName();
  /*--- Loop over the markers. ---*/
  for (iMarker = 0; iMarker < config->GetnMarker_All(); iMarker++) {
    if ((config->GetMarker_All_KindBC(iMarker) == ACTDISK_INLET) ||
        (config->GetMarker_All_KindBC(iMarker) == ACTDISK_OUTLET)) {

      /*--- Get the marker tag of the current BC marker. ---*/
      Marker_Tag = config->GetMarker_All_TagBound(iMarker);
      ifstream ActDisk_file;
      /*--- Open the file that contains the propeller data. ---*/
      ActDisk_file.open(ActDisk_filename.data(), ios::in);

      /*--- Error message if the propeller data input file fails to open. ---*/
      if (ActDisk_file.fail()) SU2_MPI::Error("Unable to open Actuator Disk Input File", CURRENT_FUNCTION);

      string text_line, text_line_appo, name[2];
      string::size_type position;

      while (getline (ActDisk_file, text_line)) {
        /*--- Check if there is the "MARKER_ACTDISK=" string in the current line. If not keep on reading. ---*/
        position = text_line.find ("MARKER_ACTDISK=");
        if(position == string::npos){continue;}
        text_line.erase (0,15);
        /*--- Read the names of the two faces of the actuator disk and assign them to the name[] array. ---*/
        istringstream NameID(text_line);
        for (int i = 0; i < 2; i++){
          NameID >> name[i];
        }

        /*--- Check if the propeller data correspond to the actual BC marker. ---*/
        if (Marker_Tag == name[0] || Marker_Tag == name[1]){
          /*--- Read and assign the coordinates of the actuator disk center. ---*/
          getline (ActDisk_file, text_line_appo);
          text_line_appo.erase (0,7);
          istringstream C_value(text_line_appo);
          for (iDim = 0; iDim < nDim; iDim++){
            C_value >> AD_Center[iDim];
          }

          /*--- Read and assign the components of the actuator disk axis versor pointing backward. ---*/
          getline (ActDisk_file, text_line_appo);
          text_line_appo.erase (0,5);
          istringstream axis_value(text_line_appo);
          for (iDim = 0; iDim < nDim; iDim++){
            axis_value >> AD_Axis[iDim];
          }

          /*--- Read and assign the value of the actuator disk radius. ---*/
          getline (ActDisk_file, text_line_appo);
          text_line_appo.erase (0,7);
          istringstream R_value(text_line_appo);
          R_value >> AD_Radius;

          /*--- Read and assign the value of the actuator disk advance ratio. ---*/
          getline (ActDisk_file, text_line_appo);
          text_line_appo.erase (0,10);
          istringstream J_value(text_line_appo);
          J_value >> AD_J;

          /*--- Read and assign the number of radial stations contained in the propeller data file. ---*/
          getline (ActDisk_file, text_line_appo);
          text_line_appo.erase (0,5);
          istringstream row_value(text_line_appo);
          row_value >> nRow;

          /*--- Assign the vectors dimension. ---*/
          rad_v.resize(nRow);
          dCt_v.resize(nRow);
          dCp_v.resize(nRow);
          dCr_v.resize(nRow);

          Fa.resize(nRow);
          Ft.resize(nRow);
          Fr.resize(nRow);

          /*--- Read and assign the values of the non-dimensional radius, thrust coefficient, power coefficient
                and radial force coefficient. ---*/
          getline (ActDisk_file, text_line_appo);
          for (iRow = 0; iRow < nRow; iRow++){
            getline (ActDisk_file, text_line_appo);
            istringstream row_val_value(text_line_appo);
            row_val_value >> rad_v[iRow] >> dCt_v[iRow] >> dCp_v[iRow] >> dCr_v[iRow];
          }

          /*--- Set the actuator disk radius value, center coordiantes values and axis coordinates values. ---*/
          ActDisk_R(iMarker) = AD_Radius;
          for (iDim = 0; iDim < nDim; iDim++){
            ActDisk_C(iMarker, iDim) = AD_Center[iDim];
            ActDisk_Axis(iMarker, iDim) = AD_Axis[iDim];
          }

          /*--- If the first radial station corresponds to the actuator disk center, the radial and tangential forces
                per unit area (Fr and Ft) are equal to zero, while the axial force per unit area (Fa) is computed using
                a linear interpolation in order to avoid a mathematical singularity at actuator disk center. ---*/
          if (rad_v[0] == 0.0){
            Fa[0] = (((2*Dens_FreeStream*pow(Vel_FreeStream[0],2))/
                    (pow(AD_J,2)*PI_NUMBER))*((dCt_v[1] - dCt_v[0])/rad_v[1])) / config->GetPressure_Ref();
            Ft[0] = 0.0;
            Fr[0] = 0.0;
          }
          else {
            Fa[0] = (dCt_v[0]*(2*Dens_FreeStream*pow(Vel_FreeStream[0],2))/
                    (pow(AD_J,2)*PI_NUMBER*rad_v[0])) / config->GetPressure_Ref();
            Ft[0] = (dCp_v[0]*(2*Dens_FreeStream*pow(Vel_FreeStream[0],2))/
                    ((AD_J*PI_NUMBER*rad_v[0])*(AD_J*PI_NUMBER*rad_v[0]))) / config->GetPressure_Ref();
            Fr[0] = (dCr_v[0]*(2*Dens_FreeStream*pow(Vel_FreeStream[0],2))/
                    (pow(AD_J,2)*PI_NUMBER*rad_v[0])) / config->GetPressure_Ref();
          }

          /*--- Loop over the radial stations. Computation of Fa (axial force per unit area), Ft (tangential force per unit area)
                and Fr (radial force per unit area).
                These equations are not valid if the freestream velocity is equal to zero (hovering condition not enabled yet). ---*/
          for (iEl = 1; iEl < nRow; iEl++){
            Fa[iEl] = (dCt_v[iEl]*(2*Dens_FreeStream*pow(Vel_FreeStream[0],2))/
                      (pow(AD_J,2)*PI_NUMBER*rad_v[iEl])) / config->GetPressure_Ref();
            Ft[iEl] = (dCp_v[iEl]*(2*Dens_FreeStream*pow(Vel_FreeStream[0],2))/
                      ((AD_J*PI_NUMBER*rad_v[iEl])*(AD_J*PI_NUMBER*rad_v[iEl]))) / config->GetPressure_Ref();
            Fr[iEl] = (dCr_v[iEl]*(2*Dens_FreeStream*pow(Vel_FreeStream[0],2))/
                      (pow(AD_J,2)*PI_NUMBER*rad_v[iEl])) / config->GetPressure_Ref();
          }

          /*--- Loop over the marker nodes. ---*/
          for (iVertex = 0; iVertex < geometry->nVertex[iMarker]; iVertex++) {
            /*--- Get the coordinates of the current node. ---*/
            iPoint = geometry->vertex[iMarker][iVertex]->GetNode();
            P = geometry->nodes->GetCoord(iPoint);

            /*--- Computation of the radius coordinates for the current node. ---*/
            GeometryToolbox::Distance(nDim, P, AD_Center, r);

            /*--- Computation of the non-dimensional radius for the current node. ---*/
            r_ = GeometryToolbox::Norm(nDim, r) / AD_Radius;

            /*--- Loop over the actuator disk radial stations. ---*/
            for (iEl = 0; iEl < nRow; iEl++){
              /*--- Check if the current node is located between rad_v[iEl] and rad_v[iEl-1]. ---*/
              if (r_ <= rad_v[iEl]){
                /*--- h is the dinstance of the current node from the previous radial element (iEl-1)
                      divided by the length of the radial element in which the node is contained. ---*/
                h = (r_-rad_v[iEl-1])/(rad_v[iEl]-rad_v[iEl-1]);
                /*--- Fx, Fy and Fz are the x, y and z components of the tangential and radial forces
                      per unit area resultant. ---*/
                if(r_ == 0.0){
                  Fx = 0.0;
                  Fy = 0.0;
                  Fz = 0.0;
                }
                /*--- _inf is the value of the previous radial element. _sup is the value of the
                      following radial element. ---*/
                else{
                  Fx_inf = (Ft[iEl-1]+Fr[iEl-1])*(r[0]/(r_*AD_Radius));
                  Fy_inf = (Ft[iEl-1]+Fr[iEl-1])*(r[2]/(r_*AD_Radius));
                  Fz_inf = -(Ft[iEl-1]+Fr[iEl-1])*(r[1]/(r_*AD_Radius));
                  Fx_sup = (Ft[iEl]+Fr[iEl])*(r[0]/(r_*AD_Radius));
                  Fy_sup = (Ft[iEl]+Fr[iEl])*(r[2]/(r_*AD_Radius));
                  Fz_sup = -(Ft[iEl]+Fr[iEl])*(r[1]/(r_*AD_Radius));

                  /*--- Fx, Fy and Fz at the current node are evaluated using a linear interpolation between
                        the end vaues of the radial element in which the current node is contained. ---*/
                  Fx = Fx_inf + (Fx_sup - Fx_inf)*h;
                  Fy = Fy_inf + (Fy_sup - Fy_inf)*h;
                  Fz = Fz_inf + (Fz_sup - Fz_inf)*h;
                }
                /*--- Set the values of Fa, Fx, Fy and Fz. Fa is evaluated using a linear interpolation. ---*/
                ActDisk_Fa[iMarker][iVertex] = Fa[iEl-1] + (Fa[iEl]-Fa[iEl-1])*h;
                ActDisk_Fx[iMarker][iVertex] = Fx;
                ActDisk_Fy[iMarker][iVertex] = Fy;
                ActDisk_Fz[iMarker][iVertex] = Fz;

                break;
              }
            }
          }
        }
      }
    }
  }
}

void CEulerSolver::SetActDisk_BEM_VLAD(CGeometry *geometry, CSolver **solver_container,
                                       CConfig *config, unsigned short iMesh, bool Output) {
  SU2_ZONE_SCOPED

  /*!
   * \function SetActDisk_BEM_VLAD
   * \brief Actuator disk model with Blade Element Method (BEM)
   * \author: Chandukrishna Y., T. N. Venkatesh and Josy Pullockara
   * Institution: Computational and Theoretical Fluid Dynamics (CTFD),
   *            CSIR - National Aerospace Laboratories, Bangalore
   *            Academy of Scientific and Innovative Research, Ghaziabad
   * \version 8.6.0 "Harrier"
   * First release date : September 26 2023
   * modified on:
   *
   * Section properties of the propeller given in an input file.
   * Cl, Cd of propeller sections need to be generated earlier and saved in this file
   * Actuator disk data initialized in function SetActDisk_BCThrust.
   * Propeller load calculated with Blade Element Method
   * Interpolated load at each point on the actuator disk is set in SetActDisk_BEM_VLAD function
   * Rest calculations follows the Variable Load (BC_ActDisk_VariableLoad) approach
   *
   */

  /*--- InputFile reading ---*/
  static int ADBem_NBlade = 0, ADBem_NSection = 0, ADBem_NAlpha = 0;
  static su2double ADBem_Diameter = 0.0, ADBem_HubRadius = 0.0, ADBem_Angle75R = 0.0;
  static std::vector<su2double> i_v, radius_v, chord_v, angle75r_v;
  static std::vector<std::vector<su2double> > alpha_m, cl_m, cd_m;

  static su2double ADBem_Omega = 0.0;
  static su2double ADBem_CG[MAXNDIM] = {0.0, 0.0, 0.0};
  static su2double ADBem_Axis[MAXNDIM] = {0.0}, ADBem_J = 0.0;
  static unsigned short ADBem_Frequency = 0;

  /*--- BEM VLAD ---*/
  const int BEM_MAX_ITER = 20;

  unsigned long InnerIter = config->GetInnerIter();
  su2double Dens_FreeStream = config->GetDensity_FreeStream();
  const su2double* Vel_FreeStream = config->GetVelocity_FreeStream();

  /*--- Input file provides force coefficients distributions along disk radius.
        Initialization necessary only at initial iteration (InnerIter == 0)
        when the tables (radius_v, chord_v, ...) are empty. ---*/
  if (radius_v.empty()) {
    /*--- Get the RPM, CG, Axis and Frequency from config. ---*/
    for (unsigned short iMarker = 0; iMarker < config->GetnMarker_All(); iMarker++) {

      if ((config->GetMarker_All_KindBC(iMarker) == ACTDISK_INLET) ||
          (config->GetMarker_All_KindBC(iMarker) == ACTDISK_OUTLET)) {

        const string Marker_Tag = config->GetMarker_All_TagBound(iMarker);
        ADBem_Omega = config->GetActDisk_Omega(Marker_Tag, 0);
        for (unsigned short iDim = 0; iDim < nDim; iDim++) {
          ADBem_CG[iDim] = config->GetActDiskBem_CG(iDim, Marker_Tag, 0);
          ADBem_Axis[iDim] = config->GetActDiskBem_Axis(iDim, Marker_Tag, 0);
        }
      }
    }
    ADBem_Frequency = config->GetActDiskBem_Frequency();

    /*--- Get the file name that contains the propeller data. ---*/
    string ActDiskBem_filename = config->GetBEM_prop_filename();
    ifstream ActDiskBem_file;

    /*--- Open the file that contains the propeller data. ---*/
    ActDiskBem_file.open(ActDiskBem_filename.data(), ios::in);

    /*--- Error message if the propeller data input file fails to open. ---*/
    if (ActDiskBem_file.fail()) SU2_MPI::Error("Unable to open Actuator Disk BEM Input File", CURRENT_FUNCTION);

    string text_line_appo;

    getline(ActDiskBem_file, text_line_appo);
    /*--- Read and assign the value of the number of propeller blades. ---*/
    getline(ActDiskBem_file, text_line_appo);
    istringstream NBlade_value(text_line_appo);
    NBlade_value >> ADBem_NBlade;

    /*--- Read and assign the value of the propeller diameter. ---*/
    getline(ActDiskBem_file, text_line_appo);
    istringstream Diameter_value(text_line_appo);
    Diameter_value >> ADBem_Diameter;

    /*--- Read and assign the value of the propeller hub radius. ---*/
    getline(ActDiskBem_file, text_line_appo);
    istringstream HubR_value(text_line_appo);
    HubR_value >> ADBem_HubRadius;

    /*--- Read and assign the value of the blade angle at 75% of R. ---*/
    getline(ActDiskBem_file, text_line_appo);
    istringstream Angle75R_value(text_line_appo);
    Angle75R_value >> ADBem_Angle75R;

    getline(ActDiskBem_file, text_line_appo);
    /*--- Read and assign the value of the number of radial propeller blade sections and alphas for each. ---*/
    getline(ActDiskBem_file, text_line_appo);
    istringstream NSection_NAlpha_value(text_line_appo);
    NSection_NAlpha_value >> ADBem_NSection >> ADBem_NAlpha;

    /*--- Assign the vector dimensions. ---*/
    i_v.resize(ADBem_NSection, 0.0);
    radius_v.resize(ADBem_NSection, 0.0);
    chord_v.resize(ADBem_NSection, 0.0);
    angle75r_v.resize(ADBem_NSection, 0.0);

    getline(ActDiskBem_file, text_line_appo);
    /*--- Read and assign the values of the propeller blade section's id, radius, chord, angle75R. ---*/
    for (int iSection = 0; iSection < ADBem_NSection; iSection++) {
      getline(ActDiskBem_file, text_line_appo);
      istringstream sectionStream(text_line_appo);
      sectionStream >> i_v[iSection] >> radius_v[iSection] >> chord_v[iSection] >> angle75r_v[iSection];
    }

    /*--- Assign the matrix dimensions. ---*/
    alpha_m.resize(ADBem_NAlpha, std::vector<su2double>(ADBem_NSection, 0.0));
    cl_m.resize(ADBem_NAlpha, std::vector<su2double>(ADBem_NSection, 0.0));
    cd_m.resize(ADBem_NAlpha, std::vector<su2double>(ADBem_NSection, 0.0));

    /*--- Read and assign the values for each of the propeller blade section's alpha, cl, cd. ---*/
    for (int iSection = 0; iSection < ADBem_NSection; iSection++) {
      getline(ActDiskBem_file, text_line_appo);
      for (int iAlpha = 0; iAlpha < ADBem_NAlpha; iAlpha++) {
        getline(ActDiskBem_file, text_line_appo);
        istringstream alphaStream(text_line_appo);
        alphaStream >> alpha_m[iAlpha][iSection] >> cl_m[iAlpha][iSection] >> cd_m[iAlpha][iSection];
      }
    }
  }

  /*--- Update the propeller load according to the modified flow field after every ADBem_Frequency inner iterations. ---*/
  if (InnerIter % ADBem_Frequency != 0) return;
  const su2double dia = ADBem_Diameter;
  const su2double r_tip = 0.5 * dia;
  su2double loc_Torque = 0.0;
  su2double loc_thrust = 0.0;
  su2double tot_area = 0.0, tot_tq = 0.0;

  su2double Normal[MAXNDIM];
  for (unsigned short iMarker = 0; iMarker < config->GetnMarker_All(); iMarker++) {
    if ((config->GetMarker_All_KindBC(iMarker) == ACTDISK_INLET) ||
        (config->GetMarker_All_KindBC(iMarker) == ACTDISK_OUTLET)) {
      for (unsigned long iVertex = 0; iVertex < geometry->nVertex[iMarker]; iVertex++) {
        const unsigned long iPoint = geometry->vertex[iMarker][iVertex]->GetNode();

        /*--- Read Swirl params. ---*/
        const su2double Omega_RPM = ADBem_Omega;
        const su2double omega_ref = config->GetOmega_Ref();
        const su2double Lref = config->GetLength_Ref();
        const su2double Omega_sw = Omega_RPM * (PI_NUMBER / 30.0) / (omega_ref);  // Swirl rate

        /*--- Center of the rotor ---*/
        su2double Origin[3] = {0.0, 0.0, 0.0};
        for (unsigned short iDim = 0; iDim < nDim; iDim++) {
          Origin[iDim] = ADBem_CG[iDim] / Lref;
        }

        /*--- Compute the distance to the center of the rotor ---*/
        geometry->vertex[iMarker][iVertex]->GetNormal(Normal);
        for (unsigned short iDim = 0; iDim < nDim; iDim++) {
          Normal[iDim] = -Normal[iDim];
        }

        /*--- Get propeller axis from config file. ---*/
        for (unsigned short iDim = 0; iDim < nDim; iDim++){
          ActDisk_Axis(iMarker, iDim) = ADBem_Axis[iDim];
        }
        su2double Area = 0.0;
        for (unsigned short iDim = 0; iDim < nDim; iDim++) {
          Area += Normal[iDim] * Normal[iDim];
        }
        Area = sqrt(Area);

        su2double UnitNormal[3] = {0.0, 0.0, 0.0};
        for (unsigned short iDim = 0; iDim < nDim; iDim++) {
          UnitNormal[iDim] = Normal[iDim] / Area;
        }

        const su2double* Coord = geometry->nodes->GetCoord(iPoint);

        su2double radius = 0.0;
        su2double radius_[3] = {0.0, 0.0, 0.0};
        for (unsigned short iDim = 0; iDim < nDim; iDim++) {
          radius += (Coord[iDim] - Origin[iDim]) * (Coord[iDim] - Origin[iDim]);
          radius_[iDim] = (Coord[iDim] - Origin[iDim]);
        }
        radius = sqrt(radius);

        /*--- Current solution at this boundary node and jumps values ---*/
        const su2double* V_domain = nodes->GetPrimitive(iPoint);

        if (abs(Omega_sw) > 1.0e-1) {
          su2double Vn = 0.0;
          for (unsigned short iDim = 0; iDim < nDim; iDim++) {
            Vn += V_domain[iDim + 1] * UnitNormal[iDim];
          }

          const su2double RPM = abs(Omega_RPM);
          const su2double rps = RPM / 60.0;
          ADBem_J = Vel_FreeStream[0] / (rps * dia);
          const su2double rho = V_domain[nDim + 2];
          const su2double T = V_domain[0];
          const su2double blade_angle = config->GetBEM_blade_angle();
          const su2double V = fabs(Vn);

          /*--- BEM model without parameter 'a' (ref?) ---*/
          su2double Torque = 0.0, dp_av = 0.0, dp_at_r = 0.0;
          {
            std::vector<su2double> DtDr(ADBem_NSection, 0.0);

            int s_prop_nblades = ADBem_NBlade;
            int sprop_sec_nalf = ADBem_NAlpha;
            int sprop_sec_nrad = ADBem_NSection;
            std::vector<su2double>& sprop_sec_r1 = radius_v;
            std::vector<su2double>& sprop_sec_chord = chord_v;
            std::vector<su2double>& sprop_sec_setangle = angle75r_v;
            std::vector<std::vector<su2double> >& sprop_sec_alf = alpha_m;
            std::vector<std::vector<su2double> >& sprop_sec_cl_arr = cl_m;
            std::vector<std::vector<su2double> >& sprop_sec_cd_arr = cd_m;

            su2double rad_p = radius;

            int j, isec, converged, n_iter;
            int NR = sprop_sec_nrad;
            su2double r_hub, alpha_corr, cl_corr_fac;
            su2double base_mach, s_mach, b_num, thrust, torque;
            su2double r_dash, t_loss, c_phi, rad, phi, alpha, radtodeg;
            su2double bnew, V0, V2, cl = 0.0, cd = 0.0, Vlocal, DqDr = 0.0, tem1, tem2, q;
            std::vector<su2double> delta_r(ADBem_NSection, 0.0);
            std::vector<su2double> b(ADBem_NSection, 0.0);
            static std::vector<su2double> Dtorq(ADBem_NSection, 0.0);

            su2double n, omega, a0, den;
            su2double ang_offset = 0.0;

            radtodeg = 180.0 / PI_NUMBER;
            r_hub = ADBem_HubRadius;
            ang_offset = blade_angle - ADBem_Angle75R;

            alpha_corr = 0.0;
            base_mach = 0.22;
            b_num = sqrt(1.0 - base_mach * base_mach);
            a0 = sqrt(1.4 * 287 * T);
            /*--- Change pitch by ang_offset and calculate delta_r for integration by trapezoidal rule. ---*/
            n = RPM / 60.0;
            omega = n * 2.0 * PI_NUMBER;

            for (j = 0; j < NR; j++) {
              if (j < 1) {
                delta_r[j] = sprop_sec_r1[j + 1] - r_hub;
              } else {
                if (j < NR - 1) {
                  delta_r[j] = sprop_sec_r1[j + 1] - sprop_sec_r1[j - 1];
                } else {
                  delta_r[j] = r_tip - sprop_sec_r1[j - 1];
                }
              }
              delta_r[j] *= 0.5;
            }

            thrust = 0.0;
            torque = 0.0;

            for (j = 0; j < NR; j++) {
              b[j] = 0.01;
              converged = 0;
              n_iter = 1;
              while (converged == 0) {
                V2 = omega * sprop_sec_r1[j] * (1 - b[j]);
                V0 = V;

                phi = atan2(V0, V2);

                alpha = sprop_sec_setangle[j] + ang_offset - radtodeg * phi + alpha_corr;
                rad = sprop_sec_r1[j];

                /*--- get cl, cd from lookup table. ---*/
                isec = j + 1;
                {
                  int i, salf = 0;
                  su2double fact;

                  /*--- interpolating values of cl and cd for given alpha. ---*/
                  if (alpha >= sprop_sec_alf[salf][isec - 1] &&
                      alpha <= sprop_sec_alf[sprop_sec_nalf - 1][isec - 1]) {
                    for (i = 0; i < sprop_sec_nalf - 1; i++) {
                      if (alpha >= sprop_sec_alf[i][isec - 1] && alpha <= sprop_sec_alf[i + 1][isec - 1]) {
                        fact = (alpha - sprop_sec_alf[i][isec - 1])
                             / (sprop_sec_alf[i + 1][isec - 1] - sprop_sec_alf[i][isec - 1]);
                        cl = sprop_sec_cl_arr[i][isec - 1]
                           + fact * (sprop_sec_cl_arr[i + 1][isec - 1] - sprop_sec_cl_arr[i][isec - 1]);
                        cd = sprop_sec_cd_arr[i][isec - 1]
                           + fact * (sprop_sec_cd_arr[i + 1][isec - 1] - sprop_sec_cd_arr[i][isec - 1]);
                      }
                    }
                  } else {
                    if (alpha < sprop_sec_alf[salf][isec - 1]) {
                      cl = sprop_sec_cl_arr[0][isec - 1];
                      cd = sprop_sec_cd_arr[0][isec - 1];
                    }
                    if (alpha > sprop_sec_alf[sprop_sec_nalf - 1][isec - 1]) {
                      cl = sprop_sec_cl_arr[sprop_sec_nalf - 1][isec - 1];
                      cd = sprop_sec_cd_arr[sprop_sec_nalf - 1][isec - 1];
                    }
                  }
                }

                Vlocal = sqrt(V0 * V0 + V2 * V2);
                q = 0.5 * rho * Vlocal * Vlocal;
                s_mach = Vlocal / a0;
                cl_corr_fac = 1.0;
                if (s_mach > base_mach) {
                  den = 1.0 - s_mach * s_mach;
                  if (den > 0.0) cl_corr_fac = b_num / sqrt(den);
                }
                cl *= cl_corr_fac;

                /*--- tip loss factor. ---*/
                r_dash = rad / r_tip + 1.0e-5;
                c_phi = cos(phi);
                t_loss = 1.0;
                if (r_dash > 0.90) {
                  t_loss = (2.0 / PI_NUMBER) * acos(exp(-(1.0 * s_prop_nblades * (1 - r_dash) / (r_dash * c_phi))));
                }

                DtDr[j] = q * s_prop_nblades * sprop_sec_chord[j] * (cl * cos(phi) - cd * sin(phi));
                DqDr = q * s_prop_nblades * sprop_sec_chord[j] * rad * (cd * cos(phi) + cl * sin(phi));

                DtDr[j] *= t_loss;
                DqDr *= t_loss;

                tem2 = DqDr / (4.0 * PI_NUMBER * rad * rad * rad * rho * V * omega);
                bnew = 0.6 * b[j] + 0.4 * tem2;
                if (bnew > 0.9) bnew = 0.9;
                if (fabs(bnew - b[j]) < 1.0e-5) {
                  converged = 1;
                }
                if (bnew < 0.1) {
                  b[j] = bnew;
                }
                n_iter++;
                if (n_iter > BEM_MAX_ITER) {
                  converged = 1;
                }
              }
              thrust = thrust + DtDr[j] * delta_r[j];
              torque = torque + DqDr * delta_r[j];
              Dtorq[j] = DqDr;
            }

            tem1 = rho * n * n * dia * dia * dia * dia;
            tem2 = tem1 * dia;

            Torque = 2.0 * PI_NUMBER * torque;
            dp_av = 2.0 * PI_NUMBER * torque;

            for (j = 0; j < NR; j++) {
              DtDr[j] /= (2.0 * PI_NUMBER * sprop_sec_r1[j]);
              Dtorq[j] = Dtorq[j];
            }

            if (rad_p < sprop_sec_r1[0]) {
              tem2 = sprop_sec_r1[0] - r_hub;
              tem1 = (rad_p - r_hub) / tem2;
              tem2 = 1.0 - tem1;
              dp_at_r = DtDr[0] * tem1;
              Torque = Dtorq[0] * tem1;
            } else {
              if (rad_p > r_tip) {
                dp_at_r = 0.0;
                Torque = 0.0;
              } else {
                if (rad_p > sprop_sec_r1[NR - 1]) {
                  tem2 = r_tip - sprop_sec_r1[NR - 1];
                  tem1 = (rad_p - sprop_sec_r1[NR - 1]) / tem2;
                  tem2 = 1.0 - tem1;
                  dp_at_r = DtDr[NR - 1] * tem2;
                  Torque = Dtorq[NR - 1] * tem2;
                } else {
                  for (j = 0; j < NR - 1; j++) {
                    if ((sprop_sec_r1[j] < rad_p) && (sprop_sec_r1[j + 1] >= rad_p)) {
                      tem2 = sprop_sec_r1[j + 1] - sprop_sec_r1[j];
                      tem1 = (rad_p - sprop_sec_r1[j]) / tem2;
                      tem2 = 1.0 - tem1;
                      dp_at_r = DtDr[j] * tem2 + DtDr[j + 1] * tem1;
                      Torque = Dtorq[j] * tem2 + Dtorq[j + 1] * tem1;
                    }
                  }
                }
              }
            }
          }

          tot_area += Area;
          loc_Torque += Torque * Area;
          tot_tq += dp_av;
          loc_thrust += dp_at_r * Area;
          const su2double Target_Press_Jump = dp_at_r;

          ActDisk_DeltaP_r[iMarker][iVertex] = Target_Press_Jump;
          ActDisk_Thrust_r[iMarker][iVertex] = dp_at_r;
          ActDisk_Torque_r[iMarker][iVertex] = Torque / (2 * PI_NUMBER * radius);
          /*--- Non-dimensionalize the elemental load. ---*/
          const su2double dCp_v = Torque * ((Omega_sw * r_tip) / (rho * rps * rps * rps * pow(dia, 5)));
          /*--- Force radial load to 0 as there is no information of radial load from BEM. ---*/
          const su2double dCr_v = 0.0;
          const su2double rad_v = radius / r_tip;
          const su2double Fa = dp_at_r;
          const su2double Ft = (dCp_v * (2 * Dens_FreeStream * pow(Vel_FreeStream[0], 2)) /
                               ((ADBem_J * PI_NUMBER * rad_v) * (ADBem_J * PI_NUMBER * rad_v))) /
                               config->GetPressure_Ref();
          const su2double Fr = (dCr_v * (2 * Dens_FreeStream * pow(Vel_FreeStream[0], 2)) /
                               (pow(ADBem_J, 2) * PI_NUMBER * rad_v)) / config->GetPressure_Ref();
          const su2double Fx = (Ft + Fr) * (radius_[0] / (radius));
          const su2double Fy = (Ft + Fr) * (radius_[2] / (radius));
          const su2double Fz = -(Ft + Fr) * (radius_[1] / (radius));
          ActDisk_Fa_BEM[iMarker][iVertex] = Fa;
          ActDisk_Fx_BEM[iMarker][iVertex] = Fx;
          ActDisk_Fy_BEM[iMarker][iVertex] = Fy;
          ActDisk_Fz_BEM[iMarker][iVertex] = Fz;
        }
      }
    }
  }
}

void CEulerSolver::SetFarfield_AoA(CGeometry *geometry, CSolver **solver_container,
                                   CConfig *config, unsigned short iMesh, bool Output) {
  SU2_ZONE_SCOPED

  const auto InnerIter = config->GetInnerIter();
  /* --- Initialize values at first iteration --- */

  if (InnerIter == 0) {
    Total_CD_Prev = 0.0;
    Total_CL_Prev = 0.0;
    Total_CMx_Prev = 0.0;
    Total_CMy_Prev = 0.0;
    Total_CMz_Prev = 0.0;
    AoA_Prev = config->GetAoA();
    dCL_dAlpha = config->GetdCL_dAlpha();
    AoA_inc = 0.0;
  }

  /*--- Retrieve the AoA (degrees) ---*/

  su2double AoA = config->GetAoA();

  /* --- Set new AoA if needed --- */

  if (fabs(AoA_inc) > 0.0 && Output) {

    /* --- Update *_Prev values with current coefficients --- */

    SetCoefficient_Gradients(config);

    Total_CD_Prev = TotalCoeff.CD;
    Total_CL_Prev = TotalCoeff.CL;
    Total_CMx_Prev = TotalCoeff.CMx;
    Total_CMy_Prev = TotalCoeff.CMy;
    Total_CMz_Prev = TotalCoeff.CMz;
    AoA_Prev = AoA;

    /*--- Compute a new value for AoA on the fine mesh only (degrees)---*/

    if (iMesh == MESH_0) {
      AoA = AoA + AoA_inc;
      config->SetAoA(AoA);
    }
    UpdateFarfieldVelocity(config);
  }
}

bool CEulerSolver::FixedCL_Convergence(CConfig* config, bool convergence) {
  SU2_ZONE_SCOPED
  const su2double Target_CL = config->GetTarget_CL();
  const auto curr_iter = config->GetInnerIter();
  const auto Iter_dCL_dAlpha = config->GetIter_dCL_dAlpha();
  bool fixed_cl_conv = false;
  AoA_inc = 0.0;

  /*--- if in Fixed CL mode, before finite differencing --- */

  if (!Start_AoA_FD){
    if (convergence){

      /* --- C_L and solution are converged, start finite differencing --- */

      if (fabs(TotalCoeff.CL-Target_CL) < (config->GetCauchy_Eps()/2)) {

        /* --- If no finite differencing required --- */

        if (Iter_dCL_dAlpha == 0){
          fixed_cl_conv = true;
          return fixed_cl_conv;
        }

        /* --- Else, set up finite differencing routine ---*/

        Iter_Update_AoA = curr_iter;
        Start_AoA_FD = true;
        fixed_cl_conv = false;
        AoA_inc = 0.001;
      }

      /* --- C_L is not converged to target value and some iterations
          have passed since last update, so update AoA --- */

      else if ((curr_iter - Iter_Update_AoA) > config->GetStartConv_Iter()){
        Iter_Update_AoA = curr_iter;
        fixed_cl_conv = false;
        if (fabs(TotalCoeff.CL-Target_CL) > (config->GetCauchy_Eps()/2)) {
          AoA_inc = (1.0/dCL_dAlpha)*(Target_CL - TotalCoeff.CL);
        }
      }
    }

    /* --- If the iteration limit between AoA updates is met, so update AoA --- */

    else if ((curr_iter - Iter_Update_AoA) == config->GetUpdate_AoA_Iter_Limit()) {
      Iter_Update_AoA = curr_iter;
      fixed_cl_conv = false;
      if (fabs(TotalCoeff.CL-Target_CL) > (config->GetCauchy_Eps()/2)) {
        AoA_inc = (1.0/dCL_dAlpha)*(Target_CL - TotalCoeff.CL);
      }
    }

    /* --- If the total iteration limit is reached, start finite differencing --- */

    if (curr_iter == config->GetnInner_Iter() - Iter_dCL_dAlpha){
      if (Iter_dCL_dAlpha == 0){
        End_AoA_FD = true;
      }
      Iter_Update_AoA = curr_iter;
      Start_AoA_FD = true;
      fixed_cl_conv = false;
      AoA_inc = 0.001;
    }
  }

  /* --- If Finite Difference Mode has ended, end simulation --- */

  if (End_AoA_FD){
    //fixed_cl_conv = true;
    return true;
  }

  /* --- If starting Finite Difference Mode --- */

  if (Start_AoA_FD){

    /* --- Disable history writing --- */

    config->SetHistory_Wrt_Freq(2, 0);

    /* --- End Finite Difference Mode if iteration limit is reached, so simualtion is converged --- */

    End_AoA_FD = ((curr_iter - Iter_Update_AoA - 2) == Iter_dCL_dAlpha ||
      curr_iter == config->GetnInner_Iter()- 2 );

    if (convergence && (curr_iter - Iter_Update_AoA) > config->GetStartConv_Iter())
      End_AoA_FD = true;

    /* --- If Finite Difference mode is ending, reset AoA and calculate Coefficient Gradients --- */

    if (End_AoA_FD){
      SetCoefficient_Gradients(config);
      config->SetAoA(AoA_Prev);
    }
  }

  return fixed_cl_conv;

}

void CEulerSolver::SetCoefficient_Gradients(CConfig *config) const{
  SU2_ZONE_SCOPED

  const su2double AoA = config->GetAoA();

  if (AoA == AoA_Prev) return;

  /*--- Calculate gradients of coefficients w.r.t. CL ---*/

  const su2double dCL = TotalCoeff.CL - Total_CL_Prev;
  const su2double dCL_dAlpha_ = dCL / (AoA - AoA_Prev);
  const su2double dCD_dCL_ = (TotalCoeff.CD-Total_CD_Prev) / dCL;
  const su2double dCMx_dCL_ = (TotalCoeff.CMx-Total_CMx_Prev) / dCL;
  const su2double dCMy_dCL_ = (TotalCoeff.CMy-Total_CMy_Prev) / dCL;
  const su2double dCMz_dCL_ = (TotalCoeff.CMz-Total_CMz_Prev) / dCL;

  /*--- Set the value of the  dOF/dCL in the config file ---*/

  config->SetdCD_dCL(dCD_dCL_);
  config->SetdCMx_dCL(dCMx_dCL_);
  config->SetdCMy_dCL(dCMy_dCL_);
  config->SetdCMz_dCL(dCMz_dCL_);
  config->SetdCL_dAlpha(dCL_dAlpha_);
}

void CEulerSolver::Evaluate_ObjFunc(const CConfig *config, CSolver**) {
  SU2_ZONE_SCOPED

  unsigned short iMarker_Monitoring, Kind_ObjFunc;
  su2double Weight_ObjFunc;

  Total_ComboObj = EvaluateCommonObjFunc(*config);

  /*--- Loop over all monitored markers, add to the 'combo' objective ---*/

  if (config->GetFixed_CL_Mode()) {
    for (iMarker_Monitoring = 0; iMarker_Monitoring < config->GetnMarker_Monitoring(); iMarker_Monitoring++) {

      Weight_ObjFunc = config->GetWeight_ObjFunc(iMarker_Monitoring);
      Kind_ObjFunc = config->GetKind_ObjFunc(iMarker_Monitoring);

      switch(Kind_ObjFunc) {
        case DRAG_COEFFICIENT:
          Total_ComboObj -= Weight_ObjFunc*config->GetdCD_dCL()*(SurfaceCoeff.CL[iMarker_Monitoring]);
          break;
        case MOMENT_X_COEFFICIENT:
          Total_ComboObj -= Weight_ObjFunc*config->GetdCMx_dCL()*(SurfaceCoeff.CL[iMarker_Monitoring]);
          break;
        case MOMENT_Y_COEFFICIENT:
          Total_ComboObj -= Weight_ObjFunc*config->GetdCMy_dCL()*(SurfaceCoeff.CL[iMarker_Monitoring]);
          break;
        case MOMENT_Z_COEFFICIENT:
          Total_ComboObj -= Weight_ObjFunc*config->GetdCMz_dCL()*(SurfaceCoeff.CL[iMarker_Monitoring]);
          break;
        default:
          break;
      }
    }
  }

  /*--- The following are not per-surface, and so to avoid that they are
   double-counted when multiple surfaces are specified, they have been
   placed outside of the loop above. In addition, multi-objective mode is
   also disabled for these objective functions (error thrown at start). ---*/

  Weight_ObjFunc = config->GetWeight_ObjFunc(0);
  Kind_ObjFunc   = config->GetKind_ObjFunc(0);

  /*--- Turbomachinery specific objective functions ---*/
  if (TurbomachineryPerformance && CTurboOutput::IsTurboObjective(Kind_ObjFunc))
    Total_ComboObj += Weight_ObjFunc * TurbomachineryPerformance->GetObjectiveValue(Kind_ObjFunc);

  switch(Kind_ObjFunc) {
    case NEARFIELD_PRESSURE:
      Total_ComboObj+=Weight_ObjFunc*Total_CNearFieldOF;
      break;
    case SURFACE_MACH:
      Total_ComboObj+=Weight_ObjFunc*config->GetSurface_Mach(0);
      break;
    default:
      break;
  }
}

void CEulerSolver::BC_Far_Field(CGeometry *geometry, CSolver **solver_container, CNumerics *conv_numerics,
                                CNumerics *visc_numerics, CConfig *config, unsigned short val_marker) {
  SU2_ZONE_SCOPED

  unsigned short iDim;
  unsigned long iVertex, iPoint, Point_Normal;

  su2double *GridVel;
  su2double Area, UnitNormal[MAXNDIM] = {0.0};
  su2double Density, Pressure, Energy,  Velocity[MAXNDIM] = {0.0};
  su2double Density_Bound, Pressure_Bound, Energy_Bound, Vel_Bound[MAXNDIM] = {0.0};
  su2double Density_Infty, Pressure_Infty, Vel_Infty[MAXNDIM] = {0.0};
  su2double SoundSpeed, Entropy, Velocity2, Vn;
  su2double SoundSpeed_Bound, Entropy_Bound, Vel2_Bound, Vn_Bound;
  su2double SoundSpeed_Infty, Entropy_Infty, Vel2_Infty, Vn_Infty, Qn_Infty;
  su2double RiemannPlus, RiemannMinus;
  su2double *V_infty, *V_domain;

  su2double Gas_Constant     = config->GetGas_ConstantND();

  bool implicit       = config->GetKind_TimeIntScheme() == EULER_IMPLICIT;
  bool viscous        = config->GetViscous();
  bool tkeNeeded = (config->GetKind_Turb_Model() == TURB_MODEL::SST);
  CVariable* turbNodes = nullptr;
  if (tkeNeeded) turbNodes = solver_container[TURB_SOL]->GetNodes();

  /*--- SPLIT_EDGE_FLUX with SST (matched k): hat flux and the closure C k rule, k_inf on inflow (JSTFIN_PLAN.md 5). ---*/
  const bool splitMatchedK = config->GetSplit_Edge_Flux() && config->GetTke_Matched_P1();
  const su2double kInfMatched = splitMatchedK ? solver_container[TURB_SOL]->GetTke_Inf() : 0.0;

  auto *Normal = new su2double[nDim];

  /*--- Loop over all the vertices on this boundary marker ---*/

  SU2_OMP_FOR_DYN(OMP_MIN_SIZE)
  for (iVertex = 0; iVertex < geometry->nVertex[val_marker]; iVertex++) {
    iPoint = geometry->vertex[val_marker][iVertex]->GetNode();

    su2double dUInfdUBound[5][5] = {0.0}, totJac[5][5] = {0.0};

    /*--- Allocate the value at the infinity ---*/
    V_infty = GetCharacPrimVar(val_marker, iVertex);

    for(int iVar = 0; iVar < nVar; ++iVar)
      for(int jVar = 0; iVar < nVar; ++iVar) {
        dUInfdUBound[iVar][jVar] = 0;
        totJac[iVar][jVar] = 0;
      }

    /*--- Check if the node belongs to the domain (i.e, not a halo node) ---*/

    if (geometry->nodes->GetDomain(iPoint)) {

      /*--- Index of the closest interior node ---*/

      Point_Normal = geometry->vertex[val_marker][iVertex]->GetNormal_Neighbor();

      /*--- Normal vector for this vertex (negate for outward convention) ---*/

      geometry->vertex[val_marker][iVertex]->GetNormal(Normal);
      for (iDim = 0; iDim < nDim; iDim++) Normal[iDim] = -Normal[iDim];
      conv_numerics->SetNormal(Normal);

      /*--- Retrieve solution at the farfield boundary node ---*/
      V_domain = nodes->GetPrimitive(iPoint);

      /*--- Construct solution state at infinity for compressible flow by
         using Riemann invariants, and then impose a weak boundary condition
         by computing the flux using this new state for U. See CFD texts by
         Hirsch or Blazek for more detail. Adapted from an original
         implementation in the Stanford University multi-block (SUmb) solver
         in the routine bcFarfield.f90 written by Edwin van der Weide,
         last modified 06-12-2005. First, compute the unit normal at the
         boundary nodes. ---*/

      Area = GeometryToolbox::Norm(nDim, Normal);
      for (iDim = 0; iDim < nDim; iDim++)
        UnitNormal[iDim] = Normal[iDim]/Area;

      /*--- Store primitive variables (density, velocities, velocity squared,
         energy, pressure, and sound speed) at the boundary node, and set some
         other quantities for clarity. Project the current flow velocity vector
         at this boundary node into the local normal direction, i.e. compute
         v_bound.n.  ---*/

      Density_Bound = V_domain[nDim+2];
      Vel2_Bound = 0.0; Vn_Bound = 0.0;
      for (iDim = 0; iDim < nDim; iDim++) {
        Vel_Bound[iDim] = V_domain[iDim+1];
        Vel2_Bound     += Vel_Bound[iDim]*Vel_Bound[iDim];
        Vn_Bound       += Vel_Bound[iDim]*UnitNormal[iDim];
      }
      Pressure_Bound   = nodes->GetPressure(iPoint);
      SoundSpeed_Bound = sqrt(Gamma*Pressure_Bound/Density_Bound);
      Entropy_Bound    = pow(Density_Bound, Gamma)/Pressure_Bound;
      Energy_Bound = nodes->GetEnergy(iPoint);
      /*--- Matched k: the ghost Jacobian on the hat state (k frozen). ---*/
      if (splitMatchedK) Energy_Bound -= turbNodes->GetSolution(iPoint, 0);

      /*--- Store the primitive variable state for the freestream. Project
         the freestream velocity vector into the local normal direction,
         i.e. compute v_infty.n. ---*/

      Density_Infty = GetDensity_Inf();
      Vel2_Infty = 0.0; Vn_Infty = 0.0;
      for (iDim = 0; iDim < nDim; iDim++) {
        Vel_Infty[iDim] = GetVelocity_Inf(iDim);
        Vel2_Infty     += Vel_Infty[iDim]*Vel_Infty[iDim];
        Vn_Infty       += Vel_Infty[iDim]*UnitNormal[iDim];
      }
      Pressure_Infty   = GetPressure_Inf();
      SoundSpeed_Infty = sqrt(Gamma*Pressure_Infty/Density_Infty);
      Entropy_Infty    = pow(Density_Infty, Gamma)/Pressure_Infty;

      /*--- Adjust the normal freestream velocity for grid movement ---*/

      Qn_Infty = Vn_Infty;
      if (dynamic_grid) {
        GridVel = geometry->nodes->GetGridVel(iPoint);
        for (iDim = 0; iDim < nDim; iDim++)
          Qn_Infty -= GridVel[iDim]*UnitNormal[iDim];
      }

      /*--- Compute acoustic Riemann invariants: R = u.n +/- 2c/(gamma-1).
         These correspond with the eigenvalues (u+c) and (u-c), respectively,
         which represent the acoustic waves. Positive characteristics are
         incoming, and a physical boundary condition is imposed (freestream
         state). This occurs when either (u.n+c) > 0 or (u.n-c) > 0. Negative
         characteristics are leaving the domain, and numerical boundary
         conditions are required by extrapolating from the interior state
         using the Riemann invariants. This occurs when (u.n+c) < 0 or
         (u.n-c) < 0. Note that grid movement is taken into account when
         checking the sign of the eigenvalue. ---*/

      /*--- Check whether (u.n+c) is greater or less than zero ---*/

      if (Qn_Infty > -SoundSpeed_Infty) {
        /*--- Subsonic inflow or outflow ---*/
        RiemannPlus = Vn_Bound + 2.0*SoundSpeed_Bound/Gamma_Minus_One;
      } else {
        /*--- Supersonic inflow ---*/
        RiemannPlus = Vn_Infty + 2.0*SoundSpeed_Infty/Gamma_Minus_One;
      }

      /*--- Check whether (u.n-c) is greater or less than zero ---*/

      if (Qn_Infty > SoundSpeed_Infty) {
        /*--- Supersonic outflow ---*/
        RiemannMinus = Vn_Bound - 2.0*SoundSpeed_Bound/Gamma_Minus_One;
      } else {
        /*--- Subsonic outflow ---*/
        RiemannMinus = Vn_Infty - 2.0*SoundSpeed_Infty/Gamma_Minus_One;
      }

      /*--- Compute a new value for the local normal velocity and speed of
         sound from the Riemann invariants. ---*/

      Vn = 0.5 * (RiemannPlus + RiemannMinus);
      SoundSpeed = 0.25 * (RiemannPlus - RiemannMinus)*Gamma_Minus_One;

      /*--- Construct the primitive variable state at the boundary for
         computing the flux for the weak boundary condition. The values
         that we choose to construct the solution (boundary or freestream)
         depend on whether we are at an inflow or outflow. At an outflow, we
         choose boundary information (at most one characteristic is incoming),
         while at an inflow, we choose infinity values (at most one
         characteristic is outgoing). ---*/

      if (Qn_Infty > 0.0)   {
        /*--- Outflow conditions ---*/
        for (iDim = 0; iDim < nDim; iDim++)
          Velocity[iDim] = Vel_Bound[iDim] + (Vn-Vn_Bound)*UnitNormal[iDim];
        Entropy = Entropy_Bound;
      } else  {
        /*--- Inflow conditions ---*/
        for (iDim = 0; iDim < nDim; iDim++)
          Velocity[iDim] = Vel_Infty[iDim] + (Vn-Vn_Infty)*UnitNormal[iDim];
        Entropy = Entropy_Infty;
      }

      /*--- Recompute the primitive variables. ---*/

      Density = pow(Entropy*SoundSpeed*SoundSpeed/Gamma,1.0/Gamma_Minus_One);
      Velocity2 = 0.0;
      for (iDim = 0; iDim < nDim; iDim++) {
        Velocity2 += Velocity[iDim]*Velocity[iDim];
      }
      Pressure = Density*SoundSpeed*SoundSpeed/Gamma;
      Energy   = Pressure/(Gamma_Minus_One*Density) + 0.5*Velocity2;
      /*--- k of the boundary node, not the free-stream value, which decays right away in the domain (#1851). ---*/
      if (tkeNeeded) Energy += turbNodes->GetSolution(iPoint,0);

      /*--- Store new primitive state for computing the flux. ---*/

      V_infty[0] = Pressure/(Gas_Constant*Density);
      for (iDim = 0; iDim < nDim; iDim++)
        V_infty[iDim+1] = Velocity[iDim];
      V_infty[nDim+1] = Pressure;
      V_infty[nDim+2] = Density;
      V_infty[nDim+3] = Energy + Pressure/Density;


      if (implicit) {
        su2double dRpdP[5], dRmdP[5], dSdP[5], dVbdP[MAXNDIM][5];

        for (int iVar = 0; iVar < nVar; ++iVar) {
          dRpdP[iVar] = 0;
          dRmdP[iVar] = 0;
          dSdP[iVar] = 0;
        }

        for (iDim = 0; iDim < MAXNDIM; ++iDim)
          for (int iVar = 0; iVar < nVar; ++iVar)
            dVbdP[iDim][iVar] = 0;

        if (Qn_Infty > -SoundSpeed_Infty){
          dRpdP[0] = 2.0/(Gamma_Minus_One);
          for (iDim = 0; iDim < nDim; ++iDim)
            dRpdP[iDim+1] = UnitNormal[iDim];
        }

        if (Qn_Infty > SoundSpeed_Infty){
          dRmdP[0] = -2.0/(Gamma_Minus_One);
          for (iDim = 0; iDim < nDim; ++iDim)
            dRmdP[iDim+1] = UnitNormal[iDim];

        }

        if (Qn_Infty > 0) dSdP[nDim+1] = 1;

        for (iDim = 0; iDim < nDim; ++iDim)
          for (int jVar = 0; jVar < nVar; ++jVar)
            dVbdP[iDim][jVar] = 0.5 * UnitNormal[iDim] * (dRpdP[jVar] + dRmdP[jVar]);

        if (Qn_Infty > 0) {
          for (iDim = 0; iDim < nDim; ++iDim) {
            for (int jDim = 0; jDim < nDim; ++jDim)
              dVbdP[iDim][jDim+1] -= UnitNormal[iDim] * UnitNormal[jDim];
            dVbdP[iDim][iDim+1] += 1;
          }
        }

        su2double dUedVe[5][5];

        dUedVe[0][0] = 1;
        for (int iVar = 1; iVar < nVar; ++iVar) dUedVe[0][iVar] = 0;

        for (iDim = 0; iDim < nDim; ++iDim){
          dUedVe[iDim+1][0] = Velocity[iDim];
          for (int jDim = 0; jDim < nDim; ++jDim){
            dUedVe[iDim+1][jDim+1] = 0;
          }
          dUedVe[iDim+1][iDim+1] = Density;
          dUedVe[iDim+1][nDim+1] = 0;
        }
        dUedVe[nDim+1][0] = 0.5*Velocity2;
        dUedVe[nDim+1][nDim+1] = 1/Gamma_Minus_One;
        for (iDim = 0; iDim < nDim; ++iDim)
          dUedVe[nDim+1][iDim+1] = Density*Velocity[iDim];


        su2double dVedPb[5][5];
        const su2double factor = pow(Entropy*SoundSpeed*SoundSpeed/Gamma,(2-Gamma)/Gamma_Minus_One);

        dVedPb[0][0] = factor * 2*Entropy*SoundSpeed/(Gamma*Gamma_Minus_One);
        dVedPb[0][nDim+1] = factor * SoundSpeed*SoundSpeed/(Gamma*Gamma_Minus_One);
        for (iDim = 0; iDim < nDim; ++iDim) dVedPb[0][iDim+1] = 0;

        for (iDim = 0; iDim < nDim; ++iDim){
          dVedPb[iDim+1][0] = 0;
          for (int jDim = 0; jDim < nDim; ++jDim){
            dVedPb[iDim+1][jDim+1] = 0;
          }
          dVedPb[iDim+1][iDim+1] = 1;
          dVedPb[iDim+1][nDim+1] = 0;
        }

        dVedPb[nDim+1][0] = factor * 2*Entropy*pow(SoundSpeed,3)/(Gamma*Gamma*Gamma_Minus_One) + 2*SoundSpeed*Density/Gamma;
        dVedPb[nDim+1][nDim+1] = pow(SoundSpeed,4) * factor / (Gamma*Gamma*Gamma_Minus_One);

        for (iDim = 0; iDim < nDim; ++iDim)
          dVedPb[nDim+1][iDim+1] = 0;

        su2double dPdU[5][5];

        dPdU[0][0] = 0.5*Gamma*Gamma_Minus_One/SoundSpeed_Bound * (Vel2_Bound/Density_Bound - Energy_Bound/Density_Bound);
        for (iDim = 0; iDim < nDim; ++iDim) dPdU[0][iDim+1] = -0.5*Gamma*Gamma_Minus_One/SoundSpeed_Bound * Vel_Bound[iDim]/Density_Bound;
        dPdU[0][nDim+1] = 0.5*Gamma*Gamma_Minus_One/(Density_Bound*SoundSpeed_Bound);

        for (iDim = 0; iDim < nDim; ++iDim){
          dPdU[iDim+1][0] = -Vel_Bound[iDim] / Density_Bound;
          for (int jDim = 0; jDim < nDim; ++jDim){
            dPdU[iDim+1][jDim+1] = 0;
          }
          dPdU[iDim+1][iDim+1] = 1.0/Density_Bound;
          dPdU[iDim+1][nDim+1] = 0;
        }
        dPdU[nDim+1][0] = Gamma*pow(Density_Bound,Gamma_Minus_One)/Pressure_Bound - 0.5*Gamma_Minus_One*Entropy_Bound*Vel2_Bound/Pressure_Bound;
        dPdU[nDim+1][nDim+1] = -Gamma_Minus_One*Entropy_Bound/Pressure_Bound;
        for (iDim = 0; iDim < nDim; ++iDim)
          dPdU[nDim+1][iDim+1] = Gamma_Minus_One*Entropy_Bound*Vel_Bound[iDim]/Pressure_Bound;



        su2double dPbdP[5][5];

        for (int iVar = 0; iVar < nVar; ++iVar)
          dPbdP[0][iVar] = 0.25 * Gamma_Minus_One * (dRpdP[iVar] - dRmdP[iVar]);

        for (iDim = 0; iDim < nDim; ++iDim) {
          for (int jVar = 0; jVar < nVar; ++jVar) {
            dPbdP[iDim+1][jVar] = dVbdP[iDim][jVar];
          }
        }

        for (int iVar = 0; iVar < nVar; ++iVar)
          dPbdP[nDim+1][iVar] = dSdP[iVar];


        su2double temp1[5][5], temp2[5][5];

        for (int iVar = 0; iVar < nVar; ++iVar)
          for (int jVar = 0; jVar < nVar; ++jVar) {
            temp1[iVar][jVar] = 0;
            for (int kVar = 0; kVar < nVar; ++kVar)
              temp1[iVar][jVar] += dUedVe[iVar][kVar] * dVedPb[kVar][jVar];
          }

        for (int iVar = 0; iVar < nVar; ++iVar)
          for (int jVar = 0; jVar < nVar; ++jVar) {
            temp2[iVar][jVar] = 0;
            for (int kVar = 0; kVar < nVar; ++kVar)
              temp2[iVar][jVar] += temp1[iVar][kVar] * dPbdP[kVar][jVar];
          }

        for (int iVar = 0; iVar < nVar; ++iVar)
          for (int jVar = 0; jVar < nVar; ++jVar) {
            dUInfdUBound[iVar][jVar] = 0;
            for (int kVar = 0; kVar < nVar; ++kVar)
              dUInfdUBound[iVar][jVar] += temp2[iVar][kVar] * dPdU[kVar][jVar];
          }


      }

      /*--- Set various quantities in the numerics class ---*/

      su2double Vhat[2][MAXNVAR + 8];
      if (splitMatchedK) {
        SetBoundaryHatStates(conv_numerics, V_domain, V_infty, turbNodes->GetSolution(iPoint, 0), Vhat);
      } else {
      conv_numerics->SetPrimitive(V_domain, V_infty);

      /*--- SST: both states contain the k of the node in their total enthalpy. ---*/

      if (config->GetKind_Turb_Model() == TURB_MODEL::SST) {

        const su2double tke = solver_container[TURB_SOL]->GetNodes()->GetSolution(iPoint, 0);

        conv_numerics->SetTurbKineticEnergy(tke, tke);

      }
      }

      if (dynamic_grid) {
        conv_numerics->SetGridVel(geometry->nodes->GetGridVel(iPoint),
                                  geometry->nodes->GetGridVel(iPoint));
      }

      /*--- Compute the convective residual using an upwind scheme ---*/

      auto residual = conv_numerics->ComputeResidual(config);

      /*--- Convective Jacobian contribution for implicit integration ---*/

      if (implicit) {

        for (int iVar = 0; iVar < nVar; ++iVar)
          for (int jVar = 0; jVar < nVar; ++jVar) {
            totJac[iVar][jVar] = residual.jacobian_i[iVar][jVar];
            for (int kVar = 0; kVar < nVar; ++kVar)
              totJac[iVar][jVar] += residual.jacobian_j[iVar][kVar] * dUInfdUBound[kVar][jVar];
          }
      }

      /*--- Update residual value ---*/

      if (splitMatchedK) {
        su2double resK[MAXNVAR] = {0.0};
        for (int iVar = 0; iVar < nVar; ++iVar) resK[iVar] = residual[iVar];
        AddBoundaryMatchedKine(iPoint, turbNodes->GetSolution(iPoint, 0), kInfMatched, resK, totJac, implicit);
        LinSysRes.AddBlock(iPoint, resK);
      } else {
        LinSysRes.AddBlock(iPoint, residual);
      }

      if (implicit) Jacobian.AddBlock2Diag(iPoint, totJac);

      /*--- Viscous residual contribution ---*/

      if (viscous) {

        /*--- Set laminar and eddy viscosity at the infinity ---*/

        V_infty[nDim+5] = nodes->GetLaminarViscosity(iPoint);
        V_infty[nDim+6] = nodes->GetEddyViscosity(iPoint);

        /*--- Set the normal vector and the coordinates ---*/

        visc_numerics->SetNormal(Normal);
        su2double Coord_Reflected[MAXNDIM];
        GeometryToolbox::PointPointReflect(nDim, geometry->nodes->GetCoord(Point_Normal),
                                                 geometry->nodes->GetCoord(iPoint), Coord_Reflected);
        visc_numerics->SetCoord(geometry->nodes->GetCoord(iPoint), Coord_Reflected);

        /*--- Primitive variables, and gradient ---*/

        visc_numerics->SetPrimitive(V_domain, V_infty);
        visc_numerics->SetPrimVarGradient(nodes->GetGradient_Primitive(iPoint),
                                          nodes->GetGradient_Primitive(iPoint));

        /*--- Turbulent kinetic energy ---*/

        if (config->GetKind_Turb_Model() == TURB_MODEL::SST)
          visc_numerics->SetTurbKineticEnergy(solver_container[TURB_SOL]->GetNodes()->GetSolution(iPoint,0),
                                              solver_container[TURB_SOL]->GetNodes()->GetSolution(iPoint,0));

        /*--- Compute and update viscous residual ---*/

        auto residual = visc_numerics->ComputeResidual(config);
        LinSysRes.SubtractBlock(iPoint, residual);

        /*--- Viscous Jacobian contribution for implicit integration ---*/

        if (implicit)
          Jacobian.SubtractBlock2Diag(iPoint, residual.jacobian_i);

      }

    }
  }
  END_SU2_OMP_FOR

  /*--- Free locally allocated memory ---*/
  delete [] Normal;

}


void CEulerSolver::BC_Far_Field_Residual(CGeometry* geometry, CSolver** solver_container, CNumerics* conv_numerics,
                                          CNumerics* visc_numerics, CConfig* config, unsigned short val_marker,
                                          unsigned long val_element, unsigned short iNode,
                                          su2double* residualBuffer) {
  unsigned short iDim, iVar, jVar;
  unsigned long iPoint, Point_Normal;
  long iVertex;

  su2double *GridVel;
  su2double Area, UnitNormal[MAXNDIM] = {0.0};
  su2double Density, Pressure, Energy,  Velocity[MAXNDIM] = {0.0};
  su2double Density_Bound, Pressure_Bound, Energy_Bound, Vel_Bound[MAXNDIM] = {0.0};
  su2double Density_Infty, Pressure_Infty, Vel_Infty[MAXNDIM] = {0.0};
  su2double SoundSpeed, Entropy, Velocity2, Vn;
  su2double SoundSpeed_Bound, Entropy_Bound, Vel2_Bound, Vn_Bound;
  su2double SoundSpeed_Infty, Entropy_Infty, Vel2_Infty, Vn_Infty, Qn_Infty;
  su2double RiemannPlus, RiemannMinus;
  su2double *V_infty, *V_domain;

  su2double Gas_Constant     = config->GetGas_ConstantND();

  su2double dUInfdUBound[5][5] = {0.0}, totJac[5][5] = {0.0};

  bool implicit       = config->GetKind_TimeIntScheme() == EULER_IMPLICIT;
  bool viscous        = config->GetViscous();
  bool tkeNeeded = config->GetKind_Turb_Model() == TURB_MODEL::SST;

  su2double Normal[MAXNDIM] = {0.0};

  /** Normal should already be pointing outward **/
  const su2double* AreaVector = geometry->bound[val_marker][val_element]->getNormal();
  Area = GeometryToolbox::Norm(nDim,AreaVector);
  for (iDim = 0; iDim < nDim; iDim++) UnitNormal[iDim] = AreaVector[iDim] / Area;
  for (iDim = 0; iDim < nDim; iDim++) Normal[iDim] = AreaVector[iDim] / 1.0;

  /*--- Loop over all the vertices on this boundary marker ---*/

  iPoint = geometry->bound[val_marker][val_element]->GetNode(iNode);
  iVertex = geometry->nodes->GetVertex(iPoint,val_marker);


    /*--- Allocate the value at the infinity ---*/
    V_infty = GetCharacPrimVar(val_marker, iVertex);

    for(int iVar = 0; iVar < nVar; ++iVar)
      for(int jVar = 0; iVar < nVar; ++iVar) {
        dUInfdUBound[iVar][jVar] = 0;
        totJac[iVar][jVar] = 0;
      }

    /*--- Check if the node belongs to the domain (i.e, not a halo node) ---*/

    if (true/*geometry->nodes->GetDomain(iPoint)*/) {

      /*--- Index of the closest interior node ---*/

      Point_Normal = geometry->vertex[val_marker][iVertex]->GetNormal_Neighbor();

      /*--- Normal vector for this vertex (negate for outward convention) ---*/

      conv_numerics->SetNormal(Normal);

      /*--- Retrieve solution at the farfield boundary node ---*/
      V_domain = nodes->GetPrimitive(iPoint);

      /*--- Construct solution state at infinity for compressible flow by
         using Riemann invariants, and then impose a weak boundary condition
         by computing the flux using this new state for U. See CFD texts by
         Hirsch or Blazek for more detail. Adapted from an original
         implementation in the Stanford University multi-block (SUmb) solver
         in the routine bcFarfield.f90 written by Edwin van der Weide,
         last modified 06-12-2005. First, compute the unit normal at the
         boundary nodes. ---*/

      /*--- Store primitive variables (density, velocities, velocity squared,
         energy, pressure, and sound speed) at the boundary node, and set some
         other quantities for clarity. Project the current flow velocity vector
         at this boundary node into the local normal direction, i.e. compute
         v_bound.n.  ---*/

      Density_Bound = V_domain[nDim+2];
      Vel2_Bound = 0.0; Vn_Bound = 0.0;
      for (iDim = 0; iDim < nDim; iDim++) {
        Vel_Bound[iDim] = V_domain[iDim+1];
        Vel2_Bound     += Vel_Bound[iDim]*Vel_Bound[iDim];
        Vn_Bound       += Vel_Bound[iDim]*UnitNormal[iDim];
      }
      Pressure_Bound   = nodes->GetPressure(iPoint);
      SoundSpeed_Bound = sqrt(Gamma*Pressure_Bound/Density_Bound);
      Entropy_Bound    = pow(Density_Bound, Gamma)/Pressure_Bound;
      Energy_Bound = nodes->GetEnergy(iPoint);

      /*--- Store the primitive variable state for the freestream and build the characteristic ghost from the
         Riemann invariants (see CFD texts by Hirsch or Blazek; adapted from bcFarfield.f90 of SUmb by Edwin van der
         Weide): P1BoundaryGhost::FarField, shared with closure C of BC_ENTROPY_P1. Positive characteristics are
         incoming and take the freestream, negative ones the boundary state; grid movement enters the eigenvalue
         check through the relative normal velocity of the freestream. ---*/

      Density_Infty = GetDensity_Inf();
      Pressure_Infty = GetPressure_Inf();
      for (iDim = 0; iDim < nDim; iDim++) Vel_Infty[iDim] = GetVelocity_Inf(iDim);

      P1BoundaryGhost::FarFieldState ghost;
      P1BoundaryGhost::FarField(nDim, Gamma, Density_Bound, Vel_Bound, Pressure_Bound, Density_Infty, Vel_Infty,
                                Pressure_Infty, UnitNormal, dynamic_grid ? geometry->nodes->GetGridVel(iPoint) : nullptr,
                                ghost);
      Qn_Infty = ghost.Qn_Infty;
      SoundSpeed_Infty = ghost.SoundSpeed_Infty;
      SoundSpeed = ghost.SoundSpeed;
      Entropy = ghost.Entropy;
      Density = ghost.Density;
      for (iDim = 0; iDim < nDim; iDim++) Velocity[iDim] = ghost.Velocity[iDim];

      /*--- Recompute the primitive variables. ---*/

      Velocity2 = 0.0;
      for (iDim = 0; iDim < nDim; iDim++) {
        Velocity2 += Velocity[iDim]*Velocity[iDim];
      }
      Pressure = ghost.Pressure;
      Energy   = Pressure/(Gamma_Minus_One*Density) + 0.5*Velocity2;
      /*--- k of the boundary node, not the free-stream value, which decays right away in the domain (#1851). ---*/
      if (tkeNeeded) Energy += solver_container[TURB_SOL]->GetNodes()->GetSolution(iPoint, 0);

      /*--- Store new primitive state for computing the flux. ---*/

      V_infty[0] = Pressure/(Gas_Constant*Density);
      for (iDim = 0; iDim < nDim; iDim++)
        V_infty[iDim+1] = Velocity[iDim];
      V_infty[nDim+1] = Pressure;
      V_infty[nDim+2] = Density;
      V_infty[nDim+3] = Energy + Pressure/Density;


      if (implicit) {
        su2double dRpdP[5], dRmdP[5], dSdP[5], dVbdP[MAXNDIM][5];

        for (int iVar = 0; iVar < nVar; ++iVar) {
          dRpdP[iVar] = 0;
          dRmdP[iVar] = 0;
          dSdP[iVar] = 0;
        }

        for (iDim = 0; iDim < MAXNDIM; ++iDim)
          for (int iVar = 0; iVar < nVar; ++iVar)
            dVbdP[iDim][iVar] = 0;

        if (Qn_Infty > -SoundSpeed_Infty){
          dRpdP[0] = 2.0/(Gamma_Minus_One);
          for (iDim = 0; iDim < nDim; ++iDim)
            dRpdP[iDim+1] = UnitNormal[iDim];
        }

        if (Qn_Infty > SoundSpeed_Infty){
          dRmdP[0] = -2.0/(Gamma_Minus_One);
          for (iDim = 0; iDim < nDim; ++iDim)
            dRmdP[iDim+1] = UnitNormal[iDim];

        }

        if (Qn_Infty > 0) dSdP[nDim+1] = 1;

        for (iDim = 0; iDim < nDim; ++iDim)
          for (int jVar = 0; jVar < nVar; ++jVar)
            dVbdP[iDim][jVar] = 0.5 * UnitNormal[iDim] * (dRpdP[jVar] + dRmdP[jVar]);

        if (Qn_Infty > 0) {
          for (iDim = 0; iDim < nDim; ++iDim) {
            for (int jDim = 0; jDim < nDim; ++jDim)
              dVbdP[iDim][jDim+1] -= UnitNormal[iDim] * UnitNormal[jDim];
            dVbdP[iDim][iDim+1] += 1;
          }
        }

        su2double dUedVe[5][5];

        dUedVe[0][0] = 1;
        for (int iVar = 1; iVar < nVar; ++iVar) dUedVe[0][iVar] = 0;

        for (iDim = 0; iDim < nDim; ++iDim){
          dUedVe[iDim+1][0] = Velocity[iDim];
          for (int jDim = 0; jDim < nDim; ++jDim){
            dUedVe[iDim+1][jDim+1] = 0;
          }
          dUedVe[iDim+1][iDim+1] = Density;
          dUedVe[iDim+1][nDim+1] = 0;
        }
        /*--- SST: k of the node (held fixed) is part of the energy of both states: d(rho E)/d rho gets + k and the
           pressure of the boundary node gets dp/d rho = (gamma-1)(|v|^2/2 - k), which enters the entropy row below
           (the sound speed row is unchanged, its form with the total E already holds for a fixed k). ---*/
        const su2double tkeJac = tkeNeeded ? solver_container[TURB_SOL]->GetNodes()->GetSolution(iPoint, 0) : 0.0;
        dUedVe[nDim+1][0] = 0.5*Velocity2 + tkeJac;
        dUedVe[nDim+1][nDim+1] = 1/Gamma_Minus_One;
        for (iDim = 0; iDim < nDim; ++iDim)
          dUedVe[nDim+1][iDim+1] = Density*Velocity[iDim];


        su2double dVedPb[5][5];
        const su2double factor = pow(Entropy*SoundSpeed*SoundSpeed/Gamma,(2-Gamma)/Gamma_Minus_One);

        dVedPb[0][0] = factor * 2*Entropy*SoundSpeed/(Gamma*Gamma_Minus_One);
        dVedPb[0][nDim+1] = factor * SoundSpeed*SoundSpeed/(Gamma*Gamma_Minus_One);
        for (iDim = 0; iDim < nDim; ++iDim) dVedPb[0][iDim+1] = 0;

        for (iDim = 0; iDim < nDim; ++iDim){
          dVedPb[iDim+1][0] = 0;
          for (int jDim = 0; jDim < nDim; ++jDim){
            dVedPb[iDim+1][jDim+1] = 0;
          }
          dVedPb[iDim+1][iDim+1] = 1;
          dVedPb[iDim+1][nDim+1] = 0;
        }

        dVedPb[nDim+1][0] = factor * 2*Entropy*pow(SoundSpeed,3)/(Gamma*Gamma*Gamma_Minus_One) + 2*SoundSpeed*Density/Gamma;
        dVedPb[nDim+1][nDim+1] = pow(SoundSpeed,4) * factor / (Gamma*Gamma*Gamma_Minus_One);

        for (iDim = 0; iDim < nDim; ++iDim)
          dVedPb[nDim+1][iDim+1] = 0;

        su2double dPdU[5][5];

        dPdU[0][0] = 0.5*Gamma*Gamma_Minus_One/SoundSpeed_Bound * (Vel2_Bound/Density_Bound - Energy_Bound/Density_Bound);
        for (iDim = 0; iDim < nDim; ++iDim) dPdU[0][iDim+1] = -0.5*Gamma*Gamma_Minus_One/SoundSpeed_Bound * Vel_Bound[iDim]/Density_Bound;
        dPdU[0][nDim+1] = 0.5*Gamma*Gamma_Minus_One/(Density_Bound*SoundSpeed_Bound);

        for (iDim = 0; iDim < nDim; ++iDim){
          dPdU[iDim+1][0] = -Vel_Bound[iDim] / Density_Bound;
          for (int jDim = 0; jDim < nDim; ++jDim){
            dPdU[iDim+1][jDim+1] = 0;
          }
          dPdU[iDim+1][iDim+1] = 1.0/Density_Bound;
          dPdU[iDim+1][nDim+1] = 0;
        }
        dPdU[nDim+1][0] = Gamma*pow(Density_Bound,Gamma_Minus_One)/Pressure_Bound - 0.5*Gamma_Minus_One*Entropy_Bound*Vel2_Bound/Pressure_Bound
                          + Gamma_Minus_One*Entropy_Bound*tkeJac/Pressure_Bound;
        dPdU[nDim+1][nDim+1] = -Gamma_Minus_One*Entropy_Bound/Pressure_Bound;
        for (iDim = 0; iDim < nDim; ++iDim)
          dPdU[nDim+1][iDim+1] = Gamma_Minus_One*Entropy_Bound*Vel_Bound[iDim]/Pressure_Bound;



        su2double dPbdP[5][5];

        for (int iVar = 0; iVar < nVar; ++iVar)
          dPbdP[0][iVar] = 0.25 * Gamma_Minus_One * (dRpdP[iVar] - dRmdP[iVar]);

        for (iDim = 0; iDim < nDim; ++iDim) {
          for (int jVar = 0; jVar < nVar; ++jVar) {
            dPbdP[iDim+1][jVar] = dVbdP[iDim][jVar];
          }
        }

        for (int iVar = 0; iVar < nVar; ++iVar)
          dPbdP[nDim+1][iVar] = dSdP[iVar];


        su2double temp1[5][5], temp2[5][5];

        for (int iVar = 0; iVar < nVar; ++iVar)
          for (int jVar = 0; jVar < nVar; ++jVar) {
            temp1[iVar][jVar] = 0;
            for (int kVar = 0; kVar < nVar; ++kVar)
              temp1[iVar][jVar] += dUedVe[iVar][kVar] * dVedPb[kVar][jVar];
          }

        for (int iVar = 0; iVar < nVar; ++iVar)
          for (int jVar = 0; jVar < nVar; ++jVar) {
            temp2[iVar][jVar] = 0;
            for (int kVar = 0; kVar < nVar; ++kVar)
              temp2[iVar][jVar] += temp1[iVar][kVar] * dPbdP[kVar][jVar];
          }

        for (int iVar = 0; iVar < nVar; ++iVar)
          for (int jVar = 0; jVar < nVar; ++jVar) {
            dUInfdUBound[iVar][jVar] = 0;
            for (int kVar = 0; kVar < nVar; ++kVar)
              dUInfdUBound[iVar][jVar] += temp2[iVar][kVar] * dPdU[kVar][jVar];
          }


      }

      /*--- Set various quantities in the numerics class ---*/

      conv_numerics->SetPrimitive(V_domain, V_infty);

      /*--- SST: both states contain the k of the node in their total enthalpy. ---*/
      if (tkeNeeded) {
        const su2double tke = solver_container[TURB_SOL]->GetNodes()->GetSolution(iPoint, 0);
        conv_numerics->SetTurbKineticEnergy(tke, tke);
      }

      if (dynamic_grid) {
        conv_numerics->SetGridVel(geometry->nodes->GetGridVel(iPoint),
                                  geometry->nodes->GetGridVel(iPoint));
      }

      /*--- Compute the convective residual using an upwind scheme ---*/

      auto residual = conv_numerics->ComputeResidual(config);

      /*--- Update residual value ---*/
      for (iVar = 0; iVar < nVar; ++iVar) residualBuffer[iVar] = residual[iVar];

      /*--- Jacobian contribution for implicit integration. ---*/
      if (implicit) {
        for (iVar = 0; iVar < nVar; ++iVar)
          for (jVar = 0; jVar < nVar; ++jVar) {
            residualBuffer[MAXNVAR + iVar*nVar + jVar] = residual.jacobian_i[iVar][jVar];
            for (int kVar = 0; kVar < nVar; ++kVar)
              residualBuffer[MAXNVAR + iVar*nVar + jVar] += residual.jacobian_j[iVar][kVar] * dUInfdUBound[kVar][jVar];
          }
      }



      /*--- Viscous residual contribution ---*/

      if (viscous) {

        /*--- Set laminar and eddy viscosity at the infinity ---*/

        V_infty[nDim+5] = nodes->GetLaminarViscosity(iPoint);
        V_infty[nDim+6] = nodes->GetEddyViscosity(iPoint);

        /*--- Set the normal vector and the coordinates ---*/

        visc_numerics->SetNormal(Normal);
        su2double Coord_Reflected[MAXNDIM];
        GeometryToolbox::PointPointReflect(nDim, geometry->nodes->GetCoord(Point_Normal),
                                           geometry->nodes->GetCoord(iPoint), Coord_Reflected);
        visc_numerics->SetCoord(geometry->nodes->GetCoord(iPoint), Coord_Reflected);

        /*--- Primitive variables, and gradient ---*/

        visc_numerics->SetPrimitive(V_domain, V_infty);
        visc_numerics->SetPrimVarGradient(nodes->GetGradient_Primitive(iPoint),
                                          nodes->GetGradient_Primitive(iPoint));

        /*--- Turbulent kinetic energy ---*/

        if (config->GetKind_Turb_Model() == TURB_MODEL::SST)
          visc_numerics->SetTurbKineticEnergy(solver_container[TURB_SOL]->GetNodes()->GetSolution(iPoint,0),
                                              solver_container[TURB_SOL]->GetNodes()->GetSolution(iPoint,0));

        /*--- Compute and update viscous residual ---*/

        auto residual = visc_numerics->ComputeResidual(config);

        for (iVar = 0; iVar < nVar; ++iVar) residualBuffer[iVar] -= residual[iVar];

        /*--- Jacobian contribution for implicit integration. ---*/
        if (implicit) {
          for (iVar = 0; iVar < nVar; ++iVar){
          for (jVar = 0; jVar < nVar; ++jVar) {
            residualBuffer[MAXNVAR + iVar*nVar + jVar] -= residual.jacobian_i[iVar][jVar];
          }
          }
        }


      }

    }


}

void CEulerSolver::BC_Riemann(CGeometry *geometry, CSolver **solver_container,
                              CNumerics *conv_numerics, CNumerics *visc_numerics,
                              CConfig *config, unsigned short val_marker) {
  SU2_ZONE_SCOPED

  const string Marker_Tag = config->GetMarker_All_TagBound(val_marker);
  const bool viscous      = config->GetViscous(),
             implicit     = config->GetKind_TimeIntScheme() == EULER_IMPLICIT,
             gravity      = config->GetGravityForce(),
             tkeNeeded    = config->GetKind_Turb_Model() == TURB_MODEL::SST,
             ideal_gas    = config->GetKind_FluidModel() == STANDARD_AIR ||
                            config->GetKind_FluidModel() == IDEAL_GAS;
  CVariable* turbNodes = nullptr;
  if (tkeNeeded) turbNodes = solver_container[TURB_SOL]->GetNodes();

  su2double **P_Tensor = new su2double*[nVar],
            **invP_Tensor = new su2double*[nVar];
  for (auto iVar = 0u; iVar < nVar; iVar++){
    P_Tensor[iVar] = new su2double[nVar];
    invP_Tensor[iVar] = new su2double[nVar];
  }

  /*--- Loop over all the vertices on this boundary marker ---*/

  SU2_OMP_FOR_DYN(OMP_MIN_SIZE)
  for (auto iVertex = 0u; iVertex < geometry->nVertex[val_marker]; iVertex++) {

    auto iPoint = geometry->vertex[val_marker][iVertex]->GetNode();

    /*--- Check if the node belongs to the domain (i.e., not a halo node) ---*/
    if (geometry->nodes->GetDomain(iPoint)) {

      /*--- Index of the closest interior node ---*/
      const auto Point_Normal = geometry->vertex[val_marker][iVertex]->GetNormal_Neighbor();

      /*--- Normal vector for this vertex (negate for outward convention) ---*/
      su2double Normal[MAXNDIM] = {0};
      geometry->vertex[val_marker][iVertex]->GetNormal(Normal);
      for (auto iDim = 0u; iDim < nDim; iDim++) Normal[iDim] = -Normal[iDim];
      conv_numerics->SetNormal(Normal);

      const auto Area = GeometryToolbox::Norm(nDim, Normal);

      su2double UnitNormal[MAXNDIM] = {0};
      for (auto iDim = 0u; iDim < nDim; iDim++) UnitNormal[iDim] = Normal[iDim]/Area;

      /*--- Retrieve solution at this boundary node ---*/
      const auto V_domain = nodes->GetPrimitive(iPoint);

      /*--- Compute the internal state u_i ---*/
      su2double Velocity_i[MAXNDIM]={0};
      for (auto iDim=0u; iDim < nDim; iDim++)
        Velocity_i[iDim] = nodes->GetVelocity(iPoint,iDim);

      const auto Velocity2_i = GeometryToolbox::SquaredNorm(nDim, Velocity_i);

      const auto Density_i = nodes->GetDensity(iPoint);

      const auto Energy_i = nodes->GetEnergy(iPoint);
      su2double StaticEnergy_i = Energy_i - 0.5*Velocity2_i;
      if (tkeNeeded) StaticEnergy_i -= turbNodes->GetSolution(iPoint, 0);

      GetFluidModel()->SetTDState_rhoe(Density_i, StaticEnergy_i);

      const auto Pressure_i = GetFluidModel()->GetPressure();
      const su2double Enthalpy_i = Energy_i + Pressure_i/Density_i;

      const auto SoundSpeed_i = GetFluidModel()->GetSoundSpeed();

      const su2double Kappa_i = GetFluidModel()->GetdPde_rho() / Density_i;
      const su2double Chi_i = GetFluidModel()->GetdPdrho_e() - Kappa_i * StaticEnergy_i;

      /*--- Build the external state u_e from boundary data and internal node ---*/
      su2double T_static{0}, P_static{0}, Pressure_e{0}, Density_static{0}, Enthalpy_e{0}, Energy_e{0}, Entropy_e{0},
                T_Total{0},  P_Total{0},  Density_e{0}, StaticEnthalpy_e{0}, StaticEnergy_e{0};

      su2double Velocity2_e{0}, NormalVelocity{0}, TangVelocity{0}, VelMag_e{0};
      su2double Velocity_e[MAXNDIM] = {0};
      const su2double * Flow_Dir, * Mach;
      switch(config->GetKind_Data_Riemann(Marker_Tag))
      {
      case TOTAL_CONDITIONS_PT:

        /*--- Retrieve the specified total conditions for this boundary. ---*/
        if (gravity) P_Total = config->GetRiemann_Var1(Marker_Tag) - geometry->nodes->GetCoord(iPoint, nDim-1)*STANDARD_GRAVITY;/// check in which case is true (only freesurface?)
        else P_Total  = config->GetRiemann_Var1(Marker_Tag);
        T_Total  = config->GetRiemann_Var2(Marker_Tag);
        Flow_Dir = config->GetRiemann_FlowDir(Marker_Tag);

        /*--- Non-dim. the inputs if necessary. ---*/
        P_Total /= config->GetPressure_Ref();
        T_Total /= config->GetTemperature_Ref();

        /* --- Computes the total state --- */
        GetFluidModel()->SetTDState_PT(P_Total, T_Total);
        Enthalpy_e = GetFluidModel()->GetStaticEnergy()+ GetFluidModel()->GetPressure()/GetFluidModel()->GetDensity();
        Entropy_e = GetFluidModel()->GetEntropy();

        /* --- Compute the boundary state u_e --- */
        Velocity2_e = Velocity2_i;
        if (nDim == 2){
          NormalVelocity= -sqrt(Velocity2_e)*Flow_Dir[0];
          TangVelocity= -sqrt(Velocity2_e)*Flow_Dir[1];
          Velocity_e[0]= UnitNormal[0]*NormalVelocity - UnitNormal[1]*TangVelocity;
          Velocity_e[1]= UnitNormal[1]*NormalVelocity + UnitNormal[0]*TangVelocity;
        }else{
          for (auto iDim = 0u; iDim < nDim; iDim++)
            Velocity_e[iDim] = sqrt(Velocity2_e)*Flow_Dir[iDim];
        }
        StaticEnthalpy_e = Enthalpy_e - 0.5 * Velocity2_e;
        GetFluidModel()->SetTDState_hs(StaticEnthalpy_e, Entropy_e);
        Density_e = GetFluidModel()->GetDensity();
        StaticEnergy_e = GetFluidModel()->GetStaticEnergy();
        Energy_e = StaticEnergy_e + 0.5 * Velocity2_e;
        break;

      case STATIC_SUPERSONIC_INFLOW_PT:

        /*--- Retrieve the specified total conditions for this boundary. ---*/
        if (gravity) P_static = config->GetRiemann_Var1(Marker_Tag) - geometry->nodes->GetCoord(iPoint, nDim-1)*STANDARD_GRAVITY;/// check in which case is true (only freesurface?)
        else P_static  = config->GetRiemann_Var1(Marker_Tag);
        T_static  = config->GetRiemann_Var2(Marker_Tag);
        Mach = config->GetRiemann_FlowDir(Marker_Tag);

        /*--- Non-dim. the inputs if necessary. ---*/
        P_static /= config->GetPressure_Ref();
        T_static /= config->GetTemperature_Ref();

        /* --- Computes the total state --- */
        GetFluidModel()->SetTDState_PT(P_static, T_static);

        /* --- Compute the boundary state u_e --- */
        for (auto iDim = 0u; iDim < nDim; iDim++)
          Velocity_e[iDim] = Mach[iDim]*GetFluidModel()->GetSoundSpeed();

        Velocity2_e = GeometryToolbox::SquaredNorm(nDim, Velocity_e);
        Density_e = GetFluidModel()->GetDensity();
        StaticEnergy_e = GetFluidModel()->GetStaticEnergy();
        Energy_e = StaticEnergy_e + 0.5 * Velocity2_e;
        break;

      case STATIC_SUPERSONIC_INFLOW_PD:

        /*--- Retrieve the specified total conditions for this boundary. ---*/

        if (gravity) P_static = config->GetRiemann_Var1(Marker_Tag) - geometry->nodes->GetCoord(iPoint, nDim-1)*STANDARD_GRAVITY;/// check in which case is true (only freesurface?)
        else P_static  = config->GetRiemann_Var1(Marker_Tag);
        Density_static  = config->GetRiemann_Var2(Marker_Tag);
        Mach = config->GetRiemann_FlowDir(Marker_Tag);

        /*--- Non-dim. the inputs if necessary. ---*/
        P_static /= config->GetPressure_Ref();
        Density_static /= config->GetDensity_Ref();

        /* --- Computes the total state --- */
        GetFluidModel()->SetTDState_Prho(P_static, Density_static);

        /* --- Compute the boundary state u_e --- */
        for (auto iDim = 0u; iDim < nDim; iDim++)
          Velocity_e[iDim] = Mach[iDim]*GetFluidModel()->GetSoundSpeed();

        Velocity2_e = GeometryToolbox::SquaredNorm(nDim, Velocity_e);
        Density_e = GetFluidModel()->GetDensity();
        StaticEnergy_e = GetFluidModel()->GetStaticEnergy();
        Energy_e = StaticEnergy_e + 0.5 * Velocity2_e;
        break;

      case DENSITY_VELOCITY:

        /*--- Retrieve the specified density and velocity magnitude ---*/
        Density_e  = config->GetRiemann_Var1(Marker_Tag);
        VelMag_e   = config->GetRiemann_Var2(Marker_Tag);
        Flow_Dir = config->GetRiemann_FlowDir(Marker_Tag);

        /*--- Non-dim. the inputs if necessary. ---*/
        Density_e /= config->GetDensity_Ref();
        VelMag_e /= config->GetVelocity_Ref();

        for (auto iDim = 0u; iDim < nDim; iDim++)
          Velocity_e[iDim] = VelMag_e*Flow_Dir[iDim];
        /*--- The total energy of the node already contains k, which is added below to every case. ---*/
        Energy_e = Energy_i;
        if (tkeNeeded) Energy_e -= turbNodes->GetSolution(iPoint, 0);
        break;

      case STATIC_PRESSURE:

        /*--- Retrieve the static pressure for this boundary. ---*/
        Pressure_e = config->GetRiemann_Var1(Marker_Tag);
        Pressure_e /= config->GetPressure_Ref();
        Density_e = Density_i;

        /* --- Compute the boundary state u_e --- */
        GetFluidModel()->SetTDState_Prho(Pressure_e, Density_e);
        for (auto iDim = 0u; iDim < nDim; iDim++)
          Velocity_e[iDim] = Velocity_i[iDim];

        Velocity2_e = GeometryToolbox::SquaredNorm(nDim, Velocity_e);
        Energy_e = GetFluidModel()->GetStaticEnergy() + 0.5*Velocity2_e;
        break;

      default:
        SU2_MPI::Error("Invalid Riemann input!", CURRENT_FUNCTION);
        break;
      }

      /*--- k of the boundary node, not the free-stream value, which decays right away in the domain (#1851).
       It is also removed from the static energy of the boundary state. ---*/
      const su2double Tke_e = tkeNeeded ? turbNodes->GetSolution(iPoint, 0) : 0.0;
      Energy_e += Tke_e;

      /*--- Compute P (matrix of right eigenvectors) ---*/
      conv_numerics->GetPMatrix(&Density_i, Velocity_i, &SoundSpeed_i, &Enthalpy_i, &Chi_i, &Kappa_i, UnitNormal, P_Tensor,
                                Tke_e);

      /*--- Compute inverse P (matrix of left eigenvectors)---*/
      conv_numerics->GetPMatrix_inv(invP_Tensor, &Density_i, Velocity_i, &SoundSpeed_i, &Chi_i, &Kappa_i, UnitNormal,
                                    Tke_e);

      /*--- eigenvalues contribution due to grid motion ---*/
      su2double ProjVelocity_i = GeometryToolbox::DotProduct(nDim, Velocity_i, UnitNormal);
      if (dynamic_grid) {
        const auto gridVel = geometry->nodes->GetGridVel(iPoint);
        const su2double ProjGridVel = GeometryToolbox::DotProduct(nDim, gridVel, UnitNormal);
        ProjVelocity_i -= ProjGridVel;
      }

      /*--- Flow eigenvalues, boundary state u_e and u_i ---*/
      su2double Lambda_i[MAXNVAR] = {0}, u_e[MAXNVAR] = {0}, u_i[MAXNVAR]={0}, u_b[MAXNVAR]={0},
                dw[MAXNVAR]={0};
      u_e[0] = Density_e;
      u_i[0] = Density_i;
      for (auto iDim = 0u; iDim < nDim; iDim++) {
        Lambda_i[iDim] = ProjVelocity_i;
        u_e[iDim+1] = Velocity_e[iDim]*Density_e;
        u_i[iDim+1] = Velocity_i[iDim]*Density_i;
      }
      Lambda_i[nVar-2] = ProjVelocity_i + SoundSpeed_i;
      Lambda_i[nVar-1] = ProjVelocity_i - SoundSpeed_i;
      u_e[nVar-1] = Energy_e*Density_e;
      u_i[nVar-1] = Energy_i*Density_i;

      /*--- Compute the characteristic jumps ---*/
      for (auto iVar = 0u; iVar < nVar; iVar++){
        dw[iVar] = 0;
        for (auto jVar = 0u; jVar < nVar; jVar++)
          dw[iVar] += invP_Tensor[iVar][jVar] * (u_e[jVar] - u_i[jVar]);
      }

      /*--- Compute the boundary state u_b using characteristics ---*/
      for (auto iVar = 0u; iVar < nVar; iVar++) {
        u_b[iVar] = u_i[iVar];
        for (auto jVar = 0u; jVar < nVar; jVar++) {
          if (Lambda_i[jVar] < 0)
            u_b[iVar] += P_Tensor[iVar][jVar]*dw[jVar];
        }
      }

      /*--- Compute the thermodynamic state in u_b ---*/
      su2double Velocity_b[MAXNDIM] = {0};
      const auto Density_b = u_b[0];
      for (auto iDim = 0u; iDim < nDim; iDim++)
        Velocity_b[iDim] = u_b[iDim+1]/Density_b;
      const auto Velocity2_b = GeometryToolbox::SquaredNorm(nDim, Velocity_b);

      const su2double Energy_b = u_b[nVar-1]/Density_b;
      const su2double StaticEnergy_b = Energy_b - 0.5*Velocity2_b - Tke_e;
      GetFluidModel()->SetTDState_rhoe(Density_b, StaticEnergy_b);

      /*--- Store number of Newton iterations at BC ---*/
      if(config->GetKind_FluidModel() == DATADRIVEN_FLUID)
        nodes->SetNewtonSolverIterations(iPoint, GetFluidModel()->GetnIter_Newton());

      const auto Pressure_b = GetFluidModel()->GetPressure();
      const auto Temperature_b = GetFluidModel()->GetTemperature();
      const su2double Enthalpy_b = Energy_b + Pressure_b/Density_b;
      const su2double Kappa_b = GetFluidModel()->GetdPde_rho() / Density_b;
      const su2double Chi_b = GetFluidModel()->GetdPdrho_e() - Kappa_b * StaticEnergy_b;

      /*--- Compute the residuals ---*/
      su2double Residual[MAXNVAR] = {0};
      conv_numerics->GetInviscidProjFlux(&Density_b, Velocity_b, &Pressure_b, &Enthalpy_b, Normal, Residual);

      /*--- Residual contribution due to grid motion ---*/
      if (dynamic_grid) {
        const auto gridVel = geometry->nodes->GetGridVel(iPoint);
        const auto projVelocity = GeometryToolbox::DotProduct(nDim, gridVel, Normal);
        for (auto iVar = 0u; iVar < nVar; iVar++)
          Residual[iVar] -= projVelocity *u_b[iVar];
      }

      /*--- Update residual value ---*/
      LinSysRes.AddBlock(iPoint, Residual);

      if (implicit) {
        su2double **Jacobian_b = new su2double*[nVar],
                  **Jacobian_i = new su2double*[nVar];
        su2double **DubDu = new su2double*[nVar];
        for (auto iVar = 0u; iVar < nVar; iVar++){
          Jacobian_b[iVar] = new su2double[nVar];
          Jacobian_i[iVar] = new su2double[nVar];
          /*--- Initialize DubDu to unit matrix---*/
          DubDu[iVar] = new su2double[nVar];
          for (auto jVar=0u; jVar < nVar; jVar++)
            DubDu[iVar][jVar] = 0;

          DubDu[iVar][iVar]=1;
        }

        /*--- Compute DubDu -= RNL---*/
        for (auto iVar=0u; iVar<nVar; iVar++){
          for (auto jVar=0u; jVar<nVar; jVar++){
            for (auto kVar=0u; kVar<nVar; kVar++){
              if (Lambda_i[kVar]<0)
                DubDu[iVar][jVar] -= P_Tensor[iVar][kVar] * invP_Tensor[kVar][jVar];
            }
          }
        }

        /*--- Compute flux Jacobian in state b ---*/
        conv_numerics->GetInviscidProjJac(Velocity_b, &Enthalpy_b, &Chi_b, &Kappa_b, Normal, 1.0, Jacobian_b, Tke_e);

        /*--- Jacobian contribution due to grid motion ---*/
        if (dynamic_grid){
          const auto gridVel = geometry->nodes->GetGridVel(iPoint);
          const auto projVelocity = GeometryToolbox::DotProduct(nDim, gridVel, Normal);
          for (auto iVar = 0u; iVar < nVar; iVar++)
            Jacobian_b[iVar][iVar] -= projVelocity;
        }

        /*--- initiate Jacobian_i to zero matrix ---*/
        for (auto iVar=0u; iVar<nVar; iVar++) {
          for (auto jVar=0u; jVar<nVar; jVar++) {
            Jacobian_i[iVar][jVar] = 0.0;
            /*--- Compute numerical flux Jacobian at node i ---*/
            for (auto kVar=0u; kVar<nVar; kVar++)
              Jacobian_i[iVar][jVar] += Jacobian_b[iVar][kVar] * DubDu[kVar][jVar];
          }
        }

        /*--- Jacobian contribution for implicit integration ---*/
        Jacobian.AddBlock2Diag(iPoint, Jacobian_i);

        for (auto iVar = 0u; iVar < nVar; iVar++) {
          delete [] Jacobian_b[iVar];
          delete [] DubDu[iVar];
          delete [] Jacobian_i[iVar];
        }
        delete [] Jacobian_b;
        delete [] DubDu;
        delete [] Jacobian_i;
      }

      /*--- Viscous contribution ---*/

      if (viscous) {

        /*--- Primitive variables, using the derived quantities ---*/
        auto V_boundary= GetCharacPrimVar(val_marker, iVertex);
        V_boundary[prim_idx.Temperature()] = Temperature_b;
        for (auto iDim = 0u; iDim < nDim; iDim++)
          V_boundary[prim_idx.Velocity() + iDim] = Velocity_b[iDim];
        V_boundary[prim_idx.Pressure()] = Pressure_b;
        V_boundary[prim_idx.Density()] = Density_b;
        V_boundary[prim_idx.Enthalpy()] = Enthalpy_b;

        /*--- Set laminar and eddy viscosity at the infinity ---*/

        V_boundary[prim_idx.LaminarViscosity()] = GetFluidModel()->GetLaminarViscosity();
        V_boundary[prim_idx.EddyViscosity()] = nodes->GetEddyViscosity(iPoint);
        V_boundary[prim_idx.ThermalConductivity()] = GetFluidModel()->GetThermalConductivity();
        V_boundary[prim_idx.CpTotal()] = GetFluidModel()->GetCp();

        /*--- Set the normal vector and the coordinates ---*/

        visc_numerics->SetNormal(Normal);
        su2double Coord_Reflected[MAXNDIM];
        GeometryToolbox::PointPointReflect(nDim, geometry->nodes->GetCoord(Point_Normal),
                                                 geometry->nodes->GetCoord(iPoint), Coord_Reflected);
        visc_numerics->SetCoord(geometry->nodes->GetCoord(iPoint), Coord_Reflected);

        /*--- Primitive variables, and gradient ---*/

        visc_numerics->SetPrimitive(V_domain, V_boundary);
        visc_numerics->SetPrimVarGradient(nodes->GetGradient_Primitive(iPoint), nodes->GetGradient_Primitive(iPoint));

        /*--- Secondary variables ---*/

        const su2double* S_domain = ideal_gas? nullptr : nodes->GetSecondary(iPoint);

        /*--- Compute secondary thermodynamic properties (partial derivatives...) ---*/
        su2double S_boundary[8];
        S_boundary[0]= GetFluidModel()->GetdPdrho_e();
        S_boundary[1]= GetFluidModel()->GetdPde_rho();

        S_boundary[2]= GetFluidModel()->GetdTdrho_e();
        S_boundary[3]= GetFluidModel()->GetdTde_rho();

        /*--- Compute secondary thermo-physical properties (partial derivatives...) ---*/

        S_boundary[4]= GetFluidModel()->Getdmudrho_T();
        S_boundary[5]= GetFluidModel()->GetdmudT_rho();

        S_boundary[6]= GetFluidModel()->Getdktdrho_T();
        S_boundary[7]= GetFluidModel()->GetdktdT_rho();

        visc_numerics->SetSecondary(S_domain, S_boundary);

        /*--- Turbulent kinetic energy ---*/

        if (tkeNeeded)
          visc_numerics->SetTurbKineticEnergy(solver_container[TURB_SOL]->GetNodes()->GetSolution(iPoint,0),
                                              solver_container[TURB_SOL]->GetNodes()->GetSolution(iPoint,0));

        /*--- Compute and update residual ---*/
        auto residual = visc_numerics->ComputeResidual(config);
        LinSysRes.SubtractBlock(iPoint, residual);

        /*--- Jacobian contribution for implicit integration ---*/
        if (implicit)
          Jacobian.SubtractBlock2Diag(iPoint, residual.jacobian_i);

      }
      /*--- Store number of Newton iterations at BC ---*/
      if(config->GetKind_FluidModel() == DATADRIVEN_FLUID)
        nodes->SetNewtonSolverIterations(iPoint, GetFluidModel()->GetnIter_Newton());

    }
  }
  END_SU2_OMP_FOR

  for(auto iVar=0u; iVar<nVar; iVar++) {
    delete [] P_Tensor[iVar];
    delete [] invP_Tensor[iVar];
  }
  delete[] P_Tensor;
  delete[] invP_Tensor;
}


void CEulerSolver::BC_TurboRiemann(CGeometry *geometry, CSolver **solver_container,
                                   CNumerics *conv_numerics, CNumerics *visc_numerics,
                                   CConfig *config, unsigned short val_marker) {
  SU2_ZONE_SCOPED

  unsigned short iDim, iVar, jVar, kVar, iSpan;
  unsigned long iPoint, Point_Normal, oldVertex, iVertex;
  const su2double *Flow_Dir;
  su2double P_Total, T_Total;
  su2double *Velocity_b, Velocity2_b, Enthalpy_b, Energy_b, StaticEnergy_b, Density_b, Kappa_b, Chi_b, Pressure_b, Temperature_b;
  su2double *Velocity_e, Velocity2_e, Enthalpy_e, Entropy_e, Energy_e = 0.0, StaticEnthalpy_e, StaticEnergy_e, Density_e = 0.0, Pressure_e;
  su2double *Velocity_i, Velocity2_i, Enthalpy_i, Energy_i, StaticEnergy_i, Density_i, Kappa_i, Chi_i, Pressure_i, SoundSpeed_i;
  su2double ProjVelocity_i;
  su2double **P_Tensor, **invP_Tensor, *Lambda_i, **Jacobian_b, **Jacobian_i, **DubDu, *dw, *u_e, *u_i, *u_b;
  su2double *gridVel, *Residual;
  su2double *V_boundary, *V_domain, *S_boundary, *S_domain;
  su2double AverageEnthalpy, AverageEntropy;
  unsigned short  iZone  = config->GetiZone();
  bool implicit = (config->GetKind_TimeIntScheme() == EULER_IMPLICIT);
  string Marker_Tag = config->GetMarker_All_TagBound(val_marker);
  unsigned short nSpanWiseSections = geometry->GetnSpanWiseSections(config->GetMarker_All_TurbomachineryFlag(val_marker));
  bool viscous = config->GetViscous();
  bool gravity = (config->GetGravityForce());
  bool tkeNeeded = (config->GetKind_Turb_Model() == TURB_MODEL::SST);
  const bool ideal_gas = config->GetKind_FluidModel() == STANDARD_AIR ||
                         config->GetKind_FluidModel() == IDEAL_GAS;
  CVariable* turbNodes = nullptr;
  if (tkeNeeded) turbNodes = solver_container[TURB_SOL]->GetNodes();

  su2double *Normal, *turboNormal, *UnitNormal, *FlowDirMix, FlowDirMixMag, *turboVelocity;
  Normal = new su2double[nDim];
  turboNormal = new su2double[nDim];
  UnitNormal = new su2double[nDim];

  Velocity_i = new su2double[nDim];
  Velocity_b = new su2double[nDim];
  Velocity_e = new su2double[nDim];
  turboVelocity = new su2double[nDim];
  FlowDirMix = new su2double[nDim];
  Lambda_i = new su2double[nVar];
  u_i = new su2double[nVar];
  u_e = new su2double[nVar];
  u_b = new su2double[nVar];
  dw = new su2double[nVar];

  Residual = new su2double[nVar];

  S_boundary = new su2double[8];

  P_Tensor = new su2double*[nVar];
  invP_Tensor = new su2double*[nVar];
  Jacobian_i = new su2double*[nVar];
  for (iVar = 0; iVar < nVar; iVar++)
  {
    P_Tensor[iVar] = new su2double[nVar];
    invP_Tensor[iVar] = new su2double[nVar];
    Jacobian_i[iVar] = new su2double[nVar];
  }

  /*--- Loop over all the vertices on this boundary marker ---*/
  for (iSpan= 0; iSpan < nSpanWiseSections; iSpan++){

    SU2_OMP_FOR_DYN(OMP_MIN_SIZE)
    for (iVertex = 0; iVertex < geometry->GetnVertexSpan(val_marker,iSpan); iVertex++) {

      /*--- using the other vertex information for retrieving some information ---*/
      oldVertex = geometry->turbovertex[val_marker][iSpan][iVertex]->GetOldVertex();
      V_boundary= GetCharacPrimVar(val_marker, oldVertex);

      /*--- Index of the closest interior node ---*/
      Point_Normal = geometry->vertex[val_marker][oldVertex]->GetNormal_Neighbor();

      /*--- Normal vector for this vertex (negate for outward convention),
       *    this normal is scaled with the area of the face of the element  ---*/
      geometry->vertex[val_marker][oldVertex]->GetNormal(Normal);
      for (iDim = 0; iDim < nDim; iDim++) Normal[iDim] = -Normal[iDim];
      conv_numerics->SetNormal(Normal);

      /*--- find the node related to the vertex ---*/
      iPoint = geometry->turbovertex[val_marker][iSpan][iVertex]->GetNode();
      if (geometry->nodes->GetDomain(iPoint)){

        /*--- Normalize Normal vector for this vertex (already for outward convention) ---*/
        geometry->turbovertex[val_marker][iSpan][iVertex]->GetNormal(UnitNormal);
        geometry->turbovertex[val_marker][iSpan][iVertex]->GetTurboNormal(turboNormal);

        /*--- Retrieve solution at this boundary node ---*/
        V_domain = nodes->GetPrimitive(iPoint);

        /* --- Compute the internal state u_i --- */
        Velocity2_i = 0;
        for (iDim=0; iDim < nDim; iDim++)
        {
          Velocity_i[iDim] = nodes->GetVelocity(iPoint,iDim);
          Velocity2_i += Velocity_i[iDim]*Velocity_i[iDim];
        }

        Density_i = nodes->GetDensity(iPoint);

        Energy_i = nodes->GetEnergy(iPoint);
        StaticEnergy_i = Energy_i - 0.5*Velocity2_i;
        if (tkeNeeded) StaticEnergy_i -= turbNodes->GetSolution(iPoint, 0);

        GetFluidModel()->SetTDState_rhoe(Density_i, StaticEnergy_i);

        Pressure_i = GetFluidModel()->GetPressure();
        Enthalpy_i = Energy_i + Pressure_i/Density_i;

        SoundSpeed_i = GetFluidModel()->GetSoundSpeed();

        Kappa_i = GetFluidModel()->GetdPde_rho() / Density_i;
        Chi_i = GetFluidModel()->GetdPdrho_e() - Kappa_i * StaticEnergy_i;

        ProjVelocity_i = 0.0;
        for (iDim = 0; iDim < nDim; iDim++)
          ProjVelocity_i += Velocity_i[iDim]*UnitNormal[iDim];

        su2double donorAverages[5] = {0.0};
        switch (config->GetKind_Data_Riemann(Marker_Tag)){
          case MIXING_IN: case MIXING_IN_1D: case MIXING_OUT: case MIXING_OUT_1D:
            for (auto mixVar = 0u; mixVar < 5; mixVar++) donorAverages[mixVar] = GetMixingState(val_marker, iSpan, mixVar);
            break;
          default:
            break;
        }
        ExtAverageDensity = donorAverages[0];
        ExtAveragePressure = donorAverages[1];
        ExtAverageTurboVelocity[0] = donorAverages[2];
        ExtAverageTurboVelocity[1] = donorAverages[3];
        if (nDim == 3) ExtAverageTurboVelocity[2] = donorAverages[4];

        /*--- Build the external state u_e from boundary data and internal node ---*/

        switch(config->GetKind_Data_Riemann(Marker_Tag))
        {
          //TODO(turbo), generalize for 3D case
          //TODO(turbo), generalize for Inlet and Outlet in for backflow treatment
          //TODO(turbo), implement not uniform inlet and radial equilibrium for the outlet
          case TOTAL_CONDITIONS_PT:

            /*--- Retrieve the specified total conditions for this boundary. ---*/
            if (gravity) P_Total = config->GetRiemann_Var1(Marker_Tag) - geometry->nodes->GetCoord(iPoint, nDim-1)*STANDARD_GRAVITY;/// check in which case is true (only freesurface?)
            else P_Total  = config->GetRiemann_Var1(Marker_Tag);
            T_Total  = config->GetRiemann_Var2(Marker_Tag);
            Flow_Dir = config->GetRiemann_FlowDir(Marker_Tag);

            /*--- Non-dim. the inputs if necessary. ---*/
            P_Total /= config->GetPressure_Ref();
            T_Total /= config->GetTemperature_Ref();

            /* --- Computes the total state --- */
            GetFluidModel()->SetTDState_PT(P_Total, T_Total);
            Enthalpy_e = GetFluidModel()->GetStaticEnergy()+ GetFluidModel()->GetPressure()/GetFluidModel()->GetDensity();
            Entropy_e = GetFluidModel()->GetEntropy();

            /* --- Compute the boundary state u_e --- */
            Velocity2_e = Velocity2_i;
            for (iDim = 0; iDim < nDim; iDim++)
              turboVelocity[iDim] = sqrt(Velocity2_e)*Flow_Dir[iDim];
            ComputeBackVelocity(turboVelocity,turboNormal, Velocity_e, config->GetMarker_All_TurbomachineryFlag(val_marker),config->GetKind_TurboMachinery(iZone));
            StaticEnthalpy_e = Enthalpy_e - 0.5 * Velocity2_e;
            GetFluidModel()->SetTDState_hs(StaticEnthalpy_e, Entropy_e);
            Density_e = GetFluidModel()->GetDensity();
            StaticEnergy_e = GetFluidModel()->GetStaticEnergy();
            Energy_e = StaticEnergy_e + 0.5 * Velocity2_e;
            if (tkeNeeded) Energy_e += turbNodes->GetSolution(iPoint, 0);
            break;

          case MIXING_IN:

            /* --- compute total averaged quantities ---*/
            GetFluidModel()->SetTDState_Prho(ExtAveragePressure, ExtAverageDensity);
            AverageEnthalpy = GetFluidModel()->GetStaticEnergy() + ExtAveragePressure/ExtAverageDensity;
            AverageEntropy  = GetFluidModel()->GetEntropy();

            FlowDirMixMag = 0;
            for (iDim = 0; iDim < nDim; iDim++)
              FlowDirMixMag += ExtAverageTurboVelocity[iDim]*ExtAverageTurboVelocity[iDim];
            for (iDim = 0; iDim < nDim; iDim++){
              FlowDirMix[iDim] = ExtAverageTurboVelocity[iDim]/sqrt(FlowDirMixMag);
            }


            /* --- Computes the total state --- */
            Enthalpy_e = AverageEnthalpy;
            Entropy_e = AverageEntropy;

            /* --- Compute the boundary state u_e --- */
            Velocity2_e = Velocity2_i;
            for (iDim = 0; iDim < nDim; iDim++){
              turboVelocity[iDim] = sqrt(Velocity2_e)*FlowDirMix[iDim];

            }
            ComputeBackVelocity(turboVelocity,turboNormal, Velocity_e, config->GetMarker_All_TurbomachineryFlag(val_marker),config->GetKind_TurboMachinery(iZone));

            StaticEnthalpy_e = Enthalpy_e - 0.5 * Velocity2_e;
            GetFluidModel()->SetTDState_hs(StaticEnthalpy_e, Entropy_e);
            Density_e = GetFluidModel()->GetDensity();
            StaticEnergy_e = GetFluidModel()->GetStaticEnergy();
            Energy_e = StaticEnergy_e + 0.5 * Velocity2_e;
            if (tkeNeeded) Energy_e += turbNodes->GetSolution(iPoint, 0);
            break;


          case MIXING_OUT:

            /*--- Retrieve the static pressure for this boundary. ---*/
            Pressure_e = ExtAveragePressure;
            Density_e = Density_i;

            /* --- Compute the boundary state u_e --- */
            GetFluidModel()->SetTDState_Prho(Pressure_e, Density_e);
            Velocity2_e = 0.0;
            for (iDim = 0; iDim < nDim; iDim++) {
              Velocity_e[iDim] = Velocity_i[iDim];
              Velocity2_e += Velocity_e[iDim]*Velocity_e[iDim];
            }
            Energy_e = GetFluidModel()->GetStaticEnergy() + 0.5*Velocity2_e;
            if (tkeNeeded) Energy_e += turbNodes->GetSolution(iPoint, 0);
            break;

          case STATIC_PRESSURE:

            /*--- Retrieve the static pressure for this boundary. ---*/
            Pressure_e = config->GetRiemann_Var1(Marker_Tag);
            Pressure_e /= config->GetPressure_Ref();
            Density_e = Density_i;

            /* --- Compute the boundary state u_e --- */
            GetFluidModel()->SetTDState_Prho(Pressure_e, Density_e);
            Velocity2_e = 0.0;
            for (iDim = 0; iDim < nDim; iDim++) {
              Velocity_e[iDim] = Velocity_i[iDim];
              Velocity2_e += Velocity_e[iDim]*Velocity_e[iDim];
            }
            Energy_e = GetFluidModel()->GetStaticEnergy() + 0.5*Velocity2_e;
            if (tkeNeeded) Energy_e += turbNodes->GetSolution(iPoint, 0);
            break;


          case RADIAL_EQUILIBRIUM:

            /*--- Retrieve the static pressure for this boundary. ---*/
            Pressure_e = RadialEquilibriumPressure[val_marker][iSpan];
            Density_e = Density_i;

            /* --- Compute the boundary state u_e --- */
            GetFluidModel()->SetTDState_Prho(Pressure_e, Density_e);
            Velocity2_e = 0.0;
            for (iDim = 0; iDim < nDim; iDim++) {
              Velocity_e[iDim] = Velocity_i[iDim];
              Velocity2_e += Velocity_e[iDim]*Velocity_e[iDim];
            }
            Energy_e = GetFluidModel()->GetStaticEnergy() + 0.5*Velocity2_e;
            if (tkeNeeded) Energy_e += turbNodes->GetSolution(iPoint, 0);
            break;

          default:
            SU2_MPI::Error("Invalid Riemann input!", CURRENT_FUNCTION);
            break;
        }

        /*--- Compute P (matrix of right eigenvectors) ---*/
        const su2double Tke_i = tkeNeeded ? turbNodes->GetSolution(iPoint, 0) : 0.0;  // SST: k in the total energy
        conv_numerics->GetPMatrix(&Density_i, Velocity_i, &SoundSpeed_i, &Enthalpy_i, &Chi_i, &Kappa_i, UnitNormal, P_Tensor,
                                  Tke_i);

        /*--- Compute inverse P (matrix of left eigenvectors)---*/
        conv_numerics->GetPMatrix_inv(invP_Tensor, &Density_i, Velocity_i, &SoundSpeed_i, &Chi_i, &Kappa_i, UnitNormal,
                                      Tke_i);

        /*--- eigenvalues contribution due to grid motion ---*/
        if (dynamic_grid){
          gridVel = geometry->nodes->GetGridVel(iPoint);

          su2double ProjGridVel = 0.0;
          for (iDim = 0; iDim < nDim; iDim++)
            ProjGridVel   += gridVel[iDim]*UnitNormal[iDim];
          ProjVelocity_i -= ProjGridVel;
        }

        /*--- Flow eigenvalues ---*/
        for (iDim = 0; iDim < nDim; iDim++)
          Lambda_i[iDim] = ProjVelocity_i;
        Lambda_i[nVar-2] = ProjVelocity_i + SoundSpeed_i;
        Lambda_i[nVar-1] = ProjVelocity_i - SoundSpeed_i;

        /* --- Compute the boundary state u_e --- */
        u_e[0] = Density_e;
        for (iDim = 0; iDim < nDim; iDim++)
          u_e[iDim+1] = Velocity_e[iDim]*Density_e;
        u_e[nVar-1] = Energy_e*Density_e;

        /* --- Compute the boundary state u_i --- */
        u_i[0] = Density_i;
        for (iDim = 0; iDim < nDim; iDim++)
          u_i[iDim+1] = Velocity_i[iDim]*Density_i;
        u_i[nVar-1] = Energy_i*Density_i;

        /*--- Compute the characteristic jumps ---*/
        for (iVar = 0; iVar < nVar; iVar++)
        {
          dw[iVar] = 0;
          for (jVar = 0; jVar < nVar; jVar++)
            dw[iVar] += invP_Tensor[iVar][jVar] * (u_e[jVar] - u_i[jVar]);

        }

        /*--- Compute the boundary state u_b using characteristics ---*/
        for (iVar = 0; iVar < nVar; iVar++)
        {
          u_b[iVar] = u_i[iVar];

          for (jVar = 0; jVar < nVar; jVar++)
          {
            if (Lambda_i[jVar] < 0)
            {
              u_b[iVar] += P_Tensor[iVar][jVar]*dw[jVar];

            }
          }
        }


        /*--- Compute the thermodynamic state in u_b ---*/
        Density_b = u_b[0];
        Velocity2_b = 0;
        for (iDim = 0; iDim < nDim; iDim++)
        {
          Velocity_b[iDim] = u_b[iDim+1]/Density_b;
          Velocity2_b += Velocity_b[iDim]*Velocity_b[iDim];
        }
        Energy_b = u_b[nVar-1]/Density_b;
        StaticEnergy_b = Energy_b - 0.5*Velocity2_b - Tke_i;  // SST: k in the total energy
        GetFluidModel()->SetTDState_rhoe(Density_b, StaticEnergy_b);
        Pressure_b = GetFluidModel()->GetPressure();
        Temperature_b = GetFluidModel()->GetTemperature();
        Enthalpy_b = Energy_b + Pressure_b/Density_b;
        Kappa_b = GetFluidModel()->GetdPde_rho() / Density_b;
        Chi_b = GetFluidModel()->GetdPdrho_e() - Kappa_b * StaticEnergy_b;

        /*--- Compute the residuals ---*/
        conv_numerics->GetInviscidProjFlux(&Density_b, Velocity_b, &Pressure_b, &Enthalpy_b, Normal, Residual);

        /*--- Residual contribution due to grid motion ---*/
        if (dynamic_grid) {
          gridVel = geometry->nodes->GetGridVel(iPoint);
          su2double projVelocity = 0.0;

          for (iDim = 0; iDim < nDim; iDim++)
            projVelocity +=  gridVel[iDim]*Normal[iDim];
          for (iVar = 0; iVar < nVar; iVar++)
            Residual[iVar] -= projVelocity *(u_b[iVar]);
        }

        if (implicit) {

          Jacobian_b = new su2double*[nVar];
          DubDu = new su2double*[nVar];
          for (iVar = 0; iVar < nVar; iVar++)
          {
            Jacobian_b[iVar] = new su2double[nVar];
            DubDu[iVar] = new su2double[nVar];
          }

          /*--- Initialize DubDu to unit matrix---*/

          for (iVar = 0; iVar < nVar; iVar++)
          {
            for (jVar = 0; jVar < nVar; jVar++)
              DubDu[iVar][jVar]= 0;

            DubDu[iVar][iVar]= 1;
          }

          /*--- Compute DubDu -= RNL---*/
          for (iVar=0; iVar<nVar; iVar++)
          {
            for (jVar=0; jVar<nVar; jVar++)
            {
              for (kVar=0; kVar<nVar; kVar++)
              {
                if (Lambda_i[kVar]<0)
                  DubDu[iVar][jVar] -= P_Tensor[iVar][kVar] * invP_Tensor[kVar][jVar];
              }
            }
          }

          /*--- Compute flux Jacobian in state b ---*/
          conv_numerics->GetInviscidProjJac(Velocity_b, &Enthalpy_b, &Chi_b, &Kappa_b, Normal, 1.0, Jacobian_b, Tke_i);

          /*--- Jacobian contribution due to grid motion ---*/
          if (dynamic_grid)
          {
            gridVel = geometry->nodes->GetGridVel(iPoint);
            su2double projVelocity = 0.0;
            for (iDim = 0; iDim < nDim; iDim++)
              projVelocity +=  gridVel[iDim]*Normal[iDim];
            for (iVar = 0; iVar < nVar; iVar++){
              Residual[iVar] -= projVelocity *(u_b[iVar]);
              Jacobian_b[iVar][iVar] -= projVelocity;
            }

          }

          /*--- initiate Jacobian_i to zero matrix ---*/
          for (iVar=0; iVar<nVar; iVar++)
            for (jVar=0; jVar<nVar; jVar++)
              Jacobian_i[iVar][jVar] = 0.0;

          /*--- Compute numerical flux Jacobian at node i ---*/
          for (iVar=0; iVar<nVar; iVar++) {
            for (jVar=0; jVar<nVar; jVar++) {
              for (kVar=0; kVar<nVar; kVar++) {
                Jacobian_i[iVar][jVar] += Jacobian_b[iVar][kVar] * DubDu[kVar][jVar];
              }
            }
          }

          for (iVar = 0; iVar < nVar; iVar++) {
            delete [] Jacobian_b[iVar];
            delete [] DubDu[iVar];
          }
          delete [] Jacobian_b;
          delete [] DubDu;
        }

        /*--- Update residual value ---*/
        LinSysRes.AddBlock(iPoint, Residual);

        /*--- Jacobian contribution for implicit integration ---*/
        if (implicit)
          Jacobian.AddBlock2Diag(iPoint, Jacobian_i);

        /*--- Viscous contribution ---*/

        if (viscous) {

          /*--- Primitive variables, using the derived quantities ---*/

          V_boundary[0] = Temperature_b;
          for (iDim = 0; iDim < nDim; iDim++)
            V_boundary[iDim+1] = Velocity_b[iDim];
          V_boundary[nDim+1] = Pressure_b;
          V_boundary[nDim+2] = Density_b;
          V_boundary[nDim+3] = Enthalpy_b;

          /*--- Set laminar and eddy viscosity at the infinity ---*/

          V_boundary[nDim+5] = GetFluidModel()->GetLaminarViscosity();
          V_boundary[nDim+6] = nodes->GetEddyViscosity(iPoint);
          V_boundary[nDim+7] = GetFluidModel()->GetThermalConductivity();
          V_boundary[nDim+8] = GetFluidModel()->GetCp();

          /*--- Set the normal vector and the coordinates ---*/

          visc_numerics->SetNormal(Normal);
          su2double Coord_Reflected[MAXNDIM];
          GeometryToolbox::PointPointReflect(nDim, geometry->nodes->GetCoord(Point_Normal),
                                                   geometry->nodes->GetCoord(iPoint), Coord_Reflected);
          visc_numerics->SetCoord(geometry->nodes->GetCoord(iPoint), Coord_Reflected);

          /*--- Primitive variables, and gradient ---*/

          visc_numerics->SetPrimitive(V_domain, V_boundary);
          visc_numerics->SetPrimVarGradient(nodes->GetGradient_Primitive(iPoint), nodes->GetGradient_Primitive(iPoint));

          /*--- Secondary variables ---*/

          S_domain = ideal_gas ? nullptr : nodes->GetSecondary(iPoint);

          /*--- Compute secondary thermodynamic properties (partial derivatives...) ---*/

          S_boundary[0]= GetFluidModel()->GetdPdrho_e();
          S_boundary[1]= GetFluidModel()->GetdPde_rho();

          S_boundary[2]= GetFluidModel()->GetdTdrho_e();
          S_boundary[3]= GetFluidModel()->GetdTde_rho();

          /*--- Compute secondary thermo-physical properties (partial derivatives...) ---*/

          S_boundary[4]= GetFluidModel()->Getdmudrho_T();
          S_boundary[5]= GetFluidModel()->GetdmudT_rho();

          S_boundary[6]= GetFluidModel()->Getdktdrho_T();
          S_boundary[7]= GetFluidModel()->GetdktdT_rho();

          visc_numerics->SetSecondary(S_domain, S_boundary);

          /*--- Turbulent kinetic energy ---*/

          if (config->GetKind_Turb_Model() == TURB_MODEL::SST)
            visc_numerics->SetTurbKineticEnergy(solver_container[TURB_SOL]->GetNodes()->GetSolution(iPoint,0),
                                                solver_container[TURB_SOL]->GetNodes()->GetSolution(iPoint,0));

          /*--- Compute and update residual ---*/

          auto residual = visc_numerics->ComputeResidual(config);
          LinSysRes.SubtractBlock(iPoint, residual);

          /*--- Jacobian contribution for implicit integration ---*/

          if (implicit)
            Jacobian.SubtractBlock2Diag(iPoint, residual.jacobian_i);

        }
      }
    }
    END_SU2_OMP_FOR
  }

  /*--- Free locally allocated memory ---*/
  delete [] Normal;
  delete [] UnitNormal;
  delete [] turboNormal;
  delete [] turboVelocity;
  delete [] Velocity_e;
  delete [] Velocity_b;
  delete [] Velocity_i;
  delete [] FlowDirMix;

  delete [] S_boundary;
  delete [] Lambda_i;
  delete [] u_i;
  delete [] u_e;
  delete [] u_b;
  delete [] dw;

  delete [] Residual;

  for (iVar = 0; iVar < nVar; iVar++)
  {
    delete [] P_Tensor[iVar];
    delete [] invP_Tensor[iVar];
    delete [] Jacobian_i[iVar];
  }
  delete [] P_Tensor;
  delete [] invP_Tensor;
  delete [] Jacobian_i;

}

void CEulerSolver::PreprocessBC_Giles(CGeometry *geometry, CConfig *config, CNumerics *conv_numerics, unsigned short marker_flag) {
  /* Implementation of Fourier Transformations for non-reflecting BC will come soon */
  su2double cj_inf,cj_out1, cj_out2, Density_i, Pressure_i, *turboNormal, *turboVelocity, *Velocity_i, AverageSoundSpeed;
  su2double *deltaprim, *cj, TwoPiThetaFreq_Pitch, pitch, theta, deltaTheta;
  unsigned short iMarker, iSpan, iMarkerTP, iDim;
  unsigned long  iPoint, kend_max, k, iVertex;
  long freq;
  unsigned short  iZone     = config->GetiZone();
  unsigned short nSpanWiseSections = geometry->GetnSpanWiseSections(marker_flag);
  turboNormal   = new su2double[nDim];
  turboVelocity = new su2double[nDim];
  Velocity_i    = new su2double[nDim];
  deltaprim     = new su2double[nVar];
  cj            = new su2double[nVar];
  complex<su2double> I, expArg;
  static complex<su2double> cktemp_inf, cktemp_out1, cktemp_out2;
  I = complex<su2double>(0.0,1.0);

  kend_max = geometry->GetnFreqSpanMax(marker_flag);
  for (iSpan= 0; iSpan < nSpanWiseSections ; iSpan++){
    for(k=0; k < 2*kend_max+1; k++){
      freq = k - kend_max;
      SU2_OMP_MASTER
      {
        cktemp_inf = complex<su2double>(0.0,0.0);
        cktemp_out1 = complex<su2double>(0.0,0.0);
        cktemp_out2 = complex<su2double>(0.0,0.0);
      } END_SU2_OMP_MASTER
      for (iMarker = 0; iMarker < config->GetnMarker_All(); iMarker++){
        for (iMarkerTP=1; iMarkerTP < config->GetnMarker_Turbomachinery()+1; iMarkerTP++){
          if (config->GetMarker_All_Turbomachinery(iMarker) == iMarkerTP){
            if (config->GetMarker_All_TurbomachineryFlag(iMarker) == marker_flag){
              complex<su2double> cktemp_inf_local{0.,0.},cktemp_out1_local{0.,0.}, cktemp_out2_local{0.,0.};
              SU2_OMP_FOR_DYN(roundUpDiv(geometry->GetnVertexSpan(iMarker,iSpan), 2*omp_get_max_threads()))
              for (iVertex = 0; iVertex < geometry->GetnVertexSpan(iMarker,iSpan); iVertex++) {

                /*--- find the node related to the vertex ---*/
                iPoint = geometry->turbovertex[iMarker][iSpan][iVertex]->GetNode();

                geometry->turbovertex[iMarker][iSpan][iVertex]->GetTurboNormal(turboNormal);
                /*--- Compute the internal state _i ---*/

                Pressure_i = nodes->GetPressure(iPoint);
                Density_i = nodes->GetDensity(iPoint);
                for (iDim = 0; iDim < nDim; iDim++)
                {
                  Velocity_i[iDim] = nodes->GetVelocity(iPoint,iDim);
                }

                ComputeTurboVelocity(Velocity_i, turboNormal, turboVelocity, marker_flag, config->GetKind_TurboMachinery(iZone));

                if(nDim ==2){
                  deltaprim[0] = Density_i - AverageDensity[iMarker][iSpan];
                  deltaprim[1] = turboVelocity[0] - AverageTurboVelocity[iMarker][iSpan][0];
                  deltaprim[2] = turboVelocity[1] - AverageTurboVelocity[iMarker][iSpan][1];
                  deltaprim[3] = Pressure_i - AveragePressure[iMarker][iSpan];
                }
                else{
                  //Here 3d
                  deltaprim[0] = Density_i - AverageDensity[iMarker][iSpan];
                  deltaprim[1] = turboVelocity[0] - AverageTurboVelocity[iMarker][iSpan][0];
                  deltaprim[2] = turboVelocity[1] - AverageTurboVelocity[iMarker][iSpan][1];
                  deltaprim[3] = turboVelocity[2] - AverageTurboVelocity[iMarker][iSpan][2]; //New char
                  deltaprim[4] = Pressure_i - AveragePressure[iMarker][iSpan];
                }

                GetFluidModel()->SetTDState_Prho(AveragePressure[iMarker][iSpan], AverageDensity[iMarker][iSpan]);
                AverageSoundSpeed = GetFluidModel()->GetSoundSpeed();
                conv_numerics->GetCharJump(AverageSoundSpeed, AverageDensity[iMarker][iSpan], deltaprim, cj);

                /*-----this is only valid 2D ----*/
                if(nDim ==2){
                  cj_out1 = cj[1];
                  cj_out2 = cj[2];
                  cj_inf  = cj[3];
                }
                else{
                  //Here 3D
                  cj_out1 = cj[1];
                  cj_out2 = cj[3];
                  cj_inf  = cj[4];
                }
                pitch      = geometry->GetMaxAngularCoord(iMarker, iSpan) - geometry->GetMinAngularCoord(iMarker,iSpan);
                theta      = geometry->turbovertex[iMarker][iSpan][iVertex]->GetRelAngularCoord();
                deltaTheta = geometry->turbovertex[iMarker][iSpan][iVertex]->GetDeltaAngularCoord();
                TwoPiThetaFreq_Pitch = 2*PI_NUMBER*freq*theta/pitch;

                expArg = complex<su2double>(cos(TwoPiThetaFreq_Pitch)) - I*complex<su2double>(sin(TwoPiThetaFreq_Pitch));
                if (freq != 0){
                  cktemp_out1_local +=  cj_out1*expArg*deltaTheta/pitch;
                  cktemp_out2_local +=  cj_out2*expArg*deltaTheta/pitch;
                  cktemp_inf_local  +=  cj_inf*expArg*deltaTheta/pitch;
                }
                else{
                  cktemp_inf_local += complex<su2double>(0.0,0.0);
                  cktemp_out1_local += complex<su2double>(0.0,0.0);
                  cktemp_out2_local += complex<su2double>(0.0,0.0);
                }
              }
              END_SU2_OMP_FOR
              SU2_OMP_CRITICAL
              {
                cktemp_inf += cktemp_inf_local;
                cktemp_out1 += cktemp_out1_local;
                cktemp_out2 += cktemp_out2_local;
              } END_SU2_OMP_CRITICAL
            }
          }
        }
      }

      BEGIN_SU2_OMP_SAFE_GLOBAL_ACCESS
      {
#ifdef HAVE_MPI
        su2double MyRe_inf = cktemp_inf.real(); su2double Re_inf = 0.0;
        su2double MyIm_inf = cktemp_inf.imag(); su2double Im_inf = 0.0;
        cktemp_inf = complex<su2double>(0.0,0.0);

        su2double MyRe_out1 = cktemp_out1.real(); su2double Re_out1 = 0.0;
        su2double MyIm_out1 = cktemp_out1.imag(); su2double Im_out1 = 0.0;
        cktemp_out1 = complex<su2double>(0.0,0.0);

        su2double MyRe_out2 = cktemp_out2.real(); su2double Re_out2 = 0.0;
        su2double MyIm_out2 = cktemp_out2.imag(); su2double Im_out2 = 0.0;
        cktemp_out2 = complex<su2double>(0.0,0.0);


        SU2_MPI::Allreduce(&MyRe_inf, &Re_inf, 1, MPI_DOUBLE, MPI_SUM, SU2_MPI::GetComm());
        SU2_MPI::Allreduce(&MyIm_inf, &Im_inf, 1, MPI_DOUBLE, MPI_SUM, SU2_MPI::GetComm());
        SU2_MPI::Allreduce(&MyRe_out1, &Re_out1, 1, MPI_DOUBLE, MPI_SUM, SU2_MPI::GetComm());
        SU2_MPI::Allreduce(&MyIm_out1, &Im_out1, 1, MPI_DOUBLE, MPI_SUM, SU2_MPI::GetComm());
        SU2_MPI::Allreduce(&MyRe_out2, &Re_out2, 1, MPI_DOUBLE, MPI_SUM, SU2_MPI::GetComm());
        SU2_MPI::Allreduce(&MyIm_out2, &Im_out2, 1, MPI_DOUBLE, MPI_SUM, SU2_MPI::GetComm());

        cktemp_inf = complex<su2double>(Re_inf,Im_inf);
        cktemp_out1 = complex<su2double>(Re_out1,Im_out1);
        cktemp_out2 = complex<su2double>(Re_out2,Im_out2);
#endif

        for (iMarker = 0; iMarker < config->GetnMarker_All(); iMarker++){
          for (iMarkerTP=1; iMarkerTP < config->GetnMarker_Turbomachinery()+1; iMarkerTP++){
            if (config->GetMarker_All_Turbomachinery(iMarker) == iMarkerTP){
              if (config->GetMarker_All_TurbomachineryFlag(iMarker) == marker_flag){
                /*-----this is only valid 2D ----*/
                if (marker_flag == INFLOW){
                  CkInflow[iMarker][iSpan][k]= cktemp_inf;
                }else{
                  CkOutflow1[iMarker][iSpan][k]=cktemp_out1;
                  CkOutflow2[iMarker][iSpan][k]=cktemp_out2;
                }
              }
            }
          }
        }
      } END_SU2_OMP_SAFE_GLOBAL_ACCESS
    }
  }

  delete [] turboVelocity;
  delete [] turboNormal;
  delete [] Velocity_i;
  delete [] deltaprim;
  delete [] cj;

}

void CEulerSolver::BC_Giles(CGeometry *geometry, CSolver **solver_container, CNumerics *conv_numerics,
                            CNumerics *visc_numerics, CConfig *config, unsigned short val_marker) {
  SU2_ZONE_SCOPED

  unsigned short iDim, iVar, jVar, iSpan;
  unsigned long  iPoint, Point_Normal, oldVertex, k, kend, kend_max, iVertex;
  su2double  *UnitNormal, *turboVelocity, *turboNormal;

  su2double *Velocity_b, Velocity2_b, Enthalpy_b, Energy_b, Density_b, Pressure_b, Temperature_b;
  su2double *Velocity_i, Velocity2_i, Energy_i, StaticEnergy_i, Density_i, Pressure_i;
  su2double Pressure_e;
  su2double *V_boundary, *V_domain, *S_boundary, *S_domain;
  unsigned short  iZone     = config->GetiZone();
  bool implicit             = (config->GetKind_TimeIntScheme() == EULER_IMPLICIT);
  string Marker_Tag         = config->GetMarker_All_TagBound(val_marker);
  bool viscous              = config->GetViscous();
  const bool ideal_gas = config->GetKind_FluidModel() == STANDARD_AIR ||
                         config->GetKind_FluidModel() == IDEAL_GAS;
  unsigned short nSpanWiseSections = geometry->GetnSpanWiseSections(config->GetMarker_All_TurbomachineryFlag(val_marker));
  su2double relfacAvgCfg       = config->GetGiles_RelaxFactorAverage(Marker_Tag);
  su2double relfacFouCfg       = config->GetGiles_RelaxFactorFourier(Marker_Tag);
  su2double *Normal;
  su2double TwoPiThetaFreq_Pitch, pitch,theta;
  const su2double *SpanWiseValues = nullptr, *FlowDir;
  su2double spanPercent, extrarelfacAvg = 0.0, deltaSpan = 0.0, relfacAvg, relfacFou, coeffrelfacAvg = 0.0;
  unsigned short Turbo_Flag;

  Normal                = new su2double[nDim];
  turboNormal           = new su2double[nDim];
  UnitNormal            = new su2double[nDim];
  turboVelocity         = new su2double[nDim];
  Velocity_i            = new su2double[nDim];
  Velocity_b            = new su2double[nDim];


  su2double AverageSoundSpeed, *AverageTurboMach, AverageEntropy, AverageEnthalpy;
  AverageTurboMach = new su2double[nDim];
  S_boundary       = new su2double[8];

  su2double  AvgMach , *cj, GilesBeta, *delta_c, **R_Matrix, *deltaprim, **R_c_inv,**R_c, alphaIn_BC, gammaIn_BC = 0,
      P_Total, T_Total, Enthalpy_BC, Entropy_BC, *R, *c_avg,*dcjs, Beta_inf2, c2js_Re, c3js_Re, cOutjs_Re, avgVel2 =0.0;

  long freq;

  delta_c       = new su2double[nVar];
  deltaprim     = new su2double[nVar];
  cj            = new su2double[nVar];
  R_Matrix      = new su2double*[nVar];
  R_c           = new su2double*[nVar-1];
  R_c_inv       = new su2double*[nVar-1];
  R             = new su2double[nVar-1];
  c_avg         = new su2double[nVar];
  dcjs          = new su2double[nVar];

  for (iVar = 0; iVar < nVar; iVar++)
  {
    R_Matrix[iVar] = new su2double[nVar];
    c_avg[iVar]    =  0.0;
    dcjs[iVar]     =  0.0;
  }
  for (iVar = 0; iVar < nVar-1; iVar++)
  {
    R_c[iVar] = new su2double[nVar-1];
    R_c_inv[iVar] = new su2double[nVar-1];
  }


  complex<su2double> I, c2ks, c2js, c3ks, c3js, cOutks, cOutjs, Beta_inf;
  I = complex<su2double>(0.0,1.0);

  /*--- Compute coeff for under relaxation of Avg and Fourier Coefficient for hub and shroud---*/
  if (nDim == 3){
    extrarelfacAvg  = config->GetExtraRelFacGiles(0);
    spanPercent     = config->GetExtraRelFacGiles(1);
    Turbo_Flag      = config->GetMarker_All_TurbomachineryFlag(val_marker);
    SpanWiseValues  = geometry->GetSpanWiseValue(Turbo_Flag);
    deltaSpan       = SpanWiseValues[nSpanWiseSections-1]*spanPercent;
    coeffrelfacAvg  = (relfacAvgCfg - extrarelfacAvg)/deltaSpan;
  }
  for (iSpan= 0; iSpan < nSpanWiseSections ; iSpan++){
    /*--- Compute under relaxation for the Hub and Shroud Avg and Fourier Coefficient---*/
    if(nDim == 3){
      if(SpanWiseValues[iSpan] <= SpanWiseValues[0] + deltaSpan){
        relfacAvg = extrarelfacAvg + coeffrelfacAvg*(SpanWiseValues[iSpan] - SpanWiseValues[0]);
        relfacFou = 0.0;
      }
      else if(SpanWiseValues[iSpan] >= SpanWiseValues[nSpanWiseSections -1] - deltaSpan){
        relfacAvg = extrarelfacAvg - coeffrelfacAvg*(SpanWiseValues[iSpan] - SpanWiseValues[nSpanWiseSections -1]);
        relfacFou = 0.0;
      }
      else{
        relfacAvg = relfacAvgCfg;
        relfacFou = relfacFouCfg;
      }
    }
    else{
      {
        relfacAvg = relfacAvgCfg;
        relfacFou = relfacFouCfg;
      }
    }

    GetFluidModel()->SetTDState_Prho(AveragePressure[val_marker][iSpan], AverageDensity[val_marker][iSpan]);
    AverageSoundSpeed = GetFluidModel()->GetSoundSpeed();
    AverageTurboMach[0] = AverageTurboVelocity[val_marker][iSpan][0]/AverageSoundSpeed;
    AverageTurboMach[1] = AverageTurboVelocity[val_marker][iSpan][1]/AverageSoundSpeed;

    if(dynamic_grid){
      AverageTurboMach[1] = AverageRelTangVelocity[val_marker][iSpan]/AverageSoundSpeed;
    }

    AvgMach = AverageTurboMach[0]*AverageTurboMach[0] + AverageTurboMach[1]*AverageTurboMach[1];

    kend     = geometry->GetnFreqSpan(val_marker, iSpan);
    kend_max = geometry->GetnFreqSpanMax(config->GetMarker_All_TurbomachineryFlag(val_marker));
    conv_numerics->GetRMatrix(AverageSoundSpeed, AverageDensity[val_marker][iSpan], R_Matrix);

    su2double donorAverages[5] = {0.0};
    switch (config->GetKind_Data_Giles(Marker_Tag)){
      case MIXING_IN: case MIXING_IN_1D: case MIXING_OUT: case MIXING_OUT_1D:
        for (auto mixVar = 0u; mixVar < 5; mixVar++) donorAverages[mixVar] = GetMixingState(val_marker, iSpan, mixVar);
        break;
      default:
        break;
    }
    BEGIN_SU2_OMP_SAFE_GLOBAL_ACCESS {
      ExtAverageDensity = donorAverages[0];
      ExtAveragePressure = donorAverages[1];
      ExtAverageTurboVelocity[0] = donorAverages[2];
      ExtAverageTurboVelocity[1] = donorAverages[3];
      if (nDim == 3) ExtAverageTurboVelocity[2] = donorAverages[4];
    } END_SU2_OMP_SAFE_GLOBAL_ACCESS

    switch(config->GetKind_Data_Giles(Marker_Tag)){

    case TOTAL_CONDITIONS_PT:

      /*--- Retrieve the specified total conditions for this inlet. ---*/
      P_Total  = config->GetGiles_Var1(Marker_Tag);
      T_Total  = config->GetGiles_Var2(Marker_Tag);
      FlowDir = config->GetGiles_FlowDir(Marker_Tag);
      alphaIn_BC = atan(FlowDir[1]/FlowDir[0]);

      gammaIn_BC = 0;
      if (nDim == 3){
        gammaIn_BC = FlowDir[2]; //atan(FlowDir[2]/FlowDir[0]);
      }

      /*--- Non-dim. the inputs---*/
      P_Total /= config->GetPressure_Ref();
      T_Total /= config->GetTemperature_Ref();

      /* --- Computes the total state --- */
      GetFluidModel()->SetTDState_PT(P_Total, T_Total);
      Enthalpy_BC = GetFluidModel()->GetStaticEnergy()+ GetFluidModel()->GetPressure()/GetFluidModel()->GetDensity();
      Entropy_BC = GetFluidModel()->GetEntropy();


      /* --- Computes the inverse matrix R_c --- */
      conv_numerics->ComputeResJacobianGiles(GetFluidModel(), AveragePressure[val_marker][iSpan], AverageDensity[val_marker][iSpan],
                                             AverageTurboVelocity[val_marker][iSpan], alphaIn_BC, gammaIn_BC, R_c, R_c_inv);

      GetFluidModel()->SetTDState_Prho(AveragePressure[val_marker][iSpan], AverageDensity[val_marker][iSpan]);
      AverageEnthalpy = GetFluidModel()->GetStaticEnergy() + AveragePressure[val_marker][iSpan]/AverageDensity[val_marker][iSpan];
      AverageEntropy  = GetFluidModel()->GetEntropy();

      avgVel2 = 0.0;
      for (iDim = 0; iDim < nDim; iDim++) avgVel2 += AverageVelocity[val_marker][iSpan][iDim]*AverageVelocity[val_marker][iSpan][iDim];
      if (nDim == 2){
        R[0] = -(AverageEntropy - Entropy_BC);
        R[1] = -(AverageTurboVelocity[val_marker][iSpan][1] - tan(alphaIn_BC)*AverageTurboVelocity[val_marker][iSpan][0]);
        R[2] = -(AverageEnthalpy + 0.5*avgVel2 - Enthalpy_BC);
      }

      else{
        R[0] = -(AverageEntropy - Entropy_BC);
        R[1] = -(AverageTurboVelocity[val_marker][iSpan][1] - tan(alphaIn_BC)*AverageTurboVelocity[val_marker][iSpan][0]);
        R[2] = -(AverageTurboVelocity[val_marker][iSpan][2] - tan(gammaIn_BC)*AverageTurboVelocity[val_marker][iSpan][0]);
        R[3] = -(AverageEnthalpy + 0.5*avgVel2 - Enthalpy_BC);

      }
      /* --- Compute the avg component  c_avg = R_c^-1 * R --- */
      for (iVar = 0; iVar < nVar-1; iVar++){
        c_avg[iVar] = 0.0;
        for (jVar = 0; jVar < nVar-1; jVar++){
          c_avg[iVar] += R_c_inv[iVar][jVar]*R[jVar];
        }
      }
      break;

    case TOTAL_CONDITIONS_PT_1D:

      /*--- Retrieve the specified total conditions for this inlet. ---*/
      P_Total  = config->GetGiles_Var1(Marker_Tag);
      T_Total  = config->GetGiles_Var2(Marker_Tag);
      FlowDir = config->GetGiles_FlowDir(Marker_Tag);
      alphaIn_BC = atan(FlowDir[1]/FlowDir[0]);

      gammaIn_BC = 0;
      if (nDim == 3){
        // Review definition of angle
        gammaIn_BC = FlowDir[2]; //atan(FlowDir[2]/FlowDir[0]);
      }

      /*--- Non-dim. the inputs---*/
      P_Total /= config->GetPressure_Ref();
      T_Total /= config->GetTemperature_Ref();

      /* --- Computes the total state --- */
      GetFluidModel()->SetTDState_PT(P_Total, T_Total);
      Enthalpy_BC = GetFluidModel()->GetStaticEnergy()+ GetFluidModel()->GetPressure()/GetFluidModel()->GetDensity();
      Entropy_BC = GetFluidModel()->GetEntropy();


      /* --- Computes the inverse matrix R_c --- */
      conv_numerics->ComputeResJacobianGiles(GetFluidModel(), AveragePressure[val_marker][iSpan], AverageDensity[val_marker][iSpan],
                                             AverageTurboVelocity[val_marker][iSpan], alphaIn_BC, gammaIn_BC, R_c, R_c_inv);

      GetFluidModel()->SetTDState_Prho(AveragePressure[val_marker][nSpanWiseSections], AverageDensity[val_marker][nSpanWiseSections]);
      AverageEnthalpy = GetFluidModel()->GetStaticEnergy() + AveragePressure[val_marker][nSpanWiseSections]/AverageDensity[val_marker][nSpanWiseSections];
      AverageEntropy  = GetFluidModel()->GetEntropy();


      avgVel2 = 0.0;
      for (iDim = 0; iDim < nDim; iDim++) avgVel2 += AverageVelocity[val_marker][iSpan][iDim]*AverageVelocity[val_marker][iSpan][iDim];
      if (nDim == 2){
        R[0] = -(AverageEntropy - Entropy_BC);
        R[1] = -(AverageTurboVelocity[val_marker][nSpanWiseSections][1] - tan(alphaIn_BC)*AverageTurboVelocity[val_marker][nSpanWiseSections][0]);
        R[2] = -(AverageEnthalpy + 0.5*avgVel2 - Enthalpy_BC);
      }

      else{
        R[0] = -(AverageEntropy - Entropy_BC);
        R[1] = -(AverageTurboVelocity[val_marker][nSpanWiseSections][1] - tan(alphaIn_BC)*AverageTurboVelocity[val_marker][nSpanWiseSections][0]);
        R[2] = -(AverageTurboVelocity[val_marker][nSpanWiseSections][2] - tan(gammaIn_BC)*AverageTurboVelocity[val_marker][nSpanWiseSections][0]);
        R[3] = -(AverageEnthalpy + 0.5*avgVel2 - Enthalpy_BC);

      }
      /* --- Compute the avg component  c_avg = R_c^-1 * R --- */
      for (iVar = 0; iVar < nVar-1; iVar++){
        c_avg[iVar] = 0.0;
        for (jVar = 0; jVar < nVar-1; jVar++){
          c_avg[iVar] += R_c_inv[iVar][jVar]*R[jVar];
        }
      }
      break;

    case MIXING_IN: case MIXING_OUT:

      /* --- Compute average jump of primitive at the mixing-plane interface--- */
      deltaprim[0] = ExtAverageDensity - AverageDensity[val_marker][iSpan];
      deltaprim[1] = ExtAverageTurboVelocity[0] - AverageTurboVelocity[val_marker][iSpan][0];
      deltaprim[2] = ExtAverageTurboVelocity[1] - AverageTurboVelocity[val_marker][iSpan][1];
      if (nDim == 2){
        deltaprim[3] = ExtAveragePressure - AveragePressure[val_marker][iSpan];
      }
      else
      {
        deltaprim[3] = ExtAverageTurboVelocity[2] - AverageTurboVelocity[val_marker][iSpan][2];
        deltaprim[4] = ExtAveragePressure - AveragePressure[val_marker][iSpan];
      }


      /* --- Compute average jump of charachteristic variable at the mixing-plane interface--- */
      GetFluidModel()->SetTDState_Prho(AveragePressure[val_marker][iSpan], AverageDensity[val_marker][iSpan]);
      AverageSoundSpeed = GetFluidModel()->GetSoundSpeed();
      conv_numerics->GetCharJump(AverageSoundSpeed, AverageDensity[val_marker][iSpan], deltaprim, c_avg);
      break;

    case MIXING_IN_1D: case MIXING_OUT_1D:

      /* --- Compute average jump of primitive at the mixing-plane interface--- */
      deltaprim[0] = ExtAverageDensity - AverageDensity[val_marker][nSpanWiseSections];
      deltaprim[1] = ExtAverageTurboVelocity[0] - AverageTurboVelocity[val_marker][nSpanWiseSections][0];
      deltaprim[2] = ExtAverageTurboVelocity[1] - AverageTurboVelocity[val_marker][nSpanWiseSections][1];
      if (nDim == 2){
        deltaprim[3] = ExtAveragePressure - AveragePressure[val_marker][nSpanWiseSections];
      }
      else
      {
        deltaprim[3] = ExtAverageTurboVelocity[2] - AverageTurboVelocity[val_marker][nSpanWiseSections][2];
        deltaprim[4] = ExtAveragePressure - AveragePressure[val_marker][nSpanWiseSections];
      }

      /* --- Compute average jump of charachteristic variable at the mixing-plane interface--- */
      GetFluidModel()->SetTDState_Prho(AveragePressure[val_marker][nSpanWiseSections], AverageDensity[val_marker][nSpanWiseSections]);
      AverageSoundSpeed = GetFluidModel()->GetSoundSpeed();
      conv_numerics->GetCharJump(AverageSoundSpeed, AverageDensity[val_marker][nSpanWiseSections], deltaprim, c_avg);
      break;


    case STATIC_PRESSURE:
      Pressure_e = config->GetGiles_Var1(Marker_Tag);
      Pressure_e /= config->GetPressure_Ref();

      /* --- Compute avg characteristic jump  --- */
      if (nDim == 2){
        c_avg[3] = -2.0*(AveragePressure[val_marker][iSpan]-Pressure_e);
      }
      else
      {
        c_avg[4] = -2.0*(AveragePressure[val_marker][iSpan]-Pressure_e);
      }
      break;

    case STATIC_PRESSURE_1D:
      Pressure_e = config->GetGiles_Var1(Marker_Tag);
      Pressure_e /= config->GetPressure_Ref();

      /* --- Compute avg characteristic jump  --- */
      if (nDim == 2){
        c_avg[3] = -2.0*(AveragePressure[val_marker][nSpanWiseSections]-Pressure_e);
      }
      else
      {
        c_avg[4] = -2.0*(AveragePressure[val_marker][nSpanWiseSections]-Pressure_e);
      }
      break;

    case RADIAL_EQUILIBRIUM:
      Pressure_e = RadialEquilibriumPressure[val_marker][iSpan];

      /* --- Compute avg characteristic jump  --- */
      c_avg[4] = -2.0*(AveragePressure[val_marker][iSpan]-Pressure_e);

      break;

    case MASS_FLOW_OUTLET:
      auto const MassFlowRate_e = config->GetGiles_Var1(Marker_Tag);
      auto const relFacMassFlowRate = config->GetGiles_Var2(Marker_Tag);

      Pressure_e = AveragePressure[val_marker][nSpanWiseSections]+relFacMassFlowRate*GetFluidModel()->GetdPdrho_e()*(AverageMassFlowRate[val_marker]-MassFlowRate_e);

      /*--- Compute avg characteristic jump  ---*/
      c_avg[nDim + 1] = -2.0*(AveragePressure[val_marker][iSpan]-Pressure_e);
      break;

    }

    /*--- Loop over all the vertices on this boundary marker ---*/

    SU2_OMP_FOR_DYN(OMP_MIN_SIZE)
    for (iVertex = 0; iVertex < geometry->GetnVertexSpan(val_marker,iSpan); iVertex++) {

      /*--- using the other vertex information for retrieving some information ---*/
      oldVertex = geometry->turbovertex[val_marker][iSpan][iVertex]->GetOldVertex();
      V_boundary= GetCharacPrimVar(val_marker, oldVertex);

      /*--- Index of the closest interior node ---*/
      Point_Normal = geometry->vertex[val_marker][oldVertex]->GetNormal_Neighbor();

      /*--- Normal vector for this vertex (negate for outward convention),
       *    this normal is scaled with the area of the face of the element  ---*/
      geometry->vertex[val_marker][oldVertex]->GetNormal(Normal);
      for (iDim = 0; iDim < nDim; iDim++) Normal[iDim] = -Normal[iDim];
      conv_numerics->SetNormal(Normal);

      /*--- find the node related to the vertex ---*/
      iPoint = geometry->turbovertex[val_marker][iSpan][iVertex]->GetNode();

      /*--- Normalize Normal vector for this vertex (already for outward convention) ---*/
      geometry->turbovertex[val_marker][iSpan][iVertex]->GetNormal(UnitNormal);
      geometry->turbovertex[val_marker][iSpan][iVertex]->GetTurboNormal(turboNormal);

      /*--- Retrieve solution at this boundary node ---*/
      V_domain = nodes->GetPrimitive(iPoint);

      /*--- Retrieve domain Secondary variables ---*/
      S_domain = ideal_gas? nullptr : nodes->GetSecondary(iPoint);

      /*--- Compute the internal state u_i ---*/
      Velocity2_i = 0;
      for (iDim = 0; iDim < nDim; iDim++)
      {
        Velocity_i[iDim] = nodes->GetVelocity(iPoint,iDim);
        Velocity2_i += Velocity_i[iDim]*Velocity_i[iDim];
      }


      Density_i = nodes->GetDensity(iPoint);

      Energy_i = nodes->GetEnergy(iPoint);
      StaticEnergy_i = Energy_i - 0.5*Velocity2_i;
      /*--- SST: the total energy contains k. ---*/
      if (config->GetKind_Turb_Model() == TURB_MODEL::SST)
        StaticEnergy_i -= solver_container[TURB_SOL]->GetNodes()->GetSolution(iPoint, 0);

      GetFluidModel()->SetTDState_rhoe(Density_i, StaticEnergy_i);

      Pressure_i = GetFluidModel()->GetPressure();

      ComputeTurboVelocity(Velocity_i, turboNormal, turboVelocity, config->GetMarker_All_TurbomachineryFlag(val_marker),config->GetKind_TurboMachinery(iZone));
      if (nDim == 2){
        deltaprim[0] = Density_i - AverageDensity[val_marker][iSpan];
        deltaprim[1] = turboVelocity[0] - AverageTurboVelocity[val_marker][iSpan][0];
        deltaprim[2] = turboVelocity[1] - AverageTurboVelocity[val_marker][iSpan][1];
        deltaprim[3] = Pressure_i - AveragePressure[val_marker][iSpan];
      }
      else{
        deltaprim[0] = Density_i - AverageDensity[val_marker][iSpan];
        deltaprim[1] = turboVelocity[0] - AverageTurboVelocity[val_marker][iSpan][0];
        deltaprim[2] = turboVelocity[1] - AverageTurboVelocity[val_marker][iSpan][1];
        deltaprim[3] = turboVelocity[2] - AverageTurboVelocity[val_marker][iSpan][2];
        deltaprim[4] = Pressure_i - AveragePressure[val_marker][iSpan];
      }

      GetFluidModel()->SetTDState_Prho(AveragePressure[val_marker][iSpan], AverageDensity[val_marker][iSpan]);
      AverageSoundSpeed = GetFluidModel()->GetSoundSpeed();
      conv_numerics->GetCharJump(AverageSoundSpeed, AverageDensity[val_marker][iSpan], deltaprim, cj);


      pitch      = geometry->GetMaxAngularCoord(val_marker, iSpan) - geometry->GetMinAngularCoord(val_marker,iSpan);
      theta      = geometry->turbovertex[val_marker][iSpan][iVertex]->GetRelAngularCoord();

      switch(config->GetKind_Data_Giles(Marker_Tag))
      {

      //Done, generalize for 3D case
      //TODO(turbo), generalize for Inlet and Outlet in for backflow treatment

      case TOTAL_CONDITIONS_PT: case MIXING_IN:case TOTAL_CONDITIONS_PT_1D: case MIXING_IN_1D:
        if(config->GetSpatialFourier()){
          if (AvgMach <= 1.0){
            Beta_inf= I*complex<su2double>(sqrt(1.0 - AvgMach));
            c2js = complex<su2double>(0.0,0.0);
            c3js = complex<su2double>(0.0,0.0);
            for(k=0; k < 2*kend_max+1; k++){
              freq = k - kend_max;
              if(freq >= (long)(-kend) && freq <= (long)(kend) && AverageTurboMach[0] > config->GetAverageMachLimit()){
                TwoPiThetaFreq_Pitch = 2*PI_NUMBER*freq*theta/pitch;

                c2ks = -CkInflow[val_marker][iSpan][k]*complex<su2double>(Beta_inf + AverageTurboMach[1])/complex<su2double>( 1.0 + AverageTurboMach[0]);
                c3ks =  CkInflow[val_marker][iSpan][k]*complex<su2double>(Beta_inf + AverageTurboMach[1])/complex<su2double>( 1.0 + AverageTurboMach[0]);
                c3ks *= complex<su2double>(Beta_inf + AverageTurboMach[1])/complex<su2double>( 1.0 + AverageTurboMach[0]);
                c2js += c2ks*(complex<su2double>(cos(TwoPiThetaFreq_Pitch))+I*complex<su2double>(sin(TwoPiThetaFreq_Pitch)));
                c3js += c3ks*(complex<su2double>(cos(TwoPiThetaFreq_Pitch))+I*complex<su2double>(sin(TwoPiThetaFreq_Pitch)));
              }
              else{
                c2js += complex<su2double>(0.0,0.0);
                c3js += complex<su2double>(0.0,0.0);
              }
            }
            c2js_Re = c2js.real();
            c3js_Re = c3js.real();

            if (nDim == 2){
              dcjs[0] = 0.0     - cj[0];
              dcjs[1] = c2js_Re - cj[1];
              dcjs[2] = c3js_Re - cj[2];
            }else{
              dcjs[0] = 0.0     - cj[0];
              dcjs[1] = c2js_Re - cj[1];
              dcjs[2] = 0.0     - cj[2];
              dcjs[3] = c3js_Re - cj[3];
            }

          }else{
            if (AverageTurboVelocity[val_marker][iSpan][1] >= 0.0){
              Beta_inf2= -sqrt(AvgMach - 1.0);
            }else{
              Beta_inf2= sqrt(AvgMach-1.0);
            }
            if (nDim == 2){
              c2js_Re = -cj[3]*(Beta_inf2 + AverageTurboMach[1])/( 1.0 + AverageTurboMach[0]);
              c3js_Re = cj[3]*(Beta_inf2 + AverageTurboMach[1])/( 1.0 + AverageTurboMach[0]);
              c3js_Re *= (Beta_inf2 + AverageTurboMach[1])/( 1.0 + AverageTurboMach[0]);
            }else{
              c2js_Re = -cj[4]*(Beta_inf2 + AverageTurboMach[1])/( 1.0 + AverageTurboMach[0]);
              c3js_Re = cj[4]*(Beta_inf2 + AverageTurboMach[1])/( 1.0 + AverageTurboMach[0]);
              c3js_Re *= (Beta_inf2 + AverageTurboMach[1])/( 1.0 + AverageTurboMach[0]);
            }


            if (nDim == 2){
              dcjs[0] = 0.0     - cj[0];
              dcjs[1] = c2js_Re - cj[1];
              dcjs[2] = c3js_Re - cj[2];
            }else{
              dcjs[0] = 0.0     - cj[0];
              dcjs[1] = c2js_Re - cj[1];
              dcjs[2] = 0.0     - cj[2];
              dcjs[3] = c3js_Re - cj[3];
            }
          }
        }
        else{
          if (nDim == 2){
            dcjs[0] = 0.0;
            dcjs[1] = 0.0;
            dcjs[2] = 0.0;
          }else{
            dcjs[0] = 0.0;
            dcjs[1] = 0.0;
            dcjs[2] = 0.0;
            dcjs[3] = 0.0;
          }
        }
        /* --- Impose Inlet BC Reflecting--- */
        delta_c[0] = relfacAvg*c_avg[0] + relfacFou*dcjs[0];
        delta_c[1] = relfacAvg*c_avg[1] + relfacFou*dcjs[1];
        delta_c[2] = relfacAvg*c_avg[2] + relfacFou*dcjs[2];
        if (nDim == 2){
          delta_c[3] = cj[3];
        }else{
          delta_c[3] = relfacAvg*c_avg[3] + relfacFou*dcjs[3];
          delta_c[4] = cj[4];
        }
        break;


      case STATIC_PRESSURE:case STATIC_PRESSURE_1D:case MIXING_OUT:case RADIAL_EQUILIBRIUM:case MIXING_OUT_1D: case MASS_FLOW_OUTLET:

        /* --- implementation of Giles BC---*/
        if(config->GetSpatialFourier()){
          if (AvgMach > 1.0){
            /* --- supersonic Giles implementation ---*/
            if (AverageTurboVelocity[val_marker][iSpan][1] >= 0.0){
              GilesBeta= -sqrt(AvgMach - 1.0);

            }else{
              GilesBeta= sqrt(AvgMach - 1.0);
            }
            if(nDim == 2){
              cOutjs_Re= (2.0 * AverageTurboMach[0])/(GilesBeta - AverageTurboMach[1])*cj[1] - (GilesBeta + AverageTurboMach[1])/(GilesBeta - AverageTurboMach[1])*cj[2];
            }
            else{
              cOutjs_Re= (2.0 * AverageTurboMach[0])/(GilesBeta - AverageTurboMach[1])*cj[1] - (GilesBeta + AverageTurboMach[1])/(GilesBeta - AverageTurboMach[1])*cj[3];
            }
            if (nDim == 2){
              dcjs[3] = cOutjs_Re - cj[3];
            }
            else{
              dcjs[4] = cOutjs_Re - cj[4];
            }
          }else{

            /* --- subsonic Giles implementation ---*/
            Beta_inf= I*complex<su2double>(sqrt(1.0  - AvgMach));
            cOutjs  = complex<su2double>(0.0,0.0);
            for(k=0; k < 2*kend_max+1; k++){
              freq = k - kend_max;
              if(freq >= (long)(-kend) && freq <= (long)(kend) && AverageTurboMach[0] > config->GetAverageMachLimit()){
                TwoPiThetaFreq_Pitch = 2*PI_NUMBER*freq*theta/pitch;
                cOutks  = complex<su2double>(2.0 * AverageTurboMach[0])/complex<su2double>(Beta_inf - AverageTurboMach[1])*CkOutflow1[val_marker][iSpan][k];
                cOutks -= complex<su2double>(Beta_inf + AverageTurboMach[1])/complex<su2double>(Beta_inf - AverageTurboMach[1])*CkOutflow2[val_marker][iSpan][k];

                cOutjs += cOutks*(complex<su2double>(cos(TwoPiThetaFreq_Pitch)) + I*complex<su2double>(sin(TwoPiThetaFreq_Pitch)));
              }
              else{
                cOutjs +=complex<su2double>(0.0,0.0);
              }
            }
            cOutjs_Re = cOutjs.real();

            if (nDim == 2){
              dcjs[3] = cOutjs_Re - cj[3];
            }
            else{
              dcjs[4] = cOutjs_Re - cj[4];
            }
          }
        }
        else{
          if (nDim == 2){
            dcjs[3] = 0.0;
          }
          else{
            dcjs[4] = 0.0;
          }
        }
        /* --- Impose Outlet BC Non-Reflecting  --- */
        delta_c[0] = cj[0];
        delta_c[1] = cj[1];
        delta_c[2] = cj[2];
        if (nDim == 2){
          delta_c[3] = relfacAvg*c_avg[3] + relfacFou*dcjs[3];
        }
        else{
          delta_c[3] = cj[3];
          delta_c[4] = relfacAvg*c_avg[4] + relfacFou*dcjs[4];
        }


        /*--- Automatically impose supersonic autoflow ---*/
        if (abs(AverageTurboMach[0]) > 1.0000){
          delta_c[0] = 0.0;
          delta_c[1] = 0.0;
          delta_c[2] = 0.0;
          delta_c[2] = 0.0;
          if (nDim == 3)delta_c[4] = 0.0;
        }

        break;

      default:
        SU2_MPI::Error("Invalid Giles input!", CURRENT_FUNCTION);
        break;
      }

      /*--- Compute primitive jump from characteristic variables  ---*/
      for (iVar = 0; iVar < nVar; iVar++)
      {
        deltaprim[iVar]=0.0;
        for (jVar = 0; jVar < nVar; jVar++)
        {
          deltaprim[iVar] +=  R_Matrix[iVar][jVar]*delta_c[jVar];
        }
      }

      /*--- retrieve boundary variables ---*/
      Density_b = AverageDensity[val_marker][iSpan] + deltaprim[0];
      turboVelocity[0] = AverageTurboVelocity[val_marker][iSpan][0] + deltaprim[1];
      turboVelocity[1] = AverageTurboVelocity[val_marker][iSpan][1] + deltaprim[2];
      if(nDim == 2){
        Pressure_b = AveragePressure[val_marker][iSpan] + deltaprim[3];
      }
      else{
        turboVelocity[2] = AverageTurboVelocity[val_marker][iSpan][2] + deltaprim[3];
        Pressure_b = AveragePressure[val_marker][iSpan] + deltaprim[4];
      }


      ComputeBackVelocity(turboVelocity, turboNormal, Velocity_b, config->GetMarker_All_TurbomachineryFlag(val_marker), config->GetKind_TurboMachinery(iZone));
      Velocity2_b = 0.0;
      for (iDim = 0; iDim < nDim; iDim++) {
        Velocity2_b+= Velocity_b[iDim]*Velocity_b[iDim];
      }

      if(Pressure_b <= 0.0 || Density_b <= 0.0 ){
        Pressure_b = Pressure_i;
        Density_b = Density_i;
        Velocity2_b = 0.0;
        for (iDim = 0; iDim < nDim; iDim++) {
          Velocity_b[iDim] = Velocity_i[iDim];
          Velocity2_b+= Velocity_b[iDim]*Velocity_b[iDim];
        }
      }

      GetFluidModel()->SetTDState_Prho(Pressure_b, Density_b);
      Energy_b = GetFluidModel()->GetStaticEnergy() + 0.5*Velocity2_b;
      Temperature_b= GetFluidModel()->GetTemperature();
      Enthalpy_b = Energy_b + Pressure_b/Density_b;
      /*--- SST: the boundary state carries the k of the node in its total enthalpy, as the interior one. ---*/
      if (config->GetKind_Turb_Model() == TURB_MODEL::SST)
        Enthalpy_b += solver_container[TURB_SOL]->GetNodes()->GetSolution(iPoint, 0);

      /*--- Primitive variables, using the derived quantities ---*/
      V_boundary[0] = Temperature_b;
      for (iDim = 0; iDim < nDim; iDim++)
        V_boundary[iDim+1] = Velocity_b[iDim];
      V_boundary[nDim+1] = Pressure_b;
      V_boundary[nDim+2] = Density_b;
      V_boundary[nDim+3] = Enthalpy_b;

      S_boundary[0]= GetFluidModel()->GetdPdrho_e();
      S_boundary[1]= GetFluidModel()->GetdPde_rho();



      /*--- Set various quantities in the solver class ---*/

      conv_numerics->SetPrimitive(V_domain, V_boundary);

      /*--- SST: both states contain the k of the node in their total enthalpy. ---*/

      if (config->GetKind_Turb_Model() == TURB_MODEL::SST) {

        const su2double tke = solver_container[TURB_SOL]->GetNodes()->GetSolution(iPoint, 0);

        conv_numerics->SetTurbKineticEnergy(tke, tke);

      }
      conv_numerics->SetSecondary(S_domain, S_boundary);


      if (dynamic_grid)
        conv_numerics->SetGridVel(geometry->nodes->GetGridVel(iPoint), geometry->nodes->GetGridVel(iPoint));

      /*--- Compute the residual using an upwind scheme ---*/

      auto residual = conv_numerics->ComputeResidual(config);

      /*--- Update residual value ---*/
      LinSysRes.AddBlock(iPoint, residual);

      /*--- Jacobian contribution for implicit integration ---*/
      if (implicit)
        Jacobian.AddBlock2Diag(iPoint, residual.jacobian_i);

      /*--- Viscous contribution ---*/

      if (viscous) {

        /*--- Set laminar and eddy viscosity at the infinity ---*/

        V_boundary[nDim+5] = GetFluidModel()->GetLaminarViscosity();
        V_boundary[nDim+6] = nodes->GetEddyViscosity(iPoint);
        V_boundary[nDim+7] = GetFluidModel()->GetThermalConductivity();
        V_boundary[nDim+8] = GetFluidModel()->GetCp();

        /*--- Set the normal vector and the coordinates ---*/

        visc_numerics->SetNormal(Normal);
        su2double Coord_Reflected[MAXNDIM];
        GeometryToolbox::PointPointReflect(nDim, geometry->nodes->GetCoord(Point_Normal),
                                                 geometry->nodes->GetCoord(iPoint), Coord_Reflected);
        visc_numerics->SetCoord(geometry->nodes->GetCoord(iPoint), Coord_Reflected);

        /*--- Primitive variables, and gradient ---*/

        visc_numerics->SetPrimitive(V_domain, V_boundary);
        visc_numerics->SetPrimVarGradient(nodes->GetGradient_Primitive(iPoint), nodes->GetGradient_Primitive(iPoint));


        /*--- Compute secondary thermodynamic properties (partial derivatives...) ---*/

        S_boundary[0]= GetFluidModel()->GetdPdrho_e();
        S_boundary[1]= GetFluidModel()->GetdPde_rho();

        S_boundary[2]= GetFluidModel()->GetdTdrho_e();
        S_boundary[3]= GetFluidModel()->GetdTde_rho();

        /*--- Compute secondary thermo-physical properties (partial derivatives...) ---*/

        S_boundary[4]= GetFluidModel()->Getdmudrho_T();
        S_boundary[5]= GetFluidModel()->GetdmudT_rho();

        S_boundary[6]= GetFluidModel()->Getdktdrho_T();
        S_boundary[7]= GetFluidModel()->GetdktdT_rho();

        visc_numerics->SetSecondary(S_domain, S_boundary);

        /*--- Turbulent kinetic energy ---*/

        if (config->GetKind_Turb_Model() == TURB_MODEL::SST)
          visc_numerics->SetTurbKineticEnergy(solver_container[TURB_SOL]->GetNodes()->GetSolution(iPoint,0),
                                              solver_container[TURB_SOL]->GetNodes()->GetSolution(iPoint,0));

        /*--- Compute and update residual ---*/

        auto residual = visc_numerics->ComputeResidual(config);
        LinSysRes.SubtractBlock(iPoint, residual);

        /*--- Jacobian contribution for implicit integration ---*/

        if (implicit)
          Jacobian.SubtractBlock2Diag(iPoint, residual.jacobian_i);

      }
      /*--- Store number of Newton iterations at BC ---*/
      if(config->GetKind_FluidModel() == DATADRIVEN_FLUID)
        nodes->SetNewtonSolverIterations(iPoint, GetFluidModel()->GetnIter_Newton());

    }
    END_SU2_OMP_FOR
  }

  /*--- Free locally allocated memory ---*/
  delete [] Normal;

  delete [] Velocity_b;
  delete [] Velocity_i;

  delete [] S_boundary;
  delete [] delta_c;
  delete [] deltaprim;
  delete [] cj;
  for (iVar = 0; iVar < nVar; iVar++)
  {
    delete [] R_Matrix[iVar];
  }
  for (iVar = 0; iVar < nVar-1; iVar++)
  {
    delete [] R_c_inv[iVar];
    delete [] R_c[iVar];
  }
  delete [] R_Matrix;
  delete [] R_c;
  delete [] R_c_inv;
  delete [] R;
  delete [] c_avg;
  delete [] dcjs;

  delete [] AverageTurboMach;
  delete [] UnitNormal;
  delete [] turboNormal;
  delete [] turboVelocity;
}

void CEulerSolver::BC_Inlet(CGeometry *geometry, CSolver **solver_container,
                            CNumerics *conv_numerics, CNumerics *visc_numerics,
                            CConfig *config, unsigned short val_marker) {
  SU2_ZONE_SCOPED
  unsigned short iDim;
  su2double P_Total, T_Total, Velocity[MAXNDIM], Velocity2, H_Total, Temperature, Riemann,
  Pressure, Density, Energy, Flow_Dir[MAXNDIM], Mach2, SoundSpeed2, SoundSpeed_Total2, Vel_Mag,
  alpha, aa, bb, cc, dd, Area, UnitNormal[MAXNDIM], Normal[MAXNDIM];
  su2double *V_inlet, *V_domain;

  const bool implicit = (config->GetKind_TimeIntScheme() == EULER_IMPLICIT);
  const su2double Two_Gamma_M1 = 2.0 / Gamma_Minus_One;
  const su2double Gas_Constant = config->GetGas_ConstantND();
  const auto Kind_Inlet_Cfg = config->GetKind_Inlet();
  const auto Marker_Tag = config->GetMarker_All_TagBound(val_marker);
  const bool tkeNeeded = (config->GetKind_Turb_Model() == TURB_MODEL::SST);
  /*--- SPLIT_EDGE_FLUX with SST (matched k): hat flux and the closure C k rule, the SST inlet k on inflow. ---*/
  const bool splitMatchedK = config->GetSplit_Edge_Flux() && config->GetTke_Matched_P1();

  /*--- Loop over all the vertices on this boundary marker ---*/

  SU2_OMP_FOR_DYN(OMP_MIN_SIZE)
  for (auto iVertex = 0ul; iVertex < geometry->nVertex[val_marker]; iVertex++) {

    /*--- Allocate the value at the inlet ---*/

    V_inlet = GetCharacPrimVar(val_marker, iVertex);

    const auto iPoint = geometry->vertex[val_marker][iVertex]->GetNode();

    /*--- Check if the node belongs to the domain (i.e., not a halo node) ---*/

    if (!geometry->nodes->GetDomain(iPoint)) continue;

    /*--- Normal vector for this vertex (negate for outward convention) ---*/

    geometry->vertex[val_marker][iVertex]->GetNormal(Normal);
    for (iDim = 0; iDim < nDim; iDim++) Normal[iDim] = -Normal[iDim];
    conv_numerics->SetNormal(Normal);

    Area = GeometryToolbox::Norm(nDim, Normal);
    for (iDim = 0; iDim < nDim; iDim++)
      UnitNormal[iDim] = Normal[iDim]/Area;

    /*--- Retrieve solution at this boundary node ---*/

    V_domain = nodes->GetPrimitive(iPoint);

    /*--- On intersections with viscous walls we set the total conditions equal to
        the static conditions of the wall point. This avoids a jump in pressure,
        energy, etc. at this location. ---*/

    const auto Kind_Inlet = geometry->nodes->GetViscousBoundary(iPoint)
        ? INLET_TYPE::TOTAL_CONDITIONS : Kind_Inlet_Cfg;

    /*--- Build the fictitious inlet state based on characteristics ---*/

    /*--- Subsonic inflow: there is one outgoing characteristic (u-c),
        therefore we can specify all but one state variable at the inlet.
        The outgoing Riemann invariant provides the final piece of info.
        Adapted from an original implementation in the Stanford University
        multi-block (SUmb) solver in the routine bcSubsonicInflow.f90
        written by Edwin van der Weide, last modified 04-20-2009. ---*/

    switch (Kind_Inlet) {

      /*--- Total properties have been specified at the inlet. ---*/

      case INLET_TYPE::TOTAL_CONDITIONS: {

        /*--- Retrieve the specified total conditions for this inlet. ---*/

        P_Total = Inlet_Ptotal[val_marker][iVertex];
        T_Total = Inlet_Ttotal[val_marker][iVertex];
        const su2double* dir = Inlet_FlowDir[val_marker][iVertex];
        const su2double mag = GeometryToolbox::Norm(nDim, dir);

        /*--- Store the unit flow direction vector.
         If requested, use the local boundary normal (negative),
         instead of the prescribed flow direction in the config. ---*/

        if (config->GetInletUseNormal()) {
          for (iDim = 0; iDim < nDim; iDim++)
            Flow_Dir[iDim] = -UnitNormal[iDim];
        } else {
          for (iDim = 0; iDim < nDim; iDim++)
            Flow_Dir[iDim] = dir[iDim]/mag;
        }

        /*--- Non-dim. the inputs if necessary. ---*/

        P_Total /= config->GetPressure_Ref();
        T_Total /= config->GetTemperature_Ref();

        if (geometry->nodes->GetViscousBoundary(iPoint)) {
          P_Total = nodes->GetPressure(iPoint);
          T_Total = nodes->GetTemperature(iPoint);
        }

        /*--- Store primitives and set some variables for clarity. ---*/

        Density = V_domain[nDim+2];
        Velocity2 = 0.0;
        for (iDim = 0; iDim < nDim; iDim++) {
          Velocity[iDim] = V_domain[iDim+1];
          Velocity2 += Velocity[iDim]*Velocity[iDim];
        }
        Energy      = V_domain[nDim+3] - V_domain[nDim+1]/V_domain[nDim+2];
        /*--- SST: the total enthalpy of the domain contains k, the imposed total enthalpy does not. ---*/
        if (tkeNeeded) Energy -= solver_container[TURB_SOL]->GetNodes()->GetSolution(iPoint,0);
        Pressure    = V_domain[nDim+1];
        H_Total     = (Gamma*Gas_Constant/Gamma_Minus_One)*T_Total;
        SoundSpeed2 = Gamma*Pressure/Density;

        /*--- Compute the acoustic Riemann invariant that is extrapolated
            from the domain interior. ---*/

        Riemann   = 2.0*sqrt(SoundSpeed2)/Gamma_Minus_One;
        for (iDim = 0; iDim < nDim; iDim++)
          Riemann += Velocity[iDim]*UnitNormal[iDim];

        /*--- Total speed of sound ---*/

        SoundSpeed_Total2 = Gamma_Minus_One*(H_Total - (Energy + Pressure/Density)+0.5*Velocity2) + SoundSpeed2;

        /*--- Dot product of normal and flow direction. This should
            be negative due to outward facing boundary normal convention. ---*/

        alpha = 0.0;
        for (iDim = 0; iDim < nDim; iDim++)
          alpha += UnitNormal[iDim]*Flow_Dir[iDim];

        /*--- Coefficients in the quadratic equation for the velocity ---*/

        aa =  1.0 + 0.5*Gamma_Minus_One*alpha*alpha;
        bb = -1.0*Gamma_Minus_One*alpha*Riemann;
        cc =  0.5*Gamma_Minus_One*Riemann*Riemann
            -2.0*SoundSpeed_Total2/Gamma_Minus_One;

        /*--- Solve quadratic equation for velocity magnitude. Value must
            be positive, so the choice of root is clear. ---*/

        dd = bb*bb - 4.0*aa*cc;
        dd = sqrt(max(0.0, dd));
        Vel_Mag   = (-bb + dd)/(2.0*aa);
        Vel_Mag   = max(0.0, Vel_Mag);
        Velocity2 = Vel_Mag*Vel_Mag;

        /*--- Compute speed of sound from total speed of sound eqn. ---*/

        SoundSpeed2 = SoundSpeed_Total2 - 0.5*Gamma_Minus_One*Velocity2;

        /*--- Mach squared (cut between 0-1), use to adapt velocity ---*/

        Mach2 = Velocity2/SoundSpeed2;
        Mach2 = min(1.0, Mach2);
        Velocity2   = Mach2*SoundSpeed2;
        Vel_Mag     = sqrt(Velocity2);
        SoundSpeed2 = SoundSpeed_Total2 - 0.5*Gamma_Minus_One*Velocity2;

        /*--- Compute new velocity vector at the inlet ---*/

        for (iDim = 0; iDim < nDim; iDim++)
          Velocity[iDim] = Vel_Mag*Flow_Dir[iDim];

        /*--- Static temperature from the speed of sound relation ---*/

        Temperature = SoundSpeed2/(Gamma*Gas_Constant);

        /*--- Static pressure using isentropic relation at a point ---*/

        Pressure = P_Total*pow((Temperature/T_Total), Gamma/Gamma_Minus_One);

        /*--- Density at the inlet from the gas law ---*/

        Density = Pressure/(Gas_Constant*Temperature);

        /*--- Using pressure, density, & velocity, compute the energy ---*/

        Energy = Pressure/(Density*Gamma_Minus_One) + 0.5*Velocity2;

        /*--- Primitive variables, using the derived quantities ---*/

        V_inlet[0] = Temperature;
        for (iDim = 0; iDim < nDim; iDim++)
          V_inlet[iDim+1] = Velocity[iDim];
        V_inlet[nDim+1] = Pressure;
        V_inlet[nDim+2] = Density;
        V_inlet[nDim+3] = Energy + Pressure/Density;

        /*--- k of the boundary node, not the free-stream value, which decays right away in the domain (#1851). ---*/
        if (tkeNeeded) V_inlet[nDim+3] += solver_container[TURB_SOL]->GetNodes()->GetSolution(iPoint,0);

        break;
      }
      /*--- Mass flow has been specified at the inlet. ---*/

      case INLET_TYPE::MASS_FLOW: {

        /*--- Retrieve the specified mass flow for the inlet. ---*/

        Density  = Inlet_Ttotal[val_marker][iVertex];
        Vel_Mag  = Inlet_Ptotal[val_marker][iVertex];
        const su2double* dir = Inlet_FlowDir[val_marker][iVertex];
        const su2double mag = GeometryToolbox::Norm(nDim, dir);


        /*--- Store the unit flow direction vector.
         If requested, use the local boundary normal (negative),
         instead of the prescribed flow direction in the config. ---*/

        if (config->GetInletUseNormal()) {
          for (iDim = 0; iDim < nDim; iDim++)
            Flow_Dir[iDim] = -UnitNormal[iDim];
        } else {
          for (iDim = 0; iDim < nDim; iDim++)
            Flow_Dir[iDim] = dir[iDim]/mag;
        }

        /*--- Non-dim. the inputs if necessary. ---*/

        Density /= config->GetDensity_Ref();
        Vel_Mag /= config->GetVelocity_Ref();

        /*--- Get primitives from current inlet state. ---*/

        for (iDim = 0; iDim < nDim; iDim++)
          Velocity[iDim] = nodes->GetVelocity(iPoint,iDim);
        Pressure    = nodes->GetPressure(iPoint);
        SoundSpeed2 = Gamma*Pressure/V_domain[nDim+2];

        /*--- Compute the acoustic Riemann invariant that is extrapolated
            from the domain interior. ---*/

        Riemann = Two_Gamma_M1*sqrt(SoundSpeed2);
        for (iDim = 0; iDim < nDim; iDim++)
          Riemann += Velocity[iDim]*UnitNormal[iDim];

        /*--- Speed of sound squared for fictitious inlet state ---*/

        SoundSpeed2 = Riemann;
        for (iDim = 0; iDim < nDim; iDim++)
          SoundSpeed2 -= Vel_Mag*Flow_Dir[iDim]*UnitNormal[iDim];

        SoundSpeed2 = max(0.0,0.5*Gamma_Minus_One*SoundSpeed2);
        SoundSpeed2 = SoundSpeed2*SoundSpeed2;

        /*--- Pressure for the fictitious inlet state ---*/

        Pressure = SoundSpeed2*Density/Gamma;

        /*--- Energy for the fictitious inlet state ---*/

        Energy = Pressure/(Density*Gamma_Minus_One) + 0.5*Vel_Mag*Vel_Mag;

        /*--- Primitive variables, using the derived quantities ---*/
        Temperature = Pressure / ( Gas_Constant * Density);
        V_inlet[0] = Temperature;
        for (iDim = 0; iDim < nDim; iDim++)
          V_inlet[iDim+1] = Vel_Mag*Flow_Dir[iDim];
        V_inlet[nDim+1] = Pressure;
        V_inlet[nDim+2] = Density;
        V_inlet[nDim+3] = Energy + Pressure/Density;

        /*--- k of the boundary node, not the free-stream value, which decays right away in the domain (#1851). ---*/
        if (tkeNeeded) V_inlet[nDim+3] += solver_container[TURB_SOL]->GetNodes()->GetSolution(iPoint,0);

        break;
      }
      default:
        SU2_MPI::Error("Unsupported INLET_TYPE.", CURRENT_FUNCTION);
        break;
    }

    /*--- Set various quantities in the solver class ---*/

    su2double Vhat[2][MAXNVAR + 8];
    if (splitMatchedK) {
      SetBoundaryHatStates(conv_numerics, V_domain, V_inlet, solver_container[TURB_SOL]->GetNodes()->GetSolution(iPoint, 0), Vhat);
    } else {
    conv_numerics->SetPrimitive(V_domain, V_inlet);

    /*--- SST: both states contain the k of the node in their total enthalpy. ---*/

    if (config->GetKind_Turb_Model() == TURB_MODEL::SST) {

      const su2double tke = solver_container[TURB_SOL]->GetNodes()->GetSolution(iPoint, 0);

      conv_numerics->SetTurbKineticEnergy(tke, tke);

    }
    }

    if (dynamic_grid)
      conv_numerics->SetGridVel(geometry->nodes->GetGridVel(iPoint), geometry->nodes->GetGridVel(iPoint));

    /*--- Compute the residual using an upwind scheme ---*/

    auto residual = conv_numerics->ComputeResidual(config);

    if (splitMatchedK) {
      su2double resK[MAXNVAR] = {0.0}, jacK[5][5] = {{0.0}};
      for (unsigned short iVar = 0; iVar < nVar; ++iVar) {
        resK[iVar] = residual[iVar];
        for (unsigned short jVar = 0; jVar < nVar; ++jVar) jacK[iVar][jVar] = implicit ? residual.jacobian_i[iVar][jVar] : 0.0;
      }
      const su2double kBc = solver_container[TURB_SOL]->GetMatchedInletKine(config, this, val_marker, iVertex);
      AddBoundaryMatchedKine(iPoint, solver_container[TURB_SOL]->GetNodes()->GetSolution(iPoint, 0), kBc, resK, jacK,
                             implicit);
      LinSysRes.AddBlock(iPoint, resK);
      if (implicit) Jacobian.AddBlock2Diag(iPoint, jacK);
      continue;
    }

    /*--- Update residual value ---*/

    LinSysRes.AddBlock(iPoint, residual);

    /*--- Jacobian contribution for implicit integration ---*/

    if (implicit)
      Jacobian.AddBlock2Diag(iPoint, residual.jacobian_i);
  }
  END_SU2_OMP_FOR

}


void CEulerSolver::BC_Inlet_Residual(CGeometry* geometry, CSolver** solver_container, CNumerics* conv_numerics,
                       CNumerics* visc_numerics, CConfig* config, unsigned short val_marker,
                       unsigned long val_element, unsigned short iNode,
                       su2double* residualBuffer) {

  unsigned short iDim;
  unsigned long iVertex, iPoint;
  su2double P_Total, T_Total, Velocity[MAXNDIM], Velocity2, H_Total, Temperature, Riemann,
      Pressure, Density, Energy, Flow_Dir[MAXNDIM], Mach2, SoundSpeed2, SoundSpeed_Total2, Vel_Mag,
      alpha, aa, bb, cc, dd, Area, UnitNormal[MAXNDIM], Normal[MAXNDIM];
  su2double *V_inlet, *V_domain;

  const bool implicit = (config->GetKind_TimeIntScheme() == EULER_IMPLICIT);
  const su2double Two_Gamma_M1 = 2.0 / Gamma_Minus_One;
  const su2double Gas_Constant = config->GetGas_ConstantND();
  const auto Kind_Inlet = config->GetKind_Inlet();
  const auto Marker_Tag = config->GetMarker_All_TagBound(val_marker);
  const bool tkeNeeded = (config->GetKind_Turb_Model() == TURB_MODEL::SST);


  /** Normal should already be pointing outward **/
  const su2double* AreaVector = geometry->bound[val_marker][val_element]->getNormal();
  Area = GeometryToolbox::Norm(nDim,AreaVector);
  for (iDim = 0; iDim < nDim; iDim++) UnitNormal[iDim] = AreaVector[iDim] / Area;
  for (iDim = 0; iDim < nDim; iDim++) Normal[iDim] = AreaVector[iDim] / 1.0;

  /*--- Loop over all the vertices on this boundary marker ---*/

  iPoint = geometry->bound[val_marker][val_element]->GetNode(iNode);
  iVertex = geometry->nodes->GetVertex(iPoint,val_marker);

    /*--- Allocate the value at the inlet ---*/

    V_inlet = GetCharacPrimVar(val_marker, iVertex);

    /*--- Check if the node belongs to the domain (i.e., not a halo node) ---*/

    if (true/*geometry->nodes->GetDomain(iPoint)*/) {

      conv_numerics->SetNormal(Normal);

      /*--- Retrieve solution at this boundary node ---*/

      V_domain = nodes->GetPrimitive(iPoint);

      /*--- Build the fictitious intlet state based on characteristics ---*/


      /*--- Subsonic inflow: there is one outgoing characteristic (u-c),
         therefore we can specify all but one state variable at the inlet.
         The outgoing Riemann invariant provides the final piece of info.
         Adapted from an original implementation in the Stanford University
         multi-block (SUmb) solver in the routine bcSubsonicInflow.f90
         written by Edwin van der Weide, last modified 04-20-2009. ---*/

      switch (Kind_Inlet) {

          /*--- Total properties (total pressure, total temperature and flow direction) or mass flow (density,
             velocity magnitude and flow direction) have been specified at the inlet. The ghost is built by
             P1BoundaryGhost::Inlet, shared with closure C of BC_ENTROPY_P1: the flow direction is the prescribed one or,
             if requested, the local boundary normal (negative); the inputs are non-dimensionalized there. ---*/

        case INLET_TYPE::TOTAL_CONDITIONS:
        case INLET_TYPE::MASS_FLOW: {

          const bool massFlow = (Kind_Inlet == INLET_TYPE::MASS_FLOW);
          P1BoundaryGhost::InletState ghost;
          /*--- SST: the total enthalpy of the domain contains k, the imposed total enthalpy does not (as BC_Inlet);
             the ghost carries the k of the boundary node, not the free-stream value (#1851). ---*/
          const su2double tkeB = tkeNeeded ? solver_container[TURB_SOL]->GetNodes()->GetSolution(iPoint, 0) : 0.0;
          P1BoundaryGhost::Inlet(nDim, massFlow, Gamma, Gas_Constant, V_domain[nDim+2], &V_domain[1], V_domain[nDim+1],
                                 V_domain[nDim+3] - tkeB, Inlet_Ttotal[val_marker][iVertex], Inlet_Ptotal[val_marker][iVertex],
                                 Inlet_FlowDir[val_marker][iVertex], config->GetInletUseNormal(), UnitNormal,
                                 massFlow ? config->GetDensity_Ref() : config->GetTemperature_Ref(),
                                 massFlow ? config->GetVelocity_Ref() : config->GetPressure_Ref(), ghost);
          Pressure = ghost.Pressure;
          Density = ghost.Density;

          /*--- Using pressure, density, & velocity, compute the energy ---*/

          if (massFlow) Energy = Pressure/(Density*Gamma_Minus_One) + 0.5*ghost.Vel_Mag*ghost.Vel_Mag;
          else Energy = Pressure/(Density*Gamma_Minus_One) + 0.5*ghost.Velocity2;
          Energy += tkeB;

          /*--- Primitive variables, using the derived quantities ---*/

          V_inlet[0] = ghost.Temperature;
          for (iDim = 0; iDim < nDim; iDim++)
            V_inlet[iDim+1] = ghost.Velocity[iDim];
          V_inlet[nDim+1] = Pressure;
          V_inlet[nDim+2] = Density;
          V_inlet[nDim+3] = Energy + Pressure/Density;

          break;
        }
        default:
          SU2_MPI::Error("Unsupported INLET_TYPE.", CURRENT_FUNCTION);
          break;
      }

      /*--- Set various quantities in the solver class ---*/

      conv_numerics->SetPrimitive(V_domain, V_inlet);

      /*--- SST: both states contain the k of the node in their total enthalpy. ---*/
      if (tkeNeeded) {
        const su2double tke = solver_container[TURB_SOL]->GetNodes()->GetSolution(iPoint, 0);
        conv_numerics->SetTurbKineticEnergy(tke, tke);
      }

      if (dynamic_grid)
        conv_numerics->SetGridVel(geometry->nodes->GetGridVel(iPoint), geometry->nodes->GetGridVel(iPoint));

      /*--- Compute the residual using an upwind scheme ---*/

      auto residual = conv_numerics->ComputeResidual(config);

      /*--- Update residual value ---*/

      for (short iVar = 0; iVar < nVar; ++iVar) residualBuffer[iVar] = residual[iVar];

      /*--- Jacobian contribution for implicit integration. ---*/
      if (implicit) {
        for (short iVar = 0; iVar < nVar; ++iVar){
          for (short jVar = 0; jVar < nVar; ++jVar) {
            residualBuffer[MAXNVAR + iVar*nVar + jVar] = residual.jacobian_i[iVar][jVar];
          }
        }
      }


    }


}

void CEulerSolver::BC_Outlet(CGeometry *geometry, CSolver **solver_container,
                             CNumerics *conv_numerics, CNumerics *visc_numerics,
                             CConfig *config, unsigned short val_marker) {
  SU2_ZONE_SCOPED
  unsigned short iVar, iDim;
  unsigned long iVertex, iPoint;
  su2double Pressure, P_Exit, Velocity[3],
  Velocity2, Entropy, Density, Energy, Riemann, Vn, SoundSpeed, Mach_Exit, Vn_Exit,
  Area, UnitNormal[3];
  su2double *V_outlet, *V_domain;

  bool implicit           = (config->GetKind_TimeIntScheme() == EULER_IMPLICIT);
  su2double Gas_Constant     = config->GetGas_ConstantND();
  string Marker_Tag       = config->GetMarker_All_TagBound(val_marker);
  bool gravity = (config->GetGravityForce());
  bool tkeNeeded = (config->GetKind_Turb_Model() == TURB_MODEL::SST);

  CVariable* turbNodes = nullptr;
  if (tkeNeeded) turbNodes = solver_container[TURB_SOL]->GetNodes();

  /*--- SPLIT_EDGE_FLUX with SST (matched k): hat flux and the closure C k rule, the node's k on backflow (outlets). ---*/
  const bool splitMatchedK = config->GetSplit_Edge_Flux() && config->GetTke_Matched_P1();

  auto *Normal = new su2double[nDim];

  /*--- Loop over all the vertices on this boundary marker ---*/

  SU2_OMP_FOR_DYN(OMP_MIN_SIZE)
  for (iVertex = 0; iVertex < geometry->nVertex[val_marker]; iVertex++) {

    /*--- Allocate the value at the outlet ---*/
    V_outlet = GetCharacPrimVar(val_marker, iVertex);

    iPoint = geometry->vertex[val_marker][iVertex]->GetNode();

    /*--- Check if the node belongs to the domain (i.e., not a halo node) ---*/
    if (geometry->nodes->GetDomain(iPoint)) {

      /*--- Normal vector for this vertex (negate for outward convention) ---*/
      geometry->vertex[val_marker][iVertex]->GetNormal(Normal);
      for (iDim = 0; iDim < nDim; iDim++) Normal[iDim] = -Normal[iDim];
      conv_numerics->SetNormal(Normal);

      Area = GeometryToolbox::Norm(nDim, Normal);
      for (iDim = 0; iDim < nDim; iDim++)
        UnitNormal[iDim] = Normal[iDim]/Area;

      /*--- Current solution at this boundary node ---*/
      V_domain = nodes->GetPrimitive(iPoint);

      /*--- Build the fictitious inlet state based on characteristics ---*/

      /*--- Retrieve the specified back pressure for this outlet. ---*/
      if (gravity) P_Exit = config->GetOutlet_Pressure(Marker_Tag) - geometry->nodes->GetCoord(iPoint, nDim-1)*STANDARD_GRAVITY;
      else P_Exit = config->GetOutlet_Pressure(Marker_Tag);

      /*--- Non-dim. the inputs if necessary. ---*/
      P_Exit = P_Exit/config->GetPressure_Ref();

      /*--- Check whether the flow is supersonic at the exit. The type
         of boundary update depends on this. ---*/
      Density = V_domain[nDim+2];
      Velocity2 = 0.0; Vn = 0.0;
      for (iDim = 0; iDim < nDim; iDim++) {
        Velocity[iDim] = V_domain[iDim+1];
        Velocity2 += Velocity[iDim]*Velocity[iDim];
        Vn += Velocity[iDim]*UnitNormal[iDim];
      }
      Pressure   = V_domain[nDim+1];
      SoundSpeed = sqrt(Gamma*Pressure/Density);
      Mach_Exit  = sqrt(Velocity2)/SoundSpeed;

      if (Mach_Exit >= 1.0) {

        /*--- Supersonic exit flow: there are no incoming characteristics,
           so no boundary condition is necessary. Set outlet state to current
           state so that upwinding handles the direction of propagation. ---*/
        for (iVar = 0; iVar < nPrimVar; iVar++) V_outlet[iVar] = V_domain[iVar];

      } else {

        /*--- Subsonic exit flow: there is one incoming characteristic,
           therefore one variable can be specified (back pressure) and is used
           to update the conservative variables. Compute the entropy and the
           acoustic Riemann variable. These invariants, as well as the
           tangential velocity components, are extrapolated. Adapted from an
           original implementation in the Stanford University multi-block
           (SUmb) solver in the routine bcSubsonicOutflow.f90 by Edwin van
           der Weide, last modified 09-10-2007. ---*/

        Entropy = Pressure*pow(1.0/Density, Gamma);
        Riemann = Vn + 2.0*SoundSpeed/Gamma_Minus_One;

        /*--- Compute the new fictious state at the outlet ---*/
        Density    = pow(P_Exit/Entropy,1.0/Gamma);
        Pressure   = P_Exit;
        SoundSpeed = sqrt(Gamma*P_Exit/Density);
        Vn_Exit    = Riemann - 2.0*SoundSpeed/Gamma_Minus_One;
        Velocity2  = 0.0;
        for (iDim = 0; iDim < nDim; iDim++) {
          Velocity[iDim] = Velocity[iDim] + (Vn_Exit-Vn)*UnitNormal[iDim];
          Velocity2 += Velocity[iDim]*Velocity[iDim];
        }
        Energy = P_Exit/(Density*Gamma_Minus_One) + 0.5*Velocity2;
        if (tkeNeeded) Energy += turbNodes->GetSolution(iPoint,0);

        /*--- Conservative variables, using the derived quantities ---*/
        V_outlet[0] = Pressure / ( Gas_Constant * Density);
        for (iDim = 0; iDim < nDim; iDim++)
          V_outlet[iDim+1] = Velocity[iDim];
        V_outlet[nDim+1] = Pressure;
        V_outlet[nDim+2] = Density;
        V_outlet[nDim+3] = Energy + Pressure/Density;

      }

      /*--- Set various quantities in the solver class ---*/
      su2double Vhat[2][MAXNVAR + 8];
      if (splitMatchedK) {
        SetBoundaryHatStates(conv_numerics, V_domain, V_outlet, turbNodes->GetSolution(iPoint, 0), Vhat);
      } else {
      conv_numerics->SetPrimitive(V_domain, V_outlet);
      /*--- SST: both states contain the k of the node in their total enthalpy. ---*/
      if (config->GetKind_Turb_Model() == TURB_MODEL::SST) {
        const su2double tke = solver_container[TURB_SOL]->GetNodes()->GetSolution(iPoint, 0);
        conv_numerics->SetTurbKineticEnergy(tke, tke);
      }
      }

      if (dynamic_grid)
        conv_numerics->SetGridVel(geometry->nodes->GetGridVel(iPoint), geometry->nodes->GetGridVel(iPoint));

      /*--- Compute the residual using an upwind scheme ---*/

      auto residual = conv_numerics->ComputeResidual(config);

      /*--- Add Residuals and Jacobians ---*/

      if (splitMatchedK) {
        su2double resK[MAXNVAR] = {0.0}, jacK[5][5] = {{0.0}};
        for (iVar = 0; iVar < nVar; ++iVar) {
          resK[iVar] = residual[iVar];
          for (unsigned short jVar = 0; jVar < nVar; ++jVar) jacK[iVar][jVar] = implicit ? residual.jacobian_i[iVar][jVar] : 0.0;
        }
        const su2double kNode = turbNodes->GetSolution(iPoint, 0);
        AddBoundaryMatchedKine(iPoint, kNode, kNode, resK, jacK, implicit);
        LinSysRes.AddBlock(iPoint, resK);
        if (implicit) Jacobian.AddBlock2Diag(iPoint, jacK);
      } else {
      LinSysRes.AddBlock(iPoint, residual);
      if (implicit)
        Jacobian.AddBlock2Diag(iPoint, residual.jacobian_i);
      }

//      /*--- Viscous contribution, commented out because serious convergence problems  ---*/
//
//      if (viscous) {
//
//        /*--- Set laminar and eddy viscosity at the infinity ---*/
//
//        V_outlet[nDim+5] = nodes->GetLaminarViscosity(iPoint);
//        V_outlet[nDim+6] = nodes->GetEddyViscosity(iPoint);
//
//        /*--- Set the normal vector and the coordinates ---*/
//
//        visc_numerics->SetNormal(Normal);
//        su2double Coord_Reflected[MAXNDIM];
//        GeometryToolbox::PointPointReflect(nDim, geometry->nodes->GetCoord(Point_Normal),
//                                                 geometry->nodes->GetCoord(iPoint), Coord_Reflected);
//        visc_numerics->SetCoord(geometry->nodes->GetCoord(iPoint), Coord_Reflected);
//
//        /*--- Primitive variables, and gradient ---*/
//
//        visc_numerics->SetPrimitive(V_domain, V_outlet);
//        visc_numerics->SetPrimVarGradient(nodes->GetGradient_Primitive(iPoint), nodes->GetGradient_Primitive(iPoint));
//
//        /*--- Turbulent kinetic energy ---*/
//
//        if (config->GetKind_Turb_Model() == TURB_MODEL::SST)
//          visc_numerics->SetTurbKineticEnergy(solver_container[TURB_SOL]->GetNodes()->GetSolution(iPoint,0),
//                                              solver_container[TURB_SOL]->GetNodes()->GetSolution(iPoint,0));
//
//        /*--- Compute and update residual ---*/
//
//        auto residual = visc_numerics->ComputeResidual(config);
//        LinSysRes.SubtractBlock(iPoint, residual);
//
//        /*--- Jacobian contribution for implicit integration ---*/
//
//        if (implicit)
//         Jacobian.SubtractBlock2Diag(iPoint, residual.acobian_i);
//
//      }

    }
  }
  END_SU2_OMP_FOR

  /*--- Free locally allocated memory ---*/
  delete [] Normal;

}


void CEulerSolver::BC_Outlet_Residual(CGeometry* geometry, CSolver** solver_container, CNumerics* conv_numerics,
                                       CNumerics* visc_numerics, CConfig* config, unsigned short val_marker,
                                       unsigned long val_element, unsigned short iNode,
                                       su2double* residualBuffer) {


  unsigned short iVar, iDim;
  unsigned long iVertex, iPoint;
  su2double Pressure, P_Exit, Velocity[3],
      Velocity2, Entropy, Density, Energy, Riemann, Vn, SoundSpeed, Mach_Exit, Vn_Exit,
      Area, UnitNormal[3];
  su2double *V_outlet, *V_domain;

  bool implicit           = (config->GetKind_TimeIntScheme() == EULER_IMPLICIT);
  su2double Gas_Constant     = config->GetGas_ConstantND();
  string Marker_Tag       = config->GetMarker_All_TagBound(val_marker);
  bool gravity = (config->GetGravityForce());
  bool tkeNeeded = (config->GetKind_Turb_Model() == TURB_MODEL::SST);

  su2double Normal[MAXNDIM] = {0.0};

  /** Normal should already be pointing outward **/
  const su2double* AreaVector = geometry->bound[val_marker][val_element]->getNormal();
  Area = GeometryToolbox::Norm(nDim,AreaVector);
  for (iDim = 0; iDim < nDim; iDim++) UnitNormal[iDim] = AreaVector[iDim] / Area;
  for (iDim = 0; iDim < nDim; iDim++) Normal[iDim] = AreaVector[iDim] / 1.0;

  /*--- Loop over all the vertices on this boundary marker ---*/

  iPoint = geometry->bound[val_marker][val_element]->GetNode(iNode);
  iVertex = geometry->nodes->GetVertex(iPoint,val_marker);


    /*--- Allocate the value at the outlet ---*/
    V_outlet = GetCharacPrimVar(val_marker, iVertex);

    iPoint = geometry->vertex[val_marker][iVertex]->GetNode();

    /*--- Check if the node belongs to the domain (i.e., not a halo node) ---*/
    if (true/*geometry->nodes->GetDomain(iPoint)*/) {

      conv_numerics->SetNormal(Normal);

      /*--- Current solution at this boundary node ---*/
      V_domain = nodes->GetPrimitive(iPoint);

      /*--- Build the fictitious inlet state based on characteristics ---*/

      /*--- Retrieve the specified back pressure for this outlet. ---*/
      if (gravity) P_Exit = config->GetOutlet_Pressure(Marker_Tag) - geometry->nodes->GetCoord(iPoint, nDim-1)*STANDARD_GRAVITY;
      else P_Exit = config->GetOutlet_Pressure(Marker_Tag);

      /*--- Non-dim. the inputs if necessary. ---*/
      P_Exit = P_Exit/config->GetPressure_Ref();

      /*--- Supersonic exit: the outlet state is the current state, the upwinding handles the direction of
         propagation. Subsonic exit: the back pressure with the extrapolated entropy, acoustic Riemann variable and
         tangential velocity (adapted from bcSubsonicOutflow.f90 of SUmb by Edwin van der Weide).
         P1BoundaryGhost::Outlet, shared with closure C of BC_ENTROPY_P1. ---*/
      P1BoundaryGhost::OutletState ghost;
      P1BoundaryGhost::Outlet(nDim, Gamma, V_domain[nDim+2], &V_domain[1], V_domain[nDim+1], P_Exit, UnitNormal, ghost);

      if (ghost.supersonic) {

        for (iVar = 0; iVar < nPrimVar; iVar++) V_outlet[iVar] = V_domain[iVar];

      } else {

        Density = ghost.Density;
        Pressure = ghost.Pressure;
        for (iDim = 0; iDim < nDim; iDim++) Velocity[iDim] = ghost.Velocity[iDim];
        Velocity2 = ghost.Velocity2;
        Energy = P_Exit/(Density*Gamma_Minus_One) + 0.5*Velocity2;
        if (tkeNeeded) Energy += solver_container[TURB_SOL]->GetNodes()->GetSolution(iPoint, 0);

        /*--- Conservative variables, using the derived quantities ---*/
        V_outlet[0] = Pressure / ( Gas_Constant * Density);
        for (iDim = 0; iDim < nDim; iDim++)
          V_outlet[iDim+1] = Velocity[iDim];
        V_outlet[nDim+1] = Pressure;
        V_outlet[nDim+2] = Density;
        V_outlet[nDim+3] = Energy + Pressure/Density;

      }

      /*--- Set various quantities in the solver class ---*/
      conv_numerics->SetPrimitive(V_domain, V_outlet);

      /*--- SST: both states contain the k of the node in their total enthalpy. ---*/
      if (tkeNeeded) {
        const su2double tke = solver_container[TURB_SOL]->GetNodes()->GetSolution(iPoint, 0);
        conv_numerics->SetTurbKineticEnergy(tke, tke);
      }

      if (dynamic_grid)
        conv_numerics->SetGridVel(geometry->nodes->GetGridVel(iPoint), geometry->nodes->GetGridVel(iPoint));

      /*--- Compute the residual using an upwind scheme ---*/

      auto residual = conv_numerics->ComputeResidual(config);

      /*--- Update residual value ---*/
      for (iVar = 0; iVar < nVar; ++iVar) residualBuffer[iVar] = residual[iVar];

      /*--- Jacobian contribution for implicit integration. ---*/
      if (implicit) {
        for (iVar = 0; iVar < nVar; ++iVar){
          for (int jVar = 0; jVar < nVar; ++jVar) {
          residualBuffer[MAXNVAR + iVar*nVar + jVar] = residual.jacobian_i[iVar][jVar];
          }
        }
      }


    }

}

void CEulerSolver::BC_Supersonic_Inlet(CGeometry *geometry, CSolver **solver_container,
                                       CNumerics *conv_numerics, CNumerics *visc_numerics,
                                       CConfig *config, unsigned short val_marker) {
  SU2_ZONE_SCOPED
  const su2double Gas_Constant = config->GetGas_ConstantND();
  const bool implicit = (config->GetKind_TimeIntScheme() == EULER_IMPLICIT);
  const auto Marker_Tag = config->GetMarker_All_TagBound(val_marker);
  const bool tkeNeeded = (config->GetKind_Turb_Model() == TURB_MODEL::SST);

  /*--- Supersonic inlet flow: there are no outgoing characteristics,
   so all flow variables can be imposed at the inlet. ---*/

  SU2_OMP_FOR_DYN(OMP_MIN_SIZE)
  for (auto iVertex = 0ul; iVertex < geometry->nVertex[val_marker]; iVertex++) {
    const auto iPoint = geometry->vertex[val_marker][iVertex]->GetNode();

    if (!geometry->nodes->GetDomain(iPoint)) continue;

    /*--- Retrieve the inlet profile, note that total conditions are reused as static. ---*/

    const su2double Temperature = Inlet_Ttotal[val_marker][iVertex] / config->GetTemperature_Ref();
    const su2double Pressure = Inlet_Ptotal[val_marker][iVertex] / config->GetPressure_Ref();
    su2double Velocity[MAXNDIM] = {0.0};
    for (unsigned short iDim = 0; iDim < nDim; iDim++) {
      Velocity[iDim] = Inlet_FlowDir[val_marker][iVertex][iDim] / config->GetVelocity_Ref();
    }

    /*--- Density at the inlet from the gas law. ---*/

    const su2double Density = Pressure / (Gas_Constant * Temperature);

    /*--- Compute the energy from the specified state. ---*/

    const su2double Velocity2 = GeometryToolbox::SquaredNorm(int(MAXNDIM), Velocity);
    const su2double Energy = Pressure / (Density * Gamma_Minus_One) + 0.5 * Velocity2;

    /*--- Primitive variables, using the derived quantities. ---*/

    auto* V_inlet = GetCharacPrimVar(val_marker, iVertex);
    V_inlet[prim_idx.Temperature()] = Temperature;
    V_inlet[prim_idx.Pressure()] = Pressure;
    V_inlet[prim_idx.Density()] = Density;
    V_inlet[prim_idx.Enthalpy()] = Energy + Pressure / Density;
    for (unsigned short iDim = 0; iDim < nDim; iDim++)
      V_inlet[iDim+prim_idx.Velocity()] = Velocity[iDim];

    /*--- k of the boundary node, not the free-stream value, which decays right away in the domain (#1851). ---*/
    if (tkeNeeded) V_inlet[prim_idx.Enthalpy()] += solver_container[TURB_SOL]->GetNodes()->GetSolution(iPoint,0);

    /*--- Current solution at this boundary node. ---*/

    const auto* V_domain = nodes->GetPrimitive(iPoint);

    /*--- Normal vector for this vertex (negate for outward convention). ---*/

    su2double Normal[MAXNDIM] = {0.0};
    geometry->vertex[val_marker][iVertex]->GetNormal(Normal);
    for (unsigned short iDim = 0; iDim < nDim; iDim++) Normal[iDim] = -Normal[iDim];

    /*--- Set various quantities in the solver class. ---*/

    conv_numerics->SetNormal(Normal);
    conv_numerics->SetPrimitive(V_domain, V_inlet);
    /*--- SST: both states contain the k of the node in their total enthalpy. ---*/
    if (config->GetKind_Turb_Model() == TURB_MODEL::SST) {
      const su2double tke = solver_container[TURB_SOL]->GetNodes()->GetSolution(iPoint, 0);
      conv_numerics->SetTurbKineticEnergy(tke, tke);
    }

    if (dynamic_grid)
      conv_numerics->SetGridVel(geometry->nodes->GetGridVel(iPoint),
                                geometry->nodes->GetGridVel(iPoint));

    /*--- Compute the residual using an upwind scheme. ---*/

    auto residual = conv_numerics->ComputeResidual(config);

    LinSysRes.AddBlock(iPoint, residual);

    /*--- Jacobian contribution for implicit integration. ---*/

    if (implicit)
      Jacobian.AddBlock2Diag(iPoint, residual.jacobian_i);

    /*--- Viscous contribution, omited to improve convergence. ---*/

  }
  END_SU2_OMP_FOR

}

void CEulerSolver::BC_Supersonic_Outlet(CGeometry *geometry, CSolver **solver_container,
                                        CNumerics *conv_numerics, CNumerics *visc_numerics,
                                        CConfig *config, unsigned short val_marker) {
  SU2_ZONE_SCOPED
  unsigned short iDim;
  unsigned long iVertex, iPoint;
  su2double *V_outlet, *V_domain;

  bool implicit = (config->GetKind_TimeIntScheme() == EULER_IMPLICIT);
  bool viscous = config->GetViscous();
  string Marker_Tag = config->GetMarker_All_TagBound(val_marker);

  auto *Normal = new su2double[nDim];

  /*--- Supersonic outlet flow: there are no ingoing characteristics,
   so all flow variables can should be interpolated from the domain. ---*/

  /*--- Loop over all the vertices on this boundary marker ---*/

  SU2_OMP_FOR_DYN(OMP_MIN_SIZE)
  for (iVertex = 0; iVertex < geometry->nVertex[val_marker]; iVertex++) {

    iPoint = geometry->vertex[val_marker][iVertex]->GetNode();

    /*--- Check if the node belongs to the domain (i.e, not a halo node) ---*/

    if (geometry->nodes->GetDomain(iPoint)) {

      /*--- Current solution at this boundary node ---*/

      V_domain = nodes->GetPrimitive(iPoint);

      /*--- Allocate the value at the outlet ---*/

      V_outlet = GetCharacPrimVar(val_marker, iVertex);

      /*--- Primitive variables, using the derived quantities ---*/

      V_outlet[0] = V_domain[0];
      for (iDim = 0; iDim < nDim; iDim++)
        V_outlet[iDim+1] = V_domain[iDim+1];
      V_outlet[nDim+1] = V_domain[nDim+1];
      V_outlet[nDim+2] = V_domain[nDim+2];
      V_outlet[nDim+3] = V_domain[nDim+3];

      /*--- Current solution at this boundary node ---*/

      V_domain = nodes->GetPrimitive(iPoint);

      /*--- Normal vector for this vertex (negate for outward convention) ---*/

      geometry->vertex[val_marker][iVertex]->GetNormal(Normal);
      for (iDim = 0; iDim < nDim; iDim++) Normal[iDim] = -Normal[iDim];

      /*--- Set various quantities in the solver class ---*/

      conv_numerics->SetNormal(Normal);
      conv_numerics->SetPrimitive(V_domain, V_outlet);
      /*--- SST: both states contain the k of the node in their total enthalpy. ---*/
      if (config->GetKind_Turb_Model() == TURB_MODEL::SST) {
        const su2double tke = solver_container[TURB_SOL]->GetNodes()->GetSolution(iPoint, 0);
        conv_numerics->SetTurbKineticEnergy(tke, tke);
      }

      if (dynamic_grid)
        conv_numerics->SetGridVel(geometry->nodes->GetGridVel(iPoint),
                                  geometry->nodes->GetGridVel(iPoint));

      /*--- Compute the residual using an upwind scheme ---*/

      auto residual = conv_numerics->ComputeResidual(config);

      LinSysRes.AddBlock(iPoint, residual);

      /*--- Jacobian contribution for implicit integration ---*/

      if (implicit)
        Jacobian.AddBlock2Diag(iPoint, residual.jacobian_i);

     /*--- Viscous contribution, commented out because serious convergence problems ---*/

     if (viscous) {

       /*--- Set laminar and eddy viscosity at the infinity ---*/

       V_outlet[nDim+5] = nodes->GetLaminarViscosity(iPoint);
       V_outlet[nDim+6] = nodes->GetEddyViscosity(iPoint);

       /*--- Set the normal vector and the coordinates ---*/

       visc_numerics->SetNormal(Normal);
       su2double Coord_Reflected[MAXNDIM];
       unsigned long Point_Normal = geometry->vertex[val_marker][iVertex]->GetNormal_Neighbor();
       GeometryToolbox::PointPointReflect(nDim, geometry->nodes->GetCoord(Point_Normal),
                                                geometry->nodes->GetCoord(iPoint), Coord_Reflected);
       visc_numerics->SetCoord(geometry->nodes->GetCoord(iPoint), Coord_Reflected);

       /*--- Primitive variables, and gradient ---*/

       visc_numerics->SetPrimitive(V_domain, V_domain);
       visc_numerics->SetPrimVarGradient(nodes->GetGradient_Primitive(iPoint), nodes->GetGradient_Primitive(iPoint));

       /*--- Turbulent kinetic energy ---*/

       if (config->GetKind_Turb_Model() == TURB_MODEL::SST)
         visc_numerics->SetTurbKineticEnergy(solver_container[TURB_SOL]->GetNodes()->GetSolution(iPoint,0),
                                             solver_container[TURB_SOL]->GetNodes()->GetSolution(iPoint,0));

       /*--- Compute and update residual ---*/

       auto residual = visc_numerics->ComputeResidual(config);
       LinSysRes.SubtractBlock(iPoint, residual);

       /*--- Jacobian contribution for implicit integration ---*/

       if (implicit)
         Jacobian.SubtractBlock2Diag(iPoint, residual.jacobian_i);
     }

    }
  }
  END_SU2_OMP_FOR

  /*--- Free locally allocated memory ---*/

  delete [] Normal;

}

void CEulerSolver::BC_Engine_Inflow(CGeometry *geometry, CSolver **solver_container, CNumerics *conv_numerics, CNumerics *visc_numerics, CConfig *config, unsigned short val_marker) {
  SU2_ZONE_SCOPED

  unsigned short iDim;
  unsigned long iVertex, iPoint;
  su2double Pressure, Inflow_Pressure = 0.0, Velocity[3], Velocity2, Entropy, Target_Inflow_MassFlow = 0.0, Target_Inflow_Mach = 0.0, Density, Energy,
  Riemann, Area, UnitNormal[3], Vn, SoundSpeed, Vn_Exit, Inflow_Pressure_inc, Inflow_Pressure_old, Inflow_Mach_old, Inflow_MassFlow_old;
  su2double *V_inflow, *V_domain;

  su2double DampingFactor = config->GetDamp_Engine_Inflow();
  bool implicit = (config->GetKind_TimeIntScheme() == EULER_IMPLICIT);
  unsigned short Kind_Engine_Inflow = config->GetKind_Engine_Inflow();
  su2double Gas_Constant = config->GetGas_ConstantND();
  string Marker_Tag = config->GetMarker_All_TagBound(val_marker);
  bool tkeNeeded = (config->GetKind_Turb_Model() == TURB_MODEL::SST);
  su2double Baseline_Press = 0.75 * config->GetPressure_FreeStreamND();
  bool Engine_HalfModel = config->GetEngine_HalfModel();

  auto *Normal = new su2double[nDim];


  if (Kind_Engine_Inflow == FAN_FACE_MACH) {

    /*--- Retrieve the specified target fan face mach at the nacelle. ---*/

    Target_Inflow_Mach = config->GetEngineInflow_Target(Marker_Tag);

    /*--- Retrieve the old fan face pressure and mach number in the nacelle (this has been computed in a preprocessing). ---*/

    Inflow_Pressure_old = config->GetInflow_Pressure(Marker_Tag);  // Note that has been computed by the code (non-dimensional).
    Inflow_Mach_old = config->GetInflow_Mach(Marker_Tag);

    /*--- Compute the pressure increment (note that increasing pressure decreases flow speed) ---*/

    Inflow_Pressure_inc = - (1.0 - (Inflow_Mach_old/Target_Inflow_Mach)) * Baseline_Press;

    /*--- Estimate the new fan face pressure ---*/

    Inflow_Pressure = (1.0 - DampingFactor)*Inflow_Pressure_old + DampingFactor * (Inflow_Pressure_old + Inflow_Pressure_inc);

  }

  if (Kind_Engine_Inflow == FAN_FACE_MDOT) {

    /*--- Retrieve the specified target mass flow (non-dimensional) at the nacelle. ---*/

    Target_Inflow_MassFlow = config->GetEngineInflow_Target(Marker_Tag) / (config->GetDensity_Ref() * config->GetVelocity_Ref());

    if (config->GetExhaustToInlet_Engine()) {
      su2double Outlet_MF = 0.0;
      unsigned short nMarker_Global = config->GetnMarker_CfgFile();
      for (unsigned short iMarker_Global = 0; iMarker_Global < nMarker_Global; iMarker_Global++) {
        if (config->GetMarker_CfgFile_KindBC(config->GetMarker_CfgFile_TagBound(iMarker_Global)) == ENGINE_EXHAUST) {
          Outlet_MF = Outlet_MF + config->GetExhaust_MassFlow(config->GetMarker_CfgFile_TagBound(iMarker_Global));
        }
      }
      Target_Inflow_MassFlow = Outlet_MF;
    }

    if (config->GetSystemMeasurements() == US) Target_Inflow_MassFlow /= 32.174;

    if (Engine_HalfModel) Target_Inflow_MassFlow /= 2.0;

    /*--- Retrieve the old fan face pressure and mach number in the nacelle (this has been computed in a preprocessing). ---*/

    Inflow_Pressure_old = config->GetInflow_Pressure(Marker_Tag);  // Note that has been computed by the code (non-dimensional).
    Inflow_MassFlow_old = config->GetInflow_MassFlow(Marker_Tag);  // same here... it is a non dimensional value

    /*--- Compute the pressure increment (note that increasing pressure decreases flow speed) ---*/

    Inflow_Pressure_inc = - (1.0 - (Inflow_MassFlow_old/Target_Inflow_MassFlow)) * Baseline_Press;

    /*--- Estimate the new fan face pressure ---*/

    Inflow_Pressure = (1.0 - DampingFactor)*Inflow_Pressure_old + DampingFactor * (Inflow_Pressure_old + Inflow_Pressure_inc);

  }

  /*--- No iterative scheme if we provide the static pressure ---*/

  if (Kind_Engine_Inflow == FAN_FACE_PRESSURE) {

    /*--- Retrieve the specified pressure (non-dimensional) at the nacelle. ---*/

    Inflow_Pressure = config->GetEngineInflow_Target(Marker_Tag) / config->GetPressure_Ref();

  }


  /*--- Loop over all the vertices on this boundary marker ---*/

  SU2_OMP_FOR_DYN(OMP_MIN_SIZE)
  for (iVertex = 0; iVertex < geometry->nVertex[val_marker]; iVertex++) {

    /*--- Allocate the value at the outlet ---*/

    V_inflow = GetCharacPrimVar(val_marker, iVertex);

    iPoint = geometry->vertex[val_marker][iVertex]->GetNode();

    /*--- Check if the node belongs to the domain (i.e, not a halo node) ---*/

    if (geometry->nodes->GetDomain(iPoint)) {

      /*--- Normal vector for this vertex (negate for outward convention) ---*/

      geometry->vertex[val_marker][iVertex]->GetNormal(Normal);
      for (iDim = 0; iDim < nDim; iDim++) Normal[iDim] = -Normal[iDim];

      Area = GeometryToolbox::Norm(nDim, Normal);
      for (iDim = 0; iDim < nDim; iDim++)
        UnitNormal[iDim] = Normal[iDim]/Area;

      /*--- Current solution at this boundary node ---*/

      V_domain = nodes->GetPrimitive(iPoint);

      /*--- Subsonic nacelle inflow: there is one incoming characteristic,
       therefore one variable can be specified (back pressure) and is used
       to update the conservative variables.

       Compute the entropy and the acoustic variable. These
       riemann invariants, as well as the tangential velocity components,
       are extrapolated. ---*/

      Density = V_domain[nDim+2];
      Velocity2 = 0.0; Vn = 0.0;
      for (iDim = 0; iDim < nDim; iDim++) {
        Velocity[iDim] = V_domain[iDim+1];
        Velocity2 += Velocity[iDim]*Velocity[iDim];
        Vn += Velocity[iDim]*UnitNormal[iDim];
      }
      Pressure   = V_domain[nDim+1];
      SoundSpeed = sqrt(Gamma*Pressure/Density);
      Entropy = Pressure*pow(1.0/Density, Gamma);
      Riemann = Vn + 2.0*SoundSpeed/Gamma_Minus_One;

      /*--- Compute the new fictious state at the outlet ---*/

      Density    = pow(Inflow_Pressure/Entropy,1.0/Gamma);
      Pressure   = Inflow_Pressure;
      SoundSpeed = sqrt(Gamma*Inflow_Pressure/Density);
      Vn_Exit    = Riemann - 2.0*SoundSpeed/Gamma_Minus_One;
      Velocity2  = 0.0;
      for (iDim = 0; iDim < nDim; iDim++) {
        Velocity[iDim] = Velocity[iDim] + (Vn_Exit-Vn)*UnitNormal[iDim];
        Velocity2 += Velocity[iDim]*Velocity[iDim];
      }

      Energy = Inflow_Pressure/(Density*Gamma_Minus_One) + 0.5*Velocity2;
      if (tkeNeeded) Energy += solver_container[TURB_SOL]->GetNodes()->GetSolution(iPoint,0);  // node k, see #1851

      /*--- Conservative variables, using the derived quantities ---*/

      V_inflow[0] = Pressure / ( Gas_Constant * Density);
      for (iDim = 0; iDim < nDim; iDim++)
        V_inflow[iDim+1] = Velocity[iDim];
      V_inflow[nDim+1] = Pressure;
      V_inflow[nDim+2] = Density;
      V_inflow[nDim+3] = Energy + Pressure/Density;
      V_inflow[nDim+4] = SoundSpeed;

      /*--- Set various quantities in the solver class ---*/

      conv_numerics->SetNormal(Normal);
      conv_numerics->SetPrimitive(V_domain, V_inflow);
      /*--- SST: both states contain the k of the node in their total enthalpy. ---*/
      if (config->GetKind_Turb_Model() == TURB_MODEL::SST) {
        const su2double tke = solver_container[TURB_SOL]->GetNodes()->GetSolution(iPoint, 0);
        conv_numerics->SetTurbKineticEnergy(tke, tke);
      }

      /*--- Set grid movement ---*/

      if (dynamic_grid)
        conv_numerics->SetGridVel(geometry->nodes->GetGridVel(iPoint), geometry->nodes->GetGridVel(iPoint));

      /*--- Compute the residual using an upwind scheme ---*/

      auto residual = conv_numerics->ComputeResidual(config);

      LinSysRes.AddBlock(iPoint, residual);

      /*--- Jacobian contribution for implicit integration ---*/

      if (implicit)
        Jacobian.AddBlock2Diag(iPoint, residual.jacobian_i);

//      /*--- Viscous contribution, commented out because serious convergence problems ---*/
//
//      if (viscous) {
//
//        /*--- Set laminar and eddy viscosity at the infinity ---*/
//
//        V_inflow[nDim+5] = nodes->GetLaminarViscosity(iPoint);
//        V_inflow[nDim+6] = nodes->GetEddyViscosity(iPoint);
//
//        /*--- Set the normal vector and the coordinates ---*/
//
//        visc_numerics->SetNormal(Normal);
//        su2double Coord_Reflected[MAXNDIM];
//        GeometryToolbox::PointPointReflect(nDim, geometry->nodes->GetCoord(Point_Normal),
//                                                 geometry->nodes->GetCoord(iPoint), Coord_Reflected);
//        visc_numerics->SetCoord(geometry->nodes->GetCoord(iPoint), Coord_Reflected);
//
//        /*--- Primitive variables, and gradient ---*/
//
//        visc_numerics->SetPrimitive(V_domain, V_inflow);
//        visc_numerics->SetPrimVarGradient(nodes->GetGradient_Primitive(iPoint), nodes->GetGradient_Primitive(iPoint));
//
//        /*--- Turbulent kinetic energy ---*/
//
//        if (config->GetKind_Turb_Model() == TURB_MODEL::SST)
//          visc_numerics->SetTurbKineticEnergy(solver_container[TURB_SOL]->GetNodes()->GetSolution(iPoint,0),
//                                              solver_container[TURB_SOL]->GetNodes()->GetSolution(iPoint,0));
//
//        /*--- Compute and update residual ---*/
//
//        auto residual = visc_numerics->ComputeResidual(config);
//        LinSysRes.SubtractBlock(iPoint, residual);
//
//        /*--- Jacobian contribution for implicit integration ---*/
//
//        if (implicit)
//          Jacobian.SubtractBlock2Diag(iPoint, residual.jacobian_i);
//
//      }

    }
  }
  END_SU2_OMP_FOR

  delete [] Normal;

}

void CEulerSolver::BC_Engine_Exhaust(CGeometry *geometry, CSolver **solver_container, CNumerics *conv_numerics, CNumerics *visc_numerics, CConfig *config, unsigned short val_marker) {
  SU2_ZONE_SCOPED

  unsigned short iDim;
  unsigned long iVertex, iPoint;
  su2double Exhaust_Pressure, Exhaust_Temperature, Velocity[3], Velocity2, H_Exhaust, Temperature, Riemann, Area, UnitNormal[3], Pressure, Density, Energy, Mach2, SoundSpeed2, SoundSpeed_Exhaust2, Vel_Mag, alpha, aa, bb, cc, dd, Flow_Dir[3];
  su2double *V_exhaust, *V_domain, Target_Exhaust_Pressure, Exhaust_Pressure_old, Exhaust_Pressure_inc;

  su2double Gas_Constant = config->GetGas_ConstantND();
  bool implicit = (config->GetKind_TimeIntScheme() == EULER_IMPLICIT);
  string Marker_Tag = config->GetMarker_All_TagBound(val_marker);
  bool tkeNeeded = (config->GetKind_Turb_Model() == TURB_MODEL::SST);
  su2double DampingFactor = config->GetDamp_Engine_Exhaust();
  su2double Baseline_Press = 0.75 * config->GetPressure_FreeStreamND();

  auto *Normal = new su2double[nDim];

  /*--- Retrieve the specified exhaust pressure in the engine (non-dimensional). ---*/

  Target_Exhaust_Pressure = config->GetExhaust_Pressure_Target(Marker_Tag) / config->GetPressure_Ref();

  /*--- Retrieve the old exhaust pressure in the engine exhaust (this has been computed in a preprocessing). ---*/

  Exhaust_Pressure_old = config->GetExhaust_Pressure(Marker_Tag);

  /*--- Compute the Pressure increment ---*/

  Exhaust_Pressure_inc = (1.0 - (Exhaust_Pressure_old/Target_Exhaust_Pressure)) * Baseline_Press;

  /*--- Estimate the new exhaust pressure ---*/

  Exhaust_Pressure = (1.0 - DampingFactor) * Exhaust_Pressure_old + DampingFactor * (Exhaust_Pressure_old + Exhaust_Pressure_inc);

  /*--- The temperature is given (no iteration is required) ---*/

  Exhaust_Temperature  = config->GetExhaust_Temperature_Target(Marker_Tag);
  Exhaust_Temperature /= config->GetTemperature_Ref();

  /*--- The pressure is given (no iteration is required) ---*/

  Exhaust_Pressure  = config->GetExhaust_Pressure_Target(Marker_Tag);
  Exhaust_Pressure /= config->GetPressure_Ref();

  /*--- Loop over all the vertices on this boundary marker ---*/

  SU2_OMP_FOR_DYN(OMP_MIN_SIZE)
  for (iVertex = 0; iVertex < geometry->nVertex[val_marker]; iVertex++) {

    /*--- Allocate the value at the exhaust ---*/

    V_exhaust = GetCharacPrimVar(val_marker, iVertex);

    iPoint = geometry->vertex[val_marker][iVertex]->GetNode();

    /*--- Check if the node belongs to the domain (i.e, not a halo node) ---*/

    if (geometry->nodes->GetDomain(iPoint)) {

      /*--- Normal vector for this vertex (negate for outward convention) ---*/

      geometry->vertex[val_marker][iVertex]->GetNormal(Normal);
      for (iDim = 0; iDim < nDim; iDim++) Normal[iDim] = -Normal[iDim];

      Area = GeometryToolbox::Norm(nDim, Normal);
      for (iDim = 0; iDim < nDim; iDim++)
        UnitNormal[iDim] = Normal[iDim]/Area;

      /*--- Current solution at this boundary node ---*/

      V_domain = nodes->GetPrimitive(iPoint);

      /*--- Subsonic inflow: there is one outgoing characteristic (u-c),
       therefore we can specify all but one state variable at the inlet.
       The outgoing Riemann invariant provides the final piece of info. ---*/

      /*--- Store primitives and set some variables for clarity. ---*/

      Density = V_domain[nDim+2];
      Velocity2 = 0.0;
      for (iDim = 0; iDim < nDim; iDim++) {
        Velocity[iDim] = V_domain[iDim+1];
        Velocity2 += Velocity[iDim]*Velocity[iDim];
      }
      Energy      = V_domain[nDim+3] - V_domain[nDim+1]/V_domain[nDim+2];
      /*--- SST: the total enthalpy of the domain contains k, the imposed total enthalpy does not. ---*/
      if (tkeNeeded) Energy -= solver_container[TURB_SOL]->GetNodes()->GetSolution(iPoint,0);
      Pressure    = V_domain[nDim+1];
      H_Exhaust   = (Gamma*Gas_Constant/Gamma_Minus_One)*Exhaust_Temperature;
      SoundSpeed2 = Gamma*Pressure/Density;

      /*--- Compute the acoustic Riemann invariant that is extrapolated
       from the domain interior. ---*/

      Riemann   = 2.0*sqrt(SoundSpeed2)/Gamma_Minus_One;
      for (iDim = 0; iDim < nDim; iDim++)
        Riemann += Velocity[iDim]*UnitNormal[iDim];

      /*--- Total speed of sound ---*/

      SoundSpeed_Exhaust2 = Gamma_Minus_One*(H_Exhaust - (Energy + Pressure/Density)+0.5*Velocity2) + SoundSpeed2;

      /*--- The flow direction is defined by the surface normal ---*/

      for (iDim = 0; iDim < nDim; iDim++)
        Flow_Dir[iDim] = -UnitNormal[iDim];

      /*--- Dot product of normal and flow direction. This should
       be negative due to outward facing boundary normal convention. ---*/

      alpha = 0.0;
      for (iDim = 0; iDim < nDim; iDim++)
        alpha += UnitNormal[iDim]*Flow_Dir[iDim];

      /*--- Coefficients in the quadratic equation for the velocity ---*/

      aa =  1.0 + 0.5*Gamma_Minus_One*alpha*alpha;
      bb = -1.0*Gamma_Minus_One*alpha*Riemann;
      cc =  0.5*Gamma_Minus_One*Riemann*Riemann - 2.0*SoundSpeed_Exhaust2/Gamma_Minus_One;

      /*--- Solve quadratic equation for velocity magnitude. Value must
       be positive, so the choice of root is clear. ---*/

      dd      = bb*bb - 4.0*aa*cc;
      dd      = sqrt(max(0.0, dd));
      Vel_Mag = (-bb + dd)/(2.0*aa);

      if (Vel_Mag >= 0.0) {

        Velocity2 = Vel_Mag*Vel_Mag;

        /*--- Compute speed of sound from total speed of sound eqn. ---*/

        SoundSpeed2 = SoundSpeed_Exhaust2 - 0.5*Gamma_Minus_One*Velocity2;
        Mach2       = Velocity2/SoundSpeed2;
        Velocity2   = Mach2*SoundSpeed2;
        Vel_Mag     = sqrt(Velocity2);
        SoundSpeed2 = SoundSpeed_Exhaust2 - 0.5*Gamma_Minus_One*Velocity2;

        /*--- Compute new velocity vector at the inlet ---*/

        for (iDim = 0; iDim < nDim; iDim++)
          Velocity[iDim] = Vel_Mag*Flow_Dir[iDim];

        /*--- Static temperature from the speed of sound relation ---*/

        Temperature = SoundSpeed2/(Gamma*Gas_Constant);

        /*--- Static pressure using isentropic relation at a point ---*/

        Pressure = Exhaust_Pressure*pow((Temperature/Exhaust_Temperature), Gamma/Gamma_Minus_One);

        /*--- Density at the exhaust from the gas law ---*/

        Density = Pressure/(Gas_Constant*Temperature);

        /*--- Using pressure, density, & velocity, compute the energy ---*/

        Energy = Pressure/(Density*Gamma_Minus_One) + 0.5*Velocity2;
        if (tkeNeeded) Energy += solver_container[TURB_SOL]->GetNodes()->GetSolution(iPoint,0);  // node k, see #1851

        /*--- Primitive variables, using the derived quantities ---*/

        V_exhaust[0] = Temperature;
        for (iDim = 0; iDim < nDim; iDim++)
          V_exhaust[iDim+1] = Velocity[iDim];
        V_exhaust[nDim+1] = Pressure;
        V_exhaust[nDim+2] = Density;
        V_exhaust[nDim+3] = Energy + Pressure/Density;
        V_exhaust[nDim+4] = sqrt(SoundSpeed2);

      }
      /*--- The flow goes in the wrong direction ---*/

      else {

        V_exhaust[0] = V_domain[0];
        for (iDim = 0; iDim < nDim; iDim++)
          V_exhaust[iDim+1] = V_domain[iDim+1];
        V_exhaust[nDim+1] = V_domain[nDim+1];
        V_exhaust[nDim+2] = V_domain[nDim+2];
        V_exhaust[nDim+3] = V_domain[nDim+3];
        V_exhaust[nDim+4] = V_domain[nDim+4];

      }

      /*--- Set various quantities in the solver class ---*/

      conv_numerics->SetNormal(Normal);
      conv_numerics->SetPrimitive(V_domain, V_exhaust);
      /*--- SST: both states contain the k of the node in their total enthalpy. ---*/
      if (config->GetKind_Turb_Model() == TURB_MODEL::SST) {
        const su2double tke = solver_container[TURB_SOL]->GetNodes()->GetSolution(iPoint, 0);
        conv_numerics->SetTurbKineticEnergy(tke, tke);
      }

      /*--- Set grid movement ---*/

      if (dynamic_grid)
        conv_numerics->SetGridVel(geometry->nodes->GetGridVel(iPoint), geometry->nodes->GetGridVel(iPoint));

      /*--- Compute the residual using an upwind scheme ---*/

      auto residual = conv_numerics->ComputeResidual(config);

      LinSysRes.AddBlock(iPoint, residual);

      /*--- Jacobian contribution for implicit integration ---*/

      if (implicit)
        Jacobian.AddBlock2Diag(iPoint, residual.jacobian_i);

//      /*--- Viscous contribution, commented out because serious convergence problems ---*/
//
//      if (viscous) {
//
//        /*--- Set laminar and eddy viscosity at the infinity ---*/
//
//        V_exhaust[nDim+5] = nodes->GetLaminarViscosity(iPoint);
//        V_exhaust[nDim+6] = nodes->GetEddyViscosity(iPoint);
//
//        /*--- Set the normal vector and the coordinates ---*/
//
//        visc_numerics->SetNormal(Normal);
//        su2double Coord_Reflected[MAXNDIM];
//        GeometryToolbox::PointPointReflect(nDim, geometry->nodes->GetCoord(Point_Normal),
//                                                 geometry->nodes->GetCoord(iPoint), Coord_Reflected);
//        visc_numerics->SetCoord(geometry->nodes->GetCoord(iPoint), Coord_Reflected);
//
//        /*--- Primitive variables, and gradient ---*/
//
//        visc_numerics->SetPrimitive(V_domain, V_exhaust);
//        visc_numerics->SetPrimVarGradient(nodes->GetGradient_Primitive(iPoint), nodes->GetGradient_Primitive(iPoint));
//
//        /*--- Turbulent kinetic energy ---*/
//
//        if (config->GetKind_Turb_Model() == TURB_MODEL::SST)
//          visc_numerics->SetTurbKineticEnergy(solver_container[TURB_SOL]->GetNodes()->GetSolution(iPoint,0),
//                                              solver_container[TURB_SOL]->GetNodes()->GetSolution(iPoint,0));
//
//        /*--- Compute and update residual ---*/
//
//        auto residual = visc_numerics->ComputeResidual(config)
//        LinSysRes.SubtractBlock(iPoint, residual);
//
//        /*--- Jacobian contribution for implicit integration ---*/
//
//        if (implicit)
//          Jacobian.SubtractBlock2Diag(iPoint, residual.jacobian_i);
//
//      }

    }
  }
  END_SU2_OMP_FOR

  delete [] Normal;

}

void CEulerSolver::BC_ActDisk_Inlet(CGeometry *geometry, CSolver **solver_container, CNumerics *conv_numerics, CNumerics *visc_numerics,
                                    CConfig *config, unsigned short val_marker) {
  SU2_ZONE_SCOPED

  unsigned short Kind_ActDisk = config->GetKind_ActDisk();

  if (Kind_ActDisk == VARIABLE_LOAD || Kind_ActDisk == BLADE_ELEMENT) {
    BC_ActDisk_VariableLoad(geometry, solver_container, conv_numerics, visc_numerics, config, val_marker, true);
  }
  else{
    BC_ActDisk(geometry, solver_container, conv_numerics, visc_numerics, config, val_marker, true);
  }

}

void CEulerSolver::BC_ActDisk_Outlet(CGeometry *geometry, CSolver **solver_container, CNumerics *conv_numerics, CNumerics *visc_numerics,
                                     CConfig *config, unsigned short val_marker) {
  SU2_ZONE_SCOPED

  unsigned short Kind_ActDisk = config->GetKind_ActDisk();

  if (Kind_ActDisk == VARIABLE_LOAD || Kind_ActDisk == BLADE_ELEMENT) {
    BC_ActDisk_VariableLoad(geometry, solver_container, conv_numerics, visc_numerics, config, val_marker, false);
  }
  else{
    BC_ActDisk(geometry, solver_container, conv_numerics, visc_numerics, config, val_marker, false);
  }

}

void CEulerSolver::BC_ActDisk(CGeometry *geometry, CSolver **solver_container, CNumerics *conv_numerics, CNumerics *visc_numerics,
                              CConfig *config, unsigned short val_marker, bool val_inlet_surface) {
  SU2_ZONE_SCOPED

  unsigned short iDim;
  unsigned long iVertex, iPoint, GlobalIndex_donor, GlobalIndex;
  su2double Pressure, Velocity[3], Target_Press_Jump, Target_Temp_Jump,
  Velocity2, Entropy, Density, Energy, Riemann, Vn, SoundSpeed, Vn_Inlet, Mach_Outlet,
  Area, UnitNormal[3], *V_outlet, *V_domain, *V_inlet, P_Total, T_Total, H_Total, Temperature,
  Mach2, SoundSpeed2, SoundSpeed_Total2, Vel_Mag, alpha, aa, bb, cc, dd;
  su2double Factor, P_static, T_static, SoS_outlet, Rho_outlet, Rho_inlet;
  su2double Vel_normal_inlet[3], Vel_tangent_inlet[3], Vel_inlet[3];
  su2double Vel_normal_outlet[3], Vel_tangent_outlet[3], Vel_outlet[3];
  su2double Vel_normal_inlet_, Vel_tangent_inlet_, Vel_inlet_;
  su2double Vel_normal_outlet_, Vel_outlet_;

  su2double Pressure_out, Density_out, SoundSpeed_out, Velocity2_out,
  Mach_out, Pressure_in, Density_in, SoundSpeed_in, Velocity2_in,
  Mach_in, PressureAdj, TemperatureAdj;

  bool implicit           = (config->GetKind_TimeIntScheme() == EULER_IMPLICIT);
  su2double Gas_Constant  = config->GetGas_ConstantND();
  bool tkeNeeded          = (config->GetKind_Turb_Model() == TURB_MODEL::SST);
  bool ratio              = (config->GetActDisk_Jump() == RATIO);
  su2double SecondaryFlow = config->GetSecondaryFlow_ActDisk();

  auto *Normal = new su2double[nDim];
  auto *Flow_Dir = new su2double[nDim];

  /*--- Loop over all the vertices on this boundary marker ---*/

  SU2_OMP_FOR_DYN(OMP_MIN_SIZE)
  for (iVertex = 0; iVertex < geometry->nVertex[val_marker]; iVertex++) {

    iPoint = geometry->vertex[val_marker][iVertex]->GetNode();
    GlobalIndex = geometry->nodes->GetGlobalIndex(iPoint);
    GlobalIndex_donor = GetDonorGlobalIndex(val_marker, iVertex);

    /*--- Check if the node belongs to the domain (i.e., not a halo node) ---*/

    if ((geometry->nodes->GetDomain(iPoint)) &&
        (GlobalIndex != GlobalIndex_donor)) {

      /*--- Normal vector for this vertex (negative for outward convention) ---*/

      geometry->vertex[val_marker][iVertex]->GetNormal(Normal);
      for (iDim = 0; iDim < nDim; iDim++) Normal[iDim] = -Normal[iDim];
      conv_numerics->SetNormal(Normal);

      Area = GeometryToolbox::Norm(nDim, Normal);
      for (iDim = 0; iDim < nDim; iDim++)
        UnitNormal[iDim] = Normal[iDim]/Area;

      /*--- Current solution at this boundary node and jumps values ---*/

      V_domain = nodes->GetPrimitive(iPoint);
      Target_Press_Jump = ActDisk_DeltaP[val_marker][iVertex];
      Target_Temp_Jump = ActDisk_DeltaT[val_marker][iVertex];

      if (val_inlet_surface) {
        V_inlet  = nodes->GetPrimitive(iPoint);
        V_outlet = DonorPrimVar[val_marker][iVertex];

        Pressure_out    = V_outlet[nDim+1];
        Density_out     = V_outlet[nDim+2];
        SoundSpeed_out  = sqrt(Gamma*Pressure_out/Density_out);

        Pressure_in    = V_inlet[nDim+1];
        Density_in     = V_inlet[nDim+2];
        SoundSpeed_in  = sqrt(Gamma*Pressure_in/Density_in);

        Velocity2_out = 0.0; Velocity2_in = 0.0;
        for (iDim = 0; iDim < nDim; iDim++) {
          Velocity2_out += V_outlet[iDim+1]*V_outlet[iDim+1];
          Velocity2_in  += V_inlet[iDim+1]*V_inlet[iDim+1];
        }

        PressureAdj = 1.0; TemperatureAdj = 1.0;
        if ((Velocity2_out > 0.0) && (Velocity2_in > 0.0)) {

          Mach_out = sqrt(Velocity2_out)/SoundSpeed_out;
          Mach_in  = sqrt(Velocity2_in)/SoundSpeed_in;

          PressureAdj    = pow( 1.0 + Mach_out * Mach_out * 0.5 * (Gamma - 1.0), Gamma / (Gamma - 1.0)) /
          pow( 1.0 + Mach_in * Mach_in * 0.5 * (Gamma - 1.0), Gamma / (Gamma - 1.0));
          TemperatureAdj = (1.0 + Mach_out * Mach_out * 0.5 * (Gamma - 1.0)) /
          (1.0 + Mach_in * Mach_in * 0.5 * (Gamma - 1.0));

        }

        if (ratio) {
          P_static = V_outlet[nDim+1] / (Target_Press_Jump/PressureAdj);
          T_static = V_outlet[0] / (Target_Temp_Jump/TemperatureAdj);
        }
        else { P_static = V_outlet[nDim+1] - Target_Press_Jump; T_static = V_outlet[0] - Target_Temp_Jump; }
      }
      else {
        V_outlet = nodes->GetPrimitive(iPoint);
        V_inlet  = DonorPrimVar[val_marker][iVertex];

        Pressure_out    = V_outlet[nDim+1];
        Density_out     = V_outlet[nDim+2];
        SoundSpeed_out  = sqrt(Gamma*Pressure_out/Density_out);

        Pressure_in    = V_inlet[nDim+1];
        Density_in     = V_inlet[nDim+2];
        SoundSpeed_in  = sqrt(Gamma*Pressure_in/Density_in);

        Velocity2_out = 0.0; Velocity2_in = 0.0;
        for (iDim = 0; iDim < nDim; iDim++) {
          Velocity2_out += V_outlet[iDim+1]*V_outlet[iDim+1];
          Velocity2_in  += V_inlet[iDim+1]*V_inlet[iDim+1];
        }

        PressureAdj = 1.0; TemperatureAdj = 1.0;
        if ((Velocity2_out > 0.0) && (Velocity2_in > 0.0)) {

          Mach_out = sqrt(Velocity2_out)/SoundSpeed_out;
          Mach_in  = sqrt(Velocity2_in)/SoundSpeed_in;

          PressureAdj    = pow( 1.0 + Mach_out * Mach_out * 0.5 * (Gamma - 1.0), Gamma / (Gamma - 1.0)) /
          pow( 1.0 + Mach_in * Mach_in * 0.5 * (Gamma - 1.0), Gamma / (Gamma - 1.0));
          TemperatureAdj = (1.0 + Mach_out * Mach_out * 0.5 * (Gamma - 1.0)) /
          (1.0 + Mach_in * Mach_in * 0.5 * (Gamma - 1.0));
        }

        if (ratio) {
          P_static = V_inlet[nDim+1] * (Target_Press_Jump/PressureAdj);
          T_static = V_inlet[0] * (Target_Temp_Jump/TemperatureAdj);
        }
        else       { P_static = V_inlet[nDim+1] + Target_Press_Jump; T_static = V_inlet[0] + Target_Temp_Jump; }
      }

      /*--- Subsonic inlet ---*/

      if (val_inlet_surface) {

        /*--- Build the fictitious intlet state based on characteristics.
         Retrieve the specified back pressure for this inlet ---*/

        Density = V_domain[nDim+2];
        Velocity2 = 0.0; Vn = 0.0;
        for (iDim = 0; iDim < nDim; iDim++) {
          Velocity[iDim] = V_domain[iDim+1];
          Velocity2 += Velocity[iDim]*Velocity[iDim];
          Vn += Velocity[iDim]*UnitNormal[iDim];
        }
        Pressure   = V_domain[nDim+1];
        SoundSpeed = sqrt(Gamma*Pressure/Density);

        Entropy = Pressure*pow(1.0/Density, Gamma);
        Riemann = Vn + 2.0*SoundSpeed/Gamma_Minus_One;

        /*--- Compute the new fictious state at the outlet ---*/

        Pressure   = P_static;
        Density    = pow(Pressure/Entropy,1.0/Gamma);
        SoundSpeed = sqrt(Gamma*Pressure/Density);
        Vn_Inlet    = Riemann - 2.0*SoundSpeed/Gamma_Minus_One;

        Velocity2  = 0.0;
        for (iDim = 0; iDim < nDim; iDim++) {
          Velocity[iDim] = Velocity[iDim] + (Vn_Inlet-Vn)*UnitNormal[iDim];
          Velocity2 += Velocity[iDim]*Velocity[iDim];
        }
        Energy = Pressure/(Density*Gamma_Minus_One) + 0.5*Velocity2;
        if (tkeNeeded) Energy += solver_container[TURB_SOL]->GetNodes()->GetSolution(iPoint,0);  // node k, see #1851

        /*--- Conservative variables, using the derived quantities ---*/

        V_inlet[0] = Pressure / ( Gas_Constant * Density);
        for (iDim = 0; iDim < nDim; iDim++)
          V_inlet[iDim+1] = Velocity[iDim];
        V_inlet[nDim+1] = Pressure;
        V_inlet[nDim+2] = Density;
        V_inlet[nDim+3] = Energy + Pressure/Density;
        V_inlet[nDim+4] = SoundSpeed;
        conv_numerics->SetPrimitive(V_domain, V_inlet);
        /*--- SST: both states contain the k of the node in their total enthalpy. ---*/
        if (config->GetKind_Turb_Model() == TURB_MODEL::SST) {
          const su2double tke = solver_container[TURB_SOL]->GetNodes()->GetSolution(iPoint, 0);
          conv_numerics->SetTurbKineticEnergy(tke, tke);
        }

      }

      /*--- Subsonic outlet ---*/

      else {

        GetFluidModel()->SetTDState_PT(P_static, T_static);
        SoS_outlet = GetFluidModel()->GetSoundSpeed();
        Rho_outlet = GetFluidModel()->GetDensity();

        /*--- We use the velocity and the density from the flow inlet
         to evaluate flow direction and mass flow ---*/

        Rho_inlet = V_inlet[nDim+2];
        for (iDim = 0; iDim < nDim; iDim++)
          Vel_inlet[iDim] = V_inlet[iDim+1];

        Vel_normal_inlet_ = 0.0; Vel_inlet_ = 0.0;
        for (iDim = 0; iDim < nDim; iDim++) {
          Vel_normal_inlet[iDim] = -Vel_inlet[iDim]*UnitNormal[iDim];
          Vel_normal_inlet_ += Vel_normal_inlet[iDim]*Vel_normal_inlet[iDim];
          Vel_inlet_+= Vel_inlet[iDim]*Vel_inlet[iDim];
        }
        Vel_inlet_ = sqrt(Vel_inlet_);
        Vel_normal_inlet_ = sqrt(Vel_normal_inlet_);

        Vel_tangent_inlet_ = 0.0;
        for (iDim = 0; iDim < nDim; iDim++) {
          Vel_tangent_inlet[iDim] = Vel_inlet[iDim] - Vel_normal_inlet[iDim];
          Vel_tangent_inlet_ += Vel_tangent_inlet[iDim]*Vel_tangent_inlet[iDim];
        }
        Vel_tangent_inlet_ = sqrt(Vel_tangent_inlet_);

        /*--- Mass flow conservation (normal direction) and
         no jump in the tangential velocity ---*/

        Vel_normal_outlet_ = (1.0-SecondaryFlow/100.0)*(Rho_inlet*Vel_normal_inlet_)/Rho_outlet;

        Vel_outlet_ = 0.0;
        for (iDim = 0; iDim < nDim; iDim++) {
          Vel_normal_outlet[iDim] = -Vel_normal_outlet_*UnitNormal[iDim];
          Vel_tangent_outlet[iDim] = Vel_tangent_inlet[iDim];
          Vel_outlet[iDim] = Vel_normal_outlet[iDim] + Vel_tangent_outlet[iDim];
          Vel_outlet_ += Vel_outlet[iDim]*Vel_outlet[iDim];
        }
        Vel_outlet_ = sqrt(Vel_outlet_);

        Mach_Outlet = min(Vel_outlet_/SoS_outlet, 1.0);

        /*--- Reevaluate the Total Pressure and Total Temperature using the
         Fan Face Mach number and the static values from the jum condition ---*/

        Factor = 1.0 + 0.5*Mach_Outlet*Mach_Outlet*Gamma_Minus_One;
        P_Total = P_static * pow(Factor, Gamma/Gamma_Minus_One);
        T_Total = T_static * Factor;

        /*--- Flow direction using the velocity direction at the outlet  ---*/

        if (Vel_outlet_ != 0.0) {
          for (iDim = 0; iDim < nDim; iDim++) Flow_Dir[iDim] = Vel_outlet[iDim]/Vel_outlet_;
        }
        else {
          for (iDim = 0; iDim < nDim; iDim++) Flow_Dir[iDim] = 0.0;
        }

        /*--- Store primitives and set some variables for clarity. ---*/

        Density = V_domain[nDim+2];
        Velocity2 = 0.0;
        for (iDim = 0; iDim < nDim; iDim++) {
          Velocity[iDim] = V_domain[iDim+1];
          Velocity2 += Velocity[iDim]*Velocity[iDim];
        }
        Energy      = V_domain[nDim+3] - V_domain[nDim+1]/V_domain[nDim+2];
        /*--- SST: the total enthalpy of the domain contains k, the imposed total enthalpy does not. ---*/
        if (tkeNeeded) Energy -= solver_container[TURB_SOL]->GetNodes()->GetSolution(iPoint,0);
        Pressure    = V_domain[nDim+1];
        H_Total     = (Gamma*Gas_Constant/Gamma_Minus_One)*T_Total;
        SoundSpeed2 = Gamma*Pressure/Density;

        /*--- Compute the acoustic Riemann invariant that is extrapolated
         from the domain interior. ---*/

        Riemann   = 2.0*sqrt(SoundSpeed2)/Gamma_Minus_One;
        for (iDim = 0; iDim < nDim; iDim++)
          Riemann += Velocity[iDim]*UnitNormal[iDim];

        /*--- Total speed of sound ---*/

        SoundSpeed_Total2 = Gamma_Minus_One*(H_Total - (Energy + Pressure/Density)+0.5*Velocity2) + SoundSpeed2;

        /*--- Dot product of normal and flow direction. This should
         be negative due to outward facing boundary normal convention. ---*/

        alpha = 0.0;
        for (iDim = 0; iDim < nDim; iDim++)
          alpha += UnitNormal[iDim]*Flow_Dir[iDim];

        /*--- Coefficients in the quadratic equation for the velocity ---*/

        aa =  1.0 + 0.5*Gamma_Minus_One*alpha*alpha;
        bb = -1.0*Gamma_Minus_One*alpha*Riemann;
        cc =  0.5*Gamma_Minus_One*Riemann*Riemann - 2.0*SoundSpeed_Total2/Gamma_Minus_One;

        /*--- Solve quadratic equation for velocity magnitude. Value must
         be positive, so the choice of root is clear. ---*/

        dd = bb*bb - 4.0*aa*cc;
        dd = sqrt(max(0.0, dd));
        Vel_Mag   = (-bb + dd)/(2.0*aa);
        Vel_Mag   = max(0.0, Vel_Mag);
        Velocity2 = Vel_Mag*Vel_Mag;

        /*--- Compute speed of sound from total speed of sound eqn. ---*/

        SoundSpeed2 = SoundSpeed_Total2 - 0.5*Gamma_Minus_One*Velocity2;

        /*--- Mach squared (cut between 0-1), use to adapt velocity ---*/

        Mach2 = min(1.0, Velocity2/SoundSpeed2);
        Velocity2   = Mach2*SoundSpeed2;
        Vel_Mag     = sqrt(Velocity2);
        SoundSpeed2 = SoundSpeed_Total2 - 0.5*Gamma_Minus_One*Velocity2;

        /*--- Compute new velocity vector at the exit ---*/

        for (iDim = 0; iDim < nDim; iDim++)
          Velocity[iDim] = Vel_Mag*Flow_Dir[iDim];

        /*--- Static temperature from the speed of sound relation ---*/

        Temperature = SoundSpeed2/(Gamma*Gas_Constant);

        /*--- Static pressure using isentropic relation at a point ---*/

        Pressure = P_Total*pow((Temperature/T_Total), Gamma/Gamma_Minus_One);

        /*--- Density at the inlet from the gas law ---*/

        Density = Pressure/(Gas_Constant*Temperature);

        /*--- Using pressure, density, & velocity, compute the energy ---*/

        Energy = Pressure/(Density*Gamma_Minus_One) + 0.5*Velocity2;
        if (tkeNeeded) Energy += solver_container[TURB_SOL]->GetNodes()->GetSolution(iPoint,0);  // node k, see #1851

        /*--- Primitive variables, using the derived quantities ---*/

        V_outlet[0] = Temperature;
        for (iDim = 0; iDim < nDim; iDim++)
          V_outlet[iDim+1] = Velocity[iDim];
        V_outlet[nDim+1] = Pressure;
        V_outlet[nDim+2] = Density;
        V_outlet[nDim+3] = Energy + Pressure/Density;
        V_outlet[nDim+4] = sqrt(SoundSpeed2);
        conv_numerics->SetPrimitive(V_domain, V_outlet);
        /*--- SST: both states contain the k of the node in their total enthalpy. ---*/
        if (config->GetKind_Turb_Model() == TURB_MODEL::SST) {
          const su2double tke = solver_container[TURB_SOL]->GetNodes()->GetSolution(iPoint, 0);
          conv_numerics->SetTurbKineticEnergy(tke, tke);
        }

      }

      /*--- Grid Movement ---*/

      if (dynamic_grid)
        conv_numerics->SetGridVel(geometry->nodes->GetGridVel(iPoint), geometry->nodes->GetGridVel(iPoint));

      /*--- Compute the residual using an upwind scheme ---*/

      auto residual = conv_numerics->ComputeResidual(config);

      /*--- Update residual value ---*/

      LinSysRes.AddBlock(iPoint, residual);

      /*--- Jacobian contribution for implicit integration ---*/

      if (implicit) Jacobian.AddBlock2Diag(iPoint, residual.jacobian_i);

//      /*--- Viscous contribution, commented out because serious convergence problems ---*/
//
//      if (viscous) {
//
//        /*--- Set laminar and eddy viscosity at the infinity ---*/
//
//        if (val_inlet_surface) {
//          V_inlet[nDim+5] = nodes->GetLaminarViscosity(iPoint);
//          V_inlet[nDim+6] = nodes->GetEddyViscosity(iPoint);
//        }
//        else {
//          V_outlet[nDim+5] = nodes->GetLaminarViscosity(iPoint);
//          V_outlet[nDim+6] = nodes->GetEddyViscosity(iPoint);
//        }
//
//        /*--- Set the normal vector and the coordinates ---*/
//
//        visc_numerics->SetNormal(Normal);
//        su2double Coord_Reflected[MAXNDIM];
//        GeometryToolbox::PointPointReflect(nDim, geometry->nodes->GetCoord(Point_Normal),
//                                                 geometry->nodes->GetCoord(iPoint), Coord_Reflected);
//        visc_numerics->SetCoord(geometry->nodes->GetCoord(iPoint), Coord_Reflected);
//
//        /*--- Primitive variables, and gradient ---*/
//
//        if (val_inlet_surface) visc_numerics->SetPrimitive(V_domain, V_inlet);
//        else visc_numerics->SetPrimitive(V_domain, V_outlet);
//
//        visc_numerics->SetPrimVarGradient(nodes->GetGradient_Primitive(iPoint), nodes->GetGradient_Primitive(iPoint));
//
//        /*--- Turbulent kinetic energy ---*/
//
//        if (config->GetKind_Turb_Model() == TURB_MODEL::SST)
//          visc_numerics->SetTurbKineticEnergy(solver_container[TURB_SOL]->GetNodes()->GetSolution(iPoint,0),
//                                              solver_container[TURB_SOL]->GetNodes()->GetSolution(iPoint,0));
//
//        /*--- Compute and update residual ---*/
//
//        auto residual = visc_numerics->ComputeResidual(config);
//        LinSysRes.SubtractBlock(iPoint, residual);
//
//        /*--- Jacobian contribution for implicit integration ---*/
//
//        if (implicit) Jacobian.SubtractBlock2Diag(iPoint, residual.jacobian_i);
//
//      }

    }

  }
  END_SU2_OMP_FOR

  /*--- Free locally allocated memory ---*/

  delete [] Normal;
  delete [] Flow_Dir;

}

void CEulerSolver::BC_ActDisk_VariableLoad(CGeometry *geometry, CSolver **solver_container, CNumerics *conv_numerics, CNumerics *visc_numerics,
                              CConfig *config, unsigned short val_marker, bool val_inlet_surface) {
  SU2_ZONE_SCOPED

  /*!
   * \function BC_ActDisk_VariableLoad
   * \brief Actuator disk model with variable load along disk radius.
   * \author: E. Saetta, L. Russo, R. Tognaccini (GitHub references EttoreSaetta, lorenzorusso07, rtogna).
   * Theoretical and Applied Aerodynamics Research Group (TAARG), University of Naples Federico II.
   * First release date : July 1st 2020
   * modified on:
   *
   * Force coefficients distribution given in an input file. Actuator disk data initialized in function SetActDisk_BCThrust.
   * Entropy, acoustic Riemann invariant R+ and  tangential velocity extrapolated  from upstream flow;
   * acoustic Riemann invariant R- is extrapolated from downstream.
   * Hovering condition simulation not available yet: freestream velocity must be different than zero.
   */

  unsigned short iDim;
  unsigned long iVertex, iPoint, GlobalIndex_donor, GlobalIndex;
  su2double Pressure, Velocity[MAXNDIM],
  Velocity2, Entropy, Density, Energy, Riemann, Vn, SoundSpeed, Vn_Inlet,
  Area, UnitNormal[MAXNDIM] = {0.0}, *V_outlet, *V_domain, *V_inlet;

  su2double Pressure_out, Density_out,
  Pressure_in, Density_in;

  su2double Prop_Axis[MAXNDIM];
  su2double Fa, Fx, Fy, Fz;
  su2double u_in, v_in, w_in, u_out, v_out, w_out, uJ, vJ, wJ;
  su2double Temperature_out, H_in, H_out;
  su2double FQ, Q_out, Density_Disk;
  su2double SoSextr, Vnextr[MAXNDIM], Vnextr_, RiemannExtr, QdMnorm[MAXNDIM], QdMnorm2, appo2, SoS_out;
  su2double Normal[MAXNDIM];

  const bool implicit = (config->GetKind_TimeIntScheme() == EULER_IMPLICIT);
  const auto Gas_Constant = config->GetGas_ConstantND();
  const bool tkeNeeded = (config->GetKind_Turb_Model() == TURB_MODEL::SST);

  unsigned short Kind_ActDisk = config->GetKind_ActDisk();

  /*--- Get the actuator disk center and axis coordinates for the current marker. ---*/
  for (iDim = 0; iDim < nDim; iDim++){
    Prop_Axis[iDim] = ActDisk_Axis(val_marker, iDim);
  }

  /*--- Loop over all the vertices on this boundary marker. ---*/
  SU2_OMP_FOR_DYN(OMP_MIN_SIZE)
  for (iVertex = 0; iVertex < geometry->nVertex[val_marker]; iVertex++) {

    iPoint = geometry->vertex[val_marker][iVertex]->GetNode();
    GlobalIndex = geometry->nodes->GetGlobalIndex(iPoint);
    GlobalIndex_donor = GetDonorGlobalIndex(val_marker, iVertex);

    /*--- Check if the node belongs to the domain (i.e., not a halo node) ---*/

    if ((geometry->nodes->GetDomain(iPoint)) &&
       (GlobalIndex != GlobalIndex_donor)) {

      /*--- Normal vector for this vertex (negative for outward convention) ---*/

      geometry->vertex[val_marker][iVertex]->GetNormal(Normal);
      for (iDim = 0; iDim < nDim; iDim++) Normal[iDim] = -Normal[iDim];
      conv_numerics->SetNormal(Normal);

      Area = GeometryToolbox::Norm(nDim, Normal);
      for (iDim = 0; iDim < nDim; iDim++)
            UnitNormal[iDim] = Normal[iDim]/Area;

      /*--- Current solution at this boundary node. ---*/

      V_domain = nodes->GetPrimitive(iPoint);

      /*--- Get the values of Fa (axial force per unit area), Fx, Fy and Fz (x, y and z components of the tangential and
            radial forces per unit area resultant). ---*/
      if (Kind_ActDisk == BLADE_ELEMENT) {
        Fa = ActDisk_Fa_BEM[val_marker][iVertex];
        Fx = ActDisk_Fx_BEM[val_marker][iVertex];
        Fy = ActDisk_Fy_BEM[val_marker][iVertex];
        Fz = ActDisk_Fz_BEM[val_marker][iVertex];
      } else { /*--- default (Kind_ActDisk == VARIABLE_LOAD) ---*/
        Fa = ActDisk_Fa[val_marker][iVertex];
        Fx = ActDisk_Fx[val_marker][iVertex];
        Fy = ActDisk_Fy[val_marker][iVertex];
        Fz = ActDisk_Fz[val_marker][iVertex];
      }

      /*--- Get the primitive variables and the extrapolated variables. ---*/
      if (val_inlet_surface){
        V_inlet = nodes->GetPrimitive(iPoint);
        V_outlet = DonorPrimVar[val_marker][iVertex];}
      else{
        V_outlet = nodes->GetPrimitive(iPoint);
        V_inlet = DonorPrimVar[val_marker][iVertex];}

      /*--- u, v and w are the three momentum components. ---*/
      Pressure_out    = V_outlet[nDim+1];
      Density_out     = V_outlet[nDim+2];
      u_out = V_outlet[1]*V_outlet[nDim+2];
      v_out = V_outlet[2]*V_outlet[nDim+2];
      w_out = V_outlet[3]*V_outlet[nDim+2];

      Pressure_in    = V_inlet[nDim+1];
      Density_in     = V_inlet[nDim+2];
      u_in = V_inlet[1]*Density_in;
      v_in = V_inlet[2]*Density_in;
      w_in = V_inlet[3]*Density_in;
      H_in = V_inlet[nDim+3]*Density_in;

      /*--- Density on the disk is computed as an everage value between the inlet and outlet values. ---*/
      Density_Disk = 0.5*(Density_in + Density_out);

      /*--- Computation of the normal momentum flowing through the disk. ---*/
      Q_out = 0.5*((u_in + u_out)*Prop_Axis[0] + (v_in + v_out)*Prop_Axis[1] + (w_in + w_out)*Prop_Axis[2]);

      FQ = Q_out/Density_Disk;

      /*--- Computation of the momentum jumps due to the tnagential and radial forces per unit area. ---*/
      if (FQ < EPS){
        uJ = 0.0;
        vJ = 0.0;
        wJ = 0.0;}
      else{
        uJ = Fx/FQ;
        vJ = Fy/FQ;
        wJ = Fz/FQ;}

      if (val_inlet_surface) {
        /*--- Build the fictitious intlet state based on characteristics.
              Retrieve the specified back pressure for this inlet ---*/

        Density = V_domain[nDim+2];
        Velocity2 = 0.0; Vn = 0.0;
        for (iDim = 0; iDim < nDim; iDim++) {
          Velocity[iDim] = V_domain[iDim+1];
          Velocity2 += Velocity[iDim]*Velocity[iDim];
          Vn += Velocity[iDim]*UnitNormal[iDim];
        }
        Pressure   = V_domain[nDim+1];
        SoundSpeed = sqrt(Gamma*Pressure/Density);

        Entropy = Pressure*pow(1.0/Density, Gamma);
        Riemann = Vn + 2.0*SoundSpeed/Gamma_Minus_One;

        /*--- Compute the new fictious state at the outlet ---*/

        Pressure   = Pressure_out - Fa;
        Density    = pow(Pressure/Entropy,1.0/Gamma);
        SoundSpeed = sqrt(Gamma*Pressure/Density);
        Vn_Inlet    = Riemann - 2.0*SoundSpeed/Gamma_Minus_One;

        Velocity2  = 0.0;
        for (iDim = 0; iDim < nDim; iDim++) {
          Velocity[iDim] = Velocity[iDim] + (Vn_Inlet-Vn)*UnitNormal[iDim];
          Velocity2 += Velocity[iDim]*Velocity[iDim];
        }
        Energy = Pressure/(Density*Gamma_Minus_One) + 0.5*Velocity2;
        if (tkeNeeded) Energy += solver_container[TURB_SOL]->GetNodes()->GetSolution(iPoint,0);  // node k, see #1851

        /*--- Conservative variables, using the derived quantities ---*/

        V_inlet[0] = Pressure / ( Gas_Constant * Density);
        for (iDim = 0; iDim < nDim; iDim++) V_inlet[iDim+1] = Velocity[iDim];
        V_inlet[nDim+1] = Pressure;
        V_inlet[nDim+2] = Density;
        V_inlet[nDim+3] = Energy + Pressure/Density;
        V_inlet[nDim+4] = SoundSpeed;
        conv_numerics->SetPrimitive(V_domain, V_inlet);
        /*--- SST: both states contain the k of the node in their total enthalpy. ---*/
        if (config->GetKind_Turb_Model() == TURB_MODEL::SST) {
          const su2double tke = solver_container[TURB_SOL]->GetNodes()->GetSolution(iPoint, 0);
          conv_numerics->SetTurbKineticEnergy(tke, tke);
        }
      }
      else {
        /*--- Acoustic Riemann invariant extrapolation form the interior domain. ---*/
        SoSextr = V_domain[nDim+4];

        Vnextr_ = 0.0;
        for (iDim = 0; iDim < nDim; iDim++){
          Vnextr[iDim] = V_domain[iDim+1]*Prop_Axis[iDim];
          Vnextr_ += Vnextr[iDim]*Vnextr[iDim];
        }
        Vnextr_ = sqrt(max(0.0,Vnextr_));
        RiemannExtr = Vnextr_ - ((2*SoSextr)/(Gamma_Minus_One));

        /*--- Assigning the momentum in tangential direction jump and the pressure jump. ---*/
        Velocity[0] = u_in + uJ;
        Velocity[1] = v_in + vJ;
        Velocity[2] = w_in + wJ;
        Pressure_out = Pressure_in + Fa;

        /*--- Computation of the momentum normal to the disk plane. ---*/
        QdMnorm[0] = u_in*Prop_Axis[0];
        QdMnorm[1] = v_in*Prop_Axis[1];
        QdMnorm[2] = w_in*Prop_Axis[2];

        QdMnorm2 = 0.0;
        for (iDim = 0; iDim < nDim; iDim++) QdMnorm2 += QdMnorm[iDim]*QdMnorm[iDim];

        /*--- Resolving the second grade equation for the density. ---*/
        appo2 = -((2*sqrt(QdMnorm2)*RiemannExtr)+((4*Gamma*Pressure_out)/(pow(Gamma_Minus_One,2))));
        Density_out = (-appo2+sqrt(max(0.0,pow(appo2,2)-4*QdMnorm2*pow(RiemannExtr,2))))/(2*pow(RiemannExtr,2));

        Velocity2 = 0;
        for (iDim = 0; iDim < nDim; iDim++) Velocity2 += (Velocity[iDim]*Velocity[iDim]);

        /*--- Computation of the enthalpy, total energy, temperature and speed of sound. ---*/
        H_out = H_in/Density_in + Fa/Density_out;
        Energy = H_out - Pressure_out/Density_out;
        if (tkeNeeded) Energy += solver_container[TURB_SOL]->GetNodes()->GetSolution(iPoint,0);  // node k, see #1851
        Temperature_out = (Energy-0.5*Velocity2/(pow(Density_out,2)))*(Gamma_Minus_One/Gas_Constant);

        SoS_out = sqrt(Gamma*Gas_Constant*Temperature_out);

        /*--- Set the primitive variables. ---*/
        V_outlet[0] = Temperature_out;
        for (iDim = 0; iDim < nDim; iDim++)
          V_outlet[iDim+1] = Velocity[iDim]/Density_out;
        V_outlet[nDim+1] = Pressure_out;
        V_outlet[nDim+2] = Density_out;
        V_outlet[nDim+3] = H_out;
        V_outlet[nDim+4] = SoS_out;
        conv_numerics->SetPrimitive(V_domain, V_outlet);
        /*--- SST: both states contain the k of the node in their total enthalpy. ---*/
        if (config->GetKind_Turb_Model() == TURB_MODEL::SST) {
          const su2double tke = solver_container[TURB_SOL]->GetNodes()->GetSolution(iPoint, 0);
          conv_numerics->SetTurbKineticEnergy(tke, tke);
        }
      }

      /*--- Grid Movement (NOT TESTED!)---*/

      if (dynamic_grid)
        conv_numerics->SetGridVel(geometry->nodes->GetGridVel(iPoint), geometry->nodes->GetGridVel(iPoint));

      /*--- Compute the residual using an upwind scheme ---*/

      auto residual = conv_numerics->ComputeResidual(config);

      /*--- Update residual value ---*/

      LinSysRes.AddBlock(iPoint, residual);

      /*--- Jacobian contribution for implicit integration ---*/

      if (implicit) Jacobian.AddBlock2Diag(iPoint, residual.jacobian_i);
    }
  }
  END_SU2_OMP_FOR
}

void CEulerSolver::PrintVerificationError(const CConfig *config, const su2double &hEff) const {
  SU2_ZONE_SCOPED

  if ((rank != MASTER_NODE) || (MGLevel != MESH_0)) return;

  if (config && !config->GetDiscrete_Adjoint()) {

    cout.precision(5);
    cout.setf(ios::scientific, ios::floatfield);

    cout << endl   << "------------------------ Global Error Analysis --------------------------" << endl;

    cout << setw(20) << "hEff            : " << setw(12) << hEff << "     | ";
    cout << setw(20) << "hEEff          : " << setw(12) << hEff;
    cout << endl;

    cout << setw(20) << "RMS Error  [Rho]: " << setw(12) << VerificationSolution->GetError_RMS(0) << "     | ";
    cout << setw(20) << "Max Error  [Rho]: " << setw(12) << VerificationSolution->GetError_RMS_Monitor(0);
    cout << endl;

    cout << setw(20) << "RMS Error [RhoU]: " << setw(12) << VerificationSolution->GetError_RMS(1) << "     | ";
    cout << setw(20) << "Max Error [RhoU]: " << setw(12) << VerificationSolution->GetError_RMS_Monitor(1);
    cout << endl;

    cout << setw(20) << "RMS Error [RhoV]: " << setw(12) << VerificationSolution->GetError_RMS(2) << "     | ";
    cout << setw(20) << "Max Error [RhoV]: " << setw(12) << VerificationSolution->GetError_RMS_Monitor(2);
    cout << endl;

    if (nDim == 3) {
      cout << setw(20) << "RMS Error [RhoW]: " << setw(12) << VerificationSolution->GetError_RMS(3) << "     | ";
      cout << setw(20) << "Max Error [RhoW]: " << setw(12) << VerificationSolution->GetError_RMS_Monitor(3);
      cout << endl;
    }

    cout << setw(20) << "RMS Error [RhoE]: " << setw(12) << VerificationSolution->GetError_RMS(nDim+1) << "     | ";
    cout << setw(20) << "Max Error [RhoE]: " << setw(12) << VerificationSolution->GetError_RMS_Monitor(nDim+1);
    cout << endl;

    cout << "-------------------------------------------------------------------------" << endl << endl;
    cout.unsetf(ios_base::floatfield);
  }
}

void CEulerSolver::SetFreeStream_Solution(const CConfig *config) {
  SU2_ZONE_SCOPED
  SU2_OMP_FOR_STAT(omp_chunk_size)
  for (auto iPoint = 0u; iPoint < nPoint; iPoint++) {
    nodes->SetSolution(iPoint,0, Density_Inf);
    for (auto iDim = 0u; iDim < nDim; iDim++) {
      nodes->SetSolution(iPoint,iDim+1, Density_Inf*Velocity_Inf[iDim]);
    }
    nodes->SetSolution(iPoint,nVar-1, Density_Inf*Energy_Inf);
  }
  END_SU2_OMP_FOR
}

void CEulerSolver::SetFreeStream_TurboSolution(CConfig *config) {
  SU2_ZONE_SCOPED

  const su2double Alpha            = config->GetAoA()*PI_NUMBER/180.0;
  const auto Mach             = config->GetMach();

  su2double turboVelocity[MAXNDIM] = {0}, cartVelocity[MAXNDIM] = {0};

  auto turboNormal = config->GetFreeStreamTurboNormal();

  GetFluidModel()->SetTDState_Prho(Pressure_Inf, Density_Inf);
  const auto SoundSpeed = GetFluidModel()->GetSoundSpeed();

  /*--- Compute the Free Stream velocity, using the Mach number ---*/
  turboVelocity[0] = cos(Alpha)*Mach*SoundSpeed;
  turboVelocity[1] = sin(Alpha)*Mach*SoundSpeed;

  const auto iZone  =  config->GetiZone();
  ComputeBackVelocity(turboVelocity, turboNormal, cartVelocity, INFLOW, config->GetKind_TurboMachinery(iZone));

  for (auto iPoint = 0u; iPoint < nPoint; iPoint++) {
    nodes->SetSolution(iPoint,0, Density_Inf);
    for (auto iDim = 0u; iDim < nDim; iDim++) {
      nodes->SetSolution(iPoint,iDim+1, Density_Inf*cartVelocity[iDim]);
    }
    nodes->SetSolution(iPoint,nVar-1, Density_Inf*Energy_Inf);

    nodes->SetPrimVar(iPoint, GetFluidModel());
    nodes->SetSecondaryVar(iPoint, GetFluidModel());
  }
}

void CEulerSolver::PreprocessAverage(CSolver **solver, CGeometry *geometry, CConfig *config, unsigned short marker_flag) {
  SU2_ZONE_SCOPED

  const auto nSpanWiseSections = config->GetnSpanWiseSections();
  const auto iZone = config->GetiZone();
  const bool spalart_allmaras = (config->GetKind_Turb_Model() == TURB_MODEL::SA);
  const bool menter_sst       = (config->GetKind_Turb_Model() == TURB_MODEL::SST);

  for (auto iSpan= 0u; iSpan < nSpanWiseSections; iSpan++){
    su2double TotalAreaVelocity[MAXNDIM]={0.0},
              TotalAreaPressure{0},
              TotalAreaDensity{0},
              TotalAreaNu{0},
              TotalAreaKine{0},
              TotalAreaOmega{0};
    for (auto iMarker = 0u; iMarker < config->GetnMarker_All(); iMarker++){
      for (auto iMarkerTP=1; iMarkerTP < config->GetnMarker_Turbomachinery()+1; iMarkerTP++){
        if (config->GetMarker_All_Turbomachinery(iMarker) == iMarkerTP){
          if (config->GetMarker_All_TurbomachineryFlag(iMarker) == marker_flag){

            /*--- Retrieve Old Solution ---*/

            /*--- Loop over the vertices to sum all the quantities pithc-wise ---*/
            for (auto iVertex = 0u; iVertex < geometry->GetnVertexSpan(iMarker,iSpan); iVertex++) {
              auto iPoint = geometry->turbovertex[iMarker][iSpan][iVertex]->GetNode();
              if (geometry->nodes->GetDomain(iPoint)){
                /*--- Compute the integral fluxes for the boundaries ---*/

                auto Pressure = nodes->GetPressure(iPoint);
                auto Density = nodes->GetDensity(iPoint);

                /*--- This is in Euler, however we also need to average the turbulent variables so we do it here too ---*/
                su2double Kine{0}, Omega{0}, Nu{0};
                if(menter_sst){
                  Kine = solver[TURB_SOL]->GetNodes()->GetSolution(iPoint,0);
                  Omega = solver[TURB_SOL]->GetNodes()->GetSolution(iPoint,1);
                }
                if(spalart_allmaras){
                  Nu = solver[TURB_SOL]->GetNodes()->GetSolution(iPoint,0);
                }

                su2double UnitNormal[MAXNDIM]={0},
                          TurboNormal[MAXNDIM]={0},
                          TurboVelocity[MAXNDIM],
                          Area;
                /*--- Normal vector for this vertex (negate for outward convention) ---*/
                geometry->turbovertex[iMarker][iSpan][iVertex]->GetNormal(UnitNormal);
                geometry->turbovertex[iMarker][iSpan][iVertex]->GetTurboNormal(TurboNormal);
                Area = geometry->turbovertex[iMarker][iSpan][iVertex]->GetArea();

                su2double Velocity[MAXNDIM]={0};
                for (auto iDim=0u; iDim<nDim;iDim++) Velocity[iDim] = nodes->GetVelocity(iPoint, iDim);

                ComputeTurboVelocity(Velocity, TurboNormal , TurboVelocity, marker_flag, config->GetKind_TurboMachinery(iZone));

                /*--- Compute different integral quantities for the boundary of interest ---*/

                TotalAreaPressure += Area*Pressure;
                TotalAreaDensity  += Area*Density;
                for (auto iDim = 0u; iDim < nDim; iDim++)
                  TotalAreaVelocity[iDim] += Area*Velocity[iDim];
                TotalAreaNu       += Area*Nu;
                TotalAreaKine     += Area*Kine;
                TotalAreaOmega    += Area*Omega;
              }
            }
          }
        }
      } // iMarkerTP
    } // iMarker

#ifdef HAVE_MPI

    auto Allreduce = [](su2double x) {
      su2double tmp = x; x = 0.0;
      SU2_MPI::Allreduce(&tmp, &x, 1, MPI_DOUBLE, MPI_SUM, SU2_MPI::GetComm());
      return x;
    };

    /*--- Add information using all the nodes ---*/

    TotalAreaDensity = Allreduce(TotalAreaDensity);
    TotalAreaPressure = Allreduce(TotalAreaPressure);
    TotalAreaNu = Allreduce(TotalAreaNu);
    TotalAreaKine = Allreduce(TotalAreaKine);
    TotalAreaOmega = Allreduce(TotalAreaOmega);

    auto* buffer = new su2double[nDim];

    auto Allreduce_inplace = [buffer](int size, su2double* x) {
      SU2_MPI::Allreduce(x, buffer, size, MPI_DOUBLE, MPI_SUM, SU2_MPI::GetComm());
      for(int i=0; i<size; ++i) x[i] = buffer[i];
    };

    Allreduce_inplace(nDim, TotalAreaVelocity);

    delete [] buffer;
  
#endif

    /*--- initialize spanwise average quantities ---*/
    for (auto iMarker = 0u; iMarker < config->GetnMarker_All(); iMarker++){
      for (auto iMarkerTP=1; iMarkerTP < config->GetnMarker_Turbomachinery()+1; iMarkerTP++){
        if (config->GetMarker_All_Turbomachinery(iMarker) == iMarkerTP){
          if (config->GetMarker_All_TurbomachineryFlag(iMarker) == marker_flag){

            auto TotalArea           = geometry->GetSpanArea(iMarker,iSpan);
            auto AverageTurboNormal  = geometry->GetAverageTurboNormal(iMarker,iSpan);

            /*--- Compute the averaged value for the boundary of interest for the span of interest ---*/

            AverageDensity[iMarker][iSpan]           = TotalAreaDensity / TotalArea;
            AveragePressure[iMarker][iSpan]          = TotalAreaPressure / TotalArea;
            for (auto iDim = 0u; iDim < nDim; iDim++)
              AverageVelocity[iMarker][iSpan][iDim]  = TotalAreaVelocity[iDim] / TotalArea;

            AverageNu[iMarker][iSpan]             = TotalAreaNu / TotalArea;
            AverageKine[iMarker][iSpan]           = TotalAreaKine / TotalArea;
            AverageOmega[iMarker][iSpan]          = TotalAreaOmega / TotalArea;

            /* --- compute static averaged quantities ---*/
            ComputeTurboVelocity(AverageVelocity[iMarker][iSpan], AverageTurboNormal , AverageTurboVelocity[iMarker][iSpan], marker_flag, config->GetKind_TurboMachinery(iZone));

            OldAverageDensity[iMarker][iSpan]               = AverageDensity[iMarker][iSpan];
            OldAveragePressure[iMarker][iSpan]              = AveragePressure[iMarker][iSpan];
            for(auto iDim = 0u; iDim < nDim;iDim++)
              OldAverageTurboVelocity[iMarker][iSpan][iDim] = AverageTurboVelocity[iMarker][iSpan][iDim];

          }
        }
      } // iMarkerTP
    } // iMarker
  } // iSpan

  /*--- initialize 1D average quantities ---*/

  for (auto iMarker = 0u; iMarker < config->GetnMarker_All(); iMarker++){
    for (auto iMarkerTP=1; iMarkerTP < config->GetnMarker_Turbomachinery()+1; iMarkerTP++){
      if (config->GetMarker_All_Turbomachinery(iMarker) == iMarkerTP){
        if (config->GetMarker_All_TurbomachineryFlag(iMarker) == marker_flag){

          auto AverageTurboNormal  = geometry->GetAverageTurboNormal(iMarker,nSpanWiseSections);

          /*--- Compute the averaged value for the boundary of interest for the span of interest ---*/

          AverageDensity[iMarker][nSpanWiseSections]          = AverageDensity[iMarker][nSpanWiseSections/2];
          AveragePressure[iMarker][nSpanWiseSections]         = AveragePressure[iMarker][nSpanWiseSections/2];
          for (auto iDim = 0u; iDim < nDim; iDim++)
            AverageVelocity[iMarker][nSpanWiseSections][iDim] = AverageVelocity[iMarker][nSpanWiseSections/2][iDim];

          AverageNu[iMarker][nSpanWiseSections]             = AverageNu[iMarker][nSpanWiseSections/2];
          AverageKine[iMarker][nSpanWiseSections]           = AverageKine[iMarker][nSpanWiseSections/2];
          AverageOmega[iMarker][nSpanWiseSections]          = AverageOmega[iMarker][nSpanWiseSections/2];

          /* --- compute static averaged quantities ---*/
          ComputeTurboVelocity(AverageVelocity[iMarker][nSpanWiseSections], AverageTurboNormal , AverageTurboVelocity[iMarker][nSpanWiseSections], marker_flag, config->GetKind_TurboMachinery(iZone));

          OldAverageDensity[iMarker][nSpanWiseSections]               = AverageDensity[iMarker][nSpanWiseSections];
          OldAveragePressure[iMarker][nSpanWiseSections]              = AveragePressure[iMarker][nSpanWiseSections];
          for(auto iDim = 0u; iDim < nDim;iDim++)
            OldAverageTurboVelocity[iMarker][nSpanWiseSections][iDim] = AverageTurboVelocity[iMarker][nSpanWiseSections][iDim];

        }
      }
    } // iMarkerTP
  } // iMarker
}


void CEulerSolver::TurboAverageProcess(CSolver **solver, CGeometry *geometry, CConfig *config, unsigned short marker_flag) {
  SU2_ZONE_SCOPED

  const auto average_process = config->GetKind_AverageProcess();
  const auto performance_average_process = config->GetKind_PerformanceAverageProcess();
  const auto iZone     = config->GetiZone();
  const bool turbulent = (config->GetKind_Turb_Model() != TURB_MODEL::NONE);
  const bool spalart_allmaras = (config->GetKind_Turb_Model() == TURB_MODEL::SA);
  const bool menter_sst       = (config->GetKind_Turb_Model() == TURB_MODEL::SST);
  const auto nSpanWiseSections = config->GetnSpanWiseSections();

  for (auto iSpan= 0; iSpan < nSpanWiseSections + 1; iSpan++){
    su2double TotalDensity{0}, TotalPressure{0}, TotalNu{0}, TotalOmega{0}, TotalKine{0}, TotalVelocity[MAXNDIM],
              TotalAreaDensity{0}, TotalAreaPressure{0}, TotalAreaNu{0}, TotalAreaOmega{0}, TotalAreaKine{0}, TotalAreaVelocity[MAXNDIM],
              TotalMassDensity{0}, TotalMassPressure{0}, TotalMassNu{0}, TotalMassOmega{0}, TotalMassKine{0}, TotalMassVelocity[MAXNDIM],
              TotalRelTangVel{0}, TotalTangFlux{0}, TotalAreaRelTangVel{0},
              TotalAbsTangVel{0}, TotalAreaAbsTangVel{0}, TotalMassAbsTangVel{0};

    su2double TotalFluxes[MAXNVAR];
    /*--- Forces initialization for contenitors ---*/
    for (auto iVar=0u;iVar<nVar;iVar++)
      TotalFluxes[iVar]= 0.0;
    for (auto iDim=0u; iDim<nDim; iDim++) {
      TotalVelocity[iDim]     = 0.0;
      TotalAreaVelocity[iDim] = 0.0;
      TotalMassVelocity[iDim] = 0.0;
    }

    auto UpdateTotalQuantities = [&](const size_t iMarker, const size_t iSpan, const size_t iVertex){
      /*--- Increment integral quantities for averaging ---*/

      const auto iPoint = geometry->turbovertex[iMarker][iSpan][iVertex]->GetNode();

      /*--- Retrieve local quantities ---*/
      const auto Pressure = nodes->GetPressure(iPoint);
      const auto Density  = nodes->GetDensity(iPoint);
      const auto Enthalpy = nodes->GetEnthalpy(iPoint);

      su2double Velocity[MAXNDIM] = {0}, UnitNormal[MAXNDIM] = {0}, TurboNormal[MAXNDIM] = {0}, TurboVelocity[MAXNDIM] = {0}, TurboGridVelocity[MAXNDIM] = {0};
      geometry->turbovertex[iMarker][iSpan][iVertex]->GetNormal(UnitNormal);
      geometry->turbovertex[iMarker][iSpan][iVertex]->GetTurboNormal(TurboNormal);
      const auto Area = geometry->turbovertex[iMarker][iSpan][iVertex]->GetArea();

      for (auto iDim=0u; iDim < nDim; iDim++) Velocity[iDim] = nodes->GetVelocity(iPoint, iDim);

      ComputeTurboVelocity(Velocity, TurboNormal , TurboVelocity, marker_flag, config->GetKind_TurboMachinery(iZone));

      /*--- Compute different integral quantities for the boundary of interest ---*/
      TotalDensity          += Density;
      TotalPressure         += Pressure;

      TotalAreaPressure         += Area*Pressure;
      TotalAreaDensity          += Area*Density;

      TotalMassPressure         += Area*(Density*TurboVelocity[0] )*Pressure;
      TotalMassDensity          += Area*(Density*TurboVelocity[0] )*Density;

      for (auto iDim = 0u; iDim < nDim; iDim++) {
        TotalVelocity[iDim] += Velocity[iDim];
        TotalAreaVelocity[iDim] += Area*Velocity[iDim];
        TotalMassVelocity[iDim] += Area*(Density*TurboVelocity[0] )*Velocity[iDim];
      }

      TotalFluxes[0]      += Area*(Density*TurboVelocity[0]);
      TotalFluxes[1]      += Area*(Density*TurboVelocity[0]*TurboVelocity[0] + Pressure);
      for (auto iDim = 2; iDim < nDim+1; iDim++)
        TotalFluxes[iDim] += Area*(Density*TurboVelocity[0]*TurboVelocity[iDim -1]);
      TotalFluxes[nDim+1] += Area*(Density*TurboVelocity[0]*Enthalpy);

      /*--- Compute relative tangential velocity ---*/
      if (dynamic_grid) {
        auto GridVel = geometry->nodes->GetGridVel(iPoint);
        ComputeTurboVelocity(GridVel, TurboNormal, TurboGridVelocity, marker_flag, config->GetKind_TurboMachinery(iZone));
      }

      TotalRelTangVel += (TurboVelocity[1] - TurboGridVelocity[1]);
      TotalAreaRelTangVel += Area*(TurboVelocity[1] - TurboGridVelocity[1]);
      TotalTangFlux += Area*Density*TurboVelocity[0]*(TurboVelocity[1] - TurboGridVelocity[1]);

      TotalAbsTangVel += TurboVelocity[1];
      TotalAreaAbsTangVel += Area*TurboVelocity[1];
      TotalMassAbsTangVel += Area*Density*TurboVelocity[0]*TurboVelocity[1];

      /*--- Compute turbulent integral quantities for the boundary of interest ---*/

      if(turbulent){
        su2double Kine{0}, Omega{0}, Nu{0};
        if(menter_sst){
          Kine = solver[TURB_SOL]->GetNodes()->GetSolution(iPoint,0);
          Omega = solver[TURB_SOL]->GetNodes()->GetSolution(iPoint,1);
        }
        if(spalart_allmaras){
          Nu = solver[TURB_SOL]->GetNodes()->GetSolution(iPoint,0);
        }

        TotalKine   += Kine;
        TotalOmega  += Omega;
        TotalNu     += Nu;

        TotalAreaKine    += Area*Kine;
        TotalAreaOmega   += Area*Omega;
        TotalAreaNu      += Area*Nu;

        TotalMassKine    += Area*(Density*TurboVelocity[0] )*Kine;
        TotalMassOmega   += Area*(Density*TurboVelocity[0] )*Omega;
        TotalMassNu      += Area*(Density*TurboVelocity[0] )*Nu;
      }
    };

    for (auto iMarker = 0u; iMarker < config->GetnMarker_All(); iMarker++){
      for (auto iMarkerTP=1; iMarkerTP < config->GetnMarker_Turbomachinery()+1; iMarkerTP++){
        if (config->GetMarker_All_Turbomachinery(iMarker) == iMarkerTP){
          if (config->GetMarker_All_TurbomachineryFlag(iMarker) == marker_flag){

            /*--- Loop over the vertices to sum all the quantities pitch-wise ---*/
            if(iSpan < nSpanWiseSections){
              for (auto iVertex = 0ul; iVertex < geometry->GetnVertexSpan(iMarker,iSpan); iVertex++) {
                UpdateTotalQuantities(iMarker, iSpan, iVertex);
              }
            } else {
              for (auto jSpan= 0u; jSpan < nSpanWiseSections; jSpan++){
                for (auto iVertex = 0ul; iVertex < geometry->GetnVertexSpan(iMarker,jSpan); iVertex++) {
                  UpdateTotalQuantities(iMarker, jSpan, iVertex);
                }
              }
            }

          } // marker_flag match
        } // iMarkerTP match
      } // iMarkerTP
    } // iMarker

#ifdef HAVE_MPI

    /*--- Add information using all the nodes ---*/

    auto Allreduce = [](su2double x) {
      su2double tmp = x; x = 0.0;
      SU2_MPI::Allreduce(&tmp, &x, 1, MPI_DOUBLE, MPI_SUM, SU2_MPI::GetComm());
      return x;
    };

    TotalDensity = Allreduce(TotalDensity);
    TotalPressure = Allreduce(TotalPressure);
    TotalAreaDensity = Allreduce(TotalAreaDensity);
    TotalAreaPressure = Allreduce(TotalAreaPressure);
    TotalMassDensity = Allreduce(TotalMassDensity);
    TotalMassPressure = Allreduce(TotalMassPressure);

    TotalNu = Allreduce(TotalNu);
    TotalKine = Allreduce(TotalKine);
    TotalOmega = Allreduce(TotalOmega);
    TotalAreaNu = Allreduce(TotalAreaNu);
    TotalAreaKine = Allreduce(TotalAreaKine);
    TotalAreaOmega = Allreduce(TotalAreaOmega);

    TotalMassNu = Allreduce(TotalMassNu);
    TotalMassKine = Allreduce(TotalMassKine);
    TotalMassOmega = Allreduce(TotalMassOmega);

    TotalRelTangVel = Allreduce(TotalRelTangVel);
    TotalAreaRelTangVel = Allreduce(TotalAreaRelTangVel);
    TotalTangFlux = Allreduce(TotalTangFlux);

    TotalAbsTangVel = Allreduce(TotalAbsTangVel);
    TotalAreaAbsTangVel = Allreduce(TotalAreaAbsTangVel);
    TotalMassAbsTangVel = Allreduce(TotalMassAbsTangVel);

    auto* buffer = new su2double[max(nVar,nDim)];

    auto Allreduce_inplace = [buffer](int size, su2double* x) {
      SU2_MPI::Allreduce(x, buffer, size, MPI_DOUBLE, MPI_SUM, SU2_MPI::GetComm());
      for(int i=0; i<size; ++i) x[i] = buffer[i];
    };

    Allreduce_inplace(nVar, TotalFluxes);
    Allreduce_inplace(nDim, TotalVelocity);
    Allreduce_inplace(nDim, TotalAreaVelocity);
    Allreduce_inplace(nDim, TotalMassVelocity);

    delete [] buffer;

#endif

    /*--- Compute pitch-wise averaged quantities ---*/
    for (auto iMarker = 0u; iMarker < config->GetnMarker_All(); iMarker++){
      for (auto iMarkerTP=1; iMarkerTP < config->GetnMarker_Turbomachinery()+1; iMarkerTP++){
        if (config->GetMarker_All_Turbomachinery(iMarker) == iMarkerTP){
          if (config->GetMarker_All_TurbomachineryFlag(iMarker) == marker_flag){

            const auto TotalArea           = geometry->GetSpanArea(iMarker,iSpan);
            const auto AverageTurboNormal  = geometry->GetAverageTurboNormal(iMarker,iSpan);
            const auto nVert               = geometry->GetnTotVertexSpan(iMarker,iSpan);

            /*--- compute normal Mach number as a check for massflow average and mixedout average ---*/
            GetFluidModel()->SetTDState_Prho(TotalAreaPressure/TotalArea, TotalAreaDensity / TotalArea);
            const su2double soundSpeed = GetFluidModel()->GetSoundSpeed(),
                      MachTest   = TotalFluxes[0]/(TotalAreaDensity*soundSpeed);

            /*--- Compute the averaged value for the boundary of interest for the span of interest ---*/

            const bool belowMachLimit = (abs(MachTest)< config->GetAverageMachLimit());
            su2double avgDensity{0}, avgPressure{0}, avgKine{0}, avgOmega{0}, avgNu{0}, avgVelocity[MAXNDIM] = {0}, avgRelTangVel{0}, avgAbsTangVel{0};
            for (auto iVar = 0u; iVar<nVar; iVar++){
              AverageFlux[iMarker][iSpan][iVar]   = TotalFluxes[iVar]/TotalArea;
              SpanTotalFlux[iMarker][iSpan][iVar] = TotalFluxes[iVar];
            }

            switch (average_process)
            {
            case ALGEBRAIC:
              /*--- compute algebraic average ---*/
              avgDensity        = TotalDensity / nVert;
              avgPressure       = TotalPressure / nVert;
              for (auto iDim = 0u; iDim < nDim; iDim++) avgVelocity[iDim] = TotalVelocity[iDim] / nVert;
              if (turbulent) {
                avgKine           = TotalKine/nVert;
                avgOmega          = TotalOmega/nVert;
                avgNu             = TotalNu/nVert;
              }
              avgRelTangVel = TotalRelTangVel/nVert;
              avgAbsTangVel = TotalAbsTangVel/nVert;
              break;
            case AREA:
            /*--- compute area average ---*/
              avgDensity     = TotalAreaDensity / TotalArea;
              avgPressure    = TotalAreaPressure / TotalArea;
              for (auto iDim = 0u; iDim < nDim; iDim++) avgVelocity[iDim] = TotalAreaVelocity[iDim] / TotalArea;
              if (turbulent) {
                avgKine = TotalAreaKine / TotalArea;
                avgOmega = TotalAreaOmega / TotalArea;
                avgNu = TotalAreaNu / TotalArea;
              }
              avgRelTangVel = TotalAreaRelTangVel / TotalArea;
              avgAbsTangVel = TotalAreaAbsTangVel / TotalArea;
              break;
            case MASSFLUX:
            /*--- compute mass-flux average ---*/
              if (belowMachLimit) {
                avgDensity = TotalAreaDensity / TotalArea;
                avgPressure = TotalAreaPressure / TotalArea;
                for (auto iDim = 0u; iDim < nDim; iDim++)  avgVelocity[iDim] = TotalAreaVelocity[iDim] / TotalArea;
                if (turbulent) {
                  avgKine = TotalAreaKine / TotalArea;
                  avgOmega = TotalAreaOmega / TotalArea;
                  avgNu = TotalAreaNu / TotalArea;
                }
                avgRelTangVel = TotalAreaRelTangVel / TotalArea;
                avgAbsTangVel = TotalAreaAbsTangVel / TotalArea;
              } else {
                avgDensity = TotalMassDensity / TotalFluxes[0];
                avgPressure = TotalMassPressure / TotalFluxes[0];
                for (auto iDim = 0u; iDim < nDim; iDim++)  avgVelocity[iDim] = TotalMassVelocity[iDim] / TotalFluxes[0];
                if (turbulent) {
                  avgKine = TotalMassKine / TotalFluxes[0];
                  avgOmega = TotalMassOmega / TotalFluxes[0];
                  avgNu = TotalMassNu / TotalFluxes[0];
                }
                avgRelTangVel = TotalTangFlux / TotalFluxes[0];
                avgAbsTangVel = TotalMassAbsTangVel / TotalFluxes[0];
              }
              break;
            case MIXEDOUT:
              /*--- compute mixed-out average ---*/
              avgDensity = TotalAreaDensity / TotalArea;
              avgPressure = TotalAreaPressure / TotalArea;
              if (belowMachLimit) {
                for (auto iDim = 0u; iDim < nDim; iDim++)
                  avgVelocity[iDim] = TotalAreaVelocity[iDim] / TotalArea;
                if (turbulent) {
                  avgKine = TotalAreaKine / TotalArea;
                  avgOmega = TotalAreaOmega / TotalArea;
                  avgNu = TotalAreaNu / TotalArea;
                }
                avgRelTangVel = TotalAreaRelTangVel / TotalArea;
                avgAbsTangVel = TotalAreaAbsTangVel / TotalArea;
              }else {
                auto val_init_pressure = OldAveragePressure[iMarker][iSpan];
                MixedOut_Average (config, val_init_pressure, AverageFlux[iMarker][iSpan], AverageTurboNormal, avgPressure, avgDensity);
                avgVelocity[0]         = ( AverageFlux[iMarker][iSpan][1] - avgPressure) / AverageFlux[iMarker][iSpan][0];
                for (auto iDim = 2; iDim < nDim +1;iDim++)
                  avgVelocity[iDim-1]  = AverageFlux[iMarker][iSpan][iDim] / AverageFlux[iMarker][iSpan][0];

                if (std::isnan(avgDensity) || std::isnan(avgPressure) || avgPressure < 0.0 || avgDensity < 0.0 ){
                  val_init_pressure = TotalAreaPressure / TotalArea;
                  MixedOut_Average (config, val_init_pressure, AverageFlux[iMarker][iSpan], AverageTurboNormal, avgPressure, avgDensity);
                  avgVelocity[0]          = ( AverageFlux[iMarker][iSpan][1] - avgPressure) / AverageFlux[iMarker][iSpan][0];
                  for (auto iDim = 2; iDim < nDim +1;iDim++)
                    avgVelocity[iDim-1]   = AverageFlux[iMarker][iSpan][iDim] / AverageFlux[iMarker][iSpan][0];
                }
                avgRelTangVel = TotalTangFlux / TotalFluxes[0];
                avgAbsTangVel = TotalMassAbsTangVel / TotalFluxes[0];
                if (turbulent) {
                  avgKine       = TotalMassKine / TotalFluxes[0];
                  avgOmega      = TotalMassOmega / TotalFluxes[0];
                  avgNu         = TotalMassNu / TotalFluxes[0];
                }
              }

              if (iSpan == nSpanWiseSections){
                AverageMassFlowRate[iMarker] = TotalFluxes[0];
              }

              break;
            default:
              SU2_MPI::Error(" Invalid AVERAGE PROCESS input!", CURRENT_FUNCTION);
              break;
            }
            AverageDensity[iMarker][iSpan] = avgDensity;
            AveragePressure[iMarker][iSpan] = avgPressure;
            if ((average_process == MIXEDOUT) && !belowMachLimit) {
              for (auto iDim = 0u; iDim < nDim; iDim++) AverageTurboVelocity[iMarker][iSpan][iDim] = avgVelocity[iDim];
            } else {
              ComputeTurboVelocity(avgVelocity, AverageTurboNormal , AverageTurboVelocity[iMarker][iSpan], marker_flag, config->GetKind_TurboMachinery(iZone));
            }
            if (turbulent) {
              AverageKine[iMarker][iSpan] = avgKine;
              AverageOmega[iMarker][iSpan] = avgOmega;
              AverageNu[iMarker][iSpan] = avgNu;
            }
            AverageRelTangVelocity[iMarker][iSpan] = avgRelTangVel;

            /* --- check if averaged quantities are correct otherwise reset the old quantities ---*/
            const bool nanSolution = (std::isnan(AverageDensity[iMarker][iSpan]) || std::isnan(AveragePressure[iMarker][iSpan]));
            const bool negSolution = (AverageDensity[iMarker][iSpan] < 0.0 || AveragePressure[iMarker][iSpan] < 0.0);
            if (nanSolution || negSolution){
              if (nanSolution)
                cout<<"nan in mixing process routine for iSpan: " << iSpan<< " in marker " << config->GetMarker_All_TagBound(iMarker)<< endl;
              else
                cout << " negative density or pressure in mixing process routine for iSpan: " << iSpan<< " in marker " << config->GetMarker_All_TagBound(iMarker)<< endl;
              AverageDensity[iMarker][iSpan]               = OldAverageDensity[iMarker][iSpan];
              AveragePressure[iMarker][iSpan]              = OldAveragePressure[iMarker][iSpan];
              for(auto iDim = 0u; iDim < nDim;iDim++)
                AverageTurboVelocity[iMarker][iSpan][iDim] = OldAverageTurboVelocity[iMarker][iSpan][iDim];
            } else {
              /* --- update old average solution ---*/
              OldAverageDensity[iMarker][iSpan]               = AverageDensity[iMarker][iSpan];
              OldAveragePressure[iMarker][iSpan]              = AveragePressure[iMarker][iSpan];
              for(auto iDim = 0u; iDim < nDim;iDim++)
                OldAverageTurboVelocity[iMarker][iSpan][iDim] = AverageTurboVelocity[iMarker][iSpan][iDim];
            }

            /*--- to avoid back flow ---*/
            if (AverageTurboVelocity[iMarker][iSpan][0] < 0.0){
              AverageTurboVelocity[iMarker][iSpan][0]       = soundSpeed*config->GetAverageMachLimit();
            }

            /*--- compute cartesian average Velocity ---*/
            ComputeBackVelocity(AverageTurboVelocity[iMarker][iSpan], AverageTurboNormal , AverageVelocity[iMarker][iSpan], marker_flag, config->GetKind_TurboMachinery(iZone));

            /*--- Store averaged performance value for the selected average method ---*/
            if (marker_flag == INFLOW) {
              DensityIn[iMarkerTP - 1][iSpan] = AverageDensity[iMarker][iSpan];
              PressureIn[iMarkerTP - 1][iSpan]  = AveragePressure[iMarker][iSpan];
              KineIn[iMarkerTP - 1][iSpan]      = AverageKine[iMarker][iSpan];
              OmegaIn[iMarkerTP - 1][iSpan]     = AverageOmega[iMarker][iSpan];
              NuIn[iMarkerTP - 1][iSpan]        = AverageNu[iMarker][iSpan];
              RelTangVelocityIn[iMarkerTP - 1][iSpan] = AverageRelTangVelocity[iMarker][iSpan];
            } else {
              DensityOut[iMarkerTP - 1][iSpan]  = AverageDensity[iMarker][iSpan];
              PressureOut[iMarkerTP - 1][iSpan] = AveragePressure[iMarker][iSpan];
              KineOut[iMarkerTP - 1][iSpan]     = AverageKine[iMarker][iSpan];
              OmegaOut[iMarkerTP - 1][iSpan]    = AverageOmega[iMarker][iSpan];
              NuOut[iMarkerTP - 1][iSpan]       = AverageNu[iMarker][iSpan];
              RelTangVelocityOut[iMarkerTP - 1][iSpan] = AverageRelTangVelocity[iMarker][iSpan];
            }

            auto TurboVel = (marker_flag == INFLOW) ? TurboVelocityIn[iMarkerTP - 1][iSpan] : TurboVelocityOut[iMarkerTP - 1][iSpan];

            if (performance_average_process == MIXEDOUT) {
              for (auto iDim = 0u; iDim < nDim; iDim++) TurboVel[iDim] = avgVelocity[iDim];
            } else {
              ComputeTurboVelocity(avgVelocity, AverageTurboNormal , TurboVel, marker_flag, config->GetKind_TurboMachinery(iZone));
            }
            /*--- Override the tangential component with the direct per-vertex turbo-frame average
             *    so that TurboVel[1] - RelTangVelocity = avg(grid tang. vel.) ---*/
            TurboVel[1] = avgAbsTangVel;
          }
        }
      } // iMarkerTP
    } // iMarker
  } // iSpan

  /*--- Compute Outlet Static Pressure if Radial equilibrium is imposed ---*/

  for (auto iMarker = 0u; iMarker < config->GetnMarker_All(); iMarker++){
    for (auto iMarkerTP=1; iMarkerTP < config->GetnMarker_Turbomachinery()+1; iMarkerTP++){
      if (config->GetMarker_All_Turbomachinery(iMarker) == iMarkerTP){
        if (config->GetMarker_All_TurbomachineryFlag(iMarker) == marker_flag){
          auto Marker_Tag         = config->GetMarker_All_TagBound(iMarker);
          if(config->GetMarker_All_Giles(iMarker) || config->GetBoolRiemann()){ // May have to implement something similar for Riemann?
            if(config->GetBoolRiemann()){
              if(config->GetKind_Data_Riemann(Marker_Tag) == RADIAL_EQUILIBRIUM){
                RadialEquilibriumPressure[iMarker][nSpanWiseSections/2] = config->GetRiemann_Var1(Marker_Tag)/config->GetPressure_Ref();
              }
            } else {
              if(config->GetKind_Data_Giles(Marker_Tag) == RADIAL_EQUILIBRIUM){
                RadialEquilibriumPressure[iMarker][nSpanWiseSections/2] = config->GetGiles_Var1(Marker_Tag)/config->GetPressure_Ref();
              }
            }
            for (auto iSpan= nSpanWiseSections/2; iSpan < nSpanWiseSections-1; iSpan++){
              const auto Radius2    = geometry->GetTurboRadius(iMarker,iSpan+1);
              const auto Radius1    = geometry->GetTurboRadius(iMarker,iSpan);
              const su2double Vt2        = AverageTurboVelocity[iMarker][iSpan +1][1]*AverageTurboVelocity[iMarker][iSpan +1][1];
              RadialEquilibriumPressure[iMarker][iSpan +1] =  RadialEquilibriumPressure[iMarker][iSpan] + AverageDensity[iMarker][iSpan +1]*Vt2/Radius2*(Radius2 - Radius1);
            }
            for (auto iSpan= nSpanWiseSections/2; iSpan > 0; iSpan--){
              const su2double Radius2    = geometry->GetTurboRadius(iMarker,iSpan);
              su2double Radius1    = geometry->GetTurboRadius(iMarker,iSpan-1);
              const su2double Vt2        = AverageTurboVelocity[iMarker][iSpan -1][1]*AverageTurboVelocity[iMarker][iSpan - 1][1];
              Radius1    = (Radius1 > EPS)? Radius1 : Radius2;
              RadialEquilibriumPressure[iMarker][iSpan -1] =  RadialEquilibriumPressure[iMarker][iSpan] - AverageDensity[iMarker][iSpan -1]*Vt2/Radius1*(Radius2 - Radius1);
            }
          } // Giles or Riemann
        } // marker_flag
      } // iMarkerTP
    } // iMarker is iMarkerTP
  } // iMarker
}

void CEulerSolver::MixedOut_Average(CConfig *config, su2double val_init_pressure, const su2double *val_Averaged_Flux,
                                     const su2double *val_normal, su2double& pressure_mix, su2double& density_mix) {
  SU2_ZONE_SCOPED

  const auto relax_factor = config->GetMixedout_Coeff(0);
  const auto toll = config->GetMixedout_Coeff(1);
  unsigned short maxiter = SU2_TYPE::Int(config->GetMixedout_Coeff(2));

  pressure_mix = val_init_pressure;

  /*--- Newton-Raphson's method with central difference formula ---*/
  unsigned short iter{0};
  while ( iter <= maxiter ) {

    const su2double density_mix = val_Averaged_Flux[0]*val_Averaged_Flux[0]/(val_Averaged_Flux[1] - pressure_mix);
    GetFluidModel()->SetTDState_Prho(pressure_mix, density_mix);
    const su2double enthalpy_mix = GetFluidModel()->GetStaticEnergy() + (pressure_mix)/(density_mix);

    GetFluidModel()->ComputeDerivativeNRBC_Prho(pressure_mix, density_mix);
    const su2double dhdP   = GetFluidModel()->GetdhdP_rho();
    const su2double dhdrho = GetFluidModel()->Getdhdrho_P();

    su2double vel[MAXNDIM] = {0};
    vel[0]  = (val_Averaged_Flux[1] - pressure_mix) / val_Averaged_Flux[0];
    for (auto iDim = 1u; iDim < nDim; iDim++)
      vel[iDim]  = val_Averaged_Flux[iDim+1] / val_Averaged_Flux[0];


    const su2double velsq = GeometryToolbox::DotProduct(nDim, vel, vel);

    const su2double f = val_Averaged_Flux[nDim+1] - val_Averaged_Flux[0]*(enthalpy_mix + velsq/2);
    const su2double df = -val_Averaged_Flux[0]*(dhdP - 1/density_mix) - dhdrho*density_mix*density_mix/val_Averaged_Flux[0];
    const su2double dx = -f/df;
    const su2double resdl = dx/val_init_pressure;
    pressure_mix += relax_factor*dx;

    iter += 1;
    if ( abs(resdl) <= toll ) {
      break;
    }

  }
  density_mix = val_Averaged_Flux[0]*val_Averaged_Flux[0]/(val_Averaged_Flux[1] - pressure_mix);

}

void CEulerSolver::GatherInOutAverageValues(CConfig *config, CGeometry *geometry){
  SU2_ZONE_SCOPED

  unsigned short iMarker, iMarkerTP;
  unsigned short iSpan;
  int markerTP;
  su2double     densityIn, pressureIn, normalVelocityIn, tangVelocityIn, radialVelocityIn, relTangVelocityIn;
  su2double     densityOut, pressureOut, normalVelocityOut, tangVelocityOut, radialVelocityOut, relTangVelocityOut;
  su2double     kineIn, omegaIn, nuIn, kineOut, omegaOut, nuOut;
  //TODO (turbo) implement interpolation so that Inflow and Outflow spanwise section can be different

  const auto nSpanWiseSections = config->GetnSpanWiseSections();

  for (iSpan= 0; iSpan < nSpanWiseSections + 1 ; iSpan++) {

#ifdef HAVE_MPI
    unsigned short i, n1, n2, n1t,n2t;
    su2double *TurbPerfIn= nullptr,*TurbPerfOut= nullptr;
    su2double *TotTurbPerfIn = nullptr,*TotTurbPerfOut = nullptr;
    int *TotMarkerTP = nullptr;

    n1          = 9;
    n2          = 9;
    n1t         = n1*size;
    n2t         = n2*size;
    TurbPerfIn  = new su2double[n1];
    TurbPerfOut = new su2double[n2];

    for (i=0;i<n1;i++)
      TurbPerfIn[i]    = -1.0;
    for (i=0;i<n2;i++)
      TurbPerfOut[i]   = -1.0;
#endif

    densityIn            = -1.0;
    pressureIn           = -1.0;
    normalVelocityIn     = -1.0;
    tangVelocityIn       = -1.0;
    radialVelocityIn     = -1.0;
    relTangVelocityIn    = -1.0;
    densityOut           = -1.0;
    pressureOut          = -1.0;
    normalVelocityOut    = -1.0;
    tangVelocityOut      = -1.0;
    radialVelocityOut    = -1.0;
    relTangVelocityOut   = -1.0;
    kineIn               = -1.0;
    omegaIn              = -1.0;
    nuIn                 = -1.0;
    kineOut              = -1.0;
    omegaOut             = -1.0;
    nuOut                = -1.0;

    markerTP             = -1;

    for (iMarker = 0; iMarker < config->GetnMarker_All(); iMarker++){
      for (iMarkerTP = 1; iMarkerTP < config->GetnMarker_Turbomachinery()+1; iMarkerTP++){
        if (config->GetMarker_All_Turbomachinery(iMarker) == iMarkerTP){
          if (config->GetMarker_All_TurbomachineryFlag(iMarker) == INFLOW){
            markerTP            = iMarkerTP;
            densityIn           = DensityIn[iMarkerTP -1][iSpan];
            pressureIn          = PressureIn[iMarkerTP -1][iSpan];
            normalVelocityIn    = TurboVelocityIn[iMarkerTP -1][iSpan][0];
            tangVelocityIn      = TurboVelocityIn[iMarkerTP -1][iSpan][1];
            if (nDim ==3){
              radialVelocityIn  = TurboVelocityIn[iMarkerTP -1][iSpan][2];
            }
            kineIn              = KineIn[iMarkerTP -1][iSpan];
            omegaIn             = OmegaIn[iMarkerTP -1][iSpan];
            nuIn                = NuIn[iMarkerTP -1][iSpan];
            relTangVelocityIn   = RelTangVelocityIn[iMarkerTP -1][iSpan];

#ifdef HAVE_MPI
            TurbPerfIn[0]  = densityIn;
            TurbPerfIn[1]  = pressureIn;
            TurbPerfIn[2]  = normalVelocityIn;
            TurbPerfIn[3]  = tangVelocityIn;
            TurbPerfIn[4]  = radialVelocityIn;
            TurbPerfIn[5]  = kineIn;
            TurbPerfIn[6]  = omegaIn;
            TurbPerfIn[7]  = nuIn;
            TurbPerfIn[8]  = relTangVelocityIn;
#endif
          }

          /*--- retrieve outlet information ---*/
          if (config->GetMarker_All_TurbomachineryFlag(iMarker) == OUTFLOW){
            densityOut           = DensityOut[iMarkerTP -1][iSpan];
            pressureOut          = PressureOut[iMarkerTP -1][iSpan];
            normalVelocityOut    = TurboVelocityOut[iMarkerTP -1][iSpan][0];
            tangVelocityOut      = TurboVelocityOut[iMarkerTP -1][iSpan][1];
            if (nDim ==3){
              radialVelocityOut  = TurboVelocityOut[iMarkerTP -1][iSpan][2];
            }
            kineOut              = KineOut[iMarkerTP -1][iSpan];
            omegaOut             = OmegaOut[iMarkerTP -1][iSpan];
            nuOut                = NuOut[iMarkerTP -1][iSpan];
            relTangVelocityOut   = RelTangVelocityOut[iMarkerTP -1][iSpan];

#ifdef HAVE_MPI
            TurbPerfOut[0]  = densityOut;
            TurbPerfOut[1]  = pressureOut;
            TurbPerfOut[2]  = normalVelocityOut;
            TurbPerfOut[3]  = tangVelocityOut;
            TurbPerfOut[4]  = radialVelocityOut;
            TurbPerfOut[5]  = kineOut;
            TurbPerfOut[6]  = omegaOut;
            TurbPerfOut[7]  = nuOut;
            TurbPerfOut[8]  = relTangVelocityOut;
#endif
          }
        }
      }
    }

#ifdef HAVE_MPI
    if (rank == MASTER_NODE){
      TotTurbPerfIn       = new su2double[n1t];
      TotTurbPerfOut      = new su2double[n2t];
      for (i=0;i<n1t;i++)
        TotTurbPerfIn[i]  = -1.0;
      for (i=0;i<n2t;i++)
        TotTurbPerfOut[i] = -1.0;
      TotMarkerTP = new int[size];
      for(int i=0; i<size; i++){
        TotMarkerTP[i]    = -1;
      }
    }
    SU2_MPI::Gather(TurbPerfIn, n1, MPI_DOUBLE, TotTurbPerfIn, n1, MPI_DOUBLE, MASTER_NODE, SU2_MPI::GetComm());
    SU2_MPI::Gather(TurbPerfOut, n2, MPI_DOUBLE,TotTurbPerfOut, n2, MPI_DOUBLE, MASTER_NODE, SU2_MPI::GetComm());
    SU2_MPI::Gather(&markerTP, 1, MPI_INT,TotMarkerTP, 1, MPI_INT, MASTER_NODE, SU2_MPI::GetComm());
    if (rank == MASTER_NODE){
      delete [] TurbPerfIn, delete [] TurbPerfOut;
    }

    if (rank == MASTER_NODE){
      for (int i=0;i<size;i++){
        if(TotTurbPerfIn[n1*i] > 0.0){
          densityIn           = TotTurbPerfIn[n1*i];
          pressureIn          = TotTurbPerfIn[n1*i+1];
          normalVelocityIn    = TotTurbPerfIn[n1*i+2];
          tangVelocityIn      = TotTurbPerfIn[n1*i+3];
          radialVelocityIn    = TotTurbPerfIn[n1*i+4];
          kineIn              = TotTurbPerfIn[n1*i+5];
          omegaIn             = TotTurbPerfIn[n1*i+6];
          nuIn                = TotTurbPerfIn[n1*i+7];
          relTangVelocityIn   = TotTurbPerfIn[n1*i+8];
          markerTP            = TotMarkerTP[i];
        }

        if(TotTurbPerfOut[n2*i] > 0.0){
          densityOut           = TotTurbPerfOut[n2*i];
          pressureOut          = TotTurbPerfOut[n2*i+1];
          normalVelocityOut    = TotTurbPerfOut[n2*i+2];
          tangVelocityOut      = TotTurbPerfOut[n2*i+3];
          radialVelocityOut    = TotTurbPerfOut[n2*i+4];
          kineOut              = TotTurbPerfOut[n2*i+5];
          omegaOut             = TotTurbPerfOut[n2*i+6];
          nuOut                = TotTurbPerfOut[n2*i+7];
          relTangVelocityOut   = TotTurbPerfOut[n2*i+8];
        }
      }

      delete [] TotTurbPerfIn, delete [] TotTurbPerfOut; delete [] TotMarkerTP;
    }

#endif

    if (rank == MASTER_NODE && markerTP > -1){
      /*----Quantities needed for computing the turbomachinery performance -----*/
      DensityIn[markerTP -1][iSpan]              = densityIn;
      PressureIn[markerTP -1][iSpan]             = pressureIn;
      TurboVelocityIn[markerTP -1][iSpan][0]     = normalVelocityIn;
      TurboVelocityIn[markerTP -1][iSpan][1]     = tangVelocityIn;
      if (nDim == 3)
        TurboVelocityIn[markerTP -1][iSpan][2]   = radialVelocityIn;
      KineIn[markerTP -1][iSpan]                 = kineIn;
      OmegaIn[markerTP -1][iSpan]                = omegaIn;
      NuIn[markerTP -1][iSpan]                   = nuIn;
      RelTangVelocityIn[markerTP -1][iSpan]      = relTangVelocityIn;

      DensityOut[markerTP -1][iSpan]             = densityOut;
      PressureOut[markerTP -1][iSpan]            = pressureOut;
      TurboVelocityOut[markerTP -1][iSpan][0]    = normalVelocityOut;
      TurboVelocityOut[markerTP -1][iSpan][1]    = tangVelocityOut;
      if (nDim == 3)
        TurboVelocityOut[markerTP -1][iSpan][2]  = radialVelocityOut;
      KineOut[markerTP -1][iSpan]                = kineOut;
      OmegaOut[markerTP -1][iSpan]               = omegaOut;
      NuOut[markerTP -1][iSpan]                  = nuOut;
      RelTangVelocityOut[markerTP -1][iSpan]     = relTangVelocityOut;
    }
  }
}

void CEulerSolver::GetInviscidProjJacPrim(const su2double* val_velocity, const su2double* val_density,
                                       su2double* val_spec_total_enthalpy, const su2double* val_normal,
                                       su2double val_scale, su2double** val_Proj_Jac_Tensor) const {
  const bool wasActive = AD::BeginPassive();
  unsigned short iDim, jDim;
  su2double sqvel, proj_vel;

  sqvel = 0.0; proj_vel = 0.0;
  for (iDim = 0; iDim < nDim; iDim++) {
    sqvel    += val_velocity[iDim]*val_velocity[iDim];
    proj_vel += val_velocity[iDim]*val_normal[iDim];
  }

  val_Proj_Jac_Tensor[0][0] = val_scale*proj_vel;
  for (iDim = 0; iDim < nDim; iDim++)
    val_Proj_Jac_Tensor[0][iDim+1] = val_scale*(*val_density)*val_normal[iDim];
  val_Proj_Jac_Tensor[0][nDim+1] = 0.0;

  for (iDim = 0; iDim < nDim; iDim++) {
    val_Proj_Jac_Tensor[iDim+1][0] = val_scale*(val_velocity[iDim]*proj_vel);
    for (jDim = 0; jDim < nDim; jDim++)
      val_Proj_Jac_Tensor[iDim+1][jDim+1] = val_scale*(*val_density)*(val_normal[jDim]*val_velocity[iDim]);
    val_Proj_Jac_Tensor[iDim+1][iDim+1] += val_scale*(*val_density)*proj_vel;
    val_Proj_Jac_Tensor[iDim+1][nDim+1] = val_scale*val_normal[iDim];
  }

  val_Proj_Jac_Tensor[nDim+1][0] = val_scale*0.5*proj_vel*(sqvel);
  for (iDim = 0; iDim < nDim; iDim++)
    val_Proj_Jac_Tensor[nDim+1][iDim+1] = val_scale*(*val_density) * (val_normal[iDim]*(*val_spec_total_enthalpy) + val_velocity[iDim]*proj_vel);
  val_Proj_Jac_Tensor[nDim+1][nDim+1] = val_scale*proj_vel*Gamma/Gamma_Minus_One;
  AD::EndPassive(wasActive);
}

void CEulerSolver::ConvectiveError(CSolver **solver, const CGeometry*geometry, const CConfig *config,
                                   unsigned long iPoint, vector<vector<double> > &weights) {

  auto varFlo    = solver[FLOW_SOL]->GetNodes();
  auto varAdjFlo = solver[ADJFLOW_SOL]->GetNodes();

  const bool turb = (config->GetKind_Turb_Model() != TURB_MODEL::NONE);
  const bool sst  = (config->GetKind_Turb_Model() == TURB_MODEL::SST);

  CVariable *varTur = nullptr, *varAdjTur = nullptr;
  if (turb) {
    varTur    = solver[TURB_SOL]->GetNodes();
    varAdjTur = solver[ADJTURB_SOL]->GetNodes();
  }

  const unsigned short nVarFlo = solver[FLOW_SOL]->GetnVar();

  double A[5][5] = {0.0}, B[5][5] = {0.0}, C[5][5] = {0.0};

  //--- Inviscid terms
  const double u  = SU2_TYPE::GetValue(varFlo->GetVelocity(iPoint, 0));
  const double v  = SU2_TYPE::GetValue(varFlo->GetVelocity(iPoint, 1));
  const double w  = (nDim == 3)? SU2_TYPE::GetValue(varFlo->GetVelocity(iPoint, 2)): 0.;
  const double v2 = SU2_TYPE::GetValue(varFlo->GetVelocity2(iPoint));
  const double e  = SU2_TYPE::GetValue(varFlo->GetEnergy(iPoint));
  const double g  = SU2_TYPE::GetValue(config->GetGamma());

  //--- Store transposed Jacobians
  if(nDim == 2) {
    A[0][1] = -u*u + (g-1.)/2.*v2; A[0][2] = -u*v; A[0][3] = -u*(g*e-(g-1)*v2);
    A[1][0] = 1.; A[1][1] = (3.-g)*u; A[1][2] = v; A[1][3] = g*e-(g-1.)/2.*(v2+2*u*u);
    A[2][1] = -(g-1.)*v; A[2][2] = u; A[2][3] = -(g-1.)*u*v;
    A[3][1] = g-1.; A[3][3] = g*u;

    B[0][1] = -u*v; B[0][2] = -v*v + (g-1.)/2.*v2; B[0][3] = -v*(g*e-(g-1)*v2);
    B[1][1] = v; B[1][2] = -(g-1.)*u; B[1][3] = -(g-1.)*u*v;
    B[2][0] = 1.; B[2][1] = u; B[2][2] = (3.-g)*v; B[2][3] = g*e-(g-1.)/2.*(v2+2*v*v);
    B[3][2] = g-1.; B[3][3] = g*v;

  }
  else{
    A[0][1] = -u*u + (g-1.)/2.*v2; A[0][2] = -u*v; A[0][3] = -u*w; A[0][4] = -u*(g*e-(g-1)*v2);
    A[1][0] = 1.; A[1][1] = (3.-g)*u; A[1][2] = v; A[1][3] = w;    A[1][4] = g*e-(g-1.)/2.*(v2+2*u*u);
    A[2][1] = -(g-1.)*v; A[2][2] = u; A[2][4] = -(g-1.)*u*v;
    A[3][1] = -(g-1.)*w; A[3][3] = u; A[3][4] = -(g-1.)*u*w;
    A[4][1] = g-1.; A[4][4] = g*u;

    B[0][1] = -u*v; B[0][2] = -v*v + (g-1.)/2.*v2; B[0][3] = -v*w; B[0][4] = -v*(g*e-(g-1)*v2);
    B[1][1] = v; B[1][2] = -(g-1.)*u; B[1][4] = -(g-1.)*u*v;
    B[2][0] = 1.; B[2][1] = u; B[2][2] = (3.-g)*v; B[2][3] = w; B[2][4] = g*e-(g-1.)/2.*(v2+2*v*v);
    B[3][2] = -(g-1.)*w; B[3][3] = v; B[3][4] = -(g-1.)*v*w;
    B[4][2] = g-1.; B[4][4] = g*v;

    C[0][1] = -u*w; C[0][2] = -v*w; C[0][3] = -w*w + (g-1.)/2.*v2; C[0][4] = -w*(g*e-(g-1)*v2);
    C[1][1] = w; C[1][3] = -(g-1.)*u; C[1][4] = -(g-1.)*u*w;
    C[2][2] = w; C[2][3] = -(g-1.)*v; C[2][4] = -(g-1.)*v*w;
    C[3][0] = 1.; C[3][1] = u; C[3][2] = v; C[3][3] = (3.-g)*w; C[3][4] = g*e-(g-1.)/2.*(v2+2*w*w);
    C[4][3] = (g-1.); C[4][4] = g*w;
  }

  //--- Contribution of k to dp/dr and dp/d(re)
  if (sst) {
    const double k = SU2_TYPE::GetValue(varTur->GetSolution(iPoint,0));
    if (nDim == 2) {
      A[0][3] += (g-1.)*k*u;
      A[1][3] += -(g-1.)*k;

      B[0][3] += (g-1.)*k*v;
      B[2][3] += -(g-1.)*k;
    }
    else {
      A[0][4] += (g-1.)*k*u;
      A[1][4] += -(g-1.)*k;

      B[0][4] += (g-1.)*k*v;
      B[2][4] += -(g-1.)*k;

      C[0][4] += (g-1.)*k*w;
      C[3][4] += -(g-1.)*k;
    }
  }

  for (auto jVar = 0; jVar < nVarFlo; ++jVar) {
    const double adjx = SU2_TYPE::GetValue(varAdjFlo->GetGradient_Adapt(iPoint, jVar, 0));
    const double adjy = SU2_TYPE::GetValue(varAdjFlo->GetGradient_Adapt(iPoint, jVar, 1));
    const double adjz = (nDim == 3)? SU2_TYPE::GetValue(varAdjFlo->GetGradient_Adapt(iPoint, jVar, 2)): 0.0;
    for (auto iVar = 0; iVar < nVarFlo; ++iVar) {
      weights[1][iVar] += -A[iVar][jVar]*adjx - B[iVar][jVar]*adjy;
      if(nDim == 3) weights[1][iVar] += -C[iVar][jVar]*adjz;
    }
  }

  //--- Turbulent terms
  if(turb) {
    const unsigned short nVarTur = solver[TURB_SOL]->GetnVar();
    if (sst) {
      for (auto iVar = 0; iVar < nVarTur; ++iVar){
        const double adjx = SU2_TYPE::GetValue(varAdjTur->GetGradient_Adapt(iPoint, iVar, 0));
        const double adjy = SU2_TYPE::GetValue(varAdjTur->GetGradient_Adapt(iPoint, iVar, 1));
        const double val  = SU2_TYPE::GetValue(varTur->GetPrimitive(iPoint, iVar));
        weights[1][nVarFlo+iVar] += - u*adjx - v*adjy;
        weights[1][0]            += val*u*adjx + val*v*adjy;
        weights[1][1]            += - val*adjx;
        weights[1][2]            += - val*adjy;
        if (nDim == 3) {
          const double adjz = SU2_TYPE::GetValue(varAdjTur->GetGradient_Adapt(iPoint, iVar, 2));
          weights[1][nVarFlo+iVar] += - w*adjz;
          weights[1][0]            += val*w*adjz;
          weights[1][3]            += - val*adjz;
        }
      }

      //--- Contribution of k to dp/d(rk)
      //--- Momentum equation
      for (auto iDim = 0; iDim < nDim; iDim++) {
        const double adj = SU2_TYPE::GetValue(varAdjFlo->GetGradient_Adapt(iPoint, iDim+1, iDim));
        weights[1][nVarFlo+0] += (g-1.)*adj;
      }
      //--- Energy equation
      const double adjx = SU2_TYPE::GetValue(varAdjFlo->GetGradient_Adapt(iPoint, nVarFlo-1, 0));
      const double adjy = SU2_TYPE::GetValue(varAdjFlo->GetGradient_Adapt(iPoint, nVarFlo-1, 1));
      weights[1][nVarFlo+0] += (g-1.)*(u*adjx+v*adjy);
      if (nDim == 3) {
        const double adjz = SU2_TYPE::GetValue(varAdjFlo->GetGradient_Adapt(iPoint, nVarFlo-1, 2));
        weights[1][nVarFlo+0] += (g-1.)*w*adjz;
      }
    }
  }

}

void CEulerSolver::ViscousError(CSolver **solver, const CGeometry*geometry, const CConfig *config,
                                 unsigned long iPoint, vector<vector<double> > &weights) {

  CVariable *varFlo    = solver[FLOW_SOL]->GetNodes(),
            *varAdjFlo = solver[ADJFLOW_SOL]->GetNodes();

  const bool turb = (config->GetKind_Turb_Model() != TURB_MODEL::NONE);
  const bool sst  = (config->GetKind_Turb_Model() == TURB_MODEL::SST);

  CVariable *varTur = nullptr;
  if (turb) varTur = solver[TURB_SOL]->GetNodes();

  const unsigned short nVarFlo = solver[FLOW_SOL]->GetnVar();

  //--- Store primitive variables and coefficients
  const double r = SU2_TYPE::GetValue(varFlo->GetDensity(iPoint));
  double u[3] = {0.0};
  u[0] = SU2_TYPE::GetValue(varFlo->GetVelocity(iPoint, 0));
  u[1] = SU2_TYPE::GetValue(varFlo->GetVelocity(iPoint, 1));
  if (nDim == 3) u[2] = SU2_TYPE::GetValue(varFlo->GetVelocity(iPoint, 2));
  const double u2 = u[0]*u[0]+u[1]*u[1]+u[2]*u[2];

  double k = 0.;
  if(sst) {
    k = SU2_TYPE::GetValue(varTur->GetSolution(iPoint,0));
  }

  const double T   = SU2_TYPE::GetValue(varFlo->GetTemperature(iPoint));
  const double nu  = SU2_TYPE::GetValue(varFlo->GetLaminarViscosity(iPoint))/r;
  const double nut = SU2_TYPE::GetValue(varFlo->GetEddyViscosity(iPoint))/r;

  const double Pr  = SU2_TYPE::GetValue(config->GetPrandtl_Lam());
  const double Prt = SU2_TYPE::GetValue(config->GetPrandtl_Turb());
  const double g   = SU2_TYPE::GetValue(config->GetGamma());
  const double R   = SU2_TYPE::GetValue(config->GetGas_ConstantND());
  const double cp  = (g/(g-1.))*R;
  const double cv  = cp/g;

  const double ONE3 = 1.0/3.0;
  const double TWO3 = 1.0/3.0;

  //--- Store gradients and stress tensor
  double gradu[3][3] = {0.0}, gradT[3] = {0.0}, gradnu[3] = {0.0}, gradnut[3] = {0.0};
  for (auto iDim = 0; iDim < nDim; iDim++) {
    for (auto jDim = 0 ; jDim < nDim; jDim++) {
      gradu[iDim][jDim] = SU2_TYPE::GetValue(varFlo->GetGradient_AuxVar_Adapt(iPoint, iDim+1, jDim));
    }
    gradT[iDim] = SU2_TYPE::GetValue(varFlo->GetGradient_AuxVar_Adapt(iPoint, 0, iDim));
    gradnu[iDim] = SU2_TYPE::GetValue(varFlo->GetGradient_AuxVar_Adapt(iPoint, nDim+1, iDim));
    gradnut[iDim] = SU2_TYPE::GetValue(varFlo->GetGradient_AuxVar_Adapt(iPoint, nDim+2, iDim));
  }

  //--- Account for wall functions
  // double wf = varFlo->GetTauWallFactor(iPoint);
  double wf = 1.0;
  double divu = 0.0; for (auto iDim = 0 ; iDim < nDim; ++iDim) divu += gradu[iDim][iDim];
  double tau[3][3] = {0.0};
  for (auto iDim = 0; iDim < nDim; ++iDim) {
    for (auto jDim = 0; jDim < nDim; ++jDim) {
      tau[iDim][jDim]  = wf*r*((nu+nut)*( gradu[jDim][iDim] + gradu[iDim][jDim] )
                       - TWO3*((nu+nut)*divu+k)*(iDim == jDim));

    }
  }

  //-------------------------//
  //--- Momentum equation ---//
  //-------------------------//

  //--- Errors wrt momentum
  for (auto iDim = 0; iDim < nDim; ++iDim) {
    const size_t ind_ii = iDim*nDim - ((iDim - 1)*iDim)/2;
    const size_t iVar = iDim+1;

    for (auto jDim = 0; jDim < nDim; ++jDim) {
      const size_t ind_ij = (iDim <= jDim) ? iDim*nDim - ((iDim - 1)*iDim)/2 + jDim - iDim
                                           : jDim*nDim - ((jDim - 1)*jDim)/2 + iDim - jDim;
      const size_t ind_jj = jDim*nDim - ((jDim - 1)*iDim)/2;
      const size_t jVar = jDim+1;

      const double hessadjui_jj = SU2_TYPE::GetValue(varAdjFlo->GetHessian(iPoint, iVar, ind_jj));
      const double hessadjuj_ij = SU2_TYPE::GetValue(varAdjFlo->GetHessian(iPoint, jVar, ind_ij));

      const double gradadjui_j = SU2_TYPE::GetValue(varAdjFlo->GetGradient_Adapt(iPoint, iVar, jDim));
      const double gradadjuj_i = SU2_TYPE::GetValue(varAdjFlo->GetGradient_Adapt(iPoint, jVar, iDim));
      const double gradadjuj_j = SU2_TYPE::GetValue(varAdjFlo->GetGradient_Adapt(iPoint, jVar, jDim));

      weights[2][iVar] -= (nu + nut) * ( hessadjui_jj + ONE3 * hessadjuj_ij * (iDim != jDim) );
      weights[1][iVar] -= (gradnu[jDim]+gradnut[jDim]) * gradadjui_j
                        + ( (gradnu[jDim]+gradnut[jDim]) * gradadjuj_i
                        -   TWO3 * (gradnu[iDim]+gradnut[iDim]) * gradadjuj_j ) * (iDim != jDim);
    }

    const double hessadjui_ii = SU2_TYPE::GetValue(varAdjFlo->GetHessian(iPoint, iVar, ind_ii));
    const double gradadjui_i = SU2_TYPE::GetValue(varAdjFlo->GetGradient_Adapt(iPoint, iVar, iDim));

    weights[2][iVar] -= ONE3 * (nu + nut) * hessadjui_ii;
    weights[1][iVar] -= ONE3 * (gradnu[iDim]+gradnut[iDim]) * gradadjui_i;
  }

  //--- Errors wrt density
  for (auto iDim = 0; iDim < nDim; ++iDim) {
    const size_t ind_ii = iDim*nDim - ((iDim - 1)*iDim)/2;
    const size_t iVar = iDim+1;

    const double gradnu_ui_i = (gradnu[iDim]+gradnut[iDim]) * u[iDim] + (nu+nut) * gradu[iDim][iDim];

    for (auto jDim = 0; jDim < nDim; ++jDim) {
      const size_t ind_ij = (iDim <= jDim) ? iDim*nDim - ((iDim - 1)*iDim)/2 + jDim - iDim
                                           : jDim*nDim - ((jDim - 1)*jDim)/2 + iDim - jDim;
      const size_t ind_jj = jDim*nDim - ((jDim - 1)*iDim)/2;
      const size_t jVar = jDim+1;

      const double hessadjui_jj = SU2_TYPE::GetValue(varAdjFlo->GetHessian(iPoint, iVar, ind_jj));
      const double hessadjuj_ij = SU2_TYPE::GetValue(varAdjFlo->GetHessian(iPoint, jVar, ind_ij));

      const double gradadjui_j = SU2_TYPE::GetValue(varAdjFlo->GetGradient_Adapt(iPoint, iVar, jDim));
      const double gradadjuj_i = SU2_TYPE::GetValue(varAdjFlo->GetGradient_Adapt(iPoint, jVar, iDim));
      const double gradadjuj_j = SU2_TYPE::GetValue(varAdjFlo->GetGradient_Adapt(iPoint, jVar, jDim));

      const double gradnu_ui_j = (gradnu[jDim]+gradnut[jDim]) * u[iDim] + (nu+nut) * gradu[iDim][jDim];

      weights[2][0] += (nu + nut) * u[iDim] * ( hessadjui_jj + ONE3 * hessadjuj_ij * (iDim != jDim) );
      weights[1][0] += gradnu_ui_j * gradadjui_j
                     + ( gradnu_ui_j * gradadjuj_i
                     -   TWO3 * gradnu_ui_i * gradadjuj_j ) * (iDim != jDim);
    }

    const double hessadjui_ii = SU2_TYPE::GetValue(varAdjFlo->GetHessian(iPoint, iVar, ind_ii));
    const double gradadjui_i = SU2_TYPE::GetValue(varAdjFlo->GetGradient_Adapt(iPoint, iVar, iDim));

    weights[2][0] += ONE3 * (nu + nut) * u[iDim] * hessadjui_ii;
    weights[1][0] += ONE3 * gradnu_ui_i * gradadjui_i;
  }

  //-----------------------//
  //--- Energy equation ---//
  //-----------------------//

  //--- Errors in shear stress work wrt momentum
  for (auto iDim = 0; iDim < nDim; ++iDim) {
    const size_t ind_ii = iDim*nDim - ((iDim - 1)*iDim)/2;
    const size_t iVar = iDim+1;

    const double gradnu_ui_i = (gradnu[iDim]+gradnut[iDim]) * u[iDim] + (nu+nut) * gradu[iDim][iDim];

    for (auto jDim = 0; jDim < nDim; ++jDim) {
      const size_t ind_ij = (iDim <= jDim) ? iDim*nDim - ((iDim - 1)*iDim)/2 + jDim - iDim
                                           : jDim*nDim - ((jDim - 1)*jDim)/2 + iDim - jDim;
      const size_t ind_jj = jDim*nDim - ((jDim - 1)*iDim)/2;

      const double hessadje_ij = SU2_TYPE::GetValue(varAdjFlo->GetHessian(iPoint, nVarFlo-1, ind_ij));
      const double hessadje_jj = SU2_TYPE::GetValue(varAdjFlo->GetHessian(iPoint, nVarFlo-1, ind_jj));

      const double gradadje_i = SU2_TYPE::GetValue(varAdjFlo->GetGradient_Adapt(iPoint, nVarFlo-1, iDim));
      const double gradadje_j = SU2_TYPE::GetValue(varAdjFlo->GetGradient_Adapt(iPoint, nVarFlo-1, jDim));

      const double gradnu_ui_j = (gradnu[jDim]+gradnut[jDim]) * u[iDim] + (nu+nut) * gradu[iDim][jDim];
      const double gradnu_uj_i = (gradnu[iDim]+gradnut[iDim]) * u[jDim] + (nu+nut) * gradu[jDim][iDim];
      const double gradnu_uj_j = (gradnu[jDim]+gradnut[jDim]) * u[jDim] + (nu+nut) * gradu[jDim][jDim];

      weights[2][iVar] -= (nu + nut) * ( u[jDim] * hessadje_ij
                                     + ( u[iDim] * hessadje_jj
                                     -   TWO3* u[jDim] * hessadje_ij ) * (iDim != jDim) );
      weights[1][iVar] -= gradnu_uj_j * gradadje_i
                        + ( gradnu_ui_j * gradadje_j
                        -   TWO3 * gradnu_uj_i * gradadje_j ) * (iDim != jDim);
      weights[1][iVar] += tau[iDim][jDim]/r * gradadje_j;
    }

    const double hessadje_ii = SU2_TYPE::GetValue(varAdjFlo->GetHessian(iPoint, nVarFlo-1, ind_ii));
    const double gradadje_i = SU2_TYPE::GetValue(varAdjFlo->GetGradient_Adapt(iPoint, nVarFlo-1, iDim));

    weights[2][iVar] -= ONE3 * (nu + nut) * hessadje_ii;
    weights[1][iVar] -= ONE3 * gradnu_ui_i * gradadje_i;
  }

  //--- Errors in shear stress work wrt density
  for (auto iDim = 0; iDim < nDim; ++iDim) {
    const size_t ind_ii = iDim*nDim - ((iDim - 1)*iDim)/2;

    const double gradi_nu_uiui = (gradnu[iDim]+gradnut[iDim]) * u[iDim]*u[iDim] + 2.0 * (nu+nut) * gradu[iDim][iDim] * u[iDim];

    for (auto jDim = 0; jDim < nDim; ++jDim) {
      const size_t ind_ij = (iDim <= jDim) ? iDim*nDim - ((iDim - 1)*iDim)/2 + jDim - iDim
                                           : jDim*nDim - ((jDim - 1)*jDim)/2 + iDim - jDim;
      const size_t ind_jj = jDim*nDim - ((jDim - 1)*iDim)/2;

      const double hessadje_ij = SU2_TYPE::GetValue(varAdjFlo->GetHessian(iPoint, nVarFlo-1, ind_ij));
      const double hessadje_jj = SU2_TYPE::GetValue(varAdjFlo->GetHessian(iPoint, nVarFlo-1, ind_jj));

      const double gradadje_i = SU2_TYPE::GetValue(varAdjFlo->GetGradient_Adapt(iPoint, nVarFlo-1, iDim));
      const double gradadje_j = SU2_TYPE::GetValue(varAdjFlo->GetGradient_Adapt(iPoint, nVarFlo-1, jDim));

      const double gradj_nu_uiuj = (gradnu[jDim]+gradnut[jDim]) * u[iDim]*u[jDim] + (nu+nut) * (gradu[iDim][jDim]*u[jDim]+gradu[jDim][jDim]*u[iDim]);
      const double gradj_nu_uiui = (gradnu[jDim]+gradnut[jDim]) * u[iDim]*u[iDim] + (nu+nut) * 2.0*(gradu[iDim][jDim]*u[iDim]);
      const double gradi_nu_uiuj = (gradnu[iDim]+gradnut[iDim]) * u[iDim]*u[jDim] + (nu+nut) * (gradu[iDim][iDim]*u[jDim]+gradu[jDim][iDim]*u[iDim]);

      weights[2][0] += (nu + nut) * u[iDim] * ( u[jDim] * hessadje_ij
                                            + ( u[iDim] * hessadje_jj
                                            -   TWO3 * u[jDim] * hessadje_ij ) * (iDim != jDim) );
      weights[1][0] += gradj_nu_uiuj * gradadje_i
                     + ( gradj_nu_uiui * gradadje_j
                     -   TWO3 * gradi_nu_uiuj * gradadje_j ) * (iDim != jDim);
      weights[1][0] -= tau[iDim][jDim]*u[iDim]/r * gradadje_j;
    }

    const double hessadje_ii = SU2_TYPE::GetValue(varAdjFlo->GetHessian(iPoint, nVarFlo-1, ind_ii));
    const double gradadje_i = SU2_TYPE::GetValue(varAdjFlo->GetGradient_Adapt(iPoint, nVarFlo-1, iDim));

    weights[2][0] += ONE3 * (nu + nut) * u[iDim] * hessadje_ii;
    weights[1][0] += ONE3 * gradi_nu_uiui * gradadje_i;
  }

  //--- Errors in heat flux wrt energy
  for (auto iDim = 0; iDim < nDim; ++iDim) {
    const size_t ind_ii = iDim*nDim - ((iDim - 1)*iDim)/2;

    const double hessadje_ii = SU2_TYPE::GetValue(varAdjFlo->GetHessian(iPoint, nVarFlo-1, ind_ii));
    const double gradadje_i = SU2_TYPE::GetValue(varAdjFlo->GetGradient_Adapt(iPoint, nVarFlo-1, iDim));

    weights[2][nVarFlo-1] -= g * (nu/Pr + nut/Prt) * hessadje_ii;
    weights[1][nVarFlo-1] -= g * (gradnu[iDim]/Pr + gradnut[iDim]/Prt) * gradadje_i;
  }

  //--- Errors in heat flux wrt momentum
  for (auto iDim = 0; iDim < nDim; ++iDim) {
    const size_t iVar = iDim+1;
    for (auto jDim = 0; jDim < nDim; ++jDim) {
      const size_t ind_jj = jDim*nDim - ((jDim - 1)*jDim)/2;

      const double hessadje_jj = SU2_TYPE::GetValue(varAdjFlo->GetHessian(iPoint, nVarFlo-1, ind_jj));
      const double gradadje_j = SU2_TYPE::GetValue(varAdjFlo->GetGradient_Adapt(iPoint, nVarFlo-1, jDim));

      weights[2][iVar] += g * (nu/Pr + nut/Prt) * u[iDim] * hessadje_jj;
      weights[1][iVar] += g * ( (gradnu[jDim]/Pr + gradnut[jDim]/Prt) * u[iDim]
                            +   (nu + nut) * gradu[iDim][jDim] ) * gradadje_j;
    }
  }

  //--- Errors in heat flux wrt density
  for (auto iDim = 0; iDim < nDim; ++iDim) {
    double ujduj = 0;
    for (auto jDim = 0; jDim < nDim; ++jDim) {
      ujduj += u[jDim]*gradu[jDim][iDim];

    }
    const size_t ind_ii = iDim*nDim - ((iDim - 1)*iDim)/2;

    const double hessadje_ii = SU2_TYPE::GetValue(varAdjFlo->GetHessian(iPoint, nVarFlo-1, ind_ii));
    const double gradadje_i = SU2_TYPE::GetValue(varAdjFlo->GetGradient_Adapt(iPoint, nVarFlo-1, iDim));

    weights[2][0] -= g * (nu/Pr + nut/Prt) * (0.5*u2 - cv*T) * hessadje_ii;
    weights[1][0] -= g * ( (gradnu[iDim]/Pr + gradnut[iDim]/Prt) * (0.5*u2 - cv*T)
                       +   (nu/Pr + nut/Prt) * (ujduj - cv*gradT[iDim]) ) * gradadje_i;
  }

  //-----------------------//
  //--- Viscosity terms ---//
  //-----------------------//

  LaminarViscosityError(solver, geometry, config, iPoint, weights);

}

void CEulerSolver::LaminarViscosityError(CSolver **solver, const CGeometry *geometry, const CConfig *config,
                                         unsigned long iPoint, vector<vector<double> > &weights) {

  CVariable *varFlo    = solver[FLOW_SOL]->GetNodes(),
            *varAdjFlo = solver[ADJFLOW_SOL]->GetNodes();

  const unsigned short nVarFlo = solver[FLOW_SOL]->GetnVar();

  //--- Store primitive variables and coefficients
  const double r = SU2_TYPE::GetValue(varFlo->GetDensity(iPoint));
  double u[3] = {0.0};
  u[0] = SU2_TYPE::GetValue(varFlo->GetVelocity(iPoint, 0));
  u[1] = SU2_TYPE::GetValue(varFlo->GetVelocity(iPoint, 1));
  if (nDim == 3) u[2] = SU2_TYPE::GetValue(varFlo->GetVelocity(iPoint, 2));
  const double u2 = u[0]*u[0]+u[1]*u[1]+u[2]*u[2];

  const double T   = SU2_TYPE::GetValue(varFlo->GetTemperature(iPoint));
  const double nu  = SU2_TYPE::GetValue(varFlo->GetLaminarViscosity(iPoint))/r;

  const double S     = SU2_TYPE::GetValue(config->GetMu_SND());
  const double dmudT = 0.5*r*nu*(T + 3*S)/(T*(T+S));

  const double Pr  = SU2_TYPE::GetValue(config->GetPrandtl_Lam());
  const double g   = SU2_TYPE::GetValue(config->GetGamma());
  const double R   = SU2_TYPE::GetValue(config->GetGas_ConstantND());
  const double cp  = (g/(g-1.))*R;
  const double cv  = cp/g;

  const double TWO3 = 2.0/3.0;

  //--- Store gradients and stress tensor
  double gradu[3][3] = {0.0}, gradT[3] = {0.0};
  for (auto iDim = 0; iDim < nDim; iDim++) {
    for (auto jDim = 0 ; jDim < nDim; jDim++) {
      gradu[iDim][jDim] = SU2_TYPE::GetValue(varFlo->GetGradient_AuxVar_Adapt(iPoint, iDim+1, jDim));
    }
    gradT[iDim] = SU2_TYPE::GetValue(varFlo->GetGradient_AuxVar_Adapt(iPoint, 0, iDim));
  }

  //--- Account for wall functions
  // double wf = varFlo->GetTauWallFactor(iPoint);
  double wf = 1.0;
  double divu = 0.0; for (auto iDim = 0 ; iDim < nDim; ++iDim) divu += gradu[iDim][iDim];
  double tauomu[3][3] = {0.0};
  for (auto iDim = 0; iDim < nDim; ++iDim) {
    for (auto jDim = 0; jDim < nDim; ++jDim) {
      tauomu[iDim][jDim] = wf * ( ( gradu[jDim][iDim] + gradu[iDim][jDim] )
                              -     TWO3*divu*(iDim == jDim) );

    }
  }

  //-------------------------//
  //--- Momentum equation ---//
  //-------------------------//

  for (auto iDim = 0; iDim < nDim; ++iDim) {
    double taudUbar = 0.0;
    for (auto jDim = 0; jDim < nDim; ++jDim) {
      const double dUbar = SU2_TYPE::GetValue(varAdjFlo->GetGradient_Adapt(iPoint, iDim+1, jDim));
      taudUbar += tauomu[iDim][jDim]*dUbar;
    }
    weights[1][nVarFlo-1] += dmudT/(r*cv)*taudUbar;
    for (auto jDim = 0; jDim < nDim; ++jDim)
      weights[1][jDim+1] -= dmudT/(r*cv)*u[jDim]*taudUbar;
    weights[1][0] += dmudT/(r*cv)*(0.5*u2-cv*T)*taudUbar;
  }

  //-----------------------//
  //--- Energy equation ---//
  //-----------------------//

  //--- Errors in shear stress work
  for (auto iDim = 0; iDim < nDim; ++iDim) {
    double work = 0;
    for (auto jDim = 0; jDim < nDim; ++jDim) {
      work += tauomu[iDim][jDim]*u[jDim];
    }
    const double dUbar = SU2_TYPE::GetValue(varAdjFlo->GetGradient_Adapt(iPoint, nVarFlo-1, iDim));

    weights[1][nVarFlo-1] += dmudT/(r*cv) * work * dUbar;
    for (auto jDim = 0; jDim < nDim; ++jDim)
        weights[1][jDim+1] -= dmudT/(r*cv) * u[jDim] * work * dUbar;
    weights[1][0] += dmudT/(r*cv) * (0.5*u2-cv*T) * work * dUbar;
  }

  //--- Errors in heat flux
  for (auto iDim = 0; iDim < nDim; ++iDim) {
    const double dUbar = SU2_TYPE::GetValue(varAdjFlo->GetGradient_Adapt(iPoint, nVarFlo-1, iDim));

    weights[1][nVarFlo-1] += dmudT*g/(r*Pr) * gradT[iDim] * dUbar;
    for (auto jDim = 0; jDim < nDim; ++jDim)
        weights[1][jDim+1] -= dmudT*g/(r*Pr) * u[jDim] * gradT[iDim] * dUbar;
    weights[1][0] += dmudT*g/(r*Pr) * (0.5*u2-cv*T) * gradT[iDim] * dUbar;
  }

}
void CEulerSolver::ComputeTurboBladePerformance(CGeometry* geometry, CConfig* config, unsigned short iBlade) {
  // Computes the turboperformance per blade in zone iBlade and stores the results in TurbomachineryPerformance.
  const auto nDim = geometry->GetnDim();
  vector<su2double> TurboPrimitiveIn, TurboPrimitiveOut;
  if (rank == MASTER_NODE) {
    std::vector<CTurbomachineryCombinedPrimitiveStates> bladePrimitives;
    auto nSpan = config->GetnSpanWiseSections();
    for (auto iSpan = 0; iSpan < nSpan + 1; iSpan++) {
      TurboPrimitiveIn = GetTurboPrimitive(iBlade, iSpan, true);
      TurboPrimitiveOut = GetTurboPrimitive(iBlade, iSpan, false);
      auto spanInletPrimitive = CTurbomachineryPrimitiveState(TurboPrimitiveIn, nDim, GetTangGridVelIn(iBlade, iSpan));
      auto spanOutletPrimitive = CTurbomachineryPrimitiveState(TurboPrimitiveOut, nDim, GetTangGridVelOut(iBlade, iSpan));
      bladePrimitives.push_back(CTurbomachineryCombinedPrimitiveStates(spanInletPrimitive, spanOutletPrimitive));
    }
    TurbomachineryPerformance->ComputeTurbomachineryPerformance(bladePrimitives, iBlade);
  }
}