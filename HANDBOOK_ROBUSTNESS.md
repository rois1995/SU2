# CFD robustness handover

Updated: 2026-10-08. This is the transferable record for the mesh-quality CFD work. Pending entries below are not measured results.

## User requirements

- Improve CFD on stretched, mixed and adapted meshes, including flow/turbulence MUSCL, gradients, limiters and convergence.
- Start from `codex/native-unsteady-performance`; work separately. Implementation and testing are authorized.
- **Use `MGLEVEL=0` in every simulation.** Multigrid is outside this campaign.
- Assess `feature_2ndOrderMixedGrids` critically; its existence is not evidence of correctness.
- Watch machine contention: one build worker, one simulation thread, low priority, sequential cases. Use `uptime` to check load.
- Report what changed, measured effects, and next steps. Retain negative results; do not equate finite residuals with convergence.
- Keep this handbook current. Papers exist in the original workspace's `Papers/` and `/media/rausa/4TB/SU2_Versions/Prove/Papers`.
- Latest user addition: explicitly test the **adapted RAE2822 mesh**, not only the input/conventional mesh.

## Workspaces and controls

- Implementation: `/tmp/su2-cfd-mesh-robustness`, branch `codex/cfd-mesh-robustness`.
- Exact starting main commit: `e49c545d700632bd9c68fc52535ef63d6ca8632d`.
- Original workspace: `/media/rausa/4TB/SU2_Versions/SU2_AdapNoExt`, branch `feat_adap_noExt`. Leave its user changes and evidence untouched.
- Main worktree: `/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated`.
- Another worker uses `/tmp/su2-metric-robustness` and `/tmp/su2-metric-build`; do not edit them.
- Build: `/tmp/su2-cfd-mesh-build`. Executable is `SU2_CFD/src/SU2_CFD` inside it; `SU2_CFD` alone is a directory.
- Current comparison control: `/tmp/su2-cfd-fixes-control/SU2_CFD`, copied before the fix integration from our QR-enabled build at source HEAD `4311028665`. Its `test_driver` is also preserved.
- Original no-QR baseline: `/tmp/su2-cfd-gradient-baseline/SU2_CFD`; reconstruction manifest in the same directory.
- Private build dependencies are symlinks to original `externals/`. Do not stage those links. Broken nested submodule metadata requires `git status --short --ignore-submodules=all` and `git diff --ignore-submodules=all`.
- Git metadata is read-only in the sandbox. Explicit branch/index mutations require the tool's escalation; normal source edits/builds do not. Cherry-pick continuation: `GIT_EDITOR=true git -c diff.ignoreSubmodules=all cherry-pick --continue`.

## Completed gradient work

Commit `c78571bc5e`: common LS/WLS kernel retains the existing least-squares objective and weights. It equilibrates coordinates and retries poorly conditioned, nonperiodic stencils using streaming Givens QR. QR rank threshold is 64 machine epsilons; retry is triggered by normalized determinant squared <= 1e-6.

- Covers ordinary flow, viscous/reconstruction, turbulence and adaptation callers.
- Periodic gradient exchange and separate Hessian/batched inverse paths retain their existing algorithm.
- 36 linear-field stencils: dimensions 2/3, coordinate scales 1e-7/1/1e7, aspect ratios 1/1e3/1e6, LS/WLS. Original failed 12 cases at tolerance 1e-7 (max error 8.18888e-4); QR max error 2.46785e-9.
- Gradient/Hessian unit suite: 17 cases, 115630 assertions; focused MPI2 and OpenMP2 checks passed. Forward/reverse standalone AD probes passed field and geometry derivative checks.
- Nonlinear fits tested against Eigen QR at aspect ratio 1e4. At 1e6 the fitted problem itself is sensitive (~9e-6); do not promise arbitrary nonlinear precision.
- Kernel timing: ordinary LS +0.3%, WLS +0.9%; stretched LS QR path ~2.41 times kernel cost. This is not a whole-solver speed measurement.
- Matched manufactured NS run: six meshes, both binaries, finite output; density errors unchanged. Only regular n=12 reached the target (iteration 192); others hit the 300-iteration cap. No CFD accuracy gain demonstrated there.
- Reproducible runner and stored results: `TestCases/gradient_robustness/run_mms.py`, `results_20261008.json`, `README.txt`; raw output `/tmp/su2-cfd-mms-comparison-final`.

## Reviewed sources and rejected assumptions

