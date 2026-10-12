# Conservative native3D donor box filtering — 2026-10-12

Working repository: /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated. Branch: codex/native-3d-transport-accounting. Source changes from10b6427658e10637090c12a5f0709db0982dcae1 add the donor prefilter and its counter/regression chain; no2D/MMG/config/operator/metric/default-budget change.

Known centroid/edge imports first reject only certified separation between the segment and a donor axis-aligned box. Three projected rectangle tests select extreme corners, with explicit volatile long-double subtraction/product/direction rounding and a conservative64eps sign bound. Platforms whose long-double precision/exponent range does not cover binary64 differences/products skip this optimization. Point queries, touching/uncertain projections and all surviving candidates retain the original exact tetrahedron predicate. Full-cell/unknown-private-query imports, grouped request format, canonical donor/P1 sensors and actual-query composition remain unchanged.

SEGMENT_BOX_SIGN_REVIEW.md records the containment/error-bound/platform assumptions. Safeguarded standalonev4 PASS7258 independent rational closed-box cases under strict/productionfastmath/narrow-long-double modes, including coarse-grid observations and reversal;0false rejections. Narrow long double retains all candidates. Production also preserves corner-selection rounding. No cache/index/collective or data buffer is introduced by the stack-only prefilter.

## Current configured qualification

box_filter_build_v2 PASS full68-object/774-member driver; binarySHA256 c1bdef2a09d1e13bd1d137910db5713de32514f7a421369d6b1a177b48de54d6. v1 was a valid intermediate build; v2 adds final direction-rounding protection before any MPI test. box_filter_mpi1/2/4_v1 PASS97cases per rank, all seven exact distributed-record suites, actual geometry/solver/current+BDF/SU2CGNS bridge and serialMMG controls. Exhaustive384tet cube/rotatedthin/non-dyadic/source-on-root/empty-owner probes preserve exact closed donor-ID unions; counters confirm actual box rejections and original invalid/memory atomic refusal.

box_filter_n4_n4_m2_v1 PASS three actual driver adaptations with fresh full exact geometry/domain/markers/metric/current+BDF/actualSU2CGNS audit. box_filter_lifecycle_mpi1_v1 PASS six actual Hessian/metric/CFD geometry/solver/BDF OFF/ON controls and independent audits. box_filter_n8_n1_m1_rounds1024_v1 PASS three events;18input/mesh/state/reference artifacts exactly match the certified packed-request control. Existing full geometry/metric/state certification is reused explicitly only for identical data; actual outputs/runtime counters checked fresh,22physical/query work fields match. Production512 and its known larger-case completion limit are unchanged.

## Actual donor work and unqualified timing sample

Second diagnostic coarsening rejects 2344837 boxes before exact tetrahedron tests. This is 55.24% of previously rejected filter candidates and 19.64% of ADT candidate observations. All six stages retain identical candidates, total filtered donors, returned payloads/batches, private retries, collective calls/bytes and accepted/rejected/round counts. No tighter donor cap, target smoothing or geometry approximation explains the reduction.

Nested donor-filter elapsed 69.991->28.158s; second coarsening engine 89.308->44.339s. Whole single fixture 126.659->78.545s. These are one-CPU/nice19/thread1 contention-authorized correctness diagnostics, not repeated/scaling/instrumentation-calibrated speedup. Nested filtering spans seed/update too and is not added to exclusive donor or parent phases.

donor_box_filtered is emitted from the source service through engine stats and both actual runtime and ENGINE control logs. Profile aggregation verifies finite nonnegative integral values and box_filtered<=filtered<=candidates, preserving old schemas without this field. Actual current n8 rows pass aggregation/phase closure. box_filter_package_checks_v1 PASS32core+13comparison lightweight/mock checks; false counter subsets and unsupported source/default/header mutations reject.

The frozen SGE comparison now pins all six relevant source files for four known versions. Original/routing/packed->box forward pairs are supported in addition to earlier pairs; packed checkpoint7f76fd9a279cf384a37f8ae137dc4dc26212ce82 is the baseline to isolate this filter. Existing frozen older bundles remain immutable and supported. No comparison bundle prepared or cluster submission. Pull/rebuild/freeze a fresh candidate before launch. Cluster repetitions must measure net wall cost, detail/profiler overhead, MPI/rank costs and practical size/rank/memory/wait limits.

BOX_FILTER_VALIDATION.json pins complete build/case/audit receipts, exact-work counters and single diagnostic clocks. All owned jobs TERMINAL; source/failed/intermediate recipes and inspectable meshes/current+BDF states/VTUs retained. Goal1 ACTIVE; Goal2 not started. No curved-geometry, robust3DBL, physical fullunsteady or affordability certificate.

## Further current whole-case qualification

BOX_FILTER_ENVELOPE_VALIDATION.md/json records fresh full audits of aspect2
turning-direction size4 wedges on N1M1/N4M2 and a H20 thin N4M2 case using this
same qualified binary. All three campaigns pass;24geometry/donor and9state files
match older routing controls exactly, and8normalized H20 frames match the
current unscaled case. Actual box-filter rank-work counts165180/22482/62070
confirm the optimization is exercised. These are sequential correctness checks,
not repeated timing/scaling or a fix to the known production512completion limit.
