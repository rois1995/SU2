import SU2Preprocessing.Controls

set_option maxRecDepth 100000
set_option maxHeartbeats 4000000
namespace SU2Preprocessing
open SU2Boundary SU2BoundaryMemory SU2Initialization

structure PrimitivePlan where
  incomplete : Bool
  deriving DecidableEq, Repr

/-- Complete finite grammar for this caller, with extra-token failure. Serial AD/macros
and the nSecondaryVar=0 call are explicitly qualified external interfaces. -/
def parsePrimitiveLoop (source : String) : Option PrimitivePlan := do
  let ts ← lex source
  let ts ← expect "unsigned long CEulerSolver::SetPrimitive_Variables(CSolver **solver_container, const CConfig *config) { SU2_ZONE_SCOPED unsigned long nonPhysicalPoints = 0; AD::StartNoSharedReading(); SU2_OMP_FOR_STAT(omp_chunk_size) for (unsigned long iPoint = 0;" ts
  let (incomplete,ts) ← match expect "iPoint < nPoint;" ts with
    | some rest => some (false,rest)
    | none => do let rest ← expect "iPoint + 1 < nPoint;" ts; pure (true,rest)
  let ts ← match expect "iPoint ++)" ts with
    | some rest => some rest
    | none => expect "++iPoint)" ts
  let [] ← expect "{ bool physical = nodes->SetPrimVar(iPoint, GetFluidModel()); nodes->SetSecondaryVar(iPoint, GetFluidModel()); if (!physical) nonPhysicalPoints++; } END_SU2_OMP_FOR AD::EndNoSharedReading(); return nonPhysicalPoints; }" ts | none
  return ⟨incomplete⟩

def planPoints (M : Nat) (plan : PrimitivePlan) : List (Fin M) :=
  if plan.incomplete then (points M).filter (fun p => p.val + 1 < M) else points M

noncomputable def executePrimitivePlan {M : Nat} (plan : PrimitivePlan) (s : Machine) :=
  executePoints (planPoints M plan) s 0

/-- Unsupported syntax is rejected, rather than ignored as an identity operation. -/
theorem unknown_caller_rejected : parsePrimitiveLoop "unsigned long f(){ arbitrary(); }" = none := by decide

end SU2Preprocessing
