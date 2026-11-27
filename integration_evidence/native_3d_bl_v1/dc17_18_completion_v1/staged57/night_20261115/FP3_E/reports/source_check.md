# FP3-E whole-item source check (R4)

Checked before the first production edit, against c53e7dbce2 and the authenticated FP3-E order. The tracked Common, SU2_CFD and UnitTests trees matched the base at intake. HEAD and its upstream reference both pointed to c53e7dbce2. The host process list and user services showed no owned job.

## Rank uniformity at the observation

- Worker enablement comes from the worker-wide step-C result in NativeRemeshRun.inc (pass success reduced on the worker communicator) and the probe value. The launcher checks the effective value on all participating ranks before accepting a run. Non-S paths leave the worker option false.
- The live queue flag is initialized from that worker option in EngineAdapt.cpp, restored from it in PreparePhaseDirection at a construction entry, and opened at StartMetricCleanup after the existing worker-wide guards. The ownership transition copies the flag unchanged into the replacement selection.
- The phase starts as Construction and repairing starts false. EnterPhase sets both from the same transition decision. The surface-only EnterSurfaceRepair transition is tied to a worker-wide SurfaceReady result and does not enable the feature. StartMetricCleanup clears repairing and marks metric cleanup through the common branch. Repair submodes leave these flags unchanged.
- The completed-round and stagnation window conditions are already common control boundaries. ObserveProgress has a worker failure vote. PhaseOwnership uses worker-wide time/refusal decisions and the rank-uniform cooldown, and either finishes the head or returns to the same post-ownership point on all ranks.
- The used flag starts false, can be set only from the common three-value SUM and deterministic window history, and survives EnterPhase. The same history/reset and request state therefore applies on all ranks. The local census validity bits are SUM inputs.

## Placement and checks

- Hold the optional plain state at invocation scope. Add its fixed bytes to the existing caller-bytes charge before boundary-parameter admission to cover both that admission and all downstream EngineExchange accounting. The closed-queue option and lab controls determine engagement. Session, cell/transport rows and timer fixed storage are unaffected.
- Observe the post-ownership Selection::Progress census. The array overload of CPassiveComm::Allreduce makes a single call. Its Communication scope follows TimedAllreduce; the disabled path reaches neither.
- ReturnToConstruction precedes the stagnation window in the head. Its four outer guards and its ordinary/no-candidate order can be retained with lazy predicates. Only an outstanding request extends the last clause. quality_exit and refine_geometry keep their existing order after the decision.
- Exact probe parsing is centralized in CNativeLocalCore3DMeasures.cpp. Ownership special-cases only FP3B for the closed queue. Existing Engine and Core probe reports match only those two enum values, so value 4 retains the D queue and policy with neither existing probe row.
- The stage trace contains round, phase, q/j debt, q/j bad and all three completeness fields. A quality migration replaces the window census with the same-round ownership_committed census. The scoped runner will retain the D safeguards and hidden tests will exercise the production predicate, reduction, guards and hand-off outcome. The two independently sealed S trials and isolated restarts are the acceptance path.

## Contradictions and readings used

1. The related EngineControl surface path treats EnterSurfaceRepair as an early exit from EnterPhase, with no assignment to phase there. Reset history at the start of EnterPhase, ahead of that early exit, as the order requires for every call. The existing surface path is ineligible.
2. Progress stores debt in long double, while the signed observation explicitly reduces doubles. Use the local long-double sum once before narrowing. The existing 17-digit stage debts supply the independent comparison within the specified relative tolerance.
3. A metric cleanup changes the live state before emitting metric_cleanup_entry. When it preempts a pending request, use that existing stage and the prior stall fire to report the preemption. The open queue emits neither new row. The next EnterPhase clears the request while retaining the used bit.
4. The one-run probe-4 observation says to report a timeout apart. The A helper retry applies to required steps 1–3. Use one launch for step 4 and report its terminal status separately.

No rank-local active guard or result contradiction requiring omission was found.
