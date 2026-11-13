unsigned long CEulerSolver::SetPrimitive_Variables(CSolver **solver_container, const CConfig *config) {
  SU2_ZONE_SCOPED

  /*--- Number of non-physical points, local to the thread, needs
   *    further reduction if function is called in parallel ---*/
  unsigned long nonPhysicalPoints = 0;

  AD::StartNoSharedReading();

  SU2_OMP_FOR_STAT(omp_chunk_size)
  for (unsigned long iPoint = 0; iPoint < nPoint; ++iPoint) {

    /*--- Compressible flow, primitive variables nDim+9, (T, vx, vy, vz, P, rho, h, c, lamMu, eddyMu, ThCond, Cp) ---*/

    bool physical = nodes->SetPrimVar(iPoint, GetFluidModel());
    nodes->SetSecondaryVar(iPoint, GetFluidModel());

    /* Check for non-realizable states for reporting. */

    if (!physical) nonPhysicalPoints++;
  }
  END_SU2_OMP_FOR

  AD::EndNoSharedReading();

  return nonPhysicalPoints;
}
