import SU2Interior.Timestep

set_option maxRecDepth 100000
set_option maxHeartbeats 8000000
noncomputable section
namespace SU2Interior
open SU2 SU2Source SU2Boundary SU2BoundaryMemory SU2Initialization SU2Preprocessing
variable {M E : Nat}

theorem put_overwrite (m : Mem) (a : Address) (x y : ℝ) : put (put m a x) a y = put m a y := by
  funext q; by_cases h : q=a <;> simp [put,Function.update_apply,h]

def writeNodes (b : Nat) (f : Fin M → ℝ) : List (Fin M) → Mem → Mem
  | [], m => m
  | p :: ps, m => writeNodes b f ps (put m (b,p.val) (f p))

theorem write_nodes_value (b : Nat) (f : Fin M → ℝ) (ps : List (Fin M)) (m : Mem) (p : Fin M) :
    writeNodes b f ps m (b,p.val) = if p ∈ ps then some (f p) else m (b,p.val) := by
  induction ps generalizing m with
  | nil => simp [writeNodes]
  | cons q ps ih =>
    rw [writeNodes,ih]
    by_cases h : p ∈ ps
    · simp [h]
    · by_cases eq : p=q
      · subst p; simp [h,put]
      · have neq : (b,p.val) ≠ (b,q.val) := by intro e; exact eq (Fin.ext (congrArg Prod.snd e))
        simp [h,eq,put,Function.update_of_ne neq]

theorem write_nodes_frame (b : Nat) (f : Fin M → ℝ) (ps : List (Fin M)) (m : Mem) :
    Frame (ps.map (fun p => (b,p.val))) m (writeNodes b f ps m) := by
  induction ps generalizing m with
  | nil => exact frame_refl _ _
  | cons p ps ih =>
    simpa only [writeNodes,List.map_cons,List.singleton_append] using
      frame_trans (put_frame m (b,p.val) (f p)) (ih (put m (b,p.val) (f p)))

theorem write_nodes_other (b d : Nat) (ne : b ≠ d) (f : Fin M → ℝ)
    (ps : List (Fin M)) (m : Mem) (i : Nat) : writeNodes d f ps m (b,i) = m (b,i) := by
  apply write_nodes_frame
  intro mem
  obtain ⟨p,_,eq⟩ := List.mem_map.mp mem
  exact ne (congrArg Prod.fst eq.symm)

theorem write_nodes_primitive (b : Nat) (hb : b ≠ 8) (f : Fin M → ℝ)
    (ps : List (Fin M)) (m : Mem) (P : Primitives M) (h : PrimitiveRep m P) :
    PrimitiveRep (writeNodes b f ps m) P := by
  intro p c
  exact (write_nodes_other 8 b (Ne.symm hb) f ps m _).trans (h p c)

/-- Projection of CFVMFlowSolverBase initialization's actual all-point CFL stores. -/
def initializeCFL (K : Nat) (cfl : ℝ) (m : Mem) : Mem :=
  writeNodes (auxBlock K 1) (fun _ : Fin M => cfl) (points M) m

theorem initial_cfl_ready (K : Nat) (cfl : ℝ) (m : Mem) :
    ∀ p : Fin M, initializeCFL (M := M) K cfl m (cflAddr K p) = some cfl := by
  intro p; exact (write_nodes_value _ _ _ _ p).trans (if_pos (points_coverage p))

theorem neighbors_execute (K : Nat) (g : Geometry M E) (P : Primitives M) (p : Fin M)
    (ns : List (Fin M × Fin E)) (m : Mem) (x : ℝ)
    (h : PrimitiveRep m P) (hx : m (lambdaAddr K p) = some x) :
    executeNeighbors K g p ns m =
      some (if ns=[] then m else put m (lambdaAddr K p) (x+(ns.map (edgeSpeed P g p)).sum)) := by
  induction ns generalizing m x with
  | nil => simp [executeNeighbors]
  | cons nb ns ih =>
    have pp := scalar_primitive_frame (auxBlock K 0) (aux_not_primitive K 0) p (x+edgeSpeed P g p nb) m P h
    have read := read_edge_speed_correct m P h g p nb
    simp only [executeNeighbors,read,Option.bind_eq_bind,Option.some_bind,hx]
    have next : put m (lambdaAddr K p) (x+edgeSpeed P g p nb) (lambdaAddr K p) = some (x+edgeSpeed P g p nb) := by simp [put]
    have tail := ih (put m (lambdaAddr K p) (x+edgeSpeed P g p nb)) (x+edgeSpeed P g p nb) pp next
    rw [tail]
    cases ns with
    | nil => simp
    | cons b bs => simp [put_overwrite,add_assoc]

