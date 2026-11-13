inline su2double GetProjVel(unsigned long iPoint, const su2double *val_vector) const final {
    su2double ProjVel = 0.0;
    for (unsigned long iDim = 0; iDim < nDim; iDim++)
      ProjVel += Primitive(iPoint, iDim+indices.Velocity())*val_vector[iDim];
    return ProjVel;
  }
