  void ComputeFlux(const Int iEdge,
                   const CConfig& config,
                   const CGeometry& geometry,
                   const CVariable& solution_,
                   const UpdateType updateType,
                   const Double updateMask,
                   CSysVector<su2double>& vector,
                   SparseMatrixType& matrix,
                   su2activevector* edgeMassFluxes) const final {

    /*--- Start preaccumulation, inputs are registered
     *    automatically in "gatherVariables". ---*/
    AD::StartPreacc();

    const bool implicit = (config.GetKind_TimeIntScheme() == EULER_IMPLICIT);
    const su2double umusclRamp = config.GetMUSCLRampValue() * config.GetNewtonKrylovRelaxation();
    const auto& solution = static_cast<const CEulerVariable&>(solution_);

    const auto iPoint = geometry.edges->GetNode(iEdge,0);
    const auto jPoint = geometry.edges->GetNode(iEdge,1);

    /*--- Geometric properties. ---*/

    const auto vector_ij = distanceVector<nDim>(iPoint, jPoint, geometry.nodes->GetCoord());

    const auto normal = gatherVariables<nDim>(iEdge, geometry.edges->GetNormal());
    const auto area = norm(normal);
    VectorDbl<nDim> unitNormal;
    for (size_t iDim = 0; iDim < nDim; ++iDim) {
      unitNormal(iDim) = normal(iDim) / area;
    }

    /*--- Reconstructed primitives. ---*/

    CPair<CCompressiblePrimitives<nDim,nPrimVar> > V1st;
    V1st.i.all = gatherVariables<nPrimVar>(iPoint, solution.GetPrimitive());
    V1st.j.all = gatherVariables<nPrimVar>(jPoint, solution.GetPrimitive());

    /*--- Recompute density and enthalpy instead of reconstructing. ---*/
    Double nonPhysical;
    auto V = reconstructPrimitives<CCompressiblePrimitives<nDim,nPrimVarGrad> >(
        iEdge, iPoint, jPoint, gamma, gasConst, muscl, umusclKappa, umusclRamp,
        typeLimiter, V1st, vector_ij, solution, nonPhysical);

    /*--- Compute conservative variables. ---*/

    CPair<CCompressibleConservatives<nDim> > U;
    U.i = compressibleConservatives(V.i);
    U.j = compressibleConservatives(V.j);

    /*--- Finalize in derived class (static polymorphism). ---*/

    const auto derived = static_cast<const Derived*>(this);
    VectorDbl<nVar> flux;
    MatrixDbl<nVar> jac_i, jac_j;
    derived->finalizeFlux(flux, jac_i, jac_j, implicit, area, unitNormal, normal,
                          V, U, iPoint, jPoint, nonPhysical, solution, geometry);

    /*--- Add the contributions from the base class (static decorator). ---*/

    Base::viscousTerms(iEdge, iPoint, jPoint, V1st, solution_, vector_ij, geometry,
                       config, area, unitNormal, implicit, flux, jac_i, jac_j);

    /*--- Stop preaccumulation. ---*/

    stopPreacc(flux);

    /*--- Update the vector and system matrix. ---*/

    updateLinearSystem(iEdge, iPoint, jPoint, implicit, updateType,
                       updateMask, flux, jac_i, jac_j, vector, matrix);

    /*--- Store the mass flux (density-equation flux) for bounded-scalar transport. ---*/

    updateEdgeMassFlux(iEdge, flux(0), edgeMassFluxes);
  }
