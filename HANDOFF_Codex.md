# Native adaptation — current handoff

Updated2026-11-07(Europe/Rome). Goal ACTIVE: TWO strictly accepted ONERA M6 Euler
CFD→mesh→CFD→mesh at50k METRIC COMPLEXITY, conservative admissible transfer, finite
continued adapted CFD and ACTUAL isolated adapted disk restart. Then dedicated
BL/constraint/MPI qualification and corresponding RANS500k. ZERO accepted meshes.
Actual point/cell counts are separate from metric complexity. Fixtures are not completion.

Geometry1e-4,q>=.2,J>=.05,maxedge1.8,requested heights remain strict. Construction
validity-first: exact positive/nondegenerate/topology/body/feature/height validity;
NO fresh q/J measurement/ranking/veto. CAD optional; no-CAD authority sufficient.
ONE connected cycle/ONE owned heavy job. Freeze compiled/executing/pinned C++,
fixtures/helpers/binary/config/inputs. Verify exact HOST handles before edits/restarts.
Preserve evidence before authorized stop/fix/retry; never repeat historical stops.

R=`integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57`.
A244=`R/m6_runs/euler_build244_visibility_real_cfd50k`(completed combined sensor).
A_MACH=`R/m6_runs/euler_build244_mach_only_real_cfd50k`(LIVE).
A237=`R/m6_runs/euler_build237_signed_bounds_real_cfd50k`.

## ACTIVE — Mach-only full TWO-event M6, frozen

User requested PRESSURE removed from adaptation sensors: ADAP_SENSOR=(MACH).
Generator/config committed3a6a558e6e; underlying C++ remains build244/236e03ed64.
Same qualified real CFD seed,L1/50k,gates/native5400/outer14400 unchanged.
Actual Mach-only production target passed32.579044s,complexity49999.999626,
finite SPD,0gradation violations; CFD fields/coords bit-identical to combined L1.

Full Mach-only M6 ACTIVE tool73370. Runner3756305/start9561866;
MPIlauncher3756595/start9562400,timeout3756594,6solver workers.
R/m6_244_mach_only_{launch,owned_startup}.json exact HOST PID/start/argv/cwd.
No stop sent. ONE heavy job,nice19/libs1/private4TBscratch. Freeze ALL current
source/helpers/binary/config/inputs. A_MACH/initial_input_provenance.json proves
actual run.cfg EXACTLY matches prepared Mach-only variant; original mesh/reference/
CFD seed byte-identical completed244. No accepted mesh yet; let this attempt drive
next work. No competing quality implementation. Latest target input preference
also applies when preparing the later M6 RANS500k configuration.

IMPORTANT: analyze_m6244_quality.py is hard-coded to the COMPLETED COMBINED case,
not A_MACH. Reuse its parser only with the explicit new path for live Mach telemetry.
Combined-target timing/work comparisons are not same-target Mach-only benchmarks.
Never repeat the old244stop or signal historical PID3440421.
Use R/analyze_m6244_mach_only.py for live first-event stage/churn/location/ladder
analysis; it reuses existing exact-census assertions with NO combined-target
benchmark. Latest observation: actual Hessian recovery61330point-sensor pairs,0WLS fallbacks.
Construction entry round31898/native656.507283:878443cells/1381geometry/0size,
shape UNMEASURED. Fresh quality entry native701.671352:106454q/106204J/1381geometry/
0size,ALL flags complete. Target differs from combined244, so no same-target speed
claim. Initial quality ownership preserves ALL census totals/flags; joined data in
A_MACH/quality_entry_observation.json. Sampled tree RSS about9.4GiB,not physical/
whole-job peak. Keep frozen; no accepted mesh yet.
A_MACH/owned_live_observation.json is current exact live handle confirmation.
Mach-only quality is active. Latest complete window round32640/native2545.211770:
951333cells/57709q/56150J/1187geometry/0size,ALL flags complete,
stuck_shape0/missing_features0/private_tracked_collectives0.
Quality age1843.540418s:net48745q/50054J/194geometry removed; net q26.440972/s.
27110commits, q retired386563/introduction337818; churn matches whole-field change.
52199/57709q-bad have no physical face (not necessarily free vertices).
Current debt max/mean1.096358; unchanged migration preserves all flags/totals.
HOST launcher identity reverified live; MemAvailable30089772kB,load6.65.
All16fixed metric samples supported; maximum relative Frobenius drift0.060889993
at recent windows, not a whole-field norm or pointwise frozen-target claim.
First-pass churn checks match net field changes. No combined-target benchmark.
A_MACH/{quality_progress,quality_counter_assessment}.json records work/service/
location/ladder data; continued ownership commits preserve all flags/totals.

