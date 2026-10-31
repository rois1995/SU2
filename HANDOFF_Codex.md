# Native adaptation — current handoff

Updated 2026-10-31. Full goal ACTIVE: strictly accepted ONERA M6 Euler
CFD → mesh → CFD → mesh at 50k metric complexity, conservative admissible actual
transfer and usable adapted disk restart; then dedicated BL/constraint/MPI
qualification and the corresponding RANS500k workflow. Report actual points/cells
separately from complexity. Strict geometry1e-4, q>=.2, J>=.05, maxedge1.8 and
requested heights. Construction keeps exact positive/nondegenerate/topology/body/
feature/height validity and intentionally omits q/J. CAD remains optional.

## Authoritative current live state

**Full M6 Euler50k two-event retry RUNNING**, exec16090:
`staged57/m6_runs/euler_build179_quality_service50k`.
Driver2685167, time2685263, timeout2685264, mpirun2685265, ranks2685269–2685274.
Exact ten identities/startticks in startup_verification.json. All ten verified live
on host /proc at the latest observation. All1396source/build/runtime/input pins
independently verified at startup and completed coarsening; all81actually mapped
libraries covered. C++/binary/helpers/config/inputs FROZEN until terminal or a
preserved authorized stop. One heavy job, nice19, libs1, MPI6, native5400s/event,
outer14400s. Two ACTUALLY accepted events are required.

Qualified checkpoint6ea65d9141; productionc132020654, test contractsa0e5e3bec7.
CFD SHAe8f81c5a941f6d03fea81c6437e9f4f6d4305044e55e6de4d6a2113bd0116072.
Config SHA5b6be56224fa8964532f92085659d63e2586ae1936d585fc30939c1ed69225a1,
authority18/SHA2e99fb7870e2c718f110da8509e4c0f7e746ce138e7f27ab57aa0dedbb6c42de;
both byte-identical175. Initial61330points/303359cells; initial restart SHA
ee7ae4690ee4c890104ac1876e4eed85354e6014233e10733862dbc6c2f1d45f is identical175.
All six initial partition cell/metric/geometry workload fields match175.
`initial_field_partition_assessment.json` is initial-field provenance, NOT adapted
transfer/restart evidence.

## Current progress and qualification

Coarsening complete:312.050native seconds vs175315.301. All six ranks match175
counts, size/geometry debts, query/cache work and prepared limits exactly:
292744private cells,2582collapses,68372size-bad/3541geometry-bad cells;
uppercalls14578/work273877600. q/J remain intentionally unmeasured.
Prepared limit396836704bytes/rank matches175. No accepted mesh yet.
ALL21split generations match175counts/debts/geometry work on all six ranks.
Size closure982.301native seconds vs175986.182,720958cells/size0. Quality entry
1031.098vs1751021.402: all720958measured/allcompleteflags1,52432q-bad/
53564J-bad/3293geometry-bad/size0; q/Jdebt22311.06548689389. ALL entry counts,
debts and geometry/cache work match175. Ownership moved360022cells once in13.998s.
Current phase: quality. First COMPLETE window:516.883quality seconds,
15496.059051234q/Jdebt,30.545410%fall,13.184824debt/s.729578cells,
46969q-bad/47848J-bad/3206geometry-bad/size0, all measured/allflags1.
Actual2201candidates/2045reserved/1800commits over46batch rounds. Baseline175
complete windows bracket this time:2.2497%fall at420.146s and3.5659%at691.041s.
Upper-query work per whole-debt reduction95001vs1751215625(~12.8xlower),
while total query work647.4Mvs338.1M over different window workloads.
This is improved useful progress, NOT strict closure. First-window assessment
verifies1396pins/ten live identities/81mapped libraries. See quality_window1_*
and quality_trend_assessment.json. Continue frozen generously.
One2s49Hz profile520samples/0lost confirms active fitting/exact predicates and
parameter publication plus waits; Allreduce76.27%inclusive overlaps other shares.
No all-rank deadlock during sample; not phase-wide cost/progress evidence.
Profile context/assessment/text reports in179run; raw perf remains LOCALONLY.
Capture/report overhead is a timing limitation, not subtracted.
Latest observation treeRSS~8.47GiB, ~23.9GiB host available; sampled sum peak
11.685GiB. Summed RSS is not physical memory or an unsampled whole-job peak.
Resources/owned_observations retained. See split_generation_assessment.json and
quality_entry_assessment.json. The new batching affects this quality phase.
See179run/coarsen_assessment.json. Timings are observations, not isolated benchmarks.

Quality-service179 qualification: build PASS111.227s/1296pins/all9handles absent;
CFD identical178. Serial29cases/8169assertions PASS8.052s/1386pins/all4handles
absent. MPI6PASS212cases/rank,280.579s/1387pins/all10handles absent; assertion
counts in log order30816/31348/33848/31291/30821/30584.17late-loaded MPI-test
hwloc libraries additionally verified by mapped device/inode, pre-campaign file
metadata and terminal hashes. Actual M6 startup has no missing mapped libraries.
`staged57/quality_service179_integration_summary.json` links complete receipts.
These checks support the full target; they do not establish M6 or BL capability.

