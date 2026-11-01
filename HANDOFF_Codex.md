# Native adaptation — current handoff

Updated2026-11-01(Europe/Rome). Goal ACTIVE: TWO strictly accepted ONERA M6 Euler CFD → mesh →
CFD → mesh events at50k metric complexity; actual conservative admissible transfer,
finite continued CFD and isolated usable adapted disk restart. Then dedicated BL/
constraint/MPI qualification and corresponding RANS500k workflow. Actual point/cell
counts are separate from target complexity. Geometry1e-4,q>=.2,J>=.05,maxedge1.8 and
requested heights stay strict. Construction preserves exact positive/nondegenerate/
topology/body/feature/height validity and deliberately omits q/J. CAD optional;
no-CAD geometry authority sufficient. ZERO accepted adapted M6 meshes so far.

## Current — build202 selected full M6 LIVE; source frozen

R=`integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57`.
LAST=`R/m6_runs/euler_build202_real_cfd50k_inspection`.
Full two-event M6 exec88642 RUNNING. This explicitly selected inspection run uses
[NativeSelectedFailureWorkflow3D] in the test-driver, calling the real
CSinglezoneDriver::StartSolver/ADAP_LOOP/Finalize with the same saved real CFD seed,
Euler50k, native5400s and unchanged gates. This is not frozen-metric replay.
Ordinary CFD runs install no capture callback. Current implementation only adds
diagnostics; no construction/repair/ownership policy or config option changed.

HOST identities at1793537141.985488 (all10 verified LIVE;1403pins unchanged):
helper1886348/start95210821;time1886569/start95211629;
timeout1886570/start95211629;launcher1886571/start95211630;
ranks1886586–1886591 all/start95211634. All rank/launcher priorities19, libs1.
LAST/owned_startup.json,command.json,pins.json,status.json,resources.jsonl are
current authority. Do not edit pinned C++/headers/tests/helpers/binary/config/inputs
while this run lives. First restarted CFD iteration and metric computation reached;
no adaptation acceptance yet. Latest source checkpoint is the diagnostic export
checkpoint accompanying this handoff; commit changes do not alter frozen files.
CFD SHA7de0cc448004521becb5609d83a0841836d7744187700244dd36d8538b8ef666.
Test-driver SHAd630234ec127b793237fb7c1dc609cd9db216b96463fc5330560a1fa01acaf04.

Capture policy: only the first rejected exit of this selected substantial attempt,
with actual event and worker rank IDs. Prefix <MESH_OUT_FILENAME>_inspection.
Stream owned/metric/reference/rejection shards,128MiB per file,1MiB admitted scratch;
exclusive creation, explicit truncation/unknown measurements. Existing observers
handle stage-election alignment; no per-trial/private-slot collectives added.
When terminal, run R/inspect_native_failure3d.py on ONLY the selected full-prefix
rank manifests into a new output directory. Save actual complete rejected_mesh.su2
and VTU plus bad cells, cross-rank cavities, reference and historical rejection data.
Raw records are416bytes, all scalars including bool8bytes. Diagnostic mesh is
unapproved, not an adapted restart. Sampling/visualization cannot certify geometry.
No mesh files on ordinary attempts or per trial. Details:
[bounded export](Handbook/ADAPTATION_BAD_CAVITY_EXPORT_20261101.md).

## Connected diagnostic qualification — PASS, not workflow completion

Build202 PASS; serial6845assertions/14cases; one MPI6 milestone22cases EACH rank.
All24actual C++ producer/reader combinations PASS (serial/MPI6, planar/genuinely
curved, quality/construction, full/unknown/capped);636artifact hashes retained.
Four full MPI cases verified complete cross-rank vertex stars and empty rank5.
Cached q/J/edge values match direct known-field measurement; construction q/J stay
unmeasured; migration preserves cached values/census flags; captured and uncaptured
repair cells/work counters match. Curved controls/body/features/bindings/requested
heights, overwrite refusal and scratch/disk cap refusal exercised.11offline Python
synthetic tests PASS. R/failure_export202_qualification.json and
failure_export202_actual_interop.json preserve exact scope/provenance.

Build201 serial found4stale transport test assertions: current-metric snapshot
performs one workers.maximum before entering the engine, so worker total includes
that call. Whole transport aggregate was already correct. Corrected nesting test
explicitly requires the extra call for a single ordinary stage; mesh gates were
unchanged. All build200/201/202 and serial/MPI handles are terminal/absent; no old
handle may be signaled. Source freezing for current M6 began after qualification.
Earlier implementation/test details and terminal199 analysis are preserved in the
[export-validation archive](Handbook/History/HANDOFF_Codex_export_validation_20261101.md).