## Preserved combined244 outcome and Mach-only preparation

Full244 first event REFUSED native5347.387821/round37843. Final metric cache is
UNKNOWN:1018217cells/measured0,ALL flags false; printed zero bad counts are NOT clean.
Last complete first quality_exit5321.382052:42q/18J/1717geometry/0size,44shape-union
ALL blocked/queue0/missing0. Late geometry construction reduces geometry1717→1418,
then fresh quality_entry5333.173759has414q/472J/1418geometry/0size. New placement
path actually tried48trials/3moves/3validated (not proof of3published commits).
It is available only in non-repairing construction; coupled quality-candidate
hypothesis is deferred in backlog, NOT an implementation.

All30shards sealed,1018217owned and metric rows,reference/rejections complete;
metric CACHE incomplete. A244/capture_ready.json distinguishes file completeness
from metric-census completeness.1520pins verified unchanged before ONE pidfdSIGTERM
only to verified launcher3440421/start8864247. Tool82306 TERMINAL1/5613.368135s,
pins unchanged; ALL10sampled host handles absent. A244/{owned_stop_once,
source_freeze_before_stop,terminal_handles}.json. Never repeat this stop.
ZERO accepted meshes; transfer/continued adapted CFD/restart remain open.

Existing bounded full-mesh exporter tool1334 TERMINAL0/367.604136s,1553pins unchanged,
all sampled handles absent. Reader tool62958 TERMINAL0/5.104009s:198598points/
1018217tets, complete finite SPD nodal tensors(min eigen.0025878931), exact PointData/
CellData lengths and SU2/VTU match. A244/inspection/rejected_mesh.vtu and .su2 saved.
Runtime q/J/geometry caches remain NaN/UNKNOWN; empty bad/cavity subsets are not clean.
A244/inspection_reader_status.json. No ParaView GUI qualification or acceptance.
[244outcome](Handbook/ADAPTATION_BUILD244_OUTCOME_20261107.md) details phase evidence.

Latest USER STEERING: MACH is the sole adaptation sensor for future M6 workflow.
R/m6_inputs/euler_mach_only50k_v1/run.cfg plus provenance.json/generator_check.json record
ONLY ADAP_SENSOR=(MACH),same saved real CFD seed/L1/50k/gates/budgets. run_m6.euler_config
is UPDATED after export terminal and verified to generate this config exactly.
Qualified seed and preserved244inputs unchanged. Build244C++unchanged; no rebuild
needed for this input change. Combined-target comparisons are not same-target
benchmarks for Mach-only.

Actual production Mach-only L1 metric check tool18760 TERMINAL0/32.579044s,
1509pins unchanged at terminal/all sampled handles absent. Same production
ComputeMetric and one CFD continuation from same saved seed, no remeshing.
R/metric_target244_mach_only/{scope,comparison}.json:complexity49999.999626,
finite SPD tensors,0gradation violations(maxratio1.000000100<1.00001),maxAR40.706
versus combined38.259. CFD fields and coordinates bit-identical to combined L1.
The helper's expected sensor directive self-check was corrected AFTER this check
terminal; underlying metric/config/CFD unchanged. Existing runner self-check and
exact generated Mach-only config now pass; build244C++ source pins unchanged.

NEXT: continue the frozen TWO-event M6 with build244/MACH-only/L1/50k and same CFD seed/gates/
native5400/outer14400. No competing quality implementation before changed-target
outcome. Deferred coupled-candidate hypothesis stays backlog. New attempt must
have fresh helper/config/runtime pins; older combined-target pins intentionally
record the earlier generator. Freeze current sources after launch.

