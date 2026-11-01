# Native adaptation — current handoff

Updated2026-11-01(Europe/Rome). Goal ACTIVE: TWO strictly accepted ONERA M6 Euler CFD → mesh →
CFD → mesh events at50k metric complexity; actual conservative admissible transfer,
finite continued CFD and isolated usable adapted disk restart. Then dedicated BL/
constraint/MPI qualification and corresponding RANS500k workflow. Actual point/cell
counts are separate from target complexity. Geometry1e-4,q>=.2,J>=.05,maxedge1.8 and
requested heights stay strict. Construction preserves exact positive/nondegenerate/
topology/body/feature/height validity and deliberately omits q/J. CAD optional;
no-CAD geometry authority sufficient. ZERO accepted adapted M6 meshes so far.

## Current —196full two-event M6 ACTIVE; sources frozen

R=`integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57`.
CURRENT=`R/m6_runs/euler_build196_real_cfd50k_witness_first`.
PREVIOUS=`R/m6_runs/euler_build194_real_cfd50k_early_exit` (TERMINAL).
Sourcecheckpoint12afb10245pushed; CFDSHAbd71ad267291f8acdec981aaf7bdf221b09836e141ab8eeda64b733fdacb937c.

Fulltwoeventexec95607;actualHOSThandles verified1793499069.2033494:
968712/start91405628, 968834/start91406099, 968835/start91406099, 968836/start91406100, 968845/start91406106, 968844/start91406106, 968843/start91406106, 968842/start91406106, 968841/start91406106, 968840/start91406106.
All10LIVE/all1401pinsunchanged/82actualmappedELFcovered:CURRENT/owned_startup.json.
Oneownedheavyjob,nice19/libs1. NoCpp/header/test/helper/binary/config/input edits.
Keep196frozen until terminal or preserved evidence justifies an authorized stop.
SamequalifiedsavedrealCFD/config/mesh/native_ref as194/191:CURRENT/early_runtime_provenance.json.
OriginalcontinuedCFD CL.269535/CD.013536,rms-5.735442. ACTUAL originalrestart00000
BYTEIDENTICAL194(SHA6bbed8da75c40eca7364cc9d3fe96f13311fad69706e013acb68ba8feaf4240a),
verifiedafteremission:early_runtime_provenance.json. This is NOT an
adaptedrestart proof. Euler50k/native5400/outer14400/2events/allstrictgatesunchanged.
Initialpartition303359cells:ALL18selectedper-rankfieldsEXACT194.
CURRENT/initial_partition_comparison.json. CoarseningCOMPLETE292709workingcells/
2627collapses:ALL17selectedper-rankfieldsEXACT194,uppercalls14870same,
traversal260034825→191026912(-26.53795%),scope/facet/preparation/supportworksame.
Engine250.049109vs312.196318s; hostconditions differ,notisolatedcodetimingclaim.
CURRENT/coarsening_comparison.json,coarsening_resources.json; all10handles/1401pins/
82mappedELFreverified1793499439.866116:coarsening_complete_live_provenance.json.
ALL18constructiongenerations COMPLETE727818workingcells,sizebad0,geometrybad3770.
ALL17selectedper-rankfieldsEXACT194 throughout. Cumulative traversal1067849648→
746099652(-30.13065%),uppercalls33088same,scope/facet/preparation/discoveryworksame.
Engine656.347589vs847.303442s;notisolatedcodetimingclaim. Constructionq/Jwere
intentionallyUNMEASURED until qualityentry. CURRENT/construction_comparison.json,
split_generation_assessment.json.

Complete qualityENTRY692.592868072engine seconds,round21582:
727818cells,54081q/53392J/3770geometry/0size,qJdebt25413.45557726506.
Globalcounts/debt AND all6rankdebts/gaugesEXACT194entry;engine203.048942searlier.
Allcellsq/Jmeasured,allcomplete/cache/shape/geometryflags1;queue359054,blocked/stuck/
missing0,publications0atentry. Theseare ENTRYcounts,notlaterprogress/acceptance.
CURRENT/quality_entry_comparison.json,quality_entry_assessment.json,
all_quality_census_assessment.json. Initialqualitymigrationconservescounts/debt/
completeflags and20selectedcache/memocounters/entries/bytesunchanged;debtspread
15.776024→1.538801:quality_initial_ownership_assessment.json. Countsnotproofeverykey.
All10handles/1401pins/82mappedELFreverified1793499954.022988:
quality_entry_live_provenance.json. Frozenqualityrepairactive;noacceptedmesh.

