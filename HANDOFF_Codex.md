# Native adaptation — current handoff

Updated2026-11-01(Europe/Rome). Goal ACTIVE: TWO strictly accepted ONERA M6 Euler CFD → mesh →
CFD → mesh events at50k metric complexity; actual conservative admissible transfer,
finite continued CFD and isolated usable adapted disk restart. Then dedicated BL/
constraint/MPI qualification and corresponding RANS500k workflow. Actual point/cell
counts are separate from target complexity. Geometry1e-4,q>=.2,J>=.05,maxedge1.8 and
requested heights stay strict. Construction preserves exact positive/nondegenerate/
topology/body/feature/height validity and deliberately omits q/J. CAD optional;
no-CAD geometry authority sufficient. ZERO accepted adapted M6 meshes so far.

## Current —198 same-source acceptance retry LIVE and FROZEN

R=`integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57`.
ACTIVE=`R/m6_runs/euler_build198_real_cfd50k_retry1`.
Full TWO-event M6 same-qualified-source acceptance retry exec64497, launched after
terminal evidence/analysis checkpoint a604d1c54f was pushed. Source7aa53848ea,
CFDSHA96bcaff470bb4571966a2db93e0e7ed0b0920e46a9b3483a0898cc7f0b19a1f1.
No source/header/test/helper/binary/config/input changes since198qualification.
All1401parent pins verified unchanged, all prior10owned identities absent and no
other owned heavy workload before launch. Reuse qualified198proof/serial/MPI6.
Same saved realCFD seed, Euler50k/native5400/outer14400/twoevents/strictgates.
This is acceptance continuation in the SAME cycle, not an old-code baseline.

Actual HOST startup1793518985.7476037: helper1441945/start93398497;
time1442099+timeout1442100/start93399070;launcher1442101/start93399070;
ranks1442105/1442106/1442107/1442108/1442109 start93399077,
rank1442110 start93399078. All10LIVE, all1401pins unchanged,69initialmappedELF
covered initially; initial_native_live_provenance.json1793519039.0905287 then
verified all82mappedELF covered/all1401pins unchanged/all10LIVE. No inferred termination
from sandbox/proc absence. Verify these exact identities before any stop/restart.
ACTIVE/owned_startup.json. Heavy processes nice19, all six ranks libs1.
ACTIVE/runtime_environment.json confirms actual rank TMPDIR and
OMPI_MCA_orte_tmpdir_base both point to owned4TBcompile-tmp/m6_build198_retry1;
actual OpenMPI session is there. Default shared-memory transport unchanged.
Root/tmp has15MBfree; no unrelatedfiles/jobs modified. Preflight actual host
load2.19/2.19/4.21,40.50GBMemAvailable,CPUpsiavg60=1.22%,4TB61.46GBfree.
R/geometry_coverage198_retry1_preflight.json. Lower contention is observed;
causality for previous timing is not established.

Actual config/mesh/native_ref/savedseed BYTEIDENTICAL parent198 atstartup.
Actual original continued00000 is now emitted and BYTEIDENTICAL parent198
(SHA6bbed8da75c40eca7364cc9d3fe96f13311fad69706e013acb68ba8feaf4240a).
ACTIVE/early_runtime_provenance.json. Original00000 is NOTadaptedrestartproof.
Initialpartition ALLselectedstablefields EXACTparent198,303359cells;
ACTIVE/initial_partition_comparison.json. Resources1793519181.948772:
MemAvailable35726056kB,sampledRSS4.013GB,CPUpsiavg60=7.11%,memorypsiavg10=0.
ACTIVE/initial_partition_resources.json. This is not a shape/acceptance result.
ZEROaccepted adapted meshes/transfer/adaptedCFD/adaptedrestart. AllCpp/header/tests/
helpers/binary/config/inputs FROZEN until actualterminal or preserved authorized
failure. No competing implementation/profiler/new config/raisedlimits/gaterelaxation.
Monitor actual complete censuses/netwholefieldq/Jbad/s,geometry counts,work and
resources. Retain197geometry continuation,as yet unexercised onM6. GoalACTIVE.

