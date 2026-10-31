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
BASELINE=`staged57/m6_runs/euler_build188_real_cfd50k_feature_safe`.
BEFORE=`staged57/m6_runs/euler_build179_real_cfd50k_before`.
PREVIOUS=`staged57/m6_runs/euler_build183_real_cfd50k_after`.

## Authoritative state —190 TERMINAL; no heavy job; freeze lifted

No owned heavy job. CURRENT=`staged57/m6_runs/euler_build190_real_cfd50k_census_ownership`.
Exec3143TERMINAL exit1 after authorized stop of remaining attempt. FIRSTevent
NATURALLY rejected on ALL6ranks:time_budget,root`Native3D private repair round
exhausts elapsed-time budget.` Native total max5407.486471s,refine5341.348765s.
Raw summaries/details/EXIT/root rejection saved BEFORE exact verified launcher145713
SIGTERM. Independent1793473407audit:ALL10handles ABSENT and ALL1401pins unchanged
BEFORE ANY next compiled source edit. CURRENT/event1_natural_failure.json,
event1_failure_owned_stop.json,terminal_handles.json. Those handles are historical;
never resignal/restart from old status. Freeze LIFTED for demonstrated next fix.
Complete terminal EXIT:765728cells,30754q/28157J/3702geometry,size0,debt4418.429452;
shape/cache/census/geometry flags complete. Queue286188,blockedfeatures153,
stuck_shape0/stuck_required0/missing0. No accepted mesh/actual transfer/adaptedrestart.
Original accepted CFD retained. Next target:SPEED; active search and positive net
repair yield remain, time budget exhausted. Geometry-query/smoothing cost path is
next review, preserving190ownership/history and all18construction improvements.
Terminal ALL7counter assessment is complete and primary reviewed. The one active
follow-up is numeric certificate retention; no new scheduler/options/limits or
construction changes.
Build190 PASS307.081s,1296pins unchanged/all2handles absent; substantial host I/O
contention while linking. Focused serial PASS28cases/8100assertions; MPI6 PASS163cases
on EACH rank, all10handles absent/pins unchanged before M6. Source dcf6f01623 pushed;
staged57/census_ownership190_qualification.json records actual binary hash/results.
Construction CLOSED:727722cells,size0,geometry4059; ALL18generations EXACT188
counts/query work. Complete quality entry1241.413606native seconds:54071q/53377J/
4059geometry,size0,debt25236.78740274; all15selected per-rank fields EXACT188.
Complete quality EXIT at4097.689184quality seconds:30754q/28157J/3702geometry,
15194published cavities,23317netq/25220netJ removed (5.69028q/s,6.15469J/s).
Five late ownership migrations preserved counts,debt,flags,caches and repair history.
Terminal ALL7groups:CURRENT/terminal_assessment.json; geometry/service breakdown:
CURRENT/terminal_geometry_service_assessment.json. Traversal3.999Bwork dominates
upper4.059B; nested timers overlap. No exhausted repair power or RAM-pressure stop.
Fair comparison under contention:CURRENT/work_comparison_terminal.json,.csv,.svg
contains complete actual snapshots through EXIT for188and190. Same saved CFD,
quality entry and starting prepared-cache state verified. At nearest actual q/J state,
19030754/28157/3702geometry vs18830961/28318/3668geometry; published cavities
15194vs15157, traversal3.999Bvs3.567B (+12.1%), elapsed4097.689vs2532.324s.
Timing is confounded by measured host contention; extra counted work is real.
No interpolation/arbitrary combined work score; CPUtime includes MPIspin.
Earlier window6/window7 work comparisons retained as immutable historical snapshots.
CURRENT/quality_service_assessment.json includes ALL ownership stage costs in windows;
all_quality_census_assessment.json has full location/feature/miss histograms,ladder,
churn,drift. quality_migration_assessment.json and first_postmigration_comparison.json
preserve comparisons. Early detailed evidence moved to linked connected design.
At midrun1793471309all1401pins unchanged/all10handles match/all82mapped files
pinned:CURRENT/midrun_provenance.json (earlier quality_entry_provenance.json retained). Passive host receipt1793470400:8CPUs,
load18.26,CPUpressure avg60~39.9%,~29GiBavailable RAM; clear CPU contention, no
memory-pressure stop. CURRENT/quality_resource_receipt.json. Source freeze lifted after terminal audit.
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
BASELINE/event1_natural_failure.json,event1_failure_owned_stop.json,terminal_handles.json.
Those188handles are historical,never infer live work from old status. Freeze was lifted
for the demonstrated fix; 190 is terminal and its freeze is lifted after the independent audit.

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
globalwork3188943 below19414976budget. BASELINE/terminal_assessment.json has ALL7
passive diagnostic categories, actual raw per-rank outcome, ladder/reasons/location/
shortedges/geometrytypes/histograms/metric drift and timing/budget receipts.

