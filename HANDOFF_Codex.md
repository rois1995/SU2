# Native adaptation — current handoff

Updated 2026-10-31. Full goal ACTIVE: TWO strictly accepted ONERA M6 Euler
CFD → mesh → CFD → mesh events at50k metric complexity, conservative admissible
actual transfer, finite continued CFD and isolated usable adapted disk restart.
Then dedicated BL/constraint/MPI qualification and corresponding RANS500k workflow.
Report actual points/cells separately from metric complexity. Geometry1e-4,q>=.2,
J>=.05,maxedge1.8,requested heights remain strict. Construction retains exact
positive/nondegenerate/topology/body/feature/height validity and omits q/J.
CAD remains optional. No accepted M6 mesh/transfer/adapted restart yet.

All staged57 paths are under `integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/`.
CURRENT=`staged57/m6_runs/euler_build188_real_cfd50k_feature_safe`.
BEFORE=`staged57/m6_runs/euler_build179_real_cfd50k_before`.
PREVIOUS=`staged57/m6_runs/euler_build183_real_cfd50k_after`.

## Authoritative state —190 full M6 LIVE; source freeze ACTIVE

ONE owned heavy job: full TWO-event Euler50k build190 M6, exec3143.
LIVE=`staged57/m6_runs/euler_build190_real_cfd50k_census_ownership`.
helper145534/start88285053,time145711/start88285813,timeout145712 and launcher145713/
start88285814,ranks145728–145733/start88285824. MPI ranks nice19, libraries1.
All source/header/test/helper/binary/runtime/config/input pins FROZEN until exact
owned handles terminal and independently audited. Same saved real-CFD seed; native
budget5400s unchanged. Do not launch another heavy job or modify compiled sources.
Build190 PASS307.081s,1296pins unchanged/all2handles absent; substantial host I/O
contention while linking. Focused serial PASS28cases/8100assertions; MPI6 PASS163cases
on EACH rank, all10handles absent/pins unchanged before M6. Source dcf6f01623 pushed;
staged57/census_ownership190_qualification.json records actual binary hash/results.
Construction CLOSED:727722cells,size0,geometry4059; ALL18generations EXACT188
counts/query work. Complete quality entry1241.413606native seconds:54071q/53377J/
4059geometry,size0,debt25236.78740274; all15selected per-rank fields EXACT188.
Latest complete window5 at2473.951533quality seconds:750209cells,40488q/38625J/
3826geometry,size0,debt9565.588440. Interval995.093620s:2374netq/2542netJ/
49geometry removed,1898commits,q2.38571/s,J2.55453/s,q1.25079/commit.
Cumulativeq5.49041/s,commits3.24097/s. Actual188nearby2532.324077s(delta-58.373s,
2.36%):30961q/28318J/3668geometry,netq9.12600/s. Observed190wall progress BEHIND;
NO isolated code-regression claim: host contention is directly measured.
Latest user asks whether machine contention explains slowdown: YES plausible major
contributor. Fresh1793472025receipt:8CPUs,load17.13,CPU PSI some avg60=39.06%,
I/O some15.53%,memorypressure0,~29GiBavailable. PSI is not own-rank CPU loss share.
LIVE/host_contention_receipts.jsonl. Window5geometrywork FELL40% vswindow4 while
wall ROSE56%; collective-call seconds392–420/rank include wait,not protocol proof.
Outer update126.56–130.59s (~13%window), not the entire slow interval. Maxprivate
rank4 354.801s,165netq published;rank1 246.797s,260netq. Shape-stuck0,missing0;
76.13%q-bad interior. LIVE/quality_census11_diagnosis.json records actual directional
changes (not hardcoded previous-window cause). Geometry510Mwork/44495uppercalls.
THREElate migrations committed rounds21632/21696/21824; global counts exact,
debt conserved within1.82e-12,all flags complete,missing/privatecollectives/churn0.
Latestcost34.448s incl observations,spread2.01595→1.25108; prepared cache entries/
bytes/querywork and16GLOBALmetric samples preserved. No acceptedmesh or closure.
Evaluation21760skipped existingimbalance1.20847<1.25 guard. Next full window pending.
LIVE/matched_quality_time_comparison.json is latest; independent window4/window5
comparison files preserve past measured pairs.179has no close complete census for
latesttime; earlier window4 had actual1479.020155s match. Never extrapolate179.
LIVE/quality_service_assessment.json includes ALL ownership stage costs in windows;
all_quality_census_assessment.json has full location/feature/miss histograms,ladder,
churn,drift. quality_migration_assessment.json and first_postmigration_comparison.json
preserve comparisons. Early detailed evidence moved to linked connected design.
At midrun1793471309all1401pins unchanged/all10handles match/all82mapped files
pinned:LIVE/midrun_provenance.json (earlier quality_entry_provenance.json retained). Passive host receipt1793470400:8CPUs,
load18.26,CPUpressure avg60~39.9%,~29GiBavailable RAM; clear CPU contention, no
memory-pressure stop. LIVE/quality_resource_receipt.json. Source freeze ACTIVE.
Build189 historical MPI failed two modified fixtures;161others/rank passed. Preserved
terminal receipts before corrections. Corrected mesh-valid vs measurement-complete
hook and restored exact inherited repair subsets without a phase change; corrected
geometry-mode fixture scope. Both agents done/no independent jobs.

