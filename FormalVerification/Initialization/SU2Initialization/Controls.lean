import SU2Initialization.Composition

noncomputable section
namespace SU2Initialization
open SU2 SU2Source SU2Boundary SU2BoundaryMemory
variable {N : Nat}

/-- Copy traversal refactoring preserves the entire logical memory, not only tested cells. -/
theorem store_permutation (b : Nat) (U : Values N) (xs ys : List (Fin N × Fin 4))
    (hp : xs.Perm ys) (m : Mem) : storeCells b U xs m = storeCells b U ys m := by
  classical
  funext a
  by_cases h : ∃ c : Fin N × Fin 4, a = (b,offset c)
  · obtain ⟨c, rfl⟩ := h
    rw [store_value, store_value]
    simp only [hp.mem_iff]
  · have hx : a ∉ xs.map (fun c => (b,offset c)) := by
      intro hm; obtain ⟨c, _, e⟩ := List.mem_map.mp hm; exact h ⟨c,e.symm⟩
    have hy : a ∉ ys.map (fun c => (b,offset c)) := by
      intro hm; obtain ⟨c, _, e⟩ := List.mem_map.mp hm; exact h ⟨c,e.symm⟩
    exact (store_frame b U xs m a hx).trans (store_frame b U ys m a hy).symm

theorem snapshot_refactor (m : Mem) (U : Values N) (h : Stored 0 m U) :
    copyCells 0 1 (visits N) m = copyCells 0 1 (visits N).reverse m := by
  rw [copy_executes 0 1 (by decide) _ m U h, copy_executes 0 1 (by decide) _ m U h]
  exact congrArg some (store_permutation 1 U _ _ (List.reverse_perm _).symm m)

def testMemory : Mem := fun a => some (if a.1 = 0 then (a.2 : ℝ) + 1 else 90)
def CopyContract (dst : Nat) (cs : List (Fin 2 × Fin 4)) : Prop :=
  ∃ m, copyCells 0 dst cs testMemory = some m ∧ m (1,7) = some 8 ∧ m (0,7) = some 8

theorem copy_positive : CopyContract 1 (visits 2) := by
  norm_num [CopyContract, copyCells, visits, offset, testMemory, put, Function.update_apply]

theorem copy_wrong_buffer_rejected : ¬ CopyContract 0 (visits 2) := by
  norm_num [CopyContract, copyCells, visits, offset, testMemory, put, Function.update_apply]

theorem copy_incomplete_rejected : ¬ CopyContract 1 ((visits 2).filter (fun c => offset c < 7)) := by
  norm_num [CopyContract, copyCells, visits, offset, testMemory, put, Function.update_apply]

def clearResidual (m : Mem) : Mem := storeCells 5 (fun _ : Fin N × Fin 4 => 0) (visits N) m

theorem residual_clear_complete (m : Mem) : Stored (N := N) 5 (clearResidual (N := N) m) (fun _ => 0) :=
  store_complete 5 _ m

theorem residual_clear_frame (m : Mem) :
    Frame ((visits N).map (fun c => (5,offset c))) m (clearResidual (N := N) m) := store_frame 5 _ _ m

def ClearContract (b : Nat) : Prop :=
  let m := storeCells b (fun _ : Fin 2 × Fin 4 => 0) (visits 2) testMemory
  m (5,7) = some 0 ∧ m (6,7) = testMemory (6,7)

theorem clear_positive : ClearContract 5 := by
  norm_num [ClearContract, storeCells, visits, offset, put, testMemory, Function.update_apply]

theorem clear_wrong_buffer_rejected : ¬ ClearContract 6 := by
  norm_num [ClearContract, storeCells, visits, offset, put, testMemory, Function.update_apply]

def testGas : IdealGas := ⟨2,1,by norm_num,by norm_num⟩
def badState : State 2 := state 1 ![2,0] 1
def backupState : State 2 := state 2 ![6,8] 30

theorem backup_admissible : Admissible backupState := by
  norm_num [Admissible, backupState, internalDensity, rho, mom, energy, state, dot, Fin.sum_univ_two]

theorem bad_rejected : rejected testGas badState := by
  rw [rejection_iff]
  norm_num [Admissible, badState, internalDensity, rho, mom, energy, state, dot, Fin.sum_univ_two]

theorem fallback_positive :
    (setPoint testGas badState backupState (fun _ => 99)).current = backupState ∧
    (setPoint testGas badState backupState (fun _ => 99)).primitive 3 = 5 := by
  have hn : ¬ Admissible badState := (rejection_iff testGas badState).mp bad_rejected
  have hs : selectedState badState backupState = backupState := by simp [selectedState, hn]
  refine ⟨(set_point_current testGas badState backupState _).trans hs, ?_⟩
  rw [(set_point_refines testGas badState backupState _ backup_admissible).2.1, hs]
  change (toPrimitive testGas backupState).pressure = 5
  norm_num [toPrimitive, internalDensity, testGas, backupState, rho, mom, energy, state, dot, Fin.sum_univ_two]

def FallbackContract (recompute : Bool) : Prop :=
  (if recompute then stagePoint testGas backupState (stagePoint testGas badState (fun _ => 99))
   else stagePoint testGas badState (fun _ => 99)) 3 =
    (toCFD backupState).pressure testGas.gamma

theorem fallback_stale_primitives_rejected : ¬ FallbackContract false := by
  change stagedPressure testGas badState ≠ (toCFD backupState).pressure testGas.gamma
  norm_num [stagedPressure, staticEnergy, velocitySquared,
    testGas, badState, backupState, toCFD, CFDLean.Conservative.pressure, CFDLean.squareSum,
    rho, mom, energy, state, dot, Fin.sum_univ_two]

/-- Meaningful numerical mutation: kinetic energy is omitted from pressure conversion. -/
def PressureContract (omitKinetic : Bool) : Prop :=
  let U := state 1 ![2,0] 3
  (if omitKinetic then (testGas.gamma - 1) * energy U else stagedPressure testGas U) =
    (toCFD U).pressure testGas.gamma

theorem pressure_positive : PressureContract false := by
  norm_num [PressureContract, stagedPressure, staticEnergy, velocitySquared, testGas, toCFD,
    CFDLean.Conservative.pressure, CFDLean.squareSum, rho, mom, energy, state, dot, Fin.sum_univ_two]

theorem kinetic_omission_rejected : ¬ PressureContract true := by
  norm_num [PressureContract, testGas, toCFD, CFDLean.Conservative.pressure,
    CFDLean.squareSum, rho, mom, energy, state, Fin.sum_univ_two]

def SeedContract (omitDensity : Bool) : Prop :=
  (if omitDensity then seed 1 3 4 15 else seed 2 3 4 15) 1 = 6

theorem density_factor_positive : SeedContract false := by norm_num [SeedContract, seed]
theorem density_factor_rejected : ¬ SeedContract true := by norm_num [SeedContract, seed]

def TailContract (erase : Bool) : Prop :=
  let out := setPoint testGas badState backupState (fun _ => 99)
  (if erase then Function.update out.primitive 10 0 else out.primitive) 10 = 99

theorem primitive_tail_positive : TailContract false := by
  change (setPoint testGas badState backupState (fun _ => 99)).primitive 10 = 99
  exact set_point_tail testGas badState backupState _ 10 (by decide)

theorem primitive_tail_erasure_rejected : ¬ TailContract true := by
  simp [TailContract]

end SU2Initialization
