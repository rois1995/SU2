import SU2Boundary.Spec
import SU2Boundary.Generated

set_option maxRecDepth 100000
set_option maxHeartbeats 4000000

noncomputable section
open scoped BigOperators
namespace SU2Boundary
open SU2 SU2Euler
variable {N : Nat}

/-- Two stationary 2D reduction iterations precede both correction stores. -/
def projection (op : Op) (n : Vec 2) (U : State 2) : State 2 :=
  let a := (0 + U (.momentum 0) * n 0) + U (.momentum 1) * n 1
  let U₀ := Function.update U (.momentum 0) (opReal op (U (.momentum 0)) (a * n 0))
  Function.update U₀ (.momentum 1) (opReal op (U₀ (.momentum 1)) (a * n 1))

theorem projection_correct (n : Vec 2) (U : State 2) :
    projection .sub n U = wallProject n 0 U := by
  funext c
  cases c with
  | mass => simp [projection, wallProject, state, rho]
  | energy => simp [projection, wallProject, state, energy, dot]
  | momentum k =>
    fin_cases k <;>
      simp [projection, wallProject, state, wallDefect, mom, dot, Fin.sum_univ_two, opReal]

def wallImpl (oldOp resOp truncOp : Op) (v : WallVisit N) (s : BoundaryState N) : BoundaryState N :=
  if v.owned then
    {s with old := Function.update s.old v.point (projection oldOp v.normal.vector (s.old v.point)),
            residual := Function.update s.residual v.point (projection resOp v.normal.vector (s.residual v.point)),
            truncation := Function.update s.truncation v.point (projection truncOp v.normal.vector (s.truncation v.point))}
  else s

theorem wall_source_contract (v : WallVisit N) (s : BoundaryState N) :
    wallImpl Generated.wallOld_original Generated.wallRes_original Generated.wallTrunc_original v s =
    wallSpec v s := by
  have a : Generated.wallOld_original = .sub := by decide
  have b : Generated.wallRes_original = .sub := by decide
  have c : Generated.wallTrunc_original = .sub := by decide
  simp only [a, b, c, wallImpl, projection_correct, wallSpec]

/-- Separate arithmetic model of the production characteristic reconstruction.
Real sqrt/rpow do not assert floating-point library correctness. -/
def farPrimitiveImpl (g : IdealGas) (n : Vec 2) (inside free : Primitive 2) : Primitive 2 :=
  let q := dot free.velocity n
  let rp := if q > -sound g free then dot inside.velocity n + 2.0 * sound g inside / (g.gamma - 1)
            else dot free.velocity n + 2.0 * sound g free / (g.gamma - 1)
  let rm := if q > sound g free then dot inside.velocity n - 2.0 * sound g inside / (g.gamma - 1)
            else dot free.velocity n - 2.0 * sound g free / (g.gamma - 1)
  let vn := 0.5 * (rp + rm)
  let c := 0.25 * (rp - rm) * (g.gamma - 1)
  let vel := if q > 0 then (fun j => inside.velocity j + (vn - dot inside.velocity n) * n j)
             else (fun j => free.velocity j + (vn - dot free.velocity n) * n j)
  let e := if q > 0 then entropy g inside else entropy g free
  let r := Real.rpow (e * c * c / g.gamma) (1 / (g.gamma - 1))
  ⟨r, vel, r * c * c / g.gamma⟩

theorem far_reconstruction_correct (g : IdealGas) (n : Vec 2) (inside free : Primitive 2) :
    farPrimitiveImpl g n inside free = farPrimitive g n inside free := by
  have hhalf : ∀ x : ℝ, 0.5 * x = x / 2 := by intro x; ring
  have hquarter : ∀ x y : ℝ, 0.25 * x * y = x * y / 4 := by intro x y; ring
  unfold farPrimitiveImpl farPrimitive characteristics rPlus rMinus advected
  simp only [show (2.0 : ℝ) = 2 by norm_num, hhalf, hquarter]
  split_ifs <;> rfl