theorem neighbor_points_execute (K : Nat) (g : Geometry M E) (P : Primitives M)
    (ps : List (Fin M)) (m : Mem) (h : PrimitiveRep m P) :
    executeNeighborPoints K g ps m = some (writeNodes (auxBlock K 0) (neighborSum P g) ps m) := by
  induction ps generalizing m with
  | nil => rfl
  | cons p ps ih =>
    let z := put m (lambdaAddr K p) 0
    have hz : PrimitiveRep z P := scalar_primitive_frame _ (aux_not_primitive K 0) p 0 m P h
    have en := neighbors_execute K g P p (g.neighbors p) z 0 hz (by simp [z,put])
    have eq : (if g.neighbors p=[] then z else put z (lambdaAddr K p) (0+((g.neighbors p).map (edgeSpeed P g p)).sum)) =
        put m (lambdaAddr K p) (neighborSum P g p) := by
      by_cases empty : g.neighbors p=[]
      · simp [empty,z,neighborSum]
      · simp [empty,z,put_overwrite,neighborSum]
    rw [eq] at en
    dsimp only [z] at en
    simp only [executeNeighborPoints,en,Option.bind_eq_bind,Option.some_bind,writeNodes]
    exact ih (put m (lambdaAddr K p) (neighborSum P g p))
      (scalar_primitive_frame _ (aux_not_primitive K 0) p (neighborSum P g p) m P h)

def OwnedLambdaRep (K N : Nat) (m : Mem) (lam : Fin M → ℝ) : Prop :=
  ∀ p, p.val < N → m (lambdaAddr K p) = some (lam p)

def boundarySpeedFootprint (K : Nat) (bs : List (BoundaryFace M)) : List Address :=
  bs.filterMap (fun b => if b.owned && b.contributes then some (lambdaAddr K b.point) else none)

theorem boundary_speeds_execute (K N : Nat) (P : Primitives M) (bs : List (BoundaryFace M))
    (m : Mem) (lam : Fin M → ℝ) (hp : PrimitiveRep m P) (hl : OwnedLambdaRep K N m lam)
    (ownership : ∀ b ∈ bs, b.owned=true → b.point.val < N) :
    ∃ mf, executeBoundarySpeeds K bs m = some mf ∧
      PrimitiveRep mf P ∧ OwnedLambdaRep K N mf (fun p => lam p+boundarySum P bs p) ∧
      Frame (boundarySpeedFootprint K bs) m mf := by
  induction bs generalizing m lam with
  | nil => exact ⟨m,rfl,hp,by simpa [boundarySum] using hl,frame_refl _ _⟩
  | cons b bs ih =>
    have ownRest : ∀ v ∈ bs, v.owned=true → v.point.val < N := fun v hv => ownership v (List.mem_cons_of_mem b hv)
    by_cases active : (b.owned && b.contributes) = true
    · have both : b.owned=true ∧ b.contributes=true := by
        cases ho : b.owned <;> cases hc : b.contributes <;> simp_all
      have own := both.1
      let x := lam b.point+boundarySpeed P b
      let mn := put m (lambdaAddr K b.point) x
      have hn : PrimitiveRep mn P := scalar_primitive_frame _ (aux_not_primitive K 0) b.point x m P hp
      let ln := fun p => lam p+if p=b.point then boundarySpeed P b else 0
      have lnRep : OwnedLambdaRep K N mn ln := by
        intro p hpn
        by_cases eq : p=b.point
        · subst p; simp [mn,ln,x,put]
        · have neq : lambdaAddr K p ≠ lambdaAddr K b.point := by
            intro e; exact eq (Fin.ext (congrArg Prod.snd e))
          simpa [mn,put,Function.update_of_ne neq,ln,eq] using hl p hpn
      obtain ⟨mf,ef,hf,lf,ff⟩ := ih mn ln hn lnRep ownRest
      refine ⟨mf,?_,hf,?_,?_⟩
      · simp only [executeBoundarySpeeds,own,both.2,Bool.and_self,ite_true,read_boundary_speed_correct m P hp,
          Option.bind_eq_bind,Option.some_bind,hl b.point (ownership b (by simp) own)]
        simpa only [pure,Option.bind_eq_bind,Option.some_bind,mn,x] using ef
      · intro p hpn
        rw [lf p hpn]
        by_cases eq : p=b.point
        · subst p; simp [ln,boundarySum,boundaryTerm,own,both.2,add_assoc]
        · have rev : b.point ≠ p := Ne.symm eq
          simp [ln,boundarySum,boundaryTerm,own,both.2,eq,rev,add_assoc]
      · simpa [boundarySpeedFootprint,own,both.2,mn] using frame_trans (put_frame m (lambdaAddr K b.point) x) ff
    · have inactive : ¬ (b.owned=true ∧ b.contributes=true) := by
        intro h; exact active (by simp [h.1,h.2])
      obtain ⟨mf,ef,hf,lf,ff⟩ := ih m lam hp hl ownRest
      refine ⟨mf,by simpa [executeBoundarySpeeds,active,inactive] using ef,hf,?_,?_⟩
      · simpa [boundarySum,boundaryTerm,active,inactive] using lf
      · simpa [boundarySpeedFootprint,active,inactive] using ff

