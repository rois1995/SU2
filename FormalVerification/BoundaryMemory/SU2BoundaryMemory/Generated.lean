import SU2BoundaryMemory.Parser

set_option maxRecDepth 100000

set_option maxHeartbeats 8000000

namespace SU2BoundaryMemory.Generated

def oldBinding_originalSource : String := "su2double* solutionOld = nodes->GetSolution_Old(iPoint);"

def oldBinding_original : Nat := (parseOldBinding oldBinding_originalSource).get (by decide)

theorem oldBinding_original_parsed : parseOldBinding oldBinding_originalSource = some oldBinding_original := by decide

def oldBinding_wrongBufferSource : String := "su2double* solutionOld = nodes->GetSolution(iPoint);"

def oldBinding_wrongBuffer : Nat := (parseOldBinding oldBinding_wrongBufferSource).get (by decide)

theorem oldBinding_wrongBuffer_parsed : parseOldBinding oldBinding_wrongBufferSource = some oldBinding_wrongBuffer := by decide

def primitiveExtra_originalSource : String := "nPrimVar = nDim + 9;"

def primitiveExtra_original : Nat := (parsePrimitiveExtra primitiveExtra_originalSource).get (by decide)

theorem primitiveExtra_original_parsed : parsePrimitiveExtra primitiveExtra_originalSource = some primitiveExtra_original := by decide

def primitiveExtra_shortStrideSource : String := "nPrimVar = nDim + 4;"

def primitiveExtra_shortStride : Nat := (parsePrimitiveExtra primitiveExtra_shortStrideSource).get (by decide)

theorem primitiveExtra_shortStride_parsed : parsePrimitiveExtra primitiveExtra_shortStrideSource = some primitiveExtra_shortStride := by decide

def cacheLookup_originalSource : String := "inline su2double* GetCharacPrimVar(unsigned short val_marker, unsigned long val_vertex) final {\n    return CharacPrimVar[val_marker][val_vertex];\n  }"

def cacheLookup_original : CacheMode := (parseCacheLookup cacheLookup_originalSource).get (by decide)

theorem cacheLookup_original_parsed : parseCacheLookup cacheLookup_originalSource = some cacheLookup_original := by decide

def cacheLookup_firstMarkerSource : String := "inline su2double* GetCharacPrimVar(unsigned short val_marker, unsigned long val_vertex) final {\n    return CharacPrimVar[0][val_vertex];\n  }"

def cacheLookup_firstMarker : CacheMode := (parseCacheLookup cacheLookup_firstMarkerSource).get (by decide)

theorem cacheLookup_firstMarker_parsed : parseCacheLookup cacheLookup_firstMarkerSource = some cacheLookup_firstMarker := by decide

def cacheLookup_firstVertexSource : String := "inline su2double* GetCharacPrimVar(unsigned short val_marker, unsigned long val_vertex) final {\n    return CharacPrimVar[val_marker][0];\n  }"

def cacheLookup_firstVertex : CacheMode := (parseCacheLookup cacheLookup_firstVertexSource).get (by decide)

theorem cacheLookup_firstVertex_parsed : parseCacheLookup cacheLookup_firstVertexSource = some cacheLookup_firstVertex := by decide

def cacheWrites_originalSource : String := "V_infty[0] = Pressure/(Gas_Constant*Density);\n      for (iDim = 0; iDim < nDim; iDim++)\n        V_infty[iDim+1] = Velocity[iDim];\n      V_infty[nDim+1] = Pressure;\n      V_infty[nDim+2] = Density;\n      V_infty[nDim+3] = Energy + Pressure/Density;"

def cacheWrites_original : Bool := (parseCacheWrites cacheWrites_originalSource).get (by decide)

theorem cacheWrites_original_parsed : parseCacheWrites cacheWrites_originalSource = some cacheWrites_original := by decide

def cacheWrites_eraseTailSource : String := "V_infty[0] = Pressure/(Gas_Constant*Density);\n      for (iDim = 0; iDim < nDim; iDim++)\n        V_infty[iDim+1] = Velocity[iDim];\n      V_infty[nDim+1] = Pressure;\n      V_infty[nDim+2] = Density;\n      V_infty[nDim+3] = Energy + Pressure/Density;\nfor (unsigned short j = 6; j < nPrimVar; ++j) V_infty[j] = 0.0;"

def cacheWrites_eraseTail : Bool := (parseCacheWrites cacheWrites_eraseTailSource).get (by decide)

theorem cacheWrites_eraseTail_parsed : parseCacheWrites cacheWrites_eraseTailSource = some cacheWrites_eraseTail := by decide

