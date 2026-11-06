# M6 metric floor investigation — isolated side diagnostic, 2026-11-06

Production sources, configurations, solver binaries and the main adaptation cycle were not modified. All new diagnostic files are in this directory. No CFD/remeshing job was launched.

## Follow-up completed: gradation and matched-target comparison

A standalone serial transcription now uses the ORIGINAL CBlasStructure eigensolver header and the same synchronous native gradation equations, ordered complete global neighborhoods, tensor intersection, physical bounds, 256-sweep cap and tolerances. It is not a replacement production implementation. An analytic two-node transport case passes.

The OLD field reproduction converges in EXACTLY the recorded 198 sweeps. Its complexity is 49,999.998582669 versus production 49,999.998577188. Every nodal tensor agrees with the recorded production field within 7.64e-9 relative Frobenius error (median 1.44e-9). There are zero directed edges above the existing 1+1e-5 gradation audit threshold. This substantially strengthens the complete metric-chain numerical diagnosis, beyond merely matching the pre-gradation integral.

The p=1 local-floor alternative, including gradation and solving its global factor again, converges in 110 sweeps at complexity 50,000.0215012. Relative complexity error is 4.31e-7, within the source's unchanged 1e-6 tolerance. It has ZERO violations on all 778,330 directed edges; maximum ratio is 1.000000099976. Minimum eigenvalue .00258789, maximum 5.35579e8 and maximum aspect ratio 38.2593 remain inside HMAX100/HMIN1e-6/ARMAX1e6. No norm, sensor, target or gate was changed.

| Matched, graded measurement | Recorded/reproduced original | Diagnostic local-floor candidate |
|---|---:|---:|
| Budget in the old farfield plateau region | 79.70% | 33.02% |
| Shock-direction spacing, y=.55 | 11.49% chord | 5.39% chord |
| Shock-direction spacing, y=.85 | 14.82% chord | 5.94% chord |
| Root trailing-corner budget, radius .05 | .0497% | .7865% |
| Tip trailing-corner budget, radius .05 | 1.8685% | 6.4385% |

A broadened independent sample selects the strongest 20% of upper-surface chordwise compression triangles in 11 span strips from y=.15 to1.15, restricting x/c to .15–.8. ALL11 have finer cross-gradient target spacing: original10.0–15.0% chord versus candidate4.37–6.54%. This is an explicit sampled compression diagnostic, not exact whole-shock extraction or a resolution acceptance gate.

The immediate corners receive stronger requests too. At near-normal-dominated tip witness15079, h_min is approximately4.32e-5 for the candidate versus2.62e-4 before. This is not evidence those requests are all physically justified, nor that construction can realize them. Main adaptation work must retain geometry/quality/height constraints.

The residual farfield allocation is not entirely the old numerical plateau: the saved flow contains finite spatial variations, and reconstructed curvature at sampled farfield stencils is well above arithmetic subtraction noise. Median farfield recovery residuals are about .04–.05 in the most distant band. This does NOT establish those small disturbances are an accurate converged physical farfield; the seed was force-qualified, not residual-converged. Under the unchanged global-volume L1 objective, tiny distributed errors over this very large domain can legitimately receive substantial weight. No farfield masking, norm change or filtering policy has been selected.

New evidence: graded_comparison.json, gradation_reproduction_assessment.json, final_provenance.json, gradation_unit_test.json and farfield_curvature_assessment.json. The first standalone original-input attempt used the wrong scalar calibration (global ungraded minimum instead of isotropic plateau); it is explicitly superseded in gradation_scalar_calibration_correction.json. Corrected inputs and all reported final comparisons use the plateau. This diagnostic mistake never affected production files.

Visualization: M6_original_wing_metric_comparison.vtu contains the ORIGINAL wing surface, actual initial CFD Mach/pressure/Cp, actual/candidate tensors and sizes. Compare CELL arrays actual_h_across_Mach_gradient and candidate_h_across_Mach_gradient, or actual_h_surface_min/candidate_h_surface_min. POINT h_min can be mainly normal. The candidate is diagnostic, NOT an adapted or accepted mesh. All exported arrays were decoded and verified exactly; no ParaView GUI qualification is claimed.

