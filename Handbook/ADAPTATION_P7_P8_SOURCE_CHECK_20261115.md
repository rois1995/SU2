# P7/P8 source-check history — both reported contradictions now corrected

2026-11-15. Historical source-check stop, preserved at a29c63f046. Reviewer2.0d-r now corrects the query-count gate; latest AGENTS/Part0 requires whole-item review. Current connected implementation and validation state: [report](ADAPTATION_P7_P8_CONNECTED_20261115.md). The following is the preserved diagnosis, not the current blocker. P8 section2.0e-r resolves the previously reported communicator mismatch. P7 check(c), which the user says remains unchanged, equates lower-level geometry-query calls with wall faces. The source and retained M6 output contradict that equality. No independent timer/gate redesign and no partial implementation of the ONE connected P7/P8/M18/M20/M21 item.

## P8 correction checked

The previous report is preserved at checkpoint a27f56cd78 and R/p7p8_source_check_20261115. Section2.0e-r now defines JobPeakRSS3D for the given communicator. Workers add idle-rank lifetime peaks captured collectively before worker execution; this is a lower bound. Exact parent-communicator sums occur at Remesh start, after the worker barrier and at Remesh end. EngineOptions carries the limit and idle contribution. No parent collective is required inside worker-only progress.

This matches the inspected structure: Remesher523 splits workers, 533–534 enters the worker World, 667–674 waits at the parent barrier and phase(10) follows it. The parent sum before the worker branch can be executed by every CFD rank. Monotonic process peaks justify the lower-bound relation even if MPI background activity raises idle-rank peaks. The exact parent values at rendezvous remain required. No code was changed to implement it yet.

## First remaining contradiction: P7's unchanged call-count gate

P7 check(c) requires the sum over ranks of `final_proof / geometry_bound` calls to equal the count of wall faces. Those calls count queries, not face proofs:

| Source | What is counted |
|---|---|
| Common/src/adaptation/CNativeAffineReference.cpp:2012–2014 | GeometryBound scope is entered by BoundPhysicalSurface. |
| Common/src/adaptation/CNativeBoundaryParameters3D.cpp:428–434 | GeometryCertificates iterates surface faces and calls EntityFaceError(Magnitude) while that entity remains within tolerance. |
| Same file:385–397 | A supported face uses SurfaceError directly; it can return without a BoundPhysicalSurface call. |
| Same file:377–383 | The query path calls BoundPhysicalSurface for Classification and may call it again for Magnitude: one or two scopes per face. |

Moving the same face function to owned-face lists does not establish one GeometryBound call per face. Its value is not a reliable proof-face counter. No new enclosing scope or revised acceptance counter was silently substituted.

Retained accepted F3b event1 evidence (read-only, no numerical rerun):

- rank0 stdout line1137 records final_proof geometry_bound calls=31,622.
- adapted_mesh_adap_00001.su2 lines873123–873124: WING3D has52,583 faces.
- FARFIELD has2,825 and SYMMETRY11,762; all markers total67,170. WING3D alone exceeds31,622. The marker labels are kept distinct;67,170 is not the wing count.

The proposal's approximately31,622 query-count prediction is compatible with the existing query timer, but its separate face-count equality is not. The required gate needs reviewer clarification/correction before this item can be implemented as written. Primary source review and a bounded read-only second opinion agree on the source distinction.

## Check status and scope

F3/F3b remains complete: authorized report-helper-only correction passed once and all six retained payloads match P6c. It was not rerun for this source check. [F3b report](ADAPTATION_F3B_20261115.md).

P7/P8/counters: no production edit, build or numerical run; ordered checks(a) units, (b) first collapse and(c) one full two-event milestone NOT STARTED. Source checking stopped at the first remaining contradiction; no complete qualification of unimplemented P7 or the new memory gate is claimed. No partial P7, option or counter changes. A5/B5 remain withdrawn; no cache/limit/geometry/quality gate changes.

Evidence: R/p7p8_corrected_source_check_20261115/source_check.json, source_hashes.json and SHA-sealed reviewer handoff/design snapshots. R=integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57. No owned heavy job was launched.
