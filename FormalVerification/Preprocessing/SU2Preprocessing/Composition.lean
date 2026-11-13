import SU2Preprocessing.Loop

set_option maxRecDepth 100000
set_option maxHeartbeats 8000000
noncomputable section
namespace SU2Preprocessing
open SU2 SU2Source SU2Boundary SU2BoundaryMemory SU2Initialization
variable {M K : Nat} {vertices : Fin K → Nat}

structure ControlState where
  machine : Machine
  errorCounter : Nat
  configNonphysical : Nat

/-- Selected mesh-zero COMM_FULL serial caller. The one-rank sum is identity.
Other common-preprocessing features are disabled by the pinned configuration. -/
def executeCommon (s0 : ControlState) : Option ControlState := do
  let s := {s0 with errorCounter := 0}
  let (processed,count) ← executePoints (points M) s.machine 0
  let s := {s with machine := processed, errorCounter := s.errorCounter + count}
  let s := {s with configNonphysical := s.errorCounter}
  return {s with machine := {s.machine with memory := clearResidual (N := M) s.machine.memory}}

def executeCommonSnapshot (s0 : ControlState) : Option ControlState := do
  let s ← executeCommon (M := M) s0
  let m ← executeSnapshot (M := M) s.machine.memory
  return {s with machine := {s.machine with memory := m}}

def residualFootprint (M : Nat) : List Address := (visits M).map (fun c => (5,offset c))
def preprocessingFootprint (M : Nat) : List Address := (points M).flatMap rowFootprint ++ residualFootprint M

theorem row_protected (p : Fin M) (a : Address) (h0 : a.1 ≠ 0) (h7 : a.1 ≠ 7) (h8 : a.1 ≠ 8) :
    a ∉ rowFootprint p := by
  intro hin
  rcases List.mem_append.mp hin with hrows | hv
  · rcases List.mem_append.mp hrows with hc | hp
    · obtain ⟨c,eq⟩ := List.mem_ofFn.mp hc
      exact h0 (congrArg Prod.fst eq.symm)
    · obtain ⟨c,eq⟩ := List.mem_ofFn.mp hp
      exact h8 (congrArg Prod.fst eq.symm)
  · have eq : a = velocityAddr p := List.mem_singleton.mp hv
    exact h7 (congrArg Prod.fst eq)

theorem preprocessing_protected (a : Address) (h0 : a.1 ≠ 0) (h5 : a.1 ≠ 5)
    (h7 : a.1 ≠ 7) (h8 : a.1 ≠ 8) : a ∉ preprocessingFootprint M := by
  intro hin
  rcases List.mem_append.mp hin with hr | hc
  · obtain ⟨p,_,ha⟩ := List.mem_flatMap.mp hr
    exact row_protected p a h0 h7 h8 ha
  · obtain ⟨c,_,eq⟩ := List.mem_map.mp hc
    exact h5 (congrArg Prod.fst eq.symm)

theorem clear_other (b : Nat) (hb : b ≠ 5) (i : Nat) (m : Mem) :
    clearResidual (N := M) m (b,i) = m (b,i) := store_other b 5 hb _ _ m i

theorem clear_data (s : Machine) (d : Data M) (h : DataRep s d) :
    DataRep ⟨clearResidual (N := M) s.memory,s.fluid⟩ d := by
  refine ⟨?_,?_,?_,?_⟩
  · intro p c; exact (clear_other 0 (by decide) _ s.memory).trans (h.current p c)
  · intro p c; exact (clear_other 1 (by decide) _ s.memory).trans (h.old p c)
  · intro p c; exact (clear_other 8 (by decide) _ s.memory).trans (h.primitive p c)
  · intro p; exact (clear_other 7 (by decide) _ s.memory).trans (h.velocity p)

/-- Boundary input state is specified from the executed point fold and zero residual.
Truncation and initial cache knowledge are framed from the PRE-preprocessing input. -/
def preprocessingState (d : Data M) (T : Residual M 2) (cache : Nat → Option Ghost) : BoundaryState M :=
  ⟨d.current,d.old,fun _ => 0,T,cache⟩