theorem far_normal_velocity (g : IdealGas) (n : UnitNormal 2) (inside free : Primitive 2) :
    dot (farPrimitive g n.vector inside free).velocity n.vector =
      ((characteristics g n.vector inside free).1 + (characteristics g n.vector inside free).2) / 2 := by
  simp only [farPrimitive, dot_add_left, dot_scale_left, n.unit, mul_one]
  ring

theorem far_tangential_velocity (g : IdealGas) (n t : Vec 2) (inside free : Primitive 2)
    (ht : dot n t = 0) :
    dot (farPrimitive g n inside free).velocity t = dot (advected n inside free).velocity t := by
  simp [farPrimitive, dot_add_left, dot_scale_left, ht]

theorem riemann_reconstruction (g : IdealGas) (rp rm : ℝ) :
    (rp + rm) / 2 + 2 * ((rp - rm) * (g.gamma - 1) / 4) / (g.gamma - 1) = rp ∧
    (rp + rm) / 2 - 2 * ((rp - rm) * (g.gamma - 1) / 4) / (g.gamma - 1) = rm := by
  have h : g.gamma - 1 ≠ 0 := ne_of_gt (sub_pos.mpr g.gamma_gt_one)
  constructor <;> field_simp <;> ring_nf

/-- Matrix dissipative accumulation in source j-then-k order. -/
def dissipationLoop (P B : Fin 4 → Fin 4 → ℝ) (lambda delta : Fin 4 → ℝ)
    (area : ℝ) (central : Fin 4 → ℝ) : Fin 4 → ℝ :=
  fun i => (List.ofFn (fun j : Fin 4 =>
    (List.ofFn (fun k : Fin 4 => P i k * lambda k * B k j)).foldl (· + ·) 0 * delta j)).foldl
      (fun flux x => flux - (1 - (1 / 2 : ℝ)) * x * area) (central i)

theorem dissipation_loop_correct (P B : Fin 4 → Fin 4 → ℝ) (lambda delta : Fin 4 → ℝ)
    (area : ℝ) (central : Fin 4 → ℝ) :
    dissipationLoop P B lambda delta area central =
      fun i => central i - area / 2 * ∑ j, (∑ k, P i k * lambda k * B k j) * delta j := by
  funext i
  simp [dissipationLoop, List.ofFn_succ, Fin.sum_univ_succ]
  ring

/-- A usable sufficient domain condition: admissible primitive inputs and nonzero
reconstructed acoustic speed imply admissible ghost data. This avoids assuming the
resulting density merely to satisfy the representation bridge. -/
theorem far_primitive_admissible (g : IdealGas) (n : Vec 2) (inside free : Primitive 2)
    (hi : PrimitiveAdmissible inside) (hf : PrimitiveAdmissible free)
    (hc : ((characteristics g n inside free).1 - (characteristics g n inside free).2) *
      (g.gamma - 1) / 4 ≠ 0) : PrimitiveAdmissible (farPrimitive g n inside free) := by
  let W := advected n inside free
  let c := ((characteristics g n inside free).1 - (characteristics g n inside free).2) *
    (g.gamma - 1) / 4
  have hw : PrimitiveAdmissible W := by
    dsimp [W, advected]
    split_ifs <;> assumption
  have hg : 0 < g.gamma := lt_trans (by norm_num) g.gamma_gt_one
  have he : 0 < entropy g W := div_pos (Real.rpow_pos_of_pos hw.1 _) hw.2
  have hs : 0 < c * c := mul_self_pos.mpr hc
  have hb : 0 < entropy g W * c * c / g.gamma := by
    have hh := mul_pos he hs
    apply div_pos _ hg
    nlinarith only [hh]
  have hr := Real.rpow_pos_of_pos hb (1 / (g.gamma - 1))
  change 0 < Real.rpow (entropy g W * c * c / g.gamma) (1 / (g.gamma - 1)) ∧ _
  refine ⟨hr, ?_⟩
  change 0 < Real.rpow (entropy g W * c * c / g.gamma) (1 / (g.gamma - 1)) * c * c / g.gamma
  apply div_pos _ hg
  simpa only [Real.rpow_eq_pow, mul_assoc] using mul_pos hr hs

