# Mach-only M6 build244 outcome

2026-11-07. First event refused; ZERO accepted meshes. Full goal remains TWO
accepted Euler50k meshes, conservative admissible transfer, finite continued CFD,
actual isolated adapted disk restart; then BL/constraint/MPI and RANS500k.

R=`integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57`;
A=`R/m6_runs/euler_build244_mach_only_real_cfd50k`.
Current owned handles and source state are authoritative in [handoff](../HANDOFF_Codex.md).

## Inputs and terminal evidence

User requested MACH as the sole adaptation sensor. The generator and actual
configuration agree: ADAP_SENSOR=(MACH). Same qualified saved real CFD seed,
original grid/reference, L1 normalization,50k metric complexity, strict geometry1e-4,
q>=.2,J>=.05,maxedge1.8,requested heights and native5400/outer14400 budgets.
Underlying production C++ is unchanged build244/236e03ed64. The metric-only check
passed: complexity49999.999626,finite SPD,zero gradation violations,CFD fields
bit-identical to the prior combined-sensor metric check. The changed sensor set
precludes same-target speed claims against combined MACH+PRESSURE runs.

Construction entry native656.507283:878443cells/1381geometry/0size; shape intentionally
unmeasured. Fresh quality entry701.671352:106454q/106204J/1381geometry/0size,complete.
First event EXIT native5347.578358/round34425: **1032169cells,2035q-bad,967J-bad,
1046geometry-bad,0size-bad**. All cells measured and all census flags complete.
Reason: `Native3D private repair round exhausts elapsed-time budget`.
The first shape pass never closed; subsequent geometry construction/reclosure was
not reached. Passing local placement fixtures did not establish this capability.

A/capture_ready.json seals30files,1032169owned and metric rows,complete reference
and historical rejection records. Metric cache/census is COMPLETE, unlike the
previous combined244terminal capture. All1520source/runtime/input pins verified
unchanged before one pidfdSIGTERM to verified owned MPIlauncher3756595/start9562400.
The first refusal made TWO accepted events impossible; no useful second-event
baseline was pursued. Runner73370terminal1/5590.219278s; pins unchanged. All10sampled
owned handles absent. Never repeat this stop. A/{source_freeze_before_stop,
owned_stop_once,terminal_handles,status}.json records the sequence.

## Counter assessment

Existing six-rank counters joined by exact census serial, including COMPLETE exit;
churn and location/histogram invariants pass. Observation helper was updated only
after full run terminal. No instrumentation or repair decisions changed during it.
A/{quality_progress,quality_counter_assessment,construction_progress_assessment}.json.

- Quality4645.907007s:104419net q/105237net J/335net geometry removed;
  whole-phase22.475482net q/s.83052commits; q retired625636,introduced521217.
- Final partial window59.257s:583net q/345net J removed,1019commits.
  Queue34414;41/2842shape-union cells have all features blocked; missing0,
  private tracked collectives0. These are time-limited residuals, not a fully
  exhausted operator set. Unblocked features do not prove eventual repairability.
- Residual q:281surface/1754without physical face; J:216surface/751without face.
  A cell without a physical face may still share constrained vertices.
- Rank q counts568/6/523/25/425/488; q+J debt max/mean1.671482.
  Whole-phase join-call wall share.418988 is not CPU idle or isolated protocol cost.
  Late ownership evaluations declined migration: predicted gain~165–198s below
  predicted cost~214–223s,including rebuild/warming/uncertainty. Existing counters
  show remaining imbalance; they do not justify a new scheduler or forced migration.
- Geometry upper queries1333597; summed upper4183.204704s, traversal3642.913284s,
  scope preparation413.772205s,facet preparation53.049959s,piece selection34.263024s.
  Traversal1806039360work units,scope12597517,facet1273940,piece117516305.
  Work units differ by operation and are not CPU seconds.
- Boundary smoothing823863candidate positions,31476kernel moves,3622.500379s,
  kernel whole-patch debt reduction1027.559530. Interior481294candidates,60002moves,
  271.350417s,debt reduction6579.055440. These kernel counters do not isolate
  published whole-field gain. Boundary and upper/traversal timings OVERLAP.
- Boundary star context reused2of998072stars. Exact union memo658692hits/674905misses;
  direct238382hits/184457misses.111retention refusals. Hits/misses alone cannot
  distinguish identical repeated misses from nearby trial positions with distinct
  exact keys. Do not broaden cache identity to manufacture reuse.
- All2842shape-union cells have an edge<.9;2704have an edge<.5. Correlation does
  not establish construction causality or authorize quality-aware construction.

## Next target and scope

**Immediate target: speed of useful quality closure.** The time budget expired
while net whole-field q/J removal remained positive and only41shape-union cells
had all features blocked. Expensive boundary/geometry work and rank waiting coexist
with mostly interior residuals. Geometry repair power is ALSO unresolved:1046bad
cells remain and previous actual contexts demonstrated genuine stored-cubic errors.
Neither a larger budget nor more commits is the proposed remedy.

Analyze existing unchanged-face/context reuse, changed-face checks, prepared
geometry/subtrees and valid inexpensive screening first. Use representative actual
Mach-only residual boundary/interior cavities and clean controls. Distinguish exact
repeated queries from nearby trial positions; a motion-tolerance bound needs proof
against unchanged exact support/reference, with full certification on uncertainty.
Sampling is diagnostic, never acceptance. Preserve seams, genuine features, body
separation, association/coordinate invalidation, active-reference/transient-memory
accounting, requested heights and all final gates. CAD remains optional.

