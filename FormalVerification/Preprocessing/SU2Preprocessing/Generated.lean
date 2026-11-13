import SU2Preprocessing.SourceGate

set_option maxRecDepth 100000

set_option maxHeartbeats 4000000

namespace SU2Preprocessing.Generated

def originalSource : String := "unsigned long CEulerSolver::SetPrimitive_Variables(CSolver **solver_container, const CConfig *config) {\n  SU2_ZONE_SCOPED\n\n  /*--- Number of non-physical points, local to the thread, needs\n   *    further reduction if function is called in parallel ---*/\n  unsigned long nonPhysicalPoints = 0;\n\n  AD::StartNoSharedReading();\n\n  SU2_OMP_FOR_STAT(omp_chunk_size)\n  for (unsigned long iPoint = 0; iPoint < nPoint; iPoint ++) {\n\n    /*--- Compressible flow, primitive variables nDim+9, (T, vx, vy, vz, P, rho, h, c, lamMu, eddyMu, ThCond, Cp) ---*/\n\n    bool physical = nodes->SetPrimVar(iPoint, GetFluidModel());\n    nodes->SetSecondaryVar(iPoint, GetFluidModel());\n\n    /* Check for non-realizable states for reporting. */\n\n    if (!physical) nonPhysicalPoints++;\n  }\n  END_SU2_OMP_FOR\n\n  AD::EndNoSharedReading();\n\n  return nonPhysicalPoints;\n}"

def original : PrimitivePlan := ⟨false⟩

theorem original_parsed : parsePrimitiveLoop originalSource = some original := by decide

def refactoredSource : String := "unsigned long CEulerSolver::SetPrimitive_Variables(CSolver **solver_container, const CConfig *config) {\n  SU2_ZONE_SCOPED\n\n  /*--- Number of non-physical points, local to the thread, needs\n   *    further reduction if function is called in parallel ---*/\n  unsigned long nonPhysicalPoints = 0;\n\n  AD::StartNoSharedReading();\n\n  SU2_OMP_FOR_STAT(omp_chunk_size)\n  for (unsigned long iPoint = 0; iPoint < nPoint; ++iPoint) {\n\n    /*--- Compressible flow, primitive variables nDim+9, (T, vx, vy, vz, P, rho, h, c, lamMu, eddyMu, ThCond, Cp) ---*/\n\n    bool physical = nodes->SetPrimVar(iPoint, GetFluidModel());\n    nodes->SetSecondaryVar(iPoint, GetFluidModel());\n\n    /* Check for non-realizable states for reporting. */\n\n    if (!physical) nonPhysicalPoints++;\n  }\n  END_SU2_OMP_FOR\n\n  AD::EndNoSharedReading();\n\n  return nonPhysicalPoints;\n}"

def refactored : PrimitivePlan := ⟨false⟩

theorem refactored_parsed : parsePrimitiveLoop refactoredSource = some refactored := by decide

def skipLastSource : String := "unsigned long CEulerSolver::SetPrimitive_Variables(CSolver **solver_container, const CConfig *config) {\n  SU2_ZONE_SCOPED\n\n  /*--- Number of non-physical points, local to the thread, needs\n   *    further reduction if function is called in parallel ---*/\n  unsigned long nonPhysicalPoints = 0;\n\n  AD::StartNoSharedReading();\n\n  SU2_OMP_FOR_STAT(omp_chunk_size)\n  for (unsigned long iPoint = 0; iPoint + 1 < nPoint; iPoint ++) {\n\n    /*--- Compressible flow, primitive variables nDim+9, (T, vx, vy, vz, P, rho, h, c, lamMu, eddyMu, ThCond, Cp) ---*/\n\n    bool physical = nodes->SetPrimVar(iPoint, GetFluidModel());\n    nodes->SetSecondaryVar(iPoint, GetFluidModel());\n\n    /* Check for non-realizable states for reporting. */\n\n    if (!physical) nonPhysicalPoints++;\n  }\n  END_SU2_OMP_FOR\n\n  AD::EndNoSharedReading();\n\n  return nonPhysicalPoints;\n}"

def skipLast : PrimitivePlan := ⟨true⟩

theorem skipLast_parsed : parsePrimitiveLoop skipLastSource = some skipLast := by decide

end SU2Preprocessing.Generated
