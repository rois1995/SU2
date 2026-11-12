import SU2.Algebra
import Mathlib.Algebra.BigOperators.Pi

noncomputable section
open scoped BigOperators
namespace SU2
variable {N d : ℕ}

/-- Exact oriented incidence cancellation, for each conserved component. -/
theorem edge_conservation (e : InteriorEdge N d) (F : State d) :
    ∑ i, edgeContribution e F i = 0 := by
  simp [edgeContribution, Finset.sum_sub_distrib]

/-- Reversing a face reverses its flux and preserves its nodal contribution. -/
theorem orientation_consistency (e : InteriorEdge N d) (F : State d) (i : Fin N) :
    (if i = e.right then -F else 0) - (if i = e.left then -F else 0) =
      edgeContribution e F i := by
  by_cases hl : i = e.left <;> by_cases hr : i = e.right <;>
    simp [edgeContribution, hl, hr, e.distinct, Ne.symm e.distinct]

theorem assembly_conservation (events : List (Event N d)) :
    ∑ i, assemble events i = 0 := by
  induction events with
  | nil => simp [assemble]
  | cons e es ih =>
    simp only [assemble, List.map_cons, List.sum_cons] at *
    rw [Finset.sum_add_distrib, edge_conservation, ih, add_zero]

/-- A serial, in-place-style model of AddBlock followed by SubtractBlock. -/
def scatter (e : InteriorEdge N d) (F : State d) (R : Residual N d) : Residual N d :=
  let R₁ := Function.update R e.left (R e.left + F)
  Function.update R₁ e.right (R₁ e.right - F)

theorem scatter_correct (e : InteriorEdge N d) (F : State d) (R : Residual N d) :
    scatter e F R = R + edgeContribution e F := by
  funext i c
  by_cases hl : i = e.left <;> by_cases hr : i = e.right
  · exact False.elim (e.distinct (hl.symm.trans hr))
  · subst i
    simp [scatter, edgeContribution, e.distinct, Ne.symm e.distinct, sub_eq_add_neg]
  · subst i
    simp [scatter, edgeContribution, e.distinct, Ne.symm e.distinct, sub_eq_add_neg]
  · simp [scatter, edgeContribution, hl, hr]

def assembleLoop (events : List (Event N d)) (R : Residual N d) : Residual N d :=
  events.foldl (fun acc e => scatter e.1 e.2 acc) R

theorem accumulation_correct (events : List (Event N d)) (R : Residual N d) :
    assembleLoop events R = R + assemble events := by
  induction events generalizing R with
  | nil =>
    funext i c
    simp [assembleLoop, assemble]
  | cons e es ih =>
    simp only [assembleLoop, List.foldl_cons] at *
    rw [ih, scatter_correct]
    funext i c
    simp [assemble, add_assoc]

/-- Different serial event orders give the same result over exact reals. -/
theorem accumulation_permutation (xs ys : List (Event N d)) (h : xs.Perm ys) (R : Residual N d) :
    assembleLoop xs R = assembleLoop ys R := by
  rw [accumulation_correct, accumulation_correct]
  congr 1
  funext i
  exact (h.map (fun e => edgeContribution e.1 e.2 i)).sum_eq

/-- Volume-weighted local update balance; no stability or positivity assertion. -/
theorem volume_update_balance (volumes : Fin N → ControlVolume d) (dt : ℝ)
    (U R : Residual N d) (c : Channel d) :
    (∑ i, (volumes i).measure * explicitUpdate volumes dt U R i c) =
      (∑ i, (volumes i).measure * U i c) - dt * ∑ i, R i c := by
  have point : ∀ i, (volumes i).measure * explicitUpdate volumes dt U R i c =
      (volumes i).measure * U i c - dt * R i c := by
    intro i
    simp only [explicitUpdate, Pi.sub_apply, Pi.smul_apply, smul_eq_mul]
    field_simp [ne_of_gt (volumes i).positive]
    ring
  simp only [point, Finset.sum_sub_distrib, Finset.mul_sum]

theorem assembled_update_conserves (volumes : Fin N → ControlVolume d) (dt : ℝ)
    (U : Residual N d) (events : List (Event N d)) (c : Channel d) :
    (∑ i, (volumes i).measure * explicitUpdate volumes dt U (assemble events) i c) =
      ∑ i, (volumes i).measure * U i c := by
  rw [volume_update_balance]
  have h := congrFun (assembly_conservation events) c
  simp only [Finset.sum_apply, Pi.zero_apply] at h
  rw [h, mul_zero, sub_zero]

end SU2
