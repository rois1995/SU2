FORCEINLINE MatrixDbl<nDim+2> pMatrix(const Double& gamma, const Double& density, const RandomAccessIterator& velocity,
                                      const Double& projVel, const Double& speedSound, const VectorDbl<nDim>& normal) {
  MatrixDbl<nDim+2> pMat;
  const Double vel2 = 0.5*squaredNorm<nDim>(velocity);

  if (nDim == 2) {
    pMat(0,0) = 1.0;
    pMat(0,1) = 0.0;

    pMat(1,0) = velocity[0];
    pMat(1,1) = density*normal(1);

    pMat(2,0) = velocity[1];
    pMat(2,1) = -density*normal(0);

    pMat(3,0) = vel2;
    pMat(3,1) = density*(velocity[0]*normal(1) - velocity[1]*normal(0));
  }
  else {
    pMat(0,0) = normal(0);
    pMat(0,1) = normal(1);
    pMat(0,2) = normal(2);

    pMat(1,0) = velocity[0]*normal(0);
    pMat(1,1) = velocity[0]*normal(1) - density*normal(2);
    pMat(1,2) = velocity[0]*normal(2) + density*normal(1);

    pMat(2,0) = velocity[1]*normal(0) + density*normal(2);
    pMat(2,1) = velocity[1]*normal(1);
    pMat(2,2) = velocity[1]*normal(2) - density*normal(0);

    pMat(3,0) = velocity[2]*normal(0) - density*normal(1);
    pMat(3,1) = velocity[2]*normal(1) + density*normal(0);
    pMat(3,2) = velocity[2]*normal(2);

    pMat(4,0) = vel2*normal(0) + density*(velocity[1]*normal(2) - velocity[2]*normal(1));
    pMat(4,1) = vel2*normal(1) - density*(velocity[0]*normal(2) - velocity[2]*normal(0));
    pMat(4,2) = vel2*normal(2) + density*(velocity[0]*normal(1) - velocity[1]*normal(0));
  }

  /*--- Last two columns. ---*/

  const Double rhoOn2 = 0.5*density;
  const Double rhoOnTwoC = rhoOn2 / speedSound;
  pMat(0,nDim) = rhoOnTwoC;
  pMat(0,nDim+1) = rhoOnTwoC;

  for (size_t iDim = 0; iDim < nDim; ++iDim) {
    pMat(iDim+1,nDim) = rhoOnTwoC * velocity[iDim] + rhoOn2 * normal(iDim);
    pMat(iDim+1,nDim+1) = rhoOnTwoC * velocity[iDim] - rhoOn2 * normal(iDim);
  }

  pMat(nDim+1,nDim) = rhoOnTwoC * vel2 + rhoOn2 * (projVel + speedSound/(gamma-1));
  pMat(nDim+1,nDim+1) = rhoOnTwoC * vel2 - rhoOn2 * (projVel - speedSound/(gamma-1));

  return pMat;
}
