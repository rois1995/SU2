# Restrict geometric edge preparation to active BL support

Checkpoint after `2ab7efce4169697e4d1da163a79e5c51047cf58d` on
`codex/native-3d-bl`. Full Goal3 remains ACTIVE/incomplete.

## What changed and why

Mandatory M6 seeding spends up to 172.95 seconds in geometric edge preparation.
Selection already reuses measured shared edges; another overlapping cache is not
the answer. An indexed local-update experiment preserved all numerical results but
provided no useful gain: legacy/candidate component ratios 0.994/0.995. It was
WITHDRAWN, with both kernels, patch, replay inputs and receipts retained in
`oneram6_uncertainty_v1/chord_local_updates_v1`. No native suite was run for it.

Retained change: the existing finite-feature envelope may be bounded by a distance
radius. A constant-radius feature represents inactive background; three padded
linear residual slabs discard or restrict features which cannot be nearer within
that radius. The default unbounded helper keeps existing behavior. Faceted 3D BL
edge preparation uses the existing fade thickness, on-wall tolerance and coordinate
roundoff padding. Beyond the radius that wall contributes nothing to the metric.
Every actual integration query still interpolates ORIGINAL sensor-only P1 and
composes the authoritative geometric BL, preserving finer demands and fade. No
metric samples are substituted by the capped geometric distance, and no quadrature,
acceptance threshold, normalization policy, cache or configuration was added.
Feature/piece/comparison/cut ceilings remain; the constant feature uses one slot.
The long-double envelope remains numerical, not an outward-rounded coverage proof.
2D and analytic geometry retain their existing kernels.

## Evidence and its limits

[Verified results and provenance](../integration_evidence/native_3d_bl_v1/oneram6_uncertainty_v1/chord_active_support_v1/summary.json).
69 actual original M6 edges, including the 32 largest candidate boxes in the declared
sample, admit 0–2544 wall faces. Two alternating-order passes take about
0.640/0.568 seconds and 0.639/0.567 seconds: approximately 11.3% lower component
time, with identical sampled active distances/crossings and 378 cuts in each pass.
This bounded contended geometry sample does NOT establish a full-seed or lifecycle
speedup. No extra complete M6/CFD run was spent on this gain alone.

Focused serial: 23 cases/502,489 assertions PASS, including microscopic/finite-wall
BL integration, corners, competing supports, curved/finer sensors and 2D controls.
MPI2 envelope/volume checks PASS. Five actual saved original-field edge replays
PASS; the two final corrected edge references retain exactly the same printed
lengths, 1.14109844258647347495 and 1.00047775663165992724. Earlier zero-length and
superseded classification experiments are not reference successes.

Three difficult-cell estimates against the highest saved two-chart references:

| Cell | Estimate | Relative reference error | Indicator covers observed error |
|---|---:|---:|---|
| 88 | 4.6567129076 | 0.7682% | yes |
| 157 | 1.3955704906 | 0.09178% | **no** |
| 329 | 0.2602543864 | 0.10737% | yes |

These are RAW9 frozen replay fields, not the remesher's distinct owner-UPPER6 field.
The references and indicators are numerical, not proven bounds. Cell157's indicator
underestimates observed error by about 1.91x. This reconfirms the global-calibration
limitation; no reliable default tolerance or 1e-6 normalization is established.
Do not attribute differences from older pilot results solely to this change: those
pilots also predate other code changes.

Actual N4/M2 initialized RANS/SA workflows at controlled scales1/10 PASS independent
geometry, positive volume, q/J, full composed length, height, sensor transport,
current+BDF conservation, SU2/CGNS and real restart. One final grid changes because
integration/normalization follows a different numerical path; each target remains
frozen during its stage. Independent budget discrepancies are about0.0960% and
0.3226%. This is two controlled planar workflows, not a new eight-workflow/general
curved/M6 qualification. Separate composed-gradation violations remain reported.

## Next substantial capability

A lightweight shape diagnostic reuses the sealed 64-parent private M6 geometry and
actual-query target tensors: 375/448 selected children fail the final shape limits;
minimum q≈0.01593 and J≈0.0007117. 66 unobserved original cells are excluded. These
are private intermediates, not a final mesh or proof of infeasibility. Full seeding
currently integrates all edges before allowing any volume operation, including
cells likely to be replaced. A complete scheduling change is more promising than
another small arithmetic or copy optimization.

[Progressive measurement design and acceptance obligations](GOAL3_PROGRESSIVE_MEASUREMENT_DESIGN.md)
is DESIGN ONLY. It must retain complete cavity dependencies, actual full metric
measurements, frozen fields, final whole-mesh certification and atomic rejection.
No accepted complete M6 mesh or whole-event affordability claim follows here.

All owned jobs terminal. One heavy job at a time, nice19/build-j1/thread1/MPI≤4;
available RAM about39–40 GiB and swap remained816 MiB. Build/resource receipts are
saved; per-process RSS is not a whole-job peak. No cluster submission, CFD advancement
or broad regression after each small edit. The original full goal stays intact.