Reproduce the additional stages after the earlier two Python commands:

1. Compile gradation_probe.cpp with g++ -std=c++17 -O2 to gradation_probe (one low-priority worker). It directly includes the existing original eigensolver header.
2. Run prepare_gradation_inputs.py.
3. Run gradation_probe original_gradation_input.bin original_graded_corrected 1 55.
4. Run gradation_probe local_gradation_input.bin local_graded_scale04236855 0.4236855 55. The selected scalar is the recorded result of five bounded trial evaluations (1,.5,.4334,.42395,.4236855), all with gradation included. This is not post-hoc tensor rescaling.
5. Run export_surface_comparison.py after both graded outputs exist.

Production integration still requires replacing the global-dependent regularization safely in the shared 2D/3D path, exact zero/flat-sensor handling, stable determinant/normalization arithmetic, existing geometry/BL callers and MPI qualification. The successful standalone reproduction is strong numerical evidence, not a production build or full workflow acceptance. No remeshing speedup is claimed from standalone sweep/runtime counts.

## Established failure

The initial build217 metric has an isotropic plateau at eigenvalue 0.0718601939500102, or requested size 3.7304034791. Its 4,621 nodes contribute 39,850.4824 of the independently integrated 49,999.9985772 complexity units (79.70097%). The independent nodal-volume integration agrees with the solver's reported total to approximately 2e-10 absolute. Configured HMAX is 100, so this is not the requested physical maximum-size limit.

The code in SU2_CFD/src/solvers/CSolver.cpp:2709–2713 divides each Hessian by its global maximum absolute component and clamps every absolute eigenvalue to 1e-16. That clamp precedes the Lp determinant powers and sensor normalization. It therefore imposes a positive background depending on a remote largest Hessian, rather than merely preventing a local singular determinant.

Independent full-grid recovery from the actual original-grid mesh and restart reproduces the recorded 79,058 first-ring and 43,602 grown point-sensor fits, 79,038 residuals above 0.05, and mean relative residual 0.15159387338. Largest recovered absolute Hessian components are approximately 1.25234e13 for Mach and 1.45427e18 for pressure. The maximum's trailing-edge neighborhood is tiny, so enormous local recovery values can control global clipping without directly consuming much complexity there.

The independently reconstructed old two-sensor pre-gradation field, at the scalar fixed by the observed plateau, integrates to 40,237.3157873, consistent with the logged 40,237.3. This is a scalar-calibrated reproduction, not an independently repeated native normalization. About 99.04% of that ungraded field is on the plateau. Extending the minimum final tensor uniformly over the entire domain alone costs 40,230.8414. Gradation materially changes the field: the final recorded plateau share is 79.7%, rather than 99%.

At 44,788 retained original nodes with unchanged coordinates, including 11,033 original wing nodes, initial and final nodal tensors were previously checked to match exactly. This failure exists before remeshing; it is not explained by metric transport at those points.

## Bounded alternative and tests

The diagnostic alternative regularizes nonzero Hessians relative to their own largest eigenvalue (using the existing 1e14 sensor conditioning ratio), handles absent/flat demands separately, and evaluates Lp determinant/normalization factors in logarithms. It is a hypothesis, not a production patch or an endorsed new parameter.

Tests pass for sensor-unit invariance, remote-outlier isolation, coordinate rotation of well-resolved tensors, rank-deficient and flat fields, finite positive physically bounded metrics at the same 50k target, and two-sensor intersection dominance/commutation/idempotence/flat-sensor handling. The synthetic old global-floor calculation changes the feature-to-weak-region metric ratio from 301,709 to 75.8 when a negligible-volume remote outlier is introduced; the alternative preserves the ratio. Another synthetic old calculation changes a feature tensor by 19.3% under coordinate rotation. The alternative removes that artificial change at the weak/feature points. The extreme anisotropic outlier itself still exhibits about 4e-7 floating-point rotational error; do not claim exact invariance at extreme condition numbers.

