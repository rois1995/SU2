void CFVMFlowSolverBase<V, R>::EdgeFluxResidual(const CGeometry *geometry,
                                                const CSolver* const* solvers,
                                                CConfig *config) {
  SU2_ZONE_SCOPED

  if (!edgeNumerics) {
    if (!ReducerStrategy && (omp_get_max_threads() > 1) &&
        (config->GetEdgeColoringGroupSize() % Double::Size != 0)) {
      SU2_MPI::Error("When using vectorization, the EDGE_COLORING_GROUP_SIZE must be divisible "
                     "by the SIMD length (2, 4, or 8).", CURRENT_FUNCTION);
    }
    InstantiateEdgeNumerics(solvers, config);

    /*--- The SIMD numerics do not use gradients of density and enthalpy. ---*/
    if (!config->GetContinuous_Adjoint()) {
      SU2_OMP_SAFE_GLOBAL_ACCESS(nPrimVarGrad = std::min<unsigned short>(nDim + 2, nPrimVarGrad);)
    }
  }

  /*--- Non-physical counter. ---*/
  unsigned long counterLocal = 0;
  SU2_OMP_MASTER
  ErrorCounter = 0;
  END_SU2_OMP_MASTER

  su2activevector* massFluxes = config->GetBounded_Scalar() ? &EdgeMassFluxes : nullptr;

  /*--- For hybrid parallel AD, pause preaccumulation if there is shared reading of
  * variables, otherwise switch to the faster adjoint evaluation mode. ---*/
  bool pausePreacc = false;
  if (ReducerStrategy) pausePreacc = AD::PausePreaccumulation();
  else AD::StartNoSharedReading();

  /*--- Loop over edge colors. ---*/
  for (auto color : EdgeColoring) {
    /*--- Chunk size is at least OMP_MIN_SIZE and a multiple of the color group size. ---*/
    SU2_OMP_FOR_DYN(nextMultiple(OMP_MIN_SIZE, color.groupSize))
    for(auto k = 0ul; k < color.size; k += Double::Size) {
      Int iEdge;
      Double mask;
      for (auto j = 0ul; j < Double::Size; ++j) {
        bool in = (k+j < color.size);
        mask[j] = in;
        iEdge[j] = color.indices[k+j*in];
      }

      if (ReducerStrategy) {
        edgeNumerics->ComputeFlux(iEdge, *config, *geometry, *nodes, UpdateType::REDUCTION, mask, EdgeFluxes, Jacobian, massFluxes);
      } else {
        edgeNumerics->ComputeFlux(iEdge, *config, *geometry, *nodes, UpdateType::COLORING, mask, LinSysRes, Jacobian, massFluxes);
      }
      if (MGLevel == MESH_0) {
        for (auto j = 0ul; j < Double::Size; ++j)
          counterLocal += (nodes->NonPhysicalEdgeCounter[iEdge[j]] > 0);
      }
    }
    END_SU2_OMP_FOR
  }

  FinalizeResidualComputation(geometry, pausePreacc, counterLocal, config);
}