Retry1 firstcomplete coarsening:292709cells/2627collapses,ALL17selected per-rank
fields EXACTparent198; traversal158224112work SAME,allpreparation/discoverywork
same. Engine268.138009267s versusparent266.985899094s.
No material timing improvement at this stage; q/Jintentionally UNMEASURED.
Splitting now running. ACTIVE/coarsening_comparison.json,coarsening_resources.json,
coarsening_live_provenance.json. All10actualHOSTidentitiesLIVE/all1401pins unchanged/
82mappedELFcovered1793519422.5346503. This is not acceptedmesh/qualityclosure.


## Parent198 first attempt — TERMINAL analysis


R=`integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57`.
CURRENT=`R/m6_runs/euler_build198_real_cfd50k_triangle_coverage`.
PREVIOUS=`R/m6_runs/euler_build197_real_cfd50k_geometry_return` (TERMINAL).
Sourcecheckpoint7aa53848ea;CFDSHA96bcaff470bb4571966a2db93e0e7ed0b0920e46a9b3483a0898cc7f0b19a1f1.
TestdriverSHA9fa9bd82d0e3ec89b2930fa20a6ee6471b9b70ff2b3d8255efd9b9d6fb817642.
Exec72458TERMINAL1. ALL6naturaltime-budget rejection,native5403.732045s,
refine5338.754460s. Complete measured EXIT802821cells,3529q/1903J/3107geometry/
0sizebad,qJdebt29.968095035807. Allcellsmeasured/cache/census/shape/geometryflags1.
This isACTUALterminalwholefield,notanearlierwindow orcachedsubset. Strictgatesfailed.
ZEROacceptedadaptedmeshes;transfer/continuedadaptedCFD/adapteddiskrestart unproved.

Natural failure preserved BEFORE signaling ONLY exact remaining ownedlauncher
1293494/start92754544. All10identities/all1401pins verified beforesignal.
ALL10ABSENT/all1401pinsunchanged1793518046.7156804:CURRENT/terminal_handles.json.
All1293335/1293492/1293493/1293494/1293498/1293499/1293500/1293501/1293502/1293504
handles HISTORICAL;NEVERresignal. At parent terminal noownedheavyjob remained. NoCpp/header/test/helper/
binary/config/input edit since198qualifiedsource. Preserveallterminalreceipts:
event1_natural_failure.json,event1_failure_owned_stop.json,status.json,
terminal_assessment.json,terminal_counter_review.json,terminal_geometry_service_assessment.json,
geometry_certification_analysis.json,terminal_resources.json.

ALL56completequalitycensuses/26windows+finalEXIT andALL7diagnosticgroups audited.
Quality4548.497238s,45208publications;net50552q/51489J/663geometryremoved,
11.114000netq/s,11.320002netJ/s,1.1182netq/publication. Final14.760462s/128pub
net95q/95J/3geometry are fullyincluded,withcompleteEXITladder/churnrows.
Queue52844,blockedfeatures574,stuck31,missing0,privatecollectives0.
Remainingq692surface/2837interior,J600surface/1303interior. Interior meansno
physicalface,notnoconstrainedvertices. All4811bad-unioncellsshort<.9;4752short<.5:
correlation,notconstructioncause. Geometryauthoritybinsareoverlappingcellflags.
26windowrepairchurn plusEXIT:396480qretired/345928introduced,
348326Jretired/296837introduced;coordinatesarepartofchurnkeys,notonlytopology.
Ladderpublications/wholefieldnet/debt reconcile.13latermigrationspreservecounts/
debt/completeflags/20selectedmemo+preparedcounters/driftavailability;notproofeverykey/history.
Constructionreturns0:197residualgeometrycontinuation NOTexercised,notdisproved.

Primarynexttarget is usefulclosureSPEED withdecliningyield/churn.31blockedbadcells
outof4811and52844queuedfeaturesdonotshowexhaustedglobalrepairpower. No construction
cause isestablishedbyshortedges. Keep197boundaryrepair-power/geometrycontinuation;
no competingcache/scheduler/envelope/newconfig/gate/budget change selected.
Rawslotceiling79584,candidates71415,reserved60232,published45208:
89.7%candidatefill,84.3%candidate-reservation,75.1%reserved-publication.
56.8%ofrawceilingpublished;rawslotsarenotguaranteedeligibleindependentusefulwork,
unreservedcandidatesdonotisolatereservationconflicts. Summedrankbusyshare.28952,
joincallshare.41319;joinincludesprotocol+wait,timersoverlap,CPUincludesMPIspin.

