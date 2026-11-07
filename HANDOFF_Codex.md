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

Production checkpoint236e03ed64 (launch metadata pushed07272ec108);243/244 introduces the connected
visibility placement change below. Historical production4d1e2ec5b0was frozen237.
238only broadened DIAGNOSTIC current-cavity coverage to non-two-face collapse trials;
239/240only extend opt-in geometry query DIAGNOSTIC with rigorous outside witnesses.
No production operator/config/policy/gate/budget change through240.240source/metadata
pushed; current244production/qualification state below supersedes historical handles.

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
terminal_handles.json authoritative.240is terminal; current244state below governs.

## ACTIVE244 — full same-seed two-event M6 frozen

All241/242 builds and diagnostic cases are TERMINAL0, pins unchanged and owned
handles absent.241tested four existing constructors on the
actual chord; all refused: inherited child inversion, immutable junction, or lack
of retained-skin visibility.242also tested four existing metric-normal damping
proposals; all inverted inherited volume connectivity. Do not enable that shortcut.
Case241...082556_3233611:33458assertions/91.044460s;
case242...084032_3276914:33498assertions/90.933268s.

[Current connected design](Handbook/ADAPTATION_VISIBILITY_PLACEMENT243_20261107.md).
Build243 TERMINAL0 tool21850/363.243376s, all1303pins unchanged, both targets current,
all sampled compiler handles absent. Actual cavity tool80518 TERMINAL0/93.042200s,
pins unchanged/all5sampled handles absent, case current_compound_20261107T091818_3385861.
Native geometry placement succeeds in3trials/1interior move on18complete-star cells.
Whole52→54 context:4q/3J/6geometry→3q/3J/6geometry, excess−74.68%,maxedge unchanged.
Existing optimizer then gives59cells/1q/1J/5geometry,excess−82.97%,strict replacement/
interface and metric admission pass. Eight prior constructor refusals preserved.
Raw GeometryRefine passes, raw Repair correctly refuses. NOT an accepted whole mesh.
A237/visibility243_assessment.json authoritative results/limits.

FULL244 ACTIVE tool82306, source236e03ed64/build244, expected TWO events.
Attempt R/m6_runs/euler_build244_visibility_real_cfd50k. Runner3440151/start8863722;
MPIlauncher3440421/start8864247, outer timeout3440420,6solver workers. Exact HOST
handles R/m6_244_owned_startup.json; live-launch R/m6_244_launch.json. No stop sent.
Native5400/outer14400,6ranks,nice19/libs1,private4TBscratch; freeze ALL source,
fixtures/helpers/binary/config/inputs. Same real CFD seed and corrected-L1/50k:
run.cfg,original mesh/reference and initialrestart byte-identical237, recorded in
attempt/initial_metric_input_provenance.json. One owned heavy workload. ZERO
accepted meshes so far. Use observed outcome; no competing implementation.
Latest joined first-event observation: coarsen round1777/293396cells/2968geometry
and surface round1957/294585cells/3171geometry match237EXACTLY, including upper
calls/traversal work. Shape intentionally unmeasured, all census/geometry/cache
flags complete. Slightly lower elapsed is not work reduction.
attempt/live_stage_observation.json has exact six-rank hashed prefixes.
Fresh first quality entry: native691.245338/871576cells/102043q/101849J/2008geometry/
0size, ALL flags complete.237entry705.18/871607cells/102058q/101860J/1997geometry.
At quality206.046414s:95863q/95724J/2010geometry, net6180q/6125Jremoved. Nearest237
snapshot239.821040s:96237q/96066J/1969geometry; ages differ34s, no exact-time/source-
isolated speedup claim. Geometry count not improved.
attempt/{quality_entry_observation,live_quality_observation}.json preserves joined
rank prefixes. Continue frozen toward outcome, no new implementation.
Latest reused churn checker (analyze_m6244_quality.py): quality395.169929s,
91877q/91928J/1980geometry/0size,ALL flags complete. Retired−introduced exactly
matches net field change. q-removal25.73/s vs23723.98/s at common round34752,
traversal ratio.9484; trajectories/ages differ, no isolated speedup claim.
Actual initial quality ownership preserves all census totals/flags and improves
q+J debt max/min6.575→1.485(max/mean1.757→1.125).
attempt/{quality_comparison237,quality_ownership_observation}.json records scope.