Qualityprogress:window15 at2441.024688seconds sinceentry,round22528:
787641workingcells,14518q/11285J/3235geometry/0size,
qJdebt556.736861507279;30169publications,
net39563q/42107J/535geometryremoved,
16.207538netq/s,17.249723netJ/s.
Latestinterval:net1528q/1527J/18geometryremoved
in123.136261s,12.409017netq/s,
0.799163netq/publication. Allprecedingownershipstages included.
All40completecensusesaudited;latestownership_committedround22528:
queue179409,blockedfeatures195,stuckshape/required4/4,
missing0,privatecollectives0.
Same-round194reference:wholefieldexactTrue,rankdebtsexactTrue,
commitsexactTrue,windowcountcountersexactTrue.
Quality2441.024688vs3072.795281s;traversal
2855374798vs4058095853(-29.6376%).
Uppercalls658182vs658182. Noisolatedtimingclaim.
Remainingqsurface/interior2167/12351,
J3324/7961.
Interior meansnophysicalface,notnoconstrainedvertices;privategainsnotpublicationgains.
11latequalitymigrations audited:counts/debt/completeflags andselected
prepared/numericmemocounters/entries/bytes/driftavailabilityconserved.
Latestmigrationround22528:debtspread1.963278→1.397392,
observedfullcost10.995898s,awaitingnextcompletewindowinterval.
Countersalone donotproveeverykey/history;unchangedqualifiedsourcesupportscontracts.
CURRENT/quality_window15_service_summary.json preservesperrankbusy/join/commits/
publisheddebtremoved/netq/J. quality_window15_comparison.json retainsactualtimebrackets.
All10handles/1401pins/82mappedELFreverified1793502334.1710014:quality_window15_live_provenance.json.
Frozenruncontinues;noacceptedmesh/transfer/adaptedrestartclaim.


196sharedUpper tries ALREADYexistingrigorous convextrianglewitness BEFORE finite
projectedgraphcoverage. No newcache/API/config/scheduler/constructionoperator/batch/
budget/gatechange. Sharedcoarsen/split/quality/boundarysmoothing/fullcertificatepath.
Uncertainbounds retain graph+directedrefinement; association/entity/feature/body/
height/reversechecks unchanged. Belowtolnumericboundmaydiffergenerally,butgeometry
hingeEXACT0;saved12boundsEXACT194. Primaryproof:R/geometry_witness196_source_review.json.

194terminal drove195EXISTINGtrace attribution,withoutchanging bounds/decisions/
workcharges. All12savedbounds/workEXACT194. Coverage300573of360066traversalwork
(83.4772%);everycoveredgraphboundalreadybelowtol. Hardqueriesmaxdepth8/7. No new
motioncache:exact-rational oldsubtreeUpper+outwardmotion optimistic810/140510work
(0.5765%)beforecost; wholeface0of12. R/geometry_certification195_attribution.json,
geometry_certification195_subtree_motion_analysis.json; minimaldiagnostic.patch
reproduces195sourcehash against194sourcef518f2d727.

196connectedqualificationPASS BEFOREfullM6:
build45.152546s/all1296pinsunchanged/all2handlesabsent;proof403assertions/2cases
16.979416s/all5handlesabsent;serial20739assertions/68cases249.189708s/all5handlesabsent;
MPI6PASS216cases on EACH6ranks262.256337s/all10handlesabsent/pinsunchanged.
AllactualMPIidentities absent at1793499040.861584BEFOREfullM6launch.
R/geometry_witness196_qualification.json,validation_plan.json contain completecase
paths,command/runtime/sourcepins,all6ANSI-strippedsummaries andhostterminalreceipts.
Same12querytraversal360066→259729(-27.8663%),ALL12boundsEXACT194;4hardqueriesimprove
24.36–30.33%,8easyqueriescost3extraops(0.43–0.49%). Not fullM6speedup/acceptedmeshproof.
R/geometry_witness196_saved_query_comparison.json.

