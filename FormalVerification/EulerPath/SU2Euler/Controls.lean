import SU2Euler.Composition
import Mathlib.Algebra.BigOperators.Fin

set_option maxRecDepth 100000
set_option maxHeartbeats 2000000

noncomputable section
namespace SU2Euler
open SU2 SU2Source Generated

/-- Two valid cells with one oriented unit-area edge; the domain is nonempty. -/
def testEdge : InteriorEdge 2 1 :=
  ⟨0, 1, by decide, ⟨fun _ => 1, by simp [dot]⟩, 1, by norm_num⟩
def testFlux : State 1 := fun _ => 3
def testLanes : List (Lane 2 1) := [(true, testEdge, testFlux), (false, testEdge, testFlux)]
def testVolumes : Fin 2 → ControlVolume 1 := fun _ => ⟨fun _ => 0, 1, by norm_num⟩
def testOld : Residual 2 1 := fun _ _ => 10
def testTrunc : Residual 2 1 := fun _ _ => 1

def TestContract (scatterCode : ScatterCode) (applyCode : Op) (negative : Bool) : Prop :=
  runStep scatterCode applyCode negative testVolumes (fun _ => 1)
    testOld 0 testTrunc testLanes id =
  stepSpec testVolumes (fun _ => 1) testOld 0 testTrunc (activeEvents testLanes) id

theorem positive_control : TestContract scatter_originalCode apply_originalCode call_originalCode :=
  composed_step_correct _ _ _ _ _ _ _

theorem refactor_control : TestContract scatter_refactoredCode apply_originalCode call_originalCode := by
  rw [scatter_refactor_ir]
  exact positive_control

theorem wrong_sign_ir : scatter_wrongSignCode = ⟨true, .add, .add⟩ := by decide
theorem wrong_mask_ir : scatter_wrongMaskCode = ⟨false, .add, .sub⟩ := by decide
theorem wrong_trunc_ir : apply_subtractTruncCode = .sub := by decide
theorem wrong_call_ir : call_wrongSignCode = false := by decide

/-- All four mutants parse successfully; each violates the same composed contract. -/
theorem wrong_sign_rejected : ¬ TestContract scatter_wrongSignCode apply_originalCode call_originalCode := by
  intro h
  have h := congrFun (congrFun h (1 : Fin 2)) (Channel.mass : Channel 1)
  norm_num [runStep, stepSpec, wrong_sign_ir, apply_source_ir, call_source_ir,
    scatterLoop, scatterLane, activeEvents, List.filterMap_cons, testLanes, testEdge, testFlux,
    testVolumes, testOld, testTrunc, opReal, assemble, edgeContribution, Function.update] at h

theorem wrong_mask_rejected : ¬ TestContract scatter_wrongMaskCode apply_originalCode call_originalCode := by
  intro h
  have h := congrFun (congrFun h (0 : Fin 2)) (Channel.mass : Channel 1)
  norm_num [runStep, stepSpec, wrong_mask_ir, apply_source_ir, call_source_ir,
    scatterLoop, scatterLane, activeEvents, List.filterMap_cons, testLanes, testEdge, testFlux,
    testVolumes, testOld, testTrunc, opReal, assemble, edgeContribution, Function.update] at h

theorem wrong_trunc_rejected : ¬ TestContract scatter_originalCode apply_subtractTruncCode call_originalCode := by
  intro h
  have h := congrFun (congrFun h (0 : Fin 2)) (Channel.mass : Channel 1)
  norm_num [runStep, stepSpec, scatter_source_ir, wrong_trunc_ir, call_source_ir,
    scatterLoop, scatterLane, activeEvents, List.filterMap_cons, testLanes, testEdge, testFlux,
    testVolumes, testOld, testTrunc, opReal, assemble, edgeContribution, Function.update] at h

theorem wrong_call_rejected : ¬ TestContract scatter_originalCode apply_originalCode call_wrongSignCode := by
  intro h
  have h := congrFun (congrFun h (0 : Fin 2)) (Channel.mass : Channel 1)
  norm_num [runStep, stepSpec, scatter_source_ir, apply_source_ir, wrong_call_ir,
    scatterLoop, scatterLane, activeEvents, List.filterMap_cons, testLanes, testEdge, testFlux,
    testVolumes, testOld, testTrunc, opReal, assemble, edgeContribution, Function.update] at h

theorem local_time_not_conservation :
    (∑ i, (testVolumes i).measure *
      runStep scatter_originalCode apply_originalCode call_originalCode
        testVolumes (fun i => if i = 0 then 1 else 2) testOld 0 0 testLanes id i .mass) ≠
      ∑ i, (testVolumes i).measure * testOld i .mass := by
  rw [composed_step_correct]
  norm_num [stepSpec, Fin.sum_univ_succ, testVolumes, testOld, activeEvents, List.filterMap_cons,
    testLanes, testEdge, testFlux, assemble, edgeContribution]

/-- Parser boundary controls do not replace the semantic mutant checks above. -/
theorem reject_extra_write : parseScatter (scatter_originalSource ++ " vec_val[0] = 0;") = none := by decide
theorem reject_directive : parseScatter ("#define BAD\n" ++ scatter_originalSource) = none := by decide
theorem reject_unclosed_comment : lex "/* unfinished" = none := by decide
theorem reject_caller_branch : parseCall "case EULER_EXPLICIT: if(Res) nodes->AddSolution(iPoint,iVar,-Res*Delta); break;" = none := by decide

/-- The migrated scalar gate retains all three accepted semantic counterexamples. -/
theorem migrated_subtract_rejected : ¬ WitnessContract update_subtractCode := by
  have h : update_subtractCode = SU2Source.Generated.subtractProgram := by decide
  rw [h]
  exact subtract_rejected

theorem migrated_current_rejected : ¬ WitnessContract update_currentReadCode := by
  have h : update_currentReadCode = SU2Source.Generated.currentReadProgram := by decide
  rw [h]
  exact currentRead_rejected

theorem migrated_target_rejected : ¬ WitnessContract update_wrongTargetCode := by
  have h : update_wrongTargetCode = SU2Source.Generated.wrongTargetProgram := by decide
  rw [h]
  exact wrongTarget_rejected

end SU2Euler
