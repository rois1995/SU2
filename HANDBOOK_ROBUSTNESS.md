# CFD robustness handover

Updated: 2026-10-09. This is the transferable record for the mesh-quality CFD work. Pending entries below are not measured results.

## Current night checkpoint (2026-10-09, 00:16 CEST)

- Latest pushed numerical commit: `489f2701fe`, branch `codex/cfd-mesh-robustness`; SA positive recovery and LINELET fixes are pushed. User explicitly requested continued overnight work and a push.
- Pushed, tested source corrections: bound-aware SST relaxation and model-consistent native Cartesian SA-neg diffusion. The full affected suite passes4036 assertions/10 cases; serial/OMP/MPI focused tests passed. Read the night sections below for measured effects/limits before accepting defaults.
- All7 solver/limiter controls finished. Frozen Venkat, flow Wang and long fixed-CFL3 reach selected density/SA monitor targets; Wang changes drag substantially. The coefficient1.0, R4 and tighter linear solve do not remove the CFL10 plateau.
- Main follow-up has8/9 completed: SST control/candidate, GG-all, LINELET, freeze/restart and positive FT2. Old negative-SA is running; model-consistent SA-neg and SST GG viscous follow afterwards. Freezing after100/500 reaches selected monitors, but live-limiter restart restores the CFL10 plateau. Flow limiter dimensional scaling is a concrete missing next target. Build is complete. Preserve immutable binaries and raw local evidence; do not rerun completed cases unnecessarily.
- Automatic review rejected raw trace uploads. Keep histories/full states/CSV traces/plots local. Push source and assessment notes; do not route rejected raw payload through another format.

## User requirements

- Improve CFD on stretched, mixed and adapted meshes, including flow/turbulence MUSCL, gradients, limiters and convergence.
- Start from `codex/native-unsteady-performance`; work separately. Implementation and testing are authorized.
- **Use `MGLEVEL=0` in every simulation.** Multigrid is outside this campaign.
- **Do not use an Euler mesh for RANS convergence assessment.** Check provenance and actual boundary-layer suitability before interpreting residuals. Earlier native frozen/adapted stress cases below are excluded. The user explicitly identified a different mesh with adapted BL geometry, recorded at the end of this handbook; do not reject that selected mesh just because its folder names its original Euler seed.
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

## Integrated source fixes

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

Additional local corrections committed in `596b18a991`:

- Compressible under-relaxation computes internal **energy density** using momentum squared divided by old/new density (PR2942). The old formula multiplied by density and can spuriously cancel updates.
- Scalar face fallback uses per-lane selection, including NaN/infinity rejection, instead of multiplication by a mask (`0*infinity` is still NaN).
- Legacy CNumerics boundary callers also pass donor/farfield density and sliding donor weight to the shared bounded correction; the new native scalar boundary path already did so.
- Relative limiter reference is an AD preaccumulation input and uses its absolute magnitude.
- `LIMITER_LOCAL_LENGTH=NO` default preserves existing global flow regularization. When enabled, Venkat/R3/R4/R5 use equivalent-circle diameter in 2D and equivalent-sphere diameter in 3D, with periodic dual volume included. No dimensional volume floor; zero/nonpositive volume uses zero length and the existing epsilon floor.
- Existing `VENKAT_LIMITER_COEFF` remains tunable. Local length is an experiment, not a universally better coefficient convention.

## Build and validation commands

Build configuration: release GCC9.4, MPI/OpenMP enabled, tests enabled, no CGNS/TecIO/MMG. Flags include `-O3 -march=native -ffast-math -fno-finite-math-only`.

During this integration, the one-worker build priority was lowered from nice=15 to nice=19 when the 8-CPU machine's load reached ~15. Lower only our Ninja process and its descendants; do not reprioritize unrelated workers. Wait for the build before starting comparison runs.

```sh
nice -n 15 /tmp/native-build-tools/ninja -C /tmp/su2-cfd-mesh-build -j1 SU2_CFD/src/SU2_CFD UnitTests/test_driver
OMP_NUM_THREADS=1 /tmp/su2-cfd-mesh-build/UnitTests/test_driver '[Limiters],[LinearAlgebra],[Gradients]'
python3 TestCases/gradient_robustness/run_mms.py /tmp/su2-cfd-fixes-control/SU2_CFD /tmp/su2-cfd-mesh-build/SU2_CFD/src/SU2_CFD /tmp/su2-cfd-fixes-mms
```

Prepend `/tmp/native-build-tools` to `PATH` when building: Meson regeneration otherwise finds an old Ninja and fails. MPI executables initialize local sockets even in serial, requiring tool escalation in this environment. Use MPI2/OMP2 only for focused correctness checks; all comparison cases are serial and `MGLEVEL=0`.