def wallOld_originalSource : String := "su2double vp = 0.0;\n    for (auto iDim = 0u; iDim < nDim; iDim++) {\n      vp += (solutionOld[iVel + iDim] - gridVel[iDim]) * UnitNormal[iDim];\n    }\n    for (auto iDim = 0u; iDim < nDim; iDim++) {\n      solutionOld[iVel + iDim] -= vp * UnitNormal[iDim];\n    }"

def wallOld_original : ProjectionPlan := (parseProjectionPlan .old wallOld_originalSource).get (by decide)

theorem wallOld_original_parsed : parseProjectionPlan .old wallOld_originalSource = some wallOld_original := by decide

def wallOld_refreshSource : String := "su2double vp = 0.0;\n    for (auto iDim = 0u; iDim < nDim; iDim++) {\n      vp += (solutionOld[iVel + iDim] - gridVel[iDim]) * UnitNormal[iDim];\n    }\n    for (auto iDim = 0u; iDim < nDim; iDim++) {\n      vp = 0.0; for (auto jDim = 0u; jDim < nDim; jDim++) { vp += (solutionOld[iVel + jDim] - gridVel[jDim]) * UnitNormal[jDim]; }\n      solutionOld[iVel + iDim] -= vp * UnitNormal[iDim];\n    }"

def wallOld_refresh : ProjectionPlan := (parseProjectionPlan .old wallOld_refreshSource).get (by decide)

theorem wallOld_refresh_parsed : parseProjectionPlan .old wallOld_refreshSource = some wallOld_refresh := by decide

def wallOld_refactoredSource : String := "su2double vp = 0.0;\n    for (auto iDim = 0u; iDim < nDim; iDim++) {\n      vp += (solutionOld[iVel + iDim] - gridVel[iDim]) * UnitNormal[iDim];\n    }\n    for (auto iDim = nDim; iDim-- > 0;) {\n      solutionOld[iVel + iDim] -= vp * UnitNormal[iDim];\n    }"

def wallOld_refactored : ProjectionPlan := (parseProjectionPlan .old wallOld_refactoredSource).get (by decide)

theorem wallOld_refactored_parsed : parseProjectionPlan .old wallOld_refactoredSource = some wallOld_refactored := by decide

def wallRes_originalSource : String := "su2double normalRes = 0.0;\n    for (auto iDim = 0u; iDim < nDim; iDim++) {\n      normalRes += LinSysRes(iPoint, iVel + iDim) * UnitNormal[iDim];\n    }\n    for (auto iDim = 0u; iDim < nDim; iDim++) {\n      LinSysRes(iPoint, iVel + iDim) -= normalRes * UnitNormal[iDim];\n    }"

def wallRes_original : ProjectionPlan := (parseProjectionPlan .residual wallRes_originalSource).get (by decide)

theorem wallRes_original_parsed : parseProjectionPlan .residual wallRes_originalSource = some wallRes_original := by decide

def wallRes_refreshSource : String := "su2double normalRes = 0.0;\n    for (auto iDim = 0u; iDim < nDim; iDim++) {\n      normalRes += LinSysRes(iPoint, iVel + iDim) * UnitNormal[iDim];\n    }\n    for (auto iDim = 0u; iDim < nDim; iDim++) {\n      normalRes = 0.0; for (auto jDim = 0u; jDim < nDim; jDim++) { normalRes += LinSysRes(iPoint, iVel + jDim) * UnitNormal[jDim]; }\n      LinSysRes(iPoint, iVel + iDim) -= normalRes * UnitNormal[iDim];\n    }"

def wallRes_refresh : ProjectionPlan := (parseProjectionPlan .residual wallRes_refreshSource).get (by decide)

theorem wallRes_refresh_parsed : parseProjectionPlan .residual wallRes_refreshSource = some wallRes_refresh := by decide

def wallRes_refactoredSource : String := "su2double normalRes = 0.0;\n    for (auto iDim = 0u; iDim < nDim; iDim++) {\n      normalRes += LinSysRes(iPoint, iVel + iDim) * UnitNormal[iDim];\n    }\n    for (auto iDim = nDim; iDim-- > 0;) {\n      LinSysRes(iPoint, iVel + iDim) -= normalRes * UnitNormal[iDim];\n    }"

def wallRes_refactored : ProjectionPlan := (parseProjectionPlan .residual wallRes_refactoredSource).get (by decide)

theorem wallRes_refactored_parsed : parseProjectionPlan .residual wallRes_refactoredSource = some wallRes_refactored := by decide

def wallTrunc_originalSource : String := "normalRes = 0.0;\n    su2double* Res_TruncError = nodes->GetResTruncError(iPoint);\n    for (auto iDim = 0u; iDim < nDim; iDim++) {\n      normalRes += Res_TruncError[iVel + iDim] * UnitNormal[iDim];\n    }\n    for (auto iDim = 0u; iDim < nDim; iDim++) {\n      Res_TruncError[iVel + iDim] -= normalRes * UnitNormal[iDim];\n    }"

