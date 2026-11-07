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
A_MACH=`R/m6_runs/euler_build244_mach_only_real_cfd50k`(terminal REFUSED).
A237=`R/m6_runs/euler_build237_signed_bounds_real_cfd50k`.

## CURRENT — Mach-only refused; measured mesh saved and independently read

User requested PRESSURE removed: ADAP_SENSOR=(MACH), generator3a6a558e6e.
Last full workflow uses build244/236e03ed64; same saved real CFD seed,L1/50k,
strict gates/native5400/outer14400. Actual metric check passed32.579044s,
complexity49999.999626,finite SPD,0gradation violations; flow fields unchanged.

Full Mach-only attempt tool73370 TERMINAL1/5590.219278s,pins verified unchanged.
First event REFUSED native5347.578358/round34425:1032169cells/2035q/967J/
1046geometry/0size,ALL cells measured and ALL census flags complete. Still first
shape pass: no shape closure or subsequent geometry-construction closure established.
ZERO accepted meshes; no transfer/continued adapted CFD/adapted restart.
A_MACH/capture_ready.json seals ALL30files,1032169owned/metric rows,complete
reference/rejections. Metric census is COMPLETE, unlike combined244final cache.
1520pins verified before ONE pidfdSIGTERM only launcher3756595/start9562400.
A_MACH/{source_freeze_before_stop,owned_stop_once,terminal_handles}.json:all10owned
handles absent. Never repeat this stop or treat old launch metadata as live.

Export73509TERMINAL0/425.718486s,1553pins unchanged;
all3sampled export handles absent. A_MACH/inspection_export_{status,pins,
owned_startup,terminal_handles}.json. Independent meshio reader16670TERMINAL0/
3.711577s:201113points/1032169tets,complete finite SPD nodal tensors(min eigen
.002704938038),correct PointData/CellData lengths,exact VTU/SU2 coordinates/tets,
measured bad counts2035q/967J/1046geometry confirmed.3841bad cells/142889neighboring
cavity cells,all retained. A_MACH/inspection_reader_status.json; no ParaView GUI or
mesh acceptance claim. Latest inspectable files A_MACH/inspection/rejected_mesh.vtu
and .su2; captured_nodal_metric has9row-major components in PointData. The surface
extract retains this volume tensor,not a separately fabricated surface field.
Historical actual-residual31976terminal0/97.991032s,pins unchanged,
case R/cases/current_residual_20261107T133112_4050059,all5handles absent. Exact q/J
oracles match all7contexts. Near-gate interior4/4and severe interior2/4stars yield
useful complete-context q/J retirement; selected shape-only boundary and shape+
geometry boundary each0/4. A_MACH/mach244_current_residual_assessment.json.

Existing compound-operator probe87221terminal0/91.806188s,pins unchanged,all5handles
absent;case R/cases/current_compound_20261107T133648_4064110. Three actual boundary
contexts,existing collapse/shell+optimization only. Shape-only boundary2useful
candidates (11q/9J→8q/7Jor9q/8J,no geometry debt);feature-adjacent8q/7J→5q/4J,
no geometry debt. Shape+geometry candidate can remove q/Jbut ALL4combined variants
increase geometry excess and remain inadmissible. Raw-collapse merit BEFORE its
optimizer was not measured,so necessity of new production coupling is UNPROVED.
A_MACH/mach244_compound_assessment.json. No production operator/scheduler change.

Earlier setup-only54378terminal53/MPI_Win_create was before tests: omitted qualified
OMPI_MCA_osc=pt2pt. Preserved failure/all4handles absent; subsequent qualified retry
uses pt2pt/private4TBscratch. Always reuse these runtime settings for singleton
native diagnostics. A_MACH/current_residual_runtime_failure.json.

R/prepare_mach244_residuals.py selects7actual cores with complete GLOBAL four-star
unions42–54cells within existing64: severe/near-gate interior,surface shape with/
without geometry debt,geometry-only,feature-adjacent,nearby clean control;no omissions.
Reuses generalized prepare_current237_witnesses.prepare;legacy entry retained.
Preparation41434terminal0/52.295735s,1309pins unchanged,all3handles absent;
1032169paired rows,33339boundary points/66674physical faces validated.
A_MACH/{mach244_coverage_assessment,residual_preparation_status}.json,
current_witnesses_v1/witness_preparation.json and compound_probe_v1/witness_preparation.json.
Current final contexts are NOT historical replay; gains overlap,not additive.