Build, focused unit/parallel/AD checks, matched MMS, both fixed-mesh RANS campaigns and the additional constant-field regression are complete as recorded below. Keep setup failures and iteration caps in the evidence.

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
- Runner: `TestCases/gradient_robustness/run_fixes.py QR_CONTROL CANDIDATE NATIVE_CONFIG BL_MESH OUTPUT --adapted-config /tmp/su2-cfd-fixes-inputs/adapted_rae/snapshot.cfg --main-baseline /tmp/su2-cfd-gradient-baseline/SU2_CFD`. Defaults to 2000 iterations, 600-second per-run cap, serial low priority, fresh output. It records failures/timeouts rather than concealing them. Completion and convergence are separate outcomes.
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
- Validate corrected cases with `--iterations 1` in a fresh smoke directory before a full campaign. The frozen test caps at the smaller of200 and the requested iteration limit. Both full campaigns subsequently completed; results are recorded below.
- All 20 corrected one-step configs passed in `/tmp/su2-cfd-fixes-rans-smoke`. Full run is `/tmp/su2-cfd-fixes-rans-v2`, log `/tmp/su2-cfd-fixes-validation/rans-runner-v2.log`, execution session `55208` (session IDs are only useful in this live tool session). It uses nice=19, OMP1, 2000 iterations / 600 seconds per case, with a separate 200-step freeze probe.
- On the excluded first QR-control stress continuation, density residual passed-8 by iteration780 but SA remained near-7.1865. This does not establish RANS convergence on an appropriate grid; retain it only as stress-response evidence.
- Validated local safeguards and tests are committed as `596b18a991`. The later documentation/results commits do not alter the candidate binary.
- First matched 2000-step input-mesh/first-order-SA pair: QR control rho=-9.8676, nu=-7.1865, CL=0.7243226, CD=0.01321485; fixes rho=-7.9330, nu=-7.1862, CL=0.7145253, CD=0.01306713. Neither satisfies both residual targets. This pair does not demonstrate improved convergence; the initial restart is close to the old operator's steady solution. Force changes are not proof of accuracy.
- Both completed first-order states are finite with positive density/internal energy. Each has 859 nu values <=1e-15: 852 wall vertices plus the same 7 interior vertices (global IDs 2779,3621,3829,9728,14594,15006,18417). Clipping is a diagnostic lead, not yet a proven explanation of the residual plateau. ASCII coordinates differ from source by <=7.11e-15 due to printed precision; use a rounding tolerance rather than bitwise equality.
- The actual-kernel SA/SST constant-field residual/Jacobian regression in `limiters.cpp` sets both conservative and primitive density consistently, injects mixed-sign edge mass fluxes, and tests bounded transport independently of model sources. It subsequently passed serial/OpenMP/MPI checks recorded below.
- SST edge-limiter capability configs in `/tmp/su2-cfd-fixes-edge/current` and `fixed` use `SCALAR_UPWIND`, flow+turbulence `VAN_ALBADA_EDGE`,20 steps,MGLEVEL0. This exercises scalar carrying-velocity limiting, which the main bounded-scalar campaign bypasses. The control rejected the limiter; the candidate passed, as recorded below.

### Euler-origin stress checks: excluded from RANS convergence assessment

**User correction: do not consider an Euler mesh for RANS convergence.** The source configs explicitly state "User-supplied Euler-type mesh, no initial BL". All twelve input/adapted variants completed 2000 iterations without reported nonfinite residuals, but these cases are excluded from RANS convergence/accuracy conclusions. Adapting the seed and passing a first-height geometry check alone do not establish adequate RANS boundary-layer resolution. Retain the raw data as numerical stress evidence. The eligible convergence comparison uses `mesh_RAE2822_turb.su2`; identify a validated adapted RANS mesh separately.

| Mesh / turbulence | Variant | log10 density RMS | log10 SA RMS | CL | CD |
| --- | --- | ---: | ---: | ---: | ---: |
| Input / first order | QR control | -9.8676 | -7.1865 | 0.7243226 | 0.01321485 |
| Input / first order | Fixes | -7.9330 | -7.1862 | 0.7145253 | 0.01306713 |
| Input / MUSCL | QR control | -7.7561 | -2.9335 | 0.7367589 | 0.01360144 |
| Input / MUSCL | Fixes | -8.6220 | -7.1243 | 0.7262033 | 0.01336738 |
| Input / MUSCL | Fixes + local h | -8.6225 | -7.1243 | 0.7261996 | 0.01336733 |
| Adapted / first order | Original main and QR control | -8.3966 | -7.5306 | 0.7242120 | 0.01381114 |
| Adapted / first order | Fixes | -8.0956 | -7.5241 | 0.7142410 | 0.01365352 |
| Adapted / MUSCL | Original main and QR control | -7.5402 | -2.8936 | 0.7360092 | 0.01417848 |
| Adapted / MUSCL | Fixes | -8.2521 | -7.4778 | 0.7253010 | 0.01394685 |
| Adapted / MUSCL | Fixes + local h | -8.2523 | -7.4777 | 0.7252970 | 0.01394675 |

- The raw turbulence-MUSCL residual response at iteration1999 changes by 4.1909 orders in SA and0.8659 in density on the input, and4.5841 and0.7120 on the adapted stress mesh. This is **not a RANS convergence result**. It reflects the combined fixes, without isolating a single commit.
- Force shifts of roughly1.4% in lift on these excluded grids are not accuracy evidence; no experimental pressure/shear comparison or mesh convergence was performed.
- Local length provides negligible additional stress-response change with coefficient0.05 and JST flow. Keep `LIMITER_LOCAL_LENGTH=NO` as default; the separate Roe boundary-layer cases determine transferability.
- Adapted outputs are finite with positive density and internal energy. The wall has522 vertices. With turbulence MUSCL, both controls clip168 interior SA values at <=1e-15; fixes/local fixes clip6. First-order variants clip the same2 interior vertices. Floor counts include wall values only when explicitly stated.
- The SA under-relaxation helper uses `abs(delta/(phi+EPS))` and cancels updates when relaxation<1e-10. This could trap a positive update from an EPS-level state; an alternative is genuinely negative updates from the stretched-grid discretization. Inspect actual linear updates, relaxation and raw residuals at clipped interior vertices before changing positivity handling. Floor counts alone do not prove either explanation.
- `summarize_fixes.py` archives exact configs, checks matched settings/hashes, compares at the last common iteration (important for timeouts), reports finite/admissible fields, and plots **only the eligible boundary-layer cases**. Every record/pair explicitly marks convergence eligibility; numerical residual-target satisfaction is separate. It requires the expected number of variants; its parser self-check and both completed report generations passed.

## Completed conventional RANS boundary-layer comparison

All20 initial campaign runs finished with finite residuals; only the six `bl_sa_*`/`bl_sst_*` variants are eligible RANS convergence cases. All six states are finite with positive density/internal energy. None meets every selected residual target at iteration1999. The conventional mesh has13937 points,18042 triangles and4800 quadrilaterals; this is a mixed-grid test, including the original BL cells.

| Model | Variant | Density log RMS, last100 median | Turbulence log RMS, last100 median | Final CL | Final CD |
| --- | --- | ---: | --- | ---: | ---: |
| SA | QR control | -6.74385 | nu=-3.58807 | 0.7463846 | 0.01434562 |
| SA | Fixes | -6.83787 | nu=-8.52650 | 0.7364345 | 0.01419346 |
| SA | Fixes + local h | -6.71052 | nu=-8.59153 | 0.7364891 | 0.01419374 |
| SST | QR control | -5.04111 | k=-3.46586; omega=3.82775 | 0.7213099 | 0.01366365 |
| SST | Fixes | -4.91987 | k=-3.40780; omega=3.82821 | 0.7213185 | 0.01366426 |
| SST | Fixes + local h | -4.92696 | k=-3.41304; omega=3.82908 | 0.7213271 | 0.01366335 |

