import SU2BoundaryMemory.Far

set_option maxRecDepth 100000
set_option maxHeartbeats 4000000
noncomputable section
namespace SU2BoundaryMemory
open SU2 SU2Source SU2Euler SU2Boundary
variable {N M K : Nat} {vertices : Fin K → Nat}

/-- Repeated visits are retained, and each visit consumes its predecessor's memory. -/
theorem fars_memory_refines (far : List (FarSite N K vertices)) (m : Mem) (s : BoundaryState N)
    (P : Primitives N) (g : IdealGas) (free : Primitive 2) (bounds : Bounds N M K vertices)
    (h : BoundaryRep vertices m s P) :
    ∃ after, executeFars M farCode g free far m = some after ∧
      BoundaryRep vertices after
        (far.foldl (fun acc v => farImpl SU2Boundary.Generated.farAdd_original g free (farVisit P v) acc) s) P ∧
      Frame (far.flatMap farFootprint) m after := by
  induction far generalizing m s with
  | nil => exact ⟨m, rfl, h, frame_refl [] m⟩
  | cons v vs ih =>
    obtain ⟨m1, e1, h1, f1⟩ := far_memory_refines m s P g free v bounds h
    obtain ⟨m2, e2, h2, f2⟩ := ih m1 _ h1
    refine ⟨m2, ?_, h2, ?_⟩
    · simp only [executeFars, e1, Option.some_bind, e2]
    · simpa only [List.flatMap_cons] using frame_trans f1 f2

theorem walls_memory_refines (walls : List (WallVisit N)) (m : Mem) (s : BoundaryState N)
    (P : Primitives N) (bounds : Bounds N M K vertices) (h : BoundaryRep vertices m s P) :
    ∃ after, executeWalls M wallCode walls m = some after ∧
      BoundaryRep vertices after (walls.foldl (fun acc v => wallSpec v acc) s) P ∧
      Frame (walls.flatMap wallFootprint) m after := by
  induction walls generalizing m s with
  | nil => exact ⟨m, rfl, h, frame_refl [] m⟩
  | cons v vs ih =>
    obtain ⟨m1, e1, h1, f1⟩ := wall_memory_refines m s P v bounds h
    obtain ⟨m2, e2, h2, f2⟩ := ih m1 _ h1
    refine ⟨m2, ?_, h2, ?_⟩
    · simp only [executeWalls, e1, Option.some_bind, e2]
    · simpa only [List.flatMap_cons] using frame_trans f1 f2

def boundaryFootprint (far : List (FarSite N K vertices)) (walls : List (WallVisit N)) : List Address :=
  far.flatMap farFootprint ++ walls.flatMap wallFootprint

theorem boundary_memory_refines (far : List (FarSite N K vertices)) (walls : List (WallVisit N))
    (m0 : Mem) (s0 : BoundaryState N) (P : Primitives N) (g : IdealGas) (free : Primitive 2)
    (bounds : Bounds N M K vertices) (h0 : BoundaryRep vertices m0 s0 P)
    (ready : FarReady g free (far.map (farVisit P))) :
    ∃ mb, executeBoundary M farCode wallCode g free far walls m0 = some mb ∧
      BoundaryRep vertices mb (boundarySpec g free (far.map (farVisit P)) walls s0) P ∧
      Frame (boundaryFootprint far walls) m0 mb := by
  obtain ⟨mf, ef, hf, ff⟩ := fars_memory_refines far m0 s0 P g free bounds h0
  obtain ⟨mb, eb, hb, fb⟩ := walls_memory_refines walls mf _ P bounds hf
  refine ⟨mb, ?_, ?_, frame_trans ff fb⟩
  · simp only [executeBoundary, ef, Option.some_bind, eb]
  · rw [← boundary_refinement g free (far.map (farVisit P)) walls s0 ready]
    simpa only [boundaryImpl, List.foldl_map, wall_source_contract] using hb

/-- Complete addresses outside old, residual, truncation and cache storage are framed. -/
theorem boundary_protected (far : List (FarSite N K vertices)) (walls : List (WallVisit N))
    (a : Address) (h1 : a.1 ≠ 1) (h5 : a.1 ≠ 5) (h6 : a.1 ≠ 6) (hc : a.1 < 9) :
    a ∉ boundaryFootprint far walls := by
  have hw : ∀ v : WallVisit N, a ∉ wallFootprint v := by
    intro v
    by_cases hv : v.owned = true
    · have hn (b : Nat) (hb : a.1 ≠ b) (c : Fin 4) : a ≠ flowAddr b v.point c :=
        fun e => hb (congrArg Prod.fst e)
      simp [wallFootprint, hv, projectionFootprint, hn 1 h1 1, hn 1 h1 2,
        hn 5 h5 1, hn 5 h5 2, hn 6 h6 1, hn 6 h6 2]
    · simp [wallFootprint, hv]
  have hf : ∀ v : FarSite N K vertices, a ∉ farFootprint v := by
    intro v
    by_cases hv : v.owned = true
    · simp only [farFootprint, hv, ite_true, List.mem_append, not_or]
      constructor
      · intro hin
        obtain ⟨j, _, eq⟩ := List.mem_map.mp hin
        have e := congrArg Prod.fst eq
        simp only [cacheAddr, Prod.fst] at e
        omega
      · intro hin
        obtain ⟨j, _, eq⟩ := List.mem_map.mp hin
        exact h5 (congrArg Prod.fst eq.symm)
    · simp [farFootprint, hv]
  simp only [boundaryFootprint, List.mem_append, List.mem_flatMap, not_or, not_exists, not_and]
  exact ⟨fun v _ => hf v, fun v _ => hw v⟩

