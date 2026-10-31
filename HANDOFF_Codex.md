# Native adaptation — current handoff

Updated 2026-10-31. Full goal ACTIVE: strictly accepted ONERA M6 Euler
CFD → mesh → CFD → mesh at 50k metric complexity, conservative admissible actual
transfer and usable adapted disk restart; then dedicated BL/constraint/MPI
qualification and the corresponding RANS500k workflow. Report actual points/cells
separately from complexity. Strict geometry1e-4, q>=.2, J>=.05, maxedge1.8 and
requested heights. Construction keeps exact positive/nondegenerate/topology/body/
feature/height validity and intentionally omits q/J. CAD remains optional.

## Authoritative current live state

**Original-grid M6 Euler CFD continuation RUNNING**, exec80366:
`staged57/m6_seed_runs/original_grid_cfd179_v3`.
Python3013761,time3014065,timeout3014066,mpirun3014067;
ranks3014072,3014075,3014078,3014081–3014083. Exact ten identities/startticks in
startup_verification.json, all verified live;1405pins unchanged,81actually mapped
libraries all covered. C++/binary/helpers/config/inputs FROZEN. Same179CFD and
original61330points/303359cells, nice19,MPI6,libs1,ADAP_LOOP=NO,COMPUTE_METRIC=NO.
Reads copied baseline_euler_resume_input.dat from validated v2checkpoint1877
(exact hash in resume_source.json and v2resume_checkpoint.json). FixedCFL5,no adaptation,linearFGMRES cap20/error1e-6,
MG0. Local3122update allowance plus original1878updates = total5000; outer3600s.
Qualification unchanged:rho log<=-6,all5residuals down>=4orders from ORIGINAL
freestream history,stable final200CL/CD,exact coordinate order,finite positive
rho/internal energy. No qualified seed or accepted mesh yet. Live history must
combine prior_history.csv + history.csv using command.resume_offset1878; using
only current history would incorrectly reset the residual reference.
At local59(global1937),rho log-5.59018,CL.269536,CD.0135358;linear7iterations,
logresidual-6.6603,AVG_CFL5. Latest hostload9.71/11.14/14.33,MemAvailable~31.6GiB.
One owned heavy job. Numbered restarts every100plus final; no earlier file overwrite.

v2TERMINAL after authorized convergence-stagnation stop. Five complete200-step
windows600–1599:mean rho log-5.60to-5.38(no improvement),CD ranges3.42e-5to5.99e-5
above~1.35e-5 allowance. Evidence:convergence_stability_assessment.json and
convergence_stop_receipt.json. Only verified launcher2953589 SIGTERM. All ten
identities ABSENT and1395pins unchanged BEFORE helper edit;terminal_handles.json.
Latest complete numberedcheckpoint1877independently coordinate/admissibility
validated(61330points,minrho.627664,mininternal103998.683),same original grid.
Saved qualification_history.csv,resume_checkpoint.json,and exact historical helper
source_snapshots/run_m6_seed.py. v2failure/exit1 is preserved; no convergence claim.
No restart from freestream. Continuation code fixes zero-based history joining so
the first resumed update is retained and the original residual reference survives.
m6_seed_continuation_checks.json supports config/history/provenance/state checks;
actual continued convergence remains pending. No production C++ changes/builds.

New run_m6.py REQUIRES a qualified seed directory, copies baseline_euler_seed.dat
and sets RESTART_SOL=YES with separate restart_flow output. Before/after first
remeshing uses the same deterministic ITER1 continuation from that saved seed.
Checks: m6_seed_input_checks.json; config/history gates and real61330point
coordinate/admissibility positive/negative checks PASS. These checks do not prove
convergence, seed resume, accepted adaptation or adapted disk restart. First seed
launch v1 terminated during manifest setup (str.resolve bug), before any CFD
launch; v2 corrected Path normalization and verified live manifest/runtime pins.
No C++ changes/builds before the requested unchanged-code saved-CFD baseline.
Latest user requests reuse of saved CFD near original mesh if available. Search
original /media/rausa/4TB/Verification/OneraM6 found only Meshes/NewMeshes, no
CFD configs/restarts/histories. Nearby CGNS metadata check is recorded in
179seed-v2/nearby_solution_search.json. Preserved local flow_checkpoint is
unconverged120iteration RANS/SA/no-slip, incompatible with current Euler slip-wall
seed; leave it intact for possible later RANS work. Continue current Euler solve
without discarding progress. Its qualified saved field is reused for all first
remeshes. Current convergence state is reported above; no qualified seed yet.

**Build179 TERMINAL after latest explicit user-authorized stop because it uses
ITER1 startup CFD, not a reasonably converged solution**, exec16090 exit1.
Run: `staged57/m6_runs/euler_build179_quality_service50k`.
All ten exact owned identities independently verified ABSENT on host /proc;
all1396source/build/runtime/input/helper pins unchanged. Receipts:
terminal_handles.json,user_authorized_stop.json,status.json. First event naturally
failed native time budget before the user-authorized stop during the next event.
No accepted mesh/transfer/adapted restart. Production CFD179 remains unchanged.

