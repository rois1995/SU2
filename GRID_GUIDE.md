# Where to inspect the grids

Working branch: `codex/native-integrated` in
`/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated`.
AdapNoExt is unchanged. Integration/robustness campaign complete; measured limits are reported in
ROBUSTNESS_SCALING_RESULTS.md.

## RAE2822 transonic cases

Actual case root: `integration_evidence/rae2822_transonic_v1`.
The latest completed common-app cases are Euler `nativefix_euler_rans_seed_v14`
and RANS `nativefix_rans_euler_seed_v23`, both on app v31 with four MPI ranks.
Euler starts from the triangulated original RANS grid and uses no BL metric.
RANS starts from the supplied Euler grid and requests first height 1e-5.
Both complete two adaptations and resume CFD with conservative transfer.

| Case | Input grid | Initial flow used for adaptation | First adapted mesh and flow | Second adapted mesh and flow | Preview |
| --- | --- | --- | --- | --- | --- |
| Euler | [input.su2](integration_evidence/rae2822_transonic_v1/nativefix_euler_rans_seed_v14/input.su2) | [flow0](integration_evidence/rae2822_transonic_v1/nativefix_euler_rans_seed_v14/flow_adap_00000.vtu) | [mesh1](integration_evidence/rae2822_transonic_v1/nativefix_euler_rans_seed_v14/mesh_adap_00001.su2), [flow1](integration_evidence/rae2822_transonic_v1/nativefix_euler_rans_seed_v14/flow_adap_00001.vtu) | [mesh2](integration_evidence/rae2822_transonic_v1/nativefix_euler_rans_seed_v14/mesh_adap_00002.su2), [flow2](integration_evidence/rae2822_transonic_v1/nativefix_euler_rans_seed_v14/flow_adap_00002.vtu) | [mesh/Mach](integration_evidence/rae2822_transonic_v1/nativefix_euler_rans_seed_v14/mesh_and_mach.png) |
| RANS | [input.su2](integration_evidence/rae2822_transonic_v1/nativefix_rans_euler_seed_v23/input.su2) | [flow0](integration_evidence/rae2822_transonic_v1/nativefix_rans_euler_seed_v23/flow_adap_00000.vtu) | [mesh1](integration_evidence/rae2822_transonic_v1/nativefix_rans_euler_seed_v23/mesh_adap_00001.su2), [flow1](integration_evidence/rae2822_transonic_v1/nativefix_rans_euler_seed_v23/flow_adap_00001.vtu) | [mesh2](integration_evidence/rae2822_transonic_v1/nativefix_rans_euler_seed_v23/mesh_adap_00002.su2), [flow2](integration_evidence/rae2822_transonic_v1/nativefix_rans_euler_seed_v23/flow_adap_00002.vtu) | [mesh/Mach](integration_evidence/rae2822_transonic_v1/nativefix_rans_euler_seed_v23/mesh_and_mach.png) |

Open the VTU files in ParaView with **Surface With Edges**, colored by Mach or
Pressure. Each contains the mesh and paired solution; matching binary restarts
and surface fields are in the same folder. Final meshes have 12,728 Euler and
36,263 RANS triangles. RANS has not met its configured residual convergence;
neither case establishes grid-converged aerodynamic forces. See
[validation and costs](RAE_CROSS_GRID_VALIDATION.md).

These are the final validated cases after removal of temporary instrumentation.
All solution/geometry and independent sensor/BL target audits pass. The earlier
accepted v13/v22 files remain available; final grids are byte-identical to them.
See [the completion audit](RAE_CROSS_GRID_COMPLETION_AUDIT.md).

Diagnostic history remains available, including
[the original rejected Euler grid](integration_evidence/rae2822_transonic_v1/euler_rejected_replay_v1/mesh_adap_00001_rejected.su2),
[its failure mask](integration_evidence/rae2822_transonic_v1/euler_rejected_replay_v1/mesh_adap_00001_rejected.vtu),
[its location preview](integration_evidence/rae2822_transonic_v1/euler_rejected_replay_v1/mesh_adap_00001_rejected_locations.png),
and [the original RANS complexity diagnosis](integration_evidence/rae2822_transonic_v1/BL_COMPLEXITY_DIAGNOSIS.md).

