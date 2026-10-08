/*!
 * \file CSolver.cpp
 * \brief Main subroutines for CSolver class.
 * \author F. Palacios, T. Economon
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


#include "../../include/solvers/CSolver.hpp"
#include "../../../Common/include/adaptation/CMeshGather.hpp"
#include "../../../Common/include/adaptation/CDistributedSearch.hpp"
#include "../../../Common/include/parallelization/CPassiveComm.hpp"

#include <limits>
#include <queue>

#include "../../include/gradients/computeGradientsGreenGauss.hpp"
#include "../../include/gradients/computeGradientsLeastSquares.hpp"
#include "../../include/gradients/computeHessians.hpp"
#include "../../include/gradients/computeHessiansQuadratic.hpp"
#include "../../include/limiters/computeLimiters.hpp"
#include "../../../Common/include/toolboxes/MMS/CIncTGVSolution.hpp"
#include "../../../Common/include/toolboxes/MMS/CInviscidVortexSolution.hpp"
#include "../../../Common/include/toolboxes/MMS/CMMSIncEulerSolution.hpp"
#include "../../../Common/include/toolboxes/MMS/CMMSIncNSSolution.hpp"
#include "../../../Common/include/toolboxes/MMS/CMMSNSTwoHalfCirclesSolution.hpp"
#include "../../../Common/include/toolboxes/MMS/CMMSNSTwoHalfSpheresSolution.hpp"
#include "../../../Common/include/toolboxes/MMS/CMMSNSUnitQuadSolution.hpp"
#include "../../../Common/include/toolboxes/MMS/CMMSNSUnitQuadSolutionWallBC.hpp"
#include "../../../Common/include/toolboxes/MMS/CNSUnitQuadSolution.hpp"
#include "../../../Common/include/toolboxes/MMS/CRinglebSolution.hpp"
#include "../../../Common/include/toolboxes/MMS/CTGVSolution.hpp"
#include "../../../Common/include/toolboxes/MMS/CUserDefinedSolution.hpp"
#include "../../../Common/include/toolboxes/printing_toolbox.hpp"
#include "../../../Common/include/toolboxes/C1DInterpolation.hpp"
#include "../../../Common/include/toolboxes/geometry_toolbox.hpp"
#include "../../../Common/include/toolboxes/CLinearPartitioner.hpp"
#include "../../../Common/include/adt/CADTPointsOnlyClass.hpp"
#include "../../include/CMarkerProfileReaderFVM.hpp"
#include "../../include/adaptation/CBoundaryLayerMetric.hpp"


CSolver::CSolver(LINEAR_SOLVER_MODE linear_solver_mode) : System(linear_solver_mode) {
  SU2_ZONE_SCOPED

  rank = SU2_MPI::GetRank();
  size = SU2_MPI::GetSize();

  adjoint = false;

  /*--- Set the multigrid level to the finest grid. This can be
        overwritten in the constructors of the derived classes. ---*/
  MGLevel = MESH_0;

  /*--- Array initialization ---*/

  OutputHeadingNames = nullptr;
  Residual           = nullptr;
  Residual_i         = nullptr;
  Residual_j         = nullptr;
  Solution           = nullptr;
  Solution_i         = nullptr;
  Solution_j         = nullptr;
  Vector             = nullptr;
  Vector_i           = nullptr;
  Vector_j           = nullptr;
  Res_Conv           = nullptr;
  Res_Visc           = nullptr;
  Res_Sour           = nullptr;
  Res_Conv_i         = nullptr;
  Res_Visc_i         = nullptr;
  Res_Conv_j         = nullptr;
  Res_Visc_j         = nullptr;
  Jacobian_i         = nullptr;
  Jacobian_j         = nullptr;
  Jacobian_ii        = nullptr;
  Jacobian_ij        = nullptr;
  Jacobian_ji        = nullptr;
  Jacobian_jj        = nullptr;
  base_nodes         = nullptr;
  nOutputVariables   = 0;
  ResLinSolver       = EPS;

  /*--- Variable initialization to avoid valgrid warnings when not used. ---*/

  IterLinSolver = 0;

  /*--- Initialize pointer for any verification solution. ---*/
  VerificationSolution  = nullptr;

  /*--- Flags for the periodic BC communications. ---*/

  rotate_periodic   = false;
  implicit_periodic = false;

  /*--- Containers to store the markers. ---*/
  nMarker = 0;

  /*--- Flags for the dynamic grid (rigid movement or unsteady deformation). ---*/
  dynamic_grid = false;

  /*--- Auxiliary data needed for CFL adaption. ---*/

  Old_Func = 0;
  New_Func = 0;
  NonLinRes_Counter = 0;

  nPrimVarGrad = 0;
  nPrimVar     = 0;

}

CSolver::~CSolver() {
  SU2_ZONE_SCOPED

  unsigned short iVar;

  /*--- Public variables, may be accessible outside ---*/

  delete [] OutputHeadingNames;

  /*--- Private ---*/

  delete [] Residual;
  delete [] Residual_i;
  delete [] Residual_j;
  delete [] Solution;
  delete [] Solution_i;
  delete [] Solution_j;
  delete [] Vector;
  delete [] Vector_i;
  delete [] Vector_j;
  delete [] Res_Conv;
  delete [] Res_Visc;
  delete [] Res_Sour;
  delete [] Res_Conv_i;
  delete [] Res_Conv_j;
  delete [] Res_Visc_i;
  delete [] Res_Visc_j;

  if (Jacobian_i != nullptr) {
    for (iVar = 0; iVar < nVar; iVar++)
      delete [] Jacobian_i[iVar];
    delete [] Jacobian_i;
  }

  if (Jacobian_j != nullptr) {
    for (iVar = 0; iVar < nVar; iVar++)
      delete [] Jacobian_j[iVar];
    delete [] Jacobian_j;
  }

  if (Jacobian_ii != nullptr) {
    for (iVar = 0; iVar < nVar; iVar++)
      delete [] Jacobian_ii[iVar];
    delete [] Jacobian_ii;
  }

  if (Jacobian_ij != nullptr) {
    for (iVar = 0; iVar < nVar; iVar++)
      delete [] Jacobian_ij[iVar];
    delete [] Jacobian_ij;
  }

  if (Jacobian_ji != nullptr) {
    for (iVar = 0; iVar < nVar; iVar++)
      delete [] Jacobian_ji[iVar];
    delete [] Jacobian_ji;
  }

  if (Jacobian_jj != nullptr) {
    for (iVar = 0; iVar < nVar; iVar++)
      delete [] Jacobian_jj[iVar];
    delete [] Jacobian_jj;
  }

  Restart_Vars = decltype(Restart_Vars){};
  Restart_Data = decltype(Restart_Data){};

  delete VerificationSolution;
}

void CSolver::GetPeriodicCommCountAndType(const CConfig* config,
                                          unsigned short commType,
                                          unsigned short &COUNT_PER_POINT,
                                          COMM_TYPE &MPI_TYPE,
                                          unsigned short &ICOUNT,
                                          unsigned short &JCOUNT) const {
  SU2_ZONE_SCOPED
  switch (commType) {
    case PERIODIC_VOLUME:
      COUNT_PER_POINT  = 1;
      MPI_TYPE         = COMM_TYPE::DOUBLE;
      break;
    case PERIODIC_NEIGHBORS:
      COUNT_PER_POINT  = 1;
      MPI_TYPE         = COMM_TYPE::UNSIGNED_SHORT;
      break;
    case PERIODIC_RESIDUAL:
      COUNT_PER_POINT  = nVar + nVar*nVar + 1;
      MPI_TYPE         = COMM_TYPE::DOUBLE;
      break;
    case PERIODIC_IMPLICIT:
      COUNT_PER_POINT  = nVar;
      MPI_TYPE         = COMM_TYPE::DOUBLE;
      break;
    case PERIODIC_LAPLACIAN:
      COUNT_PER_POINT  = nVar;
      MPI_TYPE         = COMM_TYPE::DOUBLE;
      break;
    case PERIODIC_MAX_EIG:
      COUNT_PER_POINT  = 1;
      MPI_TYPE         = COMM_TYPE::DOUBLE;
      break;
    case PERIODIC_SENSOR:
      COUNT_PER_POINT  = 2;
      MPI_TYPE         = COMM_TYPE::DOUBLE;
      break;
    case PERIODIC_SOL_GG:
    case PERIODIC_SOL_GG_R:
      COUNT_PER_POINT  = nVar*nDim;
      MPI_TYPE         = COMM_TYPE::DOUBLE;
      ICOUNT           = nVar;
      JCOUNT           = nDim;
      break;
    case PERIODIC_PRIM_GG:
    case PERIODIC_PRIM_GG_R:
      COUNT_PER_POINT  = nPrimVarGrad*nDim;
      MPI_TYPE         = COMM_TYPE::DOUBLE;
      ICOUNT           = nPrimVarGrad;
      JCOUNT           = nDim;
      break;
    case PERIODIC_SOL_LS:
    case PERIODIC_SOL_ULS:
    case PERIODIC_SOL_LS_R:
    case PERIODIC_SOL_ULS_R:
      COUNT_PER_POINT  = nDim*nDim + nVar*nDim;
      MPI_TYPE         = COMM_TYPE::DOUBLE;
      ICOUNT           = nVar;
      JCOUNT           = nDim;
      break;
    case PERIODIC_PRIM_LS:
    case PERIODIC_PRIM_ULS:
    case PERIODIC_PRIM_LS_R:
    case PERIODIC_PRIM_ULS_R:
      COUNT_PER_POINT  = nDim*nDim + nPrimVarGrad*nDim;
      MPI_TYPE         = COMM_TYPE::DOUBLE;
      ICOUNT           = nPrimVarGrad;
      JCOUNT           = nDim;
      break;
    case PERIODIC_LIM_PRIM_1:
      COUNT_PER_POINT  = nPrimVarGrad*2;
      MPI_TYPE         = COMM_TYPE::DOUBLE;
      ICOUNT           = nPrimVarGrad;
      break;
    case PERIODIC_LIM_PRIM_2:
      COUNT_PER_POINT  = nPrimVarGrad;
      MPI_TYPE         = COMM_TYPE::DOUBLE;
      ICOUNT           = nPrimVarGrad;
      break;
    case PERIODIC_LIM_SOL_1:
      COUNT_PER_POINT  = nVar*2;
      MPI_TYPE         = COMM_TYPE::DOUBLE;
      ICOUNT           = nVar;
      break;
    case PERIODIC_LIM_SOL_2:
      COUNT_PER_POINT  = nVar;
      MPI_TYPE         = COMM_TYPE::DOUBLE;
      ICOUNT           = nVar;
      break;
    case PERIODIC_ADAPT_GG:
      COUNT_PER_POINT  = base_nodes->GetAuxVar_Adapt().cols()*nDim;
      MPI_TYPE         = COMM_TYPE::DOUBLE;
      ICOUNT           = base_nodes->GetAuxVar_Adapt().cols();
      JCOUNT           = nDim;
      break;
    case PERIODIC_ADAPT_LS:
      COUNT_PER_POINT  = nDim*nDim + base_nodes->GetAuxVar_Adapt().cols()*nDim;
      MPI_TYPE         = COMM_TYPE::DOUBLE;
      ICOUNT           = base_nodes->GetAuxVar_Adapt().cols();
      JCOUNT           = nDim;
      break;
    case PERIODIC_HESS_GG:
      COUNT_PER_POINT  = nDim*nDim;
      MPI_TYPE         = COMM_TYPE::DOUBLE;
      ICOUNT           = nDim;
      JCOUNT           = nDim;
      break;
    case PERIODIC_HESS_LS:
      COUNT_PER_POINT  = 2*nDim*nDim;
      MPI_TYPE         = COMM_TYPE::DOUBLE;
      ICOUNT           = nDim;
      JCOUNT           = nDim;
      break;
    case PERIODIC_HESSIAN:
      COUNT_PER_POINT  = base_nodes->GetHessian().rows()*base_nodes->GetHessian().cols();
      MPI_TYPE         = COMM_TYPE::DOUBLE;
      break;
    case PERIODIC_METRIC:
      COUNT_PER_POINT  = 3*(nDim-1);
      MPI_TYPE         = COMM_TYPE::DOUBLE;
      break;
    default:
      SU2_MPI::Error("Unrecognized quantity for periodic communication.",
                     CURRENT_FUNCTION);
      break;
  }
}

namespace PeriodicCommHelpers {
  CVectorOfMatrix& selectGradient(CVariable* nodes, unsigned short commType) {
    switch(commType) {
      case PERIODIC_PRIM_GG:
      case PERIODIC_PRIM_LS:
      case PERIODIC_PRIM_ULS:
        return nodes->GetGradient_Primitive();
        break;
      case PERIODIC_SOL_GG:
      case PERIODIC_SOL_LS:
      case PERIODIC_SOL_ULS:
        return nodes->GetGradient();
        break;
      case PERIODIC_ADAPT_GG:
      case PERIODIC_ADAPT_LS:
        return nodes->GetGradient_Adapt();
        break;
      case PERIODIC_HESS_GG:
      case PERIODIC_HESS_LS:
        return nodes->GetHessian_Grad();
        break;
      default:
        return nodes->GetGradient_Reconstruction();
        break;
    }
  }

  const su2activematrix& selectField(CVariable* nodes, unsigned short commType) {
    switch(commType) {
      case PERIODIC_PRIM_GG:
      case PERIODIC_PRIM_LS:
      case PERIODIC_PRIM_ULS:
      case PERIODIC_PRIM_GG_R:
      case PERIODIC_PRIM_LS_R:
      case PERIODIC_PRIM_ULS_R:
      case PERIODIC_LIM_PRIM_1:
      case PERIODIC_LIM_PRIM_2:
        return nodes->GetPrimitive();
        break;
      case PERIODIC_ADAPT_LS:
        return nodes->GetAuxVar_Adapt();
        break;
      case PERIODIC_HESS_LS:
        return nodes->GetHessian_Field();
        break;
      default:
        return nodes->GetSolution();
        break;
    }
  }

  su2activematrix& selectLimiter(CVariable* nodes, unsigned short commType) {
    switch(commType) {
      case PERIODIC_LIM_PRIM_1:
      case PERIODIC_LIM_PRIM_2:
        return nodes->GetLimiter_Primitive();
        break;
      default:
        return nodes->GetLimiter();
        break;
    }
  }
}

void CSolver::InitiatePeriodicComms(CGeometry *geometry,
                                    const CConfig *config,
                                    unsigned short val_periodic_index,
                                    unsigned short commType) {
  SU2_ZONE_SCOPED

  /*--- Check for dummy communication. ---*/

  if (commType == PERIODIC_NONE) return;

  if (rotate_periodic && config->GetNEMOProblem()) {
    SU2_MPI::Error("The NEMO solvers do not support rotational periodicity yet.", CURRENT_FUNCTION);
  }

  /*--- Local variables ---*/

  bool boundary_i, boundary_j;
  bool weighted = true;

  unsigned short iVar, jVar, iDim;
  unsigned short nNeighbor       = 0;
  unsigned short COUNT_PER_POINT = 0;
  COMM_TYPE MPI_TYPE{};
  unsigned short ICOUNT          = nVar;
  unsigned short JCOUNT          = nVar;

  int iMessage, iSend, nSend;

  unsigned long iPoint, msg_offset, buf_offset, iPeriodic;

  auto *Diff      = new su2double[nVar];
  auto *Und_Lapl  = new su2double[nVar];
  auto *Sol_Min   = new su2double[std::max(nVar, nPrimVarGrad)];
  auto *Sol_Max   = new su2double[std::max(nVar, nPrimVarGrad)];
  auto *rotPrim_i = new su2double[std::max<unsigned long>({nVar, nPrimVar, base_nodes->GetAuxVar_Adapt().cols()})];
  auto *rotPrim_j = new su2double[std::max<unsigned long>({nVar, nPrimVar, base_nodes->GetAuxVar_Adapt().cols()})];

  su2double Sensor_i = 0.0, Sensor_j = 0.0, Pressure_i, Pressure_j;
  const su2double *Coord_i, *Coord_j;
  su2double r11, r12, r13, r22, r23_a, r23_b, r33, weight;
  const su2double *center, *angles, *trans;
  su2double rotMatrix2D[2][2] = {{1.0,0.0},{0.0,1.0}};
  su2double rotMatrix3D[3][3] = {{1.0,0.0,0.0},{0.0,1.0,0.0},{0.0,0.0,1.0}};
  su2double rotCoord_i[3] = {0.0}, rotCoord_j[3] = {0.0};
  su2double translation[3] = {0.0}, distance[3] = {0.0};
  const su2double zeros[3] = {0.0};
  su2activematrix Cvector;

  auto Rotate = [&](const su2double* origin, const su2double* direction, su2double* rotated) {
    if(nDim==2) GeometryToolbox::Rotate(rotMatrix2D, origin, direction, rotated);
    else GeometryToolbox::Rotate(rotMatrix3D, origin, direction, rotated);
  };

  /*--- Rotate a symmetric tensor stored as its upper triangle, rotated = R tensor R^T. ---*/

  auto RotateSymTensor = [&](const su2double* tensor, su2double* rotated) {
    su2double T[3][3] = {{0.0}}, R[3][3] = {{0.0}}, RT[3][3] = {{0.0}};
    for (unsigned short i = 0, iMet = 0; i < nDim; ++i) {
      for (unsigned short j = 0; j < nDim; ++j) R[i][j] = (nDim == 2) ? rotMatrix2D[i][j] : rotMatrix3D[i][j];
      for (unsigned short j = i; j < nDim; ++j, ++iMet) T[i][j] = T[j][i] = tensor[iMet];
    }
    for (unsigned short i = 0; i < nDim; ++i)
      for (unsigned short j = 0; j < nDim; ++j)
        for (unsigned short k = 0; k < nDim; ++k) RT[i][j] += R[i][k] * T[k][j];
    for (unsigned short i = 0, iMet = 0; i < nDim; ++i) {
      for (unsigned short j = i; j < nDim; ++j, ++iMet) {
        rotated[iMet] = 0.0;
        for (unsigned short k = 0; k < nDim; ++k) rotated[iMet] += RT[i][k] * R[j][k];
      }
    }
  };

  string Marker_Tag;

  /*--- Set the size of the data packet and type depending on quantity. ---*/

  GetPeriodicCommCountAndType(config, commType, COUNT_PER_POINT, MPI_TYPE, ICOUNT, JCOUNT);

  /*--- Allocate buffers for matrices that need rotation. ---*/

  su2activematrix jacBlock(ICOUNT,JCOUNT);
  su2activematrix rotBlock(ICOUNT,JCOUNT);

  /*--- Check to make sure we have created a large enough buffer
   for these comms during preprocessing. It will be reallocated whenever
   we find a larger count per point than currently exists. After the
   first cycle of comms, this should be inactive. ---*/

  geometry->AllocatePeriodicComms(COUNT_PER_POINT);

  /*--- Set some local pointers to make access simpler. ---*/

  su2double *bufDSend = geometry->bufD_PeriodicSend;

  unsigned short *bufSSend = geometry->bufS_PeriodicSend;

  /*--- Handle the different types of gradient and limiter. ---*/

  auto& gradient = PeriodicCommHelpers::selectGradient(base_nodes, commType);
  auto& limiter = PeriodicCommHelpers::selectLimiter(base_nodes, commType);
  auto& field = PeriodicCommHelpers::selectField(base_nodes, commType);

  /*--- Index of the first vector component of the field (and rows of its gradient), rotated with
   rotational periodicity (momentum for the solution), -1 if the variables are scalars. ---*/

  int idxVec = 1;
  if (commType == PERIODIC_ADAPT_GG || commType == PERIODIC_ADAPT_LS) idxVec = -1;
  if (commType == PERIODIC_HESS_GG || commType == PERIODIC_HESS_LS) idxVec = 0;

  /*--- Load the specified quantity from the solver into the generic
   communication buffer in the geometry class. ---*/

  if (geometry->nPeriodicSend > 0) {

    /*--- Post all non-blocking recvs first before sends. ---*/

    geometry->PostPeriodicRecvs(geometry, config, MPI_TYPE, COUNT_PER_POINT);

    for (iMessage = 0; iMessage < geometry->nPeriodicSend; iMessage++) {

      /*--- Get the offset in the buffer for the start of this message. ---*/

      msg_offset = geometry->nPoint_PeriodicSend[iMessage];

      /*--- Get the number of periodic points we need to
       communicate on the current periodic marker. ---*/

      nSend = (geometry->nPoint_PeriodicSend[iMessage+1] -
               geometry->nPoint_PeriodicSend[iMessage]);

      SU2_OMP_FOR_STAT(OMP_MIN_SIZE)
      for (iSend = 0; iSend < nSend; iSend++) {

        /*--- Get the local index for this communicated data. We need
         both the node and periodic face index (for rotations). ---*/

        iPoint    = geometry->Local_Point_PeriodicSend[msg_offset  + iSend];
        iPeriodic = geometry->Local_Marker_PeriodicSend[msg_offset + iSend];

        /*--- Retrieve the supplied periodic information. ---*/

        Marker_Tag = config->GetMarker_All_TagBound(iPeriodic);
        center     = config->GetPeriodicRotCenter(Marker_Tag);
        angles     = config->GetPeriodicRotAngles(Marker_Tag);
        trans      = config->GetPeriodicTranslation(Marker_Tag);

        /*--- Store (center+trans) as it is constant and will be added. ---*/

        translation[0] = center[0] + trans[0];
        translation[1] = center[1] + trans[1];
        translation[2] = center[2] + trans[2];

        /*--- Store angles separately for clarity. Compute sines/cosines. ---*/

        su2double Theta = angles[0];
        su2double Phi = angles[1];
        su2double Psi = angles[2];

        /*--- Compute the rotation matrix. Note that the implicit
         ordering is rotation about the x-axis, y-axis, then z-axis. ---*/

        if (nDim==2) {
          GeometryToolbox::RotationMatrix(Psi, rotMatrix2D);
        } else {
          GeometryToolbox::RotationMatrix(Theta, Phi, Psi, rotMatrix3D);
        }

        /*--- Compute the offset in the recv buffer for this point. ---*/

        buf_offset = (msg_offset + iSend)*COUNT_PER_POINT;

        /*--- Load the send buffers depending on the particular value
         that has been requested for communication. ---*/

        switch (commType) {

          case PERIODIC_VOLUME:

            /*--- Load the volume of the current periodic CV so that
             we can accumulate the total control volume size on all
             periodic faces. ---*/

            bufDSend[buf_offset] = geometry->nodes->GetVolume(iPoint) +
            geometry->nodes->GetPeriodicVolume(iPoint);

            break;

          case PERIODIC_NEIGHBORS:

            nNeighbor = 0;
            for (auto jPoint : geometry->nodes->GetPoints(iPoint)) {

              /*--- Check if this neighbor lies on the periodic face so
               that we avoid double counting neighbors on both sides. If
               not, increment the count of neighbors for the donor. ---*/

              if (!geometry->nodes->GetPeriodicBoundary(jPoint))
                nNeighbor++;
            }

            /*--- Store the number of neighbors in bufffer. ---*/

            bufSSend[buf_offset] = nNeighbor;

            break;

          case PERIODIC_RESIDUAL:

            /*--- Communicate the residual from our partial control
             volume to the other side of the periodic face. ---*/

            for (iVar = 0; iVar < nVar; iVar++) {
              bufDSend[buf_offset+iVar] = LinSysRes(iPoint, iVar);
            }

            /*--- Rotate the momentum components of the residual array. ---*/

            if (rotate_periodic) {
              Rotate(zeros, &LinSysRes(iPoint,1), &bufDSend[buf_offset+1]);
            }
            buf_offset += nVar;

            /*--- Load the time step for the current point. ---*/

            bufDSend[buf_offset] = base_nodes->GetDelta_Time(iPoint);
            buf_offset++;

            /*--- For implicit calculations, we will communicate the
             contributions to the Jacobian block diagonal, i.e., the
             impact of the point upon itself, J_ii. ---*/

            if (implicit_periodic) {

              const auto block = Jacobian.GetBlockView(iPoint, iPoint);

              for (iVar = 0; iVar < nVar; iVar++) {
                for (jVar = 0; jVar < nVar; jVar++) {
                  jacBlock[iVar][jVar] = block(iVar, jVar);
                }
              }

              /*--- Rotate the momentum columns of the Jacobian. ---*/

              if (rotate_periodic) {
                for (iVar = 0; iVar < nVar; iVar++) {
                  if (nDim == 2) {
                    jacBlock[1][iVar] = rotMatrix2D[0][0]*block(1, iVar) + rotMatrix2D[0][1]*block(2, iVar);
                    jacBlock[2][iVar] = rotMatrix2D[1][0]*block(1, iVar) + rotMatrix2D[1][1]*block(2, iVar);
                  } else {
                    jacBlock[1][iVar] = rotMatrix3D[0][0]*block(1, iVar) + rotMatrix3D[0][1]*block(2, iVar) +
                                        rotMatrix3D[0][2]*block(3, iVar);
                    jacBlock[2][iVar] = rotMatrix3D[1][0]*block(1, iVar) + rotMatrix3D[1][1]*block(2, iVar) +
                                        rotMatrix3D[1][2]*block(3, iVar);
                    jacBlock[3][iVar] = rotMatrix3D[2][0]*block(1, iVar) + rotMatrix3D[2][1]*block(2, iVar) +
                                        rotMatrix3D[2][2]*block(3, iVar);
                  }
                }
              }

              /*--- Load the Jacobian terms into the buffer for sending. ---*/

              for (iVar = 0; iVar < nVar; iVar++) {
                for (jVar = 0; jVar < nVar; jVar++) {
                  bufDSend[buf_offset] = jacBlock[iVar][jVar];
                  buf_offset++;
                }
              }
            }

            break;

          case PERIODIC_IMPLICIT:

            /*--- Communicate the solution from our master set of periodic
             nodes (from the linear solver perspective) to the passive
             periodic nodes on the matching face. This is done at the
             end of the iteration to synchronize the solution after the
             linear solve. ---*/

            for (iVar = 0; iVar < nVar; iVar++) {
              bufDSend[buf_offset+iVar] = base_nodes->GetSolution(iPoint, iVar);
            }

            /*--- Rotate the momentum components of the solution array. ---*/

            if (rotate_periodic) {
              Rotate(zeros, &base_nodes->GetSolution(iPoint)[1], &bufDSend[buf_offset+1]);
            }

            break;

          case PERIODIC_LAPLACIAN:

            /*--- For JST, the undivided Laplacian must be computed
             consistently by using the complete control volume info
             from both sides of the periodic face. ---*/

            for (iVar = 0; iVar < nVar; iVar++)
              Und_Lapl[iVar] = 0.0;

            for (auto jPoint : geometry->nodes->GetPoints(iPoint)) {

              /*--- Avoid periodic boundary points so that we do not
               duplicate edges on both sides of the periodic BC. ---*/

              if (!geometry->nodes->GetPeriodicBoundary(jPoint)) {

                /*--- Solution differences ---*/

                for (iVar = 0; iVar < nVar; iVar++)
                Diff[iVar] = (base_nodes->GetSolution(iPoint, iVar) -
                              base_nodes->GetSolution(jPoint,iVar));

                /*--- Correction for compressible flows (use enthalpy) ---*/

                if (!(config->GetKind_Regime() == ENUM_REGIME::INCOMPRESSIBLE)) {
                  Pressure_i   = base_nodes->GetPressure(iPoint);
                  Pressure_j   = base_nodes->GetPressure(jPoint);
                  Diff[nVar-1] = ((base_nodes->GetSolution(iPoint,nVar-1) + Pressure_i) -
                                  (base_nodes->GetSolution(jPoint,nVar-1) + Pressure_j));
                }

                boundary_i = geometry->nodes->GetPhysicalBoundary(iPoint);
                boundary_j = geometry->nodes->GetPhysicalBoundary(jPoint);

                /*--- Both points inside the domain, or both in the boundary ---*/
                /*--- iPoint inside the domain, jPoint on the boundary ---*/

                if (!boundary_i || boundary_j) {
                  if (geometry->nodes->GetDomain(iPoint)){
                    for (iVar = 0; iVar< nVar; iVar++)
                    Und_Lapl[iVar] -= Diff[iVar];
                  }
                }
              }
            }

            /*--- Store the components to be communicated in the buffer. ---*/

            for (iVar = 0; iVar < nVar; iVar++)
              bufDSend[buf_offset+iVar] = Und_Lapl[iVar];

            /*--- Rotate the momentum components of the Laplacian. ---*/

            if (rotate_periodic) {
              Rotate(zeros, &Und_Lapl[1], &bufDSend[buf_offset+1]);
            }

            break;

          case PERIODIC_MAX_EIG:

            /*--- Simple summation of eig calc on both periodic faces. ---*/

            bufDSend[buf_offset] = base_nodes->GetLambda(iPoint);

            break;

          case PERIODIC_SENSOR: {
            const bool msw = config->GetKind_Upwind_Flow() == UPWIND::MSW;

            /*--- For the centered schemes, the sensor must be computed
             consistently using info from the entire control volume
             on both sides of the periodic face. ---*/

            Sensor_i = 0; Sensor_j = 0;
            for (auto jPoint : geometry->nodes->GetPoints(iPoint)) {

              /*--- Avoid halos and boundary points so that we don't
               duplicate edges on both sides of the periodic BC. ---*/

              if (geometry->nodes->GetPeriodicBoundary(jPoint)) continue;

              /*--- Use density instead of pressure for incomp. flows. ---*/

              if (config->GetKind_Regime() == ENUM_REGIME::INCOMPRESSIBLE) {
                Pressure_i = base_nodes->GetDensity(iPoint);
                Pressure_j = base_nodes->GetDensity(jPoint);
              } else {
                Pressure_i = base_nodes->GetPressure(iPoint);
                Pressure_j = base_nodes->GetPressure(jPoint);
              }

              boundary_i = geometry->nodes->GetPhysicalBoundary(iPoint);
              boundary_j = geometry->nodes->GetPhysicalBoundary(jPoint);

              /*--- Both points inside domain, or both on boundary ---*/
              /*--- iPoint inside the domain, jPoint on the boundary ---*/

              if ((!boundary_i || boundary_j) && geometry->nodes->GetDomain(iPoint)) {
                if (msw) {
                  Sensor_i = fmax(Sensor_i, fabs(Pressure_j - Pressure_i)) / fmin(Pressure_i, Pressure_j);
                } else {
                  Sensor_i += (Pressure_j - Pressure_i);
                  Sensor_j += (Pressure_i + Pressure_j);
                }
              }

            }

            /*--- Store the sensor increments to buffer. After summing
             all contributions, these will be divided. ---*/

            bufDSend[buf_offset] = Sensor_i;
            buf_offset++;
            bufDSend[buf_offset] = Sensor_j;

          } break;

          case PERIODIC_SOL_GG:
          case PERIODIC_SOL_GG_R:
          case PERIODIC_PRIM_GG:
          case PERIODIC_PRIM_GG_R:
          case PERIODIC_ADAPT_GG:
          case PERIODIC_HESS_GG:

            /*--- Access and rotate the partial G-G gradient. These will be
             summed on both sides of the periodic faces before dividing
             by the volume to complete the Green-Gauss gradient calc. ---*/

            for (iVar = 0; iVar < ICOUNT; iVar++) {
              for (iDim = 0; iDim < nDim; iDim++) {
                jacBlock[iVar][iDim] = gradient(iPoint, iVar, iDim);
              }
            }

            /*--- Rotate the gradients in x,y,z space for all variables. ---*/

            for (iVar = 0; iVar < ICOUNT; iVar++) {
              Rotate(zeros, jacBlock[iVar], rotBlock[iVar]);
            }

            /*--- Rotate the vector components of the solution. ---*/

            if (rotate_periodic && idxVec >= 0) {
              for (iDim = 0; iDim < nDim; iDim++) {
                su2double d_diDim[3] = {0.0};
                for (iVar = idxVec; iVar < idxVec+nDim; ++iVar) {
                  d_diDim[iVar-idxVec] = rotBlock(iVar, iDim);
                }
                su2double rotated[3] = {0.0};
                Rotate(zeros, d_diDim, rotated);
                for (iVar = idxVec; iVar < idxVec+nDim; ++iVar) {
                  rotBlock(iVar, iDim) = rotated[iVar-idxVec];
                }
              }
            }

            /*--- Store the partial gradient in the buffer. ---*/

            for (iVar = 0; iVar < ICOUNT; iVar++) {
              for (iDim = 0; iDim < nDim; iDim++) {
                bufDSend[buf_offset+iVar*nDim+iDim] = rotBlock[iVar][iDim];
              }
            }

            break;

          case PERIODIC_SOL_LS: case PERIODIC_SOL_ULS:
          case PERIODIC_SOL_LS_R: case PERIODIC_SOL_ULS_R:
          case PERIODIC_PRIM_LS: case PERIODIC_PRIM_ULS:
          case PERIODIC_PRIM_LS_R: case PERIODIC_PRIM_ULS_R:
          case PERIODIC_ADAPT_LS: case PERIODIC_HESS_LS:

            /*--- For L-S gradient calculations with rotational periodicity,
             we will need to rotate the x,y,z components. To make the process
             easier, we choose to rotate the initial periodic point and their
             neighbor points into their location on the donor marker before
             computing the terms that we need to communicate. ---*/

            /*--- Set a flag for unweighted or weighted least-squares. ---*/

            switch(commType) {
              case PERIODIC_SOL_ULS:
              case PERIODIC_SOL_ULS_R:
              case PERIODIC_PRIM_ULS:
              case PERIODIC_PRIM_ULS_R:
                weighted = false;
                break;
              default:
                weighted = true;
                break;
            }

            /*--- Get coordinates for the current point. ---*/

            Coord_i = geometry->nodes->GetCoord(iPoint);

            /*--- Get the position vector from rotation center to point. ---*/

            GeometryToolbox::Distance(nDim, Coord_i, center, distance);

            /*--- Compute transformed point coordinates. ---*/

            Rotate(translation, distance, rotCoord_i);

            /*--- Get conservative solution and rotate if necessary. ---*/

            for (iVar = 0; iVar < ICOUNT; iVar++)
              rotPrim_i[iVar] = field(iPoint, iVar);

            if (rotate_periodic && idxVec >= 0) {
              Rotate(zeros, &field(iPoint,idxVec), &rotPrim_i[idxVec]);
            }

            /*--- Inizialization of variables ---*/

            Cvector.resize(ICOUNT,nDim) = su2double(0.0);

            r11 = 0.0;   r12 = 0.0;   r22 = 0.0;
            r13 = 0.0; r23_a = 0.0; r23_b = 0.0;  r33 = 0.0;

            for (auto jPoint : geometry->nodes->GetPoints(iPoint)) {

              /*--- Avoid periodic boundary points so that we do not
               duplicate edges on both sides of the periodic BC. ---*/

              if (!geometry->nodes->GetPeriodicBoundary(jPoint)) {

                /*--- Get coordinates for the neighbor point. ---*/

                Coord_j = geometry->nodes->GetCoord(jPoint);

                /*--- Get the position vector from rotation center. ---*/

                GeometryToolbox::Distance(nDim, Coord_j, center, distance);

                /*--- Compute transformed point coordinates. ---*/

                Rotate(translation, distance, rotCoord_j);

                /*--- Get conservative solution and rotate if necessary. ---*/

                for (iVar = 0; iVar < ICOUNT; iVar++)
                  rotPrim_j[iVar] = field(jPoint,iVar);

                if (rotate_periodic && idxVec >= 0) {
                  Rotate(zeros, &field(jPoint,idxVec), &rotPrim_j[idxVec]);
                }

                if (weighted) {
                  weight = GeometryToolbox::SquaredDistance(nDim, rotCoord_j, rotCoord_i);
                } else {
                  weight = 1.0;
                }

                /*--- Sumations for entries of upper triangular matrix R ---*/

                if (weight != 0.0) {

                  r11 += ((rotCoord_j[0]-rotCoord_i[0])*
                          (rotCoord_j[0]-rotCoord_i[0])/weight);
                  r12 += ((rotCoord_j[0]-rotCoord_i[0])*
                          (rotCoord_j[1]-rotCoord_i[1])/weight);
                  r22 += ((rotCoord_j[1]-rotCoord_i[1])*
                          (rotCoord_j[1]-rotCoord_i[1])/weight);

                  if (nDim == 3) {
                    r13   += ((rotCoord_j[0]-rotCoord_i[0])*
                              (rotCoord_j[2]-rotCoord_i[2])/weight);
                    r23_a += ((rotCoord_j[1]-rotCoord_i[1])*
                              (rotCoord_j[2]-rotCoord_i[2])/weight);
                    r23_b += ((rotCoord_j[0]-rotCoord_i[0])*
                              (rotCoord_j[2]-rotCoord_i[2])/weight);
                    r33   += ((rotCoord_j[2]-rotCoord_i[2])*
                              (rotCoord_j[2]-rotCoord_i[2])/weight);
                  }

                  /*--- Entries of c:= transpose(A)*b ---*/

                  for (iVar = 0; iVar < ICOUNT; iVar++)
                  for (iDim = 0; iDim < nDim; iDim++)
                  Cvector(iVar,iDim) += ((rotCoord_j[iDim]-rotCoord_i[iDim])*
                                          (rotPrim_j[iVar]-rotPrim_i[iVar])/weight);

                }
              }
            }

            /*--- We store and communicate the increments for the matching
             upper triangular matrix (weights) and the r.h.s. vector.
             These will be accumulated before completing the L-S gradient
             calculation for each periodic point. ---*/

            if (nDim == 2) {
              bufDSend[buf_offset] = r11;   buf_offset++;
              bufDSend[buf_offset] = r12;   buf_offset++;
              bufDSend[buf_offset] = 0.0;   buf_offset++;
              bufDSend[buf_offset] = r22;   buf_offset++;
            }
            if (nDim == 3) {
              bufDSend[buf_offset] = r11;   buf_offset++;
              bufDSend[buf_offset] = r12;   buf_offset++;
              bufDSend[buf_offset] = r13;   buf_offset++;

              bufDSend[buf_offset] = 0.0;   buf_offset++;
              bufDSend[buf_offset] = r22;   buf_offset++;
              bufDSend[buf_offset] = r23_a; buf_offset++;

              bufDSend[buf_offset] = 0.0;   buf_offset++;
              bufDSend[buf_offset] = r23_b; buf_offset++;
              bufDSend[buf_offset] = r33;   buf_offset++;
            }

            for (iVar = 0; iVar < ICOUNT; iVar++) {
              for (iDim = 0; iDim < nDim; iDim++) {
                bufDSend[buf_offset] = Cvector(iVar,iDim);
                buf_offset++;
              }
            }

            break;

          case PERIODIC_LIM_PRIM_1:
          case PERIODIC_LIM_SOL_1:

            /*--- The first phase of the periodic limiter calculation
             ensures that the proper min and max of the solution are found
             among all nodes adjacent to periodic faces. ---*/

            /*--- We send the min and max over "our" neighbours. ---*/

            for (iVar = 0; iVar < ICOUNT; iVar++) {
              Sol_Min[iVar] = base_nodes->GetSolution_Min()(iPoint, iVar);
              Sol_Max[iVar] = base_nodes->GetSolution_Max()(iPoint, iVar);
            }

            for (auto jPoint : geometry->nodes->GetPoints(iPoint)) {
              for (iVar = 0; iVar < ICOUNT; iVar++) {
                Sol_Min[iVar] = min(Sol_Min[iVar], field(jPoint, iVar));
                Sol_Max[iVar] = max(Sol_Max[iVar], field(jPoint, iVar));
              }
            }

            for (iVar = 0; iVar < ICOUNT; iVar++) {
              bufDSend[buf_offset+iVar]        = Sol_Min[iVar];
              bufDSend[buf_offset+ICOUNT+iVar] = Sol_Max[iVar];
            }

            /*--- Rotate the momentum components of the min/max. ---*/

            if (rotate_periodic) {
              Rotate(zeros, &Sol_Min[1], &bufDSend[buf_offset+1]);
              Rotate(zeros, &Sol_Max[1], &bufDSend[buf_offset+ICOUNT+1]);
            }

            break;

          case PERIODIC_LIM_PRIM_2:
          case PERIODIC_LIM_SOL_2:

            /*--- The second phase of the periodic limiter calculation
             ensures that the correct minimum value of the limiter is
             found for a node on a periodic face and stores it. ---*/

            for (iVar = 0; iVar < ICOUNT; iVar++) {
              bufDSend[buf_offset+iVar] = limiter(iPoint, iVar);
            }

            if (rotate_periodic) {
              Rotate(zeros, &limiter(iPoint,1), &bufDSend[buf_offset+1]);
            }

            break;

          case PERIODIC_HESSIAN: {

            /*--- The sensor Hessians are rotated like tensors, they will overwrite those of the
             matching periodic points on one side of the pair (see CompletePeriodicComms). ---*/

            const auto& hessian = base_nodes->GetHessian();
            const auto nMet = hessian.cols();
            su2double tensor[6] = {0.0};
            for (iVar = 0; iVar < hessian.rows(); iVar++) {
              for (iDim = 0; iDim < nMet; iDim++) tensor[iDim] = hessian(iPoint, iVar, iDim);
              RotateSymTensor(tensor, &bufDSend[buf_offset+iVar*nMet]);
            }

          } break;

          case PERIODIC_METRIC: {

            /*--- Same for the metric tensor. ---*/

            su2double tensor[6] = {0.0};
            for (iDim = 0; iDim < COUNT_PER_POINT; iDim++) tensor[iDim] = base_nodes->GetMetric(iPoint, iDim);
            RotateSymTensor(tensor, &bufDSend[buf_offset]);

          } break;

          default:
            SU2_MPI::Error("Unrecognized quantity for periodic communication.",
                           CURRENT_FUNCTION);
            break;
        }
      }
      END_SU2_OMP_FOR

      /*--- Launch the point-to-point MPI send for this message. ---*/

      geometry->PostPeriodicSends(geometry, config, MPI_TYPE, COUNT_PER_POINT, iMessage);

    }
  }

  delete [] Diff;
  delete [] Und_Lapl;
  delete [] Sol_Min;
  delete [] Sol_Max;
  delete [] rotPrim_i;
  delete [] rotPrim_j;

}

