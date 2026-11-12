import SU2BoundaryMemory.Execution

namespace SU2BoundaryMemory
open SU2Boundary
open SU2Euler (Op parseOp)

/-- Closed token grammars. Every admitted token is consumed; an extra statement,
unknown spelling or unsupported branch returns none. Source selection is external. -/
def parseOldBinding (s : String) : Option Nat := do
  let ts ← lex s
  let ts ← expect "su2double* solutionOld = nodes->" ts
  let (b, ts) ← match ts with
    | "GetSolution_Old" :: rest => some (1, rest)
    | "GetSolution" :: rest => some (0, rest)
    | _ => none
  let [] ← expect "(iPoint);" ts | none
  return b

def parsePrimitiveExtra (s : String) : Option Nat := do
  let ts ← lex s
  let ts ← expect "nPrimVar = nDim +" ts
  match ts with | ["9", ";"] => some 9 | ["4", ";"] => some 4 | _ => none

def parseCacheLookup (s : String) : Option CacheMode := do
  let ts ← lex s
  let ts ← expect "inline su2double* GetCharacPrimVar(unsigned short val_marker, unsigned long val_vertex) final { return CharacPrimVar[" ts
  let (first, ts) ← match ts with
    | "val_marker" :: rest => some (false, rest)
    | "0" :: rest => some (true, rest)
    | _ => none
  let ts ← expect "][" ts
  let (firstVertex, ts) ← match ts with
    | "val_vertex" :: rest => some (false, rest)
    | "0" :: rest => some (true, rest)
    | _ => none
  let [] ← expect "]; }" ts | none
  if first && firstVertex then none else
    return if first then .firstMarker else if firstVertex then .firstVertex else .normal

def parseExact (expected s : String) : Option Unit := do
  let ts ← lex s
  let [] ← expect expected ts | none
  return ()

def parseCacheWrites (s : String) : Option Bool := do
  let ts ← lex s
  let ts ← expect "V_infty[0] = Pressure/(Gas_Constant*Density); for (iDim = 0; iDim < nDim; iDim++) V_infty[iDim+1] = Velocity[iDim]; V_infty[nDim+1] = Pressure; V_infty[nDim+2] = Density; V_infty[nDim+3] = Energy + Pressure/Density;" ts
  if ts.isEmpty then return false
  let [] ← expect "for (unsigned short j = 6; j < nPrimVar; ++j) V_infty[j] = 0.0;" ts | none
  return true

structure ProjectionPlan where
  op : Op
  refresh : Bool
  reverse : Bool
  deriving DecidableEq, Repr

/-- The optional nested reduction and reversed correction traversal have actual
execution semantics; their syntactically valid changes are not parser failures. -/
def parseProjectionPlan (buffer : WallBuffer) (s : String) : Option ProjectionPlan := do
  let ts ← lex s
  let (binder, target, init, term, refreshTerm) := match buffer with
    | .old => ("vp", "solutionOld[iVel + iDim]", "su2double vp = 0.0;",
        "(solutionOld[iVel + iDim] - gridVel[iDim])", "(solutionOld[iVel + jDim] - gridVel[jDim])")
    | .residual => ("normalRes", "LinSysRes(iPoint, iVel + iDim)", "su2double normalRes = 0.0;",
        "LinSysRes(iPoint, iVel + iDim)", "LinSysRes(iPoint, iVel + jDim)")
    | .truncation => ("normalRes", "Res_TruncError[iVel + iDim]",
        "normalRes = 0.0; su2double* Res_TruncError = nodes->GetResTruncError(iPoint);",
        "Res_TruncError[iVel + iDim]", "Res_TruncError[iVel + jDim]")
  let ts ← expect (init ++ " for (auto iDim = 0u; iDim < nDim; iDim++) { " ++ binder ++
    " += " ++ term ++ " * UnitNormal[iDim]; }") ts
  let (reverse, ts) ← match expect "for (auto iDim = 0u; iDim < nDim; iDim++) {" ts with
    | some rest => some (false, rest)
    | none => do
        let rest ← expect "for (auto iDim = nDim; iDim-- > 0;) {" ts
        pure (true, rest)
  let (refresh, ts) := match expect (binder ++ " = 0.0; for (auto jDim = 0u; jDim < nDim; jDim++) { " ++
      binder ++ " += " ++ refreshTerm ++ " * UnitNormal[jDim]; }") ts with
    | some rest => (true, rest)
    | none => (false, ts)
  if reverse && refresh then none else do
  let ts ← expect target ts
  let (op, ts) ← parseOp ts
  let [] ← expect ("= " ++ binder ++ " * UnitNormal[iDim]; }") ts | none
  return ⟨op, refresh, reverse⟩

def parseSetBlock (s : String) : Option Op := do
  let ts ← lex s
  let ts ← expect "template <class VectorType, bool Overwrite = true> FORCEINLINE void SetBlock(unsigned long iPoint, const VectorType& block, ScalarType alpha = 1) { if (Overwrite) { for (auto i = 0ul; i < nVar; ++i) vec_val[iPoint * nVar + i] = alpha * block[i]; } else { for (auto i = 0ul; i < nVar; ++i) vec_val[iPoint * nVar + i]" ts
  let (op, ts) ← parseOp ts
  let [] ← expect "= alpha * block[i]; } }" ts | none
  return op

def parsePrimitiveGetter (s : String) : Option Unit :=
  parseExact "inline su2double* GetPrimitive(unsigned long iPoint) final { return Primitive[iPoint]; }" s

def parseAddBlock (s : String) : Option Unit :=
  parseExact "template <class VectorType> FORCEINLINE void AddBlock(unsigned long iPoint, const VectorType& block, ScalarType alpha = 1) { SetBlock<VectorType, false>(iPoint, block, alpha); }" s

def parseCacheAllocation (s : String) : Option Unit :=
  parseExact "AllocVectorOfMatrices(nVertex, nPrimVar, CharacPrimVar);" s

def parsePrimitiveAllocation (s : String) : Option Unit :=
  parseExact "Primitive.resize(nPoint, nPrimVar) = su2double(0.0);" s

def parseRowPointer (s : String) : Option Unit :=
  parseExact "return &m_data[k * N];" s

end SU2BoundaryMemory
