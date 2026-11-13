import SU2Preprocessing.Generated

noncomputable section
set_option maxRecDepth 100000
set_option maxHeartbeats 4000000
namespace SU2Preprocessing
open SU2Boundary SU2BoundaryMemory SU2Initialization
variable {M : Nat}

theorem original_loop_execution (s : Machine) :
    executePrimitivePlan (M := M) Generated.original s = executePoints (points M) s 0 := rfl

theorem refactored_loop_execution (s : Machine) :
    executePrimitivePlan (M := M) Generated.refactored s = executePrimitivePlan (M := M) Generated.original s := rfl

theorem source_skipped_row_rejected : ¬ TraversalContract (planPoints 2 Generated.skipLast) := by
  exact skipped_halo_rejected

/-- Entire parsed caller refines the protected point fold, with its frame. Internal
callee representation and serial/nSecondaryVar=0 interfaces remain declared assumptions. -/
theorem source_loop_refines (g : SU2.IdealGas) (s : Machine) (d : Data M)
    (h : DataRep s d) (hf : FluidReady g s.fluid) :
    ∃ final, executePrimitivePlan (M := M) Generated.original s = some (final,(modelPoints g (points M) d 0).2) ∧
      DataRep final (modelPoints g (points M) d 0).1 ∧ FluidReady g final.fluid ∧
      final.fluid.entropyEnabled = s.fluid.entropyEnabled ∧
      final.fluid.parameters = s.fluid.parameters ∧ final.fluid.untouched = s.fluid.untouched ∧
      Frame ((points M).flatMap rowFootprint) s.memory final.memory :=
  points_refine g (points M) s d 0 h hf

/-- The selected caller consumes the parsed complete primitive loop, then its actual
counter publication and residual clearing. These caller branches are manually mapped. -/
def executeSourceCommon (plan : PrimitivePlan) (s0 : ControlState) : Option ControlState := do
  let s := {s0 with errorCounter := 0}
  let (processed,count) ← executePrimitivePlan (M := M) plan s.machine
  let s := {s with machine := processed,errorCounter := s.errorCounter + count}
  let s := {s with configNonphysical := s.errorCounter}
  return {s with machine := {s.machine with memory := clearResidual (N := M) s.machine.memory}}

def executeSourceCommonSnapshot (plan : PrimitivePlan) (s0 : ControlState) : Option ControlState := do
  let s ← executeSourceCommon (M := M) plan s0
  let m ← executeSnapshot (M := M) s.machine.memory
  return {s with machine := {s.machine with memory := m}}

theorem source_common_execution (s0 : ControlState) :
    executeSourceCommon (M := M) Generated.original s0 = executeCommon (M := M) s0 := rfl

theorem source_snapshot_execution (s0 : ControlState) :
    executeSourceCommonSnapshot (M := M) Generated.original s0 = executeCommonSnapshot (M := M) s0 := rfl

theorem source_caller_refactor (s0 : ControlState) :
    executeSourceCommonSnapshot (M := M) Generated.refactored s0 =
      executeSourceCommonSnapshot (M := M) Generated.original s0 := rfl

theorem source_preprocessing_snapshot_refines {K : Nat} {vertices : Fin K → Nat}
    (g : SU2.IdealGas) (s0 : ControlState) (d : Data M)
    (T : SU2.Residual M 2) (cache : Nat → Option Ghost)
    (h0 : DataRep s0.machine d) (hf : FluidReady g s0.machine.fluid)
    (ht : Stored 6 s0.machine.memory (fun c => T c.1 (channel c.2)))
    (hc : CacheRep vertices s0.machine.memory cache) :
    let expected := modelPoints g (points M) d 0
    let state := preprocessingState expected.1 T cache
    ∃ pre post,
      executeSourceCommon (M := M) Generated.original s0 = some pre ∧
      executeSourceCommonSnapshot (M := M) Generated.original s0 = some post ∧
      BeforeSnapshot (vertices := vertices) pre.machine.memory state expected.1.primitive ∧
      BoundaryRep vertices post.machine.memory (snapshotState state) expected.1.primitive ∧
      Stored 0 post.machine.memory (fun c => expected.1.current c.1 (channel c.2)) ∧
      post.errorCounter = expected.2 ∧ post.configNonphysical = expected.2 ∧ post.errorCounter ≤ M ∧
      post.machine.fluid = pre.machine.fluid ∧ FluidReady g post.machine.fluid ∧
      Frame (preprocessingFootprint M ++ snapshotFootprint M) s0.machine.memory post.machine.memory :=
  preprocessing_snapshot_refines g s0 d T cache h0 hf ht hc

end SU2Preprocessing