Comparewholefieldq/J/geometrycounts and netbadcellremoval/s,geometrywork/usefulprogress
at ACTUALmatchingstates/times vs194. Preserve rankbusy/join/debtspread,capacity/
conflicts,blockedtail,interiorvsboundaryyield/churn. Timersnest;joinincludesprotocol/
wait,CPUincludesMPIspin. Counters only,no perf/GDB/oldcodebaselinecampaign.
Shape/heightclosure→residualsize/geometry→shapereclosure underSAMEcumulativebudgets;
then TWOacceptedmeshes,actual conservativeadmissibletransfer,finitecontinuedCFD,
isolatedmatchingadapteddiskrestart,BL/MPI,RANS500k. No competingimplementation;
unprovenmotion/envelope/projection/finerpreparation alternatives remainbacklog.

## Preserved194outcome and cheaper-certification analysis — terminal

CURRENT194=`R/m6_runs/euler_build194_real_cfd50k_early_exit`.
ALL6naturaltimebudgetrejections,native5402.687673s; completeEXIT801912cells,
4369q/2352J/3111geometry/0size,qJdebt29.821866148054. ALL56completecensuses inclEXIT.
Quality4442.390488s,43286publications,49712netq/51040netJ/659netgeometry,
11.190372netq/s,11.489310netJ/s. Queue65423,blockedfeatures520,stuck29,missing0,
privatecollectives0. Shape notclosed; geometry-onlynotreached.0accepted adapted meshes.
All7groups and finalpartial98.945158s(708q/555J/6geometrynet)reconcile in
terminal_assessment.json,terminal_counter_review.json,terminal_geometry_service_assessment.json,
geometry_certification_analysis.json. NexttargetSPEEDwithblockedtailriskvisible;
shortedgescorrelationnotconstructioncause. Interiorremainingq3539/4369,J1678/2352.

Naturalrejectionpreserved event1_natural_failure.json BEFORE SIGTERM ONLY exact
remaininglauncher801607/start90716193,all10identities/1401pinscheckedbeforesignal.
Exec60431terminal1; all10ABSENT/all1401pinsunchanged1793497689.333789:terminal_handles.json.
Revalidated1793498118.744672BEFORE195edit. ALL801458/801605/801606/801607/801612..801617
HISTORICAL;NEVERresignal. Sourcef518f2d727remainedunchangedthroughthe194run.
SameCFDseed/config/mesh/native_ref/ACTUALoriginalcontinuedrestartbyteidentical193/191:
early_runtime_provenance.json. CL.269535/CD.013536onoriginalcontinuation,notadaptedrestart.
All18construction/entrycountfieldsEXACT193;727818cells,54081q/53392J/3770geometry.
Throughwindow18fields/counts/debts/commitsEXACT193with5.9092%lesstraversal;laterdiverged.
Atactualalmostmatchedqualitytimes4343.445330vs1934343.580956s,current5077q/2907J/3117geo
vsbaseline8654q/5779J/3151geo. Hostcontention/trajectoriespreventisolatedtimingclaim.
All15latemigrationsconserveauditedcounts/debt/completeflags/selectedcachecounters;
notproofeverykey/history.96.9508%upperworktraversal,exactunion49.7109%hits;misscausesmixed.
Boundaryprivate3848.10ranksec/345748upperqueriesvsinterior191.12sec/0;timersnest,
privategainsnotpublishedgains. Blocked520>280/stuck29>10increasedvs193;queuefell.
All16GLOBALdriftsamples3.0549%sinceentry;notimmutablecontinuousCFDtargetproof.
Terminalcheckpointdbe7189e24pushed. Fullgoal and strict acceptance remainunchanged.

## Preserved193outcome and analysis — historical handles, never resignal

