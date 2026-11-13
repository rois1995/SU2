void CMultiGridIntegration::PreSmoothing(unsigned short RunTime_EqSystem,
                                         CGeometry**** geometry,
                                         CSolver***** solver_container,
                                         CConfig **config_container,
                                         CSolver* solver_fine,
                                         CNumerics** numerics_fine,
                                         CGeometry* geometry_fine,
                                         CSolver** solver_container_fine,
                                         CConfig *config,
                                         unsigned short iMesh,
                                         unsigned short iZone,
                                         unsigned short iRKLimit) {
  SU2_ZONE_SCOPED

  const auto& mgOpts = config->GetMGOptions();
  const bool classical_rk4 = (config->GetKind_TimeIntScheme() == CLASSICAL_RK4_EXPLICIT);
  const unsigned short nPreSmooth = mgOpts.MG_PreSmooth[iMesh];
  const unsigned long timeIter = config->GetTimeIter();
  const bool early_exit = mgOpts.MG_Smooth_EarlyExit && (nPreSmooth > 1);
  const bool need_per_step_rms = early_exit || mgOpts.MG_Smooth_Output;
  /*--- Also capture initial RMS at MESH_0 for the cross-cycle EMA controller, even with nPreSmooth==1. ---*/
  const bool need_initial_rms = need_per_step_rms || (iMesh == MESH_0 && mgOpts.MG_Smooth_EarlyExit);
  const passivedouble stag_tol = (mgOpts.MG_Smooth_StagnationTol > 0.0)
                                 ? SU2_TYPE::GetValue(mgOpts.MG_Smooth_StagnationTol) : passivedouble(1.0);

  /*--- Reset the shared early-exit flag (master only). ---*/
  SU2_OMP_SAFE_GLOBAL_ACCESS(mg_early_exit_flag = false;)
  for (unsigned short iPreSmooth = 0; iPreSmooth < nPreSmooth; iPreSmooth++) {

    /*--- Time and space integration ---*/
    for (unsigned short iRKStep = 0; iRKStep < iRKLimit; iRKStep++) {

      /*--- Send-Receive boundary conditions, and preprocessing ---*/
      solver_fine->Preprocessing(geometry_fine, solver_container_fine, config, iMesh, iRKStep, RunTime_EqSystem, false);

      if (iRKStep == 0) {

        /*--- Set the old solution ---*/
        solver_fine->Set_OldSolution();

        if (classical_rk4) solver_fine->Set_NewSolution();
        solver_fine->SetTime_Step(geometry_fine, solver_container_fine, config, iMesh, timeIter);
        Adjoint_Setup(geometry, solver_container, config_container, RunTime_EqSystem, timeIter, iZone);
      }

      /*--- Space integration ---*/
      Space_Integration(geometry_fine, solver_container_fine, numerics_fine, config, iMesh, iRKStep, RunTime_EqSystem);

      /*--- Time integration, update solution using the old solution plus the solution increment ---*/
      Time_Integration(geometry_fine, solver_container_fine, config, iRKStep, RunTime_EqSystem);

      /*--- At iRKStep==0 LinSysRes = R(u_k). ---*/
      if (iRKStep == 0 && need_initial_rms) {
        BEGIN_SU2_OMP_SAFE_GLOBAL_ACCESS {
          const passivedouble defect = ComputeLinSysResRMS(solver_fine);
          prePostEarlyExit(iPreSmooth, iMesh, defect, mgOpts, stag_tol, early_exit,
                           lastPreSmoothRMS[iMesh], lastPreSmoothExitReason[iMesh],
                           lastPreSmoothWorstStepRatio[iMesh], lastPreSmoothWorstStep[iMesh]);
        }
        END_SU2_OMP_SAFE_GLOBAL_ACCESS
        if (mg_early_exit_flag) break;
      }


      /*--- Send-Receive boundary conditions, and postprocessing ---*/
      solver_fine->Postprocessing(geometry_fine, solver_container_fine, config, iMesh);
    }
    SU2_OMP_SAFE_GLOBAL_ACCESS(lastPreSmoothIters[iMesh] = iPreSmooth + 1;)
    if (mg_early_exit_flag) break;
  }

  /*--- Record the final pre-smooth defect; MultiGrid_Cycle overwrites it with the exact
   *    d_N on non-coarsest levels. ---*/
  if (nPreSmooth > 0) {
    BEGIN_SU2_OMP_SAFE_GLOBAL_ACCESS {
      lastPreSmoothRMS[iMesh][1] = mg_prev_smooth_rms;
    }
    END_SU2_OMP_SAFE_GLOBAL_ACCESS
  }
}
