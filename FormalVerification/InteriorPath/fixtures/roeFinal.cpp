FORCEINLINE void finalizeFlux(VectorDbl<nVar>& flux,
                                MatrixDbl<nVar>& jac_i,
                                MatrixDbl<nVar>& jac_j,
                                const bool implicit,
                                const Double& area,
                                const VectorDbl<nDim>& unitNormal,
                                const VectorDbl<nDim>& normal,
                                const CPair<PrimVarType>& V,
                                const CPair<ConsVarType>& U,
                                const Int& iPoint,
                                const Int& jPoint,
                                const Double& nonPhysical,
                                const CEulerVariable& solution,
                                const CGeometry& geometry,
                                Ts&...) const {
    /*--- Roe averaged variables. ---*/

    auto roeAvg = roeAveragedVariables(gamma, V, unitNormal);

    /*--- Grid motion. ---*/

    Double projGridVel = 0.0, projVel = roeAvg.projVel;
    if (dynamicGrid) {
      const auto& gridVel = geometry.nodes->GetGridVel();
      projGridVel = 0.5*(dot(gatherVariables<nDim>(iPoint,gridVel), unitNormal)+
                         dot(gatherVariables<nDim>(jPoint,gridVel), unitNormal));
      projVel -= projGridVel;
    }

    /*--- Convective eigenvalues. ---*/

    VectorDbl<nVar> lambda;
    for (size_t iDim = 0; iDim < nDim; ++iDim) {
      lambda(iDim) = projVel;
    }
    lambda(nDim) = projVel + roeAvg.speedSound;
    lambda(nDim+1) = projVel - roeAvg.speedSound;

    /*--- Apply Mavriplis' entropy correction to eigenvalues. ---*/

    Double lambdaMin = fmax(entropyFix, nonPhysical) * (abs(projVel) + roeAvg.speedSound);
    for (size_t iVar = 0; iVar < nVar; ++iVar) {
      lambda(iVar) = fmax(abs(lambda(iVar)), lambdaMin);
    }

    /*--- Inviscid fluxes and Jacobians. ---*/

    auto flux_i = inviscidProjFlux(V.i, U.i, normal);
    auto flux_j = inviscidProjFlux(V.j, U.j, normal);

    for (size_t iVar = 0; iVar < nVar; ++iVar) {
      flux(iVar) = 0.5 * (flux_i(iVar) + flux_j(iVar));
    }
    if (implicit) {
      jac_i = inviscidProjJac(gamma, V.i.velocity(), U.i.energy(), normal, kappa);
      jac_j = inviscidProjJac(gamma, V.j.velocity(), U.j.energy(), normal, kappa);
    }

    /*--- Correct for grid motion. ---*/

    if (dynamicGrid) {
      for (size_t iVar = 0; iVar < nVar; ++iVar) {
        Double dFdU = projGridVel * area * 0.5;
        flux(iVar) -= dFdU * (U.i.all(iVar) + U.j.all(iVar));

        if (implicit) {
          jac_i(iVar,iVar) -= dFdU;
          jac_j(iVar,iVar) -= dFdU;
        }
      }
    }

    /*--- P tensor. ---*/

    auto pMat = pMatrix(gamma, roeAvg.density, roeAvg.velocity,
                        roeAvg.projVel, roeAvg.speedSound, unitNormal);

    /*--- Inverse P tensor. ---*/

    auto pMatInv = pMatrixInv(gamma, roeAvg.density, roeAvg.velocity,
                              roeAvg.projVel, roeAvg.speedSound, unitNormal);

    /*--- Diference between conservative variables at jPoint and iPoint. ---*/

    VectorDbl<nVar> deltaU;
    for (size_t iVar = 0; iVar < nVar; ++iVar) {
      deltaU(iVar) = U.j.all(iVar) - U.i.all(iVar);
    }

    /*--- Dissipation terms. ---*/

    Double dissipation = roeDissipation(iPoint, jPoint, typeDissip, solution);

    for (size_t iVar = 0; iVar < nVar; ++iVar) {
      for (size_t jVar = 0; jVar < nVar; ++jVar) {
        /*--- Compute |projModJacTensor| = P x |Lambda| x P^-1. ---*/

        Double projModJacTensor = 0.0;
        for (size_t kVar = 0; kVar < nVar; ++kVar) {
          projModJacTensor += pMat(iVar,kVar) * lambda(kVar) * pMatInv(kVar,jVar);
        }

        Double dDdU = projModJacTensor * (1-kappa) * area * dissipation;

        /*--- Update flux and Jacobians. ---*/

        flux(iVar) -= dDdU * deltaU(jVar);

        if (implicit) {
          jac_i(iVar,jVar) += dDdU;
          jac_j(iVar,jVar) -= dDdU;
        }
      }
    }
  }
