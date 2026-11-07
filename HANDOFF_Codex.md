# Native adaptation — current handoff

Updated2026-11-07(Europe/Rome). Goal ACTIVE: TWO strictly accepted ONERA M6 Euler
CFD→mesh→CFD→mesh at50k metric complexity, conservative admissible transfer, finite
continued adapted CFD and isolated ACTUAL adapted disk restart. Then dedicated
BL/constraint/MPI qualification and corresponding RANS500k. ZERO accepted meshes.
Actual point/cell counts are separate from metric complexity. Fixtures are not completion.

Keep geometry1e-4,q>=.2,J>=.05,max metric edge1.8,requested heights strict. Construction
remains validity-first: exact positive/nondegenerate/topology/body/feature/height
validity, NO fresh q/J measurement/ranking/veto. CAD optional; no-CAD sufficient.
One connected cycle/ONE owned heavy job. Freeze C++/fixtures/helpers/binary/config/
inputs through builds/validation. Verify exact HOST handles before edits/restarts.
Preserve evidence before an authorized stop/fix/retry. Do not repeat historical stops.

R=`integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57`.
A237=`R/m6_runs/euler_build237_signed_bounds_real_cfd50k`.
A235=`R/m6_runs/euler_build235_geometry_coupled_real_cfd50k`.

## TERMINAL237 — current outcome, next diagnostic

Production source checkpoint4d1e2ec5b0, pushed; terminal/inspection checkpointd423a0b661 pushed.
Full tool2688 TERMINAL1,5633.581992s,pins unchanged. First native event naturally
refused at5347.003818s/round37505:1020222cells/296q/327J/1366geometry/0size,
ALL final measurement flags TRUE. ZERO accepted meshes. All30capture files sealed
and1519source/runtime/input pins verified before ONE authorized pidfd SIGTERM of
exact owned launcher2862082/start7510715. All11owned handles now ABSENT on host,
A237/terminal_handles.json. NEVER repeat237or any historical stop.
R/m6_237_launch.json is terminal. No frozen solver remains; selected export below
is the one owned heavy workload. No C++ production edits since4d1e2ec5b0.

[Authoritative237outcome/next target](Handbook/ADAPTATION_BUILD237_OUTCOME_20261107.md).
First quality pass exhausted at44q/17J/1656geometry:46shape-union ALL blocked,
queue0,missing_features0.40q/all17J have physical faces; all46have short edge<.5.
Existing fallback reconstruction reduced geometry1656→1379 but fresh shape entry
became410q/493J. Reclosure reached296q/327J/1366g before budget. Dedicated
geometry-only phase was NOT reached. Construction intermediate q/J caches are
incomplete; do not mistake printed partial counts for fresh measurements.
Final430shape-union all have an edge<.5; latest window17stuck, so not all final
cells exhausted their repair opportunities. Short-edge correlation is not causality.

Next target is repair power and coupled surface/volume reclosure. Select actual
bounded residual boundary cavities with successful/clean controls; test ONE connected
hypothesis using existing facilities. Distinguish placement, constrained motion,
connectivity and conservative certification refusal. No new scheduling framework,
config, larger cavities, geometry representation or quality-aware construction.
Keep validity-first construction and every final gate. Geometry cost remains measured.

237only changes signed residual solve selection from fresh outward intervals.
[Proof/qualification](Handbook/ADAPTATION_GEOMETRY_SCREEN236_20261107.md):116serial
cases/82379assertions and ONE MPI6all26cases/rank; all168actual-cavity outcome records
match235 excluding work/time. Same12bounds bit-identical, traversal-5.96%.
236coarse-plane screening REJECTED/reverted; no slack cache selected. Existing clean
235two-face controls already have useful original-star alternatives; don't broaden
optional four-star preflight on eligibility alone. All common full-run stage outcomes
through36800match235; construction traversal-5.04%, last common quality~4.84%lower.
Wall differences are descriptive under contention, not isolated causal estimates.

Read-only R/analyze_m6237_{stages,quality}.py refresh exact rank/stage/round prefix
joins. Quality cumulative comparison stops at FIRST quality exit; reconstruction
changes the census outside quality-window churn counters. Latest current window
location/ladder analysis is independent. An initial cross-reconstruction churn
assertion identified this analysis-scope error; runtime/repair decisions unchanged.
Timers/rejections overlap; no-face is not free interior; commits/debt alone are not
closure. A237/terminal_outcome_assessment.json and quality_counter_assessment.json.

## Selected237mesh — saved and independently read

