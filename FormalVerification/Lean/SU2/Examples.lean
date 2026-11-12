import SU2.Implementation
import Mathlib.Data.Fin.VecNotation
import Mathlib.Tactic.NormNum

noncomputable section
namespace SU2.Examples
open SU2 SU2.Implementation

def normal2 : Vec 2 := ![1, 0]
def wall2 : Vec 2 := ![2, 0]
def initial : State 2 := state 1 ![2, 0] 5
def increment : State 2 := state (1/2) ![0, 0] 0

theorem normal2_unit : dot normal2 normal2 = 1 := by
  norm_num [dot, normal2, Fin.sum_univ_succ]

theorem initial_admissible : Admissible initial := by
  norm_num [Admissible, initial, rho, mom, energy, state, internalDensity, dot, Fin.sum_univ_succ]

theorem initial_impermeable : Impermeable normal2 wall2 initial := by
  norm_num [Impermeable, wallDefect, normal2, wall2, initial, rho, mom, state, dot, Fin.sum_univ_succ]

/-- A finite, positive-density example: the old residual rule leaks after a density update. -/
theorem repaired_defect_detected :
    wallDefect normal2 wall2
      (conservativeUpdate initial (legacyResidualProjection normal2 increment) 1) = -1 := by
  norm_num [legacyResidualProjection, dotLoop_correct, conservativeUpdate, wallDefect,
    normal2, wall2, initial, increment, rho, mom, energy, state, dot, Fin.sum_univ_succ]

theorem legacy_update_not_impermeable :
    ¬ Impermeable normal2 wall2
      (conservativeUpdate initial (legacyResidualProjection normal2 increment) 1) := by
  unfold Impermeable
  rw [repaired_defect_detected]
  norm_num

theorem repaired_update_impermeable :
    Impermeable normal2 wall2
      (conservativeUpdate initial (residualProjection normal2 wall2 increment) 1) := by
  rw [residualProjection_correct]
  exact wall_update_preserves _ _ _ _ _ initial_impermeable
    (wall_project_impermeable _ _ normal2_unit _)

/-- The defective update is admissible: failure is not an invalid-state artifact. -/
theorem legacy_result_admissible :
    Admissible (conservativeUpdate initial (legacyResidualProjection normal2 increment) 1) := by
  norm_num [Admissible, internalDensity, conservativeUpdate, legacyResidualProjection,
    dotLoop_correct, normal2, initial, increment, rho, mom, energy, state, dot, Fin.sum_univ_succ]

/-- The normal must be unit: normalization is a real obligation, not an omitted detail. -/
theorem nonunit_normal_counterexample :
    ¬ Impermeable (![2, 0] : Vec 2) wall2
      (wallProject (![2, 0] : Vec 2) wall2 (state 1 ![0, 0] 5)) := by
  norm_num [Impermeable, wallProject, wallDefect, wall2, rho, mom, energy, state, dot, Fin.sum_univ_succ]

end SU2.Examples
