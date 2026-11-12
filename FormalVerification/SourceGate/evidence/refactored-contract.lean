import SU2Source
open SU2Source
set_option maxRecDepth 100000
def selected : Program := parsedProgram "inline void AddSolution(unsigned long point, unsigned long component, su2double increment) {\n    const su2double previous = Solution_Old(point, component);\n    Solution(point, component) = previous + increment;\n  }" (by decide)
example : WitnessContract selected := by
  refine ⟨Function.update witnessMemory (0, 0) (some 5), ?_, ?_⟩
  · rfl
  · exact store_contract integerOperations witnessMemory (0, 0) (1, 0) 2 3 rfl
