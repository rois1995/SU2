# Affine runtime-reference source/backend checkpoint — full Goal3 OPEN

Full Goal3 and the general symmetry specification remain ACTIVE/incomplete.
This connected batch routes finite affine references through the existing3D
PlanarReference/import/owner-private source/operator/IO architecture. Geometry
algorithms do not dispatch on Euler/RANS; RANS adds requested BL constraints.

Implementation; qualification is limited to the explicit controls below:

- Frozen identity/version/origin/normal/error contract in native-ref version4;
  legacy versions1--3 retain their encoding. Full finite component queries and
  oriented physical labels persist. Prepared finite geometry and source-component
  association are retained per immutable reference object.
- Shared finite point/chord/face/projection and geometry-error dispatch, expanded
  conservative search boxes, resident/preparation admission and predicate counters.
- Actual-query source composition still caches sensor only. Original physical P1
  wins. Explicitly associated affine coverage copies original boundary-node sensor
  tensors into bounded dependent-coordinate prisms; targets are never interpolated
  from adapted nodes. Source IDs must retain the original-family modulo convention.
- Collective parameter/version checks and atomic refusal. Independent face collars
  do not establish simultaneous plane/plane or plane/curved-body junction coverage;
  those combinations currently reject explicitly, rather than publish possible gaps.
- A declared affine BL wall currently rejects explicitly; the BL provider must not
  silently substitute its raw faceted metric for the requested authority.

No arbitrary off-plane initial-adoption configuration is exposed. This backend
work begins with matching input geometry and a declared reference; it does not move
an existing state or replace an archived frozen donor field. Current finite core
uses straight footprint edges. Analytic intersection curves, general simultaneous
junctions, arbitrary2D runtime adoption and an exterior aerodynamic workflow remain
required and unimplemented. A valid reference does not prove construction success.
The saved annular native construction and independent feasible witness are retained;
there is no new M6 or annular run in this batch.

Executed validation: one focused serial import/codec/owner-private/priority/field
check with lifting audit, affected MPI controls and a current-binary Cartesian
accepted lifecycle for compatibility. These are distinct from full oblique
adaptation, transfer, output and real restart, which remain required.

Build uses one low-priority worker on a sampled CPU with scratch on4TB. Interrupted
build receipts are retained; that interruption batches the newly discovered BL
provider guard/control. No performance/scaling/affordability claim from these tests.

## Executed evidence

`serial_v3` PASS12 cases/3974 assertions, including actual initialized Euler and
RANS current+BDF import; `mpi4_v3` PASS5 cases/rank (155--635 assertions), including
version disagreement. Independent exact source audit PASS:48 physical,3 analytic,
24 affine stencils, original span27; all75 positive, exact original sensor trace,
independent v4 field/vertex audit and identical serial/MPI signatures. Supporting
fixture helpers match pre-build39c3d264f2 and are sealed separately.

Current-binary Cartesian `../planar_symmetry_reference_v1/workflow_v6_n4` passes
N4/M2 RANS/SA/current+BDF adaptation, output and real restart, independently audited.
It retains240/16 accepted cells for budgets20/3, conservative flow/BDF defects below
4.15e-16 and zero height/sensor/restart residuals. This establishes compatibility,
not an accepted oblique lifecycle. No standalone SU2_CFD executable rebuild is
claimed; checks execute current production libraries through UnitTests/test_driver.

The new affine allocation reserves three source tetrahedra per active declared
face, rather than twelve per volume cell; retained capacity is included in admission.
This removes a volume-based new-path reservation, not a measured event speedup.
The existing analytic allocator has not been redesigned. No CFD advance was run.

Preserved fixture failures: serial_v1 lacked BDF allocation; serial_v2 completed
Euler but its RANS options omitted Re. The targeted debugger pinpoints CConfig's
error. Corrected fixtures retain all state/geometry/metric gates. Build logs retain
the Sample/Query API error and deliberate BL-guard batching interruption.

Current replay: `python3 run.py 1 v4`, then `python3 run.py 4 v4`; explicit versions
keep qualified v3 evidence immutable. The updated launcher additionally pins fixture
helpers and verifies saved artifacts before reuse. It is not the exact v3 executed
launcher, which is retained in each source capsule. No extra solver rerun is needed
for those provenance-only additions. [Complete result/limits](report.json).

The affine private-operator projection routes are implemented but have no accepted
oblique proposal/lifecycle evidence yet. Import/query/codec success does not qualify
all reconstruction paths or resolve the saved annular blockage.
