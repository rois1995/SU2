# Native 3D closed-query filtering

Branch codex/native-3d-core; predecessor fcda83cb2b3378c55124c474e25da3aeb33f3d20.
Goal1 ACTIVE/full scope. Repository /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated.

IntersectsSegment now uses the existing robust exact-sign Orientation predicate
for positive donor validation, same-face endpoint separation and endpoint
containment. Floating arithmetic is accepted only by its existing error filter;
uncertain signs still use exact integers. Either endpoint inside the positive
convex tetrahedron proves closed intersection. Two outside endpoints whose plane
tests remain inconclusive still use exact rational clipping. Point-only contacts,
subnormal signs and canonical source ties are retained; no tolerance, target,
source data, import cap, edge integration, BL/fade/noise/2D/MMG policy changes.

query_filter_regression_v1 PASS MPI1/2/4 and independent original-P1/transport,
geometry/marker, anisotropic/cycle/repair/recovery audits. MPI1:69 cases/4542
assertions; MPI2/4:22 cases/rank. All1350 preceding mesh/source artifacts match
byte-for-byte. New tests cover regular containment, bidirectional crossings,
subnormal donors/outside points and inconclusive nonintersections. Existing
boundary-only, huge-coordinate, invalid/nonfinite and source coverage controls pass.

query_filter_sources_v1 PASS MPI1/2/4 and M<N, with box162/384/750/1296 sources,
thin8/shear4 sampling, source2058 cap rejection,18 successful events/six deliberate
private-cavity failures and36 independent mesh/metric frames. All72 larger-source
mesh/source JSON/SU2 files match query_support_sources_v3. Exact Fraction/SAT,
canonical P1/composition, analytic edge and six oracle adversaries pass.

run_source_timings.py provides three alternating verified baseline/candidate MPI1
process pairs. Same pinned probe source and identical actual query records,
donor/candidate/payload counts, accepted/rejected edits and eight mesh/source JSON
frames are asserted for every pair. Its paired_source_timings/validation.json PASS.

| Frozen event | Baseline median [range], seconds | New median [range], seconds |
|---|---|---|
| Source384 valid coarse no-op | .051452 [.050921,.052806] | .049531 [.049043,.049674] |
| Source384 wedge, six splits | .121119 [.120420,.121934] | .113765 [.113051,.118227] |
| Source750 valid coarse no-op | .617780 [.617252,.620717] | .210759 [.208175,.212063] |
| Source750 rejected private cavity | .110705 [.109507,.112068] | .042506 [.041890,.043448] |

Source750 measurement improves about2.93x in this fixed-work fixture; it performs
zero accepted edits. Faster rejection is not successful adaptation. The wedge
retains six accepted splits and its prior mesh. Parent timers include source/index
build, record migration/worker communicator/core/return; manufactured source
preparation, diagnostic output and independent audits are separate. Host contention
and phase detail remain in receipts/logs. No scaling, total CFD lifecycle or
unsteady affordability claim; modeled bytes are not RSS. This checkpoint did not
repeat a non-MPI build; the changed predicate is independent of MPI and MPI1
kernel controls ran, while the preceding actual non-MPI checkpoint remains retained.

Evidence is hash-closed and deduplicated against retained query-support artifacts;
no executables/objects/cache are published. Original inspectable grids remain in
query_support_sources_v3/mpi{1,2,4}_meshes; manifests resolve exact duplicates.
Private full-cavity cap, source2058 batching/streaming, finite coplanar geometry
patch authority, production3D guard, weighted partition/CFD/history rebuild/CGNS/
full lifecycle and cluster qualification remain unfinished. Goal1 is not complete.