Latest user supersedes prior no-stop/no-new-baseline restrictions: stop the
startup-field run; obtain and save a reasonably converged original-grid Euler
solution; then compare BEFORE and AFTER implementation using that exact same
saved seed, and base subsequent results on this new baseline. Current heavy job is saved-checkpoint CFD continuation with current179binary;
next is the unchanged-code full M6 baseline. Do not implement the repair/ownership change before preserving that
baseline's terminal evidence. One owned heavy job, nice19, libs1, MPI6; native
budget5400/event remains unchanged. See linked connected plan for convergence
criteria, baseline order and complete goal.

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
First event naturally rejected at max5403.717native seconds: metric update exhausted
elapsed-time budget. All six complete=0/final=0. Last COMPLETE census7:
3960.659quality seconds,70.302200%q/Jdebt fall;752202cells,34386q-bad/
33995J-bad/2854geometry-bad/size0;13883commits. Partial exit census is invalid:
zero printed bad/debt counts mean UNKNOWN, not closure. First-event receipt:
179run/event1_natural_failure.json. The driver proceeded to its next event; the user then explicitly authorized
stopping because the initial CFD field was not reasonably converged. Verified
launcher2685265 received SIGTERM; all ten identities are now absent.
No accepted mesh/actual transfer/adapted restart. Latest treeRSS8.296GiB,
MemAvailable~24GiB, hostload7.44/8.52/8.84; no observed RAM pressure.

Latest user additions: next milestone MUST combine parallel private repair slots
(one round publication) with debt-directed work, full-support invalidation and
passive per-census diagnostics; no new config/scheduler/time-budget increase,
no further perf/GDB. See the linked parallel-repair plan. Before the next full M6,
obtain a reasonably converged original-grid Euler solution, save/pin it and reuse
it for EVERY future first remesh. Initial ITER1 field is now superseded as the
production input requirement. Build179 keeps its original input untouched.

Earlier first window:30.545410%fall in516.883quality seconds. Upper-query
work/debt95001vs1751215625(~12.8xlower); different window workloads, not isolated
timing. First-window assessment verifies1396pins/ten live handles/81mapped libs.
One bounded2s49Hz profile520samples/0lost showed active fitting/exact predicates
and parameter publication plus waits; inclusive Allreduce76.27%overlaps other
shares. Not phase totals. Profile raw data stays LOCALONLY; diagnostic overhead
is not subtracted. Latest treeRSS~8.48GiB, ~24GiB host available; sampled sum peak
11.685GiB is not physical memory or an unsampled whole-job peak. Resource and
owned-handle receipts retained in179run. Later host-load observation rose to12.21
on8logical CPUs. One124.471s interval delivered0.752–0.763CPUcore/rank with no
new major faults. CPU service includes MPI spinning; it is not useful-work CPU.
At that earlier CPU observation there was no RAM pressure/rejection; first-event
natural time-budget rejection is now preserved. Leave unrelated processes untouched. See
cpu_service_observations.jsonl. Preserve this contention limit in timing assessment.

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

1. Qualify/save the original-grid CFD solution, then run the unchanged-code M6
   baseline with it. Reuse the same seed for the connected-change retry. Keep
   C++ unchanged throughout both CFD/baseline campaigns; check exact owned handles
   before each heavy job. Build179 is already terminal with all pins verified.
2. Preserve complete split generations, quality-entry census and complete quality
   windows. Compare counts/debt and geometry work at matching stages. Batch rounds
   now serve multiple stars, so window number alone is not equivalent work.
   Judge whole-field debt reduction per second; nested geometry/smoothing/MPI
   timings overlap. Latest user explicitly requests a new saved-CFD BEFORE/AFTER
   baseline; do not add unrelated campaigns.
3. On failure, preserve primary outcome and raw six-rank rows before any authorized
   owned-only stop. Latest user authorized stopping the startup-field run;179is
   terminal. For subsequent failures follow the authorized stop/fix/retry workflow.
   Verify ALL owned handles terminal and all pins
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

Blocked repair invalidation gap CONFIRMED by read-only source review: a healthy
cell in a rejected feature's complete imported star can change while its bad-user
cell stays unchanged; Touch skips feature vertices and ignores healthy-cell repair
users, so the feature may remain blocked. No false final acceptance follows, but
search coverage is incomplete. Runtime M6 contribution not quantified. Preserve the unchanged-code saved-CFD baseline, then fix and regress this
full-support invalidation contract before qualifying a next source. Details in backlog; no C++ edits/tests while live.

Numerical frozen-field risk OPEN: current-nodal transport changes the spatial P1
representation after topology edits; identity alone does not prove an immutable
numerical target. Quantify/resolve before claiming that semantics. Original CFD
conservative donors are separate. ITER1 was functional startup only. Latest user now requires a reasonably
converged saved original-grid solution reused for first remeshing. Resumed adaptation can collide with
numbered restart paths; rejected events consume cycles. Isolated disk restart
does not claim general adaptation-resume numbering fixed.

Overbroad serial_full170 was NOT passing; out-of-contract source-envelope/count/
timer assertions remain backlog. Qualified filters are not the whole repository
suite. Removed172convex fallback and173fragmentation hypothesis remain historical.
174prepared-subtree/overlap-lift and175bounded-guidance construction improvements
are preserved in179.

## Design and operating links

Read [AGENTS](AGENTS.md), [complete integration task](Handbook/ADAPTATION_INTEGRATED_NEXT_TASK.md),
[parallel repair and saved CFD input plan](Handbook/ADAPTATION_PARALLEL_REPAIR_20261031.md),
[build179 quality-service design/evidence](Handbook/ADAPTATION_QUALITY_SERVICE_20261031.md),
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
ownership_survey completed read-only surveys; quality_queue_survey finished
run_m6_seed.py and a bounded read-only counter/API survey. Current source-review
decisions (batch clearance and genuine current-field sampling) are in the parallel
repair plan; no C++ implementation before saved-field baseline. Both DONE, no independent heavy jobs/edits. Primary owns
integrated acceptance. No permission pending.
