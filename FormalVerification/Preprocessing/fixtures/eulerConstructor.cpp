CEulerVariable::CEulerVariable(su2double density, const su2double *velocity, su2double energy, unsigned long npoint,
                               unsigned long ndim, unsigned long nvar, const CConfig *config)
  : CFlowVariable(npoint, ndim, nvar, ndim + 9, EulerNPrimVarGrad(config, ndim), config),
    indices(ndim, 0) {

  const bool dual_time = (config->GetTime_Marching() == TIME_MARCHING::DT_STEPPING_1ST) ||
                         (config->GetTime_Marching() == TIME_MARCHING::DT_STEPPING_2ND);
  const bool classical_rk4 = (config->GetKind_TimeIntScheme_Flow() == CLASSICAL_RK4_EXPLICIT);

  nSecondaryVar = EulerNSecVar(config);

  /*--- Solution initialization ---*/

  su2double val_solution[5] = {su2double(1.0), velocity[0], velocity[1], energy, energy};
  if(nDim==3) val_solution[3] = velocity[2];

  for(unsigned long iPoint = 0; iPoint < nPoint; ++iPoint)
    for (unsigned long iVar = 0; iVar < nVar; ++iVar)
      Solution(iPoint,iVar) = density*val_solution[iVar];

  Solution_Old = Solution;

  if (classical_rk4) Solution_New = Solution;

  /*--- Allocate and initializate solution for dual time strategy ---*/

  if (dual_time) {
    Solution_time_n = Solution;
    Solution_time_n1 = Solution;
  }

  Secondary.resize(nPoint,nSecondaryVar) = su2double(0.0);

  if (config->GetAxisymmetric()){
    nAuxVar = 3;
    Grad_AuxVar.resize(nPoint,nAuxVar,nDim,0.0);
    AuxVar.resize(nPoint,nAuxVar) = su2double(0.0);
  }

  if (config->GetWind_Gust()) {
    WindGust.resize(nPoint,nDim);
  }

  if (config->GetVorticityConfinement()) {
    nAuxVar = 1;
    Grad_AuxVar.resize(nPoint, nAuxVar, nDim, 0.0);
    AuxVar.resize(nPoint, nAuxVar) = su2double(0.0);
  }

  if (config->GetKind_FluidModel() == ENUM_FLUIDMODEL::DATADRIVEN_FLUID){
    DataDrivenFluid = true;
    DatasetExtrapolation.resize(nPoint) = 0;
    NIterNewtonsolver.resize(nPoint) = 0;
    FluidEntropy.resize(nPoint) = su2double(0.0);
  }

  OutlierMitigation.resize(nPoint) = 0;
}
