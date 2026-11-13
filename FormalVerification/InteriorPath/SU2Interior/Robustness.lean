import SU2Interior.Connected

noncomputable section
namespace SU2Interior
open SU2 SU2Source SU2Euler SU2Boundary SU2BoundaryMemory SU2Initialization SU2Preprocessing
variable {M E : Nat}

def OldAdmissible (s : BoundaryState M) : Prop := ∀ p, Admissible (s.old p)

theorem processed_snapshot_admissible (g : IdealGas) (d : Data M)
    (T : Residual M 2) (cache : Nat → Option Ghost) (backups : ∀ p, Admissible (d.old p)) :
    OldAdmissible (snapshotState (preprocessingState (modelPoints g (points M) d 0).1 T cache)) :=
  complete_admissible g d backups

theorem interior_old_admissible (g : IdealGas) (geo : Geometry M E) (P : Primitives M)
    (ready : GeometryReady geo) (s : BoundaryState M) (h : OldAdmissible s) :
    OldAdmissible (interiorState g geo P ready s) := h

theorem far_old_admissible (g : IdealGas) (free : Primitive 2) (v : FarVisit M)
    (s : BoundaryState M) (h : OldAdmissible s) : OldAdmissible (farSpec g free v s) := by
  cases hv : v.owned <;> simpa [farSpec,hv] using h

theorem wall_old_admissible (v : WallVisit M) (s : BoundaryState M) (h : OldAdmissible s) :
    OldAdmissible (wallSpec v s) := by
  intro p
  by_cases hv : v.owned=true
  · by_cases eq : p=v.point
    · subst p
      simpa [wallSpec,hv] using wall_preserves_admissibility v.normal.vector 0 v.normal.unit (s.old v.point) (h v.point)
    · simpa [wallSpec,hv,Function.update_of_ne eq] using h p
  · simpa [wallSpec,hv] using h p

theorem boundary_old_admissible (g : IdealGas) (free : Primitive 2)
    (far : List (FarVisit M)) (walls : List (WallVisit M)) (s : BoundaryState M)
    (h : OldAdmissible s) : OldAdmissible (boundarySpec g free far walls s) := by
  have ff : ∀ (fs : List (FarVisit M)) (t : BoundaryState M), OldAdmissible t →
      OldAdmissible (fs.foldl (fun acc v => farSpec g free v acc) t) := by
    intro fs
    induction fs with
    | nil => intro t ht; exact ht
    | cons f fs ih => intro t ht; exact ih _ (far_old_admissible g free f t ht)
  have ww : ∀ (ws : List (WallVisit M)) (t : BoundaryState M), OldAdmissible t →
      OldAdmissible (ws.foldl (fun acc v => wallSpec v acc) t) := by
    intro ws
    induction ws with
    | nil => intro t ht; exact ht
    | cons v ws ih => intro t ht; exact ih _ (wall_old_admissible v t ht)
  exact ww walls _ (ff far s h)

/-- Explicit update writes current only. The newly computed current need NOT be
admissible; that numerical-method limitation does not invalidate this old-buffer frame. -/
theorem update_old_frame {N : Nat} (s : BoundaryState N) (vol dt : Fin N → ℝ)
    (mb mu : Mem) (h : LoopContract s vol dt mb mu) (p : Fin N) (c : Fin 4) :
    mu (flowAddr 1 p c)=mb (flowAddr 1 p c) := by
  apply h.2
  intro cell eq
  have bad := congrArg Prod.fst eq
  norm_num [flowAddr,destination] at bad

/-- A conditional exact-real reachability invariant, not a claim about NaN/Inf acceptance
or floating-point cancellation. Arbitrary inadmissible trial currents remain permitted. -/
theorem next_backup_admissible (g : IdealGas) (d : Data M) (T : Residual M 2)
    (cache : Nat → Option Ghost) (geo : Geometry M E) (ready : GeometryReady geo)
    (free : Primitive 2) (far : List (FarVisit M)) (walls : List (WallVisit M))
    (backups : ∀ p, Admissible (d.old p)) :
    OldAdmissible (boundarySpec g free far walls
      (interiorState g geo (modelPoints g (points M) d 0).1.primitive ready
        (snapshotState (preprocessingState (modelPoints g (points M) d 0).1 T cache)))) :=
  boundary_old_admissible g free far walls _
    (interior_old_admissible g geo _ ready _ (processed_snapshot_admissible g d T cache backups))

end SU2Interior
