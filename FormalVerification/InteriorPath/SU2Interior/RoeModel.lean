import SU2Interior.Geometry

set_option maxRecDepth 100000
set_option maxHeartbeats 8000000
noncomputable section
open scoped BigOperators
namespace SU2Interior
open SU2 SU2Source SU2Boundary SU2BoundaryMemory SU2Initialization SU2Preprocessing
variable {M E : Nat}

abbrev Row6 := Fin 6 → ℝ
abbrev Flux4 := Fin 4 → ℝ
abbrev Tensor4 := Fin 4 → Fin 4 → ℝ

def rowOf (P : Primitives M) (p : Fin M) : Row6 := ![P p 0,P p 1,P p 2,P p 3,P p 4,P p 5]
def rowPrimitive (V : Row6) : Primitive 2 := ⟨V 4,![V 1,V 2],V 3⟩
def rowVelocity (V : Row6) : Vec 2 := ![V 1,V 2]

/-- Actual SIMD energy operation rho*H-P, distinct from the scalar boundary expression. -/
def simdConservative (V : Row6) : Flux4 := ![V 4,V 4*V 1,V 4*V 2,V 4*V 5-V 3]

def simdPhysicalFlux (V : Row6) (n : Vec 2) : Flux4 :=
  let U := simdConservative V
  let mdot := (0+U 1*n 0)+U 2*n 1
  ![mdot,mdot*V 1+n 0*V 3,mdot*V 2+n 1*V 3,mdot*V 5]

structure Averaged where
  density : ℝ
  velocity : Vec 2
  enthalpy : ℝ
  sound : ℝ
  projection : ℝ

/-- PROVISIONAL source-derived SIMD arithmetic, not an independently approved Roe spec.
Unlike the scalar boundary routine it has no abs in the density ratio and no c2<=0 guard. -/
def simdAverages (g : IdealGas) (L R : Row6) (n : Vec 2) : Averaged :=
  let ratio := Real.sqrt (R 4/L 4)
  let inverse := 1/(ratio+1)
  let velocity := fun i => (ratio*rowVelocity R i+rowVelocity L i)*inverse
  let H := (ratio*R 5+L 5)*inverse
  let c := Real.sqrt ((g.gamma-1)*(H-(1/2 : ℝ)*dot velocity velocity))
  ⟨ratio*L 4,velocity,H,c,dot velocity n⟩

/-- The selected 2D pMatrix assignments, with their source normalization. -/
def simdP (g : IdealGas) (a : Averaged) (n : Vec 2) : Tensor4 :=
  let r := a.density
  let v := a.velocity
  let k := (1/2 : ℝ)*dot v v
  let r2 := (1/2 : ℝ)*r
  let rc := r2/a.sound
  ![![1,0,rc,rc],
    ![v 0,r*n 1,rc*v 0+r2*n 0,rc*v 0-r2*n 0],
    ![v 1,-r*n 0,rc*v 1+r2*n 1,rc*v 1-r2*n 1],
    ![k,r*(v 0*n 1-v 1*n 0),rc*k+r2*(a.projection+a.sound/(g.gamma-1)),
      rc*k-r2*(a.projection-a.sound/(g.gamma-1))]]

def simdInverseP (g : IdealGas) (a : Averaged) (n : Vec 2) : Tensor4 :=
  let v := a.velocity
  let k := (1/2 : ℝ)*dot v v
  let inverseRho := 1/a.density
  let tmp := (g.gamma-1)/(a.sound^2)
  let gc := (g.gamma-1)/(a.density*a.sound)
  ![![1-tmp*k,tmp*v 0,tmp*v 1,-tmp],
    ![(n 0*v 1-n 1*v 0)*inverseRho,n 1*inverseRho,-n 0*inverseRho,0],
    ![-a.projection*inverseRho+gc*k,n 0*inverseRho-gc*v 0,n 1*inverseRho-gc*v 1,gc],
    ![a.projection*inverseRho+gc*k,-n 0*inverseRho-gc*v 0,-n 1*inverseRho-gc*v 1,gc]]

/-- Entropy coefficient=0 and first-order nonPhysical=0; retain the actual max operation. -/
def simdSpeeds (a : Averaged) : Flux4 :=
  let minimum := max (0 : ℝ) 0 * (|a.projection|+a.sound)
  ![max |a.projection| minimum,max |a.projection| minimum,
    max |a.projection+a.sound| minimum,max |a.projection-a.sound| minimum]

