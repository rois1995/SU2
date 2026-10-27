# Conservative overlap clipping: subnormal-distance failure

Current evidence, 2026-10-27. Integrated fix is building in build15; whole-workflow
qualification is pending. All conservation/restart gates remain unchanged.

## Trigger and diagnosis

Build14 connected_fix/session39461 completed both curved Refine and Coarsen, but the
RANS BDF2 transfer check failed by0.0013328061 against2e-9. The accepted128-cell mesh
has target volume1.0018647649702257, enclosing the original unit-volume donor: its
bottom cubic boundary moves outward; the other boundary planes are unchanged.
Yet transfer reported fill0.022975264580252047 and loss0.02111049961002609. Nearest
boundary classification then assigned spurious parts to open markers. Euler's
constant field remained constant, which alone did not detect lost overlap coverage.

`staged57/transfer_coverage/overlap_probe.cpp` runs all6 donor ×128 saved target cells
through the actual linked kernels, bypassing ADT search, boundary classification
and solver initialization. It reproduces the original fill/lost totals. Raw clipping
is nonfinite for target12/donors2,3 and target50/donors2,3. The first target contains
coordinates−denorm_min on original coordinate planes; these are finite inputs.
Dual-piece sums agree with raw overlap for finite intersections. Signed donor
coverage is saved, so diagnosis does not assume all coverage errors have one sign.

The 3D clipper computed intersections as `(wk*xk + wr*xr)/(wk+wr)`. Subnormal signed
distances can underflow the weighted coordinates; reciprocal optimization can
also overflow before the quotient. The nonfinite raw volume then failed the
positive-overlap predicate and was silently classified as empty. This explains
actual missing intersections, not just a possible marker-label defect.

## Connected correction

Both polygon and polyhedron clipping now normalize opposite-sign distances by an
exact power of two (`frexp`/`ldexp`) before computing the intersection fraction.
The polyhedron uses normalized coordinate weights. Raw and dual-piece nonfinite
volumes fail explicitly instead of becoming empty intersections.

The identical saved all-pair replay after the arithmetic fix gives raw overlap
0.99999999999999933, summed dual overlap0.99999999999999956,
fill0.0018647649702263503 and loss3.6082248300317588e-16. Every donor's coverage is
1/6 to roundoff. This is focused arithmetic evidence; it does not replace the full
transfer/restart run. No mesh, marker classification, tolerance or final gate changed.

Regressions cover the exact first failing cell pair, finite overlap/dual moments,
reciprocal overlap symmetry, subnormal endpoint cuts and halfspace coefficient
scaling in2D/3D. Existing clipping/projection/transfer tests join broad serial/MPI6
qualification. The original curved RANS runtime expectation is retained unchanged.

## Other connected corrections and provenance

Two new tests had invalid premises: the80-cell star already satisfied final quality,
and a sheared box's center did not project inside every wall triangle. The star now
asserts initial quality debt and valid sizes before requiring a move. An orthogonal
stretch with its pullback metric preserves a valid first-height fixture. Ownership,
parameter rollback, collateral-direction service and budget-refusal gates remain.
A phase census no longer subtracts donor time from an update timer that did not run.

Receipts: `integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/`
- `cases/connected_fix_20261027T082636_1472866`: failed full-workflow assertions,
  projection/state/mesh outputs and pins.
- `transfer_coverage`: actual all-pair reproducer, literal target coordinates,
  command manifest, before/after logs and endpoint probe.
- `build14_*`: terminal successful build; `build15_*`: integrated rebuild.

Full M6 quality convergence, curved chart mobility, DC-17 and DC-18 remain open.
