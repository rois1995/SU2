  inline su2double GetEnergy(unsigned long iPoint) const final { return Solution(iPoint,nVar-1)/Solution(iPoint,0); }
