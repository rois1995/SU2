# Native adaptation — current handoff

Updated2026-10-31. Full explicit goal ACTIVE: strictly accepted ONERA M6 Euler
CFD→mesh→CFD→mesh at50k metric complexity, conservative admissible transfer and
usable isolated disk restart; then dedicated BL/constraint/MPI qualification and
corresponding RANS500k workflow. Actual point/cell counts are separate from metric
complexity. Strict geometry1e-4,q>=.2,J>=.05,maxedge1.8and requested heights.

## Authoritative current live state

**M6 Euler50k two-event workflow RUNNING:** exec2461,
`staged57/m6_runs/euler_build174_geometry_reuse50k`.
Driver2244727,time2244815,timeout2244816,mpirun2244817,ranks2244826–2244831.
All10exactidentities recorded in startup_verification.json(9children+driver_handle);
1396pins independently rehashed unchanged. C++/binary/helpers/config/inputs FROZEN
until terminal or preserved evidence justifies an authorized stop of verified owned
handles. Nice19/libs1/MPI6,native5400s/event,outer14400s. One owned heavy workload.
Initial61330points/303359cells; no accepted event yet. Coarsening completed at
312.718native seconds vs1661200.506s:292744cells/2582collapses,68372size-bad/
3541geometry-bad; all discrete/metric/geometry debts EXACTLYmatch166(all q/Junmeasured).
Geometry upper363.153vs1986.525rank-seconds; new facet57.796+scope50.078+selection
1.428s versus old scope1624.070(includesfacet). Facet live462.942MBvs114.506MB;
scope1136.910MBvs1170.806MB, both bounded. Observation is not isolated speedup or
accepted mesh. Comparison/coarsen_live_verification receipts saved;1396pins/all10
handles reverified. Construction size closesgen21/982.303s(maxedge1.7999928684783673),720958cells,
0size-bad/3293geometry-bad (old720963/3126); splitcounts differaftergen1, preserved
in construction_comparison_to166.json. Quality entry1020.205s(old3376.033):52432q-bad,
53564J-bad,3293geometry-bad, all720958cells measured/allflags1; qDebt8090.270020729,
JDebt14220.795466165(total22311.065486894). first_quality_assessment.jsonsaved.
Quality ownership actually moved360022cells/15.704s(allflags preserved), saved in
quality_ownership_assessment.json. First COMPLETE quality window1376.596987s,356.392safterentry:723488cells/allflags1,
52330q-bad/53326J-bad/3195geometry-bad/size0, q/Jdebt22032.921369789;
reduction1.246664%(.780445debt/s) vsoldfirst.783155%over760.286s(.230445/s).
Observed progress, different trajectory/ownership, not isolatedspeedup or acceptance.
Rank upper deltas57.472/25.215/38.417/232.961/29.846/89.160s; nested timers overlap.
PRIVATE boundary smoothing3515star dispatches/6248candidates/1597guidance/13708upper
queries/229moves/14.248debt/365.896sumseconds; interior331stars/2186candidates/
282moves/9.221debt/.504s. Negative-star reuse0; do not claim cachehit benefit there.
Private optimizer moves/debt may be rejected by outer transaction, separate from
whole-field progress. Second complete window1707.945s (687.740squality):724817cells,
52273q-bad/53170J-bad/3182geometry-bad/size0; q/Jdebt21809.127317422,
2.249727%fall/.729837debt/s. Rank3upper486.029of687.740s remains dominant.
quality_trend_assessment.jsonsaved; private progress only. Firstwindow livepin verification saved;1396pins/
10handles remainexactlive. Run continues frozen, no accepted mesh. Sampletree~6.49GiB before migration,
~26.2GiBavailable; ranks received~.94CPU/wall over648s, no isolatedtimingclaim.
Actual host quota2673639424/rank445606570before per-facet rounding. Config byte-
identical166, SHA5b6be56224fa8964532f92085659d63e2586ae1936d585fc30939c1ed69225a1.
Initial CFD restart byte-identical166, SHAee7ae4690ee4c890104ac1876e4eed85354e6014233e10733862dbc6c2f1d45f;
this is initial-state provenance ONLY, never adapted restart evidence.

