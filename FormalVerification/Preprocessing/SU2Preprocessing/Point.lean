import SU2Preprocessing.Stage

set_option maxRecDepth 100000
set_option maxHeartbeats 8000000
noncomputable section
namespace SU2Preprocessing
open SU2 SU2Source SU2Boundary SU2BoundaryMemory SU2Initialization
variable {M : Nat}

@[simp] theorem primitive_flow_ne (b : Nat) (p q : Fin M) (c : Fin 11) (d : Fin 4) (h : b ≠ 8) :
    primitiveAddr p c ≠ flowAddr b q d := Ne.symm (flow_primitive_ne b q p d c h)
@[simp] theorem velocity_flow_ne (b : Nat) (p q : Fin M) (c : Fin 4) (h : b ≠ 7) :
    velocityAddr p ≠ flowAddr b q c := Ne.symm (flow_velocity_ne b q p c h)

def backupWrites (p : Fin M) (B : State 2) (m : Mem) : Mem :=
  writeFields (flowAddr 0 p) (components B) (List.ofFn (fun c : Fin 4 => c)) m

theorem backup_execution (p : Fin M) (B : State 2) (cs : List (Fin 4)) (m : Mem)
    (h : ∀ c, m (flowAddr 1 p c) = some (B (channel c))) :
    copyBackup p cs m = some (writeFields (flowAddr 0 p) (components B) cs m) := by
  induction cs generalizing m with
  | nil => rfl
  | cons c cs ih =>
    simp only [copyBackup, h c, Option.some_bind, writeFields]
    apply ih
    intro d
    simpa [put, Function.update_apply] using h d

theorem backup_current (p : Fin M) (B : State 2) (m : Mem) (c : Fin 4) :
    backupWrites p B m (flowAddr 0 p c) = some (B (channel c)) := by
  have inj : Function.Injective (flowAddr 0 p) := fun i j h => (flow_addr_injective 0 h).2
  have hc : c ∈ List.ofFn (fun c : Fin 4 => c) := List.mem_ofFn.mpr ⟨c,rfl⟩
  simp only [backupWrites, write_fields_value _ inj, if_pos hc, components]

theorem backup_old (p q : Fin M) (B : State 2) (m : Mem) (c : Fin 4) :
    backupWrites p B m (flowAddr 1 q c) = m (flowAddr 1 q c) := by
  simp [backupWrites, writeFields, put, Function.update_apply]

theorem stage_pressure (p : Fin M) (g : IdealGas) (U : State 2) (s : Machine) :
    (stageWrites p g U s).memory (primitiveAddr p 3) = some (stagedPressure g U) := by
  by_cases hn : stagedSound2 g U < 0 <;>
    simp [stageWrites, checkWrites, hn, put, Function.update_apply]

theorem fluid_overwrite (f : Fluid) (r e r' e' : ℝ) :
    executeFluid (executeFluid f r e) r' e' = executeFluid f r' e' := by
  cases hm : f.entropyEnabled <;> simp [executeFluid, hm]

def pointWrites (p : Fin M) (g : IdealGas) (U B : State 2) (s : Machine) : Machine :=
  let first := stageWrites p g U s
  let selected := if rejected g U then B else U
  let after := if rejected g U then
      stageWrites p g B ⟨backupWrites p B first.memory,first.fluid⟩
    else first
  ⟨put after.memory (primitiveAddr p 5)
    ((energy selected + stagedPressure g selected) / rho selected),after.fluid⟩

theorem point_execution (p : Fin M) (g : IdealGas) (U B : State 2) (s : Machine)
    (hc : ∀ c, s.memory (flowAddr 0 p c) = some (U (channel c)))
    (hb : ∀ c, s.memory (flowAddr 1 p c) = some (B (channel c)))
    (hf : FluidReady g s.fluid) :
    executePoint p s = some (pointWrites p g U B s, !decide (rejected g U)) := by
  have ec := stage_execution p g U s hc hf
  have oldFirst : ∀ c, (stageWrites p g U s).memory (flowAddr 1 p c) = some (B (channel c)) :=
    fun c => (stage_other_flow p p 1 (by decide) (by decide) c g U s).trans (hb c)
  have fluidFirst : FluidReady g (stageWrites p g U s).fluid := fluid_ready_preserved _ _ _ _ hf
  by_cases hn : rejected g U
  · let mid : Machine := ⟨backupWrites p B (stageWrites p g U s).memory,(stageWrites p g U s).fluid⟩
    have copy := backup_execution p B (List.ofFn (fun c : Fin 4 => c)) _ oldFirst
    have hmid : ∀ c, mid.memory (flowAddr 0 p c) = some (B (channel c)) := backup_current p B _
    have eb := stage_execution p g B mid hmid fluidFirst
    have hcur : ∀ c, (stageWrites p g B mid).memory (flowAddr 0 p c) = some (B (channel c)) :=
      fun c => (stage_other_flow p p 0 (by decide) (by decide) c g B mid).trans (hmid c)
    have en : executeEnthalpy p (stageWrites p g B mid) =
        some (pointWrites p g U B s) := by
      simp [executeEnthalpy, hcur 3, hcur 0, stage_pressure, pointWrites, hn, mid, channel, rho, energy]
    have cp : copyBackup p (List.ofFn (fun c : Fin 4 => c)) (stageWrites p g U s).memory =
        some (backupWrites p B (stageWrites p g U s).memory) := copy
    change copyBackup p [0,1,2,3] (stageWrites p g U s).memory =
      some (backupWrites p B (stageWrites p g U s).memory) at cp
    dsimp only [mid] at eb en
    simp [executePoint, ec, hn, cp, eb, en] 

  · have hcur : ∀ c, (stageWrites p g U s).memory (flowAddr 0 p c) = some (U (channel c)) :=
      fun c => (stage_other_flow p p 0 (by decide) (by decide) c g U s).trans (hc c)
    have en : executeEnthalpy p (stageWrites p g U s) =
        some (pointWrites p g U B s) := by
      simp [executeEnthalpy, hcur 3, hcur 0, stage_pressure, pointWrites, hn, channel, rho, energy]
    simp [executePoint, ec, hn, en]

