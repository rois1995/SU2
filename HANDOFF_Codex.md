# Native adaptation — current handoff

Updated2026-11-01(Europe/Rome). Goal ACTIVE: TWO strictly accepted ONERA M6 Euler CFD → mesh →
CFD → mesh events at50k metric complexity; actual conservative admissible transfer,
finite continued CFD and isolated usable adapted disk restart. Then dedicated BL/
constraint/MPI qualification and corresponding RANS500k workflow. Actual point/cell
counts are separate from target complexity. Geometry1e-4,q>=.2,J>=.05,maxedge1.8 and
requested heights stay strict. Construction preserves exact positive/nondegenerate/
topology/body/feature/height validity and deliberately omits q/J. CAD optional;
no-CAD geometry authority sufficient. ZERO accepted adapted M6 meshes so far.

## Current —198full TWO-event M6 ACTIVE; sources FROZEN

R=`integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57`.
CURRENT=`R/m6_runs/euler_build198_real_cfd50k_triangle_coverage`.
PREVIOUS=`R/m6_runs/euler_build197_real_cfd50k_geometry_return` (TERMINAL).
Sourcecheckpoint7aa53848eaPUSHED; CFD SHA96bcaff470bb4571966a2db93e0e7ed0b0920e46a9b3483a0898cc7f0b19a1f1.
TestdriverSHA9fa9bd82d0e3ec89b2930fa20a6ee6471b9b70ff2b3d8255efd9b9d6fb817642.
Fulltwoeventexec72458 LIVE/FROZEN. ActualHOSThandles1793512551.1515205:
helper1293335/start92753994;time1293492/timeout1293493/start92754543;
launcher1293494/start92754544;ranks1293498/1293499/1293500/start92754549,
1293501/1293502/1293504/start92754550. All10LIVE/all1401pinsunchanged/82mappedELF
covered:CURRENT/owned_startup.json. Oneownedheavyjob,nice19/libs1. NoCpp/header/test/
helper/binary/config/input edits while executing. VerifyactualHOSTidentity before
any authorized stop; absentnamespace-localPID isnotterminalproof. NEVERsignal
historical197/196/MPIvalidationhandles.

Same savedrealCFDseed,Euler50k/native5400/outer14400/2events andstrictgates unchanged.
Config/mesh/native_ref/savedseed BYTEIDENTICAL197:CURRENT/early_runtime_provenance.json.
ActualoriginalcontinuedCFDCL.269535/CD.013536,rms-5.735442. ACTUALoriginalrestart00000
nowemitted,BYTEIDENTICAL197(SHA6bbed8da75c40eca7364cc9d3fe96f13311fad69706e013acb68ba8feaf4240a).
Original00000isNOTadaptedrestartproof. Original61330points/303359cells;targetcomplexity isnotpoints.
No198accepted adaptedmesh/transfer/continuedadaptedCFD/adapteddiskrestart yet.
Firstnativepartition303359cells:ALL18selectedper-rankfieldsEXACT197.
CURRENT/initial_partition_comparison.json. Q/JintentionallyUNMEASUREDinconstruction;
noacceptedmesh orshapeclosureclaim. All10actualHOSThandles/1401pins/82mappedELF
reverified1793512765.9127417:initial_partition_live_provenance.json.
CURRENT/initial_partition_resources.json preserves sampledRSS/RAM/loadwithpeaklimits.
Earlierinitial_live_reverification.json remains historical. Observeactualoutcome;
no newbaseline/perf/GDBcampaign orcompetingimplementation.

FirstcoarseningCOMPLETE292709cells/2627collapses:ALL17selectedper-rankfields
EXACT197,includinggeometry/size and complete measurement flags. Q/Jdeliberately
UNMEASURED. Uppercalls14870same; traversal191026912→158224112(-17.1718%),
upperwork199952178→167149378;support/facet/scope/pieceselectionworkUNCHANGED.
Engine266.985899vs197256.557136s;noclaimofisolatedcodewalltimespeedup.
CURRENT/coarsening_comparison.json. ActualHOSTall10handles/1401pins/82mappedELF
verified1793512980.9799993:coarsening_live_provenance.json. Currentcoarsening_resources.json
preserves sampledRSS/RAM/load/CPUpsi/memorypsi/disklimits;noresource-pressurestop.
Frozenruncontinues into splitting; no accepted mesh ormeasured shapeclosure.

