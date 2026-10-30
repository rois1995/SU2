# Geometry-aware initial adaptation ownership

Current implementation work, 2026-10-30. **No accepted M6 loop yet.**

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

The complete six-rank M6 attempt is running at
`staged57/m6_runs/euler_build155_geometry_balance50k`, driver244296; current
owned handles/resources are in its status/receipt files. Targets, budgets and
final gates remain unchanged. Initial predicted balance, actual geometry-work
balance, collective waiting, cache/memory costs and end-to-end success remain
to be assessed from this run. No second old-code campaign is needed.

## Literature baseline

Loseille–Alauzet–Menier, *Computer-Aided Design* 85 (2017), section 3.1,
[DOI](https://doi.org/10.1016/j.cad.2016.09.008), estimates insertion/collapse/
optimization work from natural and target metrics and their intersection, with
operator coefficients. It separately adds surface-face work to the adjacent
volume element. The existing SU2 density/edge-log heuristic and this structural
geometry estimate are not that exact formula. The local PDF section was read;
no source was copied. Empirical cost coefficients remain uncalibrated.