Audit commits `28e8e5a829`, `4311028665`; detailed source/CI records in `TestCases/gradient_robustness/branch_review_20261008.txt`, `branch_audit_20261008.json`, `branch_audit_probes_20261008.json`.

- Bellosta `feature_2ndOrderMixedGrids` at `f2a9f88574`: scalar/SIMD projection behavior differs, near-zero/sign handling is questionable, fallback leaves flux corrections inconsistent, 3D projection is incomplete, geometry AD inputs need review. No blind import.
- Upstream `feature_LocalGridLimiters` at `b92782db3bf35971d4b66c553792d537431e0bf5`: local-volume scaling is relevant, but its `2 sqrt(volume/pi)` formula is dimensionally wrong in 3D. It also changes projection reduction and drops U-MUSCL kappa in that path. Adopt only local-size regularization with a dimension-aware formula. Author reports mixed results: <https://github.com/su2code/SU2/discussions/2491>.
- Primary limiter references: Nishikawa AIAA 2022-1374, <https://doi.org/10.2514/6.2022-1374>; field-relative epsilon described in <https://ntrs.nasa.gov/api/citations/20230004018/downloads/NishikawaWhiteOConnell_v7.pdf>. An R4 limiter does not make the whole CFD scheme fourth order.
- Multigrid PR2931 is excluded by user instruction and has broad CI failures. Standalone line-based preconditioning is a separate possibility.
- Deferred conditional changes: SA wall-function PR2957, draft SST-energy/model-definition PR2946/2948, moving-flow PR2943/2944/2945, rotation-periodicity PR2961. Do not mix model changes into a numerical-robustness comparison.

## Integration in progress

At HEAD `302fc35c50`, 23 reviewed source commits were cherry-picked, preserving authorship. Original hashes and topics:

| Original commits | Change |
| --- | --- |
| 594127d414, 5e53851b93, 945b4d8b28, 3261db8448 | Frozen scalar limiter application, turbulence/species Van Albada edge limiter, avoid unused gradient work, apply flow edge limiter to scalar carrying velocity |
| 9a1fea2a25, 97288f046d, 9931cbd2b8, 9185416512 | Bounded scalar density/face fixes, SST edge regression, sliding donor weighting |
| 8e2b3aa1c4, 12c55044df | R4 negative-projection sign correction and tests |
| 5657a4728e, e16616e237, 43d0afdd2d | Wall-distance prerequisites, zero-width config validation, NEMO limiter output names |
| a9b9339828, 52378aca05, 42d241a63b, f6683944dc, 97c5d583f5, 285ea29c2c, eb5d695c5f | FGCRODR recycling/validation, positive monitoring frequency, restarted FGMRES tolerance, quantized subspace sizing, Krylov unit tests |
| caeebe2669, 8d96b78d70 | ILU owned-row level construction, identity-preconditioner halo exchange |
| ae77daf13e | Turbulence/transition point-limiter regularization relative to the variable magnitude/reference; Wang already has field-relative epsilon |

Meson conflict was resolved by preserving all native adaptation test registrations and adding limiter tests. Unrelated upstream numerical regression reference updates were not imported.

Additional local corrections being validated:

- Compressible under-relaxation computes internal **energy density** using momentum squared divided by old/new density (PR2942). The old formula multiplied by density and can spuriously cancel updates.
- Scalar face fallback uses per-lane selection, including NaN/infinity rejection, instead of multiplication by a mask (`0*infinity` is still NaN).
- Legacy CNumerics boundary callers also pass donor/farfield density and sliding donor weight to the shared bounded correction; the new native scalar boundary path already did so.
- Relative limiter reference is an AD preaccumulation input and uses its absolute magnitude.
- `LIMITER_LOCAL_LENGTH=NO` default preserves existing global flow regularization. When enabled, Venkat/R3/R4/R5 use equivalent-circle diameter in 2D and equivalent-sphere diameter in 3D, with periodic dual volume included. No dimensional volume floor; zero/nonpositive volume uses zero length and the existing epsilon floor.
- Existing `VENKAT_LIMITER_COEFF` remains tunable. Local length is an experiment, not a universally better coefficient convention.

## Commands and pending validation

Build configuration: release GCC9.4, MPI/OpenMP enabled, tests enabled, no CGNS/TecIO/MMG. Flags include `-O3 -march=native -ffast-math -fno-finite-math-only`.

During this integration, the one-worker build priority was lowered from nice=15 to nice=19 when the 8-CPU machine's load reached ~15. Lower only our Ninja process and its descendants; do not reprioritize unrelated workers. Wait for the build before starting comparison runs.

