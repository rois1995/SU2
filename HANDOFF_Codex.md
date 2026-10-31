# Native adaptation — current handoff

Updated 2026-10-31. Full goal ACTIVE: strictly accepted ONERA M6 Euler
CFD → mesh → CFD → mesh at 50k metric complexity, conservative admissible actual
transfer and usable adapted disk restart; then dedicated BL/constraint/MPI
qualification and the corresponding RANS500k workflow. Report actual points/cells
separately from complexity. Strict geometry1e-4, q>=.2, J>=.05, maxedge1.8 and
requested heights. Construction keeps exact positive/nondegenerate/topology/body/
feature/height validity and intentionally omits q/J. CAD remains optional.

## Authoritative current live state

**Real-CFD M6 BEFORE baseline TERMINAL; connected implementation IN PROGRESS.**
`staged57/m6_runs/euler_build179_real_cfd50k_before`,exec88683exit1.
First event naturally time-budget rejected on all6ranks,max5409.754671s native.
Preserved six raw summaries/refine details and rejection in event1_natural_failure.json,
then stopped exact verified launcher3063393 under standing failure authorization
after the driver began the next event's setup; no second mesh was accepted.
ALL10ownedhandles ABSENT;1401source/build/runtime/config/input/helper SHA256 pins
independently unchanged before ANY C++ edit:terminal_handles.json. Active build181 is listed below.
No accepted mesh/transfer/adapted restart. terminal_assessment.json identifies next
target SPEED: per-slot synchronization, widening required-debt spread, and cache
preparation. Rank4low netq-bad yield also requires requested repair-power counters;
baseline lacks the new counters, so do not retrospectively invent them.

ONE connected milestone IMPLEMENTED; build180 TERMINAL PASS, exec9648exit0,
2046.261s/all1296pins unchanged/up-to-date. Host Python3521908/Ninja3521921
independently ABSENT before edit; build180_terminal_verification.json. It has not
been qualified. Pre-validation source review confirmed variable coverage pieces/
limbs are not bounded by static16KiB/cell; exact cumulative coverage is now
admitted locally before retention and between approved parameter exports. This
is a required memory-contract correction within the SAME connected cycle.
Build181 TERMINAL PASS336.807s/all1296pins unchanged, exec43195exit0;
exact host Python3620025/Ninja3620039 ABSENT before checks. Serial first sandbox
attempt could not initialize MPI sockets (no tests); escalated serial exec35860
terminal:117cases,114pass/3new test-contract failures, all pins unchanged and
no owned processes before edits. parallel_quality181_serial_assessment.json.
Production remainsfbba658e16; tests now use explicit midpoint endpoints, distinguish
valid MemAvailable from fallback, and test q/J diagnosis after both sides measured.
Build182 TERMINAL PASS171.503s/all1296pins unchanged, exec21307exit0;
host Python3643026/Ninja3643033 independently ABSENT before serial checks.
Sourceeea054f251 pushed (production unchanged sincefbba658e16). Serial182PASS:
117cases/17611assertions, exec61300exit0/allpinsunchanged, owned Python3652953/
time3653190 independently ABSENT. cases/parallel_quality182_serial_20261031T120846_3652953.
Broad MPI6 exec59359TERMINAL timeout609.696s/noassertion failure; all10owned
handles independently ABSENT/allpinsunchanged. It is NOT a passing qualification.
Affected-contract continuation exec6606TERMINAL206.339s:161cases/rank,
160pass/1failure perrank, exact10handles independently ABSENT/allpinsunchanged.
Failure only existing NativeCollapseFilter3D single-cell/6rank optional phase
partition. Guard now skips impossible migration when global_cells<world.size,
retaining coherent ownership; global count reused in existing benefit estimate.
No construction operator/gate changes. Source current fix needs build183.
parallel_quality182_assessment.json / parallel_quality183_validation_plan.json.
run_cases.py now records --durations yes for resumable completed case evidence;
helper change occurred only after broad182terminal; no production decisions changed.
Same planned focused serial/ONE MPI6 milestone, then full TWO-event
M6 AFTER. All original constructors/operators/gates remain unchanged.
No new qualified binary yet; zero accepted meshes. Separate zero-clean quality
partition constraint, debt-directed local ceilings, MemAvailable correction with
same4GiB cap, and exact changed-support invalidation are connected. Private repair
runs all local admitted slots without per-slot collectives (including batch=1),
reserves/refunds optimizer work once, then checks every approved candidate with
ClearanceBatch, final full-vertex reservation, one routing/parameter preparation/
Publish and full changed-row refresh. Local result/scratch/transport memory and
sticky time/work/geometry failures remain admitted/elected; construction operators
and q/J-free construction semantics are preserved.

