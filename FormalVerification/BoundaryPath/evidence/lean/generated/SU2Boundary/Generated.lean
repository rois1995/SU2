import SU2Boundary.Parser

set_option maxRecDepth 100000

set_option maxHeartbeats 4000000

namespace SU2Boundary.Generated

def wallOld_originalSource : String := "su2double vp = 0.0;\n    for (auto iDim = 0u; iDim < nDim; iDim++) {\n      vp += (solutionOld[iVel + iDim] - gridVel[iDim]) * UnitNormal[iDim];\n    }\n    for (auto iDim = 0u; iDim < nDim; iDim++) {\n      solutionOld[iVel + iDim] -= vp * UnitNormal[iDim];\n    }"

def wallOld_original : SU2Euler.Op := (parseProjection .old wallOld_originalSource).get (by decide)

theorem wallOld_original_parsed : parseProjection .old wallOld_originalSource = some wallOld_original := by decide

def wallOld_wrongSignSource : String := "su2double vp = 0.0;\n    for (auto iDim = 0u; iDim < nDim; iDim++) {\n      vp += (solutionOld[iVel + iDim] - gridVel[iDim]) * UnitNormal[iDim];\n    }\n    for (auto iDim = 0u; iDim < nDim; iDim++) {\n      solutionOld[iVel + iDim] += vp * UnitNormal[iDim];\n    }"

def wallOld_wrongSign : SU2Euler.Op := (parseProjection .old wallOld_wrongSignSource).get (by decide)

theorem wallOld_wrongSign_parsed : parseProjection .old wallOld_wrongSignSource = some wallOld_wrongSign := by decide

def wallRes_originalSource : String := "su2double normalRes = 0.0;\n    for (auto iDim = 0u; iDim < nDim; iDim++) {\n      normalRes += LinSysRes(iPoint, iVel + iDim) * UnitNormal[iDim];\n    }\n    for (auto iDim = 0u; iDim < nDim; iDim++) {\n      LinSysRes(iPoint, iVel + iDim) -= normalRes * UnitNormal[iDim];\n    }"

def wallRes_original : SU2Euler.Op := (parseProjection .residual wallRes_originalSource).get (by decide)

theorem wallRes_original_parsed : parseProjection .residual wallRes_originalSource = some wallRes_original := by decide

def wallRes_wrongSignSource : String := "su2double normalRes = 0.0;\n    for (auto iDim = 0u; iDim < nDim; iDim++) {\n      normalRes += LinSysRes(iPoint, iVel + iDim) * UnitNormal[iDim];\n    }\n    for (auto iDim = 0u; iDim < nDim; iDim++) {\n      LinSysRes(iPoint, iVel + iDim) += normalRes * UnitNormal[iDim];\n    }"

def wallRes_wrongSign : SU2Euler.Op := (parseProjection .residual wallRes_wrongSignSource).get (by decide)

theorem wallRes_wrongSign_parsed : parseProjection .residual wallRes_wrongSignSource = some wallRes_wrongSign := by decide

def wallTrunc_originalSource : String := "normalRes = 0.0;\n    su2double* Res_TruncError = nodes->GetResTruncError(iPoint);\n    for (auto iDim = 0u; iDim < nDim; iDim++) {\n      normalRes += Res_TruncError[iVel + iDim] * UnitNormal[iDim];\n    }\n    for (auto iDim = 0u; iDim < nDim; iDim++) {\n      Res_TruncError[iVel + iDim] -= normalRes * UnitNormal[iDim];\n    }"

def wallTrunc_original : SU2Euler.Op := (parseProjection .truncation wallTrunc_originalSource).get (by decide)

theorem wallTrunc_original_parsed : parseProjection .truncation wallTrunc_originalSource = some wallTrunc_original := by decide

def wallTrunc_wrongSignSource : String := "normalRes = 0.0;\n    su2double* Res_TruncError = nodes->GetResTruncError(iPoint);\n    for (auto iDim = 0u; iDim < nDim; iDim++) {\n      normalRes += Res_TruncError[iVel + iDim] * UnitNormal[iDim];\n    }\n    for (auto iDim = 0u; iDim < nDim; iDim++) {\n      Res_TruncError[iVel + iDim] += normalRes * UnitNormal[iDim];\n    }"

def wallTrunc_wrongSign : SU2Euler.Op := (parseProjection .truncation wallTrunc_wrongSignSource).get (by decide)

theorem wallTrunc_wrongSign_parsed : parseProjection .truncation wallTrunc_wrongSignSource = some wallTrunc_wrongSign := by decide

def farAdd_originalSource : String := "LinSysRes.AddBlock(iPoint, residual);"

