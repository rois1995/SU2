import SU2Boundary.Boundary
import Mathlib.Data.List.FinRange

set_option maxRecDepth 100000
set_option maxHeartbeats 4000000

noncomputable section
namespace SU2Boundary
open SU2 SU2Source SU2Euler
variable {N M : Nat}

abbrev Cell (N : Nat) := Fin N × Fin 4

def offset (c : Cell N) : Nat := c.1.val * 4 + c.2.val
def destination (c : Cell N) : Address := (0, offset c)

/-- Point-major, then component-major order, including N=0. -/
def visits (N : Nat) : List (Cell N) :=
  (List.ofFn (fun p : Fin N => p)).flatMap (fun p => List.ofFn (fun c : Fin 4 => (p, c)))

theorem visits_coverage (c : Cell N) : c ∈ visits N := by
  simp only [visits, List.mem_flatMap, List.mem_ofFn]
  exact ⟨c.1, ⟨c.1, rfl⟩, c.2, rfl⟩

theorem visits_order : (visits N).Pairwise (fun a b => offset a < offset b) := by
  simp only [visits, List.pairwise_flatMap, List.mem_ofFn, List.pairwise_ofFn]
  constructor
  · intro p _ i j h
    simp only [offset]
    exact Nat.add_lt_add_left h _
  · intro i j h x hx y hy
    obtain ⟨a, rfl⟩ := hx
    obtain ⟨b, rfl⟩ := hy
    dsimp only [offset]
    have ha := a.isLt
    omega

theorem visits_no_duplicates : (visits N).Nodup := by
  apply List.Pairwise.imp _ visits_order
  intro a b h eq
  subst b
  exact (Nat.lt_irrefl _ h)

theorem offset_injective : Function.Injective (offset (N := N)) := by
  intro a b h
  have ha := a.2.isLt
  have hb := b.2.isLt
  have hp : a.1.val = b.1.val := by dsimp [offset] at h; omega
  have hc : a.2.val = b.2.val := by dsimp [offset] at h; omega
  exact Prod.ext (Fin.ext hp) (Fin.ext hc)

theorem destination_injective : Function.Injective (destination (N := N)) := by
  intro a b h
  exact offset_injective (congrArg Prod.snd h)

theorem iteration_bounds (c : Cell N) (owned : N ≤ M) (size : 8 * (M * 4) ≤ 2^64) :
    c.1.val < M ∧ c.2.val < 4 ∧ offset c < M * 4 ∧ 8 * offset c + 7 < 2^64 ∧
    c.1.val + 1 < 2^64 ∧ c.2.val + 1 < 2^16 := by
  have hp : c.1.val < M := lt_of_lt_of_le c.1.isLt owned
  have hc := c.2.isLt
  have ho := row_major_bound M 4 c.1.val c.2.val hp hc
  have hb := byte_offset_bound M 4 c.1.val c.2.val hp hc size
  dsimp only [offset]
  exact ⟨hp, hc, ho, hb, by omega, by omega⟩

theorem getter_index_bounds (p : Fin N) (owned : N ≤ M) (size : 8 * (M * 4) ≤ 2^64) :
    p.val < M ∧ 8 * p.val + 7 < 2^64 := by
  have hp : p.val < M := lt_of_lt_of_le p.isLt owned
  have hb := byte_offset_bound M 4 p.val 0 hp (by decide) size
  exact ⟨hp, by omega⟩

def selectedVisits (p v : LoopCode) (N : Nat) : List (Cell N) :=
  (visits N).filter (fun c => c.1.val < N - (if p.dropLast then 1 else 0) &&
    c.2.val < 4 - (if v.dropLast then 1 else 0))

theorem selected_visits_complete : selectedVisits Generated.points_original Generated.components_original N = visits N := by
  have hp : Generated.points_original = ⟨false⟩ := by decide
  have hv : Generated.components_original = ⟨false⟩ := by decide
  simp [selectedVisits, hp, hv]