Full original-grid independent two-sensor comparisons, all normalized to 50k, WITHOUT native gradation:

| Diagnostic | Old plateau region budget | Shock-direction spacing / local chord, y=.55 | Same, y=.85 |
|---|---:|---:|---:|
| Old global floor, p=1 | 99.04% | 44.30% | 53.68% |
| Local relative floor, p=1 | 62.55% | 5.19% | 6.11% |
| Local relative floor, p=2, sensitivity only | 2.39% | 2.85% | 3.87% |
| Local relative floor, p=4, sensitivity only | 0.0053% | 8.41% | 11.18% |
| Actual recorded production result WITH gradation | 79.70% | 11.49% | 14.82% |

The last row is not a controlled comparison with ungraded alternatives. No speedup, native-graded candidate metric, improved adapted mesh, or mesh acceptance is established. Raising p is not selected: the nonmonotonic shock response demonstrates why a global norm change needs its own objective/field evidence. At p=1 the large domain still gives substantial weight to weak but finite farfield curvature after the artificial floor is removed.

Shock measurements use the surface Mach-gradient direction of the P1 flow on original wing triangles, at span strips y=.55 and .85 (+/- .025), upper surface z>.002, 10%-chord bands around x/c=.55 and .45. They are samples, not a complete shock audit. Pressure-gradient measurements on the recorded field agree closely. Original mesh chord is inferred locally using x-y/sqrt(3).

## Corners

Actual physical tip trailing corner is near (1.144078, 1.196620, 0); the rounded tip's maximum-y point is NOT the trailing corner. Root trailing corner is near (.8059,0,0).

Within radius .05, root and actual tip trailing corners account for 0.0497% and 1.8685% of the recorded complexity. Median finest-direction normal alignment on upper triangles is .9955 and .9655 respectively. The visually small h_min is predominantly normal refinement, though tangent sizes are also small at the tip. These measurements do not establish that local curvature is all physical or all numerical.

Manufactured fits on selected actual tiny stencils show roundoff sensitivity; they omit the production floating-resolution cutoff and must not be reported as a production affine-recovery failure. Existing production roundoff treatment and two-ring comparison must be preserved in any change.

## Integration recommendation

1. Correct the global dependence of numerical Hessian regularization in the shared sensor-metric path; retain physical size/aspect limits, flat-sensor semantics and strict geometry/height/acceptance gates. Do not merely lower an arbitrary global constant or filter all corners.
2. Qualify with the saved CFD seed, exported raw per-sensor Hessians, ungraded sensor/intersection field, and final native-graded target. Compare plateau budget, both sensor contributions, shock directional sizes, corner normal/tangential sizes and total complexity. Include finite degenerate fields, unit/rotation checks and one MPI comparison using existing facilities.
3. Keep p=1 initially to isolate the correction. Treat any p change or spatial weighting as a separate measured objective decision. A larger complexity target is not needed to demonstrate this defect.
4. Only after a native-graded corrected metric is qualified should it drive another complete adaptation attempt. It changes the target, so compare matching saved-seed before/after results and do not claim repair/geometry acceptance from these diagnostic results.

## Reproduce

Run with OPENBLAS_NUM_THREADS=1 OMP_NUM_THREADS=1 MKL_NUM_THREADS=1, nice -n 19, and python3 -B:

1. metric_floor_diagnostic.py --recover (100-second recovery bound; independent full-grid recovery and primitive tests).
2. combined_metric_tests.py (two-sensor tests and normalization/sensitivity comparisons).

results.json records source/input SHA256 values and limitations; combined_results.json records comparisons; independent_recovery.npz preserves Hessians/residuals/ring choices. The recovery uses NumPy SVD least squares instead of Eigen pivoted QR and omits the production floating-resolution cutoff. It does not replay native gradation, MPI, geometry certification or adaptation. Cubic/quartic corrections were zero in the recorded production case. No globally changed floor policy has been validated in SU2 itself.
