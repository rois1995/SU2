# Larger frozen n8 diagnostic with1024rounds

Production code checkpoint bb87e97fe1d89ee69ae8deac8fb447d0996f2d5e;
branch codex/native-3d-transport-accounting. This separate fixture changes only
the existing test option from512to1024rounds. Production default512 remains
unchanged; the matched512failure in full_star_facet_n8_n1_m1_v1 remains FAIL.
Config, input mesh and first sensor snapshot are byte-identical. q>=.20,J>=.05,
L<=1.8, sensor-only original-P1 and unchanged geometry/noise policies retained.

Actual three-event campaign and strict independent full-case audit PASS:
8input/accepted/readback mesh frames,3current+BDF state frames,13closed rank-stage
rows, actualSU2/CGNS round trips and source/object/archive/tool drift all empty.
This is one MPI rank/worker, contended one-CPU correctness; not scaling evidence.

| Event | Accepted tets | Points | Refinement attempts/accepted/rejected | Coarsening attempts/accepted/rejected | Audited minimum q | Audited minimum J | Audited maximum L |
|---|---:|---:|---|---|---:|---:|---:|
| 0 | 2489 | 590 | 457/457/0 | 428/272/156 | .511339 | .288666 | 1.799855 |
| 1 | 457 | 133 | 0/0/0 | 615/457/158 | .204335 | .082768 | 1.794632 |
| 2 | 2245 | 542 | 790/789/1 | 1002/379/623 | .239746 | .090104 | 1.799088 |

Every completed stage has zero cached invalid cells/pending candidates/repairs.
All domain volumes are exactly1/2; surfaces and finite marker junctions are
preserved while surface triangle counts change. Maximum independently audited
current/BDF relative residual3.899327321487061e-16. Transported original-P1 tensor
absolute error0 in all three events and both output formats. Final actual mesh
is envelope_mesh_00003.su2; envelope_s0/s1/s2.vtu include corresponding synthetic
current/BDF states and transported tensors. These are manufactured fields, not a
physical unsteady-flow or BL certificate.

The second event needs103more attempts than512:20additional accepted collapses
and83additional rejections. The third coarsening is close to this diagnostic
cap and has623rejections. Thus the512failure is not proof of impossible metric
or geometry, but raising a budget alone does not establish efficient production
remeshing. Preserve the baseline and improve measured donor/query support cost.

Numerical fixture165.837s, external independent audit898.664s, VTU export.220s.
Audit time is external verification and must not be counted as remeshing/CFD.
These are single contended-run diagnostics, not timing claims. The audit finished
just below the local900s wrapper guard; the reusable cluster runner already has
a7200s default. No numerical or audit timeout/retry occurred in this run.

Goal1 remains ACTIVE: donor-import optimization/equivalence, repeated cluster
performance and instrumentation overhead, complete MPI traffic/wait and allocator
calibration and practical size/rank limits. No production budget change, curved
geometry/3DBL/fullunsteady/affordability claim or Goal2 activation.