- The combined fixes strongly reduce SA residuals (~4.94 orders by tail median), without establishing joint convergence or accuracy. Density oscillates with adaptive CFL; final values alone are misleading. SST has no material demonstrated turbulence convergence benefit in this setup. Local h does not justify a default change.
- Exact configs, hashes, last-common-iteration changes, tail ranges, residual gates and final-state admissibility: `TestCases/gradient_robustness/fixes_results_20261008.json`. Figures with the same stem show **only the eligible conventional RANS cases**. Reproduce: `python3 TestCases/gradient_robustness/summarize_fixes.py /tmp/su2-cfd-fixes-rans-v2 TestCases/gradient_robustness/fixes_results_20261008.json`.
- Supplementary unit/MMS/parallel/AD/mesh evidence: `TestCases/gradient_robustness/fixes_checks_20261008.json`. Logs remain in `/tmp/su2-cfd-fixes-validation`.
- The new actual-kernel SA/SST constant-field residual/Jacobian test passed: serial92 assertions; OpenMP2 92; MPI2 44/50 by rank. The affected limiter suite passed5 cases3598 assertions. Initial test-fixture issues were a two-argument primitive-index constructor and omitted positive Reynolds number; corrected before these passes. No CFD source/binary change occurred during the campaign.
- SST `SCALAR_UPWIND` with flow+turbulence `VAN_ALBADA_EDGE`: QR control rejects "Unknown limiter type"; candidate completes20 steps with finite residuals and fields. This is capability/correctness evidence, not convergence. Exact configs/results in the checks JSON and `/tmp/su2-cfd-fixes-edge`.
- The 200-step freeze test completed for both binaries; it is numerical stress evidence on the excluded earlier mesh. The changes of its bundled comparison do not isolate the frozen-limiter fix from relative epsilon changes.

## Completed user-selected adapted RANS comparison

The user supplied this exact mesh after excluding Euler grids from convergence assessment:

`/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated/ClusterResults/cases/581712_actual_euler_to_bl_n4_m4_pYES_r1/mesh_00400.su2`

- **Use the adapted mesh00400, not that case's unadapted Euler input.** It has13024 points,25576 triangles and432 AIRFOIL edges. Independent audit: positive volumes/conformity/no unused or duplicate points/elements; metric qmin0.228803; requested first height1e-5, max relative height error3.03e-12; original airfoil features retained. These are geometry checks, not convergence certification.
- Mesh SHA256 `4b805909cce30f825e8f01580b21bd06255944dd45905120ec9e959202322326`; physical longest-edge/altitude max5624.95, p991199.05.
- Snapshot `/tmp/su2-cfd-fixes-inputs/cluster_adapted_rae` includes mesh/native_ref, `solution_00599.dat`, source config and independent audit. The restart has exactly13024 points and bitwise-equal coordinates, finite fields, positive density(min0.2972315). Use latest00599 with mesh00400; earlier00399 is from the prior mesh. Source files stay untouched; hashes and selected source paths are in the snapshot manifest and checks JSON.
- Source run is dual-time2nd-order, physicaldt1e-4,600 steps,50 inner iterations, CFL10 fixed, JST flow/first-order bounded-SA transport. **The new tests are steady fixed-mesh continuations** from the same latest state, not a reproduction of unsteady evolution or physical-time convergence. Convert `TIME_DOMAIN=NO`, `TIME_MARCHING=NO` ("STEADY" is invalid SU2 syntax), and explicitly override both `ITER` and `INNER_ITER`.
- `run_fixes.py --adapted-only` runs7 variants: existing first-order turbulence(main/QR/fixes), then turbulence MUSCL(main/QR/fixes/fixes+local h), coefficient0.05, limiter updates ongoing, MGLEVEL0, source CFL10 fixed. Flow remains JST. It excludes the earlier Euler stress/conventional case reruns.
- Initial cluster smoke rejected `TIME_MARCHING=STEADY`; corrected smoke-v2 finished50 iterations because source `INNER_ITER=50` overrode `ITER=1`; smoke-v3 passed all7 configs with exactly1 iteration. A mistakenly started invalid long-config attempt exited on parsing, with no numerical results. Preserve these setup logs; they are not CFD failures.
- Completed campaign: `/tmp/su2-cfd-fixes-cluster-rans-v2`; runner log `/tmp/su2-cfd-fixes-validation/cluster-rans-v2.log`. Seven serial nice19/OMP1 variants,2000 iteration/600s caps. Candidate SHA256 remains `5b77b9acfedaf24d7ff973aff798987edf24ca9575fc480110ccc342363e58d4`.
- All7 CFD child processes exited0 with2000 history rows and final fields; each solver log ends "Exit Success". The tool wrapper returned143 after these complete results were written; its cause is unknown. Numerical completion is independently checked; this wrapper status is retained in the checks JSON rather than silently discarded.
- Archived report: `TestCases/gradient_robustness/adapted_rans_results_20261008.json`, with density/turbulence PNG/SVG figures. Reproduce: `MPLCONFIGDIR=/tmp/su2-cfd-matplotlib python3 TestCases/gradient_robustness/summarize_fixes.py /tmp/su2-cfd-fixes-cluster-rans-v2 TestCases/gradient_robustness/adapted_rans_results_20261008.json --expected-runs 7`.

| Turbulence transport | Variant | Final density log RMS | Final SA log RMS | Final CL | Final CD | Interior SA values at floor |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| First order | Original main / QR control | -5.92704 | -5.57172 | 0.7284840 | 0.01650523 | 12 |
| First order | Fixes | -6.04761 | -5.57318 | 0.7200104 | 0.01639652 | 12 |
| MUSCL | Original main / QR control | -5.80458 | -3.16160 | 0.7401102 | 0.01683151 | 122 |
| MUSCL | Fixes | -5.91715 | -5.56389 | 0.7307131 | 0.01664949 | 13 |
| MUSCL | Fixes + local h | -5.91716 | -5.56373 | 0.7307098 | 0.01664940 | 13 |

