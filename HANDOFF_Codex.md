# Native adaptation — current handoff

Updated 2026-10-31. Full goal ACTIVE: strictly accepted ONERA M6 Euler
CFD → mesh → CFD → mesh at 50k metric complexity, conservative admissible actual
transfer and usable adapted disk restart; then dedicated BL/constraint/MPI
qualification and the corresponding RANS500k workflow. Report actual points/cells
separately from complexity. Strict geometry1e-4, q>=.2, J>=.05, maxedge1.8 and
requested heights. Construction keeps exact positive/nondegenerate/topology/body/
feature/height validity and intentionally omits q/J. CAD remains optional.

## Authoritative current live state

**Full two-event M6 Euler50k BEFORE baseline RUNNING**, exec88683:
`staged57/m6_runs/euler_build179_real_cfd50k_before`.
Python3063005,time3063391,timeout3063392,mpirun3063393;
ranks3063424–3063428,3063432. Exact ten identities/startticks in
startup_verification.json. All ten LIVE,1401pins unchanged,all81actually mapped
libraries covered. C++/binary/helpers/config/inputs FROZEN to baseline outcome.
One nice19/MPI6/libs1 heavy job; unchanged qualified179CFD/source,50kcomplexity,
native5400s/event,outer14400s,TWO actual accepted events required. No C++ changes
before this saved-CFD BEFORE baseline has terminal evidence.

Latest user explicitly accepts reasonable stable CL/CD and directs stopping CFD
and proceeding with adaptation; do NOT reimpose the assistant's old residual
threshold. CFDv3is TERMINAL after verified owned stop; all ten ABSENT/1405pins
unchanged BEFORE helper edit. Dedicated read-only saved seed:
`staged57/m6_inputs/euler_original_force_seed_v1`.
CL.26953535374/CD.01353585654,ranges1.114e-7/3.121e-8 over200iterations;
61330points/303359cells,exact restart coordinate/order match,all finite,rho/IE>0.
Selected v3local1126/global3004; restart SHA
1eaee788bd62bd92a92ec539577dcbcfc0f8042dcbcec23adb04e825bb01a880.
Qualified under user force policy,NOT residual-converged aerodynamic reference.
Every first remesh reuses this saved field via independent copy+same ITER1
continuation. Initial baseline cycle0actually prints restart solution and
CL.269535/CD.013536. Initial ownership partition complete:303359cells preserved,
rank counts49602/42336/56390/49597/48833/56601. Receipt:
initial_partition_assessment.json. Geometry-cache host quota295417856bytes,
rank quota49236309bytes; compare actual quotas when judging AFTER timing.
First coarsening COMPLETE(all6caches/censuses/geometry flags consistent,q/J
intentionally unmeasured):292709cells,55180size-bad/3512geometry-bad,
2627collapses,1142.910engine seconds. coarsen_assessment.json uses per-rank stdout;
long rows interleave in combinedrun.log. Source cache estimate uses AVPHYS/free
pages,missing reclaimable cache despite~28GiB MemAvailable; prepared pool~20MiB
vs historical396MiB, repeated facet builds measured. Preserve this baseline;
include conservative MemAvailable correction (same4GiB host hardcap/shares/admission)
in the same connected parallel/debt milestone. cache_availability_assessment.json.
Different input fields prohibit isolated old-v-new timing attribution. Four split
generations COMPLETE:693014cells,10728size-bad/4411geometry-bad at max1810.707
engine seconds. q/J deliberately unmeasured. split_generation_assessment.json
contains exact complete six-rank rows. No accepted mesh yet; splitting continues. Strict geometry/q/J/size/heights/transfer gates unchanged.

See [saved CFD input and preparation evidence](Handbook/ADAPTATION_M6_CFD_SEED_20261031.md)
for exact hashes, current policy, source snapshots, former criterion failure,
actual checkpoint/state audits and reuse protocol. No repeated freestream solve.
No accepted mesh/actual transfer/adapted restart yet. After this baseline terminal,
implement the ONE connected quality change then validate and run AFTER from the
same immutable seed. The complete Euler→BL/MPI→RANS500k goal remains unchanged.

**Build179 TERMINAL after latest explicit user-authorized stop because it uses
ITER1 startup CFD, not a reasonably converged solution**, exec16090 exit1.
Run: `staged57/m6_runs/euler_build179_quality_service50k`.
All ten exact owned identities independently verified ABSENT on host /proc;
all1396source/build/runtime/input/helper pins unchanged. Receipts:
terminal_handles.json,user_authorized_stop.json,status.json. First event naturally
failed native time budget before the user-authorized stop during the next event.
No accepted mesh/transfer/adapted restart. Production CFD179 remains unchanged.

Latest user supersedes prior no-stop/no-new-baseline restrictions: stop the
startup-field run; obtain and save a real original-grid Euler solution
(accepted by reasonable stable CL/CD under latest clarification); then compare BEFORE and AFTER implementation using that exact same
saved seed, and base subsequent results on this new baseline. Current heavy job is the unchanged-code full M6 BEFORE baseline from the saved
force-qualified real CFD input. Do not implement the repair/ownership change before preserving that
baseline's terminal evidence. One owned heavy job, nice19, libs1, MPI6; native
budget5400/event remains unchanged. See linked connected plan for
latest user force-qualification policy, baseline order and complete goal.

Qualified checkpoint6ea65d9141; productionc132020654, test contractsa0e5e3bec7.
CFD SHAe8f81c5a941f6d03fea81c6437e9f4f6d4305044e55e6de4d6a2113bd0116072.
Config SHA5b6be56224fa8964532f92085659d63e2586ae1936d585fc30939c1ed69225a1,
authority18/SHA2e99fb7870e2c718f110da8509e4c0f7e746ce138e7f27ab57aa0dedbb6c42de;
both byte-identical175. Initial61330points/303359cells; initial restart SHA
ee7ae4690ee4c890104ac1876e4eed85354e6014233e10733862dbc6c2f1d45f is identical175.
All six initial partition cell/metric/geometry workload fields match175.
`initial_field_partition_assessment.json` is initial-field provenance, NOT adapted
transfer/restart evidence.

## Preserved build179 evidence

Productionc132020654/testsa0e5e3bec7/qualified6ea65d9141. Same CFD binary is used
for the current BEFORE baseline. Build179PASS111.227s/1296pins; serial29cases/
8169assertions; MPI6PASS212cases/rank, all owned handles terminal at qualification.
Receipts:staged57/quality_service179_integration_summary.json.

Historical startup-field179 first event naturally time-budget failed at5403.717s;
last COMPLETE census7:752202cells,34386q-bad/33995J-bad/2854geometry-bad/size0,
70.3022%q/Jdebt fall in3960.659quality seconds,13883commits. Final partial zero
counts mean UNKNOWN, not closure. Next target from that native outcome was speed:
per-slot synchronization and concentrated debt; new real-CFD baseline may change
that assessment. Detailed construction/windows/cache/profile history is preserved
in the linked quality-service handbook and179run receipts. No further perf/GDB.
Do not compare the old startup-field timing as same-input evidence against the
new saved-field baseline. No accepted mesh/transfer/adapted restart was delivered.

## Next actions and acceptance

1. Saved real-CFD seed is force-qualified under latest user instruction. Complete
   the unchanged-code M6 BEFORE baseline. Reuse this exact seed for AFTER. Keep
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
real saved original-grid solution reused for first remeshing; latest user accepts
reasonable stable CL/CD without further residual convergence. Resumed adaptation can collide with
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
