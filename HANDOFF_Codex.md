# Native adaptation — current handoff

Updated2026-10-31. Full explicit goal ACTIVE: strictly accepted ONERA M6 Euler
CFD→mesh→CFD→mesh at50k metric complexity, conservative admissible transfer and
usable isolated disk restart; then dedicated BL/constraint/MPI qualification and
corresponding RANS500k workflow. Actual point/cell counts are separate from metric
complexity. Strict geometry1e-4,q>=.2,J>=.05,maxedge1.8and requested heights.

## Authoritative current state

Connected bounded-guidance175 QUALIFIED: buildPASS588.881s/1296pins/all sampled
build handles absent; focused serial51cases/3885assertions PASS46.257s/all4handles
absent/1388pins unchanged; MPI6 204cases/rank PASS278.518s/all9handles absent/
1387pins independently rehashed. Summary staged57/bounded_guidance175_integration_summary.json.
Full M6 Euler50k two-event attempt **RUNNING**, exec19304:
`staged57/m6_runs/euler_build175_bounded_guidance50k`.
Driver2378026,time2378128,timeout2378129,mpirun2378130,ranks2378134–2378139.
All10exactidentities live,1396pins rehashed unchanged,81actual mapped libraries
covered by pins. Source/helper/runtime/config/input FROZEN until terminal or
preserved evidence justifies authorized stop. Native5400s/event,outer14400s,
nice19/libs1/MPI6, one owned heavy job. Config/authority byte-identical174;
initial61330points/303359cells and restart00000byte-identical174 (initial state
ONLY, never adapted restart proof). Hostcachequota4294967296/rank715827882before
rounding vs1742673639424/445606570: same policy, more available host RAM. Preserve
this distinction: coarsen confirms effective prepared limits396836704bytes/rank
IDENTICAL after whole-facet rounding, with identical cache live/miss/eviction/work
counters. Coarsen315.301s vs174312.718s:292744cells/2582collapses,
68372size-bad/3541geometry-bad; ALL discrete and q/J/size/geometry debt totals match,
q/Jcorrectly unmeasured. Upperwork273877600/14578calls identical; upperseconds
366.587vs363.153sum. New counters2185early rejections/20695faces(coarsening already
stopped at first violation before175; no new coarsen speedup expected). All1396pins/
10exactlivehandles verified at milestone. ALL21split generations match174counts/
size/geometry debts; sizeclosesgen21/986.182s(vs982.303),720958cells,maxedge
1.7999928684783673. Quality entry1021.401956916s(vs1020.205), ALL global totals
EXACTLYmatch174:52432q-bad/53564J-bad/3293geometry-bad/size0, all720958measured/
allcompleteflags1; qDebt8090.270020728689,JDebt14220.795466165202,total22311.065486894.
Quality ownership committed360022cells/13.949710106s; full own-rank rows saved.
Run frozen in quality;1396pins and10exactlivehandles reverified atentry,~23.9GiB
available. Construction/entry/ownership/live receipts saved. No accepted mesh,
actual conservative transfer or adapted disk restart.
First COMPLETE quality window1229.956504567native/208.554547651quality seconds,
vs174356.391717597quality seconds: EXACT same counts and q/Jdebt22032.921369789,
278.144117fall/1.246664% (1.333676debt/s vs.780445).723488cells,52330q-bad/
53326J-bad/3195geometry-bad/size0, allmeasured/allflags1. PRIVATE boundary
3515stars/6248candidates/1597guidance/229moves/14.248debt IDENTICAL174; upperqueries
6957vs13708,882earlyrejections/10054evaluatedfaces,202.517sumseconds vs365.896.
Interior331stars/2186candidates/282moves/9.221debt/.503s; negative-star reuse0.
Rankqualityupper58.073/25.599/35.747/64.193/26.736/72.109s (oldrank3alone232.961).
Actual counts/debt/private work agree; no bitwise mesh identity or isolatedwall
benchmark claim. Firstwindow assessment/livepin receipts saved; run remains frozen
and healthy, no accepted mesh/actual transfer/adapted restart. Let full175attempt
continue generously; do not start a second optimization from this partial window.
Thirdcompletewindow1712.443314native/691.041357quality seconds, q/Jdebt
21515.464345743 (3.565949%fall/1.151308debt/s),727191cells,52182q-bad/52880J-bad/
3121geometry-bad/size0; counts/debt EXACTLYmatch174thirdwindow(1144.000quality seconds).
Trendreceipt saved; latest pushed8a04c97b25containsfirstwindow evidence.
Pinned-source transfer/restart acceptance preparation:175run/transfer_restart_acceptance_source_review.md.
Actual transfer/restart artifacts are still absent; this is read-only preparation.
CFD SHA45f176a7b09c60ff8497d7a5ea976fc504df13947f4fb3ae489f0c5468ff06f4.
No accepted Euler mesh/event/actual transfer/adapted disk restart yet.