ACTIVE connected implementation: canonical SINGLE-triangle numeric query reuse,
implemented in build247. [Design/evidence](Handbook/ADAPTATION_QUERY_CANONICALIZATION246_20261107.md).
Both key AND computation normalize exact corner bits; exact authority/support/
tolerance and all association/feature/body/height/final gates remain. Nine charged
work units; admitted canonical scratch/key overlap; tight admission uses full query.
No motion reuse/collapse coupling or construction changes alongside this cycle.

Latest build247TERMINAL0/42.102124s,1303pins unchanged,both targets current,
all5sampled handles absent. Build246serial failure preserved:two fixture contracts
(unsorted support throws,and exact work includes9new units),not relaxed gates.
Build247serial TERMINAL0/62.274139s:38cases/1678assertions pass,
R/cases/canonical_serial_20261107T143317_2642,all5handles absent.
Actual247probeTERMINAL0/92.804422s,pins unchanged,all5handles absent,
R/cases/current_compound_20261107T143840_16331. Same candidate inventory and all
q/J/geometry bad counts;counted query work681926→631308(−7.4228%). No whole-M6
speedup established. A_MACH/canonical247_actual_comparison.json.

Build247 placement_mpi6 TERMINAL0/165.014869s:90cases on EVERY rank,
pins unchanged,all10owned handles absent. R/canonical247_qualification.json;
R/cases/placement_mpi6_20261107T145231_48860/{status,owned_startup,terminal_handles}.json.
Tool session52411is terminal. The full M6 below is the sole owned heavy job.
Main harness now preserves requested config directory for companion pinning even
when run.cfg is a symlink. Three earlier subset probes have explicit supplemental
PRELAUNCH actual-input hashes in A_MACH/actual_companion_input_provenance.json;
original harness did not cover subset TSV. Actual raw payloads were pinned.
MPI input reuse pins exact original producer snapshot and byte-identical previously
qualified geometry fixture;not a CFD baseline or production target reselection.
Qualified checkpoint f7855ed7ec PUSHED to origin/codex/native-3d-bl.
Full TWO-event M6 NOW LIVE:tool session14272,
R/m6_runs/euler_build247_canonical_mach_only_real_cfd50k (A247).
Runner58992/start10785929;launcher59206/start10786445,MPI59208/start10786446,
ranks59229–59234/start10786457–58. Exact10owned identities in A247/owned_startup.json;
R/m6_247_canonical_launch.json records host/resources/scratch/source commit.
Same saved real CFD seed,MACH-only,L1/50k,native5400/outer14400;run.cfg byte-identical
to Mach-only244(sha5ad2dc1d9a1cd8bff87f633617b191b50defbc261ab3d0b2ed0987a34722b89e).
Trace UNSET;pt2pt/private4TBscratch/nice19/libs1. Selected bounded raw refusal capture
retained;no routine VTU exports. Frozen C++/fixtures/helpers/binary/config/inputs.
Compare complete-census whole-field q/J/geometry and counted work at matching stages;
allow run to finish unless preserved evidence justifies an authorized owned stop.
Latest COMPLETE247quality_entry:native689.582599/round31898,
878443cells/106454q/106204J/1381geometry/0size,ALL measured/flags complete.
Counts EXACTLY match Mach-only244quality_entry(native701.671352).
Construction upper_work282283657→274601233,traversal263600528→256967738;
120785accepted local operations in both runs,not accepted meshes. A247/
{construction_progress_assessment,comparison244_matching_stages,quality_entry_comparison244}.json
preserve exact prefix hashes/scope. Quality closure is now active;no full-run speedup
or accepted mesh is established. No new implementation while the run is frozen.
Latest COMPLETE quality window32064/all-rank census7:native1073.575691,
895674cells/94913q/94850J/1299geometry/0size,all flags complete. Whole-field
counts match244at that round. Quality383.993092s versus418.426252s;
net11541q/11354J/82geometry removed,30.055228q/s versus27.581921q/s.
Quality upper_work207185753→174312520,traversal196356133→164853898,about16%less
counted work for matching field progress. Early wall rates remain contention-sensitive;
no full-run speedup or acceptance established. A247/latest_matched_quality_census.json.
The first three-window service/churn comparison is A247/quality_comparison244.json.
Existing analysis helper was reused with ALL literal attempt-directory occurrences
replaced;first ad-hoc wrapper had replaced only one and produced old244quality
records. Invalid derived hashes and correction are explicit in A247/
analysis247_scope_correction.json;those invalid records were never reported/pushed.
Corrected window set/counts are asserted against current complete stage groups.
No production/pinned source,input or repair decisions changed.
ZERO accepted meshes;all downstream obligations open.

