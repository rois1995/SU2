import SU2Interior.InteriorProof
import SU2Interior.CFDCompatibility

set_option maxRecDepth 100000
set_option maxHeartbeats 8000000
noncomputable section
namespace SU2Interior
open SU2 SU2Source SU2Boundary SU2BoundaryMemory SU2Initialization SU2Preprocessing
variable {M E K : Nat} {vertices : Fin K → Nat}

def cflFootprint (K M : Nat) : List Address := (points M).map (cflAddr K)
def preprocessingSnapshotFootprint (M : Nat) : List Address := preprocessingFootprint M ++ snapshotFootprint M

theorem cfl_protected (K : Nat) (c : ℝ) (m : Mem) (a : Address) (ne : a.1≠auxBlock K 1) :
    initializeCFL (M := M) K c m a = m a := by
  apply write_nodes_frame
  intro mem
  obtain ⟨p,_,eq⟩ := List.mem_map.mp mem
  exact ne (congrArg Prod.fst eq.symm)

theorem cfl_data_rep (K : Nat) (c : ℝ) (s : Machine) (d : Data M) (h : DataRep s d) :
    DataRep ⟨initializeCFL (M := M) K c s.memory,s.fluid⟩ d := by
  refine ⟨?_,?_,?_,?_⟩
  · intro p j
    dsimp only
    rw [cfl_protected K c s.memory (flowAddr 0 p j) (by unfold flowAddr auxBlock; simp only [Prod.fst]; omega)]
    exact h.current p j
  · intro p j
    dsimp only
    rw [cfl_protected K c s.memory (flowAddr 1 p j) (by unfold flowAddr auxBlock; simp only [Prod.fst]; omega)]
    exact h.old p j
  · intro p j
    dsimp only
    rw [cfl_protected K c s.memory (primitiveAddr p j) (by unfold primitiveAddr auxBlock; simp only [Prod.fst]; omega)]
    exact h.primitive p j
  · intro p
    dsimp only
    rw [cfl_protected K c s.memory (velocityAddr p) (by unfold velocityAddr auxBlock; simp only [Prod.fst]; omega)]
    exact h.velocity p

theorem preprocessing_snapshot_protected (a : Address) (h0 : a.1≠0) (h1 : a.1≠1)
    (h5 : a.1≠5) (h7 : a.1≠7) (h8 : a.1≠8) : a ∉ preprocessingSnapshotFootprint M := by
  intro mem
  rcases List.mem_append.mp mem with first | second
  · exact preprocessing_protected a h0 h5 h7 h8 first
  · obtain ⟨c,_,eq⟩ := List.mem_map.mp second
    exact h1 (congrArg Prod.fst eq.symm)

theorem time_protected (K N : Nat) (bs : List (BoundaryFace M)) (a : Address)
    (hl : a.1≠auxBlock K 0) (hd : a.1≠4) : a ∉ timestepFootprint K N M bs := by
  intro mem
  rcases List.mem_append.mp mem with eig | dt
  · exact lambda_protected K (ownedPoints N M) bs a hl eig
  · obtain ⟨p,_,eq⟩ := List.mem_map.mp dt
    exact hd (congrArg Prod.fst eq.symm)

theorem time_boundary_rep (K N : Nat) {vertices : Fin K → Nat} (bs : List (BoundaryFace M)) (m mt : Mem)
    (s : BoundaryState M) (P : Primitives M) (h : BoundaryRep vertices m s P)
    (frame : Frame (timestepFootprint K N M bs) m mt) : BoundaryRep vertices mt s P := by
  refine ⟨?_,?_,?_⟩
  · intro b p c
    rw [frame _ (time_protected K N bs (flowAddr b.block p c)
      (by have hb := buffer_block_bound b; unfold flowAddr auxBlock; simp only [Prod.fst]; omega)
      (by cases b <;> norm_num [flowAddr,Buffer.block]))]
    exact h.buffers b p c
  · intro p c
    rw [frame _ (time_protected K N bs (primitiveAddr p c)
      (by unfold primitiveAddr auxBlock; simp only [Prod.fst]; omega) (by norm_num [primitiveAddr]))]
    exact h.primitives p c
  · intro site ghost hg c
    rw [frame _ (time_protected K N bs (cacheAddr site.marker site.vertex c)
      (by have hm := site.marker.isLt; unfold cacheAddr auxBlock; simp only [Prod.fst]; omega)
      (by unfold cacheAddr; simp only [Prod.fst]; omega))]
    exact h.cache site ghost hg c

