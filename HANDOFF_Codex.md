# Native adaptation — current handoff

Updated2026-10-31. Full explicit goal ACTIVE: strictly accepted ONERA M6 Euler
CFD→mesh→CFD→mesh at50k metric complexity, conservative admissible actual transfer
and usable isolated adapted disk restart; then dedicated BL/constraint/MPI
qualification and corresponding RANS500k workflow. Actual point/cell counts are
separate from complexity. Strict geometry1e-4,q>=.2,J>=.05,maxedge1.8and requested
heights. Construction keeps exact positive/nondegenerate/topology/body/features/
height validity and intentionally omits q/J; CAD remains optional.

## Authoritative current live state

**Full M6 Euler50k two-event workflow RUNNING**, exec19304:
`staged57/m6_runs/euler_build175_bounded_guidance50k`.
Driver2378026,time2378128,timeout2378129,mpirun2378130,ranks2378134–2378139.
Exact10identities/startticks in startup_verification.json;1396source/runtime/input
pins independently verified at startup, coarsen, entry and first/third windows.
81actually mapped libraries all covered by pins. C++/binary/helpers/config/inputs
FROZEN until terminal or preserved evidence justifies authorized stop. One owned
heavy workload,nice19,libs1,MPI6,native5400s/event,outer14400s,two actual events.

Qualified source checkpoint924f531ac3; bounded-guidance candidate3747bdd26f.
Build175PASS588.881s/1296pins/all sampled build handles absent; serial51cases/
3885assertions PASS46.257s/1388pins/all4handles absent; MPI6 204cases/rank PASS
278.518s/1387pins/all9handles absent. Supporting qualification, not capability.
Summary `staged57/bounded_guidance175_integration_summary.json`.
CFD SHA45f176a7b09c60ff8497d7a5ea976fc504df13947f4fb3ae489f0c5468ff06f4.
ConfigSHA5b6be56224fa8964532f92085659d63e2586ae1936d585fc30939c1ed69225a1,
byte-identical174; authority18/SHA2e99fb7870e2c718f110da8509e4c0f7e746ce138e7f27ab57aa0dedbb6c42de.
Initial61330points/303359cells and restart00000byte-identical174:
ee7ae4690ee4c890104ac1876e4eed85354e6014233e10733862dbc6c2f1d45f.
That initial restart is NOT adapted transfer/restart evidence.

## Actual progress and comparison

Coarsen315.301native seconds vs174312.718:292744cells/2582collapses; ALL counts,
size/geometry debts, uppercalls14578/work273877600 and cache live/miss/eviction
counts identical. Initial reported hostquota4GiB/rank715827882before rounding
vs1742673639424/rank445606570; effective preparedpool396836704bytes/rank is
IDENTICAL after whole-facet rounding. Do not infer a retention difference from
initial quota labels. CPUservice~.94/core, observed timings not isolated benchmark.

ALL21split generations match174counts and size/geometry debts exactly. Size closes
986.182s(vs982.303),720958cells,maxedge1.7999928684783673. Quality entry1021.402s
(vs1020.205): all720958measured/allcompleteflags1,52432q-bad/53564J-bad/
3293geometry-bad/size0; qDebt8090.270020728689,JDebt14220.795466165202,
total22311.06548689389. ALL global entry totals match174. Quality ownership
actually moved360022cells once/13.950s, all staged flags preserved; no CFD projection.

First3complete quality windows reproduce174counts/debts in691.041quality seconds
versus1144.000. Firstwindow alone208.555vs356.392s, same1.246664%whole q/J debt
fall. PRIVATE boundary3515stars/6248candidates/1597guidance/229moves/14.248debt
identical; unioncalls6957vs13708,882earlyrejections/10054evaluatedfaces,
202.517sumseconds vs365.896. Interior331stars/2186candidates/282moves/9.221debt/
.503s. Negative-star reuse0: no measured benefit from that cache. Private work can
be rejected by the outer transaction; don't equate it to whole-field progress.

Latest preserved complete window13: native3294.526367s,
quality2273.124410s; q/Jdebt19497.613547003,
12.610119%fall/1.237703debt/s.
736954private cells,50180q-bad/50313J-bad/
2821geometry-bad/size0, allmeasured/all4completeflags1.
Lastintervaldebt/s2.114127; continue generously.
No accepted event/mesh/actual transfer/adapted restart yet. Run remains healthy.
Sampled treeRSS8.51GiB, peak summed tree12.922GiB,~24GiBavailable; not physical
memory or an individual peak. Resource/CPU/paging receipts retained in175run.

One bounded2s49Hz profile526samples/0lost: inclusiveAllreduce74.37%,surfaceunion
11.82%,guidance7.35%,boundarysmooth5.85%. Shares overlap; NOT phase totals or pure
protocol costs. Active fitting/exact geometry plus waiting, not all-rank deadlock.
Initial inline report timed out45s with DWARF errors; no-inline self/inclusive
reports passed<.3s. This is a diagnostic failure, not CFD failure. Capture/report
CPU overhead is a timing limitation; raw perf stays LOCALONLY. Profile context,
assessment and text reports saved; do not repeat a baseline campaign.

