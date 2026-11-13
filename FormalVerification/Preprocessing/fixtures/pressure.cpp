  inline bool SetPressure(unsigned long iPoint, su2double pressure) final {
    Primitive(iPoint,nDim+1) = pressure;
    return pressure <= 0.0;
  }