Qualification checkpointbeee72c100pushed before launch; live checkpoint776f08432cpushed. Its commit spawned automatic
Git GC2243698/2243704/2243705; initial guard assumed common-git-dir cwd and aborted
before signals, corrected guard verified this worktree/group and sent SIGTERM only
to that housekeeping group. AllGC handles terminal(Z/absent), receipts in live run.
GC overlapped startup; preserve as timing limitation, no isolated startup speedup.
Use git -c gc.auto=0 for subsequent checkpoint commits/pushes to avoid automatic
competing repack. Do not stop healthy M6 merely for this recorded startup overlap.

Connected MPI6exec28854TERMINAL0/278.590s,198cases/rank PASS; all9handles absent,
1387pins independently rehashed. Summary staged57/geometry_reuse174_integration_summary.json.

Build174PASS54.150s,1296pins unchanged/all13sampled handles absent. Focused serial
`geometry_reuse174_serial_20261031T022425_2226738`PASS45cases/3674assertions,
46.510s, all1388pins unchanged/all4handles absent. Includes fixed1M-work noncoplanar
graph, holes/notch/separate bodies, cache/recomputed/refusal, smoothing/heights and
exact saved M6 queries. MPI uses previous166qualified connected filters plus
NativeMappedComponent3D/NativeCurvedAtlas3D/NativeGeometryBackend3D; hidden M6 query
replay is serial only. Build174CFD SHA256:
60ffadf8e8c68b6771b940e72b510b62088da6f560679cb825e8ff31477d3a13.

Latest pushedbeee72c100contains qualified174source, diagnostics and preserved failures.
Full M6 is now live; no accepted event/artifact/transfer/adapted restart yet.

## Connected candidate and evidence

Read [latest user specification + design/evidence](Handbook/ADAPTATION_GEOMETRY_QUERY_REUSE_20261031.md).
Shared coarsen/split/quality backend reuses per-facet distance searches and smaller
joined witnesses. Full exact support remains in lower-distance forest; witness
keys use exact immutable selected pieces/resolution/tolerance, answer keys retain
full support/coordinates. Active refs/accounting and transient admission preserved.
Boundary smoothing reuses unchanged private context/atlas with retained overlap
charge; exhaustive shape-only rejected-star cache invalidates on EVERY commit,
never after resource/validity/geometry/edge failures. Boundary/interior trial/reject/
guidance/move/debt/time counters are separate. These count PRIVATE optimizer work;
published operations and whole-field debt remain separate. No blind trial cuts,
boundary freezing, mandatory CAD, new scenario switch or gate relaxation.

Prepared plane lift bounds now restrict flat-source barycentric weights using
outward projected half-plane coefficients + admitted XY shift, then a simplex-box
dual bound on ±normal residual. Keeps finite footprint/holes, displacement, model
error, full-support distances and all caps. Primary proof + bounded read-only review.
This resolves graph certificate's fixed1Mwork failure; no cap increase.

Representative12M6 projected-star queries (older verifiedbuild48authority/data,
NOT exact166state/continuation/restart): payload bit-identical, all12passing classes
preserved and cached/recomputed equality. Query1.03543→.401954s OBSERVED, not isolated
speedup. New facet.0832373/witness.00954483/selection.00128230/traversal.306159s;
witness724of10756support pieces. Work334868→409280 (more cheap box checks), identical
repeatwork1060. No capture/replay RSS comparison. Receipts:
`staged57/geometry_capture168_baseline_receipt.json`,
`geometry_reuse174_query_comparison.json`, pinned serial174output.

One unsuccessful convex-hull-only fallback172was preserved then removed: same graph
failure, slower M6sample.173trace proves all2048model/128physical pieces form one
accepted graph/full witness, no footprint/storage refusal; fragmentation hypotheses
disproved for that case.174addresses loose plane extrapolation. Exact172/173source archives and trace receipts are preserved inbeee72c100.

