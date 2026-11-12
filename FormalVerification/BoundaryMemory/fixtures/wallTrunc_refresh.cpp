normalRes = 0.0;
    su2double* Res_TruncError = nodes->GetResTruncError(iPoint);
    for (auto iDim = 0u; iDim < nDim; iDim++) {
      normalRes += Res_TruncError[iVel + iDim] * UnitNormal[iDim];
    }
    for (auto iDim = 0u; iDim < nDim; iDim++) {
      normalRes = 0.0; for (auto jDim = 0u; jDim < nDim; jDim++) { normalRes += Res_TruncError[iVel + jDim] * UnitNormal[jDim]; }
      Res_TruncError[iVel + iDim] -= normalRes * UnitNormal[iDim];
    }
