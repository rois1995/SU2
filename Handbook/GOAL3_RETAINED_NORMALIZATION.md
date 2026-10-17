# Retained original-field integration across normalization trials

Full Goal3 remains active/incomplete. This is a complete bounded FROZEN-FAMILY
replay, not production complexity initialization or an accepted remeshing stage.
Working root: SU2_NativeIntegrated; evidence relative to
`integration_evidence/native_3d_bl_v1/oneram6_uncertainty_v1`.

## What changed

The hidden native correction provider retains the exact original-query sensor
values and geometric wall samples after original-domain donor location. Every
scale trial reevaluates sensor and BL intersection at those same physical query
positions. Sensor-only original RAW9 P1 remains the source; these fixed quadrature
answers are not a replacement donor mesh or a field for arbitrary new positions.
Wall geometry is immutable, so its samples can be reused while the fade is
recomposed with the trial core. No BL tensor is interpolated across a coarse cell.

The shared `SU2Complexity::Normalize` receives a global background interval plus
BL-correction mean and3 empirical standard errors. Background bounds scale exactly
in ideal arithmetic for this homogeneous family. They are not outward numerical
certificates. Sampling error and adaptive scale selection remain heuristic.
An explicit5% tolerance is tested experimentally; no production default is chosen.
The bracket1/64..64 is diagnostic, not ADAP_HMIN/HMAX endpoints. Actual clipping,
sensor regradation and explicit normalization/adaptation stages remain unimplemented.

Same-scale requests reuse the estimate, with no new compositions. Insufficient
precision returns integration_limited rather than claiming a root. Trial work is
bounded by24 calls; fixed retained samples are never silently relabeled more
accurate. Query/cache storage is admitted against the existing2GiB service budget.

## Results

`retained_correction_normalization_v2/summary.json` covers all8 planned checks
and the existing35 controller assertions. All native cases run sequentially.

| Field/target | Result | Calls/distinct fields | Estimate / uncertainty |
|---|---|---:|---:|
|Planar16,5%,MPI1/4|estimated_tolerance|3/3|16.08603 /0.11147|
|Planar16,1e-6,MPI4|integration_limited|4/3|same fixed estimate|
|M6 seed947,1000000,5%,MPI2/4|estimated_tolerance|3/3|992919.7466 /35644.5956|
|M6 seed3557,1000000,5%,MPI4|estimated_tolerance|3/3|993123.2639 /35655.0797|
|M6 seed947,100000,5%,MPI4|outside_bracket|4/4|805701.4917 /28027.0897 at1/64|
|M6 seed947,1000000,1e-6,MPI4|integration_limited|4/3|no extra compositions on retry|

Both independent M6 seeds select logScale0.13862943611198905 (factor1.148698355).
The independent seed also satisfies the experimental5% condition at the same
selected field. This is useful held-out evidence, not a uniform statistical bound.
The100000 outcome excludes only the diagnostic family bracket; the separately
recorded strong-BL lower subset supports the broader conditional budget conflict.

Independent planar coarea references cover every trial within its heuristic
indicator; the selected true integral16.09227565 differs from target16 by0.5767%.
No new Duffy reference solutions, CFD solves or adapted-mesh reconstructions ran.

## Frozen-field identity and rejected verifier assumptions

At the unscaled preparation field, every retained source query compares cached
composition against current direct ApplyPoint: all8192 planar and5412/5364 M6 tensors are BITWISE IDENTICAL. No tensor
transport/geometry/height/positivity/shape/length/transfer gate was relaxed. Other
scales have planar integral/MPI checks, not per-trial direct tensor comparisons.

The initial v1 verifier incorrectly normalized a near-zero subtracted correction
against1. Its next attempt also imposed an unjustified64-epsilon scalar threshold.
The independent seed revealed a7.736e-10 relative old/new scalar-density difference
at a strongly anisotropic point. Exact current cached/direct tensor equality
separates field preservation from scalar spectral evaluation sensitivity after
recompilation. Mean absolute weighted-integral drift is1.412e-7 on an808697
correction (about1.75e-13 relative), far below its9414 empirical SE. It remains
recorded, not clipped or hidden; do not claim old/new scalar bitwise identity.
The final check requires exact tensor identity and1e-11 relative GLOBAL integral
compatibility, and retains the maximum scalar discrepancy independently.

`retained_correction_normalization_v1` preserves all failed verifier sources,
execution receipts, pre-audit header and successful partial native results. It is
not the completed v2 campaign. Provenance separates current consumers from archived
proposal producers; old successful inputs/PDFs are reused without rebuilding them.

## Work, reproducibility and limits

M6 retained3-trial normalization takes0.0073s MPI4/0.0108s MPI2 for seed947 and
0.0156s MPI4 for3557. No donor searches or wall queries occur DURING those trials.
These are contended replay observations, not lifecycle or scalability claims.
Preparation still includes donor-index build, routing, baseline density output,
wall samples and an extra direct-composition identity audit. Full proposal/PDF
preparation remains the separately saved~23s cost; it is not erased by this timing.

Final8-case+control wrapper49.41s, maximum processRSS666.7MiB, zero swaps. Final
single-object build75.88s/2.35GiB maximum processRSS, zero swaps. Earlier failed
verifier/build resource receipts remain in v1. One owned heavy job/nice19/thread1/
build-j1/MPI<=4, host38–40GiB available; no foreign jobs touched. Process peaks
are not aggregate MPI or full-lifecycle memory qualification.

`support_correction.py domain` now restores a missing128624240-byte original
export using committed immutable authorities, checks the original digest and
preserves the old manifest. Existing changed exports are rejected, never overwritten.
`domain_restore_check_v1` regenerated the exact digest, checked safe reuse/rejection
and removed only its temporary payload:7.22s/296.8MiB, zero swaps. The shared export
is retained locally for continued work and excluded from Git.

Reproduce after building UnitTests/test_driver with the pinned configuration:

```sh
python3 integration_evidence/native_3d_bl_v1/oneram6_uncertainty_v1/support_correction.py domain
OMP_NUM_THREADS=1 OPENBLAS_NUM_THREADS=1 MKL_NUM_THREADS=1 nice -n19 python3 \
 integration_evidence/native_3d_bl_v1/oneram6_uncertainty_v1/normalize_support_correction.py \
 m6_reachable947 global_correction_seed947_v3 1000000 .05 4 estimated_tolerance
```

Existing exact-provenance results resume; changed binary/source/config/input/helper
receipts require a fresh campaign identity, not relabeling old results.
Before production promotion, unify actual bound/gradation trial callbacks, global
sampling/work policy, parameter-consistency and collective file-I/O failure handling,
then explicit frozen normalization/adaptation/reassessment stages using the SAME
original donor field. No production tolerance/root, accepted M6 mesh, finer-mesh
reassessment, physical solution transfer or full adaptation speedup is established.
Do not respond with another subdivision/sample-limit ladder or unchanged300s run.
