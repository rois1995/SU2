# Native adaptation — current handoff

Updated2026-11-01(Europe/Rome). Goal ACTIVE: TWO strictly accepted ONERA M6 Euler CFD → mesh →
CFD → mesh events at50k metric complexity; actual conservative admissible transfer,
finite continued CFD and isolated usable adapted disk restart. Then dedicated BL/
constraint/MPI qualification and corresponding RANS500k workflow. Actual point/cell
counts are separate from target complexity. Geometry1e-4,q>=.2,J>=.05,maxedge1.8 and
requested heights stay strict. Construction preserves exact positive/nondegenerate/
topology/body/feature/height validity and deliberately omits q/J. CAD optional;
no-CAD geometry authority sufficient. ZERO accepted adapted M6 meshes so far.

## Current —197TERMINAL; certification analysis complete,goal ACTIVE

R=`integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57`.
CURRENT=`R/m6_runs/euler_build197_real_cfd50k_geometry_return`.
PREVIOUS=`R/m6_runs/euler_build196_real_cfd50k_witness_first` (TERMINAL).
Sourcecheckpoint27778de652pushed; CFD SHA7215d61fe30a6d77ec294ac67adb7fadea416ea4bde3a758b3a06810d46ce240.
Exec74120TERMINAL1. ALL6natural time-budget rejections,native5403.632129s,
refine5346.590181s. Failure preserved BEFORE stopping ONLY exact remaining owned
launcher1129722/start92102820. All10identities/all1401pins checked before signal;
all10ABSENT/all1401pinsunchanged1793511528.269277:CURRENT/terminal_handles.json.
ALL1129577/1129720/1129721/1129722/1129726..1129731handles HISTORICAL; NEVERresignal.
No owned heavy workload remains. CURRENT/terminal_resources.json preserves sampled
RSS/RAM/pressure/disk receipts; sampled summedRSS isnottruephysicalwholejobpeak.
No Cpp/header/test/helper/binary/config/input
change since197qualification. Git/evidence changes do not alter runtime pins.

Firstevent consumed a natural failure:ZEROaccepted adapted meshes. Terminal EXIT
803358cells has incomplete measurements/cache on five ranks; whole-field terminal
q/J/geometry/size UNKNOWN/null. DoNOTreportpartial15q/7J/524geometry asglobalcounts.
CURRENT/event1_natural_failure.json,event1_failure_owned_stop.json,terminal_assessment.json.
Final5.199104s/43publications afterlastcompletecensus have no complete churn/ladder
window; doNOTinvent netbad-cell counts. construction_returns=0:197's new residual
geometry continuation was NOTexercised or disproved onM6. Initial construction and
quality paths retain196's qualified witness-first certification.

LASTCOMPLETEqualitywindow26/census62/round23232,4649.703851quality seconds:
803337cells,2299q/1062J/3079geometry/0size bad,qJdebt10.996957994081.
Net51782q/52330J/691geometry through47064publishedcavities,
11.136623netq/s,11.254480netJ/s,1.100246netq/publication.
Latest124.815560s interval net671q/386J/0geometry,5.375932netq/s.
All63completequalitycensuses/ALL7diagnosticgroups audited. Queue36418,blocked601,
stuck35,missing0,privatecollectives0. qsurface/interior445/1854,J318/744;
interior meansnophysicalface,notnoconstrainedvertices. All3123bad-unioncellsshort<.9,
3075short<.5:correlation,notconstructioncause.16GLOBALdriftsamplessupported/lost0,
max2.644192%,notimmutablecontinuousoriginalCFDtargetproof.
17latequalitymigrations conservecounts/debt/completeflags and20selectedprepared/
numericmemo counters/entries/bytes/driftavailability;notproofeverykey/history.
CURRENT/terminal_counter_review.json,all_quality_census_assessment.json,
quality_window26_service_summary.json,quality_migration_assessment.json.

Actual196same-round23232:2044q/916J/3076geometry at4221.765971quality seconds;
1972299/1062/3079at4649.703851s with2.9457%lesstraversal. First22windows had
matchingmeasuredfields/work;latertrajectoriesdiffer. No isolatedcode-speedclaim.
Observedwholequalitybusyshare.286695/joincallshare.430103; joinincludesprotocol+
wait,timersoverlap,CPUincludesMPIspin. Timestampedcontentionreceipts inCURRENT/
quality_window9_host_pressure.json andquality_window10_wait_resource_service.json
arepassiveobservations,notwholewindowCPU/usefulworkcausalityproof.

Analysis-first request now completed using actual terminal outcome/existing counters:
CURRENT/geometry_certification_analysis.json,terminal_geometry_service_assessment.json;
[certification analysis](Handbook/ADAPTATION_GEOMETRY_CERTIFICATION_20261101.md).
4.858204352Btraversal/5.072525376Bupperwork(95.7749%);exactunionmemo496481hits/
509764misses(49.3400%). Aggregate misses cannotseparate nearbypositions/support/
tolerance fromidenticalevictedqueries. Existingolderpayloadhas6changed-coordinate
sameface/supportpairs,notcurrentmisspopulation. Existingunchangedfaceassociation/
coverage reuse,exactnumericmemo,preparedscopes/subtrees,star/oldmeritcontext and
validcheapcandidate screening reviewed. Boundary3866.334ranksec/privategain905.457
vsinterior213.939/privategain4635.365;privategainsarenotpublishedwholefieldq/J.
Costs overlapwithgeometryupper/traversal; donotadd. Boundaryshape screeningalready
rejects529951of608512candidates(87.09%);no blankettrialcut justified.

