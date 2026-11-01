# Review of the independent M6 technical assessment

Reviewed2026-11-01 while build202 was frozen; updated after its natural rejection.
The attachment is analysis, not implementation authorization or reproduced CFD
evidence. Its hashes and a local input copy are recorded in
`staged57/technical_assessment202_review.json`. The complete objective remains two
strictly accepted Euler50k events, conservative admissible transfer, finite
continued CFD and isolated adapted restart; then BL/MPI and RANS500k.

The assessment's investigation order is useful. It supports obtaining concrete
failure witnesses before selecting construction, repair or certification changes.
The current build202 diagnostic cycle addresses state inspection and is already
qualified; it does not yet provide faithful operator replay. No new implementation
or interruption of the frozen run follows from this review.

## Findings that change interpretation

1. **Geometry-bad is not necessarily a proved distance violation.** Confirmed in
   current source. `Selection::Add` compares `SurfaceDeviation` with tolerance and
   sets `geometry_invalid` when the returned value exceeds it. The entity path
   calls `BoundPhysicalSurface`, which returns only `mesh_to_model` from the union
   query. In `QuerySurfaceUnionAtResolution`, upper-only guidance can return a
   conservative bound above tolerance with `outside=false` when target
   approximation prevents a decision. That result can enter the geometry-bad
   count. Direct surface-error paths also use sufficient upper bounds.
   Sources: `CNativeEngine3D.cpp:760`, `CNativeBoundaryParameters3D.cpp:1360`,
   `CNativeAffineReference.cpp:1738,2018`. Consequently, counts and excess bins
   describe failed geometry admission under the current bound, not automatically
   the number or magnitude of actual Hausdorff violations. This does not prove
   that any particular remaining M6 cell is a false refusal. Full two-sided final
   certification remains strict and fails closed; uncertainty must never admit a
   mesh. A selected witness must distinguish certified outside, unresolved bound
   and exhausted/unmeasured work before choosing a repair or certificate fix.

2. **Shape closure must include J-only failures.** Verified against complete199
   counts:4368q-bad +2537J-bad -6132union =773both, leaving3595q-only and1764J-only.
   Net q removal alone is insufficient. Keep whole-field q/J counts and their
   union, geometry counts, severity and churn where complete evidence exists.

3. **Interior does not mean unconstrained.** Confirmed: `CellState::surface` means
   a cell has a physical face. A cell without one can still contain surface or
   feature vertices and belong to a star constrained by boundary cells. Thus
   build199's82.9%interior q-bad/74.1%interior J-bad must not be read as free-interior
   populations. The new per-vertex bindings/mobility dimensions permit the next
   actual capture to distinguish physical-face cells, constrained-interior cells
   and genuinely free-interior cells without changing the frozen implementation.

4. **Short-edge evidence needs clean controls and unique edges.** The observed
   6045/6132bad cells with edge<.5 does not establish redundant resolution or a
   collapse defect. Use retained clean cells from the full snapshot, match metric
   anisotropy/variation and geometric setting, and count unique edges and complete
   incident stars. Distinguish required close vertices, bad placement, topology
   and metric transitions. No construction q/J policy follows from correlation.

## What capture currently proves, and what it does not

The qualified `FailureCapture` streams complete owned coordinates/connectivity,
current nodal tensors, versions and physical faces when caps permit. It retains
cached q/J/edge/geometry data, current blocked-feature state, surface bindings,
reference controls/body/features and requested-height policies. Rank manifests
carry ownership; offline extraction reconstructs full SU2/VTU and complete global
vertex-star neighborhoods with explicit omissions. Clean controls are available
from the retained whole mesh. This is useful inspection evidence.

It is **not a faithful replay of the rejected trial**:

- Original donor/background cells are absent; composition and edge breakpoints
  are explicitly marked unserialized. Current nodal tensors do not automatically
  reproduce every production field query or its invalidation identity.
- Recent rejection metadata is bounded historical evidence. It omits proposed
  coordinates/connectivity, exact offending entity, complete old/fresh parameter
  state, imported dependencies and their versions. The final mesh may no longer
  contain the rejected cavity. Successful trials are not captured.
- A vertex-star neighborhood is complete for that extraction definition, not
  every operator's metric/geometry/contact/import support. Owner rank and the
  cavity interface are derivable from raw shards/connectivity but are not explicit
  fields in the cavity VTU. Full surface coverage/trace state is not serialized.
- Exact remaining work/resource state, eligibility and MPI peer schedule are not
  replayed. Success with a fresh replay budget is a feasibility experiment unless
  the relevant production state and rules have also been reproduced.

Reuse facilities before adding capture code: existing `NativeStageCapture` saves
owned cells, donors and boundary-parameter payloads; existing
`NativeTransactionCapture` additionally saves dependencies, old/fresh patches,
candidate point/IDs and diagnosis/reason. They are specialized bounded fixtures,
not already a production-faithful replay or complete MPI continuation. A selected
terminal witness should determine which missing piece actually matters.

## Suggestions to retain conditionally

The surface/volume coupling argument is useful at the level of an actual failing
branch. Inspect whether an existing complete private candidate is vetoed before
an available volume adjustment; do not presume coupling or collapse is absent.
The existing finite surface-splitting/geometry continuation was unexercised in199.
Build202now records3quality stages and2construction returns, so that coverage gap
is partly closed. Its complete EXIT is295q/294J/2110geometry-bad, after a smaller
shape tail and further geometry reduction. This warrants examining reclosure,
but does not prove that the same local neighborhoods cycled.

The suggested few-facet partition bound is a candidate to compare with existing
facilities, not an established missing optimization. Current `TriangleUnion::Upper`
already uses finite footprints, triangle coverage, source-simplex restriction,
plane lifting and outward bounds; adaptive traversal and filtered predicates also
exist. The proposed construction is not automatically equivalent to these, but
it needs measured residual cases and safe numerical/error accounting before it
can justify another bound. The attachment's literature is design context, not a
speed or correctness proof for this implementation; external papers were not
independently reviewed in this source-level assessment.

With build202terminal evidence preserved, inspect representative
boundary, constrained-interior, free-interior and matched clean regions. Reuse
complete census counters to separate waiting, unsuccessful search, publication
loss and churn. If inspection leaves a concrete rejection unexplained, select one
bounded existing transaction replay and verify reproduction before interpreting
alternatives. Choose one demonstrated correction, qualify affected behavior and
return to the full workflow. Do not add a competing construction strategy, cache,
scheduler, new options or weaker gates from the assessment alone.

## Terminal202 update and smallest next step

All6 raw owned/metric/reference/rejection shards are saved and complete. Combined
SU2/VTU assembly is now complete:164526points/810163tetrahedra, no omitted bad
cells/cavities or unknown measurements. See the
[actual mesh inspection](ADAPTATION_M6_INSPECTION_20261101.md). All ten exact owned processes are absent and
1403frozen pins unchanged. Use these witnesses before another full capture run.

One qualification to the replay discussion: production private repairs explicitly
use `ImportCellsLocal`/`SamplePointsLocal` with the complete current cavity and no
bootstrap fallback (`CNativeMetricTransport3D.hpp:277,307`, engine:3478,3486).
Missing original donors therefore need not prevent every local feasibility test.
Verify exact composition, geometry, support and evaluator agreement for the chosen
operator first. This still cannot reconstruct a historical candidate or its exact
resource/dependency state from the final inspection mesh alone.

No attachment proposal has been implemented as a repair policy. External papers
were not independently checked; no speedup or general local infeasibility follows
from this review.
