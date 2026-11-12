import SU2Boundary.Loop

set_option maxRecDepth 100000
set_option maxHeartbeats 4000000

noncomputable section
namespace SU2Boundary
open SU2 SU2Source SU2Euler

/-- Unchanged addition contract for the selected boundary flux-accumulation call. -/
def FluxAccumulationContract (op : Op) : Prop := ∀ r f : ℝ, opReal op r f = r + f

theorem far_add_positive : FluxAccumulationContract Generated.farAdd_original := by
  have h : Generated.farAdd_original = .add := by decide
  simp [FluxAccumulationContract, h, opReal]

theorem far_add_mutant_rejected : ¬ FluxAccumulationContract Generated.farAdd_wrongSign := by
  have h : Generated.farAdd_wrongSign = .sub := by decide
  intro bad
  have := bad 0 1
  norm_num [h, opReal] at this

def axis : UnitNormal 2 := ⟨![1, 0], by norm_num [dot, Fin.sum_univ_two]⟩
def wallWitness : WallVisit 1 := ⟨0, true, axis⟩
def witnessState : BoundaryState 1 :=
  ⟨fun _ => state 2 ![3, 4] 40, fun _ => state 2 ![3, 4] 40,
   fun _ => state 1 ![5, 6] 7, fun _ => state 2 ![7, 8] 9, fun _ => none⟩

def ThreeBufferContract (oldOp resOp truncOp : Op) : Prop :=
  let a := wallImpl oldOp resOp truncOp wallWitness witnessState
  Impermeable axis.vector 0 (a.old 0) ∧ Impermeable axis.vector 0 (a.residual 0) ∧
    Impermeable axis.vector 0 (a.truncation 0)

theorem wall_positive : ThreeBufferContract Generated.wallOld_original Generated.wallRes_original Generated.wallTrunc_original := by
  unfold ThreeBufferContract
  rw [wall_source_contract]
  exact wall_three_constraints wallWitness witnessState rfl

theorem wall_old_mutant_rejected :
    ¬ ThreeBufferContract Generated.wallOld_wrongSign Generated.wallRes_original Generated.wallTrunc_original := by
  have h : Generated.wallOld_wrongSign = .add := by decide
  norm_num [ThreeBufferContract, wallImpl, wallWitness, witnessState, h, Impermeable,
    wallDefect, projection, axis, dot, Fin.sum_univ_two, opReal, mom, state]

theorem wall_residual_mutant_rejected :
    ¬ ThreeBufferContract Generated.wallOld_original Generated.wallRes_wrongSign Generated.wallTrunc_original := by
  have h : Generated.wallRes_wrongSign = .add := by decide
  intro bad
  have hr := bad.2.1
  norm_num [wallImpl, wallWitness, witnessState, h, Impermeable, wallDefect,
    projection, axis, dot, Fin.sum_univ_two, opReal, mom, state] at hr

theorem wall_truncation_mutant_rejected :
    ¬ ThreeBufferContract Generated.wallOld_original Generated.wallRes_original Generated.wallTrunc_wrongSign := by
  have h : Generated.wallTrunc_wrongSign = .add := by decide
  intro bad
  have ht := bad.2.2
  norm_num [wallImpl, wallWitness, witnessState, h, Impermeable, wallDefect,
    projection, axis, dot, Fin.sum_univ_two, opReal, mom, state] at ht

/-- Missing even one required cell violates the full numerical contract when its
initial current value differs from the specified update. This is execution rejection,
not just a list-coverage or syntax check. -/
theorem missing_cell_rejected {N M : Nat} (cs : List (Cell N)) (m : Memory ℝ)
    (s : BoundaryState N) (vol dt : Fin N → ℝ) (ready : InputsReady m s vol (fun _ => 0) dt)
    (owned : N ≤ M) (size : 8 * (M * 4) ≤ 2^64) (c : Cell N) (missing : c ∉ cs)
    (different : m (destination c) ≠ some (updateSpec vol dt s c.1 (channel c.2))) :
    ¬ ∃ after, executeCells M Generated.volume_original Generated.periodic_original Generated.timestep_original cs m =
        some after ∧ LoopContract s vol dt m after := by
  rw [execute_cells_correct cs m s vol (fun _ => 0) dt ready owned size]
  rintro ⟨after, he, hc⟩
  cases Option.some.inj he
  have hv := hc.1 c
  rw [writes_value, if_neg missing] at hv
  exact different hv