## Start here: input grid and solution used for adaptation

Actual four-rank native CFD case folder:
`/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated/integration_evidence/integrated_native_production_v2/np4`.
These files are in the separate integration worktree, not AdapNoExt.

| Stage | Grid | Inspectable solution | SU2 binary restart |
| --- | --- | --- | --- |
| Before first adaptation | [input.su2](integration_evidence/integrated_native_production_v2/np4/input.su2) | [flow_adap_00000.vtu](integration_evidence/integrated_native_production_v2/np4/flow_adap_00000.vtu) | [solution_adap_00000.dat](integration_evidence/integrated_native_production_v2/np4/solution_adap_00000.dat) |
| First adapted grid | [mesh_adap_00001.su2](integration_evidence/integrated_native_production_v2/np4/mesh_adap_00001.su2) | [flow_adap_00001.vtu](integration_evidence/integrated_native_production_v2/np4/flow_adap_00001.vtu) | [solution_adap_00001.dat](integration_evidence/integrated_native_production_v2/np4/solution_adap_00001.dat) |

[run.cfg](integration_evidence/integrated_native_production_v2/np4/run.cfg) is the actual local configuration.
The run computes the initial flow from freestream in memory; it does not load an initial restart.
The cycle0 output records that initial-grid solution before remeshing.
Open the VTU files in ParaView: each contains both grid and solution; use
**Surface With Edges**, then color by Pressure or Mach.
Later-cycle files contain solutions after resumed CFD iterations, rather than
an isolated immediate-transfer snapshot. These are short three-iteration-per-cycle
coupling checks, not converged aerodynamic solutions. Corresponding files also
exist in the `np1` and `np2` case folders.

## Ready-to-open scaling gallery

Open the `.vtu` files in ParaView and select **Surface With Edges**.
These exports preserve the raw audited coordinates/connectivity; physical edges
are included as line cells (`physical_marker`:10 bottom,11 sides,12 top).
They describe meshes generated with the indicated number of MPI ranks;
partition ownership is not stored in this gallery.

- [8192 input triangles, generated on1 rank](integration_evidence/grid_gallery_v1/strip_8192_np1/adapted.vtu)
- [8192 input triangles, generated on2 ranks](integration_evidence/grid_gallery_v1/strip_8192_np2/adapted.vtu)
- [8192 input triangles, generated on4 ranks](integration_evidence/grid_gallery_v1/strip_8192_np4/adapted.vtu)
- [Small64-cell input, generated on4 ranks](integration_evidence/grid_gallery_v1/strip_64_np4/adapted.vtu)
- [Physical-coordinate crop preview](integration_evidence/grid_gallery_v1/strip_8192_preview.png)

Each folder also contains `adapted.su2` and `provenance.json`. Exports were
checked for exact VTU coordinates and valid SU2 geometry/topology. The large
strip is20.48 by.02: zoom into a short section to see the cells. The preview
uses the actual physical aspect ratio and a near-left crop.
Raw source data: `integration_evidence/robustness_campaign_v8/large_t512/t512_p{1,2,4}`.
Each contains `points.csv`, `cells.csv`, `faces.csv`, `native_scaling.json` and
per-rank rejection logs. The parent group's independent audit covers all3.
Source testcase: [CNativeScaling2D_tests.cpp](UnitTests/Common/adaptation/CNativeScaling2D_tests.cpp).
Export tool: [export_engine_grids.py](integration_evidence/export_engine_grids.py).

## Integrated BL, boundary adaptation and CGNS controls

Saved meshes live in `integration_evidence/integrated_primal_controls_v6/audit_np{1,2,4}`.
Examples from the4-rank run:

- [Opposing-wall seed/donor, cycle0](integration_evidence/integrated_primal_controls_v6/audit_np4/native_bl_opposing_conservative_cycle_0_donor.su2)
- [Opposing-wall adapted, cycle0](integration_evidence/integrated_primal_controls_v6/audit_np4/native_bl_opposing_conservative_cycle_0_adapted.su2)
- [Opposing-wall adapted, cycle7](integration_evidence/integrated_primal_controls_v6/audit_np4/native_bl_opposing_conservative_cycle_7_adapted.su2)
- [Native CGNS output after adaptation/reload](integration_evidence/integrated_primal_controls_v6/audit_np4/native_cgns_enabled_reload_mesh_adap_00002.cgns)
- [Produced BL metric, CGNS mesh](integration_evidence/integrated_primal_controls_v6/audit_np4/native_produced_bl_cgns_mesh_adap_00003.cgns)
- [Produced BL metric with SA, CGNS mesh](integration_evidence/integrated_primal_controls_v6/audit_np4/native_produced_bl_cgns_SA_mesh_adap_00003.cgns)

The directories contain donor/adapted meshes for every saved cycle, reference
sidecars, field/metric CSVs and transfer summaries. Source fixtures:
[CNativeBL2D_tests.cpp](UnitTests/SU2_CFD/adaptation/CNativeBL2D_tests.cpp),
[CNativeCGNS2D_tests.cpp](UnitTests/SU2_CFD/adaptation/CNativeCGNS2D_tests.cpp).
Default-MMG control outputs are in `integration_evidence/integrated_mmg_default_v6/audit_np{1,2,4}`.

## Real NACA0012 — fresh integrated grids

The integrated three-cycle testcase now passes MPI1/2/4, with all nine saved
adapted meshes independently verified against the actual frozen P1 metric and
immutable original boundary. Mach0.3, Re10000, incidence1.25 degrees; prescribed
first height0.0002 chord. These are short viscous/transfer/resumed-solve checks,
not converged aerodynamic validation.

Actual testcase root: `integration_evidence/integrated_airfoil_baseline_v8`.
[File-by-file explanation](integration_evidence/integrated_airfoil_baseline_v8/CASE_FILES.md).

- [Initial mesh used for adaptation](integration_evidence/integrated_airfoil_baseline_v8/airfoil_input.su2)
- [Exact configuration](integration_evidence/integrated_airfoil_baseline_v8/airfoil_input.cfg)
- [Four-rank donor before first adaptation](integration_evidence/integrated_airfoil_baseline_v8/audit_np4/native_airfoil_cycle_0_donor.su2)
- [Four-rank frozen nodal metric](integration_evidence/integrated_airfoil_baseline_v8/audit_np4/native_airfoil_cycle_0_metric.csv)
- [Four-rank accepted mesh after final adaptation](integration_evidence/integrated_airfoil_baseline_v8/audit_np4/native_airfoil_cycle_2_adapted.su2)

The CFD solution was held in memory; this fixture did **not** save nodal flow or
restart files. Transfer CSVs contain integral summaries, not flow snapshots.
Actual donor/immediate-transferred conserved flow CSVs are now available in
`integration_evidence/integrated_airfoil_phase_timing_v1/audit_np{1,2,4}`:
`native_airfoil_cycle_N_{donor,adapted}_solution.csv` pairs exactly by global
point ID and coordinates with the corresponding saved SU2 mesh. All18field
snapshots pass positive-density/internal-energy and mesh-pairing checks. These
are CSV fields, not SU2 restart files; actual restart/ParaView outputs are
linked in the production section and at the top of this guide.

Ready for ParaView (**Surface With Edges**):

- [Fresh integrated preview with wall detail](integration_evidence/integrated_airfoil_gallery_v8/naca0012_preview.png)
- [Initial grid:10216 triangles](integration_evidence/integrated_airfoil_gallery_v8/initial.vtu)
- [Adapted cycle0:23328 triangles](integration_evidence/integrated_airfoil_gallery_v8/np4_cycle0.vtu)
- [Adapted cycle1:25549 triangles](integration_evidence/integrated_airfoil_gallery_v8/np4_cycle1.vtu)
- [Adapted cycle2:19487 triangles](integration_evidence/integrated_airfoil_gallery_v8/np4_cycle2.vtu)