theorem put_other_scalar (b d i j : Nat) (ne : b ≠ d) (m : Mem) (x : ℝ) :
    put m (d,j) x (b,i) = m (b,i) := by
  have neq : (b,i) ≠ (d,j) := fun eq => ne (congrArg Prod.fst eq)
  simp [put,Function.update_of_ne neq]

theorem time_stores_execute (K : Nat) (cap : ℝ) (ps : List (Fin M)) (m : Mem)
    (vol cfl lam : Fin M → ℝ) (stats : TimeStats)
    (hv : ∀ p, m (2,p.val) = some (vol p))
    (hc : ∀ p, m (cflAddr K p) = some (cfl p))
    (hl : ∀ p ∈ ps, m (lambdaAddr K p) = some (lam p)) :
    executeTimeStores K cap ps m stats =
      some (writeNodes 4 (fun p => localPseudoStep (vol p) (cfl p) (lam p) cap) ps m,
        statsFold vol cfl lam ps stats) := by
  induction ps generalizing m stats with
  | nil => rfl
  | cons p ps ih =>
    have lamRead := hl p (by simp)
    have cflNe : auxBlock K 1 ≠ 4 := by unfold auxBlock; omega
    have lamNe : auxBlock K 0 ≠ 4 := by unfold auxBlock; omega
    have inputs (x : ℝ) :
        (∀ q, put m (4,p.val) x (2,q.val) = some (vol q)) ∧
        (∀ q, put m (4,p.val) x (cflAddr K q) = some (cfl q)) ∧
        (∀ q ∈ ps, put m (4,p.val) x (lambdaAddr K q) = some (lam q)) := by
      refine ⟨?_,?_,?_⟩
      · intro q; rw [put_other_scalar 2 4 _ _ (by decide)]; exact hv q
      · intro q
        change put m (4,p.val) x (auxBlock K 1,q.val) = _
        rw [put_other_scalar _ 4 _ _ cflNe]; exact hc q
      · intro q hq
        change put m (4,p.val) x (auxBlock K 0,q.val) = _
        rw [put_other_scalar _ 4 _ _ lamNe]; exact hl q (List.mem_cons_of_mem p hq)
    by_cases zeroVol : vol p = 0
    · have ready := inputs 0
      simpa [executeTimeStores,hv,zeroVol,writeNodes,localPseudoStep,statsFold] using
        ih (put m (4,p.val) 0) stats ready.1 ready.2.1 ready.2.2
    · let raw := cfl p*vol p/lam p
      have ready := inputs (min raw cap)
      simpa [executeTimeStores,hv,hc,lamRead,zeroVol,writeNodes,localPseudoStep,statsFold,raw] using
        ih (put m (4,p.val) (min raw cap))
          {stats with minimum := min stats.minimum raw,maximum := max stats.maximum raw}
          ready.1 ready.2.1 ready.2.2

def lambdaFootprint (K : Nat) (ps : List (Fin M)) (bs : List (BoundaryFace M)) : List Address :=
  ps.map (lambdaAddr K) ++ boundarySpeedFootprint K bs

theorem lambda_protected (K : Nat) (ps : List (Fin M)) (bs : List (BoundaryFace M))
    (a : Address) (ne : a.1 ≠ auxBlock K 0) : a ∉ lambdaFootprint K ps bs := by
  intro mem
  rcases List.mem_append.mp mem with points | boundary
  · obtain ⟨p,_,eq⟩ := List.mem_map.mp points
    exact ne (congrArg Prod.fst eq.symm)
  · obtain ⟨b,_,eq⟩ := List.mem_filterMap.mp boundary
    have first : a.1 = auxBlock K 0 := by
      split_ifs at eq
      cases eq
      rfl
    exact ne first

def timestepFootprint (K N M : Nat) (bs : List (BoundaryFace M)) : List Address :=
  lambdaFootprint K (ownedPoints N M) bs ++ (ownedPoints N M).map (fun p => (4,p.val))

