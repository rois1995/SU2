import SU2Initialization.Generated

set_option maxRecDepth 100000
set_option maxHeartbeats 4000000
namespace SU2Initialization
open SU2 SU2Boundary SU2BoundaryMemory

theorem source_snapshot_plan : Generated.original_snapshot = 1 := by decide
theorem source_copy_plan : Generated.original_loop = ⟨false,false⟩ := by decide

theorem source_snapshot_execution {N : Nat} (m : Mem) :
    executeCopyPlan (N := N) Generated.original_snapshot Generated.original_loop m =
    executeSnapshot (M := N) m := by
  simp [executeCopyPlan, source_snapshot_plan, source_copy_plan, executeSnapshot]

theorem source_wrong_buffer_semantic :
    ¬ CopyContract Generated.wrongBuffer_snapshot (visits 2) := by
  have h : Generated.wrongBuffer_snapshot = 0 := by decide
  rw [h]; exact copy_wrong_buffer_rejected

theorem source_incomplete_admitted : Generated.incomplete_loop = ⟨false,true⟩ := by decide

theorem source_refactor {N : Nat} (m : Mem) (U : Values N) (h : Stored 0 m U) :
    executeCopyPlan (N := N) Generated.refactored_snapshot Generated.refactored_loop m =
    executeCopyPlan (N := N) Generated.original_snapshot Generated.original_loop m := by
  have hd : Generated.refactored_snapshot = 1 := by decide
  have hp : Generated.refactored_loop = ⟨true,false⟩ := by decide
  simpa [executeCopyPlan, source_snapshot_plan, source_copy_plan, hd, hp] using (snapshot_refactor m U h).symm

theorem unsupported_statement_rejected :
    parseSnapshot (Generated.original_snapshotSource ++ " Solution(0,0)=9;") = none := by decide

theorem source_incomplete_semantic :
    ¬ (∃ m, executeCopyPlan (N := 2) Generated.original_snapshot Generated.incomplete_loop testMemory = some m ∧
      m (1,7) = some 8 ∧ m (0,7) = some 8) := by
  norm_num [executeCopyPlan, source_snapshot_plan, source_incomplete_admitted,
    copyCells, visits, offset, testMemory, put, Function.update_apply]

end SU2Initialization