void CSolver::CompletePeriodicComms(CGeometry *geometry,
                                    const CConfig *config,
                                    unsigned short val_periodic_index,
                                    unsigned short commType) {
  SU2_ZONE_SCOPED

  /*--- Check for dummy communication. ---*/

  if (commType == PERIODIC_NONE) return;

  /*--- Set the size of the data packet and type depending on quantity. ---*/

  unsigned short COUNT_PER_POINT = 0, ICOUNT = 0, JCOUNT = 0;
  COMM_TYPE MPI_TYPE{};
  GetPeriodicCommCountAndType(config, commType, COUNT_PER_POINT, MPI_TYPE, ICOUNT, JCOUNT);

  /*--- Local variables ---*/

  unsigned short nPeriodic = config->GetnMarker_Periodic();
  unsigned short iDim, jDim, iVar, jVar, iPeriodic, nNeighbor;

  unsigned long iPoint, iRecv, nRecv, msg_offset, buf_offset;

  int source, iMessage, jRecv;

  /*--- Status is global so all threads can see the result of Waitany. ---*/
  static SU2_MPI::Status status;

  auto *Diff = new su2double[nVar];

  su2double Time_Step, Volume;

  su2double **Jacobian_i = nullptr;
  if ((commType == PERIODIC_RESIDUAL) && implicit_periodic) {
    Jacobian_i = new su2double* [nVar];
    for (iVar = 0; iVar < nVar; iVar++)
      Jacobian_i[iVar] = new su2double [nVar];
  }

  /*--- Set some local pointers to make access simpler. ---*/

  const su2double *bufDRecv = geometry->bufD_PeriodicRecv;

  const unsigned short *bufSRecv = geometry->bufS_PeriodicRecv;

  /*--- Handle the different types of gradient and limiter. ---*/

  auto& gradient = PeriodicCommHelpers::selectGradient(base_nodes, commType);
  auto& limiter = PeriodicCommHelpers::selectLimiter(base_nodes, commType);

  /*--- Store the data that was communicated into the appropriate
   location within the local class data structures. ---*/

  if (geometry->nPeriodicRecv > 0) {

    for (iMessage = 0; iMessage < geometry->nPeriodicRecv; iMessage++) {

      /*--- For efficiency, recv the messages dynamically based on
       the order they arrive. ---*/

#ifdef HAVE_MPI
      /*--- Once we have recv'd a message, get the source rank. ---*/
      int ind;
      SU2_OMP_SAFE_GLOBAL_ACCESS(SU2_MPI::Waitany(geometry->nPeriodicRecv, geometry->req_PeriodicRecv, &ind, &status);)
      source = status.MPI_SOURCE;
#else
      /*--- For serial calculations, we know the rank. ---*/
      source = rank;
      SU2_OMP_BARRIER
#endif

      /*--- We know the offsets based on the source rank. ---*/

      jRecv = geometry->PeriodicRecv2Neighbor[source];

      /*--- Get the offset in the buffer for the start of this message. ---*/

      msg_offset = geometry->nPoint_PeriodicRecv[jRecv];

      /*--- Get the number of packets to be received in this message. ---*/

      nRecv = (geometry->nPoint_PeriodicRecv[jRecv+1] -
               geometry->nPoint_PeriodicRecv[jRecv]);

      SU2_OMP_FOR_STAT(OMP_MIN_SIZE)
      for (iRecv = 0; iRecv < nRecv; iRecv++) {

        /*--- Get the local index for this communicated data. ---*/

        iPoint    = geometry->Local_Point_PeriodicRecv[msg_offset  + iRecv];
        iPeriodic = geometry->Local_Marker_PeriodicRecv[msg_offset + iRecv];

        /*--- While all periodic face data was accumulated, we only store
         the values for the current pair of periodic faces. This is slightly
         inefficient when we have multiple pairs of periodic faces, but
         it simplifies the communications. ---*/

        if ((iPeriodic == val_periodic_index) ||
            (iPeriodic == val_periodic_index + nPeriodic/2)) {

          /*--- Compute the offset in the recv buffer for this point. ---*/

          buf_offset = (msg_offset + iRecv)*COUNT_PER_POINT;

          /*--- Store the data correctly depending on the quantity. ---*/

          switch (commType) {

            case PERIODIC_VOLUME:

              /*--- The periodic points need to keep track of their
               total volume spread across the periodic faces. ---*/

              Volume = (bufDRecv[buf_offset] +
                        geometry->nodes->GetPeriodicVolume(iPoint));
              geometry->nodes->SetPeriodicVolume(iPoint, Volume);

              break;

            case PERIODIC_NEIGHBORS:

              /*--- Store the extra neighbors on the periodic face. ---*/

              nNeighbor = (geometry->nodes->GetnNeighbor(iPoint) +
                           bufSRecv[buf_offset]);
              geometry->nodes->SetnNeighbor(iPoint, nNeighbor);

              break;

            case PERIODIC_RESIDUAL:

              /*--- Add contributions to total residual. ---*/

              LinSysRes.AddBlock(iPoint, &bufDRecv[buf_offset]);
              buf_offset += nVar;

              /*--- Check the computed time step against the donor
               value and keep the minimum in order to be conservative. ---*/

              Time_Step = base_nodes->GetDelta_Time(iPoint);
              if (bufDRecv[buf_offset] < Time_Step)
                base_nodes->SetDelta_Time(iPoint,bufDRecv[buf_offset]);
              buf_offset++;

              /*--- For implicit integration, we choose the first
               periodic face of each pair to be the master/owner of
               the solution for the linear system while fixing the
               solution at the matching face during the solve. Here,
               we remove the Jacobian and residual contributions from
               the passive face such that it does not participate in
               the linear solve. ---*/

              if (implicit_periodic) {

                for (iVar = 0; iVar < nVar; iVar++) {
                  for (jVar = 0; jVar < nVar; jVar++) {
                    Jacobian_i[iVar][jVar] = bufDRecv[buf_offset];
                    buf_offset++;
                  }
                }

                Jacobian.AddBlock2Diag(iPoint, Jacobian_i);

                if (iPeriodic == val_periodic_index + nPeriodic/2) {
                  for (iVar = 0; iVar < nVar; iVar++) {
                    LinSysRes(iPoint, iVar) = 0.0;
                    Jacobian.DeleteValsRowi(iPoint, iVar);
                  }
                }

              }

              break;

            case PERIODIC_IMPLICIT:

              /*--- For implicit integration, we choose the first
               periodic face of each pair to be the master/owner of
               the solution for the linear system while fixing the
               solution at the matching face during the solve. Here,
               we are updating the solution at the passive nodes
               using the new solution from the master. ---*/

              if ((implicit_periodic) &&
                  (iPeriodic == val_periodic_index + nPeriodic/2)) {

                /*--- Directly set the solution on the passive periodic
                 face that is provided from the master. ---*/

                for (iVar = 0; iVar < nVar; iVar++) {
                  base_nodes->SetSolution(iPoint, iVar, bufDRecv[buf_offset]);
                  base_nodes->SetSolution_Old(iPoint, iVar, bufDRecv[buf_offset]);
                  buf_offset++;
                }

              }

              break;

            case PERIODIC_LAPLACIAN:

              /*--- Adjust the undivided Laplacian. The accumulation was
               with a subtraction before communicating, so now just add. ---*/

              for (iVar = 0; iVar < nVar; iVar++)
                base_nodes->AddUnd_Lapl(iPoint, iVar, bufDRecv[buf_offset+iVar]);

              break;

            case PERIODIC_MAX_EIG:

              /*--- Simple accumulation of the max eig on periodic faces. ---*/

              base_nodes->AddLambda(iPoint,bufDRecv[buf_offset]);

              break;

            case PERIODIC_SENSOR:

              /*--- Simple accumulation of the sensors on periodic faces. ---*/

              if (config->GetKind_Upwind_Flow() == UPWIND::MSW) {
                iPoint_UndLapl[iPoint] = fmax(iPoint_UndLapl[iPoint], bufDRecv[buf_offset++]);
                jPoint_UndLapl[iPoint] = 1;
              } else {
                iPoint_UndLapl[iPoint] += bufDRecv[buf_offset++];
                jPoint_UndLapl[iPoint] += bufDRecv[buf_offset];
              }

              break;

            case PERIODIC_SOL_GG:
            case PERIODIC_SOL_GG_R:
            case PERIODIC_PRIM_GG:
            case PERIODIC_PRIM_GG_R:
            case PERIODIC_ADAPT_GG:
            case PERIODIC_HESS_GG:

              /*--- For G-G, we accumulate partial gradients then compute
               the final value using the entire volume of the periodic cell. ---*/

              for (iVar = 0; iVar < ICOUNT; iVar++)
                for (iDim = 0; iDim < nDim; iDim++)
                  gradient(iPoint, iVar, iDim) += bufDRecv[buf_offset+iVar*nDim+iDim];

              break;

            case PERIODIC_SOL_LS: case PERIODIC_SOL_ULS:
            case PERIODIC_SOL_LS_R: case PERIODIC_SOL_ULS_R:
            case PERIODIC_PRIM_LS: case PERIODIC_PRIM_ULS:
            case PERIODIC_PRIM_LS_R: case PERIODIC_PRIM_ULS_R:
            case PERIODIC_ADAPT_LS: case PERIODIC_HESS_LS:

              /*--- For L-S, we build the upper triangular matrix and the
               r.h.s. vector by accumulating from all periodic partial
               control volumes. ---*/

              for (iDim = 0; iDim < nDim; iDim++) {
                for (jDim = 0; jDim < nDim; jDim++) {
                  base_nodes->AddRmatrix(iPoint, iDim,jDim,bufDRecv[buf_offset]);
                  buf_offset++;
                }
              }
              for (iVar = 0; iVar < ICOUNT; iVar++) {
                for (iDim = 0; iDim < nDim; iDim++) {
                  gradient(iPoint, iVar, iDim) += bufDRecv[buf_offset];
                  buf_offset++;
                }
              }

              break;

            case PERIODIC_LIM_PRIM_1:
            case PERIODIC_LIM_SOL_1:

              /*--- Update solution min/max with min/max between "us" and
               the periodic match plus its neighbors, computation will need to
               be concluded on "our" side to account for "our" neighbors. ---*/

              for (iVar = 0; iVar < ICOUNT; iVar++) {

                /*--- Solution minimum. ---*/

                su2double Solution_Min = min(base_nodes->GetSolution_Min()(iPoint, iVar),
                                             bufDRecv[buf_offset+iVar]);
                base_nodes->GetSolution_Min()(iPoint, iVar) = Solution_Min;

                /*--- Solution maximum. ---*/

                su2double Solution_Max = max(base_nodes->GetSolution_Max()(iPoint, iVar),
                                             bufDRecv[buf_offset+ICOUNT+iVar]);
                base_nodes->GetSolution_Max()(iPoint, iVar) = Solution_Max;
              }

              break;

            case PERIODIC_LIM_PRIM_2:
            case PERIODIC_LIM_SOL_2:

              /*--- Check the min values found on the matching periodic
               faces for the limiter, and store the proper min value. ---*/

              for (iVar = 0; iVar < ICOUNT; iVar++)
                limiter(iPoint, iVar) = min(limiter(iPoint, iVar), bufDRecv[buf_offset+iVar]);

              break;

            case PERIODIC_HESSIAN:
            case PERIODIC_METRIC:

              /*--- As for PERIODIC_IMPLICIT, the values of one side of the pair are copied
               to the matching points, so both sides have the same (rotated) tensors. ---*/

              if (iPeriodic == val_periodic_index + nPeriodic/2) {
                if (commType == PERIODIC_METRIC) {
                  for (iVar = 0; iVar < COUNT_PER_POINT; iVar++)
                    base_nodes->SetMetric(iPoint, iVar, bufDRecv[buf_offset+iVar]);
                } else {
                  auto& hessian = base_nodes->GetHessian();
                  const auto nMet = hessian.cols();
                  for (iVar = 0; iVar < hessian.rows(); iVar++)
                    for (iDim = 0; iDim < nMet; iDim++)
                      hessian(iPoint, iVar, iDim) = bufDRecv[buf_offset+iVar*nMet+iDim];
                }
              }

              break;

            default:

              SU2_MPI::Error("Unrecognized quantity for periodic communication.",
                             CURRENT_FUNCTION);
              break;

          }
        }
      }
      END_SU2_OMP_FOR
    }

    /*--- Verify that all non-blocking point-to-point sends have finished.
     Note that this should be satisfied, as we have received all of the
     data in the loop above at this point. ---*/

#ifdef HAVE_MPI
    SU2_OMP_SAFE_GLOBAL_ACCESS(SU2_MPI::Waitall(geometry->nPeriodicSend, geometry->req_PeriodicSend, MPI_STATUS_IGNORE);)
#endif
  }

  delete [] Diff;

  if (Jacobian_i)
    for (iVar = 0; iVar < nVar; iVar++)
      delete [] Jacobian_i[iVar];
  delete [] Jacobian_i;

}

void CSolver::GetCommCountAndType(const CConfig* config,
                                  MPI_QUANTITIES commType,
                                  unsigned short &COUNT_PER_POINT,
                                  COMM_TYPE &MPI_TYPE) const {
  SU2_ZONE_SCOPED
  switch (commType) {
    case MPI_QUANTITIES::SOLUTION:
    case MPI_QUANTITIES::SOLUTION_OLD:
    case MPI_QUANTITIES::UNDIVIDED_LAPLACIAN:
    case MPI_QUANTITIES::SOLUTION_LIMITER:
      COUNT_PER_POINT  = nVar;
      MPI_TYPE         = COMM_TYPE::DOUBLE;
      break;
    case MPI_QUANTITIES::MAX_EIGENVALUE:
    case MPI_QUANTITIES::SENSOR:
      COUNT_PER_POINT  = 1;
      MPI_TYPE         = COMM_TYPE::DOUBLE;
      break;
    case MPI_QUANTITIES::SOLUTION_GRADIENT:
    case MPI_QUANTITIES::SOLUTION_GRAD_REC:
      COUNT_PER_POINT  = nVar*nDim;
      MPI_TYPE         = COMM_TYPE::DOUBLE;
      break;
    case MPI_QUANTITIES::PRIMITIVE_GRADIENT:
    case MPI_QUANTITIES::PRIMITIVE_GRAD_REC:
      COUNT_PER_POINT  = nPrimVarGrad*nDim;
      MPI_TYPE         = COMM_TYPE::DOUBLE;
      break;
    case MPI_QUANTITIES::PRIMITIVE_LIMITER:
      COUNT_PER_POINT  = nPrimVarGrad;
      MPI_TYPE         = COMM_TYPE::DOUBLE;
      break;
    case MPI_QUANTITIES::SOLUTION_EDDY:
      COUNT_PER_POINT  = nVar+1;
      MPI_TYPE         = COMM_TYPE::DOUBLE;
      break;
    case MPI_QUANTITIES::STOCH_SOURCE_LANG:
      COUNT_PER_POINT  = nDim;
      MPI_TYPE         = COMM_TYPE::DOUBLE;
      break;
    case MPI_QUANTITIES::DES_LENGTHSCALE:
      COUNT_PER_POINT  = 1;
      MPI_TYPE         = COMM_TYPE::DOUBLE;
      break;
    case MPI_QUANTITIES::SOLUTION_FEA:
      if (config->GetTime_Domain())
        COUNT_PER_POINT  = nVar*3;
      else
        COUNT_PER_POINT  = nVar;
      MPI_TYPE         = COMM_TYPE::DOUBLE;
      break;
    case MPI_QUANTITIES::AUXVAR_GRADIENT:
      COUNT_PER_POINT  = nDim*base_nodes->GetnAuxVar();
      MPI_TYPE         = COMM_TYPE::DOUBLE;
      break;
    case MPI_QUANTITIES::MESH_DISPLACEMENTS:
      COUNT_PER_POINT  = nDim;
      MPI_TYPE         = COMM_TYPE::DOUBLE;
      break;
    case MPI_QUANTITIES::SOLUTION_TIME_N:
      COUNT_PER_POINT  = nVar;
      MPI_TYPE         = COMM_TYPE::DOUBLE;
      break;
    case MPI_QUANTITIES::SOLUTION_TIME_N1:
      COUNT_PER_POINT  = nVar;
      MPI_TYPE         = COMM_TYPE::DOUBLE;
      break;
    case MPI_QUANTITIES::MOM_COEFF:
      COUNT_PER_POINT  = 1;
      MPI_TYPE         = COMM_TYPE::DOUBLE;
      break;
    case MPI_QUANTITIES::MOM_CORRECTION:
      COUNT_PER_POINT  = nDim;
      MPI_TYPE         = COMM_TYPE::DOUBLE;
      break;
    case MPI_QUANTITIES::HBYA_CORRECTION:
      COUNT_PER_POINT  = nDim;
      MPI_TYPE         = COMM_TYPE::DOUBLE;
      break;
    case MPI_QUANTITIES::AUXVAR_ADAPT:
      COUNT_PER_POINT  = base_nodes->GetAuxVar_Adapt().cols();
      MPI_TYPE         = COMM_TYPE::DOUBLE;
      break;
    case MPI_QUANTITIES::GRADIENT_ADAPT:
      COUNT_PER_POINT  = base_nodes->GetAuxVar_Adapt().cols()*nDim;
      MPI_TYPE         = COMM_TYPE::DOUBLE;
      break;
    case MPI_QUANTITIES::HESSIAN:
      COUNT_PER_POINT  = base_nodes->GetHessian().rows()*base_nodes->GetHessian().cols();
      MPI_TYPE         = COMM_TYPE::DOUBLE;
      break;
    case MPI_QUANTITIES::METRIC:
      COUNT_PER_POINT  = 3*(nDim-1);
      MPI_TYPE         = COMM_TYPE::DOUBLE;
      break;
    default:
      SU2_MPI::Error("Unrecognized quantity for point-to-point MPI comms.",
                     CURRENT_FUNCTION);
      break;
  }
}

namespace CommHelpers {
  CVectorOfMatrix& selectGradient(CVariable* nodes, MPI_QUANTITIES commType) {
    switch(commType) {
      case MPI_QUANTITIES::SOLUTION_GRAD_REC: return nodes->GetGradient_Reconstruction();
      case MPI_QUANTITIES::PRIMITIVE_GRADIENT: return nodes->GetGradient_Primitive();
      case MPI_QUANTITIES::PRIMITIVE_GRAD_REC: return nodes->GetGradient_Reconstruction();
      case MPI_QUANTITIES::AUXVAR_GRADIENT: return nodes->GetAuxVarGradient();
      case MPI_QUANTITIES::GRADIENT_ADAPT: return nodes->GetGradient_Adapt();
      default: return nodes->GetGradient();
    }
  }

  su2activematrix& selectLimiter(CVariable* nodes, MPI_QUANTITIES commType) {
    if (commType == MPI_QUANTITIES::PRIMITIVE_LIMITER) return nodes->GetLimiter_Primitive();
    return nodes->GetLimiter();
  }
}

