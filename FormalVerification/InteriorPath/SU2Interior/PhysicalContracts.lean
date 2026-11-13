import SU2Interior.Controls

noncomputable section
namespace SU2Interior
open SU2 SU2Source SU2Boundary SU2BoundaryMemory SU2Initialization SU2Preprocessing
variable {M E K : Nat} {vertices : Fin K → Nat}

theorem processed_sound_positive (g : IdealGas) (d : Data M)
    (backups : ∀ p, Admissible (d.old p)) (p : Fin M) :
    0 < (modelPoints g (points M) d 0).1.primitive p 6 := by
  rw [(complete_primitive_correct g d backups p).2]
  change 0 < Real.sqrt (g.gamma*g.gasConstant*temperature g (toPrimitive g (selectedState (d.current p) (d.old p))))
  apply Real.sqrt_pos.mpr
  have gamma : 0<g.gamma := lt_trans (by norm_num) g.gamma_gt_one
  exact mul_pos (mul_pos gamma g.gasConstant_pos)
    (temperature_positive g _ (primitive_admissible g _ (selected_admissible _ _ (backups p))))

theorem edge_speed_positive (P : Primitives M) (geo : Geometry M E) (ready : GeometryReady geo)
    (sound : ∀ p, 0<P p 6) (p : Fin M) (nb : Fin M × Fin E) : 0<edgeSpeed P geo p nb :=
  add_pos_of_nonneg_of_pos (abs_nonneg _) (mul_pos (div_pos (add_pos (sound p) (sound nb.1)) (by norm_num)) (ready nb.2))

theorem boundary_sum_nonnegative (P : Primitives M) (bs : List (BoundaryFace M))
    (sound : ∀ p, 0≤P p 6) (p : Fin M) : 0≤boundarySum P bs p := by
  apply List.sum_nonneg
  intro x hx
  obtain ⟨b,_,rfl⟩ := List.mem_map.mp hx
  unfold boundaryTerm
  split_ifs
  · exact add_nonneg (abs_nonneg _) (mul_nonneg (sound _) (area_nonnegative _))
  · exact le_refl _

/-- Nonempty adjacency is a geometric input obligation, not a premise asserting the
computed spectral sum. Native fixture checks do not prove this for all parsed meshes. -/
theorem spectral_positive (P : Primitives M) (geo : Geometry M E) (bs : List (BoundaryFace M))
    (ready : GeometryReady geo) (sound : ∀ p, 0<P p 6) (p : Fin M)
    (incident : geo.neighbors p ≠ []) : 0<eigenvalue P geo bs p := by
  apply add_pos_of_pos_of_nonneg _ (boundary_sum_nonnegative P bs (fun q => le_of_lt (sound q)) p)
  cases hn : geo.neighbors p with
  | nil => exact False.elim (incident hn)
  | cons nb rest =>
    change 0 < ((geo.neighbors p).map (edgeSpeed P geo p)).sum
    rw [hn,List.map_cons,List.sum_cons]
    apply add_pos_of_pos_of_nonneg (edge_speed_positive P geo ready sound p nb)
    apply List.sum_nonneg
    intro x hx
    obtain ⟨b,_,rfl⟩ := List.mem_map.mp hx
    exact le_of_lt (edge_speed_positive P geo ready sound p b)

/-- Physical guarantee derived from admissible backups and live mesh incidence,
without assuming a correctly computed timestep or positive final spectral buffer. -/
theorem configured_step_positive (g : IdealGas) (d : Data M) (geo : Geometry M E)
    (bs : List (BoundaryFace M)) (ready : GeometryReady geo)
    (backups : ∀ p, Admissible (d.old p)) (vol cfl cap : ℝ)
    (hv : 0<vol) (hc : 0<cfl) (hm : 0<cap) (p : Fin M)
    (incident : geo.neighbors p ≠ []) :
    0 < localPseudoStep vol cfl (eigenvalue (modelPoints g (points M) d 0).1.primitive geo bs p) cap :=
  step_positive vol cfl _ cap hv hc
    (spectral_positive _ geo bs ready (processed_sound_positive g d backups) p incident) hm

theorem auxiliary_byte_bounds (bounds : Bounds M M K vertices) (p : Fin M) :
    8*p.val+7<2^64 := by
  have size : 8*(M*1)≤2^64 := by have h := bounds.points; omega
  simpa using byte_offset_bound M 1 p.val 0 p.isLt (by decide) size

theorem auxiliary_blocks_distinct (K : Nat) (p q : Fin M) : lambdaAddr K p≠cflAddr K q := by
  intro eq
  have h := congrArg Prod.fst eq
  unfold lambdaAddr cflAddr auxAddr auxBlock at h
  simp only at h
  omega

theorem auxiliary_cache_separation (site : Fin K) (p : Fin M) (v c slot : Nat) :
    auxAddr K slot p≠cacheAddr site.val v c := by
  intro eq
  have h := congrArg Prod.fst eq
  have bound := site.isLt
  unfold auxAddr auxBlock cacheAddr at h
  simp only at h
  omega

end SU2Interior
