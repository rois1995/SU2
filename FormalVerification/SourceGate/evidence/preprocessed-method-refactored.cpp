inline void AddSolution(unsigned long point, unsigned long component, su2double increment) {
    const su2double previous = Solution_Old(point, component);
    Solution(point, component) = previous + increment;
  }