Existing offline inspector tool13732 TERMINAL0,425.036783s,allpins unchanged,
all3sampled owned handles absent. Independent meshio reader tool86406 TERMINAL0,
5.542227s: all4VTUs load, every PointData/CellData array has the exact required
length, all cache_valid1, all nodal tensors finite/symmetric/SPD (min eigen.0025878931).
Full rejected_mesh.vtu has198941points/1020222tets and captured_nodal_metric
9row-major components; q296/J327/geometry1366,shapeunion430,badunion1725exactlyjoin
terminal census. Maximum metric edge1.7999977268. SU2/VTU coordinates and tets match.
ALL1725bad cores and ALL25856one-node-neighbor cells retained, no omissions;
reference/rejections/ID maps available. No ParaView GUI qualification; diagnostic
SU2marker-name fallbacks remain. No source change or new acceptance claim.
A237/inspection/rejected_mesh.{vtu,su2}; bad_cells.vtu,cavities.vtu,reference.vtu.
A237/inspection_reader_status.json records output hashes and independent readback.
A237/probe_support237.py reuses prior exact topology census with explicit current
count checks and clean-cell short-edge controls; consult support237_coverage_assessment.json
for completed exact selection:159no-face/266one-face/5two-face shape-bad; five
complete contexts fit64cells (42,52,48,55,35). Clean one-face cells also commonly have
short edge<.5(54653/65234), so no causal construction diagnosis. No production repair selected.
R/prepare_current237_witnesses.py prepares these five using existing raw fixture format;
tool23557 TERMINAL0,42.698837s,all1020222rows paired,32968boundary points/65932faces,
input hashes unchanged. Five actual contexts plus cached q/J oracles prepared.

Current actual-cavity diagnostics BOTH TERMINAL0,all pins unchanged/all sampled host
handles absent: compound tool89903/case...073703_3082067,121.090848s,36470assertions;
controls tool49679/case...074059_3095033,153.948427s,34392assertions. Compound covers
THREE two-face contexts; separate current_residual covers ONLY the other TWO, avoiding
repeat controls. All FIVE old q/J fields match exactly; all20original-star controls
and13alternative candidates retain whole-context/churn/geometry/size measurements.
A237/current_residual_assessment.json, derived by R/assess_current237.py, passes exact
joins/source-hash checks. No production edits, accepted mesh, nonlocal publication
or nonzero-height proof. Fresh associations/context-only current nodal field, not
historical replay of first-pass blocked44q/17J tail. Alternatives overlap, never sum gains.

Useful original stars: geometry_clean_0(no-face)3/4, geometry_bad_1(one-face)0/4,
geometry_clean_2(two-face)2/4, geometry_bad_3(two-face)0/4, geometry_bad_4(two-face)1/4.
Bad_3's tested combined shell+geometry split+optimizer improves5q/3J→1q/0J but
geometry7→11 and excess6.56x, correctly refused. Collapse+retained-star improves
5q/3J→4q/2J but geometry excess rises~3.1%, also refused. No failure of certification
soundness follows. Bad_1 lowers debt/geometry in one star but retires NO net q/J.

## TERMINAL238 — bounded one-face hypothesis rejected

Build238only changes the DIAGNOSTIC fixture's filter: collapse/retained-star tests
now cover current non-two-face cores; shell-only branch retains its two-face guard.
Production code/config/gates/budgets unchanged. Buildtool2530 TERMINAL0,234.181180s,
all1302pins unchanged,both targets current,all compiler handles absent.
Current_compound tool45667/case...075136_3126162 TERMINAL0,125.348654s,
37975assertions/1case,allpins unchanged/all4observed handles absent. No heavy job live.
R/{build238_status,build238_terminal_handles,current238_launch}.json are authoritative.

All FIVE old q/J match exactly;20original-star controls and15alternatives measured.
All33previous237candidate outcomes unchanged (shape/geometry/counts/metric/interface/
trials,excluding work/timers). R/assess_current238.py validates this and churn identities;
A237/current_residual238_assessment.json retains all outcomes/reasons/source hashes.

One-face230222079/52-cell context:12directed collapses,onlyONE constructs. Seven
obstructed-cell refusals,three body/feature refusals,one leaves/reverses geometry
support. The constructed collapse+retained-star candidate changes4q/3J→4q/2J,geometry
bad6→6 but excess1.73696e-12→2.82190e-12(+62.46%),correctly refused. No useful
alternative. Reject production integration/support broadening from this hypothesis;
keep the diagnostic coverage fix. No larger cavity, new operator, scheduler or
construction-policy change selected; no new full-M6 baseline repeat on fixture238.

Next SAME-cycle diagnostic: distinguish actual geometric displacement vs conservative
certification on refused changed faces of this one-face context and two-face230712740.
Reuse existing query/reference export and rigorous lower/upper facilities; sampling
alone cannot prove geometry acceptance. Do not add another local operator variant or
relax whole-context geometry/shape/size/height gates without a demonstrated correction.
Current nodal/context-only field/fresh-association limits and nonlocal/height exclusions
above remain. Once justified production change is qualified, immediately full same-seed
TWO-event M6; downstream acceptance/transfer/CFD/restart obligations remain unchanged.