/-- Production reconstructs energy from primitive enthalpy. The nonzero-density
premise is a genuine domain condition, not an assumed update result. -/
def sourceConservative (H : ℝ) (W : Primitive 2) : Fin 4 → ℝ :=
  ![W.density, W.density * W.velocity 0, W.density * W.velocity 1,
    W.density * (H - W.pressure / W.density)]

theorem source_conservative_correct (g : IdealGas) (W : Primitive 2) (h : W.density ≠ 0) :
    sourceConservative (totalEnthalpy g W) W = components (toConservative g W) := by
  funext c
  fin_cases c <;> simp [sourceConservative, components, channel, toConservative, state,
    totalEnthalpy, specificEnergy]
  have hg : g.gamma - 1 ≠ 0 := ne_of_gt (sub_pos.mpr g.gamma_gt_one)
  field_simp [h, hg]
  ring

/-- Production projected flux uses the stored primitive enthalpy. -/
def storedFlux (H : ℝ) (normal : Vec 2) (W : Primitive 2) : Fin 4 → ℝ :=
  let q := dot W.velocity normal
  ![W.density * q, W.density * q * W.velocity 0 + W.pressure * normal 0,
    W.density * q * W.velocity 1 + W.pressure * normal 1, W.density * H * q]

theorem stored_flux_correct (g : IdealGas) (normal : Vec 2) (W : Primitive 2) :
    storedFlux (totalEnthalpy g W) normal W = physicalFlux g normal W := rfl

/-- Closed serial scalar-Roe implementation model. Eigenvector entries, primitive
loads and sqrt/pow are manual source representations, outside the fragment parser. -/
def roeImplementation (g : IdealGas) (n : Vec 2) (area : ℝ) (L R : Primitive 2) (HL HR : ℝ) : Fin 4 → ℝ :=
  let ratio := Real.sqrt |R.density / L.density|
  let r := ratio * L.density
  let v := fun j => (ratio * R.velocity j + L.velocity j) / (ratio + 1)
  let H := (ratio * HR + HL) / (ratio + 1)
  let c2 := (g.gamma - 1) * (H - dot v v / 2)
  if c2 ≤ 0 then 0 else
  let c := Real.sqrt c2
  let q := dot v n
  dissipationLoop (roeP g r c v n) (roeB g r c v n) ![|q|, |q|, |q+c|, |q-c|]
    (fun j => sourceConservative HR R j - sourceConservative HL L j) area
    (fun i => 0.5 * (storedFlux HL (fun j => area * n j) L i + storedFlux HR (fun j => area * n j) R i))

theorem roe_model_refinement (g : IdealGas) (n : Vec 2) (area : ℝ) (L R : Primitive 2)
    (hl : L.density ≠ 0) (hr : R.density ≠ 0) :
    roeImplementation g n area L R (totalEnthalpy g L) (totalEnthalpy g R) = boundaryRoe g n area L R := by
  unfold roeImplementation boundaryRoe
  simp only [stored_flux_correct]
  rw [source_conservative_correct g L hl, source_conservative_correct g R hr]
  split_ifs
  · rfl
  · rw [dissipation_loop_correct]
    funext i
    ring

/-- Required input-domain relation for the boundary reconstruction. Positivity and all
finite machine-operation domains are stronger outstanding caller obligations. -/
def FarReady (g : IdealGas) (free : Primitive 2) (far : List (FarVisit N)) : Prop :=
  ∀ v ∈ far, v.owned = true → v.inside.density ≠ 0 ∧
    (farPrimitive g v.normal.vector v.inside free).density ≠ 0 ∧
    v.insideEnthalpy = totalEnthalpy g v.inside

