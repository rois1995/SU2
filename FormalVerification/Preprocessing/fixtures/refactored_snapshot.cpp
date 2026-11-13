void CVariable::Set_OldSolution() {
  assert(Solution_Old.size() == Solution.size());
  parallelCopy(Solution.size(), Solution.data(), Solution_Old.data());
}
