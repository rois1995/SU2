# ONERA M6 thin-volume integration counterexample

Current production strategy: face-to-apex Duffy ray columns through the shared
parallel-chord controller. Read `columns_provenance.json`, `columns_validation.log`
and `normal_integral_check.json` for current scope. The full Goal3 is incomplete.

The complete supplied original wing and global source cell13 are retained in
`wing_cell13.dat`; its input identity is in `fixture.json`. Baseline geometry-first
integration exhausts65536 points before any density callback. Actual wall distances
agree with an independent nearest-triangle oracle. This is not a geometry-query or
Hessian error.

Current build passes7 directly affected cases/601990 assertions. M6 normal-density
integration retains1419 positive points, exact volume/first+second barycentric
moments, and independent relative error1.59272e-4 under unchanged5e-4 acceptance.
The converged independent reference is6.223134359667434e-5 (last change1.813e-7).
Band ratios1.5/1.25 produce the same rule because the shared controller has minimum
radial ratio2; these two results are not independent native refinement convergence.
No independent full tensor-density or whole adaptation proof follows.

Rejected approaches are preserved, not production fallbacks:

- `band_only_rejected_accuracy/`: accepted geometric partition, inaccurate normal integral.
- `local_density_rejected_work/`: strict tetra-local control exhausts work.
- `global_low_order_rejected_accuracy/`: native controls pass, independent normal integral fails.
- `positive_tetra_rejected_budget/`: higher-order positive tetra rule exhausts default point budget.

Root `before_*`, `bands_first*`, `*_density*`, `positive_*`, feature-partition files
and older build/check logs are historical attempts. Exact rejected sources and
binary hashes distinguish them from current column receipts. FEM positivity and
feature-plane checks do not establish current production dependencies or strategy.
`check_normal_integral.py` reuses only identity-matched converged references.

To repeat the focused native batch from this directory with the qualified binary:

```sh
SU2_NATIVE_BL_REPLAY="$PWD/wing_cell13.dat" \
  ../../build-integrated-v2/UnitTests/test_driver \
  '[NativeFacetedVolumeReplay3D],[NativeVolumeBL3D],[NativeParallelBL3D]' --reporter compact
OPENBLAS_NUM_THREADS=1 OMP_NUM_THREADS=1 python3 check_normal_integral.py
```

The separately prepared `../oneram6_frozen_v1` whole saved-state replay uses one
initial CFD iteration from the saved120-step restart and one follow-up only if
accepted. Its name reflects original intent; zero initial iterations are blocked
by contradictory unused-adjoint config checks. Exact startup failures are retained
under `invalid_*` prefixes. No production configuration validation was relaxed.

User-authorized Claude Code review prompt/result are separate advisory evidence;
inspect actual returned model and content before attributing conclusions. Review
was restricted to reading, with no editing/commands/simulations/nested delegation.
