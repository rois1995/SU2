import SU2Euler.Generated
import SU2.Assembly

set_option maxRecDepth 100000
set_option maxHeartbeats 2000000

noncomputable section
open scoped BigOperators
namespace SU2Euler
open SU2 SU2Source Generated
variable {N d : Nat}

/-- Independent specification: local pseudo-time, positive physical volumes, and an
explicit interface for intervening boundary/residual processing. The interface is not
claimed to verify the production boundary routines. -/
def stepSpec (volumes : Fin N → ControlVolume d) (dt : Fin N → ℝ)
    (old initial trunc : Residual N d) (events : List (Event N d))
    (finish : Residual N d → Residual N d) : Residual N d :=
  fun i c => old i c - (dt i / (volumes i).measure) *
    (finish (initial + assemble events) i c + trunc i c)

/-- One SIMD lane. Boolean masks are justified by the inspected caller's assignment
from its in-range comparison; non-Boolean mask inputs are outside this specialization. -/
abbrev Lane (N d : Nat) := Bool × Event N d

def activeEvents (lanes : List (Lane N d)) : List (Event N d) :=
  lanes.filterMap (fun lane => if lane.1 then some lane.2 else none)

def opReal : Op → ℝ → ℝ → ℝ
  | .add => (· + ·)
  | .sub => (· - ·)

/-- Serial interpretation of the parsed scatter. UnpackBlock's multiplication is modeled
explicitly. Its C++ interpretation is inspected and tested, not certified by this parser.
Distinct endpoints and valid component indices are carried by the mathematical types. -/
def scatterLane (code : ScatterCode) (R : Residual N d) (lane : Lane N d) : Residual N d :=
  if (if code.skipZero then !lane.1 else lane.1) then R else
  let F : State d := fun c => (if lane.1 then 1 else 0) * lane.2.2 c
  let R₁ := Function.update R lane.2.1.left
    (fun c => opReal code.left (R lane.2.1.left c) (F c))
  Function.update R₁ lane.2.1.right
    (fun c => opReal code.right (R₁ lane.2.1.right c) (F c))

def scatterLoop (code : ScatterCode) (lanes : List (Lane N d)) (R : Residual N d) : Residual N d :=
  lanes.foldl (scatterLane code) R

def runStep (scatterCode : ScatterCode) (applyCode : Op) (negative : Bool)
    (volumes : Fin N → ControlVolume d) (dt : Fin N → ℝ)
    (old initial trunc : Residual N d) (lanes : List (Lane N d))
    (finish : Residual N d → Residual N d) : Residual N d :=
  let R := finish (scatterLoop scatterCode lanes initial)
  fun i c =>
    let res := opReal applyCode (R i c) (trunc i c)
    let delta := dt i / ((volumes i).measure + 0)
    old i c + (if negative then -res else res) * delta

theorem migrated_update_unchanged : update_originalCode = SU2Source.Generated.originalProgram := by decide

theorem migrated_refactor_unchanged : update_refactoredCode = SU2Source.Generated.refactoredProgram := by decide

theorem scatter_source_ir : scatter_originalCode = ⟨true, .add, .sub⟩ := by decide

theorem scatter_refactor_ir : scatter_refactoredCode = scatter_originalCode := by decide

theorem apply_source_ir : apply_originalCode = .add := by decide

theorem call_source_ir : call_originalCode = true := by decide

theorem scale_source_ir : scale_originalCode = () := by decide

/-- Freshly extracted upstream source inherits the accepted contract without changing it. -/
theorem migrated_update_correct {α : Type} (ops : Operations α) (layout : Layout)
    (args : Inputs α) (memory : Memory α) (dest source : Address) (x : α)
    (hd : address layout args currentCell = some dest)
    (hs : address layout args oldCell = some source) (hx : memory source = some x) :
    ∃ after, execute ops layout args update_originalCode memory = some after ∧
      UpdateContract ops.add memory after dest source x args.increment := by
  rw [migrated_update_unchanged]
  exact source_update_correct ops layout args memory dest source x hd hs hx

theorem migrated_refactor_equivalent {α : Type} (ops : Operations α) (layout : Layout)
    (args : Inputs α) (memory : Memory α) :
    execute ops layout args update_refactoredCode memory =
      execute ops layout args update_originalCode memory := by
  rw [migrated_update_unchanged, migrated_refactor_unchanged]
  exact refactor_equivalent ops layout args memory

theorem scatter_lane_correct (R : Residual N d) (lane : Lane N d) :
    scatterLane scatter_originalCode R lane =
      if lane.1 then scatter lane.2.1 lane.2.2 R else R := by
  rcases lane with ⟨mask, e, F⟩
  cases mask <;> simp [scatterLane, scatter_source_ir, opReal, scatter]
  rfl

/-- Disabled padding lanes contribute nothing; active lanes preserve source order. -/
theorem masked_assembly_correct (lanes : List (Lane N d)) (R : Residual N d) :
    scatterLoop scatter_originalCode lanes R = R + assemble (activeEvents lanes) := by
  have loop : ∀ (xs : List (Lane N d)) (acc : Residual N d),
      scatterLoop scatter_originalCode xs acc = assembleLoop (activeEvents xs) acc := by
    intro xs
    induction xs with
    | nil => intro acc; rfl
    | cons lane xs ih =>
      intro acc
      simp only [scatterLoop, List.foldl_cons]
      change scatterLoop scatter_originalCode xs (scatterLane scatter_originalCode acc lane) = _
      rw [ih, scatter_lane_correct]
      cases hm : lane.1 <;> simp [activeEvents, hm, assembleLoop]
  rw [loop, accumulation_correct]

