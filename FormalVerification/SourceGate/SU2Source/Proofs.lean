import SU2Source.Generated

namespace SU2Source
open Generated

set_option maxRecDepth 100000
set_option maxHeartbeats 800000

theorem original_ir : originalProgram = scalarUpdateIR := by decide

theorem refactored_ir : refactoredProgram =
    [.bind "previous" (.load oldCell),
     .store currentCell (.add (.localValue "previous") .argument)] := by decide

/-- Covers any pure scalar arithmetic and exact cell aliases. Preconditions are only valid
addresses and an initialized source cell, not the desired postcondition. -/
theorem source_update_correct {α : Type} (ops : Operations α) (layout : Layout)
    (args : Inputs α) (memory : Memory α) (dest source : Address) (x : α)
    (hd : address layout args currentCell = some dest)
    (hs : address layout args oldCell = some source) (hx : memory source = some x) :
    ∃ after, execute ops layout args originalProgram memory = some after ∧
      UpdateContract ops.add memory after dest source x args.increment := by
  rw [original_ir]
  exact scalarUpdateIR_correct ops layout args memory dest source x hd hs hx

/-- Includes modeled failure paths; no successful-address assumption is needed. -/
theorem refactor_equivalent {α : Type} (ops : Operations α) (layout : Layout)
    (args : Inputs α) (memory : Memory α) :
    execute ops layout args refactoredProgram memory =
      execute ops layout args originalProgram memory := by
  rw [original_ir, refactored_ir]
  cases hs : address layout args oldCell with
  | none => simp [execute, scalarUpdateIR, evaluate, hs]
  | some source =>
    cases hx : memory source <;> simp [execute, scalarUpdateIR, evaluate, hs, hx]

theorem refactored_update_correct {α : Type} (ops : Operations α) (layout : Layout)
    (args : Inputs α) (memory : Memory α) (dest source : Address) (x : α)
    (hd : address layout args currentCell = some dest)
    (hs : address layout args oldCell = some source) (hx : memory source = some x) :
    ∃ after, execute ops layout args refactoredProgram memory = some after ∧
      UpdateContract ops.add memory after dest source x args.increment := by
  simp only [refactor_equivalent]
  exact source_update_correct ops layout args memory dest source x hd hs hx

/-- Composition with the frozen mathematical update, under exact-real interpretation. -/
theorem source_real_update {d : Nat} (U delta : SU2.State d) (alpha : ℝ)
    (channel : SU2.Channel d) (layout : Layout) (point component : Nat)
    (memory : Memory ℝ) (dest source : Address)
    (hd : address layout ⟨point, component, alpha * delta channel⟩ currentCell = some dest)
    (hs : address layout ⟨point, component, alpha * delta channel⟩ oldCell = some source)
    (hx : memory source = some (U channel)) :
    ∃ after, execute realOperations layout ⟨point, component, alpha * delta channel⟩
        originalProgram memory = some after ∧
      after dest = some (SU2.conservativeUpdate U delta alpha channel) ∧
      ∀ other, other ≠ dest → after other = memory other := by
  obtain ⟨after, he, _, hv, hf⟩ := source_update_correct realOperations layout
    ⟨point, component, alpha * delta channel⟩ memory dest source (U channel) hd hs hx
  exact ⟨after, he, hv, hf⟩

/-- Small exact-integer witnesses also use values exactly representable in binary64. -/
def witnessLayout : Layout := ⟨⟨0, 1, 1⟩, ⟨1, 1, 1⟩⟩
def witnessArgs : Inputs Int := ⟨0, 0, 3⟩
def witnessMemory : Memory Int := fun a =>
  if a = (0, 0) then some 11 else if a = (1, 0) then some 2 else none
def integerOperations : Operations Int := ⟨(· + ·), (· - ·)⟩

def WitnessContract (program : Program) : Prop :=
  ∃ after, execute integerOperations witnessLayout witnessArgs program witnessMemory = some after ∧
    UpdateContract integerOperations.add witnessMemory after (0, 0) (1, 0) 2 3

theorem original_witness : WitnessContract originalProgram := by
  apply source_update_correct <;> rfl

theorem subtract_ir : subtractProgram =
    [.store currentCell (.sub (.load oldCell) .argument)] := by decide

theorem currentRead_ir : currentReadProgram =
    [.store currentCell (.add (.load currentCell) .argument)] := by decide

theorem wrongTarget_ir : wrongTargetProgram =
    [.store oldCell (.add (.load oldCell) .argument)] := by decide

theorem subtract_rejected : ¬ WitnessContract subtractProgram := by
  simp [WitnessContract, subtract_ir, execute, evaluate, address, Layout.view,
    currentCell, oldCell, indexValue, witnessLayout, witnessArgs, witnessMemory,
    integerOperations, UpdateContract, Function.update]

theorem currentRead_rejected : ¬ WitnessContract currentReadProgram := by
  simp [WitnessContract, currentRead_ir, execute, evaluate, address, Layout.view,
    currentCell, oldCell, indexValue, witnessLayout, witnessArgs, witnessMemory,
    integerOperations, UpdateContract, Function.update]

theorem wrongTarget_rejected : ¬ WitnessContract wrongTargetProgram := by
  simp [WitnessContract, wrongTarget_ir, execute, evaluate, address, Layout.view,
    currentCell, oldCell, indexValue, witnessLayout, witnessArgs, witnessMemory,
    integerOperations, UpdateContract, Function.update]

/-- Unsupported control flow is rejected instead of discarded. -/
theorem reject_branch : parse
    "inline void AddSolution(unsigned long p, unsigned long v, su2double x) { if (x) { Solution(p,v) = x; } }" = none := by decide

theorem reject_trailing : parse (originalSource ++ " Solution(iPoint,iVar) = solution;") = none := by decide

theorem reject_macro : parse
    "inline void AddSolution(unsigned long p, unsigned long v, su2double x) { UPDATE(p,v,x); }" = none := by decide

theorem reject_shadow : parse
    "inline void AddSolution(unsigned long p, unsigned long v, su2double x) { const su2double x = x; }" = none := by decide

theorem reject_duplicate_parameter : parse
    "inline void AddSolution(unsigned long p, unsigned long p, su2double x) { Solution(p,p) = x; }" = none := by decide

theorem reject_keyword_parameter : parse
    "inline void AddSolution(unsigned long if, unsigned long v, su2double x) { Solution(if,v) = x; }" = none := by decide

end SU2Source
