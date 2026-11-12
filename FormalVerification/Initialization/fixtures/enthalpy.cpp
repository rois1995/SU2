  inline void SetEnthalpy(unsigned long iPoint) final {
    Primitive(iPoint, indices.Enthalpy()) =
      (Solution(iPoint,nVar-1) + Primitive(iPoint, indices.Pressure())) / Solution(iPoint,0);
  }
