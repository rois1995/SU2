Implement bounded, memory-efficient geometry caching for native adaptation,
without losing the benefit of geometry-aware MPI balancing.

The objective remains the accepted M6 Euler CFD → mesh → CFD → mesh workflow,
then BL verification and RANS. Cache improvements and passing fixtures are
supporting work, not capability completion.

1. Verify the current state before acting

Read AGENTS.md, HANDOFF_Codex.md and the current geometry-balancing handbook.
Check the active run and its actual memory receipts; do not assume build155
is still current.

The recorded build155 comparison showed:
- Approximately20.3GiB summed process-tree RSS versus11.4GiB previously.
- All six ranks near3.4GiB RSS.
- All six prepared-geometry pools near their3.208GB accounting limits.
- Memory approaching a plateau after the pools filled.
- More construction progress than the previous partitioning, but no accepted
  workflow established at that observation.

Treat cache-accounted bytes, summed RSS and actual physical memory as different
measurements. Do not infer a leak from higher RSS alone.

If the current run remains bounded with adequate headroom, let it continue.
Do not stop it merely because RAM use increased. If actual pressure, sustained
paging or a demonstrated cache bottleneck warrants interruption, preserve the
evidence and stop only the verified owned run under the existing instructions.

Do not edit pinned sources while a build or validation run is active.

2. Establish the memory breakdown

Reuse existing telemetry and inspect:
- Common/src/adaptation/CNativeAffineReference.cpp:
  CurvedSearch, prepared facet pieces, scoped targets, retention and budgets.
- MappedFacetReference construction and all query-cache budget callers.
- Native working-storage admission and geometry ResidentBytes accounting.
- Current per-rank cache/query telemetry and aggregate resource receipts.

Determine approximately how much memory belongs to:
A. Immutable geometry authority that must remain available.
B. Reusable per-facet prepared data.
C. Combined footprints/search structures for particular support scopes.
D. Temporary construction/query buffers.
E. Mesh, metric, directory, solver and transfer state.

Distinguish live allocations from allocator-retained RSS. Identify whether
growth continues after cache saturation and whether later stages need
substantially more transient memory.

3. Introduce a coherent aggregate cache budget

The current six-rank job is on one host. Its cache allowances should fit one
explicit aggregate allowance, rather than six independent allowances whose
combined cost is overlooked.

For general MPI execution, enforce this per host/shared-memory domain—not by
applying one host's available RAM to an entire multi-host job.

Reserve headroom for non-cache state and transient stages. Reuse existing
controls and admission facilities. If a new cache-specific control is necessary,
keep it narrow and document its per-host/per-rank semantics.

Start with simple, stable rank shares. Do not add frequent global collectives
inside local geometry queries or an elaborate adaptive quota controller.

A cache budget is not a complete process-memory limit. Report that distinction
and preserve all existing native working-storage refusals.

Do not silently reinterpret an existing configuration option.

4. Improve retention within that budget

The current policy retains entries until the pool fills; subsequent new scopes
may then be rebuilt repeatedly while cold entries remain resident.

Implement the smallest justified bounded replacement/admission policy:
- Distinguish reusable per-facet data from scope-specific accelerators.
- Avoid allowing one-use derived scopes to crowd out valuable reusable data.
- Use observed reuse, entry size and reconstruction cost to justify policy.
- Prefer existing containers and ownership mechanisms.
- Avoid unbounded metadata, scans or maintenance work per query.

Do not assume plain LRU is automatically sufficient or that a lower cap will
improve runtime. Compare the chosen policy with recorded M6 behavior.

Preserve exact cache identity: authority, support scope, tolerance, preparation
resolution and other relevant inputs. Do not broaden queries or approximate
keys to manufacture cache hits.

The immutable geometry definition is not disposable cache data.

5. Preserve ownership and resource correctness

An entry used by an active query must remain alive.

Do not reclaim its budget while outstanding references still retain its memory.
Account for metadata, construction scratch, and overlap between old and newly
built structures. Keep thread safety and mutable traversal scratch correct.