## Latest terminal difficult case — build199, no accepted mesh

LAST199=`R/m6_runs/euler_build199_real_cfd50k_triangle_coverage` (WINDING199).
All6 naturally rejected at native5403.368014s; COMPLETE EXIT802117cells,
4368q-bad/2537J-bad/3098geometry-bad/0size-bad, q+J debt36.326780.
Quality4131.482941s;43866publications; net49713q/50855J/672geometry removed:
12.032725netq/s and12.309140netJ/s. No conservative transfer/adapted CFD/restart.
Original-grid restart00001 and continued history after rejection are not adapted
proof. No private working mesh was saved in199 and it cannot be recovered from
counters or original-grid restart. Natural failure preceded the verified owned
stop; all old1607xxx identities absent/pins unchanged before subsequent edits.
NEVER resignal old handles. LAST199/event1_natural_failure.json and
terminal_handles.json preserve exact proof.

Traversal87.89%of upper-query work; boundary private gain906.097/3367.753ranksec,
interior4623.378/203.816 with0interior upper queries. Private merit is not published
bad-cell gain. Finalq745surface/3623interior;J656/1881. Only33fullyblocked cells;
all6132q-or-J-bad cells have edge<.9,6045edge<.5. Correlation is not construction
cause. Construction returns0:197geometry continuation remains unexercised.
Shape closure then residual size/geometry closure and shape reclosure remain needed.
Selected199states matched198retry1 throughwindow20; later ownership/trajectories
differ. Baseline198retry1 actual EXIT counts UNKNOWN. Work/timers overlap and host
pressure varied; no isolated runtime speedup claim. Full terminal counters and
matching comparisons remain in LAST199/terminal_assessment.json,
terminal_counter_review.json and geometry_certification_analysis.json.

**Read [HowToProceed.md](HowToProceed.md).** Construction-quality preferences,
geometry motion/envelope bounds and other suggestions are hypotheses, not approved
production policies. Diagnose actual remaining cavities, test a selected connected
hypothesis thoroughly, then assess net whole-field q/J-bad removal per second with
geometry-bad counts. Do not choose from commits/debt percent alone, add competing
implementations, relax gates or raise budgets. Unrelated/unproven ideas belong in
[backlog](Handbook/ADAPTATION_BACKLOG.md).

New attachment reviewed read-only while202was frozen:
[technical assessment review](Handbook/ADAPTATION_TECHNICAL_ASSESSMENT_REVIEW_20261101.md).
Confirmed: geometry-bad can include unresolved conservative upper bounds, not
proved violations; final two-sided certification remains strict. Interior labels
mean no physical face, not necessarily free vertices. Current export is inspection,
not faithful trial replay. Use actual captured witnesses/clean controls before
selecting a change; do not interrupt202or implement attachment proposals now.

Historical191–197 and198 records:
[191–197 archive](Handbook/History/HANDOFF_Codex_build191_to197_20261101.md),
[198 terminal archive](Handbook/History/HANDOFF_Codex_build198_terminal_20261101.md),
[199 archive](Handbook/History/HANDOFF_Codex_build199_20261101.md).
All their handles are historical. Incomplete EXIT flags mean whole-field counts
are unknown, never substitute an earlier subset/window as terminal truth.

## Monitor frozen M6 and preserve the full acceptance obligations

Let current M6 finish unless preserved evidence justifies the authorized owned
stop/fix/retry. At a natural first-event rejection preserve all6rank terminal
censuses, failure messages, artifact sizes/hashes, source/runtime/input pins and
current exact process identities BEFORE stopping only the verified owned launcher.
Check/close the same exec88642; require all exact HOST handles absent before any
pinned source edit. Do not infer termination from a stale log/observation timeout.
If accepted, continue event2/transfer/CFD; passing diagnostics or a saved rejected
mesh do not complete the goal. Record useful work/progress at matching actual
stages; account for contention and avoid another unchanged-source baseline campaign.

Analysis caution: historical matching-window summaries assume the first quality
pass. The retry1 stored command now distinguishes published repair churn from
whole-field construction changes if the197return crosses construction. Adapt analysis assertions to
actual phase/census coverage; an analysis assertion alone is not a solver failure.

