# Narrow actual-query donor routing

Original metric-query imports route a whole-cell box, then test its centroid and
up to six edges for each candidate donor. The new path routes those seven known
queries by their individual boxes and exact-tests only that query. It retains
closed point/edge/face/corner contact and the sender's actual rounded centroid.
Full-cell imports remain for reconstruction with unknown future queries; sources
below the256donor cap retain the cheaper box path. No target, original-P1/BL,
quality/sliver, geometry,512-round or2D/MMG/config change.

The private request carries the original vertices plus a checked one-byte query
kind in place of the prior boolean. No extra index or collective; the saturated
route bound admits up to7requests per input cell before allocation. Existing
receiver bounds, donor union cap, global failure vote and resident-state
preservation remain. Returned donors still sort by canonical key.

Repository /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated, branch
codex/native-3d-transport-accounting; based on2d3ac1e3a552a1b90a87dfa3595fc1b372783ceb.
Build/source/object/archive/binary pins: query_routing_build_v1/validation.json.

| Folder | Actual result |
|---|---|
| query_routing_build_v1 | PASS full68-object/774-member configured driver |
| query_routing_mpi1_v1 | PASS73cases;12independent frames/6state pairs/6SU2CGNS round trips; serialMMG48assertions/two cases |
| query_routing_mpi2_v1 | PASS73cases/rank;24frames/12state pairs/12round trips |
| query_routing_mpi4_v1 | PASS73cases/rank;48frames/24state pairs/24round trips |
| query_routing_lifecycle_mpi1_v1 | PASS six actual metric/Hessian OFF/ON controls and CFD/BDF replacement |
| query_routing_n4_n4_m2_v1 | PASS three events N4M2 with fresh independent audit |
| query_routing_restart_su2_n2_n1_m1_v1 | PASS persisted SU2 reference/current+BDF restart and later adaptation |
| query_routing_restart_cgns_n2_n4_m2_v1 | PASS persisted CGNS N4M2 restart and later adaptation |
| query_routing_n8_n1_m1_v1 | FAIL full512campaign, identical physical work; first-event scoped certificate and VTU PASS |
| query_routing_n8_n1_m1_rounds1024_v1 | PASS three events, exact prior certified frame/state equality plus fresh outputs/accounting |
| query_routing_package_checks_v1 | PASS31lightweight tests, two fresh certificate-corruption controls and complete emitted work/notification/nested costs |

The new regression uses384conforming tetrahedra, exceeding the donor cap. It
compares exact returned IDs against an exhaustive closed centroid/edge reference
on a cube and rotated thin source, normal distribution and a single source owner
with remote caller/empty owners. It checks original-P1 centroids, complete edge
coverage and atomic invalid/memory rejection. The existing reuse test keeps
positive reuse and unchanged one-filter/donor/tensor invariants without assuming
the old request multiplicity.

The unchanged512n8 control still rejects at second coarsening:437accepted/
75rejected,517valid private cells,266pending directions. Accepted first mesh,
BDF states and next input are identical. Separately the1024control still accepts
2489/457/2245tets with exactly matched615/1002later coarsening attempts.
audit_query_routing_equivalence.py requires the prior full independent PASS and
its exact artifact/tool pins,18byte-identical current frame/state/reference/
input artifacts, fresh actualSU2CGNS signatures and all current closed runtime
phases.22physical/query-work fields match. Full geometry/metric/state
certification is reused explicitly, not rerun. Two fresh disposable-copy negative
controls reject altered BDF state and an inverted actual output cell. No original
artifact changed. First-event reuse remains a scoped certificate with full512FAIL;
no whole-case independent_audit.json is fabricated for that failed campaign.

| Second512coarsening diagnostic | Previous | Narrow routing |
|---|---:|---:|
| Private donor phase, seconds | 67.8449 | 37.9634 |
| ADT candidate records | 4545418 | 8472788 |
| Filtered records | 1698110 | 2862395 |
| Returned donor payloads | 347287 | 347287 |
| Donor batches / query fallbacks | 2572 /121 | 2572 /121 |
| Tracked worker calls | 34040 | 34040 |
| Tracked worker payload bytes | 130179440 | 141355136 |

More requests cause more candidate records and bytes, but each metric-query
candidate requires one segment test rather than up to seven. These are single
contended correctness diagnostics. Neither these phase numbers nor the whole
1024fixture165.837->113.328s establish a repeated speedup or MPI scaling result.
Tracked bytes include local encoded transport and are not physical wire traffic.
Nested donor search/filter spans seed/update too; never add it to exclusive
phases. No separate local performance campaign was run.

Goal1 remains active. Prepare a fresh73-case closed SGE package after updating
this branch. Cluster repetitions must measure complete cost, accepted work,
extra traffic, worker subsets, instrumentation overhead, MPI API/wait and
allocator/RSS/practical limits before declaring performance qualification. No
curved geometry,3DBL/fullunsteady/affordability claim or Goal2 activation.
