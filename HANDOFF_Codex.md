# Native adaptation — current handoff

Updated 2026-10-31. Full goal ACTIVE: TWO strictly accepted ONERA M6 Euler
CFD → mesh → CFD → mesh events at50k metric complexity, conservative admissible
actual transfer, finite continued CFD and isolated usable adapted disk restart.
Then dedicated BL/constraint/MPI qualification and corresponding RANS500k workflow.
Actual points/cells are separate from metric complexity. Geometry1e-4,q>=.2,J>=.05,
maxedge1.8,requested heights stay strict. Construction preserves exact positive/
nondegenerate/topology/body/feature/height validity and deliberately omits q/J.
CAD optional; no-CAD geometry authority sufficient. No accepted adapted M6 mesh yet.

## Authoritative state —191 TERMINAL; no heavy job; source freeze lifted

R=`integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57`.
CURRENT=`R/m6_runs/euler_build191_real_cfd50k_numeric_memo` (TERMINAL).
BASELINE=`R/m6_runs/euler_build188_real_cfd50k_feature_safe`.
PREVIOUS=`R/m6_runs/euler_build190_real_cfd50k_census_ownership`.
First event NATURALLY rejected on ALL6ranks:time_budget,root`Native3D private repair
round exhausts elapsed-time budget.` Native totalmax5403.563701s; complete EXIT:
787840cells,16191q/12965J/3509geometry,size0,qJdebt713.871163452273. Every shape/
geometry/census/cache flag complete,allcells measured. Quality elapsed4277.122739s
(entry1059.456116924,EXIT5336.578855643). Queue192741,blocked326,stuck9,missing0.
No accepted adapted mesh/actual transfer/adapted CFD/restart. Original CFD retained.

Raw6summaries/refine details/completeEXIT/root rejection saved BEFORE any signal:
CURRENT/event1_natural_failure.json. Exact verified owned launcher401315 SIGTERM
under standing preserved-failure stop/fix/retry authorization; all1401pins unchanged
and all10identities matched BEFORE signal:event1_failure_owned_stop.json. Exec36816
TERMINALexit1/statussolver_exit1 after stopping remaining attempt. Independent
1793482215audit ALL10handles ABSENT/all1401pins unchanged BEFORE ANY next C++edit:
CURRENT/terminal_handles.json. All401117/401313/401314/401315/401320..401325handles
are now HISTORICAL; never resignal/restart from old status. Source freeze LIFTED for
the authorized outcome-driven analysis/fix cycle. No next source edit/build yet.

Terminal counters and existing geometry facilities reviewed: next target SPEED of
useful closure. Quality retired net37880q/40412J/550geometry at8.8564netq/s;
28245publications,1.3411netq/publication. Geometry traversal6.1464Bwork/4941.4summed
rank seconds; exact union memo303285hits/293078misses. Misses do not distinguish
nearby trials from evicted identical queries. Interior holds13743of16191q-bad;
private boundary218104upper queries/3804.8ranksec vs interior0/137.4ranksec.
Private and publication gains differ; nested times must not be added. Ten late
complete-census migrations conserved counts/debt/flags. ALL7counter groups reviewed
in CURRENT/terminal_assessment.json; terminal_geometry_service_assessment.json and
terminal_three_build_comparison.json retain scope and contention caveats.

Analysis-first result: uniform movement/slack proof certifies ZERO of12saved queries
(CURRENT/saved_query_motion_bound_analysis.json). Do not implement a motion cache
on this evidence. Existing exact unchanged-face reuse, old star context, screening
and hole-aware projected envelopes already exist. The connected candidate tightens
that existing envelope with whole-triangle coverage and outward coupled half-plane
bounds: [design/proof](Handbook/ADAPTATION_GEOMETRY_ENVELOPE_20261031.md).
Primary and bounded read-only review found no proof blocker. It is NOT implemented
or an established speedup. Require measured saved-query work benefit, focused serial/
ONE MPI6 and immediate full two-event M6 from SAME seed/50k/5400, unchanged gates.

