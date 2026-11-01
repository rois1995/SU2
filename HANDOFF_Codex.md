# Native adaptation — current handoff

Updated2026-11-01(Europe/Rome). Goal ACTIVE: TWO strictly accepted ONERA M6 Euler CFD → mesh →
CFD → mesh events at50k metric complexity; actual conservative admissible transfer,
finite continued CFD and isolated usable adapted disk restart. Then dedicated BL/
constraint/MPI qualification and corresponding RANS500k workflow. Actual point/cell
counts are separate from target complexity. Geometry1e-4,q>=.2,J>=.05,maxedge1.8 and
requested heights stay strict. Construction preserves exact positive/nondegenerate/
topology/body/feature/height validity and deliberately omits q/J. CAD optional;
no-CAD geometry authority sufficient. ZERO accepted adapted M6 meshes so far.

## Current —193M6 TERMINAL; analysis first, no heavy job

R=`integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57`.
CURRENT=`R/m6_runs/euler_build193_real_cfd50k_envelope`.
PREVIOUS=`R/m6_runs/euler_build191_real_cfd50k_numeric_memo` (TERMINAL).

First event NATURALLY rejected on ALL6ranks at unchanged native time budget:
native maximum5403.239256s; all49complete censuses audited, EXIT796684workingcells,
8628q-bad/5768J-bad/3151geometry-bad/0size-bad,qJdebt136.658541385625.
Quality4348.781705s from its complete entry54081q/53392J/3770geometry:
37144publications,net45453q/47624J/619geometry removed,
10.4519netq/s,10.9511netJ/s,1.22370netq/publication.
Queue120828,blocked280,stuckshape/required10,missing0,private trackedcollectives0.
Shape never closed; geometry-only phase NOT reached. ZEROaccepted adapted meshes,
actual adapted transfer/continued CFD/disk restart remain unproved.

Failure saved in CURRENT/event1_natural_failure.json BEFORE SIGTERM only verified
remaining launcher627536/start90036808. All10 exact host identities/all1401pins
verified before signal: event1_failure_owned_stop.json. Exec83395terminalexit1;
ALL10 host handles ABSENT/all1401pins unchanged at1793491076.8872397:
terminal_handles.json. Historical193PIDs627325/627534/627535/627536/627547..627552
must NEVER be signaled again. No owned heavy job remains; source freeze lifted
only after independent terminal audit. No new C++ edit/build has been made.
Source51c8323724; CFD SHA
8a1a2cb30f8300ac7ba281d94e6ca5e8af96638f29dad126a287e63aed308e58.
Config/mesh/native_ref/savedCFD/actual original-grid continued restart00000
BYTEIDENTICAL191: early_runtime_provenance.json. Initial CFD CL.269535/CD.013536;
same saved force-stable seed,50k/native5400/allgates unchanged.

Compared191 natural EXIT:16191q/12965J/3509geometry at4277.122739qualityseconds,
28245publications.193own-entry netq/s +18.01%,netJ/s +15.90%,total traversal
6.1464B→5.6645B(-7.84%),work/netq162259.8→124623.4(-23.20%). Different
meshes/paths/metrics and host contention prevent an isolated code timing claim.
193has fewer remaining bad cells, but is still REJECTED. Actual-time brackets and
near-state comparisons: quality_window*_comparison.json; final endpoints:
terminal191_comparison.json. Do not compare shifted window labels as equal work.

Next target remains SPEED of useful quality closure through the SAME shared geometry
path; positive yield/10stuck cells do not prove eventual repair power. Upper work
5.8366B,ofwhich traversal5.6645B(97.05%);facet/scope construction11.965Mwork.
Nested summedrank timers: upper5217.7s,traversal4640.0s,scope389.5s,facet105.3s;
never add. Exact union memo404824hits/401924misses (50.18%hits). Misses mix changed
positions/supports and evicted exact repeats; counters cannot disambiguate them.
Private boundary smoothing299817upperqueries/3808.1ranksec vsinterior0/171.3ranksec;
private merit gain is NOT published cavity gain. Existing context/certificate/screening
support and small-motion proof reviewed before adding anything:
[analysis](Handbook/ADAPTATION_GEOMETRY_CERTIFICATION_20261101.md), CURRENT/
geometry_certification_analysis.json,terminal_geometry_service_assessment.json,
terminal_assessment.json and terminal_counter_review.json (primary reviewed).

Analysis ONLY: prior-upper+uniform-displacement bound certifies0of12existing saved
queries. No motion cache justified. Existing Upper searches to preparationaccuracy/4
although caller accepts actual tolerance; caller/margin review identifies a possible
bounded earlier-success candidate, NOT measured savings or an implemented change.
No new configuration/cache/envelope/scheduler. Follow up within this cycle only
with saved-query counted-work evidence and affected serial/MPI qualification,
then immediately full TWO-event M6 using the SAME saved CFD and unchanged budgets.
Do not launch another old-code M6 baseline or perf/GDB campaign.

All18construction generations completed:727818workingcells,size0,geometry3770;
q/J deliberately UNMEASURED then. Complete quality entry990.484282689engine sec,
54081q/53392J/3770geometry,debt25413.45557726506. Coarsening selected17rankfields
EXACT191; firstsplit also exact; trajectories diverge fromgeneration2. Final
constructionround21582vs19121566. Detailed preserved construction_comparison,
coarsening_comparison,split_generation_assessment,quality_entry_comparison JSON.
Initialqualityownershipdebtspread15.776→1.5388;13late migrations conserve counts/
debt/completeflags and selected prepared/numericmemo counters/entries/bytes/drift
availability. Actual counters do not independently prove every feature history or
cache key; source/qualifiedMPI6 support the unchanged epoch/mask serialization.
quality_migration_assessment.json and quality_migration_review.json retain limits.
16GLOBALfixed metricsamples supported/lost16/0; terminal maxrelative drift3.0549%
sincequalityentry,not immutable original continuous-CFD-target proof. Resource
receipts retained; sampled sumRSS is not physical/true peak. No RAMstop was needed.

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

193is terminal with preserved evidence. In the next frozen attempt, observe existing
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