R/analyze_m6244_quality.py reuses stage/churn/exact-census joins, stopping at first
native event. A244/{construction_progress_assessment,quality_comparison237,
quality_counter_assessment}.json. First-pass net q/J counters match field change.
Bulk useful rate remained near237, not a proven speedup; final counts UNKNOWN.
Resource observations are sampled summed process RSS, not physical/whole-job peak.

## Connected placement change and qualification

[Design, actual evidence and limits](Handbook/ADAPTATION_VISIBILITY_PLACEMENT243_20261107.md).
240proved real stored-cubic violations of chord7868–7876: certified lower248.334µm
and112.330µm exceed100µm. Cheaper certification alone cannot fix these faces.
241all four existing constructors refused;242all four normal-displacement proposals
inverted cells. Do not enable that shortcut. Private PlaceSplitPatch accepted apex
inputs but only tried no-move construction; current fix adds bounded positive-
visibility proposals for free interior apices, reusing existing convex clipping.
Exact strip/association/interface/body/feature/height checks remain decisive. Existing
complete-star lookup/reservation/version checks/publication now support geometric
splits, within64cells/8apices and existing work/time/memory budgets. Reuse9reserved
construction node IDs for split+up to8movers. No quality-aware construction, new
operator/scheduler/config/cavity cap or geometry representation. Signed-bound/cache/
ownership improvements retained. Clipping header is a mechanical Common extraction,
CFD forwarding include retained; source body identical after include-path edit.

Actual243 native projection constructs18→20cell star in3trials/1interior move.
Whole52→54context:4q/3J/6geometry→3q/3J/6geometry; excess−74.68%,maxedge unchanged.
Existing optimizer gives59cells/1q/1J/5geometry,excess−82.97%; replacement/interface/
metric admission pass. Eight old constructor refusals preserved. Raw GeometryRefine
passes; raw Repair correctly refuses. A237/visibility243_assessment.json. Local result,
NOT whole-mesh acceptance or historical replay. Only one mover/no requested heights.

Build243363.243376s/build244168.129617s TERMINAL0,both targets current,1303pins unchanged,
all compiler handles absent.244production identical243; only distributed fixture added.
Actual serial cavity93.042200s TERMINAL0/pins unchanged/handles absent. Connected MPI6
milestone101.983105s TERMINAL0:55cases on EVERY rank,pins unchanged/all10handles absent.
Actual directory check covers initially empty owners, stale/veto refusal and complete
cell publication; native result agrees all6ranks. It does NOT prove Engine selection
or commit distributed parameter edits; existing curved runtime/height tests pass
separately. Full244must establish usefulness. R/visibility244_qualification.json.

A mistakenly selected legacy serial_full FAILED:305.529924s,344cases/336pass/8fail,
1161529assertions/27fail,pins unchanged/all handles absent. Do not repeat generic suite;
reuse individually passing affected cases. Historical warning in
[geometry-query handbook](Handbook/ADAPTATION_GEOMETRY_QUERY_REUSE_20261031.md)244–258.
No old-code baseline or weakened assertion/gate. Donor/scenario-count assumptions are
historically documented; zero-work support returns before changed placement. RANS
wall-policy fixture exhausts4096work,95placement trials/0moves; attribution is NOT
established, baseline equivalence NOT claimed. Preserve as required downstream BL
qualification issue. R/visibility244_broad_serial_failure.json exact failures.

## Seed, previous full outcome and saved inspectable mesh

R/m6_inputs/euler_original_force_seed_v1:61330points/303359cells,M.8395/AOA3.06,
CL.269535/CD.013536 stable force window,user accepted reasonable forces, not full
residual convergence. ITER1continues this REAL saved solution, not startupCL.059.
Corrected219local Hessian floor/log normalization/flat sensors qualified40cases/
207605assertions,MPI6every4cases. Same-seed production L1/L2 comparison at50k selected
L1; L2coarsened4/11shock points,tip allocation6.44→13.59%,maxAR38.26→199.62.
R/metric_target219_production/{selection,comparison}.json; do not reselect/reseed.
Fixed16sample drift is not a whole-target norm; retain nodal transport semantics.

