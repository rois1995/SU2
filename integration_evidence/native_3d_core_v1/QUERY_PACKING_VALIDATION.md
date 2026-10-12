# Grouped native3D donor query requests — 2026-10-12

Working repository: /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated. Branch: codex/native-3d-transport-accounting. Qualified source changes from5abcff1b38ada72a88cfd579155165bf4877a46d touch only CNativeDonor3D.cpp/hpp and its existing distributed regression.

Known metric queries now send one fixed-size cell record per owner with a seven-bit centroid/edge mask, instead of repeating its geometry in separate requests. Each owner still uses the individual closed query boxes and one exact segment predicate per candidate. The sender rounded centroid occupies the existing point-box fields; it is never recomputed remotely. Query order within each caller/cell, canonical returned donor union, full-cell/unknown-private-query imports and the cheaper small-source path remain unchanged. No new index, collective, field approximation or configurable algorithm switch.

Request serialization fields and record size are unchanged. Saturated admission covers at most one request per cell/owner plus the bounded owner-mask buffer. Caller memory caps and cavity/donor union caps are unchanged; lower predicted request storage reflects fewer real records. Sensor-only P1 and composition at every actual query, finer requests, fade, noise0 and production512 rounds are preserved.

## Qualification

Full configured68-object/774-member build PASS; binarySHA256 c9cb94011938f532edccfbc683b92dd99d0e9f8a4276138227adb2056de4bdeb. Complete97-case selection PASS per rank on MPI1/2/4, including all seven exact distributed-record suites and actual driver geometry/current+BDF/SU2CGNS audits. The existing exhaustive384tet cube/rotatedthin test now adds non-dyadic coordinates and verifies <=one request per cell/owner; source-on-root/empty-owner case requires exactlyone. Returned donor IDs match an exhaustive closed-centroid/edge reference; actual sensor/edge queries and atomic invalid/memory rejection pass. Serial MMG controls also pass.

query_packing_n4_n4_m2_v1 PASS three actual native remesh/driver replacement events with a fresh complete independent audit. query_packing_lifecycle_mpi1_v1 PASS six actual metric/Hessian/CFD geometry/solver/BDF OFF/ON controls and independent checks. query_packing_n8_n1_m1_rounds1024_v1 PASS all three diagnostic events;18mesh/state/reference/input artifacts exactly match the certified narrow-routing control. Five actual SU2/CGNS outputs and current accounting checked fresh;22physical/query work fields match. Existing complete geometry/metric certificate is reused explicitly for identical data, not described as a new geometry audit. Production512 default and its known larger-case limit are unchanged.

## Recorded payload reduction, timing still unqualified

On the second coarsening stage of the matched n8 diagnostic, tracked worker payload bytes decrease180480816 ->162969840:17510976bytes saved (9.70%). All six stages retain identical donor candidates/filtered/payload counts, batches, private-query retries, collective-call counts and accepted/rejected/round counts. Other stages retain the same tracked bytes; peak native admitted bound remains unchanged. Counters include self encoded traffic and omit vendor/direct MPI, so this is not physical wire traffic or an allocator/RSS improvement.

Recorded whole-fixture elapsed times are 113.328s for routing and 126.659s for packing. This single contended sample did not improve elapsed time; it is neither a sustained speedup nor a calibrated regression result. Packing reduces requests/bytes but leaves expensive donor predicates unchanged. Repeated cluster comparison must decide the net CPU/communication effect.

QUERY_PACKING_VALIDATION.json pins build/case/audit receipts and unchanged-work counters. Original inputs, accepted meshes/current+BDF states, logs and actual outputs remain in named query_packing_* folders. n4/n8 envelopes include inspectable VTUs. All numerical jobs TERMINAL; one CPU/nice19/thread1/MPI<=4 local correctness only.

The reusable SGE comparison now pins all four relevant source files for three exact versions. Supported forward pairs: original2d3ac1e3a552a1b90a87dfa3595fc1b372783ceb ->routing7b4e0d767fbda42ff17064ed93960316d794d5ab, original->packed, or routing->packed to isolate grouping. Existing frozen routing packages remain supported and immutable. Source/header/default/test mutations, unrelated changes, reversed pairs and mismatched build options reject before copying. query_packing_package_checks_v1 PASS31core+13comparison checks; no actual comparison bundle or cluster job submitted.

Goal1 ACTIVE; Goal2 not started. Repeated full-cost/scaling/instrumentation/wait/traffic/allocator/RSS/rank-limit qualification and the production512 larger-case limit remain open. No curved-geometry, robust3DBL, physical fullunsteady or affordability certificate.
