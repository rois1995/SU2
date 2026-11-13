CNumericsSIMD* CNumericsSIMD::CreateNumerics(const CConfig& config, int nDim, int iMesh, const CVariable* turbVars) {
#ifndef CODI_REVERSE_TYPE
  if ((Double::Size < 4) && (SU2_MPI::GetRank() == MASTER_NODE)) {
    cout << "WARNING: SU2 was not compiled for an AVX-capable architecture. Performance could be better,\n"
            "         see https://su2code.github.io/docs_v7/Build-SU2-Linux-MacOS/#compiler-optimizations" << endl;
  }
#endif
  if (nDim == 2) return createNumerics<2>(config, iMesh, turbVars);
  if (nDim == 3) return createNumerics<3>(config, iMesh, turbVars);

  return nullptr;
}