Withidenticalimmutabletarget/support andcorrespondence,outward(U+D) boundsnew
one-sided distance; associations/features/bodies/heights/reversecoverage remain
independent. Existingrigorous motion/slackevidence certifies0of12wholequeries;
optimisticsubtree810/140510work(.5765%)BEFORElookup/storage/invalidation. No new
motioncache/envelope/config/scheduler selected. Samplingneverreplacescertification.
Existingoldtrace77.8912%coveragework supports furtherMEASUREDevaluation,notproven
197speedup. Unprovenalternatives remainbacklog,notanotherimplementationcycle.
Nexttarget from197is usefulqualityclosureSPEED while retainingtheactive197boundary
repair-power/geometrycontinuation. ItsM6utility remainsunqualified. DoNOTraisebudgets
orclaimitfailed whenitwasneverreached. Fullstrictacceptance remainsrequired.

197connectedqualificationPASS BEFOREfullM6:build189.466310s/1296pinsunchanged;
focused213assertions/1case;affectedserial6411/18;MPI6PASS216cases EACH6ranks.
Allqualificationhandles terminal/pinsunchangedbeforefullrun. R/residual_geometry197_qualification.json.
Design:[residual geometry](Handbook/ADAPTATION_RESIDUAL_GEOMETRY_20261101.md).
Initial18constructiongenscounts/workEXACT196:727818cells,0size/3770geometry,
q/JintentionallyUNMEASURED. FirstqualityENTRY690.404172engine seconds:
54081q/53392J/3770geometry,counts/rankdebtsEXACT196. Initialmigration debtspread
15.776024→1.538801,selectedcountersconserved. No197phase-returnM6proof.

SavedrealCFD/config/mesh/native_ref andACTUALoriginalcontinuedrestart00000verified
BYTEIDENTICAL196. CURRENT/early_runtime_provenance.json. CL.269535/CD.013536,
rms-5.735442. This isNOTadaptedrestartproof. Euler50k/native5400/outer14400/2events/
allstrictgates unchanged. Original61330points/303359cells;workingcountsseparate.
Full2eventacceptance,conservativeadmissibletransfer,finiteadaptedCFD,isolatedadapted
diskrestart remainrequired,thenBL/MPI/RANS500k. GoalACTIVE,notcomplete/blocked/paused.

## Preserved196terminal outcome and analysis

R=`integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57`.
RUN196=`R/m6_runs/euler_build196_real_cfd50k_witness_first`.
PREVIOUS=`R/m6_runs/euler_build194_real_cfd50k_early_exit` (TERMINAL).
Sourcecheckpoint12afb10245pushed; CFDSHAbd71ad267291f8acdec981aaf7bdf221b09836e141ab8eeda64b733fdacb937c.
Exec95607TERMINAL1. ALL6natural time-budget rejections,native5402.707783s,
refine5345.112232s. Failure preserved BEFORE signaling ONLY exact remaining owned
launcher968836/start91406100,all10identities/1401pins verified before signal.
ALL10hostidentitiesABSENT/all1401pinsunchanged1793504580.738665:terminal_handles.json.
ALL968xxxhandles are HISTORICAL; NEVERresignal. No owned heavy job remains.
196source/input pins were unchanged through terminal verification;197edits start only afterward.

Samequalified saved real-CFD/config/originalmesh/native_ref as194/191:
early_runtime_provenance.json. OriginalcontinuedCFD CL.269535/CD.013536,rms-5.735442.
ACTUAL originalrestart00000 byteidentical194(SHA6bbed8da75c40eca7364cc9d3fe96f13311fad69706e013acb68ba8feaf4240a).
That is NOT an adaptedrestart proof. Euler50k/native5400/outer14400/2events/allgatesunchanged.
Original61330points/303359cells; metriccomplexity target is not meshpointcount.

All18initialconstructiongenerations COMPLETE727818cells,size0,geometry3770;
q/JintentionallyUNMEASURED untilqualityentry. ALL17selectedper-rankconstructionfields
EXACT194; traversal1067849648→746099652(-30.13065%),sameuppercalls33088,
scope/facet/preparation/discoverywork unchanged. initial_partition_comparison.json,
coarsening_comparison.json,construction_comparison.json preserve earlier work receipts.
Qualityentry692.592868s,54081q/53392J/3770geometry/0size,debt25413.45557726506,
counts/debts/all6rankgaugefieldsEXACT194entry. Initialmigrationconservescounts/
debt/completeflags+20selectedmemo/cachecounters; debtspread15.776024→1.538801.

