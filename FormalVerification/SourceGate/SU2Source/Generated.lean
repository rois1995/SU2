import SU2Source.Parser

set_option maxRecDepth 100000
set_option maxHeartbeats 800000

namespace SU2Source.Generated

def originalSource : String := "inline void AddSolution(unsigned long iPoint, unsigned long iVar, su2double solution) {\n    Solution(iPoint, iVar) = Solution_Old(iPoint, iVar) + solution;\n  }"
def originalProgram : Program := parsedProgram originalSource (by decide)
theorem original_parsed : parse originalSource = some originalProgram := by decide

def subtractSource : String := "inline void AddSolution(unsigned long iPoint, unsigned long iVar, su2double solution) {\n    Solution(iPoint, iVar) = Solution_Old(iPoint, iVar) - solution;\n  }"
def subtractProgram : Program := parsedProgram subtractSource (by decide)
theorem subtract_parsed : parse subtractSource = some subtractProgram := by decide

def currentReadSource : String := "inline void AddSolution(unsigned long iPoint, unsigned long iVar, su2double solution) {\n    Solution(iPoint, iVar) = Solution(iPoint, iVar) + solution;\n  }"
def currentReadProgram : Program := parsedProgram currentReadSource (by decide)
theorem currentRead_parsed : parse currentReadSource = some currentReadProgram := by decide

def wrongTargetSource : String := "inline void AddSolution(unsigned long iPoint, unsigned long iVar, su2double solution) {\n    Solution_Old(iPoint, iVar) = Solution_Old(iPoint, iVar) + solution;\n  }"
def wrongTargetProgram : Program := parsedProgram wrongTargetSource (by decide)
theorem wrongTarget_parsed : parse wrongTargetSource = some wrongTargetProgram := by decide

def refactoredSource : String := "inline void AddSolution(unsigned long point, unsigned long component, su2double increment) {\n    const su2double previous = Solution_Old(point, component);\n    Solution(point, component) = previous + increment;\n  }"
def refactoredProgram : Program := parsedProgram refactoredSource (by decide)
theorem refactored_parsed : parse refactoredSource = some refactoredProgram := by decide

end SU2Source.Generated