/-- The boundary call is parsed; characteristic/Roe arithmetic remains a disclosed
manual representation. No unconstrained boundary callback occurs in this model. -/
def farImpl (op : Op) (g : IdealGas) (free : Primitive 2) (v : FarVisit N)
    (s : BoundaryState N) : BoundaryState N :=
  if v.owned then
    let W := farPrimitiveImpl g v.normal.vector v.inside free
    let f := fromComponents (roeImplementation g v.normal.vector v.area v.inside W v.insideEnthalpy (totalEnthalpy g W))
    {s with residual := Function.update s.residual v.point (fun c => opReal op (s.residual v.point c) (f c)),
            cache := Function.update s.cache v.cacheIndex (some (ghost g W))}
  else s

theorem far_source_call_contract (g : IdealGas) (free : Primitive 2) (v : FarVisit N)
    (s : BoundaryState N) (ready : v.owned = true → v.inside.density ≠ 0 ∧
      (farPrimitive g v.normal.vector v.inside free).density ≠ 0 ∧
      v.insideEnthalpy = totalEnthalpy g v.inside) :
    farImpl Generated.farAdd_original g free v s = farSpec g free v s := by
  have h : Generated.farAdd_original = .add := by decide
  by_cases hv : v.owned = true
  · obtain ⟨hl, hr, hH⟩ := ready hv
    simp only [h, farImpl, far_reconstruction_correct, opReal, farSpec, hv, ite_true, hH,
      roe_model_refinement g v.normal.vector v.area v.inside _ hl hr]
    rfl
  · simp [farImpl, farSpec, hv]

def boundaryImpl (g : IdealGas) (free : Primitive 2) (far : List (FarVisit N))
    (walls : List (WallVisit N)) (s : BoundaryState N) : BoundaryState N :=
  walls.foldl (fun acc v => wallImpl Generated.wallOld_original Generated.wallRes_original
    Generated.wallTrunc_original v acc)
    (far.foldl (fun acc v => farImpl Generated.farAdd_original g free v acc) s)

theorem boundary_refinement (g : IdealGas) (free : Primitive 2) (far : List (FarVisit N))
    (walls : List (WallVisit N)) (s : BoundaryState N) (ready : FarReady g free far) :
    boundaryImpl g free far walls s = boundarySpec g free far walls s := by
  have hfar : ∀ a : BoundaryState N,
      far.foldl (fun acc v => farImpl Generated.farAdd_original g free v acc) a =
      far.foldl (fun acc v => farSpec g free v acc) a := by
    induction far with
    | nil => intro a; rfl
    | cons v vs ih =>
      intro a
      simp only [List.foldl_cons]
      rw [far_source_call_contract g free v a (ready v (List.mem_cons_self))]
      exact ih (fun w hw => ready w (List.mem_cons_of_mem v hw)) _
  simp only [boundaryImpl, wall_source_contract, hfar, boundarySpec]

theorem far_frame (g : IdealGas) (free : Primitive 2) (v : FarVisit N) (s : BoundaryState N) :
    (farSpec g free v s).current = s.current ∧ (farSpec g free v s).old = s.old ∧
    (farSpec g free v s).truncation = s.truncation ∧
    (∀ p, p ≠ v.point → (farSpec g free v s).residual p = s.residual p) ∧
    (∀ k, k ≠ v.cacheIndex → (farSpec g free v s).cache k = s.cache k) := by
  by_cases h : v.owned <;> simp [farSpec, h]
  constructor
  · intro p hp; exact Function.update_of_ne hp _ _
  · intro k hk; exact Function.update_of_ne hk _ _

theorem wall_frame (v : WallVisit N) (s : BoundaryState N) :
    (wallSpec v s).current = s.current ∧ (wallSpec v s).cache = s.cache ∧
    ∀ p, p ≠ v.point → (wallSpec v s).old p = s.old p ∧
      (wallSpec v s).residual p = s.residual p ∧ (wallSpec v s).truncation p = s.truncation p := by
  by_cases h : v.owned <;> simp [wallSpec, h]
  intro p hp
  simp [Function.update_of_ne hp]

