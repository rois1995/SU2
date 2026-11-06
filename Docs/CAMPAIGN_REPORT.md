# SA/SST implementation verification

The supported serial infrastructure is implemented and passes the bounded local
suite. **Production runs: 0. Cluster access: 0.** The full physical campaign in
the handoff was not executed. The separate adjoint estimator remains excluded.

## Source and build

Base: upstream develop `f4b67d23deaac22948cc8d2a2c0da3138ae804f6`.
EPM fixes were reused from `feature_UQ` commits `671a0f4` and `36ffbeeb`.
Field/provider: `c1464ac`; original solver coupling: `bece941`.
Branch: `rois1995/SU2:feature_UQ_SA`.

This is a shallow complete source checkout. Shell network access was unavailable;
GitHub transfers were checked against Git hashes. Pinned Eigen, MEL and Meson
sources were recovered read-only from local Git objects. No external checkout
was modified and no package was installed. The complete serial SU2_CFD builds
with MPI, OpenMP, AD, CGNS, TecIO and the Python wrapper disabled.

Tests use GCC 13.3, C++17, Python 3.12.3, NumPy 2.5.3 and SciPy 1.18.1.
The complete build is debug/O0 with native SIMD; finite-value checks remain enabled.
The runbook records reproducible commands and the manifest records source hashes.

## Final evidence

| Check | Result |
|---|---|
| EPM, coupling and replay C++ checks | 436 assertions in 15 cases pass |
| Actual scalar/SIMD viscous operators | 244 assertions in two cases pass, four SIMD lanes; O0 and optimized build |
| Python parser, provider, campaign, target and study tools | 19 tests pass, including 4,000 circular provider queries |
| Existing MeshAdaptation barycentric reference | Prior unchanged reference check: 200 queries pass |
| Complete serial solver | Builds and links |
| Bounded solver suite | 39 process checks and 22 comparisons pass; four processes intentionally reject changed inputs |
| Baseline versus zero field, SA and SST | Exact agreement in saved solution/primitive columns |
| Actual ASCII disk restarts, SA and SST, energy enabled/disabled | Maximum absolute difference 1.10e-11; threshold 1e-9 |
| Same-state QCR/EPM donor replay, 2D and 3D | Flow and turbulence residual differences exactly zero |
| Assembled prescribed and nonuniform 1C EPM scalar/SIMD residuals | Maximum differences 1.14e-13 and 1.09e-13; threshold 1e-10 |
| Nearly uniform 1C EPM scalar/SIMD residuals | Maximum difference 1.58e-13; threshold 1e-10 |
| Uniform 3C EPM scalar/SIMD residuals | Maximum difference 1.58e-13; threshold 1e-10 |
| Local MPI launcher | BLOCKED_ENVIRONMENT: socket/PMIx initialization fails, even for localhost `/usr/bin/true` |
| Physical validation, converged donor recovery, mesh studies | NOT_RUN |

The earlier implementation pass made 180 bounded process checks across debugging
and final-suite attempts; those remain in the linked record. The preparation pass
adds the expanded 39-check suite, instrumented 1C diagnosis and four donor-boundary
checks. None is a production run.

The final suite uses 81-node triangles and a 27-node tetrahedral cube. Each solve
has at most four iterations and a 45-second timeout. Diagnostic runs assemble
once and skip time integration. The compressible nonuniform comparison uses an
explicitly manufactured saved velocity state, adjusting total energy to preserve
internal energy. It is a controlled comparison, not a converged flow solution.
Residual comparisons use fixed unit reference scales in internal units; restarting
compares every common saved non-stress column, including temperature.

Raw evidence: [operators](stress_checks/operators.log),
[Python](stress_checks/python.log), [SIMD](stress_checks/simd.log),
[build](stress_checks/build.log), [final solver suite](stress_checks/solver_checks.json),
[earlier attempts](stress_checks/finalization_attempts.json),
[MPI capability check](stress_checks/mpi.log),
[manifest](stress_checks/verification.json).
The older [13-case matrix](stress_checks/run_matrix.csv) is still a separate
PREPARED_NOT_RUN campaign; it is not the executed final suite.