Build191 PASS754.100808s,all1296pins unchanged/all2handles absent before validation.
Serial PASS67cases/20499assertions,including hot/cold memo churn and12savedM6queries.
MPI6 PASS215cases on EACH6ranks in302.025170s,all10handles absent/allpins unchanged
before M6. R/numeric_memo191_qualification.json records exact cases/hashes/results.
CFD SHAab2192a723715eb5ca84ae67666aa1d83996e143928844fff75922e5cc22258e.
Source410d82c087; qualification/live provenance53cc669c4b; first-stage checkpoint
e175d78c3c pushed. These are focused checks, not a full-suite or M6 acceptance claim.

Construction CLOSED:ALL18generations,727722cells,sizebad0,geometrybad4059.
All17selected per-rank count/flag fields EXACT188 for ALL18generations. q/J were
intentionally UNMEASURED in construction; zeros were not shape success. Cumulative
traversal1305219707vs1881305964243 (-.05701%); uppercalls33158same. Memo retention
refusals0,activebytes<=fixed limits; prepared allowance396836624/rank vs188396836704
accounts80B extra fixed metadata. LIVE/construction_comparison.json and
split_generation_assessment.json retain completed snapshots. No wall-speed claim.

Quality ENTRY1059.456117native seconds:727722cells,54071q/53377J/4059geometry,size0,
qJdebt25236.787402739737. All14selected per-rank counts/debts/flags EXACT188/190.
LIVE/quality_entry_assessment.json,quality_entry_comparison.json. ALL1401pins/
10exacthandles/82mappedELFcoverage reverified at quality entry; quality_entry_provenance.json.
Preserved matched window2 at330.445887quality seconds:734404cells,49084q/47891J/
3994geometry,size0;2173publications,4987netq/5486netJ/65geometryremoved since entry.
Whole-field outcome/publications EXACT188/190 at same round21632. New traversal
488082888vs493424356 (-1.08253%),uppercalls33176same. Netq15.0917/s vs18815.6778/s,
19011.0331/s; wall-time confounded by host contention. Latest interval join-callshare
.49023 includes protocol and waiting; not pure idle. Rawslot candidatefill~94.95%,
reservationfraction84.375%,publication/reservation~85.62%; raw8-slot ceiling is not
required demand, and unreserved is not an exact conflict count. Memo18241hits/
14935misses,retentionrefusals4 (soft retain refusal, recomputation remains mandatory).
Old exact memo lacked hit/miss counters; newhitshare alone is not incremental benefit;
misses cannot label nearby trials vs evicted repeats. First short window had3.53%
less traversal,same78publications/netcounts; this is not sustained closure evidence.
ALL7complete censuses audited (including ownership stages),churn identities,flags,
globalmigrationconservation,privatecollectives0,missing0,stuck0. Latest one complete-
census redistribution round21632 committed in13.56315s including observations;
debtspread1.89661→1.47445,globalcounts/debt unchanged,ALLpreparedcache and numericmemo
counter/entry/byte deltas0,drift samples unchanged. No cache clearing or recertification.
LIVE/quality_window2_comparison.json,quality_service_assessment.json,
quality_migration_assessment.json,all_quality_census_assessment.json preserve detail.
Migration occurs AFTER window2 and belongs in next window's complete interval cost.
Historical partial windows and work comparisons are preserved in CURRENT JSON,
including quality_window7_comparison.json (last exact190same-round match,traversal
-3.00703%),quality_window10_comparison.json and quality_near_state_windows9_10.json
(explicit near-state count mismatches beyond190terminal). Do not substitute those
partial counts/times for current complete EXIT. Full39census ledger now audited:
all_quality_census_assessment.json,including ownership stages and finalEXIT. Raw
coordinate-key churn retires/reintroduces moved cells despite unchanged connectivity.
Prior window4 vs3 traversal/publication+69.7%,netq/publication-29.3% demonstrated
real work/yield changes that contention alone cannot explain; detailed proof in
quality_window4_work_diagnosis.json. No next implementation selected from partials.
Latest preterminal source/runtime/input audit1793480289alsoverifiedall1401pins/
10exacthandles/82mappedELFfiles;terminal audit above supersedes live-state notes.
Monitor complete aligned censuses, net whole-field q/J/geometry removal and distinct
geometry/preparation/maintenance/trial work counters. Timers overlap; do not add
nested scopes or call collective-call time pure idle. CPUtime includes MPIspin.

