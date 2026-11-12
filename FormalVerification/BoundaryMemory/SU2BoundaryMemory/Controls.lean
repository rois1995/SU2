import SU2BoundaryMemory.SourceConnection

set_option maxRecDepth 100000
set_option maxHeartbeats 8000000
noncomputable section
namespace SU2BoundaryMemory
open SU2 SU2Source SU2Euler SU2Boundary

/-- These are witness tests of the unchanged full representation/frame contracts.
They are hypothetical numerical mutations, not confirmed production defects. -/
def testVertices : Fin 2 → Nat := fun _ => 2
def testGas : IdealGas := ⟨2, 1, by norm_num, by norm_num⟩
def uniform : Primitive 2 := ⟨1, ![1/2,0], 1/2⟩
def oblique : UnitNormal 2 := ⟨![3/5,4/5], by norm_num [dot, Fin.sum_univ_two]⟩
def obliqueVisit : WallVisit 1 := ⟨0, true, oblique⟩
def initialState : BoundaryState 1 :=
  ⟨0, fun _ => state 2 ![3,4] 40, 0, 0, fun _ => none⟩
def initialMemory : Mem := fun a =>
  if a = (1,0) then some 2 else if a = (1,1) then some 3 else
  if a = (1,2) then some 4 else if a = (1,3) then some 40 else some 0

theorem initial_rep : BoundaryRep testVertices initialMemory initialState (fun _ _ => 0) := by
  refine ⟨?_, ?_, ?_⟩
  · intro b p c
    fin_cases p
    cases b <;> fin_cases c <;>
      norm_num [BuffersRep, Buffer.block, Buffer.get, initialMemory, initialState, flowAddr, channel, state]
  · intro p c
    norm_num [initialMemory, primitiveAddr]
  · intro site g hg
    simp [initialState] at hg

theorem test_bounds : Bounds 1 1 2 testVertices := by
  constructor <;> norm_num [testVertices]

def wallTrial (code : WallCode) (discard : Bool) (m : Mem) : Option Mem := do
  let after ← executeWall 1 code obliqueVisit m
  return if discard then m else after

def WallMemoryContract (code : WallCode) (discard : Bool) : Prop :=
  ∃ after, wallTrial code discard initialMemory = some after ∧
    BoundaryRep testVertices after (wallSpec obliqueVisit initialState) (fun _ _ => 0) ∧
    Frame (wallFootprint obliqueVisit) initialMemory after

theorem wall_memory_positive : WallMemoryContract wallCode false := by
  obtain ⟨after, he, hp, hf⟩ := wall_memory_refines initialMemory initialState (fun _ _ => 0)
    obliqueVisit test_bounds initial_rep
  exact ⟨after, by simp [wallTrial, he], hp, hf⟩

theorem bad_wall_observation (code : WallCode) (discard : Bool) (c : Fin 4)
    (bad : (wallTrial code discard initialMemory).bind (fun m => m (flowAddr 1 (0 : Fin 1) c)) ≠
      some ((wallSpec obliqueVisit initialState).old 0 (channel c))) :
    ¬ WallMemoryContract code discard := by
  rintro ⟨after, he, hp, _⟩
  rw [he, Option.some_bind] at bad
  exact bad (hp.buffers .old 0 c)

