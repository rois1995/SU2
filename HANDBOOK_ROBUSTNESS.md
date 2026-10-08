# CFD robustness handover

Updated: 2026-10-08. This is the transferable record for the mesh-quality CFD work. Pending entries below are not measured results.

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
