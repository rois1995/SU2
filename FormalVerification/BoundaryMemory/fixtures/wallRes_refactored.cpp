su2double normalRes = 0.0;
    for (auto iDim = 0u; iDim < nDim; iDim++) {
      normalRes += LinSysRes(iPoint, iVel + iDim) * UnitNormal[iDim];
    }
    for (auto iDim = nDim; iDim-- > 0;) {
      LinSysRes(iPoint, iVel + iDim) -= normalRes * UnitNormal[iDim];
    }