structure PrefixResult where
  control : ControlState
  time : TimeStats
  reconstructionCounter : Nat

/-- First configured iteration prefix: constructor CFL stores, preprocessing, old
snapshot, steady timestep, then all interior SIMD packets. Boundary processing follows. -/
def executeToBoundary (K w : Nat) (g : IdealGas) (geo : Geometry M E)
    (bs : List (BoundaryFace M)) (cfl cap : ℝ) (counter : Fin E → Nat)
    (s0 : ControlState) (prior : TimeStats) : Option PrefixResult := do
  let initialized := {s0 with machine := {s0.machine with memory := initializeCFL (M := M) K cfl s0.machine.memory}}
  let pp ← executeSourceCommonSnapshot (M := M) SU2Preprocessing.Generated.original initialized
  let (mt,stats) ← executeTimeStep K M geo bs cap pp.machine.memory prior
  let (mi,count) ← executeInterior w g geo counter mt
  return ⟨{pp with machine := {pp.machine with memory := mi},errorCounter := count},stats,count⟩

def prefixFootprint (K w M E : Nat) (geo : Geometry M E) (bs : List (BoundaryFace M)) : List Address :=
  cflFootprint K M ++ preprocessingSnapshotFootprint M ++ timestepFootprint K M M bs ++
    (allLanes w E).flatMap (laneFootprint geo)