- Every final state is finite, with positive density/internal energy; **none meets both residual targets**. The original main and QR control have identical reported histories/forces on both configurations. Do not attribute a CFD convergence gain to QR here.
- MUSCL combined-fix SA reduction at matched iteration1999 is2.4023 orders (~252 times); density reduction0.1126 orders (~1.30 times). Tail medians confirm the same interpretation: SA control-3.16167 vs fixes-5.56381; density-5.78402 vs-5.89942. Source CFL10 is held fixed for all variants.
- First-order transport gives essentially no SA residual improvement and modest density reduction. Lift/drag changes are not evidence of better accuracy on these unconverged continuations. Local length makes negligible difference and stays off by default.
- The AIRFOIL has432 wall vertices at the SA floor in all variants; counts in the table exclude them. `fixes_checks_20261008.json` stores IDs and coordinates of every clipped interior point for subsequent diagnostics. Geometry checks and positivity do not certify wall shear or heat accuracy.

## Next steps and remaining limits

1. On the **selected adapted RANS mesh**, inspect raw SA linear updates, under-relaxation and residual terms at the13 clipped interior vertices. Distinguish positive updates canceled by the relaxation threshold from genuinely negative updates caused by diffusion/gradient/source discretization. Do not change the positivity rule based only on floor counts. Check the12-point first-order plateau separately.
2. Investigate SST's omega plateau and the flow residuals on the conventional BL mesh. Separate boundary/viscous-gradient errors from linear-solver/CFL behavior using controlled changes to one setting at a time. A line-based preconditioner can be evaluated independently; **no multigrid**.
3. Assess accuracy using converged manufactured solutions on mixed/stretched grids and consistent interior/boundary flux quadrature checks before importing Bellosta flux/centroid ideas. Match physical model/settings when checking airfoil pressure, shear and integrated forces. Current force shifts are not an accuracy result.
4. Extend validation to full solver adjoints/local-volume limiter derivatives, periodic interfaces, moving grids and unsteady CFD when those paths are needed. Standalone AD/short parallel tests do not cover those workflows. Physical-model draft patches remain deferred.

Source fixes are complete for this comparison phase; unresolved plateaus are recorded, not declared solved. Preserve controls and snapshots, use fresh output directories, and compare equal iterations when limits differ. No automatic coefficient retuning or physical-model substitution is justified by this evidence.

## Night continuation, 2026-10-08 (work in progress)

User authorized continued implementation/assessment overnight and explicitly requested a push to the fork. Work remains on `codex/cfd-mesh-robustness`; MGLEVEL=0, RANS boundary-layer meshes only. Current controls are immutable copies of the previous accepted binary at `/tmp/su2-cfd-night-control/SU2_CFD`. Controlled campaigns are serial/nice19/OMP1; the LINELET rebuild uses one nice19 compile worker. No speed claims on this contended host.

### Confirmed SA floor mechanism and correction

- A 200-step continuation from the selected adapted mesh's previous 2000-step fixed/MUSCL solution traced the actual linear update and relaxation at all13 interior floor points. Geometry global IDs must be mapped to the solver's reordered internal indices. An initial diagnostic used the wrong indices and was discarded; the archived trace uses the corrected mapping.
- Global IDs2108,3717,8094 have **positive** linear updates at every traced iteration but relaxation=0: the shared helper cancels its positive relative-limited step when alpha<1e-10. The other10 have negative updates (six canceled, four tiny accepted steps clipped back to the floor). Lowering/turning off clipping globally would conflate these mechanisms.
- Correction: retain tiny **positive, single-variable SA** steps. Keep the existing MAX_UPDATE_SA relative-size limit, negative-step guard and multi-variable SST/stochastic behavior. SA_NEG bypass is unchanged. This changes neither spatial residuals nor model constants.
- Focused actual-solver tests passed: serial/OMP2 each465 assertions in2 cases; MPI2 passed417/48 assertions per rank. The regression checks positive floor recovery, the original relative-change bound and negative/zero updates.
- A300-step candidate continuation gives SA log RMS=-6.5198419 and density=-6.0409071; all three artificially frozen points recover, and two other points recover through coupling. Interior floor count13 ->8. This short diagnostic is encouraging, **not yet the matched3000-step campaign or full convergence/accuracy evidence**.

### Remaining negative updates: direct flux evidence

The native SA interior loop fuses convection and diffusion; the separate Viscous_Residual pass is empty. Do not label the fused pass as convection. A read-only diagnostic evaluated the actual flux kernel with convection/diffusion flags separately and compared their sum with the assembled residual (maximum mismatch1.63e-19). At initially clipped points receiving negative updates, inward convective transport is outweighed by outward corrected diffusion. The two-point diffusion contribution points inward; the nonorthogonal gradient contribution reverses its sign. Production/destruction is negligible at those tiny SA values. This directly identifies the discrete transport mechanism; it does not justify removing all gradient correction or changing model constants.

Local archive: `TestCases/gradient_robustness/night_diagnostics_20261008/` contains globally mapped updates, separated fluxes, point summaries and temporary diagnostic patches. Raw CSV traces remain local/ignored: automatic approval review rejected their upload as potentially sensitive mesh-derived data. Source fixes, diagnostics and assessment notes are pushed; do not publish the raw payload through another format without explicit authorization. SA instrumentation has been removed from production source. SST tracing was also removed after copying its diagnostic binary; `/tmp/su2-cfd-night-evidence/CTurbSolver.recovery.cpp` retains the accepted SA-only helper as a control. The exact input mesh/restart hashes are retained in the earlier adapted-RANS manifest. The two diagnostic continuations share the same original2000-step restart, but run lengths differ200/300; use the new matched campaign for gain comparisons.

Allmaras, Johnson & Spalart2012 discuss negative continuation for under-resolved grids/unphysical transients: <https://www.iccfd.org/iccfd7/assets/pdf/papers/ICCFD7-1902_paper.pdf>. The current official model resource is <https://tmbwg.github.io/turbmodels/spalart.html> (the old NASA URL redirects). SA_NEG is a possible separately labeled diagnostic, not an automatic model substitution. Existing nonorthogonal diffusion consistency/accuracy must be assessed before an ad hoc positivity clamp.

### Saw-tooth controls and next experiments