Controls run config was a byte-identical symlink: run_cases resolves it when adding
extra receipt pins, while fixture uses the raw environment directory and correctly
executed ONLY two cases. Supplemental current_controls_actual_input_pins.json
verifies49actual two-case input pins from PRELAUNCH preparation, unchanged at terminal.
Use a physical copy of run.cfg for future selected subdirectories so harness receipt
lookup follows actual selected inputs. Current result needs no rerun: input/output
hashes and exact two-case result rows independently establish its executed scope.
Prior tool35421/case...073615_3079660 TERMINAL53 BEFORE test execution: omitted the
previously qualified MPI pt2pt environment, MPI_Win_create failed. Pins unchanged,
all sampled handles absent before retry; preserved evidence. No repair outcome from it.
Host export receipt includes global memory PSI transient with owned VmSwap0;
wall timings remain descriptive. Reader/probe are terminal; the bounded current-cavity diagnostic below is now active.

## Acceptance obligations — all still open

Shape closure then shape-preserving residual geometry/size repair; validity-only
reconstruction requires fresh shape/size/height reclosure in SAME budgets. TWO
accepted native events, true CV mass-solve residual<=1e-10, all projected states/
timelevels admissible, scaled target-integral defect<=1e-12 BEFORE publication.
Old/new-domain integral change is separate from target defect. Finite CFD alone
is insufficient. Require actual adapted_mesh_adap_00001/2.su2+native_ref and matching
restart_flow_adap files, exact coordinates/order1e-12, finite states,rho/internal
energy positive. Fresh isolated ADAP_LOOP=NO/RESTART_SOL=YES/READ_BINARY_RESTART=YES,
distinct input/output prefixes, ACTUAL disk load and admissible continuation.
Refused-event restarts on original mesh are not adapted proof. Resume-cycle overwrite
and rejected-cycle consumption remain OPEN. Then dedicated BL curved/junction/MPI/
subsequent-operation/ownership/requested-height qualification, then RANS500k.

## Fixed input provenance and saved inspection

Saved REAL original-grid CFD seed R/m6_inputs/euler_original_force_seed_v1:
61330points/303359cells,CL.269535/CD.013536,stable force window, NOT full residual convergence.
SeedSHA1eaee788bd62bd92a92ec539577dcbcfc0f8042dcbcec23adb04e825bb01a880.
Corrected shared metric219qualified; same-seed L1/L2at50k compared and L1selected
(shock/corner/farfield/gradation evidence R/metric_target219_production/selection.json).
ConfigSHA74b2e9f476cf6ba1fdc5ebcdcb1c34a2b3c45b25e58d651e75417d89402bfe8d;
initialrestartSHA5132ee2a83635b457943c2679a3a50c5e0b733427e620cc748da53865ce5d692
BYTE-IDENTICAL219/225/227/235/237. A237/initial_metric_input_provenance.json.
First adapted metric always uses this saved real solution; event2requires accepted
mesh plus continued CFD. Do not recompute/reselect seed or assume L2better.

Full235naturally refused at native5334.959466s:1017408cells/3840q/2068J/1686g/0size,
COMPLETE final caches. All30files sealed; one authorized stop after seal, all handles
terminal. NEVER repeat it or historical227/225/219/217/210stops. Its independently read
A235/inspection/rejected_mesh.vtu WITH9-component transported nodal metric has
198318points/1017408tets; bad_cells.vtu ALL7090cores, cavities.vtu ALL220573neighbors,
reference/rejections/ID maps. Exact SU2/VTU coordinates/tets agree, metrics finite/
symmetric/SPD; no ParaView GUI check, diagnostic SU2 marker-name fallbacks. Current
saved mesh remains235; no per-trial files.227final geometry caches were UNKNOWN;
its separate centroid q/J diagnostic is not an equivalent production final census.

## Operations and records

One owned heavy job,nice19,libs1,build3workers. Check actual host RAM/load/disk,
use4TB compile-tmp. Cache-accounted bytes, summed RSS and physical RAM differ.
Fork origin https://github.com/rois1995/SU2.git, branch codex/native-3d-bl; pushes
already authorized. Git metadata outside workspace requires escalation. Use
`gc.auto=0`, `submodule.recurse=false`, exact reviewed whitelists/ignore-submodules=all.
Preserve unrelated logs/analysis/.orig/broken Eigen submodule. No raw meshes/binaries/
restarts/logs committed. Literature is design context, not a competing implementation.

- [How to proceed](HowToProceed.md)
- [Integrated task](Handbook/ADAPTATION_INTEGRATED_NEXT_TASK.md)
- [237proof/control/qualification](Handbook/ADAPTATION_GEOMETRY_SCREEN236_20261107.md)
- [235outcome](Handbook/ADAPTATION_BUILD235_OUTCOME_20261107.md)
- [Connected coupled repair](Handbook/ADAPTATION_GEOMETRY_CONFORMING_REPAIR_20261107.md)
- [Geometry witness qualification](Handbook/ADAPTATION_GEOMETRY_WITNESS_20261106.md)
- [Repair support and literature](Handbook/ADAPTATION_REPAIR_SUPPORT_20261106.md)
- [Metric correction/target choice](Handbook/ADAPTATION_METRIC_FLOOR_20261106.md)
- [Backlog](Handbook/ADAPTATION_BACKLOG.md)
- [Detailed pre-consolidation handoff](Handbook/History/HANDOFF_Codex_before_quality237_20261107.md)
