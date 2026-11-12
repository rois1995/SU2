import SU2BoundaryMemory
import SU2.Thermodynamics
import CFDLean.Core.PhysicalFlux

noncomputable section
namespace SU2Initialization
open SU2 SU2Source SU2Boundary SU2BoundaryMemory
variable {d : Nat}

def toCFD (U : State d) : CFDLean.Conservative d := ⟨rho U, mom U, energy U⟩
def primitiveCFD (W : Primitive d) : CFDLean.Primitive d := ⟨W.density, W.velocity, W.pressure⟩

theorem pressure_compatible (g : IdealGas) (U : State d) :
    (toCFD U).pressure g.gamma = (toPrimitive g U).pressure := by
  simp only [CFDLean.Conservative.pressure, toCFD, toPrimitive, internalDensity,
    CFDLean.squareSum, dot, pow_two]

theorem primitive_compatible (g : IdealGas) (U : State d) :
    (toCFD U).toPrimitive g.gamma = primitiveCFD (toPrimitive g U) := by
  simp only [CFDLean.Conservative.toPrimitive, pressure_compatible]
  rfl

theorem conservative_compatible (g : IdealGas) (W : Primitive d) :
    (primitiveCFD W).toConservative g.gamma = toCFD (toConservative g W) := by
  simp [CFDLean.Primitive.toConservative, primitiveCFD, toCFD, toConservative,
    rho, mom, energy, state, CFDLean.squareSum, dot, pow_two]

theorem admissibility_compatible (g : IdealGas) (U : State d) :
    (toCFD U).Admissible g.gamma ↔ Admissible U := by
  rw [CFDLean.Conservative.Admissible, pressure_compatible]
  exact and_congr_right (fun _ => by
    change 0 < (g.gamma - 1) * internalDensity U ↔ 0 < internalDensity U
    exact mul_pos_iff_of_pos_left (sub_pos.mpr g.gamma_gt_one))

/-- Area-vector convention is passed directly: no hidden normalization or area factor. -/
theorem flux_compatible (g : IdealGas) (U : State d) (S : Vec d) :
    CFDLean.physicalFlux g.gamma (toCFD U) S =
      ⟨dot (mom U) S,
       fun i => mom U i * (dot (mom U) S / rho U) + (toPrimitive g U).pressure * S i,
       (energy U + (toPrimitive g U).pressure) * (dot (mom U) S / rho U)⟩ := by
  simp only [CFDLean.physicalFlux, pressure_compatible]
  rfl

end SU2Initialization