`run_convergence.py` takes an explicit JSON experiment plan, checks MGLEVEL0/steady fixed-mesh settings, hashes binary/config/mesh/restart and runs fresh directories. `assess_convergence.py` records tail100 ranges/medians, jumps, CFL reductions, positive density/internal energy and wall-excluded SA floor IDs; plots density/turbulence/CFL histories.

- Conventional SA: paired3000-step continuations from the same previous2000-step state. Adaptive CFL1–30 gives396 density rises>0.2 decades and a late500-step span2.84 decades. Fixed CFL3 gives one startup rise and a late span0.076 decades; final density=-7.381877052, SA=-9.220438843. Thus the large saw-tooth is associated with aggressive CFL ramping, and a conservative fixed CFL removes its large excursions. Density remains above our target.
- Fixed CFL10, SST fixed-CFL controls, matched adapted SA recovery/accurate-Jacobian/Green–Gauss experiments and stronger solver trials are pending. Compare one changed setting at a time. NUM_METHOD_GRAD changes flow and turbulence viscous gradients; reconstruction remains WLS, so do not attribute that variant solely to turbulence.
- The templates use **dimensional** simulations. Raw density, energy and SST omega residual magnitudes have different units/scales. A common raw=-8 threshold is only an operational monitor, not an accuracy certificate or a meaningful cross-equation comparison. Record reductions, oscillations and solution changes too; avoid declaring an SST plateau solved by changing normalization.
- LINELET repair is being built/reviewed from exact upstream PR2899 head `ef2282942a242b918d1c6f2af58000ddea5d08a6`: collective cache flag, valid work arrays on ranks without lines, zero-global-line guard and MPI_UNSIGNED_LONG color count. A dedicated real-matrix test covers no-line and mixed-rank distributions with repeated application. Parallel correctness and RANS benefits remain pending; upstream MG regression settings are not imported.

Raw work is under `/tmp/su2-cfd-night-evidence`, CFL controls under `/tmp/su2-cfd-night-cfl-controls`, planned adapted campaign under `/tmp/su2-cfd-night-recovery`. Preserve completed results; rerun only failed/changed/necessary variants. Before the next report, archive outcomes, update this section's pending statuses and verify the remote branch matches the pushed commit.

### Additional controls completed (night)

- Conventional fixed-CFL10 SA plateaus at density=-5.026812624 after3000 continuation steps, despite a tight achieved BiCGSTAB residual. Its lift/drag are stable; a stable force history alone does not establish equation convergence. Fixed-CFL3 retains gradual density reduction and substantially smaller residual excursions.
- Conventional SST fixed-CFL3 reaches density=-8.012701889, k=-5.222876273, omega=+3.831553227. Fixed-CFL10 reaches density=-5.895770305, k=-4.615183247, omega=+3.832571746. The omega plateau survives CFL reduction.
- A20-step fixed-CFL3 continuation with native residual output confirms **99.9900% of omega residual squared is at global point4992**, immediately outside the upper trailing-edge corner. It is also the only interior k-floor point (192 wall vertices excluded). The residual field is a linear-system RHS (negative assembled residual); retain the sign convention when interpreting updates. Coupled SST relaxation at that point is being traced; do not yet call it a confirmed cause.
- Long solver/limiter trials are queued: ordinary/tighter FGMRES, limiter freeze after100 warm-start steps, flow R4, flow Wang, coefficient1.0 (changes both flow/turbulence regularization), and a10000-step fixed-CFL3 continuation. These use the same conventional2000-step restart. The matched adapted campaign compares unchanged control, positive-SA recovery, accurate turbulence Jacobians, and Green–Gauss viscous gradients. Reconstruction remains WLS.
- The runner now waits between cases if the1-minute load exceeds available CPU affinity, then runs one nice19/OMP1 CFD child. Waiting does not consume a case's timeout. Only one compile worker is used; check actual host load/memory periodically. Full raw fields/history traces and plots remain local following the upload rejection; assessment notes/code can be pushed separately.

### Matched adapted recovery result and literature limits

- Completed3000-step control/candidate continuations from exactly the same earlier2000-step state: control density=-6.886124359, SA=-5.562222751; positive-SA recovery density=-6.888031585, SA=-6.923324991. Thus SA improves1.3611 orders (~23times), density0.00191 orders (negligible). Both states remain finite with positive density/internal energy. Interior floors13 ->8, confirming the shorter diagnostic without declaring full convergence. CL changes0.7334550078 ->0.7333431911 and CD0.01665919681 ->0.01667311268; these are unconverged response changes, not an accuracy gain.
- Reviewed local `Papers/20090007493.pdf`: Diskin & Thomas, *Accuracy of Gradient Reconstruction on Grids with High Aspect Ratio*, NIA Report2008-12. Stable QR addresses arithmetic conditioning; Cartesian linear-fit truncation still depends on grid/solution alignment and curvature. Higher-order terms in the direction of larger spacing and approximate mapping based on wall distance are proposed. The mapping requires accurate distance/normal geometry; flat-panel wall distances can introduce normal-gradient errors on stretched curved grids, especially mixed meshes. Do not feed the existing polygonal distance into this method without testing that error.
- Reviewed local `Papers/20120001451.pdf`: Diskin & Thomas, *Effects of mesh irregularities on accuracy of finite-volume discretization schemes*. The evaluated edge formulation uses quadratic fits and average least-squares viscous gradients with face-tangent augmentation. It separates gradient, truncation and solution error. Its results do not establish that replacing SU2 WLS with ordinary Green–Gauss, or importing Bellosta formulas without consistency/MMS checks, improves our RANS accuracy.

### Setup constraint discovered in the gradient trial

The proposed JST/Green–Gauss viscous-gradient trial with WLS reconstruction exits on configuration parsing, before any history exists: CConfig rejects a separate reconstruction gradient with centered flow. This is a setup failure, not CFD divergence. The original failed directory/log is retained. A corrected `adapted_floor_gg_all` plan switches **both** gradient options to Green–Gauss; do not interpret it as a viscous-only or turbulence-only experiment. Conventional Roe trials can still use separate gradient methods. The accurate-turbulence-Jacobian adapted trial completed3000 steps with density=-6.888037854, SA=-6.918102632, essentially unchanged from the SA-recovery candidate.