Passive counters are written: private busy/join-call wall and commits; q/J churn
matching exact coordinate tetrahedra across ID-only rewrites; ladder trials,
certified/published yield, time, debt/bad-cell removal/rejections; blocked/star
queues; surface/interior/genuine-feature/miss bins; short-edge shares; geometry
anchor-authority/excess bins. At most16fixed quality-entry spatial samples follow
actual removed/added rows and ownership resets, sampling current nodal tensors
only with exact containment. Lost means unavailable tracked support, not proof
outside mesh. Drift is full target for current Euler (no composition); composed
fields explicitly report sensor-only/full_target=0, without extra geometry queries.
No perf/GDB campaign. Serial117cases passed; MPI6 qualification remains pending. Focused fixture now checks
multiple commits/rank, one final round reservation, zero private tracked
collectives, q/J churn conservation and cached-vs-recomputed field counts.

Next: build/verify183, focused serial plus affected MPI6 continuation
(parallel_quality183_validation_plan.json), then immediately full TWO-event AFTER
from the exact saved CFD seed. Do not edit C++/helpers/tests while a build/check
handle is live. Both agents DONE/read-only reviews; no independent heavy jobs.

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
Different input fields prohibit isolated old-v-new timing attribution. Seventeen split generations COMPLETE:726930cells,size-bad0/geometry-bad4197 at
max3105.412engine seconds. Full quality entry COMPLETE at3225.021:53318q-bad/
52464J-bad/4197geometry-bad/size0;q/Jdebt24844.47974341604. All726930measured,
all four complete/cache flags1. Quality ownership moved411173cells in33.543s;
before/after integer counts and census flags preserved, reported debt sum roundoff
<1e-8. Post-migration q/Jdebts/rank3756.805/3988.121/5770.169/1909.352/5746.601/
3673.432(max/min3.022). Confirms need for separate zero-clean debt constraint.
Receipts:split_generation_assessment.json,quality_entry_assessment.json,
quality_entry_ownership_assessment.json. Last COMPLETE quality census was window2at
1479.020seconds sincequality entry:740798cells,43764q-bad/42274J-bad/
4067geometry-bad/size0,q/Jdebt13087.09702308638. Cumulative4098shape-progress
commits(2.77075/s),netq-bad removal6.45968/s. Window2alone2201commits/
4719netq-bad removed in780.293s. Debts now609.820–4561.792/rank(max/min7.481).
Rank3upperqueries208.339s inwindow2for380netq-bad removed;rank4upper10.713s
for155removed,remainingq-bad11307/debt4561.792. Shows unequal remaining burden
and unequal useful yield; final target assessment still pending. Nested timers
must not be summed; election-call wall325.6–452.8s/rank includes protocol/wait.
Receipts:quality_trend_assessment.json,quality_window1_assessment.json,
quality_window2_assessment.json. No new profiler. All quantities are real-CFD
baseline only, not matched startup179timing. Plan uses separate entry debt
constraint plus existing local batch ceilings scaled to cached required demand,
retaining oldest slot and unchanged per-star trial limits; no mid-quality queue-age
reset/new scheduler/config. No accepted mesh/transfer/adapted restart.

See [saved CFD input and preparation evidence](Handbook/ADAPTATION_M6_CFD_SEED_20261031.md)
for exact hashes, current policy, source snapshots, former criterion failure,
actual checkpoint/state audits and reuse protocol. No repeated freestream solve.
No accepted mesh/actual transfer/adapted restart yet. Baseline is now terminal;
finish the ONE connected quality change, then validate and run AFTER from the
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
saved seed, and base subsequent results on this new baseline. The unchanged-code full M6 BEFORE baseline from the saved
force-qualified real CFD input is now terminal; its failure and pins are preserved.
Source edits began only after independently verifying all owned handles absent. One owned heavy job, nice19, libs1, MPI6; native
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

1. Saved real-CFD seed is force-qualified under latest user instruction. The unchanged-code BEFORE baseline is terminal with preserved failure. Reuse
   this exact seed for AFTER. Complete the connected implementation before any
   build/test campaign; check owned handles and freeze all inputs for each job.
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
search coverage is incomplete. Runtime M6 contribution not quantified. Baseline is terminal. The exact-support fix is written but unvalidated; regress
this contract before qualifying the next source. Details in the connected plan.

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
repair plan; baseline terminal prerequisites satisfied. Agents implemented only assigned
partition/cache files after terminal audit; both DONE, no independent heavy jobs.
Primary owns Engine/distributed integration and source review; build181 next.
Both agents also completed bounded local-metric API/test work and read-only review;
no independent heavy jobs. Full private round/publication and passive counters are written; validation next. Primary owns
integrated acceptance. No permission pending.
