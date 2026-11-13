import SU2Interior.Geometry

noncomputable section
namespace SU2Interior
open SU2 SU2Boundary SU2BoundaryMemory SU2Initialization SU2Preprocessing
variable {M E : Nat}

def projected (P : Primitives M) (p : Fin M) (n : Vec 2) : ℝ := P p 1*n 0+P p 2*n 1

def edgeSpeed (P : Primitives M) (g : Geometry M E) (p : Fin M) (nb : Fin M × Fin E) : ℝ :=
  |(projected P p (g.normal nb.2) + projected P nb.1 (g.normal nb.2))/2| +
    ((P p 6+P nb.1 6)/2)*faceArea (g.normal nb.2)

def boundarySpeed (P : Primitives M) (b : BoundaryFace M) : ℝ :=
  |projected P b.point b.normal|+P b.point 6*faceArea b.normal

def boundaryTerm (P : Primitives M) (b : BoundaryFace M) (p : Fin M) : ℝ :=
  if b.owned && b.contributes && decide (b.point=p) then boundarySpeed P b else 0

def boundarySum (P : Primitives M) (bs : List (BoundaryFace M)) (p : Fin M) : ℝ :=
  (bs.map (fun b => boundaryTerm P b p)).sum

def neighborSum (P : Primitives M) (g : Geometry M E) (p : Fin M) : ℝ :=
  ((g.neighbors p).map (edgeSpeed P g p)).sum

def eigenvalue (P : Primitives M) (g : Geometry M E) (bs : List (BoundaryFace M)) (p : Fin M) : ℝ :=
  neighborSum P g p + boundarySum P bs p

/-- Steady local pseudo-time, not the physical timestep of a time-accurate method. -/
def localPseudoStep (volume cfl spectralSum cap : ℝ) : ℝ :=
  if volume = 0 then 0 else min (cfl*volume/spectralSum) cap

theorem zero_volume_step (cfl lam cap : ℝ) : localPseudoStep 0 cfl lam cap = 0 := by simp [localPseudoStep]

theorem step_positive (vol cfl lam cap : ℝ) (hv : 0 < vol) (hc : 0 < cfl)
    (hl : 0 < lam) (hcap : 0 < cap) : 0 < localPseudoStep vol cfl lam cap := by
  simp only [localPseudoStep,if_neg (ne_of_gt hv)]
  exact lt_min (div_pos (mul_pos hc hv) hl) hcap

theorem step_cap (vol cfl lam cap : ℝ) (hv : vol ≠ 0) : localPseudoStep vol cfl lam cap ≤ cap := by
  simp only [localPseudoStep,if_neg hv]; exact min_le_right _ _

theorem spectral_nonnegative (P : Primitives M) (g : Geometry M E) (bs : List (BoundaryFace M))
    (sound : ∀ p, 0 ≤ P p 6) (p : Fin M) : 0 ≤ eigenvalue P g bs p := by
  apply add_nonneg
  · apply List.sum_nonneg
    intro x hx
    obtain ⟨nb,_,rfl⟩ := List.mem_map.mp hx
    exact add_nonneg (abs_nonneg _) (mul_nonneg (div_nonneg (add_nonneg (sound _) (sound _)) (by norm_num)) (area_nonnegative _))
  · apply List.sum_nonneg
    intro x hx
    obtain ⟨b,_,rfl⟩ := List.mem_map.mp hx
    unfold boundaryTerm
    split_ifs
    · exact add_nonneg (abs_nonneg _) (mul_nonneg (sound _) (area_nonnegative _))
    · exact le_refl _

/-- Algebraic speed bound used by the time estimator. This does not identify its
arithmetic mean of endpoint sounds with the different Roe-averaged sound. -/
theorem acoustic_speed_identity (q c : ℝ) (hc : 0≤c) : max |q+c| |q-c|=|q|+c := by
  by_cases hq : 0≤q
  · have plus : 0≤q+c := add_nonneg hq hc
    by_cases minus : 0≤q-c
    · rw [abs_of_nonneg plus,abs_of_nonneg minus,abs_of_nonneg hq,max_eq_left (by linarith)]
    · rw [abs_of_nonneg plus,abs_of_neg (lt_of_not_ge minus),abs_of_nonneg hq,max_eq_left (by linarith)]
  · have qneg : q<0 := lt_of_not_ge hq
    have minus : q-c<0 := by linarith
    by_cases plus : 0≤q+c
    · rw [abs_of_nonneg plus,abs_of_neg minus,abs_of_neg qneg,max_eq_right (by linarith)]
      ring
    · rw [abs_of_neg (lt_of_not_ge plus),abs_of_neg minus,abs_of_neg qneg,max_eq_right (by linarith)]
      ring

end SU2Interior
