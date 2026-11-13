import SU2Preprocessing.SourceConnection

noncomputable section
namespace SU2Preprocessing
open SU2 SU2Source SU2Boundary SU2BoundaryMemory SU2Initialization
variable {M K : Nat} {vertices : Fin K → Nat}

def zeroMemory : List Address → Mem → Mem
  | [], m => m
  | a :: rest, m => zeroMemory rest (put m a 0)

theorem zero_read (as : List Address) (m : Mem) (a : Address) :
    zeroMemory as m a = if a ∈ as then some 0 else m a := by
  induction as generalizing m with
  | nil => simp [zeroMemory]
  | cons b rest ih =>
    rw [zeroMemory,ih]
    by_cases h : a ∈ rest
    · simp [h]
    · by_cases eq : a = b <;> simp [h,eq,put,Function.update_apply]

theorem zero_frame (as : List Address) (m : Mem) : Frame as m (zeroMemory as m) := by
  intro a ha
  simp [zero_read,ha]

def primitiveCells (M : Nat) : List Address :=
  (points M).flatMap (fun p => List.ofFn (fun c : Fin 11 => primitiveAddr p c))

theorem primitive_cells_mem (p : Fin M) (c : Fin 11) : primitiveAddr p c ∈ primitiveCells M :=
  List.mem_flatMap.mpr ⟨p,points_coverage p,List.mem_ofFn.mpr ⟨c,rfl⟩⟩

/-- Projection of constructor order onto represented fields. Other auxiliary C++
allocations are outside this memory projection, rather than claimed unchanged. -/
def constructorZeros (M : Nat) : List Address :=
  (visits M).map (fun c => (0,offset c)) ++
  (visits M).map (fun c => (1,offset c)) ++
  (visits M).map (fun c => (6,offset c)) ++ primitiveCells M ++
  (points M).map velocityAddr ++ (points M).map (fun p => (4,p.val))

/-- Base zeros, flow zeros, seed stores, then deep old copy. No expected result is installed. -/
def executeConstructor (density vx vy specificEnergy : ℝ) (m : Mem) : Option Mem :=
  let z := zeroMemory (constructorZeros M) m
  let cur := storeCells 0 (fun c => seed density vx vy specificEnergy c.2) (visits M) z
  copyCells 0 1 (visits M) cur

def constructorData (density vx vy specificEnergy : ℝ) : Data M :=
  let U := fromComponents (seed density vx vy specificEnergy)
  ⟨fun _ => U,fun _ => U,fun _ _ => 0,fun _ => some 0⟩

theorem seed_components (density vx vy specificEnergy : ℝ) (c : Fin 4) :
    fromComponents (seed density vx vy specificEnergy) (channel c) = seed density vx vy specificEnergy c := by
  fin_cases c <;> simp [fromComponents,seed,channel,state]

theorem constructor_refines (density vx vy specificEnergy : ℝ) (m : Mem) (f : Fluid) :
    ∃ mi, executeConstructor (M := M) density vx vy specificEnergy m = some mi ∧
      DataRep (M := M) ⟨mi,f⟩ (constructorData density vx vy specificEnergy) ∧
      Stored (N := M) 6 mi (fun _ => 0) ∧
      (∀ p : Fin M, mi (4,p.val) = some 0) ∧
      Frame (constructorZeros M ++ (visits M).map (fun c => (0,offset c)) ++
        (visits M).map (fun c => (1,offset c))) m mi := by
  let z := zeroMemory (constructorZeros M) m
  let u : Values M := fun c => seed density vx vy specificEnergy c.2
  let cur := storeCells 0 u (visits M) z
  let mi := storeCells 1 u (visits M) cur
  have current : Stored 0 cur u := store_complete 0 u z
  have old : Stored 1 mi u := store_complete 1 u cur
  have rest (b : Nat) (h0 : b ≠ 0) (h1 : b ≠ 1) (i : Nat)
      (mem : (b,i) ∈ constructorZeros M) : mi (b,i) = some 0 := by
    change storeCells 1 u (visits M) cur (b,i) = _
    rw [store_other b 1 h1]
    change storeCells 0 u (visits M) z (b,i) = _
    rw [store_other b 0 h0]
    exact (zero_read (constructorZeros M) m (b,i)).trans (if_pos mem)
  refine ⟨mi,copy_executes 0 1 (by decide) _ cur u current,?_,?_,?_,?_⟩
  · refine ⟨?_,?_,?_,?_⟩
    · intro p c
      change mi (0,offset (p,c)) = some (fromComponents (seed density vx vy specificEnergy) (channel c))
      rw [seed_components]
      change storeCells 1 u (visits M) cur (0,offset (p,c)) = _
      rw [store_other 0 1 (by decide)]
      exact current (p,c)
    · intro p c
      change mi (1,offset (p,c)) = some (fromComponents (seed density vx vy specificEnergy) (channel c))
      rw [seed_components]; exact old (p,c)
    · intro p c
      apply rest 8 (by decide) (by decide)
      have h := primitive_cells_mem p c
      exact List.mem_append_left _ (List.mem_append_left _ (List.mem_append_right _ h))
    · intro p
      apply rest 7 (by decide) (by decide)
      have h : velocityAddr p ∈ (points M).map velocityAddr := List.mem_map.mpr ⟨p,points_coverage p,rfl⟩
      exact List.mem_append_left _ (List.mem_append_right _ h)
  · intro c
    apply rest 6 (by decide) (by decide)
    have h : (6,offset c) ∈ (visits M).map (fun c => (6,offset c)) := List.mem_map.mpr ⟨c,visits_coverage c,rfl⟩
    exact List.mem_append_left _ (List.mem_append_left _ (List.mem_append_left _ (List.mem_append_right _ h)))
  · intro p
    apply rest 4 (by decide) (by decide)
    exact List.mem_append_right _ (List.mem_map.mpr ⟨p,points_coverage p,rfl⟩)
  · exact frame_trans (frame_trans (zero_frame _ m) (store_frame 0 u _ z)) (store_frame 1 u _ cur)

/-- Scientific constructor assumption, supplied by M1's approved conversion contract.
Freestream nondimensionalization and later admissibility preservation remain open. -/
theorem constructor_backups_admissible (g : IdealGas) (W : Primitive 2) (h : PrimitiveAdmissible W) :
    ∀ p : Fin M, Admissible ((constructorData W.density (W.velocity 0) (W.velocity 1) (specificEnergy g W)).old p) :=
  fun _ => seed_admissible g W h

/-- LP64 size qualification for the additional one-scalar-per-row scratch buffer. -/
theorem velocity_byte_bounds (bounds : Bounds M M K vertices) (p : Fin M) :
    8 * p.val + 7 < 2^64 := by
  have scalar : 8 * (M * 1) ≤ 2^64 := by have size := bounds.points; omega
  simpa using byte_offset_bound M 1 p.val 0 p.isLt (by decide) scalar

theorem counter_fits_storage_bounds (g : IdealGas) (d : Data M) (bounds : Bounds M M K vertices) :
    (modelPoints g (points M) d 0).2 < 2^64 := by
  apply counter_fits g d
  have size := bounds.points
  omega

end SU2Preprocessing