/-- The main multi-operation refinement: masked source assembly, caller arithmetic,
and old-state addition implement the independent local-step specification.
`finish` is an open interface for the actual intervening boundary stage. -/
theorem composed_step_correct (volumes : Fin N → ControlVolume d) (dt : Fin N → ℝ)
    (old initial trunc : Residual N d) (lanes : List (Lane N d))
    (finish : Residual N d → Residual N d) :
    runStep scatter_originalCode apply_originalCode call_originalCode
      volumes dt old initial trunc lanes finish =
    stepSpec volumes dt old initial trunc (activeEvents lanes) finish := by
  funext i c
  simp only [runStep, masked_assembly_correct, apply_source_ir, call_source_ir,
    opReal, add_zero, ite_true, stepSpec]
  ring

/-- Local pseudo-time balance keeps the time step inside the sum. -/
theorem local_balance (volumes : Fin N → ControlVolume d) (dt : Fin N → ℝ)
    (old initial trunc : Residual N d) (events : List (Event N d))
    (finish : Residual N d → Residual N d) (c : Channel d) :
    (∑ i, (volumes i).measure * stepSpec volumes dt old initial trunc events finish i c) =
      (∑ i, (volumes i).measure * old i c) -
      ∑ i, dt i * (finish (initial + assemble events) i c + trunc i c) := by
  have point : ∀ i, (volumes i).measure * stepSpec volumes dt old initial trunc events finish i c =
      (volumes i).measure * old i c -
      dt i * (finish (initial + assemble events) i c + trunc i c) := by
    intro i
    simp only [stepSpec]
    field_simp [ne_of_gt (volumes i).positive]
    ring
  simp only [point, Finset.sum_sub_distrib]

/-- Conservation specialization requires uniform time step and no boundary/truncation
or initial residual contribution. It is not asserted for arbitrary local pseudo-time. -/
theorem uniform_interior_conservation (volumes : Fin N → ControlVolume d) (dt : ℝ)
    (old : Residual N d) (lanes : List (Lane N d)) (c : Channel d) :
    (∑ i, (volumes i).measure *
      runStep scatter_originalCode apply_originalCode call_originalCode
        volumes (fun _ => dt) old 0 0 lanes id i c) =
      ∑ i, (volumes i).measure * old i c := by
  rw [composed_step_correct]
  have eq : stepSpec volumes (fun _ => dt) old 0 0 (activeEvents lanes) id =
      explicitUpdate volumes dt old (assemble (activeEvents lanes)) := by
    funext i c
    simp [stepSpec, explicitUpdate]
  rw [eq]
  exact assembled_update_conserves volumes dt old (activeEvents lanes) c

/-- CVariable's actual ordering: density, momentum components, energy. -/
def componentIndex : Channel d → Nat
  | .mass => 0
  | .momentum k => k.val + 1
  | .energy => d + 1

theorem component_bound (c : Channel d) : componentIndex c < d + 2 := by
  cases c with
  | mass => simp [componentIndex]
  | momentum k => simp only [componentIndex]; omega
  | energy => simp [componentIndex]

def stateLayout (N d : Nat) : Layout := ⟨⟨0, N, d + 2⟩, ⟨1, N, d + 2⟩⟩

theorem caller_addresses (i : Fin N) (c : Channel d) (increment : ℝ)
    (size : 8 * (N * (d + 2)) ≤ 2^64) :
    address (stateLayout N d) ⟨i.val, componentIndex c, increment⟩ currentCell =
      some (0, i.val * (d + 2) + componentIndex c) ∧
    address (stateLayout N d) ⟨i.val, componentIndex c, increment⟩ oldCell =
      some (1, i.val * (d + 2) + componentIndex c) := by
  simp [address, stateLayout, currentCell, oldCell, Layout.view, indexValue,
    i.isLt, component_bound c, size]
  exact size

/-- Stronger source-memory result for an arbitrary selected point/component. This proves
the callee's numerical index/extent preconditions from caller bounds. Allocation validity,
old-state initialization and correspondence of `finish` to C++ remain explicit obligations. -/
theorem composed_source_cell (volumes : Fin N → ControlVolume d) (dt : Fin N → ℝ)
    (old initial trunc : Residual N d) (lanes : List (Lane N d))
    (finish : Residual N d → Residual N d) (i : Fin N) (c : Channel d)
    (memory : Memory ℝ) (size : 8 * (N * (d + 2)) ≤ 2^64)
    (initialized : memory (1, i.val * (d + 2) + componentIndex c) = some (old i c)) :
    let R := finish (scatterLoop scatter_originalCode lanes initial)
    let increment := -(opReal apply_originalCode (R i c) (trunc i c)) *
      (dt i / ((volumes i).measure + 0))
    ∃ after, execute realOperations (stateLayout N d) ⟨i.val, componentIndex c, increment⟩
        update_originalCode memory = some after ∧
      after (0, i.val * (d + 2) + componentIndex c) =
        some (stepSpec volumes dt old initial trunc (activeEvents lanes) finish i c) ∧
      ∀ other, other ≠ (0, i.val * (d + 2) + componentIndex c) → after other = memory other := by
  dsimp only
  obtain ⟨hd, hs⟩ := caller_addresses i c
    (-(opReal apply_originalCode (finish (scatterLoop scatter_originalCode lanes initial) i c)
      (trunc i c)) * (dt i / ((volumes i).measure + 0))) size
  obtain ⟨after, he, _, hv, hf⟩ := migrated_update_correct realOperations
    (stateLayout N d) _ memory _ _ (old i c) hd hs initialized
  refine ⟨after, he, ?_, hf⟩
  rw [hv]
  congr 1
  simp only [realOperations, apply_source_ir, opReal, masked_assembly_correct, add_zero, stepSpec]
  ring

end SU2Euler
