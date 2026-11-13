FORCEINLINE CPair<ReconVarType> reconstructPrimitives(const Int& iEdge,
                                                      const Int& iPoint, const Int& jPoint,
                                                      const su2double& gamma,
                                                      const su2double& gasConst,
                                                      const bool muscl,
                                                      const su2double& kappa,
                                                      const su2double& umusclRamp,
                                                      const LIMITER limiterType,
                                                      const CPair<PrimVarType>& V1st,
                                                      const VectorDbl<nDim>& vector_ij,
                                                      const VariableType& solution,
                                                      Double& nonPhysical) {
  static_assert(ReconVarType::nVar <= PrimVarType::nVar);

  const auto& gradients = solution.GetGradient_Reconstruction();
  const auto& limiters = solution.GetLimiter_Primitive();

  CPair<ReconVarType> V;

  for (size_t iVar = 0; iVar < ReconVarType::nVar; ++iVar) {
    V.i.all(iVar) = V1st.i.all(iVar);
    V.j.all(iVar) = V1st.j.all(iVar);
  }

  if (muscl) {
    /*--- Reconstruct density and enthalpy without using their gradients. ---*/
    constexpr auto nVarGrad = ReconVarType::nVar - 2;
    reconstruct<nVarGrad>(iPoint, jPoint, vector_ij, gradients, limiters, limiterType, 0, V, kappa, umusclRamp);
    /*--- Recompute density using the reconstructed pressure and temperature. ---*/
    V.i.density() = V.i.pressure() / (gasConst * V.i.temperature());
    V.j.density() = V.j.pressure() / (gasConst * V.j.temperature());

    /*--- Reconstruct enthalpy using dH/dT = Cp and dH/dv = v. Recomputing enthalpy would cause
     * stability issues because we use rho E = rho H - P, which loses its relation to temperature
     * if both rho and H are recomputed. This only seems to be an issue for wall-function meshes.
     * NOTE: This "one-sided" reconstruction does not lose much of the U-MUSCL benefit, because
     * the static enthalpy is linear, and for the KE term we do the equivalent of using the
     * average dH/dv, which is exact for quadratic functions. ---*/
    const su2double cp = gasConst * gamma / (gamma - 1);
    V.i.enthalpy() += cp * (V.i.temperature() - V1st.i.temperature());
    V.j.enthalpy() += cp * (V.j.temperature() - V1st.j.temperature());
    for (size_t iDim = 0; iDim < nDim; ++iDim) {
      V.i.enthalpy() += 0.5 * (pow(V.i.velocity(iDim), 2) - pow(V1st.i.velocity(iDim), 2));
      V.j.enthalpy() += 0.5 * (pow(V.j.velocity(iDim), 2) - pow(V1st.j.velocity(iDim), 2));
    }

    /*--- Detect a non-physical reconstruction based on negative pressure or density. ---*/
    const Double neg_p_or_rho = fmax(fmin(V.i.pressure(), V.j.pressure()) < 0.0,
                                     fmin(V.i.density(), V.j.density()) < 0.0);
    /*--- Test the sign of the Roe-averaged speed of sound. ---*/
    const Double R = sqrt(abs(V.j.density() / V.i.density()));
    /*--- Delay dividing by R+1 until comparing enthalpy and velocity magnitude. ---*/
    const Double enthalpy = R*V.j.enthalpy() + V.i.enthalpy();
    Double v_squared = 0.0;
    for (size_t iDim = 0; iDim < nDim; ++iDim) {
      v_squared += pow(R*V.j.velocity(iDim) + V.i.velocity(iDim), 2);
    }
    /*--- Multiply enthalpy by R+1 since v^2 was not divided by (R+1)^2.
     * Note: a = sqrt((gamma-1) * (H - 0.5 * v^2)) ---*/
    const Double neg_sound_speed = enthalpy * (R+1) < 0.5 * v_squared;

    /*--- Revert to first order if the state is non-physical. ---*/
    nonPhysical = fmax(neg_p_or_rho, neg_sound_speed);
    /*--- Handle SIMD dimensions 1 by 1. ---*/
    for (size_t k = 0; k < Double::Size; ++k) {
      nonPhysical[k] = solution.UpdateNonPhysicalEdgeCounter(iEdge[k], nonPhysical[k]);
      nonPhysical[k] = fmax(nonPhysical[k],
          fmax(solution.OutlierMitigation(iPoint[k]),
               solution.OutlierMitigation(jPoint[k])) / VariableType::MAX_OUTLIER_MITIGATION);
    }
    for (size_t iVar = 0; iVar < ReconVarType::nVar; ++iVar) {
      V.i.all(iVar) = nonPhysical * V1st.i.all(iVar) + (1-nonPhysical) * V.i.all(iVar);
      V.j.all(iVar) = nonPhysical * V1st.j.all(iVar) + (1-nonPhysical) * V.j.all(iVar);
    }
  } else {
    nonPhysical = 0;
  }
  return V;
}
