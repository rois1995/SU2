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
        if (rans){
          CVariable* turbNodes = solver_container[TURB_SOL]->GetNodes();
          numerics->SetTurbKineticEnergy(turbNodes->GetSolution(iPoint,0), turbNodes->GetSolution(iPoint,0));
        }
      }

      /*--- Compute Source term Residual ---*/
      auto residual = numerics->ComputeResidual(config);

      /*--- Add Residual ---*/
      LinSysRes.AddBlock(iPoint, residual);

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
