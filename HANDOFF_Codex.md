# Native adaptation — current handoff

Updated2026-11-01(Europe/Rome). Goal ACTIVE: TWO strictly accepted ONERA M6 Euler CFD → mesh →
CFD → mesh events at50k metric complexity; actual conservative admissible transfer,
finite continued CFD and isolated usable adapted disk restart. Then dedicated BL/
constraint/MPI qualification and corresponding RANS500k workflow. Actual point/cell
counts are separate from target complexity. Geometry1e-4,q>=.2,J>=.05,maxedge1.8 and
requested heights stay strict. Construction preserves exact positive/nondegenerate/
topology/body/feature/height validity and deliberately omits q/J. CAD optional;
no-CAD geometry authority sufficient. ZERO accepted adapted M6 meshes so far.

## Current —198TERMINAL; same-cycle acceptance retry preparation

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
handles HISTORICAL;NEVERresignal. Noownedheavyjob remains. NoCpp/header/test/helper/
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
Nextaction:reusequalified198inSAMEcycleforfulltwo-eventM6same savedrealCFD,
verifybetteractualhostresources andtempstoragefirst. No newbaselinecampaign or
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
Prepare a same-source acceptance retry only after resource/tempstorage provenance
is verified; once launched, keep it frozen untilterminal orpreservedfailure. Compare actual complete censuses and countedwork versus197,without
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
