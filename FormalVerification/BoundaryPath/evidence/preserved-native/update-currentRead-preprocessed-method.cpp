inline void AddSolution(unsigned long iPoint, unsigned long iVar, su2double solution) {
    Solution(iPoint, iVar) = Solution(iPoint, iVar) + solution;
  }
