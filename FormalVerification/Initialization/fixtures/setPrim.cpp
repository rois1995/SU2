bool CEulerVariable::SetPrimVar(unsigned long iPoint, CFluidModel *FluidModel) {

  bool RightVol = true;

  SetVelocity(iPoint);   // Computes velocity and velocity^2
  su2double density      = GetDensity(iPoint);
  su2double staticEnergy = GetEnergy(iPoint)-0.5*Velocity2(iPoint);

  /*--- Check will be moved inside fluid model plus error description strings ---*/

  FluidModel->SetTDState_rhoe(density, staticEnergy);

  bool check_dens  = SetDensity(iPoint);
  bool check_press = SetPressure(iPoint, FluidModel->GetPressure());
  bool check_sos   = SetSoundSpeed(iPoint, FluidModel->GetSoundSpeed2());
  bool check_temp  = SetTemperature(iPoint, FluidModel->GetTemperature());

  /*--- Check that the solution has a physical meaning ---*/

  if (check_dens || check_press || check_sos || check_temp) {

    /*--- Copy the old solution ---*/

    for (unsigned long iVar = 0; iVar < nVar; iVar++)
      Solution(iPoint, iVar) = Solution_Old(iPoint, iVar);

    /*--- Recompute the primitive variables ---*/

    SetVelocity(iPoint);   // Computes velocity and velocity^2
    su2double density = GetDensity(iPoint);
    su2double staticEnergy = GetEnergy(iPoint)-0.5*Velocity2(iPoint);
    /* check will be moved inside fluid model plus error description strings*/
    FluidModel->SetTDState_rhoe(density, staticEnergy);

    SetDensity(iPoint);
    SetPressure(iPoint, FluidModel->GetPressure());
    SetSoundSpeed(iPoint, FluidModel->GetSoundSpeed2());
    SetTemperature(iPoint, FluidModel->GetTemperature());

    RightVol = false;

  }

  SetEnthalpy(iPoint); // Requires pressure computation.

  /*--- Set look-up variables in case of data-driven fluid model ---*/
  if (DataDrivenFluid) {
    SetDataExtrapolation(iPoint, FluidModel->GetExtrapolation());
    SetEntropy(iPoint, FluidModel->GetEntropy());
  }

  return RightVol;
}
