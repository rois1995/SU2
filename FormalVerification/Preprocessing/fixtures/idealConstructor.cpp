CIdealGas::CIdealGas(su2double gamma, su2double R, bool CompEntropy) : CFluidModel() {
  Gamma = gamma;
  Gamma_Minus_One = Gamma - 1.0;
  Gas_Constant = R;
  Cp = Gamma / Gamma_Minus_One * Gas_Constant;
  Cv = Cp - R;

  dTdrho_e = 0.0;
  dTde_rho = Gamma_Minus_One / Gas_Constant;

  ComputeEntropy = CompEntropy;
}