theorem discarded_writes_rejected : ¬ WallMemoryContract wallCode true := by
  obtain ⟨after, he, _, _⟩ := wall_memory_positive
  have he' : executeWall 1 wallCode obliqueVisit initialMemory = some after := by
    simpa [wallTrial] using he
  apply bad_wall_observation wallCode true 1
  simp only [wallTrial, he', Option.bind_eq_bind, Option.some_bind, ite_true]
  norm_num [initialMemory, flowAddr, Prod.ext_iff, wallSpec, obliqueVisit, oblique,
    initialState, wallProject, state, channel, wallDefect, dot, Fin.sum_univ_two, mom, rho]

def wrongBufferCode : WallCode := {wallCode with oldBlock := Generated.oldBinding_wrongBuffer}
def refreshedCode : WallCode := {wallCode with refresh := Generated.wallOld_refresh.refresh}
def reversedCode : WallCode := {wallCode with reverse := Generated.wallOld_refactored.reverse}

theorem source_mutants_admitted : Generated.oldBinding_wrongBuffer = 0 ∧
    Generated.wallOld_refresh = ⟨.sub, true, false⟩ ∧
    Generated.wallRes_refresh = ⟨.sub, true, false⟩ ∧
    Generated.wallTrunc_refresh = ⟨.sub, true, false⟩ ∧
    Generated.wallOld_refactored = ⟨.sub, false, true⟩ ∧
    Generated.wallRes_refactored = ⟨.sub, false, true⟩ ∧
    Generated.wallTrunc_refactored = ⟨.sub, false, true⟩ ∧
    Generated.primitiveExtra_shortStride = 4 ∧ Generated.cacheLookup_firstMarker = .firstMarker ∧
    Generated.cacheLookup_firstVertex = .firstVertex ∧
    Generated.cacheWrites_eraseTail = true := by decide

theorem wrong_buffer_rejected : ¬ WallMemoryContract wrongBufferCode false := by
  apply bad_wall_observation wrongBufferCode false 1
  have ho : SU2Boundary.Generated.wallOld_original = .sub := by decide
  have hr : SU2Boundary.Generated.wallRes_original = .sub := by decide
  have ht : SU2Boundary.Generated.wallTrunc_original = .sub := by decide
  norm_num [wallTrial, executeWall, wrongBufferCode, wallCode, source_mutants_admitted.1,
    projectMemory, put, Function.update_apply, Prod.ext_iff, validRow, initialMemory, flowAddr, obliqueVisit, oblique, ho, hr, ht,
    opReal, wallSpec, initialState, wallProject, state, channel, wallDefect, dot, Fin.sum_univ_two, mom, rho]

theorem refreshed_projection_rejected : ¬ WallMemoryContract refreshedCode false := by
  apply bad_wall_observation refreshedCode false 2
  have ho : SU2Boundary.Generated.wallOld_original = .sub := by decide
  have hr : SU2Boundary.Generated.wallRes_original = .sub := by decide
  have ht : SU2Boundary.Generated.wallTrunc_original = .sub := by decide
  have hf : Generated.wallOld_refresh.refresh = true := by decide
  norm_num [wallTrial, executeWall, refreshedCode, wallCode, hf,
    projectMemory, put, Function.update_apply, Prod.ext_iff, validRow, initialMemory, flowAddr, obliqueVisit, oblique, ho, hr, ht,
    opReal, wallSpec, initialState, wallProject, state, channel, wallDefect, dot, Fin.sum_univ_two, mom, rho]
  change (36/25 : ℝ) ≠ 4 - 5 * (4/5)
  norm_num

/-- A source-compatible reordering is proved for arbitrary memory, including failure. -/
theorem wall_refactor_memory {N : Nat} (rows : Nat) (v : WallVisit N) (m : Mem) :
    executeWall rows reversedCode v m = executeWall rows wallCode v m := by
  have hf : Generated.wallOld_refactored.reverse = true := by decide
  simp only [executeWall, reversedCode, wallCode, hf]
  simp only [← projection_refactor]

theorem wall_refactor_positive : WallMemoryContract reversedCode false := by
  simpa only [WallMemoryContract, wallTrial, wall_refactor_memory] using wall_memory_positive

def cacheSite : CacheSite 2 testVertices := ⟨1, ⟨1, by decide⟩⟩
def cacheInitial : Mem := fun _ => some 99

def CacheMemoryContract (mode : CacheMode) (erase : Bool) : Prop :=
  let after := writeCache mode erase cacheSite (ghost testGas uniform) cacheInitial
  (∀ c : Fin 6, after (cacheAddr cacheSite.marker cacheSite.vertex c) =
    some (ghostField (ghost testGas uniform) c)) ∧ Frame (cacheFootprint cacheSite) cacheInitial after

theorem cache_memory_positive : CacheMemoryContract .normal false :=
  ⟨write_cache_value cacheSite _ cacheInitial, write_cache_frame cacheSite _ cacheInitial⟩

theorem cache_stride_rejected : ¬ CacheMemoryContract .shortStride false := by
  intro h
  have e := h.1 0
  norm_num [show CacheMode.normal ≠ .firstVertex from by decide,
    show CacheMode.shortStride ≠ .firstVertex from by decide,
    show CacheMode.firstMarker ≠ .firstVertex from by decide,
    writeCache, selectedCacheAddr, cacheSite, testVertices, cacheAddr, cacheInitial,
    put, Function.update_apply, Prod.ext_iff, ghostField, ghost, testGas, uniform, totalEnthalpy, specificEnergy, dot, Fin.sum_univ_two] at e
  norm_num [show CacheMode.shortStride ≠ .firstMarker from by decide,
    show cacheSite.vertex.val = 1 from rfl] at e

theorem cache_marker_rejected : ¬ CacheMemoryContract .firstMarker false := by
  intro h
  have e := h.1 0
  norm_num [show CacheMode.normal ≠ .firstVertex from by decide,
    show CacheMode.shortStride ≠ .firstVertex from by decide,
    show CacheMode.firstMarker ≠ .firstVertex from by decide,
    writeCache, selectedCacheAddr, cacheSite, testVertices, cacheAddr, cacheInitial,
    put, Function.update_apply, Prod.ext_iff, ghostField, ghost, testGas, uniform, totalEnthalpy, specificEnergy, dot, Fin.sum_univ_two] at e

theorem cache_vertex_rejected : ¬ CacheMemoryContract .firstVertex false := by
  intro h
  have e := h.1 0
  norm_num [show CacheMode.firstVertex ≠ .firstMarker from by decide,
    show CacheMode.firstVertex ≠ .shortStride from by decide,
    writeCache, selectedCacheAddr, cacheSite, testVertices, cacheAddr, cacheInitial,
    put, Function.update_apply, Prod.ext_iff, ghostField, ghost, testGas, uniform,
    totalEnthalpy, specificEnergy, dot, Fin.sum_univ_two] at e
  norm_num [show cacheSite.vertex.val = 1 from rfl] at e

theorem untouched_cache_rejected : ¬ CacheMemoryContract .normal true := by
  intro h
  have e := h.2 (cacheAddr 1 1 6) (by decide)
  norm_num [show CacheMode.normal ≠ .firstVertex from by decide,
    show CacheMode.shortStride ≠ .firstVertex from by decide,
    show CacheMode.firstMarker ≠ .firstVertex from by decide,
    writeCache, selectedCacheAddr, cacheSite, testVertices, cacheAddr, cacheInitial,
    put, Function.update_apply, Prod.ext_iff, ghostField, ghost, testGas, uniform, totalEnthalpy, specificEnergy, dot, Fin.sum_univ_two] at e
  norm_num [show CacheMode.normal ≠ .firstMarker from by decide,
    show CacheMode.normal ≠ .shortStride from by decide, show cacheSite.vertex.val = 1 from rfl] at e


def uniformP : Primitives 1 := fun _ => ![1/2,1/2,0,1/2,1,9/8,77,78,79,80,81]
def uniformU : Fin 4 → ℝ := ![1,1/2,0,5/8]
def repeatState : BoundaryState 1 :=
  ⟨fun _ => fromComponents uniformU, fun _ => fromComponents uniformU, 0, 0, fun _ => none⟩
def repeatMemory : Mem := fun a =>
  if a.1 = 8 then some (uniformP 0 ⟨a.2 % 11, Nat.mod_lt _ (by decide)⟩)
  else if a.1 = 0 ∨ a.1 = 1 then some (uniformU ⟨a.2 % 4, Nat.mod_lt _ (by decide)⟩)
  else some 0

def repeatedVisit : FarSite 1 2 testVertices := ⟨0, cacheSite, true, axis, 1, by norm_num⟩

theorem uniform_inside : inside uniformP 0 = uniform := rfl

theorem uniform_enthalpy : totalEnthalpy testGas uniform = 9/8 := by
  norm_num [totalEnthalpy, specificEnergy, testGas, uniform, dot, Fin.sum_univ_two]

theorem uniform_reconstruction : farPrimitiveImpl testGas axis.vector uniform uniform = uniform := by
  apply Primitive.ext
  · norm_num [farPrimitiveImpl, testGas, uniform, axis, sound, entropy, dot, Fin.sum_univ_two]
  · funext j
    fin_cases j <;> norm_num [farPrimitiveImpl, testGas, uniform, axis, sound, entropy, dot, Fin.sum_univ_two]
  · norm_num [farPrimitiveImpl, testGas, uniform, axis, sound, entropy, dot, Fin.sum_univ_two]

theorem uniform_roe : roeImplementation testGas axis.vector 1 uniform uniform (9/8) (9/8) =
    ![1/2,3/4,0,9/16] := by
  funext c
  fin_cases c <;> norm_num [roeImplementation, dissipation_loop_correct, testGas, uniform,
    axis, dot, Fin.sum_univ_two, storedFlux]

theorem repeat_initial_rep : BoundaryRep testVertices repeatMemory repeatState uniformP := by
  refine ⟨?_, ?_, ?_⟩
  · intro b p c
    fin_cases p
    cases b <;> fin_cases c <;>
      norm_num [Buffer.get, Buffer.block, repeatState, repeatMemory, flowAddr, channel, fromComponents, state, uniformU]
    all_goals rfl
  · intro p c
    fin_cases p
    fin_cases c <;> norm_num [repeatMemory, primitiveAddr]
  · intro site g hg
    simp [repeatState] at hg

theorem repeat_far_ready : FarReady testGas uniform
    ([repeatedVisit, repeatedVisit].map (farVisit uniformP)) := by
  intro v hv _
  have hv' : v = farVisit uniformP repeatedVisit := by simpa using hv
  subst v
  have hc : farPrimitive testGas axis.vector uniform uniform = uniform := by
    rw [← far_reconstruction_correct, uniform_reconstruction]
  change uniform.density ≠ 0 ∧ (farPrimitive testGas axis.vector uniform uniform).density ≠ 0 ∧
    uniformP 0 5 = totalEnthalpy testGas uniform
  rw [hc, uniform_enthalpy]
  norm_num [uniform, uniformP]
  rfl

theorem one_visit_mass :
    (farImpl SU2Boundary.Generated.farAdd_original testGas uniform (farVisit uniformP repeatedVisit)
      repeatState).residual 0 .mass = 1/2 := by
  have ha : SU2Boundary.Generated.farAdd_original = .add := by decide
  simp [farImpl, farVisit, repeatedVisit, uniform_inside, uniform_reconstruction, uniform_enthalpy,
    uniform_roe, uniformP, repeatState, ha, opReal, fromComponents, state]

theorem two_visit_mass :
    (boundarySpec testGas uniform ([repeatedVisit,repeatedVisit].map (farVisit uniformP)) [] repeatState).residual
      0 .mass = 1 := by
  rw [← boundary_refinement _ _ _ _ _ repeat_far_ready]
  have ha : SU2Boundary.Generated.farAdd_original = .add := by decide
  norm_num [boundaryImpl, farImpl, farVisit, repeatedVisit, uniform_inside, uniform_reconstruction,
    uniform_enthalpy, uniform_roe, uniformP, repeatState, ha, opReal, fromComponents, state]
  change roeImplementation testGas axis.vector 1 uniform uniform (9/8) (9/8) 0 +
    roeImplementation testGas axis.vector 1 uniform uniform (9/8) (9/8) 0 = 1
  rw [uniform_roe]
  norm_num

def RepeatedVisitContract (deduplicate : Bool) : Prop :=
  ∃ after, executeFars 1 farCode testGas uniform
      (if deduplicate then [repeatedVisit] else [repeatedVisit,repeatedVisit]) repeatMemory = some after ∧
    BoundaryRep testVertices after
      (boundarySpec testGas uniform ([repeatedVisit,repeatedVisit].map (farVisit uniformP)) [] repeatState) uniformP ∧
    Frame (boundaryFootprint [repeatedVisit,repeatedVisit] []) repeatMemory after

theorem repeated_visits_positive : RepeatedVisitContract false := by
  obtain ⟨after, he, hp, hf⟩ := boundary_memory_refines [repeatedVisit,repeatedVisit] [] repeatMemory repeatState
    uniformP testGas uniform test_bounds repeat_initial_rep repeat_far_ready
  refine ⟨after, ?_, hp, hf⟩
  simpa [executeBoundary, executeWalls] using he

theorem repeated_visit_omission_rejected : ¬ RepeatedVisitContract true := by
  obtain ⟨after, he, hp, _⟩ := fars_memory_refines [repeatedVisit] repeatMemory repeatState uniformP
    testGas uniform test_bounds repeat_initial_rep
  rintro ⟨bad, hb, hs, _⟩
  simp only [ite_true] at hb
  rw [he] at hb
  cases Option.some.inj hb
  have h1 := hp.buffers .residual 0 0
  have h2 := hs.buffers .residual 0 0
  have eq := h1.symm.trans h2
  change some ((farImpl SU2Boundary.Generated.farAdd_original testGas uniform
    (farVisit uniformP repeatedVisit) repeatState).residual 0 .mass) =
    some ((boundarySpec testGas uniform ([repeatedVisit,repeatedVisit].map (farVisit uniformP)) []
      repeatState).residual 0 .mass) at eq
  rw [one_visit_mass, two_visit_mass] at eq
  norm_num at eq

end SU2BoundaryMemory
