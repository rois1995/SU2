import SU2Initialization.Primitive

noncomputable section
namespace SU2Initialization
open SU2 SU2Source SU2Boundary SU2BoundaryMemory
variable {N M K : Nat} {vertices : Fin K → Nat}

/-- Entry requirements after preprocessing/interior work. Old storage is deliberately absent.
Primitive, interior residual and timestep origins remain separate, unverified interfaces. -/
structure BeforeSnapshot (m : Mem) (s : BoundaryState M) (P : Primitives M) : Prop where
  current : Stored 0 m (fun c => s.current c.1 (channel c.2))
  residual : Stored 5 m (fun c => s.residual c.1 (channel c.2))
  truncation : Stored 6 m (fun c => s.truncation c.1 (channel c.2))
  primitives : PrimitiveRep m P
  cache : CacheRep vertices m s.cache

def snapshotState (s : BoundaryState M) : BoundaryState M := {s with old := s.current}
def snapshotFootprint (M : Nat) : List Address := (visits M).map (fun c => (1,offset c))
def executeSnapshot (m : Mem) : Option Mem := copyCells 0 1 (visits M) m

theorem snapshot_refines (m : Mem) (s : BoundaryState M) (P : Primitives M)
    (h : BeforeSnapshot (vertices := vertices) m s P) :
    ∃ ms, executeSnapshot (M := M) m = some ms ∧
      BoundaryRep vertices ms (snapshotState s) P ∧
      Stored 0 ms (fun c => s.current c.1 (channel c.2)) ∧
      Frame (snapshotFootprint M) m ms := by
  let u : Values M := fun c => s.current c.1 (channel c.2)
  let ms := storeCells 1 u (visits M) m
  refine ⟨ms, copy_executes 0 1 (by decide) _ m u h.current, ?_, ?_, store_frame 1 u _ m⟩
  · refine ⟨?_, ?_, ?_⟩
    · intro b p c
      cases b with
      | old => exact store_complete 1 u m (p,c)
      | residual =>
        change ms (5,offset (p,c)) = _
        dsimp only [ms]
        rw [store_other 5 1 (by decide)]
        exact h.residual (p,c)
      | truncation =>
        change ms (6,offset (p,c)) = _
        dsimp only [ms]
        rw [store_other 6 1 (by decide)]
        exact h.truncation (p,c)
    · intro p c
      change ms (8,p.val * 11 + c.val) = _
      dsimp only [ms]
      rw [store_other 8 1 (by decide)]
      exact h.primitives p c
    · intro site ghost hc c
      change ms (9 + site.marker.val,site.vertex.val * 11 + c.val) = _
      dsimp only [ms]
      rw [store_other (9 + site.marker.val) 1 (by omega)]
      exact h.cache site ghost hc c
  · intro c; dsimp only [ms]; rw [store_other 0 1 (by decide)]; exact h.current c

theorem snapshot_geometry (m ms : Mem) (vol per dt : Fin M → ℝ)
    (h : Frame (snapshotFootprint M) m ms) (hg : GeometryRep m vol per dt) :
    GeometryRep ms vol per dt := by
  have protect (b i : Nat) (hb : b ≠ 1) : ms (b,i) = m (b,i) := by
    apply h
    intro hin
    obtain ⟨c, _, e⟩ := List.mem_map.mp hin
    exact hb (congrArg Prod.fst e.symm)
  exact ⟨fun p => (protect 2 p.val (by decide)).trans (hg.1 p),
    fun p => (protect 3 p.val (by decide)).trans (hg.2.1 p),
    fun p => (protect 4 p.val (by decide)).trans (hg.2.2 p)⟩

/-- Required output contract of the still-unverified timestep/interior stages.
No old-buffer value or complete post-boundary representation is assumed. -/
structure InteriorEntry (m : Mem) (s : BoundaryState M) (P : Primitives M) : Prop where
  residual : Stored 5 m (fun c => s.residual c.1 (channel c.2))
  truncation : Stored 6 m (fun c => s.truncation c.1 (channel c.2))
  primitives : PrimitiveRep m P
  cache : CacheRep vertices m s.cache

/-- Actual caller order: snapshot first, then an explicitly OPEN interior/timestep gap,
then proved boundaries and update. The quantified gap is a relation, not an executed
unconstrained callback, and this theorem does not verify that gap. -/
theorem snapshot_interior_boundary_update (owned : N ≤ M) (bounds : Bounds M M K vertices)
    (m0 : Mem) (U : Residual M 2)
    (hcurrent : Stored 0 m0 (fun c => U c.1 (channel c.2))) :
    ∃ ms, executeSnapshot (M := M) m0 = some ms ∧
      Stored 1 ms (fun c => U c.1 (channel c.2)) ∧
      ∀ (mi : Mem) (changed : List Address) (s : BoundaryState M) (P : Primitives M)
        (vol dt : Fin M → ℝ) (g : IdealGas) (free : Primitive 2)
        (far : List (FarSite M K vertices)) (walls : List (WallVisit M)),
        (∀ i, mi (1,i) = ms (1,i)) →
        Frame changed ms mi →
        InteriorEntry (vertices := vertices) mi s P →
        GeometryRep mi vol (fun _ => 0) dt →
        FarReady g free (far.map (farVisit P)) →
        ∃ mb mu,
          executeSourceBoundary M g free far walls mi = some mb ∧
          InputsReady mb (ownedState owned
            (boundarySpec g free (far.map (farVisit P)) walls {s with old := U}))
            (fun p => vol (p.castLE owned)) (fun _ => 0) (fun p => dt (p.castLE owned)) ∧
          executeLoop (N := N) M SU2Boundary.Generated.volume_original SU2Boundary.Generated.periodic_original
            SU2Boundary.Generated.timestep_original SU2Boundary.Generated.points_original
            SU2Boundary.Generated.components_original mb = some mu ∧
          LoopContract (ownedState owned
            (boundarySpec g free (far.map (farVisit P)) walls {s with old := U}))
            (fun p => vol (p.castLE owned)) (fun p => dt (p.castLE owned)) mb mu ∧
          Frame ((snapshotFootprint M ++ changed) ++
            (boundaryFootprint far walls ++ (visits N).map destination)) m0 mu := by
  let u : Values M := fun c => U c.1 (channel c.2)
  let ms := storeCells 1 u (visits M) m0
  have hold : Stored 1 ms u := store_complete 1 u m0
  refine ⟨ms, copy_executes 0 1 (by decide) _ m0 u hcurrent, hold, ?_⟩
  intro mi changed s P vol dt g free far walls preserved framed entry geometry ready
  have rep : BoundaryRep vertices mi {s with old := U} P := by
    refine ⟨?_, entry.primitives, entry.cache⟩
    intro b p c
    cases b with
    | old => exact (preserved (offset (p,c))).trans (hold (p,c))
    | residual => exact entry.residual (p,c)
    | truncation => exact entry.truncation (p,c)
  obtain ⟨mb, mu, eb, _, er, eu, hu, fu⟩ := source_boundary_to_complete_update owned bounds
    g free P far walls {s with old := U} mi vol dt rep geometry ready
  exact ⟨mb, mu, eb, er, eu, hu,
    frame_trans (frame_trans (store_frame 1 u _ m0) framed) fu⟩

end SU2Initialization