def farAdd_original : SU2Euler.Op := (parseBoundaryAdd farAdd_originalSource).get (by decide)

theorem farAdd_original_parsed : parseBoundaryAdd farAdd_originalSource = some farAdd_original := by decide

def farAdd_wrongSignSource : String := "LinSysRes.SubtractBlock(iPoint, residual);"

def farAdd_wrongSign : SU2Euler.Op := (parseBoundaryAdd farAdd_wrongSignSource).get (by decide)

theorem farAdd_wrongSign_parsed : parseBoundaryAdd farAdd_wrongSignSource = some farAdd_wrongSign := by decide

def points_originalSource : String := "for (unsigned long iPoint = 0; iPoint < nPointDomain; iPoint++) {"

def points_original : LoopCode := (parseLoop true points_originalSource).get (by decide)

theorem points_original_parsed : parseLoop true points_originalSource = some points_original := by decide

def points_refactoredSource : String := "for (unsigned long iPoint = 0; iPoint < nPointDomain; ++iPoint) {"

def points_refactored : LoopCode := (parseLoop true points_refactoredSource).get (by decide)

theorem points_refactored_parsed : parseLoop true points_refactoredSource = some points_refactored := by decide

def points_skipLastSource : String := "for (unsigned long iPoint = 0; iPoint < nPointDomain - 1; iPoint++) {"

def points_skipLast : LoopCode := (parseLoop true points_skipLastSource).get (by decide)

theorem points_skipLast_parsed : parseLoop true points_skipLastSource = some points_skipLast := by decide

def components_originalSource : String := "for (unsigned short iVar = 0; iVar < nVar; iVar++) {"

def components_original : LoopCode := (parseLoop false components_originalSource).get (by decide)

theorem components_original_parsed : parseLoop false components_originalSource = some components_original := by decide

def components_refactoredSource : String := "for (unsigned short iVar = 0; iVar < nVar; ++iVar) {"

def components_refactored : LoopCode := (parseLoop false components_refactoredSource).get (by decide)

theorem components_refactored_parsed : parseLoop false components_refactoredSource = some components_refactored := by decide

def components_skipLastSource : String := "for (unsigned short iVar = 0; iVar < nVar - 1; iVar++) {"

def components_skipLast : LoopCode := (parseLoop false components_skipLastSource).get (by decide)

theorem components_skipLast_parsed : parseLoop false components_skipLastSource = some components_skipLast := by decide

def volume_originalSource : String := "inline su2double& GetVolume(unsigned long iPoint) { return Volume(iPoint); }"

def volume_original : Getter := (parseGetter "GetVolume" volume_originalSource).get (by decide)

theorem volume_original_parsed : parseGetter "GetVolume" volume_originalSource = some volume_original := by decide

def volumeConst_originalSource : String := "inline const su2double& GetVolume(unsigned long iPoint) const { return Volume(iPoint); }"

def volumeConst_original : Getter := (parseGetter "GetVolume" volumeConst_originalSource).get (by decide)

theorem volumeConst_original_parsed : parseGetter "GetVolume" volumeConst_originalSource = some volumeConst_original := by decide

def periodic_originalSource : String := "inline su2double& GetPeriodicVolume(unsigned long iPoint) { return Periodic_Volume(iPoint); }"

def periodic_original : Getter := (parseGetter "GetPeriodicVolume" periodic_originalSource).get (by decide)

theorem periodic_original_parsed : parseGetter "GetPeriodicVolume" periodic_originalSource = some periodic_original := by decide

def periodicConst_originalSource : String := "inline const su2double& GetPeriodicVolume(unsigned long iPoint) const { return Periodic_Volume(iPoint); }"

def periodicConst_original : Getter := (parseGetter "GetPeriodicVolume" periodicConst_originalSource).get (by decide)

theorem periodicConst_original_parsed : parseGetter "GetPeriodicVolume" periodicConst_originalSource = some periodicConst_original := by decide

def timestep_originalSource : String := "inline su2double GetDelta_Time(unsigned long iPoint) const {return Delta_Time(iPoint); }"

def timestep_original : Getter := (parseGetter "GetDelta_Time" timestep_originalSource).get (by decide)

theorem timestep_original_parsed : parseGetter "GetDelta_Time" timestep_originalSource = some timestep_original := by decide

def timestep_wrongFieldSource : String := "inline su2double GetDelta_Time(unsigned long iPoint) const {return LocalCFL(iPoint); }"

def timestep_wrongField : Getter := (parseGetter "GetDelta_Time" timestep_wrongFieldSource).get (by decide)

theorem timestep_wrongField_parsed : parseGetter "GetDelta_Time" timestep_wrongFieldSource = some timestep_wrongField := by decide

end SU2Boundary.Generated
