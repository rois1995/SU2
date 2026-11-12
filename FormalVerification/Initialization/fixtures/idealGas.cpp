void CIdealGas::SetTDState_rhoe(su2double rho, su2double e) {
  Density = rho;
  StaticEnergy = e;
  dPdrho_e = Gamma_Minus_One * StaticEnergy;
  dPde_rho = Gamma_Minus_One * Density;
  Pressure = dPde_rho * StaticEnergy;
  Temperature = dTde_rho * StaticEnergy;
  SoundSpeed2 = Gamma * Gas_Constant * Temperature;

  if (ComputeEntropy) Entropy = Cv * log(Temperature) - Gas_Constant * log(Density);
}