void CSolver::InitiateComms(CGeometry *geometry,
                            const CConfig *config,
                            MPI_QUANTITIES commType) {
  SU2_ZONE_SCOPED

  /*--- Local variables ---*/

  unsigned short iVar, iDim;
  unsigned short COUNT_PER_POINT = 0;
  COMM_TYPE MPI_TYPE{};

  unsigned long iPoint, msg_offset, buf_offset;

  int iMessage, iSend, nSend;

  /*--- Set the size of the data packet and type depending on quantity. ---*/

  GetCommCountAndType(config, commType, COUNT_PER_POINT, MPI_TYPE);

  /*--- Check to make sure we have created a large enough buffer
   for these comms during preprocessing. This is only for the su2double
   buffer. It will be reallocated whenever we find a larger count
   per point. After the first cycle of comms, this should be inactive. ---*/

  geometry->AllocateP2PComms(COUNT_PER_POINT);

  /*--- Set some local pointers to make access simpler. ---*/

  su2double *bufDSend = geometry->bufD_P2PSend;

  /*--- Handle the different types of gradient and limiter. ---*/

  const auto nVarGrad = COUNT_PER_POINT / nDim;
  auto& gradient = CommHelpers::selectGradient(base_nodes, commType);
  auto& limiter = CommHelpers::selectLimiter(base_nodes, commType);

  /*--- Load the specified quantity from the solver into the generic
   communication buffer in the geometry class. ---*/

  if (geometry->nP2PSend > 0) {

    /*--- Post all non-blocking recvs first before sends. ---*/

    geometry->PostP2PRecvs(geometry, config, MPI_TYPE, COUNT_PER_POINT, false);

    for (iMessage = 0; iMessage < geometry->nP2PSend; iMessage++) {

      /*--- Get the offset in the buffer for the start of this message. ---*/

      msg_offset = geometry->nPoint_P2PSend[iMessage];

      /*--- Total count can include multiple pieces of data per element. ---*/

      nSend = (geometry->nPoint_P2PSend[iMessage+1] -
               geometry->nPoint_P2PSend[iMessage]);

      SU2_OMP_FOR_STAT(OMP_MIN_SIZE)
      for (iSend = 0; iSend < nSend; iSend++) {

        /*--- Get the local index for this communicated data. ---*/

        iPoint = geometry->Local_Point_P2PSend[msg_offset + iSend];

        /*--- Compute the offset in the recv buffer for this point. ---*/

        buf_offset = (msg_offset + iSend)*COUNT_PER_POINT;

        switch (commType) {
          case MPI_QUANTITIES::SOLUTION:
            for (iVar = 0; iVar < nVar; iVar++)
              bufDSend[buf_offset+iVar] = base_nodes->GetSolution(iPoint, iVar);
            break;
          case MPI_QUANTITIES::SOLUTION_OLD:
            for (iVar = 0; iVar < nVar; iVar++)
              bufDSend[buf_offset+iVar] = base_nodes->GetSolution_Old(iPoint, iVar);
            break;
          case MPI_QUANTITIES::SOLUTION_EDDY:
            for (iVar = 0; iVar < nVar; iVar++)
              bufDSend[buf_offset+iVar] = base_nodes->GetSolution(iPoint, iVar);
            bufDSend[buf_offset+nVar]   = base_nodes->GetmuT(iPoint);
            break;
          case MPI_QUANTITIES::STOCH_SOURCE_LANG:
            for (iDim = 0; iDim < nDim; iDim++)
              bufDSend[buf_offset+iDim] = base_nodes->GetLangevinSourceTerms(iPoint, iDim);
            break;
          case MPI_QUANTITIES::DES_LENGTHSCALE:
            bufDSend[buf_offset] = base_nodes->GetDES_LengthScale(iPoint);
            break;
          case MPI_QUANTITIES::UNDIVIDED_LAPLACIAN:
            for (iVar = 0; iVar < nVar; iVar++)
              bufDSend[buf_offset+iVar] = base_nodes->GetUndivided_Laplacian(iPoint, iVar);
            break;
          case MPI_QUANTITIES::SOLUTION_LIMITER:
          case MPI_QUANTITIES::PRIMITIVE_LIMITER:
            for (iVar = 0; iVar < COUNT_PER_POINT; iVar++)
              bufDSend[buf_offset+iVar] = limiter(iPoint, iVar);
            break;
          case MPI_QUANTITIES::MAX_EIGENVALUE:
            bufDSend[buf_offset] = base_nodes->GetLambda(iPoint);
            break;
          case MPI_QUANTITIES::SENSOR:
            bufDSend[buf_offset] = base_nodes->GetSensor(iPoint);
            break;
          case MPI_QUANTITIES::SOLUTION_GRADIENT:
          case MPI_QUANTITIES::PRIMITIVE_GRADIENT:
          case MPI_QUANTITIES::SOLUTION_GRAD_REC:
          case MPI_QUANTITIES::PRIMITIVE_GRAD_REC:
          case MPI_QUANTITIES::AUXVAR_GRADIENT:
          case MPI_QUANTITIES::GRADIENT_ADAPT:
            for (iVar = 0; iVar < nVarGrad; iVar++)
              for (iDim = 0; iDim < nDim; iDim++)
                bufDSend[buf_offset+iVar*nDim+iDim] = gradient(iPoint, iVar, iDim);
            break;
          case MPI_QUANTITIES::AUXVAR_ADAPT:
            for (iVar = 0; iVar < COUNT_PER_POINT; iVar++)
              bufDSend[buf_offset+iVar] = base_nodes->GetAuxVar_Adapt(iPoint, iVar);
            break;
          case MPI_QUANTITIES::HESSIAN: {
            const auto& hessian = base_nodes->GetHessian();
            const auto nMet = hessian.cols();
            for (iVar = 0; iVar < hessian.rows(); iVar++)
              for (iDim = 0; iDim < nMet; iDim++)
                bufDSend[buf_offset+iVar*nMet+iDim] = hessian(iPoint, iVar, iDim);
            break;
          }
          case MPI_QUANTITIES::METRIC:
            for (iVar = 0; iVar < COUNT_PER_POINT; iVar++)
              bufDSend[buf_offset+iVar] = base_nodes->GetMetric(iPoint, iVar);
            break;
          case MPI_QUANTITIES::SOLUTION_FEA:
            for (iVar = 0; iVar < nVar; iVar++) {
              bufDSend[buf_offset+iVar] = base_nodes->GetSolution(iPoint, iVar);
              if (config->GetTime_Domain()) {
                bufDSend[buf_offset+nVar+iVar]   = base_nodes->GetSolution_Vel(iPoint, iVar);
                bufDSend[buf_offset+nVar*2+iVar] = base_nodes->GetSolution_Accel(iPoint, iVar);
              }
            }
            break;
          case MPI_QUANTITIES::MESH_DISPLACEMENTS:
            for (iDim = 0; iDim < nDim; iDim++)
              bufDSend[buf_offset+iDim] = base_nodes->GetBound_Disp(iPoint, iDim);
            break;
          case MPI_QUANTITIES::SOLUTION_TIME_N:
            for (iVar = 0; iVar < nVar; iVar++)
              bufDSend[buf_offset+iVar] = base_nodes->GetSolution_time_n(iPoint, iVar);
            break;
          case MPI_QUANTITIES::SOLUTION_TIME_N1:
            for (iVar = 0; iVar < nVar; iVar++)
              bufDSend[buf_offset+iVar] = base_nodes->GetSolution_time_n1(iPoint, iVar);
            break;
          case MPI_QUANTITIES::MOM_COEFF:
            bufDSend[buf_offset] = base_nodes->GetMomCoeff(iPoint);
            break;
          case MPI_QUANTITIES::MOM_CORRECTION:
            for (iDim = 0; iDim < nDim; iDim++)
              bufDSend[buf_offset+iDim] = base_nodes->GetMomentumCorrection(iPoint, iDim);
            break; 
          case MPI_QUANTITIES::HBYA_CORRECTION:
            for (iDim = 0; iDim < nDim; iDim++)
              bufDSend[buf_offset+iDim] = base_nodes->GetHbyACorrection(iPoint, iDim);
            break; 
          default:
            SU2_MPI::Error("Unrecognized quantity for point-to-point MPI comms.",
                           CURRENT_FUNCTION);
            break;
        }
      }
      END_SU2_OMP_FOR

      /*--- Launch the point-to-point MPI send for this message. ---*/

      geometry->PostP2PSends(geometry, config, MPI_TYPE, COUNT_PER_POINT, iMessage, false);

    }
  }

}

void CSolver::CompleteComms(CGeometry *geometry,
                            const CConfig *config,
                            MPI_QUANTITIES commType) {
  SU2_ZONE_SCOPED

  /*--- Local variables ---*/

  unsigned short iDim, iVar;
  unsigned long iPoint, iRecv, nRecv, msg_offset, buf_offset;
  unsigned short COUNT_PER_POINT = 0;
  COMM_TYPE MPI_TYPE{};

  int ind, source, iMessage, jRecv;

  /*--- Global status so all threads can see the result of Waitany. ---*/
  static SU2_MPI::Status status;

  /*--- Set the size of the data packet and type depending on quantity. ---*/

  GetCommCountAndType(config, commType, COUNT_PER_POINT, MPI_TYPE);

  /*--- Set some local pointers to make access simpler. ---*/

  const su2double *bufDRecv = geometry->bufD_P2PRecv;

  /*--- Handle the different types of gradient and limiter. ---*/

  const auto nVarGrad = COUNT_PER_POINT / nDim;
  auto& gradient = CommHelpers::selectGradient(base_nodes, commType);
  auto& limiter = CommHelpers::selectLimiter(base_nodes, commType);

  /*--- Store the data that was communicated into the appropriate
   location within the local class data structures. ---*/

  if (geometry->nP2PRecv > 0) {

    for (iMessage = 0; iMessage < geometry->nP2PRecv; iMessage++) {

      /*--- For efficiency, recv the messages dynamically based on
       the order they arrive. ---*/

      SU2_OMP_SAFE_GLOBAL_ACCESS(SU2_MPI::Waitany(geometry->nP2PRecv, geometry->req_P2PRecv, &ind, &status);)

      /*--- Once we have recv'd a message, get the source rank. ---*/

      source = status.MPI_SOURCE;

      /*--- We know the offsets based on the source rank. ---*/

      jRecv = geometry->P2PRecv2Neighbor[source];

      /*--- Get the offset in the buffer for the start of this message. ---*/

      msg_offset = geometry->nPoint_P2PRecv[jRecv];

      /*--- Get the number of packets to be received in this message. ---*/

      nRecv = (geometry->nPoint_P2PRecv[jRecv+1] -
               geometry->nPoint_P2PRecv[jRecv]);

      SU2_OMP_FOR_STAT(OMP_MIN_SIZE)
      for (iRecv = 0; iRecv < nRecv; iRecv++) {

        /*--- Get the local index for this communicated data. ---*/

        iPoint = geometry->Local_Point_P2PRecv[msg_offset + iRecv];

        /*--- Compute the offset in the recv buffer for this point. ---*/

        buf_offset = (msg_offset + iRecv)*COUNT_PER_POINT;

        /*--- Store the data correctly depending on the quantity. ---*/

        switch (commType) {
          case MPI_QUANTITIES::SOLUTION:
            for (iVar = 0; iVar < nVar; iVar++)
              base_nodes->SetSolution(iPoint, iVar, bufDRecv[buf_offset+iVar]);
            break;
          case MPI_QUANTITIES::SOLUTION_OLD:
            for (iVar = 0; iVar < nVar; iVar++)
              base_nodes->SetSolution_Old(iPoint, iVar, bufDRecv[buf_offset+iVar]);
            break;
          case MPI_QUANTITIES::SOLUTION_EDDY:
            for (iVar = 0; iVar < nVar; iVar++)
              base_nodes->SetSolution(iPoint, iVar, bufDRecv[buf_offset+iVar]);
            base_nodes->SetmuT(iPoint,bufDRecv[buf_offset+nVar]);
            break;
          case MPI_QUANTITIES::STOCH_SOURCE_LANG:
            for (iDim = 0; iDim < nDim; iDim++)
              base_nodes->SetLangevinSourceTerms(iPoint, iDim, bufDRecv[buf_offset+iDim]);
            break;
          case MPI_QUANTITIES::DES_LENGTHSCALE:
            base_nodes->SetDES_LengthScale(iPoint, bufDRecv[buf_offset]);
            break;
          case MPI_QUANTITIES::UNDIVIDED_LAPLACIAN:
            for (iVar = 0; iVar < nVar; iVar++)
              base_nodes->SetUnd_Lapl(iPoint, iVar, bufDRecv[buf_offset+iVar]);
            break;
          case MPI_QUANTITIES::SOLUTION_LIMITER:
          case MPI_QUANTITIES::PRIMITIVE_LIMITER:
            for (iVar = 0; iVar < COUNT_PER_POINT; iVar++)
              limiter(iPoint,iVar) = bufDRecv[buf_offset+iVar];
            break;
          case MPI_QUANTITIES::MAX_EIGENVALUE:
            base_nodes->SetLambda(iPoint,bufDRecv[buf_offset]);
            break;
          case MPI_QUANTITIES::SENSOR:
            base_nodes->SetSensor(iPoint,bufDRecv[buf_offset]);
            break;
          case MPI_QUANTITIES::SOLUTION_GRADIENT:
          case MPI_QUANTITIES::PRIMITIVE_GRADIENT:
          case MPI_QUANTITIES::SOLUTION_GRAD_REC:
          case MPI_QUANTITIES::PRIMITIVE_GRAD_REC:
          case MPI_QUANTITIES::AUXVAR_GRADIENT:
          case MPI_QUANTITIES::GRADIENT_ADAPT:
            for (iVar = 0; iVar < nVarGrad; iVar++)
              for (iDim = 0; iDim < nDim; iDim++)
                gradient(iPoint,iVar,iDim) = bufDRecv[buf_offset+iVar*nDim+iDim];
            break;
          case MPI_QUANTITIES::AUXVAR_ADAPT:
            for (iVar = 0; iVar < COUNT_PER_POINT; iVar++)
              base_nodes->SetAuxVar_Adapt(iPoint, iVar, bufDRecv[buf_offset+iVar]);
            break;
          case MPI_QUANTITIES::HESSIAN: {
            auto& hessian = base_nodes->GetHessian();
            const auto nMet = hessian.cols();
            for (iVar = 0; iVar < hessian.rows(); iVar++)
              for (iDim = 0; iDim < nMet; iDim++)
                hessian(iPoint, iVar, iDim) = bufDRecv[buf_offset+iVar*nMet+iDim];
            break;
          }
          case MPI_QUANTITIES::METRIC:
            for (iVar = 0; iVar < COUNT_PER_POINT; iVar++)
              base_nodes->SetMetric(iPoint, iVar, bufDRecv[buf_offset+iVar]);
            break;
          case MPI_QUANTITIES::SOLUTION_FEA:
            for (iVar = 0; iVar < nVar; iVar++) {
              base_nodes->SetSolution(iPoint, iVar, bufDRecv[buf_offset+iVar]);
              if (config->GetTime_Domain()) {
                base_nodes->SetSolution_Vel(iPoint, iVar, bufDRecv[buf_offset+nVar+iVar]);
                base_nodes->SetSolution_Accel(iPoint, iVar, bufDRecv[buf_offset+nVar*2+iVar]);
              }
            }
            break;
          case MPI_QUANTITIES::MESH_DISPLACEMENTS:
            for (iDim = 0; iDim < nDim; iDim++)
              base_nodes->SetBound_Disp(iPoint, iDim, bufDRecv[buf_offset+iDim]);
            break;
          case MPI_QUANTITIES::SOLUTION_TIME_N:
            for (iVar = 0; iVar < nVar; iVar++)
              base_nodes->Set_Solution_time_n(iPoint, iVar, bufDRecv[buf_offset+iVar]);
            break;
          case MPI_QUANTITIES::SOLUTION_TIME_N1:
            for (iVar = 0; iVar < nVar; iVar++)
              base_nodes->Set_Solution_time_n1(iPoint, iVar, bufDRecv[buf_offset+iVar]);
            break;
          case MPI_QUANTITIES::MOM_COEFF:
            base_nodes->SetMomCoeff(iPoint, bufDRecv[buf_offset]);
            break;
          case MPI_QUANTITIES::MOM_CORRECTION:
            for (iDim = 0; iDim < nDim; iDim++)
              base_nodes->SetMomentumCorrection(iPoint, iDim, bufDRecv[buf_offset+iDim]);
            break;
          case MPI_QUANTITIES::HBYA_CORRECTION:
            for (iDim = 0; iDim < nDim; iDim++)
              base_nodes->SetHbyACorrection(iPoint, iDim, bufDRecv[buf_offset+iDim]);
            break;
          default:
            SU2_MPI::Error("Unrecognized quantity for point-to-point MPI comms.",
                           CURRENT_FUNCTION);
            break;
        }
      }
      END_SU2_OMP_FOR
    }

    /*--- Verify that all non-blocking point-to-point sends have finished.
     Note that this should be satisfied, as we have received all of the
     data in the loop above at this point. ---*/

#ifdef HAVE_MPI
    SU2_OMP_SAFE_GLOBAL_ACCESS(SU2_MPI::Waitall(geometry->nP2PSend, geometry->req_P2PSend, MPI_STATUS_IGNORE);)
#endif
  }

}

void CSolver::ResetCFLAdapt() {
  SU2_ZONE_SCOPED
  NonLinRes_Series.clear();
  Old_Func = 0;
  New_Func = 0;
  NonLinRes_Counter = 0;
}


void CSolver::AdaptCFLNumber(CGeometry **geometry,
                             CSolver   ***solver_container,
                             CConfig   *config) {
  SU2_ZONE_SCOPED

  if (config->GetCFL_Adapt() != YES) return;

  /* Adapt the CFL number on all multigrid levels using an
   exponential progression with under-relaxation approach. */

  vector<su2double> MGFactor(config->GetnMGLevels()+1,1.0);
  const su2double CFLFactorDecrease = config->GetCFL_AdaptParam(0);
  const su2double CFLFactorIncrease = config->GetCFL_AdaptParam(1);
  const su2double CFLMin            = config->GetCFL_AdaptParam(2);
  const su2double CFLMax            = config->GetCFL_AdaptParam(3);
  const su2double acceptableLinTol  = config->GetCFL_AdaptParam(4);
  const su2double startingIter      = config->GetCFL_AdaptParam(5);
  const bool fullComms              = (config->GetComm_Level() == COMM_FULL);

  /* Number of iterations considered to check for stagnation. */
  const auto Res_Count = min(100ul, config->GetnInner_Iter()-1);

  static bool reduceCFL, resetCFL, canIncrease;

  for (unsigned short iMesh = 0; iMesh <= config->GetnMGLevels(); iMesh++) {

    /* Store the mean flow, and turbulence solvers more clearly. */

    CSolver *solverFlow = solver_container[iMesh][FLOW_SOL];
    CSolver *solverTurb = solver_container[iMesh][TURB_SOL];
    CSolver *solverSpecies = solver_container[iMesh][SPECIES_SOL];

    /* Compute the reduction factor for CFLs on the coarse levels. */

    if (iMesh == MESH_0) {
      MGFactor[iMesh] = 1.0;
    } else {
      const su2double CFLRatio = config->GetCFL(iMesh)/config->GetCFL(iMesh-1);
      MGFactor[iMesh] = MGFactor[iMesh-1]*CFLRatio;
    }

    /* Check whether we achieved the requested reduction in the linear
     solver residual within the specified number of linear iterations. */

    su2double linResTurb = 0.0;
    su2double linResSpecies = 0.0;
    if ((iMesh == MESH_0) && solverTurb) linResTurb = solverTurb->GetResLinSolver();
    if ((iMesh == MESH_0) && solverSpecies) linResSpecies = solverSpecies->GetResLinSolver();

    /* Max linear residual between flow and turbulence/species transport. */
    const su2double linRes = max(solverFlow->GetResLinSolver(), max(linResTurb, linResSpecies));

    /* Tolerance limited to an acceptable value. */
    const su2double linTol = max(acceptableLinTol, config->GetLinear_Solver_Error());

    /* Check that we are meeting our nonlinear residual reduction target
     over time so that we do not get stuck in limit cycles, this is done
     on the fine grid and applied to all others. */

    BEGIN_SU2_OMP_SAFE_GLOBAL_ACCESS
    { /* Only the master thread updates the shared variables. */

    /* Check if we should decrease or if we can increase, the 20% is to avoid flip-flopping. */
    resetCFL = linRes > 0.99;
    unsigned long iter = config->GetMultizone_Problem() ? config->GetOuterIter() : config->GetInnerIter();

    /* only change CFL number when larger than starting iteration */
    reduceCFL = (linRes > 1.2*linTol) && (iter >= startingIter);

    canIncrease = (linRes < linTol) && (iter >= startingIter);

    /* Do not use the residual flip-flop criteria when we are mitigating outliers
     * because the former was never very reliable for large cases where monotonic
     * residual reduction is impossible to achieve. */
    if (!config->OptionIsSet("OUTLIER_MITIGATION_PARAM") && iMesh == MESH_0 && Res_Count > 0) {
      Old_Func = New_Func;
      if (NonLinRes_Series.empty()) NonLinRes_Series.resize(Res_Count,0.0);

      /* Sum the RMS residuals for all equations. */

      New_Func = 0.0;
      unsigned short totalVars = 0;
      for (unsigned short iVar = 0; iVar < solverFlow->GetnVar(); iVar++) {
        New_Func += log10(solverFlow->GetRes_RMS(iVar));
        ++totalVars;
      }
      if ((iMesh == MESH_0) && solverTurb) {
        for (unsigned short iVar = 0; iVar < solverTurb->GetnVar(); iVar++) {
          New_Func += log10(solverTurb->GetRes_RMS(iVar));
          ++totalVars;
        }
      }
      if ((iMesh == MESH_0) && solverSpecies) {
        for (unsigned short iVar = 0; iVar < solverSpecies->GetnVar(); iVar++) {
          New_Func += log10(solverSpecies->GetRes_RMS(iVar));
          ++totalVars;
        }
      }
      New_Func /= totalVars;

      /* Compute the difference in the nonlinear residuals between the
       current and previous iterations, taking care with very low initial
       residuals (due to initialization). */

      if ((config->GetInnerIter() == 1) && (New_Func - Old_Func > 10)) {
        Old_Func = New_Func;
      }
      NonLinRes_Series[NonLinRes_Counter] = New_Func - Old_Func;

      /* Increment the counter, if we hit the max size, then start over. */

      NonLinRes_Counter++;
      if (NonLinRes_Counter == Res_Count) NonLinRes_Counter = 0;

      /* Detect flip-flop convergence to reduce CFL and large increases
       to reset to minimum value, in that case clear the history. */

      if (config->GetInnerIter() >= Res_Count) {
        unsigned long signChanges = 0;
        su2double totalChange = 0.0;
        auto prev = NonLinRes_Series.front();
        for (const auto& val : NonLinRes_Series) {
          totalChange += val;
          signChanges += (prev > 0) ^ (val > 0);
          prev = val;
        }
        reduceCFL |= (signChanges > Res_Count/4) && (totalChange > -0.5);

        if (totalChange > 2.0) { // orders of magnitude
          resetCFL = true;
          NonLinRes_Counter = 0;
          for (auto& val : NonLinRes_Series) val = 0.0;
        }
      }
    }
    } /* End safe global access, now all threads update the CFL number. */
    END_SU2_OMP_SAFE_GLOBAL_ACCESS

    /* Loop over all points on this grid and apply CFL adaption. */

    su2double myCFLMin = 1e30, myCFLMax = 0.0, myCFLSum = 0.0;
    const su2double CFLTurbReduction = config->GetCFLRedCoeff_Turb();
    const su2double CFLSpeciesReduction = config->GetCFLRedCoeff_Species();

    SU2_OMP_MASTER
    if ((iMesh == MESH_0) && fullComms) {
      Min_CFL_Local = 1e30;
      Max_CFL_Local = 0.0;
      Avg_CFL_Local = 0.0;
    }
    END_SU2_OMP_MASTER

    SU2_OMP_FOR_STAT(roundUpDiv(geometry[iMesh]->GetnPointDomain(),omp_get_max_threads()))
    for (unsigned long iPoint = 0; iPoint < geometry[iMesh]->GetnPointDomain(); iPoint++) {

      /* Get the current local flow CFL number at this point. */

      su2double CFL = solverFlow->GetNodes()->GetLocalCFL(iPoint);

      /* Get the current under-relaxation parameters that were computed
       during the previous nonlinear update. If we have a turbulence model,
       take the minimum under-relaxation parameter between the mean flow
       and turbulence systems. */

      su2double underRelaxationFlow = solverFlow->GetNodes()->GetUnderRelaxation(iPoint);
      su2double underRelaxationTurb = 1.0;
      if ((iMesh == MESH_0) && solverTurb)
        underRelaxationTurb = solverTurb->GetNodes()->GetUnderRelaxation(iPoint);
      const su2double underRelaxation = min(underRelaxationFlow,underRelaxationTurb);

      /* If we apply a small under-relaxation parameter for stability,
       then we should reduce the CFL before the next iteration. If we
       are able to add the entire nonlinear update (under-relaxation = 1)
       then we schedule an increase the CFL number for the next iteration. */

      su2double CFLFactor = 1.0;
      if (underRelaxation < 0.1 || reduceCFL) {
        CFLFactor = CFLFactorDecrease;
      } else if ((underRelaxation >= 0.1 && underRelaxation < 1.0) || !canIncrease) {
        CFLFactor = 1.0;
      } else {
        CFLFactor = CFLFactorIncrease;
      }

      /* Check if we are hitting the min or max and adjust. */

      if (CFL*CFLFactor <= CFLMin) {
        CFL       = CFLMin;
        CFLFactor = MGFactor[iMesh];
      } else if (CFL*CFLFactor >= CFLMax) {
        CFL       = CFLMax;
        CFLFactor = MGFactor[iMesh];
      }

      /* If we detect a stalled nonlinear residual, then force the CFL
       for all points to the minimum temporarily to restart the ramp. */

      if (resetCFL) {
        CFL       = CFLMin;
        CFLFactor = MGFactor[iMesh];
      }

      /* Apply the adjustment to the CFL and store local values. */

      CFL *= CFLFactor;
      solverFlow->GetNodes()->SetLocalCFL(iPoint, CFL);
      if ((iMesh == MESH_0) && solverTurb) {
        solverTurb->GetNodes()->SetLocalCFL(iPoint, CFL * CFLTurbReduction);
      }
      if ((iMesh == MESH_0) && solverSpecies) {
        solverSpecies->GetNodes()->SetLocalCFL(iPoint, CFL * CFLSpeciesReduction);
      }

      /* Store min and max CFL for reporting on the fine grid. */

      if ((iMesh == MESH_0) && fullComms) {
        myCFLMin = min(CFL,myCFLMin);
        myCFLMax = max(CFL,myCFLMax);
        myCFLSum += CFL;
      }

    }
    END_SU2_OMP_FOR

    /* Reduce the min/max/avg local CFL numbers. */

    if ((iMesh == MESH_0) && fullComms) {
      atomicMin(myCFLMin, Min_CFL_Local);
      atomicMax(myCFLMax, Max_CFL_Local);
      atomicAdd(myCFLSum, Avg_CFL_Local);

      BEGIN_SU2_OMP_SAFE_GLOBAL_ACCESS
      { /* MPI reduction. */
        myCFLMin = Min_CFL_Local; myCFLMax = Max_CFL_Local; myCFLSum = Avg_CFL_Local;
        SU2_MPI::Allreduce(&myCFLMin, &Min_CFL_Local, 1, MPI_DOUBLE, MPI_MIN, SU2_MPI::GetComm());
        SU2_MPI::Allreduce(&myCFLMax, &Max_CFL_Local, 1, MPI_DOUBLE, MPI_MAX, SU2_MPI::GetComm());
        SU2_MPI::Allreduce(&myCFLSum, &Avg_CFL_Local, 1, MPI_DOUBLE, MPI_SUM, SU2_MPI::GetComm());
        Avg_CFL_Local /= su2double(geometry[iMesh]->GetGlobal_nPointDomain());
      }
      END_SU2_OMP_SAFE_GLOBAL_ACCESS
    }

  }

}

void CSolver::SetResidual_RMS(const CGeometry *geometry, const CConfig *config, bool force) {
  SU2_ZONE_SCOPED

  /*--- On coarse levels the reduction is skipped for performance, unless MG_Smooth_EarlyExit
   *    needs it or the caller asks for it. ---*/
  if (!force && geometry->GetMGLevel() != MESH_0 && !config->GetMGOptions().MG_Smooth_EarlyExit) return;

  BEGIN_SU2_OMP_SAFE_GLOBAL_ACCESS {

  /*--- Set the L2 Norm residual in all the processors. ---*/

  vector<su2double> rbuf_res(nVar * nDim);
  unsigned long Global_nPointDomain = 0;

  if (config->GetComm_Level() == COMM_FULL) {

    SU2_MPI::Allreduce(Residual_RMS.data(), rbuf_res.data(), nVar, MPI_DOUBLE, MPI_SUM, SU2_MPI::GetComm());
    Global_nPointDomain = geometry->GetGlobal_nPointDomain();
  }
  else {
    /*--- Reduced MPI comms have been requested. Use a local residual only. ---*/

    for (unsigned short iVar = 0; iVar < nVar; iVar++) rbuf_res[iVar] = Residual_RMS[iVar];
    Global_nPointDomain = geometry->GetnPointDomain();
  }

  for (unsigned short iVar = 0; iVar < nVar; iVar++) {

    if (std::isnan(SU2_TYPE::GetValue(rbuf_res[iVar]))) {
      SU2_MPI::Error("SU2 has diverged (NaN detected).", CURRENT_FUNCTION);
    }

    Residual_RMS[iVar] = max(EPS*EPS, sqrt(rbuf_res[iVar]/Global_nPointDomain));

    if (log10(GetRes_RMS(iVar)) > 20.0) {
      SU2_MPI::Error("SU2 has diverged (Residual > 10^20 detected).", CURRENT_FUNCTION);
    }
  }

  /*--- Set the Maximum residual in all the processors. ---*/

  if (config->GetComm_Level() == COMM_FULL) {
    SU2_MPI::Allreduce(Residual_Max.data(), rbuf_res.data(), nVar, MPI_DOUBLE, MPI_MAX, SU2_MPI::GetComm());
    for (unsigned short iVar = 0; iVar < nVar; iVar++) {
      if (Residual_Max[iVar] < rbuf_res[iVar]) {
        Point_Max[iVar] = 0;
        for (unsigned short iDim = 0; iDim < nDim; iDim++) {
          Point_Max_Coord(iVar, iDim) = std::numeric_limits<su2double>::lowest();
        }
      }
      Residual_Max[iVar] = rbuf_res[iVar];
    }
    vector<unsigned long> rbuf_point(nVar);
    SU2_MPI::Allreduce(Point_Max.data(), rbuf_point.data(), nVar, MPI_UNSIGNED_LONG, MPI_MAX, SU2_MPI::GetComm());
    SU2_MPI::Allreduce(Point_Max_Coord.data(), rbuf_res.data(), nVar*nDim, MPI_DOUBLE, MPI_MAX, SU2_MPI::GetComm());
    Point_Max = std::move(rbuf_point);
    for (unsigned short iVar = 0; iVar < nVar * nDim; iVar++) {
      Point_Max_Coord.data()[iVar] = rbuf_res[iVar];
    }
  }

  /*--- Floor the maximum residual like the RMS residual above, so an
   *    exactly-zero residual (e.g. an inert species at a cold start)
   *    cannot become a nonfinite log10 in screen or history output. ---*/

  for (auto& residual : Residual_Max) residual = max(EPS*EPS, residual);

  }
  END_SU2_OMP_SAFE_GLOBAL_ACCESS
}

void CSolver::SetResidual_BGS(const CGeometry *geometry, const CConfig *config) {
  SU2_ZONE_SCOPED

  if (geometry->GetMGLevel() != MESH_0) return;

  BEGIN_SU2_OMP_SAFE_GLOBAL_ACCESS {

  /*--- Set the L2 Norm residual in all the processors. ---*/

  vector<su2double> rbuf_res(nVar);

  SU2_MPI::Allreduce(Residual_BGS.data(), rbuf_res.data(), nVar, MPI_DOUBLE, MPI_SUM, SU2_MPI::GetComm());
  const auto Global_nPointDomain = geometry->GetGlobal_nPointDomain();

  for (unsigned short iVar = 0; iVar < nVar; iVar++) {
    Residual_BGS[iVar] = max(EPS*EPS, sqrt(rbuf_res[iVar]/Global_nPointDomain));
  }

  if (config->GetComm_Level() == COMM_FULL) {

    /*--- Set the Maximum residual in all the processors. ---*/

    const unsigned long nProcessor = size;

    su2activematrix rbuf_residual(nProcessor,nVar);
    su2matrix<unsigned long> rbuf_point(nProcessor,nVar);
    su2activematrix rbuf_coord(nProcessor*nVar, nDim);

    SU2_MPI::Allgather(Residual_Max_BGS.data(), nVar, MPI_DOUBLE, rbuf_residual.data(), nVar, MPI_DOUBLE, SU2_MPI::GetComm());
    SU2_MPI::Allgather(Point_Max_BGS.data(), nVar, MPI_UNSIGNED_LONG, rbuf_point.data(), nVar, MPI_UNSIGNED_LONG, SU2_MPI::GetComm());
    SU2_MPI::Allgather(Point_Max_Coord_BGS.data(), nVar*nDim, MPI_DOUBLE, rbuf_coord.data(), nVar*nDim, MPI_DOUBLE, SU2_MPI::GetComm());

    for (unsigned short iVar = 0; iVar < nVar; iVar++) {
      for (auto iProcessor = 0ul; iProcessor < nProcessor; iProcessor++) {
        AddRes_Max_BGS(iVar, rbuf_residual(iProcessor,iVar), rbuf_point(iProcessor,iVar), rbuf_coord[iProcessor*nVar+iVar]);
      }
    }
  }

  }
  END_SU2_OMP_SAFE_GLOBAL_ACCESS
}

void CSolver::SetRotatingFrame_GCL(CGeometry *geometry, const CConfig *config) {
  SU2_ZONE_SCOPED

  /*--- Loop interior points ---*/

  SU2_OMP_FOR_STAT(roundUpDiv(nPointDomain,2*omp_get_max_threads()))
  for (auto iPoint = 0ul; iPoint < nPointDomain; ++iPoint) {

    const su2double* GridVel_i = geometry->nodes->GetGridVel(iPoint);
    const su2double* Solution_i = base_nodes->GetSolution(iPoint);

    for (auto iNeigh = 0u; iNeigh < geometry->nodes->GetnPoint(iPoint); iNeigh++) {

      const auto iEdge = geometry->nodes->GetEdge(iPoint, iNeigh);
      const su2double* Normal = geometry->edges->GetNormal(iEdge);

      const auto jPoint = geometry->nodes->GetPoint(iPoint, iNeigh);
      const su2double* GridVel_j = geometry->nodes->GetGridVel(jPoint);

      /*--- Determine whether to consider the normal outward or inward. ---*/
      su2double dir = (iPoint < jPoint)? 0.5 : -0.5;

      su2double Flux = 0.0;
      for (auto iDim = 0u; iDim < nDim; iDim++)
        Flux += dir*(GridVel_i[iDim]+GridVel_j[iDim])*Normal[iDim];

      for (auto iVar = 0u; iVar < nVar; iVar++)
        LinSysRes(iPoint,iVar) += Flux * Solution_i[iVar];
    }
  }
  END_SU2_OMP_FOR

  /*--- Loop boundary edges ---*/

  for (auto iMarker = 0u; iMarker < geometry->GetnMarker(); iMarker++) {
    if ((config->GetMarker_All_KindBC(iMarker) != INTERNAL_BOUNDARY) &&
        (config->GetMarker_All_KindBC(iMarker) != NEARFIELD_BOUNDARY) &&
        (config->GetMarker_All_KindBC(iMarker) != PERIODIC_BOUNDARY)) {

      SU2_OMP_FOR_STAT(OMP_MIN_SIZE)
      for (auto iVertex = 0u; iVertex < geometry->GetnVertex(iMarker); iVertex++) {

        const auto iPoint = geometry->vertex[iMarker][iVertex]->GetNode();

        /*--- Grid Velocity at each edge point ---*/

        const su2double* GridVel = geometry->nodes->GetGridVel(iPoint);

        /*--- Summed normal components ---*/

        const su2double* Normal = geometry->vertex[iMarker][iVertex]->GetNormal();

        su2double Flux = GeometryToolbox::DotProduct(nDim, Normal, GridVel);

        for (auto iVar = 0u; iVar < nVar; iVar++)
          LinSysRes(iPoint,iVar) -= Flux * base_nodes->GetSolution(iPoint,iVar);
      }
      END_SU2_OMP_FOR
    }
  }

}

