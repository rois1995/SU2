normalRes = 0.0;
    su2double* Res_TruncError = nodes->GetResTruncError(iPoint);
    for (auto iDim = 0u; iDim < nDim; iDim++) {
      normalRes += Res_TruncError[iVel + iDim] * UnitNormal[iDim];
    }
    for (auto iDim = 0u; iDim < nDim; iDim++) {
      Res_TruncError[iVel + iDim] -= normalRes * UnitNormal[iDim];
    }
