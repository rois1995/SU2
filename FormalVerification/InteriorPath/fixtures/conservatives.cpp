FORCEINLINE CCompressibleConservatives<nDim> compressibleConservatives(const CCompressiblePrimitives<nDim,N>& V) {
  CCompressibleConservatives<nDim> U;
  U.density() = V.density();
  for (size_t iDim = 0; iDim < nDim; ++iDim) {
    U.momentum(iDim) = V.density() * V.velocity(iDim);
  }
  U.rhoEnergy() = V.density() * V.enthalpy() - V.pressure();
  return U;
}