theorem point_old (p q : Fin M) (c : Fin 4) (g : IdealGas) (U B : State 2) (s : Machine) :
    (pointWrites p g U B s).memory (flowAddr 1 q c) = s.memory (flowAddr 1 q c) := by
  by_cases hn : rejected g U <;>
    simp [pointWrites, hn, put, Function.update_apply, stage_other_flow, backup_old]

theorem point_fluid (p : Fin M) (g : IdealGas) (U B : State 2) (s : Machine) :
    (pointWrites p g U B s).fluid =
      executeFluid s.fluid (rho (if rejected g U then B else U))
        (staticEnergy (if rejected g U then B else U)) := by
  by_cases hn : rejected g U <;> simp [pointWrites, stageWrites, hn, fluid_overwrite]


theorem backup_current_rep (p : Fin M) (B : State 2) (m : Mem) (U : Residual M 2)
    (h : CurrentRep m U) : CurrentRep (backupWrites p B m) (Function.update U p B) := by
  intro q c
  by_cases hq : q = p
  · subst q; simpa only [Function.update_self] using backup_current p B m c
  · simp [backupWrites, writeFields, put, hq, Function.update_apply, h q c]

theorem backup_primitive_rep (p : Fin M) (B : State 2) (m : Mem) (P : Primitives M)
    (h : PrimitiveRep m P) : PrimitiveRep (backupWrites p B m) P := by
  intro q c
  simpa [backupWrites, writeFields, put, Function.update_apply] using h q c

theorem backup_velocity_rep (p : Fin M) (B : State 2) (m : Mem) (V : Fin M → Option ℝ)
    (h : VelocityRep m V) : VelocityRep (backupWrites p B m) V := by
  intro q
  simpa [backupWrites, writeFields, put, Function.update_apply] using h q

theorem primitive_put_rep (p : Fin M) (c : Fin 11) (x : ℝ) (m : Mem) (P : Primitives M)
    (h : PrimitiveRep m P) :
    PrimitiveRep (put m (primitiveAddr p c) x) (Function.update P p (Function.update (P p) c x)) := by
  intro q j
  by_cases hp : q = p <;> by_cases hc : j = c
  all_goals subst_vars
  all_goals simp_all [put, Function.update_apply, PrimitiveRep]

theorem current_primitive_put_rep (p : Fin M) (c : Fin 11) (x : ℝ) (m : Mem) (U : Residual M 2)
    (h : CurrentRep m U) : CurrentRep (put m (primitiveAddr p c) x) U := by
  intro q j
  simpa [put, Function.update_apply] using h q j

theorem velocity_primitive_put_rep (p : Fin M) (c : Fin 11) (x : ℝ) (m : Mem) (V : Fin M → Option ℝ)
    (h : VelocityRep m V) : VelocityRep (put m (primitiveAddr p c) x) V := by
  intro q
  simpa [put, Function.update_apply] using h q

theorem stage_current_rep (p : Fin M) (g : IdealGas) (U : State 2) (s : Machine)
    (C : Residual M 2) (h : CurrentRep s.memory C) : CurrentRep (stageWrites p g U s).memory C :=
  fun q c => (stage_other_flow p q 0 (by decide) (by decide) c g U s).trans (h q c)

def chosen (g : IdealGas) (U B : State 2) : State 2 := if rejected g U then B else U

