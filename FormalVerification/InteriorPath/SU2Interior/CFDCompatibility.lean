import SU2Interior.RoeModel
import CFDLean.Flux.Roe.Averages

set_option maxRecDepth 100000
set_option maxHeartbeats 8000000
noncomputable section
namespace SU2Interior
open SU2 SU2Source SU2Boundary SU2BoundaryMemory SU2Initialization SU2Preprocessing

abbrev Coordinates3 := CFDLean.Coordinates.StateCoordinates

def planeIndex (i : Fin 2) : Fin 3 := ⟨i.val,lt_trans i.isLt (by decide)⟩
def planeCoordinates (U : State 2) : Coordinates3 := ![rho U,mom U 0,mom U 1,0,energy U]

theorem plane_pressure (g : IdealGas) (U : State 2) :
    CFDLean.PhysicalJacobian.coordinatePressure g.gamma (planeCoordinates U) = (toPrimitive g U).pressure := by
  rw [CFDLean.PhysicalJacobian.pressure_formula]
  simp [planeCoordinates,toPrimitive,internalDensity,dot,Fin.sum_univ_two,pow_two]

theorem plane_admissible (g : IdealGas) (U : State 2) (h : Admissible U) :
    (CFDLean.Coordinates.decode (planeCoordinates U)).Admissible g.gamma := by
  refine ⟨h.1,?_⟩
  change 0 < CFDLean.PhysicalJacobian.coordinatePressure g.gamma (planeCoordinates U)
  rw [plane_pressure]
  exact (primitive_admissible g U h).2

/-- An input-convention relation, not an assumption about Roe averages or outputs. -/
structure RowMatches (g : IdealGas) (V : Row6) (x : Coordinates3) : Prop where
  density : V 4 = x 0
  velocity : ∀ i, rowVelocity V i = CFDLean.PhysicalJacobian.velocity x (planeIndex i)
  enthalpy : V 5 = CFDLean.PhysicalJacobian.enthalpy g.gamma x
  zeroThird : CFDLean.PhysicalJacobian.velocity x 2 = 0

theorem primitive_row_matches (g : IdealGas) (U : State 2) (prior : Fin 11 → ℝ) :
    RowMatches g (rowOf (fun _ : Fin 1 => primitiveFields g U prior) 0) (planeCoordinates U) := by
  refine ⟨rfl,?_,?_,?_⟩
  · intro i
    fin_cases i <;> simp [rowVelocity,rowOf,primitiveFields,toPrimitive,
      CFDLean.PhysicalJacobian.velocity,CFDLean.Coordinates.decode,planeCoordinates,planeIndex]
  · change enthalpy g U = _
    rw [CFDLean.PhysicalJacobian.enthalpy,plane_pressure]
    rfl
  · simp [CFDLean.PhysicalJacobian.velocity,CFDLean.Coordinates.decode,planeCoordinates]

theorem ratio_weights (a b : ℝ) (ha : 0<a) (hb : 0<b) :
    1/(Real.sqrt (b/a)+1) = Real.sqrt a/(Real.sqrt a+Real.sqrt b) ∧
    Real.sqrt (b/a)/(Real.sqrt (b/a)+1) = Real.sqrt b/(Real.sqrt a+Real.sqrt b) := by
  rw [Real.sqrt_div (le_of_lt hb)]
  have sa : Real.sqrt a ≠ 0 := ne_of_gt (Real.sqrt_pos.mpr ha)
  have sum : Real.sqrt a+Real.sqrt b ≠ 0 := ne_of_gt (add_pos (Real.sqrt_pos.mpr ha) (Real.sqrt_pos.mpr hb))
  constructor <;> field_simp [sa,sum]
  all_goals ring_nf
  all_goals simp

theorem source_weights (x y : Coordinates3) (hx : 0<x 0) (hy : 0<y 0) :
    1/(Real.sqrt (y 0/x 0)+1) = CFDLean.Roe.weight x y ∧
    Real.sqrt (y 0/x 0)*(1/(Real.sqrt (y 0/x 0)+1)) = CFDLean.Roe.weight y x := by
  have weights := ratio_weights (x 0) (y 0) hx hy
  constructor
  · simpa only [CFDLean.Roe.weight,CFDLean.Roe.root] using weights.1
  · simpa only [CFDLean.Roe.weight,CFDLean.Roe.root,add_comm,div_eq_mul_inv,one_mul] using weights.2