[Full237outcome](Handbook/ADAPTATION_BUILD237_OUTCOME_20261107.md):first event refused
native5347.003818,1020222cells/296q/327J/1366geometry/0size,ALL flags complete.
First shape pass exhausted44q/17J/1656geometry,46shape-union all blocked/queue0.
Geometry reconstruction reduced geometry but fresh shape became410q/493J; reclosure
ran out of budget. Actual geometry/volume construction obstruction selected after
[240exact face diagnosis](Handbook/ADAPTATION_CAVITY_GEOMETRY240_20261107.md), not from
literature alone. Complete final context is not faithful replay of earlier blocked tail.

Latest meaningful saved rejected mesh is A244/inspection/rejected_mesh.vtu and .su2:
198598points/1018217tets, full captured_nodal_metric9row-major. Independent reader
passed, tensor/arrays/connectivity verified. Native q/J/geometry cache unknown; do
not infer clean cells from empty bad/cavity views. Previous237inspection remains the
latest fully MEASURED rejected mesh:198941points/1020222tets,296q/327J/1366geometry,
all1725bad cores/25856neighbors, finite SPD metric/array lengths checked. No ParaView
GUI qualification. Save only meaningful terminal meshes, never every trial.

## Remaining acceptance obligations — all open

Shape closure, strict residual geometry/size closure and required shape reclosure
must fit unchanged budgets. Two accepted WHOLE meshes on all ranks, not private
proposals, balanced weights, lower memory or passing fixtures.

True CV mass-solve residual<=1e-10 for every field/time level; all projected states
admissible; scaled TARGET integral defect<=1e-12 BEFORE publication. Printed old/new
DOMAIN integral change is separate. CConservativeTransfer.cpp997–1039 checks before
arrays set; PrintSummary1420+ reports true per-field residual. Finite-CFD helper alone
is insufficient.

Actual adapted_mesh_adap_00001/2.su2+.native_ref and matching
restart_flow_adap_00001/2.dat, exact point order/coordinates1e-12, finite conservative
states,rho/internal energy positive. Refused driver may write an adap-named restart
on ORIGINAL grid; not adapted proof. Fresh ISOLATED adapted disk load with ADAP_LOOP=NO,
RESTART=YES,READ_BINARY=YES and distinct input/output prefixes, finite admissible
continuation. Resume-cycle0overwrite/rejected-event consumption remain open issues;
isolated restart does not claim those fixed. Then dedicated BL/curved/junction/
interfaces/subsequent-ops/ownership/heights MPI qualification, then RANS500k workflow.

## Working rules and linked records

[How to proceed](HowToProceed.md),[backlog](Handbook/ADAPTATION_BACKLOG.md),
[integrated task](Handbook/ADAPTATION_INTEGRATED_NEXT_TASK.md)(historical155numbers
are superseded),[repair-support references](Handbook/ADAPTATION_REPAIR_SUPPORT_20261106.md).
Literature is hypotheses, not gate equivalence or authorization for a new scheduler,
larger-cavity campaign, quality-aware construction or geometry representation.
[Prior detailed handoff archive](Handbook/History/HANDOFF_Codex_before_compact244_20261107.md)
is historical, NOT LIVE.

Origin github.com/rois1995/SU2.git,branch codex/native-3d-bl. User authorizes fork pushes
with EXACT whitelists; preserve unrelated dirty logs/.orig/untracked work. Git metadata
outside workspace at SU2_AdapNoExt/.git requires escalation; use gc.auto=0 and
submodule.recurse=false,ignore-submodules=all. Never broad stage raw meshes/binaries/
restarts/logs. Build scratch on4TB,build3workers,nice19/libs1,one heavy job. Verify actual
HOST owned handle terminal before edits/restarts; observations/timeouts/stale logs are
not termination. No extra perf/GDB or old-code baseline campaign.
