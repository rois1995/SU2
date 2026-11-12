import SU2Euler.Parser

namespace SU2Boundary
open SU2Euler

/-- Decimal punctuation is admitted only where the closed grammar expects it. Unknown
characters, extra statements and unconsumed tokens fail. No production-body skipping. -/
def lexChars : List Char → String → List String → Option (List String)
  | [], token, out => some (SU2Source.flushToken token out).reverse
  | '/' :: '*' :: rest, token, out => comment rest (SU2Source.flushToken token out)
  | c :: rest, token, out =>
    if c == ' ' || c == '\n' || c == '\r' || c == '\t' then
      lexChars rest "" (SU2Source.flushToken token out)
    else if SU2Source.identifierChar c then lexChars rest (token.push c) out
    else if "(){}[]=+-;,<>:*&!/.".toList.contains c then
      lexChars rest "" (String.singleton c :: SU2Source.flushToken token out)
    else none
where
  comment : List Char → List String → Option (List String)
    | '*' :: '/' :: rest, out => lexChars rest "" out
    | _ :: rest, out => comment rest out
    | [], _ => none

def lex (s : String) : Option (List String) := lexChars s.toList "" []
def expect (s : String) (ts : List String) : Option (List String) := do
  let xs ← lex s
  if ts.take xs.length == xs then some (ts.drop xs.length) else none

inductive WallBuffer where | old | residual | truncation
  deriving DecidableEq, Repr

/-- Complete selected reduction and correction loops. The old-buffer case also parses
its subtraction of grid velocity; setting this to zero requires the stationary branch. -/
def parseProjection (buffer : WallBuffer) (s : String) : Option Op := do
  let ts ← lex s
  let (binder, target, init, term) := match buffer with
    | .old => ("vp", "solutionOld[iVel + iDim]", "su2double vp = 0.0;",
        "(solutionOld[iVel + iDim] - gridVel[iDim])")
    | .residual => ("normalRes", "LinSysRes(iPoint, iVel + iDim)", "su2double normalRes = 0.0;",
        "LinSysRes(iPoint, iVel + iDim)")
    | .truncation => ("normalRes", "Res_TruncError[iVel + iDim]",
        "normalRes = 0.0; su2double* Res_TruncError = nodes->GetResTruncError(iPoint);",
        "Res_TruncError[iVel + iDim]")
  let ts ← expect (init ++ " for (auto iDim = 0u; iDim < nDim; iDim++) { " ++ binder ++
    " += " ++ term ++ " * UnitNormal[iDim]; } for (auto iDim = 0u; iDim < nDim; iDim++) { " ++ target) ts
  let (op, ts) ← parseOp ts
  let [] ← expect ("= " ++ binder ++ " * UnitNormal[iDim]; }") ts | none
  return op

/-- Complete boundary residual accumulation call; both operations have explicit semantics. -/
def parseBoundaryAdd (s : String) : Option Op := do
  let ts ← lex s
  let ts ← expect "LinSysRes." ts
  let (op, ts) ← match ts with
    | "AddBlock" :: rest => some (.add, rest)
    | "SubtractBlock" :: rest => some (.sub, rest)
    | _ => none
  let [] ← expect "(iPoint, residual);" ts | none
  return op

/-- Selected loop bounds. dropLast is semantic subtraction from the runtime bound. -/
structure LoopCode where
  dropLast : Bool
  deriving DecidableEq, Repr

def parseLoop (point : Bool) (s : String) : Option LoopCode := do
  let ts ← lex s
  let (ty, var, bound) := if point then ("unsigned long", "iPoint", "nPointDomain")
    else ("unsigned short", "iVar", "nVar")
  let ts ← expect ("for (" ++ ty ++ " " ++ var ++ " = 0; " ++ var ++ " < " ++ bound) ts
  let (dropLast, ts) := match ts with
    | "-" :: "1" :: rest => (true, rest)
    | rest => (false, rest)
  let ts ← expect ";" ts
  let ts ← match expect (var ++ "++") ts with
    | some rest => some rest
    | none => expect ("++" ++ var) ts
  let [] ← expect ") {" ts | none
  return ⟨dropLast⟩

inductive Getter where | volume | periodic | timestep | localCFL
  deriving DecidableEq, Repr

def getterBlock : Getter → Nat
  | .volume => 2 | .periodic => 3 | .timestep => 4 | .localCFL => 7

/-- Complete getter method, with both const and mutable volume overloads supported.
The parsed storage member controls the load; it is not inferred from the method name. -/
def parseGetter (name : String) (s : String) : Option Getter := do
  let ts ← lex s
  let ts ← expect "inline" ts
  let ts := match ts with | "const" :: rest => rest | rest => rest
  let ts ← expect "su2double" ts
  let ts := match ts with | "&" :: rest => rest | rest => rest
  let ts ← expect (name ++ "(unsigned long iPoint)") ts
  let ts := match ts with | "const" :: rest => rest | rest => rest
  let ts ← expect "{ return" ts
  let (field, ts) ← match ts with
    | "Volume" :: rest => some (.volume, rest)
    | "Periodic_Volume" :: rest => some (.periodic, rest)
    | "Delta_Time" :: rest => some (.timestep, rest)
    | "LocalCFL" :: rest => some (.localCFL, rest)
    | _ => none
  let [] ← expect "(iPoint); }" ts | none
  return field

end SU2Boundary
