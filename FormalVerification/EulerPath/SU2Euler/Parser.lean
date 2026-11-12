import SU2Source

namespace SU2Euler

/-- Closed token language for the selected serial SIMD scatter and Euler caller fragments.
Comment handling is part of the kernel computation; directives and unknown characters fail. -/
def lexChars : List Char → String → List String → Option (List String)
  | [], token, out => some (SU2Source.flushToken token out).reverse
  | '/' :: '*' :: rest, token, out => comment rest (SU2Source.flushToken token out)
  | c :: rest, token, out =>
    if c == ' ' || c == '\n' || c == '\r' || c == '\t' then
      lexChars rest "" (SU2Source.flushToken token out)
    else if SU2Source.identifierChar c then lexChars rest (token.push c) out
    else if "(){}[]=+-;,<>:*&!/".toList.contains c then
      lexChars rest "" (String.singleton c :: SU2Source.flushToken token out)
    else none
where
  comment : List Char → List String → Option (List String)
    | '*' :: '/' :: rest, out => lexChars rest "" out
    | _ :: rest, out => comment rest out
    | [], _ => none

def lex (s : String) : Option (List String) := lexChars s.toList "" []

def expect (s : String) (tokens : List String) : Option (List String) := do
  let expected ← lex s
  if tokens.take expected.length == expected then some (tokens.drop expected.length) else none

inductive Op where | add | sub
  deriving DecidableEq, Repr

def parseOp : List String → Option (Op × List String)
  | "+" :: rest => some (.add, rest)
  | "-" :: rest => some (.sub, rest)
  | _ => none

structure ScatterCode where
  skipZero : Bool
  left : Op
  right : Op
  deriving DecidableEq, Repr

/-- Consumes the complete template and method. The binder array binder may be renamed.
The arithmetic operators and mask guard are parsed, not replaced by a default algorithm. -/
def parseScatter (s : String) : Option ScatterCode := do
  let ts ← lex s
  let ts ← expect "template <size_t N, class T, class VecTypeSIMD, class F = ScalarType> FORCEINLINE void UpdateBlocks(simd::Array<T,N> iPoint, simd::Array<T,N> jPoint, const VecTypeSIMD& vector, simd::Array<F,N> mask = 1) { constexpr size_t nVar = VecTypeSIMD::StaticSize; assert(nVar == this->nVar); ScalarType" ts
  let binder :: ts := ts | none
  if !(binder == "vec" || binder == "unpacked") then none else do
    let ts ← expect (binder ++ "[N][nVar]; UnpackBlock(vector, mask, " ++ binder ++ "); for(size_t k = 0; k < N; ++k) { if(mask[k]") (binder :: ts)
    let guard :: "=" :: "0" :: ts := ts | none
    let skipZero ← if guard == "=" then some true else if guard == "!" then some false else none
    let ts ← expect ") continue; SU2_OMP_SIMD for(size_t i = 0; i < nVar; ++i) { vec_val[iPoint[k]*nVar+i]" ts
    let (left, ts) ← parseOp ts
    let ts ← expect ("= " ++ binder ++ "[k][i]; vec_val[jPoint[k]*nVar+i]") ts
    let (right, ts) ← parseOp ts
    let [] ← expect ("= " ++ binder ++ "[k][i]; } } }") ts | none
    return ⟨skipZero, left, right⟩

/-- Full Identity::apply method, including its return expression. -/
def parseApply (s : String) : Option Op := do
  let ts ← lex s
  let ts ← expect "FORCEINLINE su2double apply(unsigned short iVar, const su2double* res, const su2double* resTrunc) { return res[iVar]" ts
  let (op, ts) ← parseOp ts
  let [] ← expect "resTrunc[iVar]; }" ts | none
  return op

/-- Complete selected switch arm; true means unary negation before multiplication. -/
def parseCall (s : String) : Option Bool := do
  let ts ← lex s
  let ts ← expect "case EULER_EXPLICIT: nodes->AddSolution(iPoint,iVar," ts
  let (negative, ts) ← match ts with
    | "-" :: rest => some (true, rest)
    | "+" :: rest => some (false, rest)
    | _ => none
  let [] ← expect "Res*Delta); break;" ts | none
  return negative

/-- The two scale declarations are checked in full. Getter and division semantics remain
explicit interpretation obligations, as do C++ types, macros, aliasing and compilation. -/
def parseScale (s : String) : Option Unit := do
  let ts ← lex s
  let [] ← expect "su2double Vol = geometry->nodes->GetVolume(iPoint) + geometry->nodes->GetPeriodicVolume(iPoint); su2double Delta = nodes->GetDelta_Time(iPoint) / Vol;" ts | none
  return ()

end SU2Euler