theorem loop_refactor_preserves_order :
    selectedVisits Generated.points_refactored Generated.components_refactored N =
    selectedVisits Generated.points_original Generated.components_original N := by
  have hp : Generated.points_refactored = Generated.points_original := by decide
  have hv : Generated.components_refactored = Generated.components_original := by decide
  rw [hp, hv]

/-- Initialization relation for the read-only numerical buffers. Current may contain
arbitrary values or be uninitialized; old is the POST-boundary buffer. Separate block
identities exclude aliasing. Volume/dt origin and real decoding remain caller obligations. -/
def InputsReady (m : Memory ℝ) (s : BoundaryState N) (vol periodic dt : Fin N → ℝ) : Prop :=
  (∀ c : Cell N, m (1, offset c) = some (s.old c.1 (channel c.2))) ∧
  (∀ c : Cell N, m (5, offset c) = some (s.residual c.1 (channel c.2))) ∧
  (∀ c : Cell N, m (6, offset c) = some (s.truncation c.1 (channel c.2))) ∧
  (∀ p, m (2, p.val) = some (vol p)) ∧
  (∀ p, m (3, p.val) = some (periodic p)) ∧
  (∀ p, m (4, p.val) = some (dt p))

def readGetter (g : Getter) (m : Memory ℝ) (p : Nat) : Option ℝ := m (getterBlock g, p)

theorem getter_connection (m : Memory ℝ) (s : BoundaryState N) (vol per dt : Fin N → ℝ)
    (h : InputsReady m s vol per dt) (p : Fin N) :
    readGetter Generated.volume_original m p.val = some (vol p) ∧
    readGetter Generated.periodic_original m p.val = some (per p) ∧
    readGetter Generated.timestep_original m p.val = some (dt p) := by
  exact ⟨h.2.2.2.1 p, h.2.2.2.2.1 p, h.2.2.2.2.2 p⟩

theorem getter_const_overloads : Generated.volumeConst_original = Generated.volume_original ∧
    Generated.periodicConst_original = Generated.periodic_original := by decide

/-- Executes the actual selected apply, Euler call and AddSolution IR. Getters read from
memory on each iteration. Failure is retained for missing cells or invalid matrix extents. -/
def executeCell (rows : Nat) (volumeGetter periodicGetter timestepGetter : Getter) (m : Memory ℝ) (c : Cell N) :
    Option (Memory ℝ) := do
  let vol ← readGetter volumeGetter m c.1.val
  let per ← readGetter periodicGetter m c.1.val
  let dt ← readGetter timestepGetter m c.1.val
  let r ← m (5, offset c)
  let t ← m (6, offset c)
  let res := opReal SU2Euler.Generated.apply_originalCode r t
  let inc := (if SU2Euler.Generated.call_originalCode then -res else res) * (dt / (vol + per))
  execute realOperations (stateLayout rows 2) ⟨c.1.val, c.2.val, inc⟩
    SU2Euler.Generated.update_originalCode m

def executeCells (rows : Nat) (volumeGetter periodicGetter timestepGetter : Getter) :
    List (Cell N) → Memory ℝ → Option (Memory ℝ)
  | [], m => some m
  | c :: cs, m => (executeCell rows volumeGetter periodicGetter timestepGetter m c).bind (executeCells rows volumeGetter periodicGetter timestepGetter cs)

/-- Checked source-loop interpretation. The diagnostic bound-minus-one variant
fails explicitly on an empty point domain instead of modeling unsigned underflow
as natural-number saturation. Original loop bounds support N=0. -/
def executeLoop (rows : Nat) (volumeGetter periodicGetter timestepGetter : Getter) (points vars : LoopCode)
    (m : Memory ℝ) : Option (Memory ℝ) :=
  if points.dropLast = true ∧ N = 0 then none else
    executeCells rows volumeGetter periodicGetter timestepGetter (selectedVisits points vars N) m

theorem original_loop_execution (rows : Nat) (volumeGetter periodicGetter timestepGetter : Getter) (m : Memory ℝ) :
    executeLoop (N := N) rows volumeGetter periodicGetter timestepGetter Generated.points_original Generated.components_original m =
      executeCells rows volumeGetter periodicGetter timestepGetter (visits N) m := by
  have hp : Generated.points_original.dropLast = false := by decide
  simp [executeLoop, hp, selected_visits_complete]

