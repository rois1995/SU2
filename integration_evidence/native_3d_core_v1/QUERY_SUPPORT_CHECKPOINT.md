# Native 3D query-support donor checkpoint

Branch `codex/native-3d-core`; predecessor
`a1a23b12c4c9ccd174c8147a0d5620a9c9ade32f`. Goal 1 remains ACTIVE/full scope.
Repository: `/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated`.
Final source matrix MPI1/2/4 and independent audits PASS with stable current pins.

## Result and contract

A geometrically and metrically valid coarse tetrahedron can intersect too many
original cells to import their entire volume. Metric measurement only queries
its actual centroid and six edges. `ImportMetricCells` retains every original
donor with a closed intersection with those sites, including point-only touches.
It uses the existing exact integer/rational clipping kernel, with no rounded
root comparison or geometric tolerance. `Centroid` is shared with metric checks.
Original sensor-only P1 data and canonical original-node tie breaking stay intact;
composition runs at every actual query/cache hit. No composed tensor travels on
wire. Donor/segment cap256 and q>=.20/J>=.05/L<=1.8 are unchanged.

The engine keeps its cheaper whole-cell imports first. It halves failed batches
as before and tries a query-support import once only after a one-cell donor-cap
failure. `query_import_retries` counts that escalation. Invalid inputs and memory
failures retain rejection. Full private cavities still need whole-cell coverage:
this change admits measurements without pretending their initial centroid/edge
footprint covers every later reconstruction. Imports remain collective on the
explicit worker communicator, with empty inactive callers and fixed-width records.
The frozen source/index, accepted input and source storage are preserved.
No production 2D/MMG/BL/fade/Hessian/noise/config/transport code changed.

## Evidence

`query_support_regression_v3/validation.json` and `actual_nonmpi/validation.json`
PASS, stable current source pins. MPI1:68 cases/4535 assertions; MPI2/4:22 cases
per rank. Independent original-P1/residual, geometry/marker, anisotropic, repeated
refine/coarsen, private/automatic repair and recovery audits pass. The saved
`mesh_identity.json` verifies all1350 preceding mesh/source JSON/SU2 artifacts
byte-identical. Actual non-MPI:9 cases/115 assertions plus6->48->6->48 cycles,
repair/recovery, exhaustion, input preservation and composed-edge refusal PASS.

Final larger-source evidence is `query_support_sources_v3`. Box sources have
162/384/750/1296 tetrahedra;384-source sampling also tests thin8/shear4 maps.
Each coarse tetrahedron imports162/150/188/242 donors, respectively. Source162
retains the cheaper conservative boxes; its exact necessary footprint is82.
A source2058 adversary needs294 and still rejects at the unchanged256 cap.
The selected source-size matrix brackets that query-import limit; it is not a
universal mesh-size limit. Frozen sensor preparation is outside event timing.

The MPI1/2/4 worker sets are M={N,max(1,N-1),max(1,N/2)}. Mesh/source records
migrate to M and accepted or rejected mesh records return to N. The matrix has
18 successful events (12 valid coarse no-ops and six wedge1->8 refinements with
six splits each) and six deliberate source750 private-cavity cap rejections.
All accepted input bytes remain unchanged. There are36 initial/accepted/rejected
frames, exported from actual C++ records and independently audited for positive
volumes, conformity/manifold links, convex-reference coverage, pairwise overlap,
finite facet/marker area, original-P1 shape and exact source-resolved edge gates.
These SU2 grids are independent audit exports, not a production SU2/CGNS writer test.

Independent Fraction comparisons check every required closed intersection,
canonical centroid P1/actual composition, and analytic affine-sensor edge length.
Per rank campaign:3276 full44-axis closed SAT pairs,20160 query-support donor
comparisons,48 legacy P1 observations and36 centroid/six-edge observations.
Six negative controls reject missing records/donors and incorrect P1/edge values.
C++ also asserts composition on cache hits. Invalid/inverted/nonfinite/over-budget
requests preserve the source; actual MPI source ownership and M<N paths are used.

Inspect `query_support_sources_v3/mpi{1,2,4}_meshes/`:
`coarse_initial.su2`, `dense_initial.su2`, `wedge_initial.su2`, `wedge_m*.su2`,
`dense_m*.su2` and `dense_wedge_m*_rejected.su2`. Matching JSON retains original
source tensors and actual mesh/facet records. Dense wedge rejection occurs after
successful initial measurement, at the full private-cavity import; its input
remains oversized in the frozen target, so it is clearly labelled rejected.

## Measured cost and rejected approaches

`query_support_sources_v1` passed correctness but always used query imports for
measurements. Its source384 MPI1 wedge took .41932s, versus preceding .12843s;
exact filtering dominated. That policy is rejected. `v2` restricts query imports
to one-cell cap failures and restores the whole-cell path for existing cases.

The shared closed clipping helper also delayed zero-width edge-trace rejection.
`query_support_regression_v2/paired_checkpoint_timings` records three alternating
old/new process pairs with matched work: coarsen ON medians .1950774/.2371451s,
nonoverlapping ranges .1918466-.1979824/.2331655-.2429528. This performance
regression is rejected. Final clipping distinguishes closed donor admission from
positive-width edge integration, restoring the original early trace rejection.

`query_support_regression_v3/paired_checkpoint_timings` PASS: three alternating
verified binary pairs, same control source and matched accepted/rejected/edge
work. Each process includes three OFF/ON trials: nine internal observations per
column, not nine independent runs. Coarsen ON medians baseline .194757913s/new
.194757800s; ranges .191958899-.198727862/.192906966-.196385717 overlap. OFF
medians .194753087/.194821534s. Refine ON .048996920/.049073946s; repair ON
.011627697/.011564047s. These small contended controls establish removal of the
observed regression; they do not establish general speedup or scalability.

Final larger-source event times, exclusive/nested phase records, modeled work
bounds and parent maxima are retained in logs/audits/summary. Parent includes
record migration, worker communicator, engine source/index build and mesh return.
Original source preparation and diagnostic export/audit are separate. Do not
sum rank maxima or nested timers as wall time. Bounds describe requested storage,
not RSS. Detail OFF/ON controls and host contention logs remain available.

## Remaining work

Private cavity import is still capped; staged known-query reconstruction or
bounded streamed measurement is needed where whole-cavity donors exceed256.
The source2058 footprint also needs smaller query batches or streaming, not an
increased cap or softened target. Measure exact query-filter/trace costs before
larger campaigns. Original coplanar finite-triangle seams still restrict boundary
movement/coarsening; a compatible finite geometry-patch authority remains needed.

Production3D guard, tetrahedral weighted ParMETIS, actual CFD geometry/solution/
history rebuild/transfer, native SU2/CGNS output, full lifecycle CPU/wait/imbalance/
RSS and cluster repetitions/scaling remain unfinished. Composed-target edge
acceptance still rejects until geometry-resolved integration exists. No robust
BL construction, curved geometry, unsteady3D or CFD affordability claim. Goal1
must not be completed from this manufactured/frozen checkpoint.
