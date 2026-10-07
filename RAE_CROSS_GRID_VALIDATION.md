# Native cross-grid RAE2822 validation

Both requested cross-grid lifecycles complete on the same cleaned native solver (app v31, branch `codex/native-integrated`). AdapNoExt was left untouched. Both actual MPI4 simulations, saved-flow/geometry checks and independent full-target audits pass. Core, configured CGNS/default-MMG/general MPI transfer, and adapter/captured-cavity regressions pass on1/2/4ranks. Final requirement proof is in [RAE_CROSS_GRID_COMPLETION_AUDIT.md](RAE_CROSS_GRID_COMPLETION_AUDIT.md).

| Path | Accepted triangles, cycles 1 / 2 | Independent minimum q | Independent maximum metric length |
|---|---:|---:|---:|
| Euler from triangulated RANS mesh | 10,444 / 12,728 | 0.225156 / 0.182442 | 1.797574 / 1.798842 |
| RANS from supplied Euler mesh | 24,833 / 36,263 | 0.200002 / 0.221504 | 1.799986 / 1.799995 |

Euler has no BL metric. RANS requests first height 1e-5, growth 1.2 and thickness 0.02. Maximum independently measured relative first-height error is 3.78e-12. Both paths retain the original reference within 1e-6, marker coverage and declared features. Boundaries adapt: Euler AIRFOIL192->318->254 edges; RANS192->350->852. FARFIELD also adapts.

Every saved restart pairs exactly with its mesh; VTU fields pair at their stored precision. Density, pressure and internal energy stay positive; SA stays nonnegative. Conservative projection reports no inadmissible states; both transfers resume CFD. Euler reaches density residual -8 on both adapted meshes. RANS reaches the configured 2,000-iteration cap, with final density -7.9567 and SA -7.1865; do not claim configured convergence or grid-converged forces.

## What fixed Euler

The first mesh is byte-identical to the appv28/v11 rejected control. Its second ordinary adaptation again reaches the two old shape blockages. One atomic surface transaction adds four boundary points, moves free apices and reconnects privately, finishing q=0.182442 and L=1.798842 in about 0.0178s. Both final Euler and RANS meshes are byte-identical to the accepted appv30 controls. The two RANS meshes also match the earlier v21 controls.

The fallback imports/reserves complete bounded dependencies, uses the same frozen target, excludes prescribed/protected wall layers, and allows at most eight private boundary splits. Intermediate shapes can be poorer, but publication requires q>=0.18, nonworsening maximum length and nonincreasing size deficit (strict decrease for size-only repair), unchanged artificial interfaces, valid physical geometry and participant certificates. Ordinary imports are unchanged. Local replay also resolves the separate upper-wall length blockage from Euler v12; that control remains archived as rejected evidence.

## Cost and demonstrated limits

| Path | CFD solve total | Adaptation total | Full solver elapsed |
|---|---:|---:|---:|
| Euler | 106.52s | 80.25s | 187.38s |
| RANS | 220.05s | 200.76s | 421.24s |

These are single workstation observations, not a clean repeated speedup study. Adaptation is about75% of total CFD solve time for Euler and91% for RANS in this replay; its startup cost from the Euler-type RANS seed is107.78s versus9.05s of initial CFD. The new Euler compound repair takes about0.0178s; ordinary transaction/selection/communication work dominates its adaptation. RANS first repair uses29joint transactions and about51.77s. Earlier appv30 runs under contention cost172.34s/211.55s of adaptation against142.05s/278.11s of CFD. Do not attribute differences solely to removal of temporary tracing or claim adaptation is always cheaper than CFD.

Actual validated scope: 2D triangular meshes, static retained polylines, conservative transfer, adaptive boundaries, up to4MPI ranks and36,263 accepted triangles. Do not infer 3D, moving CAD, large-rank scalability, general metric feasibility or aerodynamic convergence. Bounded dependency/transport admission and rejection preserve the accepted state when repair cannot finish. Remaining performance work should profile and batch ordinary selection/import/commit traffic while preserving the same ownership and final validation rules.

## Evidence

Cases are under `integration_evidence/rae2822_transonic_v1/nativefix_euler_rans_seed_v14` and `nativefix_rans_euler_seed_v23`: adapted SU2 meshes, same-index restart/VTU/surface fields, plots, solver logs, run manifests, independent inspections and metric audits.

`integration_evidence/native_cross_grid_v31_case_audit.json` contains exact case timings, q/length/height/geometry measurements, transfer recovery counts, all final MPI results, preserved-file hashes and a recheck of759production sources. `native_cross_grid_build_v31/evidence.json` pins the solver and its source snapshots. `native_release_core_v1` passes42cases/rank; `native_release_output_mpi_v1` passes44cases/rank (configured CGNS input/output/disabled output, rejected exports, MMG defaults and real MPI interpolation/conservative transfer); `native_release_adapter_captured_v1` passes3cases/rank (eight-cycle adapter control, produced BL and captured two-point MPI repair/rollback). All three groups pass on1/2/4ranks with the same test binary. `native_release_validation_v1.json` and `native_cross_grid_build_v31/build_state.json` record terminal success.

The earlier broad supplemental attempt accidentally selected a serial projection helper on two ranks; its failure remains archived. The corrected group exercises actual MPI transfer tests. No failure or cancelled attempt has been relabelled as passing.

Temporary phase tracing, failure-only full replay capture and unused corner/probe experiments were removed from the runtime source. Normal rejected meshes and location diagnostics remain; appv30 source archives preserve the removed experiments. The independent audits confirm the unchanged target and final grids. `native_euler_joint_surface_probe_v3` retains exact rejected patches and reproducible local replay.

The separate shock-demand/allocation diagnosis remains in `nativefix_rans_euler_seed_v21/SHOCK_ADAPTATION_ASSESSMENT.md`. Satisfying the supplied sensor/BL target does not prove that its shock allocation is optimal.
