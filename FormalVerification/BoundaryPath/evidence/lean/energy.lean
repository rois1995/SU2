import SU2Boundary
open SU2Boundary
example : LoopWitnessContract Generated.points_original Generated.components_skipLast := by
  simp only [component_mutant_rejected]
