import SU2Boundary
open SU2Boundary
example : ThreeBufferContract Generated.wallOld_original Generated.wallRes_original Generated.wallTrunc_wrongSign := by
  simp only [wall_truncation_mutant_rejected]
