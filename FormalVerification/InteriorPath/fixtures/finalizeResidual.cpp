void FinalizeResidualComputation(const CGeometry *geometry, bool pausePreacc,
                                          unsigned long localCounter, CConfig* config) {
    SU2_ZONE_SCOPED

    /*--- Restore preaccumulation and adjoint evaluation state. ---*/
    AD::ResumePreaccumulation(pausePreacc);
    if (!ReducerStrategy) AD::EndNoSharedReading();

    if (ReducerStrategy) {
      SumEdgeFluxes(geometry);
      if (config->GetKind_TimeIntScheme() == EULER_IMPLICIT) {
        Jacobian.SetDiagonalAsColumnSum();
      }
    }

    /*--- Warning message about non-physical reconstructions. ---*/
    if ((MGLevel == MESH_0) && (config->GetComm_Level() == COMM_FULL)) {
      /*--- Add counter results for all threads. ---*/
      atomicAdd(localCounter, ErrorCounter);

      /*--- Add counter results for all ranks. ---*/
      BEGIN_SU2_OMP_SAFE_GLOBAL_ACCESS {
        localCounter = ErrorCounter;
        SU2_MPI::Reduce(&localCounter, &ErrorCounter, 1, MPI_UNSIGNED_LONG, MPI_SUM, MASTER_NODE, SU2_MPI::GetComm());
        config->SetNonphysical_Reconstr(ErrorCounter);
      }
      END_SU2_OMP_SAFE_GLOBAL_ACCESS
    }
  }
