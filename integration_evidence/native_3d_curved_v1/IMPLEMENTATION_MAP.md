# Goal 2 initial implementation map

Base:23197eba1715f44d61c9ad3a070f61441dfb0560; branch codex/native-3d-curved-local.
Local-only, Goal ACTIVE. No production curved remeshing claim from the first study.

Four integrated steps:
1. Geometry authority plus coupled curved split/move/reconstruction, feature and
   component binding, geometric-error/clearance/orientation checks and persistence.
2. Explicit metric coverage before freezing: retain original P1 inside its domain,
   admit a bounded exterior extension with provenance; maintain actual-query
   composition and reject unsupported/out-of-envelope queries. No nearest-donor
   fallback in FrozenField and no sensor substitution for composed3DBL.
3. MPI-complete dependency/geometry/source admission and atomic publication, then
   actual transfer/rebuild/SU2CGNS/reference/restart on local MPI1/2/4 and M<N.
4. Small actual3DEuler solution-derived-metric cycles and solver continuation;
   independent mesh/geometry/metric/state/positivity/conservation/restart checks.

Connected code inspected:
- CNativeBoundary3D.hpp/cpp: PlanarReference exact finite-facet binding, complete
  edge/vertex-star split/collapse/move, private prescribed-skin validation.
- CNativeCavity3D.cpp: retains coordinates of old skin vertices present in replacement;
  a moved curved vertex needs its existing distinct new identity and caller-proved
  changed physical skin, not simply relaxed facet containment. Changed-skin callers
  must prove physical/domain closure; no arbitrary changed-skin certificate exists.
- CNativeField3D.hpp: original sensor-only P1 and exact source-segment integration;
  outside-domain queries reject. CNativeDonor3D and distributed query routing must
  admit any declared exterior donor coverage before private operations.
- CNativeReferenceIO3D.cpp: version1 persists finite planar facets only. Curved
  authority needs bounded explicit persistence with backward-compatible v1 decoding.
- CBarycentricTransfer.cpp: canonical marker-aware closest-face stencil and distance
  limits already support new points outside the donor mesh.
- CConservativeTransfer.cpp: MPI projection includes newly covered S_n and lost S_d
  regions, with positivity/distance/conservation diagnostics. Reuse this machinery;
  verify correspondence and histories rather than inventing another transfer.

First executable study: NativeCurvedCoverage3D in CNativeField3D_tests.cpp. Unit
sphere-octant edge projection leaves the initial tetrahedron; exact original P1
queries and edge integration reject. Explicit three-tet exterior shell with frozen
2Iboundary trace covers projection while original interior P1 remains bit-identical
and out-of-envelope queries still reject. This is a manually prescribed coverage
experiment, not a production padding algorithm, MPI geometry integration or a
solution to spatially varying/component-adjacent/anisotropic extension.
Radius tolerance1e-12R and integral relative check1e-12 are declared in the study.
Do not extrapolate its success into geometry/quality/general-domain capabilities.

No cluster/timing/scaling/large-wing or production3DBL qualification. Preserve
Goal1 source-only/interior-P1/finer-sensor/actual-query2DBL/fade/NOISE0/default/MMG.
Keep one owned heavy job, MPI<=4, threads1, nice19/oneCPU, build-j1; announce full
case folders and push source/proof checkpoints. Goal2 closes only after all4steps
have actual integrated local proofs, including real CFD-derived metrics and restart.

## Existing2D extension traced after user question

CNativeField2D.hpp FieldPatch.evaluate: exact original P1 first; separately counted
four-epsilon roundoff containment; then nearest original boundary-edge trace from
associated physical components within the declared extension_limit. Runtime passes
ADAP_HAUSD as that limit via CNativeEngine2D.hpp donor.import. MPI import uses2*limit
coverage padding while actual queries enforce one limit. The sensor is blended on
the boundary edge, and geometricBL composition uses actual query coordinates.
NativeField2D tests explicitly reject excessive distance and unrelated components.
PolylineReference preserves the original polyline, not an unprovided smooth curve.

3D direction: reuse bounded associated-boundary trace semantics with triangular
faces and explicit geometry/component provenance. Evaluate consistent edge lengths
through admitted exterior regions; do not add a Query fallback while leaving exact
original-source edge coverage unchanged. The manual exterior-tet study remains a
useful admission/integration control, not a chosen production padding scheme.
Existing barycentric/conservative transfer rules are a separate solution policy.

## Qualified first analytic geometry implementation

See ANALYTIC_GEOMETRY_CHECKPOINT.md and analytic_geometry_v3/validation.json.
Sphere/bump finite references, projected complete-edge-star split/inverse,
inversion rollback and bounded version2 persistence are implemented. Finite-corner
admission uses actual finite arc/edge distance, not independently widened planes.
Mixed curved/planar patch expansion, cylinder, chart-union reconnection, final
geometry/error/clearance logic and source coverage remain pending. Runtime rejects
analytic references until those contracts connect. This is partial progress toward
steps1–2, not completion of any of the four integrated requirements.