void CSolver::SetAuxVar_Gradient_GG(CGeometry *geometry, const CConfig *config) {
  SU2_ZONE_SCOPED

  const auto& solution = base_nodes->GetAuxVar();
  auto& gradient = base_nodes->GetAuxVarGradient();

  computeGradientsGreenGauss(this, MPI_QUANTITIES::AUXVAR_GRADIENT, PERIODIC_NONE, *geometry,
                             *config, solution, 0, base_nodes->GetnAuxVar(), -1, gradient);
}

void CSolver::SetAuxVar_Gradient_LS(CGeometry *geometry, const CConfig *config) {
  SU2_ZONE_SCOPED

  bool weighted = true;
  const auto& solution = base_nodes->GetAuxVar();
  auto& gradient = base_nodes->GetAuxVarGradient();
  auto& rmatrix  = base_nodes->GetRmatrix();

  computeGradientsLeastSquares(this, MPI_QUANTITIES::AUXVAR_GRADIENT, PERIODIC_NONE, *geometry, *config,
                               weighted, solution, 0, base_nodes->GetnAuxVar(), -1, gradient, rmatrix);
}

void CSolver::SetHessian_Adapt(CGeometry *geometry, const CConfig *config) {
  SU2_ZONE_SCOPED
  const auto hessianStart = SU2_MPI::Wtime();

  const auto requestedMethod = static_cast<ENUM_FLOW_GRADIENT>(config->GetKind_Hessian_Method());
  const auto method = requestedMethod == QUADRATIC_LEAST_SQUARES ? WEIGHTED_LEAST_SQUARES : requestedMethod;
  const auto nSensor = config->GetnAdap_Sensor();
  const bool simplexRecovery = nSensor != 0 && config->GetAdap_Sensor(0) != "GOAL";
  const auto& sensor = base_nodes->GetAuxVar_Adapt();
  auto& gradient = base_nodes->GetGradient_Adapt();
  auto& hessian = base_nodes->GetHessian();

  /*--- The Green-Gauss implementation uses static arrays of this size. ---*/
  if (nSensor > 20) SU2_MPI::Error("Too many adaptation sensors.", CURRENT_FUNCTION);

  /*--- Sensor values on halo points. ---*/

  InitiateComms(geometry, config, MPI_QUANTITIES::AUXVAR_ADAPT);
  CompleteComms(geometry, config, MPI_QUANTITIES::AUXVAR_ADAPT);

  /*--- Gradients of the sensors (scalars), with periodic contributions. Halo values are
   *    communicated, they are needed to differentiate the gradients. The normal gradient is removed
   *    on symmetry planes only, not on Euler walls (see computeHessians). ---*/

  switch (method) {
    case GREEN_GAUSS:
      computeGradientsGreenGauss(this, MPI_QUANTITIES::GRADIENT_ADAPT, PERIODIC_ADAPT_GG, *geometry, *config,
                                 sensor, 0, nSensor, -1, gradient, false, true, simplexRecovery);
      break;
    case WEIGHTED_LEAST_SQUARES:
      computeGradientsLeastSquares(this, MPI_QUANTITIES::GRADIENT_ADAPT, PERIODIC_ADAPT_LS, *geometry, *config,
                                   true, sensor, 0, nSensor, -1, gradient, base_nodes->GetRmatrix(), false);
      break;
    default:
      SU2_MPI::Error("Unsupported NUM_METHOD_HESS.", CURRENT_FUNCTION);
      break;
  }

  /*--- Hessians, as gradients of the gradients. The work arrays of the variables are used
   *    because periodic communications access them. ---*/

  computeHessians(this, method, *geometry, *config, gradient, 0, nSensor, base_nodes->GetHessian_Field(),
                  base_nodes->GetHessian_Grad(), base_nodes->GetRmatrix(), hessian, true, simplexRecovery);

  if (requestedMethod == QUADRATIC_LEAST_SQUARES) {
    computeHessiansQuadratic(*geometry, nSensor, sensor, gradient, hessian,
                             SU2_TYPE::GetValue(config->GetAdap_Hessian_Noise()));
    if (config->GetnMarker_SymWall() != 0) {
      if (nDim == 2) {
        correctGradientsSymmetry<2>(*geometry, *config, 0, nSensor, -1, gradient, false);
        correctHessiansSymmetry<2>(*geometry, *config, 0, nSensor, base_nodes->GetHessian_Grad(), hessian);
      } else {
        correctGradientsSymmetry<3>(*geometry, *config, 0, nSensor, -1, gradient, false);
        correctHessiansSymmetry<3>(*geometry, *config, 0, nSensor, base_nodes->GetHessian_Grad(), hessian);
      }
    }
    InitiateComms(geometry, config, MPI_QUANTITIES::GRADIENT_ADAPT);
    CompleteComms(geometry, config, MPI_QUANTITIES::GRADIENT_ADAPT);
  } else if (method == WEIGHTED_LEAST_SQUARES) {
    /*--- One geometric stencil for all sensors and both derivative passes; count owners only.
     *    A finite Hessian can still be unreliable when these normal equations lose numerical rank. ---*/
    const auto eps = std::numeric_limits<passivedouble>::epsilon();
    unsigned long local[2] = {}, global[2] = {};
    passivedouble localWorst = 1.0, worst = 1.0;
    for (auto point = 0ul; point < nPointDomain; ++point) {
      const auto rcond = detail::hessianStencilReciprocalCondition(nDim, point, base_nodes->GetRmatrix());
      localWorst = std::min(localWorst, rcond);
      if (rcond <= 64 * eps) ++local[0];
      else if (rcond < sqrt(eps)) ++local[1];
    }
    CPassiveComm::Allreduce(local, global, 2, CPassiveComm::Op::SUM);
    CPassiveComm::Allreduce(&localWorst, &worst, 1, CPassiveComm::Op::MIN);
    if (rank == MASTER_NODE) {
      cout << "Hessian WLS stencil statistics: " << global[0] << " numerically deficient, " << global[1]
           << " poorly conditioned owned points; minimum normalized reciprocal condition " << worst << "." << endl;
      if (global[0] + global[1] > 0)
        cout << "WARNING: Weighted least-squares Hessians may be inaccurate even when finite. "
             << "Check directional coverage of the reconstruction stencils." << endl;
    }
  }

  /*--- Same Hessians on both sides of periodic boundaries, then on halo points. ---*/

  for (unsigned short iPeriodic = 1; iPeriodic <= config->GetnMarker_Periodic() / 2; ++iPeriodic) {
    InitiatePeriodicComms(geometry, config, iPeriodic, PERIODIC_HESSIAN);
    CompletePeriodicComms(geometry, config, iPeriodic, PERIODIC_HESSIAN);
  }

  InitiateComms(geometry, config, MPI_QUANTITIES::HESSIAN);
  CompleteComms(geometry, config, MPI_QUANTITIES::HESSIAN);
  const passivedouble hessianLocal = SU2_MPI::Wtime() - hessianStart;
  passivedouble hessianElapsed = 0;
  CPassiveComm::Allreduce(&hessianLocal, &hessianElapsed, 1, CPassiveComm::Op::MAX);
  if (rank == MASTER_NODE) cout << "Hessian recovery: " << hessianElapsed << " s (maximum rank time)." << endl;

}

void CSolver::IntersectMetrics(unsigned short nDim, const su2double (&A)[3][3], const su2double (&B)[3][3],
                               su2double (&C)[3][3]) {

  auto matMul = [nDim](const su2double (&X)[3][3], const su2double (&Y)[3][3], su2double (&Z)[3][3]) {
    for (auto i = 0u; i < nDim; ++i) {
      for (auto j = 0u; j < nDim; ++j) {
        Z[i][j] = 0.0;
        for (auto k = 0u; k < nDim; ++k) Z[i][j] += X[i][k] * Y[k][j];
      }
    }
  };

  /*--- Use S=A+B as the whitening frame. In that frame A has eigenvalues lambda in [0,1]
   *    and B=I-A, so the same intersection has eigenvalues max(lambda,1-lambda).
   *    This avoids whitening by a nearly rank-one sensor when the other sensor supplies its missing direction.
   *    Passive common scaling cancels algebraically and protects the matrix products from physical-unit extremes. ---*/
  passivedouble scale = 0.0;
  for (auto i = 0u; i < nDim; ++i)
    for (auto j = 0u; j < nDim; ++j)
      scale = max(scale, max(fabs(SU2_TYPE::GetValue(A[i][j])), fabs(SU2_TYPE::GetValue(B[i][j]))));
  if (!(scale > 0.0) || !std::isfinite(scale))
    SU2_MPI::Error("Metric intersection requires finite SPD tensors.", CURRENT_FUNCTION);

  su2double sum[3][3], normalizedA[3][3];
  for (auto i = 0u; i < nDim; ++i)
    for (auto j = 0u; j < nDim; ++j) {
      normalizedA[i][j] = A[i][j] / scale;
      sum[i][j] = normalizedA[i][j] + B[i][j] / scale;
    }
  su2double vec[3][3], val[3], work[3], sqrtVal[3], invSqrtVal[3];
  su2double sqrtSum[3][3], invSqrtSum[3][3], T[3][3], tmp[3][3], result[3][3];
  CBlasStructure::EigenDecomposition(sum, vec, val, nDim, work);
  for (auto i = 0u; i < nDim; ++i) {
    if (!(SU2_TYPE::GetValue(val[i]) > 0.0))
      SU2_MPI::Error("Metric intersection sum is numerically singular; check tensor conditioning.", CURRENT_FUNCTION);
    sqrtVal[i] = sqrt(val[i]);
    invSqrtVal[i] = 1.0 / sqrtVal[i];
  }
  CBlasStructure::EigenRecomposition(sqrtSum, vec, sqrtVal, nDim);
  CBlasStructure::EigenRecomposition(invSqrtSum, vec, invSqrtVal, nDim);

  matMul(invSqrtSum, normalizedA, tmp);
  matMul(tmp, invSqrtSum, T);
  CBlasStructure::EigenDecomposition(T, vec, val, nDim, work);
  for (auto i = 0u; i < nDim; ++i) val[i] = fmax(val[i], 1.0 - val[i]);
  CBlasStructure::EigenRecomposition(T, vec, val, nDim);
  matMul(sqrtSum, T, tmp);
  matMul(tmp, sqrtSum, result);
  for (auto i = 0u; i < nDim; ++i)
    for (auto j = 0u; j < nDim; ++j) C[i][j] = scale * (0.5 * result[i][j] + 0.5 * result[j][i]);

}

vector<unsigned long> CSolver::FindSharpWallPoints(const CGeometry* geometry, const CConfig* config) {
  const auto nDim = geometry->GetnDim();
  const su2double cosAngle = cos(config->GetAdap_Angle() * PI_NUMBER / 180.0);

  /*--- Outward unit normal of each wall face (pointing away from its volume element, so that the orientation of
   *    the boundary elements does not matter), for each of its points. ---*/

  using PointNormal = std::pair<unsigned long, std::array<su2double, 3>>;
  vector<PointNormal> normals;

  for (auto iMarker = 0u; iMarker < geometry->GetnMarker(); ++iMarker) {
    if (!config->GetSolid_Wall(iMarker)) continue;

    for (auto iElem = 0ul; iElem < geometry->GetnElem_Bound(iMarker); ++iElem) {
      const auto* face = geometry->bound[iMarker][iElem];
      const auto* elem = geometry->elem[face->GetDomainElement()];
      const auto nNode = face->GetnNodes();

      su2double x[4][3] = {{0.0}}, faceCenter[3] = {0.0}, elemCenter[3] = {0.0}, normal[3] = {0.0};
      for (auto iNode = 0u; iNode < nNode; ++iNode) {
        for (auto iDim = 0u; iDim < nDim; ++iDim) {
          x[iNode][iDim] = geometry->nodes->GetCoord(face->GetNode(iNode), iDim);
          faceCenter[iDim] += x[iNode][iDim] / nNode;
        }
      }
      for (auto iNode = 0u; iNode < elem->GetnNodes(); ++iNode)
        for (auto iDim = 0u; iDim < nDim; ++iDim)
          elemCenter[iDim] += geometry->nodes->GetCoord(elem->GetNode(iNode), iDim) / elem->GetnNodes();

      if (nDim == 2) {
        normal[0] = x[1][1] - x[0][1];
        normal[1] = x[0][0] - x[1][0];
      } else {
        /*--- Triangle: cross product of two sides; quadrilateral: of the diagonals. ---*/
        su2double a[3], b[3];
        const auto last = nNode - 1;
        GeometryToolbox::Distance(3, x[nNode == 3 ? 1 : 2], x[0], a);
        GeometryToolbox::Distance(3, x[last], x[nNode == 3 ? 0 : 1], b);
        GeometryToolbox::CrossProduct(a, b, normal);
      }
      su2double outward[3] = {0.0};
      GeometryToolbox::Distance(nDim, faceCenter, elemCenter, outward);
      const su2double norm = GeometryToolbox::Norm(nDim, normal);
      if (norm == 0.0) continue;
      const su2double sign = (GeometryToolbox::DotProduct(nDim, normal, outward) < 0.0) ? -1.0 : 1.0;

      std::array<su2double, 3> unit = {0.0, 0.0, 0.0};
      for (auto iDim = 0u; iDim < nDim; ++iDim) unit[iDim] = sign * normal[iDim] / norm;
      for (auto iNode = 0u; iNode < nNode; ++iNode) normals.emplace_back(face->GetNode(iNode), unit);
    }
  }

  /*--- A domain point is sharp if the normals of two of its wall faces differ by more than ADAP_ANGLE. ---*/

  std::sort(normals.begin(), normals.end(),
            [](const PointNormal& a, const PointNormal& b) { return a.first < b.first; });

  vector<unsigned long> sharp;
  for (auto begin = normals.begin(); begin != normals.end();) {
    auto end = begin;
    while (end != normals.end() && end->first == begin->first) ++end;

    bool isSharp = false;
    for (auto a = begin; a != end && !isSharp; ++a)
      for (auto b = a + 1; b != end && !isSharp; ++b)
        isSharp = GeometryToolbox::DotProduct(nDim, a->second.data(), b->second.data()) < cosAngle;

    if (isSharp && geometry->nodes->GetDomain(begin->first)) sharp.push_back(begin->first);
    begin = end;
  }
  return sharp;
}

