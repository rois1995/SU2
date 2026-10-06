# Where to inspect the grids

Working branch: `codex/native-integrated` in
`/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated`.
AdapNoExt is unchanged. Goal active; robustness/scaling limits are still being measured.

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
Prepared donor/transferred solution export will run with the later instrumented
fixture; it is not available yet.

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
The prior v1 folder preserves a config-parser failure (PARAVIEW_BINARY was invalid;
this branch uses PARAVIEW for binary XML). No solver iterations ran in that attempt.

Actual Euler goal-control restarts already exist in
`integration_evidence/integrated_goal_runtime_v7/{warm_p1,warm_p2,cold_p1,cold_p2}`,
paired by cycle with their input/adapted meshes. These are separate MMG/discrete-
adjoint controls. cold_p2 diverged during the final adjoint solve; its saved cycle2
primal restart is admissible and reproduced the adjoint failure in a static replay.
They are not native viscous solution snapshots or converged aerodynamic evidence.