```sh
nice -n 15 /tmp/native-build-tools/ninja -C /tmp/su2-cfd-mesh-build -j1 SU2_CFD/src/SU2_CFD UnitTests/test_driver
OMP_NUM_THREADS=1 /tmp/su2-cfd-mesh-build/UnitTests/test_driver '[Limiters],[LinearAlgebra],[Gradients]'
python3 TestCases/gradient_robustness/run_mms.py /tmp/su2-cfd-fixes-control/SU2_CFD /tmp/su2-cfd-mesh-build/SU2_CFD/src/SU2_CFD /tmp/su2-cfd-fixes-mms
```

Prepend `/tmp/native-build-tools` to `PATH` when building: Meson regeneration otherwise finds an old Ninja and fails. MPI executables initialize local sockets even in serial, requiring tool escalation in this environment. Use MPI2/OMP2 only for focused correctness checks; all comparison cases are serial and `MGLEVEL=0`.

Pending: finish targeted limiter/bounds/under-relaxation tests; rebuild; run unit/parallel/AD checks; compare fixed-mesh RANS against the preserved QR control and repeat MMS; record hashes, exact configs, residuals, forces, iteration caps, nonconvergence and limits. Do not describe these as passed until recorded below.

Focused parallel validation should exercise real matrices: on a small manufactured mesh compare MPI2/OMP1 with MPI2/OMP2 using `LINEAR_SOLVER_ILU_LEVEL_SCHEDULING=YES`; compare MPI1/MPI2 with `LINEAR_SOLVER_PREC=NONE` and tight linear tolerance for identity halo exchange. Krylov unit tests use a mock identity preconditioner and alone do not validate the real halo fix. Limiter unit calls execute inside actual OpenMP parallel regions; assertions remain outside them.

Standalone AD build flags (from the worktree): `c++ -std=c++17 -O2 -ffast-math -fno-finite-math-only -ffunction-sections -fdata-sections -Wl,--gc-sections -fmax-errors=3 -Iexternals/codi/include -Iexternals/eigen -Iexternals/CLI11 UnitTests/SU2_CFD/anisotropic_ls_ad.cpp`. Add `-DCODI_FORWARD_TYPE` for forward; add both `-DCODI_REVERSE_TYPE -DCODI_JACOBIAN_LINEAR_TAPE` for reverse. The probe now includes accepted/rejected scalar-face derivatives. This does not replace a whole-solver adjoint test or local-volume limiter derivative validation.

Potential existing comparison input: native integration evidence `frozen/fine/inputs/input.su2` and `solution` under the main worktree; RAE2822 SA, Mach0.729, Re6.5e6, AoA2.31. The metric evidence `.../native_gradation_residual_v1/validated/frozen-12000/rae/weighted_least_squares-mpi1/run.cfg` is a zero-iteration postprocessing config, not a completed convergence run. Disable metric/adaptation activity for matched fixed-mesh continuation. Its JST flow and first-order turbulence will not exercise every new limiter; additional matched turbulence-MUSCL cases are needed.

### Adapted RAE input selection

- Source: `/media/rausa/4TB/SU2_Versions/SU2_AdapNoExt/integration_evidence/metric_robustness/native_gradation_residual_v1/validated/mesh-smoke/mesh_adap_00001.su2` and `solution_adap_00001.dat`.
- Snapshot: `/tmp/su2-cfd-fixes-inputs/adapted_rae/`; `snapshot.cfg` uses absolute mesh/restart paths. `manifest.json` records hashes and source paths. Never modify the external worker's originals.
- Actual adapted mesh: 15929 points, 31285 triangles (input: 18591 points, 36263 triangles). Mesh SHA256 `4cd3c068a4db0af272312ecc9638dffd635235810d5d7a6b81a15f2de6ba71e5`.
- Independent source validation reports positive volumes, conforming connectivity, no unused/duplicate points/elements, exact restart pairing, and minimum metric quality 0.3613. This is geometry/admissibility evidence, **not CFD convergence**: source run took only one flow step before/after remeshing.
- Matched candidate/current continuation holds this mesh and initial restart fixed, disables adaptation/metric computation and uses MGLEVEL=0. Test original first-order turbulence and matched turbulence MUSCL with global/local regularization.
- Independent physical longest-edge/altitude maximum: input 5546.10, adapted 3917.70; 99th percentiles 1004.24 and 910.74. Source inspection reports adapted first-height relative error 2.35e-12 for requested 1e-5. Metric quality and physical aspect ratio are different measures.
- Conventional BL comparison mesh: `/media/rausa/4TB/SU2_Versions/Prove/MeanFlowBugs/10_convergence/c2_rae2822_sa/mesh_RAE2822_turb.su2`, 13937 points, same AIRFOIL/FARFIELD tags; separate SA and SST fresh starts.
- Runner: `TestCases/gradient_robustness/run_fixes.py QR_CONTROL CANDIDATE NATIVE_CONFIG BL_MESH OUTPUT --adapted-config /tmp/su2-cfd-fixes-inputs/adapted_rae/snapshot.cfg --main-baseline /tmp/su2-cfd-gradient-baseline/SU2_CFD`. Defaults to 2000 iterations, 600-second per-run cap, serial low priority, fresh output. It records failures/timeouts rather than concealing them. No runs of the new candidate have passed yet.
- Runner variant `current` means the QR-only control at `4311028665`; `main` means the matched original e49 main binary without QR. Both are compared on adapted cases, so the limiter/convergence integration can be distinguished from the earlier gradient work.
- The coefficient-comparison cases use `LIMITER_ITER=999999` (normal continuing updates), avoiding a freeze/calibration confound. A separate 200-step adapted-SA case freezes at iteration 50 to exercise the frozen scalar-limiter fix. Its effect is not attributed solely to coefficients.