def loopState : BoundaryState 2 :=
  ⟨fun _ _ => 99, fun _ _ => 2, fun _ _ => 3, fun _ _ => 1, fun _ => none⟩
def loopMemory : Memory ℝ := fun a =>
  if a.1 = 0 then some 99 else if a.1 = 1 then some 2 else if a.1 = 2 then some 2 else
  if a.1 = 3 then some 0 else if a.1 = 4 then some (1/2) else if a.1 = 5 then some 3 else
  if a.1 = 6 then some 1 else if a.1 = 7 then some 4 else none

theorem loop_ready : InputsReady loopMemory loopState (fun _ => 2) (fun _ => 0) (fun _ => 1/2) := by
  simp [InputsReady, loopMemory, loopState]

def LoopWitnessContract (points components : LoopCode) : Prop :=
  ∃ after, executeLoop (N := 2) 2 Generated.volume_original Generated.periodic_original Generated.timestep_original
    points components loopMemory = some after ∧
    LoopContract loopState (fun _ => 2) (fun _ => 1/2) loopMemory after

theorem loop_positive : LoopWitnessContract Generated.points_original Generated.components_original := by
  exact complete_update_loop loopMemory loopState (fun _ => 2) (fun _ => 1/2) loop_ready (by decide) (by norm_num)

theorem loop_refactor_positive : LoopWitnessContract Generated.points_refactored Generated.components_refactored := by
  have hp : Generated.points_refactored = Generated.points_original := by decide
  have hv : Generated.components_refactored = Generated.components_original := by decide
  rw [hp, hv]
  exact loop_positive

theorem point_mutant_rejected : ¬ LoopWitnessContract Generated.points_skipLast Generated.components_original := by
  simp only [LoopWitnessContract, executeLoop, show ¬ (2 = 0) from by decide, and_false, ite_false]
  apply missing_cell_rejected _ loopMemory loopState (fun _ => 2) (fun _ => 1/2) loop_ready
    (by decide : 2 ≤ 2) (by norm_num) (1, 0)
  · decide
  · norm_num [loopMemory, destination, updateSpec, loopState]

theorem component_mutant_rejected : ¬ LoopWitnessContract Generated.points_original Generated.components_skipLast := by
  simp only [LoopWitnessContract, executeLoop, show ¬ (2 = 0) from by decide, and_false, ite_false]
  apply missing_cell_rejected _ loopMemory loopState (fun _ => 2) (fun _ => 1/2) loop_ready
    (by decide : 2 ≤ 2) (by norm_num) (0, 3)
  · decide
  · norm_num [loopMemory, destination, updateSpec, loopState]

/-- Getter contract fixes timestep storage, independently of the selected getter body. -/
def TimestepContract (g : Getter) : Prop := readGetter g loopMemory 0 = some (1/2)

theorem timestep_positive : TimestepContract Generated.timestep_original := rfl

theorem timestep_mutant_rejected : ¬ TimestepContract Generated.timestep_wrongField := by
  have h : Generated.timestep_wrongField = .localCFL := by decide
  norm_num [TimestepContract, readGetter, h, getterBlock, loopMemory]

theorem reject_unknown_projection : parseProjection .old (Generated.wallOld_originalSource ++ " break;") = none := by decide

theorem reject_extra_loop_condition : parseLoop true "for (unsigned long iPoint = 0; iPoint < nPointDomain && ready; iPoint++) {" = none := by decide

theorem reject_extra_getter_statement : parseGetter "GetVolume" "inline su2double& GetVolume(unsigned long iPoint) { log(); return Volume(iPoint); }" = none := by decide

end SU2Boundary