void CSolver::ComputeMetric(CGeometry *geometry, const CConfig *config, const vector<su2double>* givenMetric,
                            bool boundaryLayer, const SU2NativeBoundary2D::ReferenceState* nativeReference) {
  SU2_ZONE_SCOPED

  const auto nSensor = config->GetnAdap_Sensor();
  const su2double p = config->GetAdap_Norm();
  const su2double eigMax = 1.0 / pow(config->GetAdap_Hmin(), 2);
  const su2double eigMin = 1.0 / pow(config->GetAdap_Hmax(), 2);
  const su2double arMax2 = pow(config->GetAdap_ARmax(), 2);
  const su2double complexity = config->GetAdap_Complexity();

  const bool nativeMetric = config->GetKind_Adap_Remesher() == ADAP_REMESHER::NATIVE_CAVITY;
  const bool geometricBL = boundaryLayer && config->GetnAdap_BL() > 0 &&
                            config->GetKind_Adap_Remesher() == ADAP_REMESHER::NATIVE_CAVITY;

  /*--- Normalize each sensor before decomposition and determinant powers. The Lp metric is homogeneous
   *    in H, so this preserves its shape while removing dependence on sensor units and avoiding overflow.
   *    Passive scaling is only a numerical preconditioner; it cancels analytically in the normalized metric. ---*/
  vector<passivedouble> localHessianScale(nSensor, 0.0), hessianScale(nSensor, 0.0);
  if (givenMetric == nullptr) {
    for (auto point = 0ul; point < nPointDomain; ++point) {
      for (auto sensor = 0u; sensor < nSensor; ++sensor) {
        su2double H[3][3] = {{0.0}};
        base_nodes->GetHessianMat(point, sensor, H);
        bool finite = true;
        passivedouble largest = 0.0;
        for (auto i = 0u; i < nDim; ++i)
          for (auto j = 0u; j < nDim; ++j) {
            const auto value = SU2_TYPE::GetValue(H[i][j]);
            finite = finite && std::isfinite(value);
            largest = std::max(largest, fabs(value));
          }
        if (finite) localHessianScale[sensor] = std::max(localHessianScale[sensor], largest);
      }
    }
    CPassiveComm::Allreduce(localHessianScale.data(), hessianScale.data(), nSensor, CPassiveComm::Op::MAX);
  }

  /*--- Eigen decomposition of |H| for a sensor. Non-finite Hessians are replaced by zero,
   *    small eigenvalues are bounded away from zero. ---*/

  auto absHessian = [&](unsigned long iPoint, unsigned short iSensor, su2double (&vec)[3][3], su2double (&val)[3],
                        unsigned long* nInvalid = nullptr) {
    su2double H[3][3] = {{0.0}}, work[3];
    base_nodes->GetHessianMat(iPoint, iSensor, H);
    bool finite = true;
    for (auto i = 0u; i < nDim; ++i)
      for (auto j = 0u; j < nDim; ++j) finite = finite && std::isfinite(SU2_TYPE::GetValue(H[i][j]));
    if (!finite) {
      if (nInvalid != nullptr) ++*nInvalid;
      for (auto i = 0u; i < nDim; ++i)
        for (auto j = 0u; j < nDim; ++j) H[i][j] = 0.0;
    }
    const auto normalization = hessianScale[iSensor] > 0.0 ? hessianScale[iSensor] : 1.0;
    for (auto i = 0u; i < nDim; ++i)
      for (auto j = 0u; j < nDim; ++j) H[i][j] /= normalization;
    CBlasStructure::EigenDecomposition(H, vec, val, nDim, work);
    for (auto i = 0u; i < nDim; ++i) val[i] = fmax(fabs(val[i]), 1e-16);
  };

  auto determinant = [&](const su2double (&val)[3]) {
    su2double det = 1.0;
    for (auto i = 0u; i < nDim; ++i) det *= val[i];
    return det;
  };

  /*--- Order of the operations:
   *    1. Lp-optimal metric of each sensor, scaled to the target complexity (so that sensors of any magnitude
   *       weigh the same), then intersection of the sensor metrics.
   *    2. One global factor scales the intersection so that the final metric has the target complexity
   *       (ADAP_COMPLEXITY is the complexity of the final metric, not of each sensor).
   *    3. Size and aspect ratio bounds, on the final tensor: the intersection of bounded metrics with different
   *       eigenvectors can violate them. The bounds change the complexity, so the global factor is solved
   *       together with them (the complexity is monotone in the factor).
   *    With one sensor and inactive bounds the factor is 1: the Lp-optimal metric scaled to the complexity. ---*/

  /*--- Integral of det(|H|)^(p/(2p+n)) for each sensor, used to reach the target complexity. ---*/

  vector<su2double> localScale(nSensor, 0.0), globalScale(nSensor, 0.0);

  if (givenMetric == nullptr) {
    vector<unsigned long> localInvalid(nSensor, 0), globalInvalid(nSensor, 0);
#if !defined(CODI_FORWARD_TYPE) && !defined(CODI_REVERSE_TYPE)
    vector<passivedouble> sensorTerms(nPointDomain * nSensor);
#endif
    for (unsigned long iPoint = 0; iPoint < nPointDomain; ++iPoint) {
      const su2double volume = geometry->nodes->GetVolume(iPoint);
      for (auto iSensor = 0u; iSensor < nSensor; ++iSensor) {
        su2double vec[3][3], val[3];
        absHessian(iPoint, iSensor, vec, val, &localInvalid[iSensor]);
        const auto term = pow(determinant(val), p / (2 * p + nDim)) * volume;
#if !defined(CODI_FORWARD_TYPE) && !defined(CODI_REVERSE_TYPE)
        sensorTerms[iPoint * nSensor + iSensor] = SU2_TYPE::GetValue(term);
#else
        localScale[iSensor] += term;
#endif
      }
    }
#if !defined(CODI_FORWARD_TYPE) && !defined(CODI_REVERSE_TYPE)
    // Reuse the protected compensated reducer; tiny normalization errors can move weak sensor directions.
    CAccurateSumBatch sensorSums;
    for (auto sensor = 0u; sensor < nSensor; ++sensor)
      sensorSums.Add(sensorTerms.empty() ? nullptr : sensorTerms.data() + sensor, nPointDomain, nSensor);
    sensorSums.Reduce();
    for (auto sensor = 0u; sensor < nSensor; ++sensor) {
      if (!sensorSums.Finite(sensor)) SU2_MPI::Error("Non-finite sensor normalization integral.", CURRENT_FUNCTION);
      globalScale[sensor] = sensorSums.Get(sensor);
    }
#else
    // Retain active MPI sums so differentiated builds preserve their existing tape/derivative semantics.
    SU2_MPI::Allreduce(localScale.data(), globalScale.data(), nSensor, MPI_DOUBLE, MPI_SUM, SU2_MPI::GetComm());
#endif
    SU2_MPI::Allreduce(localInvalid.data(), globalInvalid.data(), nSensor, MPI_UNSIGNED_LONG, MPI_SUM,
                       SU2_MPI::GetComm());
    if (rank == MASTER_NODE) {
      for (auto iSensor = 0u; iSensor < nSensor; ++iSensor) {
        if (globalInvalid[iSensor] == 0) continue;
        cout << "WARNING: Adaptation sensor " << iSensor + 1 << ": replaced non-finite Hessians by zero at "
             << globalInvalid[iSensor] << " owned mesh points. Check the sensor and its gradient reconstruction."
             << endl;
      }
    }
  } else if (givenMetric->size() < nPointDomain * nDim * (nDim + 1) / 2) {
    SU2_MPI::Error("The given metric has fewer values than the points of the mesh.", CURRENT_FUNCTION);
  }

  /*--- Lp-optimal metric of a sensor, scaled to the complexity, in its eigen decomposition. ---*/

  auto sensorMetric = [&](unsigned long iPoint, unsigned short iSensor, su2double (&vec)[3][3], su2double (&val)[3]) {
    absHessian(iPoint, iSensor, vec, val);
    const su2double factor = pow(complexity / globalScale[iSensor], 2.0 / nDim) *
                             pow(determinant(val), -1.0 / (2 * p + nDim));
    for (auto i = 0u; i < nDim; ++i) val[i] *= factor;
  };

  /*--- Intersection of the sensor metrics, stored as the metric of the point until the bounds are applied. Each
   *    sensor metric is first limited to an eigenvalue ratio of 1e14 (aspect ratio 1e7), for the accuracy of the
   *    intersection; this is looser than the aspect ratio bound unless a sensor asks for sizes far below ADAP_HMIN. ---*/

  if (nSensor > 1 && givenMetric == nullptr) {
    for (unsigned long iPoint = 0; iPoint < nPointDomain; ++iPoint) {
      su2double metric[3][3] = {{0.0}};

      bool first = true;
      for (auto iSensor = 0u; iSensor < nSensor; ++iSensor) {
        if (hessianScale[iSensor] == 0.0) continue; // A flat sensor must not impose an isotropic competing metric.
        su2double vec[3][3], val[3], current[3][3];
        sensorMetric(iPoint, iSensor, vec, val);
        const su2double valMax = *max_element(val, val + nDim);
        for (auto i = 0u; i < nDim; ++i) val[i] = fmax(val[i], 1e-14 * valMax);
        CBlasStructure::EigenRecomposition(current, vec, val, nDim);

        if (first) {
          for (auto i = 0u; i < nDim; ++i)
            for (auto j = 0u; j < nDim; ++j) metric[i][j] = current[i][j];
          first = false;
        } else {
          su2double previous[3][3];
          for (auto i = 0u; i < nDim; ++i)
            for (auto j = 0u; j < nDim; ++j) previous[i][j] = metric[i][j];
          IntersectMetrics(nDim, previous, current, metric);
        }
      }
      if (first) { // All sensors are flat: the complexity and bounds define a uniform fallback.
        for (auto i = 0u; i < nDim; ++i) metric[i][i] = 1.0;
      }
      base_nodes->SetMetricMat(iPoint, metric);
    }
  }

  /*--- Eigen decomposition of the metric before the global factor and the bounds. ---*/

  auto unboundedMetric = [&](unsigned long iPoint, su2double (&vec)[3][3], su2double (&val)[3]) {
    if (givenMetric != nullptr) {
      su2double metric[3][3] = {{0.0}}, work[3];
      const auto* row = &(*givenMetric)[iPoint * nDim * (nDim + 1) / 2];
      for (unsigned short iDim = 0, iMet = 0; iDim < nDim; ++iDim)
        for (unsigned short jDim = iDim; jDim < nDim; ++jDim, ++iMet) metric[iDim][jDim] = metric[jDim][iDim] = row[iMet];
      CBlasStructure::EigenDecomposition(metric, vec, val, nDim, work);
      for (auto i = 0u; i < nDim; ++i) val[i] = fmax(val[i], 1e-300);
    } else if (nSensor == 1) {
      sensorMetric(iPoint, 0, vec, val);
    } else {
      su2double metric[3][3] = {{0.0}}, work[3];
      base_nodes->GetMetricMat(iPoint, metric);
      CBlasStructure::EigenDecomposition(metric, vec, val, nDim, work);
    }
  };

  /*--- Final eigenvalues for a global factor: sizes between ADAP_HMIN and ADAP_HMAX, then the aspect ratio bound
   *    (which only raises the small eigenvalues, so the sizes stay within their bounds). ---*/

  auto boundEigenvalues = [&](su2double scale, su2double (&val)[3]) {
    su2double valMax = 0.0;
    for (auto i = 0u; i < nDim; ++i) {
      val[i] = fmin(fmax(scale * val[i], eigMin), eigMax);
      valMax = fmax(valMax, val[i]);
    }
    for (auto i = 0u; i < nDim; ++i) val[i] = fmax(val[i], valMax / arMax2);
  };

  /*--- Unbounded eigenvalues of all points, for the complexity as a function of the global factor. ---*/

  vector<su2double> eigenvalues(nPointDomain * nDim);
  vector<CBoundaryLayerMetric::Tensor> blFrames(nativeMetric ? nPointDomain : 0);
  vector<CBoundaryLayerMetric::Tensor> blMetric(nativeMetric ? nPoint : 0);
  for (unsigned long iPoint = 0; iPoint < nPointDomain; ++iPoint) {
    su2double vec[3][3], val[3];
    unboundedMetric(iPoint, vec, val);
    for (auto i = 0u; i < nDim; ++i) eigenvalues[iPoint * nDim + i] = val[i];
    if (nativeMetric)
      for (auto i = 0u; i < nDim; ++i)
        for (auto j = 0u; j < nDim; ++j) blFrames[iPoint].m[i][j] = vec[i][j];
  }

  /*--- The donor remains a sensor-only P1 field. Complexity samples compose original geometry
   *    at actual quadrature positions, using the same ApplySample/ApplyPoint policy as private queries. ---*/
  const auto geometryStart = SU2_MPI::Wtime();
  std::shared_ptr<CBoundaryLayerMetric> nativeLayers;
  struct CellRule {
    std::array<unsigned long, 3> nodes;
    std::vector<CBoundaryLayerMetric::IntegrationPoint2D> points;
  };
  vector<CellRule> integration;
  unsigned long localQuadraturePoints = 0;
  if (geometricBL && nDim == 2) {
    nativeLayers = nativeReference != nullptr
        ? CBoundaryLayerMetric::FromNativeReference(*nativeReference, *config)
        : std::make_shared<CBoundaryLayerMetric>(*geometry, *config);
    const auto owned = OwnedSimplices(*geometry);
    for (unsigned long cell = 0; cell < owned.size(); ++cell) {
      std::array<unsigned long, 3> nodes;
      for (unsigned k = 0; k < 3; ++k) nodes[k] = owned.nodes[3*cell+k];
      std::sort(nodes.begin(), nodes.end(), [&](auto a, auto b) {
        return geometry->nodes->GetGlobalIndex(a) < geometry->nodes->GetGlobalIndex(b);
      });
      std::array<std::array<double, 2>, 3> coordinates;
      for (unsigned k = 0; k < 3; ++k)
        for (unsigned d = 0; d < 2; ++d) coordinates[k][d] = SU2_TYPE::GetValue(geometry->nodes->GetCoord(nodes[k],d));
      auto points = nativeLayers->IntegrationRule2D(coordinates);
      localQuadraturePoints += points.size();
      integration.push_back({nodes, std::move(points)});
    }
  }
  passivedouble localGeometryTime=SU2_MPI::Wtime()-geometryStart, geometryTime=0;
  unsigned long quadraturePoints=0;
  if(nativeMetric) {
    CPassiveComm::Allreduce(&localGeometryTime,&geometryTime,1,CPassiveComm::Op::MAX);
    CPassiveComm::Allreduce(&localQuadraturePoints,&quadraturePoints,1,CPassiveComm::Op::SUM);
  }
  su2double frozenCore = eigMin, integratedComplexity = 0;
  auto syncSensor = [&]() {
    for (auto point = 0ul; point < nPointDomain; ++point) base_nodes->SetMetricMat(point, blMetric[point].m);
    InitiateComms(geometry, config, MPI_QUANTITIES::METRIC);
    CompleteComms(geometry, config, MPI_QUANTITIES::METRIC);
    for (auto point = nPointDomain; point < nPoint; ++point) base_nodes->GetMetricMat(point, blMetric[point].m);
  };
  auto density = [&](const CBoundaryLayerMetric::Tensor& metric) {
    su2double vec[3][3], val[3], work[3];
    CBlasStructure::EigenDecomposition(metric.m, vec, val, nDim, work);
    return sqrt(determinant(val));
  };

  vector<vector<unsigned long>> metricNeighbors(nativeMetric ? nPointDomain : 0);
  if (nativeMetric)
    for (auto point = 0ul; point < nPointDomain; ++point) {
      const auto neighbors = geometry->nodes->GetPoints(point);
      metricNeighbors[point].assign(neighbors.begin(), neighbors.end());
      std::sort(metricNeighbors[point].begin(), metricNeighbors[point].end(), [&](auto a, auto b) {
        return geometry->nodes->GetGlobalIndex(a) < geometry->nodes->GetGlobalIndex(b);
      });
    }
  vector<CBoundaryLayerMetric::Tensor> previous(nativeMetric ? nPoint : 0), inverse(nativeMetric ? nPointDomain : 0);
  bool gradationConverged = true;
  unsigned gradationSweeps = 0;
  passivedouble gradationViolation = 1;
  unsigned long complexityTrials = 0, totalGradationSweeps = 0, totalGradedPoints = 0;
  passivedouble gradationSeconds = 0, haloSeconds = 0, reductionSeconds = 0;
  auto gradeNativeMetric = [&]() {
    /*--- Metric-space homogeneous gradation (Alauzet, equation 9): transport M_j to i as
     *    M_j / (1 + log(hgrad) sqrt(dx^T M_j dx))^2, then intersect. Synchronous sweeps and global-ID
     *    neighbor order make the result independent of MPI ownership. Only sensor tensors enter these sweeps; no geometric wall tensor or hard-normal reset is propagated. ---*/
    const auto gradationStart = SU2_MPI::Wtime();
    const auto growth = log(config->GetAdap_Hgrad());
    // ponytail: propagation is synchronous for MPI reproducibility; a work-queue solver needs equivalent ownership semantics.
    constexpr unsigned maxSweeps = 80;
    gradationConverged = false;
    for (unsigned sweep = 0; sweep < maxSweeps; ++sweep) {
      ++totalGradationSweeps;
      const auto haloStart = SU2_MPI::Wtime();
      for (auto point = 0ul; point < nPointDomain; ++point) base_nodes->SetMetricMat(point, blMetric[point].m);
      InitiateComms(geometry, config, MPI_QUANTITIES::METRIC);
      CompleteComms(geometry, config, MPI_QUANTITIES::METRIC);
      for (auto point = nPointDomain; point < nPoint; ++point) base_nodes->GetMetricMat(point, blMetric[point].m);
      haloSeconds += SU2_MPI::Wtime() - haloStart;
      previous = blMetric;
      passivedouble localChange = 0, globalChange = 0;
      for (auto point = 0ul; point < nPointDomain; ++point) {
        ++totalGradedPoints;
        su2double vec[3][3], val[3], work[3];
        CBlasStructure::EigenDecomposition(previous[point].m, vec, val, nDim, work);
        for (auto i = 0u; i < nDim; ++i) {
          if (!(val[i] > 0) || !std::isfinite(SU2_TYPE::GetValue(val[i])))
            SU2_MPI::Error("Sensor gradation received a non-SPD trial metric.", CURRENT_FUNCTION);
          for (auto j = 0u; j < nDim; ++j) inverse[point].m[j][i] = vec[j][i] / sqrt(val[i]);
        }
        auto& current = blMetric[point];
        for (const auto neighbor : metricNeighbors[point]) {
          su2double delta[3] = {}, distance2 = 0, transported[3][3] = {}, whitened[3][3] = {};
          for (auto i = 0u; i < nDim; ++i)
            delta[i] = geometry->nodes->GetCoord(point, i) - geometry->nodes->GetCoord(neighbor, i);
          for (auto i = 0u; i < nDim; ++i)
            for (auto j = 0u; j < nDim; ++j) distance2 += delta[i] * previous[neighbor].m[i][j] * delta[j];
          const auto factor = 1 / pow(1 + growth * sqrt(fmax(0.0, distance2)), 2);
          for (auto i = 0u; i < nDim; ++i)
            for (auto j = 0u; j < nDim; ++j) {
              transported[i][j] = factor * previous[neighbor].m[i][j];
              if (nDim != 2)
                for (auto a = 0u; a < nDim; ++a)
                  for (auto b = 0u; b < nDim; ++b)
                    whitened[i][j] +=
                        inverse[point].m[a][i] * factor * previous[neighbor].m[a][b] * inverse[point].m[b][j];
            }
          if (nDim == 2) {
            /*--- A 2x2 PSD difference certifies that this transport is already satisfied, without an eigensolve. ---*/
            const auto xx = (1 + 1e-7) * current.m[0][0] - transported[0][0];
            const auto yy = (1 + 1e-7) * current.m[1][1] - transported[1][1];
            const auto xy = (1 + 1e-7) * current.m[0][1] - transported[0][1];
            if (xx >= 0 && yy >= 0 && xx * yy >= xy * xy) continue;
          } else {
            su2double eigenvectors[3][3], eigenvalues[3], workspace[3];
            CBlasStructure::EigenDecomposition(whitened, eigenvectors, eigenvalues, nDim, workspace);
            if (*max_element(eigenvalues, eigenvalues + nDim) <= 1 + 1e-7) continue;
          }
          su2double intersection[3][3];
          IntersectMetrics(nDim, current.m, transported, intersection);
          for (auto i = 0u; i < nDim; ++i)
            for (auto j = 0u; j < nDim; ++j) current.m[i][j] = 0.5 * (intersection[i][j] + intersection[j][i]);
        }
        CBlasStructure::EigenDecomposition(current.m, vec, val, nDim, work);
        boundEigenvalues(1, val);
        CBlasStructure::EigenRecomposition(current.m, vec, val, nDim);
      }
      for (auto point = 0ul; point < nPointDomain; ++point) {
        su2double difference[3][3] = {}, vec[3][3], val[3], work[3];
        for (auto i = 0u; i < nDim; ++i)
          for (auto j = 0u; j < nDim; ++j)
            for (auto a = 0u; a < nDim; ++a)
              for (auto b = 0u; b < nDim; ++b)
                difference[i][j] += inverse[point].m[a][i] * (blMetric[point].m[a][b] - previous[point].m[a][b]) *
                                    inverse[point].m[b][j];
        CBlasStructure::EigenDecomposition(difference, vec, val, nDim, work);
        for (auto i = 0u; i < nDim; ++i) localChange = std::max(localChange, fabs(SU2_TYPE::GetValue(val[i])));
      }
      const auto reductionStart = SU2_MPI::Wtime();
      CPassiveComm::Allreduce(&localChange, &globalChange, 1, CPassiveComm::Op::MAX);
      reductionSeconds += SU2_MPI::Wtime() - reductionStart;
      gradationSweeps = sweep + 1;
      if (globalChange < 1e-8) {
        gradationConverged = true;
        break;
      }
    }
    gradationSeconds += SU2_MPI::Wtime() - gradationStart;
  };

  /*--- Logarithm of the final complexity over the target, for the logarithm of the global factor. ---*/

  auto complexityError = [&](su2double logScale) {
    ++complexityTrials;
    const su2double scale = exp(logScale);
    su2double local = 0.0, global = 0.0;
    su2double localSmallest = std::numeric_limits<passivedouble>::max(), smallest = 0.0;
    for (unsigned long iPoint = 0; iPoint < nPointDomain; ++iPoint) {
      su2double val[3] = {0.0};
      for (auto i = 0u; i < nDim; ++i) val[i] = eigenvalues[iPoint * nDim + i];
      boundEigenvalues(scale, val);
      if (nativeMetric) {
        CBlasStructure::EigenRecomposition(blMetric[iPoint].m, blFrames[iPoint].m, val, nDim);
        localSmallest = fmin(localSmallest, *min_element(val, val + nDim));
      } else local += sqrt(determinant(val)) * geometry->nodes->GetVolume(iPoint);
    }
    if (nativeMetric) {
      SU2_MPI::Allreduce(&localSmallest, &smallest, 1, MPI_DOUBLE, MPI_MIN, SU2_MPI::GetComm());
      gradeNativeMetric();
      syncSensor(); // Include the final update, not the halos from the start of its last sweep.
      localSmallest = std::numeric_limits<passivedouble>::max();
      for (auto point = 0ul; point < nPointDomain; ++point) {
        su2double vec[3][3], val[3], work[3];
        CBlasStructure::EigenDecomposition(blMetric[point].m, vec, val, nDim, work);
        const auto& m=blMetric[point];
        const su2double smallest=nDim==2
            ? CBoundaryLayerMetric::CoreEigenvalue2D(SU2_TYPE::GetValue(m.m[0][0]),SU2_TYPE::GetValue(m.m[0][1]),SU2_TYPE::GetValue(m.m[1][1]))
            : *min_element(val,val+nDim);
        localSmallest = fmin(localSmallest,smallest);
      }
      SU2_MPI::Allreduce(&localSmallest, &frozenCore, 1, MPI_DOUBLE, MPI_MIN, SU2_MPI::GetComm());
      if (nativeLayers) {
        for (const auto& cell : integration)
          for (const auto& point : cell.points) {
            CBoundaryLayerMetric::Tensor metric;
            for (unsigned k = 0; k < 3; ++k)
              for (unsigned i = 0; i < 2; ++i)
                for (unsigned j = 0; j < 2; ++j)
                  metric.m[i][j] += point.barycentric[k] * blMetric[cell.nodes[k]].m[i][j];
            for (const auto& sample : point.samples)
              nativeLayers->ApplySample(sample.wall, sample.sample, metric, frozenCore, nullptr, false);
            local += point.weight * density(metric);
          }
      } else {
        for (auto point = 0ul; point < nPointDomain; ++point)
          local += density(blMetric[point]) * geometry->nodes->GetVolume(point);
      }
    }
    SU2_MPI::Allreduce(&local, &global, 1, MPI_DOUBLE, MPI_SUM, SU2_MPI::GetComm());
    integratedComplexity = global;
    return log(global / complexity);
  };

  /*--- Logarithm of the global factor for the target complexity. Start from the factor without bounds (exactly 1 when
   *    the metric already has the target complexity). With active bounds, solve with the Illinois method between the
   *    factor that gives ADAP_HMAX everywhere and the one that gives the smallest sizes allowed everywhere (the
   *    complexity does not change beyond them). bracketed is false if the target is outside that range. ---*/

  const su2double tol = 1e-6;

  auto solveGlobalFactor = [&](su2double& logScale, su2double& error, bool& bracketed) {
    su2double localValues[2] = {0.0, 0.0}, globalValues[2] = {0.0, 0.0};  // complexity, largest eigenvalue
    su2double localSmallest = std::numeric_limits<passivedouble>::max(), smallest = 0.0;

    for (unsigned long iPoint = 0; iPoint < nPointDomain; ++iPoint) {
      su2double val[3] = {0.0};
      for (auto i = 0u; i < nDim; ++i) {
        val[i] = eigenvalues[iPoint * nDim + i];
        if (val[i] > 0.0) localSmallest = fmin(localSmallest, val[i]);
        localValues[1] = fmax(localValues[1], val[i]);
      }
      localValues[0] += sqrt(fabs(determinant(val))) * geometry->nodes->GetVolume(iPoint);
    }
    SU2_MPI::Allreduce(&localValues[0], &globalValues[0], 1, MPI_DOUBLE, MPI_SUM, SU2_MPI::GetComm());
    SU2_MPI::Allreduce(&localValues[1], &globalValues[1], 1, MPI_DOUBLE, MPI_MAX, SU2_MPI::GetComm());
    SU2_MPI::Allreduce(&localSmallest, &smallest, 1, MPI_DOUBLE, MPI_MIN, SU2_MPI::GetComm());

    const su2double absoluteLow = log(eigMin / globalValues[1]);
    const su2double absoluteHigh = log(eigMax / fmin(smallest, globalValues[1]));
    logScale = log(complexity / globalValues[0]) * 2.0 / nDim;
    if (fabs(logScale) < tol) logScale = 0.0;
    if (nativeMetric) logScale = fmin(fmax(logScale, absoluteLow), absoluteHigh);
    error = complexityError(logScale);
    bracketed = true;

    if (fabs(error) <= tol) return;

    su2double xLow = absoluteLow, xHigh = absoluteHigh, fLow = 0, fHigh = 0;
    if (nativeMetric) {
      /*--- Each trial grades a complete tensor field. Bracket near the unbounded estimate first,
       *    expanding to the same hard endpoints only when necessary; avoid grading both extreme fields. ---*/
      su2double step = log(4.0);
      if (error < 0) {
        xLow = logScale; fLow = error;
        xHigh = fmin(absoluteHigh, logScale + step);
        fHigh = complexityError(xHigh);
        while (fHigh < 0 && xHigh < absoluteHigh) {
          step *= 2;
          xHigh = fmin(absoluteHigh, logScale + step);
          fHigh = complexityError(xHigh);
        }
      } else {
        xHigh = logScale; fHigh = error;
        xLow = fmax(absoluteLow, logScale - step);
        fLow = complexityError(xLow);
        while (fLow > 0 && xLow > absoluteLow) {
          step *= 2;
          xLow = fmax(absoluteLow, logScale - step);
          fLow = complexityError(xLow);
        }
      }
      if (fabs(fLow) <= tol) { logScale = xLow; error = fLow; return; }
      if (fabs(fHigh) <= tol) { logScale = xHigh; error = fHigh; return; }
    } else {
      fLow = complexityError(xLow);
      fHigh = complexityError(xHigh);
    }

    if (fLow >= 0.0) {
      bracketed = false;
      logScale = xLow;
      error = fLow;
    } else if (fHigh <= 0.0) {
      bracketed = false;
      logScale = xHigh;
      error = fHigh;
    } else {
      if (logScale > xLow && logScale < xHigh) {
        if (error < 0.0) { xLow = logScale; fLow = error; }
        else { xHigh = logScale; fHigh = error; }
      }
      int side = 0;
      for (int iter = 0; iter < 100 && fabs(error) > tol && xHigh - xLow > 1e-14 * fmax(1.0, fabs(xLow)); ++iter) {
        logScale = (xLow * fHigh - xHigh * fLow) / (fHigh - fLow);
        error = complexityError(logScale);
        if (error < 0.0) {
          xLow = logScale; fLow = error;
          if (side == -1) fHigh *= 0.5;
          side = -1;
        } else {
          xHigh = logScale; fHigh = error;
          if (side == 1) fLow *= 0.5;
          side = 1;
        }
      }
    }
  };

  su2double logScale = 0.0, error = 0.0;
  bool bracketed = true;
  solveGlobalFactor(logScale, error, bracketed);

  /*--- Isotropic metric at sharp wall corners (ADAP_ISO_CORNER, 2D). The remesher keeps such a corner and sizes the
   *    two walls on its sides independently, with the anisotropic metric along each of them; the solver's wall normal
   *    at the corner depends on the ratio of the two edge lengths. At the corner, all eigenvalues become the largest
   *    one (the smallest size, in every direction). Around it, the metric is intersected with the isotropic size
   *    h_c + log(ADAP_HGRAD) d, h_c the size at the corner (bounds and global factor applied), d the distance along
   *    mesh edges: the growth that the gradation of the remesher allows (MMG: h2 = h1 + log(hgrad) l), so there is
   *    no jump. It is propagated from the corners as a shortest path, as long as it is smaller than the largest size
   *    of the metric (beyond that, the intersection changes nothing). The intersection with an isotropic metric keeps
   *    the eigenvectors and raises each eigenvalue to at least 1/h^2. The sizes depend on the global factor, and the
   *    factor on the complexity they add, so both are repeated until the factor no longer changes (the corner
   *    regions hold a small part of the complexity, two or three passes); the final metric has the target
   *    complexity. With a fixed surface (ADAP_SURFACE= NO) the remesher keeps the corner edges as they are, so the
   *    corner metric is not used. ---*/

  vector<su2double> isoEigenvalue;
  unsigned long nCorner = 0, nIsoPoint = 0;

  if (config->GetAdap_Iso_Corner() && config->GetAdap_Surface()) {
    const auto corners = FindSharpWallPoints(geometry, config);
    unsigned long nLocal = corners.size();
    SU2_MPI::Allreduce(&nLocal, &nCorner, 1, MPI_UNSIGNED_LONG, MPI_SUM, SU2_MPI::GetComm());

    if (nDim == 2 && nCorner > 0) {
      const su2double growth = log(config->GetAdap_Hgrad());
      const su2double noSize = std::numeric_limits<passivedouble>::max();
      const auto anisotropic = eigenvalues;
      vector<su2double> size(geometry->GetnPoint());
      isoEigenvalue.resize(nPointDomain);

      /*--- Smallest and largest size of the anisotropic metric of a point, with the bounds and a global factor. ---*/
      auto boundedSizes = [&](unsigned long iPoint, su2double scale, su2double& hSmall, su2double& hLarge) {
        su2double val[3] = {0.0};
        for (auto i = 0u; i < nDim; ++i) val[i] = anisotropic[iPoint * nDim + i];
        boundEigenvalues(scale, val);
        hSmall = 1.0 / sqrt(*max_element(val, val + nDim));
        hLarge = 1.0 / sqrt(*min_element(val, val + nDim));
      };

      for (int iter = 0; iter < 10; ++iter) {
        const su2double scale = exp(logScale);
        using QueueEntry = std::pair<su2double, unsigned long>;
        std::priority_queue<QueueEntry, vector<QueueEntry>, std::greater<QueueEntry>> queue;
        std::fill(size.begin(), size.end(), noSize);

        for (const auto iPoint : corners) {
          su2double hSmall, hLarge;
          boundedSizes(iPoint, scale, hSmall, hLarge);
          size[iPoint] = hSmall;
          queue.emplace(hSmall, iPoint);
        }

        /*--- Shortest paths across the partitions: Dijkstra over the points of this rank (the domain points are
         *    updated, the halo points are sources with the sizes of the ranks that own them), then the sizes of the
         *    domain points to the halos of the other ranks, until no halo size changes on any rank. The sizes are the
         *    ones of a single Dijkstra over the whole mesh (the same sums along the same paths). ---*/
        for (unsigned long pass = 0;; ++pass) {
          while (!queue.empty()) {
            const auto h = queue.top().first;
            const auto iPoint = queue.top().second;
            queue.pop();
            if (h > size[iPoint]) continue;
            const auto coord_i = geometry->nodes->GetCoord(iPoint);
            for (const auto jPoint : geometry->nodes->GetPoints(iPoint)) {
              if (jPoint >= nPointDomain) continue;
              const auto coord_j = geometry->nodes->GetCoord(jPoint);
              const su2double hNew = h + growth * GeometryToolbox::Distance(nDim, coord_i, coord_j);
              if (hNew >= size[jPoint]) continue;
              su2double hSmall, hLarge;
              boundedSizes(jPoint, scale, hSmall, hLarge);
              if (hNew >= hLarge) continue;
              size[jPoint] = hNew;
              queue.emplace(hNew, jPoint);
            }
          }
          if (SU2_MPI::GetSize() <= 1) break;
          const vector<su2double> previous(size.begin() + nPointDomain, size.end());
          CMeshGather::ExchangeHalo(*geometry, size.data(), 1);
          unsigned long nChanged = 0, nChangedGlobal = 0;
          for (auto iPoint = nPointDomain; iPoint < geometry->GetnPoint(); ++iPoint) {
            if (size[iPoint] < previous[iPoint - nPointDomain]) {
              queue.emplace(size[iPoint], iPoint);
              ++nChanged;
            }
          }
          SU2_MPI::Allreduce(&nChanged, &nChangedGlobal, 1, MPI_UNSIGNED_LONG, MPI_SUM, SU2_MPI::GetComm());
          if (nChangedGlobal == 0) break;
          if (pass > geometry->GetGlobal_nPointDomain()) {
            SU2_MPI::Error("The corner sizes did not converge across the partitions.", CURRENT_FUNCTION);
          }
        }

        /*--- Eigenvalues before the global factor; exactly isotropic at the corners, whatever the factor and the
         *    bounds. ---*/
        nLocal = 0;
        for (unsigned long iPoint = 0; iPoint < nPointDomain; ++iPoint) {
          isoEigenvalue[iPoint] = 0.0;
          if (size[iPoint] < noSize) {
            isoEigenvalue[iPoint] = 1.0 / (pow(size[iPoint], 2) * scale);
            ++nLocal;
          }
        }
        for (const auto iPoint : corners)
          isoEigenvalue[iPoint] = *max_element(&anisotropic[iPoint * nDim], &anisotropic[iPoint * nDim] + nDim);

        for (unsigned long iPoint = 0; iPoint < nPointDomain; ++iPoint)
          for (auto i = 0u; i < nDim; ++i)
            eigenvalues[iPoint * nDim + i] = fmax(anisotropic[iPoint * nDim + i], isoEigenvalue[iPoint]);

        const su2double previous = logScale;
        solveGlobalFactor(logScale, error, bracketed);
        if (fabs(logScale - previous) < tol) break;
      }
      SU2_MPI::Allreduce(&nLocal, &nIsoPoint, 1, MPI_UNSIGNED_LONG, MPI_SUM, SU2_MPI::GetComm());
    }
  }

  /*--- Final metric. Re-evaluate the selected endpoint if the root was infeasible. ---*/
  if (nativeMetric) error = complexityError(logScale);
  passivedouble composedViolation = 1;
  unsigned long sensorBadEdges = 0, composedBadEdges = 0;
  if (nativeMetric) {
    syncSensor();
    vector<CBoundaryLayerMetric::Tensor> composed = blMetric;
    if (nativeLayers)
      for (auto point = 0ul; point < nPoint; ++point)
        nativeLayers->ApplyPoint(geometry->nodes->GetCoord(point), composed[point], frozenCore, nullptr, false);
    auto transportRatio = [&](const CBoundaryLayerMetric::Tensor& a, const CBoundaryLayerMetric::Tensor& b,
                              const su2double* x, const su2double* y) {
      su2double vec[3][3], val[3], work[3], inv[3][3], transformed[3][3] = {}, distance2 = 0;
      CBlasStructure::EigenDecomposition(a.m, vec, val, nDim, work);
      for (unsigned i = 0; i < nDim; ++i) {
        for (unsigned j = 0; j < nDim; ++j) {
          inv[i][j] = vec[i][j]/sqrt(val[j]);
          distance2 += (x[i]-y[i])*b.m[i][j]*(x[j]-y[j]);
        }
      }
      const auto factor = 1 / pow(1+log(config->GetAdap_Hgrad())*sqrt(fmax(0.0,distance2)),2);
      for (unsigned i = 0; i < nDim; ++i)
        for (unsigned j = 0; j < nDim; ++j)
          for (unsigned k = 0; k < nDim; ++k)
            for (unsigned l = 0; l < nDim; ++l)
              transformed[i][j] += inv[k][i]*factor*b.m[k][l]*inv[l][j];
      CBlasStructure::EigenDecomposition(transformed, vec, val, nDim, work);
      return SU2_TYPE::GetValue(*max_element(val,val+nDim));
    };
    passivedouble localRatio[2] = {1,1}, globalRatio[2] = {};
    unsigned long localBad[2] = {}, globalBad[2] = {};
    for (auto point = 0ul; point < nPointDomain; ++point) {
      for (const auto neighbor : metricNeighbors[point]) {
        const auto* x = geometry->nodes->GetCoord(point); const auto* y = geometry->nodes->GetCoord(neighbor);
        const auto sensor = transportRatio(blMetric[point],blMetric[neighbor],x,y);
        const auto combined = transportRatio(composed[point],composed[neighbor],x,y);
        localRatio[0] = std::max(localRatio[0],sensor); localRatio[1] = std::max(localRatio[1],combined);
        localBad[0] += sensor > 1+1e-5; localBad[1] += combined > 1+1e-5;
      }
    }
    CPassiveComm::Allreduce(localRatio,globalRatio,2,CPassiveComm::Op::MAX);
    CPassiveComm::Allreduce(localBad,globalBad,2,CPassiveComm::Op::SUM);
    gradationViolation = globalRatio[0]; composedViolation = globalRatio[1];
    sensorBadEdges = globalBad[0]; composedBadEdges = globalBad[1];
  }


  const su2double scale = exp(logScale);
  su2double localMinDensity = std::numeric_limits<passivedouble>::max(), localMaxDensity = 0.0;
  su2double localMaxAR = 0.0, localComplexity = 0.0, localCoreComplexity = 0.0;

  for (unsigned long iPoint = 0; iPoint < nPointDomain; ++iPoint) {
    su2double vec[3][3], val[3], metric[3][3] = {{0.0}};
    if (nativeMetric) {
      for (auto i = 0u; i < nDim; ++i) {
        val[i] = eigenvalues[iPoint*nDim+i];
        for (auto j = 0u; j < nDim; ++j) vec[i][j] = blFrames[iPoint].m[i][j];
      }
    } else unboundedMetric(iPoint, vec, val);
    if (!isoEigenvalue.empty()) {
      for (auto i = 0u; i < nDim; ++i) val[i] = fmax(val[i], isoEigenvalue[iPoint]);
    }
    boundEigenvalues(scale, val);
    localCoreComplexity += sqrt(determinant(val))*geometry->nodes->GetVolume(iPoint);
    if (nativeMetric) {
      for (auto i = 0u; i < nDim; ++i)
        for (auto j = 0u; j < nDim; ++j) metric[i][j] = blMetric[iPoint].m[i][j];
      su2double work[3];
      CBlasStructure::EigenDecomposition(metric, vec, val, nDim, work);
    } else CBlasStructure::EigenRecomposition(metric, vec, val, nDim);
    base_nodes->SetMetricMat(iPoint, metric);

    const su2double density = sqrt(determinant(val));
    localMinDensity = fmin(localMinDensity, density);
    localMaxDensity = fmax(localMaxDensity, density);
    localMaxAR = fmax(localMaxAR, sqrt(*max_element(val, val + nDim) / *min_element(val, val + nDim)));
    localComplexity += density * geometry->nodes->GetVolume(iPoint);
  }

  su2double minDensity = 0.0, maxDensity = 0.0, maxAR = 0.0, totComplexity = 0.0;
  SU2_MPI::Allreduce(&localMinDensity, &minDensity, 1, MPI_DOUBLE, MPI_MIN, SU2_MPI::GetComm());
  SU2_MPI::Allreduce(&localMaxDensity, &maxDensity, 1, MPI_DOUBLE, MPI_MAX, SU2_MPI::GetComm());
  SU2_MPI::Allreduce(&localMaxAR, &maxAR, 1, MPI_DOUBLE, MPI_MAX, SU2_MPI::GetComm());
  SU2_MPI::Allreduce(&localComplexity, &totComplexity, 1, MPI_DOUBLE, MPI_SUM, SU2_MPI::GetComm());
  su2double coreComplexity = 0;
  SU2_MPI::Allreduce(&localCoreComplexity, &coreComplexity, 1, MPI_DOUBLE, MPI_SUM, SU2_MPI::GetComm());
  metricComplexityPreBL = SU2_TYPE::GetValue(nativeMetric ? totComplexity : coreComplexity);
  metricComplexityFinal = SU2_TYPE::GetValue(nativeMetric ? integratedComplexity : totComplexity);
  metricComplexityBracketed = bracketed;

  passivedouble workLocal[3] = {gradationSeconds, haloSeconds, reductionSeconds}, workGlobal[3] = {};
  unsigned long globalGradedPoints = 0;
  if (nativeMetric) {
    CPassiveComm::Allreduce(workLocal, workGlobal, 3, CPassiveComm::Op::MAX);
    CPassiveComm::Allreduce(&totalGradedPoints, &globalGradedPoints, 1, CPassiveComm::Op::SUM);
  }
  if (rank == MASTER_NODE) {
    cout << (nativeMetric ? "Sensor-only donor metric statistics:" : "Metric field statistics:") << endl;
    cout << "Minimum density: " << minDensity << "." << endl;
    cout << "Maximum density: " << maxDensity << "." << endl;
    cout << "Maximum cell AR: " << maxAR << "." << endl;
    cout << (nativeMetric ? "Mesh complexity after native constraints: " : "Mesh complexity: ")
         << metricComplexityFinal << " (ADAP_COMPLEXITY= " << complexity << ")." << endl;
    if (nativeMetric) {
      cout << "Sensor metric complexity before gradation: " << coreComplexity << "." << endl;
      cout << "Sensor-only donor complexity after gradation: " << totComplexity << "." << endl;
      cout << "Native geometric integration: " << quadraturePoints << " points, preparation " << geometryTime << " s." << endl;
      cout << "Native metric work: " << complexityTrials << " complexity trials, " << totalGradationSweeps
           << " total gradation sweeps, " << globalGradedPoints << " owned-point visits; gradation " << workGlobal[0] << " s, halo " << workGlobal[1]
           << " s, sweep reductions " << workGlobal[2] << " s (maximum accumulated rank times)." << endl;
      cout << "Native sensor gradation: " << gradationSweeps << " sweeps, "
           << (gradationConverged ? "fixed point reached" : "iteration limit reached")
           << ", maximum transported-metric ratio " << gradationViolation
           << ", directed edges above 1+1e-5: " << sensorBadEdges << "." << endl;
      cout << "Native composed nodal audit: maximum transported-metric ratio " << composedViolation
           << ", directed edges above 1+1e-5: " << composedBadEdges
           << ". Sensor gradation is not a combined-field certificate." << endl;
      if (sensorBadEdges) cout << "WARNING: Sensor transported-metric constraints remain unsatisfied." << endl;
      if (composedBadEdges) cout << "WARNING: Composed geometric BL transport violations remain; no nodal wall correction is spread." << endl;
      if (geometricBL && fabs(error) > tol) {
        cout << "WARNING: Geometric BL complexity target " << complexity << " is "
             << (bracketed ? "not converged" : "outside the attainable bounds")
             << "; integrated complexity " << integratedComplexity << ". BL and finer sensor demands are retained." << endl;
      }

    }
    if (config->GetGoal_Oriented_Metric()) {
      cout << "Mesh complexity before the boundary-layer metric (16 digits): " << std::setprecision(16) << totComplexity
           << ", relative to ADAP_COMPLEXITY " << totComplexity / complexity - 1.0 << std::setprecision(6)
           << (bracketed ? " (target bracketed by the bounds)." : " (target outside the bounds).") << endl;
    }
    if (nDim == 2 && nCorner > 0) {
      cout << "Sharp wall corners: " << nCorner << ", isotropic metric on " << nIsoPoint << " points (ADAP_ISO_CORNER)."
           << endl;
    } else if (nCorner > 0) {
      cout << "Sharp wall points: " << nCorner << ", the isotropic corner metric (ADAP_ISO_CORNER) is applied in 2D "
           << "only, the metric is not changed." << endl;
    }
    if (!geometricBL && fabs(error) > tol && !bracketed) {
      cout << "WARNING: The mesh complexity " << totComplexity << " differs from ADAP_COMPLEXITY= " << complexity
           << ", which cannot be reached with the bounds ADAP_HMIN, ADAP_HMAX and ADAP_ARMAX. The metric has the "
           << (error > 0.0 ? "largest sizes (ADAP_HMAX)" : "smallest sizes allowed") << " everywhere." << endl;
    } else if (!geometricBL && fabs(error) > tol) {
      cout << "WARNING: The mesh complexity " << totComplexity << " did not converge to ADAP_COMPLEXITY= "
           << complexity << "." << endl;
    }
  }

  /*--- Boundary-layer metric of the wall markers (ADAP_BL_MARKER), intersected with the final metric: after the
   *    global factor and the bounds, so the requested wall resolution is kept whatever the complexity (which can
   *    then exceed ADAP_COMPLEXITY, it is printed). The fade at the outer edge of each layer goes to the isotropic
   *    metric of the largest size of the metric. ---*/

  if (boundaryLayer && config->GetnAdap_BL() > 0 &&
      config->GetKind_Adap_Remesher() != ADAP_REMESHER::NATIVE_CAVITY) {
    vector<su2double> coord(nPointDomain * nDim);
    vector<CBoundaryLayerMetric::Tensor> metric(nPointDomain);
    su2double localSmallest = std::numeric_limits<passivedouble>::max(), smallest = 0.0;
    for (unsigned long iPoint = 0; iPoint < nPointDomain; ++iPoint) {
      for (auto iDim = 0u; iDim < nDim; ++iDim) coord[iPoint * nDim + iDim] = geometry->nodes->GetCoord(iPoint, iDim);
      base_nodes->GetMetricMat(iPoint, metric[iPoint].m);
      su2double vec[3][3], val[3], work[3];
      CBlasStructure::EigenDecomposition(metric[iPoint].m, vec, val, nDim, work);
      localSmallest = fmin(localSmallest, *min_element(val, val + nDim));
    }
    SU2_MPI::Allreduce(&localSmallest, &smallest, 1, MPI_DOUBLE, MPI_MIN, SU2_MPI::GetComm());

    CBoundaryLayerMetric layers(*geometry, *config);
    const auto reports = layers.Apply(coord, metric, smallest);

    su2double localValues[2] = {0.0, 0.0}, globalValues[2] = {0.0, 0.0};  // complexity, largest aspect ratio
    for (unsigned long iPoint = 0; iPoint < nPointDomain; ++iPoint) {
      base_nodes->SetMetricMat(iPoint, metric[iPoint].m);
      su2double vec[3][3], val[3], work[3];
      CBlasStructure::EigenDecomposition(metric[iPoint].m, vec, val, nDim, work);
      localValues[0] += sqrt(fabs(determinant(val))) * geometry->nodes->GetVolume(iPoint);
      localValues[1] = fmax(localValues[1], sqrt(*max_element(val, val + nDim) / *min_element(val, val + nDim)));
    }
    SU2_MPI::Allreduce(&localValues[0], &globalValues[0], 1, MPI_DOUBLE, MPI_SUM, SU2_MPI::GetComm());
    SU2_MPI::Allreduce(&localValues[1], &globalValues[1], 1, MPI_DOUBLE, MPI_MAX, SU2_MPI::GetComm());

    for (unsigned short iWall = 0; iWall < reports.size(); ++iWall) {
      const auto& layer = layers.GetLayer(iWall);
      unsigned long local[3] = {reports[iWall].nPoint, reports[iWall].nChanged, reports[iWall].nFloor};
      unsigned long global[3] = {0, 0, 0};
      SU2_MPI::Allreduce(local, global, 3, MPI_UNSIGNED_LONG, MPI_SUM, SU2_MPI::GetComm());
      su2double localAR = reports[iWall].maxAspectRatio, maxWallAR = 0.0;
      SU2_MPI::Allreduce(&localAR, &maxWallAR, 1, MPI_DOUBLE, MPI_MAX, SU2_MPI::GetComm());
      if (rank == MASTER_NODE) {
        const su2double full = fmax(layer.firstHeight, 0.9 * layer.thickness);
        cout << "Boundary-layer metric " << layer.marker << ": first height " << layer.firstHeight << ", growth "
             << layer.growth << ", thickness " << layer.thickness << " (fade over " << layer.thickness - full
             << "), on " << global[0] << " points, finer than the metric on " << global[1]
             << ", tangential floor next to the wall on " << global[2] << ", largest aspect ratio " << maxWallAR
             << "." << endl;
        if (maxWallAR > config->GetAdap_ARmax()) {
          cout << "WARNING: The boundary-layer metric of " << layer.marker << " has aspect ratios up to " << maxWallAR
               << " (tangential wall size / wall-normal size), above ADAP_ARMAX= " << config->GetAdap_ARmax()
               << "; it is not limited by ADAP_ARMAX." << endl;
        }
      }
    }
    metricComplexityFinal = SU2_TYPE::GetValue(globalValues[0]);
    if (rank == MASTER_NODE) {
      cout << "Mesh complexity with the boundary-layer metric: " << globalValues[0] << " (ADAP_COMPLEXITY= "
           << complexity << ", ratio " << globalValues[0] / complexity << "). Maximum cell AR: " << globalValues[1]
           << "." << endl;
    }
  }

  /*--- Same metric on both sides of periodic boundaries, then on halo points. ---*/

  for (unsigned short iPeriodic = 1; iPeriodic <= config->GetnMarker_Periodic() / 2; ++iPeriodic) {
    InitiatePeriodicComms(geometry, config, iPeriodic, PERIODIC_METRIC);
    CompletePeriodicComms(geometry, config, iPeriodic, PERIODIC_METRIC);
  }

  InitiateComms(geometry, config, MPI_QUANTITIES::METRIC);
  CompleteComms(geometry, config, MPI_QUANTITIES::METRIC);
}

