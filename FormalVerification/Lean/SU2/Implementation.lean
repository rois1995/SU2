import SU2.Wall
import SU2.Assembly
import SU2.Thermodynamics
import Mathlib.Algebra.BigOperators.Fin

/-! Hand-written exact-real models of selected source slices; NOT a verified C++ translation. -/
noncomputable section
namespace SU2.Implementation
open SU2
variable {d : ℕ}

def sumLoop (xs : List ℝ) (acc : ℝ) : ℝ := xs.foldl (fun s x => s + x) acc

theorem sumLoop_correct (xs : List ℝ) (acc : ℝ) : sumLoop xs acc = acc + xs.sum := by
  induction xs generalizing acc with
  | nil => simp [sumLoop]
  | cons x xs ih =>
    simp only [sumLoop, List.foldl_cons] at *
    rw [ih]
    simp [add_assoc]

def dotLoop (a b : Vec d) : ℝ := sumLoop (List.ofFn fun i => a i * b i) 0

theorem dotLoop_correct (a b : Vec d) : dotLoop a b = dot a b := by
  simp [dotLoop, sumLoop_correct, dot, List.sum_ofFn]

/-- BC_Sym_Plane solutionOld slice and MultigridProjectEulerWall compressible slice. -/
def stateProjection (n w : Vec d) (U : State d) : State d :=
  let gridVel := fun i => w i * rho U
  let vp := dotLoop (fun i => mom U i - gridVel i) n
  let gridNormalVel := dotLoop w n
  state (rho U) (fun i => mom U i - vp * n i) (energy U - gridNormalVel * vp)

/-- BC_Sym_Plane residual/truncation slice, with independent scalar accumulation. -/
def residualProjection (n w : Vec d) (U : State d) : State d :=
  let gridNormalVel := dotLoop w n
  let normalRes := dotLoop (mom U) n - gridNormalVel * rho U
  let newEnergy := energy U - gridNormalVel * normalRes
  state (rho U) (fun i => mom U i - normalRes * n i) newEnergy

theorem stateProjection_correct (n w : Vec d) (U : State d) :
    stateProjection n w U = wallProject n w U := by
  have h : dotLoop (fun i => mom U i - w i * rho U) n = wallDefect n w U := by
    rw [dotLoop_correct]
    simp only [mul_comm (w _) (rho U), dot_sub_left, dot_scale_left, wallDefect]
  simp only [stateProjection, h, dotLoop_correct, wallProject]

theorem residualProjection_correct (n w : Vec d) (U : State d) :
    residualProjection n w U = wallProject n w U := by
  simp only [residualProjection, dotLoop_correct, wallProject, wallDefect]
  rw [mul_comm (dot w n) (rho U)]

theorem model_satisfies_contract (n w : Vec d) (hn : dot n n = 1) (U : State d) :
    WallContract n w U (stateProjection n w U) := by
  rw [stateProjection_correct]
  exact wall_project_contract n w hn U

theorem corrected_update_impermeable (n w : Vec d) (hn : dot n n = 1)
    (U D : State d) (alpha : ℝ) :
    Impermeable n w (conservativeUpdate (stateProjection n w U)
      (residualProjection n w D) alpha) := by
  rw [stateProjection_correct, residualProjection_correct]
  exact wall_update_preserves n w _ _ alpha (wall_project_impermeable n w hn U)
    (wall_project_impermeable n w hn D)

/-- Historical parent: momentum residual is projected in the stationary frame. -/
def legacyResidualProjection (n : Vec d) (U : State d) : State d :=
  let normalRes := dotLoop (mom U) n
  state (rho U) (fun i => mom U i - normalRes * n i) (energy U)

/-- Candidate refactor of the residual slice: rename the temporary only. -/
def renamedResidualProjection (n w : Vec d) (U : State d) : State d :=
  let gridNormalVel := dotLoop w n
  let wallResidual := dotLoop (mom U) n - gridNormalVel * rho U
  let newEnergy := energy U - gridNormalVel * wallResidual
  state (rho U) (fun i => mom U i - wallResidual * n i) newEnergy

theorem rename_preserves_model (n w : Vec d) (U : State d) :
    renamedResidualProjection n w U = residualProjection n w U := rfl

theorem renamed_satisfies_spec (n w : Vec d) (U : State d) :
    renamedResidualProjection n w U = wallProject n w U :=
  (rename_preserves_model n w U).trans (residualProjection_correct n w U)

/-- Refactor experiment: extract the original ascending accumulation into a helper. -/
def normalMomentum (n : Vec d) (U : State d) : ℝ :=
  (List.ofFn fun i => mom U i * n i).foldl (fun acc term => acc + term) 0

theorem normalMomentum_correct (n : Vec d) (U : State d) :
    normalMomentum n U = dot (mom U) n := by
  exact dotLoop_correct (mom U) n

def factoredResidualProjection (n w : Vec d) (U : State d) : State d :=
  let g := dotLoop w n
  let a := normalMomentum n U - g * rho U
  state (rho U) (fun i => mom U i - a * n i) (energy U - g * a)

theorem factored_satisfies_spec (n w : Vec d) (U : State d) :
    factoredResidualProjection n w U = wallProject n w U := by
  simp only [factoredResidualProjection, normalMomentum_correct, dotLoop_correct,
    wallProject, wallDefect]
  rw [mul_comm (dot w n) (rho U)]

end SU2.Implementation