A local analytic curved bow-stencil probe (`/tmp/su2-cfd-night-evidence/probe_bow_stencil.py`) uses four points at angles±0.05 and radii1±0.05/aspect. Column-scaled NumPy least-squares exactly reproduces Cartesian affine data, but for a radial-linear field its normal-gradient relative error is0.8620 at aspect100,0.999984 at10000,0.9999999984 at1000000. Exact radial mapping reduces that error below4e-16. This is a synthetic stencil demonstration of truncation error, **not SU2 implementation/PDE validation**. A separate structured curved-layer probe did not show universal gains from mapping or a quadratic two-ring fit. Grid/solution/stencil dependence matters; no mapped-gradient production change is justified yet.

### Fixed-CFL10 flow plateau localization

A20-step SA continuation from the completed conventional fixed-CFL10 state with native residual output gives99.9187% of density residual squared at just10 global points. The four largest points0,1,2,3 lie immediately below the trailing edge and contribute99.4682%; they are interior points, not wall vertices. This is a localized flow plateau, not a mesh-wide turbulence-floor issue (conventional SA has no interior floors). Ordinary FGMRES gives density=-5.027616267, effectively the same as BiCGSTAB=-5.026812624 at matched3000 steps. Tight FGMRES and frozen/alternative limiters remain pending.

Geometry correction: conventional AIRFOIL wall point4991 joins wall points4965 and25; both surface branches meet at that one point. The conventional mesh therefore has a **sharp trailing edge**, not a finite blunt segment. An earlier informal interpretation of its nonzero y-coordinate as bluntness was incorrect. Do not infer geometry from a single coordinate. The SST hotspot4992 is immediately outside the upper side of this sharp corner; the large SA flow hotspots are below it. Local residual fields/summaries stay under `/tmp/su2-cfd-night-evidence` and their diagnostic case directories.

### Confirmed SST relaxation mechanism; LINELET correctness passed

- The100-step SST trace at global4992 confirms k remains at1e-10 and receives a conservative update about-1.27e-4. It sets common alpha≈3.5335e-7. Omega's conservative update≈-339009 is a feasible physical change≈-755518 from omega≈45064911, but the shared factor admits only≈-0.267 per iteration. Clipping then discards the negative k change. This directly explains the omega freeze; it does not eliminate the nonzero k residual at its bound.
- A minimal candidate excludes conservative turbulence directions already at a lower bound with a negative update from the common relative relaxation limit. If every component is thus bound-active, alpha stays0. Positive bound-recovery steps and negative steps above their bounds retain the existing relative restriction. Only SST calls this helper with conservative variables; SA/transition handling remains unchanged. Actual CompleteImplicitIteration tests cover k-active, omega-active, both-active, near-bound and positive k recovery, including density conversion. Build/tests and matched SST CFD assessment are pending; **not yet an accepted improvement**. Production tracing has been removed.
- LINELET tests now pass serial40 assertions/2 cases, OMP2 40/2, MPI2 24/29 assertions per rank/2 cases. The second regression includes a truly empty point partition on one rank and repeated cached construction. The first serial attempt crashed because the new fixture requested FEM/deformation matrix connectivity, which allocates a different preconditioner; corrected to real FVM connectivity before these passes. Preserve the failed setup log as fixture evidence, not a production solver failure. RANS convergence impact and serial/MPI full-solver smoke remain pending.

### Candidate and limiter validation updates

- SST candidate actual-update tests passed serial/OMP2 735 assertions/3 cases and MPI2 543/192 assertions by rank/3 cases. The complete affected limiter/LINELET/bounded-transport suite passed3998 assertions/9 cases. The initial SST test compilation used the wrong two-argument primitive-index wrapper constructor; corrected to its existing four-argument API before these passes.
- A matched100-step SST continuation from the completed fixed-CFL3 control gives omega log RMS3.831567423 ->0.7580911114 (3.0735 orders), density-8.028933894 ->-8.028562917 (negligible), k-5.251613648 ->-4.460768169 (temporarily worse). Hotspot omega falls≈45.0649million ->39.8363million; k stays at1e-10. Thus the shared-update stall is corrected, but joint SST convergence is not established. Matched3000-step control/candidate runs are queued. Both100-step fields are finite.
- LINELET full-solver smoke passed10 steps serial and MPI2 on the user-selected adapted RANS mesh with MGLEVEL0, finite histories and positive density/internal energy. Parallel/no-line fixes are pushed in `a1d9a93eb9`; RANS convergence impact remains queued.
- At conventional CFL10, limiter freeze after100 warm-start steps reaches configured density/SA monitors in2162 steps: density=-8.000399815, SA=-10.01167651, finite/admissible fields. At common iteration2161 it improves density2.9736 orders and SA1.2387 against continuously updated limiters. This is the strongest flow-monitor improvement so far, but not an accuracy proof or consistency check with recomputed limiters. Freeze-after500 and a1000-step unfrozen restart are queued. The experiment freezes both flow/turbulence point limiters via the shared LIMITER_ITER setting.
- Tight FGMRES does not improve the CFL10 plateau (density=-5.027616294, virtually identical to ordinary FGMRES). Flow R4 also does not remove it (density=-4.942421512, SA=-8.507209605). Do not retune defaults based on these runs. The coefficient1.0 and flow-Wang cases remain pending.
- Follow-up plan `/tmp/su2-cfd-night-evidence/followup_plan.json` waits for all7 original solver/limiter cases, then runs SST control/candidate, corrected GG-all, adapted LINELET10, conventional LINELET3, freeze500 and unfrozen restart, one long CFD child at a time. Logs `/tmp/su2-cfd-night-evidence/followup_runner.log`; campaign `/tmp/su2-cfd-night-followup`. Immutable SST candidate `/tmp/su2-cfd-night-evidence/SU2_CFD_sst_bound`. The source correction is now pushed in `489f2701fe`; the longer assessment remains pending.

### Wang result: convergence gain with a material solution change

Flow VENKATAKRISHNAN_WANG (turbulence limiter unchanged) reaches the configured density/SA monitor thresholds at2313 steps, CFL10, without freezing: density=-8.000211841, SA=-9.567436095. Fields are finite/admissible and there are no interior SA floors. However CL=0.7312201066 and CD=0.01171804470, versus frozen-Venkat CL=0.7365377330 and CD=0.01419518257: drag differs about-17.45%. This is a material change in the spatial solution, not evidence of improved accuracy. Keep Wang as an assessed alternative; do not adopt it as default based on residual reduction alone. Pressure/shear, mesh refinement and matching physical reference data remain needed. Frozen Venkat leaves forces much closer to continuously updated low-CFL Venkat, but its final limiter consistency is still being checked.

