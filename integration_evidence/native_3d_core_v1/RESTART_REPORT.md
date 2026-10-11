# Persistent manufactured 3D restart checkpoint

Branch: `codex/native-3d-core`. Base: `b936b29e160905e070cab5ca86ff7618bea135a4`.
Implementation/evidence commit: `425c32f7480f989e4967c4dd79c1ee7e6e1f6c70`, pushed to origin/codex/native-3d-core with the full remote SHA verified.

All eight actual restart cases and the disabled control pass. The existing native driver writes two compact binary BDF2 states on the adapted mesh, is destroyed, and is reconstructed through the real SU2/CGNS reader and `Preprocess(3)`. Coarsening and refinement then continue through the existing native backend and accepted replacement. There is no manual initialization of the restarted histories.

The original faceted geometry authority stays byte-identical in all three sidecars. The post-restart remesher independently loads that authority into a pending publication; the fixture checks it remains unpublished until accepted replacement. Each next event constructs and freezes its own sensor-only P1 donor source. A complete tensor donor field persisted across restart is **not** demonstrated.

## Frozen conditions

`RESTART_MATRIX.json` freezes n2 wedges, exact volume1/2, adaptable oblique x+y=1 and bottom x=.5 marker junction, aspect1.5, spatially changing direction and strength1+.125x. Event scales4/1/4; q>=.20, scaled Jacobian>=.05, L<=1.8, default512 rounds and ADAP_HESSIAN_NOISE=0 remain unchanged. The first event uses18 points/24 tetrahedra; its accepted46 tetrahedra/24 points form the persisted checkpoint.

Checkpoint output is `envelope_mesh_00001.su2` or `.cgns` with `.native_ref`. Actual binary state files are `envelope_restart_00002.dat` (current) and `envelope_restart_00001.dat` (older). `envelope_restart.cfg` sets RESTART_SOL=YES / RESTART_ITER=3 and points to that mesh. File suffixes describe the fixture checkpoints and BDF slots; this fixture performs no PDE time integration.

## Results

| Format | N/M | Tetrahedra after three events | Restart state residual | Restart phase max (s) |
|---|---:|---|---:|---:|
| SU2 | 1/1 | 46 → 9 → 38 | 3.844e-16 | 0.009773 |
| SU2 | 2/2 | 46 → 9 → 36 | 3.844e-16 | 0.022379 |
| SU2 | 4/2 | 46 → 14 → 53 | 3.844e-16 | 0.028249 |
| SU2 | 4/4 | 46 → 14 → 46 | 3.844e-16 | 0.031346 |
| CGNS | 1/1 | 46 → 9 → 38 | 3.844e-16 | 0.049726 |
| CGNS | 2/2 | 46 → 9 → 36 | 3.844e-16 | 0.055391 |
| CGNS | 4/2 | 46 → 9 → 36 | 3.844e-16 | 0.098572 |
| CGNS | 4/4 | 46 → 14 → 46 | 3.844e-16 | 0.105201 |

The binary parser reused from TestCases/adaptation/capability/capcheck.py finds both files exactly equal to the checkpoint states, matched by physical coordinates. Maximum manufactured-state residual is3.8441481613e-16; transported and reloaded sensor tensor residuals are0. The resumed mesh audit checks exact incidence, embedding, positive volume, finite-facet coverage and markers, plus independently integrated frozen-P1 edge lengths and physical quality/sliver gates. Wedge-only minima q=.3214334361 and J=.1525128859, maximum L=1.7930625853.

Each restart audit rejects a changed older state, missing node, swapped current/older file and missing BDF file. These are independent checker controls; no new runtime missing-sidecar fault-injection campaign is claimed. Existing reference tests separately reject malformed authority and check missing-file preservation.

The disabled box control retains13 byte-identical raw geometry/source/state/readback files relative to wedge_control_box_n1_v1. restart_regression_v1 links all68 configured test objects and passes65 selected native/reference/2D+3Dbridge cases on every MPI1/2/4 rank, plus serial MMG2D/3D48assertions. No source/object/archive drift; all actual production archives and thin members are unchanged from the wedge checkpoint. No production, 2D, MMG, BL, metric or numeric-tolerance edit was required.

## Timing and practical scope

826 closed exclusive/nested rank-phase rows are in restart_rank_phases.csv; parent maxima, useful accepted edit rates and work counts are in RESTART_CHECKPOINT_VALIDATION.json. Restart write/teardown/reload phases are separate from the normal event parent. Mesh output has its own existing profile. The fixture write phase also contains bounded state preservation checks; teardown includes restart-config serialization. Record emission is excluded from its own profile parent. No per-rank maxima are summed to estimate elapsed time.

All runs are single sequential correctness pilots on a contended workstation at nice19, j1, MPI<=4 and one numerical-library thread. Differences in IDs/partition after reader reconstruction can change the subsequent valid trajectory: SU2 N4M2 ends46/14/53 cells, CGNS N4M2 ends46/9/36. They must not be interpreted as fixed-work performance comparisons. The small restart costs in the table are observations only, not speedup or scaling evidence.

Maximum requested native working bound8MiB; process lifetime RSS high-water mark34.77MiB. They measure different quantities and are not a calibrated end-to-end memory model.

## Inspectable evidence

Every case retains input/accepted JSON, actual adapted meshes and sidecars, current/BDF state records, binary restart files, config, logs, independent audits, source snapshots and stable pins. Example: restart_cgns_n2_n4_m2_v1/envelope_s2.vtu and envelope_mesh_00003.cgns, with envelope_s2_states.json; the restart checkpoint is envelope_mesh_00001.cgns and its two binary state files. Test executables, object files and Python caches are excluded from publication. Earlier failed evidence remains untouched.

Full Goal1 stays active. Larger domain/first-failure limits, complete traffic/wait/private CPU and memory calibration, disabled-detail overhead and closed SGE repeated/scaling packages remain open. No curved geometry, 3D BL construction, PDE unsteady integration, temporal accuracy or CFD affordability claim is made. No overnight cluster launch is requested.
