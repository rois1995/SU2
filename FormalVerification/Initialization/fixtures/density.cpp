  inline bool SetDensity(unsigned long iPoint) final {
    Primitive(iPoint, indices.Density()) = Solution(iPoint,0);
    return Primitive(iPoint, indices.Density()) <= 0.0;
  }