Reporting note: `assess_convergence.py` groups by exact mesh/restart/model and currently uses the first run within that group as its reference. Inspect `changed_options`; its automatic comparisons can contain more than one changed setting. Use the fixed-CFL10 Venkat control for isolated solver/limiter changes and compare exact common iteration. Failed parser cases with no history are now retained correctly in the report, verified using the rejected initial GG setup. Result snapshots for future runs are replaced atomically so readers cannot see a partly written JSON file.

### Next gradient prototype idea (analytic test only)

On the same synthetic curved bow stencil, augment the Cartesian affine fit with one zero-gradient curvature mode `q_j = (d_j-d_i) - n_i dot (x_j-x_i)`, where d is the **exact smooth** wall distance and n its exact unit gradient at i. Fit columns(dx,dy,q), return the first two coefficients. The q derivative at i is zero, so a full-rank fit reproduces both Cartesian affine data and this radial-linear example. A column-scaled NumPy test gives radial-gradient relative errors2.2e-16,4.9e-14,3.5e-12 at aspect100,10000,1000000; affine errors1.2e-14,1.9e-12,2.1e-10. Scaled condition still rises5.2 ->500 ->50000; QR/rank checks remain necessary. This is an inference/prototype, not an imported paper formula or a solver implementation.

The mode is unusable without qualification on flat walls (q≈0/rank loss), distance discontinuities, sharp corners or inaccurate polygonal distance normals. Test rank fallback, curved smooth-wall and boundary manufactured solutions, and conservation/face-gradient consistency before touching production gradients. The local structured-layer probe did not give universal benefits, and the adapted floor diagnosis does not yet isolate curvature error from nonorthogonal flux correction. Do not assert this mode solves the current negative SA updates. Local numeric toy result `/tmp/su2-cfd-night-evidence/curvature_mode_probe.json`.

### Separately labeled negative-SA assessment

Two more adapted-mesh diagnostics are queued at the end of followup_plan: `adapted_sa_ft2` with SA_OPTIONS=WITHFT2 and `adapted_sa_neg_ft2` with SA_OPTIONS=(NEGATIVE,WITHFT2). The current SA-noft2 setup remains unchanged. SU2's ParseSAOptions requires FT2 for its standard SA-neg option; switching straight from NONE to NEGATIVE would both fail validation and confound the ft2/model change. Compare the two FT2 runs to isolate negative continuation, then compare positive FT2 against the existing positive/noft2 candidate. These are explicitly **different physical-model diagnostic variants**, not implementation fixes or automatic recommendations. Negative nu_tilde counts in SA-neg are not "positivity clipping"; assess finite density/internal energy, nonnegative eddy viscosity and residual behavior separately. Do not use these model changes to claim improved accuracy or to conceal the unresolved nonorthogonal diffusion issue.

### Negative-SA native diffusion consistency correction

The local negative-SA path was also audited rather than assumed correct. Its previous fn arguments modify the whole row coefficient, including the part representing cb2*|grad(nu_tilde)|^2. ICCFD7-1902 Eq.14 leaves that gradient-squared term unchanged and modifies only the shared diffusivity `nu + nu_tilde*fn`. For nu=1 and nu_tilde=-2+x, the published nonlinear diffusion operator is-0.567, whereas the old actual coefficient kernel tends to-2.433 as h shrinks. Planar source assembly contains no compensating gradient-squared correction. The failing actual-kernel regression is retained at `/tmp/su2-cfd-night-evidence/sa_neg_consistency_before.log`.

Correction: retain the existing positive-SA row expression and add the same `nu_tilde_average*(fn(nu_tilde_average/nu_average)-1)/sigma` to both rows only for SA-neg. This restores the continuum operator while leaving cb2 unchanged. Individual row coefficients need not all be positive on mixed-sign edges; the pair's energy contribution must dissipate. The regression tests nonlinear refinement and30 mixed-sign energy pairs. It passes38 assertions serial/OMP2 and38 per MPI rank; the complete affected suite passes4036 assertions/10 cases. Native scalar dispatch uses su2double (not SIMD lanes), verified in DispatchEqnCount.

A100-step corrected-SA-neg/FT2 adapted-RANS smoke passes with finite fields, positive density/internal energy, nonnegative eddy viscosity and11 negative working-variable values (not clipping). Final density=-5.901403634, SA=-6.439697788. This is short capability/model evidence, not joint convergence or accuracy. A paired10-step positive-SA control has identical printed histories; fields differ only at roundoff (max abs/(1+abs(control))1.20e-13), so the positive branch is numerically preserved. No physical-model substitution/default was made. Full negative-SA/FT2 comparisons remain queued.

The follow-up plan now contains9 cases (corrected model binary was still building when the long queue started). The corrected negative-SA case is separately queued in `/tmp/su2-cfd-night-evidence/sa_neg_corrected_plan.json`, campaign `/tmp/su2-cfd-night-sa-neg-corrected`, runner log `sa_neg_corrected_runner.log`, after all9 follow-up cases. Its immutable binary is `/tmp/su2-cfd-night-evidence/SU2_CFD_sa_neg_consistent`. The old negative-SA code remains an explicitly labeled control; do not describe its results as implementing the published PDE correctly. Assessment grouping now includes SA/SST option strings so different model choices are not silently compared as the same physical model. Negative working-variable values are counted separately from positive-SA floor bands.

### Long fixed-CFL3 control completed

Conventional SA, continuously updated Venkat limiters: the10000-step cap terminates on the configured density/SA monitors after7197 continuation steps (same original2000-step restart), density=-8.000024726, SA=-9.989636962. This gives a smoother, slower alternative to freezing at CFL10. The coefficient1.0 run completed3000 steps with density=-5.036685599, SA=-8.774734900, so simply raising that coefficient does not remove the CFL10 plateau. Freeze and Wang stop earlier on the same monitor criterion; compare common iteration and physical model, not unequal final-iteration reductions.