HISTORICAL full TWO-event build188 TERMINAL, exec16914exit1. First event naturally rejected
`Native3D private repair round exhausts elapsed-time budget.` All6stop=time_budget;
native total max5403.529847s,refine5345.821634s. Accepted original CFD retained.
Preserved raw6summaries/details/EXIT/root rejection BEFORE verified launcher4103464
SIGTERM during second-event setup. Independent audit1793464454:ALL10owned handles
ABSENT and ALL1401pins unchanged BEFORE ANY next compiled source edit.
CURRENT/event1_natural_failure.json,event1_failure_owned_stop.json,terminal_handles.json.
Those188handles are historical,never infer live work from old status. Freeze was lifted
for the demonstrated fix; current190 M6 freeze above is ACTIVE.

188production sourceb64674701a,qualification34f458f5ed; CFD SHA
191ccb8869b3320de35a56ed91f608790897ebe7626741841e3edaf077eb4ca1. Qualified serial+
MPI6 scope in staged57/feature_publication188_qualification.json; no whole-suite claim.
Genuine-feature preflight held through this complete attempt: no publication failure.
Private-slot collectives0/unmeasuredchurn0. Existing cache/parallel/debt improvements
preserved. Same real-CFD seed AND initial continued field BYTEIDENTICAL179/183/188;
ALL18construction-generation totals/query work and quality entry EXACT183.
Construction727722cells,size0,geometry4059 at855.998s; quality entry892.080s:
54071q/53377J/4059geometry,size0,debt25236.787403. Timing depends on host load.

Complete terminal EXIT at4452.518quality seconds:786068cells,
17190q-bad/14369J-bad/3526geometry-bad,size0,debt1344.661704;28742commits
(6.455/s),36881netq-bad removed(8.283/s). ALLmeasurement/census/cache/shape/geometry
flags complete,not an accepted mesh. Rankdebt43.122/63.967/136.232/63.839/962.227/
75.274;spread22.314 vsentry1.456,rank4holds71.56%. Only6stuck badcells;
queue199145/blockedfeatures324. All16GLOBALdriftpoints supported/lost0,max.0836193713,
currentnodal full_target1 (not immutable original field). Sampled maxTREE RSS
11597664256bytes(~10.8GiB),not physical/job peak. No actual memory-pressure stop;
globalwork3188943 below19414976budget. CURRENT/terminal_assessment.json has ALL7
passive diagnostic categories, actual raw per-rank outcome, ladder/reasons/location/
shortedges/geometrytypes/histograms/metric drift and timing/budget receipts.

Closest179matchingtime comparison:1881473.820s vs1791479.020s(-5.2s):
38969q/36891J/3861geometry vs43764/42274/4067,size0both;commits/s6.156vs2.771,
netqbadremoved/s10.247vs6.460,debtspread3.330vs7.481. No interpolatedcounts or
isolated-speed claim. BEFORE terminal census was incomplete; do not compare its
last-window counts as if terminal. CURRENT/matched_before_quality_comparison.json.

## Active next implementation/validation cycle

Next target:SPEED, repeat complete-census quality ownership with preserved search
state. Source confirms quality ownership is entry-only despite late debt skew.
Cached cell/edge certificates already migrate, but feature ages/blocked state and
specialized phase modes must survive; blindly repeating migration would reset them.
[Connected design/acceptance](Handbook/ADAPTATION_CENSUS_OWNERSHIP_20261031.md).
Connected correction dcf6f01623 qualified in190 serial/MPI6; full M6 now LIVE.
Fixed14feature age/active/blocked masks migrate with CachedRow; exact required
membership/range checked, oldest=min,blocked=all prior contributors. Existing
user/severity/oldest queues reconstructed from inherited subsets; specialized modes
retained. Periodic hook permits bad meshes only after complete measurement census.
No new scheduler/options or construction changes. Preserve current freeze; assess
actual NET whole-field q/J-bad removal/s and geometry counts, not commits/debt
alone. Latest user assessment requirements are in the connected design. Compare
churn/net yield per commit, capacity/reservations, imbalance/synchronization, geometry
cost/trial and surface/interior population/private smoothing efficiency separately.
No competing implementation or extra profiling. New ownership_committed stage
is a complete census and splits diagnostic windows: interval totals must include
all complete quality stages, not only window rows. Migration count changes are not
repair churn. Qualification:staged57/census_ownership190_qualification.json.

