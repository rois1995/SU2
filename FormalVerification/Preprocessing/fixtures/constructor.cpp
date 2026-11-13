su2double val_solution[5] = {su2double(1.0), velocity[0], velocity[1], energy, energy};
  if(nDim==3) val_solution[3] = velocity[2];

  for(unsigned long iPoint = 0; iPoint < nPoint; ++iPoint)
    for (unsigned long iVar = 0; iVar < nVar; ++iVar)
      Solution(iPoint,iVar) = density*val_solution[iVar];

  Solution_Old = Solution;
