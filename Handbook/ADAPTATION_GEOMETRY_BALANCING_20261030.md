# Geometry-aware initial adaptation ownership

Current implementation work, 2026-10-30. **No accepted M6 loop yet.**

Build157is now stopped and execution paused for a user-requested context reset.
The historical155observations below are baseline evidence, not a live run. Read
the [current stop handoff](ADAPTATION_CHECKPOINT_20261030_STOP.md) and
[verbatim integrated next task](ADAPTATION_INTEGRATED_NEXT_TASK.md).

## Baseline and objective

The endorsed Euler target is 50,000 metric complexity; 500,000 is reserved for
RANS. Gates and budgets remain unchanged: geometry tolerance 1e-4, q>=.2,
J>=.05, maximum metric edge 1.8, 5,400 seconds/native event and 14,400 seconds
for two events. These targets are not promises of output point/cell counts.

The user requested stopping build151 to implement geometry-aware initial MPI
balancing. Only its verified mpirun was signaled; all nine recorded handles are
terminal and runtime/source pins verified unchanged. Its 3,216-second partial
attempt is baseline evidence, not a certification failure or accepted result.
Receipts: `staged57/m6_runs/euler_build151_target50k/{observation_stop,terminal_handles,status}.json`.
Full baseline: `staged57/build151_50k_baseline.json` under dc17_18_completion_v1.

| Completed stage | Engine seconds | Global cells | Size-bad cells | Size hinge squared | Atlas max/min |
|---|---:|---:|---:|---:|---:|
| Coarsening pass | 2272.87 | 292712 | 68366 | 55550.19 | 18.70 |
| Split generation 1 | 2504.79 | 301143 | 69092 | 53090.37 | 29.05 |
| Split generation 2 | 2703.84 | 309750 | 69815 | 50821.95 | 39.31 |
| Split generation 3 | 2945.83 | 318389 | 70544 | 48696.57 | 44.00 |

Size-bad counts rose while hinge debt fell: count alone is not convergence.
Rank 0 alone understated the remaining global work. The final mesh was private;
quality improvement, acceptance and transfer had not been reached.

A five-second, 1,086-sample profile had zero lost samples and approximately 79%
of aggregate cycles in collective waiting. Rank 5 accounted for approximately
13.1% in surface-union work, including 10.1% in triangle-union construction.
Inclusive percentages overlap; this is a window, not a whole-run decomposition.
Its pool retained 3,208,249,152/3,208,249,344 bytes, 8,029 facets and 1,190 scopes.
Saturation does not establish repeated-scope locality or an eviction benefit.

## Connected implementation

Keep existing metric `WorkWeights` initially. Compute a separate geometry cost
using the same actual-point nodal target samples, without another donor query,
candidate certification, q/J screening or eagerly constructed fine tessellation.
The existing physical-face ownership assigns a boundary face to one owned cell.
Edge operations are apportioned by physical-face incidence; vertex operations
are apportioned by the replicated accepted boundary vertex-star incidence.
Patch checks scale with that star size. Ghost copies do not receive extra work.

The estimator distinguishes current direct chart support from current cross-chart
queries and predicts possible crossings in later boundary edits. Inputs include
supporting original facets, cheap cubic control-net deviation versus unchanged
geometry tolerance, physical/chart size, protected-edge incidence, target edge
insertion/collapse demand and already observed prepared-piece counts. It predicts
operation demand separately from structural cost per operation. Surface-star size
is a proxy for volume-cavity cost, not an exact complete-volume valence.

Four-way quadratic flatness scaling and n log(n) preparation work are structural
heuristics with unit coefficients, **not calibrated seconds or an implementation
of the paper's formula**. The prediction's piece/span dynamic ranges are capped;
actual certification work, tolerance, admission and tessellation are unchanged.
Prepared-piece observations are reduced by original facet across ranks before
weighting. The structural-only estimator does not read local cache history into
its cost. Actual caches stay local; no destination cache-hit benefit is assumed.

Geometry cost is linearly scaled by a common global maximum to integer 0..1024;
zero-cost interiors remain zero, positive costs round up. Record the scale and
raw work. Small costs can quantize together when an outlier dominates: inspect
the actual distribution rather than claiming exact preservation of ratios.
The partitioner uses independent metric, cell-count and geometry constraints
when global geometry work is positive; otherwise it retains two constraints.
Vendored METIS permits zero individual components; ParMETIS normalizes by the
positive global component total. Per-weight/index totals and storage are checked.

