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

FULL235 FIRST EVENT TERMINAL/REFUSED at unchanged time budget: engine5334.959466s,
round36852,1017408cells/3840q/2068J/1686geometry/0size. Unlike227, FINAL census is
COMPLETE on all6ranks; cached q/J/geometry are measured, not unknown. All30selected
capture files sealed with hashes/owned416-byte-record counts and final-census joins:
A235/capture_ready.json, metric_census_complete=true. No accepted mesh.

After sealing, SIGTERM sent ONCE via pidfd to verified launcher2473948/start6446871
because a first refused event makes TWO accepted meshes impossible in2events.
NEVER repeat this stop. A235/owned_stop_once.json. Full wrapper session38789
TERMINAL1,5595.034596s; statuspins_verified=true/changed[]. All11recorded host
handles ABSENT:terminal_handles.json. Source/runtime/input1519pins unchanged at
source_freeze_before_stop.json and rechecked after all exports/readback in
terminal_source_receipt.json. No CFD solver workload remains; production source
still unchanged235. Export session52272 TERMINAL0,427.843911s,pins unchanged; all3
export host handles absent. Reader session12859 TERMINAL0,4.287199s. NO OWNED HEAVY
JOB. Existing inspector and all four VTUs independently read; full coordinates/
tetrahedra matchSU2, metric tensors finite/symmetric/SPD, actual bad counts match.
A235/inspection/rejected_mesh.vtu WITH metric:198318points/1017408tets; all7090bad
cores and220573neighbor cells saved without omissions, reference/rejection/ID maps.
q/J/geometry caches COMPLETE, unlike227. No ParaView GUI check; diagnostic SU2 has
marker-name fallbacks. A235/inspection_reader_status.json has hashes/checks.
[235outcome and next target](Handbook/ADAPTATION_BUILD235_OUTCOME_20261107.md).
Next target SPEED OF USEFUL SHAPE CLOSURE, analysis first using existing geometry/
smoothing facilities and actual residual cavities; no speedup from slack bounds
established. No new implementation selected. Preserve source235 for bounded actual
query/cavity controls before one connected correction and full M6 retry. Do not start another M6 before selecting and
validating the one connected next step from this outcome.

A235/owned_startup.json retains OLD handles only; they are terminal, never live.
Same correctedL1/50k/realCFD seed/config as227, initialrestart byte-identical219/225/227.
Terminal counter report PRIMARY-REVIEWED against exact rank/stage/round/census joins,
prefix hashes and independently read VTU counts:terminal_quality235_assessment.json.
Nearest COMPLETE227byquality age36864:4342.589895436s versus235EXIT4365.537211719s,
gap22.947316283s; counts4606q/2421J/1711g versus3840q/2068J/1686g. No exact-time or
causal speedup claim. Final66.0207s net861q/523J removed,g+3; boundary49.21versus
interior4.03rank-s (overlapping scopes). Exact memo misses do not identify nearby
queries. Queue63623/stuck28/missing0; short-edge correlation is not construction
causality. Fixeddrift16unique global samples is not a whole-target norm.
Detailed current outcome, data limits and next analysis are in the235outcome link.


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

Focused serial/ONE MPI6 milestone passed; full same-seed two-event235now refused
as above. Ownership-admission collective AND was qualified in the SAME milestone;
no split observed227. Do not launch a separate full run solely for this guard.

## Historical227 comparison and inspection

227first native event naturally refused at time budget; fullsession50998terminal1.
Unlike235, its final cached counts were UNKNOWN. Last complete36992:2691q/1295J/
1700geometry/0size. One sealed-evidence stop sent launcher1929998/start4973666;
all handles terminal, NEVER repeat it (or225/219/217/210historical stops).
A227/inspection actual198364points/1017523tets and full metric are saved; final q/J/
geometry cache unavailable. Separate centroid shape diagnostic recovers2368q/1101J,
not an equivalent complete production geometry census. Use235COMPLETE measured
artifact for current inspection. [227outcome](Handbook/ADAPTATION_BUILD227_OUTCOME_20261107.md).

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
- [235complete-attempt outcome](Handbook/ADAPTATION_BUILD235_OUTCOME_20261107.md)
- [227geometry witness qualification](Handbook/ADAPTATION_GEOMETRY_WITNESS_20261106.md)
- [Repair/support and literature](Handbook/ADAPTATION_REPAIR_SUPPORT_20261106.md)
- [Metric floor and L1/L2choice](Handbook/ADAPTATION_METRIC_FLOOR_20261106.md)
- [225outcome](Handbook/ADAPTATION_BUILD225_OUTCOME_20261106.md)
- [Backlog](Handbook/ADAPTATION_BACKLOG.md)
- [Detailed pre235handoff history](Handbook/History/HANDOFF_Codex_before_geometry235_20261107.md)
