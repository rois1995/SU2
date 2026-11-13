CNumericsSIMD* createUpwindIdealNumerics(const CConfig& config, int iMesh, const CVariable* turbVars) {
  CNumericsSIMD* obj = nullptr;
  switch (config.GetKind_Upwind_Flow()) {
    case UPWIND::ROE:
      obj = new CRoeScheme<ViscousDecorator>(config, iMesh, turbVars);
      break;
    case UPWIND::MSW:
      obj = new CMSWScheme<ViscousDecorator>(config, iMesh, turbVars);
      break;
    default:
      break;
  }
  return obj;
}
