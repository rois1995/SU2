import SU2Initialization.Compatibility

noncomputable section
namespace SU2Initialization
open SU2 SU2Source SU2Boundary SU2BoundaryMemory
variable {N : Nat}

abbrev Values (N : Nat) := Fin N × Fin 4 → ℝ

def Stored (b : Nat) (m : Mem) (U : Values N) : Prop :=
  ∀ c, m (b, offset c) = some (U c)

def storeCells (b : Nat) (U : Values N) : List (Fin N × Fin 4) → Mem → Mem
  | [], m => m
  | c :: cs, m => storeCells b U cs (put m (b,offset c) (U c))

/-- Loads from evolving memory, then writes the other buffer. No expected result input. -/
def copyCells (src dst : Nat) : List (Fin N × Fin 4) → Mem → Option Mem
  | [], m => some m
  | c :: cs, m => do
      let x ← m (src,offset c)
      copyCells src dst cs (put m (dst,offset c) x)

theorem stored_put_other (src dst : Nat) (h : src ≠ dst) (m : Mem) (U : Values N)
    (c : Fin N × Fin 4) (x : ℝ) (hm : Stored src m U) :
    Stored src (put m (dst,offset c) x) U := by
  intro q
  simpa [put, Function.update_of_ne (show (src,offset q) ≠ (dst,offset c) from
    fun e => h (congrArg Prod.fst e))] using hm q

theorem copy_executes (src dst : Nat) (h : src ≠ dst) (cs : List (Fin N × Fin 4))
    (m : Mem) (U : Values N) (hm : Stored src m U) :
    copyCells src dst cs m = some (storeCells dst U cs m) := by
  induction cs generalizing m with
  | nil => rfl
  | cons c cs ih =>
    simp only [copyCells, hm c, Option.some_bind, storeCells]
    exact ih _ (stored_put_other src dst h m U c (U c) hm)

theorem store_value (b : Nat) (U : Values N) (cs : List (Fin N × Fin 4))
    (m : Mem) (c : Fin N × Fin 4) :
    storeCells b U cs m (b,offset c) = if c ∈ cs then some (U c) else m (b,offset c) := by
  induction cs generalizing m with
  | nil => simp [storeCells]
  | cons a cs ih =>
    rw [storeCells, ih]
    by_cases hc : c ∈ cs
    · simp [hc]
    · by_cases ha : c = a
      · subst a; simp [hc, put]
      · have hn : (b,offset c) ≠ (b,offset a) := fun e => ha (offset_injective (congrArg Prod.snd e))
        simp [hc, ha, put, Function.update_of_ne hn]

theorem store_frame (b : Nat) (U : Values N) (cs : List (Fin N × Fin 4)) (m : Mem) :
    Frame (cs.map (fun c => (b,offset c))) m (storeCells b U cs m) := by
  induction cs generalizing m with
  | nil => exact frame_refl _ _
  | cons c cs ih =>
    simpa only [storeCells, List.map_cons, List.singleton_append] using
      frame_trans (put_frame m (b,offset c) (U c)) (ih (put m (b,offset c) (U c)))

theorem store_complete (b : Nat) (U : Values N) (m : Mem) :
    Stored b (storeCells b U (visits N) m) U := by
  intro c
  simp [store_value, visits_coverage c]

theorem store_other (src dst : Nat) (h : src ≠ dst) (U : Values N)
    (cs : List (Fin N × Fin 4)) (m : Mem) (i : Nat) :
    storeCells dst U cs m (src,i) = m (src,i) := by
  apply store_frame dst U cs m
  intro hin
  obtain ⟨c, _, e⟩ := List.mem_map.mp hin
  exact h (congrArg Prod.fst e.symm)

/-- The constructor's selected 2D non-restart values; energy is specific total energy. -/
def seed (density vx vy specificEnergy : ℝ) : Fin 4 → ℝ :=
  ![density * 1, density * vx, density * vy, density * specificEnergy]

def constructMemory (density vx vy specificEnergy : ℝ) (m : Mem) : Option Mem :=
  let m1 := storeCells 6 (fun _ : Fin N × Fin 4 => 0) (visits N) m
  let m2 := storeCells 0 (fun c => seed density vx vy specificEnergy c.2) (visits N) m1
  copyCells 0 1 (visits N) m2

theorem initialize_refines (density vx vy specificEnergy : ℝ) (m : Mem) :
    ∃ mi, constructMemory (N := N) density vx vy specificEnergy m = some mi ∧
      Stored (N := N) 0 mi (fun c => seed density vx vy specificEnergy c.2) ∧
      Stored (N := N) 1 mi (fun c => seed density vx vy specificEnergy c.2) ∧
      Stored (N := N) 6 mi (fun _ => 0) ∧
      Frame ((visits N).map (fun c => (6,offset c)) ++
        (visits N).map (fun c => (0,offset c)) ++ (visits N).map (fun c => (1,offset c))) m mi := by
  let z := storeCells 6 (fun _ : Fin N × Fin 4 => 0) (visits N) m
  let u : Values N := fun c => seed density vx vy specificEnergy c.2
  let cur := storeCells 0 u (visits N) z
  let old := storeCells 1 u (visits N) cur
  have hc : Stored 0 cur u := store_complete 0 u z
  refine ⟨old, copy_executes 0 1 (by decide) _ cur u hc, ?_, store_complete 1 u cur, ?_, ?_⟩
  · intro c; dsimp only [old]; rw [store_other 0 1 (by decide)]; exact hc c
  · intro c
    dsimp only [old, cur]
    rw [store_other 6 1 (by decide), store_other 6 0 (by decide)]
    exact store_complete 6 _ m c
  · simpa only [List.append_assoc] using
      frame_trans (frame_trans (store_frame 6 _ _ m) (store_frame 0 u _ z)) (store_frame 1 u _ cur)

/-- Every flat C++ copy-loop index has one point/component address, including all
halo rows. For N=0 the interval is empty. -/
theorem flat_copy_coverage (i : Nat) (hi : i < N * 4) :
    ∃ c : Fin N × Fin 4, c ∈ visits N ∧ offset c = i := by
  have hp : i / 4 < N := (Nat.div_lt_iff_lt_mul (by decide : 0 < 4)).mpr hi
  let c : Fin N × Fin 4 := (⟨i / 4,hp⟩,⟨i % 4,Nat.mod_lt _ (by decide)⟩)
  refine ⟨c, visits_coverage c, ?_⟩
  simpa only [offset, c, Nat.mul_comm] using Nat.div_add_mod i 4

/-- The constructor's supplied specific energy is related to M1 only under this explicit
thermodynamic formula; freestream nondimensionalization is not proved here. -/
theorem seed_compatible (g : IdealGas) (W : Primitive 2) (h : W.density ≠ 0) :
    seed W.density (W.velocity 0) (W.velocity 1) (specificEnergy g W) =
      components (toConservative g W) := by
  funext c
  fin_cases c <;> simp [seed, components, channel, toConservative, state, specificEnergy]
  field_simp [h, ne_of_gt (sub_pos.mpr g.gamma_gt_one)]
  ring

theorem seed_admissible (g : IdealGas) (W : Primitive 2) (h : PrimitiveAdmissible W) :
    Admissible (fromComponents (seed W.density (W.velocity 0) (W.velocity 1) (specificEnergy g W))) := by
  rw [seed_compatible g W (ne_of_gt h.1), components_roundtrip]
  apply (admissibility_compatible g _).mp
  rw [← conservative_compatible]
  exact CFDLean.primitive_admissible_toConservative g.gamma (primitiveCFD W) g.gamma_gt_one h

end SU2Initialization
