# Native adaptation — current handoff

Updated2026-11-07(Europe/Rome). Goal ACTIVE: TWO strictly accepted ONERA M6 Euler
CFD→mesh→CFD→mesh at50k metric complexity, conservative admissible transfer, finite
continued adapted CFD and isolated actual adapted disk restart. Then BL/constraint/
MPI qualification and corresponding RANS500k. ZERO accepted adapted meshes.
Actual point/cell counts are separate from complexity; fixtures are not completion.

Keep geometry1e-4,q>=.2,J>=.05,maxedge1.8,requested heights strict. Construction is
validity-first: exact positive/nondegenerate/topology/body/feature/height validity,
NO fresh q/J measurement/ranking/veto. CAD optional; no-CAD authority sufficient.
One connected cycle/one owned heavy job. Freeze C++/fixtures/helpers/binary/config/
inputs through builds/validation. Verify exact host handles terminal before edits/
restarts. Preserve failures; authorized stop/fix/retry remains in force.

R=`integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57`.
A227=`R/m6_runs/euler_build227_witness_real_cfd50k`.
A235=`R/m6_runs/euler_build235_geometry_coupled_real_cfd50k`.

## ACTIVE235: geometry-conforming private quality candidate

Production source checkpoint3839534f25 (connected235production/qualification);
preceding pushed evidencec70641d475(mid-quality),c7b3a78de4. Earlier8107f77356local feasibility.
Production/fixture changes committed; preserve unrelated working changes.
Build235exec12651TERMINAL0,63.151040s,1302pins unchanged/both targets up to date.
All recorded host compiler/linker handles absent. Build234also terminal0,168.133394s.

Actual current_residualexec59187TERMINAL0,96.829376s,pins unchanged; all handles
2425292/2425500/2425501/2425502/2425503host ABSENT. Production complete50-cell
candidate PASSES:50→53cells,q2→0/J1→0/geometry4→2, geometry excess-89.39%, maxedge
unchanged,143/160trials,.943326s local. R/current_residual235_assessment.json.

Serial integrationexec89949TERMINAL0,244.277670s, all3groups exit0/pins unchanged:
quality13cases/4586assertions,atlas88/30542,failure-census14/6845. With actual case,
116serial cases/78351assertions. MPI6exec61924TERMINAL0,6.375343s, EACH rank26cases,
pins unchanged. All observed serial/MPI host handles absent:
R/{serial235_terminal_handles,mpi235_terminal_handles}.json. Qualification terminal.
R/geometry_conforming235_qualification.json records exact scope/limits. Adjacent
MPI protocols qualified; exact joint two-face predicate not explicitly asserted
by those fixtures, full M6 is first exact-predicate MPI workflow test.

SOLE OWNED HEAVY JOB: full two-event235 M6 LIVE, tool session38789. Existing
run_m6.py build235 euler_build235_geometry_coupled_real_cfd50k 2 with saved realCFD
seed and --inspect-rejected. Source/helpers/binary/config/inputs FROZEN. Same
correctedL1/50k/native5400/outer14400; nice19/libs1,pt2pt,unique4TB MPI scratch.
Host handles verified by PID/startticks/argv/cwd:
helper2473644/6446256, runner2473645/6446259,
GNUtime2473939/6446870, timeout2473947/6446870, launcher2473948/6446871;
workers2473951–2473955/start6446874,2473956/start6446875.
Exact identities in A235/owned_startup.json; R/m6_235_launch.json launch receipt.
All1519source/runtime/input pins verified; config SHA unchanged and initial restart
BYTE-IDENTICAL219/225/227: A235/initial_target_identity.json. Latest observation
A235/owned_observations.jsonl: all11handles live, availableRAM~31.1GiB, load~10.
Summed RSS is not unique physical memory. Construction underway; no accepted mesh.
NEXT: observe existing run and compare counted work/whole-field q/J/geometry at
matching stages against227. Do not edit pinned sources or launch another heavy job.
Stop only after preserved demonstrated failure/bottleneck/resource evidence under
authorized policy; no stop signal sent235. Full M6 determines next implementation.
Quality entry235 COMPLETE all6ranks:871607cells/102058q/101860J/1997geometry/0size,
EXACT same entry counts as227. All matching construction-stage counts/checked
operation/query/traversal/cache work agree; same prepared limits. Quality-entry
native969.422s vs775.819s; construction-entry902.062s vs730.001s. Wall-time
variation is observational, not isolated implementation overhead. WorkerCPU/wall
~.88–.91 over sampled251s (includes MPI spinning, not pure useful CPU).
A235/{comparison227_matching_stages,quality_entry_comparison227}.json preserves
prefix-verified data. NEXT evaluate net whole-field q/J removal and geometry/churn
from quality entry; remaining time is smaller, budgets unchanged. No accepted mesh.
LATEST COMPLETE36096/native4144.457s:993905cells/21070q/17509J/1749geometry/0size,
all completeness flags1. Twenty-four windows, quality3175.035s; net80988q/84351J/
248geometry removed. Netq25.5078/s vs22725.4864/s at matching round; traversal
.946623x227. No useful timing speedup/closure established. First window EXACT227
counts/churn/checked work; second diverges rank5, later populations/ownership differ.
A235/quality_comparison227.json reuses existing checker by explicit labels/paths,
verifies source prefixes and global q/J churn reconciliation. Nearest measured
quality-time comparisons retain actual gaps; no interpolated counts/causal claim.
Quality migrations keep full-field counts/measurement flags; initial and34752
checks in quality_ownership235_assessment.json. Publication.144s excludes policy/
preparation/later warming; preceding-window→publication13.169s at34752. Later raw
complete census groups preserve further migrations. Call timers overlap, not CPU idle.
All1519pins unchanged at late recheck:source_freeze_late_quality_observation.json.
Primary reviewed both bounded location summaries against hashes/exact stage-round-
rank/census joins:quality_early_location235_assessment.json and
quality_mid_location235_assessment.json. Mid snapshot35904:26843q/23653J/1764geometry;
24856q have no physical face, but can touch constrained vertices. Do not infer
unconstrained volume from face-only labels; exact location net changes are in artifact.
In its last window optimizer net1447q/1539J; topology fallback net100q/105J from
28/6304trials in3.90rank-s, so low yield alone is not the dominant timing cost.
Stuck4/39251bad union, blocked266/queue346669, missing0. Nearlyall bad cells have
edge<.9 (correlation, not construction-causality proof). Fixeddrift16DISTINCTglobal
points repeated on6ranks, not96distinct; all supported, maxsampledrelativeFrobenius
.038733 at35904, not a whole-target norm. No competing implementation.