/-- Proof helper: an independently supplied value is stored in each visited current cell. -/
def writes (value : Cell N → ℝ) : List (Cell N) → Memory ℝ → Memory ℝ
  | [], m => m
  | c :: cs, m => writes value cs (Function.update m (destination c) (some (value c)))

def cellValue (s : BoundaryState N) (vol per dt : Fin N → ℝ) (c : Cell N) : ℝ :=
  s.old c.1 (channel c.2) - dt c.1 / (vol c.1 + per c.1) *
    (s.residual c.1 (channel c.2) + s.truncation c.1 (channel c.2))

theorem inputs_after_store (m : Memory ℝ) (s : BoundaryState N) (vol per dt : Fin N → ℝ)
    (h : InputsReady m s vol per dt) (c : Cell N) (x : ℝ) :
    InputsReady (Function.update m (destination c) (some x)) s vol per dt := by
  simpa [InputsReady, destination, Function.update_apply] using h

theorem execute_cell_correct (m : Memory ℝ) (s : BoundaryState N) (vol per dt : Fin N → ℝ)
    (ready : InputsReady m s vol per dt) (owned : N ≤ M) (size : 8 * (M * 4) ≤ 2^64) (c : Cell N) :
    executeCell M Generated.volume_original Generated.periodic_original Generated.timestep_original m c =
      some (Function.update m (destination c) (some (cellValue s vol per dt c))) := by
  obtain ⟨hv, hp, ht⟩ := getter_connection m s vol per dt ready c.1
  have hpoint : c.1.val < M := lt_of_lt_of_le c.1.isLt owned
  have hcomp := c.2.isLt
  simp only [executeCell, hv, hp, ht, ready.2.1 c, ready.2.2.1 c,
    Option.bind_eq_bind, Option.some_bind, apply_source_ir, call_source_ir, ite_true, opReal,
    migrated_update_unchanged, original_ir]
  have hd : ∀ x : ℝ, address (stateLayout M 2) ⟨c.1.val, c.2.val, x⟩ currentCell = some (destination c) := by
    intro x
    simp [address, stateLayout, currentCell, Layout.view, indexValue, destination, offset, hpoint, hcomp, size]
    exact size
  have hs : ∀ x : ℝ, address (stateLayout M 2) ⟨c.1.val, c.2.val, x⟩ oldCell = some (1, offset c) := by
    intro x
    simp [address, stateLayout, oldCell, Layout.view, indexValue, offset, hpoint, hcomp, size]
    exact size
  rw [scalarUpdateIR_exec realOperations _ _ _ _ _ _ (hd _) (hs _) (ready.1 c)]
  congr 3
  dsimp [realOperations, cellValue]
  ring

theorem execute_cells_correct (cs : List (Cell N)) (m : Memory ℝ) (s : BoundaryState N)
    (vol per dt : Fin N → ℝ) (ready : InputsReady m s vol per dt)
    (owned : N ≤ M) (size : 8 * (M * 4) ≤ 2^64) :
    executeCells M Generated.volume_original Generated.periodic_original Generated.timestep_original cs m =
      some (writes (cellValue s vol per dt) cs m) := by
  induction cs generalizing m with
  | nil => rfl
  | cons c cs ih =>
    simp only [executeCells, execute_cell_correct m s vol per dt ready owned size c, Option.some_bind, writes]
    exact ih _ (inputs_after_store m s vol per dt ready c _)

theorem writes_value (value : Cell N → ℝ) (cs : List (Cell N)) (m : Memory ℝ) (c : Cell N) :
    writes value cs m (destination c) = if c ∈ cs then some (value c) else m (destination c) := by
  induction cs generalizing m with
  | nil => simp [writes]
  | cons a cs ih =>
    rw [writes, ih]
    by_cases hc : c ∈ cs
    · simp [hc]
    · by_cases ha : c = a
      · subst a; simp [hc]
      · have hd : destination c ≠ destination a := fun h => ha (destination_injective h)
        simp [hc, ha, Function.update_of_ne hd]

