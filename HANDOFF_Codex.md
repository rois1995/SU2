# Native adaptation — current handoff

Updated2026-11-01(Europe/Rome). Goal ACTIVE: TWO strictly accepted ONERA M6 Euler CFD → mesh →
CFD → mesh events at50k metric complexity; actual conservative admissible transfer,
finite continued CFD and isolated usable adapted disk restart. Then dedicated BL/
constraint/MPI qualification and corresponding RANS500k workflow. Actual point/cell
counts are separate from target complexity. Geometry1e-4,q>=.2,J>=.05,maxedge1.8 and
requested heights stay strict. Construction preserves exact positive/nondegenerate/
topology/body/feature/height validity and deliberately omits q/J. CAD optional;
no-CAD geometry authority sufficient. ZERO accepted adapted M6 meshes so far.

## Current —193full two-event M6 ACTIVE; sources frozen

R=`integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57`.
CURRENT=`R/m6_runs/euler_build193_real_cfd50k_envelope`.
PREVIOUS=`R/m6_runs/euler_build191_real_cfd50k_numeric_memo` (TERMINAL).
Other historical comparisons: euler_build188_real_cfd50k_feature_safe and
 euler_build190_real_cfd50k_census_ownership under R/m6_runs.

Full2event exec83395; helper627325/start90036257,time627534/start90036807,
timeout627535 andlauncher627536/start90036808,ranks627547..627552/start90036815.
All10host identities/all1401pins/82actually mapped ELF files verified at1793485365;
CURRENT/owned_startup.json. Source51c8323724; CFD SHA
8a1a2cb30f8300ac7ba281d94e6ca5e8af96638f29dad126a287e63aed308e58.
Config/mesh/native_ref/savedCFD and actual continued original-grid restart00000
BYTEIDENTICAL191. CURRENT/early_runtime_provenance.json.
Initial CFD CL.269535/CD.013536; first remesh uses the same ITER1 continuation from
saved force-stable seed, never a fresh freestream field. Native5400s,strict50k and
all gates unchanged. Construction ALL18generations COMPLETE:727818workingcells,
sizebad0,geometrybad3770; ALL complete geometry/census/cache flags, q/J deliberately
UNMEASURED(shape_complete0). Cumulative traversal1131850186vs1911305219707(-13.2828%);
uppercalls33088vs33158; engineelapsed942.549761vs1002.191890s. Trajectories differ
fromgeneration2:96morecells/289fewer geometry-bad atconstructionexit. Stronger
certificates and changed mesh both contribute; no isolated timing/shape/acceptance
claim. Current finalconstructionround21582vs19121566: observation windows are
shifted, so do NOT compare same window index or round as identical quality work.
Complete quality ENTRY at990.484282689engine seconds:727818cells,54081q/53392J/
3770geometry,size0,qJdebt25413.45557726506; ALL cells measured/completeflags1.
Compared191entry:+96cells/+10q/+15J/-289geometry/+176.6681745253debt. Use each
run own entry and actual quality times,not shifted window labels/rounds. Initial
quality ownership migration conserved counts/debt/completeflags,selected cache/memo counters
AND live bytes/entries unchanged; debtmax/min15.7760→1.53880. First3complete
censuses audited,private trackedcollectives0/commits0atentry,no repair progress yet.
CURRENT/quality_entry_assessment.json,quality_entry_comparison.json,
quality_initial_ownership_assessment.json and all_quality_census_assessment.json.
All10handles/all1401pins/82ELF reverified1793486628:quality_entry_provenance.json.
Preserved quality WINDOW9 at2242.153528s sinceentry:769810workingcells,
26152q/23403J/3368geometry,size0,qJdebt2882.675577276201.
Cumulative17847publications,net27929q/29989J/402geometry removed:
12.4563netq/s,1.56491netq/publication. Latest interval:
3182q/3356J/55geometry in332.529239s,
9.5691netq/s (includes preceding migration cost). Migration AFTER
this window is charged to the next complete interval. All26complete censuses
are audited; latest stageownership_committed at round22144,queue259683,
blocked160,stuck/missing0,private trackedcollectives0.
Coordinate-key churn includes moved cells despite unchanged connectivity. Judge net
counts,not raw retirements/debt percent. Records:CURRENT/quality_trend_assessment.json,
quality_service_assessment.json and all_quality_census_assessment.json.

