void CEulerVariable::SetSecondaryVar(unsigned long iPoint, CFluidModel *FluidModel) {
  if (nSecondaryVar == 0) return;

  /*--- Compute secondary thermo-physical properties (partial derivatives...) ---*/

  SetdPdrho_e(iPoint, FluidModel->GetdPdrho_e());
  SetdPde_rho(iPoint, FluidModel->GetdPde_rho());

}
