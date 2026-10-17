# Global complexity: support envelopes and feasibility experiments

Current source checkpoint base d3e16ca7ecb3d8ae02750327ffae9de0a0d3a1ac.
All paths below are relative to
`integration_evidence/native_3d_bl_v1/oneram6_uncertainty_v1`.
These experiments do not change production normalization, the frozen field or
mesh acceptance. Full Goal3 remains incomplete. Preserve the original host-rank
raw9 field for volume queries; owner-UPPER6 edge data is a distinct representation.

## Finite envelopes: rejected as useful global stopping criteria

`support_bound.py`, `support_mass_envelope_v1` and `support_mass_envelope_v2`
retain positive unresolved-cell estimates and calculate finite exact-arithmetic
support upper envelopes. These are not outward-rounded machine certificates.
Face prisms, edge sectors and vertex cones cover possible wall support. Reciprocal
normal-height moments use the lower size `max(h0, 2(g-1)/(g+1)*r)`; density bounds
use positive sensor/tangent/normal Loewner majorants. Conservative support overlap
may count volume several times; this is permitted only for an upper envelope.

The second variant retains an anisotropic sensor matrix majorant and an
angle-weighted logarithmic tangential bound. It does not change the metric.
The three saved reference cells lie within both envelopes. However, the sum of
error envelopes for the14 previously unresolved cells is1099544.52 in v1 and
1093315.57 in v2: only0.57% improvement and over ten times the requested complete
budget100000. Cell88 upper583603.27 versus reference4.69276 is particularly loose.
Large coarse support and uniform fine tangential extrema lose the essential
space/direction correlation. Do not adopt these envelopes as useful uncertainty
stopping decisions, or respond with another scalar-bound/subdivision ladder.

Work was bounded to17 original cells: v1 elapsed5.44s/peak processRSS299.7MiB;
v2 elapsed4.75s/301.4MiB, zero swaps. These are local analysis receipts, not full
normalization/lifecycle measurements. Both retain source snapshots and input,
reference, production metric and prefix digests. The self-check includes random
SPD/directional bounds and growth-law limits.

## Supporting-plane floor: inconclusive

`wall_floor.py`, `support_planes.cpp` and `supporting_wall_floor_v1` build disjoint
cones inside original boundary tetrahedra. QHull only selects candidate original
wall facets; native exact orientation checks every original wall vertex against
each supporting plane. Native exact containment checks the cone apex. Projection
inside the original triangle uses integer arithmetic on the binary64 coordinates.
Only one cone contribution per original parent cell is counted. The mass uses a
numerical analytic reciprocal-height lower integral and a log-tangent Jensen
lower density; it is not an outward-rounded numerical certificate.

Only348 of38800 original wall facets survived this convex-support preselection.
All348 exact geometric checks pass, in distinct original cells. Their numerical
lower mass2524.9616 (looser vertex bound2327.3848) does NOT establish that100000 is
infeasible. Whole producer elapsed3.27s. Global support is too restrictive for the
actual wing triangulation; do not extrapolate this subset to all wall facets.

The standalone helper links the existing exact predicate object, without CFD:

```sh
c++ -std=c++17 -O2 -I. \
 integration_evidence/native_3d_bl_v1/oneram6_uncertainty_v1/support_planes.cpp \
 integration_evidence/build-integrated-v2/Common/src/libSU2AccurateSum.a.p/adaptation_CNativePredicates3D.cpp.o \
 -o integration_evidence/native_3d_bl_v1/oneram6_uncertainty_v1/supporting_wall_floor_v1/support_planes
```

The local ELF is not tracked. The tiny self-check packet is retained. Full input
geometry packets are deterministic, hashed in provenance and deleted after use.
The original wall, mesh and captured field remain the input authorities.

## Direct wall-support sampling: preparation only

`proposal_budget.py` and `geometric_correction_sampler_v1/preparation.json` compute
proposal weights, not a global integral. The full-strength radius is0.009. The
full-space face/edge/vertex proxy masses are1549728.86/164530.34/390431.16,
sum2104690.36. They are NOT the physical complexity or a domain lower bound.
Preparation1.43s/peak processRSS79.3MiB, zero swaps. A general sampler has not been
implemented; it would require complete overlap density and original-domain
coverage, including corners and edges. Do not equate weights with validated
sensor-plus-BL correction integration.

The smaller next experiment, `wall_prism.py`, uses only face prisms clipped to
their actual original boundary tetrahedron. Select at most one face per parent
tetrahedron, so admitted subsets are disjoint without a new donor-search service.
Both normal signs are sampled, then the existing exact original-cell predicate
checks domain membership. This is a SUBSET lower mass, not full-field integration.
It can falsify a requested budget but cannot certify that a feasible budget exists.

Sample face probabilities are proportional face area times tangent proxy density;
radial density is proportional `1/max(h0, 2(g-1)/(g+1)*r)`. Inverse PDF uses actual
rounded CDF increments. At each admitted query the unchanged native wall model
provides full-strength density from its retained matrix logarithm; spectral density
is compared separately. Contributions beyond full strength are zero, preserving
the meaning of this lower subset without changing the production fade.