Firstshape-pass COMPLETE measured return to construction atengine5243.296932372,
round24056:804092cells,47q/26J/3051geometry/0size,debt6.328205450462.
Shape did NOTclose; geometry-only repair was NOTreached. Quality4550.7040643s,
50883qualitypublications from cumulativeacceptedcounterdelta,net54034q/53366J/
719geometry,11.873767netq/s,11.726977netJ/s.
83completequalitycensuses/38windows audited; last window at5236.121391430:
48q/26J/3052geometry,queue19,blockedfeatures582,stuck46,missing0,privatecollectives0.
Published windowchurn413246qretired/359213introduced,353926Jretired/300560introduced;
ladder q/J/debt/publications reconcile for50868fullyemittedqualitypublications.
Finalpartial7.175541s/15publications/net1q,0J,1geometry lacks a quality-ladder row;
keep independently measured stage deltas separate,do not invent missing counters.
quality_first_pass_construction_return.json,terminal_assessment.json,terminal_counter_review.json.

Lastcompletequalitywindow q45surface/3interior,J26surface/0interior;46of48badcells
have allrequiredfeaturesblocked. Only6q/2J near genuinefeatures,unknown0.
All48bad-unioncells haveanedge<.5; correlation does NOTprove constructioncause.
16GLOBALdriftsamples supported/no lost,max2.7962%; notimmutablecontinuousCFDtargetproof.
At actualalmostmatching qualitytime4444.594995vs1944442.390488s:
196569q/221J/3063geometry vs1944369q/2352J/3111geometry. Trajectories/hostconditionsdiffer;
not isolatedcode timing. Throughwindow20 measuredfields/debts/publications EXACT194
with~29%lesstraversal; fromwindow21trajectories differ. Laterreference states notextrapolated.
21latequalitymigrations conserveauditedcounts/debt/flags/selectedcache/memocounters;
notproofeverykey/history. Privateboundary/interiorsmoothing gains are notpublicationgains.

After the blockedshape pass, existingphase logic returned to broadcoarsening.
Terminal EXIT797738cells has INCOMPLETEcensus: measured797697, q/Jmeasured785651.
Wholefieldterminal q/J/geometry/size counts are UNKNOWN/null; cached47/26/2921/0
are measuredsubsetvalues,NEVERaccepted or wholefield terminalcounts.
No assertion that construction worsened or repaired shape is justified by these data.
event1_natural_failure.json,event1_failure_owned_stop.json,terminal_resources.json.

Nexttarget: REPAIRPOWER for the residual physical-boundary shape/geometry coupling,
while retaining196certification savings. Speedalone doesnotreopen a rejectedunchangedstar.
197selected the existingfinite refinement continuation described above; itsfullM6
firsteventnaturallytime-budgetrejectedbeforethecontinuationwasreached. SeeCURRENT197terminalanalysis.
Do not start another baseline/perf/GDB campaign,raisebudgets,relaxgates or addconfigoptions.
Cheaper-certanalysis reuses196proof trace:259729traversalwork,202306coverage(77.8912%),
11489lift,32289distance; allcoveredgraphboundsalreadybelowtol. Cold/recomputedworks
agree. Same12older168queries,NOT currentmissfrequency or blockedstarrepresentativeness.
R/geometry_certification196_existing_trace_attribution.json. Exactunionmemoqualityhits
528359/misses551243; misses doNOTclassify newpositions/support/tolerance vsevictedrepeats.
Wholeface motion-slack0of12; optimistic subtreeworkceiling0.5765%beforecachecost.
No motioncache/newenvelope justified; existingtriangle-firstcoverage remainsUNPROVENbacklog.
Fullcurrentanalysis:Handbook/ADAPTATION_GEOMETRY_CERTIFICATION_20261101.md.

196sharedUpper tries ALREADYexistingrigorous convextrianglewitness BEFORE finite
projectedgraphcoverage. No newcache/API/config/scheduler/constructionoperator/batch/
budget/gatechange. Sharedcoarsen/split/quality/boundarysmoothing/fullcertificatepath.
Uncertainbounds retain graph+directedrefinement; association/entity/feature/body/
height/reversechecks unchanged. Belowtolnumericboundmaydiffergenerally,butgeometry
hingeEXACT0;saved12boundsEXACT194. Primaryproof:R/geometry_witness196_source_review.json.
PASSIVE matchedwindow15 private-smoothing review:RUN196/quality_window15_smoothing_comparison.json.
Boundary/interior counts andprivategainsEXACT194;boundary2077.90vs2587.34summedranksec,
interior132.15vs144.86. Timersnest/hostconditionsdiffer;privategainsnotpublicationgains.
No screening/trial/repairdecision changes or extra profiler were used.


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

197fulltwoeventM6 is TERMINAL after natural first-event time-budget rejection.
Terminal counters and cheaper-certification analysis are preserved above. Keep the
existing197cycle; select further code only from measured useful-work savings.
No new implementation is selected by the analysis. Verify host resources and all
provenance before another heavy job; preserve independently accepted stages.

Read-only downstreamreadinessreview:RUN196/workflow_acceptance_readiness.json.
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