ALL18completedinitialsplitgenerations retain17selectedper-rankfieldsEXACT197.
Latestgeneration18:727818cells,0sizebad/3770geometrybad,q/JUNMEASURED.
Uppercalls33088same;traversal746099652→532351350(-28.6488%),support/facet/scope/
pieceselectionworkunchanged. Engine738.427450vs197654.713796s;noisolatedwalltime
speedupclaim. CURRENT/split_generation_assessment.json,construction_comparison.json.
Historicalfirstgeneration1and4liveauditreceipts retained,notcurrentphaseclaims.

FIRSTCOMPLETEqualityENTRY789.000275927engine seconds,round21582:
727818cells,54081q/53392J/3770geometry/0size,qJdebt25413.45557726506.
Allcounts/debt/rankdebts/gaugesEXACT197entry;98.596104engine seconds later.
Cache/census/shape/geometryflagsALL1,allcellsmeasured. Queue359054,blocked/stuck/
missing0,qualitypublications0atentry. Initialmigration preservesglobalcounts/debt/
completeflags and20selectedcache/memo counters/bytes/entries;debtspread15.776024→
1.538801. Countersdonotproveeverykey/history. CURRENT/quality_entry_comparison.json,
quality_entry_assessment.json,quality_initial_ownership_assessment.json,
all_quality_census_assessment.json. Qualityrepairnowactive,noacceptedmesh.
ActualHOSTall10identities/1401pins/82mappedELFverified1793513597.250606:
quality_entry_live_provenance.json. quality_entry_resources.json preservessampledRSS/
RAM/load/pressure/diskwithpeaklimits. Noresource-pressurestopjustified.
New197residualgeometrycontinuation NOTyetexercised on198M6. DoNOTeditpinnedsources.

198extends197's existing residualgeometry continuation with ONLY an existing N3
finitecoverage proof order: nominaltriangle before rectangle; rectangle/segment
fallback for failed/degenerate/nonfinite triangle proofs; allpadding/orientation/
finitefootprint/lift/admission/finalgates unchanged. CNativeEngine3D.cpp UNCHANGED.
No newcache/config/scheduler/gates/budgets; no-CADauthority stays sufficient.
Design:[certification](Handbook/ADAPTATION_GEOMETRY_CERTIFICATION_20261101.md).
R/geometry_coverage198_validation_plan.json,source_review.json,saved_query_assessment.json.
Saved12traversal259729→198551(-23.5546%),ALLboundsEXACT196,cached/recomputed equal.
All4hardqueries21–28%lesswork;all12improve. Trace proves61178lesscoveragework with
unchangeddomains1164/graphs790/distance32289/lift11489. Countedwork evidence,not
fullM6speedup/acceptance. R/geometry_coverage198_trace_attribution.json.

Firstmeasuredmatching-round divergence atwindow8/round22080:
q/J/geometrybadcountsSTILLequal197,but198has3fewercells(765317vs765320),
1morepublication(15543vs15542),rank0/globalqJdebt+.0009937660043.
CauseNOTisolated fromaggregatecounters. No assumptionofidenticaltrajectoryafter7;
compareactualbadcounts/work/stages/times,notassumedfuturestates. Allgates/pins
unchanged;nointerruption justified. CURRENT/quality_first_measured_divergence.json.

