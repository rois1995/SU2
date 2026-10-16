# Goal 3 — next construction capability: metric-driven intermediate fronts

Full Goal3 remains active. This is the next construction step after the complete
accepted curved-mesh milestone, not a new goal or a replacement for its gates.
This handbook preserves the rejected fixed-band extension and its diagnosis.
The subsequent [coordinated coarsened-front construction](GOAL3_COARSENED_METRIC_FRONTS.md)
now accepts the complete high-ratio target under unchanged gates. The qualified
pre-front MPI/actual RANS checkpoint remains separately scoped.

## Limitation established before implementation

The current normal size is
`hn(d) = max(h, 2*(h+(growth-1)*d)/(growth+1))`. Its integral is piecewise exact:
`d/h` up to `h/2`, then
`0.5 + (growth+1)/(2*(growth-1))*log((h+(growth-1)*d)/(h+(growth-1)*h/2))`.
The first-band proposal uses normal span1.62. For growth1.2/thickness.2, the
unfaded region ends at.18. Ignoring the remaining fade gives a lower bound:

| First height | Unfaded metric span | Span after ideal first front | Minimum remaining segments with length<=1.8 |
|---|---:|---:|---:|
| .05 | 2.95858 | 1.33858 | 1 |
| .01 | 8.36910 | 6.74910 | 4 |
| .001 | 19.83584 | 18.21584 | 11 |

[70-digit exact-form/independent-quadrature check](../integration_evidence/native_3d_bl_v1/region_construction_v1/normal_span_design_v1/result.json).
This is an ideal radial column; it proves the span obstruction to one direct
transition edge. It is not an actual high-ratio mesh, a bound on total cells,
a geometry/height feasibility proof or a guarantee of acceptable tetrahedra.
A direct outer-to-inner edge is present in the current Band connectivity regardless
of its interface diagonal. Repositioning one transition apex cannot remove that
edge or its metric demand. Another local placement fallback misses the limitation.

## Connected design to implement

Keep the coordinated surface and prescribed-height first-row construction. Replace
the single transition jump with a metric-directed sequence of complete intermediate
fronts. Select advancement using actual original-P1-plus-geometric-BL point/edge
queries, not Euler/RANS labels, fixture coordinates or a fixed number of rows.
Each front must preserve shared interfaces and planar-cap constraints. Stop only
when the admitted core interface is outside the requested BL support; stronger
sensor demands continue into the existing hierarchical core.

Transition bands do not require a prescribed-height apex per wall face. Assess a
consistent three-tetrahedron prism subdivision for these free bands, reusing global
vertex ordering for shared diagonals. This may avoid multiplying the current
apex-and-edge-star cell count at every front. Positive orientation, physical cap
geometry and actual q/J/full-edge checks must establish whether that subdivision
is admissible; its combinatorial cell count alone establishes nothing.

No source, fade, first-height, q/J, geometry or edge limit changes. Count all fronts,
trials and generated cells in cumulative work/time/memory admission. If the declared
work limit cannot contain the required mesh, report that limit with the retained
input and rejected proposal; do not raise it silently or publish partial layers.
Multi-front radial construction does not by itself solve interacting/nonvisible/
exterior geometry or make generation distributed across workers.

## Acceptance and cost

Use the original difficult/stronger rotated target, a high thickness/height ratio
that defeats the old direct transition, and a distinct varying sensor. Require a
complete mesh with exact embedding/geometry/feet/height, q/J, full composed lengths
and original-source preservation. Reuse the new independent full-cylinder metric
reference for its actual constant-sensor scope; nonconstant sources require their
own original-P1 reference. Then run the affected MPI/actual CFD lifecycle controls
at a meaningful integration boundary, preserving the full campaign and failures.

Measure front count, cell count, field/edge calls and work before optimizing. Keep
construction planning/bands/core/validation clocks separate when measuring runtime;
include metric/partition/migration/return/rebuild/transfer/output for affordability.
No scaling or adaptation/CFD ratio follows from a small local correctness run.

## Saved complete-target prism assessment

`front_prism_design_v1` retains the accepted first row and core and replaces the
transition with ordered3-tet prisms. Complete face conformity and actual-centroid
q/J pass, but eight critical full edges reach2.026214617>1.8; preserve the rejected
2184-cell proposal/reference. A two-step proposalv2 has2688 cells and critical
full edges<=1.71741, but actual-centroid qmin.185057/Jmin.030779 fail. Neither is
accepted or a full embedding audit. These bounded saved-mesh checks took under
a second apiece, without new SU2 runs. The production change therefore retains
the established coordinated star-band construction instead of adding prism
connectivity. Full-field-controlled advancement, positive progress and event
work/time/cell admission limit the sequence; final validation remains unchanged.

## Native intermediate-front result and next strategy

The extension uses actual composed radial-prefix integrals, positive inward
progress and charged front trials, reusing Band/first row/core/final gates. It
compiles. Original rotatedv12 independently accepts3108 cells/work4057 with exactly
the same geometric tetrahedra, actual q/J/tensors and full edges as auditedv10;
all frozen source/reference bytes match. The complete independent numerical metric
reference is reused by explicit exact comparison, not by assuming similar output.

High-ratio height.01/T.2/length.2 fails at4097 charged work before completing the
next band. Three private fronts/3367 cells are saved as rejected; retained input
is independently identical. There are84 physical wall triangles and each current
Band costs840 tetrahedra. The unfaded remaining normal span requires at least4
more bands even at full1.8 radial allowance:4200 cells already exceed4096 before
core/sampling/planning. See `native_front_constant_v1/construction_cost_lower_bound.json`.
A larger step alone cannot fix this construction-class obstruction.

The metric-nearest wall-apex hypothesis was checked independently rather than
added to production: original apex shapes already pass at36 angular intervals.
Both full surface diagonals are too long there (best-around maximum2.04172);42
intervals give1.78315. The frozen field genuinely requires the denser wall, so
apex placement does not remove this sampling cost. The source and gates stay.

The next connected design must reduce transition cost while coordinating its
point distribution/connectivity with the core interface. The failed simple
prisms show why efficient cell counts alone are insufficient: direct prisms
violate lengths and shorter steps violate actual q/J near the fade. Investigate
whole-front distribution and terminal-core placement together on saved complete
meshes before another native build. Do not add local fallback ladders or increase
work/time. Six production phase clocks now separate sampling/first-band/transition/
junction/core/validation; new MPI/actual RANS/profile qualification remains pending.
