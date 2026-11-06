# SA/SST implementation verification

The user's scope is infrastructure and small local tests only. **CFD solves: 0.
Production runs: 0. Cluster access: 0.** The handoff's requested simulation
campaign was therefore not executed. The separate adjoint estimator is excluded.

## Source and build state

The checkout starts at upstream develop `f4b67d23deaac22948cc8d2a2c0da3138ae804f6`.
EPM fixes and their verified brief are reused from `feature_UQ` commits
`671a0f4` and `36ffbeeb`. The field/provider commit is `c1464ac`; solver coupling
is `bece941`. Branch: `rois1995/SU2:feature_UQ_SA`.

Shell network access was unavailable, so source and commits were transferred
through the GitHub connection and their Git hashes checked. The initially
unavailable tracked TecIO archive was restored from a local checkout and its
blob hash verified. This is a shallow source checkout, not a full-history clone.
Pinned Eigen, MEL and Meson source archives were recovered read-only from local
Git objects. Other optional/build dependencies were not initialized. No files in
the source checkouts on the external drive were changed.

The focused tests use GCC 13.3.0, C++17, O0 for the operator executable and O2 for
the field probe. Python is 3.12 with NumPy 2.5.3 and SciPy 1.18.1. The driver,
SST solver, output files and actual SIMD numerics factory also pass syntax
compilation with the pinned Eigen/MEL headers. The complete serial SU2_CFD
executable now builds and links with MPI, OpenMP, AD, CGNS and TecIO disabled.
Three incorrect upstream relative include paths in the mixing-plane and flow
traction interfaces were corrected to complete that build. These fixes do not
change the interface equations. The build commands and logs are linked below.

## Evidence

| Check | Status | Result |
|---|---|---|
| Reused EPM orientation/signed-source tests plus new operator tests | PASS | 418 assertions in 13 C++ cases |
| SA and SA-neg unchanged source; zero-field SST recovery | PASS | Actual source operators |
| Momentum, compressible work, incompressible thermal sign/conservation | PASS | Actual scalar viscous operators; density/velocity finite differences |
| SST signed work, both caps, exact wall skip, tiny positive k | PASS | Actual SST source operator |
| Matched EPM settings/blends and QCR donor difference | PASS | Local tensor/source checks |
| Parser rejection and sidecar identity | PASS | 2 Python tests with multiple subprocess cases |
| Field/provider/generator | PASS | 7 Python tests, including all 4,000 circular queries |
| Preparation/extraction/comparison tools | PASS | 3 tests using synthetic CSV evidence |
| Nodal donor fit and incompatible-wall rejection | PASS | 1 synthetic test |
| Existing MeshAdaptation barycentric reference | PASS | 200 additional queries |
| Tiny complete input preparation | PASS | 13 parser-checked configurations, all PREPARED_NOT_RUN |
| Full serial SU2 executable link | PASS | Complete solver; optional formats, AD and parallel support disabled |
| Scalar/SIMD viscous operator agreement | PASS | 172 assertions, four SIMD lanes; 2D prescribed stress and 2D/3D matched EPM at scales 0, 0.4 and 1 |
| End-to-end baseline regression | NOT_RUN | No flow solve |
| Scalar/SIMD assembled residual comparison | NOT_RUN | SIMD factory compiles; no runtime parity claim |
| MPI/OpenMP flow execution, partition changes | NOT_RUN | No parallel flow test performed |
| True disk-checkpoint continuation | NOT_RUN | Sidecar tests are not a flow restart |
| Exact donor face/source replay and recovery | UNSUPPORTED | Nodal export/fit exists; exact replay harness still needed |
| Nonlinear response, mesh refinement, paired SST scenarios | NOT_RUN | Execution excluded from current scope |
| Physical validation or uncertainty coverage | NOT_RUN | No independent flow-reference comparison |

Raw evidence: [operator log](stress_checks/operators.log),
[Python checks](stress_checks/python.log),
[SIMD operator log](stress_checks/simd.log),
[complete build log](stress_checks/build.log),
[build/check manifest](stress_checks/verification.json), and
[prepared run matrix](stress_checks/run_matrix.csv).
Field-specific results are in [FIELD_GENERATION_REPORT.md](FIELD_GENERATION_REPORT.md).

Reproduce the focused checks with:

```bash
python3 SU2_PY/stress_field/run_local_checks.py --build-dir /tmp/stress-checks --v07-root /path/to/SA_stress_uncertainty_v07
```

On this machine the available scientific packages were selected with
`PYTHONPATH=/home/rausa/Software/Python-3.12.12-INSTALL/lib/python3.12/site-packages`.
No package was installed or downloaded for these tests.

## Stable limitations

- **L1 — Physical meaning.** A tested scenario range is not a confidence interval,
  error guarantee or demonstrated model improvement.
- **L2 — Geometry.** Coupled scope is planar/wall-free with zero correction on
  physical boundaries. General curved geometry, periodicity and moving meshes
  are rejected. The supplied circle remains OFFLINE_ONLY.
- **L3 — Parallel and persistence evidence.** Rank identity and restart sidecar
  code exists; actual parallel results and checkpoint continuation remain untested.
- **L4 — Derivatives.** The implicit matrix retains the parent approximation and
  some native stress-dependent energy terms. The complete new tangent is not
  supplied. Adjoint/direct-differentiation modes are rejected.
- **L5 — Size and cost.** The small field is replicated on each rank with explicit
  size guards. There is no distributed field storage or runtime query-cost report.
- **L6 — Donor recovery.** Target export fits the declared map at identical nodes
  and labels the result COUPLING_CONTROL. Exact face/source replay tables, a
  residual-recovery harness and represented-field convergence studies remain open.
- **L7 — Qualification.** The complete serial build and scalar/SIMD operator
  comparison pass. End-to-end baseline/zero regression, assembled SIMD residual
  parity and a physical campaign remain untested. The tiny
  examples are wiring inputs, not converged benchmark cases.

## Decision

The core research interface is implemented and locally checked within the stated
scope. The full v1.2 campaign infrastructure is not yet qualified; exact discrete
donor replay is an additional implementation task, not merely an unrun test.
The next qualification steps are baseline/zero flow comparison, actual checkpoint
continuation and the donor residual-recovery harness. Do not launch production work or interpret uncertainty coverage
from these results. The [assessment](SA_SST_APPROACH_ASSESSMENT.md) gives the
scientific reasoning in plain language; the [runbook](STRESS_DISCREPANCY.md)
explains the available commands and restrictions.
