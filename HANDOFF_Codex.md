# Native adaptation — current handoff

Updated2026-11-01(Europe/Rome). Goal ACTIVE: TWO strictly accepted ONERA M6 Euler CFD → mesh →
CFD → mesh events at50k metric complexity; actual conservative admissible transfer,
finite continued CFD and isolated usable adapted disk restart. Then dedicated BL/
constraint/MPI qualification and corresponding RANS500k workflow. Actual point/cell
counts are separate from target complexity. Geometry1e-4,q>=.2,J>=.05,maxedge1.8 and
requested heights stay strict. Construction preserves exact positive/nondegenerate/
topology/body/feature/height validity and deliberately omits q/J. CAD optional;
no-CAD geometry authority sufficient. ZERO accepted adapted M6 meshes so far.

## Current —194full two-event M6 ACTIVE; sources frozen

R=`integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57`.
CURRENT=`R/m6_runs/euler_build194_real_cfd50k_early_exit`.
PREVIOUS=`R/m6_runs/euler_build193_real_cfd50k_envelope` (TERMINAL).
OLDER=`R/m6_runs/euler_build191_real_cfd50k_numeric_memo` (TERMINAL).

Full2eventexec60431; helper801458/start90715641,time801605/timeout801606/
launcher801607/start90716193,ranks801612..801614/start90716197,
801615..801617/start90716198. All10actual host identities/all1401pins/82mappedELF
verified1793492159.6435194:CURRENT/owned_startup.json. Sourcef518f2d727; CFD SHA
434846cf1fca385747e52a8167b4912d853f7d4de65b9ea9642f71308966951a.
Config/mesh/native_ref/savedrealCFD BYTEIDENTICAL193:
early_runtime_provenance.json. Actual original-grid continued restart00000 now
BYTEIDENTICAL193(SHA6bbed8da75c40eca7364cc9d3fe96f13311fad69706e013acb68ba8feaf4240a).
Initial continued CFD CL.269535/CD.013536. This is NOT adapted restart proof.
Same seed,ITER1continuation,50k/native5400/outer14400/allstrictgates unchanged.
Initial partition303359cells:ALL18selected per-rankfields EXACT193.
Coarsening COMPLETE292709cells/2627collapses:ALL17selected fields EXACT193;
traversal277428465→260034825(-6.2696%),engine312.196318vs358.820959s.
ALL18construction generations COMPLETE727818workingcells,sizebad0,geometrybad3770;
ALL17selected per-rank countfields EXACT193 throughout. Cumulative traversal
1131850186→1067849648(-5.6545%),uppercalls33088same,scope/facet/preparation/support
work same. Engine847.303442vs942.549761s; wall conditions differ,not an isolated
code timing claim. Construction q/J deliberately UNMEASURED(shapecomplete0).
CURRENT/initial_partition_comparison.json,coarsening_comparison.json,
construction_comparison.json,split_generation_assessment.json preserve raw rows/limits.

Complete quality ENTRY at895.641809767engine seconds,round21582:
727818cells,54081q/53392J/3770geometry/0size,qJdebt25413.45557726506.
Globalcounts/debt AND all6rankdebts EXACT193qualityentry; engineentry94.842473s
earlier. Allcellsq/Jmeasured,allcompletecache/census/shape/geometryflags1. Entry queue359054,blocked/stuck/missing0,publications0; current progress below.
Current repair runs frozen; no accepted mesh or closure-rate claim atentry.
quality_entry_assessment.json,quality_entry_comparison.json,
all_quality_census_assessment.json. Initial quality ownership migration conserved
counts/debt/completeflags and20selectedcache/memocounters/entries/bytes unchanged;
debtmax/min15.776024→1.538801:quality_initial_ownership_assessment.json. Counter
identity does not independently prove every entrykey/feature history.