Read-only downstreamreadinessreview:
R/m6_runs/euler_build196_real_cfd50k_witness_first/workflow_acceptance_readiness.json.
Existing175transfer_restart_acceptance_source_review.md and audit_native_unsteady.py
were located/reused; P1integralaudit needs IMMEDIATEposttransfer states,notlaterCFD.
Existing workflowreceipts alone are insufficient transfer/state/restartproof.
Actual pinned distributedtransfer unconditionally gates finite trueresidual<=1e-10,
fulladmissibility and1e-12scaledtargetintegrals before publication; inspect ACTUAL
per-eventprojectiontable/source provenance. Donor-vs-newintegralchange is a different
quantity (CLOSEDsliverrule/open-boundary domaincorrection). Existing validate_state
can be reused on an isolatedcopy of ADAPTEDmesh namedm6.su2 and actualmatchingflow;
functionuses parsedpointcount,not61330hardcode. Thenactual RESTART_SOLYES/ADAP_LOOPNO
probe with distinctinput/outputprefixes onlyafterownedrunterminal. No newhelper/
source/configchange now; no adaptedstate/restartclaim.

Existing closure plan: shape/height → residual size → geometry-only repair via
EnableGeometryRepairs/strict complete-patch guidance,or valid construction return,
then shape remeasurement/reclosure.Neither191nor193reached geometry-only entry. Same
cumulative5400s/work/memory limits. Require TWOstrict accepted mesh/native_ref pairs,
actual conservative admissible transfer(double residual1e-10,targetintegral1e-12),
finite continued CFD,rho/internalenergy>0. Then isolated COPYfinal adapted mesh/
native_ref+matchingflow,actual RESTART_SOLYES/ADAP_LOOPNO/correct SOLUTION_FILENAME/
distinct outputs,coordinate/order/state audit. Original seed00000 is NOT adapted
restart evidence. Then dedicated BL curved/junction/interface/subsequent-operation/
redistribution/requested-height MPI qualification and RANS500k. Goal stays ACTIVE.

Readonly seed R/m6_inputs/euler_original_force_seed_v1:61330points/303359cells,
Mach.8395,AOA3.06,CLmean.2695353537415/CDmean.0135358565364,last200ranges1.114e-7/
3.121e-8,finite/rho/IEpositive/exact coordinate/order. Userforce-policy qualified,
not a residual-converged aero reference. SeedSHA1eaee788bd62bd92a92ec539577dcbcfc0f8042dcbcec23adb04e825bb01a880.
Reuse per first remesh; distinct SOLUTION_FILENAMEbaseline_euler_seed/RESTART_FILENAMErestart_flow.

Open limits: historical191transported nodal tensors drifted~8.3%at16fixed samples;
193/196/197samples are recorded with their actual windows in the archived evidence.
Neither runtime tags nor these samples prove immutable original continuous-CFD-target
fidelity. General
adaptation resume cycle0/path collision and rejected-cycle consumption remain open;
isolated solver restart does not prove general adaptation resume fixed. Broad182MPI
timed out; serial_full170 NOT passing. BL/RANS qualification not started.

## Operating links and discipline

[AGENTS](AGENTS.md) · [complete task](Handbook/ADAPTATION_INTEGRATED_NEXT_TASK.md) ·
[winding screening](Handbook/ADAPTATION_GEOMETRY_WINDING_20261101.md) ·
[certification analysis](Handbook/ADAPTATION_GEOMETRY_CERTIFICATION_20261101.md) ·
[envelope](Handbook/ADAPTATION_GEOMETRY_ENVELOPE_20261031.md) ·
[memo](Handbook/ADAPTATION_NUMERIC_MEMO_20261031.md) ·
[ownership](Handbook/ADAPTATION_CENSUS_OWNERSHIP_20261031.md) ·
[geometry/smoothing](Handbook/ADAPTATION_GEOMETRY_QUERY_REUSE_20261031.md) ·
[feature](Handbook/ADAPTATION_FEATURE_PUBLICATION_20261031.md) ·
[saved CFD](Handbook/ADAPTATION_M6_CFD_SEED_20261031.md) ·
[cache/ownership](Handbook/ADAPTATION_CACHE_PHASE_20261030.md) ·
[backlog](Handbook/ADAPTATION_BACKLOG.md).

One owned heavy job,nice19,libs1,build2–4workers(helper3),MPI6bounded; TMPDIR4TB
compile-tmp,not nearly full root/tmp. Check host RAM/load/disk; sampled summedRSS
is not physical memory/job peak. Preserve resource/pin/handle receipts,leave other
jobs alone. Fork pushes authorized:originhttps://github.com/rois1995/SU2.git branch
codex/native-3d-bl. Git metadata outside writable root:escalated git -c gc.auto=0
-c submodule.recurse=false; diff/status --ignore-submodules=all. Whitelist adds; no
binaries/meshes/restarts/rawperf. Leave unrelated155logs/.orig/submodules/untracked
steering/history files untouched. Primary owns integrated design/source/acceptance;
bounded read-only routine agents done, no independent jobs.