[Current connected design](Handbook/ADAPTATION_GEOMETRY_CONFORMING_REPAIR_20261107.md):
existing batch preflight optionally merges four vertex stars for a measured owned
q/J-bad+geometry-bad two-face core, within EXISTING64-cell bound, before existing
FetchBatch/ReserveBatch. Oversized growth retains original feature. Owner/version
checks, empty-rank alignment, fair oldest slot, worst-first order, rejected-star
invalidation and global full-vertex-disjoint reservation remain authoritative.
NO new per-slot collective, scheduler/config, support limit or construction policy.

After a stalled ordinary local pass, ONE private shell→reference-projected split→
existing optimization candidate may use REMAINING work/time/sweep allowance.
Nested optimization cannot recurse. Extra shadow/geometry storage admitted in same
ceiling. Failed child work charged; progress/IDs credited only on full admission.
Original whole-context q/J-bad counts must not increase and one must decrease;
merit/height must progress; geometry excess cannot increase; new-edge guard and
exact canonical prescribed interface must pass. Caller retains replacement/contact/
height/exact changed-edge/publication/final gates. Private contexts owned at use.

Actual23250-cell proof: q2→0/J1→0/geometry4→2, geometry excess-89.39%, max edge
unchanged;99optimizer trials. NOT global mesh/publication/height qualification.
Prototype233primitive early-admission FAILED actual-star regression; removed.
234complete private-candidate regression FAILED; found incorrect unsorted/noncanonical
interface packaging.235uses existing canonical map convention, preserves strong
same regression and tightens reserved-ID guards. Failure exact source patches/
hashes saved R/coupled233_failure.json and coupled234_failure.json against8107f77356.
No broad/MPI/full M6 run on233/234. Current regression required useful PRODUCTION
COMPLETE50-cell-context progress, matching new support union; original-star controls
retained. It passes; source remains frozen through current qualification.

Focused serial and ONE MPI6 milestone pass; IMMEDIATELY full same
saved CFD seed/correctedL1/50k/two-event M6 at5400native/14400outer. Pending optional
ownership admission collective-AND guard is qualified in this SAME milestone;
no actual split observed in227. Do not start a separate full run solely for a guard.

## Terminal227 and actual saved mesh

Full M6exec50998TERMINAL1,5824.165090s,pins verified/changed[]. First native event1
naturally rejected time budget at5335.996158s/round37022. Final1017523cells but ALL
final census flags0, printed zero q/J/geometry counts UNKNOWN, never passing.
Last COMPLETE census36992:1017224cells/2691q/1295J/1700geometry/0size-bad.
After sealed30file capture, SIGTERM sent ONCE via pidfd to verified launcher1929998/
start4973666; NEVER repeat. All10solver handles absent,1519pins unchanged. Historical
225/219/217/210stops also NEVER repeat. A227/{capture_ready,owned_stop_once,
terminal_handles,terminal_source_receipt}.json. Detailed [227outcome](Handbook/ADAPTATION_BUILD227_OUTCOME_20261107.md).

Selected A227/inspection/rejected_mesh.vtu WITH9component captured_nodal_metric,
also rejected_mesh.su2:198364points/1017523tets. Full mesh+metric COMPLETE; all point
tensors finite/symmetric/SPD, independent meshio reader passes/exact SU2 coordinates.
Final cached q/J/geometry allNaN/cache_valid0. Empty original bad/cavity views mean
UNAVAILABLE classification, not zero bad cells. No ParaView GUI qualification.
Surface extraction inherits SAME3Dnodal tensor; no separate surface tensor fabricated.

