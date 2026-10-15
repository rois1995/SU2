# Goal3 — coordinated metric-quality patch optimization

Active repository `/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated`, branch
`codex/native-3d-bl`. Full Goal3 ACTIVE; published construction baseline remains
c29ffa96b0a9550cd6df82c56c79aa20ad2e93be (1832 cells/38 long edges, rejected).
No accepted curved mesh or new MPI/CFD/performance qualification.

## Preserved rejected study

The wall-front and dependency-scheduling experiments are published separately on
`codex/native-3d-wall-front-study`, commit
43d32f9b0bc6726b642bb65889d15fd14ba53ad1. [Source, handbooks and exact study evidence](https://github.com/rois1995/SU2/tree/43d32f9b0bc6726b642bb65889d15fd14ba53ad1).
[Verified receipts/summary](https://github.com/rois1995/SU2/tree/43d32f9b0bc6726b642bb65889d15fd14ba53ad1/integration_evidence/native_3d_bl_v1/wall_reconstruction_v1/wall_front_study_summary.json).
No failed experiment's production strategy is enabled on the active branch.

- Geometry-only fits exist for all16 oversized cylinder edges;11 retained-cell
  candidates stay positive. Selective/all-row normal construction both have16
  positive numerical geometry candidates. This does not establish native metric
  feasibility or complete mesh generation.
- Native50-cell actual-cylinder construction succeeds. A zero-trial fixed-point
  hint would skip repair;256 actual trials make two flips/four moves/one removal,
  reducing shape loss1.452684612→.285096654. q=.125061608/J=.036637573 still reject.
  Complete hint196 star has22 cells, union64; the worst cells have no artificial
  face directly on them. More geometry closure alone is not demonstrated to fix it.
- Four geometry/ownership controls pass1726 assertions. The unchanged1402-cell
  full4096-work experiment regresses to1765 cells/58 long edges (published38).
  A final alternative-before-growth revision still rejects the actual worst-edge
  round with zero edits/identical623 work. Its full campaign was not repeated.
- All native failures and source/build/binary/config/input/helper hashes remain
  on the study branch. No new MPI run was warranted for this rejected strategy.

## Next connected capability

Implement bounded coordinated movement driven by metric-quality residuals inside
existing private reconstruction, coupled with its reconnections. Existing spring/
axis suggestions do not use the quality residual's coupled sensitivity. This is a
hypothesis to test on the actual hard patch and full target, not an assumed cure.

The user's `MeshAdaptation/Scripts/FunScripts/MetricOptimize3D_Fun.py` is a useful
algorithm reference: analytic geometry derivatives, sparse least-squares and
spatial metric sensitivity. Its nodal tensor averages and acceptance bounds differ;
do not copy its field representation or treat its successful patches as native
acceptance. No new runtime Python/SciPy dependency is proposed.

- Reuse native determinant/quality measures, frozen original field, constrained
  movement, reserved identities, fixed artificial interfaces and atomic final gates.
  Optimize free interior points and wall apices in their height-plane tangent
  spaces; boundary changes remain governed by the existing geometric operators.
- For a constant tensor, q is proportional to D^(2/3)/S, where D is metric volume
  and S is the sum of six metric squared edge lengths. Analytic geometry
  derivatives are cheap. Native target samples belong at actual cell centroids.
  Where needed, six centered actual-field samples per active centroid give spatial
  metric sensitivity shared across all12 coordinate derivatives. Do not average
  evolving nodal tensors or change the frozen target/fade/gradation to get closure.
- Use metric-scaled variables and bounded damped coupled steps. Every trial must
  retain positive geometry and pass actual-query objective/height checks; final
  q=.20/J=.05, full integrated lengths, strict changed-edge progress, geometry,
  complete dependencies/clearance and publication gates stay unchanged. Preserve
  useful-progress/stagnation/work/time/memory diagnostics; no budget ladder.
- Validate analytic derivatives against finite differences on constant anisotropic
  tensors, then the actual failed patch (50 cells and its64-cell closure as needed)
  and the original1402-cell strict full target. A lower patch loss is not success.
  Use distinct fields/geometries and MPI at the connected milestone. Keep complete
  cold construction/removal/current+BDF/restart/viscous/performance gates intact.
- Count query/linearization/solve/validation work separately. Establish benefit on
  accepted meshes; rejected or contended runs cannot establish CFD affordability.

All owned jobs are terminal. The local build currently contains the study binary;
rebuild the active source before any run and verify provenance. The next build
should group the complete optimizer/caller/validation change. Do not rerun a full
suite merely for this documentation or corrected replay telemetry.