The existing triangular-face dual graph supplies locality and existing atomic
migration preserves IDs, connectivity, versions, physical bindings and nodal
metrics. Original donors follow matching destinations. CFD ownership remains
distinct from private adaptation ownership. Directory and engine state are built
after migration. **This change supplies initial balancing only**, not arbitrary
in-event migration. Geometry caches stay local and retain their existing limits.
Spreading geometry work can increase aggregate cache residency; sample it.

## Measurements and qualification

Emit compact per-rank partition rows with predicted metric/geometry loads and
cell counts before/after, raw operation/work estimates, quantization scale,
migration time, graph cut and accounted memory. Existing resource receipts sample
aggregate process-tree RSS; this is not a measured exclusive-memory peak.

At engine stage boundaries record actual geometry trials/refusals, direct/union
query counts, useful query elapsed/work, scope attempts/build time/work, facet and
scope hits/misses, retention refusals and pool occupancy. Scope construction time
is nested in union time: do not add both. Per-face conservative upper-bound refusal
is distinct from an operator refusal and from a proven outside witness.
Failure-election collective elapsed was previously untracked by World; record it
separately from existing exchanges/reductions. These times include communication
and waiting, not pure idle time. No new collectives enter geometry query paths.

Compile and run focused estimator/cache/migration checks, then the complete same
Euler case. Compare actual rank query work/time and collective elapsed at the
same construction/quality milestones, including global debt and mesh size:
partitioning can change operator order and the resulting private mesh. Retain
the existing profile rather than launching another old-code M6 campaign.
Improved predicted balance is diagnostic. Completion still requires two strictly
accepted meshes, conservative admissible transfer, CFD continuation and disk
restart; then verify BL generation and pursue RANS under its 500k target.

Current validation: production build153 passes (132.295s, all1295 pins unchanged).
Build152's missing constant include is corrected; its625.903s failed build and
terminal-handle receipt remain saved. Focused serial153/154 each pass17/18 cases;
the new estimator fixture first omitted mobility policy, then supplied a duplicate
reference instead of the authority's actual shared object. Both setup errors are
corrected without production changes. Build154 passes69.202s; build155 passes66.199s.
Corrected estimator155 passes42assertions/1case in6.970s. Six-rank connected155
passes13cases/rank (1132/1322/1762/1683/2946/1337assertions),7.039s, pins unchanged.
In the clustered-cost fixture, initial cells and metric work are equal per rank;
maximum geometry/mean improves2.0->1.02778. Final cells206..222 (mean216),
metric509..555 (mean540), geometry1648..1776 (mean1728), cut278/arcs4752.
This is focused qualification, not a completed adapted mesh.

The six-rank build155 M6 attempt is terminal after the user requested the new
coordination work immediately. Only the verified owned launcher was signaled;
all nine recorded handles are gone and pins verified. It ended at3798.03s with
no accepted mesh or transfer. See `staged57/build155_m6_summary.json` and the
attempt's stop, terminal and resource receipts.

### Observed target progress, still private construction

The initial M6 partition reduces predicted geometry max/mean2.591->1.143;
metric max/mean1.095 and cell max/mean1.079. Estimation max0.367s,
partition0.193s, migration0.261s,113113moved cells, cut7470/arcs1164482.
Global quantization rounding adds4.39% relative to the rounded total;34529/48684
positive cells have weight1. This does not prove actual cost prediction accuracy.

| Same completed stage | Baseline engine seconds | New engine seconds | Old/new time |
|---|---:|---:|---:|
| Coarsening pass | 2272.87 | 1180.13 | 1.926 |
| Split generation1 | 2504.79 | 1304.17 | 1.921 |
| Split generation2 | 2703.84 | 1420.47 | 1.903 |
| Split generation3 | 2945.83 | 1562.03 | 1.886 |

Coarsening leaves292694cells versus292712baseline; size hinge55524.19 versus
55550.19. At generation3 the counts are318372 versus318389, hinge48758.19
versus48696.57. These are comparable private states, with changed operator order;
host contention and observation overhead prevent a controlled speedup claim.
Actual local geometry elapsed max/mean is1.281 at coarsening and1.241 at gen3;
coarsening atlas-work max/min falls18.70->3.34. The separate5s profile has66.05%
Allreduce cycles versus79.01%baseline, with geometry active on several ranks.

