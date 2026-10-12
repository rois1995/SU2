# Goal 1 requirement audit — 2026-10-12

Repository: /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated.
Current implementation branch: codex/native-3d-transport-accounting.
Production checkpoint:7f76fd9a279cf384a37f8ae137dc4dc26212ce82.
Goal1 ACTIVE. Goal2 not activated. This audit retains the complete original Goal1
scope in NATIVE_3D_DEVELOPMENT_GOALS.md; it does not redefine it around passing tests.

| Requirement | Current evidence | Verdict / remaining proof |
|---|---|---|
| Opt-in tetrahedral backend, separate branch, unchanged2D/MMG | Current native3D factory/support gate; query_packing_build_v1 full68-object build; NativeBridge2D and serialMMG controls in packing MPI matrix; earlier2D reuse checkpoint retained | Implemented/tested within declared scope. Packing touches only3D donor code/header/test. AdapNoExt untouched. |
| Orientation/SPD/canonical IDs/incidence and frozen tetrahedral criteria | CNativePredicates3D/Metric3D, selected reference/mesh tests; packing97-case MPI1/2/4; q>=.20/J>=.05/L<=1.8 independent audits | Verified for tested faceted/manufactured envelope, not arbitrary industrial meshes. |
| Sensor-only original-P1, bounded reuse and actual-query composition | FrozenField caches <=2048 sensor samples and <=256 donors; Query composes at each point/cache hit; distributed original-P1/composition and query-footprint controls | Verified supported sensor-only remeshing and composition-hook behavior. Production3DBL/composed edge integration explicitly rejects; not certified as available. |
| Split/collapse/move/face-edge reconnection, complete dependencies and atomic publication | Full97-case selection includes all29 directory/donor/engine plus3partition declarations; seven exact distributed audits; stale/rank-veto/memory/duplicate/rollback cases | Verified targeted repairs and repeated cycles; finite operator envelope remains. No proof of arbitrary cavity escape. |
| Adaptable planar triangles, feature/marker junctions, coupled volume reconstruction | Planar/coplanar/repair/bridge suites and whole n4N4M2/n8 cycles; exact facet/domain/manifold/no-overlap audits | Verified declared planar faceted domains. Curved authority/CAD is Goal2. |
| Tetrahedral cost-weighted partition, M=N/M<N and return to CFD ranks | WorkWeights uses metric volume, six-edge refine/coarsen demand and q/J demand; two-constraint partition controls; actual N4M2 mesh/solver/current+BDF replacement | Implemented and correctness-tested. Weight coefficients are a static proxy explicitly marked shortcut; actual rank-cost prediction/calibration remains open. |
| Manufactured aspect/size/direction matrix, first failure and residuals | Current packing cube/rotatedthin/non-dyadic/full driver checks; n8 exact18artifact equivalence; historical thinH10/H20/aspect2 source-pinned audits | Bounded evidence preserved. Current production512 limit remains identified; historic broader matrices belong to earlier pins, not automatically rerun/current-source proof. |
| SU2/CGNS writing/reload, current+BDF transfer, accepted-state preservation | Packing bridge and n4 fresh independent audits, n8 fresh actual outputs/exact states; six actual metric/Hessian/CFD/BDF controls | Verified declared paths. Historical persistent-restart checks remain separately pinned; no full physical unsteady or force accuracy claim. |
| Metric/Hessian/gradation/complexity, graph/ParMETIS/migration, query/cavity/commit/return/rebuild/transfer accounting | Native runtime/engine/partition/driver wall and CPU scopes, current counters, independent closure checks; actual lifecycle and query subcosts | Instrumentation closes on tested paths. Frozen sensors are distinct from actual CFD metric preparation. Optional/unexecuted stages and rejected3DBL need no fabricated timings. |
| Cache/accepted/rejected/communication and per-rank work/imbalance | Current logs; immutable payload/collective/work comparison; core/comparison parsers | Counts verified. Tracked bytes include self and exclude vendor/direct MPI. MPI API elapsed and CPU/elapsed difference are not pure wait/wire. Need cluster rank-cost calibration. |
| Peak RSS/admitted buffers, migration/private scratch and practical memory | Saturated admission/atomic refusal tests; runtime native_bound/process_peak_rss | Requested-storage bounds and lifetime RSS observed, not a calibrated allocator bound or practical large-size/rank envelope. |
| Repeated timing, useful accepted work/sec, detail/profiler overhead, size/M/rank limits | Closed reusable SGE packages, fixed-source pairs, three paired repetitions and OFF/ON controls prepared;31core/13comparison checks | NOT achieved. No actual new cluster comparison submitted/results available. Local single samples are diagnostics. |
| No unexplained unbounded scan/growth/retry | Donor ADT broad-phase candidate counts and exact filtering identified as dominant;512 transaction limit and1024diagnostic outcomes preserved | Known broad boxes can admit many original donors; mask packing reduces bytes, not candidate/predicate count. Standalone segment-box experiment investigates this with no production change. Larger practical costs need measurement. |
| Evidence/handoff/push and compact closed dependencies | Owned receipts/source/meshes/states/failures; strict four-file pins and closed regular-file cluster bundles; current pushed checkpoint | Retained. No cluster requests while user unavailable; no local performance campaigns. |

The remaining performance measurements are required before Goal1 completion.
The size8 production512 failure is a work-budget limit: current trajectory needs
more accepted/rejected transactions, while exact1024diagnostic feasibility is
proved. It is not266 invalid cells and is not fixed by payload packing. Any
algorithmic change must preserve the declared metric/geometry/height/state/cap
contracts rather than relabel the diagnostic budget as production success.

Available independent work: conservative donor broad-phase improvement and its
production correctness qualification, stronger non-reconstructible-cavity
replay/diagnosis if a genuine failure is found, and completion of missing
calibration tooling. Goal2 cannot start while the complete performance gate lacks
proof. R<1/R<=.20 physical-CFD affordability remains later physical workload work,
not a manufactured frozen-grid claim.