Cache hits, misses, replacement and rebuilding must retain cumulative work
accounting and sticky budget-exhaustion behavior. Resource exhaustion remains
fail-closed.

Do not change geometry tolerance, metrics, wall heights, reconstruction rules,
quality gates or transfer/restart obligations. Keep cache policy ephemeral;
avoid an archive-format change unless genuinely unavoidable.

6. Add compact validation and telemetry

Record per rank and per host:
- Cache allowance and live accounted bytes by category.
- Entry counts, hits/misses and retention refusals.
- Evicted bytes/entries and reconstruction cost.
- Sampled aggregate memory and relevant transient peaks.

Use compact summaries rather than enormous per-query logs.

Focused checks should cover:
- Repeated useful queries mixed with many unique scopes.
- A deliberately tight cache allowance.
- Eviction/rebuild without dangling references or accounting errors.
- Cached versus recomputed certification behavior.
- Cumulative work/storage exhaustion.
- MPI budget allocation, including unequal ranks per host where supported.

Use representative existing geometry fixtures. Do not build a large new
benchmark framework.

7. Validate against the actual workflow

Run the endorsed M6 Euler target with unchanged acceptance gates and comparable
inputs. Reuse valid baseline evidence instead of repeating a full old-code run
solely for benchmarking.

Compare memory and useful progress at equivalent stage milestones. Determine
whether reduced retention causes extra rebuilding or merely frees cold data.

Success means:
- Predictable aggregate cache use with adequate transient headroom.
- Correct accounting and certification.
- Preserved or improved useful M6 progress.
- Continued pursuit of the strictly accepted complete Euler workflow.

Follow the existing one-heavy-job rule,2–4 build-worker limit, source freezing,
evidence preservation and regular checkpoint pushes.

Report the measured tradeoff honestly. Do not declare the memory problem solved
from a lower RSS value alone, or the adaptation capability complete from tests.

Extend this task to include phase-aware adaptation repartitioning.

Treat partitioning, cache residency and memory admission as one coordinated
design. Preserve all requirements in the original prompt.

1. Implement phase-specific work estimates and safe repartitioning boundaries

The initial adaptation partition should estimate coarsening work. Evaluate
repartitioning again at:
- Coarsening → splitting.
- Construction → quality improvement.

Use task-specific estimates:
- Coarsening: short-edge demand, complete vertex-star work, geometry remapping.
- Splitting: oversized-edge demand, expected insertions, cavity and geometry cost.
- Quality: measured repair demand and expected cavity/geometry work.

Compute quality-specific information at the permitted quality-entry census.
Do not introduce q/J screening into validity-only construction.

Reuse:
- CNativePartition3D.cpp: WorkWeights and Repartition.
- CNativeImport3D.cpp: WorkingWeights.
- CNativeEngine3D.cpp: phase transitions and generation coordination.
- Existing ownership migration, directory and resource-admission facilities.

Do not repartition automatically after every split generation or lane switch.
Initially support major-phase boundaries, with a decision based on measured or
predicted benefit. A focused case must demonstrate that the migration path
actually executes correctly.

2. Make the decision include the complete transition cost

Evaluate whether the expected reduction in remaining critical-path time exceeds:
- Weight estimation and graph partitioning.
- Cell and attribute migration.
- Ownership-directory and selection-state rebuilding.
- Required measurement/routing reconstruction.
- Geometry-cache warming.

Do not judge overhead from the partitioner call alone.

Use recorded useful-work costs and workload changes where available. Distinguish
predicted balance from measured balance and retain explicit uncertainty in the
estimate. Prefer a simple defensible policy over a complex forecasting framework.

3. Preserve event semantics through ownership changes

Repartition only at a coordinated quiescent boundary:
- No outstanding candidate transactions or unsettled publications.
- Reservations and pass claims handled explicitly.
- Checkpoints preserved, migrated or retired consistently.

