import Mathlib.Data.Real.Basic
import Mathlib.Algebra.BigOperators.Field
import Mathlib.Data.Fintype.Pi
import Mathlib.Data.Fintype.Sum
import Mathlib.Tactic.DeriveFintype
import Mathlib.Data.List.OfFn

/-! Exact-real mathematical specification. No C++ or IEEE-754 semantics are asserted. -/
noncomputable section
open scoped BigOperators
namespace SU2

abbrev Vec (d : ℕ) := Fin d → ℝ

def dot {d : ℕ} (u v : Vec d) : ℝ := ∑ i, u i * v i

/-- Separate tags prevent accidental confusion between density and momentum. -/
inductive Channel (d : ℕ) where
  | mass
  | momentum (i : Fin d)
  | energy
  deriving DecidableEq, Fintype

abbrev State (d : ℕ) := Channel d → ℝ

def rho {d : ℕ} (U : State d) : ℝ := U .mass
def mom {d : ℕ} (U : State d) : Vec d := fun i => U (.momentum i)
def energy {d : ℕ} (U : State d) : ℝ := U .energy

def state {d : ℕ} (r : ℝ) (m : Vec d) (E : ℝ) : State d
  | .mass => r
  | .momentum i => m i
  | .energy => E

structure IdealGas where
  gamma : ℝ
  gasConstant : ℝ
  gamma_gt_one : 1 < gamma
  gasConstant_pos : 0 < gasConstant

@[ext] structure Primitive (d : ℕ) where
  density : ℝ
  velocity : Vec d
  pressure : ℝ

def internalDensity {d : ℕ} (U : State d) : ℝ :=
  energy U - dot (mom U) (mom U) / (2 * rho U)

def Admissible {d : ℕ} (U : State d) : Prop :=
  0 < rho U ∧ 0 < internalDensity U

def PrimitiveAdmissible {d : ℕ} (W : Primitive d) : Prop :=
  0 < W.density ∧ 0 < W.pressure

def toPrimitive {d : ℕ} (g : IdealGas) (U : State d) : Primitive d :=
  ⟨rho U, fun i => mom U i / rho U, (g.gamma - 1) * internalDensity U⟩

def toConservative {d : ℕ} (g : IdealGas) (W : Primitive d) : State d :=
  state W.density (fun i => W.density * W.velocity i)
    (W.pressure / (g.gamma - 1) + W.density * dot W.velocity W.velocity / 2)

def temperature {d : ℕ} (g : IdealGas) (W : Primitive d) : ℝ :=
  W.pressure / (W.density * g.gasConstant)

def enthalpy {d : ℕ} (g : IdealGas) (U : State d) : ℝ :=
  (energy U + (toPrimitive g U).pressure) / rho U

structure ControlVolume (d : ℕ) where
  center : Vec d
  measure : ℝ
  positive : 0 < measure

structure UnitNormal (d : ℕ) where
  vector : Vec d
  unit : dot vector vector = 1

structure InteriorEdge (N d : ℕ) where
  left : Fin N
  right : Fin N
  distinct : left ≠ right
  normal : UnitNormal d
  area : ℝ
  area_pos : 0 < area

/-- An edge flux is already integrated over the oriented dual face. -/
def edgeContribution {N d : ℕ} (e : InteriorEdge N d) (F : State d) (i : Fin N) : State d :=
  (if i = e.left then F else 0) - (if i = e.right then F else 0)

abbrev Residual (N d : ℕ) := Fin N → State d
abbrev Event (N d : ℕ) := InteriorEdge N d × State d

/-- Mathematical assembly is a sum of signed incidences, including repeated events. -/
def assemble {N d : ℕ} (events : List (Event N d)) : Residual N d :=
  fun i => (events.map fun e => edgeContribution e.1 e.2 i).sum

def explicitUpdate {N d : ℕ} (volumes : Fin N → ControlVolume d) (dt : ℝ)
    (U R : Residual N d) : Residual N d :=
  fun i => U i - (dt / (volumes i).measure) • R i

/-- Applies to states and to conservative increments; increments need not be admissible. -/
def wallDefect {d : ℕ} (n w : Vec d) (U : State d) : ℝ :=
  dot (mom U) n - rho U * dot w n

def Impermeable {d : ℕ} (n w : Vec d) (U : State d) : Prop := wallDefect n w U = 0

/-- Normal correction, with the corresponding wall pressure work in total energy. -/
def wallProject {d : ℕ} (n w : Vec d) (U : State d) : State d :=
  let a := wallDefect n w U
  state (rho U) (fun i => mom U i - a * n i) (energy U - dot w n * a)

/-- Independent relational contract: unchanged density, impermeability, a normal-only
momentum change, and its wall-work energy change. It does not name wallProject. -/
def WallContract {d : ℕ} (n w : Vec d) (U V : State d) : Prop :=
  rho V = rho U ∧ Impermeable n w V ∧
  ∃ a : ℝ, (∀ i, mom V i = mom U i - a * n i) ∧ energy V = energy U - dot w n * a

/-- The component-wise CVariable scalar-overload update, with a shared relaxation factor. -/
def conservativeUpdate {d : ℕ} (U delta : State d) (alpha : ℝ) : State d := U + alpha • delta

end SU2
