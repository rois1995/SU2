CNumericsSIMD* createNumerics(const CConfig& config, int iMesh, const CVariable* turbVars) {
  CNumericsSIMD* obj = nullptr;
  const bool ideal_gas = (config.GetKind_FluidModel() == STANDARD_AIR) ||
                         (config.GetKind_FluidModel() == IDEAL_GAS);

  switch (config.GetKind_ConvNumScheme_Flow()) {
    case SPACE_UPWIND:
      if (config.GetViscous()) {
        if (ideal_gas)
          obj = createUpwindIdealNumerics<CCompressibleViscousFlux<nDim> >(config, iMesh, turbVars);
        else
          obj = createUpwindGeneralNumerics<CGeneralCompressibleViscousFlux<nDim> >(config, iMesh, turbVars);
      }
      else {
        if (ideal_gas)
          obj = createUpwindIdealNumerics<CNoViscousFlux<nDim> >(config, iMesh, turbVars);
        else
          obj = createUpwindGeneralNumerics<CNoViscousFlux<nDim> >(config, iMesh, turbVars);
      }
      break;

    case SPACE_CENTERED:
      if (config.GetViscous()) {
        if (ideal_gas)
          obj = createCenteredNumerics<CCompressibleViscousFlux<nDim> >(config, iMesh, turbVars);
        else
          obj = createCenteredNumerics<CGeneralCompressibleViscousFlux<nDim> >(config, iMesh, turbVars);
      }
      else {
        obj = createCenteredNumerics<CNoViscousFlux<nDim> >(config, iMesh, turbVars);
      }
      break;
    default:
    break;
  }

  return obj;
}
