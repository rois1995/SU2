import SU2Preprocessing.Execution

set_option maxRecDepth 100000
set_option maxHeartbeats 4000000
noncomputable section
namespace SU2Preprocessing
open SU2 SU2Source SU2Boundary SU2BoundaryMemory SU2Initialization
variable {M : Nat}

@[simp] theorem velocity_addr_eq (p q : Fin M) : velocityAddr p = velocityAddr q ↔ p = q := by
  simp [velocityAddr, Fin.val_inj]

@[simp] theorem primitive_velocity_ne (p q : Fin M) (c : Fin 11) :
    primitiveAddr p c ≠ velocityAddr q := by
  intro h; have e := congrArg Prod.fst h; simp [primitiveAddr, velocityAddr] at e
@[simp] theorem velocity_primitive_ne (p q : Fin M) (c : Fin 11) :
    velocityAddr p ≠ primitiveAddr q c := Ne.symm (primitive_velocity_ne q p c)
@[simp] theorem flow_velocity_ne (b : Nat) (p q : Fin M) (c : Fin 4) (h : b ≠ 7) :
    flowAddr b p c ≠ velocityAddr q := by
  intro e; exact h (congrArg Prod.fst e)
@[simp] theorem flow_primitive_ne (b : Nat) (p q : Fin M) (c : Fin 4) (d : Fin 11) (h : b ≠ 8) :
    flowAddr b p c ≠ primitiveAddr q d := by
  intro e; exact h (congrArg Prod.fst e)

def velocityWrites (p : Fin M) (U : State 2) (m : Mem) : Mem :=
  let vx := mom U 0 / rho U
  let vy := mom U 1 / rho U
  patch [(velocityAddr p,0),(primitiveAddr p 1,vx),(velocityAddr p,0+vx^2),
    (primitiveAddr p 2,vy),(velocityAddr p,(0+vx^2)+vy^2)] m

theorem velocity_identity (U : State 2) :
    (0+(mom U 0 / rho U)^2)+(mom U 1 / rho U)^2 = velocitySquared U := by
  simp [velocitySquared, dot, Fin.sum_univ_two, pow_two]

theorem velocity_other_flow (b : Nat) (h7 : b ≠ 7) (h8 : b ≠ 8)
    (p q : Fin M) (c : Fin 4) (U : State 2) (m : Mem) :
    velocityWrites p U m (flowAddr b q c) = m (flowAddr b q c) := by
  simp [velocityWrites, patch, put, flowAddr, primitiveAddr, velocityAddr, h7, h8, Function.update_apply]

theorem velocity_value (p : Fin M) (U : State 2) (m : Mem) :
    velocityWrites p U m (velocityAddr p) = some (velocitySquared U) := by
  simp [velocityWrites, patch, put, velocityAddr, primitiveAddr, velocitySquared, dot, Fin.sum_univ_two, pow_two, Function.update_apply]

theorem velocity_execution (p : Fin M) (U : State 2) (m : Mem)
    (h : ∀ c, m (flowAddr 0 p c) = some (U (channel c))) :
    executeVelocity p m = some (velocityWrites p U m) := by
  have hr : m (0,p.val*4) = some (rho U) := by simpa [flowAddr,channel,rho] using h 0
  have hx : m (0,p.val*4+1) = some (mom U 0) := by simpa [flowAddr,channel,mom] using h 1
  have hy : m (0,p.val*4+2) = some (mom U 1) := by simpa [flowAddr,channel,mom] using h 2
  simp [executeVelocity, velocityWrites, patch, put, flowAddr, primitiveAddr, velocityAddr,
    hr, hx, hy, Function.update_apply]

theorem fluid_ready_preserved (f : Fluid) (density e : ℝ) (g : IdealGas) (h : FluidReady g f) :
    FluidReady g (executeFluid f density e) := by
  cases hm : f.entropyEnabled <;> simpa [FluidReady, executeFluid, hm] using h

theorem fluid_frame (f : Fluid) (density e : ℝ) :
    (executeFluid f density e).parameters = f.parameters ∧
    (executeFluid f density e).entropyEnabled = f.entropyEnabled ∧
    (executeFluid f density e).untouched = f.untouched := by
  cases hm : f.entropyEnabled <;> simp [executeFluid, hm]

theorem fluid_values (g : IdealGas) (f : Fluid) (U : State 2) (h : FluidReady g f) :
    (executeFluid f (rho U) (staticEnergy U)).density = rho U ∧
    (executeFluid f (rho U) (staticEnergy U)).staticEnergy = staticEnergy U ∧
    (executeFluid f (rho U) (staticEnergy U)).dPdrho = (g.gamma-1)*staticEnergy U ∧
    (executeFluid f (rho U) (staticEnergy U)).dPde = (g.gamma-1)*rho U ∧
    (executeFluid f (rho U) (staticEnergy U)).pressure = stagedPressure g U ∧
    (executeFluid f (rho U) (staticEnergy U)).temperature = stagedTemperature g U ∧
    (executeFluid f (rho U) (staticEnergy U)).sound2 = stagedSound2 g U := by
  unfold FluidReady at h
  cases hm : f.entropyEnabled <;> simp [executeFluid, hm, h, gasParameters,
    stagedPressure, stagedTemperature, stagedSound2]

