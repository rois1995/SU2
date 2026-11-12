import SU2Euler
import Mathlib.Analysis.SpecialFunctions.Pow.Real
import Mathlib.Data.Matrix.Notation
import Mathlib.Tactic.FinCases

noncomputable section
open scoped BigOperators
namespace SU2Boundary
open SU2

/-- This new candidate selects two-dimensional compressible, stationary, serial Euler. -/
def channel : Fin 4 → Channel 2 := ![.mass, .momentum 0, .momentum 1, .energy]
def components (U : State 2) : Fin 4 → ℝ := fun c => U (channel c)
def fromComponents (u : Fin 4 → ℝ) : State 2 := state (u 0) ![u 1, u 2] (u 3)

theorem components_roundtrip (U : State 2) : fromComponents (components U) = U := by
  funext c
  cases c with
  | mass => rfl
  | energy => rfl
  | momentum i => fin_cases i <;> rfl

theorem channel_index (c : Fin 4) : SU2Euler.componentIndex (channel c) = c.val := by
  fin_cases c <;> rfl

def sound (g : IdealGas) (W : Primitive 2) : ℝ := Real.sqrt (g.gamma * W.pressure / W.density)
def specificEnergy (g : IdealGas) (W : Primitive 2) : ℝ :=
  W.pressure / ((g.gamma - 1) * W.density) + dot W.velocity W.velocity / 2
def totalEnthalpy (g : IdealGas) (W : Primitive 2) : ℝ := specificEnergy g W + W.pressure / W.density
def entropy (g : IdealGas) (W : Primitive 2) : ℝ := Real.rpow W.density g.gamma / W.pressure

def rPlus (g : IdealGas) (n : Vec 2) (W : Primitive 2) : ℝ :=
  dot W.velocity n + 2 * sound g W / (g.gamma - 1)
def rMinus (g : IdealGas) (n : Vec 2) (W : Primitive 2) : ℝ :=
  dot W.velocity n - 2 * sound g W / (g.gamma - 1)

/-- Incoming/outgoing characteristic selection. Strict inequalities include sonic and
zero-normal-velocity cases in their actual else branches. No admissibility is assumed here. -/
def characteristics (g : IdealGas) (n : Vec 2) (inside free : Primitive 2) : ℝ × ℝ :=
  let q := dot free.velocity n
  (if -sound g free < q then rPlus g n inside else rPlus g n free,
   if sound g free < q then rMinus g n inside else rMinus g n free)

def advected (n : Vec 2) (inside free : Primitive 2) : Primitive 2 :=
  if 0 < dot free.velocity n then inside else free

/-- Independent far-field reconstruction from characteristic and advected data. -/
def farPrimitive (g : IdealGas) (n : Vec 2) (inside free : Primitive 2) : Primitive 2 :=
  let rs := characteristics g n inside free
  let vn := (rs.1 + rs.2) / 2
  let c := (rs.1 - rs.2) * (g.gamma - 1) / 4
  let W := advected n inside free
  let density := Real.rpow (entropy g W * c * c / g.gamma) (1 / (g.gamma - 1))
  ⟨density, fun j => W.velocity j + (vn - dot W.velocity n) * n j,
   density * c * c / g.gamma⟩

/-- Characteristic primitive cache entries touched by the production routine. -/
structure Ghost where
  primitive : Primitive 2
  temperature : ℝ
  enthalpy : ℝ

def ghost (g : IdealGas) (W : Primitive 2) : Ghost :=
  ⟨W, W.pressure / (g.gasConstant * W.density), totalEnthalpy g W⟩

/-- Physical Euler flux through an area normal (not a unit normal). -/
def physicalFlux (g : IdealGas) (normal : Vec 2) (W : Primitive 2) : Fin 4 → ℝ :=
  let q := dot W.velocity normal
  ![W.density * q, W.density * q * W.velocity 0 + W.pressure * normal 0,
    W.density * q * W.velocity 1 + W.pressure * normal 1,
    W.density * totalEnthalpy g W * q]

/-- Closed 2D Roe right eigenvectors in the selected production normalization. -/
def roeP (g : IdealGas) (r c : ℝ) (v n : Vec 2) : Fin 4 → Fin 4 → ℝ :=
  let k := dot v v / 2
  let q := dot v n
  ![![1, 0, r / c / 2, r / c / 2],
    ![v 0, r * n 1, (v 0 * (r / c) + n 0 * r) / 2, (v 0 * (r / c) - n 0 * r) / 2],
    ![v 1, -r * n 0, (v 1 * (r / c) + n 1 * r) / 2, (v 1 * (r / c) - n 1 * r) / 2],
    ![k, r * (v 0 * n 1 - v 1 * n 0),
      (k * (r / c) + r * q + r * c / (g.gamma - 1)) / 2,
      (k * (r / c) - r * q + r * c / (g.gamma - 1)) / 2]]

