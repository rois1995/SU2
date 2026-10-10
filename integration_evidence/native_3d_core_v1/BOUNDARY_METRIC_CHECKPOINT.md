# Native 3D coupled planar-boundary and metric checkpoint — 2026-10-10

Working repository: `/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated`.
Branch: `codex/native-3d-core`, predecessor `5f46583e5b81b95aa4dba9f538f2ae0a9573bdca`.
Goal 1 remains ACTIVE. `NATIVE_3D_DEVELOPMENT_GOALS.md` retains its full completion
contract. This checkpoint adds privately validated operators and five complete
manufactured serial mesh cycles; it does not enable SU2's 3D adaptation driver.

## Implementation and admission

`CNativeBoundary3D` couples complete tetrahedral edge-star bisection with the
incident physical triangles. An immutable finite original-facet reference owns
markers; new children inherit that reference. Exact coplanarity, finite-facet
membership, orientation, manifold sphere/disk links and the prescribed oriented
skin are checked together before either volume or surface output changes.
A required edited interface face without physical authority rejects with its IDs.
The inverse operation pairs the complete removed-vertex star and physical siblings,
restoring their parent connectivity. It is inverse bisection, not general collapse.
Coordinates must represent an exact on-edge binary64 point. No silent snapping,
geometry tolerance or changed metric is used to force acceptance.

The existing fixed-interface cavity path is unchanged. The new prescribed-interface
validator requires its caller to prove the physical surface replacement and volume
closure. Exact on-edge subdivision/paired inverse provide that proof here; the
validator alone cannot certify an arbitrary changed physical surface. Global star
completeness, ID reservation, valid embedded source and authoritative skin are
caller preconditions; local topology cannot certify an MPI dependency import.

`CNativeMetric3D` samples shape in the target at each actual cell centroid and
integrates every unique edge through the original sensor P1 donors. Final completion
requires q >= .20, J >= .05 and integrated L <= 1.8. Bounded private refinement may
retain pre-existing long edges but must strictly reduce excess on changed edges;
coarsening must remove cells without adding oversized edges. Repair must reduce
size excess or improve quality/sliver bounds without increasing that excess.
Unchanged edge integrals are reused by exact identity and coordinates only within
the same frozen-field audit. Common residuals cancel without subtracting large sums.
No persistent unbounded metric-edge cache was added.

Actual-point composition still runs at every point query, including cache hits.
`TargetEdgeLength` explicitly rejects a composed target until geometry-resolved
integration is implemented; sensor-only length is never substituted for a combined
sensor-plus-BL acceptance test. This preserves the representation contract without
claiming a real native 3D BL provider. 2D/MMG source, fade and noise settings are
unchanged. Strict passive library and Catch registration include the new units.

## Evidence and inspectable adapted grids

All paths below are relative to this evidence directory:

- `boundary_metric_controls_v3`: final strict-linked Catch validation, 37 cases /
  2,651 assertions and 290 independent Fraction orientation signs. It also runs
  without Git on PATH and retains all listed source snapshots and hashes.
- `coupled_meshes_v2`: five independently audited refinement/coarsening cycles,
  182 accepted edits in total, on box, height 1e-4 and 1e-8 boxes, a sheared domain,
  and a rotated spatially varying P1 tensor field. The coarse event freezes new
  donors on the accepted adapted mesh, rather than reusing the initial six donors.
- `coupled_meshes_v2/*_adapted.su2`: accepted manufactured-target meshes, with actual
  six side markers. Corresponding initial/coarsened SU2 files and JSON donor/mesh/
  facet data permit inspection and independent replay. These have no CFD solution.

| Cases | Tetrahedra initial / adapted / coarsened | Wall triangles initial / adapted / coarsened | Final q min | Final J min | Final L max |
| --- | --- | --- | --- | --- | --- |
| box, thin_1e4, thin_1e8, sheared | 6 / 48 / 6 | 12 / 48 / 12 | .755953 | .577350 | 1.732051 |
| rotated_spatial | 6 / 40 / 6 | 12 / 40 / 12 | .658696 | .420157 | 1.749855 |

The independent Python audit uses actual-binary64 Fraction predicates and volumes,
all-pair tetrahedron separating-axis overlap checks, face cancellation and manifold
links, finite-facet membership/coverage and marker preservation. For these convex
reference domains, containment, exact volume equality and nonoverlap close the gap
check. P1 interpolation, metric shape and source-resolved edge lengths are evaluated
independently, with Fraction clipping and high-precision Decimal integration for
the spatial field. Initial fine-field frames deliberately fail the length gate;
all adapted/coarsened frames pass. Coarsening restores original geometry and
connectivity. All 15 exported grids and 15 mesh JSON files are byte-identical to
`coupled_meshes_v1` after adding the separate repair gate.

