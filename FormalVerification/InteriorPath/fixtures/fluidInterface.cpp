void CFVMFlowSolverBase<V, FlowRegime>::BC_Fluid_Interface(CGeometry* geometry, CSolver** solver_container,
                                                           CNumerics* conv_numerics, CNumerics* visc_numerics,
                                                           CConfig* config) {
  SU2_ZONE_SCOPED

  const bool ideal_gas = config->GetKind_FluidModel() == STANDARD_AIR || config->GetKind_FluidModel() == IDEAL_GAS;

  unsigned long iVertex, jVertex, iPoint, Point_Normal = 0;
  unsigned short iDim, iVar, jVar, iMarker, nDonorVertex;

  bool implicit = (config->GetKind_TimeIntScheme() == EULER_IMPLICIT);
  bool viscous = config->GetViscous();

  su2double Normal[MAXNDIM] = {0.0};
  su2double PrimVar_i[MAXNVAR] = {0.0};
  su2double PrimVar_j[MAXNVAR] = {0.0};
  su2double Secondary_j[MAXNVAR] = {0.0};
  su2double Residual[MAXNVAR] = {0.0};
  su2double** Jacobian_i = new su2double*[nVar];
  for (iVar = 0; iVar < nVar; iVar++) Jacobian_i[iVar] = new su2double[nVar];

  su2double weight;
  su2double P_static, rho_static;

  for (iMarker = 0; iMarker < config->GetnMarker_All(); iMarker++) {
    if (config->GetMarker_All_KindBC(iMarker) == FLUID_INTERFACE) {
      SU2_OMP_FOR_DYN(OMP_MIN_SIZE)
      for (iVertex = 0; iVertex < geometry->nVertex[iMarker]; iVertex++) {
        iPoint = geometry->vertex[iMarker][iVertex]->GetNode();

        if (geometry->nodes->GetDomain(iPoint)) {
          nDonorVertex = GetnSlidingStates(iMarker, iVertex);

          /*--- Initialize Residual, this will serve to accumulate the average ---*/

          for (iVar = 0; iVar < nVar; iVar++) {
            Residual[iVar] = 0.0;
            for (jVar = 0; jVar < nVar; jVar++) Jacobian_i[iVar][jVar] = 0.0;
          }

          /*--- Loop over the nDonorVertexes and compute the averaged flux ---*/

          for (jVertex = 0; jVertex < nDonorVertex; jVertex++) {
            Point_Normal = geometry->vertex[iMarker][iVertex]->GetNormal_Neighbor();

            for (iVar = 0; iVar < nPrimVar; iVar++) {
              PrimVar_i[iVar] = nodes->GetPrimitive(iPoint, iVar);
              PrimVar_j[iVar] = GetSlidingState(iMarker, iVertex, iVar, jVertex);
            }

            /*--- Get the weight computed in the interpolator class for the j-th donor vertex ---*/

            weight = GetSlidingState(iMarker, iVertex, nPrimVar, jVertex);

            /*--- Set primitive variables ---*/

            conv_numerics->SetPrimitive(PrimVar_i, PrimVar_j);

            if (FlowRegime == ENUM_REGIME::COMPRESSIBLE && !ideal_gas) {
              P_static = PrimVar_j[nDim + 1];
              rho_static = PrimVar_j[nDim + 2];
              GetFluidModel()->SetTDState_Prho(P_static, rho_static);

              Secondary_j[0] = GetFluidModel()->GetdPdrho_e();
              Secondary_j[1] = GetFluidModel()->GetdPde_rho();

              conv_numerics->SetSecondary(nodes->GetSecondary(iPoint), Secondary_j);
            }

            /*--- Set the normal vector ---*/

            geometry->vertex[iMarker][iVertex]->GetNormal(Normal);
            for (iDim = 0; iDim < nDim; iDim++) Normal[iDim] = -Normal[iDim];

            conv_numerics->SetNormal(Normal);

            if (dynamic_grid)
              conv_numerics->SetGridVel(geometry->nodes->GetGridVel(iPoint), geometry->nodes->GetGridVel(iPoint));

            /*--- Compute the convective residual using an upwind scheme ---*/

            auto residual = conv_numerics->ComputeResidual(config);

            /*--- Accumulate the residuals to compute the average ---*/

            for (iVar = 0; iVar < nVar; iVar++) {
              Residual[iVar] += weight * residual[iVar];
              for (jVar = 0; jVar < nVar; jVar++) Jacobian_i[iVar][jVar] += weight * residual.jacobian_i[iVar][jVar];
            }
          }

          /*--- Add Residuals and Jacobians ---*/

          LinSysRes.AddBlock(iPoint, Residual);

          if (implicit) Jacobian.AddBlock2Diag(iPoint, Jacobian_i);

          if (viscous) {
            /*--- Initialize Residual, this will serve to accumulate the average ---*/

            for (iVar = 0; iVar < nVar; iVar++) {
              Residual[iVar] = 0.0;
              for (jVar = 0; jVar < nVar; jVar++) Jacobian_i[iVar][jVar] = 0.0;
            }

            /*--- Loop over the nDonorVertexes and compute the averaged flux ---*/

            for (jVertex = 0; jVertex < nDonorVertex; jVertex++) {
              PrimVar_j[nDim + 5] = GetSlidingState(iMarker, iVertex, nDim + 5, jVertex);
              PrimVar_j[nDim + 6] = GetSlidingState(iMarker, iVertex, nDim + 6, jVertex);

              /*--- Get the weight computed in the interpolator class for the j-th donor vertex ---*/

              weight = GetSlidingState(iMarker, iVertex, nPrimVar, jVertex);

              /*--- Set the normal vector and the coordinates ---*/

              visc_numerics->SetNormal(Normal);
              su2double Coord_Reflected[MAXNDIM];
              GeometryToolbox::PointPointReflect(nDim, geometry->nodes->GetCoord(Point_Normal),
                                                        geometry->nodes->GetCoord(iPoint), Coord_Reflected);
              visc_numerics->SetCoord(geometry->nodes->GetCoord(iPoint), Coord_Reflected);

              /*--- Primitive variables, and gradient ---*/

              visc_numerics->SetPrimitive(PrimVar_i, PrimVar_j);
              visc_numerics->SetPrimVarGradient(nodes->GetGradient_Primitive(iPoint),
                                                nodes->GetGradient_Primitive(iPoint));

              /*--- Turbulent kinetic energy ---*/

              if (config->GetKind_Turb_Model() == TURB_MODEL::SST)
                visc_numerics->SetTurbKineticEnergy(solver_container[TURB_SOL]->GetNodes()->GetSolution(iPoint, 0),
                                                    solver_container[TURB_SOL]->GetNodes()->GetSolution(iPoint, 0));

              /*--- Compute and update residual ---*/

              auto residual = visc_numerics->ComputeResidual(config);

              /*--- Accumulate the residuals to compute the average ---*/

              for (iVar = 0; iVar < nVar; iVar++) {
                Residual[iVar] += weight * residual[iVar];
                for (jVar = 0; jVar < nVar; jVar++) Jacobian_i[iVar][jVar] += weight * residual.jacobian_i[iVar][jVar];
              }
            }

            LinSysRes.SubtractBlock(iPoint, Residual);

            /*--- Jacobian contribution for implicit integration ---*/

            if (implicit) Jacobian.SubtractBlock2Diag(iPoint, Jacobian_i);
          }
        }
      }
      END_SU2_OMP_FOR
    }
  }

  for (iVar = 0; iVar < nVar; iVar++) delete[] Jacobian_i[iVar];
  delete[] Jacobian_i;
}