theorem point_current (p : Fin M) (g : IdealGas) (U B : Residual M 2) (s : Machine)
    (h : CurrentRep s.memory U) :
    CurrentRep (pointWrites p g (U p) (B p) s).memory
      (Function.update U p (chosen g (U p) (B p))) := by
  have first := stage_current_rep p g (U p) s U h
  by_cases hn : rejected g (U p)
  · have copied := backup_current_rep p (B p) _ U first
    have second := stage_current_rep p g (B p)
      ⟨backupWrites p (B p) (stageWrites p g (U p) s).memory,(stageWrites p g (U p) s).fluid⟩ _ copied
    simpa only [pointWrites, chosen, if_pos hn] using current_primitive_put_rep p 5
      ((energy (B p)+stagedPressure g (B p))/rho (B p)) _ _ second
  · have finished := current_primitive_put_rep p 5
      ((energy (U p)+stagedPressure g (U p))/rho (U p)) _ _ first
    have eq : Function.update U p (U p) = U := by funext q; by_cases hq : q = p <;> simp_all [Function.update_apply]
    simpa only [pointWrites, chosen, if_neg hn, eq] using finished

theorem stage_point_pressure (g : IdealGas) (U : State 2) (P : Fin 11 → ℝ) :
    stagePoint g U P 3 = stagedPressure g U := rfl

def intermediateFields (g : IdealGas) (U B : State 2) (P : Fin 11 → ℝ) : Fin 11 → ℝ :=
  if rejected g U then stagePoint g B (stagePoint g U P) else stagePoint g U P

theorem point_primitive_formula (g : IdealGas) (U B : State 2) (P : Fin 11 → ℝ) :
    (setPoint g U B P).primitive = Function.update (intermediateFields g U B P) 5
      ((energy (chosen g U B)+stagedPressure g (chosen g U B))/rho (chosen g U B)) := by
  by_cases hn : rejected g U <;>
    simp only [setPoint, intermediateFields, chosen, hn, if_true, if_false, stage_point_pressure]

theorem point_primitives (p : Fin M) (g : IdealGas) (U B : State 2) (s : Machine)
    (P : Primitives M) (h : PrimitiveRep s.memory P) :
    PrimitiveRep (pointWrites p g U B s).memory
      (Function.update P p (setPoint g U B (P p)).primitive) := by
  let first := stageWrites p g U s
  let P1 := Function.update P p (stagePoint g U (P p))
  have h1 : PrimitiveRep first.memory P1 := stage_primitives p g U s P h
  let after := if rejected g U then stageWrites p g B ⟨backupWrites p B first.memory,first.fluid⟩ else first
  let P2 := Function.update P p (intermediateFields g U B (P p))
  have h2 : PrimitiveRep after.memory P2 := by
    by_cases hn : rejected g U
    · have copied := backup_primitive_rep p B first.memory P1 h1
      have second := stage_primitives p g B ⟨backupWrites p B first.memory,first.fluid⟩ P1 copied
      have eq : Function.update P1 p (stagePoint g B (P1 p)) = P2 := by
        funext q
        by_cases hq : q = p <;> simp_all [P1, P2, intermediateFields, Function.update_apply]
      simpa only [after, if_pos hn, eq] using second
    · simpa only [after, P2, intermediateFields, if_neg hn] using h1
  have finished := primitive_put_rep p 5
    ((energy (chosen g U B)+stagedPressure g (chosen g U B))/rho (chosen g U B)) after.memory P2 h2
  have eq : Function.update P2 p (Function.update (P2 p) 5
      ((energy (chosen g U B)+stagedPressure g (chosen g U B))/rho (chosen g U B))) =
      Function.update P p (setPoint g U B (P p)).primitive := by
    funext q
    by_cases hq : q = p <;> simp_all [P2, Function.update_apply, point_primitive_formula]
  rw [eq] at finished
  simpa only [pointWrites, chosen, first, after] using finished

theorem point_velocity (p : Fin M) (g : IdealGas) (U B : State 2) (s : Machine)
    (V : Fin M → Option ℝ) (h : VelocityRep s.memory V) :
    VelocityRep (pointWrites p g U B s).memory
      (Function.update V p (some (velocitySquared (chosen g U B)))) := by
  have h1 := stage_velocity p g U s V h
  by_cases hn : rejected g U
  · have copied := backup_velocity_rep p B _ _ h1
    have second := stage_velocity p g B
      ⟨backupWrites p B (stageWrites p g U s).memory,(stageWrites p g U s).fluid⟩ _ copied
    have eq : Function.update (Function.update V p (some (velocitySquared U))) p (some (velocitySquared B)) =
        Function.update V p (some (velocitySquared B)) := by
      funext q; by_cases hq : q = p <;> simp_all [Function.update_apply]
    simpa only [pointWrites, chosen, if_pos hn, eq] using velocity_primitive_put_rep p 5
      ((energy B+stagedPressure g B)/rho B) _ _ second
  · simpa only [pointWrites, chosen, if_neg hn] using velocity_primitive_put_rep p 5
      ((energy U+stagedPressure g U)/rho U) _ _ h1

end SU2Preprocessing
