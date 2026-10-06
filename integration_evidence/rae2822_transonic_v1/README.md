# RAE2822 transonic adaptation trials

Worktree: `/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated`, branch
`codex/native-integrated`. Cases run sequentially on four MPI ranks with one
thread per rank. AdapNoExt remains untouched.

## Current results on the user-provided Euler-type seed

Both cases use an exact copy of `mesh_RAE2822_euler.su2`: 3,592 points,
6,952 triangles, no initial BL. Input validation and hash are in
[euler_seed_provenance.json](euler_seed_provenance.json).

| Case directory | Outcome |
| --- | --- |
| `euler_no_bl_seed_v1` | Exit 0, two accepted native adaptations, 12,788 final triangles. All three density residual criteria met. |
| `rans_no_bl_seed_v1` | Stopped by user during first remesh. Coarse initial SA solve met its configured criteria; no adapted mesh accepted. |

Conditions: Mach 0.729, angle 2.31 degrees; RANS-SA Reynolds number 6.5e6.
RANS requests first height 1e-5, growth 1.2, thickness 0.02 through adaptation.
It has not demonstrated resolved-wall RANS performance or final y+.
Both models request Mach/Pressure metrics, complexities 4,000/6,000,
conservative solution transfer and adaptive boundary sampling.

See the [Euler mesh and Mach preview](euler_no_bl_seed_v1/mesh_and_mach.png),
[its results](euler_no_bl_seed_v1/RESULTS.md) and full
[final mesh and flow](euler_no_bl_seed_v1/flow_adap_00002.vtu).
Inter-grid force changes remain large; do not treat this as grid-converged
aerodynamic validation.

The [BL complexity diagnosis](BL_COMPLEXITY_DIAGNOSIS.md) explains the RANS
refinement demand. One sharp-tip node contributes 89% of the nodal complexity.
The [hotspot preview](bl_complexity_hotspot.png) and saved numerical diagnosis
localize that demand. No metric fix has been implemented yet.

## Earlier failed input and diagnostic replay

`euler_native` preserves a config-parser failure. `euler_native_v2` used the
triangulated official RANS seed and returned incomplete quality. The prepared
`rans_sa_native` was never run and was superseded by the user's supplied mesh.

`euler_rejected_replay_v1` reproduces the earlier quality failure with a solver
change that adds diagnostic export only. See its [results and exact locations](euler_rejected_replay_v1/RESULTS.md),
[rejected grid](euler_rejected_replay_v1/mesh_adap_00001_rejected.su2),
[ParaView failure mask](euler_rejected_replay_v1/mesh_adap_00001_rejected.vtu),
and [failure preview](euler_rejected_replay_v1/mesh_adap_00001_rejected_locations.png).
There are 34 almost-collapsed upper trailing-edge failures and 2 lower-surface
shape failures. The accepted input mesh and solution were retained.

`input.su2` pairs with `flow_adap_00000.vtu` and `solution_adap_00000.dat`.
Accepted `mesh_adap_0000N.su2` pairs with flow/restart of the same index.
Rejected meshes contain geometry and diagnostics only, with no transferred CFD solution.
All raw case outputs are preserved locally. Per-case `run_evidence.json` records
terminal state; `user_stop.json` identifies the intentional RANS cancellation.