/-- Explicit formula used as the left eigenvector matrix; no inverse identity is assumed. -/
def roeB (g : IdealGas) (r c : ℝ) (v n : Vec 2) : Fin 4 → Fin 4 → ℝ :=
  let k := dot v v / 2
  let q := dot v n / r
  let b := (g.gamma - 1) / c^2
  let a := (g.gamma - 1) / (r * c)
  ![![1 - b * k, b * v 0, b * v 1, -b],
    ![-n 1 / r * v 0 + n 0 / r * v 1, n 1 / r, -n 0 / r, 0],
    ![-q + a * k, n 0 / r - a * v 0, n 1 / r - a * v 1, a],
    ![q + a * k, -n 0 / r - a * v 0, -n 1 / r - a * v 1, a]]

/-- Selected scalar boundary Roe flux: kappa=1/2, entropy fix=0, no low dissipation,
no grid movement. The nonpositive Roe sound-speed-squared branch returns zero.
Exact-real totalization does not imply validity of C++ divisions/pow for inadmissible states. -/
def boundaryRoe (g : IdealGas) (n : Vec 2) (area : ℝ) (L R : Primitive 2) : Fin 4 → ℝ :=
  let ratio := Real.sqrt |R.density / L.density|
  let r := ratio * L.density
  let v := fun j => (ratio * R.velocity j + L.velocity j) / (ratio + 1)
  let H := (ratio * totalEnthalpy g R + totalEnthalpy g L) / (ratio + 1)
  let c2 := (g.gamma - 1) * (H - dot v v / 2)
  if c2 ≤ 0 then 0 else
  let c := Real.sqrt c2
  let P := roeP g r c v n
  let B := roeB g r c v n
  let q := dot v n
  let lambda : Fin 4 → ℝ := ![|q|, |q|, |q + c|, |q - c|]
  let delta := fun j => components (toConservative g R) j - components (toConservative g L) j
  fun i => (physicalFlux g (fun j => area * n j) L i +
            physicalFlux g (fun j => area * n j) R i) / 2 -
    area / 2 * ∑ j, (∑ k, P i k * lambda k * B k j) * delta j

structure BoundaryState (N : Nat) where
  current : Residual N 2
  old : Residual N 2
  residual : Residual N 2
  truncation : Residual N 2
  cache : Nat → Option Ghost

structure FarVisit (N : Nat) where
  point : Fin N
  cacheIndex : Nat
  owned : Bool
  normal : UnitNormal 2
  area : ℝ
  area_pos : 0 < area
  inside : Primitive 2
  insideEnthalpy : ℝ

structure WallVisit (N : Nat) where
  point : Fin N
  owned : Bool
  normal : UnitNormal 2

/-- Far-field is flux accumulation plus cache update. Primitive input is a snapshot
of the preprocessed primitive buffer; its relationship to conservative current is an obligation. -/
def farSpec {N : Nat} (g : IdealGas) (free : Primitive 2) (v : FarVisit N)
    (s : BoundaryState N) : BoundaryState N :=
  if v.owned then
    let W := farPrimitive g v.normal.vector v.inside free
    let F := fromComponents (boundaryRoe g v.normal.vector v.area v.inside W)
    {s with residual := Function.update s.residual v.point (s.residual v.point + F),
            cache := Function.update s.cache v.cacheIndex (some (ghost g W))}
  else s

/-- Final stationary wall changes three buffers, with no stationary energy flux addition. -/
def wallSpec {N : Nat} (v : WallVisit N) (s : BoundaryState N) : BoundaryState N :=
  if v.owned then
    {s with old := Function.update s.old v.point (wallProject v.normal.vector 0 (s.old v.point)),
            residual := Function.update s.residual v.point (wallProject v.normal.vector 0 (s.residual v.point)),
            truncation := Function.update s.truncation v.point (wallProject v.normal.vector 0 (s.truncation v.point))}
  else s

def boundarySpec {N : Nat} (g : IdealGas) (free : Primitive 2)
    (far : List (FarVisit N)) (walls : List (WallVisit N)) (s : BoundaryState N) : BoundaryState N :=
  walls.foldl (fun acc v => wallSpec v acc) (far.foldl (fun acc v => farSpec g free v acc) s)

/-- Post-boundary old, residual and truncation enter the independent Euler update. -/
def updateSpec {N : Nat} (vol dt : Fin N → ℝ) (s : BoundaryState N) : Residual N 2 :=
  fun p c => s.old p c - dt p / vol p * (s.residual p c + s.truncation p c)

end SU2Boundary