void CSolver::SetSolution_Gradient_GG(CGeometry *geometry, const CConfig *config, short idxVel, bool reconstruction) {
  SU2_ZONE_SCOPED

  const auto& solution = base_nodes->GetSolution();
  auto& gradient = reconstruction? base_nodes->GetGradient_Reconstruction() : base_nodes->GetGradient();
  const auto comm = reconstruction? MPI_QUANTITIES::SOLUTION_GRAD_REC : MPI_QUANTITIES::SOLUTION_GRADIENT;
  const auto commPer = reconstruction? PERIODIC_SOL_GG_R : PERIODIC_SOL_GG;
  computeGradientsGreenGauss(this, comm, commPer, *geometry, *config, solution, 0, nVar, idxVel, gradient);
}

void CSolver::SetSolution_Gradient_LS(CGeometry *geometry, const CConfig *config, short idxVel, bool reconstruction) {
  SU2_ZONE_SCOPED

  /*--- Set a flag for unweighted or weighted least-squares. ---*/
  bool weighted;
  PERIODIC_QUANTITIES commPer;

  if (reconstruction) {
    weighted = (config->GetKind_Gradient_Method_Recon() == WEIGHTED_LEAST_SQUARES);
    commPer = weighted? PERIODIC_SOL_LS_R : PERIODIC_SOL_ULS_R;
  }
  else {
    weighted = (config->GetKind_Gradient_Method() == WEIGHTED_LEAST_SQUARES);
    commPer = weighted? PERIODIC_SOL_LS : PERIODIC_SOL_ULS;
  }

  const auto& solution = base_nodes->GetSolution();
  auto& rmatrix = base_nodes->GetRmatrix();
  auto& gradient = reconstruction? base_nodes->GetGradient_Reconstruction() : base_nodes->GetGradient();
  const auto comm = reconstruction? MPI_QUANTITIES::SOLUTION_GRAD_REC : MPI_QUANTITIES::SOLUTION_GRADIENT;

  computeGradientsLeastSquares(this, comm, commPer, *geometry, *config, weighted, solution, 0, nVar, idxVel, gradient, rmatrix);
}

