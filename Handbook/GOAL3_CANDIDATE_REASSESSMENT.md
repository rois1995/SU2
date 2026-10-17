# Goal3: original-field reassessment on the actual candidate partition

2026-10-17; full Goal3 ACTIVE/incomplete. Production base f6b4417.
[Complete evidence, exact pins and receipts](../integration_evidence/native_3d_bl_v1/oneram6_uncertainty_v1/candidate_reassessment_v1/README.md).

## What is implemented

Before native3D reference/flow publication, construct a bounded cell-importance
proposal from actual candidate centroids and volumes. Every centroid and sampled
point queries the unchanged original sensor-only P1 and actual geometric BL,
including existing explicitly eligible analytic extensions. Candidate tensors
never replace the original donor field. Geometry/height/positivity/q/J/full composed
edges, state preservation, current+BDF/SA transfer and restart checks are unchanged.
The parent original-source index is built once and reused for reader return.

Canonical geometry and256 virtual buckets make point streams independent of MPI
ownership without gathering the complete mesh. Every cell has positive admitted
probability. Unresolved probability intervals, storage or time retain the available
complete centroid sum and unknown uncertainty; invalid fields/source/protocols
reject. No cell or unresolved BL contribution is silently discarded. Sampling is
bounded to8192 points by this caller, and queries respect the existing64-point
source API. Native storage/deadline admission remains active. Strict determinant
magnitudes reuse existing exact predicates for integration density/volume.

The global MPI estimate uses centered residuals; its indicator is3 times the larger
of pointwise SEM and eight-block SEM. This is HEURISTIC, not a proven error bound,
calibrated default accuracy or guarantee against unseen tails. Density/interpolation
roundoff has no independently bounded term here; a tiny SEM is not an exact root. The new result and
separate `reassessment` phase expose mass, uncertainty, work and cost. The phase
includes source preparation moved from reader return; it is not wholly added cost.
No scale is changed during local operations and no correction epoch is implemented.

## Evidence and outcome

| Comparison | Result | Scope |
|---|---|---|
| Smooth P1/control,6 to384 integration tetrahedra | Indicator .00389351 to .000906442,4.30x lower; refined error .01634% | Subdivision control, not actual adaptation |
| Actual accepted planar RANS SA,6 to24 tetrahedra | Indicator .165326 to .0155528,10.63x lower; error .7910% to .05619%,14.08x lower | Same original P1/BL,8192 points, N4/M2 current+BDF/SA/restart/SU2+CGNS |
| Frozen complete M6 RAW9, coarse partition | Centroid596350.33; sampled979024.48 +/-101701.02 (10.39% heuristic) | Negative control; raw field differs from production coarsest trial |

M6 centroid misses about37% relative to the independent support-stream pooled
estimate948103.94. Sampling restores an estimate consistent with that approximate
holdout, but its point SEM33900 is much worse than existing equal-budget geometric
support sampling. Thus candidate weighting is not a replacement for complete-
support initialization on coarse input. No exact whole-M6 continuum reference or
accepted M6 mesh is asserted. The support holdout also has its own uncertainty.

The accepted planar reference is3.6941577302. Actual adapted estimate3.6962333832
has absolute error.002075653 inside indicator.015552794. The declared fixture
budget1 is below this constrained field's mass (about269.4% discrepancy); its
existing provisional/outside-bracket status is not a normalized-budget success.
Reassessment must not hide that discrepancy or rescale clipped tensors to fix it.

MPI1/2/4 sampler clouds match exactly; global reductions agree within declared
roundoff. All8192 M6 queries/donors/densities/weights match MPI2/4. Independent
original-sensor audit covers every sample;63 exact rational containment/weight
checks and32 independent composed checks on the accepted planar control pass.
Focused primitive/source faults, affected curved/reader lifecycle and independent
RANS geometry/full-field/transfer audit pass. Source bytes stay unchanged; one
source build serves reassessment and return. No full regression or CFD rerun.

## Cost, failures and remaining work

Actual planar reassessment phase .02094s with BL/.01661s without; these tiny cases
are correctness measurements, not scaling or unsteady affordability. M6 metadata
proposal .2624s/draw .00388s on rank0 excludes the centroid/source preparation;
all preparation belongs in full cost. Replay preparation is not production-speed
qualification. Build/phase/global/reduction/memory receipts are retained.

One owned heavy job/nice19/build-j1/thread1/MPI<=4. Largest build process2.41GiB,
replay667MiB; observed available RAM about39–41GiB, existing816MiB swap unchanged,
GNU time swaps0. These are process maxima and observations, not whole-job peaks.
Initial compile typos, failed128-point source batch, and report-only NumPy Boolean
serialization failure are preserved. The64-point fix passed focused checks before
MPI/lifecycle checks. Completed runtime cases were reused after the reporting
failure; archived executed helper and explicit report-helper transition verify it.

The refinement hypothesis is supported on one actual accepted planar case, not on
M6 or general thin/curved/overlapping layers. Remaining: calibrated global error/
allocation, an explicit bounded normalization/adaptation correction stage from
original unclipped frames and bounds/gradation, accepted complete difficult M6
construction, viscous accuracy and whole-event affordability. No extra sampler,
subdivision/count ladder or unchanged full M6 retry is justified by this result.
