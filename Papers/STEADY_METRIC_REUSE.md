# Steady metric reuse validation

The steady adaptation loop now consumes the metric just produced by Postprocess, matching the existing time-window path. Configuration/support checks still run before the loop. Public RemeshFromMetric and AdaptMesh continue recomputing from potentially changed solutions. No persistent cache, validity flags or new option is introduced; unsteady, adjoint and external refresh paths are unchanged.

The new regression counts exactly one metric calculation on each of the two meshes and a fresh calculation for explicit public remeshing. It passes at 1, 2 and 4 MPI ranks. Existing time-window state, physical-time-step and discarded-output regressions pass. Initial test-fixture failures (unsupported empty OUTPUT_FILES, then omitted ADAP_SENSOR) are preserved separately from production validation.

A four-rank matched RAE2822 native RANS cycle runs 2000 resumed iterations. Relative to the prior improved-GG executable, its adapted mesh and original-wall sidecar are byte-identical and every saved coordinate, flow, turbulence, primitive, wall diagnostic, Hessian and metric field is bitwise identical on both meshes. Prior independent mesh, BL/frozen-metric, admissibility and pairing checks therefore apply to these identical outputs.

Metric calls fall from three to two: one duplicate full graded complexity solve is removed per steady replacement. Current metric calls take 19.6733 and 16.7390 seconds. Whole-process elapsed time is 124.164 seconds versus the previous 130.398; single observations under contention do not certify a repeatable speedup. The structural saving is one full metric build. CFD convergence and forces are unchanged bitwise.

Raw inputs, outputs, exact executable, source fingerprints, patches, counters and logs are retained under /media/rausa/4TB/SU2_Versions/SU2_AdapNoExt/integration_evidence/metric_robustness/steady_metric_reuse_v1. The next pass targets repeated native gradation/complexity work; geometric BL composition, sensor demands and MPI acceptance gates remain required. This validated fix is separate from any gradation candidate.