theorem writes_frame (value : Cell N → ℝ) (cs : List (Cell N)) (m : Memory ℝ) (a : Address)
    (h : ∀ c ∈ cs, a ≠ destination c) : writes value cs m a = m a := by
  induction cs generalizing m with
  | nil => rfl
  | cons c cs ih =>
    rw [writes, ih _ (fun d hd => h d (List.mem_cons_of_mem c hd))]
    exact Function.update_of_ne (h c (List.mem_cons_self)) _ _

/-- Independent whole-loop postcondition includes every component at every owned point,
plus all other memory (halo current, old, geometry, dt, residual and truncation). -/
def LoopContract (s : BoundaryState N) (vol dt : Fin N → ℝ) (before after : Memory ℝ) : Prop :=
  (∀ c : Cell N, after (destination c) = some (updateSpec vol dt s c.1 (channel c.2))) ∧
  (∀ a, (∀ c : Cell N, a ≠ destination c) → after a = before a)

theorem complete_update_loop (m : Memory ℝ) (s : BoundaryState N) (vol dt : Fin N → ℝ)
    (ready : InputsReady m s vol (fun _ => 0) dt)
    (owned : N ≤ M) (size : 8 * (M * 4) ≤ 2^64) :
    ∃ after, executeLoop (N := N) M Generated.volume_original Generated.periodic_original Generated.timestep_original
      Generated.points_original Generated.components_original m = some after ∧
      LoopContract s vol dt m after := by
  rw [original_loop_execution, execute_cells_correct _ m s vol (fun _ => 0) dt ready owned size]
  refine ⟨_, rfl, ?_, ?_⟩
  · intro c
    rw [writes_value, if_pos (visits_coverage c)]
    simp [cellValue, updateSpec]
  · intro a ha
    exact writes_frame _ _ _ _ (fun c _ => ha c)

/-- Boundary refinement composes with all update cells, without the old abstract finish
interface. Memory readiness refers explicitly to the computed boundary buffers. -/
theorem boundary_and_complete_update (g : IdealGas) (free : Primitive 2) (far : List (FarVisit N))
    (walls : List (WallVisit N)) (s : BoundaryState N) (m : Memory ℝ) (vol dt : Fin N → ℝ)
    (farReady : FarReady g free far)
    (ready : InputsReady m (boundaryImpl g free far walls s) vol (fun _ => 0) dt)
    (owned : N ≤ M) (size : 8 * (M * 4) ≤ 2^64) :
    ∃ after, executeLoop (N := N) M Generated.volume_original Generated.periodic_original Generated.timestep_original
      Generated.points_original Generated.components_original m = some after ∧
      LoopContract (boundarySpec g free far walls s) vol dt m after := by
  rw [boundary_refinement g free far walls s farReady] at ready
  exact complete_update_loop m _ vol dt ready owned size

/-- Reuses the pending candidate's proved assembly result without modifying it.
Flux values, edge visits, primitive/geometry data and initialization remain supplied. -/
theorem assembled_boundary_complete_update (g : IdealGas) (free : Primitive 2)
    (far : List (FarVisit N)) (walls : List (WallVisit N)) (s : BoundaryState N)
    (lanes : List (Lane N 2)) (m : Memory ℝ) (vol dt : Fin N → ℝ)
    (farReady : FarReady g free far)
    (ready : InputsReady m (boundaryImpl g free far walls
      {s with residual := scatterLoop SU2Euler.Generated.scatter_originalCode lanes s.residual})
      vol (fun _ => 0) dt)
    (owned : N ≤ M) (size : 8 * (M * 4) ≤ 2^64) :
    ∃ after, executeLoop (N := N) M Generated.volume_original Generated.periodic_original Generated.timestep_original
      Generated.points_original Generated.components_original m = some after ∧
      LoopContract (boundarySpec g free far walls
        {s with residual := s.residual + assemble (activeEvents lanes)}) vol dt m after := by
  rw [masked_assembly_correct] at ready
  exact boundary_and_complete_update g free far walls _ m vol dt farReady ready owned size

end SU2Boundary