## Problems found and resolved

Actual disk continuation exposed an upstream incompressible state inconsistency:
the energy-disabled restart/reset path wrote temperature into a slot that now
stores enthalpy. Both assignments now use the existing freestream enthalpy.
Full saved-output comparisons pass with energy enabled and disabled.

Matched EPM nodal output attempted a nonzero-work calculation at exact wall
nodes with zero turbulence energy. It now respects the same wall skip as the
SST source. The correction has not been hidden by suppressing output tests.

The existing USE_VECTORIZATION setting did not select scalar Roe/MSW assembly.
Diagnostics now explicitly honor it and report the actual path. Earlier apparent
parity results that exercised SIMD twice are recorded as invalid parity evidence.
Normal solver selection is unchanged outside diagnostics.

## Arithmetic failure resolved; donor gate retained

The original nearly uniform 1C error was 2.1703143698436483e-10, above 1e-10.
Actual query traces located the cause in shared SIMD gradient correction:
`dot - Vj + Vi` loses a small projection against the common velocity offset.
The scalar path uses `dot - (Vj - Vi)`. Matching that order fixes the error.
All 307 diagnostic query inputs/stresses then agree exactly; original-case
assembled error falls to 1.6259e-13. The independently regenerated final case
passes at 1.5716e-13. The tolerance and EPM physics are unchanged.

The new directional-derivative regression fails before the fix and passes after
it, including an optimized fast-math check. Temporary trace instrumentation was
removed. [Diagnosis](stress_checks/epm_investigation.json),
[old-formula failure](stress_checks/epm_regression_before.log),
[optimized test](stress_checks/simd_optimized.log).
Exactly repeated eigenvalues still do not define a unique anisotropic direction;
that mathematical limitation was not the justification for accepting this failure.

The tiny QCR/EPM donors were also tested against the physical-field representation.
Both have nonzero boundary corrections, so the mandated zero-boundary map rejects
them with relative maximum nodal error 1.0. Removing the map for an offline-only
fit gives near-machine-precision nodal agreement, but solver startup correctly
rejects those unrestricted fields before iteration. This identifies a boundary
contract obstruction, not failed convergence. Nonlinear recovery remains unrun.
See the [recovery preparation](stress_checks/recovery_preparation.md).

Twenty local MPI jobs are parser/field-checked and prepared, but a fresh localhost
capability probe still fails to initialize sockets. The [physical study design](validation_study/README.md)
contains three cases, eight nominal references and 58 unexecuted run definitions.
It is design-ready, not solver-ready: experimental inputs, uncertainty and meshes
remain explicit prerequisites. No production or cluster work was performed.

The deliberately strong SST toy field also produces negative total-covariance
eigenvalues over 65.625% of owned volume at tolerance 1e-12. Lower and upper
production limits affect 75% and 1.5625%. The absolute integrated production/work
gap is about 8.83e-4 in internal units. The new diagnostics expose these conditions;
they do not silently clip them. This fixture is not a physically accepted scenario.

## Limits and decision

The interface supports fixed planar/wall-free geometry with prescribed correction
zero on physical boundaries. Coupled curved geometry remains unsupported; the
circle is an offline provider test. Exact replay is bounded, serial and diagnostic;
it is not a physical field or a nonlinear recovery method. The physical field is
replicated per rank; timing and storage counters are available, but parallel
performance and repartitioned continuation are not qualified.

The implicit derivative is still incomplete. Adjoint/direct differentiation is
rejected. More starting states, converged recovery, mesh refinement and independent
reference data are needed before predictive claims. The infrastructure can now
support those studies when separately authorized; successful tiny tests do not
turn the scenario range into a confidence interval.
