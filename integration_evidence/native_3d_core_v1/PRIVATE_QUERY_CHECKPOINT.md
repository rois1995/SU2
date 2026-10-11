# Native 3D bounded private-query reconstruction

Branch codex/native-3d-core; predecessor dfe841a4ceaa0e7d453960e9afa386e28d7271f4.
Repository /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated. Goal1 ACTIVE/full scope.

Whole-volume donor imports remain the fast path. Only their collective cap failure
uses the new fallback: build a known geometric proposal, import its actual
centroids and closed edges in batches<=16, halve a rejected batch to one cell,
and assemble complete cell minima/unique-edge measurements before the shared
frozen progress gate. Each movement/reconnection proposal gets fresh query
support; restricted support is never reused for arbitrary cavity points.
Empty losing workers participate in matching collectives; unchanged-mesh
measurement failure is elected globally. Source/import/admission failure rejects
the private event. A single cell whose query union exceeds256 still rejects.

Geometric direct proposals now precede donor import. An impossible collapse on
an already-admissible coarsening mesh needs no metric/donor work. Invalid-field
coarsening and refinement retain their coordinated repair fallback. Geometry
buffers enter import memory accounting. Donor cap256, q>=.20, J>=.05, L<=1.8,
original immutable sensor-only P1, canonical ties and scalar reuse coordinate
checks remain unchanged. Composition still executes at actual queries/cache
hits; unsupported composed edge integration rejects, never substitutes sensors.
No 2D/MMG/BL/fade/noise/solver/config/build-flag changes.

private_query_regression_v4 PASS MPI1/2/4 and every independent transport/P1,
geometry/marker, anisotropic/cycle/repair/recovery audit: MPI1 70 cases/4565
assertions;22 cases/rank MPI2/4. All1350 previous mesh/source JSON/SU2 files
are byte-identical. actual_nonmpi PASS:9 passive cases/115 assertions and
repeated refine/coarsen/repair, rejection and OFF/ON controls.

private_query_sources_v3 PASS MPI1/2/4, M=N and M<N:63 independently audited
initial/accepted/rejected frames,36 successful events and six deliberate
round-exhaustion rejections. Source750 now completes one->eight refinement
(six splits), two->one general wall coarsening and four->four interior repair
(one movement). Source/accepted input bytes remain unchanged; exhausted private
progress is exported as _rejected. All126 JSON/SU2 frames match the pre-
geometry-first private-query baseline. The existing exact SAT/closed support,
canonical/P1/composition, analytic length checks and seven rejection adversaries
pass. Immutable fixture expectations are cached during adversarial log replay;
every altered observation is still checked. Source1296 support measurement
passes; source2058 needs294 query donors and rejects. This is a fixture envelope,
not a universal source-size bound.

Grids and matching frozen metric/source states: private_query_sources_v3/
mpi{1,2,4}_meshes/{dense_wedge,dense_coarsen,dense_repair}_m*.su2 and .json.
Initial grids use _initial; exhausted progress uses dense_reject_m*_rejected.
These are manufactured frozen fields, without CFD solution/history files.

Three alternating MPI1 binary pairs use the identical pinned probe and
identical target observations, accepted/rejected edits and meshes. Import work
is recorded separately because the optimization removes unnecessary queries.
Parent times include source/index construction, mesh/source migration, worker
communicator/core and return; fixture generation/export/independent audit are
outside this operational timer. Default-build results:

| Frozen event | Baseline median [range], seconds | Geometry-first median [range], seconds |
|---|---|---|
| coarse_m1 | 0.049277 [0.048729,0.050041] | 0.052801 [0.052328,0.054281] |
| wedge_m1 | 0.111537 [0.111508,0.113287] | 0.121100 [0.118052,0.126446] |
| dense_m1 | 0.209673 [0.205816,0.209820] | 0.226535 [0.223722,0.226904] |
| dense_wedge_m1 | 0.193837 [0.192261,0.194350] | 0.208535 [0.207761,0.228766] |
| dense_coarsen_m1 | 0.443577 [0.442003,0.443984] | 0.136109 [0.135138,0.136680] |
| dense_repair_m1 | 0.460441 [0.459674,0.463187] | 0.493447 [0.491639,0.493714] |
| dense_reject_m1 | 0.129088 [0.126630,0.132110] | 0.137610 [0.136976,0.139753] |

Coarsening retains13 rounds/one accepted/12 rejected proposals while batches
fall19->7, private query retries13->1, candidates23850->5850 and payloads
3277->1021. Its default parent median improves about3.26x, useful accepted
edits/s about2.25->7.35 for this fixture. Faster exhausted rejection is not
success. The other default MPI1 binaries show a consistent6-9% timing increase;
the same-CPU6 repetition retains it. Only CNativeEngine3D.o differs, and shared
kernel symbols shift despite identical shared objects. A layout-only relink
(-Ttext=0x4d90) restores their prior addresses and overlapping unchanged-case
ranges in three further CPU6 pairs;14 MPI1 mesh/source frames match. This
is strong evidence of a layout contribution, not a production-build speedup.
No linker padding/alignment flag is added to SU2. Default measurements and all
diagnostic receipts remain retained. Binary/layout variability must be assessed
again in the actual production build.

Three alternating non-MPI baseline/candidate pairs retain identical accepted
and edge work with three internal OFF/ON trials/process. Coarsening ON medians
.198715/.193381s have overlapping ranges; refinement/repair also overlap.
Nine internal observations are not nine independent process repetitions.

Retained failed history: regression_v1 omitted split facet authority; v2 tried
fixed-skin coning to a new boundary point, which correctly rejects. The corrected
facet-aware split fixture passes in v3/v4. sources_v1 incorrectly enforced
accepted shape on deliberately distorted repair input; corrected v2/v3 audit
keeps every accepted shape strict and adds the distorted accepted-mesh adversary.

Goal1 remains incomplete: finite compatible coplanar authorities, production
native3D opt-in, weighted tetrahedral ParMETIS, actual CFD solution/history
return/rebuild/transfer, actual CGNS output, full CPU/wait/RSS/lifecycle and
scaling qualification remain. Streamed proposal CPU scopes currently include
election CPU; narrow these before private CPU/wait qualification. Production
CheckSupport still rejects nDim!=2. No curved geometry, BL construction, unsteady
3D or CFD affordability claim. Continue the full original goal.