def sourceDissipation (P Q : Tensor4) (speeds delta : Flux4) (area : ℝ) (central : Flux4) : Flux4 :=
  fun i => (List.ofFn (fun j : Fin 4 => j)).foldl (fun flux j =>
    let tensor := (List.ofFn (fun k : Fin 4 => P i k*speeds k*Q k j)).foldl (· + ·) 0
    let derivative := tensor*(1-(1/2 : ℝ))*area*1
    flux-derivative*delta j) (central i)

/-- This independent finite-sum identity verifies the nested scalar accumulation;
it does not assert that P,Q or speeds are an approved Roe eigendecomposition. -/
theorem source_dissipation_correct (P Q : Tensor4) (speeds delta : Flux4) (area : ℝ) (central : Flux4) :
    sourceDissipation P Q speeds delta area central =
      fun i => central i-area/2*∑ j, (∑ k, P i k*speeds k*Q k j)*delta j := by
  funext i
  simp [sourceDissipation,List.ofFn_succ,Fin.sum_univ_succ]
  ring

theorem source_dissipation_zero (P Q : Tensor4) (speeds : Flux4) (area : ℝ) (central : Flux4) :
    sourceDissipation P Q speeds (fun _ => 0) area central = central := by
  funext i
  simp [sourceDissipation,List.ofFn_succ]

def simdFlux (g : IdealGas) (normal : Vec 2) (L R : Row6) : Flux4 :=
  let area := faceArea normal
  let unit := fun i => normal i/area
  let a := simdAverages g L R unit
  let left := simdConservative L
  let right := simdConservative R
  sourceDissipation (simdP g a unit) (simdInverseP g a unit) (simdSpeeds a)
    (fun c => right c-left c) area (fun c => (1/2 : ℝ)*(simdPhysicalFlux L normal c+simdPhysicalFlux R normal c))

theorem simd_equal_state (g : IdealGas) (n : Vec 2) (V : Row6) :
    simdFlux g n V V = simdPhysicalFlux V n := by
  simp only [simdFlux,sub_self,source_dissipation_zero]
  funext c; ring

theorem simd_physical_stored (V : Row6) (n : Vec 2) :
    simdPhysicalFlux V n = storedFlux (V 5) n (rowPrimitive V) := by
  funext c
  fin_cases c <;> simp [simdPhysicalFlux,simdConservative,storedFlux,rowPrimitive,dot,Fin.sum_univ_two] <;> ring

/-- Equal-state consistency with the preserved physical Euler flux, under the declared
enthalpy convention. Complete Roe mathematics still awaits independent CFDLean approval. -/
theorem simd_equal_state_physical (g : IdealGas) (n : Vec 2) (V : Row6)
    (enthalpy : V 5=totalEnthalpy g (rowPrimitive V)) :
    simdFlux g n V V = physicalFlux g n (rowPrimitive V) := by
  rw [simd_equal_state,simd_physical_stored,enthalpy,stored_flux_correct]

/-- All six gather columns are read even though temperature is unused by first-order Roe. -/
def readRow6 (m : Mem) (p : Fin M) : Option Row6 := do
  let t ← m (primitiveAddr p 0)
  let vx ← m (primitiveAddr p 1)
  let vy ← m (primitiveAddr p 2)
  let pressure ← m (primitiveAddr p 3)
  let density ← m (primitiveAddr p 4)
  let H ← m (primitiveAddr p 5)
  return ![t,vx,vy,pressure,density,H]

def executeFlux (g : IdealGas) (geo : Geometry M E) (e : Fin E) (m : Mem) : Option Flux4 := do
  let L ← readRow6 m (geo.left e)
  let R ← readRow6 m (geo.right e)
  return simdFlux g (geo.normal e) L R

def edgeFlux (g : IdealGas) (geo : Geometry M E) (P : Primitives M) (e : Fin E) : Flux4 :=
  simdFlux g (geo.normal e) (rowOf P (geo.left e)) (rowOf P (geo.right e))

theorem read_row_correct (m : Mem) (P : Primitives M) (h : PrimitiveRep m P) (p : Fin M) :
    readRow6 m p = some (rowOf P p) := by
  unfold PrimitiveRep at h
  simp [readRow6,h,rowOf]

theorem execute_flux_correct (g : IdealGas) (geo : Geometry M E) (e : Fin E)
    (m : Mem) (P : Primitives M) (h : PrimitiveRep m P) :
    executeFlux g geo e m = some (edgeFlux g geo P e) := by
  simp [executeFlux,read_row_correct m P h,edgeFlux]

end SU2Interior
