su2double normalRes = 0.0;
    for (auto iDim = 0u; iDim < nDim; iDim++) {
      normalRes += LinSysRes(iPoint, iVel + iDim) * UnitNormal[iDim];
    }
    for (auto iDim = 0u; iDim < nDim; iDim++) {
      LinSysRes(iPoint, iVel + iDim) -= normalRes * UnitNormal[iDim];
    }