Connected change: one oldest-demand slot then cached required-debt priority;
bounded independent full-star repair batches; explicit distributed reservation
priority; per-slot current geometry/metric publication and private context; one
combined changed-frontier measurement and blocked-star invalidation before next
selection. Large coordinate stars, all gates and transient admission preserved.
Failed serial17827/29 is preserved. Two stale first-layer assertions were corrected:
zero construction shape screens; private metric bootstrap may fill sensor tensors
while preserving exact mesh attributes. New tensor/mesh/height/input checks pass.
Build176compile failure and177/178build evidence are preserved. No production gate
changed. Source design/review and detailed history are linked below.

## Next actions and acceptance

1. Let179continue frozen generously to its outcome unless preserved evidence
   establishes an actual error/resource/clear computational stop. Verify exact
   owned handles on escalated host /proc; sandbox namespaces can hide live PIDs.
   Stale logs or an observation timeout do not establish termination.
2. Preserve complete split generations, quality-entry census and complete quality
   windows. Compare counts/debt and geometry work at matching stages. Batch rounds
   now serve multiple stars, so window number alone is not equivalent work.
   Judge whole-field debt reduction per second; nested geometry/smoothing/MPI
   timings overlap. No new baseline campaign or unrelated implementation cycle.
3. On failure, preserve primary outcome and raw six-rank rows before any authorized
   owned-only stop. If necessary stop only verified launcher2685265 to prevent an
   unchanged retry after first-event failure. Verify ALL ten terminal and all pins
   before any pinned C++/helper edits. Then one connected fix, qualify, retry M6.
4. Require TWO actual accepted events, published mesh/native_ref pairs, conservative
   admissible transfer and finite continued CFD. Inspect raw per-field residuals
   (double gate1e-10), final admissibility and integral changes. Review175run/
   transfer_restart_acceptance_source_review.md: source preparation is not actual
   transfer evidence. Post-CFD integrals include flow evolution.
5. After full workflow terminal, isolated COPY final accepted mesh/native_ref and
   matching solution, pin everything. Check header, point count/order/coordinates,
   finite rho/momentum/energy and positive internal energy, then ACTUAL CFD
   continuation with RESTART_SOL=YES, ADAP_LOOP=NO, correct SOLUTION_FILENAME and
   distinct RESTART_FILENAME/output. Initial restart00000 does not qualify.
6. Then dedicated curved/junction/interface/subsequent-operation/redistribution/
   requested-height BL qualification under MPI, implement demonstrated BL needs,
   and complete RANS500k. No Euler/RANS reconstruction switches.

## Preserved failure and open correctness obligations

175TERMINAL: first event naturally time_budget on all six before authorized
launcher stop; all ten absent and1396pins unchanged. No accepted mesh/transfer/
adapted restart. Last COMPLETE window32:751803cells,46537q-bad/45853J-bad/
2521geometry-bad/size0; q/Jdebt16512.529724794,25.9895%fall in4290.944quality
seconds. Exit had95unmeasured cells and is PARTIAL. Primary failure is quality/
geometry closure within budget. Receipts:175run/terminal_assessment.json,
terminal_handles.json,event1_time_budget_stop_receipt.json.175reduced geometry
cost (first three windows691vs1741144s) but did not deliver an accepted mesh.
Older174supervised bottleneck stop and166natural time-budget rejection remain
archived; do not confuse their handles with179.

Numerical frozen-field risk OPEN: current-nodal transport changes the spatial P1
representation after topology edits; identity alone does not prove an immutable
numerical target. Quantify/resolve before claiming that semantics. Original CFD
conservative donors are separate. ITER1 is endorsed functional startup, not a
flow-converged aerodynamic qualification. Resumed adaptation can collide with
numbered restart paths; rejected events consume cycles. Isolated disk restart
does not claim general adaptation-resume numbering fixed.

Overbroad serial_full170 was NOT passing; out-of-contract source-envelope/count/
timer assertions remain backlog. Qualified filters are not the whole repository
suite. Removed172convex fallback and173fragmentation hypothesis remain historical.
174prepared-subtree/overlap-lift and175bounded-guidance construction improvements
are preserved in179.

## Design and operating links

Read [AGENTS](AGENTS.md), [complete integration task](Handbook/ADAPTATION_INTEGRATED_NEXT_TASK.md),
[active quality-service design/evidence](Handbook/ADAPTATION_QUALITY_SERVICE_20261031.md),
[geometry/smoothing design and evidence](Handbook/ADAPTATION_GEOMETRY_QUERY_REUSE_20261031.md),
[findings assessment](Handbook/ADAPTATION_FINDINGS_ASSESSMENT_20261031.md),
[quality history](Handbook/ADAPTATION_QUALITY_CLOSURE_20261030.md),
[cache/phase ownership design](Handbook/ADAPTATION_CACHE_PHASE_20261030.md),
[backlog](Handbook/ADAPTATION_BACKLOG.md). Historical live notes do not override here.
All staged57 paths above are under integration_evidence/native_3d_bl_v1/
dc17_18_completion_v1/. Previous175qualification is linked in the geometry handbook.

One owned heavy job, nice19, build2–4workers(helper3), MPI6 bounded, libs1,
TMPDIR4TBcompile-tmp. Preserve actual source/build/binary/runtime/config/input/helper
provenance. Fork pushes authorized: https://github.com/rois1995/SU2.git,
branch codex/native-3d-bl. Use git -c gc.auto=0 -c submodule.recurse=false and
--ignore-submodules=all due unrelated broken Eigen worktree. Whitelist adds;
leave unrelated155logs/binaries/meshes/restarts/rawperf untouched. Efficient
ownership_survey/quality_queue_survey agents are DONE, no jobs/edits; primary owns
integrated acceptance. No permission pending.