ALL6ranks naturally time-budget rejected before any signal:native5403.239256s.
All49complete censuses audited; EXIT796684workingcells,8628q/5768J/3151geometry/
0size,qJdebt136.658541385625. Quality4348.781705s from54081q/53392J/3770geometry;
37144publications,net45453q/47624J/619geometry,10.4519netq/s,10.9511netJ/s,
1.22370netq/publication. Queue120828,blocked280,stuckshape/required10,missing0,
private trackedcollectives0. Shape never closed; geometry-only phase not reached.
ZEROaccepted adapted meshes/transfer/adapted restart. Failure preserved PREVIOUS/
event1_natural_failure.json BEFORE SIGTERM only verified launcher627536/start90036808;
all10identities/all1401pins checked before signal:event1_failure_owned_stop.json.
Exec83395terminal1;all10ABSENT/all1401pins unchanged1793491076.8872397:
terminal_handles.json; reverified before194edit:R/geometry_early_exit194_preflight.json.
ALL193PIDs627325/627534/627535/627536/627547..627552historical; NEVER resignal.

Compared191own-entry netq/s+18.01%,netJ/s+15.90%;traversal6.1464B→5.6645B(-7.84%),
work/netq162259.8→124623.4(-23.20%). Different trajectories/metrics/host contention:
no isolated code timing. Actual time brackets/nearstates:PREVIOUS/
quality_window*_comparison.json; endpoints:terminal191_comparison.json.
Upperwork5.8366B,traversal97.05%,facet/scopeconstruction11.965M. Exactunion memo
404824hits/401924misses(50.18%hits); misses mix nearby positions/support changes/
evicted identical queries. Boundary private299817upperqueries/3808.1ranksec vs
interior0/171.3; private gains not publication effects. Allseven diagnosticgroups
primary reviewed:terminal_counter_review.json,terminal_geometry_service_assessment.json,
terminal_assessment.json,geometry_certification_analysis.json. Next target SPEED
through same shared certification; positive yield/10stuck do not prove repairpower.

Existing unchanged-face/parameter-delta reuse,prepared geometry,starcontext,
cheapshape/height/edge/partialsum screens already reviewed. Simple priorupper+
maxpairedvertexmotion bound certifies0of12savedqueries; no motioncache justified.
193all18construction complete727818cells,size0,geometry3770,q/JUNMEASUREDthen;
qualityentry990.484282689engine sec. Trajectories differ from191generation2;
compareactualstates,not shifted window labels. Initialqualitymigration+13late
migrations conserve counts/debt/flags/selectedcachememocounters/entries/bytes/drift
availability. Source/qualifiedMPI6 support history; counters do not independently
prove every feature history/cache key.16GLOBALmetricsamples terminaldrift3.0549%
fromqualityentry,not immutable original continuous-CFD-target proof.

## Connected envelope change and qualification

[Design/proof](Handbook/ADAPTATION_GEOMETRY_ENVELOPE_20261031.md) follows191natural
failure. Existing PlaneUnion envelope now tries whole nominal source-triangle
coverage when its rectangle is too broad, adds a rigorous outward box-displacement
padding once, and tightens residuals with concrete nonnegative coupled half-plane
dual bounds. Existing hole-aware footprint/index/prepared pools/outward arithmetic
reused. Uncertain bounds retain full traversal; N10reverse path unchanged. No new
cache/options/scheduler/repair slots/time limits or construction operators. Preserve
191memo,190ownership/history,whole-patch merit,oldest fairness,worst-first/rejected-
star invalidation,full-vertex-disjoint reservation,fresh curved contexts,versioned
dependencies and complete publication. Association/features/bodies/heights remain.

Analysis first: simple prior-upper+uniform-vertex-displacement bound certifies ZERO
of12saved queries; no motion cache implemented. Exact union memo hits303285/misses
293078 in191quality; misses do not distinguish changed positions from evicted
identical queries. Existing unchanged-face coverage reuse,star contexts and cheap
screens reviewed before editing. Detailed negative proof in PREVIOUS/
saved_query_motion_bound_analysis.json (older168queries,not current191population).