Closest179matchingtime comparison:1881473.820s vs1791479.020s(-5.2s):
38969q/36891J/3861geometry vs43764/42274/4067,size0both;commits/s6.156vs2.771,
netqbadremoved/s10.247vs6.460,debtspread3.330vs7.481. No interpolatedcounts or
isolated-speed claim. BEFORE terminal census was incomplete; do not compare its
last-window counts as if terminal. BASELINE/matched_before_quality_comparison.json.

## Active next implementation/validation cycle

190complete-census redistribution cycle is terminal, not the next implementation.
Preserve qualified dcf6f01623: fixed14feature ages/active/blocked state, exact inherited
feature subsets, specialized modes, oldest fairness/worst-first and dependency
invalidation. Repeated ownership executed5times at complete censuses; geometry caches
and numeric data survived. Qualification:staged57/census_ownership190_qualification.json.
[190design/history](Handbook/ADAPTATION_CENSUS_OWNERSHIP_20261031.md).
Next target:SPEED of useful quality closure; do not revert construction or relax gates.
Terminal ALL7diagnostics:CURRENT/terminal_assessment.json,primary reviewed counts,
histogram quantities,rejection labels and all7groups.
One next connected design:[numeric certificate retention](Handbook/ADAPTATION_NUMERIC_MEMO_20261031.md).
Replace flushall by existing bounded cost-aware clock,exact keys/fixed capacity;
add memo counters and mixed-retention validation. Live flush impact remains unmeasured.
No new scheduler/options/batches/passes/limits or construction changes.
All owned handles are terminal; source freeze lifted only after independent audit.
C++ and focused mixed-retention test are written, UNBUILT/UNVALIDATED. Build191
reserved next; staged57/numeric_memo191_validation_plan.json records scope. All
previous handles independently rechecked absent; prebuild host receipt preserved.
Primary/read-only routine review completed; staged57/numeric_memo191_source_review.json.
Union maintenance work remains mandatory and fail-closed; no independent build/run.
Keep all complete quality stages in counter intervals, including ownership stages;
local count migration is not repair progress. CPU contention remains a qualification
on timing comparisons; separate counted work/near-quality-state evidence is preserved.

Existing counters show upper queries dominated by traversal, not facet rebuilding;
BASELINE/quality_service_geometry_assessment.json,geometry_guidance_source_review.md.
Numeric FaceMemo exact-key reuse ALREADYexists; its flushall effect remains
UNMEASURED. This cycle changes its retention and adds counters to measure benefit. Upper/traversal/smoothing/optimizer/busy/join scopes
overlap; never add or infer pure idle. Saved12-query payload and authority hashes
verified in reusable_geometry_query_inventory.json; older inputs,NOTexact188state.

Qualified focused serial and MPI6 (age/blocked/mode/census/weights/empty ranks/curved/
heights/resource/cache invariants);190full TWO-event M6 rejected firstevent.
After the next connected qualification, immediately retry full TWO-event SAMEsavedCFD,
50k/5400s unchanged. Shape/height then residual
geometry/size repair with shape reclosure under same cumulative limits. Require TWO
strict accepted mesh/native_ref pairs, conservative admissible actual transfer
(double residual1e-10,targetintegral1e-12),finite CFD,rho/internalenergy>0; then
isolated actual adapted disk CFD restart (copy mesh/native_ref+matchingflow,
RESTART_SOLYES/ADAP_LOOPNO/correct SOLUTION_FILENAME/distinct outputs). Seed00000
never proves adapted restart. Then dedicated BL/constraint/MPI and RANS500k.

Terminal evidence/design pushed5dbfea1bfe; connected source/test checkpoint3f791fe715
pushed explicitly unvalidated before build189; dcf6f01623 corrections qualified190
serial/MPI6 and full M6 now terminal; no accepted mesh. No acceptedmesh/
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
