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

## Live authority —188full M6, source freeze ACTIVE

ONE owned heavy job: full TWO-event build188, exec16914. Helper4103308/start87384875;
time4103462/timeout4103463/launcher4103464/start87385499;
ranks4103468–73/start87385504. ALL10identities match. ALL1401source/header/test/
helper/binary/config/input pins unchanged; all82mapped runtime files pinned/matching
(latest independent provenance audit1793461513). Keep frozen until verified terminal
or preserved actual failure/bottleneck/resource pressure justifies authorized stop.
An observation timeout is not termination. No extra perf/GDB/baseline campaign,
gate relaxation or native budget increase. Nice19,libs1,MPI6,50k/5400s per event.
CFD SHA191ccb8869b3320de35a56ed91f608790897ebe7626741841e3edaf077eb4ca1.

Connected source checkpointb64674701a; qualified34f458f5ed. Current188 qualified
CheckReplacement locally reuses noncommitting Prepare on complete old/fresh boundary
deltas against replicated accepted state. Quality guards/final private backstop
refuse invalid proposals before one atomic publication, retaining valid peers.
Exact unchanged accepted-face contracts skip delta rebuilding. No per-slot MPI;
final combined clearance/reservation/version/parameter checks remain. Construction
unchanged. [Feature design/evidence](Handbook/ADAPTATION_FEATURE_PUBLICATION_20261031.md).

Qualification: build188PASS54.151s/all1296pins unchanged/exact handles absent.
Affected187serialPASS21cases/6148assertions. Affected187MPI163cases/rank completed;
only newfixture's local-directory-change assertion failed on unaffected rank3.
All other assertions passed. ONLYtestpin187→188 changed to global-change assertion;
focused188serialPASS2cases/71assertions and MPI6bothcases/all6ranks PASS, pins intact,
all owned handles terminal before M6. Source/runtime scope and reuse are explicit in
staged57/feature_publication188_qualification.json. No full-repository-suite claim.
Legacy nonlinear PSLG extension refused before preflight; original legacy case
retained, nonlinear entity case validates M6 path. Failed fixtures preserved.

## Actual current attempt

Seed AND initial continued original-grid CFD BYTEIDENTICAL179/183. Input61330points/
303359cells; initial ownership matches183 across all17compared fields. Coarsening
EXACT183:292709cells,2627collapses,size55180,geometry3512,14870uppercalls/work303014856.
ALL18construction-generation measured totals EXACT183, including query work/calls.
Construction size closure at855.998native seconds:727722cells,size0,geometry4059.
Quality entry892.080s (1831500.747s):54071q/53377J/4059geometry,size0,debt25236.787403.
Construction speed differs with host contention, not quality-only publication fix.
Ownership410245cells/11.124s, allcounts/flags/debtweights preserved; spread15.800→1.456.
First3quality windows EXACT183counts/debt/commits. Bywindow3 newpreflight adds4.2%
quality atlaswork; upper/support/facet/certification work unchanged. Timers overlap.

Actual prior failure barrier PASSED: window4/round21760/5837commits beyond183fatal
round21746/5421commits, no rejection. CURRENT/feature_publication_barrier_assessment.json.
Latest COMPLETE window8/round22016 at1932.789quality seconds:757326cells,
34759q-bad/32465J-bad/3786geometry-bad,size0,debt6558.646277;12317commits(6.373/s),
19312netq-bad removed(9.992/s). No acceptance. All complete census/cache/shape/geometry
flags intact. Window8commits1593,qchurn17603removed/15438created,J16165/13871;
privatecollectives0/unmeasuredchurn0/stuckshape0,queue309101/blockedfeatures158.
Join-callshares[.425,.713,.434,.544,.537,.575] include protocol+wait,notpureidle.
All16GLOBALdriftpoints supported/lost0,max.084197787 (replicated views,not96samples).
Passive reporting audit checks location/features/histograms against wholefield;
ladder totals agree with actual net q/J/debt removals through ALL8windows (roundoff).
CURRENT/quality_window*_counters_assessment.json,passive_census_integrity_assessment.json.

Closest actual179comparison:1881473.820s vs1791479.020s(-5.2s,0.35%):
38969q/36891J/3861geometry vs43764/42274/4067,size0both. Commits/s6.156vs2.771;
netqbadremoved/s10.247vs6.460;debtspread3.330vs7.481. Earlier673vs699s pair retained.
Hostload/cache/ownership/entrycounts differ; no isolated speed/exact clock claim,
no interpolated counts. CURRENT/matched_before_quality_comparison.json.
Latest passive1793461889:ALL10identities match,~29GiBavailable,treeRSS~8.65GiB
(sampled sum,not physical/job peak),load8.77,no rejection. Source freeze ACTIVE.
Latest pushed7de0640d0a includes quality6/matched1479s comparison. Quality7/8 and
integrity audit are local completed evidence, next checkpoint. Both agents done/no jobs.

## Continue the complete workflow

1. Observe the LIVE attempt; pinned sources/helpers/binaries/config/inputs unchanged.
   No restart from elapsed time/stale logs. Preserve complete all-rank censuses.
2. On actual failure, preserve first event's raw6rank summaries/refine details/EXIT
   and root rejection BEFORE stopping only verified owned launcher. Audit ALLhandles
   terminal and ALLpins unchanged BEFORE any compiled source edit. One connected
   fix/validation/retry; no unrelated additions. Actual outcome drives next target.
3. Shape/height close first, then residual geometry-directed repair and size handoff,
   with shape remeasurement/reclosure after geometry changes under same cumulative
   budgets. This phase has not yet been reached on accepted M6. Do not relax gates.
4. Require TWO strict accepted mesh/native_ref pairs, actual conservative admissible
   transfer(double residual1e-10,targetintegral1e-12),finite continued CFD and all
   point/history rho>0/internalenergy>0. Passing fixtures/private progress are not this.
5. After workflow terminal, isolated COPY final accepted mesh/native_ref+matchingflow;
   audit header/order/coords/finite-positive state; ACTUALdisk CFD continuation,
   RESTART_SOLYES/ADAP_LOOPNO/correctSOLUTION_FILENAME/distinctoutputs. Original seed/
   restart00000 never proves adapted restart. Then dedicated BL/MPI and RANS500k.

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
