su2double normalRes = 0.0;
    for (auto iDim = 0u; iDim < nDim; iDim++) {
      normalRes += LinSysRes(iPoint, iVel + iDim) * UnitNormal[iDim];
    }
    for (auto iDim = 0u; iDim < nDim; iDim++) {
      normalRes = 0.0; for (auto jDim = 0u; jDim < nDim; jDim++) { normalRes += LinSysRes(iPoint, iVel + jDim) * UnitNormal[jDim]; }
      LinSysRes(iPoint, iVel + iDim) -= normalRes * UnitNormal[iDim];
    }