Terminalgeometry:970363uppercalls,3.450813654Bupperwork,
3.244891247Btraversal;nestedsummedranksecondsupper4562.948/traversal3799.071/
scope527.868/facet112.395/support183.940. Donotaddoverlappingtimers.
Unionmemo482312hits/488051misses;missesdon'tseparatenearbypositions/support/tolerance
fromevictedidenticalqueries. Privateboundary608200candidates,532056shaperejects,
24862moves/privategain904.885/3721.463ranksec;interior27576moves/privategain4626.509/
216.631ranksec. Privategainsarenotpublishedq/Jgains. Existingunchangedface/context/
prepareddata/numericmemo/screening reviewed;0of12whole-querymotion-slackproofs and
optimisticsubtree.5765%beforeoverheads stilldonotjustifynewcache/envelope.

198savedquerywork-23.5546%,all12boundsEXACT196;actualconstructionALL18gensselected
countsEXACT197with28.6488%lesstraversal. First7qualitywindowsselectedcounts/pubmatch;
firstmeasureddivergenceat8. Laterstates/trialsNOTassumedidentical.
Full-runlowercountedwork hasNOTestablishedfaster netclosure:19811.114netq/s vs197
11.1366throughitsLATESTCOMPLETEwindow4649.704s(2299q/1062J/3079geometry).
197terminalEXITisincomplete;UNKNOWNterminalcountsarenotacomparisonbaseline.
Atnearbybefore198terminalqualitytime,1974524.888s had2970q/1448J/3079geometry,
vs1984548.497s3529/1903/3107. Differenttrajectories/hostconditions,notisolatedcodegain.

198connectedqualificationPASS:build54.189982s/1296pinsunchanged;proof403assertions/
2cases18.287446s;affectedserial20145/64cases244.606102s;MPI6216casesEACH6ranks
282.659565s.AllownedqualificationhandlesABSENT/pinsunchangedbeforeM6.
R/geometry_coverage198_qualification.json. NoCpp/binarychangeafterthisqualification.
SameoriginalCFDseed/config/mesh/native_ref/actualoriginalrestart00000BYTEIDENTICAL197.
Original61330points/303359cells;target50kcomplexityisnotpointcount. CL.269535/CD.013536,
rms-5.735442;original00000isNOTadaptedrestartproof. Allstrictgates andnative5400/
outer14400/2eventsunchanged. NoCADrequired.

Afterterminal:CPUpsiavg10/60~1%,MemAvailable39599648kB,load2.17/4.87/6.88;
runhadCPUpsiavg6025.59%andload12.9snapshots. Resourceschanged,buttimingcausality
isnotisolated. SampledsummedRSSmax15.543GB(decimal),notphysical/truewholejobpeak.
ROOT/tmpfreeONLY15MB;4TBworkspace61.46GBfree,/dev/shm32GBfree. No unrelatedfiles/jobs
modified. Beforenextfullacceptanceattempt,redirectTMPDIRandinstalledOpenMPI
OMPI_MCA_orte_tmpdir_base to4TBcompile-tmp;keepvaderdefault/dev/shm. Thisisruntime
storage,notnewnativeconfig/transport/math. Recordactualchildenvironment/paths.
This plan was executed as ACTIVEretry1 above after resources/tempstorage checks:
reusequalified198inSAMEcycleforfulltwo-eventM6same savedrealCFD. No newbaselinecampaign or
sourcechange. Strictacceptedmeshes/transfer/finiteadaptedCFD/isolatedadaptedrestart,
thenBL/MPI/RANS500k remainrequired. GoalACTIVE.

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

198firstfulltwoeventM6 is TERMINAL with complete measured rejection.
Same-source retry1 is LIVE with resource/tempstorage provenance verified. Keep it
frozen untilterminal orpreservedfailure. Compare actual complete censuses and
countedwork versus197 and parent198,without
confusing unmeasured construction shape with closure. No newimplementation while
this run is healthy; checkpoint accepted stages independently. Actualterminaloutcome
chooses nextblocker; regular evidence pushes and current handoff remainrequired.
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
