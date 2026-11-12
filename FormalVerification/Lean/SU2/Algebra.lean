import SU2.Spec
import Mathlib.Tactic.Ring
import Mathlib.Tactic.FieldSimp
import Mathlib.Tactic.Linarith
import Mathlib.Tactic.Positivity
import Mathlib.Tactic.Abel

noncomputable section
open scoped BigOperators
namespace SU2
variable {d : ℕ}

@[simp] theorem dot_add_left (u v n : Vec d) :
    dot (fun i => u i + v i) n = dot u n + dot v n := by
  simp [dot, add_mul, Finset.sum_add_distrib]

@[simp] theorem dot_sub_left (u v n : Vec d) :
    dot (fun i => u i - v i) n = dot u n - dot v n := by
  simp [dot, sub_mul, Finset.sum_sub_distrib]

@[simp] theorem dot_scale_left (a : ℝ) (u n : Vec d) :
    dot (fun i => a * u i) n = a * dot u n := by
  simp [dot, mul_assoc, Finset.mul_sum]

theorem dot_comm (u v : Vec d) : dot u v = dot v u := by
  simp [dot, mul_comm]

@[simp] theorem dot_scale_right (a : ℝ) (u n : Vec d) :
    dot u (fun i => a * n i) = a * dot u n := by
  rw [dot_comm, dot_scale_left, dot_comm n u]

@[simp] theorem dot_sub_right (u v n : Vec d) :
    dot u (fun i => v i - n i) = dot u v - dot u n := by
  simp [dot, mul_sub, Finset.sum_sub_distrib]

theorem dot_self_nonneg (u : Vec d) : 0 ≤ dot u u := by
  exact Finset.sum_nonneg fun i _ => mul_self_nonneg (u i)

@[simp] theorem rho_state (r : ℝ) (m : Vec d) (E : ℝ) : rho (state r m E) = r := rfl
@[simp] theorem mom_state (r : ℝ) (m : Vec d) (E : ℝ) : mom (state r m E) = m := rfl
@[simp] theorem energy_state (r : ℝ) (m : Vec d) (E : ℝ) : energy (state r m E) = E := rfl

theorem state_ext (U V : State d) (hr : rho U = rho V)
    (hm : ∀ i, mom U i = mom V i) (hE : energy U = energy V) : U = V := by
  funext c
  cases c with
  | mass => exact hr
  | momentum i => exact hm i
  | energy => exact hE

end SU2
