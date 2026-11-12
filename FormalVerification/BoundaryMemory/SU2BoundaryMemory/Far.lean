import SU2BoundaryMemory.Cache

set_option maxRecDepth 100000
set_option maxHeartbeats 4000000
noncomputable section
namespace SU2BoundaryMemory
open SU2 SU2Source SU2Euler SU2Boundary
variable {N M K : Nat} {vertices : Fin K → Nat}

def flowColumns : List (Fin 4) := [0,1,2,3]
def residualFootprint (p : Fin N) : List Address := flowColumns.map (flowAddr 5 p)

def residualWrites (op : Op) (p : Fin N) (r f : Fin 4 → ℝ) (m : Mem) : Mem :=
  writeFields (flowAddr 5 p) (fun c => opReal op (r c) (f c)) flowColumns m

theorem accumulate_execution (op : Op) (p : Fin N) (r f : Fin 4 → ℝ) (m : Mem)
    (h : ∀ c, m (flowAddr 5 p c) = some (r c)) :
    accumulateResidual op p f m = some (residualWrites op p r f m) := by
  simp [accumulateResidual, residualWrites, writeFields, flowColumns, put, h]

theorem accumulate_frame (op : Op) (p : Fin N) (r f : Fin 4 → ℝ) (m : Mem) :
    Frame (residualFootprint p) m (residualWrites op p r f m) :=
  write_fields_frame _ _ _ _

def accumulatedState (op : Op) (p : Fin N) (f : Fin 4 → ℝ) (s : BoundaryState N) : BoundaryState N :=
  {s with residual := Function.update s.residual p (fun c => opReal op (s.residual p c) (fromComponents f c))}

theorem accumulated_buffers (m : Mem) (s : BoundaryState N) (p : Fin N) (op : Op)
    (f : Fin 4 → ℝ) (h : BuffersRep m s) :
    BuffersRep (residualWrites op p (components (s.residual p)) f m) (accumulatedState op p f s) := by
  have h1 := h .old
  have h5 := h .residual
  have h6 := h .truncation
  simp only [Buffer.block, Buffer.get] at h1 h5 h6
  intro b q c
  cases b <;> by_cases hp : q = p <;> subst_vars <;> fin_cases c <;>
    simp_all [residualWrites, writeFields, flowColumns, put, accumulatedState,
      Buffer.get, Buffer.block, Function.update_apply, components, fromComponents, channel, state]

theorem accumulate_memory_refines (m : Mem) (s : BoundaryState N) (P : Primitives N)
    (p : Fin N) (op : Op) (f : Fin 4 → ℝ) (h : BoundaryRep vertices m s P) :
    ∃ after, accumulateResidual op p f m = some after ∧
      BoundaryRep vertices after (accumulatedState op p f s) P ∧ Frame (residualFootprint p) m after := by
  have hf := accumulate_frame op p (components (s.residual p)) f m
  refine ⟨residualWrites op p (components (s.residual p)) f m,
    accumulate_execution op p _ f m (fun c => h.buffers .residual p c),
    ⟨accumulated_buffers m s p op f h.buffers, ?_, ?_⟩, hf⟩
  · intro q c
    rw [hf]
    · exact h.primitives q c
    · simp only [residualFootprint, List.mem_map, not_exists, not_and]
      intro j _ eq
      exact flow_primitive_separate 5 (by decide) p q j c eq
  · intro site g hg c
    rw [hf]
    · exact h.cache site g hg c
    · simp only [residualFootprint, List.mem_map, not_exists, not_and]
      intro j _ eq
      exact cache_flow_separate 5 (by decide) p j site.marker site.vertex c eq.symm

def farFootprint (v : FarSite N K vertices) : List Address :=
  if v.owned then cacheFootprint v.cache ++ residualFootprint v.point else []

/-- Every source operand comes from pre-boundary or previously written memory.
The target state is a postcondition, never an execution argument. -/
theorem far_memory_refines (m : Mem) (s : BoundaryState N) (P : Primitives N)
    (g : IdealGas) (free : Primitive 2) (v : FarSite N K vertices)
    (bounds : Bounds N M K vertices) (h : BoundaryRep vertices m s P) :
    ∃ after, executeFar M farCode g free v m = some after ∧
      BoundaryRep vertices after (farImpl SU2Boundary.Generated.farAdd_original g free (farVisit P v) s) P ∧
      Frame (farFootprint v) m after := by
  by_cases hv : v.owned = true
  · let W := farPrimitiveImpl g v.normal.vector (inside P v.point) free
    let mc := writeCache .normal false v.cache (ghost g W) m
    have hc := cache_memory_rep m s P v.cache (ghost g W) h
    have hi := read_interior_correct m P v.point bounds h.primitives
    have hp := read_primitive_correct mc P v.point bounds hc.primitives
    have hr := read_cached_written m v.cache (ghost g W)
    obtain ⟨after, he, ha, hf⟩ := accumulate_memory_refines mc _ P v.point
      SU2Boundary.Generated.farAdd_original
      (roeImplementation g v.normal.vector v.area (inside P v.point) W (P v.point 5) (totalEnthalpy g W)) hc
    dsimp only [mc, W] at hp hr he
    refine ⟨after, ?_, ?_, ?_⟩
    · simp only [executeFar, farCode, valid_cache bounds v.cache, not_true_eq_false, ite_false,
        hv, ite_true, hi, Option.bind_eq_bind, Option.some_bind]
      rw [hp, Option.some_bind, hr, Option.some_bind]
      exact he
    · simpa only [farImpl, farVisit, hv, ite_true, accumulatedState, cachedState, ghost, W] using ha
    · simpa only [farFootprint, hv, ite_true, mc] using
        frame_trans (write_cache_frame v.cache (ghost g W) m) hf
  · refine ⟨m, ?_, ?_, ?_⟩
    · simp [executeFar, valid_cache bounds v.cache, hv]
    · simpa [farImpl, farVisit, hv] using h
    · simpa [farFootprint, hv] using frame_refl [] m

end SU2BoundaryMemory
