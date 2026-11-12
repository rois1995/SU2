import SU2Initialization.Storage

noncomputable section
namespace SU2Initialization
open SU2 SU2Boundary SU2BoundaryMemory

def velocitySquared (U : State 2) : ℝ :=
  dot (fun i => mom U i / rho U) (fun i => mom U i / rho U)
def staticEnergy (U : State 2) : ℝ := energy U / rho U - velocitySquared U / 2
def stagedPressure (g : IdealGas) (U : State 2) : ℝ := (g.gamma - 1) * rho U * staticEnergy U
def stagedTemperature (g : IdealGas) (U : State 2) : ℝ := (g.gamma - 1) / g.gasConstant * staticEnergy U
def stagedSound2 (g : IdealGas) (U : State 2) : ℝ := g.gamma * g.gasConstant * stagedTemperature g U

def rejected (g : IdealGas) (U : State 2) : Prop :=
  rho U ≤ 0 ∨ stagedPressure g U ≤ 0 ∨ stagedSound2 g U < 0 ∨ stagedTemperature g U ≤ 0

instance (g : IdealGas) (U : State 2) : Decidable (rejected g U) := Classical.propDecidable _

theorem staged_pressure_correct (g : IdealGas) (U : State 2) (h : rho U ≠ 0) :
    stagedPressure g U = (toPrimitive g U).pressure := pressure_specific_energy g U h

theorem staged_temperature_correct (g : IdealGas) (U : State 2) (h : rho U ≠ 0) :
    stagedTemperature g U = temperature g (toPrimitive g U) := by
  rw [temperature, ← staged_pressure_correct g U h]
  change (g.gamma - 1) / g.gasConstant * staticEnergy U =
    ((g.gamma - 1) * rho U * staticEnergy U) / (rho U * g.gasConstant)
  field_simp [h, ne_of_gt g.gasConstant_pos]
  ring

theorem admissible_not_rejected (g : IdealGas) (U : State 2) (h : Admissible U) : ¬ rejected g U := by
  have hp := (primitive_admissible g U h).2
  have ht := temperature_positive g (toPrimitive g U) (primitive_admissible g U h)
  rw [← staged_pressure_correct g U (ne_of_gt h.1)] at hp
  rw [← staged_temperature_correct g U (ne_of_gt h.1)] at ht
  have ha : 0 < stagedSound2 g U := mul_pos (mul_pos (lt_trans zero_lt_one g.gamma_gt_one) g.gasConstant_pos) ht
  exact fun bad => bad.elim (not_le.mpr h.1) (fun rest =>
    rest.elim (not_le.mpr hp) (fun tail => tail.elim (not_lt.mpr ha.le) (not_le.mpr ht)))

theorem rejection_iff (g : IdealGas) (U : State 2) : rejected g U ↔ ¬ Admissible U := by
  constructor
  · intro hb ha; exact admissible_not_rejected g U ha hb
  · intro hn
    by_contra hb
    have hr : 0 < rho U := lt_of_not_ge (fun h => hb (Or.inl h))
    have hp : 0 < stagedPressure g U := lt_of_not_ge (fun h => hb (Or.inr (Or.inl h)))
    rw [staged_pressure_correct g U (ne_of_gt hr)] at hp
    have hi : 0 < internalDensity U := (mul_pos_iff_of_pos_left (sub_pos.mpr g.gamma_gt_one)).mp hp
    exact hn ⟨hr,hi⟩

/-- The negative sound-speed branch leaves its destination unchanged, as C++ does. -/
def stagePoint (g : IdealGas) (U : State 2) (prior : Fin 11 → ℝ) : Fin 11 → ℝ :=
  ![stagedTemperature g U, mom U 0 / rho U, mom U 1 / rho U,
    stagedPressure g U, rho U, prior 5,
    if stagedSound2 g U < 0 then prior 6 else Real.sqrt (stagedSound2 g U),
    prior 7, prior 8, prior 9, prior 10]

structure PointResult where
  current : State 2
  primitive : Fin 11 → ℝ
  velocity2 : ℝ
  physical : Bool

