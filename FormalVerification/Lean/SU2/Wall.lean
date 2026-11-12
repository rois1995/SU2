import SU2.Algebra

noncomputable section
namespace SU2
variable {d : ℕ}

theorem wall_project_impermeable (n w : Vec d) (hn : dot n n = 1) (U : State d) :
    Impermeable n w (wallProject n w U) := by
  change dot (fun i => mom U i - wallDefect n w U * n i) n - rho U * dot w n = 0
  simp only [dot_sub_left, dot_scale_left, hn, mul_one, wallDefect]
  ring

theorem wall_project_contract (n w : Vec d) (hn : dot n n = 1) (U : State d) :
    WallContract n w U (wallProject n w U) := by
  exact ⟨rfl, wall_project_impermeable n w hn U, wallDefect n w U, fun _ => rfl, rfl⟩

theorem wall_contract_unique (n w : Vec d) (hn : dot n n = 1) (U V : State d)
    (h : WallContract n w U V) : V = wallProject n w U := by
  obtain ⟨hr, hv, a, hm, hE⟩ := h
  have hmfun : mom V = fun i => mom U i - a * n i := funext hm
  have ha : a = wallDefect n w U := by
    unfold Impermeable wallDefect at hv
    rw [hr, hmfun, dot_sub_left, dot_scale_left, hn] at hv
    unfold wallDefect
    linarith
  apply state_ext
  · exact hr
  · intro i
    simpa only [wallProject, mom, state, ha] using hm i
  · simpa only [wallProject, energy, state, ha] using hE

theorem wall_project_fixed (n w : Vec d) (U : State d) (h : Impermeable n w U) :
    wallProject n w U = U := by
  unfold Impermeable at h
  apply state_ext
  · rfl
  · intro i
    change mom U i - wallDefect n w U * n i = mom U i
    rw [h, zero_mul, sub_zero]
  · change energy U - dot w n * wallDefect n w U = energy U
    rw [h, mul_zero, sub_zero]

theorem wall_project_idempotent (n w : Vec d) (hn : dot n n = 1) (U : State d) :
    wallProject n w (wallProject n w U) = wallProject n w U := by
  exact wall_project_fixed n w _ (wall_project_impermeable n w hn U)

theorem wall_defect_update (n w : Vec d) (U D : State d) (alpha : ℝ) :
    wallDefect n w (conservativeUpdate U D alpha) =
      wallDefect n w U + alpha * wallDefect n w D := by
  change dot (fun i => mom U i + alpha * mom D i) n -
    (rho U + alpha * rho D) * dot w n = _
  simp only [dot_add_left, dot_scale_left, wallDefect]
  ring

theorem wall_update_preserves (n w : Vec d) (U D : State d) (alpha : ℝ)
    (hU : Impermeable n w U) (hD : Impermeable n w D) :
    Impermeable n w (conservativeUpdate U D alpha) := by
  unfold Impermeable at *
  rw [wall_defect_update, hU, hD]
  ring

/-- The nonzero-density precondition is necessary to speak of velocity. -/
theorem impermeable_velocity (n w : Vec d) (U : State d) (hr : rho U ≠ 0) :
    Impermeable n w U ↔ dot (fun i => mom U i / rho U - w i) n = 0 := by
  have hd : dot (fun i => mom U i / rho U) n = (rho U)⁻¹ * dot (mom U) n := by
    simp only [div_eq_mul_inv, mul_comm (mom U _) ((rho U)⁻¹), dot_scale_left]
  rw [dot_sub_left, hd]
  unfold Impermeable wallDefect
  constructor <;> intro h <;> field_simp [hr] at * <;> nlinarith

/-- Pressure work and momentum correction raise internal energy by this exact square. -/
theorem wall_internal_energy (n w : Vec d) (hn : dot n n = 1) (U : State d)
    (hr : rho U ≠ 0) :
    internalDensity (wallProject n w U) =
      internalDensity U + (wallDefect n w U)^2 / (2 * rho U) := by
  change energy U - dot w n * wallDefect n w U -
    dot (fun i => mom U i - wallDefect n w U * n i)
      (fun i => mom U i - wallDefect n w U * n i) / (2 * rho U) = _
  simp only [dot_sub_left, dot_sub_right, dot_scale_left, dot_scale_right, hn]
  rw [dot_comm n (mom U)]
  unfold wallDefect internalDensity
  field_simp [hr]
  ring

theorem wall_preserves_admissibility (n w : Vec d) (hn : dot n n = 1) (U : State d)
    (h : Admissible U) : Admissible (wallProject n w U) := by
  refine ⟨h.1, ?_⟩
  rw [wall_internal_energy n w hn U (ne_of_gt h.1)]
  exact add_pos_of_pos_of_nonneg h.2 (div_nonneg (sq_nonneg _) (le_of_lt (mul_pos (by norm_num) h.1)))

end SU2