Diagnostic245baseline remains historical,traces/raw-control only:
Traced actual probe68907terminal0/92.918482s,pins unchanged,
R/cases/current_compound_20261107T135435_4105949. Runner and all4process IDs sampled
during execution absent;no pre-exit start-tick snapshot,no stop sent. Exact scope in
case/terminal_handles.json. Same3actual boundary contexts and current nodal field.
A_MACH/{current_query245_attempt,mach245_query_identity_assessment,
mach245_raw_collapse_assessment}.json. R/analyze_mach245_queries.py self-check passes.
1907complete queries,0unresolved:1516identical repeats ALLhits,391misses603099work.
64vertex-permutation aliases consume73450work(12.178763%of miss work in this corpus).
127nearby same-support queries have exact rational numeric margin;45181work(7.49%).
These are bounded local opportunities,not measured M6speedups or association proofs.
Raw collapse for core211800460/retained279303589:11q/9J→13q/13J fails Repair;
combined retained-star optimizer→8q/7J with geometry0 and all local checks. Another
raw7q/7J becomes9q/8J after optimizer,so blanket coupling is unjustified. Safe MPI
retained-star reservation/closure remains open;recorded backlog,not competing work.
Repair/construction decisions are unchanged by245; observation helper updated AFTER full run
terminal to join the first COMPLETE exit census as well as windows. Churn/histogram/
rank/census assertions pass. Use R/analyze_m6244_mach_only.py,not the hard-coded
combined-target analyze_m6244_quality.py. Final quality metadata in
A_MACH/{quality_progress,quality_counter_assessment,construction_progress_assessment}.json.

Quality4645.907007s:net104419q/105237J/335geometry removed,22.475482net q/s;
83052commits,q retired625636/introduction521217. Terminal41/2842shape-union cells
all-features blocked,queue34414,missing0,private tracked collectives0. Most residual
q-bad1754/2035have no physical face (not necessarily free vertices). Rank q counts
568/6/523/25/425/488. Join-call wall share.418988 is NOT CPU idle.
Geometry traversal3642.913284summed rank seconds,scope build413.772205,
upper4183.204704; these timers OVERLAP boundary smoothing3622.500379.
Boundary823863candidates/31476kernel moves versus interior481294/60002;
interior271.350417s. Kernel moves/debt are not global published bad-cell gain.
Exact union memo658692hits/674905misses; these do NOT distinguish identical query
misses from nearby trial positions. Star-context reuse2/998072boundary stars.
Late redistribution was evaluated and declined: estimated gain~165–198s below
estimated cost~214–223s. Do not invent a scheduler or blindly force migration.

NEXT active cycle: analysis FIRST, selecting cheaper geometry-query/smoothing work
from these counters and actual residual contexts. Speed is the immediate measured
blocker (time exhausted while most bad cells still had unblocked features); geometry
repair power also remains open. Inspect existing reuse/screening/prepared support
before ONE justified connected fix. Deferred coupled-candidate hypothesis is not
implemented or automatically selected. Preserve exact association/invalidation,
seams/features/body/height/resource/final gates. Unproven ideas stay backlog.
Then return immediately to full TWO-event M6/MACH-only/same seed/L1/50k/unchanged
budgets. No same-target speed claim against older combined MACH+PRESSURE runs.
Latest sensor preference also applies to future M6 RANS500k configuration.
[Mach-only terminal outcome](Handbook/ADAPTATION_MACH_ONLY244_OUTCOME_20261107.md).

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

The Mach-only attempt has now refused; its terminal outcome above supersedes launch
plans. Future attempts need fresh helper/config/runtime pins. Earlier combined-target
pins intentionally record the previous generator.

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

Latest meaningful saved mesh is A_MACH/inspection/rejected_mesh.vtu and .su2:
201113points/1032169tets,complete captured_nodal_metric9row-major,all measured
2035q/967J/1046geometry/0size.3841bad cores/142889neighboring cavities,reference and
historical rejection records retained; independent tensor/array/count/connectivity
checks passed. No ParaView GUI or strict whole-mesh acceptance qualification.
Previous combined244has UNKNOWN runtime caches; older237is separately measured.
Save only meaningful terminal meshes,never every trial.

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