Migrate working cells and their associated attributes without changing IDs,
versions, geometry bindings or metric values. Rebuild rank-local directories,
queues and routes as required.

Preserve the frozen field definition and identity within its existing validity
scope. Repartitioning itself must not redefine the target from the adapted mesh,
renormalize it, or restart adaptation.

Preserve cumulative work/time accounting and globally unique allocation epochs.
Do not reset budgets or ID counters by restarting the engine at each phase.

Ownership-only migration requires data redistribution, not another conservative
CFD projection. Keep the accepted CFD donor solution intact and retain the normal
mesh-change transfer obligations.

4. Integrate cache policy with the new ownership

Keep simple, stable per-rank shares of the per-host cache allowance initially.
Moving cells does not require dynamically changing cache quotas.

Do not serialize and migrate entire geometry caches by default. Conversely,
do not clear all caches merely because ownership changes:
- Immutable geometry and valid prepared data remain mathematically reusable.
- Preserve useful entries.
- Allow bounded replacement of cold scope-specific structures as locality changes.

Partition identity or phase name must not become a cache-key component unless
the underlying mathematical query genuinely depends on it.

Account for migration's transient overlap: old and incoming cell storage,
directories, active cache references and query scratch. Admission must cover the
transition peak, not only the final ownership.

Do not reclaim cache accounting while outstanding references retain allocations.
Do not add collectives inside ordinary local geometry queries.

5. Validate the combined behavior

Add focused checks covering:
- Different expensive regions for successive phases.
- Ownership changes with preserved cells, attributes and frozen-field evaluation.
- Correct queue rebuilding and continued refinement/repair.
- Tight cache budgets, retained useful entries, and replacement after migration.
- Transition-memory admission and failure leaving a coherent prior state.
- Cumulative budgets and ID uniqueness across multiple phase changes.

Then run the complete endorsed M6 workflow under unchanged acceptance gates.

Report at equivalent phase milestones:
- Predicted and actual useful-work imbalance.
- Collective waiting.
- Transition and cache-warming costs.
- Cache reuse/rebuilding.
- Aggregate memory, including transition peaks.
- Total progress and elapsed time toward accepted meshes.

Success requires a beneficial combined tradeoff. Lower RSS, balanced weights,
faster partitioning, or passing fixtures alone are insufficient.

Keep the current healthy run intact while gathering evidence. Implement only
after its pinned sources can safely change, or after an interruption justified
under the existing authorized workflow.

Do not let this become an independent infrastructure project. Continue toward
the accepted Euler CFD → mesh → CFD → mesh loop, then BL verification and RANS.

BL compatibility is required now; full BL construction work is a later milestone.

The cache and phase-repartitioning changes must preserve existing wall policies,
geometry associations, requested first-height constraints and complete
wall-cavity dependencies. Do not introduce Euler-only assumptions, fixed wall
coordinates, permanent column requirements or solver-label reconstruction modes.

Ownership changes and cache replacement must not invalidate height/clearance
checks or silently change their meaning. Preserve the existing distinction
between mandatory geometric/height constraints and deferred q/J improvement.

Memory admission must accommodate changing mesh size and temporary migration
storage; do not treat the initial Euler cell count as a bound on future demand.

Reuse a small set of existing first-height and joint-wall regression checks
when validating shared changes. These checks do not establish BL capability.

Keep the immediate capability objective the complete accepted M6 Euler loop.
After it succeeds, perform a dedicated BL qualification covering curved walls,
junctions, MPI interfaces, subsequent mesh operations and redistribution.
Use its evidence to implement BL-specific work estimates, coordination and
memory policies before starting the full RANS workflow.


2026-10-31user extension: [geometry query reuse milestone](ADAPTATION_GEOMETRY_QUERY_REUSE_20261031.md)
contains the latest specification verbatim. Keep live166frozen until terminal or
preserved evidence justifies authorized stop; use its outcome, then if geometry
query cost remains dominant implement one connected reuse improvement across all
three phases and return immediately to full M6. Full original goal/gates persist.
