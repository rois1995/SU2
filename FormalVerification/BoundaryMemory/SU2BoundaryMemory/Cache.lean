import SU2BoundaryMemory.Wall

set_option maxRecDepth 100000
set_option maxHeartbeats 4000000
noncomputable section
namespace SU2BoundaryMemory
open SU2 SU2Source SU2Boundary
variable {N M K : Nat} {vertices : Fin K → Nat}

def cacheColumns : List (Fin 6) := [0,1,2,3,4,5]
def cacheFootprint (site : CacheSite K vertices) : List Address :=
  cacheColumns.map (fun c : Fin 6 => cacheAddr site.marker site.vertex c)

theorem cache_columns_complete (c : Fin 6) : c ∈ cacheColumns := by fin_cases c <;> decide

theorem cache_column_injective (site : CacheSite K vertices) :
    Function.Injective (fun c : Fin 6 => cacheAddr site.marker site.vertex c) := by
  intro i j h
  have ho := congrArg Prod.snd h
  simp only [cacheAddr, Prod.snd] at ho
  exact Fin.ext (Nat.add_left_cancel ho)

theorem write_cache_fields (site : CacheSite K vertices) (g : Ghost) (m : Mem) :
    writeCache .normal false site g m =
      writeFields (fun c : Fin 6 => cacheAddr site.marker site.vertex c) (ghostField g) cacheColumns m := rfl

theorem write_cache_value (site : CacheSite K vertices) (g : Ghost) (m : Mem) (c : Fin 6) :
    writeCache .normal false site g m (cacheAddr site.marker site.vertex c) = some (ghostField g c) := by
  rw [write_cache_fields, write_fields_value _ (cache_column_injective site), if_pos (cache_columns_complete c)]

theorem write_cache_frame (site : CacheSite K vertices) (g : Ghost) (m : Mem) :
    Frame (cacheFootprint site) m (writeCache .normal false site g m) := by
  rw [write_cache_fields]
  exact write_fields_frame (fun c : Fin 6 => cacheAddr site.marker site.vertex c) (ghostField g) cacheColumns m

theorem cache_other_row (s t : CacheSite K vertices) (i j : Fin 6)
    (h : cacheKey t.marker t.vertex ≠ cacheKey s.marker s.vertex) :
    cacheAddr t.marker t.vertex i ≠ cacheAddr s.marker s.vertex j := by
  intro eq
  have e := cache_addr_injective (i := i.castLE (by decide)) (j := j.castLE (by decide)) eq
  exact h (by rw [e.1, e.2.1])

theorem cache_tail_frame (site : CacheSite K vertices) (g : Ghost) (m : Mem)
    (c : Fin 11) (hc : 6 ≤ c.val) :
    writeCache .normal false site g m (cacheAddr site.marker site.vertex c) =
      m (cacheAddr site.marker site.vertex c) := by
  apply write_cache_frame site g m
  intro hin
  obtain ⟨j, _, eq⟩ := (List.mem_map (f := fun j : Fin 6 => cacheAddr site.marker site.vertex j)).mp (show _ ∈ cacheColumns.map (fun j : Fin 6 => cacheAddr site.marker site.vertex j) from hin)
  have e := cache_addr_injective (i := j.castLE (by decide)) (j := c) eq
  have hj := j.isLt
  have he := congrArg Fin.val e.2.2
  simp only [Fin.coe_castLE] at he
  omega

def cachedState (s : BoundaryState N) (site : CacheSite K vertices) (g : Ghost) : BoundaryState N :=
  {s with cache := Function.update s.cache (cacheKey site.marker site.vertex) (some g)}

theorem cache_memory_rep (m : Mem) (s : BoundaryState N) (P : Primitives N)
    (site : CacheSite K vertices) (g : Ghost) (h : BoundaryRep vertices m s P) :
    BoundaryRep vertices (writeCache .normal false site g m) (cachedState s site g) P := by
  have hf := write_cache_frame site g m
  refine ⟨?_, ?_, ?_⟩
  · intro b p c
    rw [hf]
    · exact h.buffers b p c
    · intro hin
      obtain ⟨j, _, eq⟩ := List.mem_map.mp hin
      exact cache_flow_separate b.block (Nat.le_of_lt (buffer_block_bound b)) p c site.marker site.vertex j eq
  · intro p c
    rw [hf]
    · exact h.primitives p c
    · intro hin
      obtain ⟨j, _, eq⟩ := List.mem_map.mp hin
      exact primitive_cache_separate p c site.marker site.vertex j eq.symm
  · intro t w hw c
    by_cases hk : cacheKey t.marker t.vertex = cacheKey site.marker site.vertex
    · have hwg : g = w := by simpa [cachedState, hk] using hw
      subst w
      obtain ⟨hm,hv⟩ := cache_key_injective hk
      rw [hm,hv]
      exact write_cache_value site g m c
    · have hw' : s.cache (cacheKey t.marker t.vertex) = some w := by
        simpa [cachedState, Function.update_of_ne hk] using hw
      rw [hf]
      · exact h.cache t w hw' c
      · intro hin
        obtain ⟨j, _, eq⟩ := (List.mem_map (f := fun j : Fin 6 => cacheAddr site.marker site.vertex j)).mp (show _ ∈ cacheColumns.map (fun j : Fin 6 => cacheAddr site.marker site.vertex j) from hin)
        exact cache_other_row site t c j hk eq.symm

theorem read_cached_written (m : Mem) (site : CacheSite K vertices) (g : Ghost) :
    readCached .normal site (writeCache .normal false site g m) = some (g.primitive, g.enthalpy) := by
  have h := write_cache_value site g m
  simp only [readCached, selectedCacheAddr]
  change (do
    let vx ← writeCache .normal false site g m (cacheAddr site.marker site.vertex 1)
    let vy ← writeCache .normal false site g m (cacheAddr site.marker site.vertex 2)
    let pr ← writeCache .normal false site g m (cacheAddr site.marker site.vertex 3)
    let ro ← writeCache .normal false site g m (cacheAddr site.marker site.vertex 4)
    let en ← writeCache .normal false site g m (cacheAddr site.marker site.vertex 5)
    pure ((⟨ro, ![vx,vy], pr⟩ : Primitive 2), en)) = _
  simp only [show (1 : Nat) = ((1 : Fin 6) : Nat) from rfl,
    show (2 : Nat) = ((2 : Fin 6) : Nat) from rfl,
    show (3 : Nat) = ((3 : Fin 6) : Nat) from rfl,
    show (4 : Nat) = ((4 : Fin 6) : Nat) from rfl,
    show (5 : Nat) = ((5 : Fin 6) : Nat) from rfl,
    h 1, h 2, h 3, h 4, h 5, Option.bind_eq_bind, Option.some_bind]
  dsimp only [ghostField, Matrix.cons_val_zero, Matrix.cons_val_succ]
  congr 2
  cases g with
  | mk w t e =>
    cases w with
    | mk r v p =>
      congr 1
      funext i
      fin_cases i <;> rfl

end SU2BoundaryMemory