Qualityprogress:window17,round22656,qualityelapsed3265.043419s sinceFIRSTentry:
790664cells,13018q/9873J/3185geometry/0sizebad,
qJdebt422.050867617069;32234publishedqualitycavities.
Wholefieldnet41063q/43519J/585geometry,
12.576556netq/s,13.328766netJ/s,includingALLelapsedphases.
Publishedrepairnet41063q/43519J;constructioneditsafterfirstentry0.
Latestinterval138.611979s:net1198q/1097J/
35geometry,8.642832netq/s.
Constructioncrossinginterval=False;donotlabelconstructionchangesrepairchurn.
All42completequalitycensusesaudited;latestownership_committedround22656:
queue166479,blockedfeatures252,stuckshape/required
8/8,missing0,
privatecollectives0.
Actualsame-round197reference:fieldsexactFalse,rankdebtsexactFalse,
commitsexactFalse,windowcountcountersexactFalse;
relativegeometrytraversalworkchange-0.32091877.
No byteidenticalmesh orisolatedcodetimingclaim;timersnest/joinincludesprotocol+wait.
11latequalitymigrations audited:counts/debt/completeflags andselectedprepared/numericmemo
counters/entries/bytes/driftavailabilityconserved;notproofeverykey/history.
Latestmigrationround22656:debtspread3.712834→1.550844,
observedfullcost15.969329s,awaitingnextcompletewindowinterval.
CURRENT/quality_migration_assessment.json retains receipts;doNOTaddnestedatomic/fullcost timers.
Remainingqsurface/interior1839/11179,J2743/7130.
Interior meansnophysicalface,notnoconstrainedvertices. Private smoothinggainsarenotpublishedcavitygains.
CURRENT/quality_window17_service_summary.json,quality_window17_comparison.json,
quality_service_assessment.json,all_quality_census_assessment.json preserve counters andactualcomparisons.
ActualHOSTall10handles/1401pins/82mappedELFverified1793516752.51673:quality_window17_live_provenance.json.
Frozenruncontinues;nocensusaloneprovesmesh/workflowacceptance.

198connectedqualificationPASS BEFOREfullM6:
build54.189982s/1296pinsunchanged/2hosthandlesABSENT1793511884.925061;
focused403assertions/2cases18.287446s/5handlesABSENT/pinsunchanged;
affectedserial20145assertions/64cases244.606102s/5handlesABSENT/pinsunchanged;
MPI6PASS216cases EACH6ranks282.659565s/10actualHOSThandlesABSENT/pinsunchanged
1793512524.691580 BEFOREfullM6launch. Per-rankassertions31716/31982/35150/32505/
32468/31993. R/geometry_coverage198_qualification.json retainssummaries/provenance.
No broadfullsuite,fullM6orBL/RANSacceptanceclaim. Hostpreflight load4.64/5.20/5.68,
MemAvailable40068472kB,memorypsi0,noownedheavyprocesses,4TBfree61.51GB,
rootfree200.85MB: R/geometry_coverage198_M6_preflight.json. Use4TBcompile-tmp.

Compare netwholefieldq/Jbad/s,geometrybad,perrankbusy/join/debtspread/churn and
boundary/interioryield with197actualmatchingstages/work. Geometrywork/counters
muststay separatefromcontendedwalltime. Initialconstructionq/JareUNMEASURED.
Shape/heightclosure→residualsize/geometryrepair or197finitegeometrycontinuation→
freshshaperemeasurement/reclosure,samecumulativebudgets. ALLfinalstrictgates,
conservativeadmissibletransfer,finitecontinuedCFD andisolatedadapteddiskrestart
remainrequired,thenBL/MPI/RANS500k. GoalACTIVE;ZEROaccepted adaptedmeshes sofar.

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

198fulltwoeventM6 is ACTIVE/FROZEN after connected serial/MPI qualification.
Keep the full attempt running until terminal or preserved evidence justifies an
authorizedstop. Compare actual complete censuses and countedwork versus197,without
confusing unmeasured construction shape with closure. No newimplementation while
this run is healthy; checkpoint accepted stages independently. Actualterminaloutcome
chooses nextblocker; regular evidence pushes and current handoff remainrequired.
Analysis caution: the stored matching-window summary assumes the first quality
pass. If the197return crosses construction, reconcile published repair churn
separately from whole-field construction changes. Adapt analysis assertions to
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
