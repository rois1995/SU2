import SU2Preprocessing.Point

set_option maxRecDepth 100000
set_option maxHeartbeats 8000000
noncomputable section
namespace SU2Preprocessing
open SU2 SU2Source SU2Boundary SU2BoundaryMemory SU2Initialization
variable {M : Nat}

theorem point_frame (p : Fin M) (g : IdealGas) (U B : State 2) (s : Machine) :
    Frame (rowFootprint p) s.memory (pointWrites p g U B s).memory := by
  intro a ha
  have hc : ∀ c, a ≠ flowAddr 0 p c := by
    intro c eq; exact ha (eq.symm ▸ row_flow_mem p c)
  have hp : ∀ c, a ≠ primitiveAddr p c := by
    intro c eq; exact ha (eq.symm ▸ row_primitive_mem p c)
  have hv : a ≠ velocityAddr p := fun eq => ha (eq.symm ▸ row_velocity_mem p)
  by_cases hn : rejected g U <;> by_cases hu : stagedSound2 g U < 0 <;> by_cases hb : stagedSound2 g B < 0
  all_goals simp only [pointWrites, hn, stageWrites, checkWrites, hu, hb, if_true, if_false]
  all_goals simp [velocityWrites, backupWrites, writeFields, patch, put, Function.update_apply, hc, hp, hv]

/-- Logical representations at the actual pre-preprocessing input, not a post-state. -/
structure Data (M : Nat) where
  current : Residual M 2
  old : Residual M 2
  primitive : Primitives M
  velocity : Fin M → Option ℝ

structure DataRep (s : Machine) (d : Data M) : Prop where
  current : CurrentRep s.memory d.current
  old : OldRep s.memory d.old
  primitive : PrimitiveRep s.memory d.primitive
  velocity : VelocityRep s.memory d.velocity

def advanceData (g : IdealGas) (p : Fin M) (d : Data M) : Data M :=
  let result := setPoint g (d.current p) (d.old p) (d.primitive p)
  ⟨Function.update d.current p result.current, d.old,
    Function.update d.primitive p result.primitive, Function.update d.velocity p (some result.velocity2)⟩

theorem chosen_point_current (g : IdealGas) (U B : State 2) (P : Fin 11 → ℝ) :
    chosen g U B = (setPoint g U B P).current := rfl

theorem point_data (g : IdealGas) (p : Fin M) (s : Machine) (d : Data M) (h : DataRep s d) :
    DataRep (pointWrites p g (d.current p) (d.old p) s) (advanceData g p d) := by
  refine ⟨?_, ?_, point_primitives p g (d.current p) (d.old p) s d.primitive h.primitive, ?_⟩
  · simpa only [advanceData, ← chosen_point_current] using point_current p g d.current d.old s h.current
  · intro q c
    exact (point_old p q c g (d.current p) (d.old p) s).trans (h.old q c)
  · exact point_velocity p g (d.current p) (d.old p) s d.velocity h.velocity

def modelPoints (g : IdealGas) : List (Fin M) → Data M → Nat → Data M × Nat
  | [], d, count => (d,count)
  | p :: ps, d, count => modelPoints g ps (advanceData g p d)
      (count + if (setPoint g (d.current p) (d.old p) (d.primitive p)).physical then 0 else 1)

/-- The loop spec is a fold of the protected point contract, independent of memory effects. -/
theorem points_refine (g : IdealGas) (ps : List (Fin M)) (s : Machine) (d : Data M) (count : Nat)
    (h : DataRep s d) (hf : FluidReady g s.fluid) :
    ∃ final, executePoints ps s count = some (final,(modelPoints g ps d count).2) ∧
      DataRep final (modelPoints g ps d count).1 ∧ FluidReady g final.fluid ∧
      final.fluid.entropyEnabled = s.fluid.entropyEnabled ∧
      final.fluid.parameters = s.fluid.parameters ∧ final.fluid.untouched = s.fluid.untouched ∧
      Frame (ps.flatMap rowFootprint) s.memory final.memory := by
  induction ps generalizing s d count with
  | nil => exact ⟨s,rfl,h,hf,rfl,rfl,rfl,frame_refl _ _⟩
  | cons p ps ih =>
    let first := pointWrites p g (d.current p) (d.old p) s
    let delta := if (setPoint g (d.current p) (d.old p) (d.primitive p)).physical then 0 else 1
    have ep := point_execution p g (d.current p) (d.old p) s (h.current p) (h.old p) hf
    have hd := point_data g p s d h
    have ready : FluidReady g first.fluid := by
      rw [point_fluid]; exact fluid_ready_preserved _ _ _ _ hf
    obtain ⟨final,ef,hdf,hff,mode,params,extra,frame⟩ := ih first (advanceData g p d) (count+delta) hd ready
    have ff := fluid_frame s.fluid (rho (chosen g (d.current p) (d.old p))) (staticEnergy (chosen g (d.current p) (d.old p)))
    have eqf : first.fluid = executeFluid s.fluid (rho (chosen g (d.current p) (d.old p)))
        (staticEnergy (chosen g (d.current p) (d.old p))) := point_fluid p g _ _ s
    refine ⟨final, ?_, hdf, hff, mode.trans ?_, params.trans ?_, extra.trans ?_, ?_⟩
    · simpa [executePoints, ep, modelPoints, first, delta, setPoint] using ef
    · exact eqf ▸ ff.2.1
    · exact eqf ▸ ff.1
    · exact eqf ▸ ff.2.2
    · simpa only [List.flatMap_cons] using frame_trans (point_frame p g _ _ s) frame

def points (M : Nat) : List (Fin M) := List.ofFn (fun p : Fin M => p)

theorem points_coverage (p : Fin M) : p ∈ points M := List.mem_ofFn.mpr ⟨p,rfl⟩

theorem points_order : (points M).Pairwise (fun p q => p.val < q.val) := by
  simp only [points, List.pairwise_ofFn]
  intro i j h; exact h

theorem points_no_duplicates : (points M).Nodup := by
  apply List.Pairwise.imp _ points_order
  intro a b h eq; subst b; exact Nat.lt_irrefl _ h

theorem points_count : (points M).length = M := by simp [points]

theorem counter_bound (g : IdealGas) (ps : List (Fin M)) (d : Data M) (count : Nat) :
    (modelPoints g ps d count).2 ≤ count + ps.length := by
  induction ps generalizing d count with
  | nil => simp [modelPoints]
  | cons p ps ih =>
    simp only [modelPoints]
    have h := ih (advanceData g p d) (count + if (setPoint g (d.current p) (d.old p) (d.primitive p)).physical then 0 else 1)
    by_cases hp : (setPoint g (d.current p) (d.old p) (d.primitive p)).physical = true
    · simp only [if_pos hp, add_zero, List.length_cons] at h ⊢; omega
    · simp only [if_neg hp, List.length_cons] at h ⊢; omega

end SU2Preprocessing
