# Incremental distributed tetrahedral refinement checkpoint

2026-10-10. Working repository:
`/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated`.
Branch `codex/native-3d-core`, predecessor
`0d09c431ea5b07099cf7d671c5c5f9169e4a2433`.
Goal 1 remains active with every deliverable in NATIVE_3D_DEVELOPMENT_GOALS.md.
This is a complete frozen-target refinement engine on the tested box, rather
than a single split. It does not enable the production SU2 3D remesher.

## Implementation

CNativeEngine3D::Refine uses an explicit World worker communicator, a private
working mesh, the immutable distributed original sensor service and complete
edge-star lookup/import. A per-cell metric status and reference-counted per-edge
length map feed an ordered candidate set. Initial cells are measured once;
only published children are remeasured. Retired cells/edges are removed rather
than accumulating stale heap entries. There is no whole-mesh candidate or source
scan on each sweep. Imports are batched, with a common bounded halving fallback
from sixteen cells to one when their donor union is inadmissible. The current
six-cell campaign exercises batching but has zero halving retries; the nonzero
fallback path still needs a larger conforming source fixture.

Each synchronous round elects conflict-free complete patches, reserves globally
unique node/cell ranges, couples exact edge subdivision to planar boundary
triangles, checks private shape/sliver gates and changed-edge progress against
the unchanged target, then routes children back to their original parent owners.
Directory publication stages mesh and dependency metadata before common commit.
The per-owner publication limits remain 64 retired and 128 fresh cells; a larger
combined round rejects rather than silently widening them. Selection-cache
update can fail after private publication: the whole event then rejects, and
accepted CFD input remains unchanged. This is event rejection preservation,
not allocation-atomic publication of the auxiliary selection cache.

Completion requires every working cell's strict metric gate: q >= .20,
J >= .05, original-field edge length <= 1.8. No target weakening, reduced finer
sensor demand, or relaxed quality/embedding guard is used to finish.

The metric validators expose optional reuse of verified scalar edge integrals
from the same immutable original sensor target. They check exact endpoint IDs,
coordinates and positive finite lengths before reuse; moved endpoints recompute.
The engine uses only event-local, rank-local measurements. Retired scalar values
are retained in a bounded temporary map until children are measured. No long-double
values are rounded and exchanged to authorize a remote gate. Cell quality always
queries the actual centroid. Reuse is disabled for composed fields; unsupported
geometric edge integration still rejects instead of accepting a sensor substitute.
The reuse precondition is enforced by the engine's immutable source lifetime;
the standalone validator's caller must supply measurements from that same target.

Source authority, geometric reference, accepted mesh and frozen composition must
already be consistent and conforming. Source coverage remains exactly checked
for newly evaluated edges/queries. Cached integrals retain their earlier exact
coverage proof only for the same frozen source and unchanged endpoints. Reference
facets <= 256, imported donors <= 256, and cavity limits remain explicit. Modeled
requested storage includes working/accepted/source/cache/history and bounded
scratch; caller-owned geometry/CFD buffers belong in caller_bytes. This is not
allocator/RSS certification or hard-OOM recovery. Owner-preserving child routing
is not tetrahedral cost-weighted ParMETIS repartitioning.

## Correctness evidence and inspectable grids

Final campaign:
`integration_evidence/native_3d_core_v1/engine_controls_v3/validation.json`.

- MPI1: 54 cases / 2891 assertions, including all 38 3D kernel cases.
- MPI2: 16 cases per rank / 216 and 230 assertions.
- MPI4: 16 cases per rank / 223, 222, 221 and 236 assertions.
- Full refinement on N=M=1/2/4 and worker subsets M=N-1 and M=N/2, with mesh
  record migration to M and return to N. Inactive ranks stay outside workers.
- Original accepted input is byte-unchanged after success and forced round-limit,
  storage, mismatched options and exhausted-ID-range failures. No solution/history
  transfer is claimed by the mesh-record fixture.
- New reuse controls check identical lengths, moved coordinates, NaN fallback
  and composed-field refusal. Profiling OFF/ON retains identical meshes/work.
- Independent Python rational audits verify actual MPI records: positive volumes,
  no overlaps/gaps in the declared unit box, conforming manifold skin, complete
  immutable facet coverage/markers, and original-P1 metric gates. Actual donor
  transport/canonical P1/actual-query thin-composition checks retain zero residual.

Nine completed engine grids across the worker matrices have 48 tetrahedra,
27 nodes and 48 physical triangles, starting from six tetrahedra/eight nodes/
twelve triangles. Every original facet has four children and retains its marker.
Unit volume is exact; minimum q=.7559526299369239, minimum J=.5773502691896257,
maximum metric edge=1.7320508075688772. The main runs accept 19 splits; MPI4 needs
22 rounds and has three rejected private attempts. This partition-dependent
trajectory is audited separately, not interpreted as a fixed-work scaling curve.