## Integration validation recorded so far

2026-10-08, candidate CFD SHA256 `5b77b9acfedaf24d7ff973aff798987edf24ca9575fc480110ccc342363e58d4`:

- Normal rebuild passed, one Ninja worker. Final limiter test source was rebuilt once more before executing tests.
- Serial limiter/linear-algebra/gradient/config suite: 23 cases, 28407 assertions, passed. Hessian reliability: 8 cases, 91431 assertions, passed. NEMO primitive-limiter output naming: 1 case, 26 assertions, passed.
- Actual OpenMP2 limiter/anisotropic-gradient regions: 6 cases, 27544 assertions, passed.
- MPI2 ownership/limiter scaling: 3 cases, 13573 assertions per rank, passed.
- Forward/reverse standalone AD: 12 QR field/geometry cases plus 2 scalar fallback cases each, passed. The infinite face returns the finite cell value and its derivative. Full-solver adjoints and local-volume limiter derivatives remain untested.
- Matched manufactured NS: all 12 runs finite, all six reported error vectors identical to the QR control. Regular n=12 stopped after 192 iterations; others hit 300. This establishes no demonstrated manufactured-solution accuracy gain.
- Real parallel CFD on the regular n=12 manufactured mesh, 20 steps, MGLEVEL0, linear tolerance1e-10: ILU MPI2/OMP1 vs MPI2/OMP2 max conserved-field scaled difference `1.8143e-15`; identity MPI1 vs MPI2 `1.6432e-15`. Both finite. The ILU case explicitly enables level scheduling.
- Valgrind on the MPI2/OMP2 ILU case: both ranks exit successfully, zero invalid-access errors. Leak checking and undefined-value checks were disabled to focus on the original halo-row memory bug; this is not a leak audit.
- Raw validation logs: `/tmp/su2-cfd-fixes-validation`; parallel case configs/results/logs: `/tmp/su2-cfd-fixes-parallel`.
- First RANS attempt `/tmp/su2-cfd-fixes-rans` rejected the retained `ADAP_BL_MARKER` with `COMPUTE_METRIC=NO`. It was stopped without touching other jobs. The runner now removes source `ADAP_*` options, disables adapted mesh output, and requests ordinary solution/primitive fields for fixed-mesh comparisons. Preserve the failed logs; they are configuration failures, not numerical outcomes.
- Validate all corrected cases with `--iterations 1` in a fresh smoke directory before the full campaign. The frozen test caps at the smaller of 200 and the requested iteration limit. Full RANS outcomes remain pending.
- All 20 corrected one-step configs passed in `/tmp/su2-cfd-fixes-rans-smoke`. Full run is `/tmp/su2-cfd-fixes-rans-v2`, log `/tmp/su2-cfd-fixes-validation/rans-runner-v2.log`, execution session `55208` (session IDs are only useful in this live tool session). It uses nice=19, OMP1, 2000 iterations / 600 seconds per case, with a separate 200-step freeze probe.
- At the first QR-control continuation, density residual passed -8 by iteration780 but SA residual remained near -7.1865. This is an early observation, not a final matched comparison; requiring both residuals matters.
- Validated local safeguards and tests are committed as `596b18a991`. The later documentation/results commits do not alter the candidate binary.
