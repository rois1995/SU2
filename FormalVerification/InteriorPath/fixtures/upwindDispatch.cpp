void CEulerSolver::Upwind_Residual(CGeometry *geometry, CSolver **solver_container,
                                   CNumerics **numerics_container, CConfig *config, unsigned short iMesh) {
  SU2_ZONE_SCOPED

  const bool ideal_gas = (config->GetKind_FluidModel() == STANDARD_AIR) ||
                         (config->GetKind_FluidModel() == IDEAL_GAS);
  const bool low_mach_corr = config->Low_Mach_Correction();

  /*--- Use vectorization if the scheme supports it. ---*/
  if (ideal_gas && !low_mach_corr) {
    if (config->GetKind_Upwind_Flow() == UPWIND::ROE || config->GetKind_Upwind_Flow() == UPWIND::MSW) {
      EdgeFluxResidual(geometry, solver_container, config);
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

      /*--- Recompute the reconstructed quantities in a thermodynamically consistent way. ---*/

      if (!ideal_gas || low_mach_corr) {
        ComputeConsistentExtrapolation(GetFluidModel(), nDim, Primitive_i, Secondary_i);
        ComputeConsistentExtrapolation(GetFluidModel(), nDim, Primitive_j, Secondary_j);
      }

      /*--- Low-Mach number correction. ---*/

      if (low_mach_corr) {
        LowMachPrimitiveCorrection(GetFluidModel(), nDim, Primitive_i, Primitive_j);
      }

      /*--- Check for non-physical solutions after reconstruction. If found, use the
       cell-average value of the solution. This is a locally 1st order approximation,
       which is typically only active during the start-up of a calculation. ---*/

      bool neg_pres_or_rho_i = (Primitive_i[prim_idx.Pressure()] < 0.0) || (Primitive_i[prim_idx.Density()] < 0.0);
      bool neg_pres_or_rho_j = (Primitive_j[prim_idx.Pressure()] < 0.0) || (Primitive_j[prim_idx.Density()] < 0.0);

      su2double R = sqrt(fabs(Primitive_j[prim_idx.Density()]/Primitive_i[prim_idx.Density()]));
      su2double sq_vel = 0.0;
      for (auto iDim = 0u; iDim < nDim; iDim++) {
        su2double RoeVelocity = (R * Primitive_j[iDim + prim_idx.Velocity()] +
                                 Primitive_i[iDim + prim_idx.Velocity()]) / (R+1);
        sq_vel += pow(RoeVelocity, 2);
      }
      su2double RoeEnthalpy = (R * Primitive_j[prim_idx.Enthalpy()] + Primitive_i[prim_idx.Enthalpy()]) / (R+1);

      const bool neg_sound_speed = ((Gamma-1)*(RoeEnthalpy-0.5*sq_vel) < 0.0);
      bool bad_recon = neg_sound_speed || neg_pres_or_rho_i || neg_pres_or_rho_j;
      bad_recon = nodes->UpdateNonPhysicalEdgeCounter(iEdge, bad_recon);
      counter_local += bad_recon;

      numerics->SetPrimitive(bad_recon? V_i : Primitive_i,  bad_recon? V_j : Primitive_j);
      numerics->SetSecondary(bad_recon? S_i : Secondary_i,  bad_recon? S_j : Secondary_j);

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
