FORCEINLINE void SetTime_Step_impl(const SoundSpeedFunc& soundSpeed,
                                     const LambdaViscFunc& lambdaVisc,
                                     CGeometry *geometry,
                                     CSolver **solver_container,
                                     CConfig *config,
                                     unsigned short iMesh,
                                     unsigned long Iteration) {

    const bool viscous       = config->GetViscous();
    const bool implicit      = (config->GetKind_TimeIntScheme() == EULER_IMPLICIT);
    const bool time_stepping = (config->GetTime_Marching() == TIME_MARCHING::TIME_STEPPING);
    const bool dual_time     = (config->GetTime_Marching() == TIME_MARCHING::DT_STEPPING_1ST) ||
                               (config->GetTime_Marching() == TIME_MARCHING::DT_STEPPING_2ND);
    const su2double K_v = 0.25;

    /*--- Init thread-shared variables to compute min/max values.
     *    Critical sections are used for this instead of reduction
     *    clauses for compatibility with OpenMP 2.0 (Windows...). ---*/

    ompMasterAssignBarrier(Min_Delta_Time,1e30, Max_Delta_Time,0.0, Global_Delta_UnstTimeND,1e30);

    /*--- Loop domain points. ---*/

    SU2_OMP_FOR_DYN(omp_chunk_size)
    for (auto iPoint = 0ul; iPoint < nPointDomain; ++iPoint) {

      /*--- Set maximum eigenvalues to zero. ---*/

      nodes->SetMax_Lambda_Inv(iPoint,0.0);

      if (viscous)
        nodes->SetMax_Lambda_Visc(iPoint,0.0);

      /*--- Loop over the neighbors of point i. ---*/

      for (unsigned short iNeigh = 0; iNeigh < geometry->nodes->GetnPoint(iPoint); ++iNeigh)
      {
        auto jPoint = geometry->nodes->GetPoint(iPoint,iNeigh);

        auto iEdge = geometry->nodes->GetEdge(iPoint,iNeigh);
        auto Normal = geometry->edges->GetNormal(iEdge);
        auto Area2 = GeometryToolbox::SquaredNorm(nDim, Normal);

        /*--- Mean Values ---*/

        su2double Mean_ProjVel = 0.5 * (nodes->GetProjVel(iPoint,Normal) + nodes->GetProjVel(jPoint,Normal));
        su2double Mean_SoundSpeed = soundSpeed(*nodes, iPoint, jPoint) * sqrt(Area2);

        /*--- Adjustment for grid movement ---*/

        if (dynamic_grid) {
          const su2double *GridVel_i = geometry->nodes->GetGridVel(iPoint);
          const su2double *GridVel_j = geometry->nodes->GetGridVel(jPoint);

          for (unsigned short iDim = 0; iDim < nDim; iDim++)
            Mean_ProjVel -= 0.5 * (GridVel_i[iDim] + GridVel_j[iDim]) * Normal[iDim];
        }

        /*--- Inviscid contribution ---*/

        su2double Lambda = fabs(Mean_ProjVel) + Mean_SoundSpeed;
        nodes->AddMax_Lambda_Inv(iPoint,Lambda);

        /*--- Viscous contribution ---*/

        if (!viscous) continue;

        Lambda = lambdaVisc(*nodes, iPoint, jPoint) * Area2;
        nodes->AddMax_Lambda_Visc(iPoint, Lambda);
      }

    }
    END_SU2_OMP_FOR

    /*--- Loop boundary edges ---*/

    for (unsigned short iMarker = 0; iMarker < geometry->GetnMarker(); iMarker++) {
      if ((config->GetMarker_All_KindBC(iMarker) != INTERNAL_BOUNDARY) &&
          (config->GetMarker_All_KindBC(iMarker) != SYMMETRY_PLANE) &&
          (config->GetMarker_All_KindBC(iMarker) != EULER_WALL) &&
          (config->GetMarker_All_KindBC(iMarker) != NEARFIELD_BOUNDARY) &&
          (config->GetMarker_All_KindBC(iMarker) != PERIODIC_BOUNDARY)) {

        SU2_OMP_FOR_STAT(OMP_MIN_SIZE)
        for (auto iVertex = 0ul; iVertex < geometry->GetnVertex(iMarker); iVertex++) {

          /*--- Point identification, Normal vector and area ---*/

          auto iPoint = geometry->vertex[iMarker][iVertex]->GetNode();

          if (!geometry->nodes->GetDomain(iPoint)) continue;

          auto Normal = geometry->vertex[iMarker][iVertex]->GetNormal();
          auto Area2 = GeometryToolbox::SquaredNorm(nDim, Normal);

          /*--- Mean Values ---*/

          su2double ProjVel = nodes->GetProjVel(iPoint,Normal);
          su2double SoundSpeed = soundSpeed(*nodes, iPoint) * sqrt(Area2);

          /*--- Adjustment for grid movement ---*/

          if (dynamic_grid) {
            ProjVel -= GeometryToolbox::DotProduct(nDim, Normal, geometry->nodes->GetGridVel(iPoint));
          }

          /*--- Inviscid contribution ---*/

          su2double Lambda = fabs(ProjVel) + SoundSpeed;
          nodes->AddMax_Lambda_Inv(iPoint, Lambda);

          /*--- Viscous contribution ---*/

          if (!viscous) continue;

          Lambda = lambdaVisc(*nodes,iPoint) * Area2;
          nodes->AddMax_Lambda_Visc(iPoint, Lambda);
        }
        END_SU2_OMP_FOR
      }
    }

    /*--- Each element uses their own speed, steady state simulation. ---*/
    {
      /*--- Thread-local variables for min/max reduction. ---*/
      su2double minDt = 1e30, maxDt = 0.0;

      SU2_OMP_FOR_(schedule(static,omp_chunk_size) SU2_NOWAIT)
      for (auto iPoint = 0ul; iPoint < nPointDomain; iPoint++) {

        su2double Vol = geometry->nodes->GetVolume(iPoint);

        if (Vol != 0.0) {
          su2double Local_Delta_Time = nodes->GetLocalCFL(iPoint)*Vol / nodes->GetMax_Lambda_Inv(iPoint);

          if(viscous) {
            su2double dt_visc = nodes->GetLocalCFL(iPoint)*K_v*Vol*Vol / nodes->GetMax_Lambda_Visc(iPoint);
            Local_Delta_Time = min(Local_Delta_Time, dt_visc);
          }

          minDt = min(minDt, Local_Delta_Time);
          maxDt = max(maxDt, Local_Delta_Time);

          nodes->SetDelta_Time(iPoint, min(Local_Delta_Time, config->GetMax_DeltaTime()));
        }
        else {
          nodes->SetDelta_Time(iPoint,0.0);
        }
      }
      END_SU2_OMP_FOR

      /*--- Min/max over threads. ---*/
      atomicMin(minDt, Min_Delta_Time);
      atomicMax(maxDt, Max_Delta_Time);
    }

    /*--- Compute the min/max dt (in parallel, now over mpi ranks). ---*/

    BEGIN_SU2_OMP_SAFE_GLOBAL_ACCESS {
      if (config->GetComm_Level() == COMM_FULL) {
        su2double rbuf_time;
        SU2_MPI::Allreduce(&Min_Delta_Time, &rbuf_time, 1, MPI_DOUBLE, MPI_MIN, SU2_MPI::GetComm());
        Min_Delta_Time = rbuf_time;

        SU2_MPI::Allreduce(&Max_Delta_Time, &rbuf_time, 1, MPI_DOUBLE, MPI_MAX, SU2_MPI::GetComm());
        Max_Delta_Time = rbuf_time;
      }
      Global_Delta_Time = Min_Delta_Time;
    } END_SU2_OMP_SAFE_GLOBAL_ACCESS

    /*--- For exact time solution use the minimum delta time of the whole mesh. ---*/
    if (time_stepping) {

      /*--- If the unsteady CFL is set to zero, it uses the defined unsteady time step,
       *    otherwise it computes the time step based on the unsteady CFL. ---*/

      BEGIN_SU2_OMP_SAFE_GLOBAL_ACCESS
      {
        if (config->GetUnst_CFL() == 0.0) {
          Global_Delta_Time = config->GetDelta_UnstTime();
        }
        else {
          Global_Delta_Time = Min_Delta_Time;
        }
        Max_Delta_Time = Global_Delta_Time;

        config->SetDelta_UnstTimeND(Global_Delta_Time);
      }
      END_SU2_OMP_SAFE_GLOBAL_ACCESS

      /*--- Sets the regular CFL equal to the unsteady CFL. ---*/

      SU2_OMP_FOR_STAT(omp_chunk_size)
      for (auto iPoint = 0ul; iPoint < nPointDomain; iPoint++) {
        nodes->SetLocalCFL(iPoint, config->GetUnst_CFL());
        nodes->SetDelta_Time(iPoint, Global_Delta_Time);
      }
      END_SU2_OMP_FOR

    }

    /*--- Recompute the unsteady time step for the dual time strategy if the unsteady CFL is different from 0.
     * This is only done once because in dual time the time step cannot be variable. ---*/

    if (dual_time && (Iteration == config->GetRestart_Iter()) && (config->GetUnst_CFL() != 0.0) && (iMesh == MESH_0)) {

      /*--- Thread-local variable for reduction. ---*/
      su2double glbDtND = 1e30;

      SU2_OMP_FOR_(schedule(static,omp_chunk_size) SU2_NOWAIT)
      for (auto iPoint = 0ul; iPoint < nPointDomain; iPoint++) {
        glbDtND = min(glbDtND, config->GetUnst_CFL()*Global_Delta_Time / nodes->GetLocalCFL(iPoint));
      }
      END_SU2_OMP_FOR
      atomicMin(glbDtND, Global_Delta_UnstTimeND);

      BEGIN_SU2_OMP_SAFE_GLOBAL_ACCESS
      {
        SU2_MPI::Allreduce(&Global_Delta_UnstTimeND, &glbDtND, 1, MPI_DOUBLE, MPI_MIN, SU2_MPI::GetComm());
        Global_Delta_UnstTimeND = glbDtND;

        config->SetDelta_UnstTimeND(Global_Delta_UnstTimeND);
      }
      END_SU2_OMP_SAFE_GLOBAL_ACCESS
    }

    /*--- The pseudo local time (explicit integration) cannot be greater than the physical time ---*/

    if (dual_time && !implicit) {
      SU2_OMP_FOR_STAT(omp_chunk_size)
      for (auto iPoint = 0ul; iPoint < nPointDomain; iPoint++) {
        su2double dt = min((2.0/3.0)*config->GetDelta_UnstTimeND(), nodes->GetDelta_Time(iPoint));
        nodes->SetDelta_Time(iPoint, dt);
      }
      END_SU2_OMP_FOR
    }
  }