## One active optimization/validation cycle — existing-envelope analysis

[Numeric certificate retention](Handbook/ADAPTATION_NUMERIC_MEMO_20261031.md) follows
190terminal time-budget failure. Existing exact FaceMemo now uses bounded cost-aware
incremental clock eviction, same fixed4MiB/rank capacity, stable keys/value returns,
explicit metadata and maintenance accounting. No duplicate cache/options/scheduler/
construction changes. Preserve190ownership/history, oldest fairness/worst-first,
rejected-star invalidation, full-vertex-disjoint reservations, fresh curved contexts,
versioned dependencies and complete publication. Union maintenance remains mandatory
cumulative work and fails closed; direct API has no AtlasBudget, counts maintenance.
R/numeric_memo191_source_review.json documents primary/read-only routine review.

Latest user steering: AFTER frozen attempt, analyze cheaper geometry certification
within this SAME cycle. Distinguish exact repeats from changed trial positions;
review existing unchanged-face reuse, changed-face-only checks, early bounds and
inexpensive screening BEFORE adding code. Investigate tolerance-slack motion bounds
only with proven certificate meaning/direction, unchanged authority/support, exact
association/dependency invalidation, outward rounding and resource accounting.
Unknown/insufficient bounds fall back to full certification; sampling is insufficient.
Conservative envelopes only if existing facilities are insufficient. No speedup
established, no new options/competing implementation; unproven alternatives in backlog.
Choose further work from measured traversal savings and net bad-cell removal/s.

## Preserved previous outcome and fair comparison

190firstevent naturally time-budget rejected ALL6ranks:Native total max5407.486471s,
quality4097.689184s. Complete EXIT765728cells,30754q/28157J/3702geometry,size0;
15194publications,23317netq/25220netJ removed. Queue286188,blocked153,stuck0,missing0.
Next target SPEED; active repairs and positive net yield remained. Upper4.059Bwork,
traversal3.999B; support/preparation much smaller. Private boundary112528upperqueries/
3095.541ranksec vsinterior0queries/107.085s. Nested times not additive. Source
dcf6f01623 qualified serial/MPI6; all18construction generations EXACT188 counts/work.
Five complete-census ownership migrations preserved counts/debt/cache/flags/history.
ALL7terminal diagnostic groups primary reviewed in PREVIOUS/terminal_assessment.json;
service details terminal_geometry_service_assessment.json. Churn uses sorted exact-
coordinate tet keys: moved cells retire/introduce even with unchanged connectivity.

Natural failure preserved BEFORE SIGTERM only verified remaining launcher145713;
independent1793473407audit ALL10ABSENT/all1401pins unchanged BEFORE191source edits.
PREVIOUS/event1_natural_failure.json,event1_failure_owned_stop.json,terminal_handles.json.
Those IDs are HISTORICAL,never resignal. 188and183terminal audits/failures likewise
preserved in their case folders. 183genuine-feature preflight fixed before188 and
held through complete188/190. Preserve that construction/publication improvement.

PREVIOUS/work_comparison_terminal.json,.csv,.svg compares actual complete snapshots:
near q/Jstate19030754/28157/3702geometry vs18830961/28318/3668geometry; cavities
15194vs15157,traversal3.999Bvs3.567B(+12.1%),wall4097.689vs2532.324s. Same quality
entry/cache/input; trajectories differ and measured host contention confounds timing.
No interpolation/arbitrary combined work score. Earlier window snapshots historical.
188terminal:17190q/14369J/3526geometry,size0,quality4452.518s,28742commits;
rank4held71.56%debt,spread22.314. 190repeat census redistribution addresses that.
179real-CFD last complete1479.020quality-second census43764q/42274J/4067geometry;
its terminal census was INCOMPLETE, not zero badcells. Old startup runs use different
fields and are not fair saved-CFD baselines. Detailed history in linked designs.

## Saved input, acceptance still required and open limits

