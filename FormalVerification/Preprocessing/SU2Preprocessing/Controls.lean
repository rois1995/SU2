import SU2Preprocessing.Properties

noncomputable section
namespace SU2Preprocessing
open SU2 SU2Source SU2Boundary SU2BoundaryMemory SU2Initialization

def loopFixture : Data 2 := ⟨fun _ => badState,fun _ => backupState,fun _ _ => 99,fun _ => some 9⟩

/-- Same numerical requirement for a complete traversal and for the skipped-row mutant. -/
def TraversalContract (ps : List (Fin 2)) : Prop :=
  ∀ q, Admissible ((modelPoints SU2Initialization.testGas ps loopFixture 0).1.current q)

theorem traversal_positive : TraversalContract (points 2) := by
  intro q
  exact complete_admissible SU2Initialization.testGas loopFixture (fun _ => backup_admissible) q

theorem skipped_halo_rejected : ¬ TraversalContract [0] := by
  intro h
  have valid := h 1
  have untouched := (model_unvisited SU2Initialization.testGas [0] loopFixture 0 1 (by decide)).1
  rw [untouched] at valid
  exact ((rejection_iff SU2Initialization.testGas badState).mp bad_rejected) valid

/-- Moving the snapshot before fallback destroys the good backup; no post-state premise. -/
def SnapshotOrderContract (early : Bool) : Prop :=
  let d := if early then {loopFixture with old := loopFixture.current} else loopFixture
  Admissible ((modelPoints SU2Initialization.testGas (points 2) d 0).1.current 0)

theorem snapshot_order_positive : SnapshotOrderContract false := by
  exact complete_admissible SU2Initialization.testGas loopFixture (fun _ => backup_admissible) 0

theorem model_same_bad (ps : List (Fin 2)) (count : Nat) (P : Primitives 2)
    (V : Fin 2 → Option ℝ) :
    (modelPoints SU2Initialization.testGas ps ⟨fun _ => badState,fun _ => badState,P,V⟩ count).1.current = fun _ => badState := by
  induction ps generalizing count P V with
  | nil => rfl
  | cons p ps ih =>
    have curr : Function.update (fun _ : Fin 2 => badState) p
        (setPoint SU2Initialization.testGas badState badState (P p)).current = fun _ => badState := by
      simp [setPoint,Function.update_eq_self]
    simpa only [modelPoints,advanceData,curr] using ih _
      (Function.update P p (setPoint SU2Initialization.testGas badState badState (P p)).primitive)
      (Function.update V p (some (setPoint SU2Initialization.testGas badState badState (P p)).velocity2))

theorem early_snapshot_rejected : ¬ SnapshotOrderContract true := by
  change ¬ Admissible ((modelPoints SU2Initialization.testGas (points 2)
    ⟨fun _ => badState,fun _ => badState,fun _ _ => 99,fun _ => some 9⟩ 0).1.current 0)
  rw [model_same_bad]
  exact (rejection_iff SU2Initialization.testGas badState).mp bad_rejected

/-- The actual enabled-entropy branch, versus syntactically valid skipped assignment. -/
def EntropyContract (skip : Bool) : Prop :=
  let f := freshFluid SU2Initialization.testGas true
  let out := executeFluid (if skip then {f with entropyEnabled := false} else f) 1 2
  out.entropy = Real.log 2

theorem entropy_positive : EntropyContract false := by
  norm_num [EntropyContract,executeFluid,freshFluid,gasParameters,SU2Initialization.testGas]

theorem skipped_entropy_rejected : ¬ EntropyContract true := by
  have pos : 0 < Real.log 2 := Real.log_pos (by norm_num)
  norm_num [EntropyContract,executeFluid,freshFluid,gasParameters,SU2Initialization.testGas]
  exact ne_of_lt pos

/-- Equivalent grouping of the selected scalar computation; no change of specification. -/
def fluidRefactor (f : Fluid) (density specificInternal : ℝ) : Fluid :=
  let dp := f.parameters.gammaMinusOne * density
  let t := f.parameters.dTde * specificInternal
  { f with
    density := density
    staticEnergy := specificInternal
    dPdrho := f.parameters.gammaMinusOne * specificInternal
    dPde := dp
    pressure := dp * specificInternal
    temperature := t
    sound2 := f.parameters.gamma * f.parameters.gasConstant * t
    entropy := if f.entropyEnabled then f.parameters.cv * Real.log t - f.parameters.gasConstant * Real.log density else f.entropy }

theorem fluid_refactor_correct (f : Fluid) (density specificInternal : ℝ) :
    fluidRefactor f density specificInternal = executeFluid f density specificInternal := by
  cases h : f.entropyEnabled <;> simp [fluidRefactor,executeFluid,h]

end SU2Preprocessing