theorem wall_three_constraints (v : WallVisit N) (s : BoundaryState N) (h : v.owned = true) :
    Impermeable v.normal.vector 0 ((wallSpec v s).old v.point) ∧
    Impermeable v.normal.vector 0 ((wallSpec v s).residual v.point) ∧
    Impermeable v.normal.vector 0 ((wallSpec v s).truncation v.point) := by
  simp only [wallSpec, h, ite_true, Function.update_self]
  exact ⟨wall_project_impermeable _ _ v.normal.unit _,
    wall_project_impermeable _ _ v.normal.unit _, wall_project_impermeable _ _ v.normal.unit _⟩

theorem wall_old_admissible (v : WallVisit N) (s : BoundaryState N) (h : v.owned = true)
    (ha : Admissible (s.old v.point)) : Admissible ((wallSpec v s).old v.point) := by
  simpa [wallSpec, h] using wall_preserves_admissibility v.normal.vector 0 v.normal.unit _ ha

theorem wall_mass_energy_frame (v : WallVisit N) (s : BoundaryState N) (p : Fin N) :
    rho ((wallSpec v s).old p) = rho (s.old p) ∧
    energy ((wallSpec v s).old p) = energy (s.old p) ∧
    rho ((wallSpec v s).residual p) = rho (s.residual p) ∧
    energy ((wallSpec v s).residual p) = energy (s.residual p) ∧
    rho ((wallSpec v s).truncation p) = rho (s.truncation p) ∧
    energy ((wallSpec v s).truncation p) = energy (s.truncation p) := by
  by_cases hv : v.owned = true <;> by_cases hp : p = v.point <;>
    simp [wallSpec, hv, hp, Function.update_of_ne, wallProject, rho, energy, state, dot]

/-- Last owned wall visit at this point; incompatible normals need not commute. -/
theorem projected_update_impermeable (v : WallVisit N) (s : BoundaryState N)
    (h : v.owned = true) (vol dt : Fin N → ℝ) :
    Impermeable v.normal.vector 0 (updateSpec vol dt (wallSpec v s) v.point) := by
  obtain ⟨ho, hr, ht⟩ := wall_three_constraints v s h
  have hadd : Impermeable v.normal.vector 0
      ((wallSpec v s).residual v.point + (wallSpec v s).truncation v.point) := by
    convert wall_update_preserves v.normal.vector 0 _ _ 1 hr ht using 1
    simp [conservativeUpdate]
  convert wall_update_preserves v.normal.vector 0 _ _ (-(dt v.point / vol v.point)) ho hadd using 1
  funext c
  simp [updateSpec, conservativeUpdate]
  ring

theorem boundary_current_frame (g : IdealGas) (free : Primitive 2) (far : List (FarVisit N))
    (walls : List (WallVisit N)) (s : BoundaryState N) :
    (boundarySpec g free far walls s).current = s.current := by
  have fw : ∀ ws (a : BoundaryState N),
      (List.foldl (fun acc v => wallSpec v acc) a ws).current = a.current := by
    intro ws
    induction ws with
    | nil => intro a; rfl
    | cons v vs ih => intro a; rw [List.foldl_cons, ih, (wall_frame v a).1]
  have ff : ∀ fs (a : BoundaryState N),
      (List.foldl (fun acc v => farSpec g free v acc) a fs).current = a.current := by
    intro fs
    induction fs with
    | nil => intro a; rfl
    | cons v vs ih => intro a; rw [List.foldl_cons, ih, (far_frame g free v a).1]
  exact (fw walls _).trans (ff far s)

theorem unowned_far_frame (g : IdealGas) (free : Primitive 2) (v : FarVisit N)
    (s : BoundaryState N) (h : v.owned = false) : farSpec g free v s = s := by
  simp [farSpec, h]

theorem unowned_wall_frame (v : WallVisit N) (s : BoundaryState N) (h : v.owned = false) :
    wallSpec v s = s := by
  simp [wallSpec, h]

end SU2Boundary