theorem boundary_geometry_frame (far : List (FarSite N K vertices)) (walls : List (WallVisit N))
    (m0 mb : Mem) (vol per dt : Fin N → ℝ) (h : Frame (boundaryFootprint far walls) m0 mb)
    (hg : GeometryRep m0 vol per dt) : GeometryRep mb vol per dt := by
  refine ⟨?_, ?_, ?_⟩
  · intro p
    rw [h _ (boundary_protected far walls (2,p.val) (by norm_num) (by norm_num) (by norm_num) (by norm_num))]
    exact hg.1 p
  · intro p
    rw [h _ (boundary_protected far walls (3,p.val) (by norm_num) (by norm_num) (by norm_num) (by norm_num))]
    exact hg.2.1 p
  · intro p
    rw [h _ (boundary_protected far walls (4,p.val) (by norm_num) (by norm_num) (by norm_num) (by norm_num))]
    exact hg.2.2 p

/-- Restrict total allocated-point data to the prefix traversed by the explicit update.
Boundary visits can reference any allocated point, including unowned halo points. -/
def ownedState (owned : N ≤ M) (s : BoundaryState M) : BoundaryState N :=
  ⟨fun p => s.current (p.castLE owned), fun p => s.old (p.castLE owned),
    fun p => s.residual (p.castLE owned), fun p => s.truncation (p.castLE owned), s.cache⟩

theorem owned_readiness (owned : N ≤ M) (m : Mem) (s : BoundaryState M)
    (vol per dt : Fin M → ℝ) (h : BuffersRep m s) (hg : GeometryRep m vol per dt) :
    InputsReady m (ownedState owned s) (fun p => vol (p.castLE owned))
      (fun p => per (p.castLE owned)) (fun p => dt (p.castLE owned)) := by
  exact ⟨fun c => h .old (c.1.castLE owned) c.2,
    fun c => h .residual (c.1.castLE owned) c.2,
    fun c => h .truncation (c.1.castLE owned) c.2,
    fun p => hg.1 (p.castLE owned), fun p => hg.2.1 (p.castLE owned), fun p => hg.2.2 (p.castLE owned)⟩

/-- Principal composition. The only memory premises concern m0, before boundaries.
It derives mb and InputsReady by executing the boundary model, then uses the protected
complete update theorem. Initial interior residuals, primitive/geometry/timestep origin
and the provisional far-field thermodynamic bridge remain explicit caller obligations. -/
theorem boundary_to_complete_update (owned : N ≤ M) (bounds : Bounds M M K vertices)
    (g : IdealGas) (free : Primitive 2) (P : Primitives M)
    (far : List (FarSite M K vertices)) (walls : List (WallVisit M))
    (s0 : BoundaryState M) (m0 : Mem) (vol dt : Fin M → ℝ)
    (h0 : BoundaryRep vertices m0 s0 P) (hg0 : GeometryRep m0 vol (fun _ => 0) dt)
    (farReady : FarReady g free (far.map (farVisit P))) :
    let sb := boundarySpec g free (far.map (farVisit P)) walls s0
    let so := ownedState owned sb
    let vo := fun p => vol (p.castLE owned)
    let dto := fun p => dt (p.castLE owned)
    ∃ mb mu,
      executeBoundary M farCode wallCode g free far walls m0 = some mb ∧
      BoundaryRep vertices mb sb P ∧
      InputsReady mb so vo (fun _ => 0) dto ∧
      executeLoop (N := N) M SU2Boundary.Generated.volume_original SU2Boundary.Generated.periodic_original
        SU2Boundary.Generated.timestep_original SU2Boundary.Generated.points_original
        SU2Boundary.Generated.components_original mb = some mu ∧
      LoopContract so vo dto mb mu ∧
      Frame (boundaryFootprint far walls ++ (visits N).map destination) m0 mu := by
  dsimp only
  obtain ⟨mb, eb, hb, fb⟩ := boundary_memory_refines far walls m0 s0 P g free bounds h0 farReady
  have hgb := boundary_geometry_frame far walls m0 mb vol (fun _ => 0) dt fb hg0
  have ready := owned_readiness owned mb _ vol (fun _ => 0) dt hb.buffers hgb
  have size : 8 * (M * 4) ≤ 2^64 := by have h := bounds.points; omega
  obtain ⟨mu, eu, hu⟩ := complete_update_loop mb _ _ _ ready owned size
  refine ⟨mb, mu, eb, hb, ready, eu, hu, frame_trans fb ?_⟩
  intro a ha
  exact hu.2 a (fun c eq => ha (List.mem_map.mpr ⟨c, visits_coverage c, eq.symm⟩))

end SU2BoundaryMemory