For a chosen positive cap Z, replace each nonnegative importance weight Y by
`min(Y,Z)`. This reduces the expectation, so it remains a lower mass even if the
proxy does not majorize the true density. The bounded-sample one-sided Hoeffding
radius is `Z sqrt(log(1/delta)/(2N))`. It is a statistical statement conditional on
ideal independent continuous samples, exact PDF/Jacobian/field and original-domain
embedding. Floating sampling/coordinates are numerical, not machine-certified.
Fixed N and delta are selected before sampling; no target-triggered optional stop.
An independent seed and matching MPI query outputs are required before interpreting
an apparent budget conflict. No CFD rerun or broad regression is needed for this
fixture-only extension; existing production algorithms are unchanged.

## Completed numerical feasibility result

Two predeclared16384-sample seeds731/1741 give subset masses384178.9484 and
382097.6485. The one-sided Hoeffding radius is19706.0571 per seed atdelta0.005,
giving lower estimates364472.8913 and362391.5914. Their means differ0.543%.
Both lower estimates hold simultaneously with at least99% confidence CONDITIONAL
on the stated ideal sampling/field assumptions; this is not a proven floating
bound. Neither seed clipped any positive weight.5315/5331 samples lie in their
original parent cell, all within the actual native full-strength band. All38800
wall faces have distinct original parent tetrahedra in this mesh. Actual native
logarithm/spectral density discrepancies are at most5.344e-9 relative, far smaller
than the statistical margin; these are two evaluations of the same metric law,
not independent physical models.

The first seed's ALL16384 rows, including exact containment flags and native
densities, match identically on1/4 MPI ranks. Statistics here are merged from the
saved rank rows afterward. This qualifies MPI query consistency, NOT a production
collective uncertainty allocator or full-field root solve.

The conservative lower of these two numerical lower estimates is3.62392 times
the requested100000 complexity (+262.39%). Bare-wall density within these disjoint
strong-band subsets is a lower contribution to the composed sensor-plus-BL field:
metric intersection preserves the full-strength wall demand and finer sensors
only add demand. The original P1 field is not changed, nor is a wall tensor sampled
at nodes and spread through the original cells. This estimate requires no sensor
scale, refit or core-dependent fade.

Consequently the proposed initial refinement cannot make the SAME immutable
continuum field attain100000 merely by resolving integration better. The observed
integration difficulty and the independent reconstruction/time-limit failure
remain real, separate issues. This result does not prove a different feasible
budget, construct a mesh, or refute that refinement might ease quadrature. Current
production normalization already has an out-of-range branch that can choose the
closest attainable complexity with a warning. Reaching that branch efficiently,
quantifying its actual mass, and respecting an explicit construction/work budget
remain unimplemented here. Do not silently change100000, h0, wall tangent sizes,
fade or any acceptance gate to make this case pass.

Preparation3.83/4.72s, process peak about301MiB. Native replay.713/.799s for the two
serial seeds and.936s for the MPI4 repetition; whole Python wrappers3.2–3.5s
including provenance I/O/launch/analysis. Query processes peak46–51MiB (maximum
process, not MPI aggregate), zero swaps. Fixture-only incremental build68.54s,
peak2.35GiB, one worker/nice19. Host stayed around39–40GiB available. This is cheap
feasibility evidence, not complete adaptation throughput or scaling performance.
All owned jobs are terminal. No CFD step, new normalization or accepted grid.

Receipts: `wall_prism_budget_summary_v1.json`, `wall_prism_floor_seed731_v2` and
`wall_prism_floor_seed1741_v1`. The preparation failure inseed731_v1 is preserved:
extended-precision parsed text was initially used as a binary64 dictionary key.
Rounding to the actual native representation matches ALL19506 original wall nodes
exactly and all38800 face keys; no nearest-node/tolerance substitution was used.

Reproduce from the repository root, with a rebuilt test_driver and fresh labels:

```sh
OMP_NUM_THREADS=1 OPENBLAS_NUM_THREADS=1 MKL_NUM_THREADS=1 \
 nice -n19 taskset -c 0 python3 \
 integration_evidence/native_3d_bl_v1/oneram6_uncertainty_v1/wall_prism.py \
 prepare NEW_LABEL 731 16384
OMP_NUM_THREADS=1 OPENBLAS_NUM_THREADS=1 MKL_NUM_THREADS=1 \
 nice -n19 taskset -c 0 python3 \
 integration_evidence/native_3d_bl_v1/oneram6_uncertainty_v1/wall_prism.py \
 run NEW_LABEL 1
```

Run the last command with4 ranks only after the previous run ends; the helper
launches MPI and compares every row to the saved serial result. Preparation refuses
to overwrite evidence. Runtime reuse verifies original inputs and source/build/
binary/configuration pins through the existing replay runner. Captured input and
sample mapping are retained for exact queries; no solver checkpoint duplication.
