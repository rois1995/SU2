import SU2Initialization
import Mathlib.Analysis.SpecialFunctions.Log.Basic

noncomputable section
namespace SU2Preprocessing
open SU2 SU2Source SU2Boundary SU2BoundaryMemory SU2Initialization
variable {M : Nat}

structure GasParameters where
  gamma : ℝ
  gammaMinusOne : ℝ
  gasConstant : ℝ
  cp : ℝ
  cv : ℝ
  dTdrho : ℝ
  dTde : ℝ

def gasParameters (g : IdealGas) : GasParameters :=
  let gm := g.gamma - 1
  let cp := g.gamma / gm * g.gasConstant
  ⟨g.gamma,gm,g.gasConstant,cp,cp-g.gasConstant,0,gm/g.gasConstant⟩

/-- The separate fluid object is disjoint from point buffers in this logical model.
Concrete object allocation, aliasing, pointers and lifetime remain C++ obligations. -/
structure Fluid where
  parameters : GasParameters
  entropyEnabled : Bool
  density : ℝ
  staticEnergy : ℝ
  dPdrho : ℝ
  dPde : ℝ
  pressure : ℝ
  temperature : ℝ
  sound2 : ℝ
  entropy : ℝ
  untouched : Nat → ℝ

structure Machine where
  memory : Mem
  fluid : Fluid

def freshFluid (g : IdealGas) (entropyEnabled : Bool) : Fluid :=
  ⟨gasParameters g,entropyEnabled,0,0,0,0,0,0,0,0,fun _ => 0⟩

def FluidReady (g : IdealGas) (f : Fluid) : Prop := f.parameters = gasParameters g

theorem fresh_fluid_ready (g : IdealGas) (enabled : Bool) : FluidReady g (freshFluid g enabled) := rfl

def velocityAddr (p : Fin M) : Address := (7,p.val)

def patch (entries : List (Address × ℝ)) (m : Mem) : Mem :=
  entries.foldl (fun mem entry => put mem entry.1 entry.2) m

theorem patch_frame (entries : List (Address × ℝ)) (m : Mem) :
    Frame (entries.map Prod.fst) m (patch entries m) := by
  induction entries generalizing m with
  | nil => exact frame_refl _ _
  | cons e es ih =>
    simpa only [patch, List.foldl_cons, List.map_cons, List.singleton_append] using
      frame_trans (put_frame m e.1 e.2) (ih (put m e.1 e.2))

def rowFootprint (p : Fin M) : List Address :=
  (List.ofFn (fun c : Fin 4 => flowAddr 0 p c)) ++
  (List.ofFn (fun c : Fin 11 => primitiveAddr p c)) ++ [velocityAddr p]

theorem row_flow_mem (p : Fin M) (c : Fin 4) : flowAddr 0 p c ∈ rowFootprint p := by
  apply List.mem_append.mpr
  exact Or.inl (List.mem_append.mpr (Or.inl (List.mem_ofFn.mpr ⟨c,rfl⟩)))
theorem row_primitive_mem (p : Fin M) (c : Fin 11) : primitiveAddr p c ∈ rowFootprint p := by
  apply List.mem_append.mpr
  exact Or.inl (List.mem_append.mpr (Or.inr (List.mem_ofFn.mpr ⟨c,rfl⟩)))
theorem row_velocity_mem (p : Fin M) : velocityAddr p ∈ rowFootprint p := by simp [rowFootprint]

theorem frame_weaken {xs ys : List Address} {m n : Mem}
    (h : Frame xs m n) (inc : ∀ a, a ∈ xs → a ∈ ys) : Frame ys m n := by
  intro a ha
  exact h a (fun hx => ha (inc a hx))

def readFlow (b : Nat) (p : Fin M) (m : Mem) : Option (State 2) := do
  let density ← m (flowAddr b p 0)
  let mx ← m (flowAddr b p 1)
  let my ← m (flowAddr b p 2)
  let energy ← m (flowAddr b p 3)
  return state density ![mx,my] energy

theorem read_flow_correct (b : Nat) (p : Fin M) (m : Mem) (U : State 2)
    (h : ∀ c, m (flowAddr b p c) = some (U (channel c))) : readFlow b p m = some U := by
  have eq := components_roundtrip U
  simpa only [readFlow, h, Option.some_bind, pure, fromComponents, components, channel] using congrArg some eq

def CurrentRep (m : Mem) (U : Residual M 2) : Prop :=
  ∀ p c, m (flowAddr 0 p c) = some (U p (channel c))
def OldRep (m : Mem) (U : Residual M 2) : Prop :=
  ∀ p c, m (flowAddr 1 p c) = some (U p (channel c))
def VelocityRep (m : Mem) (V : Fin M → Option ℝ) : Prop := ∀ p, m (velocityAddr p) = V p

end SU2Preprocessing
