FORCEINLINE MatrixDbl<nDim+2> pMatrixInv(const Double& gamma, const Double& density,
                                         const RandomAccessIterator& velocity, const Double& projVel,
                                         const Double& speedSound, const VectorDbl<nDim>& normal) {
  MatrixDbl<nDim+2> pMatInv;

  const Double c2 = pow(speedSound,2);
  const Double vel2 = 0.5*squaredNorm<nDim>(velocity);
  const Double oneOnRho = 1 / density;

  if (nDim == 2) {
    Double tmp = (gamma-1)/c2;
    pMatInv(0,0) = 1.0 - tmp*vel2;
    pMatInv(0,1) = tmp*velocity[0];
    pMatInv(0,2) = tmp*velocity[1];
    pMatInv(0,3) = -tmp;

    pMatInv(1,0) = (normal(0)*velocity[1]-normal(1)*velocity[0])*oneOnRho;
    pMatInv(1,1) = normal(1)*oneOnRho;
    pMatInv(1,2) = -normal(0)*oneOnRho;
    pMatInv(1,3) = 0.0;
  }
  else {
    Double tmp = (gamma-1)/c2 * normal(0);
    pMatInv(0,0) = normal(0) - tmp*vel2 - (normal(2)*velocity[1]-normal(1)*velocity[2])*oneOnRho;
    pMatInv(0,1) = tmp*velocity[0];
    pMatInv(0,2) = tmp*velocity[1] + normal(2)*oneOnRho;
    pMatInv(0,3) = tmp*velocity[2] - normal(1)*oneOnRho;
    pMatInv(0,4) = -tmp;

    tmp = (gamma-1)/c2 * normal(1);
    pMatInv(1,0) = normal(1) - tmp*vel2 + (normal(2)*velocity[0]-normal(0)*velocity[2])*oneOnRho;
    pMatInv(1,1) = tmp*velocity[0] - normal(2)*oneOnRho;
    pMatInv(1,2) = tmp*velocity[1];
    pMatInv(1,3) = tmp*velocity[2] + normal(0)*oneOnRho;
    pMatInv(1,4) = -tmp;

    tmp = (gamma-1)/c2 * normal(2);
    pMatInv(2,0) = normal(2) - tmp*vel2 - (normal(1)*velocity[0]-normal(0)*velocity[1])*oneOnRho;
    pMatInv(2,1) = tmp*velocity[0] + normal(1)*oneOnRho;
    pMatInv(2,2) = tmp*velocity[1] - normal(0)*oneOnRho;
    pMatInv(2,3) = tmp*velocity[2];
    pMatInv(2,4) = -tmp;
  }

  /*--- Last two rows. ---*/

  const Double gamma_minus_1_on_rho_times_c = (gamma-1) / (density*speedSound);

  for (size_t iVar = nDim; iVar < nDim+2; ++iVar) {
    Double sign = (iVar==nDim)? 1 : -1;
    pMatInv(iVar,0) = -sign*projVel*oneOnRho + gamma_minus_1_on_rho_times_c * vel2;
    for (size_t iDim = 0; iDim < nDim; ++iDim) {
      pMatInv(iVar,iDim+1) = sign*normal(iDim)*oneOnRho - gamma_minus_1_on_rho_times_c * velocity[iDim];
    }
    pMatInv(iVar,nDim+1) = gamma_minus_1_on_rho_times_c;
  }

  return pMatInv;
}
