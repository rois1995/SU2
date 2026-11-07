# SU2 SST–EPM implementation and validation requirements

**Implementation:** Sections 1–11. **What to simulate, how to reuse a primal restart, executable commands, and how to obtain/check sensitivities:** [Section 12](#12-operational-runbook-simulations-and-sensitivity-results). **Acceptance checklist:** Section 13.

## 1. Task contract

Implement an opt-in research capability that estimates the responses of five eigenspace perturbation method (EPM) cases using:

1. One converged ordinary SST baseline flow solution.
2. One converged, fully coupled discrete adjoint for each scalar quantity of interest (QoI).
3. Five parameter recordings/reverse evaluations using that same baseline and adjoint.

The estimator must return the baseline QoI, five first derivatives, five first-order endpoint predictions, and a linearized response envelope. Add a nonlinear continuation mode so the derivatives and endpoint predictions can be checked against solutions of exactly the same parameterized equations.

This is not an implementation of a probabilistic uncertainty interval or a guarantee of bounds. It is not one ordinary flow run that produces a validated EPM envelope without additional work. Independent lift and drag estimates normally require separate adjoints; an adjoint for a weighted objective gives that combination's sensitivity.

**Implementation status (2026-11-07):** branch `rois1995/SU2:feature_UQ` starts at the upstream commit below. Reference corrections and experimental 2D/3D extraction are implemented. The complete serial reverse-AD executable builds and passes tiny 2D and 3D end-to-end regressions: all primal/adjoint residuals below `1e-10`, unchanged adjoint iterations with extraction enabled, full interior-edge coverage, finite nonzero five-case sensitivities, saved-adjoint restart agreement, and an unchanged primal restart. Local numerics tests pass 721 assertions in seven passive cases and 695 in four reverse-AD cases, including the fused edge kernel. MPI reductions, synchronized probes and master-only CSV output are implemented; modified driver/flux sources pass MPI/MeDiPack syntax compilation. Two-rank runtime validation is blocked by the sandbox's socket permissions (`errno=1`) and remains pending. No production runs or cluster access occurred. Nonlinear derivative and endpoint validation remain separate pending gates.

**Study constraint: preserve classical EPM.** Keep the corrected classical 1C/2C/3C targets, orientation permutations, and endpoint controls. Do not introduce spectral smoothing, average admissible orientations into a weaker stress tensor, or attenuate anisotropy to obtain a derivative. The earlier smooth-target proposal is withdrawn: at isotropy it collapses all five targets to 3C and changes the uncertainty experiment. The smoothing prototype has been removed; its historical commit remains available for audit. The reference-model bug fixes remain in place.

**Adjoint status:** a common baseline adjoint remains appropriate wherever the classical target correction has a suitable continuous or justified directional limit. Degenerate targets require explicit orientation diagnostics and qualification; no globally valid live-EPM derivative is established by selecting an arbitrary eigensolver basis. Section 5.5 specifies the model-preserving investigation. The first extraction implementation now exists behind `SST_EPM_POSTPROCESS=YES`: runtime scalar registration in the native adjoint driver, passive classical target corrections in scalar and fused-edge viscous momentum/energy fluxes and limited SST sources, reuse of the coupled adjoint, sampled eigenspace probes, and CSV output. A preparation-only paper-case generator now supplies ordinary baselines, coupled drag/lift adjoints and the five classical endpoints for NACA 0012, the NASA hump and ONERA M6. Nonlinear continuation, wide-SIMD/OpenMP extraction, nonlinear CFD Taylor tests, and an execution/comparison campaign runner remain pending. MPI runtime validation is pending despite implemented distributed extraction. The broader `SST_EPM_ANALYSIS/CASE/AMPLITUDE/OUTPUT` interface below remains proposed and is not accepted by this branch.

**Execution contract:** only small local tests are authorized for this task. Production campaigns and all cluster access are prohibited. The numerical campaigns below are future validation instructions and must not be launched under the current authorization. Pushing this branch to the user's fork is explicitly authorized.

### Implemented extraction interface

For a compatible saved ordinary-SST primal and its coupled adjoint configuration, use this overlay (not a complete case):

```ini
SOLVER= RANS
KIND_TURB_MODEL= SST
SST_OPTIONS= V2003m
MATH_PROBLEM= DISCRETE_ADJOINT
OBJECTIVE_FUNCTION= DRAG
OBJECTIVE_WEIGHT= 1
FROZEN_VISC_DISC= NO
FROZEN_LIMITER_DISC= NO
INCONSISTENT_DISC= NO
USE_VECTORIZATION= NO
MGLEVEL= 0
CFL_ADAPT= NO
TIME_DISCRE_FLOW= EULER_IMPLICIT
TIME_DISCRE_TURB= EULER_IMPLICIT
SLOPE_LIMITER_FLOW= NONE
MUSCL_TURB= NO
RAMP_MUSCL= NO
UQ_DELTA_B= 1
UQ_URLX= 1
UQ_COMPONENT= 1
UQ_PERMUTE= NO
SST_EPM_POSTPROCESS= YES
```

Use a reverse-AD build without OpenMP. The driver accepts 2D and 3D; one-process execution is locally verified and MPI execution awaits its two-rank check. The fused edge kernel is included even though `USE_VECTORIZATION=NO` remains required. Smooth stationary ideal-gas flow without extra coupled physics, moving/deforming grids, wall functions, rough walls, periodic interfaces, Newton–Krylov, fixed-lift control, or ramping is required. A single unit-weight DRAG or LIFT objective is accepted. Unsupported combinations fail explicitly. The normal baseline and classical UQ runs leave the new option disabled.

Extraction runs in the adjoint driver's postprocess after its native convergence criterion passes, before the usual geometry sensitivity recording. Each replay clears old indices, restores all direct state blocks, registers the single persistent amplitude before dependencies, records one direct iteration, seeds the saved coupled state adjoints and native objective, reverses, extracts only the amplitude derivative, and clears derivatives. It does not call the state-adjoint extraction/update. Zero-amplitude flow/turbulence iteration outputs and the saved adjoint arrays are checked against their original values; an altered baseline objective also stops extraction. Direct states and dependencies are restored before subsequent processing.

`sst_epm_sensitivities.csv` is written by rank 0 only after all replays succeed. Rows include `dimension` and `mpi_ranks`. Every rank registers a local amplitude; its reverse derivative is summed with a passive MPI reduction. Global ambiguity counts keep the recording/reverse sequence identical on all ranks. It contains the baseline, five canonical case gradients/predictions, and diagnostic rows when small eigenvalue gaps affect the target. `probe_pair=3, probe_angle=0` identifies the canonical result. Other rows swap a pair of nearly tied eigenvectors using a 90-degree rotation: pair 0=(0,1), 1=(0,2), 2=(1,2), with zero-based ascending baseline eigenvalue indices. These swaps preserve the full target eigenvalues and avoid introducing finite out-of-plane shear merely by continuously tilting a planar eigensystem. There are five canonical recordings and up to three extra recordings per ambiguous anisotropic case (at most 17 total).

The fixed dimensionless gap threshold is `1e-10` in baseline anisotropy eigenvalues. `ambiguous_evaluations` counts numerical evaluations, not unique cells; fused interior edges, scalar boundary fluxes and SST sources contribute. Separate `stress_evaluations` and `source_evaluations` counters expose omitted execution paths. In MPI these are sums of rank-local evaluations, including any duplicated partition work; they are not decomposition-independent cell counts. Near ties these probes are conditioning diagnostics, not exact additional members of the classical five-case set. They sample a few basis permutations, not every orientation or spatially varying orientation field. Agreement among samples does not prove a unique derivative, rotation-invariant uncertainty range, or global extrema. No stress averaging or target attenuation is used.

Rows are labeled `frozen_forcing_unverified`, with `primal_converged=unverified` and `adjoint_converged=true` meaning only that SU2's configured adjoint criterion passed. The console range includes the baseline and all sampled predictions. It is not a validated nonlinear envelope. This first version writes no full provenance manifest: preserve the mesh, input configuration, source/build identity, restart and convergence histories alongside the CSV. Use separate directories for separate objectives. A rerun replaces this CSV.

The extraction scalar is internal; setting it to nonzero in a local test verifies contribution identities, not a user-accessible nonlinear continuation solver. State derivatives of the passive target correction are intentionally absent at zero amplitude; ordinary baseline flow/turbulence derivatives remain active. Reference and extraction use the same corrected tensor and production floor/cap, including the existing zero-mu_t omega-production fallback. The unresolved physical-limit qualification for that fallback still applies.

### Tiny local infrastructure verification

Use [the smoke-test runner and commands](../TestCases/disc_adj_rans/sst_epm_adjoint/README.md). It creates its own 117-point 2D rectangle or 315-point 3D box with spanwise flow, uses a coupled drag adjoint, and limits each solver invocation to 2,000 iterations and 300 seconds. The test compares feature-disabled/enabled adjoint histories, checks every equation's convergence, verifies stress evaluations cover all interior mesh edges, extracts all five sensitivities, reloads the saved adjoint, and checks the primal restart checksum. Configs, logs, histories, restarts and CSVs remain in the selected local test directory.

[Recorded smoke-test evidence](../TestCases/disc_adj_rans/sst_epm_adjoint/smoke_results.json) contains executable/source checksums, residuals, timings, sensitivities and restart differences. These coarse meshes are not physically resolved validation cases.

| Case | Points | Primal history rows | Adjoint history rows | Stress/source evaluations per canonical replay |
|---|---:|---:|---:|---:|
| 2D | 117 | 1532 | 174 | 243 / 104 |
| 3D | 315 | 545 | 95 | 1043 / 270 |

The end-to-end audit identified a necessary integration correction: `USE_VECTORIZATION=NO` does **not** bypass the combined Roe convective/viscous edge kernel. Its reverse-AD scalar-lane path now registers the amplitude inside preaccumulation and applies the same shared stress correction as scalar boundary fluxes. A scalar-class unit test or a nonzero source sensitivity alone would not detect omission of interior momentum/energy forcing. The separate evaluation counters and mesh-edge coverage check now guard against that omission.

Full builds also require the corrected relative includes in `CMixingPlaneInterface.hpp/.cpp` and `CFlowTractionInterface.cpp`; their old paths could resolve SU2 headers from an unrelated neighbouring checkout through a dependency include directory. The lightweight local build used available CoDiPack, Eigen and MEL headers, without changing that neighbouring source checkout. Exact dependency revisions were not independently verified against pinned gitlinks.

For MPI, the runner uses `mpirun --host localhost -np 2`, reuses the matching serial primal, and compares sensitivities and probes with the serial result. The current sandbox rejects the launcher's socket creation before application execution. MPI driver and flux syntax checks pass with real MPI/MeDiPack headers, but no full MPI executable/run or rank-invariance result is claimed. Production campaigns and all cluster access remain prohibited.

### Inspected source

The source assessment used public `su2code/SU2`, branch `develop`, commit:

`f4b67d23deaac22948cc8d2a2c0da3138ae804f6`, committed 2026-10-31; the configuration template identifies SU2 8.6.0 “Harrier”.

[Inspected commit](https://github.com/su2code/SU2/commit/f4b67d23deaac22948cc8d2a2c0da3138ae804f6)

At the start of implementation:

- Read repository instructions and inspect the actual checkout, branch, build options, and available tests. Record the actual commit.
- Confirm the named symbols below still exist and recheck the two EPM correctness findings. Do not reapply a fix already present in the target checkout.
- Preserve unrelated changes. Work on an isolated branch/worktree when appropriate.
- Implement locally and keep `feature_UQ` on the user's fork updated, as explicitly requested. Do not merge upstream.
- If a build or required numerical check cannot run, report the precise blocker and distinguish implemented, statically reviewed, and numerically verified work.

## 2. First implementation scope

Use a deliberately explicit first supported configuration:

- Steady, single-zone, finite-volume compressible RANS with an ideal-gas model.
- SST-2003m, stationary mesh, smooth wall-resolved stationary walls.
- Start integration verification with a two-dimensional attached airfoil and a scalar drag objective; add lift after the first gradient passes.
- Use a simple supported implicit iteration, no multigrid, and a fixed CFL for the first acceptance case.
- Retain the full flow–turbulence coupling in the adjoint.

For the new feature, reject unsupported combinations with actionable messages. Initial exclusions include unsteady flow, moving grids/walls, multizone coupling, transition, compressibility corrections, sustaining terms, alternative production modifiers, axisymmetry, wall functions/rough walls, fixed-lift control, and additional coupled physics. Verify three-dimensional support before advertising it. Ordinary SU2 configurations with the new feature disabled must retain their existing availability.

These exclusions limit the initial implementation and validation workload; they are not claims that the method cannot be extended.

Require the existing options:

```ini
FROZEN_VISC_DISC= NO
FROZEN_LIMITER_DISC= NO
INCONSISTENT_DISC= NO
```

Do not freeze `k`, `omega`, eddy viscosity, SST blending functions, or the baseline production response to make the adjoint cheaper. The full coupled baseline adjoint supplies their first-order response.

## 3. Mathematical definition: fix the path before coding

Let `q` contain all flow and SST state variables participating in the recorded steady iteration. Let `R_0(q)` be the exact ordinary SST-2003m discrete equation residual, including boundaries. Let `R_m^EPM(q)` be the residual for corrected nonlinear EPM case `m`, with its endpoint controls fixed.

Use this equation-level continuation:

\[
R_m(q,a)=R_0(q)+a\left[R_m^{\mathrm{EPM}}(q)-R_0(q)\right],
\qquad 0\le a\le 1.
\]

Thus, as local functions of the state:

\[
R_m(q,0)=R_0(q),\qquad R_m(q,1)=R_m^{\mathrm{EPM}}(q).
\]

The common baseline is `R_0(q_0)=0`. The amplitude `a` is a new active scalar. It is not `UQ_DELTA_B`, `UQ_URLX`, or a differentiable version of a permutation flag.

### Why this path is necessary

The inspected code has two obstacles to using an existing UQ control as a common origin:

- Setting the barycentric displacement `UQ_DELTA_B` to zero does not disable a finite eigenvector permutation.
- Ordinary SST-2003m and the current full-Reynolds-stress EPM branch use different constitutive conventions. In particular, the ordinary modified-model momentum stress omits the isotropic turbulent contribution that the full EPM tensor includes. Merely setting the EPM tensor blending factor to zero does not recover the ordinary SSTm residual.

The new continuation must recover the ordinary baseline exactly, even when those endpoint conventions differ. Consequently, its sensitivity includes the convention change as well as the eigenspace perturbation. Label it “ordinary SST to corrected EPM response”; do not silently describe it as pure anisotropy sensitivity.

Compute a diagnostic at the frozen baseline using full-stress EPM with zero barycentric displacement and no permutation. Report its residual difference from ordinary SST, and preferably its adjoint projection. This exposes the convention offset without requiring another converged flow solution. Do not subtract that offset from the estimator without explicitly defining and validating a different target model.

### Blend final discrete contributions

Implement the homotopy in the affected local contributions, rather than evaluating the whole PDE twice:

\[
F(a)=F_0+a(F_m-F_0),\qquad
S(a)=S_0+a(S_m-S_0).
\]

`F` includes the complete affected viscous flux and `S` includes the complete affected SST source contribution. Form each endpoint's production and apply its intended limiters before blending the endpoint source contributions.

This distinction is part of the model definition:

\[
(1-a)\operatorname{clip}(P_0)+a\operatorname{clip}(P_m)
\ne
\operatorname{clip}\left((1-a)P_0+aP_m\right)
\]

in general. Do not accidentally implement the second expression while claiming the first. Put source clipping and limiting inside each endpoint contribution before blending. Post-blend operations must be linear or demonstrably equivalent/inactive for the supported states; otherwise they define a different homotopy that must be specified and validated explicitly. Merely documenting an unchanged nonlinear clip is insufficient. Endpoint-identity checks alone do not establish the interior path.

Intermediate amplitudes define an equation-level continuation. They need not correspond to a conventional single stress-based closure. At `a=1`, the residual and reported objective must match the corrected standalone EPM case.

### Objective consistency

For most initial aerodynamic objectives at a smooth stationary wall, much of the dependence is through the solution. Nevertheless, audit the actual objective routine.

If the endpoint changes the objective's explicit constitutive evaluation, define a consistent objective path, for example:

\[
J_m(q,a)=J_0(q)+a[J_m^{\mathrm{EPM}}(q)-J_0(q)].
\]

Require the zero-amplitude identity as a local function, so that the baseline `J_q` is common to all five modes. Preserve any direct derivative `J_a`. A tape containing a baseline-only objective cannot recover an explicit dependence that was never implemented.

### What the estimator returns

For each case:

\[
g_m=\left.\frac{dJ_m(q_m(a),a)}{da}\right|_{a=0},
\qquad
\widehat J_m(a)=J_0+a g_m.
\]

The first-order endpoint prediction is `Jhat_m=J_0+g_m`. Include `J_0` in the minimum and maximum when reporting the predicted spread.

The five cases are the two orientations for 1C, the two orientations for 2C, and one 3C-directed case. Keep internal labels `1C_KEEP`, `1C_SWAP`, `2C_KEEP`, `2C_SWAP`, `3C` until production tests establish their max/min interpretation. Local production extrema do not guarantee extrema of global lift, drag, or other QoIs.

| New case | Endpoint `UQ_COMPONENT` | Endpoint `UQ_PERMUTE` |
|---|---:|---|
| `1C_KEEP` | 1 | NO |
| `1C_SWAP` | 1 | YES |
| `2C_KEEP` | 2 | NO |
| `2C_SWAP` | 2 | YES |
| `3C` | 3 | NO |

With full displacement and full tensor replacement, the 3C tensor is isotropic and its orientation is immaterial. At partial controls the effective tensor is generally not isotropic; with partial barycentric displacement a permuted 3C-directed case can also differ. The defined five-case set at partial controls is therefore a chosen set of targets, not a claim to include every distinct orientation.

## 4. Existing code to reuse and where to change it

All links below refer to the inspected commit. Treat symbols and responsibilities as the primary anchors if line numbers move.

| Area | Source and current behavior | Required work |
|---|---|---|
| Configuration | [CConfig.hpp](https://github.com/su2code/SU2/blob/f4b67d23deaac22948cc8d2a2c0da3138ae804f6/Common/include/CConfig.hpp), [CConfig.cpp](https://github.com/su2code/SU2/blob/f4b67d23deaac22948cc8d2a2c0da3138ae804f6/Common/src/CConfig.cpp), [option_structure.hpp](https://github.com/su2code/SU2/blob/f4b67d23deaac22948cc8d2a2c0da3138ae804f6/Common/include/option_structure.hpp), [config_template.cfg](https://github.com/su2code/SU2/blob/f4b67d23deaac22948cc8d2a2c0da3138ae804f6/config_template.cfg) | Add explicit analysis mode, passive case selector, and active continuation amplitude; validate combinations; document proposed options. |
| Tensor perturbation | [CNumerics.hpp](https://github.com/su2code/SU2/blob/f4b67d23deaac22948cc8d2a2c0da3138ae804f6/SU2_CFD/include/numerics/CNumerics.hpp), `ComputePerturbedRSM` | Correct/verify eigenvector permutation, preserve a single target-tensor definition, and provide reliable passive evaluation for parameter extraction. |
| Eigensystem convention | [blas_structure.hpp](https://github.com/su2code/SU2/blob/f4b67d23deaac22948cc8d2a2c0da3138ae804f6/Common/include/linear_algebra/blas_structure.hpp) | Confirm eigenvector layout and eigenvalue ordering before changing permutation logic. |
| SST production | [turb_sources.hpp](https://github.com/su2code/SU2/blob/f4b67d23deaac22948cc8d2a2c0da3138ae804f6/SU2_CFD/include/numerics/turbulent/turb_sources.hpp), `PerturbedStrainMag` and SST `ComputeResidual` | Replace the EPM production construction with the signed stress–velocity-gradient contraction, preserve the selected SST limits, then blend endpoint source contributions. |
| Scalar viscous flux | [flow_diffusion.cpp](https://github.com/su2code/SU2/blob/f4b67d23deaac22948cc8d2a2c0da3138ae804f6/SU2_CFD/src/numerics/flow/flow_diffusion.cpp) | Add the baseline-to-endpoint flux continuation, including energy work and AD parameter inputs; cover every enabled scalar path. |
| SIMD viscous flux | [diffusion/common.hpp](https://github.com/su2code/SU2/blob/f4b67d23deaac22948cc8d2a2c0da3138ae804f6/SU2_CFD/include/numerics_simd/flow/diffusion/common.hpp), [viscous_fluxes.hpp](https://github.com/su2code/SU2/blob/f4b67d23deaac22948cc8d2a2c0da3138ae804f6/SU2_CFD/include/numerics_simd/flow/diffusion/viscous_fluxes.hpp) | Match scalar flux behavior; avoid constructor-cached inactive parameter copies; audit the enclosing preaccumulation boundary. |
| Tape input/coupling | [CDiscAdjFluidIteration.cpp](https://github.com/su2code/SU2/blob/f4b67d23deaac22948cc8d2a2c0da3138ae804f6/SU2_CFD/src/iteration/CDiscAdjFluidIteration.cpp) | Register the new scalar before dependencies and the direct iteration; retain flow and turbulence state coupling. |
| Parameter ownership/extraction | [CDiscAdjSolver.cpp](https://github.com/su2code/SU2/blob/f4b67d23deaac22948cc8d2a2c0da3138ae804f6/SU2_CFD/src/solvers/CDiscAdjSolver.cpp) and corresponding header | Add persistent active parameter ownership and sensitivity storage, using existing scalar-registration/MPI-reduction patterns. |
| Adjoint driver | [CDiscAdjSinglezoneDriver.cpp](https://github.com/su2code/SU2/blob/f4b67d23deaac22948cc8d2a2c0da3138ae804f6/SU2_CFD/src/drivers/CDiscAdjSinglezoneDriver.cpp) and corresponding header | Add five parameter replays after one converged adjoint; preserve baseline state and adjoint; seed native outputs/objective exactly once. |
| Forces and objective | [CFVMFlowSolverBase.inl](https://github.com/su2code/SU2/blob/f4b67d23deaac22948cc8d2a2c0da3138ae804f6/SU2_CFD/include/solvers/CFVMFlowSolverBase.inl), relevant flow solver and objective code | Audit explicit amplitude dependence and endpoint identity. Do not indiscriminately add turbulent traction at a smooth wall. |
| Output | [CAdjFlowCompOutput.cpp](https://github.com/su2code/SU2/blob/f4b67d23deaac22948cc8d2a2c0da3138ae804f6/SU2_CFD/src/output/CAdjFlowCompOutput.cpp) and corresponding header | Expose gradients and predictions, plus a machine-readable case table and convergence/provenance metadata. |
| Tests/examples | [UnitTests/SU2_CFD/numerics](https://github.com/su2code/SU2/tree/f4b67d23deaac22948cc8d2a2c0da3138ae804f6/UnitTests/SU2_CFD/numerics), [disc_adj_rans/naca0012](https://github.com/su2code/SU2/tree/f4b67d23deaac22948cc8d2a2c0da3138ae804f6/TestCases/disc_adj_rans/naca0012), [rans_uq/naca0012](https://github.com/su2code/SU2/tree/f4b67d23deaac22948cc8d2a2c0da3138ae804f6/TestCases/rans_uq/naca0012) | Add focused tests and a converged common-baseline comparison case. Existing examples are starting points, not validated configurations for this method. |

Also audit `CNumerics.cpp`: current UQ settings are copied into numerics objects during construction. New AD inputs cannot rely on copies made before tape registration.

The existing discrete-adjoint NACA0012 SST example selects `V1994m`. Explicitly change the new example to `V2003m`. The UQ example's short iteration count and high angle of attack are unsuitable as an unmodified first gradient-verification case.

## 5. Prerequisite: establish a trustworthy nonlinear EPM reference

Do this in a separate logical change from the estimator, so reference-model corrections and approximation error remain distinguishable.

### 5.1 Verify and correct eigenvector permutation

The upstream defect was confirmed: `ComputePerturbedRSM` exchanged rows 0 and 2, while `EigenRecomposition` uses eigenvectors as columns. The implementation now exchanges columns 0 and 2 in the shared helper, reaching scalar flux, SIMD flux, and source callers. The rotated-tensor unit test checks all three component targets and both orientations. Scalar/SIMD assembled-flux agreement remains untested.

The intended operation changes the assignment of ordered eigenvalues to eigenvectors. A row exchange instead changes coordinate components and is not generally the intended orientation perturbation.

Use a non-axis-aligned symmetric tensor with distinct eigenvalues to test:

- Reconstruction without perturbation.
- The intended eigenvalue-to-eigenvector reassignment.
- Symmetry and trace.
- Behavior under a rigid rotation of coordinates.
- Local production ordering for the keep/swap pair.

An axis-aligned diagonal tensor alone will not expose this error.

### 5.2 Use the actual stress contraction for production

Let `R_ij=<u_i' u_j'>` denote Reynolds stress per unit mass, as returned by the common EPM helper after the `UQ_URLX` tensor blend. Use this final effective tensor, not the unblended target. The raw production per unit volume is:

\[
P_m^{raw}=-\rho\sum_{ij}R_{ij}^{m}\frac{\partial U_i}{\partial x_j}.
\]

For the initial SST-2003m target, apply the existing nonnegative floor and intended production cap to this scalar; use the resulting production consistently in the `k` and `omega` equations, retaining the model's coefficients and source signs. Retain the existing relationship `p_omega = (alpha_blended * rho / mu_t) * p_k`, using the corrected, capped `p_k` and a shared, documented policy for vanishing `mu_t` in reference and estimator.

The former SST-2003 code reconstructed a strain-like tensor and used `mu_t times norm squared`. It now uses the signed contraction of the final blended Reynolds stress, followed by the existing nonnegative floor and cap. Both source equations use the resulting capped production. The existing SST-1994 UQ strain-norm path is deliberately retained pending a separate formulation review. Ordinary SST production is unchanged. For corrected EPM with `mu_t <= 0`, omega production is defined as zero to avoid division by zero; this convention is not a proven continuous limit and must be revisited before live adjoint claims. Source tests exercise ordinary SST and corrected EPM, both orientations, active/inactive caps, density, volume, source signs, and zero eddy viscosity.

Do not delete the ordinary SST production path. Do not alter its result at `a=0`. Do not generalize the SST-2003 production rule to SST-1994 without a separate formulation check.

Focused checks:

- An analytical simple-shear state.
- A partially isotropized tensor: the raw deviatoric contribution must scale linearly with its amplitude, rather than quadratically.
- A tensor giving negative raw production: retain the negative raw value until the documented model floor is applied.
- A cap-active and cap-inactive case.
- Consistent source residual signs, density factors, and cell-volume factors.
- Correct handling of the full 3×3 tensor in a two-dimensional calculation; out-of-plane components still matter for eigenvalues and trace.

### 5.3 Keep endpoint controls explicit

The existing `UQ_URLX` is used in an algebraic tensor blend:

\[
R_{\mathrm{eff}}=R_{\mathrm{base}}+
f(R_{\mathrm{target}}-R_{\mathrm{base}}).
\]

It therefore changes the converged modeled tensor. Do not describe varying it as a purely iterative relaxation that leaves the endpoint model unchanged.

Record both `UQ_DELTA_B` and `UQ_URLX` for every result. Values of 1 and 1 request full barycentric displacement and full tensor replacement in this construction. A lower tensor-blending factor defines a weaker endpoint. Do not silently lower it to obtain convergence while keeping a “full endpoint” label.

At partial displacement, an input Boussinesq tensor need not already be realizable. Do not claim that every intermediate convex blend is positive semidefinite without checking the starting tensor and intended assumptions.

### 5.4 Small k and eigenvalue degeneracy

The helper still divides by positive `k`. The exact smooth-wall state `k=0, mu_t=0` now returns the zero Reynolds tensor before eigendecomposition. No arbitrary small-k floor, eigenvalue-gap tolerance, taper, or realizability projection was introduced. Inconsistent states with `k=0, mu_t!=0`, tiny positive `k`, and eigenvalue-gap diagnostics remain unresolved; do not treat this patch as a general regularization. Repeated eigenvalues make individual eigenvectors nonunique.

A deterministic eigenvector tie rule is not automatically invariant under rotation. When a repeated baseline eigenspace is assigned unequal target eigenvalues, the target itself can be ambiguous.

The local counterexample uses trace-free strain `diag(-epsilon, 0, epsilon)` and a rigidly rotated copy, at fixed positive `rho=mu_t=k=1`. Both baseline Reynolds tensors approach `(2/3) I`. Full 1C and 2C target tensors retain different orientations as epsilon tends to zero; their `(0,0)` entries differ by more than 0.1 at every tested scale. The tensor map therefore has no unique isotropic-state limit. This is an obstruction to a general live-target derivative, not proof that every integrated QoI or every particular directional derivative fails.

Establish a suitable continuous target limit at the actual baseline, or justify a directional limit agreeing with the selected target. Without it, a saved-forcing sensitivity must not be labeled a verified live-continuation derivative. A deterministic tie rule alone does not establish continuity. Freestream and zero-strain regions make this a relevant model issue rather than an exotic test input.

Do not introduce a taper, realizability projection, spectral smoothing, or orientation averaging to remove the ambiguity: the user requires the classical EPM model. Preserve existing physical admissibility handling, test near-degenerate states, and report unresolved ambiguity honestly. Any future correction to the small-k or zero-viscosity policy must be justified as a consistent physical limit, shared by reference and extraction, rather than an adjoint-only workaround.

### 5.5 Model-preserving treatment of ambiguous orientations

The target is the original five corrected nonlinear EPM responses. A proposed acceleration must be compared against those equations; mathematical convenience is not a reason to substitute a weaker target model.

1. At the baseline, distinguish degeneracies that leave the reconstructed target unchanged from those that assign unequal target eigenvalues within a repeated baseline eigenspace. For example, full 3C is orientation independent, and some double ties leave a particular 1C/2C target unchanged. A small gap alone is a diagnostic, not permission to modify the tensor.
2. Where the relevant target is unique and continuous, evaluate the full classical endpoint-minus-baseline correction passively and propagate the active amplitude through the native recorded iteration. Preserve all coupled baseline state derivatives. This avoids differentiating individual eigenvectors without modifying EPM.
3. Where the classical target is ambiguous, retain each admissible orientation as a full-strength EPM tensor. Investigate sensitivity to rotations within the degenerate eigenspace using the same converged baseline adjoint, with extra parameter evaluations as necessary. Do not average these tensors: averaging responses or exploring their range is a different operation from averaging stresses before solving.
4. Treat any resulting range as orientation dependence of the linearized estimate, not as five unique live-EPM derivatives or a certified nonlinear uncertainty envelope. A particular saved tensor yields a well-defined frozen-forcing sensitivity, but agreement with a live EPM solution branch still requires directional-limit or nonlinear evidence. Near ties, orientation probes are conditioning diagnostics; they do not silently redefine a nondegenerate classical case.
5. Preserve the original nonlinear equations and the `a=1` endpoint identity in all verification cases. Do not label a frozen-forcing nonlinear solve as classical EPM: the live target must be recomputed from the current state. If five unique derivatives cannot be justified for a particular baseline, report that limitation instead of changing EPM or manufacturing five values.

This is an investigation path, not a completed solution of the nonsmooth problem. It preserves the possibility of one baseline and one adjoint per QoI, but orientation exploration can require more than five inexpensive replays; no fixed replay count or speed-up is established. Local orientation extrema of a raw stress projection are not automatically extrema of the full discrete sensitivity: limited SST production, energy work, boundaries, explicit objective terms, and the native iteration map must all be accounted for. In particular, the stored SU2 iteration-map adjoint cannot be contracted directly with a raw PDE residual difference.

The existing zero-eddy-viscosity omega-production fallback is finite but has not been proved to be a continuous physical limit. Keep this as a separate unresolved gate; withdrawing spectral smoothing does not resolve small-k/source-limit behavior.

## 6. Add the active continuation without losing derivatives

### 6.1 Runtime parameter plumbing

Use a persistent `su2double` amplitude owned by an appropriate solver/configuration context. Register that exact active scalar before dependencies and residual evaluation.

Give this scalar a single FLOW owner. Keep its registration/extraction separate from the existing farfield-parameter branch's incidental restrictions; use the new feature's own supported-configuration checks. `RegisterSolutionExtra` is for iterated extra state, not a substitute for scalar-parameter registration.

Numerics objects must consume the registered scalar at runtime, through a reference/accessor or an on-tape assignment. Merely declaring a field `su2double` is insufficient if all flux objects retained independent copies made before registration.

At every affected preaccumulation boundary:

- Register the amplitude as an input.
- Keep the active amplitude-to-flux/source expression inside the recorded region.
- Preserve its derivative through SIMD packing, scalar conversions, thread-local objects, and reductions.
- Register the actual outputs of the affected operation.

Audit source preaccumulation in `turb_sources.hpp`, scalar flux preaccumulation in `flow_diffusion.cpp`, and the enclosing SIMD operator's input registration. A missing preaccumulation input can produce an apparently plausible zero gradient.

Do not use a runtime shortcut equivalent to:

```cpp
if (amplitude == 0) return baseline_contribution;
```

during parameter recording. The value at zero is the baseline, but the derivative of `baseline + amplitude * correction` must remain present.

### 6.2 Avoid unnecessary differentiation of the EPM eigensystem

For the first derivative at zero, the target correction can be evaluated passively:

\[
b_m=R_m^{\mathrm{EPM}}(q_0)-R_0(q_0).
\]

A zero-amplitude extraction tape may use:

\[
R_0(q)+a\,b_m^{saved}.
\]

At the expansion point it has the required baseline state partial derivative and the required parameter partial derivative. Equivalence to the derivative of the live solution branch additionally assumes a sufficiently regular baseline problem and a suitable continuous target correction, or a justified directional limit agreeing with the saved correction. Separate partial derivatives alone are insufficient if joint first-order behavior fails. Apply the same qualification to an explicit objective correction. Preserve all ordinary baseline AD and all subsequent propagation of amplitude-induced state changes through the iteration.

This removes the need to differentiate eigenvectors in the MVP. It does not freeze the turbulence response: the baseline flow, `k`, and `omega` remain coupled in the adjoint.

Apply the same reasoning to any explicit objective correction. Detach the complete endpoint-minus-baseline correction, not selected arbitrary pieces.

Restrictions:

- This is an extraction implementation at `a=0`, not a nonlinear finite-amplitude model.
- In nonlinear continuation, recompute both endpoint contributions at the current state on every residual evaluation.
- In a multistage/block update, evaluate saved corrections at the corresponding restored stage states. Do not inject a correction into a different update stage. Initial validation avoids multigrid and keeps the iteration simple.
- Remaining primal convergence error can affect equivalence between stage evaluations; converge the baseline sufficiently and quantify the resulting noise.
- Stop-gradient prevents eigenvector derivative problems; it does not resolve ambiguity in the passive target tensor.
- If the continuity requirement fails, define and validate a common regularized target for reference and estimator, or label the result only as a sensitivity to the saved frozen forcing. Do not claim it is a verified derivative of live nonlinear EPM.

### 6.3 Preserve energy and boundary consistency

Changing the momentum stress requires the corresponding stress-work contribution to the compressible energy flux. Reuse a common tensor-to-viscous-flux route so the change does not affect momentum alone.

Audit interior edges, applicable boundary viscous fluxes, strong boundary rows, source contributions, and objective evaluation. The exact endpoint identities must hold for the assembled equations, not just an interior helper.

Smooth wall force evaluation currently uses molecular viscosity in the relevant routine. Do not add eddy-viscosity traction there merely because the interior tensor changes. Verify the wall model and the actual direct objective dependence for the supported wall-resolved case.

Keep the baseline preconditioning/update definition at zero amplitude. If using approximate implicit Jacobians for convergence, document them as iteration machinery; they are not the exact residual derivative required by a residual-adjoint formula.

## 7. Reuse the native discrete adjoint correctly

### 7.1 Residual and iteration-map adjoints are different objects

A theoretical residual adjoint satisfies:

\[
R_{0,q}^{T}\psi=J_q^{T},
\qquad
g_m=J_a-\psi^{T}b_m.
\]

However, the inspected SU2 discrete-adjoint driver records one iteration of the direct solver. Write the recorded map as `(q,a) -> (G(q,a), H(q,a))`, where `q^+=G(q,a)` and `H` is the actual taped objective computation. The corresponding native adjoint satisfies:

\[
(I-G_q^{T})\lambda=H_q^{T},
\qquad
g_m=H_a+\lambda^{T}G_a.
\]

Retain the driver's actual computational ordering and seed the objective through the tape. Calling `SetObjFunction` after `DirectRun` does not by itself establish `H=J composed with G`: the objective can read cached quantities computed during the iteration. Verify that its value at the steady solution matches the intended reported QoI.

**Do not multiply SU2's stored iteration-map adjoint directly by the raw equation residual difference.** For a simple map `G=q-BR`, a residual-adjoint conversion involves `B^T`. SU2's block update, linear solve, or other iteration machinery must be accounted for by its recorded derivative, not guessed.

### 7.2 Recommended extraction lifecycle

Reuse `SetRecording`, `RegisterInput`, `SetDependencies`, `DirectRun`, `RegisterOutput`, and the native objective/adjoint seeding patterns. Add a dedicated parameter recording path if that is cleaner than overloading geometry sensitivity recording.

Follow the native clear-then-record lifecycle. `SetRecording(CLEAR_INDICES)` executes a passive direct iteration; it is not merely index cleanup. The following recording call restores the saved direct states again. Restore or rebuild custom EPM data for the final restored baseline and recorded stage, rather than assuming data prepared before the clearing pass remain valid.

Pseudocode describes responsibilities, not existing APIs:

```text
converge ordinary SST state q0
save all state and solver context needed to replay the baseline iteration

record the ordinary baseline iteration and objective
converge the fully coupled baseline discrete adjoint lambda0
save lambda0 for every participating state block

for each of the five passive EPM cases:
    restore q0 and the baseline replay context
    restore/preserve lambda0
    clear old tape indices and derivative buffers safely
    select the endpoint case; set active amplitude to zero

    prepare passive endpoint-minus-baseline contributions as needed
    begin recording
    register active amplitude before dependencies and numerics use it
    register any other inputs required by the native recording contract
    evaluate the same baseline dependencies and direct iteration,
        with active amplitude times the passive correction
    register the native state outputs and objective
    end recording

    seed every participating state output with the saved lambda0
    seed the native objective exactly once using the existing rank convention
    perform one reverse evaluation
    extract the amplitude derivative and reduce it globally exactly once
    save g[case]
    clear derivative buffers without overwriting lambda0

restore the baseline state/context for output or later sensitivity work
write J0, g[case], J0 + g[case], and diagnostics
```

This is five parameter recordings/reverse evaluations, not five reconvergences of the adjoint.

The native seed already includes any direct objective dependence actually recorded. Do not add a separately computed `J_a` a second time.

Because the same adjoint is reused, verify that all five zero-amplitude recordings share the same state-map derivative and baseline objective derivative. Equality of residual values alone is insufficient if a mode changes the iteration, preconditioner, limiter behavior, or hidden state.

### 7.3 State, threads, and MPI

- Restore all relevant direct state, gradients/dependencies, turbulence state, limiter status, and iteration-control context. Prepare coupled passive corrections only after every participating direct solver has been restored and dependencies are consistent, or at the corresponding residual stage. Preparing them in the FLOW scalar-reset hook can be too early because TURB restoration may follow it.
- Preserve the converged adjoint arrays across recordings. After the parameter reverse, extract only parameter sensitivities before clearing derivatives. Do not call `IterateDiscAdj`: it also extracts new state adjoints and would overwrite the arrays that must seed every case.
- Before registration, set the amplitude to zero and reset its old AD index. After registration, preserve that active scalar's identity through dependent calculations and reverse evaluation; assigning a passive zero at this point can sever its dependence. After extraction, clear accumulated adjoints. Follow this order for every case.
- Register shared scalar inputs with the correct thread/tape ownership. Do not register unrelated copies or create races in shared configuration.
- Use the existing scalar-sensitivity reduction pattern as a reference. A globally shared amplitude can have rank-local contributions; sum them once.
- Preserve the master's objective seed and native differentiated communication. Do not multiply the objective seed by the number of ranks or count ghost/halo contributions again.
- Verify serial versus at least two MPI ranks with the same physical case and decomposition-independent tolerances.

A five-component active parameter vector can later expose all projections in one recording, with all components zero. That is an optimization after the simpler five-replay implementation is correct. Nonlinear comparisons activate one case at a time.

## 8. Proposed configuration interface

**The `SST_EPM_ANALYSIS/CASE/AMPLITUDE/OUTPUT` names below remain proposed and are not implemented. The current experimental entry point is `SST_EPM_POSTPROCESS` described in Section 1.** Adjust names to repository conventions, preserving these semantics.

| Option | Proposed values | Meaning |
|---|---|---|
| `SST_EPM_ANALYSIS` | `NONE`, `CONTINUATION`, `ADJOINT_ESTIMATE` | Disabled by default; direct continuation or five-case adjoint estimate. |
| `SST_EPM_CASE` | `1C_KEEP`, `1C_SWAP`, `2C_KEEP`, `2C_SWAP`, `3C` | Selects a nonlinear continuation case. Estimation explicitly evaluates all five. |
| `SST_EPM_AMPLITUDE` | `0 <= a <= 1` | Independent continuation amplitude; require zero for estimation. |
| `SST_EPM_OUTPUT` | filename, if needed | Case table/metadata output; follow existing output conventions. |

Reuse existing `UQ_DELTA_B` and `UQ_URLX` only as fixed endpoint controls, with explicit values in examples. Derive component/permutation from the new case selector; reject conflicting selectors rather than silently choosing one.

For the new mode, retain `SST_OPTIONS=V2003m` as the ordinary baseline. Do not require the legacy `UQ` modifier, which changes the baseline branch. Internally evaluate the corrected target contributions without toggling global configuration during a tape.

Example intended estimator configuration:

```ini
SOLVER= RANS
KIND_TURB_MODEL= SST
SST_OPTIONS= V2003m
MATH_PROBLEM= DISCRETE_ADJOINT
OBJECTIVE_FUNCTION= DRAG

FROZEN_VISC_DISC= NO
FROZEN_LIMITER_DISC= NO
INCONSISTENT_DISC= NO

% Proposed options to implement:
SST_EPM_ANALYSIS= ADJOINT_ESTIMATE
SST_EPM_AMPLITUDE= 0.0

% Existing options, fixing the target model:
UQ_DELTA_B= 1.0
UQ_URLX= 1.0
```

This is a configuration fragment, not a complete runnable case. Supply the mesh, boundary conditions, restart filenames, numerics, and convergence settings in the example deliverable.

For nonlinear continuation use `MATH_PROBLEM=DIRECT`, select `CONTINUATION`, one case, and a finite amplitude.

For standalone endpoint-identity checks, the corrected legacy EPM branch uses `SST_OPTIONS=(V2003m,UQ)` with the corresponding existing `UQ_COMPONENT` and `UQ_PERMUTE`. Keep the new analysis mode disabled there.

## 9. Required verification and acceptance gates

Do not conflate a correct gradient with an accurate full-amplitude extrapolation.

### Gate A — reference EPM correctness

Add the focused tensor and production tests in Section 5. Include scalar/SIMD agreement for enabled paths. Establish the target model and record any changed existing UQ regression values with a scientific explanation.

Do not update expected values merely to make tests pass.

### Gate B — exact endpoint and baseline identities

At representative admissible states, not only a converged solution:

- `a=0`: assembled residuals, affected face fluxes/sources, boundaries, and objective equal ordinary SST.
- `a=1`: they equal corrected standalone EPM for each mode.
- At `a=0`, state-map and objective derivatives agree across modes and with the baseline.
- New feature disabled: ordinary SST behavior is unchanged.
- Deliberately zero residual and explicit objective corrections produce zero parameter sensitivity. A zero residual correction alone does not imply a zero sensitivity if the objective has direct amplitude dependence. Do not use `UQ_DELTA_B=0` with an active permutation as a zero-correction test.

Use scale-aware floating-point tolerances and record them.

### Gate C — local parameter derivative and replay correctness

At a fixed restored state, compare the recorded one-iteration parameter derivative `G_a` against finite differences of the same iteration and explicit objective evaluation.

This detects lost active parameters, wrong signs, preaccumulation omissions, stale constructor copies, stage errors, and broken reductions before a costly full-solution check.

Confirm the saved adjoint and baseline replay state survive all five cases. Exercise the selected scalar/SIMD and MPI paths.

### Gate D — coupled gradient verification

For a smooth attached case, converge the baseline and small positive-amplitude nonlinear continuation solutions using identical endpoint definitions.

For successive small amplitudes `h, h/2, h/4`, evaluate:

\[
E_m(h)=\left|J_m(h)-J_0-hg_m\right|.
\]

In a smooth regime with sufficiently converged solves, expect a second-order Taylor remainder until numerical/convergence noise dominates. Choose amplitudes and tolerances from measured solver noise; do not hardcode an unrealistically small finite-difference step.

Use positive one-sided tests at the boundary `a=0`. Record production caps, SST max/min switches, and eigenvalue gaps. At a nondifferentiable switch, document the branch/directional behavior rather than interpreting every loss of quadratic scaling as an AD bug.

Require all five derivatives to pass the relevant checks, including a nonzero sensitivity case. A zero gradient alone is not a useful validation.

### Gate E — finite-amplitude accuracy study

After derivative correctness, compare first-order predictions against nonlinear solutions at a useful progression such as `a=0.05, 0.1, 0.25, 0.5, 1`. Begin with an attached case, then use an adverse-pressure-gradient or separated case with a credible steady solution.

The same mesh, model corrections, endpoint factors, numerical settings, objective, and convergence standards must be used on both sides.

For each case and amplitude report:

- Nonlinear QoI, first-order prediction, signed difference, and absolute difference.
- A sensible normalized error scale, with protection when the nonlinear change is close to zero.
- Which case supplies the observed minimum and maximum.
- Predicted and nonlinear envelope widths, and how many of the five nonlinear samples fall inside the predicted envelope.
- Primal/adjoint convergence and any clipping, eigenvalue, or steady-solution issues.

Coverage of these five deterministic samples is not probabilistic coverage or proof of global bounds.

If a full-amplitude solution fails to converge, report that failure. Do not substitute an unconverged iteration as the reference or quietly weaken the target.

### Cost accounting

Measure separately:

- Baseline nonlinear solve.
- Baseline adjoint convergence.
- Parameter recordings and reverse evaluations, including any linear solves they invoke.
- Five nonlinear EPM reference solves.
- Memory/tape footprint, process count, and hardware.

The intended production cost is one baseline plus one adjoint per objective plus inexpensive parameter work. The validation campaign deliberately performs additional nonlinear calculations. Report measured timings before claiming a speed-up.

## 10. Output contract

Write a table with at least:

```text
case, objective, J_baseline, dJ_da_at_zero, J_linear_at_one,
delta_b, tensor_blend_factor, primal_converged, adjoint_converged
```

Put units/normalization, actual commit, solver/build/AD options, mesh identifier, configuration provenance, iteration settings, and gradient-verification status in a companion metadata object or documented output header. Record whether endpoint bug fixes are present.

Add nonlinear values/errors when running the comparison script. Keep unavailable nonlinear results empty or explicitly marked unavailable; never present linear estimates as completed nonlinear simulations.

Print a clear console summary that identifies the approximation as first order and distinguishes the endpoint amplitude from `UQ_DELTA_B` and `UQ_URLX`.

## 11. Suggested implementation order and deliverables

Use this order so each stage has an independently assessable purpose:

1. Confirm current source behavior; add discriminating EPM tests and correct the reference tensor/production paths if still needed.
2. Add direct continuation and prove baseline/endpoint identities.
3. Add active scalar plumbing and verify one-iteration parameter derivatives.
4. Add reuse of the converged coupled adjoint for five parameter replays.
5. Add machine-readable output, configuration checks, and a converged example.
6. Verify coupled derivatives, then run the finite-amplitude comparison.

Deliver:

- A focused source patch with a clear explanation of reference corrections versus estimator additions.
- Unit/integration tests for the concrete risks above, integrated with the repository's existing test framework.
- A complete small example case and commands for ordinary baseline, adjoint estimate, continuation, and standalone endpoint checks.
- A comparison script that reads SU2 outputs and produces the case/error/cost table without launching unexpected large jobs.
- The staged campaign helper and fully expanded run configurations specified in Section 12, including reuse of a compatible existing primal and direct access to the five sensitivity values.
- Developer documentation of the continuation, adjoint convention, supported configurations, and limitations.
- A final report of exact commands run, results, failures, numerical tolerances, and unverified extensions.

Do not implement a new adjoint solver, stochastic surrogate, Hessian method, or full-field uncertainty propagation in this first task. Do not claim that a scalar adjoint produces uncertainty bands for every field point. A small set of scalar QoIs is the intended use; full-field responses would motivate tangent solves or another approach.

## 12. Operational runbook: simulations and sensitivity results

This section defines a future validation workflow after implementing Sections 1–11 and resolving the target-limit gate. It is not authorized for execution in this infrastructure-only task. It also defines the user workflow for an already implemented patch. The new `SST_EPM_*` options, campaign helper, and sensitivity CSV files below are **implementation requirements**, not features available in unmodified SU2. Only the tiny infrastructure regressions described above have been executed; they do not validate this campaign.

### 12.1 Build and identify the executables

Build the normal and reverse-mode AD executables from the same modified source revision. At the inspected SU2 commit, the relevant existing build options are `enable-normal`, `enable-autodiff`, `with-mpi`, and `with-omp`. Use the repository's `meson.py` wrapper so its preconfiguration runs.

From the repository root, a representative build is below. The shell examples use Bash and retain the `EPM_*` variables in the same shell; a campaign runner should instead pass explicit paths and working directories to its subprocesses.

```bash
set -euo pipefail
EPM_REPO_ROOT="$PWD"
EPM_INSTALL_PREFIX="$EPM_REPO_ROOT/install-epm"

python3 meson.py setup build-epm \
  --prefix="$EPM_INSTALL_PREFIX" \
  -Denable-normal=true \
  -Denable-autodiff=true \
  -Dwith-mpi=enabled \
  -Dwith-omp=false

./ninja -C build-epm install

EPM_PRIMAL="$EPM_INSTALL_PREFIX/bin/SU2_CFD"
EPM_ADJOINT="$EPM_INSTALL_PREFIX/bin/SU2_CFD_AD"
```

If an installation already exists, inspect and reuse a compatible build directory or choose a new one; do not overwrite unrelated build settings. If MPI is unavailable, use `-Dwith-mpi=disabled` and report MPI verification as pending. Start with one process and no OpenMP, then verify two MPI ranks.

Record source commit, patch/diff identity, compiler, build configuration, executable paths, MPI library, and process count in the campaign manifest. Building a forward-mode/direct-differentiation executable is unnecessary for this workflow.

Sources: [build options](https://github.com/su2code/SU2/blob/f4b67d23deaac22948cc8d2a2c0da3138ae804f6/meson_options.txt), [CFD executable targets](https://github.com/su2code/SU2/blob/f4b67d23deaac22948cc8d2a2c0da3138ae804f6/SU2_CFD/src/meson.build), [build wrapper](https://github.com/su2code/SU2/blob/f4b67d23deaac22948cc8d2a2c0da3138ae804f6/meson.py).

### 12.2 First physical case and exact mesh

Use a controlled, attached-flow numerical verification case:

| Setting | Initial value |
|---|---|
| Geometry | Two-dimensional NACA0012, unit chord |
| Mach number | 0.15 |
| Chord Reynolds number | 6,000,000 |
| Angle of attack | 2 degrees |
| Freestream temperature | 300 K |
| Turbulence model | Fully turbulent SST-2003m |
| Freestream turbulence intensity | 0.001, expressed as a fraction |
| Freestream turbulent/laminar viscosity ratio | 10 |
| Wall | Smooth, stationary, no-slip, adiabatic |
| Other boundary | Farfield |
| Initial objective | Drag coefficient, unit objective weight |
| Second objective | Lift coefficient, separate adjoint |
| EPM endpoint controls | `UQ_DELTA_B=1`, `UQ_URLX=1` |

These are proposed controlled verification inputs; they are not a claim to reproduce a particular experimental validation condition. Confirm the baseline is attached from wall shear and field data. A nonzero angle avoids making lift verification trivial through symmetry.

Use the actual mesh from the separate official `su2code/TestCases` repository, pinned to `4d2b3728e93f657199c9eec1919c4a752e1159cb`:

[Official n0012_225-65.su2 mesh](https://raw.githubusercontent.com/su2code/TestCases/4d2b3728e93f657199c9eec1919c4a752e1159cb/rans/naca0012/n0012_225-65.su2)

Verified asset metadata:

- Actual repository path: `rans/naca0012/n0012_225-65.su2`.
- Size: 1,253,807 bytes.
- Git blob hash: `a16d4dafde7d62c7fec3e4a54717cb30d79ccf81`.
- `NDIME=2`, `NPOIN=14576`, `NELEM=14336`.
- Boundary markers: `airfoil` and `farfield`.
- The corresponding adjoint-directory mesh is a symlink; use the actual path above to avoid downloading a file containing only the symlink target.

For example, from the repository root:

```bash
EPM_RUN_ROOT="$EPM_REPO_ROOT/epm-runs/naca0012"
mkdir -p "$EPM_RUN_ROOT/mesh"

curl --fail --location \
  "https://raw.githubusercontent.com/su2code/TestCases/4d2b3728e93f657199c9eec1919c4a752e1159cb/rans/naca0012/n0012_225-65.su2" \
  --output "$EPM_RUN_ROOT/mesh/n0012_225-65.su2"

git hash-object "$EPM_RUN_ROOT/mesh/n0012_225-65.su2"
sha256sum "$EPM_RUN_ROOT/mesh/n0012_225-65.su2"
```

Require the Git blob hash to match the value above, validate mesh headers/markers, and save the computed SHA256. Use the exact same mesh bytes for all calculations in the comparison. Measure and report wall resolution, including observed `y+`, after the baseline run. This is a modest verification mesh, not evidence of grid-independent physical uncertainty estimates.

Use the existing discrete-adjoint NACA0012 SST configuration as a starting point, with explicit overrides. Recommended initial numerics are Roe with MUSCL for the flow, no flow slope limiter, scalar-upwind turbulence with `MUSCL_TURB=NO`, implicit Euler for both systems, `MGLEVEL=0`, `CFL_NUMBER=5.0`, and `CFL_ADAPT=NO`. Start with FGMRES/ILU, `LINEAR_SOLVER_ERROR=1E-10`, and a sufficient inner iteration cap such as 100; set the supported discrete-adjoint linear-solver controls consistently. These are proposed starting settings, not verified convergence guarantees.

Keep the same spatial discretization, reconstruction, boundaries, physical inputs, and objective normalization in every comparison. If convergence requires changing iteration controls, record the change and rebuild/reconverge the affected adjoint map where applicable. Do not change the physical endpoint to hide a convergence failure.

### 12.3 Reuse an existing primal when it is compatible

A fresh primal solve is **not automatically required**. The test is whether the saved state still satisfies the intended baseline equations:

\[
R_{\mathrm{new}}(q_{\mathrm{saved}},0)\approx0.
\]

Require the original mesh and complete flow/SST restart, including `k` and `omega`, plus enough configuration/provenance to establish model compatibility. Restore or rebuild dependent fields consistently.

| Existing data | Action |
|---|---|
| Converged ordinary SST-2003m, same mesh, physical settings, residual discretization, and unchanged baseline | Load, verify residual/objective, and reuse. |
| Same baseline but restart precision or incomplete convergence leaves appreciable residuals | Continue the primal from the saved state and write a distinct new restart. |
| A different SST variant, changed baseline equations, mesh, boundaries, or physical inputs | Treat the old state only as an initial guess and reconverge the intended baseline. |
| Old nonlinear EPM state produced before reference-model corrections | Recompute/reconverge before using it as a corrected EPM reference. |
| Existing adjoint | Reuse only if its objective, coupling, recorded map, and derivatives are identical; otherwise use a fresh adjoint or a compatible initial guess and reconverge. |

In particular, the inspected adjoint NACA0012 example uses `V1994m`; its converged solution is not automatically a `V2003m` solution. An upgrade between SU2 versions can also change defaults or residual implementation.

Evaluate all baseline flow and turbulence residual components with the new code, not only lift/drag stability. Preserve the input restart. If a short restart check advances the solution, give its output a distinct name and record whether the original or reconverged state is selected as `q0`.

The adjoint itself performs direct preprocessing and tape-recording iterations. These are part of sensitivity evaluation and do not mean the primal must be converged again from scratch.

### 12.4 Implemented paper-case preparation

The preparation-only implementation is [paper_cases/prepare.py](../TestCases/disc_adj_rans/sst_epm_adjoint/paper_cases/prepare.py), with the complete physical definitions, mesh sources and acceptance plan in [paper_cases/README.md](../TestCases/disc_adj_rans/sst_epm_adjoint/paper_cases/README.md). It emits only options accepted by this branch. The internal common/case overlays are expanded into standalone `run.cfg` files; SU2 has no implicit include requirement.

From the repository root, prepare each case without launching a solver:

```bash
PAPER_CASES=TestCases/disc_adj_rans/sst_epm_adjoint/paper_cases
python3 "$PAPER_CASES/prepare.py" naca0012 build-epm-local/paper/naca0012
python3 "$PAPER_CASES/prepare.py" nasa_hump build-epm-local/paper/nasa_hump
python3 "$PAPER_CASES/prepare.py" onera_m6 build-epm-local/paper/onera_m6
```

Without `--mesh`, the manifests explicitly say `mesh_required`. With `--mesh /path/to/mesh.su2`, the generator validates dimension/count/marker metadata, copies the ASCII SU2 mesh and records its source path and SHA256. Always use a new output directory. Geometry correctness, wall resolution, mesh independence and experimental setup matching remain separate gates. No benchmark mesh is bundled. The NASA hump converter accepts the official single-block 2D no-plenum PLOT3D format and preserves chord units, full precision and source hash; it has only been tested on synthetic input so far.

| Path under each case root | Purpose |
|---|---|
| `mesh/mesh.su2` | Supplied mesh; absent until explicitly supplied |
| `baseline/run.cfg` | Ordinary direct SST-2003m baseline |
| `baseline/restart_flow.dat` | Future converged binary baseline output |
| `adjoint_drag/run.cfg`, `adjoint_lift/run.cfg` | Fresh coupled adjoints, same primal, extraction enabled |
| `adjoint_*/sst_epm_sensitivities.csv` | Future per-objective extraction output |
| `nonlinear/<case>/run.cfg` | Five standalone corrected classical endpoints |
| `manifest.json` | Job dependencies, input hashes, unrun status and pending gates |

All generated paths resolve from the job's actual working directory. Each run writes its own restart/history/output files and cannot overwrite the common baseline. Fresh adjoints use `RESTART_SOL=NO`, `SOLUTION_FILENAME=../baseline/restart_flow` and `SST_EPM_POSTPROCESS=YES`; loading the saved primal is implicit in the discrete-adjoint mode. Use a separate saved-adjoint restart only intentionally, following SU2's objective suffix convention. Do not reuse a frozen-viscosity adjoint as a converged fully coupled one.

Classical endpoints use `SST_OPTIONS=(V2003m,UQ)`, `UQ_DELTA_B=1`, `UQ_URLX=1`, and the component/permutation mapping in Section 3. They restart from the ordinary baseline as an initial guess. Every primal outputs both CD and CL, so there is no additional primal solve per objective.

**Unavailable continuation stage:** the proposed `SST_EPM_ANALYSIS/CASE/AMPLITUDE/OUTPUT` interface is not accepted. No small-amplitude configurations are generated. The h=0.005, 0.01, 0.02 Taylor matrix in Sections 12.7–12.8 remains a requirement blocked on that implementation. Reducing `UQ_DELTA_B` or relabeling endpoint secants would not implement the same residual continuation. A future continuation endpoint at a=1 may replace the standalone classical solve only after residual/objective identity is verified.

### 12.5 Checks and future execution stages

The lightweight preparation tests use synthetic mesh metadata and a synthetic PLOT3D grid:

```bash
python3 "$PAPER_CASES/test_prepare.py"
python3 "$PAPER_CASES/test_prepare.py" --binary /path/to/SU2_CFD_AD
```

Both tests pass locally. The optional executable check invokes only `--dryrun`: all 24 configurations pass parsing; the three baselines finish dummy-geometry preprocessing, while the 21 dependent configurations reach the expected missing-primal-restart error. SU2's dry run still attempts restart loading. These checks validate neither actual meshes nor converged physical solutions.

Production execution remains prohibited under the current authorization. Once separately authorized and meshes qualified, run `SU2_CFD_AD run.cfg > run.log 2>&1` **from the selected job directory**, using the compatible reverse-AD build without OpenMP. Complete and verify the baseline before any dependent jobs. Run the two adjoints and five endpoints only in their respective directories. No automatic run/resume/comparison or cluster launcher is provided. The manifest records preparation, not later execution status; archive actual commands, logs, times, executable/source/restart hashes and convergence certificates with each result.

A zero exit code and the 20,000-iteration cap are not convergence certificates. Apply Section 12.9 to every primal and coupled adjoint, including tighter-solve noise checks. Reuse a compatible saved primal only after Section 12.3's checks. Count baseline, adjoint convergence, tape/replays and nonlinear reference costs separately. Do not silently launch validation cases when requesting an estimate.

The existing tiny `check_extraction.py --ranks 2 --reference ...` is the local MPI verification path. Its runtime gate remains pending because the sandbox cannot create the required sockets. Do not infer parallel correctness from syntax compilation or bypass that restriction. Production MPI execution requires a later successful runtime check.

### 12.6 Exactly how the sensitivity result is obtained

The implemented `SST_EPM_POSTPROCESS=YES` driver performs the five canonical replays after the selected baseline adjoint is converged, with additional eigenspace probes when required. For each replay:

1. Restore the same baseline and preserve the converged flow and turbulence adjoint arrays.
2. Register the persistent amplitude at zero before dependent computations.
3. Record the direct iteration and the actual native objective, with the case correction inserted as specified.
4. Seed the state outputs with the saved adjoint and the steady native objective with the existing master-rank seed.
5. Reverse the tape once.
6. Read `SU2_TYPE::GetDerivative(amplitude)` before clearing the derivative storage.
7. Apply the correct scalar MPI accumulation exactly once and store the global result.
8. Write the five results to the requested CSV after completing the replays, without overwriting the saved state adjoint.

The result is:

\[
g_m=\left.\frac{dJ_m}{da}\right|_0=H_a+\lambda^T G_a.
\]

This is the derivative with respect to the **new dimensionless amplitude**. Do not negate the extracted value, add the direct objective term again, or multiply/divide by `UQ_DELTA_B` or `UQ_URLX`. Those controls are already included in the endpoint correction.

The patch must explicitly write the new model-parameter derivative. Existing geometric sensitivity files, adjoint fields, adjoint residuals, `SENS_AOA`, `SENS_MACH`, and `SENS_PRESS` are not this result.

No `SU2_DOT` invocation is required for this scalar extraction. At the inspected commit, its main program constructs a deformation driver; the requested derivative is obtained directly from the CFD AD tape. Sources: [existing scalar extraction/reduction](https://github.com/su2code/SU2/blob/f4b67d23deaac22948cc8d2a2c0da3138ae804f6/SU2_CFD/src/solvers/CDiscAdjSolver.cpp), [SU2_DOT entry point](https://github.com/su2code/SU2/blob/f4b67d23deaac22948cc8d2a2c0da3138ae804f6/SU2_DOT/src/SU2_DOT.cpp).

Require exactly five case rows per scalar objective, with at least the Section 10 fields and these additional diagnostics:

```text
case,objective,a_reference,J_baseline,dJ_da_at_zero,J_linear_at_one,
delta_b,tensor_blend_factor,epsilon_J_baseline,epsilon_g,
primal_converged,adjoint_converged,parameter_replay_valid,verification_status
```

Set `a_reference=0`. Use round-trip double precision, normally 17 significant decimal digits, in the custom writer independently of screen-output precision. Missing values must be explicit, not silently replaced by zero.

For each row:

\[
\Delta\widehat J_m(a)=a\,g_m,\qquad
\widehat J_m(a)=J_0+a\,g_m.
\]

For `J=CD` or `J=CL`, the sensitivity is a coefficient change per unit amplitude. Positive `g_m` means increasing the amplitude initially increases that signed coefficient.

Export and check the exact objective scalar evaluated and registered by `SetObjFunction`, then seeded by `SetAdjObjFunction`, including its weights, normalization, sign, and monitored markers. For the initial unit-weight, single-objective examples, demonstrate that this scalar equals the reported physical coefficient. If `J=sC+c` with constant nonzero `s`, convert by `dC/da=g_J/s` and record the mapping. Do not compare a weighted composite objective gradient against an unweighted coefficient.

As a minimal user-facing extraction check, from `adjoint_drag/` the following should print the five rows using only the Python standard library:

```bash
python3 - <<'PY'
import csv
with open("epm_drag.csv", newline="") as f:
    rows = list(csv.DictReader(f))
assert len(rows) == 5, "Expected one row for each EPM case."
for row in rows:
    j0 = float(row["J_baseline"])
    g = float(row["dJ_da_at_zero"])
    print(row["case"],
          "dCD/da =", format(g, ".17g"),
          "CD_linear(a=1) =", format(j0 + g, ".17g"))
PY
```

The comparison helper must additionally validate the five unique case names, status flags, input identities, and consistency of the repeated baseline value. The small reader above only demonstrates how to access the result.

### 12.7 Required simulation matrix and honest run counts

Use the following staged core campaign. Small amplitudes `0.02,0.01,0.005` are proposed starting probes, not guaranteed to be in a linear regime.

| Stage | Calculations | Distinct converged primal states | Coupled adjoint solves |
|---|---|---:|---:|
| Baseline | Ordinary SST-2003m at 2 degrees | 1, or reuse | 0 |
| Drag estimator | Same baseline, five parameter replays | 0 | 1 |
| Lift estimator | Same baseline, five parameter replays | 0 | 1 |
| Gradient verification | Five cases at each of `a=0.02,0.01,0.005` | 15 | 0 |
| Full endpoints | Five cases at `a=1` | 5 | 0 |
| **Core total** | Both physical coefficients saved in every primal run | **21 including baseline** | **2** |
| Optional response curves | Five cases at each of `a=0.1,0.5` | **10 additional** | 0 |

The two estimator runs involve ten parameter recordings/reverse evaluations in total. Residual-identity checks, one-iteration derivative checks, and the convention-offset diagnostic are evaluations rather than converged primal states; measure their cost separately.

For routine estimation of drag alone, the work is zero or one new baseline convergence solve, one adjoint, and five parameter replays. The additional 20 nonlinear states belong to validation. A second objective requires another adjoint but can reuse every nonlinear state if its objective was saved.

A sensible execution order is: baseline; drag adjoint; a pilot small-amplitude check for a case with a resolved nonzero slope; remaining gradient checks; lift adjoint; full endpoints. Reuse the pilot points in the 15-state total. Do not proceed with an unexplained derivative discrepancy.

Start every core small-amplitude and endpoint run from the common baseline. This makes the timing of the five full endpoint solves comparable. If an endpoint needs intermediate continuation to converge, record the extra states and include their cost in obtaining it. Do not count initialization from a previously converged perturbed state as free.

The full-amplitude continuation solution is reusable as a corrected standalone EPM reference after the endpoint residual/objective identities pass. Extra standalone solves are needed only to resolve a concrete remaining inconsistency, not as routine duplicate calculations.

### 12.8 Compute derivative checks and prediction errors

For a converged nonlinear continuation value `J_m(a)`, evaluate the same objective used by the adjoint and compute:

\[
\Delta J_m(a)=J_m(a)-J_0,
\qquad
g_{\mathrm{FD},m}(a)=\frac{J_m(a)-J_0}{a},
\]

\[
J_{\mathrm{linear},m}(a)=J_0+a g_m,
\qquad
T_m(a)=J_m(a)-J_{\mathrm{linear},m}(a).
\]

Use `T_m(a)` as the signed prediction error: positive means the linear prediction is too low. Its absolute value is the Taylor remainder magnitude and the absolute prediction error.

For two positive amplitudes `h,h/2`, also calculate the second-order one-sided Richardson estimate:

\[
g_{\mathrm{R},m}(h)
=2g_{\mathrm{FD},m}(h/2)-g_{\mathrm{FD},m}(h)
=\frac{4J_m(h/2)-J_m(h)-3J_0}{h}.
\]

For the proposed triplet, this gives estimates at `h=0.02` and `h=0.01`. In a smooth regime, expect `g_FD-g_AD=O(h)`, `g_R-g_AD=O(h^2)`, and `T=O(h^2)`.

When both remainders are resolved, compute:

\[
p_T(h)=\log_2\frac{|T_m(h)|}{|T_m(h/2)|}.
\]

Approximately two is the usual leading behavior. Do not reject a correct result because the quadratic coefficient vanishes or because both remainders are already below the numerical noise floor.

At `a=1`, report the full endpoint comparison:

\[
J_{\mathrm{linear},m}(1)=J_0+g_m,\qquad
e_m=J_m(1)-[J_0+g_m].
\]

Do not interpret `[J_m(1)-J_0]` as an independently measured derivative. It is a finite endpoint change. Keep `g_AD`, `g_FD(h)`, and endpoint changes in distinct columns. Populate `g_FD` for the declared small-amplitude verification rows; leave it unavailable in endpoint rows. If exporting the quotient at other finite amplitudes, call it `secant_slope` explicitly.

For each objective, compute the predicted and nonlinear minimum/maximum over the same set containing the baseline and five cases. Report the extremizing cases, both envelope widths, and inclusion of the five nonlinear endpoint samples in the predicted range. This is a deterministic comparison, not a probability or a confidence interval.

Report absolute errors first. A change-relative error `|e_m|/|J_m(1)-J_0|` is useful only when the nonlinear change is resolved and appreciably nonzero. Otherwise mark that percentage unavailable; do not hide the issue with an arbitrary denominator. A separate error normalized by a declared fixed objective scale may be reported.

### 12.9 Convergence, precision, and acceptance

Use a generous iteration cap, for example 20,000, as a failure limit rather than convergence evidence. Monitor all flow and SST residual components. An initial goal of at least eight orders of residual reduction is useful, but it is not sufficient by itself and may not apply to a restart already near convergence.

Record absolute residuals and objective drift over a final window, initially 200 iterations. Extend or tighten the baseline and smallest-amplitude solves where needed to estimate the remaining numerical change. A quiet history window alone can miss slow drift.

Estimate:

- `epsilon_J(a)`: numerical uncertainty in each converged objective from further convergence/tighter settings.
- `epsilon_g`: sensitivity stability under further adjoint convergence and, where necessary, a tighter primal baseline.

These are numerical error estimates, not statistical confidence limits. A new extraction at a later adjoint checkpoint is useful for measuring `epsilon_g`; preserve the converged adjoint and count extra replays.

Use conservative propagated estimates:

\[
\epsilon_{\mathrm{FD}}(h)
=\frac{\epsilon_J(h)+\epsilon_J(0)}{h},
\]

\[
\epsilon_{\mathrm{R}}(h)
=\frac{4\epsilon_J(h/2)+\epsilon_J(h)+3\epsilon_J(0)}{h},
\]

\[
\epsilon_T(h)
=\epsilon_J(h)+\epsilon_J(0)+h\epsilon_g.
\]

Do not assume the common baseline error is independent random noise across runs.

Declare an absolute-plus-relative sensitivity tolerance:

\[
\tau_g=\tau_{\mathrm{abs}}
+\tau_{\mathrm{rel}}\max(|g_m|,|g_{\mathrm{R},m}|).
\]

For the initial coefficient-based pilot, proposed starting values are `tau_abs=1e-6` coefficient per unit amplitude and `tau_rel=1e-2`. These are engineering acceptance settings to record and assess, not mathematical guarantees or a substitute for step refinement.

Require the estimated numerical uncertainty to use only a small part of the tolerance budget; a useful initial check is `epsilon_R+epsilon_g <= 0.1*tau_g`. Then assess `|g_AD-g_R| <= tau_g` together with step-refinement behavior. Assign `gradient_verified` using the finest noise-resolved Richardson pair and refinement evidence, rather than demanding a truncation-dominated coarse pair pass or accepting an isolated passing row. Only fit Taylor orders using remainders comfortably above noise, for example `|T| > 10*epsilon_T`.

Reduce the probe amplitude when truncation effects dominate and the changes remain resolved. If halving amplifies convergence/rounding errors, tighten the solves or test a larger amplitude, with an initial pilot ceiling such as 0.1, then recheck linearity. Reuse completed points and log all added calculations.

A zero or tiny slope can be legitimate; use absolute tolerances and test at least one resolved nonzero case to exercise parameter plumbing. A zero slope does not establish that the full endpoint response is small.

Use explicit result statuses, such as:

- `gradient_verified`.
- `gradient_inconclusive_noise`.
- `gradient_disagreement`.
- `target_limit_unverified`.
- `nonlinear_reference_unconverged`.
- `endpoint_error_measured`.

Production switches, nonunique steady solutions, and ambiguous eigenspaces require interpretation. An inconclusive finite-difference check must not be silently counted as a pass or automatically called an AD bug.

### 12.10 Comparison files, figures, and final report

The postprocessor must produce one row per case/objective/amplitude, including:

```text
case,objective,amplitude,J_baseline,J_nonlinear,delta_J,
J_linear,g_AD,g_FD,Taylor_remainder,abs_prediction_error,
epsilon_J,epsilon_FD,epsilon_T,primal_converged,status
```

Put the two-step quantities in `gradient_check.csv` with an explicit coarse-step column:

```text
case,objective,h,g_AD,g_R,gradient_discrepancy,
epsilon_R,epsilon_g,tolerance,Taylor_order,status
```

Require joins by case, objective, amplitude, baseline identity, model settings, and mesh hash. Reject mismatched or missing inputs instead of silently comparing unrelated runs. Use final converged objective values at full precision, not rounded console values or blindly the last row of an unconverged history.

The final report must contain:

1. The actual five sensitivities for each computed objective and their verification status.
2. Taylor/finite-difference evidence, including step sizes and noise estimates.
3. Linear and nonlinear endpoint values, signed/absolute errors, and envelope comparison.
4. Cost for baseline, each adjoint, parameter extraction, nonlinear validation, and any retries.
5. Mesh/model/build provenance and supported configuration.
6. Failures and unresolved limitations.

Generate standard scientific plots when available: `J(a)` versus `J0+a*g` for each case, a Taylor-remainder plot with only resolved points, and endpoint comparison plots. Save the tabular data behind every plot. Actual nonlinear `Cp/Cf` curves can help explain discrepancies; these are not adjoint-predicted uncertainty fields.

Report one-objective and two-objective costs separately. Five nonlinear EPM calculations provide all monitored quantities together; the adjoint method needs one solve per independent scalar objective.

### 12.11 Paper matrix and completion boundary

Use three principal cases, with the exact preparation inputs and primary references in [paper_cases/README.md](../TestCases/disc_adj_rans/sst_epm_adjoint/paper_cases/README.md):

| Case | Conditions | Role |
|---|---|---|
| NACA 0012 | 2D, M=0.15, Re=6e6, alpha=2 degrees | Attached-flow verification, nonzero drag and lift |
| NASA wall-mounted hump, no plenum/control | 2D, M=0.1, Re=936000 | Separation, reattachment and model sensitivity |
| ONERA M6 test 2308 | 3D, M=0.8395, Re=11.72e6, alpha=3.06 degrees | Three-dimensional transonic challenge |

The hump replaces high-incidence NACA as the main separated-flow benchmark. A 15-degree NACA calculation may be a later robustness check, but incidence alone proves neither separation nor a credible steady solution. The 2-degree NACA point is controlled verification, not a matched experimental Cp condition. ONERA M6 uses centered JST consistently across all jobs because extraction excludes slope limiters; the Roe reconstruction-gradient option must not be applied to JST.

All cases retain SST-2003m and full classical EPM targets. The available adjoint QoIs are drag and lift only. Hump forces integrate the complete lower wall, with a fixed pressure/area convention; they are estimator-reference quantities, not isolated-hump experimental forces. Cp, Cf, reattachment and section profiles are nonlinear diagnostics, not adjoint-predicted local uncertainty bands. NASA's posted SSTm results are not an exact reference for this branch's model variant.

At one mesh per case, the currently preparable matrix comprises 18 primal solves (three baselines plus 15 endpoints), six adjoints and 30 canonical parameter replays inside those adjoint jobs. Three meshes per case would give 72 jobs, before retries or extra eigenspace probes. If the full planned small-amplitude matrix is later implemented at one mesh per case, it adds 45 primal states: 63 primal states plus six adjoints, not 24 total jobs. Validation cost must be separated from routine estimation cost.

No physical case has been run here. Actual benchmark meshes, three-grid/y+ evidence, converged solutions, numerical-noise budgets, nonlinear Taylor checks and full-endpoint errors remain pending. The NASA mesh converter is synthetically tested; actual grid conversion/inspection is pending. If a case or endpoint does not converge steadily, record that limitation instead of using an arbitrary iteration, weakening EPM, discarding the case silently, or switching to an unsteady problem under the same claim.

A successful Taylor check establishes derivative correctness under the stated regularity/degeneracy qualifications; acceptable full-endpoint error is a separate empirical outcome. Paper figures must report individual endpoint errors as well as envelope errors, and one-/two-QoI costs separately. Infrastructure and preparation checks alone do not establish uncertainty bounds or speed-up.

## 13. Completion checklist

Current branch: corrected classical targets and scalar/fused-edge extraction in 2D/3D are implemented. Passive and reverse-AD numerics tests, full serial executable linkage, and tiny serial driver/restart checks pass. MPI accumulation, coordinated probe loops and single-writer output are implemented and compile-checked but have not run on multiple ranks. Wide SIMD/OpenMP extraction, broader boundary/physics coverage, nonlinear CFD Taylor checks, nonlinear continuation and full-endpoint accuracy remain unverified. Tiny-case sensitivity files are infrastructure evidence, not validated uncertainty bounds or a speed-up result.

Reproduce the lightweight tests without building or running a CFD executable, from the repository root. This Bash snippet requires the normal external headers, including CoDiPack for reverse mode. `CODI_INCLUDE` can point to an existing local CoDiPack include directory. During these local checks, the available sibling checkout's headers were used; their exact revision was not independently verified against the pinned submodule.

```bash
for mode in passive reverse; do
  build="build-epm-local/$mode"
  mkdir -p "$build"
  printf '#define CATCH_CONFIG_MAIN\n#include "catch.hpp"\n' > "$build/main.cpp"
  flags=(-std=c++17 -O0 -ffunction-sections -fdata-sections -I externals/catch2 -I externals/eigen -I externals/mel)
  tests=(UnitTests/SU2_CFD/numerics/epm_extraction_tests.cpp)
  if [[ "$mode" == reverse ]]; then
    flags+=(-DCODI_REVERSE_TYPE -DCODI_JACOBIAN_LINEAR_TAPE
            -I "${CODI_INCLUDE:-externals/codi/include}")
  else
    tests+=(UnitTests/SU2_CFD/numerics/epm_tests.cpp
            UnitTests/SU2_CFD/numerics/epm_source_tests.cpp)
  fi
  objects=()
  for source in \
    Common/src/CConfig.cpp \
    SU2_CFD/src/numerics/CNumerics.cpp \
    SU2_CFD/src/numerics/flow/flow_diffusion.cpp \
    Common/include/parallelization/mpi_structure.cpp \
    Common/src/toolboxes/printing_toolbox.cpp \
    Common/src/toolboxes/SwapBytes.cpp \
    Common/src/fem/fem_gauss_jacobi_quadrature.cpp \
    Common/src/basic_types/ad_structure.cpp \
    "${tests[@]}" "$build/main.cpp"
  do
    object="$build/$(basename "$source" .cpp).o"
    g++ "${flags[@]}" -c "$source" -o "$object" || exit 1
    objects+=("$object")
  done
  g++ -Wl,--gc-sections "${objects[@]}" -o "$build/test_epm" || exit 1
  "$build/test_epm" '[EPM],[EPMExtraction]' || exit 1
done
```

The extraction test is also registered in `UnitTests/meson.build` for passive and reverse-AD test drivers. Local tests cover zero-amplitude flux/source values and state derivatives, full classical local endpoints, final-contribution blending, amplitude finite differences with preaccumulation enabled, capped/uncapped production, full-strength tie probes and the accepted serial configuration. These unit tests are complemented by the tiny complete-driver checks below. The runtime guards have been exercised on both dimensions. Wider boundary coverage, distributed runtime replay and nonlinear CFD Taylor checks remain pending.


The feature is ready for review when:

- The baseline and corrected EPM endpoints are defined and tested.
- All five modes have the same zero-amplitude baseline and adjoint operator.
- The coupled scalar sensitivity is computed through SU2's native recorded iteration, with correct objective seeding and MPI accumulation.
- New parameters survive caching, preaccumulation, scalar/SIMD routes, and state restoration.
- Nonlinear small-amplitude tests support the gradients.
- Full-amplitude prediction error and cost are reported separately from derivative correctness, or explicitly marked not yet evaluated.
- Ordinary SST with the feature disabled is unaffected.
- Documentation and outputs consistently describe a linearized model-response estimate, not validated uncertainty bounds.
- The generated baseline, adjoint, and nonlinear configurations are runnable with documented commands; sensitivity CSVs and comparison tables are delivered for completed stages, with pending or failed runs explicitly identified.

If only early gates are completed, deliver the working partial implementation with an exact remaining-work list. Do not relabel it as a validated one-baseline EPM uncertainty method.