theorem fluid_entropy (f : Fluid) (density e : ℝ) :
    (executeFluid f density e).entropy =
      if f.entropyEnabled then
        f.parameters.cv * Real.log (f.parameters.dTde * e) - f.parameters.gasConstant * Real.log density
      else f.entropy := by
  cases hm : f.entropyEnabled <;> simp [executeFluid, hm]

def thermoWrites (p : Fin M) (U : State 2) (s : Machine) : Machine :=
  ⟨velocityWrites p U s.memory, executeFluid s.fluid (rho U) (staticEnergy U)⟩

theorem thermo_execution (p : Fin M) (U : State 2) (s : Machine)
    (h : ∀ c, s.memory (flowAddr 0 p c) = some (U (channel c))) :
    executeThermo p s = some (thermoWrites p U s) := by
  have hr : velocityWrites p U s.memory (flowAddr 0 p 0) = some (rho U) :=
    (velocity_other_flow 0 (by decide) (by decide) p p 0 U s.memory).trans (h 0)
  have he : velocityWrites p U s.memory (flowAddr 0 p 3) = some (energy U) :=
    (velocity_other_flow 0 (by decide) (by decide) p p 3 U s.memory).trans (h 3)
  have er : energy U / rho U - (2 : ℝ)⁻¹ * velocitySquared U = staticEnergy U := by
    unfold staticEnergy; ring
  simp [executeThermo, velocity_execution p U s.memory h, hr, he,
    velocity_value, er, thermoWrites]

def checkWrites (p : Fin M) (g : IdealGas) (U : State 2) (m : Mem) : Mem :=
  let m := put m (primitiveAddr p 4) (rho U)
  let m := put m (primitiveAddr p 3) (stagedPressure g U)
  let m := if stagedSound2 g U < 0 then m else put m (primitiveAddr p 6) (Real.sqrt (stagedSound2 g U))
  put m (primitiveAddr p 0) (stagedTemperature g U)

def stageWrites (p : Fin M) (g : IdealGas) (U : State 2) (s : Machine) : Machine :=
  ⟨checkWrites p g U (velocityWrites p U s.memory),executeFluid s.fluid (rho U) (staticEnergy U)⟩

theorem stage_execution (p : Fin M) (g : IdealGas) (U : State 2) (s : Machine)
    (h : ∀ c, s.memory (flowAddr 0 p c) = some (U (channel c))) (hf : FluidReady g s.fluid) :
    executeStage p s = some (stageWrites p g U s, decide (rejected g U)) := by
  have hr : velocityWrites p U s.memory (flowAddr 0 p 0) = some (rho U) :=
    (velocity_other_flow 0 (by decide) (by decide) p p 0 U s.memory).trans (h 0)
  obtain ⟨_,_,_,_,hp,ht,hs⟩ := fluid_values g s.fluid U hf
  have flags : (decide (rho U ≤ 0) || decide (stagedPressure g U ≤ 0) ||
      decide (stagedSound2 g U < 0) || decide (stagedTemperature g U ≤ 0)) = decide (rejected g U) := by
    simp [rejected, Bool.or_assoc]
  simp only [executeStage, thermo_execution p U s h, Option.some_bind, executeChecks, thermoWrites,
    hr, hp, ht, hs]
  by_cases hn : stagedSound2 g U < 0 <;>
    simp [checkWrites, stageWrites, hn, hr, hp, ht, hs, flags, rejected, Bool.or_assoc]

theorem stage_other_flow (p q : Fin M) (b : Nat) (h7 : b ≠ 7) (h8 : b ≠ 8)
    (c : Fin 4) (g : IdealGas) (U : State 2) (s : Machine) :
    (stageWrites p g U s).memory (flowAddr b q c) = s.memory (flowAddr b q c) := by
  by_cases hn : stagedSound2 g U < 0 <;>
    simpa [stageWrites, checkWrites, hn, put, flowAddr, primitiveAddr, h8, Function.update_apply] using
      velocity_other_flow b h7 h8 p q c U s.memory

theorem stage_primitives (p : Fin M) (g : IdealGas) (U : State 2) (s : Machine)
    (P : Primitives M) (h : PrimitiveRep s.memory P) :
    PrimitiveRep (stageWrites p g U s).memory (Function.update P p (stagePoint g U (P p))) := by
  intro q c
  by_cases hq : q = p <;> by_cases hn : stagedSound2 g U < 0
  all_goals subst_vars
  all_goals simp only [stageWrites, checkWrites, stagePoint, hn]
  all_goals fin_cases c
  all_goals simp_all [stageWrites, checkWrites, velocityWrites, patch, put, stagePoint,
    Function.update_apply, PrimitiveRep]

theorem stage_velocity (p : Fin M) (g : IdealGas) (U : State 2) (s : Machine)
    (V : Fin M → Option ℝ) (h : VelocityRep s.memory V) :
    VelocityRep (stageWrites p g U s).memory (Function.update V p (some (velocitySquared U))) := by
  intro q
  by_cases hq : q = p <;> by_cases hn : stagedSound2 g U < 0
  all_goals subst_vars
  all_goals simp only [stageWrites, checkWrites, hn]
  all_goals simp_all [VelocityRep, stageWrites, checkWrites, velocityWrites, patch, put,
    velocitySquared, dot, Fin.sum_univ_two, pow_two, Function.update_apply]

end SU2Preprocessing
