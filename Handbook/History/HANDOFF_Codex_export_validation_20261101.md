# Historical export-validation handoff — not current live state

Preserved before build202 selected M6 launch update. Consult root HANDOFF_Codex.md
for current handles; historical RUNNING notes below are not current.

# Native adaptation — current handoff

Updated2026-11-01(Europe/Rome). Goal ACTIVE: TWO strictly accepted ONERA M6 Euler CFD → mesh →
CFD → mesh events at50k metric complexity; actual conservative admissible transfer,
finite continued CFD and isolated usable adapted disk restart. Then dedicated BL/
constraint/MPI qualification and corresponding RANS500k workflow. Actual point/cell
counts are separate from target complexity. Geometry1e-4,q>=.2,J>=.05,maxedge1.8 and
requested heights stay strict. Construction preserves exact positive/nondegenerate/
topology/body/feature/height validity and deliberately omits q/J. CAD optional;
no-CAD geometry authority sufficient. ZERO accepted adapted M6 meshes so far.

## Current — consolidation checkpoint; build199 terminal; diagnostic work is WIP

R=`integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57`.
LAST=`R/m6_runs/euler_build199_real_cfd50k_triangle_coverage`.
The inherited directory label describes neither the source nor a new baseline:
this is WINDING build199, source86d292e517; CFD SHA
 ea0067da5f39f2cf09b75bbc737bfda14d57fd4decc15b1f5d1454000a6408da.
Testdriver SHA ba37efed110714d5b89420991015a01440da4ae454a7bd2398ddde4f83b7ada4.
These identify the last qualified build199 source/binaries. The worktree now has
uncompiled diagnostic C++ edits and an offline prototype; do not claim the current
source was validated by the old binaries. Build200 compiled and its focused producer check passed. Build201 compiled. Its serial campaign found a stale transport-nesting test assertion; build202 is compiling that test correction with sources frozen. No new M6 run has started.
Build PASS; focused3306assertions/10cases; serial23048/72; one MPI6 milestone
216cases EACH rank. Full qualification, construction comparisons and earlier live
receipts are preserved in the [build199 archive](Handbook/History/HANDOFF_Codex_build199_20261101.md).

Full M6 exec47583 TERMINAL1. All6 ranks naturally time-budget rejected at
native maximum5403.368014s. COMPLETE EXIT:802117cells/4368q-bad/2537J-bad/
3098geometry-bad/0size-bad, q+J debt36.326780. All6 cache/census/shape/geometry
flags complete; these are actual terminal counts, not an earlier subset.
52complete censuses,24quality windows plus fully measured final interval.
Quality4131.482941s removed net49713q/50855J/672geometry through43866publications:
12.032725netq/s and12.309140netJ/s. Final interval471publications/247q/190J/8geometry
is included. Zero accepted adapted meshes; no conservative transfer, adapted CFD
or adapted disk restart. Original-grid restart00001 and continued history after
rejection are not adapted evidence.

Natural failure preserved1793531764.146309 before any signal; SHA
 e9600cda9d01b4f7b8f017db1bc3b903d6d77266a2cc58a2631f2a61e848bb5b.
Only verified launcher1607403/start94121591 received SIGTERM1793531826.436573.
All10 exact HOST identities ABSENT/all1401pins unchanged1793531839.785239.
LAST/event1_natural_failure.json,event1_failure_owned_stop.json,terminal_handles.json.
All1607xxx handles are historical: NEVER resignal. No owned heavy job remains.
Source freezing ended only after that terminal proof. The diagnostic cache-view/
rejection-metadata edits started after pre_export_implementation_audit.json verified
all10 identities absent/all1401old pins unchanged1793533690.643077.

Traversal1.471449262B/upper1.674224805B work (87.89%); nested rank seconds:
upper4069.367,traversal3385.624,scope469.009,facet107.581,support164.203.
Exact union memo472431hits/475942misses; misses do not classify nearby positions
versus evicted exact queries. Boundary private gain906.097/3367.753ranksec;
interior4623.378/203.816, with0interior upper queries. These are private merit,
not published bad-cell gains. Terminalq745surface/3623interior; J656/1881.
33fullyblocked bad cells,67428queued features,581blocked features. All6132q-or-J
bad cells have an edge<.9;6045have one<.5. Correlation is not construction cause.
Construction returns0:197residual-geometry continuation is unexercised, not disproved.
All seven counter groups reconcile through COMPLETE EXIT. LAST/terminal_assessment.json,
terminal_counter_review.json,terminal_geometry_service_assessment.json and
geometry_certification_analysis.json retain data and limits. Fixed16metric samples
remain supported but do not prove immutable original continuous-CFD-target fidelity.

