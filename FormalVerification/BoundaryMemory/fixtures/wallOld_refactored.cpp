su2double vp = 0.0;
    for (auto iDim = 0u; iDim < nDim; iDim++) {
      vp += (solutionOld[iVel + iDim] - gridVel[iDim]) * UnitNormal[iDim];
    }
    for (auto iDim = nDim; iDim-- > 0;) {
      solutionOld[iVel + iDim] -= vp * UnitNormal[iDim];
    }
