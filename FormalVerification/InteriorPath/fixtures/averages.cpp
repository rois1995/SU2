FORCEINLINE CRoeVariables<nDim> roeAveragedVariables(const Double& gamma,
                                                     const CPair<PrimVarType>& V,
                                                     const VectorDbl<nDim>& normal) {
  CRoeVariables<nDim> roeAvg;
  Double R = sqrt(V.j.density() / V.i.density());
  Double D = 1 / (R+1);
  roeAvg.density = R * V.i.density();
  for (size_t iDim = 0; iDim < nDim; ++iDim) {
    roeAvg.velocity(iDim) = (R*V.j.velocity(iDim) + V.i.velocity(iDim)) * D;
  }
  roeAvg.enthalpy = (R*V.j.enthalpy() + V.i.enthalpy()) * D;
  roeAvg.speedSound = sqrt((gamma-1) * (roeAvg.enthalpy - 0.5*squaredNorm(roeAvg.velocity)));
  roeAvg.projVel = dot(roeAvg.velocity, normal);
  return roeAvg;
}
