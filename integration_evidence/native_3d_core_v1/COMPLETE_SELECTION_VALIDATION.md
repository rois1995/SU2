# Expanded native3D correctness selection — 2026-10-12

Working repository: /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated. Branch: codex/native-3d-transport-accounting.

The prior73-case reusable selection omitted the generic NativeDistributed3D tag. Adding it includes24existing cases:21directory/donor/engine controls and3partition controls. The complete selected set is97cases per MPI rank. This changes the reusable correctness harness and packaged auditors; the configured68-object test-driver binary, compiled C++ implementation, frozen targets and production512-round guard are unchanged.

## Verified current-binary evidence

| Folder | MPI ranks | Cases per rank | Distributed geometry/anisotropic/cycle/repair/automatic/coplanar frames |
|---|---:|---:|---|
| query_routing_complete_mpi1_v2 | 1 | 97 | 7 / 10 / 25 / 30 / 72 / 4 |
| query_routing_complete_mpi2_v1 | 2 | 97 | 9 / 15 / 45 / 48 / 108 / 7 |
| query_routing_complete_mpi4_v1 | 4 | 97 | 11 / 20 / 65 / 66 / 144 / 10 |

All97cases per rank PASS on MPI1/2/4. Each run also passes the existing exact driver geometry/current+BDF/SU2CGNS bridge audit and seven reused distributed-record auditors: audit_distributed, audit_donors, audit_anisotropic, audit_cycles, audit_repairs, audit_engine_repairs and audit_coplanar. Serial MMG2D/3D roundtrip controls pass48assertions. The omitted controls now directly exercise the current routing build: complete vertex/edge/face dependencies, partition constraints and worker return, stale/duplicate/high-ID/rank-veto/memory refusal, remote/composed original-P1 queries, repeated refine/coarsen and coupled move/reconnection.

Frame counts include initial and privately rejected progress, not only accepted meshes. Individual certificates explicitly distinguish strict accepted gates from partial diagnostic progress. This does not convert rejected progress into accepted grids.

COMPLETE_SELECTION_VALIDATION.json pins the full case/bridge/distributed audit receipts. Case validation pins all inputs/output/runner/auditor files, source/object/archive/binary identities and zero in-run drift. BinarySHA256: bb5a7664dda805daac6bbcc4b16e3da3ee3759f33475937beedd6a177c35955e. Current three successful case folders retain about29.8MB of compact logs, JSON/mesh/state records and audits; no duplicate test binary or build objects are copied.

## Reusable cluster execution

The core run.py correctness selection now expects97cases and invokes all seven independent suites after each actual bridge audit. prepare.py includes those seven existing auditors and their existing dependencies in the closed bundle. Failed/timed-out tools, non-PASS certificates and old73-case summaries stop later runs and retain diagnostic files. Lightweight31core plus12comparison checks pass in complete_selection_package_checks_v3. The two-source comparison still uses each frozen package own correctness runner, so old baseline72/73counts remain valid on their own declared scope; a fresh current candidate uses97. The compiled donor-routing comparison source pair is unchanged.

Preserved harness diagnostics: complete_selection_package_checks_v1 caught a mock label collision with audit_dependencies; fixed in v2/v3. query_routing_complete_mpi1_v1 has all97C++cases PASS but overallFAIL because the initial expected94count omitted the three partition tests; no independent suite was claimed for it. Its original source/count diagnostics are retained. The corrected serial receipt is query_routing_complete_mpi1_v2.

## Remaining Goal 1 gates

Size8 production512 completion remains limited. The separately certified1024diagnostic campaign establishes feasibility, not a changed production budget. Repeated complete-cost/scaling/instrumentation-overhead/MPI-wait/traffic/allocator/RSS/practical-rank calibration remains outstanding; the SGE comparison is prepared but has not been submitted. All local jobs here are terminal, one CPU/nice19/MPI<=4/one thread correctness checks under contention. No local timing campaign, speedup/affordability, curved-geometry, robust3DBL, full unsteady CFD or fullGoal1 completion claim. Goal1 ACTIVE; Goal2 not started.
