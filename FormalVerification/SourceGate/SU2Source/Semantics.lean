import SU2.Wall

namespace SU2Source

inductive Field where | current | old
  deriving DecidableEq, Repr
inductive Index where | point | component
  deriving DecidableEq, Repr
structure Access where
  field : Field
  row : Index
  col : Index
  deriving DecidableEq, Repr
inductive Expr where
  | argument
  | load (access : Access)
  | localValue (name : String)
  | add (left right : Expr)
  | sub (left right : Expr)
  deriving DecidableEq, Repr
inductive Statement where
  | bind (name : String) (value : Expr)
  | store (target : Access) (value : Expr)
  deriving DecidableEq, Repr
abbrev Program := List Statement

/-- Whole scalar cells; equal addresses model exact aliasing, not partial byte overlap. -/
abbrev Address := Nat × Nat
abbrev Memory (α : Type) := Address → Option α
structure MatrixView where
  block : Nat
  rows : Nat
  cols : Nat
  deriving DecidableEq, Repr
structure Layout where
  current : MatrixView
  old : MatrixView
  deriving DecidableEq, Repr
structure Inputs (α : Type) where
  point : Nat
  component : Nat
  increment : α

/-- Arithmetic is a parameter. Instantiating the carrier with bits does not specify IEEE-754. -/
structure Operations (α : Type) where
  add : α → α → α
  sub : α → α → α

abbrev Binary64Bits := BitVec 64
abbrev Binary64Operations := Operations Binary64Bits

def indexValue {α : Type} (args : Inputs α) : Index → Nat
  | .point => args.point
  | .component => args.component

def Layout.view (layout : Layout) : Field → MatrixView
  | .current => layout.current
  | .old => layout.old

/-- LP64, eight-byte scalar view. Invalid indices/byte extents have no modeled execution. -/
def address {α : Type} (layout : Layout) (args : Inputs α) (access : Access) : Option Address :=
  let view := layout.view access.field
  let row := indexValue args access.row
  let col := indexValue args access.col
  if row < view.rows ∧ col < view.cols ∧ 8 * (view.rows * view.cols) ≤ 2^64 then
    some (view.block, row * view.cols + col)
  else none

def evaluate {α : Type} (ops : Operations α) (layout : Layout) (args : Inputs α)
    (memory : Memory α) (locals : String → Option α) : Expr → Option α
  | .argument => some args.increment
  | .load access => (address layout args access).bind memory
  | .localValue name => locals name
  | .add a b => do return ops.add (← evaluate ops layout args memory locals a)
                                  (← evaluate ops layout args memory locals b)
  | .sub a b => do return ops.sub (← evaluate ops layout args memory locals a)
                                  (← evaluate ops layout args memory locals b)

def execute {α : Type} (ops : Operations α) (layout : Layout) (args : Inputs α)
    (program : Program) (memory : Memory α) (locals : String → Option α := fun _ => none) :
    Option (Memory α) :=
  match program with
  | [] => some memory
  | .bind name value :: rest => do
      let x ← evaluate ops layout args memory locals value
      execute ops layout args rest memory (Function.update locals name (some x))
  | .store target value :: rest => do
      let x ← evaluate ops layout args memory locals value
      let dest ← address layout args target
      execute ops layout args rest (Function.update memory dest (some x)) locals

/-- Independent single-cell contract, using the pre-state read and a frame condition. -/
def UpdateContract {α : Type} (add : α → α → α) (before after : Memory α)
    (dest source : Address) (oldValue increment : α) : Prop :=
  before source = some oldValue ∧ after dest = some (add oldValue increment) ∧
  ∀ other, other ≠ dest → after other = before other

def currentCell : Access := ⟨.current, .point, .component⟩
def oldCell : Access := ⟨.old, .point, .component⟩

/-- Reference IR is a proof aid; generated source programs are parsed independently. -/
def scalarUpdateIR : Program := [.store currentCell (.add (.load oldCell) .argument)]

theorem scalarUpdateIR_exec {α : Type} (ops : Operations α) (layout : Layout)
    (args : Inputs α) (memory : Memory α) (dest source : Address) (x : α)
    (hd : address layout args currentCell = some dest)
    (hs : address layout args oldCell = some source) (hx : memory source = some x) :
    execute ops layout args scalarUpdateIR memory =
      some (Function.update memory dest (some (ops.add x args.increment))) := by
  simp [scalarUpdateIR, execute, evaluate, hd, hs, hx]

theorem store_contract {α : Type} (ops : Operations α) (memory : Memory α)
    (dest source : Address) (x delta : α) (hx : memory source = some x) :
    UpdateContract ops.add memory (Function.update memory dest (some (ops.add x delta)))
      dest source x delta := by
  refine ⟨hx, ?_, ?_⟩
  · simp
  · intro other h
    simp [Function.update_of_ne h]

theorem scalarUpdateIR_correct {α : Type} (ops : Operations α) (layout : Layout)
    (args : Inputs α) (memory : Memory α) (dest source : Address) (x : α)
    (hd : address layout args currentCell = some dest)
    (hs : address layout args oldCell = some source) (hx : memory source = some x) :
    ∃ after, execute ops layout args scalarUpdateIR memory = some after ∧
      UpdateContract ops.add memory after dest source x args.increment := by
  exact ⟨_, scalarUpdateIR_exec ops layout args memory dest source x hd hs hx,
    store_contract ops memory dest source x args.increment hx⟩

theorem row_major_bound (rows cols row col : Nat) (hr : row < rows) (hc : col < cols) :
    row * cols + col < rows * cols := by
  calc
    row * cols + col < row * cols + cols := Nat.add_lt_add_left hc _
    _ = (row + 1) * cols := by simp [Nat.add_mul]
    _ ≤ rows * cols := Nat.mul_le_mul_right cols (Nat.succ_le_of_lt hr)

theorem byte_offset_bound (rows cols row col : Nat) (hr : row < rows) (hc : col < cols)
    (hs : 8 * (rows * cols) ≤ 2^64) : 8 * (row * cols + col) + 7 < 2^64 := by
  have h := row_major_bound rows cols row col hr hc
  omega

noncomputable def realOperations : Operations ℝ := ⟨(· + ·), (· - ·)⟩

/-- This bridge uses real arithmetic; the machine increment is supplied by the caller. -/
theorem real_component_bridge {d : Nat} (U delta : SU2.State d) (alpha : ℝ)
    (channel : SU2.Channel d) :
    realOperations.add (U channel) (alpha * delta channel) =
      SU2.conservativeUpdate U delta alpha channel := rfl

/-- A diagnostic definition only: finite decoding and bounds on this deviation remain obligations. -/
noncomputable def finiteAdditionDeviation {α : Type} (ops : Operations α)
    (decode : α → Option ℝ) (x delta : α) : Option ℝ := do
  let a ← decode x
  let b ← decode delta
  let result ← decode (ops.add x delta)
  return result - (a + b)

end SU2Source
