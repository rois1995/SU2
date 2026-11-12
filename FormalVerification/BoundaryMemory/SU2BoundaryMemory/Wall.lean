import SU2BoundaryMemory.Execution

set_option maxRecDepth 100000
set_option maxHeartbeats 4000000
noncomputable section
namespace SU2BoundaryMemory
open SU2 SU2Source SU2Euler SU2Boundary
variable {N M K : Nat} {vertices : Fin K → Nat}

/-- Proof aid for the two scalar stores, with values obtained from actual input loads. -/
def projectionWrites (block : Nat) (p : Fin N) (n : Vec 2) (op : Op) (x y : ℝ) (m : Mem) : Mem :=
  let d := (0 + x * n 0) + y * n 1
  put (put m (flowAddr block p 1) (opReal op x (d * n 0)))
    (flowAddr block p 2) (opReal op y (d * n 1))

def projectionFootprint (b : Nat) (p : Fin N) : List Address := [flowAddr b p 1, flowAddr b p 2]

theorem project_execution (m : Mem) (b : Nat) (p : Fin N) (n : Vec 2) (op : Op) (x y : ℝ)
    (hx : m (flowAddr b p 1) = some x) (hy : m (flowAddr b p 2) = some y) :
    projectMemory b p n op false false m = some (projectionWrites b p n op x y m) := by
  simp [projectMemory, projectionWrites, hx, hy, put]

theorem projection_writes_frame (m : Mem) (b : Nat) (p : Fin N) (n : Vec 2) (op : Op) (x y : ℝ) :
    Frame (projectionFootprint b p) m (projectionWrites b p n op x y m) := by
  exact frame_trans (put_frame m _ _) (put_frame (put m _ _) _ _)

/-- Reversing independent correction stores preserves the completed reduction and all memory. -/
theorem projection_refactor (m : Mem) (b : Nat) (p : Fin N) (n : Vec 2) (op : Op) :
    projectMemory b p n op false false m = projectMemory b p n op false true m := by
  cases hx : m (flowAddr b p 1) with
  | none => simp [projectMemory, hx]
  | some x =>
    cases hy : m (flowAddr b p 2) with
    | none => simp [projectMemory, hx, hy]
    | some y =>
      have hne : flowAddr b p 1 ≠ flowAddr b p 2 := by simp
      let d := (0 + x * n 0) + y * n 1
      simpa [projectMemory, hx, hy, put, d] using congrArg some
        (Function.update_comm hne (some (opReal op x (d * n 0))) (some (opReal op y (d * n 1))) m)

def projectedState (b : Buffer) (p : Fin N) (n : Vec 2) (op : Op) (s : BoundaryState N) : BoundaryState N :=
  b.set s (Function.update (b.get s) p (projection op n (b.get s p)))

theorem projected_cache (b : Buffer) (p : Fin N) (n : Vec 2) (op : Op) (s : BoundaryState N) :
    (projectedState b p n op s).cache = s.cache := by cases b <;> rfl

theorem projected_buffers (m : Mem) (s : BoundaryState N) (b : Buffer) (p : Fin N) (n : Vec 2) (op : Op)
    (h : BuffersRep m s) :
    BuffersRep (projectionWrites b.block p n op (b.get s p (.momentum 0)) (b.get s p (.momentum 1)) m)
      (projectedState b p n op s) := by
  have h1 := h .old
  have h5 := h .residual
  have h6 := h .truncation
  simp only [Buffer.block, Buffer.get] at h1 h5 h6
  intro d q c
  cases b <;> cases d <;> by_cases hp : q = p <;> subst_vars <;> fin_cases c <;>
    simp_all [projectedState, Buffer.set, Buffer.get, Buffer.block, projectionWrites, put,
      projection, channel, Function.update_apply]