Positive matrix cases have zero private rejections; negative Catch controls cover
incomplete dependencies, facet departures, budgets, bad sibling pairing, output
rollback, excessive size, bad shape and unsupported composed targets. They are
separate evidence, not a difficult-case rejection-rate benchmark.

Earlier boundary_controls_v1 and boundary_metric_controls_v1/v2 and the first
no-Git check are retained. Their unit runner recompiled the metric source in its
test compile instead of linking metric.o; v3 fixes that and is authoritative for
strict-linked unit validation. Full-mesh probes already linked the strict metric
object. v3 object hashes match those used by coupled_meshes_v2. Source receipts
reject in-flight edits; no-Git snapshots preserve the listed source versions,
while replay still requires the SU2 checkout and its headers/external dependencies.
Objects/executables stay local and are not published.

## Cost observations and next priorities

One sequential low-priority process, numerical-library threads 1, under foreign
Python/MPI contention. `probe_summary.json` closes exclusive parent phases; field
query/edge/trace timers are nested diagnostics and must not be added to parents.
Single observations from coupled_meshes_v2, summed over five cases:

| Exclusive phase | Seconds | Share of 2.723354 s parent |
| --- | ---: | ---: |
| Selection, including repeated full toy-mesh metric scans | 1.247643 | 45.8% |
| Private metric validation | 1.337250 | 49.1% |
| Geometric proposal/validation | .064687 | 2.4% |
| Field creation, commit, final gate, output, residual | .073774 | 2.7% |

There are 8,390 actual edge evaluations and 1,556 common-edge integral reuses.
The fine-field edge timer is 1.521708 s, including 1.511935 s in tracing; these are
nested costs across selection/validation. The controller's whole-mesh rescans are
intentional tiny-probe scaffolding, not a production selection strategy. The source
tracing and repeated selection work need attention before larger campaigns.
No speedup, scaling, instrumentation-overhead or CFD-affordability claim follows
from one contended observation. Peak measured patch bounds are 48 cells, 27 nodes
and 98 edges, including whole toy-mesh scans. Private old/new budgets remain64/128;
the probe itself stops at128 cells and96 sweeps.

Next: reduce measured source tracing and repeated evaluation cost without changing
frozen work/target, and validate numerical association for rounded boundary points
and more general planar coarsening. Then implement actual MPI dependency/version/
atomic admission, cost-weighted M <= N workers and return/transfer, and the SU2
runtime/CGNS/full correctness and performance matrix. No transaction is migrated
while outstanding. The paper principles and precise local reading scope remain in
DESIGN_REFERENCES.md/papers_reference.json; no additional paper is needed here.

Known limits: reference authority is individual original triangles, so inverse
coarsening needs parent ancestry and cannot merge unrelated coplanar facets yet.
Arbitrary rotated physical facets can lack an exactly representable midpoint and
are not qualified. Thin mapped boxes start geometrically thin; they do not prove
cold Euler-to-BL construction. Whole-mesh selection presently exercises split and
inverse coarsening; move/reconnection have private geometry/metric controls only.
Native runtime still rejects3D. Full Meson/AD/CFD/MPI, solution/history transfer,
CGNS, general embedding, BL generation and practical scaling remain unchecked.

## Replay

Check contention and announce a NEW folder before each run. No existing evidence
is overwritten. Use one owned process and one numerical-library thread:

```bash
export PYTHONDONTWRITEBYTECODE=1 OMP_NUM_THREADS=1 OPENBLAS_NUM_THREADS=1 MKL_NUM_THREADS=1
nice -n 19 python3 integration_evidence/native_3d_core_v1/run_geometry.py \
  integration_evidence/native_3d_core_v1/boundary_metric_controls_NEW
nice -n 19 python3 integration_evidence/native_3d_core_v1/run_coupled.py \
  integration_evidence/native_3d_core_v1/boundary_metric_controls_NEW \
  integration_evidence/native_3d_core_v1/coupled_meshes_NEW
```

The first run rebuilds validated strict objects; the second checks their receipt,
source and object hashes before linking, generates all meshes and runs the
independent audit. Git is optional for provenance; without it the first run retains
all listed source snapshots. Both commands run locally on small cases; they are
not an SGE scaling campaign or a full solver build.