Existing counters show upper queries dominated by traversal, not facet rebuilding;
CURRENT/quality_service_geometry_assessment.json,geometry_guidance_source_review.md.
Numeric FaceMemo exact-key reuse ALREADYexists; its flushall effect is UNMEASURED/
backlog,not this implementation. Upper/traversal/smoothing/optimizer/busy/join scopes
overlap; never add or infer pure idle. Saved12-query payload and authority hashes
verified in reusable_geometry_query_inventory.json; older inputs,NOTexact188state.

Qualified focused serial and MPI6 (age/blocked/mode/census/weights/empty ranks/curved/
heights/resource/cache invariants); now full TWO-event M6 from SAMEsavedCFD,
50k/5400s unchanged. Shape/height then residual
geometry/size repair with shape reclosure under same cumulative limits. Require TWO
strict accepted mesh/native_ref pairs, conservative admissible actual transfer
(double residual1e-10,targetintegral1e-12),finite CFD,rho/internalenergy>0; then
isolated actual adapted disk CFD restart (copy mesh/native_ref+matchingflow,
RESTART_SOLYES/ADAP_LOOPNO/correct SOLUTION_FILENAME/distinct outputs). Seed00000
never proves adapted restart. Then dedicated BL/constraint/MPI and RANS500k.

Terminal evidence/design pushed5dbfea1bfe; connected source/test checkpoint3f791fe715
pushed explicitly unvalidated before build189; dcf6f01623 corrections qualified190
serial/MPI6 and full M6 now LIVE. No acceptedmesh/
transfer/restart.

## Reusable input, history and open limits

Readonly real-CFD seed:staged57/m6_inputs/euler_original_force_seed_v1. User accepts
reasonable stable CL/CD:CL.26953535374,CD.01353585654;200iteration ranges1.114e-7/
3.121e-8,allfinite/rho/IEpositive/exact original-grid order+coordinates. Reuse every
first remesh; never reimpose assistant residual threshold or repeat freestream solve.
Not a residual-converged aerodynamic reference or adapted restart. Seed SHA
1eaee788bd62bd92a92ec539577dcbcfc0f8042dcbcec23adb04e825bb01a880; continued00000SHA
6bbed8da75c40eca7364cc9d3fe96f13311fad69706e013acb68ba8feaf4240a matches179/183/188.

183 historical terminal:genuine-feature publication failure before5400s, original
CFD retained. Raw6rank failure/stop/all10absent/all1401pins intact BEFORE edits in
PREVIOUS/event1_natural_failure.json,event1_failure_owned_stop.json,terminal_handles.json.
179real-CFD historical terminal:natural time-budget rejection5409.755s; rawfailure
preserved before owned stop, allhandles absent/allpins intact before edits. Its last
complete1479.020quality-second window43764q/42274J/4067geometry,size0,4098commits.
Old startup175/179 use different fields, not matched real-CFD baselines.

Existing numerical risk: current-nodal P1 transport drifts despite same cache field
identity. Final production q/J/edge gates use transported tensors, without independent
original continuous-target recertification. Empty identity is a cache tag. PREVIOUS/
metric_authority_source_review.md has primary-reviewed source paths. Do not claim
immutable original-target certification. General adaptation resume cycle0path collision
and rejected-cycle consumption remain open; isolated restart does not prove general
adaptation resume fixed. Broad182MPI timed out; serial_full170 was NOT passing.
Dedicated BL/RANS qualification not started. Full goal remains incomplete.

## Operating links

[AGENTS](AGENTS.md) · [complete task](Handbook/ADAPTATION_INTEGRATED_NEXT_TASK.md) ·
[census ownership](Handbook/ADAPTATION_CENSUS_OWNERSHIP_20261031.md) ·
[feature fix](Handbook/ADAPTATION_FEATURE_PUBLICATION_20261031.md) ·
[parallel design](Handbook/ADAPTATION_PARALLEL_REPAIR_20261031.md) ·
[saved CFD](Handbook/ADAPTATION_M6_CFD_SEED_20261031.md) ·
[geometry/smoothing](Handbook/ADAPTATION_GEOMETRY_QUERY_REUSE_20261031.md) ·
[cache/ownership](Handbook/ADAPTATION_CACHE_PHASE_20261030.md) ·
[findings](Handbook/ADAPTATION_FINDINGS_ASSESSMENT_20261031.md) ·
[backlog](Handbook/ADAPTATION_BACKLOG.md) ·
[preserved detailed prior handoff](Handbook/History/HANDOFF_Codex_before_build188_quality8_20261031.md).

One owned heavy job,nice19,libs1,build2–4workers(helper3),MPI6bounded; TMPDIR4TB
compile-tmp,not nearly-full root/tmp. Check actual RAM/load, leave others untouched.
Fork pushes authorized:https://github.com/rois1995/SU2.git branch codex/native-3d-bl.
Git metadata outside writable root:escalated git -c gc.auto=0 -c submodule.recurse=false;
diff/status --ignore-submodules=all (broken unrelated Eigen worktree). Whitelist adds;
no binaries/meshes/restarts/rawperf; preserve unrelated155logs/.orig/submodules/
untracked steering/history files. Primary retains integrated acceptance.
