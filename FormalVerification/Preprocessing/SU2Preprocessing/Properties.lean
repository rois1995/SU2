import SU2Preprocessing.Composition

noncomputable section
namespace SU2Preprocessing
open SU2 SU2Source SU2Boundary SU2BoundaryMemory SU2Initialization
variable {M : Nat}

theorem velocity_bounds (p : Fin M) : (velocityAddr p).2 < M := p.isLt

theorem velocity_injective (p q : Fin M) (h : velocityAddr p = velocityAddr q) : p = q :=
  Fin.ext (congrArg Prod.snd h)

theorem velocity_flow_separate (b : Nat) (hb : b ≠ 7) (p q : Fin M) (c : Fin 4) :
    flowAddr b p c ≠ velocityAddr q := fun h => hb (congrArg Prod.fst h)

theorem velocity_primitive_separate (p q : Fin M) (c : Fin 11) : velocityAddr p ≠ primitiveAddr q c := by
  simp [velocityAddr, primitiveAddr]

theorem counter_fits (g : IdealGas) (d : Data M) (h : M < 2^64) :
    (modelPoints g (points M) d 0).2 < 2^64 := by
  have bound := counter_bound g (points M) d 0
  rw [zero_add, points_count] at bound
  exact lt_of_le_of_lt bound h

theorem model_old (g : IdealGas) (ps : List (Fin M)) (d : Data M) (count : Nat) :
    (modelPoints g ps d count).1.old = d.old := by
  induction ps generalizing d count with
  | nil => rfl
  | cons p ps ih => exact ih _ _

theorem model_unvisited (g : IdealGas) (ps : List (Fin M)) (d : Data M) (count : Nat)
    (q : Fin M) (hq : q ∉ ps) :
    (modelPoints g ps d count).1.current q = d.current q ∧
    (modelPoints g ps d count).1.primitive q = d.primitive q ∧
    (modelPoints g ps d count).1.velocity q = d.velocity q := by
  induction ps generalizing d count with
  | nil => exact ⟨rfl,rfl,rfl⟩
  | cons p ps ih =>
    have neq : q ≠ p := by intro h; exact hq (by simp [h])
    have rest : q ∉ ps := fun h => hq (List.mem_cons_of_mem p h)
    simpa only [modelPoints, advanceData, Function.update_of_ne neq] using ih (advanceData g p d) _ rest

/-- Arbitrary real trials are allowed. Only the physical guarantee needs admissible old rows. -/
theorem model_admissible (g : IdealGas) (ps : List (Fin M)) (d : Data M) (count : Nat)
    (backups : ∀ p, Admissible (d.old p)) (q : Fin M)
    (visited : q ∈ ps ∨ Admissible (d.current q)) :
    Admissible ((modelPoints g ps d count).1.current q) := by
  induction ps generalizing d count with
  | nil => exact visited.resolve_left (by simp)
  | cons p ps ih =>
    apply ih (advanceData g p d) _ backups
    rcases visited with member | valid
    · rcases List.mem_cons.mp member with eq | tail
      · subst q
        right
        simp only [advanceData, Function.update_self, set_point_current]
        exact selected_admissible _ _ (backups p)
      · exact Or.inl tail
    · by_cases eq : q = p
      · subst q
        right
        simp only [advanceData, Function.update_self, set_point_current]
        exact selected_admissible _ _ (backups p)
      · right
        simpa only [advanceData, Function.update_of_ne eq] using valid

theorem complete_admissible (g : IdealGas) (d : Data M) (backups : ∀ p, Admissible (d.old p)) (q : Fin M) :
    Admissible ((modelPoints g (points M) d 0).1.current q) :=
  model_admissible g _ d 0 backups q (Or.inl (points_coverage q))

theorem model_tail (g : IdealGas) (ps : List (Fin M)) (d : Data M) (count : Nat)
    (q : Fin M) (c : Fin 11) (hc : 7 ≤ c.val) :
    (modelPoints g ps d count).1.primitive q c = d.primitive q c := by
  induction ps generalizing d count with
  | nil => rfl
  | cons p ps ih =>
    rw [modelPoints, ih]
    by_cases eq : q = p
    · subst q
      simp only [advanceData, Function.update_self]
      exact set_point_tail g _ _ _ c hc
    · simp only [advanceData, Function.update_of_ne eq]

/-- A no-duplicate traversal computes the protected point operation for every visited
row, using its original input. Repeated lists retain the more general stateful fold theorem. -/
theorem model_row (g : IdealGas) (ps : List (Fin M)) (d : Data M) (count : Nat)
    (unique : ps.Nodup) (q : Fin M) (visited : q ∈ ps) :
    let result := setPoint g (d.current q) (d.old q) (d.primitive q)
    (modelPoints g ps d count).1.current q = result.current ∧
    (modelPoints g ps d count).1.primitive q = result.primitive ∧
    (modelPoints g ps d count).1.velocity q = some result.velocity2 := by
  dsimp only
  induction ps generalizing d count with
  | nil => simp at visited
  | cons p ps ih =>
    have no := List.nodup_cons.mp unique
    by_cases eq : q = p
    · subst q
      have unchanged := model_unvisited g ps (advanceData g p d)
        (count + if (setPoint g (d.current p) (d.old p) (d.primitive p)).physical then 0 else 1) p no.1
      simpa only [modelPoints,advanceData,Function.update_self] using unchanged
    · have member : q ∈ ps := (List.mem_cons.mp visited).resolve_left eq
      have result := ih (advanceData g p d)
        (count + if (setPoint g (d.current p) (d.old p) (d.primitive p)).physical then 0 else 1) no.2 member
      simpa only [modelPoints,advanceData,Function.update_of_ne eq] using result

theorem complete_primitive_correct (g : IdealGas) (d : Data M)
    (backups : ∀ p, Admissible (d.old p)) (q : Fin M) :
    let result := (modelPoints g (points M) d 0).1
    result.current q = selectedState (d.current q) (d.old q) ∧
    result.primitive q = primitiveFields g (selectedState (d.current q) (d.old q)) (d.primitive q) := by
  obtain ⟨cur,prim,_⟩ := model_row g (points M) d 0 points_no_duplicates q (points_coverage q)
  have scientific := set_point_refines g (d.current q) (d.old q) (d.primitive q) (backups q)
  exact ⟨cur.trans scientific.1,prim.trans scientific.2.1⟩

/-- The tail statement is an exact memory frame for every row, including halo rows. -/
theorem preprocessing_tail (g : IdealGas) (s0 : ControlState) (d : Data M)
    (h : DataRep s0.machine d) (hf : FluidReady g s0.machine.fluid)
    (q : Fin M) (c : Fin 11) (hc : 7 ≤ c.val) :
    ∃ pre, executeCommon (M := M) s0 = some pre ∧
      pre.machine.memory (primitiveAddr q c) = s0.machine.memory (primitiveAddr q c) := by
  obtain ⟨processed,ep,hd,_,_,_,_,_⟩ := points_refine g (points M) s0.machine d 0 h hf
  let pre : ControlState := ⟨⟨clearResidual (N := M) processed.memory,processed.fluid⟩,
    (modelPoints g (points M) d 0).2,(modelPoints g (points M) d 0).2⟩
  refine ⟨pre,?_,?_⟩
  · simp [executeCommon,ep,pre]
  · change clearResidual (N := M) processed.memory (8,q.val*11+c.val) = _
    rw [clear_other 8 (by decide)]
    change processed.memory (primitiveAddr q c) = _
    rw [hd.primitive,model_tail g _ d 0 q c hc]
    exact (h.primitive q c).symm

end SU2Preprocessing