An overbroad generic serial_full170was stopped after preserved assertion failures:
1388pins/4exactidentities verified, only timeout2146037signalled, terminal143/270.596s,
all4handles gone. NOT a passing campaign. Older source-envelope/count/timer-scope
expectations outside166qualified filters are assessed in active handbook/backlog;
do not fix unrelated fixtures or rerun generic serial_full in this cycle.
Relevant work-estimator fixture171nowexplicit16MiB + actual retained-piece assertion,
keeping strict work>dand rank-independent ownership checks; production quotas unchanged.
Serial171192/193cases passed (graph failed);174focused resolves that shared failure.

## Next actions

1. Let current M6 finish frozen unless preserved evidence establishes an authorized
   computational/construction/certification/transfer/CFD/resource stop. Observe exact
   owned handles, resources and matching stages; no second heavy job or C++ edits.
2. On failure, preserve full evidence and pins, stop only verified owned launcher
   if needed to prevent unchanged retries, verify ALL handles terminal, then fix
   the demonstrated blocker as one connected cycle and return to complete M6.
3. Compare matching coarsen/split/quality geometry work, useful debt/time progress,
   transition/cache costs and aggregate sampled memory with terminal166. Nested timers
   overlap; old scope timer included facets. Do not infer speedup from changed CPU
   service/quotas or equate target complexity to actual counts.
4. Require TWO actually accepted events, strict final meshes/reference publication,
   conservative/admissible transfer and finite continued CFD. After workflow terminal,
   isolated COPY of actual accepted final mesh/native_ref/matching solution, verify
   headers/coordinates/positive state, then actual RESTART_SOL=YES/ADAP_LOOP=NO run
   with distinct output basename. Initial61330point restart is not adapted evidence.
5. Then dedicated curved/junction/interface/subsequent-op/redistribution/height BL
   qualification under MPI, implement its demonstrated needs, and RANS500k workflow.

## Last actual M6 outcome / persistent risks

`euler_build166_consistent_merit50k` TERMINAL unsuccessful. All10owned handles absent,
1396pins unchanged. First event native time_budget before authorized stop of only
verified mpirun1804149. No accepted mesh/transfer/adapted restart. Full failure and
stop evidence under staged57/m6_runs/euler_build166_consistent_merit50k.
Construction matched164through21: size closes3339.282s/720963private cells. Quality
entry3376.033s:52476q-bad/53533J-bad/3126geometry-bad. Eventmax5412.469s; quality
1968.095s/158rounds/928commits only2.533%partial debt reduction;121unmeasured cells.
Rank0quality geometry upper1427.877s includes493.183scope-build; peers wait. Peak
sampled tree12.632GiB, steady8.13GiB, ample RAM; CPU contention .77–.97service limits
wall comparisons. Initial CFD one iteration is functional workflow, not converged flow.

[Supplied findings assessment](Handbook/ADAPTATION_FINDINGS_ASSESSMENT_20261031.md)
records queue/slots/short-edge hypotheses, startup-field scope and restart risks.
**Acceptance risk still open:** current-nodal tensor transport can change spatial P1
field after topology changes; identity preservation alone does not prove immutable
numerical target. Quantify/resolve required semantics before strict frozen-field
claim. Original conservative CFD donors are distinct. Loop resume can collide with
restart basenames and rejected events consume cycles; isolated restart must avoid
collision, two requested events never imply two accepted events.

## Operating rules / links

ReadAGENTS.md, [complete task](Handbook/ADAPTATION_INTEGRATED_NEXT_TASK.md),
[quality evidence](Handbook/ADAPTATION_QUALITY_CLOSURE_20261030.md),
[cache/ownership design](Handbook/ADAPTATION_CACHE_PHASE_20261030.md), active handbook.
One owned heavy workload,nice19,build2–4workers(helper3),MPI6bounded,libs1,TMPDIR4TB
compile-tmp. Verify terminal handles before pinned C++ edits. Reuse evidence with
source/build/binary/runtime/config/input provenance. Preserve unrelated dirty logs.
Regular pushes authorized tohttps://github.com/rois1995/SU2.git,
branchcodex/native-3d-bl; no approval pending. Use git -c submodule.recurse=false and
--ignore-submodules=all due unrelated broken Eigen worktree. Force-add ONLY whitelisted
evidence, never binaries/meshes/restarts/raw perf. Escalated host/git tools authorized.
Bounded efficient agentownership_surveyis done, no edits/jobs. Primary owns acceptance.