Readonly saved real CFD: R/m6_inputs/euler_original_force_seed_v1. Original61330points/
303359cells; Mach.8395,AOA3.06. User accepts reasonable stable forces:CL.26953535374,
CD.01353585654,last200ranges1.114e-7/3.121e-8,finite/rho/IEpositive/exact grid order.
Not a residual-converged aerodynamic reference or adapted restart; no assistant
residual threshold. SeedSHA1eaee788bd62bd92a92ec539577dcbcfc0f8042dcbcec23adb04e825bb01a880.
Continued original-grid restart SHA6bbed8da75c40eca7364cc9d3fe96f13311fad69706e013acb68ba8feaf4240a
BYTEIDENTICAL179/183/188/190/191; LIVE/early_runtime_provenance.json. Initial partition
and all18selected estimated-demand fields EXACT188/190; initial_partition_comparison.json.
Reuse seed each first remesh, distinct SOLUTION_FILENAME/RESTART_FILENAME. No new
freestream solve. Saved12query payload10448B/SHA37ea9781db1699c3b523bdeb94887a9f097931aa9c5378ba034fbab9657fdb0b
is older168projected-star input, not current M6 mesh;191cached/recomputed replay passed.

Still require actual shape/height closure → residual geometry/size closure → shape
reclosure under SAME cumulative5400s/work/memory limits. TWOstrict accepted mesh/
native_ref pairs,actual conservative admissible transfer(double residual1e-10,
targetintegral1e-12),finite continued CFD,rho/internalenergy>0. Then isolatedCOPYfinal
adaptedmesh/native_ref+matchingflow; actual RESTART_SOLYES/ADAP_LOOPNO/correct
SOLUTION_FILENAME/distinct outputs, coordinate/order/state audit. Seed00000 never
proves adapted restart. Then dedicated BL curved/junction/interface/subsequent-
operation/redistribution/requested-height MPI qualification and RANS500k.

Open numeric risk: final q/J/edge gates use current transported nodal tensors;
16GLOBALfixed samples show~8.4%max drift, not independently immutable original
continuous-CFD-target certification. Empty metric identity is a cache tag. General
adaptation resume cycle0/path collision and rejected-cycle consumption remain open;
isolated solver restart does not prove general adaptation resume fixed. Broad182MPI
timed out; serial_full170 was NOT passing. BL/RANS qualification not started.

## Operating links and discipline

[AGENTS](AGENTS.md) · [complete task](Handbook/ADAPTATION_INTEGRATED_NEXT_TASK.md) ·
[census ownership](Handbook/ADAPTATION_CENSUS_OWNERSHIP_20261031.md) ·
[feature fix](Handbook/ADAPTATION_FEATURE_PUBLICATION_20261031.md) ·
[parallel design](Handbook/ADAPTATION_PARALLEL_REPAIR_20261031.md) ·
[saved CFD](Handbook/ADAPTATION_M6_CFD_SEED_20261031.md) ·
[geometry/smoothing](Handbook/ADAPTATION_GEOMETRY_QUERY_REUSE_20261031.md) ·
[cache/ownership](Handbook/ADAPTATION_CACHE_PHASE_20261030.md) ·
[findings](Handbook/ADAPTATION_FINDINGS_ASSESSMENT_20261031.md) ·
[backlog](Handbook/ADAPTATION_BACKLOG.md).

One owned heavy job,nice19,libs1,build2–4workers(helper3),MPI6bounded; TMPDIR4TB
compile-tmp,not root/tmp. Check actual host RAM/load; sampled summedRSS is not physical
memory/job peak. Preserve resource/pin/handle receipts; leave unrelated processes alone.
Fork pushes authorized:https://github.com/rois1995/SU2.git branch codex/native-3d-bl.
Git metadata outside writable root:escalated git -c gc.auto=0 -c submodule.recurse=false;
diff/status --ignore-submodules=all. Whitelist adds; no binaries/meshes/restarts/rawperf;
leave unrelated155logs/.orig/submodules/untracked steering/history files untouched.
Primary owns integrated design/source/acceptance; bounded routine agents done/no jobs.
