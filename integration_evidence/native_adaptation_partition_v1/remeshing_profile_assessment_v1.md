# Remeshing profiling assessment — 2026-10-08

Goal remains active. Source checkpoint bdb59520885ff0686e2cc40d46a661f37074a7c5.
Latest case: /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated/integration_evidence/native_frozen_rae_euler_to_bl_workers3_v1/runtime_np4.
N=4 CFD ranks, M=3 remeshing workers. Frozen original sensor; no CFD solve or solution transfer in this case.
Independent frozen metric, geometry, topology, quality, length and first-height audit PASS.
Whole process 37.816206 s; maximum complete Remesh call 37.349963 s.

| Scope | Seconds |
|---|---:|
| Partition work-weight estimation, max rank | 0.0253951 |
| Dual-graph construction, max rank | 0.00168231 |
| ParMETIS partition calculation, max rank | 0.0176282 |
| Working-cell migration, max rank | 0.00209062 |
| Worker interval, including engine setup, adaptation and final gates/statistics, max rank | 37.2642 |
| Reader slices and pending reference bindings for return to CFD, max rank | 0.0275372 |

Partitioning is included inside the complete Remesh call. Maxima can be on different ranks: summing them is not an exact additive critical-path accounting. The return timer does not include subsequent ReplaceMesh, CFD geometry partition/setup or solution/history transfer.

| Operator phase (elapsed, maximum across ranks, accumulated over visits) | Seconds |
|---|---:|
| Wall height | 0.141209 |
| Surface split | 0.348533 |
| Surface remove | 0.229426 |
| Surface redistribute | 1.61342 |
| Bulk remove | 3.04236 |
| Bulk split | 21.9394 |
| Bulk flip | 2.8932 |
| Bulk move | 7.00336 |

Bulk split dominates elapsed operator time. Coordinated repair (1.46686 s) is already nested in these operator phases, so must not be added again.

| Worker diagnostic | Minimum / mean / maximum seconds |
|---|---|
| Candidate selection | 2.38572 / 3.85048 / 6.24459 |
| Private reconstruction, including private metric evaluations | 4.03715 / 8.71515 / 15.0236 |
| Tracked collectives | 5.2712 / 7.43679 / 8.86172 |
| Largest private transaction | 0.0188087 / 0.153214 / 0.418077 |

These are diagnostic totals, not additive global wall phases. Collective timing includes waiting; validation failure elections and direct MPI calls outside World tracking are not fully covered. Donor import/routing, directory work, local packing, transaction validation and publication are not yet individually timed. Private reconstruction spans metric interpolation/geometric BL and topology work, so its cost cannot yet be attributed completely between those functions.

Excluded CFD rank slept for 37.266 s and consumed 0.27253 s process CPU. Inactive wall time overlaps active-worker time. Imbalance persists among active workers.

Existing instrumentation establishes phase bottlenecks with clocks around operations, avoiding clocks per metric query. Earlier Linux CPU stack samples support attribution, but were collected before the latest donor index/partition/subgroup implementation and are not a current-source profile.

Required next profiling: exclusive per-rank transaction scopes for dependency discovery/import, original-sensor donor discovery/import, reconstruction, validation and commit; keep whole inclusive timers and explicit unclassified overhead. Separate outer setup/final gates and return serialization from actual CFD geometry rebuilding/partition and direct-original solution/history transfer. Use current-source sampled stacks to split interpolation, geometric BL, topology work and MPI frames. Do not add per-query clocks or per-operation profiling collectives. Preserve CPU/PSI/load evidence and compare repeated full adaptation lifecycle costs, including both working and CFD return partitions.

Evidence: case solver.log, native_frozen_timing_rank_*.csv, run_evidence.json, independent_frozen_metric_audit.json. Current one-trial M1/M2/M3/M4 whole times: 65.529 / 45.866 / 37.816 / 42.567 s. Different mesh trajectories and operation counts: no fixed-work speedup or repeatability claim.