Checkpointf136c64f1f contains four-window trend, bounded profile and compact handoff;
c730d9f8ed contains the pinned-source transfer/restart acceptance review.
All detailed current design, failures and previous qualification are linked below.

## Immediate actions and acceptance

1. Let current175full attempt continue frozen generously to its outcome unless
   preserved evidence establishes an actual error/resource/clear computational stop
   with a concrete connected fix. Do not stop merely from linear debt projection.
   Verify exact owned handles on escalated HOST /proc; sandbox namespaces may hide
   live host PIDs. Stale logs/observation timeout are not termination.
2. Preserve complete quality windows, source/binary/runtime/input provenance and
   resources. Nested guidance/union/smoothing/MPI timers overlap. No new C++ cycle
   or heavy workload while this attempt is live. Additional ideas remain backlog.
3. On a demonstrated failure, preserve the primary event outcome before stopping
   only verified owned launcher2378130 if necessary to prevent unchanged retry;
   verify ALL10terminal and pins before any pinned-source edits. Then one connected
   fix/qualification and immediate complete M6 retry. Diagnostics are not completion.
4. TWO actually accepted events, published mesh/native_ref pairs, conservative
   admissible transfer and finite continued CFD are required. Inspect actual raw
   per-field residuals (double gate1e-10), final admissibility and integral changes.
   Review175run/transfer_restart_acceptance_source_review.md; it is preparation,
   not transfer evidence. Post-CFD integral changes include flow evolution.
5. After full workflow terminal: isolated COPY of accepted final mesh/native_ref/
   matching solution; pin all. Verify header, point count/order/coordinates, finite
   rho/momentum/energy and positive internal energy, then ACTUAL CFD continuation
   RESTART_SOL=YES,ADAP_LOOP=NO,correct steady SOLUTION_FILENAME and distinct
   RESTART_FILENAME/output basename. Initialrestart00000does not qualify.
6. Then dedicated curved/junction/interface/subsequent-op/redistribution/requested-
   height BL qualification under MPI, implement demonstrated BL needs, and RANS500k.

## Preserved outcomes and correctness risks

Previous174TERMINAL supervised computational-bottleneck stop after3completequality
windows: only3.565949%debt fall in1144quality seconds, rank3upper789/1144s. All10
handles absent/1396pins unchanged; primary outcome NOT native timeout. Its full
receipts remain under euler_build174_geometry_reuse50k. Earlier166TERMINAL first-
event time_budget onall6ranks before authorized launcher stop; no accepted mesh.
175must not be confused with those historical handles or build numbers.

Numerical frozen-field risk OPEN: current-nodal tensor transport changes spatial
P1 representation after topology edits; identity preservation alone does not prove
an immutable numerical target. Quantify/resolve required semantics before such a
claim. Original conservative CFD donors are separate. InitialCFDITER1is endorsed
functional startup field, not flow-converged M6 accuracy. Resumed loop can collide
with numbered restart paths; rejected events consume cycles. Isolated restart
avoids collisions but does not claim general adaptation-resume numbering fixed.

Overbroad serial_full170was NOT passing; out-of-contract source-envelope/count/
timer expectations remain backlog. Removed convex-only172fallback and173diagnostic
sources/failures are archived.174overlap-conditioned plane lift passed fixed1Mcap;
175preserves it and all latest validity-first construction/whole-patch merit work.

## Design and operating links

ReadAGENTS.md and [complete integration task](Handbook/ADAPTATION_INTEGRATED_NEXT_TASK.md).
[Active geometry/smoothing design and evidence](Handbook/ADAPTATION_GEOMETRY_QUERY_REUSE_20261031.md)
contains latest user instructions verbatim, full174/175changes, proofs and failures.
[Findings assessment](Handbook/ADAPTATION_FINDINGS_ASSESSMENT_20261031.md),
[quality history](Handbook/ADAPTATION_QUALITY_CLOSURE_20261030.md),
[cache/phase ownership design](Handbook/ADAPTATION_CACHE_PHASE_20261030.md),
[backlog](Handbook/ADAPTATION_BACKLOG.md). Historical live notes do not override here.

One heavy job,nice19,build2–4workers(helper3),MPI6bounded,libs1,TMPDIR4TBcompile-tmp.
Reuse evidence only with relevant source/build/binary/runtime/config/input/helper
provenance. Forkpushes authorized:https://github.com/rois1995/SU2.git,
branchcodex/native-3d-bl. Use git -c gc.auto=0 -c submodule.recurse=false and
--ignore-submodules=all due unrelated broken Eigen worktree. AutoGC once competed
with174startup and was verified/stopped; do not trigger it again. Whitelist adds;
never sweep unrelated dirty155logs, binaries, meshes, restarts or rawperf. Escalated
host/git tools authorized; no permission pending. Efficient bounded ownership_survey
agent is DONE, no jobs/edits; primary owns integrated design and acceptance.
