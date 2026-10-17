# Original-family correction stages — IN PROGRESS, NOT VALIDATED

Current work after published d9daf0e78fdedaedcbd8fc62211dd5747c0d5f37.
Full Goal3 ACTIVE. Both binaries rebuilt; serial controls/actual two-stage RANS
lifecycle and independent geometry/full-field/BDF/SA/restart audits PASS. Sequential
MPI campaign running (session97657, at most4 ranks); no compiler alongside it.
No CFD time advancement or cluster run.
The previous checkpoint's binaries do NOT validate this working source.

The missing capability is an explicit correction between frozen remeshing stages.
A candidate can reassess its original field, but initial ComputeMetric discarded
unbounded frames; rescaling already clipped/graded donors would solve another field.

Implemented working draft, not qualified:

- CSolver captures original eigenframes/eigenvalues, original node IDs/coordinates,
  selected sensor values, original geometry authority/layers and bound/gradation/
  budget controls. Replays skip Hessian/sensor recovery, rerun existing clipping and
  gradation from original frames, use an external global assessor and restore the
  CFD metric and its diagnostics afterward. No original donor is refitted.
- ReassessComplexity optionally retains geometry/identity-only donor stencils,
  routed to original publishers. Exact long-double barycentric codec is shared
  without changing donor-sample wire order. Trials fetch current original-node
  values; original generated-collar IDs map back through the existing source span.
- Candidate assessment verifies source bindings, caches actual-point geometry,
  recomposes the BL/current core each trial and reduces stable global variance.
  Indicator includes empirical point/block SEM plus a first-order conditioning
  term; none is a proven uncertainty bound or calibrated default tolerance.
- Native remesher draft supports at most two private stages, preserves working
  candidate geometry and original source routing, shares mesh work/deadline, resets
  frozen source/index identity between stages and returns/transfers/publishes once.
- Driver wiring uses the current solver family and shares its unchanged geometric
  BL authority with remeshing. Strict successful initialization keeps its old path;
  the correction policy addresses provisional initialization, then requires a
  material reassessed discrepancy and an admissible scale direction. Bounds with
  no admissible direction report outside_reassessed_scale_bounds.

Finish/review the whole connected batch BEFORE a broad campaign:

1. Review MPI/allocation/exception paths and resource accounting, field/source
   identity, donor aliases, two-stage control flow and callback lifetimes.
2. Add focused replay tests: exact original frame reuse at saturation, changed
   nonhomogeneous/clipped field, restore source/Hessian/current+BDF state, stale/
   policy/mismatched-rank/empty-rank/codec faults, shared work/time limits.
3. Exercise actual accepted two-stage remesh and one final transfer/restart on
   N=M and N4/M2. Check stage-specific original fields; do not use candidate-node
   tensors as the new source. Reuse affected acceptance auditors.
4. Reuse frozen M6 evidence early: inspect original family/same-point field work and
   scale-bound classification without an unchanged300s construction rerun. Initial
   coarsest demand is already far above100000; correction cannot solve that
   construction failure. Do not claim accepted M6 or full Goal3 closure.
5. One connected build-j1/nice19/thread1; focused correctness pilot, then affected
   MPI/curved/RANS/2D checks. Preserve any failure and source/binary/input/helper
   identities. No test/checkpoint loop after each small edit. Observe actual host
   RAM/load before compute; one heavy job, MPI<=4. Announce full case folder.

Open current limitations: no compiled evidence; normalizer/frame/source/stage code
is a draft. Default fixed8192 samples and heuristic uncertainty are not accuracy
certification. Dense/sparse cost, snapshot memory, root trials, actual total event
cost and stage budget discrepancy must be measured. Root callbacks must not enter
MPI conditionally on local failure; source-original metrics must be restored even
when replay fails. Preserve all geometry/h0/positivity/q/J/full-edge/transfer gates.

Current validation preparation: original-frame/clipping and source reversion,
physical-source aliases with unused CFD IDs/empty publishers, conditioning scaling,
collective deadline/storage faults; actual two-stage RANS in both controlled initial
normalization-error directions, shared mesh work cap, one final transfer, SA/BDF and
real CGNS restart. Controlled errors are explicitly test injections, not natural M6
integration evidence. Independent old-field Duffy/coarea/mesh audits are prepared.
Initial missing ReferenceState include and sparse-neighbor size interface errors
are preserved in syntax logs; corrected affected interfaces pass. The full build
and runtime results, not syntax checks, must qualify the implementation.

Serial evidence now qualified in `frozen_normalization_stages_v1/pilot_summary.json`:
original-family mass21.76446 ->20.00000 +/-0.04471 (heuristic); independent19.99506.
Actual two-stage RANS controlled scales1/10 accept240/178 tetrahedra, work433/911,
root calls6/7; references19.93549/19.93633 versus target20 (~0.32% low). Full q/J/
edge/h0/geometry checks, exact sensor transport, BDF/SA, SU2/CGNS and restart pass.
The initial runtime fixture failed because its fresh work-cap CConfig lacked
mesh-bound marker ordering. Preserve `pilot_runtime_np1`; corrected marker tag/
kind/send-receive bindings pass in `attempt2_pilot_runtime_np1`. The shared work cap
rejects at77 used operations without CFD/reference publication. A report-wrapper
Path argument error occurred before any MPI run and is preserved separately.
Sealed exact serial native/audit evidence is reused by `mpi_campaign.py`; do not
rerun it while its pins/manifest remain valid.

Reporting limits: engine/BridgeStats counters describe the last stage, exclusive
phase times and cumulative_work span both. normalization.changed describes a
proposal; stages==2 establishes its application (a rejected shared-cap attempt
can report changed=true with only1 stage). Density/geometry query counters printed
by rank0 are local to that rank, not global counts. Full MPI/remaining checks and
final documentation/push are still pending.
