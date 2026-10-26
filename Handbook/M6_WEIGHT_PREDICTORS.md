# M6 partition weights — diagnostic55

Status: source audit and offline descriptors reproduced; predictive calibration
and alternate offline partitioning not performed. [Decision](M6_DIAGNOSTIC_DECISION.md).

## Current implementation

`CNativePartition3D.cpp:15–45`, `WorkWeights`, samples the original sensor and
composed target at each tetrahedron centroid, and target tensors at its four nodes.
It sums a baseline of one, a sensor metric-volume/unit-regular-tet-volume term,
logarithmic long/short-edge pressure and centroid q/J deficits. Edge pressures use
the maximum of centroid and endpoint metric norms, not the authoritative integrated
edge length. The result is rounded up to a positive integer and capped at 1024.
Invalid geometry/tensors and nonfinite/nonpositive work reject. This proxy neither
changes the target nor certifies a cell/edge.

Weights are computed once when the remesher's initial working partition is prepared,
not after every edit. The production path uses original-source queries; it does not
replace the authoritative field with log-Euclidean nodal averaging. For this Euler
input the offline descriptor uses original affine P1 centroid tensors and no BL
modification. Its binary64 arithmetic is a transcription, not the production scaled
long-double predicate implementation.

The graph has one vertex per tetrahedron, adjacency across nonphysical triangular
faces. Constraints are `(work,1)` with equal per-part target shares and `(1.15,1.15)`
tolerance. ParMETIS options are `{1,0,0}`; single-active-graph-rank fallback uses
METIS seed zero. There are no edge weights. Source headers report ParMETIS 4.0.3
and METIS 5.1.0, `idx_t` width 64, `real_t` width 32; exact headers and runtime
provenance are pinned. No API replacement has been made.

## Measured and reproduced distribution

Validation54 predicted-work sum is 5,664,358 before and after migration. Max/mean
falls from 4.6991 to 1.08449; cell max/mean is 1.08468 afterwards. 102,883 cells
migrate. Graph+partition+migration take at most 3.200 s/rank; original donor
migration adds its separate 0.512–0.580 s phase. The logged graph has 1,164,482
directed arcs. The production structure stores edgecut, but the current performance
record does not print it; no numerical edge-cut claim is made here.

The independent current-formula transcription sums to exactly the same 5,664,358.
Its min/median/p95/max are 2/4/153/591, with zero weights at the 1024 cap and zero
raw values above it. This is replica-only evidence against clipping as the explanation
for this input. Exact equality of every individual production weight is not established
because those values were not exported. Per-rank clipping is therefore not claimed;
the offline spatial-bin clipping counts are all zero.

## Candidate comparison

`diagnostic55/weights.py` computes on the same original tetrahedra:

1. Uniform per-tetrahedron weights.
2. Current formula with its ceiling and cap, independently transcribed.
3. Target metric complexity `volume*sqrt(det(M_target_at_centroid))`.
4. Separate insertion, collapse and optimization terms from the natural-metric
   intersection model. Equal coefficients are an illustrative descriptor, not a fit.

For B formed by the three edges from vertex zero, the regular unit tetrahedron
Gram matrix G has diagonal 1 and off-diagonal 1/2. The natural metric is
`B^-T G B^-1`. Generalized eigenvalues of `(B^T M B,G)` give the intersection
density by clipping those eigenvalues below one. Thus the natural metric volume
is `1/(6 sqrt(2))` for every nondegenerate input tetrahedron. Assertions check SPD,
finite positive eigenvalues, nonnegative insertion/collapse terms and intersection
density at least as large as either operand, to stated numerical tolerance.

Self-check: an anisotropic tetrahedron's aligned target `diag(100,1,1)` and rotated
target `diag(1,100,1)` have equal determinant and anisotropy. Both give target
complexity 0.11785113. The aligned insertion/collapse terms are zero; the rotated
terms are each 1.06066017. A simultaneous rigid rotation of mesh and metric preserves
the result. The aligned regular-in-metric case also checks q=J=1 and raw current
weight two. These establish descriptor behavior, not runtime prediction.

`weight_predictor_comparison.csv` contains eight explicitly spatial bins for all
four descriptors; `offline_weight_descriptors.npz` retains cell-level values locally.
The bins are not MPI ownership partitions or independent CPU observations. The
authoritative P1 metric is never modified by this computation.

## What cannot yet be concluded

There are no measured per-region useful-compute labels, so prediction error, ranking
accuracy, calibrated coefficients and held-out performance are unavailable. Six
synchronized rank totals cannot identify per-cell CPU costs. No model is selected.
No alternative graph partition was run: the current proxy is already balanced and
there is no validated cost target against which to assess a different partition.
Offline predicted balance alone would not demonstrate a speedup.

BL columns/layers, surface geometry work and changing repair difficulty require
additional observations. The early prefix and late full run have different ranks
dominating refusals, which further limits extrapolating one fixed rank aggregate.
Return migration, CFD reconstruction and conservative transfer costs on a successful
full M6 remain unmeasured. AdaptiveRepart/RefineKway are therefore future candidates,
not recommended replacements for the current PartKway call.