Select ONE connected hypothesis from measured traversal savings and net bad-cell
gain. Deferred placement-plus-optimizer coupling is not implemented or selected by
this report. No new scheduler/configuration/cavity-size campaign. After affected
validation, return immediately to full TWO-event M6 with this same Mach-only seed
and unchanged gates/budgets. Transfer,continued adapted CFD,disk restart,BL/MPI and
RANS500k all remain unproved. Construction remains validity-first.

The meaningful terminal export73509passed425.718486s with all1553pins unchanged;
all3sampled export handles absent. A/inspection/rejected_mesh.vtu and .su2 contain
201113points/1032169tets,full captured_nodal_metric(9row-major components in PointData),
measured q/J/geometry/edge CellData. All3841bad cores and142889neighboring cavity
cells are retained with reference and historical rejection records. No per-trial
exports were made. Surface extraction retains this same volume tensor; no separate
surface metric is fabricated.

Independent meshio5.3.5/numpy1.26.4reader16670passed3.711577s:all PointData/CellData
lengths correct,q/J/geometry absent from PointData,tensors finite/symmetric/SPD
(min eigen.002704938038),all metric availability flags true,cell caches complete,
2035q/967J/1046geometry confirmed,exact VTU/SU2 points/tets,output hashes unchanged.
A/inspection_reader_status.json. Meshio assigns numeric tags to diagnostic Marker_0/1/2
names; this is not native boundary-association or strict gate qualification.
No ParaView GUI validation or mesh acceptance is claimed.

Source inventory for the next analysis: SmoothBoundary already prepares its atlas
once per star,applies a projected shape upper screen before MoveVertex,uses fresh
merit and incident-edge checks before guidance,and lazily computes the old guidance
once. GeometryGuidanceExcess stops a nonnegative same-order partial sum above the
rejection threshold; BoundPhysicalSurface memoizes exact authority/support/tolerance/
triangle queries. Those facilities must be reused. Boundary candidate rejections
were23653validity/709144shape/7432edge/37399geometry;118133guidance calls evaluated
655563faces,with34321early guidance rejections. These aggregate counters do not
establish how much candidate reordering,motion slack or changed-face-only reuse
would save. Prior twelve-query motion-slack results were negative; no new cache or
speedup is selected or established by this source review.

## Actual residual and existing combined-operator checks

Preparation41434terminal0/52.295735s,all1309pins unchanged,all3owned handles absent.
R/prepare_mach244_residuals.py selects7actual terminal cores with complete GLOBAL
four-star unions42–54cells within existing64. It reuses the original current-star
preparer through a small input/terminal-schema generalization; no C++ change.
1032169owned/metric rows,33339boundary points/66674physical faces validated.
A/{mach244_coverage_assessment,residual_preparation_status}.json and
current_witnesses_v1/witness_preparation.json preserve hashes and selected IDs.

One initial setup attempt54378failed MPI_Win_create/exit53 before any test: omitted
previously qualified OMPI_MCA_osc=pt2pt. Its pins were unchanged/all4sampled handles
absent; failure preserved in A/current_residual_runtime_failure.json. Qualified
retry restores pt2pt and private4TBscratch,not an algorithm change.

Existing diagnostic31976terminal0/97.991032s,pins unchanged/all5handles absent,
case R/cases/current_residual_20261107T133112_4050059. Exact q/Joracles match all7
current nodal contexts. Four complete single-vertex stars per core:

| Core role | Core ID | Context cells | Useful stars of4 |
|---|---:|---:|---:|
| Severe interior shape |210829569|48|2|
| Near-gate interior q |204870656|49|4|
| Surface shape,geometry-clean core |211800460|42|0|
| Surface shape+geometry |206935582|54|0|
| Surface geometry-only core |209482881|54|1|
| Feature-adjacent shape |211800451|45|1|
| Nearby clean surface core |208808321|43|2|

Useful here requires nonincreasing complete-context q/Jcounts,at least one reduced,
no geometry count/excess regression,and existing metric/replacement/interface checks.
A clean CORE need not have clean neighbors. These candidates are not global
publications or acceptance,and overlapping gains must not be added. The clean-core
control includes one candidate increasing q count while reducing J count; debt/metric
admission alone does not guarantee net q retirement. A/mach244_current_residual_assessment.json.

Existing compound probe87221terminal0/91.806188s,pins unchanged/all5handles absent,
case R/cases/current_compound_20261107T133648_4064110. Same three boundary contexts,
unchanged operators/cavity limits/geometry authority:

- Shape-only context:two of12directed collapses construct. Collapse+retained-star
  optimization produces11q/9J→8q/7Jor9q/8J,with zero geometry excess retained.
- Feature-adjacent context:one of12constructs;combined result8q/7J→5q/4J,
  zero geometry excess retained.
- Shape+geometry context:one collapse and an existing shell/split construct.
  All4combined results regress geometry excess. Shelling gives1q/1J→0q/0J but
  excess4.382556e-12→5.316350e-11;existing subsequent split lowers that to1.831859e-11,
  still worse than original. No acceptable combined candidate is established there.

A/mach244_compound_assessment.json. The diagnostic measures collapse AFTER its
optimizer,not the raw pre-optimization merit. It therefore does NOT prove new
production coupling is necessary. Query/candidate ordering savings also remain
unmeasured. Continue the same geometry-cost analysis: extend the existing work
trace with exact query identity/hit/work/bound data and expose raw-collapse merit
in this diagnostic before choosing a connected production fix. Prior negative
motion-slack results remain valid for their sampled scope; no new cache/config/
scheduler/operator/quality-aware-construction policy is selected.