Memory cost is material: sampled summed process-tree RSS reaches20.3GiB versus
11.4GiB in the stopped baseline. Caches are nearly full on all ranks. Scope
construction accounts for80.3%of measured upper-query time at coarsening;
2290scope hits,10099misses/builds,2182scope retention refusals. Balancing has
not eliminated preparation cost or proved that eviction would help.

Evidence: `staged57/geometry_balance155_m6_comparison.json`, and the attempt's
`initial_partition_assessment.json`, `coarsening_stage_assessment.json`,
`stage_assessment.json`, profile/progress receipts and per-rank logs. The run was stopped at the user's request. **No mesh accepted, no quality stage
completed, no transfer/CFD/restart loop validated.**

### Two-lane coordination: implementation underway

The user explicitly superseded the prior instruction to wait for build155's
result and requested immediate implementation using its preserved baseline.
The primary agent is implementing split lanes and bounded-progress coordination;
this work is not yet implemented or validated. Final generation22 at3654.96s
had484886 private cells,46763 accepted splits,86916 size-bad cells and size hinge
16466.18;2905 geometry-bad cells and geometry debt0.00038483. Maximum reported
metric edge was3.8544. No quality stage was reached. Sampled process-tree RSS
peaked at about20.8GiB; it is not a whole-job peak.

The source supports pass-long neighboring-edge retirement as a major attempt
coverage limit (`Selection::ServiceClaimedVertex`, called after publication),
and curved proposals remain one/rank/round. Current generation-local long-edge
sums count shared replicas; quoted attempt percentages need a unique-owned-edge
census before being treated as exact. High accepted/proposed ratios make broad
operator refusal a less plausible primary cause. There is no50k q/J baseline.

Standalone Refine already omits bidirectional pass claims and retains fresh
lookup/full reservation/versioned publication. This is relevant reuse evidence
for relaxing split claims, not a reason to weaken any transaction check. Keep
collapse policy unchanged. Full expanded footprints and geometry/BL effects
still need existing certification/reservation; cell-only ReserveInteriors has a
different, narrower contract and must not be substituted blindly.

Local physical incidence is a scheduling hint. Late imported/expanded boundary
or BL effects need an explicit routing policy if the lane is intended to isolate
cheap interior work. Count late discoveries. Global lane transitions must cover
unmeasured surface work under progressive BL seeding, pending conflict losers,
reactivated edges and collective call order. Freeze the entry frontier and put
descendants in the next finite generation. A global least-priority eligible
proposal can make progress; lowest rank alone is not the complete priority rule.

A bounded window using unique-edge debt/maximum length and geometry debt is a
sounder handoff criterion than any accepted edit. Complete consistent censuses,
noise tolerance, strict final gates and unchanged cumulative budgets remain
necessary. It is not a guarantee of adequate quality time. Surface lanes still
execute collective rounds; lane separation does not remove surface lockstep or
preparation costs. Preserve the endorsed surface-first scheduling policy. The
review cites an older feedback_surface_first memory; its absence from this
workspace is not evidence that the user never gave that instruction.

Defer general balanced placement, multipoint insertion and curved multi-slot
batching pending target evidence. At generation15, build155's stalled maximum
finally changes5.9563->5.4412;32901splits accepted, still no quality stage.
An age/last-disposition receipt should distinguish unattempted bulk retirement,
ordinary conflicts, pass deferral and actual rejection rather than infer why it
stalled. Proposed future tests are neighboring interior edges across owners,
late boundary/BL expansion, finite frontier/descendant servicing, frozen-metric
measurements after projection, serial/MPI6 topology/publication/provenance and
the same full M6 workflow. Diagnostics are not completion.

## Literature baseline

Loseille–Alauzet–Menier, *Computer-Aided Design* 85 (2017), section 3.1,
[DOI](https://doi.org/10.1016/j.cad.2016.09.008), estimates insertion/collapse/
optimization work from natural and target metrics and their intersection, with
operator coefficients. It separately adds surface-face work to the adjacent
volume element. The existing SU2 density/edge-log heuristic and this structural
geometry estimate are not that exact formula. The local PDF section was read;
no source was copied. Empirical cost coefficients remain uncalibrated.
