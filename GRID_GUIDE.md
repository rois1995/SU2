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

## Real NACA0012

The fresh integrated three-cycle NACA run is queued behind repeated scaling and
affine robustness controls. Its eventual output is
`integration_evidence/integrated_airfoil_baseline_v8/audit_np{1,2,4}/native_airfoil_cycle_{0,1,2}_adapted.su2`.

You can inspect this existing **earlier native-baseline** result immediately:
[4-rank NACA, adapted cycle2](/media/rausa/4TB/SU2_Versions/SU2_AdapNoExt/BL_NATIVE_INTEGRATION_WORK/native_airfoil_wall_metric_guard_runtime_v1/audit_np4/native_airfoil_cycle_2_adapted.su2).
It is prior-goal evidence, not a fresh result for the integrated branch.
Input profile: [native_NACA0012.cfg](QuickStart/native_NACA0012.cfg).
Source testcase: [CNativeAirfoil2D_tests.cpp](UnitTests/SU2_CFD/adaptation/CNativeAirfoil2D_tests.cpp).

Current verified results and live supervisor details:
[ROBUSTNESS_SCALING_RESULTS.md](ROBUSTNESS_SCALING_RESULTS.md),
[HANDOFF_Codex.md](HANDOFF_Codex.md).
