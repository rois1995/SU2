# Native adaptation — current handoff

Updated2026-11-07(Europe/Rome). Goal ACTIVE: TWO strictly accepted ONERA M6 Euler
CFD→mesh→CFD→mesh at50k metric complexity, conservative admissible transfer, finite
continued adapted CFD and ACTUAL isolated adapted disk restart. Then dedicated
BL/constraint/MPI qualification and corresponding RANS500k. ZERO accepted meshes.
Actual point/cell counts are separate from metric complexity. Fixtures are not completion.

Geometry1e-4,q>=.2,J>=.05,maxedge1.8,requested heights remain strict. Construction
validity-first: exact positive/nondegenerate/topology/body/feature/height validity,
NO fresh q/J measurement/ranking/veto. CAD optional; no-CAD authority sufficient.
One connected cycle/ONE owned heavy job. Freeze C++/fixtures/helpers/binary/config/
inputs through builds/validation. Verify exact HOST handles before edits/restarts.
Preserve evidence before authorized stop/fix/retry; never repeat historical stops.

R=`integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57`.
A237=`R/m6_runs/euler_build237_signed_bounds_real_cfd50k`.
A235=`R/m6_runs/euler_build235_geometry_coupled_real_cfd50k`.

## Current240 — terminal, exact actual geometry diagnosis

Latest pushed checkpoint65a7db8ee9; production source4d1e2ec5b0unchanged since237.
238only broadened DIAGNOSTIC current-cavity coverage to non-two-face collapse trials;
239/240only extend opt-in geometry query DIAGNOSTIC with rigorous outside witnesses.
No production operator/config/policy/gate/budget change. Current Common test source
and240metadata need the next exact-whitelist checkpoint. All owned heavy jobs terminal.

[Authoritative240geometry findings/next action](Handbook/ADAPTATION_CAVITY_GEOMETRY240_20261107.md).
One-face230222079/52-cell context: all4original stars give no net q/J gain. Only one
of12directed collapses constructs(291052210→7876); optimizer makes NO change. Context
4q/3J→4q/2J but geometry excess+62.46%, correctly refused. The238hypothesis is REJECTED
for production; no support broadening/new fallback. Three other sampled contexts
already have useful original stars. All33prior candidate outcomes unchanged.

Because optimizer did not change the constructed collapse, exact face rewrite is
available from existing raw records. R/prepare_collapse238_queries.py exports FOUR
old/TWO collapsed faces, all anchor16734, captured coordinates/bindings unchanged;
A237/collapse238_geometry_queries_v1/geometry_query_preparation.json before/after hashes.
Existing query fixture validates fresh associations, C++ support discovery and
cached/repeated/recomputed bounds. Area-weighted production-guidance delta matches
native candidate1.0849396192800913e-12 to3.73e-16relative. Raw union bound can be looser
than production direct correspondence; use production_guidance_upper for the score.

BOTH collapsed faces have certified whole-component distance lower248.334µm and
112.330µm versus100µm gate. Old face and first collapsed face share original chord
7876–7868, whose exact midpoint has the248.334µm violation. Original facet16734
contains both endpoints. These are true violations of stored cubic authority;
cheaper certification alone cannot make these faces satisfy FINAL geometry. This
is not proof that intermediate weighted bounds are tight or every refusal physical.

Outside witnesses use15dyadic points/refused face, exact ParametricPointBounds,
existing ClosestCurved lower/upper for whole hinted same-marker connected cubic
component, rigorous source-point rounding correction, downward subtraction and an
EXTRA conservative max(mapping error)=1e-11margin. Both kernels use same stored cubic
nets; this margin is not a missing kernel term and makes no separate CAD claim.
No acceptance from samples; exceptions UNKNOWN. Each closest query work<=16384,
workspace<=8GiB,precision tolerance/64. All60probes complete,16prove outside.
R/assess_collapse240.py verifies exact rational point membership/displacement/lower
correction and guidance delta; A237/collapse240_geometry_assessment.json.

