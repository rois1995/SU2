import SU2Source
open SU2Source
set_option maxRecDepth 100000
def selected : Program := parsedProgram "inline void AddSolution(unsigned long iPoint, unsigned long iVar, su2double solution) {\n    Solution(iPoint, iVar) = Solution(iPoint, iVar) + solution;\n  }" (by decide)
example : WitnessContract selected := by
  refine ⟨Function.update witnessMemory (0, 0) (some 5), ?_, ?_⟩
  · rfl
  · exact store_contract integerOperations witnessMemory (0, 0) (1, 0) 2 3 rfl
