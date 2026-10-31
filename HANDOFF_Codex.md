# Native adaptation — current handoff

Updated 2026-10-31. Full goal ACTIVE: TWO strictly accepted ONERA M6 Euler
CFD → mesh → CFD → mesh events at50k metric complexity, conservative admissible
actual transfer, finite continued CFD and isolated usable adapted disk restart.
Then dedicated BL/constraint/MPI qualification and corresponding RANS500k workflow.
Actual points/cells are separate from metric complexity. Geometry1e-4,q>=.2,J>=.05,
maxedge1.8,requested heights stay strict. Construction preserves exact positive/
nondegenerate/topology/body/feature/height validity and deliberately omits q/J.
CAD optional; no-CAD geometry authority sufficient. No accepted adapted M6 mesh yet.

## Authoritative live state —191 FULL TWO-EVENT M6; sources FROZEN

R=`integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57`.
LIVE=`R/m6_runs/euler_build191_real_cfd50k_numeric_memo`.
BASELINE=`R/m6_runs/euler_build188_real_cfd50k_feature_safe`.
PREVIOUS=`R/m6_runs/euler_build190_real_cfd50k_census_ownership`.
One owned heavy job:exec36816. Helper401117/start89158697,time401313/start89159335,
timeout401314 and launcher401315/start89159336; ranks401320..401324/start89159345,
rank401325/start89159346. LIVE/owned_startup.json records exact host identities.
All10handles matched1793477253; source/runtime/input all1401pins unchanged at
1793476745; all82mappedELFfiles pinned at startup. MPI non-ELF shared-memory files
are transient state, separately recorded. Initial all-mapping false-positive audit
is preserved locally, corrected by executable/library classification; no pin failure.
Build/header/test/helper/binary/config/input remain FROZEN until verified terminal.
Let run finish unless preserved failure/bottleneck/resource evidence justifies the
authorized exact-owned stop/fix/retry workflow. Forecasts or stale logs do not justify
stopping. Current process/session identities, not historical IDs/status, control.

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
Last audited window5 at1437.660574quality seconds:750209cells,40488q/38625J/
3826geometry,size0;8018publications,13583netq/14752netJ/233geometryremoved since entry.
Outcome and publications EXACT190 same round21824; traversal2285274456vs2347388609
(-2.64610%). quality_window5_comparison.json. Latest interval333.720535s:
2374netq/2542netJ/49geometry,1898commits,q7.11374/s,J7.61715/s;
join-callshare.57760 includes protocol/wait,not pure idle. Cumulativeq9.44799/s vs
1905.49041/s,not isolated timing. Before third migration debtspread2.01595,after1.25108;
queue323635,blocked134,stuck0/missing0/privatecollectives0. Geometry gates unmet.
Prior window4 (quality1103.940039s)42862q/41167J/3875geometry,6120publications matched
190 with2.53571%less traversal. Window4 vs3 actual traversal+63.0% despite publications
-3.9%; traversal/publication+69.7%,upperqueries/publication+33.9%,work/query+26.7%,
netq/publication-29.3%. Contention alone cannot explain those counted work/yield
changes; no next code selected before terminal. quality_window4_work_diagnosis.json
retains private boundary/interior/rung/rejection/location detail and provenance.
Three late redistributions committed21632/21696/21824,globalcounts/debt preserved
within1.82e-12,ALLpreparedcache and numericmemo counters/entries/bytes unchanged,
drift samples unchanged. Latest cost14.386965s incl observations belongs in NEXT
window interval. Evaluation21760skipped. ALL14complete censuses audited;84rankcache
snapshots within fixed numeric/prepared allowances. At1793479251ALL1401pins unchanged/
10exacthandles live/82mappedELFfiles pinned:midrun_quality4_provenance.json.
No source edits,source freeze active. Historical window2/3 comparisons retained.
Current outcome pending; no actual transfer/adapted restart. Source remains FROZEN.
Monitor complete aligned censuses, net whole-field q/J/geometry removal and distinct
geometry/preparation/maintenance/trial work counters. Timers overlap; do not add
nested scopes or call collective-call time pure idle. CPUtime includes MPIspin.

## One active implementation/validation cycle

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