Selected states/churn/preparation work match198retry1 throughwindow20. Confirmed
all6-rank decisions migrate=0 afterround22848/22912 differ from baseline migrate=1;
first measured selected-state difference is window21. Later trajectories are not
assumed identical. Terminal comparison uses actual current EXIT at4131.482941qualitys
and actual baseline brackets4096.783336/4224.419803s, without interpolation.
The baseline's actual EXIT counts are UNKNOWN. Counted predicate work is not uniform
CPU cost; nested wall timers and variable host pressure do not isolate code speedup.
Initial construction fields match all18generations; traversal fell52.87689%, but
construction elapsed1133.899305vs730.857434s. Saved12older-query traversal fell48.8293%;
these are not representative late-query or full-workflow speedup proofs.

**Resume from [HowToProceed.md](HowToProceed.md).** The user requested consolidation
and clarified that meshes should be saved only for selected substantial milestones
when inspection is useful, not every trial or routine M6 attempt. Save the actual
complete unapproved mesh as .su2, plus inspection views. Their construction-quality
ideas are hypotheses requiring thorough testing, not authorization to change policy.

Current WIP: Engine3D diagnostic cache view, actual cached q/J retention/migration,
bounded recent rejection metadata and opt-in FailureCapture streaming producer.
A protected driver diagnostic hook plus hidden NativeSelectedFailureWorkflow3D
case selects one failed exit across the actual complete CFD adaptation loop; no
automatic ordinary-run writer, per-trial files or new CFD options. Worker rank IDs
come from the stage view. Raw cells use the actual416-byte RecordStream encoding
(all arithmetic fields, including bool, occupy8bytes); the initial381-byte
prototype assumption was wrong and is corrected in producer/reader/tests.
Offline Python checks:11focused synthetic-format tests PASS, including a truly
empty fourth rank. Build200 PASS, focused actual-producer check327assertions/1case
PASS, sources unchanged; all6serial producer/reader cases PASS (quality/construction
x full/unknown/capped). Complete available source reconstructed into SU2 and VTU;
unknown measurements and truncated source remain explicit. See
R/failure_export200_actual_interop.json; this is NOT an M6 capture or acceptance.
Build200 exec81930 TERMINAL0; ninja1838845/start95044638 ABSENT. Focused exec48137
TERMINAL0; owned1855887 ABSENT. After terminal proof, test coverage adds actual
curved controls, body/features/bindings/requested heights and cached q/J validation
after ownership migration; selected full-driver harness now calls Finalize.
Build201 exec34034 TERMINAL0; ninja1858931/start95110067 ABSENT. Serial exec32081
TERMINAL4, owned1869260 ABSENT. Case failure_capture_serial_20261101T123426_1869178:
14cases/6843assertions,13cases/6839assertions PASS; ONLY4transport nesting checks
failed in NativeDefault3D. Exact cause: Remesher snapshot_source performs one
workers.maximum before entering AdaptMetric, so worker totals legitimately exceed
engine totals by one call. Whole communication totals were correct. The corrected
test checks aggregate nesting and explicitly requires that extra call for a single
ordinary stage; no acceptance/gate/repair code changed. All12actual serial
producer-reader cases PASS, including genuine curved controls, feature/body/binding
metadata and requested heights; R/failure_export201_actual_interop.json pins168
input/output artifacts. Cached q/J checks after ownership migration also passed.
Build202 exec40550 RUNNING, host ninja1873133; source/helper pins1297. Only the
transport test correction recompiles. R/build202_status.json is authoritative.
No MPI qualification or new M6 attempt yet. Root/tmp nearly full; use4TBscratch.