theorem common_refines (g : IdealGas) (s0 : ControlState) (d : Data M)
    (T : Residual M 2) (cache : Nat → Option Ghost)
    (h0 : DataRep s0.machine d) (hf : FluidReady g s0.machine.fluid)
    (ht : Stored 6 s0.machine.memory (fun c => T c.1 (channel c.2)))
    (hc : CacheRep vertices s0.machine.memory cache) :
    let expected := modelPoints g (points M) d 0
    ∃ pre, executeCommon (M := M) s0 = some pre ∧
      DataRep pre.machine expected.1 ∧ FluidReady g pre.machine.fluid ∧
      BeforeSnapshot (vertices := vertices) pre.machine.memory (preprocessingState expected.1 T cache) expected.1.primitive ∧
      pre.errorCounter = expected.2 ∧ pre.configNonphysical = expected.2 ∧
      pre.errorCounter ≤ M ∧
      pre.machine.fluid.parameters = s0.machine.fluid.parameters ∧
      pre.machine.fluid.entropyEnabled = s0.machine.fluid.entropyEnabled ∧
      pre.machine.fluid.untouched = s0.machine.fluid.untouched ∧
      Frame (preprocessingFootprint M) s0.machine.memory pre.machine.memory := by
  dsimp only
  obtain ⟨processed, ep, hd, ready, mode, params, extra, frame⟩ := points_refine g (points M) s0.machine d 0 h0 hf
  let pre : ControlState := ⟨⟨clearResidual (N := M) processed.memory,processed.fluid⟩,
    (modelPoints g (points M) d 0).2,(modelPoints g (points M) d 0).2⟩
  have cleared := clear_data processed _ hd
  have combined : Frame (preprocessingFootprint M) s0.machine.memory pre.machine.memory :=
    frame_trans frame (residual_clear_frame processed.memory)
  have truncation : Stored 6 pre.machine.memory (fun c => T c.1 (channel c.2)) := by
    intro c
    rw [combined (6,offset c) (preprocessing_protected (M := M) (6,offset c) (by norm_num) (by norm_num) (by norm_num) (by norm_num))]
    exact ht c
  have cacheRep : CacheRep vertices pre.machine.memory cache := by
    intro site ghost hghost c
    rw [combined (cacheAddr site.marker site.vertex c)
      (preprocessing_protected (M := M) (cacheAddr site.marker site.vertex c)
        (by simp only [cacheAddr, Prod.fst]; omega) (by simp only [cacheAddr, Prod.fst]; omega)
        (by simp only [cacheAddr, Prod.fst]; omega) (by simp only [cacheAddr, Prod.fst]; omega))]
    exact hc site ghost hghost c
  refine ⟨pre, ?_, cleared, ready, ?_, rfl,rfl, ?_, params, mode, extra, combined⟩
  · simp [executeCommon, ep, pre]
  · exact ⟨(fun c => cleared.current c.1 c.2), residual_clear_complete processed.memory, truncation, cleared.primitive, cacheRep⟩
  · simpa only [zero_add, points_count] using counter_bound g (points M) d 0

/-- Principal result: all memory premises concern s0 BEFORE preprocessing.
It executes every point, residual clearing and the later complete snapshot. -/
theorem preprocessing_snapshot_refines (g : IdealGas) (s0 : ControlState) (d : Data M)
    (T : Residual M 2) (cache : Nat → Option Ghost)
    (h0 : DataRep s0.machine d) (hf : FluidReady g s0.machine.fluid)
    (ht : Stored 6 s0.machine.memory (fun c => T c.1 (channel c.2)))
    (hc : CacheRep vertices s0.machine.memory cache) :
    let expected := modelPoints g (points M) d 0
    let state := preprocessingState expected.1 T cache
    ∃ pre post,
      executeCommon (M := M) s0 = some pre ∧ executeCommonSnapshot (M := M) s0 = some post ∧
      BeforeSnapshot (vertices := vertices) pre.machine.memory state expected.1.primitive ∧
      BoundaryRep vertices post.machine.memory (snapshotState state) expected.1.primitive ∧
      Stored 0 post.machine.memory (fun c => expected.1.current c.1 (channel c.2)) ∧
      post.errorCounter = expected.2 ∧ post.configNonphysical = expected.2 ∧ post.errorCounter ≤ M ∧
      post.machine.fluid = pre.machine.fluid ∧ FluidReady g post.machine.fluid ∧
      Frame (preprocessingFootprint M ++ snapshotFootprint M) s0.machine.memory post.machine.memory := by
  dsimp only
  obtain ⟨pre,ep,_,ready,before,count,stored,countBound,_,_,_,frame⟩ := common_refines g s0 d T cache h0 hf ht hc
  obtain ⟨ms,es,rep,current,snapshotFrame⟩ := snapshot_refines pre.machine.memory _ _ before
  let post : ControlState := {pre with machine := {pre.machine with memory := ms}}
  refine ⟨pre,post,ep,?_,before,rep,current,count,stored,countBound,rfl,ready,frame_trans frame snapshotFrame⟩
  simp [executeCommonSnapshot,ep,es,post]

end SU2Preprocessing