Gallery also contains all four SU2 copies, SVG and provenance. All audited input
hashes were rechecked; VTU coordinates/connectivity match the source exactly.
physical_marker1=airfoil,2=farfield,0=volume.
[Exporter](integration_evidence/export_airfoil_gallery.py) accepts the source,
audit and destination arguments used to generate this fresh gallery.
The previous native-baseline gallery remains in `real_airfoil_gallery_v1` and
its earlier evidence is preserved.
Source fixture: [CNativeAirfoil2D_tests.cpp](UnitTests/SU2_CFD/adaptation/CNativeAirfoil2D_tests.cpp).

Current verified results and live supervisor details:
[ROBUSTNESS_SCALING_RESULTS.md](ROBUSTNESS_SCALING_RESULTS.md),
[HANDOFF_Codex.md](HANDOFF_Codex.md).


## Self-contained production cases — actual flow output campaign

`integration_evidence/integrated_native_production_v2/np{1,2,4}` each contains
local `input.su2` and `run.cfg`. The fresh runner uses the actual SU2_CFD adaptation
loop and requests double binary restart and ParaView flow/metric output.
Cycle0 solution belongs to input.su2; later flow/restart files belong to
mesh_adap_0000N.su2 with the same cycle index. Inspect actual files and evidence
before treating any case as completed.
[Exact config template](integration_evidence/integrated_native_production_v2/run_template.cfg).
[Sequential runner](integration_evidence/run_native_production_checks_v3.py).
[Live status](integration_evidence/native_production_chain_v3.json).

Serial cycle0 and cycle1 actual flow files are available now:

- [Flow on initial grid](integration_evidence/integrated_native_production_v2/np1/flow_adap_00000.vtu)
- [Flow on first adapted grid](integration_evidence/integrated_native_production_v2/np1/flow_adap_00001.vtu)
- [First accepted grid](integration_evidence/integrated_native_production_v2/np1/mesh_adap_00001.su2)
- [Actual first-adapted-grid restart](integration_evidence/integrated_native_production_v2/np1/solution_adap_00001.dat)

These are real solution outputs; open VTU in ParaView and select Pressure/Mach.
Exact double-restart/mesh coordinate pairing and positive density/internal energy
were checked for both (5233/11811 points), pinned in production_available_flow_v1.
All1/2/4 production lifecycles and saved flow/metric checks now pass. Every
plotted scalar and coordinate matches the double restart rounded to Float32
across all12 rank/cycle outputs (production_scalar_pairing_v1.json). All nine
independent frozen-P1/reference target audits now PASS. Exact target tensors
were also compared with the actual double restarts; all audited input pins
were rechecked (native_production_complete_v1.json).

[Final four-rank native flow on the19487-triangle grid](integration_evidence/integrated_native_production_v2/np4/flow_adap_00003.vtu).
[Its actual restart](integration_evidence/integrated_native_production_v2/np4/solution_adap_00003.dat).
[Its accepted SU2 grid](integration_evidence/integrated_native_production_v2/np4/mesh_adap_00003.su2).

The prior v1 folder preserves a config-parser failure (PARAVIEW_BINARY was invalid;
this branch uses PARAVIEW for binary XML). No solver iterations ran in that attempt.

Actual Euler goal-control restarts already exist in
`integration_evidence/integrated_goal_runtime_v7/{warm_p1,warm_p2,cold_p1,cold_p2}`,
paired by cycle with their input/adapted meshes. These are separate MMG/discrete-
adjoint controls. cold_p2 diverged during the final adjoint solve; its saved cycle2
primal restart is admissible and reproduced the adjoint failure in a static replay.
They are not native viscous solution snapshots or converged aerodynamic evidence.
