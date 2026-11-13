import SU2Interior.Robustness

namespace SU2Interior
open SU2 SU2Source SU2Boundary SU2BoundaryMemory SU2Euler

/-- Closed grammar for the complete selected timestep store statement. This is a
leaf source gate, not a parser for the entire timestep routine. -/
def parseTimeStore (s : String) : Option Bool := do
  let ts ← SU2Boundary.lex s
  let ts ← SU2Boundary.expect "nodes->SetDelta_Time(iPoint," ts
  let (capped,ts) ← match SU2Boundary.expect "min(Local_Delta_Time, config->GetMax_DeltaTime())" ts with
    | some rest => some (true,rest)
    | none => match SU2Boundary.expect "std::min(Local_Delta_Time, config->GetMax_DeltaTime())" ts with
      | some rest => some (true,rest)
      | none => do let rest ← SU2Boundary.expect "Local_Delta_Time" ts; pure (false,rest)
  let [] ← SU2Boundary.expect ");" ts | none
  return capped

noncomputable def sourceTimeStore {M : Nat} (capped : Bool) (p : Fin M) (raw cap : ℝ) (m : Mem) : Mem :=
  put m (4,p.val) (if capped then min raw cap else raw)

/-- Full existing SIMD scatter grammar is reused, preserving its accepted controls. -/
noncomputable def sourceScatterComponents {M : Nat} (code : ScatterCode) (p q : Fin M) (f : Flux4) :
    List (Fin 4) → Mem → Option Mem
  | [], m => some m
  | c :: cs, m => do
    let ri ← m (flowAddr 5 p c)
    let m := put m (flowAddr 5 p c) (opReal code.left ri (f c))
    let rj ← m (flowAddr 5 q c)
    let m := put m (flowAddr 5 q c) (opReal code.right rj (f c))
    sourceScatterComponents code p q f cs m

/-- Component semantics are connected for active lanes; the admitted source code's
mask semantics are separately retained by the packet loop and inherited scatter gate. -/
theorem source_scatter_components_correct {M : Nat} (p q : Fin M) (f : Flux4)
    (cs : List (Fin 4)) (m : Mem) :
    sourceScatterComponents ⟨true,.add,.sub⟩ p q f cs m = executeScatter p q f cs m := by
  induction cs generalizing m with
  | nil => rfl
  | cons c cs ih => simp only [sourceScatterComponents,executeScatter,opReal]; simp_rw [ih]

theorem time_unknown_syntax_rejected : parseTimeStore "nodes->SetDelta_Time(iPoint, custom());"=none := by decide

theorem time_extra_statement_rejected :
    parseTimeStore "nodes->SetDelta_Time(iPoint, Local_Delta_Time); erased();"=none := by decide

end SU2Interior
