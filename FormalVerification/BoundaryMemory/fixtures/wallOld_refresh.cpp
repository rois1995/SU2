su2double vp = 0.0;
    for (auto iDim = 0u; iDim < nDim; iDim++) {
      vp += (solutionOld[iVel + iDim] - gridVel[iDim]) * UnitNormal[iDim];
    }
    for (auto iDim = 0u; iDim < nDim; iDim++) {
      vp = 0.0; for (auto jDim = 0u; jDim < nDim; jDim++) { vp += (solutionOld[iVel + jDim] - gridVel[jDim]) * UnitNormal[jDim]; }
      solutionOld[iVel + iDim] -= vp * UnitNormal[iDim];
    }