Previous build174 M6 is TERMINAL after an authorized computational-bottleneck stop.
Exec2461 terminal1, all10 exact owned handles ABSENT, all1396 pins unchanged.
Run `staged57/m6_runs/euler_build174_geometry_reuse50k`; receipts
geometry_bottleneck_stop_receipt.json and terminal_handles.json. This was a
supervised performance stop, NOT native time_budget failure. No accepted mesh,
event, transfer or adapted restart. The current owned workload is175above.

174 preserved construction: coarsening312.718native seconds vs1661200.506 with
identical counts/debts; size closesgen21/982.303s,720958cells/maxedge1.7999928684783673.
Quality entry1020.205s vs1663376.033; ownership360022cells/15.704s.
Three COMPLETE quality windows end2164.205s/1144.000quality seconds:727191cells,
52182q-bad/52880J-bad/3121geometry-bad/size0; q/Jdebt21515.464345743,
3.565949%fall/.695456debt/s. Rank3upper789of1144seconds remains dominant.
PRIVATE boundary smoothing10166stars/18774candidates/4242guidance/35032uppercalls/
757moves/75.096debt/1130.549sumseconds; interior849stars/5641candidates/747moves/
28.834debt/1.223s. Negative-star reuse0. Private work is not global accepted debt.
Peak sampled SUM process-tree12.915GiB, not physical RAM or an individual peak.
Current code/input provenance and prior matched receipts remain in the run.
Startup automatic Git GC overlapped72.389s; verified own group was stopped and
allGC handles terminal. Use git -c gc.auto=0. No isolated speedup claim.

Qualified connected follow-up: bounded whole-patch fitting guidance. Existing
GeometryGuidanceExcess sums nonnegative face terms but SmoothBoundary computes
complete losing candidate scores. Preserve exact face order/arithmetic and old
complete score; stop only when finite partial sum already exceeds it. Partial
results must be explicit rejection-only, never accepted/cached as complete scores.
Retain all mandatory geometry/topology/body/features/heights and final certificates.
Reuse existing exact backend memo; no speculative second face cache or scheduling
framework. Qualify threshold equivalence/near ties/resources and existing curved/
seam/hole/body/height behavior serial/MPI, then immediately complete M6 retry.
Build175PASS588.881s/all1296pins unchanged/all sampled handles absent; CFD SHA
45f176a7b09c60ff8497d7a5ea976fc504df13947f4fb3ae489f0c5468ff06f4.
Serial/MPI175qualification is terminal/passed as above. Preserve qualified source
for immediate completeM6; no further implementation before its outcome.

Checkpoint3747bdd26f contains terminal174evidence and the initial candidate.
This qualification checkpoint records passing175serial/MPI and immediateM6next.
Qualification checkpointbeee72c100 contains174source and serial/MPI qualification.
CFD SHA60ffadf8e8c68b6771b940e72b510b62088da6f560679cb825e8ff31477d3a13;
configSHA5b6be56224fa8964532f92085659d63e2586ae1936d585fc30939c1ed69225a1.
All current goal obligations and metric-field/restart risks below remain open.

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

1. Let current175full M6 run frozen until terminal or preserved evidence justifies
   an authorized computational/construction/certification/transfer/resource stop.
   Verify exact handles; no second heavy job or pinned-source edits.
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