Coverage limit for the new SA-neg correction: the native edge operator is corrected and tested on Cartesian/planar cases. The separate axisymmetric source in CSourceBase_TurbSA::ResidualAxisymmetricDiffusion was inspected and already uses nu+nu_tilde*fn for negative SA, consistent with the published coefficient. No current RAE test exercises axisymmetric CFD; do not claim that workflow is fully validated. Whole-solver adjoint, moving-grid and unsteady validation remain outside this fixed-mesh phase.

### Completed matched3000-step SST comparison

Same fixed-CFL3 restart/model/settings: old shared relaxation gives density=-8.508574094, k=-5.597206834, omega=+3.831781004; bound-aware SST gives density=-8.506788512, k=-5.600710310, omega=-0.8108856556. Omega improves4.6427 orders (~43900times), while density/k are effectively unchanged by these raw monitors. CL0.7213043171 ->0.7213044746 and CD0.01366317202 ->0.01366317693 change negligibly. This confirms the stalled-update correction in a long matched run; it **does not establish joint SST convergence** or solve the interior k-floor spatial residual. Positivity/finite field verification and the remaining follow-up cases are recorded separately at completion.

### Remaining SST residual and completed GG-all trial

A20-step residual-output diagnostic after the bound-aware SST3000-step run reproduces the reported raw norms from the native RHS fields. Point4992 now contributes only0.137% of omega residual squared (formerly99.990%). Omega's largest residual is at4940, with5.55%; the largest10 carry42.23%. The single k-floor point4992 still carries88.99% of k residual squared and has RHS≈-2.79017e-4. A purely diagnostic projection that discards bound-active negative k RHS changes log RMS-5.601 ->-6.080, still above the monitor target. **This projection is not a solver/convergence change** and is not used to declare convergence. Even the rest of k needs further reduction; the floor hotspot remains a discrete consistency issue.

Corrected Green–Gauss-all trial on the adapted RANS mesh completes3000 steps with density=-6.911358408, SA=-8.248298636, CL=0.7314702494, CD=0.01666413744. It changes both reconstruction and viscous gradients for JST flow and turbulence. Compare against WLS/SA-recovery at identical iteration: density=-6.888031585, SA=-6.923324991, CL=0.7333431911, CD=0.01667311268. SA improves1.3250 orders (~21.1times), density only0.0233 orders; all8 remaining interior floor-band points recover (8 ->0), with finite fields and positive density/internal energy. Lift changes≈-0.255%. This is a promising configuration effect, **not gradient accuracy proof or a justified default change**. The earlier WLS-reconstruction/GG-viscous setup was invalid; retain that parser failure separately.

### Additional targeted SST gradient follow-up

After the remaining main follow-ups, the corrected-SA-neg plan also includes `sst_bound_gg_visc`: same model/fixed-CFL3 restart as the completed SST candidate, NUM_METHOD_GRAD=GREEN_GAUSS but NUM_METHOD_GRAD_RECON=WEIGHTED_LEAST_SQUARES. Conventional Roe supports that split, unlike adapted JST. This changes **flow and turbulence viscous/source gradients together**, retaining reconstruction. It tests the unresolved k-floor spatial issue after the update stall was removed. Attribute any response to that combined gradient setting; no default change or isolated k-diffusion conclusion follows automatically. The output campaign remains `/tmp/su2-cfd-night-sa-neg-corrected`; its plan filename predates this extra targeted case.

### LINELET and limiter consistency follow-ups completed

LINELET3000-step assessments produce no important improvement under the present25-linear-iteration/error0.05 budget: adapted density=-6.908397660, SA=-6.922105777 versus ILU/recovery density=-6.888031585, SA=-6.923324991; conventional fixed-CFL3 density=-7.379850021, SA=-9.213989029 versus ILU density=-7.381877052, SA=-9.220438843. The adapted final flow linear count increases from2 to9. These are residual/correctness findings, not host-contention timing comparisons or a claim that line preconditioning cannot help other settings.

Freezing after500 continuation iterations reaches the selected density/SA targets in2160 steps (density=-8.000101342, SA=-10.012270760), essentially matching freeze-after100 at2162. But restarting the completed freeze-after100 state with limiters recomputed each iteration at CFL10 returns after1000 steps to density=-5.027209820, SA=-8.776103982. CL/CD remain close to the frozen result. This rejects interpreting the frozen-residual result as convergence of the continuously updated limiter equations. The long fixed-CFL3 result remains the tested path to those selected monitors with live limiters. LIMITER_ITER freezes both flow and turbulence; this experiment alone does not isolate either one.

### Missing flow limiter dimensional scaling: concrete next target

CFVMFlowSolverBase::SetPrimitive_Limiter calls computeLimiters without its optional refValue argument. Thus the relative normalization already implemented for turbulence is absent from flow Venkat/R3/R4/R5. The shared kernel normalizes projected slopes and extrema differences by max(abs(local field),abs(reference)) only when that argument is supplied. With DIMENSIONAL reference mode, the flow epsilon remains an absolute scalar; increasing VENKAT_LIMITER_COEFF does not fix unit dependence.

For the exact Venkat function, y=delta*(delta+proj)+eps2 and limiter=(y+delta*proj)/(y+2*proj*proj). A pressure example with proj=10, delta=5, K=0.05, reference pressure30000 and reference length1 gives limiter0.45454570248 in Pa versus0.99866991798 after rescaling pressure by30000. Raising K to1 gives only0.45652173913 in Pa. Relative normalization gives the same result across either unit representation. This arithmetic probe establishes unit dependence, not the cause of the RANS plateau.

Primary support: Nishikawa, White & O'Connell2023, section4, https://ntrs.nasa.gov/api/citations/20230004018/downloads/NishikawaWhiteOConnell_v7.pdf. Their field scaling includes a speed-of-sound floor for velocity components (avoiding a zero transverse reference). Their higher-order cell-centered scheme is not directly transferable to SU2's vertex-centered residual. Before a flow patch, inspect primitive indices/references in every flow family, AD inputs and zero components; add actual-kernel unit-rescaling checks and matched RANS/accuracy comparisons. No flow scaling patch/default change was made in this phase. This is a higher-priority next step than blindly increasing K or freezing limiters.