View `engine_controls_v3/mpi{1,2,4}_meshes/engine_full.su2` or `engine_m*.su2`.
`engine_rejected.su2` retains the one-round 12-tet private mesh and its oversized
edge (maximum 2.828); it is not an accepted adaptation result. Original and earlier
one-step refinement/inverse grids remain beside them. The 27 JSON and 27 SU2 files
are byte-identical to v2 within each partition; work/acceptance counters match.
Receipt: `engine_controls_v3/reuse_comparison.json`.

Actual non-MPI rebuild/control also passes in `engine_controls_v3/serial_engine`:
full refinement, independent whole-mesh metric gate, planar skin, round-limit
rejection and composed-target refusal. Peak RSS is 4680 KiB for this tiny process,
not an upper bound for larger cases. Earlier v1/v2 logs, pins and source snapshots
are retained; duplicate earlier grids can be reconstructed from their actual logs.

## Performance evidence and limits

Default-off engine detail gives exclusive initialization, seed/cache preparation,
selection, dependencies, reservation, donor import, reconstruction, private metric,
routing, publication, update and completion phases, plus a residual other phase.
Exact query/index/cache/interpolation/composition and edge search/trace/integration
are nested detail: do not add them again. Private CPU excludes surrounding failure
votes; process CPU includes MPI busy waiting. House collective instrumentation is
partial, not a complete MPI-wait accounting. The engine parent timer excludes final
object destruction; standalone external observations include it. Neither timer
covers the complete CFD adaptation lifecycle, partitioning, return/rebuild/transfer
or requested output, which remain required Goal 1 work.

The v2 MPI1 diagnostic took .1985 s: original-cell exact edge tracing used .1717 s;
sensor interpolation .0000646 s. Selection was negligible. Reusing the already
verified scalar results addresses that measured cost without weakening predicates.

| Main fixture | Selection edge integrations before -> after | All field edge integrations before -> after |
|---|---:|---:|
| MPI1 | 540 -> 117 | 819 -> 234 |
| MPI2, sum of ranks | 540 -> 174 | 819 -> 306 |
| MPI4, sum of ranks | 540 -> 180 | 861 -> 358 |

The frozen target, 19 accepted edits and exact grid remain unchanged. MPI1 v3's
single diagnostic is .0671 s, with .0444 s in exact tracing, .0244 s in the private
metric phase and .0222 s in cache update. Modeled peak working request is
2,319,640 bytes. These single observations explain work, not scaling.

Three alternating baseline/candidate MPI1 repetitions were then run sequentially
using the verified v2/v3 binaries, identical meshes and accepted operations:
`engine_reuse_timing_v1/validation.json`. Baseline engine time spans .2006-.2304 s,
median .2080 s; reuse spans .0712-.0848 s, median .0763 s. Median observed local
engine speed ratio is about 2.73, with useful accepted work about 91 -> 249 edits/s.
This is a small, contended, constant-target box result, not a full SU2 speedup,
MPI scaling result, BL result or CFD affordability measurement. Raw per-rank
phases, CPU/work counters and host load/process snapshots are retained.

Three actual non-MPI detail OFF/ON pairs preserve identical work and geometry.
OFF external times span .0567-.0610 s, median .0589 s; ON .0566-.0587 s, median
.0582 s. These samples do not resolve a detail cost above observed variability;
zero overhead is not established. Both modes retain counters and existing house
instrumentation. Receipt: `serial_engine/detail_comparison.json`.

## Replay and remaining Goal 1 work

Use fresh full result paths; runners print them and retain failures:

```sh
python3 integration_evidence/native_3d_core_v1/run_distributed.py /absolute/new_engine_controls
python3 integration_evidence/native_3d_core_v1/run_serial_engine.py /absolute/new_engine_controls/serial_engine
python3 integration_evidence/native_3d_core_v1/run_engine_comparison.py /absolute/baseline_engine_controls /absolute/new_engine_controls /absolute/new_fixed_work_results
```

The comparison requires both locally built pinned binaries, not committed binaries.
Reconstruct v2 from its source snapshots and base commit before building a baseline.
Compute nodes need no Git; a closed checkout supplies the sources. SGE larger
correctness/scaling preparation will follow the anisotropic/coarsening engine,
without asking the unavailable user to launch jobs tonight.

Still required: full rotated/spatially varying anisotropic MPI engine campaigns,
repeated refinement/coarsening, general collapse/movement/reconnection, robust
rounded-point planar association where needed, larger-source fallback controls,
weighted tetrahedral ParMETIS/migration, actual runtime/solution/history/rebuild,
independent CGNS output, full lifecycle phase/wait/memory closure, 2D regressions,
and size/rank/instrumentation/cluster qualification. Existing kernel/serial
anisotropic operator successes do not prove these engine requirements.
Production 3D remains guarded. 2D/MMG code and finer sensor/BL/fade/noise policies
are unchanged; no curved geometry, robust BL construction or unsteady 3D claim.
