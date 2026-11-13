import SU2Interior.TimestepProof
import SU2Interior.RoeModel

set_option maxRecDepth 100000
set_option maxHeartbeats 8000000
noncomputable section
namespace SU2Interior
open SU2 SU2Source SU2Boundary SU2BoundaryMemory SU2Initialization SU2Preprocessing
variable {M E K : Nat} {vertices : Fin K → Nat}

@[simp] theorem flow_address_equal (b : Nat) (p q : Fin M) (c d : Fin 4) :
    flowAddr b p c = flowAddr b q d ↔ p=q ∧ c=d := by
  constructor
  · exact flow_addr_injective b
  · rintro ⟨rfl,rfl⟩; rfl

/-- CSysVector SIMD overload: component-major within each lane; each subtraction
reads the memory following that component's addition. -/
def executeScatter (p q : Fin M) (f : Flux4) : List (Fin 4) → Mem → Option Mem
  | [], m => some m
  | c :: cs, m => do
    let ri ← m (flowAddr 5 p c)
    let m := put m (flowAddr 5 p c) (ri+f c)
    let rj ← m (flowAddr 5 q c)
    let m := put m (flowAddr 5 q c) (rj-f c)
    executeScatter p q f cs m

def scatterWrites (p q : Fin M) (ri rj f : Flux4) : List (Fin 4) → Mem → Mem
  | [], m => m
  | c :: cs, m => scatterWrites p q ri rj f cs
      (put (put m (flowAddr 5 p c) (ri c+f c)) (flowAddr 5 q c) (rj c-f c))

def scatterFootprint (p q : Fin M) (cs : List (Fin 4)) : List Address :=
  cs.flatMap (fun c => [flowAddr 5 p c,flowAddr 5 q c])

theorem scatter_execution (p q : Fin M) (ne : p≠q) (f ri rj : Flux4) (m : Mem)
    (hi : ∀ c, m (flowAddr 5 p c)=some (ri c))
    (hj : ∀ c, m (flowAddr 5 q c)=some (rj c)) :
    executeScatter p q f flowColumns m = some (scatterWrites p q ri rj f flowColumns m) := by
  simp [executeScatter,scatterWrites,flowColumns,put,Function.update_apply,flow_address_equal,ne,Ne.symm ne,hi,hj]

theorem scatter_frame (p q : Fin M) (f ri rj : Flux4) (cs : List (Fin 4)) (m : Mem) :
    Frame (scatterFootprint p q cs) m (scatterWrites p q ri rj f cs m) := by
  induction cs generalizing m with
  | nil => exact frame_refl _ _
  | cons c cs ih =>
    simpa only [scatterWrites,scatterFootprint,List.flatMap_cons,List.cons_append,List.nil_append,List.singleton_append,List.append_assoc] using
      frame_trans (frame_trans (put_frame m (flowAddr 5 p c) (ri c+f c))
        (put_frame (put m (flowAddr 5 p c) (ri c+f c)) (flowAddr 5 q c) (rj c-f c)))
        (ih (put (put m (flowAddr 5 p c) (ri c+f c)) (flowAddr 5 q c) (rj c-f c)))

def scatterState (p q : Fin M) (f : Flux4) (s : BoundaryState M) : BoundaryState M :=
  let first := Function.update s.residual p (s.residual p+fromComponents f)
  {s with residual := Function.update first q (first q-fromComponents f)}

theorem scatter_buffers (p q : Fin M) (ne : p≠q) (f : Flux4) (m : Mem) (s : BoundaryState M)
    (h : BuffersRep m s) :
    BuffersRep (scatterWrites p q (components (s.residual p)) (components (s.residual q)) f flowColumns m)
      (scatterState p q f s) := by
  intro b z c
  have hi := h .residual p
  have hj := h .residual q
  have hz := h b z c
  cases b <;> by_cases zp : z=p <;> by_cases zq : z=q <;> subst_vars <;>
    fin_cases c <;>
    simp_all [scatterWrites,flowColumns,scatterState,Buffer.block,Buffer.get,put,
      Function.update_apply,components,fromComponents,channel,state,flow_address_equal]

theorem scatter_protected (p q : Fin M) (a : Address) (hb : a.1≠5) :
    a ∉ scatterFootprint p q flowColumns := by
  intro mem
  obtain ⟨c,_,ha⟩ := List.mem_flatMap.mp mem
  simp only [List.mem_cons,List.not_mem_nil,or_false] at ha
  rcases ha with eq | eq <;> exact hb (congrArg Prod.fst eq)

theorem scatter_memory_refines (p q : Fin M) (ne : p≠q) (f : Flux4) (m : Mem)
    (s : BoundaryState M) (P : Primitives M) (h : BoundaryRep vertices m s P) :
    ∃ mf, executeScatter p q f flowColumns m = some mf ∧
      BoundaryRep vertices mf (scatterState p q f s) P ∧
      Frame (scatterFootprint p q flowColumns) m mf := by
  let mf := scatterWrites p q (components (s.residual p)) (components (s.residual q)) f flowColumns m
  have frame := scatter_frame p q f (components (s.residual p)) (components (s.residual q)) flowColumns m
  refine ⟨mf,scatter_execution p q ne f _ _ m (h.buffers .residual p) (h.buffers .residual q),
    ⟨scatter_buffers p q ne f m s h.buffers,?_,?_⟩,frame⟩
  · intro z c
    dsimp only [mf]
    rw [frame _ (scatter_protected p q (primitiveAddr z c) (by norm_num [primitiveAddr]))]
    exact h.primitives z c
  · intro site ghost hg c
    dsimp only [mf]
    rw [frame _ (scatter_protected p q (cacheAddr site.marker site.vertex c) (by unfold cacheAddr; simp; omega))]
    exact h.cache site ghost hg c

/-- The actual normal/area may be zero in a raw object. This explicit contract is
provided by the separately modeled EPS² repair, not inferred from a residual theorem. -/
def GeometryReady (geo : Geometry M E) : Prop := ∀ e, 0 < faceArea (geo.normal e)

def repairGeometry (eps : ℝ) (geo : Geometry M E) : Geometry M E :=
  {geo with normal := fun e => repairNormal eps (geo.normal e)}

theorem repaired_geometry_ready (eps : ℝ) (he : eps≠0) (geo : Geometry M E) :
    GeometryReady (repairGeometry eps geo) := fun e => repaired_area_positive eps he (geo.normal e)

def mathematicalEdge (geo : Geometry M E) (ready : GeometryReady geo) (e : Fin E) : InteriorEdge M 2 :=
  ⟨geo.left e,geo.right e,endpoint_distinct geo e,
    ⟨fun i => geo.normal e i/faceArea (geo.normal e),normalized_area _ (ready e)⟩,
    faceArea (geo.normal e),ready e⟩

theorem scatter_state_mathematical (geo : Geometry M E) (ready : GeometryReady geo)
    (e : Fin E) (f : Flux4) (s : BoundaryState M) :
    (scatterState (geo.left e) (geo.right e) f s).residual =
      SU2.scatter (mathematicalEdge geo ready e) (fromComponents f) s.residual := rfl

end SU2Interior