Current244build TERMINAL0 tool10499/168.129617s,1303pins unchanged/both targets
current/all compiler handles absent. Legacy serial_full tool85577 TERMINAL27/305.529924s,344cases/336pass/8fail,
1161529assertions/27fail. Pins unchanged/all5handles absent. Failed broad campaign
is NOT qualification. Known historical donor-count/scenario assumptions documented
in ADAPTATION_GEOMETRY_QUERY_REUSE_20261031.md244–258; two less-documented failures
reviewed: zero-work support returns before changed placement; planar RANS
wall policy consumes existing4096work and refuses, with placement95trials/0moves.
No baseline equivalence proven; retain as downstream BL qualification issue. No old-code baseline or expectation/gate changes.
R/visibility244_broad_serial_failure.json preserves exact failures.
Connected MPI6 tool33555 TERMINAL0/101.983105s,55cases on EVERY rank,
all frozen pins unchanged/all10sampled handles absent. Actual native placement and
whole-neighbor optimized result match on all6ranks; stale/veto/refusal and complete
cell-record publication pass with initially empty owners. Engine selection and
distributed parameter edit are not established by that direct directory check;
existing curved runtime and requested-height cases pass separately.
R/visibility244_qualification.json authoritative scope/provenance. Filter reuses qualified connected groups plus actual curved-star
publication, not the obsolete generic suite. Reuse individually passing affected
serial cases; do not repeat generic broad campaign.
Production unchanged243;
fixture adds actual curved-star stale-dependency/veto/complete-publication check with
all initial owners onrank0 and empty peers. R/run_cases.py placement_mpi6 combines
that actual check and existing connected MPI6 suite. Need source-frozen terminal
build then affected serial and ONE MPI6 milestone before full same-seed M6.
Current UNVALIDATED production changes restore bounded positivity-only
placement inside existing PlaceSplitPatch, which previously tried no movable-apex
positions. Reuse existing convex clipping as a proposal kernel; exact strip,
association, interface, positivity and requested-height admission remain decisive.
Existing engine complete-star lookup/reservation/dependencies/publication now admit
free interior apex support for geometric splits, within the same64-cell/8-apex caps,
existing work/time/memory budgets and existing9-node identity allocation. No q/J
construction measurement, new operator/scheduler/config or relaxed final gate.
Common clipping header is a mechanical extraction; CFD retains a forwarding header.
Existing signed-bound/cache/ownership improvements remain intact.

A bounded floating-point feasibility probe on the actual18-cell apex star suggests
positive children can exist with relocated free apex, but uses projection proxies:
NOT native association/geometry acceptance and NOT an infeasibility proof.
A237/apex242_feasibility_proposals.json records hashes and exact-positive checks.
Next: actual cavity raw/optimized whole-context assessment, affected serial
and ONE MPI6 milestone, then immediately full same-seed two-event M6. Need prove
geometry-triggered multi-apex IDs/dependencies, empty ranks, curved contexts/heights,
cache equivalence and conservative clipping consumers. Zero accepted meshes.

Preserved rejection[7868,7876] at37419 has oldcells[227654017,227654022],shape unmeasured,
failure0,EMPTY reason. Proves consideration, NOT which primitive failed; current
context differs. Geometry-directed joint split exists, but complete apex/strip searches
are gated differently from metric/height paths. This remains a QUESTION,not cause.
RefitWallSplit requires nonempty heights; don't reuse it blindly. No larger cavity,
new operator/scheduler/geometry representation/construction-policy change selected.
After a justified connected production correction and affected serial/ONE MPI6
milestone, immediately full same-seed TWO-event M6. Do not declare fixtures completion.

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