theorem projected_rep (m : Mem) (s : BoundaryState N) (P : Primitives N) (b : Buffer)
    (p : Fin N) (n : Vec 2) (op : Op) (h : BoundaryRep vertices m s P) :
    BoundaryRep vertices
      (projectionWrites b.block p n op (b.get s p (.momentum 0)) (b.get s p (.momentum 1)) m)
      (projectedState b p n op s) P := by
  have hf := projection_writes_frame m b.block p n op (b.get s p (.momentum 0)) (b.get s p (.momentum 1))
  refine ⟨projected_buffers m s b p n op h.buffers, ?_, ?_⟩
  · intro q c
    rw [hf]
    · exact h.primitives q c
    · have h1 := (flow_primitive_separate b.block (buffer_block_bound b) p q 1 c).symm
      have h2 := (flow_primitive_separate b.block (buffer_block_bound b) p q 2 c).symm
      simpa [projectionFootprint] using And.intro h1 h2
  · intro site g hg c
    rw [projected_cache] at hg
    rw [hf]
    · exact h.cache site g hg c
    · have hb : b.block ≤ 8 := Nat.le_of_lt (buffer_block_bound b)
      have h1 := cache_flow_separate b.block hb p 1 site.marker site.vertex c
      have h2 := cache_flow_separate b.block hb p 2 site.marker site.vertex c
      simpa [projectionFootprint] using And.intro h1 h2

theorem project_memory_refines (m : Mem) (s : BoundaryState N) (P : Primitives N) (b : Buffer)
    (p : Fin N) (n : Vec 2) (op : Op) (h : BoundaryRep vertices m s P) :
    ∃ after, projectMemory b.block p n op false false m = some after ∧
      BoundaryRep vertices after (projectedState b p n op s) P ∧ Frame (projectionFootprint b.block p) m after := by
  refine ⟨projectionWrites b.block p n op (b.get s p (.momentum 0)) (b.get s p (.momentum 1)) m, ?_,
    projected_rep m s P b p n op h, projection_writes_frame m b.block p n op _ _⟩
  exact project_execution m b.block p n op _ _ (by simpa [channel] using h.buffers b p 1)
    (by simpa [channel] using h.buffers b p 2)


def wallFootprint (v : WallVisit N) : List Address :=
  if v.owned then projectionFootprint 1 v.point ++ projectionFootprint 5 v.point ++
    projectionFootprint 6 v.point else []

theorem wall_memory_refines (m : Mem) (s : BoundaryState N) (P : Primitives N)
    (v : WallVisit N) (bounds : Bounds N M K vertices) (h : BoundaryRep vertices m s P) :
    ∃ after, executeWall M wallCode v m = some after ∧
      BoundaryRep vertices after (wallSpec v s) P ∧ Frame (wallFootprint v) m after := by
  by_cases hv : v.owned = true
  · obtain ⟨m1, e1, h1, f1⟩ := project_memory_refines m s P .old v.point v.normal.vector
      SU2Boundary.Generated.wallOld_original h
    obtain ⟨m2, e2, h2, f2⟩ := project_memory_refines m1 _ P .residual v.point v.normal.vector
      SU2Boundary.Generated.wallRes_original h1
    obtain ⟨m3, e3, h3, f3⟩ := project_memory_refines m2 _ P .truncation v.point v.normal.vector
      SU2Boundary.Generated.wallTrunc_original h2
    simp only [Buffer.block] at e1 e2 e3
    refine ⟨m3, ?_, ?_, ?_⟩
    · simpa only [executeWall, wallCode, hv, ite_true, valid_point_flow bounds v.point,
        Buffer.block, Option.bind_eq_bind, Option.some_bind, e1, e2] using e3
    · rw [← wall_source_contract]
      simpa only [wallImpl, hv, ite_true, projectedState, Buffer.set, Buffer.get] using h3
    · simpa only [wallFootprint, hv, ite_true, Buffer.block] using frame_trans (frame_trans f1 f2) f3
  · refine ⟨m, ?_, ?_, ?_⟩
    · simp [executeWall, hv]
    · simpa [wallSpec, hv] using h
    · simpa [wallFootprint, hv] using frame_refl [] m

end SU2BoundaryMemory