void CSolver::SetUndivided_Laplacian(CGeometry *geometry, const CConfig *config) {
  SU2_ZONE_SCOPED

  /*--- Loop domain points. ---*/

  SU2_OMP_FOR_DYN(256)
  for (unsigned long iPoint = 0; iPoint < nPointDomain; ++iPoint) {

    const bool boundary_i = geometry->nodes->GetPhysicalBoundary(iPoint);

    /*--- Initialize. ---*/
    for (unsigned short iVar = 0; iVar < nVar; iVar++)
      base_nodes->SetUnd_Lapl(iPoint, iVar, 0.0);

    /*--- Loop over the neighbors of point i. ---*/
    for (auto jPoint : geometry->nodes->GetPoints(iPoint)) {

      bool boundary_j = geometry->nodes->GetPhysicalBoundary(jPoint);

      /*--- If iPoint is boundary it only takes contributions from other boundary points. ---*/
      if (boundary_i && !boundary_j) continue;

      /*--- Add solution differences, with correction for compressible flows which use the enthalpy. ---*/

      for (unsigned short iVar = 0; iVar < nVar; iVar++) {
        su2double delta = base_nodes->GetSolution(jPoint,iVar)-base_nodes->GetSolution(iPoint,iVar);
        base_nodes->AddUnd_Lapl(iPoint, iVar, delta);
      }
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

void CSolver::Add_External_To_Solution() {
  SU2_ZONE_SCOPED
  for (unsigned long iPoint = 0; iPoint < nPoint; iPoint++) {
    base_nodes->AddSolution(iPoint, base_nodes->Get_External(iPoint));
  }

  base_nodes->Add_ExternalExtra_To_SolutionExtra();
}

void CSolver::Add_Solution_To_External() {
  SU2_ZONE_SCOPED
  for (unsigned long iPoint = 0; iPoint < nPoint; iPoint++) {
    base_nodes->Add_External(iPoint, base_nodes->GetSolution(iPoint));
  }

  base_nodes->Set_ExternalExtra_To_SolutionExtra();
}

void CSolver::Update_Cross_Term(CConfig *config, su2passivematrix &cross_term) {
  SU2_ZONE_SCOPED

  /*--- This method is for discrete adjoint solvers and it is used in multi-physics
   *    contexts, "cross_term" is the old value, the new one is in "Solution".
   *    We update "cross_term" and the sum of all cross terms (in "External")
   *    with a fraction of the difference between new and old.
   *    When "alpha" is 1, i.e. no relaxation, we effectively subtract the old
   *    value and add the new one to the total ("External"). ---*/

  vector<su2double> solution(nVar);
  passivedouble alpha = SU2_TYPE::GetValue(config->GetAitkenStatRelax());

  for (unsigned long iPoint = 0; iPoint < nPoint; iPoint++) {
    for (unsigned short iVar = 0; iVar < nVar; iVar++) {
      passivedouble
      new_val = SU2_TYPE::GetValue(base_nodes->GetSolution(iPoint,iVar)),
      delta = alpha * (new_val - cross_term(iPoint,iVar));
      /*--- Update cross term. ---*/
      cross_term(iPoint,iVar) += delta;
      solution[iVar] = delta;
    }
    /*--- Update the sum of all cross-terms. ---*/
    base_nodes->Add_External(iPoint, solution.data());
  }
}

void CSolver::SetGridVel_Gradient(CGeometry *geometry, const CConfig *config) const {
  SU2_ZONE_SCOPED

  /// TODO: No comms needed for this gradient? The Rmatrix should be allocated somewhere.

  const auto& gridVel = geometry->nodes->GetGridVel();
  auto& gridVelGrad = geometry->nodes->GetGridVel_Grad();
  auto rmatrix = CVectorOfMatrix(nPoint,nDim,nDim);

  computeGradientsLeastSquares(nullptr, MPI_QUANTITIES::GRID_VELOCITY, PERIODIC_NONE, *geometry, *config,
                               true, gridVel, 0, nDim, 0, gridVelGrad, rmatrix);
}

void CSolver::SetSolution_Limiter(CGeometry *geometry, const CConfig *config) {
  SU2_ZONE_SCOPED

  const auto kindLimiter = config->GetKind_SlopeLimit();
  const auto umusclKappa = config->GetMUSCL_Kappa();
  const auto& solution = base_nodes->GetSolution();
  const auto& gradient = base_nodes->GetGradient_Reconstruction();
  auto& solMin = base_nodes->GetSolution_Min();
  auto& solMax = base_nodes->GetSolution_Max();
  auto& limiter = base_nodes->GetLimiter();

  computeLimiters(kindLimiter, this, MPI_QUANTITIES::SOLUTION_LIMITER, PERIODIC_LIM_SOL_1, PERIODIC_LIM_SOL_2,
                  *geometry, *config, 0, nVar, umusclKappa, solution, gradient, solMin, solMax, limiter);
}

void CSolver::GaussElimination(su2double** A, su2double* rhs, unsigned short nVar) {
  SU2_ZONE_SCOPED

  short iVar, jVar, kVar;
  su2double weight, aux;

  if (nVar == 1)
    rhs[0] /= A[0][0];
  else {

    /*--- Transform system in Upper Matrix ---*/

    for (iVar = 1; iVar < (short)nVar; iVar++) {
      for (jVar = 0; jVar < iVar; jVar++) {
        weight = A[iVar][jVar]/A[jVar][jVar];
        for (kVar = jVar; kVar < (short)nVar; kVar++)
          A[iVar][kVar] -= weight*A[jVar][kVar];
        rhs[iVar] -= weight*rhs[jVar];
      }
    }

    /*--- Backwards substitution ---*/

    rhs[nVar-1] = rhs[nVar-1]/A[nVar-1][nVar-1];
    for (iVar = (short)nVar-2; iVar >= 0; iVar--) {
      aux = 0;
      for (jVar = iVar+1; jVar < (short)nVar; jVar++)
        aux += A[iVar][jVar]*rhs[jVar];
      rhs[iVar] = (rhs[iVar]-aux)/A[iVar][iVar];
      if (iVar == 0) break;
    }
  }

}

void CSolver::Aeroelastic(CSurfaceMovement *surface_movement, CGeometry *geometry, CConfig *config, unsigned long TimeIter) {
  SU2_ZONE_SCOPED

  /*--- Variables used for Aeroelastic case ---*/

  su2double Cl, Cd, Cn, Ct, Cm, Cn_rot;
  su2double Alpha = config->GetAoA()*PI_NUMBER/180.0;
  vector<su2double> structural_solution(4,0.0); //contains solution(displacements and rates) of typical section wing model.

  unsigned short iMarker, iMarker_Monitoring, Monitoring;
  string Marker_Tag, Monitoring_Tag;

  /*--- Loop over markers and find the ones being monitored. ---*/

  for (iMarker = 0; iMarker < config->GetnMarker_All(); iMarker++) {
    Monitoring = config->GetMarker_All_Monitoring(iMarker);
    if (Monitoring == YES) {

      /*--- Find the particular marker being monitored and get the forces acting on it. ---*/

      for (iMarker_Monitoring = 0; iMarker_Monitoring < config->GetnMarker_Monitoring(); iMarker_Monitoring++) {
        Monitoring_Tag = config->GetMarker_Monitoring_TagBound(iMarker_Monitoring);
        Marker_Tag = config->GetMarker_All_TagBound(iMarker);
        if (Marker_Tag == Monitoring_Tag) {

          Cl = GetSurface_CL(iMarker_Monitoring);
          Cd = GetSurface_CD(iMarker_Monitoring);

          /*--- For typical section wing model want the force normal to the airfoil (in the direction of the spring) ---*/
          Cn = Cl*cos(Alpha) + Cd*sin(Alpha);
          Ct = -Cl*sin(Alpha) + Cd*cos(Alpha);

          Cm = GetSurface_CMz(iMarker_Monitoring);

          /*--- Calculate forces for the Typical Section Wing Model taking into account rotation ---*/

          /*--- Note that the calculation of the forces and the subsequent displacements ...
           is only correct for the airfoil that starts at the 0 degree position ---*/

          if (config->GetKind_GridMovement() == AEROELASTIC_RIGID_MOTION) {
            su2double Omega, dt, psi;
            dt = config->GetDelta_UnstTimeND();
            Omega  = (config->GetRotation_Rate(2)/config->GetOmega_Ref());
            psi = Omega*(dt*TimeIter);

            /*--- Correct for the airfoil starting position (This is hardcoded in here) ---*/
            if (Monitoring_Tag == "Airfoil1") {
              psi = psi + 0.0;
            }
            else if (Monitoring_Tag == "Airfoil2") {
              psi = psi + 2.0/3.0*PI_NUMBER;
            }
            else if (Monitoring_Tag == "Airfoil3") {
              psi = psi + 4.0/3.0*PI_NUMBER;
            }
            else
              cout << "WARNING: There is a marker that we are monitoring that doesn't match the values hardcoded above!" << endl;

            cout << Monitoring_Tag << " position " << psi*180.0/PI_NUMBER << " degrees. " << endl;

            Cn_rot = Cn*cos(psi) - Ct*sin(psi); //Note the signs are different for accounting for the AOA.
            Cn = Cn_rot;
          }

          /*--- Solve the aeroelastic equations for the particular marker(surface) ---*/

          SolveTypicalSectionWingModel(geometry, Cn, Cm, config, iMarker_Monitoring, structural_solution);

          break;
        }
      }

      /*--- Compute the new surface node locations ---*/
      surface_movement->AeroelasticDeform(geometry, config, TimeIter, iMarker, iMarker_Monitoring, structural_solution);

    }

  }

}

void CSolver::SetUpTypicalSectionWingModel(vector<vector<su2double> >& Phi, vector<su2double>& omega, CConfig *config) {
  SU2_ZONE_SCOPED

  /*--- Retrieve values from the config file ---*/
  su2double w_h = config->GetAeroelastic_Frequency_Plunge();
  su2double w_a = config->GetAeroelastic_Frequency_Pitch();
  su2double x_a = config->GetAeroelastic_CG_Location();
  su2double r_a = sqrt(config->GetAeroelastic_Radius_Gyration_Squared());
  su2double w = w_h/w_a;

  // Mass Matrix
  vector<vector<su2double> > M(2,vector<su2double>(2,0.0));
  M[0][0] = 1;
  M[0][1] = x_a;
  M[1][0] = x_a;
  M[1][1] = r_a*r_a;

  // Stiffness Matrix
  //  vector<vector<su2double> > K(2,vector<su2double>(2,0.0));
  //  K[0][0] = (w_h/w_a)*(w_h/w_a);
  //  K[0][1] = 0.0;
  //  K[1][0] = 0.0;
  //  K[1][1] = r_a*r_a;

  /* Eigenvector and Eigenvalue Matrices of the Generalized EigenValue Problem. */

  vector<vector<su2double> > Omega2(2,vector<su2double>(2,0.0));
  su2double aux; // auxiliary variable
  aux = sqrt(pow(r_a,2)*pow(w,4) - 2*pow(r_a,2)*pow(w,2) + pow(r_a,2) + 4*pow(x_a,2)*pow(w,2));
  Phi[0][0] = (r_a * (r_a - r_a*pow(w,2) + aux)) / (2*x_a*pow(w, 2));
  Phi[0][1] = (r_a * (r_a - r_a*pow(w,2) - aux)) / (2*x_a*pow(w, 2));
  Phi[1][0] = 1.0;
  Phi[1][1] = 1.0;

  Omega2[0][0] = (r_a * (r_a + r_a*pow(w,2) - aux)) / (2*(pow(r_a, 2) - pow(x_a, 2)));
  Omega2[0][1] = 0;
  Omega2[1][0] = 0;
  Omega2[1][1] = (r_a * (r_a + r_a*pow(w,2) + aux)) / (2*(pow(r_a, 2) - pow(x_a, 2)));

  /* Nondimesionalize the Eigenvectors such that Phi'*M*Phi = I and PHI'*K*PHI = Omega */
  // Phi'*M*Phi = D
  // D^(-1/2)*Phi'*M*Phi*D^(-1/2) = D^(-1/2)*D^(1/2)*D^(1/2)*D^(-1/2) = I,  D^(-1/2) = inv(sqrt(D))
  // Phi = Phi*D^(-1/2)

  vector<vector<su2double> > Aux(2,vector<su2double>(2,0.0));
  vector<vector<su2double> > D(2,vector<su2double>(2,0.0));
  // Aux = M*Phi
  for (int i=0; i<2; i++) {
    for (int j=0; j<2; j++) {
      Aux[i][j] = 0;
      for (int k=0; k<2; k++) {
        Aux[i][j] += M[i][k]*Phi[k][j];
      }
    }
  }

  // D = Phi'*Aux
  for (int i=0; i<2; i++) {
    for (int j=0; j<2; j++) {
      D[i][j] = 0;
      for (int k=0; k<2; k++) {
        D[i][j] += Phi[k][i]*Aux[k][j]; //PHI transpose
      }
    }
  }

  //Modify the first column
  Phi[0][0] = Phi[0][0] * 1/sqrt(D[0][0]);
  Phi[1][0] = Phi[1][0] * 1/sqrt(D[0][0]);
  //Modify the second column
  Phi[0][1] = Phi[0][1] * 1/sqrt(D[1][1]);
  Phi[1][1] = Phi[1][1] * 1/sqrt(D[1][1]);

  // Sqrt of the eigenvalues (frequency of vibration of the modes)
  omega[0] = sqrt(Omega2[0][0]);
  omega[1] = sqrt(Omega2[1][1]);

}

void CSolver::SolveTypicalSectionWingModel(CGeometry *geometry, su2double Cl, su2double Cm, CConfig *config, unsigned short iMarker, vector<su2double>& displacements) {
  SU2_ZONE_SCOPED

  /*--- The aeroelastic model solved in this routine is the typical section wing model
   The details of the implementation are similar to those found in J.J. Alonso
   "Fully-Implicit Time-Marching Aeroelastic Solutions" 1994. ---*/

  /*--- Retrieve values from the config file ---*/
  su2double w_alpha = config->GetAeroelastic_Frequency_Pitch();
  su2double vf      = config->GetAeroelastic_Flutter_Speed_Index();
  su2double b       = config->GetLength_Reynolds()/2.0; // airfoil semichord, Reynolds length is by defaul 1.0
  su2double dt      = config->GetDelta_UnstTimeND();
  dt = dt*w_alpha; //Non-dimensionalize the structural time.

  /*--- Structural Equation damping ---*/
  vector<su2double> xi(2,0.0);

  /*--- Eigenvectors and Eigenvalues of the Generalized EigenValue Problem. ---*/
  vector<vector<su2double> > Phi(2,vector<su2double>(2,0.0));   // generalized eigenvectors.
  vector<su2double> w(2,0.0);        // sqrt of the generalized eigenvalues (frequency of vibration of the modes).
  SetUpTypicalSectionWingModel(Phi, w, config);

  /*--- Solving the Decoupled Aeroelastic Problem with second order time discretization Eq (9) ---*/

  /*--- Solution variables description. //x[j][i], j-entry, i-equation. // Time (n+1)->np1, n->n, (n-1)->n1 ---*/
  vector<vector<su2double> > x_np1(2,vector<su2double>(2,0.0));

  /*--- Values from previous movement of spring at true time step n+1
   We use this values because we are solving for delta changes not absolute changes ---*/
  vector<vector<su2double> > x_np1_old = config->GetAeroelastic_np1(iMarker);

  /*--- Values at previous timesteps. ---*/
  vector<vector<su2double> > x_n = config->GetAeroelastic_n(iMarker);
  vector<vector<su2double> > x_n1 = config->GetAeroelastic_n1(iMarker);

  /*--- Set up of variables used to solve the structural problem. ---*/
  vector<su2double> f_tilde(2,0.0);
  vector<vector<su2double> > A_inv(2,vector<su2double>(2,0.0));
  su2double detA;
  su2double s1, s2;
  vector<su2double> rhs(2,0.0); //right hand side
  vector<su2double> eta(2,0.0);
  vector<su2double> eta_dot(2,0.0);

  /*--- Forcing Term ---*/
  su2double cons = vf*vf/PI_NUMBER;
  vector<su2double> f(2,0.0);
  f[0] = cons*(-Cl);
  f[1] = cons*(2*-Cm);

  //f_tilde = Phi'*f
  for (int i=0; i<2; i++) {
    f_tilde[i] = 0;
    for (int k=0; k<2; k++) {
      f_tilde[i] += Phi[k][i]*f[k]; //PHI transpose
    }
  }

  /*--- solve each decoupled equation (The inverse of the 2x2 matrix is provided) ---*/
  for (int i=0; i<2; i++) {
    /* Matrix Inverse */
    detA = 9.0/(4.0*dt*dt) + 3*w[i]*xi[i]/(dt) + w[i]*w[i];
    A_inv[0][0] = 1/detA * (3/(2.0*dt) + 2*xi[i]*w[i]);
    A_inv[0][1] = 1/detA * 1;
    A_inv[1][0] = 1/detA * -w[i]*w[i];
    A_inv[1][1] = 1/detA * 3/(2.0*dt);

    /* Source Terms from previous iterations */
    s1 = (-4*x_n[0][i] + x_n1[0][i])/(2.0*dt);
    s2 = (-4*x_n[1][i] + x_n1[1][i])/(2.0*dt);

    /* Problem Right Hand Side */
    rhs[0] = -s1;
    rhs[1] = f_tilde[i]-s2;

    /* Solve the equations */
    x_np1[0][i] = A_inv[0][0]*rhs[0] + A_inv[0][1]*rhs[1];
    x_np1[1][i] = A_inv[1][0]*rhs[0] + A_inv[1][1]*rhs[1];

    eta[i] = x_np1[0][i]-x_np1_old[0][i];  // For displacements, the change(deltas) is used.
    eta_dot[i] = x_np1[1][i]; // For velocities, absolute values are used.
  }

  /*--- Transform back from the generalized coordinates to get the actual displacements in plunge and pitch  q = Phi*eta ---*/
  vector<su2double> q(2,0.0);
  vector<su2double> q_dot(2,0.0);
  for (int i=0; i<2; i++) {
    q[i] = 0;
    q_dot[i] = 0;
    for (int k=0; k<2; k++) {
      q[i] += Phi[i][k]*eta[k];
      q_dot[i] += Phi[i][k]*eta_dot[k];
    }
  }

  su2double dh = b*q[0];
  su2double dalpha = q[1];

  su2double h_dot = w_alpha*b*q_dot[0];  //The w_a brings it back to actual time.
  su2double alpha_dot = w_alpha*q_dot[1];

  /*--- Set the solution of the structural equations ---*/
  displacements[0] = dh;
  displacements[1] = dalpha;
  displacements[2] = h_dot;
  displacements[3] = alpha_dot;

  /*--- Calculate the total plunge and total pitch displacements for the unsteady step by summing the displacement at each sudo time step ---*/
  su2double pitch, plunge;
  pitch = config->GetAeroelastic_pitch(iMarker);
  plunge = config->GetAeroelastic_plunge(iMarker);

  config->SetAeroelastic_pitch(iMarker , pitch+dalpha);
  config->SetAeroelastic_plunge(iMarker , plunge+dh/b);

  /*--- Set the Aeroelastic solution at time n+1. This gets update every sudo time step
   and after convering the sudo time step the solution at n+1 get moved to the solution at n
   in SetDualTime_Solver method ---*/

  config->SetAeroelastic_np1(iMarker, x_np1);

}

void CSolver::Restart_OldGeometry(CGeometry *geometry, CConfig *config) const {
  SU2_ZONE_SCOPED

  BEGIN_SU2_OMP_SAFE_GLOBAL_ACCESS {

  /*--- This function is intended for dual time simulations ---*/

  int Unst_RestartIter;
  ifstream restart_file_n;

  string filename = config->GetSolution_FileName();
  string filename_n;

  /*--- Auxiliary vector for storing the coordinates ---*/
  su2double Coord[3] = {0.0};

  /*--- Variables for reading the restart files ---*/
  string text_line;
  long iPoint_Local;
  unsigned long iPoint_Global_Local = 0, iPoint_Global = 0;

  /*--- First, we load the restart file for time n ---*/

  /*-------------------------------------------------------------------------------------------*/

  /*--- Modify file name for an unsteady restart ---*/
  if (config->GetRestart()) Unst_RestartIter = SU2_TYPE::Int(config->GetRestart_Iter())-1;
  else Unst_RestartIter = SU2_TYPE::Int(config->GetUnst_AdjointIter())-1;
  filename_n = config->GetFilename(filename, ".csv", Unst_RestartIter);

  /*--- Open the restart file, throw an error if this fails. ---*/

  restart_file_n.open(filename_n.data(), ios::in);
  if (restart_file_n.fail()) {
    SU2_MPI::Error(string("There is no flow restart file ") + filename_n, CURRENT_FUNCTION);
  }

  /*--- First, set all indices to a negative value by default, and Global n indices to 0 ---*/
  iPoint_Global_Local = 0; iPoint_Global = 0;

  /*--- Read all lines in the restart file ---*/
  /*--- The first line is the header ---*/

  getline (restart_file_n, text_line);

  for (iPoint_Global = 0; iPoint_Global < geometry->GetGlobal_nPointDomain(); iPoint_Global++ ) {

    getline (restart_file_n, text_line);

    vector<string> point_line = PrintingToolbox::split(text_line, ',');

    /*--- Retrieve local index. If this node from the restart file lives
     on the current processor, we will load and instantiate the vars. ---*/

    iPoint_Local = geometry->GetGlobal_to_Local_Point(iPoint_Global);

    if (iPoint_Local > -1) {

      Coord[0] = PrintingToolbox::stod(point_line[1]);
      Coord[1] = PrintingToolbox::stod(point_line[2]);
      if (nDim == 3){
        Coord[2] = PrintingToolbox::stod(point_line[3]);
      }
      geometry->nodes->SetCoord_n(iPoint_Local, Coord);

      iPoint_Global_Local++;
    }
  }

  /*--- Detect a wrong solution file ---*/

  if (iPoint_Global_Local < geometry->GetnPointDomain()) {
    SU2_MPI::Error(string("The solution file ") + filename + string(" doesn't match with the mesh file!\n") +
                   string("It could be empty lines at the end of the file."), CURRENT_FUNCTION);
  }

  /*--- Close the restart file ---*/

  restart_file_n.close();

  /*-------------------------------------------------------------------------------------------*/
  /*-------------------------------------------------------------------------------------------*/

  /*--- Now, we load the restart file for time n-1, if the simulation is 2nd Order ---*/

  if (config->GetTime_Marching() == TIME_MARCHING::DT_STEPPING_2ND) {

    ifstream restart_file_n1;
    string filename_n1;

    /*--- Modify file name for an unsteady restart ---*/
    if (config->GetRestart()) Unst_RestartIter = SU2_TYPE::Int(config->GetRestart_Iter())-2;
    else Unst_RestartIter = SU2_TYPE::Int(config->GetUnst_AdjointIter())-2;
    filename_n1 = config->GetFilename(filename, ".csv", Unst_RestartIter);

    /*--- Open the restart file, throw an error if this fails. ---*/

    restart_file_n1.open(filename_n1.data(), ios::in);
    if (restart_file_n1.fail()) {
        SU2_MPI::Error(string("There is no flow restart file ") + filename_n1, CURRENT_FUNCTION);

    }

    /*--- First, set all indices to a negative value by default, and Global n indices to 0 ---*/
    iPoint_Global_Local = 0; iPoint_Global = 0;

    /*--- Read all lines in the restart file ---*/
    /*--- The first line is the header ---*/

    getline (restart_file_n1, text_line);

    for (iPoint_Global = 0; iPoint_Global < geometry->GetGlobal_nPointDomain(); iPoint_Global++ ) {

      getline (restart_file_n1, text_line);

      vector<string> point_line = PrintingToolbox::split(text_line, ',');

      /*--- Retrieve local index. If this node from the restart file lives
       on the current processor, we will load and instantiate the vars. ---*/

      iPoint_Local = geometry->GetGlobal_to_Local_Point(iPoint_Global);

      if (iPoint_Local > -1) {

        Coord[0] = PrintingToolbox::stod(point_line[1]);
        Coord[1] = PrintingToolbox::stod(point_line[2]);
        if (nDim == 3){
          Coord[2] = PrintingToolbox::stod(point_line[3]);
        }

        geometry->nodes->SetCoord_n1(iPoint_Local, Coord);

        iPoint_Global_Local++;
      }

    }

    /*--- Detect a wrong solution file ---*/

    if (iPoint_Global_Local < geometry->GetnPointDomain()) {
      SU2_MPI::Error(string("The solution file ") + filename + string(" doesn't match with the mesh file!\n") +
                     string("It could be empty lines at the end of the file."), CURRENT_FUNCTION);
    }

    /*--- Close the restart file ---*/

    restart_file_n1.close();

  }

  }
  END_SU2_OMP_SAFE_GLOBAL_ACCESS

  /*--- It's necessary to communicate this information ---*/

  geometry->InitiateComms(geometry, config, MPI_QUANTITIES::COORDINATES_OLD);
  geometry->CompleteComms(geometry, config, MPI_QUANTITIES::COORDINATES_OLD);

}

void CSolver::Read_SU2_Restart_ASCII(CGeometry *geometry, const CConfig *config, string val_filename) {
  SU2_ZONE_SCOPED

  ifstream restart_file;
  string text_line, Tag;
  unsigned short iVar;
  long iPoint_Local = 0; unsigned long iPoint_Global = 0;
  int counter = 0;
  fields.clear();

  Restart_Vars.resize(5);

  string error_string = "Note: ASCII restart files must be in CSV format since v7.0.\n"
                        "Check https://su2code.github.io/docs/Guide-to-v7 for more information.";

  /*--- First, check that this is not a binary restart file. ---*/

  auto* fname = val_filename.data();
  int magic_number;

#ifndef HAVE_MPI

  /*--- Serial binary input. ---*/

  FILE *fhw;
  fhw = fopen(fname,"rb");
  size_t ret;

  /*--- Error check for opening the file. ---*/

  if (!fhw) {
    SU2_MPI::Error(string("Unable to open SU2 restart file ") + fname, CURRENT_FUNCTION);
  }

  /*--- Attempt to read the first int, which should be our magic number. ---*/

  ret = fread(&magic_number, sizeof(int), 1, fhw);
  if (ret != 1) {
    SU2_MPI::Error("Error reading restart file.", CURRENT_FUNCTION);
  }

  /*--- Check that this is an SU2 binary file. SU2 binary files
   have the hex representation of "SU2" as the first int in the file. ---*/

  if (magic_number == SU2_RESTART_MAGIC_NUMBER) {
    SU2_MPI::Error(string("File ") + string(fname) + string(" is a binary SU2 restart file, expected ASCII.\n") +
                   string("SU2 reads/writes binary restart files by default.\n") +
                   string("Note that backward compatibility for ASCII restart files is\n") +
                   string("possible with the READ_BINARY_RESTART option."), CURRENT_FUNCTION);
  }

  fclose(fhw);

#else

  /*--- Parallel binary input using MPI I/O. ---*/

  MPI_File fhw;
  int ierr;

  /*--- All ranks open the file using MPI. ---*/

  ierr = MPI_File_open(SU2_MPI::GetComm(), fname, MPI_MODE_RDONLY, MPI_INFO_NULL, &fhw);

  /*--- Error check opening the file. ---*/

  if (ierr) {
    SU2_MPI::Error(string("SU2 ASCII restart file ") + string(fname) + string(" not found.\n") + error_string,
                   CURRENT_FUNCTION);
  }

  /*--- Have the master attempt to read the magic number. ---*/

  if (rank == MASTER_NODE)
    MPI_File_read(fhw, &magic_number, 1, MPI_INT, MPI_STATUS_IGNORE);

  /*--- Broadcast the number of variables to all procs and store clearly. ---*/

  SU2_MPI::Bcast(&magic_number, 1, MPI_INT, MASTER_NODE, SU2_MPI::GetComm());

  /*--- Check that this is an SU2 binary file. SU2 binary files
   have the hex representation of "SU2" as the first int in the file. ---*/

  if (magic_number == SU2_RESTART_MAGIC_NUMBER) {
    SU2_MPI::Error(string("File ") + string(fname) + string(" is a binary SU2 restart file, expected ASCII.\n") +
                   string("SU2 reads/writes binary restart files by default.\n") +
                   string("Note that backward compatibility for ASCII restart files is\n") +
                   string("possible with the READ_BINARY_RESTART option."), CURRENT_FUNCTION);
  }

  MPI_File_close(&fhw);

#endif

  /*--- Open the restart file ---*/

  restart_file.open(val_filename.data(), ios::in);

  /*--- In case there is no restart file ---*/

  if (restart_file.fail()) {
    SU2_MPI::Error(string("SU2 ASCII restart file ") + string(fname) + string(" not found.\n") + error_string,
                   CURRENT_FUNCTION);
  }

  /*--- Identify the number of fields (and names) in the restart file ---*/

  getline (restart_file, text_line);

  char delimiter = ',';
  fields = PrintingToolbox::split(text_line, delimiter);

  if (fields.size() <= 1) {
    SU2_MPI::Error(string("Restart file does not seem to be a CSV file.\n") + error_string, CURRENT_FUNCTION);
  }

  for (unsigned short iField = 0; iField < fields.size(); iField++){
    PrintingToolbox::trim(fields[iField]);
  }

  /*--- Set the number of variables, one per field in the
   restart file (without including the PointID) ---*/

  Restart_Vars[1] = (int)fields.size() - 1;

  /*--- Allocate memory for the restart data. ---*/

  Restart_Data.resize(Restart_Vars[1]*geometry->GetnPointDomain());

  /*--- Read all lines in the restart file and extract data. ---*/

  for (iPoint_Global = 0; iPoint_Global < geometry->GetGlobal_nPointDomain(); iPoint_Global++) {

    if (!getline (restart_file, text_line)) break;

    /*--- Retrieve local index. If this node from the restart file lives
     on the current processor, we will load and instantiate the vars. ---*/

    iPoint_Local = geometry->GetGlobal_to_Local_Point(iPoint_Global);

    if (iPoint_Local > -1) {

      vector<string> point_line = PrintingToolbox::split(text_line, delimiter);

      /*--- Store the solution (starting with node coordinates) --*/

      for (iVar = 0; iVar < Restart_Vars[1]; iVar++)
        Restart_Data[counter*Restart_Vars[1] + iVar] = SU2_TYPE::GetValue(PrintingToolbox::stod(point_line[iVar+1]));

      /*--- Increment our local point counter. ---*/

      counter++;

    }
  }

  if (iPoint_Global != geometry->GetGlobal_nPointDomain())
    SU2_MPI::Error("The solution file does not match the mesh, currently only binary files can be interpolated.",
                   CURRENT_FUNCTION);

}

void CSolver::Read_SU2_Restart_Binary(CGeometry *geometry, const CConfig *config, string val_filename) {
  SU2_ZONE_SCOPED

  char str_buf[CGNS_STRING_SIZE];
  auto* fname = val_filename.data();
  const int nRestart_Vars = SU2_RESTART_HEADER_SIZE;
  Restart_Vars.resize(nRestart_Vars);
  fields.clear();

#ifndef HAVE_MPI

  /*--- Serial binary input. ---*/

  FILE *fhw;
  fhw = fopen(fname,"rb");
  size_t ret;

  /*--- Error check for opening the file. ---*/

  if (!fhw) {
    SU2_MPI::Error(string("Unable to open SU2 restart file ") + string(fname), CURRENT_FUNCTION);
  }

  /*--- First, read the number of variables and points. ---*/

  ret = fread(Restart_Vars.data(), sizeof(int), nRestart_Vars, fhw);
  if (ret != (unsigned long)nRestart_Vars) {
    SU2_MPI::Error("Error reading restart file.", CURRENT_FUNCTION);
  }

  /*--- Check that this is an SU2 binary file. SU2 binary files
   have the hex representation of "SU2" as the first int in the file. ---*/

  if (Restart_Vars[0] != SU2_RESTART_MAGIC_NUMBER) {
    SU2_MPI::Error(string("File ") + string(fname) + string(" is not a binary SU2 restart file.\n") +
                   string("SU2 reads/writes binary restart files by default.\n") +
                   string("Note that backward compatibility for ASCII restart files is\n") +
                   string("possible with the READ_BINARY_RESTART option."), CURRENT_FUNCTION);
  }

  /*--- Store the number of fields and points to be read for clarity. The file may
   have been written by a build of different precision, in which case the data needs
   to be converted after reading it. ---*/

  const unsigned long nFields = Restart_Vars[1];
  const unsigned long nPointFile = Restart_Vars[2];
  const int scalarSize = GetSU2BinaryScalarSize(Restart_Vars[SU2_RESTART_PRECISION_IDX]);

  /*--- Read the variable names from the file. Note that we are adopting a
   fixed length of 33 for the string length to match with CGNS. This is
   needed for when we read the strings later. We pad the beginning of the
   variable string vector with the Point_ID tag that wasn't written. ---*/

  fields.push_back("Point_ID");
  for (auto iVar = 0u; iVar < nFields; iVar++) {
    ret = fread(str_buf, sizeof(char), CGNS_STRING_SIZE, fhw);
    if (ret != (unsigned long)CGNS_STRING_SIZE) {
      SU2_MPI::Error("Error reading restart file.", CURRENT_FUNCTION);
    }
    fields.push_back(str_buf);
  }

  /*--- For now, create a temp 1D buffer to read the data from file. ---*/

  Restart_Data.resize(nFields*nPointFile);

  /*--- Read in the data for the restart at all local points. ---*/

  if (scalarSize == static_cast<int>(sizeof(passivedouble))) {
    ret = fread(Restart_Data.data(), scalarSize, nFields*nPointFile, fhw);
  } else {
    vector<char> buffer(nFields*nPointFile*scalarSize);
    ret = fread(buffer.data(), scalarSize, nFields*nPointFile, fhw);
    SU2BinaryDataToPassive(buffer.data(), scalarSize, nFields*nPointFile, Restart_Data.data());
  }
  if (ret != nFields*nPointFile) {
    SU2_MPI::Error("Error reading restart file.", CURRENT_FUNCTION);
  }

  /*--- Close the file. ---*/

  fclose(fhw);

#else

  /*--- Parallel binary input using MPI I/O. ---*/

  MPI_File fhw;
  SU2_MPI::Status status;
  MPI_Datatype etype, filetype;
  MPI_Offset disp;

  /*--- All ranks open the file using MPI. ---*/

  int ierr = MPI_File_open(SU2_MPI::GetComm(), fname, MPI_MODE_RDONLY, MPI_INFO_NULL, &fhw);

  if (ierr) SU2_MPI::Error(string("Unable to open SU2 restart file ") + string(fname), CURRENT_FUNCTION);

  /*--- First, read the number of variables and points (i.e., cols and rows),
   which we will need in order to read the file later. Also, read the
   variable string names here. Only the master rank reads the header. ---*/

  if (rank == MASTER_NODE)
    MPI_File_read(fhw, Restart_Vars.data(), nRestart_Vars, MPI_INT, MPI_STATUS_IGNORE);

  /*--- Broadcast the number of variables to all procs and store clearly. ---*/

  SU2_MPI::Bcast(Restart_Vars.data(), nRestart_Vars, MPI_INT, MASTER_NODE, SU2_MPI::GetComm());

  /*--- Check that this is an SU2 binary file. SU2 binary files
   have the hex representation of "SU2" as the first int in the file. ---*/

  if (Restart_Vars[0] != SU2_RESTART_MAGIC_NUMBER) {
    SU2_MPI::Error(string("File ") + string(fname) + string(" is not a binary SU2 restart file.\n") +
                   string("SU2 reads/writes binary restart files by default.\n") +
                   string("Note that backward compatibility for ASCII restart files is\n") +
                   string("possible with the READ_BINARY_RESTART option."), CURRENT_FUNCTION);
  }

  /*--- Store the number of fields and points to be read for clarity. The file may
   have been written by a build of different precision, in which case the data needs
   to be converted after reading it. ---*/

  const unsigned long nFields = Restart_Vars[1];
  const unsigned long nPointFile = Restart_Vars[2];
  const int scalarSize = GetSU2BinaryScalarSize(Restart_Vars[SU2_RESTART_PRECISION_IDX]);

  /*--- Read the variable names from the file. Note that we are adopting a
   fixed length of 33 for the string length to match with CGNS. This is
   needed for when we read the strings later. ---*/

  char *mpi_str_buf = new char[nFields*CGNS_STRING_SIZE];
  if (rank == MASTER_NODE) {
    disp = nRestart_Vars*sizeof(int);
    MPI_File_read_at(fhw, disp, mpi_str_buf, nFields*CGNS_STRING_SIZE,
                     MPI_CHAR, MPI_STATUS_IGNORE);
  }

  /*--- Broadcast the string names of the variables. ---*/

  SU2_MPI::Bcast(mpi_str_buf, nFields*CGNS_STRING_SIZE, MPI_CHAR,
                 MASTER_NODE, SU2_MPI::GetComm());

  /*--- Now parse the string names and load into the config class in case
   we need them for writing visualization files (SU2_SOL). ---*/

  fields.emplace_back("Point_ID");
  for (auto iVar = 0u; iVar < nFields; iVar++) {
    const auto index = iVar*CGNS_STRING_SIZE;
    string field_buf("\"");
    for (int iChar = 0; iChar < CGNS_STRING_SIZE; iChar++) {
      str_buf[iChar] = mpi_str_buf[index + iChar];
    }
    field_buf.append(str_buf);
    field_buf.append("\"");
    fields.emplace_back(field_buf.c_str());
  }

  /*--- Free string buffer memory. ---*/

  delete [] mpi_str_buf;

  /*--- The data portion of the file holds scalars of the precision recorded in the
   header, which is not necessarily that of this build. Describe them as opaque
   blocks of bytes so that the file views do not depend on the build precision. ---*/

  MPI_Type_contiguous(scalarSize, MPI_BYTE, &etype);
  MPI_Type_commit(&etype);

  /*--- We need to ignore the 4 ints describing the nVar_Restart and nPoints,
   along with the string names of the variables. ---*/

  disp = nRestart_Vars*sizeof(int) + CGNS_STRING_SIZE*nFields*sizeof(char);

  /*--- Define a derived datatype for this rank's set of non-contiguous data
   that will be placed in the restart. Here, we are collecting each one of the
   points which are distributed throughout the file in blocks of nVar_Restart data. ---*/

  int nBlock;
  int *blocklen = nullptr;
  MPI_Aint *displace = nullptr;

  if (nPointFile == geometry->GetGlobal_nPointDomain() ||
      config->GetKind_SU2() == SU2_COMPONENT::SU2_SOL) {
    /*--- No interpolation, each rank reads the indices it needs. ---*/
    nBlock = geometry->GetnPointDomain();

    blocklen = new int[nBlock];
    displace = new MPI_Aint[nBlock];
    int counter = 0;
    for (auto iPoint_Global = 0ul; iPoint_Global < geometry->GetGlobal_nPointDomain(); ++iPoint_Global) {
      if (geometry->GetGlobal_to_Local_Point(iPoint_Global) > -1) {
        blocklen[counter] = nFields;
        displace[counter] = iPoint_Global*nFields*scalarSize;
        counter++;
      }
    }
  }
  else {
    /*--- Interpolation required, read large blocks of data. ---*/
    nBlock = 1;

    blocklen = new int[nBlock];
    displace = new MPI_Aint[nBlock];

    const auto partitioner = CLinearPartitioner(nPointFile,0);

    blocklen[0] = nFields*partitioner.GetSizeOnRank(rank);
    displace[0] = nFields*partitioner.GetFirstIndexOnRank(rank)*scalarSize;
  }

  MPI_Type_create_hindexed(nBlock, blocklen, displace, etype, &filetype);
  MPI_Type_commit(&filetype);

  /*--- Set the view for the MPI file write, i.e., describe the location in
   the file that this rank "sees" for writing its piece of the restart file. ---*/

  MPI_File_set_view(fhw, disp, etype, filetype, (char*)"native", MPI_INFO_NULL);

  /*--- For now, create a temp 1D buffer to read the data from file. ---*/

  const int bufSize = nBlock*blocklen[0];
  Restart_Data.resize(bufSize);

  /*--- Collective call for all ranks to read from their view simultaneously,
   converting the data if the file precision does not match this build. ---*/

  if (scalarSize == static_cast<int>(sizeof(passivedouble))) {
    MPI_File_read_all(fhw, Restart_Data.data(), bufSize, etype, &status);
  } else {
    vector<char> buffer(static_cast<unsigned long>(bufSize)*scalarSize);
    MPI_File_read_all(fhw, buffer.data(), bufSize, etype, &status);
    SU2BinaryDataToPassive(buffer.data(), scalarSize, bufSize, Restart_Data.data());
  }

  /*--- All ranks close the file after writing. ---*/

  MPI_File_close(&fhw);

  /*--- Free the derived datatypes and release temp memory. ---*/

  MPI_Type_free(&filetype);
  MPI_Type_free(&etype);

  delete [] blocklen;
  delete [] displace;

#endif

  if (nPointFile != geometry->GetGlobal_nPointDomain() &&
      config->GetKind_SU2() != SU2_COMPONENT::SU2_SOL) {
    InterpolateRestartData(geometry, config);
  }
}

void CSolver::InterpolateRestartData(const CGeometry *geometry, const CConfig *config) {
  SU2_ZONE_SCOPED

  if (geometry->GetGlobal_nPointDomain() == 0) return;

  if (size != SINGLE_NODE && size % 2)
    SU2_MPI::Error("Number of ranks must be multiple of 2.", CURRENT_FUNCTION);

  if (config->GetFEMSolver())
    SU2_MPI::Error("Cannot interpolate the restart file for FEM problems.", CURRENT_FUNCTION);

  /* Challenges:
   *  - Do not use too much memory by gathering the restart data in all ranks.
   *  - Do not repeat too many computations in all ranks.
   * Solution?:
   *  - Build a local ADT for the domain points (not the restart points).
   *  - Find the closest target point for each donor, which does not match all targets.
   *  - "Diffuse" the data to neighbor points.
   *  Complexity is approx. Nlt + (Nlt + Nd) log(Nlt) where Nlt is the LOCAL number
   *  of target points and Nd the TOTAL number of donors. */

  const unsigned long nFields = Restart_Vars[1];
  const unsigned long nPointFile = Restart_Vars[2];
  const auto t0 = SU2_MPI::Wtime();
  int nRecurse = 0;
  const int maxNRecurse = 128;

  if (rank == MASTER_NODE) {
    cout << "\nThe number of points in the restart file (" << nPointFile << ") does not match "
            "the mesh (" << geometry->GetGlobal_nPointDomain() << ").\n"
            "A recursive nearest neighbor interpolation will be performed." << endl;
  }

  su2activematrix localVars(nPointDomain, nFields);
  localVars = su2double(0.0);
  {
  su2vector<uint8_t> isMapped(nPoint);
  isMapped = false;

  /*--- ADT of local target points. ---*/
  {
  const auto& coord = geometry->nodes->GetCoord();
  vector<unsigned long> index(nPointDomain);
  iota(index.begin(), index.end(), 0ul);

  CADTPointsOnlyClass adt(nDim, nPointDomain, coord.data(), index.data(), false);
  vector<unsigned long>().swap(index);

  /*--- Copy local donor restart data, which will circulate over all ranks. ---*/

  const auto partitioner = CLinearPartitioner(nPointFile,0);

  unsigned long nPointDonorMax = 0;
  for (int i=0; i<size; ++i)
    nPointDonorMax = max(nPointDonorMax, partitioner.GetSizeOnRank(i));

  su2activematrix sendBuf(nPointDonorMax, nFields);

  for (auto iPoint = 0ul; iPoint < nPointDonorMax; ++iPoint) {
    const auto iPointDonor = min(iPoint,partitioner.GetSizeOnRank(rank)-1ul);
    for (auto iVar = 0ul; iVar < nFields; ++iVar)
      sendBuf(iPoint,iVar) = Restart_Data[iPointDonor*nFields+iVar];
  }

  Restart_Data = decltype(Restart_Data){};

  /*--- Make room to receive donor data from other ranks, and to map it to target points. ---*/

  su2activematrix donorVars(nPointDonorMax, nFields);
  vector<su2double> donorDist(nPointDomain, 1e12);

  /*--- Circle over all ranks. ---*/

  const int dst = (rank+1) % size; // send to next
  const int src = (rank-1+size) % size; // receive from prev.
  const int count = sendBuf.size();

  for (int iStep = 0; iStep < size; ++iStep) {

    swap(sendBuf, donorVars);

    if (iStep) {
      /*--- Odd ranks send and then receive, and vice versa. ---*/
      if (rank%2) SU2_MPI::Send(sendBuf.data(), count, MPI_DOUBLE, dst, 0, SU2_MPI::GetComm());
      else SU2_MPI::Recv(donorVars.data(), count, MPI_DOUBLE, src, 0, SU2_MPI::GetComm(), MPI_STATUS_IGNORE);

      if (rank%2==0) SU2_MPI::Send(sendBuf.data(), count, MPI_DOUBLE, dst, 0, SU2_MPI::GetComm());
      else SU2_MPI::Recv(donorVars.data(), count, MPI_DOUBLE, src, 0, SU2_MPI::GetComm(), MPI_STATUS_IGNORE);
    }

    /*--- Find the closest target for each donor. ---*/

    vector<su2double> targetDist(donorVars.rows());
    vector<unsigned long> iTarget(donorVars.rows());

    SU2_OMP_PARALLEL_(for schedule(dynamic,4*OMP_MIN_SIZE))
    for (auto iDonor = 0ul; iDonor < donorVars.rows(); ++iDonor) {
      int r=0;
      adt.DetermineNearestNode(donorVars[iDonor], targetDist[iDonor], iTarget[iDonor], r);
    }
    END_SU2_OMP_PARALLEL

    /*--- Keep the closest donor for each target (this is separate for OpenMP). ---*/

    for (auto iDonor = 0ul; iDonor < donorVars.rows(); ++iDonor) {
      const auto iPoint = iTarget[iDonor];
      const auto dist = targetDist[iDonor];

      if (dist < donorDist[iPoint]) {
        donorDist[iPoint] = dist;
        isMapped[iPoint] = true;
        for (auto iVar = 0ul; iVar < donorVars.cols(); ++iVar)
          localVars(iPoint,iVar) = donorVars(iDonor,iVar);
      }
    }
  }
  } // everything goes out of scope except "localVars" and "isMapped"

  /*--- Recursively diffuse the nearest neighbor data. ---*/

  auto nDonor = isMapped;
  bool done = false;

  SU2_OMP_PARALLEL
  while (!done && nRecurse < maxNRecurse) {
    SU2_OMP_FOR_DYN(roundUpDiv(nPointDomain,2*omp_get_num_threads()))
    for (auto iPoint = 0ul; iPoint < nPointDomain; ++iPoint) {
      /*--- Do not change points that are already interpolated. ---*/
      if (isMapped[iPoint]) continue;

      /*--- Boundaries to boundaries and domain to domain. ---*/
      const bool boundary_i = geometry->nodes->GetSolidBoundary(iPoint);

      for (const auto jPoint : geometry->nodes->GetPoints(iPoint)) {
        if (!isMapped[jPoint]) continue;
        /*--- Take data from anywhere if we are looping too many times. ---*/
        if (boundary_i != geometry->nodes->GetSolidBoundary(jPoint) && nRecurse < 8) continue;

        nDonor[iPoint]++;

        for (auto iVar = 0ul; iVar < localVars.cols(); ++iVar)
          localVars(iPoint,iVar) += localVars(jPoint,iVar);
      }

      if (nDonor[iPoint] > 0) {
        for (auto iVar = 0ul; iVar < localVars.cols(); ++iVar)
          localVars(iPoint,iVar) /= nDonor[iPoint];
        nDonor[iPoint] = true;
      }
    }
    END_SU2_OMP_FOR

    /*--- Repeat while all points are not mapped. ---*/

    SU2_OMP_MASTER {
      done = true;
      ++nRecurse;
    }
    END_SU2_OMP_MASTER

    bool myDone = true;

    SU2_OMP_FOR_STAT(16*OMP_MIN_SIZE)
    for (auto iPoint = 0ul; iPoint < nPointDomain; ++iPoint) {
      isMapped[iPoint] = nDonor[iPoint];
      myDone &= nDonor[iPoint];
    }
    END_SU2_OMP_FOR

    SU2_OMP_ATOMIC
    done &= myDone;

    SU2_OMP_BARRIER
  }
  END_SU2_OMP_PARALLEL

  } // everything goes out of scope except "localVars"

  if (nRecurse == maxNRecurse) {
    SU2_MPI::Error("Limit number of recursions reached, the meshes may be too different.", CURRENT_FUNCTION);
  }

  /*--- Move to Restart_Data in ascending order of global index, which is how a matching restart would have been read. ---*/

  Restart_Data.resize(nPointDomain*nFields);
  Restart_Vars[2] = nPointDomain;

  int counter = 0;
  for (auto iPoint_Global = 0ul; iPoint_Global < geometry->GetGlobal_nPointDomain(); ++iPoint_Global) {
    const auto iPoint = geometry->GetGlobal_to_Local_Point(iPoint_Global);
    if (iPoint >= 0) {
      for (auto iVar = 0ul; iVar < nFields; ++iVar)
        Restart_Data[counter*nFields+iVar] = SU2_TYPE::GetValue(localVars(iPoint,iVar));
      counter++;
    }
  }
  int nRecurseMax = 0;
  SU2_MPI::Reduce(&nRecurse, &nRecurseMax, 1, MPI_INT, MPI_MAX, MASTER_NODE, SU2_MPI::GetComm());

  if (rank == MASTER_NODE) {
    cout << "Number of recursions: " << nRecurseMax << ".\n"
            "Elapsed time: " << SU2_MPI::Wtime()-t0 << "s.\n" << endl;
  }
}

void CSolver::Read_SU2_Restart_Metadata(CGeometry *geometry, CConfig *config, bool adjoint, const string& val_filename) const {
  SU2_ZONE_SCOPED

  su2double AoA_ = config->GetAoA();
  su2double AoS_ = config->GetAoS();
  su2double BCThrust_ = config->GetInitial_BCThrust();
  su2double dCD_dCL_ = config->GetdCD_dCL();
  su2double dCMx_dCL_ = config->GetdCMx_dCL();
  su2double dCMy_dCL_ = config->GetdCMy_dCL();
  su2double dCMz_dCL_ = config->GetdCMz_dCL();
  su2double SPPressureDrop_ = config->GetStreamwise_Periodic_PressureDrop();
  string::size_type position;
  unsigned long InnerIter_ = 0;
  ifstream restart_file;

  /*--- Carry on with ASCII metadata reading. ---*/

  restart_file.open(val_filename.data(), ios::in);
  if (restart_file.fail()) {
    if (rank == MASTER_NODE) {
      cout << " Warning: There is no restart file (" << val_filename.data() << ")."<< endl;
      cout << " Computation will continue without updating metadata parameters." << endl;
    }
  }
  else {

    string text_line;

    /*--- Space for extra info (if any) ---*/

    while (getline (restart_file, text_line)) {

      /*--- External iteration ---*/

      position = text_line.find ("ITER=",0);
      if (position != string::npos) {
       // TODO: 'ITER=' has 5 chars, not 9!
        text_line.erase (0,9); InnerIter_ = atoi(text_line.c_str());
      }

      /*--- Angle of attack ---*/

      position = text_line.find ("AOA=",0);
      if (position != string::npos) {
        text_line.erase (0,4); AoA_ = atof(text_line.c_str());
      }

      /*--- Sideslip angle ---*/

      position = text_line.find ("SIDESLIP_ANGLE=",0);
      if (position != string::npos) {
        text_line.erase (0,15); AoS_ = atof(text_line.c_str());
      }

      /*--- BCThrust angle ---*/

      position = text_line.find ("INITIAL_BCTHRUST=",0);
      if (position != string::npos) {
        text_line.erase (0,17); BCThrust_ = atof(text_line.c_str());
      }

      /*--- dCD_dCL coefficient ---*/

      position = text_line.find ("DCD_DCL_VALUE=",0);
      if (position != string::npos) {
        text_line.erase (0,14); dCD_dCL_ = atof(text_line.c_str());
      }

      /*--- dCMx_dCL coefficient ---*/

      position = text_line.find ("DCMX_DCL_VALUE=",0);
      if (position != string::npos) {
        text_line.erase (0,15); dCMx_dCL_ = atof(text_line.c_str());
      }

      /*--- dCMy_dCL coefficient ---*/

      position = text_line.find ("DCMY_DCL_VALUE=",0);
      if (position != string::npos) {
        text_line.erase (0,15); dCMy_dCL_ = atof(text_line.c_str());
      }

      /*--- dCMz_dCL coefficient ---*/

      position = text_line.find ("DCMZ_DCL_VALUE=",0);
      if (position != string::npos) {
        text_line.erase (0,15); dCMz_dCL_ = atof(text_line.c_str());
      }

      /*--- Streamwise periodic pressure drop for prescribed massflow cases. ---*/

      position = text_line.find ("STREAMWISE_PERIODIC_PRESSURE_DROP=",0);
      if (position != string::npos) {
        // Erase the name from the line, 'STREAMWISE_PERIODIC_PRESSURE_DROP=' has 34 chars.
        text_line.erase (0,34); SPPressureDrop_ = atof(text_line.c_str());
      }

    }

    /*--- Close the restart meta file. ---*/

    restart_file.close();

  }


  /*--- Load the metadata. ---*/

  /*--- Angle of attack ---*/

  if (!config->GetDiscard_InFiles()) {
    if ((config->GetAoA() != AoA_) && (rank == MASTER_NODE)) {
      cout.precision(6);
      cout <<"WARNING: AoA in the solution file (" << AoA_ << " deg.) +" << endl;
      cout << "         AoA offset in mesh file (" << config->GetAoA_Offset() << " deg.) = " << AoA_ + config->GetAoA_Offset() << " deg." << endl;
    }
    config->SetAoA(AoA_ + config->GetAoA_Offset());
  }

  else {
    if ((config->GetAoA() != AoA_) && (rank == MASTER_NODE))
      cout <<"WARNING: Discarding the AoA in the solution file." << endl;
  }

  /*--- Sideslip angle ---*/

  if (!config->GetDiscard_InFiles()) {
    if ((config->GetAoS() != AoS_) && (rank == MASTER_NODE)) {
      cout.precision(6);
      cout <<"WARNING: AoS in the solution file (" << AoS_ << " deg.) +" << endl;
      cout << "         AoS offset in mesh file (" << config->GetAoS_Offset() << " deg.) = " << AoS_ + config->GetAoS_Offset() << " deg." << endl;
    }
    config->SetAoS(AoS_ + config->GetAoS_Offset());
  }
  else {
    if ((config->GetAoS() != AoS_) && (rank == MASTER_NODE))
      cout <<"WARNING: Discarding the AoS in the solution file." << endl;
  }

  /*--- BCThrust ---*/

  if (!config->GetDiscard_InFiles()) {
    if ((config->GetInitial_BCThrust() != BCThrust_) && (rank == MASTER_NODE))
      cout <<"WARNING: SU2 will use the initial BC Thrust provided in the solution file: " << BCThrust_ << " lbs." << endl;
    config->SetInitial_BCThrust(BCThrust_);
  }
  else {
    if ((config->GetInitial_BCThrust() != BCThrust_) && (rank == MASTER_NODE))
      cout <<"WARNING: Discarding the BC Thrust in the solution file." << endl;
  }


  if (!config->GetDiscard_InFiles()) {

    if ((config->GetdCD_dCL() != dCD_dCL_) && (rank == MASTER_NODE))
      cout <<"WARNING: SU2 will use the dCD/dCL provided in the direct solution file: " << dCD_dCL_ << "." << endl;
    config->SetdCD_dCL(dCD_dCL_);

    if ((config->GetdCMx_dCL() != dCMx_dCL_) && (rank == MASTER_NODE))
      cout <<"WARNING: SU2 will use the dCMx/dCL provided in the direct solution file: " << dCMx_dCL_ << "." << endl;
    config->SetdCMx_dCL(dCMx_dCL_);

    if ((config->GetdCMy_dCL() != dCMy_dCL_) && (rank == MASTER_NODE))
      cout <<"WARNING: SU2 will use the dCMy/dCL provided in the direct solution file: " << dCMy_dCL_ << "." << endl;
    config->SetdCMy_dCL(dCMy_dCL_);

    if ((config->GetdCMz_dCL() != dCMz_dCL_) && (rank == MASTER_NODE))
      cout <<"WARNING: SU2 will use the dCMz/dCL provided in the direct solution file: " << dCMz_dCL_ << "." << endl;
    config->SetdCMz_dCL(dCMz_dCL_);

  }

  else {

    if ((config->GetdCD_dCL() != dCD_dCL_) && (rank == MASTER_NODE))
      cout <<"WARNING: Discarding the dCD/dCL in the direct solution file." << endl;

    if ((config->GetdCMx_dCL() != dCMx_dCL_) && (rank == MASTER_NODE))
      cout <<"WARNING: Discarding the dCMx/dCL in the direct solution file." << endl;

    if ((config->GetdCMy_dCL() != dCMy_dCL_) && (rank == MASTER_NODE))
      cout <<"WARNING: Discarding the dCMy/dCL in the direct solution file." << endl;

    if ((config->GetdCMz_dCL() != dCMz_dCL_) && (rank == MASTER_NODE))
      cout <<"WARNING: Discarding the dCMz/dCL in the direct solution file." << endl;

  }

  if (!config->GetDiscard_InFiles()) {
    if ((config->GetStreamwise_Periodic_PressureDrop() != SPPressureDrop_) && (rank == MASTER_NODE))
      cout <<"WARNING: SU2 will use the STREAMWISE_PERIODIC_PRESSURE_DROP provided in the direct solution file: " << std::setprecision(16) << SPPressureDrop_ << endl;
    config->SetStreamwise_Periodic_PressureDrop(SPPressureDrop_);
  }
  else {
    if ((config->GetStreamwise_Periodic_PressureDrop() != SPPressureDrop_) && (rank == MASTER_NODE))
      cout <<"WARNING: Discarding the STREAMWISE_PERIODIC_PRESSURE_DROP in the direct solution file." << endl;
  }

  /*--- External iteration ---*/

  if ((!config->GetDiscard_InFiles()) && (!adjoint || (adjoint && config->GetRestart())))
    config->SetExtIter_OffSet(InnerIter_);

}

void CSolver::LoadInletProfile(CGeometry **geometry,
                               CSolver ***solver,
                               CConfig *config,
                               int val_iter,
                               unsigned short val_kind_solver,
                               unsigned short val_kind_marker) const {
  SU2_ZONE_SCOPED

  /*-- First, set the solver and marker kind for the particular problem at
   hand. Note that, in the future, these routines can be used for any solver
   and potentially any marker type (beyond inlets). ---*/

  const auto KIND_SOLVER = val_kind_solver;
  const auto KIND_MARKER = val_kind_marker;


  auto profile_filename = config->GetInlet_FileName();

  const auto turbulence = config->GetKind_Turb_Model() != TURB_MODEL::NONE;
  const unsigned short nVar_Turb = turbulence ? solver[MESH_0][TURB_SOL]->GetnVar() : 0;

  const auto species = config->GetKind_Species_Model() != SPECIES_MODEL::NONE;
  const unsigned short nVar_Species = species ? solver[MESH_0][SPECIES_SOL]->GetnVar() : 0;

  /*--- names of the columns in the profile ---*/
  vector<string> columnNames;
  vector<string> columnValues;

  /*--- Count the number of columns that we have for this flow case,
   excluding the coordinates. Here, we have 2 entries for the total
   conditions or mass flow, another nDim for the direction vector, and
   finally entries for the number of turbulence variables. This is only
   necessary in case we are writing a template profile file or for Inlet
   Interpolation purposes. ---*/

  const unsigned short nCol_InletFile = 2 + nDim + nVar_Turb + nVar_Species;

  /*--- for incompressible flow, we can switch the energy equation off ---*/
  /*--- for now, we write the temperature even if we are not using it ---*/
  /*--- because a number of routines depend on the presence of the temperature field ---*/
  //if (config->GetEnergy_Equation() ==false)
  //nCol_InletFile = nCol_InletFile -1;

  // create vector of column names
  for (unsigned short iMarker = 0; iMarker < config->GetnMarker_All(); iMarker++) {

    /*--- Skip if this is the wrong type of marker. ---*/
    if (config->GetMarker_All_KindBC(iMarker) != KIND_MARKER) continue;

    const string Marker_Tag = config->GetMarker_All_TagBound(iMarker);

    std::stringstream columnName,columnValue;
    columnValue << setprecision(15);
    columnValue << std::scientific;

    // Set the variables to store the flow variables. For a subsonic inlet the total conditions
    // are stored in p_value and t_value and the flow direction in flow_dir_or_vel, while for a
    // supersonic inlet the static conditions are stored in p_value and t_value and the flow
    // velocity in flow_dir_or_vel.
    su2double p_value{}, t_value{};
    const su2double* flow_dir_or_vel = nullptr;

    if (KIND_MARKER == INLET_FLOW) {
      p_value = config->GetInletPtotal(Marker_Tag);
      t_value = config->GetInletTtotal(Marker_Tag);
      flow_dir_or_vel = config->GetInletFlowDir(Marker_Tag);
    } else if (KIND_MARKER == SUPERSONIC_INLET) {
      p_value = config->GetInlet_Pressure(Marker_Tag);
      t_value = config->GetInlet_Temperature(Marker_Tag);
      flow_dir_or_vel = config->GetInlet_Velocity(Marker_Tag);
    } else {
      SU2_MPI::Error("Unsupported type of inlet.", CURRENT_FUNCTION);
    }
    columnValue << t_value << "\t" << p_value << "\t";
    for (unsigned short iDim = 0; iDim < nDim; iDim++) {
      columnValue << flow_dir_or_vel[iDim] << "\t";
    }

    columnName << left << setw(24) << "# COORD-X" << left << setw(24) << "COORD-Y";
    if (nDim == 3) columnName << left << setw(24) << "COORD-Z";

    if (KIND_MARKER == SUPERSONIC_INLET) {
      columnName << left << setw(24) << "TEMPERATURE" << left << setw(24) << "PRESSURE";
    } else if (config->GetKind_Regime() == ENUM_REGIME::COMPRESSIBLE) {
      switch (config->GetKind_Inlet()) {
        /*--- compressible conditions ---*/
        case INLET_TYPE::TOTAL_CONDITIONS:
          columnName << left << setw(24) << "TOTAL_TEMPERATURE" << left << setw(24) << "TOTAL_PRESSURE";
          break;
        case INLET_TYPE::MASS_FLOW:
          columnName << left << setw(24) << "DENSITY" << left << setw(24) << "VELOCITY";
          break;
        default:
          SU2_MPI::Error("Unsupported INLET_TYPE.", CURRENT_FUNCTION);
          break;
      }
    } else {
      switch (config->GetKind_Inc_Inlet(Marker_Tag)) {
        /*--- incompressible conditions ---*/
        case INLET_TYPE::VELOCITY_INLET:
          columnName << left << setw(24) << "TEMPERATURE " << left << setw(24) << "VELOCITY";
          break;
        case INLET_TYPE::PRESSURE_INLET:
          columnName << left << setw(24) << "TEMPERATURE" << left << setw(24) << "PRESSURE";
          break;
        default:
          SU2_MPI::Error("Unsupported INC_INLET_TYPE.", CURRENT_FUNCTION);
          break;
      }
    }

    if (KIND_MARKER == SUPERSONIC_INLET) {
      columnName << left << setw(24) << "VELOCITY-X" << left << setw(24) << "VELOCITY-Y";
      if (nDim == 3) columnName << left << setw(24) << "VELOCITY-Z";
    } else {
      columnName << left << setw(24) << "NORMAL-X" << left << setw(24) << "NORMAL-Y";
      if (nDim == 3) columnName << left << setw(24) << "NORMAL-Z";
    }

    switch (TurbModelFamily(config->GetKind_Turb_Model())) {
      case TURB_FAMILY::NONE:
        break;
      case TURB_FAMILY::SA:
        /*--- 1-equation turbulence model: SA ---*/
        columnName << left << setw(24) << "NU_TILDE";
        columnValue << config->GetNuFactor_FreeStream() * config->GetViscosity_FreeStream() / config->GetDensity_FreeStream() <<"\t";
        break;
      case TURB_FAMILY::KW:
        /*--- 2-equation turbulence model (SST) ---*/
        columnName << left << setw(24) << "TKE" << left << setw(24) << "DISSIPATION";
        columnValue << config->GetTke_FreeStream() << "\t" << config->GetOmega_FreeStream() <<"\t";
        break;
    }

    switch (config->GetKind_Species_Model()) {
      case SPECIES_MODEL::NONE: break;
      case SPECIES_MODEL::SPECIES_TRANSPORT:
        for (unsigned short iVar = 0; iVar < nVar_Species; iVar++) {
          columnName << left << setw(24) << "SPECIES_" + std::to_string(iVar);
          columnValue << config->GetInlet_SpeciesVal(Marker_Tag)[iVar] << "\t";
        }
        break;
      case SPECIES_MODEL::FLAMELET: {
        const auto& flamelet_config_options = config->GetFlameletParsedOptions();
        /*--- 2-equation flamelet model ---*/
        columnName << left << setw(24) << "PROGRESSVAR" << left << setw(24) << "ENTHALPYTOT";
        columnValue << config->GetInlet_SpeciesVal(Marker_Tag)[0] << "\t" << config->GetInlet_SpeciesVal(Marker_Tag)[1] <<"\t";
        /*--- auxiliary species transport equations ---*/
        for (unsigned short iReactant = 0; iReactant < flamelet_config_options.n_user_scalars; iReactant++) {
          columnName << left << setw(24) << flamelet_config_options.user_scalar_names[iReactant];
          columnValue << config->GetInlet_SpeciesVal(Marker_Tag)[flamelet_config_options.n_control_vars + iReactant] << "\t";
        }
        break;
      }
    }

    columnNames.push_back(columnName.str());
    columnValues.push_back(columnValue.str());

  }

  /*--- There are no markers of this type. ---*/

  const unsigned short has_names = !columnNames.empty();
  unsigned short any_has_names;
  SU2_MPI::Allreduce(&has_names, &any_has_names, 1, MPI_UNSIGNED_SHORT, MPI_MAX, SU2_MPI::GetComm());
  if (!any_has_names) return;

  /*--- Read the profile data from an ASCII file. ---*/

  CMarkerProfileReaderFVM profileReader(geometry[MESH_0], config, profile_filename, KIND_MARKER, nCol_InletFile, columnNames, columnValues);

  /*--- Load data from the restart into correct containers. ---*/

  unsigned long Marker_Counter = 0;
  unsigned short local_failure = 0;

  const su2double tolerance = config->GetInlet_Profile_Matching_Tolerance();

  for (auto iMarker = 0ul; iMarker < config->GetnMarker_All(); iMarker++) {

    /*--- Skip if this is the wrong type of marker. ---*/

    if (config->GetMarker_All_KindBC(iMarker) != KIND_MARKER) continue;

    /*--- Get tag in order to identify the correct inlet data. ---*/

    const auto Marker_Tag = config->GetMarker_All_TagBound(iMarker);

    for (auto jMarker = 0ul; jMarker < profileReader.GetNumberOfProfiles(); jMarker++) {

      /*--- If we have not found the matching marker string, continue to next marker. ---*/

      if (profileReader.GetTagForProfile(jMarker) != Marker_Tag) continue;

      /*--- Increment our counter for marker matches. ---*/

      Marker_Counter++;

      /*--- Get data for this profile. ---*/

      const vector<passivedouble>& Inlet_Data = profileReader.GetDataForProfile(jMarker);
      const auto nColumns = profileReader.GetNumberOfColumnsInProfile(jMarker);
      vector<su2double> Inlet_Data_Interpolated ((nCol_InletFile+nDim)*geometry[MESH_0]->nVertex[iMarker]);

      /*--- Define Inlet Values vectors before and after interpolation (if needed) ---*/
      vector<su2double> Inlet_Values(nCol_InletFile+nDim);
      vector<su2double> Inlet_Interpolated(nColumns);

      const auto nRows = profileReader.GetNumberOfRowsInProfile(jMarker);

      /*--- Pointer to call Set and Evaluate functions. ---*/
      vector<C1DInterpolation*> interpolator(nColumns,nullptr);
      string interpolation_function, interpolation_type;

      /*--- Define the reference for interpolation. ---*/
      unsigned short radius_index=0;
      vector<su2double> InletRadii = profileReader.GetColumnForProfile(jMarker, radius_index);
      vector<su2double> Interpolation_Column (nRows);

      bool Interpolate = true;

      switch(config->GetKindInletInterpolationFunction()){

        case (INLET_SPANWISE_INTERP::NONE):
          Interpolate = false;
          break;

        case (INLET_SPANWISE_INTERP::AKIMA_1D):
          for (auto iCol=0ul; iCol < nColumns; iCol++){
            Interpolation_Column = profileReader.GetColumnForProfile(jMarker, iCol);
            interpolator[iCol] = new CAkimaInterpolation(InletRadii,Interpolation_Column);
          }
          interpolation_function = "AKIMA";
          break;

        case (INLET_SPANWISE_INTERP::LINEAR_1D):
          for (auto iCol=0ul; iCol < nColumns; iCol++){
            Interpolation_Column = profileReader.GetColumnForProfile(jMarker, iCol);
            interpolator[iCol] = new CLinearInterpolation(InletRadii,Interpolation_Column);
          }
          interpolation_function = "LINEAR";
          break;

        case (INLET_SPANWISE_INTERP::CUBIC_1D):
          for (auto iCol=0ul; iCol < nColumns; iCol++){
            Interpolation_Column = profileReader.GetColumnForProfile(jMarker, iCol);
            interpolator[iCol] = new CCubicSpline(InletRadii,Interpolation_Column);
          }
          interpolation_function = "CUBIC";
          break;

        default:
          SU2_MPI::Error("Unknown type of interpolation function for inlets.\n",CURRENT_FUNCTION);
          break;
      }

      if (Interpolate){
        switch(config->GetKindInletInterpolationType()){
          case(INLET_INTERP_TYPE::VR_VTHETA):
            interpolation_type="VR_VTHETA";
            break;
          case(INLET_INTERP_TYPE::ALPHA_PHI):
            interpolation_type="ALPHA_PHI";
            break;
        }
        cout<<"Inlet Interpolation being done using "<<interpolation_function
            <<" function and type "<<interpolation_type<<" for "<< Marker_Tag<<endl;
        if(nDim == 3)
          cout<<"Ensure the flow direction is in z direction"<<endl;
        else if (nDim == 2)
          cout<<"Ensure the flow direction is in x direction"<<endl;
      }
      else {
        cout<<"No Inlet Interpolation being used"<<endl;
      }

      /*--- Loop through the nodes on this marker. ---*/

      for (auto iVertex = 0ul; iVertex < geometry[MESH_0]->nVertex[iMarker]; iVertex++) {

        const auto iPoint = geometry[MESH_0]->vertex[iMarker][iVertex]->GetNode();
        const auto Coord = geometry[MESH_0]->nodes->GetCoord(iPoint);

        if (!Interpolate) {

          su2double min_dist = 1e16;

          /*--- Find the distance to the closest point in our inlet profile data. ---*/

          for (auto iRow = 0ul; iRow < nRows; iRow++) {

            /*--- Get the coords for this data point. ---*/

            const auto index = iRow*nColumns;

            const auto dist = GeometryToolbox::Distance(nDim, Coord, &Inlet_Data[index]);

            /*--- Check is this is the closest point and store data if so. ---*/

            if (dist < min_dist) {
              min_dist = dist;
              for (auto iVar = 0ul; iVar < nColumns; iVar++)
                Inlet_Values[iVar] = Inlet_Data[index+iVar];
            }

          }

          /*--- If the diff is less than the tolerance, match the two.
          We could modify this to simply use the nearest neighbor, or
          eventually add something more elaborate here for interpolation. ---*/

          if (min_dist < tolerance) {

            solver[MESH_0][KIND_SOLVER]->SetInletAtVertex(Inlet_Values.data(), iMarker, iVertex);

          } else {

            unsigned long GlobalIndex = geometry[MESH_0]->nodes->GetGlobalIndex(iPoint);
            cout << "WARNING: Did not find a match between the points in the inlet file\n";
            cout << "and point " << GlobalIndex;
            cout << std::scientific;
            cout << " at location: [" << Coord[0] << ", " << Coord[1];
            if (nDim==3) cout << ", " << Coord[2];
            cout << "]\n";
            cout << "Distance to closest point: " << min_dist << "\n";
            cout << "Current tolerance:         " << tolerance << "\n\n";
            cout << "You can increase the tolerance for point matching by changing the value\n";
            cout << "of the option INLET_MATCHING_TOLERANCE in your *.cfg file." << endl;
            local_failure++;
            break;
          }

        }
        else { // Interpolate

          /* --- Calculating the radius and angle of the vertex ---*/
          /* --- Flow should be in z direction for 3D cases ---*/
          /* --- Or in x direction for 2D cases ---*/
          const su2double Interp_Radius = sqrt(pow(Coord[0],2)+ pow(Coord[1],2));
          const su2double Theta = atan2(Coord[1],Coord[0]);

          /* --- Evaluating and saving the final spline data ---*/
          for (auto iVar=0ul; iVar < nColumns; iVar++){

            /*---Evaluate spline will get the respective value of the Data set (column) specified
            for that interpolator[iVar], cycling through all columns to get all the
            data for that vertex ---*/
            Inlet_Interpolated[iVar]=interpolator[iVar]->EvaluateSpline(Interp_Radius);
            if (Interp_Radius < InletRadii.front() || Interp_Radius > InletRadii.back()) {
              cout << "WARNING: Did not find a match between the radius in the inlet file " ;
              cout << std::scientific;
              cout << "at location: [" << Coord[0] << ", " << Coord[1];
              if (nDim == 3) {cout << ", " << Coord[2];}
              cout << "]";
              cout << " with Radius: "<< Interp_Radius << endl;
              cout << "You can add a row for Radius: " << Interp_Radius <<" in the inlet file ";
              cout << "to eliminate this issue or give proper data" << endl;
              local_failure++;
              break;
            }
          }

          /*--- Correcting for Interpolation Type ---*/

          Inlet_Values = CorrectedInletValues(Inlet_Interpolated, Theta, nDim, Coord,
                                              nVar_Turb, config->GetKindInletInterpolationType());

          solver[MESH_0][KIND_SOLVER]->SetInletAtVertex(Inlet_Values.data(), iMarker, iVertex);

          for (unsigned short iVar=0; iVar < (nCol_InletFile+nDim); iVar++)
            Inlet_Data_Interpolated[iVertex*(nCol_InletFile+nDim)+iVar] = Inlet_Values[iVar];

        }

      } // end iVertex loop

      if (config->GetPrintInlet_InterpolatedData()) {
        PrintInletInterpolatedData(Inlet_Data_Interpolated, profileReader.GetTagForProfile(jMarker),
                                   geometry[MESH_0]->nVertex[iMarker], nDim, nCol_InletFile+nDim);
      }

      for (auto& interp : interpolator) delete interp;

    } // end jMarker loop

    if (local_failure > 0) break;

  } // end iMarker loop

  unsigned short global_failure;
  SU2_MPI::Allreduce(&local_failure, &global_failure, 1, MPI_UNSIGNED_SHORT, MPI_SUM, SU2_MPI::GetComm());

  if (global_failure > 0) {
    SU2_MPI::Error("Prescribed inlet data does not match markers within tolerance.", CURRENT_FUNCTION);
  }

  /*--- Copy the inlet data down to the coarse levels if multigrid is active.
   Here, we use a face area-averaging to restrict the values. ---*/

  for (auto iMesh = 1u; iMesh <= config->GetnMGLevels(); iMesh++) {
    for (auto iMarker = 0u; iMarker < config->GetnMarker_All(); iMarker++) {
      if (config->GetMarker_All_KindBC(iMarker) == KIND_MARKER) {

        const auto Marker_Tag = config->GetMarker_All_TagBound(iMarker);

        /*--- Check the number of columns and allocate temp array. ---*/

        unsigned short nColumns = 0;
        for (auto jMarker = 0ul; jMarker < profileReader.GetNumberOfProfiles(); jMarker++) {
          if (profileReader.GetTagForProfile(jMarker) == Marker_Tag) {
            nColumns = profileReader.GetNumberOfColumnsInProfile(jMarker);
            break;
          }
        }
        vector<su2double> Inlet_Values(nColumns);
        vector<su2double> Inlet_Fine(nColumns);

        /*--- Loop through the nodes on this marker. ---*/

        for (auto iVertex = 0ul; iVertex < geometry[iMesh]->nVertex[iMarker]; iVertex++) {

          /*--- Get the coarse mesh point and compute the boundary area. ---*/

          const auto iPoint = geometry[iMesh]->vertex[iMarker][iVertex]->GetNode();
          const auto Normal = geometry[iMesh]->vertex[iMarker][iVertex]->GetNormal();
          const su2double Area_Parent = GeometryToolbox::Norm(nDim, Normal);

          /*--- Reset the values for the coarse point. ---*/

          for (auto& v : Inlet_Values) v = 0.0;

          /*-- Loop through the children and extract the inlet values
           from those nodes that lie on the boundary as well as their
           boundary area. We build a face area-averaged value for the
           coarse point values from the fine grid points. Note that
           children from the interior volume will not be included in
           the averaging. ---*/

          for (auto iChildren = 0u; iChildren < geometry[iMesh]->nodes->GetnChildren_CV(iPoint); iChildren++) {
            const auto Area_Children =
                solver[iMesh-1][KIND_SOLVER]->GetInletAtVertex(iMarker, iVertex, geometry[iMesh-1], Inlet_Fine.data());
            for (auto iVar = 0u; iVar < nColumns; iVar++)
              Inlet_Values[iVar] += Inlet_Fine[iVar] * Area_Children / Area_Parent;
          }

          /*--- Set the boundary area-averaged inlet values for the coarse point. ---*/

          solver[iMesh][KIND_SOLVER]->SetInletAtVertex(Inlet_Values.data(), iMarker, iVertex);

        }
      }
    }
  }

}


void CSolver::ComputeVertexTractions(CGeometry *geometry, const CConfig *config){
  SU2_ZONE_SCOPED

  const bool viscous_flow = config->GetViscous();
  const su2double Pressure_Inf = config->GetPressure_FreeStreamND();

  for (auto iMarker = 0u; iMarker < config->GetnMarker_All(); iMarker++) {

    /*--- If this is defined as a wall ---*/
    if (!config->GetSolid_Wall(iMarker)) continue;

    // Loop over the vertices
    for (auto iVertex = 0ul; iVertex < geometry->nVertex[iMarker]; iVertex++) {

      // Recover the point index
      const auto iPoint = geometry->vertex[iMarker][iVertex]->GetNode();

      su2double auxForce[3] = {0.0};

      // Check if the node belongs to the domain (i.e, not a halo node).
      if (geometry->nodes->GetDomain(iPoint)) {

        // Get the normal at the vertex: this normal goes inside the fluid domain.
        const su2double* Normal = geometry->vertex[iMarker][iVertex]->GetNormal();

        // Retrieve the values of pressure
        const su2double Pn = base_nodes->GetPressure(iPoint);

        // Calculate tn in the fluid nodes for the inviscid term --> Units of force (non-dimensional).
        for (unsigned short iDim = 0; iDim < nDim; iDim++)
          auxForce[iDim] = -(Pn-Pressure_Inf)*Normal[iDim];

        // Calculate tn in the fluid nodes for the viscous term
        if (viscous_flow) {
          const su2double Viscosity = base_nodes->GetLaminarViscosity(iPoint);
          su2double Tau[3][3] = {{}};
          CNumerics::ComputeStressTensor(nDim, Tau, base_nodes->GetVelocityGradient(iPoint), Viscosity);
          for (unsigned short iDim = 0; iDim < nDim; iDim++) {
            auxForce[iDim] += GeometryToolbox::DotProduct(nDim, Tau[iDim], Normal);
          }
        }
      }

      // Redimensionalize the forces (Lref is 1, thus only Pref is needed).
      for (unsigned short iDim = 0; iDim < nDim; iDim++) {
        VertexTraction[iMarker][iVertex][iDim] = config->GetPressure_Ref() * auxForce[iDim];
      }
    }
  }

}

void CSolver::RegisterVertexTractions(CGeometry *geometry, const CConfig *config){
  SU2_ZONE_SCOPED

  unsigned short iMarker, iDim;
  unsigned long iVertex, iPoint;

  /*--- Loop over all the markers ---*/
  for (iMarker = 0; iMarker < config->GetnMarker_All(); iMarker++) {

    /*--- If this is defined as a wall ---*/
    if (!config->GetSolid_Wall(iMarker)) continue;

    /*--- Loop over the vertices ---*/
    SU2_OMP_FOR_STAT(OMP_MIN_SIZE)
    for (iVertex = 0; iVertex < geometry->nVertex[iMarker]; iVertex++) {

      /*--- Recover the point index ---*/
      iPoint = geometry->vertex[iMarker][iVertex]->GetNode();

      /*--- Check if the node belongs to the domain (i.e, not a halo node) ---*/
      if (!geometry->nodes->GetDomain(iPoint)) continue;

      /*--- Register the vertex traction as output ---*/
      for (iDim = 0; iDim < nDim; iDim++) {
        AD::RegisterOutput(VertexTraction[iMarker][iVertex][iDim]);
      }
    }
    END_SU2_OMP_FOR
  }

}

void CSolver::SetVertexTractionsAdjoint(CGeometry *geometry, const CConfig *config){
  SU2_ZONE_SCOPED

  unsigned short iMarker, iDim;
  unsigned long iVertex, iPoint;
  AD::Identifier index;

  /*--- Loop over all the markers ---*/
  for (iMarker = 0; iMarker < config->GetnMarker_All(); iMarker++) {

    /*--- If this is defined as a wall ---*/
    if (!config->GetSolid_Wall(iMarker)) continue;

    /*--- Loop over the vertices ---*/
    SU2_OMP_FOR_STAT(OMP_MIN_SIZE)
    for (iVertex = 0; iVertex < geometry->nVertex[iMarker]; iVertex++) {

      /*--- Recover the point index ---*/
      iPoint = geometry->vertex[iMarker][iVertex]->GetNode();

      /*--- Check if the node belongs to the domain (i.e, not a halo node) ---*/
      if (!geometry->nodes->GetDomain(iPoint)) continue;

      /*--- Set the adjoint of the vertex traction from the value received ---*/
      for (iDim = 0; iDim < nDim; iDim++) {
        AD::SetIndex(index, VertexTraction[iMarker][iVertex][iDim]);
        AD::SetDerivative(index, SU2_TYPE::GetValue(VertexTractionAdjoint[iMarker][iVertex][iDim]));
      }
    }
    END_SU2_OMP_FOR
  }

}


void CSolver::SetVerificationSolution(unsigned short nDim,
                                      unsigned short nVar,
                                      CConfig        *config) {
  SU2_ZONE_SCOPED

  /*--- Determine the verification solution to be set and
        allocate memory for the corresponding class. ---*/
  switch( config->GetVerification_Solution() ) {

    case VERIFICATION_SOLUTION::NONE:
      VerificationSolution = nullptr; break;
    case VERIFICATION_SOLUTION::INVISCID_VORTEX:
      VerificationSolution = new CInviscidVortexSolution(nDim, nVar, MGLevel, config); break;
    case VERIFICATION_SOLUTION::RINGLEB:
      VerificationSolution = new CRinglebSolution(nDim, nVar, MGLevel, config); break;
    case VERIFICATION_SOLUTION::NS_UNIT_QUAD:
      VerificationSolution = new CNSUnitQuadSolution(nDim, nVar, MGLevel, config); break;
    case VERIFICATION_SOLUTION::TAYLOR_GREEN_VORTEX:
      VerificationSolution = new CTGVSolution(nDim, nVar, MGLevel, config); break;
    case VERIFICATION_SOLUTION::INC_TAYLOR_GREEN_VORTEX:
      VerificationSolution = new CIncTGVSolution(nDim, nVar, MGLevel, config); break;
    case VERIFICATION_SOLUTION::MMS_NS_UNIT_QUAD:
      VerificationSolution = new CMMSNSUnitQuadSolution(nDim, nVar, MGLevel, config); break;
    case VERIFICATION_SOLUTION::MMS_NS_UNIT_QUAD_WALL_BC:
      VerificationSolution = new CMMSNSUnitQuadSolutionWallBC(nDim, nVar, MGLevel, config); break;
    case VERIFICATION_SOLUTION::MMS_NS_TWO_HALF_CIRCLES:
      VerificationSolution = new CMMSNSTwoHalfCirclesSolution(nDim, nVar, MGLevel, config); break;
    case VERIFICATION_SOLUTION::MMS_NS_TWO_HALF_SPHERES:
      VerificationSolution = new CMMSNSTwoHalfSpheresSolution(nDim, nVar, MGLevel, config); break;
    case VERIFICATION_SOLUTION::MMS_INC_EULER:
      VerificationSolution = new CMMSIncEulerSolution(nDim, nVar, MGLevel, config); break;
    case VERIFICATION_SOLUTION::MMS_INC_NS:
      VerificationSolution = new CMMSIncNSSolution(nDim, nVar, MGLevel, config); break;
    case VERIFICATION_SOLUTION::USER_DEFINED_SOLUTION:
      VerificationSolution = new CUserDefinedSolution(nDim, nVar, MGLevel, config); break;
  }
}

void CSolver::ComputeResidual_Multizone(const CGeometry *geometry, const CConfig *config){
  SU2_ZONE_SCOPED

  SU2_OMP_PARALLEL {

  /*--- Set Residuals to zero ---*/
  SU2_OMP_MASTER
  for (unsigned short iVar = 0; iVar < nVar; iVar++){
    Residual_BGS[iVar] = 0.0;
    Residual_Max_BGS[iVar] = 0.0;
  }
  END_SU2_OMP_MASTER

  vector<su2double> resMax(nVar,0.0), resRMS(nVar,0.0);
  vector<const su2double*> coordMax(nVar,nullptr);
  vector<unsigned long> idxMax(nVar,0);

  /*--- Set the residuals and BGSSolution_k to solution for next multizone outer iteration. ---*/
  SU2_OMP_FOR_STAT(roundUpDiv(nPoint,2*omp_get_num_threads()))
  for (unsigned long iPoint = 0; iPoint < nPoint; iPoint++) {
    const su2double domain = (iPoint < nPointDomain);
    for (unsigned short iVar = 0; iVar < nVar; iVar++) {
      const su2double Res = (base_nodes->Get_BGSSolution(iPoint,iVar) - base_nodes->Get_BGSSolution_k(iPoint,iVar))*domain;

      /*--- Update residual information for current thread. ---*/
      resRMS[iVar] += Res*Res;
      if (fabs(Res) > resMax[iVar]) {
        resMax[iVar] = fabs(Res);
        idxMax[iVar] = iPoint;
        coordMax[iVar] = geometry->nodes->GetCoord(iPoint);
      }
    }
  }
  END_SU2_OMP_FOR

  /*--- Reduce residual information over all threads in this rank. ---*/
  SU2_OMP_CRITICAL
  for (unsigned short iVar = 0; iVar < nVar; iVar++) {
    Residual_BGS[iVar] += resRMS[iVar];
    AddRes_Max_BGS(iVar, resMax[iVar], geometry->nodes->GetGlobalIndex(idxMax[iVar]), coordMax[iVar]);
  }
  END_SU2_OMP_CRITICAL
  SU2_OMP_BARRIER

  SetResidual_BGS(geometry, config);

  }
  END_SU2_OMP_PARALLEL
}

void CSolver::BasicLoadRestart(CGeometry *geometry, const CConfig *config, const string& filename, unsigned long skipVars) {
  SU2_ZONE_SCOPED

  /*--- Read and store the restart metadata. ---*/

//  Read_SU2_Restart_Metadata(geometry[MESH_0], config, true, filename);

  /*--- Load data from the restart into correct containers. ---*/

  unsigned long iPoint_Global_Local = 0;

  for (auto iPoint_Global = 0ul; iPoint_Global < geometry->GetGlobal_nPointDomain(); iPoint_Global++ ) {

    /*--- Retrieve local index. If this node from the restart file lives
     on the current processor, we will load and instantiate the vars. ---*/

    const auto iPoint_Local = geometry->GetGlobal_to_Local_Point(iPoint_Global);

    if (iPoint_Local > -1) {

      /*--- We need to store this point's data, so jump to the correct
       offset in the buffer of data from the restart file and load it. ---*/

      const auto index = iPoint_Global_Local*Restart_Vars[1] + skipVars;

      for (auto iVar = 0u; iVar < nVar; iVar++) {
        base_nodes->SetSolution(iPoint_Local, iVar, Restart_Data[index+iVar]);
      }

      iPoint_Global_Local++;
    }

  }

  /*--- Delete the class memory that is used to load the restart. ---*/

  Restart_Vars = decltype(Restart_Vars){};
  Restart_Data = decltype(Restart_Data){};

  /*--- Detect a wrong solution file ---*/

  if (iPoint_Global_Local != nPointDomain) {
    SU2_MPI::Error(string("The solution file ") + filename + string(" doesn't match with the mesh file!\n") +
                   string("It could be empty lines at the end of the file."), CURRENT_FUNCTION);
  }
}

void CSolver::SavelibROM(CGeometry *geometry, CConfig *config, bool converged) {
  SU2_ZONE_SCOPED

#if defined(HAVE_LIBROM) && !defined(CODI_FORWARD_TYPE) && !defined(CODI_REVERSE_TYPE)
  const bool unsteady            = config->GetTime_Domain();
  const string filename          = config->GetlibROMbase_FileName();
  const unsigned long TimeIter   = config->GetTimeIter();
  const unsigned long nTimeIter  = config->GetnTime_Iter();
  const int maxBasisDim          = config->GetMax_BasisDim();
  const int save_freq            = config->GetRom_SaveFreq();
  int dim = int(nPointDomain * nVar);
  bool incremental = false;

  if (!u_basis_generator) {

    /*--- Define SVD basis generator ---*/
    auto timesteps = static_cast<int>(nTimeIter - TimeIter);
    CAROM::Options svd_options = CAROM::Options(dim, timesteps, -1,
                                                false, true).setMaxBasisDimension(int(maxBasisDim));

    if (config->GetKind_PODBasis() == POD_KIND::STATIC) {
      if (rank == MASTER_NODE) std::cout << "Creating static basis generator." << std::endl;

      if (unsteady) {
        if (rank == MASTER_NODE) std::cout << "Incremental basis generator recommended for unsteady simulations." << std::endl;
      }
    }
    else {
      if (rank == MASTER_NODE) std::cout << "Creating incremental basis generator." << std::endl;

      svd_options.setIncrementalSVD(1.0e-3, config->GetDelta_UnstTime(),
                                    1.0e-2, config->GetDelta_UnstTime()*nTimeIter, true).setDebugMode(false);
      incremental = true;
    }

    u_basis_generator.reset(new CAROM::BasisGenerator(
      svd_options, incremental,
      filename));

    // Save mesh ordering
    std::ofstream f;
    f.open(filename + "_mesh_" + to_string(rank) + ".csv");
      for (unsigned long iPoint = 0; iPoint < nPointDomain; iPoint++) {
        unsigned long globalPoint = geometry->nodes->GetGlobalIndex(iPoint);
        auto Coord = geometry->nodes->GetCoord(iPoint);

        for (unsigned long iDim; iDim < nDim; iDim++) {
          f << Coord[iDim] << ", ";
        }
        f << globalPoint << "\n";
      }
    f.close();
  }

  if (unsteady && (TimeIter % save_freq == 0)) {
    // give solution and time steps to libROM:
    su2double dt = config->GetDelta_UnstTime();
    su2double t =  config->GetCurrent_UnstTime();
    u_basis_generator->takeSample(const_cast<su2double*>(base_nodes->GetSolution().data()), t, dt);
  }

  /*--- End collection of data and save POD ---*/

  if (converged) {

    if (!unsteady) {
       // dt is different for each node, so just use a placeholder dt
       su2double dt = base_nodes->GetDelta_Time(0);
       su2double t = dt*TimeIter;
       u_basis_generator->takeSample(const_cast<su2double*>(base_nodes->GetSolution().data()), t, dt);
    }

    if (config->GetKind_PODBasis() == POD_KIND::STATIC) {
      u_basis_generator->writeSnapshot();
    }

    if (rank == MASTER_NODE) std::cout << "Computing SVD" << std::endl;
    int rom_dim = u_basis_generator->getSpatialBasis()->numColumns();

    if (rank == MASTER_NODE) std::cout << "Basis dimension: " << rom_dim << std::endl;
    u_basis_generator->endSamples();

    if (rank == MASTER_NODE) std::cout << "ROM Sampling ended" << std::endl;
  }

#else
  SU2_MPI::Error("SU2 was not compiled with libROM support.", CURRENT_FUNCTION);
#endif

}
