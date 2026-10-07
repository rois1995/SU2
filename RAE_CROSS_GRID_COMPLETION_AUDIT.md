# RAE cross-grid goal completion audit

Status: **all runtime, source and artifact requirements verified; commit pending**.
App v31 is the current cleaned source and passes the complete same-build campaign.
Worktree: `/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated`, branch
`codex/native-integrated`. AdapNoExt remains untouched.

| Requirement from the active objective | Authoritative evidence | Current determination |
| --- | --- | --- |
| RANS starts from the supplied Euler-type grid, constructs h0=1e-5 without coarse-seed BL interpolation, accepts two adaptations | `nativefix_rans_euler_seed_v23/run.cfg`, `input.su2`, solver log, saved inspections and independent composite-metric audits; CSolver sensor-only routing and query-time composition on original geometry | PASS current v31/v23; two accepted meshes, exact supplied seed, geometric BL query and both composite audits |
| Euler starts from original triangulated RANS grid with no BL metric and accepts two adaptations without shape/length rejection | `nativefix_euler_rans_seed_v14/run.cfg`, `input.su2`, solver log and independent original-connectivity P1 metric audit | PASS current v31/v14; no BL settings, two accepted meshes and both independent P1 audits |
| Actual conservative transfer is admissible and CFD resumes on both accepted meshes | Each current case's log, exact paired restart/mesh and VTU inspections, positive density/internal energy/pressure, nonnegative SA | PASS current v31 both cases; two transfers, zero inadmissible states/recovery patches, positive paired outputs |
| Surfaces adapt; markers, features, original reference tolerance and RANS height are preserved | Independent case inspections and reference audits, boundary edge counts and prescribed-height errors | PASS current v31; marker/features retained, adaptive AIRFOIL/FARFIELD counts, reference<=1e-6 and height error<=3.78e-12 |
| MPI ownership, participant certificates, atomic rollback and bounded dependency rejection | `native_release_core_v1/evidence.json`, tests of shape/size compound publication, fault2/fault4 byte retention and dependency cap | PASS current v31 on1/2/4ranks,42cases/rank |
| Existing configured CGNS input/output, disabled-output behavior, rejected exports, default MMG and general MPI interpolation/transfer remain functional | `native_release_output_mpi_v1/evidence.json` and actual selected test sources | PASS current v31 on1/2/4ranks,44cases/rank |
| Actual blocked BL cavity exercises two insertions through native MPI import/publication and rollback | `native_release_adapter_captured_v1/evidence.json`, saved four-rank fixture, explicit NativeRAEJointMPI test | PASS current v31 on1/2/4ranks; actual two-point publication and byte-exact participant rollback |
| Current implementation is identified by executable and source hashes | `native_cross_grid_build_v31/evidence.json`,759production source snapshots, current test archives | PASS terminal campaign; current source759files, solver/test binaries and archived artifacts SHA-verified |
| Reviewable accepted and rejected grids, paired flow/restart files, failure locations and guide | Actual files in case folders and `GRID_GUIDE.md`; earlier rejected Euler controls retained | PASS final v31 input/adapted meshes, restart/VTU/surface fields and plots; earlier rejected outputs retained |
| Practical runtime/complexity limits are measured and stated honestly | Per-case CFD/adaptation/transfer timing, accepted cell counts and MPI width, `RAE_CROSS_GRID_VALIDATION.md`, historical scaling envelope in `ROBUSTNESS_SCALING_RESULTS.md` | PASS final cost and scope recorded: up to4ranks/36,263triangles; substantial adaptation cost stated |
| HANDOFF reflects actual jobs and remaining work; selected source changes committed on the separate branch | `HANDOFF_Codex.md`, live process inspection, Git branch/status/diff | Final source review complete; HANDOFF/docs being finalized and selective commit pending |

All paths above are relative to `integration_evidence` unless they name root
documents. Cases are under `rae2822_transonic_v1`. The main final campaign is
`native_release_validation_v1.json`; the live parent is recorded in
`native_cross_grid_build_v31/build_state.json`. Never replace a historical failed
or cancelled attempt with a passing label. The earlier broad supplemental filter
passed serial checks but incorrectly selected a serial geometry projection under
MPI; its np2abort remains archived separately.

Completion of this goal does not imply converged aerodynamic forces, native3D,
unsteady native adaptation, moving/CAD geometry or large-rank speedup. Actual
RANS v23 stopped at its2000iteration cap short of both configured residual limits.
Adaptation remains a significant cost, sometimes exceeding CFD; the current goal
requires measuring that limit, not silently relaxing mesh contracts to hide it.

Recheck the retained proof with `python3 integration_evidence/check_native_cross_grid_completion_v31.py`. It verifies current source/binary identities, all actual cases and MPI groups, and the stored artifact hashes; it does not change already recorded evidence.
