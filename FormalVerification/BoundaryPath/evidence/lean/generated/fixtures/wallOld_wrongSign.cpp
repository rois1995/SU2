su2double vp = 0.0;
    for (auto iDim = 0u; iDim < nDim; iDim++) {
      vp += (solutionOld[iVel + iDim] - gridVel[iDim]) * UnitNormal[iDim];
    }
    for (auto iDim = 0u; iDim < nDim; iDim++) {
      solutionOld[iVel + iDim] += vp * UnitNormal[iDim];
    }