192focused proof PASS307assertions/2cases; total saved-query traversal -4.25%,but
4queries worse.193refines only extra-bound scheduling when old objective already
certifies. Build193PASS57.163014s/all1296pins unchanged/all2host handles absent.
193proof PASS307assertions/2cases18.260551s; serial PASS68cases/20643assertions in
255.632856s; MPI6 PASS216cases on EACH6ranks in283.589702s. ALL respective host
handles absent/allpins unchanged BEFORE full M6. Qualification is focused,not a
full-suite or M6 acceptance claim. R/geometry_envelope193_qualification.json,
geometry_envelope193_source_review.json and geometry_envelope193_saved_query_comparison.json.
Exact SAME12saved-query traversal401486→379146 (-5.56%);8queries much cheaper,
4hard queries still worse. Cached/repeated/recomputed equality passed. The complete193outcome above shows improved net closure with changed trajectories;
no isolated code speedup or acceptance claim. Follow-up alternatives are backlog only.

## Preserved191outcome — historical, never resignal its handles

First event NATURALLY time-budget rejected ALL6ranks before any signal. Native max
5403.563701s; complete EXIT787840workingcells,16191q/12965J/3509geometry,size0,
qJdebt713.871163452273. ALL shape/geometry/census/cache flags complete,allcells
measured. Quality4277.122739s,28245publications,net37880q/40412J/550geometry removed:
8.8564netq/s,1.3411netq/publication. Queue192741,blocked326,stuck9,missing0.
Raw failure preserved PREVIOUS/event1_natural_failure.json BEFORE SIGTERM only
verified remaining launcher401315. All10identities/all1401pins checked before signal;
all10ABSENT/all1401pins unchanged at1793482215 BEFORE192source edit. Receipts:
event1_failure_owned_stop.json,terminal_handles.json. Exec36816terminalexit1. All
401117/401313/401314/401315/401320..401325 handles HISTORICAL; NEVER resignal.

Next target was SPEED of useful closure,not a claim repair power is solved. Quality
traversal6.1464Bwork/4941.4summedranksec;upper596363calls/5423.3ranksec. Support/facet/
scope preparation much smaller. Private boundary218104upperqueries/3804.8ranksec vs
interior0queries/137.4ranksec. Private and publication gains differ; timers nested,
do not add. Interior13743of16191q-bad (no physical face,not no constrained vertices).
Coordinate-key churn330546qretired/292666introduced; moved cells retire/reintroduce
despite unchanged connectivity. Ten late complete-census migrations conserve global
counts/debt/flags/cache/memo/drift samples. All39complete censuses and ALL7diagnostic
groups reviewed: PREVIOUS/terminal_assessment.json,terminal_geometry_service_assessment.json,
terminal_three_build_comparison.json,all_quality_census_assessment.json,
quality_migration_assessment.json. Historical partial windows never replace EXIT.

191construction ALL18generations/727722cells,size0,geometry4059; selected counts
EXACT188/190, q/J deliberately UNMEASURED.191quality ENTRY54071q/53377J/4059geometry,
debt25236.7874.188terminal17190q/14369J/3526geometry,quality4452.518s,8.283netq/s;
190terminal30754q/28157J/3702geometry,quality4097.689s,5.690netq/s. Different
trajectories and host contention; no isolated wall speedup claim. Historical179
real-CFD terminal census INCOMPLETE,not zero bad cells. Old startup fields differ.

## Next monitoring and acceptance

196fulltwoeventM6 is ACTIVE/FROZEN after complete connectedserial/MPIqualification.
Observecompletealignedfirst-eventcensuses and allwholefieldgates;compareactualuseful
states/work vs194,with timingcontentionand pathdifferences explicit. Keeponeowned
heavyjob,pins/handles/resourcesaudited,checkpointacceptedstagesindependently.
No newimplementationwhilethisrunishealthy. Useactualoutcometochoose thenextblocker.

Read-only downstreamreadinessreview:CURRENT/workflow_acceptance_readiness.json.
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

Open limits:191transported nodal tensors drifted~8.3%at16fixed samples;193sampled
drift above is from its recordedwindow. Neither proves immutable original continuous-CFD-target
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
