import SU2Initialization.SourceGate

set_option maxRecDepth 100000

set_option maxHeartbeats 4000000

namespace SU2Initialization.Generated

def original_snapshotSource : String := "void CVariable::Set_OldSolution() {\n  assert(Solution_Old.size() == Solution.size());\n  parallelCopy(Solution.size(), Solution.data(), Solution_Old.data());\n}"

def original_snapshot : Nat := (parseSnapshot original_snapshotSource).get (by decide)

theorem original_snapshot_parsed : parseSnapshot original_snapshotSource = some original_snapshot := by decide

def original_loopSource : String := "template <class T, class U>\nvoid parallelCopy(size_t size, const T* src, U* dst) {\n  SU2_OMP_FOR_STAT(2048)\n  for (size_t i = 0; i < size; ++i) dst[i] = src[i];\n  END_SU2_OMP_FOR\n}"

def original_loop : CopyPlan := (parseCopyLoop original_loopSource).get (by decide)

theorem original_loop_parsed : parseCopyLoop original_loopSource = some original_loop := by decide

def wrongBuffer_snapshotSource : String := "void CVariable::Set_OldSolution() {\n  assert(Solution_Old.size() == Solution.size());\n  parallelCopy(Solution.size(), Solution.data(), Solution.data());\n}"

def wrongBuffer_snapshot : Nat := (parseSnapshot wrongBuffer_snapshotSource).get (by decide)

theorem wrongBuffer_snapshot_parsed : parseSnapshot wrongBuffer_snapshotSource = some wrongBuffer_snapshot := by decide

def wrongBuffer_loopSource : String := "template <class T, class U>\nvoid parallelCopy(size_t size, const T* src, U* dst) {\n  SU2_OMP_FOR_STAT(2048)\n  for (size_t i = 0; i < size; ++i) dst[i] = src[i];\n  END_SU2_OMP_FOR\n}"

def wrongBuffer_loop : CopyPlan := (parseCopyLoop wrongBuffer_loopSource).get (by decide)

theorem wrongBuffer_loop_parsed : parseCopyLoop wrongBuffer_loopSource = some wrongBuffer_loop := by decide

def incomplete_snapshotSource : String := "void CVariable::Set_OldSolution() {\n  assert(Solution_Old.size() == Solution.size());\n  parallelCopy(Solution.size(), Solution.data(), Solution_Old.data());\n}"

def incomplete_snapshot : Nat := (parseSnapshot incomplete_snapshotSource).get (by decide)

theorem incomplete_snapshot_parsed : parseSnapshot incomplete_snapshotSource = some incomplete_snapshot := by decide

def incomplete_loopSource : String := "template <class T, class U>\nvoid parallelCopy(size_t size, const T* src, U* dst) {\n  SU2_OMP_FOR_STAT(2048)\n  for (size_t i = 0; i + 1 < size; ++i) dst[i] = src[i];\n  END_SU2_OMP_FOR\n}"

def incomplete_loop : CopyPlan := (parseCopyLoop incomplete_loopSource).get (by decide)

theorem incomplete_loop_parsed : parseCopyLoop incomplete_loopSource = some incomplete_loop := by decide

def refactored_snapshotSource : String := "void CVariable::Set_OldSolution() {\n  assert(Solution_Old.size() == Solution.size());\n  parallelCopy(Solution.size(), Solution.data(), Solution_Old.data());\n}"

def refactored_snapshot : Nat := (parseSnapshot refactored_snapshotSource).get (by decide)

theorem refactored_snapshot_parsed : parseSnapshot refactored_snapshotSource = some refactored_snapshot := by decide

def refactored_loopSource : String := "template <class T, class U>\nvoid parallelCopy(size_t size, const T* src, U* dst) {\n  SU2_OMP_FOR_STAT(2048)\n  for (size_t i = size; i-- > 0;) dst[i] = src[i];\n  END_SU2_OMP_FOR\n}"

def refactored_loop : CopyPlan := (parseCopyLoop refactored_loopSource).get (by decide)

theorem refactored_loop_parsed : parseCopyLoop refactored_loopSource = some refactored_loop := by decide

end SU2Initialization.Generated
