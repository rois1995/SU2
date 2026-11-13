  inline void SetVelocity(unsigned long iPoint) final {
    Velocity2(iPoint) = 0.0;
    for (unsigned long iDim = 0; iDim < nDim; iDim++) {
      Primitive(iPoint,iDim+indices.Velocity()) = Solution(iPoint,iDim+1) / Solution(iPoint,0);
      Velocity2(iPoint) += pow(Primitive(iPoint,iDim+indices.Velocity()),2);
    }
  }
