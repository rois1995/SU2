# Bounded geometry residency and phase ownership — active integration

Resumed from pushed `adc31b971b`, implementation `f104557fa6`/build157.
The explicit user resumption supersedes the one-time handoff pause. There is one
active implementation/validation cycle. No accepted M6 adaptation exists yet.
Euler target remains 50k metric complexity, RANS 500k. All geometry, q/J, size,
height, transfer and disk-restart gates remain strict. Construction split lanes,
finite frontiers, full reservations and versioned publication are preserved.

## Provenance and observed limitation

`staged57/cache_phase_resumption_provenance.json` independently rehashed all
1397 distinct handoff pins (run/build/serial/MPI manifests), with no changes;
all ten recorded owned host handles were absent. HEAD equals the local tracking
ref for the pushed branch. Existing unrelated logs and untracked files remain.
Host before build158: 34 GiB available, load 5.32/5.72/7.81, nearly full swap;
root 3 GiB and repository disk 64 GiB available. Use 4TB compile-tmp.

Build157's private geometry closure has no remaining oversized edges but 3131
geometry-invalid cells. Scope preparation consumed 5584/7327 nested upper-query
rank-seconds; cold scopes occupied full pools while later scopes were refused.
Recorded summed tree RSS 21.664 GiB is not cache bytes or physical memory.
Initial geometry balancing was useful; phase migration did not exist.

## Connected candidate (not yet validated)

Collective reference loading gives each MPI shared-memory domain at most 4 GiB
of geometry query cache, additionally capped at a quarter of observed free
physical pages, then divides the domain allowance equally among its ranks.
This is a new explicit cache-only policy, not a reinterpretation of native working
storage. Noncollective decode retains its previous local allowance semantics.
No query-path collectives or sidecar-format changes. The answer memo is charged
against the quota; tight allowances omit that optional memo. Native working
storage admission remains independently mandatory.

Prepared data has separate facet and scope pools, each half the remaining
allowance. Bounded clock replacement protects observed reuse with capped credit
based on reconstruction work per byte. One-use scopes start with no credit.
Up to 64 entries are examined per admission; refusal remains possible rather
than unbounded scanning. Active shared references pin both data and accounting.
Map/list/key metadata is included conservatively. Scratch and old/new overlap
remain query/working-storage obligations, outside retained-cache bytes. Exact
authority, support, tolerance and preparation-resolution keys are preserved.
Compact stage rows add live facet/scope bytes, entries and eviction accounting.

Initial metric and geometry workload prediction selects coarsening demand.
The engine evaluates ownership once at coarsening-to-splitting and once after
the permitted full quality-entry census. No generation or lane migration.
Weights use measured edge demand, complete distributed vertex-star counts and geometry preparation;
quality weights additionally use measured q/J/height and geometry debt.
The automatic decision uses predicted imbalance, observed attempted-edit cost,
initialization/seeding, estimation, prior transitions and scope-warming allowance,
with a twofold uncertainty hurdle. This is a heuristic, not calibrated CPU time
or proof of improved balance. Actual transition costs are recorded separately.

At a quiescent transition, stage a new Owned map, repartition it with the existing
atomic migration facility, route exact cached census/edge certificates with the
same destinations, build a new directory, and stage new selection queues/frontier.
Publish these together after collective agreement. Full old/incoming ownership,
records, old/new directory and selection storage overlap is admitted. An active
rollback checkpoint retains its ownership rather than being partly migrated.
Old phase pass claims and rejection queues retire together at the boundary.

CavityMetricSource retains the original bootstrap, references the stable working
map object and reads transported nodal sensors. CFD ownership/donors stay intact;
no projection, engine restart, target renormalization, field identity change or
ID/work/time reset occurs. The worker communicator membership stays fixed.
Geometry caches remain local and bounded, without phase/rank query keys.

Geometry-only invalid rows now join quality repair queues, closing the handoff's
known omission. A split-specific concurrent-commit counter qualifies split lane
coverage independently of collapse commits.

## Validation and next acceptance

Build158 is terminal FAILED561.841s on a Catch2 assertion-expression syntax error;
production targets compiled, all1296pins unchanged and owned PID765540 absent.
The assertion is corrected. Review also identified phase deadline checks, now
added at safe staging/publication points, and cache replacement now charges its
bounded maintenance against cumulative query work. No target result exists yet.
Build159 is now ACTIVE and frozen: owned Ninja PID805786, exec session60312.
Inspect `staged57/build159_status.json`, pins and the actual host handle before
editing sources/tests/helpers or launching validation. Three low-priority workers,
single-threaded libraries and 4TB compile-tmp remain in force.
Focused checks cover cached/recomputed geometry and eviction, host share math,
phase migration through opposing metric demands, field/donor identity, finite
split scheduling/concurrency, existing first-height and joint-wall behavior.
A failure injected through the existing stage observer checks that staged migration
can be rejected without publishing new ownership. Complete-star census checks
cover different rank layouts and tight-memory atomic refusal.
DiagnosticAlways is an internal test control to exercise both migration paths;
production defaults to the profitability decision.

After affected checks and one connected regression, run the complete two-event
M6 Euler attempt with the existing harness and gates. Compare equivalent phase
milestones, cache rebuilding, transition cost, useful work and sampled aggregate
memory. Fixtures and memory reduction are not completion. Accepted Euler must
include conservative admissible transfer, mesh/reference publication, continued
CFD and independent usable disk restart. Then qualify curved/junction/MPI BL and
redistribution before the corresponding RANS500k workflow.

Any additional ideas belong in the [backlog](ADAPTATION_BACKLOG.md) unless a demonstrated blocker
requires them. Preserve stop/fix/retry evidence and push reviewable checkpoints.
