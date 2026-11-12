import SU2.Algebra

noncomputable section
namespace SU2
variable {d : ℕ}

theorem primitive_roundtrip (g : IdealGas) (W : Primitive d) (hr : W.density ≠ 0) :
    toPrimitive g (toConservative g W) = W := by
  have hg : g.gamma - 1 ≠ 0 := ne_of_gt (sub_pos.mpr g.gamma_gt_one)
  apply Primitive.ext
  · rfl
  · funext i
    simp [toPrimitive, toConservative, rho, mom, state, hr]
  · simp only [toPrimitive, toConservative, internalDensity, rho_state, mom_state, energy_state,
      dot_scale_left, dot_scale_right]
    field_simp [hr, hg]
    ring

theorem conservative_roundtrip (g : IdealGas) (U : State d) (hr : rho U ≠ 0) :
    toConservative g (toPrimitive g U) = U := by
  have hg : g.gamma - 1 ≠ 0 := ne_of_gt (sub_pos.mpr g.gamma_gt_one)
  apply state_ext
  · rfl
  · intro i
    change rho U * (mom U i / rho U) = mom U i
    field_simp [hr]
  · simp only [toConservative, toPrimitive, energy, state, internalDensity, div_eq_mul_inv]
    have hdot : dot (fun i => mom U i * (rho U)⁻¹) (fun i => mom U i * (rho U)⁻¹) =
        (rho U)⁻¹ * (rho U)⁻¹ * dot (mom U) (mom U) := by
      simp only [mul_comm (mom U _) ((rho U)⁻¹), dot_scale_left, dot_scale_right]
      ring
    rw [hdot]
    field_simp [hr, hg]
    ring

theorem primitive_admissible (g : IdealGas) (U : State d) (h : Admissible U) :
    PrimitiveAdmissible (toPrimitive g U) := by
  exact ⟨h.1, mul_pos (sub_pos.mpr g.gamma_gt_one) h.2⟩

theorem temperature_positive (g : IdealGas) (W : Primitive d) (h : PrimitiveAdmissible W) :
    0 < temperature g W := by
  exact div_pos h.2 (mul_pos h.1 g.gasConstant_pos)

/-- Matches the staged ideal-gas production formula, on its valid-state branch. -/
theorem pressure_specific_energy (g : IdealGas) (U : State d) (hr : rho U ≠ 0) :
    (g.gamma - 1) * rho U *
      (energy U / rho U - dot (fun i => mom U i / rho U) (fun i => mom U i / rho U) / 2) =
    (toPrimitive g U).pressure := by
  simp only [div_eq_mul_inv, toPrimitive, internalDensity]
  have hdot : dot (fun i => mom U i * (rho U)⁻¹) (fun i => mom U i * (rho U)⁻¹) =
      (rho U)⁻¹ * (rho U)⁻¹ * dot (mom U) (mom U) := by
    simp only [mul_comm (mom U _) ((rho U)⁻¹), dot_scale_left, dot_scale_right]
    ring
  rw [hdot]
  field_simp [hr]
  ring

end SU2
