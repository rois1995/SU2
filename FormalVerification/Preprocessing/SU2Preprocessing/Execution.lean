import SU2Preprocessing.Memory

noncomputable section
namespace SU2Preprocessing
open SU2 SU2Source SU2Boundary SU2BoundaryMemory SU2Initialization
variable {M : Nat}

/-- Full source order, including reset, freshly stored primitive reads and scratch accumulation. -/
def executeVelocity (p : Fin M) (m : Mem) : Option Mem := do
  let m := put m (velocityAddr p) 0
  let mx ← m (flowAddr 0 p 1)
  let density ← m (flowAddr 0 p 0)
  let m := put m (primitiveAddr p 1) (mx / density)
  let vx ← m (primitiveAddr p 1)
  let v2 ← m (velocityAddr p)
  let m := put m (velocityAddr p) (v2 + vx^2)
  let my ← m (flowAddr 0 p 2)
  let density ← m (flowAddr 0 p 0)
  let m := put m (primitiveAddr p 2) (my / density)
  let vy ← m (primitiveAddr p 2)
  let v2 ← m (velocityAddr p)
  return put m (velocityAddr p) (v2 + vy^2)

/-- Selected CIdealGas::SetTDState_rhoe, including the enabled entropy branch. -/
def executeFluid (f : Fluid) (density specificInternal : ℝ) : Fluid :=
  let f := {f with density := density}
  let f := {f with staticEnergy := specificInternal}
  let f := {f with dPdrho := f.parameters.gammaMinusOne * f.staticEnergy}
  let f := {f with dPde := f.parameters.gammaMinusOne * f.density}
  let f := {f with pressure := f.dPde * f.staticEnergy}
  let f := {f with temperature := f.parameters.dTde * f.staticEnergy}
  let f := {f with sound2 := f.parameters.gamma * f.parameters.gasConstant * f.temperature}
  if f.entropyEnabled then
    {f with entropy := f.parameters.cv * Real.log f.temperature - f.parameters.gasConstant * Real.log f.density}
  else f

def executeThermo (p : Fin M) (s : Machine) : Option Machine := do
  let m ← executeVelocity p s.memory
  let density ← m (flowAddr 0 p 0)
  let energy ← m (flowAddr 0 p 3)
  let densityAgain ← m (flowAddr 0 p 0)
  let v2 ← m (velocityAddr p)
  let staticEnergy := energy / densityAgain - (1/2 : ℝ) * v2
  return ⟨m, executeFluid s.fluid density staticEnergy⟩

/-- All four setters run before the disjunction is tested. Negative sound2 skips its write. -/
def executeChecks (p : Fin M) (s : Machine) : Option (Machine × Bool) := do
  let density ← s.memory (flowAddr 0 p 0)
  let m := put s.memory (primitiveAddr p 4) density
  let badDensity := decide (density ≤ 0)
  let pressure := s.fluid.pressure
  let m := put m (primitiveAddr p 3) pressure
  let badPressure := decide (pressure ≤ 0)
  let sound2 := s.fluid.sound2
  let badSound := decide (sound2 < 0)
  let m := if badSound then m else put m (primitiveAddr p 6) (Real.sqrt sound2)
  let temperature := s.fluid.temperature
  let m := put m (primitiveAddr p 0) temperature
  let badTemperature := decide (temperature ≤ 0)
  return (⟨m,s.fluid⟩, badDensity || badPressure || badSound || badTemperature)

def executeStage (p : Fin M) (s : Machine) : Option (Machine × Bool) := do
  let s ← executeThermo p s
  executeChecks p s

/-- The fallback really reads old cells and updates current cells one at a time. -/
def copyBackup (p : Fin M) : List (Fin 4) → Mem → Option Mem
  | [], m => some m
  | c :: cs, m => do
    let x ← m (flowAddr 1 p c)
    copyBackup p cs (put m (flowAddr 0 p c) x)

def executeEnthalpy (p : Fin M) (s : Machine) : Option Machine := do
  let energy ← s.memory (flowAddr 0 p 3)
  let pressure ← s.memory (primitiveAddr p 3)
  let density ← s.memory (flowAddr 0 p 0)
  return ⟨put s.memory (primitiveAddr p 5) ((energy + pressure) / density),s.fluid⟩

def executePoint (p : Fin M) (s : Machine) : Option (Machine × Bool) := do
  let (s,bad) ← executeStage p s
  let s ← if bad then do
      let m ← copyBackup p (List.ofFn (fun c : Fin 4 => c)) s.memory
      let (next,_) ← executeStage p ⟨m,s.fluid⟩
      pure next
    else pure s
  let s ← executeEnthalpy p s
  return (s,!bad)

/-- Serial traversal includes every allocated row, regardless of owned/halo membership. -/
def executePoints : List (Fin M) → Machine → Nat → Option (Machine × Nat)
  | [], s, count => some (s,count)
  | p :: ps, s, count => do
    let (s,physical) ← executePoint p s
    executePoints ps s (count + if physical then 0 else 1)

def executePreprocessing (s : Machine) : Option (Machine × Nat) := do
  let (s,count) ← executePoints (List.ofFn (fun p : Fin M => p)) s 0
  return (⟨clearResidual (N := M) s.memory,s.fluid⟩,count)

def executePreprocessingSnapshot (s : Machine) : Option (Machine × Nat) := do
  let (s,count) ← executePreprocessing (M := M) s
  let m ← executeSnapshot (M := M) s.memory
  return (⟨m,s.fluid⟩,count)

end SU2Preprocessing