Actual nearest-q comparison:19326152q/23403J/3368geometry vs191
26177q/23273J/3668geometry,qualitytimes2242.154vs3094.790s;
publications17847vs19163,traversal3047417252vs4527402117
(32.69%less),work/netq109113.0vs162307.4.
Different meshes/paths/metrics and host contention: no interpolation,identical-work
or isolated timing claim. Actual time brackets and earlier snapshots are preserved:
CURRENT/quality_window1..9_comparison.json. Source freeze intact:CURRENT/
quality_window9_live_provenance.json verifiesall10handles/1401pins/82mappedELF.

7actual late complete-census migrations conserve counts/debt/flags and selected
prepared/numeric memo counters/entries/bytes/drift availability. Latest round22144:
debtmax/min1.64272→1.46377,
full observed cost14.412406s. CURRENT/
quality_migration_assessment.json; initial excluded by stage identity,not old round.
The unchanged14epoch/mask serialization and qualifiedMPI6 support history preservation;
actual counters do NOT independently prove every per-feature history or cache key.
Bounded primary-reviewed source/receipt limits:quality_migration_review.json (first2
late migrations,canonical10censusprefix hash retained).

Current badpopulation:21276q/15984Jinterior,4876q/7419Jsurface.
Interior means no physical face,not unconstrained vertices. Private boundary/interior
smoothing work/debt is separate in quality_service_assessment.json; it is NOT published
cavity gain. Timers nest; join calls include protocol/wait. Sixreports replicate
16GLOBALfixed metricsamples,supported16/lost0,maxdrift
3.22540%sincequalityentry; no original continuous-CFD-target fidelity proof.
Resource receipts:quality_window9_resources.json; sampled summedRSS is not physical
or true peak. Keep193frozen; choose further implementation from terminal outcome.


 Receipts CURRENT/
construction_comparison.json,split_generation_assessment.json,
construction_complete_provenance.json (all10handles/1401pins/82ELF at1793486484).
Coarsening selected17per-rank fields EXACT191:292709cells/2627collapses,uppercalls
14870same,traversal -5.4705%; firstsplit selected17fields EXACT191,traversal -8.156%.
Later counts differ. Initialpartition all18fields EXACT191;188differs only104B/rank
reported native accounting. CURRENT/coarsening_comparison.json and
initial_partition_comparison.json preserve matched-stage limits.

One owned heavy job. Do not edit ANY pinned C++/test/helper/binary/config/input.
Before restart/edit/stop, reverify exact actual host handles; sandbox process
namespaces cannot see host jobs. Use escalated read for host /proc audits. Do not
interpret namespace-local resource PIDs as host handles. No perf/GDB/new old-code
baseline campaign. Counters only at existing complete censuses, low overhead.

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
4hard queries still worse. Cached/repeated/recomputed equality passed. No production
speedup or net closure gain established. Follow-up alternatives are backlog only.

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

Keep193frozen until terminal or preserved evidence justifies authorized stop under
AGENTS. Observe existing complete aligned per-rank censuses: net whole-field q/J-
bad removal/s,geometry-bad counts,committed cavity yield/churn,interior versus boundary
work,geometry traversal/preparation/memo,rank debt spread,busy/join/resource evidence.
Join/collective-call time includes protocol and waiting; CPUtime includes MPIspin.
Compare actual matching stages/work; do not silently equate changed trajectories.
Original-grid continued restart00000 equality verified; this is not adapted restart evidence.

Existing closure plan: shape/height → residual size → geometry-only repair via
EnableGeometryRepairs/strict complete-patch guidance,or valid construction return,
then shape remeasurement/reclosure.191never reached geometry-only entry. Same
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
