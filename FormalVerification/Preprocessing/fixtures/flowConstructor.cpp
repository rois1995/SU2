CFlowVariable::CFlowVariable(unsigned long npoint, unsigned long ndim, unsigned long nvar, unsigned long nprimvar,
                             unsigned long nprimvargrad, const CConfig* config)
    : CVariable(npoint, ndim, nvar, config),
      Gradient_Reconstruction(config->GetReconstructionGradientRequired() ? Gradient_Aux : Gradient_Primitive) {
  nPrimVar = nprimvar;
  nPrimVarGrad = nprimvargrad;

  /*--- Allocate residual structures for multigrid. ---*/

  Res_TruncError.resize(nPoint, nVar) = su2double(0.0);

  for (unsigned long iMesh = 0; iMesh <= config->GetnMGLevels(); iMesh++) {
    if (config->GetMGOptions().MG_CorrecSmooth[iMesh] > 0) {
      Residual_Sum.resize(nPoint, nVar);
      Residual_Old.resize(nPoint, nVar);
      break;
    }
  }

  /*--- Primitive variables and gradients (see derived classes for what is stored each column) ---*/

  Primitive.resize(nPoint, nPrimVar) = su2double(0.0);

  if (config->GetMUSCL_Flow() || config->GetViscous() || config->GetContinuous_Adjoint() || config->GetKind_Incomp_System() == INCOMP_SYSTEM::PRESSURE_BASED) {
    Gradient_Primitive.resize(nPoint, nPrimVarGrad, nDim, 0.0);
  }

  if (config->GetReconstructionGradientRequired() && config->GetKind_ConvNumScheme_Flow() != SPACE_CENTERED) {
    Gradient_Aux.resize(nPoint, nPrimVarGrad, nDim, 0.0);
  }

  if (config->GetLeastSquaresRequired()) {
    Rmatrix.resize(nPoint, nDim, nDim, 0.0);
  }

  /*--- Allocate undivided laplacian (centered) ---*/

  if (config->GetKind_ConvNumScheme_Flow() == SPACE_CENTERED) {
    Undivided_Laplacian.resize(nPoint, nVar);
  }

  /*--- Allocate the slope limiter (MUSCL upwind) ---*/

  if (config->GetKind_SlopeLimit_Flow() != LIMITER::NONE && config->GetKind_SlopeLimit_Flow() != LIMITER::VAN_ALBADA_EDGE) {
    Limiter_Primitive.resize(nPoint, nPrimVarGrad) = su2double(0.0);
    Solution_Max.resize(nPoint, nPrimVarGrad) = su2double(0.0);
    Solution_Min.resize(nPoint, nPrimVarGrad) = su2double(0.0);
  }

  Velocity2.resize(nPoint) = su2double(0.0);
  Max_Lambda_Inv.resize(nPoint) = su2double(0.0);
  Delta_Time.resize(nPoint) = su2double(0.0);
  Lambda.resize(nPoint) = su2double(0.0);
  Sensor.resize(nPoint) = su2double(0.0);

  /* Under-relaxation parameter. */
  UnderRelaxation.resize(nPoint) = su2double(1.0);
  LocalCFL.resize(nPoint) = su2double(0.0);

  /* Non-physical point (first-order) initialization. */
  Non_Physical.resize(nPoint) = false;
  Non_Physical_Counter.resize(nPoint) = 0;

  if (config->GetMultizone_Problem()) {
    Set_BGSSolution_k();
  }

  if (config->GetTime_Marching() == TIME_MARCHING::HARMONIC_BALANCE) {
    HB_Source.resize(nPoint, nVar) = su2double(0.0);
  }
}
