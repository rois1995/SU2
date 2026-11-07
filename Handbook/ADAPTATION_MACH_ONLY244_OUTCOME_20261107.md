# Mach-only M6 build244 outcome

2026-11-07. First event refused; ZERO accepted meshes. Full goal remains TWO
accepted Euler50k meshes, conservative admissible transfer, finite continued CFD,
actual isolated adapted disk restart; then BL/constraint/MPI and RANS500k.

R=`integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57`;
A=`R/m6_runs/euler_build244_mach_only_real_cfd50k`.
Live export handles and freeze are authoritative in [handoff](../HANDOFF_Codex.md).

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

The meaningful terminal rejected-mesh export is underway using the existing bounded
exporter: mesh,full nodal tensor,measured cell fields,bad cells,neighboring cavities,
reference and historical rejection records. Its live state is in the handoff;
this paragraph does not claim export or independent reader success.