Native centroid shape recovery228:1017523rows,2368q/1101J,union3258. Diagnostic
shape_inspection/{bad_cells,cavities}.vtu WITH metric and explicit q/J_centroid fields;
141392complete one-node-neighbor cells, exact raw ID/version join/readback verified.
Geometry remains unknown.1414bad cells touch wing vertices without physical face;
1341free interior,452wing face,39other physical face,12other-boundary-touching.
Do not mistake face-only interior labels for unconstrained volume. No trial VTUs.

Raw shell query231: one new face per tested shell rigorously outside exact queried
support at125.5107/104.3380µm vs100µm; other new face certified within. Production
upper bounds reproduce native shell guidance deltas to relative1.9e-16. Faster
certification alone cannot admit those raw faces. Not optimized-position replay,
not a statistical sample or arbitrary other-body nearest-distance claim.

## Input, comparison and downstream acceptance

Saved REAL CFD seed R/m6_inputs/euler_original_force_seed_v1:61330points/303359cells,
CL.269535/CD.013536, stable force window, NOT full residual convergence. SeedSHA
1eaee788bd62bd92a92ec539577dcbcfc0f8042dcbcec23adb04e825bb01a880.
Corrected shared metric219qualified; same-seed L1/L2at50k compared, L1selected
(shock/corner/farfield/gradation evidence R/metric_target219_production/selection.json).
Do NOT assume L2better. M6configSHA74b2e9f476cf6ba1fdc5ebcdcb1c34a2b3c45b25e58d651e75417d89402bfe8d,
initial restartSHA5132ee2a83635b457943c2679a3a50c5e0b733427e620cc748da53865ce5d692
BYTE-IDENTICAL225/219. Copy same seed for first adaptation; event2requires accepted
mesh plus continued CFD. Post-rejection CFD on original grid is not adapted proof.

227witness screening reduced construction traversal17.6%, same76checked stage/
operation vectors. First19quality windows match225through35776, then legitimate
remaining-time migration differs. Later paths/work populations differ: no isolated
kernel timing attribution. Last complete window110.6795s: net853q/502Jremoved,
1211commits, geometry removed2. Boundary12063candidates/445moves/72.60rank-s,
interior5641/676/3.26rank-s. Timers overlap; private debt credits not global gain.
R/terminal_quality_window_assessment.json under A227. Judge full-field net q/J-bad
removal/s, geometry counts/churn and counted work, not commit rate/debt alone.

After shape closes: residual geometry/size closure must preserve shape/heights;
validity-only reconstruction needs shape remeasurement/reclosure, same budgets.
Require TWO accepted meshes. BEFOREpublication: trueCV conservation residual<=1e-10,
all projected states/timelevels admissible, scaled target integral1e-12. Report
old-vs-new-domain integral change separately. Finite post-CFD alone is not proof.
Verify actual adapted mesh/native_ref/restart files, exact point order/coords1e-12,
finite conserved states/rho/internal energy, then fresh isolated ADAP_LOOP=NO/
RESTART_SOL=YES/READ_BINARY_RESTART=YES with distinct input/output prefixes and
actual disk load/finite admissible continuation. Resume-cycle0overwrite/rejected
cycle consumption remain OPEN; isolated solver restart does not claim them fixed.
Then dedicated BL curved/junction/interface/subsequent-operation/ownership/height
MPI qualification, then RANS500k. No acceptance/transfer/adapted-CFD/restart yet.

## Operations and linked records

One owned heavy job,nice19,libs1,build3workers; check host RAM/load. Scratch
integration_evidence/build-integrated-v2/compile-tmp on4TB. Account process RSS,
cache bytes and physical memory separately. Fork origin https://github.com/rois1995/SU2.git,
branch codex/native-3d-bl. Git metadata outside workspace requires escalation;
use gc.auto=0/submodule.recurse=false/ignore-submodules=all, exact reviewed whitelists.
No mesh/binary/restart/raw logs committed. Preserve unrelated logs/analysis/.orig/
broken Eigen submodule. Local218shape diagnostic was reviewed/integrated, not lost.

- [How to proceed](HowToProceed.md)
- [Integrated task](Handbook/ADAPTATION_INTEGRATED_NEXT_TASK.md)
- [Current geometry-conforming repair](Handbook/ADAPTATION_GEOMETRY_CONFORMING_REPAIR_20261107.md)
- [227geometry witness qualification](Handbook/ADAPTATION_GEOMETRY_WITNESS_20261106.md)
- [Repair/support and literature](Handbook/ADAPTATION_REPAIR_SUPPORT_20261106.md)
- [Metric floor and L1/L2choice](Handbook/ADAPTATION_METRIC_FLOOR_20261106.md)
- [225outcome](Handbook/ADAPTATION_BUILD225_OUTCOME_20261106.md)
- [Backlog](Handbook/ADAPTATION_BACKLOG.md)
- [Detailed pre235handoff history](Handbook/History/HANDOFF_Codex_before_geometry235_20261107.md)