Quality progress (latest complete):window18 at3682.965882seconds sinceentry,
794720workingcells,10007q/7071J/3169geometry/0sizebad,
qJdebt199.404717080274;35271publications,net44074q/46321J/
601geometry removed,11.966986netq/s,12.577092netJ/s.
All49complete censuses audited,latestwindow round22720;queue136394,
blockedfeatures296,stuckshape/required9/9,missing0,
private trackedcollectives0.
quality_trend_assessment.json,quality_service_assessment.json,
all_quality_census_assessment.json preserve whole-field/netchurn/service/location data.
Actual matched-state summaries and actual-time brackets retain counts/work/limits:
quality_window*_service_summary.json and quality_window*_comparison.json.
Latest interval:net1588q/1497J/
24geometry removed in241.535784s,
6.574595netq/s,0.790837netq/publication.
Includes preceding migration cost; any newer transition belongs to its next interval.
Latest same-round193reference:wholefieldexactTrue,
rankdebtsexactTrue,commitsexactTrue.
Quality3682.965882vs4160.423044s;traversal
5105061364vs5425675543
(-5.9092%),netq/s
11.966986vs10.593634.
quality_window18_service_summary.json; actual-time brackets retained separately.
Remaining q/Jsurface1409/1957,
interior8598/5114;interior means no physical face,
not necessarily unconstrained vertices. Private merit gains are not publication gains.
14late complete-census migrations conserve counts/debt/flags and
selected prepared/numericmemo counters/entries/bytes/drift availability. Latest
round22720:debtspread1.89543→1.53978,
observedfullcost11.446501s;awaiting next completewindow interval.
quality_migration_assessment.json. Counters do not independently prove every
featureage/rejection history/cachekey; unchanged source/qualifiedMPI6 support them.
Quality progress end.

All10hosthandles/1401pins/82mappedELF reverified1793496950.014876:
quality_window18_live_provenance.json. quality_window18_resources.json preserves
sampled summedRSS/hostload/RAM/PSI; sampledRSS is not physical or truepeak.
No memorypressure stop justified. One owned heavyjob,nice19/libs1; no pinned
C++/test/helper/binary/config/input edits. Source remainsf518f2d727.

194is the bounded follow-up to193terminal analysis,not a second optimization cycle:
existing TriangleUnion::Upper stops at an already rigorous bound<=actual tolerance,
rather than searching for quarter-tolerance,and also stops after adequate triangle
witnesses. Same shared coarsen/split/quality/boundary path. No newcache/config/
scheduler/construction operators,batch sizes or budgets. Final directions/support/
association/features/body/height gates remain. Quantitative in-tolerance upper
values may change in general,contributing EXACT0to geometry hinge; fullproof/caller
limits:R/geometry_early_exit194_source_review.json and
[analysis](Handbook/ADAPTATION_GEOMETRY_CERTIFICATION_20261101.md).

Build194PASS84.234683s/all1296pins unchanged/all2hosthandles absent before proof.
ProofPASS403assertions/2cases18.236763s; serialPASS20739assertions/68cases257.556372s;
MPI6PASS216cases on EACH6ranks283.602851s. All respective5/5/10hosthandles absent/
allpins unchanged BEFORE M6. R/geometry_early_exit194_qualification.json.
Initial summary parser failed to strip MPI ANSI coloring; corrected summary reads
all6pass lines. Underlying exit/pins/terminal audits passed before launch; no test
failure/retry. Saved SAME12queries traversal379146→360066(-5.032%),ALL12lesswork,
ALLboundsEXACT193; cached/recomputed equality passed. No full-run speedup or accepted
mesh claim. R/geometry_early_exit194_saved_query_comparison.json.
Sourcef518f2d727and qualification/fullrun checkpointe45c88289bpushed.
CURRENT/native_setup_provenance.json reverifiedall10handles/1401pins/82ELF at
1793492296.0092645; native_entry_resources.json preserves headroom/load/sampleRSS.

Keep194frozen until terminal or preserved evidence justifies authorized stop under
AGENTS. Compare actual whole-field q/J-bad removal/s and geometry counts,geometry
work per useful progress,rankservice/join/debtspread,capacity/conflicts,interior vs
boundary yield/churn. Timers nest; no adding overlap or claiming pure MPI idle.
Same counters only/no perf/GDB/newoldcodebaseline campaign. Shape→residualgeometry/
size→shape reclosure under unchanged cumulative5400budget,thenactualtransfer/
continuedCFD/isolated adapted restart,BL/MPI,RANS500k.

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

193is terminal with preserved evidence. In the current194frozen attempt, observe existing
complete aligned per-rank censuses: net whole-field q/J-
bad removal/s,geometry-bad counts,committed cavity yield/churn,interior versus boundary
work,geometry traversal/preparation/memo,rank debt spread,busy/join/resource evidence.
Join/collective-call time includes protocol and waiting; CPUtime includes MPIspin.
Compare actual matching stages/work; do not silently equate changed trajectories.
Original-grid continued restart00000 equality verified; this is not adapted restart evidence.

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