def wallTrunc_original : ProjectionPlan := (parseProjectionPlan .truncation wallTrunc_originalSource).get (by decide)

theorem wallTrunc_original_parsed : parseProjectionPlan .truncation wallTrunc_originalSource = some wallTrunc_original := by decide

def wallTrunc_refreshSource : String := "normalRes = 0.0;\n    su2double* Res_TruncError = nodes->GetResTruncError(iPoint);\n    for (auto iDim = 0u; iDim < nDim; iDim++) {\n      normalRes += Res_TruncError[iVel + iDim] * UnitNormal[iDim];\n    }\n    for (auto iDim = 0u; iDim < nDim; iDim++) {\n      normalRes = 0.0; for (auto jDim = 0u; jDim < nDim; jDim++) { normalRes += Res_TruncError[iVel + jDim] * UnitNormal[jDim]; }\n      Res_TruncError[iVel + iDim] -= normalRes * UnitNormal[iDim];\n    }"

def wallTrunc_refresh : ProjectionPlan := (parseProjectionPlan .truncation wallTrunc_refreshSource).get (by decide)

theorem wallTrunc_refresh_parsed : parseProjectionPlan .truncation wallTrunc_refreshSource = some wallTrunc_refresh := by decide

def wallTrunc_refactoredSource : String := "normalRes = 0.0;\n    su2double* Res_TruncError = nodes->GetResTruncError(iPoint);\n    for (auto iDim = 0u; iDim < nDim; iDim++) {\n      normalRes += Res_TruncError[iVel + iDim] * UnitNormal[iDim];\n    }\n    for (auto iDim = nDim; iDim-- > 0;) {\n      Res_TruncError[iVel + iDim] -= normalRes * UnitNormal[iDim];\n    }"

def wallTrunc_refactored : ProjectionPlan := (parseProjectionPlan .truncation wallTrunc_refactoredSource).get (by decide)

theorem wallTrunc_refactored_parsed : parseProjectionPlan .truncation wallTrunc_refactoredSource = some wallTrunc_refactored := by decide

def setBlock_originalSource : String := "template <class VectorType, bool Overwrite = true>\n  FORCEINLINE void SetBlock(unsigned long iPoint, const VectorType& block, ScalarType alpha = 1) {\n    if (Overwrite) {\n      for (auto i = 0ul; i < nVar; ++i) vec_val[iPoint * nVar + i] = alpha * block[i];\n    } else {\n      for (auto i = 0ul; i < nVar; ++i) vec_val[iPoint * nVar + i] += alpha * block[i];\n    }\n  }"

def setBlock_original : SU2Euler.Op := (parseSetBlock setBlock_originalSource).get (by decide)

theorem setBlock_original_parsed : parseSetBlock setBlock_originalSource = some setBlock_original := by decide

def primitiveGetter_originalSource : String := "inline su2double* GetPrimitive(unsigned long iPoint) final { return Primitive[iPoint]; }"

def primitiveGetter_original : Unit := (parsePrimitiveGetter primitiveGetter_originalSource).get (by decide)

theorem primitiveGetter_original_parsed : parsePrimitiveGetter primitiveGetter_originalSource = some primitiveGetter_original := by decide

def addBlock_originalSource : String := "template <class VectorType>\n  FORCEINLINE void AddBlock(unsigned long iPoint, const VectorType& block, ScalarType alpha = 1) {\n    SetBlock<VectorType, false>(iPoint, block, alpha);\n  }"

def addBlock_original : Unit := (parseAddBlock addBlock_originalSource).get (by decide)

theorem addBlock_original_parsed : parseAddBlock addBlock_originalSource = some addBlock_original := by decide

def cacheAllocation_originalSource : String := "AllocVectorOfMatrices(nVertex, nPrimVar, CharacPrimVar);"

def cacheAllocation_original : Unit := (parseCacheAllocation cacheAllocation_originalSource).get (by decide)

theorem cacheAllocation_original_parsed : parseCacheAllocation cacheAllocation_originalSource = some cacheAllocation_original := by decide

def primitiveAllocation_originalSource : String := "Primitive.resize(nPoint, nPrimVar) = su2double(0.0);"

def primitiveAllocation_original : Unit := (parsePrimitiveAllocation primitiveAllocation_originalSource).get (by decide)

theorem primitiveAllocation_original_parsed : parsePrimitiveAllocation primitiveAllocation_originalSource = some primitiveAllocation_original := by decide

def rowPointer_originalSource : String := "return &m_data[k * N];"

def rowPointer_original : Unit := (parseRowPointer rowPointer_originalSource).get (by decide)

theorem rowPointer_original_parsed : parseRowPointer rowPointer_originalSource = some rowPointer_original := by decide

end SU2BoundaryMemory.Generated
