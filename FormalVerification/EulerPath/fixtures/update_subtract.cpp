inline void AddSolution(unsigned long iPoint, unsigned long iVar, su2double solution) {
    Solution(iPoint, iVar) = Solution_Old(iPoint, iVar) - solution;
  }
