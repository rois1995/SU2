import SU2Initialization.Controls

namespace SU2Initialization
open SU2Boundary

structure CopyPlan where
  reverse : Bool
  incomplete : Bool
  deriving DecidableEq, Repr

/-- Finite complete grammar, including semantic controls. Extra tokens are rejected. -/
def parseSnapshot (source : String) : Option Nat := do
  let ts ← lex source
  let ts ← expect "void CVariable::Set_OldSolution() { assert(Solution_Old.size() == Solution.size()); parallelCopy(Solution.size(), Solution.data()," ts
  let (dst,ts) ← match expect "Solution_Old.data()" ts with
    | some rest => some (1,rest)
    | none => do let rest ← expect "Solution.data()" ts; pure (0,rest)
  let [] ← expect "); }" ts | none
  return dst

def parseCopyLoop (source : String) : Option CopyPlan := do
  let ts ← lex source
  let ts ← expect "template <class T, class U> void parallelCopy(size_t size, const T* src, U* dst) { SU2_OMP_FOR_STAT(2048) for (size_t i =" ts
  let (plan,ts) ← match expect "0; i < size; ++i)" ts with
    | some rest => some (⟨false,false⟩,rest)
    | none => match expect "0; i + 1 < size; ++i)" ts with
      | some rest => some (⟨false,true⟩,rest)
      | none => do let rest ← expect "size; i-- > 0;)" ts; pure (⟨true,false⟩,rest)
  let [] ← expect "dst[i] = src[i]; END_SU2_OMP_FOR }" ts | none
  return plan

noncomputable def executeCopyPlan {N : Nat} (dst : Nat) (plan : CopyPlan) (m : SU2BoundaryMemory.Mem) :=
  let cs := visits N
  let cs := if plan.incomplete then cs.filter (fun c => offset c + 1 < N * 4) else cs
  let cs := if plan.reverse then cs.reverse else cs
  copyCells 0 dst cs m

end SU2Initialization