theorem source_average_velocity (g : IdealGas) (L R : Row6) (x y : Coordinates3) (n : Vec 2)
    (hl : RowMatches g L x) (hr : RowMatches g R y) (hx : 0<x 0) (hy : 0<y 0) (i : Fin 2) :
    (simdAverages g L R n).velocity i = CFDLean.Roe.averageVelocity x y (planeIndex i) := by
  have weights := source_weights x y hx hy
  change (Real.sqrt (R 4/L 4)*rowVelocity R i+rowVelocity L i)*(1/(Real.sqrt (R 4/L 4)+1)) = _
  rw [hl.density,hr.density]
  calc
    _ = (Real.sqrt (y 0/x 0)*(1/(Real.sqrt (y 0/x 0)+1)))*rowVelocity R i+
        (1/(Real.sqrt (y 0/x 0)+1))*rowVelocity L i := by ring
    _ = CFDLean.Roe.averageVelocity x y (planeIndex i) := by
      rw [weights.2,weights.1,hl.velocity,hr.velocity]
      exact add_comm _ _

theorem source_average_enthalpy (g : IdealGas) (L R : Row6) (x y : Coordinates3) (n : Vec 2)
    (hl : RowMatches g L x) (hr : RowMatches g R y) (hx : 0<x 0) (hy : 0<y 0) :
    (simdAverages g L R n).enthalpy = CFDLean.Roe.averageEnthalpy g.gamma x y := by
  have weights := source_weights x y hx hy
  change (Real.sqrt (R 4/L 4)*R 5+L 5)*(1/(Real.sqrt (R 4/L 4)+1)) = _
  rw [hl.density,hr.density]
  calc
    _ = (Real.sqrt (y 0/x 0)*(1/(Real.sqrt (y 0/x 0)+1)))*R 5+
        (1/(Real.sqrt (y 0/x 0)+1))*L 5 := by ring
    _ = CFDLean.Roe.averageEnthalpy g.gamma x y := by
      rw [weights.2,weights.1,hl.enthalpy,hr.enthalpy]
      exact add_comm _ _

theorem source_average_sound (g : IdealGas) (L R : Row6) (x y : Coordinates3) (n : Vec 2)
    (hl : RowMatches g L x) (hr : RowMatches g R y) (hx : 0<x 0) (hy : 0<y 0) :
    (simdAverages g L R n).sound = CFDLean.Roe.soundSpeed g.gamma x y := by
  have vel := source_average_velocity g L R x y n hl hr hx hy
  have H := source_average_enthalpy g L R x y n hl hr hx hy
  have z : CFDLean.Roe.averageVelocity x y 2=0 := by simp [CFDLean.Roe.averageVelocity,hl.zeroThird,hr.zeroThird]
  have square : dot (simdAverages g L R n).velocity (simdAverages g L R n).velocity =
      CFDLean.squareSum (CFDLean.Roe.averageVelocity x y) := by
    simp only [dot,Fin.sum_univ_two,CFDLean.Coordinates.squareSum_three]
    rw [vel 0,vel 1,z]
    simp [pow_two,planeIndex]
  change Real.sqrt ((g.gamma-1)*((simdAverages g L R n).enthalpy-
    (1/2 : ℝ)*dot (simdAverages g L R n).velocity (simdAverages g L R n).velocity)) = _
  rw [H,square]
  unfold CFDLean.Roe.soundSpeed CFDLean.Roe.soundSquare
  congr 1
  ring

/-- Reuses independently accepted M2b sound positivity. This does not certify the
P/Q eigensystem, entropy policy, absolute operator or complete numerical Roe flux. -/
theorem source_sound_positive (g : IdealGas) (L R : Row6) (x y : Coordinates3) (n : Vec 2)
    (hl : RowMatches g L x) (hr : RowMatches g R y)
    (hx : (CFDLean.Coordinates.decode x).Admissible g.gamma)
    (hy : (CFDLean.Coordinates.decode y).Admissible g.gamma) : 0<(simdAverages g L R n).sound := by
  rw [source_average_sound g L R x y n hl hr hx.1 hy.1]
  exact CFDLean.Roe.soundSpeed_pos g.gamma x y g.gamma_gt_one hx hy

end SU2Interior