User's next diagnostic: bounded export of remaining bad cells, neighboring cavities
and reference surface, carrying q/J, geometry excess, metric edge lengths and
rejection evidence. Existing 3D stage/rejected-state/trial hooks and bounded
rank-local NativeStageCapture are available but not wired into production CFD.
Build199 retained no working-volume/cavity artifact; its discarded private mesh
cannot be reconstructed from aggregate counters or original-grid restart files.
Active plan: [bounded bad-cavity export](Handbook/ADAPTATION_BAD_CAVITY_EXPORT_20261101.md).
Use explicit opt-in callback installation for a selected milestone. Do not add
automatic capture on every rejection or WRT_ADAP_MESH use. Reuse those hooks,
cached measurements and bounded rank-local snapshot streaming;
extract complete cross-rank cavities offline by global IDs, with explicit coverage/
truncation and exact reference provenance. No new scheduler/configuration, gate
relaxation, candidate change or additional speedup is established. Unproven motion/
envelope alternatives remain in the backlog. After focused validation and one
MPI6 milestone, immediately full two-event M6 with the same saved CFD50k/5400;
inspect exports on rejection or continue accepted transfer/CFD/restart on success.

## Previous198 terminal evidence — never resignal

Full preserved handoff and exact historical handles/receipts:
[build198 terminal archive](Handbook/History/HANDOFF_Codex_build198_terminal_20261101.md).
Latest198retry1 TERMINAL69b502b359:all6natural5403.711369s rejection;all10HOSTidentities
ABSENT/all1401pinsunchanged1793524505.578266 before199edits. ActualEXIT803395cells,
all6cache/censusflags0:terminalwholefieldq/J/geometry/size UNKNOWN. LastCOMPLETE
window803251cells/2985q/1636J/3098geometry/0size,quality4537.669830s. Final18.564879s/
291publications haveunknowngatechanges. Net51096q/51756J/672geometry tolastcomplete,
46164publications,11.260405netq/s. All7groups audited.37stuck/4162badunion,47453queue.
Constructionreturns0:197continuationunexercised,notdisproved. Noacceptedmesh.

Parent198firstattempt COMPLETEEXIT802821cells/3529q/1903J/3107geometry/0size;
69b/parentterminalcheckpointa604d1c54f. Same-source retrymatchedparentselectedstates/
workthroughwindow21,extra migrationafter22912,firstsnapshotdifference atwindow22.
No causal timing/migrationbenefit claim. Retryterminal_comparison.json showsactual
parentbracketingtimes,notinterpolation. Geometrycostskew/unprovenownershipalternative
andnegative motionbound evidence remain in[certification analysis](Handbook/ADAPTATION_GEOMETRY_CERTIFICATION_20261101.md)
andbacklog. No furtherunchanged-source retry selected. All144xxxx/129xxxx historical
handles mustNEVERberesignaled. All199handles are now historical too; current terminal proof is above.

## Historical evidence — all terminal,never resignal

Full build191–197 handoff detail is preserved in the
[historical archive](Handbook/History/HANDOFF_Codex_build191_to197_20261101.md),
including exact handles, source/runtime/input receipts, terminal failures and all
prior counter/geometry analyses. Historical ACTIVE notes never override CURRENT.

| Run | Preserved measured outcome | Limit |
|---|---|---|
|197|Lastcomplete2299q/1062J/3079geometry|TerminalEXITincomplete;returnbranchunexercised|
|196|Firstshape-pass47q/26J/3051geometry|Thenbroadcoarsening;terminalEXITincomplete|
|194|CompleteEXIT4369q/2352J/3111geometry|Noacceptedmesh|
|193|CompleteEXIT8628q/5768J/3151geometry|Noacceptedmesh|
|191|CompleteEXIT16191q/12965J/3509geometry|Noacceptedmesh|

197/196 true terminalwholefield q/J/geometry/size areUNKNOWN; do not substitute
cached subsets or earlier complete states. All natural failures were preserved
BEFORE verified owned-launcher stops; all exact identities/pins independently
verified terminal before subsequent source edits. Full receipts remain in R/m6_runs.

## Next monitoring and acceptance

Build199 is terminal. The user requested consolidation; do not start a new heavy
run during this checkpoint. On resumption, finish and qualify the opt-in bounded failure
export within the same cycle, then return immediately to full two-event M6 from
the saved real CFD seed. Keep all strict gates, construction improvements, cache/
ownership safety and cumulative budgets. The complete Euler acceptance/transfer/
continued-CFD/isolated adapted restart obligation remains; then BL/MPI and RANS500k.

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