Build239tool51943terminal0,42.107258s,pins unchanged,all compiler handles absent;
NO validation239. Extra margin added only after terminal. Build240tool75913terminal0,
45.143734s,all1302pins unchanged,both targets current,all compiler handles absent.
Query tool49570/case...080717_3175207terminal0,53.562074s,443assertions/1case,pins and
supplemental source/preparation pins unchanged,all5sampled handles absent. Runner
start ticks captured; job ended before detailed tree/start sampling; no stop sent.
R/{build240_status,build240_terminal_handles,query240_launch}.json and case
terminal_handles.json authoritative. No live frozen workload; sources may change.

## Next action — same cycle, actual geometry/volume reconstruction

Assess EXISTING projected SplitEdge/InsertEdgeCavity/ReconstructSplitPatch on the
actual chord7876–7868 and complete CURRENT neighboring volume in captured52cells.
First identify primitive projection/positive-cell/interface/dependency obstruction
or later quality undoing useful geometry refinement. Do not add another local
variant, larger cavity, scheduler, geometry representation or construction-policy
change from these data. No new full-M6 baseline repeat on diagnostic-only240.

Preserved rejection ring includes edge[7868,7876] at37419,oldcells[227654017,227654022],
shape unmeasured,failure0,EMPTY reason. Source records rejected proposals; it proves
consideration, NOT failed primitive or faithful replay. Current context differs.
Source review: geometry-directed joint split exists, but complete apex/strip searches
are gated differently from metric/height paths. This is a QUESTION, not established
cause. RefitWallSplit explicitly requires nonempty heights; don't reuse it blindly.
After a justified connected production correction and affected serial/ONE MPI6
milestone, immediately complete SAME-seed two-event M6. All downstream goals persist.

## Full237 outcome and selected actual metric mesh

[Full outcome, controls and hypotheses](Handbook/ADAPTATION_BUILD237_OUTCOME_20261107.md).
First native event naturally refused at5347.003818s/round37505:1020222cells/296q/
327J/1366geometry/0size,ALLfinal measurement flags TRUE. First pass exhausted44q/17J,
46shape-union ALL blocked,queue0;40q/all17J have physical faces. Fallback surface
reconstruction reduced geometry1656→1379 then fresh shape entry410q/493J. Dedicated
geometry-only phase NOT reached. Construction intermediate q/J caches are incomplete.
Final430shape-union not all stuck(latest17stuck); all have edge<.5, but clean one-face
cells also often do(54653/65234), so short-edge correlation is not causality.

All30raw files sealed,1519source/runtime/input pins verified before ONE authorized
pidfd SIGTERM of exact launcher2862082/start7510715 after first refusal. Tool2688
terminal1,5633.581992s,pins unchanged,all11owned handles absent. NEVER repeat this stop
or any235/227/225/219/217/210historical stop. R/m6_237_launch.json terminal.

Selected export tool13732terminal0,425.036783s,pins unchanged/allhandles absent;
independent meshio reader tool86406terminal0,5.542227s: all4VTUs load, exact PointData/
CellData lengths,all cache_valid1,finite/symmetric/SPD nodal metrics(min eigen.0025878931).
A237/inspection/rejected_mesh.vtu WITH9-component captured_nodal_metric and
rejected_mesh.su2:198941points/1020222tets.296q/327J/1366g,shapeunion430,badunion1725
exactlyjoin census,maxedge1.7999977268. SU2/VTU coordinates/tets match. ALL1725bad cores
and ALL25856one-node-neighbor cells retained,no omissions; cavities/reference/rejections/
ID maps available. No ParaView GUI check; diagnostic SU2 marker-name fallbacks.
No routine per-trial mesh dumps; latest selected inspectable mesh is237.