def setPoint (g : IdealGas) (current backup : State 2) (prior : Fin 11 → ℝ) : PointResult :=
  let first := stagePoint g current prior
  let bad := rejected g current
  let selected := if bad then backup else current
  let fields := if bad then stagePoint g backup first else first
  ⟨selected, Function.update fields 5 ((energy selected + fields 3) / rho selected),
    velocitySquared selected, !decide bad⟩

/-- Independent choice rule uses admissibility, not the implementation's flag checks. -/
def selectedState (current backup : State 2) : State 2 :=
  by classical exact if Admissible current then current else backup

/-- Temperature and acoustic fields extend M1 with the explicitly declared ideal-gas R. -/
def primitiveFields (g : IdealGas) (U : State 2) (prior : Fin 11 → ℝ) : Fin 11 → ℝ :=
  ![temperature g (toPrimitive g U), (toPrimitive g U).velocity 0, (toPrimitive g U).velocity 1,
    (toPrimitive g U).pressure, rho U, enthalpy g U,
    Real.sqrt (g.gamma * g.gasConstant * temperature g (toPrimitive g U)),
    prior 7, prior 8, prior 9, prior 10]

theorem selected_admissible (current backup : State 2) (hb : Admissible backup) :
    Admissible (selectedState current backup) := by
  classical
  by_cases hc : Admissible current <;> simp [selectedState, hc, hb]

theorem set_point_current (g : IdealGas) (current backup : State 2) (prior : Fin 11 → ℝ) :
    (setPoint g current backup prior).current = selectedState current backup := by
  classical
  by_cases h : Admissible current <;> simp [setPoint, selectedState, rejection_iff, h]

theorem set_point_tail (g : IdealGas) (current backup : State 2) (prior : Fin 11 → ℝ)
    (c : Fin 11) (h : 7 ≤ c.val) : (setPoint g current backup prior).primitive c = prior c := by
  classical
  by_cases hb : rejected g current <;>
    fin_cases c <;> simp_all [setPoint, stagePoint, Function.update_apply]

theorem stage_valid (g : IdealGas) (U : State 2) (prior : Fin 11 → ℝ) (h : Admissible U) :
    Function.update (stagePoint g U prior) 5
      ((energy U + stagePoint g U prior 3) / rho U) = primitiveFields g U prior := by
  have ht := temperature_positive g (toPrimitive g U) (primitive_admissible g U h)
  have ha : ¬ stagedSound2 g U < 0 := by
    unfold stagedSound2
    rw [staged_temperature_correct g U (ne_of_gt h.1)]
    exact not_lt.mpr (le_of_lt (mul_pos (mul_pos (lt_trans zero_lt_one g.gamma_gt_one) g.gasConstant_pos) ht))
  simp only [stagePoint, if_neg ha]
  funext c
  fin_cases c <;> simp [primitiveFields, staged_pressure_correct g U (ne_of_gt h.1),
    staged_temperature_correct g U (ne_of_gt h.1), stagedSound2, toPrimitive, enthalpy]

theorem set_point_refines (g : IdealGas) (current backup : State 2) (prior : Fin 11 → ℝ)
    (hb : Admissible backup) :
    (setPoint g current backup prior).current = selectedState current backup ∧
    (setPoint g current backup prior).primitive = primitiveFields g (selectedState current backup) prior ∧
    ((setPoint g current backup prior).physical = true ↔ Admissible current) := by
  classical
  refine ⟨set_point_current g current backup prior, ?_, ?_⟩
  · by_cases hc : Admissible current
    · simpa [setPoint, selectedState, rejection_iff, hc] using stage_valid g current prior hc
    · have htail : primitiveFields g backup (stagePoint g current prior) = primitiveFields g backup prior := by
        funext c
        fin_cases c <;> simp [primitiveFields, stagePoint]
      simpa [setPoint, selectedState, rejection_iff, hc, htail] using
        (stage_valid g backup (stagePoint g current prior) hb).trans htail
  · simp [setPoint, rejection_iff]

end SU2Initialization