/-- Principal composition. Every memory premise concerns s0 BEFORE preprocessing.
Both dt and interior residual are computed, not installed as supplied expected outputs.
Geometry construction/heap decoding and full numerical Roe science remain explicit gaps. -/
theorem preprocessing_to_boundary_memory (w : Nat) (g : IdealGas) (geo : Geometry M E)
    (bs : List (BoundaryFace M)) (ready : GeometryReady geo) (cfl cap : ℝ)
    (counter : Fin E → Nat) (s0 : ControlState) (prior : TimeStats) (d : Data M)
    (T : Residual M 2) (cache : Nat → Option Ghost) (vol : Fin M → ℝ)
    (hd : DataRep s0.machine d) (hf : FluidReady g s0.machine.fluid)
    (ht : Stored 6 s0.machine.memory (fun c => T c.1 (channel c.2)))
    (hc : CacheRep vertices s0.machine.memory cache)
    (hv : ∀ p, s0.machine.memory (2,p.val) = some (vol p))
    (hper : ∀ p : Fin M, s0.machine.memory (3,p.val)=some 0) :
    let data := (modelPoints g (points M) d 0).1
    let ppState := snapshotState (preprocessingState data T cache)
    let state := interiorState g geo data.primitive ready ppState
    let dt := fun p => localPseudoStep (vol p) cfl (eigenvalue data.primitive geo bs p) cap
    ∃ result, executeToBoundary K w g geo bs cfl cap counter s0 prior = some result ∧
      BoundaryRep vertices result.control.machine.memory state data.primitive ∧
      GeometryRep result.control.machine.memory vol (fun _ => 0) dt ∧
      CurrentRep result.control.machine.memory data.current ∧
      result.reconstructionCounter = reconstructionCount (allLanes w E) counter ∧
      result.control.configNonphysical = (modelPoints g (points M) d 0).2 ∧
      Frame (prefixFootprint K w M E geo bs) s0.machine.memory result.control.machine.memory := by
  dsimp only
  let mc := initializeCFL (M := M) K cfl s0.machine.memory
  let initial : ControlState := {s0 with machine := {s0.machine with memory := mc}}
  have dc := cfl_data_rep K cfl s0.machine d hd
  have tc : Stored 6 mc (fun c => T c.1 (channel c.2)) := by
    intro c
    dsimp only [mc]
    rw [cfl_protected K cfl s0.machine.memory (6,offset c) (by unfold auxBlock; simp only [Prod.fst]; omega)]
    exact ht c
  have cc : CacheRep vertices mc cache := by
    intro site ghost hg c
    dsimp only [mc]
    rw [cfl_protected K cfl s0.machine.memory (cacheAddr site.marker site.vertex c)
      (by have hm := site.marker.isLt; unfold cacheAddr auxBlock; simp only [Prod.fst]; omega)]
    exact hc site ghost hg c
  obtain ⟨pre,pp,_,ep,_,rp,cur,_,count,_,_,_,fp⟩ :=
    source_preprocessing_snapshot_refines g initial d T cache dc hf tc cc
  dsimp only [initial,mc] at fp
  have cflFrame := write_nodes_frame (auxBlock K 1) (fun _ : Fin M => cfl) (points M) s0.machine.memory
  have vp : ∀ p, pp.machine.memory (2,p.val)=some (vol p) := by
    intro p
    rw [fp _ (preprocessing_snapshot_protected (2,p.val) (by norm_num) (by norm_num) (by norm_num) (by norm_num) (by norm_num))]
    rw [cfl_protected K cfl s0.machine.memory (2,p.val) (by unfold auxBlock; simp only [Prod.fst]; omega)]
    exact hv p
  have cp : ∀ p : Fin M, pp.machine.memory (cflAddr K p)=some cfl := by
    intro p
    rw [fp _ (preprocessing_snapshot_protected (cflAddr K p)
      (by unfold cflAddr auxAddr auxBlock; simp only [Prod.fst]; omega) (by unfold cflAddr auxAddr auxBlock; simp only [Prod.fst]; omega)
      (by unfold cflAddr auxAddr auxBlock; simp only [Prod.fst]; omega) (by unfold cflAddr auxAddr auxBlock; simp only [Prod.fst]; omega)
      (by unfold cflAddr auxAddr auxBlock; simp only [Prod.fst]; omega))]
    exact initial_cfl_ready K cfl s0.machine.memory p
  obtain ⟨mt,stats,et,dt,_,_,_,vt,_,ft⟩ := timestep_refines K M geo bs _ vol (fun _ => cfl) cap pp.machine.memory prior rp.primitives vp cp
    (fun b _ _ => b.point.isLt)
  have rt := time_boundary_rep K M bs pp.machine.memory mt _ _ rp ft
  obtain ⟨mi,ei,ri,fi⟩ := interior_refines w g geo _ ready counter mt _ rt
  let postControl : ControlState := {pp with
    machine := {pp.machine with memory := mi}
    errorCounter := reconstructionCount (allLanes w E) counter}
  let result : PrefixResult := ⟨postControl,stats,reconstructionCount (allLanes w E) counter⟩
  have combined := frame_trans (frame_trans (frame_trans cflFrame fp) ft) fi
  refine ⟨result,?_,ri,?_,?_,rfl,count,combined⟩
  · dsimp only [initial,mc] at ep
    simp only [executeToBoundary,ep,Option.bind_eq_bind,Option.some_bind,et,ei]
    rfl
  · dsimp only [result,postControl]
    refine ⟨?_,?_,?_⟩
    · intro p
      rw [fi _ (interior_protected geo _ (2,p.val) (by norm_num))]
      exact vt p
    · intro p
      rw [fi _ (interior_protected geo _ (3,p.val) (by norm_num))]
      rw [ft _ (time_protected K M bs (3,p.val) (by unfold auxBlock; simp only [Prod.fst]; omega) (by norm_num))]
      rw [fp _ (preprocessing_snapshot_protected (3,p.val) (by norm_num) (by norm_num) (by norm_num) (by norm_num) (by norm_num))]
      rw [cfl_protected K cfl s0.machine.memory (3,p.val) (by unfold auxBlock; simp only [Prod.fst]; omega)]
      exact hper p
    · intro p
      rw [fi _ (interior_protected geo _ (4,p.val) (by norm_num))]
      exact dt p p.isLt
  · intro p c
    dsimp only [result,postControl]
    rw [fi _ (interior_protected geo _ (flowAddr 0 p c) (by norm_num [flowAddr]))]
    rw [ft _ (time_protected K M bs (flowAddr 0 p c)
      (by unfold flowAddr auxBlock; simp only [Prod.fst]; omega) (by norm_num [flowAddr]))]
    exact cur (p,c)

end SU2Interior