Five actual complete contexts prepared42.698837s,1020222rows paired,32968boundary
points/65932faces,hashes unchanged.237compound36470assertions/121.090848s plus only
its skipped controls34392assertions/153.948427s pass.238compound37975assertions/
125.348654s pass,20stars/15alternatives,allpins unchanged/allhandles absent.
R/assess_current{237,238}.py and A237/current_residual{,238}_assessment.json.
Alternatives overlap; no summing gains. Fresh associations/context-only current nodal
field,old q/J exact; captured edge equality unavailable,candidate edges freshly checked.
No historical blocked-tail/nonlocal publication/nonzero-height proof. Controls config
symlink required supplemental49actual-directory pins, verified unchanged; use physical
run.cfg copies for future selected folders so harness receipt lookup follows selection.
Prior tool35421MPIstartup53 did not execute tests; restored previously qualified
OMPI_MCA_osc=pt2pt/private4TBscratch only after terminal/absent handles. Preserve it.

## Fixed CFD/target and acceptance obligations

Saved REAL original-grid CFD R/m6_inputs/euler_original_force_seed_v1:
61330points/303359cells,CL.269535/CD.013536,stable force window,not full residual convergence.
SeedSHA1eaee788bd62bd92a92ec539577dcbcfc0f8042dcbcec23adb04e825bb01a880.
Corrected production L1at50k selected over L2on SAME seed/constraints, shock/corner/
farfield/gradation evidence R/metric_target219_production/selection.json. Do not reselect.
ConfigSHA74b2e9f476cf6ba1fdc5ebcdcb1c34a2b3c45b25e58d651e75417d89402bfe8d;
initialrestartSHA5132ee2a83635b457943c2679a3a50c5e0b733427e620cc748da53865ce5d692
byte-identical219/225/227/235/237. First metric uses saved real CFD; second requires
accepted mesh and continued CFD. Current transported nodal field is not a whole-target
drift norm; repartition must preserve attributes. A237/initial_metric_input_provenance.json.

237signed-residual skip is ONLY production delta since235:116serial/82379assertions,
ONE MPI6all26cases/rank;actual-cavity controls preserved. Matching full-run outcomes
through36800equal235;construction traversal-5.04%,common quality~4.84%less. Wall times
include contention; no isolated causal speedup.236screening rejected/reverted,no slack
cache selected. Timer/rejection scopes overlap; commits/private debt/RSS are not closure.

Require shape closure,shape-preserving residual geometry/size repair; validity-only
reconstruction requires shape/size/height reclosure inside SAME budgets. TWO accepted
native events, true CV mass-solve residual<=1e-10, all projected states/timelevels
admissible,scaled target-integral defect<=1e-12 BEFORE publication. Old/new-domain
integral change is separate. Finite CFD or run_m6 finite-integral helper insufficient.
Actual adapted_mesh_adap_00001/2.su2+native_ref and matching restart_flow_adap files,
exact point order/coords1e-12,finite conserved states,rho/internalenergypositive.
Fresh isolated ADAP_LOOP=NO/RESTART_SOL=YES/READ_BINARY_RESTART=YES,distinct input/
output prefixes, ACTUAL adapted disk load and admissible continuation. Refused-event
restart on original grid is not adapted proof. Resume-cycle overwrite/rejected-cycle
consumption OPEN. Then BL curved/junction/MPI-interface/subsequent-operation/ownership/
requested-height qualification, then corresponding RANS500k. ZERO accepted meshes.

## Operations and records

One owned heavy job,nice19,libs1,build3workers. Check actual HOST RAM/load/disk,
use4TB compile-tmp. Cache-accounted bytes,summed RSS and physical RAM differ. Latest
build host~36.3GiB available,no demonstrated owned swap pressure. Global memory PSI
transients were recorded; host contention makes wall comparisons descriptive.
Fork origin https://github.com/rois1995/SU2.git,branch codex/native-3d-bl,push authorized.
Git metadata outside workspace requires escalation; gc.auto=0,submodule.recurse=false,
exact reviewed whitelists/ignore-submodules=all. Preserve unrelated logs/analysis/.orig/
broken Eigen. No raw meshes/binaries/restarts/logs committed. Literature is design
context,not a competing implementation. See HowToProceed.md, integrated task,
ADAPTATION_GEOMETRY_SCREEN236_20261107.md, ADAPTATION_REPAIR_SUPPORT_20261106.md,
ADAPTATION_METRIC_FLOOR_20261106.md and ADAPTATION_BACKLOG.md for linked detail/history.