/-- Complete selected steady inviscid timestep execution, from primitive/volume/CFL
INPUTS only. No initial timestep or final spectral-sum representation is assumed. -/
theorem timestep_refines (K N : Nat) (g : Geometry M E) (bs : List (BoundaryFace M))
    (P : Primitives M) (vol cfl : Fin M → ℝ) (cap : ℝ) (m : Mem) (prior : TimeStats)
    (hp : PrimitiveRep m P) (hv : ∀ p, m (2,p.val) = some (vol p))
    (hc : ∀ p, m (cflAddr K p) = some (cfl p))
    (ownership : ∀ b ∈ bs, b.owned=true → b.point.val < N) :
    ∃ mt stats, executeTimeStep K N g bs cap m prior = some (mt,stats) ∧
      (∀ p : Fin M, p.val<N → mt (4,p.val) = some (localPseudoStep (vol p) (cfl p) (eigenvalue P g bs p) cap)) ∧
      (∀ p : Fin M, N≤p.val → mt (4,p.val) = m (4,p.val)) ∧
      OwnedLambdaRep K N mt (eigenvalue P g bs) ∧ PrimitiveRep mt P ∧
      (∀ p, mt (2,p.val) = some (vol p)) ∧
      stats = statsFold vol cfl (eigenvalue P g bs) (ownedPoints N M) (initialTimeStats prior) ∧
      Frame (timestepFootprint K N M bs) m mt := by
  let ps := ownedPoints N M
  let mn := writeNodes (auxBlock K 0) (neighborSum P g) ps m
  have ep := neighbor_points_execute K g P ps m hp
  have pp : PrimitiveRep mn P := write_nodes_primitive _ (aux_not_primitive K 0) _ ps m P hp
  have lr : OwnedLambdaRep K N mn (neighborSum P g) := by
    intro p owned
    exact (write_nodes_value _ _ ps m p).trans (if_pos (owned_coverage N p owned))
  obtain ⟨mb,eb,pb,lb,fb⟩ := boundary_speeds_execute K N P bs mn (neighborSum P g) pp lr ownership
  have spectralFrame : Frame (lambdaFootprint K ps bs) m mb :=
    frame_trans (write_nodes_frame _ _ ps m) fb
  have volNe : 2 ≠ auxBlock K 0 := by unfold auxBlock; omega
  have cflNe : auxBlock K 1 ≠ auxBlock K 0 := by unfold auxBlock; omega
  have vb : ∀ p, mb (2,p.val) = some (vol p) := by
    intro p
    rw [spectralFrame _ (lambda_protected K ps bs _ volNe)]
    exact hv p
  have cb : ∀ p, mb (cflAddr K p) = some (cfl p) := by
    intro p
    rw [spectralFrame _ (lambda_protected K ps bs _ cflNe)]
    exact hc p
  have le : ∀ p ∈ ps, mb (lambdaAddr K p) = some (eigenvalue P g bs p) := by
    intro p mem
    exact lb p ((owned_membership N p).mp mem)
  let f := fun p => localPseudoStep (vol p) (cfl p) (eigenvalue P g bs p) cap
  let mt := writeNodes 4 f ps mb
  have et := time_stores_execute K cap ps mb vol cfl (eigenvalue P g bs) (initialTimeStats prior) vb cb le
  have tf := write_nodes_frame 4 f ps mb
  refine ⟨mt,_,?_,?_,?_,?_,write_nodes_primitive 4 (by decide) f ps mb P pb,?_,rfl,frame_trans spectralFrame tf⟩
  · dsimp only [ps,mn] at ep eb et
    simp only [executeTimeStep,ep,Option.bind_eq_bind,Option.some_bind,eb,et]
    rfl
  · intro p owned
    exact (write_nodes_value 4 f ps mb p).trans (if_pos (owned_coverage N p owned))
  · intro p halo
    have notOwned : p ∉ ps := by rw [owned_membership]; omega
    change writeNodes 4 f ps mb (4,p.val) = _
    rw [write_nodes_value,if_neg notOwned]
    exact spectralFrame (4,p.val) (lambda_protected K ps bs _ (by unfold auxBlock; omega))
  · intro p owned
    change writeNodes 4 f ps mb (auxBlock K 0,p.val) = _
    rw [write_nodes_other (auxBlock K 0) 4 (by unfold auxBlock; omega)]
    exact lb p owned
  · intro p
    exact (write_nodes_other 2 4 (by decide) f ps mb p.val).trans (vb p)

end SU2Interior

