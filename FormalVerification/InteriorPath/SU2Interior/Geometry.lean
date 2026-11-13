import SU2Preprocessing
import Mathlib.Data.List.Induction

set_option maxRecDepth 100000
set_option maxHeartbeats 8000000
noncomputable section
namespace SU2Interior
open SU2 SU2Source SU2Boundary SU2BoundaryMemory SU2Initialization SU2Preprocessing
open scoped BigOperators

/-- Initial read-only geometry object. Its construction from mesh input remains a
separate source obligation; valid indexing here is explicit, never a post-residual premise. -/
structure Geometry (M E : Nat) where
  left : Fin E → Fin M
  right : Fin E → Fin M
  ordered : ∀ e, (left e).val < (right e).val
  normal : Fin E → Vec 2
  neighbors : Fin M → List (Fin M × Fin E)

/-- Source adjacency obligation: neighbors enumerate incident edges, with the opposite
endpoint. Proving this for CGeometry::SetEdges is separate from traversal correctness. -/
def IncidenceValid {M E : Nat} (g : Geometry M E) : Prop :=
  ∀ p, (g.neighbors p).Perm ((List.ofFn (fun e : Fin E => e)).filterMap fun e =>
    if p = g.left e then some (g.right e,e) else if p = g.right e then some (g.left e,e) else none)

structure BoundaryFace (M : Nat) where
  point : Fin M
  marker : Nat
  vertex : Nat
  normal : Vec 2
  owned : Bool
  /-- Selected timestep filter: far-field true, Euler-wall false. -/
  contributes : Bool

def faceArea (n : Vec 2) : ℝ := Real.sqrt (dot n n)

def dualNormal (a b : Vec 2) : Vec 2 := ![b 1-a 1, -(b 0-a 0)]
def dualPieceVolume (point midpoint centroid : Vec 2) : ℝ :=
  (1/2 : ℝ) * |(centroid 0-point 0)*(midpoint 1-point 1) -
    (centroid 1-point 1)*(midpoint 0-point 0)|

theorem dual_normal_reversal (a b : Vec 2) : dualNormal b a = -dualNormal a b := by
  funext i; fin_cases i <;> simp [dualNormal]

theorem dual_piece_nonnegative (p m c : Vec 2) : 0 ≤ dualPieceVolume p m c :=
  mul_nonneg (by norm_num) (abs_nonneg _)

theorem dual_piece_positive (p m c : Vec 2)
    (noncollinear : (c 0-p 0)*(m 1-p 1)-(c 1-p 1)*(m 0-p 0) ≠ 0) :
    0 < dualPieceVolume p m c := mul_pos (by norm_num) (abs_pos.mpr noncollinear)

def repairNormal (eps : ℝ) (n : Vec 2) : Vec 2 :=
  if dot n n = 0 then ![eps*eps,0] else n

theorem area_nonnegative (n : Vec 2) : 0 ≤ faceArea n := Real.sqrt_nonneg _

theorem dot_self_nonnegative (n : Vec 2) : 0 ≤ dot n n := by
  simp only [dot,Fin.sum_univ_two]
  nlinarith [sq_nonneg (n 0),sq_nonneg (n 1)]

theorem repaired_area_positive (eps : ℝ) (he : eps ≠ 0) (n : Vec 2) :
    0 < faceArea (repairNormal eps n) := by
  unfold faceArea repairNormal
  split_ifs with hz
  · apply Real.sqrt_pos.mpr
    simp only [dot,Fin.sum_univ_two,Matrix.cons_val_zero,Matrix.cons_val_one,zero_mul,add_zero]
    exact mul_self_pos.mpr (mul_ne_zero he he)
  · exact Real.sqrt_pos.mpr (lt_of_le_of_ne (dot_self_nonnegative n) (Ne.symm hz))

theorem normalized_area (n : Vec 2) (hn : 0 < faceArea n) :
    dot (fun i => n i / faceArea n) (fun i => n i / faceArea n) = 1 := by
  have square : faceArea n * faceArea n = dot n n := by
    simpa [faceArea,pow_two] using Real.sq_sqrt (dot_self_nonnegative n)
  simp only [dot,Fin.sum_univ_two] at square ⊢
  field_simp [ne_of_gt hn]
  nlinarith

/-- One serial color containing std::iota(0,nEdge). -/
def naturalEdges (E : Nat) : List (Fin E) := List.ofFn (fun e => e)

theorem natural_edge_coverage {E : Nat} (e : Fin E) : e ∈ naturalEdges E := List.mem_ofFn.mpr ⟨e,rfl⟩

theorem natural_edge_order (E : Nat) : (naturalEdges E).Pairwise (fun a b => a.val < b.val) := by
  simp only [naturalEdges,List.pairwise_ofFn]; intro i j h; exact h

theorem natural_edge_unique (E : Nat) : (naturalEdges E).Nodup := by
  apply List.Pairwise.imp _ (natural_edge_order E)
  intro a b h eq; subst b; exact Nat.lt_irrefl _ h

theorem endpoint_distinct {M E : Nat} (g : Geometry M E) (e : Fin E) : g.left e ≠ g.right e := by
  intro eq; have h := g.ordered e; rw [eq] at h; exact Nat.lt_irrefl _ h

/-- Actual argument roles of CEdge::GetVolume(point,midpoint,centroid) at the call site. -/
def sourceDualVolume (point midpoint centroid : Vec 2) : ℝ :=
  (1/2 : ℝ)*|(midpoint 0-centroid 0)*(point 1-centroid 1)-
    (midpoint 1-centroid 1)*(point 0-centroid 0)|

theorem source_dual_volume_correct (p m c : Vec 2) : sourceDualVolume p m c=dualPieceVolume p m c := by
  unfold sourceDualVolume dualPieceVolume
  congr 2
  ring

def sourceAddNormal (old a b : Vec 2) : Vec 2 :=
  ![old 0+(b 1-a 1),old 1-(b 0-a 0)]

theorem source_add_normal_correct (old a b : Vec 2) : sourceAddNormal old a b=old+dualNormal a b := by
  funext i
  fin_cases i
  · rfl
  · exact sub_eq_add_neg _ _

theorem source_normal_fold_correct (pieces : List (Vec 2 × Vec 2)) (old : Vec 2) :
    pieces.foldl (fun normal piece => sourceAddNormal normal piece.1 piece.2) old =
      old+(pieces.map (fun piece => dualNormal piece.1 piece.2)).sum := by
  induction pieces generalizing old with
  | nil => simp
  | cons piece pieces ih =>
    rw [List.foldl_cons, ih]
    rw [source_add_normal_correct]
    simp only [List.map_cons, List.sum_cons]
    exact add_assoc old (dualNormal piece.1 piece.2) ((pieces.map (fun item => dualNormal item.1 item.2)).sum)

end SU2Interior
